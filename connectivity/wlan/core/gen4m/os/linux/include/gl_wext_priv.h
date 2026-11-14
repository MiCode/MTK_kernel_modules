/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_wext_priv.h
 *    \brief  This file includes private ioctl support.
 */


#ifndef _GL_WEXT_PRIV_H
#define _GL_WEXT_PRIV_H
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
/* If it is set to 1, iwpriv will support register read/write */
#define CFG_SUPPORT_PRIV_MCR_RW         1

/*******************************************************************************
 *			E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
/* New wireless extensions API - SET/GET convention (even ioctl numbers are
 * root only)
 */
#define IOCTL_SET_INT                   (SIOCIWFIRSTPRIV + 0)
#define IOCTL_GET_INT                   (SIOCIWFIRSTPRIV + 1)

#define IOCTL_SET_ADDRESS               (SIOCIWFIRSTPRIV + 2)
#define IOCTL_GET_ADDRESS               (SIOCIWFIRSTPRIV + 3)
#define IOCTL_SET_STR                   (SIOCIWFIRSTPRIV + 4)
#define IOCTL_GET_STR                   (SIOCIWFIRSTPRIV + 5)
#define IOCTL_SET_KEY                   (SIOCIWFIRSTPRIV + 6)
#define IOCTL_GET_KEY                   (SIOCIWFIRSTPRIV + 7)
#define IOCTL_SET_STRUCT                (SIOCIWFIRSTPRIV + 8)
#define IOCTL_GET_STRUCT                (SIOCIWFIRSTPRIV + 9)
#define IOCTL_SET_STRUCT_FOR_EM         (SIOCIWFIRSTPRIV + 11)
#define IOCTL_SET_INTS                  (SIOCIWFIRSTPRIV + 12)
#define IOCTL_GET_INTS                  (SIOCIWFIRSTPRIV + 13)
#define IOCTL_SET_DRIVER                (SIOCIWFIRSTPRIV + 14)
#define IOCTL_GET_DRIVER                (SIOCIWFIRSTPRIV + 15)

#if CFG_SUPPORT_QA_TOOL
#define IOCTL_QA_TOOL_DAEMON			(SIOCIWFIRSTPRIV + 16)
#define IOCTL_IWPRIV_ATE                (SIOCIWFIRSTPRIV + 17)
#endif

#if CFG_SUPPORT_NAN
#define IOCTL_NAN_STRUCT (SIOCIWFIRSTPRIV + 20)
#endif

#define IOC_AP_GET_STA_LIST     (SIOCIWFIRSTPRIV+19)
#define IOC_AP_SET_MAC_FLTR     (SIOCIWFIRSTPRIV+21)
#define IOC_AP_SET_CFG          (SIOCIWFIRSTPRIV+23)
#define IOC_AP_STA_DISASSOC     (SIOCIWFIRSTPRIV+25)
#define IOC_AP_SET_NSS           (SIOCIWFIRSTPRIV+27)
#define IOC_AP_SET_BW           (SIOCIWFIRSTPRIV+29)

#define PRIV_CMD_REG_DOMAIN             0
#define PRIV_CMD_BEACON_PERIOD          1
#define PRIV_CMD_ADHOC_MODE             2

#if CFG_TCP_IP_CHKSUM_OFFLOAD
#define PRIV_CMD_CSUM_OFFLOAD       3
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

#define PRIV_CMD_ROAMING                4
#define PRIV_CMD_VOIP_DELAY             5
#define PRIV_CMD_POWER_MODE             6

#define PRIV_CMD_WMM_PS                 7
#define PRIV_CMD_BT_COEXIST             8
#define PRIV_GPIO2_MODE                 9

#define PRIV_CUSTOM_SET_PTA			10
#define PRIV_CUSTOM_CONTINUOUS_POLL     11
#define PRIV_CUSTOM_SINGLE_ANTENNA		12
#define PRIV_CUSTOM_BWCS_CMD			13
#define PRIV_CUSTOM_DISABLE_BEACON_DETECTION	14	/* later */
#define PRIV_CMD_OID                    15
#define PRIV_SEC_MSG_OID                16

#define PRIV_CMD_TEST_MODE              17
#define PRIV_CMD_TEST_CMD               18
#if BUILD_QA_DBG
#define PRIV_CMD_ACCESS_MCR             19
#endif
#define PRIV_CMD_SW_CTRL                20

#if 1				/* ANTI_PRIVCY */
#define PRIV_SEC_CHECK_OID              21
#endif

