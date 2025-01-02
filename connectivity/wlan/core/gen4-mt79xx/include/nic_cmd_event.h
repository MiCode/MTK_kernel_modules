/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/nic_cmd_event.h#1
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

#if (CFG_SUPPORT_802_11AX == 1)
#include "he_ie.h"
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

#define MAX_IE_LENGTH   (600)
#define MAX_WSC_IE_LENGTH   (400)


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

#if CFG_SUPPORT_QA_TOOL
#define IQ_FILE_LINE_OFFSET     18
#define IQ_FILE_IQ_STR_LEN	 8
#define RTN_IQ_DATA_LEN         1024	/* return 1k per packet */

#define MCAST_WCID_TO_REMOVE	0

/* Network type */
#define NETWORK_INFRA	BIT(16)
#define NETWORK_P2P		BIT(17)
#define NETWORK_IBSS	BIT(18)
#define NETWORK_MESH	BIT(19)
#define NETWORK_BOW		BIT(20)
#define NETWORK_WDS		BIT(21)

/* Station role */
#define STA_TYPE_STA BIT(0)
#define STA_TYPE_AP BIT(1)
#define STA_TYPE_ADHOC BIT(2)
#define STA_TYPE_TDLS BIT(3)
#define STA_TYPE_WDS BIT(4)

/* Connection type */
#define CONNECTION_INFRA_STA		(STA_TYPE_STA|NETWORK_INFRA)
#define CONNECTION_INFRA_AP		(STA_TYPE_AP|NETWORK_INFRA)
#define CONNECTION_P2P_GC			(STA_TYPE_STA|NETWORK_P2P)
#define CONNECTION_P2P_GO			(STA_TYPE_AP|NETWORK_P2P)
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
#define CONNECTION_P2P_DEVICE		(NETWORK_P2P)
#endif
#define CONNECTION_MESH_STA		(STA_TYPE_STA|NETWORK_MESH)
#define CONNECTION_MESH_AP		(STA_TYPE_AP|NETWORK_MESH)
#define CONNECTION_IBSS_ADHOC		(STA_TYPE_ADHOC|NETWORK_IBSS)
#define CONNECTION_TDLS	\
		(STA_TYPE_STA|NETWORK_INFRA|STA_TYPE_TDLS)
#define CONNECTION_WDS			(STA_TYPE_WDS|NETWORK_WDS)

#define ICAP_CONTENT_ADC		0x10000006
#define ICAP_CONTENT_TOAE		0x7
#define ICAP_CONTENT_SPECTRUM	0xB
#define ICAP_CONTENT_RBIST		0x10
#define ICAP_CONTENT_DCOC		0x20
#define ICAP_CONTENT_FIIQ		0x48
#define ICAP_CONTENT_FDIQ		0x49

#if CFG_SUPPORT_BUFFER_MODE

struct CMD_EFUSE_BUFFER_MODE {
	uint8_t ucSourceMode;
	uint8_t ucCount;
	uint8_t ucCmdType;  /* 0:6632, 1: 7668 */
	uint8_t ucReserved;
	uint8_t aBinContent[BUFFER_BIN_PAGE_SIZE];
};

struct CMD_EFUSE_BUFFER_MODE_CONNAC_T {
	uint8_t ucSourceMode;
	uint8_t ucContentFormat;
	uint16_t u2Count;
	uint8_t aBinContent[BUFFER_BIN_PAGE_SIZE];
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

#if CFG_SUPPORT_ADVANCE_CONTROL
/* command type */
#define CMD_ADV_CONTROL_SET (1<<15)
#define CMD_PTA_CONFIG_TYPE (0x1)
#define CMD_AFH_CONFIG_TYPE (0x2)
#define CMD_BA_CONFIG_TYPE (0x3)
#define CMD_GET_REPORT_TYPE (0x4)
#define CMD_NOISE_HISTOGRAM_TYPE (0x5)

/* get report related */
enum _ENUM_GET_REPORT_ACTION_T {
	CMD_GET_REPORT_ENABLE = 1,
	CMD_GET_REPORT_DISABLE,
	CMD_GET_REPORT_RESET,
	CMD_GET_REPORT_GET,
	CMD_SET_REPORT_SAMPLE_DUR,
	CMD_SET_REPORT_SAMPLE_POINT,
	CMD_SET_REPORT_TXTHRES,
	CMD_SET_REPORT_RXTHRES,
	CMD_GET_REPORT_ACTIONS
};

#define EVENT_REPORT_OFDM_FCCA (16)
#define EVENT_REPORT_OFDM_FCCA_FEILD (0xffff)
#define EVENT_REPORT_CCK_FCCA (0)
#define EVENT_REPORT_CCK_FCCA_FEILD (0xffff)
#define EVENT_REPORT_OFDM_SIGERR (16)
#define EVENT_REPORT_OFDM_SIGERR_FEILD (0xffff)
#define EVENT_REPORT_CCK_SIGERR (0)
#define EVENT_REPORT_CCK_SIGERR_FEILD (0xffff)

struct _CMD_ADV_CONFIG_HEADER {
	uint16_t u2Type;
	uint16_t u2Len;
};
#endif

#if CFG_SUPPORT_TRAFFIC_REPORT
/* latency - type - smallsize */
struct CMD_RLM_AIRTIME_MON {
	uint16_t u2Type;
	uint16_t u2Len;
#if (CFG_SUPPORT_TRAFFIC_REPORT == 2)
	/* parameter */
	uint8_t ucBand;
	uint8_t ucAction;
	uint8_t reserved[2];
	/* report 1 */
	uint32_t u4FalseCCA;
	uint32_t u4HdrCRC;
	uint32_t u4PktSent;
	uint32_t u4PktRetried;
	uint32_t u4PktTxfailed;
	uint32_t u4RxMPDU;
	uint32_t u4RxFcs;
	/* air time report */
	uint32_t u4FetchSt; /* ms */
	uint32_t u4FetchEd; /* ms */
	uint32_t u4ChBusy; /* us */
	uint32_t u4ChIdle; /* us */
	uint32_t u4TxAirTime; /* us */
	uint32_t u4RxAirTime; /* us */
	uint32_t u4phyRxTime; /* us */
	uint32_t u4TimerDur; /* ms */
	uint32_t u4FetchCost; /* us */
	int32_t TimerDrift; /* ms */
	int16_t u2SamplePoints; /* ms */
	int8_t ucTxThres; /* ms */
	int8_t ucRxThres; /* ms */
#else
	/* parameter */
	uint8_t ucBand;
	uint8_t ucAction;
	/* uint8_t reserved[2]; */ /* shift below 2 members replace reserved*/
	int8_t ucTxThres; /* ms */
	int8_t ucRxThres; /* ms */
	/* report 1 */
	uint32_t u4FalseCCA;
	uint32_t u4HdrCRC;
	uint16_t u2PktSent;
	uint16_t u2PktRetried;
	uint16_t u2PktTxfailed;
	uint16_t u2RxMPDU;
	uint16_t u2RxFcs;
	/* air time report */
	uint32_t u4FetchSt; /* ms */
	uint32_t u4FetchEd; /* ms */
	/* uint16_t u4ChBusy; */ /* us */
	uint16_t u2ChIdle; /* us */
	uint16_t u2TxAirTime; /* us */
	uint16_t u2RxAirTime; /* us */
	uint16_t u2TimerDur; /* ms */
	uint16_t u2FetchCost; /* us */
	int32_t TimerDrift; /* ms */
	int16_t i2SamplePoints; /* ms */
	int32_t i4total_sample_duration_sixSec; /* duration in 60s */
	int32_t i4total_chIdle_duration_sixSec; /* duration in 60s */
	int32_t i4total_TXTime_duration_sixSec; /* duration in 60s */
	int32_t i4total_RxTime_duration_sixSec; /* duration in 60s */
	int32_t i4total_OBSS_duration_sixSec;/* duration in 60s */
	/* int32_t i4total_nonWIFI_duration_sixSec; */
#endif
};
#endif

#if CONFIG_WIFI_ANTENNA_REPORT
struct CMD_ANTENNA_REPORT{
	uint16_t u2Type;
	uint16_t u2Len;
	/* parameter */
	uint8_t ucAction;
	uint32_t ucPeriodTime;           /* unit 50ms */
	/* WF0 */
	uint32_t u4WF0AntDivEn;
	uint32_t u4WF0Ant;              /* Ant0, Ant1 */
	uint32_t u4WF0AntCtrMode;       /* 0: Auto Mode, 1: Force Mode */
	uint32_t u4WF0AntForceMode;     /* 0: Per-Packet(HW), 1: Period Switch(SW) */
	uint32_t u4WF0Ant0PerPacketCount;
	uint32_t u4WF0Ant1PerPacketCount;
	uint32_t u4WF0Ant0PeriodSwitchCount;
	uint32_t u4WF0Ant1PeriodSwitchCount;
	/* WF1 */
	uint32_t u4WF1AntDivEn;
	uint32_t u4WF1Ant;              /* Ant0, Ant1 */
	uint32_t u4WF1AntCtrMode;       /* 0: Auto Mode, 1: Force Mode */
	uint32_t u4WF1AntForceMode;     /* 0: Per-Packet(HW), 1: Period Switch(SW) */
	uint32_t u4WF1Ant0PerPacketCount;
	uint32_t u4WF1Ant1PerPacketCount;
	uint32_t u4WF1Ant0PeriodSwitchCount;
	uint32_t u4WF1Ant1PeriodSwitchCount;
};

/* antenna report related */
enum ENUM_ANTENNA_REPORT_ACTION_T {
    CMD_ANTENNA_REPORT_GET = 1,
    CMD_ANTENNA_REPORT_RESET
};
#endif

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

