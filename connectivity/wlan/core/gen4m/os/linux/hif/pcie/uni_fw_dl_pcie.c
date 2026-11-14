// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   uni_fw_dl.c
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "uni_fw_dl_pcie.h"
#include "uni_fw_dl_shm_ops.h"
#include "doorbell.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define MAX_MSG_TRIGGER_IDX		0xFFFF
#define MAX_NOTIF_TRIGGER_IDX		0xFFFF
#if (CFG_MTK_FPGA_PLATFORM != 0)
#define MSG_TIMEOUT_MS			(1000 * 5) /* ms */
#define SLOT_TIMEOUT_MS			(1000 * 30) /* ms */
#else
#define MSG_TIMEOUT_MS			(100 * 1) /* ms */
#define SLOT_TIMEOUT_MS			(1000 * 1) /* ms */
#endif /* CFG_MTK_FPGA_PLATFORM */
#define DEFAULT_SLOT_NUM		(512)
#define DEFAULT_SLOT_SIZE		(2 * 1024)
#define MIN_SLOT_NUM			(2)

#define INC_MSG_TRIGGER_IDX(__idx) \
	((__idx) = ((__idx) + 1) % MAX_MSG_TRIGGER_IDX)

#define INC_SLOT_RDY_IDX(__idx, __max_cnt) \
	((__idx) = ((__idx) + 1) % (__max_cnt))

#define IS_SLOT_RING_BUFF_FULL(__rdy_idx, __done_idx, __max_num) \
	((((__rdy_idx) + 1) % (__max_num)) == (__done_idx))

#define IS_SLOT_RING_BUFF_EMPTY(__rdy_idx, __done_idx, __max_num) \
	((__rdy_idx) == (__done_idx))

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static u_int8_t
uniFwdlPcieWaitMsgDone(struct ADAPTER *prAdapter,
		       struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
		       uint32_t u4TimeoutMs)
{
	uint32_t u4Time = kalGetTimeTick();
	u_int8_t fgDone = FALSE;

	do {
		if (prUniFwdlPcieCtx->u2MsgTriggerIdx ==
		    prUniFwdlPcieCtx->u2MsgDoneIdx) {
			fgDone = TRUE;
			break;
		}

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				      MSEC_TO_SYSTIME(u4TimeoutMs))) {
			DBGLOG(UNI_FWDL, ERROR,
				"TIMEOUT, trigger_idx=%u done_idx=%u\n",
				prUniFwdlPcieCtx->u2MsgTriggerIdx,
				prUniFwdlPcieCtx->u2MsgDoneIdx);
			uniFwdlShmDump(prAdapter, &prUniFwdlPcieCtx->rShmCtx);
			break;
		}

		kalMsleep(10);
	} while (TRUE);

	return fgDone;
}

static u_int8_t
uniFwdlPcieWaitSlotNonFull(struct ADAPTER *prAdapter,
			   struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
			   uint32_t u4TimeoutMs)
{
	struct SLOT_TABLE *prSlotTable = &prUniFwdlPcieCtx->rSlotTable;
	uint32_t u4Time = kalGetTimeTick();
	u_int8_t fgNonFull = FALSE;

	do {
		if (!IS_SLOT_RING_BUFF_FULL(prSlotTable->u4SlotRdyIdx,
					    prSlotTable->u4SlotDoneIdx,
					    prSlotTable->u4SlotNum)) {
			fgNonFull = TRUE;
			break;
		}

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				      MSEC_TO_SYSTIME(u4TimeoutMs))) {
			DBGLOG(UNI_FWDL, ERROR,
				"TIMEOUT, rdy_idx=%u done_idx=%u num=%u\n",
				prSlotTable->u4SlotRdyIdx,
				prSlotTable->u4SlotDoneIdx,
				prSlotTable->u4SlotNum);
			break;
		}

		kalMsleep(10);
	} while (TRUE);

	return fgNonFull;
}

static u_int8_t
uniFwdlPcieWaitSlotEmpty(struct ADAPTER *prAdapter,
			 struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
			 uint32_t u4TimeoutMs)
{
	struct SLOT_TABLE *prSlotTable = &prUniFwdlPcieCtx->rSlotTable;
	uint32_t u4Time = kalGetTimeTick();
	u_int8_t fgEmpty = FALSE;

	do {
		if (IS_SLOT_RING_BUFF_EMPTY(prSlotTable->u4SlotRdyIdx,
					    prSlotTable->u4SlotDoneIdx,
					    prSlotTable->u4SlotNum)) {
			fgEmpty = TRUE;
			break;
		}

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				      MSEC_TO_SYSTIME(u4TimeoutMs))) {
			DBGLOG(UNI_FWDL, ERROR,
				"TIMEOUT, rdy_idx=%u done_idx=%u num=%u\n",
				prSlotTable->u4SlotRdyIdx,
				prSlotTable->u4SlotDoneIdx,
				prSlotTable->u4SlotNum);
			break;
		}

		kalMsleep(10);
	} while (TRUE);

	return fgEmpty;
}

static u_int8_t
uniFwdlPcieWaitCommState(struct ADAPTER *prAdapter,
			 struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
			 enum UNI_FWDL_COMM_STATE eTargetState,
			 uint32_t u4TimeoutMs)
{
	uint32_t u4CurrCommState = 0, u4Time = kalGetTimeTick();
	u_int8_t fgDone = FALSE;

	do {
		uniFwdlShmRdCommState(&prUniFwdlPcieCtx->rShmCtx,
				      &u4CurrCommState);
		if (eTargetState ==
		    (enum UNI_FWDL_COMM_STATE)u4CurrCommState) {
			fgDone = TRUE;
			break;
		}

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				      MSEC_TO_SYSTIME(u4TimeoutMs))) {
			DBGLOG(UNI_FWDL, ERROR,
				"TIMEOUT(%u) target=%u current=%u\n",
				u4TimeoutMs, eTargetState, u4CurrCommState);
			break;
		}

		kalMsleep(10);
	} while (TRUE);

	return fgDone;
}

