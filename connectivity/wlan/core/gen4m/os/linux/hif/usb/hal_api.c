/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
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

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

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
u_int8_t halVerifyChipID(IN struct ADAPTER *prAdapter)
{
	uint32_t u4CIR = 0;
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	if (prAdapter->fgIsReadRevID)
		return TRUE;

	prChipInfo = prAdapter->chip_info;

	HAL_MCR_RD(prAdapter, TOP_HCR, &u4CIR);
	DBGLOG(INIT, TRACE, "Chip ID: 0x%lx\n", u4CIR);

	if (u4CIR != prChipInfo->chip_id)
		return FALSE;

	HAL_MCR_RD(prAdapter, TOP_HVR, &u4CIR);
	DBGLOG(INIT, TRACE, "Revision ID: 0x%lx\n", u4CIR);

	prAdapter->ucRevID = (uint8_t) (u4CIR & 0xF);
	prAdapter->fgIsReadRevID = TRUE;
	return TRUE;
}

uint32_t
halRxWaitResponse(IN struct ADAPTER *prAdapter, IN uint8_t ucPortIdx, OUT uint8_t *pucRspBuffer,
		  IN uint32_t u4MaxRespBufferLen, OUT uint32_t *pu4Length)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct RX_CTRL *prRxCtrl;
	u_int8_t ret = FALSE;
	struct BUS_INFO *prBusInfo;

	DEBUGFUNC("halRxWaitResponse");

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
					prRxCtrl->pucRxCoalescingBufPtr, HIF_RX_COALESCING_BUFFER_SIZE);

				if (ret == TRUE) {
					prHifInfo->eEventEpType = EVENT_EP_TYPE_DATA_EP;
				} else {
					ucPortIdx = USB_EVENT_EP_IN;
					ret = kalDevPortRead(prAdapter->prGlueInfo, ucPortIdx,
						ALIGN_4(u4MaxRespBufferLen) + LEN_USB_RX_PADDING_CSO,
						prRxCtrl->pucRxCoalescingBufPtr, HIF_RX_COALESCING_BUFFER_SIZE);
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
		prRxCtrl->pucRxCoalescingBufPtr, HIF_RX_COALESCING_BUFFER_SIZE);

	kalMemCopy(pucRspBuffer, prRxCtrl->pucRxCoalescingBufPtr, u4MaxRespBufferLen);
	*pu4Length = u4MaxRespBufferLen;

	if (ret == FALSE)
		u4Status = WLAN_STATUS_FAILURE;

	return u4Status;
}

uint32_t halTxUSBSendCmd(IN struct GLUE_INFO *prGlueInfo, IN uint8_t ucTc, IN struct CMD_INFO *prCmdInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint16_t u2OverallBufferLength = 0;
	unsigned long flags;
	struct HW_MAC_TX_DESC *prTxDesc;
	uint8_t ucQueIdx;
	struct mt66xx_chip_info *prChipInfo;

	if (prHifInfo->state != USB_STATE_LINK_UP &&
	    !(prHifInfo->state == USB_STATE_PRE_RESUME && prCmdInfo->ucCID == 0))
		return WLAN_STATUS_FAILURE;

	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxCmdFreeQ, &prHifInfo->rTxCmdQLock);
	if (prUsbReq == NULL)
		return WLAN_STATUS_RESOURCES;

	prBufCtrl = prUsbReq->prBufCtrl;

	if ((TFCB_FRAME_PAD_TO_DW(prCmdInfo->u4TxdLen + prCmdInfo->u4TxpLen) + LEN_USB_UDMA_TX_TERMINATOR) >
	    prBufCtrl->u4BufSize) {
		DBGLOG(HAL, ERROR, "Command TX buffer underflow!\n");
		return WLAN_STATUS_RESOURCES;
	}

	DBGLOG(HAL, INFO, "TX URB[0x%p]\n", prUsbReq->prUrb);

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	HAL_WRITE_HIF_TXD(prChipInfo, prBufCtrl->pucBuf, (prCmdInfo->u4TxdLen + prCmdInfo->u4TxpLen));
	u2OverallBufferLength += prChipInfo->u2HifTxdSize;

	if (prCmdInfo->u4TxdLen) {
		memcpy((prBufCtrl->pucBuf + u2OverallBufferLength), prCmdInfo->pucTxd, prCmdInfo->u4TxdLen);
		u2OverallBufferLength += prCmdInfo->u4TxdLen;
	}

	if (prCmdInfo->u4TxpLen) {
		memcpy((prBufCtrl->pucBuf + u2OverallBufferLength), prCmdInfo->pucTxp, prCmdInfo->u4TxpLen);
		u2OverallBufferLength += prCmdInfo->u4TxpLen;
	}

	prTxDesc = (struct HW_MAC_TX_DESC *)prBufCtrl->pucBuf;
	ucQueIdx = HAL_MAC_TX_DESC_GET_QUEUE_INDEX(prTxDesc);
	/* For H2CDMA Tx CMD mapping */
	/* Mapping port1 queue0~3 to queue28~31, and CR4 will unmask this */
	HAL_MAC_TX_DESC_SET_QUEUE_INDEX(prTxDesc, (ucQueIdx | USB_TX_CMD_QUEUE_MASK));

	/* DBGLOG_MEM32(SW4, INFO, prBufCtrl->pucBuf, 32); */

	memset(prBufCtrl->pucBuf + u2OverallBufferLength, 0,
	       ((TFCB_FRAME_PAD_TO_DW(u2OverallBufferLength) - u2OverallBufferLength) + LEN_USB_UDMA_TX_TERMINATOR));
	prBufCtrl->u4WrIdx = TFCB_FRAME_PAD_TO_DW(u2OverallBufferLength) + LEN_USB_UDMA_TX_TERMINATOR;

	prUsbReq->prPriv = (void *) prCmdInfo;
	usb_fill_bulk_urb(prUsbReq->prUrb,
			  prHifInfo->udev,
			  usb_sndbulkpipe(prHifInfo->udev, arTcToUSBEP[ucTc]),
			  (void *)prUsbReq->prBufCtrl->pucBuf,
			  prBufCtrl->u4WrIdx, halTxUSBSendCmdComplete, (void *)prUsbReq);

