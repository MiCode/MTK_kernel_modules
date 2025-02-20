// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/******************************************************************************
*[File]             hif_api.c
*[Version]          v1.0
*[Revision Date]    2015-09-08
*[Author]
*[Description]
*    The program provides USB HIF APIs
*[Copyright]
*    Copyright (C) 2015 MediaTek Incorporation. All Rights Reserved.
******************************************************************************/

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include "precomp.h"

#include <linux/usb.h>
#include <linux/mutex.h>

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include "mt66xx_reg.h"
#include "hal_wfsys_reset_mt7961.h"
#include "hif.h"

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

#define MAX_DUMP_CMD_LEN	(128)
#define MAX_DUMP_DATA_LEN	(128)

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static const uint16_t arTcToUSBEP[USB_TC_NUM] = {
	USB_DATA_BULK_OUT_EP4,
	USB_DATA_BULK_OUT_EP5,
	USB_DATA_BULK_OUT_EP6,
	USB_DATA_BULK_OUT_EP7,
	USB_DATA_BULK_OUT_EP8,
	USB_DATA_BULK_OUT_EP9,

	/* Second HW queue */
#if NIC_TX_ENABLE_SECOND_HW_QUEUE
	USB_DATA_BULK_OUT_EP9,
	USB_DATA_BULK_OUT_EP9,
	USB_DATA_BULK_OUT_EP9,
	USB_DATA_BULK_OUT_EP9,
#endif
};

static const uint8_t arDmashdlGrpToTc[USB_DMASHDL_DATA_GROUP_NUM] = {
	TC0_INDEX,
	TC1_INDEX,
	TC2_INDEX,
	TC3_INDEX,
	USB_DBDC1_TC,
};

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/*----------------------------------------------------------------------------*/
/*!
* @brief Verify the CHIP ID
*
* @param prAdapter      a pointer to adapter private data structure.
*
*
* @retval TRUE          CHIP ID is the same as the setting compiled
* @retval FALSE         CHIP ID is different from the setting compiled
*/
/*----------------------------------------------------------------------------*/
u_int8_t halVerifyChipID(struct ADAPTER *prAdapter)
{
	uint32_t u4CIR = 0;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	if (prAdapter->fgIsReadRevID)
		return TRUE;

	prChipInfo = prAdapter->chip_info;

	HAL_MCR_RD(prAdapter, prChipInfo->top_hcr, &u4CIR);
	DBGLOG(INIT, TRACE, "Chip ID: 0x%4x\n", u4CIR);

	if (u4CIR != prChipInfo->chip_id)
		return FALSE;

	HAL_MCR_RD(prAdapter, prChipInfo->top_hvr, &u4CIR);
	DBGLOG(INIT, TRACE, "Revision ID: 0x%4x\n", u4CIR);

	prAdapter->ucRevID = (uint8_t) (u4CIR & 0xF);
	prAdapter->fgIsReadRevID = TRUE;
	return TRUE;
}

uint32_t
halRxWaitResponse(struct ADAPTER *prAdapter,
		  uint8_t ucPortIdx, uint8_t *pucRspBuffer,
		  uint32_t u4MaxRespBufferLen, uint32_t *pu4Length,
		  uint32_t u4WaitingInterval, uint32_t u4TimeoutValue)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct RX_CTRL *prRxCtrl;
	u_int8_t ret = FALSE;
	struct BUS_INFO *prBusInfo;

	ASSERT(prAdapter);
	ASSERT(pucRspBuffer);

	prRxCtrl = &prAdapter->rRxCtrl;
	prBusInfo = prAdapter->chip_info->bus_info;

	if (prBusInfo->asicUsbEventEpDetected)
		ucPortIdx = prBusInfo->asicUsbEventEpDetected(prAdapter);
	else {
		if (prHifInfo->fgEventEpDetected == FALSE) {
			/* NOTE: This is temporary compatiable code with old/new CR4 FW to detect
			 *       which EVENT endpoint that.
			 *       CR4 FW is using. If the new EP4IN-using CR4 FW works without
			 *       any issue for a while,
			 *       this code block will be removed.
			 */
			if (prAdapter->fgIsCr4FwDownloaded) {
				ucPortIdx = USB_DATA_EP_IN;
				ret = kalDevPortRead(prAdapter->prGlueInfo, ucPortIdx,
					ALIGN_4(u4MaxRespBufferLen) + LEN_USB_RX_PADDING_CSO,
					prRxCtrl->pucRxCoalescingBufPtr,
					HIF_RX_COALESCING_BUFFER_SIZE,
					FALSE);

				if (ret == TRUE) {
					prHifInfo->eEventEpType = EVENT_EP_TYPE_DATA_EP;
				} else {
					ucPortIdx = USB_EVENT_EP_IN;
					ret = kalDevPortRead(prAdapter->prGlueInfo, ucPortIdx,
						ALIGN_4(u4MaxRespBufferLen) + LEN_USB_RX_PADDING_CSO,
						prRxCtrl->pucRxCoalescingBufPtr,
						HIF_RX_COALESCING_BUFFER_SIZE,
						FALSE);
				}
				prHifInfo->fgEventEpDetected = TRUE;

				kalMemCopy(pucRspBuffer, prRxCtrl->pucRxCoalescingBufPtr, u4MaxRespBufferLen);
				*pu4Length = u4MaxRespBufferLen;

				if (ret == FALSE)
					u4Status = WLAN_STATUS_FAILURE;
				return u4Status;
			}

			ucPortIdx = USB_EVENT_EP_IN;
		} else {
			if (prHifInfo->eEventEpType == EVENT_EP_TYPE_DATA_EP)
				if (prAdapter->fgIsCr4FwDownloaded)
					ucPortIdx = USB_DATA_EP_IN;
				else
					ucPortIdx = USB_EVENT_EP_IN;
			else
				ucPortIdx = USB_EVENT_EP_IN;
		}
	}
	ret = kalDevPortRead(prAdapter->prGlueInfo, ucPortIdx,
		ALIGN_4(u4MaxRespBufferLen) + LEN_USB_RX_PADDING_CSO,
		prRxCtrl->pucRxCoalescingBufPtr, HIF_RX_COALESCING_BUFFER_SIZE,
		FALSE);

	kalMemCopy(pucRspBuffer, prRxCtrl->pucRxCoalescingBufPtr, u4MaxRespBufferLen);
	*pu4Length = u4MaxRespBufferLen;

	if (ret == FALSE)
		u4Status = WLAN_STATUS_FAILURE;

	return u4Status;
}

uint32_t halTxUSBSendCmd(struct GLUE_INFO *prGlueInfo, uint8_t ucTc,
		struct CMD_INFO *prCmdInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint16_t u2OverallBufferLength = 0;
	unsigned long flags;
	uint8_t ucQueIdx;
	struct mt66xx_chip_info *prChipInfo;
	int ret;
	struct TX_DESC_OPS_T *prTxDescOps;

	if (!(prHifInfo->state == USB_STATE_LINK_UP ||
		prHifInfo->state == USB_STATE_PRE_RESUME ||
		prHifInfo->state == USB_STATE_PRE_SUSPEND))
		return WLAN_STATUS_FAILURE;

	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxCmdFreeQ, &prHifInfo->rTxCmdQLock);
	if (prUsbReq == NULL) {
		DBGLOG(HAL, ERROR, "TX CMD CID[0x%X] SEQ[%d] no URB!\n",
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		return WLAN_STATUS_RESOURCES;
	}

	prBufCtrl = prUsbReq->prBufCtrl;

	if ((TFCB_FRAME_PAD_TO_DW(prCmdInfo->u4TxdLen + prCmdInfo->u4TxpLen) + LEN_USB_UDMA_TX_TERMINATOR) >
	    prBufCtrl->u4BufSize) {
		DBGLOG(HAL, ERROR, "TX CMD CID[0x%X] buffer underflow!\n",
		       prCmdInfo->ucCID);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxCmdFreeQ, prUsbReq,
				&prHifInfo->rTxCmdQLock, FALSE);
		return WLAN_STATUS_RESOURCES;
	}

	DBGLOG(HAL, INFO, "TX CMD CID[0x%X] URB[0x%p] SEQ[%d]\n",
			prCmdInfo->ucCID,
			prUsbReq->prUrb, prCmdInfo->ucCmdSeqNum);

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prTxDescOps = prChipInfo->prTxDescOps;
	HAL_WRITE_HIF_TXD(prChipInfo, prBufCtrl->pucBuf,
			(prCmdInfo->u4TxdLen + prCmdInfo->u4TxpLen),
				TXD_PKT_FORMAT_COMMAND);
	u2OverallBufferLength += prChipInfo->u2HifTxdSize;

	if (prCmdInfo->u4TxdLen) {
		memcpy((prBufCtrl->pucBuf + u2OverallBufferLength), prCmdInfo->pucTxd, prCmdInfo->u4TxdLen);
		u2OverallBufferLength += prCmdInfo->u4TxdLen;
	}

	if (prCmdInfo->u4TxpLen) {
		memcpy((prBufCtrl->pucBuf + u2OverallBufferLength), prCmdInfo->pucTxp, prCmdInfo->u4TxpLen);
		u2OverallBufferLength += prCmdInfo->u4TxpLen;
	}

	if (prChipInfo->is_support_cr4 && prTxDescOps->nic_txd_queue_idx_op) {
		void *prTxDesc = (void *)(prBufCtrl->pucBuf
					+ prChipInfo->u2HifTxdSize);
		ucQueIdx = prTxDescOps->nic_txd_queue_idx_op(
			prTxDesc, 0, FALSE);
		/* For H2CDMA Tx CMD mapping
		 * Mapping port1 queue0~3 to queue28~31,
		 * and CR4 will unmask this.
		 */
		prTxDescOps->nic_txd_queue_idx_op(
			prTxDesc,
			(ucQueIdx | USB_TX_CMD_QUEUE_MASK),
			TRUE);
	}

	/* DBGLOG_MEM32(SW4, INFO, prBufCtrl->pucBuf, 32); */
	memset(prBufCtrl->pucBuf + u2OverallBufferLength, 0,
	       ((TFCB_FRAME_PAD_TO_DW(u2OverallBufferLength) - u2OverallBufferLength) + LEN_USB_UDMA_TX_TERMINATOR));
	prBufCtrl->u4WrIdx = TFCB_FRAME_PAD_TO_DW(u2OverallBufferLength) + LEN_USB_UDMA_TX_TERMINATOR;

	if (prAdapter->rWifiVar.fgDumpTxD) {
		uint32_t dump_len = 0;

		if (prBufCtrl->u4WrIdx > MAX_DUMP_CMD_LEN)
			dump_len = MAX_DUMP_CMD_LEN;
		else
			dump_len = prBufCtrl->u4WrIdx;
		DBGLOG(HAL, INFO, "Dump CMD TXD: (total length: %d)\n",
		       prBufCtrl->u4WrIdx);
		dumpMemory8(prBufCtrl->pucBuf, dump_len);
	}

	prUsbReq->prPriv = (void *) prCmdInfo;
	usb_fill_bulk_urb(prUsbReq->prUrb,
			  prHifInfo->udev,
			  usb_sndbulkpipe(prHifInfo->udev, arTcToUSBEP[ucTc]),
			  (void *)prUsbReq->prBufCtrl->pucBuf,
			  prBufCtrl->u4WrIdx, halTxUSBSendCmdComplete, (void *)prUsbReq);

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	ret = glUsbSubmitUrb(prHifInfo, prUsbReq->prUrb, SUBMIT_TYPE_TX_CMD);
	if (ret) {
		DBGLOG(HAL, ERROR,
			"glUsbSubmitUrb() reports error (%d) [%s] (EP%d OUT)\n",
			ret, __func__, arTcToUSBEP[ucTc]);
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxCmdFreeQ);
		spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);
		return WLAN_STATUS_FAILURE;
	}
	list_add_tail(&prUsbReq->list, &prHifInfo->rTxCmdSendingQ);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	if (wlanIsChipRstRecEnabled(prGlueInfo->prAdapter)
			&& wlanIsChipNoAck(prGlueInfo->prAdapter)) {
		wlanChipRstPreAct(prGlueInfo->prAdapter);
		DBGLOG(HAL, ERROR, "usb trigger whole reset\n");
		HAL_WIFI_FUNC_CHIP_RESET(prGlueInfo->prAdapter);
	}
	return u4Status;
}