static u_int8_t
uniFwdlPcieWaitNotif(struct ADAPTER *prAdapter,
		     struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
		     uint16_t u2ExpectNotifId,
		     uint32_t u4TimeoutMs)
{
	uint32_t u4Time = kalGetTimeTick();
	u_int8_t fgDone = FALSE;

	do {
		if (prUniFwdlPcieCtx->u2LastNotifId == u2ExpectNotifId) {
			fgDone = TRUE;
			break;
		}

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				      MSEC_TO_SYSTIME(u4TimeoutMs))) {
			DBGLOG(UNI_FWDL, ERROR,
				"TIMEOUT(%u) expect=%u last=%u\n",
				u4TimeoutMs,
				u2ExpectNotifId,
				prUniFwdlPcieCtx->u2LastNotifId);
			break;
		}

		kalMsleep(10);
	} while (TRUE);

	return fgDone;
}

static void
uniFwdlPcieTriggerWfFwdlDoorbell(struct ADAPTER *prAdapter,
				 struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
				 u_int8_t fgSlotDl)
{
	uint32_t u4Timeout =
		MSEC_TO_SYSTIME(prUniFwdlPcieCtx->u4MinDoorbellTime);

	if (fgSlotDl)
		goto trigger;

	do {
		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(),
				      prUniFwdlPcieCtx->u4LastDoorbellTime,
				      u4Timeout))
			break;

		kalUsleep_range(900, 1000);
	} while (TRUE);

	prUniFwdlPcieCtx->u4LastDoorbellTime = kalGetTimeTick();

trigger:
	triggerWfFwdlDoorbell(prAdapter);
}

static struct SLOT_ENTRY *uniFwdlPcieAllocSlot(struct GL_HIF_INFO *prHif,
					       uint32_t u4SlotSize)
{
	struct SLOT_ENTRY *prEntry;

	prEntry = (struct SLOT_ENTRY *)
		kalMemZAlloc(sizeof(*prEntry), VIR_MEM_TYPE);
	if (!prEntry)
		return NULL;

	prEntry->rDmaMem.align_size = u4SlotSize;
	prEntry->rDmaMem.va =
		KAL_DMA_ALLOC_COHERENT(prHif->pdev,
				       prEntry->rDmaMem.align_size,
				       &prEntry->rDmaMem.pa);
	if (!prEntry->rDmaMem.va) {
		DBGLOG(UNI_FWDL, WARN,
			"Alloc slot (size=%u) failed\n",
			u4SlotSize);
		kalMemFree(prEntry, VIR_MEM_TYPE, sizeof(*prEntry));
		return NULL;
	}

	kalMemZero(prEntry->rDmaMem.va, prEntry->rDmaMem.align_size);

	return prEntry;
}

static void uniFwdlPcieFreeSlot(struct GL_HIF_INFO *prHif,
				struct SLOT_ENTRY *prEntry)
{
	if (prEntry) {
		if (prEntry->rDmaMem.va && prEntry->rDmaMem.align_size)
			KAL_DMA_FREE_COHERENT(prHif->pdev,
					      prEntry->rDmaMem.align_size,
					      prEntry->rDmaMem.va,
					      prEntry->rDmaMem.pa);

		kalMemFree(prEntry, VIR_MEM_TYPE, sizeof(*prEntry));
	}
}

static uint32_t uniFwdlPcieAllocSlots(struct GL_HIF_INFO *prHif,
				      uint32_t u4FwSize,
				      struct LINK *prSlotList,
				      uint32_t u4MaxSlotNum,
				      uint32_t *pu4SlotSize,
				      u_int8_t *pfgSlotDlMode)
{
	struct SLOT_ENTRY *prEntry, *prNextEntry;
	uint32_t u4Idx = 0;

	/* Non-ACK mode */
	if (*pfgSlotDlMode == FALSE) {
		prEntry = uniFwdlPcieAllocSlot(prHif, ALIGN_4(u4FwSize));

		if (prEntry) {
			*pu4SlotSize = ALIGN_4(u4FwSize);

			DBGLOG(UNI_FWDL, TRACE,
				"Slot entry pa=%pa size=0x%x\n",
				&prEntry->rDmaMem.pa,
				prEntry->rDmaMem.align_size);

			LINK_INSERT_TAIL(prSlotList, &prEntry->rLinkEntry);

			return WLAN_STATUS_SUCCESS;
		}

		if (ALIGN_4(u4FwSize) <= DEFAULT_SLOT_SIZE) {
			DBGLOG(UNI_FWDL, ERROR,
				"Alloc slot size (%u) <= Default slot size (%u), failed directly\n",
				ALIGN_4(u4FwSize), DEFAULT_SLOT_SIZE);
			return WLAN_STATUS_FAILURE;
		}

		DBGLOG(UNI_FWDL, INFO,
			"Alloc slot size (%u) failed, fallback to slot DL mode\n",
			ALIGN_4(u4FwSize));
		*pfgSlotDlMode = TRUE;
	}

	/* ACK mode */
	for (u4Idx = 0; u4Idx < u4MaxSlotNum; u4Idx++) {
		prEntry = uniFwdlPcieAllocSlot(prHif, DEFAULT_SLOT_SIZE);

		if (!prEntry) {
			DBGLOG(UNI_FWDL, WARN,
				"[%u] Alloc slot failed, size=%u\n",
				u4Idx, DEFAULT_SLOT_SIZE);
			break;
		}

		LINK_INSERT_TAIL(prSlotList, &prEntry->rLinkEntry);
	}

	if (prSlotList->u4NumElem < MIN_SLOT_NUM) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc slot num(%u) < min required num(%u)\n",
			prSlotList->u4NumElem, MIN_SLOT_NUM);
		LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prSlotList,
					 rLinkEntry, struct SLOT_ENTRY) {
			uniFwdlPcieFreeSlot(prHif, prEntry);
			LINK_REMOVE_KNOWN_ENTRY(prSlotList,
						&(prEntry->rLinkEntry));
		}
		return WLAN_STATUS_FAILURE;
	}

	*pu4SlotSize = DEFAULT_SLOT_SIZE;

	return WLAN_STATUS_SUCCESS;
}