#if CFG_USB_CONSISTENT_DMA
	prUsbReq->prUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
#endif
	spin_lock_irqsave(&prHifInfo->rTxCmdQLock, flags);
	u4Status = usb_submit_urb(prUsbReq->prUrb, GFP_ATOMIC);
	if (u4Status) {
		DBGLOG(HAL, ERROR, "usb_submit_urb() reports error (%d)(EP%d OUT)\n", u4Status, arTcToUSBEP[ucTc]);
		/* glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxCmdFreeQ, &prHifInfo->rTxCmdQLock, prUsbReq); */
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxCmdFreeQ);
		spin_unlock_irqrestore(&prHifInfo->rTxCmdQLock, flags);
		return WLAN_STATUS_FAILURE;
	}

	/* glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxCmdSendingQ, &prHifInfo->rTxCmdQLock, prUsbReq); */
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

void halTxUSBProcessCmdComplete(IN struct ADAPTER *prAdapter, struct USB_REQ *prUsbReq)
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
	nicTxReleaseResource_PSE(prAdapter, TC4_INDEX, nicTxGetPageCount(prAdapter, u4SentDataSize, TRUE), TRUE);
}

void halTxCancelSendingCmd(IN struct ADAPTER *prAdapter, IN struct CMD_INFO *prCmdInfo)
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

void halTxCancelAllSending(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct USB_REQ *prUsbReq, *prUsbReqNext;
	struct GL_HIF_INFO *prHifInfo;
#if CFG_USB_TX_AGG
	uint8_t ucTc;
#endif

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	list_for_each_entry_safe(prUsbReq, prUsbReqNext, &prHifInfo->rTxCmdSendingQ, list) {
		usb_kill_urb(prUsbReq->prUrb);
	}

#if CFG_USB_TX_AGG
	for (ucTc = 0; ucTc < USB_TC_NUM; ++ucTc)
		usb_kill_anchored_urbs(&prHifInfo->rTxDataAnchor[ucTc]);
#else
	usb_kill_anchored_urbs(&prHifInfo->rTxDataAnchor);
#endif
}

#if CFG_USB_TX_AGG
uint32_t halTxUSBSendAggData(IN struct GL_HIF_INFO *prHifInfo, IN uint8_t ucTc, IN struct USB_REQ *prUsbReq)
{
	struct GLUE_INFO *prGlueInfo = prHifInfo->prGlueInfo;
	struct BUF_CTRL *prBufCtrl = prUsbReq->prBufCtrl;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, LEN_USB_UDMA_TX_TERMINATOR);
	prBufCtrl->u4WrIdx += LEN_USB_UDMA_TX_TERMINATOR;

	if (prHifInfo->state != USB_STATE_LINK_UP) {
		list_del_init(&prUsbReq->list);
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataCompleteQ);

#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
		kalSetIntEvent(prGlueInfo);
#else
		/*tasklet_hi_schedule(&prGlueInfo->rTxCompleteTask);*/
		tasklet_schedule(&prGlueInfo->rTxCompleteTask);
#endif
		return WLAN_STATUS_FAILURE;
	}

	list_del_init(&prUsbReq->list);

	usb_fill_bulk_urb(prUsbReq->prUrb,
			  prHifInfo->udev,
			  usb_sndbulkpipe(prHifInfo->udev, arTcToUSBEP[ucTc]),
			  (void *)prBufCtrl->pucBuf, prBufCtrl->u4WrIdx, halTxUSBSendDataComplete, (void *)prUsbReq);
#if CFG_USB_CONSISTENT_DMA
	prUsbReq->prUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
