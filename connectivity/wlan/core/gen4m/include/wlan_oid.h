/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "wlan_oid.h"
 *    \brief This file contains the declairation file of the WLAN OID processing
 *         routines of Windows driver for MediaTek Inc.
 *         802.11 Wireless LAN Adapters.
 */

#ifndef _WLAN_OID_H
#define _WLAN_OID_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "bitmap.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define PARAM_MAX_LEN_SSID                      32

#define PARAM_MAC_ADDR_LEN                      6

#define ETHERNET_HEADER_SZ                      14
#define ETHERNET_MIN_PKT_SZ                     60
#define ETHERNET_MAX_PKT_SZ                     1514

#define PARAM_MAX_LEN_RATES                     8
#define PARAM_MAX_LEN_RATES_EX                  16

#define PARAM_AUTH_REQUEST_REAUTH               0x01
#define PARAM_AUTH_REQUEST_KEYUPDATE            0x02
#define PARAM_AUTH_REQUEST_PAIRWISE_ERROR       0x06
#define PARAM_AUTH_REQUEST_GROUP_ERROR          0x0E


#define PARAM_EEPROM_READ_METHOD_GETSIZE        0
#define PARAM_EEPROM_READ_METHOD_READ           1
#define PARAM_EEPROM_READ_NVRAM					2
#define PARAM_EEPROM_WRITE_NVRAM				3


#define PARAM_WHQL_RSSI_MAX_DBM                 (0)
#define PARAM_WHQL_RSSI_INITIAL_DBM             (-50)
#define PARAM_WHQL_RSSI_MIN_DBM                 (-110)

#define PARAM_DEVICE_WAKE_UP_ENABLE                     0x00000001
#define PARAM_DEVICE_WAKE_ON_PATTERN_MATCH_ENABLE       0x00000002
#define PARAM_DEVICE_WAKE_ON_MAGIC_PACKET_ENABLE        0x00000004

#define PARAM_WAKE_UP_MAGIC_PACKET              0x00000001
#define PARAM_WAKE_UP_PATTERN_MATCH             0x00000002
#define PARAM_WAKE_UP_LINK_CHANGE               0x00000004

/* Packet filter bit definitioin (UINT_32 bit-wise definition) */
#define PARAM_PACKET_FILTER_DIRECTED            0x00000001
#define PARAM_PACKET_FILTER_MULTICAST           0x00000002
#define PARAM_PACKET_FILTER_ALL_MULTICAST       0x00000004
#define PARAM_PACKET_FILTER_BROADCAST           0x00000008
#define PARAM_PACKET_FILTER_PROMISCUOUS         0x00000020
#define PARAM_PACKET_FILTER_ALL_LOCAL           0x00000080
#define PARAM_PACKET_FILTER_P2P_MASK		0xF0000000
#define PARAM_PACKET_FILTER_PROBE_REQ		0x80000000
#define PARAM_PACKET_FILTER_ACTION_FRAME	0x40000000
#define PARAM_PACKET_FILTER_AUTH		0x20000000
#define PARAM_PACKET_FILTER_ASSOC_REQ		0x10000000
#define PARAM_PACKET_FILTER_SUPPORTED   (PARAM_PACKET_FILTER_DIRECTED | \
					 PARAM_PACKET_FILTER_MULTICAST | \
					 PARAM_PACKET_FILTER_BROADCAST | \
					 PARAM_PACKET_FILTER_ALL_MULTICAST)

#define PARAM_MEM_DUMP_MAX_SIZE         1536

#define BT_PROFILE_PARAM_LEN		8

/* Based on EEPROM layout 20160120 */
#define EFUSE_ADDR_MAX			0x3BF
#if CFG_SUPPORT_BUFFER_MODE

/* For MT7668 */
#define EFUSE_CONTENT_BUFFER_START	0x03A
#define EFUSE_CONTENT_BUFFER_END	0x1D9
#define EFUSE_CONTENT_BUFFER_SIZE	(EFUSE_CONTENT_BUFFER_END - \
					 EFUSE_CONTENT_BUFFER_START + 1)

/* For MT6632 */
#define EFUSE_CONTENT_SIZE		16

#define EFUSE_BLOCK_SIZE		16
#define EEPROM_SIZE			1184

#if defined MT7915 || defined MT7961 || defined MT7902
#define MAX_EEPROM_BUFFER_SIZE	0xe00
#define BUFFER_BIN_PAGE_SIZE	0x400
#else
/* Based on EEPROM layout Bellwether */
#define MAX_EEPROM_BUFFER_SIZE		0x1800  //From 1450 to 6K
#define BUFFER_BIN_PAGE_SIZE		0x400
#endif

#define BUFFER_BIN_TOTAL_PAGE_MASK	BITS(5, 7)
#define BUFFER_BIN_TOTAL_PAGE_SHIFT	5
#define BUFFER_BIN_PAGE_INDEX_MASK	BITS(2, 4)
#define BUFFER_BIN_PAGE_INDEX_SHIFT	2

#endif /* CFG_SUPPORT_BUFFER_MODE */

#if CFG_SUPPORT_TX_BF
#define TXBF_CMD_NEED_TO_RESPONSE(u4TxBfCmdId)	\
					(u4TxBfCmdId == BF_PFMU_TAG_READ || \
					 u4TxBfCmdId == BF_PROFILE_READ)
#endif /* CFG_SUPPORT_TX_BF */
#define MU_CMD_NEED_TO_RESPONSE(u4MuCmdId)	\
					(u4MuCmdId == MU_GET_CALC_INIT_MCS || \
					 u4MuCmdId == MU_HQA_GET_QD || \
					 u4MuCmdId == MU_HQA_GET_CALC_LQ)
#if CFG_SUPPORT_MU_MIMO
/* @NITESH: MACROS For Init MCS calculation (MU Metric Table) */
#define NUM_MUT_FEC             2
#define NUM_MUT_MCS             10
#define NUM_MUT_NR_NUM          3
#define NUM_MUT_INDEX           8

#define NUM_OF_USER             2
#define NUM_OF_MODUL            5
#endif /* CFG_SUPPORT_MU_MIMO */

#define SER_ACTION_QUERY                    0
#define SER_ACTION_SET                      1
#define SER_ACTION_SET_ENABLE_MASK          2
#define SER_ACTION_RECOVER                  3
#define SER_ACTION_L0P5_CTRL                4

/* SER_ACTION_SET sub action */
#define SER_SET_DISABLE         0
#define SER_SET_ENABLE          1

/* SER_ACTION_SET_ENABLE_MASK mask define */
#define SER_ENABLE_TRACKING         BIT(0)
#define SER_ENABLE_L1_RECOVER       BIT(1)
#define SER_ENABLE_L2_RECOVER       BIT(2)
#define SER_ENABLE_L3_RX_ABORT      BIT(3)
#define SER_ENABLE_L3_TX_ABORT      BIT(4)
#define SER_ENABLE_L3_TX_DISABLE    BIT(5)
#define SER_ENABLE_L3_BF_RECOVER    BIT(6)

/* SER_ACTION_RECOVER recover method */
#define SER_SET_L0_RECOVER         0
#define SER_SET_L1_RECOVER         1
#define SER_SET_L2_RECOVER         2
#define SER_SET_L3_RX_ABORT        3
#define SER_SET_L3_TX_ABORT        4
#define SER_SET_L3_TX_DISABLE      5
#define SER_SET_L3_BF_RECOVER      6

/* SER_ACTION_L0P5_CTRL sub action */
#define SER_ACTION_L0P5_CTRL_PAUSE_WDT          (0)
#define SER_ACTION_L0P5_CTRL_RESUME_WDT         (1)
#define SER_ACTION_L0P5_CTRL_WM_HANG            (2)
#define SER_ACTION_L0P5_CTRL_BUS_HANG           (3)
/* SER user command */
#define SER_USER_CMD_DISABLE         0
#define SER_USER_CMD_ENABLE          1

#define SER_USER_CMD_ENABLE_MASK_TRACKING_ONLY      (200)
#define SER_USER_CMD_ENABLE_MASK_L1_RECOVER_ONLY    (201)
#define SER_USER_CMD_ENABLE_MASK_L2_RECOVER_ONLY    (202)
#define SER_USER_CMD_ENABLE_MASK_L3_RX_ABORT_ONLY   (203)
#define SER_USER_CMD_ENABLE_MASK_L3_TX_ABORT_ONLY   (204)
#define SER_USER_CMD_ENABLE_MASK_L3_TX_DISABLE_ONLY (205)
#define SER_USER_CMD_ENABLE_MASK_L3_BFRECOVER_ONLY  (206)
#define SER_USER_CMD_ENABLE_MASK_RECOVER_ALL        (207)


/* Use a magic number to prevent human mistake */
#define SER_USER_CMD_L0_RECOVER          956
#define SER_USER_CMD_L1_RECOVER          995

#define SER_USER_CMD_L2_BN0_RECOVER      (300)
#define SER_USER_CMD_L2_BN1_RECOVER      (301)
#define SER_USER_CMD_L3_RX0_ABORT        (302)
#define SER_USER_CMD_L3_RX1_ABORT        (303)
#define SER_USER_CMD_L3_TX0_ABORT        (304)
#define SER_USER_CMD_L3_TX1_ABORT        (305)
#define SER_USER_CMD_L3_TX0_DISABLE      (306)
#define SER_USER_CMD_L3_TX1_DISABLE      (307)
#define SER_USER_CMD_L3_BF_RECOVER       (308)

#define SER_USER_CMD_L0P5_PAUSE_WDT      (400)
#define SER_USER_CMD_L0P5_RESUME_WDT     (401)
#define SER_USER_CMD_L0P5_WM_HANG        (444)
#define SER_USER_CMD_L0P5_RECOVER        (488)
#define SER_USER_CMD_L0P5_BUS_HANG       (499)

#define TXPOWER_MAN_SET_INPUT_ARG_NUM 4

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
#define TXPOWER_INFO_INPUT_ARG_NUM 2
#define TXPOWER_FORMAT_LEGACY 0
#define TXPOWER_FORMAT_HE 1

/* 1M, 2M, 5.5M, 11M */
#define MODULATION_SYSTEM_CCK_NUM       4

/* 6M, 9M, 12M, 18M, 24M, 36M, 48M, 54M */
#define MODULATION_SYSTEM_OFDM_NUM      8

#define MODULATION_SYSTEM_HT20_NUM      8       /* MCS0~7 */
#define MODULATION_SYSTEM_HT40_NUM      9       /* MCS0~7, MCS32 */
#define MODULATION_SYSTEM_VHT20_NUM     12      /* MCS0~11 */
#define MODULATION_SYSTEM_VHT40_NUM     MODULATION_SYSTEM_VHT20_NUM
#define MODULATION_SYSTEM_VHT80_NUM     MODULATION_SYSTEM_VHT20_NUM
#define MODULATION_SYSTEM_VHT160_NUM    MODULATION_SYSTEM_VHT20_NUM

#define MODULATION_SYSTEM_HE_26_MCS_NUM      12
#define MODULATION_SYSTEM_HE_52_MCS_NUM      12
#define MODULATION_SYSTEM_HE_106_MCS_NUM     12
#define MODULATION_SYSTEM_HE_242_MCS_NUM     12
#define MODULATION_SYSTEM_HE_484_MCS_NUM     12
#define MODULATION_SYSTEM_HE_996_MCS_NUM     12
#define MODULATION_SYSTEM_HE_996X2_MCS_NUM   12

#define MODULATION_SYSTEM_EHT_MCS_NUM		16
#define MODULATION_SYSTEM_EHT_26_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_52_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_106_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_242_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_484_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996X2_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996X4_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_26_52_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_26_106_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_484_242_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996_484_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996_484_242_MCS_NUM \
	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996X2_484_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996X3_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM
#define MODULATION_SYSTEM_EHT_996X3_484_MCS_NUM	MODULATION_SYSTEM_EHT_MCS_NUM


#define TXPOWER_RATE_CCK_OFFSET         (0)
#define TXPOWER_RATE_OFDM_OFFSET        (TXPOWER_RATE_CCK_OFFSET  + \
					 MODULATION_SYSTEM_CCK_NUM)
#define TXPOWER_RATE_HT20_OFFSET        (TXPOWER_RATE_OFDM_OFFSET + \
					 MODULATION_SYSTEM_OFDM_NUM)
#define TXPOWER_RATE_HT40_OFFSET        (TXPOWER_RATE_HT20_OFFSET + \
					 MODULATION_SYSTEM_HT20_NUM)
#define TXPOWER_RATE_VHT20_OFFSET       (TXPOWER_RATE_HT40_OFFSET + \
					 MODULATION_SYSTEM_HT40_NUM)
#define TXPOWER_RATE_VHT40_OFFSET       (TXPOWER_RATE_VHT20_OFFSET + \
					 MODULATION_SYSTEM_VHT20_NUM)
#define TXPOWER_RATE_VHT80_OFFSET       (TXPOWER_RATE_VHT40_OFFSET + \
					 MODULATION_SYSTEM_VHT40_NUM)
#define TXPOWER_RATE_VHT160_OFFSET      (TXPOWER_RATE_VHT80_OFFSET + \
					 MODULATION_SYSTEM_VHT80_NUM)

#define TXPOWER_RATE_HE26_OFFSET    (TXPOWER_RATE_VHT160_OFFSET + \
					MODULATION_SYSTEM_VHT160_NUM)
#define TXPOWER_RATE_HE52_OFFSET    (TXPOWER_RATE_HE26_OFFSET + \
					MODULATION_SYSTEM_HE_26_MCS_NUM)
#define TXPOWER_RATE_HE106_OFFSET   (TXPOWER_RATE_HE52_OFFSET + \
					MODULATION_SYSTEM_HE_52_MCS_NUM)
#define TXPOWER_RATE_HE242_OFFSET   (TXPOWER_RATE_HE106_OFFSET + \
					MODULATION_SYSTEM_HE_106_MCS_NUM)
#define TXPOWER_RATE_HE484_OFFSET   (TXPOWER_RATE_HE242_OFFSET + \
					MODULATION_SYSTEM_HE_242_MCS_NUM)
#define TXPOWER_RATE_HE996_OFFSET   (TXPOWER_RATE_HE484_OFFSET + \
					MODULATION_SYSTEM_HE_484_MCS_NUM)
#define TXPOWER_RATE_HE996X2_OFFSET (TXPOWER_RATE_HE996_OFFSET + \
					MODULATION_SYSTEM_HE_996_MCS_NUM)

#define TXPOWER_RATE_EHT26_OFFSET	(TXPOWER_RATE_HE996X2_OFFSET + \
					 MODULATION_SYSTEM_HE_996X2_MCS_NUM)
#define TXPOWER_RATE_EHT52_OFFSET	(TXPOWER_RATE_EHT26_OFFSET + \
					 MODULATION_SYSTEM_EHT_26_MCS_NUM)
#define TXPOWER_RATE_EHT106_OFFSET	(TXPOWER_RATE_EHT52_OFFSET + \
					 MODULATION_SYSTEM_EHT_52_MCS_NUM)
#define TXPOWER_RATE_EHT242_OFFSET	(TXPOWER_RATE_EHT106_OFFSET + \
					 MODULATION_SYSTEM_EHT_106_MCS_NUM)
#define TXPOWER_RATE_EHT484_OFFSET	(TXPOWER_RATE_EHT242_OFFSET + \
					 MODULATION_SYSTEM_EHT_242_MCS_NUM)
#define TXPOWER_RATE_EHT996_OFFSET	(TXPOWER_RATE_EHT484_OFFSET + \
					 MODULATION_SYSTEM_EHT_484_MCS_NUM)
#define TXPOWER_RATE_EHT996X2_OFFSET	(TXPOWER_RATE_EHT996_OFFSET + \
					 MODULATION_SYSTEM_EHT_996_MCS_NUM)
#define TXPOWER_RATE_EHT996X4_OFFSET	(TXPOWER_RATE_EHT996X2_OFFSET + \
					 MODULATION_SYSTEM_EHT_996X2_MCS_NUM)
#define TXPOWER_RATE_EHT26_52_OFFSET	(TXPOWER_RATE_EHT996X4_OFFSET + \
					 MODULATION_SYSTEM_EHT_996X4_MCS_NUM)
#define TXPOWER_RATE_EHT26_106_OFFSET	(TXPOWER_RATE_EHT26_52_OFFSET + \
					 MODULATION_SYSTEM_EHT_26_52_MCS_NUM)
#define TXPOWER_RATE_EHT484_242_OFFSET	(TXPOWER_RATE_EHT26_106_OFFSET + \
					 MODULATION_SYSTEM_EHT_26_106_MCS_NUM)
#define TXPOWER_RATE_EHT996_484_OFFSET	(TXPOWER_RATE_EHT484_242_OFFSET + \
					 MODULATION_SYSTEM_EHT_484_242_MCS_NUM)
#define TXPOWER_RATE_EHT996_484_242_OFFSET \
					(TXPOWER_RATE_EHT996_484_OFFSET + \
					 MODULATION_SYSTEM_EHT_996_484_MCS_NUM)
#define TXPOWER_RATE_EHT996X2_484_OFFSET \
					(TXPOWER_RATE_EHT996_484_242_OFFSET + \
				MODULATION_SYSTEM_EHT_996_484_242_MCS_NUM)
#define TXPOWER_RATE_EHT996X3_OFFSET	(TXPOWER_RATE_EHT996X2_484_OFFSET + \
					MODULATION_SYSTEM_EHT_996X2_484_MCS_NUM)
#define TXPOWER_RATE_EHT996X3_484_OFFSET \
					(TXPOWER_RATE_EHT996X3_OFFSET + \
					 MODULATION_SYSTEM_EHT_996X3_MCS_NUM)
#define TXPOWER_RATE_NUM		(TXPOWER_RATE_EHT996X3_484_OFFSET + \
					MODULATION_SYSTEM_EHT_996X3_484_MCS_NUM)
#endif

#if CFG_SUPPORT_LOWLATENCY_MODE
#define GED_EVENT_GAS               (1 << 4)
#define GED_EVENT_NETWORK           (1 << 11)
#define GED_EVENT_DOPT_WIFI_SCAN    (1 << 12)
#define GED_EVENT_DISABLE_ROAMING   (1 << 14)
#define GED_EVENT_CAM_MODE          (1 << 15)
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#define MAX_MLO_MGMT_SUPPORT_MLD_NUM		1
#define MAX_MLO_MGMT_SUPPORT_AC_NUM		4

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
#define MAC_ICS_MODE		2
#define PHY_ICS_MODE		3
#endif /* #if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1)) */

#define LP_CMD_QUERY	0
#define LP_CMD_SET		1

#define LP_TAG_GET_SLP_CNT_INFO		0
#define LP_TAG_SET_KEEP_PWR_CTRL	1

#define MAX_MIB_TAG_CNT		74
/* must >= UNI_CMD_MIB_CNT_MAX_NUM */
#define MAX_UNI_CMD_MIB_NUM	212

#define ROAMING_NOT_ALLOWED		0
#define ROAMING_ALLOWED_WITHIN_ESS	1
#define ROAMING_MODE_AGGRESSIVE		2

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/* Parameters of User Configuration which match to NDIS5.1                    */
/*----------------------------------------------------------------------------*/

enum ENUM_PARAM_PHY_TYPE {
	PHY_TYPE_802_11ABG = 0,	/*!< Can associated with 802.11abg AP,
				 * Scan dual band.
				 */
	PHY_TYPE_802_11BG,	/*!< Can associated with 802_11bg AP,
				 * Scan single band and not report 802_11a
				 * BSSs.
				 */
	PHY_TYPE_802_11G,	/*!< Can associated with 802_11g only AP,
				 * Scan single band and not report 802_11ab
				 * BSSs.
				 */
	PHY_TYPE_802_11A,	/*!< Can associated with 802_11a only AP,
				 * Scan single band and not report 802_11bg
				 * BSSs.
				 */
	PHY_TYPE_802_11B,	/*!< Can associated with 802_11b only AP
				 * Scan single band and not report 802_11ag
				 * BSSs.
				 */
	PHY_TYPE_NUM		/* 5 */
};

enum ENUM_PARAM_OP_MODE {
	NET_TYPE_IBSS = 0,	/*!< Try to merge/establish an AdHoc,
				 * do periodic SCAN for merging.
				 */
	NET_TYPE_INFRA,		/*!< Try to join an Infrastructure,
				 * do periodic SCAN for joining.
				 */
	NET_TYPE_AUTO_SWITCH,	/*!< Try to join an Infrastructure,
				 * if fail then try to merge or
				 */
	/*  establish an AdHoc, do periodic SCAN for joining or merging. */
	NET_TYPE_DEDICATED_IBSS,/*!< Try to merge an AdHoc first, */
	/* if fail then establish AdHoc permanently, no more SCAN. */
	NET_TYPE_NUM		/* 4 */
};

struct PARAM_CONNECT {
	uint32_t u4SsidLen;	/*!< SSID length in bytes.
				 * Zero length is broadcast(any) SSID
				 */
	uint8_t *pucSsid;
	uint8_t *pucBssid;
	uint8_t *pucBssidHint;
	uint32_t u4CenterFreq;
	uint8_t ucBssIdx;
	uint8_t *pucIEs;
	uint32_t u4IesLen;
	uint8_t fgTestMode;
	uint16_t u2LinkIdBitmap;
};

struct PARAM_EXTERNAL_AUTH {
	uint8_t bssid[PARAM_MAC_ADDR_LEN];
	uint16_t status;
	uint8_t ucBssIdx;
};

struct PARAM_OP_MODE {
	enum ENUM_PARAM_OP_MODE eOpMode;
	uint8_t ucBssIdx;
};

/* This is enum defined for user to select an AdHoc Mode */
enum ENUM_PARAM_AD_HOC_MODE {
	AD_HOC_MODE_11B = 0,	/*!< Create 11b IBSS if we support
				 * 802.11abg/802.11bg.
				 */
	AD_HOC_MODE_MIXED_11BG,	/*!< Create 11bg mixed IBSS if we support
				 * 802.11abg/802.11bg/802.11g.
				 */
	AD_HOC_MODE_11G,	/*!< Create 11g only IBSS if we support
				 * 802.11abg/802.11bg/802.11g.
				 */
	AD_HOC_MODE_11A,	/*!< Create 11a only IBSS if we support
				 * 802.11abg.
				 */
	AD_HOC_MODE_NUM		/* 4 */
};

enum ENUM_PARAM_NETWORK_TYPE {
	PARAM_NETWORK_TYPE_FH,
	PARAM_NETWORK_TYPE_DS,
	PARAM_NETWORK_TYPE_OFDM5,
	PARAM_NETWORK_TYPE_OFDM24,
	PARAM_NETWORK_TYPE_AUTOMODE,
	PARAM_NETWORK_TYPE_NUM	/*!< Upper bound, not real case */
};

struct PARAM_NETWORK_TYPE_LIST {
	uint32_t NumberOfItems;	/*!< At least 1 */
	enum ENUM_PARAM_NETWORK_TYPE eNetworkType[1];
};

enum ENUM_PARAM_PRIVACY_FILTER {
	PRIVACY_FILTER_ACCEPT_ALL,
	PRIVACY_FILTER_8021xWEP,
	PRIVACY_FILTER_NUM
};

enum ENUM_RELOAD_DEFAULTS {
	ENUM_RELOAD_WEP_KEYS
};

struct PARAM_PM_PACKET_PATTERN {
	uint32_t Priority;	/* Importance of the given pattern. */
	uint32_t Reserved;	/* Context information for transports. */
	uint32_t MaskSize;	/* Size in bytes of the pattern mask. */
	uint32_t PatternOffset;	/* Offset from beginning of this */
	/* structure to the pattern bytes. */
	uint32_t PatternSize;	/* Size in bytes of the pattern. */
	uint32_t PatternFlags;	/* Flags (TBD). */
};