	EXT_CMD_ID_EFUSE_FREE_BLOCK = 0x4F,
	EXT_CMD_ID_DUMP_MEM = 0x57,
	EXT_CMD_ID_TX_POWER_FEATURE_CTRL = 0x58,
	EXT_CMD_ID_SER = 0x81,
#if (CFG_SUPPORT_TWT == 1)
	EXT_CMD_ID_TWT_AGRT_UPDATE = 0x94,
#endif
	EXT_CMD_ID_SYSDVT_TEST = 0x99,
#if (CFG_SUPPORT_802_11AX == 1)
	EXT_CMD_ID_SR_CTRL = 0xA8,
#endif
	EXT_CMD_ID_CR4_DMASHDL_DVT = 0xAB,
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

#if CFG_WOW_SUPPORT

/* Filter Flag */
#define WOWLAN_FF_DROP_ALL                      BIT(0)
#define WOWLAN_FF_SEND_MAGIC_TO_HOST            BIT(1)
#define WOWLAN_FF_ALLOW_ARP                     BIT(2)
#define WOWLAN_FF_ALLOW_BMC                     BIT(3)
#define WOWLAN_FF_ALLOW_UC                      BIT(4)
#define WOWLAN_FF_ALLOW_1X                      BIT(5)
#define WOWLAN_FF_ALLOW_ARP_REQ2ME              BIT(6)

/* wow detect type */
#define WOWLAN_DETECT_TYPE_NONE                 0
#define WOWLAN_DETECT_TYPE_MAGIC                BIT(0)
#define WOWLAN_DETECT_TYPE_ANY                  BIT(1)
#define WOWLAN_DETECT_TYPE_DISCONNECT           BIT(2)
#define WOWLAN_DETECT_TYPE_GTK_REKEY_FAILURE    BIT(3)
#define WOWLAN_DETECT_TYPE_BCN_LOST             BIT(4)
#define WOWLAN_DETECT_TYPE_SCHD_SCAN_SSID_HIT   BIT(5)

/* Wakeup command bit define */
#define PF_WAKEUP_CMD_BIT0_OUTPUT_MODE_EN   BIT(0)
#define PF_WAKEUP_CMD_BIT1_OUTPUT_DATA      BIT(1)
#define PF_WAKEUP_CMD_BIT2_WAKEUP_LEVEL     BIT(2)

#define PM_WOWLAN_REQ_START         0x1
#define PM_WOWLAN_REQ_STOP          0x2

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

enum ENUM_PF_OPCODE {
	PF_OPCODE_ADD = 0,
	PF_OPCODE_DEL,
	PF_OPCODE_ENABLE,
	PF_OPCODE_DISABLE,
	PF_OPCODE_NUM
};

enum ENUM_SCN_FUNC_MASK {
	ENUM_SCN_RANDOM_MAC_EN = (1 << 0),
	ENUM_SCN_DBDC_SCAN_DIS = (1 << 1),
	ENUM_SCN_DBDC_SCAN_TYPE3 = (1 << 2),
	ENUM_SCN_USE_PADDING_AS_BSSID = (1 << 3),
	ENUM_SCN_RANDOM_SN_EN = (1 << 4),
};

enum ENUM_WOW_WAKEUP_REASON {
	ENUM_PF_CMD_TYPE_MAGIC                         = 0,
	ENUM_PF_CMD_TYPE_BITMAP                        = 1,
	ENUM_PF_CMD_TYPE_ARPNS                         = 2,
	ENUM_PF_CMD_TYPE_GTK_REKEY                     = 3,
	ENUM_PF_CMD_TYPE_COALESCING_FILTER             = 4,
	ENUM_PF_CMD_TYPE_HW_GLOBAL_ENABLE              = 5,
	ENUM_PF_CMD_TYPE_TCP_SYN                       = 6,
	ENUM_PF_CMD_TYPE_TDLS                          = 7,
	ENUM_PF_CMD_TYPE_DISCONNECT                    = 8,
	ENUM_PF_CMD_TYPE_IPV4_UDP                      = 9,
	ENUM_PF_CMD_TYPE_IPV4_TCP                      = 10,
	ENUM_PF_CMD_TYPE_IPV6_UDP                      = 11,
	ENUM_PF_CMD_TYPE_IPV6_TCP                      = 12,
	ENUM_PF_CMD_TYPE_BEACON_LOST                   = 13,
	ENUM_PF_CMD_TYPE_UNDEFINED                     = 255,
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
#if (CFG_SUPPORT_802_11AX == 1)
#define EXT_EVENT_ID_SR_INFO 0xA8
#endif
/*#endif*/

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
#define EXT_EVENT_ID_TX_POWER_FEATURE_CTRL  0x58
#endif

#define SCHED_SCAN_CHANNEL_TYPE_SPECIFIED      (0)
#define SCHED_SCAN_CHANNEL_TYPE_DUAL_BAND      (1)
#define SCHED_SCAN_CHANNEL_TYPE_2G4_ONLY       (2)
#define SCHED_SCAN_CHANNEL_TYPE_5G_ONLY        (3)

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
	TWT_AGRT_CTRL_RESET
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