#endif

	usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rTxDataAnchor[ucTc]);
	u4Status = usb_submit_urb(prUsbReq->prUrb, GFP_ATOMIC);
	if (u4Status) {
		DBGLOG(HAL, ERROR, "usb_submit_urb() reports error (%d) [%s] (EP%d OUT)\n",
			u4Status, __func__, arTcToUSBEP[ucTc]);
		halTxUSBProcessMsduDone(prHifInfo->prGlueInfo, prUsbReq);
		prBufCtrl->u4WrIdx = 0;
		usb_unanchor_urb(prUsbReq->prUrb);
		list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataFreeQ[ucTc]);
		return WLAN_STATUS_FAILURE;
	}

	return u4Status;
}
#endif

uint32_t halTxUSBSendData(IN struct GLUE_INFO *prGlueInfo, IN struct MSDU_INFO *prMsduInfo)
{
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
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
#if CFG_USB_TX_AGG
	unsigned long flags;
#endif

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	skb = (struct sk_buff *)prMsduInfo->prPacket;
	pucBuf = skb->data;
	u4Length = skb->len;
	u4TotalLen = u4Length + prChipInfo->u2HifTxdSize;
	ucTc = USB_TRANS_MSDU_TC(prMsduInfo);

#if CFG_USB_TX_AGG
	spin_lock_irqsave(&prHifInfo->rTxDataFreeQLock, flags);

	if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc])) {
		if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
			spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
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
				spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
				DBGLOG(HAL, ERROR, "run out of rTxDataFreeQ #2!!\n");
				wlanProcessQueuedMsduInfo(prGlueInfo->prAdapter, prMsduInfo);
				return WLAN_STATUS_FAILURE;
			}
		}

		prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
		prBufCtrl = prUsbReq->prBufCtrl;
	}

	HAL_WRITE_HIF_TXD(prChipInfo, prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, u4Length);
	prBufCtrl->u4WrIdx += prChipInfo->u2HifTxdSize;
	memcpy(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, pucBuf, u4Length);
	prBufCtrl->u4WrIdx += u4Length;

	u4PaddingLength = (ALIGN_4(u4TotalLen) - u4TotalLen);
	if (u4PaddingLength) {
		memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, u4PaddingLength);
		prBufCtrl->u4WrIdx += u4PaddingLength;
	}

	if (!prMsduInfo->pfTxDoneHandler)
		QUEUE_INSERT_TAIL(&prUsbReq->rSendingDataMsduInfoList, (struct QUE_ENTRY *) prMsduInfo);

	if (usb_anchor_empty(&prHifInfo->rTxDataAnchor[ucTc]))
		halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);

	spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
#else
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataFreeQ, &prHifInfo->rTxDataFreeQLock);
	if (prUsbReq == NULL) {
		DBGLOG(HAL, ERROR, "run out of rTxDataFreeQ!!\n");
		wlanProcessQueuedMsduInfo(prGlueInfo->prAdapter, prMsduInfo);
		return WLAN_STATUS_RESOURCES;
	}

	prBufCtrl = prUsbReq->prBufCtrl;
	prBufCtrl->u4WrIdx = 0;

	HAL_WRITE_HIF_TXD(prChipInfo, prBufCtrl->pucBuf, u4Length);
	prBufCtrl->u4WrIdx += prChipInfo->u2HifTxdSize;

	memcpy(prBufCtrl->pucBuf + prChipInfo->u2HifTxdSize, pucBuf, u4Length);
	prBufCtrl->u4WrIdx += u4Length;

	u4PaddingLength = (ALIGN_4(u4TotalLen) - u4TotalLen);
	if (u4PaddingLength) {
		memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, u4PaddingLength);
		prBufCtrl->u4WrIdx += u4PaddingLength;
	}

	memset(prBufCtrl->pucBuf + prBufCtrl->u4WrIdx, 0, LEN_USB_UDMA_TX_TERMINATOR);
	prBufCtrl->u4WrIdx += LEN_USB_UDMA_TX_TERMINATOR;

	if (!prMsduInfo->pfTxDoneHandler)
		QUEUE_INSERT_TAIL(&prUsbReq->rSendingDataMsduInfoList, (struct QUE_ENTRY *) prMsduInfo);

	*((uint8_t *)&prUsbReq->prPriv) = ucTc;
	usb_fill_bulk_urb(prUsbReq->prUrb,
			  prHifInfo->udev,
			  usb_sndbulkpipe(prHifInfo->udev, arTcToUSBEP[ucTc]),
			  (void *)prUsbReq->prBufCtrl->pucBuf,
			  prBufCtrl->u4WrIdx, halTxUSBSendDataComplete, (void *)prUsbReq);
#if CFG_USB_CONSISTENT_DMA
	prUsbReq->prUrb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
