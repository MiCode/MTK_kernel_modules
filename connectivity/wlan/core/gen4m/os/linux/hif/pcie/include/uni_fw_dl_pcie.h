/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl_pcie.h
 */

#ifndef _UNI_FW_DL_PCIE_H
#define _UNI_FW_DL_PCIE_H


struct SLOT_ENTRY {
	struct LINK_ENTRY rLinkEntry;

	struct HIF_MEM rDmaMem;
};

struct SLOT_TABLE {
	struct HIF_MEM rDmaMem;

	uint32_t u4SlotRdyIdx;
	uint32_t u4SlotDoneIdx;
	uint32_t u4SlotNum;
	uint32_t u4SlotSize;

	struct SLOT_ENTRY *prSlotEntries[];
};

enum UNI_FWDL_PCIE_DL_STATE {
	UNI_FWDL_PCIE_DL_STATE_IDLE,
	UNI_FWDL_PCIE_DL_STATE_MSG_NOTIF,
	UNI_FWDL_PCIE_DL_STATE_SLOT_DL,
	UNI_FWDL_PCIE_DL_STATE_NUM,
};

struct UNI_FWLD_PCIE_CTX {
	uint32_t u4SyncInfo;

	uint32_t u4AllocSize;

	uint16_t u2MsgTriggerIdx;
	uint16_t u2MsgDoneIdx;

	uint16_t u2NotifTriggerIdx;
	uint16_t u2NotifDoneIdx;

	uint16_t u2LastNotifId;
	uint32_t u4MinDoorbellTime;
	uint32_t u4LastDoorbellTime;

	UNI_FWDL_NOTIF_CB pfnNotifCb;

	struct HIF_MEM rXtalMem;

	struct UNI_FWLD_SHM_CTX rShmCtx;

	struct LINK rSlotList;
	/* rSlotTable must be the last element due to undetermined array number
	 * i.e. prSlotEntries[]
	 */
	struct SLOT_TABLE rSlotTable;
};

uint32_t uniFwdlPcieInit(struct ADAPTER *prAdapter,
			 UNI_FWDL_NOTIF_CB pfnNotifCb,
			 uint32_t u4FwSize);

void uniFwdlPcieDeInit(struct ADAPTER *prAdapter);

void uniFwdlPcieDump(struct ADAPTER *prAdapter);

uint32_t uniFwdlPcieQueryHwInfo(struct ADAPTER *prAdapter, uint8_t *prBuff,
				uint32_t u4BuffSize);

uint32_t uniFwdlPcieStartDl(struct ADAPTER *prAdapter,
			    struct START_DL_PARAMS *prParams);

uint32_t uniFwdlPcieDlBlock(struct ADAPTER *prAdapter, uint8_t *prBuff,
			    uint32_t u4BuffSize);

uint32_t uniFwdlPcieSendMsg(struct ADAPTER *prAdapter,
			    enum UNI_FWDL_HIF_DL_RADIO_TYPE eRadio,
			    uint8_t ucMsgId,
			    uint8_t *prMsgData, uint32_t u4MsgDataSz);

void uniFwdlPcieRcvInterrupt(struct ADAPTER *prAdapter);

#endif /* _UNI_FW_DL_PCIE_H */
