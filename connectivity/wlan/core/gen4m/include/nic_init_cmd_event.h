/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nic_init_cmd_event.h"
 *    \brief This file contains the declairation file of the WLAN
 *    initialization routines for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_INIT_CMD_EVENT_H
#define _NIC_INIT_CMD_EVENT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_typedef.h"
#include "wsys_cmd_handler_fw.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define INIT_CMD_STATUS_SUCCESS                 0
#define INIT_CMD_STATUS_REJECTED_INVALID_PARAMS 1
#define INIT_CMD_STATUS_REJECTED_CRC_ERROR      2
#define INIT_CMD_STATUS_REJECTED_DECRYPT_FAIL   3
#define INIT_CMD_STATUS_UNKNOWN                 4

#define INIT_PKT_FT_CMD				0x2
#define INIT_PKT_FT_PDA_FWDL		0x3

#define INIT_CMD_PQ_ID              (0x8000)
#define INIT_CMD_PACKET_TYPE_ID     (0xA0)

#define INIT_CMD_PDA_PQ_ID          (0xF800)
#define INIT_CMD_PDA_PACKET_TYPE_ID (0xA0)

#if (CFG_UMAC_GENERATION >= 0x20)
#define TXD_Q_IDX_MCU_RQ0   0
#define TXD_Q_IDX_MCU_RQ1   1
#define TXD_Q_IDX_MCU_RQ2   2
#define TXD_Q_IDX_MCU_RQ3   3

#define TXD_Q_IDX_PDA_FW_DL 0x1E

/* DW0 Bit31 */
#define TXD_P_IDX_LMAC  0
#define TXD_P_IDX_MCU   1

/* DW1 Bit 14:13 */
#define TXD_HF_NON_80211_FRAME      0x0
#define TXD_HF_CMD                  0x1
#define TXD_HF_80211_NORMAL         0x2
#define TXD_HF_80211_ENHANCEMENT    0x3

/* DW1 Bit 15 */
#define TXD_FT_OFFSET               15
#define TXD_FT_SHORT_FORMAT         0x0
#define TXD_FT_LONG_FORMAT          0x1

/* DW1 Bit 16 */
#define TXD_TXDLEN_OFFSET           16
#define TXD_TXDLEN_1PAGE             0x0
#define TXD_TXDLEN_2PAGE             0x1

/* DW1 Bit 25:24 */
#define TXD_PKT_FT_CUT_THROUGH      0x0
#define TXD_PKT_FT_STORE_FORWARD    0X1
#define TXD_PKT_FT_CMD              0X2
#define TXD_PKT_FT_PDA_FW           0X3
#endif

enum ENUM_INIT_CMD_ID {
	/* 0 means firmware download */
	INIT_CMD_ID_DOWNLOAD_CONFIG = 1,
	INIT_CMD_ID_WIFI_START,
	INIT_CMD_ID_ACCESS_REG,
	INIT_CMD_ID_QUERY_PENDING_ERROR,
	INIT_CMD_ID_PATCH_START,
	INIT_CMD_ID_PATCH_WRITE,
	INIT_CMD_ID_PATCH_FINISH,
	INIT_CMD_ID_PHY_ACTION,
	INIT_CMD_ID_LOG_TIME_SYNC,

	INIT_CMD_ID_PATCH_SEMAPHORE_CONTROL = 0x10,
	INIT_CMD_ID_BT_PATCH_SEMAPHORE_CONTROL = 0x11,
	INIT_CMD_ID_ZB_PATCH_SEMAPHORE_CONTROL = 0x12,
	INIT_CMD_ID_CO_PATCH_DOWNLOAD_CONFIG = 0x13,
	INIT_CMD_ID_HIF_LOOPBACK = 0x20,
	INIT_CMD_ID_LOG_BUF_CTRL = 0x21,
	INIT_CMD_ID_QUERY_INFO = 0x22,
	INIT_CMD_ID_EMI_FW_DOWNLOAD_CONFIG = 0x23,
	INIT_CMD_ID_EMI_FW_TRIGGER_AXI_DMA = 0x24,
	INIT_CMD_ID_DFD_INFO_QUERY = 0x25,

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
	INIT_CMD_ID_DYN_MEM_MAP_PATCH_FINISH = 0x40,
	INIT_CMD_ID_DYN_MEM_MAP_FW_FINISH = 0x41,
#endif
#if CFG_WLAN_LK_FWDL_SUPPORT
	INIT_CMD_ID_FW_IMAGE_START = 0x42,
#endif

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
	INIT_CMD_ID_DECOMPRESSED_WIFI_START = 0xFF,
#endif
	INIT_CMD_ID_NUM
};