#endif

	usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rTxDataAnchor);
	u4Status = usb_submit_urb(prUsbReq->prUrb, GFP_ATOMIC);
	if (u4Status) {
		DBGLOG(HAL, ERROR, "usb_submit_urb() reports error (%d) [%s] (EP%d OUT)\n",
				u4Status, __func__, arTcToUSBEP[ucTc]);
		halTxUSBProcessMsduDone(prHifInfo->prGlueInfo, prUsbReq);
		prBufCtrl->u4WrIdx = 0;
		usb_unanchor_urb(prUsbReq->prUrb);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rTxDataFreeQ, prUsbReq, &prHifInfo->rTxDataFreeQLock, FALSE);
		return WLAN_STATUS_FAILURE;
	}
#endif

	if (wlanIsChipRstRecEnabled(prGlueInfo->prAdapter)
			&& wlanIsChipNoAck(prGlueInfo->prAdapter)) {
		wlanChipRstPreAct(prGlueInfo->prAdapter);
		DBGLOG(HAL, ERROR, "usb trigger whole reset\n");
		HAL_WIFI_FUNC_CHIP_RESET(prGlueInfo->prAdapter);
	}
	return u4Status;
}

uint32_t halTxUSBKickData(IN struct GLUE_INFO *prGlueInfo)
{
#if CFG_USB_TX_AGG
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
	uint8_t ucTc;
	unsigned long flags;

	spin_lock_irqsave(&prHifInfo->rTxDataFreeQLock, flags);

	for (ucTc = TC0_INDEX; ucTc < USB_TC_NUM; ucTc++) {
		if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc]))
			continue;

		prUsbReq = list_entry(prHifInfo->rTxDataFreeQ[ucTc].next, struct USB_REQ, list);
		prBufCtrl = prUsbReq->prBufCtrl;

		if (prBufCtrl->u4WrIdx)
			halTxUSBSendAggData(prHifInfo, ucTc, prUsbReq);
	}

	spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
#endif

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
	/*tasklet_hi_schedule(&prGlueInfo->rTxCompleteTask);*/
	tasklet_schedule(&prGlueInfo->rTxCompleteTask);
#endif
}

void halTxUSBProcessMsduDone(IN struct GLUE_INFO *prGlueInfo, struct USB_REQ *prUsbReq)
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
		nicTxGetPageCount(prGlueInfo->prAdapter, u4SentDataSize, TRUE), TRUE);
}

void halTxUSBProcessDataComplete(IN struct ADAPTER *prAdapter, struct USB_REQ *prUsbReq)
{
	uint8_t ucTc;
	u_int8_t fgFfa;
	struct urb *urb = prUsbReq->prUrb;
	struct GL_HIF_INFO *prHifInfo = prUsbReq->prHifInfo;
#if CFG_USB_TX_AGG
	struct BUF_CTRL *prBufCtrl = prUsbReq->prBufCtrl;
#endif
	unsigned long flags;

	ucTc = *((uint8_t *)&prUsbReq->prPriv) & TC_MASK;
	fgFfa =  *((uint8_t *)&prUsbReq->prPriv) & FFA_MASK;

	if (urb->status != 0) {
		DBGLOG(TX, ERROR, "[%s] send DATA fail (status = %d)\n", __func__, urb->status);
		/* TODO: handle error */
	}

	halTxUSBProcessMsduDone(prAdapter->prGlueInfo, prUsbReq);

	spin_lock_irqsave(&prHifInfo->rTxDataFreeQLock, flags);
#if CFG_USB_TX_AGG
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
#else
	list_add_tail(&prUsbReq->list, &prHifInfo->rTxDataFreeQ);
#endif
	spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);

	if (!HAL_IS_TX_DIRECT(prAdapter)) {
		if (kalGetTxPendingCmdCount(prAdapter->prGlueInfo) > 0 || wlanGetTxPendingFrameCount(prAdapter) > 0)
			kalSetEvent(prAdapter->prGlueInfo);
		kalSetTxEvent2Hif(prAdapter->prGlueInfo);
	}
}

uint32_t halRxUSBEnqueueRFB(IN struct ADAPTER *prAdapter, IN uint8_t *pucBuf, IN uint32_t u4Length,
	IN uint32_t u4MinRfbCnt)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct RX_CTRL *prRxCtrl = &prAdapter->rRxCtrl;
	struct SW_RFB *prSwRfb = (struct SW_RFB *) NULL;
	struct HW_MAC_RX_DESC *prRxStatus;
	uint32_t u4RemainCount;
	uint16_t u2RxByteCount;
	uint8_t *pucRxFrame;
	uint32_t u4EnqCnt = 0;