void halTxUSBSendCmdComplete(struct urb *urb)
{
	struct USB_REQ *prUsbReq = urb->context;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;
	unsigned long flags;

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_del_init(&prUsbReq->list);
	list_add_tail(&prUsbReq->list, &prHifInfo->rTxCmdCompleteQ);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	kalSetIntEvent(prGlueInfo);
#else
	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_del_init(&prUsbReq->list);
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	halTxUSBProcessCmdComplete(prGlueInfo->prAdapter, prUsbReq);
#endif
}

void halTxUSBProcessCmdComplete(struct ADAPTER *prAdapter,
		struct USB_REQ *prUsbReq)
{
	struct urb *urb = prUsbReq->prUrb;
	uint32_t u4SentDataSize;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;

	if (urb->status != 0) {
		DBGLOG(TX, ERROR, "[%s] send CMD fail (status = %d)\n", __func__, urb->status);
		/* TODO: handle error */
	}

	DBGLOG(HAL, INFO, "TX CMD DONE: URB[0x%p]\n", urb);

	glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxCmdFreeQ, prUsbReq, &prHifInfo->rTxCmdQLock, FALSE);

	u4SentDataSize = urb->actual_length - LEN_USB_UDMA_TX_TERMINATOR;
	nicTxReleaseResource_PSE(prAdapter, TC4_INDEX,
		halTxGetCmdPageCount(prAdapter, u4SentDataSize, TRUE), TRUE);
}

void halTxCancelSendingCmd(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo)
{
	struct USB_REQ *prUsbReq, *prNext;
	unsigned long flags;
	struct urb *urb = NULL;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_for_each_entry_safe(prUsbReq, prNext, &prHifInfo->rTxCmdSendingQ, list) {
		if (prUsbReq->prPriv == (void *) prCmdInfo) {
			list_del_init(&prUsbReq->list);
			urb = prUsbReq->prUrb;
			break;
		}
	}
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);

	if (urb) {
		prCmdInfo->pfHifTxCmdDoneCb = NULL;
		usb_kill_urb(urb);
	}
}

void halTxCancelAllSending(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct USB_REQ *prUsbReq, *prUsbReqNext;
	struct GL_HIF_INFO *prHifInfo;
	uint8_t ucTc;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdSendingQ, list) {
		usb_kill_urb(prUsbReq->prUrb);
	}

	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc)
		usb_kill_anchored_urbs(&prHifInfo->rTxDataAnchor[ucTc]);
}

void halCancelTxRx(struct ADAPTER *prAdapter)
{
	/* stop TX BULK OUT URB */
	halTxCancelAllSending(prAdapter);

	/* stop RX BULK IN URB */
	halDisableInterrupt(prAdapter);
}

#if CFG_CHIP_RESET_SUPPORT
uint32_t halToggleWfsysRst(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct CHIP_DBG_OPS *prChipDbg;
	uint32_t u4CrVal;
	u_int8_t fgStatus;

#define CONN_SEMA00_M0_OWN_STA        0x18070000
#define CONN_SEMA_OWN_BY_M0_STA_REP_1 0x18070400

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prChipDbg = prChipInfo->prDebugOps;

	/* Although we've already executed show_mcu_debug_info() in
	 * glResetTriggerCommon(), it may not be a successful operation at that
	 * time in USB mode due to the context is soft_irq which forbids the
	 * execution of usb_control_msg(). Since we don't prefer to refactor it
	 * at this stage, this is a workaround by performing it again here
	 * where the context is kernel thread from workqueue.
	 */
	if (prChipDbg->show_mcu_debug_info)
		prChipDbg->show_mcu_debug_info(prAdapter, NULL, 0,
					       DBG_MCU_DBG_ALL, NULL);

	/* set USB EP_RST_OPT as reset scope excludes toggle bit,
	 * sequence number, etc.
	 */
	if (prBusInfo->asicUsbEpctlRstOpt)
		prBusInfo->asicUsbEpctlRstOpt(prAdapter, FALSE);

	HAL_UHW_RD(prAdapter, CONN_SEMA00_M0_OWN_STA, &u4CrVal, &fgStatus);
	DBGLOG(HAL, INFO, "Read CONN_SEMA00_M0_OWN_STA: 0x%x\n", u4CrVal);

	/* assert WF L0.5 reset */
	if (prChipInfo->asicWfsysRst)
		prChipInfo->asicWfsysRst(prAdapter, TRUE);

	/* wait 2 ticks of 32K */
	kalMsleep(20);

	/* de-assert WF L0.5 reset */
	if (prChipInfo->asicWfsysRst)
		prChipInfo->asicWfsysRst(prAdapter, FALSE);

	if (prChipInfo->asicPollWfsysSwInitDone)
		if (!prChipInfo->asicPollWfsysSwInitDone(prAdapter)) {
			DBGLOG(HAL, ERROR,
			       "L0.5 reset polling sw init done fail\n");
			return WLAN_STATUS_FAILURE;
		}

	HAL_UHW_RD(prAdapter, CONN_SEMA_OWN_BY_M0_STA_REP_1,
		&u4CrVal, &fgStatus);
	DBGLOG(HAL, INFO, "Read CONN_SEMA_OWN_BY_M0_STA_REP_1: 0x%x\n",
		u4CrVal);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_CHIP_RESET_SUPPORT */

uint32_t halTxUSBSendAggData(struct GL_HIF_INFO *prHifInfo, uint8_t ucTc,
		struct USB_REQ *prUsbReq)
{
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;
	struct BUF_CTRL *prBufCtrl = prUsbReq->prBufCtrl;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	int ret;

	memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, LEN_USB_UDMA_TX_TERMINATOR);
	prBufCtrl->u4WrIdx += LEN_USB_UDMA_TX_TERMINATOR;

	if (prHifInfo->state != USB_STATE_LINK_UP) {
		/* No need to dequeue prUsbReq because LINK is not up */
		prBufCtrl->u4WrIdx = 0;
		return WLAN_STATUS_FAILURE;
	}

	list_del_init(&prUsbReq->list);

	usb_fill_bulk_urb(prUsbReq->prUrb,
			  prHifInfo->udev,
			  usb_sndbulkpipe(prHifInfo->udev, arTcToUSBEP[ucTc]),
			  (void *)prBufCtrl->pucBuf, prBufCtrl->u4WrIdx, halTxUSBSendDataComplete, (void *)prUsbReq);

	usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rTxDataAnchor[ucTc]);
	ret = glUsbSubmitUrb(prHifInfo, prUsbReq->prUrb, SUBMIT_TYPE_TX_DATA);
	if (ret) {
		DBGLOG(HAL, ERROR,
			"glUsbSubmitUrb() reports error (%d) [%s] (EP%d OUT)\n",
			ret, __func__, arTcToUSBEP[ucTc]);
		halTxUSBProcessMsduDone(prGlueInfo, prUsbReq);
		prBufCtrl->u4WrIdx = 0;
		usb_unanchor_urb(prUsbReq->prUrb);
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataCompleteQ);
#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
		kalSetIntEvent(prGlueInfo);
#else
		tasklet_schedule(&prGlueInfo->rTxCompleteTask);
#endif
		return WLAN_STATUS_FAILURE;
	}

	return u4Status;
}

static uint8_t halUsbDetermineTc(struct mt66xx_chip_info *prChipInfo,
				struct MSDU_INFO *prMsduInfo)
{
	uint8_t ucTc, ucDmashdlTc;
	uint8_t *pucBuf;
	uint8_t ucQueIdx, ucGrpIdx;
	struct TX_DESC_OPS_T *prTxDescOps;
	struct DMASHDL_CFG *prCfg = prChipInfo->bus_info->prDmashdlCfg;

	pucBuf = ((struct sk_buff *)prMsduInfo->prPacket)->data;
	prTxDescOps = prChipInfo->prTxDescOps;
	ucQueIdx = prTxDescOps->nic_txd_queue_idx_op(
		pucBuf, 0, FALSE);

	ucTc = (prMsduInfo->ucWmmQueSet) ? USB_DBDC1_TC : prMsduInfo->ucTC;

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	if (prMsduInfo->fgMgmtUseDataQ)
		ucTc = prMsduInfo->ucTC;
#endif

	ucGrpIdx = prCfg->aucQueue2Group[ucQueIdx];
	if (ucGrpIdx < USB_DMASHDL_DATA_GROUP_NUM) {
		ucDmashdlTc = arDmashdlGrpToTc[ucGrpIdx];
		if (ucTc != ucDmashdlTc) {
			DBGLOG(HAL, INFO, "ucTc mismatch! (%d != %d)\n", ucTc,
			       ucDmashdlTc);

			return ucDmashdlTc;
		}
	} else {
		DBGLOG(HAL, WARN, "unexpected DMASHDL group number: %d",
		       ucGrpIdx);
	}

	return ucTc;
}

