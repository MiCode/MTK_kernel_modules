/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nic_cmd_event.h"
 *  \brief This file contains the declairation file of the WLAN OID processing
 *	 routines of Windows driver for MediaTek Inc.
 *   802.11 Wireless LAN Adapters.
 */

#ifndef _NIC_CMD_EVENT_H
#define _NIC_CMD_EVENT_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_vendor.h"

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
#include "rlm_txpwr_limit.h"
#endif

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_ie.h"
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#include "eht_ie.h"
#endif

#include "wsys_cmd_handler_fw.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CMD_PQ_ID           (0x8000)
#define CMD_PACKET_TYPE_ID  (0xA0)

#define PKT_FT_CMD			0x2

#define CMD_STATUS_SUCCESS      0
#define CMD_STATUS_REJECTED     1
#define CMD_STATUS_UNKNOWN      2

/* Action field in structure CMD_CH_PRIVILEGE_T */
#define CMD_CH_ACTION_REQ           0
#define CMD_CH_ACTION_ABORT         1

/* Status field in structure EVENT_CH_PRIVILEGE_T */
#define EVENT_CH_STATUS_GRANT       0

/*CMD_POWER_OFFSET_T , follow 5G sub-band*/
/* #define MAX_SUBBAND_NUM             8 */
/*  */
/*  */
/*  */
/*  */
#define S2D_INDEX_CMD_H2N		0x0
#define S2D_INDEX_CMD_C2N		0x1
#define S2D_INDEX_CMD_H2C		0x2
#define S2D_INDEX_CMD_H2N_H2C	0x3

#define S2D_INDEX_EVENT_N2H		0x0
#define S2D_INDEX_EVENT_N2C		0x1
#define S2D_INDEX_EVENT_C2H		0x2
#define S2D_INDEX_EVENT_N2H_N2C	0x3

#define RDD_EVENT_HDR_SIZE              20
#define RDD_ONEPLUSE_SIZE               8 /* size of one pulse is 8 bytes */
#define RDD_PULSE_OFFSET0               0
#define RDD_PULSE_OFFSET1               1
#define RDD_PULSE_OFFSET2               2
#define RDD_PULSE_OFFSET3               3
#define RDD_PULSE_OFFSET4               4
#define RDD_PULSE_OFFSET5               5
#define RDD_PULSE_OFFSET6               6
#define RDD_PULSE_OFFSET7               7

#if (CFG_SUPPORT_DFS_MASTER == 1)
#define RDD_IN_SEL_0                    0
#define RDD_IN_SEL_1                    1
#endif

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
#define TXPOWER_EVENT_SHOW_ALL_RATE_TXPOWER_INFO    0x5
#endif

/* Network type */
#define NETWORK_INFRA	BIT(16)
#define NETWORK_P2P	BIT(17)
#define NETWORK_IBSS	BIT(18)
#define NETWORK_MESH	BIT(19)
#define NETWORK_BOW	BIT(20)
#define NETWORK_WDS	BIT(21)
#define NETWORK_NAN     BIT(22)

/* Station role */
#define STA_TYPE_STA 	BIT(0)
#define STA_TYPE_AP 	BIT(1)
#define STA_TYPE_ADHOC 	BIT(2)
#define STA_TYPE_TDLS 	BIT(3)
#define STA_TYPE_WDS 	BIT(4)

/* Connection type */
#define CONNECTION_INFRA_STA	(STA_TYPE_STA|NETWORK_INFRA)
#define CONNECTION_INFRA_AP		(STA_TYPE_AP|NETWORK_INFRA)
#define CONNECTION_P2P_GC		(STA_TYPE_STA|NETWORK_P2P)
#define CONNECTION_P2P_GO		(STA_TYPE_AP|NETWORK_P2P)
#define CONNECTION_P2P_DEVICE	(NETWORK_P2P)
#define CONNECTION_MESH_STA		(STA_TYPE_STA|NETWORK_MESH)
#define CONNECTION_MESH_AP		(STA_TYPE_AP|NETWORK_MESH)
#define CONNECTION_IBSS_ADHOC	(STA_TYPE_ADHOC|NETWORK_IBSS)
#define CONNECTION_NAN			(NETWORK_NAN)
#define CONNECTION_TDLS			(STA_TYPE_TDLS|NETWORK_INFRA)
#define CONNECTION_WDS			(STA_TYPE_WDS|NETWORK_WDS)

#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
#define CH_MAX_NUM                    128
#endif

#if (CFG_SUPPORT_PHY_ICS == 1)
#define MAX_PHY_ICS_DUMP_DATA_CNT	256
#endif /* CFG_SUPPORT_PHY_ICS */

/*
 * Definitions for extension CMD_ID
 */
enum ENUM_EXT_CMD_ID {
	EXT_CMD_ID_EFUSE_ACCESS = 0x01,
	EXT_CMD_ID_RF_REG_ACCESS = 0x02,
	EXT_CMD_ID_EEPROM_ACCESS = 0x03,
	EXT_CMD_ID_RF_TEST = 0x04,
	EXT_CMD_ID_RADIO_ON_OFF_CTRL = 0x05,
	EXT_CMD_ID_WIFI_RX_DISABLE = 0x06,
	EXT_CMD_ID_PM_STATE_CTRL = 0x07,
	EXT_CMD_ID_CHANNEL_SWITCH = 0x08,
	EXT_CMD_ID_NIC_CAPABILITY = 0x09,
	EXT_CMD_ID_AP_PWR_SAVING_CLEAR = 0x0A,
	EXT_CMD_ID_SET_WTBL2_RATETABLE = 0x0B,
	EXT_CMD_ID_GET_WTBL_INFORMATION = 0x0C,
	EXT_CMD_ID_ASIC_INIT_UNINIT_CTRL = 0x0D,
	EXT_CMD_ID_MULTIPLE_REG_ACCESS = 0x0E,
	EXT_CMD_ID_AP_PWR_SAVING_CAPABILITY = 0x0F,
	EXT_CMD_ID_SECURITY_ADDREMOVE_KEY = 0x10,
	EXT_CMD_ID_SET_TX_POWER_CONTROL = 0x11,
	EXT_CMD_ID_SET_THERMO_CALIBRATION = 0x12,
	EXT_CMD_ID_FW_LOG_2_HOST = 0x13,
	EXT_CMD_ID_AP_PWR_SAVING_START = 0x14,
	EXT_CMD_ID_MCC_OFFLOAD_START = 0x15,
	EXT_CMD_ID_MCC_OFFLOAD_STOP = 0x16,
	EXT_CMD_ID_LED = 0x17,
	EXT_CMD_ID_PACKET_FILTER = 0x18,
	EXT_CMD_ID_COEXISTENCE = 0x19,
	EXT_CMD_ID_PWR_MGT_BIT_WIFI = 0x1B,
	EXT_CMD_ID_GET_TX_POWER = 0x1C,
	EXT_CMD_ID_BF_ACTION = 0x1E,

	EXT_CMD_ID_WMT_CMD_OVER_WIFI = 0x20,
	EXT_CMD_ID_EFUSE_BUFFER_MODE = 0x21,
	EXT_CMD_ID_OFFLOAD_CTRL = 0x22,
	EXT_CMD_ID_THERMAL_PROTECT = 0x23,
	EXT_CMD_ID_CLOCK_SWITCH_DISABLE = 0x24,
	EXT_CMD_ID_STAREC_UPDATE = 0x25,
	EXT_CMD_ID_BSSINFO_UPDATE = 0x26,
	EXT_CMD_ID_EDCA_SET = 0x27,
	EXT_CMD_ID_SLOT_TIME_SET = 0x28,
	EXT_CMD_ID_DEVINFO_UPDATE = 0x2A,
	EXT_CMD_ID_NOA_OFFLOAD_CTRL = 0x2B,
	EXT_CMD_ID_GET_SENSOR_RESULT = 0x2C,
	EXT_CMD_ID_TMR_CAL = 0x2D,
	EXT_CMD_ID_WAKEUP_OPTION = 0x2E,
	EXT_CMD_ID_OBTW = 0x2F,

	EXT_CMD_ID_GET_TX_STATISTICS = 0x30,
	EXT_CMD_ID_AC_QUEUE_CONTROL = 0x31,
	EXT_CMD_ID_WTBL_UPDATE = 0x32,
	EXT_CMD_ID_BCN_UPDATE = 0x33,

	EXT_CMD_ID_DRR_CTRL = 0x36,
	EXT_CMD_ID_BSSGROUP_CTRL = 0x37,
	EXT_CMD_ID_VOW_FEATURE_CTRL = 0x38,
	EXT_CMD_ID_PKT_PROCESSOR_CTRL = 0x39,
	EXT_CMD_ID_PALLADIUM = 0x3A,
	EXT_CMD_ID_GET_MAC_INFO = 0x3C,
#if CFG_SUPPORT_MU_MIMO
	EXT_CMD_ID_MU_CTRL = 0x40,
#endif /* CFG_SUPPORT_MU_MIMO */
	EXT_CMD_ID_EFUSE_BUFFER_RD = 0x4E,
	EXT_CMD_ID_EFUSE_FREE_BLOCK = 0x4F,
	EXT_CMD_ID_DUMP_MEM = 0x57,
	EXT_CMD_ID_TX_POWER_FEATURE_CTRL = 0x58,
	EXT_CMD_ID_SER = 0x81,
	EXT_CMD_ID_TWT_AGRT_UPDATE = 0x94,
	EXT_CMD_ID_SYSDVT_TEST = 0x99,
#if (CFG_SUPPORT_802_11AX == 1)
	EXT_CMD_ID_SR_CTRL = 0xA8,
#endif
	EXT_CMD_ID_CR4_DMASHDL_DVT = 0xAB,
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	EXT_CMD_ID_TWT_STA_GET_CNM_GRANTED = 0xAC,
#endif
#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
	EXT_CMD_ID_BT_CTRL = 0xBF,
#endif

	EXT_CMD_ID_END
};

#if (CFG_SUPPORT_802_11AX == 1)
/* SR Command */
enum ENUM_SR_CMD_SUBID {
	/** SET **/
	SR_CMD_Reserve = 0x0,
	SR_CMD_SET_SR_CAP_SREN_CTRL,
	SR_CMD_SET_SR_CAP_ALL_CTRL,
	SR_CMD_SET_SR_PARA_ALL_CTRL,
	SR_CMD_SET_SR_GLO_VAR_DROP_TA_CTRL,
	SR_CMD_SET_SR_GLO_VAR_STA_CTRL,
	SR_CMD_SET_SR_GLO_VAR_STA_INIT_CTRL,
	SR_CMD_SET_SR_COND_ALL_CTRL,
	SR_CMD_SET_SR_RCPI_TBL_ALL_CTRL,
	SR_CMD_SET_SR_RCPI_TBL_OFST_ALL_CTRL,
	SR_CMD_SET_SR_Q_CTRL_ALL_CTRL,
	SR_CMD_SET_SR_IBPD_ALL_CTRL,
	SR_CMD_SET_SR_NRT_ALL_CTRL,
	SR_CMD_SET_SR_NRT_RESET_CTRL,
	SR_CMD_SET_SR_NRT_CTRL_ALL_CTRL,
	/** GET **/
	SR_CMD_GET_SR_CAP_ALL_INFO,
	SR_CMD_GET_SR_PARA_ALL_INFO,
	SR_CMD_GET_SR_GLO_VAR_SINGLE_DROP_TA_INFO,
	SR_CMD_GET_SR_IND_ALL_INFO,
	SR_CMD_GET_SR_COND_ALL_INFO,
	SR_CMD_GET_SR_RCPI_TBL_ALL_INFO,
	SR_CMD_GET_SR_RCPI_TBL_OFST_ALL_INFO,
	SR_CMD_GET_SR_Q_CTRL_ALL_INFO,
	SR_CMD_GET_SR_IBPD_ALL_INFO,
	SR_CMD_GET_SR_NRT_ALL_INFO,
	SR_CMD_GET_SR_NRT_CTRL_ALL_INFO,
	SR_CMD_NUM
};
/* End SR Command */

/* SR Event */
enum ENUM_SR_EVENT_SUBID {
	SR_EVENT_Reserve = 0x0,
	/** GET **/
	SR_EVENT_GET_SR_CAP_ALL_INFO,
	SR_EVENT_GET_SR_PARA_ALL_INFO,
	SR_EVENT_GET_SR_GLO_VAR_SINGLE_DROP_TA_INFO,
	SR_EVENT_GET_SR_IND_ALL_INFO,
	SR_EVENT_GET_SR_COND_ALL_INFO,
	SR_EVENT_GET_SR_RCPI_TBL_ALL_INFO,
	SR_EVENT_GET_SR_RCPI_TBL_OFST_ALL_INFO,
	SR_EVENT_GET_SR_Q_CTRL_ALL_INFO,
	SR_EVENT_GET_SR_IBPD_ALL_INFO,
	SR_EVENT_GET_SR_NRT_ALL_INFO,
	SR_EVENT_GET_SR_NRT_CTRL_ALL_INFO,
	SR_EVENT_NUM
};
/* End SR Event */
#endif

#if 0
enum NDIS_802_11_WEP_STATUS {
	Ndis802_11WEPEnabled,
	Ndis802_11Encryption1Enabled = Ndis802_11WEPEnabled,
	Ndis802_11WEPDisabled,
	Ndis802_11EncryptionDisabled = Ndis802_11WEPDisabled,
	Ndis802_11WEPKeyAbsent,
	Ndis802_11Encryption1KeyAbsent = Ndis802_11WEPKeyAbsent,
	Ndis802_11WEPNotSupported,
	Ndis802_11EncryptionNotSupported = Ndis802_11WEPNotSupported,
	Ndis802_11TKIPEnable,
	Ndis802_11Encryption2Enabled = Ndis802_11TKIPEnable,
	Ndis802_11Encryption2KeyAbsent,
	Ndis802_11AESEnable,
	Ndis802_11Encryption3Enabled = Ndis802_11AESEnable,
	Ndis802_11CCMP256Enable,
	Ndis802_11GCMP128Enable,
	Ndis802_11GCMP256Enable,
	Ndis802_11Encryption3KeyAbsent,
	Ndis802_11TKIPAESMix,
	/* TKIP or AES mix */
	Ndis802_11Encryption4Enabled = Ndis802_11TKIPAESMix,
	Ndis802_11Encryption4KeyAbsent,
	Ndis802_11GroupWEP40Enabled,
	Ndis802_11GroupWEP104Enabled,
#ifdef WAPI_SUPPORT
	Ndis802_11EncryptionSMS4Enabled,	/* WPI SMS4 support */
#endif /* WAPI_SUPPORT */
};
#endif

#define MAX_UEVENT_LEN	300

#if CFG_SUPPORT_BUFFER_MODE

struct CMD_EFUSE_BUFFER_MODE {
	uint8_t ucSourceMode;
	uint8_t ucCount;
	uint8_t ucCmdType;  /* 0:6632, 1: 7668 */
	uint8_t ucReserved;
	uint8_t aBinContent[MAX_EEPROM_BUFFER_SIZE];
};

struct CMD_EFUSE_BUFFER_MODE_CONNAC_T {
	uint8_t ucSourceMode;
	uint8_t ucContentFormat;
	uint16_t u2Count;
#if defined MT7915 || defined MT7961
	uint8_t aBinContent[BUFFER_BIN_PAGE_SIZE];
#else
	uint8_t aBinContent[MAX_EEPROM_BUFFER_SIZE];
#endif
};

struct CMD_EFUSE_BUFFER_RD_T {
	uint8_t ucSourceMode;
	uint8_t ucContentFormat;
	uint16_t u2Offset;
	uint16_t u2Count;
	uint16_t u2Reserved;
};

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
struct CMD_ACCESS_EFUSE {
	uint32_t u4Address;
	uint32_t u4Valid;
	uint8_t aucData[16];
};

struct CMD_EFUSE_FREE_BLOCK {
	uint8_t  ucGetFreeBlock;
	uint8_t  ucVersion;
	uint8_t  ucDieIndex;
	uint8_t  ucReserved;
};

struct CMD_GET_TX_POWER {
	uint8_t ucTxPwrType;
	uint8_t ucCenterChannel;
	uint8_t ucDbdcIdx; /* 0:Band 0, 1: Band1 */
	uint8_t ucBand; /* 0:G-band 1: A-band*/
	uint8_t ucReserved[4];
};

/*#endif*/

#endif /* CFG_SUPPORT_BUFFER_MODE */

#if CFG_SUPPORT_QA_TOOL
#define IQ_FILE_LINE_OFFSET     18
#define IQ_FILE_IQ_STR_LEN	 8
#define RTN_IQ_DATA_LEN         1024	/* return 1k per packet */

#define MCAST_WCID_TO_REMOVE	0


#define ICAP_CONTENT_ADC		0x10000006
#define ICAP_CONTENT_TOAE		0x7
#define ICAP_CONTENT_SPECTRUM	0xB
#define ICAP_CONTENT_RBIST		0x10
#define ICAP_CONTENT_DCOC		0x20
#define ICAP_CONTENT_FIIQ		0x48
#define ICAP_CONTENT_FDIQ		0x49