static void uniFwdlPcieFreeSlots(struct GL_HIF_INFO *prHif,
				 struct LINK *prSlotList)
{
	struct SLOT_ENTRY *prEntry, *prNextEntry;

	LINK_FOR_EACH_ENTRY_SAFE(prEntry, prNextEntry, prSlotList,
				 rLinkEntry, struct SLOT_ENTRY) {
		uniFwdlPcieFreeSlot(prHif, prEntry);
	}
}

static uint32_t
uniFwdlPcieAllocSlotTable(struct GL_HIF_INFO *prHif,
			  struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
			  struct LINK *prSlotList,
			  uint32_t u4SlotSize)
{
	struct SLOT_TABLE *prSlotTable = &prUniFwdlPcieCtx->rSlotTable;
	struct SLOT_ENTRY *prEntry;
	uint32_t u4Idx = 0;

	prSlotTable->u4SlotNum = prSlotList->u4NumElem;
	prSlotTable->u4SlotSize = u4SlotSize;
	prSlotTable->rDmaMem.align_size = prSlotList->u4NumElem *
		sizeof(uint64_t);
	prSlotTable->rDmaMem.va =
		KAL_DMA_ALLOC_COHERENT(prHif->pdev,
				       prSlotTable->rDmaMem.align_size,
				       &prSlotTable->rDmaMem.pa);
	if (!prSlotTable->rDmaMem.va) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc slot table dma mem %u failed.\n",
			prSlotTable->rDmaMem.align_size);
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(UNI_FWDL, INFO,
		"slot_size=0x%x, slot_num=%u, slot_table pa=%pa\n",
		u4SlotSize,
		prSlotList->u4NumElem,
		&prSlotTable->rDmaMem.pa);

	kalMemZero(prSlotTable->rDmaMem.va, prSlotTable->rDmaMem.align_size);

	LINK_FOR_EACH_ENTRY(prEntry, prSlotList, rLinkEntry,
			    struct SLOT_ENTRY) {
		uint8_t *prAllocVa;
		uint64_t u8Addr;

		prSlotTable->prSlotEntries[u4Idx] = prEntry;

		u8Addr = (uint64_t)prEntry->rDmaMem.pa;
		prAllocVa = (uint8_t *)prSlotTable->rDmaMem.va +
			(u4Idx * sizeof(uint64_t));
		kalMemCopyToIo(prAllocVa, &u8Addr, sizeof(u8Addr));
		DBGLOG(UNI_FWDL, LOUD, "[%u] pa=%pa\n",
			u4Idx, &prEntry->rDmaMem.pa);

		u4Idx++;
	}

	return WLAN_STATUS_SUCCESS;
}

static void
uniFwdlPcieFreeSlotTable(struct GL_HIF_INFO *prHif,
			 struct SLOT_TABLE *prSlotTable)
{
	if (prSlotTable && prSlotTable->rDmaMem.va &&
	    prSlotTable->rDmaMem.align_size)
		KAL_DMA_FREE_COHERENT(prHif->pdev,
				      prSlotTable->rDmaMem.align_size,
				      prSlotTable->rDmaMem.va,
				      prSlotTable->rDmaMem.pa);
}

static uint32_t
uniFwdlPcieAllocXtalPkt(struct GL_HIF_INFO *prHif,
			struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx,
			uint32_t u4XtalPktSize)
{
	struct HIF_MEM *prXtalMem = &prUniFwdlPcieCtx->rXtalMem;

	prXtalMem->align_size = u4XtalPktSize;
	prXtalMem->va =
		KAL_DMA_ALLOC_COHERENT(prHif->pdev,
				       prXtalMem->align_size,
				       &prXtalMem->pa);

	if (!prXtalMem->va) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc xtal pkt %u failed\n",
			prXtalMem->align_size);
		return WLAN_STATUS_FAILURE;
	}

	kalMemZero(prXtalMem->va, prXtalMem->align_size);

	return WLAN_STATUS_FAILURE;
}

static void uniFwdlPcieFreeXtalPkt(struct GL_HIF_INFO *prHif,
				   struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx)
{
	struct HIF_MEM *prXtalMem = &prUniFwdlPcieCtx->rXtalMem;

	if (prXtalMem->va && prXtalMem->align_size)
		KAL_DMA_FREE_COHERENT(prHif->pdev,
				      prXtalMem->align_size,
				      prXtalMem->va,
				      prXtalMem->pa);
}