	uint8_t aucBuffer[0];
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

	uint8_t aucBuffer[0];
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
	uint8_t ucNormalMode;
	uint8_t ucScreenOffMode;
	uint8_t ucDeviceSuspendMode;
	uint8_t arAddress[MAX_NUM_GROUP_ADDR][PARAM_MAC_ADDR_LEN];
};

enum ENUM_RXM_MCFILTER_L2_SETTING {
	RXM_MCFILTER_L2_APLLY_FW_DEFAULT = 0,
	RXM_MCFILTER_L2_DROP_ALL = 1,
	RXM_MCFILTER_L2_RX_WHITE_LIST = 2,
	RXM_MCFILTER_L2_RX_ALL = 3,
	RXM_MCFILTER_L2_MAX = 7
};

/** RXM MC Filter L3 settings */
enum ENUM_RXM_MCFILTER_L3_SETTING {
	RXM_MCFILTER_L3_APPLY_FW_DEFAULT = 0,
	RXM_MCFILTER_L3_DROP_ALL = 1,
	RXM_MCFILTER_L3_RX_IPV4_IN_L2_WHITELIST = 2,
	RXM_MCFILTER_L3_RX_IPV6_IN_L2_WHITELIST = 3,
	RXM_MCFILTER_L3_RX_IPV4_IPV6_IN_L2_WHITELIST = 4,
	RXM_MCFILTER_L3_RX_ALL = 5,
	RXM_MCFILTER_L3_MAX = 7
};

/* MC Filter */
#define MCFILTER_L2_SETTING_OFFSET 0
#define MCFILTER_L2_SETTING_MASK BITS(0, 3)
#define MCFILTER_L3_SETTING_OFFSET 4
#define MCFILTER_L3_SETTING_MASK BITS(4, 7)

/** RXM MC Filter Default Setting in normal mode */
#define RXM_MCFILTER_L2_NORMAL_DEFAULT_SETTING  RXM_MCFILTER_L2_RX_ALL
#define RXM_MCFILTER_L3_NORMAL_DEFAULT_SETTING  RXM_MCFILTER_L3_RX_ALL

/** RXM MC Filter Default Setting in screen-off mode */
#define RXM_MCFILTER_L2_SCRENNOFF_DEFAULT_SETTING \
	RXM_MCFILTER_L2_RX_WHITE_LIST
#define RXM_MCFILTER_L3_SCRENNOFF_DEFAULT_SETTING \
	RXM_MCFILTER_L3_RX_IPV4_IPV6_IN_L2_WHITELIST

/** RXM MC Filter Default Setting in device suspend mode */
#define RXM_MCFILTER_L2_DEVICESUSPEND_DEFAULT_SETTING \
	RXM_MCFILTER_L2_RX_WHITE_LIST
#define RXM_MCFILTER_L3_DEVICESUSPEND_DEFAULT_SETTING \
	RXM_MCFILTER_L3_RX_IPV4_IPV6_IN_L2_WHITELIST

#define MCFILTER_MERGE(_L2, _L3) ((_L3) << MCFILTER_L3_SETTING_OFFSET | _L2)
#define MCFILTER_GET_L2(_Byte) \
	(((_Byte) & MCFILTER_L2_SETTING_MASK) >> MCFILTER_L2_SETTING_OFFSET)
#define MCFILTER_GET_L3(_Byte) \
	(((_Byte) & MCFILTER_L3_SETTING_MASK) >> MCFILTER_L3_SETTING_OFFSET)
#define MCFILTER_SET_L2(_Byte, _ePolicy) \
	((_Byte) = (((_Byte) & ~(MCFILTER_L2_SETTING_MASK)) | \
	((_ePolicy) << MCFILTER_L2_SETTING_OFFSET)))
#define MCFILTER_SET_L3(_Byte, _ePolicy) \
	((_Byte) = (((_Byte) & ~(MCFILTER_L3_SETTING_MASK)) | \
	((_ePolicy) << MCFILTER_L3_SETTING_OFFSET)))

#define MCFILTER_COMPARE_L2_POLICY(_Byte, _ePolicy) \
	((MCFILTER_GET_L2(_Byte)) == _ePolicy)
#define MCFILTER_COMPARE_L3_POLICY(_Byte, _ePolicy) \
	((MCFILTER_GET_L3(_Byte)) == _ePolicy)

#define MCFILTER_NORMAL_DEFAULT \
	(MCFILTER_MERGE(RXM_MCFILTER_L2_NORMAL_DEFAULT_SETTING, \
	RXM_MCFILTER_L3_NORMAL_DEFAULT_SETTING))
#define MCFILTER_SCREEN_OFF_DEFAULT \
	(MCFILTER_MERGE(RXM_MCFILTER_L2_SCRENNOFF_DEFAULT_SETTING, \
	RXM_MCFILTER_L3_SCRENNOFF_DEFAULT_SETTING))
#define MCFILTER_DEVICE_SUSPNED_DEFAULT (MCFILTER_SCREEN_OFF_DEFAULT)

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
	uint8_t aucBuffer[0];
};