enum ENUM_INIT_EVENT_ID {
	INIT_EVENT_ID_CMD_RESULT = 1,
	INIT_EVENT_ID_ACCESS_REG,
	INIT_EVENT_ID_PENDING_ERROR,
	INIT_EVENT_ID_PATCH_SEMA_CTRL,
	INIT_EVENT_ID_PHY_ACTION,
	INIT_EVENT_ID_BT_PATCH_SEMA_CTRL = 6,
	INIT_EVENT_ID_ZB_PATCH_SEMA_CTRL = 7,
	INIT_EVENT_ID_LOG_BUF_CTRL,
	INIT_EVENT_ID_QUERY_INFO_RESULT,
};

enum ENUM_INIT_PATCH_STATUS {
	PATCH_STATUS_NO_SEMA_NEED_PATCH = 0,	/* no SEMA, need patch */
	PATCH_STATUS_NO_NEED_TO_PATCH,	/* patch is DL & ready */
	PATCH_STATUS_GET_SEMA_NEED_PATCH,	/* get SEMA, need patch */
	PATCH_STATUS_RELEASE_SEMA	/* release SEMA */
};

enum _PATCH_FNSH_TYPE {
	PATCH_FNSH_TYPE_WF = 0,
	PATCH_FNSH_TYPE_BT = 1,
	PATCH_FNSH_TYPE_WF_MD = 2, /* flowwing is BT PATCH download flow */
	PATCH_FNSH_TYPE_ZB = 3
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct WIFI_CMD_INFO {
	uint16_t u2InfoBufLen;
	uint8_t *pucInfoBuffer;
	uint8_t ucCID;
	uint8_t ucExtCID;
	uint8_t ucPktTypeID;
	uint8_t ucSetQuery;
	uint8_t ucS2DIndex;
};

/* commands */
struct INIT_CMD_DOWNLOAD_CONFIG {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4DataMode;
};

struct INIT_CMD_CO_DOWNLOAD_CONFIG {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4DataMode;
	uint32_t u4SecInfo;
	uint32_t u4BinType;
};

#define START_OVERRIDE_START_ADDRESS    BIT(0)
#define START_DELAY_CALIBRATION         BIT(1)
#define START_WORKING_PDA_OPTION        BIT(2)
#define START_CRC_CHECK                 BIT(3)
#define CHANGE_DECOMPRESSION_TMP_ADDRESS    BIT(4)

#define WIFI_FW_DOWNLOAD_SUCCESS            0
#define WIFI_FW_DOWNLOAD_INVALID_PARAM      1
#define WIFI_FW_DOWNLOAD_INVALID_CRC        2
#define WIFI_FW_DOWNLOAD_DECRYPTION_FAIL    3
#define WIFI_FW_DOWNLOAD_UNKNOWN_CMD        4
#define WIFI_FW_DOWNLOAD_TIMEOUT            5
#define WIFI_FW_DOWNLOAD_SEC_BOOT_CHK_FAIL  6

#if CFG_SUPPORT_COMPRESSION_FW_OPTION
#define WIFI_FW_DECOMPRESSION_FAILED        0xFF
struct INIT_CMD_WIFI_DECOMPRESSION_START {
	uint32_t u4Override;
	uint32_t u4Address;
	uint32_t u4Region1length;
	uint32_t u4Region2length;
	uint32_t	u4Region1Address;
	uint32_t	u4Region2Address;
	uint32_t	u4BlockSize;
	uint32_t u4Region1CRC;
	uint32_t u4Region2CRC;
	uint32_t	u4DecompressTmpAddress;
};
#endif

struct INIT_CMD_WIFI_START {
	uint32_t u4Override;
	uint32_t u4Address;
};

#define PATCH_GET_SEMA_CONTROL		1
#define PATCH_RELEASE_SEMA_CONTROL	0
struct INIT_CMD_PATCH_SEMA_CONTROL {
	uint8_t ucGetSemaphore;
	uint8_t aucReserved[3];
};

struct INIT_CMD_PATCH_FINISH {
	uint8_t ucCheckCrc;
#if CFG_SUPPORT_WIFI_DL_BT_PATCH || CFG_SUPPORT_WIFI_DL_ZB_PATCH
	uint8_t ucType;
	uint8_t aucReserved[2];
#else
	uint8_t aucReserved[3];
#endif
};

#if CFG_SUPPORT_WIFI_DL_BT_PATCH
struct INIT_CMD_BT_PATCH_SEMA_CTRL {
	uint8_t ucGetSemaphore;
	uint8_t aucReserved[3];
	uint32_t u4Addr;
	uint8_t aucReserved1[4];
};

struct INIT_EVENT_BT_PATCH_SEMA_CTRL_T {
	uint8_t ucStatus; /* refer to enum ENUM_INIT_PATCH_STATUS */
	uint8_t ucReserved[3];
	uint32_t u4RemapAddr;
	uint8_t ucReserved1[4];
};
#endif /* CFG_SUPPORT_WIFI_DL_BT_PATCH */

#if CFG_SUPPORT_WIFI_DL_ZB_PATCH
struct INIT_CMD_ZB_PATCH_SEMA_CTRL {
	uint8_t ucGetSemaphore;
	uint8_t aucReserved[3];
	uint32_t u4Addr;
	uint32_t u4SecInfo; /* Only this field is different to BT's */
};
#endif /* CFG_SUPPORT_WIFI_DL_ZB_PATCH */

struct INIT_CMD_ACCESS_REG {
	uint8_t ucSetQuery;
	uint8_t aucReserved[3];
	uint32_t u4Address;
	uint32_t u4Data;
};

#if (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1)
#define HAL_PHY_ACTION_MAGIC_NUM			0x556789AA
#define HAL_PHY_ACTION_VERSION				0x01

#define HAL_PHY_ACTION_CAL_FORCE_CAL_REQ	0x01
#define HAL_PHY_ACTION_CAL_FORCE_CAL_RSP	0x81
#define HAL_PHY_ACTION_CAL_USE_BACKUP_REQ	0x02
#define HAL_PHY_ACTION_CAL_USE_BACKUP_RSP	0x82
#define HAL_PHY_ACTION_ERROR                0xff

enum ENUM_HAL_PHY_ACTION_STATUS {
	HAL_PHY_ACTION_STATUS_SUCCESS = 0x00,
	HAL_PHY_ACTION_STATUS_FAIL,
	HAL_PHY_ACTION_STATUS_RECAL,
	HAL_PHY_ACTION_STATUS_EPA_ELNA,
};

struct INIT_CMD_PHY_ACTION_CAL {
	uint8_t ucCmd;
	uint8_t ucCalSaveResult;
	uint8_t ucSkipCal;
	uint8_t aucReserved[1];
};

struct INIT_EVENT_PHY_ACTION_RSP {
	uint8_t ucEvent;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
	uint32_t u4EmiAddress;
	uint32_t u4EmiLength;
	uint32_t u4Temperatue;
};

enum ENUM_HAL_PHY_ACTION_TAG {
	HAL_PHY_ACTION_TAG_NVRAM,
	HAL_PHY_ACTION_TAG_CAL,
	HAL_PHY_ACTION_TAG_COM_FEM,
	HAL_PHY_ACTION_TAG_LAA,
	HAL_PHY_ACTION_TAG_NUM,
};

struct HAL_PHY_ACTION_TLV {
	uint16_t u2Tag;
	uint16_t u2BufLength;
	uint8_t  aucBuffer[];
};

struct HAL_PHY_ACTION_TLV_HEADER {
	uint32_t u4MagicNum;
	uint8_t  ucTagNums;
	uint8_t  ucVersion;
	uint16_t u2BufLength;
	uint8_t  aucBuffer[];
};
#endif /* (CFG_SUPPORT_PRE_ON_PHY_ACTION == 1) */

/* Events */
struct INIT_HIF_RX_HEADER {
	struct INIT_WIFI_EVENT rInitWifiEvent;
};

struct INIT_EVENT_ACCESS_REG {
	uint32_t u4Address;
	uint32_t u4Data;
};

enum FW_LOG_CMD_CTRL_TYPE {
	FW_LOG_CTRL_CMD_GET_BASE_ADDR,
	FW_LOG_CTRL_CMD_UPDATE_MCU_READ,
	FW_LOG_CTRL_CMD_UPDATE_WIFI_READ,
	FW_LOG_CTRL_CMD_UPDATE_BT_READ,
	FW_LOG_CTRL_CMD_UPDATE_GPS_READ,
	FW_LOG_CTRL_CMD_NUM
};

struct INIT_CMD_LOG_BUF_CTRL {
	uint32_t u4Address_MCU;
	uint32_t u4Address_WIFI;
	uint32_t u4Address_BT;
	uint32_t u4Address_GPS;
	/*
	 * BIT[0]:Log buffer control block base address
	 * BIT[1~4]: Update MCU/WiFi/BT/GPS read pointer
	 */
	uint8_t ucType;
	uint8_t aucReserved[3];
};

struct INIT_WIFI_EVENT_LOG_BUF_CTRL {
	/*
	 * BIT[0]:Log buffer control block base address
	 * BIT[1~4]: Update MCU/WiFi/BT/GPS read pointer
	 */
	uint8_t ucType;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
	uint32_t u4Address;
	uint32_t u4Reserved;
};

enum INIT_CMD_QUERY_TYPE {
	INIT_CMD_QUERY_TYPE_PMIC_INFO = 0,
	INIT_CMD_QUERY_TYPE_FWDL_EMI_SIZE,
	INIT_CMD_QUERY_TYPE_NUM
};

struct INIT_CMD_QUERY_INFO {
	uint32_t u4QueryBitmap;
	uint8_t aucReserved[4];
};

struct INIT_EVENT_QUERY_INFO {
	uint16_t u2TotalElementNum;
	uint16_t u2Length;
	uint8_t aucTlvBuffer[];
};

struct INIT_EVENT_TLV_GENERAL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucBuffer[];
};