struct CMD_SET_TX_TARGET_POWER {
	int8_t cTxPwr2G4Cck;       /* signed, in unit of 0.5dBm */
	int8_t cTxPwr2G4Dsss;      /* signed, in unit of 0.5dBm */
	uint8_t ucTxTargetPwr;		/* Tx target power base for all*/
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

#if CFG_SUPPORT_MU_MIMO
enum {
	/* debug commands */
	MU_SET_ENABLE = 0,
	MU_GET_ENABLE,
	MU_SET_MUPROFILE_ENTRY,
	MU_GET_MUPROFILE_ENTRY,
	MU_SET_GROUP_TBL_ENTRY,
	MU_GET_GROUP_TBL_ENTRY,
	MU_SET_CLUSTER_TBL_ENTRY,
	MU_GET_CLUSTER_TBL_ENTRY,
	MU_SET_GROUP_USER_THRESHOLD,
	MU_GET_GROUP_USER_THRESHOLD,
	MU_SET_GROUP_NSS_THRESHOLD,
	MU_GET_GROUP_NSS_THRESHOLD,
	MU_SET_TXREQ_MIN_TIME,
	MU_GET_TXREQ_MIN_TIME,
	MU_SET_SU_NSS_CHECK,
	MU_GET_SU_NSS_CHECK,
	MU_SET_CALC_INIT_MCS,
	MU_GET_CALC_INIT_MCS,
	MU_SET_TXOP_DEFAULT,
	MU_GET_TXOP_DEFAULT,
	MU_SET_SU_LOSS_THRESHOLD,
	MU_GET_SU_LOSS_THRESHOLD,
	MU_SET_MU_GAIN_THRESHOLD,
	MU_GET_MU_GAIN_THRESHOLD,
	MU_SET_SECONDARY_AC_POLICY,
	MU_GET_SECONDARY_AC_POLICY,
	MU_SET_GROUP_TBL_DMCS_MASK,
	MU_GET_GROUP_TBL_DMCS_MASK,
	MU_SET_MAX_GROUP_SEARCH_CNT,
	MU_GET_MAX_GROUP_SEARCH_CNT,
	MU_GET_MU_PROFILE_TX_STATUS_CNT,
	MU_SET_TRIGGER_MU_TX,
	/* F/W flow test commands */
	MU_SET_TRIGGER_GID_MGMT_FRAME,
	/* HQA STA commands */
	MU_HQA_SET_STA_PARAM = 60,
	/* HQA AP Commands*/
	MU_HQA_SET_ENABLE = 70,
	MU_HQA_SET_SNR_OFFSET,
	MU_HQA_SET_ZERO_NSS,
	MU_HQA_SET_SPEED_UP_LQ,
	MU_HQA_SET_GROUP,
	MU_HQA_SET_MU_TABLE,
	MU_HQA_SET_SU_TABLE,
	MU_HQA_SET_CALC_LQ,
	MU_HQA_GET_CALC_LQ,
	MU_HQA_SET_CALC_INIT_MCS,
	MU_HQA_GET_CALC_INIT_MCS,
	MU_HQA_GET_QD,
};
#endif /* CFG_SUPPORT_MU_MIMO */
#endif /* CFG_SUPPORT_QA_TOOL */


enum ENUM_SCN_FUNC_MASK {
	ENUM_SCN_RANDOM_MAC_EN = (1 << 0),
	ENUM_SCN_DBDC_SCAN_DIS = (1 << 1),
	ENUM_SCN_DBDC_SCAN_TYPE3 = (1 << 2),
	ENUM_SCN_USE_PADDING_AS_BSSID = (1 << 3),
	ENUM_SCN_RANDOM_SN_EN = (1 << 4),
	ENUM_SCN_SPLIT_SCAN_EN = (1 << 5),
	ENUM_SCN_FENCE_SCAN_EN = (1 << 6),
	ENUM_SCN_OCE_SCAN_EN = (1 << 7),
};

enum ENUM_SCN_FUNC_EXT_MASK {
	ENUM_SCN_MDT_SCAN = (1 << 0),
	ENUM_SCN_ML_PROBE = (1 << 1),
	ENUM_SCN_LIT_FORCE = (1 << 2),
	ENUM_SCN_LIT_FORCE_PRECISE = (1 << 3),
	ENUM_SCN_LIT_FROCE_BEST_EFFORT = (1 << 4),
	ENUM_SCN_RNR_SCAN = (1 << 13),
	ENUM_SCN_LLW_SCAN = (1 << 14),
	ENUM_SCN_SILENT_SCAN = (1 << 15)
};

enum ENUM_SCN_SOURCE_MASK {
	ENUM_SCN_NORMAL = (1 << 0),
	ENUM_SCN_ROMAING = (1 << 1),
	/* FW trigger scan */
	ENUM_SCN_SOURCE_FW = (1 << 2),
	/* Sensor hub trigger scan */
	ENUM_SCN_SOURCE_FENCE = (1 << 3),
	ENUM_SCN_SOURCE_MASK_NUM
};

#if CFG_WOW_SUPPORT

/* Filter Flag */
#define WOWLAN_FF_DROP_ALL                      BIT(0)
#define WOWLAN_FF_SEND_MAGIC_TO_HOST            BIT(1)
#define WOWLAN_FF_ALLOW_ARP                     BIT(2)
#define WOWLAN_FF_ALLOW_BMC                     BIT(3)
#define WOWLAN_FF_ALLOW_UC                      BIT(4)
#define WOWLAN_FF_ALLOW_1X                      BIT(5)
#define WOWLAN_FF_ALLOW_ARP_REQ2ME              BIT(6)

/* wow detect type (8 bits)*/
#define WOWLAN_DETECT_TYPE_NONE                 0
#define WOWLAN_DETECT_TYPE_MAGIC                BIT(0)
#define WOWLAN_DETECT_TYPE_ANY                  BIT(1)
#define WOWLAN_DETECT_TYPE_DISCONNECT           BIT(2)
#define WOWLAN_DETECT_TYPE_GTK_REKEY_FAILURE    BIT(3)
#define WOWLAN_DETECT_TYPE_BCN_LOST             BIT(4)
#define WOWLAN_DETECT_TYPE_SCHD_SCAN_SSID_HIT   BIT(5)
#define WOWLAN_DETECT_TYPE_BITMAP               BIT(6)
#define WOWLAN_DETECT_TYPE_MC_DATA              BIT(7)

/* wow detect type Extension (16 bits)*/
#define WOWLAN_DETECT_TYPE_EXT_NONE             0
#define WOWLAN_DETECT_TYPE_EXT_PORT             BIT(0)
#define WOWLAN_DETECT_TYPE_EXT_IPV4_TCP_SYN     BIT(1)
#define WOWLAN_DETECT_TYPE_EXT_IPV6_TCP_SYN     BIT(2)
#define WOWLAN_DETECT_TYPE_EXT_EAPOL_REQUEST    BIT(3)
#define WOWLAN_DETECT_TYPE_EXT_ACTION           BIT(4)
#define WOWLAN_DETECT_TYPE_EXT_IPV6_ICMP        BIT(5)
#define WOWLAN_DETECT_TYPE_EXT_ROAMING_INTR     BIT(6)

/* Wakeup command bit define */
#define PF_WAKEUP_CMD_BIT0_OUTPUT_MODE_EN   BIT(0)
#define PF_WAKEUP_CMD_BIT1_OUTPUT_DATA      BIT(1)
#define PF_WAKEUP_CMD_BIT2_WAKEUP_LEVEL     BIT(2)

#define PM_WOWLAN_REQ_START                 0x1
#define PM_WOWLAN_REQ_STOP                  0x2
#define PM_WOWLAN_REQ_WINDOWS_START         0x3
#define PM_WOWLAN_REQ_WINDOWS_STOP          0x4

struct EVENT_WOWLAN_NOTIFY {
	uint8_t	ucNetTypeIndex;
	uint8_t	aucReserved[3];
};

/* PACKETFILTER CAPABILITY TYPE */

#define PACKETF_CAP_TYPE_ARP			BIT(1)
#define PACKETF_CAP_TYPE_MAGIC			BIT(2)
#define PACKETF_CAP_TYPE_BITMAP			BIT(3)
#define PACKETF_CAP_TYPE_EAPOL			BIT(4)
#define PACKETF_CAP_TYPE_TDLS			BIT(5)
#define PACKETF_CAP_TYPE_CF				BIT(6)
#define PACKETF_CAP_TYPE_HEARTBEAT		BIT(7)
#define PACKETF_CAP_TYPE_TCP_SYN		BIT(8)
#define PACKETF_CAP_TYPE_UDP_SYN		BIT(9)
#define PACKETF_CAP_TYPE_BCAST_SYN		BIT(10)
#define PACKETF_CAP_TYPE_MCAST_SYN		BIT(11)
#define PACKETF_CAP_TYPE_V6				BIT(12)
#define PACKETF_CAP_TYPE_TDIM			BIT(13)


enum _ENUM_FUNCTION_SELECT {
	FUNCTION_PF				= 1,
	FUNCTION_BITMAP			= 2,
	FUNCTION_EAPOL			= 3,
	FUNCTION_TDLS			= 4,
	FUNCTION_ARPNS			= 5,
	FUNCTION_CF				= 6,
	FUNCTION_MODE			= 7,
	FUNCTION_BSSID			= 8,
	FUNCTION_MGMT			= 9,
	FUNCTION_BMC_DROP		= 10,
	FUNCTION_UC_DROP		= 11,
	FUNCTION_ALL_TOMCU		= 12,
};

struct CMD_PACKET_FILTER_CAP {
	uint8_t			ucCmd;
	uint16_t			packet_cap_type;
	uint8_t			aucReserved1[1];
	/* GLOBAL */
	uint32_t			PFType;
	uint32_t			FunctionSelect;
	uint32_t			Enable;
	/* MAGIC */
	uint8_t			ucBssid;
	uint16_t			usEnableBits;
	uint8_t			aucReserved5[1];
	/* DTIM */
	uint8_t			DtimEnable;
	uint8_t			DtimValue;
	uint8_t			aucReserved2[2];
	/* BITMAP_PATTERN_T */
	uint32_t			Index;
	uint32_t			Offset;
	uint32_t			FeatureBits;
	uint32_t			Resv;
	uint32_t			PatternLength;
	uint32_t			Mask[4];
	uint32_t			Pattern[32];
	/* COALESCE */
	uint32_t			FilterID;
	uint32_t			PacketType;
	uint32_t			CoalesceOP;
	uint8_t			FieldLength;
	uint8_t			CompareOP;
	uint8_t			FieldID;
	uint8_t			aucReserved3[1];
	uint32_t			Pattern3[4];
	/* TCPSYN */
	uint32_t			AddressType;
	uint32_t			TCPSrcPort;
	uint32_t			TCPDstPort;
	uint32_t			SourceIP[4];
	uint32_t			DstIP[4];
	uint8_t			aucReserved4[64];
};
#endif /*CFG_WOW_SUPPORT*/

#if CFG_SUPPORT_WIFI_HOST_OFFLOAD
enum ENUM_PF_OPCODE {
	PF_OPCODE_ADD = 0,
	PF_OPCODE_DEL,
	PF_OPCODE_ENABLE,
	PF_OPCODE_DISABLE,
	PF_OPCODE_NUM
};

struct CMD_TCP_GENERATOR {
	enum ENUM_PF_OPCODE eOpcode;
	uint32_t u4ReplyId;
	uint32_t u4Period;
	uint32_t u4Timeout;
	uint32_t u4IpId;
	uint32_t u4DestPort;
	uint32_t u4SrcPort;
	uint32_t u4Seq;
	uint8_t aucDestIp[4];
	uint8_t aucSrcIp[4];
	uint8_t aucDestMac[6];
	uint8_t ucBssId;
	uint8_t aucReserved1[1];
	uint8_t aucReserved2[64];
};

struct CMD_PATTERN_GENERATOR {
	enum ENUM_PF_OPCODE eOpcode;
	uint32_t u4ReplyId;
	uint32_t u4EthernetLength;
	uint32_t u4Period;
	uint8_t aucEthernetFrame[128];
	uint8_t ucBssId;
	uint8_t aucReserved1[3];
	uint8_t aucReserved2[64];
};

struct CMD_BITMAP_FILTER {
	enum ENUM_PF_OPCODE eOpcode;
	uint32_t u4ReplyId;
	uint32_t u4Offset;
	uint32_t u4Length;
	uint8_t aucPattern[64];
	uint8_t aucBitMask[64];
	u_int8_t fgIsEqual;
	u_int8_t fgIsAccept;
	uint8_t ucBssId;
	uint8_t aucReserved1[1];
	uint8_t aucReserved2[64];
};

#endif /*CFG_SUPPORT_WIFI_HOST_OFFLOAD*/

struct CMD_RX_PACKET_FILTER {
	uint32_t u4RxPacketFilter;
	uint8_t aucReserved[64];
};


#if defined(MT6632)
#define S2D_INDEX_CMD_H2N      0x0
#define S2D_INDEX_CMD_C2N      0x1
#define S2D_INDEX_CMD_H2C      0x2
#define S2D_INDEX_CMD_H2N_H2C  0x3

#define S2D_INDEX_EVENT_N2H     0x0
#define S2D_INDEX_EVENT_N2C     0x1
#define S2D_INDEX_EVENT_C2H     0x2
#define S2D_INDEX_EVENT_N2H_N2C 0x3
#endif

#define EXT_EVENT_TARGET_TX_POWER  0x1

#define EXT_EVENT_ID_CMD_RESULT 0x00
#define EXT_EVENT_ID_EFUSE_ACCESS 0x01
#define EXT_EVENT_ID_RF_REG_ACCESS 0x02
#define EXT_EVENT_ID_EEPROM_ACCESS 0x03
#define EXT_EVENT_ID_RF_TEST 0x04
#define EXT_EVENT_ID_PS_SYNC 0x05
#define EXT_EVENT_ID_SLEEPY_NOTIFY 0x06
#define EXT_EVENT_ID_WLAN_ERROR 0x07
#define EXT_EVENT_ID_NIC_CAPABILITY 0x09
#define EXT_EVENT_ID_AP_PWR_SAVING_CLEAR 0x0A
#define EXT_EVENT_ID_SET_WTBL2_RATETABLE 0x0B
#define EXT_EVENT_ID_GET_WTBL2_INFORMATION 0x0C
#define EXT_EVENT_ID_MULTIPLE_REG_ACCESS 0x0E
#define EXT_EVENT_ID_AP_PWR_SAVING_CAPABILITY 0x0F
#define EXT_EVENT_ID_SECURITY_ADDREMOVE_KEY 0x10
#define EXT_EVENT_ID_FW_LOG_2_HOST 0x13
#define EXT_EVENT_ID_AP_PWR_SAVING_START 0x14
#define EXT_EVENT_ID_PACKET_FILTER 0x18
#define EXT_EVENT_ID_COEXISTENCE 0x19
#define EXT_EVENT_ID_BEACON_LOSS 0x1A
#define EXT_EVENT_ID_PWR_MGT_BIT_WIFI 0x1B
#define EXT_EVENT_ID_GET_TX_POWER 0x1C

#define EXT_EVENT_ID_WMT_EVENT_OVER_WIFI 0x20
#define EXT_EVENT_ID_MCC_TRIGGER 0x21
#define EXT_EVENT_ID_THERMAL_PROTECT 0x22
#define EXT_EVENT_ID_ASSERT_DUMP 0x23
#define EXT_EVENT_ID_GET_SENSOR_RESULT 0x2C
#define EXT_EVENT_ID_ROAMING_DETECT_NOTIFICATION 0x2D
#define EXT_EVENT_ID_TMR_CAL 0x2E
#define EXT_EVENT_ID_RA_THROUGHPUT_BURST 0x2F

#define EXT_EVENT_ID_GET_TX_STATISTIC 0x30
#define EXT_EVENT_ID_PRETBTT_INT 0x31
#define EXT_EVENT_ID_WTBL_UPDATE 0x32

#define EXT_EVENT_ID_BF_STATUS_READ 0x35
#define EXT_EVENT_ID_DRR_CTRL 0x36
#define EXT_EVENT_ID_BSSGROUP_CTRL 0x37
#define EXT_EVENT_ID_VOW_FEATURE_CTRL 0x38
#define EXT_EVENT_ID_PKT_PROCESSOR_CTRL 0x39
#define EXT_EVENT_ID_RDD_REPORT 0x3A
#define EXT_EVENT_ID_DEVICE_CAPABILITY 0x3B
#define EXT_EVENT_ID_MAC_INFO 0x3C
#define EXT_EVENT_ID_ATE_TEST_MODE 0x3D
#define EXT_EVENT_ID_CAC_END 0x3E
#define EXT_EVENT_ID_MU_CTRL 0x40

#define EXT_EVENT_ID_DBDC_CTRL 0x45
#define EXT_EVENT_ID_CONFIG_MUAR 0x48

#define EXT_EVENT_ID_RX_AIRTIME_CTRL 0x4a
#define EXT_EVENT_ID_AT_PROC_MODULE 0x4b
#define EXT_EVENT_ID_MAX_AMSDU_LENGTH_UPDATE 0x4c
#define EXT_EVENT_ID_EFUSE_FREE_BLOCK 0x4d
#define EXT_EVENT_ID_MURA_CTRL 0x4d
#define EXT_EVENT_ID_CSA_NOTIFY 0x4F
#define EXT_EVENT_ID_WIFI_SPECTRUM 0x50
#define EXT_EVENT_ID_TMR_CALCU_INFO 0x51
#define EXT_EVENT_ID_DUMP_MEM 0x57
#define EXT_EVENT_ID_TX_POWER_FEATURE_CTRL 0x58

#define EXT_EVENT_ID_G_BAND_256QAM_PROBE_RESULT 0x6B
#define EXT_EVENT_ID_MPDU_TIME_UPDATE 0x6F
#define EXT_EVENT_ID_SER 0x81
#define EXT_EVENT_ID_SYSDVT_TEST 0x99
#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
#define EXT_EVENT_ID_BT_CTRL	0x9C
#endif
#if (CFG_SUPPORT_802_11AX == 1)
#define EXT_EVENT_ID_SR_INFO 0xA8
#endif
/*#endif*/

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
#define EXT_EVENT_ID_TX_POWER_FEATURE_CTRL  0x58
#endif

#if (CFG_SUPPORT_TWT == 1)
/* TWT related definitions */
#define TWT_AGRT_MAX_NUM        16
#define TWT_GRP_MAX_NUM         8
#define TWT_GRP_MAX_MEMBER_CNT  8

/*
 * Bitmap definition for ucAgrtParaBitmap field
 * in struct _EXT_CMD_TWT_ARGT_UPDATE_T
 */
#define TWT_AGRT_PARA_BITMAP_IS_TRIGGER			BIT(0)
#define TWT_AGRT_PARA_BITMAP_IS_ANNOUNCE		BIT(1)
#define TWT_AGRT_PARA_BITMAP_IS_PROTECT			BIT(2)

#define TWT_AGRT_PARA_BITMAP_TRIGGER_OFFSET		0
#define TWT_AGRT_PARA_BITMAP_ANNCE_OFFSET		1
#define TWT_AGRT_PARA_BITMAP_PROTECT_OFFSET		2

enum _TWT_AGRT_CTRL_CODE_T {
	TWT_AGRT_CTRL_ADD = 0,
	TWT_AGRT_CTRL_MODIFY,
	TWT_AGRT_CTRL_DELETE,
	TWT_AGRT_CTRL_TEARDOWN,
	TWT_AGRT_CTRL_RESET,
	TWT_AGRT_CTRL_SUSPEND,
	TWT_AGRT_CTRL_SUSPEND_RESUME,
	TWT_AGRT_CTRL_AGRT_ALLOC,
	TWT_AGRT_CTRL_AGRT_REALEASE,
	TWT_AGRT_CTRL_CNM_ABORT
};
#endif

/* ID for different MAC Info */
enum {
	MAC_INFO_TYPE_RESERVE = 0,
	MAC_INFO_TYPE_CHANNEL_BUSY_CNT = 0x1,
	MAC_INFO_TYPE_TSF = 0x2,
	MAC_INFO_TYPE_MIB = 0x3,
	MAC_INFO_TYPE_EDCA = 0x4,
	MAC_INFO_TYPE_WIFI_INT_CNT = 0x5,
	MAC_INFO_TYPE_TWT_STA_CNM = 0x6,
};

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
#ifndef LINUX
#endif
/* for Event Packet (via HIF-RX) */
struct PSE_CMD_HDR {
	/* DW0 */
	uint16_t u2TxByteCount;
	uint16_t u2Reserved1: 10;
	uint16_t u2Qidx: 5;
	uint16_t u2Pidx: 1;

	/* DW1 */
	uint16_t u2Reserved2: 13;
	uint16_t u2Hf: 2;
	uint16_t u2Ft: 1;
	uint16_t u2Reserved3: 8;
	uint16_t u2PktFt: 2;
	uint16_t u2Reserved4: 6;

	/* DW2~7 */
	uint32_t au4Reserved[6];
};

struct WIFI_CMD {
	uint16_t u2TxByteCount;	/* Max value is over 2048 */
	uint16_t u2PQ_ID;	/* Must be 0x8000 (Port1, Queue 0) */

	uint8_t ucWlanIdx;
	uint8_t ucHeaderFormat;
	uint8_t ucHeaderPadding;
	uint8_t ucPktFt: 2;
	uint8_t ucOwnMAC: 6;
	uint32_t au4Reserved1[6];

	uint16_t u2Length;
	uint16_t u2PqId;

	uint8_t ucCID;
	uint8_t ucPktTypeID;	/* Must be 0x20 (CMD Packet) */
	uint8_t ucSetQuery;
	uint8_t ucSeqNum;

	/* padding fields, hw may auto modify this field */
	uint8_t ucD2B0Rev;
	uint8_t ucExtenCID;	/* Extend CID */
	uint8_t ucS2DIndex;	/* Index for Src to Dst in CMD usage */
	uint8_t ucExtCmdOption;	/* Extend CID option */

	uint8_t ucCmdVersion;
	uint8_t ucReserved2[3];
	uint32_t au4Reserved3[4];	/* padding fields */

	uint8_t aucBuffer[];
};

/* for Command Packet (via HIF-TX) */
/* following CM's documentation v0.7 */
struct WIFI_EVENT {
	uint16_t u2PacketLength;
	uint16_t u2PacketType;	/* Must be filled with 0xE000 (EVENT Packet) */
	uint8_t ucEID;
	uint8_t ucSeqNum;
	uint8_t ucEventVersion;
	uint8_t aucReserved[1];

	uint8_t ucExtenEID;
	uint8_t aucReserved2[2];
	uint8_t ucS2DIndex;

	uint8_t aucBuffer[];
};

enum ENUM_CMD_TEST_CTRL_ACT {
	CMD_TEST_CTRL_ACT_SWITCH_MODE = 0,
	CMD_TEST_CTRL_ACT_SET_AT = 1,
	CMD_TEST_CTRL_ACT_GET_AT = 2,
	CMD_TEST_CTRL_ACT_SET_AT_ENG = 3,
	CMD_TEST_CTRL_ACT_GET_AT_ENG = 4,
	CMD_TEST_CTRL_ACT_NUM
};

enum ENUM_CMD_TEST_CTRL_ACT_SWITCH_MODE_OP {
	CMD_TEST_CTRL_ACT_SWITCH_MODE_NORMAL = 0,
	CMD_TEST_CTRL_ACT_SWITCH_MODE_RF_TEST = 1,
	CMD_TEST_CTRL_ACT_SWITCH_MODE_ICAP = 2,
	CMD_TEST_CTRL_ACT_SWITCH_MODE_NUM
};

/* CMD_ID_TEST_CTRL */
struct CMD_TEST_CTRL {
	uint8_t ucAction;
	uint8_t aucReserved[3];
	union {
		uint32_t u4OpMode;
		uint32_t u4ChannelFreq;
		struct PARAM_MTK_WIFI_TEST_STRUCT rRfATInfo;
	} u;
};

struct CMD_TEST_CTRL_EXT_T {
	uint8_t ucAction;
	uint8_t ucIcapLen;
	uint8_t aucReserved[2];
	union {
		uint32_t u4OpMode;
		uint32_t u4ChannelFreq;
		struct PARAM_MTK_WIFI_TEST_STRUCT_EXT_T rRfATInfo;
	} u;
};

/* EVENT_TEST_STATUS */
struct PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT {
	uint32_t u4PktSentStatus;
	uint32_t u4PktSentCount;
	uint16_t u2AvgAlc;
	uint8_t ucCckGainControl;
	uint8_t ucOfdmGainControl;
};

struct PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT {
	/*!< number of packets that Rx ok from interrupt */
	uint32_t u4IntRxOk;
	/*!< number of packets that CRC error from interrupt */
	uint32_t u4IntCrcErr;
	/*!< number of packets that is short preamble from interrupt */
	uint32_t u4IntShort;
	/*!< number of packets that is long preamble from interrupt */
	uint32_t u4IntLong;
	/*!< number of packets that Rx ok from PAU */
	uint32_t u4PauRxPktCount;
	/*!< number of packets that CRC error from PAU */
	uint32_t u4PauCrcErrCount;
	/*!< number of packets that is short preamble from PAU */
	uint32_t u4PauRxFifoFullCount;
	uint32_t u4PauCCACount;	/*!< CCA rising edge count */
};

union EVENT_TEST_STATUS {
	struct PARAM_MTK_WIFI_TEST_STRUCT rATInfo;
	/* PARAM_CUSTOM_RFTEST_TX_STATUS_STRUCT_T   rTxStatus; */
	/* PARAM_CUSTOM_RFTEST_RX_STATUS_STRUCT_T   rRxStatus; */
};

/* CMD_ID_DEFAULT_KEY_ID */
struct CMD_DEFAULT_KEY {
	uint8_t ucBssIdx;
	uint8_t ucKeyId;
	uint8_t ucWlanIndex;
	uint8_t ucMulticast;
};

/* WPA2 PMKID cache structure */
struct PMKID_ENTRY {
	struct LINK_ENTRY rLinkEntry;
	struct PARAM_PMKID rBssidInfo;
	uint16_t u2StatusCode;
	uint16_t u2ReasonCode;
};

struct CMD_802_11_PMKID {
	uint32_t u4BSSIDInfoCount;
	struct PMKID_ENTRY *arPMKIDInfo[1];
};

struct CMD_GTK_REKEY_DATA {
	uint8_t aucKek[16];
	uint8_t aucKck[16];
	uint8_t aucReplayCtr[8];
};

struct CMD_CSUM_OFFLOAD {
	uint16_t u2RxChecksum;	/* bit0: IP, bit1: UDP, bit2: TCP */
	uint16_t u2TxChecksum;	/* bit0: IP, bit1: UDP, bit2: TCP */
};

/* CMD_BASIC_CONFIG */
struct CMD_BASIC_CONFIG {
	uint8_t ucNative80211;
	uint8_t ucCtrlFlagAssertPath;
	uint8_t ucCtrlFlagDebugLevel;
	uint8_t aucReserved[1];
	struct CMD_CSUM_OFFLOAD rCsumOffload;
	uint8_t	ucCrlFlagSegememt;
	uint8_t	aucReserved2[3];
};

/* CMD_MAC_MCAST_ADDR */
struct CMD_MAC_MCAST_ADDR {
	uint32_t u4NumOfGroupAddr;
	uint8_t ucBssIndex;
	uint8_t aucReserved[3];
	uint8_t arAddress[MAX_NUM_GROUP_ADDR][PARAM_MAC_ADDR_LEN];
};

/* CMD_ACCESS_EEPROM */
struct CMD_ACCESS_EEPROM {
	uint16_t u2Offset;
	uint16_t u2Data;
};

/* EVENT_CONNECTION_STATUS */
struct EVENT_CONNECTION_STATUS {
	uint8_t ucMediaStatus;
	uint8_t ucReasonOfDisconnect;