uint32_t uniFwdlPcieInit(struct ADAPTER *prAdapter,
			 UNI_FWDL_NOTIF_CB pfnNotifCb,
			 uint32_t u4FwSize)
{
	struct UNI_FWDL_INFO *prUniFwdlInfo =
		prAdapter->chip_info->uni_fwdl_info;
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct GL_HIF_INFO *prHif = &prAdapter->prGlueInfo->rHifInfo;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx = NULL;
	struct UNI_FWLD_SHM_CTX *prShmCtx;
	struct LINK rSlotList;
	uint32_t u4AllocSize, u4SlotSize = 0, u4SyncInfo,
		u4Status = WLAN_STATUS_FAILURE;
	u_int8_t fgSlotDlMode;

	KAL_SPIN_LOCK_DECLARATION();

	LINK_INITIALIZE(&rSlotList);

	u4SyncInfo = prUniFwdlInfo->u4SyncInfo;
	fgSlotDlMode = u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL) ?
		TRUE : FALSE;

	u4Status = uniFwdlPcieAllocSlots(prHif, u4FwSize, &rSlotList,
					 DEFAULT_SLOT_NUM,
					 &u4SlotSize, &fgSlotDlMode);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	u4AllocSize = sizeof(*prUniFwdlPcieCtx) +
		sizeof(struct SLOT_ENTRY *) * rSlotList.u4NumElem;
	prUniFwdlPcieCtx = (struct UNI_FWLD_PCIE_CTX *)
		kalMemZAlloc(u4AllocSize, VIR_MEM_TYPE);
	if (!prUniFwdlPcieCtx) {
		DBGLOG(UNI_FWDL, ERROR,
			"Alloc UNI_FWLD_PCIE_CTX failed, slot_num=%u size=%u\n",
			rSlotList.u4NumElem, u4AllocSize);
		u4Status = WLAN_STATUS_FAILURE;
		goto free_slots;
	}

	prUniFwdlPcieCtx->u4MinDoorbellTime =
		prAdapter->chip_info->uni_fwdl_info->u4MinDoorbellTime;
	prUniFwdlPcieCtx->u4LastDoorbellTime = kalGetTimeTick();
	prUniFwdlPcieCtx->u2LastNotifId = UNI_FWDL_NOTIF_ID_NUM;
	prShmCtx = &prUniFwdlPcieCtx->rShmCtx;
	u4Status = uniFwdlShmInit(prAdapter, prShmCtx);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto free_ctx;

	u4Status = uniFwdlPcieAllocSlotTable(prHif, prUniFwdlPcieCtx,
					     &rSlotList, u4SlotSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto deinit_shm;

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_DL_XTAL_PKT)) {
		u4Status = uniFwdlPcieAllocXtalPkt(prHif, prUniFwdlPcieCtx,
						   prCtx->u4XtalPktSize);
		if (u4Status != WLAN_STATUS_SUCCESS)
			goto free_table;
	}

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_SHM_EMI)) {
		struct UNI_FWDL_MSG_HOST_EMI_ADDR_UPDATE rMsg;

		kalMemZero(&rMsg, sizeof(rMsg));
		rMsg.u8RwShmAddr = (uint64_t)prShmCtx->rRwMem.pa;
		rMsg.u4RwShmSize = prShmCtx->rRwMem.align_size;
		rMsg.u8RoShmAddr = (uint64_t)prShmCtx->rRoMem.pa;
		rMsg.u4RoShmSize = prShmCtx->rRoMem.align_size;

		if (uniFwdlPcieSendMsg(prAdapter,
				       UNI_FWDL_HIF_DL_RADIO_TYPE_WF,
				       UNI_FWDL_MSG_ID_HOST_EMI_ADDR_UPDATE,
				       (uint8_t *)&rMsg,
				       sizeof(rMsg)) != WLAN_STATUS_SUCCESS)
			goto free_xtal_pkt;

		if (uniFwdlPcieWaitNotif(prAdapter, prUniFwdlPcieCtx,
					 UNI_FWDL_NOTIF_ID_EMI_ADDR_UPDATE,
					 MSG_TIMEOUT_MS) == FALSE)
			goto free_xtal_pkt;

		uniFwdlShmChangeEmiMode(&prUniFwdlPcieCtx->rShmCtx);
	}

	if (fgSlotDlMode)
		u4SyncInfo |= BIT(UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL);
	if (prAdapter->chip_info && prAdapter->chip_info->bus_info) {
		struct pcie_msi_info *prMsiInfo;

		prMsiInfo = &prAdapter->chip_info->bus_info->pcie_msi_info;
		if (!prMsiInfo->fgMsiEnabled) {
#if (CFG_SUPPORT_UNI_FWDL_PCIE_INTX == 1)
			u4SyncInfo |= BIT(UNI_FWDL_SYNC_INFO_PCIE_INTX);
#else
			DBGLOG(UNI_FWDL, INFO,
				"PCIE MSI NOT supported.\n",
				u4SyncInfo);
			goto free_xtal_pkt;
#endif /* CFG_SUPPORT_UNI_FWDL_PCIE_INTX */
		}
	}

	prUniFwdlPcieCtx->u4AllocSize = u4AllocSize;
	prUniFwdlPcieCtx->u4SyncInfo = u4SyncInfo;
	prUniFwdlPcieCtx->pfnNotifCb = pfnNotifCb;
	LINK_INITIALIZE(&prUniFwdlPcieCtx->rSlotList);
	LINK_MERGE_TO_HEAD(&prUniFwdlPcieCtx->rSlotList, &rSlotList);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);
	prCtx->prHifCtx = (void *)prUniFwdlPcieCtx;
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);

	DBGLOG(UNI_FWDL, INFO, "u4SyncInfo=0x%x\n", u4SyncInfo);

	u4Status = WLAN_STATUS_SUCCESS;

	goto exit;

free_xtal_pkt:
	uniFwdlPcieFreeXtalPkt(prHif, prUniFwdlPcieCtx);

free_table:
	uniFwdlPcieFreeSlotTable(prHif, &prUniFwdlPcieCtx->rSlotTable);

deinit_shm:
	uniFwdlShmDeInit(prAdapter, prShmCtx);

free_ctx:
	kalMemFree(prUniFwdlPcieCtx, VIR_MEM_TYPE, u4AllocSize);

free_slots:
	uniFwdlPcieFreeSlots(prHif, &rSlotList);

exit:
	return u4Status;
}

void uniFwdlPcieDeInit(struct ADAPTER *prAdapter)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct GL_HIF_INFO *prHif = &prAdapter->prGlueInfo->rHifInfo;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	struct UNI_FWLD_SHM_CTX *prShmCtx = &prUniFwdlPcieCtx->rShmCtx;
	struct LINK rSlotList;

	KAL_SPIN_LOCK_DECLARATION();

	LINK_INITIALIZE(&rSlotList);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);
	prCtx->prHifCtx = NULL;
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);

	LINK_MERGE_TO_HEAD(&rSlotList, &prUniFwdlPcieCtx->rSlotList);
	prUniFwdlPcieCtx->pfnNotifCb = NULL;

	uniFwdlPcieFreeXtalPkt(prHif, prUniFwdlPcieCtx);

	uniFwdlPcieFreeSlotTable(prHif, &prUniFwdlPcieCtx->rSlotTable);

	uniFwdlShmDeInit(prAdapter, prShmCtx);

	kalMemFree(prUniFwdlPcieCtx, VIR_MEM_TYPE,
		   prUniFwdlPcieCtx->u4AllocSize);

	uniFwdlPcieFreeSlots(prHif, &rSlotList);
}