/* Combine ucTpTestMode and ucSigmaTestMode in one flag */
/* ucTpTestMode == 0, for normal driver */
/* ucTpTestMode == 1, for pure throughput test mode (ex: RvR) */
/* ucTpTestMode == 2, for sigma TGn/TGac/PMF */
/* ucTpTestMode == 3, for sigma WMM PS */
enum ENUM_TP_TEST_MODE {
	ENUM_TP_TEST_MODE_NORMAL = 0,
	ENUM_TP_TEST_MODE_THROUGHPUT,
	ENUM_TP_TEST_MODE_SIGMA_AC_N_PMF,
	ENUM_TP_TEST_MODE_SIGMA_WMM_PS,
	ENUM_TP_TEST_MODE_NUM
};

enum ENUM_EEPROM_CONTENT_FORMAT {
	CONTENT_FORMAT_BIN_CONTENT = 0,
	CONTENT_FORMAT_WHOLE_CONTENT = 1,
	CONTENT_FORMAT_MULTIPLE_SECTIONS = 2
};

/*--------------------------------------------------------------*/
/*! \brief Struct definition to indicate specific event.        */
/*--------------------------------------------------------------*/
enum ENUM_STATUS_TYPE {
	ENUM_STATUS_TYPE_AUTHENTICATION,
	ENUM_STATUS_TYPE_MEDIA_STREAM_MODE,
	ENUM_STATUS_TYPE_CANDIDATE_LIST,
	ENUM_STATUS_TYPE_FT_AUTH_STATUS,
	ENUM_STATUS_TYPE_NUM	/*!< Upper bound, not real case */
};

struct PARAM_802_11_CONFIG_FH {
	uint32_t u4Length;	/*!< Length of structure */
	uint32_t u4HopPattern;	/*!< Defined as 802.11 */
	uint32_t u4HopSet;	/*!< to one if non-802.11 */
	uint32_t u4DwellTime;	/*!< In unit of Kusec */
};

struct PARAM_802_11_CONFIG {
	uint32_t u4Length;	/*!< Length of structure */
	uint32_t u4BeaconPeriod;	/*!< In unit of Kusec */
	uint32_t u4ATIMWindow;	/*!< In unit of Kusec */
	uint32_t u4DSConfig;	/*!< Channel frequency in unit of kHz */
	struct PARAM_802_11_CONFIG_FH rFHConfig;
};

struct PARAM_STATUS_INDICATION {
	enum ENUM_STATUS_TYPE eStatusType;
};

struct PARAM_AUTH_REQUEST {
	uint32_t u4Length;	/*!< Length of this struct */
	uint8_t arBssid[PARAM_MAC_ADDR_LEN];
	uint32_t u4Flags;	/*!< Definitions are as follows */
};

struct PARAM_PMKID {
	uint8_t arBSSID[PARAM_MAC_ADDR_LEN];
	uint8_t arPMKID[IW_PMKID_LEN];
	uint8_t ucBssIdx;
	struct PARAM_SSID rSsid;
	uint8_t arPMK[64];
	uint16_t u2PMKLen;
	uint8_t arFilsCacheId[2];
	uint8_t fgFilsCacheIdSet;
	uint32_t u4Expiration;
};

struct PARAM_PMKID_CANDIDATE {
	uint8_t arBSSID[PARAM_MAC_ADDR_LEN];
	uint32_t u4Flags;
};

struct PARAM_INDICATION_EVENT {
	struct PARAM_STATUS_INDICATION rStatus;
	union {
		struct PARAM_AUTH_REQUEST rAuthReq;
		struct PARAM_PMKID_CANDIDATE rCandi;
	};
};

/*! \brief Capabilities, privacy, rssi and IEs of each BSSID */
struct PARAM_BSSID_EX {
	uint32_t u4Length; /*!< sizeof(PARAM_BSSID_EX) + u2IELength  */
	uint8_t arMacAddress[PARAM_MAC_ADDR_LEN];	/*!< BSSID */
	uint8_t Reserved[2];
	struct PARAM_SSID rSsid;	/*!< SSID */
	uint32_t u4Privacy;	/*!< Need WEP encryption */
	int32_t rRssi;	/*!< in dBm */
	enum ENUM_PARAM_NETWORK_TYPE eNetworkTypeInUse;
	struct PARAM_802_11_CONFIG rConfiguration;
	enum ENUM_PARAM_OP_MODE eOpMode;
	uint8_t rSupportedRates[PARAM_MAX_LEN_RATES_EX];
	uint32_t u4IELength;
	uint8_t *pucIE; /* point to associated IE saved in aucScanIEBuf */
};

struct PARAM_BSSID_LIST_EX {
	uint32_t u4NumberOfItems;	/*!< at least 1 */
	struct PARAM_BSSID_EX arBssid[];
};

struct PARAM_LINK_BSS_INFO {
	uint8_t ucBssIndex;
	uint8_t aucMacAddr[PARAM_MAC_ADDR_LEN];
};

struct PARAM_WEP {
	uint32_t u4Length;	/*!< Length of structure */
	uint32_t u4KeyIndex;	/*!< 0: pairwise key, others group keys */
	uint32_t u4KeyLength;	/*!< Key length in bytes */
	uint8_t aucKeyMaterial[32];	/*!< Key content by above setting */
};

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
struct PARAM_FILS {
	const uint8_t *pucErpUsername;
	uint16_t u2ErpUsernameLen;
	const uint8_t *pucErpRealm;
	uint16_t pucErpRealmLen;
	uint32_t u4ErpNextSeqNum;
	const uint8_t *pucErpRrk;
	uint16_t u2ErpRrkLen;
};
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

/*! \brief Key mapping of BSSID */
struct PARAM_KEY {
	uint32_t u4Length;	/*!< Length of structure */
	uint32_t u4KeyIndex;	/*!< KeyID */
	uint32_t u4KeyLength;	/*!< Key length in bytes */
	uint8_t arBSSID[PARAM_MAC_ADDR_LEN];	/*!< MAC address */
	uint64_t rKeyRSC;
	uint8_t ucBssIdx;
	int32_t i4LinkId;
	uint8_t ucCipher;
	uint8_t aucKeyMaterial[32];	/*!< Key content by above setting */
	/* Following add to change the original windows structure */
	uint8_t aucKeyPn[6];
};

struct PARAM_REMOVE_KEY {
	uint32_t u4Length;	/*!< Length of structure */
	uint32_t u4KeyIndex;	/*!< KeyID */
	uint8_t arBSSID[PARAM_MAC_ADDR_LEN];	/*!< MAC address */
	uint8_t ucBssIdx;
	int32_t i4LinkId;
};

/*! \brief Default key */
struct PARAM_DEFAULT_KEY {
	uint8_t ucKeyID;
	uint8_t ucUnicast;
	uint8_t ucMulticast;
	uint8_t ucBssIdx;
	int32_t i4LinkId;
};

enum TSC_TYPE {
	TSC_TYPE_GTK_PN,
	TSC_TYPE_IGTK_PN,
	TSC_TYPE_BIGTK_PN,
	TSC_TYPE_NUM,
};

struct PARAM_TX_TSC_ENTRY {
	uint8_t ucTscType;
	uint8_t aucKeyPn[6];
};

struct PARAM_TX_TSC_INFO {
	uint8_t ucBssIdx;
	uint16_t ucWlanIdx;
	uint32_t u4TscCount;
	struct PARAM_TX_TSC_ENTRY aucEntries[TSC_TYPE_NUM];
};

struct PARAM_BEACON_KEY {
	uint8_t ucRoleIdx;
	uint8_t ucKeyIdx;
};


#if CFG_SUPPORT_WAPI
enum ENUM_KEY_TYPE {
	ENUM_WPI_PAIRWISE_KEY = 0,
	ENUM_WPI_GROUP_KEY
};

enum ENUM_WPI_PROTECT_TYPE {
	ENUM_WPI_NONE,
	ENUM_WPI_RX,
	ENUM_WPI_TX,
	ENUM_WPI_RX_TX
};

struct PARAM_WPI_KEY {
	enum ENUM_KEY_TYPE eKeyType;
	enum ENUM_WPI_PROTECT_TYPE eDirection;
	uint8_t ucKeyID;
	uint8_t aucRsv[3];
	uint8_t aucAddrIndex[12];
	uint32_t u4LenWPIEK;
	uint8_t aucWPIEK[256];
	uint32_t u4LenWPICK;
	uint8_t aucWPICK[256];
	uint8_t aucPN[16];
	uint8_t ucBssIdx;
};
#endif

enum PARAM_POWER_MODE {
	Param_PowerModeCAM,
	Param_PowerModeMAX_PSP,
	Param_PowerModeFast_PSP,
	Param_PowerModeMax	/* Upper bound, not real case */
};

enum PARAM_DEVICE_POWER_STATE {
	ParamDeviceStateUnspecified = 0,
	ParamDeviceStateD0,
	ParamDeviceStateD1,
	ParamDeviceStateD2,
	ParamDeviceStateD3,
	ParamDeviceStateMaximum
};

struct PARAM_POWER_MODE_ {
	uint8_t ucBssIdx;
	enum PARAM_POWER_MODE ePowerMode;
};

#if CFG_SUPPORT_802_11D

/*! \brief The enumeration definitions for OID_IPN_MULTI_DOMAIN_CAPABILITY */
enum PARAM_MULTI_DOMAIN_CAPABILITY {
	ParamMultiDomainCapDisabled,
	ParamMultiDomainCapEnabled
};
#endif

struct COUNTRY_STRING_ENTRY {
	uint8_t aucCountryCode[2];
	uint8_t aucEnvironmentCode[2];
};

struct COUNTRY_CODE_SETTING {
	uint8_t aucCountryCode[4];
	uint8_t ucCountryLength;
	uint8_t fgNeedHoldRtnlLock;
};

/* Power management related definition and enumerations */
#define UAPSD_NONE	0
#define UAPSD_AC0	(BIT(0) | BIT(4))
#define UAPSD_AC1	(BIT(1) | BIT(5))
#define UAPSD_AC2	(BIT(2) | BIT(6))
#define UAPSD_AC3	(BIT(3) | BIT(7))
#define UAPSD_ALL	(UAPSD_AC0 | UAPSD_AC1 | UAPSD_AC2 | UAPSD_AC3)

enum ENUM_POWER_SAVE_PROFILE {
	ENUM_PSP_CONTINUOUS_ACTIVE = 0,
	ENUM_PSP_CONTINUOUS_POWER_SAVE,
	ENUM_PSP_FAST_SWITCH,
	ENUM_PSP_NUM
};

struct LINK_SPEED_EX_ {
	int8_t cRssi;

	uint8_t fgIsLinkQualityValid;
	OS_SYSTIME rLinkQualityUpdateTime;
	int8_t cLinkQuality;

	uint8_t fgIsLinkRateValid;
	OS_SYSTIME rLinkRateUpdateTime;
	uint32_t u2TxLinkSpeed;
	uint32_t u2RxLinkSpeed;
	uint32_t u4RxBw;

	uint8_t ucMediumBusyPercentage;
	uint8_t ucIsLQ0Rdy;
};

struct PARAM_LINK_SPEED_EX {
	struct LINK_SPEED_EX_ rLq[MAX_BSSID_NUM];
};

/*--------------------------------------------------------------*/
/*! \brief Set/Query authentication and encryption capability.  */
/*--------------------------------------------------------------*/
struct PARAM_AUTH_ENCRYPTION {
	enum ENUM_PARAM_AUTH_MODE eAuthModeSupported;
	enum ENUM_WEP_STATUS eEncryptStatusSupported;
};

struct PARAM_CAPABILITY {
	uint32_t u4Length;
	uint32_t u4Version;
	uint32_t u4NoOfAuthEncryptPairsSupported;
	struct PARAM_AUTH_ENCRYPTION
		arAuthenticationEncryptionSupported[1];
};

#define NL80211_KCK_LEN                 16
#define NL80211_KEK_LEN                 16
#define NL80211_REPLAY_CTR_LEN          8
#define NL80211_KEYRSC_LEN		8

#define REKEY_COUNT_RESET_TIME		20000 /* msec */
#define REKEY_COUNT_LIMIT		1000

struct PARAM_GTK_REKEY_DATA {
	uint8_t aucKek[NL80211_KEK_LEN];
	uint8_t aucKck[NL80211_KCK_LEN];
	uint8_t aucReplayCtr[NL80211_REPLAY_CTR_LEN];
	uint8_t ucBssIndex;
	uint8_t ucRekeyMode;
	uint8_t ucCurKeyId;
	uint8_t ucRsv;
	uint32_t u4Proto;
	uint32_t u4PairwiseCipher;
	uint32_t u4GroupCipher;
	uint32_t u4KeyMgmt;
	uint32_t u4MgmtGroupCipher;
};

#if CFG_SUPPORT_WIFI_ICCM
struct PARAM_CUSTOM_ICCM_STRUCT {
	uint8_t u4Enable;
	uint8_t u4EnablePrintFw;
	uint32_t u4Value;
};
#endif

#if CFG_SUPPORT_WIFI_POWER_METRICS
struct PARAM_CUSTOM_POWER_METRICS_STRUCT {
	uint32_t u4Enable;
	uint32_t u4Value;
};
#endif

struct PARAM_CUSTOM_MCR_RW_STRUCT {
	uint32_t u4McrOffset;
	uint32_t u4McrData;
};

struct PARAM_MDVT_STRUCT {
	uint32_t u4ModuleId;
	uint32_t u4CaseId;
	uint8_t ucCapId;
	uint8_t ucReseved[3];
};

struct STAREC_COMMON {
	/* Basic STA record (Group0) */
	uint16_t u2Tag;		/* Tag = 0x00 */
	uint16_t u2Length;
	uint32_t u4ConnectionType;
	uint8_t ucConnectionState;
	uint8_t ucIsQBSS;
	uint16_t u2AID;
	uint8_t aucPeerMacAddr[6];
	uint16_t u2ExtraInfo;
};

/* RDD */
#define ATE_RDD_LOG_SIZE 8 /* Pulse size * num of pulse = 8 * 32 for one event*/
#define INC_RING_INDEX3(_idx, _RingSize)		\
{ \
	(_idx) = (_idx+1) % (_RingSize); \
	KAL_MB_W(); \
}
/* Log_Dump for Gen4m, align struct EVENT_WIFI_RDD_TEST */
struct _ATE_RDD_LOG {
	uint32_t u4Prefix;
	uint32_t u4Count;
	uint8_t byPass;
	uint8_t aucBuffer[ATE_RDD_LOG_SIZE];
};

struct _ATE_LOG_DUMP_ENTRY {
	uint32_t log_type; // UINT32 log_type;
	uint8_t un_dumped; // UINT8 un_dumped;
	struct _ATE_RDD_LOG rdd;
};

struct test_rdd_dump_params_s {
	uint32_t rdd_cnt;
	uint32_t rdd_dw_num;
};

struct _ATE_LOG_DUMP_CB {
	uint8_t overwritable; // UINT8 overwritable;
	uint8_t is_dumping; // UINT8 is_dumping;
	uint8_t is_overwritten; // UINT8 is_overwritten;
	int32_t idx; // INT32 idx;
	int32_t len; // INT32 len;
	uint32_t recal_curr_type; // UINT32 recal_curr_type;
#ifdef LOGDUMP_TO_FILE
	int file_idx; // INT32 file_idx;
	RTMP_OS_FD_EXT fd;
#endif
	struct _ATE_LOG_DUMP_ENTRY *entry;
};

/* Device information (Tag0) */
struct CMD_DEVINFO_ACTIVE {
	uint16_t u2Tag;		/* Tag = 0x00 */
	uint16_t u2Length;
	uint8_t ucActive;
	uint8_t ucBandNum;
	uint8_t aucOwnMacAddr[6];
	uint8_t ucOwnMacIdx;
	uint8_t aucReserve[3];
};

struct BSSINFO_BASIC {
	/* Basic BSS information (Tag1) */
	uint16_t u2Tag;		/* Tag = 0x01 */
	uint16_t u2Length;
	uint32_t u4NetworkType;
	uint8_t ucActive;
	uint8_t ucReserve0;
	uint16_t u2BcnInterval;
	uint8_t aucBSSID[6];
	uint8_t ucWmmIdx;
	uint8_t ucDtimPeriod;
	uint8_t ucBcMcWlanidx;	/* indicate which wlan-idx used for MC/BC
				 * transmission.
				 */
	uint8_t ucCipherSuit;
	uint8_t acuReserve[6];
};

struct BSSINFO_CONNECT_OWN_DEV {
	uint16_t u2Tag;	/* Tag = 0x00 */
	uint16_t u2Length;
	uint8_t  ucHwBSSIndex;
	uint8_t  ucOwnMacIdx;
	uint8_t  ucDbdcIdx;
	uint8_t  aucReserve;
	uint32_t u4ConnectionType;
	uint32_t u4Reserved;
};

/* Ext DevInfo Tag */
enum EXT_ENUM_DEVINFO_TAG_HANDLE {
	DEV_INFO_ACTIVE = 0,
	DEV_INFO_BSSID,
	DEV_INFO_MAX_NUM
};

/* STA record TLV tag */
enum EXT_ENUM_STAREC_TAG_HANDLE {
	STA_REC_BASIC = 0,
	STA_REC_RA,
	STA_REC_RA_COMMON_INFO,
	STA_REC_RA_UPDATE,
	STA_REC_BF,
	STA_REC_MAUNAL_ASSOC,
	STA_REC_BA = 6,
	STA_REC_MAX_NUM
};

enum {
	BSS_INFO_OWN_MAC = 0,
	BSS_INFO_BASIC = 1,
	BSS_INFO_RF_CH = 2,
	BSS_INFO_PM = 3,
	BSS_INFO_UAPSD = 4,
	BSS_INFO_ROAM_DETECTION = 5,
	BSS_INFO_LQ_RM = 6,
	BSS_INFO_EXT_BSS = 7,
	BSS_INFO_BROADCAST_INFO = 8,
	BSS_INFO_SYNC_MODE = 9,
	BSS_INFO_MAX_NUM
};

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
struct UNI_BASIC_BSSINFO_UPDATE {
	uint8_t ucOwnMacIdx;
	uint8_t ucBssIdx;
	uint8_t ucBandIdx;
	uint8_t ucBssId[MAC_ADDR_LEN];
};
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
struct PARAM_CUSTOM_SET_RDD_REPORT {
	uint8_t ucDbdcIdx; /* 0:Band 0, 1: Band1 */
};

struct PARAM_CUSTOM_SET_RADAR_DETECT_MODE {
	/* 0:Switch channel, 1: Don't switch channel */
	uint8_t ucRadarDetectMode;
};
#endif

#if CFG_SUPPORT_BUFFER_MODE
struct BIN_CONTENT {
	uint16_t u2Addr;
	uint8_t ucValue;
	uint8_t ucReserved;
};

struct PARAM_CUSTOM_EFUSE_BUFFER_MODE {
	uint8_t ucSourceMode;
	uint8_t ucCount;
	uint8_t ucCmdType;
	uint8_t ucReserved;
	uint8_t aBinContent[MAX_EEPROM_BUFFER_SIZE];
};

struct PARAM_CUSTOM_EFUSE_BUFFER_MODE_CONNAC_T {
	uint8_t ucSourceMode;
	uint8_t ucContentFormat;
	uint16_t u2Count;
	uint8_t aBinContent[BUFFER_BIN_PAGE_SIZE]; //Align with fw buffer size
};

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
struct PARAM_CUSTOM_ACCESS_EFUSE {
	uint32_t u4Address;
	uint32_t u4Valid;
	uint8_t aucData[16];
};

struct PARAM_CUSTOM_EFUSE_FREE_BLOCK {
	uint8_t ucGetFreeBlock;
	uint8_t ucGetTotalBlock;
	uint8_t ucDieIdx;
	uint8_t aucReserved[1];
};

struct PARAM_CUSTOM_EFUSE_BUFFER_RD_T {
	uint8_t ucSourceMode;
	uint8_t ucContentFormat;
	uint16_t u2Offset;
	uint16_t u2Count;
	uint16_t u2Reserved;
};

struct PARAM_CUSTOM_GET_TX_POWER {
	uint8_t ucTxPwrType;
	uint8_t ucCenterChannel;
	uint8_t ucDbdcIdx; /* 0:Band 0, 1: Band1 */
	uint8_t ucBand; /* 0:G-band 1: A-band*/
	uint8_t ucReserved[4];
};

/*#endif*/

#endif /* CFG_SUPPORT_BUFFER_MODE */

#if CFG_SUPPORT_TX_BF
enum BF_ACTION_CATEGORY {
	BF_SOUNDING_OFF = 0,
	BF_SOUNDING_ON,
	BF_DATA_PACKET_APPLY,
	BF_PFMU_MEM_ALLOCATE,
	BF_PFMU_MEM_RELEASE,
	BF_PFMU_TAG_READ,
	BF_PFMU_TAG_WRITE,
	BF_PROFILE_READ,
	BF_PROFILE_WRITE,
	BF_PN_READ,
	BF_PN_WRITE,
	BF_PFMU_MEM_ALLOC_MAP_READ,
	BF_AID_SET,
	BF_STA_REC_READ,
	BF_PHASE_CALIBRATION,
	BF_IBF_PHASE_COMP,
	BF_LNA_GAIN_CONFIG,
	BF_PROFILE_WRITE_20M_ALL,
	BF_APCLIENT_CLUSTER,
	BF_AWARE_CTRL,
	BF_HW_ENABLE_STATUS_UPDATE,
	BF_REPT_CLONED_STA_TO_NORMAL_STA,
	BF_GET_QD,
	BF_BFEE_HW_CTRL,
	BF_PFMU_SW_TAG_WRITE,
	BF_MOD_EN_CTRL,
	BF_DYNSND_EN_INTR,
	BF_DYNSND_CFG_DMCS_TH,
	BF_DYNSND_EN_PFID_INTR,
	BF_CONFIG,
	BF_PFMU_DATA_WRITE,
	BF_FBRPT_DBG_INFO_READ,
	BF_CMD_TXSND_INFO,
	BF_CMD_PLY_INFO,
	BF_CMD_MU_METRIC
};

enum {
	DEVINFO_ACTIVE = 0,
	DEVINFO_MAX_NUM = 1,
};

enum {
	DEVINFO_ACTIVE_FEATURE = (1 << DEVINFO_ACTIVE),
	DEVINFO_MAX_NUM_FEATURE = (1 << DEVINFO_MAX_NUM)
};