	uint8_t ucInfraMode;
	uint8_t ucSsidLen;
	uint8_t aucSsid[PARAM_MAX_LEN_SSID];
	uint8_t aucBssid[PARAM_MAC_ADDR_LEN];
	uint8_t ucAuthenMode;
	uint8_t ucEncryptStatus;
	uint16_t u2BeaconPeriod;
	uint16_t u2AID;
	uint16_t u2ATIMWindow;
	uint8_t ucNetworkType;
	uint8_t aucReserved[1];
	uint32_t u4FreqInKHz;

#if CFG_ENABLE_WIFI_DIRECT
	uint8_t aucInterfaceAddr[PARAM_MAC_ADDR_LEN];
#endif

};

/* EVENT_NIC_CAPABILITY */
#define FEATURE_FLAG0_NIC_CAPABILITY_V2 BIT(0)

struct EVENT_NIC_CAPABILITY {
	uint16_t u2ProductID;
	uint16_t u2FwVersion;
	uint16_t u2DriverVersion;
	uint8_t ucHw5GBandDisabled;
	uint8_t ucEepromUsed;
	uint8_t aucMacAddr[6];
	uint8_t ucEndianOfMacAddrNumber;
	uint8_t ucHwNotSupportAC;

	uint8_t ucRfVersion;
	uint8_t ucPhyVersion;
	uint8_t ucRfCalFail;
	uint8_t ucBbCalFail;
	uint8_t aucDateCode[16];
	uint32_t u4FeatureFlag0;
	uint32_t u4FeatureFlag1;
	uint32_t u4CompileFlag0;
	uint32_t u4CompileFlag1;
	uint8_t aucBranchInfo[4];
	uint8_t ucFwBuildNumber;
	uint8_t ucHwSetNss1x1;
	uint8_t ucHwNotSupportDBDC;
	uint8_t ucHwBssIdNum;
	uint8_t aucReserved1[56];
};

struct EVENT_NIC_CAPABILITY_V2 {
	uint16_t u2TotalElementNum;
	uint8_t aucReserved[2];
	uint8_t aucBuffer[];
};

struct NIC_CAPABILITY_V2_ELEMENT {
	uint32_t tag_type; /* NIC_CAPABILITY_V2_TAG_T */
	uint32_t body_len;
	uint8_t aucbody[];
};

typedef uint32_t(*NIC_CAP_V2_ELEMENT_HDLR)(
	struct ADAPTER *prAdapter, uint8_t *buff);
struct NIC_CAPABILITY_V2_REF_TABLE {
	uint32_t tag_type; /* NIC_CAPABILITY_V2_TAG_T */
	NIC_CAP_V2_ELEMENT_HDLR hdlr;
};

enum NIC_CAPABILITY_V2_TAG {
	TAG_CAP_TX_RESOURCE = 0x0,
	TAG_CAP_TX_EFUSEADDRESS = 0x1,
	TAG_CAP_COEX_FEATURE = 0x2,
	TAG_CAP_SINGLE_SKU = 0x3,
	TAG_CAP_CSUM_OFFLOAD = 0x4,
	TAG_CAP_HW_VERSION = 0x5,
	TAG_CAP_SW_VERSION = 0x6,
	TAG_CAP_MAC_ADDR = 0x7,
	TAG_CAP_PHY_CAP = 0x8,
	TAG_CAP_MAC_CAP = 0x9,
	TAG_CAP_FRAME_BUF_CAP = 0xa,
	TAG_CAP_BEAMFORM_CAP = 0xb,
	TAG_CAP_LOCATION_CAP = 0xc,
	TAG_CAP_MUMIMO_CAP = 0xd,
	TAG_CAP_BUFFER_MODE_INFO = 0xe,
	TAG_CAP_HW_ADIE_VERSION = 0x14,
#if CFG_SUPPORT_ANT_SWAP
	TAG_CAP_ANTSWP = 0x16,
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	TAG_CAP_6G_CAP = 0x18,
#endif
	TAG_CAP_HOST_STATUS_EMI_OFFSET = 0x19,
#if CFG_FAST_PATH_SUPPORT
	TAG_CAP_FAST_PATH = 0x1a,
#endif
#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
	TAG_CAP_PSE_RX_QUOTA = 0x1b,
#endif

	TAG_CAP_LLS_DATA_EMI_OFFSET = 0x1c, /* Shared EMI offset for LLS */
	TAG_CAP_CASAN_LOAD_TYPE = 0x1d,
	TAG_CAP_REDL_INFO = 0x1e,
	TAG_CAP_HOST_SUSPEND_INFO = 0x1f,
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
#if (CFG_SUPPORT_QA_TOOL == 1)
	TAG_CAP_RF_TEST_CAP = 0x21,
#endif
#endif
	TAG_CAP_MLO_CAP = 0x22,
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	TAG_CAP_STATS_REG_MONTR_EMI_OFFSET = 0x23,
#endif
#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
	TAG_CAP_SW_SYNC_BY_EMI = 0x25,
#endif
	TAG_CAP_LIMITED = 0x27,
	TAG_CAP_P2P = 0x28,
#if CFG_SUPPORT_MLR
	TAG_CAP_MLR_CAP = 0x38,
#endif
	TAG_CAP_PERF_IND_FROM_EMI = 0x40,
#if CFG_SUPPORT_MBRAIN
	TAG_CAP_MBRAIN_EMI_INFO = 0x41,
#if CFG_SUPPORT_MBRAIN_BIGDATA
	TAG_CAP_MBRAIN_BIGDATA_VER = 0x42,
#endif
#endif
	UNI_CAP_CNM_BAND = 0x43,
	TAG_CAP_TOTAL
};

#if CFG_TCP_IP_CHKSUM_OFFLOAD
struct NIC_CSUM_OFFLOAD {
	uint8_t ucIsSupportCsumOffload;  /* 1: Support, 0: Not Support */
	uint8_t aucReseved[3];
};
#endif

struct NIC_COEX_FEATURE {
	uint32_t u4FddMode;  /* TRUE for COEX FDD mode */
};

struct NIC_EFUSE_ADDRESS {
	uint32_t u4EfuseStartAddress;  /* Efuse Start Address */
	uint32_t u4EfuseEndAddress;   /* Efuse End Address */
};

__KAL_ATTRIB_PACKED_FRONT__
struct TPUT_FACTOR_LIST_T {
	/** Event source: TPUT_EVENT_START ~ */
	uint32_t u4EvtId;

	/** Factor version */
	uint32_t u4Ver;