struct INIT_EVENT_QUERY_INFO_PMIC {
	uint32_t u4PmicId;
	uint32_t u4Length;
	uint8_t aucPMICCoreDumpbuf[];
};

struct INIT_EVENT_QUERY_INFO_FWDL_EMI_SIZE {
	uint32_t u4Length;
};

struct INIT_CMD_EMI_FW_DOWNLOAD_CONFIG {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4DataMode;
};

struct INIT_CMD_EMI_FW_TRIGGER_AXI_DMA {
	uint32_t u4DownloadSize;
	uint8_t ucDoneBit;
	uint8_t aucReserved[3];
};

#define DEBUG_INFO_DFD_MAX_EVENT_LEN		(1024)
#define DEBUG_INFO_PMIC_DUMP_LENG		(240)
#define DEBUG_INFO_DFD_CB_INFRA_INFO_LENG	(768)
#define DEBUG_INFO_DFD_CB_INFRA_SRAM_LENG	(1024)
#define DEBUG_INFO_DFD_CB_INFRA_WF_SRAM_LENG	(1024*4)
#define DEBUG_INFO_DFD_CB_INFRA_BT_SRAM_LENG	(1024*4)
#define DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO_LENG	(128)
#define DEBUG_INFO_DFD_WF_DEBUG_INFO_LENG	(640+16)
#define DEBUG_INFO_DFD_BT_DEBUG_INFO_LENG	(768+16)

