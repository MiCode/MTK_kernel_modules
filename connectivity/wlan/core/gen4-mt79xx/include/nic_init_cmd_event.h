/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/nic_init_cmd_event.h#1
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
	INIT_CMD_ID_PATCH_SEMAPHORE_CONTROL = 0x10,
	INIT_CMD_ID_BT_PATCH_SEMAPHORE_CONTROL = 0x11,
	INIT_CMD_ID_ZB_PATCH_SEMAPHORE_CONTROL = 0x12,
	INIT_CMD_ID_HIF_LOOPBACK = 0x20,

#if (CFG_DOWNLOAD_DYN_MEMORY_MAP == 1)
	INIT_CMD_ID_DYN_MEM_MAP_PATCH_FINISH = 0x40,
	INIT_CMD_ID_DYN_MEM_MAP_FW_FINISH = 0x41,
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
	INIT_EVENT_ID_ZB_PATCH_SEMA_CTRL = 7
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
	uint8_t ucExtCmdOption;
};

/* commands */
struct INIT_CMD_DOWNLOAD_CONFIG {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4DataMode;
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

struct INIT_CMD_ACCESS_REG {
	uint8_t ucSetQuery;
	uint8_t aucReserved[3];
	uint32_t u4Address;
	uint32_t u4Data;
};

#if CFG_SUPPORT_WIFI_DL_BT_PATCH
struct INIT_CMD_BT_PATCH_SEMA_CTRL {
	uint8_t ucGetSemaphore;
	uint8_t aucReserved[3];
	uint32_t u4Addr;
	uint8_t aucReserved1[4];
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

/* Events */
struct INIT_HIF_RX_HEADER {
	struct INIT_WIFI_EVENT rInitWifiEvent;
};

struct INIT_EVENT_ACCESS_REG {
	uint32_t u4Address;
	uint32_t u4Data;
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