union PFMU_PROFILE_TAG1 {
	struct {
		uint32_t ucProfileID         : 10;
		/* [9:0]     : 0 ~ 1023 */
		uint32_t ucTxBf              : 1;
		/* [10]      : 0: iBF, 1: eBF */
		uint32_t ucDBW               : 2;
		/* [12:11]   : 0/1/2/3: DW20/40/80/160NC */
		uint32_t ucLM                : 2;
		/* [14:13]   : 0/1/2/3: Legacy/HT/VHT/HE */
		uint32_t ucSU_MU             : 1;
		/* [15]      : 0:SU, 1: MU */
		uint32_t ucNrow              : 3;
		/* [18:16]   : Nrow 3bits for 8x8 */
		uint32_t ucNcol              : 3;
		/* [21:19]   : Ncol 3bits for 8x8 */
		uint32_t ucCodeBook          : 2;
		/* [23:22]   : Code book */
		uint32_t ucNgroup            : 2;
		/* [25:24]   : Ngroup */
		uint32_t ucReserved          : 2;
		/* [27:26]   : Reserved */
		uint32_t ucInvalidProf       : 1;
		/* [28]      : 0:default,
		 *	       1: This profile number is invalid by SW
		 */
		uint32_t ucRMSD              : 3;
		/* [31:29]   : RMSD value from CE */
		uint32_t ucMemAddr1ColIdx    : 6;
		/* [37:32]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr1RowIdx    : 10;
		/* [47:38]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr2ColIdx    : 6;
		/* [53:48]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr2RowIdx    : 10;
		/* [63:54]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr3ColIdx    : 6;
		/* [69:64]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr3RowIdx    : 10;
		/* [79:70]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr4ColIdx    : 6;
		/* [85:80]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr4RowIdx    : 10;
		/* [95:86]   : row index : 0 ~ 63 */
		uint32_t ucRuStartIdx        : 7;
		/* [102:96]  : 0~73, only for HE profile (V matrix RU index) */
		uint32_t ucReserved1         : 1;
		/* [113]     : Reserved */
		uint32_t ucRuEndIdx          : 7;
		/* [110:104] : 0~73, only for HE profile (V matrix RU index) */
		uint32_t ucReserved2         : 1;
		/* [111]     : Reserved */
		uint32_t ucMobCalEn          : 1;
		/* [112]     : Mobility detection calculation enable */
		uint32_t ucReserved3         : 15;
		/* [127:113] : Reserved */
		uint32_t ucSNR_STS0          : 8;
		/* [135:128] : SNR_STS0 */
		uint32_t ucSNR_STS1          : 8;
		/* [143:136] : SNR_STS1 */
		uint32_t ucSNR_STS2          : 8;
		/* [151:144] : SNR_STS2 */
		uint32_t ucSNR_STS3          : 8;
		/* [159:152] : SNR_STS3 */
		uint32_t ucSNR_STS4          : 8;
		/* [167:160] : SNR_STS4 */
		uint32_t ucSNR_STS5          : 8;
		/* [175:168] : SNR_STS5 */
		uint32_t ucSNR_STS6          : 8;
		/* [183:176] : SNR_STS6 */
		uint32_t ucSNR_STS7          : 8;
		/* [191:184] : SNR_STS7 */
	} rField;

	struct {
		uint32_t ucProfileID         : 10;
		/* [9:0]     : 0 ~ 1023 */
		uint32_t ucTxBf              : 1;
		/* [10]      : 0: iBF, 1: eBF */
		uint32_t ucDBW               : 3;
		/* [13:11]   : 0/1/2/3/4: DW20/40/80/160/320 */
		uint32_t ucLM                : 3;
		/* [16:14]   : 0/1/2/3/4: Legacy/HT/VHT/HE/EHT */
		uint32_t ucSU_MU             : 1;
		/* [17]      : 0:SU, 1: MU */
		uint32_t ucNrow              : 3;
		/* [20:18]   : Nrow 3bits for 8x8 */
		uint32_t ucNcol              : 3;
		/* [23:21]   : Ncol 3bits for 8x8 */
		uint32_t ucCodeBook          : 2;
		/* [25:24]   : Code book */
		uint32_t ucNgroup            : 2;
		/* [27:26]   : Ngroup */
		uint32_t ucInvalidProf       : 1;
		/* [28]      : 0:default, 1:
		 *                    This profile number is invalid by SW
		 */
		uint32_t ucRserved0          : 3;
		/* [31:29]   : Reserved */
		uint32_t ucMemAddr1ColIdx    : 7;
		/* [38:32]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr1RowIdx    : 9;
		/* [47:39]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr2ColIdx    : 7;
		/* [54:48]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr2RowIdx    : 9;
		/* [63:55]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr3ColIdx    : 7;
		/* [70:64]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr3RowIdx    : 9;
		/* [79:71]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr4ColIdx    : 7;
		/* [86:80]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr4RowIdx    : 9;
		/* [95:87]   : row index : 0 ~ 63 */
		uint32_t ucPartialBWInfo     : 9;
		/* [104:96]  : Bitmap,
		 *               Follow NDP Partial BW Info (V matrix RU index)
		 */
		uint32_t ucReserved1         : 7;
		/* [111:105] : Reserved */
		uint32_t ucMobCalEn          : 1;
		/* [112]     : Mobility detection calculation enable */
		uint32_t ucReserved3         : 3;
		/* [115:113] : Reserved */
		uint32_t ucMobRuAlloc        : 9;
		/* [124:116] : EHT profile use the full 9-bit */
		uint32_t ucReserved4         : 3;
		/* [127:125] : Reserved */
		uint32_t ucSNR_STS0          : 8;
		/* [135:128] : SNR_STS0 */
		uint32_t ucSNR_STS1          : 8;
		/* [143:136] : SNR_STS1 */
		uint32_t ucSNR_STS2          : 8;
		/* [151:144] : SNR_STS2 */
		uint32_t ucSNR_STS3          : 8;
		/* [159:152] : SNR_STS3 */
		uint32_t ucSNR_STS4          : 8;
		/* [167:160] : SNR_STS4 */
		uint32_t ucSNR_STS5          : 8;
		/* [175:168] : SNR_STS5 */
		uint32_t ucSNR_STS6          : 8;
		/* [183:176] : SNR_STS6 */
		uint32_t ucSNR_STS7          : 8;
		/* [191:184] : SNR_STS7 */
	} rFieldv2; /* For CONNAC 3.0 EHT */

	struct {
		uint32_t ucProfileID         : 10;
		/* [9:0]     : 0 ~ 1023 */
		uint32_t ucTxBf              : 1;
		/* [10]      : 0: iBF, 1: eBF */
		uint32_t ucDBW               : 3;
		/* [13:11]   : 0/1/2/3/4: DW20/40/80/160/320 */
		uint32_t ucLM                : 3;
		/* [16:14]   : 0/1/2/3/4: Legacy/HT/VHT/HE/EHT */
		uint32_t ucSU_MU             : 1;
		/* [17]      : 0:SU, 1: MU */
		uint32_t ucNrow              : 3;
		/* [20:18]   : Nrow 3bits for 8x8 */
		uint32_t ucNcol              : 3;
		/* [23:21]   : Ncol 3bits for 8x8 */
		uint32_t ucCodeBook          : 2;
		/* [25:24]   : Code book */
		uint32_t ucNgroup            : 2;
		/* [27:26]   : Ngroup */
		uint32_t ucInvalidProf       : 1;
		/* [28]      : 0:default
		 *             1:This profile number is invalid by SW
		 */
		uint32_t ucRserved0          : 3;
		/* [31:29]   : Reserved */
		uint32_t ucMemAddr1ColIdx    : 7;
		/* [38:32]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr1RowIdx    : 9;
		/* [47:39]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr2ColIdx    : 7;
		/* [54:48]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr2RowIdx    : 9;
		/* [63:55]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr3ColIdx    : 7;
		/* [70:64]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr3RowIdx    : 9;
		/* [79:71]   : row index : 0 ~ 63 */
		uint32_t ucMemAddr4ColIdx    : 7;
		/* [86:80]   : column index : 0 ~ 5 */
		uint32_t ucMemAddr4RowIdx    : 9;
		/* [95:87]   : row index : 0 ~ 63 */
		uint32_t ucRuStartIdx        : 7;
		/* [102:96]  : 0~73, only for HE profile (V matrix RU index) */
		uint32_t ucReserved1         : 1;
		/* [103]     : Reserved */
		uint32_t ucRuEndIdx          : 7;
		/* [110:104] : 0~73, only for HE profile (V matrix RU index) */
		uint32_t ucReserved2         : 1;
		/* [111]     : Reserved */
		uint32_t ucMobCalEn          : 1;
		/* [112]     : Mobility detection calculation enable */
		uint32_t ucReserved3         : 3;
		/* [115:113] : Reserved */
		uint32_t ucMobRuAlloc        : 9;
		/* [124:116] : EHT profile use the full 9-bit */
		uint32_t ucReserved4         : 3;
		/* [127:125] : Reserved */
		uint32_t ucSNR_STS0          : 8;
		/* [135:128] : SNR_STS0 */
		uint32_t ucSNR_STS1          : 8;
		/* [143:136] : SNR_STS1 */
		uint32_t ucSNR_STS2          : 8;
		/* [151:144] : SNR_STS2 */
		uint32_t ucSNR_STS3          : 8;
		/* [159:152] : SNR_STS3 */
		uint32_t ucSNR_STS4          : 8;
		/* [167:160] : SNR_STS4 */
		uint32_t ucSNR_STS5          : 8;
		/* [175:168] : SNR_STS5 */
		uint32_t ucSNR_STS6          : 8;
		/* [183:176] : SNR_STS6 */
		uint32_t ucSNR_STS7          : 8;
		/* [191:184] : SNR_STS7 */
	} rFieldv3; /* For CONNAC 3.0 HE */
	uint32_t au4RawData[7];
};

union PFMU_PROFILE_TAG2 {
	struct {
		uint32_t u2SmartAnt       : 24;
		/* [23:0]    : Smart Ant config */
		uint32_t ucSEIdx          : 5;
		/* [28:24]   : SE index */
		uint32_t ucReserved       : 3;
		/* [31:29]   : Reserved */
		uint32_t ucReserved1      : 8;
		/* [39:32]   : Reserved */
		uint32_t ucRMSDThd        : 3;
		/* [42:40]   : RMSD Threshold */
		uint32_t ucReserved2      : 5;
		/* [47:43]   : Reserved */
		uint32_t uciBfTimeOut     : 8;
		/* [55:48]   : iBF timeout limit */
		uint32_t ucReserved3      : 8;
		/* [63:56]   : Reserved */
		uint32_t ucReserved4      : 16;
		/* [79:64]   : Reserved */
		uint32_t uciBfDBW         : 2;
		/* [81:80]   : iBF desired DBW 0/1/2/3 : BW20/40/80/160NC */
		uint32_t uciBfNcol        : 3;
		/* [84:82]   : iBF desired Ncol = 1 ~ 8 */
		uint32_t uciBfNrow        : 3;
		/* [87:85]   : iBF desired Nrow = 1 ~ 8 */
		uint32_t uciBfRu          : 8;
		/* [95:88]   : Desired RX packet RU index, only for HE profile
		 *		(OFDMA data RU index, not V matrix RU index)
		 */
		uint32_t ucMobDeltaT      : 8;
		/* [103:96]  : Mobility detection delta T value.
		 *		Resolution: 1ms. Max = 255ms.
		 */
		uint32_t ucMobLQResult    : 7;
		/* [110:104] : Mobility detection calculation result. U1.6 */
		uint32_t ucReserved5      : 1;
		/* [111]     : Reserved */
		uint32_t ucReserved6      : 16;
		/* [127:112] : Reserved */
	} rField;

	struct {
		uint32_t u2SmartAnt	  : 24;
		/* [23:0]    : Smart Ant config */
		uint32_t ucSEIdx	  : 5;
		/* [28:24]   : SE index */
		uint32_t ucReserved0	  : 3;
		/* [31:29]   : Reserved */
		uint32_t ucReserved1	  : 16;
		/* [47:32]   : Reserved */
		uint32_t uciBfTimeOut	  : 8;
		/* [55:48]   : iBF timeout limit */
		uint32_t ucReserved2	  : 8;
		/* [63:56]   : Reserved */
		uint32_t uciBfDBW	  : 3;
		/* [66:64]   : iBF desired DBW 0/1/2/3/4 : BW20/40/80/160/320 */
		uint32_t uciBfNcol	  : 3;
		/* [69:67]   : iBF desired Ncol = 1 ~ 8 */
		uint32_t uciBfNrow	  : 3;
		/* [72:70]   : iBF desired Nrow = 1 ~ 8 */
		uint32_t uciBfRu	  : 9;
		/* [81:73]   : Desired RX packet RU index */
		uint32_t ucReserved3	  : 14;
		/* [95:82]   : Reserved */
		uint32_t ucMobDeltaT	  : 8;
		/* [103:96]  : Mobility detection delta T value.
		 *                            Resolution: 1ms. Max = 255ms.
		 */
		uint32_t ucMobLQResult	  : 7;
		/* [110:104] : Mobility detection calculation result. U1.6 */
		uint32_t ucReserved4	  : 1;
		/* [111]     : Reserved */
		uint32_t ucReserved5	  : 16;
		/* [127:112] : Reserved */
	} rFieldv2; /* For CONNAC 3.0 */

	uint32_t au4RawData[7];
};

struct TXBF_LOW_SEG_ANGEL {
	/* DATA 0 */
	uint32_t u2Phi11          : 9;
	uint32_t ucPsi21          : 7;
	uint32_t u2Phi21          : 9;
	uint32_t ucPsi31          : 7;

	/* DATA 1*/
	uint32_t u2Phi31          : 9;
	uint32_t ucPsi41          : 7;
	uint32_t u2Phi41          : 9;
	uint32_t ucPsi51          : 7;

	/* DATA 2*/
	uint32_t u2Phi51          : 9;
	uint32_t ucPsi61          : 7;
	uint32_t u2Phi61          : 9;
	uint32_t ucPsi71          : 7;

	/* DATA 3*/
	uint32_t u2Phi71          : 9;
	uint32_t ucPsi81          : 7;
	uint32_t u2Phi22          : 9;
	uint32_t ucPsi32          : 7;

	/* DATA 4*/
	uint32_t u2Phi32          : 9;
	uint32_t ucPsi42          : 7;
	uint32_t u2Phi42          : 9;
	uint32_t ucPsi52          : 7;

	/* DATA 5*/
	uint32_t u2Phi52          : 9;
	uint32_t ucPsi62          : 7;
	uint32_t u2Phi62          : 9;
	uint32_t ucPsi72          : 7;

	/* DATA 6*/
	uint32_t u2Phi72          : 9;
	uint32_t ucPsi82          : 7;
	uint32_t u2Phi33          : 9;
	uint32_t ucPsi43          : 7;

	/* DATA 7*/
	uint32_t u2Phi43          : 9;
	uint32_t ucPsi53          : 7;
	uint32_t u2Phi53          : 9;
	uint32_t ucPsi63          : 7;

	/* DATA 8*/
	uint32_t u2Phi63          : 9;
	uint32_t ucPsi73          : 7;
	uint32_t u2Phi73          : 9;
	uint32_t ucPsi83          : 7;

	/* DATA 9*/
	uint32_t u2Phi44          : 9;
	uint32_t ucPsi54          : 7;
	uint32_t u2Phi54          : 9;
	uint32_t ucPsi64          : 7;

	/* DATA 10*/
	uint32_t u2Phi64          : 9;
	uint32_t ucPsi74          : 7;
	uint32_t u2Phi74          : 9;
	uint32_t ucPsi84          : 7;

	/* DATA 11*/
	uint32_t u2Phi55          : 9;
	uint32_t ucPsi65          : 7;
	uint32_t u2Phi65          : 9;
	uint32_t ucPsi75          : 7;

	/* DATA 12*/
	uint32_t u2Phi75          : 9;
	uint32_t ucPsi85          : 7;
	uint32_t u2Phi66          : 9;
	uint32_t ucPsi76          : 7;

	/* DATA 13*/
	uint32_t u2Phi76          : 9;
	uint32_t ucPsi86          : 7;
	uint32_t u2Phi77          : 9;
	uint32_t ucPsi87          : 7;
};

struct TXBF_HIGH_SEG_ANGEL {
	/* DATA 14 */
	uint32_t u2Phi11          : 9;
	uint32_t ucPsi21          : 7;
	uint32_t u2Phi21          : 9;
	uint32_t ucPsi31          : 7;

	/* DATA 15*/
	uint32_t u2Phi31          : 9;
	uint32_t ucPsi41          : 7;
	uint32_t u2Phi41          : 9;
	uint32_t ucPsi51          : 7;

	/* DATA 16*/
	uint32_t u2Phi51          : 9;
	uint32_t ucPsi61          : 7;
	uint32_t u2Phi61          : 9;
	uint32_t ucPsi71          : 7;

	/* DATA 17*/
	uint32_t u2Phi71          : 9;
	uint32_t ucPsi81          : 7;
	uint32_t u2Phi22          : 9;
	uint32_t ucPsi32          : 7;

	/* DATA 18*/
	uint32_t u2Phi32          : 9;
	uint32_t ucPsi42          : 7;
	uint32_t u2Phi42          : 9;
	uint32_t ucPsi52          : 7;

	/* DATA 19*/
	uint32_t u2Phi52          : 9;
	uint32_t ucPsi62          : 7;
	uint32_t u2Phi62          : 9;
	uint32_t ucPsi72          : 7;

	/* DATA 20*/
	uint32_t u2Phi72          : 9;
	uint32_t ucPsi82          : 7;
	uint32_t u2Phi33          : 9;
	uint32_t ucPsi43          : 7;

	/* DATA 21*/
	uint32_t u2Phi43          : 9;
	uint32_t ucPsi53          : 7;
	uint32_t u2Phi53          : 9;
	uint32_t ucPsi63          : 7;

	/* DATA 22*/
	uint32_t u2Phi63          : 9;
	uint32_t ucPsi73          : 7;
	uint32_t u2Phi73          : 9;
	uint32_t ucPsi83          : 7;

	/* DATA 23*/
	uint32_t u2Phi44          : 9;
	uint32_t ucPsi54          : 7;
	uint32_t u2Phi54          : 9;
	uint32_t ucPsi64          : 7;

	/* DATA 24*/
	uint32_t u2Phi64          : 9;
	uint32_t ucPsi74          : 7;
	uint32_t u2Phi74          : 9;
	uint32_t ucPsi84          : 7;

	/* DATA 25*/
	uint32_t u2Phi55          : 9;
	uint32_t ucPsi65          : 7;
	uint32_t u2Phi65          : 9;
	uint32_t ucPsi75          : 7;

	/* DATA 26*/
	uint32_t u2Phi75          : 9;
	uint32_t ucPsi85          : 7;
	uint32_t u2Phi66          : 9;
	uint32_t ucPsi76          : 7;

	/* DATA 27*/
	uint32_t u2Phi76          : 9;
	uint32_t ucPsi86          : 7;
	uint32_t u2Phi77          : 9;
	uint32_t ucPsi87          : 7;
};

struct TXBF_BFEE_LOW_SEG_SNR {
	/* DATA 28 */
	uint32_t u2dSNR00        : 10;
	uint32_t u2dSNR01        : 10;
	uint32_t u2dSNR02        : 10;
	uint32_t u2dSNR03        : 2;

	/* DATA 29 */
	uint32_t u2dSNR03_MSB    : 8;
	uint32_t u2dSNR04        : 10;
	uint32_t u2dSNR05        : 10;
	uint32_t u2dSNR06        : 4;

	/* DATA 30 */
	uint32_t u2dSNR06_MSB    : 6;
	uint32_t u2dSNR07        : 10;
	uint32_t reserved        : 16;
};

struct TXBF_BFEE_HIGH_SEG_SNR {
	/* DATA 30 */
	uint32_t reserved        : 16;
	uint32_t u2dSNR00        : 10;
	uint32_t u2dSNR01        : 6;

	/* DATA 31 */
	uint32_t u2dSNR01_MSB    : 4;
	uint32_t u2dSNR02        : 10;
	uint32_t u2dSNR03        : 10;
	uint32_t u2dSNR04        : 8;

	/* DATA 32 */
	uint32_t u2dSNR04_MSB    : 2;
	uint32_t u2dSNR05        : 10;
	uint32_t u2dSNR06        : 10;
	uint32_t u2dSNR07        : 10;
};

union PFMU_DATA {
	struct {
		struct TXBF_LOW_SEG_ANGEL rLowSegAng;
		struct TXBF_HIGH_SEG_ANGEL rHighSegAng;
		struct TXBF_BFEE_LOW_SEG_SNR rLowSegSnr;
		struct TXBF_BFEE_HIGH_SEG_SNR rHighSegSnr;
	} rField;
	uint32_t au4RawData[33];
};

union ORIGIN_PFMU_DATA {
	struct {
		uint32_t u2Phi11: 9;
		uint32_t ucPsi21: 7;
		uint32_t u2Phi21: 9;
		uint32_t ucPsi31: 7;
		uint32_t u2Phi31: 9;
		uint32_t ucPsi41: 7;
		uint32_t u2Phi22: 9;
		uint32_t ucPsi32: 7;
		uint32_t u2Phi32: 9;
		uint32_t ucPsi42: 7;
		uint32_t u2Phi33: 9;
		uint32_t ucPsi43: 7;
		uint32_t u2dSNR00: 4;
		uint32_t u2dSNR01: 4;
		uint32_t u2dSNR02: 4;
		uint32_t u2dSNR03: 4;
		uint32_t u2Reserved: 16;
	} rField;
	uint32_t au4RawData[5];
};

struct PFMU_HE_INFO {
	uint32_t u4Config;
	uint8_t fgSU_MU;
	uint8_t u1RuStartIdx;
	uint8_t u1RuEndIdx;
	uint8_t fgTriggerSu;
	uint8_t fgTriggerMu;
	uint8_t fgNg16Su;
	uint8_t fgNg16Mu;
	uint8_t fgCodebook42Su;
	uint8_t fgCodebook75Mu;
	uint8_t u1HeLtf;
	uint8_t uciBfNcol;
	uint8_t uciBfNrow;
	uint8_t ucNrBw160;
	uint8_t ucNcBw160;
};

enum PFMU_HE_MANUAL_CONF {
	MANUAL_HE_SU_MU = 0,
	MANUAL_HE_RU_RANGE,
	MANUAL_HE_TRIGGER,
	MANUAL_HE_NG16,
	MANUAL_HE_CODEBOOK,
	MANUAL_HE_LTF,
	MANUAL_HE_IBF,
	MANUAL_HE_BW160
};

struct PROFILE_TAG_READ {
	uint8_t ucTxBfCategory;
	uint8_t ucProfileIdx;
	uint8_t fgBfer;
	uint8_t ucBandIdx;
};

struct PROFILE_TAG_WRITE {
	uint8_t ucTxBfCategory;
	uint8_t ucPfmuId;
	uint8_t fgBFer;
	uint8_t ucBandIdx;
	uint8_t ucBuffer[64];
};

struct PROFILE_DATA_READ {
	uint8_t ucTxBfCategory;
	uint8_t ucPfmuIdx;
	uint8_t fgBFer;
	uint8_t ucBandIdx;
	uint8_t ucReserved[2];
	uint16_t u2SubCarIdx;
};

struct PROFILE_DATA_WRITE {
	uint8_t ucTxBfCategory;
	uint8_t ucPfmuIdx;
	uint8_t u2SubCarrIdxLsb;
	uint8_t u2SubCarrIdxMsb;
	union ORIGIN_PFMU_DATA rTxBfPfmuData;
};

struct PROFILE_PN_READ {
	uint8_t ucTxBfCategory;
	uint8_t ucPfmuIdx;
	uint8_t ucReserved[2];
};

struct PROFILE_PN_WRITE {
	uint8_t ucTxBfCategory;
	uint8_t ucPfmuIdx;
	uint16_t u2bw;
	uint8_t ucBuf[32];
};

enum BF_SOUNDING_MODE {
	SU_SOUNDING = 0,
	MU_SOUNDING,
	SU_PERIODIC_SOUNDING,
	MU_PERIODIC_SOUNDING,
	AUTO_SU_PERIODIC_SOUNDING,
	TXCMD_NONTB_SU_SOUNDING,
	TXCMD_VHT_MU_SOUNDING,
	TXCMD_TB_PER_BRP_SOUNDING,
	TXCMD_TB_SOUNDING,
	SOUNDING_MAX
};