void uniFwdlPcieDump(struct ADAPTER *prAdapter)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	struct UNI_FWLD_SHM_CTX *prShmCtx = &prUniFwdlPcieCtx->rShmCtx;

	uniFwdlShmDump(prAdapter, prShmCtx);
}

uint32_t uniFwdlPcieQueryHwInfo(struct ADAPTER *prAdapter, uint8_t *prBuff,
				uint32_t u4BuffSize)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	struct UNI_FWDL_NOTIF_HW_INFO *prInfo =
		(struct UNI_FWDL_NOTIF_HW_INFO *)prBuff;
	struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *prEntry;
	uint32_t u4Value = 0, u4Size = 0;
	uint8_t ucIdx = 0;
	uint8_t *prPos = prBuff;

	if ((u4Size + sizeof(*prInfo)) > u4BuffSize)
		goto error;

	u4Size += sizeof(*prInfo);
	prPos = prBuff + u4Size;

	if ((u4Size + sizeof(*prEntry) + sizeof(u4Value)) > u4BuffSize)
		goto error;
	uniFwdlShmRdChipId(&prUniFwdlPcieCtx->rShmCtx, &u4Value);
	prEntry = (struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *)prPos;
	prEntry->u2Tag = UNI_FWDL_NOTIF_HW_INFO_TAG_CHIP_ID;
	prEntry->u2Length = sizeof(u4Value);
	kalMemCopy(prEntry->aucData, &u4Value, sizeof(u4Value));
	u4Size += sizeof(*prEntry) + sizeof(u4Value);
	prPos = prBuff + u4Size;
	ucIdx++;

	if ((u4Size + sizeof(*prEntry) + sizeof(u4Value)) > u4BuffSize)
		goto error;
	uniFwdlShmRdHwVer(&prUniFwdlPcieCtx->rShmCtx, &u4Value);
	prEntry = (struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *)prPos;
	prEntry->u2Tag = UNI_FWDL_NOTIF_HW_INFO_TAG_HW_VER;
	prEntry->u2Length = sizeof(u4Value);
	kalMemCopy(prEntry->aucData, &u4Value, sizeof(u4Value));
	u4Size += sizeof(*prEntry) + sizeof(u4Value);
	prPos = prBuff + u4Size;
	ucIdx++;

	if ((u4Size + sizeof(*prEntry) + sizeof(u4Value)) > u4BuffSize)
		goto error;
	uniFwdlShmRdFwVer(&prUniFwdlPcieCtx->rShmCtx, &u4Value);
	prEntry = (struct UNI_FWDL_NOTIF_HW_INFO_ENTRY *)prPos;
	prEntry->u2Tag = UNI_FWDL_NOTIF_HW_INFO_TAG_FW_VER;
	prEntry->u2Length = sizeof(u4Value);
	kalMemCopy(prEntry->aucData, &u4Value, sizeof(u4Value));
	u4Size += sizeof(*prEntry) + sizeof(u4Value);
	prPos = prBuff + u4Size;
	ucIdx++;

	prInfo->u4EntriesNum = ucIdx;

	if (prUniFwdlPcieCtx->pfnNotifCb)
		prUniFwdlPcieCtx->pfnNotifCb(prAdapter,
					     UNI_FWDL_NOTIF_ID_QUERY_HW_INFO,
					     prInfo,
					     u4Size);

	return WLAN_STATUS_SUCCESS;

error:
	return WLAN_STATUS_FAILURE;
}