uint32_t halTxUSBSendData(struct GLUE_INFO *prGlueInfo,
		struct MSDU_INFO *prMsduInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint32_t u4PaddingLength;
	struct sk_buff *skb;
	uint8_t ucTc;
	uint8_t *pucBuf;
	uint32_t u4Length;
	uint32_t u4TotalLen;
	unsigned long flags;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	skb = (struct sk_buff *)prMsduInfo->prPacket;
	pucBuf = skb->data;
	u4Length = skb->len;
	u4TotalLen = u4Length + prChipInfo->u2HifTxdSize;
	ucTc = halUsbDetermineTc(prChipInfo, prMsduInfo);
#if (CFG_SUPPORT_DMASHDL_SYSDVT)
	if (prMsduInfo->ucPktType == ENUM_PKT_ICMP) {
		if (DMASHDL_DVT_QUEUE_MAPPING_TYPE1(prGlueInfo->prAdapter)
		|| DMASHDL_DVT_QUEUE_MAPPING_TYPE2(prGlueInfo->prAdapter)) {
			/* send ping packets to each EP for DMASHDL DVT */
			ucTc = prMsduInfo->ucTarQueue % TC_NUM;
			/* skip TC4, TC4=>EP8 is reserved for CMD */
			if (ucTc == TC4_INDEX)
				ucTc = TC_NUM;
			DMASHDL_DVT_INC_PING_PKT_CNT(prGlueInfo->prAdapter,
				prMsduInfo->ucTarQueue);
		}
	}
#endif /* CFG_SUPPORT_DMASHDL_SYSDVT */

	spin_lock_irqsave(&prHifInfo->rTxDataQLock, flags);

	if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc])) {
		if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
			spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);
			DBGLOG(HAL, ERROR, "run out of rTxDataFreeQ #1!!\n");
			wlanProcessQueuedMsduInfo(prGlueInfo->prAdapter, prMsduInfo);
			return WLAN_STATUS_RESOURCES;
		}
	}
	prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
	prBufCtrl = prUsbReq->prBufCtrl;

	if (prHifInfo->u4AggRsvSize[ucTc] < ALIGN_4(u4TotalLen))
		DBGLOG(HAL, ERROR, "u4AggRsvSize[%hhu] count FAIL (%u, %u)\n",
		       ucTc, prHifInfo->u4AggRsvSize[ucTc], u4TotalLen);
	prHifInfo->u4AggRsvSize[ucTc] -= ALIGN_4(u4TotalLen);

	if (prBufCtrl->u4WrIdx + ALIGN_4(u4TotalLen) + LEN_USB_UDMA_TX_TERMINATOR > prBufCtrl->u4BufSize) {
		halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);

		if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc])) {
			if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
				spin_unlock_irqrestore(&prHifInfo->rTxDataQLock,
							flags);
				DBGLOG(HAL, ERROR, "run out of rTxDataFreeQ #2!!\n");
				wlanProcessQueuedMsduInfo(prGlueInfo->prAdapter, prMsduInfo);
				return WLAN_STATUS_FAILURE;
			}
		}

		prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
		prBufCtrl = prUsbReq->prBufCtrl;
	}

	HAL_WRITE_HIF_TXD(prChipInfo, prBufCtrl->pucBuf + prBufCtrl->u4WrIdx,
				u4Length, TXD_PKT_FORMAT_TXD_PAYLOAD);
	prBufCtrl->u4WrIdx += prChipInfo->u2HifTxdSize;
	memcpy(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, pucBuf, u4Length);
	prBufCtrl->u4WrIdx += u4Length;

	u4PaddingLength = (ALIGN_4(u4TotalLen) - u4TotalLen);
	if (u4PaddingLength) {
		memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, u4PaddingLength);
		prBufCtrl->u4WrIdx += u4PaddingLength;
	}

	if (prAdapter->rWifiVar.fgDumpTxD) {
		uint32_t dump_len = 0;

		if (u4Length > MAX_DUMP_DATA_LEN)
			dump_len = MAX_DUMP_DATA_LEN;
		else
			dump_len = u4Length;
		DBGLOG(HAL, INFO, "Dump DATA TXD: (total length: %d)\n",
		       u4Length);
		dumpMemory8(pucBuf, dump_len);
	}

	if (!prMsduInfo->pfTxDoneHandler)
		QUEUE_INSERT_TAIL(&prUsbReq->rSendingDataMsduInfoList,
				prMsduInfo);

	if (usb_anchor_empty(&prHifInfo->rTxDataAnchor[ucTc]))
		halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);

	spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);

	if (wlanIsChipRstRecEnabled(prGlueInfo->prAdapter)
			&& wlanIsChipNoAck(prGlueInfo->prAdapter)) {
		wlanChipRstPreAct(prGlueInfo->prAdapter);
		DBGLOG(HAL, ERROR, "usb trigger whole reset\n");
		HAL_WIFI_FUNC_CHIP_RESET(prGlueInfo->prAdapter);
	}
	return u4Status;
}

uint32_t halTxUSBKickData(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint8_t ucTc;
	unsigned long flags;

	spin_lock_irqsave(&prHifInfo->rTxDataQLock, flags);

	for (ucTc = TC0_INDEX; ucTc < USB_TC_NUM; ucTc++) {
		if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc]))
			continue;

		prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
		prBufCtrl = prUsbReq->prBufCtrl;

		if (prBufCtrl->u4WrIdx)
			halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);
	}

	spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);

	return WLAN_STATUS_SUCCESS;
}

void halTxUSBSendDataComplete(struct urb *urb)
{
	struct USB_REQ *prUsbReq = urb->context;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;

	glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ, prUsbReq, &prHifInfo->rTxDataQLock, FALSE);

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	kalSetIntEvent(prGlueInfo);
#else
	tasklet_schedule(&prGlueInfo->rTxCompleteTask);
#endif
}

void halTxUSBProcessMsduDone(struct GLUE_INFO *prGlueInfo,
		struct USB_REQ *prUsbReq)
{
	uint8_t ucTc;
	struct QUE rFreeQueue;
	struct QUE *prFreeQueue;
	struct urb *urb = prUsbReq->prUrb;
	uint32_t u4SentDataSize;

	ucTc = *((uint8_t *)&prUsbReq->prPriv) & TC_MASK;

	prFreeQueue = &rFreeQueue;
	QUEUE_INITIALIZE(prFreeQueue);
	QUEUE_MOVE_ALL((prFreeQueue), (&(prUsbReq->rSendingDataMsduInfoList)));
	if (g_pfTxDataDoneCb)
		g_pfTxDataDoneCb(prGlueInfo, prFreeQueue);

	u4SentDataSize = urb->actual_length - LEN_USB_UDMA_TX_TERMINATOR;
	nicTxReleaseResource_PSE(prGlueInfo->prAdapter, ucTc,
		halTxGetDataPageCount(prGlueInfo->prAdapter, u4SentDataSize,
			TRUE), TRUE);
}

void halTxUSBProcessDataComplete(struct ADAPTER *prAdapter,
		struct USB_REQ *prUsbReq)
{
	uint8_t ucTc;
	u_int8_t fgFfa;
	struct urb *urb = prUsbReq->prUrb;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct BUF_CTRL *prBufCtrl = prUsbReq->prBufCtrl;
	unsigned long flags;

	ucTc = *((uint8_t *)&prUsbReq->prPriv) & TC_MASK;
	fgFfa =  *((uint8_t *)&prUsbReq->prPriv) & FFA_MASK;

	if (urb->status != 0) {
		DBGLOG(TX, ERROR, "[%s] send DATA fail (status = %d)\n", __func__, urb->status);
		/* TODO: handle error */
	}

	halTxUSBProcessMsduDone(prAdapter->prGlueInfo, prUsbReq);

	spin_lock_irqsave(&prHifInfo->rTxDataQLock, flags);
	prBufCtrl->u4WrIdx = 0;

	if ((fgFfa == FALSE) || list_empty(&prHifInfo->rTxDataFreeQ[ucTc]))
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataFreeQ[ucTc]);
	else
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataFfaQ);

	if (usb_anchor_empty(&prHifInfo->rTxDataAnchor[ucTc])) {
		prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
		prBufCtrl = prUsbReq->prBufCtrl;

		if (prBufCtrl->u4WrIdx != 0)
			halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);	/* TODO */
	}
	spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);

	if (!HAL_IS_TX_DIRECT(prAdapter)) {
		if (kalGetTxPendingCmdCount(prAdapter->prGlueInfo) > 0 || wlanGetTxPendingFrameCount(prAdapter) > 0)
			kalSetEvent(prAdapter->prGlueInfo);
#if CFG_SUPPORT_MULTITHREAD
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
#endif
	}
}