#define PRIV_CMD_WSC_PROBE_REQ          22

#define PRIV_CMD_P2P_VERSION                   23

#define PRIV_CMD_GET_CH_LIST            24

#define PRIV_CMD_SET_TX_POWER_NO_USED           25

#define PRIV_CMD_BAND_CONFIG            26

#define PRIV_CMD_DUMP_MEM               27

#define PRIV_CMD_P2P_MODE               28

#if CFG_SUPPORT_QA_TOOL
#define PRIV_QACMD_SET					29
#endif

#define PRIV_CMD_MET_PROFILING          33

#if CFG_WOW_SUPPORT
#define PRIV_CMD_SET_WOW_ENABLE			34
#define PRIV_CMD_SET_WOW_PAR			35
#endif

#ifdef UT_TEST_MODE
#define PRIV_CMD_UT		36
#endif /* UT_TEST_MODE */

#define PRIV_CMD_SET_SER                37

/* Get FW manifest version */
#define  PRIV_CMD_GET_FW_VERSION        38


/* dynamic tx power control */
#define PRIV_CMD_SET_PWR_CTRL		40

/* wifi type: 11g, 11n, ... */
#define  PRIV_CMD_GET_WIFI_TYPE		41

/* fos_change begin */
#define PRIV_CMD_CONNSTATUS			42
#if CFG_SUPPORT_STAT_STATISTICS
#define PRIV_CMD_STAT				43
#endif
#if CFG_SUPPORT_WAKEUP_STATISTICS
#define PRIV_CMD_INT_STAT			44
#endif
#if CFG_SUPPORT_EXCEPTION_STATISTICS
#define PRIV_CMD_EXCEPTION_STAT		45
#endif
#define PRIV_CMD_SHOW_CHANNEL		46

#define PRIV_CMD_SET_MDVT		47



/* 802.3 Objects (Ethernet) */
#define OID_802_3_CURRENT_ADDRESS           0x01010102

/* IEEE 802.11 OIDs */
#define OID_802_11_SUPPORTED_RATES              0x0D01020E
#define OID_802_11_CONFIGURATION                0x0D010211

/* PnP and PM OIDs, NDIS default OIDS */
#define OID_PNP_SET_POWER                               0xFD010101

#define OID_CUSTOM_OID_INTERFACE_VERSION                0xFFA0C000

/* MT5921 specific OIDs */
#define OID_CUSTOM_BT_COEXIST_CTRL                      0xFFA0C580
#define OID_CUSTOM_POWER_MANAGEMENT_PROFILE             0xFFA0C581
#define OID_CUSTOM_PATTERN_CONFIG                       0xFFA0C582
#define OID_CUSTOM_BG_SSID_SEARCH_CONFIG                0xFFA0C583
#define OID_CUSTOM_VOIP_SETUP                           0xFFA0C584
#define OID_CUSTOM_ADD_TS                               0xFFA0C585
#define OID_CUSTOM_DEL_TS                               0xFFA0C586
#define OID_CUSTOM_SLT                               0xFFA0C587
#define OID_CUSTOM_ROAMING_EN                           0xFFA0C588
#define OID_CUSTOM_WMM_PS_TEST                          0xFFA0C589
#define OID_CUSTOM_COUNTRY_STRING                       0xFFA0C58A
#define OID_CUSTOM_MULTI_DOMAIN_CAPABILITY              0xFFA0C58B
#define OID_CUSTOM_GPIO2_MODE                           0xFFA0C58C
#define OID_CUSTOM_CONTINUOUS_POLL                      0xFFA0C58D
#define OID_CUSTOM_DISABLE_BEACON_DETECTION             0xFFA0C58E

/* CR1460, WPS privacy bit check disable */
#define OID_CUSTOM_DISABLE_PRIVACY_CHECK                0xFFA0C600

/* Precedent OIDs */
#define OID_CUSTOM_MCR_RW                               0xFFA0C801
#define OID_CUSTOM_EEPROM_RW                            0xFFA0C803
#define OID_CUSTOM_SW_CTRL                              0xFFA0C805
#define OID_CUSTOM_MEM_DUMP                             0xFFA0C807