struct NIC_CAPABILITY_V2_ELEMENT {
	uint32_t tag_type; /* NIC_CAPABILITY_V2_TAG_T */
	uint32_t body_len;
	uint8_t aucbody[0];
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
	TAG_CAP_WFDMA_REALLOC = 0x17,
	TAG_CAP_6G_CAP = 0x18,
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

struct CAP_PHY_CAP {
	uint8_t ucHt; /* 1:support, 0:not*/
	uint8_t ucVht; /* 1:support, 0:not*/
	uint8_t uc5gBand; /* 1:support, 0:not*/
	/* 0: BW20, 1:BW40, 2:BW80, 3:BW160, 4:BW80+80 */
	uint8_t ucMaxBandwidth;
	uint8_t ucNss; /* 1:1x1, 2:2x2, ... */
	uint8_t ucDbdc; /* 1:support, 0:not*/
	uint8_t ucTxLdpc; /* 1:support, 0:not*/
	uint8_t ucRxLdpc; /* 1:support, 0:not*/
	uint8_t ucTxStbc; /* 1:support, 0:not*/
	uint8_t ucRxStbc; /* 1:support, 0:not*/
	/* BIT(0): 2G4_WF0, BIT(1): 5G_WF0, BIT(2): 2G4_WF1, BIT(3): 5G_WF1 */
	uint8_t ucHwWifiPath;
	uint8_t ucHe;
};

struct CAP_MAC_CAP {
	uint8_t ucHwBssIdNum; /* HW BSSID number */
	uint8_t ucWmmSet; /* 1: AC0~3, 2: AC0~3 and AC10~13, ... */
	uint8_t ucWtblEntryNum; /* WTBL entry number */
	uint8_t ucReserved;
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

struct CAP_WFDMA_REALLOC_T {
	uint8_t ucVersion;
	uint8_t ucIsSupportWfdmaRealloc;
	uint8_t aucReserved[2];
	uint32_t u4ReallocCmdTotalResource;
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
		(IN struct ADAPTER *prAdapter,
		 IN uint8_t *pucEventBuf);
	void (*nicTxResourceInit)(IN struct ADAPTER *prAdapter);
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

/* CMD_ID_ACCESS_REG & EVENT_ID_ACCESS_REG */
struct CMD_ACCESS_REG {
	uint32_t u4Address;
	uint32_t u4Data;
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

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
/* CMD_ID_CAL_BACKUP_IN_HOST_V2 & EVENT_ID_CAL_BACKUP_IN_HOST_V2 */
struct CMD_CAL_BACKUP_STRUCT_V2 {
	uint8_t	ucReason;
	uint8_t	ucAction;
	uint8_t	ucNeedResp;
	uint8_t	ucFragNum;
	uint8_t	ucRomRam;
	uint32_t	u4ThermalValue;
	uint32_t u4Address;
	uint32_t	u4Length;
	uint32_t	u4RemainLength;
	uint32_t	au4Buffer[PARAM_CAL_DATA_DUMP_MAX_NUM];
};

struct CMD_CAL_BACKUP_STRUCT {
	uint8_t	ucReason;
	uint8_t	ucAction;
	uint8_t	ucNeedResp;
	uint8_t	ucFragNum;
	uint8_t	ucRomRam;
	uint32_t	u4ThermalValue;
	uint32_t u4Address;
	uint32_t	u4Length;
	uint32_t	u4RemainLength;
};
#endif

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
	uint8_t aucBuffer[1];
};

#if CFG_SUPPORT_QA_TOOL
struct CMD_ACCESS_RX_STAT {
	uint32_t u4SeqNum;
	uint32_t u4TotalNum;
};

struct EVENT_ACCESS_RX_STAT {
	uint32_t u4SeqNum;
	uint32_t u4TotalNum;
	uint32_t au4Buffer[1];
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

#define CMD_DEVINFO_UPDATE_HDR_SIZE 8
struct CMD_DEV_INFO_UPDATE {
	uint8_t ucOwnMacIdx;
	uint8_t ucDbdcIdx;
	uint16_t u2TotalElementNum;
	uint8_t ucAppendCmdTLV;
	uint8_t aucReserve[3];
	uint8_t aucBuffer[0];
	/* CMD_DEVINFO_ACTIVE_T rCmdDevInfoActive; */
};

#define CMD_BSSINFO_UPDATE_HDR_SIZE 8
struct CMD_BSS_INFO_UPDATE {
	uint8_t ucBssIndex;
	uint8_t ucReserve;
	uint16_t u2TotalElementNum;
	uint32_t u4Reserve;
	/* CMD_BSSINFO_BASIC_T rCmdBssInfoBasic; */
	uint8_t aucBuffer[0];
};

/*  STA record command */
#define CMD_STAREC_UPDATE_HDR_SIZE 8
struct CMD_STAREC_UPDATE {
	uint8_t ucBssIndex;
	uint8_t ucWlanIdx;
	uint16_t u2TotalElementNum;
	uint32_t u4Reserve;
	uint8_t aucBuffer[0];
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
#endif /* CFG_SUPPORT_TX_BF */
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

#if CFG_SUPPORT_DFS_MASTER
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

struct CMD_RDD_ON_OFF_CTRL {
	uint8_t ucDfsCtrl;
	uint8_t ucRddIdx;
	uint8_t ucRddRxSel;
	uint8_t ucSetVal;
	uint8_t aucReserve[4];
};
#endif

struct CMD_PERF_IND {
	/* DWORD_0 - Common Part */
	uint8_t  ucCmdVer;
	uint8_t  aucPadding0[1];
	uint16_t u2CmdLen;       /* cmd size including common part and body. */
	/* DWORD_1 ~ x - Command Body */
	uint32_t u4VaildPeriod;   /* in ms */
	/* Current State */
	uint32_t ulCurTxBytes[4];   /* in Bps */
	uint32_t ulCurRxBytes[4];   /* in Bps */
	uint16_t u2CurRxRate[4];     /* Unit 500 Kbps */
	uint8_t ucCurRxRCPI0[4];
	uint8_t ucCurRxRCPI1[4];
	uint8_t ucCurRxNss[4];
	uint8_t ucCurRxNss2[4];
	uint32_t au4Reserve[62];
};

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
	uint8_t aucReserved[3];
};

/* CMD_SET_DOMAIN_INFO */
struct CMD_SET_DOMAIN_INFO {
	uint16_t u2CountryCode;
	uint16_t u2IsSetPassiveScan;
	struct CMD_SUBBAND_INFO rSubBand[MAX_SUBBAND_NUM];

	uint8_t uc2G4Bandwidth;	/* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t uc5GBandwidth;	/* CONFIG_BW_20_40M or CONFIG_BW_20M */
	uint8_t aucReserved[2];
};

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

/* CMD_SET_PWR_LIMIT_TABLE */
struct CMD_CHANNEL_POWER_LIMIT {
	uint8_t ucCentralCh;

	int8_t cPwrLimitCCK;
	int8_t cPwrLimit20L; /* MCS0~4 */
	int8_t cPwrLimit20H; /* MCS5~8 */
	int8_t cPwrLimit40L; /* MCS0~4 */
	int8_t cPwrLimit40H; /* MCS5~9 */
	int8_t cPwrLimit80L; /* MCS0~4 */
	int8_t cPwrLimit80H; /* MCS5~9 */
	int8_t cPwrLimit160L; /* MCS0~4 */
	int8_t cPwrLimit160H; /* MCS5~9 */