uint32_t halRxUSBEnqueueRFB(
	struct ADAPTER *prAdapter,
	uint8_t *pucBuf,
	uint32_t u4Length,
	uint32_t u4MinRfbCnt,
	struct list_head *prCompleteQ)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	struct SW_RFB *prSwRfb = (struct SW_RFB *) NULL;
	void *prRxStatus;
	uint32_t u4RemainCount;
	uint16_t u2RxByteCount;
	uint8_t *pucRxFrame;
	uint32_t u4EnqCnt = 0;
	struct BUS_INFO *prBusInfo;
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	uint32_t *pu4HwAppendDW;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
	struct RX_DESC_OPS_T *prRxDescOps;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prRxDescOps = prChipInfo->prRxDescOps;
	ASSERT(prRxDescOps->nic_rxd_get_rx_byte_count);
	ASSERT(prRxDescOps->nic_rxd_get_pkt_type);

	pucRxFrame = pucBuf;
	u4RemainCount = u4Length;
	while (u4RemainCount > 4) {
		/*
		 * For different align support.
		 * Ex. We need to do 8byte align for 7915u.
		 */
		if (prBusInfo->asicUsbRxByteCount)
			u2RxByteCount = prBusInfo->asicUsbRxByteCount(prAdapter,
				prBusInfo, pucRxFrame);
		else {
			u2RxByteCount =
				prRxDescOps->nic_rxd_get_rx_byte_count(
								pucRxFrame);
			u2RxByteCount = ALIGN_4(u2RxByteCount)
				+ LEN_USB_RX_PADDING_CSO;
		}

		if (u2RxByteCount <= CFG_RX_MAX_PKT_SIZE) {
			prSwRfb = NULL;
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
			if (prRxCtrl->rFreeSwRfbList.u4NumElem > u4MinRfbCnt)
				QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb, struct SW_RFB *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

			if (!prSwRfb)
				return u4Length - u4RemainCount;

			kalMemCopy(prSwRfb->pucRecvBuff, pucRxFrame, u2RxByteCount);

			prRxStatus = prSwRfb->prRxStatus;
			ASSERT(prRxStatus);

			prSwRfb->ucPacketType =
				prRxDescOps->nic_rxd_get_pkt_type(prRxStatus);
			/* DBGLOG(RX, TRACE, ("ucPacketType = %d\n", prSwRfb->ucPacketType)); */
#if CFG_TCP_IP_CHKSUM_OFFLOAD
			pu4HwAppendDW = (uint32_t *) prRxStatus;
			pu4HwAppendDW +=
				(ALIGN_4(u2RxByteCount - LEN_USB_RX_PADDING_CSO)
									>> 2);
			prSwRfb->u4TcpUdpIpCksStatus = *pu4HwAppendDW;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

			if (HAL_IS_RX_DIRECT(prAdapter)) {
				switch (prSwRfb->ucPacketType) {
				case RX_PKT_TYPE_RX_DATA:
#if CFG_SUPPORT_RX_NAPI
					if (prGlueInfo->prRxDirectNapi &&
						KAL_FIFO_IN(&prGlueInfo
							->rRxKfifoQ, prSwRfb)) {
						kalNapiSchedule(prAdapter);
					} else
#endif
					{
						spin_lock_bh(&prGlueInfo
							->rSpinLock
							[SPIN_LOCK_RX_DIRECT]);
						nicRxProcessDataPacket(
							prAdapter, prSwRfb);
						spin_unlock_bh(&prGlueInfo
							->rSpinLock
							[SPIN_LOCK_RX_DIRECT]);
					}
					break;
				default:
					KAL_ACQUIRE_SPIN_LOCK(prAdapter,
							SPIN_LOCK_RX_QUE);
					QUEUE_INSERT_TAIL(
						&prRxCtrl->rReceivedRfbList,
						&prSwRfb->rQueEntry);
					KAL_RELEASE_SPIN_LOCK(prAdapter,
							SPIN_LOCK_RX_QUE);
					u4EnqCnt++;
					break;
				}
			} else {
				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
						SPIN_LOCK_RX_QUE);
				QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList,
						&prSwRfb->rQueEntry);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
						SPIN_LOCK_RX_QUE);
				u4EnqCnt++;
			}
			RX_INC_CNT(prRxCtrl, RX_MPDU_TOTAL_COUNT);
		} else {
			DBGLOG(RX, WARN, "Rx byte count:%u exceeds SW_RFB max length:%u\n!",
				u2RxByteCount, CFG_RX_MAX_PKT_SIZE);
			DBGLOG_MEM32(RX, WARN, pucRxFrame,
				     prChipInfo->rxd_size);
			break;
		}

		u4RemainCount -= u2RxByteCount;
		pucRxFrame += u2RxByteCount;
	}

	if (u4EnqCnt) {
		set_bit(GLUE_FLAG_RX_BIT, &(prGlueInfo->ulFlag));
		wake_up_interruptible(&(prGlueInfo->waitq));
	}
	return u4Length;
}

uint32_t halRxUSBReceiveEvent(struct ADAPTER *prAdapter, u_int8_t fgFillUrb)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	int ret;

	while (1) {
		if (prAdapter == NULL ||
		    GLUE_GET_REF_CNT(prAdapter->fgIsIntEnable) == 0)
			break;

		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, &prHifInfo->rRxEventQLock);
		if (prUsbReq == NULL)
			return WLAN_STATUS_RESOURCES;

		usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rRxEventAnchor);

		prUsbReq->prBufCtrl->u4ReadSize = 0;
		if (prHifInfo->eEventEpType == EVENT_EP_TYPE_INTR && fgFillUrb) {
			usb_fill_int_urb(prUsbReq->prUrb,
				prHifInfo->udev,
				usb_rcvintpipe(prHifInfo->udev,
						USB_EVENT_EP_IN),
				(void *)prUsbReq->prBufCtrl->pucBuf,
				prUsbReq->prBufCtrl->u4BufSize,
				halRxUSBReceiveEventComplete,
				(void *)prUsbReq,
				1);
		} else if (prHifInfo->eEventEpType == EVENT_EP_TYPE_BULK) {
			usb_fill_bulk_urb(prUsbReq->prUrb,
				prHifInfo->udev,
				usb_rcvbulkpipe(prHifInfo->udev,
							USB_EVENT_EP_IN),
				(void *)prUsbReq->prBufCtrl->pucBuf,
				prUsbReq->prBufCtrl->u4BufSize,
				halRxUSBReceiveEventComplete,
				(void *)prUsbReq);
		}
		ret = glUsbSubmitUrb(prHifInfo, prUsbReq->prUrb,
					SUBMIT_TYPE_RX_EVENT);
		if (ret) {
			DBGLOG(HAL, ERROR,
				"glUsbSubmitUrb() reports error (%d) [%s] (EP%d IN)\n",
				ret, __func__, (USB_EVENT_EP_IN & 0x0F));
			usb_unanchor_urb(prUsbReq->prUrb);
			glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq,
					&prHifInfo->rRxEventQLock, FALSE);
			break;
		}
	}

	return u4Status;
}

void halRxUSBReceiveEventComplete(struct urb *urb)
{
	struct USB_REQ *prUsbReq = urb->context;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;

	if (!(prHifInfo->state == USB_STATE_LINK_UP ||
			prHifInfo->state == USB_STATE_PRE_RESUME ||
			prHifInfo->state == USB_STATE_PRE_SUSPEND)) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
		return;
	}

	/* Hif power off wifi, drop rx packets and continue polling RX packets until RX path empty */
	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
		halRxUSBReceiveEvent(prGlueInfo->prAdapter, FALSE);
		return;
	}

	if (urb->status == -ESHUTDOWN || urb->status == -ENOENT) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
		DBGLOG(RX, ERROR, "USB device shutdown skip Rx [%s]\n", __func__);
		return;
	}

#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	DBGLOG(RX, TRACE, "[%s] Rx URB[0x%p] Len[%u] Sts[%u]\n", __func__, urb, urb->actual_length, urb->status);

	glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventCompleteQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);

	kalSetIntEvent(prGlueInfo);
#else
	if (urb->status == 0) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventCompleteQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);

		tasklet_schedule(&prGlueInfo->rRxTask);
	} else {
		DBGLOG(RX, ERROR, "[%s] receive EVENT fail (status = %d)\n", __func__, urb->status);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);

		halRxUSBReceiveEvent(prGlueInfo->prAdapter, FALSE);
	}
#endif
}

#if CFG_CHIP_RESET_SUPPORT
uint32_t halRxUSBReceiveWdt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	int ret;

	while (1) {
		if (prAdapter == NULL ||
		    GLUE_GET_REF_CNT(prAdapter->fgIsIntEnable) == 0)
			break;

		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rRxWdtFreeQ,
					   &prHifInfo->rRxWdtQLock);
		if (prUsbReq == NULL)
			return WLAN_STATUS_RESOURCES;

		usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rRxWdtAnchor);

		prUsbReq->prBufCtrl->u4ReadSize = 0;

		usb_fill_int_urb(prUsbReq->prUrb,
				 prHifInfo->udev,
				 usb_rcvintpipe(prHifInfo->udev,
						USB_WDT_EP_IN),
				 (void *)prUsbReq->prBufCtrl->pucBuf,
				 prUsbReq->prBufCtrl->u4BufSize,
				 halRxUSBReceiveWdtComplete,
				 (void *)prUsbReq,
				 1);

		ret = glUsbSubmitUrb(prHifInfo, prUsbReq->prUrb,
				     SUBMIT_TYPE_RX_WDT);
		if (ret) {
			DBGLOG(HAL, ERROR,
			       "glUsbSubmitUrb() reports error (%d)\n", ret);

			usb_unanchor_urb(prUsbReq->prUrb);
			glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxWdtFreeQ,
					prUsbReq, &prHifInfo->rRxWdtQLock,
					FALSE);
			break;
		}
	}

	return u4Status;
}

void halRxUSBReceiveWdtComplete(struct urb *urb)
{
	struct USB_REQ *prUsbReq = urb->context;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (!(prHifInfo->state == USB_STATE_LINK_UP ||
			prHifInfo->state == USB_STATE_PRE_RESUME ||
			prHifInfo->state == USB_STATE_PRE_SUSPEND)) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxWdtFreeQ, prUsbReq,
						&prHifInfo->rRxWdtQLock, FALSE);
		return;
	}

	if (urb->status == -ESHUTDOWN || urb->status == -ENOENT) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxWdtFreeQ, prUsbReq,
						&prHifInfo->rRxWdtQLock, FALSE);
		DBGLOG(RX, ERROR, "USB device shutdown skip Rx\n");
		return;
	}

	if (urb->status == 0) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxWdtCompleteQ,
				prUsbReq, &prHifInfo->rRxWdtQLock, FALSE);

		tasklet_schedule(&prGlueInfo->rRxTask);
	} else {
		DBGLOG(RX, ERROR, "receive WDT fail (status = %d)\n",
		       urb->status);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxWdtFreeQ, prUsbReq,
				&prHifInfo->rRxWdtQLock, FALSE);

		halRxUSBReceiveWdt(prGlueInfo->prAdapter);
	}
}
#endif /* CFG_CHIP_RESET_SUPPORT */

uint32_t halRxUSBReceiveData(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct USB_REQ *prUsbReq;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	int ret;

	if (!prAdapter) {
		DBGLOG(INIT, WARN, "Adapter NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	prGlueInfo = prAdapter->prGlueInfo;

	if (!prGlueInfo) {
		DBGLOG(INIT, WARN, "GlueInfo NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	prHifInfo = &prGlueInfo->rHifInfo;

	while (1) {
		if (prAdapter == NULL ||
		    GLUE_GET_REF_CNT(prAdapter->fgIsIntEnable) == 0)
			break;

		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, &prHifInfo->rRxDataQLock);
		if (prUsbReq == NULL)
			return WLAN_STATUS_RESOURCES;

		usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rRxDataAnchor);

		prUsbReq->prBufCtrl->u4ReadSize = 0;
		usb_fill_bulk_urb(prUsbReq->prUrb,
				  prHifInfo->udev,
				  usb_rcvbulkpipe(prHifInfo->udev, USB_DATA_EP_IN),
				  (void *)prUsbReq->prBufCtrl->pucBuf,
				  prUsbReq->prBufCtrl->u4BufSize, halRxUSBReceiveDataComplete, (void *)prUsbReq);
		ret = glUsbSubmitUrb(prHifInfo, prUsbReq->prUrb,
					SUBMIT_TYPE_RX_DATA);
		if (ret) {
			DBGLOG(HAL, ERROR,
				"glUsbSubmitUrb() reports error (%d) [%s] (EP%d IN)\n",
				ret, __func__, (USB_EVENT_EP_IN & 0x0F));
			usb_unanchor_urb(prUsbReq->prUrb);
			glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);
			break;
		}
	}

	return u4Status;
}

void halRxUSBReceiveDataComplete(struct urb *urb)
{
	struct USB_REQ *prUsbReq = urb->context;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;

	if (!(prHifInfo->state == USB_STATE_LINK_UP ||
			prHifInfo->state == USB_STATE_PRE_RESUME ||
			prHifInfo->state == USB_STATE_PRE_SUSPEND)) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);
		return;
	}

	/* Hif power off wifi, drop rx packets and continue polling RX packets until RX path empty */
	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);
		halRxUSBReceiveData(prGlueInfo->prAdapter);
		return;
	}

	if (urb->status == -ESHUTDOWN || urb->status == -ENOENT) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);
		DBGLOG(RX, ERROR, "USB device shutdown skip Rx [%s]\n", __func__);
		return;
	}