/* RF Test specific OIDs */
#define OID_CUSTOM_TEST_MODE                            0xFFA0C901
#define OID_CUSTOM_TEST_RX_STATUS                       0xFFA0C903
#define OID_CUSTOM_TEST_TX_STATUS                       0xFFA0C905
#define OID_CUSTOM_ABORT_TEST_MODE                      0xFFA0C906
#define OID_CUSTOM_MTK_WIFI_TEST                        0xFFA0C911
#define OID_CUSTOM_TEST_ICAP_MODE                       0xFFA0C913

/* BWCS */
#define OID_CUSTOM_BWCS_CMD                             0xFFA0C931
#define OID_CUSTOM_SINGLE_ANTENNA                       0xFFA0C932
#define OID_CUSTOM_SET_PTA                              0xFFA0C933

/* NVRAM */
#define OID_CUSTOM_MTK_NVRAM_RW                         0xFFA0C941
#define OID_CUSTOM_CFG_SRC_TYPE                         0xFFA0C942
#define OID_CUSTOM_EEPROM_TYPE                          0xFFA0C943

#if CFG_SUPPORT_WAPI
#define OID_802_11_WAPI_MODE                            0xFFA0CA00
#define OID_802_11_WAPI_ASSOC_INFO                      0xFFA0CA01
#define OID_802_11_SET_WAPI_KEY                         0xFFA0CA02
#endif

#if CFG_SUPPORT_WPS2
#define OID_802_11_WSC_ASSOC_INFO                       0xFFA0CB00
#endif

#if CFG_SUPPORT_LOWLATENCY_MODE
#define OID_CUSTOM_LOWLATENCY_MODE			0xFFA0CC00
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#define OID_IPC_WIFI_LOG_UI                             0xFFA0CC01
#define OID_IPC_WIFI_LOG_LEVEL                          0xFFA0CC02

#if CFG_SUPPORT_ANT_SWAP
#define OID_CUSTOM_QUERY_ANT_SWAP_CAPABILITY		0xFFA0CD00
#endif

#if CFG_SUPPORT_NCHO
#define CMD_NCHO_COMP_TIMEOUT			1500	/* ms */
#define CMD_NCHO_AF_DATA_LENGTH			1040
#endif

#ifdef UT_TEST_MODE
#define OID_UT                                          0xFFA0CD00
#endif /* UT_TEST_MODE */

/* Define magic key of test mode (Don't change it for future compatibity) */
#define PRIV_CMD_TEST_MAGIC_KEY                         2011
#define PRIV_CMD_TEST_MAGIC_KEY_ICAP                         2013

#define IW_PRIV_SET_BUF_SIZE			2000
#define IW_PRIV_GET_BUF_SIZE			2047
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* NIC BBCR configuration entry structure */
struct PRIV_CONFIG_ENTRY {
	uint8_t ucOffset;
	uint8_t ucValue;
};

#if CFG_SUPPORT_ADVANCE_CONTROL
enum {
		CMD_ADVCTL_NOISE_ID = 1,
		CMD_ADVCTL_POP_ID,
		CMD_ADVCTL_ED_ID,
		CMD_ADVCTL_PD_ID,
		CMD_ADVCTL_MAX_RFGAIN_ID,
		CMD_ADVCTL_ADM_CTRL_ID,
		CMD_ADVCTL_BCN_TH_ID = 9,
		CMD_ADVCTL_DEWEIGHTING_TH_ID,
		CMD_ADVCTL_DEWEIGHTING_NOISE_ID,
		CMD_ADVCTL_DEWEIGHTING_WEIGHT_ID,
		CMD_ADVCTL_ACT_INTV_ID,
		CMD_ADVCTL_1RPD,
		CMD_ADVCTL_MMPS,
		CMD_ADVCTL_RXC_ID = 17,
		CMD_ADVCTL_SNR_ID = 18,
		CMD_ADVCTL_BCNTIMOUT_NUM_ID = 19,
		CMD_ADVCTL_EVERY_TBTT_ID = 20,
		CMD_ADVCTL_MAX
};
#endif /* CFG_SUPPORT_ADVANCE_CONTROL */

#if CFG_AP_80211KVR_INTERFACE
extern struct sock *nl_sk;
#define EV_WLAN_MULTIAP_START \
	((0xA000 | 0x200) + 0x50)
#define EV_WLAN_MULTIAP_BSS_METRICS_RESPONSE \
	(EV_WLAN_MULTIAP_START + 0x09)
#define EV_WLAN_MULTIAP_STA_TOPOLOGY_NOTIFY \
	(EV_WLAN_MULTIAP_START + 0x08)
#define EV_WLAN_MULTIAP_ASSOC_STA_METRICS_RESPONSE \
	(EV_WLAN_MULTIAP_START + 0x0a)