	uint8_t ucFlag; /*Not used in driver*/
	uint8_t aucReserved[1];
};

struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT {
	uint16_t u2CountryCode;
	uint8_t ucCountryFlag; /*Not used in driver*/
	uint8_t ucNum; /*Numbers of channel to set power limit*/
	uint8_t ucTempVersion; /*Temp use for 160nc power limit implementation*/
	uint8_t aucReserved[3];
	struct CMD_CHANNEL_POWER_LIMIT
		rChannelPowerLimit[1]; /*Channel power limit entries to be set*/
};

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
	struct CMD_CHANNEL_POWER_LIMIT_V2 rChannelPowerLimit[0];
};

#define BF_TX_PWR_LIMIT_SECTION_NUM 17
#define BF_TX_PWR_LIMIT_ELEMENT_NUM 10
#define TX_PWR_LIMIT_SECTION_NUM 15
#define TX_LEGACY_PWR_LIMIT_SECTION_NUM 4
#define TX_PWR_LIMIT_ELEMENT_NUM 12
#define TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN 4
#define TX_PWR_LIMIT_MAX_VAL 63

#define POWER_LIMIT_SKU_CCK_NUM 4
#define POWER_LIMIT_SKU_OFDM_NUM 8
#define POWER_LIMIT_SKU_OFDM40_NUM 8
#define POWER_LIMIT_SKU_OFDM80_NUM 8
#define POWER_LIMIT_SKU_OFDM160_NUM 8
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

struct CHANNEL_TX_PWR_LIMIT {
	uint8_t ucChannel;
	int8_t rTxPwrLimitValue[TX_PWR_LIMIT_SECTION_NUM]
		[TX_PWR_LIMIT_ELEMENT_NUM];
	int8_t rTxBfBackoff[POWER_LIMIT_TXBF_BACKOFF_PARAM_NUM];
};

struct CHANNEL_TX_LEGACY_PWR_LIMIT {
	uint8_t ucChannel;
	int8_t rTxLegacyPwrLimitValue[TX_LEGACY_PWR_LIMIT_SECTION_NUM]
		[TX_PWR_LIMIT_ELEMENT_NUM];
};

struct TX_PWR_LIMIT_DATA {
	uint32_t countryCode;
	uint32_t ucChNum;
	struct CHANNEL_TX_PWR_LIMIT *rChannelTxPwrLimit;
};

struct TX_PWR_LEGACY_LIMIT_DATA {
	uint32_t countryCode;
	uint32_t ucChNum;
	struct CHANNEL_TX_LEGACY_PWR_LIMIT *rChannelTxLegacyPwrLimit;
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
	uint8_t ucPacketSeq;
	uint8_t ucStatus;
	uint16_t u2SequenceNumber;

	uint8_t ucWlanIndex;
	uint8_t ucTxCount;
	uint16_t u2TxRate;

	uint8_t ucFlag;
	uint8_t ucTid;
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
	uint8_t ucReserved;
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
	uint8_t ucRfCenterFreqSeg1;	/* To support 80/160MHz bandwidth */
	uint8_t ucRfCenterFreqSeg2;	/* To support 80/160MHz bandwidth */
	uint8_t ucReqType;
	uint8_t ucDBDCBand;
	uint8_t aucReserved;
	uint32_t u4MaxInterval;	/* In unit of ms */