#if CFG_TCP_IP_CHKSUM_OFFLOAD
	uint32_t *pu4HwAppendDW;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	pucRxFrame = pucBuf;
	u4RemainCount = u4Length;
	while (u4RemainCount > 4) {
		u2RxByteCount = HAL_RX_STATUS_GET_RX_BYTE_CNT((struct HW_MAC_RX_DESC *) pucRxFrame);
		u2RxByteCount = ALIGN_4(u2RxByteCount) + LEN_USB_RX_PADDING_CSO;

		if (u2RxByteCount <= CFG_RX_MAX_PKT_SIZE) {
			prSwRfb = NULL;
			KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
			if (prRxCtrl->rFreeSwRfbList.u4NumElem > u4MinRfbCnt)
				QUEUE_REMOVE_HEAD(&prRxCtrl->rFreeSwRfbList, prSwRfb, struct SW_RFB *);
			KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

			if (!prSwRfb)
				return (u4Length - u4RemainCount);

			kalMemCopy(prSwRfb->pucRecvBuff, pucRxFrame, u2RxByteCount);

			prRxStatus = prSwRfb->prRxStatus;
			ASSERT(prRxStatus);

			prSwRfb->ucPacketType = (uint8_t) HAL_RX_STATUS_GET_PKT_TYPE(prRxStatus);
			/* DBGLOG(RX, TRACE, ("ucPacketType = %d\n", prSwRfb->ucPacketType)); */
#if CFG_TCP_IP_CHKSUM_OFFLOAD
			pu4HwAppendDW = (uint32_t *) prRxStatus;
			pu4HwAppendDW += (ALIGN_4(prRxStatus->u2RxByteCount) >> 2);
			prSwRfb->u4TcpUdpIpCksStatus = *pu4HwAppendDW;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */
#if DBG
			DBGLOG(RX, TRACE, "Rx status flag = %x wlan index = %d SecMode = %d\n",
			       prRxStatus->u2StatusFlag, prRxStatus->ucWlanIdx, HAL_RX_STATUS_GET_SEC_MODE(prRxStatus));
#endif
			if (HAL_IS_RX_DIRECT(prAdapter)) {
				switch (prSwRfb->ucPacketType) {
				case RX_PKT_TYPE_RX_DATA:
#if CFG_SUPPORT_SNIFFER
					if (prGlueInfo->fgIsEnableMon) {
						nicRxProcessMonitorPacket(prAdapter, prSwRfb);
						break;
					}
#endif
					spin_lock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_RX_DIRECT]);
					nicRxProcessDataPacket(prAdapter, prSwRfb);
					spin_unlock_bh(&prGlueInfo->rSpinLock[SPIN_LOCK_RX_DIRECT]);
					break;
				default:
					KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
					QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList, &prSwRfb->rQueEntry);
					KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
					u4EnqCnt++;
					break;
				}
			} else {
				KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
				QUEUE_INSERT_TAIL(&prRxCtrl->rReceivedRfbList, &prSwRfb->rQueEntry);
				KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_QUE);
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

uint32_t halRxUSBReceiveEvent(IN struct ADAPTER *prAdapter, IN u_int8_t fgFillUrb)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (/*prGlueInfo->ulFlag & GLUE_FLAG_HALT || */prHifInfo->state != USB_STATE_LINK_UP)
		return WLAN_STATUS_FAILURE;

	while (1) {
		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, &prHifInfo->rRxEventQLock);
		if (prUsbReq == NULL)
			return WLAN_STATUS_RESOURCES;

		usb_anchor_urb(prUsbReq->prUrb, &prHifInfo->rRxEventAnchor);

		prUsbReq->prBufCtrl->u4ReadSize = 0;
		if (prHifInfo->eEventEpType == EVENT_EP_TYPE_INTR && fgFillUrb) {
			usb_fill_int_urb(prUsbReq->prUrb,
					  prHifInfo->udev,
					  usb_rcvintpipe(prHifInfo->udev, USB_EVENT_EP_IN),
					  (void *)prUsbReq->prBufCtrl->pucBuf, prUsbReq->prBufCtrl->u4BufSize,
					  halRxUSBReceiveEventComplete, (void *)prUsbReq, 1);
		} else if (prHifInfo->eEventEpType == EVENT_EP_TYPE_BULK) {
			usb_fill_bulk_urb(prUsbReq->prUrb,
					  prHifInfo->udev,
					  usb_rcvbulkpipe(prHifInfo->udev, USB_EVENT_EP_IN),
					  (void *)prUsbReq->prBufCtrl->pucBuf, prUsbReq->prBufCtrl->u4BufSize,
					  halRxUSBReceiveEventComplete, (void *)prUsbReq);
		}
		u4Status = usb_submit_urb(prUsbReq->prUrb, GFP_ATOMIC);

		if (u4Status) {
			DBGLOG(HAL, ERROR, "usb_submit_urb() reports error (%d) [%s] (EP%d IN)\n",
				u4Status, __func__, (USB_EVENT_EP_IN & 0x0F));
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

	if (/*prGlueInfo->ulFlag & GLUE_FLAG_HALT || */prHifInfo->state != USB_STATE_LINK_UP) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
		return;
	}

	/* Hif power off wifi, drop rx packets and continue polling RX packets until RX path empty */
	if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
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

		/*tasklet_hi_schedule(&prGlueInfo->rRxTask);*/
		tasklet_schedule(&prGlueInfo->rRxTask);
	} else {
		DBGLOG(RX, ERROR, "[%s] receive EVENT fail (status = %d)\n", __func__, urb->status);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxEventFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);

		halRxUSBReceiveEvent(prGlueInfo->prAdapter, FALSE);
	}