	uint8_t Cont[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct TPUT_SUB_FACTOR_T {
	/** sub header */
	uint8_t ucTag;
	uint16_t u2Len;
	uint8_t ucRes;

	uint8_t Cont[];
} __KAL_ATTRIB_PACKED__;

__KAL_ATTRIB_PACKED_FRONT__
struct TPUT_BKRS_FACTOR_T {
	/** Collection type: TPUT_COLL_CMD_INIT ~ */
	uint32_t u4EvtId;

	/** Factor type: _TPUT_FACTOR_TAG */
	uint32_t u4FactorType;

	/** sub header */
	/* TPUT_FACTOR_TYPE_LMAC_TMAC
	 * ~ TPUT_FACTOR_TYPE_LMAC_RMAC
	 */
	uint8_t ucTag;
	uint16_t u2Len;
	uint8_t ucIndex;

	/* reference to backup/restore table */
	uint32_t u4AddrStart, u4AddrEnd;
	uint32_t u4RegNum;

	uint8_t Cont[];
} __KAL_ATTRIB_PACKED__;

struct CAP_HW_VERSION {
	uint16_t u2ProductID; /* CHIP ID */
	uint8_t ucEcoVersion; /* ECO version */
	uint8_t ucReserved;
	uint32_t u4MacIpID; /* MAC IP version */
	uint32_t u4BBIpID; /* Baseband IP version */
	uint32_t u4TopIpID; /* TOP IP version */
	uint32_t u4ConfigId;  /* Configuration ID */
};

struct CAP_HW_ADIE_VERSION {
	uint16_t u2ProductID; /* CHIP ID */
	uint16_t u2EcoVersion; /* ECO version */
	uint32_t aucReserved[4];
};

struct CAP_SW_VERSION {
	uint16_t u2FwVersion; /* FW version <major.minor> */
	uint16_t u2FwBuildNumber; /* FW build number */
	uint8_t aucBranchInfo[4]; /* Branch name in ASCII */
	uint8_t aucDateCode[16]; /* FW build data code */
};

struct CAP_MAC_ADDR {
	uint8_t aucMacAddr[6];
	uint8_t aucReserved[2];
};

enum ENUM_CAP_PHY_MAX_BW {
	CAP_PHY_MAX_BW20, /* support 20 MHZ */
	CAP_PHY_MAX_BW40, /* support 20/40 MHZ */
	CAP_PHY_MAX_BW80, /* support 20/40/80 MHZ */
	CAP_PHY_MAX_BW160, /* support 20/40/80/160 MHZ */
	CAP_PHY_MAX_BW8080, /* support 20/40/80/160/8080 MHZ */
	CAP_PHY_MAX_BW320, /* support 20/40/80/160/8080/320 MHZ */
	CAP_PHY_MAX_BW_NUM,
};

struct CAP_PHY_CAP {
	uint8_t ucHt; /* 1:support, 0:not*/
	uint8_t ucVht; /* 1:support, 0:not*/
	uint8_t uc5gBand; /* 1:support, 0:not*/
	/* 0: BW20, 1:BW40, 2:BW80, 3:BW160, 4:BW80+80, 5:BW320 */
	uint8_t ucMaxBandwidth;
	uint8_t ucNss; /* 1:1x1, 2:2x2, ... */
	uint8_t ucDbdc; /* 1:support, 0:not*/
	uint8_t ucTxLdpc; /* 1:support, 0:not*/
	uint8_t ucRxLdpc; /* 1:support, 0:not*/
	uint8_t ucTxStbc; /* 1:support, 0:not*/
	uint8_t ucRxStbc; /* 1:support, 0:not*/
	/* BIT(0): WLAN_FLAG_2G4_WF0
	 * BIT(1): WLAN_FLAG_5G_WF0
	 * BIT(2): WLAN_FLAG_2G4_WF1
	 * BIT(3): WLAN_FLAG_5G_WF1
	 */
	uint8_t ucWifiPath;
	uint8_t ucHe; /* 1:support, 0:not*/
#if (CFG_SUPPORT_802_11BE == 1)
	uint8_t ucEht; /* 1:support, 0:not*/
#endif
};

struct CAP_LIMITED {
	uint8_t ucLimitedMaxMcsMap2g; /* Limited 2G Max MCS map */
	uint8_t ucLimitedMaxMcsMap5g; /* Limited 5G Max MCS map */
	uint8_t ucLimitedMaxMcsMap6g; /* Limited 6G Max MCS map */
	uint8_t ucReserved[1];
};

enum ENUM_P2P_CAP_BITMAP {
	P2P_CAP_SUPPORT_PNOA = 0,
};

#define P2P_CAP_SUPPORT_PNOA_BIT	BIT(P2P_CAP_SUPPORT_PNOA)

struct CAP_P2P {
	uint32_t u4P2pCap;	/* Bit(0): PNOA, Bits(1-31): Reserved */
};

enum ENUM_PHY_CAP_CNM_BAND_BW {
	ENUM_PHY_CAP_CNM_NOT_SUPP    = 0,
	ENUM_PHY_CAP_CNM_BW_20MHZ    = 1,
	ENUM_PHY_CAP_CNM_BW_40MHZ    = 2,
	ENUM_PHY_CAP_CNM_BW_80MHZ    = 3,
	ENUM_PHY_CAP_CNM_BW_160MHZ   = 4,
	ENUM_PHY_CAP_CNM_BW_80P80MHZ = 5,
	ENUM_PHY_CAP_CNM_BW_5MHZ     = 6,
	ENUM_PHY_CAP_CNM_BW_10MHZ    = 7,
	ENUM_PHY_CAP_CNM_BW_320MHZ   = 8,
	ENUM_PHY_CAP_CNM_BW_NUM
};

struct CAP_CNM_BAND {
	uint8_t ucBandNum;	/* Actually supoorted number of hw band */
	uint8_t aucMaxBw2g[4];
	uint8_t aucMaxBw5g[4];
	uint8_t aucMaxBw6g[4];
	uint8_t aucMaxNss[4];
	uint8_t aucMaxMcs[4];
	uint8_t aucReserved[7];
};

#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
#define RX_QUOTA_MAGIC_NUM 10000
struct CAP_PSE_RX_QUOTA {
	uint32_t u4MaxQuotaBytes;
	uint32_t u4MinQuotaBytes;
};
#endif

struct CAP_MAC_CAP {
	uint8_t ucHwBssIdNum; /* HW BSSID number */
	uint8_t ucWmmSet; /* 1: AC0~3, 2: AC0~3 and AC10~13, ... */
	uint8_t ucWtblEntryNum; /* WTBL entry number */
	uint8_t ucSwBssIdNum; /* SW BssInfo number */
};

struct CAP_FRAME_BUF_CAP {
	/* 1: support in-chip Tx AMSDU (HW or CR4) */
	uint8_t ucChipTxAmsdu;
	/* 2: support 2 MSDU in AMSDU, 3:support 3 MSDU in AMSDU,...*/
	uint8_t ucTxAmsduNum;
	/* Rx AMSDU size, 0:4K, 1:8K,2:12K */
	uint8_t ucRxAmsduSize;
	uint8_t ucReserved;
	/* Txd entry number */
	uint32_t u4TxdCount;
	/* Txd and packet buffer in KB (cut through sysram size) */
	uint32_t u4PacketBufSize;
};

struct CAP_BEAMFORM_CAP {
	uint8_t ucBFer; /* Tx beamformer, 1:support, 0:not*/
	uint8_t ucIBFer; /* Tx implicit beamformer 1:support, 0:not*/
	uint8_t ucBFee; /* Rx beamformee, 1:support, 0:not */
	uint8_t ucReserved;
	uint32_t u4BFerCap; /* Tx beamformere cap */
	uint32_t u4BFeeCap; /* Rx beamformee cap */
};

struct CAP_LOCATION_CAP {
	uint8_t ucTOAE; /* 1:support, 0:not */
	uint8_t aucReserved[3];
};

struct CAP_MUMIMO_CAP {
	uint8_t ucMuMimoRx; /* 1:support, 0:not */
	uint8_t ucMuMimoTx; /* 1:support, 0:not */
	uint8_t aucReserved[2];
};

#if CFG_SUPPORT_ANT_SWAP
struct CAP_ANTSWP {
	uint8_t  ucVersion;
	uint8_t  ucRsvd;
	uint8_t  ucIsSupported;	/* 1:support, 0:not */
	uint8_t  ucReserved[1];
};
#endif

#if CFG_SUPPORT_MLR
struct CAP_MLR_CAP {
	/* Version of MLR feature */
	uint8_t ucVersion;
	uint8_t aucReserved[3];
	/* Bit(0):MLR-v1,
	 * Bit(1):MLR-v2,
	 * Bit(2):MLR+,
	 * Bit(3):ALR,
	 * Bit(4):Dual CTS
	 */
	uint32_t u4MlrSupportBitmap;
};
#endif

struct NIC_HOST_STATUS_EMI_OFFSET {
	uint32_t u4EmiOffset;  /* TRUE for COEX FDD mode */
};

#if (CFG_SUPPORT_WIFI_6G == 1)
struct CAP_6G_CAP {
	uint8_t ucIsSupport6G;  /* 1: Support, 0: Not Support */
	uint8_t ucHwWifiPath;	/* BIT(0): 6G_WF0, BIT(1): 6G_WF1 */
	/* Support DBDC A+A, 1:Enable, 0 Disable */
	uint8_t ucWifiDBDCAwithA;
	/* Minimum Frequency Interval require for DBDC A+A */
	uint8_t ucWifiDBDCAwithAMinimumFrqInterval;

};
#endif

#if CFG_SUPPORT_802_11BE_MLO
struct CAP_MLO_CAP {
	uint8_t ucNonApMldEMLSupport; /* Non-AP Mld EML 1:support, 0:not*/
	uint8_t ucMlrcOpCap; /* Non-AP Ml reconfig op 1:support, 0:not */
	uint8_t ucMaxSimuLinks; /* num of max simultaneous links */
	uint8_t  ucLink3BandLimitBitmap;
	uint16_t u2NonApMldEMLCap; /* Non-AP Mld EML cap */
	uint16_t u2Reserved;
	uint8_t ucNonApHyMloSupport; /* Hybrid MLo 1:support, 0 :not */
	uint8_t ucMlcSupportCap; /* MLC BIT(0): 1:support, 0 :not */
	uint8_t ucReserved[2];
};
#endif

#if (CFG_SUPPORT_PERF_IND == 1)
struct CAP_PERF_IND_FROM_EMI {
	u_int8_t fgPerfIndicatorFromEMIFWSupport;
	uint8_t ucPerfIndicatorFromEMIFWVer;
	uint8_t ucReserved[2];
};
#endif

/**
 * EMI shared memory and the offset of key structure fields.
 *
 * @u4DataEmiOffset: pointer to shared EMI memory, in the structure of
 *			struct STATS_LLS_WIFI_IFACE_STAT x N +
 *			struct PEER_INFO_RATE_STAT +
 *			struct WIFI_RADIO_CHANNEL_STAT.
 * @u4OffsetInfo: info in STATS_LLS_WIFI_IFACE_STAT
 * @u4OffsetAc: ac in STATS_LLS_WIFI_IFACE_STAT
 * @u4OffsetPeerInfo: offset to struct PEER_INFO_RATE_STAT peer_info
 *		      in LLS FW report
 * @u4OffsetRadioStat: offset to struct WIFI_RADIO_CHANNEL_STAT radio
 *		       in LLS FW report
 * @u4OffsetTxTimerPerLevels: tx_time_per_levels in STATS_LLS_WIFI_RADIO_STAT
 * @u4OffsetRxTime: rx_time in STATS_LLS_WIFI_RADIO_STAT
 * @u4OffsetChannel: channel in WIFI_RADIO_CHANNEL_STAT
 *
 * @u4TxTimePerLevelEmiOffset: pointer to shared EMI memory, as array of
 *                             uint32_t of u4NumTxPowerLevels entries
 * @u4NumTxPowerLevels: the size of buffer pointed by u4TxTimePerLevelEmiOffset
 */
struct CAP_LLS_DATA_EMI_OFFSET {
	uint32_t u4DataEmiOffset;
	uint32_t u4OffsetInfo;
	uint32_t u4OffsetAc;
	uint32_t u4OffsetPeerInfo;
	uint32_t u4OffsetRadioStat;
	uint32_t u4OffsetTxTimerPerLevels;
	uint32_t u4OffsetRxTime;
	uint32_t u4OffsetChannel;

	uint32_t u4TxTimePerLevelEmiOffset;
	uint32_t u4NumTxPowerLevels;
};

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
struct CAP_STATS_REG_MONTR_EMI_OFFSET {
	uint32_t u4EmiOffset;
	uint32_t u4OffsetOfBasic;
	uint32_t u4OffsetOfLq;
	uint32_t u4OffsetOfStaStats;
	uint32_t u4OffsetOfLlsStatus;
	uint32_t u4OffsetOfLastTxRateInfo;
};
#endif

#if CFG_SUPPORT_MBRAIN
enum MBRAIN_EMI_OFFSET_TYPE {
    /* modules should add offset define here */
	/*
	 * example.
	 * MBRAIN_EMI_OFFSET_TEST,
	 * MBRAIN_EMI_OFFSET_TEST2,
	 */
#if CFG_SUPPORT_WIFI_ICCM
	MBRAIN_EMI_OFFSET_ICCM,
#endif
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
	MBRAIN_EMI_OFFSET_TXPWR,
#endif
#if CFG_SUPPORT_PCIE_MBRAIN
	MBRAIN_EMI_OFFSET_PCIE,
#endif
#if CFG_SUPPORT_MBRAIN_BIGDATA
	MBRAIN_EMI_OFFSET_BSS_STAT,
	MBRAIN_EMI_OFFSET_ABT_CNT,
	MBRAIN_EMI_OFFSET_PHY_CNT,
	MBRAIN_EMI_OFFSET_STA_INFO,
#endif /* #if CFG_SUPPORT_MBRAIN_BIGDATA */
	MBRAIN_EMI_OFFSET_NUM
};

struct MBRAIN_OFFSET_INFO {
	uint32_t u4Tag;
	uint32_t u4EmiOffset;
};

struct CAP_MBRAIN_EMI_INFO {
	uint32_t u4PcieGenSwRsvd;
	uint32_t u4OffsetNum;
};

#if CFG_SUPPORT_MBRAIN_BIGDATA
struct CAP_MBRAIN_BIGDATA_VER {
	uint32_t u4Ver;
};
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */
#endif

#define EFUSE_SECTION_TABLE_SIZE        (10)   /* It should not be changed. */

struct EFUSE_SECTION_T {
	uint16_t         u2StartOffset;
	uint16_t         u2Length;
};

struct CAP_BUFFER_MODE_INFO_T {
	uint8_t ucVersion; /* Version */
	uint8_t ucFormatSupportBitmap; /* Format Support Bitmap*/
	uint16_t u2EfuseTotalSize; /* Total eFUSE Size */
	struct EFUSE_SECTION_T arSections[EFUSE_SECTION_TABLE_SIZE];
	/* NOTE: EFUSE_SECTION_TABLE_SIZE should be fixed to
	 * 10 so that driver don't change.
	 */
};

struct CAP_CASAN_LOAD_TYPE_T {
	uint32_t u4CasanLoadType;	/* Type of CASAN 0:Normal 1:CASAN */
};

struct CAP_REDL_INFO {
	uint32_t u4RedlOffset;
	uint32_t u4RedlLen;
	uint32_t u4Reserved;
};

enum ENUM_HOST_SUSPEND_ADDR_TYPE {
	ENUM_HOST_SUSPEND_ADDR_TYPE_NONE,
	ENUM_HOST_SUSPEND_ADDR_TYPE_EMI,
	ENUM_HOST_SUSPEND_ADDR_TYPE_CONN_W_R_REG,
	ENUM_HOST_SUSPEND_ADDR_TYPE_CONN_SET_CLR_REG,
	ENUM_HOST_SUSPEND_ADDR_TYPE_NUM
};

struct CAP_HOST_SUSPEND_INFO_T {
	enum ENUM_HOST_SUSPEND_ADDR_TYPE eType;
	uint32_t u4SetAddr;
	uint32_t u4ClrAddr;
	uint32_t u4Mask;
	uint8_t u4Shift;
	uint8_t ucReserved[3];
};

/*
 * NIC_TX_RESOURCE_REPORT_EVENT related definition
 */

#define NIC_TX_RESOURCE_REPORT_VERSION_PREFIX (0x80000000)
#define NIC_TX_RESOURCE_REPORT_VERSION_1 \
	(NIC_TX_RESOURCE_REPORT_VERSION_PREFIX | 0x1)
#define NIC_TX_RESOURCE_REPORT_VERSION_2 \
	(NIC_TX_RESOURCE_REPORT_VERSION_PREFIX | 0x2)

struct nicTxRsrcEvtHdlr {
	uint32_t u4Version;

	uint32_t(*nicEventTxResource)
		(struct ADAPTER *prAdapter,
		 uint8_t *pucEventBuf);
	void (*nicTxResourceInit)(struct ADAPTER *prAdapter);
};

struct NIC_TX_RESOURCE {
	/* the total usable resource for MCU port */
	uint32_t u4CmdTotalResource;
	/* the unit of a MCU resource */
	uint32_t u4CmdResourceUnit;
	/* the total usable resource for LMAC port */
	uint32_t u4DataTotalResource;
	/* the unit of a LMAC resource */
	uint32_t u4DataResourceUnit;
};

struct tx_resource_report_v1 {
	/*
	 * u4Version: NIC_TX_RESOURCE_REPORT_VERSION_1
	 */
	uint32_t u4Version;

	/*
	 * the followings are content for u4Verion = 0x80000001
	 */
	uint32_t u4HifDataPsePageQuota;
	uint32_t u4HifCmdPsePageQuota;
	uint32_t u4HifDataPlePageQuota;
	uint32_t u4HifCmdPlePageQuota;

	/*
	 * u4PlePsePageSize: the unit of a page in PLE and PSE
	 * [31:16] PLE page size
	 * [15:0] PLE page size
	 */
	uint32_t u4PlePsePageSize;

	/*
	 * ucPpTxAddCnt: the extra pse resource needed by HW
	 */
	uint8_t ucPpTxAddCnt;

	uint8_t ucReserved[3];
};


/* modified version of WLAN_BEACON_FRAME_BODY_T for simplier buffering */
struct WLAN_BEACON_FRAME_BODY_T_LOCAL {
	/* Beacon frame body */
	uint32_t au4Timestamp[2];	/* Timestamp */
	uint16_t u2BeaconInterval;	/* Beacon Interval */
	uint16_t u2CapInfo;	/* Capability */
	/* Various IEs, start from SSID */
	uint8_t aucInfoElem[MAX_IE_LENGTH];
	/* This field is *NOT* carried by F/W but caculated by nic_rx */
	uint16_t u2IELength;
};

/* EVENT_SCAN_RESULT */
struct EVENT_SCAN_RESULT {
	int32_t i4RSSI;
	uint32_t u4LinkQuality;
	uint32_t u4DSConfig;	/* Center frequency */
	uint32_t u4DomainInfo;	/* Require CM opinion */
	uint32_t u4Reserved;
	uint8_t ucNetworkType;
	uint8_t ucOpMode;
	uint8_t aucBssid[MAC_ADDR_LEN];
	uint8_t aucRatesEx[PARAM_MAX_LEN_RATES_EX];
	struct WLAN_BEACON_FRAME_BODY_T_LOCAL rBeaconFrameBody;
};

struct EVENT_PMKID_CANDIDATE_LIST {
	uint32_t u4Version;	/*!< Version */
	uint32_t u4NumCandidates;	/*!< How many candidates follow */
	struct PARAM_PMKID_CANDIDATE arCandidateList[1];
};

struct EVENT_CMD_RESULT {
	uint8_t ucCmdID;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
};

#if CFG_SUPPORT_WIFI_ICCM
struct CMD_ICCM_INFO_T {
	uint8_t u4Enable;
	uint8_t u4EnablePrintFw;
	uint32_t u4Value;
};
#endif

#if CFG_SUPPORT_WIFI_POWER_METRICS
struct CMD_POWER_METRICS_INFO_T {
	uint32_t u4Enable;
	uint32_t u4Value;
};

struct POWER_STATE_INFO {
	uint32_t u4SleepTime;
	uint32_t u4RxListenTime;
	uint32_t u4TxTime;
	uint32_t u4RxTime;
};

struct EVENT_POWER_METRICS_INFO_T {
	uint32_t u4TotalTime;
	uint32_t u4Band;
	uint32_t u4Protocol;
	uint32_t u4Nss[2];
	struct POWER_STATE_INFO u4BandRatio;
	uint32_t arStatsPmCckRateStat[4];
	uint32_t arStatsPmOfdmRateStat[8];
	uint32_t arStatsPmHtRateStat[32];
	uint32_t arStatsPmVhtRateStat[30];
	uint32_t arStatsPmHeRateStat[48];
	uint32_t arStatsPmEhtRateStat[80];
};
#endif

/* CMD_ID_ACCESS_REG & EVENT_ID_ACCESS_REG */
struct CMD_ACCESS_REG {
	uint32_t u4Address;
	uint32_t u4Data;
};

/* CMD_ID_SET_MDVT */
struct CMD_MDVT_CFG {
	uint32_t u4ModuleId;
	uint32_t u4CaseId;
	uint8_t ucCapId;
	uint8_t ucReserved[3];
};

#define MAX_ATXOP_PARAM_NUM 32

/* CMD_ID_SET_ATXOP */
struct CMD_ATXOP_CFG {
	uint32_t u4Cmd;
	uint32_t au4Param[MAX_ATXOP_PARAM_NUM];
};

#define COEX_CTRL_BUF_LEN 460
#define COEX_INFO_LEN 115

/* CMD_COEX_HANDLER & EVENT_COEX_HANDLER */
/************************************************/
/*  UINT_32 u4SubCmd : Coex Ctrl Sub Command    */
/*  UINT_8 aucBuffer : Reserve for Sub Command  */
/*                    Data Structure            */
/************************************************/
struct COEX_CMD_HANDLER {
	uint32_t u4SubCmd;
	uint8_t aucBuffer[COEX_CTRL_BUF_LEN];
};

#if (CFG_WIFI_ISO_DETECT == 1)
/* Sub Command Data Structure */
/************************************************/
/*  UINT_32 u4IsoPath : BITS[7:0]:WF Path (WF0/WF1)*/
/*                      BITS[15:8]:BT Path (BT0/BT1)*/
/*  UINT_32 u4Channel : WF Channel*/
/*  UINT_32 u4Isolation  : Isolation value*/
/************************************************/
struct COEX_CMD_ISO_DETECT {
	uint32_t u4IsoPath;
	uint32_t u4Channel;
	uint32_t u4Isolation;
};
#endif
/************************************************/
/*  PCHAR   pucCoexInfo : CoexInfoTag           */
/************************************************/
struct CMD_COEX_GET_INFO {
	uint32_t   u4CoexInfo[COEX_INFO_LEN];
};

/* Coex Command Used  */
enum ENUM_COEX_CMD_CTRL {
	/* Set */
	COEX_CMD_SET_RX_DATA_INFO = 0x00,
	/* Get */
	COEX_CMD_GET_ISO_DETECT = 0x80,
	COEX_CMD_GET_INFO = 0x81,
	COEX_CMD_NUM
};

struct CMD_ACCESS_CHN_LOAD {
	uint32_t u4Address;
	uint32_t u4Data;
	uint16_t u2Channel;
	uint8_t aucReserved[2];
};

struct CMD_GET_LTE_SAFE_CHN {
	uint8_t ucIndex;
	uint8_t ucFlags;
	uint8_t aucReserved0[2];
	uint8_t aucReserved2[16];
};

/* CMD_DUMP_MEMORY */
struct CMD_DUMP_MEM {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
	uint32_t u4IcapContent;
#endif				/* CFG_SUPPORT_QA_TOOL */
	uint8_t ucFragNum;
};

struct EVENT_DUMP_MEM {
	uint32_t u4Address;
	uint32_t u4Length;
	uint32_t u4RemainLength;
#if CFG_SUPPORT_QA_TOOL
	uint32_t eIcapContent;
#endif				/* CFG_SUPPORT_QA_TOOL */
	uint8_t ucFragNum;
	uint8_t aucBuffer[];
};

#define CMD_DEVINFO_UPDATE_HDR_SIZE 8
struct CMD_DEV_INFO_UPDATE {
	uint8_t ucOwnMacIdx;
	uint8_t ucDbdcIdx;
	uint16_t u2TotalElementNum;
	uint8_t ucAppendCmdTLV;
	uint8_t aucReserve[3];
	uint8_t aucBuffer[];
	/* CMD_DEVINFO_ACTIVE_T rCmdDevInfoActive; */
};

#define CMD_BSSINFO_UPDATE_HDR_SIZE 8
struct CMD_BSS_INFO_UPDATE {
	uint8_t ucBssIndex;
	uint8_t ucReserve;
	uint16_t u2TotalElementNum;
	uint32_t u4Reserve;
	/* CMD_BSSINFO_BASIC_T rCmdBssInfoBasic; */
	uint8_t aucBuffer[];
};

/*  STA record command */
#define CMD_STAREC_UPDATE_HDR_SIZE 8
struct CMD_STAREC_UPDATE {
	uint8_t ucBssIndex;
	uint8_t ucWlanIdx;
	uint16_t u2TotalElementNum;
	uint32_t u4Reserve;
	uint8_t aucBuffer[];
};

#if CFG_SUPPORT_TX_BF
union CMD_TXBF_ACTION {
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

struct EXT_EVENT_BF_STATUS_T {
	uint8_t ucEventCategoryID;
	uint8_t ucBw;
	uint16_t u2SubCarrIdx;
	u_int8_t fgBFer;
	uint8_t aucReserved[3];
	uint8_t aucBuf[1000]; /* temp size */
};

struct EVENT_PFMU_TAG_READ {
	union PFMU_PROFILE_TAG1 ru4TxBfPFMUTag1;
	union PFMU_PROFILE_TAG2 ru4TxBfPFMUTag2;
};

#endif

#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
struct CMD_ACCESS_RX_STAT {
	uint32_t u4SeqNum;
	uint32_t u4TotalNum;
};

struct EVENT_ACCESS_RX_STAT {
	uint32_t u4SeqNum;
	uint32_t u4TotalNum;
	uint32_t au4Buffer[];
};

#else
struct CMD_ACCESS_RX_STAT {
	uint16_t u2SeqNum;
	uint8_t ucDbdcIdx;
	/* bit[0] in event structure will tell new / old firmware format */
	/* bit[4] in event structure will tell precise_rssi */
	uint8_t ucData;
	uint32_t u4TotalNum;
};

struct EVENT_ACCESS_RX_STAT {
	uint16_t u2SeqNum;
	uint8_t ucDbdcIdx;
	/* bit[0] in event structure will tell new / old firmware format */
	uint8_t	ucData;
	uint32_t u4TotalNum;
	uint32_t au4Buffer[];
};
#endif

#if CFG_SUPPORT_QA_TOOL
#if CFG_SUPPORT_MU_MIMO
struct EVENT_HQA_GET_QD {
	uint32_t u4EventId;
	uint32_t au4RawData[14];
};

struct EVENT_HQA_GET_MU_CALC_LQ {
	uint32_t u4EventId;
	struct MU_STRUCT_LQ_REPORT rEntry;
};

struct EVENT_SHOW_GROUP_TBL_ENTRY {
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

union CMD_MUMIMO_ACTION {
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
#endif /* CFG_SUPPORT_QA_TOOL */

struct CMD_SW_DBG_CTRL {
	uint32_t u4Id;
	uint32_t u4Data;
	/* Debug Support */
	uint32_t u4DebugCnt[64];
};

struct CMD_FW_LOG_2_HOST_CTRL {
	uint8_t ucFwLog2HostCtrl;
	uint8_t ucMcuDest;
	uint8_t ucReserve[2];
};

struct CMD_CHIP_CONFIG {
	uint16_t u2Id;
	uint8_t ucType;
	uint8_t ucRespType;
	uint16_t u2MsgSize;
	uint8_t aucReserved0[2];
	uint8_t aucCmd[CHIP_CONFIG_RESP_SIZE];
};

/* CMD_ID_LINK_ATTRIB */
struct CMD_LINK_ATTRIB {
	int8_t cRssiTrigger;
	uint8_t ucDesiredRateLen;
	uint16_t u2DesiredRate[32];
	uint8_t ucMediaStreamMode;
	uint8_t aucReserved[1];
};

/* CMD_ID_NIC_POWER_CTRL */
struct CMD_NIC_POWER_CTRL {
	uint8_t ucPowerMode;
	uint8_t aucReserved[3];
};

/* CMD_ID_POWER_SAVE_MODE */
struct CMD_PS_PROFILE {
	uint8_t ucBssIndex;
	uint8_t ucPsProfile;
	uint8_t aucReserved[2];
};

#if (CFG_SUPPORT_TSF_SYNC == 1)
struct CMD_TSF_SYNC {
	/* DWORD_0 - Common Part */
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;       /* cmd size including common part and body. */

	/* DWORD_1 ~ x - Command Body */
	uint64_t u8TsfValue;
	uint8_t fgIsLatch;
	uint8_t ucBssIndex;
	uint8_t aucReserved[2];
};
#endif

#if CFG_SUPPORT_P2P_RSSI_QUERY
/* EVENT_LINK_QUALITY */
struct EVENT_LINK_QUALITY_EX {
	int8_t cRssi;
	int8_t cLinkQuality;
	uint16_t u2LinkSpeed;
	uint8_t ucMediumBusyPercentage;
	uint8_t ucIsLQ0Rdy;
	int8_t cRssiP2P;		/* For P2P Network. */
	int8_t cLinkQualityP2P;
	uint16_t u2LinkSpeedP2P;
	uint8_t ucMediumBusyPercentageP2P;
	uint8_t ucIsLQ1Rdy;
};
#endif

/* EVENT_ID_FW_SLEEPY_NOTIFY */
struct EVENT_SLEEPY_INFO {
	uint8_t ucSleepyState;
	uint8_t aucReserved[3];
};

/* CMD_BT_OVER_WIFI */
struct CMD_BT_OVER_WIFI {
	uint8_t ucAction;	/* 0: query, 1: setup, 2: destroy */
	uint8_t ucChannelNum;
	uint8_t rPeerAddr[PARAM_MAC_ADDR_LEN];
	uint16_t u2BeaconInterval;
	uint8_t ucTimeoutDiscovery;
	uint8_t ucTimeoutInactivity;
	uint8_t ucRole;
	uint8_t PAL_Capabilities;
	uint8_t cMaxTxPower;
	uint8_t ucChannelBand;
	uint8_t ucReserved[1];
};

enum ENUM_REG_DOMAIN {
	ENUM_RDM_CE = 0,
	ENUM_RDM_FCC,
	ENUM_RDM_JAP,
	ENUM_RDM_JAP_W53,
	ENUM_RDM_JAP_W56,
	ENUM_RDM_CHN,
	ENUM_RDM_KR,
	ENUM_RDM_REGION_NUM
};

#if (CFG_SUPPORT_DFS_MASTER == 1)

struct CMD_RDD_ON_OFF_CTRL {
	uint8_t ucDfsCtrl;
	uint8_t ucRddIdx;
	uint8_t ucRddRxSel;
	uint8_t ucSetVal;
	uint8_t ucBssIdx;
	uint8_t ucCacLPEn;
	uint8_t aucReserve[2];
};
#endif

struct CMD_SET_ACL_POLICY {
	uint8_t ucBssIdx;
	uint8_t ucPolicy;
	uint8_t aucAddr[MAC_ADDR_LEN];
	uint8_t aucReserve[4];
};
struct CMD_PERF_IND_PARM {
	uint32_t u4CurTxBytes;    /* in Bps */
	uint32_t u4CurRxBytes;    /* in Bps */
	uint16_t u2CurRxRate;    /* Unit 500 Kbps */
	uint8_t ucCurRxRCPI0;
	uint8_t ucCurRxRCPI1;
	uint8_t ucCurRxNss;
	uint8_t ucCurRxNss2;
	uint16_t u2Reserve;
};
struct CMD_PERF_IND {
	/* DWORD_0 - Common Part */
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;       /* cmd size including common part and body. */
	/* DWORD_1 ~ x - Command Body */
	uint32_t u4VaildPeriod;   /* in ms */
	uint8_t ucBssNum;
	uint8_t  ucReserve[3];
	struct CMD_PERF_IND_PARM rUniCmdParm[MAX_BSSID_NUM];
	uint32_t u4WtblBitMap;
	//uint32_t au4Reserve[62];
};

#if CFG_SUPPORT_SMART_GEAR
struct CMD_SMART_GEAR_PARAM {
	uint8_t ucSGEnable;/*0: disable, 1: Enable, 2: Action for ucSGSpcCmd*/
	uint8_t ucSGSpcCmd;/* 1: Force 1x1, 2: Force 2x2, 0: Automatic*/
	uint8_t ucNSSCap; /*NSS Capbility*/
	uint8_t ucSGCfg; /* DON'T SET, IT IS USED FOR WIFI.CFG WIFI*/
	/* DON'T SET, IT IS USED FOR WIFI.CFG WIFI 2.4G Favor ANT path*/
	uint8_t ucSG24GFavorANT;
	/* DON'T SET, IT IS USED FOR WIFI.CFG WIFI 5G Favor ANT path*/
	uint8_t ucSG5GFavorANT;
};

struct EVENT_SMART_GEAT_STATE {
	bool fgIsEnable;
	uint32_t  u4StateIdx;
};
#endif

/* EVENT_BT_OVER_WIFI */
struct EVENT_BT_OVER_WIFI {
	uint8_t ucLinkStatus;
	uint8_t ucSelectedChannel;
	int8_t cRSSI;
	uint8_t ucReserved[1];
};

/* Same with DOMAIN_SUBBAND_INFO */
struct CMD_SUBBAND_INFO {
	uint8_t ucRegClass;
	uint8_t ucBand;
	uint8_t ucChannelSpan;
	uint8_t ucFirstChannelNum;
	uint8_t ucNumChannels;
	uint8_t fgDfs;         /* Type: BOOLEAN (fgDfsNeeded) */
	uint8_t aucReserved[2];
};

/* CMD_SET_DOMAIN_INFO */
struct CMD_SET_DOMAIN_INFO {
	uint16_t u2CountryCode;
	uint16_t u2IsSetPassiveScan;
	struct CMD_SUBBAND_INFO rSubBand[MAX_SUBBAND_NUM];

	uint8_t uc2G4Bandwidth;	/* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t uc5GBandwidth;	/* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t ucRegCat;
	uint8_t aucReserved[1];
};

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG

#define POWER_LIMIT_ANT_CONFIG_NUM 60

struct CMD_CHANNEL_POWER_LIMIT_ANT {

	int8_t cTagIdx;  /* -1 means end */
	int8_t cBandIdx;
	int8_t cAntIdx;
	int8_t cValue;
};

#endif

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
struct CMD_CHANNEL_POWER_LIMIT_TX_PWR_ENV {
	uint8_t fgPwrLmtEnable;
	uint8_t ucBand;
	uint8_t ucPriCh;
	uint8_t ucPwrLmtNum;
	int8_t aicMaxTxPwrLmt[TX_PWR_ENV_MAX_TXPWR_BW_NUM];
};
#endif

struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT {
    /* D-WORD 0 */
	uint16_t u2CountryCode;
	uint8_t ucCategoryId;
	uint8_t ucNum; /*Numbers of channel to set power limit*/

	/* D-WORD 1 */
	u_int8_t fgPwrTblKeep;
	uint8_t ucBandIdx;
	/* u1LimitType: enum ENUM_COUNTRY_CHANNEL_TXPOWER_LIMIT_FORMAT */
	uint8_t ucLimitType;
	uint8_t ucVersion;

	union {

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
		/*Channel power limit entries to be set*/
		struct CMD_CHANNEL_POWER_LIMIT
			rChannelPowerLimit[MAX_CMD_SUPPORT_CHANNEL_NUM];
		/*Channel HE power limit entries to be set*/
		struct CMD_CHANNEL_POWER_LIMIT_HE
			rChPwrLimtHE[MAX_CMD_SUPPORT_CHANNEL_NUM];
		/*Channel HE BW160 power limit entries to be set*/
		struct CMD_CHANNEL_POWER_LIMIT_HE_BW160
			rChPwrLimtHEBW160[MAX_CMD_SUPPORT_CHANNEL_NUM];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		struct CMD_CHANNEL_POWER_LIMIT_EHT
			rChPwrLimtEHT[MAX_CMD_EHT_SUPPORT_CHANNEL_NUM];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
		struct CMD_CHANNEL_POWER_LIMIT_6E
			rChPwrLimt6E[MAX_CMD_SUPPORT_CHANNEL_NUM];
		struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G
			rChPwrLimtLegacy_6G[MAX_CMD_SUPPORT_CHANNEL_NUM];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		struct CMD_CHANNEL_POWER_LIMIT_EHT_6G
			rChPwrLimtEHT_6G[MAX_CMD_EHT_6G_SUPPORT_CHANNEL_NUM];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /*if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
		struct CMD_CHANNEL_POWER_LIMIT_ANT
			rChPwrLimtAnt[POWER_LIMIT_ANT_CONFIG_NUM];
#endif
#if (CFG_SUPPORT_TX_PWR_ENV == 1)
		struct CMD_CHANNEL_POWER_LIMIT_TX_PWR_ENV rTxPwrEnvPwrLmt;
#endif
	} u;
};

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
struct CMD_EMI_POWER_LIMIT_FORMAT {
	uint8_t u1RFBandNum;
	uint8_t u1ProtocolNum;
	uint8_t u1ApplyMethod;
	uint8_t u1ScenarioType;
	uint8_t u1MultiBandVer;
	uint8_t u1MultiBandNum;
	uint8_t u1MultiBandSize;
	uint8_t u1reserve[1];
	struct EMI_POWER_LIMIT_INFO
		rTxpwrEmiInfo[PWR_LIMIT_RF_BAND_NUM][PWR_LIMIT_PROTOCOL_NUM];
};
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1*/

#if (CFG_SUPPORT_SINGLE_SKU == 1)
struct CMD_CHANNEL_POWER_LIMIT_V2 {
	uint8_t ucCentralCh;
	uint8_t ucReserved[3];

	uint8_t tx_pwr_dsss_cck;
	uint8_t tx_pwr_dsss_bpsk;

	uint8_t tx_pwr_ofdm_bpsk; /* 6M, 9M */
	uint8_t tx_pwr_ofdm_qpsk; /* 12M, 18M */
	uint8_t tx_pwr_ofdm_16qam; /* 24M, 36M */
	uint8_t tx_pwr_ofdm_48m;
	uint8_t tx_pwr_ofdm_54m;

	uint8_t tx_pwr_ht20_bpsk; /* MCS0*/
	uint8_t tx_pwr_ht20_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_ht20_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_ht20_mcs5; /* MCS5*/
	uint8_t tx_pwr_ht20_mcs6; /* MCS6*/
	uint8_t tx_pwr_ht20_mcs7; /* MCS7*/

	uint8_t tx_pwr_ht40_bpsk; /* MCS0*/
	uint8_t tx_pwr_ht40_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_ht40_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_ht40_mcs5; /* MCS5*/
	uint8_t tx_pwr_ht40_mcs6; /* MCS6*/
	uint8_t tx_pwr_ht40_mcs7; /* MCS7*/
	uint8_t tx_pwr_ht40_mcs32; /* MCS32*/

	uint8_t tx_pwr_vht20_bpsk; /* MCS0*/
	uint8_t tx_pwr_vht20_qpsk; /* MCS1, MCS2*/
	uint8_t tx_pwr_vht20_16qam; /* MCS3, MCS4*/
	uint8_t tx_pwr_vht20_64qam; /* MCS5, MCS6*/
	uint8_t tx_pwr_vht20_mcs7;
	uint8_t tx_pwr_vht20_mcs8;
	uint8_t tx_pwr_vht20_mcs9;

	uint8_t tx_pwr_vht_40;
	uint8_t tx_pwr_vht_80;
	uint8_t tx_pwr_vht_160c;
	uint8_t tx_pwr_vht_160nc;
	uint8_t tx_pwr_lg_40;
	uint8_t tx_pwr_lg_80;

	uint8_t tx_pwr_1ss_delta;
	uint8_t ucReserved_1[2];
};

struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2 {
	uint8_t ucNum;
	uint8_t eband; /*ENUM_BAND_T*/
	uint8_t usReserved[2];
	uint32_t countryCode;
	struct CMD_CHANNEL_POWER_LIMIT_V2 rChannelPowerLimit[];
};

#define BF_TX_PWR_LIMIT_SECTION_NUM 17
#define BF_TX_PWR_LIMIT_ELEMENT_NUM 10

#define TX_PWR_LIMIT_SECTION_NUM 35
#define TX_PWR_LIMIT_ELEMENT_NUM 16

#define TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN 4
#define TX_PWR_LIMIT_MAX_VAL 63

#define POWER_LIMIT_SKU_CCK_NUM 4
#define POWER_LIMIT_SKU_OFDM_NUM 8
#define POWER_LIMIT_SKU_HT20_NUM 8
#define POWER_LIMIT_SKU_HT40_NUM 9
#define POWER_LIMIT_SKU_VHT20_NUM 10
#define POWER_LIMIT_SKU_VHT40_NUM 10
#define POWER_LIMIT_SKU_VHT80_NUM 10
#define POWER_LIMIT_SKU_VHT160_NUM 10
#define POWER_LIMIT_SKU_VHT20_2_NUM 12
#define POWER_LIMIT_SKU_VHT40_2_NUM 12
#define POWER_LIMIT_SKU_VHT80_2_NUM 12
#define POWER_LIMIT_SKU_VHT160_2_NUM 12


#define POWER_LIMIT_SKU_RU26_NUM 12
#define POWER_LIMIT_SKU_RU52_NUM 12
#define POWER_LIMIT_SKU_RU106_NUM 12
#define POWER_LIMIT_SKU_RU242_NUM 12
#define POWER_LIMIT_SKU_RU484_NUM 12
#define POWER_LIMIT_SKU_RU996_NUM 12
#define POWER_LIMIT_SKU_RU996X2_NUM 12

#define POWER_LIMIT_SKU_EHT26_NUM 16
#define POWER_LIMIT_SKU_EHT52_NUM 16
#define POWER_LIMIT_SKU_EHT106_NUM 16
#define POWER_LIMIT_SKU_EHT242_NUM 16
#define POWER_LIMIT_SKU_EHT484_NUM 16
#define POWER_LIMIT_SKU_EHT996_NUM 16
#define POWER_LIMIT_SKU_EHT996X2_NUM 16
#define POWER_LIMIT_SKU_EHT26_52_NUM 16
#define POWER_LIMIT_SKU_EHT26_106_NUM 16
#define POWER_LIMIT_SKU_EHT484_242_NUM 16
#define POWER_LIMIT_SKU_EHT996_484_NUM 16
#define POWER_LIMIT_SKU_EHT996_484_242_NUM 16
#define POWER_LIMIT_SKU_EHT996X2_484_NUM 16
#define POWER_LIMIT_SKU_EHT996X3_NUM 16
#define POWER_LIMIT_SKU_EHT996X3_484_NUM 16
#define POWER_LIMIT_SKU_EHT996X4_NUM 16


struct CHANNEL_TX_PWR_LIMIT {
	uint8_t ucChannel;
	int8_t rTxPwrLimitValue[TX_PWR_LIMIT_SECTION_NUM]
		[TX_PWR_LIMIT_ELEMENT_NUM];
	int8_t rTxBfBackoff[POWER_LIMIT_TXBF_BACKOFF_PARAM_NUM];
};

struct TX_PWR_LIMIT_DATA {
	uint32_t countryCode;
	uint32_t ucChNum;
	struct CHANNEL_TX_PWR_LIMIT *rChannelTxPwrLimit;
};

#endif

#endif

struct PATTERN_DESCRIPTION {
	uint8_t fgCheckBcA1;
	uint8_t fgCheckMcA1;
	uint8_t ePatternHeader;
	uint8_t fgAndOp;
	uint8_t fgNotOp;
	uint8_t ucPatternMask;
	uint16_t u2PatternOffset;
	uint8_t aucPattern[8];
};

struct CMD_RAW_PATTERN_CONFIGURATION {
	struct PATTERN_DESCRIPTION arPatternDesc[4];
};

struct CMD_PATTERN_FUNC_CONFIG {
	u_int8_t fgBcA1En;
	u_int8_t fgMcA1En;
	u_int8_t fgBcA1MatchDrop;
	u_int8_t fgMcA1MatchDrop;
};

struct EVENT_TX_DONE {
	uint8_t ucPacketSeq; /* Match ucPID in MSDU_INFO */
	uint8_t ucStatus;
	uint16_t u2SequenceNumber; /* L2 SN */

	uint8_t ucWlanIndex;
	uint8_t ucTxCount;
	uint16_t u2TxRate;

	uint8_t ucFlag;
	uint8_t ucTid; /* Match ucUserPriority in MSDU_INFO */
	uint8_t ucRspRate;
	uint8_t ucRateTableIdx;

	uint8_t ucBandwidth;
	uint8_t ucTxPower;
	uint8_t ucFlushReason;
	uint8_t aucReserved0[1];

	uint32_t u4TxDelay;
	uint32_t u4Timestamp;
	uint32_t u4AppliedFlag;

	uint8_t aucRawTxS[28];

	uint8_t aucReserved1[32];
};

enum ENUM_TXS_APPLIED_FLAG {
	TX_FRAME_IN_AMPDU_FORMAT = 0,
	TX_FRAME_EXP_BF,
	TX_FRAME_IMP_BF,
	TX_FRAME_PS_BIT
};

enum ENUM_TXS_CONTROL_FLAG {
	TXS_WITH_ADVANCED_INFO = 0,
	TXS_IS_EXIST
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
enum ENUM_DFS_CTRL {
	RDD_STOP = 0,
	RDD_START,
	RDD_DET_MODE,
	RDD_RADAR_EMULATE,
	TESTMODE_RDD_STOP,
	TESTMODE_RDD_START,
	RDD_START_TXQ = 20
};
#endif

struct CMD_BSS_ACTIVATE_CTRL {
	uint8_t ucBssIndex;
	uint8_t ucActive;
	uint8_t ucNetworkType;
	uint8_t ucOwnMacAddrIndex;
	uint8_t aucBssMacAddr[6];
	uint8_t ucBMCWlanIndex;
	uint8_t ucMldLinkIdx;
};

enum ENUM_RTS_POLICY {
	RTS_POLICY_AUTO,
	RTS_POLICY_STATIC_BW,
	RTS_POLICY_DYNAMIC_BW,
	RTS_POLICY_LEGACY,
	RTS_POLICY_NO_RTS
};

enum _ENUM_CMD_UPDATE_STA_RECORD_VER_T {
	CMD_UPDATE_STAREC_VER0 = 0,
	CMD_UPDATE_STAREC_VER1,
	CMD_UPDATE_STAREC_VER_MAX
};

struct CMD_BEACON_TEMPLATE_UPDATE {
	/* 0: update randomly,
	 * 1: update all,
	 * 2: delete all (1 and 2 will update directly without search)
	 */
	uint8_t ucUpdateMethod;
	uint8_t ucBssIndex;
	uint8_t aucReserved[2];
	uint16_t u2Capability;
	uint16_t u2IELen;
	uint8_t aucIE[MAX_IE_LENGTH];
};

struct GSCN_CHANNEL_INFO {
	uint8_t ucBand;
	uint8_t ucChannelNumber; /* Channel Number */
	uint8_t ucPassive;       /* 0:active, 1:passive scan; ignored for DFS */
	uint8_t aucReserved[1];
	uint32_t u4DwellTimeMs;  /* dwell time hint */
};

enum ENUM_WIFI_BAND {
	WIFI_BAND_UNSPECIFIED,
	WIFI_BAND_BG = 1,              /* 2.4 GHz */
	WIFI_BAND_A = 2,               /* 5 GHz without DFS */
	WIFI_BAND_A_DFS = 4,           /* 5 GHz DFS only */
	WIFI_BAND_A_WITH_DFS = 6,      /* 5 GHz with DFS */
	WIFI_BAND_ABG = 3,             /* 2.4 GHz + 5 GHz; no DFS */
	WIFI_BAND_ABG_WITH_DFS = 7,    /* 2.4 GHz + 5 GHz with DFS */
};

struct GSCAN_BUCKET {
	uint16_t u2BucketIndex;  /* bucket index, 0 based */
	/* desired period, in millisecond;
	 * if this is too low, the firmware should choose to generate
	 * results as fast as it can instead of failing the command
	 */
	uint8_t ucBucketFreqMultiple;
	/* report_events semantics -
	 *  This is a bit field; which defines following bits -
	 *  REPORT_EVENTS_EACH_SCAN
	 *      report a scan completion event after scan. If this is not set
	 *      then scan completion events should be reported if
	 *      report_threshold_percent or report_threshold_num_scans is
	 *				    reached.
	 *  REPORT_EVENTS_FULL_RESULTS
	 *      forward scan results (beacons/probe responses + IEs)
	 *      in real time to HAL, in addition to completion events
	 *      Note: To keep backward compatibility, fire completion
	 *	events regardless of REPORT_EVENTS_EACH_SCAN.
	 *  REPORT_EVENTS_NO_BATCH
	 *      controls if scans for this bucket should be placed in the
	 *      history buffer
	 */
	uint8_t ucReportFlag;
	uint8_t ucMaxBucketFreqMultiple; /* max_period / base_period */
	uint8_t ucStepCount;
	uint8_t ucNumChannels;
	uint8_t aucReserved[1];
	enum ENUM_WIFI_BAND
	eBand; /* when UNSPECIFIED, use channel list */
	/* channels to scan; these may include DFS channels */
	struct GSCN_CHANNEL_INFO arChannelList[8];
};

struct CMD_GSCN_REQ {
	uint8_t ucFlags;
	uint8_t ucNumScnToCache;
	uint8_t aucReserved[2];
	uint32_t u4BufferThreshold;
	uint32_t u4BasePeriod; /* base timer period in ms */
	uint32_t u4NumBuckets;
	/* number of APs to store in each scan in the */
	uint32_t u4MaxApPerScan;
	/* BSSID/RSSI history buffer (keep the highest RSSI APs) */
	struct GSCAN_BUCKET arBucket[8];
};

struct CMD_GSCN_SCN_COFIG {
	uint8_t ucNumApPerScn;       /* GSCAN_ATTRIBUTE_NUM_AP_PER_SCAN */
	uint32_t u4BufferThreshold;  /* GSCAN_ATTRIBUTE_REPORT_THRESHOLD */
	uint32_t u4NumScnToCache;    /* GSCAN_ATTRIBUTE_NUM_SCANS_TO_CACHE */
};

/* 20150107  Daniel Added complete channels number in the scan done event */
/* before*/
/*
 * typedef struct EVENT_SCAN_DONE {
 *	UINT_8          ucSeqNum;
 *	UINT_8          ucSparseChannelValid;
 *	CHANNEL_INFO_T  rSparseChannel;
 * } EVENT_SCAN_DONE, *P_EVENT_SCAN_DONE;
 */
/* after */

struct CMD_BATCH_REQ {
	uint8_t ucSeqNum;
	uint8_t ucNetTypeIndex;
	uint8_t ucCmd;		/* Start/ Stop */
	/* an integer number of scans per batch */
	uint8_t ucMScan;
	/* an integer number of the max AP to remember per scan */
	uint8_t ucBestn;
	/* an integer number of highest-strength AP for which we'd */
	uint8_t ucRtt;
	/* like approximate distance reported */
	uint8_t ucChannel;	/* channels */
	uint8_t ucChannelType;
	uint8_t ucChannelListNum;
	uint8_t aucReserved[3];
	uint32_t u4Scanfreq;	/* an integer number of seconds between scans */
	struct CHANNEL_INFO arChannelList[32];	/* channels */
};

struct CMD_CH_PRIVILEGE {
	uint8_t ucBssIndex;
	uint8_t ucTokenID;
	uint8_t ucAction;
	uint8_t ucPrimaryChannel;
	uint8_t ucRfSco;
	uint8_t ucRfBand;
	uint8_t ucRfChannelWidth;	/* To support 80/160MHz bandwidth */
	/* Means CCFS0 for spec, means center freq for FW */
	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */
	uint8_t ucReqType;
	uint8_t ucDBDCBand;
	uint8_t aucReserved;
	uint32_t u4MaxInterval;	/* In unit of ms */
	uint8_t aucReserved2[8];
};

struct CMD_TX_PWR {
	int8_t cTxPwr2G4Cck;	/* signed, in unit of 0.5dBm */
	int8_t cTxPwr2G4Dsss;	/* signed, in unit of 0.5dBm */
	int8_t acReserved[2];

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

struct CMD_TX_AC_PWR {
	int8_t ucBand;
#if 0
	int8_t c11AcTxPwr_BPSK;
	int8_t c11AcTxPwr_QPSK;
	int8_t c11AcTxPwr_16QAM;
	int8_t c11AcTxPwr_MCS5_MCS6;
	int8_t c11AcTxPwr_MCS7;
	int8_t c11AcTxPwr_MCS8;
	int8_t c11AcTxPwr_MCS9;
	int8_t c11AcTxPwrVht40_OFFSET;
	int8_t c11AcTxPwrVht80_OFFSET;
	int8_t c11AcTxPwrVht160_OFFSET;
#else
	struct AC_PWR_SETTING_STRUCT rAcPwr;
#endif
};

struct CMD_RSSI_PATH_COMPASATION {
	int8_t c2GRssiCompensation;
	int8_t c5GRssiCompensation;
};
struct CMD_5G_PWR_OFFSET {
	int8_t cOffsetBand0;	/* 4.915-4.980G */
	int8_t cOffsetBand1;	/* 5.000-5.080G */
	int8_t cOffsetBand2;	/* 5.160-5.180G */
	int8_t cOffsetBand3;	/* 5.200-5.280G */
	int8_t cOffsetBand4;	/* 5.300-5.340G */
	int8_t cOffsetBand5;	/* 5.500-5.580G */
	int8_t cOffsetBand6;	/* 5.600-5.680G */
	int8_t cOffsetBand7;	/* 5.700-5.825G */
};

struct CMD_PWR_PARAM {
	uint32_t au4Data[28];
	uint32_t u4RefValue1;
	uint32_t u4RefValue2;
};

struct CMD_PHY_PARAM {
	uint8_t aucData[144];	/* eFuse content */
};

struct CMD_AUTO_POWER_PARAM {
	/* 0: Disable 1: Enalbe 0x10: Change parameters */
	uint8_t ucType;
	uint8_t ucBssIndex;
	uint8_t aucReserved[2];
	uint8_t aucLevelRcpiTh[3];
	uint8_t aucReserved2[1];
	int8_t aicLevelPowerOffset[3];	/* signed, in unit of 0.5dBm */
	uint8_t aucReserved3[1];
	uint8_t aucReserved4[8];
};

/*for WMMAC, CMD_ID_UPDATE_AC_PARAMS*/
struct CMD_UPDATE_AC_PARAMS {
	uint8_t  ucAcIndex; /*0 ~3, from AC0 to AC3*/
	uint8_t  ucBssIdx;  /*no use*/
	/* if 0, disable ACM for ACx specified by
	** ucAcIndex, otherwise in unit of 32us
	*/
	uint16_t u2MediumTime;
	/* rate to be used to tx packet with priority
	** ucAcIndex , unit: bps
	*/
	uint32_t u4PhyRate;
	uint16_t u2EDCALifeTime; /* msdu life time for this TC, unit: 2TU */
	/* if we use fix rate to tx packets, should tell
	** firmware the limited retries
	*/
	uint8_t ucRetryCount;
	uint8_t aucReserved[5];
};

/* S56 Traffic Stream Metrics */
struct CMD_SET_TSM_STATISTICS_REQUEST {
	uint8_t ucEnabled; /* 0, disable; 1, enable; */
	uint8_t ucBssIdx; /* always AIS Bss index now */
	uint8_t ucAcIndex; /* wmm ac index, the statistics should be on this TC
			      */
	uint8_t ucTid;

	uint8_t aucPeerAddr
		[MAC_ADDR_LEN]; /* packet to the target address to be mesured */
	uint8_t ucBin0Range;
	uint8_t aucReserved[3];

	/* if this variable is 0, followed variables are meaningless
	** only report once for a same trigger condition in this time frame
	** for triggered mode: bit(0):average, bit(1):consecutive,
	** bit(2):delay
	*/
	uint8_t ucTriggerCondition;
	uint8_t ucAvgErrThreshold;
	uint8_t ucConsecutiveErrThreshold;
	uint8_t ucDelayThreshold;
	uint8_t ucMeasureCount;
	uint8_t ucTriggerTimeout; /* unit: 100 TU*/
};

struct CMD_GET_TSM_STATISTICS {
	uint8_t ucBssIdx; /* always AIS Bss now */
	/* wmm ac index, the statistics should be on this TC or TS */
	uint8_t ucAcIndex;
	uint8_t ucTid; /* */

	uint8_t aucPeerAddr
		[MAC_ADDR_LEN]; /* indicating the RA for the measured frames */
	/* for triggered mode: bit(0):average, bit(1):consecutive,
	** bit(2):delay
	*/
	uint8_t ucReportReason;
	uint16_t u2Reserved;

	uint32_t u4PktTxDoneOK;
	uint32_t u4PktDiscard; /* u2PktTotal - u2PktTxDoneOK */
	uint32_t u4PktFail; /* failed count for exceeding retry limit */
	uint32_t u4PktRetryTxDoneOK;
	uint32_t u4PktQosCfPollLost;

	/* 802.11k - Average Packet Transmission delay for all packets per this
	** TC or TS
	*/
	uint32_t u4AvgPktTxDelay;
	/* 802.11k - Average Packet Queue Delay */
	uint32_t u4AvgPktQueueDelay;
	uint64_t u8StartTime; /* represented by TSF */
	/* sum of packets whose packet tx delay is less than Bi (i=0~6) range
	** value(unit: TU)
	*/
	uint32_t au4PktCntBin[6];
};

struct CMD_DBDC_SETTING {
	uint8_t ucDbdcEn;
	uint8_t ucWmmBandBitmap;
	uint8_t ucUpdateSettingNextChReq;
	uint8_t aucPadding0[1];
	uint8_t ucCmdVer;
	uint8_t ucDBDCAAMode; /* 1 is set DBDC A+A Mode */
	uint16_t u2CmdLen;
	uint8_t ucPrimaryChannel;
	uint8_t ucWmmQueIdx;
	uint8_t ucRfBand;
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	uint8_t ucNoResp;
#endif
	uint8_t ucReason;
	uint8_t aucPadding3[24];
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
struct LONG_PULSE_BUFFER {
	uint32_t u4LongStartTime;
	uint16_t u2LongPulseWidth;
	int16_t i2LongPulsePower;
	uint8_t u1MDRDYFlag; /* bit1: mdray_early_flag, bit0: mdrdy_late_flag */
	uint8_t a1cReserve[3];
};

struct PERIODIC_PULSE_BUFFER {
	uint32_t u4PeriodicStartTime;
	uint16_t u2PeriodicPulseWidth;
	int16_t i2PeriodicPulsePower;
	uint8_t u1MDRDYFlag; /* bit1: mdray_early_flag, bit0: mdrdy_late_flag */
	uint8_t a1cReserve[3];
};

struct WH_RDD_PULSE_CONTENT {
	uint32_t u4HwStartTime;
	uint16_t u2HwPulseWidth;
	int16_t i2HwPulsePower;
	uint8_t fgScPass;
	uint8_t fgSwReset;
	uint8_t u1MDRDYFlag; /* bit1: mdray_early_flag, bit0: mdrdy_late_flag */
	uint8_t u1TxActive;	/* bit1: tx_early_flag, bit0: tx_late_flag */
};

struct EVENT_RDD_REPORT {
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
	uint32_t u4OutPRIStgDmin;
	/* Staggered PRF radar: min PRI Difference between 1st and 2nd  */
	struct LONG_PULSE_BUFFER arLongPulse[32];
	struct PERIODIC_PULSE_BUFFER arPeriodicPulse[32];
	struct WH_RDD_PULSE_CONTENT arContent[32];
};
#endif

struct EVENT_DEBUG_MSG {
	uint16_t u2DebugMsgId;
	uint8_t ucMsgType;
	uint8_t ucFlags;		/* unused */
	uint32_t u4Value;	/* memory addre or ... */
	uint16_t u2MsgSize;
	uint8_t aucReserved0[2];
	uint8_t aucMsg[];
};

struct CMD_EDGE_TXPWR_LIMIT {
	int8_t cBandEdgeMaxPwrCCK;
	int8_t cBandEdgeMaxPwrOFDM20;
	int8_t cBandEdgeMaxPwrOFDM40;
	int8_t cBandEdgeMaxPwrOFDM80;
};

struct CMD_POWER_OFFSET {
	uint8_t ucBand;		/*1:2.4G ;  2:5G */
	/*the max num subband is 5G, devide with 8 subband */
	uint8_t ucSubBandOffset[MAX_SUBBAND_NUM_5G];
	uint8_t aucReverse[3];

};

struct CMD_NVRAM_SETTING {

	struct WIFI_CFG_PARAM_STRUCT rNvramSettings;

};
struct CMD_NVRAM_FRAGMENT {
	/*restrict NVRAM TX CMD size to 1500 bytes*/
	/*because FW WFDMA MAX buf size is 1600 Byte*/
	uint8_t aucReverse[1500];
};
#if CFG_SUPPORT_TDLS
struct CMD_TDLS_CH_SW {
	u_int8_t fgIsTDLSChSwProhibit;
	/* uint8_t ucBssIndex; */
};
#endif

struct CMD_SET_DEVICE_MODE {
	uint16_t u2ChipID;
	uint16_t u2Mode;
};

#if CFG_SUPPORT_RDD_TEST_MODE
struct CMD_RDD_CH {
	uint8_t ucRddTestMode;
	uint8_t ucRddShutCh;
	uint8_t ucRddStartCh;
	uint8_t ucRddStopCh;
	uint8_t ucRddDfs;
	uint8_t ucReserved;
	uint8_t ucReserved1;
	uint8_t ucReserved2;
};
#endif

struct EVENT_ICAP_STATUS {
	uint8_t ucRddStatus;
	uint8_t aucReserved[3];
	uint32_t u4StartAddress;
	uint32_t u4IcapSieze;
#if CFG_SUPPORT_QA_TOOL
	uint32_t u4IcapContent;
#endif				/* CFG_SUPPORT_QA_TOOL */
};

#if CFG_SUPPORT_QA_TOOL
struct ADC_BUS_FMT {
	uint32_t u4Dcoc0Q: 14;	/* [13:0] */
	uint32_t u4Dcoc0I: 14;	/* [27:14] */
	uint32_t u4DbgData1: 4;	/* [31:28] */

	uint32_t u4Dcoc1Q: 14;	/* [45:32] */
	uint32_t u4Dcoc1I: 14;	/* [46:59] */
	uint32_t u4DbgData2: 4;	/* [63:60] */

	uint32_t u4DbgData3;	/* [95:64] */
};

struct IQC_BUS_FMT {
	int32_t u4Iqc0Q: 12;	/* [11:0] */
	int32_t u4Iqc0I: 12;	/* [23:12] */
	int32_t u4Na1: 8;		/* [31:24] */

	int32_t u4Iqc1Q: 12;	/* [43:32] */
	int32_t u4Iqc1I: 12;	/* [55:44] */
	int32_t u4Na2: 8;		/* [63:56] */

	int32_t u4Na3;		/* [95:64] */
};

struct IQC_160_BUS_FMT {
	int32_t u4Iqc0Q1: 12;	/* [11:0] */
	int32_t u4Iqc0I1: 12;	/* [23:12] */
	uint32_t u4Iqc0Q0P1: 8;	/* [31:24] */

	int32_t u4Iqc0Q0P2: 4;	/* [35:32] */
	int32_t u4Iqc0I0: 12;	/* [47:36] */
	int32_t u4Iqc1Q1: 12;	/* [59:48] */
	uint32_t u4Iqc1I1P1: 4;	/* [63:60] */

	int32_t u4Iqc1I1P2: 8;	/* [71:64] */
	int32_t u4Iqc1Q0: 12;	/* [83:72] */
	int32_t u4Iqc1I0: 12;	/* [95:84] */
};

struct SPECTRUM_BUS_FMT {
	int32_t u4DcocQ: 12;	/* [11:0] */
	int32_t u4DcocI: 12;	/* [23:12] */
	int32_t u4LpfGainIdx: 4;	/* [27:24] */
	int32_t u4LnaGainIdx: 2;	/* [29:28] */
	int32_t u4AssertData: 2;	/* [31:30] */
};

struct PACKED_ADC_BUS_FMT {
	uint32_t u4AdcQ0T2: 4;	/* [19:16] */
	uint32_t u4AdcQ0T1: 4;	/* [11:8] */
	uint32_t u4AdcQ0T0: 4;	/* [3:0] */

	uint32_t u4AdcI0T2: 4;	/* [23:20] */
	uint32_t u4AdcI0T1: 4;	/* [15:12] */
	uint32_t u4AdcI0T0: 4;	/* [7:4] */

	uint32_t u4AdcQ0T5: 4;	/* [43:40] */
	uint32_t u4AdcQ0T4: 4;	/* [35:32] */
	uint32_t u4AdcQ0T3: 4;	/* [27:24] */

	uint32_t u4AdcI0T5: 4;	/* [47:44] */
	uint32_t u4AdcI0T4: 4;	/* [39:36] */
	uint32_t u4AdcI0T3: 4;	/* [31:28] */

	uint32_t u4AdcQ1T2: 4;	/* [19:16] */
	uint32_t u4AdcQ1T1: 4;	/* [11:8] */
	uint32_t u4AdcQ1T0: 4;	/* [3:0] */

	uint32_t u4AdcI1T2: 4;	/* [23:20] */
	uint32_t u4AdcI1T1: 4;	/* [15:12] */
	uint32_t u4AdcI1T0: 4;	/* [7:4] */

	uint32_t u4AdcQ1T5: 4;	/* [43:40] */
	uint32_t u4AdcQ1T4: 4;	/* [35:32] */
	uint32_t u4AdcQ1T3: 4;	/* [27:24] */

	uint32_t u4AdcI1T5: 4;	/* [47:44] */
	uint32_t u4AdcI1T4: 4;	/* [39:36] */
	uint32_t u4AdcI1T3: 4;	/* [31:28] */
};

union ICAP_BUS_FMT {
	struct ADC_BUS_FMT rAdcBusData;	/* 12 bytes */
	struct IQC_BUS_FMT rIqcBusData;	/* 12 bytes */
	struct IQC_160_BUS_FMT rIqc160BusData;	/* 12 bytes */
	struct SPECTRUM_BUS_FMT rSpectrumBusData;	/* 4  bytes */
	struct PACKED_ADC_BUS_FMT rPackedAdcBusData;	/* 12 bytes */
};
#endif /* CFG_SUPPORT_QA_TOOL */

#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)

struct CHANNEL_TIMING_T {
	uint32_t u4ActiveTime;
	uint32_t u4BusyTime;
	uint32_t u4TxTime;
	uint16_t u2ChannelNum;
	uint8_t  aucPadding[2];
};

struct EVENT_CHANNEL_TIMING_INFO {
	struct CHANNEL_TIMING_T rChannelTiming[CH_MAX_NUM];
};
#endif

struct CMD_SET_TXPWR_CTRL {
	int8_t c2GLegacyStaPwrOffset;	/* Unit: 0.5dBm, default: 0 */
	int8_t c2GHotspotPwrOffset;
	int8_t c2GP2pPwrOffset;
	int8_t c2GBowPwrOffset;
	int8_t c5GLegacyStaPwrOffset;	/* Unit: 0.5dBm, default: 0 */
	int8_t c5GHotspotPwrOffset;
	int8_t c5GP2pPwrOffset;
	int8_t c5GBowPwrOffset;
	/* TX power policy when concurrence
	 * in the same channel
	 * 0: Highest power has priority
	 * 1: Lowest power has priority
	 */
	uint8_t ucConcurrencePolicy;
	int8_t acReserved1[3];	/* Must be zero */

	/* Power limit by channel for all data rates */
	int8_t acTxPwrLimit2G[14];	/* Channel 1~14, Unit: 0.5dBm */
	int8_t acTxPwrLimit5G[4];	/* UNII 1~4 */
	int8_t acReserved2[2];	/* Must be zero */
};

struct SSID_MATCH_SETS {
	int32_t i4RssiThresold;
	uint8_t aucSsid[32];
	uint8_t ucSsidLen;
	uint8_t aucPadding_1[3];
};

struct CMD_SCHED_SCAN_REQ {
	uint8_t ucVersion;
	uint8_t ucSeqNum;
	uint8_t fgStopAfterIndication;
	uint8_t ucSsidNum;
	uint8_t ucMatchSsidNum;
	uint8_t aucPadding_0;
	uint16_t u2IELen;
	struct PARAM_SSID auSsid[10];
	struct SSID_MATCH_SETS auMatchSsid[16];
	uint8_t ucChannelType;
	uint8_t ucChnlNum;
	uint8_t ucMspEntryNum;
	uint8_t ucScnFuncMask;
	struct CHANNEL_INFO aucChannel[64];
	uint16_t au2MspList[10];
	uint8_t ucBssIndex;
	uint8_t aucPadding_4[3];
	uint32_t u4DelayStartInSec;
	uint32_t u4FastScanIteration;
	uint32_t u4FastScanPeriod;
	uint32_t u4SlowScanPeriod;
	uint8_t aucRandomMac[MAC_ADDR_LEN];
	uint8_t aucPadding_3[38];
	/* keep last */
	uint8_t aucIE[];             /* MUST be the last for IE content */
};

struct EVENT_SCHED_SCAN_DONE {
	uint8_t ucSeqNum;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
};

/* For wake up option */
enum ENUM_CMD_HIF_WAKEUP_TYPE_T {
	ENUM_CMD_HIF_WAKEUP_TYPE_PCIE = 0,
	ENUM_CMD_HIF_WAKEUP_TYPE_USB = 1,
	ENUM_CMD_HIF_WAKEUP_TYPE_GPIO = 2,
	ENUM_CMD_HIF_WAKEUP_TYPE_SDIO = 3,
	ENUM_CMD_HIF_WAKEUP_TYPE_AXI = 4,
	ENUM_CMD_HIF_WAKEUP_TYPE_NUM  = 5,
};

enum ENUM_HIF_TYPE {
	ENUM_HIF_TYPE_SDIO = 0x00,
	ENUM_HIF_TYPE_USB = 0x01,
	ENUM_HIF_TYPE_PCIE = 0x02,
	ENUM_HIF_TYPE_GPIO = 0x03,
};

enum ENUM_HIF_DIRECTION {
	ENUM_HIF_TX = 0x01,
	ENUM_HIF_RX = 0x02,
	ENUM_HIF_TRX = 0x03,
};

enum ENUM_HIF_TRAFFIC_STATUS {
	ENUM_HIF_TRAFFIC_BUSY = 0x01,
	ENUM_HIF_TRAFFIC_IDLE = 0x02,
	ENUM_HIF_TRAFFIC_INVALID = 0x3,
};

struct EVENT_HIF_CTRL {
	uint8_t ucHifType;
	uint8_t ucHifTxTrafficStatus;
	uint8_t ucHifRxTrafficStatus;
	uint8_t ucHifSuspend;
#if (CFG_WIFI_PCIE_L2_MOBILE_ONLY == 1)
	uint16_t u2HifPcieUserCount;
	uint8_t aucReserved2[30];
#else
	uint8_t aucReserved2[32];
#endif
};

#if CFG_SUPPORT_BUILD_DATE_CODE
struct CMD_GET_BUILD_DATE_CODE {
	uint8_t aucReserved[4];
};
#endif

struct CMD_GET_STA_STATISTICS {
	uint8_t ucIndex;
	uint8_t ucFlags;
	uint8_t ucReadClear;
	uint8_t ucLlsReadClear;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	uint8_t ucResetCounter;
	uint8_t aucReserved1[1];
	uint8_t aucReserved2[16];
};

struct CMD_QUERY_STATISTICS {
	uint8_t ucBssIndex;
	uint8_t aucReserved1[3];
	uint8_t aucReserved2[8];
};

/* per access category statistics */
struct WIFI_WMM_AC_STAT_GET_FROM_FW {
	uint32_t u4TxFailMsdu;
	uint32_t u4TxRetryMsdu;
};

/* CFG_SUPPORT_WFD */
struct EVENT_STA_STATISTICS {
	/* Event header */
	/* UINT_16     u2Length; */
	/* Must be filled with 0x0001 (EVENT Packet) */
	/* UINT_16     u2Reserved1; */
	/* UINT_8            ucEID; */
	/* UINT_8      ucSeqNum; */
	/* UINT_8            aucReserved2[2]; */

	/* Event Body */
	uint8_t ucVersion;
	uint8_t aucReserved1[3];
	uint32_t u4Flags;	/* Bit0: valid */

	uint8_t ucStaRecIdx;
	uint8_t ucNetworkTypeIndex;
	uint8_t ucWTEntry;
	uint8_t aucReserved4[1];

	uint8_t ucMacAddr[MAC_ADDR_LEN];
	uint8_t ucPer;		/* base: 128 */
	uint8_t ucRcpi;

	uint32_t u4PhyMode;	/* SGI BW */
	uint16_t u2LinkSpeed;	/* unit is 0.5 Mbits */
	uint8_t ucLinkQuality;
	uint8_t ucLinkReserved;

	uint32_t u4TxCount;
	uint32_t u4TxFailCount;
	uint32_t u4TxLifeTimeoutCount;
	uint32_t u4TxDoneAirTime;
	/* Transmit in the air (wtbl) */
	uint32_t u4TransmitCount;
	/* Transmit without ack/ba in the air (wtbl) */
	uint32_t u4TransmitFailCount;

	struct WIFI_WMM_AC_STAT_GET_FROM_FW
		arLinkStatistics[AC_NUM];	/*link layer statistics */

	uint8_t ucTemperature;
	uint8_t ucSkipAr;
	uint8_t ucArTableIdx;
	uint8_t ucRateEntryIdx;
	uint8_t ucRateEntryIdxPrev;
	uint8_t ucTxSgiDetectPassCnt;
	uint8_t ucAvePer;
#if (CFG_SUPPORT_RA_GEN == 0)
	uint8_t aucArRatePer[AR_RATE_TABLE_ENTRY_MAX];
	uint8_t aucRateEntryIndex[AUTO_RATE_NUM];
#else
	uint32_t u4AggRangeCtrl_0;
	uint32_t u4AggRangeCtrl_1;
	uint8_t ucRangeType;
#if (CFG_SUPPORT_CONNAC2X == 1) || (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	uint32_t u4AggRangeCtrl_2;
	uint32_t u4AggRangeCtrl_3;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	uint32_t u4AggRangeCtrl_4;
	uint32_t u4AggRangeCtrl_5;
	uint32_t u4AggRangeCtrl_6;
	uint32_t u4AggRangeCtrl_7;
#else
	uint8_t aucReserved5[16];
#endif

#else
	uint8_t aucReserved5[24];
#endif
#endif
	uint8_t ucArStateCurr;
	uint8_t ucArStatePrev;
	uint8_t ucArActionType;
	uint8_t ucHighestRateCnt;
	uint8_t ucLowestRateCnt;
	uint16_t u2TrainUp;
	uint16_t u2TrainDown;
	uint32_t u4Rate1TxCnt;
	uint32_t u4Rate1FailCnt;
	struct TX_VECTOR_BBP_LATCH rTxVector[ENUM_BAND_NUM];
	struct MIB_INFO_STAT rMibInfo[ENUM_BAND_NUM];
	u_int8_t fgIsForceTxStream;
	u_int8_t fgIsForceSeOff;
#if (CFG_SUPPORT_RA_GEN == 0)
	uint8_t aucReserved6[17];
#else
	uint16_t u2RaRunningCnt;
	uint8_t ucRaStatus;
	uint8_t ucFlag;
	uint8_t aucTxQuality[MAX_TX_QUALITY_INDEX];
	uint8_t ucTxRateUpPenalty;
	uint8_t ucLowTrafficMode;
	uint8_t ucLowTrafficCount;
	uint8_t ucLowTrafficDashBoard;
	uint8_t ucDynamicSGIState;
	uint8_t ucDynamicSGIScore;
	uint8_t ucDynamicBWState;
	uint8_t ucDynamicGband256QAMState;
	uint8_t ucVhtNonSpRateState;
#endif
	uint8_t aucReserved[4];
};

struct EVENT_LTE_SAFE_CHN {
	uint8_t ucVersion;
	uint8_t aucReserved[3];
	uint32_t u4Flags;	/* Bit0: valid */
	struct LTE_SAFE_CHN_INFO rLteSafeChn;
};

struct EVENT_WIFI_RDD_TEST {
	uint32_t u4FuncIndex;
	uint32_t u4FuncLength;
	uint32_t u4Prefix;
	uint32_t u4Count;
	uint8_t ucRddIdx;
	uint8_t aucReserve[3];
	uint8_t aucBuffer[];
};

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
struct CMD_ICS_SNIFFER_INFO {
	/* DWORD_0 - Common Part*/
	/*Include system all and PSSniffer */
	uint8_t	ucCmdVer;
	uint8_t	ucAction;
	uint16_t u2CmdLen;
	/* DWORD_1 ~ x */
	uint8_t ucModule;
	uint8_t ucFilter;
	uint8_t ucOperation;
	uint8_t aucPadding0;
	uint16_t ucCondition[7];
	uint8_t aucPadding1[62];
};
#endif /* CFG_SUPPORT_ICS */

#if (CFG_SUPPORT_PHY_ICS == 1)
struct CMD_PHY_ICS_EVENT_INFO {
	uint8_t ucBandIdx;
	uint8_t ucPartition;
	uint16_t u2EventGroup;
	uint32_t u4EventID;
};

struct CMD_PHY_ICS_START_INFO {
	uint16_t u2Action;
	uint16_t u2Timer;
};
#endif

#if CFG_SUPPORT_MSP
/* EVENT_ID_WTBL_INFO */
struct EVENT_WLAN_INFO {

	struct PARAM_TX_CONFIG	rWtblTxConfig;
	struct PARAM_SEC_CONFIG	rWtblSecConfig;
	struct PARAM_KEY_CONFIG	rWtblKeyConfig;
	struct PARAM_PEER_RATE_INFO	rWtblRateInfo;
	struct PARAM_PEER_BA_CONFIG	rWtblBaConfig;
	struct PARAM_PEER_CAP	rWtblPeerCap;
	struct PARAM_PEER_RX_COUNTER_ALL rWtblRxCounter;
	struct PARAM_PEER_TX_COUNTER_ALL rWtblTxCounter;
};

/* EVENT_ID_MIB_INFO */
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
struct EVENT_MIB_INFO {
	struct MIB_INFO arMibInfo[MAX_MIB_TAG_CNT];
};
#else
struct EVENT_MIB_INFO {
	struct HW_MIB_COUNTER	    rHwMibCnt;
	struct HW_MIB2_COUNTER	    rHwMib2Cnt;
	struct HW_TX_AMPDU_METRICS	    rHwTxAmpduMts;

};
#endif
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
struct EVENT_GET_DPD_CACHE {
	uint8_t		ucDpdCacheNum;
	uint8_t		ucReserved[3];
	uint32_t	u4DpdCacheCh[PER_CH_CAL_CACHE_NUM];
	uint8_t		ucDpdCachePath[PER_CH_CAL_CACHE_NUM];
};
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
struct EVENT_TX_MCS_INFO {
	uint16_t    au2TxRateCode[MCS_INFO_SAMPLE_CNT];
	uint8_t     aucTxBw[MCS_INFO_SAMPLE_CNT];
	uint8_t     aucTxSgi[MCS_INFO_SAMPLE_CNT];
	uint8_t     aucTxLdpc[MCS_INFO_SAMPLE_CNT];
	uint8_t     aucTxRatePer[MCS_INFO_SAMPLE_CNT];
};
#endif

/*#if (CFG_EEPROM_PAGE_ACCESS == 1)*/
struct EVENT_ACCESS_EFUSE {

	uint32_t         u4Address;
	uint32_t         u4Valid;
	uint8_t          aucData[16];

};

struct EXT_EVENT_EFUSE_FREE_BLOCK {
	uint8_t  ucFreeBlockNum;
	uint8_t  ucVersion;
	uint8_t  ucTotalBlockNum;
	uint8_t  ucReserved;
};

struct EXT_EVENT_GET_TX_POWER {

	uint8_t  ucTxPwrType;
	uint8_t  ucEfuseAddr;
	uint8_t  ucTx0TargetPower;
	uint8_t  ucDbdcIdx;

};

struct EXT_EVENT_RF_TEST_RESULT_T {
	uint32_t u4FuncIndex;
	uint32_t u4PayloadLength;
	uint8_t  aucEvent[];
};

struct EXT_EVENT_RBIST_DUMP_DATA_T {
	uint32_t u4FuncIndex;
	uint32_t u4PktNum;
	uint32_t u4Bank;
	uint32_t u4DataLength;
	uint32_t u4WFCnt;
	uint32_t u4SmplCnt;
	uint32_t u4Reserved[6];
	uint32_t u4Data[256];
};

#if (CFG_SUPPORT_PHY_ICS == 1)
struct EXT_EVENT_PHY_ICS_DUMP_DATA_T {
	uint32_t u4FuncIndex; /* 0x14 = 20 */
	uint32_t u4PktNum;
	uint32_t u4PhyTimestamp;
	uint32_t u4DataLen;
	uint32_t u4Reserved[5];
	uint32_t u4Data[MAX_PHY_ICS_DUMP_DATA_CNT];
};
#endif /* #if (CFG_SUPPORT_PHY_ICS == 1) */

struct EXT_EVENT_RBIST_CAP_STATUS_T {
	uint32_t u4FuncIndex;
	uint32_t u4CapDone;
	uint32_t u4Reserved[15];
};

struct EXT_EVENT_RECAL_DATA_T {
	uint32_t u4FuncIndex;
	uint32_t u4Type;	/* 0 for string, 1 for int data */
	union {
		uint8_t ucData[32];
		uint32_t u4Data[3];
	} u;
};


struct CMD_SUSPEND_MODE_SETTING {
	uint8_t ucBssIndex;
	uint8_t ucEnableSuspendMode;
	uint8_t ucMdtim; /* LP parameter */
	uint8_t ucWowSuspend;
	uint8_t ucReserved2[64];
};

struct EVENT_UPDATE_COEX_PHYRATE {
	uint8_t ucVersion;
	uint8_t aucReserved1[3];    /* 4 byte alignment */
	uint32_t u4Flags;
	uint32_t au4PhyRateLimit[MAX_BSSID_NUM + 1];
	uint8_t ucSupportSisoOnly;
	uint8_t ucWfPathSupport;
	uint8_t aucReserved2[2];    /* 4 byte alignment */
};

#if CFG_WIFI_TXPWR_TBL_DUMP
struct CMD_GET_TXPWR_TBL {
	/* DWORD_0 - Common Part */
	uint8_t  ucCmdVer;
	uint8_t  ucAction;
	uint16_t u2CmdLen;

	/* DWORD_1 ~ x - Command Body */
	uint8_t ucDbdcIdx;
	uint8_t aucReserved[3];
};

struct EVENT_GET_TXPWR_TBL {
	/* DWORD_0 - Common Part */
	uint8_t  ucEvtVer;
	uint8_t  ucAction;
	uint16_t u2EvtLen;

	/* DWORD_1 ~ x - Command Body */
	uint8_t ucCenterCh;
	uint8_t aucReserved[3];
	struct POWER_LIMIT tx_pwr_tbl[TXPWR_TBL_NUM];
};
#endif /* CFG_WIFI_TXPWR_TBL_DUMP */

enum ENUM_COEX_MODE {
	COEX_NONE_BT,
	COEX_TDD_MODE,
	COEX_HBD_MODE,
	COEX_FDD_MODE,
};

struct EVENT_COEX_STATUS {
	uint8_t ucVersion;       /* default v1.0 = 0x01 */
	uint8_t ucCoexMode;      /* 0:non-bt 1:TDD 2:Hybrid 3:FDD */
	uint8_t ucBtOnOff;
	uint8_t ucBtRssi;
	uint16_t u2BtProfile;
	uint8_t fgIsBAND2G4Coex;
#if (CFG_SUPPORT_AVOID_DESENSE == 1)
	uint8_t fgIs5GsupportEPA; /* If 5G support EPA, WFA didn't desense BT */
	uint8_t aucReserved1[4]; /* 4 byte alignment */
#else
	uint8_t aucReserved1[5]; /* 4 byte alignment */
#endif
};

#if (CFG_SUPPORT_PKT_OFLD == 1)
struct CMD_OFLD_INFO {
	/* restrict buffer size to 1500 bytes */
	/* because FW WFDMA MAX buf size is 1600 Byte */
	uint8_t ucType;
	uint8_t ucOp;
	uint8_t ucFragNum;
	uint8_t ucFragSeq;
	uint32_t u4TotalLen;
	uint32_t u4BufLen;
	uint8_t aucBuf[PKT_OFLD_BUF_SIZE];
};

struct CMD_APF_INFO {
	uint8_t ucBssIdx;
	uint8_t ucAisIdx;
	uint32_t u4ApfVer;
	uint32_t u4ApfBufSize;
};
#endif /* CFG_SUPPORT_PKT_OFLD */

struct EVENT_REPORT_U_EVENT {
	uint8_t aucData[MAX_UEVENT_LEN];
};

#if (CFG_SUPPORT_TWT == 1)
/*
 * Important: Used for Communication between Host and WM-CPU,
 * should be packed and DW-aligned and in little-endian format
 */
struct _EXT_CMD_TWT_ARGT_UPDATE_T {
	/* DW0 */
	uint8_t ucAgrtTblIdx;
	uint8_t ucAgrtCtrlFlag;
	uint8_t ucOwnMacId;
	uint8_t ucFlowId;
	/* DW1 */
	/* Specify the peer ID (MSB=0) or group ID (MSB=1)
	 * (10 bits for StaIdx, MSB to identify if it is for groupId)
	 */
	uint16_t u2PeerIdGrpId;

	/* Same as SPEC definition. 8 bits, in unit of 256 us */
	uint8_t  ucAgrtSpDuration;
	/* So that we know which BSS TSF should be used for this AGRT */
	uint8_t  ucBssIndex;
	/* DW2, DW3, DW4 */
	uint32_t u4AgrtSpStartTsfLow;
	uint32_t u4AgrtSpStartTsfHigh;
	uint16_t u2AgrtSpWakeIntvlMantissa;
	uint8_t  ucAgrtSpWakeIntvlExponent;
	uint8_t  ucIsRoleAp;		/* 1: AP, 0: STA */
	/* DW5 */
	/* For Bitmap definition, please refer to
	* TWT_AGRT_PARA_BITMAP_IS_TRIGGER and etc
	*/
	uint8_t  ucAgrtParaBitmap;
	uint8_t  ucReserved_a;
	/* Following field is valid ONLY when peerIdGrpId is a group ID */
	uint16_t u2Reserved_b;
	/* DW6 */
	uint8_t  ucGrpMemberCnt;
	uint8_t  ucReserved_c;
	uint16_t u2Reserved_d;
#if 0
	/* DW7 ~ DW10 */
	uint16_t au2StaList[TWT_GRP_MAX_MEMBER_CNT];
#endif

#if (CFG_SUPPORT_RTWT == 1)
	/* DW7 RTWT traffic info */
	uint8_t ucTrafficInfoPresent;
	uint8_t ucDlUlBmpValid;
	uint8_t ucDlBmp;
	uint8_t ucUlBmp;
#endif
};
#endif

#if (CFG_SUPPORT_802_11AX == 1)
struct _CMD_RLM_UPDATE_SR_PARMS_T {
	/* DWORD_0 - Common Part */
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;       /* Cmd size including common part and body */

	/* DWORD_1 afterwards - Command Body */
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

	uint8_t  aucPadding2[32];
};

/** SR Capability */
struct _WH_SR_CAP_T {
	/** RMAC */
	uint8_t fgSrEn;
	uint8_t fgSrgEn;
	uint8_t fgNonSrgEn;
	uint8_t fgSingleMdpuRtsctsEn;
	uint8_t fgHdrDurEn;
	uint8_t fgTxopDurEn;
	uint8_t fgNonSrgInterPpduPresv;
	uint8_t fgSrgInterPpduPresv;
	/** AGG */
	uint8_t fgSrRemTimeEn;
	uint8_t fgProtInSrWinDis;
	uint8_t fgTxCmdDlRateSelEn;
	/** MIB */
	uint8_t fgAmpduTxCntEn;
};

/** SR Indicator */
struct _WH_SR_IND_T {
	/** RMAC */
	uint8_t u1NonSrgInterPpduRcpi;
	uint8_t u1SrgInterPpduRcpi;
	uint16_t u2NonSrgVldCnt;
	uint16_t u2SrgVldCnt;
	uint16_t u2IntraBssPpduCnt;
	uint16_t u2InterBssPpduCnt;
	uint16_t u2NonSrgPpduVldCnt;
	uint16_t u2SrgPpduVldCnt;
	/** Reserve for 4-byte aligned*/
	uint8_t u1Reserved[2];
	/** MIB */
	uint32_t u4SrAmpduMpduCnt;
	uint32_t u4SrAmpduMpduAckedCnt;
};

/* SR CMD related structure*/
struct _SR_CMD_T {
	uint8_t u1CmdSubId;
	uint8_t u1ArgNum;
	uint8_t u1DbdcIdx;
	uint8_t u1Status;
	uint8_t u1DropTaIdx;
	uint8_t u1StaIdx;
	uint8_t u1Rsv[2];
};

struct _SR_CMD_SR_CAP_T {
	struct _SR_CMD_T rSrCmd;
	struct _WH_SR_CAP_T rSrCap;
};

struct _SR_CMD_SR_IND_T {
	struct _SR_CMD_T rSrCmd;
	struct _WH_SR_IND_T rSrInd;
};

/* SR EVENT related structure*/
struct _SR_EVENT_T {
	uint8_t u1EventSubId;
	uint8_t u1ArgNum;
	uint8_t u1DbdcIdx;
	uint8_t u1Status;
	uint8_t u1DropTaIdx;
	uint8_t u1StaIdx;
	uint8_t u1Rsv[2];
};

enum ENUM_SR_EVENT_STATUS_T {
	SR_STATUS_SUCCESS = 0x0,
	SR_STATUS_SANITY_FAIL,
	SR_STATUS_CALL_MIDDLE_FAIL,
	SR_STATUS_SW_HW_VAL_NOT_SYNC,
	SR_STATUS_UNKNOWN,
	SR_STATUS_NUM
};

struct _SR_EVENT_SR_CAP_T {
	struct _SR_EVENT_T rSrEvent;
	struct _WH_SR_CAP_T rSrCap;
};

struct _SR_EVENT_SR_IND_T {
	struct _SR_EVENT_T rSrEvent;
	struct _WH_SR_IND_T rSrInd;
};

struct _EXTRA_ARG_TSF_T {
	uint8_t  ucHwBssidIndex;
	uint8_t  aucReserved[3];
};

union _EXTRA_ARG_MAC_INFO_T {
	struct _EXTRA_ARG_TSF_T rTsfArg;
};

struct _EXT_CMD_GET_MAC_INFO_T {
	uint16_t u2MacInfoId;
	uint8_t  aucReserved[2];
	union _EXTRA_ARG_MAC_INFO_T rExtraArgument;
};
#endif

struct TSF_RESULT_T {
	uint32_t u4TsfBitsLow;
	uint32_t u4TsfBitsHigh;
};

union MAC_INFO_RESULT_T {
	struct TSF_RESULT_T rTsfResult;
};

struct EXT_EVENT_MAC_INFO_T {
	uint16_t  u2MacInfoId;
	uint8_t   aucReserved[2];
	union MAC_INFO_RESULT_T rMacInfoResult;
};

struct EXT_CMD_EVENT_DUMP_MEM_T {
	uint32_t u4MemAddr;
	uint8_t ucData[64];
};

/*#endif*/
struct CMD_TDLS_PS_T {
	/* 0: disable tdls power save; 1: enable tdls power save */
	uint8_t	ucIsEnablePs;
	uint8_t	aucReserved[3];
};

/** struct for power rate control command **/
struct CMD_POWER_RATE_TXPOWER_CTRL_T {
	uint8_t u1PowerCtrlFormatId;
	uint8_t u1PhyMode;
	uint8_t u1TxRate;
	uint8_t u1BW;
	uint8_t u1BandIdx;
	int8_t  i1TxPower;
	uint8_t u1Reserved[2];
};


#if (CFG_SUPPORT_TXPOWER_INFO == 1)
struct CMD_TX_POWER_SHOW_INFO_T {
	uint8_t ucPowerCtrlFormatId;
	uint8_t ucTxPowerInfoCatg;
	uint8_t ucBandIdx;
	uint8_t ucReserved;
};

struct EXT_EVENT_TXPOWER_ALL_RATE_POWER_INFO_T {
	uint8_t ucTxPowerCategory;
	uint8_t ucBandIdx;
	uint8_t ucChBand;
	uint8_t ucReserved;

	/* Rate power info */
	struct FRAME_POWER_CONFIG_INFO_T rRatePowerInfo;

	/* tx Power Max/Min Limit info */
	int8_t icPwrMaxBnd;
	int8_t icPwrMinBnd;
	uint8_t ucReserved2;
};
#endif

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
struct CMD_TX_POWER_PERCENTAGE_CTRL_T {
	uint8_t ucPowerCtrlFormatId;
	bool  fgPercentageEnable;
	uint8_t ucBandIdx;
	uint8_t ucReserved;
};

struct CMD_TX_POWER_PERCENTAGE_DROP_CTRL_T {
	uint8_t ucPowerCtrlFormatId;
	uint8_t i1PowerDropLevel;
	uint8_t ucBandIdx;
	uint8_t ucReserved;
};
#endif

struct EXT_EVENT_MAX_AMSDU_LENGTH_UPDATE {
	uint8_t ucWlanIdx;
	uint8_t ucAmsduLen;
};

#if CFG_SUPPORT_MLR
struct EVENT_MLR_FSM_UPDATE {
	uint16_t u2WlanIdx;
	uint8_t ucMlrMode;
	uint8_t ucMlrState;
	/* MLR TXD fixed rate index (only used for REBB segment) */
	uint8_t ucMlrTxdFrIdx;
	/* MLR enable Tx fragment or not */
	uint8_t ucTxFragEn;
	uint8_t aucReserved[2];
};
#endif

#if CFG_SUPPORT_NAN
struct _CMD_EVENT_TLV_COMMOM_T {
	uint16_t u2TotalElementNum;
	uint8_t aucReserved[2];
	uint8_t aucBuffer[];
};

struct _CMD_EVENT_TLV_ELEMENT_T {
	uint32_t tag_type;
	uint32_t body_len; /* size of the following aucbody[] */
	uint8_t aucbody[];
};

struct _TXM_CMD_EVENT_TEST_T {
	uint32_t u4TestValue0;
	uint32_t u4TestValue1;
	uint8_t ucTestValue2;
	uint8_t aucReserved[3]; /*For 4 bytes alignment*/
};

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_MASTER_PREFERENCE_T {
	uint8_t ucMasterPreference;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

struct EVENT_UPDATE_NAN_TX_STATUS {
	uint8_t aucFlowCtrl[CFG_STA_REC_NUM];
};

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_UPDATE_ATTR_STRUCT {
	uint8_t ucAttrId;
	uint16_t u2AttrLen;
	uint8_t aucAttrBuf[1024];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_DW_INTERVAL_T {
	uint8_t ucDWInterval;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_GET_DEVICE_INFO {
	uint8_t ucVersion;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

enum ENUM_NAN_DISC_BCN_TYPE {
	ENUM_DISC_BCN_PERIOD = 0,
	ENUM_DISC_BCN_SLOT
};

/* Due to the firmware size limitation, we have temporarily set it to 256.
 * The data sent from wifip2pd is often 35, and the recommended setting
 * is 512 or 1024.
 */
#define NAN_CUSTOM_ATTRIBUTE_MAX_SIZE 256
struct NanCustomAttribute {
	u16 length;
	u8 data[NAN_CUSTOM_ATTRIBUTE_MAX_SIZE];
};

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_DFSP_CONFIG {
	uint16_t version;
	uint16_t length;
	uint16_t flags;
	/* bit 0 = enable;no other defined */
	/* duration of no beacon for suspension */
	uint16_t max_bcn_miss_duration;
	uint8_t mcsp_ttl;
	uint8_t bcsa_cnt;
	uint8_t max_empty_aw;
	uint16_t mon_chan;
	/* passive monitor channel */
	uint8_t mon_bssid[MAC_ADDR_LEN];
	/* bssid of the AP */
	uint16_t max_bcn_miss_af_duration;
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_UPDATE_CUSTOM_ATTR_T {
	uint16_t u2Length;
	uint8_t aucReserved[2];
	uint8_t aucData[256];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

struct NAN_EVENT_REPORT_DW_T {
	uint32_t expected_tsf_h;
	uint32_t expected_tsf_l;
	uint32_t actual_tsf_h;
	uint32_t actual_tsf_l;
	uint16_t channel;
	uint16_t dw_num;
};

struct NAN_EVENT_DEVICE_ROLE_T {
	uint8_t ucNanDeviceRole;
	uint8_t ucHopCount;
	uint8_t aucReserved[2];
};

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_LOWPOWER_CTRL_T {
	uint8_t ucEnabled;
	uint8_t aucReserved[3];
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

struct _NAN_EVENT_DEVICE_INFO {
	uint8_t ucIsEnabled;
	uint8_t aucSelfMacAddr[MAC_ADDR_LEN];
	uint8_t ucFwElectionEnable;
	uint32_t u4NanDeviceRole;
	uint32_t u4NanDeviceState;
	uint8_t ucMasterPreference;
	uint8_t ucRandomFactor;
	uint8_t ucHopCount;
	uint8_t aucClusterID[MAC_ADDR_LEN];
	uint8_t aucAnchorMasterMacAddr[MAC_ADDR_LEN];
	uint8_t ucAmMasterPreference;
	uint8_t ucAmRandomFactor;
	uint8_t aucParentMacAddr[MAC_ADDR_LEN];
	uint8_t ucParentMasterPreference;
	uint8_t ucParentRandomFactor;
	uint32_t u4AMBTT;
	uint32_t au4Tsf[2];
};

struct _NAN_EVENT_REPORT_BEACON {
	enum ENUM_BAND eRfBand;
	int32_t i4Rssi;
	uint32_t au4LocalTsf[2];
	uint16_t u2BeaconLength;
	uint16_t u2TxMode;
	uint8_t ucRate;
	uint8_t ucHwChnl;
	uint8_t ucBw;
	uint8_t aucAnchorMasterRank[ANCHOR_MASTER_RANK_NUM];
	uint8_t aucReserved[5];
	uint8_t aucBeaconFrame[];
};

struct _NAN_EVENT_LOWPOWER_CTRL {
	uint8_t ucPeerSchRecordTxMap;
	uint8_t aucRsvd[7];
};

enum _ENUM_NAN_SUB_CMD {
	NAN_CMD_TEST = 0,       /* 0 */
	NAN_TXM_TEST = 1,
	NAN_CMD_MASTER_PREFERENCE = 2,
	NAN_CMD_HOP_COUNT = 3,
	NAN_CMD_PUBLISH = 4,
	NAN_CMD_CANCEL_PUBLISH = 5,     /* 5 */
	NAN_CMD_UPDATE_PUBLISH = 6,
	NAN_CMD_SUBSCRIBE = 7,
	NAN_CMD_CANCEL_SUBSCRIBE = 8,
	NAN_CMD_TRANSMIT = 9,
	NAN_CMD_ENABLE_REQUEST = 10,     /* 10 */
	NAN_CMD_DISABLE_REQUEST = 11,
	NAN_CMD_UPDATE_AVAILABILITY = 12,
	NAN_CMD_UPDATE_CRB = 13,
	NAN_CMD_CRB_HANDSHAKE_TOKEN = 14,
	NAN_CMD_MANAGE_PEER_SCH_RECORD = 15, /* 15 */
	NAN_CMD_MAP_STA_RECORD = 16,
	NAN_CMD_RANGING_REPORT_DISC = 17,
	NAN_CMD_FTM_PARAM = 18,
	NAN_CMD_UPDATE_PEER_UAW = 19,
	NAN_CMD_UPDATE_ATTR = 20,    /* 20 */
	NAN_CMD_UPDATE_PHY_SETTING = 21,
	NAN_CMD_UPDATE_POTENTIAL_CHNL_LIST = 22,
	NAN_CMD_UPDATE_AVAILABILITY_CTRL = 23,
	NAN_CMD_UPDATE_PEER_CAPABILITY = 24,
	NAN_CMD_ADD_CSID = 25,   /* 25 */
	NAN_CMD_MANAGE_SCID = 26,
	NAN_CMD_CHANGE_ADDRESS = 27,
	NAN_CMD_SET_SCHED_VERSION = 28,
	NAN_CMD_SET_NAN_CONFIG = 29,
	NAN_CMD_SET_DISC_BCN = 30, /* 30 */
	NAN_CMD_UPDATE_POTENTIAL_AVAILABILITY = 31,
	NAN_CMD_UPDATE_CUSTOM_ATTR = 32,
	NAN_CMD_DFSP_CONFIG = 33,
	NAN_CMD_GET_DEVICE_INFO = 34,
	NAN_CMD_VENDOR_PAYLOAD = 35,
	NAN_CMD_LOWPOWER_CTRL = 37,
	NAN_CMD_SET_HOST_ELECTION = 42,
	NAN_CMD_SET_ELECTION_ROLE = 43,
	NAN_CMD_INSTANT_COMM_MODE = 44,
	NAN_CMD_PUBLISH_EXT = 50,
	NAN_CMD_SUBSCRIBE_EXT = 51,
	NAN_CMD_TRANSMIT_EXT = 52,
	NAN_CMD_SET_DW_INTERVAL = 60,
	NAN_CMD_ENABLE_UNSYNC = 61,

	/* EXT_CMD Part */
	/* Reserve for vendor r, 100 ~ 199 */

	/* Reserve for vendor s, 200 ~ 299 */
	NAN_CMD_EXT_CUSTOM_CMD = 200,
	NAN_CMD_EXT_CLUSTER = 252,
	NAN_CMD_EXT_P2P = 255,
	NAN_CMD_EXT_MERGING_DIRECTION = 256,
	NAN_CMD_EXT_SYNC = 257,
	NAN_CMD_EXT_MERGING = 258,
	NAN_CMD_EXT_SCHEDULING = 259,
	NAN_CMD_EXT_USD = 264,
	NAN_CMD_EXT_ASC = 265,

	NAN_CMD_NUM
};

enum _ENUM_NAN_SUB_EVENT {
	NAN_EVENT_TEST = 0, /* 0 */
	NAN_EVENT_DISCOVERY_RESULT = 1,
	NAN_EVENT_FOLLOW_EVENT = 2,
	NAN_EVENT_MASTER_IND_ATTR = 3,
	NAN_EVENT_CLUSTER_ID_UPDATE = 4,
	NAN_EVENT_REPLIED_EVENT = 5,    /* 5 */
	NAN_EVENT_PUBLISH_TERMINATE_EVENT = 6,
	NAN_EVENT_SUBSCRIBE_TERMINATE_EVENT = 7,
	NAN_EVENT_ID_SCHEDULE_CONFIG = 8,
	NAN_EVENT_ID_PEER_AVAILABILITY = 9,
	NAN_EVENT_ID_PEER_CAPABILITY = 10,   /* 10 */
	NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN = 11,
	NAN_EVENT_ID_DATA_NOTIFY = 12,
	NAN_EVENT_FTM_DONE = 13,
	NAN_EVENT_RANGING_BY_DISC = 14,
	NAN_EVENT_NDL_FLOW_CTRL = 15,    /* 15 */
	NAN_EVENT_DW_INTERVAL = 16,
	NAN_EVENT_NDL_DISCONNECT = 17,
	NAN_EVENT_ID_PEER_CIPHER_SUITE_INFO = 18,
	NAN_EVENT_ID_PEER_SEC_CONTEXT_INFO = 19,
	NAN_EVENT_ID_DE_EVENT_IND = 20,  /* 20 */
	NAN_EVENT_SELF_FOLLOW_EVENT = 21,
	NAN_EVENT_DISABLE_IND = 22,
	NAN_EVENT_NDL_FLOW_CTRL_V2 = 23,
	NAN_EVENT_ID_DEVICE_CAPABILITY = 24,
	NAN_EVENT_DISC_BCN_PERIOD = 25,  /* 25 */
	NAN_EVENT_DFSP_CSA = 26,
	NAN_EVENT_DFSP_CSA_COMPLETE = 27,
	NAN_EVENT_DFSP_SUSPEND_RESUME = 28,
	NAN_EVENT_REPORT_DW_START = 29,
	NAN_EVENT_REPORT_DW_END = 30, /* 30 */
	NAN_EVENT_DEVICE_ROLE = 31,
	NAN_EVENT_REPORT_BEACON = 32,
	NAN_EVENT_DEVICE_INFO = 33,
	NAN_EVENT_SERVICE_DISC_CAPABILITY = 34,
	NAN_EVENT_MATCH_EXPIRE = 35,
	NAN_EVENT_SLOT_STATISTICS = 36,
	NAN_EVENT_LOWPOWER_CTRL = 37,
	NAN_EVENT_RANGING_CTRL = 38,
	NAN_EVENT_VENDOR_DISCOVERY_RESULT = 50, /* 50 */
	NAN_EVENT_VENDOR_PUBLISH_REPLIED_EVENT = 51,
	NAN_EVENT_VENDOR_FOLLOW_UP_RX_EVENT = 52,
	NAN_EVENT_VENDOR_FOLLOW_UP_TX_EVENT = 53,

	NAN_EVENT_NUM
};
#endif

#if CFG_SUPPORT_RTT
struct CMD_RTT_REQUEST {
	uint8_t ucSeqNum;
	uint8_t fgEnable;              /* request or cancel */
	uint8_t ucConfigNum;
	uint8_t ucPaddings[5];
	struct RTT_CONFIG arRttConfigs[CFG_RTT_MAX_CANDIDATES];
};

struct CMD_RTT_INSTALL_LTF_KEYSEED {
	uint8_t  ucAddRemove;
	uint8_t  ucLtfKeyseedLen;
	uint16_t u2WlanIdx;
	uint8_t  aucLtfKeyseed[48];
};

struct EVENT_RTT_CAPABILITIES {
	struct RTT_CAPABILITIES rCapabilities;
};

struct EVENT_RTT_RESULT {
	struct RTT_RESULT rResult;
	uint16_t u2IELen;
	/* Keep it last */
	uint8_t aucIE[];
};

struct EVENT_RTT_DONE {
	uint8_t ucSeqNum;
};
#endif /* CFG_SUPPORT_RTT */

#if (CFG_COALESCING_INTERRUPT == 1)
/* parsing IPv4/IPv6 UDP/TCP header */
#define CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV4_TCP BIT(0)
#define CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV4_UDP BIT(1)
#define CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV6_TCP BIT(2)
#define CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV6_UDP BIT(3)
#define CMD_PF_CF_COALESCING_INT_FILTER_MASK_DEFAULT \
		(CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV4_TCP | \
		CMD_PF_CF_COALESCING_INT_FILTER_MASK_IPV4_UDP)

struct CMD_PF_CF_COALESCING_INT {
	/* DWORD_0 - Common Part */
	uint8_t	ucCmdVer;
	uint8_t	ucAction;
	uint16_t u2CmdLen;

	uint8_t ucPktThEn;
	uint8_t ucTmrThEn;
	uint16_t u2MaxPkt;
	uint16_t u2MaxTime;
	uint16_t u2FilterMask;
	uint8_t  aucReserved[64];
};

struct EVENT_PF_CF_COALESCING_INT_DONE {
	/* DWORD_0 - Common Part */
	uint8_t	ucEvtVer;
	uint8_t	ucAction;
	uint16_t u2EvtLen;

	uint8_t  fgEnable;
	uint8_t  aucPadding1[3];
	uint8_t  aucReserved[64];
};
#endif

#if (CFG_VOLT_INFO == 1)
struct CMD_SEND_VOLT_INFO_T {
	uint16_t u2Volt;
	uint8_t  aucReserved[2];
};

struct EVENT_GET_VOLT_INFO_T {
	uint16_t u2Volt;
	uint8_t  aucReserved[2];
};
#endif /* CFG_VOLT_INFO  */

/* Any change to this structure, please sync to struct PARAM_SER_INFO_T, too. */
struct EXT_EVENT_SER_T {
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

enum ENUM_KEEP_PWR_PARA {
	ENUM_KEEP_PWR_PHY_LMAC,
	ENUM_KEEP_PWR_PHY,
	ENUM_KEEP_PWR_LMAC,
	ENUM_KEEP_PWR_RFDIG
};

struct CMD_LP_DBG_CTRL {
	uint8_t ucSubCmdId;
	uint8_t ucTag;
	uint8_t ucBandIdx;
	uint8_t ucKeepPwr;	/* 0: PHY+LMAC, 1:PHY, 2:LMAC, 3:RFDIG */
	uint8_t ucRfdigStatus;
	uint8_t aucReserved[3];
};

#if (CFG_PCIE_GEN_SWITCH == 1)
struct CMD_UPDATA_LP_PARAM {
	uint8_t ucPcieTransitionStatus	;/*0: Init, 1: Start, 2: End*/
};
#endif

#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
struct CMD_UPDATE_PCIE_PARAM {
	uint8_t ucGen;
	uint8_t ucLane;
};
#endif /* CFG_EAP_PCIE_GEN_SWITCH */


#if (CFG_HW_DETECT_REPORT == 1)
struct EVENT_HW_DETECT_REPORT {
	bool fgIsReportNode;
	uint8_t aucReserved[3];
	uint8_t aucStrBuffer[HW_DETECT_REPORT_STR_MAX_LEN];
};
#endif

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
/* ucAction value options */
#define CMD_BT_CTRL_GET_COREDUMP_HEADER 0
#define CMD_BT_CTRL_GET_COREDUMP_DATA 1

struct EXT_CMD_BT_CTRL {
	uint8_t  ucAction;
	uint32_t u4Reserved[4];
};

struct EXT_EVENT_BT_CTRL {
	uint32_t u4Addr;
	uint32_t u4Length;
	uint32_t u4Round;
	uint32_t u4DumpLeave;
	uint32_t u4CurrentRound;
	uint32_t u4Done;
	uint32_t u4Reserved[20];
};

#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

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
#define NIC_FILL_CMD_TX_HDR(__prAd, __pucInfoBuffer, __u2InfoBufLen, \
	__ucCID, __ucPktTypeID, __pucSeqNum, __fgSetQuery, __ppCmdBuf, \
	__bucInitCmd, __ucExtCID, __ucS2DIndex) \
{ \
	struct mt66xx_chip_info *__prChipInfo; \
	struct WIFI_CMD_INFO __wifi_cmd_info; \
	__prChipInfo = __prAd->chip_info; \
	__wifi_cmd_info.pucInfoBuffer = __pucInfoBuffer; \
	__wifi_cmd_info.u2InfoBufLen = __u2InfoBufLen; \
	__wifi_cmd_info.ucCID = __ucCID; \
	__wifi_cmd_info.ucExtCID = __ucExtCID; \
	__wifi_cmd_info.ucPktTypeID = __ucPktTypeID; \
	__wifi_cmd_info.ucSetQuery = __fgSetQuery; \
	__wifi_cmd_info.ucS2DIndex = __ucS2DIndex; \
	if (__bucInitCmd) { \
		ASSERT(__prChipInfo->asicFillInitCmdTxd); \
		__prChipInfo->asicFillInitCmdTxd(__prAd, &(__wifi_cmd_info), \
			(&__u2InfoBufLen), __pucSeqNum, (void **)__ppCmdBuf); \
	} else { \
		ASSERT(__prChipInfo->asicFillCmdTxd); \
		__prChipInfo->asicFillCmdTxd(__prAd, &(__wifi_cmd_info), \
			__pucSeqNum, (void **)__ppCmdBuf); \
	} \
}

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void nicCmdEventQueryMcrRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryCfgRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventPfmuTagRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_TX_BF */

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf);

#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(struct ADAPTER *prAdapter,
		      struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicCmdEventGetCalcLq(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicCmdEventGetCalcInitMcs(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_MU_MIMO */
#endif /* CFG_SUPPORT_QA_TOOL */

void nicCmdEventQuerySwCtrlRead(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryChipConfig(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);

void nicCmdEventQueryRfTestATInfo(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf);

void nicCmdEventSetCommon(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventSetIpAddress(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

/* fos_change begin */
#if CFG_SUPPORT_SET_IPV6_NETWORK
void nicCmdEventSetIpv6Address(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif /* fos_change end */

void nicCmdEventQueryLinkQuality(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo,
				 uint8_t *pucEventBuf);

void nicUpdateStatistics(struct ADAPTER *prAdapter,
	struct PARAM_802_11_STATISTICS_STRUCT *prStatistics,
	struct EVENT_STATISTICS *prEventStatistics
);
void nicCmdEventQueryStatistics(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);

#if CFG_SUPPORT_LLS
void nicCmdEventQueryLinkStats(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf);
#endif

void nicCmdEventEnterRfTest(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventLeaveRfTest(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryMcastAddr(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryEepromRead(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);

void nicCmdEventSetStopSchedScan(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo,
				 uint8_t *pucEventBuf);

/* for timeout check */
void nicOidCmdTimeoutCommon(struct ADAPTER *prAdapter,
			    struct CMD_INFO *prCmdInfo);

void nicCmdTimeoutCommon(struct ADAPTER *prAdapter,
			 struct CMD_INFO *prCmdInfo);

void nicOidCmdEnterRFTestTimeout(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo);

#if CFG_SUPPORT_BUILD_DATE_CODE
void nicCmdEventBuildDateCode(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

void nicUpdateStaStats(struct ADAPTER *prAdapter,
	struct EVENT_STA_STATISTICS *prEvent,
	struct PARAM_GET_STA_STATISTICS *prStaStatistics,
	uint8_t ucStaRecIdx, bool fgIsMibDiff);

void nicCmdEventQueryStaStatistics(struct ADAPTER
				   *prAdapter, struct CMD_INFO *prCmdInfo,
				   uint8_t *pucEventBuf);

void nicCmdEventQueryBugReport(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

#if CFG_ENABLE_WIFI_DIRECT
void nicCmdEventQueryLteSafeChn(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf);
#endif

#if CFG_SUPPORT_BATCH_SCAN
void nicCmdEventBatchScanResult(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);
#endif

void nicEventRddPulseDump(struct ADAPTER *prAdapter,
			  uint8_t *pucEventBuf);

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
void nicCmdEventQueryTxPowerInfo(struct ADAPTER
				 *prAdapter, struct CMD_INFO *prCmdInfo,
				 uint8_t *pucEventBuf);
#endif

void nicCmdEventQueryWlanInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryMibInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);

void nicCmdEventQueryNicCapabilityV2(struct ADAPTER
				     *prAdapter, uint8_t *pucEventBuf);

uint32_t nicParsingNicCapV2(struct ADAPTER *prAdapter,
	uint32_t u4Type, uint8_t *pucEventBuf);

uint32_t nicCmdEventQueryNicTxResource(struct ADAPTER
				       *prAdapter, uint8_t *pucEventBuf);
uint32_t nicCmdEventQueryNicEfuseAddr(struct ADAPTER
				      *prAdapter, uint8_t *pucEventBuf);
uint32_t nicCmdEventQueryNicCoexFeature(struct ADAPTER
					*prAdapter, uint8_t *pucEventBuf);
#if CFG_TCP_IP_CHKSUM_OFFLOAD
uint32_t nicCmdEventQueryNicCsumOffload(struct ADAPTER
					*prAdapter, uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_PKT_OFLD == 1)
void nicCmdEventQueryOfldInfo(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);

void nicCmdEventQueryApfInfo(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo,
				uint8_t *pucEventBuf);
#endif

uint32_t nicCfgChipCapHwVersion(struct ADAPTER
				*prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipCapSwVersion(struct ADAPTER
				*prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMacAddr(struct ADAPTER *prAdapter,
			      uint8_t *pucEventBuf);
uint32_t nicCfgChipCapPhyCap(struct ADAPTER *prAdapter,
			     uint8_t *pucEventBuf);
uint32_t nicCfgChipCapLimited(struct ADAPTER *prAdapter,
				 uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMacCap(struct ADAPTER *prAdapter,
			     uint8_t *pucEventBuf);
uint32_t nicCfgChipCapFrameBufCap(struct ADAPTER
				  *prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipCapBeamformCap(struct ADAPTER
				  *prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipCapLocationCap(struct ADAPTER
				  *prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMuMimoCap(struct ADAPTER
				*prAdapter, uint8_t *pucEventBuf);
uint32_t nicCfgChipAdieHwVersion(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf);
#if CFG_SUPPORT_ANT_SWAP
uint32_t nicCfgChipCapAntSwpCap(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_P2PGO_ACS == 1)
uint32_t nicCfgChipP2PCap(struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);

#endif
uint32_t nicCfgChipCnmCap(struct ADAPTER *prAdapter, uint8_t *pucEventBuf);

uint32_t nicCmdEventHostStatusEmiOffset(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf);
uint32_t nicCfgChipCapFastPath(struct ADAPTER *prAdapter,
			       uint8_t *pucEventBuf);

#if CFG_SUPPORT_MLR
uint32_t nicCfgChipCapMlr(struct ADAPTER *prAdapter,
			       uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
#if (CFG_SUPPORT_QA_TOOL == 1)
uint32_t nicCmdEventTestmodeCap(struct ADAPTER
	  *prAdapter, uint8_t *pucEventBuf);
#endif
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
uint32_t nicCfgChipCap6GCap(struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);
#endif

#if CFG_SUPPORT_802_11BE_MLO
uint32_t nicCfgChipCapMLO(struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf);
#endif

#if CFG_SUPPORT_LLS
uint32_t nicCmdEventLinkStatsEmiOffset(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf);
#endif
uint32_t nicCmdEventCasanLoadType(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf);

#if CFG_ENABLE_FW_DOWNLOAD
#if IS_ENABLED(CFG_MTK_WIFI_SUPPORT_UDS_FWDL)
uint32_t nicCfgChipCapRedlInfo(struct ADAPTER *prAdapter,
			       uint8_t *pucEventBuf);
#endif
#endif

#if CFG_SUPPORT_REG_STAT_FROM_EMI
uint32_t nicCfgChipCapStatsRegMontrEmiOffset(
		struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);
#endif

#if (CFG_MTK_WIFI_SUPPORT_SW_SYNC_BY_EMI == 1)
uint32_t nicCfgGetSwSyncEMIOffset(
	struct ADAPTER *prAdapter,
	uint8_t *pucEventBuf);
#endif

#if CFG_SUPPORT_MBRAIN
uint32_t nicCfgChipMbrEmiInfo(
		struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);
#if CFG_SUPPORT_MBRAIN_BIGDATA
uint32_t nicCfgChipMbrBigDataVer(
		struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */
#endif

#if (CFG_SUPPORT_PERF_IND == 1)
uint32_t nicCfgChipCapPerfIndFromEMI(
		struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf);
#endif

uint32_t nicCmdEventHostSuspendInfo(struct ADAPTER *prAdapter,
					uint8_t *pucEventBuf);

void nicExtEventICapIQData(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf);
#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
void nicExtCmdEventSolicitICapIQData(struct ADAPTER *prAdapter,
					struct CMD_INFO *prCmdInfo,
					uint8_t *pucEventBuf);
#endif
void nicEventLinkQuality(struct ADAPTER *prAdapter,
			 struct WIFI_EVENT *prEvent);
void nicEventLayer0ExtMagic(struct ADAPTER *prAdapter,
			    struct WIFI_EVENT *prEvent);
void nicEventMicErrorInfo(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent);
void nicEventScanDone(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);
void nicEventSchedScanDone(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
void nicEventSleepyNotify(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent);
void nicExtEventPhyIcsDumpEmiRawData(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf);
void nicExtEventPhyIcsRawData(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf);
void nicEventBtOverWifi(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
void nicEventStatistics(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
void nicEventTputFactorHandler(struct ADAPTER *prAdapter,
		  struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_LLS
void nicEventStatsLinkStats(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
#endif
void nicEventWlanInfo(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);
void nicEventMibInfo(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
void nicEventBeaconTimeout(struct ADAPTER *prAdapter,
			   struct WIFI_EVENT *prEvent);
#if CFG_ENABLE_WIFI_DIRECT
void nicEventUpdateNoaParams(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent);
void nicEventStaAgingTimeout(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent);
void nicEventApObssStatus(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent);
#endif
void nicEventRoamingStatus(struct ADAPTER *prAdapter,
			   struct WIFI_EVENT *prEvent);
void nicEventSendDeauth(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
void nicEventUpdateRddStatus(struct ADAPTER *prAdapter,
			     struct WIFI_EVENT *prEvent);
void nicEventUpdateBwcsStatus(struct ADAPTER *prAdapter,
			      struct WIFI_EVENT *prEvent);
void nicEventUpdateBcmDebug(struct ADAPTER *prAdapter,
			    struct WIFI_EVENT *prEvent);
void nicEventAddPkeyDone(struct ADAPTER *prAdapter,
			 struct WIFI_EVENT *prEvent);
void nicEventDebugMsg(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_TDLS
void nicEventTdls(struct ADAPTER *prAdapter,
		  struct WIFI_EVENT *prEvent);
#endif
void nicEventRssiMonitor(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
void nicEventDumpMem(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
#if (CFG_CE_ASSERT_DUMP == 1)
void nicEventAssertDump(struct ADAPTER *prAdapter,
			struct WIFI_EVENT *prEvent);
#endif
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
void nicCmdEventQueryMdnsStats(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf);

void nicEventMdnsStats(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif
#endif

void nicEventHifCtrl(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
void nicEventRddSendPulse(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent);
void nicEventUpdateCoexPhyrate(struct ADAPTER *prAdapter,
			       struct WIFI_EVENT *prEvent);
void nicEventUpdateCoexStatus(struct ADAPTER *prAdapter,
			       struct WIFI_EVENT *prEvent);
uint32_t nicEventQueryTxResource_v1(struct ADAPTER
				    *prAdapter, uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResource_v2(struct ADAPTER
				    *prAdapter, uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResourceEntry(struct ADAPTER
				      *prAdapter, uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResource(struct ADAPTER
				 *prAdapter, uint8_t *pucEventBuf);
void nicCmdEventQueryCnmInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicEventCnmInfo(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
void nicEventCoexCtrl(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventDetectReplayInfo(struct ADAPTER *prAdapter,
		uint8_t ucKeyId, uint8_t ucKeyType, uint8_t ucBssIdx);
void nicCmdEventSetAddKey(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicOidCmdTimeoutSetAddKey(struct ADAPTER *prAdapter,
			       struct CMD_INFO *prCmdInfo);
#endif

#if CFG_SUPPORT_802_PP_DSCB
void nicEventUpdateStaticPPDscb(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif

#if CFG_SUPPORT_NAN
uint32_t nicNanAddNewTlvElement(uint32_t u4Tag, uint32_t u4BodyLen,
				uint32_t prCmdBufferLen,
				struct _CMD_EVENT_TLV_COMMOM_T *prCmdBuffer);

struct _CMD_EVENT_TLV_ELEMENT_T *
nicNanGetTargetTlvElement(uint16_t u2TargetTlvElement,
			  struct _CMD_EVENT_TLV_COMMOM_T *prCmdBuffer);

void nicNanEventDispatcher(struct ADAPTER *prAdapter,
			   struct WIFI_EVENT *prEvent);
void nicNanIOEventHandler(struct ADAPTER *prAdapter,
			  struct WIFI_EVENT *prEvent);
void nicNanGetCmdInfoQueryTestBuffer(
	struct _TXM_CMD_EVENT_TEST_T **prCmdInfoQueryTestBuffer);

void nicNanEventSTATxCTL(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf);

#if CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL
void nicNanNdlFlowCtrlEvt(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf);
void nicNanNdlFlowCtrlEvtV2(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf);
#endif

void nicNanVendorEventHandler(struct ADAPTER *prAdapter,
			      struct WIFI_EVENT *prEvent);
#endif /* CFG_SUPPORT_NAN */

void nicEventReportUEvent(struct ADAPTER *prAdapter,
		     struct WIFI_EVENT *prEvent);
#if (CFG_WOW_SUPPORT == 1)
void nicEventWowWakeUpReason(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif
#if (CFG_COALESCING_INTERRUPT == 1)
void nicEventCoalescingIntDone(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif
#if CFG_WIFI_TXPWR_TBL_DUMP
void nicCmdEventGetTxPwrTbl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf);
#endif

void tputEventFactorHandler(struct ADAPTER *prAdapter,
		  struct WIFI_EVENT *prEvent);

#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
uint32_t nicCfgChipPseRxQuota(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_TSF_SYNC == 1)
void nicCmdEventLatchTSF(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

#if CFG_SUPPORT_FW_DROP_SSN
void nicEventHandleFwDropSSN(struct ADAPTER *prAdapter,
	struct EVENT_STORED_FW_DROP_SSN_INFO *prSSN);
void nicEventFwDropSSN(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif /* CFG_SUPPORT_FW_DROP_SSN */

#if CFG_SUPPORT_BAR_DELAY_INDICATION
void nicEventHandleDelayBar(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
void nicCmdEventRttCapabilities(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicEventRttDone(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);
void nicEventRttResult(struct ADAPTER *prAdapter,
		      struct WIFI_EVENT *prEvent);

void nicEventHandleAddBa(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec, uint8_t ucTid,
			uint16_t u2WinSize, uint16_t u2WinStart);

#if (CFG_WIFI_ISO_DETECT == 1)
void nicCmdEventQueryCoexIso(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

#if (CFG_WIFI_GET_DPD_CACHE == 1)
void nicCmdEventQueryDpdCache(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
void nicCmdEventQueryTxMcsInfo(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicEventTxMcsInfo(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif

void nicCmdEventQuerySerInfo(struct ADAPTER *prAdapter,
			     struct CMD_INFO *prCmdInfo,
			     uint8_t *pucEventBuf);

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
void nicCmdEventListmode(struct ADAPTER
				  *prAdapter, struct CMD_INFO *prCmdInfo,
				  uint8_t *pucEventBuf);
#endif
#endif

#if (CFG_VOLT_INFO == 1)
void nicEventGetVnf(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif
#if CFG_SUPPORT_WIFI_POWER_METRICS
void nicEventPowerMetricsStatGetInfo(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif
void nicCmdEventGetSlpCntInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
void nicCmdEventLpKeepPwrCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf);
#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
void nicEventChannelTime(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
#endif

#if (CFG_HW_DETECT_REPORT == 1)
void nicEventHwDetectReport(struct ADAPTER *prAdapter,
		struct WIFI_EVENT *prEvent);
#endif

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
void nicCmdEventQueryBtCtrl(struct ADAPTER *prAdapter,
			    struct EXT_EVENT_BT_CTRL *prEvtBtCtrl);
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

void nicExtEventIcerDumpEmiRawData(struct ADAPTER *prAdapter,
				uint8_t *pucEventBuf);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _NIC_CMD_EVENT_H */