uint32_t uniFwdlPcieStartDl(struct ADAPTER *prAdapter,
			    struct START_DL_PARAMS *prParams)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	struct SLOT_TABLE *prSlotTable = &prUniFwdlPcieCtx->rSlotTable;
	struct HIF_MEM *prXtalMem = &prUniFwdlPcieCtx->rXtalMem;
	uint32_t u4SyncInfo, u4Status = WLAN_STATUS_SUCCESS;

	u4SyncInfo = prUniFwdlPcieCtx->u4SyncInfo;

	uniFwdlShmWrRadioType(&prUniFwdlPcieCtx->rShmCtx, prParams->eRadio);

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL)) {
		uniFwdlShmWrSecSlotTblAddr(&prUniFwdlPcieCtx->rShmCtx,
					   prSlotTable->rDmaMem.pa);
		uniFwdlShmWrSecSlotTblSize(&prUniFwdlPcieCtx->rShmCtx,
					   prSlotTable->rDmaMem.align_size);
		uniFwdlShmWrSlotNum(&prUniFwdlPcieCtx->rShmCtx,
				    prSlotTable->u4SlotNum);
		uniFwdlShmWrSlotSize(&prUniFwdlPcieCtx->rShmCtx,
				     prSlotTable->u4SlotSize);
		uniFwdlShmWrDlSize(&prUniFwdlPcieCtx->rShmCtx,
				   prParams->u4HeaderSz);

		DBGLOG(UNI_FWDL, INFO,
			"slot table=0x%llx size=0x%x slot_num=%u slot_size=0x%x header_sz=0x%x\n",
			(uint64_t)prSlotTable->rDmaMem.pa,
			prSlotTable->rDmaMem.align_size,
			prSlotTable->u4SlotNum,
			prSlotTable->u4SlotSize,
			prParams->u4HeaderSz);
	} else {
		struct SLOT_ENTRY *prSlotEntry;

		if (!prParams->prFwBuffer || prParams->u4FwSize == 0) {
			DBGLOG(UNI_FWDL, ERROR,
				"Invalid fw binary 0x%p %u\n",
				prParams->prFwBuffer,
				prParams->u4FwSize);
			u4Status = WLAN_STATUS_FAILURE;
			goto exit;
		}

		prSlotEntry = prSlotTable->prSlotEntries[0];
		kalMemCopyToIo(prSlotEntry->rDmaMem.va,
			       prParams->prFwBuffer,
			       prParams->u4FwSize);
		uniFwdlShmWrDlAddr(&prUniFwdlPcieCtx->rShmCtx,
				   prSlotEntry->rDmaMem.pa);
		uniFwdlShmWrDlSize(&prUniFwdlPcieCtx->rShmCtx,
				   prSlotEntry->rDmaMem.align_size);

		DBGLOG(UNI_FWDL, INFO,
			"slot entry[0] pa=0x%llx size=0x%x\n",
			(uint64_t)prSlotEntry->rDmaMem.pa,
			prSlotEntry->rDmaMem.align_size);
	}

	uniFwdlShmWrSyncInfo(&prUniFwdlPcieCtx->rShmCtx, u4SyncInfo);

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_DL_XTAL_PKT)) {
		uniFwdlShmWrCtxInfoAddr_A(&prUniFwdlPcieCtx->rShmCtx,
					  prXtalMem->pa);
		uniFwdlShmWrCtxInfoSize_A(&prUniFwdlPcieCtx->rShmCtx,
					  prXtalMem->align_size);
	}

	uniFwdlShmWrCalibrationAction(&prUniFwdlPcieCtx->rShmCtx,
				      prParams->eAction);

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_CO_DL)) {
		uniFwdlShmWrCoDlRadio(&prUniFwdlPcieCtx->rShmCtx,
				      prParams->eCoDlRadioBitmap);
		uniFwdlShmWrCoDlSecMapSize_WF(&prUniFwdlPcieCtx->rShmCtx,
					      prParams->u4CoDlWfHeaderSz);
		uniFwdlShmWrCoDlSecMapSize_BT(&prUniFwdlPcieCtx->rShmCtx,
					      prParams->u4CoDlBtHeaderSz);
		uniFwdlShmWrCoDlSecMapSize_ZB(&prUniFwdlPcieCtx->rShmCtx,
					      prParams->u4CoDlZbHeaderSz);
	}

	if (u4SyncInfo & BIT(UNI_FWDL_SYNC_INFO_FW_TIMEOUT))
		uniFwdlShmWrTODuration(&prUniFwdlPcieCtx->rShmCtx,
				       prParams->u4FwTimeoutDuration);

	INC_MSG_TRIGGER_IDX(prUniFwdlPcieCtx->u2MsgTriggerIdx);
	uniFwdlShmWrMsgIdx(&prUniFwdlPcieCtx->rShmCtx,
			   prUniFwdlPcieCtx->u2MsgTriggerIdx);
	uniFwdlShmWrMsgId(&prUniFwdlPcieCtx->rShmCtx,
			  UNI_FWDL_MSG_ID_START_DL);

	uniFwdlPcieTriggerWfFwdlDoorbell(prAdapter, prUniFwdlPcieCtx, FALSE);

	if (uniFwdlPcieWaitMsgDone(prAdapter, prUniFwdlPcieCtx,
				   MSG_TIMEOUT_MS) == FALSE)
		u4Status = WLAN_STATUS_FAILURE;

exit:
	DBGLOG(UNI_FWDL, INFO, "u4Status=0x%x\n", u4Status);

	return u4Status;
}