	/* record original 20 /40 /80/160MHz bandwidth form AP's IE */
	uint8_t ucRfChannelWidthFromAP;
	uint8_t ucRfCenterFreqSeg1FromAP;
	uint8_t ucRfCenterFreqSeg2FromAP;
	uint8_t aucReserved2[5];
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
	uint8_t aucPadding2[1];
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
	uint32_t u4OutPRIStgDmin; /* Staggered PRF radar: min PRI Difference between 1st and 2nd  */
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
	uint8_t aucMsg[1];
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
#if CFG_SUPPORT_ADVANCE_CONTROL
struct CMD_ADV_CONFIG_HEADER {
	uint16_t u2Type;
	uint16_t u2Len;
};
#endif
#if CFG_WIFI_SUPPORT_NOISE_HISTOGRAM
/* noise histogram related */
enum ENUM_NOISE_HISTOGRAM_ACTION_T {
	CMD_NOISE_HISTOGRAM_ENABLE = 1,
	CMD_NOISE_HISTOGRAM_DISABLE,
	CMD_NOISE_HISTOGRAM_RESET,
	CMD_NOISE_HISTOGRAM_GET,
#if CFG_IPI_2CHAIN_SUPPORT
	CMD_NOISE_HISTOGRAM_GET2
#endif
};
struct CMD_NOISE_HISTOGRAM_REPORT {
	uint16_t u2Type;
	uint16_t u2Len;
	/* parameter */
	uint8_t ucAction;
	uint8_t reserved[3];
	/* IPI_report */
	uint32_t u4IPI0;  /* Power <= -92 */
	uint32_t u4IPI1;  /* -92 < Power <= -89 */
	uint32_t u4IPI2;  /* -89 < Power <= -86 */
	uint32_t u4IPI3;  /* -86 < Power <= -83 */
	uint32_t u4IPI4;  /* -83 < Power <= -80 */
	uint32_t u4IPI5;  /* -80 < Power <= -75 */
	uint32_t u4IPI6;  /* -75 < Power <= -70 */
	uint32_t u4IPI7;  /* -70 < Power <= -65 */
	uint32_t u4IPI8;  /* -65 < Power <= -60 */
	uint32_t u4IPI9;  /* -60 < Power <= -55 */
	uint32_t u4IPI10; /* -55 < Power  */
};
#endif

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
	uint8_t aucIE[0];             /* MUST be the last for IE content */
};

struct EVENT_SCHED_SCAN_DONE {
	uint8_t ucSeqNum;
	uint8_t ucStatus;
	uint8_t aucReserved[2];
};

#if CFG_SUPPORT_CSI
enum CSI_EVENT_TLV_TAG {
	CSI_EVENT_VERSION,
	CSI_EVENT_CBW,
	CSI_EVENT_RSSI,
	CSI_EVENT_SNR,
	CSI_EVENT_BAND,
	CSI_EVENT_CSI_NUM,
	CSI_EVENT_CSI_I_DATA,
	CSI_EVENT_CSI_Q_DATA,
	CSI_EVENT_DBW,
	CSI_EVENT_CH_IDX,
	CSI_EVENT_TA,
	CSI_EVENT_EXTRA_INFO,
	CSI_EVENT_RX_MODE,
	CSI_EVENT_RSVD1,
	CSI_EVENT_RSVD2,
	CSI_EVENT_RSVD3,
	CSI_EVENT_RSVD4,
	CSI_EVENT_H_IDX,
	CSI_EVENT_TX_RX_IDX,    /* csi event end, must be exist */
	CSI_EVENT_TLV_TAG_NUM,
};

#define CSI_EVENT_RX_IDX_MASK	BITS(0, 15)
#define CSI_EVENT_RX_IDX_SHFIT	0
#define CSI_EVENT_TX_IDX_MASK	BITS(16, 31)
#define CSI_EVENT_TX_IDX_SHFIT	16
#define CSI_EVENT_RX_MODE_MASK  BITS(0, 15)
#define CSI_EVENT_RX_MODE_SHFIT 0
#define CSI_EVENT_RATE_MASK	BITS(16, 31)
#define CSI_EVENT_RATE_SHFIT    16

#endif

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
	uint8_t aucReserved2[32];
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
#if (CFG_SUPPORT_CONNAC2X == 0)
	uint8_t aucReserved5[24];
#else
	uint32_t u4AggRangeCtrl_2;
	uint32_t u4AggRangeCtrl_3;
	uint8_t aucReserved5[16];
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

struct CMD_STATS_LOG {
	uint32_t u4DurationInMs;
	uint8_t aucReserved[32];
};

struct EVENT_WIFI_RDD_TEST {
	uint32_t u4FuncIndex;
	uint32_t u4FuncLength;
	uint32_t u4Prefix;
	uint32_t u4Count;
	uint8_t ucRddIdx;
	uint8_t aucReserve[3];
	uint8_t aucBuffer[0];
};

#if (CFG_SUPPORT_ICS == 1)
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
struct EVENT_MIB_INFO {
	struct HW_MIB_COUNTER	    rHwMibCnt;
	struct HW_MIB2_COUNTER	    rHwMib2Cnt;
	struct HW_MIB3_COUNTER	    rHwMib3Cnt;
	struct HW_TX_AMPDU_METRICS	    rHwTxAmpduMts;

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

#if (CFG_WIFI_GET_DPD_CACHE == 1)
struct EVENT_GET_DPD_CACHE {
	uint8_t		ucDpdCacheNum;
	uint8_t		ucReserved[3];
	uint32_t	u4DpdCacheCh[PER_CH_CAL_CACHE_NUM];
	uint8_t		ucDpdCachePath[PER_CH_CAL_CACHE_NUM];
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
	uint8_t  aucEvent[0];
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
	uint32_t u4Data[256];
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
	/* DW7 ~ DW10 */
	uint16_t au2StaList[TWT_GRP_MAX_MEMBER_CNT];
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

struct EXT_EVENT_MAX_AMSDU_LENGTH_UPDATE {
	uint8_t ucWlanIdx;
	uint8_t ucAmsduLen;
};

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

#if CFG_SUPPORT_CSI

#define CSI_INFO_RSVD1 BIT(0)
#define CSI_INFO_RSVD2 BIT(1)

struct CSI_TLV_ELEMENT {
	uint32_t tag_type;
	uint32_t body_len;
	uint8_t aucbody[0];
};

enum CSI_CONTROL_MODE_T {
	CSI_CONTROL_MODE_STOP,
	CSI_CONTROL_MODE_START,
	CSI_CONTROL_MODE_SET,
	CSI_CONTROL_MODE_NUM
};

enum CSI_CONFIG_ITEM_T {
	CSI_CONFIG_RSVD1,
	CSI_CONFIG_WF,
	CSI_CONFIG_RSVD2,
	CSI_CONFIG_FRAME_TYPE,
	CSI_CONFIG_TX_PATH,
	CSI_CONFIG_OUTPUT_FORMAT,
	CSI_CONFIG_INFO,
	CSI_CONFIG_ITEM_NUM
};

struct CMD_CSI_CONTROL_T {
	uint8_t ucBandIdx;
	uint8_t ucMode;
	uint8_t ucCfgItem;
	uint8_t ucValue1;
	uint8_t ucValue2;
};

enum CSI_OUTPUT_FORMAT_T {
	CSI_OUTPUT_RAW,
	CSI_OUTPUT_TONE_MASKED,
	CSI_OUTPUT_TONE_MASKED_SHIFTED,
	CSI_OUTPUT_FORMAT_NUM
};

#endif

#if CFG_SUPPORT_NAN
struct _CMD_EVENT_TLV_COMMOM_T {
	uint16_t u2TotalElementNum;
	uint8_t aucReserved[2];
	uint8_t aucBuffer[0];
};

struct _CMD_EVENT_TLV_ELEMENT_T {
	uint32_t tag_type;
	uint32_t body_len;
	uint8_t aucbody[0];
};

struct _TXM_CMD_EVENT_TEST_T {
	uint32_t u4TestValue0;
	uint32_t u4TestValue1;
	uint8_t ucTestValue2;
	uint8_t aucReserved[3]; /*For 4 bytes alignment*/
};

struct _NAN_CMD_MASTER_PREFERENCE_T {
	uint8_t ucMasterPreference;
	uint8_t aucReserved[3];
};

struct EVENT_UPDATE_NAN_TX_STATUS {
	uint8_t aucFlowCtrl[CFG_STA_REC_NUM];
};

struct _NAN_CMD_UPDATE_ATTR_STRUCT {
	uint8_t ucAttrId;
	uint16_t u2AttrLen;
	uint8_t aucAttrBuf[1024];
};

enum _ENUM_NAN_SUB_CMD {
	NAN_CMD_TEST, /* 0 */
	NAN_TXM_TEST,
	NAN_CMD_MASTER_PREFERENCE,
	NAN_CMD_HOP_COUNT,
	NAN_CMD_PUBLISH,
	NAN_CMD_CANCEL_PUBLISH, /* 5 */
	NAN_CMD_UPDATE_PUBLISH,
	NAN_CMD_SUBSCRIBE,
	NAN_CMD_CANCEL_SUBSCRIBE,
	NAN_CMD_TRANSMIT,
	NAN_CMD_ENABLE_REQUEST, /* 10 */
	NAN_CMD_DISABLE_REQUEST,
	NAN_CMD_UPDATE_AVAILABILITY,
	NAN_CMD_UPDATE_CRB,
	NAN_CMD_CRB_HANDSHAKE_TOKEN,
	NAN_CMD_MANAGE_PEER_SCH_RECORD, /* 15 */
	NAN_CMD_MAP_STA_RECORD,
	NAN_CMD_RANGING_REPORT_DISC,
	NAN_CMD_FTM_PARAM,
	NAN_CMD_UPDATE_PEER_UAW,
	NAN_CMD_UPDATE_ATTR, /* 20 */
	NAN_CMD_UPDATE_PHY_SETTING,
	NAN_CMD_UPDATE_POTENTIAL_CHNL_LIST,
	NAN_CMD_UPDATE_AVAILABILITY_CTRL,
	NAN_CMD_UPDATE_PEER_CAPABILITY,
	NAN_CMD_ADD_CSID,
	NAN_CMD_MANAGE_SCID,

	NAN_CMD_NUM
};

enum _ENUM_NAN_SUB_EVENT {
	NAN_EVENT_TEST, /* 0 */
	NAN_EVENT_DISCOVERY_RESULT,
	NAN_EVENT_FOLLOW_EVENT,
	NAN_EVENT_MASTER_IND_ATTR,
	NAN_EVENT_CLUSTER_ID_UPDATE,
	NAN_EVENT_REPLIED_EVENT, /* 5 */
	NAN_EVENT_PUBLISH_TERMINATE_EVENT,
	NAN_EVENT_SUBSCRIBE_TERMINATE_EVENT,
	NAN_EVENT_ID_SCHEDULE_CONFIG,
	NAN_EVENT_ID_PEER_AVAILABILITY,
	NAN_EVENT_ID_PEER_CAPABILITY, /* 10 */
	NAN_EVENT_ID_CRB_HANDSHAKE_TOKEN,
	NAN_EVENT_ID_DATA_NOTIFY,
	NAN_EVENT_FTM_DONE,
	NAN_EVENT_RANGING_BY_DISC,
	NAN_EVENT_NDL_FLOW_CTRL, /* 15 */
	NAN_EVENT_DW_INTERVAL,
	NAN_EVENT_NDL_DISCONNECT,
	NAN_EVENT_ID_PEER_CIPHER_SUITE_INFO,
	NAN_EVENT_ID_PEER_SEC_CONTEXT_INFO,
	NAN_EVENT_ID_DE_EVENT_IND,  /* 20 */
	NAN_EVENT_SELF_FOLLOW_EVENT,
	NAN_EVENT_DISABLE_IND,