#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataCompleteQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);

	kalSetIntEvent(prGlueInfo);
#else
	if (urb->status == 0) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataCompleteQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);

		tasklet_schedule(&prGlueInfo->rRxTask);
	} else {
		DBGLOG(RX, ERROR, "[%s] receive DATA fail (status = %d)\n", __func__, urb->status);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);

		halRxUSBReceiveData(prGlueInfo->prAdapter);
	}
#endif
}

void halRxUSBProcessEventDataComplete(struct ADAPTER *prAdapter,
	struct list_head *prCompleteQ, struct list_head *prFreeQ, uint32_t u4MinRfbCnt)
{
	struct USB_REQ *prUsbReq;
	struct urb *prUrb;
	struct BUF_CTRL *prBufCtrl;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint8_t *pucBufAddr;
	uint32_t u4BufLen;
	static u_int8_t s_fgOutOfSwRfb = FALSE;
	static uint32_t s_u4OutOfSwRfbPrintLimit;

	/* lock with rRxDataQLock if processing queue is data queue */
	/* and vice versa                                           */
	spinlock_t *prLock =
		(prCompleteQ == &prHifInfo->rRxDataCompleteQ) ?
		(&prHifInfo->rRxDataQLock) :
		(&prHifInfo->rRxEventQLock);

	/* Process complete event/data */
	prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, prLock);
	while (prUsbReq) {
		prUrb = prUsbReq->prUrb;
		prBufCtrl = prUsbReq->prBufCtrl;

		DBGLOG(RX, LOUD, "[%s] Rx URB[0x%p] Len[%u] Sts[%u]\n", __func__,
			prUrb, prUrb->actual_length, prUrb->status);

		if (prUrb->status != 0) {
			DBGLOG(RX, ERROR, "[%s] receive EVENT/DATA fail (status = %d)\n", __func__, prUrb->status);

			goto next_urb;
		}

#if CFG_CHIP_RESET_SUPPORT
		if (prAdapter->chip_info->fgIsSupportL0p5Reset) {

			KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);

			/* During WFSYS_RESET_STATE_REINIT,
			* driver might need to receive
			* events like response
			* of EXT_CMD_ID_EFUSE_BUFFER_MODE.
			 */
			if (prAdapter->eWfsysResetState
					!= WFSYS_RESET_STATE_IDLE &&
			    prAdapter->eWfsysResetState
					!= WFSYS_RESET_STATE_REINIT) {
				KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
					SPIN_LOCK_WFSYS_RESET);

				DBGLOG(RX, ERROR,
				       "skip rx urb process due to L0.5 reset\n");

				goto next_urb;
			}

			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);

		}

#endif /*CFG_CHIP_RESET_SUPPORT */

		pucBufAddr = prBufCtrl->pucBuf + prBufCtrl->u4ReadSize;
		u4BufLen = prUrb->actual_length - prBufCtrl->u4ReadSize;

		prBufCtrl->u4ReadSize += halRxUSBEnqueueRFB(
				prAdapter,
				pucBufAddr,
				u4BufLen,
				u4MinRfbCnt,
				prCompleteQ);

		if (unlikely(prUrb->actual_length - prBufCtrl->u4ReadSize > 4)) {
			if (s_fgOutOfSwRfb == FALSE) {
				if ((long)jiffies - (long)s_u4OutOfSwRfbPrintLimit > 0) {
					DBGLOG(RX, WARN, "Out of SwRfb!\n");
					s_u4OutOfSwRfbPrintLimit = jiffies + MSEC_TO_JIFFIES(SW_RFB_LOG_LIMIT_MS);
				}
				s_fgOutOfSwRfb = TRUE;
			}
			glUsbEnqueueReq(prHifInfo, prCompleteQ, prUsbReq,
					prLock, TRUE);

			set_bit(GLUE_FLAG_RX_BIT, &prGlueInfo->ulFlag);
			wake_up_interruptible(&prGlueInfo->waitq);

			schedule_delayed_work(&prGlueInfo->rRxPktDeAggWork, MSEC_TO_JIFFIES(SW_RFB_RECHECK_MS));
			break;
		}

		if (unlikely(s_fgOutOfSwRfb == TRUE))
			s_fgOutOfSwRfb = FALSE;

next_urb:
		glUsbEnqueueReq(prHifInfo, prFreeQ, prUsbReq, prLock, FALSE);
		prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, prLock);
	}
}

#if CFG_CHIP_RESET_SUPPORT
void halRxUSBProcessWdtComplete(struct ADAPTER *prAdapter,
				struct list_head *prCompleteQ,
				struct list_head *prFreeQ, uint32_t u4MinRfbCnt)
{
	struct USB_REQ *prUsbReq;
	struct urb *prUrb;
	struct BUF_CTRL *prBufCtrl;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	spinlock_t *prLock = &prHifInfo->rRxWdtQLock;
	uint8_t *pucBufAddr;
	uint32_t u4BufLen;

#define USB_WATCHDOG_TIMEOUT_EVENT        0xFF

	/* Process complete WDT interrupt */
	prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, prLock);
	while (prUsbReq) {
		prUrb = prUsbReq->prUrb;
		prBufCtrl = prUsbReq->prBufCtrl;

		DBGLOG(RX, LOUD, "Rx URB[0x%p] Len[%u] Sts[%u]\n",
			prUrb, prUrb->actual_length, prUrb->status);

		if (prUrb->status != 0) {
			DBGLOG(RX, ERROR, "receive WDT fail (status = %d)\n",
			       prUrb->status);

			glUsbEnqueueReq(prHifInfo, prFreeQ, prUsbReq, prLock,
					FALSE);
			prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ,
						prLock);
			continue;
		}

#if CFG_CHIP_RESET_SUPPORT
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);

		if (prAdapter->eWfsysResetState != WFSYS_RESET_STATE_IDLE) {
			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);
			DBGLOG(RX, ERROR,
				   "skip rx urb process due to L0.5 reset\n");

			goto next_urb;
		}
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
#endif /* CFG_CHIP_RESET_SUPPORT */

		pucBufAddr = prBufCtrl->pucBuf + prBufCtrl->u4ReadSize;
		u4BufLen = prUrb->actual_length - prBufCtrl->u4ReadSize;

		if (u4BufLen == 1 &&
		    pucBufAddr[0] == USB_WATCHDOG_TIMEOUT_EVENT)
			GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_WDT);
		else {
			DBGLOG(RX, ERROR, "receive unexpected WDT packet\n");
			dumpMemory8(pucBufAddr, u4BufLen);
			WARN_ON(1);
		}

next_urb:
		glUsbEnqueueReq(prHifInfo, prFreeQ, prUsbReq, prLock, FALSE);
		prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, prLock);
	}
}
#endif /* CFG_CHIP_RESET_SUPPORT */

/*----------------------------------------------------------------------------*/
/*!
* @brief enable global interrupt
*
* @param prAdapter pointer to the Adapter handler
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void halEnableInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	GLUE_SET_REF_CNT(1, prAdapter->fgIsIntEnable);

	halRxUSBReceiveData(prAdapter);
	if (prHifInfo->eEventEpType != EVENT_EP_TYPE_DATA_EP)
		halRxUSBReceiveEvent(prAdapter, TRUE);

#if CFG_CHIP_RESET_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset &&
	    prAdapter->chip_info->bus_info->fgIsSupportWdtEp)
		halRxUSBReceiveWdt(prAdapter);
#endif

	glUdmaRxAggEnable(prGlueInfo, TRUE);
} /* end of halEnableInterrupt() */

/*----------------------------------------------------------------------------*/
/*!
* @brief disable global interrupt
*
* @param prAdapter pointer to the Adapter handler
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void halDisableInterrupt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	GLUE_SET_REF_CNT(0, prAdapter->fgIsIntEnable);

	usb_kill_anchored_urbs(&prHifInfo->rRxDataAnchor);
	usb_kill_anchored_urbs(&prHifInfo->rRxEventAnchor);
#if CFG_CHIP_RESET_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset &&
	    prAdapter->chip_info->bus_info->fgIsSupportWdtEp)
		usb_kill_anchored_urbs(&prHifInfo->rRxWdtAnchor);
#endif

	if (!wlanIsChipNoAck(prAdapter))
		glUdmaRxAggEnable(prGlueInfo, FALSE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to process the POWER OFF procedure.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
u_int8_t halSetDriverOwn(struct ADAPTER *prAdapter)
{
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This routine is used to process the POWER ON procedure.
*
* \param[in] pvAdapter Pointer to the Adapter structure.
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void halSetFWOwn(struct ADAPTER *prAdapter, u_int8_t fgEnableGlobalInt)
{
}

void halWakeUpWiFi(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo = NULL;
	u_int8_t fgResult = FALSE;
	u_int8_t fgVdrPwrOnResult = FALSE;
	uint8_t ucCount = 0;
	uint32_t u4Value;

	DBGLOG(INIT, INFO, "Power on Wi-Fi....\n");

	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;

	HAL_WIFI_FUNC_READY_CHECK(prAdapter, WIFI_FUNC_INIT_DONE, &fgResult);

	/* If check wifi already on before send power on vendor req,
	*  the only case is BT help wifi dl patch, in this case need check
	*  power on vendor req have already done or will result CMD fail
	*  due to cal done will switch mcu state(if CMD send during switch
	*  MCU state, CMD packet will be clear due to mcu will do MAC uninit).
	*/
	if (fgResult) {
		/* Need clr vdr_pwr_on ack CR status first, due to FW
		 * not clear this.
		 */
		HAL_MCR_WR(prAdapter, prChipInfo->vdr_pwr_on, 0);
		HAL_MCR_RD(prAdapter, prChipInfo->vdr_pwr_on, &u4Value);

		/* This polling mechanism in falcon need patch dl done, due
		 * do FW add this mechanism in patch, after falcon will add
		 * this in rom code.
		 */
		prChipInfo->is_need_check_vdr_pwr_on = TRUE;

		/* Check vdr_pwr_on ack CR already be clear. */
		while (u4Value != 0) {
			kalMdelay(1);
			HAL_MCR_RD(prAdapter, prChipInfo->vdr_pwr_on, &u4Value);
			if (ucCount >= 5) {
				DBGLOG(INIT, ERROR,
					"Access vdr_pwr_on CR fail!!!\n");
				prChipInfo->is_need_check_vdr_pwr_on = FALSE;

				break;
			}
			ucCount++;
		}
		ucCount = 0;
	}
	if (!prChipInfo->is_need_check_vdr_pwr_on)
		fgVdrPwrOnResult = TRUE;

	while (1) {
		HAL_WIFI_FUNC_POWER_ON(prAdapter);
		kalMdelay(50);
		HAL_WIFI_FUNC_READY_CHECK(prAdapter, WIFI_FUNC_INIT_DONE, &fgResult);
		if (prChipInfo->is_need_check_vdr_pwr_on)
			HAL_VDR_PWR_ON_READY_CHECK(prAdapter,
				&fgVdrPwrOnResult);

		if (fgResult && fgVdrPwrOnResult)
			break;

		ucCount++;

		if (ucCount >= 40) {
			DBGLOG(INIT, WARN, "Power on failed!!!\n");
			break;
		}
	}

	if (prHifInfo->state == USB_STATE_WIFI_OFF && fgResult)
		glUsbSetState(prHifInfo,
			USB_STATE_LINK_UP);

	prAdapter->fgIsFwOwn = FALSE;
}