enum BF_CBW {
	BF_CBW20 = 0,
	BF_CBW40,
	BF_CBW80,
	BF_CBW160,
	BF_CBW320,
};

struct TXBF_PFMU_STA_INFO {
	uint16_t u2PfmuId;	/* 0xFFFF means no access right for PFMU */
	uint8_t fgSU_MU;		/* 0 : SU, 1 : MU */
	uint8_t u1TxBfCap;      /* BIT(0) = 1 if ETxBf is supported :
				 * BIT(1) = 1 if ITxBf is supported
				 */
	uint8_t ucSoundingPhy;	/* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
	uint8_t ucNdpaRate;
	uint8_t ucNdpRate;
	uint8_t ucReptPollRate;
	uint8_t ucTxMode;	/* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
	uint8_t ucNc;
	uint8_t ucNr;
	uint8_t ucCBW;		/* 0 : 20M, 1 : 40M, 2 : 80M, 3 : 80 + 80M */
	uint8_t ucTotMemRequire;
	uint8_t ucMemRequire20M;
	uint8_t ucMemRow0;
	uint8_t ucMemCol0;
	uint8_t ucMemRow1;
	uint8_t ucMemCol1;
	uint8_t ucMemRow2;
	uint8_t ucMemCol2;
	uint8_t ucMemRow3;
	uint8_t ucMemCol3;
	uint16_t u2SmartAnt;
	uint8_t ucSEIdx;
	uint8_t ucAutoSoundingCtrl;
	uint8_t uciBfTimeOut;
	uint8_t uciBfDBW;
	uint8_t uciBfNcol;
	uint8_t uciBfNrow;
	uint8_t u1NrBw160;
	uint8_t u1NcBw160;
	uint8_t u1RuStartIdx;
	uint8_t u1RuEndIdx;
	uint8_t fgTriggerSu;
	uint8_t fgTriggerMu;
	uint8_t fgNg16Su;
	uint8_t fgNg16Mu;
	uint8_t fgCodebook42Su;
	uint8_t fgCodebook75Mu;
	uint8_t u1HeLtf;
};

struct STA_REC_UPD_ENTRY {
	struct TXBF_PFMU_STA_INFO rTxBfPfmuStaInfo;
	uint8_t aucAddr[PARAM_MAC_ADDR_LEN];
	uint8_t ucAid;
	uint8_t ucRsv;
};

struct CMD_STAREC_BF {
	uint16_t u2Tag;		/* Tag = 0x02 */
	uint16_t u2Length;
	struct TXBF_PFMU_STA_INFO rTxBfPfmuInfo;
	uint8_t ucReserved[3];
};


struct TX_BF_SOUNDING_START {
	uint8_t ucTxBfCategory;
	uint8_t ucSuMuSndMode;
	uint8_t ucStaNum;
	uint8_t ucReserved;
	uint8_t ucWlanId[4];
	uint32_t u4SoundingInterval;	/* By ms */
};

struct TX_BF_SOUNDING_STOP {
	uint8_t ucTxBfCategory;
	uint8_t ucSndgStop;
	uint8_t ucReserved[2];
};

struct TX_BF_TX_APPLY {
	uint8_t ucTxBfCategory;
	uint8_t ucWlanId;
	uint8_t fgETxBf;
	uint8_t fgITxBf;
	uint8_t fgMuTxBf;
	uint8_t ucReserved[3];
};

struct TX_BF_PFMU_MEM_ALLOC {
	uint8_t ucTxBfCategory;
	uint8_t ucSuMuMode;
	uint8_t ucWlanIdx;
	uint8_t ucReserved;
};

struct TX_BF_PFMU_MEM_RLS {
	uint8_t ucTxBfCategory;
	uint8_t ucWlanId;
	uint8_t ucReserved[2];
};

#if CFG_SUPPORT_TX_BF_FPGA
struct TX_BF_PROFILE_SW_TAG_WRITE {
	uint8_t ucTxBfCategory;
	uint8_t ucLm;
	uint8_t ucNr;
	uint8_t ucNc;
	uint8_t ucBw;
	uint8_t ucCodebook;
	uint8_t ucgroup;
	uint8_t ucTxBf;
	uint8_t ucReserved;
};
#endif

union PARAM_CUSTOM_TXBF_ACTION_STRUCT {
	struct PROFILE_TAG_READ rProfileTagRead;
	struct PROFILE_TAG_WRITE rProfileTagWrite;
	struct PROFILE_DATA_READ rProfileDataRead;
	struct PROFILE_DATA_WRITE rProfileDataWrite;
	struct PROFILE_PN_READ rProfilePnRead;
	struct PROFILE_PN_WRITE rProfilePnWrite;
	struct TX_BF_SOUNDING_START rTxBfSoundingStart;
	struct TX_BF_SOUNDING_STOP rTxBfSoundingStop;
	struct TX_BF_TX_APPLY rTxBfTxApply;
	struct TX_BF_PFMU_MEM_ALLOC rTxBfPfmuMemAlloc;
	struct TX_BF_PFMU_MEM_RLS rTxBfPfmuMemRls;
#if CFG_SUPPORT_TX_BF_FPGA
	struct TX_BF_PROFILE_SW_TAG_WRITE rTxBfProfileSwTagWrite;
#endif
};

struct PARAM_CUSTOM_STA_REC_UPD_STRUCT {
	uint8_t ucBssIndex;
	uint8_t ucWlanIdx;
	uint16_t u2TotalElementNum;
	uint8_t ucAppendCmdTLV;
	uint8_t ucMuarIdx;
	uint8_t aucReserve[2];
	uint32_t *prStaRec;
	struct CMD_STAREC_BF rCmdStaRecBf;
};

struct BSSINFO_ARGUMENT {
	uint8_t OwnMacIdx;
	uint8_t ucBssIndex;
	uint8_t Bssid[PARAM_MAC_ADDR_LEN];
	uint8_t ucBcMcWlanIdx;
	uint8_t ucPeerWlanIdx;
	uint32_t NetworkType;
	uint32_t u4ConnectionType;
	uint8_t CipherSuit;
	uint8_t Active;
	uint8_t WmmIdx;
	uint32_t u4BssInfoFeature;
	uint8_t aucBuffer[];
};

struct PARAM_CUSTOM_PFMU_TAG_READ_STRUCT {
	union PFMU_PROFILE_TAG1 ru4TxBfPFMUTag1;
	union PFMU_PROFILE_TAG2 ru4TxBfPFMUTag2;
};
#endif /* CFG_SUPPORT_TX_BF */

#if CFG_SUPPORT_QA_TOOL
struct PARAM_CUSTOM_SET_TX_TARGET_POWER {
	int8_t cTxPwr2G4Cck;	/* signed, in unit of 0.5dBm */
	int8_t cTxPwr2G4Dsss;	/* signed, in unit of 0.5dBm */
	uint8_t ucTxTargetPwr;	/* Tx target power base for all*/
	uint8_t ucReserved;

	int8_t cTxPwr2G4OFDM_BPSK;
	int8_t cTxPwr2G4OFDM_QPSK;
	int8_t cTxPwr2G4OFDM_16QAM;
	int8_t cTxPwr2G4OFDM_Reserved;
	int8_t cTxPwr2G4OFDM_48Mbps;
	int8_t cTxPwr2G4OFDM_54Mbps;

	int8_t cTxPwr2G4HT20_BPSK;
	int8_t cTxPwr2G4HT20_QPSK;
	int8_t cTxPwr2G4HT20_16QAM;
	int8_t cTxPwr2G4HT20_MCS5;
	int8_t cTxPwr2G4HT20_MCS6;
	int8_t cTxPwr2G4HT20_MCS7;

	int8_t cTxPwr2G4HT40_BPSK;
	int8_t cTxPwr2G4HT40_QPSK;
	int8_t cTxPwr2G4HT40_16QAM;
	int8_t cTxPwr2G4HT40_MCS5;
	int8_t cTxPwr2G4HT40_MCS6;
	int8_t cTxPwr2G4HT40_MCS7;

	int8_t cTxPwr5GOFDM_BPSK;
	int8_t cTxPwr5GOFDM_QPSK;
	int8_t cTxPwr5GOFDM_16QAM;
	int8_t cTxPwr5GOFDM_Reserved;
	int8_t cTxPwr5GOFDM_48Mbps;
	int8_t cTxPwr5GOFDM_54Mbps;

	int8_t cTxPwr5GHT20_BPSK;
	int8_t cTxPwr5GHT20_QPSK;
	int8_t cTxPwr5GHT20_16QAM;
	int8_t cTxPwr5GHT20_MCS5;
	int8_t cTxPwr5GHT20_MCS6;
	int8_t cTxPwr5GHT20_MCS7;

	int8_t cTxPwr5GHT40_BPSK;
	int8_t cTxPwr5GHT40_QPSK;
	int8_t cTxPwr5GHT40_16QAM;
	int8_t cTxPwr5GHT40_MCS5;
	int8_t cTxPwr5GHT40_MCS6;
	int8_t cTxPwr5GHT40_MCS7;
};

#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
struct PARAM_CUSTOM_ACCESS_RX_STAT {
	uint32_t u4SeqNum;
	uint32_t u4TotalNum;
};
#else
struct PARAM_CUSTOM_ACCESS_RX_STAT {
	uint16_t u2SeqNum;
	uint8_t ucDbdcIdx;
	/* bit[0] in event structure will tell new / old firmware format */
	uint8_t	ucData;
	uint32_t u4TotalNum;
};
#endif

#if CFG_SUPPORT_MU_MIMO
struct PARAM_CUSTOM_SHOW_GROUP_TBL_ENTRY_STRUCT {
	uint32_t u4EventId;
	uint8_t index;
	uint8_t numUser: 2;
	uint8_t BW: 2;
	uint8_t NS0: 2;
	uint8_t NS1: 2;
	/* UINT_8       NS2:1; */
	/* UINT_8       NS3:1; */
	uint8_t PFIDUser0;
	uint8_t PFIDUser1;
	/* UINT_8       PFIDUser2; */
	/* UINT_8       PFIDUser3; */
	u_int8_t fgIsShortGI;
	u_int8_t fgIsUsed;
	u_int8_t fgIsDisable;
	uint8_t initMcsUser0: 4;
	uint8_t initMcsUser1: 4;
	/* UINT_8       initMcsUser2:4; */
	/* UINT_8       initMcsUser3:4; */
	uint8_t dMcsUser0: 4;
	uint8_t dMcsUser1: 4;
	/* UINT_8       dMcsUser2:4; */
	/* UINT_8       dMcsUser3:4; */
};

struct PARAM_CUSTOM_GET_QD_STRUCT {
	uint32_t u4EventId;
	uint32_t au4RawData[14];
};

struct MU_STRUCT_LQ_REPORT {
	int lq_report[NUM_OF_USER][NUM_OF_MODUL];
};

struct PARAM_CUSTOM_GET_MU_CALC_LQ_STRUCT {
	uint32_t u4EventId;
	struct MU_STRUCT_LQ_REPORT rEntry;
};

struct MU_GET_CALC_INIT_MCS {
	uint8_t ucgroupIdx;
	uint8_t ucRsv[3];
};

struct MU_SET_INIT_MCS {
	uint8_t ucNumOfUser;	/* zero-base: 0~3: means 1~2 users */
	uint8_t ucBandwidth;	/* zero-base: 0:20 hz 1:40 hz 2: 80 hz 3: 160 */
	uint8_t ucNssOfUser0;	/* zero-base: 0~1 means uesr0 use 1~2 ss,
				 *            if no use keep 0
				 */
	uint8_t ucNssOfUser1;	/* zero-base: 0~1 means uesr0 use 1~2 ss,
				 *            if no use keep 0
				 */
	uint8_t ucPfMuIdOfUser0;/* zero-base: for now, uesr0 use pf mu id 0 */
	uint8_t ucPfMuIdOfUser1;/* zero-base: for now, uesr1 use pf mu id 1 */
	uint8_t ucNumOfTxer;	/* 0~3: mean use 1~4 anntain, for now,
				 *      should fix 3
				 */
	uint8_t ucSpeIndex;	/* add new field to fill"special extension
				 * index" which replace reserve
				 */
	uint32_t u4GroupIndex;	/* 0~ :the index of group table entry for
				 *     calculation
				 */
};

struct MU_SET_CALC_LQ {
	uint8_t ucNumOfUser;	/* zero-base: 0~3: means 1~2 users */
	uint8_t ucBandwidth;	/* zero-base: 0:20 hz 1:40 hz 2: 80 hz 3: 160 */
	uint8_t ucNssOfUser0;	/* zero-base: 0~1 means uesr0 use 1~2 ss,
				 *            if no use keep 0
				 */
	uint8_t ucNssOfUser1;	/* zero-base: 0~1 means uesr0 use 1~2 ss,
				 *            if no use keep 0
				 */
	uint8_t ucPfMuIdOfUser0;/* zero-base: for now, uesr0 use pf mu id 0 */
	uint8_t ucPfMuIdOfUser1;/* zero-base: for now, uesr1 use pf mu id 1 */
	uint8_t ucNumOfTxer;	/* 0~3: mean use 1~4 anntain, for now,
				 *      should fix 3
				 */
	uint8_t ucSpeIndex;	/* add new field to fill"special extension
				 * index" which replace reserve
				 */
	uint32_t u4GroupIndex;	/* 0~ : the index of group table entry for
				 *      calculation
				 */
};

struct MU_GET_LQ {
	uint8_t ucType;
	uint8_t ucRsv[3];
};

struct MU_SET_SNR_OFFSET {
	uint8_t ucVal;
	uint8_t ucRsv[3];
};

struct MU_SET_ZERO_NSS {
	uint8_t ucVal;
	uint8_t ucRsv[3];
};

struct MU_SPEED_UP_LQ {
	uint32_t u4Val;
};

struct MU_SET_MU_TABLE {
	/* UINT_16  u2Type; */
	/* UINT_32  u4Length; */
	uint8_t aucMetricTable[NUM_MUT_NR_NUM * NUM_MUT_FEC *
			       NUM_MUT_MCS * NUM_MUT_INDEX];
};

struct MU_SET_GROUP {
	uint32_t u4GroupIndex;	/* Group Table Idx */
	uint32_t u4NumOfUser;
	uint32_t u4User0Ldpc;
	uint32_t u4User1Ldpc;
	uint32_t u4ShortGI;
	uint32_t u4Bw;
	uint32_t u4User0Nss;
	uint32_t u4User1Nss;
	uint32_t u4GroupId;
	uint32_t u4User0UP;
	uint32_t u4User1UP;
	uint32_t u4User0MuPfId;
	uint32_t u4User1MuPfId;
	uint32_t u4User0InitMCS;
	uint32_t u4User1InitMCS;
	uint8_t aucUser0MacAddr[PARAM_MAC_ADDR_LEN];
	uint8_t aucUser1MacAddr[PARAM_MAC_ADDR_LEN];
};

struct MU_GET_QD {
	uint8_t ucSubcarrierIndex;
	/* UINT_32 u4Length; */
	/* UINT_8 *prQd; */
};

struct MU_SET_ENABLE {
	uint8_t ucVal;
	uint8_t ucRsv[3];
};

struct MU_SET_GID_UP {
	uint32_t au4Gid[2];
	uint32_t au4Up[4];
};

struct MU_TRIGGER_MU_TX {
	uint8_t  fgIsRandomPattern;	/* is random pattern or not */
	uint32_t u4MsduPayloadLength0;	/* payload length of the MSDU for
					 * user 0
					 */
	uint32_t u4MsduPayloadLength1;	/* payload length of the MSDU for
					 * user 1
					 */
	uint32_t u4MuPacketCount;	/* MU TX count */
	uint32_t u4NumOfSTAs;		/* number of user in the MU TX */
	uint8_t   aucMacAddrs[2][6];	/* MAC address of users*/
};

struct PARAM_CUSTOM_MUMIMO_ACTION_STRUCT {
	uint8_t ucMuMimoCategory;
	uint8_t aucRsv[3];
	union {
		struct MU_GET_CALC_INIT_MCS rMuGetCalcInitMcs;
		struct MU_SET_INIT_MCS rMuSetInitMcs;
		struct MU_SET_CALC_LQ rMuSetCalcLq;
		struct MU_GET_LQ rMuGetLq;
		struct MU_SET_SNR_OFFSET rMuSetSnrOffset;
		struct MU_SET_ZERO_NSS rMuSetZeroNss;
		struct MU_SPEED_UP_LQ rMuSpeedUpLq;
		struct MU_SET_MU_TABLE rMuSetMuTable;
		struct MU_SET_GROUP rMuSetGroup;
		struct MU_GET_QD rMuGetQd;
		struct MU_SET_ENABLE rMuSetEnable;
		struct MU_SET_GID_UP rMuSetGidUp;
		struct MU_TRIGGER_MU_TX rMuTriggerMuTx;
	} unMuMimoParam;
};
#endif /* CFG_SUPPORT_MU_MIMO */

/* QA tool: maunal assoc */
struct CMD_MANUAL_ASSOC_STRUCT {
	/*
	 *	uint8_t              ucBssIndex;
	 *	uint8_t              ucWlanIdx;
	 *	uint16_t             u2TotalElementNum;
	 *	uint32_t             u4Reserve;
	 */
	/* extension */
	uint16_t u2Tag;		/* Tag = 0x05 */
	uint16_t u2Length;
	uint8_t aucMac[MAC_ADDR_LEN];
	uint8_t ucType;
	uint8_t ucWtbl;
	uint8_t ucOwnmac;
	uint8_t ucMode;
	uint8_t ucBw;
	uint8_t ucNss;
	uint8_t ucPfmuId;
	uint8_t ucMarate;
	uint8_t ucSpeIdx;
	uint8_t ucaid;
};

#endif /* CFG_SUPPORT_QA_TOOL */

struct PARAM_CUSTOM_MEM_DUMP_STRUCT {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
	uint32_t u4IcapContent;
#endif				/* CFG_SUPPORT_QA_TOOL */
	uint8_t ucFragNum;
};

struct PARAM_CUSTOM_SW_CTRL_STRUCT {
	uint32_t u4Id;
	uint32_t u4Data;
};

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
struct PARAM_CUSTOM_ICS_SNIFFER_INFO_STRUCT {
	/* Include system all and PSSniffer */
	uint8_t ucModule;
	uint8_t ucAction;
	uint8_t ucFilter;
	uint8_t ucOperation;
	uint16_t ucCondition[7];
	uint8_t aucPadding0[62];
};
#endif /* CFG_SUPPORT_ICS */
#if (CFG_SUPPORT_PHY_ICS == 1)
struct PARAM_CUSTOM_PHYICS_EVENT_STRUCT {
	uint8_t ucBandIdx;
	uint8_t ucPartition;
	uint16_t u2EventGroup;
	uint32_t u4EventID;
};

struct PARAM_CUSTOM_PHYICS_START_STRUCT {
	uint16_t u2Action;
	uint16_t u2Timer;
};
#endif

struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT {
	uint16_t u2Id;
	uint8_t ucType;
	uint8_t ucRespType;
	uint16_t u2MsgSize;
	uint8_t aucReserved0[2];
	uint8_t aucCmd[CHIP_CONFIG_RESP_SIZE];
};

struct PARAM_CUSTOM_KEY_CFG_STRUCT {
	uint8_t aucKey[WLAN_CFG_KEY_LEN_MAX];
	uint8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];
	uint32_t u4Flag;
};

struct EEPROM_RW_INFO {
	uint8_t ucEepromIndex;
	uint8_t reserved;
	uint16_t u2EepromData;
};
struct NVRAM_RW_INFO {
	uint16_t u2NvIndex;
	uint16_t u2NvData;
};

struct PARAM_CUSTOM_EEPROM_RW_STRUCT {
	uint8_t ucMethod;
	union {
		struct EEPROM_RW_INFO rEeprom;
		struct NVRAM_RW_INFO rNvram;
	} info;
};

struct PARAM_CUSTOM_WMM_PS_TEST_STRUCT {
	uint8_t bmfgApsdEnAc;		/* b0~3: trigger-en AC0~3.
					 * b4~7: delivery-en AC0~3
					 */
	uint8_t ucIsEnterPsAtOnce;	/* enter PS immediately without 5 second
					 * guard after connected
					 */
	uint8_t ucIsDisableUcTrigger;	/* not to trigger UC on beacon TIM is
					 * matched (under U-APSD)
					 */
	uint8_t reserved;
	uint8_t ucBssIdx;
};

struct PARAM_CUSTOM_NOA_PARAM_STRUCT {
	uint32_t u4NoaDurationMs;
	uint32_t u4NoaIntervalMs;
	uint32_t u4NoaCount;
	uint8_t ucBssIdx;
};

struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT {
	uint32_t u4CTwindowMs;
	uint8_t ucBssIdx;
};

struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT {
	uint8_t ucBssIdx;
	uint8_t fgEnAPSD;
	uint8_t fgEnAPSD_AcBe;
	uint8_t fgEnAPSD_AcBk;
	uint8_t fgEnAPSD_AcVo;
	uint8_t fgEnAPSD_AcVi;
	uint8_t ucMaxSpLen;
	uint8_t aucResv[2];
};

struct PARAM_CUSTOM_P2P_SET_STRUCT {
	uint32_t u4Enable;
	uint32_t u4Mode;
};

struct PARAM_CUSTOM_P2P_SET_WITH_LOCK_STRUCT {
	uint32_t u4Enable;
	uint32_t u4Mode;
	uint8_t fgIsRtnlLockAcquired;
	u_int8_t fgIsWiphyLockHeld;
};

#define MAX_NUMBER_OF_ACL 20

enum ENUM_PARAM_CUSTOM_ACL_POLICY {
	PARAM_CUSTOM_ACL_POLICY_DISABLE,
	PARAM_CUSTOM_ACL_POLICY_ACCEPT,
	PARAM_CUSTOM_ACL_POLICY_DENY,
	PARAM_CUSTOM_ACL_POLICY_CLEAR,
	PARAM_CUSTOM_ACL_POLICY_ADD,
	PARAM_CUSTOM_ACL_POLICY_REMOVE,
};

struct PARAM_CUSTOM_ACL_ENTRY {
	uint8_t aucAddr[MAC_ADDR_LEN];
	uint16_t u2Rsv;
};

struct PARAM_CUSTOM_ACL {
	enum ENUM_PARAM_CUSTOM_ACL_POLICY ePolicy;
	uint32_t u4Num;
	struct PARAM_CUSTOM_ACL_ENTRY rEntry[MAX_NUMBER_OF_ACL];
};

enum ENUM_CFG_SRC_TYPE {
	CFG_SRC_TYPE_EEPROM,
	CFG_SRC_TYPE_NVRAM,
	CFG_SRC_TYPE_UNKNOWN,
	CFG_SRC_TYPE_NUM
};

enum ENUM_EEPROM_TYPE {
	EEPROM_TYPE_NO,
	EEPROM_TYPE_PRESENT,
	EEPROM_TYPE_NUM
};

enum ENUM_ICAP_STATE {
	ICAP_STATE_INIT = 0,
	ICAP_STATE_START = 1,
	ICAP_STATE_QUERY_STATUS = 2,
	ICAP_STATE_FW_DUMPING = 3,
	ICAP_STATE_FW_DUMP_DONE = 4,
	ICAP_STATE_QA_TOOL_CAPTURE = 5,
	ICAP_STATE_NUM
};