#define EV_WLAN_MULTIAP_UNASSOC_STA_METRICS_RESPONSE \
	(EV_WLAN_MULTIAP_START + 0x0b)
#define EV_WLAN_MULTIAP_BEACON_METRICS_RESPONSE \
	(EV_WLAN_MULTIAP_START + 0x0c)
#define EV_WLAN_MULTIAP_STEERING_BTM_REPORT \
	(EV_WLAN_MULTIAP_START + 0x0d)
#define EV_WLAN_MULTIAP_TOPOLOGY_RESPONSE \
	(EV_WLAN_MULTIAP_START + 0x0e)
#define EV_WLAN_MULTIAP_BSS_STATUS_REPORT \
	(EV_WLAN_MULTIAP_START + 0x0f)
#endif /*CFG_AP_80211KVR_INTERFACE*/

typedef uint32_t(*PFN_OID_HANDLER_FUNC_REQ) (
	struct ADAPTER *prAdapter,
	void *pvBuf, uint32_t u4BufLen,
	uint32_t *pu4OutInfoLen);

enum ENUM_OID_METHOD {
	ENUM_OID_GLUE_ONLY,
	ENUM_OID_GLUE_EXTENSION,
	ENUM_OID_DRIVER_CORE
};

/* OID set/query processing entry */
struct WLAN_REQ_ENTRY {
	uint32_t rOid;		/* OID */
	const char *pucOidName;	/* OID name text */
	u_int8_t fgQryBufLenChecking;
	u_int8_t fgSetBufLenChecking;
	enum ENUM_OID_METHOD eOidMethod;
	uint32_t u4InfoBufLen;
	PFN_OID_HANDLER_FUNC_REQ pfOidQueryHandler; /* PFN_OID_HANDLER_FUNC */
	PFN_OID_HANDLER_FUNC_REQ pfOidSetHandler; /* PFN_OID_HANDLER_FUNC */
};

struct NDIS_TRANSPORT_STRUCT {
	uint32_t ndisOidCmd;
	uint32_t inNdisOidlength;
	uint32_t outNdisOidLength;
#if CFG_SUPPORT_QA_TOOL
	uint8_t ndisOidContent[20];
#else
	uint8_t ndisOidContent[16];
#endif	/* CFG_SUPPORT_QA_TOOL */
};

#if CFG_SUPPORT_NAN
enum _ENUM_NAN_CONTROL_ID {
	/* SD 0x00 */
	ENUM_NAN_PUBLISH = 0x00,
	ENUM_CANCEL_PUBLISH = 0x01,
	ENUM_NAN_SUBSCIRBE = 0x02,
	EMUM_NAN_CANCEL_SUBSCRIBE = 0x03,
	ENUM_NAN_TRANSMIT = 0x04,
	ENUM_NAN_UPDATE_PUBLISH = 0x05,
	ENUM_NAN_GAS_SCHEDULE_REQ = 0x06,

	/* DATA 0x10 */
	ENUM_NAN_DATA_REQ = 0x10,
	ENUM_NAN_DATA_RESP = 0x11,
	ENUM_NAN_DATA_END = 0x12,
	ENUM_NAN_DATA_UPDTAE = 0x13,

	/* RANGING 0x20 */
	ENUM_NAN_RG_REQ = 0x20,
	ENUM_NAN_RG_CANCEL = 0x21,
	ENUM_NAN_RG_RESP = 0x22,

	/* ENABLE/DISABLE NAN function */
	ENUM_NAN_ENABLE_REQ = 0x30,
	ENUM_NAN_DISABLE_REQ = 0x31,

	/* CONFIG 0x40 */
	ENUM_NAN_CONFIG_MP = 0x40,
	ENUM_NAN_CONFIG_HC = 0x41,
	ENUM_NAN_CONFIG_RANFAC = 0x42,
	ENUM_NAN_CONFIG_AWDW = 0x43
};

enum _ENUM_NAN_STATUS_REPORT {
	/* SD 0X00 */
	ENUM_NAN_SD_RESULT = 0x00,
	ENUM_NAN_REPLIED = 0x01,
	ENUM_NAN_SUB_TERMINATE = 0x02,
	ENUM_NAN_PUB_TERMINATE = 0x03,
	ENUM_NAN_RECEIVE = 0x04,
	ENUM_NAN_GAS_CONFIRM = 0x05,