#define DEBUG_INFO_WIFI_DFD_DUMP_LENG	\
	(DEBUG_INFO_DFD_CB_INFRA_INFO_LENG+ \
	 DEBUG_INFO_DFD_CB_INFRA_SRAM_LENG+ \
	 DEBUG_INFO_DFD_CB_INFRA_WF_SRAM_LENG+ \
	 DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO_LENG+ \
	 DEBUG_INFO_DFD_WF_DEBUG_INFO_LENG)

enum INIT_CMD_DFD_INFO_TYPE {
	DEBUG_INFO_PMIC_DUMP,
	DEBUG_INFO_DFD_CB_INFRA_INFO,
	DEBUG_INFO_DFD_CB_INFRA_SRAM,
	DEBUG_INFO_DFD_CB_INFRA_WF_SRAM,
	DEBUG_INFO_DFD_CB_INFRA_BT_SRAM,
	DEBUG_INFO_DFD_CB_INFRA_DEBUG_INFO,
	DEBUG_INFO_DFD_WF_DEBUG_INFO,
	DEBUG_INFO_DFD_BT_DEBUG_INFO,
	NUM_OF_SYSTEM_DEBUG_INFO,
};

struct DEBUG_DUMP_REGIOM {
	uint32_t u4Type;
	uint32_t u4Length;
};

struct INIT_CMD_DFD_INFO_QUERY {
	uint32_t u4InfoIdx;
	uint32_t u4Offset;
	uint32_t u4Length;
	uint32_t aucReserved[2];
};

struct INIT_EVENT_DFD_INFO_QUERY {
	uint32_t u4Length;
	uint8_t  aucDFDInfoBuf[DEBUG_INFO_DFD_MAX_EVENT_LEN];
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                            P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _NIC_INIT_CMD_EVENT_H */