void halEnableFWDownload(struct ADAPTER *prAdapter, u_int8_t fgEnable)
{
#if (CFG_UMAC_GENERATION >= 0x20)
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	if (prChipInfo->asicEnableFWDownload) {
		prChipInfo->asicEnableFWDownload(prAdapter, fgEnable);
	} else {
		uint32_t u4Value = 0;

		HAL_MCR_RD(prAdapter, UDMA_TX_QSEL, &u4Value);

		if (fgEnable)
			u4Value |= FW_DL_EN;
		else
			u4Value &= ~FW_DL_EN;

		HAL_MCR_WR(prAdapter, UDMA_TX_QSEL, u4Value);
	}
#endif
}

void halDevInit(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;

	glUdmaRxAggEnable(prGlueInfo, FALSE);
	glUdmaTxRxEnable(prGlueInfo, TRUE);
}
u_int32_t halTxGetFreeCmdCnt(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct USB_REQ *prUsbReq, *prNext;
	unsigned long flags;
	u_int16_t u2Cnt = 0;

	if (prAdapter == NULL) {
		DBGLOG(HAL, ERROR, "prAdapter is NULL error\n");
		return 0;
	}
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	if (prHifInfo == NULL) {
		DBGLOG(HAL, ERROR, "prHifInfo is NULL error\n");
		return 0;
	}

	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	list_for_each_entry_safe(prUsbReq,
			prNext, &prHifInfo->rTxCmdFreeQ, list)
		u2Cnt++;
	spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);
	return u2Cnt;
}

u_int8_t halTxIsCmdBufEnough(struct ADAPTER *prAdapter)
{
	return TRUE;
}

u_int8_t halTxIsDataBufEnough(struct ADAPTER *prAdapter,
		struct MSDU_INFO *prMsduInfo)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint8_t ucTc;
	struct sk_buff *skb;
	uint32_t u4Length;
	struct mt66xx_chip_info *prChipInfo;

	unsigned long flags;

	prChipInfo = prAdapter->chip_info;
	skb = (struct sk_buff *)prMsduInfo->prPacket;
	u4Length = skb->len;
	u4Length += prChipInfo->u2HifTxdSize;
	ucTc = halUsbDetermineTc(prChipInfo, prMsduInfo);

	spin_lock_irqsave(&prHifInfo->rTxDataQLock, flags);

	if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc])) {
		if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
			spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);
			return FALSE;
		}
	}

	prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
	prBufCtrl = prUsbReq->prBufCtrl;

	if (prHifInfo->rTxDataFreeQ[ucTc].next->next == &prHifInfo->rTxDataFreeQ[ucTc]) {
		/* length of rTxDataFreeQ equals 1 */
		if (prBufCtrl->u4WrIdx + ALIGN_4(u4Length) >
		    prBufCtrl->u4BufSize - prHifInfo->u4AggRsvSize[ucTc] - LEN_USB_UDMA_TX_TERMINATOR) {
			/* Buffer is not enough */
			if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
				spin_unlock_irqrestore(&prHifInfo->rTxDataQLock,
							flags);
				return FALSE;
			}
		}
	}
	prHifInfo->u4AggRsvSize[ucTc] += ALIGN_4(u4Length);

	spin_unlock_irqrestore(&prHifInfo->rTxDataQLock, flags);
	return TRUE;
}

u_int8_t halTxIsBssCntFull(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return FALSE;
}

uint8_t halTxRingDataSelect(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo)
{
	return 0;
}

void halUpdateTxMaxQuota(struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo;
	uint32_t u4Ret;
	uint32_t u4Quota;
	bool fgRun;
	uint8_t ucWmmIndex;

	KAL_SPIN_LOCK_DECLARATION();

	prBusInfo = prAdapter->chip_info->bus_info;

	for (ucWmmIndex = 0; ucWmmIndex < prAdapter->ucWmmSetNum;
		ucWmmIndex++) {
		u4Ret = WLAN_STATUS_SUCCESS;

		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);
		u4Quota = prAdapter->rWmmQuotaReqCS[ucWmmIndex].u4Quota;
		fgRun = prAdapter->rWmmQuotaReqCS[ucWmmIndex].fgRun;
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UPDATE_WMM_QUOTA);

		if (fgRun) {
			if (prBusInfo->updateTxRingMaxQuota) {
				u4Ret = prBusInfo->updateTxRingMaxQuota(
							prAdapter,
							(uint16_t)ucWmmIndex,
							u4Quota);
			} else {
				DBGLOG(HAL, INFO,
					"updateTxRingMaxQuota not implemented\n");
				u4Ret = WLAN_STATUS_NOT_ACCEPTED;
			}
		}

		DBGLOG(HAL, INFO,
			"WmmQuota,Run,%u,Wmm,%u,Quota,0x%x,ret=0x%x\n",
			fgRun, ucWmmIndex, u4Quota, u4Ret);
		if (u4Ret != WLAN_STATUS_PENDING) {
			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_UPDATE_WMM_QUOTA);
			prAdapter->rWmmQuotaReqCS[ucWmmIndex].fgRun
				= false;
			KAL_RELEASE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_UPDATE_WMM_QUOTA);
		}
	}
}

void halUpdateBssTokenCnt(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex)
{
}

#if (CFG_TX_HIF_CREDIT_FEATURE == 1)
void halAdjustBssTxCredit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
}

uint32_t halGetBssTxCredit(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	return 0;
}

u_int8_t halTxIsBssCreditCntFull(uint32_t u4TxCredit)
{
	return FALSE;
}
#endif

void halProcessTxInterrupt(struct ADAPTER *prAdapter)
{
#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	struct USB_REQ *prUsbReq;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	/* Process complete Tx cmd */
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxCmdCompleteQ, &prHifInfo->rTxCmdQLock);
	while (prUsbReq) {
		halTxUSBProcessCmdComplete(prAdapter, prUsbReq);
		prUsbReq = glUsbDequeueReq(prHifInfo,
						&prHifInfo->rTxCmdCompleteQ,
						&prHifInfo->rTxCmdQLock);
	}

	/* Process complete Tx data */
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ,
					&prHifInfo->rTxDataQLock);
	while (prUsbReq) {
		halTxUSBProcessDataComplete(prAdapter, prUsbReq);
		prUsbReq = glUsbDequeueReq(prHifInfo,
						&prHifInfo->rTxDataCompleteQ,
						&prHifInfo->rTxDataQLock);
	}
#endif
}

bool halHifSwInfoInit(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	if (prBusInfo->DmaShdlInit)
		prBusInfo->DmaShdlInit(prAdapter);

	if (prChipInfo->asicUsbInit)
		prChipInfo->asicUsbInit(prAdapter, prChipInfo);

	if (prBusInfo->asicUdmaRxFlush)
		prBusInfo->asicUdmaRxFlush(prAdapter, FALSE);

	/* set USB EP_RST_OPT as reset scope excludes toggle
	 * bit,sequence number, etc.
	 */
	if (prBusInfo->asicUsbEpctlRstOpt)
		prBusInfo->asicUsbEpctlRstOpt(prAdapter,
					      FALSE);

	return true;
}

void halHifSwInfoUnInit(struct GLUE_INFO *prGlueInfo)
{
}

void halRxProcessMsduReport(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{

}

u_int8_t halProcessToken(struct ADAPTER *prAdapter,
	uint32_t u4Token,
	struct QUE *prFreeQueue)
{
	return 0;
}

void halMsduReportStats(struct ADAPTER *prAdapter, uint32_t u4Token,
	uint32_t u4MacLatency, uint32_t u4AirLatency, uint32_t u4Stat)
{
}

static uint32_t halTxGetPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t u4RequiredBufferSize;
	uint32_t u4PageCount;
	uint32_t u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH + prChipInfo->txd_append_size;

	/* Frame Buffer
	 *  |<--Tx Descriptor-->|<--Tx descriptor padding-->|
	 *  <--802.3/802.11 Header-->|<--Header padding-->|<--Payload-->|
	 */

	if (fgIncludeDesc)
		u4RequiredBufferSize = u4FrameLength;
	else
		u4RequiredBufferSize = u4TxHeadRoomSize + u4FrameLength;

	u4RequiredBufferSize = ALIGN_4(u4RequiredBufferSize);

	if (NIC_TX_PAGE_SIZE_IS_POWER_OF_2)
		u4PageCount = (u4RequiredBufferSize + (NIC_TX_PAGE_SIZE - 1)) >> NIC_TX_PAGE_SIZE_IN_POWER_OF_2;
	else
		u4PageCount = (u4RequiredBufferSize + (NIC_TX_PAGE_SIZE - 1)) / NIC_TX_PAGE_SIZE;

	return u4PageCount;
}

uint32_t halTxGetDataPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	return halTxGetPageCount(prAdapter, u4FrameLength, fgIncludeDesc);
}

uint32_t halTxGetCmdPageCount(struct ADAPTER *prAdapter,
	uint32_t u4FrameLength, u_int8_t fgIncludeDesc)
{
	return halTxGetPageCount(prAdapter, u4FrameLength, fgIncludeDesc);
}

uint32_t halTxPollingResource(struct ADAPTER *prAdapter, uint8_t ucTC)
{
	return WLAN_STATUS_SUCCESS;
}

