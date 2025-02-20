/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */
#if (CFG_MTK_WIFI_SUPPORT_IPC == 1)
#ifndef _WLAN_IPC_H
#define _WLAN_IPC_H
/*******************************************************************************
 *                              M A C R O S
 *******************************************************************************
 */
#define UNI_FW_HDR_BUILD_DATE_LENGTH		20
#define UNI_FW_HDR_CHIP_ID_ECO_VER_LENGTH	8
#define UNI_FW_HDR_PATCH_VER_LENGTH		4

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
enum IPC_RW_TYPE {
	WLAN_IPC_WRITE,
	WLAN_IPC_READ,
	WLAN_IPC_SET,
	WLAN_IPC_CLR
};

enum ENUM_IPC_BOOT_STAGE {
	BOOT_STAGE_ROM,
	BOOT_STAGE_SECOND,
	BOOT_STAGE_OS,
	BOOT_STAGE_ABORT_HDL,
	BOOT_STAGE_COREDUMP,
	BOOT_STAGE_COREDUMP_FINISH,
	BOOT_STAGE_NUM
};

enum ENUM_IPC_POLLING_TYPE {
	PCI_CFG_SPACE,
	CONN_VON_SYSRAM,
	IPC_POLLING_TYPE_NUM
};

enum ENUM_IMG_RESP {
	IMG_RESP_DEFAULT,
	IMG_RESP_SUCCESS,
	IMG_RESP_FAILURE,
	IMG_RESP_DL_IMG_BEGIN,
	IMG_RESP_CHK_SEC,
	IMG_RESP_SEC_VAL_ERR,
	IMG_RESP_IMG_INVALID,
	IMG_RESP_NUM
};

struct UNI_FW_HDR_FORMAT_T {
	uint8_t aucBuildDate[UNI_FW_HDR_BUILD_DATE_LENGTH];
	uint8_t aucChipIDEcoVer[UNI_FW_HDR_CHIP_ID_ECO_VER_LENGTH];
	uint8_t aucPatchVer[UNI_FW_HDR_PATCH_VER_LENGTH];
};

struct WLAN_IPC_INFO {
	/* Constants */
	const uint32_t wfmcu_doorbell_pci_cfg_space_base_offset;
	const uint32_t cbmcu_doorbell_pci_cfg_space_base_offset;
	const uint32_t bitmap_pci_cfg_space_base_offset;
	const uint32_t conn_von_sysram_base_addr;
	const void * const conn_von_sysram_layout;
	const void * const bitmap_layout;
	const void * const wfmcu_doorbell_layout;
	const void * const cbmcu_doorbell_layout;
	/*Callback functions */
	uint32_t (*ipcCheckStatus)(struct GLUE_INFO *prGlueInfo,
				   enum ENUM_IPC_POLLING_TYPE eType,
				   const uint8_t fgIsBitCheck,
				   const uint8_t u1BitShift,
				   uint32_t *pu4Val,
				   const uint32_t u4ExpVal,
				   const uint32_t u4Offset,
				   const uint32_t u4Range,
				   const uint32_t u4PollingTimes,
				   const uint32_t u4Delay);
	uint32_t (*ipcAccessPciCfgSpace)(const enum IPC_RW_TYPE eType,
					 int32_t i4Offset,
					 uint32_t u4BitOffset,
					 uint32_t * const pu4Value);
	uint8_t (*ipcAccessConnVonSysRam)(struct GLUE_INFO *prGlueInfo,
					  const enum IPC_RW_TYPE eType,
					  const uint32_t u4Offset,
					  uint32_t * const pu4Addr,
					  const uint32_t u4Size);
	uint32_t (*ipcLoadFirmware)(struct ADAPTER *prAdapter,
				    uint8_t **apucFwNameTable);
	void (*ipcSetupWiFiMcuEmiAddr)(struct ADAPTER *prAdapter);
	void (*ipcSetupCbMcuEmiAddr)(struct ADAPTER *prAdapter);
};

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint8_t wlanIPCAccessConnVonSysRam(struct GLUE_INFO *prGlueInfo,
					enum IPC_RW_TYPE eType,
					uint32_t u4Offset,
					uint32_t *pu4Addr,
					uint32_t u4Size);
uint32_t wlanIPCAccessPciCfgSpace(enum IPC_RW_TYPE eType,
				int32_t i4Offset,
				uint32_t u4BitOffset,
				uint32_t *pu4Value);
uint32_t wlanIPCCheckStatus(struct GLUE_INFO *prGlueInfo,
			enum ENUM_IPC_POLLING_TYPE eType,
			uint8_t fgIsBitCheck,
			uint8_t u1BitShift,
			uint32_t      *pu4Val,
			uint32_t u4ExpVal,
			uint32_t u4Offset,
			uint32_t u4Range,
			uint32_t u4PollingTimes,
			uint32_t u4Delay);
uint8_t wlanIPCLoadFirmware(struct GLUE_INFO *prGlueInfo,
			uint8_t **apucFwNameTable);
uint32_t wlanGetUniFwHeaderInfo(const void *pvFwBuffer);
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */
#endif /* _WLAN_IPC_H_ */