uint32_t uniFwdlPcieDlBlock(struct ADAPTER *prAdapter, uint8_t *prBuff,
			    uint32_t u4BuffSize)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	struct SLOT_TABLE *prSlotTable = &prUniFwdlPcieCtx->rSlotTable;
	struct SLOT_ENTRY *prEntry;
	uint32_t u4DlSize = 0, u4DlSlotSize, u4SlotNum = 0,
		u4Status = WLAN_STATUS_SUCCESS;
	uint8_t *prPos = prBuff;

	if (!(prUniFwdlPcieCtx->u4SyncInfo &
	      BIT(UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL))) {
		DBGLOG(UNI_FWDL, ERROR,
			"Unexpect behavior for Non-SLOT-DL mode\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (uniFwdlPcieWaitCommState(prAdapter, prUniFwdlPcieCtx,
				     UNI_FWDL_COMM_STATE_SLOT_DL,
				     SLOT_TIMEOUT_MS) == FALSE) {
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	DBGLOG(UNI_FWDL, INFO, "u4BuffSize=0x%x\n",
		u4BuffSize);

	do {
		if (uniFwdlPcieWaitSlotNonFull(prAdapter, prUniFwdlPcieCtx,
					       SLOT_TIMEOUT_MS) == FALSE) {
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		prEntry = prSlotTable->prSlotEntries[
			prSlotTable->u4SlotRdyIdx];
		u4DlSlotSize = kal_min_t(uint32_t,
					 prSlotTable->u4SlotSize,
					 (u4BuffSize - u4DlSize));

		kalMemCopyToIo(prEntry->rDmaMem.va, prPos, u4DlSlotSize);
		INC_SLOT_RDY_IDX(prSlotTable->u4SlotRdyIdx,
				 prSlotTable->u4SlotNum);
		uniFwdlShmWrSlotRdyIdx(&prUniFwdlPcieCtx->rShmCtx,
				       prSlotTable->u4SlotRdyIdx);

		INC_MSG_TRIGGER_IDX(prUniFwdlPcieCtx->u2MsgTriggerIdx);
		uniFwdlShmWrMsgIdx(&prUniFwdlPcieCtx->rShmCtx,
				   prUniFwdlPcieCtx->u2MsgTriggerIdx);
		uniFwdlShmWrMsgId(&prUniFwdlPcieCtx->rShmCtx,
				  UNI_FWDL_MSG_ID_SLOT_IDX_UPDATE);
		u4SlotNum++;

		DBGLOG(UNI_FWDL, LOUD,
			"msg_idx=%u slot_rdy_idx=%u pa=0x%llx size=0x%x\n",
			prUniFwdlPcieCtx->u2MsgTriggerIdx,
			prSlotTable->u4SlotRdyIdx,
			(uint64_t)prEntry->rDmaMem.pa,
			u4DlSlotSize);

		uniFwdlPcieTriggerWfFwdlDoorbell(prAdapter, prUniFwdlPcieCtx,
						 TRUE);

		prPos += u4DlSlotSize;
		u4DlSize += u4DlSlotSize;
	} while (u4DlSize < u4BuffSize);

	if (uniFwdlPcieWaitSlotEmpty(prAdapter, prUniFwdlPcieCtx,
				     SLOT_TIMEOUT_MS) == FALSE) {
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

exit:
	DBGLOG(UNI_FWDL, INFO, "u4SlotNum=%u u4Status=0x%x\n",
		u4SlotNum, u4Status);
	if (u4Status != WLAN_STATUS_SUCCESS)
		uniFwdlPcieDump(prAdapter);
	return u4Status;
}

uint32_t uniFwdlPcieSendMsg(struct ADAPTER *prAdapter,
			    enum UNI_FWDL_HIF_DL_RADIO_TYPE eRadio,
			    uint8_t ucMsgId,
			    uint8_t *prMsgData, uint32_t u4MsgDataSz)
{
	struct UNI_FWDL_CTX *prCtx = &prAdapter->rUniFwdlCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx =
		(struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	switch (ucMsgId) {
	case UNI_FWDL_MSG_ID_RESUME_DL:
		/* do nothing */
		break;
	case UNI_FWDL_MSG_ID_SET_SKU_CONFIG:
		/* do nothing */
		break;
	case UNI_FWDL_MSG_ID_HOST_EMI_ADDR_UPDATE:
	{
		struct UNI_FWDL_MSG_HOST_EMI_ADDR_UPDATE *prMsg =
			(struct UNI_FWDL_MSG_HOST_EMI_ADDR_UPDATE *)prMsgData;

		uniFwdlShmWrCtxInfoAddr_A(&prUniFwdlPcieCtx->rShmCtx,
					  prMsg->u8RwShmAddr);
		uniFwdlShmWrCtxInfoSize_A(&prUniFwdlPcieCtx->rShmCtx,
					  prMsg->u4RwShmSize);
		uniFwdlShmWrCtxInfoAddr_B(&prUniFwdlPcieCtx->rShmCtx,
					  prMsg->u8RoShmAddr);
		uniFwdlShmWrCtxInfoSize_B(&prUniFwdlPcieCtx->rShmCtx,
					  prMsg->u4RoShmSize);
	}
		break;
	default:
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto exit;
	}

	uniFwdlShmWrRadioType(&prUniFwdlPcieCtx->rShmCtx, eRadio);

	INC_MSG_TRIGGER_IDX(prUniFwdlPcieCtx->u2MsgTriggerIdx);
	uniFwdlShmWrMsgIdx(&prUniFwdlPcieCtx->rShmCtx,
			   prUniFwdlPcieCtx->u2MsgTriggerIdx);
	uniFwdlShmWrMsgId(&prUniFwdlPcieCtx->rShmCtx, ucMsgId);

	uniFwdlPcieTriggerWfFwdlDoorbell(prAdapter, prUniFwdlPcieCtx, FALSE);

	if (uniFwdlPcieWaitMsgDone(prAdapter, prUniFwdlPcieCtx,
				   MSG_TIMEOUT_MS) == FALSE) {
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	switch (ucMsgId) {
	case UNI_FWDL_MSG_ID_SET_SKU_CONFIG:
		if (uniFwdlPcieWaitNotif(prAdapter, prUniFwdlPcieCtx,
					 UNI_FWDL_NOTIF_ID_SKU_CONFIG_RESP,
					 MSG_TIMEOUT_MS) == FALSE) {
			u4Status = WLAN_STATUS_FAILURE;
			goto exit;
		}
		break;
	default:
		break;
	}

exit:
	DBGLOG(UNI_FWDL, INFO, "u4Status=0x%x\n", u4Status);
	return u4Status;
}

static void uniFwdlPcieHandleMsgUpdate(struct ADAPTER *prAdapter,
				       struct UNI_FWLD_PCIE_CTX *prCtx)
{
	uint16_t u2MsgDoneIdx;

	uniFwdlShmRdMsgDoneIdx(&prCtx->rShmCtx, &u2MsgDoneIdx);
	if (prCtx->u2MsgDoneIdx == u2MsgDoneIdx)
		return;

	DBGLOG(UNI_FWDL, INFO,
		"Msg done idx update %u -> %u\n",
		prCtx->u2MsgDoneIdx,
		u2MsgDoneIdx);
	prCtx->u2MsgDoneIdx = u2MsgDoneIdx;
}

static void uniFwdlPcieHandleNotifUpdate(struct ADAPTER *prAdapter,
					 struct UNI_FWLD_PCIE_CTX *prCtx)
{
	uint16_t u2NotifTriggerIdx, u2NotifId;
	u_int8_t fgNotifyFw = TRUE;

	uniFwdlShmRdNotifIdx(&prCtx->rShmCtx, &u2NotifTriggerIdx);
	if (prCtx->u2NotifDoneIdx == u2NotifTriggerIdx)
		return;

	uniFwdlShmRdNotifId(&prCtx->rShmCtx, &u2NotifId);
	prCtx->u2LastNotifId = u2NotifId;

	if (u2NotifId != UNI_FWDL_NOTIF_ID_SLOT_DONE_IDX)
		DBGLOG(UNI_FWDL, INFO,
			"notif id=%u idx=%u\n",
			u2NotifId, u2NotifTriggerIdx);
	else
		DBGLOG(UNI_FWDL, LOUD,
			"notif id=%u idx=%u\n",
			u2NotifId, u2NotifTriggerIdx);

	switch (u2NotifId) {
	case UNI_FWDL_NOTIF_ID_BROM_PATCH_DONE:
	{
		struct UNI_FWDL_NOTIF_NROM_PATCH_DONE rDone;
		uint16_t u2RadioType;

		uniFwdlShmRdBlockRadio(&prCtx->rShmCtx,
				       &u2RadioType);

		kalMemZero(&rDone, sizeof(rDone));
		rDone.eRadioType =
			(enum UNI_FWDL_HIF_DL_RADIO_TYPE)u2RadioType;

		if (prCtx->pfnNotifCb)
			prCtx->pfnNotifCb(prAdapter, u2NotifId,
					  (uint8_t *)&rDone,
					  sizeof(rDone));
	}
		break;

	case UNI_FWDL_NOTIF_ID_DL_DONE:
	{
		struct UNI_FWDL_NOTIF_DL_DONE rDlDone;
		uint16_t u2RadioType;

		uniFwdlShmRdBlockRadio(&prCtx->rShmCtx, &u2RadioType);

		kalMemZero(&rDlDone, sizeof(rDlDone));
		rDlDone.eRadioType =
			(enum UNI_FWDL_HIF_DL_RADIO_TYPE)u2RadioType;

		if (prCtx->pfnNotifCb)
			prCtx->pfnNotifCb(prAdapter, u2NotifId,
					  (uint8_t *)&rDlDone,
					  sizeof(rDlDone));
	}
		break;

	case UNI_FWDL_NOTIF_ID_FWDL_RESP:
	{
		struct UNI_FWDL_NOTIF_FWDL_RESP rResp;
		uint32_t u4CommState, u4Resp;
		uint16_t u2BlkRadio, u2BlkId, u2ErrorCode;

		uniFwdlShmRdBlockRadio(&prCtx->rShmCtx, &u2BlkRadio);
		uniFwdlShmRdDlResp(&prCtx->rShmCtx, &u4Resp);
		uniFwdlShmRdBlockId(&prCtx->rShmCtx, &u2BlkId);
		uniFwdlShmRdErrorCode(&prCtx->rShmCtx, &u2ErrorCode);
		uniFwdlShmRdCommState(&prCtx->rShmCtx, &u4CommState);

		kalMemZero(&rResp, sizeof(rResp));
		rResp.eDlRadio = (enum UNI_FWDL_HIF_DL_RADIO_TYPE)u2BlkRadio;
		rResp.eResp = (enum UNI_FWDL_RESP_STATUS)u4Resp;
		rResp.u2DlBlockId = u2BlkId;
		rResp.u2ErrorCode = u2ErrorCode;
		rResp.eCommState = (enum UNI_FWDL_COMM_STATE)u4CommState;

		if (prCtx->pfnNotifCb)
			prCtx->pfnNotifCb(prAdapter, u2NotifId,
					  (uint8_t *)&rResp,
					  sizeof(rResp));
	}
		break;

	case UNI_FWDL_NOTIF_ID_BOOT_STAGE:
	{
		struct UNI_FWDL_NOTIF_BOOT_STAGE rStage;
		uint32_t u4BootStage;

		uniFwdlShmRdBootStage(&prCtx->rShmCtx, &u4BootStage);

		kalMemZero(&rStage, sizeof(rStage));
		rStage.eBootStage = (enum BOOT_STAGE)u4BootStage;

		if (prCtx->pfnNotifCb)
			prCtx->pfnNotifCb(prAdapter, u2NotifId,
					  (uint8_t *)&rStage,
					  sizeof(rStage));
	}
		break;

	case UNI_FWDL_NOTIF_ID_EMI_ADDR_UPDATE:
		break;

	case UNI_FWDL_NOTIF_ID_SLOT_DONE_IDX:
	{
		struct SLOT_TABLE *prSlotTable = &prCtx->rSlotTable;
		uint32_t u4SlotDoneIdx;

		uniFwdlShmRdSlotDoneIdx(&prCtx->rShmCtx, &u4SlotDoneIdx);
		if (prSlotTable->u4SlotDoneIdx != u4SlotDoneIdx) {
			DBGLOG(UNI_FWDL, LOUD,
				"Slot done idx update %u -> %u\n",
				prSlotTable->u4SlotDoneIdx,
				u4SlotDoneIdx);
			prSlotTable->u4SlotDoneIdx = u4SlotDoneIdx;
		}
		fgNotifyFw = FALSE;
	}
		break;

	case UNI_FWDL_NOTIF_ID_SKU_CONFIG_RESP:
		break;

	default:
		DBGLOG(UNI_FWDL, TRACE,
			"Unknown notif id %u\n",
			u2NotifId);
		break;
	}

	if (prCtx->u2NotifDoneIdx != u2NotifTriggerIdx) {
		if (u2NotifId != UNI_FWDL_NOTIF_ID_SLOT_DONE_IDX)
			DBGLOG(UNI_FWDL, INFO,
				"Notif done idx update %u -> %u\n",
				prCtx->u2NotifDoneIdx,
				u2NotifTriggerIdx);
		else
			DBGLOG(UNI_FWDL, LOUD,
				"Notif done idx update %u -> %u\n",
				prCtx->u2NotifDoneIdx,
				u2NotifTriggerIdx);
		prCtx->u2NotifDoneIdx = u2NotifTriggerIdx;

		uniFwdlShmWrNotifDoneIdx(&prCtx->rShmCtx,
					 prCtx->u2NotifDoneIdx);
		if (fgNotifyFw)
			uniFwdlPcieTriggerWfFwdlDoorbell(prAdapter, prCtx,
							 FALSE);
	}
}

void uniFwdlPcieRcvInterrupt(struct ADAPTER *prAdapter)
{
	struct UNI_FWDL_CTX *prCtx;
	struct UNI_FWLD_PCIE_CTX *prUniFwdlPcieCtx;

	KAL_SPIN_LOCK_DECLARATION();

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);

	prCtx = &prAdapter->rUniFwdlCtx;
	prUniFwdlPcieCtx = (struct UNI_FWLD_PCIE_CTX *)prCtx->prHifCtx;
	if (!prUniFwdlPcieCtx) {
		DBGLOG(UNI_FWDL, WARN,
			"Skip irq due to NOT in downloading state\n");
		goto exit;
	}

	uniFwdlPcieHandleMsgUpdate(prAdapter, prUniFwdlPcieCtx);
	uniFwdlPcieHandleNotifUpdate(prAdapter, prUniFwdlPcieCtx);

exit:
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_UNI_FWDL);
}