struct PARAM_QOS_TSINFO {
	uint8_t ucTrafficType;	/* Traffic Type: 1 for isochronous 0 for
				 *               asynchronous
				 */
	uint8_t ucTid;		/* TSID: must be between 8 ~ 15 */
	uint8_t ucDirection;	/* direction */
	uint8_t ucAccessPolicy;	/* access policy */
	uint8_t ucAggregation;	/* aggregation */
	uint8_t ucApsd;		/* APSD */
	uint8_t ucuserPriority;	/* user priority */
	uint8_t ucTsInfoAckPolicy;	/* TSINFO ACK policy */
	uint8_t ucSchedule;	/* Schedule */
};

struct PARAM_QOS_TSPEC {
	struct PARAM_QOS_TSINFO rTsInfo;	/* TS info field */
	uint16_t u2NominalMSDUSize;	/* nominal MSDU size */
	uint16_t u2MaxMSDUsize;	/* maximum MSDU size */
	uint32_t u4MinSvcIntv;	/* minimum service interval */
	uint32_t u4MaxSvcIntv;	/* maximum service interval */
	uint32_t u4InactIntv;	/* inactivity interval */
	uint32_t u4SpsIntv;	/* suspension interval */
	uint32_t u4SvcStartTime;	/* service start time */
	uint32_t u4MinDataRate;	/* minimum Data rate */
	uint32_t u4MeanDataRate;	/* mean data rate */
	uint32_t u4PeakDataRate;	/* peak data rate */
	uint32_t u4MaxBurstSize;	/* maximum burst size */
	uint32_t u4DelayBound;	/* delay bound */
	uint32_t u4MinPHYRate;	/* minimum PHY rate */
	uint16_t u2Sba;		/* surplus bandwidth allowance */
	uint16_t u2MediumTime;	/* medium time */
	uint8_t  ucDialogToken;
};

struct PARAM_QOS_ADDTS_REQ_INFO {
	struct PARAM_QOS_TSPEC rTspec;
};

struct PARAM_VOIP_CONFIG {
	uint32_t u4VoipTrafficInterval;	/* 0: disable VOIP configuration */
};

/*802.11 Statistics Struct*/
struct PARAM_802_11_STATISTICS_STRUCT {
	uint8_t ucInvalid;
	union LARGE_INTEGER rTransmittedFragmentCount;
	union LARGE_INTEGER rMulticastTransmittedFrameCount;
	union LARGE_INTEGER rFailedCount;
	union LARGE_INTEGER rRetryCount;
	union LARGE_INTEGER rMultipleRetryCount;
	union LARGE_INTEGER rRTSSuccessCount;
	union LARGE_INTEGER rRTSFailureCount;
	union LARGE_INTEGER rACKFailureCount;
	union LARGE_INTEGER rFrameDuplicateCount;
	union LARGE_INTEGER rReceivedFragmentCount;
	union LARGE_INTEGER rMulticastReceivedFrameCount;
	union LARGE_INTEGER rFCSErrorCount;
	union LARGE_INTEGER rMdrdyCnt;
	union LARGE_INTEGER rChnlIdleCnt;
	uint32_t u4HwAwakeDuration;
	uint32_t u4RstReason;
	uint64_t u8RstTime;
	uint32_t u4RoamFailCnt;
	uint64_t u8RoamFailTime;
	uint8_t u2TxDoneDelayIsARP;
	uint32_t u4ArriveDrvTick;
	uint32_t u4EnQueTick;
	uint32_t u4DeQueTick;
	uint32_t u4LeaveDrvTick;
	uint32_t u4CurrTick;
	uint64_t u8CurrTime;
};

/* Linux Network Device Statistics Struct */
struct PARAM_LINUX_NETDEV_STATISTICS {
	uint32_t u4RxPackets;
	uint32_t u4TxPackets;
	uint32_t u4RxBytes;
	uint32_t u4TxBytes;
	uint32_t u4RxErrors;
	uint32_t u4TxErrors;
	uint32_t u4Multicast;
};


#if CFG_SUPPORT_LLS
/* Cmd */
struct CMD_GET_STATS_LLS {
	uint32_t u4Tag; /* enum ENUM_STATS_LLS_TLV_TAG_ID */
	uint8_t ucArg0;
	uint8_t ucArg1;
	uint8_t ucArg2;
	uint8_t ucArg3;
};

/* Used in Event as well */
enum ENUM_STATS_LLS_TLV_TAG_ID {
	STATS_LLS_TAG_LLS_DATA           = 0,
	STATS_LLS_TAG_PPDU_LATENCY       = 1,
	STATS_LLS_TAG_CURRENT_TX_RATE    = 2,
	STATS_LLS_TAG_SET_WFD_TX_BITRATE_MONTR    = 3,
	STATS_LLS_TAG_GET_WFD_PRED_TX_BITRATE     = 4,
	STATS_LLS_TAG_BSS_CURRENT_TX_RATE     = 7,
	STATS_LLS_TAG_GET_WFD_BSS_PRED_TX_BITRATE     = 8,
	STATS_LLS_TAG_MAX_NUM
};

/* Event */
enum ENUM_STATS_LLS_UPDATE_STATUS {
	STATS_LLS_UPDATE_STATUS_SUCCESS = 0,
	STATS_LLS_UPDATE_STATUS_FAIL    = 1,
};

struct EVENT_STATS_LLS_DATA {
	enum ENUM_STATS_LLS_UPDATE_STATUS eUpdateStatus;
};


#define STATS_LATENCY_LEVEL_NUM 4
#define STATS_LATENCY_CATEGORY_NUM (STATS_LATENCY_LEVEL_NUM + 1)
struct EVENT_STATS_LLS_TX_LATENCY {
	uint32_t arLatencyLevel[STATS_LATENCY_LEVEL_NUM];
	uint32_t arLatencyMpduCntPerLevel[STATS_LATENCY_CATEGORY_NUM];
};

struct _STATS_LLS_TX_RATE_INFO {
	uint32_t rate :6,
	mode :4,
	nsts :3,
	stbc :1,
	bw :2,
	reserved :16;
};

struct EVENT_STATS_LLS_TX_RATE_INFO {
	struct _STATS_LLS_TX_RATE_INFO arTxRateInfo[BSSID_NUM];
};

struct EVENT_STATS_LLS_TX_BIT_RATE {
	uint32_t au4CurrentBitrate[BSSID_NUM];
	uint32_t au4PredictBitrate[BSSID_NUM];
};

struct STATS_LLS_TX_BIT_RATE {
	uint32_t u4CurrentBitrate;
	uint32_t u4PredictBitrate;
};
#endif /* CFG_SUPPORT_LLS */

struct PARAM_QUERY_STA_RX_INFO {
	struct STA_RECORD *prStaRec;
	uint32_t *pu4CurRate;
	uint32_t *pu4MaxRate;
	struct RxRateInfo *prRxRateInfo;
	int32_t *pu4Rssi;
};

#if CFG_SUPPORT_MBRAIN_BIGDATA
#define IPI_HIST_LEVEL_NUM 11

struct BIG_DATA_BSS_CNT_T {
	uint32_t u4RtsFail;
	uint32_t u4RtsTx;
	uint32_t u4BaseRtsFail;
	uint32_t u4BaseRtsTx;
	uint8_t ucCuAll;
	uint8_t ucCuNotMe;
	uint8_t rsvd[10];
};

struct BIG_DATA_ABT_CNT {
	uint32_t u4TmacAbt;
	uint32_t u4AggAbt;
	uint32_t u4ArbAbt;
	uint32_t u4PtaAbt;
	uint32_t u4CcaAbt;
	uint32_t u4MloAbt;
};

struct BIG_DATA_PHY_CNT {
	uint32_t u4CckPd;
	uint32_t u4OfdmPd;
	uint32_t u4CckSfdErr;
	uint32_t u4CckSigErr;
	uint32_t u4OfdmTaqErr;
	uint32_t u4OfdmSigErr;
	uint32_t u4CckFcsErr;
	uint32_t u4OfdmFcsErr;
	uint32_t u4CckMdrdy;
	uint32_t u4OfdmMdrdy;
	uint32_t u4CckRxOk;
	uint32_t u4OfdmRxOk;
	uint32_t u4CckTx;
	uint32_t u4OfdmTx;
	uint32_t u4Pop;
	uint32_t au4IPIHist[MAX_ANTENNA_NUM][IPI_HIST_LEVEL_NUM];
	uint8_t ucNbiPwr;
	uint16_t u2NbiFreq;
	uint8_t ucRsvd;
};

#define BIG_DATA_MAX_STA_NUM 10
struct BIG_DATA_STA_INFO {
	uint16_t u2WtblIdx;
	uint8_t fgValid;
	uint8_t reserved;
#if CFG_SUPPORT_STA_INFO
	uint32_t u4RxBmcMgmtCnt;
#endif
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint16_t u2TxLinkSpeed; //Unit: 100Kbps
	uint8_t aucSnr[MAX_ANTENNA_NUM];
	uint8_t aucRespRcpi[MAX_ANTENNA_NUM];
	uint8_t aucRsvd1[12];
};

struct PARAM_QUERY_STA_BIG_DATA {
	uint8_t ucWlanIdx;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucRsvd;
	uint8_t aucSnr[MAX_ANTENNA_NUM];
	uint8_t aucRespRcpi[MAX_ANTENNA_NUM];
	uint16_t u2TxLinkSpeed; //Unit: 100Kbps
#if CFG_SUPPORT_STA_INFO
	uint32_t u4RxBmcMgmtCnt;
#endif
};

struct PARAM_QUERY_TRX_LATENCY_BIG_DATA {
	struct BIG_DATA_ABT_CNT rAbtCnt;
	uint32_t u4RtsFailRate;
	uint32_t au4IPIHist[MAX_ANTENNA_NUM][IPI_HIST_LEVEL_NUM];
	union {
		struct {
			int16_t pwr;
			uint16_t freq;
		} fields;
		uint32_t u4Nbi;
	};
	uint8_t ucCuAll;
	uint8_t ucCuNotMe;
	uint8_t ucPhyRxPer;
	uint8_t rsvd;
};
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */

struct TX_LAT_MONTR_PARAM_STRUCT {
	bool fgEnabled;
	uint32_t u4Intvl;
	uint32_t u4DriverCrit;
	uint32_t u4MacCrit;
	bool fgIsAvg;
};

struct CMD_RDD_ON_OFF_CTRL_T {
	uint8_t ucDfsCtrl;
	uint8_t ucRddIdx;
	uint8_t ucRddRxSel;
	uint8_t ucSetVal;
	uint8_t aucReserved[4];
};

__KAL_ATTRIB_PACKED_FRONT__
struct PARAM_MTK_WIFI_TEST_STRUCT {
	uint32_t u4FuncIndex;
	uint32_t u4FuncData;
}__KAL_ATTRIB_PACKED__;

#define MAX_IQ_ARRAY_WF_CNT 4
struct _RBIST_IQ_DATA_T {
	int32_t u4IQArray[MAX_IQ_ARRAY_WF_CNT][2]; /* IQ_Array[WF][IQ] */
};

struct RECAL_DATA_T {
	uint32_t u4CalId;
	uint32_t u4CalAddr;
	uint32_t u4CalValue;
};

struct RECAL_INFO_T {
	u_int8_t fgDumped;
	uint32_t u4Count;
	struct RECAL_DATA_T *prCalArray;
};

struct ICAP_INFO_T {
	enum ENUM_ICAP_STATE eIcapState;
	uint32_t u4CapNode;

#if CFG_SUPPORT_QA_TOOL
	/* for MT6632/MT7668 file dump mode */
	uint16_t u2DumpIndex;
	uint32_t au4Offset[2][2];
	uint32_t au4IQData[256];

	/* for MT7663/Connad FW parser mode */
	uint32_t u4IQArrayIndex;
	uint32_t u4ICapEventCnt;	/* Count of packet getting from FW */
	uint32_t au4ICapDumpIndex[4][2];/* Count of packet sent to QA Tool,
					 * 4 WF * 2 I/Q
					 */
	struct _RBIST_IQ_DATA_T *prIQArray;
#endif
};

struct RF_TEST_CALIBRATION_T {
	uint32_t	u4FuncData;
	uint8_t	ucDbdcIdx;
	uint8_t	aucReserved[3];
};

struct TX_TONE_PARAM_T {
	uint8_t ucAntIndex;
	uint8_t ucToneType;
	uint8_t ucToneFreq;
	uint8_t ucDbdcIdx;
	int32_t i4DcOffsetI;
	int32_t i4DcOffsetQ;
	uint32_t u4Band;
};

struct CONTINUOUS_TX_PARAM_T {
	uint8_t ucCtrlCh;
	uint8_t ucCentralCh;
	uint8_t ucBW;
	uint8_t ucAntIndex;
	uint16_t u2RateCode;
	uint8_t ucBand;
	uint8_t ucTxfdMode;
};

struct TX_TONE_POWER_GAIN_T {
	uint8_t ucAntIndex;
	uint8_t ucTonePowerGain;
	uint8_t ucBand;
	uint8_t aucReserved[1];
};

struct EXT_CMD_RDD_ON_OFF_CTRL_T {
	uint8_t ucDfsCtrl;
	uint8_t ucRddIdx;
	uint8_t ucRddRxSel;
	uint8_t ucSetVal;
	uint8_t aucReserved[4];
};

struct SET_ADC_T {
	uint32_t  u4ChannelFreq;
	uint8_t	ucAntIndex;
	uint8_t	ucBW;
	uint8_t   ucSX;
	uint8_t	ucDbdcIdx;
	uint8_t	ucRunType;
	uint8_t	ucFType;
	uint8_t	aucReserved[2];		/* Reserving For future */
};

struct SET_RX_GAIN_T {
	uint8_t	ucLPFG;
	uint8_t   ucLNA;
	uint8_t	ucDbdcIdx;
	uint8_t	ucAntIndex;
};

struct SET_TTG_T {
	uint32_t  u4ChannelFreq;
	uint32_t  u4ToneFreq;
	uint8_t	ucTTGPwrIdx;
	uint8_t	ucDbdcIdx;
	uint8_t	ucXtalFreq;
	uint8_t	aucReserved[1];
};

struct TTG_ON_OFF_T {
	uint8_t	ucTTGEnable;
	uint8_t	ucDbdcIdx;
	uint8_t	ucAntIndex;
	uint8_t	aucReserved[1];
};

struct RBIST_CAP_START_T {
	uint32_t u4Trigger;
	uint32_t u4RingCapEn;
	uint32_t u4TriggerEvent;
	uint32_t u4CaptureNode;
	uint32_t u4CaptureLen;    /* Unit : IQ Sample */
	uint32_t u4CapStopCycle;  /* Unit : IQ Sample */
	uint32_t u4MacTriggerEvent;
	uint32_t u4SourceAddressLSB;
	uint32_t u4SourceAddressMSB;
	uint32_t u4BandIdx;
	uint32_t u4BW;
	uint32_t u4EnBitWidth;/* 0:32bit, 1:96bit, 2:128bit */
	uint32_t u4Architech;/* 0:on-chip, 1:on-the-fly */
	uint32_t u4PhyIdx;
	uint32_t u4EmiStartAddress;
	uint32_t u4EmiEndAddress;
	uint32_t u4EmiMsbAddress;
	uint32_t u4CapSource;
	uint32_t u4Reserved[2];
};

struct RBIST_DUMP_IQ_T {
	uint32_t u4WfNum;
	uint32_t u4IQType;
	uint32_t u4IcapCnt; /*IQ Sample Count*/
	uint32_t u4IcapDataLen;
#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
	int32_t *pIcapData;
#else
	uint8_t *pucIcapData;
#endif
};


struct RBIST_DUMP_RAW_DATA_T {
	uint32_t u4Address;
	uint32_t u4AddrOffset;
	uint32_t u4Bank;
	uint32_t u4BankSize;/* Uint:Kbytes */
	uint32_t u4WFNum;
	uint32_t u4IQType;
	uint32_t u4Reserved[6];
};

/* FuncIndex */
enum FUNC_IDX {
	RE_CALIBRATION = 0x01,
	CALIBRATION_BYPASS = 0x02,
	TX_TONE_START = 0x03,
	TX_TONE_STOP = 0x04,
	CONTINUOUS_TX_START = 0x05,
	CONTINUOUS_TX_STOP = 0x06,
	RF_AT_EXT_FUNCID_TX_TONE_RF_GAIN = 0x07,
	RF_AT_EXT_FUNCID_TX_TONE_DIGITAL_GAIN = 0x08,
	CAL_RESULT_DUMP_FLAG = 0x09,
	RDD_TEST_MODE  = 0x0A,
	SET_ICAP_CAPTURE_START = 0x0B,
	GET_ICAP_CAPTURE_STATUS = 0x0C,
	SET_ADC = 0x0D,
	SET_RX_GAIN = 0x0E,
	SET_TTG = 0x0F,
	TTG_ON_OFF = 0x10,
	GET_ICAP_RAW_DATA = 0x11,
	SET_TX_TONE_GAIN_OFFSET = 0x12,
	GET_TX_TONE_GAIN_OFFSET = 0x13,
	GET_PHY_ICS_RAW_DATA = 0x14
};

struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T {
	uint32_t u4FuncIndex;
	union {
		uint32_t u4FuncData;
		uint32_t u4CalDump;
		struct RF_TEST_CALIBRATION_T rCalParam;
		struct TX_TONE_PARAM_T rTxToneParam;
		struct CONTINUOUS_TX_PARAM_T rConTxParam;
		struct TX_TONE_POWER_GAIN_T rTxToneGainParam;
		struct RBIST_CAP_START_T rICapInfo;
		struct RBIST_DUMP_RAW_DATA_T rICapDump;
		struct CMD_RDD_ON_OFF_CTRL_T rRDDParam;
		struct SET_ADC_T rSetADC;
		struct SET_RX_GAIN_T rSetRxGain;
		struct SET_TTG_T rSetTTG;
		struct TTG_ON_OFF_T rTTGOnOff;
	} Data;
};

#define PARAM_PROTOCOL_ID_DEFAULT       0x00
#define PARAM_PROTOCOL_ID_TCP_IP        0x02
#define PARAM_PROTOCOL_ID_IPX           0x06
#define PARAM_PROTOCOL_ID_NBF           0x07
#define PARAM_PROTOCOL_ID_MAX           0x0F
#define PARAM_PROTOCOL_ID_MASK          0x0F

/* for NDIS OID_GEN_NETWORK_LAYER_ADDRESSES */
struct PARAM_NETWORK_ADDRESS_IP {
	uint16_t sin_port;
	uint32_t in_addr;
	uint8_t sin_zero[8];
};

/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
struct PARAM_NETWORK_ADDRESS_IPV6 {
	uint16_t sin_port;
	uint8_t addr[16];
};
#endif /* fos_change end */


struct PARAM_NETWORK_ADDRESS {
	uint16_t u2AddressLength;/* length in bytes of Address[] in this */
	uint16_t u2AddressType;	/* type of this address
				 * (PARAM_PROTOCOL_ID_XXX above)
				 */
	uint8_t aucAddress[];	/* actually AddressLength bytes long */
};

/* The following is used with OID_GEN_NETWORK_LAYER_ADDRESSES to set network
 * layer addresses on an interface
 */
struct PARAM_NETWORK_ADDRESS_LIST {
	uint8_t ucBssIdx;
	uint16_t u2AddressType;	/* type of this address
				 * (NDIS_PROTOCOL_ID_XXX above)
				 */
	uint32_t u4AddressCount;/* number of addresses following */

	/**
	 * Followed by one or more flexible size struct PARAM_NETWORK_ADDRESS,
	 * the original structure struct PARAM_NETWORK_ADDRESS aucAddress[];
	 * was removed since declaring a flexible array member here will cause
	 * arrays of objects containing zero-size array.
	 */
};

#if CFG_SUPPORT_MSP
/* Should by chip */
struct PARAM_SEC_CONFIG {
	u_int8_t fgWPIFlag;
	u_int8_t fgRV;
	u_int8_t fgIKV;
	u_int8_t fgRKV;

	u_int8_t fgRCID;
	u_int8_t fgRCA1;
	u_int8_t fgRCA2;
	u_int8_t fgEvenPN;

	uint8_t ucKeyID;
	uint8_t ucMUARIdx;
	uint8_t ucCipherSuit;
	uint8_t aucReserved[1];
};

struct PARAM_TX_CONFIG {
	uint8_t aucPA[6];
	u_int8_t fgSW;
	u_int8_t fgDisRxHdrTran;

	u_int8_t fgAADOM;
	uint8_t ucPFMUIdx;
	uint16_t u2PartialAID;

	u_int8_t fgTIBF;
	u_int8_t fgTEBF;
	u_int8_t fgIsHT;
	u_int8_t fgIsVHT;

	u_int8_t fgMesh;
	u_int8_t fgBAFEn;
	u_int8_t fgCFAck;
	u_int8_t fgRdgBA;

	u_int8_t fgRDG;
	u_int8_t fgIsPwrMgt;
	u_int8_t fgRTS;
	u_int8_t fgSMPS;

	u_int8_t fgTxopPS;
	u_int8_t fgDonotUpdateIPSM;
	u_int8_t fgSkipTx;
	u_int8_t fgLDPC;

	u_int8_t fgIsQoS;
	u_int8_t fgIsFromDS;
	u_int8_t fgIsToDS;
	u_int8_t fgDynBw;

	u_int8_t fgIsAMSDUCrossLG;
	u_int8_t fgCheckPER;
	u_int8_t fgIsGID63;
	u_int8_t fgIsHE;

	u_int8_t fgVhtTIBF;
	u_int8_t fgVhtTEBF;
	u_int8_t fgVhtLDPC;
	u_int8_t fgHeLDPC;
};

struct PARAM_KEY_CONFIG {
	uint8_t                 aucKey[32];
};

struct PARAM_PEER_RATE_INFO {
	uint8_t                 ucCounterMPDUFail;
	uint8_t                 ucCounterMPDUTx;
	uint8_t                 ucRateIdx;
	uint8_t                 ucReserved[1];

	uint16_t                au2RateCode[AUTO_RATE_NUM];
};

struct PARAM_PEER_BA_CONFIG {
	uint8_t			ucBaEn;
	uint8_t			ucRsv[3];
	uint32_t			u4BaWinSize;
};

struct PARAM_ANT_ID_CONFIG {
	uint8_t			ucANTIDSts0;
	uint8_t			ucANTIDSts1;
	uint8_t			ucANTIDSts2;
	uint8_t			ucANTIDSts3;
};

struct PARAM_PEER_CAP {
	struct PARAM_ANT_ID_CONFIG	rAntIDConfig;

	uint8_t			ucTxPowerOffset;
	uint8_t			ucCounterBWSelector;
	uint8_t			ucChangeBWAfterRateN;
	uint8_t			ucFrequencyCapability;
	uint8_t			ucSpatialExtensionIndex;

	u_int8_t			fgG2;
	u_int8_t			fgG4;
	u_int8_t			fgG8;
	u_int8_t			fgG16;

	uint8_t			ucMMSS;
	uint8_t			ucAmpduFactor;
	uint8_t			ucReserved[1];
};

struct PARAM_PEER_RX_COUNTER_ALL {
	uint8_t			ucRxRcpi0;
	uint8_t			ucRxRcpi1;
	uint8_t			ucRxRcpi2;
	uint8_t			ucRxRcpi3;

	uint8_t			ucRxCC0;
	uint8_t			ucRxCC1;
	uint8_t			ucRxCC2;
	uint8_t			ucRxCC3;

	u_int8_t			fgRxCCSel;
	uint8_t			ucCeRmsd;
	uint8_t			aucReserved[2];
};