void halSerHifReset(struct ADAPTER *prAdapter)
{
	uint32_t i;

	/**
	 * usb_reset_endpoint - Reset an endpoint's state.
	 * @dev: the device whose endpoint is to be reset
	 * @epaddr: the endpoint's address.  Endpoint number for output,
	 *	endpoint number + USB_DIR_IN for input
	 *
	 * Resets any host-side endpoint state such as the toggle bit,
	 * sequence number or current window.
	 *
	 * void usb_reset_endpoint(struct usb_device *dev, unsigned int epaddr);
	 */

	/* SER new flow: just flush out ep out fifo,
	* not to reset ep out endpoint
	*/
#if 0
	/* reset ALL BULK OUT endpoints */
	for (i = USB_DATA_BULK_OUT_EP4; i <= USB_DATA_BULK_OUT_EP9; i++)
		usb_reset_endpoint(prAdapter->prGlueInfo->rHifInfo.udev, i);
#endif
	/* reset ALL BULK IN endpoints */
	for (i = USB_DATA_BULK_IN_EP4; i <= USB_DATA_BULK_IN_EP5; i++)
		usb_reset_endpoint(prAdapter->prGlueInfo->rHifInfo.udev,
					i | USB_DIR_IN);
}

void halProcessRxInterrupt(struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo;

	if (prAdapter == NULL || prAdapter->prGlueInfo == NULL)
		return;
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	/* Process complete data */
	halRxUSBProcessEventDataComplete(prAdapter, &prHifInfo->rRxDataCompleteQ,
		&prHifInfo->rRxDataFreeQ, USB_RX_DATA_RFB_RSV_CNT);
	halRxUSBReceiveData(prAdapter);

	if (prHifInfo->eEventEpType != EVENT_EP_TYPE_DATA_EP) {
		/* Process complete event */
		halRxUSBProcessEventDataComplete(prAdapter, &prHifInfo->rRxEventCompleteQ,
			&prHifInfo->rRxEventFreeQ, USB_RX_EVENT_RFB_RSV_CNT);
		halRxUSBReceiveEvent(prAdapter, FALSE);
	}

#if CFG_CHIP_RESET_SUPPORT
	if (prAdapter->chip_info->fgIsSupportL0p5Reset &&
	    prAdapter->chip_info->bus_info->fgIsSupportWdtEp) {
		halRxUSBProcessWdtComplete(prAdapter,
					   &prHifInfo->rRxWdtCompleteQ,
					   &prHifInfo->rRxWdtFreeQ,
					   USB_RX_WDT_RFB_RSV_CNT);

		halRxUSBReceiveWdt(prAdapter);
	}
#endif
}

uint32_t halDumpHifStatus(struct ADAPTER *prAdapter, uint8_t *pucBuf,
		uint32_t u4Max)
{
	uint32_t u4CpuIdx, u4DmaIdx, u4Int, u4GloCfg, u4Reg;
	uint32_t u4Len = 0;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint8_t pBuffer[512] = {0};

	HAL_MCR_RD(prAdapter, 0x820b0118, &u4CpuIdx);
	HAL_MCR_RD(prAdapter, 0x820b011c, &u4DmaIdx);
	HAL_MCR_RD(prAdapter, 0x820b0220, &u4Int);
	HAL_MCR_RD(prAdapter, 0x820b0204, &u4GloCfg);

	LOGBUF(pucBuf, u4Max, u4Len, "\n");
	LOGBUF(pucBuf, u4Max, u4Len, "PDMA1R1 CPU[%u] DMA[%u] INT[0x%08x] CFG[0x%08x]\n", u4CpuIdx,
		u4DmaIdx, u4Int, u4GloCfg);

	HAL_MCR_RD(prAdapter, UDMA_WLCFG_0, &u4Reg);

	LOGBUF(pucBuf, u4Max, u4Len, "UDMA WLCFG[0x%08x]\n", u4Reg);

	LOGBUF(pucBuf, u4Max, u4Len, "\n");
	LOGBUF(pucBuf, u4Max, u4Len, "VenderID: %04x\n",
		glGetUsbDeviceVendorId(prGlueInfo->rHifInfo.udev));
	LOGBUF(pucBuf, u4Max, u4Len, "ProductID: %04x\n",
		glGetUsbDeviceProductId(prGlueInfo->rHifInfo.udev));

	glGetUsbDeviceManufacturerName(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pucBuf, u4Max, u4Len, "Manufacturer: %s\n",
		pBuffer);

	glGetUsbDeviceProductName(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pucBuf, u4Max, u4Len, "Product: %s\n", pBuffer);

	glGetUsbDeviceSerialNumber(prGlueInfo->rHifInfo.udev, pBuffer,
		sizeof(pBuffer));
	LOGBUF(pucBuf, u4Max, u4Len, "SerialNumber: %s\n",
		pBuffer);

	return u4Len;
}

void halGetCompleteStatus(struct ADAPTER *prAdapter, uint32_t *pu4IntStatus)
{
#if CFG_USB_RX_HANDLE_IN_HIF_THREAD || CFG_USB_TX_HANDLE_IN_HIF_THREAD
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
#endif

	*pu4IntStatus = 0;

#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	if (!list_empty(&prHifInfo->rRxDataCompleteQ) || !list_empty(&prHifInfo->rRxEventCompleteQ))
		*pu4IntStatus |= WHISR_RX0_DONE_INT;
#endif

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	if (!list_empty(&prHifInfo->rTxDataCompleteQ) || !list_empty(&prHifInfo->rTxCmdCompleteQ))
		*pu4IntStatus |= WHISR_TX_DONE_INT;
#endif
}

u_int8_t halIsPendingRx(struct ADAPTER *prAdapter)
{
#if CFG_USB_RX_HANDLE_IN_HIF_THREAD
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	if (!list_empty(&prHifInfo->rRxDataCompleteQ) || !list_empty(&prHifInfo->rRxEventCompleteQ))
		return TRUE;
	else
		return FALSE;
#else
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Send HIF_CTRL command to inform FW stop send packet/event to host
*	suspend = 1
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return (void)
*/
/*----------------------------------------------------------------------------*/
void halUSBPreSuspendCmd(struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl;
	uint32_t rStatus;

	kalMemSet(&rCmdHifCtrl, 0, sizeof(struct CMD_HIF_CTRL));

	rCmdHifCtrl.ucHifType = ENUM_HIF_TYPE_USB;
	rCmdHifCtrl.ucHifDirection = ENUM_HIF_TX;
	rCmdHifCtrl.ucHifStop = 1;
	rCmdHifCtrl.ucHifSuspend = 1;
	rCmdHifCtrl.u4WakeupHifType = ENUM_CMD_HIF_WAKEUP_TYPE_USB;

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_HIF_CTRL,	/* ucCID */
				      TRUE,	/* fgSetQuery */
				      FALSE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      NULL,	/* pfCmdDoneHandler */
				      NULL,	/* pfCmdTimeoutHandler */
				      sizeof(struct CMD_HIF_CTRL),	/* u4SetQueryInfoLen */
				      (uint8_t *)&rCmdHifCtrl,	/* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
				     );

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Send HIF_CTRL command to inform FW allow send packet/event to host
*	suspend = 0
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return (void)
*/
/*----------------------------------------------------------------------------*/
void halUSBPreResumeCmd(struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl;
	uint32_t rStatus;

	kalMemSet(&rCmdHifCtrl, 0, sizeof(struct CMD_HIF_CTRL));

	rCmdHifCtrl.ucHifType = ENUM_HIF_TYPE_USB;
	rCmdHifCtrl.ucHifDirection = ENUM_HIF_TX;
	rCmdHifCtrl.ucHifStop = 0;
	rCmdHifCtrl.ucHifSuspend = 0;
	rCmdHifCtrl.u4WakeupHifType = ENUM_CMD_HIF_WAKEUP_TYPE_USB;

	rStatus = wlanSendSetQueryCmd(prAdapter, /* prAdapter */
				CMD_ID_HIF_CTRL,	/* ucCID */
				TRUE,	/* fgSetQuery */
				FALSE,	/* fgNeedResp */
				FALSE,	/* fgIsOid */
				NULL,	/* nicEventHifCtrl */
				NULL,	/* pfCmdTimeoutHandler */
				sizeof(struct CMD_HIF_CTRL),
				(uint8_t *)&rCmdHifCtrl, /* pucInfoBuffer */
				NULL,	/* pvSetQueryBuffer */
				0		/* u4SetQueryBufferLen */
	    );

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}

void halUSBPreSuspendDone(struct ADAPTER *prAdapter, struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf)
{
	unsigned long flags;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	spin_lock_irqsave(&prHifInfo->rStateLock, flags);

	if (prHifInfo->state == USB_STATE_LINK_UP
		|| prHifInfo->state == USB_STATE_PRE_SUSPEND)
		prHifInfo->state = USB_STATE_SUSPEND;
	else
		DBGLOG(HAL, ERROR, "Previous USB state (%d)!\n",
			prHifInfo->state);

	spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
}

void halUSBPreSuspendTimeout(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo)
{
	unsigned long flags;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	spin_lock_irqsave(&prHifInfo->rStateLock, flags);

	if (prHifInfo->state == USB_STATE_LINK_UP
		|| prHifInfo->state == USB_STATE_PRE_SUSPEND)
		prHifInfo->state = USB_STATE_PRE_SUSPEND_FAIL;
	else
		DBGLOG(HAL, ERROR, "Previous USB state (%d)!\n",
			prHifInfo->state);

	spin_unlock_irqrestore(&prHifInfo->rStateLock, flags);
}

uint32_t halGetValidCoalescingBufSize(struct ADAPTER *prAdapter)
{
	uint32_t u4BufSize;

	if (HIF_TX_COALESCING_BUFFER_SIZE > HIF_RX_COALESCING_BUFFER_SIZE)
		u4BufSize = HIF_TX_COALESCING_BUFFER_SIZE;
	else
		u4BufSize = HIF_RX_COALESCING_BUFFER_SIZE;

	return u4BufSize;
}

uint32_t halAllocateIOBuffer(struct ADAPTER *prAdapter)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t halReleaseIOBuffer(struct ADAPTER *prAdapter)
{
	return WLAN_STATUS_SUCCESS;
}

void halProcessAbnormalInterrupt(struct ADAPTER *prAdapter)
{

}

void halProcessSoftwareInterrupt(struct ADAPTER *prAdapter)
{

}

void halDeAggRxPktWorker(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo = CONTAINER_OF(work, struct GLUE_INFO,
						    rRxPktDeAggWork.work);

	tasklet_schedule(&prGlueInfo->rRxTask);
}

void halRxTasklet(unsigned long data)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;

	halProcessRxInterrupt(prGlueInfo->prAdapter);
}

void halTxCompleteTasklet(unsigned long data)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;

	/* Process complete Tx data */
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ, &prHifInfo->rTxDataQLock);
	while (prUsbReq) {
		halTxUSBProcessDataComplete(prGlueInfo->prAdapter, prUsbReq);
		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ, &prHifInfo->rTxDataQLock);
	}
}