#endif
}

uint32_t halRxUSBReceiveData(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct USB_REQ *prUsbReq;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (/*prGlueInfo->ulFlag & GLUE_FLAG_HALT || */prHifInfo->state != USB_STATE_LINK_UP)
		return WLAN_STATUS_FAILURE;

	while (1) {
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
		u4Status = usb_submit_urb(prUsbReq->prUrb, GFP_ATOMIC);
		if (u4Status) {
			DBGLOG(HAL, ERROR, "usb_submit_urb() reports error (%d) [%s]\n", u4Status, __func__);
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

	if (/*prGlueInfo->ulFlag & GLUE_FLAG_HALT || */prHifInfo->state != USB_STATE_LINK_UP) {
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);
		return;
	}

	/* Hif power off wifi, drop rx packets and continue polling RX packets until RX path empty */
	if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
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

		/*tasklet_hi_schedule(&prGlueInfo->rRxTask);*/
		tasklet_schedule(&prGlueInfo->rRxTask);
	} else {
		DBGLOG(RX, ERROR, "[%s] receive DATA fail (status = %d)\n", __func__, urb->status);
		glUsbEnqueueReq(prHifInfo, &prHifInfo->rRxDataFreeQ, prUsbReq, &prHifInfo->rRxDataQLock, FALSE);

		halRxUSBReceiveData(prGlueInfo->prAdapter);
	}
#endif
}

void halRxUSBProcessEventDataComplete(IN struct ADAPTER *prAdapter,
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

	/* Process complete event/data */
	prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, &prHifInfo->rRxEventQLock);
	while (prUsbReq) {
		prUrb = prUsbReq->prUrb;
		prBufCtrl = prUsbReq->prBufCtrl;

		DBGLOG(RX, LOUD, "[%s] Rx URB[0x%p] Len[%u] Sts[%u]\n", __func__,
			prUrb, prUrb->actual_length, prUrb->status);

		if (prUrb->status != 0) {
			DBGLOG(RX, ERROR, "[%s] receive EVENT/DATA fail (status = %d)\n", __func__, prUrb->status);

			glUsbEnqueueReq(prHifInfo, prFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
			prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, &prHifInfo->rRxEventQLock);
			continue;
		}

		pucBufAddr = prBufCtrl->pucBuf + prBufCtrl->u4ReadSize;
		u4BufLen = prUrb->actual_length - prBufCtrl->u4ReadSize;

		prBufCtrl->u4ReadSize += halRxUSBEnqueueRFB(prAdapter, pucBufAddr, u4BufLen, u4MinRfbCnt);

		if (unlikely(prUrb->actual_length - prBufCtrl->u4ReadSize > 4)) {
			if (s_fgOutOfSwRfb == FALSE) {
				if ((long)jiffies - (long)s_u4OutOfSwRfbPrintLimit > 0) {
					DBGLOG(RX, WARN, "Out of SwRfb!\n");
					s_u4OutOfSwRfbPrintLimit = jiffies + MSEC_TO_JIFFIES(SW_RFB_LOG_LIMIT_MS);
				}
				s_fgOutOfSwRfb = TRUE;
			}
			glUsbEnqueueReq(prHifInfo, prCompleteQ, prUsbReq, &prHifInfo->rRxEventQLock, TRUE);

			set_bit(GLUE_FLAG_RX_BIT, &prGlueInfo->ulFlag);
			wake_up_interruptible(&prGlueInfo->waitq);

			schedule_delayed_work(&prGlueInfo->rRxPktDeAggWork, MSEC_TO_JIFFIES(SW_RFB_RECHECK_MS));
			break;
		}

		if (unlikely(s_fgOutOfSwRfb == TRUE))
			s_fgOutOfSwRfb = FALSE;

		glUsbEnqueueReq(prHifInfo, prFreeQ, prUsbReq, &prHifInfo->rRxEventQLock, FALSE);
		prUsbReq = glUsbDequeueReq(prHifInfo, prCompleteQ, &prHifInfo->rRxEventQLock);
	}
}