struct PARAM_PEER_TX_COUNTER_ALL {
	uint16_t u2Rate1TxCnt;
	uint16_t u2Rate1FailCnt;
	uint16_t u2Rate2OkCnt;
	uint16_t u2Rate3OkCnt;
	uint16_t u2CurBwTxCnt;
	uint16_t u2CurBwFailCnt;
	uint16_t u2OtherBwTxCnt;
	uint16_t u2OtherBwFailCnt;
};

struct PARAM_HW_WLAN_INFO {
	uint32_t			u4Index;
	struct PARAM_TX_CONFIG	rWtblTxConfig;
	struct PARAM_SEC_CONFIG	rWtblSecConfig;
	struct PARAM_KEY_CONFIG	rWtblKeyConfig;
	struct PARAM_PEER_RATE_INFO	rWtblRateInfo;
	struct PARAM_PEER_BA_CONFIG  rWtblBaConfig;
	struct PARAM_PEER_CAP	rWtblPeerCap;
	struct PARAM_PEER_RX_COUNTER_ALL rWtblRxCounter;
	struct PARAM_PEER_TX_COUNTER_ALL rWtblTxCounter;
};

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
struct MIB_INFO {
	uint32_t u4Counter;
	uint64_t u8Data;
};

struct PARAM_HW_MIB_INFO {
	uint16_t u2Index;
	uint16_t u2TagCount;
	/* Declair a bitmap to indicate which mib CR are required
	 * au4TagBitmap[0]: mibIdx 0~31
	 * au4TagBitmap[1]: mibIdx 32~63
	 * au4TagBitmap[2]: mibIdx 64~95
	 * ...
	 */
	DECLARE_BITS(au4TagBitmap, MAX_UNI_CMD_MIB_NUM);
	struct MIB_INFO arMibInfo[MAX_MIB_TAG_CNT];
};
#else
struct HW_TX_AMPDU_METRICS {
	uint32_t u4TxSfCnt;
	uint32_t u4TxAckSfCnt;
	uint32_t u2TxAmpduCnt;
	uint32_t u2TxRspBaCnt;
	uint16_t u2TxEarlyStopCnt;
	uint16_t u2TxRange1AmpduCnt;
	uint16_t u2TxRange2AmpduCnt;
	uint16_t u2TxRange3AmpduCnt;
	uint16_t u2TxRange4AmpduCnt;
	uint16_t u2TxRange5AmpduCnt;
	uint16_t u2TxRange6AmpduCnt;
	uint16_t u2TxRange7AmpduCnt;
	uint16_t u2TxRange8AmpduCnt;
	uint16_t u2TxRange9AmpduCnt;
#if (CFG_SUPPORT_802_11AX == 1)
	uint16_t u2TxRange10AmpduCnt;
	uint16_t u2TxRange11AmpduCnt;
	uint16_t u2TxRange12AmpduCnt;
	uint16_t u2TxRange13AmpduCnt;
	uint16_t u2TxRange14AmpduCnt;
	uint16_t u2TxRange15AmpduCnt;
	uint16_t u2TxRange16AmpduCnt;
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	/* TODO */
#endif
};

struct HW_MIB_COUNTER {
	uint32_t u4RxFcsErrCnt;
	uint32_t u4RxFifoFullCnt;
	uint32_t u4RxMpduCnt;
	uint32_t u4RxAMPDUCnt;
	uint32_t u4RxTotalByte;
	uint32_t u4RxValidAMPDUSF;
	uint32_t u4RxValidByte;
	uint32_t u4ChannelIdleCnt;
	uint32_t u4RxVectorDropCnt;
	uint32_t u4DelimiterFailedCnt;
	uint32_t u4RxVectorMismatchCnt;
	uint32_t u4MdrdyCnt;
	uint32_t u4CCKMdrdyCnt;
	uint32_t u4OFDMLGMixMdrdy;
	uint32_t u4OFDMGreenMdrdy;
	uint32_t u4PFDropCnt;
	uint32_t u4RxLenMismatchCnt;
	uint32_t u4PCcaTime;
	uint32_t u4SCcaTime;
	uint32_t u4CcaNavTx;
	uint32_t u4PEDTime;
	uint32_t u4BeaconTxCnt;
	uint32_t au4BaMissedCnt[BSSID_NUM];
	uint32_t au4RtsTxCnt[BSSID_NUM];
	uint32_t au4FrameRetryCnt[BSSID_NUM];
	uint32_t au4FrameRetry2Cnt[BSSID_NUM];
	uint32_t au4RtsRetryCnt[BSSID_NUM];
	uint32_t au4AckFailedCnt[BSSID_NUM];
};

struct HW_MIB2_COUNTER {
	uint32_t u4Tx40MHzCnt;
	uint32_t u4Tx80MHzCnt;
	uint32_t u4Tx160MHzCnt;
};

struct PARAM_HW_MIB_INFO {
	uint32_t			u4Index;
	struct HW_MIB_COUNTER	rHwMibCnt;
	struct HW_MIB2_COUNTER	rHwMib2Cnt;
	struct HW_TX_AMPDU_METRICS	rHwTxAmpduMts;
};
#endif
#endif

struct PARAM_SET_TX_AGG_LIMIT_INFO {
	uint16_t u2TxAmpduNum;
	uint8_t ucSet;
};

struct PARAM_SET_TX_AMSDU_NUM_LIMIT_INFO {
	uint8_t ucTxAmsduNum;
	uint8_t ucSet;
	uint8_t aucReserved[2];
};

#if CFG_WIFI_TXPWR_TBL_DUMP
struct PARAM_CMD_GET_TXPWR_TBL {
	uint8_t ucDbdcIdx;
	uint8_t ucCenterCh;
	struct POWER_LIMIT tx_pwr_tbl[TXPWR_TBL_NUM];
};

enum ENUM_TXPWR_TYPE {
	DSSS = 0,
	OFDM_24G,
	OFDM_5G,
	HT20,
	HT40,
	VHT20,
	VHT40,
	VHT80,
	VHT160,
#if (CFG_WIFI_TXPWR_TBL_DUMP_HE == 1)
	HE26,
	HE52,
	HE106,
	HE242,
	HE484,
	HE996,
	HE996X2,
#endif
	TXPWR_TYPE_NUM,
};

enum ENUM_STREAM_MODE {
	STREAM_SISO,
	STREAM_CDD,
	STREAM_MIMO,
	STREAM_NUM
};

struct txpwr_table_entry {
	char mcs[STREAM_NUM][8];
	unsigned int idx;
};

struct txpwr_table {
	char phy_mode[8];
	struct txpwr_table_entry *tables;
	int n_tables;
};
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */

/*--------------------------------------------------------------*/
/*! \brief For Fixed Rate Configuration (Registry)              */
/*--------------------------------------------------------------*/
enum ENUM_REGISTRY_FIXED_RATE {
	FIXED_RATE_NONE,
	FIXED_RATE_1M,
	FIXED_RATE_2M,
	FIXED_RATE_5_5M,
	FIXED_RATE_11M,
	FIXED_RATE_6M,
	FIXED_RATE_9M,
	FIXED_RATE_12M,
	FIXED_RATE_18M,
	FIXED_RATE_24M,
	FIXED_RATE_36M,
	FIXED_RATE_48M,
	FIXED_RATE_54M,
	FIXED_RATE_MCS0_20M_800NS,
	FIXED_RATE_MCS1_20M_800NS,
	FIXED_RATE_MCS2_20M_800NS,
	FIXED_RATE_MCS3_20M_800NS,
	FIXED_RATE_MCS4_20M_800NS,
	FIXED_RATE_MCS5_20M_800NS,
	FIXED_RATE_MCS6_20M_800NS,
	FIXED_RATE_MCS7_20M_800NS,
	FIXED_RATE_MCS0_20M_400NS,
	FIXED_RATE_MCS1_20M_400NS,
	FIXED_RATE_MCS2_20M_400NS,
	FIXED_RATE_MCS3_20M_400NS,
	FIXED_RATE_MCS4_20M_400NS,
	FIXED_RATE_MCS5_20M_400NS,
	FIXED_RATE_MCS6_20M_400NS,
	FIXED_RATE_MCS7_20M_400NS,
	FIXED_RATE_MCS0_40M_800NS,
	FIXED_RATE_MCS1_40M_800NS,
	FIXED_RATE_MCS2_40M_800NS,
	FIXED_RATE_MCS3_40M_800NS,
	FIXED_RATE_MCS4_40M_800NS,
	FIXED_RATE_MCS5_40M_800NS,
	FIXED_RATE_MCS6_40M_800NS,
	FIXED_RATE_MCS7_40M_800NS,
	FIXED_RATE_MCS32_800NS,
	FIXED_RATE_MCS0_40M_400NS,
	FIXED_RATE_MCS1_40M_400NS,
	FIXED_RATE_MCS2_40M_400NS,
	FIXED_RATE_MCS3_40M_400NS,
	FIXED_RATE_MCS4_40M_400NS,
	FIXED_RATE_MCS5_40M_400NS,
	FIXED_RATE_MCS6_40M_400NS,
	FIXED_RATE_MCS7_40M_400NS,
	FIXED_RATE_MCS32_400NS,
	FIXED_RATE_NUM
};

enum ENUM_BT_CMD {
	BT_CMD_PROFILE = 0,
	BT_CMD_UPDATE,
	BT_CMD_NUM
};

enum ENUM_BT_PROFILE {
	BT_PROFILE_CUSTOM = 0,
	BT_PROFILE_SCO,
	BT_PROFILE_ACL,
	BT_PROFILE_MIXED,
	BT_PROFILE_NO_CONNECTION,
	BT_PROFILE_NUM
};

struct PTA_PROFILE {
	enum ENUM_BT_PROFILE eBtProfile;
	union {
		uint8_t aucBTPParams[BT_PROFILE_PARAM_LEN];
		/*  0: sco reserved slot time,
		 *  1: sco idle slot time,
		 *  2: acl throughput,
		 *  3: bt tx power,
		 *  4: bt rssi
		 *  5: VoIP interval
		 *  6: BIT(0) Use this field, BIT(1) 0 apply single/ 1 dual PTA
		 *     setting.
		 */
		uint32_t au4Btcr[4];
	} u;
};

struct PTA_IPC {
	uint8_t ucCmd;
	uint8_t ucLen;
	union {
		struct PTA_PROFILE rProfile;
		uint8_t aucBTPParams[BT_PROFILE_PARAM_LEN];
	} u;
};

/*--------------------------------------------------------------*/
/*! \brief CFG80211 Scan Request Container                      */
/*--------------------------------------------------------------*/
struct PARAM_SCAN_REQUEST_EXT {
	struct PARAM_SSID rSsid;
	uint32_t u4IELength;
	uint8_t *pucIE;
	uint8_t ucBssIndex;
};

struct PARAM_SCAN_REQUEST_ADV {
	uint8_t fgNeedMloScan;
	uint8_t fgIsRrm;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint16_t u2ChannelDwellTime;
	uint16_t u2ChannelMinDwellTime;
	uint8_t ucShortSsidNum;
	uint32_t u4SsidNum;
	struct PARAM_SSID rSsid[CFG_SCAN_SSID_MAX_NUM];
	uint8_t ucScanType;
	uint8_t ucSSIDType;
	uint32_t u4IELength;
	uint8_t aucIEBuf[MAX_IE_LENGTH];
	uint32_t u4ChannelNum;
	struct RF_CHANNEL_INFO arChannel[MAXIMUM_OPERATION_CHANNEL_LIST];
	uint8_t ucScnFuncMask;
	uint32_t u4ScnFuncMaskExtend;
	uint8_t aucRandomMac[MAC_ADDR_LEN];
	uint8_t ucBssIndex;
	uint32_t u4Flags;
	uint8_t aucExtBssid[CFG_SCAN_OOB_MAX_NUM][MAC_ADDR_LEN];
	/* For OOB discovery*/
	uint8_t ucBssidMatchCh[CFG_SCAN_OOB_MAX_NUM];
	uint8_t ucBssidMatchSsidInd[CFG_SCAN_OOB_MAX_NUM];
	u_int8_t fgOobRnrParseEn;
};

/*--------------------------------------------------------------*/
/*! \brief CFG80211 Scheduled Scan Request Container            */
/*--------------------------------------------------------------*/
#if CFG_SUPPORT_SCHED_SCAN
struct PARAM_SCHED_SCAN_REQUEST {
	uint32_t u4SsidNum;         /* passed in the probe_reqs */
	struct PARAM_SSID arSsid[CFG_SCAN_HIDDEN_SSID_MAX_NUM];
	uint32_t u4MatchSsidNum;   /* matched for a scan request */
	struct PARAM_SSID arMatchSsid[CFG_SCAN_SSID_MATCH_MAX_NUM];
	int32_t ai4RssiThold[CFG_SCAN_SSID_MATCH_MAX_NUM];
	int32_t i4MinRssiThold;
	uint8_t ucScnFuncMask;
	uint8_t aucRandomMac[MAC_ADDR_LEN];
	uint8_t aucRandomMacMask[MAC_ADDR_LEN];
	uint32_t u4IELength;
	uint8_t *pucIE;
	uint16_t u2ScanInterval;	/* in second */
	uint8_t ucChnlNum;
	struct CHANNEL_INFO aucChannel[MAXIMUM_OPERATION_CHANNEL_LIST];
	uint8_t ucBssIndex;
};
#endif /* CFG_SUPPORT_SCHED_SCAN */

#if CFG_SUPPORT_PASSPOINT
struct PARAM_HS20_SET_BSSID_POOL {
	u_int8_t fgIsEnable;
	uint8_t ucNumBssidPool;
	uint8_t arBSSID[8][PARAM_MAC_ADDR_LEN];
	uint8_t ucBssIndex;
};

#endif /* CFG_SUPPORT_PASSPOINT */

/*--------------------------------------------------------------*/
/*! \brief MTK Auto Channel Selection related Container         */
/*--------------------------------------------------------------*/
enum ENUM_SAFE_CH_MASK {
	ENUM_SAFE_CH_MASK_BAND_2G4 = 0,
	ENUM_SAFE_CH_MASK_BAND_5G_0 = 1,
	ENUM_SAFE_CH_MASK_BAND_5G_1 = 2,
	ENUM_SAFE_CH_MASK_BAND_6G = 3,
	ENUM_SAFE_CH_MASK_MAX_NUM = 4,
};

struct LTE_SAFE_CHN_INFO {
	/* ENUM_SAFE_CH_MASK_MAX_NUM */
	uint32_t au4SafeChannelBitmask[ENUM_SAFE_CH_MASK_MAX_NUM];
};

struct PARAM_CHN_LOAD_INFO {
	/* Per-CHN Load */
	enum ENUM_BAND eBand;
	uint8_t ucChannel;
	uint16_t u2APNum;
	uint32_t u4Dirtiness;
	uint8_t ucReserved;
};

struct PARAM_CHN_RANK_INFO {
	enum ENUM_BAND eBand;
	uint8_t ucChannel;
	uint32_t u4Dirtiness;
	uint8_t ucReserved;
};

struct PARAM_GET_CHN_INFO {
	struct LTE_SAFE_CHN_INFO rLteSafeChnList;
	struct PARAM_CHN_LOAD_INFO rEachChnLoad[MAX_CHN_NUM];
	struct PARAM_CHN_RANK_INFO rChnRankList[MAX_CHN_NUM];
	uint8_t aucReserved[3];
};

struct PARAM_PREFER_CHN_INFO {
	uint8_t ucChannel;
	uint8_t aucReserved[3];
	uint32_t u4Dirtiness;
};

struct CNM_STATUS {
	uint8_t              fgDbDcModeEn;
	uint8_t              ucChNumB0;
	uint8_t              ucChNumB1;
	uint8_t              usReserved;
};

struct CNM_CH_LIST {
	uint8_t              ucChNum[4];
};

struct EXT_CMD_SER_T {
	uint8_t ucAction;
	uint8_t ucSerSet;
	uint8_t ucDbdcIdx;
	uint8_t aucReserve[1];
};

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
struct HAL_FRAME_POWER_SET_T {
	int8_t icFramePowerDbm;
};

struct FRAME_POWER_CONFIG_INFO_T {
	struct HAL_FRAME_POWER_SET_T
		aicFramePowerConfig[TXPOWER_RATE_NUM][ENUM_BAND_NUM];
};

struct PARAM_TXPOWER_ALL_RATE_POWER_INFO_T {
	uint8_t ucTxPowerCategory;
	uint8_t ucBandIdx;
	uint8_t ucChBand;
	uint8_t u1Format;/*0:Legacy,1:HE format*/

	/* Rate power info */
	struct FRAME_POWER_CONFIG_INFO_T rRatePowerInfo;

	/* tx Power Max/Min Limit info */
	int8_t icPwrMaxBnd;
	int8_t icPwrMinBnd;
	int8_t ucReserved2;
};
#endif


struct PARAM_TXPOWER_BY_RATE_SET_T {
	uint8_t u1PhyMode;
	uint8_t u1TxRate;
	uint8_t u1BW;
	int8_t  i1TxPower;
};

#if CFG_SUPPORT_OSHARE
/* OSHARE Mode */
#define MAX_OSHARE_MODE_LENGTH		64
#define OSHARE_MODE_MAGIC_CODE		0x18
#define OSHARE_MODE_CMD_V1		0x1

struct OSHARE_MODE_T {
	uint8_t   cmdVersion; /* CMD version = OSHARE_MODE_CMD_V1 */
	uint8_t   cmdType; /* 1-set  0-query */
	uint8_t   magicCode; /* It's like CRC, OSHARE_MODE_MAGIC_CODE */
	uint8_t   cmdBufferLen; /* buffer length <= 64 */
	uint8_t   buffer[MAX_OSHARE_MODE_LENGTH];
};

struct OSHARE_MODE_SETTING_V1_T {
	uint8_t   osharemode; /* 0: disable, 1:Enable */
	uint8_t   reserved[7];
};
#endif

struct PARAM_WIFI_LOG_LEVEL_UI {
	uint32_t u4Version;
	uint32_t u4Module;
	uint32_t u4Enable;
};

struct PARAM_WIFI_LOG_LEVEL {
	uint32_t u4Version;
	uint32_t u4Module;
	uint32_t u4Level;
};

struct PARAM_GET_WIFI_TYPE {
	void *prNetDev;
	uint8_t arWifiTypeName[8];
};

enum ENUM_WIFI_LOG_LEVEL_VERSION_T {
	ENUM_WIFI_LOG_LEVEL_VERSION_V1 = 1,
	ENUM_WIFI_LOG_LEVEL_VERSION_NUM
};

enum ENUM_WIFI_LOG_LEVEL_T {
	ENUM_WIFI_LOG_LEVEL_DEFAULT = 0,
	ENUM_WIFI_LOG_LEVEL_MORE,
	ENUM_WIFI_LOG_LEVEL_EXTREME,
	ENUM_WIFI_LOG_LEVEL_UV,
	ENUM_WIFI_LOG_LEVEL_NUM
};

enum ENUM_WIFI_LOG_MODULE_T {
	ENUM_WIFI_LOG_MODULE_DRIVER = 0,
	ENUM_WIFI_LOG_MODULE_FW,
	ENUM_WIFI_LOG_MODULE_NUM,
};

enum ENUM_WIFI_LOG_LEVEL_SUPPORT_T {
	ENUM_WIFI_LOG_LEVEL_SUPPORT_DISABLE = 0,
	ENUM_WIFI_LOG_LEVEL_SUPPORT_ENABLE,
	ENUM_WIFI_LOG_LEVEL_SUPPORT_NUM
};

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
struct PARAM_GET_LINK_QUALITY_INFO {
	uint8_t ucBssIdx;
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo;
};
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if CFG_SUPPORT_MBO
struct PARAM_BSS_DISALLOWED_LIST {
	uint32_t u4NumBssDisallowed;
	/* MAX_FW_ROAMING_BLOCKLIST_SIZE */
	uint8_t aucList[MAC_ADDR_LEN * 16];
};
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
struct PARAM_TX_MCS_INFO {
	uint8_t   ucStaIndex;
	uint16_t  au2TxRateCode[MCS_INFO_SAMPLE_CNT];
	uint8_t   aucTxBw[MCS_INFO_SAMPLE_CNT];
	uint8_t   aucTxSgi[MCS_INFO_SAMPLE_CNT];
	uint8_t   aucTxLdpc[MCS_INFO_SAMPLE_CNT];
	uint8_t   aucTxRatePer[MCS_INFO_SAMPLE_CNT];
};
#endif

#if CFG_SUPPORT_LLW_SCAN
struct PARAM_LLW_SCAN {
	uint8_t ucDfsChDwellTimeMs;
	uint8_t ucNonDfsChDwellTimeMs;
	uint16_t u2OpChStayTimeMs;
	uint16_t u2OpChAwayTimeMs;
	uint8_t ucPerScanChannelCnt;
};
#endif

#if CFG_SUPPORT_MANIPULATE_TID
struct PARAM_MANIUPLATE_TID {
	/* 0:default; others:enable */
	uint8_t ucMode;
	uint8_t ucTid;
	uint32_t u4Uid;
};
#endif

#define MAX_AX_BLOCKLIST_ENTRIES 16
struct PARAM_AX_BLOCKLIST {
	uint8_t ucType;
	uint8_t ucCount;
	uint8_t aucList[MAX_AX_BLOCKLIST_ENTRIES][MAC_ADDR_LEN];
};

struct PARAM_CUS_BLOCKLIST {
	uint8_t ucType;

	struct PARAM_SSID rSSID;
	uint8_t aucBSSID[MAC_ADDR_LEN];
	uint32_t u4Frequency;
	enum ENUM_BAND eBand;

	uint8_t ucLimitReason;
	uint8_t ucLimitType;
	uint32_t u4LimitTimeout;
};

struct PARAM_STBC_MRC {
	uint8_t ucType; /* 0: STBC, 1: MRC */
	uint8_t ucBssIndex;
	uint8_t fgEnable;
};

enum ENUM_AX_BLOCKLIST_TYPE {
	BLOCKLIST_AX_TO_AC = 0,
	BLOCKLIST_DIS_HE_HTC = 1,
	BLOCKLIST_NUM
};

struct PARAM_MULTICAST_LIST {
	uint8_t ucBssIdx;
	uint8_t ucAddrNum;
	uint8_t aucMcAddrList[MAX_NUM_GROUP_ADDR][MAC_ADDR_LEN];
	u_int8_t fgIsOid;
};

#if (CFG_SUPPORT_PKT_OFLD == 1)
struct PARAM_OFLD_INFO {
	/*restrict buffer size to 1500 bytes*/
	/*because FW WFDMA MAX buf size is 1600 Byte*/
	uint8_t ucType;
	uint8_t ucOp;
	uint8_t ucFragNum;
	uint8_t ucFragSeq;
	uint32_t u4TotalLen;
	uint32_t u4BufLen;
	uint8_t aucBuf[PKT_OFLD_BUF_SIZE];
};

struct PARAM_APF_INFO {
	uint8_t ucBssIdx;
	uint8_t ucAisIdx;
	uint32_t u4ApfVer;
	uint32_t u4ApfBufSize;
};

#endif /* CFG_SUPPORT_PKT_OFLD */

#define COEX_CTRL_BUF_LEN 460
#define COEX_INFO_LEN 115

/* CMD_COEX_CTRL & EVENT_COEX_CTRL */
/************************************************/
/*  UINT_32 u4SubCmd : Coex Ctrl Sub Command    */
/*  UINT_8 aucBuffer : Reserve for Sub Command  */
/*                        Data Structure        */
/************************************************/
struct PARAM_COEX_HANDLER {
	uint32_t u4SubCmd;
	uint8_t  aucBuffer[COEX_CTRL_BUF_LEN];
};