	/* DATA 0X00 */
	ENUM_NAN_DATA_INDICATION = 0x10,
	ENUM_NAN_DATA_TERMINATE = 0x11,
	ENUM_NAN_DATA_CONFIRM = 0x12,

	/* RANGING 0X00 */
	ENUM_NAN_RG_INDICATION = 0x20,
	ENUM_NAN_RG_RESULT = 0x21
};
#endif

enum AGG_RANGE_TYPE_T {
	ENUM_AGG_RANGE_TYPE_TX = 0,
	ENUM_AGG_RANGE_TYPE_TRX = 1,
	ENUM_AGG_RANGE_TYPE_RX = 2
};

/*******************************************************************************
 *			P U B L I C   D A T A
 *******************************************************************************
 */
 /* To indocate if WFA test bed */
extern uint8_t g_IsWfaTestBed;
extern uint8_t g_IsTwtLogo;

#if (CFG_SUPPORT_802_11AX == 1)
extern uint8_t  g_fgHTSMPSEnabled;
#endif

/*******************************************************************************
 *			P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *			M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *			F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

int
priv_set_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra);

int
priv_get_int(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra);

int
priv_get_ints(struct net_device *prNetDev,
	      struct iw_request_info *prIwReqInfo,
	      union iwreq_data *prIwReqData, char *pcExtra);

int
priv_set_struct(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra);

int
priv_get_struct(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra);

/* fos_change begin */
int
priv_get_string(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra);
/* fos_change end */

int
priv_set_driver(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra);

int
priv_set_ap(struct net_device *prNetDev,
		struct iw_request_info *prIwReqInfo,
		union iwreq_data *prIwReqData, char *pcExtra);

int priv_support_ioctl(struct net_device *prDev,
		       struct ifreq *prReq, int i4Cmd);

int priv_support_driver_cmd(struct net_device *prDev,
			    struct ifreq *prReq, int i4Cmd);

#ifdef CFG_ANDROID_AOSP_PRIV_CMD
int android_private_support_driver_cmd(struct net_device *prDev,
struct ifreq *prReq, int i4Cmd);
#endif /* CFG_ANDROID_AOSP_PRIV_CMD */

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
int priv_support_mdns_offload(struct net_device *prDev,
				struct ifreq *prReq, int i4Cmd);
#endif
#endif
int32_t priv_driver_cmds(struct GLUE_INFO *prGlueInfo,
			 struct net_device *prNetDev,
			 int8_t *pcCommand, int32_t i4TotalLen);

int priv_driver_set_cfg(struct net_device *prNetDev,
			char *pcCommand, int i4TotalLen);

#if CFG_SUPPORT_QA_TOOL
int
priv_ate_set(struct net_device *prNetDev,
	     struct iw_request_info *prIwReqInfo,
	     union iwreq_data *prIwReqData, char *pcExtra);
#endif

#if CFG_SUPPORT_NAN_PRIV
int priv_nan_struct(struct net_device *prNetDev,
		    struct iw_request_info *prIwReqInfo,
		    union iwreq_data *prIwReqData, char *pcExtra);
#endif

#if CFG_AP_80211KVR_INTERFACE
int32_t MulAPAgentMontorSendMsg(uint16_t msgtype,
	void *pvmsgbuf, int32_t i4TotalLen);
#endif /* CFG_AP_80211KVR_INTERFACE */

