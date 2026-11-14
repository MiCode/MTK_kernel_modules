/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  uni_fw_dl.h
 */

#ifndef _UNI_FW_DL_H
#define _UNI_FW_DL_H

#include "uni_fw_dl_shm.h"
#include "uni_fw_dl_hif.h"


#define UNI_FWDL_MAX_SECTION_NUM			32

enum GLOBAL_SUBSYS {
	GLOBAL_SUBSYS_BT = 3,
	GLOBAL_SUBSYS_WF,
	GLOBAL_SUBSYS_GPS,
	GLOBAL_SUBSYS_ZB,
	GLOBAL_SUBSYS_NUM,
};

enum SUBSYS_SEC_TYPE {
	SUBSYS_SEC_TYPE_RESERVED,
	SUBSYS_SEC_TYPE_RELEASE_INFO,
	SUBSYS_SEC_TYPE_BINARY_INFO,
	SUBSYS_SEC_TYPE_IDX_LOG,
	SUBSYS_SEC_TYPE_TX_PWR,
	SUBSYS_SEC_TYPE_SIGNATURE,
	SUBSYS_SEC_TYPE_NUM,
};

enum SUBSYS_SEC_TYPE_SUBSYS {
	SUBSYS_SEC_TYPE_SUBSYS_RESERVED,
	SUBSYS_SEC_TYPE_SUBSYS_CBMCU,
	SUBSYS_SEC_TYPE_SUBSYS_FM,
	SUBSYS_SEC_TYPE_SUBSYS_BT,
	SUBSYS_SEC_TYPE_SUBSYS_WF_WM,
	SUBSYS_SEC_TYPE_SUBSYS_GPS,
	SUBSYS_SEC_TYPE_SUBSYS_ZB,
	SUBSYS_SEC_TYPE_SUBSYS_WF_DSP,
	SUBSYS_SEC_TYPE_SUBSYS_WF_WA,
	SUBSYS_SEC_TYPE_SUBSYS_NUM,
};

enum SUBSYS_SEC_TYPE_SEGMENT {
	SUBSYS_SEC_TYPE_SEGMENT_GENERAL,
	SUBSYS_SEC_TYPE_SEGMENT_REBB,
	SUBSYS_SEC_TYPE_SEGMENT_MOBILE,
	SUBSYS_SEC_TYPE_SEGMENT_CE,
	SUBSYS_SEC_TYPE_SEGMENT_NUM,
};

struct UNI_FWDL_SEC_MAP_INFO {
	enum SUBSYS_SEC_TYPE eType;
	enum SUBSYS_SEC_TYPE_SUBSYS eSubsys;
	enum SUBSYS_SEC_TYPE_SEGMENT eSegment;
	uint32_t u4SectionOffset;
	uint32_t u4SectionSize;
	uint8_t aucSectionSpecific[52];
	uint8_t *prContent;
};

struct UNI_FWDL_GLOBAL_DESCRIPTOR {
	enum GLOBAL_SUBSYS eGlobalSubsysType;
	uint32_t u4SectionNum;
};

struct UNI_FWDL_IMAGE_HEADER {
	uint8_t aucPackTime[32];
	uint8_t aucChipIdNEcoVer[16];
	uint32_t u4PatchVer;
};

struct UNI_FWDL_BINARY_INFO {
	uint8_t *prFwBuffer;
	uint32_t u4FwSize;
	uint8_t *prHeader;
	uint32_t u4HeaderSz;
	struct UNI_FWDL_IMAGE_HEADER rHeader;
	struct UNI_FWDL_GLOBAL_DESCRIPTOR rDescriptor;
	struct UNI_FWDL_SEC_MAP_INFO aucSecMaps[UNI_FWDL_MAX_SECTION_NUM];
};

__KAL_ATTRIB_PACKED_FRONT__
struct MULTI_HEADER_V2_SEC_RAW_FORMAT {
	uint32_t u4Type;
	uint32_t u4Offset;
	uint32_t u4Size;
	uint8_t aucSpecific[52];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct MULTI_HEADER_V2_RAW_FORMAT {
	uint8_t aucPackTime[20];
	uint8_t aucChipIdNEcoVer[8];
	uint32_t u4PatchVer;
	uint8_t aucReserved1[4];
	uint32_t u4GlobalSubsys;
	uint8_t aucReserved2[4];
	uint32_t u4SecNum;
	uint8_t aucReserved3[48];
	struct MULTI_HEADER_V2_SEC_RAW_FORMAT aucSections[];
} __KAL_ATTRIB_PACKED__;

enum UNI_FWDL_SYNC_INFO {
	UNI_FWDL_SYNC_INFO_SUPPORT_MSG_NOTIF,
	UNI_FWDL_SYNC_INFO_DL_XTAL_PKT,
	UNI_FWDL_SYNC_INFO_CO_DL,
	UNI_FWDL_SYNC_INFO_DFD_DUMP,
	UNI_FWDL_SYNC_INFO_BROM_NOTIF,
	UNI_FWDL_SYNC_INFO_FW_TIMEOUT,
	UNI_FWDL_SYNC_INFO_SHM_EMI,
	UNI_FWDL_SYNC_INFO_PCIE_INTX,
	UNI_FWDL_SYNC_INFO_SUPPORT_SLOT_DL,
	UNI_FWDL_SYNC_INFO_NUM,
};

enum UNI_FWLD_WAKEUP_SOURCE {
	WAKEUP_SOURCE_HALT,
	WAKEUP_SOURCE_QUERY_HW_INFO,
	WAKEUP_SOURCE_DL_DONE,
	WAKEUP_SOURCE_DL_FAIL,
	WAKEUP_SOURCE_REQ_DL,
	WAKEUP_SOURCE_BROM_PATCH_DONE,
	WAKEUP_SOURCE_NUM,
};

struct UNI_FWDL_INFO {
	const uint32_t u4SyncInfo;
	const uint32_t u4FwTimeoutDuration;
	const uint8_t ucCalibrationAction;
	struct UNI_FWDL_HIF_OPS rHifOps;
	struct UNI_FWDL_SHM_OPS rShmOps;
	void (*trigger_wf_fwdl_doorbell)(struct ADAPTER *ad);
	const uint32_t u4MinDoorbellTime;
};

struct UNI_FWDL_CTX {
	wait_queue_head_t rWaitQ;
	unsigned long ulFlags;

	uint16_t u2ErrorCode;

	enum UNI_FWDL_HIF_DL_RADIO_TYPE eCurrentDlRadio;
	uint16_t u2CurrentDlBlockId;

	uint32_t u4XtalPktSize;

	void *prHifCtx;
};

uint32_t uniFwdlGetReleaseManifest(struct GLUE_INFO *prGlueInfo,
				   uint8_t *pucManifestBuffer,
				   uint32_t *pu4ManifestSize,
				   uint32_t u4BufferMaxSize);

uint32_t uniFwdlDownloadFW(struct ADAPTER *prAdapter);
#endif /* _UNI_FW_DL_H */