	NAN_EVENT_NUM
};
#endif
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
	__bucInitCmd, __ucExtCID, __ucS2DIndex, __ucExtCmdOption) \
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
	__wifi_cmd_info.ucExtCmdOption = __ucExtCmdOption; \
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

#if CFG_SUPPORT_CSI

#define GET_CSI_RX_IDX(TRX_IDX)	\
	((TRX_IDX & CSI_EVENT_RX_IDX_MASK) >> CSI_EVENT_RX_IDX_SHFIT)

#define GET_CSI_TX_IDX(TRX_IDX)	\
	((TRX_IDX & CSI_EVENT_TX_IDX_MASK) >> CSI_EVENT_TX_IDX_SHFIT)

#define GET_CSI_RX_MODE(DATA) \
	((DATA & CSI_EVENT_RX_MODE_MASK) >> CSI_EVENT_RX_MODE_SHFIT)

#define GET_CSI_RATE(DATA) \
	((DATA & CSI_EVENT_RATE_MASK) >> CSI_EVENT_RATE_SHFIT)

#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
void nicCmdEventQueryMcrRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

#if CFG_SUPPORT_QA_TOOL
void nicCmdEventQueryRxStatistics(IN struct ADAPTER
				  *prAdapter, IN struct CMD_INFO *prCmdInfo,
				  IN uint8_t *pucEventBuf);

uint32_t nicTsfRawData2IqFmt(struct EVENT_DUMP_MEM *prEventDumpMem,
	struct ICAP_INFO_T *prIcap);
uint32_t nicExtTsfRawData2IqFmt(
	struct EXT_EVENT_RBIST_DUMP_DATA_T *prEventDumpMem,
	struct ICAP_INFO_T *prIcap);

int32_t GetIQData(struct ADAPTER *prAdapter,
		  int32_t **prIQAry, uint32_t *prDataLen, uint32_t u4IQ,
		  uint32_t u4GetWf1);

#if CFG_SUPPORT_TX_BF
void nicCmdEventPfmuDataRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventPfmuTagRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_TX_BF */
#if CFG_SUPPORT_MU_MIMO
void nicCmdEventGetQd(IN struct ADAPTER *prAdapter,
		      IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicCmdEventGetCalcLq(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicCmdEventGetCalcInitMcs(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_MU_MIMO */
#endif /* CFG_SUPPORT_QA_TOOL */

#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
void nicCmdEventQueryCalBackupV2(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf);
#endif
#if 0
void nicEventQueryMemDump(IN struct ADAPTER *prAdapter,
			  IN uint8_t *pucEventBuf);
#endif

void nicCmdEventQueryMemDump(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQuerySwCtrlRead(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);

void nicCmdEventQueryChipConfig(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);

void nicCmdEventQueryRfTestATInfo(IN struct ADAPTER
				  *prAdapter, IN struct CMD_INFO *prCmdInfo,
				  IN uint8_t *pucEventBuf);

void nicCmdEventSetCommon(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventSetIpAddress(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryLinkQuality(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf);

void nicCmdEventQueryLinkSpeed(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryLinkSpeedEx(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryStatistics(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);

void nicCmdEventEnterRfTest(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventLeaveRfTest(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryMcastAddr(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryEepromRead(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);

void nicCmdEventSetMediaStreamMode(IN struct ADAPTER
				   *prAdapter, IN struct CMD_INFO *prCmdInfo,
				   IN uint8_t *pucEventBuf);

void nicCmdEventSetStopSchedScan(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf);

/* for timeout check */
void nicOidCmdTimeoutCommon(IN struct ADAPTER *prAdapter,
			    IN struct CMD_INFO *prCmdInfo);

void nicCmdTimeoutCommon(IN struct ADAPTER *prAdapter,
			 IN struct CMD_INFO *prCmdInfo);

void nicOidCmdEnterRFTestTimeout(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo);

#if CFG_SUPPORT_BUILD_DATE_CODE
void nicCmdEventBuildDateCode(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif

void nicCmdEventQueryStaStatistics(IN struct ADAPTER
				   *prAdapter, IN struct CMD_INFO *prCmdInfo,
				   IN uint8_t *pucEventBuf);

void nicCmdEventQueryBugReport(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryLteSafeChn(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo,
	IN uint8_t *pucEventBuf);
#if CFG_SUPPORT_ADVANCE_CONTROL
void nicCmdEventQueryAdvCtrl(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);
#endif
#if CFG_SUPPORT_BATCH_SCAN
void nicCmdEventBatchScanResult(IN struct ADAPTER
				*prAdapter, IN struct CMD_INFO *prCmdInfo,
				IN uint8_t *pucEventBuf);
#endif

void nicEventRddPulseDump(IN struct ADAPTER *prAdapter,
			  IN uint8_t *pucEventBuf);

#if (CFG_SUPPORT_TXPOWER_INFO == 1)
void nicCmdEventQueryTxPowerInfo(IN struct ADAPTER
				 *prAdapter, IN struct CMD_INFO *prCmdInfo,
				 IN uint8_t *pucEventBuf);
#endif

void nicCmdEventQueryWlanInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

void nicCmdEventQueryMibInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
void nicCmdEventQueryCfgRead(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicParsingNicCapV2(IN struct ADAPTER *prAdapter,
	IN uint32_t u4Type, IN uint8_t *pucEventBuf);
#endif
void nicCmdEventQueryNicCapabilityV2(IN struct ADAPTER
				     *prAdapter, IN uint8_t *pucEventBuf);

uint32_t nicCmdEventQueryNicTxResource(IN struct ADAPTER
				       *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCmdEventQueryNicEfuseAddr(IN struct ADAPTER
				      *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCmdEventQueryNicCoexFeature(IN struct ADAPTER
					*prAdapter, IN uint8_t *pucEventBuf);
#if CFG_TCP_IP_CHKSUM_OFFLOAD
uint32_t nicCmdEventQueryNicCsumOffload(IN struct ADAPTER
					*prAdapter, IN uint8_t *pucEventBuf);
#endif
uint32_t nicCfgChipCapHwVersion(IN struct ADAPTER
				*prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapSwVersion(IN struct ADAPTER
				*prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMacAddr(IN struct ADAPTER *prAdapter,
			      IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapPhyCap(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMacCap(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapFrameBufCap(IN struct ADAPTER
				  *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapBeamformCap(IN struct ADAPTER
				  *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapLocationCap(IN struct ADAPTER
				  *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipCapMuMimoCap(IN struct ADAPTER
				*prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicCfgChipAdieHwVersion(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf);
#if CFG_SUPPORT_ANT_SWAP
uint32_t nicCfgChipCapAntSwpCap(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_WFDMA_REALLOC == 1)
uint32_t nicCmdEventQueryNicWfdmaRealloc(IN struct ADAPTER *prAdapter,
					 IN uint8_t *pucEventBuf);
#endif /* CFG_SUPPORT_WFDMA_REALLOC */

#if (CFG_SUPPORT_WIFI_6G == 1)
uint32_t nicCfgChipCap6GCap(IN struct ADAPTER *prAdapter,
	IN uint8_t *pucEventBuf);
#endif

void nicExtEventICapIQData(IN struct ADAPTER *prAdapter,
			   IN uint8_t *pucEventBuf);
void nicExtEventQueryMemDump(IN struct ADAPTER *prAdapter,
			     IN uint8_t *pucEventBuf);
void nicEventLinkQuality(IN struct ADAPTER *prAdapter,
			 IN struct WIFI_EVENT *prEvent);
void nicEventLayer0ExtMagic(IN struct ADAPTER *prAdapter,
			    IN struct WIFI_EVENT *prEvent);
void nicEventMicErrorInfo(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent);
void nicEventScanDone(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent);
void nicEventSchedScanDone(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
void nicEventSleepyNotify(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent);
void nicEventBtOverWifi(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
void nicEventStatistics(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
void nicEventWlanInfo(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent);
void nicEventMibInfo(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent);
void nicEventBeaconTimeout(IN struct ADAPTER *prAdapter,
			   IN struct WIFI_EVENT *prEvent);
void nicEventUpdateNoaParams(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent);
void nicEventStaAgingTimeout(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent);
void nicEventApObssStatus(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent);
void nicEventRoamingStatus(IN struct ADAPTER *prAdapter,
			   IN struct WIFI_EVENT *prEvent);
void nicEventSendDeauth(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
void nicEventUpdateRddStatus(IN struct ADAPTER *prAdapter,
			     IN struct WIFI_EVENT *prEvent);
void nicEventUpdateBwcsStatus(IN struct ADAPTER *prAdapter,
			      IN struct WIFI_EVENT *prEvent);
void nicEventUpdateBcmDebug(IN struct ADAPTER *prAdapter,
			    IN struct WIFI_EVENT *prEvent);
void nicEventAddPkeyDone(IN struct ADAPTER *prAdapter,
			 IN struct WIFI_EVENT *prEvent);
void nicEventIcapDone(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_CAL_RESULT_BACKUP_TO_HOST
void nicEventCalAllDone(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
#endif

void nicEventDebugMsg(IN struct ADAPTER *prAdapter,
		      IN struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_TDLS
void nicEventTdls(IN struct ADAPTER *prAdapter,
		  IN struct WIFI_EVENT *prEvent);
#endif
void nicEventRssiMonitor(IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent);
void nicEventDumpMem(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent);
void nicEventAssertDump(IN struct ADAPTER *prAdapter,
			IN struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_ONE_TIME_CAL
void nicEventGetOneTimeCalData(
	IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent
);
#endif

void nicEventHifCtrl(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent);
void nicEventRddSendPulse(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent);
void nicEventUpdateCoexPhyrate(IN struct ADAPTER *prAdapter,
			       IN struct WIFI_EVENT *prEvent);
uint32_t nicEventQueryTxResource_v1(IN struct ADAPTER
				    *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResource_v2(IN struct ADAPTER
				    *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResourceEntry(IN struct ADAPTER
				      *prAdapter, IN uint8_t *pucEventBuf);
uint32_t nicEventQueryTxResource(IN struct ADAPTER
				 *prAdapter, IN uint8_t *pucEventBuf);
void nicCmdEventQueryCnmInfo(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicEventCnmInfo(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent);
void nicEventCoexCtrl(IN struct ADAPTER *prAdapter,
		     IN struct WIFI_EVENT *prEvent);
#if CFG_SUPPORT_REPLAY_DETECTION
void nicCmdEventSetAddKey(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicOidCmdTimeoutSetAddKey(IN struct ADAPTER *prAdapter,
			       IN struct CMD_INFO *prCmdInfo);
#endif
#if (CFG_WOW_SUPPORT == 1)
void nicEventWowWakeUpReason(IN struct ADAPTER *prAdapter,
	IN struct WIFI_EVENT *prEvent);
#endif
#if (CFG_COALESCING_INTERRUPT == 1)
void nicEventCoalescingIntDone(IN struct ADAPTER *prAdapter,
		IN struct WIFI_EVENT *prEvent);
#endif
#if CFG_WIFI_TXPWR_TBL_DUMP
void nicCmdEventGetTxPwrTbl(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo,
		IN uint8_t *pucEventBuf);
#endif

void nicCmdEventQuerySerInfo(IN struct ADAPTER *prAdapter,
			     IN struct CMD_INFO *prCmdInfo,
			     IN uint8_t *pucEventBuf);

#if (CFG_WIFI_ISO_DETECT == 1)
void nicCmdEventQueryCoexIso(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
void nicCmdEventQueryTxMcsInfo(IN struct ADAPTER *prAdapter,
		IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
void nicEventTxMcsInfo(IN struct ADAPTER *prAdapter,
		IN struct WIFI_EVENT *prEvent);
#endif

#if CFG_SUPPORT_NAN
struct _CMD_EVENT_TLV_ELEMENT_T *
nicGetTargetTlvElement(IN uint16_t u2TargetTlvElement, IN void *prCmdBuffer);

uint32_t nicAddNewTlvElement(IN uint32_t u4Tag, IN uint32_t u4BodyLen,
			     IN uint32_t prCmdBufferLen, IN void *prCmdBuffer);

uint32_t nicDumpTlv(IN void *prCmdBuffer);
void nicNanEventTestProcess(IN struct ADAPTER *prAdapter,
			    IN struct WIFI_EVENT *prEvent);
void nicNanEventDispatcher(IN struct ADAPTER *prAdapter,
			   IN struct WIFI_EVENT *prEvent);
void nicNanIOEventHandler(IN struct ADAPTER *prAdapter,
			  IN struct WIFI_EVENT *prEvent);
void nicNanGetCmdInfoQueryTestBuffer(
	struct _TXM_CMD_EVENT_TEST_T **prCmdInfoQueryTestBuffer);
void nicNanTestQueryInfoDone(IN struct ADAPTER *prAdapter,
			     IN struct CMD_INFO *prCmdInfo,
			     IN uint8_t *pucEventBuf);
void nicNanEventSTATxCTL(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf);
void nicNanNdlFlowCtrlEvt(IN struct ADAPTER *prAdapter, IN uint8_t *pcuEvtBuf);
void nicNanVendorEventHandler(IN struct ADAPTER *prAdapter,
			      IN struct WIFI_EVENT *prEvent);
#endif
#if (CFG_WIFI_GET_DPD_CACHE == 1)
void nicCmdEventQueryDpdCache(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif

#if (CFG_SUPPORT_TSF_SYNC == 1)
void nicCmdEventLatchTSF(IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo, IN uint8_t *pucEventBuf);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _NIC_CMD_EVENT_H */