/* Hif power off wifi */
uint32_t halHifPowerOffWifi(struct ADAPTER *prAdapter)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Power off Wi-Fi!\n");

	/* Power off Wi-Fi */
	wlanSendNicPowerCtrlCmd(prAdapter, TRUE);

	rStatus = wlanCheckWifiFunc(prAdapter, FALSE);

	glUsbSetState(&prAdapter->prGlueInfo->rHifInfo, USB_STATE_WIFI_OFF);

	nicDisableInterrupt(prAdapter);

	wlanClearPendingInterrupt(prAdapter);

	halTxCancelAllSending(prAdapter);

	return rStatus;
}

void halPrintHifDbgInfo(struct ADAPTER *prAdapter)
{
	struct CHIP_DBG_OPS *prDbgOps;

	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "prAdapter is NULL\n");
		return;
	}

	prDbgOps = prAdapter->chip_info->prDebugOps;

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PLE))
		if (prDbgOps && prDbgOps->showPleInfo)
			prDbgOps->showPleInfo(prAdapter, FALSE);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PSE))
		if (prDbgOps && prDbgOps->showPseInfo)
			prDbgOps->showPseInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PDMA))
		if (prDbgOps && prDbgOps->showPdmaInfo)
			prDbgOps->showPdmaInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_DMASCH))
		if (prDbgOps && prDbgOps->showDmaschInfo)
			prDbgOps->showDmaschInfo(prAdapter);

	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_MAC))
		if (prDbgOps && prDbgOps->dumpMacInfo)
			prDbgOps->dumpMacInfo(prAdapter);

#if (CFG_SUPPORT_DEBUG_SOP == 1)
	if (prAdapter->u4HifDbgFlag & (DEG_HIF_ALL | DEG_HIF_PLATFORM_DBG)) {
		if (prDbgOps && prDbgOps->show_debug_sop_info)
			prDbgOps->show_debug_sop_info(prAdapter,
				SLAVENORESP);
	}
#endif
	prAdapter->u4HifDbgFlag = 0;
}

u_int8_t halIsTxResourceControlEn(struct ADAPTER *prAdapter)
{
	return FALSE;
}

void halTxResourceResetHwTQCounter(struct ADAPTER *prAdapter)
{
}

static uint32_t halGetHifTxPageSize(struct ADAPTER *prAdapter)
{
	return HIF_TX_PAGE_SIZE;
}

uint32_t halGetHifTxCmdPageSize(struct ADAPTER *prAdapter)
{
	return halGetHifTxPageSize(prAdapter);
}

uint32_t halGetHifTxDataPageSize(struct ADAPTER *prAdapter)
{
	return halGetHifTxPageSize(prAdapter);
}

uint32_t halSerGetMcuEvent(struct ADAPTER *prAdapter, u_int8_t fgClear)
{
	struct GLUE_INFO *prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4SerAction;

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;

	/* get MCU SER event */
	if (!kalDevRegRead(prGlueInfo,
			   prChipInfo->u4SerUsbMcuEventAddr,
			   &u4SerAction)) {
		DBGLOG(NIC, WARN, "get MCU SER event fail\n");

		u4SerAction = 0;

		goto out;
	}

	if (u4SerAction && fgClear) {
		DBGLOG(NIC, INFO, "u4SerAction=0x%08X\n", u4SerAction);

		/* clear MCU SER event */
		kalDevRegWrite(prGlueInfo,
			       prChipInfo->u4SerUsbMcuEventAddr, 0);
	}

out:
	return u4SerAction;
}

void halSerSyncTimerHandler(struct ADAPTER *prAdapter)
{
	static u_int8_t ucSerState = ERR_RECOV_STOP_IDLE;
	uint32_t u4SerAction;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;

	if (prAdapter->prGlueInfo->rHifInfo.state == USB_STATE_SUSPEND)
		return;

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;

	/* get MCU SER event */
	u4SerAction = halSerGetMcuEvent(prAdapter, TRUE);
	if (u4SerAction == 0)
		return;

	if (prAdapter->rWifiVar.eEnableSerL1 !=
	     FEATURE_OPT_SER_ENABLE) {
		if (prChipInfo->asicDumpSerDummyCR)
			prChipInfo->asicDumpSerDummyCR(prAdapter);

		DBGLOG(HAL, WARN,
		       "[SER][L1] Bypass L1 reset due to wifi.cfg\n");

		GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SER_L1_FAIL);

		return;
	}

	switch (ucSerState) {
	case ERR_RECOV_STOP_IDLE:
		if (u4SerAction == ERROR_DETECT_STOP_PDMA) {
			if (prChipInfo->asicDumpSerDummyCR)
				prChipInfo->asicDumpSerDummyCR(prAdapter);

			DBGLOG(HAL, INFO,
				"SER(E) Host stop HIF tx/rx operation\n");

			/* change SER FSM to SER_STOP_HOST_TX_RX */
			nicSerStopTxRx(prAdapter);
			/* stop TX BULK OUT URB */
			halTxCancelAllSending(prAdapter);
			/* stop RX BULK IN URB */
			halDisableInterrupt(prAdapter);

			DBGLOG(HAL, INFO,
			"SER(F) Host ACK HIF tx/rx stop operation done\n");

			/* Send Host stops TX/RX done response to mcu */
			kalDevRegWrite(prAdapter->prGlueInfo,
					prChipInfo->u4SerUsbHostAckAddr,
					MCU_INT_PDMA0_STOP_DONE);
			ucSerState = ERR_RECOV_STOP_PDMA0;
		} else {
			/* do nothing */
		}
		break;

	case ERR_RECOV_STOP_PDMA0:
		if (u4SerAction == ERROR_DETECT_RESET_DONE) {
			DBGLOG(HAL, INFO, "SER(L) Host re-initialize WFDMA\n");
			DBGLOG(HAL, INFO, "SER(M) Host enable WFDMA\n");

			if (prChipInfo->asicUsbInit)
				prChipInfo->asicUsbInit(prAdapter, prChipInfo);

			DBGLOG(HAL, INFO,
				"SER(N) Host ACK WFDMA init done\n");
			/* Send Host stops TX/RX done response to mcu */
			kalDevRegWrite(prAdapter->prGlueInfo,
					prChipInfo->u4SerUsbHostAckAddr,
					MCU_INT_PDMA0_INIT_DONE);

			ucSerState = ERR_RECOV_RESET_PDMA0;
		} else {
			/* do nothing */
		}
		break;
	case ERR_RECOV_RESET_PDMA0:
		if (u4SerAction == ERROR_DETECT_RECOVERY_DONE) {
			if (prBusInfo->DmaShdlInit)
				prBusInfo->DmaShdlInit(prAdapter);

			DBGLOG(HAL, INFO,
				"SER(Q) Host ACK MCU SER handle done\n");
			/* Send Host stops TX/RX done response to mcu */
			kalDevRegWrite(prAdapter->prGlueInfo,
				prChipInfo->u4SerUsbHostAckAddr,
				MCU_INT_PDMA0_RECOVERY_DONE);
			ucSerState = ERR_RECOV_WAIT_MCU_NORMAL;
		} else {
			/* do nothing */
		}
		break;

	case ERR_RECOV_WAIT_MCU_NORMAL:
		if (u4SerAction == ERROR_DETECT_MCU_NORMAL_STATE) {
#if (CFG_SUPPORT_ADHOC) || (CFG_ENABLE_WIFI_DIRECT)
			/* update Beacon frame if operating in AP mode. */
			DBGLOG(HAL, INFO, "SER(T) Host re-initialize BCN\n");
			nicSerReInitBeaconFrame(prAdapter);
#endif
			DBGLOG(HAL, INFO,
				"SER(U) Host reset TX/RX endpoint\n");

			/* It's surprising that the toggle bit or sequence
			 * number of USB endpoints in some USB hosts cannot be
			 * reset by kernel API usb_reset_endpoint(). In order to
			 * prevent this IOT, we tend to do not reset toggle bit
			 * and sequence number on both device and host in some
			 * project like MT7961. In MT7961, we can choose that
			 * endpoints reset excludes toggle bit and sequence
			 * number through asicUsbEpctlRstOpt().
			 */
			if (!prBusInfo->asicUsbEpctlRstOpt)
				halSerHifReset(prAdapter);

			/* resume TX/RX */
			nicSerStartTxRx(prAdapter);

			halEnableInterrupt(prAdapter);

			ucSerState = ERR_RECOV_STOP_IDLE;
		} else {
			/* do nothing */
		}
		break;
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Check if HIF state is LINK_UP or READY for USB TX/RX
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return (TRUE: ready, FALSE: not ready)
*/
/*----------------------------------------------------------------------------*/
bool halIsHifStateLinkup(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	if (!prAdapter->prGlueInfo)
		return FALSE;

	if (prAdapter->prGlueInfo->rHifInfo.state != USB_STATE_LINK_UP)
		return FALSE;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Check if HIF state is READY for upper layer cfg80211
*
* @param prGlueInfo      Pointer to the GlueInfo structure.
*
* @return (TRUE: ready, FALSE: not ready)
*/
/*----------------------------------------------------------------------------*/
u_int8_t halIsHifStateReady(struct GLUE_INFO *prGlueInfo, uint8_t *pucState)
{

	if (!prGlueInfo)
		return FALSE;

	if (prGlueInfo->u4ReadyFlag == 0)
		return FALSE;

	if (pucState)
		*pucState = prGlueInfo->rHifInfo.state;

	if (prGlueInfo->rHifInfo.state != USB_STATE_LINK_UP)
		return FALSE;

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief Check if HIF state is during supend process
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return (TRUE: suspend, reject the caller action. FALSE: not suspend)
*/
/*----------------------------------------------------------------------------*/
bool halIsHifStateSuspend(struct ADAPTER *prAdapter)
{
	enum usb_state state;

	if (!prAdapter)
		return FALSE;

	if (!prAdapter->prGlueInfo)
		return FALSE;

	state = prAdapter->prGlueInfo->rHifInfo.state;

	if (state == USB_STATE_SUSPEND)
		return TRUE;

	return FALSE;
}

void halUpdateHifConfig(struct ADAPTER *prAdapter)
{
}

void halDumpHifStats(struct ADAPTER *prAdapter)
{
}

uint32_t halSetSuspendFlagToFw(struct ADAPTER *prAdapter,
	u_int8_t fgSuspend)
{
	return WLAN_STATUS_SUCCESS;
}