/*----------------------------------------------------------------------------*/
/*!
* @brief enable global interrupt
*
* @param prAdapter pointer to the Adapter handler
*
* @return (none)
*/
/*----------------------------------------------------------------------------*/
void halEnableInterrupt(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	halRxUSBReceiveData(prAdapter);
	if (prHifInfo->eEventEpType != EVENT_EP_TYPE_DATA_EP)
		halRxUSBReceiveEvent(prAdapter, TRUE);

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
void halDisableInterrupt(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;

	usb_kill_anchored_urbs(&prHifInfo->rRxDataAnchor);
	usb_kill_anchored_urbs(&prHifInfo->rRxEventAnchor);

	glUdmaRxAggEnable(prGlueInfo, FALSE);
	prAdapter->fgIsIntEnable = FALSE;
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
u_int8_t halSetDriverOwn(IN struct ADAPTER *prAdapter)
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
void halSetFWOwn(IN struct ADAPTER *prAdapter, IN u_int8_t fgEnableGlobalInt)
{
}

void halWakeUpWiFi(IN struct ADAPTER *prAdapter)
{
	u_int8_t fgResult;
	uint8_t ucCount = 0;

#if CFG_SUPPORT_PMIC_SPI_CLOCK_SWITCH
	uint32_t u4Value = 0;
	/*E1 PMIC clock workaround*/
	HAL_MCR_RD(prAdapter, TOP_CKGEN2_CR_PMIC_CK_MANUAL, &u4Value);

	if ((TOP_CKGEN2_CR_PMIC_CK_MANUAL_MASK & u4Value) == 0)
		HAL_MCR_WR(prAdapter, TOP_CKGEN2_CR_PMIC_CK_MANUAL, (TOP_CKGEN2_CR_PMIC_CK_MANUAL_MASK|u4Value));
	HAL_MCR_RD(prAdapter, TOP_CKGEN2_CR_PMIC_CK_MANUAL, &u4Value);
	DBGLOG(INIT, INFO, "PMIC SPI clock switch = %s\n",
		(TOP_CKGEN2_CR_PMIC_CK_MANUAL_MASK&u4Value)?"SUCCESS":"FAIL");
#endif

	DBGLOG(INIT, INFO, "Power on Wi-Fi....\n");

	HAL_WIFI_FUNC_READY_CHECK(prAdapter, WIFI_FUNC_INIT_DONE, &fgResult);

	while (!fgResult) {
		HAL_WIFI_FUNC_POWER_ON(prAdapter);
		kalMdelay(50);
		HAL_WIFI_FUNC_READY_CHECK(prAdapter, WIFI_FUNC_INIT_DONE, &fgResult);

		ucCount++;

		if (ucCount >= 5) {
			DBGLOG(INIT, WARN, "Power on failed!!!\n");
			break;
		}
	}

	prAdapter->fgIsFwOwn = FALSE;
}

void halEnableFWDownload(IN struct ADAPTER *prAdapter, IN u_int8_t fgEnable)
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

void halDevInit(IN struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;

	ASSERT(prAdapter);
	prGlueInfo = prAdapter->prGlueInfo;

	glUdmaRxAggEnable(prGlueInfo, FALSE);
	glUdmaTxRxEnable(prGlueInfo, TRUE);
}

u_int8_t halTxIsDataBufEnough(IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo)
{
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
#if CFG_USB_TX_AGG
	struct USB_REQ *prUsbReq;
	struct BUF_CTRL *prBufCtrl;
#endif
	uint8_t ucTc;
	struct sk_buff *skb;
	uint32_t u4Length;
	struct mt66xx_chip_info *prChipInfo;

	unsigned long flags;

	prChipInfo = prAdapter->chip_info;
	skb = (struct sk_buff *)prMsduInfo->prPacket;
	u4Length = skb->len;
	u4Length += prChipInfo->u2HifTxdSize;
	ucTc = USB_TRANS_MSDU_TC(prMsduInfo);

	spin_lock_irqsave(&prHifInfo->rTxDataFreeQLock, flags);

#if CFG_USB_TX_AGG
	if (list_empty(&prHifInfo->rTxDataFreeQ[ucTc])) {
		if (glUsbBorrowFfaReq(prHifInfo, ucTc) == FALSE) {
			spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
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
				spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
				return FALSE;
			}
		}
	}
	prHifInfo->u4AggRsvSize[ucTc] += ALIGN_4(u4Length);
#else
	if (list_empty(&prHifInfo->rTxDataFreeQ)) {
		spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);

		return FALSE;
	}
#endif

	spin_unlock_irqrestore(&prHifInfo->rTxDataFreeQLock, flags);
	return TRUE;
}

uint8_t halTxRingDataSelect(IN struct ADAPTER *prAdapter,
	IN struct MSDU_INFO *prMsduInfo)
{
	return TX_RING_DATA0_IDX_0;
}

uint32_t halUpdateTxMaxQuota(IN struct ADAPTER *prAdapter,
	IN uint16_t u2Port, IN uint32_t u4MaxQuota)
{
	return WLAN_STATUS_SUCCESS;
}

void halProcessTxInterrupt(IN struct ADAPTER *prAdapter)
{
#if CFG_USB_TX_HANDLE_IN_HIF_THREAD
	struct USB_REQ *prUsbReq;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	/* Process complete Tx cmd */
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxCmdCompleteQ, &prHifInfo->rTxCmdQLock);
	while (prUsbReq) {
		halTxUSBProcessCmdComplete(prAdapter, prUsbReq);
		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxCmdCompleteQ, &prHifInfo->rTxCmdQLock);
	}

	/* Process complete Tx data */
	prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ, &prHifInfo->rTxCmdQLock);
	while (prUsbReq) {
		halTxUSBProcessDataComplete(prAdapter, prUsbReq);
		prUsbReq = glUsbDequeueReq(prHifInfo, &prHifInfo->rTxDataCompleteQ, &prHifInfo->rTxCmdQLock);
	}