/* Mediatek ioctl private commnad handler */
int priv_driver_set_ap_start(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_proc_set_ap_start(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_linkspeed(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_ap_get_sta_linkspeed(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_ap_set_ant_nss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_suspend_mode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_disablepartial(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_band(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_country(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_IDC_CH_SWITCH
int priv_driver_set_csa_ex(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_IDC_CH_SWITCH */
int priv_driver_get_country(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_channels(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_WFD
int priv_driver_set_miracast(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_WFD */
int priv_driver_set_sw_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_RA_GEN == 1)
int priv_driver_set_fixed_fallback(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int32_t priv_driver_set_ra_debug_proc(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_RA_GEN */
#if (CFG_SUPPORT_TXPOWER_INFO == 1)
int32_t priv_driver_get_txpower_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_TXPOWER_INFO */
int32_t priv_driver_txpower_man_set(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_set_unified_fixed_rate(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_unified_auto_rate(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_set_unified_mlo_agc_tx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_unified_mld_rec(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11BE_MLO */
#else
int priv_driver_set_fixed_rate(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_UNIFIED_COMMAND */
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_set_pp_cap_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_pp_alg_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_hm_alg_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_UNIFIED_COMMAND */
int priv_driver_boostcpu(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
int priv_driver_sniffer(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_ICS || CFG_SUPPORT_PHY_ICS */
#if (CFG_SUPPORT_PHY_ICS == 1)
int priv_driver_phyics_eventon(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_phyics_start(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_PHY_ICS */
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
int priv_driver_set_monitor(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */
int priv_driver_get_sw_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_WIFI_POWER_METRICS
int priv_driver_set_pwr_met(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
int priv_driver_get_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_drv_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_drv_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_MTK_WIFI_SW_EMI_RING
int priv_driver_get_emi_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_MTK_WIFI_SW_EMI_RING */
int priv_driver_set_uhw_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_uhw_mcr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_test_mode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_test_mode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_test_cmd(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_test_result(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_sta_stat(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_rx_stat(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_bf_cn(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_acl_policy(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_add_acl_entry(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_del_acl_entry(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_acl_entry(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_clear_acl_entry(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_NAN
int priv_driver_set_nan_start(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_master_ind(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_range(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_faw_reset(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_faw_config(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_faw_apply(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_nan_stat(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_nan_5g_160(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_NAN_FAST_DISC
int priv_driver_set_nan_fast_discovery(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
int priv_driver_nan_setch(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_NAN */
#if (CFG_SUPPORT_DFS_MASTER == 1)
int priv_driver_set_dfs_channel_available(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_dfs_state(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_dfs_help(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_dfs_cac_time(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_dfs_cac_start(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_dfs_cac_stop(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_rddreport(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_radarmode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_radarevent(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_rdd_op_mode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DFS_MASTER */
#if CFG_SUPPORT_IDC_CH_SWITCH
int priv_driver_set_idc_bmp(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_IDC_CH_SWITCH */
#if CFG_WOW_SUPPORT
int priv_driver_set_wow(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_wow_enable(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_wow_par(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_wow_udpport(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_wow_tcpport(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_wow_port(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_wow_reason(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_dongle_type(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_MDNS_OFFLOAD
int priv_driver_show_mdns_record(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_enable_mdns_offload(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_disable_mdns_offload(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_mdns_wake_flag(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_hitcounter(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_misscounter(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_enable_mdns_ipv6_wakeup(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);
#if TEST_CODE_FOR_MDNS
int priv_driver_send_mdns_record(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_add_mdns_record(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_test_add_mdns_record(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_passthtough_forward_all(
	 struct net_device *prNetDev,
	 char *pcCommand,  int i4TotalLen);
int priv_driver_set_passthtough_drop_all(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);

int priv_driver_set_passthtough_list(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);
int priv_driver_add_passthtough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);

int priv_driver_del_passthtough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);
int priv_driver_resetall_passthrough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);
int priv_driver_show_mdns_passthrough(struct net_device *prNetDev,
		 char *pcCommand,  int i4TotalLen);
#endif /* TEST_CODE_FOR_MDNS */
#endif /* CFG_SUPPORT_MDNS_OFFLOAD */
#endif /* CFG_WOW_SUPPORT */
int priv_driver_set_adv_pws(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_mdtim(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_QA_TOOL
int priv_driver_get_rx_statistics(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_QA_TOOL */
#if CFG_SUPPORT_MSP
int priv_driver_get_sta_statistics(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_bss_statistics(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_sta_index(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_sta_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_wtbl_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_mib_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_fw_log(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_MSP */
int priv_driver_set_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_em_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_em_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_chip_config(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_chip_config(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_version(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_cnm(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ais(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_capab_rsdb(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_DBDC
int priv_driver_set_dbdc(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DBDC */
int priv_driver_get_que_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_que_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_hif_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_tp_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ch_rank_list(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ch_dirtiness(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_efuse_ops(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if defined(_HIF_SDIO) && (MTK_WCN_HIF_SDIO == 0)
int priv_driver_cccr_ops(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
#if CFG_SUPPORT_ADVANCE_CONTROL
int priv_driver_set_noise(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_noise(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_pop(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
int priv_driver_set_ed(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ed(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DYNAMIC_EDCCA */
int priv_driver_set_pd(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_maxrfgain(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_ADVANCE_CONTROL */
#if (CFG_SURVEY_DUMP_FULL_CHANNEL == 1)
int priv_driver_get_survey_dump(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
int priv_driver_set_drv_ser(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_amsdu_num(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_amsdu_size(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_set_p2p_ps(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_p2p_noa(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_ENABLE_WIFI_DIRECT */
#ifdef UT_TEST_MODE
int priv_driver_run_ut(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* UT_TEST_MODE */
int priv_driver_get_wifi_type(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
int priv_driver_set_power_control(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT */
#if CFG_SUPPORT_TRX_LIMITED_CONFIG
int priv_driver_set_sap_force_trx_config(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_force_trx_support(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
#if CFG_WMT_RESET_API_SUPPORT
int priv_driver_trigger_whole_chip_reset(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_trigger_wfsys_reset(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_WMT_RESET_API_SUPPORT */
#if (CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1)
int priv_driver_get_uwtbl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_CONNAC2X || CFG_SUPPORT_CONNAC3X */
int priv_driver_set_tx_force_amsdu(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_run_hqa(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_calibration(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_DBDC
int priv_driver_set_sta1ss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DBDC */
#if CFG_WLAN_ASSISTANT_NVRAM
int priv_driver_set_nvram(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_nvram(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_WLAN_ASSISTANT_NVRAM */
int priv_driver_support_nvram(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_MTK_WIFI_SW_WFDMA
int priv_driver_set_sw_wfdma(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_MTK_WIFI_SW_WFDMA */
int priv_driver_get_hapd_channel(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_POWER_THROTTLING == 1)
int priv_driver_set_pwr_level(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_pwr_temp(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_POWER_THROTTLING */
int priv_driver_thermal_protect_enable(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_thermal_protect_disable(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_thermal_protect_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_thermal_protect_duty_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_thermal_protect_duty_cfg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_thermal_protect_state_act(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_mdvt(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_dump_mld(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_dump_mld_bss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_dump_mld_sta(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_dump_eml(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_MLC == 1)
int priv_driver_dump_mlc(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_MLC */
#endif /* CFG_SUPPORT_802_11BE_MLO */
int priv_driver_set_multista_use_case(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_bainfo(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_TSF_SYNC == 1)
int priv_driver_get_tsf_value(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_TSF_SYNC */
int priv_driver_get_mcu_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_DEBUG_SOP == 1)
int priv_driver_get_sleep_dbg_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_DEBUG_SOP */
#if (CFG_SUPPORT_802_11BE_MLO == 1)
int priv_driver_preset_linkid(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ml_probereq(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ml_bss_num(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ml_capa(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_ml_prefer_freqlist(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int priv_driver_get_ml_2nd_freq(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_802_11BE_T2LM_NEGO == 1)
int priv_driver_t2lm_request(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_t2lm_teardown(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11BE_T2LM_NEGO */
#endif /* CFG_SUPPORT_802_11BE_MLO */
#if (CFG_WIFI_GET_DPD_CACHE == 1)
int priv_driver_get_dpd_cache(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_WIFI_GET_DPD_CACHE */
int priv_driver_coex_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_WIFI_GET_MCS_INFO == 1)
int32_t priv_driver_get_mcs_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_WIFI_GET_MCS_INFO */
int priv_driver_get_ser_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_emi_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_emi_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_query_thermal_temp(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_AP_80211KVR_INTERFACE
int32_t priv_driver_MulAPAgent_bss_status_report(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_bss_report_info(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_sta_report_info(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_sta_measurement_control(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_sta_measurement_info(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_set_allow_sta(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_set_block_sta(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
#endif /* CFG_AP_80211KVR_INTERFACE */
#if CFG_AP_80211K_SUPPORT
int32_t priv_driver_MulAPAgent_beacon_report_request(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int32_t priv_driver_MulAPAgent_cu_report_request(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
#endif /* CFG_AP_80211K_SUPPORT */
#if CFG_AP_80211V_SUPPORT
int32_t priv_driver_MulAPAgent_BTM_request(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
#endif /* CFG_AP_80211V_SUPPORT */
int priv_driver_get_sleep_cnt_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_lp_keep_pwr_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_bf(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_nss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_p2p_ap_bw(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_amsdu_tx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_amsdu_rx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ampdu_tx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ampdu_rx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tx_ampdu_num(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tx_amsdu_num(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_qos(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_CSI
int priv_driver_set_csi(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_CSI */
#if CFG_SUPPORT_RTT
int priv_driver_set_rtt(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_RTT */
#if (CFG_SUPPORT_802_11AX == 1)
int priv_driver_muedca_override(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ba_size(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_trx_ba_size(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tp_test_mode(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_mcsmap(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tx_ppdu(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_ldpc(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tx_force_amsdu(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_om_ch_bw(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_om_rx_nss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_om_tx_nss(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_om_mu_dis(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_om_mu_data_dis(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_rx_ctrl_to_muti_bss(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_802_11BE == 1)
int priv_driver_set_eht_om(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_eht_om_rx_nss_ext(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int priv_driver_set_eht_om_ch_bw_ext(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int priv_driver_set_eht_om_tx_nsts_ext(
	struct net_device *prNetDev, char *pcCommand, int i4TotalLen);
int priv_driver_set_ehtmcsmap(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11BE */
int priv_driver_set_tx_om_packet(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_tx_cck_1m_pwr(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_pad_dur(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_sr_enable(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_sr_cap(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_sr_ind(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_pp_rx(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_set_smpsparams(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11AX */
#if (CFG_SUPPORT_TWT == 1)
int priv_driver_set_twtparams(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_TWT */
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
int priv_driver_epcs_send(struct net_device *prNetDev, char *pcCommand,
		int u4TotalLen);
#endif /* CFG_SUPPORT_802_11BE_EPCS */
#if CFG_SUPPORT_802_11K
int priv_driver_neighbor_request(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11K */
#if CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT
int priv_driver_bss_transition_query(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_802_11V_BSS_TRANSITION_MGT */
int priv_driver_get_mem_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_txd_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_PCIE_GEN_SWITCH
int priv_driver_set_pcie_speed(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_PCIE_GEN_SWITCH */
#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
int priv_driver_set_pcie_mode(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen);
int priv_driver_get_pcie_mode(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen);
#endif /* CFG_EAP_PCIE_GEN_SWITCH */

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
int priv_driver_phy_ctrl(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
int priv_driver_set_6g_pwr_mode(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen);
#endif
#if CFG_SUPPORT_WED_PROXY
int priv_driver_set_wed_enable(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen);
int priv_driver_set_drv_mcr_directly(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_get_drv_mcr_directly(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
int priv_driver_get_power_limit_emi_data(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */
#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
int priv_driver_set_tasar(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_TAS_HOST_CONTROL == 1 */
int priv_driver_set_atxop(struct net_device *prNetDev, char *pcCommand,
			int i4TotalLen);
int priv_driver_show_tr_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_ple_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_pse_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_csr_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_dmasch_info(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if CFG_SUPPORT_EASY_DEBUG
int priv_driver_fw_param(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
#if (CFG_PCIE_GEN_SWITCH == 1)
int priv_driver_set_genswitch(struct net_device *prNetDev,
			 char *pcCommand, int i4TotalLen);
#endif /* CFG_PCIE_GEN_SWITCH */
int priv_driver_tspec_operation(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_it_operation(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#if (CFG_SUPPORT_ROAMING == 1)
int priv_driver_roaming_policy(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
#endif
int priv_driver_fw_event(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_uapsd(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);
int priv_driver_show_ahdbg(struct net_device *prNetDev,
	char *pcCommand, int i4TotalLen);

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
int priv_driver_set_mddp_test(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen);
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP */

int priv_driver_dump_wfsys_cpupcr(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen);

#if CFG_ENABLE_WIFI_DIRECT
int priv_driver_update_wmm_params(struct net_device *prNetDev,
				  char *pcCommand, int i4TotalLen);

#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
int priv_driver_trigger_critical_update(struct net_device *prNetDev,
					char *pcCommand, int i4TotalLen);
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */

int32_t priv_driver_get_bw160_capa(struct net_device *prNetDev, char *pcCommand,
				   int32_t i4TotalLen);

#if (CFG_P2P2_SUPPORT_GC_REQ_CSA == 1)
int priv_driver_set_p2p2_gc_csa(struct net_device *prNetDev,
				char *pcCommand, int i4TotalLen);
#endif /* CFG_P2P2_SUPPORT_GC_REQ_CSA */

int priv_driver_get_rdd_status(struct net_device *prNetDev,
			  char *pcCommand, int i4TotalLen);
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if (CFG_SUPPORT_FACT_CAL == 1)
int priv_driver_fact_cal(struct net_device *prNetDev,
		char *pcCommand, int i4TotalLen);
#endif

#if CFG_SUPPORT_FIPS
int priv_driver_fips_test(struct net_device *prNetDev,
			  char *pcCommand, int i4TotalLen);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#endif /* _GL_WEXT_PRIV_H */