#if (CFG_WIFI_ISO_DETECT == 1)
/* Isolation Structure */
/************************************************/
/*  UINT_32 u4IsoPath : BITS[7:0]:WF Path (WF0/WF1)*/
/*                      BITS[15:8]:BT Path (BT0/BT1)*/
/*  UINT_32 u4Channel : WF Channel*/
/*  UINT_32 u4Isolation  : Isolation value     */
/************************************************/
struct PARAM_COEX_ISO_DETECT {
	uint32_t u4IsoPath;
	uint32_t u4Channel;
	uint32_t u4Isolation;
};
#endif

/* Coex Info Structure */
/************************************************/
/*  char   cCoexInfo[];                        */
/************************************************/
struct PARAM_COEX_GET_INFO {
	uint32_t   u4CoexInfo[COEX_INFO_LEN];
};

#if (CFG_WIFI_GET_DPD_CACHE == 1)
struct PARAM_GET_DPD_CACHE {
	uint8_t		ucDpdCacheNum;
	uint8_t		ucReserved[3];
	uint32_t	u4DpdCacheCh[PER_CH_CAL_CACHE_NUM];
	uint8_t		ucDpdCachePath[PER_CH_CAL_CACHE_NUM];
};
#endif

#if CFG_AP_80211KVR_INTERFACE
struct T_MULTI_AP_BSS_METRICS_RESP {
	uint32_t uIfIndex;
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t u8Channel;
	uint16_t u16AssocStaNum;
	uint8_t u8ChanUtil;
	int32_t iChanNoise;
};

struct T_MULTI_AP_STA_ASSOC_METRICS_RESP {
	uint32_t uIfIndex;
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t mStaMac[MAC_ADDR_LEN];
	uint64_t uBytesSent;
	uint64_t uBytesRecv;
	uint64_t uPktsSent;
	uint64_t uPktsRecv;
	uint64_t uPktsTxError;
	uint32_t uPktsRxError;
	uint32_t uRetransCnt;
	int32_t iRssi;
	uint32_t uPhyTxRate;
	uint32_t uPhyRxRate;
	uint32_t uAssocRate;
	uint32_t uDeltaTime;
};

 /* TODO: check this value in user-space */
#define STA_CAP_LEN_MAX 512
__KAL_ATTRIB_PACKED_FRONT__
struct T_MULTI_AP_STA_EVENT_NOTIFY {
	uint8_t mStaMac[MAC_ADDR_LEN];
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t	u8Status;
	uint32_t uCapLen;
	uint8_t u8Cap[STA_CAP_LEN_MAX];
} __KAL_ATTRIB_PACKED__;

#define SAP_HTCAP_TXSTREAMNUM_OFFSET 0
#define SAP_HTCAP_RXSTREAMNUM_OFFSET 2
#define SAP_HTCAP_SGIFOR20M_OFFSET   4
#define SAP_HTCAP_SGIFOR40M_OFFSET   5
#define SAP_HTCAP_HTFOR40M_OFFSET    6
#define SAP_HTCAP_RESERVED       BIT(7)

#define SAP_VHTCAP_TXSTREAMNUM_OFFSET   0
#define SAP_VHTCAP_RXSTREAMNUM_OFFSET   3
#define SAP_VHTCAP_SGIFOR80M_OFFSET     6
#define SAP_VHTCAP_SGIFOR160M_OFFSET    7
#define SAP_VHTCAP_VHTFORDUAL80M_OFFSET 8
#define SAP_VHTCAP_VHTFOR160M_OFFSET    9
#define SAP_VHTCAP_SUBEAMFORMER_OFFSET  10
#define SAP_VHTCAP_MUBEAMFORMER_OFFSET  11
#define SAP_VHTCAP_RESERVED     BITS(12, 15)

#define SAP_HECAP_TXSTREAMNUM_OFFSET   0
#define SAP_HECAP_RXSTREAMNUM_OFFSET   3
#define SAP_HECAP_HEFORDUAL80M_OFFSET  6
#define SAP_HECAP_HEFOR160M_OFFSET     7
#define SAP_HECAP_SUBEAMFORMER_OFFSET  8
#define SAP_HECAP_MUBEAMFORMER_OFFSET  9
#define SAP_HECAP_ULMUMIMO_OFFSET      10
#define SAP_HECAP_ULMUMIMOOFDMA_OFFSET 11
#define SAP_HECAP_DLMUMIMOOFDMA_OFFSET 12
#define SAP_HECAP_ULOFDMA_OFFSET       13
#define SAP_HECAP_DLOFDMA_OFFSET       14
#define SAP_HECAP_RESERVED         BIT(15)

#define SAP_UNASSOC_METRICS_STA_MAX 16

__KAL_ATTRIB_PACKED_FRONT__
struct T_MULTI_AP_BSS_STATUS_REPORT {
	uint32_t uIfIndex;
	uint8_t mBssid[MAC_ADDR_LEN];
	uint32_t uStatus;
	uint8_t u8Channel;
	uint8_t u8OperClass;
	uint8_t u8Txpower;
	uint32_t uBand;
	uint8_t uHtCap;
	uint16_t u16VhtTxMcs;
	uint16_t u16VhtRxMcs;
	uint16_t u16VhtCap;
	uint8_t u8HeMcsNum;
	uint8_t u8HeMcs[16];
	uint16_t u16HeCap;
} __KAL_ATTRIB_PACKED__;

struct T_MULTI_AP_STA_UNASSOC_METRICS {
	uint8_t mStaMac[MAC_ADDR_LEN];
	uint32_t uTime;
	int32_t iRssi;
	uint8_t u8Channel;
};

struct T_MULTI_AP_STA_UNASSOC_METRICS_RESP {
	uint32_t uIfIndex;
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t u8StaNum;
	struct T_MULTI_AP_STA_UNASSOC_METRICS
		tMetrics[SAP_UNASSOC_METRICS_STA_MAX];
};
#endif /* CFG_AP_80211KVR_INTERFACE */

#if CFG_AP_80211K_SUPPORT
#define ELEM_LEN_MAX 1024
struct T_MULTI_AP_BEACON_METRICS_RESP {
	uint8_t mStaMac[MAC_ADDR_LEN];
	uint8_t u8ElemNum;
	uint32_t uElemLen;
	uint8_t uElem[ELEM_LEN_MAX];
};

struct PARAM_CUSTOM_BCN_REP_REQ_STRUCT {
	uint8_t aucPeerMac[MAC_ADDR_LEN];
	uint16_t u2Repetition;
	uint16_t u2MeasureDuration;
	uint8_t ucOperClass;
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint8_t aucSsid[PARAM_MAX_LEN_SSID + 1];
	uint8_t ucChannel;
	uint16_t u2RandomInterval;
	uint8_t ucMeasurementMode;
	uint8_t ucReportCondition;
	uint8_t ucReportReference;
	uint8_t ucReportingDetail;
	uint8_t ucNumberOfRequest;
	uint8_t ucRequestElemList[ELEM_LEN_MAX];
	uint8_t ucNumberOfAPChanReport;
	uint8_t ucChanList[MAX_CHN_NUM];
};

struct PARAM_CUSTOM_CU_REP_REQ_STRUCT {
	uint8_t ucBssIdx;
	uint8_t ucUnitNum;
	uint8_t aucPeerMac[MAC_ADDR_LEN];
	uint16_t u2Repetition;
	uint16_t u2MeasureDuration;
	enum ENUM_BAND eBand;
	uint16_t u2RandomInterval;
	uint8_t ucMeasurementMode;
	uint8_t ucChanList[MAX_CHN_NUM];
};

#endif /* CFG_AP_80211K_SUPPORT */

#if CFG_AP_80211V_SUPPORT
struct T_MAC_CHAN {
	uint8_t mMac[MAC_ADDR_LEN];
	uint32_t u4BSSIDInfo;
	uint8_t ucOperClass;
	uint8_t ucChannel;
	uint8_t ucPhyType;
	uint8_t ucPreference;
};

#define BCN_REQ_PARAM_REQUESTMODE_OFFSET 0
#define BCN_REQ_PARAM_DISIMMINENT_OFFSET 1
#define BCN_REQ_PARAM_ABRIDGED_OFFSET    2
#define BCN_REQ_PARAM_RESERVED     BITS(3, 7)

#define STEER_STA_NUM_MAX 32
#define DEST_BSSID_NUM_MAX 32

struct T_MULTI_AP_STA_STEERING_REQ {
	uint32_t uIfindex;
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t tReqParam;
	uint16_t u16OpptyWin;
	uint16_t u16DisassocTimer;
	uint8_t u8StaNum;
	uint8_t tStaList[MAC_ADDR_LEN][STEER_STA_NUM_MAX];
	uint8_t u8BssidNum;
	struct T_MAC_CHAN tBssidList[DEST_BSSID_NUM_MAX];
};

struct T_MULTI_AP_STA_STEERING_REPORT {
	uint8_t mStaMac[MAC_ADDR_LEN];
	uint8_t mBssid[MAC_ADDR_LEN];
	uint8_t u8Status;
	uint8_t mDestBssid[MAC_ADDR_LEN];
};

struct PARAM_CUSTOM_BTM_REQ_STRUCT {
	uint8_t aucPeerMac[MAC_ADDR_LEN];
	uint8_t ucEssImm;
	uint16_t u2DisassocTimer;
	uint8_t ucAbridged;
	uint8_t ucValidityInterval;
	uint8_t ucTargetBSSIDCnt;
	uint8_t aucSessionUrl[256];
	struct T_MAC_CHAN ucTargetBSSIDList[DEST_BSSID_NUM_MAX];
};
#endif /* CFG_AP_80211V_SUPPORT */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLO_AGC_DISP_PARAM_TX {
	uint8_t u1AgcStateTx;
	uint8_t au1DispPolTx[MAX_MLO_MGMT_SUPPORT_AC_NUM];
	uint8_t u1DispRatioTx;
	uint8_t u1DispOrderTx;
	uint16_t u2DispMgfTx;
};

struct MLO_AGC_DISP_PARAM_TRIG {
	uint8_t u1AgcStateTrig;
	uint8_t au1DispPolTrig[MAX_MLO_MGMT_SUPPORT_AC_NUM];
	uint8_t u1DispRatioTrig;
	uint8_t u1DispMuLenTrig;
	uint16_t u2DispMgfTrig;
};

struct MLO_OVLP_RPT_CNT {
	uint16_t u2Corr0;
	uint16_t u2Corr1;
	uint16_t u2InCorr0;
	uint16_t u2InCorr1;
};

struct MLD_RECORD_LINK {
	u_int8_t fgActive;
	u_int8_t fgSuspend;
	uint8_t u1ParentMldRecIdx; /* Parent MLD Record Index */
	uint8_t u1Band;
	uint16_t u2WlanIdx;
	struct MLO_AGC_DISP_PARAM_TX rAgcDispParamTx;
	struct MLO_AGC_DISP_PARAM_TRIG rAgcDispParamTrig;
	struct MLO_OVLP_RPT_CNT arOvlpRptCntTx[MAX_MLO_MGMT_SUPPORT_AC_NUM];
	struct MLO_OVLP_RPT_CNT arOvlpRptCntTrig[MAX_MLO_MGMT_SUPPORT_AC_NUM];
};

/* UNI_CMD_MLD_REC(Tag=0x02) */
struct PARAM_MLD_REC {
	uint8_t u1MldRecState;
	uint8_t u1MldRecIdx;
	uint16_t u2MldIdx;
	uint16_t u2PrimaryMldId;
	uint16_t u2SecondMldId;
	u_int8_t fgAllStrLinks;
	uint8_t u1StrBmp;
	uint8_t u1EmlsrBmp;
	uint8_t u1ActiveLinkNum;
	uint8_t u1ActiveLinkBmp;
	u_int8_t fgAgcAggressiveMode[2]; /* Trig:0 Tx:1 */
	struct MLD_RECORD_LINK arMldRecLink[MLD_LINK_MAX];
};

struct PARAM_EML_DEBUG_INFO {
	uint32_t u4TimeoutMs;
	uint16_t u2StaRecMldIdx;
	uint8_t ucEmlsrBitMap;
	/* protocol link index to band index */
	uint8_t auMldLinkIdx[3];
	/* same with ENUM_EML_STATE_T */
	uint8_t ucCurrentState;
	/* AP MLD responded the EML notification frame */
	uint8_t ucEmlNegotiated;
};

#if (CFG_SUPPORT_MLC == 1)

struct PARAM_MLC_REQ {
	enum ENUM_MLC_MODE eMlcMode;
	uint32_t u4Data1;
	uint32_t u4Data2;
};

enum ENUM_MLC_STATUS {
	MLC_STATUS_SUCCESS = 0,
	MLC_STATUS_ERROR = 1,
	MLC_STATUS_BUSY = 2,
};

struct PARAM_MLC_LINK_INFO {
	uint8_t ucLinkId;
	uint8_t ucLinkState;
	uint32_t u4FreqInMHz;
};

struct PARAM_MLC_QUERY_LINK_STATE {
	enum ENUM_MLC_MODE eMlcMode;
	uint8_t ucLinkNum;
	struct PARAM_MLC_LINK_INFO arLinkInfo[MLD_LINK_MAX];
};

struct PARAM_MLC_QUERY_INFO {
	uint32_t u4AvailablePlanBmap;
	uint8_t ucCurrentPlan;
	uint8_t ucCurrentUser;
};

union PARAM_MLC {
	struct PARAM_MLC_REQ rReq;
	struct PARAM_MLC_QUERY_LINK_STATE rQueryLinkState;
	struct PARAM_MLC_QUERY_INFO rQueryInfo;
};

#endif /* CFG_SUPPORT_MLC */

#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
struct PARAM_QUERY_ML_CHNL_COND {
	uint32_t u4Status;
};
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */
#endif /* CFG_SUPPORT_802_11BE_MLO */

#if (CFG_SUPPORT_ML_RECONFIG == 1)
struct PARAM_LR_REQUEST {
	uint16_t u2DelLinkIdBitmap;
	uint16_t u2AddLinkIdBitmap;
};
#endif /* CFG_SUPPORT_ML_RECONFIG */

/* This structure is a replication of struct EXT_EVENT_SER_T.
 * Thus, we are able to simply do memory copy from EXT_EVENT_SER_T to
 * PARAM_SER_INFO_T when receiving EXT_EVENT_ID_SER.
 */
struct PARAM_SER_INFO_T {
/* Represents the current supporting EXT_EVENT_ID_SER version in driver.
 * Each time we extend this structure in the future, we shall increment
 * EXT_EVENT_SER_VER.
 */
#ifdef EXT_EVENT_SER_VER
#undef EXT_EVENT_SER_VER
#endif
#define EXT_EVENT_SER_VER        0

/* Don't change these constants.
 * We define these definitions for readability, not for flexibility.
 * For example, if RAM_BAND_NUM changes from 2 to 3 in future project,
 * then we shall add new structure members (ex: uint8_t ucSerL2RecoverCntBand2;)
 * and increment EXT_EVENT_SER_VER for compatibility, but shall not simply
 * change EXT_EVENT_SER_RAM_BAND_NUM from 2 to 3.
 */
#ifdef EXT_EVENT_SER_RAM_BAND_NUM
#undef EXT_EVENT_SER_RAM_BAND_NUM
#endif
#define EXT_EVENT_SER_RAM_BAND_NUM        2    /* RAM_BAND_NUM */
#ifdef EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER
#undef EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER
#endif
/* MAX_HW_ERROR_INT_NUMBER */
#define EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER        32

	/* DWORD_0 - Common Part */
	/* if the structure size is changed, the ucEvtVer shall be increased. */
	uint8_t  ucEvtVer;
	uint8_t  aucPadding0[1];
	/* event size including common part and body. */
	uint16_t u2EvtLen;

	/* ucEvtVer = 0 definition BEGIN */

	/* DWORD_1 - Body */
	uint8_t  ucEnableSER;
	uint8_t  ucSerL1RecoverCnt;
	uint8_t  ucSerL2RecoverCnt;
	uint8_t  ucSerL3BfRecoverCnt;

	/* DWORD_2 */
	uint8_t  ucSerL3RxAbortCnt[EXT_EVENT_SER_RAM_BAND_NUM];
	uint8_t  ucSerL3TxAbortCnt[EXT_EVENT_SER_RAM_BAND_NUM];

	/* DWORD_3 */
	uint8_t  ucSerL3TxDisableCnt[EXT_EVENT_SER_RAM_BAND_NUM];
	uint8_t  ucSerL4RecoverCnt[EXT_EVENT_SER_RAM_BAND_NUM];