#endif
}

bool halHifSwInfoInit(IN struct ADAPTER *prAdapter)
{

}

void halRxProcessMsduReport(IN struct ADAPTER *prAdapter, IN OUT struct SW_RFB *prSwRfb)
{

}

uint32_t halTxGetPageCount(IN struct ADAPTER *prAdapter, IN uint32_t u4FrameLength, IN u_int8_t fgIncludeDesc)
{
#if CFG_USB_TX_AGG
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
#else
	return 1;
#endif
}

uint32_t halTxPollingResource(IN struct ADAPTER *prAdapter, IN uint8_t ucTC)
{
	return WLAN_STATUS_SUCCESS;
}

void halSerHifReset(IN struct ADAPTER *prAdapter)
{
}

void halProcessRxInterrupt(IN struct ADAPTER *prAdapter)
{
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

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
}

uint32_t halDumpHifStatus(IN struct ADAPTER *prAdapter, IN uint8_t *pucBuf, IN uint32_t u4Max)
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

void halGetCompleteStatus(IN struct ADAPTER *prAdapter, OUT uint32_t *pu4IntStatus)
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

u_int8_t halIsPendingRx(IN struct ADAPTER *prAdapter)
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

void halUSBPreSuspendCmd(IN struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl;
	uint32_t rStatus;

	rCmdHifCtrl.ucHifType = ENUM_HIF_TYPE_USB;
	rCmdHifCtrl.ucHifDirection = ENUM_HIF_TX;
	rCmdHifCtrl.ucHifStop = 1;
#if 0
	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				      CMD_ID_HIF_CTRL,	/* ucCID */
				      FALSE,	/* fgSetQuery */
				      TRUE,	/* fgNeedResp */
				      FALSE,	/* fgIsOid */
				      usbPreSuspendDone,	/* pfCmdDoneHandler */
				      usbPreSuspendTimeout,	/* pfCmdTimeoutHandler */
				      sizeof(struct CMD_HIF_CTRL),	/* u4SetQueryInfoLen */
				      (uint8_t *)&rCmdHifCtrl,	/* pucInfoBuffer */
				      NULL,	/* pvSetQueryBuffer */
				      0	/* u4SetQueryBufferLen */
	    );
#else
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
#endif

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}

void halUSBPreSuspendDone(IN struct ADAPTER *prAdapter, IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.state = USB_STATE_PRE_SUSPEND_DONE;
}

void halUSBPreSuspendTimeout(IN struct ADAPTER *prAdapter, IN struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.state = USB_STATE_PRE_SUSPEND_FAIL;
}

uint32_t halGetValidCoalescingBufSize(IN struct ADAPTER *prAdapter)
{
	uint32_t u4BufSize;

	if (HIF_TX_COALESCING_BUFFER_SIZE > HIF_RX_COALESCING_BUFFER_SIZE)
		u4BufSize = HIF_TX_COALESCING_BUFFER_SIZE;
	else
		u4BufSize = HIF_RX_COALESCING_BUFFER_SIZE;

	return u4BufSize;
}

uint32_t halAllocateIOBuffer(IN struct ADAPTER *prAdapter)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t halReleaseIOBuffer(IN struct ADAPTER *prAdapter)
{
	return WLAN_STATUS_SUCCESS;
}

void halProcessAbnormalInterrupt(IN struct ADAPTER *prAdapter)
{

}

void halProcessSoftwareInterrupt(IN struct ADAPTER *prAdapter)
{

}

void halDeAggRxPktWorker(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo = ENTRY_OF(work, struct GLUE_INFO, rRxPktDeAggWork);

	/*tasklet_hi_schedule(&prGlueInfo->rRxTask);*/
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
uint32_t halHifPowerOffWifi(IN struct ADAPTER *prAdapter)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(INIT, INFO, "Power off Wi-Fi!\n");

	/* Power off Wi-Fi */
	wlanSendNicPowerCtrlCmd(prAdapter, TRUE);

	rStatus = wlanCheckWifiFunc(prAdapter, FALSE);

	prAdapter->prGlueInfo->rHifInfo.state = USB_STATE_WIFI_OFF;

	nicDisableInterrupt(prAdapter);

	wlanClearPendingInterrupt(prAdapter);

	halTxCancelAllSending(prAdapter);

	return rStatus;
}

void halPrintHifDbgInfo(IN struct ADAPTER *prAdapter)
{

}

u_int8_t halIsTxResourceControlEn(IN struct ADAPTER *prAdapter)
{
	return FALSE;
}

void halTxResourceResetHwTQCounter(IN struct ADAPTER *prAdapter)
{
}

uint32_t halGetHifTxPageSize(IN struct ADAPTER *prAdapter)
{
	return HIF_TX_PAGE_SIZE;
}
