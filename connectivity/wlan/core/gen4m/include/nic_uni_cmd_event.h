/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nic_uni_cmd_event.h"
 *  \brief This file contains the declairation file of the WLAN OID processing
 *	 routines of Windows driver for MediaTek Inc.
 *   802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_UNI_CMD_EVENT_H
#define _NIC_UNI_CMD_EVENT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_vendor.h"
#include "wsys_cmd_handler_fw.h"
#include "wlan_lib.h"
#if CFG_SUPPORT_MBRAIN
#include "gl_mbrain.h"
#endif
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define CMD_FLAG_UNI_CMD_MASK                   (0x40)
#define CMD_FLAG_UNI_CMD_OFFSET                 (6)

#define UNI_CMD_OPT_BIT_0_ACK        BIT(0) /* for SET command */
#define UNI_CMD_OPT_BIT_1_UNI_CMD    BIT(1) /* 1: unified cmd, 0:original cmd */
#define UNI_CMD_OPT_BIT_2_SET_QUERY  BIT(2) /* 1: set, 0:query */
#define UNI_CMD_OPT_BIT_1_UNI_EVENT  BIT(1)
#define UNI_CMD_OPT_BIT_2_UNSOLICIT_EVENT  BIT(2)


#define UNI_CMD_MSG_HEADER_LEN (sizeof(WIFI_UNI_CMD))
#define UNI_CMD_MAX_INBAND_CMD_LEN (1600 - UNI_CMD_MSG_HEADER_LEN)

#define MAX_UNI_EVENT_FAIL_TAG_COUNT 10

/* Customer Config Cmd for Gen4m */
#define UNICMD_CHIP_CONFIG_RESP_SIZE       320

#define UNI_MLD_LINK_MAX 3

/* UNI_CMD_SUSPEND_WOW_WAKEUP_PORT */
#define UNI_CMD_MAX_TCP_UDP_PORT 10

/* UNI_CMD_TWT_ARGT_UPDATE */
#define UNI_TWT_GRP_MAX_MEMBER_CNT  8

/* UNI_CMD_EDCA_AC_PARM */
#define UNI_CMD_EDCA_AIFS_BIT		(1 << 0)
#define UNI_CMD_EDCA_WIN_MIN_BIT	(1 << 1)
#define UNI_CMD_EDCA_WIN_MAX_BIT	(1 << 2)
#define UNI_CMD_EDCA_TXOP_BIT		(1 << 3)
#define UNI_CMD_EDCA_ALL_BITS	\
	(UNI_CMD_EDCA_AIFS_BIT | UNI_CMD_EDCA_WIN_MIN_BIT | \
	 UNI_CMD_EDCA_WIN_MAX_BIT | UNI_CMD_EDCA_TXOP_BIT)

/* UNI_EVENT_UPDATE_COEX_PHYRATE */
#define UNI_BSS_INFO_NUM 5

/* UNI_EVENT_MD_SAFE_CHN */
#define UNI_WIFI_CH_MASK_IDX_NUM 4

/* UNI_CMD_ACCESS_REG */
#define UNI_IS_RFCR(_addr) (((_addr) & BITS(24,31)) == 0x99000000)
#define UNI_STREAM_FROM_RFCR(_addr) (((_addr) & BITS(16,23)) >> 16)

/* UNI_CMD_ID_EFUSE_CONTROL usage */
#define BUFFER_MODE_CONTENT_MAX 1024
#define BUFFER_ACCESS_CONTENT_MAX 16

/* UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP */
#define DELAY_WAKEUP_PKT_INFO_LEN 43

/* MT6639 efuse bank number is 5, which is the current MAX */
#define MAX_EFUSE_BANK_NUM  5

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/* This is for the usage of assign a 2 byte Wcid to WcidL and WcidH */
#define WCID_SET_H_L(_HnVer, _L, _u2Value) \
	do { \
		_HnVer = (uint8_t)((((uint16_t)(_u2Value)) >> 8) & 0xff); \
		_L = (uint8_t)(((uint16_t)(_u2Value)) & 0xff); \
	} while (0)


/*******************************************************************************
 *			T Y P E   D E C L A R A T I O N S
 *******************************************************************************
 */

#if (CFG_SUPPORT_802_11BE_MLO == 1)
struct MLD_BSS_INFO;
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

#if (CFG_PCIE_GEN_SWITCH == 1)
extern u_int8_t g_ucReceiveGenSwitch;
extern u_int8_t g_ucBypassException;
#endif

#if (CFG_SUPPORT_ML_RECONFIG == 1)
struct MLD_STA_RECORD;
#endif

/*******************************************************************************
 *                                 COMMAND
 *******************************************************************************
 */

struct WIFI_UNI_CMD_INFO {
	uint16_t u2InfoBufLen;
	uint8_t *pucInfoBuffer;
	uint16_t u2CID;
	uint8_t  ucPktTypeID;
	uint8_t  ucS2DIndex;
	uint8_t  ucOption;
};

struct WIFI_UNI_CMD {
	uint16_t u2TxByteCount;	/* Max value is over 2048 */
	uint16_t u2PQ_ID;	/* Must be 0x8000 (Port1, Queue 0) */

	uint8_t ucWlanIdx;
	uint8_t ucHeaderFormat;
	uint8_t ucHeaderPadding;
	uint8_t ucPktFt: 2;
	uint8_t ucOwnMAC: 6;
	uint32_t au4Reserved1[6];

	uint16_t u2Length;
	uint16_t u2CID;

	uint8_t aucReserved[1];
	uint8_t ucPktTypeID;	/* Must be 0xA0 (CMD Packet) */
	uint8_t ucFragNum;
	uint8_t ucSeqNum;

	uint16_t u2Checksum;
	uint8_t ucS2DIndex;	/* Index for Src to Dst in CMD usage */
	uint8_t ucOption;	/* CID option */

	uint8_t aucReserved2[4];
	uint8_t aucBuffer[];
};

struct WIFI_UNI_SETQUERY_INFO {
	/* legacy cmd*/
	uint8_t ucCID;
	uint8_t ucExtCID;
	uint8_t fgSetQuery;
	uint8_t fgNeedResp;
	uint8_t fgIsOid;
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler;
	uint32_t u4SetQueryInfoLen;
	uint8_t *pucInfoBuffer;
	void *pvSetQueryBuffer;
	uint32_t u4SetQueryBufferLen;
	enum EUNM_CMD_SEND_METHOD eMethod;

	/* uni cmds */
	struct LINK rUniCmdList;
};

struct WIFI_UNI_CMD_ENTRY {
	struct LINK_ENTRY rLinkEntry;

	uint8_t ucUCID;
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler;
	uint32_t u4SetQueryInfoLen;
	uint8_t *pucInfoBuffer;
};

enum ENUM_UNI_CMD_ID {
	UNI_CMD_ID_DEVINFO		= 0x01, /* Update DEVINFO */
	UNI_CMD_ID_BSSINFO		= 0x02, /* Update BSSINFO */
	UNI_CMD_ID_STAREC_INFO		= 0x03, /* Update STAREC */
	UNI_CMD_ID_EDCA_SET		= 0x04, /* Update EDCA Set */
	UNI_CMD_ID_SUSPEND		= 0x05, /* Suspend */
	UNI_CMD_ID_OFFLOAD		= 0x06, /* Offload feature config */
	UNI_CMD_ID_HIF_CTRL		= 0x07, /* hif control */
	UNI_CMD_ID_BAND_CONFIG		= 0x08, /* Band Config */
	UNI_CMD_ID_REPT_MUAR		= 0x09, /* Repeater Mode Muar Config */
	UNI_CMD_ID_NORM_MUAR		= 0x0A, /* Normal Mode Muar Config */
	UNI_CMD_ID_WSYS_CONFIG		= 0x0B, /* WSYS Configuration */
	UNI_CMD_ID_ROAMING		= 0x0C, /* Roaming FSM switch */
	UNI_CMD_ID_ACCESS_REG		= 0x0D, /* Access Register */
	UNI_CMD_ID_CHIP_CONFIG		= 0x0E, /* Custom Chip Configuration*/
	UNI_CMD_ID_POWER_CTRL		= 0x0F, /* NIC Power control */
	UNI_CMD_ID_CFG_SMESH		= 0x10, /* Smesh Config */
	UNI_CMD_ID_RRM_11K		= 0x11, /* 802.11K RRM*/
	UNI_CMD_ID_RX_HDR_TRAN		= 0x12, /* Rx header translation */
	UNI_CMD_ID_SER			= 0x13, /* SER */
	UNI_CMD_ID_TWT			= 0x14, /* 80211AX TWT*/
	UNI_CMD_ID_SET_DOMAIN_INFO	= 0x15, /* Set Domain info */
	UNI_CMD_ID_SCAN_REQ		= 0x16, /* Scan Request */
	UNI_CMD_ID_IDC			= 0x17, /* IDC */
	UNI_CMD_ID_ECC_OPER		= 0x18, /* ECC Operation */
	UNI_CMD_ID_RDD_ON_OFF_CTRL	= 0x19, /* RDD On/Off Control */
	UNI_CMD_ID_GET_MAC_INFO		= 0x1A, /* Get MAC info */
	UNI_CMD_ID_TDLS			= 0x1B, /* TDLS */
	UNI_CMD_ID_TXCMD_CTRL		= 0x1D, /* Txcmd ctrl */
	UNI_CMD_ID_SET_DROP_PACKET_CFG	= 0x1E,  /* Set Packet Drop cfg */
	UNI_CMD_ID_BA_OFFLOAD		= 0x1F, /* BA Offload */
	UNI_CMD_ID_P2P			= 0x20, /* P2P */
	UNI_CMD_ID_SMART_GEAR		= 0x21, /* Smart Gear */
	UNI_CMD_ID_MIB			= 0x22, /* Get MIB counter */
	UNI_CMD_ID_GET_STATISTICS	= 0x23, /* Get Statistics */
	UNI_CMD_ID_SNIFFER_MODE		= 0x24, /* Sniffer Mode */
	UNI_CMD_ID_SR			= 0x25, /* SR */
	UNI_CMD_ID_SCS			= 0x26, /* SCS */
	UNI_CMD_ID_CNM			= 0x27, /*CNM*/
	UNI_CMD_ID_MBMC			= 0x28, /*MBMC*/
	UNI_CMD_ID_DVT			= 0x29, /* DVT */
	UNI_CMD_ID_GPIO			= 0x2A, /* GPIO setting*/
	UNI_CMD_ID_TXPOWER		= 0x2B, /* RLM Tx Power */
	UNI_CMD_ID_POWER_LIMIT		= 0x2C, /* Tx Power Limit*/
	UNI_CMD_ID_EFUSE_CONTROL	= 0x2D, /* EFUSE Control */
	UNI_CMD_ID_NVRAM_SETTINGS	= 0x2E, /* Set NVRAM setting */
	UNI_CMD_ID_RA			= 0x2F, /* RA */
	UNI_CMD_ID_SPECTRUM		= 0x30, /* Spectrum */
	UNI_CMD_ID_MURU			= 0x31, /* MURU */
	UNI_CMD_ID_TESTMODE_RX_STAT	= 0x32, /* testmode Rx statistic */
	UNI_CMD_ID_BF			= 0x33, /* BF */
	UNI_CMD_ID_CHAN_SWITCH		= 0x34, /* Channel Switch */
	UNI_CMD_ID_THERMAL		= 0x35, /* Thermal control */
	UNI_CMD_ID_NOISE_FLOOR		= 0x36, /* Noise Floor */
	UNI_CMD_ID_VOW			= 0x37, /* VOW */
	UNI_CMD_ID_PP			= 0x38, /* PP */
	UNI_CMD_ID_TPC			= 0x39, /* TPC */
	UNI_CMD_ID_MEC			= 0x3A, /* MEC */
	UNI_CMD_ID_POWER_METRICS	= 0x3F, /* Power Metrics */
	UNI_CMD_ID_FR_TABLE		= 0x40, /* Set Fixed Rate TBL */
	UNI_CMD_ID_RSSI_MONITOR		= 0x41, /* Set monitoring RSSI range */
	UNI_CMD_ID_TEST_TR_PARAM	= 0x42, /* Set/Get testmode parameter */
	UNI_CMD_ID_MQM_UPDATE_MU_EDCA_PARMS = 0x43, /* MU */
	UNI_CMD_ID_PERF_IND = 0x44, /* Support performance indicate*/
	UNI_CMD_ID_FRM_IND_FROM_HOST = 0x45, /* Support Host connect indicate*/
	UNI_CMD_ID_TESTMODE_CTRL	= 0x46, /* testmode RF	*/
	UNI_CMD_ID_HANDLE_PRECAL_RESULT = 0x47, /* Handle Pre Cal Result*/
	UNI_CMD_ID_ICS			= 0x49, /* ICS */
	UNI_CMD_ID_CSI			= 0x4A, /* TM CSI control*/
	UNI_CMD_ID_VLAN_CFG		= 0x4B, /* VLAN */
	UNI_CMD_ID_CAL			= 0x4C, /* CAL */
	UNI_CMD_ID_HWCFG_CTRL		= 0x4E, /* Hwcfg */
	UNI_CMD_ID_SWACI_CTRL		= 0x4F, /* SWACI */
	UNI_CMD_ID_DYN_WMM_CTRL		= 0x50, /* DYN_WMM */
	UNI_CMD_ID_EAP_CTRL		= 0x51, /* EAP */
	UNI_CMD_ID_PHY_STATE_INFO	= 0x52, /* PHY_STATE */
	UNI_CMD_ID_LED			= 0x53, /* LED */
	UNI_CMD_ID_FAST_PATH		= 0x54, /* Fast Path */
	UNI_CMD_ID_NAN			= 0x56, /* NAN */
	UNI_CMD_ID_MLO			= 0x59, /* MLO */
	UNI_CMD_ID_ACL_POLICY		= 0x5A, /* ACL */
	UNI_CMD_ID_SEND_VOLT_INFO	= 0x5B, /* VOLT_INFO */
	UNI_CMD_ID_RTT			= 0x5D, /* RTT */
	UNI_CMD_ID_PKT_OFLD		= 0x60, /* Packet Offload */
	UNI_CMD_ID_KEEP_ALIVE		= 0x61, /* Keep alive */
	UNI_CMD_ID_LOW_LATENCY_MODE     = 0x62, /* Low Latency Mode */
	UNI_CMD_ID_GAMING_MODE          = 0x63, /* Gaming Mode */
	UNI_CMD_ID_MDNS_RECORD		= 0x64, /* Keep alive */
	UNI_CMD_ID_SET_SAP		= 0x70, /* SAP */
	UNI_CMD_ID_LP_DBG_CTRL		= 0x71, /* LP */
	UNI_CMD_ID_UWB_COEX		= 0x75, /* UWB COEX */
	UNI_CMD_ID_BT_CTRL		= 0x7B, /* BT control */
	UNI_CMD_ID_FACT_CAL		= 0x7C, /* Factory Calibration*/
	UNI_CMD_ID_HM			= 0x7D, /* Hybrid mlo */
	UNI_CMD_ID_RESET_TX_SCRAMBLE	= 0x7E, /* TX RESET SCRAMBLE */
	UNI_CMD_ID_PHY_LIST_DUMP	= 0x7F, /* Get Phy CR */
	UNI_CMD_ID_MLC			= 0x81, /* Multi-link Control */
	UNI_CMD_ID_UPDATE_LP	= 0x84, /*Update LP Parameter*/
#if CFG_SUPPORT_FIPS
	UNI_CMD_ID_FIPS		= 0x85, /* FIPS test */
#endif
	UNI_CMD_ID_COEX	= 0x87, /*notify FW coex cmd*/
	UNI_CMD_ID_UPDATE_PCIE	= 0x89, /*Update PCIE Parameter*/
	UNI_CMD_ID_PHY_ICS = 0x8A, /*PHY ICS*/
	UNI_CMD_ID_SHR_ANT_SWCH		= 0x92, /* Shared Antenna Switch */
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DEVINFO {
	/* fixed field */
	uint8_t ucOwnMacIdx;
	uint8_t ucDbdcIdx;
	uint8_t aucPadding[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* DevInfo command Tag */
enum ENUM_UNI_CMD_DEVINFO_TAG {
	UNI_CMD_DEVINFO_TAG_ACTIVE = 0,
	UNI_CMD_DEVINFO_TAG_NUM
};

/* DevInfo information (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DEVINFO_ACTIVE {
	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;
	uint8_t ucActive;
	uint8_t ucMldLinkIdx;
	uint8_t aucOwnMacAddr[6];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO {
	/* fixed field */
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* BssInfo command Tag */
enum ENUM_UNI_CMD_BSSINFO_TAG {
	UNI_CMD_BSSINFO_TAG_BASIC = 0,
	UNI_CMD_BSSINFO_TAG_RA = 1,
	UNI_CMD_BSSINFO_TAG_RLM = 2,
	UNI_CMD_BSSINFO_TAG_PROTECT = 3,
	UNI_CMD_BSSINFO_TAG_BSS_COLOR = 4,
	UNI_CMD_BSSINFO_TAG_HE = 5,
	UNI_CMD_BSSINFO_TAG_11V_MBSSID = 6,
	UNI_CMD_BSSINFO_TAG_BCN_CONTENT = 7,
	UNI_CMD_BSSINFO_TAG_BCN_CSA = 8,
	UNI_CMD_BSSINFO_TAG_BCN_BCC = 9,
	UNI_CMD_BSSINFO_TAG_BCN_MBSSID = 0xA,
	UNI_CMD_BSSINFO_TAG_RATE = 0xB,
	UNI_CMD_BSSINFO_TAG_WAPI = 0xC,
	UNI_CMD_BSSINFO_TAG_SAP = 0xD,
	UNI_CMD_BSSINFO_TAG_P2P = 0xE,
	UNI_CMD_BSSINFO_TAG_QBSS = 0xF,
	UNI_CMD_BSSINFO_TAG_SEC = 0x10,
	UNI_CMD_BSSINFO_TAG_BCN_PROT = 0x11,
	UNI_CMD_BSSINFO_TAG_TXCMD = 0x12,
	UNI_CMD_BSSINFO_TAG_UAPSD = 0x13,
	UNI_CMD_BSSINFO_TAG_WMM_PS_TEST = 0x14,
	UNI_CMD_BSSINFO_TAG_POWER_SAVE = 0x15,
	UNI_CMD_BSSINFO_TAG_PM_ENABLE = 0x16,
	UNI_CMD_BSSINFO_TAG_IFS_TIME = 0x17,
	UNI_CMD_BSSINFO_TAG_STA_IOT = 0x18,
	UNI_CMD_BSSINFO_TAG_OFFLOAD_PKT = 0x19,
	UNI_CMD_BSSINFO_TAG_MLD = 0x1A,
	UNI_CMD_BSSINFO_TAG_PM_DISABLE = 0x1B,
	UNI_CMD_BSSINFO_TAG_MAX_IDLE_PERIOD = 0x1D,
	UNI_CMD_BSSINFO_TAG_EHT = 0x1E,
	UNI_CMD_BSSINFO_TAG_FILS_DISCOVERY = 0x22,
	UNI_CMD_BSSINFO_NUM
};

typedef uint32_t(*PFN_UNI_CMD_BSSINFO_TAG_HANDLER) (struct ADAPTER
	*ad, uint8_t *buf, struct CMD_SET_BSS_INFO *cmd);

struct UNI_CMD_BSSINFO_TAG_HANDLE {
	uint32_t u4Size;
	PFN_UNI_CMD_BSSINFO_TAG_HANDLER pfHandler;
};

/* BssInfo basic information (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BASIC {
	uint16_t u2Tag;          // Tag = 0x00
	uint16_t u2Length;
	uint8_t  ucActive;
	uint8_t  ucOwnMacIdx;
	uint8_t  ucHwBSSIndex;
	uint8_t  ucDbdcIdx;
	uint32_t u4ConnectionType;
	uint8_t  ucConnectionState;
	uint8_t  ucWmmIdx;
	uint8_t  aucBSSID[6];
	uint16_t u2BcMcWlanidx;  // indicate which wlan-idx used for MC/BC transmission.
	uint16_t u2BcnInterval;
	uint8_t  ucDtimPeriod;
	uint8_t  ucPhyMode;
	uint16_t u2StaRecIdxOfAP;
	uint16_t u2NonHTBasicPhyType;
	uint8_t  ucPhyModeExt;
	uint8_t  ucMldLinkIdx;
}__KAL_ATTRIB_PACKED__;

/* BssInfo RA information (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_RA {
	uint16_t  u2Tag;                 /* Tag = 0x01 */
	uint16_t  u2Length;
	uint8_t  fgShortPreamble;
	uint8_t  fgTestbedForceShortGI;
	uint8_t  fgTestbedForceGreenField;
	uint8_t   ucHtMode;
	uint8_t  fgSeOff;
	uint8_t   ucAntennaIndex;
	uint16_t  u2MaxPhyRate;
	uint8_t   ucForceTxStream;
	uint8_t   aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo RLM information (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_RLM {
	uint16_t u2Tag;  /* Tag = 0x02 */
	uint16_t u2Length;
	uint8_t  ucPrimaryChannel;
	uint8_t  ucCenterChannelSeg0;
	uint8_t  ucCenterChannelSeg1;
	uint8_t  ucBandwidth;
	uint8_t  ucTxStream;
	uint8_t  ucRxStream;
	uint8_t  ucHtOpInfo1; /* for mobile segment */
	uint8_t  ucSCO;    /* for mobile segment */
	uint8_t	ucRfBand;    /**<
	    *	rf band      | Value
	    *	----------   | ------
	    *	CMD_BAND_2G4 | 1
	    *	CMD_BAND_5G  | 2
	    *	CMD_BAND_6G  | 3
	    */
	uint8_t	ucPaddings[3];
} __KAL_ATTRIB_PACKED__;

/*the enum elements syncs with the definition of host driver*/
enum ENUM_PROTECTION_MODE_T {
	/* 11n */
	HT_NON_MEMBER_PROTECT = BIT(1),
	HT_BW20_PROTECT = BIT(2),
	HT_NON_HT_MIXMODE_PROTECT = BIT(3),
	LEGACY_ERP_PROTECT = BIT(5),
	VEND_LONG_NAV_PROTECT = BIT(6),
	VEND_GREEN_FIELD_PROTECT = BIT(7),
	VEND_RIFS_PROTECT = BIT(8),
};

/* BssInfo protection information (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_PROTECT {
	uint16_t u2Tag;  /* Tag = 0x03 */
	uint16_t u2Length;
	uint32_t u4ProtectMode;
} __KAL_ATTRIB_PACKED__;

/* BssInfo bss color information (Tag4) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BSS_COLOR {
	uint16_t u2Tag;  /* Tag = 0x4 */
	uint16_t u2Length;
	uint8_t fgEnable;
	uint8_t  ucBssColor;
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* BssInfo HE information (Tag5) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_HE {
	uint16_t u2Tag;  /* Tag = 0x05 */
	uint16_t u2Length;
	uint16_t u2TxopDurationRtsThreshold;
	uint8_t  ucDefaultPEDuration;
	uint8_t fgErSuDisable; /* for mobile segment */
	uint16_t au2MaxNssMcs[3];
	uint8_t uc6GPwrRegBitMap;
	uint8_t  aucPadding[1];
} __KAL_ATTRIB_PACKED__;

/* BssInfo 11v MBSSID information (Tag6) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_11V_MBSSID {
	uint16_t u2Tag;  /* Tag = 0x06 */
	uint16_t u2Length;
	uint8_t  ucMaxBSSIDIndicator;
	uint8_t  ucMBSSIDIndex;
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BCN_CONTENT {
	uint16_t u2Tag;       /* Tag = 0x07 */
	uint16_t u2Length;
	uint16_t u2TimIeOffset;
	uint16_t u2CsaIeOffset;
	uint16_t u2BccIeOffset;
	uint8_t  ucAction;
	uint8_t  aucPktContentType;
	uint16_t u2PktLength;
	uint8_t  aucPktContent[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_FILS_REQ {
	uint16_t u2Tag;       /* Tag = 0x22 */
	uint16_t u2Length;
	uint32_t u4MinInterval;
	uint32_t u4MaxInterval;
	uint16_t u2PktLength;
	uint8_t aucReserved[2];
	uint8_t  aucPktContent[];
} __KAL_ATTRIB_PACKED__;

enum BCN_CONTENT_ACTION {
	BCN_ACTION_DISABLE = 0,
	BCN_ACTION_ENABLE = 1,
	UPDATE_PROBE_RSP = 2,
	UPDATE_UNSOL_PROBE_RSP = 3,
};


/* BssInfo BCN CSA information (Tag8) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BCN_CSA {
	uint16_t u2Tag;       /* Tag = 0x08 */
	uint16_t u2Length;
	uint8_t  ucCsaCount;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo BCN BCC information (Tag9) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BCN_BCC {
	uint16_t u2Tag;       /* Tag = 0x9 */
	uint16_t u2Length;
	uint8_t  ucBccCount;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo BCN Mbssid-index ie information (Tag10) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BCN_MBSSID {
	uint16_t u2Tag;       /* Tag = 0xA */
	uint16_t u2Length;
	uint32_t u4Dot11vMbssidBitmap;
	uint16_t u2MbssidIeOffset[32];
} __KAL_ATTRIB_PACKED__;

/* BssInfo RATE information (Tag11) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_RATE {
	uint16_t u2Tag;  /* Tag = 0x0B */
	uint16_t u2Length;
	uint16_t u2OperationalRateSet; /* for mobile segment */
	uint16_t u2BSSBasicRateSet;    /* for mobile segment */
	uint16_t u2BcRate; //for WA
	uint16_t u2McRate; //for WA
	uint8_t  ucPreambleMode; //for WA
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo WAPI information (Tag12) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_WAPI {
	uint16_t u2Tag;  /* Tag = 0x0C */
	uint16_t u2Length;
	uint8_t fgWapiMode;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo SAP information (Tag13) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_SAP {
	uint16_t u2Tag;  /* Tag = 0x0D */
	uint16_t u2Length;
	uint8_t fgIsHiddenSSID;
	uint8_t  aucPadding[2];
	uint8_t  ucSSIDLen;
	uint8_t  aucSSID[32];
} __KAL_ATTRIB_PACKED__;

/* BssInfo P2P information (Tag14) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_P2P {
	uint16_t u2Tag;  /* Tag = 0x0E */
	uint16_t u2Length;
	uint32_t  u4PrivateData;
} __KAL_ATTRIB_PACKED__;

/* BssInfo QBSS information (Tag15) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_QBSS {
	uint16_t u2Tag;  /* Tag = 0x0F */
	uint16_t u2Length;
	uint8_t  ucIsQBSS;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo Security information (Tag16) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_SEC {
	uint16_t u2Tag;  /* Tag = 0x10 */
	uint16_t u2Length;
	uint8_t  ucAuthMode;/**<
	  *     Auth Mode             | Value | Note          |
	  *     --------------------  | ------|-------------- |
	  *     AUTH_MODE_OPEN        | 0     | -             |
	  *     AUTH_MODE_SHARED      | 1     | Shared key    |
	  *     AUTH_MODE_AUTO_SWITCH | 2     | Either open system or shared key |
	  *     AUTH_MODE_WPA         | 3     | -             |
	  *     AUTH_MODE_WPA_PSK     | 4     | -             |
	  *     AUTH_MODE_WPA_NONE    | 5     | For Ad hoc    |
	  *     AUTH_MODE_WPA2        | 6     | -             |
	  *     AUTH_MODE_WPA2_PSK    | 7     | -             |
	  *     AUTH_MODE_WPA2_FT     | 8     | 802.11r       |
	  *     AUTH_MODE_WPA2_FT_PSK | 9     | 802.11r       |
	  *     AUTH_MODE_WPA_OSEN    | 10    | -             |
	  *     AUTH_MODE_WPA3_SAE    | 11    | -             |
	  */
	uint8_t  ucEncStatus;
	uint8_t  ucCipherSuit; /* wa */
	uint8_t  aucPadding[1];
} __KAL_ATTRIB_PACKED__;

/* BssInfo BCN Prot. information (Tag 0x11) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_BCN_PROT {
	uint16_t u2Tag; /* Tag = 0x11 */
	uint16_t u2Length;
	uint8_t aucBcnProtPN[6];
	uint8_t ucBcnProtEnabled;  /* 0: off, 1: SW mode, 2:HW mode */
	uint8_t ucBcnProtCipherId;
	uint8_t aucBcnProtKey[32];
	uint8_t ucBcnProtKeyId;
	uint8_t ucBmcWlanIndex;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

/* TxCMD Mode information (Tag 0x12) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_TXCMD {
	uint16_t u2Tag;  /* Tag = 0x12 */
	uint16_t u2Length;
	uint8_t fgUseTxCMD;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo UAPSD information (Tag 0x13) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_UAPSD {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucBmpDeliveryAC;
	uint8_t  ucBmpTriggerAC;
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* BssInfo WMM PS test information (Tag 0x14) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_WMM_PS_TEST {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucIsEnterPsAtOnce;
	uint8_t  ucIsDisableUcTrigger;
	uint8_t  aucPadding[2];
}__KAL_ATTRIB_PACKED__;

/* BssInfo Power Save information (Tag 0x15) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_POWER_SAVE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucPsProfile;/**<
	  *     Power Save Mode                 | Value | Note                     |
	  *     --------------------            | ------|--------------            |
	  *     ENUM_PSP_CONTINUOUS_ACTIVE      | 0     | Leave power save mode    |
	  *     ENUM_PSP_CONTINUOUS_POWER_SAVE  | 1     | Enter power save mode    |
	  *     ENUM_PSP_FAST_SWITCH            | 2     | Fast switch mode         |
	  *     ENUM_PSP_TWT                    | 3     | twt                      |
	  *     ENUM_PSP_TWT_SP                 | 4     | twt sp                   |
	  */
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo STA connection information (Tag 0x16) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_STA_PM_ENABLE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2BcnInterval;
	uint8_t  ucDtimPeriod;
	uint8_t  ucBmpDeliveryAC;
	uint8_t  ucBmpTriggerAC;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo IFS time information (Tag 0x17) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_IFS_TIME {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgSlotValid;
	uint8_t fgSifsValid;
	uint8_t fgRifsValid;
	uint8_t fgEifsValid;
	uint16_t u2SlotTime;
	uint16_t u2SifsTime;
	uint16_t u2RifsTime;
	uint16_t u2EifsTime;
	uint8_t fgEifsCckValid;
	uint8_t aucPadding[1];
	uint16_t u2EifsCckTime;
} __KAL_ATTRIB_PACKED__;

/* BssInfo Mobile need information (Tag 0x18) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_IOT {
	uint16_t u2Tag; /* Tag = 0x18 */
	uint16_t u2Length;
	uint8_t ucIotApBmp;
	uint8_t ucIotApAct;
	uint8_t aucReserved[2];
	uint64_t u8IotApBmp;
} __KAL_ATTRIB_PACKED__;

/* BssInfo IFS time information (Tag 0x19) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_OFFLOAD_PKT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucTxType;
	uint8_t  ucTxMode;
	uint8_t  ucTxInterval;
	uint8_t  fgEnable;
	uint16_t u2Wcid;
	uint16_t u2OffloadPktLength;
	uint8_t  aucPktContent[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNSOLICIT_TX {
	UNSOLICIT_TX_PROBE_RSP = 0,
	UNSOLICIT_TX_FILS_DISC = 1,
	UNSOLICIT_TX_QOS_NULL  = 2 /* packet injector */
};

/* BssInfo MLD information (Tag 0x1A) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_MLD {
	uint16_t u2Tag;   /* Tag = 0x1A */
	uint16_t u2Length;
	/*
	 * the own MLD id for MLD address, range from 0 to 63
	 * 0xFF means this is a legacy bss, not in any MLD group
	 */
	uint8_t  ucGroupMldId;
	/*
	 * own_mac_addr in mat_table idx
	 * the own MLD id for ownmac address, range from 0 to 63
	 * 0xFF means no need to do remapping
	 */
	uint8_t  ucOwnMldId;
	/*
	 * the MLD address
	 * legacy=don't care
	 */
	uint8_t  aucOwnMldAddr[MAC_ADDR_LEN];
	/*
	 * for own mac id remapping for AGG, range from 0 to 15,
	 * 0xFF means no need to do remapping
	 */
	uint8_t  ucOmRemapIdx;

	/*
	 * MLD per link id, legacy is 0xff
	 */
	uint8_t  ucLinkId;
	/*
	 * EML enabled
	 */
	uint8_t  ucEmlEnabled;
	/*
	 * Max simultaneous links
	 */
	uint8_t  ucMaxSimuLinks;
	/*
	 * hybrid mlo enabled
	 */
	uint8_t  ucHmloEnabled;

	/*
	 * for 4 byte alignment
	 */
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo Max Idle Period element (Tag 0x1D) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_MAX_IDLE_PERIOD {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2MaxIdlePeriod;
	uint8_t  ucIdleOptions;
	uint8_t  ucReserved;
} __KAL_ATTRIB_PACKED__;

/* BssInfo EHT information (Tag 0x1E) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_EHT {
	uint16_t u2Tag;  /* Tag = 0x1E */
	uint16_t u2Length;
	uint8_t  fgIsEhtOpPresent;
	uint8_t  fgIsEhtDscbPresent;
	uint8_t  ucEhtCtrl;
	uint8_t  ucEhtCcfs0;
	uint8_t  ucEhtCcfs1;
	uint8_t  ucPadding1;
	uint16_t u2EhtDisSubChanBitmap;
	uint8_t  fgEhtMcs15Disable;
	uint8_t  aucPadding2[3];
} __KAL_ATTRIB_PACKED__;

/* BssInfo STA PM disable (Tag 0x1B) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSSINFO_STA_PM_DISABLE
{
    uint16_t u2Tag;
    uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* Common part of CMD_STAREC */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC {
	/* Fixed field*/
	uint8_t ucBssInfoIdx;
	uint8_t ucWlanIdxL;
	uint8_t aucPadding[4];
	uint8_t ucWlanIdxHnVer;
	uint8_t aucPadding2[1];

	/* TLV */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/*  STA record TLV tag */
enum ENUM_UNI_CMD_STAREC_TAG {
	UNI_CMD_STAREC_TAG_BASIC               	= 0x00,
	UNI_CMD_STAREC_TAG_RA                  	= 0x01,
	UNI_CMD_STAREC_TAG_RA_COMMON_INFO	= 0x02,
	UNI_CMD_STAREC_TAG_RA_UPDATE           	= 0x03,
	UNI_CMD_STAREC_TAG_BF                  	= 0x04,
	UNI_CMD_STAREC_TAG_MAUNAL_ASSOC        	= 0x05,
	UNI_CMD_STAREC_TAG_BA                  	= 0x06,
	UNI_CMD_STAREC_TAG_STATE_CHANGED       	= 0x07,
	UNI_CMD_STAREC_TAG_HT_BASIC            	= 0x09,
	UNI_CMD_STAREC_TAG_VHT_BASIC           	= 0x0a,
	UNI_CMD_STAREC_TAG_AP_PS               	= 0x0b,
	UNI_CMD_STAREC_TAG_INSTALL_KEY         	= 0x0c,
	UNI_CMD_STAREC_TAG_WTBL                	= 0x0d,
	UNI_CMD_STAREC_TAG_HW_AMSDU            	= 0x0f,
	UNI_CMD_STAREC_TAG_AAD_OM              	= 0x10,
	UNI_CMD_STAREC_TAG_INSTALL_KEY_V2      	= 0x11,
	UNI_CMD_STAREC_TAG_MURU                	= 0x12,
	UNI_CMD_STAREC_TAG_BFEE                	= 0x14,
	UNI_CMD_STAREC_TAG_PHY_INFO            	= 0x15,
	UNI_CMD_STAREC_TAG_BA_OFFLOAD          	= 0x16,
	UNI_CMD_STAREC_TAG_HE_6G_CAP          	= 0x17,
	UNI_CMD_STAREC_TAG_INSTALL_DEFAULT_KEY 	= 0x18,
	UNI_CMD_STAREC_TAG_HE_BASIC		= 0x19,
	UNI_CMD_STAREC_TAG_MLD_SETUP		= 0x20,
	UNI_CMD_STAREC_TAG_EHT_MLD		= 0x21,
	UNI_CMD_STAREC_TAG_EHT_BASIC		= 0x22,
	UNI_CMD_STAREC_TAG_MLD_TEARDOWN		= 0x23,
	UNI_CMD_STAREC_TAG_UAPSD		= 0x24,
	UNI_CMD_STAREC_TAG_REMOVE		= 0x25,
	UNI_CMD_STAREC_TAG_GET_PN		= 0x26,
	UNI_CMD_STAREC_TAG_INSTALL_KEY_V3	= 0x27,
	UNI_CMD_STAREC_TAG_FAST_ALL		= 0x2C,
	UNI_CMD_STAREC_TAG_MLR_INFO		= 0x2D,
	UNI_CMD_STAREC_TAG_NAN			= 0x2E,
	UNI_CMD_STAREC_TAG_T2LM			= 0x3E,
	UNI_CMD_STAREC_TAG_MLD_RECFG_STATE	= 0x3F,
	UNI_CMD_STAREC_TAG_INSTALL_LTF_KEYSEED	= 0x40,
	UNI_CMD_STAREC_TAG_EHT_MLD_V2		= 0x49,
	UNI_CMD_STAREC_TAG_MAX_NUM
};

typedef uint32_t(*PFN_UNI_CMD_STAREC_TAG_HANDLER) (struct ADAPTER
	*ad, uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd);

struct UNI_CMD_STAREC_TAG_HANDLE {
	uint32_t u4Size;
	PFN_UNI_CMD_STAREC_TAG_HANDLER pfHandler;
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_BASIC {
	/* Basic STA record (Group0) */
	uint16_t u2Tag;		/* Tag = 0x00 */
	uint16_t u2Length;
	uint32_t u4ConnectionType;
	uint8_t	ucConnectionState;
	uint8_t	ucIsQBSS;
	uint16_t u2AID;
	uint8_t	aucPeerMacAddr[6];
	/*This is used especially for 7615 to indicate this STAREC is to create new one or simply update
	In some case host may send new STAREC without delete old STAREC in advance. (ex: lost de-auth or get assoc twice)
	We need extra info to know if this is a brand new STAREC or not
	Consider backward compatibility, we check bit 0 in this reserve.
	Only the bit 0 is on, N9 go new way to update STAREC if bit 1 is on too.
	If neither bit match, N9 go orinal way to update STAREC. */
	uint16_t u2ExtraInfo;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_STATE_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucStaState;
	uint8_t audPaddings[3];
	uint32_t u4Flags;
	uint8_t ucVhtOpMode;   /* VHT operting mode, bit 7: Rx NSS Type, bit 4-6: Rx NSS, bit 0-1: Channel Width */
	uint8_t ucActionType;
	uint8_t audPaddings2[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TXBF_PFMU_STA_INFO {
	uint16_t   u2PfmuId;           /* 0xFFFF means no access right for PFMU */
	uint8_t   fgSU_MU;            /* 0 : SU, 1 : MU */
	uint8_t    u1TxBfCap;          /* 0 : ITxBf, 1 : ETxBf */
	uint8_t    ucSoundingPhy;      /* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
	uint8_t    ucNdpaRate;
	uint8_t    ucNdpRate;
	uint8_t    ucReptPollRate;
	uint8_t    ucTxMode;           /* 0: legacy, 1: OFDM, 2: HT, 4: VHT */
	uint8_t    ucNc;
	uint8_t    ucNr;
	uint8_t    ucCBW;              /* 0 : 20M, 1 : 40M, 2 : 80M, 3 : 80 + 80M */
	uint8_t    ucTotMemRequire;
	uint8_t    ucMemRequire20M;
	uint8_t    ucMemRow0;
	uint8_t    ucMemCol0;
	uint8_t    ucMemRow1;
	uint8_t    ucMemCol1;
	uint8_t    ucMemRow2;
	uint8_t    ucMemCol2;
	uint8_t    ucMemRow3;
	uint8_t    ucMemCol3;
	uint16_t   u2SmartAnt;
	uint8_t    ucSEIdx;
	uint8_t    ucAutoSoundingCtrl; /* Bit7: low traffic indicator, Bit6: Stop sounding for this entry, Bit5~0: postpone sounding */
	uint8_t    uciBfTimeOut;
	uint8_t    uciBfDBW;
	uint8_t    uciBfNcol;
	uint8_t    uciBfNrow;
	uint8_t    nr_bw160;
	uint8_t    nc_bw160;
	uint8_t    ru_start_idx;
	uint8_t    ru_end_idx;
	uint8_t    trigger_su;
	uint8_t    trigger_mu;
	uint8_t    ng16_su;
	uint8_t    ng16_mu;
	uint8_t    codebook42_su;
	uint8_t    codebook75_mu;
	uint8_t    he_ltf;
	uint8_t    u1Reserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_BF {
	uint16_t u2Tag;      /* Tag = 0x04 */
	uint16_t u2Length;
	struct UNI_CMD_TXBF_PFMU_STA_INFO  rTxBfPfmuInfo;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_HT_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2HtCap;
	uint16_t u2HtExtendedCap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_VHT_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4VhtCap;
	uint16_t u2VhtRxMcsMap;
	uint16_t u2VhtTxMcsMap;
	uint8_t	ucRTSBWSig; /* muru use only, ignored */
	uint8_t	aucReserve[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_PHY_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2BSSBasicRateSet;
	uint8_t ucDesiredPhyTypeSet;
	uint8_t ucAmpduParam;
	uint8_t ucRtsPolicy;
	uint8_t ucRCPI;
	uint8_t aucReserve[2];
} __KAL_ATTRIB_PACKED__;

/* Update RA Info */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_RA_INFO {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint16_t      u2DesiredNonHTRateSet;
	uint8_t       aucRxMcsBitmask[10];
} __KAL_ATTRIB_PACKED__;

/* Update BA_OFFLOAD Info */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_BA_OFFLOAD_INFO {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint8_t       ucTxAmpdu;
	uint8_t       ucRxAmpdu;
	uint8_t       ucTxAmsduInAmpdu;
	uint8_t       ucRxAmsduInAmpdu;
	uint32_t      u4TxMaxAmsduInAmpduLen;
	uint16_t      u2TxBaSize;
	uint16_t      u2RxBaSize;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_DEFAULT_KEY {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint8_t       ucKeyId;
	uint8_t       ucMulticast;
	uint8_t       aucReserve[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_HE_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t	    aucHeMacCapInfo[6];
	uint8_t	    aucHePhyCapInfo[11];
	uint8_t	    ucPktExt;
	/*0: BW80, 1: BW160, 2: BW8080*/
	uint16_t     au2RxMaxNssMcs[3];
} __KAL_ATTRIB_PACKED__;

/* Update HE 6g Info */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_HE_6G_CAP {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2He6gBandCapInfo;
	uint8_t aucReserve[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_AMSDU {
	uint16_t  u2Tag;		/* Tag = 0x05 */
	uint16_t  u2Length;
	uint8_t   ucMaxAmsduNum;
	uint8_t   ucMaxMpduSize;
    	uint8_t   ucAmsduEnable;
	uint8_t   acuReserve[1];
} __KAL_ATTRIB_PACKED__;

/* mld starec setup (Tag 0x20) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_MLD_SETUP {
	uint16_t  u2Tag;                 /* Tag = 0x20 */
	uint16_t  u2Length;
	uint8_t   aucPeerMldAddr[MAC_ADDR_LEN];
	uint16_t  u2PrimaryMldId;
	uint16_t  u2SecondMldId;
	uint16_t  u2SetupWlanId;
	uint8_t   ucLinkNumber;
	uint8_t   audPaddings[3];
	uint8_t   aucLinkInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_LINK_INFO {
	uint16_t  u2WlanIdx;
	uint8_t   ucBssIdx;
	uint8_t   ucTidBitmap;
} __KAL_ATTRIB_PACKED__;

/* starec MLD level information (Tag 0x21) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_EHT_MLD {
	uint16_t u2Tag;		/* Tag = 0x21 */
	uint16_t u2Length;
	uint8_t fgEPCS;
	uint8_t fgMldType;
	uint8_t ucReserved;
	/*
	 * pucStrBitmap[0] bit[0]: don't care
	 * pucStrBitmap[0] bit[1]: band0 and band1 str capability
	 * pucStrBitmap[0] bit[2]: band0 and band2 str capability
	 * pucStrBitmap[1] bit[0]: band1 and band0 str capability
	 * pucStrBitmap[1] bit[1]: don't care
	 * pucStrBitmap[1] bit[2]: band1 and band2 str capability
	 * pucStrBitmap[2] bit[0]: band2 and band0 str capability
	 * pucStrBitmap[2] bit[1]: band2 and band1 str capability
	 * pucStrBitmap[2] bit[2]: don't care
	 */
	uint8_t afgStrCapBitmap[UNI_MLD_LINK_MAX];
	uint8_t aucEmlCap[3];
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* starec link level EHT information (Tag 0x22) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_EHT_BASIC {
	uint16_t  u2Tag;		/* Tag = 0x22 */
	uint16_t  u2Length;
	uint8_t   aucPadding[2];
	uint16_t   u2EhtMacCap;
	uint64_t   u8EhtPhyCap; /* BIT0 ~ BIT63 */
	uint64_t   u8EhtPhyCapExt; /* BIT64 ~ BIT127 */
	uint8_t   aucMcsMap20MHzSta[4];
	uint8_t   aucMcsMap80MHz[3];
	uint8_t   aucMcsMap160MHz[3];
	uint8_t   aucMcsMap320MHz[3];
	uint8_t   aucPaddings[3];
} __KAL_ATTRIB_PACKED__;

struct BFEE_STA_REC {
	uint8_t   fgFbIdentityMatrix;
	uint8_t   fgIgnFbk;
	uint8_t   fgRxsmmEnable;
};

/* starec link level BFee information (Tag 0x14) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_BFEE {
	uint16_t  u2Tag;
	uint16_t  u2Length;
	struct BFEE_STA_REC  rBfeeStaRec;
	uint8_t   aucPaddings[1];
} __KAL_ATTRIB_PACKED__;

/* MLD STAREC teardown (Tag 0x23) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_MLD_TEARDOWN {
	uint16_t  u2Tag;                 /* Tag = 0x23 */
	uint16_t  u2Length;
} __KAL_ATTRIB_PACKED__;

/* Update UAPSD Info */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_UAPSD_INFO {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint8_t       fgIsUapsdSupported;
	uint8_t       ucUapsdAc;
	uint8_t       ucUapsdSp;
	uint8_t       aucReserve[1];
} __KAL_ATTRIB_PACKED__;

#define UNI_CMD_STAREC_FASTALL_FLAG_UPDATE_BAND BIT(0)

/* Update all sta info (Tag 0x2C) */
struct UNI_CMD_STAREC_FASTALL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t ucUpdateFlag;
	uint8_t aucReserve[16];
};

/* Remove STAREC */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_REMOVE_INFO {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint8_t       ucActionType; /* ENUM_STA_REC_CMD_ACTION_T */
	uint8_t       aucReserve[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_INSTALL_KEY3 {
	uint16_t      u2Tag;
	uint16_t      u2Length;
	uint8_t       ucAddRemove;
	uint8_t       ucTxKey;
	uint8_t       ucKeyType;
	uint8_t       ucIsAuthenticator;
	uint8_t       aucPeerAddr[6];
	uint8_t       ucBssIdx;
	uint8_t       ucAlgorithmId;
	uint8_t       ucKeyId;
	uint8_t       ucKeyLen;
	uint8_t       ucWlanIndex;
	uint8_t       ucMgmtProtection;
	uint8_t       aucKeyMaterial[32];
	uint8_t       aucKeyRsc[16];
} __KAL_ATTRIB_PACKED__;

/* STAREC get PN (Tag 0x26) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_GET_PN {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucPn[6];
	uint8_t ucTscType; /* 0:GTK_PN, 1:IPN, 2:BIPN */
	uint8_t aucReserved;
} __KAL_ATTRIB_PACKED__;

/* MLR information (Tag 0x2d) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_MLR_INFO {
	uint16_t  u2Tag;		/* Tag = 0x2d */
	uint16_t  u2Length;
	uint8_t   ucMlrMode;
	uint8_t   ucMlrState;
	/* MLR state force mode(0:don't care/1:IDLE/2:START)
	 * only used for REBB segment
	 */
	uint8_t   ucMlrStateForceType;
	uint8_t   aucReserved[1];
} __KAL_ATTRIB_PACKED__;

/* NAN information (Tag 0x2e) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_NAN {
	uint16_t  u2Tag;		/* Tag = 0x2e */
	uint16_t  u2Length;
	uint8_t   ucOtherWlanIndex;
	uint8_t   aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* t2lm (Tag 0x3E) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_T2LM {
	uint16_t  u2Tag;                 /* Tag = 0x3E */
	uint16_t  u2Length;
	uint8_t   ucLinkNumber;
	uint8_t   audPaddings[3];
	uint8_t   aucLinkInfo[];
} __KAL_ATTRIB_PACKED__;

/* MLD STAREC ml reconfig state (Tag 0x3F) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_MLD_RECFG_STATE {
	uint16_t u2Tag;                 /* Tag = 0x3F */
	uint16_t u2Length;
	uint8_t ucLinkNumber;
	uint8_t audPaddings[3];
	uint8_t aucLinkInfo[0];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_RECFG_LINK_INFO {
	uint16_t  u2WlanIdx;
	uint8_t   ucBssIdx;
	uint8_t   fgApRemoval;
} __KAL_ATTRIB_PACKED__;

/* LTF keyseed (Tag 0x40) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_INSTALL_LTE_KEYSEED {
	uint16_t  u2Tag;                 /* Tag = 0x40 */
	uint16_t  u2Length;
	uint8_t   ucAddRemove;
	uint8_t   ucLtfKeyseedLen;
	uint16_t  u2WlanIdx;
	uint8_t   aucLtfKeyseed[48];
} __KAL_ATTRIB_PACKED__;

/* starec MLD level information v2 (Tag 0x49) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STAREC_EHT_MLD_V2 {
	uint16_t u2Tag;		/* Tag = 0x49 */
	uint16_t u2Length;
	uint16_t u2ExtMldCap;
	uint8_t aucReserved[14];
} __KAL_ATTRIB_PACKED__;

/* EDCA set command (0x04) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EDCA {
	/* fixed field */
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* EDCA set command Tag */
enum ENUM_UNI_CMD_EDCA_TAG {
	UNI_CMD_EDCA_TAG_AC_PARM = 0,
	UNI_CMD_EDCA_TAG_NUM
};

/* EDCA AC Parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EDCA_AC_PARM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucAcIndex;
	uint8_t ucValidBitmap;
	/**<
	  *      Define            | BIT | Note              |
	  *      ------------------|-----|------------------ |
	  *      MASK_AIFS_SET     | 0   | 0x01, AIFSN       |
	  *      MASK_WINMIN_SET   | 1   | 0x02, CW min      |
	  *      MASK_WINMAX_SET   | 2   | 0x04, CW max      |
	  *      MASK_TXOP_SET     | 3   | 0x08, TXOP Limit  |
	  */
	uint8_t ucCWmin;
	uint8_t ucCWmax;
	uint16_t u2TxopLimit;
	uint8_t ucAifsn;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SUSPEND {
	/*fixed field*/
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                             | ID  | structure
	*   -------------                   | ----| -------------
	*   UNI_CMD_SUSPEND_MODE_SETTING    | 0x0 | UNI_CMD_SUSPEND_MODE_SETTING_T
	*   UNI_CMD_SUSPEND_WOW_CTRL        | 0x1 | UNI_CMD_SUSPEND_WOW_CTRL_T
	*   UNI_CMD_SUSPEND_WOW_GPIO_PARAM  | 0x2 | UNI_CMD_SUSPEND_WOW_GPIO_PARAM_T
	*   UNI_CMD_SUSPEND_WOW_WAKEUP_PORT | 0x3 | UNI_CMD_SUSPEND_WOW_WAKEUP_PORT_T
	*   UNI_CMD_SUSPEND_WOW_PATTERN     | 0x4 | UNI_CMD_SUSPEND_WOW_PATTERN_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Suspend command Tag */
enum ENUM_UNI_CMD_SUSPEND_TAG {
	UNI_CMD_SUSPEND_TAG_MODE_SETTING = 0,
	UNI_CMD_SUSPEND_TAG_WOW_CTRL = 1,
	UNI_CMD_SUSPEND_TAG_WOW_GPIO_PARAM = 2,
	UNI_CMD_SUSPEND_TAG_WOW_WAKEUP_PORT = 3,
	UNI_CMD_SUSPEND_TAG_WOW_PATTERN = 4,
	UNI_CMD_SUSPEND_TAG_NUM
};

/* suspend mode setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SUSPEND_MODE_SETTING {
	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;

	uint8_t ucScreenStatus;
	uint8_t ucMdtim;
	uint8_t ucWowSuspend;
	uint8_t aucPadding[1];

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SUSPEND_WOW_CTRL {
	uint16_t u2Tag;                   // Tag = 0x01
	uint16_t u2Length;

	uint8_t ucCmd;
	uint8_t ucDetectType;
	uint8_t ucWakeupHif;
	uint8_t aucPadding[1];
	uint16_t u2DetectTypeExt;

	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SUSPEND_WOW_GPIO_PARAM {
	uint16_t u2Tag;                   // Tag = 0x02
	uint16_t u2Length;

	uint8_t ucGpioPin;
	uint8_t ucTriggerLvl;
	uint8_t aucPadding[2];

	uint32_t u4GpioInterval;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SUSPEND_WOW_WAKEUP_PORT {
	uint16_t u2Tag;                   // Tag = 0x03
	uint16_t u2Length;

	uint8_t ucIPv4UdpPortCnt;
	uint8_t ucIPv4TcpPortCnt;
	uint8_t ucIPv6UdpPortCnt;
	uint8_t ucIPv6TcpPortCnt;

	uint16_t ausIPv4UdpPort[UNI_CMD_MAX_TCP_UDP_PORT];
	uint16_t ausIPv4TcpPort[UNI_CMD_MAX_TCP_UDP_PORT];
	uint16_t ausIPv6UdpPort[UNI_CMD_MAX_TCP_UDP_PORT];
	uint16_t ausIPv6TcpPort[UNI_CMD_MAX_TCP_UDP_PORT];

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

struct UNI_CMD_OFFLOAD {
	/*fixed field*/
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
        *
        *   TAG                             | ID  | structure
        *   -------------                   | ----| -------------
        *   UNI_CMD_OFFLOAD_ARPNS_IPV4      | 0x0 | UNI_CMD_OFFLOAD_ARPNS_IPV4_T
        *   UNI_CMD_OFFLOAD_ARPNS_IPV6      | 0x1 | UNI_CMD_OFFLOAD_ARPNS_IPV6_T
        *   UNI_CMD_OFFLOAD_GTK_REKEY       | 0x2 | UNI_CMD_OFFLOAD_GTK_REKEY_T
        *   UNI_CMD_OFFLOAD_BMC_RPY_DETECT  | 0x3 | UNI_CMD_OFFLOAD_BMC_RPY_DETECT_T
        */
};

enum ENUM_UNI_CMD_OFFLOAD_TAG {
	UNI_CMD_OFFLOAD_TAG_ARPNS_IPV4 = 0,
	UNI_CMD_OFFLOAD_TAG_ARPNS_IPV6 = 1,
	UNI_CMD_OFFLOAD_TAG_GTK_REKEY = 2,
	UNI_CMD_OFFLOAD_TAG_BMC_RPY_DETECT = 3,
	UNI_CMD_OFFLOAD_TAG_NUM
};

struct IPV4_ADDRESS_V0 {
	uint8_t aucIpAddr[IPV4_ADDR_LEN];
};

struct IPV4_ADDRESS {
	uint8_t aucIpAddr[IPV4_ADDR_LEN];
	uint8_t aucIpMask[IPV4_ADDR_LEN];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_OFFLOAD_ARPNS_IPV4 {
	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;

	uint8_t ucEnable;
	uint8_t ucIpv4AddressCount;
	uint8_t ucVersion;
	uint8_t aucPadding[1];

	struct IPV4_ADDRESS arIpv4NetAddress[];
} __KAL_ATTRIB_PACKED__;

struct IPV6_ADDRESS {
	uint8_t aucIpAddr[IPV6_ADDR_LEN];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_OFFLOAD_ARPNS_IPV6 {
	uint16_t u2Tag;                   // Tag = 0x01
	uint16_t u2Length;

	uint8_t ucEnable;
	uint8_t ucIpv6AddressCount;
	uint8_t aucPadding[2];

	struct IPV6_ADDRESS arIpv6NetAddress[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_OFFLOAD_GTK_REKEY {
	uint16_t u2Tag;                   // Tag = 0x02
	uint16_t u2Length;

	uint8_t aucKek[16];
	uint8_t aucKck[16];
	uint8_t aucReplayCtr[8];

	uint8_t ucRekeyMode;
	uint8_t ucCurKeyId;
	uint8_t ucOption;
	uint8_t aucPadding[1];

	uint32_t u4Proto;
	uint32_t u4PairwiseCipher;
	uint32_t u4GroupCipher;
	uint32_t u4KeyMgmt;
	uint32_t u4MgmtGroupCipher;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_OFFLOAD_BMC_RPY_DETECT {
	uint16_t u2Tag;                   // Tag = 0x03
	uint16_t u2Length;

	uint8_t aucBMCOffloadKeyRscPN[8];

	uint8_t ucMode;
	uint8_t ucKeyId;
	uint8_t aucPadding[2];

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_HIF_CTRL {
	/*fixed field*/
	uint8_t ucHifType;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                             | ID  | structure
	*   -------------                   | ----| -------------
	*   UNI_CMD_HIF_CTRL_BASIC          | 0x0 | UNI_CMD_HIF_CTRL_BASIC_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Suspend command Tag */
enum ENUM_UNI_CMD_HIF_CTRL_TAG {
	UNI_CMD_HIF_CTRL_TAG_BASIC = 0,
	UNI_CMD_HIF_CTRL_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_HIF_CTRL_BASIC {
	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;

	uint8_t ucHifSuspend;
	uint8_t aucPadding[3];

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG {
	/*fixed field*/
	uint8_t ucDbdcIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   -------------                    | ----| -------------
        *   UNI_CMD_BAND_CONFIG_RADIO_ONOFF    | 0x0 | UNI_CMD_BAND_CONFIG_RADIO_ONOFF_T
        *   UNI_CMD_BAND_CONFIG_RXV_CTRL       | 0x1 | UNI_CMD_BAND_CONFIG_RXV_CTRL_T
        *   UNI_CMD_BAND_CONFIG_SET_RX_FILTER  | 0x2 | UNI_CMD_BAND_CONFIG_SET_RX_FILTER_T
        *   UNI_CMD_BAND_CONFIG_DROP_CTRL_FRAME| 0x3 | UNI_CMD_BAND_CONFIG_DROP_CTRL_FRAME_T
        *   UNI_CMD_BAND_CONFIG_AGG_AC_LIMIT   | 0x4 | UNI_CMD_BAND_CONFIG_AGG_AC_LIMIT_T
        *   UNI_CMD_BAND_CONFIG_EDCCA_ENABLE   | 0x5 | UNI_CMD_BAND_CONFIG_EDCCA_ENABLE_CTRL_T
        *   UNI_CMD_BAND_CONFIG_EDCCA_THRESHOLD| 0x6 | UNI_CMD_BAND_CONFIG_EDCCA_THRESHOLD_CTRL_T
        */
} __KAL_ATTRIB_PACKED__;

/* Band config Tag */
enum ENUM_UNI_CMD_BAND_CONFIG_TAG {
	UNI_CMD_BAND_CONFIG_TAG_RADIO_ONOFF = 0,
	UNI_CMD_BAND_CONFIG_TAG_RXV_CTRL = 1,
	UNI_CMD_BAND_CONFIG_TAG_SET_RX_FILTER = 2,
	UNI_CMD_BAND_CONFIG_TAG_DROP_CTRL_FRAME = 3,
	UNI_CMD_BAND_CONFIG_TAG_AGG_AC_LIMIT = 4,
	UNI_CMD_BAND_CONFIG_TAG_EDCCA_ENABLE = 5,
	UNI_CMD_BAND_CONFIG_TAG_EDCCA_THRESHOLD = 6,
	UNI_CMD_BAND_CONFIG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_RADIO_ONOFF {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgRadioOn;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_RXV_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucRxvOfRxEnable;
	uint8_t  ucRxvOfTxEnable;
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_SET_RX_FILTER {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4RxPacketFilter;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_DROP_CTRL_FRAME {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucDropRts;
	uint8_t ucDropCts;
	uint8_t ucDropUnwantedCtrl;
	uint8_t aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_AGG_AC_LIMIT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucWmmIdx;
	uint8_t ucAc;
	uint16_t u2AggLimit;
} __KAL_ATTRIB_PACKED__;

/* EDCCA OnOff Control (Tag5) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_EDCCA_ENABLE_CTRL {
	uint16_t u2Tag;    // Tag = 0x05
	uint16_t u2Length;
	uint8_t fgEDCCAEnable;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* EDCCA Threshold Control (Tag6) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BAND_CONFIG_EDCCA_THRESHOLD_CTRL {
	uint16_t u2Tag;    // Tag = 0x06
	uint16_t u2Length;
	uint8_t u1EDCCAThreshold[3];
	uint8_t fginit;
} __KAL_ATTRIB_PACKED__;

/* MUAR set command (0x0A) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MUAR {
	/* fixed field */
	uint8_t ucBand;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Muar command Tag */
enum UNI_CMD_MUAR_TAG {
	UNI_CMD_MUAR_TAG_CLEAN = 0,
	UNI_CMD_MUAR_TAG_MC_FILTER = 1,
	UNI_CMD_MUAR_TAG_ENTRY = 2,
	UNI_CMD_MUAR_TAG_NUM
};

/* MUAR Config Parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MUAR_CLEAN_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucHwBssIndex;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* MC Config Parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MC_FILTER_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucBssIndex;
	uint8_t  ucByPassMcTblCheck;
	uint8_t  ucNormalMode;
	uint8_t  ucScreenOffMode;
	uint8_t  ucDeviceSuspendMode;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* MC Config Parameters (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MUAR_ENTRY_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgSmesh;
	uint8_t ucHwBssIndex;
	uint8_t ucMuarIdx;
	uint8_t ucEntryAdd;
	uint8_t aucMacAddr[6];
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* WSYS Config set command (0x0B) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_WSYS_CONFIG {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   UNI_CMD_WSYS_CONFIG_FW_LOG_CTRL  | 0x0 | UNI_CMD_FW_LOG_CTRL_BASIC_T
	*   UNI_CMD_WSYS_CONFIG_FW_DBG_CTRL  | 0x1 | UNI_CMD_FW_DBG_CTRL_T
	*   UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL  | 0x2 | UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL_T
	*   UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG | 0x3 | UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG_T
	*   UNI_CMD_HOSTREPORT_TX_LATENCY_CONFIG| 0x4 | UNI_CMD_WSYS_CONFIG_HOSTREPORT_TX_LATENCY_T
	*   UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL | 0x5 | UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL_T
	*/
}  __KAL_ATTRIB_PACKED__;

/* WSYS Config set command TLV List */
enum ENUM_UNI_CMD_WSYS_CONFIG_TAG {
	UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_CTRL = 0,
	UNI_CMD_WSYS_CONFIG_TAG_FW_DBG_CTRL = 1,
	UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_UI_CTRL = 2,
	UNI_CMD_WSYS_CONFIG_TAG_FW_BASIC_CONFIG = 3,
	UNI_CMD_WSYS_CONFIG_TAG_HOSTREPORT_TX_LATENCY_CONFIG = 4,
	UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_BUFFER_CTRL = 5,
	UNI_CMD_WSYS_CONFIG_TAG_NUM
};

/* FW Log Basic Setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FW_LOG_CTRL_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucFwLog2HostCtrl;
	uint8_t ucFwLog2HostInterval;
	/**<
	  *      Time takes effect only if these conditions are true:
	  *      1. FW log destinations include host
	  *      2. ucFwLog2HostInterval > 0 (Unit: second)
	  */
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* FW Log UI Setting (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t ucVersion; // default is 1
	uint32_t ucLogLevel;// 0: Default, 1: More, 2: Extreme
	uint8_t  aucReserved[4];
} __KAL_ATTRIB_PACKED__;

/* FW Debug Level Setting (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2RxChecksum;   /* bit0: IP, bit1: UDP, bit2: TCP */
	uint16_t u2TxChecksum;   /* bit0: IP, bit1: UDP, bit2: TCP */
	uint8_t ucCtrlFlagAssertPath;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* FW Log Buffer Control Setting (Tag5) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4MCUAddr;
	uint32_t u4WFAddr;
	uint32_t u4BTAddr;
	uint32_t u4GPSAddr;
	/*
	 * BIT[0]:Get Log buffer control block base address
	 * BIT[1~4]: update MCU/WiFi/BT/GPS read pointer
	 */
	uint8_t ucType;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ROAMING {
	/* fixed field */
	uint8_t ucBssInfoIdx;
	uint8_t ucDbdcIdx;
	uint8_t aucReserved[2];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**<the TLVs includer in this field:
        *
        *  TAG                                | ID   | structure
        *  -------------                      | -----| -------------
        *  UNI_CMD_ROAMING_TRANSIT_FSM        | 0x0  | UNI_CMD_ROAMING_TRANSIT_FSM_T
        */
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_ROAMING_TAG {
	UNI_CMD_ROAMING_TAG_TRANSIT_FSM = 0,
	UNI_CMD_ROAMING_TAG_NUM
};

/* Show roaming (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ROAMING_TRANSIT_FSM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2Event;
	uint16_t u2Data;
	uint32_t eReason; /*ENUM_ROAMING_REASON_T*/
	uint32_t u4RoamingTriggerTime;
	uint16_t u2RcpiLowThreshold;
	u_int8_t fgIsAggressive;
	uint8_t  aucPadding[1];
} __KAL_ATTRIB_PACKED__;

/* register access command (0x0D) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACCESS_REG {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   ---------------------------------|-----|--------------
        *   UNI_CMD_ACCESS_REG_BASIC         | 0x0 | UNI_CMD_ACCESS_REG_BASIC_T
        *   UNI_CMD_ACCESS_RF_REG_BASIC      | 0x1 | UNI_CMD_ACCESS_RF_REG_BASIC_T
        */
} __KAL_ATTRIB_PACKED__;

/* Register access command TLV List */
enum ENUM_UNI_CMD_ACCESS_REG_TAG {
	UNI_CMD_ACCESS_REG_TAG_BASIC = 0,
	UNI_CMD_ACCESS_REG_TAG_RF_REG_BASIC,
	UNI_CMD_ACCESS_REG_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACCESS_REG_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Addr;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACCESS_RF_REG_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WifiStream;
	uint16_t u2Reserved;
	uint32_t u4Addr;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

/* Chip Config set command (0x0E) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                                    | ID  | structure
        *   ---------------------------------------|-----|--------------
        *   UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL        | 0x0 | UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL_T
        *   UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG       | 0x1 | UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG_T
        *   UNI_CMD_CHIP_CONFIG_CHIP_CFG           | 0x2 | UNI_CMD_CHIP_CONFIG_CHIP_CFG_T
        *   UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY     | 0x3 | UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY_T
        */
} __KAL_ATTRIB_PACKED__;

/* Chip Config set command TLV List */
enum ENUM_UNI_CMD_CHIP_CONFIG_TAG {
	UNI_CMD_CHIP_CONFIG_TAG_SW_DBG_CTRL = 0,
	UNI_CMD_CHIP_CONFIG_TAG_CUSTOMER_CFG = 1,
	UNI_CMD_CHIP_CONFIG_TAG_CHIP_CFG = 2,
	UNI_CMD_CHIP_CONFIG_TAG_NIC_CAPABILITY = 3,
	UNI_CMD_CHIP_CONFIG_TAG_NUM
};

/* SW DBG control Setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL {
	uint16_t u2Tag; // Tag = 0x00
	uint16_t u2Length;
	uint32_t u4Id;
	uint32_t u4Data;
}__KAL_ATTRIB_PACKED__;

/* Customer Configuration Setting (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t cmdBufferLen;
	uint8_t itemNum;
	uint8_t aucPadding[1];
	uint8_t aucbuffer[MAX_CMD_BUFFER_LENGTH];
}__KAL_ATTRIB_PACKED__;

/* get or set chip configuration (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG_CHIP_CFG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucbuffer[];
}__KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP {
	uint16_t u2Id;
	uint8_t ucType;
	uint8_t ucRespType;
	uint16_t u2MsgSize;
	uint8_t aucReserved0[2];
	uint8_t aucCmd[UNICMD_CHIP_CONFIG_RESP_SIZE];
}__KAL_ATTRIB_PACKED__;

/* Get Connsys Cpability (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY {
	uint16_t u2Tag;
	uint16_t u2Length;
}__KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_POWER_CTRL {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                      | ID  | structure
	*   -------------------------|-----|--------------
	*   UNI_CMD_POWER_OFF        | 0x0 | UNI_CMD_POWER_OFF_T
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_POWER_CTRL_TAG {
	UNI_CMD_POWER_CTRL_TAG_OFF = 0,
	UNI_CMD_POWER_CTRL_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_POWER_OFF {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPowerMode;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* RRM_11K command (0x11) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RRM_11K {
	/* fixed field */
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**<the TLVs includer in this field:
	*
	*  TAG					| ID   | structure
	*  ------------------------------------ | -----| -------------
	*  UNI_CMD_SET_RRM_CAPABILITY		| 0x0  | UNI_CMD_SET_RRM_CAPABILITY_T
	*  UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT	| 0x1  | UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT_T
	*/
} __KAL_ATTRIB_PACKED__;

/* RRM 11K command Tag */
enum ENUM_UNI_CMD_RRM_11K_TAG {
	UNI_CMD_SET_RRM_CAPABILITY = 0,
	UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT =  1,
	UNI_CMD_RRM_11K_MAX_NUM
};

/* RRM capability Information (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_RRM_CAPABILITY_PARAM {
	/* Tag = 0x00 */
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t  ucRrmEnable;
	uint8_t  ucCapabilities[5];
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* AP constraint power limit Information (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT_PARAM {
	/* Tag = 0x01 */
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucPwrSetEnable;
	int8_t cMaxTxPwr;
	int8_t cMinTxPwr;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

/* Rx header translation command (0x12) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**<the TLVs includer in this field:
        *
        *  TAG                    | ID   | structure
        *  -------------          | -----| -------------
        *  UNI_CMD_SER_QUERY      | 0x0  | UNI_CMD_SER_QUERY_T
        *  UNI_CMD_SER_ENABLE     | 0x1  | UNI_CMD_SER_ENABLE_T
        *  UNI_CMD_SER_SET        | 0x2  | UNI_CMD_SER_SET_T
        *  UNI_CMD_SER_TRIGGER    | 0x3  | UNI_CMD_SER_TRIGGER_T
        */
} __KAL_ATTRIB_PACKED__;

/* SER command TLV List */
enum ENUM_UNI_CMD_SER_TAG {
	UNI_CMD_SER_TAG_QUERY = 0,
	UNI_CMD_SER_TAG_ENABLE = 1,
	UNI_CMD_SER_TAG_SET = 2,
	UNI_CMD_SER_TAG_TRIGGER = 3,
	UNI_CMD_SER_TAG_L0P5_CTRL = 4,
	UNI_CMD_SER_TAG_NUM
};

/* Show ser (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER_QUERY {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* Enable/disable ser (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER_ENABLE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgEnable;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* config ser (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER_SET {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4EnableMask;
} __KAL_ATTRIB_PACKED__;

/* trigger ser recovery (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER_TRIGGER {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucTriggerMethod;
	uint8_t ucDbdcIdx;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SER_L05 {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucCtrlAction;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TWT {
	/*fixed field*/
	uint8_t ucBssInfoIdx;
	uint8_t aucPadding[3];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*  TAG                         | ID   | structure
	*  -------------               | -----| -------------
	*  UNI_CMD_TWT_AGRT_UPDATE       | 0x0  | UNI_CMD_TWT_ARGT_UPDATE_T
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_TWT_TAG {
	UNI_CMD_TWT_TAG_AGRT_UPDATE = 0,
	UNI_CMD_TWT_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TWT_ARGT_UPDATE {
	uint16_t    u2Tag;    /* Tag = 0x00 */
	uint16_t    u2Length;
	uint8_t      ucAgrtTblIdx;
	uint8_t      ucAgrtCtrlFlag;
	uint8_t      ucOwnMacId;
	uint8_t      ucFlowId;
	uint16_t     u2PeerIdGrpId;
	uint8_t      ucAgrtSpDuration;
	uint8_t      ucBssIndex;
	uint32_t     u4AgrtSpStartTsf_low;
	uint32_t     u4AgrtSpStartTsf_high;
	uint16_t     u2AgrtSpWakeIntvlMantissa;
	uint8_t      ucAgrtSpWakeIntvlExponent;
	uint8_t      fgIsRoleAp;
	uint8_t      ucAgrtParaBitmap;
	uint8_t      ucReserved_a;
	uint16_t     u2Reserved_b;
	uint8_t      ucGrpMemberCnt;
	uint8_t      ucReserved_c;
	uint16_t     u2Reserved_d;
#if 0
	uint16_t     au2StaList[UNI_TWT_GRP_MAX_MEMBER_CNT];
#endif
#if (CFG_SUPPORT_RTWT == 1)
	/* DW7 RTWT traffic info */
	uint8_t ucTrafficInfoPresent;
	uint8_t ucDlUlBmpValid;
	uint8_t ucDlBmp;
	uint8_t ucUlBmp;
#endif
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DOMAIN_SET_INFO {
	/* fixed field */
	uint32_t u4CountryCode;
	uint8_t  uc2G4Bandwidth; /* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t  uc5GBandwidth;  /* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t  uc6GBandwidth;
	uint8_t  ucRegCat;
	/* tlv */
	uint8_t  aucTlvBuffer[];
	/**< the TLVs included in this field:
	*
	*   TAG                          | ID   | structure
	*   -------------                | ---- | -------------
	*   ENUM_UNI_CMD_DOMAIN_SUBBAND  | 0x01 | UNI_CMD_DOMAIN_SET_INFO_DOMAIN_SUBBAND_T
	*   ENUM_UNI_CMD_DOMAIN_ACTCHNL  | 0x02 | UNI_CMD_DOMAIN_SET_INFO_DOMAIN_ACTIVE_CHANNEL_LIST_T
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_DOMAIN_TAG {
	UNI_CMD_DOMAIN_TAG_SUBBAND = 1,
	UNI_CMD_DOMAIN_TAG_ACTCHNL = 2,
	UNI_CMD_DOMAIN_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_SUBBAND {
	uint16_t u2Tag;                   // Tag = 0x01
	uint16_t u2Length;
	uint16_t u2IsSetPassiveScan;
	uint8_t  aucReserved[1];
	uint8_t  ucSubBandNum;
	uint8_t  aucSubBandInfoBuffer[]; // UNI_CMD_DOMAIN_SUBBAND_INFO
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DOMAIN_SUBBAND_INFO {
	uint8_t ucRegClass;
	uint8_t ucBand;
	uint8_t ucChannelSpan;
	uint8_t ucFirstChannelNum;
	uint8_t ucNumChannels;
	uint8_t fgDfs;         /* Type: BOOLEAN (fgDfsNeeded) */
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_ACTIVE_CHANNEL_LIST {
	uint16_t u2Tag;                   // Tag = 0x02
	uint16_t u2Length;
	uint8_t  u1ActiveChNum2g;
	uint8_t  u1ActiveChNum5g;
	uint8_t  u1ActiveChNum6g;
	uint8_t  aucReserved[1];
	uint8_t  aucActChnlListBuffer[]; // DOMAIN_CHANNEL_T
} __KAL_ATTRIB_PACKED__;

/* IDC command (0x17) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_IDC {
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *   TAG                        | ID  | structure
        *   ---------------------------|-----|--------------
        *    UNI_CMD_ID_GET_IDC_CHN                              | 0x00 | UNI_CMD_GET_IDC_CHN_T
        *    UNI_CMD_ID_ALWAYS_SCAN_PARAM_SETTING     | 0x01 | UNI_CMD_ALWAYS_SCAN_PARAM_SETTING_T
        *    UNI_CMD_ID_CCCI_MSG                               | 0x02 | UNI_CMD_CCCI_MSG_T
        *    UNI_CMD_ID_3WIRE_GROUP                        | 0x03 | UNI_CMD_3WIRE_GROUP_T
        */

} __KAL_ATTRIB_PACKED__;

/* IDC config Tag */
enum ENUM_UNI_CMD_IDC_TAG {
	UNI_CMD_IDC_TAG_GET_IDC_CHN = 0,
	UNI_CMD_IDC_TAG_ALWAYS_SCAN_PARAM_SETTING = 1,
	UNI_CMD_IDC_TAG_CCCI_MSG = 2,
	UNI_CMD_IDC_TAG_3WIRE_GROUP = 3,
#if CFG_SUPPORT_IDC_RIL_BRIDGE
	UNI_CMD_IDC_TAG_RIL_BRIDGE = 4,
#endif
	UNI_CMD_IDC_TAG_NUM
};

/* IDC Setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_GET_IDC_CHN {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* IDC Setting (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ALWAYS_SCAN_PARAM_SETTING {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint8_t  fgAlwaysScanEnable;
	uint8_t     aucReserved[3];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_IDC_RIL_BRIDGE
/* IDC Setting (Tag4) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RIL_BRIDGE {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint8_t    ucRat; /* LTE or NR */
	uint8_t    fgIsChannelSelectByAcs;
	uint8_t    aucReserved1[2];
	uint32_t   u4Band;
	uint32_t   u4Channel;
} __KAL_ATTRIB_PACKED__;
#endif

#if CFG_SUPPORT_UWB_COEX
/* UWB COEX command (0x75) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UWB_COEX {
	uint8_t ucReserved[4];
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* UWB COEX config Tag */
enum ENUM_UNI_CMD_UWB_COEX_TAG {
	UNI_CMD_UWB_COEX_TAG_ENABLE = 0,
	UNI_CMD_UWB_COEX_TAG_SET_PREPARE_TIME = 1,
	UNI_CMD_UWB_COEX_TAG_NUM
};

/* UWB COEX Setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UWB_COEX_ENABLE {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint32_t   u4Enable;
	uint32_t   u4StartCh;
	uint32_t   u4EndCh;
} __KAL_ATTRIB_PACKED__;

/* UWB COEX Setting (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UWB_COEX_PREPARE {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint32_t   u4Time;
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN {
	/* fixed field */
	uint8_t ucSeqNum;
	uint8_t ucBssIndex;
	uint8_t aucPadding[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

typedef uint16_t(*PFN_UNI_CMD_SCAN_TAG_HANDLER) (struct ADAPTER
	*ad, uint8_t *buf, struct CMD_SCAN_REQ_V2 *cmd);

struct UNI_CMD_SCAN_TAG_HANDLE {
	uint32_t u4Size;
	PFN_UNI_CMD_SCAN_TAG_HANDLER pfHandler;
};

typedef uint16_t(*PFN_UNI_CMD_SCHED_SCAN_TAG_HANDLER) (struct ADAPTER
	*ad, uint8_t *buf, struct CMD_SCHED_SCAN_REQ *cmd);

struct UNI_CMD_SCHED_SCAN_TAG_HANDLE {
	uint32_t u4Size;
	PFN_UNI_CMD_SCHED_SCAN_TAG_HANDLER pfHandler;
};

enum ENUM_UNI_CMD_SCAN_TAG {
	UNI_CMD_SCAN_TAG_SCAN_REQ             = 1,
	UNI_CMD_SCAN_TAG_SCAN_CANCEL          = 2,
	UNI_CMD_SCAN_TAG_SCHED_SCAN_REQ       = 3,
	UNI_CMD_SCAN_TAG_SCHED_SCAN_ENABLE    = 4,
	/** Reserved for Sched Scan / Custom Scan */

	UNI_CMD_SCAN_TAG_SCAN_SSID            = 10,
	UNI_CMD_SCAN_TAG_SCAN_BSSID           = 11,
	UNI_CMD_SCAN_TAG_SCAN_CHANNEL         = 12,
	UNI_CMD_SCAN_TAG_SCAN_IE              = 13,
	UNI_CMD_SCAN_TAG_SCAN_MISC            = 14,
	UNI_CMD_SCAN_TAG_SCAN_SSID_MATCH_SETS = 15,

	UNI_CMD_SCAN_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_REQ {
	uint16_t u2Tag;                   // Tag = 0x01
	uint16_t u2Length;

	uint8_t ucScanType;
	uint8_t ucNumProbeReq;
	uint8_t ucScnFuncMask;
	uint8_t ucScnSourceMask;
	uint16_t u2ChannelMinDwellTime;
	uint16_t u2ChannelDwellTime;
	uint16_t u2TimeoutValue;
	uint16_t u2ProbeDelayTime;
	uint32_t u4ScnFuncMaskExtend;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_CANCEL {
	uint16_t u2Tag;                   // Tag = 0x02
	uint16_t u2Length;

	uint8_t ucIsExtChannel;     /* For P2P channel extension. */
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_SSID {
	uint16_t u2Tag;                   // Tag = 0x0a
	uint16_t u2Length;

	uint8_t ucSSIDType;
	uint8_t ucSSIDNum;
	uint8_t ucIsShortSSID;
	uint8_t ucReserved;
	uint8_t aucSsidBuffer[]; // PARAM_SSID_T
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_BSSID {
	uint16_t u2Tag;                   // Tag = 0x0b
	uint16_t u2Length;

	uint8_t aucBssid[MAC_ADDR_LEN];
	uint8_t ucBssidMatchCh;
	uint8_t ucBssidMatchSsidInd;
	uint8_t ucRcpi;
	uint8_t ucBssidMatchShortSsidInd;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_CHANNEL_INFO {
	uint16_t u2Tag;                   // Tag = 0x0c
	uint16_t u2Length;

	uint8_t ucChannelType;
	uint8_t ucChannelListNum;
	uint8_t aucPadding[2];
	uint8_t aucChnlInfoBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_IE {
	uint16_t u2Tag;                   // Tag = 0x0d
	uint16_t u2Length;

	uint16_t u2IELen;
	uint8_t  ucBand;
	uint8_t  ucPadding;
	uint8_t  aucIEBuffer[];  //depends on u2IELen
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_MISC {
	uint16_t u2Tag;                   // Tag = 0x0e
	uint16_t u2Length;

	uint8_t aucRandomMac[MAC_ADDR_LEN];
	uint8_t ucShortSSIDNum;
	uint8_t ucPerScanChannelCnt;
	uint8_t ucDfsChDwellTimeMs;
	uint16_t u2OpChStayTimeMs;
	uint8_t aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_SCHED_SCAN_REQ {
	uint16_t u2Tag;                   // Tag = 0x03
	uint16_t u2Length;

	uint8_t  ucVersion;
	uint8_t  fgStopAfterIndication;
	uint8_t  ucMspEntryNum;
	uint8_t  ucScnFuncMask;
	uint16_t au2MspList[10];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_SCHED_SCAN_ENABLE {
	uint16_t u2Tag;                   // Tag = 0x04
	uint16_t u2Length;

	uint8_t  ucSchedScanAct;  //ENUM_SCHED_SCAN_ACT
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SCAN_SSID_MATCH_SETS {
	uint16_t u2Tag;                   // Tag = 0x15
	uint16_t u2Length;

	uint8_t  ucMatchSsidNum;
	uint8_t  aucReserved[3];
	uint8_t  aucMatchSsidBuffer[]; // SCAN_SCHED_SSID_MATCH_SETS_T
} __KAL_ATTRIB_PACKED__;

/* RDD set command (0x19) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RDD {
	/*fixed field*/
	uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*  TAG                          | ID   | structure
	*  -------------                | -----| -------------
	*  UNI_CMD_ID_RDD_ON_OFF_CTRL   | 0x0  | UNI_CMD_RDD_ON_OFF_CTRL_PARM_T
	*/
} __KAL_ATTRIB_PACKED__;

/* RDD set command Tag */
enum ENUM_UNI_CMD_RDD_TAG {
	UNI_CMD_RDD_TAG_ON_OFF_CTRL_PARM = 0,
	UNI_CMD_RDD_TAG_NUM
};

/* RDD on off command (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RDD_ON_OFF_CTRL_PARM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t u1DfsCtrl;
	uint8_t u1RddIdx;
	uint8_t u1RddRxSel;
	uint8_t u1SetVal;
	uint8_t ucBssIdx;
	uint8_t ucCacLPEn;
	uint8_t aucReserve[2];
} __KAL_ATTRIB_PACKED__;

/* Get mac info command (0x1A) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_GET_MAC_INFO {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	* TAG                             | ID  | structure
	* --------------------------------|-----|--------------
	* UNI_CMD_MAC_INFO_TAG_TSF        | 0x0 | UNI_CMD_MAC_INFO_TSF
	* UNI_CMD_MAC_INFO_TAG_TWT_STA_CNM| 0x1 | UNI_CMD_MAC_INFO_TWT_STA_CNM
	* UNI_CMD_MAC_INFO_TAG_TSF_SYNC   | 0x3 | UNI_CMD_MAC_INFO_TSF_SYNC
	*/
} __KAL_ATTRIB_PACKED__;

/* Get mac info command TLV List */
enum ENUM_UNI_CMD_MAC_INFO_TAG {
	UNI_CMD_MAC_INFO_TAG_TSF = 0,
	UNI_CMD_MAC_INFO_TAG_TWT_STA_CNM = 1,
	UNI_CMD_MAC_INFO_TAG_TSF_SYNC = 3,
	UNI_CMD_MAC_INFO_TAG_NUM
};

/* Get tsf time (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MAC_INFO_TSF {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucDbdcIdx;
	uint8_t ucHwBssidIndex;
	uint8_t ucBssIndex;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

/* TWT STA req CNM CH usage (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MAC_INFO_TWT_STA_CNM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucDbdcIdx;
	uint8_t ucHwBssidIndex;
	uint8_t ucBssIndex;
	uint8_t fgTwtEn;
	uint32_t u4TwtCnmAbortTimeoutMs;
} __KAL_ATTRIB_PACKED__;

/* Get tsf_sync time (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MAC_INFO_TSF_SYNC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgIsLatch;
	uint8_t ucBssIndex;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* TDLS command (0x1B) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TDLS {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   ---------------------------------|-----|--------------
        *   UNI_CMD_SET_TDLS_CH_SW        | 0x0 | UNI_CMD_SET_TDLS_CH_SW_T
        */
} __KAL_ATTRIB_PACKED__;

/* TDLS set command TLV List */
enum ENUM_UNI_CMD_TDLS_TAG {
	UNI_CMD_TDLS_TAG_SET_TDLS_CH_SW = 0,
	UNI_CMD_TDLS_TAG_NUM
};

/* FW Log Basic Setting (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_TDLS_CH_SW {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgIsTDLSChSwProhibit;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* BA offload set command (0x1F) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BA_OFFLOAD {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                          | ID  | structure
	*   -----------------------------|-----|--------------
	*   UNI_CMD_TX_AMPDU             | 0x0 | UNI_CMD_TX_AMPDU_T
	*   UNI_CMD_RX_AMPDU             | 0x1 | UNI_CMD_RX_AMPDU_T
	*   UNI_CMD_TX_AGG_LIMIT         | 0x2 | UNI_CMD_TX_AGG_LIMIT_T
	*   UNI_CMD_TX_AMSDU_NUM_LIMIT   | 0x3 | UNI_CMD_TX_AMSDU_NUM_LIMIT_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Get BA offload command TLV List */
enum UNI_CMD_BA_OFFLOAD_TAG {
	UNI_CMD_BA_OFFLOAD_TAG_TX_AMPDU = 0,
	UNI_CMD_BA_OFFLOAD_TAG_RX_AMPDU = 1,
	UNI_CMD_BA_OFFLOAD_TAG_TX_AGG_LIMIT = 2,
	UNI_CMD_BA_OFFLOAD_TAG_TX_AMSDU_NUM_LIMIT = 3,
	UNI_CMD_BA_OFFLOAD_TAG_NUM
};

/* TX AMPDU (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TX_AMPDU_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  fgEnable;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* RX AMPDU (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RX_AMPDU_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  fgEnable;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* TX AMPDU SIZE (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TX_AGG_LIMIT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2TxAmpduNum;
	uint8_t ucBssIdx;
	uint8_t ucSet;
} __KAL_ATTRIB_PACKED__;

/* TX max AMSDU NUM (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TX_AMSDU_NUM_LIMIT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucTxAmsduNum;
	uint8_t ucBssIdx;
	uint8_t ucSet;
	uint8_t ucReserved;
} __KAL_ATTRIB_PACKED__;

/* P2P command (0x20) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_P2P {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                        |  ID  | structure
	*   ---------------------------|------|--------------
	*   UNI_CMD_SET_NOA_PARAM      | 0x00 | UNI_CMD_SET_NOA_PARAM_T
	*   UNI_CMD_SET_OPPPS_PARAM    | 0x01 | UNI_CMD_SET_OPPPS_PARAM_T
	*   UNI_CMD_SET_LO_START       | 0x02 | UNI_CMD_SET_LO_START_PARAM_T
	*   UNI_CMD_SET_LO_STOP        | 0x03 | UNI_CMD_SET_LO_STOP_PARAM_T
	*/
} __KAL_ATTRIB_PACKED__;

/* P2P command TLV List */
enum ENUM_UNI_CMD_P2P_TAG {
	UNI_CMD_P2P_TAG_SET_NOA_PARAM = 0,
	UNI_CMD_P2P_TAG_SET_OPPPS_PARAM = 1,
	UNI_CMD_P2P_TAG_SET_LO_START = 2,
	UNI_CMD_P2P_TAG_SET_LO_STOP = 3,
	UNI_CMD_P2P_TAG_NUM
};

/* Set NOA parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_NOA_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4NoaDurationMs;
	uint32_t u4NoaIntervalMs;
	uint32_t u4NoaCount;
	uint8_t  ucBssIdx;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* Set OPPPS parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_OPPPS_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4CTwindowMs;
	uint8_t  ucBssIdx;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* Set listen offload start parameters (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_P2P_LO_START_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIndex;
	uint8_t aucReserved1[3];
	uint16_t u2ListenPrimaryCh;
	uint16_t u2Period;
	uint16_t u2Interval;
	uint16_t u2Count;
	uint32_t u4IELen;
	uint8_t aucReserved2[8];
	uint8_t aucIE[];
} __KAL_ATTRIB_PACKED__;

/* Set listen offload stop parameters (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_P2P_LO_STOP_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIndex;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* Smart gear command (0x21) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SMART_GEAR {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *   TAG                        | ID  | structure
        *   ---------------------------|-----|--------------
        *   UNI_CMD_SMART_GEAR_PARAM   | 0x00| UNI_CMD_SMART_GEAR_PARAM_T
        */
} __KAL_ATTRIB_PACKED__;

/* Smart gear command TLV List */
enum ENUM_UNI_CMD_SMART_GEAR_TAG {
	UNI_CMD_SMART_GEAR_TAG_PARAM = 0,
	UNI_CMD_SMART_GEAR_TAG_NUM
};

/* Set smart gear parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SMART_GEAR_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSGEnable;
	uint8_t ucSGSpcCmd;
	uint8_t ucNSSCap;
	uint8_t ucSGCfg;
	uint8_t ucSG24GFavorANT;
	uint8_t ucSG5GFavorANT;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_WIFI_POWER_METRICS
/* Power metrics command (0x3F) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_POWER_METRICS {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*   TAG                        | ID  | structure
	*   ---------------------------|-----|--------------
	*   UNI_CMD_POWER_METRICS_PARAM| 0x00| UNI_CMD_POWER_METRICS_PARAM_T
	*   UNI_CMD_ICCM_PARAM         | 0x01| UNI_CMD_ICCM_PARAM_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Power metrics command TLV List */
enum ENUM_UNI_CMD_POWER_METRICS_TAG {
	UNI_CMD_POWER_METRICS_TAG_PARAM = 0,
	UNI_CMD_ICCM_TAG_PARAM = 1,
	UNI_CMD_POWER_METRICS_TAG_NUM
};

/* Set power metrics parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_POWER_METRICS_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t u4Enable;
	uint32_t u4Value;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* Set ICCM parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ICCM_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t u4Enable;
	uint8_t u4EnablePrintFw;
	uint32_t u4Value;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MIB_INFO {
	/*fixed field*/
	uint8_t ucBand;
	uint8_t ucReserved[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 *   TAG                              | ID  | structure
	 *   -------------                    | ----| -------------
	 *   UNI_CMD_MIB_TAG_DATA             | 0x0 | UNI_CMD_MIB_DATA_T
	 */
} __KAL_ATTRIB_PACKED__;

/* MIB command Tag */
enum ENUM_UNI_CMD_MIB_TAG {
	UNI_CMD_MIB_DATA_TAG = 0,
	UNI_CMD_MIB_MAX_NUM,
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MIB_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Counter;
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_MIB_RMAC_COUNTER_TAG {
	UNI_CMD_MIB_CNT_RX_FCS_ERR = 0,
	UNI_CMD_MIB_CNT_RX_FIFO_OVERFLOW = 1,
	UNI_CMD_MIB_CNT_RX_MPDU = 2,
	UNI_CMD_MIB_CNT_AMDPU_RX_COUNT = 3,
	UNI_CMD_MIB_CNT_RX_TOTAL_BYTE = 4,
	UNI_CMD_MIB_CNT_RX_VALID_AMPDU_SF = 5,
	UNI_CMD_MIB_CNT_RX_VALID_BYTE = 6,
	UNI_CMD_MIB_CNT_CHANNEL_IDLE = 7,
	UNI_CMD_MIB_CNT_VEC_DROP = 8,
	UNI_CMD_MIB_CNT_DELIMITER_FAIL = 9,
	UNI_CMD_MIB_CNT_VEC_MISMATCH = 10,
	UNI_CMD_MIB_CNT_MDRDY = 11,
	UNI_CMD_MIB_CNT_RX_CCK_MDRDY_TIME = 12,
	UNI_CMD_MIB_CNT_RX_OFDM_LG_MIXED_MDRDY_TIME = 13,
	UNI_CMD_MIB_CNT_RX_OFDM_GREEN_MDRDY_TIME = 14,
	UNI_CMD_MIB_CNT_PF_DROP = 15,
	UNI_CMD_MIB_CNT_LEN_MISMATCH = 16,
	UNI_CMD_MIB_CNT_P_CCA_TIME = 17,
	UNI_CMD_MIB_CNT_S_CCA_TIME = 18,
	UNI_CMD_MIB_CNT_CCA_NAV_TX_TIME = 19,
	UNI_CMD_MIB_CNT_P_ED_TIME = 20,
	UNI_CMD_MIB_CNT_BCN_TX = 21,
	UNI_CMD_MIB_CNT_TX_BW_20MHZ = 22,
	UNI_CMD_MIB_CNT_TX_BW_40MHZ = 23,
	UNI_CMD_MIB_CNT_TX_BW_80MHZ = 24,
	UNI_CMD_MIB_CNT_TX_BW_160MHZ = 25,
	UNI_CMD_RMAC_CNT_OBSS_AIRTIME = 26,
	UNI_CMD_RMAC_CNT_NONWIFI_AIRTIME = 27,
	UNI_CMD_MIB_CNT_TX_DUR_CNT = 28,
	UNI_CMD_MIB_CNT_RX_DUR_CNT = 29,
	UNI_CMD_MIB_CNT_BA_CNT = 30,
	UNI_CMD_MIB_CNT_MAC2PHY_TX_TIME = 31,
	UNI_CMD_MIB_CNT_RX_OUT_OF_RANGE_COUNT = 32,
	UNI_CMD_MIB_CNT_IBF_TX = 33,
	UNI_CMD_MIB_CNT_EBF_TX = 34,
	UNI_CMD_MIB_CNT_MUBF_TX = 35,
	UNI_CMD_MIB_CNT_RX_BF_VHTFBK = 36,
	UNI_CMD_MIB_CNT_RX_BF_HTFBK = 37,
	UNI_CMD_MIB_CNT_RX_BF_HEFBK = 38,
	UNI_CMD_MIB_CNT_BFEE_TXFBK_BFPOLL_TRI = 39,
	UNI_CMD_MIB_CNT_BFEE_TXFBK_TRI = 40,
	UNI_CMD_MIB_CNT_BFEE_RX_FBKCQI = 41,
	UNI_CMD_MIB_CNT_BFEE_RX_NDP = 42,
	UNI_CMD_MIB_CNT_RX_BF_IBF_UPT = 43,
	UNI_CMD_MIB_CNT_RX_BF_EBF_UPT = 44,
	UNI_CMD_MIB_CNT_BFEE_SP_ABORT = 45,
	UNI_CMD_MIB_CNT_BFEE_TB_LEN_ERR = 46,
	UNI_CMD_MIB_CNT_BFEE_TXFBK_MUTE = 47,
	UNI_CMD_MIB_CNT_BFEE_TMAC_ABORT = 48,
	UNI_CMD_MIB_CNT_BFEE_TXFBK_CPL = 49,
	UNI_CMD_MIB_CNT_BFEE_COANT_BLKTX = 50,
	UNI_CMD_MIB_CNT_BFEE_FBK_SEG = 51,
	UNI_CMD_MIB_CNT_NAV_TIME = 52,
	UNI_CMD_MIB_CNT_TX_ABORT_CNT1 = 53,
	UNI_CMD_MIB_CNT_TX_ABORT_CNT0 = 54,
	UNI_CMD_MIB_CNT_TX_ABORT_CNT3 = 55,
	UNI_CMD_MIB_CNT_TX_ABORT_CNT2 = 56,
	UNI_CMD_MIB_CNT_BSS0_RTS_TX_CNT = 57,
	UNI_CMD_MIB_CNT_BSS1_RTS_TX_CNT = 58,
	UNI_CMD_MIB_CNT_BSS2_RTS_TX_CNT = 59,
	UNI_CMD_MIB_CNT_BSS3_RTS_TX_CNT = 60,
	UNI_CMD_MIB_CNT_BSS0_RTS_RETRY = 61,
	UNI_CMD_MIB_CNT_BSS1_RTS_RETRY = 62,
	UNI_CMD_MIB_CNT_BSS2_RTS_RETRY = 63,
	UNI_CMD_MIB_CNT_BSS3_RTS_RETRY = 64,
	UNI_CMD_MIB_CNT_BSS0_BA_MISS = 65,
	UNI_CMD_MIB_CNT_BSS1_BA_MISS = 66,
	UNI_CMD_MIB_CNT_BSS2_BA_MISS = 67,
	UNI_CMD_MIB_CNT_BSS3_BA_MISS = 68,
	UNI_CMD_MIB_CNT_BSS0_ACK_FAIL = 69,
	UNI_CMD_MIB_CNT_BSS1_ACK_FAIL = 70,
	UNI_CMD_MIB_CNT_BSS2_ACK_FAIL = 71,
	UNI_CMD_MIB_CNT_BSS3_ACK_FAIL = 72,
	UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY = 73,
	UNI_CMD_MIB_CNT_BSS1_FRAME_RETRY = 74,
	UNI_CMD_MIB_CNT_BSS2_FRAME_RETRY = 75,
	UNI_CMD_MIB_CNT_BSS3_FRAME_RETRY = 76,
	UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY_2 = 77,
	UNI_CMD_MIB_CNT_BSS1_FRAME_RETRY_2 = 78,
	UNI_CMD_MIB_CNT_BSS2_FRAME_RETRY_2 = 79,
	UNI_CMD_MIB_CNT_BSS3_FRAME_RETRY_2 = 80,
	UNI_CMD_MIB_CNT_RX_A1_SEARCH = 81,
	UNI_CMD_MIB_CNT_RX_DROP_MPDU = 82,
	UNI_CMD_MIB_CNT_RX_UNWANTED = 83,
	UNI_CMD_MIB_CNT_RX_FCS_OK = 84,
	UNI_CMD_MIB_CNT_SU_TX_OK = 85,
	UNI_CMD_MIB_CNT_TX_FULL_BW = 86,
	UNI_CMD_MIB_CNT_TX_AUTO_BW = 87,
	UNI_CMD_MIB_CNT_CCA_SAMPLE_IDLE = 88,
	UNI_CMD_MIB_CNT_CCA_SAMPLE_ACTIVE = 89,
	UNI_CMD_MIB_CNT_CCA_TIME = 90,
	UNI_CMD_MIB_CNT_S_20BW_CCA_TIME = 91,
	UNI_CMD_MIB_CNT_S_40BW_CCA_TIME = 92,
	UNI_CMD_MIB_CNT_S_80BW_CCA_TIME = 93,
	UNI_CMD_MIB_CNT_S_160BW_CCA_TIME = 94,
	UNI_CMD_MIB_CNT_S_P20BW_0_ED_TIME = 95,
	UNI_CMD_MIB_CNT_S_P20BW_1_ED_TIME = 96,
	UNI_CMD_MIB_CNT_S_P20BW_2_ED_TIME = 97,
	UNI_CMD_MIB_CNT_S_P20BW_3_ED_TIME = 98,
	UNI_CMD_MIB_CNT_S_P20BW_4_ED_TIME = 99,
	UNI_CMD_MIB_CNT_S_P20BW_5_ED_TIME = 100,
	UNI_CMD_MIB_CNT_S_P20BW_6_ED_TIME = 101,
	UNI_CMD_MIB_CNT_S_P20BW_7_ED_TIME = 102,
	UNI_CMD_MIB_CNT_S_P20BW_8_ED_TIME = 103,
	UNI_CMD_MIB_CNT_S_P20BW_9_ED_TIME = 104,
	UNI_CMD_MIB_CNT_S_P20BW_10_ED_TIME = 105,
	UNI_CMD_MIB_CNT_S_P20BW_11_ED_TIME = 106,
	UNI_CMD_MIB_CNT_S_P20BW_12_ED_TIME = 107,
	UNI_CMD_MIB_CNT_S_P20BW_13_ED_TIME = 108,
	UNI_CMD_MIB_CNT_S_P20BW_14_ED_TIME = 109,
	UNI_CMD_MIB_CNT_S_P20BW_15_ED_TIME = 110,
	UNI_CMD_MIB_CNT_TX_BW_320MHZ = 111,
	UNI_CMD_MIB_CNT_TX_DDLMT_RNG0 = 112, /* Rng0~Rng4 112~116 */
	UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY_3 = 117, /* 117~120 */
	UNI_CMD_MIB_CNT_BSS0_TX = 121, /* 121~124 */
	UNI_CMD_MIB_CNT_BSS0_TX_DATA = 125, /* 125~128 */
	UNI_CMD_MIB_CNT_BSS0_TX_BYTE = 129, /* 129~132 */
	UNI_CMD_MIB_CNT_RX_OK_BSS0 = 133, /* 133~136 */
	UNI_CMD_MIB_CNT_RX_BYTE_BSS0 = 137, /* 137~140 */
	UNI_CMD_MIB_CNT_RX_DATA_BSS0 = 141, /* 141~144 */
	UNI_CMD_MIB_CNT_MBSS0_TX_OK = 145, /* 145~160 */
	UNI_CMD_MIB_CNT_MBSS0_TX_BYTE = 161, /* 161~176 */
	UNI_CMD_MIB_CNT_RX_OK_MBSS0 = 177, /* 177~192 */
	UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 = 193, /* 193~208 */
	UNI_CMD_MIB_CNT_AMPDU = 209,
	UNI_CMD_MIB_CNT_AMPDU_MPDU = 210,
	UNI_CMD_MIB_CNT_AMPDU_ACKED = 211,
	UNI_CMD_MIB_CNT_MAX_NUM
};

struct MIB_CNT_MAP {
	enum ENUM_UNI_CMD_MIB_RMAC_COUNTER_TAG tag;
	uint64_t *pu8Counter;
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_GET_STATISTICS {
	/*fixed field*/
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   -------------                    | ----| -------------
        *   UNI_CMD_BASIC_STATISTICS         | 0x0 | UNI_CMD_BASIC_STATISTICS_T
        *   UNI_CMD_LINK_QUALITY             | 0x1 | UNI_CMD_LINK_QUALITY_T
        */
} __KAL_ATTRIB_PACKED__;

/* Get Statistics Tag */
enum ENUM_UNI_CMD_GET_STATISTICS_TAG {
	UNI_CMD_GET_STATISTICS_TAG_BASIC = 0,
	UNI_CMD_GET_STATISTICS_TAG_LINK_QUALITY = 1,
	UNI_CMD_GET_STATISTICS_TAG_STA = 2,
	UNI_CMD_GET_STATISTICS_TAG_BUG_REPORT = 3,
	UNI_CMD_GET_STATISTICS_TAG_EML_STATS = 6,
	UNI_CMD_GET_STATISTICS_TAG_REGULAR_STATS = 7,
	UNI_CMD_GET_STATISTICS_TAG_BSS_LINK_QUALITY = 8,
	UNI_CMD_GET_STATISTICS_TAG_GET_ML_CHNL_COND = 9,
	/* Reserved range for compatible with ENUM_STATS_LLS_TLV_TAG_ID */
	UNI_CMD_GET_STATISTICS_TAG_LINK_LAYER_STATS = 0x80,
	UNI_CMD_GET_STATISTICS_TAG_PPDU_LATENCY,
	UNI_CMD_GET_STATISTICS_TAG_CURRENT_TX_RATE,
	UNI_CMD_GET_STATISTICS_TAG_BEACON_REPORT = 0x86,
	UNI_CMD_GET_STATISTICS_TAG_BSS_CURRENT_TX_RATE = 0x87,
	UNI_CMD_GET_STATISTICS_TAG_BSS_PRED_TX_BITRATE = 0x88,
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BASIC_STATISTICS {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_LINK_QUALITY {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BUG_REPORT {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_STA_STATISTICS {
	uint16_t u2Tag;
	uint16_t u2Length;
	/** Peer SW station record index */
	uint8_t  u1Index;
	/** TRUE: After event, clear TX fail count & lifetimeout count */
	uint8_t  ucReadClear;
	/** TRUE: After event, clear all AC statistics for the peer */
	uint8_t  ucLlsReadClear;
	/** TRUE: (RA) clear TransmitCount, TransmitFailCount,
	 * Rate1TxCnt, Rate1FailCnt */
	uint8_t  ucResetCounter;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_REGULAR_STATS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSS_LINK_QUALITY {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_GET_ML_CHNL_COND {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BEACON_REPORT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucEnable;
	uint8_t ucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_LINK_LAYER_STATS {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucArg0;
	uint8_t ucArg1;
	uint8_t ucArg2;
	uint8_t ucArg3;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CURRENT_TX_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucArg0;
	uint8_t ucArg1;
	uint8_t ucArg2;
	uint8_t ucArg3;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BSS_CURRENT_TX_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucArg0;
	uint8_t ucArg1;
	uint8_t ucArg2;
	uint8_t ucArg3;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EML_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PKT_DROP {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	*
	*   TAG                                                                               | ID   | structure
	*   -----------------------------------------              | ---- | -----------------------------------
	*   ENUM_UNI_CMD_ID_SET_DROP_PACKET_CFG             | 0x00 | UNI_CMD_SET_DROP_PACKET_PARAM
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_PKT_DROP_TAG {
	UNI_CMD_PKT_DROP_TAG_CFG                    = 1,
	UNI_CMD_PKT_DROP_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PKT_DROP_SETTING {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucMagicCode;
	uint8_t ucCmdBufferLen;    //buffer length
	uint8_t aucReserved[24];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SNIFFER_MODE {
	/* fixed field */
	uint8_t ucBandIdx;
	uint8_t ucReserved[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Sniffer mode command TLV List */
enum ENUM_UNI_CMD_SNIFFER_MODE_TAG {
	UNI_CMD_SNIFFER_MODE_TAG_ENABLE = 0,
	UNI_CMD_SNIFFER_MODE_TAG_CONFIG = 1,
	UNI_CMD_SNIFFER_MODE_TAG_NUM
};

/* Set sniffer mode parameters (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SNIFFER_MODE_ENABLE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSNEnable;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* Set sniffer mode parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SNIFFER_MODE_CONFIG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2Aid;
	uint8_t ucBand;
	uint8_t ucChannelWidth;
	uint8_t ucPriChannel;
	uint8_t ucSco;
	uint8_t ucChannelS1;
	uint8_t ucChannelS2;
	uint8_t fgDropFcsErrorFrame;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

struct UNI_CMD_SR {
	/*Fixed Fields*/
	uint8_t u1BandIdx;
	uint8_t au1Padding[3];
	/*TLV*/
	uint8_t au1TlvBuffer[];
};

/* SR Command Tag ID */
enum ENUM_UNI_CMD_SR_TAG {
	UNI_CMD_SR_TAG_RSV = 0x0,
	UNI_CMD_SR_TAG_CFG_SR_ENABLE = 0x1,
	UNI_CMD_SR_TAG_CFG_SR_SD_ENABLE = 0x2,
	UNI_CMD_SR_TAG_CFG_SR_MODE = 0x3,
	UNI_CMD_SR_TAG_CFG_DISRT_ENABLE = 0x4,
	UNI_CMD_SR_TAG_CFG_DISRT_MIN_RSSI = 0x5,
	UNI_CMD_SR_TAG_CFG_SR_BF = 0x6,
	UNI_CMD_SR_TAG_CFG_SR_ATF = 0x7,
	UNI_CMD_SR_TAG_CFG_TXC_QUEUE = 0x8,
	UNI_CMD_SR_TAG_CFG_TXC_QID = 0x9,
	UNI_CMD_SR_TAG_CFG_TXC_PATH = 0xA,
	UNI_CMD_SR_TAG_CFG_AC_METHOD = 0xB,
	UNI_CMD_SR_TAG_CFG_SR_PERIOD_THR = 0xC,
	UNI_CMD_SR_TAG_CFG_QUERY_TXD_METHOD = 0xD,
	UNI_CMD_SR_TAG_CFG_SR_SD_CG_RATIO = 0xE,
	UNI_CMD_SR_TAG_CFG_SR_SD_OBSS_RATIO = 0xF,
	UNI_CMD_SR_TAG_CFG_PROFILE = 0x10,
	UNI_CMD_SR_TAG_CFG_FNQ_ENABLE = 0x11,
	UNI_CMD_SR_TAG_CFG_DPD_ENABLE = 0x12,
	UNI_CMD_SR_TAG_CFG_SR_TX_ENABLE = 0x13,
	UNI_CMD_SR_TAG_CFG_SR_SD_OM_ENABLE = 0x14,
	UNI_CMD_SR_TAG_CFG_SR_DABS_MODE = 0x15,
	UNI_CMD_SR_TAG_SW_SRG_BITMAP = 0x80,
	UNI_CMD_SR_TAG_SW_MESH_SRG_BITMAP = 0x81,
	UNI_CMD_SR_TAG_SW_SRG_BITMAP_REFRESH = 0x82,
	UNI_CMD_SR_TAG_SW_CNT = 0x83,
	UNI_CMD_SR_TAG_SW_SD = 0x84,
	UNI_CMD_SR_TAG_SW_GLOVAR_DROPTA_INFO = 0x85,
	UNI_CMD_SR_TAG_SW_GLOVAR_STA_INFO = 0x86,
	UNI_CMD_SR_TAG_UPDATE_SR_PARAMS = 0x87,
	UNI_CMD_SR_TAG_HW_CAP = 0xC0,
	UNI_CMD_SR_TAG_HW_PARA = 0xC1,
	UNI_CMD_SR_TAG_HW_COND = 0xC2,
	UNI_CMD_SR_TAG_HW_RCPI_TBL = 0xC3,
	UNI_CMD_SR_TAG_HW_RCPI_TBL_OFST = 0xC4,
	UNI_CMD_SR_TAG_HW_Q_CTRL = 0xC5,
	UNI_CMD_SR_TAG_HW_IBPD = 0xC6,
	UNI_CMD_SR_TAG_HW_NRT = 0xC7,
	UNI_CMD_SR_TAG_HW_NRT_CTRL = 0xC8,
	UNI_CMD_SR_TAG_HW_NRT_RESET = 0xC9,
	UNI_CMD_SR_TAG_HW_CAP_SREN = 0xCA,
	UNI_CMD_SR_TAG_HW_IND = 0xCB,
	UNI_CMD_SR_TAG_HW_FNQ = 0xCC,
	UNI_CMD_SR_TAG_HW_FRMFILT = 0xCD,
	UNI_CMD_SR_TAG_HW_INTERPS_CTRL = 0xCE,
	UNI_CMD_SR_TAG_HW_INTERPS_DBG = 0xCF,
	UNI_CMD_SR_TAG_HW_SIGA_FLAG = 0xD0,
	UNI_CMD_SR_TAG_NUM,
};

struct UNI_CMD_SR_UPDATE_SR_PARMS {
	/* DW_0 */
	uint16_t u2Tag;
	uint16_t u2Length;

	// DWORD_1 - Common Part
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;       /* Cmd size including common part and body */

	// DWORD_2 afterwards - Command Body
	uint8_t  ucBssIndex;
	uint8_t  ucSRControl;
	uint8_t  ucNonSRGObssPdMaxOffset;
	uint8_t  ucSRGObssPdMinOffset;
	uint8_t  ucSRGObssPdMaxOffset;
	uint8_t  aucPadding1[3];
	uint32_t u4SRGBSSColorBitmapLow;
	uint32_t u4SRGBSSColorBitmapHigh;
	uint32_t u4SRGPartialBSSIDBitmapLow;
	uint32_t u4SRGPartialBSSIDBitmapHigh;

	uint8_t aucPadding2[32];
};

__KAL_ATTRIB_PACKED_FRONT__
struct WH_SR_IND {
	/* RMAC */
	/* DW_1 */
	uint16_t u2NonSrgVldCnt;
	uint16_t u2SrgVldCnt;
	/* DW_2 */
	uint16_t u2IntraBssPpduCnt;
	uint16_t u2InterBssPpduCnt;
	/* DW_3 */
	uint16_t u2NonSrgPpduVldCnt;
	uint16_t u2SrgPpduVldCnt;
	/* MIB */
	/* DW_4 */
	uint32_t u4SrAmpduMpduCnt;
	/* DW_5 */
	uint32_t u4SrAmpduMpduAckedCnt;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct WH_SR_CAP {
	/* RMAC */
	/* DW_1 */
	uint8_t fgSrEn;
	uint8_t fgSrgEn;
	uint8_t fgNonSrgEn;
	uint8_t fgSingleMdpuRtsctsEn;
	/* DW_2 */
	uint8_t fgHdrDurEn;
	uint8_t fgTxopDurEn;
	uint8_t fgNonSrgInterPpduPresv;
	uint8_t fgSrgInterPpduPresv;
	/* DW_3 */
	uint8_t fgSMpduNoTrigEn;
	uint8_t fgSrgBssidOrder;
	uint8_t fgCtsAfterRts;
	uint8_t fgSrpOldRxvEn;
	/* DW_4 */
	uint8_t fgSrpNewRxvEn;
	uint8_t fgSrpDataOnlyEn;
	uint8_t fgFixedRateSrREn;
	uint8_t fgWtblSrREn;
	/* AGG  */
	/* DW_5 */
	uint8_t fgSrRemTimeEn;
	uint8_t fgProtInSrWinDis;
	uint8_t fgTxCmdDlRateSelEn;
	/* MIB  */
	uint8_t fgAmpduTxCntEn;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SR_IND {
	/* DW_0 */
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SR_CAP {
	/* DW_0 */
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CNM {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   UNI_CMD_CNM_CH_PRIVILEGE_REQ     | 0x0 | UNI_CMD_CNM_CH_PRIVILEGE_REQ_T
	*   UNI_CMD_CNM_CH_PRIVILEGE_ABORT   | 0x1 | UNI_CMD_CNM_CH_PRIVILEGE_ABORT_T
	*   UNI_CMD_CNM_GET_INFO             | 0x2 | UNI_CMD_CNM_GET_INFO_T
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_CNM_TAG {
	UNI_CMD_CNM_TAG_CH_PRIVILEGE_REQ = 0,
	UNI_CMD_CNM_TAG_CH_PRIVILEGE_ABORT = 1,
	UNI_CMD_CNM_TAG_GET_INFO = 2,
	UNI_CMD_CNM_TAG_CH_PRIVILEGE_MLO_SUB_REQ = 3,
	UNI_CMD_CNM_TAG_NUM
};

enum ENUM_UNI_CMD_CNM_CHANNEL_WIDTH {
	UNI_CMD_CNM_CHANNEL_WIDTH_20_40MHZ = 0,
	UNI_CMD_CNM_CHANNEL_WIDTH_80MHZ    = 1,
	UNI_CMD_CNM_CHANNEL_WIDTH_160MHZ   = 2,
	UNI_CMD_CNM_CHANNEL_WIDTH_80P80MHZ = 3,
	UNI_CMD_CNM_CHANNEL_WIDTH_5MHZ     = 4,
	UNI_CMD_CNM_CHANNEL_WIDTH_10MHZ    = 5,
	UNI_CMD_CNM_CHANNEL_WIDTH_320MHZ   = 6,
	UNI_CMD_CNM_CHANNEL_WIDTH_NUM,
};

enum ENUM_UNI_CMD_CNM_CH_REQ_BAND {
	UNI_CMD_CNM_CH_REQ_BAND_0 = 0,
	UNI_CMD_CNM_CH_REQ_BAND_1 = 1,
	UNI_CMD_CNM_CH_REQ_BAND_2 = 2,
	UNI_CMD_CNM_CH_REQ_BAND_NUM = CONFIG_BAND_NUM,
	UNI_CMD_CNM_CH_REQ_BAND_ALL = 0xFE,
	UNI_CMD_CNM_CH_REQ_BAND_AUTO = 0xFF,
};

enum ENUM_CNM_CH_PRIVILEGE_REQ_EXTRA_INFO {
	CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_MULTI_LINK = 0,
	CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_EMLSR = 1,
	CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CNM_CH_PRIVILEGE_REQ {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint8_t          ucBssIndex;
	uint8_t          ucTokenID;
	uint8_t          ucPrimaryChannel;
	uint8_t          ucRfSco;
	uint8_t          ucRfBand;
	uint8_t          ucRfChannelWidth;   /* To support 80/160MHz bandwidth */
	uint8_t          ucRfCenterFreqSeg1; /* To support 80/160MHz bandwidth */
	uint8_t          ucRfCenterFreqSeg2; /* To support 80/160MHz bandwidth */
	uint8_t          ucRfChannelWidthFromAP;  /* record original 20 /40 /80/160MHz bandwidth from AP's IE */
	uint8_t          ucRfCenterFreqSeg1FromAP;
	uint8_t          ucRfCenterFreqSeg2FromAP;
	uint8_t          ucReqType;          /* ENUM_CH_REQ_TYPE_T */
	uint32_t         u4MaxInterval;      /* In unit of ms */
	uint8_t          ucDBDCBand;
	/* CNM Function Mask: Each bit represents whether to turn on
	 * specific function
	 */
	uint8_t          ucCnmFuncMask;
	uint8_t          ucExtraInfo;
	uint8_t          aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CNM_CH_PRIVILEGE_ABORT {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint8_t          ucBssIndex;
	uint8_t          ucTokenID;
	uint8_t          ucDBDCBand;
	uint8_t          aucReserved[5];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CNM_GET_INFO {
	uint16_t         u2Tag;
	uint16_t         u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MBMC {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   UNI_CMD_MBMC_TAG_SETTING         | 0x0 | UNI_CMD_MBMC_SETTING
	*/
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_CMD_MBMC_TAG {
	UNI_CMD_MBMC_TAG_SETTING = 0,
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	UNI_CMD_MBMC_NO_RESP_TAG_SETTING = 1,
#endif
	UNI_CMD_MBMC_TAG_MAX_NUM
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MBMC_SETTING {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucMbmcEn;
	uint8_t ucAAModeEn;
	uint8_t ucRfBand;
	uint8_t ucReason;
} __KAL_ATTRIB_PACKED__;

/* Set DVT config Tag */
enum ENUM_UNI_CMD_DVT_TYPE_TAG {
	UNI_CMD_MODULE_DVT = 0,
	UNI_CMD_SYSTEM_DVT = 1,
	UNI_CMD_DVT_TYPE_TAG_MAX_NUM
};

enum ENUM_UNI_CMD_DVT_TAG {
	UNI_CMD_DVT_TAG_SET_PARA = 0,
	UNI_CMD_DVT_TAG_SET_TRB_BLOCK = 1,
	UNI_CMD_DVT_TAG_SET_BLOCK_MODE = 2,
	UNI_CMD_DVT_TAG_GET_DMA_RESULT = 3,
	UNI_CMD_DVT_TAG_SET_LPON_TEST_START = 4,
	UNI_CMD_DVT_TAG_GET_LPON_TSF_RESULT = 5,
	UNI_CMD_DVT_TAG_SET_WTBL_DURING_TEST = 6,
	UNI_CMD_DVT_TAG_GET_WTBL_RESULT = 7,
	UNI_CMD_DVT_TAG_MAX_NUM
};

struct UNI_CMD_DVT {
	uint8_t ucTestType;
	uint8_t aucPadding[3];
	uint8_t aucTlvBuffer[];
};

struct UNI_CMD_MDVT_PARA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2ModuleId;
	uint16_t u2CaseId;
	uint8_t ucCapId;
	uint8_t ucReserved[3];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_POWER_LIMIT {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/** Power Limit command TLV List */
__KAL_ATTRIB_PACKED_FRONT__
enum UNI_CMD_POWER_LIMIT_TAG {
	UNI_CMD_POWER_LIMIT_TABLE_CTRL = 0,
	UNI_CMD_POWER_LIMIT_PER_RATE_TABLE = 1,
	UNI_CMD_POWER_LIMIT_TX_PWR_ENV = 2,
	UNI_CMD_POWER_LIMIT_FWREGD_CTRL = 3,
	UNI_CMD_POWER_LIMIT_EMI_INFO = 4,
	UNI_CMD_POWER_LIMIT_TAG_MAX_NUM
} __KAL_ATTRIB_PACKED__;

/* Power limit table (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_PWR_LIMIT_PARAM {
	/* Tag = 0x00 */
	uint16_t u2Tag;
	uint16_t u2Length;

	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT config;
} __KAL_ATTRIB_PACKED__;

/* Power limit per rate table (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_PWR_LIMIT_PER_RATE_TABLE_PARAM {
	/* Tag = 0x01 */
	uint16_t u2Tag;
	uint16_t u2Length;

	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE config;
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
/* Power limit table (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_PWR_LIMIT_EMI_INFO {
	/* Tag = 0x03 */
	uint16_t u2Tag;
	uint16_t u2Length;

	struct CMD_EMI_POWER_LIMIT_FORMAT config;
} __KAL_ATTRIB_PACKED__;
#endif

/*UNI_CMD_ID_TXPOWER */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TXPOWER_CONFIG {
    /*fixed field*/
    uint8_t aucPadding[4];
    /* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NVRAM_SETTINGS {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/** Nvram command TLV List */
__KAL_ATTRIB_PACKED_FRONT__
enum UNI_CMD_NVRAM_SETTINGS_TAG {
	UNI_CMD_NVRAM_SETTINGS_FRAGMENT = 0,
	UNI_CMD_NVRAM_SETTINGS_LEGACY = 1,
	UNI_CMD_NVRAM_SETTINGS_NUM
} __KAL_ATTRIB_PACKED__;

/* Nvram settings (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NVRAM_SETTINGS_FRAGMENT_PARAM {
	/* Tag = 0x00 */
	uint16_t u2Tag;
	uint16_t u2Length;

	struct CMD_NVRAM_FRAGMENT config;
} __KAL_ATTRIB_PACKED__;

/* Nvram settings (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NVRAM_SETTINGS_LEGACY_PARAM {
	/* Tag = 0x01 */
	uint16_t u2Tag;
	uint16_t u2Length;

	struct CMD_NVRAM_SETTING config;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RA {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                                        | ID   |
	*   -------------------------  | --   |
	*   UNI_CMD_RA_GET_RU_RA_INFO    | 0x0 |
	*   UNI_CMD_RA_GET_HERA_RELATED_INFO    | 0x1 |
	*   UNI_CMD_RA_GET_MU_RA_INFO           | 0x2 |
	*   UNI_CMD_RA_HERA_STBC_PRIORITY       | 0x3 |
	*   UNI_CMD_RA_OPTION_CTRL              | 0x4 |
	*   UNI_CMD_RA_SET_VHT_1024_QAM     | 0x5 |
	*   UNI_CMD_RA_CFG_PTEC_PER_PPDU        | 0x6 |
	*   UNI_CMD_RA_CFG_MU_INIT_RATE_INTV    | 0x7 |
	*   UNI_CMD_RA__CFG_MU_DIS_SWITCH_SU    | 0x8 |
	*   UNI_CMD_RA_SET_VHT_RATE_FOR_2G      | 0x9 |
	*   UNI_CMD_RA_FIX_RATE_WO_STA_UPDATE   | 0xA |
	*   UNI_CMD_RA_SUPPORT_MCS_CAP_CTRL | 0xB |
	*   UNI_CMD_RA_DBG_CTRL             | 0xC |
	*   UNI_CMD_RA_GET_TX_RATE              | 0xD |
	*   UNI_CMD_RA_SET_MAX_PHY_RATE     | 0xE |
	*   UNI_CMD_RA_SET_FIXED_RATE          | 0xF |
	*   UNI_CMD_RA_SET_FIXED_RATE_UL_TRIG    | 0x10|
	*   UNI_CMD_RA_SET_AUTO_RATE     | 0x11|
	*/
} __KAL_ATTRIB_PACKED__;


/* RA Tag */
enum ENUM_UNI_CMD_RA_TAG {
	UNI_CMD_RA_TAG_GET_RU_RA_INFO = 0x0,
	UNI_CMD_RA_TAG_GET_HERA_RELATED_INFO = 0x01,
	UNI_CMD_RA_TAG_GET_MU_RA_INFO = 0x02,
	UNI_CMD_RA_TAG_HERA_STBC_PRIORITY = 0x03,
	UNI_CMD_RA_TAG_OPTION_CTRL = 0x04,
	UNI_CMD_RA_TAG_SET_VHT_1024_QAM = 0x05,
	UNI_CMD_RA_TAG_CFG_PTEC_PER_PPDU = 0x06,
	UNI_CMD_RA_TAG_CFG_MU_INIT_RATE_INTV = 0x07,
	UNI_CMD_RA_TAG_CFG_MU_DIS_SWITCH_SU = 0x08,
	UNI_CMD_RA_TAG_SET_VHT_RATE_FOR_2G = 0x09,
	UNI_CMD_RA_TAG_FIX_RATE_WO_STA_UPDATE = 0x0A,
	UNI_CMD_RA_TAG_SUPPORT_MCS_CAP_CTRL = 0x0B,
	UNI_CMD_RA_TAG_DBG_CTRL = 0x0C,
	UNI_CMD_RA_TAG_GET_TX_RATE = 0x0D,
	UNI_CMD_RA_TAG_SET_MAX_PHY_RATE = 0x0E,
	UNI_CMD_RA_TAG_SET_FIXED_RATE = 0x0F,
	UNI_CMD_RA_TAG_SET_FIXED_RATE_UL_TRIG = 0x10,
	UNI_CMD_RA_TAG_SET_AUTO_RATE = 0x11,
	UNI_CMD_RA_TAG_NUM
};

enum ENUM_UNI_CMD_RA_FIXED_RATE_VER {
	UNI_CMD_RA_FIXED_RATE_VER1 = 0x0,
	UNI_CMD_RA_FIXED_RATE_VER_MAX_NUM
};

/*RA fixed rate Parameters (Tag 0x0F) */
struct UNI_CMD_RA_SET_FIXED_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */

	uint16_t u2Version;
	uint8_t  aucBuffer[];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RA_SET_FIXED_RATE_V1 {
	uint16_t u2WlanIdx;
	uint8_t  u1PhyMode;
	uint8_t  u1Stbc;
	uint16_t u2ShortGi;
	uint8_t  u1Bw;
	uint8_t  u1Ecc;
	uint8_t  u1Mcs;
	uint8_t  u1Nss;
	uint16_t u2HeLtf;
	uint8_t  u1Spe;
	uint8_t  u1ShortPreamble;
	uint16_t u2Reserve;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RA_SET_AUTO_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint16_t u2WlanIdx;
	uint8_t u1AutoRateEn;
	uint8_t u1Mode;
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLO {
	/* fixed field */
	uint8_t u1BandIdx;
	uint8_t au1Reserved[3];

	/* tlv */
	uint8_t au1TlvBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLO {
	/* fixed field */
	uint8_t u1BandIdx;
	uint8_t au1Reserved[3];

	/* tlv */
	uint8_t au1TlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* MLO Tag */
enum ENUM_UNI_CMD_MLO_TAG {
	UNI_CMD_MLO_TAG_RSV = 0x0,
	UNI_CMD_MLO_TAG_MLO_MGMT = 0x1,
	UNI_CMD_MLO_TAG_MLD_REC = 0x2,
	UNI_CMD_MLO_TAG_MLD_REC_LINK = 0x3,
	UNI_CMD_MLO_TAG_MLD_REC_LINK_AGC_TX = 0x4,
	UNI_CMD_MLO_TAG_MLD_REC_LINK_AGC_TRIG = 0x5,
	UNI_CMD_MLO_TAG_MLD_MLSR_CONCURENT_PRECONNECT = 0x7,
};

/* UNI_CMD_MLO_TAG_MLD_REC(Tag=0x2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_MLD_REC {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

/* UNI_CMD_MLO_TAG_MLD_REC_LINK_AGC_TX(Tag=0x04) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLO_MLD_REC_LINK_AGC_TX {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint8_t u1MldRecIdx;
	uint8_t u1MldRecLinkIdx;
	uint8_t u1AcIdx;
	uint8_t u1DispPolTx;

	uint8_t u1DispRatioTx;
	uint8_t u1DispOrderTx;
	uint16_t u2DispMgfTx;
} __KAL_ATTRIB_PACKED__;

/* UNI_CMD_MLO_TAG_MLD_REC(Tag=0x2) */
struct UNI_EVENT_GET_MLD_REC {
	uint16_t u2Tag;
	uint16_t u2Length;

	struct PARAM_MLD_REC rMldRec;
};

/*UNI_CMD_MLO_TAG_MLD_MLSR_CONCURENT_PRECONNECT(Tag=0x7)*/
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLD_MLSR_CONCURENT_PRECONNECT {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucMlsrRemainBssIndex;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

#endif

/*PP command (Tag 0x38) */
struct UNI_CMD_PP {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 * TAG                        | ID  | structure
	 * -------------------        | ----| -------------
	 *    UNI_CMD_PP_EN_CTRL         | 0x0 | UNI_CMD_PP_EN_CTRL_T
	 *    UNI_CMD_PP_ALG_CTRL        | 0x1 | UNI_CMD_PP_ALG_CTRL_T
	 *    UNI_CMD_PP_DSCB_CTRL       | 0x2 | UNI_CMD_PP_DSCB_CTRL_T
	 */
};
/** @} */

enum UNI_CMD_ID_PP_TAG {
	UNI_CMD_PP_TAG_EN_CTRL = 0x0,
	UNI_CMD_PP_TAG_ALG_CTRL = 0x1,
	UNI_CMD_PP_DSCB_CTRL = 0x2,
	UNI_CMD_PP_MAX_NUM
};

enum UNI_CMD_PP_ALG_CMD_ACTION {
	UNI_CMD_PP_ALG_SET_TIMER = 0,
	UNI_CMD_PP_ALG_SET_THR = 1,
	UNI_CMD_PP_ALG_GET_STATISTICS = 2,
	UNI_CMD_PP_ALG_MAX_NUM
};

enum PP_HW_CTRL_MODE {
	PP_NO_PP = 0x0,
	PP_HW_PP = 0x1,
	PP_SW_PP = 0x2,
	PP_SW_THEN_HW_PP = 0x3
};

enum PP_MGMT_MODE {
	PP_MGMT_ALG = 0,
	PP_MGMT_MANUAL = 1,
	PP_MGMT_IGNORE_ECO_SYSTEM = 2
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PP_EN_CTRL_T {
	uint16_t  u2Tag;
	uint16_t  u2Length;

	/* tag specific part */
	uint8_t    u1PpMgmtMode;
	uint8_t    u1DbdcIdx;
	uint8_t    u1PpCtrl;
	uint8_t    u1PpMgmtEn;
	uint16_t   u1PpBitMap;
	uint8_t    u1Reserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PP_ALG_CTRL {
	uint16_t  u2Tag;
	uint16_t  u2Length;

	/* tag specific part */
	uint32_t u4PpTimerIntv;
	uint32_t u4ThrX2_Value;
	uint32_t u4ThrX2_Shift;
	uint32_t u4ThrX3_Value;
	uint32_t u4ThrX3_Shift;
	uint32_t u4ThrX4_Value;
	uint32_t u4ThrX4_Shift;
	uint32_t u4ThrX5_Value;
	uint32_t u4ThrX5_Shift;
	uint32_t u4ThrX6_Value;
	uint32_t u4ThrX6_Shift;
	uint32_t u4ThrX7_Value;
	uint32_t u4ThrX7_Shift;
	uint32_t u4ThrX8_Value;
	uint32_t u4ThrX8_Shift;
	uint8_t u1DbdcIdx;
	uint8_t u1PpAction;
	uint8_t u1Reset;
	uint8_t u1Reserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PP_DSCB_CTRL_T {
	uint16_t  u2Tag;
	uint16_t  u2Length;

	/* tag specific part */
	uint8_t  ucBssInfoIdx;
	uint8_t  fgIsEhtOpPresent;
	uint8_t  fgIsEhtDscbPresent;
	uint8_t  ucEhtCtrl;
	uint8_t  ucEhtCcfs0;
	uint8_t  ucEhtCcfs1;
	uint16_t u2EhtDisSubChanBitmap;
	uint8_t  u1Reserved[1];
} __KAL_ATTRIB_PACKED__;

/*HM command (Tag 0x??) */
struct UNI_CMD_HM {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 *   TAG                            | ID   |
	 *   -------------------------      | --   |
	 */
};
/** @} */
enum UNI_CMD_ID_HM_TAG {
    /** SET **/
	UNI_CMD_HM_TAG_ALG_CTRL = 0x0,
	UNI_CMD_HM_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_HM_ALG_CTRL_T {
	uint16_t  u2Tag;
	uint16_t  u2Length;

    /* tag specific part */
	uint8_t u1HmManualModeEn;
	uint8_t u1ForceObss;
	uint8_t u1ForceBT;
	uint8_t u1HmForcePlan;
	uint8_t u1ObssTimePercntg;
	uint8_t u1BTPercntg;
	uint8_t u1Reserved[6];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_TX_BF
/* BF command (0x33) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BF {
    /* fixed field */
    uint8_t aucReserved[4];
    /* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

typedef void (*PFN_UNI_CMD_BF_HANDLER) (union CMD_TXBF_ACTION *cmd,
	struct UNI_CMD_BF *uni_cmd);

struct UNI_CMD_BF_HANDLE {
	uint32_t u4Size;
	PFN_UNI_CMD_BF_HANDLER pfHandler;
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler;
};

/* BF cmd tags */
enum ENUM_UNI_CMD_BF_TAG {
	UNI_CMD_BF_TAG_SOUNDING_OFF = 0x00,
	UNI_CMD_BF_TAG_SOUNDING_ON = 0x01,
	UNI_CMD_BF_TAG_DATA_PACKET_APPLY = 0x02,
	UNI_CMD_BF_TAG_PFMU_MEM_ALLOCATE = 0x03,
	UNI_CMD_BF_TAG_PFMU_MEM_RELEASE = 0x04,
	UNI_CMD_BF_TAG_PFMU_TAG_READ = 0x05,
	UNI_CMD_BF_TAG_PFMU_TAG_WRITE = 0x06,
	UNI_CMD_BF_TAG_PROFILE_READ = 0x7,
	UNI_CMD_BF_TAG_PROFILE_WRITE = 0x8,
	UNI_CMD_BF_TAG_PFMU_MEM_ALLOC_MAP_READ = 0x09,
	UNI_CMD_BF_TAG_AID_SET = 0x0a,
	UNI_CMD_BF_TAG_STA_REC_READ = 0x0b,
	UNI_CMD_BF_TAG_PHASE_CALIBRATION = 0x0c,
	UNI_CMD_BF_TAG_IBF_PHASE_COMP = 0x0d,
	UNI_CMD_BF_TAG_LNA_GAIN_CONFIG = 0x0e,
	UNI_CMD_BF_TAG_PROFILE_WRITE_20M_ALL = 0x0f,
	UNI_CMD_BF_TAG_APCLIENT_CLUSTER = 0x10,
	UNI_CMD_BF_TAG_HW_ENABLE_STATUS_UPDATE = 0x11,
	UNI_CMD_BF_TAG_BFEE_HW_CTRL = 0x12,
	UNI_CMD_BF_TAG_PFMU_SW_TAG_WRITE = 0x13,
	UNI_CMD_BF_TAG_MOD_EN_CTRL = 0x14,
	UNI_CMD_BF_TAG_CONFIG = 0x15,
	UNI_CMD_BF_TAG_PFMU_DATA_WRITE = 0x16,
	UNI_CMD_BF_TAG_FBRPT_DBG_INFO_READ = 0x17,
	UNI_CMD_BF_TAG_CMD_TXSND_INFO = 0x18,
	UNI_CMD_BF_TAG_CMD_PLY_INFO = 0x19,
	UNI_CMD_BF_TAG_CMD_MU_METRIC = 0x1a,
	UNI_CMD_BF_TAG_CMD_TXCMD = 0x1b,
	UNI_CMD_BF_TAG_CMD_CFG_PHY = 0x1c,
	UNI_CMD_BF_TAG_CMD_SND_CNT = 0x1d,
	UNI_CMD_BF_TAG_CMD_NUM
};

struct UNI_CMD_BF_SOUNDING_STOP {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSndgStop;
	uint8_t ucReserved[2];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BF_SND {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t u1SuMuSndMode;
	uint8_t u1StaNum;
	uint8_t au1Reserved[2];
	uint16_t u2WlanId[4];
	uint32_t u4SndIntv;
} __KAL_ATTRIB_PACKED__;

struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPfmuId;
	uint8_t fgBFer;
	uint8_t u1TxBf;
	uint8_t ucReserved[5];
	uint32_t au4BfPfmuTag1RawData[7];
	uint32_t au4BfPfmuTag2RawData[7];
};

struct UNI_CMD_BF_PROFILE_PN_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPfmuIdx;
	uint8_t ucReserved[2];
};

struct UNI_CMD_BF_PROFILE_PN_WRITE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPfmuIdx;
	uint16_t u2bw;
	uint8_t ucBuf[32];
};

struct UNI_CMD_BF_PROFILE_DATA_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPfmuIdx;
	uint8_t fgBFer;
	uint16_t u2SubCarIdx;
	uint8_t u1TxBf;
	uint8_t ucReserved[3];
};

struct UNI_CMD_BF_PROFILE_DATA_WRITE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2SubCarIdx;
	uint8_t ucPfmuIdx;
	uint8_t ucReserved[5];
	union PFMU_DATA rTxBfPfmuData;
};

struct UNI_CMD_BF_TX_APPLY {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t ucWlanId;
	uint8_t fgETxBf;
	uint8_t fgITxBf;
	uint8_t fgMuTxBf;
	uint8_t fgPhaseCali;
	uint8_t ucReserved[2];
};

struct UNI_CMD_BF_PFMU_MEM_ALLOC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t ucWlanIdx;
	uint8_t u1SuMu;
	uint8_t ucReserved[5];
};

struct UNI_CMD_BF_PFMU_MEM_RLS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucWlanId;
	uint8_t ucReserved[2];
};

/* BF read BF StaRec (Tag11) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BF_STAREC_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint8_t  au1Reserved[2];
} __KAL_ATTRIB_PACKED__;

#endif

struct UNI_CMD_STAREC_MANUAL_ASSOC {
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


__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_THERMAL {
	/* fix field */
	uint8_t au1Reserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_THERMAL_TAG {
	UNI_CMD_THERMAL_TAG_FEATURE_TEMPERATURE_QUERY = 0x0,
	UNI_CMD_THERMAL_TAG_FEATURE_MANUAL_CTRL = 0x1,
	UNI_CMD_THERMAL_TAG_FEATURE_BASIC_INFO_QUERY = 0x2,
	UNI_CMD_THERMAL_TAG_FEATURE_TASK_MANUAL_CTRL = 0x3,
	UNI_CMD_THERMAL_TAG_PROTECT_PARAMETER_CTRL = 0x4,
	UNI_CMD_THERMAL_TAG_PROTECT_BASIC_INFO = 0x5,
	UNI_CMD_THERMAL_TAG_PROTECT_ENABLE = 0x6,
	UNI_CMD_THERMAL_TAG_PROTECT_DISABLE = 0x7,
	UNI_CMD_THERMAL_TAG_PROTECT_DUTY_CONFIG = 0x8,
	UNI_CMD_THERMAL_TAG_PROTECT_MECH_INFO = 0x9,
	UNI_CMD_THERMAL_TAG_PROTECT_DUTY_INFO = 0xA,
	UNI_CMD_THERMAL_TAG_PROTECT_STATE_ACT = 0xB,
	UNI_CMD_THERMAL_TAG_FEATURE_DDIE_INFO = 0xC,
	UNI_CMD_THERMAL_TAG_FEATURE_ADC_TEMPERATURE_QUERY = 0xD,
	UNI_CMD_THERMAL_TAG_NUM
};

enum THERMAL_SENSOR_INFO_ACTION {
	THERMAL_SENSOR_INFO_TEMPERATURE = 0,
	THERMAL_SENSOR_INFO_ADC,
	THERMAL_SENSOR_INFO_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_THERMAL_SENSOR_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucThermalCtrlFormatId;
	uint8_t ucActionIdx;
	uint8_t ucBandIdx;
	uint8_t ucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_THERMAL_DDIE_SENSOR_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucThermalCtrlFormatId;
	uint8_t ucActionIdx;
	uint8_t ucSensorIdx;
	uint8_t ucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_THERMAL_TEMP_ADC_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucThermalCtrlFormatId;
	uint8_t ucType;
	uint8_t ucIndex;
	uint8_t ucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RSSI_MONITOR {
	/*fixed field*/
	uint8_t aucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                        | ID   | structure
	*   -------------------------  | ---- | -------------
	*   UNI_CMD_RSSI_MONITOR_SET   | 0x0  | UNI_CMD_RSSI_MONITOR_SET_T
	*/
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_RSSI_MONITOR_TAG {
	UNI_CMD_RSSI_MONITOR_TAG_SET = 0x0,
	UNI_CMD_RSSI_MONITOR_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RSSI_MONITOR_SET {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* Tag specific part */
	uint8_t fgEnable;
	int8_t  cMaxRssi;
	int8_t  cMinRssi;
	uint8_t ucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MQM_UPDATE_MU_EDCA {
	/*fixed field*/
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                                 | ID  | structure
	*   ----------------------------------- | ----| -------------
	*   UNI_CMD_MQM_UPDATE_MU_EDCA_PARMS    | 0x0 |UNI_CMD_MQM_UPDATE_MU_EDCA_T
	*/
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_CMD_MQM_UPDATE_MU_EDCA_TAG {
	UNI_CMD_MQM_UPDATE_MU_EDCA_TAG_PARMS = 0x0,
	UNI_CMD_MQM_UPDATE_MU_EDCA_TAG_NUM
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MU_EDCA_PARAMS {
	uint8_t ucECWmin;
	uint8_t ucECWmax;
	uint8_t ucAifsn;
	uint8_t ucIsACMSet;
	uint8_t ucMUEdcaTimer;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UPDATE_MU_EDCA {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint8_t  ucBssIndex;
	uint8_t  fgIsQBSS;
	uint8_t  ucWmmSet;
	uint8_t  ucReserved;
	struct UNI_CMD_MU_EDCA_PARAMS arMUEdcaParams[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PERF_IND {
	/*fixed field*/
	uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*  TAG                             | ID   | structure
	*  -------------                   | -----| -------------
	*  UNI_CMD_ID_PERF_IND             | 0x0  | UNI_CMD_ID_PERF_IND_PARM_T
	*  UNI_CMD_PERF_IND_TAG_TPUT_FACTOR| 0x1  | UNI_CMD_PERF_IND_TPUT_FACTOR

	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_PERF_IND_TAG {
	UNI_CMD_PERF_IND_TAG_PARM = 0,
	UNI_CMD_PERF_IND_TAG_TPUT_FACTOR = 1,
	UNI_CMD_PERF_IND_TAG_PARM_V2 = 2,
	UNI_CMD_PERF_IND_TAG_NUM
};
__KAL_ATTRIB_PACKED_FRONT__
struct STRU_UNI_CMD_PERF_IND_PARM {
	uint32_t u4CurTxBytes;    /* in Bps */
	uint32_t u4CurRxBytes;    /* in Bps */
	uint16_t u2CurRxRate;    /* Unit 500 Kbps */
	uint8_t ucCurRxRCPI0;
	uint8_t ucCurRxRCPI1;
	uint8_t ucCurRxNss;
	uint8_t ucCurRxNss2;
	uint16_t u2Reserve;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PERF_IND_PARM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucCmdVer;
	uint8_t aucPadding0[1];
	uint16_t u2CmdLen;       // cmd size including common part and body.
	uint32_t u4VaildPeriod;   /* in ms */
	uint8_t ucBssNum;
	uint8_t  ucReserve[3];
	struct STRU_UNI_CMD_PERF_IND_PARM rUniCmdParm[MAX_BSSID_NUM];
	//uint32_t au4Padding[62]; /* reserve for future*/
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PERF_IND_TPUT_FACTOR {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4WtblBitMap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ID_FRM_IND_FROM_HOST {
	/*fixed field*/
	uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
        *
        *  TAG                          | ID   | structure
        *  -------------                | -----| -------------
        *  UNI_CMD_ID_FRM_IND_FROM_HOST   | 0x0  | UNI_CMD_ID_FRM_IND_FROM_HOST_PARM_T
        */
} __KAL_ATTRIB_PACKED__;

/* RDD set command Tag */
enum ENUM_UNI_CMD_FRM_IND_FROM_HOST_TAG {
	UNI_CMD_FRM_IND_FROM_HOST_TAG_PARM = 0,
	UNI_CMD_FRM_IND_FROM_HOST_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FRM_IND_FROM_HOST_PARM {
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;
	uint8_t  ucStaIdx;
	uint8_t  ucBssIdx;
	uint8_t  ucTransmitType;
	uint8_t  ucProtocolType;
	uint8_t  ucProtocolSubType;
	uint8_t  ucRateValid;
	uint8_t  aucPadding1[2];
	uint32_t u4Rate;
	uint32_t u4Len;
	uint8_t  TxS;
	uint8_t  aucPadding3[3];
	uint8_t  aucPadding4[64];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE {
	/* fix field*/
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                            | ID  | structure
	*   -------------------------------|-----|--------------
	*   UNI_CMD_TESTMODE_TAG_CTRL      | 0x0 | UNI_CMD_TESTMODE_CTRL
	*   UNI_CMD_TESTMODE_TAG_LISTMODE  | 0x1 | UNI_CMD_TESTMODE_LISTMODE
	*   UNI_CMD_TESTMODE_TAG_XO_CAL    | 0x2 | UNI_CMD_TESTMODE_XO_CAL
	*   UNI_CMD_TESTMODE_TAG_PL_CAL    | 0x3 | UNI_CMD_TESTMODE_PL_CAL
	*/
} __KAL_ATTRIB_PACKED__;

/** testmode RF test command TLV List */
enum ENUM_UNI_CMD_TESTMODE_CTRL_TAG {
	UNI_CMD_TESTMODE_TAG_CTRL = 0x0,
	UNI_CMD_TESTMODE_TAG_LISTMODE = 0x1,
	UNI_CMD_TESTMODE_TAG_XO_CAL = 0x2,
	UNI_CMD_TESTMODE_TAG_PL_CAL = 0x3,
	UNI_CMD_TESTMODE_TAG_NUM
};

/** @addtogroup UNI_CMD_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_CTRL(0x00)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * to set testmode RF parameter.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x00
 * @param[in] u2Length      the length of this TLV, should be 8
 * @param[in] ucAction      set action of testmode
 * @param[in] aucReserved   Reserved
 * @param[in] u4OpMode      Operation mode
 * @param[in] u4ChannelFreq Frequency of channel
 * @param[in] rRfATInfo     RF information
 */
/* Set testmode RF parameter cmd struct (Tag 0x00) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucAction;
	uint8_t aucReserved[3];
	union {
		uint32_t u4OpMode;
		uint32_t u4ChannelFreq;
		struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T rRfATInfo;
	}u;
}__KAL_ATTRIB_PACKED__;
/** @} */

#define TESTMODE_LISTMODE_DATA_LEN	780

/** @addtogroup UNI_CMD_ID_TESTMODE_LISTMODE
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_LISTMODE(0x01)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * to set testmode listmode.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x01
 * @param[in] u2Length      the length of this TLV
 * @param[in] aucData       list mode data
 */
/* Set testmode listmode cmd struct (Tag 0x01) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_LISTMODE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucData[TESTMODE_LISTMODE_DATA_LEN];
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_CMD_ID_TESTMODE_XO_CAL
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_XO_CAL(0x02)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * Xtal calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u4CalType     the type of calibration
 * @param[in] u4ClkSrc      the clock source for calibration
 * @param[in] u4Mode        the mode for calibration
 * @param[in] u4TargetReq   the target frequency for calibration
 */
/* Set testmode XO calibratrion cmd struct (Tag 0x02) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_XO_CAL_REQ {
	uint32_t u4CalType;
	uint32_t u4ClkSrc;
	uint32_t u4Mode;
	uint32_t u4TargetReq;
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_CMD_ID_TESTMODE_XO_CAL
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_XO_CAL(0x02)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * to set testmode Xtal calibration.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x02
 * @param[in] u2Length      the length of this TLV
 * @param[in] rXoReq        XO request parameters
 */
/* Set testmode XO calibratrion cmd struct (Tag 0x02) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_XO_CAL {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_CMD_TESTMODE_XO_CAL_REQ rXoReq;
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_CMD_ID_TESTMODE_PL_CAL
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_PL_CAL(0x03)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * production line calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u4BandIdx		dbdc band index
 * @param[in] u4PLCalId		production line calibration id
 * @param[in] u4Action		calibration action
 * @param[in] u4Flags		reserved field
 * @param[in] u4InCnt		input parameter count
 * @param[in] u4InData		input parameter data
 */
/* Set testmode PL calibratrion cmd struct (Tag 0x03) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_PL_CAL_REQ {
	uint32_t u4BandIdx;
	uint32_t u4PLCalId;
	uint32_t u4Action;
	uint32_t u4Flags;
	uint32_t u4InCnt;
	uint32_t u4InData[PLCAL_MAX_CNT];
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_CMD_ID_TESTMODE_PL_CAL
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_TAG_PL_CAL(0x03)
 * of UNI_CMD_ID_TESTMODE_CTRL command (0x46)
 * to do testmode production line calibration.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x03
 * @param[in] u2Length      the length of this TLV
 * @param[in] rPlReq        PL request parameters
 */
/* Set testmode PL calibratrion cmd struct (Tag 0x03) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_PL_CAL {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_CMD_TESTMODE_PL_CAL_REQ rPlReq;
} __KAL_ATTRIB_PACKED__;
/** @} */

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_RX_STAT {
	/* fix field*/
	uint8_t u1BandIdx;
	uint8_t ucReserved[3];
	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*TAG                                 |ID |structure
	*------------------------------------|---|--------------
	*UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL|0x8|UNI_CMD_TESTMODE_RX_GET_STAT_ALL_T
	*/
}__KAL_ATTRIB_PACKED__;

/** @} */

/** testmode rx statistic command TLV List */
enum ENUM_UNI_CMD_TESTMODE_RX_TAG {
	UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL = 0x8,
	UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL_V2 = 0x9,
	UNI_CMD_TESTMODE_RX_TAG_GET_STAT_TLV = 0xA,
	UNI_CMD_TESTMODE_RX_TAG_NUM,
};

/** @addtogroup UNI_CMD_ID_TESTMODE_RX_STAT
 * @{
 */
/**
 * This structure is used for UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL(0x08)
 * of UNI_CMD_ID_TESTMODE_RX_STAT command (0x32)
 * to set user of band.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x08
 * @param[in] u2Length      the length of this TLV, should be 8
 * @param[in] u1DbdcIdx     choose band
 * @param[in] aucReserved   Reserved
 */
/* Get rx info all cmd struct (Tag 0x08) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t u1DbdcIdx;
	uint8_t u1Data;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;
/** @} */

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ICS {
	/*fixed field*/
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                               | ID   | structure
	*   -------------------    | ----| -------------
	*   UNI_CMD_ICS_CTRL       | 0x0 | UNI_CMD_ICS_SNIFFER_T
	*/
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_ICS_TAG {
	UNI_CMD_ICS_TAG_CTRL = 0x0,
	UNI_CMD_ICS_TAG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ICS_SNIFFER {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint8_t ucCmdVer;
	uint8_t ucAction;
	uint16_t u2CmdLen;
	uint8_t ucModule;
	uint8_t ucFilter;
	uint8_t ucOperation;
	uint8_t aucPadding0;
	uint16_t ucCondition[7];
	uint8_t aucPadding1[62];
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_SUPPORT_PHY_ICS == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PHY_ICS {
	/*fixed field*/
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
       /**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   -------------------              | ----| -------------
	*   UNI_CMD_PHY_ICS_EVENT_CTRL       | 0x0 | UNI_CMD_PHY_ICS_EVENT_T
	*   UNI_CMD_PHY_ICS_START_CTRL       | 0x1 | UNI_CMD_PHY_ICS_START_T
	*/
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_PHY_ICS_TAG {
	UNI_CMD_PHY_ICS_EVENT_TAG_CTRL = 0x0,
	UNI_CMD_PHY_ICS_START_TAG_CTRL = 0x1,
	UNI_CMD_PHY_ICS_TAG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PHY_ICS_EVENT {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint8_t ucBandIdx;
	uint8_t ucPartition;
	uint16_t u2EventGroup;
	uint32_t u4EventID;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PHY_ICS_START {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint16_t u2Action;
	uint16_t u2Timer;
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_PHY_CTRL_LIST_DUMP {

	/*fixed field*/
	uint8_t ucAction;
	uint8_t aucReserved[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 *   TAG                             | ID  | structure
	 *   -------------                   | ----| -------------
	 *   UNI_CMD_PHY_LIST_DUMP_RESERVED  | 0x0 | UNI_CMD_PHY_LSIT_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_HW_PHY    | 0x1 | UNI_CMD_PHY_LSIT_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_HW_EMLSR  | 0x2 | UNI_CMD_PHY_LSIT_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_SW_PHY    | 0x3 | UNI_CMD_PHY_LSIT_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_SW_EMLSR  | 0x4 | UNI_CMD_PHY_LSIT_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_HW_PHY1   | 0x5 | UNI_CMD_PHY_LIST_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_SW_PHY1   | 0x6 | UNI_CMD_PHY_LIST_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_HW_PHY2   | 0x7 | UNI_CMD_PHY_LIST_DUMP_T
	 *   UNI_CMD_PHY_LIST_DUMP_SW_PHY2   | 0x8 | UNI_CMD_PHY_LIST_DUMP_T
	 */

} __KAL_ATTRIB_PACKED__;

/* Suspend command Tag */

enum ENUM_UNI_CMD_PHY_LIST_DUMP_TAG {
	UNI_CMD_PHY_LIST_DUMP_TAG_RESERVED = 0,
	UNI_CMD_PHY_LIST_DUMP_TAG_HW_PHY = 1,
	UNI_CMD_PHY_LIST_DUMP_TAG_HW_EMLSR = 2,
	UNI_CMD_PHY_LIST_DUMP_TAG_SW_PHY = 3,
	UNI_CMD_PHY_LIST_DUMP_TAG_SW_EMLSR = 4,
	UNI_CMD_PHY_LIST_DUMP_TAG_HW_PHY1 = 5,
	UNI_CMD_PHY_LIST_DUMP_TAG_SW_PHY1 = 6,
	UNI_CMD_PHY_LIST_DUMP_TAG_HW_PHY2 = 7,
	UNI_CMD_PHY_LIST_DUMP_TAG_SW_PHY2 = 8,
	UNI_CMD_PHY_LIST_DUMP_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__

struct UNI_CMD_PHY_LIST_DUMP_CR {

	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;
	uint32_t u4Stamp;
} __KAL_ATTRIB_PACKED__;


__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACS_POLICY {
	/*fixed field*/
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                            | ID  | structure
	*   -------------------------------| ----| --------------------------
	*   UNI_CMD_ACS_POLICY_TAG_SETTING | 0x0 | UNI_CMD_ACS_POLICY_SETTING
	*/
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_ACS_POLICY_TAG {
	UNI_CMD_ACS_POLICY_TAG_SETTING = 0x0,
	UNI_CMD_ACS_POLICY_TAG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACS_POLICY_SETTING {
	uint16_t u2Tag;
	uint16_t u2Length;

	/* tag specific part */
	uint8_t ucBssIdx;
	uint8_t ucPolicy;
	uint8_t aucAddr[MAC_ADDR_LEN];

} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_ID_EFUSE_CONTROL command (0x2D) to access EFUSE
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucReserved    Fixed field
 * @param[in] aucTlvBuffer   TLVs
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EFUSE {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 *TAG                               |ID  |structure
	 *----------------------------------|----|-------------
	 *UNI_CMD_EFUSE_CTRL_TAG_ACCESS     |0x01|UNI_CMD_ACCESS_EFUSE
	 *UNI_CMD_EFUSE_CTRL_TAG_BUFFER_MODE|0x02|UNI_CMD_EFUSE_BUFFER_MODE
	 *UNI_CMD_EFUSE_CTRL_TAG_FREE_BLOCK |0x03|UNI_CMD_EFUSE_FREE_BLOCK
	 *UNI_CMD_EFUSE_CTRL_TAG_BUFFER_RD  |0x04|UNI_CMD_EFUSE_BUFFER_MODE_READ
	 */
}__KAL_ATTRIB_PACKED__;

enum UNI_CMD_EFUSE_CTRL_TAG {
	UNI_CMD_EFUSE_CTRL_TAG_ACCESS                     = 1,
	UNI_CMD_EFUSE_CTRL_TAG_BUFFER_MODE,
	UNI_CMD_EFUSE_CTRL_TAG_FREE_BLOCK,
	UNI_CMD_EFUSE_CTRL_TAG_BUFFER_RD,
	UNI_CMD_EFUSE_CTRL_TAG_MAX_NUM
};

/** This structure is used for UNI_CMD_ID_EFUSE_CONTROL command (0x2D)
 * to do EFUSE eFuse or Buffer mode operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag
 * @param[in] u2Length
 * @param[in] ucSourceMode       0: eFuse mode; 1: Buffer mode
 * @param[in] ucContentFormat    0: Bin Content;
 *                               1: Whole Content;
 *                               2: Multiple Sections
 * @param[in] u2Count            Total number of aBinContent elements
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EFUSE_BUFFER_MODE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucSourceMode;       /* 0: eFuse mode; 1: Buffer mode */
	uint8_t  ucContentFormat;    /* 0: Bin Content;
                                    1: Whole Content;
                                    2: Multiple Sections */
	uint16_t u2Count;            /* Total number of aBinContent elements */
	uint8_t aucBinContent[BUFFER_MODE_CONTENT_MAX];
}__KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_ID_EFUSE_CONTROL command (0x2D)
 * to do EFUSE ACCESS operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag
 * @param[in] u2Length
 * @param[in] u4Address     for access address
 * @param[in] u4Valid       Compatible to original CMD field.
 *                          Currently only use bit 0
 *                          [0]:1 -> valid, [0]:0 -> invalid
 * @param[in] aucData[16]   get address value
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_ACCESS_EFUSE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Address; /* for access address */
	uint32_t u4Valid;   /* Compatible to original CMD field.
                           Currently only use bit 0
                           [0]:1 -> valid, [0]:0 -> invalid */
	uint8_t aucData[16];
}__KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_ID_EFUSE_CONTROL command (0x2D)
 * to do EFUSE free block operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag
 * @param[in] u2Length
 * @param[in] ucGetFreeBlock   the get free block number
 * @param[in] ucVersion        0: original format ; 1: modified format
 * @param[in] ucDieIndex       for 7663, 0: D die ; 1: A die
 * @param[in] ucReserved
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EFUSE_FREE_BLOCK {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucGetFreeBlock; /* the get free block number */
	uint8_t  ucVersion; /* 0: original format ; 1: modified format */
	uint8_t  ucDieIndex; /* for 7663, 0: D die ; 1: A die */
	uint8_t  ucReserved;
}__KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_ID_EFUSE_CONTROL command (0x2D)
 * to do EFUSE or Buffer mode read data operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag
 * @param[in] u2Length
 * @param[in] u1SourceMode      0: eFuse mode; 1: Buffer mode
 * @param[in] u1ContentFormat   0: Bin Content;
 *                              1: Whole Content; 2: Multiple Sections
 * @param[in] u2Offset          Read Offset
 * @param[in] u2Count           Read Total Counts
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EFUSE_BUFFER_MODE_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  u1SourceMode;       /* 0: eFuse mode; 1: Buffer mode */
	uint8_t  u1ContentFormat;    /* 0: Bin Content; 1: Whole Content;
                                    2: Multiple Sections */
	uint16_t u2Offset;           /* Read Offset */
	uint32_t u2Count;            /* Read Total Counts */
}__KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_RTT
/* RTT command (0x5D) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RTT {
	/* fixed field */
	uint8_t ucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                            |  ID  | structure
	*   -------------------------------|------|--------------
	*   UNI_CMD_RTT_TAG_GET_CAPA       | 0x00 | UNI_CMD_RTT_GET_CAPA_T
	*   UNI_CMD_RTT_TAG_RANGE_REQ_MC   | 0x01 | UNI_CMD_RTT_RANGE_REQ_MC_T
	*   UNI_CMD_RTT_TAG_RANGE_RSP_MC   | 0x02 | UNI_CMD_RTT_RANGE_RSP_MC_T
	*   UNI_CMD_RTT_TAG_RANGE_REQ_AZ_NTB | 0x03 | UNI_CMD_RTT_REQ_AZ_NTB_T
	*/
} __KAL_ATTRIB_PACKED__;

/* RTT command TLV List */
enum ENUM_UNI_CMD_RTT_TAG {
	UNI_CMD_RTT_TAG_GET_CAPA = 0,
	UNI_CMD_RTT_TAG_RANGE_REQ_MC = 1,      /* 11mc initiator */
	UNI_CMD_RTT_TAG_RANGE_RSP_MC = 2,      /* 11mc responder */
	UNI_CMD_RTT_TAG_RANGE_REQ_AZ_NTB = 3,  /* 11AZ NTB */
	UNI_CMD_RTT_TAG_NUM
};
/* Get RTT Capabilities (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RTT_GET_CAPA_T {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* 11mc ranging request (Tag1) */
struct RTT_CONFIG_MC {
	uint8_t aucAddr[MAC_ADDR_LEN]; /* peer device mac address */
	uint8_t eType; /* enum ENUM_RTT_TYPE */
	uint8_t ePeer; /* enum ENUM_RTT_PEER_TYPE */
	struct WIFI_CHANNEL_INFO rChannel;
	uint16_t u2BurstPeriod;
	uint16_t u2NumBurstExponent;
	uint16_t u2PreferencePartialTsfTimer;
	uint8_t ucNumFramesPerBurst;
	uint8_t ucNumRetriesPerRttFrame;
	uint8_t ucNumRetriesPerFtmr;
	uint8_t ucLciRequest;
	uint8_t ucLcrRequest;
	uint8_t ucBurstDuration;
	uint8_t ePreamble; /* enum ENUM_WIFI_RTT_PREAMBLE */
	uint8_t eBw; /* enum ENUM_WIFI_RTT_BW */

	/* bellow are for internal useages */
	uint8_t eBand; /* enum ENUM_BAND */
	uint8_t ucPrimaryChannel;
	uint8_t ucS1;
	uint8_t ucS2;
	uint8_t eChannelWidth; /* enum ENUM_CHANNEL_WIDTH */
	uint8_t ucBssIndex;
	uint8_t eEventType;  /* enum ENUM_LOC_EVENT_TYPE_T*/
	uint8_t ucASAP;
	uint8_t ucFtmMinDeltaTime; //mc: UNIT:100us
	uint8_t ucReserved; // 4 byte align
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RTT_RANGE_REQ_MC_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSeqNum;
	uint8_t fgEnable;              /* request or cancel */
	uint8_t ucConfigNum;
	uint8_t ucPaddings[5];
	struct RTT_CONFIG_MC arRttConfigs[CFG_RTT_MAX_CANDIDATES];
} __KAL_ATTRIB_PACKED__;

/* 11mc ranging request (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RTT_RANGE_RSP_MC_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSeqNum;
	uint8_t fgEnable;              /* request or cancel */
	uint8_t ucConfigNum;
	uint8_t ucPaddings[5];
	struct RTT_CONFIG arRttConfigs[CFG_RTT_MAX_CLIENTS];
} __KAL_ATTRIB_PACKED__;

/* 11az NTB ranging request (Tag3) */
struct RTT_CONFIG_AZ_NTB {
	uint8_t aucAddr[MAC_ADDR_LEN]; /* peer device mac address */
	uint8_t eType; /* enum ENUM_RTT_TYPE */
	uint8_t ePeer; /* enum ENUM_RTT_PEER_TYPE */
	struct WIFI_CHANNEL_INFO rChannel;
	uint16_t u2BurstPeriod;
	uint16_t u2NumBurstExponent;
	uint16_t u2PreferencePartialTsfTimer;
	uint8_t ucNumFramesPerBurst;
	uint8_t ucNumRetriesPerRttFrame;
	uint8_t ucNumRetriesPerFtmr;
	uint8_t ucLciRequest;
	uint8_t ucLcrRequest;
	uint8_t ucBurstDuration;
	uint8_t ePreamble; /* enum ENUM_WIFI_RTT_PREAMBLE */
	uint8_t eBw; /* enum ENUM_WIFI_RTT_BW */

	/* bellow are for internal useages */
	uint8_t eBand; /* enum ENUM_BAND */
	uint8_t ucPrimaryChannel;
	uint8_t ucS1;
	uint8_t ucS2;
	uint8_t eChannelWidth; /* enum ENUM_CHANNEL_WIDTH */
	uint8_t ucBssIndex;
	uint8_t eEventType;  /* enum ENUM_LOC_EVENT_TYPE_T*/
	uint8_t ucASAP;
	uint8_t ucFtmMinDeltaTime; //mc: UNIT:100us
	uint8_t ucReserved; // 4 byte align

	/* 11az */
	uint64_t u8NtbMinMeasurementTime;
	uint64_t u8NtbMaxMeasurementTime;
	uint8_t ucI2rLmrFeedback;
	uint8_t ucImmeR2iFeedback;
	uint8_t ucImmeI2rFeedback;
	uint8_t ucForceReplyI2rLmr;
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RTT_RANGE_REQ_AZ_NTB_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSeqNum;
	uint8_t fgEnable;              /* request or cancel */
	uint8_t ucConfigNum;
	uint8_t ucPaddings[5];
	struct RTT_CONFIG_AZ_NTB arRttConfigs[CFG_RTT_MAX_CANDIDATES];
} __KAL_ATTRIB_PACKED__;

#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_NAN
/* NAN set command (0x56) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NAN {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                      | ID  | structure
	*   -------------------------|-----|--------------
	*   UNI_CMD_NAN              | 0x0 | UNI_CMD_NAN_T
	*/
} __KAL_ATTRIB_PACKED__;

/* NAN set command Tag */
enum ENUM_UNI_CMD_NAN_TAG {
	UNI_CMD_NAN_TAG_SET_MASTER_PREFERENCE = 0,
	UNI_CMD_NAN_TAG_PUBLISH = 1,
	UNI_CMD_NAN_TAG_CANCEL_PUBLISH = 2,
	UNI_CMD_NAN_TAG_UPDATE_PUBLISH = 3,
	UNI_CMD_NAN_TAG_SUBSCRIBE = 4,
	UNI_CMD_NAN_TAG_CANCEL_SUBSCRIBE = 5,
	UNI_CMD_NAN_TAG_TRANSMIT = 6,
	UNI_CMD_NAN_TAG_ENABLE_REQUEST = 7,
	UNI_CMD_NAN_TAG_DISABLE_REQUEST = 8,
	UNI_CMD_NAN_TAG_UPDATE_AVAILABILITY = 9,
	UNI_CMD_NAN_TAG_UPDATE_CRB = 10,
	UNI_CMD_NAN_TAG_CRB_HANDSHAKE_TOKEN = 11,
	UNI_CMD_NAN_TAG_MANAGE_PEER_SCH_RECORD = 12,
	UNI_CMD_NAN_TAG_MAP_STA_RECORD = 13,
	UNI_CMD_NAN_TAG_RANGING_REPORT_DISC = 14,
	UNI_CMD_NAN_TAG_FTM_PARAM = 15,
	UNI_CMD_NAN_TAG_UPDATE_PEER_UAW = 16,
	UNI_CMD_NAN_TAG_UPDATE_ATTR = 17,
	UNI_CMD_NAN_TAG_UPDATE_PHY_SETTING = 18,
	UNI_CMD_NAN_TAG_UPDATE_POTENTIAL_CHNL_LIST = 19,
	UNI_CMD_NAN_TAG_UPDATE_AVAILABILITY_CTRL = 20,
	UNI_CMD_NAN_TAG_UPDATE_PEER_CAPABILITY = 21,
	UNI_CMD_NAN_TAG_ADD_CSID = 22,
	UNI_CMD_NAN_TAG_MANAGE_SCID = 23,
	UNI_CMD_NAN_TAG_CHANGE_ADDRESS = 24,
	UNI_CMD_NAN_TAG_SET_SCHED_VERSION = 25,
	UNI_CMD_NAN_TAG_SET_DW_INTERVAL = 26,
	UNI_CMD_NAN_TAG_ENABLE_UNSYNC = 30,
	UNI_CMD_NAN_TAG_GET_DEVICE_INFO = 33,
	UNI_CMD_NAN_TAG_VENDOR_PAYLOAD = 35,
	UNI_CMD_NAN_TAG_LOWPOWER_CTRL = 37,
	UNI_CMD_NAN_TAG_SET_HOST_ELECTION = 42,
	UNI_CMD_NAN_TAG_SET_ELECTION_ROLE = 43,
	UNI_CMD_NAN_TAG_INSTANT_COMM_MODE = 44,

	/* EXT_CMD Part */
	/* Reserve for vendor r, 100 ~ 199 */

	/* Reserve for vendor s, 200 ~ 299 */
	UNI_CMD_NAN_TAG_EXT_CUSTOM_CMD = 200,
	NAN_CMD_EXT_TAG_CLUSTER = 252,
	NAN_CMD_EXT_TAG_P2P = 255,
	NAN_CMD_EXT_TAG_MERGING_DIRECTION = 256,
	NAN_CMD_EXT_TAG_SYNC = 257,
	NAN_CMD_EXT_TAG_MERGING = 258,
	NAN_CMD_EXT_TAG_SCHEDULING = 259,
	NAN_CMD_EXT_TAG_USD = 264,
	NAN_CMD_EXT_TAG_ASC = 265,
	UNI_CMD_NAN_TAG_MAX_NUM
};

/* Set Master Performance (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NAN_SET_MASTER_PREFERENCE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucMasterPreference;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* Enable NAN (Tag7) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NAN_ENABLE_REQUEST {
	uint16_t u2Tag;
	uint16_t u2Length;

	struct NanEnableRequest request;
} __KAL_ATTRIB_PACKED__;

/* Disable NAN (Tag8) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NAN_DISABLE_REQUEST {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucPadding[4];
} __KAL_ATTRIB_PACKED__;

/* Cancel Publish (Tag2), Cancel Subscribe (Tag5) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_NAN_CANCEL_REQUEST {
	/* Indicate the service is publish or
	 * subscribe, 1=publish, 0=subscribe
	 */
	uint16_t publish_or_subscribe;
	/* Publish or Subscribe Id of an earlier Publish/Subscribe */
	uint16_t publish_subscribe_id;
} __KAL_ATTRIB_PACKED__;
#endif

#if CFG_SUPPORT_CSI
/* CSI set command (0x48) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI {
	uint8_t ucBandIdx;
	uint8_t ucReserved[3];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*   TAG                           | ID   | structure
	*   ---------------------------   |----- |--------------
	*    UNI_CMD_CSI_STOP             | 0x00 | UNI_CMD_CSI_STOP
	*    UNI_CMD_CSI_START            | 0x01 | UNI_CMD_CSI_START
	*    UNI_CMD_CSI_SET_FRAME_TYPE   | 0x02 | UNI_CMD_CSI_SET_FRAME_TYPE
	*    UNI_CMD_CSI_SET_CHAIN_NUMBER | 0x03 | UNI_CMD_CSI_SET_CHAIN_NUMBER
	*    UNI_CMD_CSI_SET_FILTER_MODE  | 0x04 | UNI_CMD_CSI_SET_FILTER_MODE
	*/
} __KAL_ATTRIB_PACKED__;

/* CSI set command Tag */
enum ENUM_UNI_CMD_CSI_TAG {
	UNI_CMD_CSI_TAG_STOP = 0,
	UNI_CMD_CSI_TAG_START = 1,
	UNI_CMD_CSI_TAG_SET_FRAME_TYPE = 2,
	UNI_CMD_CS_TAGI_SET_CHAIN_NUMBER = 3,
	UNI_CMD_CSI_TAG_SET_FILTER_MODE = 4,
};

/* Stop capturing CSI data (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI_STOP {
	uint16_t   u2Tag;
	uint16_t   u2Length;
} __KAL_ATTRIB_PACKED__;

/* Start capturing CSI data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI_START {
	uint16_t   u2Tag;
	uint16_t   u2Length;
} __KAL_ATTRIB_PACKED__;

/* Set frame type (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI_SET_FRAME_TYPE {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint8_t    ucFrameTypeIndex;
	uint32_t    u4FrameType;
	uint8_t    aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* Set max chain number (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI_SET_CHAIN_NUMBER {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint8_t    ucMaxChain;
	uint8_t    aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* Set filter mode (Tag4) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_CSI_SET_FILTER_MODE {
	uint16_t   u2Tag;
	uint16_t   u2Length;
	uint8_t    ucOperation;
	uint8_t    aucPadding[1];
	uint8_t    aucMACAddr[6];
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_VOLT_INFO == 1)
struct UNI_CMD_SEND_VOLT_INFO {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

struct UNI_CMD_SEND_VOLT_INFO_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* cmd body */
	struct CMD_SEND_VOLT_INFO_T rVolt;
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_SEND_VOLT_INFO_TAG {
	UNI_CMD_SEND_VOLT_INFO_TAG_BASIC = 0x0,
	UNI_CMD_SEND_VOLT_INFO_TAG_NUM
};
#endif /* CFG_VOLT_INFO */
#if CFG_SUPPORT_PKT_OFLD
/* Packet offload set command (0x60) */
struct UNI_CMD_PKT_OFLD {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                                  | ID  | structure
	*   --------------------------------|---|--------------
	*   UNI_CMD_PKT_OFLD_TAG_APF_INSTALL|0x0| UNI_CMD_PKT_OFLD_GENERAL_OP_T
	*   UNI_CMD_PKT_OFLD_TAG_APF_QUERY  |0x1| UNI_CMD_PKT_OFLD_GENERAL_OP_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Packet offload set command TLV List */
enum ENUM_UNI_CMD_PKT_OFLD_TAG {
	UNI_CMD_PKT_OFLD_TAG_APF_INSTALL = 0,
	UNI_CMD_PKT_OFLD_TAG_APF_QUERY,
	UNI_CMD_PKT_OFLD_TAG_IGMP_OFLD,
	UNI_CMD_PKT_OFLD_TAG_RA_OFLD,
	UNI_CMD_PKT_OFLD_TAG_NUM
};

/* Packet offload setting (Tag0) */
struct UNI_CMD_PKT_OFLD_GENERAL_OP {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucType;
	uint8_t ucOp;
	uint8_t ucFragNum;
	uint8_t ucFragSeq;
	uint32_t u4TotalLen;
	uint32_t u4BufLen;
	uint8_t aucBuf[PKT_OFLD_BUF_SIZE];
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_KEEP_ALIVE {
	/* fixed field */
	uint8_t aucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                        | ID   | structure
	*   -------------------------  | ---- | -------------
	*   UNI_CMD_KEEP_ALIVE_SET     | 0x0  | UNI_CMD_KEEP_ALIVE_SET_T
	*/
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_KEEP_ALIVE_TAG {
	UNI_CMD_KEEP_ALIVE_TAG_SET = 0x0,
	UNI_CMD_KEEP_ALIVE_TAG_MAX_NUM
};
/** @addtogroup UNI_CMD_ID_KEEP_ALIVE
 *  @{
 */
/** This structure is used for UNI_CMD_KEEP_ALIVE_SET tag(0x0) of
 * UNI_CMD_ID_KEEP_ALIVE command (0x61)
 * to periodic send a packet to keep connection alive
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag        should be 0x0
 * @param[in] u2Length     should be sizeof(UNI_CMD_KEEP_ALIVE_SET_T)
 * @param[in] fgEnable
 * @param[in] ucIndex
 * @param[in] u2IpPktLen
 * @param[in] pIpPkt
 * @param[in] ucSrcMacAddr
 * @param[in] ucDstMacAddr
 * @param[in] u4PeriodMsec

 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_KEEP_ALIVE_SET {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* Tag specific part */
	uint8_t fgEnable;
	uint8_t  ucIndex;
	uint16_t u2IpPktLen;
	uint8_t  pIpPkt[256];
	uint8_t  ucSrcMacAddr[6];
	uint8_t  ucDstMacAddr[6];
	uint32_t u4PeriodMsec;
} __KAL_ATTRIB_PACKED__;

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
struct UNI_CMD_MDNS_RECORDE {
	/* fixed field */
	uint8_t aucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                        | ID   | structure
	*   -------------------------  | ---- | -------------
	*   UNI_CMD_KEEP_ALIVE_SET     | 0x0  | UNI_CMD_KEEP_ALIVE_SET_T
	*/
} __KAL_ATTRIB_PACKED__;


enum UNI_CMD_MDNS_RECORDE_TAG {
	UNI_CMD_MDNS_RECORDE_TAG_ENABLE = 0x0,
	UNI_CMD_MDNS_RECORDE_TAG_GET_HIT,
	UNI_CMD_MDNS_RECORDE_TAG_GET_MISS,
	UNI_CMD_MDNS_RECORDE_TAG_IPV6_WAKEUP,
	UNI_CMD_MDNS_RECORDE_TAG_MAX_NUM
};

struct UNI_CMD_MDNS_RECORDE_ENABLE {
	uint16_t u2Tag;
	/* tlv */
	uint16_t u2Length;
	uint8_t ucWakeFlag;
	uint8_t EnableFlag;
	uint8_t aucReserved[2];
	/* 26 bytes */
	struct WLAN_MAC_HEADER_QoS_T aucMdnsMacHdr;
	uint8_t aucReserved2[2];
	/* 8 bytes */
	uint8_t aucMdnsUdpHdr[UDP_HEADER_LENGTH];
	/* 20 bytes */
	uint8_t aucMdnsIPHdr[IPV4_HEADER_LENGTH];
};

struct UNI_CMD_MDNS_RECORDE_GET_HIT {
	uint16_t u2Tag;
	/* tlv */
	uint16_t u2Length;
	uint8_t ucRecordId;
	uint8_t aucReserved[3];
};

struct UNI_CMD_MDNS_RECORDE_GET_MISS {
	uint16_t u2Tag;
	/* tlv */
	uint16_t u2Length;
};

struct UNI_CMD_MDNS_RECORDE_IPV6_WAKEUP {
	uint16_t u2Tag;
	/* tlv */
	uint16_t u2Length;

	uint8_t ucWakeFlag;
	uint8_t ucPassthroughBehavior;
	uint8_t ucIPV6WakeupFlag;
	uint8_t ucPayloadOrder;

	/* 2 bytes */
	uint16_t u2PayloadTotallength;
	uint8_t aucReserved[2];

	/* 1024 bytes */
	uint8_t ucPayload[MAX_MDNS_TRANSFER_SIZE];
};

#endif /* #if CFG_SUPPORT_MDNS_OFFLOAD */
#endif /* #if CFG_WOW_SUPPORT */

/* Reset Tx Scramble Seed (0x73) */
struct UNI_CMD_RESET_TX_SCRAMBLE {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**
	 * the TLVs included in this field:
	 * TAG                           | ID  | structure
	 * ------------------------------|-----|--------------------------------
	 * UNI_CMD_RESET_TX_SCRAMBLE_TAG | 0x0 | UNI_CMD_RESET_TX_SCRAMBLE_PARAM
	 */
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_RESET_TX_SCRAMBLE_TAG {
	UNI_CMD_RESET_TX_SCRAMBLE_TAG = 0x0,
	UNI_CMD_RESET_TX_SCRAMBLE_TAG_MAX_NUM,
};

/* Gaming Mode command (0x63) */
struct UNI_CMD_GAMING_MODE {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	* TAG                              |ID |structure
	* ---------------------------------|---|-------------
	* UNI_CMD_FORCE_RTS_GAMING_MODE_SET|0x0|UNI_CMD_FORCE_RTS_GAMING_MODE_T
	* UNI_CMD_GAMING_MODE_ATXOP_SET    |0x1|UNI_CMD_GAMING_MODE_T
	*/
} __KAL_ATTRIB_PACKED__;
/* Get Gaming Mode command TLV List */
enum ENUM_UNI_CMD_GAMING_MODE_TAG {
	UNI_CMD_GAMING_MODE_PROCESS = 0,
	UNI_CMD_GAMING_MODE_ATXOP_SET = 1,
	UNI_CMD_GAMING_MODE_MAX_NUM
};

/**
 * This structure is used for UNI_CMD_GAMING_MODE_PROCESS(0x00)
 * of UNI_CMD_GAMING_MODE command (0x63) to calculate MIC
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                   should be 0x00
 * @param[in] u2Length                the length of this TLV, should be 8
 * @param[in] ucForceRtsEn
 * @param[in] ucRtsPktNum
 * @param[in] aucReserved[2]
 */
/* DPP_LOW_LATENCY_MODE (Tag0) */
struct UNI_CMD_GAMING_MODE_PROCESS_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucForceRtsEn;
	uint8_t ucRtsPktNum;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_GAMING_MODE_ATXOP_SET tag(0x1) of
 * UNI_CMD_ID_GAMING_MODE command (0x63)
 * to set force rts gaming mode
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag              should be 0x0
 * @param[in] u2Length           the length of this TLV, should be
				sizeof(UNI_CMD_GAMING_MODE_ATXOP_SET_T)
 * @param[in] u4Cmd
 * @param[in] ucRtsPktNum
 * @param[in] aucReserved[2]
 */

struct UNI_CMD_GAMING_MODE_ATXOP_SET_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* tag specific part */
	uint32_t u4Cmd;
	uint32_t au4Param[MAX_ATXOP_PARAM_NUM];
} __KAL_ATTRIB_PACKED__;

#if CFG_FAST_PATH_SUPPORT
/* Fast Path command (0x54) */
struct UNI_CMD_FAST_PATH {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   UNI_CMD_FAST_PATH_PROCESS        | 0x0 | UNI_CMD_FAST_PATH_PROCESS_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Get FAST PATH command TLV List */
enum ENUM_UNI_CMD_FAST_PATH_TAG {
	UNI_CMD_FAST_PATH_PROCESS = 0,
	UNI_CMD_FAST_PATH_MAX_NUM
};

/**
 * This structure is used for UNI_CMD_FAST_PATH_PROCESS(0x00)
 * of UNI_CMD_FAST_PATH_PROCESS command (0x54) to calculate MIC
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                   should be 0x00
 * @param[in] u2Length                the length of this TLV, should be 8
 * @param[in] aucOwnMac               Mac address bring by Driver
 * @param[in] u2RandomNum             Random number genetate by Driver
 * @param[in] u4Keybitmap             Keybitmap send from Driver
 */
/* FAST PATH (Tag0) */
struct UNI_CMD_FAST_PATH_PROCESS_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  aucOwnMac[6];
	uint16_t u2RandomNum;
	uint32_t u4Keybitmap[4];
} __KAL_ATTRIB_PACKED__;
#endif

/* Low Power debug control command (0x71) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_LP_DBG_CTRL {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_LP_DBG_CTRL_TAG {
	UNI_CMD_LP_DBG_CTRL_TAG_GET_SLP_CNT_INFO = 0x0,
	UNI_CMD_LP_DBG_CTRL_TAG_KEEP_PWR_CTRL,
	UNI_CMD_LP_DBG_CTRL_TAG_MAX_NUM
};

/* Get sleep count info (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_LP_GET_SLP_CNT_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* Keep power control (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_LP_KEEP_PWR_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBandIdx;
	uint8_t ucKeepPwr;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_FACT_CAL == 1)
/* Factory cal command (0x7C) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL {
	/* fixed field */
	uint8_t aucRsvd[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;
/**< the TLVs included in this field:
 *
 *   TAG                           | ID   | structure
 *   -------------                 | ---- | -------------
 *   UNI_CMD_FACT_CAL_GET          | 0x00 | UNI_CMD_FACT_CAL_GET_T
 *   UNI_CMD_FACT_CAL_SET          | 0x01 | UNI_CMD_FACT_CAL_SET_T
 *   UNI_CMD_FACT_CAL_TRIGGER      | 0x02 | UNI_CMD_FACT_CAL_TRIGGER_T
 *   UNI_CMD_FACT_CAL_UPDATE_FLAG  | 0x03 | UNI_CMD_FACT_CAL_UPDATE_FLAG_T
 *   UNI_CMD_FACT_CAL_RAPID_GET    | 0x04 | UNI_CMD_FACT_CAL_RAPID_GET_T
 *   UNI_CMD_FACT_CAL_RAPID_SET    | 0x05 | UNI_CMD_FACT_CAL_RAPID_SET_T
 */

/* Fact cal command Tag */
enum ENUM_UNI_CMD_ID_FACT_CAL_TAG {
	UNI_CMD_FACT_CAL_TAG_GET            = 0,
	UNI_CMD_FACT_CAL_TAG_SET            = 1,
	UNI_CMD_FACT_CAL_TAG_TRIGGER        = 2,
	UNI_CMD_FACT_CAL_TAG_UPDATE_FLAG    = 3,
	UNI_CMD_FACT_CAL_TAG_RAPID_GET      = 4,
	UNI_CMD_FACT_CAL_TAG_RAPID_SET      = 5,
	UNI_CMD_ID_FACT_CAL_TAG_MAX_NUM
};

/** This structure is used for UNI_CMD_FACT_CAL_GET(0x00)
 * of UNI_CMD_ID_FACT_CAL command (0x7C) to get calbration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x00
 * @param[in] u2Length         the length of this TLV
 * @param[in] u4Data           data value
 * @param[in] u4SeqNum         sequence number
 * @param[in] u4BufDataLength  data length
 * @param[in] u1CalType        calbration type
 * @param[in] u1Done           done
 * @param[in] u1Rsvd           rsvd
 * @param[in] u1BufData        cal data
 */
/* Fact cal get command (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_GET {
	uint16_t u2Tag;
	uint16_t u2Length;

#define FACT_CAL_DATA_BUF_MAXSIZE 1400
	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint8_t u1CalType;
	uint8_t u1Done;
	uint8_t u1Band;
	uint8_t u1Channel;
	uint8_t u1BufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_FACT_CAL_SET(0x01)
 * of UNI_CMD_ID_FACT_CAL command (0x7C) to set calbration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x01
 * @param[in] u2Length         the length of this TLV
 * @param[in] u4Data           data value
 * @param[in] u4SeqNum         sequence number
 * @param[in] u4BufDataLength  data length
 * @param[in] u1CalType        calbration type
 * @param[in] u1Done           done
 * @param[in] u1Rsvd           rsvd
 * @param[in] u1BufData        cal data
 */
/* Fact cal set command (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_SET {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint8_t u1CalType;
	uint8_t u1Done;
	uint8_t u1Rsvd[2];
	uint8_t u1BufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_FACT_CAL_TRIGGER(0x02)
 * of UNI_CMD_ID_FACT_CAL command (0x7C) to trigger calbration.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x02
 * @param[in] u2Length         the length of this TLV
 *
 */
/* Fact cal trigger command (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_TRIGGER {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_FACT_CAL_UPDATE_FLAG(0x03)
 * of UNI_CMD_ID_FACT_CAL command (0x7C) to update calbration flag.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x03
 * @param[in] u2Length         the length of this TLV
 * @param[in] u4Data           the status of bypass flow control
 *
 */
/* Fact cal update flag command (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_UPDATE_FLAG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Data;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_RAPID_GET {
	uint16_t u2Tag;
	uint16_t u2Length;

#define FACT_CAL_DATA_BUF_MAXSIZE 1400
	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint32_t u4Channel;
	uint8_t u1CalType;
	uint8_t u1Done;
	uint8_t u1Band;
	uint8_t u1Rsvd;
	uint8_t u1BufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_CMD_FACT_CAL_RAPID_SET(0x05)
 * of UNI_CMD_ID_FACT_CAL command (0x7C) to set calbration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x01
 * @param[in] u2Length         the length of this TLV
 * @param[in] u4Data           data value
 * @param[in] u4SeqNum         sequence number
 * @param[in] u4BufDataLength  data length
 * @param[in] u1CalType        calbration type
 * @param[in] u1Done           done
 * @param[in] u1Rsvd           rsvd
 * @param[in] u1BufData        cal data
 */
/* Fact cal set command (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_RAPID_SET {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint8_t u1CalType;
	uint8_t u1Done;
	uint8_t u1Rsvd[2];
	uint8_t u1BufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_FACT_CAL */

/* SAP command (UNI_CMD_ID_SET_SAP 0x70) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SAP {
	/* fixed field */
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 * TAG				| ID  | structure
	 * -----------------------------|-----|--------------
	 * UNI_CMD_SAP_TAG_RSV		| 0x00|
	 * UNI_CMD_SET_SAP_RPS_TAG_SET	| 0x01| UNI_CMD_SET_SAP_RPS_SET_T
	 * UNI_CMD_SET_SAP_SUS_TAG_SET	| 0x02| UNI_CMD_SET_SAP_SUS_SET_T
	 * UNI_CMD_SAP_BCN_CRI_UPD	| 0x03| UNI_CMD_SAP_BCN_CRI_UPD_T
	 */
} __KAL_ATTRIB_PACKED__;

/* SAP command TLV List */
enum UNI_CMD_SET_SAP_RPS_TAG {
	UNI_CMD_SAP_TAG_RSV = 0x0,
	UNI_CMD_SET_SAP_RPS_TAG_SET = 0x1,
	UNI_CMD_SET_SAP_SUS_TAG_SET = 0x2,
	UNI_CMD_SAP_BCN_CRI_UPD = 0x3,
	UNI_CMD_SAP_TAG_MAX_NUM
};

/* UNI_CMD_SET_SAP_SUS_TAG_SET (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_SAP_RPS_SET_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgEnable;
	uint8_t ucPhase;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* UNI_CMD_SET_SAP_SUS_TAG_SET (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SET_SAP_SUS_SET_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgEnable;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

/* UNI_CMD_SAP_BCN_CRI_UPD (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SAP_BCN_CRI_UPD_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2UpdateBssBitmap;
	uint16_t u2BypassSeqBitmap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC {
	/* fixed field */
	uint8_t ucGroupMldId;
	uint8_t ucTokenID;
	uint8_t ucReserved[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_CMD_MLC_TAG {
	UNI_CMD_MLC_TAG_REQ_DEFAULT = 0,
	UNI_CMD_MLC_TAG_REQ_USER_CONFIG = 1,
	UNI_CMD_MLC_TAG_REQ_ACTIVE_NUM = 2,
	UNI_CMD_MLC_TAG_QUERY_LINK_STATE = 3,
	UNI_CMD_MLC_TAG_REQ_LOW_POWER = 4,
	UNI_CMD_MLC_TAG_REQ_LOW_LATENCY = 5,
	UNI_CMD_MLC_TAG_REQ_HIGH_TPUT = 6,
	UNI_CMD_MLC_TAG_QUERY_INFO = 7,
	UNI_CMD_MLC_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_DEFAULT {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_USER_CONFIG {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucLinkNum;
	uint8_t aucReserved[3];
	uint8_t aucLinkInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_LINK_INFO {
	uint16_t u2WlanIdx;
	uint8_t ucBssIdx;
	uint8_t ucLinkState;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_ACTIVE_NUM {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucActiveLinkNum;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_LOW_POWER {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_LOW_LATENCY {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_REQ_HIGH_TPUT {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_QUERY_LINK_STATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_MLC_QUERY_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucReserved[4];
} __KAL_ATTRIB_PACKED__;

/* COEX command (0x87) */
struct UNI_CMD_COEX_T {
	uint8_t ucReserved[4];

/* tlv */
	uint8_t aucTlvBuffer[];
/**< the TLVs included in this field:
 *   TAG                        | ID  | structure
 *   ---------------------------|-----|--------------
 *    UNI_CMD_COEX_STOP_CONNECT_PROTECT | 0x00 |
 *    UNI_CMD_STOP_CONNECT_PROTECT
 */
} __KAL_ATTRIB_PACKED__;

/* COEX config Tag */
enum UNI_CMD_COEX_TAG_T {
	UNI_CMD_COEX_STOP_CONNECT_PROTECT = 0,
	UNI_CMD_COEX_MAX_NUM
} __KAL_ATTRIB_PACKED__;

/** @addtogroup UNI_CMD_ID_COEX
 * @{
 */
/**
 * This structure is used for
 * UNI_CMD_COEX_STOP_CONNECT_PROTECT (0x00)
 * of UNI_CMD_ID_COEX command (0x87) to stop connect protect
 *
 * @param[in] ucBssInfoIdx            the BSS info index
 */
/* COEX Setting (Tag0) */
struct UNI_CMD_COEX_STOP_CONNECT_PROTECT_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssInfoIdx;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;
/** @} */


#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BT_CTRL {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[0];
	/**<the TLVs includer in this field:
	 *
	 *  TAG                    | ID   | structure
	 *  -------------          | -----| -------------
	 *  UNI_CMD_BT_CTRL_HEAD   | 0x0  | UNI_CMD_BT_CTRL_HEAD_T
	 *  UNI_CMD_BT_CTRL_DATA   | 0x1  | UNI_CMD_BT_CTRL_DATA_T
	 */
} __KAL_ATTRIB_PACKED__;

/* BT_CTRL command TLV List */
enum UNI_CMD_BT_CTRL_TAG {
	UNI_CMD_BT_CTRL_HEAD = 0,
	UNI_CMD_BT_CTRL_DATA = 1,
	UNI_CMD_BT_CTRL_MAX_NUM
};

/* get header (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BT_CTRL_HEAD {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/* get data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_BT_CTRL_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
/* shared antenna switch command (0x92) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SHR_ANT_SWCH {
	/* fixed field */
	uint8_t u1Reserved[4];
	/* tlv */
	uint8_t au1TlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* share antenna switch command Tag */
enum ENUM_UNI_CMD_SHR_ANT_SWCH_TAG {
	UNI_CMD_ID_SHR_ANT_SWCH_TRIG_EVT = 0,
	UNI_CMD_ID_SHR_ANT_SWCH_TAG_NUM
};

/* share antenna switch index (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_SHR_ANT_SWCH_TRIG_EVT {
	uint16_t u2Tag;  /* Tag = 0x00 */
	uint16_t u2Length;
	uint32_t u4Reserved;
} __KAL_ATTRIB_PACKED__;
#endif

/*******************************************************************************
 *                                 Event
 *******************************************************************************
 */

struct WIFI_UNI_EVENT {
	uint16_t u2PacketLength;
	uint16_t u2PacketType;	/* Must be filled with 0xE000 (EVENT Packet) */

	uint8_t ucEID;
	uint8_t ucSeqNum;
	uint8_t ucOption;
	uint8_t aucReserved[1];

	uint8_t ucExtenEID;
	uint8_t aucReserved2[2];
	uint8_t ucS2DIndex;

	uint8_t aucBuffer[];
};

/*
 * TLV element structure should start with a 2-byte Tag field and a 2-byte
 * length field and pad to 4-byte alignment. The u2Length field indicate
 * the length of the whole TLV including tag and length field.
 */
struct TAG_HDR {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucBuffer[];
};

enum ENUM_UNI_EVENT_ID {
	UNI_EVENT_ID_CMD_RESULT      = 0x01,  /* Generic event for return cmd status */
	UNI_EVENT_ID_BMC_RPY_DT      = 0x02,
	UNI_EVENT_ID_HIF_CTRL	     = 0x03,
	UNI_EVENT_ID_FW_LOG_2_HOST   = 0x04,
	UNI_EVENT_ID_ROAMING	     = 0x05,
	UNI_EVENT_ID_ACCESS_REG      = 0x06,
	UNI_EVENT_ID_CHIP_CONFIG     = 0x07,
	UNI_EVENT_ID_SMESH_INFO      = 0x08,
	UNI_EVENT_ID_IE_COUNTDOWN    = 0x09,
	UNI_EVENT_ID_ASSERT_DUMP     = 0x0A,
	UNI_EVENT_ID_SLEEP_NOTIFY    = 0x0b,
	UNI_EVENT_ID_BEACON_TIMEOUT  = 0x0C,
	UNI_EVENT_ID_PS_SYNC	     = 0x0D,
	UNI_EVENT_ID_SCAN_DONE	     = 0x0E,
	UNI_EVENT_ID_STAREC	     = 0x0F,
	UNI_EVENT_ID_ECC_CAL	     = 0x10,
	UNI_EVENT_ID_RDD	     = 0x11,
	UNI_EVENT_ID_ADD_KEY_DONE    = 0x12,
	UNI_EVENT_ID_OBSS_UPDATE     = 0x13,
	UNI_EVENT_ID_SER	     = 0x14,
	UNI_EVENT_ID_IDC	     = 0x17,
	UNI_EVENT_ID_MAC_INFO	     = 0x1A,
	UNI_EVENT_ID_TDLS	     = 0x1B,
	UNI_EVENT_ID_SAP	     = 0x1C,
	UNI_EVENT_ID_TXCMD_CTRL      = 0x1D,
	UNI_EVENT_ID_P2P	     = 0x1F,
	UNI_EVENT_ID_BAND_CONFIG     = 0x21,
	UNI_EVENT_ID_MIB	     = 0x22,
	UNI_EVENT_ID_STATISTICS      = 0x23,
	UNI_EVENT_ID_SR 	     = 0x25,
	UNI_EVENT_ID_SCS	     = 0x26,
	UNI_EVENT_ID_CNM	     = 0x27,
	UNI_EVENT_ID_MBMC	     = 0x28,
	UNI_EVENT_ID_BSS_IS_ABSENCE  = 0x29,
	UNI_EVENT_ID_TXPOWER	     = 0x2A,
	UNI_EVENT_ID_WSYS_CONFIG     = 0x2B,
	UNI_EVENT_ID_BA_OFFLOAD      = 0x2C,
	UNI_EVENT_ID_STATUS_TO_HOST  = 0x2D,
	UNI_EVENT_ID_RA 	     = 0x2F,
	UNI_EVENT_ID_SPECTRUM	     = 0x30,
	UNI_EVENT_ID_TESTMODE_RX_STAT_INFO  = 0x32,
	UNI_EVENT_ID_BF 	     = 0X33,
	UNI_EVENT_ID_SDVT_STAT	     = 0x34,
	UNI_EVENT_ID_THERMAL	     = 0x35,
	UNI_EVENT_ID_NOISE_FLOOR     = 0x36,
	UNI_EVENT_ID_VOW	     = 0x37,
	UNI_EVENT_ID_TPC	     = 0x38,
	UNI_EVENT_ID_MEC	     = 0x3A,
	UNI_EVENT_ID_POWER_METRICS   = 0x3F,
	UNI_EVENT_ID_RSSI_MONITOR    = 0x41,
	UNI_EVENT_ID_TEST_TR_PARAM   = 0x42,
	UNI_EVENT_ID_CHIP_CAPABILITY = 0x43,
	UNI_EVENT_ID_UPDATE_COEX_PHYRATE = 0x44,
	UNI_EVENT_ID_TESTMODE_CTRL   = 0x46,
	UNI_EVENT_ID_TWT_SYNC	     = 0x47,
	UNI_EVENT_ID_EFUSE_CONTROL   = 0x48,
	UNI_EVENT_ID_CSI	     = 0x4A,
	UNI_EVENT_ID_VLAN_CFG	     = 0x4B,
	UNI_EVENT_ID_CAL	     = 0x4C,
	UNI_EVENT_ID_RXFE_CTRL	     = 0x4D,
	UNI_EVENT_ID_HWCFG_CTRL      = 0x4E,
	UNI_EVENT_ID_EAP_CTRL	     = 0x51,
	UNI_EVENT_ID_PHY_STATE_INFO  = 0x52,
	UNI_EVENT_ID_BSS_ER	     = 0x53,
	UNI_EVENT_ID_FAST_PATH	     = 0x54,
	UNI_EVENT_ID_MDDP	     = 0x55,
	UNI_EVENT_ID_NAN	     = 0x56,
	UNI_EVENT_ID_MLO	     = 0x59,
	UNI_EVENT_ID_PP		     = 0x5A,
	UNI_EVENT_ID_WOW	     = 0x5B,
	UNI_EVENT_ID_GET_VOLT_INFO   = 0x5C,
	UNI_EVENT_ID_RTT	     = 0x5D,
	UNI_EVENT_ID_PKT_OFLD	     = 0x60,
	UNI_EVENT_ID_DELAY_BAR       = 0x61,
	UNI_EVENT_ID_FW_DROP_SSN     = 0x62,
	UNI_EVENT_ID_MDNS_REOCRD     = 0x64,
	UNI_EVENT_ID_LP_DBG_CTRL     = 0x71,
	UNI_EVENT_ID_HW_DETECT_REPORT = 0x76,
	UNI_EVENT_ID_UPDATE_LP       = 0x77,
	UNI_EVENT_ID_BT_CTRL         = 0x7B,
#if (CFG_SUPPORT_FACT_CAL == 1)
	UNI_EVENT_ID_FACT_CAL        = 0x7C,
#endif
	UNI_EVENT_ID_PHY_LIST_DUMP   = 0x7f,
	UNI_EVENT_ID_OMI	     = 0x84,
	UNI_EVENT_ID_MLC	     = 0x86,
	UNI_EVENT_ID_MBRAIN      = 0x89,
	UNI_EVENT_ID_SHR_ANT_SWCH    = 0x92,
	UNI_EVENT_ID_COEX_ICER  = 0x93,  /* ICER cmd */
	UNI_EVENT_ID_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CMD_RESULT {
	uint16_t u2CID;
	uint8_t aucReserved[2];
	uint32_t u4Status;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_HIF_CTRL {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* HIF_CTRL event Tag */
enum ENUM_UNI_EVENT_HIF_CTRL_TAG {
	UNI_EVENT_HIF_CTRL_TAG_BASIC = 0,
	UNI_EVENT_HIF_CTRL_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_HIF_CTRL_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucCID;
	uint8_t aucPadding[3];
	uint32_t u4Status;
	uint8_t ucHifType;
	uint8_t ucHifTxTrafficStatus;
	uint8_t ucHifRxTrafficStatus;
	uint8_t ucHifSuspend;
#if (CFG_WIFI_PCIE_L2_MOBILE_ONLY == 1)
	uint16_t u2HifPcieUserCount;
	uint8_t aucReserved[2];
#else
	uint8_t aucReserved[4];
#endif
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_FW_LOG2HOST {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   ---------------------------------|-----|--------------
        *   UNI_EVENT_FW_LOG_FORMAT          | 0x0 | UNI_EVENT_FW_LOG_FORMAT (Should always be the last TLV element)
        */
} __KAL_ATTRIB_PACKED__;

/* FW Log 2 Host event Tag */
enum ENUM_UNI_EVENT_FWLOG2HOST_TAG {
	UNI_EVENT_FWLOG2HOST_TAG_FORMAT = 0,
	UNI_EVENT_FWLOG2HOST_TAG_NUM
};

/* FW Log with Format (Tag0) */
struct UNI_EVENT_FW_LOG_FORMAT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucMsgFmt;
	uint8_t ucReserved[3];
	uint8_t acMsg[];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PP {
	/* fixed field */
	uint8_t au1Reserved[4];

	/* tlv */
	uint8_t au1TlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_PP_TAG {
	UNI_EVENT_PP_TAG_ALG_CTRL = 0x1,
	UNI_EVENT_SAP_TAG_SAP_DSCB_IE = 0x2,
	UNI_EVENT_PP_TAG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PP_ALG_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4PpTimerIntv;
	uint32_t u4ThrX2_Value;
	uint32_t u4ThrX2_Shift;
	uint32_t u4ThrX3_Value;
	uint32_t u4ThrX3_Shift;
	uint32_t u4ThrX4_Value;
	uint32_t u4ThrX4_Shift;
	uint32_t u4ThrX5_Value;
	uint32_t u4ThrX5_Shift;
	uint32_t u4ThrX6_Value;
	uint32_t u4ThrX6_Shift;
	uint32_t u4ThrX7_Value;
	uint32_t u4ThrX7_Shift;
	uint32_t u4ThrX8_Value;
	uint32_t u4ThrX8_Shift;
	uint32_t u4SwPpTime;
	uint32_t u4HwPpTime;
	uint32_t u4NoPpTime;
	uint32_t u4AutoBwTime;
	uint8_t  u1DbdcIdx;
	uint8_t  u1Reserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ROAMING {
	/* fixed field */
	uint8_t ucBssIndex;
	uint8_t ucDbdcIdx;
	uint8_t aucPadding[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

struct UNI_EVENT_MDNS_RECORD {
	/* DWORD_0 */
	uint8_t ucVersion;
	uint8_t ucType; /* 0: invalid, 1: Hit 2: Miss */
	uint16_t u2ControlFlag;
	/* DWORD_1 */
	uint32_t u4MdnsHitMiss;
	/* DWORD_2 */
	uint8_t aucReserved2[64];
};

enum ENUM_UNI_EVENT_ROAMING_TAG {
	UNI_EVENT_ROAMING_TAG_STATUS  = 0,
	UNI_EVENT_ROAMING_TAG_NUM
};

/* Roaming status (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ROAMING_STATUS {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint16_t u2Event;
	uint16_t u2Data;
	uint32_t eReason; /*ENUM_ROAMING_REASON_T*/
	uint32_t u4RoamingTriggerTime;
	uint16_t u2RcpiLowThreshold;
	uint16_t u2RcpiHighThreshold;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ACCESS_REG {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                              | ID  | structure
        *   ---------------------------------|-----|---------------------------------
        *   UNI_EVENT_ACCESS_REG_BASIC       | 0x0 | UNI_EVENT_ACCESS_REG_BASIC_T
        *   UNI_EVENT_ACCESS_RF_REG_BASIC    | 0x1 | UNI_EVENT_ACCESS_RF_REG_BASIC_T
        */
} __KAL_ATTRIB_PACKED__;

/* Register access event Tag */
__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_EVENT_ACCESS_REG_TAG {
	UNI_EVENT_ACCESS_REG_TAG_BASIC = 0,
	UNI_EVENT_ACCESS_REG_TAG_RF_REG_BASIC,
	UNI_EVENT_ACCESS_REG_TAG_NUM
} __KAL_ATTRIB_PACKED__;

/* Access Register (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ACCESS_REG_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Addr;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

/* Access RF address (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ACCESS_RF_REG_BASIC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WifiStream;
	uint16_t u2Reserved;
	uint32_t u4Addr;
	uint32_t u4Value;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CHIP_CONFIG {
	/* fixed field */
	uint16_t u2TotalElementNum;
	uint8_t aucPadding[2];
	/* tlv */

	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_CHIP_CONFIG_TAG {
	UNI_EVENT_CHIP_CONFIG_TAG_SW_DBG_CTRL,
	UNI_EVENT_CHIP_CONFIG_TAG_CUSTOMER_CFG,
	UNI_EVENT_CHIP_CONFIG_TAG_CHIP_CFG ,
	UNI_EVENT_CHIP_CONFIG_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_IE_COUNTDOWNT {
	/* fixed field */
	uint8_t ucBand;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* IE countdown event Tag */
enum ENUM_UNI_EVENT_IE_COUNTDOWN_TAG
{
	UNI_EVENT_IE_COUNTDOWN_CSA = 0,
	UNI_EVENT_IE_COUNTDOWN_BCC = 1,
	UNI_EVENT_IE_COUNTDOWN_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CSA_NOTIFY
{
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint8_t ucOwnMacIdx;
	uint8_t ucChannelSwitchCount;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

/* BCC notify Parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BCC_NOTIFY
{
	uint16_t u2Tag;    // Tag = 0x01
	uint16_t u2Length;
	uint8_t ucOwnMacIdx;
	uint8_t ucColorSwitchCount;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SLEEP_NOTIFY {
	/*fixed field*/
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                             | ID  | structure
	*   -------------                   | ----| -------------
	*   UNI_EVENT_SLEEP_INFO          | 0x0 | UNI_EVENT_SLEEP_INFO_T
	*/
} __KAL_ATTRIB_PACKED__;

/* Sleep Notify event Tag */
enum ENUM_UNI_EVENT_SLEEP_NOTYFY_TAG {
	UNI_EVENT_SLEEP_NOTYFY_TAG_SLEEP_INFO = 0,
	UNI_EVENT_SLEEP_NOTYFY_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SLEEP_INFO {
	uint16_t u2Tag;                   // Tag = 0x00
	uint16_t u2Length;

	uint8_t ucSleepyState;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SPECTRUM {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	*   TAG                           | ID  | structure
	*   ------------------------------|-----|-----------------------------
	*   UNI_EVENT_SPECTRUM_TAG_STATUS | 0x0 | UNI_EVENT_SPECTRUM_STATUS
	*   UNI_EVENT_SPECTRUM_TAG_DATA   | 0x1 | UNI_EVENT_SPECTRUM_DATA
	*   UNI_EVENT_SPECTRUM_TAG_PHY_ICS_DATA | 0x2 | UNI_EVENT_PHY_ICS_DATA
	*/
} __KAL_ATTRIB_PACKED__;

/* Spectrum Tag */
enum UNI_EVENT_SPECTRUM_TAG {
	UNI_EVENT_SPECTRUM_TAG_STATUS = 0,
	UNI_EVENT_SPECTRUM_TAG_DATA,
	UNI_EVENT_SPECTRUM_TAG_PHY_ICS_DATA,
	UNI_EVENT_SPECTRUM_TAG_NUM
};

/* SPECTRUM_STATUS (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SPECTRUM_STATUS {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4FuncIndex;
	uint32_t u4CapDone;
	uint32_t u4Reserved[15];
} __KAL_ATTRIB_PACKED__;

/* SPECTRUM_DATA (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SPECTRUM_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4FuncIndex;
	uint32_t u4PktNum;
	uint32_t u4Bank;
	uint32_t u4DataLen;
	uint32_t u4WFCnt;
	uint32_t u4SmplCnt;
	uint32_t u4Reserved[6];
	int32_t i4Data[256];
} __KAL_ATTRIB_PACKED__;

#if (CFG_SUPPORT_PHY_ICS == 1)
/* PHY_ICS_DATA (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PHY_ICS_DUMP_RAW_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4FuncIndex;
	uint32_t u4PktNum;
	uint32_t u4PhyTimestamp;
	uint32_t u4DataLen;
	uint32_t u4Reserved[5];
	uint32_t u4Data[MAX_PHY_ICS_DUMP_DATA_CNT];
} __KAL_ATTRIB_PACKED__;
#endif /* #if (CFG_SUPPORT_ICS == 1) */

enum ENUM_UNI_BCN_TIMEOUT_REASON {
	UNI_ENUM_BCN_LOSS_STA = 0x00,
	UNI_ENUM_BCN_LOSS_ADHOC = 0x01,
	UNI_ENUM_BCN_NULL_FRAME_THRESHOLD = 0x02,
	UNI_ENUM_BCN_PERIOD_NOT_ILLIGAL = 0x03,
	UNI_ENUM_BCN_CONNECTION_FAIL = 0x04,
	UNI_ENUM_BCN_ALLOCAT_NULL_PKT_FAIL_THRESHOLD = 0x05,
	UNI_ENUM_BCN_UNSPECIF_REASON = 0x06,
	UNI_ENUM_BCN_NULL_FRAME_LIFE_TIMEOUT = 0x07,
	UNI_ENUM_BCN_LOSS_AP_DISABLE = 0x08,
	UNI_ENUM_BCN_LOSS_AP_ERROR = 0x09,
	UNI_ENUM_BCN_MLINK_NULL_FRAME_THRESHOLD = 0x0a,
	UNI_ENUM_BCN_LINK_RECOVERY = 0x0b,
	UNI_ENUM_BCN_PROT_ERROR = 0x0c,
	UNI_ENUM_BCN_TIMEOUT_REASON_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BEACON_TIMEOUT {
	/* fixed field */
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Beacon Timeout event Tag */
enum ENUM_UNI_EVENT_BEACON_TIMEOUT_TAG {
	UNI_EVENT_BEACON_TIMEOUT_TAG_INFO = 0,
	UNI_EVENT_BEACON_TIMEOUT_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BEACON_TIMEOUT_INFO {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint8_t ucReasonCode;
	uint8_t ucMLBTOLinkNum;
	uint8_t ucLinkDtThreshold;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PS_SYNC {
	/*fixed field*/
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*   TAG                             | ID  | structure
	*   -------------                   | ----| -------------
	*   UNI_EVENT_CLIENT_PS_INFO          | 0x0 | UNI_EVENT_CLIENT_PS_INFO_T
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_PS_SYNC_TAG {
	UNI_EVENT_PS_SYNC_TAG_CLIENT_PS_INFO = 0,
	UNI_EVENT_PS_SYNC_TAG_NUM
};

/* PS SYNC (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CLIENT_PS_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucPsBit;
	uint8_t aucPadding[1];
	uint16_t ucWtblIndex;
	uint8_t ucBufferSize;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SCAN_DONE {
	/* fixed field */
	uint8_t ucSeqNum;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_SCAN_DONE_TAG {
	UNI_EVENT_SCAN_DONE_TAG_BASIC      = 0,
	UNI_EVENT_SCAN_DONE_TAG_SPARSECHNL = 1,
	UNI_EVENT_SCAN_DONE_TAG_CHNLINFO   = 2,
	UNI_EVENT_SCAN_DONE_TAG_NLO        = 3,
	UNI_EVENT_SCAN_DONE_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SCAN_DONE_BASIC {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;

	uint8_t  ucCompleteChanCount;
	uint8_t  ucCurrentState;
	uint8_t  ucScanDoneVersion;
	uint8_t  fgIsPNOenabled;
	uint32_t u4ScanDurBcnCnt;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SCAN_DONE_SPARSECHNL {
	uint16_t u2Tag;    // Tag = 0x01
	uint16_t u2Length;

	uint8_t ucSparseChannelValid;
	uint8_t ucBand;
	uint8_t ucChannelNum;
	uint8_t ucSparseChannelArrayValidNum;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SCAN_DONE_CHNLINFO {
	uint16_t u2Tag;    // Tag = 0x02
	uint16_t u2Length;

	uint8_t ucNumOfChnl;
	uint8_t aucReserved[3];
	uint8_t aucChnlInfoBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CHNLINFO {
	uint8_t ucChannelNum;
	uint8_t ucChannelBAndPCnt;
	uint8_t ucChannelMDRDYCnt;
	uint8_t ucChannelUtilVal;
	uint16_t u2ChannelIdleTime;
	uint16_t u2ChannelScanTime;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SCAN_DONE_NLO {
	uint16_t u2Tag;    // Tag = 0x03
	uint16_t u2Length;

	uint8_t  ucStatus;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STAREC {
	/* fixed field */
	uint16_t u2WlanIdx;
	uint8_t aucPadding[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* starec event Tag */
enum UNI_EVENT_STAREC_TAG {
	UNI_EVENT_STAREC_TAG_UPDATE_MAX_AMSDU_LEN,
	UNI_EVENT_STAREC_TAG_PN_INFO        = 0x26,
	UNI_EVENT_STAREC_TAG_MLO_LINK_STATE = 0x27,
	UNI_EVENT_STAREC_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STAREC_PN_INFO {
	uint16_t u2Tag;               /* Tag = 0x26 */
	uint16_t u2Length;
	uint8_t  aucPn[6];
	uint8_t  ucTscType;
	uint8_t  aucReserved;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STAREC_UPDATE_MAX_AMSDU_LEN {
	uint16_t u2Tag;               /* Tag = 0x00 */
	uint16_t u2Length;
	uint16_t u2AmsduLen;
	uint8_t  aucPadding[2];
} __KAL_ATTRIB_PACKED__;

enum ENUM_MLO_LINK_STATE {
	MLO_LINK_STATE_ACTIVE = 0,
	MLO_LINK_STATE_INACTIVE,
	MLO_LINK_STATE_MAX_NUM
};

enum ENUM_MLO_LINK_STATE_CHANGE_REASON {
	MLO_LINK_STATE_CHANGE_REASON_DEFAULT = 0,
	MLO_LINK_STATE_CHANGE_REASON_TEST,
	MLO_LINK_STATE_CHANGE_REASON_T2LM,
	MLO_LINK_STATE_CHANGE_REASON_EMLSR,
	MLO_LINK_STATE_CHANGE_REASON_LINK_RECOMMEND,
	MLO_LINK_STATE_CHANGE_REASON_RCPI,
	MLO_LINK_STATE_CHANGE_REASON_COEX,
	MLO_LINK_STATE_CHANGE_REASON_CONCURRENT,
	MLO_LINK_STATE_CHANGE_REASON_TPUT_HIGH,
	MLO_LINK_STATE_CHANGE_REASON_TPUT_LOW,
	MLO_LINK_STATE_CHANGE_REASON_AP_REMOVAL,
	MLO_LINK_STATE_CHANGE_REASON_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STAREC_MLO_LINK_STATE {
	uint16_t u2Tag;               /* Tag = 0x01 */
	uint16_t u2Length;
	uint8_t ucLinkState;
	uint8_t ucReason;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RDD
{
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
        *
        *   TAG                          | ID  | structure
        *   -------------                | ----| -------------
        *   UNI_EVENT_RDD_SEND_PULSE     | 0x0 | UNI_EVENT_RDD_SEND_PULSE_T
        *   UNI_EVENT_RDD_REPORT         | 0x1 | UNI_EVENT_RDD_REPORT_T
        */
} __KAL_ATTRIB_PACKED__;

/* RDD
 event Tag */
enum ENUM_UNI_EVENT_RDD_TAG {
	UNI_EVENT_RDD_TAG_SEND_PULSE = 0,
	UNI_EVENT_RDD_TAG_REPORT = 1,
	UNI_EVENT_RDD_TAG_NUM
};

#define INC_RING_INDEX2(_idx, _RingSize)		\
{ \
	(_idx) = (_idx+1) % (_RingSize); \
	KAL_MB_W(); \
}

/* Beacon timeout reason (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RDD_SEND_PULSE {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint8_t u1RddIdx;
	uint8_t u1LongDetected;
	uint8_t u1ConstantPRFDetected;
	uint8_t u1StaggeredPRFDetected;
	uint8_t u1RadarTypeIdx;
	uint8_t u1PeriodicPulseNum;
	uint8_t u1LongPulseNum;
	uint8_t u1HwPulseNum;
	uint8_t u1OutLPN;    /* Long Pulse Number */
	uint8_t u1OutSPN;    /* Short Pulse Number */
	uint8_t u1OutCRPN;
	uint8_t u1OutCRPW;   /* Constant PRF Radar: Pulse Number */
	uint8_t u1OutCRBN;   /* Constant PRF Radar: Burst Number */
	uint8_t u1OutSTGPN;  /* Staggered PRF radar: Staggered pulse number */
	uint8_t u1OutSTGPW;  /* Staggered PRF radar: maximum pulse width */
	uint8_t u1Reserve;
	uint32_t u4OutPRI_CONST;
	uint32_t u4OutPRI_STG1;
	uint32_t u4OutPRI_STG2;
	uint32_t u4OutPRI_STG3;
	/* Staggered PRF radar: min PRI Difference between 1st and 2nd	*/
	uint32_t u4OutPRIStgDmin;
	/* event body  */
	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Per band SER counter (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RDD_REPORT {
	uint16_t   u2Tag;    // Tag = 0x01
	uint16_t   u2Length;
	uint32_t u4FuncIndex;
	uint32_t u4FuncLength;
	uint32_t u4Prefix;
	uint32_t u4Count;
	uint8_t ucRddIdx;
	uint8_t aucReserve[3];
	uint8_t aucBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ADD_KEY_DONE {
	/* fixed field */
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Add key done event Tag */
enum ENUM_UNI_EVENT_ADD_KEY_DONE_TAG {
	UNI_EVENT_ADD_KEY_DONE_TAG_PKEY = 0,
	UNI_EVENT_ADD_KEY_DONE_TAG_NUM
};

/* Add PKey down (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_ADD_PKEY_DONE {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint8_t aucStaAddr[6];
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_OBSS_UPDATE
{
	/* fixed field */
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* OBSS update event Tag */
enum ENUM_UNI_EVENT_OBSS_UPDATE_TAG
{
	UNI_EVENT_OBSS_UPDATE_TAG_STATUS = 0,
	UNI_EVENT_OBSS_UPDATE_TAG_NUM
};

/* Update OBSS STATUS (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_OBSS_STATUS {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint8_t  ucObssErpProtectMode;
	uint8_t  ucObssHtProtectMode;
	uint8_t  ucObssGfOperationMode;
	uint8_t  ucObssRifsOperationMode;
	uint8_t  ucObssBeaconForcedTo20M;
	uint8_t  aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TDLS {
	/* fixed field */
	uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* TDLS event Tag */
enum UNI_EVENT_TDLS_TAG {
	UNI_EVENT_TDLS_TAG_TEAR_DOWN = 0,
	UNI_EVENT_TDLS_TAG_NUM
};

/* TDLS tear down reason (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TDLS_TEAR_DOWN {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint32_t u4Subid;
	/* In Uni_Cmd: FW StaIdx == WlanIdx,convert */
	uint32_t u4WlanIdx;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_IDC {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*   TAG                             | ID  | structure
	*   -------------                   | ----| -------------
	*   UNI_EVENT_MD_SAFE_CHN   | 0x0 | UNI_EVENT_MD_SAFE_CHN_T
	*   UNI_EVENT_CCCI_MSG         | 0x1 | UNI_EVENT_CCCI_MSG_T
	*
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_IDC_TAG {
	UNI_EVENT_IDC_TAG_MD_SAFE_CHN = 0,
	UNI_EVENT_IDC_TAG_CCCI_MSG = 1,
	UNI_EVENT_IDC_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MD_SAFE_CHN {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucVersion;
	uint8_t aucReserved1[3];    /* 4 byte alignment */
	uint32_t u4Flags;            /* Bit0: valid */
	uint32_t u4SafeChannelBitmask[UNI_WIFI_CH_MASK_IDX_NUM]; /* WIFI_CH_MASK_IDX_NUM = 4 */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MAC_IFNO {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/* the TLVs included in this field:
	 * TAG                               | ID  | structure
	 * ----------------------------------|-----|--------------
	 * UNI_EVENT_MAC_INFO_TAG_TSF        | 0x0 | UNI_EVENT_MAC_INFO_TSF
	 * UNI_EVENT_MAC_INFO_TAG_TWT_STA_CNM| 0x1 |
	 *                                        UNI_EVENT_MAC_INFO_TWT_STA_CNM
	 * UNI_EVENT_MAC_INFO_TAG_TSF_SYNC   | 0x3 | UNI_EVENT_MAC_INFO_TSF_SYNC
	 */
} __KAL_ATTRIB_PACKED__;

/* Mac info event Tag */
enum ENUM_UNI_EVENT_MAC_INFO_TAG {
	UNI_EVENT_MAC_INFO_TAG_TSF  = 0,
	UNI_EVENT_MAC_INFO_TAG_TWT_STA_CNM = 1,
	UNI_EVENT_MAC_INFO_TAG_TSF_SYNC  = 3,
	UNI_EVENT_MAC_INFO_TAG_NUM
};

/* Beacon timeout reason (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MAC_INFO_TSF {
	uint16_t   u2Tag;    // Tag = 0x00
	uint16_t   u2Length;
	uint8_t    ucDbdcIdx;
	uint8_t    ucHwBssidIndex;
	uint8_t    aucPadding[2];
	uint32_t   u4TsfBit0_31;
	uint32_t   u4TsfBit63_32;
} __KAL_ATTRIB_PACKED__;

/* Uni event for TWT STA req CNM CH usage (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MAC_INFO_TWT_STA_CNM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucDbdcIdx;
	uint8_t ucHwBssidIndex;
	uint8_t ucBssIndex;
	uint8_t fgCnmGranted;
} __KAL_ATTRIB_PACKED__;

/* Uni event for TSF_SYNC (Tag3) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MAC_INFO_TSF_SYNC {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucBssIndex;
	uint8_t  aucPadding[3];
	uint64_t u8TsfValue;
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_WIFI_POWER_METRICS
struct UNI_EVENT_ID_POWER_METRICS {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
};

/* Power Metrics event tag */
enum UNI_EVENT_ID_POWER_METRICS_TAG {
	UNI_EVENT_POWER_METRICS_INFO_TAG = 0,
	UNI_EVENT_ICCM_TAG = 1,
	UNI_EVENT_POWER_METRICS_TAG_NUM
};

struct UNI_POWER_STATE_INFO {
	uint32_t u4SleepTime;
	uint32_t u4RxListenTime;
	uint32_t u4TxTime;
	uint32_t u4RxTime;
};

struct UNI_EVENT_ID_POWER_METRICS_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	uint32_t u4TotalTime;
	uint32_t u4Band;
	uint32_t u4Protocol;
	uint32_t u4Nss[2];
	struct UNI_POWER_STATE_INFO u4BandRatio;
	uint32_t arStatsPmCckRateStat[4];
	uint32_t arStatsPmOfdmRateStat[8];
	uint32_t arStatsPmHtRateStat[32];
	uint32_t arStatsPmVhtRateStat[30];
	uint32_t arStatsPmHeRateStat[48];
	uint32_t arStatsPmEhtRateStat[80];
};

struct UNI_EVENT_ID_PWR_MET_ICCM_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	uint32_t u4TotalTime;
	struct UNI_POWER_STATE_INFO u4BandRatio[5];
};
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MIB_INFO {
	/* fixed field */
	uint8_t ucBand;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* MIB command Tag */
enum ENUM_UNI_EVENT_MIB_TAG {
	UNI_EVENT_MIB_DATA_TAG = 0,
	UNI_EVENT_MIB_MAX_NUM,
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MIB_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Counter;
	uint64_t u8Data;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STATISTICS {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Statistics event tag */
enum ENUM_UNI_EVENT_STATISTICS_TAG {
	UNI_EVENT_STATISTICS_TAG_BASIC = 0,
	UNI_EVENT_STATISTICS_TAG_LINK_QUALITY = 1,
	UNI_EVENT_STATISTICS_TAG_STA = 2,
	UNI_EVENT_STATISTICS_TAG_BUG_REPORT = 3,
	UNI_EVENT_STATISTICS_TAG_TX_STATS = 4,
	UNI_EVENT_STATISTICS_TAG_ALL_STATS = 5,
	UNI_EVENT_STATISTICS_TAG_EML_STATS = 6,
	UNI_EVENT_STATISTICS_TAG_BSS_LINK_QUALITY = 8,
	UNI_EVENT_STATISTICS_TAG_GET_ML_CHNL_COND = 9,
	UNI_EVENT_STATISTICS_TAG_GET_ML_CHNL_COND_RESULT = 10,
	/* Reserved range for compatible with ENUM_STATS_LLS_TLV_TAG_ID */
	UNI_EVENT_STATISTICS_TAG_LINK_LAYER_STATS = 0x80,
	UNI_EVENT_STATISTICS_TAG_PPDU_LATENCY,
	UNI_EVENT_STATISTICS_TAG_CURRENT_TX_RATE,
	UNI_EVENT_STATISTICS_TAG_SET_WFD_TX_BITRATE_MONTR,
	UNI_EVENT_STATISTICS_TAG_GET_WFD_PRED_TX_BITRATE,
	UNI_EVENT_STATISTICS_TAG_UEVENT = 0x86,
	UNI_EVENT_STATISTICS_TAG_BSS_CURRENT_TX_RATE = 0x87,
	UNI_EVENT_STATISTICS_TAG_GET_BSS_PRED_TX_BITRATE = 0x88,
};

/* Basic Scan down notify Parameters (Tag00) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BASIC_STATISTICS {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	uint32_t     u4HwMacAwakeDuration; /* SlotIdleHwAwake */
	uint64_t     u8TransmittedFragmentCount;
	uint64_t     u8MulticastTransmittedFrameCount;
	uint64_t     u8FailedCount;
	uint64_t     u8RetryCount;
	uint64_t     u8MultipleRetryCount;
	uint64_t     u8RTSSuccessCount;
	uint64_t     u8RTSFailureCount;
	uint64_t     u8ACKFailureCount;
	uint64_t     u8FrameDuplicateCount;
	uint64_t     u8ReceivedFragmentCount;
	uint64_t     u8MulticastReceivedFrameCount;
	uint64_t     u8FCSErrorCount;
	uint64_t     u8MdrdyCnt;
	uint64_t     u8ChnlIdleCnt;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BUG_REPORT {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	/* BugReportVersion */
	uint32_t     u4BugReportVersion;

	/* FW Module State */
	uint32_t     u4FWState; //LP, roaming
	uint32_t     u4FWScanState;
	uint32_t     u4FWCnmState;

	/* Scan Counter */
	uint32_t     u4ReceivedBeaconCount;
	uint32_t     u4ReceivedProbeResponseCount;
	uint32_t     u4SentProbeRequestCount;
	uint32_t     u4SentProbeRequestFailCount;

	/* Roaming Counter */
	uint32_t     u4RoamingDebugFlag;
	uint32_t     u4RoamingThreshold;
	uint32_t     u4RoamingCurrentRcpi;

	/* RF Counter */
	uint32_t     u4RFPriChannel;
	uint32_t     u4RFChannelS1;
	uint32_t     u4RFChannelS2;
	uint32_t     u4RFChannelWidth;
	uint32_t     u4RFSco;

	/* Coex Counter */
	uint32_t     u4BTProfile;
	uint32_t     u4BTOn;
	uint32_t     u4LTEOn;

	/* Low Power Counter */
	uint32_t     u4LPTxUcPktNum;
	uint32_t     u4LPRxUcPktNum;
	uint32_t     u4LPPSProfile;

	/* Base Band Counter */
	uint32_t     u4OfdmPdCnt;
	uint32_t     u4CckPdCnt;
	uint32_t     u4CckSigErrorCnt;
	uint32_t     u4CckSfdErrorCnt;
	uint32_t     u4OfdmSigErrorCnt;
	uint32_t     u4OfdmTaqErrorCnt;
	uint32_t     u4OfdmFcsErrorCnt;
	uint32_t     u4CckFcsErrorCnt;
	uint32_t     u4OfdmMdrdyCnt;
	uint32_t     u4CckMdrdyCnt;
	uint32_t     u4PhyCcaStatus;
	uint32_t     u4WifiFastSpiStatus;

	/* Mac RX Counter */
	uint32_t     u4RxMdrdyCount;
	uint32_t     u4RxFcsErrorCount;
	uint32_t     u4RxbFifoFullCount;
	uint32_t     u4RxMpduCount;
	uint32_t     u4RxLengthMismatchCount;
	uint32_t     u4RxCcaPrimCount;
	uint32_t     u4RxEdCount;

	uint32_t     u4LmacFreeRunTimer;
	uint32_t     u4WtblReadPointer;
	uint32_t     u4RmacWritePointer;
	uint32_t     u4SecWritePointer;
	uint32_t     u4SecReadPointer;
	uint32_t     u4DmaReadPointer;

	/* Mac TX Counter */
	uint32_t     u4TxChannelIdleCount;
	uint32_t     u4TxCcaNavTxTime;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_LINK_QUALITY {
	int8_t       cRssi; /* AIS Network. */
	int8_t       cLinkQuality;
	uint16_t     u2LinkSpeed;            /* TX rate1 */
	uint8_t      ucMediumBusyPercentage; /* Read clear */
	uint8_t      ucIsLQ0Rdy;                 /* Link Quality BSS0 Ready. */
	uint8_t      aucReserve[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LINK_QUALITY {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_LINK_QUALITY rLq[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STA_STATISTICS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LINK_STATS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_LLS
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LINK_LAYER_STATS {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct EVENT_STATS_LLS_DATA data;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PPDU_LATENCY {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct EVENT_STATS_LLS_TX_LATENCY latency;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CURRENT_TX_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct EVENT_STATS_LLS_TX_RATE_INFO rate_info;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PRED_TX_BITRATE {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct EVENT_STATS_LLS_TX_BIT_RATE bitrate;
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_EML_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4TimeoutMs;
	uint16_t u2StaRecMldIdx;
	uint8_t ucEmlsrBitMap;
	/* same with ENUM_EML_STATE_T */
	uint8_t ucCurrentState;
	/* AP MLD responded the EML notification frame */
	uint8_t ucEmlNegotiated;
	/* Reserved for backward compatible for ucMaxMldLinkNum*/
	uint8_t ucReserved0;
	/* protocol link index to band index */
	uint8_t auMldLinkIdx[3];
	uint8_t aucReserved1[7];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_LINK_QUALITY {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
	struct UNI_LINK_QUALITY rLq;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_GET_ML_CHNL_COND {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Status;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_GET_ML_CHNL_COND_RESULT {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct ML_CHNL_COND_RESULT rChnlCond;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UEVENT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_LLS
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_TX_RATE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
	struct _STATS_LLS_TX_RATE_INFO rTxRateInfo;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_PRED_TX_BR {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
	struct STATS_LLS_TX_BIT_RATE rBitRate;
} __KAL_ATTRIB_PACKED__;
#endif

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SAP {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* SAP event Tag */
enum ENUM_UNI_EVENT_SAP_TAG {
	UNI_EVENT_SAP_TAG_AGING_TIMEOUT = 0,
	UNI_EVENT_SAP_TAG_UPDATE_STA_FREE_QUOTA = 1,
	UNI_EVENT_SAP_TAG_NOTIFY_AP_GO_STARTED = 2,
	UNI_EVENT_SAP_TAG_NOTIFY_LINK_TSF = 3,
	UNI_EVENT_SAP_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SAP_AGING_TIMEOUT {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_STA_FREE_QUOTA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint8_t  ucUpdateMode;
	uint8_t  ucFreeQuota;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_NOTIFY_AP_GO_STARTED {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucBssIdx;
	uint8_t  aucReserved[3];
} __KAL_ATTRIB_PACKED__;

struct LINK_TSF_ENTRY {
	uint8_t ucBssIdx;
	uint8_t aucReserved[3];
	uint32_t u4TsfBit0_31;
	uint32_t u4TsfBit63_32;
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LINK_TSF {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucLinkNum;
	uint8_t aucReserved[3];
	struct LINK_TSF_ENTRY aucLinkTsf[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SAP_DSCB_IE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucBssIndex;
	uint8_t  fgIsDscbEnable;
	uint16_t u2DscbBitmap;
	uint8_t  aucReserved[4];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RSSI_MONITOR {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*   TAG                         | ID  | structure
	*   --------------------------  | --- | -------------
	*   UNI_EVENT_RSSI_MONITOR_INFO | 0x0 | UNI_EVENT_RSSI_MONITOR_T
	*/
} __KAL_ATTRIB_PACKED__;

/* RSSI monitor event Tag */
enum ENUM_UNI_EVENT_RSSI_MONITOR_TAG {
	UNI_EVENT_RSSI_MONITOR_TAG_INFO = 0,
	UNI_EVENT_RSSI_MONITOR_TAG_NUM
};

/* Rssi monitor info (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RSSI_MONITOR_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;

	int32_t   cRssi;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_P2P {
	/*fixed field*/
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*                TAG              | ID  | structure
	*   ------------------------------| ----| -------------
	*   UNI_EVENT_UPDATE_NOA_PARAM    | 0x00| UNI_EVENT_UPDATE_NOA_PARAM_T
	*   UNI_EVENT_LO_STOP_PARAM       | 0x01| UNI_EVENT_LO_STOP_PARAM_T
	*/
} __KAL_ATTRIB_PACKED__;

/* P2P event Tag */
enum ENUM_UNI_EVENT_P2P_TAG {
	UNI_EVENT_P2P_TAG_UPDATE_NOA_PARAM = 0,
	UNI_EVENT_P2P_TAG_LO_STOP_PARAM = 1,
	UNI_EVENT_P2P_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_NOA_TIMING {
	uint8_t ucIsInUse;              /* Indicate if this entry is in use or not */
	uint8_t ucCount;                /* Count */
	uint8_t aucReserved[2];

	uint32_t u4Duration;             /* Duration */
	uint32_t u4Interval;             /* Interval */
	uint32_t u4StartTime;            /* Start Time */
} __KAL_ATTRIB_PACKED__;

/* UPDATE_NOA_PARAM (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_NOA_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t  ucBssIndex;
	uint8_t  ucEnableOppPS;
	uint16_t u2CTWindow;

	uint8_t  ucNoAIndex;
	uint8_t  ucNoATimingCount;
	uint8_t  aucReserved[2];
	struct UNI_NOA_TIMING  arEventNoaTiming[8/*P2P_MAXIMUM_NOA_COUNT*/];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_P2P_LO_STOP_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIndex;
	uint8_t aucReserved1[3];
	uint16_t u2ListenCount;
	uint8_t aucReserved2[2];
	uint32_t u4Reason;
	uint8_t aucReserved3[8];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                                      | ID  | structure
	*   -----------------------------------------|-----|--------------
	*   UNI_EVENT_CNM_CH_PRIVILEGE_GRANT         | 0x0 | UNI_EVENT_CH_PRIVILEGE_GRANT_T
	*   UNI_EVENT_CNM_GET_CHANNEL_INFO           | 0x1 | UNI_EVENT_CNM_GET_CHANNEL_INFO_T
	*   UNI_EVENT_CNM_GET_BSS_INFO               | 0x2 | UNI_EVENT_CNM_GET_BSS_INFO_T
	*   UNI_EVENT_CNM_OPMODE_CHANGE              | 0x3 | UNI_EVENT_CNM_OPMODE_CHANGE_T
	*   UNI_EVENT_CNM_CH_PRIVILEGE_MLO_SUB_GRANT | 0x4 | UNI_EVENT_CNM_CH_PRIVILEGE_GRANT_T
	*   UNI_EVENT_CNM_OPMODE_CHANGE_RDD          | 0x5 | UNI_EVENT_CNM_OPMODE_CHANGE_RDD_T
	*   UNI_EVENT_CNM_CH_GRANT_INFO              | 0x6 | UNI_EVENT_CNM_CH_GRANT_INFO_T
	*/
} __KAL_ATTRIB_PACKED__;

/** channel privilege command TLV List */
__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_EVENT_CNM_TAG {
	UNI_EVENT_CNM_TAG_CH_PRIVILEGE_GRANT = 0,
	UNI_EVENT_CNM_TAG_GET_CHANNEL_INFO = 1,
	UNI_EVENT_CNM_TAG_GET_BSS_INFO = 2,
	UNI_EVENT_CNM_TAG_OPMODE_CHANGE = 3,
	UNI_EVENT_CNM_TAG_CH_PRIVILEGE_MLO_SUB_GRANT = 4,
	UNI_EVENT_CNM_TAG_OPMODE_CHANGE_RDD = 5,
	UNI_EVENT_CNM_TAG_CH_GRANT_INFO = 6,
	UNI_EVENT_CNM_TAG_NUM
}__KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_CH_PRIVILEGE_GRANT {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint8_t          ucBssIndex;
	uint8_t          ucTokenID;
	uint8_t          ucStatus;
	uint8_t          ucPrimaryChannel;
	uint8_t          ucRfSco;
	uint8_t          ucRfBand;
	uint8_t          ucRfChannelWidth;   /* To support 80/160MHz bandwidth */
	uint8_t          ucRfCenterFreqSeg1; /* To support 80/160MHz bandwidth */
	uint8_t          ucRfCenterFreqSeg2; /* To support 80/160MHz bandwidth */
	uint8_t          ucReqType;
	uint8_t          ucDBDCBand;         // ENUM_CMD_REQ_DBDC_BAND_T
	uint8_t          aucReserved[1];
	uint32_t         u4GrantInterval;    /* In unit of ms */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_CH_GRANT_INFO {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint8_t          ucTxNss;        /*Set channel TX Nss*/
	uint8_t          ucRxNss;        /*Set channel RX Nss*/
	uint8_t          ucChannelWidth; /*Set channel Bandwidth*/
	uint8_t          aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_GET_CHANNEL_INFO {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	uint8_t      ucDBDCBand;
	uint8_t      fgIsCnmTimelineEnabled;
	uint8_t      ucOpChNum;
	uint8_t      fgIsDBDCEnabled;

	uint8_t      aucChnlInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_GET_BSS_INFO {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	uint8_t      ucBssNum;
	uint8_t      aucReserved[3];

	uint8_t      aucBssInfo[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_CHANNEL_INFO {
	uint8_t      ucPriChannel;
	uint8_t      ucChBw;
	uint8_t      ucChSco;
	uint8_t      ucChannelS1;
	uint8_t      ucChannelS2;
	uint8_t      ucChBssNum;
	uint16_t     u2ChBssBitmapList;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_BSS_INFO {
	uint8_t      ucBssType;
	uint8_t      ucBssInuse;
	uint8_t      ucBssActive;
	uint8_t      ucBssConnectState;
	uint8_t      ucBssPriChannel;
	uint8_t      ucBssDBDCBand;
	uint8_t      ucBssOMACIndex;
	uint8_t      ucBssOMACDBDCBand;
	uint8_t      ucBssOpTxNss;
	uint8_t      ucBssOpRxNss;
	uint8_t      ucBssLinkIdx;
	uint8_t      aucReserved[1];
}__KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_OPMODE_CHANGE {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint16_t         u2BssBitmap;
	uint8_t          ucEnable;
	uint8_t          ucOpTxNss;
	uint8_t          ucOpRxNss;
	uint8_t          ucReason;
	uint8_t          ucBandWidth;
	uint8_t          aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CNM_RDD_OPMODE_CHANGE {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint16_t         u2BssBitmap;
	uint8_t          ucEnable;
	uint8_t          ucOpTxNss;
	uint8_t          ucOpRxNss;
	uint8_t          ucReason;
	uint8_t          ucPriChannel;
	uint8_t          ucChBw;
	uint8_t          ucAction;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MBMC {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];

	/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   UNI_EVENT_MBMC_TAG_SWITCH_DONE   | 0x0 | UNI_EVENT_MBMC_SWITCH_DONE
	*/
} __KAL_ATTRIB_PACKED__;

/** MBMC Event TLV List */
__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_EVENT_MBMC_TAG {
	UNI_EVENT_MBMC_TAG_SWITCH_DONE = 0,
	UNI_EVENT_MBMC_TAG_NUM
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MBMC_SWITCH_DONE {
	uint16_t  u2Tag;
	uint16_t  u2Length;
	uint8_t   ucMBMCCmdSuccess;
	uint8_t   ucBssIndexValidBitmap;
	uint16_t  u2UsedBssBandIndexBitmap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_IS_ABSENCE {
	/* fixed field */
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* BSS Absence or Presence Event Tag */
enum ENUM_UNI_EVENT_BSS_IS_ABSENCE_TAG {
	UNI_EVENT_BSS_IS_ABSENCE_TAG_INFO = 0,
	UNI_EVENT_BSS_IS_ABSENCE_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_IS_ABSENCE_INFO {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	/* Event Body */
	uint8_t	    ucIsAbsent;
	uint8_t	    ucBssFreeQuota;
	uint8_t	    aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_STATUS_TO_HOST {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TXPOWER {
    /* fixed field */
    uint8_t aucPadding[4];
    /* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_TXPOWER_INFO_TAG {
	UNI_EVENT_TXPOWER_SHOW_INFO = 0,
	UNI_EVENT_TXPOWER_UPDATE_COMPENSATE_TABLE = 1,
	UNI_EVENT_TXPOWER_UPDATE_EPA_STATUS = 2,
	UNI_EVENT_TXPOWER_TARGET_POWER_INGO_GET = 3,
	UNI_EVENT_TXPOWER_SHOW_ALL_RATE_TXPOWER_INFO = 5,
	UNI_EVENT_TXPOWER_THERMAL_COMPENSATE_TABLE_SHOW_INFO = 6,
	UNI_EVENT_TXPOWER_TXV_BBP_POWER_SHOW_INFO = 7,
	UNI_EVENT_TXPOWER_POWER_SKU_TABLE_SHOW_INFO = 8,
	UNI_EVENT_TXPOWER_POWER_LIMIT_EMI_STATUS = 9,
	UNI_EVENT_TXPOWER_INFO_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TXPOWER_RSP {
    uint16_t u2Tag;
    uint16_t u2Length;
	uint8_t aucBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_WSYS_CONFIG {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_WSYS_CONFIG_TAG {
	UNI_EVENT_FW_LOG_UI_INFO = 0,
	UNI_EVENT_FW_LOG_BUFFER_CTRL = 1,
	UNI_EVENT_WSYS_CONFIG_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_FW_LOG_BUFFER_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucType;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
	uint32_t u4Address;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BA_OFFLOAD {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_BA_OFFLOAD_TAG {
	UNI_EVENT_BA_OFFLOAD_TAG_RX_ADDBA  = 0,
	UNI_EVENT_BA_OFFLOAD_TAG_RX_DELBA  = 1,
	UNI_EVENT_BA_OFFLOAD_TAG_TX_ADDBA  = 2,
	UNI_EVENT_BA_OFFLOAD_TAG_TX_AGG_LIMIT  = 3,
	UNI_EVENT_BA_OFFLOAD_TAG_TX_AMSDU_NUM_LIMIT  = 4,
	UNI_EVENT_BA_OFFLOAD_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RX_ADDBA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint16_t u2WinSize;
	uint16_t u2BATimeoutValue;
	uint16_t u2BAStartSeqCtrl;
	uint8_t	ucDialogToken;
	uint8_t	ucTid;
	uint8_t	aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RX_DELBA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint8_t	ucTid;
	uint8_t	aucReserved[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TX_ADDBA {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint16_t u2WinSize;
	uint16_t u2SSN;
	uint8_t	ucTid;
	/* AMSDU in AMPDU is enabled or not (TID bitmap)*/
	uint8_t	ucAmsduEnBitmap;
	/* Max AMSDU length */
	uint8_t u4MaxMpduLen;
	/*
	 * Note: If length of a packet < u4MinMpduLen then it shall not be
	 * aggregated in an AMSDU
	 */
	/* Min MPDU length to be AMSDU */
	uint8_t u4MinMpduLen;
	/*
	 * AMSDU count/length limits by count *OR* length
	 * Count: MPDU count in an AMSDU shall not exceed ucMaxMpduCount
	 * Length: AMSDU length shall not exceed u4MaxMpduLen
	 */
	/* Max MPDU count in an AMSDU */
	uint8_t	ucMaxMpduCount;
	uint8_t	aucReserved[15];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TX_AGG_LIMIT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	/* 0: success, others: fail */
	uint8_t ucStatus;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TX_AMSDU_NUM_LIMIT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIdx;
	/* 0: success, others: fail */
	uint8_t ucStatus;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

/* status to host event tag */
enum ENUM_UNI_EVENT_STATUS_TO_HOST_TAG {
	UNI_EVENT_STATUS_TO_HOST_TAG_TX_DONE  = 0,
	UNI_EVENT_STATUS_TO_HOST_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TX_DONE {
	uint16_t     u2Tag;
	uint16_t     u2Length;

	/** HW PID */
	uint8_t      ucPacketSeq;
	/** TX_RESULT_xx */
	uint8_t      ucStatus;
	/** Packet sequence number */
	uint16_t     u2SequenceNumber;

	/** WLAN index (WTBL) */
	uint8_t      ucWlanIndex;
	/** TX count (includes retry count) */
	uint8_t      ucTxCount;
	/** TX success rate (see right page) */
	uint16_t     u2TxRate;

	/** TXS_WITH_ADVANCED_INFO or TXS_IS_EXIST */
	uint8_t      ucFlag;
	/** Packet TID */
	uint8_t      ucTid;
	/** Response rate */
	uint8_t      ucRspRate;
	/** The last transmission rate index from WLAN table */
	uint8_t      ucRateTableIdx;

	/** TX Bandwidth, the bandwidth used to transmit this PPDU */
	uint8_t      ucBandwidth;
	uint8_t      ucTxPower; /** TX Power dBm */
	/** (SW) the packet is flushed to tx done, here is the reason */
	uint8_t      ucFlushReason;
	uint8_t      aucReserved0[1];

	/** Unit is 32us. Transmitted by UMAC till UMAC gets tx status */
	uint32_t     u4TxDelay;
	/** local TSF when 1st bit of MAC header TX from MAC to PHY */
	uint32_t     u4Timestamp;
	/** (SW) Reference to _ENUM_TXS_APPLIED_FLAG_T */
	uint32_t     u4AppliedFlag;
}__KAL_ATTRIB_PACKED__;

/* BF event (0x33) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BF {
	/* fixed field */
	uint8_t au1Reserved[4];

	/* tlv */
	uint8_t au1TlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* BF event tags */
enum ENUM_UNI_EVENT_BF_TAG {
	UNI_EVENT_BF_TAG_PFMU = 0x5,
	UNI_EVENT_BF_TAG_PFMU_DATA = 0x7,
	UNI_EVENT_BF_TAG_PFMU_MEM_ALLOC_MAP = 0x9,
	UNI_EVENT_BF_TAG_STAREC = 0xB,
	UNI_EVENT_BF_TAG_CAL_PHASE = 0xC,
	UNI_EVENT_BF_TAG_FBK_INFO = 0x17,
	UNI_EVENT_BF_TAG_TXSND_INFO = 0x18,
	UNI_EVENT_BF_TAG_PLY_INFO = 0x19,
	UNI_EVENT_BF_TAG_METRIC_INFO = 0x1A,
	UNI_EVENT_BF_TAG_TXCMD_CFG_INFO = 0x1B,
	UNI_EVENT_BF_TAG_SND_CNT_INFO = 0x1D,
	UNI_EVENT_BF_TAG_NUM
};

#if CFG_SUPPORT_TX_BF
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BF_STA_REC {
	uint16_t u2Tag;
	uint16_t u2Length;

	struct TXBF_PFMU_STA_INFO rTxBfPfmuInfo;
} __KAL_ATTRIB_PACKED__;

struct UNI_EVENT_BF_PFMU_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t fgBfer;
	uint8_t au1Reserved[3];
	union PFMU_PROFILE_TAG1 ru4TxBfPFMUTag1;
	union PFMU_PROFILE_TAG2 ru4TxBfPFMUTag2;
};
#endif

#define UNI_THERMAL_PROTECT_TYPE_NTX_CTRL	0
#define UNI_THERMAL_PROTECT_TYPE_DUTY_CTRL	1
#define UNI_THERMAL_PROTECT_TYPE_RADIO_CTRL	2
#define UNI_THERMAL_PROTECT_TYPE_NUM		3

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL {
    /* fixed field */
    uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_RSP {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t aucBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_SENSOR_INFO {
	uint8_t ucCategory;
	uint8_t ucReserved[3];
	uint32_t u4SensorResult;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_DDIE_SENSOR_INFO {
	uint8_t ucCategory;
	uint8_t ucReserved[3];
	uint32_t u4SensorResult;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_TEMP_ADC_INFO {
	uint8_t ucCategory;
	uint8_t ucReserved[3];
	uint32_t u4Temp;
	uint32_t u4Adc;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_PROTECT_MECH_INFO {
	uint8_t ucSubEventId;
	uint8_t uc1BandIdx;
	uint8_t ucReserved[2];
	uint8_t ucProtectionType[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint8_t ucReserved2;
	uint8_t ucTriggerType[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint8_t ucReserved3;
	int32_t i4TriggerTemp[UNI_THERMAL_PROTECT_TYPE_NUM];
	int32_t i4RestoreTemp[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint16_t u2RecheckTime[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint8_t ucReserved4[2];
	uint8_t ucState[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint8_t ucReserved6;
	bool fgEnable[UNI_THERMAL_PROTECT_TYPE_NUM];
	uint8_t ucReserved7;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_THERMAL_PROTECT_DUTY_INFO {
	uint8_t ucSubEventId;
	uint8_t ucBandIdx;
	uint8_t ucDuty0;
	uint8_t ucDuty1;
	uint8_t ucDuty2;
	uint8_t ucDuty3;
	uint8_t ucReserved[2];
} __KAL_ATTRIB_PACKED__;

enum UNI_THERMAL_EVENT_CATEGORY {
	UNI_THERMAL_EVENT_TEMPERATURE_INFO = 0x0,
	UNI_THERMAL_EVENT_THERMAL_SENSOR_BASIC_INFO = 0x1,
	UNI_THERMAL_EVENT_THERMAL_SENSOR_TASK_RESPONSE = 0x2,
	UNI_THERMAL_EVENT_THERMAL_PROTECT_MECH_INFO = 0x3,
	UNI_THERMAL_EVENT_THERMAL_PROTECT_DUTY_INFO = 0x4,
	UNI_THERMAL_EVENT_DDIE_SENSOR_INFO = 0x5,
	UNI_THERMAL_EVENT_SENSOR_ADC_TEMP_INFO = 0x05,
	UNI_THERMAL_EVENT_THERMAL_PROTECT_DUTY_UPDATE = 0x6,
	UNI_THERMAL_EVENT_THERMAL_PROTECT_RADIO_UPDATE = 0x7,
	UNI_THERMAL_EVENT_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CHIP_CAPABILITY
{
	/* fixed field */
	uint16_t u2TotalElementNum;
	uint8_t aucPadding[2];
	/* tlv */

	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* CHIP CAPABILITY Tag */
enum ENUM_UNI_EVENT_CHIP_CAPABILITY_TAG
{
	UNI_EVENT_CHIP_CAPABILITY_TAG_TX_RESOURCE = 0x0,
	UNI_EVENT_CHIP_CAPABILITY_TAG_TX_EFUSEADDRESS = 0x1,
	UNI_EVENT_CHIP_CAPABILITY_TAG_COEX_FEATURE = 0x2,
	UNI_EVENT_CHIP_CAPABILITY_TAG_SINGLE_SKU = 0x3,
	UNI_EVENT_CHIP_CAPABILITY_TAG_CSUM_OFFLOAD = 0x4,
	UNI_EVENT_CHIP_CAPABILITY_TAG_HW_VERSION = 0x5,
	UNI_EVENT_CHIP_CAPABILITY_TAG_SW_VERSION = 0x6,
	UNI_EVENT_CHIP_CAPABILITY_TAG_MAC_ADDR = 0x7,
	UNI_EVENT_CHIP_CAPABILITY_TAG_PHY_CAP = 0x8,
	UNI_EVENT_CHIP_CAPABILITY_TAG_MAC_CAP = 0x9,
	UNI_EVENT_CHIP_CAPABILITY_TAG_FRAME_BUF_CAP = 0xa,
	UNI_EVENT_CHIP_CAPABILITY_TAG_BEAMFORM_CAP = 0xb,
	UNI_EVENT_CHIP_CAPABILITY_TAG_LOCATION_CAP = 0xc,
	UNI_EVENT_CHIP_CAPABILITY_TAG_MUMIMO_CAP = 0xd,
	UNI_EVENT_CHIP_CAPABILITY_TAG_BUFFER_MODE_INFO = 0xe,
	UNI_EVENT_CHIP_CAPABILITY_TAG_R_MODE_CAP = 0xf,
	UNI_EVENT_CHIP_CAPABILITY_TAG_CMD_ID_SUPPORT_LIST = 0x10,
	UNI_EVENT_CHIP_CAPABILITY_TAG_CMD_EXTID_SUPPORT_LIST = 0x11,
	UNI_EVENT_CHIP_CAPABILITY_TAG_CMD_TAGID_SUPPORT_LIST_24 = 0x12,
	UNI_EVENT_CHIP_CAPABILITY_TAG_CMD_TAGID_SUPPORT_LIST_56 = 0x13,
	UNI_EVENT_CHIP_CAPABILITY_TAG_HW_ADIE_VERSION = 0x14,
	UNI_EVENT_CHIP_CAPABILITY_TAG_ATCMD_FEATURE_SUPPORT_LIST = 0x15,
	UNI_EVENT_CHIP_CAPABILITY_TAG_ANT_SWAP = 0x16,
	UNI_EVENT_CHIP_CAPABILITY_TAG_WFDMA_REALLOC = 0x17,
	UNI_EVENT_CHIP_CAPABILITY_TAG_HOST_STATUS_ADDRESS = 0x19,
	UNI_EVENT_CHIP_CAPABILITY_TAG_FAST_PATH = 0x1A,
	UNI_EVENT_CHIP_CAPABILITY_TAG_PSE_RX_QUOTA = 0x1B,
	UNI_EVENT_CHIP_CAPABILITY_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_COEX
{
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                              | ID  | structure
	*   ---------------------------------|-----|--------------
	*   EVENT_UPDATE_COEX_PHYRATE          | 0x0 | EVENT_UPDATE_COEX_PHYRATE (Should always be the last TLV element)
	*/
} __KAL_ATTRIB_PACKED__;

/* FW Log 2 Host event Tag */
enum ENUM_UNI_EVENT_UPDATE_COEX_TAG
{
	UNI_EVENT_UPDATE_COEX_TAG_PHYRATE = 0,
	UNI_EVENT_UPDATE_COEX_TAG_NUM
};

/* FW Log with Format (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_COEX_PHYRATE
{
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Flags;
	uint32_t au4PhyRateLimit[MAX_BSSID_NUM + 1];
} __KAL_ATTRIB_PACKED__;

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 *  @page
 *     <br/>
 *     This event is for testmode RF test. <br/>
 */
/**
 * This structure is used for UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * for testmode RF test
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] ucReserved       Reserved
 * @param[in] aucTlvBuffer     TLVs
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_CTRL {
	/*fix field*/
	uint8_t au1Reserved[4];

	/*tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	*   TAG                           | ID  | structure
	*   ------------------------------|-----|--------------
	*   UNI_EVENT_TESTMODE_TAG_RESULT | 0x0 | UNI_EVENT_TESTMODE_RESULT_TLV
	*   UNI_EVENT_TESTMODE_TAG_XO_CAL | 0x1 | UNI_EVENT_TESTMODE_XO_CAL
	*   UNI_EVENT_TESTMODE_TAG_PL_CAL | 0x2 | UNI_EVENT_TESTMODE_PL_CAL
	*/
} __KAL_ATTRIB_PACKED__;
/** @} */

/* testmode RF test event tag */
enum UNI_EVENT_TESTMODE_TAG {
	UNI_EVENT_TESTMODE_TAG_RESULT = 0x0,
	UNI_EVENT_TESTMODE_TAG_XO_CAL = 0x1,
	UNI_EVENT_TESTMODE_TAG_PL_CAL = 0x2,
	UNI_EVENT_TESTMODE_TAG_NUM
};

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_RESULT tag(0x0) of
 * UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * to report testmode RF status.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x00
 * @param[in] u2Length             the length of this TLV
 * @param[in] aucBuffer            Icap , recal event
 */
/* Testmode RF status (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_RESULT_TLV {
    uint16_t u2Tag;
    uint16_t u2Length;
	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_RESULT {
	uint32_t u4FuncIndex;
	uint32_t u4PayloadLength;
	uint8_t  aucEvent[];
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_TAG_XO_CAL (0x1) of
 * UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * to report testmode XO calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u4AxmFreq     XO AXM mode measured frequency
 * @param[in] u4AxmC1Freq   XO AXM mode C1 frequency value
 * @param[in] u4AxmC2Freq   XO AXM mode C2 frequency value
 * @param[in] u4AxmC1Comp   XO AXM mode C1 compensation value
 * @param[in] u4AxmC2Comp   XO AXM mode C2 compensation value
 * @param[in] u4BtmFreq     XO BTM mode measured frequency
 * @param[in] u4BtmC1Freq   XO BTM mode C1 frequency value
 * @param[in] u4BtmC2Freq   XO BTM mode C2 frequency value
 * @param[in] u4BtmC1Comp   XO BTM mode C1 compensation value
 * @param[in] u4BtmC2Comp   XO BTM mode C2 compensation value
 */
/* Testmode XO calibration data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_XO_CAL_DATA {
	uint32_t u4AxmFreq;
	uint32_t u4AxmC1Freq;
	uint32_t u4AxmC2Freq;
	uint32_t u4AxmC1Comp;
	uint32_t u4AxmC2Comp;

	uint32_t u4BtmFreq;
	uint32_t u4BtmC1Freq;
	uint32_t u4BtmC2Freq;
	uint32_t u4BtmC1Comp;
	uint32_t u4BtmC2Comp;
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_TAG_XO_CAL (0x1) of
 * UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * to report testmode XO calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag      should be 0x01
 * @param[in] u2Length   the length of this TLV
 * @param[in] rXoCal     the XO calibration data
 */
/* Testmode XO calibration data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_XO_CAL {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_EVENT_TESTMODE_XO_CAL_DATA rXoCal;
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_TAG_PL_CAL (0x2) of
 * UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * to report testmode PL calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u4OutCnt    the number of output data
 * @param[in] u4OutData   the output data
 */
/* Testmode PL calibration data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_PL_CAL_DATA {
	uint32_t u4OutCnt;
	uint32_t u4OutData[PLCAL_MAX_CNT];
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_EVENT_ID_TESTMODE_CTRL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_TAG_PL_CAL (0x2) of
 * UNI_EVENT_ID_TESTMODE_CTRL event (0x46)
 * to report testmode PL calibration data.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x02
 * @param[in] u2Length             the length of this TLV
 * @param[in] rPlCal               the PL calibration data
 */
/* Testmode PL calibration data (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_PL_CAL {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_EVENT_TESTMODE_PL_CAL_DATA rPlCal;
} __KAL_ATTRIB_PACKED__;
/** @} */

/** @addtogroup UNI_EVENT_ID_TESTMODE_RX_STAT_INFO
 * @{
 *  @page
 *     <br/>
 *     This event is for testmode rx statistic related operation. <br/>
 */
/**
 * This structure is used for UNI_EVENT_ID_TESTMODE_RX_STAT_INFO event (0x32)
 * for testmode rx statistic related operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] ucReserved       Reserved
 * @param[in] aucTlvBuffer     TLVs
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_RX_STAT {
	/* fix field */
	uint8_t u1TagNum;
	uint8_t au1Reserved[3];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*
	* TAG                                   |ID |structure
	* --------------------------------------|---|-------------
	* UNI_EVENT_TESTMODE_RX_STAT_TAG_GET_ALL|0x6|UNI_EVENT_TESTMODE_RX_STAT_ALL
	*/
} __KAL_ATTRIB_PACKED__;
/** @} */

/* testmode rx statistic event tag */
enum UNI_EVENT_TESTMODE_RX_STAT_TAG {
	UNI_EVENT_TESTMODE_RX_STAT_TAG_GET_ALL = 0x6,
	UNI_EVENT_TESTMODE_RX_STAT_TAG_NUM
};
/** @addtogroup UNI_EVENT_ID_TESTMODE_RX_STAT_ALL
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_RX_STAT_TAG_GET_ALL(0x06)
 * of UNI_EVENT_ID_TESTMODE_RX_STAT_INFO command (0x32)
 * to update common part of testmode rx statistic related information.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x06
 * @param[in] u2Length      the length of this TLV
 * @param[in] pu4Buffer     pointer of rx statistic all
 */
/* Update rx statistic all event struct (Tag 0x06) */

#define UNI_EVENT_TESTMODE_RX_STAT_ALL_ITEM	76
#define UNI_TM_MAX_BAND_NUM 4
#define UNI_TM_MAX_ANT_NUM 8
#define UNI_TM_MAX_USER_NUM 16

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_STAT_ALL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4Buffer[UNI_EVENT_TESTMODE_RX_STAT_ALL_ITEM];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_BAND {
	/* mac part */
	uint16_t u2MacRxFcsErrCnt;
	uint16_t u2MacRxLenMisMatch;
	uint16_t u2MacRxFcsOkCnt;
	uint8_t u1Reserved1[2];
	uint32_t u4MacRxMdrdyCnt;

	/* phy part */
	uint16_t u2PhyRxFcsErrCntCck;
	uint16_t u2PhyRxFcsErrCntOfdm;
	uint16_t u2PhyRxPdCck;
	uint16_t u2PhyRxPdOfdm;
	uint16_t u2PhyRxSigErrCck;
	uint16_t u2PhyRxSfdErrCck;
	uint16_t u2PhyRxSigErrOfdm;
	uint16_t u2PhyRxTagErrOfdm;
	uint16_t u2PhyRxMdrdyCntCck;
	uint16_t u2PhyRxMdrdyCntOfdm;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_USER {
	int32_t i4FreqOffsetFromRx;
	int32_t i4Snr;
	uint32_t u4FcsErrorCnt;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_COMM {
	uint16_t u2MacRxFifoFull;
	uint8_t u1Reserved1[2];

	uint32_t u4AciHitLow;
	uint32_t u4AciHitHigh;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_RXV {
	uint16_t u2Rcpi;
	int16_t i2Rssi;
	int16_t i2Snr;
	int16_t i2AdcRssi;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_RSSI {
	int8_t i1RssiIb;
	int8_t i1RssiWb;
	uint16_t u2CcaIdlePwr;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_BAND_EXT1 {
	/* mac part */
	uint32_t u4RxU2MMpduCnt;

	/* Common part*/
	uint8_t u1BandIdx;
	uint8_t u1Reserved;

	/* phy part */
	uint16_t u2PhyRxPdAlr;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_COMM_EXT1 {
	uint32_t u4DrvRxCnt;
	uint32_t u4Sinr;
	uint32_t u4MuRxCnt;
	/* mac part */
	uint8_t u1Reserved0[4];

	/* phy part */
	uint8_t u1EhtSigMcs;
	uint8_t u1Reserved1[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_TESTMODE_STATINFO_USER_EXT1 {
	int8_t i1NeVarDbAllUser;
	uint8_t u1Reserved1[3];
} __KAL_ATTRIB_PACKED__;

/** @addtogroup UNI_EVENT_ID_TESTMODE_RX_STAT_INFO
 * @{
 */
/**
 * This structure is used for UNI_EVENT_TESTMODE_RX_STAT(0x01) of
 * UNI_EVENT_ID_TESTMODE_RX_STAT_INFO command (0x32) to update
 * testmode rx statistic related information.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag         should be 0x01
 * @param[in] u2Length      the length of this TLV
 * @param[in] rInfoBand     show band part of rx statistic related information
 * @param[in] rInfoBandExt1 show band part of rx statistic related information
 * @param[in] rInfoComm     show common part of rx statistic related information
 * @param[in] rInfoRXV      show rxv part of rx statistic related information
 * @param[in] rInfoFagc     show fagc part of rx statistic related information
 * @param[in] rInfoInst     show inst part of rx statistic related information
 * @param[in] rInfoUser     show user part of rx statistic related information
 */
/* Update rx statistic event struct (Tag 0x07) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_TESTMODE_STAT_ALL_V2 {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct UNI_TESTMODE_STATINFO_BAND rInfoBand;
	struct UNI_TESTMODE_STATINFO_BAND_EXT1 rInfoBandExt1;
	struct UNI_TESTMODE_STATINFO_COMM rInfoComm;
	struct UNI_TESTMODE_STATINFO_COMM_EXT1 rInfoCommExt1;

	/* rxv part */
	struct UNI_TESTMODE_STATINFO_RXV rInfoRXV[UNI_TM_MAX_ANT_NUM];

	/* RSSI */
	struct UNI_TESTMODE_STATINFO_RSSI rInfoFagc[UNI_TM_MAX_ANT_NUM];
	struct UNI_TESTMODE_STATINFO_RSSI rInfoInst[UNI_TM_MAX_ANT_NUM];

	/* User */
	struct UNI_TESTMODE_STATINFO_USER rInfoUser[UNI_TM_MAX_USER_NUM];
	struct UNI_TESTMODE_STATINFO_USER_EXT1
		rInfoUserExt1[UNI_TM_MAX_USER_NUM];
} __KAL_ATTRIB_PACKED__;
/** @} */

/* BSS ER event */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BSS_ER {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* BSS ER event tags */
enum ENUM_UNI_EVENT_BSS_ER_TAG {
	UNI_EVENT_BSS_ER_TAG_TX_MODE = 0,
	UNI_EVENT_MLR_TAG_FSM_UPDATE = 1,
	UNI_EVENT_BSS_ER_TAG_NUM
};

struct UNI_EVENT_BSS_ER_TX_MODE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssInfoIdx;
	uint8_t ucErMode; //ENUM_EVENT_ER_TX_MODE_ER_MODE_T
	uint8_t aucPadding1[2];
	uint8_t aucPadding2[16];
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLR_FSM_UPDATE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2WlanIdx;
	uint8_t ucMlrMode;  /* ENUM_MLR_MODE */
	uint8_t ucMlrState; /* ENUM_MLR_STATE */
	/* MLR TXD fixed rate index (only used for REBB segment) */
	uint8_t ucMlrTxdFrIdx;
	/* MLR enable Tx fragment or not */
	uint8_t ucTxFragEn;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;

/* EFUSE event Tag */
enum ENUM_UNI_EVENT_EFUSE_TAG {
	UNI_EVENT_EFUSE_TAG_BUFFER_MODE_READ = 0,
	UNI_EVENT_EFUSE_TAG_FREE_BLOCK = 1,
	UNI_EVENT_EFUSE_TAG_ACCESS = 2,
	UNI_EVENT_EFUSE_TAG_MAX_NUM
};

/** This structure is used for UNI_EVENT_ID_EFUSE event (0x48)
 * to do EFUSE related operation
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucReserved[4]
 * @param[in] aucTlvBuffer
 *
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_EFUSE_CONTROL {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*TAG                             | ID  |structure
	*-------------                   | --- |-------------
	*UNI_EVENT_EFUSE_BUFFER_MODE_READ| 0x0 |UNI_EVENT_EFUSE_BUFFER_MODE_READ_T
	*UNI_EVENT_EFUSE_FREE_BLOCK      | 0x1 |UNI_EVENT_EFUSE_FREE_BLOCK_T
	*UNI_EVENT_EFUSE_ACCESS          | 0x2 |UNI_EVENT_EFUSE_ACCESS_T
	*/
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_EFUSE_BUFFER_MODE_READ tag(0x00)
 * of UNI_EVENT_ID_EFUSE_CONTROL Event (0x48)
 * to get Buffer mode read data
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x00
 * @param[in] u2Length             the length of this TLV
 * @param[in] u1SourceMode         0: eFuse mode; 1: Buffer mode
 * @param[in] u1ContentFormat      0: Bin Content; 1: Whole Content;
 *                                 2: Multiple Sections
 * @param[in] u2Offset             Read Offset
 * @param[in] u2Count              Read Total Counts
 * @param[out] BinContent[];       the content of read
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_EFUSE_BUFFER_MODE_READ {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  u1SourceMode;       /* 0: eFuse mode; 1: Buffer mode */
	uint8_t  u1ContentFormat;    /* 0: Bin Content;
                                    1: Whole Content;
                                    2: Multiple Sections */
	uint16_t u2Offset;           /* Read Offset */
	uint32_t u2Count;            /* Read Total Counts */
	uint8_t  BinContent[];       /* The content of read */
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_EFUSE_FREE_BLOCK
 * tag(0x01) of UNI_EVENT_ID_EFUSE_CONTROL Event (0x48)
 * to free EFUSE memory.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x01
 * @param[in] u2Length             the length of this TLV
 * @param[in] ucGetFreeBlock       the get back free block
 * @param[in] ucVersion            0: original format;
 *                                 1: modified format
 * @param[in] ucTotalBlockNum      Total Block
 * @param[in] ucReserved
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_EFUSE_FREE_BLOCK {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucGetFreeBlock;
	uint8_t  ucVersion;        /* 0: original format ; 1: modified format */
	uint8_t  ucTotalBlockNum;  /* Total Block */
	uint8_t  ucReserved;
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_EFUSE_ACCESS tag(0x02) of
 * UNI_EVENT_ID_EFUSE_CONTROL Event (0x48)
 * to access EFUSE info.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x01
 * @param[in] u2Length             the length of this TLV
 * @param[in] u4EventVer           event version
 * @param[in] u4Address            read address
 * @param[in] u4Valid              [0]:1 -> valid, [0]:0 -> invalid
 * @param[in] u4Size               get size
 * @param[in] u4MagicNum           magic number
 * @param[in] u4Type               Reserved
 * @param[in] u4Reserved[4]        Reserved
 * @param[in] aucData[32]          get data
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_EFUSE_ACCESS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4EventVer;
	uint32_t u4Address;
	uint32_t u4Valid;       /* Compatible to original CMD field.
                              Currently only use bit 0
                              [0]:1 -> valid, [0]:0 -> invalid */
	uint32_t u4Size;
	uint32_t u4MagicNum;
	uint32_t u4Type;        /* Reserved */
	uint32_t u4Reserved[4]; /* Reserved */
	uint8_t aucData[32];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_RTT
/* RTT event (0x5D) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RTT {
	/*fixed field*/
	uint8_t ucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[]; /**< the TLVs included in this field:
	*
	*                TAG               | ID   | structure
	*   -------------------------------| -----| -------------
	*   UNI_EVENT_RTT_TAG_RTT_CAPA     | 0x00 | UNI_EVENT_RTT_CAPA_T
	*   UNI_EVENT_RTT_TAG_RTT_RESULT   | 0x01 | UNI_EVENT_RTT_RESULT_T
	*   UNI_EVENT_RTT_TAG_RTT_DONE     | 0x02 | UNI_EVENT_RTT_DONE_T
	*/
} __KAL_ATTRIB_PACKED__;
/* RTT command TLV List */
enum ENUM_UNI_EVENT_RTT_TAG {
	UNI_EVENT_RTT_TAG_RTT_CAPA = 0,
	UNI_EVENT_RTT_TAG_RTT_RESULT = 1,
	UNI_EVENT_RTT_TAG_RTT_DONE = 2,
	UNI_EVENT_RTT_TAG_NUM
};

/* Location capabilities (CMD_GET_LOC_CAP_T) retrieved from FW */
__KAL_ATTRIB_PACKED_FRONT__
struct LOC_CAPABILITIES_T {
	uint16_t u2LocInitSupported;
	uint16_t u2LocResSupported;
	uint8_t ucLciSupport;
	uint8_t ucLcrSupport;
	uint16_t u2PreambleSupport;
	uint16_t u2BwSupport;
	uint16_t u2AzBwSupport;
	uint32_t u4MinDeltaTimePerPacket;
	uint32_t u4Reserved;
} __KAL_ATTRIB_PACKED__;

/* RTT capabilities (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RTT_CAPA_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct LOC_CAPABILITIES_T rCapabilities;
} __KAL_ATTRIB_PACKED__;
/* RTT result (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RTT_RESULT_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucNumPerBurstPeer;
	uint8_t ucRetryAfterDuration;
	uint32_t u4BurstNum;
	uint32_t u4MeasurementNumber;
	uint32_t u4SuccessNumber;
	uint32_t eStatus; /* enum ENUM_RTT_STATUS */
	uint32_t eType; /* enum ENUM_RTT_TYPE */
	int32_t i4Rssi;
	int32_t i4RssiSpread;
	struct WIFI_RATE rTxRate;
	struct WIFI_RATE rRxRate;
	int64_t i8Rtt;
	int64_t i8RttSd;
	int64_t i8RttSpread;
	int32_t i4DistanceMM;
	int32_t i4DistanceSdMM;
	int32_t i4DistanceSpreadMM;
	int64_t i8Ts;
	int32_t i4BurstDuration;
	int32_t i4NegotiatedBustNum;
	uint16_t u2IELen;
	uint8_t aucReserved[2];
	/* Keep it last */
	uint8_t aucIE[];
} __KAL_ATTRIB_PACKED__;
/* RTT done (Tag2) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_RTT_DONE_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucSeqNum;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_RTT */

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_EVENT_TLV_ELEMENT_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucbody[];
} __KAL_ATTRIB_PACKED__;

#if CFG_MTK_MDDP_SUPPORT
/** @addtogroup UNI_EVENT_ID_MDDP
 *  @{
 */

/** This structure is used for UNI_EVENT_ID_MDDP event (0x55)
 * to response MDDP operation.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] ucBand           operation band
 * @param[in] aucTlvBuffer     TLVs
 */
struct UNI_EVENT_MDDP {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *   TAG                          | ID  | structure
	 *   -------------                | ----| -------------
	 *   UNI_EVENT_MDDP_FILTER_RULE   | 0x0 | UNI_EVENT_MDDP_FILTER_RULE_T
	 *   UNI_EVENT_MDDP_EXCEPTION     | 0x1 | UNI_EVENT_MDDP_EXCEPTION_T
	 *
	 */
};
/** @} */

/* MDDP event tag */
enum UNI_EVENT_MDDP_TAG {
	UNI_EVENT_MDDP_FILTER_RULE,
	UNI_EVENT_MDDP_EXCEPTION,
	UNI_EVENT_MDDP_FWOWN_RETRY,
	UNI_EVENT_MDDP_MAX_NUM
};

/** @addtogroup UNI_EVENT_ID_MDDP
 * @{
 */
/**
 * This structure is used for UNI_EVENT_MDDP_FILTER_RULE
 * tag(0x0) of UNI_EVENT_ID_MDDP event (0x55)
 * to report MDDP packet filter bitmap.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x00
 * @param[in] u2Length             the length of this TLV
 * @param[in] ucPfType             the type of packet filter
 * @param[in] ucPfNum              the number of packet filter
 * @param[in] aucPfStatusBitmap    the status bitmap of packet filter
 */
/* CSI data (Tag0) */
struct UNI_EVENT_MDDP_FILTER_RULE {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucPfType;
	uint8_t  ucPfNum;
	uint8_t  aucPadding1[2];
	uint8_t  aucPfStatusBitmap[256];
};
/** @} */

/** @addtogroup UNI_EVENT_ID_MDDP
 * @{
 */
/**
 * This structure is used for UNI_EVENT_MDDP_EXCEPTION
 * tag(0x1) of UNI_EVENT_ID_MDDP event (0x55)
 * to report MDDP exception.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x01
 * @param[in] u2Length             the length of this TLV
 * @param[in] u4ExceptionIdx       the index of MDDP exception
 */
/* CSI data (Tag0) */
struct UNI_EVENT_MDDP_EXCEPTION {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4ExceptionIdx;
};

struct UNI_EVENT_MDDP_FWOWN_RETRY {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4RetryCnt;
};
/** @} */
#endif /* CFG_MTK_MDDP_SUPPORT */

#if CFG_SUPPORT_NAN
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_NAN {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];

} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_NAN_TAG {
	UNI_EVENT_NAN_TAG_DISCOVERY_RESULT = 0,
	UNI_EVENT_NAN_TAG_FOLLOW_EVENT = 1,
	UNI_EVENT_NAN_TAG_MASTER_IND_ATTR = 2,
	UNI_EVENT_NAN_TAG_CLUSTER_ID_UPDATE = 3,
	UNI_EVENT_NAN_TAG_REPLIED_EVENT = 4,
	UNI_EVENT_NAN_TAG_PUBLISH_TERMINATE_EVENT = 5,
	UNI_EVENT_NAN_TAG_SUBSCRIBE_TERMINATE_EVENT = 6,
	UNI_EVENT_NAN_TAG_ID_SCHEDULE_CONFIG = 7,
	UNI_EVENT_NAN_TAG_ID_PEER_AVAILABILITY = 8,
	UNI_EVENT_NAN_TAG_ID_PEER_CAPABILITY = 9,
	UNI_EVENT_NAN_TAG_ID_CRB_HANDSHAKE_TOKEN = 10,
	UNI_EVENT_NAN_TAG_ID_DATA_NOTIFY = 11,
	UNI_EVENT_NAN_TAG_FTM_DONE = 12,
	UNI_EVENT_NAN_TAG_RANGING_BY_DISC = 13,
	UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL = 14,
	UNI_EVENT_NAN_TAG_DW_INTERVAL = 15,
	UNI_EVENT_NAN_TAG_NDL_DISCONNECT = 16,
	UNI_EVENT_NAN_TAG_ID_PEER_CIPHER_SUITE_INFO = 17,
	UNI_EVENT_NAN_TAG_ID_PEER_SEC_CONTEXT_INFO = 18,
	UNI_EVENT_NAN_TAG_ID_DE_EVENT_IND = 19,
	UNI_EVENT_NAN_TAG_SELF_FOLLOW_EVENT = 20,
	UNI_EVENT_NAN_TAG_DISABLE_IND = 21,
	UNI_EVENT_NAN_TAG_NDL_FLOW_CTRL_V2 = 22,
	UNI_EVENT_NAN_TAG_ID_DEVICE_CAPABILITY = 23,
	UNI_EVENT_NAN_ID_MATCH_EXPIRE = 24,
	UNI_EVENT_NAN_TAG_DISC_BCN_PERIOD = 25,
	UNI_EVENT_NAN_DEVICE_INFO = 27,
	UNI_EVENT_NAN_TAG_REPORT_BEACON = 28,
	UNI_EVENT_NAN_TAG_SLOT_STATISTICS = 29,
	UNI_EVENT_NAN_TAG_LOWPOWER_CTRL = 37,
	UNI_EVENT_NAN_TAG_RANGING_CTRL = 38,
	UNI_EVENT_NAN_TAG_DFSP_CSA = 56,
	UNI_EVENT_NAN_TAG_DFSP_CSA_COMPLETE = 57,
	UNI_EVENT_NAN_TAG_DFSP_SUSPEND_RESUME = 58,
	UNI_EVENT_NAN_TAG_REPORT_DW_START = 59,
	UNI_EVENT_NAN_TAG_REPORT_DW_END = 60, /* 60 */
	UNI_EVENT_NAN_TAG_DEVICE_ROLE = 61,
	UNI_EVENT_NAN_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_NAN_DISCOVERY_EVENT {
	uint16_t u2Tag;    // Tag = 0x00
	uint16_t u2Length;
	uint16_t u2SubscribeID;
	uint16_t u2PublishID;
	uint16_t u2Service_info_len;
	uint8_t aucSerive_specificy_info[255];
	uint16_t u2Service_update_indicator;
	uint8_t aucNanAddress[MAC_ADDR_LEN];
	uint8_t ucRange_measurement;
	uint8_t ucFSDType;
	uint8_t ucDataPathParm;
	uint8_t aucSecurityInfo[32];
} __KAL_ATTRIB_PACKED__;
#endif

enum UNI_EVENT_SR_TAG {
	UNI_EVENT_SR_TAG_HW_CAP = 0xC0,
	UNI_EVENT_SR_TAG_HW_IND = 0xC9
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SR_HW_IND {
	uint16_t u2Tag;
	uint16_t u2Length;
	struct WH_SR_IND rSrInd;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SR_HW_CAP {
	/* DW_0 */
	uint16_t u2Tag;
	uint16_t u2Length;
	/* DW_1 - DW_5 */
	struct WH_SR_CAP rSrCap;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SR {
	/*Fixed Fields*/
	uint8_t u1DbdcIdx;
	uint8_t au1Padding[3];
	/*TLV*/
	uint8_t au1TlvBuffer[];/*  the TLVs included in this field: */
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_WOW {
	/*fixed field*/
	uint8_t ucBssIndex;
	uint8_t aucReserved[3];
	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Wake On WLAN event Tag */
enum ENUM_UNI_EVENT_WOW_TAG {
	UNI_EVENT_WOW_TAG_WAKEUP_REASON = 0,
	UNI_EVENT_WOW_TAG_RX_DEAUTH_REASON,
	UNI_EVENT_WOW_TAG_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_WOW_WAKEUP_REASON_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t  ucReason;
	/*   0: MAGIC
	 *   1: BITMAP
	 *   3: GTK_REKEY_FAIL
	 *   8: DISCONNECT
	 *   9: IPV4_UDP
	 *  10: IPV4_TCP
	 *  11: IPV6_UDP
	 *  12: IPV6_TCP
	 *  13: BEACON_LOST
	 *  14: IPV6_ICMP
	 *  15: IPV4_TCP_SYN
	 *  16: IPV6_TCP_SYN
	 *  17: EAPOL_REQUEST
	 *  19: UC2M
	 *  20: ROAMING_INTR
	 *  21: SCHED_SCN_MATCH
	 *  22: ACTION
	 *  23: MC_DATA
	 * 255: UNDEFINED (default init value)
	 */
	uint8_t  ucReserved;
	uint16_t u2WowWakePort;
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_WOW_TAG_RX_DEAUTH_REASON tag(0x01) of
 * UNI_EVENT_WOW Event (0x5B)
 * to send wow wakeup reason.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x01
 * @param[in] u2Length             the length of this TLV
 * @param[in] u2RxDeauthReason     the deauth reason
 */

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_WOW_RX_DEAUTH_REASON_INFO {
	uint16_t   u2Tag;
	uint16_t   u2Length;

	uint16_t   u2RxDeauthReason;
	uint8_t    aucReserved[2];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_CSI
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CSI {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];/**< the TLVs included in this field:
	*   TAG                  | ID  | structure
	*   -------------        | ----| -------------
	*   UNI_EVENT_CSI_DATA   | 0x0 | UNI_EVENT_CSI_DATA_T
	*
	*/
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_CSI_TAG {
	UNI_EVENT_CSI_TAG_DATA = 0,
	UNI_EVENT_CSI_TAG_MAX_NUM
};

/**
 * This structure is used for UNI_EVENT_CSI_DATA tag(0x0) of UNI_EVENT_ID_CSI
 * event (0x4A) to report CSI data. (unsolicited)
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                should be 0x00
 * @param[in] u2Length             the length of this TLV
 * @param[in] aucBuffer            the CSI data
 */
/* CSI data (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_CSI_DATA {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t  aucBuffer[];
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_VOLT_INFO == 1)
struct UNI_EVENT_GET_VOLT_INFO {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

struct UNI_EVENT_GET_VOLT_INFO_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	uint16_t u2Volt;
	uint8_t  aucReserved[2];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_GET_VOLT_INFO_TAG {
	UNI_EVENT_GET_VOLT_INFO_TAG_BASIC = 0x0,
	UNI_EVENT_GET_VOLT_INFO_TAG_NUM
};
#endif /* CFG_VOLT_INFO */

#if (CFG_HW_DETECT_REPORT == 1)
struct UNI_EVENT_HW_DETECT_REPORT {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

struct UNI_EVENT_HW_DETECT_REPORT_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	bool	 fgIsReportNode;
	uint8_t  aucReserved[3];
	uint8_t  aucStrBuffer[HW_DETECT_REPORT_STR_MAX_LEN];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_HW_DETECT_REPORT_TAG {
	UNI_EVENT_HW_DETECT_REPORT_BASIC = 0x0,
	UNI_EVENT_HW_DETECT_REPORT_TAG_NUM
};
#endif /* CFG_HW_DETECT_REPORT */

#if CFG_SUPPORT_PKT_OFLD
struct UNI_EVENT_PKT_OFLD {
	/*Fixed Fields*/
	uint8_t aucPadding[4];
	/*TLV*/
	uint8_t aucTlvBuffer[];/*  the TLVs included in this field: */
} __KAL_ATTRIB_PACKED__;

#endif

#if CFG_SUPPORT_BAR_DELAY_INDICATION
struct UNI_EVENT_DELAY_BAR {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
};

/* Delay Bar event tag */
enum UNI_EVENT_DELAY_BAR_TAG {
	UNI_EVENT_DELAY_BAR_INFO_TAG = 0,
	UNI_EVENT_DELAY_BAR_TAG_NUM
};

struct UNI_STORED_BAR_INFO {
	uint16_t u2SSN;
	uint8_t ucTid;
	uint8_t ucStaRecIdx;
	uint8_t ucStoredBARCount;
	uint8_t aucPadding[3];
};

struct UNI_EVENT_DELAY_BAR_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	uint8_t ucEvtVer;
	uint8_t ucBaNum;
	uint8_t aucPadding[2];
	struct UNI_STORED_BAR_INFO
		arBAR[BAR_DELAY_INDICATION_BA_MAX];
};
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */

#if CFG_FAST_PATH_SUPPORT
/** This structure is used for UNI_EVENT_ID_FAST_PATH event (0x54)
 *
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucReserved      reserved fixed field
 * @param[in] aucTlvBuffer     TLVs
 */
/* Fast Path event */
struct UNI_EVENT_ID_FAST_PATH {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Fast Path event tags */
enum ENUM_UNI_EVENT_FAST_PATH_TAG {
	UNI_EVENT_FAST_PATH_PROCESS  = 0,
	UNI_EVENT_ID_FAST_PATH_MAX_NUM
};

/**
 * This structure is used for UNI_EVENT_FAST_PATH_PROCESS tag(0x00)
 *  of UNI_EVENT_ID_FAST_PATH event (0x54)
 * The event is for send fast path MIC to driver
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag                     Tag id
 * @param[in] u2Length                  The length of this TLV
 * @param[in] u2Mic                     message integrity check
 * @param[in] ucKeynum                  To tell Driver use which key match
 * @param[in] u4KeybitmapMatchStatus    Tell if Keybitmap match success or not
 */
struct UNI_EVENT_FAST_PATH_PROCESS_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2Mic;
	uint8_t ucKeynum;
	uint8_t u4KeybitmapMatchStatus;
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_FAST_PATH_SUPPORT */

#if (CFG_CE_ASSERT_DUMP == 1)
struct UNI_EVENT_ASSERT_DUMP {
	uint8_t ucBssIndex;
	uint8_t aucPadding[3];
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

struct UNI_EVENT_ASSERT_CONTENT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t aucBuf[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_ASSERT_DUMP_TAG {
	UNI_EVENT_ASSERT_DUMP_BASIC = 0x0,
	UNI_EVENT_ASSERT_DUMP_TAG_NUM
};
#endif

struct UNI_CMD_RX_HDR_TRAN_TAG_HANDLE {
	uint32_t u4Size;
};

struct UNI_CMD_RX_HDR_TRAN_BLOCKLIST_ENTRY {
	uint8_t ucBlockListIdx;
	u_int8_t fgEnable;
	uint16_t u2EtherType;
};

struct UNI_CMD_RX_HDR_TRAN_PARM {
	/* UNI_CMD_RX_HDR_TRAN_ENABLE = 0 */
	u_int8_t fgEnable;
	u_int8_t fgCheckBssid;
	uint8_t ucTranslationMode;

	/* UNI_CMD_RX_HDR_TRAN_VLAN_CONFIG = 1 */
	u_int8_t fgInsertVlan;
	u_int8_t fgRemoveVlan;
	u_int8_t fgUseQosTid;

	/* UNI_CMD_RX_HDR_TRAN_BLOCKLIST_CONFIG = 2 */
	uint8_t ucBlockListCnt;
	struct UNI_CMD_RX_HDR_TRAN_BLOCKLIST_ENTRY *list;
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RX_HDR_TRAN {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum UNI_CMD_RX_HDR_TRAN_TAG {
	UNI_CMD_RX_HDR_TRAN_ENABLE = 0,
	UNI_CMD_RX_HDR_TRAN_VLAN_CONFIG = 1,
	UNI_CMD_RX_HDR_TRAN_BLOCKLIST_CONFIG = 2,
	UNI_CMD_RX_HDR_TRAN_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RX_HDR_TRAN_ENABLE {
	uint16_t u2Tag;
	uint16_t u2Length;
	u_int8_t fgEnable;
	u_int8_t fgCheckBssid;
	uint8_t ucTranslationMode;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RX_HDR_TRAN_VLAN {
	uint16_t u2Tag;
	uint16_t u2Length;
	u_int8_t fgInsertVlan;
	u_int8_t fgRemoveVlan;
	u_int8_t fgUseQosTid;
	uint8_t aucPadding[1];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_RX_HDR_TRAN_BLOCKLIST {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBlockListIdx;
	u_int8_t fgEnable;
	uint16_t u2EtherType;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LP_DBG_CTRL {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum UNI_EVENT_LP_DBG_CTRL_TAG {
	UNI_EVENT_LP_DBG_CTRL_TAG_GET_SLP_CNT_INFO = 0x0,
	UNI_EVENT_LP_DBG_CTRL_TAG_KEEP_PWR_CTRL,
	UNI_EVENT_LP_DBG_CTRL_TAG_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LP_GET_SLP_CNT_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t au4LmacSlpCnt[2];
	uint32_t u4WfsysSlpCnt;
	uint32_t u4CbinfraSlpCnt;
	uint32_t u4ChipSlpCnt;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_LP_KEEP_PWR_CTRL {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucKeepPwr;
	uint8_t ucStatus;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PHY_LIST_DUMP {
	/*fixed field*/
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_PHY_LIST_DUMP_TAG {
	UNI_EVENT_PHY_LIST_DUMP_TAG_RESERVED = 0x0,
	UNI_EVENT_PHY_LIST_DUMP_TAG_HW_PHY = 1,
	UNI_EVENT_PHY_LIST_DUMP_TAG_HW_EMLSR = 2,
	UNI_EVENT_PHY_LIST_DUMP_TAG_SW_PHY = 3,
	UNI_EVENT_PHY_LIST_DUMP_TAG_SW_EMLSR = 4,
	UNI_EVENT_PHY_LIST_DUMP_TAG_NUM

};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_PHY_LIST_DUMP_CR {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint32_t u4ListSize;
	uint8_t aucReserved[4];

} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_FW_DROP_SSN
struct UNI_EVENT_FW_DROP_SSN {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
};

/* Delay Bar event tag */
enum UNI_EVENT_FW_DROP_SSN_TAG {
	UNI_EVENT_FW_DROP_SSN_INFO_TAG = 0,
	UNI_EVENT_FW_DROP_SSN_INFO_TAG_NUM
};

struct UNI_EVENT_FW_DROP_SSN_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	uint8_t ucDrpPktNum;
	uint8_t aucPadding[3];
	struct EVENT_STORED_FW_DROP_SSN_INFO
		arSSN[FW_DROP_SSN_MAX];
};
#endif /* CFG_SUPPORT_FW_DROP_SSN */

/** This structure is used for UNI_EVENT_UPDATE_LP event (0x77)
 *
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucReserved      reserved fixed field
 * @param[in] aucTlvBuffer     TLVs
 */
/* Update LP event */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_LP {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Update LP event tags */
enum ENUM_UNI_EVENT_UPDATE_LP_TAG {
	UNI_EVENT_UPDATE_LP_TX_DELAY = 0,
	UNI_EVENT_UPDATE_LP_GEN_SWITCH,
	UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP = 2,
	UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT = 3,
	UNI_EVENT_UPDATE_LP_TAG_NUM
};

/**
 * This structure is used for UNI_EVENT_UPDATE_LP_TX_DELAY tag(0x00)
 * of UNI_EVENT_UPDATE_LP event (0x77) to identify Tx delay status
 *
 * @param[in] u2Tag         Tag id
 * @param[in] u2Length      The length of this TLV
 * @param[in] i4Scen        Scen tput value
 * @param[in] u4Tx          Tx tput value
 * @param[in] u4Rx          Rx tput value
 * @param[in] ucDelay       Tx delay timeout
 * @param[in] u4PktCnt      Tx delay buffer count
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_LP_TX_DELAY_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	int32_t i4Scen;
	uint32_t u4Tx;
	uint32_t u4Rx;
	uint8_t ucDelay;
	uint8_t aucPadding[3];
	uint32_t u4PktCnt;
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP tag(0x02)
 * of UNI_EVENT_UPDATE_LP event (0x77) to identify delayed wakeup info
 *
 * @param[in] u2Tag         Tag id
 * @param[in] u2Length      The length of this TLV
 * @param[in] ucWakeupId    Event type for delayed wakeup
 * @param[in] ucPktCnt      Buffered packet count
 * @param[in] ucDataLen     Length of buffered packet information
 * @param[in] aucPktInfo    Information of buffered packet
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucBssIndex;
	uint8_t ucWakeupId;
	uint8_t ucPktCnt;
	uint8_t ucDataLen;
	uint8_t aucPadding[2];
	uint8_t aucPktInfo[DELAY_WAKEUP_PKT_INFO_LEN];
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT tag(0x03)
 * of UNI_EVENT_UPDATE_LP event (0x77) to identify leakdy AP detection statues
 *
 * @param[in] u2Tag         Tag id
 * @param[in] u2Length      The length of this TLV
 * @param[in] ucDectionType Leaky AP Detection Type
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT_T {
uint16_t     u2Tag;
uint16_t     u2Length;
uint8_t      ucDectionType;
uint8_t      aucPadding[3];
} __KAL_ATTRIB_PACKED__;


#if (CFG_SUPPORT_FACT_CAL == 1)
/** This structure is used for UNI_EVENT_ID_FACT_CAL event
 * (0x7C)(unsolicited) to report fact data to host
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucReserved      reserved fixed field
 * @param[in] aucTlvBuffer     TLVs
 */
/* Factory Calibration event */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_FACT_CAL {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Update Factory Calibration event tags */
enum UNI_EVENT_FACT_CAL_TAG {
	UNI_EVENT_FACT_CAL_GET_DATA_TAG       = 0x0,   // For cx driver used
	UNI_EVENT_FACT_CAL_RAPID_GET_DATA_TAG = 0x1,   // For SDK driver used
	UNI_EVENT_FACT_CAL_TAG_MAX_TAG_NUM
};

/**
 * This structure is used for send RAPID_GET/RAPID_SET cmd data
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FACT_CAL_DATA {
	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint32_t u4Channel;
	uint8_t ucCalType;
	uint8_t ucDone;
	uint8_t ucBand;
	uint8_t u1Rsvd;
	uint8_t aucBufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_FACT_CAL_RAPID_GET_DATA_TAG tag(0x01)
 of UNI_EVENT_ID_FACT_CAL event (0x7C) to report fact data to host
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag            should be 0x00
 * @param[in] u2Length         the length of this TLV
 * @param[in] u4Data           fact data
 * @param[in] u4SeqNum         seq num
 * @param[in] u4BufDataLength  buffer length
 * @param[in] u1CalType        calbration type
 * @param[in] u1Done           done
 * @param[in] u1Rsvd           rsvd
 * @param[in] u1BufData        cal data
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_FACT_CAL_RAPID_GET_DATA {
	uint16_t u2Tag;    // Tag = 0x01
	uint16_t u2Length;

	uint32_t u4Data;
	uint32_t u4SeqNum;
	uint32_t u4BufDataLength;
	uint8_t ucCalType;
	uint8_t ucDone;
	uint8_t ucSubDone;
	uint8_t ucBand;
	uint8_t ucChannel;
	uint8_t rvsd[3];
	uint8_t aucBufData[FACT_CAL_DATA_BUF_MAXSIZE];
} __KAL_ATTRIB_PACKED__;
#endif /* CFG_SUPPORT_FACT_CAL */

/* Update MLO event tags */
enum ENUM_UNI_EVENT_MLO {
	UNI_EVENT_MLD_MLSR_CONCURRENT_DONE = 0x7,
	UNI_EVENT_UPDATE_MLO_NUM
};

/**
 * This structure is used for UNI_CMD_MLO_TAG_MLD_MLSR_CONCURRENT_PRECONNECT
 * tag(0x07) of UNI_EVENT_ID_MLO event (0x59) to response driver MLSR ready.
 *
 * @param[in] u2Tag         Tag id
 * @param[in] u2Length      The length of this TLV
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLSR_CONCURRENT_PRECONNECT {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

/** This structure is used for UNI_EVENT_ID_OMI event (0x84)
 * to do OMI's related synchronization with Driver.
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] aucPadding        aucPadding
 * @param[in] aucTlvBuffer      TLVs
 */
/* Update OMI event */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_OMI {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Update LP event tags */
enum ENUM_UNI_EVENT_UPDATE_OMI_TAG {
	UNI_EVENT_NOTIFY_OMI_RX = 0,
	UNI_EVENT_OMI_MAX_NUM
};

/**
 * This structure is used for UNI_EVENT_NOTIFY_OMI_RX(0x00)
 * of UNI_EVENT_ID_OMI event (0x84)
 * to Notify Driver Do Channel Selection
 * @version Supported from ver:1.0.0.0
 *
 * @param[in] u2Tag
 *	should be 0x00
 * @param[in] u2Length
 *	the length of this TLV, sizeof(UNI_EVENT_NOTIFY_OMI_RX_T)
 * @param[in] u2StaRecIndex
 *	index of StaRec that receive this OMI.
 * @param[in] ucRxNss
 *	Parsed RxNSS from OMI.
 * @param[in] ucTxNss
 *	Parsed TxNSS from OMI. (STA Should Ignore this parameter.)
 * @param[in] ucBW
 *	Parsed BW from OMI. Type:
 *		typedef enum _WH_ENUM_PHY_BW
 *		{
 *			WH_PHY_BW_20 = 0,
 *			WH_PHY_BW_40,
 *			WH_PHY_BW_80,
 *			WH_PHY_BW_160,
 *			WH_PHY_BW_10,
 *			WH_PHY_BW_5,
 *			WH_PHY_BW_8080,
 *			WH_PHY_BW_320,
 *			WH_PHY_BW_NUM
 *		} WH_ENUM_PHY_BW, *P_WH_ENUM_PHY_BW;
 *
 * @param[in] fgDisMuUL
 * @param[in] fgDisMuULData
 * @param[in] ucRxNssExt
 * @param[in] ucBWExt
 * @param[in] ucTxNstsExt

 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_NOTIFY_OMI_RX_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint16_t u2StaRecIndex;
	uint8_t ucRxNss;
	uint8_t ucTxNsts;
	uint8_t ucBW;
	uint8_t fgDisMuUL;
	uint8_t fgDisMuULData;
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucRxNssExt;
	uint8_t ucBWExt;
	uint8_t ucTxNstsExt;
	uint8_t ucReserved[2];
#else
	uint8_t ucReserved[5];
#endif
} __KAL_ATTRIB_PACKED__;

/**
 * This structure is used for UNI_EVENT_UPDATE_LP_GEN_SWITCH tag(0x01)
 * of UNI_EVENT_UPDATE_LP event (0x77) to identify Tx delay status
 *
 * @param[in] u2Tag         Tag id
 * @param[in] u2Length      The length of this TLV
 * @param[in] ucGenSwitchStatus        Gen Switch Status
 */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_UPDATE_LP_GEN_SWITCH_T {
	uint16_t     u2Tag;
	uint16_t     u2Length;
	uint8_t      ucGenSwitchStatus;
	uint8_t      aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UPDATE_LP {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* Gen Switch command TLV List */
enum ENUM_UNI_CMD_UPDATE_LP_TAG {
	UNI_CMD_UPDATE_LP_TAG_DYN_QOS_PARAM = 0,
	UNI_CMD_UPDATE_LP_TAG_GEN_SWITCH_PARAM,
	UNI_CMD_UPDATE_LP_TAG_PCIE_PARAM,
	UNI_CMD_UPDATE_LP_TAG_NUM
};

/* Set gen switch parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UPDATE_LP_GEN_SWITCH_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucPcieTransitionStatus;
	uint8_t aucPadding[3];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UPDATE_PCIE {
	/* fixed field */
	uint8_t ucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *
	 *   TAG                             | ID  | structure
	 *   -------------                   | ----| -------------
	 *   UNI_CMD_UPDATE_LP_TAG_PCIE_PARAM| 0x0 | UNI_CMD_UPDATE_PCIE_PARAM
	 */
} __KAL_ATTRIB_PACKED__;

/* Set gen switch parameters (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_UPDATE_PCIE_PARAM {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucGen;
	uint8_t ucLane;
	uint8_t aucPadding[2];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLC {
	/* fixed field */
	uint8_t ucGroupMldId;
	uint8_t ucTokenID;
	uint8_t aucReserved[2];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_MLC_TAG {
	UNI_EVENT_MLC_TAG_RESP  = 0,
	UNI_EVENT_MLC_TAG_QUERY  = 1,
	UNI_EVENT_MLC_TAG_NUM
};

enum ENUM_MLC_RESP_STATUS {
	MLC_RESP_STATUS_SUCCESS = 0,
	MLC_RESP_STATUS_ERROR = 1,
	MLC_RESP_STATUS_INVALID_DATA = 2,
	MLC_RESP_STATUS_BUSY = 3,
	MLC_RESP_STATUS_NOT_SUPPORTED = 4,
	MLC_RESP_STATUS_NOT_ACCEPTED = 5,
};

/* MLC response (Tag0) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLC_RESP {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucMlcMode;
	uint8_t aucReserved[3];
	uint32_t u4Status;
} __KAL_ATTRIB_PACKED__;

/* MLC query link (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLC_QUERY_LINK_STATE {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint8_t ucMlcMode;
	uint8_t ucLinkNum;
	uint8_t aucReserved[6];
	uint8_t aucLinkInfo[];
} __KAL_ATTRIB_PACKED__;

/* MLC query info (Tag1) */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MLC_QUERY_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;

	uint32_t u4AvailablePlanBmap;
	uint8_t ucCurrentPlan;
	uint8_t ucCurrentUser;
	uint8_t aucReserved[2];
} __KAL_ATTRIB_PACKED__;


#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BT_CTRL {
	/* fixed field */
	uint8_t aucReserved[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* BT control event Tag */
enum UNI_EVENT_BT_CTRL_TAG {
	UNI_EVENT_BT_CTRL_GET_COREDUMP_HEAD = 0,
	UNI_EVENT_BT_CTRL_GET_COREDUMP_DATA = 1,
	UNI_EVENT_BT_CTRL_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BT_CTRL_HEAD {
	uint16_t   u2Tag;    /* Tag = 0x00 */
	uint16_t   u2Length;
	/* coredump header */
	uint32_t addr;
	uint32_t length;
	uint32_t round;
	uint32_t dumpLeave;
	uint32_t currentRound;
	uint32_t done;
	uint32_t reserved[20];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_BT_CTRL_DATA_T {
	uint16_t   u2Tag;    // Tag = 0x01
	uint16_t   u2Length;
} __KAL_ATTRIB_PACKED__;

#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_MBRAIN {
	/* fixed field */
	uint8_t aucPadding[4];
	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_MBRAIN_TAG {
	UNI_EVENT_MBRAAIN_TAG_TXPWR_RPT,
	UNI_EVENT_MBRAAIN_TAG_NUM
};

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
struct UNI_EVENT_TXPWR_MBRAIN_INFO {
	uint16_t u2Tag;
	uint16_t u2Length;
	/* event body */
	struct TXPWR_MBRAIN_RPT_T rMbrRpt;
} __KAL_ATTRIB_PACKED__;
#endif

#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SHR_ANT_SWCH {
	/* fixed field */
	uint8_t u1Reserved[4];

	/* tlv */
	uint8_t au1TlvBuffer[];

	/**< the TLVs included in this field:
	 *
	 * TAG                          | ID  | structure
	 * -----------------------------|-----|--------------
	 * UNI_EVENT_SHR_ANT_SWCH_UPDATE| 0x0 | UNI_EVENT_SHR_ANT_SWCH_UPDATE_T
	 */
} __KAL_ATTRIB_PACKED__;

/** shared antenna switch command TLV List */
__KAL_ATTRIB_PACKED_FRONT__
enum ENUM_UNI_EVENT_SHR_ANT_SWCH_TAG {
	UNI_EVENT_SHR_ANT_SWCH_UPDATE = 0,
	UNI_EVENT_SHR_ANT_SWCH_TAG_NUM
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_SHR_ANT_SWCH_UPDATE_T {
	uint16_t         u2Tag;
	uint16_t         u2Length;
	uint8_t          u1Grant;
	uint8_t          u1Reserved[3];
} __KAL_ATTRIB_PACKED__;
#endif

struct UNI_EVENT_ID_COEX_ICER {
	/* fixed field */
	uint8_t aucPadding[4];

	/* tlv */
	uint8_t aucTlvBuffer[];
	/**< the TLVs included in this field:
	 *   TAG                             | ID  | structure
	 *   -------------                   | ----| -------------
	 *   UNI_EVENT_COEX_ICER_DUMP   | 0x0 | UNI_EVENT_COEX_ICER_DUMP_T
	 *
	 */
} __KAL_ATTRIB_PACKED__;

enum ENUM_UNI_EVENT_COEX_ICER_TAG {
	UNI_EVENT_COEX_ICER_DUMP = 0,
	UNI_EVENT_COEX_ICER_MAX_NUM
};

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_EVENT_COEX_ICER_DUMP_T {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucVersion;
	uint8_t aucReserved1[3];    /* 4 byte alignment */
	uint32_t u4WifiSysTimestamp;
	uint32_t u4BaseAddress;
	uint32_t u4DataLen;
	uint32_t au4Reserved2[2];
} __KAL_ATTRIB_PACKED__;

#if CFG_SUPPORT_FIPS
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FIPS {
	/* fixed field */
	uint8_t ucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[];
} __KAL_ATTRIB_PACKED__;

/* tag struct */
__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FIPS_TC_TAG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucFragNum;
	uint8_t ucFragTotal;
	uint16_t u2TCTotalLen;
	uint16_t u2TCBufferLen;
	uint8_t aucReserved[2];
	uint8_t aucTCBuffer[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FIPS_GET_STATUS_TAG {
	uint16_t u2Tag;
	uint16_t u2Length;
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct UNI_CMD_FIPS_GET_RESULT_TAG {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucFragNum;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__;

/* tag enum */
enum ENUM_UNI_CMD_FIPS_TAG {
	UNI_CMD_FIPS_TC = 0,
	UNI_CMD_FIPS_GET_STATUS,
	UNI_CMD_FIPS_GET_RESULT
};

struct UNI_EVENT_ID_FIPS {
	/* fixed field */
	uint8_t aucReserved[4];
	/* tlv */
	uint8_t aucTlvBuffer[];
};

struct UNI_EVENT_FIPS_STATUS {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucStatus;
	uint8_t ucFragTotal;
	uint8_t aucReserved[2];
};

enum ENUM_FIPS_STATUS {
	FIPS_STATUS_WAITING = 0,
	FIPS_STATUS_RECEIVING,
	FIPS_STATUS_RECEIVED,
	FIPS_STATUS_SEC_START,
	FIPS_STATUS_SEC_FINISH,
	FIPS_STATUS_MISSING,
	FIPS_STATUS_FAIL,
};

struct UNI_EVENT_FIPS_RESULT {
	uint16_t u2Tag;
	uint16_t u2Length;
	uint8_t ucFragNum;
	uint8_t ucFragTotal;
	uint16_t u2TRTotalLen;
	uint16_t u2TRBufferLen;
	uint8_t aucReserved[2];
	uint8_t aucTRBuffer[];
};

enum UNI_EVENT_ID_FIPS_TAG {
	UNI_EVENT_FIPS_STATUS = 0,
	UNI_EVENT_FIPS_RESULT,
	UNI_EVENT_FIPS_MAX_NUM
};
#endif /* CFG_SUPPORT_FIPS */

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

#define NIC_FILL_UNI_CMD_TX_HDR(__prAd, __pucInfoBuffer, __u2InfoBufLen, \
	__u2CID, __ucPktTypeID, __pucSeqNum, __ppCmdBuf, \
	__ucS2DIndex, __ucOption) \
{ \
	struct mt66xx_chip_info *__prChipInfo; \
	struct WIFI_UNI_CMD_INFO __wifi_cmd_info; \
	__prChipInfo = __prAd->chip_info; \
	__wifi_cmd_info.pucInfoBuffer = __pucInfoBuffer; \
	__wifi_cmd_info.u2InfoBufLen = __u2InfoBufLen; \
	__wifi_cmd_info.u2CID = __u2CID; \
	__wifi_cmd_info.ucPktTypeID = __ucPktTypeID; \
	__wifi_cmd_info.ucS2DIndex = __ucS2DIndex; \
	__wifi_cmd_info.ucOption = __ucOption; \
	ASSERT(__prChipInfo->asicFillUniCmdTxd); \
	__prChipInfo->asicFillUniCmdTxd(__prAd, &(__wifi_cmd_info), \
		__pucSeqNum, (void **)__ppCmdBuf); \
}

#define GET_UNI_CMD_DATA_LEN(_cmd) \
	(((struct CMD_INFO *)(_cmd))->u4SetInfoLen)

#define GET_UNI_CMD_DATA(_cmd) \
	(((struct CMD_INFO *)(_cmd))->pucSetInfoBuffer)

#define GET_UNI_EVENT_DATA_LEN(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->u2PacketLength - \
				sizeof(struct WIFI_UNI_EVENT))

#define GET_UNI_EVENT_DATA(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->aucBuffer)

#define GET_UNI_EVENT_SEQ(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->ucSeqNum)

#define GET_UNI_EVENT_ID(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->ucEID)

#define GET_UNI_EVENT_EXT_ID(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->ucExtenEID)

#define GET_UNI_EVENT_OPTION(_evt) \
	(((struct WIFI_UNI_EVENT *)(_evt))->ucOption)

#define IS_UNI_EVENT(_evt) \
	((GET_UNI_EVENT_OPTION(_evt) & UNI_CMD_OPT_BIT_1_UNI_EVENT) > 0 ? \
	TRUE : FALSE)

#define IS_UNI_UNSOLICIT_EVENT(_evt) IS_UNI_EVENT(_evt) && \
	((GET_UNI_EVENT_OPTION(_evt) & UNI_CMD_OPT_BIT_2_UNSOLICIT_EVENT) > 0 \
	? TRUE : FALSE)


#define TAG_ID(fp)	(((struct TAG_HDR *) fp)->u2Tag)
#define TAG_LEN(fp)	(((struct TAG_HDR *) fp)->u2Length)
#define TAG_DATA(fp)	(((struct TAG_HDR *) fp)->aucBuffer)
#define TAG_HDR_LEN	sizeof(struct TAG_HDR)

/**
 * In case the length field carries incorrect value breacking the TLV stride,
 * the caller shall check the for-each exit result by checking the final value
 * of _u2Offset against _u2TlvBufLen as in nicUniCmdEventQueryNicCapabilityV2().
 */
#define TAG_FOR_EACH(_pucTlvBuf, _u2TlvBufLen, _u2Offset) \
for ((_u2Offset) = 0U;	\
	((((_u2Offset) + TAG_HDR_LEN) <= (_u2TlvBufLen)) && \
	 (TAG_LEN(_pucTlvBuf) >= TAG_HDR_LEN) && \
	 (((_u2Offset) + TAG_LEN(_pucTlvBuf)) <= (_u2TlvBufLen))); \
	(_u2Offset) += TAG_LEN(_pucTlvBuf), (_pucTlvBuf) += TAG_LEN(_pucTlvBuf))


/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

uint32_t wlanSendSetQueryCmdHelper(struct ADAPTER *prAdapter,
		    uint8_t ucCID,
		    uint8_t ucExtCID,
		    u_int8_t fgSetQuery,
		    u_int8_t fgNeedResp,
		    u_int8_t fgIsOid,
		    PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
		    PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
		    uint32_t u4SetQueryInfoLen,
		    uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
		    uint32_t u4SetQueryBufferLen,
		    enum EUNM_CMD_SEND_METHOD eMethod);

uint32_t wlanSendSetQueryUniCmd(struct ADAPTER *prAdapter,
			uint8_t ucUCID,
			u_int8_t fgSetQuery,
			u_int8_t fgNeedResp,
			u_int8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
			uint32_t u4SetQueryInfoLen,
			uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
			uint32_t u4SetQueryBufferLen);

uint32_t wlanSendSetQueryUniCmdAdv(struct ADAPTER *prAdapter,
			uint8_t ucUCID,
			u_int8_t fgSetQuery,
			u_int8_t fgNeedResp,
			u_int8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
			uint32_t u4SetQueryInfoLen,
			uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
			uint32_t u4SetQueryBufferLen,
			enum EUNM_CMD_SEND_METHOD eMethod);

/*******************************************************************************
 *                   Command
 *******************************************************************************
 */

uint32_t nicUniCmdExtCommon(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdNotSupport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdScanReqV2(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdScanCancel(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSchedScanEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSchedScanReq(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdBssActivateCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdCustomerCfg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdChipCfg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSwDbgCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetRxFilter(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetMbmc(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdPowerCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetDomain(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdRemoveStaRec(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdBcnContent(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdFilsDiscovery(struct ADAPTER *ad,
		uint8_t bss_idx, uint32_t max_interval,
		uint32_t min_interval, uint8_t *ie, uint16_t ie_len);
uint32_t nicUniCmdPmDisable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdPmEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetBssInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetBssRlm(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if (CFG_SUPPORT_802_11BE_MLO == 1) && (CFG_MLD_INFO_PRESETUP == 1)
uint32_t nicUniCmdSetBssMld(struct ADAPTER *ad,
		struct BSS_INFO *prBssInfo);
uint32_t nicUniCmdSetStarecMld(struct ADAPTER *ad,
		struct STA_RECORD *prStaRec);
#endif
uint32_t nicUniCmdBcnProt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdUpdateStaRec(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdChPrivilege(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdCnmGetInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdWsysFwLogUI(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdWsysFwLog2Host(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdWsysFwBasicConfig(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetSuspendMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetWOWLAN(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdPowerSaveMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetWmmPsTestParams(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetUapsd(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdTwtArgtUpdate(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdStaRecUpdateExt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if CFG_SUPPORT_TX_BF
uint32_t nicUniCmdBFAction(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
uint32_t nicUniCmdSerAction(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if (CFG_SUPPORT_TWT == 1)
uint32_t nicUniCmdGetTsf(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
uint32_t nicUniCmdTwtStaGetCnmGranted(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
uint32_t nicUniUpdateDevInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdUpdateEdcaSet(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdAccessReg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
void nicUniCmdEventSetPhyCtrl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
uint32_t nicUniCmdUpdateMuEdca(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdUpdateSrParams(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdOffloadIPV4(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdOffloadIPV6(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetIdcChnl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if CFG_SUPPORT_IDC_RIL_BRIDGE
uint32_t nicUniCmdSetIdcRilBridge(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
#if CFG_SUPPORT_UWB_COEX
uint32_t nicUniCmdSetUwbCoexEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetUwbCoexPrepare(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
uint32_t nicUniCmdSetSGParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetMonitor(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if CFG_SUPPORT_ROAMING
uint32_t nicUniCmdRoaming(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
uint32_t nicUniCmdPerfInd(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdInstallKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdInstallDefaultKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdOffloadKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdHifCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdRddOnOffCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdTdls(struct ADAPTER *ad,
                struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetMdvt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetP2pNoa(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetP2pOppps(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetP2pLoStart(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetP2pLoStop(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetStaStatistics(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetStatistics(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetLinkQuality(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdBeaconReport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetLinkStats(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdGetBugReport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdMldStaTeardown(struct ADAPTER *ad,
		struct STA_RECORD *prStaRec);
uint32_t UniCmdSetRecSecPnInfo(struct ADAPTER *ad,
	struct PARAM_TX_TSC_INFO *tsc);
uint32_t nicUniCmdSetApConstraintPwrLimit(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetRrmCapability(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetCountryPwrLimit(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetCountryPwrLimitPerRate(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetNvramSettings(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t wlanResetTxScrambleSeed(struct ADAPTER *ad,
		uint8_t ucHwBandIdx);
#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
uint32_t nicUniCmdTestmodeListmode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif /* (CONFIG_WLAN_SERVICE == 1) */
#endif
uint32_t nicUniCmdTestmodeCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if CFG_SUPPORT_XONVRAM
uint32_t nicUniCmdTestmodeXOCal(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
#endif /* CFG_SUPPORT_XONVRAM */
#if CFG_SUPPORT_PLCAL
uint32_t nicUniCmdTestmodePlCal(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
#endif /* CFG_SUPPORT_PLCAL */
#if CFG_SUPPORT_QA_TOOL
uint32_t nicUniExtCmdTestmodeCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdTestmodeRxStat(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdTestmodeRxStatTlv(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);

#endif

#if (CFG_SUPPORT_NAN == 1)
int32_t nanGetSubCmdId(uint32_t tag, uint16_t *u2CmdTag);
#endif

uint32_t nicUniCmdSetTxAmpdu(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetRxAmpdu(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetMultiAddr(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);

uint32_t nicUniCmdSetCoexStopConnProtect(struct ADAPTER *ad, uint8_t ucBssIdx);

uint32_t nicUniCmdSetRssiMonitor(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetIcsSniffer(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetPhyIcsEventOn(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetPhyIcsStart(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdTxPowerCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdThermalProtect(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdEfuseAccess(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdEfuseFreeBlock(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdEfuseBufferMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdNan(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdFwLogQueryBase(struct ADAPTER *ad,
	uint32_t *addr);
uint32_t nicUniCmdFwLogUpdateRead(struct ADAPTER *ad,
	enum FW_LOG_CMD_CTRL_TYPE type,
	uint32_t addr);
uint32_t nicUniCmdSR(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdACLPolicy(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdMibInfo(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdQueryThermalAdieTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
uint32_t nicUniCmdQueryThermalDdieTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
uint32_t nicUniCmdQueryThermalAdcTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
void nicUniEventEfuseAccess(struct ADAPTER	*prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);

uint32_t nicUniCmdSetCsiControl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if (CFG_VOLT_INFO == 1)
uint32_t nicUniCmdSendVnf(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
#if CFG_SUPPORT_WIFI_POWER_METRICS
uint32_t nicUniCmdPowerMetricsStatSetParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
uint32_t nicUniCmdFastPath(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);

#if CFG_SUPPORT_PKT_OFLD
uint32_t nicUniCmdPktOfldOp(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif

uint32_t nicUniCmdKeepAlive(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
uint32_t nicUniCmdMdnsRecorde(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
#endif
uint32_t nicUniCmdLpDbgCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);

#if CFG_SUPPORT_WIFI_ICCM
uint32_t nicUniCmdIccmSetParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif

uint32_t nicUniCmdGamingMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t nicUniCmdQueryEmlInfo(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen);
#if (CFG_SUPPORT_MLC == 1)
uint32_t nicUniCmdSendMlcRequest(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	struct PARAM_MLC_REQ *prMlcReq);
uint32_t nicUniCmdSendMlcQueryLinkState(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvQueryBuffer, uint32_t u4QueryBufferLen);
uint32_t nicUniCmdSendMlcQueryInfo(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvQueryBuffer, uint32_t u4QueryBufferLen);
#endif /* CFG_SUPPORT_MLC */
#endif /* CFG_SUPPORT_802_11BE_MLO */

uint32_t nicUniCmdSetSapRps(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdSetSapSus(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
uint32_t nicUniCmdSapBcnCriUpd(struct ADAPTER *ad, uint8_t idx);
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */

#if CFG_SUPPORT_RTT
uint32_t nicUniCmdRttGetCapabilities(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdRttRangeRequest(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdRttInstallLtfKeyseed(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif

#if CFG_SUPPORT_TPUT_FACTOR
uint32_t nicUniCmdGetTputFactor(struct ADAPTER *ad, uint32_t u4WtblFlag);
#endif

#if CFG_SUPPORT_WED_PROXY
uint32_t nicUniCmdStaRecConnType(struct ADAPTER *ad,
		uint32_t legacy_sta_type);
#endif

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
uint32_t nicUniCmdPowerLimitEmiInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t nicUniCmdUpdateLowPowerParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
uint32_t nicUniCmdUpdatePcieParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif /*CFG_EAP_PCIE_GEN_SWITCH*/

#if (CFG_SUPPORT_TSF_SYNC == 1)
uint32_t nicUniCmdUpdateTsfSyncParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif

uint32_t nicUniCmdPpEnCtrl(struct ADAPTER *ad, uint8_t ucMode,
			   uint8_t ucDbdxIdx, uint8_t ucCtrl,
			   uint8_t ucMgmtEn, uint16_t u2Bitmap,
			   u_int8_t fgIsOid);

uint32_t nicUniCmdPpAlgoCtrl(struct ADAPTER *ad,
			     struct UNI_CMD_PP_ALG_CTRL *para,
			     u_int8_t fgIsOid);

#if (CFG_SUPPORT_FACT_CAL == 1)
uint32_t nicUniCmdFactCal(struct ADAPTER *prAdapter,
		uint32_t u4Action,
		struct UNI_CMD_FACT_CAL_DATA *prCalData);
#endif

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
uint32_t nicUniCmdBtCtrl(struct ADAPTER *prAdapter,
			    struct WIFI_UNI_SETQUERY_INFO *prInfo);
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

#if (CFG_SUPPORT_ML_RECONFIG == 1)
uint32_t nicUniCmdUpdateMldRecfgState(struct ADAPTER *ad,
		struct MLD_STA_RECORD *prMldStaRec);
#endif /* CFG_SUPPORT_ML_RECONFIG */

#if CFG_SUPPORT_FIPS
uint32_t nicUniCmdFipsTc(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdFipsGetStatus(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
uint32_t nicUniCmdFipsGetResult(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info);
#endif
/*******************************************************************************
 *                   Event
 *******************************************************************************
 */

void nicRxProcessUniEventPacket(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb);
void nicUniCmdEventSetCommon(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);
void nicUniCmdTimeoutCommon(struct ADAPTER *prAdapter,
			    struct CMD_INFO *prCmdInfo);

/*******************************************************************************
 *                   Solicited Event
 *******************************************************************************
 */

void nicUniCmdEventQueryCfgRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventQueryChipConfig(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventQuerySwDbgCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniCmdStaRecHandleEventPkt(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);
void nicUniEventQueryIdcChnl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf);
#if CFG_SUPPORT_TX_BF
void nicUniEventBFStaRec(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif
void nicUniCmdEventQueryMcrRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if (CFG_SUPPORT_TWT == 1)
void nicUniCmdEventGetTsfDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
void nicUniCmdEventTWTGetCnmGrantedDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

void nicUniCmdEventInstallKey(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventQueryCnmInfo(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventStaStatistics(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventStatistics(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventLinkQuality(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicUniSolicitEventBaOffload(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
void nicCollectRegStatFromEmi(struct ADAPTER *prAdapter);
#endif
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
void nicUniEventAllStatsOneCmd(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif
void nicUniEventQueryRfTestATInfo(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#if CFG_SUPPORT_XONVRAM
void nicUniEventRfTestXoCal(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
#endif /* CFG_SUPPORT_XONVRAM */
#if CFG_SUPPORT_PLCAL
void nicUniEventRfTestPlCal(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
#endif /* CFG_SUPPORT_PLCAL */
void nicUniEventQueryRxStatAll(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventQueryRxStatAllCon3(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventQueryRxStatInfoTlv(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventBugReport(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventLinkStats(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventRfTestHandler(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniCmdEventTxPowerInfo(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventEfuseControl(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventEfuseAccess(struct ADAPTER	*prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventEfuseFreeBlock(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventFwLogQueryBase(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
void nicUniCmdEventQueryMldRec(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventThermalAdieTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
void nicUniEventThermalDdieTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
void nicUniEventThermalAdcTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
void nicUniEventMibInfo(struct ADAPTER *ad,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if CFG_SUPPORT_PKT_OFLD
void nicUniEventQueryOfldInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif
void nicUniCmdEventLpDbgCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void nicUniEventEmlInfo(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event);
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
void nicUniEventMLSRSwitchDone(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
#if (CFG_SUPPORT_MLC == 1)
void nicUniEventMlcReqDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventMlcQueryLinkStateDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicUniEventMlcQueryInfoDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_MLC */

#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
void nicUniCmdEventGetMlChnlCond(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */
#endif /* CFG_SUPPORT_802_11BE_MLO */
#if CFG_SUPPORT_RTT
void nicUniEventRttCapabilities(struct ADAPTER	*prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_FACT_CAL == 1)
void nicUniEventGetFactCalData(struct ADAPTER *prAdapter,
	struct WIFI_UNI_EVENT *uni_evt);
#endif
/*******************************************************************************
 *                   Unsolicited Event
 *******************************************************************************
 */

void nicUniEventScanDone(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventChMngrHandleChEvent(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventMbmcHandleEvent(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventStatusToHost(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventBaOffload(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventSleepNotify(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventPhyIcsRawData(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventBeaconTimeout(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventUpdateCoex(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventIdc(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventBssIsAbsence(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_ENABLE_WIFI_DIRECT
void nicUniEventPsSync(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventSap(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventOBSS(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_SUPPORT_ROAMING
void nicUniEventRoaming(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventAddKeyDone(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventPpCb(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventFwLog2Host(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_ENABLE_WIFI_DIRECT
void nicUniEventP2p(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if (CFG_SUPPORT_DFS_MASTER == 1)
void nicUniEventRDD(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
static uint32_t MT_ATEInsertRDD(
	struct _ATE_LOG_DUMP_ENTRY *entry,
	uint8_t *data, uint32_t len);
uint32_t MT_ATEInsertLog(
	struct ADAPTER *prAdapter,
	uint8_t *log, uint32_t len);
#endif
void nicUniEventCountdown(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventStaRec(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_SUPPORT_TDLS
void nicUniEventTdls(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventBssER(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventRssiMonitor(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventHifCtrl(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventNan(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_SUPPORT_TX_BF
void nicUniEventBF(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventSR(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if CFG_WOW_SUPPORT
void nicUniEventWow(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventCsiData(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniUnsolicitStatsEvt(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if (CFG_VOLT_INFO == 1)
void nicUniEventGetVnf(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
#if CFG_SUPPORT_WIFI_POWER_METRICS
void nicUniEventPowerMetricsStatGetInfo(struct ADAPTER *ad,
		struct WIFI_UNI_EVENT *evt);
#endif
#if CFG_SUPPORT_BAR_DELAY_INDICATION
void nicUniEventDelayBar(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
void nicUniEventFastPath(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventThermalProtect(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#if (CFG_HW_DETECT_REPORT == 1)
void nicUniEventHwDetectReport(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif /* CFG_HW_DETECT_REPORT */
#if CFG_SUPPORT_FW_DROP_SSN
void nicUniEventFwDropSSN(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif /* CFG_SUPPORT_FW_DROP_SSN */
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
void nicUniEventMdnsStats(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif /* CFG_SUPPORT_MDNS_OFFLOAD */
#endif
#if CFG_SUPPORT_RTT
void nicUniEventRtt(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif
#if (CFG_CE_ASSERT_DUMP == 1)
void nicUniEventAssertDump(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventUpdateLp(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
void nicUniEventTxPower(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);

uint32_t nicUniCmdRxHdrTransUpdate(struct ADAPTER *ad,
	struct UNI_CMD_RX_HDR_TRAN_PARM *param);
#if CFG_MTK_MDDP_SUPPORT
void nicUniEventMddp(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt);
#endif
#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
void nicUniEventShrAntSwch(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt);
#endif
void nicUniEventOmi(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);

#if CFG_SUPPORT_FIPS
void nicUniEventFips(struct ADAPTER *prAdapter, struct CMD_INFO *prCmdInfo,
		     uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
void nicUniCmdEventQueryBtCtrl(struct ADAPTER *prAdapter,
			    struct WIFI_UNI_EVENT *prEvt);
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

#if CFG_SUPPORT_MBRAIN
void nicUniUnsolicitMbrEvt(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);
#endif

void nicUniEventIcerRawData(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */


#endif /* _NIC_CMD_EVENT_H */