	/* DWORD_4 ~ DWORD_35 */
	uint16_t u2LMACError6Cnt[EXT_EVENT_SER_RAM_BAND_NUM]
				[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_36 ~ DWORD_67 */
	uint16_t u2LMACError7Cnt[EXT_EVENT_SER_RAM_BAND_NUM]
				[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_68 ~ DWORD_83 */
	uint16_t u2PSEErrorCnt[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_84 ~ DWORD_99 */
	uint16_t u2PSEError1Cnt[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_100 ~ DWORD_115 */
	uint16_t u2PLEErrorCnt[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_116 ~ DWORD_131 */
	uint16_t u2PLEError1Cnt[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* DWORD_132 ~ DWORD_147 */
	uint16_t u2PLEErrorAmsduCnt[EXT_EVENT_SER_MAX_HW_ERROR_INT_NUMBER];

	/* ucEvtVer = 0 definition END */

	/* ucEvtVer = 1 definition BEGIN */
	/* ... */
};

#if CFG_SUPPORT_LOWLATENCY_MODE
struct PARAM_LOWLATENCY_DATA {
	uint32_t u4Events;
	uint32_t u4UdpDelayBound;
	uint32_t u4TcpDelayBound;
	uint32_t u4DataPhyRate;
	uint32_t u4UdpPriority;
	uint32_t u4TcpPriority;
	uint32_t u4SupportProtocol;
};
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

struct PARAM_SLEEP_CNT_INFO {
	uint32_t au4LmacSlpCnt[2];
	uint32_t u4WfsysSlpCnt;
	uint32_t u4CbinfraSlpCnt;
	uint32_t u4ChipSlpCnt;
};

struct MSG_ADD_DEL_MLD_LINK {
	uint8_t ucAction; /* 0: del, 1: add */
	uint8_t ucMldBssIdx;
	uint8_t ucRoleIdx;
	uint32_t u4LinkId;
	enum ENUM_IFTYPE eIftype;
	uint8_t aucMldAddr[MAC_ADDR_LEN];
	uint8_t aucLinkAddr[MAC_ADDR_LEN];
	void *prNetDevice;
	void *prWdev;
};

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
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
/*--------------------------------------------------------------*/
/* Routines to set parameters or query information.             */
/*--------------------------------------------------------------*/
/***** Routines in wlan_oid.c *****/
uint32_t
wlanoidQueryNetworkTypesSupported(struct ADAPTER *prAdapter,
				  void *pvQueryBuffer,
				  uint32_t u4QueryBufferLen,
				  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryNetworkTypeInUse(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetNetworkTypeInUse(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryBssid(struct ADAPTER *prAdapter,
		  void *pvQueryBuffer,
		  uint32_t u4QueryBufferLen,
		  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryLinkBssInfo(struct ADAPTER *prAdapter,
		  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetBssidListScanAdv(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryBssidList(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetBssid(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetConnect(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

uint32_t
wlanoidUpdateConnect(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetSsid(struct ADAPTER *prAdapter,
	       void *pvSetBuffer,
	       uint32_t u4SetBufferLen,
	       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQuerySsid(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryInfrastructureMode(struct ADAPTER *prAdapter,
			       void *pvQueryBuffer,
			       uint32_t u4QueryBufferLen,
			       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetInfrastructureMode(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryAuthMode(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer,
		     uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAuthMode(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetAuthorized(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

#if 0
uint32_t
wlanoidQueryPrivacyFilter(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer,
			  uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetPrivacyFilter(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetEncryptionStatus(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryEncryptionStatus(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAddWep(struct ADAPTER *prAdapter,
		 void *pvSetBuffer,
		 uint32_t u4SetBufferLen,
		 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetRemoveWep(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetAddKey(struct ADAPTER *prAdapter,
		 void *pvSetBuffer,
		 uint32_t u4SetBufferLen,
		 uint32_t *pu4SetInfoLen);

uint32_t
wlanSetAddKey(struct ADAPTER *prAdapter, void *pvSetBuffer,
		 uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen,
		 uint8_t fgIsOID);

uint32_t
wlanoidSetRemoveKey(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);

uint32_t
wlanSetRemoveKey(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen, uint8_t fgIsOid);

uint32_t
wlanoidQueryCapability(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryFrequency(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetFrequency(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryAtimWindow(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAtimWindow(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetChannel(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

uint32_t
wlanoidRssiMonitor(struct ADAPTER *prAdapter,
		   void *pvQueryBuffer,
		   uint32_t u4QueryBufferLen,
		   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRssi(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRssiTrigger(struct ADAPTER *prAdapter,
			void *pvQueryBuffer,
			uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetRssiTrigger(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryRtsThreshold(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetRtsThreshold(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQuery802dot11PowerSaveProfile(struct ADAPTER
				     *prAdapter,
				     void *pvQueryBuffer,
				     uint32_t u4QueryBufferLen,
				     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSet802dot11PowerSaveProfile(struct ADAPTER
				   *prAdapter,
				   void *prSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSet802dot11PowerSaveProfileImpl(struct ADAPTER
				   *prAdapter,
				   void *prSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen,
				   u_int8_t fgIsOid);

uint32_t
wlanoidSetPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidDelPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidFlushPmkid(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQuerySupportedRates(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryPermanentAddr(struct ADAPTER *prAdapter,
			  void *pvQueryBuf,
			  uint32_t u4QueryBufLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryCurrentAddr(struct ADAPTER *prAdapter,
			void *pvQueryBuf,
			uint32_t u4QueryBufLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryPermanentAddr(struct ADAPTER *prAdapter,
			  void *pvQueryBuf,
			  uint32_t u4QueryBufLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryMaxLinkSpeed(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryLinkSpeed(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer,
			  uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetTxLatMontrParam(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_BUFFER_MODE
uint32_t wlanoidSetEfusBufferMode(struct ADAPTER
				  *prAdapter,
				  void *pvSetBuffer,
				  uint32_t u4SetBufferLen,
				  uint32_t *pu4SetInfoLen);

uint32_t wlanoidConnacSetEfusBufferMode(struct ADAPTER
					*prAdapter,
					void *pvSetBuffer,
					uint32_t u4SetBufferLen,
					uint32_t *pu4SetInfoLen);

uint32_t wlanoidConnacSetEfuseBufferRD(struct ADAPTER
					*prAdapter,
					void *pvSetBuffer,
					uint32_t u4SetBufferLen,
					uint32_t *pu4SetInfoLen);

/* #if (CFG_EEPROM_PAGE_ACCESS == 1) */
uint32_t
wlanoidQueryProcessAccessEfuseRead(struct ADAPTER
				   *prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryProcessAccessEfuseWrite(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryEfuseFreeBlock(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryGetTxPower(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);
/*#endif*/

#endif /* CFG_SUPPORT_BUFFER_MODE */

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
uint32_t
wlanoidBssInfoBasicUnify(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidBssInfoBasic(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);
uint32_t
wlanoidBssInfoConOwnDev(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);

uint32_t
wlanoidDevInfoActive(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidInitAisFsm(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);


uint32_t
wlanoidUninitAisFsm(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_QA_TOOL
uint32_t
wlanoidQueryRxStatistics(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRxStatTlv(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidManualAssoc(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t wlanoidMuMimoAction(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_TX_BF
uint32_t
wlanoidTxBfAction(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

uint32_t wlanoidStaRecUpdate(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);

uint32_t wlanoidStaRecBFUpdate(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen);

uint32_t wlanoidStaRecBFRead(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen);

#endif /* CFG_SUPPORT_TX_BF */

#if CFG_SUPPORT_SMART_GEAR
uint32_t
wlandioSetSGStatus(struct ADAPTER *prAdapter,
			uint8_t ucSGEnable,
			uint8_t ucSGSpcCmd,
			uint8_t ucNSS);
#endif

uint32_t
wlanoidQueryMcrRead(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetMcrWrite(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_WIFI_ICCM
uint32_t
wlanoidSetIccm(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_WIFI_POWER_METRICS
uint32_t
wlanoidSetPowerMetrics(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidQueryDrvMcrRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetDrvMcrWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_WED_PROXY
uint32_t
wlanoidQueryDrvMcrReadDirectly(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetDrvMcrWriteDirectly(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidQueryEmiMcrRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryUhwMcrRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetUhwMcrWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQuerySwCtrlRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetSwCtrlWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetFixRate(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);


uint32_t
wlanoidSetAutoRate(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidSetMloAgcTx(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidGetMldRec(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidGetMlChnlCond(struct ADAPTER
				  *prAdapter,
				  void *pvQueryBuffer,
				  uint32_t u4QueryBufferLen,
				  uint32_t *pu4QueryInfoLen);
#endif
#endif

uint32_t
wlanoidSetPpCap(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetPpAlgCtrl(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetHmAlg(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetChipConfig(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanSetChipConfig(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen,
		     uint8_t fgIsOid);

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
uint32_t
wlanSetIcsSniffer(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen, u_int8_t isOid);

uint32_t
wlanoidSetIcsSniffer(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_ICS */

#if (CFG_SUPPORT_PHY_ICS == 1)
uint32_t
wlanoidSetPhyIcsEventOn(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetPhyIcsStart(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

#endif /* CFG_SUPPORT_PHY_ICS */

uint32_t
wlanoidQueryChipConfig(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetKeyCfg(struct ADAPTER *prAdapter,
		 void *pvSetBuffer,
		 uint32_t u4SetBufferLen,
		 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryEepromRead(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetEepromWrite(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryRfTestRxStatus(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRfTestTxStatus(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryOidInterfaceVersion(struct ADAPTER
				*prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryVendorId(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer,
		     uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryMulticastList(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer,
			  uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetMulticastList(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_NAN
uint32_t wlanoidSetNANMulticastList(struct ADAPTER *prAdapter,
				    uint8_t ucBssIdx, void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidQueryRcvError(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer,
		     uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRcvNoBuffer(struct ADAPTER *prAdapter,
			void *pvQueryBuffer,
			uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryRcvCrcError(struct ADAPTER *prAdapter,
			void *pvQueryBuffer,
			uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryStatistics(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryBugReport(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

#ifdef LINUX

uint32_t
wlanoidQueryStatisticsForLinux(struct ADAPTER *prAdapter,
			       void *pvQueryBuffer,
			       uint32_t u4QueryBufferLen,
			       uint32_t *pu4QueryInfoLen);

#endif

uint32_t
wlanoidQueryRcvOk(struct ADAPTER *prAdapter,
		  void *pvQueryBuffer,
		  uint32_t u4QueryBufferLen,
		  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryXmitOk(struct ADAPTER *prAdapter,
		   void *pvQueryBuffer,
		   uint32_t u4QueryBufferLen,
		   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryXmitError(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryXmitOneCollision(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryXmitMoreCollisions(struct ADAPTER *prAdapter,
			       void *pvQueryBuffer,
			       uint32_t u4QueryBufferLen,
			       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryXmitMaxCollisions(struct ADAPTER *prAdapter,
			      void *pvQueryBuffer,
			      uint32_t u4QueryBufferLen,
			      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetCurrentPacketFilter(struct ADAPTER *prAdapter,
			      void *pvSetBuffer,
			      uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen);

uint32_t wlanoidSetPacketFilter(struct ADAPTER *prAdapter,
				void *pvPacketFiltr,
				u_int8_t fgIsOid,
				void *pvSetBuffer,
				uint32_t u4SetBufferLen);

uint32_t
wlanoidQueryCurrentPacketFilter(struct ADAPTER
				*prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAcpiDevicePowerState(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryAcpiDevicePowerState(struct ADAPTER
				 *prAdapter,
				 void *pvQueryBuffer,
				 uint32_t u4QueryBufferLen,
				 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetDisassociate(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryFragThreshold(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer,
			  uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetFragThreshold(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryAdHocMode(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAdHocMode(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		    uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryBeaconInterval(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetBeaconInterval(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetCurrentAddr(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
uint32_t
wlanoidSetCSUMOffload(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

uint32_t
wlanoidSetNetworkAddress(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);
/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
uint32_t
wlanoidSetIPv6NetworkAddress(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);
#endif /* fos_change end */


uint32_t
wlanoidQueryMaxFrameSize(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryMaxTotalSize(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetCurrentLookahead(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

/* RF Test related APIs */
uint32_t
wlanoidRftestSetTestMode(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_XONVRAM
uint32_t
wlanoidRftestDoXOCal(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
uint32_t
wlanoidRftestDoPlCal(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);
#endif /* CFG_SUPPORT_PLCAL */

uint32_t
wlanoidRftestSetTestIcapMode(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidRftestSetAbortTestMode(struct ADAPTER *prAdapter,
			      void *pvSetBuffer,
			      uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidRftestQueryAutoTest(struct ADAPTER *prAdapter,
			   void *pvQueryBuffer,
			   uint32_t u4QueryBufferLen,
			   uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidRftestSetAutoTest(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);
uint32_t
wlanoidExtRfTestICapStart(struct ADAPTER *prAdapter,
			  void *pvSetBuffer,
			  uint32_t u4SetBufferLen,
			  uint32_t *pu4SetInfoLen);
uint32_t
wlanoidExtRfTestICapStatus(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

void wlanoidRfTestICapRawDataProc(struct ADAPTER *prAdapter,
				  uint32_t u4CapStartAddr,
				  uint32_t u4TotalBufferSize);

#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
uint32_t
wlanoidRfTestICapCopyDataToQA(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_WAPI
uint32_t
wlanoidSetWapiMode(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetWapiAssocInfo(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetWapiKey(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);
#endif

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
uint32_t
wlanoidSetFilsConnInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

#if CFG_ENABLE_WAKEUP_ON_LAN
uint32_t
wlanoidSetAddWakeupPattern(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetRemoveWakeupPattern(struct ADAPTER *prAdapter,
			      void *pvSetBuffer,
			      uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryEnableWakeup(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *u4QueryInfoLen);

uint32_t
wlanoidSetEnableWakeup(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetWiFiWmmPsTest(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetTxAmpdu(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetTxAggLimit(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetTxAmsduNumLimit(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetAddbaReject(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryNvramRead(struct ADAPTER *prAdapter,
		      void *pvQueryBuffer,
		      uint32_t u4QueryBufferLen,
		      uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetNvramWrite(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryCfgSrcType(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryEepromType(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen);

#if CFG_SUPPORT_LLW_SCAN
uint32_t
wlanoidSetLatency(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetCountryCode(struct ADAPTER *prAdapter,
		      void *pvSetBuffer,
		      uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetScanMacOui(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryWlanInfo(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer,
		     uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen);

uint32_t
wlanQueryWlanInfo(struct ADAPTER *prAdapter,
		     void *pvQueryBuffer,
		     uint32_t u4QueryBufferLen,
		     uint32_t *pu4QueryInfoLen,
		     uint8_t fgIsOid);

uint32_t
wlanoidQueryMibInfo(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen);

uint32_t
wlanQueryMibInfo(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen,
		 uint8_t fgIsOid);

uint32_t
wlanoidSetFwLog2Host(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetPhyCtrl(struct ADAPTER *prAdapter,
		    void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

#if 0
uint32_t
wlanoidSetNoaParam(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetOppPsParam(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetUApsdParam(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif

/*----------------------------------------------------------------------------*/
uint32_t
wlanoidSetBT(struct ADAPTER *prAdapter,
	     void *pvSetBuffer,
	     uint32_t u4SetBufferLen,
	     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryBT(struct ADAPTER *prAdapter,
	       void *pvQueryBuffer,
	       uint32_t u4QueryBufferLen,
	       uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetTxPower(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

#if 0
uint32_t
wlanoidQueryBtSingleAntenna(
	struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetBtSingleAntenna(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetPta(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryPta(
	struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);
#endif

#if CFG_ENABLE_WIFI_DIRECT
uint32_t
wlanoidSetP2pMode(struct ADAPTER *prAdapter,
		  void *pvSetBuffer,
		  uint32_t u4SetBufferLen,
		  uint32_t *pu4SetInfoLen);

uint32_t
wlanoidP2pDelIface(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen);

uint32_t
wlanoidP2pDelIfaceDone(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_NAN
uint32_t wlanoidSetNANMode(struct ADAPTER *prAdapter, void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetDefaultKey(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetGtkRekeyData(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_SCHED_SCAN
uint32_t
wlanoidSetStartSchedScan(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetStopSchedScan(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_SCHED_SCAN */

#if CFG_SUPPORT_PASSPOINT
uint32_t
wlanoidSetHS20Info(struct ADAPTER *prAdapter,
		   void *pvSetBuffer,
		   uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_PASSPOINT */

uint32_t
wlanoidNotifyFwSuspend(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryCnm(
	struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidPacketKeepAlive(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_DBDC
uint32_t
wlanoidSetDbdcEnable(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif /*#if CFG_SUPPORT_DBDC*/

#if CFG_SUPPORT_QA_TOOL
uint32_t
wlanoidQuerySetTxTargetPower(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);
#endif

#if (CFG_SUPPORT_DFS_MASTER == 1)
uint32_t
wlanoidQuerySetRddReport(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQuerySetRadarDetectMode(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen);

uint32_t
wlanoidInitRddLog(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetRddStart(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetRddStop(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryRddLog(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryRddLogContent(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

#endif

uint32_t
wlanoidLinkDown(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

#if CFG_WIFI_TXPWR_TBL_DUMP
uint32_t
wlanoidGetTxPwrTbl(struct ADAPTER *prAdapter,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen);
#endif

uint32_t
wlanoidAisPreSuspend(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_CSI
uint32_t
wlanoidSetCSIControl(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidDisableTdlsPs(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t wlanoidSetSer(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

uint32_t wlanoidSerExtCmd(struct ADAPTER *prAdapter,
			  uint8_t ucAction,
			  uint8_t ucSerSet,
			  uint8_t ucDbdcIdx);

uint32_t
wlanoidAbortScan(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen);

uint32_t wlanoidSetDrvSer(struct ADAPTER *prAdapter,
			  void *pvSetBuffer,
			  uint32_t u4SetBufferLen,
			  uint32_t *pu4SetInfoLen);
uint32_t wlanoidSetAmsduNum(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen);
uint32_t wlanoidSetAmsduSize(struct ADAPTER *prAdapter,
			     void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);

/* Show Consys debug information*/
uint32_t
wlanoidShowPdmaInfo(struct ADAPTER *prAdapter,
		    void *pvSetBuffer, uint32_t u4SetBufferLen,
		    uint32_t *pu4SetInfoLen);
uint32_t
wlanoidShowPseInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
uint32_t
wlanoidShowPleInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
uint32_t
wlanoidShowCsrInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);
uint32_t
wlanoidShowDmaschInfo(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
/* end Show Consys debug information*/

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t
wlanoidSetMddpGenSwitch(struct ADAPTER *prAdapter,
		      void *pvSetBuffer, uint32_t u4SetBufferLen,
		      uint32_t *pu4SetInfoLen);
#endif /* CFG_PCIE_GEN_SWITCH */

uint32_t
wlanoidShowAhdbgInfo(struct ADAPTER *prAdapter,
		   void *pvSetBuffer, uint32_t u4SetBufferLen,
		   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetTxPowerByRateManual(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
uint32_t
wlanoidQueryTxPowerInfo(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_LLS
uint32_t
wlanQueryLinkStats(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen);
#endif

#if CFG_SUPPORT_MBO
uint32_t wlanoidBssDisallowedList(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);

#endif

#if CFG_SUPPORT_ROAMING
uint32_t wlanoidSetDrvRoamingPolicy(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);
#endif

uint32_t wlanoidSetMultiStaPrimaryInterface(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);

#if (CFG_SUPPORT_ANDROID_DUAL_STA == 1)
uint32_t wlanoidSetMultiStaUseCase(struct ADAPTER
				    *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);
#endif

#if CFG_SUPPORT_OSHARE
uint32_t
wlanoidSetOshareMode(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidQueryWifiLogLevelSupport(struct ADAPTER *prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryWifiLogLevel(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidSetWifiLogLevel(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_LOWLATENCY_MODE
uint32_t
wlanoidSetLowLatencyMode(struct ADAPTER *prAdapter,
			 void *pvSetBuffer,
			 uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if CFG_SUPPORT_ANT_SWAP
uint32_t wlanoidQueryAntennaSwap(struct ADAPTER *prAdapter,
				void *pvQueryBuffer,
				uint32_t u4QueryBufferLen,
				uint32_t *pu4QueryInfoLen);
#endif


#if CFG_SUPPORT_EASY_DEBUG
uint32_t wlanoidSetFwParam(struct ADAPTER *prAdapter,
			   void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_EASY_DEBUG */

uint32_t wlanoidUpdateFtIes(struct ADAPTER *prAdapter, void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen);

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
uint32_t wlanoidSetMonitor(struct ADAPTER *prAdapter,
				void *pvSetBuffer,
				uint32_t u4SetBufferLen,
				uint32_t *pu4SetInfoLen);
#endif

#if (CFG_SUPPORT_802_11BE_EPCS == 1)
uint32_t wlanoidSendEpcs(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_802_11BE_EPCS */

uint32_t wlanoidSendNeighborRequest(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);

uint32_t wlanoidSendBTMQuery(struct ADAPTER *prAdapter, void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen);

uint32_t wlanoidPktProcessIT(struct ADAPTER *prAdapter, void *pvBuffer,
			     uint32_t u4BufferLen, uint32_t *pu4InfoLen);

uint32_t wlanoidFwEventIT(struct ADAPTER *prAdapter, void *pvBuffer,
			  uint32_t u4BufferLen, uint32_t *pu4InfoLen);

uint32_t wlanoidTspecOperation(struct ADAPTER *prAdapter, void *pvBuffer,
			       uint32_t u4BufferLen,
			       uint32_t *pu4InfoLen);

uint32_t wlanoidDumpUapsdSetting(struct ADAPTER *prAdapter, void *pvBuffer,
				 uint32_t u4BufferLen, uint32_t *pu4InfoLen);

uint32_t wlanoidGetWifiType(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen);

uint32_t wlanoidRfTestICapGetIQData(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);


#if CFG_SUPPORT_LINK_QUALITY_MONITOR
uint32_t wlanoidGetLinkQualityInfo(struct ADAPTER *prAdapter,
				   void *pvSetBuffer,
				   uint32_t u4SetBufferLen,
				   uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
uint32_t
wlanoidQueryStatsOneCmd(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);
#endif

uint32_t
wlanoidQueryStaLastRxRssi(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

#if CFG_SUPPORT_MBRAIN_BIGDATA
uint32_t
wlanoidQueryStaBigDataByWidx(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);
uint32_t
wlanoidQueryTrxLatBigData(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
uint32_t wlanoidTxPowerControl(struct ADAPTER *prAdapter,
			       void *pvSetBuffer,
			       uint32_t u4SetBufferLen,
			       uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidExternalAuthDone(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
uint32_t
wlanoidIndicateBssInfo(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryRoamScnChnl(struct ADAPTER *prAdapter,
			void *pvQueryBuffer,
			uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidAddRoamScnChnl(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetAxBlocklist(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

#if (CFG_WIFI_AUTO_RECOVER == 1)
enum {
	MTK_ACTION_SER_CHIP_RESET,
	MTK_ACTION_SER_WIFISYS_RESET,
	MTK_ACTION_SER_L1,
	MTK_ACTION_SER_L3,
	MTK_ACTION_DEL_BA,
	MTK_ACTION_ALL,
};
uint32_t
wlanoidMtkAction(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);

uint32_t
wlanoidMtkActionQuery(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetCusBlocklist(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidForceStbcMrc(struct ADAPTER *prAdapter,
		void *pvSetBuffer,
		uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidPresetLinkId(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

#if (CFG_SUPPORT_802_11BE_T2LM_NEGO == 1)
uint32_t
wlanoidSendT2LMRequest(struct ADAPTER *
		prAdapter, void *pvSetBuffer,
		uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSendT2LMTeardown(struct ADAPTER *
		prAdapter, void *pvSetBuffer,
		uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_802_11BE_T2LM_NEGO */

#if (CFG_SUPPORT_MLC == 1)
uint32_t
wlanoidSetMlcMode(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryMlcLinkState(struct ADAPTER *prAdapter, void *pvQueryBuffer,
	uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryMlcInfo(struct ADAPTER *prAdapter, void *pvQueryBuffer,
	uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);
#endif /* CFG_SUPPORT_MLC */
#endif /* CFG_SUPPORT_802_11BE_MLO */

uint32_t
wlanoidThermalProtectAct(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetMdvt(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
uint32_t wlanoidSetMdnsCmdToFw(struct ADAPTER *prAdapter,
				void *pvSetBuffer,
				uint32_t u4SetBufferLen,
				uint32_t *pu4SetInfoLen);

uint32_t wlanoidGetMdnsHitMiss(struct ADAPTER *prAdapter,
				void *pvSetBuffer,
				uint32_t u4SetBufferLen,
				uint32_t *pu4SetInfoLen);
#endif
#endif
#if (CFG_SUPPORT_TSF_SYNC == 1)
uint32_t
wlanoidLatchTSF(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen);
#endif

#if (CFG_SUPPORT_PKT_OFLD == 1)
uint32_t
wlanoidSetOffloadInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryOffloadInfo(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

uint32_t
wlanoidQueryApfInfo(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

#endif /* CFG_SUPPORT_PKT_OFLD */

#if (CFG_WIFI_ISO_DETECT == 1)
uint32_t
wlanoidQueryCoexIso(struct ADAPTER *prAdapter,
		    void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen,
		    uint32_t *pu4QueryInfoLen);
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
uint32_t
wlanoidQueryDpdCache(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen);
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
uint32_t
wlanoidTxQueryMcsInfo(struct ADAPTER *prAdapter,
		 void *pvQueryBuffer,
		 uint32_t u4QueryBufferLen,
		 uint32_t *pu4QueryInfoLen);
#endif

#if CFG_AP_80211K_SUPPORT
uint32_t wlanoidSendBeaconReportRequest(struct ADAPTER *prAdapter,
					void *pvSetBuffer,
					uint32_t u4SetBufferLen,
					uint32_t *pu4SetInfoLen);
uint32_t wlanoidSendCuReportRequest(struct ADAPTER *prAdapter,
				    void *pvSetBuffer,
				    uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);
#endif /* CFG_AP_80211K_SUPPORT */

#if CFG_AP_80211V_SUPPORT
uint32_t wlanoidSendBTMRequest(struct ADAPTER *prAdapter,
				    void *pvSetBuffer, uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);
#endif /* CFG_AP_80211V_SUPPORT */

uint32_t wlanoidEnableVendorSpecifiedRpt(struct ADAPTER *prAdapter,
				    void *pvSetBuffer, uint32_t u4SetBufferLen,
				    uint32_t *pu4SetInfoLen);

uint32_t wlanoidQuerySerInfo(struct ADAPTER *prAdapter,
			     void *pvQueryBuffer,
			     uint32_t u4QueryBufferLen,
			     uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryThermalAdieTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryThermalDdieTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);

uint32_t
wlanoidQueryThermalAdcTemp(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);

uint32_t wlanoidGetRttCapabilities(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);

uint32_t wlanoidHandleRttRequest(struct ADAPTER *prAdapter,
			 void *pvSetBuffer, uint32_t u4SetBufferLen,
			 uint32_t *pu4SetInfoLen);

#if (CONFIG_WLAN_SERVICE == 1)
uint32_t
wlanoidListMode(struct ADAPTER *prAdapter,
			 void *pvQueryBuffer,
			 uint32_t u4QueryBufferLen,
			 uint32_t *pu4QueryInfoLen);
#endif

uint32_t wlanoidGetSleepCntInfo(struct ADAPTER *prAdapter,
			void *pvGetBuffer,
			uint32_t u4GetBufferLen,
			uint32_t *pu4GetInfoLen);

uint32_t wlanoidSetLpKeepPwrCtrl(struct ADAPTER *prAdapter,
			void *pvSetBuffer,
			uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_MANIPULATE_TID
uint32_t
wlanoidManipulateTid(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_MANIPULATE_TID */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidQueryEmlInfo(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen);
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
uint32_t
wlanoidSet6GPwrMode(struct ADAPTER *prAdapter,
		     void *pvSetBuffer, uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */


#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
uint32_t
wlanoidSendPwrLimitToEmi(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif

uint32_t
wlanoidSetATXOP(struct ADAPTER *prAdapter,
			   void *pvSetBuffer, uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen);

#if CFG_SUPPORT_WED_PROXY
uint32_t
wlanoidWedAttachWarp(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
uint32_t
wlanoidWedDetachWarp(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
uint32_t
wlanoidWedSuspend(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
uint32_t
wlanoidWedResume(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
uint32_t
wlanoidWedRecoveryStatus(struct ADAPTER *prAdapter,
		     void *pvSetBuffer,
		     uint32_t u4SetBufferLen,
		     uint32_t *pu4SetInfoLen);
#endif

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t
wlanoidAddDelMldLink(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);
#endif

#if CFG_ENABLE_WIFI_DIRECT
uint32_t
wlanoidQueryLteSafeChannel(struct ADAPTER *prAdapter,
			void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			uint32_t *pu4QueryInfoLen);

#if (CFG_SUPPORT_SAP_BCN_PROT == 1)
uint32_t
wlanoidGetTxTsc(struct ADAPTER *prAdapter,
		void *pvSetBuffer, uint32_t u4SetBufferLen,
		uint32_t *pu4SetInfoLen);

uint32_t
wlanoidSetDefaultBcnKey(struct ADAPTER *prAdapter,
			void *pvSetBuffer, uint32_t u4SetBufferLen,
			uint32_t *pu4SetInfoLen);
#endif /* CFG_SUPPORT_SAP_BCN_PROT */
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t
wlandioStopPcieStatus(struct ADAPTER *prAdapter,
		uint8_t ucPcieStatus);
#endif
#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
uint32_t
wlanoidSetPcieMode(struct ADAPTER *prAdapter,
		       void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen);
#endif /*CFG_EAP_PCIE_GEN_SWITCH*/

uint32_t
wlanoidCcmRetrigger(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
uint32_t wlanoidBtCoreDumpCtrl(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */
#if CFG_SUPPORT_NAN
uint32_t
wlanoidGetNanDeviceInfo(struct ADAPTER *prAdapter,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen,
	uint32_t *pu4QueryInfoLen);
#endif

#if CFG_SUPPORT_FIPS
uint32_t
wlanoidFipsTc(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);
uint32_t
wlanoidFipsGetStatus(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);
uint32_t
wlanoidFipsGetResult(struct ADAPTER *prAdapter, void *pvQueryBuffer,
		    uint32_t u4QueryBufferLen, uint32_t *pu4QueryInfoLen);
#endif

#endif /* _WLAN_OID_H */
