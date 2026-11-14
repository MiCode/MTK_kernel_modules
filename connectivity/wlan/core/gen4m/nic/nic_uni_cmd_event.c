// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   nic_uni_cmd_event.c
 *    \brief  Callback functions for Command packets.
 *
 *	Various Event packet handlers which will be setup in the callback
 *  function of a command packet.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_ate_agent.h"
#include "gl_ics.h"

#if CFG_SUPPORT_CSI
#include "gl_csi.h"
#endif

#if (CFG_HW_DETECT_REPORT == 1)
#include "conn_dbg.h"
#endif

#if CFG_MTK_MDDP_SUPPORT
#include "mddp.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#if (CFG_SUPPORT_NAN == 1)
enum ENUM_UNI_CMD_NAN_TAG nanUniCommandTag[NAN_CMD_NUM] = {
	[NAN_CMD_MASTER_PREFERENCE] = UNI_CMD_NAN_TAG_SET_MASTER_PREFERENCE,
	[NAN_CMD_PUBLISH] = UNI_CMD_NAN_TAG_PUBLISH,
	[NAN_CMD_CANCEL_PUBLISH] = UNI_CMD_NAN_TAG_CANCEL_PUBLISH,
	[NAN_CMD_UPDATE_PUBLISH] = UNI_CMD_NAN_TAG_UPDATE_PUBLISH,
	[NAN_CMD_SUBSCRIBE] = UNI_CMD_NAN_TAG_SUBSCRIBE,
	[NAN_CMD_CANCEL_SUBSCRIBE] = UNI_CMD_NAN_TAG_CANCEL_SUBSCRIBE,
	[NAN_CMD_TRANSMIT] = UNI_CMD_NAN_TAG_TRANSMIT,
	[NAN_CMD_ENABLE_REQUEST] = UNI_CMD_NAN_TAG_ENABLE_REQUEST,
	[NAN_CMD_DISABLE_REQUEST] = UNI_CMD_NAN_TAG_DISABLE_REQUEST,
	[NAN_CMD_UPDATE_AVAILABILITY] = UNI_CMD_NAN_TAG_UPDATE_AVAILABILITY,
	[NAN_CMD_UPDATE_CRB] = UNI_CMD_NAN_TAG_UPDATE_CRB,
	[NAN_CMD_MANAGE_PEER_SCH_RECORD] =
		UNI_CMD_NAN_TAG_MANAGE_PEER_SCH_RECORD,
	[NAN_CMD_MAP_STA_RECORD] = UNI_CMD_NAN_TAG_MAP_STA_RECORD,
	[NAN_CMD_RANGING_REPORT_DISC] = UNI_CMD_NAN_TAG_RANGING_REPORT_DISC,
	[NAN_CMD_FTM_PARAM] = UNI_CMD_NAN_TAG_FTM_PARAM,
	[NAN_CMD_UPDATE_PEER_UAW] = UNI_CMD_NAN_TAG_UPDATE_PEER_UAW,
	[NAN_CMD_UPDATE_ATTR] = UNI_CMD_NAN_TAG_UPDATE_ATTR,
	[NAN_CMD_UPDATE_PHY_SETTING] = UNI_CMD_NAN_TAG_UPDATE_PHY_SETTING,
	[NAN_CMD_UPDATE_POTENTIAL_CHNL_LIST] =
		UNI_CMD_NAN_TAG_UPDATE_POTENTIAL_CHNL_LIST,
	[NAN_CMD_UPDATE_AVAILABILITY_CTRL] =
		UNI_CMD_NAN_TAG_UPDATE_AVAILABILITY_CTRL,
	[NAN_CMD_UPDATE_PEER_CAPABILITY] =
		UNI_CMD_NAN_TAG_UPDATE_PEER_CAPABILITY,
	[NAN_CMD_ADD_CSID] = UNI_CMD_NAN_TAG_ADD_CSID,
	[NAN_CMD_MANAGE_SCID] = UNI_CMD_NAN_TAG_MANAGE_SCID,
	[NAN_CMD_SET_SCHED_VERSION] = UNI_CMD_NAN_TAG_SET_SCHED_VERSION,
	[NAN_CMD_SET_DW_INTERVAL] = UNI_CMD_NAN_TAG_SET_DW_INTERVAL,
	[NAN_CMD_ENABLE_UNSYNC] = UNI_CMD_NAN_TAG_ENABLE_UNSYNC,
	[NAN_CMD_GET_DEVICE_INFO] = UNI_CMD_NAN_TAG_GET_DEVICE_INFO,
	[NAN_CMD_VENDOR_PAYLOAD] = UNI_CMD_NAN_TAG_VENDOR_PAYLOAD,
	[NAN_CMD_LOWPOWER_CTRL] = UNI_CMD_NAN_TAG_LOWPOWER_CTRL,
	[NAN_CMD_SET_HOST_ELECTION] = UNI_CMD_NAN_TAG_SET_HOST_ELECTION,
	[NAN_CMD_SET_ELECTION_ROLE] = UNI_CMD_NAN_TAG_SET_ELECTION_ROLE,
	[NAN_CMD_INSTANT_COMM_MODE] = UNI_CMD_NAN_TAG_INSTANT_COMM_MODE,

	[NAN_CMD_EXT_CLUSTER] = NAN_CMD_EXT_TAG_CLUSTER,
	[NAN_CMD_EXT_P2P] = NAN_CMD_EXT_TAG_P2P,
	[NAN_CMD_EXT_MERGING_DIRECTION] = NAN_CMD_EXT_TAG_MERGING_DIRECTION,
	[NAN_CMD_EXT_SYNC] = NAN_CMD_EXT_TAG_SYNC,
	[NAN_CMD_EXT_MERGING] = NAN_CMD_EXT_TAG_MERGING,
	[NAN_CMD_EXT_SCHEDULING] = NAN_CMD_EXT_TAG_SCHEDULING,
	[NAN_CMD_EXT_USD] = NAN_CMD_EXT_TAG_USD,
	[NAN_CMD_EXT_ASC] = NAN_CMD_EXT_TAG_ASC,
	[NAN_CMD_EXT_CUSTOM_CMD] = UNI_CMD_NAN_TAG_EXT_CUSTOM_CMD,
};
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
typedef uint32_t (*PROCESS_LEGACY_TO_UNI_FUNCTION) (struct ADAPTER *,
	struct WIFI_UNI_SETQUERY_INFO *);

typedef void(*PROCESS_RX_UNI_EVENT_FUNCTION) (struct ADAPTER *,
	struct WIFI_UNI_EVENT *);

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

static PROCESS_LEGACY_TO_UNI_FUNCTION arUniCmdTable[CMD_ID_END] = {
	[CMD_ID_SCAN_REQ_V2] = nicUniCmdScanReqV2,
	[CMD_ID_SCAN_CANCEL] = nicUniCmdScanCancel,
	[CMD_ID_SET_SCAN_SCHED_ENABLE] = nicUniCmdSchedScanEnable,
	[CMD_ID_SET_SCAN_SCHED_REQ] = nicUniCmdSchedScanReq,
	[CMD_ID_BSS_ACTIVATE_CTRL] = nicUniCmdBssActivateCtrl,
	[CMD_ID_GET_SET_CUSTOMER_CFG] = nicUniCmdCustomerCfg,
	[CMD_ID_CHIP_CONFIG] = nicUniCmdChipCfg,
	[CMD_ID_WTBL_INFO] = nicUniCmdNotSupport,
	[CMD_ID_SW_DBG_CTRL] = nicUniCmdSwDbgCtrl,
	[CMD_ID_REMOVE_STA_RECORD] = nicUniCmdRemoveStaRec,
	[CMD_ID_INDICATE_PM_BSS_CREATED] = nicUniCmdNotSupport,
	[CMD_ID_INDICATE_PM_BSS_ABORT] = nicUniCmdPmDisable,
	[CMD_ID_INDICATE_PM_BSS_CONNECTED] = nicUniCmdPmEnable,
	[CMD_ID_UPDATE_BEACON_CONTENT] = nicUniCmdBcnContent,
	[CMD_ID_SET_BSS_INFO] = nicUniCmdSetBssInfo,
	[CMD_ID_SET_WMM_PS_TEST_PARMS] = nicUniCmdSetWmmPsTestParams,
	[CMD_ID_SET_UAPSD_PARAM] = nicUniCmdSetUapsd,
	[CMD_ID_UPDATE_STA_RECORD] = nicUniCmdUpdateStaRec,
	[CMD_ID_CH_PRIVILEGE] = nicUniCmdChPrivilege,
	[CMD_ID_GET_CNM] = nicUniCmdCnmGetInfo,
	[CMD_ID_SET_RX_FILTER] = nicUniCmdSetRxFilter,
	[CMD_ID_SET_DBDC_PARMS] = nicUniCmdSetMbmc,
	[CMD_ID_NIC_POWER_CTRL] = nicUniCmdPowerCtrl,
	[CMD_ID_SET_DOMAIN_INFO] = nicUniCmdSetDomain,
	[CMD_ID_LOG_UI_INFO] = nicUniCmdWsysFwLogUI,
	[CMD_ID_FW_LOG_2_HOST] = nicUniCmdWsysFwLog2Host,
	[CMD_ID_BASIC_CONFIG] = nicUniCmdWsysFwBasicConfig,
	[CMD_ID_SET_SUSPEND_MODE] = nicUniCmdSetSuspendMode,
	[CMD_ID_SET_WOWLAN] = nicUniCmdSetWOWLAN,
	[CMD_ID_POWER_SAVE_MODE] = nicUniCmdPowerSaveMode,
	[CMD_ID_LAYER_0_EXT_MAGIC_NUM] = nicUniCmdExtCommon,
	[CMD_ID_UPDATE_WMM_PARMS] = nicUniCmdUpdateEdcaSet,
	[CMD_ID_ACCESS_REG] = nicUniCmdAccessReg,
	[CMD_ID_SET_BSS_RLM_PARAM] = nicUniCmdSetBssRlm,
	[CMD_ID_MQM_UPDATE_MU_EDCA_PARMS] = nicUniCmdUpdateMuEdca,
	[CMD_ID_RLM_UPDATE_SR_PARAMS] = nicUniCmdUpdateSrParams,
	[CMD_ID_SET_IP_ADDRESS] = nicUniCmdOffloadIPV4,
	[CMD_ID_SET_IPV6_ADDRESS] = nicUniCmdOffloadIPV6,
	[CMD_ID_GET_LTE_CHN] = nicUniCmdGetIdcChnl,
#if CFG_SUPPORT_IDC_RIL_BRIDGE
	[CMD_ID_SET_IDC_RIL] = nicUniCmdSetIdcRilBridge,
#endif
#if CFG_SUPPORT_UWB_COEX
	[CMD_ID_SET_UWB_COEX_ENABLE] = nicUniCmdSetUwbCoexEnable,
	[CMD_ID_SET_UWB_COEX_PREPARE] = nicUniCmdSetUwbCoexPrepare,
#endif
#if CFG_SUPPORT_ROAMING
	[CMD_ID_ROAMING_TRANSIT] = nicUniCmdRoaming,
#endif
	[CMD_ID_MIB_INFO] = nicUniCmdMibInfo,
	[CMD_ID_GET_STA_STATISTICS] = nicUniCmdGetStaStatistics,
	[CMD_ID_GET_STATISTICS] = nicUniCmdGetStatistics,
	[CMD_ID_SET_REPORT_BEACON] = nicUniCmdBeaconReport,
	[CMD_ID_GET_LINK_QUALITY] = nicUniCmdGetLinkQuality,
	[CMD_ID_GET_BUG_REPORT] = nicUniCmdGetBugReport,
	[CMD_ID_GET_STATS_LLS] = nicUniCmdGetLinkStats,
	[CMD_ID_PERF_IND] = nicUniCmdPerfInd,
	[CMD_ID_SG_PARAM] = nicUniCmdSetSGParam,
	[CMD_ID_SET_MONITOR] = nicUniCmdSetMonitor,
	[CMD_ID_ADD_REMOVE_KEY] = nicUniCmdInstallKey,
	[CMD_ID_DEFAULT_KEY_ID] = nicUniCmdInstallDefaultKey,
	[CMD_ID_SET_GTK_REKEY_DATA] = nicUniCmdOffloadKey,
	[CMD_ID_HIF_CTRL] = nicUniCmdHifCtrl,
	[CMD_ID_RDD_ON_OFF_CTRL] = nicUniCmdRddOnOffCtrl,
	[CMD_ID_SET_TDLS_CH_SW] = nicUniCmdTdls,
	[CMD_ID_SET_NOA_PARAM] = nicUniCmdSetP2pNoa,
	[CMD_ID_SET_OPPPS_PARAM] = nicUniCmdSetP2pOppps,
	[CMD_ID_SET_P2P_LO_START] = nicUniCmdSetP2pLoStart,
	[CMD_ID_SET_P2P_LO_STOP] = nicUniCmdSetP2pLoStop,
	[CMD_ID_SET_AP_CONSTRAINT_PWR_LIMIT] = nicUniCmdSetApConstraintPwrLimit,
	[CMD_ID_SET_RRM_CAPABILITY] = nicUniCmdSetRrmCapability,
	[CMD_ID_SET_COUNTRY_POWER_LIMIT] = nicUniCmdSetCountryPwrLimit,
	[CMD_ID_SET_MDVT] = nicUniCmdSetMdvt,
	[CMD_ID_SET_COUNTRY_POWER_LIMIT_PER_RATE] =
			nicUniCmdSetCountryPwrLimitPerRate,
	[CMD_ID_SET_NVRAM_SETTINGS] = nicUniCmdSetNvramSettings,
	[CMD_ID_TEST_CTRL] = nicUniCmdTestmodeCtrl,
#if CFG_SUPPORT_QA_TOOL
	[CMD_ID_ACCESS_RX_STAT] = nicUniCmdTestmodeRxStat,
#if (CONFIG_WLAN_SERVICE == 1)
	[CMD_ID_LIST_MODE] = nicUniCmdTestmodeListmode,
#endif
#endif
	[CMD_ID_SET_FORCE_RTS] = nicUniCmdGamingMode,
#if (CFG_SAP_SUS_SUPPORT == 1)
	[CMD_ID_SET_SAP_SUS]   = nicUniCmdSetSapSus,
#endif
#if (CFG_SAP_RPS_SUPPORT == 1)
	[CMD_ID_SET_SAP_RPS]   = nicUniCmdSetSapRps,
#endif
	[CMD_ID_TX_AMPDU] = nicUniCmdSetTxAmpdu,
	[CMD_ID_ADDBA_REJECT] = nicUniCmdSetRxAmpdu,
	[CMD_ID_MAC_MCAST_ADDR] = nicUniCmdSetMultiAddr,
	[CMD_ID_RSSI_MONITOR] = nicUniCmdSetRssiMonitor,
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	[CMD_ID_SET_ICS_SNIFFER] = nicUniCmdSetIcsSniffer,
#endif
#if (CFG_SUPPORT_PHY_ICS == 1)
	[CMD_ID_SET_PHY_ICS_EVENT] = nicUniCmdSetPhyIcsEventOn,
	[CMD_ID_SET_PHY_ICS_START] = nicUniCmdSetPhyIcsStart,
#endif
#if (CFG_SUPPORT_RTT == 1)
	[CMD_ID_RTT_GET_CAPABILITIES] = nicUniCmdRttGetCapabilities,
	[CMD_ID_RTT_RANGE_REQUEST] = nicUniCmdRttRangeRequest,
	[CMD_ID_RTT_INSTALL_LTF_KEYSEED] = nicUniCmdRttInstallLtfKeyseed,
#endif
#if (CFG_SUPPORT_NAN == 1)
	[CMD_ID_NAN_EXT_CMD] = nicUniCmdNan,
#endif
	[CMD_ID_SET_ACL_POLICY] = nicUniCmdACLPolicy,
#if (CFG_SUPPORT_CSI == 1)
	[CMD_ID_CSI_CONTROL] = nicUniCmdSetCsiControl,
#endif
#if (CFG_VOLT_INFO == 1)
	[CMD_ID_SEND_VOLT_INFO] = nicUniCmdSendVnf,
#endif
#if CFG_FAST_PATH_SUPPORT
	[CMD_ID_FAST_PATH] = nicUniCmdFastPath,
#endif
#if CFG_SUPPORT_PKT_OFLD
	[CMD_ID_PKT_OFLD] = nicUniCmdPktOfldOp,
#endif
	[CMD_ID_WFC_KEEP_ALIVE] = nicUniCmdKeepAlive,
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
	[CMD_ID_SET_MDNS_RECORD] = nicUniCmdMdnsRecorde,
#endif
#endif
	[CMD_ID_LP_DBG_CTRL] = nicUniCmdLpDbgCtrl,
#if CFG_SUPPORT_WIFI_ICCM
	[CMD_ID_SET_ICCM] = nicUniCmdIccmSetParam,
#endif
#if CFG_SUPPORT_WIFI_POWER_METRICS
	[CMD_ID_POWER_METRICS] = nicUniCmdPowerMetricsStatSetParam,
#endif

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	[CMD_ID_SET_PWR_LIMIT_EMI_INFO] = nicUniCmdPowerLimitEmiInfo,
#endif

#if (CFG_PCIE_GEN_SWITCH == 1)
	[CMD_ID_UPDATE_LP] = nicUniCmdUpdateLowPowerParam,
#endif
#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
	[CMD_ID_UPDATE_PCIE] = nicUniCmdUpdatePcieParam,
#endif /*CFG_EAP_PCIE_GEN_SWITCH*/

#if (CFG_SUPPORT_TSF_SYNC == 1)
	[CMD_ID_BEACON_TSF_SYNC] = nicUniCmdUpdateTsfSyncParam,
#endif
#if CFG_SUPPORT_FIPS
	[CMD_ID_FIPS_TC] = nicUniCmdFipsTc,
	[CMD_ID_FIPS_GET_STATUS] = nicUniCmdFipsGetStatus,
	[CMD_ID_FIPS_GET_RESULT] = nicUniCmdFipsGetResult,
#endif

};

static PROCESS_LEGACY_TO_UNI_FUNCTION arUniExtCmdTable[EXT_CMD_ID_END] = {
	[EXT_CMD_ID_TWT_AGRT_UPDATE] = nicUniCmdTwtArgtUpdate,
	[EXT_CMD_ID_STAREC_UPDATE] = nicUniCmdStaRecUpdateExt,
#if CFG_SUPPORT_TX_BF
	[EXT_CMD_ID_BF_ACTION] = nicUniCmdBFAction,
#endif
	[EXT_CMD_ID_SER] = nicUniCmdSerAction,
#if (CFG_SUPPORT_TWT == 1)
	[EXT_CMD_ID_GET_MAC_INFO] = nicUniCmdGetTsf,
#endif
	[EXT_CMD_ID_DEVINFO_UPDATE] = nicUniUpdateDevInfo,
	[EXT_CMD_ID_TX_POWER_FEATURE_CTRL] = nicUniCmdTxPowerCtrl,
	[EXT_CMD_ID_THERMAL_PROTECT] = nicUniCmdThermalProtect,
#if CFG_SUPPORT_QA_TOOL
	[EXT_CMD_ID_RF_TEST] = nicUniExtCmdTestmodeCtrl,
#endif
	[EXT_CMD_ID_EFUSE_ACCESS] = nicUniCmdEfuseAccess,
	[EXT_CMD_ID_EFUSE_BUFFER_MODE] = nicUniCmdEfuseBufferMode,
	[EXT_CMD_ID_EFUSE_FREE_BLOCK] = nicUniCmdEfuseFreeBlock,
	[EXT_CMD_ID_SR_CTRL] = nicUniCmdSR,
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	[EXT_CMD_ID_TWT_STA_GET_CNM_GRANTED] = nicUniCmdTwtStaGetCnmGranted,
#endif
#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
	[EXT_CMD_ID_BT_CTRL] = nicUniCmdBtCtrl,
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */
};

static PROCESS_RX_UNI_EVENT_FUNCTION arUniEventTable[UNI_EVENT_ID_NUM] = {
	[UNI_EVENT_ID_SCAN_DONE] = nicUniEventScanDone,
	[UNI_EVENT_ID_CNM] = nicUniEventChMngrHandleChEvent,
	[UNI_EVENT_ID_MBMC] = nicUniEventMbmcHandleEvent,
	[UNI_EVENT_ID_STATUS_TO_HOST] = nicUniEventStatusToHost,
	[UNI_EVENT_ID_BA_OFFLOAD] = nicUniEventBaOffload,
	[UNI_EVENT_ID_SLEEP_NOTIFY] = nicUniEventSleepNotify,
	[UNI_EVENT_ID_BEACON_TIMEOUT] = nicUniEventBeaconTimeout,
	[UNI_EVENT_ID_UPDATE_COEX_PHYRATE] = nicUniEventUpdateCoex,
	[UNI_EVENT_ID_IDC] = nicUniEventIdc,
	[UNI_EVENT_ID_BSS_IS_ABSENCE] = nicUniEventBssIsAbsence,
#if CFG_ENABLE_WIFI_DIRECT
	[UNI_EVENT_ID_PS_SYNC] = nicUniEventPsSync,
	[UNI_EVENT_ID_SAP] = nicUniEventSap,
#endif
	[UNI_EVENT_ID_OBSS_UPDATE] = nicUniEventOBSS,
#if CFG_SUPPORT_ROAMING
	[UNI_EVENT_ID_ROAMING] = nicUniEventRoaming,
#endif
	[UNI_EVENT_ID_ADD_KEY_DONE] = nicUniEventAddKeyDone,
	[UNI_EVENT_ID_FW_LOG_2_HOST] = nicUniEventFwLog2Host,
#if CFG_ENABLE_WIFI_DIRECT
	[UNI_EVENT_ID_P2P] = nicUniEventP2p,
	[UNI_EVENT_ID_IE_COUNTDOWN] = nicUniEventCountdown,
#endif
	[UNI_EVENT_ID_STAREC] = nicUniEventStaRec,
#if (CFG_SUPPORT_DFS_MASTER == 1)
	[UNI_EVENT_ID_RDD] = nicUniEventRDD,
#endif
#if CFG_SUPPORT_TDLS
	[UNI_EVENT_ID_TDLS] = nicUniEventTdls,
#endif
	[UNI_EVENT_ID_BSS_ER] = nicUniEventBssER,
	[UNI_EVENT_ID_RSSI_MONITOR] = nicUniEventRssiMonitor,
	[UNI_EVENT_ID_HIF_CTRL] = nicUniEventHifCtrl,
#if (CFG_SUPPORT_RTT == 1)
	[UNI_EVENT_ID_RTT] = nicUniEventRtt,
#endif
	[UNI_EVENT_ID_NAN] = nicUniEventNan,
#if CFG_SUPPORT_TX_BF
	[UNI_EVENT_ID_BF] = nicUniEventBF,
#endif
	[UNI_EVENT_ID_PP] = nicUniEventPpCb,
#if CFG_WOW_SUPPORT
	[UNI_EVENT_ID_WOW] = nicUniEventWow,
#endif
	[UNI_EVENT_ID_CSI] = nicUniEventCsiData,
	[UNI_EVENT_ID_STATISTICS] = nicUniUnsolicitStatsEvt,
	[UNI_EVENT_ID_SR] = nicUniEventSR,
	[UNI_EVENT_ID_SPECTRUM] = nicUniEventPhyIcsRawData,
#if (CFG_VOLT_INFO == 1)
	[UNI_EVENT_ID_GET_VOLT_INFO] = nicUniEventGetVnf,
#endif
#if CFG_SUPPORT_BAR_DELAY_INDICATION
	[UNI_EVENT_ID_DELAY_BAR] = nicUniEventDelayBar,
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */
#if CFG_SUPPORT_FW_DROP_SSN
	[UNI_EVENT_ID_FW_DROP_SSN] = nicUniEventFwDropSSN,
#endif /* CFG_SUPPORT_FW_DROP_SSN */
#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
	[UNI_EVENT_ID_MDNS_REOCRD] = nicUniEventMdnsStats,
#endif
#endif
	[UNI_EVENT_ID_FAST_PATH] = nicUniEventFastPath,
	[UNI_EVENT_ID_THERMAL] = nicUniEventThermalProtect,
#if (CFG_CE_ASSERT_DUMP == 1)
	[UNI_EVENT_ID_ASSERT_DUMP] = nicUniEventAssertDump,
#endif
#if (CFG_HW_DETECT_REPORT == 1)
	[UNI_EVENT_ID_HW_DETECT_REPORT] = nicUniEventHwDetectReport,
#endif
	[UNI_EVENT_ID_UPDATE_LP] = nicUniEventUpdateLp,
#if CFG_SUPPORT_WIFI_POWER_METRICS
	[UNI_EVENT_ID_POWER_METRICS] = nicUniEventPowerMetricsStatGetInfo,
#endif

#if CFG_MTK_MDDP_SUPPORT
	[UNI_EVENT_ID_MDDP] = nicUniEventMddp,
#endif /* CFG_MTK_MDDP_SUPPORT */

	[UNI_EVENT_ID_TXPOWER] = nicUniEventTxPower,
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	[UNI_EVENT_ID_MLO] = nicUniEventMLSRSwitchDone,
#endif
#if (CFG_SUPPORT_FACT_CAL)
	[UNI_EVENT_ID_FACT_CAL] = nicUniEventGetFactCalData,
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	[UNI_EVENT_ID_OMI] = nicUniEventOmi,
#endif
#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
	[UNI_EVENT_ID_BT_CTRL] = nicUniCmdEventQueryBtCtrl,
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */
#if CFG_SUPPORT_MBRAIN
	[UNI_EVENT_ID_MBRAIN] = nicUniUnsolicitMbrEvt,
#endif
#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
	[UNI_EVENT_ID_SHR_ANT_SWCH] = nicUniEventShrAntSwch,
#endif
	[UNI_EVENT_ID_COEX_ICER] = nicUniEventIcerRawData,
};

extern struct RX_EVENT_HANDLER arEventTable[];
extern uint32_t arEventTableSize;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define UNI_CMD_STAREC_INVALID_WTBL_IDX 0x7ff

#define	RUN_RX_EVENT_HANDLER(_EID, _pdata) \
	nicUniEventHelper(ad, evt, _EID, 0, \
		(uint8_t *)(_pdata), sizeof(*(_pdata)))

#define	RUN_RX_EVENT_HANDLER_EXT(_EID, _pdata, _len) \
	nicUniEventHelper(ad, evt, _EID, 0, (uint8_t *)(_pdata), _len)

#define	RUN_RX_EXT_EVENT_HANDLER(_EXT_EID, _pdata) \
	nicUniEventHelper(ad, evt, EVENT_ID_LAYER_0_EXT_MAGIC_NUM, _EXT_EID, \
		(uint8_t *)(_pdata), sizeof(*(_pdata)))

#define DUMP_PKT_INFO_16 \
	"%02x %02x %02x %02x %02x %02x %02x %02x" \
	"%02x %02x %02x %02x %02x %02x %02x %02x" \
	"%02x %02x %02x %02x %02x %02x %02x %02x" \
	"%02x %02x %02x %02x %02x %02x %02x %02x\n"

#define DUMP_PKT_INFO_6 \
	"%02x %02x %02x %02x %02x %02x\n"

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
struct WIFI_UNI_CMD_ENTRY *nicUniCmdAllocEntry(struct ADAPTER *ad,
	uint8_t ucid, uint32_t max_cmd_len,
	PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler)
{
	struct WIFI_UNI_CMD_ENTRY *entry;

	/* alloc entry + cmd_buf together */
	entry = (struct WIFI_UNI_CMD_ENTRY *) cnmMemAlloc(ad, RAM_TYPE_BUF,
			sizeof(*entry) + max_cmd_len);

	if (!entry)
		return NULL;

	kalMemZero(entry, sizeof(*entry) + max_cmd_len);
	entry->pucInfoBuffer = ((uint8_t *) entry) + sizeof(*entry);
	entry->u4SetQueryInfoLen = max_cmd_len;
	entry->pfCmdDoneHandler = pfCmdDoneHandler;
	entry->pfCmdTimeoutHandler = pfCmdTimeoutHandler;
	entry->ucUCID = ucid;

	return entry;
}

void nicUniCmdFreeEntry(struct ADAPTER *ad, struct WIFI_UNI_CMD_ENTRY *entry)
{
	if (entry)
		cnmMemFree(ad, entry);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief command packet generation utility
 *
 * \param[in] prAdapter          Pointer to the Adapter structure.
 * \param[in] ucCID              Command ID
 * \param[in] fgSetQuery         Set or Query
 * \param[in] fgNeedResp         Need for response
 * \param[in] pfCmdDoneHandler   Function pointer when command is done
 * \param[in] u4SetQueryInfoLen  The length of the set/query buffer
 * \param[in] pucInfoBuffer      Pointer to set/query buffer
 *
 *
 * \retval WLAN_STATUS_PENDING
 * \retval WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanSendSetQueryCmdHelper(struct ADAPTER *prAdapter,
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
		    enum EUNM_CMD_SEND_METHOD eMethod)
{
	struct WIFI_UNI_SETQUERY_INFO info;
	struct LINK *link = &info.rUniCmdList;
	struct WIFI_UNI_CMD_ENTRY *entry, *next;
	uint32_t status = 0;

	if (kalIsResetting()) {
		DBGLOG(INIT, WARN, "Chip resetting, skip\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (!arUniCmdTable[ucCID])
		return wlanSendSetQueryCmdAdv(prAdapter,
			ucCID, ucExtCID, fgSetQuery, fgNeedResp, fgIsOid,
			pfCmdDoneHandler, pfCmdTimeoutHandler,
			u4SetQueryInfoLen, pucInfoBuffer,
			pvSetQueryBuffer, u4SetQueryBufferLen,
			eMethod);

	/* prepare info */
	info.ucCID = ucCID;
	info.ucExtCID = ucExtCID;
	info.fgSetQuery = fgSetQuery;
	info.fgNeedResp = fgNeedResp;
	info.fgIsOid = fgIsOid;
	info.pfCmdDoneHandler = pfCmdDoneHandler;
	info.pfCmdTimeoutHandler = pfCmdTimeoutHandler;
	info.u4SetQueryInfoLen = u4SetQueryInfoLen;
	info.pucInfoBuffer = pucInfoBuffer;
	info.pvSetQueryBuffer = pvSetQueryBuffer;
	info.u4SetQueryBufferLen = u4SetQueryBufferLen;
	info.eMethod = eMethod;
	LINK_INITIALIZE(&info.rUniCmdList);

	/* collect unified cmd info */
	status = arUniCmdTable[ucCID](prAdapter, &info);
	if (status != WLAN_STATUS_SUCCESS)
		goto done;

	LINK_FOR_EACH_ENTRY_SAFE(entry, next,
		link, rLinkEntry, struct WIFI_UNI_CMD_ENTRY) {

		DBGLOG(REQ, TRACE,
			"UCMD[0x%x] SET[%d] RSP[%d] OID[%d] LEN[%d]\n",
			entry->ucUCID, info.fgSetQuery, info.fgNeedResp,
			info.fgIsOid, entry->u4SetQueryInfoLen);
		status = wlanSendSetQueryUniCmdAdv(prAdapter,
			entry->ucUCID, info.fgSetQuery, info.fgNeedResp,
			info.fgIsOid, entry->pfCmdDoneHandler,
			entry->pfCmdTimeoutHandler, entry->u4SetQueryInfoLen,
			entry->pucInfoBuffer, info.pvSetQueryBuffer,
			info.u4SetQueryBufferLen, info.eMethod);
	}
done:
	/* clear before return, in case uni handler already insert any entry */
	LINK_FOR_EACH_ENTRY_SAFE(entry, next,
		link, rLinkEntry, struct WIFI_UNI_CMD_ENTRY) {

		LINK_REMOVE_KNOWN_ENTRY(link, entry);
		/* releaes info */
		nicUniCmdFreeEntry(prAdapter, entry);
	}

	return status;
}

uint32_t wlanSendSetQueryUniCmd(struct ADAPTER *prAdapter,
			uint8_t ucUCID,
			u_int8_t fgSetQuery,
			u_int8_t fgNeedResp,
			u_int8_t fgIsOid,
			PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
			PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
			uint32_t u4SetQueryInfoLen,
			uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
			uint32_t u4SetQueryBufferLen)
{
	return wlanSendSetQueryUniCmdAdv(prAdapter,
			ucUCID, fgSetQuery, fgNeedResp, fgIsOid,
			pfCmdDoneHandler, pfCmdTimeoutHandler,
			u4SetQueryInfoLen, pucInfoBuffer,
			pvSetQueryBuffer, u4SetQueryBufferLen,
			CMD_SEND_METHOD_ENQUEUE);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief command packet generation utility
*
* \param[in] prAdapter	   Pointer to the Adapter structure.
* \param[in] ucCID		   Command ID
* \param[in] fgSetQuery	   Set or Query
* \param[in] fgNeedResp	   Need for response
* \param[in] pfCmdDoneHandler   Function pointer when command is done
* \param[in] u4SetQueryInfoLen  The length of the set/query buffer
* \param[in] pucInfoBuffer	   Pointer to set/query buffer
*
*
* \retval WLAN_STATUS_PENDING
* \retval WLAN_STATUS_FAILURE
*/
/*----------------------------------------------------------------------------*/
uint32_t
wlanSendSetQueryUniCmdAdv(struct ADAPTER *prAdapter,
	      uint8_t ucUCID,
	      u_int8_t fgSetQuery,
	      u_int8_t fgNeedResp,
	      u_int8_t fgIsOid,
	      PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
	      PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
	      uint32_t u4SetQueryInfoLen,
	      uint8_t *pucInfoBuffer, void *pvSetQueryBuffer,
	      uint32_t u4SetQueryBufferLen,
	      enum EUNM_CMD_SEND_METHOD eMethod)
{
	struct GLUE_INFO *prGlueInfo;
	struct CMD_INFO *prCmdInfo;
	uint8_t *pucCmfBuf;
	struct mt66xx_chip_info *prChipInfo;
	uint16_t cmd_size;
	uint8_t ucOption;
	uint32_t status = WLAN_STATUS_PENDING;

	if (kalIsResetting()) {
		DBGLOG(INIT, WARN, "Chip resetting, skip\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	prChipInfo = prAdapter->chip_info;
	cmd_size = prChipInfo->u2UniCmdTxHdrSize + u4SetQueryInfoLen;
	prCmdInfo = cmdBufAllocateCmdInfo(prAdapter, cmd_size);

	if (!prCmdInfo) {
		DBGLOG(INIT, ERROR, "Allocate CMD_INFO_T ==> FAILED.\n");
		return WLAN_STATUS_RESOURCES;
	}

	/* Setup common CMD Info Packet */
	prCmdInfo->eCmdType = COMMAND_TYPE_NETWORK_IOCTL;
	prCmdInfo->u2InfoBufLen = cmd_size;
	prCmdInfo->pfCmdDoneHandler = pfCmdDoneHandler;
	prCmdInfo->pfCmdTimeoutHandler = pfCmdTimeoutHandler;
	prCmdInfo->fgIsOid = fgIsOid;
	prCmdInfo->ucCID = ucUCID;
	prCmdInfo->fgSetQuery = fgSetQuery;
	prCmdInfo->fgNeedResp = fgNeedResp;
	prCmdInfo->u4SetInfoLen = u4SetQueryInfoLen;
	prCmdInfo->pvInformationBuffer = pvSetQueryBuffer;
	prCmdInfo->u4InformationBufferLength = u4SetQueryBufferLen;
	ucOption = UNI_CMD_OPT_BIT_1_UNI_CMD;
	if (fgSetQuery) /* it is a SET command */
		ucOption |= (prCmdInfo->fgNeedResp ? UNI_CMD_OPT_BIT_0_ACK : 0);
	ucOption |= (fgSetQuery ? UNI_CMD_OPT_BIT_2_SET_QUERY : 0);

	// TODO: uni cmd, fragment
	/* Setup WIFI_CMD_T (no payload) */
	NIC_FILL_UNI_CMD_TX_HDR(prAdapter,
		prCmdInfo->pucInfoBuffer,
		prCmdInfo->u2InfoBufLen,
		prCmdInfo->ucCID,
		CMD_PACKET_TYPE_ID,
		&prCmdInfo->ucCmdSeqNum,
		&pucCmfBuf, S2D_INDEX_CMD_H2N,
		ucOption);
	prCmdInfo->pucSetInfoBuffer = pucCmfBuf;
	if (u4SetQueryInfoLen > 0 && pucInfoBuffer != NULL)
		kalMemCopy(pucCmfBuf, pucInfoBuffer, u4SetQueryInfoLen);

	switch (eMethod) {
	case CMD_SEND_METHOD_ENQUEUE:
		if (prCmdInfo->fgIsOid) {
			prGlueInfo->OidEntry.prCmdInfo = prCmdInfo;
			DBGLOG(NIC, LOUD, "prIoReq->prCmdInfo=%p", prCmdInfo);
		}
		/* insert into prCmdQueue */
		kalEnqueueCommand(prGlueInfo,
				  (struct QUE_ENTRY *) prCmdInfo);

		/* wakeup txServiceThread later */
		GLUE_SET_EVENT(prGlueInfo);
		break;
	case CMD_SEND_METHOD_REQ_RESOURCE:
		status = wlanSendCommand(prAdapter, prCmdInfo);
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		break;
	case CMD_SEND_METHOD_TX:
		status = nicTxCmd(prAdapter, prCmdInfo, TC4_INDEX);
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		break;
	}

	return status;
}

uint32_t nicUniCmdExtCommon(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM)
		return WLAN_STATUS_NOT_ACCEPTED;

	if (!arUniExtCmdTable[info->ucExtCID])
		return WLAN_STATUS_NOT_SUPPORTED;

	return arUniExtCmdTable[info->ucExtCID](ad, info);
}

uint32_t nicUniCmdNotSupport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	DBGLOG(INIT, ERROR, "CID[0x%x] not support!!\n", info->ucCID);
	return WLAN_STATUS_SUCCESS;
}

uint16_t nicUniCmdScanTagReq(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_REQ *tag = (struct UNI_CMD_SCAN_REQ *)buf;

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_REQ;
	tag->u2Length = sizeof(*tag);
	tag->ucScanType = cmd->ucScanType;
	tag->ucNumProbeReq = cmd->ucNumProbeReq;
	tag->ucScnFuncMask = cmd->ucScnFuncMask;
	tag->ucScnSourceMask = cmd->ucScnSourceMask;
	tag->u2ChannelMinDwellTime = cmd->u2ChannelMinDwellTime;
	tag->u2ChannelDwellTime = cmd->u2ChannelDwellTime;
	tag->u2TimeoutValue = cmd->u2TimeoutValue;
	tag->u2ProbeDelayTime = cmd->u2ProbeDelayTime;
	tag->u4ScnFuncMaskExtend = cmd->u4ScnFuncMaskExtend;
	return tag->u2Length;
}

uint16_t nicUniCmdScanTagSsid(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_SSID *tag = (struct UNI_CMD_SCAN_SSID *)buf;
	uint8_t i;
	uint8_t ssid_num, ssid_ext_num;
	uint8_t *pos = tag->aucSsidBuffer;
	uint16_t len;

	if (cmd->ucSSIDNum > 4)
		ssid_num = 4;
	else
		ssid_num = cmd->ucSSIDNum;

	if (cmd->ucSSIDExtNum > 6)
		ssid_ext_num = 6;
	else
		ssid_ext_num = cmd->ucSSIDExtNum;

	if ((ssid_num + ssid_ext_num) == 0)
		return 0;

	len = sizeof(*tag) +
		(ssid_num + ssid_ext_num) * sizeof(struct PARAM_SSID);

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_SSID;
	tag->u2Length = len;
	tag->ucSSIDType = cmd->ucSSIDType;
	tag->ucSSIDNum = ssid_num + ssid_ext_num;
	tag->ucIsShortSSID = 0;
	for (i = 0; i < ssid_num; i++) {
		kalMemCopy(pos, &cmd->arSSID[i], sizeof(struct PARAM_SSID));
		pos += sizeof(struct PARAM_SSID);
	}
	for (i = 0; i < ssid_ext_num; i++) {
		kalMemCopy(pos, &cmd->arSSIDExtend[i],
					sizeof(struct PARAM_SSID));
		pos += sizeof(struct PARAM_SSID);
	}
	return tag->u2Length;
}

uint16_t nicUniCmdScanTagShortSsid(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_SSID *tag = (struct UNI_CMD_SCAN_SSID *)buf;
	struct PARAM_SSID arShortSSID;
	uint8_t i;
	uint8_t *pos = tag->aucSsidBuffer;
	uint32_t short_ssid_num = kal_min_t(uint32_t, cmd->ucShortSSIDNum,
					CFG_SCAN_OOB_MAX_NUM);
	uint16_t len = sizeof(*tag) +
		short_ssid_num * sizeof(struct PARAM_SSID);

	if (short_ssid_num == 0)
		return 0;

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_SSID;
	tag->u2Length = len;
	tag->ucSSIDType = cmd->ucSSIDType;
	tag->ucSSIDNum = short_ssid_num;
	tag->ucIsShortSSID = 1;

	arShortSSID.u4SsidLen = MAX_SHORT_SSID_LEN;
	for (i = 0; i < short_ssid_num; i++) {
		kalMemCopy(&arShortSSID.aucSsid,
			&cmd->aucShortSSID[i],
			MAX_SHORT_SSID_LEN);
		kalMemCopy(pos, &arShortSSID,
			sizeof(struct PARAM_SSID));
		pos += sizeof(struct PARAM_SSID);
	}

	return tag->u2Length;
}

uint16_t nicUniCmdScanTagBssid(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_BSSID *tag = (struct UNI_CMD_SCAN_BSSID *)buf;
	uint8_t i;

	if (cmd->ucScnFuncMask & ENUM_SCN_USE_PADDING_AS_BSSID ||
		cmd->u4ScnFuncMaskExtend & ENUM_SCN_ML_PROBE) {
		for (i = 0; i < CFG_SCAN_OOB_MAX_NUM; i++) {
			tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_BSSID;
			tag->u2Length = sizeof(*tag);
			kalMemCopy(tag->aucBssid,
				cmd->aucExtBSSID[i], MAC_ADDR_LEN);

			tag->ucBssidMatchCh = cmd->ucBssidMatchCh[i];
			tag->ucBssidMatchSsidInd = cmd->ucBssidMatchSsidInd[i];
			tag->ucBssidMatchShortSsidInd =
				cmd->ucBssidMatchShortSsidInd[i];
			tag++;
		}
		return ((uint8_t *)tag) - buf;
	} else {
		tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_BSSID;
		tag->u2Length = sizeof(*tag);
		kalMemCopy(tag->aucBssid, cmd->aucBSSID, MAC_ADDR_LEN);

		tag->ucBssidMatchCh = 0;
		tag->ucBssidMatchSsidInd = CFG_SCAN_OOB_MAX_NUM;
		tag->ucBssidMatchShortSsidInd = CFG_SCAN_OOB_MAX_NUM;

		return tag->u2Length;
	}
}

uint16_t nicUniCmdScanTagChnlInfo(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_CHANNEL_INFO *tag =
		(struct UNI_CMD_SCAN_CHANNEL_INFO *)buf;
	uint8_t i;
	uint8_t chnl_num, chnl_ext_num;
	uint8_t *pos = tag->aucChnlInfoBuffer;
	uint16_t len;

	if (cmd->ucChannelListNum > 32)
		chnl_num = 32;
	else
		chnl_num = cmd->ucChannelListNum;

	if (cmd->ucChannelListExtNum > 32)
		chnl_ext_num = 32;
	else
		chnl_ext_num = cmd->ucChannelListExtNum;

	if (cmd->ucChannelType == SCAN_CHANNEL_FULL &&
		(chnl_num + chnl_ext_num) == 0)
		return 0;

	len = sizeof(*tag) +
	ALIGN_4((chnl_num + chnl_ext_num) * sizeof(struct CHANNEL_INFO));

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_CHANNEL;
	tag->u2Length = len;
	tag->ucChannelType = cmd->ucChannelType;
	tag->ucChannelListNum = chnl_num + chnl_ext_num;
	for (i = 0; i < chnl_num; i++) {
		kalMemCopy(pos, &cmd->arChannelList[i],
			sizeof(struct CHANNEL_INFO));
		pos += sizeof(struct CHANNEL_INFO);
	}
	for (i = 0; i < chnl_ext_num; i++) {
		kalMemCopy(pos, &cmd->arChannelListExtend[i],
			sizeof(struct CHANNEL_INFO));
		pos += sizeof(struct CHANNEL_INFO);
	}
	return tag->u2Length;
}

uint16_t nicUniCmdScanTagIe(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_IE *tag;
	uint8_t *pos, *end;
	uint16_t fix = sizeof(struct UNI_CMD_SCAN_IE);

	pos = buf;
	end = buf + sizeof(struct UNI_CMD_SCAN_IE) + 600;

	if (cmd->u2IELen + cmd->u2IELenMl > 0) {
		if (pos + fix + ALIGN_4(cmd->u2IELen + cmd->u2IELenMl) < end) {
			tag = (struct UNI_CMD_SCAN_IE *)pos;
			tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_IE;
			tag->u2Length = fix +
				ALIGN_4(cmd->u2IELen + cmd->u2IELenMl);
			tag->u2IELen = cmd->u2IELen + cmd->u2IELenMl;
			tag->ucBand = BAND_NULL;
			if (cmd->u2IELen)
				kalMemCopy(tag->aucIEBuffer,
					cmd->aucIE, cmd->u2IELen);
			if (cmd->u2IELenMl) {
				kalMemCopy(tag->aucIEBuffer + cmd->u2IELen,
					cmd->aucIEMl, cmd->u2IELenMl);

				DBGLOG(INIT, DEBUG, "Dump ML IE\n");
				DBGLOG_MEM8(INIT, DEBUG,
					cmd->aucIEMl, cmd->u2IELenMl);
			}
			pos += tag->u2Length;
		} else {
			DBGLOG(INIT, ERROR, "no space for default IE\n");
		}
	}

	if (cmd->u2IELen2G4) {
		if (pos + fix + ALIGN_4(cmd->u2IELen2G4) < end) {
			tag = (struct UNI_CMD_SCAN_IE *)pos;
			tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_IE;
			tag->u2Length = fix + ALIGN_4(cmd->u2IELen2G4);
			tag->u2IELen = cmd->u2IELen2G4;
			tag->ucBand = BAND_2G4;
			kalMemCopy(tag->aucIEBuffer,
				cmd->aucIE2G4, cmd->u2IELen2G4);
			pos += tag->u2Length;

			DBGLOG(INIT, DEBUG, "Dump 2G4 IE\n");
			DBGLOG_MEM8(INIT, DEBUG,
				    tag->aucIEBuffer, tag->u2IELen);
		} else {
			DBGLOG(INIT, ERROR, "no space for 2G4 IE\n");
		}
	}
	if (cmd->u2IELen5G) {
		if (pos + fix + ALIGN_4(cmd->u2IELen5G) < end) {
			tag = (struct UNI_CMD_SCAN_IE *)pos;
			tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_IE;
			tag->u2Length = fix + ALIGN_4(cmd->u2IELen5G);
			tag->u2IELen = cmd->u2IELen5G;
			tag->ucBand = BAND_5G;
			kalMemCopy(tag->aucIEBuffer,
				cmd->aucIE5G, cmd->u2IELen5G);
			pos += tag->u2Length;

			DBGLOG(INIT, DEBUG, "Dump 5G IE\n");
			DBGLOG_MEM8(INIT, DEBUG,
				    tag->aucIEBuffer, tag->u2IELen);
		} else {
			DBGLOG(INIT, ERROR, "no space for 5G IE\n");
		}
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (cmd->u2IELen6G) {
		if (pos + fix + ALIGN_4(cmd->u2IELen6G) < end) {
			tag = (struct UNI_CMD_SCAN_IE *)pos;
			tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_IE;
			tag->u2Length = fix + ALIGN_4(cmd->u2IELen6G);
			tag->u2IELen = cmd->u2IELen6G;
			tag->ucBand = BAND_6G;
			kalMemCopy(tag->aucIEBuffer,
				cmd->aucIE6G, cmd->u2IELen6G);
			pos += tag->u2Length;

			DBGLOG(INIT, DEBUG, "Dump 6G IE\n");
			DBGLOG_MEM8(INIT, DEBUG,
				    tag->aucIEBuffer, tag->u2IELen);
		} else {
			DBGLOG(INIT, ERROR, "no space for 6G IE\n");
		}
	}
#endif
	return pos - buf;
}

uint16_t nicUniCmdScanTagMisc(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCAN_REQ_V2 *cmd)
{
	struct UNI_CMD_SCAN_MISC *tag = (struct UNI_CMD_SCAN_MISC *)buf;

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_MISC;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(tag->aucRandomMac, cmd->aucRandomMac, MAC_ADDR_LEN);

	tag->ucShortSSIDNum = cmd->ucShortSSIDNum;
	tag->u2OpChStayTimeMs = cmd->u2OpChStayTimeMs;
	tag->ucDfsChDwellTimeMs = cmd->ucDfsChDwellTimeMs;
	tag->ucPerScanChannelCnt = cmd->ucPerScanChannelCnt;

	return tag->u2Length;
}

struct UNI_CMD_SCAN_TAG_HANDLE arSetScanReqTable[] = {
	{sizeof(struct UNI_CMD_SCAN_REQ), nicUniCmdScanTagReq},
	{sizeof(struct UNI_CMD_SCAN_SSID) +
	 sizeof(struct PARAM_SSID) * CFG_SCAN_SSID_MAX_NUM,
	 nicUniCmdScanTagSsid},
	{sizeof(struct UNI_CMD_SCAN_SSID) +
	 sizeof(struct PARAM_SSID) * CFG_SCAN_OOB_MAX_NUM,
	 nicUniCmdScanTagShortSsid},
	{sizeof(struct UNI_CMD_SCAN_BSSID) * CFG_SCAN_OOB_MAX_NUM,
	 nicUniCmdScanTagBssid},
	{sizeof(struct UNI_CMD_SCAN_CHANNEL_INFO) +
	 sizeof(struct CHANNEL_INFO) * 64,
	 nicUniCmdScanTagChnlInfo},
	{sizeof(struct UNI_CMD_SCAN_IE) + 600, nicUniCmdScanTagIe},
	{sizeof(struct UNI_CMD_SCAN_MISC), nicUniCmdScanTagMisc},
};

uint32_t nicUniCmdScanReqV2(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SCAN_REQ_V2 *cmd;
	struct UNI_CMD_SCAN *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t *pos;
	uint32_t max_cmd_len = 0;
	int i;

	if (info->ucCID != CMD_ID_SCAN_REQ_V2 ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SCAN_REQ_V2 *)info->pucInfoBuffer;
	max_cmd_len += sizeof(struct UNI_CMD_SCAN);
	for (i = 0; i < ARRAY_SIZE(arSetScanReqTable); i++)
		max_cmd_len += arSetScanReqTable[i].u4Size;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SCAN_REQ,
			max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	pos = entry->pucInfoBuffer;
	uni_cmd = (struct UNI_CMD_SCAN *) pos;
	uni_cmd->ucSeqNum = cmd->ucSeqNum;
	uni_cmd->ucBssIndex = cmd->ucBssIndex;
	pos += sizeof(*uni_cmd);
	for (i = 0; i < ARRAY_SIZE(arSetScanReqTable); i++)
		pos += arSetScanReqTable[i].pfHandler(ad, pos, cmd);
	entry->u4SetQueryInfoLen = pos - entry->pucInfoBuffer;

	ASSERT(entry->u4SetQueryInfoLen <= max_cmd_len);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdBssInfoPhyMode(uint8_t ucPhyTypeSet)
{
	uint32_t phy_mode = 0;

	if (ucPhyTypeSet & PHY_TYPE_BIT_HR_DSSS)
		phy_mode |= WMODE_B;

	if (ucPhyTypeSet & PHY_TYPE_BIT_ERP) {
		phy_mode |= WMODE_G;
	}

	if (ucPhyTypeSet & PHY_TYPE_BIT_OFDM) {
		phy_mode |= WMODE_A;
	}

	if (ucPhyTypeSet & PHY_TYPE_BIT_HT) {
		phy_mode |= WMODE_GN;
		phy_mode |= WMODE_AN;
	}

	if (ucPhyTypeSet & PHY_TYPE_BIT_VHT)
		phy_mode |= WMODE_AC;

#if (CFG_SUPPORT_802_11AX == 1)
	if (ucPhyTypeSet & PHY_TYPE_BIT_HE) {
		phy_mode |= WMODE_AX_24G;
		phy_mode |= WMODE_AX_5G;
		phy_mode |= WMODE_AX_6G;
	}
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	if (ucPhyTypeSet & PHY_TYPE_BIT_EHT) {
		phy_mode |= WMODE_BE_24G;
		phy_mode |= WMODE_BE_5G;
		phy_mode |= WMODE_BE_6G;
	}
#endif

	return phy_mode;
}

uint32_t nicUniCmdBssInfoMld(struct ADAPTER *ad,
	uint8_t *buf, uint8_t ucBssIndex)
{
	struct UNI_CMD_BSSINFO_MLD *tag = (struct UNI_CMD_BSSINFO_MLD *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, ucBssIndex);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBssInfo = mldBssGetByBss(ad, bss);
#endif
	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_MLD;
	tag->u2Length = sizeof(*tag);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (prMldBssInfo) {
		tag->ucGroupMldId = prMldBssInfo->ucGroupMldId;
		tag->ucOwnMldId = bss->ucOwnMldId;
		COPY_MAC_ADDR(tag->aucOwnMldAddr, prMldBssInfo->aucOwnMldAddr);
		tag->ucOmRemapIdx = prMldBssInfo->ucOmRemapIdx;
		tag->ucLinkId = bss->ucLinkId;
		tag->ucEmlEnabled = prMldBssInfo->ucEmlEnabled;
		tag->ucMaxSimuLinks = prMldBssInfo->ucMaxSimuLinks;
		tag->ucHmloEnabled = prMldBssInfo->ucHmloEnabled;
	} else
#endif
	{
		tag->ucGroupMldId = MLD_GROUP_NONE;
		tag->ucOwnMldId = bss->ucOwnMldId;
		COPY_MAC_ADDR(tag->aucOwnMldAddr, bss->aucOwnMacAddr);
		tag->ucOmRemapIdx = OM_REMAP_IDX_NONE;
		tag->ucLinkId = MLD_LINK_ID_NONE;
		tag->ucEmlEnabled = 0;
		tag->ucMaxSimuLinks = 0;
		tag->ucHmloEnabled = 0;
	}

	DBGLOG(INIT, DEBUG,
		"Bss=%d, GroupMldId=%d, OwnMldId=%d, OmRemapIdx=%d, LinkId=%d, Eml=%d, MaxSimuLinks=%d, OwnMldAddr="
		MACSTR "HyMlo=%d\n",
		bss->ucBssIndex,
		tag->ucGroupMldId,
		tag->ucOwnMldId,
		tag->ucOmRemapIdx,
		tag->ucLinkId,
		tag->ucEmlEnabled,
		tag->ucMaxSimuLinks,
		MAC2STR(tag->aucOwnMldAddr),
		tag->ucHmloEnabled);

	return tag->u2Length;
}

uint32_t nicUniCmdBssActivateCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_BSS_ACTIVATE_CTRL *cmd;
	struct UNI_CMD_DEVINFO *dev_cmd;
	struct UNI_CMD_BSSINFO *bss_cmd;
	struct UNI_CMD_DEVINFO_ACTIVE *dev_active_tag;
	struct UNI_CMD_BSSINFO_BASIC *bss_basic_tag;
	struct UNI_CMD_BSSINFO_MLD *mld_tag;
	struct WIFI_UNI_CMD_ENTRY *dev_entry = NULL, *bss_entry = NULL;
	uint32_t max_cmd_len;
	struct BSS_INFO *bss;
	uint32_t phy_mode;

	cmd = (struct CMD_BSS_ACTIVATE_CTRL *) info->pucInfoBuffer;
	bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (info->ucCID != CMD_ID_BSS_ACTIVATE_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	/* update devinfo */
	max_cmd_len = sizeof(struct UNI_CMD_DEVINFO) +
		      sizeof(struct UNI_CMD_DEVINFO_ACTIVE);
	dev_entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_DEVINFO,
			max_cmd_len, NULL, NULL);
	if (!dev_entry)
		goto fail;
	dev_cmd = (struct UNI_CMD_DEVINFO *) dev_entry->pucInfoBuffer;
	dev_cmd->ucOwnMacIdx = cmd->ucOwnMacAddrIndex;
#if (CFG_SUPPORT_CONNAC3X == 1)
	dev_cmd->ucDbdcIdx = bss->ucBssIndex == ad->ucP2PDevBssIdx ?
		ENUM_BAND_ALL : ENUM_BAND_AUTO;
#else
	dev_cmd->ucDbdcIdx = ENUM_BAND_AUTO;
#endif
	dev_active_tag = (struct UNI_CMD_DEVINFO_ACTIVE *)dev_cmd->aucTlvBuffer;
	dev_active_tag->u2Tag = UNI_CMD_DEVINFO_TAG_ACTIVE;
	dev_active_tag->u2Length = sizeof(*dev_active_tag);
	dev_active_tag->ucActive = cmd->ucActive;
	dev_active_tag->ucMldLinkIdx = cmd->ucMldLinkIdx;
	COPY_MAC_ADDR(dev_active_tag->aucOwnMacAddr, cmd->aucBssMacAddr);

	/* update bssinfo */
	max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
		sizeof(struct UNI_CMD_BSSINFO_BASIC) +
		sizeof(struct UNI_CMD_BSSINFO_MLD);
	bss_entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);
	if (!bss_entry)
		goto fail;

	bss_cmd = (struct UNI_CMD_BSSINFO *) bss_entry->pucInfoBuffer;
	bss_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	bss_basic_tag = (struct UNI_CMD_BSSINFO_BASIC *)bss_cmd->aucTlvBuffer;
	bss_basic_tag->u2Tag = UNI_CMD_BSSINFO_TAG_BASIC;
	bss_basic_tag->u2Length = sizeof(*bss_basic_tag);
	bss_basic_tag->ucActive = cmd->ucActive;
	bss_basic_tag->ucDbdcIdx = ENUM_BAND_AUTO;
	bss_basic_tag->ucOwnMacIdx = cmd->ucOwnMacAddrIndex;
	bss_basic_tag->ucHwBSSIndex = cmd->ucOwnMacAddrIndex;
	bss_basic_tag->u4ConnectionType = bssInfoConnType(ad, bss);
	bss_basic_tag->ucConnectionState = bss->eConnectionState;
	bss_basic_tag->ucWmmIdx = bss->ucWmmQueSet;
	COPY_MAC_ADDR(bss_basic_tag->aucBSSID, bss->aucBSSID);
	bss_basic_tag->u2BcMcWlanidx = cmd->ucBMCWlanIndex;
	bss_basic_tag->u2BcnInterval = bss->u2BeaconInterval;
	bss_basic_tag->ucDtimPeriod = bss->ucDTIMPeriod;
	bss_basic_tag->u2StaRecIdxOfAP = STA_REC_INDEX_NOT_FOUND;
	bss_basic_tag->u2NonHTBasicPhyType = bss->ucNonHTBasicPhyType;
	phy_mode = nicUniCmdBssInfoPhyMode(bss->ucPhyTypeSet);
	bss_basic_tag->ucPhyMode = phy_mode & 0xff;
	bss_basic_tag->ucPhyModeExt = (phy_mode >> 8) & 0xff;
	bss_basic_tag->ucMldLinkIdx = cmd->ucMldLinkIdx;
	mld_tag = (struct UNI_CMD_BSSINFO_MLD *)
		(bss_cmd->aucTlvBuffer + sizeof(*bss_basic_tag));
	mld_tag->u2Tag = UNI_CMD_BSSINFO_TAG_MLD;
	mld_tag->u2Length = sizeof(*mld_tag);
	mld_tag->ucGroupMldId = MLD_GROUP_NONE;
	mld_tag->ucOwnMldId = bss->ucOwnMldId;
	COPY_MAC_ADDR(mld_tag->aucOwnMldAddr, bss->aucOwnMacAddr);
	mld_tag->ucOmRemapIdx = OM_REMAP_IDX_NONE;
	mld_tag->ucLinkId = MLD_LINK_ID_NONE;
	DBGLOG(INIT, DEBUG,
		"%s DevInfo[OMAC=%d, DBDC=%d], BssInfo%d[DBDC=%d, OMAC=%d, WMM=%d, ConnType=%d, ConnState=%d, BcIdx=%d, PhyMode=0x%x, PhyModeEx=0x%x, DTIM=%d]\n",
		cmd->ucActive ? "Activate" : "Deactivate",
		dev_cmd->ucOwnMacIdx, dev_cmd->ucDbdcIdx,
		bss_cmd->ucBssInfoIdx, bss_basic_tag->ucDbdcIdx,
		bss_basic_tag->ucOwnMacIdx, bss_basic_tag->ucWmmIdx,
		bss_basic_tag->u4ConnectionType,
		bss_basic_tag->ucConnectionState, bss_basic_tag->u2BcMcWlanidx,
		bss_basic_tag->ucPhyMode, bss_basic_tag->ucPhyModeExt,
		bss_basic_tag->ucDtimPeriod);

	if (cmd->ucActive) {
		/* activate devinfo first */
		LINK_INSERT_TAIL(&info->rUniCmdList, &dev_entry->rLinkEntry);
		LINK_INSERT_TAIL(&info->rUniCmdList, &bss_entry->rLinkEntry);
	} else {
		/* deactivate bssinfo first */
		LINK_INSERT_TAIL(&info->rUniCmdList, &bss_entry->rLinkEntry);
		LINK_INSERT_TAIL(&info->rUniCmdList, &dev_entry->rLinkEntry);
	}

	return WLAN_STATUS_SUCCESS;

fail:
	nicUniCmdFreeEntry(ad, dev_entry);
	nicUniCmdFreeEntry(ad, bss_entry);
	return WLAN_STATUS_RESOURCES;
}

uint32_t nicUniCmdCustomerCfg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_HEADER *cmd;
	struct UNI_CMD_CHIP_CONFIG *uni_cmd;
	struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CHIP_CONFIG) +
	     		       sizeof(struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG);

	if (info->ucCID != CMD_ID_GET_SET_CUSTOMER_CFG ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_HEADER *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CHIP_CONFIG, max_cmd_len,
			info->fgSetQuery ? NULL : nicUniCmdEventQueryCfgRead,
			info->fgSetQuery ? NULL : nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CHIP_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_CHIP_CONFIG_TAG_CUSTOMER_CFG;
	tag->u2Length = sizeof(*tag);
	tag->cmdBufferLen = cmd->cmdBufferLen;
	tag->itemNum = cmd->itemNum;
	kalMemCopy(tag->aucbuffer, cmd->buffer, cmd->cmdBufferLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdChipCfg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_CHIP_CONFIG *cmd;
	struct UNI_CMD_CHIP_CONFIG *uni_cmd;
	struct UNI_CMD_CHIP_CONFIG_CHIP_CFG *tag;
	struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP *resp;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CHIP_CONFIG) +
			       sizeof(struct UNI_CMD_CHIP_CONFIG_CHIP_CFG) +
			       sizeof(struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP);

	if (info->ucCID != CMD_ID_CHIP_CONFIG ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CHIP_CONFIG *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CHIP_CONFIG, max_cmd_len,
			info->fgSetQuery ? nicUniCmdEventSetCommon :
			nicUniEventQueryChipConfig,
			nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CHIP_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_CHIP_CONFIG_CHIP_CFG *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_CHIP_CONFIG_TAG_CHIP_CFG;
	tag->u2Length = sizeof(*tag) + sizeof(*resp);
	resp = (struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP *) tag->aucbuffer;
	resp->u2Id = cmd->u2Id;
	resp->u2MsgSize = cmd->u2MsgSize;
	resp->ucType = cmd->ucType;
	resp->ucRespType = cmd->ucRespType;
	kalMemCopy(resp->aucCmd, cmd->aucCmd, cmd->u2MsgSize);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSwDbgCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SW_DBG_CTRL *cmd;
	struct UNI_CMD_CHIP_CONFIG *uni_cmd;
	struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CHIP_CONFIG) +
			       sizeof(struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL);

	if (info->ucCID != CMD_ID_SW_DBG_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SW_DBG_CTRL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CHIP_CONFIG,	max_cmd_len,
			info->fgSetQuery ? nicUniCmdEventSetCommon :
			nicUniEventQuerySwDbgCtrl, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CHIP_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_CHIP_CONFIG_TAG_SW_DBG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->u4Id = cmd->u4Id;
	tag->u4Data = cmd->u4Data;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetRxFilter(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_RX_PACKET_FILTER *cmd;
	struct UNI_CMD_BAND_CONFIG *uni_cmd;
	struct UNI_CMD_BAND_CONFIG_SET_RX_FILTER *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BAND_CONFIG) +
	       		       sizeof(struct UNI_CMD_BAND_CONFIG_SET_RX_FILTER);

	if (info->ucCID != CMD_ID_SET_RX_FILTER ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_RX_PACKET_FILTER *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BAND_CONFIG,
				max_cmd_len, nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BAND_CONFIG *) entry->pucInfoBuffer;
	uni_cmd->ucDbdcIdx = ENUM_BAND_ALL;
	tag = (struct UNI_CMD_BAND_CONFIG_SET_RX_FILTER *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BAND_CONFIG_TAG_SET_RX_FILTER;
	tag->u2Length = sizeof(*tag);
	tag->u4RxPacketFilter = cmd->u4RxPacketFilter;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetMbmc(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_DBDC_SETTING *cmd;
	struct UNI_CMD_MBMC *uni_cmd;
	struct UNI_CMD_MBMC_SETTING *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MBMC) +
	     		       sizeof(struct UNI_CMD_MBMC_SETTING);

	if (info->ucCID != CMD_ID_SET_DBDC_PARMS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_DBDC_SETTING *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MBMC,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MBMC *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MBMC_SETTING *) uni_cmd->aucTlvBuffer;
#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
	if (cmd->ucNoResp)
		tag->u2Tag = UNI_CMD_MBMC_NO_RESP_TAG_SETTING;
	else
#endif
		tag->u2Tag = UNI_CMD_MBMC_TAG_SETTING;
	tag->u2Length = sizeof(*tag);
	tag->ucMbmcEn = cmd->ucDbdcEn;
	tag->ucAAModeEn = cmd->ucDBDCAAMode;
	tag->ucRfBand = 0; /* unused */
	tag->ucReason = cmd->ucReason;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetMdvt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_MDVT_CFG *cmd;
	struct UNI_CMD_DVT *dvt_cmd;
	struct UNI_CMD_MDVT_PARA *mdvt;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MDVT_PARA) +
			       sizeof(struct UNI_CMD_DVT);

	if (info->ucCID != CMD_ID_SET_MDVT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_MDVT_CFG *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_DVT,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	dvt_cmd = (struct UNI_CMD_DVT *) entry->pucInfoBuffer;
	dvt_cmd->ucTestType = UNI_CMD_MODULE_DVT;

	mdvt = (struct UNI_CMD_MDVT_PARA *) dvt_cmd->aucTlvBuffer;
	mdvt->u2Tag = UNI_CMD_DVT_TAG_SET_PARA;
	mdvt->u2Length = sizeof(*mdvt);
	mdvt->u2ModuleId = cmd->u4ModuleId;
	mdvt->u2CaseId = cmd->u4CaseId;
	mdvt->ucCapId = cmd->ucCapId;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdPowerCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_NIC_POWER_CTRL *cmd;
	struct UNI_CMD_POWER_CTRL *uni_cmd;
	struct UNI_CMD_POWER_OFF *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_POWER_CTRL) +
	     		       sizeof(struct UNI_CMD_POWER_OFF);

	if (info->ucCID != CMD_ID_NIC_POWER_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_NIC_POWER_CTRL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_CTRL,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_CTRL *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_POWER_OFF *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_POWER_CTRL_TAG_OFF;
	tag->u2Length = sizeof(*tag);
	tag->ucPowerMode = cmd->ucPowerMode;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetDomainV1(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_DOMAIN_INFO *cmd;
	struct UNI_CMD_DOMAIN_SET_INFO *uni_cmd;
	struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_SUBBAND *tag;
	struct UNI_CMD_DOMAIN_SUBBAND_INFO *sub;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_DOMAIN_SET_INFO) +
	     	sizeof(struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_SUBBAND) +
		sizeof(struct UNI_CMD_DOMAIN_SUBBAND_INFO) * MAX_SUBBAND_NUM;
	uint8_t i;

	cmd = (struct CMD_SET_DOMAIN_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SET_DOMAIN_INFO,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_DOMAIN_SET_INFO *) entry->pucInfoBuffer;
	uni_cmd->u4CountryCode = cmd->u2CountryCode;
	uni_cmd->uc2G4Bandwidth = cmd->uc2G4Bandwidth;
	uni_cmd->uc5GBandwidth = cmd->uc5GBandwidth;
	uni_cmd->uc6GBandwidth = cmd->uc5GBandwidth; // TODO: uni cmd, 6g
	uni_cmd->ucRegCat = cmd->ucRegCat;

	tag = (struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_SUBBAND *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_DOMAIN_TAG_SUBBAND;
	tag->u2Length = sizeof(*tag) +
		sizeof(struct UNI_CMD_DOMAIN_SUBBAND_INFO) * MAX_SUBBAND_NUM;

	tag->u2IsSetPassiveScan = cmd->u2IsSetPassiveScan;
	tag->ucSubBandNum = MAX_SUBBAND_NUM;
	sub = (struct UNI_CMD_DOMAIN_SUBBAND_INFO *)tag->aucSubBandInfoBuffer;
	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		sub->ucRegClass = cmd->rSubBand[i].ucRegClass;
		sub->ucBand = cmd->rSubBand[i].ucBand;
		sub->ucChannelSpan = cmd->rSubBand[i].ucChannelSpan;
		sub->ucFirstChannelNum = cmd->rSubBand[i].ucFirstChannelNum;
		sub->ucNumChannels = cmd->rSubBand[i].ucNumChannels;
		sub->fgDfs = cmd->rSubBand[i].fgDfs;
		sub++;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	DBGLOG(RLM, TRACE,
	       "[UNI]Domain: uni_CC=%c%c, cmd_CC=%c%c, uni_Reg=%d, cmd_Reg=%d\n",
	       (uni_cmd->u4CountryCode & 0xff00) >> 8,
	       (uni_cmd->u4CountryCode & 0x00ff),
	       ((cmd->u2CountryCode & 0xff00) >> 8),
	       (cmd->u2CountryCode & 0x00ff),
	       uni_cmd->ucRegCat,
	       cmd->ucRegCat);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetDomainV2(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_DOMAIN_INFO_V2 *cmd;
	struct UNI_CMD_DOMAIN_SET_INFO *uni_cmd;
	struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_ACTIVE_CHANNEL_LIST *tag;
	struct CMD_DOMAIN_CHANNEL *chl_dst, *chl_src;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len;
	uint8_t i;
	uint8_t valid_channel_count = 0;

	cmd = (struct CMD_SET_DOMAIN_INFO_V2 *) info->pucInfoBuffer;
	valid_channel_count = cmd->arActiveChannels.u1ActiveChNum2g +
			      cmd->arActiveChannels.u1ActiveChNum5g +
			      cmd->arActiveChannels.u1ActiveChNum6g;
	if (valid_channel_count > MAX_CHN_NUM) {
		DBGLOG(INIT, WARN,
			"valid_channel_count=%d more than MAX_CHN_NUM\n",
			valid_channel_count);
		return WLAN_STATUS_NOT_ACCEPTED;
	}
	max_cmd_len = sizeof(struct UNI_CMD_DOMAIN_SET_INFO) +
	     sizeof(struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_ACTIVE_CHANNEL_LIST) +
	     (valid_channel_count * sizeof(struct CMD_DOMAIN_CHANNEL));
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SET_DOMAIN_INFO,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_DOMAIN_SET_INFO *) entry->pucInfoBuffer;
	uni_cmd->u4CountryCode = cmd->u4CountryCode;
	uni_cmd->uc2G4Bandwidth = cmd->uc2G4Bandwidth;
	uni_cmd->uc5GBandwidth = cmd->uc5GBandwidth;
	uni_cmd->uc6GBandwidth = cmd->uc6GBandwidth;

	tag = (struct UNI_CMD_DOMAIN_SET_INFO_DOMAIN_ACTIVE_CHANNEL_LIST *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_DOMAIN_TAG_ACTCHNL;
	tag->u2Length = sizeof(*tag) +
			sizeof(struct CMD_DOMAIN_CHANNEL) * valid_channel_count;
	tag->u1ActiveChNum2g = cmd->arActiveChannels.u1ActiveChNum2g;
	tag->u1ActiveChNum5g = cmd->arActiveChannels.u1ActiveChNum5g;
	tag->u1ActiveChNum6g = cmd->arActiveChannels.u1ActiveChNum6g;

	chl_dst = (struct CMD_DOMAIN_CHANNEL*) tag->aucActChnlListBuffer;
	chl_src = cmd->arActiveChannels.arChannels;
	for (i = 0; i < valid_channel_count; i++) {
		kalMemCopy(chl_dst, chl_src, sizeof(*chl_dst));
		chl_dst++;
		chl_src++;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetDomain(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	if (info->ucCID != CMD_ID_SET_DOMAIN_INFO)
		return WLAN_STATUS_NOT_ACCEPTED;

	if (info->u4SetQueryInfoLen == sizeof(struct CMD_SET_DOMAIN_INFO))
		return nicUniCmdSetDomainV1(ad, info);
	else
		return nicUniCmdSetDomainV2(ad, info);
}

uint32_t nicUniCmdScanCancel(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SCAN_CANCEL *cmd;
	struct UNI_CMD_SCAN *uni_cmd;
	struct UNI_CMD_SCAN_CANCEL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SCAN) +
	     		       sizeof(struct UNI_CMD_SCAN_CANCEL);

	if (info->ucCID != CMD_ID_SCAN_CANCEL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SCAN_CANCEL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SCAN_REQ,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SCAN *) entry->pucInfoBuffer;
	uni_cmd->ucSeqNum = cmd->ucSeqNum;
	uni_cmd->ucBssIndex = 0; /* unused */
	tag = (struct UNI_CMD_SCAN_CANCEL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_CANCEL;
	tag->u2Length = sizeof(*tag);
	tag->ucIsExtChannel = cmd->ucIsExtChannel;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSchedScanEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_SCHED_SCAN_ENABLE *cmd;
	struct UNI_CMD_SCAN *uni_cmd;
	struct UNI_CMD_SCAN_SCHED_SCAN_ENABLE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SCAN) +
	     		       sizeof(struct UNI_CMD_SCAN_SCHED_SCAN_ENABLE);

	if (info->ucCID != CMD_ID_SET_SCAN_SCHED_ENABLE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_SCHED_SCAN_ENABLE *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SCAN_REQ,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SCAN *) entry->pucInfoBuffer;
	uni_cmd->ucSeqNum = 0; /* unused */
	uni_cmd->ucBssIndex = 0; /* unused */
	tag = (struct UNI_CMD_SCAN_SCHED_SCAN_ENABLE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SCAN_TAG_SCHED_SCAN_ENABLE;
	tag->u2Length = sizeof(*tag);
	tag->ucSchedScanAct = cmd->ucSchedScanAct;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint16_t nicUniCmdSchedScanTagReq(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SCHED_SCAN_REQ *cmd)
{
	struct UNI_CMD_SCAN_SCHED_SCAN_REQ *tag =
			(struct UNI_CMD_SCAN_SCHED_SCAN_REQ *)buf;

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCHED_SCAN_REQ;
	tag->u2Length = sizeof(*tag);
	tag->ucVersion = cmd->ucVersion;
	tag->fgStopAfterIndication = cmd->fgStopAfterIndication;
	tag->ucMspEntryNum = cmd->ucMspEntryNum;
	tag->ucScnFuncMask = cmd->ucScnFuncMask;
	kalMemCopy(tag->au2MspList, cmd->au2MspList, sizeof(tag->au2MspList));

	return tag->u2Length;
}

uint16_t nicUniCmdSchedScanTagSsid(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCHED_SCAN_REQ *cmd)
{
	struct UNI_CMD_SCAN_SSID *tag = (struct UNI_CMD_SCAN_SSID *)buf;
	uint8_t i;
	uint8_t num;
	uint8_t *pos = tag->aucSsidBuffer;
	uint16_t len;

	if (cmd->ucSsidNum > 10)
		num = 10;
	else if (cmd->ucSsidNum != 0)
		num = cmd->ucSsidNum;
	else
		return 0;

	len = sizeof(*tag) + num * sizeof(struct PARAM_SSID);

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_SSID;
	tag->u2Length = len;
	tag->ucSSIDType = SCAN_REQ_SSID_SPECIFIED;
	tag->ucSSIDNum = num;
	for (i = 0; i < num; i++) {
		kalMemCopy(pos, &cmd->auSsid[i], sizeof(struct PARAM_SSID));
		pos += sizeof(struct PARAM_SSID);
	}

	return tag->u2Length;
}

uint16_t nicUniCmdSchedScanTagChnlInfo(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCHED_SCAN_REQ *cmd)
{
	struct UNI_CMD_SCAN_CHANNEL_INFO *tag =
		(struct UNI_CMD_SCAN_CHANNEL_INFO *)buf;
	uint8_t i;
	uint8_t num;
	uint8_t *pos = tag->aucChnlInfoBuffer;
	uint16_t len;

	if (cmd->ucChnlNum > 64)
		num = 64;
	else if (cmd->ucChnlNum != 0)
		num = cmd->ucChnlNum;
	else
		return 0;

	len = sizeof(*tag) +
			ALIGN_4(num * sizeof(struct CHANNEL_INFO));

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_CHANNEL;
	tag->u2Length = len;
	tag->ucChannelType = cmd->ucChannelType;
	tag->ucChannelListNum = num;
	for (i = 0; i < num; i++) {
		kalMemCopy(pos, &cmd->aucChannel[i],
			sizeof(struct CHANNEL_INFO));
		pos += sizeof(struct CHANNEL_INFO);
	}

	return tag->u2Length;
}

uint16_t nicUniCmdSchedScanTagIe(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCHED_SCAN_REQ *cmd)
{
	struct UNI_CMD_SCAN_IE *tag = (struct UNI_CMD_SCAN_IE *)buf;

	if (cmd->u2IELen == 0)
		return 0;

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_IE;
	tag->u2Length = sizeof(*tag) + ALIGN_4(cmd->u2IELen);
	tag->u2IELen = cmd->u2IELen;
	kalMemCopy(tag->aucIEBuffer, cmd->aucIE, cmd->u2IELen);

	return tag->u2Length;
}

uint16_t nicUniCmdSchedScanTagSsidMatchSets(struct ADAPTER *ad, uint8_t *buf,
	struct CMD_SCHED_SCAN_REQ *cmd)
{
	struct UNI_CMD_SCAN_SSID_MATCH_SETS *tag =
		(struct UNI_CMD_SCAN_SSID_MATCH_SETS *)buf;
	uint8_t i;
	uint8_t num;
	uint8_t *pos = tag->aucMatchSsidBuffer;
	uint16_t len;

	if (cmd->ucMatchSsidNum > 16)
		num = 16;
	else if (cmd->ucMatchSsidNum != 0)
		num = cmd->ucMatchSsidNum;
	else
		return 0;

	len = sizeof(*tag) + num * sizeof(struct SSID_MATCH_SETS);

	tag->u2Tag = UNI_CMD_SCAN_TAG_SCAN_SSID_MATCH_SETS;
	tag->u2Length = len;
	tag->ucMatchSsidNum = num;
	for (i = 0; i < num; i++) {
		kalMemCopy(pos, &cmd->auMatchSsid[i],
			sizeof(struct SSID_MATCH_SETS));
		pos += sizeof(struct SSID_MATCH_SETS);
	}

	return tag->u2Length;
}

struct UNI_CMD_SCHED_SCAN_TAG_HANDLE arSetSchedScanReqTable[] = {
	{sizeof(struct UNI_CMD_SCAN_SCHED_SCAN_REQ), nicUniCmdSchedScanTagReq},
	{sizeof(struct UNI_CMD_SCAN_SSID) + sizeof(struct PARAM_SSID) * 10,
	 nicUniCmdSchedScanTagSsid},
	{sizeof(struct UNI_CMD_SCAN_CHANNEL_INFO) +
	 sizeof(struct CHANNEL_INFO) * 64,
	 nicUniCmdSchedScanTagChnlInfo},
	{sizeof(struct UNI_CMD_SCAN_IE) + 600, nicUniCmdSchedScanTagIe},
	{sizeof(struct UNI_CMD_SCAN_SSID_MATCH_SETS) +
	 sizeof(struct SSID_MATCH_SETS) * 16,
	 nicUniCmdSchedScanTagSsidMatchSets},
};

uint32_t nicUniCmdSchedScanReq(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SCHED_SCAN_REQ *cmd;
	struct UNI_CMD_SCAN *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t *pos;
	uint32_t max_cmd_len = 0;
	int i;

	if (info->ucCID != CMD_ID_SET_SCAN_SCHED_REQ)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SCHED_SCAN_REQ *)info->pucInfoBuffer;
	max_cmd_len += sizeof(struct UNI_CMD_SCAN);
	for (i = 0; i < ARRAY_SIZE(arSetSchedScanReqTable); i++)
		max_cmd_len += arSetSchedScanReqTable[i].u4Size;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SCAN_REQ,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	pos = entry->pucInfoBuffer;
	uni_cmd = (struct UNI_CMD_SCAN *) pos;
	uni_cmd->ucSeqNum = cmd->ucSeqNum;
	uni_cmd->ucBssIndex = cmd->ucBssIndex;
	pos += sizeof(*uni_cmd);
	for (i = 0; i < ARRAY_SIZE(arSetSchedScanReqTable); i++)
		pos += arSetSchedScanReqTable[i].pfHandler(ad, pos, cmd);
	entry->u4SetQueryInfoLen = pos - entry->pucInfoBuffer;

	ASSERT(entry->u4SetQueryInfoLen <= max_cmd_len);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdWsysFwLogUI(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_EVENT_LOG_UI_INFO *cmd;
	struct UNI_CMD_WSYS_CONFIG *uni_cmd;
	struct UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
	     		      sizeof(struct UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL);

	if (info->ucCID != CMD_ID_LOG_UI_INFO ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_EVENT_LOG_UI_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_WSYS_CONFIG,
		  max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_WSYS_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_WSYS_CONFIG_FW_LOG_UI_CTRL *)
						uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_UI_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucVersion = cmd->ucVersion;
	tag->ucLogLevel = cmd->ucLogLevel;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdWsysFwLog2Host(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_FW_LOG_2_HOST_CTRL *cmd;
	struct UNI_CMD_WSYS_CONFIG *uni_cmd;
	struct UNI_CMD_FW_LOG_CTRL_BASIC *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
			       sizeof(struct UNI_CMD_FW_LOG_CTRL_BASIC);

	if (info->ucCID != CMD_ID_FW_LOG_2_HOST ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_FW_LOG_2_HOST_CTRL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_WSYS_CONFIG,
		  max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_WSYS_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_FW_LOG_CTRL_BASIC *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucFwLog2HostCtrl = cmd->ucFwLog2HostCtrl;
	tag->ucFwLog2HostInterval = 0;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdWsysFwBasicConfig(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_BASIC_CONFIG *cmd;
	struct UNI_CMD_WSYS_CONFIG *uni_cmd;
	struct UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
	     		     sizeof(struct UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG);

	if (info->ucCID != CMD_ID_BASIC_CONFIG ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_BASIC_CONFIG *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_WSYS_CONFIG,
		  max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_WSYS_CONFIG *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_WSYS_CONFIG_FW_BASIC_CONFIG *)
						uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_WSYS_CONFIG_TAG_FW_BASIC_CONFIG;
	tag->u2Length = sizeof(*tag);
	tag->u2RxChecksum = cmd->rCsumOffload.u2RxChecksum;
	tag->u2TxChecksum = cmd->rCsumOffload.u2TxChecksum;
	tag->ucCtrlFlagAssertPath = cmd->ucCtrlFlagAssertPath;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetSuspendMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SUSPEND_MODE_SETTING *cmd;
	struct UNI_CMD_SUSPEND *uni_cmd;
	struct UNI_CMD_SUSPEND_MODE_SETTING *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SUSPEND) +
	     		       sizeof(struct UNI_CMD_SUSPEND_MODE_SETTING);

	if (info->ucCID != CMD_ID_SET_SUSPEND_MODE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SUSPEND_MODE_SETTING *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SUSPEND,
		  max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SUSPEND *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_SUSPEND_MODE_SETTING *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SUSPEND_TAG_MODE_SETTING;
	tag->u2Length = sizeof(*tag);
	tag->ucScreenStatus = cmd->ucEnableSuspendMode;
	tag->ucMdtim = cmd->ucMdtim;
	tag->ucWowSuspend = cmd->ucWowSuspend;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

void nicUniCmdWowEventSetCb(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	if (prCmdInfo->ucCID == UNI_CMD_ID_SUSPEND) {
		DBGLOG(INIT, STATE, "CMD_ID_SET_WOWLAN cmd done\n");
		prAdapter->fgSetWowDone = TRUE;
	}
}

uint32_t nicUniCmdSetWOWLAN(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_WOWLAN_PARAM *cmd;
	struct UNI_CMD_SUSPEND *uni_cmd;
	struct UNI_CMD_SUSPEND_WOW_CTRL *ctrl_tag;
	struct UNI_CMD_SUSPEND_WOW_GPIO_PARAM *gpio_tag;
	struct UNI_CMD_SUSPEND_WOW_WAKEUP_PORT *port_tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
	     		       sizeof(struct UNI_CMD_SUSPEND_WOW_CTRL) +
			       sizeof(struct UNI_CMD_SUSPEND_WOW_GPIO_PARAM) +
			       sizeof(struct UNI_CMD_SUSPEND_WOW_WAKEUP_PORT);
	uint8_t *pos;

	if (info->ucCID != CMD_ID_SET_WOWLAN ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_WOWLAN_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SUSPEND,
		  max_cmd_len, nicUniCmdWowEventSetCb, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SUSPEND *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = 0; /* unused */
	pos = uni_cmd->aucTlvBuffer;

	DBGLOG(PF, STATE,
			"unicmd DetectType[0x%x] DetectTypeExt[0x%x]\n",
			cmd->ucDetectType,
			cmd->u2DetectTypeExt);

	ctrl_tag = (struct UNI_CMD_SUSPEND_WOW_CTRL *) pos;
	ctrl_tag->u2Tag = UNI_CMD_SUSPEND_TAG_WOW_CTRL;
	ctrl_tag->u2Length = sizeof(*ctrl_tag);
	ctrl_tag->ucCmd = cmd->ucCmd;
	ctrl_tag->ucDetectType = cmd->ucDetectType;
	ctrl_tag->ucWakeupHif = cmd->astWakeHif[0].ucWakeupHif;
	ctrl_tag->u2DetectTypeExt = cmd->u2DetectTypeExt;
	pos += sizeof(*ctrl_tag);

	gpio_tag = (struct UNI_CMD_SUSPEND_WOW_GPIO_PARAM *) pos;
	gpio_tag->u2Tag = UNI_CMD_SUSPEND_TAG_WOW_GPIO_PARAM;
	gpio_tag->u2Length = sizeof(*gpio_tag);
	gpio_tag->ucGpioPin = cmd->astWakeHif[0].ucGpioPin;
	gpio_tag->ucTriggerLvl = cmd->astWakeHif[0].ucTriggerLvl;
	gpio_tag->u4GpioInterval = cmd->astWakeHif[0].u4GpioInterval;
	pos += sizeof(*gpio_tag);

	port_tag = (struct UNI_CMD_SUSPEND_WOW_WAKEUP_PORT *) pos;
	port_tag->u2Tag = UNI_CMD_SUSPEND_TAG_WOW_WAKEUP_PORT;
	port_tag->u2Length = sizeof(*port_tag);
	port_tag->ucIPv4UdpPortCnt = cmd->stWowPort.ucIPv4UdpPortCnt;
	port_tag->ucIPv4TcpPortCnt = cmd->stWowPort.ucIPv4TcpPortCnt;
	port_tag->ucIPv6UdpPortCnt = cmd->stWowPort.ucIPv6UdpPortCnt;
	port_tag->ucIPv6TcpPortCnt = cmd->stWowPort.ucIPv6TcpPortCnt;
	kalMemCopy(port_tag->ausIPv4UdpPort, cmd->stWowPort.ausIPv4UdpPort,
		sizeof(port_tag->ausIPv4UdpPort));
	kalMemCopy(port_tag->ausIPv4TcpPort, cmd->stWowPort.ausIPv4TcpPort,
		sizeof(port_tag->ausIPv4TcpPort));
	kalMemCopy(port_tag->ausIPv6UdpPort, cmd->stWowPort.ausIPv6UdpPort,
		sizeof(port_tag->ausIPv6UdpPort));
	kalMemCopy(port_tag->ausIPv6TcpPort, cmd->stWowPort.ausIPv6TcpPort,
		sizeof(port_tag->ausIPv6TcpPort));
	pos += sizeof(*port_tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

void nicUniCmdNicCapability(struct ADAPTER *prAdapter)
{
	struct UNI_CMD_CHIP_CONFIG *uni_cmd;
	struct UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CHIP_CONFIG) +
	     		      sizeof(struct UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY);

	uni_cmd = (struct UNI_CMD_CHIP_CONFIG *)
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(NIC, ERROR, "uni_cmd is NULL\n");
		return;
	}

	tag = (struct UNI_CMD_CHIP_CONFIG_NIC_CAPABILITY *)
			uni_cmd->aucTlvBuffer;
	if (!tag) {
		DBGLOG(NIC, ERROR, "tag is NULL\n");
		return;
	}

	tag->u2Tag = UNI_CMD_CHIP_CONFIG_TAG_NIC_CAPABILITY;
	tag->u2Length = sizeof(*tag);

	wlanSendSetQueryUniCmdAdv(
		prAdapter, UNI_CMD_ID_CHIP_CONFIG, FALSE,
		TRUE, FALSE, NULL, NULL, max_cmd_len,
		(uint8_t *) uni_cmd, NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);

	cnmMemFree(prAdapter, uni_cmd);
}

uint32_t nicUniCmdEventQueryNicCapabilityV2(struct ADAPTER *ad,
				     struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_CHIP_CAPABILITY);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t u4Status = WLAN_STATUS_FAILURE;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return WLAN_STATUS_FAILURE;
	}

	if (data == NULL) {
		DBGLOG(NIC, ERROR, "Get NULL data\n");
		return WLAN_STATUS_FAILURE;
	}

	/* copy tag to legacy event */
	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		u4Status = nicParsingNicCapV2(ad, TAG_ID(tag), TAG_DATA(tag));
		if (u4Status != WLAN_STATUS_SUCCESS)
			DBGLOG_MEM8(NIC, TRACE, tag,
				TAG_HDR_LEN + TAG_LEN(tag));
	}

	if (tags_len != offset) {
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		return WLAN_STATUS_FAILURE;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanQueryNicCapabilityV2(struct ADAPTER *prAdapter)
{
	uint32_t u4RxPktLength;
	uint8_t *prEventBuff;
	struct WIFI_UNI_EVENT *prEvent;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t chip_id;
	uint32_t u4Time;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	chip_id = prChipInfo->chip_id;

	/* Get Nic resource information from FW */
	if (!prChipInfo->isNicCapV1
	    || (prAdapter->u4FwFeatureFlag0 &
		FEATURE_FLAG0_NIC_CAPABILITY_V2)) {

		DBGLOG(INIT, TRACE, "Support NIC_CAPABILITY_V2 feature\n");

		/* get nic capability */
		nicUniCmdNicCapability(prAdapter);

		/*
		 * receive UNI_EVENT_ID_CHIP_CONFIG
		 */
		/* allocate event buffer */
		prEventBuff = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
					  CFG_RX_MAX_PKT_SIZE);
		if (!prEventBuff) {
			DBGLOG(INIT, WARN, "event buffer alloc failed!\n");
			return WLAN_STATUS_FAILURE;
		}

		/* get event */
		u4Time = kalGetTimeTick();
		while (TRUE) {
			if (nicRxWaitResponse(prAdapter,
					      1,
					      prEventBuff,
					      CFG_RX_MAX_PKT_SIZE,
					      &u4RxPktLength)
			    != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN,
				       "wait for event failed!\n");

				/* free event buffer */
				cnmMemFree(prAdapter, prEventBuff);

				return WLAN_STATUS_FAILURE;
			}

			if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				MSEC_TO_SYSTIME(3000))) {
				DBGLOG(HAL, ERROR,
					"Query nic capability timeout\n");
				return WLAN_STATUS_FAILURE;
			}

			/* header checking .. */
			if ((NIC_RX_GET_U2_SW_PKT_TYPE(prEventBuff) &
				prChipInfo->u2RxSwPktBitMap) !=
				prChipInfo->u2RxSwPktEvent) {
				DBGLOG(INIT, WARN,
				       "skip unexpected Rx pkt type[0x%04x]\n",
				       NIC_RX_GET_U2_SW_PKT_TYPE(prEventBuff));

				continue;
			}

			prEvent = (struct WIFI_UNI_EVENT *)
				(prEventBuff + prChipInfo->rxd_size);

			if (!IS_UNI_EVENT(prEvent) ||
			    prEvent->ucEID != UNI_EVENT_ID_CHIP_CAPABILITY) {
				DBGLOG(INIT, WARN,
				       "skip unexpected event ID[0x%02x] option:%d\n",
				        prEvent->ucEID, GET_UNI_EVENT_OPTION(prEvent));

				continue;
			} else {
				/* hit */
				break;
			}

		}

		u4Status = nicUniCmdEventQueryNicCapabilityV2(prAdapter,
				prEvent);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "handle nic capability fail\n");
			DBGLOG_MEM8(INIT, ERROR, prEventBuff,
				CFG_RX_MAX_PKT_SIZE);
		}

		/*
		 * free event buffer
		 */
		cnmMemFree(prAdapter, prEventBuff);
	}

	/* Fill capability for different Chip version */
	if (chip_id == 0x6632) {
		/* 6632 only */
		prAdapter->fgIsSupportBufferBinSize16Byte = TRUE;
		prAdapter->fgIsSupportDelayCal = FALSE;
		prAdapter->fgIsSupportGetFreeEfuseBlockCount = FALSE;
		prAdapter->fgIsSupportQAAccessEfuse = FALSE;
		prAdapter->fgIsSupportPowerOnSendBufferModeCMD = FALSE;
		prAdapter->fgIsSupportGetTxPower = FALSE;
	} else {
		prAdapter->fgIsSupportBufferBinSize16Byte = FALSE;
		prAdapter->fgIsSupportDelayCal = TRUE;
		prAdapter->fgIsSupportGetFreeEfuseBlockCount = TRUE;
		prAdapter->fgIsSupportQAAccessEfuse = TRUE;
		prAdapter->fgIsSupportPowerOnSendBufferModeCMD = TRUE;
		prAdapter->fgIsSupportGetTxPower = TRUE;
	}

	return u4Status;
}

uint32_t nicUniCmdRemoveStaRec(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_REMOVE_STA_RECORD *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_REMOVE_INFO *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct UNI_CMD_STAREC_MLD_TEARDOWN *mld_teardown;
	struct STA_RECORD *sta;
	struct MLD_STA_RECORD *mld_sta;
#endif
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
	     		       sizeof(struct UNI_CMD_STAREC_REMOVE_INFO);
	uint32_t widx;

	if (info->ucCID != CMD_ID_REMOVE_STA_RECORD ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_REMOVE_STA_RECORD *) info->pucInfoBuffer;
	if (cmd->ucActionType == STA_REC_CMD_ACTION_STA ||
	    cmd->ucActionType == STA_REC_CMD_ACTION_BSS_EXCLUDE_STA) {
		if (cmd->ucStaIndex < CFG_STA_REC_NUM &&
		    ad->arStaRec[cmd->ucStaIndex].fgIsInUse)
			widx = ad->arStaRec[cmd->ucStaIndex].ucWlanIndex;
		else
			return WLAN_STATUS_INVALID_DATA;
	} else {
		/* remove by bss, widx is not checked */
		widx = UNI_CMD_STAREC_INVALID_WTBL_IDX;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	sta = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);
	mld_sta = mldStarecGetByStarec(ad, sta);

	if (mld_sta)
		max_cmd_len += sizeof(struct UNI_CMD_STAREC_MLD_TEARDOWN);
#endif

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);

	tag = (struct UNI_CMD_STAREC_REMOVE_INFO *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_REMOVE;
	tag->u2Length = sizeof(*tag);
	tag->ucActionType = cmd->ucActionType;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	if (mld_sta) {
		mld_teardown = (struct UNI_CMD_STAREC_MLD_TEARDOWN *)
				(uni_cmd->aucTlvBuffer + sizeof(*tag));
		mld_teardown->u2Tag = UNI_CMD_STAREC_TAG_MLD_TEARDOWN;
		mld_teardown->u2Length = sizeof(*mld_teardown);

		DBGLOG(INIT, DEBUG,
			"Teardown mld_sta(%d) bss(%d) sta(%d)\n",
			mld_sta->ucIdx,
			sta->ucBssIndex,
			sta->ucIndex);
	}
#endif

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdBcnContent(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_BEACON_TEMPLATE_UPDATE *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_BCN_CONTENT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
	     		       sizeof(struct UNI_CMD_BSSINFO_BCN_CONTENT);

	if (info->ucCID != CMD_ID_UPDATE_BEACON_CONTENT)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_BEACON_TEMPLATE_UPDATE *) info->pucInfoBuffer;
	/* 2 for u2CapInfo */
	max_cmd_len = ALIGN_4(max_cmd_len + 2 + cmd->u2IELen);
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_BSSINFO_BCN_CONTENT *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_BCN_CONTENT;
	tag->u2Length = ALIGN_4(sizeof(*tag) + 2 + cmd->u2IELen);
	if (cmd->ucUpdateMethod == IE_UPD_METHOD_UPDATE_PROBE_RSP)
		tag->ucAction = UPDATE_PROBE_RSP;
	else if (cmd->ucUpdateMethod == IE_UPD_METHOD_DELETE_ALL)
		tag->ucAction = BCN_ACTION_DISABLE;
	else if (cmd->ucUpdateMethod == IE_UPD_METHOD_UNSOL_PROBE_RSP)
		tag->ucAction = UPDATE_UNSOL_PROBE_RSP;
	else
		tag->ucAction = BCN_ACTION_ENABLE;
	tag->u2PktLength = 2 + cmd->u2IELen;
	/* the aucPktContent field only include
	 * capablity field and followed IEs
	 */
	tag->aucPktContentType = 1;
	kalMemCopy(tag->aucPktContent, &cmd->u2Capability, 2);
	kalMemCopy(tag->aucPktContent + 2, cmd->aucIE, cmd->u2IELen);

	DBGLOG(INIT, DEBUG, "Bss=%d, Action=%d, PktLen=%d\n",
		cmd->ucBssIndex,
		tag->ucAction,
		tag->u2PktLength);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdFilsDiscovery(struct ADAPTER *ad,
		uint8_t bss_idx, uint32_t max_interval,
		uint32_t min_interval, uint8_t *ie, uint16_t ie_len)
{
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_FILS_REQ *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
			       sizeof(struct UNI_CMD_BSSINFO_FILS_REQ);
	uint32_t status = WLAN_STATUS_SUCCESS;

	max_cmd_len = ALIGN_4(max_cmd_len + ie_len);
	uni_cmd = (struct UNI_CMD_BSSINFO *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BSSINFO ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd->ucBssInfoIdx = bss_idx;
	tag = (struct UNI_CMD_BSSINFO_FILS_REQ *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_FILS_DISCOVERY;
	tag->u2Length = ALIGN_4(sizeof(*tag) + ie_len);
	tag->u4MinInterval = min_interval;
	tag->u4MaxInterval = max_interval;
	tag->u2PktLength = ie_len;
	kalMemCopy(tag->aucPktContent, ie, ie_len);

	DBGLOG(INIT, DEBUG, "bss=%d, min=%d, max=%d, ie_len=%d\n",
		bss_idx, min_interval, max_interval, ie_len);

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_BSSINFO,
					TRUE,
					FALSE,
					FALSE,
					nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					NULL,
					0);

	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdPmDisable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_INDICATE_PM_BSS_ABORT *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_STA_PM_DISABLE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
	     		       sizeof(struct UNI_CMD_BSSINFO_STA_PM_DISABLE);

	if (info->ucCID != CMD_ID_INDICATE_PM_BSS_ABORT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_INDICATE_PM_BSS_ABORT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_BSSINFO_STA_PM_DISABLE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_PM_DISABLE;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdPmEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_INDICATE_PM_BSS_CONNECTED *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_STA_PM_ENABLE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
	     		       sizeof(struct UNI_CMD_BSSINFO_STA_PM_ENABLE);

	if (info->ucCID != CMD_ID_INDICATE_PM_BSS_CONNECTED ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_INDICATE_PM_BSS_CONNECTED *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_BSSINFO_STA_PM_ENABLE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_PM_ENABLE;
	tag->u2Length = sizeof(*tag);
	tag->u2BcnInterval = cmd->u2BeaconInterval;
	tag->ucDtimPeriod = cmd->ucDtimPeriod;
	tag->ucBmpDeliveryAC = cmd->ucBmpDeliveryAC;
	tag->ucBmpTriggerAC = cmd->ucBmpTriggerAC;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdBssInfoTagBasic(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_BASIC *tag =
		(struct UNI_CMD_BSSINFO_BASIC *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);
	uint32_t phy_mode;

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->ucActive = IS_BSS_ACTIVE(bss);
	tag->ucOwnMacIdx = bss->ucOwnMacIndex;
	tag->ucHwBSSIndex = bss->ucOwnMacIndex;
	tag->ucDbdcIdx = cmd->ucDBDCBand;
	tag->u4ConnectionType = bssInfoConnType(ad, bss);
	tag->ucConnectionState = bss->eConnectionState;
	tag->ucWmmIdx = cmd->ucWmmSet;
	COPY_MAC_ADDR(tag->aucBSSID, cmd->aucBSSID);
	tag->u2BcMcWlanidx = cmd->ucBMCWlanIndex;
	tag->u2BcnInterval = bss->u2BeaconInterval;
	tag->ucDtimPeriod = bss->ucDTIMPeriod;
	tag->u2StaRecIdxOfAP = secGetWlanIdxByStaIdx(ad, cmd->ucStaRecIdxOfAP);
	tag->u2NonHTBasicPhyType = cmd->ucNonHTBasicPhyType;
	phy_mode = nicUniCmdBssInfoPhyMode(cmd->ucPhyTypeSet);
	tag->ucPhyMode = phy_mode & 0xff;
	tag->ucPhyModeExt = (phy_mode >> 8) & 0xff;

	return tag->u2Length;
}

uint32_t nicUniCmdSetBssRlmImpl(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_RLM_PARAM *cmd)
{
	struct UNI_CMD_BSSINFO_RLM *rlm_tag;
	struct UNI_CMD_BSSINFO_PROTECT *prot_tag;
	struct UNI_CMD_BSSINFO_IFS_TIME *ifs_tag;
	uint8_t *pos = buf;

	rlm_tag = (struct UNI_CMD_BSSINFO_RLM *) pos;
	rlm_tag->u2Tag = UNI_CMD_BSSINFO_TAG_RLM;
	rlm_tag->u2Length = sizeof(*rlm_tag);
	rlm_tag->ucPrimaryChannel = cmd->ucPrimaryChannel;
	rlm_tag->ucCenterChannelSeg0 = cmd->ucVhtChannelFrequencyS1;
	rlm_tag->ucCenterChannelSeg1 = cmd->ucVhtChannelFrequencyS2;
	rlm_tag->ucTxStream = cmd->ucTxNss;
	rlm_tag->ucRxStream = cmd->ucRxNss;
	rlm_tag->ucHtOpInfo1 = cmd->ucHtOpInfo1;
	rlm_tag->ucSCO = cmd->ucRfSco;
	rlm_tag->ucRfBand = cmd->ucRfBand;
	rlm_tag->ucBandwidth = BW_20;
	switch (cmd->ucVhtChannelWidth) {
	case VHT_OP_CHANNEL_WIDTH_320_1:
	case VHT_OP_CHANNEL_WIDTH_320_2:
		rlm_tag->ucBandwidth = BW_320;
		break;
	case VHT_OP_CHANNEL_WIDTH_80P80:
		rlm_tag->ucBandwidth = BW_8080;
		break;
	case VHT_OP_CHANNEL_WIDTH_160:
		rlm_tag->ucBandwidth = BW_160;
		break;
	case VHT_OP_CHANNEL_WIDTH_80:
		rlm_tag->ucBandwidth = BW_80;
		break;
	case VHT_OP_CHANNEL_WIDTH_20_40:
		if (cmd->ucRfSco != CHNL_EXT_SCN)
			rlm_tag->ucBandwidth = BW_40;
		break;
	default:
		break;
	}
	pos += sizeof(*rlm_tag);

	prot_tag = (struct UNI_CMD_BSSINFO_PROTECT *) pos;
	prot_tag->u2Tag = UNI_CMD_BSSINFO_TAG_PROTECT;
	prot_tag->u2Length = sizeof(*prot_tag);
	if (cmd->ucErpProtectMode)
		prot_tag->u4ProtectMode |= LEGACY_ERP_PROTECT;
	switch (cmd->ucHtProtectMode) {
	case HT_PROTECT_MODE_NON_MEMBER:
		prot_tag->u4ProtectMode |= HT_NON_MEMBER_PROTECT;
		break;
	case HT_PROTECT_MODE_20M:
		prot_tag->u4ProtectMode |= HT_BW20_PROTECT;
		break;
	case HT_PROTECT_MODE_NON_HT:
		prot_tag->u4ProtectMode |= HT_NON_HT_MIXMODE_PROTECT;
		break;
	default:
		break;
	}
	if (cmd->ucGfOperationMode == GF_MODE_PROTECT)
		prot_tag->u4ProtectMode |= VEND_GREEN_FIELD_PROTECT;
	pos += sizeof(*prot_tag);

	ifs_tag = (struct UNI_CMD_BSSINFO_IFS_TIME *) pos;
	ifs_tag->u2Tag = UNI_CMD_BSSINFO_TAG_IFS_TIME;
	ifs_tag->u2Length = sizeof(*ifs_tag);
	ifs_tag->fgSlotValid = TRUE;
	if (cmd->ucUseShortSlotTime)
		ifs_tag->u2SlotTime = 9;
	else
		ifs_tag->u2SlotTime = 20;
	pos += sizeof(*ifs_tag);

	return pos - buf;
}

uint32_t nicUniCmdBssInfoTagRlm(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	return nicUniCmdSetBssRlmImpl(ad, buf, &cmd->rBssRlmParam);
}

uint32_t nicUniCmdBssInfoTagRate(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_RATE *tag = (struct UNI_CMD_BSSINFO_RATE *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_RATE;
	tag->u2Length = sizeof(*tag);
	tag->u2OperationalRateSet = cmd->u2OperationalRateSet;
	tag->u2BSSBasicRateSet = cmd->u2BSSBasicRateSet;

	return tag->u2Length;
}

uint32_t nicUniCmdBssInfoTagSec(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_SEC *tag = (struct UNI_CMD_BSSINFO_SEC *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_SEC;
	tag->u2Length = sizeof(*tag);
	tag->ucAuthMode = cmd->ucAuthMode;
	tag->ucEncStatus = cmd->ucEncStatus;

	return tag->u2Length;
}

uint32_t nicUniCmdBssInfoTagQbss(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_QBSS *tag = (struct UNI_CMD_BSSINFO_QBSS *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_QBSS;
	tag->u2Length = sizeof(*tag);
	tag->ucIsQBSS = cmd->ucIsQBSS;
	return tag->u2Length;
}
#if CFG_ENABLE_WIFI_DIRECT
uint32_t nicUniCmdBssInfoTagSap(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_SAP *tag = (struct UNI_CMD_BSSINFO_SAP *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_SAP;
	tag->u2Length = sizeof(*tag);
	tag->fgIsHiddenSSID = cmd->ucHiddenSsidMode != ENUM_HIDDEN_SSID_NONE;
	COPY_SSID(tag->aucSSID, tag->ucSSIDLen, cmd->aucSSID, cmd->ucSSIDLen);

	return tag->u2Length;
}

uint32_t nicUniCmdBssInfoTagP2p(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_P2P *tag = (struct UNI_CMD_BSSINFO_P2P *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_P2P;
	tag->u2Length = sizeof(*tag);
	tag->u4PrivateData = cmd->u4PrivateData;

	return tag->u2Length;
}
#endif

#if (CFG_SUPPORT_802_11AX == 1)
uint32_t nicUniCmdBssInfoTagHe(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_HE *tag = (struct UNI_CMD_BSSINFO_HE *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_HE;
	tag->u2Length = sizeof(*tag);
	tag->u2TxopDurationRtsThreshold =
		((bss->ucHeOpParams[0] &
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_MASK) >>
			HE_OP_PARAM0_TXOP_DUR_RTS_THRESHOLD_SHFT) |
		(((bss->ucHeOpParams[1] &
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_MASK) >>
			HE_OP_PARAM1_TXOP_DUR_RTS_THRESHOLD_SHFT) << 4);
	tag->au2MaxNssMcs[0] = CPU_TO_LE16(bss->u2HeBasicMcsSet);
	tag->au2MaxNssMcs[1] = CPU_TO_LE16(bss->u2HeBasicMcsSet);
	tag->au2MaxNssMcs[2] = CPU_TO_LE16(bss->u2HeBasicMcsSet);
	tag->ucDefaultPEDuration = ((bss->ucHeOpParams[0] &
		HE_OP_PARAM0_DEFAULT_PE_DUR_MASK) >>
		HE_OP_PARAM0_DEFAULT_PE_DUR_SHFT);
	tag->fgErSuDisable = (bss->ucHeOpParams[2] &
		(1 << HE_OP_PARAM2_ER_SU_DISABLE_SHFT)) ? TRUE : FALSE;
	tag->uc6GPwrRegBitMap = cmd->uc6GPwrRegBitMap;

	return tag->u2Length;
}

uint32_t nicUniCmdBssInfoTagBssColor(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_BSS_COLOR *tag =
		(struct UNI_CMD_BSSINFO_BSS_COLOR *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_BSS_COLOR;
	tag->u2Length = sizeof(*tag);
	tag->fgEnable =	(bss->ucBssColorInfo & HE_OP_BSSCOLOR_BSS_COLOR_DISABLE)
			? FALSE : TRUE;
	tag->ucBssColor = ((bss->ucBssColorInfo & HE_OP_BSSCOLOR_BSS_COLOR_MASK)
			>> HE_OP_BSSCOLOR_BSS_COLOR_SHFT);
	return tag->u2Length;
}

#endif

#if (CFG_SUPPORT_802_11BE == 1)
uint32_t nicUniCmdBssInfoTagEht(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_EHT *tag = (struct UNI_CMD_BSSINFO_EHT *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_EHT;
	tag->u2Length = sizeof(*tag);
	tag->fgIsEhtOpPresent = bss->fgIsEhtOpPresent;
	tag->fgIsEhtDscbPresent =
		bss->fgIsEhtDscbPresent;
	tag->ucEhtCtrl = bss->ucEhtCtrl;
	tag->ucEhtCcfs0 = bss->ucEhtCcfs0;
	tag->ucEhtCcfs1 = bss->ucEhtCcfs1;
	tag->u2EhtDisSubChanBitmap =
		bss->u2EhtDisSubChanBitmap;
	tag->fgEhtMcs15Disable = bss->fgEhtMcs15Disable;

	return tag->u2Length;
}
#endif /* #if (CFG_SUPPORT_802_11BE== 1) */

uint32_t nicUniCmdBssInfoTagMBSSID(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_11V_MBSSID *tag =
		(struct UNI_CMD_BSSINFO_11V_MBSSID *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_11V_MBSSID;
	tag->u2Length = sizeof(*tag);
	tag->ucMaxBSSIDIndicator = cmd->ucMaxBSSIDIndicator;
	tag->ucMBSSIDIndex = cmd->ucMBSSIDIndex;

	DBGLOG(INIT, TRACE, "MBSS: MaxBSSIDIndicator=%d, MBSSIDIndex=%d\n",
		tag->ucMaxBSSIDIndicator, tag->ucMBSSIDIndex);

	return tag->u2Length;
}

uint32_t nicUniCmdBssInfoTagWapi(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_WAPI *tag = (struct UNI_CMD_BSSINFO_WAPI *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_WAPI;
	tag->u2Length = sizeof(*tag);
	tag->fgWapiMode = cmd->ucWapiMode;

	return tag->u2Length;
}

#if CFG_SUPPORT_IOT_AP_BLOCKLIST
uint32_t nicUniCmdBssInfoTagStaIot(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_IOT *tag = (struct UNI_CMD_BSSINFO_IOT *)buf;

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_STA_IOT;
	tag->u2Length = sizeof(*tag);
	tag->ucIotApAct = cmd->ucIotApAct;
	tag->u8IotApBmp = cmd->u8IotApAct;
	DBGLOG(NIC, DEBUG, "v0:v1 %u:%u\n", tag->ucIotApAct, tag->u8IotApBmp);
	return tag->u2Length;
}
#endif

uint32_t nicUniCmdBssInfoTagMld(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	return nicUniCmdBssInfoMld(ad, buf, cmd->ucBssIndex);
}

uint32_t nicUniCmdBssInfoTagMaxIdlePeriod(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_SET_BSS_INFO *cmd)
{
	struct UNI_CMD_BSSINFO_MAX_IDLE_PERIOD *tag =
		(struct UNI_CMD_BSSINFO_MAX_IDLE_PERIOD *)buf;
	struct BSS_INFO *bss = GET_BSS_INFO_BY_INDEX(ad, cmd->ucBssIndex);

	if (!bss) {
		DBGLOG(INIT, WARN, "Bss=%d not found", cmd->ucBssIndex);
		return 0;
	}

	tag->u2Tag = UNI_CMD_BSSINFO_TAG_MAX_IDLE_PERIOD;
	tag->u2Length = sizeof(*tag);
	tag->u2MaxIdlePeriod = bss->u2MaxIdlePeriod;
	tag->ucIdleOptions = bss->ucIdleOptions;

	DBGLOG(INIT, DEBUG, "Bss=%d, MaxIdlePeriod=%d, IdleOptions=%d\n",
		bss->ucBssIndex,
		tag->u2MaxIdlePeriod,
		tag->ucIdleOptions);

	return tag->u2Length;
}

struct UNI_CMD_BSSINFO_TAG_HANDLE arSetBssInfoTable[] = {
	{sizeof(struct UNI_CMD_BSSINFO_BASIC), nicUniCmdBssInfoTagBasic},
	{sizeof(struct UNI_CMD_BSSINFO_RLM) +
	 sizeof(struct UNI_CMD_BSSINFO_PROTECT)	+
	 sizeof(struct UNI_CMD_BSSINFO_IFS_TIME), nicUniCmdBssInfoTagRlm},
	{sizeof(struct UNI_CMD_BSSINFO_RATE), nicUniCmdBssInfoTagRate},
	{sizeof(struct UNI_CMD_BSSINFO_SEC), nicUniCmdBssInfoTagSec},
	{sizeof(struct UNI_CMD_BSSINFO_QBSS), nicUniCmdBssInfoTagQbss},
#if CFG_ENABLE_WIFI_DIRECT
	{sizeof(struct UNI_CMD_BSSINFO_SAP), nicUniCmdBssInfoTagSap},
	{sizeof(struct UNI_CMD_BSSINFO_P2P), nicUniCmdBssInfoTagP2p},
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	{sizeof(struct UNI_CMD_BSSINFO_HE), nicUniCmdBssInfoTagHe},
	{sizeof(struct UNI_CMD_BSSINFO_BSS_COLOR), nicUniCmdBssInfoTagBssColor},
#endif

#if (CFG_SUPPORT_802_11BE == 1)
	{sizeof(struct UNI_CMD_BSSINFO_EHT),
					nicUniCmdBssInfoTagEht},
#endif
	{sizeof(struct UNI_CMD_BSSINFO_11V_MBSSID), nicUniCmdBssInfoTagMBSSID},
	{sizeof(struct UNI_CMD_BSSINFO_WAPI), nicUniCmdBssInfoTagWapi},
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
	{sizeof(struct UNI_CMD_BSSINFO_IOT), nicUniCmdBssInfoTagStaIot},
#endif
	{sizeof(struct UNI_CMD_BSSINFO_MLD), nicUniCmdBssInfoTagMld},
#if (CFG_SUPPORT_BSS_MAX_IDLE_PERIOD == 1)
	{sizeof(struct UNI_CMD_BSSINFO_MAX_IDLE_PERIOD),
					nicUniCmdBssInfoTagMaxIdlePeriod}
#endif
};

uint32_t nicUniCmdSetBssInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_BSS_INFO *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t *pos;
	uint32_t max_cmd_len = 0;
	int i;

	if (info->ucCID != CMD_ID_SET_BSS_INFO ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_BSS_INFO *) info->pucInfoBuffer;
	max_cmd_len += sizeof(struct UNI_CMD_BSSINFO);
	for (i = 0; i < ARRAY_SIZE(arSetBssInfoTable); i++)
		max_cmd_len += arSetBssInfoTable[i].u4Size;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	pos = entry->pucInfoBuffer;
	uni_cmd = (struct UNI_CMD_BSSINFO *) pos;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	pos += sizeof(*uni_cmd);
	for (i = 0; i < ARRAY_SIZE(arSetBssInfoTable); i++)
		pos += arSetBssInfoTable[i].pfHandler(ad, pos, cmd);
	entry->u4SetQueryInfoLen = pos - entry->pucInfoBuffer;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdStaRecTagFastAll(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_BSS_RLM_PARAM *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_FASTALL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
				   sizeof(struct UNI_CMD_STAREC_FASTALL);

	if (info->ucCID != CMD_ID_SET_BSS_RLM_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_BSS_RLM_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad,
		UNI_CMD_ID_STAREC_INFO, max_cmd_len,
		NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer,
		uni_cmd->ucWlanIdxL, UNI_CMD_STAREC_INVALID_WTBL_IDX);
	tag = (struct UNI_CMD_STAREC_FASTALL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_FAST_ALL;
	tag->u2Length = sizeof(struct UNI_CMD_STAREC_FASTALL);
	tag->ucUpdateFlag = UNI_CMD_STAREC_FASTALL_FLAG_UPDATE_BAND;

	DBGLOG(INIT, DEBUG, "bss=%d\n",
		cmd->ucBssIndex);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetBssRlm(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_BSS_RLM_PARAM *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
	     		       sizeof(struct UNI_CMD_BSSINFO_RLM) +
			       sizeof(struct UNI_CMD_BSSINFO_PROTECT) +
			       sizeof(struct UNI_CMD_BSSINFO_IFS_TIME);

	if (info->ucCID != CMD_ID_SET_BSS_RLM_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_BSS_RLM_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO, max_cmd_len,
			NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	nicUniCmdSetBssRlmImpl(ad, uni_cmd->aucTlvBuffer, cmd);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdBcnProt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_802_11_KEY *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_BCN_PROT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
			       sizeof(struct UNI_CMD_BSSINFO_BCN_PROT);
	struct PARAM_KEY *key;

	if (info->ucCID != CMD_ID_ADD_REMOVE_KEY ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_802_11_KEY *) info->pucInfoBuffer;
	if (cmd->ucKeyId < 6 || cmd->ucKeyId > 7)
		return WLAN_STATUS_NOT_ACCEPTED;

	key = (struct PARAM_KEY *) info->pvSetQueryBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len,
			nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIdx;
	tag = (struct UNI_CMD_BSSINFO_BCN_PROT *)uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_BCN_PROT;
	tag->u2Length = sizeof(*tag);

	if (cmd->ucAddRemove == 0) { /* remove BIGTK */
		tag->ucBcnProtEnabled = 0;
	} else if (cmd->ucAddRemove == 1) { /* add BIGTK */
		kalMemCopy(tag->aucBcnProtPN, key->aucKeyPn,
			sizeof(tag->aucBcnProtPN));
		if (cmd->ucAlgorithmId == CIPHER_SUITE_BIP_CMAC_128) {
			tag->ucBcnProtCipherId = CIPHER_SUITE_BCN_PROT_CMAC_128;
			tag->ucBcnProtEnabled = 2; /* HW mode */
		} else if (cmd->ucAlgorithmId == CIPHER_SUITE_BIP_CMAC_256) {
			tag->ucBcnProtCipherId = CIPHER_SUITE_BCN_PROT_CMAC_256;
			tag->ucBcnProtEnabled = 2; /* HW mode */
		} else if (cmd->ucAlgorithmId == CIPHER_SUITE_BIP_GMAC_256) {
			tag->ucBcnProtCipherId = CIPHER_SUITE_BCN_PROT_GMAC_256;
#if CFG_SUPPORT_SW_BIP_GMAC
			tag->ucBcnProtEnabled = 1; /* SW mode */
#else
			tag->ucBcnProtEnabled = 2; /* HW mode */
#endif
		} else {
			DBGLOG(INIT, DEBUG,
				"unsupported cipher for BCN PROT: %d",
				cmd->ucAlgorithmId);
		}
		kalMemCopy(tag->aucBcnProtKey, cmd->aucKeyMaterial,
			sizeof(tag->aucBcnProtKey));
	}
	tag->ucBcnProtKeyId = cmd->ucKeyId;
	tag->ucBmcWlanIndex = cmd->ucWlanIndex;
	DBGLOG(INIT, DEBUG,
		"%s BIGTK Bss=%d, ucBcnProtEnabled=%d, ucBcnProtCipherId=%d, ucBcnProtKeyId=%d, wlanidx=%d\n",
		(cmd->ucAddRemove ? "Add" : "Remove"),
		uni_cmd->ucBssInfoIdx,
		tag->ucBcnProtEnabled,
		tag->ucBcnProtCipherId,
		tag->ucBcnProtKeyId,
		tag->ucBmcWlanIndex);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdPowerSaveMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_PS_PROFILE *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_POWER_SAVE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
	     		       sizeof(struct UNI_CMD_BSSINFO_POWER_SAVE);

	if (info->ucCID != CMD_ID_POWER_SAVE_MODE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_PS_PROFILE *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO, max_cmd_len,
			info->fgIsOid ? nicUniCmdEventSetCommon : NULL,
			info->fgIsOid ? nicUniCmdTimeoutCommon : NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_BSSINFO_POWER_SAVE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BSSINFO_TAG_POWER_SAVE;
	tag->u2Length = sizeof(*tag);
	tag->ucPsProfile = cmd->ucPsProfile;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetWmmPsTestParams(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_WMM_PS_TEST_STRUCT *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_UAPSD *uapsd;
	struct UNI_CMD_BSSINFO_WMM_PS_TEST *wmm;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
			       sizeof(struct UNI_CMD_BSSINFO_UAPSD) +
	     		       sizeof(struct UNI_CMD_BSSINFO_WMM_PS_TEST);

	if (info->ucCID != CMD_ID_SET_WMM_PS_TEST_PARMS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_WMM_PS_TEST_STRUCT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;

	uapsd = (struct UNI_CMD_BSSINFO_UAPSD *) uni_cmd->aucTlvBuffer;
	uapsd->u2Tag = UNI_CMD_BSSINFO_TAG_UAPSD;
	uapsd->u2Length = sizeof(*uapsd);
	uapsd->ucBmpDeliveryAC = (cmd->bmfgApsdEnAc & BITS(4, 7)) >> 4;
	uapsd->ucBmpTriggerAC = cmd->bmfgApsdEnAc & BITS(0, 3);

	wmm = (struct UNI_CMD_BSSINFO_WMM_PS_TEST *)
		(uni_cmd->aucTlvBuffer + sizeof(*uapsd));
	wmm->u2Tag = UNI_CMD_BSSINFO_TAG_WMM_PS_TEST;
	wmm->u2Length = sizeof(*wmm);
	wmm->ucIsEnterPsAtOnce = cmd->ucIsEnterPsAtOnce;
	wmm->ucIsDisableUcTrigger = cmd->ucIsDisableUcTrigger;

	DBGLOG(INIT, DEBUG, "DeAC=%x BmpTrigAC=%x Once=%d DisUcTrig=%d\n",
		uapsd->ucBmpDeliveryAC,
		uapsd->ucBmpTriggerAC,
		wmm->ucIsEnterPsAtOnce,
		wmm->ucIsDisableUcTrigger);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetUapsd(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_CUSTOM_UAPSD_PARAM_STRUCT *cmd;
	struct UNI_CMD_BSSINFO *uni_cmd;
	struct UNI_CMD_BSSINFO_UAPSD *uapsd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
			       sizeof(struct UNI_CMD_BSSINFO_UAPSD);
	struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *param;

	if (info->ucCID != CMD_ID_SET_UAPSD_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CUSTOM_UAPSD_PARAM_STRUCT *) info->pucInfoBuffer;
	param = (struct PARAM_CUSTOM_UAPSD_PARAM_STRUCT *)
							info->pvSetQueryBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BSSINFO, max_cmd_len,
			nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BSSINFO *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = param->ucBssIdx;

	uapsd = (struct UNI_CMD_BSSINFO_UAPSD *) uni_cmd->aucTlvBuffer;
	uapsd->u2Tag = UNI_CMD_BSSINFO_TAG_UAPSD;
	uapsd->u2Length = sizeof(*uapsd);
	uapsd->ucBmpDeliveryAC =
		((cmd->fgEnAPSD_AcBe << 0) |
		 (cmd->fgEnAPSD_AcBk << 1) |
		 (cmd->fgEnAPSD_AcVi << 2) |
		 (cmd->fgEnAPSD_AcVo << 3));
	uapsd->ucBmpTriggerAC =
		((cmd->fgEnAPSD_AcBe << 0) |
		 (cmd->fgEnAPSD_AcBk << 1) |
		 (cmd->fgEnAPSD_AcVi << 2) |
		 (cmd->fgEnAPSD_AcVo << 3));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdTwtArgtUpdate(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_TWT == 1)
	struct _EXT_CMD_TWT_ARGT_UPDATE_T *cmd;
	struct UNI_CMD_TWT *uni_cmd;
	struct UNI_CMD_TWT_ARGT_UPDATE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TWT) +
	     		       sizeof(struct UNI_CMD_TWT_ARGT_UPDATE);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_TWT_AGRT_UPDATE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _EXT_CMD_TWT_ARGT_UPDATE_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TWT, max_cmd_len,
			NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TWT *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_TWT_ARGT_UPDATE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TWT_TAG_AGRT_UPDATE;
	tag->u2Length = sizeof(*tag);
	tag->ucAgrtTblIdx = cmd->ucAgrtTblIdx;
	tag->ucAgrtCtrlFlag = cmd->ucAgrtCtrlFlag;
	tag->ucBssIndex = cmd->ucBssIndex;
	tag->ucOwnMacId = cmd->ucOwnMacId;
	tag->ucFlowId = cmd->ucFlowId;
	tag->u2PeerIdGrpId = cmd->u2PeerIdGrpId;
	tag->ucAgrtSpDuration = cmd->ucAgrtSpDuration;
	tag->u4AgrtSpStartTsf_low = cmd->u4AgrtSpStartTsfLow;
	tag->u4AgrtSpStartTsf_high = cmd->u4AgrtSpStartTsfHigh;
	tag->u2AgrtSpWakeIntvlMantissa = cmd->u2AgrtSpWakeIntvlMantissa;
	tag->ucAgrtSpWakeIntvlExponent = cmd->ucAgrtSpWakeIntvlExponent;
	tag->fgIsRoleAp = cmd->ucIsRoleAp;
	tag->ucAgrtParaBitmap = cmd->ucAgrtParaBitmap;
	tag->ucReserved_a = cmd->ucReserved_a;
	tag->u2Reserved_b = cmd->u2Reserved_b;
	tag->ucGrpMemberCnt = cmd->ucGrpMemberCnt;
	tag->ucReserved_c = cmd->ucReserved_c;
	tag->u2Reserved_d = cmd->u2Reserved_d;
#if 0  /* No need for STA*/
	kalMemCopy(tag->au2StaList, cmd->au2StaList, sizeof(tag->au2StaList));
#endif

#if (CFG_SUPPORT_RTWT == 1)
	/* DW7 RTWT traffic info */
	tag->ucTrafficInfoPresent = cmd->ucTrafficInfoPresent;
	tag->ucDlUlBmpValid = cmd->ucDlUlBmpValid;
	tag->ucDlBmp = cmd->ucDlBmp;
	tag->ucUlBmp = cmd->ucUlBmp;
#endif

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdStaRecUpdateExt(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_STAREC_UPDATE *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC);
	uint16_t widx = 0;

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_STAREC_UPDATE)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_STAREC_UPDATE *) info->pucInfoBuffer;
	switch (TAG_ID(cmd->aucBuffer)) {
	case STA_REC_BASIC:
		max_cmd_len += sizeof(struct UNI_CMD_STAREC_BASIC);
		break;
	case STA_REC_BF:
		max_cmd_len += sizeof(struct UNI_CMD_STAREC_BF);
		break;
	case STA_REC_MAUNAL_ASSOC:
		max_cmd_len += sizeof(struct UNI_CMD_STAREC_MANUAL_ASSOC);
		break;
	default:
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO, max_cmd_len,
			nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	widx = (uint16_t) cmd->ucWlanIdx;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL,
		widx);

	switch (TAG_ID(cmd->aucBuffer)) {
	case STA_REC_BASIC: {
		struct STAREC_COMMON *cmn;
		struct UNI_CMD_STAREC_BASIC *tag;

		cmn = (struct STAREC_COMMON *) cmd->aucBuffer;
		tag = (struct UNI_CMD_STAREC_BASIC *) uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_STAREC_TAG_BASIC;
		tag->u2Length = sizeof(*tag);
		COPY_MAC_ADDR(tag->aucPeerMacAddr, cmn->aucPeerMacAddr);
		tag->u4ConnectionType = cmn->u4ConnectionType;
		tag->ucConnectionState = cmn->ucConnectionState;
		tag->ucIsQBSS = cmn->ucIsQBSS;
		tag->u2AID = cmn->u2AID;
		tag->u2ExtraInfo = cmn->u2ExtraInfo;
	}
		break;
#if CFG_SUPPORT_TX_BF
	case STA_REC_BF: {
		struct CMD_STAREC_BF *bf;
		struct UNI_CMD_STAREC_BF *tag;

		bf = (struct CMD_STAREC_BF *) cmd->aucBuffer;
		tag = (struct UNI_CMD_STAREC_BF *) uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_STAREC_TAG_BF;
		tag->u2Length = sizeof(*tag);
		tag->rTxBfPfmuInfo.u2PfmuId = bf->rTxBfPfmuInfo.u2PfmuId;
		tag->rTxBfPfmuInfo.fgSU_MU = bf->rTxBfPfmuInfo.fgSU_MU;
		tag->rTxBfPfmuInfo.u1TxBfCap = bf->rTxBfPfmuInfo.u1TxBfCap;
		tag->rTxBfPfmuInfo.ucSoundingPhy =
			bf->rTxBfPfmuInfo.ucSoundingPhy;
		tag->rTxBfPfmuInfo.ucNdpaRate = bf->rTxBfPfmuInfo.ucNdpaRate;
		tag->rTxBfPfmuInfo.ucNdpRate = bf->rTxBfPfmuInfo.ucNdpRate;
		tag->rTxBfPfmuInfo.ucReptPollRate =
			bf->rTxBfPfmuInfo.ucReptPollRate;
		tag->rTxBfPfmuInfo.ucTxMode = bf->rTxBfPfmuInfo.ucTxMode;
		tag->rTxBfPfmuInfo.ucNc = bf->rTxBfPfmuInfo.ucNc;
		tag->rTxBfPfmuInfo.ucNr = bf->rTxBfPfmuInfo.ucNr;
		tag->rTxBfPfmuInfo.ucCBW = bf->rTxBfPfmuInfo.ucCBW;
		tag->rTxBfPfmuInfo.ucTotMemRequire =
			bf->rTxBfPfmuInfo.ucTotMemRequire;
		tag->rTxBfPfmuInfo.ucMemRequire20M =
			bf->rTxBfPfmuInfo.ucMemRequire20M;
		tag->rTxBfPfmuInfo.ucMemRow0 = bf->rTxBfPfmuInfo.ucMemRow0;
		tag->rTxBfPfmuInfo.ucMemCol0 = bf->rTxBfPfmuInfo.ucMemCol0;
		tag->rTxBfPfmuInfo.ucMemRow1 = bf->rTxBfPfmuInfo.ucMemRow1;
		tag->rTxBfPfmuInfo.ucMemCol1 = bf->rTxBfPfmuInfo.ucMemCol1;
		tag->rTxBfPfmuInfo.ucMemRow2 = bf->rTxBfPfmuInfo.ucMemRow2;
		tag->rTxBfPfmuInfo.ucMemCol2 = bf->rTxBfPfmuInfo.ucMemCol2;
		tag->rTxBfPfmuInfo.ucMemRow3 = bf->rTxBfPfmuInfo.ucMemRow3;
		tag->rTxBfPfmuInfo.ucMemCol3 = bf->rTxBfPfmuInfo.ucMemCol3;
		tag->rTxBfPfmuInfo.u2SmartAnt = bf->rTxBfPfmuInfo.u2SmartAnt;
		tag->rTxBfPfmuInfo.ucSEIdx = bf->rTxBfPfmuInfo.ucSEIdx;
		tag->rTxBfPfmuInfo.ucAutoSoundingCtrl =
			bf->rTxBfPfmuInfo.ucAutoSoundingCtrl;
		tag->rTxBfPfmuInfo.uciBfTimeOut =
			bf->rTxBfPfmuInfo.uciBfTimeOut;
		tag->rTxBfPfmuInfo.uciBfDBW = bf->rTxBfPfmuInfo.uciBfDBW;
		tag->rTxBfPfmuInfo.uciBfNcol = bf->rTxBfPfmuInfo.uciBfNcol;
		tag->rTxBfPfmuInfo.uciBfNrow = bf->rTxBfPfmuInfo.uciBfNrow;
		tag->rTxBfPfmuInfo.nr_bw160 = bf->rTxBfPfmuInfo.u1NrBw160;
		tag->rTxBfPfmuInfo.nc_bw160 = bf->rTxBfPfmuInfo.u1NcBw160;
		tag->rTxBfPfmuInfo.ru_start_idx =
			bf->rTxBfPfmuInfo.u1RuStartIdx;
		tag->rTxBfPfmuInfo.ru_end_idx = bf->rTxBfPfmuInfo.u1RuEndIdx;
		tag->rTxBfPfmuInfo.trigger_su = bf->rTxBfPfmuInfo.fgTriggerSu;
		tag->rTxBfPfmuInfo.trigger_mu = bf->rTxBfPfmuInfo.fgTriggerMu;
		tag->rTxBfPfmuInfo.ng16_su = bf->rTxBfPfmuInfo.fgNg16Su;
		tag->rTxBfPfmuInfo.ng16_mu = bf->rTxBfPfmuInfo.fgNg16Mu;
		tag->rTxBfPfmuInfo.codebook42_su =
			bf->rTxBfPfmuInfo.fgCodebook42Su;
		tag->rTxBfPfmuInfo.codebook75_mu =
			bf->rTxBfPfmuInfo.fgCodebook75Mu;
		tag->rTxBfPfmuInfo.he_ltf = bf->rTxBfPfmuInfo.u1HeLtf;

	}
		break;
#endif
#if CFG_SUPPORT_QA_TOOL
	case STA_REC_MAUNAL_ASSOC: {
		struct CMD_MANUAL_ASSOC_STRUCT *cmn;
		struct UNI_CMD_STAREC_MANUAL_ASSOC *tag;

		cmn = (struct CMD_MANUAL_ASSOC_STRUCT *) cmd->aucBuffer;
		tag = (struct UNI_CMD_STAREC_MANUAL_ASSOC *)
			uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_STAREC_TAG_MAUNAL_ASSOC;
		tag->u2Length = sizeof(*tag);
		COPY_MAC_ADDR(tag->aucMac, cmn->aucMac);
		tag->ucType = cmn->ucType;
		tag->ucWtbl = cmn->ucWtbl;
		tag->ucOwnmac = cmn->ucOwnmac;
		tag->ucMode = cmn->ucMode;
		tag->ucBw = cmn->ucBw;
		tag->ucNss = cmn->ucNss;
		tag->ucPfmuId = cmn->ucPfmuId;
		tag->ucMarate = cmn->ucMarate;
		tag->ucSpeIdx = cmn->ucSpeIdx;
		tag->ucaid = cmn->ucaid;
	}
		break;
#endif
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_TX_BF
void nicUniCmdBFActionSoundOff(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_SOUNDING_STOP *tag;

	tag = (struct UNI_CMD_BF_SOUNDING_STOP *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_SOUNDING_OFF;
	tag->u2Length = sizeof(*tag);
	tag->ucSndgStop = cmd->rTxBfSoundingStop.ucSndgStop;
}

void nicUniCmdBFActionSoundOn(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_SND *tag;
	uint32_t i;

	tag = (struct UNI_CMD_BF_SND *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_SOUNDING_ON;
	tag->u2Length = sizeof(*tag);
	tag->u1SuMuSndMode = cmd->rTxBfSoundingStart.ucSuMuSndMode;
	tag->u1StaNum = cmd->rTxBfSoundingStart.ucStaNum;
	for (i = 0; i < 4; i++)
		tag->u2WlanId[i] = cmd->rTxBfSoundingStart.ucWlanId[i];
	tag->u4SndIntv = cmd->rTxBfSoundingStart.u4SoundingInterval;
}

void nicUniCmdBFActionTxApply(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_TX_APPLY *tag;

	tag = (struct UNI_CMD_BF_TX_APPLY *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_DATA_PACKET_APPLY;
	tag->u2Length = sizeof(*tag);
	tag->ucWlanId = cmd->rTxBfTxApply.ucWlanId;
	tag->fgETxBf = cmd->rTxBfTxApply.fgETxBf;
	tag->fgITxBf = cmd->rTxBfTxApply.fgITxBf;
	tag->fgMuTxBf = cmd->rTxBfTxApply.fgMuTxBf;
	tag->fgPhaseCali = cmd->rTxBfTxApply.ucReserved[0];
	DBGLOG(NIC, DEBUG,
		"WlanId:%d, ETxBf:%d, ITxBf:%d, MuTxBf:%d, PhaseCali:%d\n",
		tag->ucWlanId,
		tag->fgETxBf,
		tag->fgITxBf,
		tag->fgMuTxBf,
		tag->fgPhaseCali);
}

void nicUniCmdBFActionMemAlloc(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PFMU_MEM_ALLOC *tag;

	tag = (struct UNI_CMD_BF_PFMU_MEM_ALLOC *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PFMU_MEM_ALLOCATE;
	tag->u2Length = sizeof(*tag);
	tag->u1SuMu = cmd->rTxBfPfmuMemAlloc.ucSuMuMode;
	tag->ucWlanIdx = cmd->rTxBfPfmuMemAlloc.ucWlanIdx;
}

void nicUniCmdBFActionMemRelease(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PFMU_MEM_RLS *tag;

	tag = (struct UNI_CMD_BF_PFMU_MEM_RLS *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PFMU_MEM_RELEASE;
	tag->u2Length = sizeof(*tag);
	tag->ucWlanId = cmd->rTxBfPfmuMemRls.ucWlanId;
	tag->ucReserved[0] = cmd->rTxBfPfmuMemRls.ucReserved[0];
	tag->ucReserved[1] = cmd->rTxBfPfmuMemRls.ucReserved[1];
}

void nicUniCmdBFActionTagRead(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE *tag;

	tag = (struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PFMU_TAG_READ;
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuId = cmd->rProfileTagRead.ucProfileIdx;
	tag->fgBFer = cmd->rProfileTagRead.fgBfer;
	tag->u1TxBf = cmd->rProfileTagRead.ucBandIdx;
	DBGLOG(NIC, DEBUG, "ucPfmuId:%d, fgBFer:%d, u1TxBf:%d\n", tag->ucPfmuId,
		tag->fgBFer,
		tag->u1TxBf);
}

void nicUniCmdBFActionTagWrite(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE *tag;

	tag = (struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PFMU_TAG_WRITE;
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuId = cmd->rProfileTagWrite.ucPfmuId;
	tag->fgBFer = cmd->rProfileTagWrite.fgBFer;
	tag->u1TxBf = cmd->rProfileTagWrite.ucBandIdx;
	memcpy(tag->au4BfPfmuTag1RawData, cmd->rProfileTagWrite.ucBuffer,
		sizeof(tag->au4BfPfmuTag1RawData));
	memcpy(tag->au4BfPfmuTag2RawData, cmd->rProfileTagWrite.ucBuffer +
		sizeof(tag->au4BfPfmuTag1RawData),
		sizeof(tag->au4BfPfmuTag2RawData));
	DBGLOG(NIC, DEBUG, "ucPfmuId:%d, fgBFer:%d, u1TxBf:%d\n", tag->ucPfmuId,
		tag->fgBFer,
		tag->u1TxBf);
	dumpMemory32((uint32_t *)tag->au4BfPfmuTag1RawData,
		sizeof(tag->au4BfPfmuTag1RawData));
	dumpMemory32((uint32_t *)tag->au4BfPfmuTag2RawData,
		sizeof(tag->au4BfPfmuTag2RawData));
}

void nicUniCmdBFActionDataRead(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PROFILE_DATA_READ *tag;

	tag = (struct UNI_CMD_BF_PROFILE_DATA_READ *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PROFILE_READ;
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuIdx = cmd->rProfileDataRead.ucPfmuIdx;
	tag->fgBFer = cmd->rProfileDataRead.fgBFer;
	tag->u2SubCarIdx = cmd->rProfileDataRead.u2SubCarIdx;
	tag->u1TxBf = cmd->rProfileDataRead.ucBandIdx;
}

void nicUniCmdBFActionDataWrite(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	struct UNI_CMD_BF_PROFILE_DATA_WRITE *tag;

	tag = (struct UNI_CMD_BF_PROFILE_DATA_WRITE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BF_TAG_PROFILE_WRITE;
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuIdx = cmd->rProfileDataWrite.ucPfmuIdx;
	tag->u2SubCarIdx = cmd->rProfileDataWrite.u2SubCarrIdxLsb;
	DBGLOG(NIC, DEBUG, "ucPfmuIdx:%d, u2SubCarIdx:%d\n", tag->ucPfmuIdx,
		tag->u2SubCarIdx);
	memcpy(&tag->rTxBfPfmuData, &cmd->rProfileDataWrite.rTxBfPfmuData,
		sizeof(union ORIGIN_PFMU_DATA));
}

void nicUniCmdBFActionPnRead(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	/* Todo: Add tag id for UNI_CMD_PROFILE_PN_READ */
#if 0
	struct UNI_CMD_BF_PROFILE_PN_READ *tag;

	tag = (struct UNI_CMD_BF_PROFILE_PN_READ *) uni_cmd->aucTlvBuffer;
	/* tag->u2Tag */
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuIdx = cmd->rProfilePnRead.ucPfmuIdx;
	tag->ucReserved[0] = cmd->rProfilePnRead.ucReserved[0];
	tag->ucReserved[1] = cmd->rProfilePnRead.ucReserved[1];
#endif
}

void nicUniCmdBFActionPnWrite(
	union CMD_TXBF_ACTION *cmd, struct UNI_CMD_BF *uni_cmd)
{
	/* Todo: Add tag id for UNI_CMD_PROFILE_PN_WRITE */
#if 0
	struct UNI_CMD_BF_PROFILE_PN_WRITE *tag;

	tag = (struct UNI_CMD_BF_PROFILE_PN_WRITE *) uni_cmd->aucTlvBuffer;
	/* tag->u2Tag */
	tag->u2Length = sizeof(*tag);
	tag->ucPfmuIdx = cmd->rProfilePnWrite.ucPfmuIdx;
	tag->u2bw = cmd->rProfilePnWrite.u2bw;
	memcpy(tag->ucBuf, cmd->rProfilePnWrite.ucBuf, 32);
#endif
}

struct  UNI_CMD_BF_HANDLE arBFActionTable[] = {
	{sizeof(struct UNI_CMD_BF_SOUNDING_STOP), nicUniCmdBFActionSoundOff,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_SND), nicUniCmdBFActionSoundOn,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_TX_APPLY), nicUniCmdBFActionTxApply,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PFMU_MEM_ALLOC), nicUniCmdBFActionMemAlloc,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PFMU_MEM_RLS), nicUniCmdBFActionMemRelease,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE),
		nicUniCmdBFActionTagRead, nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_TAG_READ_WRITE),
		nicUniCmdBFActionTagWrite, nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_DATA_READ),
		nicUniCmdBFActionDataRead, nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_DATA_WRITE),
		nicUniCmdBFActionDataWrite, nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_PN_READ), nicUniCmdBFActionPnRead,
		nicUniCmdEventSetCommon},
	{sizeof(struct UNI_CMD_BF_PROFILE_PN_WRITE), nicUniCmdBFActionPnWrite,
		nicUniCmdEventSetCommon},
};

uint32_t nicUniCmdBFAction(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	union CMD_TXBF_ACTION *cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BF);
	uint8_t bf_action_id;

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_BF_ACTION)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (union CMD_TXBF_ACTION *) info->pucInfoBuffer;

	bf_action_id = cmd->rProfileTagRead.ucTxBfCategory;
	if (bf_action_id >= ARRAY_SIZE(arBFActionTable)) {
		DBGLOG(NIC, ERROR, "unknown ACTION_ID:%d\n", bf_action_id);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	max_cmd_len += arBFActionTable[bf_action_id].u4Size;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BF, max_cmd_len,
			arBFActionTable[bf_action_id].pfCmdDoneHandler,
			nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	arBFActionTable[bf_action_id].pfHandler(cmd,
		(struct UNI_CMD_BF *) entry->pucInfoBuffer);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicUniCmdSerAction(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct EXT_CMD_SER_T *cmd;
	struct UNI_CMD_SER *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SER);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_SER)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct EXT_CMD_SER_T *) info->pucInfoBuffer;
	switch (cmd->ucAction) {
	case SER_ACTION_QUERY:
		max_cmd_len += sizeof(struct UNI_CMD_SER_QUERY);
		break;
	case SER_ACTION_SET:
		max_cmd_len += sizeof(struct UNI_CMD_SER_ENABLE);
		break;
	case SER_ACTION_SET_ENABLE_MASK:
		max_cmd_len += sizeof(struct UNI_CMD_SER_SET);
		break;
	case SER_ACTION_RECOVER:
		max_cmd_len += sizeof(struct UNI_CMD_SER_TRIGGER);
		break;
	case SER_ACTION_L0P5_CTRL:
		max_cmd_len += sizeof(struct UNI_CMD_SER_L05);
		break;
	default:
		DBGLOG(NIC, ERROR, "unknown action %d\n", cmd->ucAction);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SER, max_cmd_len,
			NULL, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SER *) entry->pucInfoBuffer;
	switch (cmd->ucAction) {
	case SER_ACTION_QUERY: {
		struct UNI_CMD_SER_QUERY *tag =
			(struct UNI_CMD_SER_QUERY *)uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_SER_TAG_QUERY;
		tag->u2Length = sizeof(*tag);
	}
		break;
	case SER_ACTION_SET: {
		struct UNI_CMD_SER_ENABLE *tag =
			(struct UNI_CMD_SER_ENABLE *)uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_SER_TAG_ENABLE;
		tag->u2Length = sizeof(*tag);
		tag->fgEnable = cmd->ucSerSet;
	}
		break;
	case SER_ACTION_SET_ENABLE_MASK: {
		struct UNI_CMD_SER_SET *tag =
			(struct UNI_CMD_SER_SET *)uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_SER_TAG_SET;
		tag->u2Length = sizeof(*tag);
		tag->u4EnableMask = cmd->ucSerSet;
	}
		break;
	case SER_ACTION_RECOVER: {
		struct UNI_CMD_SER_TRIGGER *tag =
			(struct UNI_CMD_SER_TRIGGER *)uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_SER_TAG_TRIGGER;
		tag->u2Length = sizeof(*tag);
		tag->ucTriggerMethod = cmd->ucSerSet;
		tag->ucDbdcIdx = cmd->ucDbdcIdx;
	}
		break;
	case SER_ACTION_L0P5_CTRL:{
		struct UNI_CMD_SER_L05 *tag =
			(struct UNI_CMD_SER_L05 *)uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_SER_TAG_L0P5_CTRL;
		tag->u2Length = sizeof(*tag);
		tag->ucCtrlAction = cmd->ucSerSet;
	}
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#if (CFG_SUPPORT_TWT == 1)
uint32_t nicUniCmdGetTsf(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct _EXT_CMD_GET_MAC_INFO_T *cmd;
	struct UNI_CMD_GET_MAC_INFO *uni_cmd;
	struct UNI_CMD_MAC_INFO_TSF *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	struct _TWT_GET_TSF_CONTEXT_T *twt;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_MAC_INFO) +
	     		       sizeof(struct UNI_CMD_MAC_INFO_TSF);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_GET_MAC_INFO)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _EXT_CMD_GET_MAC_INFO_T *) info->pucInfoBuffer;
	twt = (struct _TWT_GET_TSF_CONTEXT_T *) info->pvSetQueryBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_MAC_INFO, max_cmd_len,
			nicUniCmdEventGetTsfDone, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_MAC_INFO *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MAC_INFO_TSF *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MAC_INFO_TAG_TSF;
	tag->u2Length = sizeof(*tag);
 	tag->ucDbdcIdx = ENUM_BAND_AUTO;
	tag->ucHwBssidIndex = cmd->rExtraArgument.rTsfArg.ucHwBssidIndex;
	tag->ucBssIndex = twt->ucBssIdx;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
uint32_t nicUniCmdTwtStaGetCnmGranted(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct _EXT_CMD_GET_MAC_INFO_T *cmd;
	struct UNI_CMD_GET_MAC_INFO *uni_cmd;
	struct UNI_CMD_MAC_INFO_TWT_STA_CNM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	struct _TWT_GET_TSF_CONTEXT_T *twt;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_MAC_INFO) +
			sizeof(struct UNI_CMD_MAC_INFO_TWT_STA_CNM);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_TWT_STA_GET_CNM_GRANTED)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _EXT_CMD_GET_MAC_INFO_T *) info->pucInfoBuffer;
	twt = (struct _TWT_GET_TSF_CONTEXT_T *) info->pvSetQueryBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_MAC_INFO, max_cmd_len,
			nicUniCmdEventTWTGetCnmGrantedDone, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_MAC_INFO *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MAC_INFO_TWT_STA_CNM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MAC_INFO_TAG_TWT_STA_CNM;
	tag->u2Length = sizeof(*tag);
	tag->ucDbdcIdx = ENUM_BAND_AUTO;
	tag->ucHwBssidIndex = cmd->rExtraArgument.rTsfArg.ucHwBssidIndex;
	tag->ucBssIndex = twt->ucBssIdx;
	tag->fgTwtEn = (twt->ucTwtStaCnmReason == TWT_STA_CNM_SETUP) ? 1 : 0;
	tag->u4TwtCnmAbortTimeoutMs = ad->rWifiVar.u4TwtCnmAbortTimeoutMs;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicUniUpdateDevInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_DEV_INFO_UPDATE *cmd;
	struct CMD_DEVINFO_ACTIVE *data;
	struct UNI_CMD_DEVINFO *uni_cmd;
	struct UNI_CMD_DEVINFO_ACTIVE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_DEVINFO) +
			       sizeof(struct UNI_CMD_DEVINFO_ACTIVE);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	    info->ucExtCID != EXT_CMD_ID_DEVINFO_UPDATE)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_DEV_INFO_UPDATE *) info->pucInfoBuffer;
	data = (struct CMD_DEVINFO_ACTIVE *) cmd->aucBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_DEVINFO, max_cmd_len,
			nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_DEVINFO *) entry->pucInfoBuffer;
	uni_cmd->ucOwnMacIdx = cmd->ucOwnMacIdx;
	uni_cmd->ucDbdcIdx = cmd->ucDbdcIdx;

	tag = (struct UNI_CMD_DEVINFO_ACTIVE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_DEVINFO_TAG_ACTIVE;
	tag->u2Length = sizeof(*tag);
	tag->ucActive = data->ucActive;
	COPY_MAC_ADDR(tag->aucOwnMacAddr, data->aucOwnMacAddr);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdStaRecConnType(struct ADAPTER *ad, uint32_t legacy_sta_type)
{
	if (legacy_sta_type == STA_TYPE_LEGACY_AP)
		return CONNECTION_INFRA_AP;
	else if (legacy_sta_type == STA_TYPE_LEGACY_CLIENT)
		return CONNECTION_INFRA_STA;
#if CFG_ENABLE_WIFI_DIRECT
	else if (legacy_sta_type == STA_TYPE_P2P_GO)
		return CONNECTION_P2P_GO;
	else if (legacy_sta_type == STA_TYPE_P2P_GC)
		return CONNECTION_P2P_GC;
#endif
#if CFG_SUPPORT_TDLS
	else if (legacy_sta_type == STA_TYPE_DLS_PEER)
		return CONNECTION_TDLS;
#endif
#if CFG_SUPPORT_NAN
	else if (legacy_sta_type == STA_TYPE_NAN)
		return CONNECTION_NAN;
#endif

	DBGLOG(NIC, ERROR, "wrong sta_type=%d\n", legacy_sta_type);
	return CONNECTION_INFRA_STA;
}

uint32_t nicUniCmdStaRecTagBasic(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_BASIC *tag = (struct UNI_CMD_STAREC_BASIC *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->u4ConnectionType = nicUniCmdStaRecConnType(ad, cmd->ucStaType);
	tag->ucConnectionState = STATE_CONNECTED;
	tag->ucIsQBSS = cmd->ucIsQoS;
	tag->u2AID = cmd->u2AssocId;
	COPY_MAC_ADDR(tag->aucPeerMacAddr, cmd->aucMacAddr);
	tag->u2ExtraInfo = STAREC_COMMON_EXTRAINFO_V2 |
			   STAREC_COMMON_EXTRAINFO_NEWSTAREC;

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagState(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_STATE_INFO *tag =
		(struct UNI_CMD_STAREC_STATE_INFO *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_STATE_CHANGED;
	tag->u2Length = sizeof(*tag);
	tag->ucStaState = cmd->ucStaState;
	tag->u4Flags = cmd->u4Flags;
	tag->ucVhtOpMode = cmd->ucVhtOpMode;
	tag->ucActionType = STA_REC_CMD_ACTION_STA;

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagPhyInfo(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_PHY_INFO *tag =
		(struct UNI_CMD_STAREC_PHY_INFO *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_PHY_INFO;
	tag->u2Length = sizeof(*tag);
	tag->u2BSSBasicRateSet = cmd->u2BSSBasicRateSet;
	tag->ucDesiredPhyTypeSet = cmd->ucDesiredPhyTypeSet;
	tag->ucAmpduParam = cmd->ucAmpduParam;
	tag->ucRtsPolicy = cmd->ucRtsPolicy;
	tag->ucRCPI = cmd->ucRCPI;

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagHtInfo(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_HT_INFO *tag =
		(struct UNI_CMD_STAREC_HT_INFO *)buf;

	if (cmd->u2HtCapInfo == 0 && cmd->u2HtExtendedCap == 0)
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_HT_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->u2HtCap = cmd->u2HtCapInfo;
	tag->u2HtExtendedCap = cmd->u2HtExtendedCap;

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagVhtInfo(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_VHT_INFO *tag =
		(struct UNI_CMD_STAREC_VHT_INFO *)buf;

	if (cmd->u4VhtCapInfo == 0)
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_VHT_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->u4VhtCap = cmd->u4VhtCapInfo;
	tag->u2VhtRxMcsMap = cmd->u2VhtRxMcsMap;
	tag->u2VhtTxMcsMap = cmd->u2VhtTxMcsMap;

	return tag->u2Length;
}

#if (CFG_SUPPORT_802_11AX == 1)
uint32_t nicUniCmdStaRecTagHeBasic(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_HE_BASIC *tag =
		(struct UNI_CMD_STAREC_HE_BASIC *)buf;
	static uint8_t zero[HE_PHY_CAP_BYTE_NUM];

	if (!kalMemCmp(cmd->ucHeMacCapInfo, zero, HE_MAC_CAP_BYTE_NUM) &&
	    !kalMemCmp(cmd->ucHePhyCapInfo, zero, HE_PHY_CAP_BYTE_NUM))
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_HE_BASIC;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(tag->aucHeMacCapInfo, cmd->ucHeMacCapInfo,
			sizeof(tag->aucHeMacCapInfo));
	kalMemCopy(tag->aucHePhyCapInfo, cmd->ucHePhyCapInfo,
			sizeof(tag->aucHePhyCapInfo));
	tag->ucPktExt = 2; /* mobile */
	tag->au2RxMaxNssMcs[0] = cmd->u2HeRxMcsMapBW80;
	tag->au2RxMaxNssMcs[1] = cmd->u2HeRxMcsMapBW160;
	tag->au2RxMaxNssMcs[2] = cmd->u2HeRxMcsMapBW80P80;

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagHe6gCap(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_HE_6G_CAP *tag =
		(struct UNI_CMD_STAREC_HE_6G_CAP *)buf;

	if (cmd->u2He6gBandCapInfo == 0)
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_HE_6G_CAP;
	tag->u2Length = sizeof(*tag);
	tag->u2He6gBandCapInfo = cmd->u2He6gBandCapInfo;
	return tag->u2Length;
}
#endif

#if (CFG_SUPPORT_802_11BE == 1)
uint32_t nicUniCmdStaRecTagEhtInfo(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_EHT_BASIC *tag =
		(struct UNI_CMD_STAREC_EHT_BASIC *)buf;
	static uint8_t zero[EHT_PHY_CAP_BYTE_NUM];

	if (!kalMemCmp(cmd->ucEhtMacCapInfo, zero, EHT_MAC_CAP_BYTE_NUM) &&
	    !kalMemCmp(cmd->ucEhtPhyCapInfo, zero, EHT_PHY_CAP_BYTE_NUM))
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_EHT_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->aucPadding[0] = 0xff; /* not used */
	WLAN_GET_FIELD_16(&cmd->ucEhtMacCapInfo[0], &tag->u2EhtMacCap);
	WLAN_GET_FIELD_64(&cmd->ucEhtPhyCapInfo[0], &tag->u8EhtPhyCap);
	tag->u8EhtPhyCapExt = (uint64_t) cmd->ucEhtPhyCapInfo[8];
	WLAN_GET_FIELD_32(&cmd->aucMcsMap20MHzSta[0], &tag->aucMcsMap20MHzSta);
	WLAN_GET_FIELD_24(&cmd->aucMcsMap80MHz[0], &tag->aucMcsMap80MHz);
	WLAN_GET_FIELD_24(&cmd->aucMcsMap160MHz[0], &tag->aucMcsMap160MHz);
	WLAN_GET_FIELD_24(&cmd->aucMcsMap320MHz[0], &tag->aucMcsMap320MHz);

	DBGLOG(INIT, DEBUG,
		"[%d] bss=%d,mac_cap=0x%x,phy_cap=0x%lx,phy_cap_ext=0x%lx\n",
		cmd->ucStaIndex,
		cmd->ucBssIndex,
		tag->u2EhtMacCap,
		tag->u8EhtPhyCap,
		tag->u8EhtPhyCapExt);

	return tag->u2Length;
}
#endif

#if CFG_SUPPORT_RXSMM_ALLOWLIST
uint32_t nicUniCmdStaRecTagBfee(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_BFEE *tag =
		(struct UNI_CMD_STAREC_BFEE *)buf;

	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);

	if (!prStaRec) {
		DBGLOG(INIT, ERROR, "[BF]prStaRec is NULL!\n");
		return 0;
	}

	tag->u2Tag = UNI_CMD_STAREC_TAG_BFEE;
	tag->u2Length = sizeof(*tag);
	tag->rBfeeStaRec.fgFbIdentityMatrix = FALSE;
	tag->rBfeeStaRec.fgIgnFbk = FALSE;
	tag->rBfeeStaRec.fgRxsmmEnable = prStaRec->fgRxsmmEnable;

	DBGLOG(INIT, TRACE, "[BF]fgRxsmmEnable=%d\n",
		tag->rBfeeStaRec.fgRxsmmEnable);

	return tag->u2Length;
}
#endif


#if (CFG_SUPPORT_802_11BE_MLO == 1)
static uint32_t nicUniCmdStaRecTagMldSetupImpl(struct ADAPTER *ad,
	uint8_t *buf, uint8_t ucBssIndex, uint8_t ucStaIndex)
{
	struct UNI_CMD_STAREC_MLD_SETUP *tag = (struct UNI_CMD_STAREC_MLD_SETUP *)buf;
	struct UNI_CMD_STAREC_LINK_INFO *link;
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, ucStaIndex);
	struct MLD_STA_RECORD *prMldStaRec = mldStarecGetByStarec(ad, prStaRec);
	struct LINK *prStaList;
	struct STA_RECORD *prCurStaRec;

	if (!prStaRec || !prMldStaRec)
		return 0;

	prStaList = &prMldStaRec->rStarecList;
	tag->u2Tag = UNI_CMD_STAREC_TAG_MLD_SETUP;
	tag->u2Length = sizeof(*tag) + sizeof(*link) * prStaList->u4NumElem;
	COPY_MAC_ADDR(tag->aucPeerMldAddr, prMldStaRec->aucPeerMldAddr);
	tag->u2PrimaryMldId = prMldStaRec->u2PrimaryMldId;
	tag->u2SecondMldId = prMldStaRec->u2SecondMldId;
	tag->u2SetupWlanId = prMldStaRec->u2SetupWlanId;
	tag->ucLinkNumber = prStaList->u4NumElem;

	DBGLOG(INIT, DEBUG,
	       "[%d] bss=%d,pri=%d,sec=%d,setup=%d,num=%d,mac=" MACSTR "\n",
	       prStaRec->ucIndex,
	       ucBssIndex,
	       tag->u2PrimaryMldId,
	       tag->u2SecondMldId,
	       tag->u2SetupWlanId,
	       tag->ucLinkNumber,
	       MAC2STR(prMldStaRec->aucPeerMldAddr));
#if (CFG_SUPPORT_MLD_LOG == 1) && (CFG_SUPPORT_802_11BE_MLO == 1)
	mldLogSetup(ad, prStaRec, MLD_LOG_SUCCESS);
	mldLogT2LMStatus(ad, prStaRec);
#endif

	link = (struct UNI_CMD_STAREC_LINK_INFO *)tag->aucLinkInfo;
	LINK_FOR_EACH_ENTRY(prCurStaRec, prStaList, rLinkEntryMld,
			struct STA_RECORD) {
		link->ucBssIdx = prCurStaRec->ucBssIndex;
		link->u2WlanIdx = prCurStaRec->ucWlanIndex;
		DBGLOG(INIT, DEBUG, "\tbss=%d,wlan_idx=%d\n",
			link->ucBssIdx,
			link->u2WlanIdx);
		link++;
	}

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagMldSetup(struct ADAPTER *ad,
		uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);

	if (!prStaRec ||
			prStaRec->ucStaState != STA_STATE_3)
		return 0;

	return nicUniCmdStaRecTagMldSetupImpl(ad, buf,
			cmd->ucBssIndex, cmd->ucStaIndex);
}


#if (CFG_MLD_INFO_PRESETUP == 1)
uint32_t nicUniCmdSetBssMld(struct ADAPTER *ad,
		struct BSS_INFO *prBssInfo)
{
	struct UNI_CMD_BSSINFO *bss_cmd = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t max_cmd_len = 0;
	uint32_t ret_len = 0;

	ASSERT(prBssInfo);

	/* update bssinfo mld info */
	max_cmd_len = sizeof(struct UNI_CMD_BSSINFO) +
		sizeof(struct UNI_CMD_BSSINFO_MLD);

	bss_cmd = (struct UNI_CMD_BSSINFO *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!bss_cmd) {
		DBGLOG(INIT, ERROR, "Allocate UNI_CMD_BSSINFO failed.\n");
		return WLAN_STATUS_RESOURCES;
	}

	bss_cmd->ucBssInfoIdx = prBssInfo->ucBssIndex;

	ret_len = nicUniCmdBssInfoMld(ad, bss_cmd->aucTlvBuffer,
			prBssInfo->ucBssIndex);
	if (ret_len > 0) {
		status = wlanSendSetQueryUniCmd(ad,
				UNI_CMD_ID_BSSINFO,
				TRUE,
				FALSE,
				FALSE,
				nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon,
				max_cmd_len,
				(void *)bss_cmd, NULL, 0);
		cnmMemFree(ad, bss_cmd);

		/* convert WLAN_STATUS_PENDING to success */
		if (status == WLAN_STATUS_PENDING)
			status = WLAN_STATUS_SUCCESS;
	}

	return status;
}

uint32_t nicUniCmdSetStarecMld(struct ADAPTER *ad,
		struct STA_RECORD *prStaRec)
{
	struct UNI_CMD_STAREC *sta_cmd = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t max_cmd_len = 0;
	uint32_t ret_len = 0;
	uint16_t widx = 0;

	ASSERT(prStaRec);

	/* update sta rec ML info */
	max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
		sizeof(struct UNI_CMD_STAREC_MLD_SETUP) +
		sizeof(struct UNI_CMD_STAREC_LINK_INFO) * UNI_MLD_LINK_MAX;

	sta_cmd = (struct UNI_CMD_STAREC *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!sta_cmd) {
		DBGLOG(INIT, ERROR, "Allocate UNI_CMD_STAREC failed.\n");
		return WLAN_STATUS_RESOURCES;
	}

	sta_cmd->ucBssInfoIdx = prStaRec->ucBssIndex;
	widx = (uint16_t) secGetWlanIdxByStaIdx(ad, prStaRec->ucIndex);
	WCID_SET_H_L(sta_cmd->ucWlanIdxHnVer, sta_cmd->ucWlanIdxL, widx);

	ret_len = nicUniCmdStaRecTagMldSetupImpl(ad, sta_cmd->aucTlvBuffer,
			prStaRec->ucBssIndex, prStaRec->ucIndex);
	if (ret_len > 0) {
		/* setup correct size */
		max_cmd_len = sizeof(struct UNI_CMD_STAREC) + ret_len;

		status = wlanSendSetQueryUniCmd(ad,
				UNI_CMD_ID_STAREC_INFO,
				TRUE,
				FALSE,
				FALSE,
				nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon,
				max_cmd_len,
				(void *)sta_cmd, NULL, 0);
		cnmMemFree(ad, sta_cmd);

		/* convert WLAN_STATUS_PENDING to success */
		if (status == WLAN_STATUS_PENDING)
			status = WLAN_STATUS_SUCCESS;
	}

	return status;
}
#endif /* CFG_MLD_INFO_PRESETUP */

uint32_t nicUniCmdMldStaTeardown(struct ADAPTER *ad,
	struct STA_RECORD *prStaRec)
{
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_MLD_TEARDOWN *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
			sizeof(struct UNI_CMD_STAREC_MLD_TEARDOWN);
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint16_t widx = 0;

	uni_cmd = (struct UNI_CMD_STAREC *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd->ucBssInfoIdx = prStaRec->ucBssIndex;
	widx = (uint16_t) prStaRec->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);
	tag = (struct UNI_CMD_STAREC_MLD_TEARDOWN *)uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_MLD_TEARDOWN;
	tag->u2Length = sizeof(*tag);

	DBGLOG(INIT, DEBUG, "[%d] bss_idx: %d\n",
		prStaRec->ucIndex,
		uni_cmd->ucBssInfoIdx);

	status = wlanSendSetQueryUniCmd(ad,
			     UNI_CMD_ID_STAREC_INFO,
			     TRUE,
			     FALSE,
			     FALSE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);
	return status;
}

#if (CFG_SUPPORT_ML_RECONFIG == 1)
uint32_t nicUniCmdUpdateMldRecfgState(struct ADAPTER *ad,
	struct MLD_STA_RECORD *mld_starec)
{
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_MLD_RECFG_STATE *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
			       sizeof(struct UNI_CMD_STAREC_MLD_RECFG_STATE);
	struct UNI_CMD_STAREC_RECFG_LINK_INFO *link;
	struct LINK *sta_list;
	struct STA_RECORD *sta, *cur;
	uint16_t widx = 0;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (!mld_starec)
		return WLAN_STATUS_FAILURE;

	sta_list = &mld_starec->rStarecList;
	sta = LINK_PEEK_HEAD(sta_list, struct STA_RECORD, rLinkEntryMld);
	if (!sta)
		return WLAN_STATUS_FAILURE;

	max_cmd_len += sta_list->u4NumElem *
		sizeof(struct UNI_CMD_STAREC_RECFG_LINK_INFO);
	uni_cmd = (struct UNI_CMD_STAREC *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd->ucBssInfoIdx = sta->ucBssIndex;
	widx = (uint16_t) sta->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);
	tag = (struct UNI_CMD_STAREC_MLD_RECFG_STATE *)uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_MLD_RECFG_STATE;
	tag->u2Length = sizeof(*tag) + sta_list->u4NumElem *
		sizeof(struct UNI_CMD_STAREC_RECFG_LINK_INFO);
	tag->ucLinkNumber = sta_list->u4NumElem;

	DBGLOG(INIT, INFO,
		"[%d] bss=%d,pri=%d,sec=%d,setup=%d,num=%d,mac=" MACSTR "\n",
		sta->ucIndex,
		sta->ucBssIndex,
		mld_starec->u2PrimaryMldId,
		mld_starec->u2SecondMldId,
		mld_starec->u2SetupWlanId,
		sta_list->u4NumElem,
		MAC2STR(mld_starec->aucPeerMldAddr));

	link = (struct UNI_CMD_STAREC_RECFG_LINK_INFO *)tag->aucLinkInfo;
	LINK_FOR_EACH_ENTRY(cur, sta_list, rLinkEntryMld, struct STA_RECORD) {
		link->ucBssIdx = cur->ucBssIndex;
		link->u2WlanIdx = cur->ucWlanIndex;
		link->fgApRemoval = cur->fgApRemoval;

		DBGLOG(INIT, INFO,
			"\tbss=%d,wlan_idx=%d,del=%d\n",
			link->ucBssIdx,
			link->u2WlanIdx,
			link->fgApRemoval);
		link++;
	}

	status = wlanSendSetQueryUniCmd(ad,
			     UNI_CMD_ID_STAREC_INFO,
			     TRUE,
			     FALSE,
			     FALSE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);
	return status;
}
#endif /* CFG_SUPPORT_ML_RECONFIG */

uint32_t nicUniCmdStaRecTagEhtMld(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);
	struct UNI_CMD_STAREC_EHT_MLD *tag = (struct UNI_CMD_STAREC_EHT_MLD *)buf;
	struct MLD_STA_RECORD *prMldStarec = mldStarecGetByStarec(ad, prStaRec);

	if (!prStaRec || prStaRec->ucStaState != STA_STATE_3)
		return 0;

	if (!prMldStarec)
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_EHT_MLD;
	tag->u2Length = sizeof(struct UNI_CMD_STAREC_EHT_MLD);
	tag->fgEPCS = prMldStarec->fgEPCS;
	tag->fgMldType = prMldStarec->fgMldType;

	kalMemCopy(tag->afgStrCapBitmap,
		prMldStarec->aucStrBitmap,
		sizeof(tag->afgStrCapBitmap));
	kalMemCopy(tag->aucEmlCap,
		&prMldStarec->u2EmlCap,
		sizeof(prMldStarec->u2EmlCap));

	DBGLOG(INIT, DEBUG,
		"[%d] bss=%d,epcs=%d,eml=0x%04x,str[0x%x,0x%x,0x%x] mldType=%d\n",
		prStaRec->ucIndex,
		cmd->ucBssIndex,
		tag->fgEPCS,
		*(uint16_t *)tag->aucEmlCap,
		tag->afgStrCapBitmap[0],
		tag->afgStrCapBitmap[1],
		tag->afgStrCapBitmap[2],
		tag->fgMldType);

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagEhtMldV2(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);
	struct UNI_CMD_STAREC_EHT_MLD_V2 *tag =
			(struct UNI_CMD_STAREC_EHT_MLD_V2 *)buf;
	struct MLD_STA_RECORD *prMldStarec = mldStarecGetByStarec(ad, prStaRec);

	if (!prStaRec || prStaRec->ucStaState != STA_STATE_3)
		return 0;

	if (!prMldStarec)
		return 0;

	tag->u2Tag = UNI_CMD_STAREC_TAG_EHT_MLD_V2;
	tag->u2Length = sizeof(struct UNI_CMD_STAREC_EHT_MLD_V2);
	tag->u2ExtMldCap = prMldStarec->u2ExtMldCap;

	DBGLOG(INIT, INFO,
		"[%d] bss=%d,extmldcap=0x%x\n",
		prStaRec->ucIndex,
		cmd->ucBssIndex,
		tag->u2ExtMldCap);

	return tag->u2Length;
}
#endif

static void nicUniCmdSetRecSecPnInfoEvt(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_STAREC *evt;
	struct UNI_CMD_STAREC_GET_PN *tag;
	struct PARAM_TX_TSC_INFO *tsc;
	uint8_t i;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_STAREC)
		return;

	evt = (struct UNI_EVENT_STAREC *)uni_evt->aucBuffer;
	tsc = cmd->pvInformationBuffer;

	DBGLOG(NIC, DEBUG, "wlan_idx=%d\n",
		evt->u2WlanIdx);

	if (!cmd->pvInformationBuffer ||
	    tsc->ucWlanIdx != evt->u2WlanIdx)
		goto exit;

	tag = (struct UNI_CMD_STAREC_GET_PN *)evt->aucTlvBuffer;
	DBGLOG(NIC, DEBUG, "tag=%d\n", tag->u2Tag);
	if (tag->u2Tag != UNI_CMD_STAREC_TAG_GET_PN)
		goto exit;

	for (i = 0; i < tsc->u4TscCount; i++, tag++) {
		DBGLOG(NIC, DEBUG, "\t[%d] type=%d\n",
			i,
			tag->ucTscType);
		if (tag->ucTscType != tsc->aucEntries[i].ucTscType)
			continue;

		DBGLOG_MEM8(NIC, DEBUG, tag->aucPn, sizeof(tag->aucPn));
		kalMemCopy(tsc->aucEntries[i].aucKeyPn,
			   tag->aucPn,
			   sizeof(tag->aucPn));
	}

exit:
	if (cmd->fgIsOid)
		kalOidComplete(ad->prGlueInfo, cmd,
			       cmd->u4InformationBufferLength,
			       WLAN_STATUS_SUCCESS);
}

uint32_t UniCmdSetRecSecPnInfo(struct ADAPTER *ad,
	struct PARAM_TX_TSC_INFO *tsc)
{
	struct UNI_CMD_STAREC *uni_cmd = NULL;
	struct UNI_CMD_STAREC_GET_PN *tag;
	uint32_t max_cmd_len = 0, status = WLAN_STATUS_SUCCESS;
	uint8_t i;

	if (!tsc || tsc->u4TscCount == 0) {
		DBGLOG(NIC, ERROR, "tsc=0x%p tsc_cnt=%d\n",
			tsc,
			tsc != NULL ? tsc->u4TscCount : 0);
		status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
		tsc->u4TscCount * sizeof(struct UNI_CMD_STAREC_GET_PN);
	uni_cmd = (struct UNI_CMD_STAREC *) cnmMemAlloc(ad,
							RAM_TYPE_MSG,
							max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR, "Allocate UNI_CMD ==> FAILED.\n");
		status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	DBGLOG(NIC, DEBUG, "bss=%d wlan_idx=%d tsc_count=%d\n",
		tsc->ucBssIdx,
		tsc->ucWlanIdx,
		tsc->u4TscCount);

	uni_cmd->ucBssInfoIdx = tsc->ucBssIdx;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL,
		     tsc->ucWlanIdx);
	tag = (struct UNI_CMD_STAREC_GET_PN *)uni_cmd->aucTlvBuffer;
	for (i = 0; i < tsc->u4TscCount; i++, tag++) {
		tag->u2Tag = UNI_CMD_STAREC_TAG_GET_PN;
		tag->u2Length = sizeof(*tag);
		tag->ucTscType = tsc->aucEntries[i].ucTscType;
		kalMemCopy(tag->aucPn,
			   tsc->aucEntries[i].aucKeyPn,
			   sizeof(tag->aucPn));
		DBGLOG(NIC, DEBUG, "\t[%d] type=%d\n",
			i,
			tag->ucTscType);
	}

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_STAREC_INFO,
					FALSE,
					TRUE,
					TRUE,
					nicUniCmdSetRecSecPnInfoEvt,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					tsc,
					sizeof(*tsc));

exit:
	if (uni_cmd)
		cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdStaRecTagRA(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_RA_INFO *tag =
		(struct UNI_CMD_STAREC_RA_INFO *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_RA;
	tag->u2Length = sizeof(*tag);
	tag->u2DesiredNonHTRateSet = cmd->u2DesiredNonHTRateSet;
	kalMemCopy(tag->aucRxMcsBitmask, cmd->aucRxMcsBitmask, 10);

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagBAOffload(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_BA_OFFLOAD_INFO *tag =
		(struct UNI_CMD_STAREC_BA_OFFLOAD_INFO *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_BA_OFFLOAD;
	tag->u2Length = sizeof(*tag);
	tag->ucTxAmpdu = cmd->ucTxAmpdu;
	tag->ucRxAmpdu = cmd->ucRxAmpdu;
	tag->ucTxAmsduInAmpdu = cmd->ucTxAmsduInAmpdu;
	tag->ucRxAmsduInAmpdu = cmd->ucRxAmsduInAmpdu;
	tag->u4TxMaxAmsduInAmpduLen = cmd->u4TxMaxAmsduInAmpduLen;

	if (cmd->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11BE) {
		tag->u2TxBaSize = cmd->rBaSize.rEhtBaSize.u2TxBaSize;
		tag->u2RxBaSize = cmd->rBaSize.rEhtBaSize.u2RxBaSize;
	} else if (cmd->ucDesiredPhyTypeSet & PHY_TYPE_SET_802_11AX) {
		tag->u2TxBaSize = cmd->rBaSize.rHeBaSize.u2TxBaSize;
		tag->u2RxBaSize = cmd->rBaSize.rHeBaSize.u2RxBaSize;
	} else {
		tag->u2TxBaSize = cmd->rBaSize.rHtVhtBaSize.ucTxBaSize;
		tag->u2RxBaSize = cmd->rBaSize.rHtVhtBaSize.ucRxBaSize;
	}

	DBGLOG(NIC, TRACE,
		"StaRec[%u] WIDX[%u] Ampdu[%d,%d], Amsdu[%d,%d,len=%d] BaSize[%d,%d]",
		cmd->ucStaIndex,
		cmd->ucWlanIndex,
		tag->ucTxAmpdu,
		tag->ucRxAmpdu,
		tag->ucTxAmsduInAmpdu,
		tag->ucRxAmsduInAmpdu,
		tag->u4TxMaxAmsduInAmpduLen,
		tag->u2TxBaSize,
		tag->u2RxBaSize);

	return tag->u2Length;
}

uint32_t nicUniCmdStaRecTagUapsd(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct UNI_CMD_STAREC_UAPSD_INFO *tag =
		(struct UNI_CMD_STAREC_UAPSD_INFO *)buf;

	tag->u2Tag = UNI_CMD_STAREC_TAG_UAPSD;
	tag->u2Length = sizeof(*tag);
	tag->fgIsUapsdSupported = cmd->ucIsUapsdSupported;
	tag->ucUapsdAc = cmd->ucUapsdAc;
	tag->ucUapsdSp = cmd->ucUapsdSp;
	return tag->u2Length;
}

#if CFG_SUPPORT_MLR
uint32_t nicUniCmdStaRecTagMlrInfo(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);
	struct UNI_CMD_STAREC_MLR_INFO *tag =
		(struct UNI_CMD_STAREC_MLR_INFO *)buf;

	if (!prStaRec) {
		DBGLOG(REQ, WARN, "MLR unicmd - prStaRec is NULL\n");
		return 0;
	}

	tag->u2Tag = UNI_CMD_STAREC_TAG_MLR_INFO;
	tag->u2Length = sizeof(struct UNI_CMD_STAREC_MLR_INFO);
	tag->ucMlrMode = cmd->ucMlrMode;
	tag->ucMlrState = cmd->ucMlrState;

	DBGLOG(REQ, DEBUG,
		"MLR unicmd - StaRec[%u] WIDX[%u] ucStaState[%u] MLR[0x%02x, 0x%02x] ucMlrMode[0x%02x] ucMlrState[%u] RCPI=%d(RSSI=%d)\n",
		cmd->ucStaIndex,
		cmd->ucWlanIndex,
		prStaRec->ucStaState,
		ad->u4MlrSupportBitmap,
		prStaRec->ucMlrSupportBitmap,
		tag->ucMlrMode,
		tag->ucMlrState,
		prStaRec->ucRCPI,
		RCPI_TO_dBm(prStaRec->ucRCPI));

	return tag->u2Length;
}
#endif

#if CFG_SUPPORT_NAN
uint32_t nicUniCmdStaRecTagNan(struct ADAPTER *ad,
	uint8_t *buf, struct CMD_UPDATE_STA_RECORD *cmd)
{
	struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(ad, cmd->ucStaIndex);
	struct UNI_CMD_STAREC_NAN *tag =
		(struct UNI_CMD_STAREC_NAN *)buf;

	if (!prStaRec) {
		DBGLOG(REQ, WARN, "NAN unicmd - prStaRec is NULL\n");
		return 0;
	}

	tag->u2Tag = UNI_CMD_STAREC_TAG_NAN;
	tag->u2Length = sizeof(struct UNI_CMD_STAREC_NAN);
	tag->ucOtherWlanIndex = cmd->ucOtherWlanIndex;

	DBGLOG(REQ, TRACE,
		"NAN unicmd - StaRec[%u] WIDX[%u] other WIDX[%u]\n",
		cmd->ucStaIndex,
		cmd->ucWlanIndex,
		cmd->ucOtherWlanIndex);

	return tag->u2Length;
}
#endif


struct UNI_CMD_STAREC_TAG_HANDLE arUpdateStaRecTable[] = {
	{sizeof(struct UNI_CMD_STAREC_BASIC), nicUniCmdStaRecTagBasic},
	{sizeof(struct UNI_CMD_STAREC_HT_INFO), nicUniCmdStaRecTagHtInfo},
	{sizeof(struct UNI_CMD_STAREC_VHT_INFO), nicUniCmdStaRecTagVhtInfo},
#if (CFG_SUPPORT_802_11AX == 1)
	{sizeof(struct UNI_CMD_STAREC_HE_BASIC), nicUniCmdStaRecTagHeBasic},
	{sizeof(struct UNI_CMD_STAREC_HE_6G_CAP), nicUniCmdStaRecTagHe6gCap},
#endif
	{sizeof(struct UNI_CMD_STAREC_STATE_INFO), nicUniCmdStaRecTagState},
	{sizeof(struct UNI_CMD_STAREC_PHY_INFO), nicUniCmdStaRecTagPhyInfo},
	{sizeof(struct UNI_CMD_STAREC_RA_INFO), nicUniCmdStaRecTagRA},
	{sizeof(struct UNI_CMD_STAREC_BA_OFFLOAD_INFO),
	 nicUniCmdStaRecTagBAOffload},
	{sizeof(struct UNI_CMD_STAREC_UAPSD_INFO), nicUniCmdStaRecTagUapsd},
#if CFG_SUPPORT_RXSMM_ALLOWLIST
	{sizeof(struct UNI_CMD_STAREC_BFEE), nicUniCmdStaRecTagBfee},
#endif
#if (CFG_SUPPORT_802_11BE == 1)
	{sizeof(struct UNI_CMD_STAREC_EHT_BASIC), nicUniCmdStaRecTagEhtInfo},
#endif
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	{sizeof(struct UNI_CMD_STAREC_EHT_MLD), nicUniCmdStaRecTagEhtMld},
	{sizeof(struct UNI_CMD_STAREC_EHT_MLD_V2), nicUniCmdStaRecTagEhtMldV2},
	{sizeof(struct UNI_CMD_STAREC_MLD_SETUP) +
	 sizeof(struct UNI_CMD_STAREC_LINK_INFO) * UNI_MLD_LINK_MAX,
	 nicUniCmdStaRecTagMldSetup},
#endif
#if (CFG_SUPPORT_MLR == 1)
	{sizeof(struct UNI_CMD_STAREC_MLR_INFO), nicUniCmdStaRecTagMlrInfo},
#endif
#if (CFG_SUPPORT_NAN == 1)
	{sizeof(struct UNI_CMD_STAREC_NAN), nicUniCmdStaRecTagNan},
#endif

};

uint32_t nicUniCmdUpdateStaRec(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_UPDATE_STA_RECORD *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t *pos;
	uint32_t max_cmd_len = 0;
	int i;
	uint16_t widx = 0;

	if (info->ucCID != CMD_ID_UPDATE_STA_RECORD ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_UPDATE_STA_RECORD *) info->pucInfoBuffer;
	max_cmd_len += sizeof(struct UNI_CMD_STAREC);
	for (i = 0; i < ARRAY_SIZE(arUpdateStaRecTable); i++)
		max_cmd_len += arUpdateStaRecTable[i].u4Size;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO, max_cmd_len,
		cmd->ucNeedResp ? nicUniCmdStaRecHandleEventPkt : NULL,	NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	pos = entry->pucInfoBuffer;
	uni_cmd = (struct UNI_CMD_STAREC *) pos;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	widx = (uint16_t) cmd->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);
	pos += sizeof(*uni_cmd);
	for (i = 0; i < ARRAY_SIZE(arUpdateStaRecTable); i++)
		pos += arUpdateStaRecTable[i].pfHandler(ad, pos, cmd);
	entry->u4SetQueryInfoLen = pos - entry->pucInfoBuffer;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

enum ENUM_UNI_CMD_CNM_CH_REQ_BAND
nicUniCmdChReqBandType(enum ENUM_MBMC_BN eBand)
{
#define B2B(x) case ENUM_BAND_##x: eUniBand = UNI_CMD_CNM_CH_REQ_BAND_##x; break

	enum ENUM_UNI_CMD_CNM_CH_REQ_BAND eUniBand;

	switch (eBand) {
	B2B(0);
	B2B(1);
#if (CFG_SUPPORT_CONNAC3X == 1)
	B2B(2);
#endif
	B2B(ALL);
	B2B(AUTO);
	default:
		DBGLOG(RLM, WARN, "unexpected band: %d\n", eBand);
		eUniBand = UNI_CMD_CNM_CH_REQ_BAND_AUTO;
		break;
	}

	return eUniBand;
}

static enum ENUM_UNI_CMD_CNM_CHANNEL_WIDTH
		channelWidthTransform(enum ENUM_CHANNEL_WIDTH eRfChannelWidth)
{
	enum ENUM_UNI_CMD_CNM_CHANNEL_WIDTH eWidth;

	switch (eRfChannelWidth) {
	case CW_20_40MHZ:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_20_40MHZ;
		break;
	case CW_80MHZ:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_80MHZ;
		break;
	case CW_160MHZ:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_160MHZ;
		break;
	case CW_80P80MHZ:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_80P80MHZ;
		break;
	case CW_320_1MHZ:
	case CW_320_2MHZ:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_320MHZ;
		break;
	default:
		eWidth = UNI_CMD_CNM_CHANNEL_WIDTH_20_40MHZ;
		break;
	}

	return eWidth;
}

static uint32_t nicUniCmdChReqPrivilege(struct ADAPTER *ad,
		struct MSG_CH_REQ *msg,
		struct WIFI_UNI_CMD_ENTRY **out_entry)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct BSS_INFO *bss;
	struct MLD_BSS_INFO *mld_bss;
#endif
	struct UNI_CMD_CNM *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	struct UNI_CMD_CNM_CH_PRIVILEGE_REQ *tag;
	uint8_t i = 0;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CNM);

	max_cmd_len += sizeof(struct UNI_CMD_CNM_CH_PRIVILEGE_REQ) *
		(msg->ucExtraChReqNum + 1);
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CNM,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CNM *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_CNM_CH_PRIVILEGE_REQ *)&uni_cmd->aucTlvBuffer[0];
	for (i = 0; i < msg->ucExtraChReqNum + 1; i++, tag++) {
		struct MSG_CH_REQ *sub_req = NULL;
		uint8_t extra = 0;

		if (i == 0) {
			sub_req = (struct MSG_CH_REQ *)msg;
			tag->u2Tag = UNI_CMD_CNM_TAG_CH_PRIVILEGE_REQ;

#if (CFG_SUPPORT_802_11BE_MLO == 1)
			bss = GET_BSS_INFO_BY_INDEX(ad, sub_req->ucBssIndex);
			mld_bss = mldBssGetByBss(ad, bss);
#endif
		} else {
			sub_req = (struct MSG_CH_REQ *)&msg[i];
			tag->u2Tag = UNI_CMD_CNM_TAG_CH_PRIVILEGE_MLO_SUB_REQ;
		}
		tag->u2Length = sizeof(*tag);
		tag->ucTokenID = sub_req->ucTokenID;
		tag->ucReqType = sub_req->eReqType;
		tag->u4MaxInterval = sub_req->u4MaxInterval;
		tag->ucBssIndex = sub_req->ucBssIndex;
		tag->ucRfBand = sub_req->eRfBand;
		tag->ucPrimaryChannel = sub_req->ucPrimaryChannel;
		tag->ucRfChannelWidth =
			(uint8_t)
			channelWidthTransform(sub_req->eRfChannelWidth);
		tag->ucRfSco = sub_req->eRfSco;
		tag->ucRfCenterFreqSeg1 = sub_req->ucRfCenterFreqSeg1;
		tag->ucRfCenterFreqSeg2 = sub_req->ucRfCenterFreqSeg2;

		/* If the caller doesn't fill the fields of AP, keep them
		 * same as STA's capability.
		 */
		if (sub_req->ucRfCenterFreqSeg1FromAP != 0) {
			tag->ucRfChannelWidthFromAP =
				(uint8_t)channelWidthTransform(
					sub_req->eRfChannelWidthFromAP);
			tag->ucRfCenterFreqSeg1FromAP =
				sub_req->ucRfCenterFreqSeg1FromAP;
			tag->ucRfCenterFreqSeg2FromAP =
				sub_req->ucRfCenterFreqSeg2FromAP;
		} else {
			tag->ucRfChannelWidthFromAP =
				tag->ucRfChannelWidth;
			tag->ucRfCenterFreqSeg1FromAP =
				sub_req->ucRfCenterFreqSeg1;
			tag->ucRfCenterFreqSeg2FromAP =
				sub_req->ucRfCenterFreqSeg2;
		}

		tag->ucDBDCBand = nicUniCmdChReqBandType(sub_req->eDBDCBand);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		if (IS_BSS_APGO(bss)) {
			if (IS_MLD_BSSINFO_MULTI(mld_bss))
				extra |= BIT(
				CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_MULTI_LINK);
		} else {
			if (msg->ucExtraChReqNum >= 1)
				extra |= BIT(
				CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_MULTI_LINK);
			if (mld_bss && mld_bss->ucEmlEnabled)
				extra |= BIT(
				CNM_CH_PRIVILEGE_REQ_EXTRA_INFO_EMLSR);
		}
#endif
		tag->ucExtraInfo = extra;

		DBGLOG(INIT, DEBUG,
			"bss=%d,token=%d,type=%d,interval=%d,ch[%d %d %d %d %d %d],ap[%d %d %d],dbdc=%d,extra=%u\n",
			tag->ucBssIndex,
			tag->ucTokenID,
			tag->ucReqType,
			tag->u4MaxInterval,
			tag->ucRfBand,
			tag->ucPrimaryChannel,
			tag->ucRfChannelWidth,
			tag->ucRfSco,
			tag->ucRfCenterFreqSeg1,
			tag->ucRfCenterFreqSeg2,
			tag->ucRfChannelWidthFromAP,
			tag->ucRfCenterFreqSeg1FromAP,
			tag->ucRfCenterFreqSeg2FromAP,
			tag->ucDBDCBand,
			tag->ucExtraInfo);
	}
	*out_entry = entry;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t nicUniCmdChAbortPrivilege(struct ADAPTER *ad,
		struct MSG_CH_ABORT *msg,
		struct WIFI_UNI_CMD_ENTRY **out_entry)
{
	struct UNI_CMD_CNM *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	struct UNI_CMD_CNM_CH_PRIVILEGE_ABORT *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CNM);

	max_cmd_len += sizeof(struct UNI_CMD_CNM_CH_PRIVILEGE_ABORT);
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CNM,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CNM *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_CNM_CH_PRIVILEGE_ABORT *)&uni_cmd->aucTlvBuffer[0];
	tag->u2Tag = UNI_CMD_CNM_TAG_CH_PRIVILEGE_ABORT;
	tag->u2Length = sizeof(*tag);
	tag->ucBssIndex = msg->ucBssIndex;
	tag->ucTokenID = msg->ucTokenID;
	tag->ucDBDCBand = nicUniCmdChReqBandType(msg->eDBDCBand);

	DBGLOG(INIT, DEBUG, "bss=%d,token=%d,dbdc=%d\n",
		tag->ucBssIndex,
		tag->ucTokenID,
		tag->ucDBDCBand);

	*out_entry = entry;

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdChPrivilege(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_CH_PRIVILEGE *cmd;
	struct WIFI_UNI_CMD_ENTRY *entry = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (info->ucCID != CMD_ID_CH_PRIVILEGE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CH_PRIVILEGE *) info->pucInfoBuffer;

	if (cmd->ucAction == CMD_CH_ACTION_ABORT)
		status = nicUniCmdChAbortPrivilege(ad, info->pvSetQueryBuffer, &entry);
	else
		status = nicUniCmdChReqPrivilege(ad, info->pvSetQueryBuffer, &entry);

	if (status == WLAN_STATUS_SUCCESS && entry)
		LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return status;
}

uint32_t nicUniCmdCnmGetInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct PARAM_GET_CNM_T *cmd;
	struct UNI_CMD_CNM *uni_cmd;
	struct UNI_CMD_CNM_GET_INFO *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CNM) +
			       sizeof(struct UNI_CMD_CNM_GET_INFO);

	if (info->ucCID != CMD_ID_GET_CNM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct PARAM_GET_CNM_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CNM,
		max_cmd_len, nicUniEventQueryCnmInfo, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_CNM *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_CNM_GET_INFO *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_CNM_TAG_GET_INFO;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdAccessReg(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ACCESS_REG *cmd;
	struct UNI_CMD_ACCESS_REG *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_ACCESS_REG);

	if (info->ucCID != CMD_ID_ACCESS_REG ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ACCESS_REG *) info->pucInfoBuffer;

	if (UNI_IS_RFCR(cmd->u4Address))
		max_cmd_len += sizeof(struct UNI_CMD_ACCESS_RF_REG_BASIC);
	else
		max_cmd_len += sizeof(struct UNI_CMD_ACCESS_REG_BASIC);

	if (info->fgSetQuery)
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_ACCESS_REG,
				max_cmd_len, nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon);
	else
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_ACCESS_REG,
				max_cmd_len, nicUniCmdEventQueryMcrRead,
				nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_ACCESS_REG *) entry->pucInfoBuffer;
	if (UNI_IS_RFCR(cmd->u4Address)) {
		struct UNI_CMD_ACCESS_RF_REG_BASIC *tag =
		   (struct UNI_CMD_ACCESS_RF_REG_BASIC *) uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_ACCESS_REG_TAG_RF_REG_BASIC;
		tag->u2Length = sizeof(*tag);
		tag->u2WifiStream = UNI_STREAM_FROM_RFCR(cmd->u4Address);
		tag->u4Addr = cmd->u4Address;
		tag->u4Value = cmd->u4Data;
	} else {
		struct UNI_CMD_ACCESS_REG_BASIC *tag =
		      (struct UNI_CMD_ACCESS_REG_BASIC *) uni_cmd->aucTlvBuffer;

		tag->u2Tag = UNI_CMD_ACCESS_REG_TAG_BASIC;
		tag->u2Length = sizeof(*tag);
		tag->u4Addr = cmd->u4Address;
		tag->u4Value = cmd->u4Data;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

void nicUniCmdEventSetPhyCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_PHY_LIST_DUMP *phycr_evt =
		(struct UNI_EVENT_PHY_LIST_DUMP *)uni_evt->aucBuffer;
	struct UNI_EVENT_PHY_LIST_DUMP_CR *tag =
		(struct UNI_EVENT_PHY_LIST_DUMP_CR *) phycr_evt->aucTlvBuffer;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	DBGLOG(NIC, DEBUG, "PhyCtrl List Size: %ld", tag->u4ListSize);
	nicCmdEventSetCommon(prAdapter, prCmdInfo, pucEventBuf);
}

uint32_t nicUniCmdUpdateEdcaSet(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_UPDATE_WMM_PARMS *cmd;
	struct UNI_CMD_EDCA *uni_cmd;
	struct UNI_CMD_EDCA_AC_PARM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_EDCA) +
			sizeof(struct UNI_CMD_EDCA_AC_PARM) * WMM_AC_INDEX_NUM;
	uint8_t i;

	if (info->ucCID != CMD_ID_UPDATE_WMM_PARMS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_UPDATE_WMM_PARMS *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_EDCA_SET,
					max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_EDCA *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_EDCA_AC_PARM *) uni_cmd->aucTlvBuffer;
	for (i = 0; i < WMM_AC_INDEX_NUM; ++i) {
		tag->u2Tag = UNI_CMD_EDCA_TAG_AC_PARM;
		tag->u2Length = sizeof(*tag);
		tag->ucAcIndex = i;
		tag->ucValidBitmap = UNI_CMD_EDCA_ALL_BITS;
		tag->ucCWmin = (uint8_t) kalFfs(
			cmd->arACQueParms[i].u2CWmin + 1) - 1;
		tag->ucCWmax = (uint8_t) kalFfs(
			cmd->arACQueParms[i].u2CWmax + 1) - 1;
		tag->u2TxopLimit = cmd->arACQueParms[i].u2TxopLimit;
		tag->ucAifsn = (uint8_t) cmd->arACQueParms[i].u2Aifsn;
		tag++;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdUpdateMuEdca(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_802_11AX == 1)
	struct _CMD_MQM_UPDATE_MU_EDCA_PARMS_T *cmd;
	struct UNI_CMD_MQM_UPDATE_MU_EDCA *uni_cmd;
	struct UNI_CMD_UPDATE_MU_EDCA *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MQM_UPDATE_MU_EDCA) +
	     		       sizeof(struct UNI_CMD_UPDATE_MU_EDCA);
	uint8_t i;

	if (info->ucCID != CMD_ID_MQM_UPDATE_MU_EDCA_PARMS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _CMD_MQM_UPDATE_MU_EDCA_PARMS_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MQM_UPDATE_MU_EDCA_PARMS,
			max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MQM_UPDATE_MU_EDCA *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_UPDATE_MU_EDCA *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MQM_UPDATE_MU_EDCA_TAG_PARMS;
	tag->u2Length = sizeof(*tag);
	tag->ucBssIndex = cmd->ucBssIndex;
	tag->fgIsQBSS = cmd->fgIsQBSS;
	tag->ucWmmSet = cmd->ucWmmSet;

	for (i = 0; i < 4; i++) {
		tag->arMUEdcaParams[i].ucECWmin =
			cmd->arMUEdcaParams[i].ucECWmin;
		tag->arMUEdcaParams[i].ucECWmax =
			cmd->arMUEdcaParams[i].ucECWmax;
		tag->arMUEdcaParams[i].ucAifsn =
			cmd->arMUEdcaParams[i].ucAifsn;
		tag->arMUEdcaParams[i].ucIsACMSet =
			cmd->arMUEdcaParams[i].ucIsACMSet;
		tag->arMUEdcaParams[i].ucMUEdcaTimer =
			cmd->arMUEdcaParams[i].ucMUEdcaTimer;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdUpdateSrParams(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_802_11AX == 1)
	struct _CMD_RLM_UPDATE_SR_PARMS_T *cmd;
	struct UNI_CMD_SR *uni_cmd;
	struct UNI_CMD_SR_UPDATE_SR_PARMS *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SR) +
	     		       sizeof(struct UNI_CMD_SR_UPDATE_SR_PARMS);

	if (info->ucCID != CMD_ID_RLM_UPDATE_SR_PARAMS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _CMD_RLM_UPDATE_SR_PARMS_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SR, max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SR *) entry->pucInfoBuffer;
	uni_cmd->u1BandIdx = ENUM_BAND_AUTO;
	tag = (struct UNI_CMD_SR_UPDATE_SR_PARMS *) uni_cmd->au1TlvBuffer;
	tag->u2Tag = UNI_CMD_SR_TAG_UPDATE_SR_PARAMS;
	tag->u2Length = sizeof(*tag);
	tag->ucCmdVer = cmd->ucCmdVer;
	tag->u2CmdLen = cmd->u2CmdLen;
	tag->ucBssIndex = cmd->ucBssIndex;
	tag->ucSRControl = cmd->ucSRControl;
	tag->ucNonSRGObssPdMaxOffset = cmd->ucNonSRGObssPdMaxOffset;
	tag->ucSRGObssPdMinOffset = cmd->ucSRGObssPdMinOffset;
	tag->ucSRGObssPdMaxOffset = cmd->ucSRGObssPdMaxOffset;
	tag->u4SRGBSSColorBitmapLow = cmd->u4SRGBSSColorBitmapLow;
	tag->u4SRGBSSColorBitmapHigh = cmd->u4SRGBSSColorBitmapHigh;
	tag->u4SRGPartialBSSIDBitmapLow = cmd->u4SRGPartialBSSIDBitmapLow;
	tag->u4SRGPartialBSSIDBitmapHigh = cmd->u4SRGPartialBSSIDBitmapHigh;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdOffloadIPV4(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_NETWORK_ADDRESS_LIST *cmd;
	struct UNI_CMD_OFFLOAD *uni_cmd;
	struct UNI_CMD_OFFLOAD_ARPNS_IPV4 *tag;
	struct IPV4_ADDRESS *ipv4;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len;
	uint8_t i;

	if (info->ucCID != CMD_ID_SET_IP_ADDRESS)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_NETWORK_ADDRESS_LIST *) info->pucInfoBuffer;
	max_cmd_len = sizeof(struct UNI_CMD_OFFLOAD) +
		      sizeof(struct UNI_CMD_OFFLOAD_ARPNS_IPV4) +
		      sizeof(struct IPV4_ADDRESS) * cmd->ucAddressCount;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_OFFLOAD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_OFFLOAD *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_OFFLOAD_ARPNS_IPV4 *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_OFFLOAD_TAG_ARPNS_IPV4;
	tag->u2Length = sizeof(*tag) +
			sizeof(struct IPV4_ADDRESS) * cmd->ucAddressCount;
	tag->ucIpv4AddressCount = cmd->ucAddressCount;
	tag->ucVersion = cmd->ucVersion;
	ipv4 = tag->arIpv4NetAddress;
	for (i = 0; i < cmd->ucAddressCount; i++) {
		kalMemCopy(ipv4, &cmd->arNetAddress[i], sizeof(*ipv4));
		ipv4++;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdOffloadIPV6(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_IPV6_NETWORK_ADDRESS_LIST *cmd;
	struct UNI_CMD_OFFLOAD *uni_cmd;
	struct UNI_CMD_OFFLOAD_ARPNS_IPV6 *tag;
	struct IPV6_ADDRESS *ipv6;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len;
	uint8_t i;

	if (info->ucCID != CMD_ID_SET_IPV6_ADDRESS)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_IPV6_NETWORK_ADDRESS_LIST *) info->pucInfoBuffer;
	max_cmd_len = sizeof(struct UNI_CMD_OFFLOAD) +
		      sizeof(struct UNI_CMD_OFFLOAD_ARPNS_IPV6) +
		      sizeof(struct IPV6_ADDRESS) * cmd->ucAddressCount;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_OFFLOAD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_OFFLOAD *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_OFFLOAD_ARPNS_IPV6 *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_OFFLOAD_TAG_ARPNS_IPV6;
	tag->u2Length = sizeof(*tag) +
			sizeof(struct IPV6_ADDRESS) * cmd->ucAddressCount;
	tag->ucIpv6AddressCount = cmd->ucAddressCount;
	ipv6 = tag->arIpv6NetAddress;
	for (i = 0; i < cmd->ucAddressCount; i++) {
		kalMemCopy(ipv6, &cmd->arNetAddress[i], sizeof(*ipv6));
		ipv6++;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGetIdcChnl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_GET_LTE_SAFE_CHN *cmd;
	struct UNI_CMD_IDC *uni_cmd;
	struct UNI_CMD_GET_IDC_CHN *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_IDC) +
	     		       sizeof(struct UNI_CMD_GET_IDC_CHN);

	if (info->ucCID != CMD_ID_GET_LTE_CHN ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_GET_LTE_SAFE_CHN *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_IDC,
		max_cmd_len, nicUniEventQueryIdcChnl, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_IDC *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_GET_IDC_CHN *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_IDC_TAG_GET_IDC_CHN;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_IDC_RIL_BRIDGE
uint32_t nicUniCmdSetIdcRilBridge(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_IDC_RIL_BRIDGE *cmd;
	struct UNI_CMD_IDC *uni_cmd;
	struct UNI_CMD_RIL_BRIDGE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len =
		sizeof(struct UNI_CMD_IDC) +
		sizeof(struct UNI_CMD_RIL_BRIDGE);

	if (info->ucCID != CMD_ID_SET_IDC_RIL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_IDC_RIL_BRIDGE *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_IDC,
		max_cmd_len,
		NULL,
		NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_IDC *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_RIL_BRIDGE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_IDC_TAG_RIL_BRIDGE;
	tag->u2Length = sizeof(*tag);

	tag->ucRat = cmd->ucRat;
	tag->u4Band = cmd->u4Band;
	tag->u4Channel = cmd->u4Channel;

	DBGLOG(INIT, DEBUG,
		"Update CP channel info [%d,%d,%d]\n",
		cmd->ucRat, cmd->u4Band, cmd->u4Channel);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_UWB_COEX
uint32_t nicUniCmdSetUwbCoexEnable(
	struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_UWB_COEX_ENABLE *cmd;
	struct UNI_CMD_UWB_COEX *uni_cmd;
	struct UNI_CMD_UWB_COEX_ENABLE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len =
		sizeof(struct UNI_CMD_UWB_COEX) +
		sizeof(struct UNI_CMD_UWB_COEX_ENABLE);

	if (info->ucCID != CMD_ID_SET_UWB_COEX_ENABLE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_UWB_COEX_ENABLE *)
		info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad,
		UNI_CMD_ID_UWB_COEX,
		max_cmd_len,
		NULL,
		NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_UWB_COEX *)
		entry->pucInfoBuffer;
	tag = (struct UNI_CMD_UWB_COEX_ENABLE *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_UWB_COEX_TAG_ENABLE;
	tag->u2Length = sizeof(*tag);

	tag->u4Enable = cmd->u4Enable;
	tag->u4StartCh = cmd->u4StartCh;
	tag->u4EndCh = cmd->u4EndCh;

	DBGLOG(INIT, DEBUG,
		"Enable, startch, endch = [%d,%d,%d]\n",
		tag->u4Enable,
		tag->u4StartCh,
		tag->u4EndCh);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetUwbCoexPrepare(
	struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_UWB_COEX_PREPARE *cmd;
	struct UNI_CMD_UWB_COEX *uni_cmd;
	struct UNI_CMD_UWB_COEX_PREPARE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len =
		sizeof(struct UNI_CMD_UWB_COEX) +
		sizeof(struct UNI_CMD_UWB_COEX_PREPARE);

	if (info->ucCID != CMD_ID_SET_UWB_COEX_PREPARE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_UWB_COEX_PREPARE *)
		info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad,
		UNI_CMD_ID_UWB_COEX,
		max_cmd_len,
		NULL,
		NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_UWB_COEX *)
		entry->pucInfoBuffer;
	tag = (struct UNI_CMD_UWB_COEX_PREPARE *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_UWB_COEX_TAG_SET_PREPARE_TIME;
	tag->u2Length = sizeof(*tag);

	tag->u4Time = cmd->u4Time;

	DBGLOG(INIT, DEBUG, "Prepare time = %d\n", tag->u4Time);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#endif

uint32_t nicUniCmdSetSGParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if CFG_SUPPORT_SMART_GEAR
	struct CMD_SMART_GEAR_PARAM *cmd;
	struct UNI_CMD_SMART_GEAR *uni_cmd;
	struct UNI_CMD_SMART_GEAR_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SMART_GEAR) +
	     		       sizeof(struct UNI_CMD_SMART_GEAR_PARAM);

	if (info->ucCID != CMD_ID_SG_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SMART_GEAR_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SMART_GEAR,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SMART_GEAR *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SMART_GEAR_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SMART_GEAR_TAG_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->ucSGEnable = cmd->ucSGEnable;
	tag->ucSGSpcCmd = cmd->ucSGSpcCmd;
	tag->ucNSSCap = cmd->ucNSSCap;
	tag->ucSGCfg = cmd->ucSGCfg;
	tag->ucSG24GFavorANT = cmd->ucSG24GFavorANT;
	tag->ucSG5GFavorANT = cmd->ucSG5GFavorANT;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

#if CFG_SUPPORT_WIFI_ICCM
uint32_t nicUniCmdIccmSetParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ICCM_INFO_T *cmd;
	struct UNI_CMD_POWER_METRICS *uni_cmd;
	struct UNI_CMD_ICCM_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_POWER_METRICS) +
			sizeof(struct UNI_CMD_ICCM_PARAM);

	if (info->ucCID != CMD_ID_SET_ICCM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ICCM_INFO_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_METRICS,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_METRICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_ICCM_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_ICCM_TAG_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->u4Enable = cmd->u4Enable;
	tag->u4EnablePrintFw = cmd->u4EnablePrintFw;
	tag->u4Value = cmd->u4Value;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_WIFI_POWER_METRICS
uint32_t nicUniCmdPowerMetricsStatSetParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_POWER_METRICS_INFO_T *cmd;
	struct UNI_CMD_POWER_METRICS *uni_cmd;
	struct UNI_CMD_POWER_METRICS_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_POWER_METRICS) +
			sizeof(struct UNI_CMD_POWER_METRICS_PARAM);

	if (info->ucCID != CMD_ID_POWER_METRICS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_POWER_METRICS_INFO_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_METRICS,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_METRICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_POWER_METRICS_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_POWER_METRICS_TAG_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->u4Enable = cmd->u4Enable;
	tag->u4Value = cmd->u4Value;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicUniCmdSetMonitor(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_MONITOR_SET_INFO *cmd;
	struct UNI_CMD_SNIFFER_MODE *uni_cmd;
	struct UNI_CMD_SNIFFER_MODE_ENABLE *tag0;
	struct UNI_CMD_SNIFFER_MODE_CONFIG *tag1;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len;

	cmd = (struct CMD_MONITOR_SET_INFO *) info->pucInfoBuffer;
	max_cmd_len = sizeof(struct UNI_CMD_SNIFFER_MODE) +
		sizeof(struct UNI_CMD_SNIFFER_MODE_ENABLE) +
		sizeof(struct UNI_CMD_SNIFFER_MODE_CONFIG);

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SNIFFER_MODE,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SNIFFER_MODE *) entry->pucInfoBuffer;
	uni_cmd->ucBandIdx = cmd->ucBandIdx;

	tag0 = (struct UNI_CMD_SNIFFER_MODE_ENABLE *) uni_cmd->aucTlvBuffer;
	tag0->u2Tag = UNI_CMD_SNIFFER_MODE_TAG_ENABLE;
	tag0->u2Length = sizeof(*tag0);
	tag0->ucSNEnable = cmd->ucEnable;

	tag1 = (struct UNI_CMD_SNIFFER_MODE_CONFIG *)
		(uni_cmd->aucTlvBuffer + tag0->u2Length);
	tag1->u2Tag = UNI_CMD_SNIFFER_MODE_TAG_CONFIG;
	tag1->u2Length = sizeof(*tag1);
	tag1->ucBand = cmd->ucBand;
	tag1->ucPriChannel = cmd->ucPriChannel;
	tag1->ucSco = cmd->ucSco;
	tag1->ucChannelWidth = cmd->ucChannelWidth;
	tag1->ucChannelS1 = cmd->ucChannelS1;
	tag1->ucChannelS2 = cmd->ucChannelS2;
	tag1->u2Aid = cmd->u2Aid;
	tag1->fgDropFcsErrorFrame = cmd->fgDropFcsErrorFrame;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_ROAMING
uint32_t nicUniCmdRoaming(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ROAMING_TRANSIT *cmd;
	struct UNI_CMD_ROAMING *uni_cmd;
	struct UNI_CMD_ROAMING_TRANSIT_FSM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_ROAMING) +
	     		       sizeof(struct UNI_CMD_ROAMING_TRANSIT_FSM);

	if (info->ucCID != CMD_ID_ROAMING_TRANSIT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ROAMING_TRANSIT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_ROAMING,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_ROAMING *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssidx;
	uni_cmd->ucDbdcIdx = ENUM_BAND_AUTO;
	tag = (struct UNI_CMD_ROAMING_TRANSIT_FSM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_ROAMING_TAG_TRANSIT_FSM;
	tag->u2Length = sizeof(*tag);
	tag->u2Event = cmd->u2Event;
	tag->u2Data = cmd->u2Data;
	tag->eReason = cmd->eReason;
	tag->u4RoamingTriggerTime = cmd->u4RoamingTriggerTime;
	tag->u2RcpiLowThreshold = cmd->u2RcpiLowThreshold;
	tag->fgIsAggressive = cmd->fgIsAggressive;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t nicUniCmdPerfInd(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_PERF_IND *cmd;
	struct UNI_CMD_PERF_IND *uni_cmd;
	struct UNI_CMD_PERF_IND_PARM *tag;
#if CFG_SUPPORT_TPUT_FACTOR
	struct UNI_CMD_PERF_IND_TPUT_FACTOR *tput_factor_tag;
#endif
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PERF_IND) +
#if CFG_SUPPORT_TPUT_FACTOR
			sizeof(struct UNI_CMD_PERF_IND_PARM) +
			sizeof(struct UNI_CMD_PERF_IND_TPUT_FACTOR);
#else
			sizeof(struct UNI_CMD_PERF_IND_PARM);
#endif

	if (info->ucCID != CMD_ID_PERF_IND ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_PERF_IND *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_PERF_IND,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_PERF_IND *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_PERF_IND_PARM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_PERF_IND_TAG_PARM_V2;
	tag->u2Length = sizeof(*tag);
	tag->ucCmdVer = cmd->ucCmdVer;
	tag->u2CmdLen = cmd->u2CmdLen;
	tag->u4VaildPeriod = cmd->u4VaildPeriod;
	tag->ucBssNum = ad->ucSwBssIdNum;

	kalMemCopy(tag->rUniCmdParm, cmd->rUniCmdParm,
				sizeof(tag->rUniCmdParm));

#if CFG_SUPPORT_TPUT_FACTOR
	tput_factor_tag = (struct UNI_CMD_PERF_IND_TPUT_FACTOR *)
		(uni_cmd->aucTlvBuffer + sizeof(*tag));
	tput_factor_tag->u2Tag = UNI_CMD_PERF_IND_TAG_TPUT_FACTOR;
	tput_factor_tag->u2Length = sizeof(*tput_factor_tag);
	tput_factor_tag->u4WtblBitMap = cmd->u4WtblBitMap;
#endif

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdInstallKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_802_11_KEY *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_INSTALL_KEY3 *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
	     		       sizeof(struct UNI_CMD_STAREC_INSTALL_KEY3);
	uint16_t widx = 0;

	if (info->ucCID != CMD_ID_ADD_REMOVE_KEY ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_802_11_KEY *) info->pucInfoBuffer;

	/* bigtk */
	if (cmd->ucKeyId >= 6 && cmd->ucKeyId <= 7)
		return nicUniCmdBcnProt(ad, info);

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO,
		max_cmd_len, cmd->ucAddRemove ? nicUniCmdEventInstallKey :
		nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIdx;
	widx = (uint16_t) cmd->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);
	tag = (struct UNI_CMD_STAREC_INSTALL_KEY3 *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_INSTALL_KEY_V3;
	tag->u2Length = sizeof(*tag);
	tag->ucAddRemove = cmd->ucAddRemove;
	tag->ucTxKey = cmd->ucTxKey;
	tag->ucKeyType = cmd->ucKeyType;
	tag->ucIsAuthenticator = cmd->ucIsAuthenticator;
	COPY_MAC_ADDR(tag->aucPeerAddr, cmd->aucPeerAddr);
	tag->ucBssIdx = cmd->ucBssIdx;
	tag->ucAlgorithmId = cmd->ucAlgorithmId;
	tag->ucKeyId = cmd->ucKeyId;
	tag->ucKeyLen = cmd->ucKeyLen;
	tag->ucWlanIndex = cmd->ucWlanIndex;
	tag->ucMgmtProtection = cmd->ucMgmtProtection;
	kalMemCopy(tag->aucKeyMaterial, cmd->aucKeyMaterial,
		sizeof(tag->aucKeyMaterial));
	kalMemCopy(tag->aucKeyRsc, cmd->aucKeyRsc, sizeof(tag->aucKeyRsc));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdInstallDefaultKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_DEFAULT_KEY *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_DEFAULT_KEY *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
	     		       sizeof(struct UNI_CMD_STAREC_DEFAULT_KEY);
	uint16_t widx = 0;

	if (info->ucCID != CMD_ID_DEFAULT_KEY_ID ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_DEFAULT_KEY *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIdx;
	widx = (uint16_t) cmd->ucWlanIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer, uni_cmd->ucWlanIdxL, widx);
	tag = (struct UNI_CMD_STAREC_DEFAULT_KEY *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_INSTALL_DEFAULT_KEY;
	tag->u2Length = sizeof(*tag);
	tag->ucKeyId = cmd->ucKeyId;
	tag->ucMulticast = cmd->ucMulticast;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdOffloadKey(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct PARAM_GTK_REKEY_DATA *cmd;
	struct UNI_CMD_OFFLOAD *uni_cmd;
	struct UNI_CMD_OFFLOAD_GTK_REKEY *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_OFFLOAD) +
	     		       sizeof(struct UNI_CMD_OFFLOAD_GTK_REKEY);

	if (info->ucCID != CMD_ID_SET_GTK_REKEY_DATA ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct PARAM_GTK_REKEY_DATA *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_OFFLOAD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_OFFLOAD *) entry->pucInfoBuffer;
	uni_cmd->ucBssInfoIdx = cmd->ucBssIndex;
	tag = (struct UNI_CMD_OFFLOAD_GTK_REKEY *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_OFFLOAD_TAG_GTK_REKEY;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(tag->aucKek, cmd->aucKek, sizeof(tag->aucKek));
	kalMemCopy(tag->aucKck, cmd->aucKck, sizeof(tag->aucKck));
	kalMemCopy(tag->aucReplayCtr,
		cmd->aucReplayCtr, sizeof(tag->aucReplayCtr));

	if (cmd->ucRekeyMode == GTK_REKEY_CMD_MODE_OFFLOAD_UPDATE) {
		tag->ucRekeyMode = GTK_REKEY_CMD_MODE_OFFLOAD_UPDATE;
		tag->ucOption = GTK_REKEY_UPDATE_ONLY;
	} else if (cmd->ucRekeyMode == GTK_REKEY_CMD_MODE_OFFLOAD_ON) {
		tag->ucRekeyMode = GTK_REKEY_CMD_MODE_OFFLOAD_UPDATE;
		tag->ucOption = GTK_REKEY_UPDATE_AND_ON;
	} else if (cmd->ucRekeyMode == GTK_REKEY_CMD_MODE_OFLOAD_OFF) {
		tag->ucRekeyMode = GTK_REKEY_CMD_MODE_OFLOAD_OFF;
	}

	tag->ucCurKeyId = 0;
	tag->u4Proto = cmd->u4Proto;
	tag->u4PairwiseCipher = cmd->u4PairwiseCipher;
	tag->u4GroupCipher = cmd->u4GroupCipher;
	tag->u4KeyMgmt = cmd->u4KeyMgmt;
	tag->u4MgmtGroupCipher = cmd->u4MgmtGroupCipher;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdHifCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_HIF_CTRL *cmd;
	struct UNI_CMD_HIF_CTRL *uni_cmd;
	struct UNI_CMD_HIF_CTRL_BASIC *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_HIF_CTRL) +
	     		       sizeof(struct UNI_CMD_HIF_CTRL_BASIC);

	if (info->ucCID != CMD_ID_HIF_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_HIF_CTRL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_HIF_CTRL,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_HIF_CTRL *) entry->pucInfoBuffer;
	uni_cmd->ucHifType = cmd->ucHifType;
	tag = (struct UNI_CMD_HIF_CTRL_BASIC *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_HIF_CTRL_TAG_BASIC;
	tag->u2Length = sizeof(*tag);
	tag->ucHifSuspend = cmd->ucHifSuspend;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdRddOnOffCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_DFS_MASTER == 1)
	struct CMD_RDD_ON_OFF_CTRL *cmd;
	struct UNI_CMD_RDD *uni_cmd;
	struct UNI_CMD_RDD_ON_OFF_CTRL_PARM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RDD) +
			       sizeof(struct UNI_CMD_RDD_ON_OFF_CTRL_PARM);

	if (info->ucCID != CMD_ID_RDD_ON_OFF_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_RDD_ON_OFF_CTRL *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RDD_ON_OFF_CTRL,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RDD *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_RDD_ON_OFF_CTRL_PARM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RDD_TAG_ON_OFF_CTRL_PARM;
	tag->u2Length = sizeof(*tag);
	tag->u1DfsCtrl = cmd->ucDfsCtrl;
	tag->u1RddIdx = cmd->ucRddIdx;
	tag->u1RddRxSel = cmd->ucRddRxSel;
	tag->ucBssIdx = cmd->ucBssIdx;
	tag->u1SetVal = cmd->ucSetVal;
	tag->ucCacLPEn = cmd->ucCacLPEn;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdTdls(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_TDLS == 1)
	struct CMD_TDLS_CH_SW *cmd;
	struct UNI_CMD_TDLS *uni_cmd;
	struct UNI_CMD_SET_TDLS_CH_SW *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TDLS) +
	     		       sizeof(struct UNI_CMD_SET_TDLS_CH_SW);

	if (info->ucCID != CMD_ID_SET_TDLS_CH_SW ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_TDLS_CH_SW *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TDLS,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TDLS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_TDLS_CH_SW *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TDLS_TAG_SET_TDLS_CH_SW;
	tag->u2Length = sizeof(*tag);
	tag->fgIsTDLSChSwProhibit = cmd->fgIsTDLSChSwProhibit;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdSetP2pNoa(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_CUSTOM_NOA_PARAM_STRUCT *cmd;
	struct UNI_CMD_P2P *uni_cmd;
	struct UNI_CMD_SET_NOA_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_P2P) +
			       sizeof(struct UNI_CMD_SET_NOA_PARAM);

	if (info->ucCID != CMD_ID_SET_NOA_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CUSTOM_NOA_PARAM_STRUCT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_P2P,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_P2P *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_NOA_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_P2P_TAG_SET_NOA_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->u4NoaDurationMs = cmd->u4NoaDurationMs;
	tag->u4NoaIntervalMs = cmd->u4NoaIntervalMs;
	tag->u4NoaCount = cmd->u4NoaCount;
	tag->ucBssIdx = cmd->ucBssIdx;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetP2pOppps(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_CUSTOM_OPPPS_PARAM_STRUCT *cmd;
	struct UNI_CMD_P2P *uni_cmd;
	struct UNI_CMD_SET_OPPPS_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_P2P) +
			       sizeof(struct UNI_CMD_SET_OPPPS_PARAM);

	if (info->ucCID != CMD_ID_SET_OPPPS_PARAM ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CUSTOM_OPPPS_PARAM_STRUCT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_P2P,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_P2P *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_OPPPS_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_P2P_TAG_SET_OPPPS_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->u4CTwindowMs = cmd->u4CTwindowMs;
	tag->ucBssIdx = cmd->ucBssIdx;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetP2pLoStart(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_P2P_LO_START_STRUCT *cmd;
	struct UNI_CMD_P2P *uni_cmd;
	struct UNI_CMD_SET_P2P_LO_START_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_P2P) +
		sizeof(struct UNI_CMD_SET_P2P_LO_START_PARAM);

	if (info->ucCID != CMD_ID_SET_P2P_LO_START)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_P2P_LO_START_STRUCT *)
		info->pucInfoBuffer;
	max_cmd_len += cmd->u4IELen;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_P2P,
		max_cmd_len, nicUniCmdEventSetCommon,
		nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_P2P *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_P2P_LO_START_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_P2P_TAG_SET_LO_START;
	tag->u2Length = sizeof(*tag);
	tag->ucBssIndex = cmd->ucBssIndex;
	tag->u2ListenPrimaryCh = cmd->u2ListenPrimaryCh;
	tag->u2Period = cmd->u2Period;
	tag->u2Interval = cmd->u2Interval;
	tag->u2Count = cmd->u2Count;
	tag->u4IELen = cmd->u4IELen;
	kalMemCopy(tag->aucIE, cmd->aucIE, tag->u4IELen);

	DBGLOG(INIT, DEBUG,
		"p2p_lo, b: %d, c: %d, p: %d, i: %d, count: %d\n",
		tag->ucBssIndex,
		tag->u2ListenPrimaryCh,
		tag->u2Period,
		tag->u2Interval,
		tag->u2Count);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetP2pLoStop(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_P2P_LO_STOP_STRUCT *cmd;
	struct UNI_CMD_P2P *uni_cmd;
	struct UNI_CMD_SET_P2P_LO_STOP_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_P2P) +
		sizeof(struct UNI_CMD_SET_P2P_LO_STOP_PARAM);

	if (info->ucCID != CMD_ID_SET_P2P_LO_STOP)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_P2P_LO_STOP_STRUCT *)
		info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_P2P,
		max_cmd_len, nicUniCmdEventSetCommon,
		nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_P2P *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_P2P_LO_STOP_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_P2P_TAG_SET_LO_STOP;
	tag->u2Length = sizeof(*tag);
	tag->ucBssIndex = cmd->ucBssIndex;

	DBGLOG(REQ, DEBUG, "p2p_lo stop: %d\n", tag->ucBssIndex);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetApConstraintPwrLimit(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if CFG_SUPPORT_802_11K
	struct CMD_SET_AP_CONSTRAINT_PWR_LIMIT *cmd;
	struct UNI_CMD_RRM_11K *uni_cmd;
	struct UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RRM_11K) +
		sizeof(struct UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT_PARAM);

	if (info->ucCID != CMD_ID_SET_AP_CONSTRAINT_PWR_LIMIT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_AP_CONSTRAINT_PWR_LIMIT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RRM_11K,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RRM_11K *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SET_AP_CONSTRAINT_PWR_LIMIT;
	tag->u2Length = sizeof(*tag);
	tag->ucPwrSetEnable = cmd->ucPwrSetEnable;
	tag->cMaxTxPwr = cmd->cMaxTxPwr;
	tag->cMinTxPwr = cmd->cMinTxPwr;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t nicUniCmdSetRrmCapability(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if CFG_SUPPORT_802_11K
	struct CMD_SET_RRM_CAPABILITY *cmd;
	struct UNI_CMD_RRM_11K *uni_cmd;
	struct UNI_CMD_SET_RRM_CAPABILITY_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RRM_11K) +
		sizeof(struct UNI_CMD_SET_RRM_CAPABILITY_PARAM);

	if (info->ucCID != CMD_ID_SET_RRM_CAPABILITY ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_RRM_CAPABILITY *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RRM_11K,
		max_cmd_len, NULL, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RRM_11K *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_SET_RRM_CAPABILITY_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SET_RRM_CAPABILITY;
	tag->u2Length = sizeof(*tag);
	tag->ucRrmEnable = cmd->ucRrmEnable;
	kalMemCopy(tag->ucCapabilities, cmd->ucCapabilities,
		sizeof(tag->ucCapabilities));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif

}

uint32_t nicUniCmdSetCountryPwrLimit(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *cmd;
	struct UNI_CMD_POWER_LIMIT *uni_cmd;
	struct UNI_CMD_SET_PWR_LIMIT_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_POWER_LIMIT) +
		sizeof(struct UNI_CMD_SET_PWR_LIMIT_PARAM);
	uint8_t tag_id = 0;

	cmd = (struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *)
		info->pucInfoBuffer;
	tag_id = cmd->ucCategoryId;

	if (info->ucCID != CMD_ID_SET_COUNTRY_POWER_LIMIT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd) ||
	    (tag_id != POWER_LIMIT_TABLE_CTRL &&
		tag_id != POWER_LIMIT_TX_PWR_ENV_CTRL))
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_LIMIT,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_LIMIT *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_SET_PWR_LIMIT_PARAM *)
		uni_cmd->aucTlvBuffer;
	if (tag_id == POWER_LIMIT_TABLE_CTRL)
		tag->u2Tag = UNI_CMD_POWER_LIMIT_TABLE_CTRL;
	else if (tag_id == POWER_LIMIT_TX_PWR_ENV_CTRL)
		tag->u2Tag = UNI_CMD_POWER_LIMIT_TX_PWR_ENV;

	tag->u2Length = sizeof(*tag);
	kalMemCopy(&tag->config, cmd, sizeof(tag->config));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetCountryPwrLimitPerRate(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *cmd;
	struct UNI_CMD_POWER_LIMIT *uni_cmd;
	struct UNI_CMD_SET_PWR_LIMIT_PER_RATE_TABLE_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

#if 1 /* check max_cmd_len length */
	uint32_t max_cmd_len =
		sizeof(struct UNI_CMD_POWER_LIMIT) +
		sizeof(uint16_t) + sizeof(uint16_t) +
		info->u4SetQueryInfoLen;
#else
	uint32_t max_cmd_len =
		sizeof(struct UNI_CMD_POWER_LIMIT)+
		sizeof(struct UNI_CMD_SET_PWR_LIMIT_PER_RATE_TABLE_PARAM);
#endif

#if 1
	if (info->ucCID != CMD_ID_SET_COUNTRY_POWER_LIMIT_PER_RATE)
		return WLAN_STATUS_NOT_ACCEPTED;
#else
	if (info->ucCID != CMD_ID_SET_COUNTRY_POWER_LIMIT_PER_RATE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;
#endif

	cmd = (struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *)
		info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_LIMIT,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_LIMIT *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_SET_PWR_LIMIT_PER_RATE_TABLE_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_POWER_LIMIT_PER_RATE_TABLE;

#if 1
	tag->u2Length = sizeof(tag->u2Tag) +
					sizeof(tag->u2Length) +
					info->u4SetQueryInfoLen;

	kalMemCopy(&tag->config, cmd, info->u4SetQueryInfoLen);
	DBGLOG_MEM8(INIT, DEBUG, &tag->config, info->u4SetQueryInfoLen);
#else
	tag->u2Length = sizeof(*tag);
#endif
	kalMemCopy(&tag->config, cmd, sizeof(tag->config));
	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdNvramFragmentHandler(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_NVRAM_FRAGMENT *cmd;
	struct UNI_CMD_NVRAM_SETTINGS *uni_cmd;
	struct UNI_CMD_NVRAM_SETTINGS_FRAGMENT_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_NVRAM_SETTINGS) +
		sizeof(struct UNI_CMD_NVRAM_SETTINGS_FRAGMENT_PARAM);

	if (info->ucCID != CMD_ID_SET_NVRAM_SETTINGS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_NVRAM_FRAGMENT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_NVRAM_SETTINGS,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_NVRAM_SETTINGS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_NVRAM_SETTINGS_FRAGMENT_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_NVRAM_SETTINGS_FRAGMENT;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(&tag->config, cmd, sizeof(tag->config));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdNvramLegacyHandler(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_NVRAM_SETTING *cmd;
	struct UNI_CMD_NVRAM_SETTINGS *uni_cmd;
	struct UNI_CMD_NVRAM_SETTINGS_LEGACY_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_NVRAM_SETTINGS) +
		sizeof(struct UNI_CMD_NVRAM_SETTINGS_LEGACY_PARAM);

	if (info->ucCID != CMD_ID_SET_NVRAM_SETTINGS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_NVRAM_SETTING *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_NVRAM_SETTINGS,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_NVRAM_SETTINGS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_NVRAM_SETTINGS_LEGACY_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_NVRAM_SETTINGS_LEGACY;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(&tag->config, cmd, sizeof(tag->config));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetNvramSettings(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	uint8_t fgIsFragCmd = !!(ad->chip_info->is_support_nvram_fragment);

	DBGLOG(RX, LOUD, "fgIsFragCmd[%d]", fgIsFragCmd);

	if (fgIsFragCmd)
		return nicUniCmdNvramFragmentHandler(ad, info);
	else
		return nicUniCmdNvramLegacyHandler(ad, info);
}

/* reset TX scramble seed */
uint32_t wlanResetTxScrambleSeed(struct ADAPTER *ad, uint8_t ucHwBandIdx)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_RESET_TX_SCRAMBLE *uni_cmd;
	struct UNI_CMD_RESET_TX_SCRAMBLE_PARAM *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RESET_TX_SCRAMBLE) +
			       sizeof(struct UNI_CMD_RESET_TX_SCRAMBLE_PARAM);

	uni_cmd = (struct UNI_CMD_RESET_TX_SCRAMBLE *)cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_RESET_TX_SCRAMBLE FAILED\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_RESET_TX_SCRAMBLE_PARAM *)uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RESET_TX_SCRAMBLE_TAG;
	tag->u2Length = sizeof(*tag);
	tag->ucDbdcIdx = ucHwBandIdx;

	status = wlanSendSetQueryUniCmd(ad,
			     UNI_CMD_ID_RESET_TX_SCRAMBLE,
			     TRUE,
			     FALSE,
			     FALSE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (uint8_t *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);

	return status;
}

uint32_t nicUniCmdSetTxAmpdu(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_TX_AMPDU *cmd;
	struct UNI_CMD_BA_OFFLOAD *uni_cmd;
	struct UNI_CMD_TX_AMPDU_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BA_OFFLOAD) +
		sizeof(struct UNI_CMD_TX_AMPDU_PARAM);

	if (info->ucCID != CMD_ID_TX_AMPDU ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_TX_AMPDU *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BA_OFFLOAD,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BA_OFFLOAD *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_TX_AMPDU_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BA_OFFLOAD_TAG_TX_AMPDU;
	tag->u2Length = sizeof(*tag);
	tag->fgEnable = cmd->fgEnable;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetRxAmpdu(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ADDBA_REJECT *cmd;
	struct UNI_CMD_BA_OFFLOAD *uni_cmd;
	struct UNI_CMD_RX_AMPDU_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_BA_OFFLOAD) +
		sizeof(struct UNI_CMD_RX_AMPDU_PARAM);

	if (info->ucCID != CMD_ID_ADDBA_REJECT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ADDBA_REJECT *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_BA_OFFLOAD,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_BA_OFFLOAD *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_RX_AMPDU_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_BA_OFFLOAD_TAG_RX_AMPDU;
	tag->u2Length = sizeof(*tag);
	/* Legacy ADDBA reject true equal to UNI CMD RX AMPDU false */
	tag->fgEnable = !cmd->fgEnable;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetMultiAddr(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_MAC_MCAST_ADDR *cmd;
	struct UNI_CMD_MUAR *uni_cmd;
	struct UNI_CMD_MUAR_CLEAN_PARAM *clean_tag;
	struct UNI_CMD_MUAR_ENTRY_PARAM *config_tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MUAR) +
		sizeof(struct UNI_CMD_MUAR_CLEAN_PARAM);
	uint8_t ucIdx = 0;

	if (info->ucCID != CMD_ID_MAC_MCAST_ADDR ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_MAC_MCAST_ADDR *) info->pucInfoBuffer;
	max_cmd_len +=
		sizeof(struct UNI_CMD_MUAR_ENTRY_PARAM) * cmd->u4NumOfGroupAddr;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_NORM_MUAR,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MUAR *) entry->pucInfoBuffer;
	uni_cmd->ucBand = ENUM_BAND_AUTO;

	DBGLOG(RX, TRACE, "clean_tag[%p]", uni_cmd->aucTlvBuffer);

	/* Fill-in clean tag*/
	clean_tag = (struct UNI_CMD_MUAR_CLEAN_PARAM *) uni_cmd->aucTlvBuffer;
	clean_tag->u2Tag = UNI_CMD_MUAR_TAG_CLEAN;
	clean_tag->u2Length = sizeof(*clean_tag);
	clean_tag->ucHwBssIndex = cmd->ucBssIndex;
	clean_tag++;

	DBGLOG(RX, TRACE, "Number of address[%d] config_tag[%p]",
		cmd->u4NumOfGroupAddr, clean_tag);

	/* Fill-in config tag*/
	config_tag = (struct UNI_CMD_MUAR_ENTRY_PARAM *) clean_tag;
	for (ucIdx = 0; ucIdx < cmd->u4NumOfGroupAddr; ucIdx++, config_tag++) {
		config_tag->u2Tag = UNI_CMD_MUAR_TAG_ENTRY;
		config_tag->u2Length = sizeof(*config_tag);
		config_tag->fgSmesh = FALSE;
		config_tag->ucHwBssIndex = cmd->ucBssIndex;
		config_tag->ucMuarIdx = 0xFF;
		config_tag->ucEntryAdd = TRUE;
		COPY_MAC_ADDR(config_tag->aucMacAddr, cmd->arAddress[ucIdx]);
	}
	DBGLOG_MEM8(NIC, TRACE, uni_cmd->aucTlvBuffer,
		(sizeof(struct UNI_CMD_MUAR_CLEAN_PARAM) +
		sizeof(struct UNI_CMD_MUAR_ENTRY_PARAM) * cmd->u4NumOfGroupAddr));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetRssiMonitor(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct PARAM_RSSI_MONITOR_T *cmd;
	struct UNI_CMD_RSSI_MONITOR *uni_cmd;
	struct UNI_CMD_RSSI_MONITOR_SET *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RSSI_MONITOR) +
		sizeof(struct UNI_CMD_RSSI_MONITOR_SET);

	if (info->ucCID != CMD_ID_RSSI_MONITOR ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct PARAM_RSSI_MONITOR_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RSSI_MONITOR,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RSSI_MONITOR *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_RSSI_MONITOR_SET *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RSSI_MONITOR_TAG_SET;
	tag->u2Length = sizeof(*tag);
	tag->fgEnable = cmd->enable;
	tag->cMaxRssi = cmd->max_rssi_value;
	tag->cMinRssi = cmd->min_rssi_value;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;

}

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
uint32_t nicUniCmdSetIcsSniffer(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ICS_SNIFFER_INFO *cmd;
	struct UNI_CMD_ICS *uni_cmd;
	struct UNI_CMD_ICS_SNIFFER *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_ICS) +
		sizeof(struct UNI_CMD_ICS_SNIFFER);

	if (info->ucCID != CMD_ID_SET_ICS_SNIFFER ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ICS_SNIFFER_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_ICS,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_ICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_ICS_SNIFFER *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_ICS_TAG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucAction = cmd->ucAction;
	tag->ucModule = cmd->ucModule;
	tag->ucFilter = cmd->ucFilter;
	tag->ucOperation = cmd->ucOperation;
	kalMemCopy(&tag->ucCondition, &cmd->ucCondition,
		sizeof(tag->ucCondition));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_PHY_ICS == 1)
uint32_t nicUniCmdSetPhyIcsEventOn(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_PHY_ICS_EVENT_INFO *cmd;
	struct UNI_CMD_PHY_ICS *uni_cmd;
	struct UNI_CMD_PHY_ICS_EVENT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PHY_ICS) +
		sizeof(struct UNI_CMD_PHY_ICS_EVENT);

	if (info->ucCID != CMD_ID_SET_PHY_ICS_EVENT ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_PHY_ICS_EVENT_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_PHY_ICS,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_PHY_ICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_PHY_ICS_EVENT *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_PHY_ICS_EVENT_TAG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucBandIdx = cmd->ucBandIdx;
	tag->ucPartition = cmd->ucPartition;
	tag->u2EventGroup = cmd->u2EventGroup;
	tag->u4EventID = cmd->u4EventID;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetPhyIcsStart(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_PHY_ICS_START_INFO *cmd;
	struct UNI_CMD_PHY_ICS *uni_cmd;
	struct UNI_CMD_PHY_ICS_START *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PHY_ICS) +
		sizeof(struct UNI_CMD_PHY_ICS_START);

	if (info->ucCID != CMD_ID_SET_PHY_ICS_START ||
		info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_PHY_ICS_START_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_PHY_ICS,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_PHY_ICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_PHY_ICS_START *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_PHY_ICS_START_TAG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->u2Action = cmd->u2Action;
	tag->u2Timer = cmd->u2Timer;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_PHY_ICS */

uint32_t nicUniCmdACLPolicy(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_ACL_POLICY *cmd;
	struct UNI_CMD_ACS_POLICY *uni_cmd;
	struct UNI_CMD_ACS_POLICY_SETTING *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_ACS_POLICY) +
		sizeof(struct UNI_CMD_ACS_POLICY_SETTING);

	if (info->ucCID != CMD_ID_SET_ACL_POLICY ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_ACL_POLICY *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_ACL_POLICY,
		max_cmd_len, NULL, NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_ACS_POLICY *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_ACS_POLICY_SETTING *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_ACS_POLICY_TAG_SETTING;
	tag->u2Length = sizeof(*tag);
	tag->ucBssIdx = cmd->ucBssIdx;
	tag->ucPolicy = cmd->ucPolicy;
	COPY_MAC_ADDR(tag->aucAddr, cmd->aucAddr);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdMibInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct PARAM_HW_MIB_INFO *prParamMibInfo = {0};
	struct UNI_CMD_MIB_INFO *uni_cmd;
	struct UNI_CMD_MIB_DATA *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t i, count = 0;
	uint32_t max_cmd_len;

	_Static_assert(MAX_UNI_CMD_MIB_NUM >= UNI_CMD_MIB_CNT_MAX_NUM,
		"MAX_UNI_CMD_MIB_NUM should >= UNI_CMD_MIB_CNT_MAX_NUM");

	if (info->ucCID != CMD_ID_MIB_INFO ||
	    info->u4SetQueryInfoLen != sizeof(*prParamMibInfo))
		return WLAN_STATUS_NOT_ACCEPTED;

	/* make sure prParamMibInfo->au4TagBitmap is enough
	 * to present all mib idx
	 */
	prParamMibInfo = (struct PARAM_HW_MIB_INFO *) info->pucInfoBuffer;
	if (prParamMibInfo->u2TagCount > MAX_MIB_TAG_CNT)
		return WLAN_STATUS_INVALID_DATA;

	max_cmd_len = sizeof(struct UNI_CMD_MIB_INFO) +
		sizeof(struct UNI_CMD_MIB_DATA) * prParamMibInfo->u2TagCount;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MIB,
		max_cmd_len, nicUniEventMibInfo, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MIB_INFO *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MIB_DATA *) uni_cmd->aucTlvBuffer;
	uni_cmd->ucBand = (uint8_t)prParamMibInfo->u2Index;

	for (i = 0; i < UNI_CMD_MIB_CNT_MAX_NUM; i++) {
		if (IS_SET_BITMAP(prParamMibInfo->au4TagBitmap, i)) {
			tag->u2Tag = UNI_CMD_MIB_DATA_TAG;
			tag->u2Length = sizeof(*tag);
			tag->u4Counter = i;
			DBGLOG(REQ, LOUD, "mibIdx:%u len:%u\n",
				i, tag->u2Length);
			tag += 1;
			count += 1;
		}
		if (count == prParamMibInfo->u2TagCount)
			break;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGetStaStatistics(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_GET_STA_STATISTICS *cmd;
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_STA_STATISTICS *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
			       sizeof(struct UNI_CMD_STA_STATISTICS);

	if (info->ucCID != CMD_ID_GET_STA_STATISTICS ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_GET_STA_STATISTICS *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len, nicUniEventStaStatistics, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_STA_STATISTICS *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_STA;
	tag->u2Length = sizeof(*tag);
	tag->u1Index = secGetWlanIdxByStaIdx(ad, cmd->ucIndex);
	tag->ucReadClear = cmd->ucReadClear;
	tag->ucLlsReadClear = cmd->ucLlsReadClear;
	tag->ucResetCounter = cmd->ucResetCounter;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGetStatistics(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_BASIC_STATISTICS *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
			       sizeof(struct UNI_CMD_BASIC_STATISTICS);

	if (info->ucCID != CMD_ID_GET_STATISTICS)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len, nicUniEventStatistics, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_BASIC_STATISTICS *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BASIC;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdBeaconReport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_BEACON_REPORT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
			       sizeof(struct UNI_CMD_BEACON_REPORT);
	struct CMD_SET_REPORT_BEACON_STRUCT *cmd =
		(struct CMD_SET_REPORT_BEACON_STRUCT *)
		info->pucInfoBuffer;

	if (info->ucCID != CMD_ID_SET_REPORT_BEACON)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad,
		UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len,
		nicUniCmdEventSetCommon,
		nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_BEACON_REPORT *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BEACON_REPORT;
	tag->u2Length = sizeof(*tag);
	tag->ucEnable = cmd->ucReportBcnEn;

	DBGLOG(SCN, INFO, "Set: %d\n", cmd->ucReportBcnEn);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGetLinkQuality(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_LINK_QUALITY *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
	     		       sizeof(struct UNI_CMD_LINK_QUALITY);

	if (info->ucCID != CMD_ID_GET_LINK_QUALITY)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len, nicUniEventLinkQuality, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_LINK_QUALITY *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_LINK_QUALITY;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

/**
 * This function handles the command conversion for legacy multiple functional
 * command CMD_ID_GET_STATS_LLS(0x84).
 * Tags embedded in cmd will be set in the unified command tag field with new
 * values.
 */
uint32_t nicUniCmdGetLinkStats(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	uint32_t status = WLAN_STATUS_NOT_SUPPORTED;
#if CFG_SUPPORT_LLS
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_LINK_LAYER_STATS *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
			       sizeof(struct UNI_CMD_LINK_LAYER_STATS);
	struct CMD_GET_STATS_LLS *cmd =
		(struct CMD_GET_STATS_LLS *)info->pucInfoBuffer;

	if (info->ucCID != CMD_ID_GET_STATS_LLS)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len, nicUniEventLinkStats, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_LINK_LAYER_STATS *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = cmd->u4Tag + UNI_CMD_GET_STATISTICS_TAG_LINK_LAYER_STATS;
	tag->u2Length = sizeof(*tag);
	tag->ucArg0 = cmd->ucArg0;
	tag->ucArg1 = cmd->ucArg1;
	tag->ucArg2 = cmd->ucArg2;
	tag->ucArg3 = cmd->ucArg3;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	status = WLAN_STATUS_SUCCESS;
#endif
	return status;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
uint32_t nicUniCmdQueryEmlInfo(struct ADAPTER *ad,
		void *pvQueryBuffer,
		uint32_t u4QueryBufferLen)
{
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_EML_INFO *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
				sizeof(struct UNI_CMD_EML_INFO);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_GET_STATISTICS ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}
	tag = (struct UNI_CMD_EML_INFO *)uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_EML_STATS;
	tag->u2Length = sizeof(*tag);

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_GET_STATISTICS,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventEmlInfo,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);

	return status;
}

#if (CFG_SUPPORT_MLC == 1)
uint32_t nicUniCmdSendMlcRequest(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo, struct PARAM_MLC_REQ *prMlcReq)
{
	struct UNI_CMD_MLC *uni_cmd;
	struct MLD_STA_RECORD *mld_starec;
	uint32_t status;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLC);
	uint8_t *pos;

	mld_starec = mldBssGetPeekClient(prAdapter, prMldBssInfo);

	if (!prMldBssInfo || !prMlcReq || !mld_starec)
		return WLAN_STATUS_NOT_ACCEPTED;

	switch (prMlcReq->eMlcMode) {
	case MLC_MODE_DEFAULT:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_DEFAULT);
		break;
	case MLC_MODE_USER_CONFIG:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_USER_CONFIG) +
			mld_starec->rStarecList.u4NumElem *
			sizeof(struct UNI_CMD_MLC_LINK_INFO);
		break;
	case MLC_MODE_ACTIVE_NUM:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_ACTIVE_NUM);
		break;
	case MLC_MODE_LOW_POWER:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_LOW_POWER);
		break;
	case MLC_MODE_LOW_LATENCY:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_LOW_LATENCY);
		break;
	case MLC_MODE_HIGH_TPUT:
		max_cmd_len += sizeof(struct UNI_CMD_MLC_REQ_HIGH_TPUT);
		break;
	default:
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	uni_cmd = (struct UNI_CMD_MLC *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(ML, ERROR,
		       "Allocate UNI_CMD_MLC ==> FAILED.\n");
		return WLAN_STATUS_RESOURCES;
	}

	uni_cmd->ucGroupMldId = prMldBssInfo->ucGroupMldId;
	uni_cmd->ucTokenID = ++prMldBssInfo->ucMlcToken;
	pos = uni_cmd->aucTlvBuffer;

	DBGLOG(ML, INFO,  "GrpMldId[%d] Token[%d] Mode[%d]\n",
		uni_cmd->ucGroupMldId, uni_cmd->ucTokenID, prMlcReq->eMlcMode);

	if (prMlcReq->eMlcMode == MLC_MODE_DEFAULT) {
		struct UNI_CMD_MLC_REQ_DEFAULT *tag;

		tag = (struct UNI_CMD_MLC_REQ_DEFAULT *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_DEFAULT;
		tag->u2Length = sizeof(*tag);
	} else if (prMlcReq->eMlcMode == MLC_MODE_USER_CONFIG) {
		struct UNI_CMD_MLC_REQ_USER_CONFIG *tag;
		uint32_t valid_links = prMlcReq->u4Data1;
		uint32_t active_links = prMlcReq->u4Data2;
		struct UNI_CMD_MLC_LINK_INFO *link;
		struct LINK *list;
		struct STA_RECORD *cur;

		tag = (struct UNI_CMD_MLC_REQ_USER_CONFIG *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_USER_CONFIG;
		tag->u2Length = sizeof(*tag);
		tag->ucLinkNum = 0;

		list = &mld_starec->rStarecList;
		link = (struct UNI_CMD_MLC_LINK_INFO *)tag->aucLinkInfo;
		LINK_FOR_EACH_ENTRY(cur, list, rLinkEntryMld,
				struct STA_RECORD) {
			if (!(valid_links & BIT(cur->ucLinkId)))
				continue;

			link->ucBssIdx = cur->ucBssIndex;
			link->u2WlanIdx = cur->ucWlanIndex;
			if (!!(active_links & BIT(cur->ucLinkId)))
				link->ucLinkState = MLO_LINK_STATE_ACTIVE;
			else
				link->ucLinkState = MLO_LINK_STATE_INACTIVE;

			DBGLOG(INIT, DEBUG, "\tbss=%d,wlan_idx=%d,state=%d\n",
				link->ucBssIdx,
				link->u2WlanIdx,
				link->ucLinkState);

			link++;
			tag->u2Length += sizeof(struct UNI_CMD_MLC_LINK_INFO);
			tag->ucLinkNum++;
		}
	} else if (prMlcReq->eMlcMode == MLC_MODE_ACTIVE_NUM) {
		struct UNI_CMD_MLC_REQ_ACTIVE_NUM *tag;

		tag = (struct UNI_CMD_MLC_REQ_ACTIVE_NUM *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_ACTIVE_NUM;
		tag->u2Length = sizeof(*tag);
		tag->ucActiveLinkNum = (uint8_t) prMlcReq->u4Data1;
	} else if (prMlcReq->eMlcMode == MLC_MODE_LOW_POWER) {
		struct UNI_CMD_MLC_REQ_LOW_POWER *tag;

		tag = (struct UNI_CMD_MLC_REQ_LOW_POWER *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_LOW_POWER;
		tag->u2Length = sizeof(*tag);
	} else if (prMlcReq->eMlcMode == MLC_MODE_LOW_LATENCY) {
		struct UNI_CMD_MLC_REQ_LOW_LATENCY *tag;

		tag = (struct UNI_CMD_MLC_REQ_LOW_LATENCY *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_LOW_LATENCY;
		tag->u2Length = sizeof(*tag);
	} else if (prMlcReq->eMlcMode == MLC_MODE_HIGH_TPUT) {
		struct UNI_CMD_MLC_REQ_HIGH_TPUT *tag;

		tag = (struct UNI_CMD_MLC_REQ_HIGH_TPUT *) pos;
		tag->u2Tag = UNI_CMD_MLC_TAG_REQ_HIGH_TPUT;
		tag->u2Length = sizeof(*tag);
	}

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_MLC,
			     TRUE,
			     TRUE,
			     TRUE,
			     nicUniEventMlcReqDone,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}

uint32_t nicUniCmdSendMlcQueryLinkState(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvQueryBuffer, uint32_t u4QueryBufferLen)
{
	struct UNI_CMD_MLC *uni_cmd;
	struct UNI_CMD_MLC_QUERY_LINK_STATE *tag;
	uint32_t status;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLC) +
			       sizeof(struct UNI_CMD_MLC_QUERY_LINK_STATE);

	if (!prMldBssInfo)
		return WLAN_STATUS_NOT_ACCEPTED;

	uni_cmd = (struct UNI_CMD_MLC *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(ML, ERROR,
		       "Allocate UNI_CMD_MLC ==> FAILED.\n");
		return 0;
	}

	uni_cmd->ucGroupMldId = prMldBssInfo->ucGroupMldId;
	uni_cmd->ucTokenID = ++prMldBssInfo->ucMlcToken;
	tag = (struct UNI_CMD_MLC_QUERY_LINK_STATE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MLC_TAG_QUERY_LINK_STATE;
	tag->u2Length = sizeof(*tag);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_MLC,
			     FALSE,
			     FALSE,
			     TRUE,
			     nicUniEventMlcQueryLinkStateDone,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, pvQueryBuffer, u4QueryBufferLen);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}

uint32_t nicUniCmdSendMlcQueryInfo(struct ADAPTER *prAdapter,
	struct MLD_BSS_INFO *prMldBssInfo,
	void *pvQueryBuffer, uint32_t u4QueryBufferLen)
{
	struct UNI_CMD_MLC *uni_cmd;
	struct UNI_CMD_MLC_QUERY_INFO *tag;
	uint32_t status;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MLC) +
			       sizeof(struct UNI_CMD_MLC_QUERY_INFO);

	if (!prMldBssInfo)
		return WLAN_STATUS_NOT_ACCEPTED;

	uni_cmd = (struct UNI_CMD_MLC *) cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(ML, ERROR,
		       "Allocate UNI_CMD_MLC ==> FAILED.\n");
		return 0;
	}

	uni_cmd->ucGroupMldId = prMldBssInfo->ucGroupMldId;
	uni_cmd->ucTokenID = ++prMldBssInfo->ucMlcToken;
	tag = (struct UNI_CMD_MLC_QUERY_INFO *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MLC_TAG_QUERY_INFO;
	tag->u2Length = sizeof(*tag);

	status = wlanSendSetQueryUniCmd(prAdapter,
			     UNI_CMD_ID_MLC,
			     FALSE,
			     FALSE,
			     TRUE,
			     nicUniEventMlcQueryInfoDone,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, pvQueryBuffer, u4QueryBufferLen);

	cnmMemFree(prAdapter, uni_cmd);

	return status;
}
#endif /* CFG_SUPPORT_MLC */
#endif /* CFG_SUPPORT_802_11BE_MLO */

uint32_t nicUniCmdTestmodeCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_TEST_CTRL *cmd;
	struct UNI_CMD_TESTMODE *uni_cmd;
	struct UNI_CMD_TESTMODE_CTRL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE) +
				sizeof(struct UNI_CMD_TESTMODE_CTRL);

	if (info->ucCID != CMD_ID_TEST_CTRL)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_TEST_CTRL *) info->pucInfoBuffer;

	switch (cmd->ucAction) {
		case CMD_TEST_CTRL_ACT_SWITCH_MODE:
			if(cmd->u.u4OpMode == CMD_TEST_CTRL_ACT_SWITCH_MODE_NORMAL) {
				entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
						max_cmd_len, nicCmdEventLeaveRfTest,
						nicUniCmdTimeoutCommon);

			} else {
				/* CMD_TEST_CTRL_ACT_SWITCH_MODE_RF_TEST */
				/* CMD_TEST_CTRL_ACT_SWITCH_MODE_ICAP */
				entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
						max_cmd_len, nicCmdEventEnterRfTest,
						nicOidCmdEnterRFTestTimeout);
			}
			break;

		case CMD_TEST_CTRL_ACT_SET_AT:
			/* convert for unify cmd */
			cmd->ucAction = CMD_TEST_CTRL_ACT_SET_AT_ENG;
			entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);
			break;

		case CMD_TEST_CTRL_ACT_GET_AT:
			/* convert for unify cmd */
			cmd->ucAction = CMD_TEST_CTRL_ACT_GET_AT_ENG;
			entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
					max_cmd_len, nicUniEventQueryRfTestATInfo,
					nicUniCmdTimeoutCommon);
		break;

		default:
			return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TESTMODE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_TESTMODE_CTRL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TESTMODE_TAG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucAction = cmd->ucAction;
	kalMemCopy(&tag->u, &cmd->u, sizeof(cmd->u));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_XONVRAM
uint32_t nicUniCmdTestmodeXOCal(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct TEST_MODE_XO_CAL *data = pvQueryBuffer;
	struct UNI_CMD_TESTMODE *uni_cmd;
	struct UNI_CMD_TESTMODE_XO_CAL *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE_CTRL) +
		sizeof(struct UNI_CMD_TESTMODE_XO_CAL);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_TESTMODE *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(RFTEST, ERROR,
		       "Allocate UNI_CMD_TESTMODE_CTRL ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_TESTMODE_XO_CAL *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TESTMODE_TAG_XO_CAL;
	tag->u2Length = sizeof(*tag);
	tag->rXoReq.u4CalType = data->u4CalType;
	tag->rXoReq.u4ClkSrc = data->u4ClkSrc;
	tag->rXoReq.u4Mode = data->u4Mode;
	tag->rXoReq.u4TargetReq = data->u4TargetReq;

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_TESTMODE_CTRL,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventRfTestXoCal,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);
	return status;
}
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
uint32_t nicUniCmdTestmodePlCal(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct TEST_MODE_PL_CAL *data = pvQueryBuffer;
	struct UNI_CMD_TESTMODE *uni_cmd;
	struct UNI_CMD_TESTMODE_PL_CAL *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE_CTRL) +
		sizeof(struct UNI_CMD_TESTMODE_PL_CAL);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_TESTMODE *) cnmMemAlloc(ad,
		RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(RFTEST, ERROR,
			"Allocate UNI_CMD_TESTMODE_CTRL ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_TESTMODE_PL_CAL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TESTMODE_TAG_PL_CAL;
	tag->u2Length = sizeof(*tag);

	tag->rPlReq.u4BandIdx = data->u4BandIdx;
	tag->rPlReq.u4PLCalId = data->u4PLCalId;
	tag->rPlReq.u4Action = data->u4Action;
	tag->rPlReq.u4Flags = data->u4Flags;
	tag->rPlReq.u4InCnt = data->u4InCnt;
	memcpy(tag->rPlReq.u4InData, data->u4InData, sizeof(data->u4InData));

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_TESTMODE_CTRL,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventRfTestPlCal,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);
	return status;
}
#endif /* CFG_SUPPORT_PLCAL */

#if CFG_SUPPORT_QA_TOOL
#if (CONFIG_WLAN_SERVICE == 1)
uint32_t nicUniCmdTestmodeListmode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_TESTMODE *uni_cmd;
	struct UNI_CMD_TESTMODE_LISTMODE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE_CTRL) +
				sizeof(struct UNI_CMD_TESTMODE_LISTMODE);

	if (info->ucCID != CMD_ID_LIST_MODE)
		return WLAN_STATUS_NOT_ACCEPTED;

	if (info->u4SetQueryInfoLen > TESTMODE_LISTMODE_DATA_LEN)
		return WLAN_STATUS_RESOURCES;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
			max_cmd_len, nicCmdEventListmode,
			nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TESTMODE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_TESTMODE_LISTMODE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TESTMODE_TAG_LISTMODE;
	tag->u2Length = sizeof(*tag);
	kalMemCopy(tag->aucData,
				info->pucInfoBuffer,
				info->u4SetQueryInfoLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif /* (CONFIG_WLAN_SERVICE == 1) */
#endif

#if CFG_SUPPORT_QA_TOOL
uint32_t nicUniExtCmdTestmodeCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_TEST_CTRL_EXT_T *cmd;
	struct UNI_CMD_TESTMODE *uni_cmd;
	struct UNI_CMD_TESTMODE_CTRL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry = NULL;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE) +
				sizeof(struct UNI_CMD_TESTMODE_CTRL);

	TRACE_FUNC(NIC, DEBUG, "%s\n");

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		info->ucExtCID != EXT_CMD_ID_RF_TEST)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_TEST_CTRL_EXT_T *) info->pucInfoBuffer;

	switch (cmd->ucAction) {
		case ACTION_IN_RFTEST:
			if (cmd->u.rRfATInfo.u4FuncIndex == SET_ICAP_CAPTURE_START) {
				entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
						max_cmd_len, nicUniCmdEventSetCommon,
						nicUniCmdTimeoutCommon);
			} else if (cmd->u.rRfATInfo.u4FuncIndex == GET_ICAP_CAPTURE_STATUS) {
				entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
						max_cmd_len, nicUniEventRfTestHandler,
						nicUniCmdTimeoutCommon);
			} else if (cmd->u.rRfATInfo.u4FuncIndex == GET_ICAP_RAW_DATA) {
				entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_CTRL,
						max_cmd_len, nicUniEventRfTestHandler,
						nicUniCmdTimeoutCommon);
			} else {
				DBGLOG(NIC, WARN, "Err Test cmd funcId=%d\n",
					cmd->u.rRfATInfo.u4FuncIndex);
			}
			break;

		default:
			return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TESTMODE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_TESTMODE_CTRL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_TESTMODE_TAG_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucAction = cmd->ucAction;
	kalMemCopy(&tag->u, &cmd->u, sizeof(tag->u));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}


uint32_t nicUniCmdTestmodeRxStat(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ACCESS_RX_STAT *cmd;
	struct UNI_CMD_TESTMODE_RX_STAT *uni_cmd;
	struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TESTMODE_RX_STAT) +
				sizeof(struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL);

	if (info->ucCID != CMD_ID_ACCESS_RX_STAT)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ACCESS_RX_STAT *) info->pucInfoBuffer;
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_RX_STAT,
				max_cmd_len, nicUniEventQueryRxStatAll,
				nicUniCmdTimeoutCommon);
#else
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TESTMODE_RX_STAT,
				max_cmd_len, nicUniEventQueryRxStatAllCon3,
				nicUniCmdTimeoutCommon);
#endif

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TESTMODE_RX_STAT *)entry->pucInfoBuffer;
	tag = (struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL *)uni_cmd->aucTlvBuffer;
	tag->u2Length = sizeof(*tag);
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	tag->u2Tag = UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL;
	tag->u1DbdcIdx = 0;
#else
	tag->u2Tag = UNI_CMD_TESTMODE_RX_TAG_GET_STAT_ALL_V2;
	tag->u1DbdcIdx = cmd->ucDbdcIdx;
	tag->u1Data = cmd->ucData;
#endif

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdTestmodeRxStatTlv(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct UNI_CMD_TESTMODE_RX_STAT *uni_cmd;
	struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL buf = {0};
	struct RFTEST_RX_STAT_INFO_TLV *pst_rx_info;
	uint32_t max_cmd_len = 0;
	uint32_t status = WLAN_STATUS_SUCCESS;

	pst_rx_info = (struct RFTEST_RX_STAT_INFO_TLV *)pvQueryBuffer;

	if (pst_rx_info->ucTagNum == 0) {
		DBGLOG(RFTEST, ERROR, "Invalid data TagNum[%d]\n",
			pst_rx_info->ucTagNum);
		return WLAN_STATUS_FAILURE;
	}

	max_cmd_len = sizeof(struct UNI_CMD_TESTMODE_RX_STAT) +
		(pst_rx_info->ucTagNum *
			sizeof(struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL));

	uni_cmd = (struct UNI_CMD_TESTMODE_RX_STAT *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);

	if (!uni_cmd) {
		DBGLOG(RFTEST, ERROR,
		       "Allocate UNI_CMD_GET_STATISTICS ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd->u1BandIdx = pst_rx_info->ucDbdcIdx;

	buf.u2Tag = UNI_CMD_TESTMODE_RX_TAG_GET_STAT_TLV;
	buf.u2Length =
		sizeof(struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL) +
		sizeof(struct UNI_CMD_TESTMODE_RX_STAT);
	buf.u1DbdcIdx = pst_rx_info->ucDbdcIdx;
	kalMemCopy(uni_cmd->aucTlvBuffer, (uint8_t *)&buf,
			sizeof(struct UNI_CMD_TESTMODE_RX_GET_STAT_ALL));

	status = wlanSendSetQueryUniCmd(ad,
				UNI_CMD_ID_TESTMODE_RX_STAT,
				FALSE,
				TRUE,
				TRUE,
				nicUniEventQueryRxStatInfoTlv,
				nicUniCmdTimeoutCommon,
				max_cmd_len,
				(void *)uni_cmd,
				pvQueryBuffer,
				u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);

	return status;
}

#endif

uint32_t nicUniCmdSR(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct _SR_CMD_SR_IND_T *cmd;
	struct UNI_CMD_SR *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SR);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		info->ucExtCID != EXT_CMD_ID_SR_CTRL)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct _SR_CMD_SR_IND_T *) info->pucInfoBuffer;

	switch (cmd->rSrCmd.u1CmdSubId) {
	case SR_CMD_GET_SR_IND_ALL_INFO: {
		struct UNI_CMD_SR_IND *tag;

		max_cmd_len += sizeof(struct UNI_CMD_SR_IND);
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SR,
			max_cmd_len, nicUniCmdEventSetCommon,
			nicUniCmdTimeoutCommon);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		uni_cmd = (struct UNI_CMD_SR *) entry->pucInfoBuffer;
		uni_cmd->u1BandIdx = cmd->rSrCmd.u1DbdcIdx;

		tag = (struct UNI_CMD_SR_IND *) uni_cmd->au1TlvBuffer;
		tag->u2Tag = UNI_CMD_SR_TAG_HW_IND;
		tag->u2Length = sizeof(*tag);
	}
		break;
	case SR_CMD_GET_SR_CAP_ALL_INFO: {
		struct UNI_CMD_SR_CAP *tag;

		max_cmd_len += sizeof(struct UNI_CMD_SR_CAP);
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SR,
			max_cmd_len, nicUniCmdEventSetCommon,
			nicUniCmdTimeoutCommon);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		uni_cmd = (struct UNI_CMD_SR *) entry->pucInfoBuffer;
		uni_cmd->u1BandIdx = cmd->rSrCmd.u1DbdcIdx;

		tag = (struct UNI_CMD_SR_CAP *) uni_cmd->au1TlvBuffer;
		tag->u2Tag = UNI_CMD_SR_TAG_HW_CAP;
		tag->u2Length = sizeof(*tag);
	}
		break;
	case SR_CMD_SET_SR_CAP_SREN_CTRL: {
		struct UNI_CMD_SR_CAP *tag;
		struct _SR_CMD_SR_CAP_T *cmd_sr;

		max_cmd_len += sizeof(struct UNI_CMD_SR_CAP);
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SR,
			max_cmd_len, NULL,
			NULL);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		cmd_sr = (struct _SR_CMD_SR_CAP_T *) info->pucInfoBuffer;
		uni_cmd = (struct UNI_CMD_SR *) entry->pucInfoBuffer;
		uni_cmd->u1BandIdx = cmd_sr->rSrCmd.u1DbdcIdx;

		tag = (struct UNI_CMD_SR_CAP *) uni_cmd->au1TlvBuffer;
		tag->u2Tag = UNI_CMD_SR_TAG_HW_CAP_SREN;
		tag->u2Length = sizeof(*tag);
		tag->u4Value = cmd_sr->rSrCap.fgSrEn;
	}
		break;
	default: {
		DBGLOG(NIC, DEBUG, "No Support SR CMD:%d\n",
			cmd->rSrCmd.u1CmdSubId);
		return WLAN_STATUS_NOT_ACCEPTED;
	}
		break;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGetBugReport(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	struct UNI_CMD_BUG_REPORT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
			       sizeof(struct UNI_CMD_BUG_REPORT);

	if (info->ucCID != CMD_ID_GET_BUG_REPORT)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_STATISTICS,
		max_cmd_len, nicUniEventBugReport, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_STATISTICS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_BUG_REPORT *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BUG_REPORT;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdTxPowerCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_TXPOWER_CONFIG *uni_cmd;
	struct TAG_HDR *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t tag_id = *info->pucInfoBuffer;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_TXPOWER_CONFIG) +
			       sizeof(struct TAG_HDR) +
			       info->u4SetQueryInfoLen;

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
	   (tag_id != TX_POWER_SHOW_INFO && tag_id != TX_RATE_POWER_CTRL &&
	   tag_id != PERCENTAGE_CTRL && tag_id != PERCENTAGE_DROP_CTRL))
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_TXPOWER, max_cmd_len,
		tag_id == TX_POWER_SHOW_INFO ? nicUniCmdEventTxPowerInfo :
		nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_TXPOWER_CONFIG *) entry->pucInfoBuffer;
	tag = (struct TAG_HDR *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = tag_id;
	tag->u2Length = sizeof(*tag) + info->u4SetQueryInfoLen;
	kalMemCopy(tag->aucBuffer, info->pucInfoBuffer,
		info->u4SetQueryInfoLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdThermalProtect(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_THERMAL *uni_cmd;
	struct TAG_HDR *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint8_t tag_id = *info->pucInfoBuffer;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_THERMAL) +
			       sizeof(struct TAG_HDR) +
			       info->u4SetQueryInfoLen;

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM)
		return WLAN_STATUS_NOT_ACCEPTED;

	switch (tag_id) {
	case THERMAL_PROTECT_ENABLE:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_ENABLE;
		break;
	case THERMAL_PROTECT_DISABLE:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_DISABLE;
		break;
	case THERMAL_PROTECT_DUTY_CONFIG:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_DUTY_CONFIG;
		break;
	case THERMAL_PROTECT_STATE_ACT:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_STATE_ACT;
		break;
	case THERMAL_PROTECT_MECH_INFO:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_MECH_INFO;
		break;
	case THERMAL_PROTECT_DUTY_INFO:
		tag_id = UNI_CMD_THERMAL_TAG_PROTECT_DUTY_INFO;
		break;
	default:
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_THERMAL, max_cmd_len,
		nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_THERMAL *) entry->pucInfoBuffer;
	tag = (struct TAG_HDR *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = tag_id;
	tag->u2Length = sizeof(*tag) + info->u4SetQueryInfoLen;
	kalMemCopy(tag->aucBuffer, info->pucInfoBuffer,
		info->u4SetQueryInfoLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdEfuseBufferMode(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_EFUSE_BUFFER_MODE_CONNAC_T *cmd;
	struct UNI_CMD_EFUSE *uni_cmd;
	struct UNI_CMD_EFUSE_BUFFER_MODE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_EFUSE) +
		sizeof(struct UNI_CMD_EFUSE_BUFFER_MODE);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		info->ucExtCID != EXT_CMD_ID_EFUSE_BUFFER_MODE)
		return WLAN_STATUS_NOT_ACCEPTED;

	info->fgSetQuery = TRUE;
	info->fgNeedResp = TRUE;

	cmd = (struct CMD_EFUSE_BUFFER_MODE_CONNAC_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_EFUSE_CONTROL, max_cmd_len,
		nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_EFUSE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_EFUSE_BUFFER_MODE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_EFUSE_CTRL_TAG_BUFFER_MODE;
	tag->u2Length = sizeof(*tag);
	tag->ucSourceMode = cmd->ucSourceMode;
	tag->ucContentFormat = cmd->ucContentFormat;
	tag->u2Count = cmd->u2Count;
	kalMemCopy(tag->aucBinContent, cmd->aBinContent,
		BUFFER_MODE_CONTENT_MAX);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdEfuseAccess(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_ACCESS_EFUSE *cmd;
	struct UNI_CMD_EFUSE *uni_cmd;
	struct UNI_CMD_ACCESS_EFUSE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_EFUSE) +
		sizeof(struct UNI_CMD_ACCESS_EFUSE);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		info->ucExtCID != EXT_CMD_ID_EFUSE_ACCESS)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_ACCESS_EFUSE *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_EFUSE_CONTROL, max_cmd_len,
		nicUniEventEfuseAccess, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_EFUSE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_ACCESS_EFUSE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_EFUSE_CTRL_TAG_ACCESS;
	tag->u2Length = sizeof(*tag);
	tag->u4Address = cmd->u4Address;
	tag->u4Valid = cmd->u4Valid;
	kalMemCopy(tag->aucData, cmd->aucData,
		BUFFER_ACCESS_CONTENT_MAX);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdEfuseFreeBlock(struct ADAPTER *ad,
	struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_EFUSE_FREE_BLOCK *cmd;
	struct UNI_CMD_EFUSE *uni_cmd;
	struct UNI_CMD_EFUSE_FREE_BLOCK *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_EFUSE) +
		sizeof(struct UNI_CMD_EFUSE_FREE_BLOCK);

	if (info->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		info->ucExtCID != EXT_CMD_ID_EFUSE_FREE_BLOCK)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_EFUSE_FREE_BLOCK *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_EFUSE_CONTROL, max_cmd_len,
		nicUniEventEfuseFreeBlock, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_EFUSE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_EFUSE_FREE_BLOCK *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_EFUSE_CTRL_TAG_FREE_BLOCK;
	tag->u2Length = sizeof(*tag);
	tag->ucGetFreeBlock = cmd->ucGetFreeBlock;
	tag->ucVersion = cmd->ucVersion;
	tag->ucDieIndex = cmd->ucDieIndex;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdSetCoexStopConnProtect(struct ADAPTER *ad, uint8_t ucBssIdx)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct BSS_INFO *prBssInfo;
	struct UNI_CMD_COEX_T *uni_cmd;
	struct UNI_CMD_COEX_STOP_CONNECT_PROTECT_T *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_COEX_T) +
			sizeof(struct UNI_CMD_COEX_STOP_CONNECT_PROTECT_T);

	prBssInfo = GET_BSS_INFO_BY_INDEX(ad, ucBssIdx);
	ASSERT(prBssInfo);

	uni_cmd = (struct UNI_CMD_COEX_T *) cnmMemAlloc(ad,
				RAM_TYPE_MSG, max_cmd_len);

	if (!uni_cmd) {
		DBGLOG(INIT, ERROR, "Allocate UNI_CMD_COEX_T failed.\n");
		return WLAN_STATUS_RESOURCES;
	}

	tag = (struct UNI_CMD_COEX_STOP_CONNECT_PROTECT_T *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_COEX_STOP_CONNECT_PROTECT;
	tag->ucBssInfoIdx = ucBssIdx;
	tag->u2Length = sizeof(*tag);

	status = wlanSendSetQueryUniCmd(ad,
			UNI_CMD_ID_COEX,
			TRUE,
			FALSE,
			FALSE,
			nicUniCmdEventSetCommon,
			nicUniCmdTimeoutCommon,
			max_cmd_len,
			(void *)uni_cmd, NULL, 0);
	cnmMemFree(ad, uni_cmd);

	/* convert WLAN_STATUS_PENDING to success */
	if (status == WLAN_STATUS_PENDING)
		status = WLAN_STATUS_SUCCESS;

	return status;
}

#if (CFG_SUPPORT_RTT == 1)
uint32_t nicUniCmdRttGetCapabilities(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_RTT *uni_cmd;
	struct UNI_CMD_RTT_GET_CAPA_T *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_RTT) +
		sizeof(struct UNI_CMD_RTT_GET_CAPA_T);

	if (info->ucCID != CMD_ID_RTT_GET_CAPABILITIES)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RTT,
		max_cmd_len, nicUniEventRttCapabilities,
		nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RTT *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_RTT_GET_CAPA_T *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_RTT_TAG_GET_CAPA;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdRttRangeRequest(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_RTT_REQUEST *cmd;
	struct UNI_CMD_RTT *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = 0;
	uint8_t i;

	if (info->ucCID != CMD_ID_RTT_RANGE_REQUEST ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_RTT_REQUEST *) info->pucInfoBuffer;
	if (!cmd ||
		cmd->ucConfigNum == 0 ||
		cmd->ucConfigNum > CFG_RTT_MAX_CANDIDATES)
		return WLAN_STATUS_INVALID_DATA;

	max_cmd_len = sizeof(struct UNI_CMD_RTT);

	if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11MC) {
		if (cmd->arRttConfigs[0].ePeer == RTT_PEER_AP ||
			cmd->arRttConfigs[0].ePeer == RTT_PEER_NAN_RSTA)
			max_cmd_len +=
				sizeof(struct UNI_CMD_RTT_RANGE_REQ_MC_T);
		else if (cmd->arRttConfigs[0].ePeer == RTT_PEER_STA ||
			cmd->arRttConfigs[0].ePeer == RTT_PEER_NAN_ISTA)
			max_cmd_len +=
				sizeof(struct UNI_CMD_RTT_RANGE_RSP_MC_T);
		else
			return WLAN_STATUS_INVALID_DATA;
	} else if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11AZ_NTB)
		max_cmd_len += sizeof(struct UNI_CMD_RTT_RANGE_REQ_AZ_NTB_T);
	else
		return WLAN_STATUS_INVALID_DATA;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_RTT,
		max_cmd_len, nicUniCmdEventSetCommon,
		nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RTT *) entry->pucInfoBuffer;
	if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11MC) {
		struct UNI_CMD_RTT_RANGE_REQ_MC_T *tag;

		tag = (struct UNI_CMD_RTT_RANGE_REQ_MC_T *)
			uni_cmd->aucTlvBuffer;

		switch (cmd->arRttConfigs[0].ePeer) {
		case RTT_PEER_AP:
		case RTT_PEER_NAN_RSTA:
			tag->u2Tag = UNI_CMD_RTT_TAG_RANGE_REQ_MC;
			tag->u2Length =
				sizeof(struct UNI_CMD_RTT_RANGE_REQ_MC_T);
			break;
		case RTT_PEER_STA:
		case RTT_PEER_NAN_ISTA:
			tag->u2Tag = UNI_CMD_RTT_TAG_RANGE_RSP_MC;
			tag->u2Length =
				sizeof(struct UNI_CMD_RTT_RANGE_RSP_MC_T);
			break;
		default:
			DBGLOG(REQ, ERROR,
				"11mc rtt request, invalid peer type: %d\n",
				cmd->arRttConfigs[0].ePeer);
			nicUniCmdFreeEntry(ad, entry);
			return WLAN_STATUS_INVALID_DATA;
		}

		tag->ucSeqNum = cmd->ucSeqNum;
		tag->fgEnable = cmd->fgEnable;
		tag->ucConfigNum = cmd->ucConfigNum;

		for (i = 0; i < cmd->ucConfigNum; i++) {
			kalMemCopy(&tag->arRttConfigs[i],
				&cmd->arRttConfigs[i],
				sizeof(struct RTT_CONFIG_MC));
		}

		dumpMemory32((uint32_t *)tag->arRttConfigs,
			sizeof(struct RTT_CONFIG) * cmd->ucConfigNum);

		DBGLOG(REQ, DEBUG,
			"11mc rtt request, tag: %d, seq: %d, enable: %d\n",
			tag->u2Tag, tag->ucSeqNum, tag->fgEnable);
	} else if (cmd->arRttConfigs[0].eType == RTT_TYPE_2_SIDED_11AZ_NTB) {
		struct UNI_CMD_RTT_RANGE_REQ_AZ_NTB_T *tag;

		tag = (struct UNI_CMD_RTT_RANGE_REQ_AZ_NTB_T *)
			uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_RTT_TAG_RANGE_REQ_AZ_NTB;
		tag->u2Length = sizeof(*tag);
		tag->ucSeqNum = cmd->ucSeqNum;
		tag->fgEnable = cmd->fgEnable;
		tag->ucConfigNum = cmd->ucConfigNum;

		for (i = 0; i < cmd->ucConfigNum; i++) {
			kalMemCopy(&tag->arRttConfigs[i],
				&cmd->arRttConfigs[i],
				sizeof(struct RTT_CONFIG_AZ_NTB));
		}

		dumpMemory32((uint32_t *)tag->arRttConfigs,
			sizeof(struct RTT_CONFIG_AZ_NTB) * cmd->ucConfigNum);

		DBGLOG(REQ, DEBUG,
			"11az rtt request, tag: %d, seq: %d, enable: %d\n",
			tag->u2Tag, tag->ucSeqNum, tag->fgEnable);
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdRttInstallLtfKeyseed(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_RTT_INSTALL_LTF_KEYSEED *cmd;
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_INSTALL_LTE_KEYSEED *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
		sizeof(struct UNI_CMD_STAREC_INSTALL_LTE_KEYSEED);

	if (info->ucCID != CMD_ID_RTT_INSTALL_LTF_KEYSEED ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_RTT_INSTALL_LTF_KEYSEED *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_STAREC_INFO,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_STAREC *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_STAREC_INSTALL_LTE_KEYSEED *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_INSTALL_LTF_KEYSEED;
	tag->u2Length = sizeof(*tag);
	tag->ucAddRemove = cmd->ucAddRemove;
	tag->ucLtfKeyseedLen = cmd->ucLtfKeyseedLen;
	tag->u2WlanIdx = cmd->u2WlanIdx;
	kalMemCopy(tag->aucLtfKeyseed, cmd->aucLtfKeyseed,
		cmd->ucLtfKeyseedLen);

	DBGLOG(REQ, DEBUG,
		"add=%d, wlanIdx=%d, LftKeyseedLen=%d\n",
		tag->ucAddRemove, tag->u2WlanIdx, tag->ucLtfKeyseedLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_RTT */

#if CFG_SUPPORT_NAN
struct WIFI_UNI_CMD_ENTRY *nicNanUniCmdGenEntry(struct ADAPTER *ad,
			uint16_t u2Tag, uint16_t u2Length, uint8_t **ppucCmdBuf)
{
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len;
	struct UNI_CMD_NAN *uni_cmd;
	struct UNI_CMD_EVENT_TLV_ELEMENT_T *tag;

	max_cmd_len = sizeof(struct UNI_CMD_NAN) +
		      sizeof(struct UNI_CMD_EVENT_TLV_ELEMENT_T) +
		      u2Length;

	if (u2Tag == UNI_CMD_NAN_TAG_GET_DEVICE_INFO) {
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_NAN,
				max_cmd_len,
				nanDevEventQueryDeviceInfo,
				nicUniCmdTimeoutCommon);
	} else {
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_NAN,
				max_cmd_len,
				nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon);
	}

	if (!entry)
		return NULL;

	uni_cmd = (struct UNI_CMD_NAN *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_EVENT_TLV_ELEMENT_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = u2Tag;
	/* The length counts the size of structure and following payload */
	tag->u2Length = u2Length + sizeof(struct UNI_CMD_EVENT_TLV_ELEMENT_T);

	*ppucCmdBuf = tag->aucbody;

	return entry;
}

#if (CFG_SUPPORT_NAN == 1)
int32_t nanGetSubCmdId(uint32_t tag, uint16_t *u2CmdTag)
{
	enum ENUM_UNI_CMD_NAN_TAG eCmdTag = -1;

	if (tag >= NAN_CMD_NUM)
		return -1;

	eCmdTag = nanUniCommandTag[tag];
	if (unlikely(eCmdTag == 0) && tag != NAN_CMD_MASTER_PREFERENCE)
		return -1;

	if (u2CmdTag)
		*u2CmdTag = eCmdTag;
	return eCmdTag;
}
#endif

uint32_t nicUniCmdNan(struct ADAPTER *ad, struct WIFI_UNI_SETQUERY_INFO *info)
{
#if (CFG_SUPPORT_NAN == 1)

	struct WIFI_UNI_CMD_ENTRY *entry;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement;
	uint8_t *ucCmdBuf;
	uint16_t u2CmdTag;
	uint16_t u2CmdLength;


	if (info->ucCID != CMD_ID_NAN_EXT_CMD)
		return WLAN_STATUS_NOT_ACCEPTED;

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)info->pucInfoBuffer;

	/* Current implementation only holds one Tag in one event */
	prTlvElement = nicNanGetTargetTlvElement(1, prTlvCommon);

	if (prTlvElement == NULL)
		return WLAN_STATUS_FAILURE;

	if (nanGetSubCmdId(prTlvElement->tag_type, &u2CmdTag) < 0) {
		DBGLOG(NAN, ERROR, "command %u conversion failed\n",
		       prTlvElement->tag_type);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	/* u2CmdTag set by nanGetSubCmdId() successfully */
	u2CmdLength = prTlvElement->body_len;

	entry = nicNanUniCmdGenEntry(ad, u2CmdTag, u2CmdLength, &ucCmdBuf);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	kalMemCopy(ucCmdBuf, prTlvElement->aucbody, u2CmdLength);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;

#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}
#endif

uint32_t nicUniCmdFwLogQueryBase(struct ADAPTER *ad,
	uint32_t *addr)
{
	struct UNI_CMD_WSYS_CONFIG *uni_cmd;
	struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
		sizeof(struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_WSYS_CONFIG *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_BUFFER_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucType = BIT(FW_LOG_CTRL_CMD_GET_BASE_ADDR);

	status = wlanSendSetQueryUniCmd(ad,
			UNI_CMD_ID_WSYS_CONFIG,
			TRUE,
			TRUE,
			FALSE,
			nicUniEventFwLogQueryBase,
			nicUniCmdTimeoutCommon,
			max_cmd_len,
			(void *)uni_cmd,
			addr,
			sizeof(*addr));

	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdFwLogUpdateRead(struct ADAPTER *ad,
	enum FW_LOG_CMD_CTRL_TYPE type,
	uint32_t addr)
{
	struct UNI_CMD_WSYS_CONFIG *uni_cmd;
	struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_WSYS_CONFIG) +
		sizeof(struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_WSYS_CONFIG *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_WSYS_CONFIG_FW_LOG_BUFFER_CTRL *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_WSYS_CONFIG_TAG_FW_LOG_BUFFER_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->ucType = BIT(type);
	switch (type) {
	case FW_LOG_CTRL_CMD_UPDATE_MCU_READ:
		tag->u4MCUAddr = addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_WIFI_READ:
		tag->u4WFAddr = addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_BT_READ:
		tag->u4BTAddr = addr;
		break;
	case FW_LOG_CTRL_CMD_UPDATE_GPS_READ:
		tag->u4GPSAddr = addr;
		break;
	default:
		DBGLOG(INIT, ERROR, "Unsupported type: %d\n", type);
		goto exit;
	}

	status = wlanSendSetQueryUniCmd(ad,
			UNI_CMD_ID_WSYS_CONFIG,
			TRUE,
			FALSE,
			FALSE,
			nicUniCmdEventSetCommon,
			nicUniCmdTimeoutCommon,
			max_cmd_len,
			(void *)uni_cmd, NULL, 0);

exit:
	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdQueryThermalAdieTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct THERMAL_TEMP_DATA *data = pvQueryBuffer;
	struct UNI_CMD_THERMAL *uni_cmd;
	struct UNI_CMD_THERMAL_SENSOR_INFO *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_THERMAL) +
		sizeof(struct UNI_CMD_THERMAL_SENSOR_INFO);
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (data->eType != THERMAL_TEMP_TYPE_ADIE) {
		DBGLOG(INIT, ERROR,
		       "Unexpected type %d\n",
		       data->eType);
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd = (struct UNI_CMD_THERMAL *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_THERMAL_SENSOR_INFO *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_THERMAL_TAG_FEATURE_TEMPERATURE_QUERY;
	tag->u2Length = sizeof(*tag);
	tag->ucThermalCtrlFormatId = 0;
	tag->ucActionIdx = THERMAL_SENSOR_INFO_TEMPERATURE;
	tag->ucBandIdx = data->ucIdx;

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_THERMAL,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventThermalAdieTemp,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdQueryThermalDdieTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct THERMAL_TEMP_DATA *data = pvQueryBuffer;
	struct UNI_CMD_THERMAL *uni_cmd;
	struct UNI_CMD_THERMAL_DDIE_SENSOR_INFO *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_THERMAL) +
		sizeof(struct UNI_CMD_THERMAL_DDIE_SENSOR_INFO);
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (data->eType != THERMAL_TEMP_TYPE_DDIE) {
		DBGLOG(INIT, ERROR,
		       "Unexpected type %d\n",
		       data->eType);
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd = (struct UNI_CMD_THERMAL *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_THERMAL_DDIE_SENSOR_INFO *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_THERMAL_TAG_FEATURE_DDIE_INFO;
	tag->u2Length = sizeof(*tag);
	tag->ucThermalCtrlFormatId = 0;
	tag->ucActionIdx = THERMAL_SENSOR_INFO_TEMPERATURE;
	tag->ucSensorIdx = data->ucIdx;

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_THERMAL,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventThermalDdieTemp,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdQueryThermalAdcTemp(struct ADAPTER *ad,
	void *pvQueryBuffer,
	uint32_t u4QueryBufferLen)
{
	struct THERMAL_TEMP_DATA_V2 *data = pvQueryBuffer;
	struct UNI_CMD_THERMAL *uni_cmd;
	struct UNI_CMD_THERMAL_TEMP_ADC_INFO *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_THERMAL) +
		sizeof(struct UNI_CMD_THERMAL_TEMP_ADC_INFO);
	uint32_t status = WLAN_STATUS_SUCCESS;

	uni_cmd = (struct UNI_CMD_THERMAL *) cnmMemAlloc(ad,
			RAM_TYPE_MSG, max_cmd_len);

	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_BF ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_THERMAL_TEMP_ADC_INFO *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_THERMAL_TAG_FEATURE_ADC_TEMPERATURE_QUERY;
	tag->u2Length = sizeof(*tag);
	tag->ucThermalCtrlFormatId = 0;
	tag->ucType = data->ucType;
	tag->ucIndex = data->ucIdx;

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_THERMAL,
					FALSE,
					TRUE,
					TRUE,
					nicUniEventThermalAdcTemp,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd,
					pvQueryBuffer,
					u4QueryBufferLen);

	cnmMemFree(ad, uni_cmd);
	return status;
}

uint32_t nicUniCmdSetCsiControl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
#if CFG_SUPPORT_CSI
	struct CMD_CSI_CONTROL_T *cmd;
	struct UNI_CMD_CSI *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_CSI);

	if (info->ucCID != CMD_ID_CSI_CONTROL ||
		 info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_CSI_CONTROL_T *) info->pucInfoBuffer;

	switch (cmd->ucMode) {
	case CSI_CONTROL_MODE_STOP: {
		struct UNI_CMD_CSI_STOP *tag;

		max_cmd_len += sizeof(struct UNI_CMD_CSI_STOP);
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CSI,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		uni_cmd = (struct UNI_CMD_CSI *) entry->pucInfoBuffer;
		uni_cmd->ucBandIdx = cmd->ucBandIdx;

		tag = (struct UNI_CMD_CSI_STOP *) uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_CSI_TAG_STOP;
		tag->u2Length = sizeof(*tag);
#if CFG_CSI_DEBUG
		DBGLOG(NIC, DEBUG, "[CSI] Stop!\n");
#endif
	}
		break;
	case CSI_CONTROL_MODE_START: {
		struct UNI_CMD_CSI_START *tag;

		max_cmd_len += sizeof(struct UNI_CMD_CSI_START);
		entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CSI,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		uni_cmd = (struct UNI_CMD_CSI *) entry->pucInfoBuffer;
		uni_cmd->ucBandIdx = cmd->ucBandIdx;

		tag = (struct UNI_CMD_CSI_START *) uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_CSI_TAG_START;
		tag->u2Length = sizeof(*tag);
#if CFG_CSI_DEBUG
		DBGLOG(NIC, DEBUG, "[CSI] Start!\n");
#endif
	}
		break;
	case CSI_CONTROL_MODE_SET: {
		if (cmd->ucCfgItem == CSI_CONFIG_FRAME_TYPE) {
			struct UNI_CMD_CSI_SET_FRAME_TYPE *tag;

			max_cmd_len +=
				sizeof(struct UNI_CMD_CSI_SET_FRAME_TYPE);
			entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CSI,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);
			if (!entry)
				return WLAN_STATUS_RESOURCES;

			uni_cmd = (struct UNI_CMD_CSI *) entry->pucInfoBuffer;
			uni_cmd->ucBandIdx = cmd->ucBandIdx;

			tag = (struct UNI_CMD_CSI_SET_FRAME_TYPE *)
				uni_cmd->aucTlvBuffer;
			tag->u2Tag = UNI_CMD_CSI_TAG_SET_FRAME_TYPE;
			tag->u2Length = sizeof(*tag);
			tag->ucFrameTypeIndex = cmd->ucValue1;
			tag->u4FrameType = cmd->u4Value2;
#if CFG_CSI_DEBUG
			DBGLOG(NIC, DEBUG,
			   "[CSI] Set frame type %d, %d\n",
				tag->ucFrameTypeIndex,
				tag->u4FrameType);
#endif
		} else if (cmd->ucCfgItem == CSI_CONFIG_CHAIN_NUMBER) {
			struct UNI_CMD_CSI_SET_CHAIN_NUMBER *tag;

			max_cmd_len +=
				sizeof(struct UNI_CMD_CSI_SET_CHAIN_NUMBER);
			entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_CSI,
					max_cmd_len, nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon);
			if (!entry)
				return WLAN_STATUS_RESOURCES;

			uni_cmd = (struct UNI_CMD_CSI *) entry->pucInfoBuffer;
			uni_cmd->ucBandIdx = cmd->ucBandIdx;
			tag = (struct UNI_CMD_CSI_SET_CHAIN_NUMBER *)
				uni_cmd->aucTlvBuffer;
			tag->u2Tag = UNI_CMD_CS_TAGI_SET_CHAIN_NUMBER;
			tag->u2Length = sizeof(*tag);
			tag->ucMaxChain = cmd->ucValue1;
#if CFG_CSI_DEBUG
			DBGLOG(NIC, DEBUG,
			   "[CSI] Set chain number %d ucBandIdx=%d\n",
				tag->ucMaxChain, uni_cmd->ucBandIdx);
#endif
		} else {
			DBGLOG(NIC, WARN, "[CSI] No Support CSI CfgItem:%d\n",
							cmd->ucCfgItem);
			return WLAN_STATUS_NOT_ACCEPTED;
		}
	}
		break;
	default: {
		DBGLOG(NIC, WARN, "[CSI] No Support CSI Mode:%d\n",
				cmd->ucMode);
		return WLAN_STATUS_NOT_ACCEPTED;
	}
		break;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
#else
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}

#if (CFG_VOLT_INFO == 1)
uint32_t nicUniCmdSendVnf(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SEND_VOLT_INFO_T *cmd;
	struct UNI_CMD_SEND_VOLT_INFO *uni_cmd;
	struct UNI_CMD_SEND_VOLT_INFO_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SEND_VOLT_INFO) +
		sizeof(struct UNI_CMD_SEND_VOLT_INFO_PARAM);

	if (info->ucCID != CMD_ID_SEND_VOLT_INFO ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SEND_VOLT_INFO_T *)
		info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SEND_VOLT_INFO,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SEND_VOLT_INFO *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_SEND_VOLT_INFO_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SEND_VOLT_INFO_TAG_BASIC;
	tag->u2Length = sizeof(*tag);

	kalMemCopy(&tag->rVolt, cmd, sizeof(tag->rVolt));

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_VOLT_INFO */

#if CFG_FAST_PATH_SUPPORT
uint32_t nicUniCmdFastPath(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_FAST_PATH *cmd;
	struct UNI_CMD_FAST_PATH *uni_cmd;
	struct UNI_CMD_FAST_PATH_PROCESS_T *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_FAST_PATH) +
			       sizeof(struct UNI_CMD_FAST_PATH_PROCESS_T);

	if (info->ucCID != CMD_ID_FAST_PATH ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_FAST_PATH *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_FAST_PATH,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_FAST_PATH *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_FAST_PATH_PROCESS_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_FAST_PATH_PROCESS;
	tag->u2Length = sizeof(*tag);

	kalMemCopy(tag->aucOwnMac, cmd->aucOwnMac, MAC_ADDR_LEN);
	tag->u2RandomNum = cmd->u2RandomNum;
	kalMemCopy(tag->u4Keybitmap, cmd->u4Keybitmap, KEY_BITMAP_LEN_BYTE);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_PKT_OFLD
uint32_t nicUniCmdPktOfldOp(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_PKT_OFLD *uni_cmd;
	struct UNI_CMD_PKT_OFLD_GENERAL_OP *tag;
	struct PARAM_OFLD_INFO *prInfo;

	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PKT_OFLD) +
			       sizeof(struct UNI_CMD_PKT_OFLD_GENERAL_OP);

	if (info->ucCID != CMD_ID_PKT_OFLD ||
		info->u4SetQueryInfoLen != sizeof(*prInfo))
		return WLAN_STATUS_NOT_ACCEPTED;

	prInfo = (struct PARAM_OFLD_INFO *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_PKT_OFLD, max_cmd_len,
			info->fgNeedResp ? nicUniEventQueryOfldInfo :
			nicUniCmdEventSetCommon,
			nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_PKT_OFLD *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_PKT_OFLD_GENERAL_OP *) uni_cmd->aucTlvBuffer;

	if (prInfo->ucType == PKT_OFLD_TYPE_APF) {
		if (info->fgNeedResp)
			tag->u2Tag = UNI_CMD_PKT_OFLD_TAG_APF_QUERY;
		else
			tag->u2Tag = UNI_CMD_PKT_OFLD_TAG_APF_INSTALL;
	} else if (prInfo->ucType == PKT_OFLD_TYPE_IGMP) {
		tag->u2Tag = UNI_CMD_PKT_OFLD_TAG_IGMP_OFLD;
	}

	tag->u2Length = sizeof(*tag);
	tag->ucType = prInfo->ucType;
	tag->ucOp = prInfo->ucOp;
	tag->ucFragNum = prInfo->ucFragNum;
	tag->ucFragSeq = prInfo->ucFragSeq;
	tag->u4TotalLen = prInfo->u4TotalLen;
	tag->u4BufLen = prInfo->u4BufLen;
	kalMemCopy(tag->aucBuf, prInfo->aucBuf,
			prInfo->u4BufLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif
#if (CFG_CE_ASSERT_DUMP == 1)
void initCECoredump(struct ADAPTER *ad)
{
	uint16_t len = 0;
#if defined(_HIF_PCIE)
	struct BUS_INFO *prBusInfo;

	prBusInfo = ad->chip_info->bus_info;
#endif
	DBGLOG(NIC, DEBUG, "##### Start Coredump!\n");
	DBGLOG(NIC, DEBUG, "manifest: %s\n", ad->rVerInfo.aucReleaseManifest);

	ad->fgKeepPrintCoreDump = TRUE;
	ad->fgN9AssertDumpOngoing = TRUE;

#if defined(_HIF_PCIE)
	if (prBusInfo->bypassWfWdt)
		prBusInfo->bypassWfWdt(ad, TRUE);
#endif
	len = kalStrnLen(ad->rVerInfo.aucReleaseManifest,
		sizeof(ad->rVerInfo.aucReleaseManifest));

	kalEnqCoreDumpLog(ad, ";", kalStrLen(";"));
	kalEnqCoreDumpLog(ad, ad->rVerInfo.aucReleaseManifest, len);
	kalEnqCoreDumpLog(ad, "\n\0", kalStrLen("\n\0"));

	wlanCorDumpTimerInit(ad);
}

void appendCECoredump(struct ADAPTER *ad, uint8_t *buf, uint16_t len)
{
#if defined(_HIF_PCIE)
	struct BUS_INFO *prBusInfo;

	prBusInfo = ad->chip_info->bus_info;
#endif
	if (!kalStrnCmp(buf, ";;[CONNSYS] coredump start", 26))
		ad->fgKeepPrintCoreDump = FALSE;

	if (ad->fgKeepPrintCoreDump) {
		DBGLOG(NIC, DEBUG, "%s", buf);
		DBGLOG_MEM32(NIC, TRACE, buf, len);
	}

	kalEnqCoreDumpLog(ad, buf, len);

	wlanCorDumpTimerReset(ad);

	if (kalStrStr(buf, "; coredump end")) {
		DBGLOG(NIC, DEBUG, "##### Finish Coredump!\n");
		ad->fgN9AssertDumpOngoing = FALSE;

		cnmTimerStopTimer(ad, &ad->rN9CorDumpTimer);
#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
		if (wlanBtCoreDumpInfo(FALSE, FALSE) == TRUE) {
			/* Reset the BT coredump ctrl info */
			wlanBtCoreDumpInfo(TRUE, FALSE);
		} else
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */
		{
			GL_DEFAULT_RESET_TRIGGER(ad, RST_FW_ASSERT);
		}
#if defined(_HIF_PCIE)
		if (prBusInfo->bypassWfWdt)
			prBusInfo->bypassWfWdt(ad, FALSE);
#endif
	}
}

void nicUniEventAssertDump(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len = 0;
	uint8_t *tag = NULL;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_ASSERT_DUMP);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t *dump = NULL;
	uint32_t len = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;

	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_ASSERT_DUMP_BASIC:
			if (!ad->fgN9AssertDumpOngoing) {
				initCECoredump(ad);

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
				if (kalMemCmp(tag + sizeof(struct TAG_HDR),
					"bt_coredump", 11))
					wlanBtCoreDumpInfo(TRUE, FALSE);
				else
					wlanBtCoreDumpInfo(TRUE, TRUE);

				if (wlanBtCoreDumpInfo(FALSE, FALSE) == TRUE) {
					uint8_t ucAction =
						CMD_BT_CTRL_GET_COREDUMP_HEADER;
					wlanoidBtCoreDumpCtrl(ad, &ucAction,
								0, NULL);
				}
#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

				/* skip first line (disable cache) */
				break;
			}

			len = TAG_LEN(tag) - sizeof(struct TAG_HDR);
			dump = kalMemAlloc(len + 1, VIR_MEM_TYPE);
			if (!dump) {
				DBGLOG(NIC, WARN, "len=%d OOM\n", len);
				break;
			}
			kalMemCopy(dump, tag + sizeof(struct TAG_HDR), len);
			dump[len] = '\0';
			appendCECoredump(ad, dump, len + 1);

			kalMemFree(dump, VIR_MEM_TYPE, len + 1);
			break;
		default:
			DBGLOG(NIC, WARN, "unimplement tag:%d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

uint32_t nicUniCmdKeepAlive(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct PARAM_PACKET_KEEPALIVE_T *cmd;
	struct UNI_CMD_KEEP_ALIVE *uni_cmd;
	struct UNI_CMD_KEEP_ALIVE_SET *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_KEEP_ALIVE) +
		sizeof(struct UNI_CMD_KEEP_ALIVE_SET);

	if (info->ucCID != CMD_ID_WFC_KEEP_ALIVE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct PARAM_PACKET_KEEPALIVE_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_KEEP_ALIVE,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_KEEP_ALIVE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_KEEP_ALIVE_SET *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_KEEP_ALIVE_TAG_SET;
	tag->u2Length = sizeof(*tag);
	tag->fgEnable = cmd->enable;
	tag->ucIndex = cmd->index;
	tag->u2IpPktLen = cmd->u2IpPktLen;
	kalMemCopy(tag->pIpPkt, cmd->pIpPkt, sizeof(tag->pIpPkt));
	COPY_MAC_ADDR(tag->ucSrcMacAddr, cmd->ucSrcMacAddr);
	COPY_MAC_ADDR(tag->ucDstMacAddr, cmd->ucDstMacAddr);
	tag->u4PeriodMsec = cmd->u4PeriodMsec;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
uint32_t nicUniCmdMdnsRecordeEnable(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info, uint32_t max_cmd_len)
{
	struct CMD_MDNS_PARAM_T *cmd;
	struct UNI_CMD_MDNS_RECORDE *uni_cmd;
	struct UNI_CMD_MDNS_RECORDE_ENABLE *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

	cmd = (struct CMD_MDNS_PARAM_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MDNS_RECORD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MDNS_RECORDE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MDNS_RECORDE_ENABLE *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MDNS_RECORDE_TAG_ENABLE;
	tag->u2Length = sizeof(*tag);
	if (cmd->ucCmd == MDNS_CMD_DISABLE)
		tag->EnableFlag = FALSE;
	else if (cmd->ucCmd == MDNS_CMD_ENABLE) {
		tag->EnableFlag = TRUE;

		tag->ucWakeFlag = cmd->ucWakeFlag;
	    kalMemCopy(&(tag->aucMdnsMacHdr),
				&(cmd->aucMdnsMacHdr),
				sizeof(tag->aucMdnsMacHdr));
		kalMemCopy(tag->aucMdnsIPHdr,
				cmd->aucMdnsIPHdr,
				sizeof(tag->aucMdnsIPHdr));
		kalMemCopy(tag->aucMdnsUdpHdr,
				cmd->aucMdnsUdpHdr,
				sizeof(tag->aucMdnsUdpHdr));
	}
	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdMdnsRecordeGetHit(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info, uint32_t max_cmd_len)
{
	struct CMD_MDNS_PARAM_T *cmd;
	struct UNI_CMD_MDNS_RECORDE *uni_cmd;
	struct UNI_CMD_MDNS_RECORDE_GET_HIT *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

	cmd = (struct CMD_MDNS_PARAM_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MDNS_RECORD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MDNS_RECORDE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MDNS_RECORDE_GET_HIT *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MDNS_RECORDE_TAG_GET_HIT;
	tag->u2Length = sizeof(*tag);
	tag->ucRecordId = cmd->ucRecordId;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdMdnsRecordeGetMiss(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info, uint32_t max_cmd_len)
{
	struct CMD_MDNS_PARAM_T *cmd;
	struct UNI_CMD_MDNS_RECORDE *uni_cmd;
	struct UNI_CMD_MDNS_RECORDE_GET_MISS *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

	cmd = (struct CMD_MDNS_PARAM_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MDNS_RECORD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MDNS_RECORDE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MDNS_RECORDE_GET_MISS *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MDNS_RECORDE_TAG_GET_MISS;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdMdnsRecordeIpv6WakeUp(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info, uint32_t max_cmd_len)
{
	struct CMD_MDNS_PARAM_T *cmd;
	struct UNI_CMD_MDNS_RECORDE *uni_cmd;
	struct UNI_CMD_MDNS_RECORDE_IPV6_WAKEUP *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;

	cmd = (struct CMD_MDNS_PARAM_T *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_MDNS_RECORD,
		max_cmd_len, nicUniCmdEventSetCommon, nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_MDNS_RECORDE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MDNS_RECORDE_IPV6_WAKEUP *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MDNS_RECORDE_TAG_IPV6_WAKEUP;
	tag->u2Length = sizeof(*tag);
	tag->ucWakeFlag = cmd->ucWakeFlag;
	tag->ucPassthroughBehavior = cmd->ucPassthroughBehavior;
	tag->ucIPV6WakeupFlag = cmd->ucIPV6WakeupFlag;
	tag->ucPayloadOrder = cmd->ucPayloadOrder;
	tag->u2PayloadTotallength = cmd->u2PayloadTotallength;
	kalMemCopy(tag->ucPayload,
			cmd->ucPayload,
			sizeof(tag->ucPayload));
	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdMdnsRecorde(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_MDNS_PARAM_T *cmd;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_MDNS_RECORDE);
	uint32_t u4status = WLAN_STATUS_SUCCESS;

	if (info->ucCID != CMD_ID_SET_MDNS_RECORD ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_MDNS_PARAM_T *) info->pucInfoBuffer;
	switch (cmd->ucCmd) {
	case MDNS_CMD_ENABLE:
		max_cmd_len += sizeof(struct UNI_CMD_MDNS_RECORDE_ENABLE);
		u4status = nicUniCmdMdnsRecordeEnable(ad, info, max_cmd_len);
		break;
	case MDNS_CMD_DISABLE:
		max_cmd_len += sizeof(struct UNI_CMD_MDNS_RECORDE_ENABLE);
		u4status = nicUniCmdMdnsRecordeEnable(ad, info, max_cmd_len);
		break;
	case MDNS_CMD_GET_HITCOUNTER:
		max_cmd_len += sizeof(struct UNI_CMD_MDNS_RECORDE_GET_HIT);
		u4status = nicUniCmdMdnsRecordeGetHit(ad, info, max_cmd_len);
		break;
	case MDNS_CMD_GET_MISSCOUNTER:
		max_cmd_len += sizeof(struct UNI_CMD_MDNS_RECORDE_GET_MISS);
		u4status = nicUniCmdMdnsRecordeGetMiss(ad, info, max_cmd_len);
		break;
	case MDNS_CMD_SET_IPV6_WAKEUP_FLAG:
		max_cmd_len += sizeof(struct UNI_CMD_MDNS_RECORDE_IPV6_WAKEUP);
		u4status =
			nicUniCmdMdnsRecordeIpv6WakeUp(ad, info, max_cmd_len);
		break;
	default:
		break;
	}
	return u4status;
}
#endif
#endif

uint32_t nicUniCmdLpDbgCtrl(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_LP_DBG_CTRL *cmd;
	struct UNI_CMD_LP_DBG_CTRL *uni_cmd;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_LP_DBG_CTRL);

	if (info->ucCID != CMD_ID_LP_DBG_CTRL ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_LP_DBG_CTRL *) info->pucInfoBuffer;

	switch (cmd->ucSubCmdId) {
	case LP_CMD_QUERY:
		if (cmd->ucTag == LP_TAG_GET_SLP_CNT_INFO)
			max_cmd_len +=
			sizeof(struct UNI_CMD_LP_GET_SLP_CNT_INFO);
		break;
	case LP_CMD_SET:
		if (cmd->ucTag == LP_TAG_SET_KEEP_PWR_CTRL)
			max_cmd_len += sizeof(struct UNI_CMD_LP_KEEP_PWR_CTRL);
		break;
	default:
		DBGLOG(NIC, ERROR, "unknown subCmd %d\n", cmd->ucSubCmdId);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_LP_DBG_CTRL, max_cmd_len,
			nicUniCmdEventLpDbgCtrl, nicUniCmdTimeoutCommon);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_LP_DBG_CTRL *) entry->pucInfoBuffer;
	switch (cmd->ucSubCmdId) {
	case LP_CMD_QUERY: {
		if (cmd->ucTag == LP_TAG_GET_SLP_CNT_INFO) {
			struct UNI_CMD_LP_GET_SLP_CNT_INFO *tag =
				(struct UNI_CMD_LP_GET_SLP_CNT_INFO *)
				uni_cmd->aucTlvBuffer;

			tag->u2Tag = UNI_CMD_LP_DBG_CTRL_TAG_GET_SLP_CNT_INFO;
			tag->u2Length = sizeof(*tag);
		}
	}
		break;
	case LP_CMD_SET: {
		if (cmd->ucTag == LP_TAG_SET_KEEP_PWR_CTRL) {
			struct UNI_CMD_LP_KEEP_PWR_CTRL *tag =
				(struct UNI_CMD_LP_KEEP_PWR_CTRL *)
				uni_cmd->aucTlvBuffer;

			tag->u2Tag = UNI_CMD_LP_DBG_CTRL_TAG_KEEP_PWR_CTRL;
			tag->u2Length = sizeof(*tag);
			tag->ucBandIdx = cmd->ucBandIdx;
			tag->ucKeepPwr = cmd->ucKeepPwr;
		}
	}
		break;
	default:
		break;
	}

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdGamingMode(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_FORCE_RTS *cmd;
	struct UNI_CMD_GAMING_MODE *uni_cmd;
	struct UNI_CMD_GAMING_MODE_PROCESS_T *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GAMING_MODE) +
		sizeof(struct UNI_CMD_GAMING_MODE_PROCESS_T);

	if (info->ucCID != CMD_ID_SET_FORCE_RTS ||
		info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_SET_FORCE_RTS *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GAMING_MODE,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GAMING_MODE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_GAMING_MODE_PROCESS_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_GAMING_MODE_PROCESS;
	tag->u2Length = sizeof(*tag);

	tag->ucForceRtsEn = cmd->ucForceRtsEn;
	tag->ucRtsPktNum = cmd->ucRtsPktNum;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#if (CFG_SAP_RPS_SUPPORT == 1)
uint32_t nicUniCmdSetSapRps(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_FW_SAP_RPS *cmd;
	struct UNI_CMD_SAP *uni_cmd;
	struct UNI_CMD_SET_SAP_RPS_SET_T *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SAP) +
		sizeof(struct UNI_CMD_SET_SAP_RPS_SET_T);

	cmd = (struct CMD_SET_FW_SAP_RPS *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SET_SAP,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SAP *) entry->pucInfoBuffer;
	uni_cmd->ucBssIdx = cmd->ucBssIdx;
	tag = (struct UNI_CMD_SET_SAP_RPS_SET_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SET_SAP_RPS_TAG_SET;
	tag->u2Length = sizeof(*tag);

	tag->ucPhase = cmd->ucSapRpsPhase;
	tag->fgEnable = cmd->ucForceSapRpsEn;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SAP_SUS_SUPPORT == 1)
uint32_t nicUniCmdSetSapSus(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_SET_FW_SAP_SUS *cmd;
	struct UNI_CMD_SAP *uni_cmd;
	struct UNI_CMD_SET_SAP_SUS_SET_T *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SAP) +
		sizeof(struct UNI_CMD_SET_SAP_SUS_SET_T);

	cmd = (struct CMD_SET_FW_SAP_SUS *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_SET_SAP,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_SAP *) entry->pucInfoBuffer;
	uni_cmd->ucBssIdx = cmd->ucBssIdx;
	tag = (struct UNI_CMD_SET_SAP_SUS_SET_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SET_SAP_SUS_TAG_SET;
	tag->u2Length = sizeof(*tag);

	tag->fgEnable = cmd->ucForceSapSusEn;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_SUPPORT_SAP_BCN_CRI_UPD == 1)
uint32_t nicUniCmdSapBcnCriUpd(struct ADAPTER *ad, uint8_t idx)
{
	struct UNI_CMD_SAP *uni_cmd;
	struct UNI_CMD_SAP_BCN_CRI_UPD_T *tag;
	struct BSS_INFO *bss;
	struct MLD_BSS_INFO *mld_bss;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_SAP) +
		sizeof(struct UNI_CMD_SAP_BCN_CRI_UPD_T);

	if (!ad || !IS_BSS_INDEX_VALID(idx)) {
		DBGLOG(INIT, ERROR,
		       "ad=0x%p idx=%u\n",
		       ad, idx);
		return WLAN_STATUS_INVALID_DATA;
	}

	bss = GET_BSS_INFO_BY_INDEX(ad, idx);
	if (!bss) {
		DBGLOG(INIT, ERROR,
		       "get bss failed idx=%u\n",
		       idx);
		return WLAN_STATUS_INVALID_DATA;
	}

	mld_bss = mldBssGetByBss(ad, bss);
	if (!mld_bss) {
		DBGLOG(INIT, ERROR,
		       "get mld bss failed idx=%u\n",
		       idx);
		return WLAN_STATUS_INVALID_DATA;
	}

	uni_cmd = (struct UNI_CMD_SAP *) cnmMemAlloc(ad, RAM_TYPE_MSG,
						     max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_SAP (%u) ==> FAILED.\n",
		       max_cmd_len);
		return WLAN_STATUS_RESOURCES;
	}

	uni_cmd->ucBssIdx = idx;
	tag = (struct UNI_CMD_SAP_BCN_CRI_UPD_T *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_SAP_BCN_CRI_UPD;
	tag->u2Length = sizeof(*tag);
	LINK_FOR_EACH_ENTRY(bss, &mld_bss->rBssList,
			    rLinkEntryMld, struct BSS_INFO) {
		tag->u2UpdateBssBitmap |= BIT(bss->ucBssIndex);
	}

	wlanSendSetQueryUniCmd(ad,
			       UNI_CMD_ID_SET_SAP,
			       TRUE,
			       FALSE,
			       FALSE,
			       nicUniCmdEventSetCommon,
			       nicUniCmdTimeoutCommon,
			       max_cmd_len,
			       (void *)uni_cmd,
			       NULL,
			       0);

	cnmMemFree(ad, uni_cmd);
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_SAP_BCN_CRI_UPD */

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
uint32_t nicUniCmdPowerLimitEmiInfo(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_EMI_POWER_LIMIT_FORMAT *cmd;
	struct UNI_CMD_POWER_LIMIT *uni_cmd;
	struct UNI_CMD_SET_PWR_LIMIT_EMI_INFO *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_POWER_LIMIT) +
		sizeof(struct UNI_CMD_SET_PWR_LIMIT_EMI_INFO);

	if (info->ucCID != CMD_ID_SET_PWR_LIMIT_EMI_INFO)
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_EMI_POWER_LIMIT_FORMAT *)
		info->pucInfoBuffer;

	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_POWER_LIMIT,
		max_cmd_len,
		NULL,
		NULL);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_POWER_LIMIT *) entry->pucInfoBuffer;

	tag = (struct UNI_CMD_SET_PWR_LIMIT_EMI_INFO *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_POWER_LIMIT_EMI_INFO;

	tag->u2Length = sizeof(tag->u2Tag)
		+ sizeof(tag->u2Length)
		+ info->u4SetQueryInfoLen;

	kalMemCopy(&tag->config, cmd, info->u4SetQueryInfoLen);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

uint32_t nicUniCmdPpEnCtrl(struct ADAPTER *ad, uint8_t ucMode,
			   uint8_t ucDbdxIdx, uint8_t ucCtrl,
			   uint8_t ucMgmtEn, uint16_t u2Bitmap,
			   u_int8_t fgIsOid)
{
	struct UNI_CMD_PP *uni_cmd;
	struct UNI_CMD_PP_EN_CTRL_T *tag;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_PP) +
			       sizeof(struct UNI_CMD_PP_EN_CTRL_T);
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (ad == NULL)
		return WLAN_STATUS_ADAPTER_NOT_READY;

	uni_cmd = (struct UNI_CMD_PP *)
		cnmMemAlloc(ad, RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(NIC, ERROR,
		       "Allocate UNI_CMD_PP (%u) ==> FAILED.\n",
		       max_cmd_len);
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_PP_EN_CTRL_T *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_PP_TAG_EN_CTRL;
	tag->u2Length = sizeof(*tag);
	tag->u1DbdcIdx = ucDbdxIdx;
	tag->u1PpMgmtMode = ucMode;
	tag->u1PpMgmtEn = ucMgmtEn;
	tag->u1PpCtrl = ucCtrl;
	tag->u1PpBitMap = u2Bitmap;

	DBGLOG(NIC, DEBUG,
		"mode=%u dbdc=%u ctrl=%u mgmt_en=%u bitmap=%u oid=%u\n",
		ucMode, ucDbdxIdx, ucCtrl, ucMgmtEn, u2Bitmap, fgIsOid);

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_PP,
					TRUE,
					FALSE,
					fgIsOid,
					nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon,
					max_cmd_len,
					(void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);

	return status;
}

uint32_t nicUniCmdPpAlgoCtrl(struct ADAPTER *ad,
			     struct UNI_CMD_PP_ALG_CTRL *para,
			     u_int8_t fgIsOid)
{
	struct UNI_CMD_PP *uni_cmd;
	struct UNI_CMD_PP_ALG_CTRL *tag;
	uint32_t u4MaxCmdLen = sizeof(struct UNI_CMD_PP) +
			       sizeof(struct UNI_CMD_PP_ALG_CTRL);
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (!ad) {
		DBGLOG(NIC, ERROR,
			"prAdapter is null !!!!\n");
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd = (struct UNI_CMD_PP *)
		cnmMemAlloc(ad, RAM_TYPE_MSG, u4MaxCmdLen);
	if (!uni_cmd) {
		DBGLOG(NIC, ERROR,
			"uni_cmd is null !!!!\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (struct UNI_CMD_PP_ALG_CTRL *)uni_cmd->aucTlvBuffer;
	kalMemCopy(tag, para, sizeof(*tag));
	tag->u2Tag = UNI_CMD_PP_TAG_ALG_CTRL;
	tag->u2Length = sizeof(*tag);

	status = wlanSendSetQueryUniCmd(ad,
					UNI_CMD_ID_PP,
					TRUE,
					FALSE,
					fgIsOid,
					nicUniCmdEventSetCommon,
					nicUniCmdTimeoutCommon,
					u4MaxCmdLen,
					(void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);

	return status;
}

#if (CFG_SUPPORT_FACT_CAL == 1)
uint32_t nicUniCmdFactCal(struct ADAPTER *prAdapter,
		uint32_t u4Action,
		struct UNI_CMD_FACT_CAL_DATA *prCalData)
{
	struct UNI_CMD_FACT_CAL *uni_cmd = NULL;
	uint32_t max_cmd_len = 0;
	uint32_t status = WLAN_STATUS_SUCCESS;

	DBGLOG(NIC, TRACE, "u4Action=%u, ucCalType=%u, u4CalParam=0x%x\n",
			u4Action, prCalData->ucCalType, prCalData->u4Data);

	/* Alloc cmd mem */
	if (u4Action == FACT_CAL_ACTION_GET) {
		max_cmd_len = sizeof(struct UNI_CMD_FACT_CAL) +
				sizeof(struct UNI_CMD_FACT_CAL_RAPID_GET);
	} else if (u4Action == FACT_CAL_ACTION_SET) {
		max_cmd_len = sizeof(struct UNI_CMD_FACT_CAL) +
				sizeof(struct UNI_CMD_FACT_CAL_RAPID_SET);
	} else {
		DBGLOG(NIC, ERROR, "Error Action Type=%d\n", u4Action);
		return WLAN_STATUS_FAILURE;
	}

	uni_cmd = (struct UNI_CMD_FACT_CAL *)cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(NIC, ERROR, "Allocate UNI_CMD_FACT_CAL FAILED\n");
		return WLAN_STATUS_FAILURE;
	}

	switch (u4Action) {
	case FACT_CAL_ACTION_GET: {
		struct UNI_CMD_FACT_CAL_RAPID_GET *tag =
			(struct UNI_CMD_FACT_CAL_RAPID_GET *)
				uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_FACT_CAL_TAG_RAPID_GET;
		tag->u2Length = sizeof(*tag);
		tag->u4Data = prCalData->u4Data;
		tag->u1CalType = prCalData->ucCalType;
		tag->u1Band = prCalData->ucBand;
		tag->u4Channel = prCalData->u4Channel;
		break;
	}

	case FACT_CAL_ACTION_SET: {
		struct UNI_CMD_FACT_CAL_RAPID_SET *tag =
			(struct UNI_CMD_FACT_CAL_RAPID_SET *)
				uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_FACT_CAL_TAG_RAPID_SET;
		tag->u2Length = sizeof(*tag);
		tag->u1CalType = prCalData->ucCalType;
		tag->u4Data = prCalData->u4Data;
		tag->u4SeqNum = prCalData->u4SeqNum;
		tag->u1Done = prCalData->ucDone;
		if (prCalData->u4BufDataLength <= FACT_CAL_DATA_BUF_MAXSIZE)
			kalMemCopy(tag->u1BufData,
				prCalData->aucBufData,
				prCalData->u4BufDataLength);
		else {
			DBGLOG(NIC, ERROR,
				"Data size %d is bigger than max size %d\n",
				prCalData->u4BufDataLength,
				FACT_CAL_DATA_BUF_MAXSIZE);
			return WLAN_STATUS_FAILURE;
		}
		tag->u4BufDataLength = prCalData->u4BufDataLength;
		break;
	}

	default:
		DBGLOG(NIC, ERROR, "Error Action Type=%d\n", u4Action);
		return WLAN_STATUS_FAILURE;
	}

	status = wlanSendSetQueryUniCmd(prAdapter,
				UNI_CMD_ID_FACT_CAL,
				TRUE,
				FALSE,
				FALSE,
				nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon,
				max_cmd_len,
				(uint8_t *)uni_cmd, NULL, 0);

	cnmMemFree(prAdapter, uni_cmd);

	/* Convert WLAN_STATUS_PENDING to success */
	if (status == WLAN_STATUS_PENDING)
		status = WLAN_STATUS_SUCCESS;

	return status;
}
#endif //CFG_SUPPORT_FACT_CAL

/*******************************************************************************
 *                                 Event
 *******************************************************************************
 */

void nicUniEventHelper(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt,
	uint8_t eid, uint8_t ext_id, uint8_t *data, uint32_t len)
{
	struct WIFI_EVENT *legacy;
	uint32_t size = sizeof(*legacy) + len;
	uint8_t i;

	legacy = (struct WIFI_EVENT *) kalMemAlloc(size, VIR_MEM_TYPE);
	if (!legacy) {
		DBGLOG(NIC, WARN, "eid=%d ext_id=%d OOM\n", eid, ext_id);
		return;
	}

	legacy->u2PacketLength = size;
	legacy->u2PacketType = evt->u2PacketType;
	legacy->ucEID = eid;
	legacy->ucSeqNum = evt->ucSeqNum;
	legacy->ucEventVersion = evt->ucOption;
	legacy->ucExtenEID = ext_id;
	legacy->ucS2DIndex = evt->ucS2DIndex;

	if (data && len)
		kalMemCopy(legacy->aucBuffer, data, len);

	for (i = 0; i < arEventTableSize; i++) {
		if (eid == arEventTable[i].eEID) {
			arEventTable[i].pfnHandler(ad, legacy);
			break;
		}
	}

	kalMemFree(legacy, VIR_MEM_TYPE, size);
}

/*******************************************************************************
 *                   Solicited Event
 *******************************************************************************
 */

void nicRxProcessUniEventPacket(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb)
{
	struct mt66xx_chip_info *prChipInfo;
	struct CMD_INFO *prCmdInfo;
	struct WIFI_UNI_EVENT *prEvent;

	ASSERT(prAdapter);
	ASSERT(prSwRfb);
	prChipInfo = prAdapter->chip_info;
	prEvent = (struct WIFI_UNI_EVENT *)
			(prSwRfb->pucRecvBuff + prChipInfo->rxd_size);

	if (prEvent->u2PacketLength > RX_GET_PACKET_MAX_SIZE(prAdapter)
		|| prEvent->u2PacketLength <
			(sizeof(struct WIFI_UNI_EVENT) + TAG_HDR_LEN)) {
		DBGLOG(NIC, ERROR,
			"Invalid RX uni event: ID[0x%02X] SEQ[%u] LEN[%u] OPT[0x%x]\n",
			prEvent->ucEID, prEvent->ucSeqNum,
			prEvent->u2PacketLength, prEvent->ucOption);
		nicRxReturnRFB(prAdapter, prSwRfb);
		return;
	}

	if (prEvent->ucEID != UNI_EVENT_ID_FW_LOG_2_HOST) {
		DBGLOG(NIC, TRACE,
			"RX UNI EVENT: ID[0x%02X] SEQ[%u] LEN[%u] OPT[0x%x]\n",
			prEvent->ucEID, prEvent->ucSeqNum,
			prEvent->u2PacketLength, prEvent->ucOption);
	}

#if (CFG_SUPPORT_STATISTICS == 1)
	wlanWakeLogEvent(prAdapter->prGlueInfo, prEvent->ucEID);
#endif

	if (IS_UNI_UNSOLICIT_EVENT(prEvent)) {
		if (GET_UNI_EVENT_ID(prEvent) < UNI_EVENT_ID_NUM) {
			if (arUniEventTable[GET_UNI_EVENT_ID(prEvent)])
				arUniEventTable[GET_UNI_EVENT_ID(prEvent)](
					prAdapter, prEvent);
		} else {
			DBGLOG(RX, TRACE,
				"UNHANDLED RX UNI EVENT: ID[0x%02X]\n",
				GET_UNI_EVENT_ID(prEvent));
		}
	} else {
		prCmdInfo = nicGetPendingCmdInfo(prAdapter,
						prEvent->ucSeqNum);

		if (prCmdInfo != NULL) {
			/* input prEvent instead of prEvent->aucBuffer because
			 * handler needs the size of buffer to parse tags
			 */
			if (prCmdInfo->pfCmdDoneHandler)
				prCmdInfo->pfCmdDoneHandler(
					prAdapter, prCmdInfo,
					(uint8_t *)prEvent);
			else if (prCmdInfo->fgIsOid)
				kalOidComplete(
					prAdapter->prGlueInfo,
					prCmdInfo,
					0,
					WLAN_STATUS_SUCCESS);

			/* return prCmdInfo */
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		} else {
			DBGLOG(RX, TRACE,
				"UNHANDLED RX EVENT: ID[0x%02X] SEQ[%u] LEN[%u]\n",
			  prEvent->ucEID, prEvent->ucSeqNum,
			  prEvent->u2PacketLength);
		}
	}

	/* Reset Chip NoAck flag */
	if (prAdapter->fgIsChipNoAck) {
		DBGLOG_LIMITED(RX, WARN,
		       "Got response from chip, clear NoAck flag!\n");
		KAL_WARN_ON(TRUE);
	}
	prAdapter->ucOidTimeoutCount = 0;
	prAdapter->fgIsChipNoAck = FALSE;

	nicRxReturnRFB(prAdapter, prSwRfb);
}

void nicUniCmdEventSetCommon(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{
	DBGLOG(NIC, TRACE, "prCmdInfo:ucCID=0x%x SQ=%d NR=%d OID=%d\n",
		prCmdInfo->ucCID, prCmdInfo->fgSetQuery,
		prCmdInfo->fgNeedResp, prCmdInfo->fgIsOid);

	if (!prCmdInfo->fgSetQuery || prCmdInfo->fgNeedResp) {
		struct WIFI_UNI_EVENT *evt =
			(struct WIFI_UNI_EVENT *) pucEventBuf;

		DBGLOG(NIC, TRACE, "Resp:EID=0x%x seq=%d option=%d\n",
			evt->ucEID, evt->ucSeqNum, evt->ucOption);
		nicCmdEventSetCommon(prAdapter, prCmdInfo, evt->aucBuffer);
	} else {
		nicCmdEventSetCommon(prAdapter, prCmdInfo, pucEventBuf);
	}
}

void nicUniCmdTimeoutCommon(struct ADAPTER *prAdapter,
			    struct CMD_INFO *prCmdInfo)
{
	DBGLOG(NIC, TRACE, "prCmdInfo:ucCID=0x%x SQ=%d NR=%d OID=%d\n",
		prCmdInfo->ucCID, prCmdInfo->fgSetQuery,
		prCmdInfo->fgNeedResp, prCmdInfo->fgIsOid);
	nicOidCmdTimeoutCommon(prAdapter, prCmdInfo);
}

void nicUniCmdEventQueryCfgRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_CHIP_CONFIG *evt =
		(struct UNI_EVENT_CHIP_CONFIG *)uni_evt->aucBuffer;
	struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG *tag =
		(struct UNI_CMD_CHIP_CONFIG_CUSTOMER_CFG *) evt->aucTlvBuffer;
	struct CMD_HEADER legacy = {0};

	legacy.itemNum = tag->itemNum;
	legacy.cmdBufferLen = tag->cmdBufferLen;
	kalMemCopy(legacy.buffer, tag->aucbuffer, tag->cmdBufferLen);

	nicCmdEventQueryCfgRead(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

void nicUniEventQueryChipConfig(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_CHIP_CONFIG *evt =
		(struct UNI_EVENT_CHIP_CONFIG *)uni_evt->aucBuffer;
	struct UNI_CMD_CHIP_CONFIG_CHIP_CFG *tag =
		(struct UNI_CMD_CHIP_CONFIG_CHIP_CFG *) evt->aucTlvBuffer;
	struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP *resp =
		(struct UNI_CMD_CHIP_CONFIG_CHIP_CFG_RESP *) tag->aucbuffer;
	struct CMD_CHIP_CONFIG legacy = {0};

	legacy.u2Id = resp->u2Id;
	legacy.ucType = resp->ucType;
	legacy.ucRespType = resp->ucRespType;
	legacy.u2MsgSize = resp->u2MsgSize;
	kalMemCopy(legacy.aucCmd, resp->aucCmd, resp->u2MsgSize);

	nicCmdEventQueryChipConfig(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

void nicUniEventQuerySwDbgCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_CHIP_CONFIG *evt =
		(struct UNI_EVENT_CHIP_CONFIG *)uni_evt->aucBuffer;
	struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL *tag =
		(struct UNI_CMD_CHIP_CONFIG_SW_DBG_CTRL *) evt->aucTlvBuffer;
	struct CMD_SW_DBG_CTRL legacy = {0};

	legacy.u4Id = tag->u4Id;
	legacy.u4Data = tag->u4Data;

	nicCmdEventQuerySwCtrlRead(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

void nicUniCmdStaRecHandleEventPkt(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct UNI_CMD_STAREC *uni_cmd =
		(struct UNI_CMD_STAREC *) GET_UNI_CMD_DATA(prCmdInfo);
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_CMD_RESULT *evt =
		(struct UNI_EVENT_CMD_RESULT *)uni_evt->aucBuffer;

	DBGLOG(NIC, TRACE,
		"cmd_result:ucCID=0x%x, status=%d, wlanidx=%d\n",
		evt->u2CID, evt->u4Status, uni_cmd->ucWlanIdxL);

	if (evt->u2CID == UNI_CMD_ID_STAREC_INFO && evt->u4Status == 0) {
		struct STA_RECORD *prStaRec = cnmGetStaRecByIndex(prAdapter,
			secGetStaIdxByWlanIdx(prAdapter, uni_cmd->ucWlanIdxL));

		if (prStaRec &&
		    IS_BSS_INDEX_AIS(prAdapter, prStaRec->ucBssIndex))
			DBGLOG(NIC, INFO,
				"<CONN> STA_READY bidx=%d widx=%d\n",
				prStaRec->ucBssIndex, prStaRec->ucWlanIndex);

		qmActivateStaRec(prAdapter, prStaRec);
	}
}

void nicUniEventQueryIdcChnl(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo,
		uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_IDC *evt = (struct UNI_EVENT_IDC *)uni_evt->aucBuffer;
	struct UNI_EVENT_MD_SAFE_CHN *tag =
		(struct UNI_EVENT_MD_SAFE_CHN *) evt->aucTlvBuffer;
	struct EVENT_LTE_SAFE_CHN legacy = {0};

	legacy.ucVersion = tag->ucVersion;
	legacy.u4Flags = tag->u4Flags;
	kalMemCopy(legacy.rLteSafeChn.au4SafeChannelBitmask,
		   tag->u4SafeChannelBitmask,
		   sizeof(uint32_t) * ENUM_SAFE_CH_MASK_MAX_NUM);

#if CFG_ENABLE_WIFI_DIRECT
	nicCmdEventQueryLteSafeChn(prAdapter, prCmdInfo, (uint8_t *)&legacy);
#endif
}

#if CFG_SUPPORT_TX_BF
void nicUniEventBFStaRec(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_BF *evt = (struct UNI_EVENT_BF *)uni_evt->aucBuffer;
	struct UNI_EVENT_BF_STA_REC *tag =
		(struct UNI_EVENT_BF_STA_REC *) evt->au1TlvBuffer;
	struct TXBF_PFMU_STA_INFO *info =
		(struct TXBF_PFMU_STA_INFO *) &tag->rTxBfPfmuInfo;

	DBGLOG(RFTEST, DEBUG,
		 "====================================== BF StaRec ========================================\n"
		 "rStaRecBf.u2PfmuId	  = %d\n"
		 "rStaRecBf.fgSU_MU	  = %d\n"
		 "rStaRecBf.u1TxBfCap	  = %d\n"
		 "rStaRecBf.ucSoundingPhy = %d\n"
		 "rStaRecBf.ucNdpaRate	  = %d\n"
		 "rStaRecBf.ucNdpRate	  = %d\n"
		 "rStaRecBf.ucReptPollRate= %d\n"
		 "rStaRecBf.ucTxMode	  = %d\n"
		 "rStaRecBf.ucNc	  = %d\n"
		 "rStaRecBf.ucNr	  = %d\n"
		 "rStaRecBf.ucCBW	  = %d\n"
		 "rStaRecBf.ucTotMemRequire = %d\n"
		 "rStaRecBf.ucMemRequire20M = %d\n"
		 "rStaRecBf.ucMemRow0	  = %d\n"
		 "rStaRecBf.ucMemCol0	  = %d\n"
		 "rStaRecBf.ucMemRow1	  = %d\n"
		 "rStaRecBf.ucMemCol1	  = %d\n"
		 "rStaRecBf.ucMemRow2	  = %d\n"
		 "rStaRecBf.ucMemCol2	  = %d\n"
		 "rStaRecBf.ucMemRow3	  = %d\n"
		 "rStaRecBf.ucMemCol3	  = %d\n",
		 info->u2PfmuId,
		 info->fgSU_MU,
		 info->u1TxBfCap,
		 info->ucSoundingPhy,
		 info->ucNdpaRate,
		 info->ucNdpRate,
		 info->ucReptPollRate,
		 info->ucTxMode,
		 info->ucNc,
		 info->ucNr,
		 info->ucCBW,
		 info->ucTotMemRequire,
		 info->ucMemRequire20M,
		 info->ucMemRow0,
		 info->ucMemCol0,
		 info->ucMemRow1,
		 info->ucMemCol1,
		 info->ucMemRow2,
		 info->ucMemCol2,
		 info->ucMemRow3,
		 info->ucMemCol3);
	DBGLOG(RFTEST, DEBUG,
		 "rStaRecBf.u2SmartAnt	  = 0x%x\n"
		 "rStaRecBf.ucSEIdx	  = %d\n"
		 "rStaRecBf.ucAutoSoundingCtrl = %d\n"
		 "rStaRecBf.uciBfTimeOut  = 0x%x\n"
		 "rStaRecBf.uciBfDBW	  = %d\n"
		 "rStaRecBf.uciBfNcol	  = %d\n"
		 "rStaRecBf.uciBfNrow	  = %d\n"
		 "rStaRecBf.nr_bw160	  = %d\n"
		 "rStaRecBf.nc_bw160	  = %d\n"
		 "rStaRecBf.ru_start_idx  = %d\n"
		 "rStaRecBf.ru_end_idx	  = %d\n"
		 "rStaRecBf.trigger_su	  = %d\n"
		 "rStaRecBf.trigger_mu	  = %d\n"
		 "rStaRecBf.ng16_su	  = %d\n"
		 "rStaRecBf.ng16_mu	  = %d\n"
		 "rStaRecBf.codebook42_su = %d\n"
		 "rStaRecBf.codebook75_mu = %d\n"
		 "rStaRecBf.he_ltf	      = %d\n"
		 "=======================================================================================\n",
		 info->u2SmartAnt,
		 info->ucSEIdx,
		 info->ucAutoSoundingCtrl,
		 info->uciBfTimeOut,
		 info->uciBfDBW,
		 info->uciBfNcol,
		 info->uciBfNrow,
		 info->u1NrBw160,
		 info->u1NcBw160,
		 info->u1RuStartIdx,
		 info->u1RuEndIdx,
		 info->fgTriggerSu,
		 info->fgTriggerMu,
		 info->fgNg16Su,
		 info->fgNg16Mu,
		 info->fgCodebook42Su,
		 info->fgCodebook75Mu,
		 info->u1HeLtf);
	nicCmdEventSetCommon(prAdapter, prCmdInfo, pucEventBuf);
}
#endif

void nicUniCmdEventQueryMcrRead(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct UNI_CMD_ACCESS_REG *uni_cmd =
		(struct UNI_CMD_ACCESS_REG *) GET_UNI_CMD_DATA(prCmdInfo);
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_ACCESS_REG *evt =
		(struct UNI_EVENT_ACCESS_REG *)uni_evt->aucBuffer;
	struct CMD_ACCESS_REG legacy = {0};


	if (TAG_ID(uni_cmd->aucTlvBuffer) == UNI_CMD_ACCESS_REG_TAG_BASIC) {
		struct UNI_EVENT_ACCESS_REG_BASIC *tag =
			(struct UNI_EVENT_ACCESS_REG_BASIC *) evt->aucTlvBuffer;

		legacy.u4Address = tag->u4Addr;
		legacy.u4Data = tag->u4Value;
	} else { /* RF */
		struct UNI_EVENT_ACCESS_RF_REG_BASIC *tag =
		     (struct UNI_EVENT_ACCESS_RF_REG_BASIC *) evt->aucTlvBuffer;

		legacy.u4Address = tag->u4Addr;
		legacy.u4Data = tag->u4Value;
	}

	DBGLOG(RFTEST, DEBUG, "CMD:0x%x read addr=0x%x, value=0x%x\n",
		TAG_ID(uni_cmd->aucTlvBuffer), legacy.u4Address, legacy.u4Data);
	nicCmdEventQueryMcrRead(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

#if (CFG_SUPPORT_TWT == 1)
void nicUniCmdEventGetTsfDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MAC_IFNO *evt =
		(struct UNI_EVENT_MAC_IFNO *)uni_evt->aucBuffer;
	struct UNI_EVENT_MAC_INFO_TSF *tag =
		(struct UNI_EVENT_MAC_INFO_TSF *) evt->aucTlvBuffer;
	struct EXT_EVENT_MAC_INFO_T legacy = {0};
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt;
	struct BSS_INFO *prBssInfo;
#endif

	legacy.rMacInfoResult.rTsfResult.u4TsfBitsLow = tag->u4TsfBit0_31;
	legacy.rMacInfoResult.rTsfResult.u4TsfBitsHigh = tag->u4TsfBit63_32;

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	if (!prAdapter) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prAdapter\n");

		return;
	}

	if (!prCmdInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prMsgHdr\n");

		return;
	}

	prGetTsfCtxt = (struct _TWT_GET_TSF_CONTEXT_T *)
		prCmdInfo->pvInformationBuffer;

	if (!prGetTsfCtxt) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prGetTsfCtxt\n");

		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prGetTsfCtxt->ucBssIdx);

	if (!prBssInfo) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prBssInfo\n");

		return;
	}

	if (IS_BSS_AP(prAdapter, prBssInfo)) {
		twtHotspotPlannerGetTsfDone(prAdapter,
			prCmdInfo, (uint8_t *)&legacy);
	} else {
#endif
		twtPlannerGetTsfDone(prAdapter, prCmdInfo, (uint8_t *)&legacy);
#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	}
#endif
}
#endif

#if (CFG_SUPPORT_TWT_STA_CNM == 1)
void nicUniCmdEventTWTGetCnmGrantedDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MAC_IFNO *evt =
		(struct UNI_EVENT_MAC_IFNO *)uni_evt->aucBuffer;
	struct UNI_EVENT_MAC_INFO_TWT_STA_CNM *tag =
		(struct UNI_EVENT_MAC_INFO_TWT_STA_CNM *) evt->aucTlvBuffer;
	struct _TWT_GET_TSF_CONTEXT_T *prGetTsfCtxt;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIdx, ucFlowId;
	uint8_t ucAgrtTblIdx;
	uint8_t ucFlowId_real;
	enum _ENUM_TWT_TYPE_T eTwtType;

	prGetTsfCtxt = (struct _TWT_GET_TSF_CONTEXT_T *)
		prCmdInfo->pvInformationBuffer;

	if (!prGetTsfCtxt) {
		DBGLOG(TWT_PLANNER, ERROR,
			"Invalid prGetTsfCtxt\n");

		return;
	}

	DBGLOG(TWT_PLANNER, ERROR,
		"TWT STA(%d,%d) R=%d CNM granted result %d\n",
		tag->ucBssIndex,
		tag->ucDbdcIdx,
		prGetTsfCtxt->ucTwtStaCnmReason,
		tag->fgCnmGranted);

	if (prGetTsfCtxt->ucTwtStaCnmReason == TWT_STA_CNM_SETUP) {
		if (tag->fgCnmGranted == TRUE)
			twtPlannerGetCnmGrantedDone(prAdapter, prCmdInfo, NULL);
		else {
			/* Release prGetTsfCtxt by twtPlannerSetParams */
			/* For the CNM granted failure case */
			kalMemFree(prGetTsfCtxt, VIR_MEM_TYPE,
				sizeof(*prGetTsfCtxt));
		}
	} else if (prGetTsfCtxt->ucTwtStaCnmReason == TWT_STA_CNM_TEARDOWN) {
		ucBssIdx = prGetTsfCtxt->ucBssIdx;
		ucFlowId = prGetTsfCtxt->ucTWTFlowId;

		/* For teardown we don't need this */
		kalMemFree(prGetTsfCtxt, VIR_MEM_TYPE, sizeof(*prGetTsfCtxt));

		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);

		if (prBssInfo == NULL) {
			DBGLOG(TWT_PLANNER, WARN, "prBssInfo is null\n");
			return;
		}

		prStaRec = prBssInfo->prStaRecOfAP;

		if (prStaRec == NULL) {
			DBGLOG(TWT_PLANNER, WARN, "prStaRec is null\n");
			return;
		}

		/* Find and delete the agreement entry in the driver */
		ucFlowId_real = ucFlowId;

		ucAgrtTblIdx = twtPlannerDrvAgrtFind(prAdapter,
			prBssInfo->ucBssIndex, ucFlowId, &ucFlowId_real);

		if (ucAgrtTblIdx >= TWT_AGRT_MAX_NUM) {
			DBGLOG(TWT_PLANNER, ERROR,
				"Cannot find the flow %u to be deleted\n",
				ucFlowId);

			return;
		}

		ucFlowId = ucFlowId_real;

		/* Get TWT type*/
		eTwtType = twtPlannerDrvAgrtGetTwtTypeByIndex(
				prAdapter, ucAgrtTblIdx);

		if (eTwtType >= ENUM_TWT_TYPE_NUM) {
			DBGLOG(TWT_PLANNER, ERROR,
				"TWT[%d] incorrect TWT type %d\n",
				ucFlowId, eTwtType);

			return;
		}

		/*
		 * To setup CNM abort timer in case teardown timeout
		 */
		twtReqFsmTeardownTimeoutInit(
			prAdapter,
			prStaRec,
			ucFlowId,
			&eTwtType);

		/* Do the teardown thing in existing flow */
		twtPlannerSendReqTeardown(prAdapter, prStaRec, ucFlowId);
	}
}
#endif

void nicUniCmdEventInstallKey(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct UNI_CMD_STAREC *uni_cmd =
		(struct UNI_CMD_STAREC *) GET_UNI_CMD_DATA(prCmdInfo);
	struct UNI_CMD_STAREC_INSTALL_KEY3 *tag =
	   (struct UNI_CMD_STAREC_INSTALL_KEY3 *) uni_cmd->aucTlvBuffer;

	nicUniCmdEventSetCommon(prAdapter, prCmdInfo, pucEventBuf);

#if CFG_SUPPORT_REPLAY_DETECTION
	nicCmdEventDetectReplayInfo(prAdapter, tag->ucKeyId,
		tag->ucKeyType, tag->ucBssIdx);
#endif

}

void nicUniEventQueryCnmInfo(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_CNM);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint8_t fail_cnt = 0;
	struct PARAM_GET_CNM_T legacy = {0};
	uint8_t i, j;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(CNM, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_CNM_TAG_GET_BSS_INFO: {
			struct UNI_EVENT_CNM_GET_BSS_INFO *info =
				(struct UNI_EVENT_CNM_GET_BSS_INFO *)tag;
			struct UNI_EVENT_CNM_BSS_INFO *bss =
			      (struct UNI_EVENT_CNM_BSS_INFO *)info->aucBssInfo;

			for (i = 0; i < info->ucBssNum; i++) {
				if (i > MAX_BSSID_NUM)
					break;

				legacy.ucBssInuse[i] = bss->ucBssInuse;
				legacy.ucBssActive[i] = bss->ucBssActive;
				legacy.ucBssConnectState[i] =
							bss->ucBssConnectState;
				legacy.ucBssCh[i] = bss->ucBssPriChannel;
				legacy.ucBssDBDCBand[i] = bss->ucBssDBDCBand;
				legacy.ucBssOMACSet[i] = bss->ucBssOMACIndex;
				legacy.ucBssOMACDBDCBand[i] =
							bss->ucBssOMACDBDCBand;
				legacy.ucBssOpTxNss[i] = bss->ucBssOpTxNss;
				legacy.ucBssOpRxNss[i] = bss->ucBssOpRxNss;
				legacy.ucBssLinkIdx[i] = bss->ucBssLinkIdx;

				bss++;
			}
		}
			break;
		case UNI_EVENT_CNM_TAG_GET_CHANNEL_INFO: {
			struct UNI_EVENT_CNM_GET_CHANNEL_INFO *info =
				(struct UNI_EVENT_CNM_GET_CHANNEL_INFO *)tag;
			struct UNI_EVENT_CNM_CHANNEL_INFO *chnl =
			 (struct UNI_EVENT_CNM_CHANNEL_INFO *)info->aucChnlInfo;
			uint8_t b = info->ucDBDCBand;

			if (b >= ENUM_BAND_NUM)
				break;

			legacy.fgIsDbdcEnable = info->fgIsDBDCEnabled;
			legacy.ucOpChNum[b] = info->ucOpChNum;
			for (i = 0; i < info->ucOpChNum; i++) {
				if (i >= MAX_OP_CHNL_NUM) break;

				legacy.ucChList[b][i] = chnl->ucPriChannel;
				legacy.ucChBw[b][i] = chnl->ucChBw;
				legacy.ucChSco[b][i] = chnl->ucChSco;
				legacy.ucChNetNum[b][i] = chnl->ucChBssNum;

				for (j = 0; j < MAX_BSSID_NUM; j++) {
					if (chnl->u2ChBssBitmapList & BIT(j))
						legacy.ucChBssList[b][i][j] = 1;
				}

				chnl++;
			}
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(CNM, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	nicCmdEventQueryCnmInfo(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

void nicUniEventPhyIcsRawData(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
#if CFG_SUPPORT_PHY_ICS
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_SPECTRUM);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint16_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_SPECTRUM_TAG_PHY_ICS_DATA:{

#if ((CFG_SUPPORT_PHY_ICS_V3 == 1) || (CFG_SUPPORT_PHY_ICS_V4 == 1))
			nicExtEventPhyIcsDumpEmiRawData(ad, tag);
#else
			nicExtEventPhyIcsRawData(ad, tag);
#endif
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#endif
}

void nicUniEventIcerRawData(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
#if CFG_SUPPORT_PTA_ICS_V0
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_ID_COEX_ICER);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint16_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_COEX_ICER_DUMP:{
			nicExtEventIcerDumpEmiRawData(ad, tag);
		}
			break;

		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#endif
}


void nicUniEventRfTestHandler(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_TESTMODE_CTRL);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint8_t fail_cnt = 0;
	struct mt66xx_chip_info *prChipInfo = NULL;
#if CFG_SUPPORT_QA_TOOL
	struct ATE_OPS_T *prAteOps = NULL;
#endif
	struct ICAP_INFO_T *prIcapInfo;
	struct UNI_EVENT_TESTMODE_RESULT *prRfResult;
	struct EXT_EVENT_RBIST_CAP_STATUS_T *prCapStatus;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);
#if CFG_SUPPORT_QA_TOOL
	prAteOps = prChipInfo->prAteOps;
	ASSERT(prAteOps);
#endif
	prIcapInfo = &prAdapter->rIcapInfo;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_TESTMODE_TAG_RESULT: {

			prRfResult = (struct UNI_EVENT_TESTMODE_RESULT *)
			(tag + sizeof(struct UNI_EVENT_TESTMODE_RESULT_TLV));

			if (prRfResult->u4FuncIndex == GET_ICAP_CAPTURE_STATUS) {

				prCapStatus =
					(struct EXT_EVENT_RBIST_CAP_STATUS_T *)
					(tag +
					sizeof(
					struct UNI_EVENT_TESTMODE_RESULT_TLV));

				DBGLOG(RFTEST, DEBUG, "%s:iCapDone=%d\n",
						__func__,
					   prCapStatus->u4CapDone);

				if (prCapStatus->u4CapDone)
					/* Rsp to QAtool ok */
					prAdapter->ucICapDone = 1;
				else
					/* Rsp to QAtool wait */
					prAdapter->ucICapDone = 0;

				DBGLOG(RFTEST, DEBUG, "%s:pAd->ucICapDone=%d\n",
						__func__,
					   prAdapter->ucICapDone);

			} else if (prRfResult->u4FuncIndex == GET_ICAP_RAW_DATA) {

				DBGLOG(RFTEST, DEBUG, "%s:u4FuncIndex=%d\n",
						__func__,
					   prRfResult->u4FuncIndex);
#if CFG_SUPPORT_QA_TOOL
#if (CFG_SUPPORT_ICAP_SOLICITED_EVENT == 1)
				if (prAteOps->getICapDataDumpCmdEvent) {
					prAteOps->getICapDataDumpCmdEvent(
					prAdapter,
					prCmdInfo,
					tag +
					sizeof(
					struct UNI_EVENT_TESTMODE_RESULT_TLV));
				}
#else
			if (prAteOps->getRbistDataDumpEvent) {
				prAteOps->getRbistDataDumpEvent(
				prAdapter,
				tag +
				sizeof(struct UNI_EVENT_TESTMODE_RESULT_TLV));
			}
#endif
#endif

			} else {
				DBGLOG(NIC, WARN, "Unknown funcIdx rf test event = %d\n",
					prRfResult->u4FuncIndex);
			}
		}
			break;

		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	if (prCmdInfo->fgIsOid) {
		kalOidComplete(prGlueInfo,
				prCmdInfo,
				sizeof(struct CMD_TEST_CTRL_EXT_T),
				WLAN_STATUS_SUCCESS);
	}

}

void nicUniEventMibInfo(struct ADAPTER *ad,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint8_t *tag;
	uint16_t tags_len;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_MIB_INFO);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint16_t offset = 0;
	uint8_t i = 0;
	uint16_t u2BandIdx;
	uint32_t u4QueryInfoLen = sizeof(struct PARAM_HW_MIB_INFO);
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MIB_INFO *evt =
		(struct UNI_EVENT_MIB_INFO *) uni_evt->aucBuffer;
	struct PARAM_HW_MIB_INFO *prParamMibInfo;
	struct GLUE_INFO *prGlueInfo = ad->prGlueInfo;
	struct MIB_STATS *prMibStats;
	uint64_t *rMibMapTable[UNI_CMD_MIB_CNT_MAX_NUM];

	prParamMibInfo = (struct PARAM_HW_MIB_INFO *)
			    prCmdInfo->pvInformationBuffer;
	u2BandIdx = (uint16_t)evt->ucBand;
	if (u2BandIdx >= ENUM_BAND_NUM)
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_INVALID_DATA);
	prMibStats = &ad->rMibStats[u2BandIdx];

	/* init mib mapping table */
	rMibMapTable[UNI_CMD_MIB_CNT_RX_FCS_ERR] = &prMibStats->u4RxFcsErrCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_FIFO_OVERFLOW] =
		&prMibStats->u4RxFifoFullCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_MPDU] = &prMibStats->u4RxMpduCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_AMDPU_RX_COUNT] =
		&prMibStats->u4RxAMPDUCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_TOTAL_BYTE] =
		&prMibStats->u4RxTotalByte;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_VALID_AMPDU_SF] =
		&prMibStats->u4RxValidAMPDUSF;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_VALID_BYTE] =
		&prMibStats->u4RxValidByte;
	rMibMapTable[UNI_CMD_MIB_CNT_CHANNEL_IDLE] =
		&prMibStats->u4ChannelIdleCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_VEC_DROP] = &prMibStats->u4RxVectorDropCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_DELIMITER_FAIL] =
		&prMibStats->u4DelimiterFailedCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_VEC_MISMATCH] =
		&prMibStats->u4RxVectorMismatchCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_MDRDY] = &prMibStats->u4MdrdyCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_CCK_MDRDY_TIME] =
		&prMibStats->u4CCKMdrdyCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_OFDM_LG_MIXED_MDRDY_TIME] =
		&prMibStats->u4OFDMLGMixMdrdy;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_OFDM_GREEN_MDRDY_TIME] =
		&prMibStats->u4OFDMGreenMdrdy;
	rMibMapTable[UNI_CMD_MIB_CNT_PF_DROP] = &prMibStats->u4PFDropCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_LEN_MISMATCH] =
		&prMibStats->u4RxLenMismatchCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_P_CCA_TIME] = &prMibStats->u4PCcaTime;
	rMibMapTable[UNI_CMD_MIB_CNT_S_CCA_TIME] = &prMibStats->u4SCcaTime;
	rMibMapTable[UNI_CMD_MIB_CNT_CCA_NAV_TX_TIME] = &prMibStats->u4CcaNavTx;
	rMibMapTable[UNI_CMD_MIB_CNT_P_ED_TIME] = &prMibStats->u4PEDTime;
	rMibMapTable[UNI_CMD_MIB_CNT_BCN_TX] = &prMibStats->u4BeaconTxCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_BW_20MHZ] = &prMibStats->u4Tx20MHzCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_BW_40MHZ] = &prMibStats->u4Tx40MHzCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_BW_80MHZ] = &prMibStats->u4Tx80MHzCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_BW_160MHZ] = &prMibStats->u4Tx160MHzCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_BW_320MHZ] = &prMibStats->u4Tx320MHzCnt;
	rMibMapTable[UNI_CMD_RMAC_CNT_OBSS_AIRTIME] =
		&prMibStats->u8ObssAirTime;
	rMibMapTable[UNI_CMD_RMAC_CNT_NONWIFI_AIRTIME] =
		&prMibStats->u8NonWifiAirTime;
	rMibMapTable[UNI_CMD_MIB_CNT_TX_DUR_CNT] = &prMibStats->u8TxDurCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_DUR_CNT] = &prMibStats->u8RxDurCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_BA_CNT] = &prMibStats->u8BACnt;
	rMibMapTable[UNI_CMD_MIB_CNT_MAC2PHY_TX_TIME] =
		&prMibStats->u4Mac2PHYTxTime;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_OUT_OF_RANGE_COUNT] =
		&prMibStats->u4RxOutOfRangeCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_RX_FCS_OK] = &prMibStats->u4MacRxFcsOkCnt;
	rMibMapTable[UNI_CMD_MIB_CNT_AMPDU] = &prMibStats->u4TxAmpdu;
	rMibMapTable[UNI_CMD_MIB_CNT_AMPDU_MPDU] = &prMibStats->u4TxAmpduMpdu;
	rMibMapTable[UNI_CMD_MIB_CNT_AMPDU_ACKED] = &prMibStats->u4TxAmpduAcked;

	for (i = 0; i < HW_BSSID_NUM; i++) {
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_RTS_TX_CNT + i] =
			&prMibStats->au4RtsTxCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_RTS_RETRY + i] =
			&prMibStats->au4RtsRetryCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_BA_MISS + i] =
			&prMibStats->au4BaMissedCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_ACK_FAIL + i] =
			&prMibStats->au4AckFailedCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY + i] =
			&prMibStats->au4FrameRetryCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY_2 + i] =
			&prMibStats->au4FrameRetry2Cnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_FRAME_RETRY_3 + i] =
			&prMibStats->au4FrameRetry3Cnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_TX + i] =
			&prMibStats->au4TxCnt[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_TX_DATA + i] =
			&prMibStats->au4TxData[i];
		rMibMapTable[UNI_CMD_MIB_CNT_BSS0_TX_BYTE + i] =
			&prMibStats->au4TxByte[i];
		rMibMapTable[UNI_CMD_MIB_CNT_RX_OK_BSS0 + i] =
			&prMibStats->au4RxOk[i];
		rMibMapTable[UNI_CMD_MIB_CNT_RX_BYTE_BSS0 + i] =
			&prMibStats->au4RxByte[i];
		rMibMapTable[UNI_CMD_MIB_CNT_RX_DATA_BSS0 + i] =
			&prMibStats->au4RxData[i];
	}

	for (i = 0; i < 16; i++) {
		rMibMapTable[UNI_CMD_MIB_CNT_MBSS0_TX_OK + i] =
			&prMibStats->au4MbssTxOk[i];
		rMibMapTable[UNI_CMD_MIB_CNT_MBSS0_TX_BYTE + i] =
			&prMibStats->au4MbssTxByte[i];
		rMibMapTable[UNI_CMD_MIB_CNT_RX_OK_MBSS0 + i] =
			&prMibStats->au4MbssRxOk[i];
		rMibMapTable[UNI_CMD_MIB_CNT_RX_BYTE_MBSS0 + i] =
			&prMibStats->au4MbssRxByte[i];
	}

	for (i = 0; i < 5; i++) {
		rMibMapTable[UNI_CMD_MIB_CNT_TX_DDLMT_RNG0 + i] =
			&prMibStats->au4TxDdlmtRng[i];
	}

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		if (i >= MAX_MIB_TAG_CNT)
			break;
		switch (TAG_ID(tag)) {
		case UNI_EVENT_MIB_DATA_TAG: {
			struct UNI_EVENT_MIB_DATA *tlv =
				(struct UNI_EVENT_MIB_DATA *) tag;
			uint32_t u4Counter = tlv->u4Counter;

			if (u4Counter >= UNI_CMD_MIB_CNT_MAX_NUM)
				continue;
			prParamMibInfo->arMibInfo[i].u4Counter = u4Counter;
			prParamMibInfo->arMibInfo[i].u8Data = tlv->u8Data;
			if (rMibMapTable[u4Counter]) {
				*(rMibMapTable[u4Counter]) = tlv->u8Data;
				DBGLOG(REQ, LOUD, "mib idx:%u data:%llu\n",
						u4Counter,
						*rMibMapTable[u4Counter]);
				i += 1;
			}
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
}

void nicUniEventStaStatistics(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_STA_STATISTICS *tag =
		(struct UNI_EVENT_STA_STATISTICS *) evt->aucTlvBuffer;

	nicCmdEventQueryStaStatistics(prAdapter, prCmdInfo, tag->aucBuffer);
}

void nicUniEventStatistics(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_BASIC_STATISTICS *basic =
		(struct UNI_EVENT_BASIC_STATISTICS *)evt->aucTlvBuffer;
	struct EVENT_STATISTICS legacy = {0};

	legacy.rTransmittedFragmentCount.QuadPart =
		basic->u8TransmittedFragmentCount;
	legacy.rMulticastTransmittedFrameCount.QuadPart =
		basic->u8MulticastTransmittedFrameCount;
	legacy.rFailedCount.QuadPart = basic->u8FailedCount;
	legacy.rRetryCount.QuadPart = basic->u8RetryCount;
	legacy.rMultipleRetryCount.QuadPart = basic->u8MultipleRetryCount;
	legacy.rRTSSuccessCount.QuadPart = basic->u8RTSSuccessCount;
	legacy.rRTSFailureCount.QuadPart = basic->u8RTSFailureCount;
	legacy.rACKFailureCount.QuadPart = basic->u8ACKFailureCount;
	legacy.rFrameDuplicateCount.QuadPart = basic->u8FrameDuplicateCount;
	legacy.rReceivedFragmentCount.QuadPart = basic->u8ReceivedFragmentCount;
	legacy.rMulticastReceivedFrameCount.QuadPart =
		basic->u8MulticastReceivedFrameCount;
	legacy.rFCSErrorCount.QuadPart = basic->u8FCSErrorCount;
	legacy.rMdrdyCnt.QuadPart = basic->u8MdrdyCnt;
	legacy.rChnlIdleCnt.QuadPart = basic->u8ChnlIdleCnt;
	legacy.u4HwMacAwakeDuration = basic->u4HwMacAwakeDuration;

	nicCmdEventQueryStatistics(prAdapter, prCmdInfo, (uint8_t*)&legacy);
}

void nicUniEventLinkQuality(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_LINK_QUALITY *tag =
		(struct UNI_EVENT_LINK_QUALITY *) evt->aucTlvBuffer;
	struct EVENT_LINK_QUALITY legacy = {0};
	uint8_t i;

	for (i = 0; i < 4; i++) {
		legacy.rLq[i].cRssi = tag->rLq[i].cRssi;
		legacy.rLq[i].cLinkQuality = tag->rLq[i].cLinkQuality;
		legacy.rLq[i].u2LinkSpeed = tag->rLq[i].u2LinkSpeed;
		legacy.rLq[i].ucMediumBusyPercentage =
				tag->rLq[i].ucMediumBusyPercentage;
		legacy.rLq[i].ucIsLQ0Rdy = tag->rLq[i].ucIsLQ0Rdy;
	}

	nicCmdEventQueryLinkQuality(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
void nicUniEvtBasicStatToLegacy(
	struct UNI_EVENT_BASIC_STATISTICS *prUniEvt,
	struct EVENT_STATISTICS *prLegacy
)
{
	kalMemSet(prLegacy, 0, sizeof(*prLegacy));
	prLegacy->rTransmittedFragmentCount.QuadPart =
		prUniEvt->u8TransmittedFragmentCount;
	prLegacy->rMulticastTransmittedFrameCount.QuadPart =
		prUniEvt->u8MulticastTransmittedFrameCount;
	prLegacy->rFailedCount.QuadPart =
		prUniEvt->u8FailedCount;
	prLegacy->rRetryCount.QuadPart =
		prUniEvt->u8RetryCount;
	prLegacy->rMultipleRetryCount.QuadPart =
		prUniEvt->u8MultipleRetryCount;
	prLegacy->rRTSSuccessCount.QuadPart =
		prUniEvt->u8RTSSuccessCount;
	prLegacy->rRTSFailureCount.QuadPart =
		prUniEvt->u8RTSFailureCount;
	prLegacy->rACKFailureCount.QuadPart =
		prUniEvt->u8ACKFailureCount;
	prLegacy->rFrameDuplicateCount.QuadPart =
		prUniEvt->u8FrameDuplicateCount;
	prLegacy->rReceivedFragmentCount.QuadPart =
		prUniEvt->u8ReceivedFragmentCount;
	prLegacy->rMulticastReceivedFrameCount.QuadPart =
		prUniEvt->u8MulticastReceivedFrameCount;
	prLegacy->rFCSErrorCount.QuadPart = prUniEvt->u8FCSErrorCount;
	prLegacy->rMdrdyCnt.QuadPart = prUniEvt->u8MdrdyCnt;
	prLegacy->rChnlIdleCnt.QuadPart = prUniEvt->u8ChnlIdleCnt;
	prLegacy->u4HwMacAwakeDuration = prUniEvt->u4HwMacAwakeDuration;
}

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
void nicCollectRegStatFromEmi(struct ADAPTER
	*prAdapter
)
{
	struct UNI_EVENT_BASIC_STATISTICS *prBasicUniEvt;
	struct PARAM_802_11_STATISTICS_STRUCT *prStat;
	struct EVENT_STATISTICS legacy = {0};
	struct EMI_LINK_QUALITY *prEmiLQ;
	struct UNI_LINK_QUALITY *prUlq;
	struct EMI_STA_STATISTICS *prEmiStaStats;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	struct EMI_TX_RATE_INFO rLlsRateInfo = {0};
#endif
	struct STA_RECORD *prStaRec = NULL;
	uint8_t i, ucBssIdx;

	/* basic statistics */
	prBasicUniEvt = kalMemAlloc(
		sizeof(struct UNI_EVENT_BASIC_STATISTICS),
		VIR_MEM_TYPE);

	if (!prBasicUniEvt)
		goto exit;
	prStat = &(prAdapter->rStat);
	kalMemCopyFromIo(prBasicUniEvt,
		&prAdapter->prStatsAllRegStat->rBasicStatistics,
		sizeof(*prBasicUniEvt));

	nicUniEvtBasicStatToLegacy(prBasicUniEvt, &legacy);
	nicUpdateStatistics(prAdapter, prStat, &legacy);

	kalMemFree(prBasicUniEvt, VIR_MEM_TYPE,
		sizeof(struct UNI_EVENT_BASIC_STATISTICS));

	/* link quality */
	prEmiLQ = kalMemAlloc(
		sizeof(struct EMI_LINK_QUALITY),
		VIR_MEM_TYPE);
	if (!prEmiLQ)
		goto exit;
	kalMemCopyFromIo(prEmiLQ,
		&prAdapter->prStatsAllRegStat->rLq,
		sizeof(*prEmiLQ));

#define TEMP_LOG_TEMPLATE \
	"rLq[%u] cRssi(0x%px):%d cLinkQuality(0x%px):%d"\
	" u2LinkSpeed(0x%px):%u ucMediumBusyPercentage(0x%px):%u"\
	" ucIsLQ0Rdy(0x%px):%d\n"

	for (i = 0;
		i < MAX_BSSID_NUM && i < ARRAY_SIZE(prEmiLQ->rLq); i++) {
		struct LINK_SPEED_EX_ *prLq;

		if (!prEmiLQ->rLq[i].ucIsLQ0Rdy)
			continue;

		DBGLOG(NIC, TRACE,
			TEMP_LOG_TEMPLATE, i,
			prEmiLQ->rLq[i].cRssi,
				prEmiLQ->rLq[i].cRssi,
			prEmiLQ->rLq[i].cLinkQuality,
				prEmiLQ->rLq[i].cLinkQuality,
			prEmiLQ->rLq[i].u2LinkSpeed,
				prEmiLQ->rLq[i].u2LinkSpeed,
			prEmiLQ->rLq[i].ucMediumBusyPercentage,
				prEmiLQ->rLq[i].ucMediumBusyPercentage,
			prEmiLQ->rLq[i].ucIsLQ0Rdy,
				prEmiLQ->rLq[i].ucIsLQ0Rdy);

#undef TEMP_LOG_TEMPLATE

		prUlq = &prEmiLQ->rLq[i];
		prLq = &prAdapter->rLinkQuality.rLq[i];

		nicUpdateLinkQuality(prAdapter, i, prUlq->cRssi,
				prUlq->cLinkQuality, prUlq->u2LinkSpeed,
				prUlq->ucMediumBusyPercentage,
				prUlq->ucIsLQ0Rdy);
		DBGLOG(NIC, TRACE,
			"ucBssIdx=%d, TxRate=%u, RxRate=%u signal=%d\n",
			i,
			prLq->u2TxLinkSpeed,
			prLq->u2RxLinkSpeed,
			prLq->cRssi);
	}
	kalMemFree(prEmiLQ, VIR_MEM_TYPE,
		sizeof(struct EMI_LINK_QUALITY));

	/* sta Stats */
	prEmiStaStats = kalMemAlloc(
		sizeof(struct EMI_STA_STATISTICS),
		VIR_MEM_TYPE);
	if (!prEmiStaStats)
		goto exit;

	for (i = 0; i < REG_STATS_STA_MAX_NUM; i++) {
		kalMemCopyFromIo(prEmiStaStats,
			&prAdapter->prStatsAllRegStat->rStaStats[i],
			sizeof(*prEmiStaStats));
		if (prEmiStaStats->rStaStats.ucVersion != 1)
			continue;
		prStaRec = cnmGetStaRecByIndex(prAdapter,
					secGetStaIdxByWlanIdx(prAdapter,
					prEmiStaStats->rStaStats.ucStaRecIdx));
		if (!prStaRec)
			continue;
		ucBssIdx = prStaRec->ucBssIndex;
		prQueryStaStatistics =
			&prAdapter->rQueryStaStatistics[ucBssIdx];
		nicUpdateStaStats(prAdapter,
			&prEmiStaStats->rStaStats, prQueryStaStatistics,
			prStaRec->ucIndex, FALSE);
		prQueryStaStatistics->u4TxDataCount =
			prEmiStaStats->u4TxDataCount;
		DBGLOG(REQ, TRACE, "update staStats[%u] wlanIdx(%u)\n",
			i, prEmiStaStats->rStaStats.ucStaRecIdx);
	}
	kalMemFree(prEmiStaStats, VIR_MEM_TYPE,
		sizeof(struct EVENT_STA_STATISTICS));

	/* get bw from emi */
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	if (prAdapter->fgTxRateOffsetMapped) {
		kalMemCopyFromIo(&rLlsRateInfo,
				&prAdapter->prStatsAllRegStat->rLlsRateInfo,
				sizeof(rLlsRateInfo));
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			prAdapter->prGlueInfo->u4TxBwCache[i] =
				rLlsRateInfo.arTxRateInfo[i].bw;
			DBGLOG(NIC, TRACE,
				"ucBssIdx=%d, bw=%u\n", i,
				prAdapter->prGlueInfo->u4TxBwCache[i]);
		}
	}
#endif
exit:
	DBGLOG(REQ, LOUD, "%s done\n", __func__);
}
#endif

void nicUniEventAllStatsOneCmd(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 0)
	/* update to cache directly */
	/* linkQuality: prAdapter->rLinkQuality.rLq[ucBssIndex] */
	/* staStats: prGlueInfo->prAdapter->rQueryStaStatistics[ucBssIndex] */
	/* Stats: prAdapter->rStat */
	uint8_t *tag;
	uint16_t tags_len;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_STATISTICS);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint16_t offset = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(RX, TRACE, "tag=%u, tag->u2Length=%u\n",
					TAG_ID(tag), TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_STATISTICS_TAG_BASIC: {
			struct UNI_EVENT_BASIC_STATISTICS *tlv =
				(struct UNI_EVENT_BASIC_STATISTICS *) tag;

			struct EVENT_STATISTICS legacy = {0};
			struct PARAM_802_11_STATISTICS_STRUCT *prStat;

			prStat = &(prAdapter->rStat);

			nicUniEvtBasicStatToLegacy(tlv, &legacy);

			DBGLOG(RX, TRACE,
				"Fail:%lu Retry:%lu RtsF:%lu AckF:%lu Idle:%lu\n",
					tlv->u8FailedCount, tlv->u8RetryCount,
					tlv->u8RTSFailureCount,
					tlv->u8ACKFailureCount,
					tlv->u8ChnlIdleCnt);
			nicUpdateStatistics(prAdapter, prStat, &legacy);
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_LINK_QUALITY: {
			struct UNI_EVENT_LINK_QUALITY *tlv =
				(struct UNI_EVENT_LINK_QUALITY *) tag;
			struct UNI_LINK_QUALITY *prUlq;
			uint8_t i;

			for (i = 0;
			     i < MAX_BSSID_NUM && i < ARRAY_SIZE(tlv->rLq);
			     i++) {
				struct LINK_SPEED_EX_ *prLq;

				if (!tlv->rLq[i].ucIsLQ0Rdy)
					continue;

				prUlq = &tlv->rLq[i];

				nicUpdateLinkQuality(prAdapter, i, prUlq->cRssi,
					prUlq->cLinkQuality, prUlq->u2LinkSpeed,
					prUlq->ucMediumBusyPercentage,
					prUlq->ucIsLQ0Rdy);

				prLq = &prAdapter->rLinkQuality.rLq[i];
				DBGLOG(NIC, TRACE,
					"ucBssIdx=%d, TxRate=%u, RxRate=%u signal=%d\n",
					i,
					prLq->u2TxLinkSpeed,
					prLq->u2RxLinkSpeed,
					prLq->cRssi);
			}
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_STA: {
			struct UNI_EVENT_STA_STATISTICS *tlv =
				(struct UNI_EVENT_STA_STATISTICS *) tag;
			struct EVENT_STA_STATISTICS *prStaStatsLegacy;
			struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;
			struct STA_RECORD *prStaRec = NULL;
			uint8_t ucBssIdx;

			prStaStatsLegacy =
				(struct EVENT_STA_STATISTICS *) tlv->aucBuffer;
			prStaRec = cnmGetStaRecByIndex(prAdapter,
					secGetStaIdxByWlanIdx(
						prAdapter,
						prStaStatsLegacy->ucStaRecIdx));

			if (!prStaRec)
				continue;

			ucBssIdx = prStaRec->ucBssIndex;
			prQueryStaStatistics =
				&prAdapter->rQueryStaStatistics[ucBssIdx];
			nicUpdateStaStats(prAdapter,
				prStaStatsLegacy, prQueryStaStatistics,
				prStaRec->ucIndex, TRUE);
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_BSS_LINK_QUALITY: {
			struct UNI_EVENT_BSS_LINK_QUALITY *tlv =
				(struct UNI_EVENT_BSS_LINK_QUALITY *) tag;
			struct LINK_SPEED_EX_ *prLq;
			struct UNI_LINK_QUALITY *prUlq;
			uint8_t ucBssIdx;

			ucBssIdx = tlv->ucBssIdx;

			if (!tlv->rLq.ucIsLQ0Rdy || ucBssIdx >= MAX_BSSID_NUM)
				continue;

			prUlq = &tlv->rLq;
			prLq = &prAdapter->rLinkQuality.rLq[ucBssIdx];

			nicUpdateLinkQuality(prAdapter, ucBssIdx, prUlq->cRssi,
				prUlq->cLinkQuality, prUlq->u2LinkSpeed,
				prUlq->ucMediumBusyPercentage,
				prUlq->ucIsLQ0Rdy);

			DBGLOG(NIC, TRACE,
				"ucBssIdx=%d, TxRate=%u, RxRate=%u signal=%d\n",
				ucBssIdx,
				prLq->u2TxLinkSpeed,
				prLq->u2RxLinkSpeed,
				prLq->cRssi);
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_LINK_LAYER_STATS: {
			struct UNI_EVENT_LINK_STATS *tlv =
				(struct UNI_EVENT_LINK_STATS *) tag;
			uint32_t resultSize;

			resultSize = tlv->u2Length - sizeof(*tlv);
			if (resultSize != sizeof(struct EVENT_STATS_LLS_DATA)) {
				DBGLOG(RX, WARN,
					"tag=%u resultSize=%u length mismatch.",
					tlv->u2Tag, resultSize);
			}
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_CURRENT_TX_RATE: {
			struct UNI_EVENT_CURRENT_TX_RATE *tlv =
				(struct UNI_EVENT_CURRENT_TX_RATE *) tag;
			struct EVENT_STATS_LLS_TX_RATE_INFO *prTxRate;
			uint8_t i = 0;

			prTxRate = (struct EVENT_STATS_LLS_TX_RATE_INFO *)
				   &tlv->rate_info;
			for (i = 0; i < BSSID_NUM; i++) {
				prAdapter->prGlueInfo->u4TxBwCache[i] =
					prTxRate->arTxRateInfo[i].bw;
			}
			break;
		}
		case UNI_EVENT_STATISTICS_TAG_BSS_CURRENT_TX_RATE: {
			struct UNI_EVENT_BSS_TX_RATE *tlv =
				(struct UNI_EVENT_BSS_TX_RATE *) tag;
			uint8_t ucBssIdx;

			ucBssIdx = tlv->ucBssIdx;
			if (ucBssIdx >= MAX_BSSID_NUM)
				continue;

			prAdapter->prGlueInfo->u4TxBwCache[ucBssIdx] =
				tlv->rTxRateInfo.bw;

			break;
		}
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}
	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#else
	uint32_t u4EmiUpdateMs = 0;

	nicCollectRegStatFromEmi(prAdapter);

	/* latest fw/driver sync fw clock */
	kalMemCopyFromIo(&u4EmiUpdateMs,
			&prAdapter->prStatsAllRegStat->u4LastUpdateTime,
			sizeof(uint32_t));

	prAdapter->u8RegStatSyncFwUs = MSEC_TO_USEC(u4EmiUpdateMs);
	prAdapter->u8RegStatSyncDrvUs = kalGetBootTime();
	DBGLOG(REQ, TRACE, "sync time drv:%lld.%06lld fw:%u(%lld.%06lld)\n",
		USEC_TO_SEC(prAdapter->u8RegStatSyncDrvUs),
		USEC_REM_TO_SEC(prAdapter->u8RegStatSyncDrvUs),
		u4EmiUpdateMs,
		USEC_TO_SEC(prAdapter->u8RegStatSyncFwUs),
		USEC_REM_TO_SEC(prAdapter->u8RegStatSyncFwUs));
#endif

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			0, WLAN_STATUS_SUCCESS);

}
#endif

void nicUniEventQueryRfTestATInfo(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_TESTMODE_CTRL *evt =
		(struct UNI_EVENT_TESTMODE_CTRL *)uni_evt->aucBuffer;
	struct UNI_EVENT_TESTMODE_RESULT_TLV *tag =
		(struct UNI_EVENT_TESTMODE_RESULT_TLV *)evt->aucTlvBuffer;

	nicCmdEventQueryRfTestATInfo(prAdapter, prCmdInfo, tag->aucBuffer);
}

#if CFG_SUPPORT_XONVRAM
void nicUniEventRfTestXoCal(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_TESTMODE_CTRL *evt;
	struct UNI_EVENT_TESTMODE_XO_CAL *tag;
	struct TEST_MODE_XO_CAL *response;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_TESTMODE_CTRL)
		return;

	evt = (struct UNI_EVENT_TESTMODE_CTRL *)uni_evt->aucBuffer;

	tag = (struct UNI_EVENT_TESTMODE_XO_CAL *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_EVENT_TESTMODE_TAG_XO_CAL)
		return;

	if (cmd->pvInformationBuffer) {
		response = cmd->pvInformationBuffer;
		response->u4AxmFreq = tag->rXoCal.u4AxmFreq;
		response->u4AxmC1Freq = tag->rXoCal.u4AxmC1Freq;
		response->u4AxmC2Freq = tag->rXoCal.u4AxmC2Freq;
		response->u4AxmC1Comp = tag->rXoCal.u4AxmC1Comp;
		response->u4AxmC2Comp = tag->rXoCal.u4AxmC2Comp;
		response->u4BtmFreq = tag->rXoCal.u4BtmFreq;
		response->u4BtmC1Freq = tag->rXoCal.u4BtmC1Freq;
		response->u4BtmC2Freq = tag->rXoCal.u4BtmC2Freq;
		response->u4BtmC1Comp = tag->rXoCal.u4BtmC1Comp;
		response->u4BtmC2Comp = tag->rXoCal.u4BtmC2Comp;
	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
void nicUniEventRfTestPlCal(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_TESTMODE_CTRL *evt;
	struct UNI_EVENT_TESTMODE_PL_CAL *tag;
	struct TEST_MODE_PL_CAL *response;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_TESTMODE_CTRL)
		return;

	evt = (struct UNI_EVENT_TESTMODE_CTRL *)uni_evt->aucBuffer;

	tag = (struct UNI_EVENT_TESTMODE_PL_CAL *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_EVENT_TESTMODE_TAG_PL_CAL)
		return;

	if (cmd->pvInformationBuffer) {
		response = cmd->pvInformationBuffer;
		response->u4OutCnt = tag->rPlCal.u4OutCnt;
		memcpy(response->u4OutData, tag->rPlCal.u4OutData,
			sizeof(tag->rPlCal.u4OutData));
	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}
#endif /* CFG_SUPPORT_PLCAL */

#if CFG_SUPPORT_QA_TOOL
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
void nicUniEventQueryRxStatAll(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_TESTMODE_RX_STAT *rx_evt =
		(struct UNI_EVENT_TESTMODE_RX_STAT *)uni_evt->aucBuffer;
	struct UNI_EVENT_TESTMODE_STAT_ALL *tag =
		(struct UNI_EVENT_TESTMODE_STAT_ALL *)rx_evt->aucTlvBuffer;

	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t *prElement = (uint32_t *)&g_HqaRxStat;
	uint32_t i, u4Temp;

	for (i = 0; i < UNI_EVENT_TESTMODE_RX_STAT_ALL_ITEM; i++) {
		u4Temp = NTOHL(tag->u4Buffer[i]);
		kalMemCopy(prElement, &u4Temp, 4);

		if (i < (UNI_EVENT_TESTMODE_RX_STAT_ALL_ITEM - 1))
			prElement++;
	}

	if (prCmdInfo->fgIsOid) {
		kalOidComplete(prGlueInfo,
						prCmdInfo,
						sizeof(struct CMD_ACCESS_RX_STAT),
						WLAN_STATUS_SUCCESS);
	}
}
#else
void nicUniEventRxStatCastMap(struct UNI_EVENT_TESTMODE_STAT_ALL_V2 *tag,
	struct PARAM_RX_STAT *pHqaRxStat)
{
	uint8_t u1Idx = 0, u1BandIdx = 0;

	u1BandIdx = tag->rInfoBandExt1.u1BandIdx;

	/* Band part */
	pHqaRxStat->rInfoBand[u1BandIdx].u4MacRxFcsErrCnt =
		(uint32_t)(tag->rInfoBand.u2MacRxFcsErrCnt);
	pHqaRxStat->rInfoBand[u1BandIdx].u4MacRxLenMisMatch =
		(uint32_t)(tag->rInfoBand.u2MacRxLenMisMatch);
	pHqaRxStat->rInfoBand[u1BandIdx].u4MacRxFcsOkCnt =
		(uint32_t)(tag->rInfoBand.u2MacRxFcsOkCnt);
	pHqaRxStat->rInfoBand[u1BandIdx].u4MacRxMdrdyCnt =
		(uint32_t)(tag->rInfoBand.u4MacRxMdrdyCnt);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxFcsErrCntCck =
		(uint32_t)(tag->rInfoBand.u2PhyRxFcsErrCntCck);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxFcsErrCntOfdm =
		(uint32_t)(tag->rInfoBand.u2PhyRxFcsErrCntOfdm);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxPdCck =
		(uint32_t)(tag->rInfoBand.u2PhyRxPdCck);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxPdOfdm =
		(uint32_t)(tag->rInfoBand.u2PhyRxPdOfdm);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxSigErrCck =
		(uint32_t)(tag->rInfoBand.u2PhyRxSigErrCck);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxSfdErrCck =
		(uint32_t)(tag->rInfoBand.u2PhyRxSfdErrCck);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxSigErrOfdm =
		(uint32_t)(tag->rInfoBand.u2PhyRxSigErrOfdm);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxTagErrOfdm =
		(uint32_t)(tag->rInfoBand.u2PhyRxTagErrOfdm);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxMdrdyCntCck =
		(uint32_t)(tag->rInfoBand.u2PhyRxMdrdyCntCck);
	pHqaRxStat->rInfoBand[u1BandIdx].u4PhyRxMdrdyCntOfdm =
		(uint32_t)(tag->rInfoBand.u2PhyRxMdrdyCntOfdm);
	pHqaRxStat->rInfoBandExt1[u1BandIdx].u4PhyRxPdAlr =
		(uint32_t)(tag->rInfoBandExt1.u2PhyRxPdAlr);
	pHqaRxStat->rInfoBandExt1[u1BandIdx].u4RxU2MMpduCnt =
		(uint32_t)(tag->rInfoBandExt1.u4RxU2MMpduCnt);

	/* Path part */
	for (u1Idx = 0; u1Idx < UNI_TM_MAX_ANT_NUM; u1Idx++) {
		pHqaRxStat->rInfoRXV[u1Idx].u4Rcpi =
			(uint32_t)(tag->rInfoRXV[u1Idx].u2Rcpi);
		pHqaRxStat->rInfoRXV[u1Idx].u4Rssi =
			(uint32_t)(tag->rInfoRXV[u1Idx].i2Rssi);
		pHqaRxStat->rInfoRXV[u1Idx].u4AdcRssi =
			(uint32_t)(tag->rInfoRXV[u1Idx].i2AdcRssi);
		pHqaRxStat->rInfoFagc[u1Idx].u4RssiIb =
			(uint32_t)(tag->rInfoFagc[u1Idx].i1RssiIb);
		pHqaRxStat->rInfoFagc[u1Idx].u4RssiWb =
			(uint32_t)(tag->rInfoFagc[u1Idx].i1RssiWb);
		pHqaRxStat->rInfoInst[u1Idx].u4RssiIb =
			(uint32_t)(tag->rInfoInst[u1Idx].i1RssiIb);
		pHqaRxStat->rInfoInst[u1Idx].u4RssiWb =
			(uint32_t)(tag->rInfoInst[u1Idx].i1RssiWb);
		pHqaRxStat->rInfoInst[u1Idx].u4CcaIdlePwr =
			(uint32_t)(tag->rInfoInst[u1Idx].u2CcaIdlePwr);
	}

	/* User part */
	for (u1Idx = 0; u1Idx < UNI_TM_MAX_USER_NUM; u1Idx++) {
		pHqaRxStat->rInfoUser[u1Idx].u4FreqOffsetFromRx =
			(uint32_t)(tag->rInfoUser[u1Idx].i4FreqOffsetFromRx);
		pHqaRxStat->rInfoUser[u1Idx].u4Snr =
			(uint32_t)(tag->rInfoUser[u1Idx].i4Snr);
		pHqaRxStat->rInfoUser[u1Idx].u4FcsErrorCnt =
			tag->rInfoUser[u1Idx].u4FcsErrorCnt;
		pHqaRxStat->rInfoUserExt1[u1Idx].u4NeVarDbAllUser =
			(uint32_t)(tag->rInfoUserExt1[u1Idx].i1NeVarDbAllUser);
	}

	/* Common part */
	pHqaRxStat->rInfoComm[u1BandIdx].u4AciHitLow =
		tag->rInfoComm.u4AciHitLow;
	pHqaRxStat->rInfoComm[u1BandIdx].u4AciHitHigh =
		tag->rInfoComm.u4AciHitHigh;
	pHqaRxStat->rInfoComm[u1BandIdx].u4MacRxFifoFull =
		(uint32_t)(tag->rInfoComm.u2MacRxFifoFull);
	pHqaRxStat->rInfoCommExt1[u1BandIdx].u4DrvRxCnt =
		tag->rInfoCommExt1.u4DrvRxCnt;
	pHqaRxStat->rInfoCommExt1[u1BandIdx].u4MuRxCnt =
		tag->rInfoCommExt1.u4MuRxCnt;
	pHqaRxStat->rInfoCommExt1[u1BandIdx].u4Sinr =
		tag->rInfoCommExt1.u4Sinr;
	pHqaRxStat->rInfoCommExt1[u1BandIdx].u4EhtSigMcs =
		tag->rInfoCommExt1.u1EhtSigMcs;
}

void nicUniEventQueryRxStatAllCon3(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_TESTMODE_RX_STAT *rx_evt =
		(struct UNI_EVENT_TESTMODE_RX_STAT *)uni_evt->aucBuffer;
	struct UNI_EVENT_TESTMODE_STAT_ALL_V2 *tag =
		(struct UNI_EVENT_TESTMODE_STAT_ALL_V2 *)rx_evt->aucTlvBuffer;

	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	uint32_t *prElement = (uint32_t *)&g_HqaRxStat;
	uint32_t i, u4Temp;
	uint8_t u1BandIdx = 0;

	u1BandIdx = tag->rInfoBandExt1.u1BandIdx;

	nicUniEventRxStatCastMap(tag, &g_HqaRxStat);

	for (i = 0; i < (sizeof(struct PARAM_RX_STAT)/sizeof(uint32_t)); i++) {
		u4Temp = ntohl(*(prElement));
		kalMemCopy(prElement, &u4Temp, 4);
		if (i < ((sizeof(struct PARAM_RX_STAT)/sizeof(uint32_t))-1))
			prElement++;
	}

	if (prCmdInfo->fgIsOid) {
		kalOidComplete(prGlueInfo,
			prCmdInfo,
			sizeof(struct CMD_ACCESS_RX_STAT),
			WLAN_STATUS_SUCCESS);
	}
}

#endif

void nicUniEventQueryRxStatInfoTlv(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct GLUE_INFO *pr_glue_info = prAdapter->prGlueInfo;
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint16_t fixed_len = sizeof(struct UNI_EVENT_TESTMODE_RX_STAT);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	struct RFTEST_RX_STAT_INFO_TLV *pst_rx_info = NULL;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(RFTEST, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	if (prCmdInfo->pvInformationBuffer) {
		pst_rx_info =
			(struct RFTEST_RX_STAT_INFO_TLV *)
			prCmdInfo->pvInformationBuffer;
	} else {
		DBGLOG(RFTEST, ERROR, "pvInformationBuffer is NULL.\n");
		return;
	}

	if (data_len > sizeof(pst_rx_info->au4Data)) {
		DBGLOG(RFTEST, ERROR, "data_len(%d) invalid\n", data_len);
		return;
	}

	memcpy((uint8_t *)(pst_rx_info->au4Data), data, data_len);
	pst_rx_info->u4EvtLen = data_len;

	if (prCmdInfo->fgIsOid) {
		kalOidComplete(pr_glue_info,
			prCmdInfo,
			data_len,
			WLAN_STATUS_SUCCESS);
	}
}

#endif
void nicUniEventBugReport(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_BUG_REPORT *tag =
		(struct UNI_EVENT_BUG_REPORT *)evt->aucTlvBuffer;

	nicCmdEventQueryBugReport(prAdapter, prCmdInfo,
		(uint8_t *)&tag->u4BugReportVersion);
}

void nicUniEventLinkStats(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
#if CFG_SUPPORT_LLS
	uint16_t fixed_len = sizeof(struct UNI_EVENT_STATISTICS);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint16_t resultSize = data_len - fixed_len;

	DBGLOG(RX, TRACE, "resultSize=%u BufLen=%u",
			resultSize,
			prCmdInfo->u4InformationBufferLength);

	if (prCmdInfo->u4InformationBufferLength < resultSize) {
		DBGLOG(RX, WARN, "Overflow resultSize=%u, BufLen=%u",
			 resultSize,
			prCmdInfo->u4InformationBufferLength);
		if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo, prCmdInfo, 0,
				WLAN_STATUS_FAILURE);
		return;
	}

	if (resultSize < prCmdInfo->u4InformationBufferLength)
		prCmdInfo->u4InformationBufferLength = resultSize;

	kalMemZero(prCmdInfo->pvInformationBuffer,
		prCmdInfo->u4InformationBufferLength);

	memcpy((uint8_t *)prCmdInfo->pvInformationBuffer,
		data + fixed_len, resultSize);

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo, resultSize,
			WLAN_STATUS_SUCCESS);
#endif
}

void nicUniCmdEventTxPowerInfo(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_TXPOWER_INFO == 1)
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_TXPOWER *evt =
		(struct UNI_EVENT_TXPOWER *)uni_evt->aucBuffer;
	struct UNI_EVENT_TXPOWER_RSP *tag =
		(struct UNI_EVENT_TXPOWER_RSP *)evt->aucTlvBuffer;

	nicCmdEventQueryTxPowerInfo(prAdapter, prCmdInfo, tag->aucBuffer);
#endif
}

void nicUniEventEfuseControl(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	//TODO: Support Unify command
}

void nicUniEventFwLogQueryBase(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_WSYS_CONFIG *evt;
	struct UNI_EVENT_FW_LOG_BUFFER_CTRL *tag;
	uint32_t *addr;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_WSYS_CONFIG)
		return;

	evt = (struct UNI_EVENT_WSYS_CONFIG *)uni_evt->aucBuffer;
	tag = (struct UNI_EVENT_FW_LOG_BUFFER_CTRL *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_EVENT_FW_LOG_BUFFER_CTRL)
		return;

	if (tag->ucType != BIT(FW_LOG_CTRL_CMD_GET_BASE_ADDR))
		return;

	addr = (uint32_t *)cmd->pvInformationBuffer;

	if (addr)
		*addr = tag->u4Address;
}

void nicUniEventThermalAdieTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_THERMAL *evt;
	struct UNI_EVENT_THERMAL_RSP *tag;
	struct UNI_EVENT_THERMAL_SENSOR_INFO *info;
	struct THERMAL_TEMP_DATA *data;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_THERMAL)
		return;

	evt = (struct UNI_EVENT_THERMAL *)uni_evt->aucBuffer;

	tag = (struct UNI_EVENT_THERMAL_RSP *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_THERMAL_EVENT_TEMPERATURE_INFO)
		return;

	info = (struct UNI_EVENT_THERMAL_SENSOR_INFO *)tag->aucBuffer;
	if (cmd->pvInformationBuffer) {
		data = cmd->pvInformationBuffer;

		data->u4Temperature = info->u4SensorResult;
		data->u4Temperature *= 1000;
	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}

void nicUniEventThermalDdieTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_THERMAL *evt;
	struct UNI_EVENT_THERMAL_RSP *tag;
	struct UNI_EVENT_THERMAL_DDIE_SENSOR_INFO *info;
	struct THERMAL_TEMP_DATA *data;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_THERMAL)
		return;

	evt = (struct UNI_EVENT_THERMAL *)uni_evt->aucBuffer;

	tag = (struct UNI_EVENT_THERMAL_RSP *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_THERMAL_EVENT_DDIE_SENSOR_INFO)
		return;

	info = (struct UNI_EVENT_THERMAL_DDIE_SENSOR_INFO *)tag->aucBuffer;
	if (cmd->pvInformationBuffer) {
		data = cmd->pvInformationBuffer;

		data->u4Temperature = info->u4SensorResult;
	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}

void nicUniEventThermalAdcTemp(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_THERMAL *evt;
	struct UNI_EVENT_THERMAL_RSP *tag;
	struct UNI_EVENT_THERMAL_TEMP_ADC_INFO *info;
	struct UNI_EVENT_THERMAL_TEMP_ADC_INFO *calbk;
	struct THERMAL_TEMP_DATA_V2 *data;
	uint8_t itemNum = 0, *pNum = NULL, index = 0;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_THERMAL)
		return;

	evt = (struct UNI_EVENT_THERMAL *)uni_evt->aucBuffer;

	tag = (struct UNI_EVENT_THERMAL_RSP *)evt->aucTlvBuffer;
	if (tag->u2Tag != UNI_THERMAL_EVENT_SENSOR_ADC_TEMP_INFO)
		return;

	info = (struct UNI_EVENT_THERMAL_TEMP_ADC_INFO *)tag->aucBuffer;

	itemNum = (tag->u2Length - sizeof(struct UNI_EVENT_THERMAL_RSP)) /
				sizeof(struct UNI_EVENT_THERMAL_TEMP_ADC_INFO);

	if (cmd->pvInformationBuffer) {
		data = cmd->pvInformationBuffer;
		pNum = data->pu1SensorResult;
		*pNum = itemNum;
		pNum++;
		calbk = (struct UNI_EVENT_THERMAL_TEMP_ADC_INFO *)pNum;

		for (index = 0; index < itemNum; index++, calbk++, info++) {
			calbk->u4Adc = info->u4Adc;
			calbk->u4Temp = info->u4Temp;
		}

	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
void nicUniEventEmlInfo(struct ADAPTER *ad,
	struct CMD_INFO *cmd, uint8_t *event)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)event;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_EML_INFO *tag =
		(struct UNI_EVENT_EML_INFO *)evt->aucTlvBuffer;
	struct PARAM_EML_DEBUG_INFO *data;
	uint8_t ucLinkidx = 0;

	uni_evt = (struct WIFI_UNI_EVENT *) event;
	if (uni_evt->ucEID != UNI_EVENT_ID_STATISTICS)
		return;

	if (tag->u2Tag != UNI_EVENT_STATISTICS_TAG_EML_STATS)
		return;

	if (cmd->pvInformationBuffer) {
		data = cmd->pvInformationBuffer;

		data->u4TimeoutMs = tag->u4TimeoutMs;
		data->u2StaRecMldIdx = tag->u2StaRecMldIdx;
		data->ucEmlsrBitMap = tag->ucEmlsrBitMap;
		data->ucCurrentState = tag->ucCurrentState;
		data->ucEmlNegotiated = tag->ucEmlNegotiated;

		for (ucLinkidx = 0; ucLinkidx < 3; ucLinkidx++)
			data->auMldLinkIdx[ucLinkidx] =
				tag->auMldLinkIdx[ucLinkidx];
	}

	kalOidComplete(ad->prGlueInfo, cmd, cmd->u4InformationBufferLength,
		WLAN_STATUS_SUCCESS);
}

#if (CFG_MLO_CONCURRENT_SINGLE_PHY == 1)
void nicUniEventMLSRSwitchDone(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt)
{
	uint8_t *tag;
	uint16_t tags_len;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_MLO);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint16_t offset = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_MLD_MLSR_CONCURRENT_DONE: {
			/*MLSR Switch done*/
			DBGLOG(NIC, WARN, "MLSR Switch Done\n");
			ad->ucNeedWaitFWMlsrSWDone = FALSE;
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}
}
#endif

#if (CFG_SUPPORT_MLC == 1)
void nicUniEventMlcReqDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MLC *evt =
		(struct UNI_EVENT_MLC *)uni_evt->aucBuffer;
	struct UNI_EVENT_MLC_RESP *tag =
		(struct UNI_EVENT_MLC_RESP *) evt->aucTlvBuffer;
	struct MLD_BSS_INFO *mld_bssinfo;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	if (uni_evt->ucEID != UNI_EVENT_ID_MLC ||
	    tag->u2Tag != UNI_EVENT_MLC_TAG_RESP) {
		DBGLOG(ML, WARN,
			"Ignore Event[%d] CID[0x%x] OID[%d] TagId[%d]\n",
			uni_evt->ucEID, prCmdInfo->ucCID,
			prCmdInfo->fgIsOid, tag->u2Tag);
		u4Status = WLAN_STATUS_FAILURE;
		goto done;
	}

	DBGLOG(ML, INFO, "GrpMldId=%d TokenId=%d status=0x%x\n",
		evt->ucGroupMldId, evt->ucTokenID, tag->u4Status);

	mld_bssinfo = mldBssGetByIdx(prAdapter, evt->ucGroupMldId);
	if (!mld_bssinfo) {
		DBGLOG(ML, WARN, "mld bssinfo %d not exist\n",
			evt->ucGroupMldId);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}
	if (mld_bssinfo->ucMlcToken != evt->ucTokenID) {
		DBGLOG(ML, WARN, "Token %d doesn't match expected %d\n",
			evt->ucTokenID, mld_bssinfo->ucMlcToken);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}

done:
	if (prCmdInfo->fgIsOid)
		/* Update Query Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			0, u4Status == WLAN_STATUS_SUCCESS ?
			tag->u4Status : u4Status);
}

void nicUniEventMlcQueryLinkStateDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MLC *evt =
		(struct UNI_EVENT_MLC *)uni_evt->aucBuffer;
	struct UNI_EVENT_MLC_QUERY_LINK_STATE *tag =
		(struct UNI_EVENT_MLC_QUERY_LINK_STATE *) evt->aucTlvBuffer;
	struct UNI_CMD_MLC_LINK_INFO *link;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct PARAM_MLC_QUERY_LINK_STATE *prQueryBuffer;
	uint32_t u4QueryInfoLen = 0, u4Status = WLAN_STATUS_SUCCESS;
	uint8_t i;

	if (uni_evt->ucEID != UNI_EVENT_ID_MLC ||
	    tag->u2Tag != UNI_EVENT_MLC_TAG_QUERY) {
		DBGLOG(ML, WARN,
			"Ignore Event[%d] CID[0x%x] OID[%d] TagId[%d]\n",
			uni_evt->ucEID, prCmdInfo->ucCID,
			prCmdInfo->fgIsOid, tag->u2Tag);
		u4Status = WLAN_STATUS_FAILURE;
		goto done;
	}

	DBGLOG(ML, INFO, "GrpMldId=%d TokenId=%d MlcMode=%d LinkNum=%d\n",
		evt->ucGroupMldId, evt->ucTokenID,
		tag->ucMlcMode, tag->ucLinkNum);

	mld_bssinfo = mldBssGetByIdx(prAdapter, evt->ucGroupMldId);
	if (!mld_bssinfo) {
		DBGLOG(ML, WARN, "mld bssinfo %d not exist\n",
			evt->ucGroupMldId);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}
	if (mld_bssinfo->ucMlcToken != evt->ucTokenID) {
		DBGLOG(ML, WARN, "Token %d doesn't match expected %d\n",
			evt->ucTokenID, mld_bssinfo->ucMlcToken);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}

	prQueryBuffer = (struct PARAM_MLC_QUERY_LINK_STATE *)
			prCmdInfo->pvInformationBuffer;
	prQueryBuffer->eMlcMode = tag->ucMlcMode;
	prQueryBuffer->ucLinkNum = tag->ucLinkNum;
	link = (struct UNI_CMD_MLC_LINK_INFO *)tag->aucLinkInfo;
	for (i = 0; i < tag->ucLinkNum && i < MLD_LINK_MAX; i++, link++) {
		struct PARAM_MLC_LINK_INFO *prInfo =
			&prQueryBuffer->arLinkInfo[i];
		struct BSS_INFO *prBssInfo;
		struct STA_RECORD *prStaRec;

		prStaRec = cnmGetStaRecByWlanIndex(prAdapter, link->u2WlanIdx);
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, link->ucBssIdx);

		if (!prStaRec || !prBssInfo)
			continue;

		prInfo->ucLinkId = prStaRec->ucLinkId;
		prInfo->ucLinkState = link->ucLinkState;
		prInfo->u4FreqInMHz = nicChannelNum2Freq(
			prBssInfo->ucPrimaryChannel, prBssInfo->eBand) / 1000;

		DBGLOG(ML, INFO, "\tbss=%d,wlan_idx=%d,state=%d,freq=%d\n",
			link->ucBssIdx,
			link->u2WlanIdx,
			link->ucLinkState,
			prInfo->u4FreqInMHz);
	}

	u4QueryInfoLen = sizeof(struct PARAM_MLC_QUERY_LINK_STATE);

done:
	if (prCmdInfo->fgIsOid)
		/* Update Query Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, u4Status);
}

void nicUniEventMlcQueryInfoDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_MLC *evt =
		(struct UNI_EVENT_MLC *)uni_evt->aucBuffer;
	struct UNI_EVENT_MLC_QUERY_INFO *tag =
		(struct UNI_EVENT_MLC_QUERY_INFO *) evt->aucTlvBuffer;
	struct MLD_BSS_INFO *mld_bssinfo;
	struct PARAM_MLC_QUERY_INFO *prQueryBuffer;
	uint32_t u4QueryInfoLen = 0, u4Status = WLAN_STATUS_SUCCESS;

	if (uni_evt->ucEID != UNI_EVENT_ID_MLC ||
	    tag->u2Tag != UNI_EVENT_MLC_TAG_QUERY) {
		DBGLOG(ML, WARN,
			"Ignore Event[%d] CID[0x%x] OID[%d] TagId[%d]\n",
			uni_evt->ucEID, prCmdInfo->ucCID,
			prCmdInfo->fgIsOid, tag->u2Tag);
		u4Status = WLAN_STATUS_FAILURE;
		goto done;
	}

	DBGLOG(ML, INFO,
		"GrpMldId=%d TokenId=%d AvailablePlanBmap=0x%0x CurPlan=%d CurUser=%d\n",
		evt->ucGroupMldId, evt->ucTokenID,
		tag->u4AvailablePlanBmap, tag->ucCurrentPlan,
		tag->ucCurrentUser);

	mld_bssinfo = mldBssGetByIdx(prAdapter, evt->ucGroupMldId);
	if (!mld_bssinfo) {
		DBGLOG(ML, WARN, "mld bssinfo %d not exist\n",
			evt->ucGroupMldId);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}
	if (mld_bssinfo->ucMlcToken != evt->ucTokenID) {
		DBGLOG(ML, WARN, "Token %d doesn't match expected %d\n",
			evt->ucTokenID, mld_bssinfo->ucMlcToken);
		u4Status = WLAN_STATUS_NOT_ACCEPTED;
		goto done;
	}

	prQueryBuffer = (struct PARAM_MLC_QUERY_INFO *)
			prCmdInfo->pvInformationBuffer;

	prQueryBuffer->u4AvailablePlanBmap = tag->u4AvailablePlanBmap;
	prQueryBuffer->ucCurrentPlan = tag->ucCurrentPlan;
	prQueryBuffer->ucCurrentUser = tag->ucCurrentUser;

done:
	if (prCmdInfo->fgIsOid)
		/* Update Query Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, u4Status);
}
#endif /* CFG_SUPPORT_MLC */

#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
void nicUniCmdEventGetMlChnlCond(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_STATISTICS *evt =
		(struct UNI_EVENT_STATISTICS *)uni_evt->aucBuffer;
	struct UNI_EVENT_GET_ML_CHNL_COND *tag =
		(struct UNI_EVENT_GET_ML_CHNL_COND *) evt->aucTlvBuffer;
	struct PARAM_QUERY_ML_CHNL_COND *prParam;

	if (uni_evt->ucEID != UNI_EVENT_ID_STATISTICS ||
	    tag->u2Tag != UNI_EVENT_STATISTICS_TAG_GET_ML_CHNL_COND) {
		DBGLOG(ML, WARN,
			"Ignore Event[%d] CID[0x%x] OID[%d] TagId[%d]\n",
			uni_evt->ucEID, prCmdInfo->ucCID,
			prCmdInfo->fgIsOid, tag->u2Tag);
		return;
	}

	prParam = prCmdInfo->pvInformationBuffer;
	prParam->u4Status = tag->u4Status;

	if (prCmdInfo->fgIsOid)
		/* Update Query Information Length */
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       0, WLAN_STATUS_SUCCESS);
}
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */
#endif /* CFG_SUPPORT_802_11BE_MLO */

/*******************************************************************************
 *                   Unsolicited Event
 *******************************************************************************
 */

void nicUniEventScanDone(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_SCAN_DONE);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	uint8_t fgIsValidScanDone = TRUE;
	int i;
	struct UNI_EVENT_SCAN_DONE *scan_done;
	struct EVENT_SCAN_DONE legacy = {0};

	scan_done = (struct UNI_EVENT_SCAN_DONE *) data;
	legacy.ucSeqNum = scan_done->ucSeqNum;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(SCN, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_SCAN_DONE_TAG_BASIC: {
			struct UNI_EVENT_SCAN_DONE_BASIC *basic =
				(struct UNI_EVENT_SCAN_DONE_BASIC *) tag;
			/* Event Type TAG_BASIC should have 12 bytes contents.*/
			if (basic->u2Length !=
				sizeof(struct UNI_EVENT_SCAN_DONE_BASIC)) {
				fgIsValidScanDone = FALSE;
				break;
			}
			legacy.ucCompleteChanCount = basic->ucCompleteChanCount;
			legacy.ucCurrentState = basic->ucCurrentState;
			legacy.ucScanDoneVersion = basic->ucScanDoneVersion;
			legacy.fgIsPNOenabled = basic->fgIsPNOenabled;
			legacy.u4ScanDurBcnCnt = basic->u4ScanDurBcnCnt;
		}
			break;
		case UNI_EVENT_SCAN_DONE_TAG_SPARSECHNL: {
			struct UNI_EVENT_SCAN_DONE_SPARSECHNL *sparse =
				(struct UNI_EVENT_SCAN_DONE_SPARSECHNL *) tag;
			/* Event Type TAG_SPARSECHNL should
			 * have 8 bytes contents.
			 */
			if (sparse->u2Length !=
				sizeof(struct UNI_EVENT_SCAN_DONE_SPARSECHNL)) {
				fgIsValidScanDone = FALSE;
				break;
			}
			legacy.ucSparseChannelValid =
				sparse->ucSparseChannelValid;
			legacy.rSparseChannel.ucBand = sparse->ucBand;
			legacy.rSparseChannel.ucChannelNum =
				sparse->ucChannelNum;
			legacy.ucSparseChannelArrayValidNum =
				sparse->ucSparseChannelArrayValidNum;
		}
			break;
		case UNI_EVENT_SCAN_DONE_TAG_CHNLINFO: {
			struct UNI_EVENT_SCAN_DONE_CHNLINFO *chnlinfo =
				(struct UNI_EVENT_SCAN_DONE_CHNLINFO *) tag;
			struct UNI_EVENT_CHNLINFO *chnl =
				(struct UNI_EVENT_CHNLINFO *)
				chnlinfo->aucChnlInfoBuffer;

			ASSERT(chnlinfo->ucNumOfChnl <
				SCAN_DONE_EVENT_MAX_CHANNEL_NUM);
			ASSERT(chnlinfo->u2Length == sizeof(*chnlinfo) +
				chnlinfo->ucNumOfChnl * sizeof(*chnl));
			for (i = 0; i < chnlinfo->ucNumOfChnl &&
				    i < SCAN_DONE_EVENT_MAX_CHANNEL_NUM;
				    i++, chnl++) {
				legacy.aucChannelNum[i] =
					chnl->ucChannelNum;
				legacy.au2ChannelIdleTime[i] =
					chnl->u2ChannelIdleTime;
				legacy.aucChannelBAndPCnt[i] =
					chnl->ucChannelBAndPCnt;
				legacy.aucChannelMDRDYCnt[i] =
					chnl->ucChannelMDRDYCnt;
				legacy.au2ChannelScanTime[i] =
					chnl->u2ChannelScanTime;
				legacy.aucChannelUtilVal[i] =
					chnl->ucChannelUtilVal;
			}
		}
			break;
		case UNI_EVENT_SCAN_DONE_TAG_NLO:{
			struct UNI_EVENT_SCAN_DONE_NLO *nlo =
				(struct UNI_EVENT_SCAN_DONE_NLO *) tag;
			struct EVENT_SCHED_SCAN_DONE sched;
			/* Event Type NLO should have 8 bytes contents. */
			if (nlo->u2Length
				!= sizeof(struct UNI_EVENT_SCAN_DONE_NLO)) {
				fgIsValidScanDone = FALSE;
				break;
			}
			sched.ucStatus = nlo->ucStatus;
			sched.ucSeqNum = legacy.ucSeqNum;

			/* sched scan done, return directly */
			scnEventSchedScanDone(ad, &sched);
			return;
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(SCN, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	/* Check whether FW Event State and
	 * Complete Channel Number are correct.
	 */
	if (legacy.ucCurrentState != FW_SCAN_STATE_SCAN_DONE) {
		DBGLOG(NIC, ERROR, "ucCurrentState(%d) is Invalid!\n",
			legacy.ucCurrentState);
		fgIsValidScanDone = FALSE;
	}
	if (legacy.ucSparseChannelValid == 1 &&
		(legacy.ucCompleteChanCount !=
			legacy.ucSparseChannelArrayValidNum)){
		DBGLOG(NIC, ERROR,
			"CompleteChnlCnt(%d) and ChnlArrNum(%d) are mismatched!\n"
			, legacy.ucCompleteChanCount,
			legacy.ucSparseChannelArrayValidNum);
		fgIsValidScanDone = FALSE;
	}

	if (fgIsValidScanDone == TRUE)
		scnEventScanDone(ad, &legacy, TRUE);
}

uint32_t nicUniUpdateStaRecFastAll(
	struct ADAPTER *ad,
	struct BSS_INFO *bss)
{
	struct UNI_CMD_STAREC *uni_cmd;
	struct UNI_CMD_STAREC_FASTALL *tag;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_STAREC) +
				   sizeof(struct UNI_CMD_STAREC_FASTALL);

	if (!ad || !bss)
		return WLAN_STATUS_FAILURE;

	if ((IS_BSS_AIS(bss) && !bss->fgIsAisSwitchingChnl)
	   || (IS_BSS_GC(bss) && !bss->prStaRecOfAP)
#if CFG_ENABLE_WIFI_DIRECT
	   || (IS_BSS_APGO(bss) && !bssGetClientCount(ad, bss)))
#else
		)
#endif
		return WLAN_STATUS_INVALID_DATA;

	uni_cmd = (struct UNI_CMD_STAREC *)
		cnmMemAlloc(ad,
		RAM_TYPE_MSG, max_cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
			   "Allocate UNI_CMD_STAREC ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}
	uni_cmd->ucBssInfoIdx = bss->ucBssIndex;
	WCID_SET_H_L(uni_cmd->ucWlanIdxHnVer,
		uni_cmd->ucWlanIdxL, UNI_CMD_STAREC_INVALID_WTBL_IDX);

	tag = (struct UNI_CMD_STAREC_FASTALL *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_STAREC_TAG_FAST_ALL;
	tag->u2Length = sizeof(*tag);
	tag->ucUpdateFlag = UNI_CMD_STAREC_FASTALL_FLAG_UPDATE_BAND;

	status = wlanSendSetQueryUniCmd(ad,
				 UNI_CMD_ID_STAREC_INFO,
				 TRUE,
				 FALSE,
				 FALSE,
				 nicUniCmdEventSetCommon,
				 nicUniCmdTimeoutCommon,
				 max_cmd_len,
				 (void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);

	return status;
}

#if (CFG_SUPPORT_DFS_MASTER == 1)
static uint32_t MT_ATEInsertRDD(
	struct _ATE_LOG_DUMP_ENTRY *entry,
	uint8_t *data, uint32_t len)
{
	int8_t ret = 0;
	struct _ATE_RDD_LOG *result = NULL;
	uint32_t *pulse = 0;

	if (!entry)
		goto err0;

	if (!data)
		goto err0;

	kalMemZero(entry, sizeof(*entry));

	entry->un_dumped = TRUE;

	if (len > sizeof(entry->rdd))
		len = sizeof(entry->rdd);

	kalMemMove((uint8_t *)&entry->rdd, data, len);

	result = &entry->rdd;
	pulse = (uint32_t *)result->aucBuffer;

	DBGLOG(INIT, ERROR,
		"[RDD]0x%08x %08x\n", pulse[0], pulse[1]);

	return ret;
err0:
	DBGLOG(RFTEST, ERROR, "%s: NULL entry %p, data %p\n",
			__func__, entry, data);
	return -1;
}

uint32_t MT_ATEInsertLog(struct ADAPTER *prAdapter, uint8_t *log, uint32_t len)
{
	uint32_t ret = 0;

	struct _ATE_LOG_DUMP_CB *log_cb;
	uint32_t idx = 0;
	uint32_t is_dumping = 0;
	uint32_t (*insert_func)(
		struct _ATE_LOG_DUMP_ENTRY *entry,
		uint8_t *data, uint32_t len) = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;


	insert_func = MT_ATEInsertRDD;

	if (!insert_func)
		goto err1;

	log_cb = &prAdapter->rRddRawData;
	idx = log_cb->idx;
	is_dumping = log_cb->is_dumping;

	if (is_dumping)
		goto err1;

	if ((log_cb->idx + 1) == log_cb->len) {
		if (!log_cb->overwritable)
			goto err0;
		else
			log_cb->is_overwritten = TRUE;
	}

	if (!log_cb->entry)
		goto err0;

	ret = insert_func(&log_cb->entry[idx], log, len);

	if (ret)
		goto err0;

	INC_RING_INDEX2(log_cb->idx, log_cb->len);
	DBGLOG(CNM, WARN,
	"idx:%d, log_cb->idx:%d\n",
			  idx, log_cb->idx);
	return ret;
err0:
	DBGLOG(CNM, ERROR, "[WARN]: idx:%x, overwritable:%x\n",
			  idx, (log_cb) ? log_cb->overwritable:0xff);
err1:
	DBGLOG(CNM, ERROR, "Log dumping\n");
	return status;

}

void nicUniEventRDD(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_RDD);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;

	DBGLOG_MEM8(CNM, TRACE, data, data_len);

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;

	switch (TAG_ID(tag)) {
	case UNI_EVENT_RDD_TAG_SEND_PULSE: {
		struct UNI_EVENT_RDD_SEND_PULSE *pulse =
			(struct UNI_EVENT_RDD_SEND_PULSE *)tag;
		struct EVENT_RDD_REPORT rEventBody = {0};

		rEventBody.u1RddIdx = pulse->u1RddIdx;
		rEventBody.u1LongDetected = pulse->u1LongDetected;
		rEventBody.u1ConstantPRFDetected
			= pulse->u1ConstantPRFDetected;
		rEventBody.u1StaggeredPRFDetected
			= pulse->u1StaggeredPRFDetected;
		rEventBody.u1RadarTypeIdx = pulse->u1RadarTypeIdx;
		rEventBody.u1PeriodicPulseNum
			= pulse->u1PeriodicPulseNum;
		rEventBody.u1LongPulseNum = pulse->u1LongPulseNum;
		rEventBody.u1HwPulseNum = pulse->u1HwPulseNum;
		rEventBody.u1OutLPN = pulse->u1OutLPN;
		rEventBody.u1OutSPN = pulse->u1OutSPN;
		rEventBody.u1OutCRPN = pulse->u1OutCRPN;
		rEventBody.u1OutCRPW = pulse->u1OutCRPW;
		rEventBody.u1OutCRBN = pulse->u1OutCRBN;
		rEventBody.u1OutSTGPN = pulse->u1OutSTGPN;
		rEventBody.u1Reserve = pulse->u1Reserve;
		rEventBody.u4OutPRI_CONST = pulse->u4OutPRI_CONST;
		rEventBody.u4OutPRI_STG1 = pulse->u4OutPRI_STG1;
		rEventBody.u4OutPRI_STG2 = pulse->u4OutPRI_STG2;
		rEventBody.u4OutPRI_STG3 = pulse->u4OutPRI_STG3;
		rEventBody.u4OutPRIStgDmin = pulse->u4OutPRIStgDmin;
		DBGLOG(CNM, WARN,
			"legacy event tag = %d\n",
			TAG_ID(tag));
		RUN_RX_EVENT_HANDLER(EVENT_ID_RDD_REPORT,
			&rEventBody);
	}
		break;

	case UNI_EVENT_RDD_TAG_REPORT: {

		struct _ATE_RDD_LOG unit;
		struct UNI_EVENT_RDD_REPORT *log =
			(struct UNI_EVENT_RDD_REPORT *)tag;
		uint64_t *data = (uint64_t *)log->aucBuffer;
		int8_t i = 0;
		uint64_t len = 0;
		uint32_t dbg_len = 0;
		uint32_t *tmp = 0;
		int8_t k = 0;

		log->u4FuncLength = le2cpu32(log->u4FuncLength);
		log->u4Prefix = le2cpu32(log->u4Prefix);
		log->u4Count = le2cpu32(log->u4Count);

		dbg_len = (log->u4FuncLength
			- sizeof(struct UNI_EVENT_RDD_REPORT)
			+ sizeof(log->u4FuncIndex)
			+ sizeof(log->u4FuncIndex)) >> 2;

		len = dbg_len / 2;

		tmp = (uint32_t *)log->aucBuffer;

		for (k = 0; k < log->u4Count; k = k + 8) {
			DBGLOG(CNM, WARN,
			"RDD RAW DWORD1 %8d\t%3d\t\t%4d\t\t%d\t\t%d\t\t%02x%02x%02x%02x"
			, 1
			, 1
			, 1
			, 1
			, 1
			, log->aucBuffer[k+3], log->aucBuffer[k+2]
			, log->aucBuffer[k+1], log->aucBuffer[k]);
			DBGLOG(CNM, WARN,
			" %02x%02x%02x%02x\n"
			, log->aucBuffer[k+7], log->aucBuffer[k+6]
			, log->aucBuffer[k+5], log->aucBuffer[k+4]);
		}

		for (i = 0; i < dbg_len; i++)
			DBGLOG(CNM, WARN,
				"RDD RAW DWORD%d:%08x\n", i, tmp[i]);

		DBGLOG(CNM, WARN,
		"RDD FuncLen:%u, len:%u, prefix:%08x, cnt:%u, dbg_len:%u, len:%u\n",
		log->u4FuncLength, len, log->u4Prefix,
		log->u4Count, dbg_len, len);

		kalMemZero(&unit, sizeof(unit));
		unit.u4Prefix = log->u4Prefix;
		unit.u4Count = log->u4Count/8;

		for (i = 0; i < len; i++) {
			kalMemCopy(unit.aucBuffer, data++,
				ATE_RDD_LOG_SIZE);
			MT_ATEInsertLog(ad,
				(uint8_t *)&unit, sizeof(unit));
			unit.byPass = TRUE;
		}
	}
		break;

	default:
		fail_cnt++;
		DBGLOG(CNM, WARN,
			"invalid tag = %d, cnt: %d\n",
			TAG_ID(tag), fail_cnt);
		break;
	}

}
#endif /*(CFG_SUPPORT_DFS_MASTER == 1)*/

void nicUniUpdateMbmcIdx(struct ADAPTER *ad,
	uint8_t ucBssIdx,
	uint8_t ucBandIdx)
{
	struct BSS_INFO *prBssInfo = GET_BSS_INFO_BY_INDEX(ad,
		ucBssIdx);

	if (prBssInfo) {
		DBGLOG(CNM, INFO, "ucBssIdx=%d, eHwBandIdx=%d, ucBandIdx=%d\n",
			ucBssIdx, prBssInfo->eHwBandIdx, ucBandIdx);

		if (prBssInfo->eHwBandIdx != ucBandIdx &&
		    prBssInfo->eHwBandIdx != ENUM_BAND_AUTO)
			nicUniUpdateStaRecFastAll(ad, prBssInfo);
		prBssInfo->eBackupHwBandIdx = prBssInfo->eHwBandIdx;
		prBssInfo->eHwBandIdx = (enum ENUM_MBMC_BN)ucBandIdx;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		mldBssUpdateBandIdxBitmap(ad, prBssInfo);
#endif
	} else {
		DBGLOG(CNM, ERROR, "ucBssIdx=%d, ucBandIdx=%d\n",
			ucBssIdx, ucBandIdx);
	}
}

void nicUniEventChMngrHandleChEvent(struct ADAPTER *ad,
	struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_CNM);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	uint8_t bss_idx = MAX_BSSID_NUM + 1;
	uint8_t req_type = CH_REQ_TYPE_NUM;
	uint8_t s1 = 0;

	DBGLOG_MEM8(CNM, TRACE, data, data_len);

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(CNM, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_CNM_TAG_CH_PRIVILEGE_MLO_SUB_GRANT: {
			struct UNI_EVENT_CNM_CH_PRIVILEGE_GRANT *grant =
				(struct UNI_EVENT_CNM_CH_PRIVILEGE_GRANT *)tag;

			/* for CH_GRANT_INFO */
			bss_idx = grant->ucBssIndex;
			req_type = grant->ucReqType;

			nicUniUpdateMbmcIdx(ad, grant->ucBssIndex,
				grant->ucDBDCBand);
		}
			break;
		case UNI_EVENT_CNM_TAG_CH_PRIVILEGE_GRANT: {
			struct UNI_EVENT_CNM_CH_PRIVILEGE_GRANT *grant =
				(struct UNI_EVENT_CNM_CH_PRIVILEGE_GRANT *)tag;
			struct EVENT_CH_PRIVILEGE legacy = {0};

			/* for CH_GRANT_INFO */
			bss_idx = grant->ucBssIndex;
			req_type = grant->ucReqType;
			s1 = grant->ucRfCenterFreqSeg1;

			nicUniUpdateMbmcIdx(ad, grant->ucBssIndex,
				grant->ucDBDCBand);

			legacy.ucBssIndex = grant->ucBssIndex;
			legacy.ucTokenID = grant->ucTokenID;
			legacy.ucStatus = grant->ucStatus;
			legacy.ucPrimaryChannel = grant->ucPrimaryChannel;
			legacy.ucRfSco = grant->ucRfSco;
			legacy.ucRfBand = grant->ucRfBand;
			switch (grant->ucRfChannelWidth) {
			case UNI_CMD_CNM_CHANNEL_WIDTH_20_40MHZ:
				legacy.ucRfChannelWidth = CW_20_40MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_80MHZ:
				legacy.ucRfChannelWidth = CW_80MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_160MHZ:
				legacy.ucRfChannelWidth = CW_160MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_80P80MHZ:
				legacy.ucRfChannelWidth = CW_80P80MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_320MHZ:
				legacy.ucRfChannelWidth = rlmGetVhtOpBw320ByS1(
					grant->ucRfCenterFreqSeg1);
				break;
			default:
				legacy.ucRfChannelWidth = CW_20_40MHZ;
				break;
			}
			legacy.ucRfCenterFreqSeg1 = grant->ucRfCenterFreqSeg1;
			legacy.ucRfCenterFreqSeg2 = grant->ucRfCenterFreqSeg2;
			legacy.ucReqType = grant->ucReqType;
			legacy.ucDBDCBand = grant->ucDBDCBand;
			legacy.u4GrantInterval = grant->u4GrantInterval;

			RUN_RX_EVENT_HANDLER(EVENT_ID_CH_PRIVILEGE, &legacy);
		}
			break;
		case UNI_EVENT_CNM_TAG_CH_GRANT_INFO: {
			struct UNI_EVENT_CNM_CH_GRANT_INFO *info =
				(struct UNI_EVENT_CNM_CH_GRANT_INFO *)tag;
			struct BSS_INFO *prBssInfo =
				GET_BSS_INFO_BY_INDEX(ad, bss_idx);

			if (prBssInfo &&
				(req_type == CH_REQ_TYPE_JOIN ||
				 req_type == CH_REQ_TYPE_GO_START_BSS)) {
				prBssInfo->ucGrantTxNss = info->ucTxNss;
				prBssInfo->ucGrantRxNss = info->ucRxNss;

				switch (info->ucChannelWidth) {
				case UNI_CMD_CNM_CHANNEL_WIDTH_160MHZ:
					prBssInfo->ucGrantBW = MAX_BW_160MHZ;
					break;
				case UNI_CMD_CNM_CHANNEL_WIDTH_320MHZ:
					prBssInfo->ucGrantBW =
						rlmGetVhtOpBw320ByS1(s1) ==
						VHT_OP_CHANNEL_WIDTH_320_1 ?
						MAX_BW_320_1MHZ :
						MAX_BW_320_2MHZ;
					break;
				default:
					prBssInfo->ucGrantBW = MAX_BW_UNKNOWN;
				break;
				}

				DBGLOG(CNM, INFO,
					"Channel granted TxNss = %d, RxNss = %d, BW = %d\n",
					prBssInfo->ucGrantTxNss,
					prBssInfo->ucGrantRxNss,
					prBssInfo->ucGrantBW);
			}
		}
			break;
		case UNI_EVENT_CNM_TAG_OPMODE_CHANGE: {
			struct UNI_EVENT_CNM_OPMODE_CHANGE *opmode =
				(struct UNI_EVENT_CNM_OPMODE_CHANGE *)tag;
			struct EVENT_OPMODE_CHANGE legacy = {0};

			legacy.ucBssBitmap = (uint8_t) opmode->u2BssBitmap;
			legacy.ucEnable = opmode->ucEnable;
			legacy.ucOpTxNss = opmode->ucOpTxNss;
			legacy.ucOpRxNss = opmode->ucOpRxNss;
			legacy.ucReason = opmode->ucReason;
			legacy.ucBandWidth = opmode->ucBandWidth;
			switch (opmode->ucBandWidth) {
			case UNI_CMD_CNM_CHANNEL_WIDTH_20_40MHZ:
				legacy.ucBandWidth = MAX_BW_40MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_80MHZ:
				legacy.ucBandWidth = MAX_BW_80MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_160MHZ:
				legacy.ucBandWidth = MAX_BW_160MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_80P80MHZ:
				legacy.ucBandWidth = MAX_BW_80_80_MHZ;
				break;
			case UNI_CMD_CNM_CHANNEL_WIDTH_320MHZ:
				/* TODO: AIS, GO:
				 * MAX_BW_320_1MHZ, MAX_BW_320_2MHZ
				 *
				 * hardcode set to 320_2
				 */
				legacy.ucBandWidth = MAX_BW_320_2MHZ;
				break;
			default:
				legacy.ucBandWidth = MAX_BW_UNKNOWN;
				break;
			}

			RUN_RX_EVENT_HANDLER(EVENT_ID_OPMODE_CHANGE, &legacy);
		}
			break;
		case UNI_EVENT_CNM_TAG_OPMODE_CHANGE_RDD: {
			struct UNI_EVENT_CNM_RDD_OPMODE_CHANGE *opmode =
				(struct UNI_EVENT_CNM_RDD_OPMODE_CHANGE *)tag;
			struct EVENT_RDD_OPMODE_CHANGE legacy = {0};

			legacy.ucBssBitmap = (uint8_t) opmode->u2BssBitmap;
			legacy.ucEnable = opmode->ucEnable;
			legacy.ucOpTxNss = opmode->ucOpTxNss;
			legacy.ucOpRxNss = opmode->ucOpRxNss;
			legacy.ucReason = opmode->ucReason;
			legacy.ucPriChannel = opmode->ucPriChannel;
			legacy.ucChBw = opmode->ucChBw;
			legacy.ucAction = opmode->ucAction;
			RUN_RX_EVENT_HANDLER(EVENT_ID_RDD_OPMODE_CHANGE,
				&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(CNM, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

static void nicUniEventMbmcSwitchUpdate(
	struct ADAPTER *ad, struct UNI_EVENT_MBMC_SWITCH_DONE *switch_evt)
{
	uint8_t bitmap = 0;
	uint16_t bands = 0;
	uint8_t bss = 0;

	if (!ad || !switch_evt || !(switch_evt->ucMBMCCmdSuccess))
		return;

	bitmap = switch_evt->ucBssIndexValidBitmap;
	bands = switch_evt->u2UsedBssBandIndexBitmap;

	/* Using 1 bit show each existed BSS index
	 * Using 2 bits show each existed BSS Band index
	 * for Example:
	 *    if Bss0 is on band1, Bss2 is on band2, and Bss3 is on band1
	 *    ucBssIndexValidBitmap = 13 (00001101)
	 *    u2UsedBssBandIndexBitmap = 97 (00000000 01100001)
	 */
	for (bss = 0; bss < 8 && bitmap > 0 ; bss++) {
		if (bitmap & 0x1) {
			uint8_t band = bands & BITS(0, 1);

			DBGLOG(CNM, TRACE,
				"Update BSS%d to Band%d\n", bss, band);
			nicUniUpdateMbmcIdx(ad, bss, band);
		}
		bitmap >>= 1;
		bands >>= 2;
	}
}

void nicUniEventMbmcHandleEvent(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_MBMC);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(CNM, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_MBMC_TAG_SWITCH_DONE:
			nicUniEventMbmcSwitchUpdate(
				ad, (struct UNI_EVENT_MBMC_SWITCH_DONE *)tag);

			RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_DBDC_SWITCH_DONE,
								NULL, 0);
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(CNM, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventStatusToHost(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_STATUS_TO_HOST);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_STATUS_TO_HOST_TAG_TX_DONE:{
			struct UNI_EVENT_TX_DONE *tx =
				(struct UNI_EVENT_TX_DONE *) tag;
			struct EVENT_TX_DONE legacy = {0};

			legacy.ucPacketSeq = tx->ucPacketSeq;
			legacy.ucStatus = tx->ucStatus;
			legacy.u2SequenceNumber = tx->u2SequenceNumber;
			legacy.ucWlanIndex = tx->ucWlanIndex;
			legacy.ucTxCount = tx->ucTxCount;
			legacy.u2TxRate = tx->u2TxRate;

			legacy.ucFlag = tx->ucFlag;
			legacy.ucTid = tx->ucTid;
			legacy.ucRspRate = tx->ucRspRate;
			legacy.ucRateTableIdx = tx->ucRateTableIdx;

			legacy.ucBandwidth = tx->ucBandwidth;
			legacy.ucTxPower = tx->ucTxPower;
			legacy.ucFlushReason = tx->ucFlushReason;

			legacy.u4TxDelay = tx->u4TxDelay;
			legacy.u4Timestamp = tx->u4Timestamp;
			legacy.u4AppliedFlag = tx->u4AppliedFlag;

			RUN_RX_EVENT_HANDLER(EVENT_ID_TX_DONE, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

/**
 * nicUniHandleEventRxAddBa() - Handle ADD RX BA unified Event from the FW
 * @prAdapter: Adapter pointer
 * @prEvent: The event packet from the FW
 */
static void nicUniHandleEventRxAddBa(struct ADAPTER *prAdapter,
		struct UNI_EVENT_RX_ADDBA *prEvent)
{
	struct STA_RECORD *prStaRec;
	uint8_t ucStaRecIdx;
	uint16_t u2WinStart;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_STA_RECORD *prMldSta = NULL;
#endif

	DBGLOG(QM, DEBUG, "QM:Event +RxBaEht\n");

	ucStaRecIdx = secGetStaIdxByWlanIdx(prAdapter, prEvent->u2WlanIdx);
	prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaRecIdx);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldSta = mldStarecGetByStarec(prAdapter, prStaRec);
	if (prMldSta) {
		uint8_t ucStaIndex = secGetStaIdxByWlanIdx(prAdapter,
					prMldSta->u2PrimaryMldId);

		prStaRec = QM_GET_STA_REC_PTR_FROM_INDEX(prAdapter, ucStaIndex);
		if (ucStaRecIdx != ucStaIndex)
			DBGLOG(QM, DEBUG,
				"Change primary wlan_idx from %d to %d\n",
				ucStaRecIdx, ucStaIndex);
	}
#endif

	u2WinStart = prEvent->u2BAStartSeqCtrl >> OFFSET_BAR_SSC_SN;
	nicEventHandleAddBa(prAdapter, prStaRec, prEvent->ucTid,
			prEvent->u2WinSize, u2WinStart);
}

void nicUniEventBaOffload(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BA_OFFLOAD);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_BA_OFFLOAD_TAG_RX_ADDBA:{
			struct UNI_EVENT_RX_ADDBA *ba =
				(struct UNI_EVENT_RX_ADDBA *) tag;
			nicUniHandleEventRxAddBa(ad, ba);
#if CFG_SUPPORT_WED_PROXY
			wedStaRecRxAddBaUpdate(ad, ba);
#endif
		}
			break;
		case UNI_EVENT_BA_OFFLOAD_TAG_RX_DELBA:{
			struct UNI_EVENT_RX_DELBA *ba =
				(struct UNI_EVENT_RX_DELBA *) tag;
			struct EVENT_RX_DELBA legacy = {0};

			legacy.ucStaRecIdx =
				secGetStaIdxByWlanIdx(ad, ba->u2WlanIdx);
			legacy.ucTid = ba->ucTid;

			RUN_RX_EVENT_HANDLER(EVENT_ID_RX_DELBA, &legacy);
		}
			break;
		case UNI_EVENT_BA_OFFLOAD_TAG_TX_ADDBA:{
			struct UNI_EVENT_TX_ADDBA *ba =
				(struct UNI_EVENT_TX_ADDBA *) tag;
			struct EVENT_TX_ADDBA legacy = {0};

			legacy.ucStaRecIdx =
				secGetStaIdxByWlanIdx(ad, ba->u2WlanIdx);
			legacy.ucTid = ba->ucTid;
			/* WARN: ba1024 win size is truncated, it's okay
			 * now because qmHandleEventTxAddBa doesn't use it
			 */
			legacy.ucWinSize = ba->u2WinSize;
			legacy.ucAmsduEnBitmap = ba->ucAmsduEnBitmap;
			legacy.u2SSN = ba->u2SSN;
			legacy.ucMaxMpduCount = ba->ucMaxMpduCount;
			legacy.u4MaxMpduLen = ba->u4MaxMpduLen;
			legacy.u4MinMpduLen = ba->u4MinMpduLen;

			RUN_RX_EVENT_HANDLER(EVENT_ID_TX_ADDBA, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniSolicitEventBaOffload(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	uint8_t *tag;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BA_OFFLOAD);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	uint16_t tags_len = data_len - fixed_len;
	uint16_t offset = 0;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_BA_OFFLOAD_TAG_TX_AGG_LIMIT: {
			struct UNI_EVENT_TX_AGG_LIMIT *ba =
				(struct UNI_EVENT_TX_AGG_LIMIT *) tag;
			DBGLOG(NIC, DEBUG, "Tag(%d) bss:%u status:%u\n",
				TAG_ID(tag), ba->ucBssIdx, ba->ucStatus);
			if (ba->ucStatus)
				u4Status = WLAN_STATUS_FAILURE;
			break;
		}
		case UNI_EVENT_BA_OFFLOAD_TAG_TX_AMSDU_NUM_LIMIT: {
			struct UNI_EVENT_TX_AMSDU_NUM_LIMIT *ba =
				(struct UNI_EVENT_TX_AMSDU_NUM_LIMIT *) tag;
			DBGLOG(NIC, DEBUG, "Tag(%d) bss:%u status:%u\n",
				TAG_ID(tag), ba->ucBssIdx, ba->ucStatus);
			if (ba->ucStatus)
				u4Status = WLAN_STATUS_FAILURE;
			break;
		}
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	if (prCmdInfo->fgIsOid)
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			0, u4Status);
}

void nicUniEventSleepNotify(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_SLEEP_NOTIFY);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_SLEEP_NOTYFY_TAG_SLEEP_INFO:{
			struct UNI_EVENT_SLEEP_INFO *info =
				(struct UNI_EVENT_SLEEP_INFO *) tag;
			struct EVENT_SLEEPY_INFO legacy = {0};

			legacy.ucSleepyState = info->ucSleepyState;

			RUN_RX_EVENT_HANDLER(EVENT_ID_SLEEPY_INFO, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventBeaconTimeout(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BEACON_TIMEOUT);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_BEACON_TIMEOUT *timeout;
	struct EVENT_BSS_BEACON_TIMEOUT legacy = {0};

	timeout = (struct UNI_EVENT_BEACON_TIMEOUT *) data;
	legacy.ucBssIndex = timeout->ucBssIndex;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_BEACON_TIMEOUT_TAG_INFO:{
			struct UNI_EVENT_BEACON_TIMEOUT_INFO *info =
				(struct UNI_EVENT_BEACON_TIMEOUT_INFO *) tag;

			legacy.ucReasonCode = info->ucReasonCode;
			legacy.ucLinkDtThreshold = info->ucLinkDtThreshold;

			RUN_RX_EVENT_HANDLER(EVENT_ID_BSS_BEACON_TIMEOUT,
					&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventUpdateCoex(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_UPDATE_COEX);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	uint8_t i;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_UPDATE_COEX_TAG_PHYRATE:{
			struct UNI_EVENT_UPDATE_COEX_PHYRATE *phyrate =
				(struct UNI_EVENT_UPDATE_COEX_PHYRATE *) tag;
			struct EVENT_UPDATE_COEX_PHYRATE legacy = {0};

			for (i = 0; i < MAX_BSSID_NUM + 1; i++) {
				legacy.au4PhyRateLimit[i] =
					phyrate->au4PhyRateLimit[i];
			}

			RUN_RX_EVENT_HANDLER(EVENT_ID_UPDATE_COEX_PHYRATE,
					&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventIdc(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_IDC);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_IDC_TAG_MD_SAFE_CHN:{
			struct UNI_EVENT_MD_SAFE_CHN *chn =
				(struct UNI_EVENT_MD_SAFE_CHN *) tag;
			struct EVENT_LTE_SAFE_CHN legacy = {0};

			legacy.ucVersion = chn->ucVersion;
			legacy.u4Flags = chn->u4Flags;
			kalMemCopy(legacy.rLteSafeChn.au4SafeChannelBitmask,
				chn->u4SafeChannelBitmask,
				sizeof(uint32_t) * ENUM_SAFE_CH_MASK_MAX_NUM);

#if CFG_SUPPORT_IDC_CH_SWITCH
			RUN_RX_EVENT_HANDLER(EVENT_ID_LTE_IDC_REPORT, &legacy);
#endif
		}
			break;
		case UNI_EVENT_IDC_TAG_CCCI_MSG:
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventBssIsAbsence(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BSS_IS_ABSENCE);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_BSS_IS_ABSENCE *absence;

	absence = (struct UNI_EVENT_BSS_IS_ABSENCE *) data;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, LOUD, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_BSS_IS_ABSENCE_TAG_INFO: {
			struct UNI_EVENT_BSS_IS_ABSENCE_INFO *info=
				(struct UNI_EVENT_BSS_IS_ABSENCE_INFO *) tag;
			struct EVENT_BSS_ABSENCE_PRESENCE legacy = {0};

			legacy.ucBssIndex = absence->ucBssIndex;
			legacy.ucIsAbsent = info->ucIsAbsent;
			legacy.ucBssFreeQuota = info->ucBssFreeQuota;

			RUN_RX_EVENT_HANDLER(EVENT_ID_BSS_ABSENCE_PRESENCE,
								&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if CFG_ENABLE_WIFI_DIRECT

void nicUniEventPsSync(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_PS_SYNC);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_PS_SYNC_TAG_CLIENT_PS_INFO: {
			struct UNI_EVENT_CLIENT_PS_INFO *ps =
				(struct UNI_EVENT_CLIENT_PS_INFO *) tag;
			struct EVENT_STA_CHANGE_PS_MODE legacy = {0};

			legacy.ucStaRecIdx =
				secGetStaIdxByWlanIdx(ad, ps->ucWtblIndex);
			legacy.ucIsInPs = ps->ucPsBit;
			legacy.ucFreeQuota = ps->ucBufferSize;

			RUN_RX_EVENT_HANDLER(EVENT_ID_STA_CHANGE_PS_MODE,
								&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventSap(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_SAP);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_SAP_TAG_AGING_TIMEOUT: {
			struct UNI_EVENT_SAP_AGING_TIMEOUT *aging =
				(struct UNI_EVENT_SAP_AGING_TIMEOUT *) tag;
			struct EVENT_STA_AGING_TIMEOUT legacy = {0};

			legacy.ucStaRecIdx =
				secGetStaIdxByWlanIdx(ad, aging->u2WlanIdx);

			RUN_RX_EVENT_HANDLER(EVENT_ID_STA_AGING_TIMEOUT,
								&legacy);
		}
			break;
		case UNI_EVENT_SAP_TAG_UPDATE_STA_FREE_QUOTA: {
			struct UNI_EVENT_UPDATE_STA_FREE_QUOTA *quota =
				(struct UNI_EVENT_UPDATE_STA_FREE_QUOTA *) tag;
			struct EVENT_STA_UPDATE_FREE_QUOTA legacy = {0};

			legacy.ucStaRecIdx =
				secGetStaIdxByWlanIdx(ad, quota->u2WlanIdx);
			legacy.ucUpdateMode = quota->ucUpdateMode;
			legacy.ucFreeQuota = quota->ucFreeQuota;

			RUN_RX_EVENT_HANDLER(EVENT_ID_STA_UPDATE_FREE_QUOTA,
								&legacy);
		}
			break;
#ifdef CFG_AP_GO_DELAY_CARRIER_ON
		case UNI_EVENT_SAP_TAG_NOTIFY_AP_GO_STARTED: {
			struct UNI_EVENT_NOTIFY_AP_GO_STARTED *started =
				(struct UNI_EVENT_NOTIFY_AP_GO_STARTED *) tag;
			struct MSG_P2P_NOTIFY_APGO_STARTED *prNotifyMsg = NULL;

			prNotifyMsg = (struct MSG_P2P_NOTIFY_APGO_STARTED *)
				cnmMemAlloc(ad, RAM_TYPE_MSG,
					    sizeof(*prNotifyMsg));
			if (!prNotifyMsg) {
				DBGLOG(NIC, ERROR, "Alloc mem(%zu) failed\n",
					sizeof(*prNotifyMsg));
				break;
			}

			prNotifyMsg->rMsgHdr.eMsgId =
				MID_MNY_P2P_NOTIFY_APGO_STARTED;
			prNotifyMsg->ucBssIdx = started->ucBssIdx;
			mboxSendMsg(ad, MBOX_ID_0,
				    (struct MSG_HDR *)prNotifyMsg,
				    MSG_SEND_METHOD_UNBUF);
		}
			break;
#endif /* CFG_AP_GO_DELAY_CARRIER_ON */
#if (CFG_SUPPORT_SAP_LINK_TSF_DIFF == 1)
		case UNI_EVENT_SAP_TAG_NOTIFY_LINK_TSF: {
			struct UNI_EVENT_LINK_TSF *tsf =
				(struct UNI_EVENT_LINK_TSF *) tag;
			struct LINK_TSF_ENTRY *entry;
			struct BSS_INFO *bss;
			uint8_t i;

			for (i = 0; i < tsf->ucLinkNum && i < MLD_LINK_MAX;
			     i++) {
				entry = &tsf->aucLinkTsf[i];
				bss = GET_BSS_INFO_BY_INDEX(ad,
					entry->ucBssIdx);

				if (!bss)
					continue;

				bss->i8TsfValue =
					((int64_t)entry->u4TsfBit0_31) +
					((int64_t)entry->u4TsfBit63_32 << 32);
				DBGLOG(NIC, INFO, "bss=%u tsf=%lld\n",
					bss->ucBssIndex, bss->i8TsfValue);
			}
		}
			break;
#endif /* CFG_SUPPORT_SAP_LINK_TSF_DIFF */
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif
void nicUniEventOBSS(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_OBSS_UPDATE);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_OBSS_UPDATE *obss;
	struct EVENT_AP_OBSS_STATUS legacy = {0};

	obss = (struct UNI_EVENT_OBSS_UPDATE *) data;
	legacy.ucBssIndex = obss->ucBssIndex;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_OBSS_UPDATE_TAG_STATUS: {
			struct UNI_EVENT_OBSS_STATUS *status =
				(struct UNI_EVENT_OBSS_STATUS *) tag;

			legacy.ucObssErpProtectMode =
				status->ucObssErpProtectMode;
			legacy.ucObssHtProtectMode =
				status->ucObssHtProtectMode;
			legacy.ucObssGfOperationMode =
				status->ucObssGfOperationMode;
			legacy.ucObssRifsOperationMode =
				status->ucObssRifsOperationMode;
			legacy.ucObssBeaconForcedTo20M =
				status->ucObssBeaconForcedTo20M;

			RUN_RX_EVENT_HANDLER(EVENT_ID_AP_OBSS_STATUS,
								&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if CFG_SUPPORT_ROAMING
void nicUniEventRoaming(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_ROAMING);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_ROAMING *roam;
	struct CMD_ROAMING_TRANSIT legacy = {0};

	roam = (struct UNI_EVENT_ROAMING *) data;
	legacy.ucBssidx = roam->ucBssIndex;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_ROAMING_TAG_STATUS: {
			struct UNI_EVENT_ROAMING_STATUS *status =
				(struct UNI_EVENT_ROAMING_STATUS *) tag;

			legacy.u2Event = status->u2Event;
			legacy.u2Data = status->u2Data;
			legacy.u2RcpiLowThreshold = status->u2RcpiLowThreshold;
			legacy.eReason = status->eReason;
			legacy.u4RoamingTriggerTime =
				status->u4RoamingTriggerTime;
			legacy.u2RcpiHighThreshold =
				status->u2RcpiHighThreshold;

			roamingFsmProcessEvent(ad, &legacy);
		}
		break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

void nicUniEventAddKeyDone(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_ADD_KEY_DONE);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_ADD_KEY_DONE *done;
	struct EVENT_ADD_KEY_DONE_INFO legacy = {0};

	done = (struct UNI_EVENT_ADD_KEY_DONE *) data;
	legacy.ucBSSIndex = done->ucBssIndex;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_ADD_KEY_DONE_TAG_PKEY: {
			struct UNI_EVENT_ADD_PKEY_DONE *pkey =
				(struct UNI_EVENT_ADD_PKEY_DONE *) tag;

			COPY_MAC_ADDR(legacy.aucStaAddr, pkey->aucStaAddr);

			RUN_RX_EVENT_HANDLER(EVENT_ID_ADD_PKEY_DONE, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

static void nicUniEventPpStat(
	struct ADAPTER *ad,
	struct UNI_EVENT_PP_ALG_CTRL *tag)
{
	DBGLOG(REQ, DEBUG,
		"\x1b[32m ============= Pp Band%d Stat ============= \x1b[m\n",
		tag->u1DbdcIdx);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpTimerIntv            = %d              \x1b[m\n",
		tag->u4PpTimerIntv);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX2: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX2_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX2: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX2_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX3: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX3_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX3: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX3_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX4: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX4_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX4: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX4_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX5: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX5_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX5: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX5_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX6: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX6_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX6: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX6_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX7: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX7_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX7: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX7_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX8: Value	  = %d                   \x1b[m\n",
		tag->u4ThrX8_Value);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4PpThrX8: Shift	  = %d                   \x1b[m\n",
		tag->u4ThrX8_Shift);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4SwPpTime               = %d (Unit: %d ms)\x1b[m\n",
		tag->u4SwPpTime,
		tag->u4PpTimerIntv);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4HwPpTime               = %d (Unit: %d ms)\x1b[m\n",
		tag->u4HwPpTime,
		tag->u4PpTimerIntv);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4NoPpTime               = %d (Unit: %d ms)\x1b[m\n",
		tag->u4NoPpTime,
		tag->u4PpTimerIntv);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m u4AutoBwTime             = %d (Unit: %d ms)\x1b[m\n",
		tag->u4AutoBwTime,
		tag->u4PpTimerIntv);
	DBGLOG(REQ, DEBUG,
		"\x1b[32m ========================================== \x1b[m\n");

}

void nicUniEventPpCb(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	struct UNI_EVENT_PP *Ppevt = (struct UNI_EVENT_PP *)evt->aucBuffer;
	struct UNI_EVENT_PP_ALG_CTRL *tag =
		(struct UNI_EVENT_PP_ALG_CTRL *) Ppevt->au1TlvBuffer;

	switch (TAG_ID(tag)) {
	case UNI_EVENT_PP_TAG_ALG_CTRL:
		nicUniEventPpStat(ad, tag);
		break;
#if CFG_SUPPORT_802_PP_DSCB
	case UNI_EVENT_SAP_TAG_SAP_DSCB_IE: {
		struct UNI_EVENT_SAP_DSCB_IE *dscb =
			(struct UNI_EVENT_SAP_DSCB_IE *) tag;
		struct EVENT_STA_SAP_DSCB_IE legacy = {0};

		legacy.ucBssIndex = dscb->ucBssIndex;
		legacy.fgIsDscbEnable = dscb->fgIsDscbEnable;
		legacy.u2DscbBitmap = dscb->u2DscbBitmap;

		RUN_RX_EVENT_HANDLER(EVENT_ID_STATIC_PP_DSCB,
							&legacy);
	}
		break;
#endif
	default:
		DBGLOG(REQ, ERROR, "\x1b[31m %s: Invalid tag = %d\x1b[m\n"
			, __func__, TAG_ID(tag));
		break;
	}
}


void nicUniEventFwLog2Host(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_FW_LOG2HOST);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_FWLOG2HOST_TAG_FORMAT: {
			struct UNI_EVENT_FW_LOG_FORMAT *log =
				(struct UNI_EVENT_FW_LOG_FORMAT *) tag;
			struct EVENT_DEBUG_MSG *legacy;
			uint32_t len = log->u2Length - sizeof(*log);
			/* Reserved to set [u2MsgSize] = '\0' later */
			uint32_t size = sizeof(*legacy) + len + sizeof('\0');

			if (log->u2Length < sizeof(*log)) {
				DBGLOG(NIC, WARN, "Invalid tag length=%d\n",
					log->u2Length);
				break;
			}

			legacy = kalMemAlloc(size, VIR_MEM_TYPE);
			if (!legacy) {
				DBGLOG(NIC, WARN, "tag=%d OOM\n", TAG_ID(tag));
				break;
			}

			kalMemZero(legacy, sizeof(*legacy));

			legacy->ucMsgType = log->ucMsgFmt;
			legacy->u2MsgSize = len;
			kalMemCopy(legacy->aucMsg, log->acMsg, len);

			RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_DEBUG_MSG,
							legacy, size);

			kalMemFree(legacy, size, VIR_MEM_TYPE);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if CFG_ENABLE_WIFI_DIRECT
void nicUniEventP2p(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_P2P);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_P2P_TAG_UPDATE_NOA_PARAM: {
			struct UNI_EVENT_UPDATE_NOA_PARAM *noa =
				(struct UNI_EVENT_UPDATE_NOA_PARAM *) tag;
			struct EVENT_UPDATE_NOA_PARAMS legacy = {0};
			uint8_t i;

			legacy.ucBssIndex = noa->ucBssIndex;
			legacy.ucEnableOppPS = noa->ucEnableOppPS;
			legacy.u2CTWindow = noa->u2CTWindow;
			legacy.ucNoAIndex = noa->ucNoAIndex;
			legacy.ucNoATimingCount = noa->ucNoATimingCount;

			for (i = 0; i < 8; i++) {
				legacy.arEventNoaTiming[i].ucIsInUse =
					noa->arEventNoaTiming[i].ucIsInUse;
				legacy.arEventNoaTiming[i].ucCount =
					noa->arEventNoaTiming[i].ucCount;
				legacy.arEventNoaTiming[i].u4Duration =
					noa->arEventNoaTiming[i].u4Duration;
				legacy.arEventNoaTiming[i].u4Interval =
					noa->arEventNoaTiming[i].u4Interval;
				legacy.arEventNoaTiming[i].u4StartTime =
					noa->arEventNoaTiming[i].u4StartTime;
			}

			RUN_RX_EVENT_HANDLER(EVENT_ID_UPDATE_NOA_PARAMS,
								&legacy);
		}
			break;
		case UNI_EVENT_P2P_TAG_LO_STOP_PARAM: {
			struct UNI_EVENT_P2P_LO_STOP_PARAM *lostop =
				(struct UNI_EVENT_P2P_LO_STOP_PARAM *)tag;
			struct EVENT_P2P_LO_STOP_T legacy = {0};

			legacy.ucBssIndex = lostop->ucBssIndex;
			legacy.u4Reason = lostop->u4Reason;
			legacy.u2ListenCount = lostop->u2ListenCount;

			RUN_RX_EVENT_HANDLER(EVENT_ID_P2P_LO_STOP,
				&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventCountdown(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_IE_COUNTDOWNT);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_IE_COUNTDOWN_CSA: {
			struct UNI_EVENT_CSA_NOTIFY *csa =
				(struct UNI_EVENT_CSA_NOTIFY *) tag;

			DBGLOG(CNM, INFO, "OM=%d, count=%d\n",
				csa->ucOwnMacIdx, csa->ucChannelSwitchCount);
			RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_CSA_DONE, NULL, 0);
		}
			break;
		case UNI_EVENT_IE_COUNTDOWN_BCC: {
			// TODO: uni cmd
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

void nicUniEventStaRec(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_STAREC);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_STAREC *common;
	struct WIFI_VAR *prWifiVar;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	prWifiVar = &ad->rWifiVar;

	common = (struct UNI_EVENT_STAREC *) data;
	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_STAREC_TAG_UPDATE_MAX_AMSDU_LEN: {
			struct UNI_EVENT_STAREC_UPDATE_MAX_AMSDU_LEN *len =
			    (struct UNI_EVENT_STAREC_UPDATE_MAX_AMSDU_LEN *)tag;
			struct EXT_EVENT_MAX_AMSDU_LENGTH_UPDATE legacy = {0};

			legacy.ucWlanIdx = common->u2WlanIdx;
			legacy.ucAmsduLen = len->u2AmsduLen;
			RUN_RX_EXT_EVENT_HANDLER(
				EXT_EVENT_ID_MAX_AMSDU_LENGTH_UPDATE, &legacy);
		}
			break;
		case UNI_EVENT_STAREC_TAG_PN_INFO: {
			// TODO: uni cmd
		}
			break;

		case UNI_EVENT_STAREC_TAG_MLO_LINK_STATE: {
#if (CFG_SUPPORT_802_11BE_MLO == 1)
			struct UNI_EVENT_STAREC_MLO_LINK_STATE *state =
			    (struct UNI_EVENT_STAREC_MLO_LINK_STATE *)tag;
			struct BSS_INFO *prBssInfo;
			struct STA_RECORD *prStaRec;
			struct MLD_STA_RECORD *prMldStaRec;

			DBGLOG(ML, INFO,
				"widx=%d state=%d reason=%d\n",
				common->u2WlanIdx, state->ucLinkState,
				state->ucReason);

			prStaRec = cnmGetStaRecByWlanIndex(ad,
				common->u2WlanIdx);
			prMldStaRec = mldStarecGetByStarec(ad, prStaRec);
			if (prStaRec && prMldStaRec) {
				if (state->ucLinkState == MLO_LINK_STATE_ACTIVE)
					prMldStaRec->u8ActiveStaBitmap |=
						BIT(prStaRec->ucIndex);
				else
					prMldStaRec->u8ActiveStaBitmap &=
						~BIT(prStaRec->ucIndex);
				DBGLOG(ML, INFO,
					"bss=%d sta=%d widx=%d ActiveStaBitmap=0x%llx\n",
					prStaRec->ucBssIndex, prStaRec->ucIndex,
					prStaRec->ucWlanIndex,
					prMldStaRec->u8ActiveStaBitmap);
#if (CFG_SUPPORT_MLD_LOG == 1) && (CFG_SUPPORT_802_11BE_MLO == 1)
				mldLogLink(ad,
					prStaRec,
					prMldStaRec,
					state->ucLinkState,
					state->ucReason);
#endif

#if CFG_SUPPORT_NAN
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
				if (prWifiVar->ucNanEnable6gReschedInit == 1 &&
				    nanIsOn(ad)) {
					nanSchedUpdateP2pAisMcc(ad);
					nanRescheduleNdlIfNeeded(ad,
								 AIS_CONNECTED,
								 NULL);
				}
#endif
#endif

#ifdef CFG_SUPPORT_TWT_EXT
				twtmldCheckTeardown(ad,
					prMldStaRec->u8ActiveStaBitmap);
#endif
				prBssInfo = GET_BSS_INFO_BY_INDEX(ad,
					prStaRec->ucBssIndex);
				ccmChannelSwitchProducer(ad, prBssInfo,
							 __func__);
			}
#endif /* CFG_SUPPORT_802_11BE_MLO */
		}
			break;

		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if CFG_SUPPORT_TDLS
void nicUniEventTdls(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_TDLS);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_TDLS_TAG_TEAR_DOWN: {
			struct UNI_EVENT_TDLS_TEAR_DOWN *down =
			    (struct UNI_EVENT_TDLS_TEAR_DOWN *)tag;
			struct TDLS_EVENT legacy = {0};

			legacy.u4HostId = TDLS_HOST_EVENT_TEAR_DOWN;
			legacy.u4SubId = down->u4Subid;
			legacy.u4StaIdx =
				secGetStaIdxByWlanIdx(ad, down->u4WlanIdx);

			RUN_RX_EVENT_HANDLER(EVENT_ID_TDLS, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

void nicUniEventBssER(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BSS_ER);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_BSS_ER_TAG_TX_MODE: {

#if (CFG_SUPPORT_HE_ER == 1)
			struct UNI_EVENT_BSS_ER_TX_MODE *mode =
			    (struct UNI_EVENT_BSS_ER_TX_MODE *)tag;
			struct EVENT_ER_TX_MODE legacy = {0};

			legacy.ucBssInfoIdx = mode->ucBssInfoIdx;
			legacy.ucErMode = mode->ucErMode;

			RUN_RX_EVENT_HANDLER(EVENT_ID_BSS_ER_TX_MODE, &legacy);
#endif
		}
			break;
		case UNI_EVENT_MLR_TAG_FSM_UPDATE: {
#if (CFG_SUPPORT_MLR == 1)
			struct UNI_EVENT_MLR_FSM_UPDATE *fsm =
				(struct UNI_EVENT_MLR_FSM_UPDATE *)tag;
			struct EVENT_MLR_FSM_UPDATE legacy = {0};

			legacy.u2WlanIdx = fsm->u2WlanIdx;
			legacy.ucMlrMode = fsm->ucMlrMode;
			legacy.ucMlrState = fsm->ucMlrState;
			legacy.ucMlrTxdFrIdx = fsm->ucMlrTxdFrIdx;
			legacy.ucTxFragEn = fsm->ucTxFragEn;

			RUN_RX_EVENT_HANDLER(EVENT_ID_MLR_FSM_UPDATE, &legacy);
#endif
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventRssiMonitor(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_RSSI_MONITOR);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_RSSI_MONITOR_TAG_INFO: {
			struct UNI_EVENT_RSSI_MONITOR_INFO *info =
			    (struct UNI_EVENT_RSSI_MONITOR_INFO *)tag;

			RUN_RX_EVENT_HANDLER(EVENT_ID_RSSI_MONITOR,
				&info->cRssi);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventHifCtrl(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_HIF_CTRL);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_HIF_CTRL_TAG_BASIC: {
			struct UNI_EVENT_HIF_CTRL_BASIC *basic =
			    (struct UNI_EVENT_HIF_CTRL_BASIC *)tag;
			struct EVENT_HIF_CTRL legacy = {0};

			legacy.ucHifType = basic->ucHifType;
			legacy.ucHifTxTrafficStatus =
				basic->ucHifTxTrafficStatus;
			legacy.ucHifRxTrafficStatus =
				basic->ucHifRxTrafficStatus;
			legacy.ucHifSuspend = basic->ucHifSuspend;
#if (CFG_WIFI_PCIE_L2_MOBILE_ONLY == 1)
			legacy.u2HifPcieUserCount = basic->u2HifPcieUserCount;
#endif
			RUN_RX_EVENT_HANDLER(EVENT_ID_HIF_CTRL,	&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if CFG_SUPPORT_RTT
void nicUniEventRttCapabilities(struct ADAPTER
	*prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_RTT *evt =
		(struct UNI_EVENT_RTT *)uni_evt->aucBuffer;
	struct UNI_EVENT_RTT_CAPA_T *tag =
		(struct UNI_EVENT_RTT_CAPA_T *) evt->aucTlvBuffer;
	struct LOC_CAPABILITIES_T *prLocCapa =
		(struct LOC_CAPABILITIES_T *) &tag->rCapabilities;
	struct EVENT_RTT_CAPABILITIES legacy = {0};
	struct RTT_CAPABILITIES *prRttCapa =
		(struct RTT_CAPABILITIES *) &legacy.rCapabilities;

	prRttCapa->fgRttOneSidedSupported =
		prLocCapa->u2LocInitSupported & BIT(0) ? TRUE : FALSE;
	prRttCapa->fgRttFtmSupported =
		prLocCapa->u2LocInitSupported & BIT(1) ? TRUE : FALSE;
	prRttCapa->fgLciSupported = prLocCapa->ucLciSupport;
	prRttCapa->fgLcrSupported = prLocCapa->ucLcrSupport;
	prRttCapa->ucPreambleSupport = (uint8_t)(prLocCapa->u2PreambleSupport);
	prRttCapa->ucBwSupport = (uint8_t)(prLocCapa->u2BwSupport);
	prRttCapa->fgMcVersion = 80;

	nicCmdEventRttCapabilities(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}

void nicUniEventRtt(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_RTT);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_RTT_TAG_RTT_RESULT: {
			struct UNI_EVENT_RTT_RESULT_T *rr =
				(struct UNI_EVENT_RTT_RESULT_T *) tag;
			struct EVENT_RTT_RESULT *legacy = NULL;
			struct RTT_RESULT *lr = NULL;
			uint32_t u4Size;

			u4Size = sizeof(struct EVENT_RTT_RESULT);
			if (rr->u2IELen < 512) /* LCI, LCR */
				u4Size += rr->u2IELen;
			legacy = kalMemAlloc(u4Size, VIR_MEM_TYPE);

			if (legacy == NULL) {
				DBGLOG(NIC, WARN, "Out of memroy\n");
				return;
			}

			lr = &legacy->rResult;
			kalMemCopy(lr->aucMacAddr, rr->aucMacAddr,
				MAC_ADDR_LEN);
			lr->u4BurstNum = rr->u4BurstNum;
			lr->u4MeasurementNumber = rr->u4MeasurementNumber;
			lr->u4SuccessNumber = rr->u4SuccessNumber;
			lr->ucNumPerBurstPeer = rr->ucNumPerBurstPeer;
			lr->eStatus = rr->eStatus;
			lr->ucRetryAfterDuration = rr->ucRetryAfterDuration;
			lr->eType = rr->eType;
			lr->i4Rssi = rr->i4Rssi;
			lr->i4RssiSpread = rr->i4RssiSpread;
			kalMemCopy(&lr->rTxRate, &rr->rTxRate,
				sizeof(struct WIFI_RATE));
			kalMemCopy(&lr->rRxRate, &rr->rRxRate,
				sizeof(struct WIFI_RATE));
			lr->i8Rtt = rr->i8Rtt;
			lr->i8RttSd = rr->i8RttSd;
			lr->i8RttSpread = rr->i8RttSpread;
			lr->i4DistanceMM = rr->i4DistanceMM;
			lr->i4DistanceSdMM = rr->i4DistanceSdMM;
			lr->i4DistanceSpreadMM = rr->i4DistanceSpreadMM;
			lr->i8Ts = rr->i8Ts;
			lr->i4BurstDuration = rr->i4BurstDuration;
			lr->i4NegotiatedBustNum = rr->i4NegotiatedBustNum;

			if (rr->u2IELen < 512) { /* LCI, LCR */
				legacy->u2IELen = rr->u2IELen;
				kalMemCopy(legacy->aucIE,
					rr->aucIE, rr->u2IELen);
			} else
				legacy->u2IELen = 0;

			RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_RTT_RESULT,
				legacy, u4Size);

			kalMemFree(legacy, VIR_MEM_TYPE, u4Size);
		}
			break;
		case UNI_EVENT_RTT_TAG_RTT_DONE: {
			struct UNI_EVENT_RTT_DONE_T *rd =
				(struct UNI_EVENT_RTT_DONE_T *)tag;
			struct EVENT_RTT_DONE legacy = {0};

			legacy.ucSeqNum = rd->ucSeqNum;

			RUN_RX_EVENT_HANDLER(EVENT_ID_RTT_DONE,
				&legacy);
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

void nicUniEventNan(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
#if CFG_SUPPORT_NAN
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_NAN);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t *legacy;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		legacy = (uint8_t *)kalMemAlloc(TAG_LEN(tag), VIR_MEM_TYPE);
		if (!legacy) {
			DBGLOG(NIC, WARN, "tag=%d OOM\n", TAG_ID(tag));
			break;
		}

		kalMemCopy(legacy, tag, TAG_LEN(tag));


		RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_NAN_EXT_EVENT,
			legacy, TAG_LEN(tag));
		kalMemFree(legacy, VIR_MEM_TYPE, TAG_LEN(tag));

	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#endif
}

void nicUniCmdEventQueryMldRec(struct ADAPTER *prAdapter,
		struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_MLO *mlo_evt =
		(struct UNI_EVENT_MLO *)uni_evt->aucBuffer;
	struct UNI_EVENT_GET_MLD_REC *tag =
		(struct UNI_EVENT_GET_MLD_REC *)mlo_evt->au1TlvBuffer;
	struct PARAM_MLD_REC *prEvtMldRec = &tag->rMldRec;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	ASSERT(pucEventBuf);

	if (prCmdInfo->fgIsOid) {
		struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
		uint32_t u4QueryInfoLen = sizeof(struct PARAM_MLD_REC);

		kalMemCopy(prCmdInfo->pvInformationBuffer,
				prEvtMldRec,
				u4QueryInfoLen);

		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}

#endif
}

#if CFG_SUPPORT_TX_BF
void nicUniEventBF(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_BF);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_CMD_BF_TAG_PFMU_TAG_READ: {
			uint8_t au4RawDataTag1[7], au4RawDataTag2[7];
			struct UNI_EVENT_BF *uni_evt = (struct UNI_EVENT_BF *)
				evt->aucBuffer;
			struct UNI_EVENT_BF_PFMU_READ *tag =
				(struct UNI_EVENT_BF_PFMU_READ *)
					uni_evt->au1TlvBuffer;

			union PFMU_PROFILE_TAG1 *prPfmuTag1 =
				(union PFMU_PROFILE_TAG1 *)
					&tag->ru4TxBfPFMUTag1;
			union PFMU_PROFILE_TAG2 *prPfmuTag2 =
				(union PFMU_PROFILE_TAG2 *)
					&tag->ru4TxBfPFMUTag2;

			memcpy(au4RawDataTag1, prPfmuTag1,
				sizeof(au4RawDataTag1));
			memcpy(au4RawDataTag2, prPfmuTag2,
				sizeof(au4RawDataTag2));
#if CFG_SUPPORT_BF_CN_PRIV_CMD
			memcpy(&ad->rPfmuTag1, prPfmuTag1,
				sizeof(*prPfmuTag1));
#endif /* CFG_SUPPORT_BF_CN_PRIV_CMD */

			DBGLOG(INIT, DEBUG,
				"========================== (R)Tag1 info ==========================\n"
				" Row data0 : %x, Row data1 : %x, Row data2 : %x, Row data3 : %x\n",
				au4RawDataTag1[0], au4RawDataTag1[1],
				au4RawDataTag1[2], au4RawDataTag1[3]);
			DBGLOG(INIT, DEBUG,
				" Row data4 : %x, Row data5 : %x, Row data6 : %x\n",
				au4RawDataTag1[4], au4RawDataTag1[5],
				au4RawDataTag1[6]);
			if (prPfmuTag1->rFieldv2.ucLM == PFMU_EHT) {
				DBGLOG(INIT, DEBUG,
					"ProfileID = %d Invalid status = %d\n",
					prPfmuTag1->rFieldv2.ucProfileID,
					prPfmuTag1->rFieldv2.ucInvalidProf);
				DBGLOG(INIT, DEBUG, "0:iBF / 1:eBF = %d\n",
					prPfmuTag1->rFieldv2.ucTxBf);
				DBGLOG(INIT, DEBUG,
					"DBW(0/1/2/3 BW20/40/80/160NC) = %d\n",
					prPfmuTag1->rFieldv2.ucDBW);
				DBGLOG(INIT, DEBUG, "0:SU / 1:MU = %d\n",
					prPfmuTag1->rFieldv2.ucSU_MU);
				DBGLOG(INIT, DEBUG,
					"Nrow = %d, Ncol = %d, Ng = %d, LM = %d\n",
					prPfmuTag1->rFieldv2.ucNrow,
					prPfmuTag1->rFieldv2.ucNcol,
					prPfmuTag1->rFieldv2.ucNgroup,
					prPfmuTag1->rFieldv2.ucLM);
				DBGLOG(INIT, DEBUG,
					"ucCodeBook = %d, ucMobRuAlloc = %d\n",
					prPfmuTag1->rFieldv2.ucCodeBook,
					prPfmuTag1->rFieldv2.ucMobRuAlloc);
				DBGLOG(INIT, DEBUG,
					"Mem1(%d, %d), Mem2(%d, %d), Mem3(%d, %d), Mem4(%d, %d)\n",
					prPfmuTag1->rFieldv2.ucMemAddr1ColIdx,
					prPfmuTag1->rFieldv2.ucMemAddr1RowIdx,
					prPfmuTag1->rFieldv2.ucMemAddr2ColIdx,
					prPfmuTag1->rFieldv2.ucMemAddr2RowIdx,
					prPfmuTag1->rFieldv2.ucMemAddr3ColIdx,
					prPfmuTag1->rFieldv2.ucMemAddr3RowIdx,
					prPfmuTag1->rFieldv2.ucMemAddr4ColIdx,
					prPfmuTag1->rFieldv2.ucMemAddr4RowIdx);
				DBGLOG(INIT, DEBUG,
					"ucPartialBWInfo = 0x%x ucMobCalEn = 0x%x\n",
					prPfmuTag1->rFieldv2.ucPartialBWInfo,
					prPfmuTag1->rFieldv2.ucMobCalEn);
				DBGLOG(INIT, DEBUG,
					"SNR STS0=0x%x, SNR STS1=0x%x, SNR STS2=0x%x, SNR STS3=0x%x\n",
					prPfmuTag1->rFieldv2.ucSNR_STS0,
					prPfmuTag1->rFieldv2.ucSNR_STS1,
					prPfmuTag1->rFieldv2.ucSNR_STS2,
					prPfmuTag1->rFieldv2.ucSNR_STS3);
				DBGLOG(INIT, DEBUG,
					"SNR STS4=0x%x, SNR STS5=0x%x, SNR STS6=0x%x, SNR STS7=0x%x\n",
					prPfmuTag1->rFieldv2.ucSNR_STS4,
					prPfmuTag1->rFieldv2.ucSNR_STS5,
					prPfmuTag1->rFieldv2.ucSNR_STS6,
					prPfmuTag1->rFieldv2.ucSNR_STS7);
			} else {
				DBGLOG(INIT, DEBUG,
					"ProfileID = %d Invalid status = %d\n",
					prPfmuTag1->rFieldv3.ucProfileID,
					prPfmuTag1->rFieldv3.ucInvalidProf);
				DBGLOG(INIT, DEBUG, "0:iBF / 1:eBF = %d\n",
					prPfmuTag1->rFieldv3.ucTxBf);
				DBGLOG(INIT, DEBUG,
					"DBW(0/1/2/3 BW20/40/80/160NC) = %d\n",
					prPfmuTag1->rFieldv3.ucDBW);
				DBGLOG(INIT, DEBUG, "0:SU / 1:MU = %d\n",
					prPfmuTag1->rFieldv3.ucSU_MU);
				DBGLOG(INIT, DEBUG,
					"Nrow = %d, Ncol = %d, Ng = %d, LM = %d\n",
					prPfmuTag1->rFieldv3.ucNrow,
					prPfmuTag1->rFieldv3.ucNcol,
					prPfmuTag1->rFieldv3.ucNgroup,
					prPfmuTag1->rFieldv3.ucLM);
				DBGLOG(INIT, DEBUG,
					"ucCodeBook = %d, ucMobRuAlloc = %d\n",
					prPfmuTag1->rFieldv3.ucCodeBook,
					prPfmuTag1->rFieldv3.ucMobRuAlloc);
				DBGLOG(INIT, DEBUG,
					"Mem1(%d, %d), Mem2(%d, %d), Mem3(%d, %d), Mem4(%d, %d)\n",
					prPfmuTag1->rFieldv3.ucMemAddr1ColIdx,
					prPfmuTag1->rFieldv3.ucMemAddr1RowIdx,
					prPfmuTag1->rFieldv3.ucMemAddr2ColIdx,
					prPfmuTag1->rFieldv3.ucMemAddr2RowIdx,
					prPfmuTag1->rFieldv3.ucMemAddr3ColIdx,
					prPfmuTag1->rFieldv3.ucMemAddr3RowIdx,
					prPfmuTag1->rFieldv3.ucMemAddr4ColIdx,
					prPfmuTag1->rFieldv3.ucMemAddr4RowIdx);
				DBGLOG(INIT, DEBUG, "ucMobCalEn = 0x%x\n",
					prPfmuTag1->rFieldv3.ucMobCalEn);
				DBGLOG(INIT, DEBUG,
					"SNR STS0=0x%x, SNR STS1=0x%x, SNR STS2=0x%x, SNR STS3=0x%x\n",
					prPfmuTag1->rFieldv3.ucSNR_STS0,
					prPfmuTag1->rFieldv3.ucSNR_STS1,
					prPfmuTag1->rFieldv3.ucSNR_STS2,
					prPfmuTag1->rFieldv3.ucSNR_STS3);
				DBGLOG(INIT, DEBUG,
					"SNR STS4=0x%x, SNR STS5=0x%x, SNR STS6=0x%x, SNR STS7=0x%x\n",
					prPfmuTag1->rFieldv3.ucSNR_STS4,
					prPfmuTag1->rFieldv3.ucSNR_STS5,
					prPfmuTag1->rFieldv3.ucSNR_STS6,
					prPfmuTag1->rFieldv3.ucSNR_STS7);
			}

			DBGLOG(INIT, DEBUG,
				"========================== (R)Tag2 info ==========================\n"
				" Row data0 : %x, Row data1 : %x, Row data2 : %x\n",
				au4RawDataTag2[0], au4RawDataTag2[1],
				au4RawDataTag2[2]);
			DBGLOG(INIT, DEBUG,
				" Raw data3 : %x, Raw data4 : %x, Raw data5 : %x, Raw data6 : %x\n",
				au4RawDataTag2[3], au4RawDataTag2[4],
				au4RawDataTag2[5], au4RawDataTag2[6]);
			DBGLOG(INIT, DEBUG, "Smart Ant Cfg = %d\n",
				prPfmuTag2->rFieldv2.u2SmartAnt);
			DBGLOG(INIT, DEBUG, "SE index = %d\n",
				prPfmuTag2->rFieldv2.ucSEIdx);
			DBGLOG(INIT, DEBUG,
				"iBF lifetime limit(unit:4ms) = 0x%x\n",
				prPfmuTag2->rFieldv2.uciBfTimeOut);
			DBGLOG(INIT, DEBUG,
				"iBF desired DBW = %d\n	0/1/2/3 : BW20/40/80/160NC\n",
				prPfmuTag2->rFieldv2.uciBfDBW);
			DBGLOG(INIT, DEBUG,
				"iBF desired Ncol = %d\n  0/1/2 : Ncol = 1 ~ 3\n",
				prPfmuTag2->rFieldv2.uciBfNcol);
			DBGLOG(INIT, DEBUG,
				"iBF desired Nrow = %d\n  0/1/2/3 : Nrow = 1 ~ 4\n",
				prPfmuTag2->rFieldv2.uciBfNrow);
			DBGLOG(INIT, DEBUG,
				"iBf Ru = %d\n",
				prPfmuTag2->rFieldv2.uciBfRu);
			DBGLOG(INIT, DEBUG,
				"ucMobDeltaT = %d, ucMobLQResult = %d\n",
				prPfmuTag2->rFieldv2.ucMobDeltaT,
				prPfmuTag2->rFieldv2.ucMobLQResult);
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

#if CFG_WOW_SUPPORT
void nicUniEventWow(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_WOW);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	struct UNI_EVENT_WOW *wow;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	wow = (struct UNI_EVENT_WOW *) data;

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_WOW_TAG_WAKEUP_REASON: {
			struct UNI_EVENT_WOW_WAKEUP_REASON_INFO
				*prUniWakeUpReason =
			    (struct UNI_EVENT_WOW_WAKEUP_REASON_INFO *)tag;
			struct EVENT_WOW_WAKEUP_REASON_INFO legacy = {0};

			legacy.reason = prUniWakeUpReason->ucReason;
			legacy.u2WowWakePort = prUniWakeUpReason->u2WowWakePort;
			RUN_RX_EVENT_HANDLER(
				EVENT_ID_WOW_WAKEUP_REASON,
				&legacy
			);
		}
			break;
		case UNI_EVENT_WOW_TAG_RX_DEAUTH_REASON: {
			struct UNI_EVENT_WOW_RX_DEAUTH_REASON_INFO
				*prUniRxDeauthReason =
			    (struct UNI_EVENT_WOW_RX_DEAUTH_REASON_INFO *)tag;
			struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;

			prBssInfo = GET_BSS_INFO_BY_INDEX(ad,
				wow->ucBssIndex);

			if (!prBssInfo) {
				DBGLOG(NIC, WARN,
					"Bss=%d not found", wow->ucBssIndex);
				return;
			}

			DBGLOG(NIC, DEBUG, "Deauth Reason Code: %d\n",
				prUniRxDeauthReason->u2RxDeauthReason);

			prBssInfo->u2DeauthReason =
				      prUniRxDeauthReason->u2RxDeauthReason;

			aisFsmStateAbort(ad,
				DISCONNECT_REASON_CODE_LOCALLY,
				FALSE,
				wow->ucBssIndex);

		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

void nicUniEventCsiData(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
#if CFG_SUPPORT_CSI
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_CSI);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t *legacy;
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;

	TAG_FOR_EACH(tag, tags_len, offset) {
#if CFG_CSI_DEBUG
		DBGLOG(NIC, DEBUG, "[CSI] Tag(%d, %d)\n",
			TAG_ID(tag), TAG_LEN(tag));
#endif

		switch (TAG_ID(tag)) {
		case UNI_EVENT_CSI_TAG_DATA: {
			int32_t tag_header_len = sizeof(struct TAG_HDR);
			uint8_t *tag_buffer;
			int32_t legacy_len;

			legacy_len = tags_len - tag_header_len;
			tag_buffer = tag + tag_header_len;
			legacy = (uint8_t *) kalMemAlloc(legacy_len,
							VIR_MEM_TYPE);
			if (!legacy) {
				DBGLOG(NIC, WARN, "[CSI] tag=%d OOM\n",
					TAG_ID(tag));
				break;
			}

			kalMemCopy(legacy, tag_buffer, legacy_len);
			RUN_RX_EVENT_HANDLER_EXT(EVENT_ID_CSI_DATA,
						legacy, legacy_len);
			kalMemFree(legacy, VIR_MEM_TYPE, tags_len);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#endif
}

static void nicUniEventUevent(struct ADAPTER *prAdapter, uint8_t *pucBuf)
{
	struct EVENT_REPORT_U_EVENT *prEventData;

	prEventData = (struct EVENT_REPORT_U_EVENT *) pucBuf;
	if (prEventData != NULL) {
		DBGLOG(NIC, TRACE, "UEvent: %s\n",
		prEventData->aucData);
		kalSendUevent(prAdapter, prEventData->aucData);
	}
}

void nicUniUnsolicitStatsEvt(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint8_t *tag;
	uint16_t tags_len;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_STATISTICS);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint16_t offset = 0;
#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
	struct ML_CHNL_COND_RESULT arMlChnlCond[MLD_LINK_MAX];
	uint8_t ucMlChnlCondLinkNum = 0;
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_STATISTICS_TAG_UEVENT: {
			struct UNI_EVENT_UEVENT *tlv =
				(struct UNI_EVENT_UEVENT *) tag;
			nicUniEventUevent(ad, tlv->aucBuffer);
		}
			break;
#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
		case UNI_EVENT_STATISTICS_TAG_GET_ML_CHNL_COND_RESULT: {
			struct UNI_EVENT_GET_ML_CHNL_COND_RESULT *tlv =
				(struct UNI_EVENT_GET_ML_CHNL_COND_RESULT *)
					tag;
			kalMemCopy(&arMlChnlCond[ucMlChnlCondLinkNum],
				&tlv->rChnlCond,
				sizeof(struct ML_CHNL_COND_RESULT));
			ucMlChnlCondLinkNum++;
		}
			break;
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

#if (CFG_SUPPORT_ML_CHNL_CONDITION == 1)
	if (ucMlChnlCondLinkNum != 0)
		kalReportMlChnlCond(ad, arMlChnlCond, ucMlChnlCondLinkNum);
#endif /* CFG_SUPPORT_ML_CHNL_CONDITION */
}

#if CFG_SUPPORT_MBRAIN
void nicUniUnsolicitMbrEvt(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint8_t *tag;
	uint16_t tags_len;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_MBRAIN);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint16_t offset = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset)
	{
		switch (TAG_ID(tag)) {
			/* each user should handle tlv here.
			 * parsing tlv and transform to mbrain struct, then
			 * notify mbrain or cache in driver and notify later.
			 */
		case UNI_EVENT_MBRAAIN_TAG_TXPWR_RPT: {
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
			struct UNI_EVENT_TXPWR_MBRAIN_INFO *txpwr_mbrain_info =
				(struct UNI_EVENT_TXPWR_MBRAIN_INFO *)tag;

			mbr_wifi_txpwr_uni_event_handler(ad,
					&(txpwr_mbrain_info->rMbrRpt));
#endif
			break;
		}
		default:
			DBGLOG(NIC, WARN, "invalid tag:%u\n",
				TAG_ID(tag));
			break;
		}
	}
}
#endif

void nicUniEventSR(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_SR);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, DEBUG, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_SR_TAG_HW_IND: {
			struct UNI_EVENT_SR_HW_IND *sr =
				(struct UNI_EVENT_SR_HW_IND *) tag;
			DBGLOG(NIC, DEBUG,
				"DW_1: [u2NonSrgVldCnt = %u:u2SrgVldCnt = %u]\n",
				sr->rSrInd.u2NonSrgVldCnt,
				sr->rSrInd.u2SrgVldCnt);
			DBGLOG(NIC, DEBUG,
				"DW_2_3: [u2IntraBssPpduCnt = %u: u2InterBssPpduCnt = %u: u2NonSrgPpduVldCnt = %u: u2SrgPpduVldCnt = %u]\n",
				sr->rSrInd.u2IntraBssPpduCnt,
				sr->rSrInd.u2InterBssPpduCnt,
				sr->rSrInd.u2NonSrgPpduVldCnt,
				sr->rSrInd.u2SrgPpduVldCnt);
			DBGLOG(NIC, DEBUG,
				"DW_4_5: [u4SrAmpduMpduCnt = %u: u4SrAmpduMpduAckedCnt = %u]\n",
				sr->rSrInd.u4SrAmpduMpduCnt,
				sr->rSrInd.u4SrAmpduMpduAckedCnt);
		}
			break;
		case UNI_EVENT_SR_TAG_HW_CAP: {
			struct UNI_EVENT_SR_HW_CAP *sr =
				(struct UNI_EVENT_SR_HW_CAP *) tag;
			DBGLOG(NIC, DEBUG,
				"DW_1: [fgSrEn:%d, fgSrgEn:%d, fgNonSrgEn:%d, fgSingleMdpuRtsctsEn:%d]\n",
				sr->rSrCap.fgSrEn,
				sr->rSrCap.fgSrgEn,
				sr->rSrCap.fgNonSrgEn,
				sr->rSrCap.fgSingleMdpuRtsctsEn);
			DBGLOG(NIC, DEBUG,
				"DW_2: [fgHdrDurEn:%d, fgTxopDurEn:%d, fgNonSrgInterPpduPresv:%d, fgSrgInterPpduPresv:%d]\n",
				sr->rSrCap.fgHdrDurEn,
				sr->rSrCap.fgTxopDurEn,
				sr->rSrCap.fgNonSrgInterPpduPresv,
				sr->rSrCap.fgSrgInterPpduPresv);
			DBGLOG(NIC, DEBUG,
				"DW_3: [fgSMpduNoTrigEn:%d, fgSrgBssidOrder:%d, fgCtsAfterRts:%d, fgSrpOldRxvEn:%d]\n",
				sr->rSrCap.fgSMpduNoTrigEn,
				sr->rSrCap.fgSrgBssidOrder,
				sr->rSrCap.fgCtsAfterRts,
				sr->rSrCap.fgSrpOldRxvEn);
			DBGLOG(NIC, DEBUG,
				"DW_4: [fgSrpNewRxvEn:%d, fgSrpDataOnlyEn:%d, fgFixedRateSrREn:%d, fgWtblSrREn:%d]\n",
				sr->rSrCap.fgSrpNewRxvEn,
				sr->rSrCap.fgSrpDataOnlyEn,
				sr->rSrCap.fgFixedRateSrREn,
				sr->rSrCap.fgWtblSrREn);
			DBGLOG(NIC, DEBUG,
				"DW_5: [fgSrRemTimeEn:%d, fgProtInSrWinDis:%d, fgTxCmdDlRateSelEn:%d, fgAmpduTxCntEn:%d]\n",
				sr->rSrCap.fgSrRemTimeEn,
				sr->rSrCap.fgProtInSrWinDis,
				sr->rSrCap.fgTxCmdDlRateSelEn,
				sr->rSrCap.fgAmpduTxCntEn);
		}
			break;
		default:
			fail_cnt++;
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if (CFG_VOLT_INFO == 1)
void nicUniEventGetVnf(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_GET_VOLT_INFO);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_GET_VOLT_INFO_TAG_BASIC: {
			struct UNI_EVENT_GET_VOLT_INFO_PARAM *volt_info =
				(struct UNI_EVENT_GET_VOLT_INFO_PARAM *)tag;
			struct EVENT_GET_VOLT_INFO_T legacy;
			kalMemZero(&legacy,
				sizeof(struct EVENT_GET_VOLT_INFO_T));

			legacy.u2Volt = volt_info->u2Volt;
			DBGLOG(SW4, DEBUG, "%s volt[%d]",
				__func__, legacy.u2Volt);
			RUN_RX_EVENT_HANDLER(EVEN_ID_GET_VOLT_INFO, &legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif /* CFG_VOLT_INFO */

#if CFG_SUPPORT_WIFI_POWER_METRICS
void nicUniEventPowerMetricsStatGetInfo(struct ADAPTER *ad,
					struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_ID_POWER_METRICS);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;
	uint32_t i = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_POWER_METRICS_INFO_TAG: {
			struct UNI_EVENT_ID_POWER_METRICS_INFO *pm_info =
				(struct UNI_EVENT_ID_POWER_METRICS_INFO *)tag;
			struct EVENT_POWER_METRICS_INFO_T legacy;

			kalMemZero(&legacy,
				sizeof(struct EVENT_POWER_METRICS_INFO_T));

			legacy.u4TotalTime = pm_info->u4TotalTime;
			legacy.u4Band = pm_info->u4Band;
			legacy.u4Protocol = pm_info->u4Protocol;
			legacy.u4BandRatio.u4SleepTime =
				pm_info->u4BandRatio.u4SleepTime;
			legacy.u4BandRatio.u4RxListenTime =
				pm_info->u4BandRatio.u4RxListenTime;
			legacy.u4BandRatio.u4TxTime =
				pm_info->u4BandRatio.u4TxTime;
			legacy.u4BandRatio.u4RxTime =
				pm_info->u4BandRatio.u4RxTime;

			for (i = 0; i < ARRAY_SIZE(pm_info->u4Nss); i++)
				legacy.u4Nss[i] = pm_info->u4Nss[i];

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmCckRateStat);
				i++) {
				legacy.arStatsPmCckRateStat[i] =
					pm_info->arStatsPmCckRateStat[i];
			}

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmOfdmRateStat);
				i++) {
				legacy.arStatsPmOfdmRateStat[i] =
					pm_info->arStatsPmOfdmRateStat[i];
			}

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmHtRateStat);
				i++) {
				legacy.arStatsPmHtRateStat[i] =
					pm_info->arStatsPmHtRateStat[i];
			}

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmVhtRateStat);
				i++) {
				legacy.arStatsPmVhtRateStat[i] =
					pm_info->arStatsPmVhtRateStat[i];
			}

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmHeRateStat);
				i++) {
				legacy.arStatsPmHeRateStat[i] =
					pm_info->arStatsPmHeRateStat[i];
			}

			for (i = 0;
				i < ARRAY_SIZE(pm_info->arStatsPmEhtRateStat);
				i++) {
				legacy.arStatsPmEhtRateStat[i] =
					pm_info->arStatsPmEhtRateStat[i];
			}

			RUN_RX_EVENT_HANDLER(EVENT_ID_POWER_METRICS, &legacy);
		}
			break;
		case UNI_EVENT_ICCM_TAG: {
			struct UNI_EVENT_ID_PWR_MET_ICCM_INFO *pm_info =
				(struct UNI_EVENT_ID_PWR_MET_ICCM_INFO *)tag;

			DBGLOG(NIC, DEBUG,
				"PBCM: %d [%d:%d:%d:%d][%d:%d:%d:%d][%d:%d:%d:%d][%d:%d:%d:%d]\n",
				pm_info->u4TotalTime,
				pm_info->u4BandRatio[0].u4TxTime,
				pm_info->u4BandRatio[0].u4RxTime,
				pm_info->u4BandRatio[0].u4RxListenTime,
				pm_info->u4BandRatio[0].u4SleepTime,
				pm_info->u4BandRatio[1].u4TxTime,
				pm_info->u4BandRatio[1].u4RxTime,
				pm_info->u4BandRatio[1].u4RxListenTime,
				pm_info->u4BandRatio[1].u4SleepTime,
				pm_info->u4BandRatio[2].u4TxTime,
				pm_info->u4BandRatio[2].u4RxTime,
				pm_info->u4BandRatio[2].u4RxListenTime,
				pm_info->u4BandRatio[2].u4SleepTime,
				pm_info->u4BandRatio[3].u4TxTime,
				pm_info->u4BandRatio[3].u4RxTime,
				pm_info->u4BandRatio[3].u4RxListenTime,
				pm_info->u4BandRatio[3].u4SleepTime);

			DBGLOG(NIC, DEBUG,
				"[ICCM] Totaltime =%d Txtime =%d Rxtime =%d Rxlistentime =%d Sleeptime =%d\n",
				pm_info->u4TotalTime,
				pm_info->u4BandRatio[4].u4TxTime,
				pm_info->u4BandRatio[4].u4RxTime,
				pm_info->u4BandRatio[4].u4RxListenTime,
				pm_info->u4BandRatio[4].u4SleepTime);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif

#if CFG_SUPPORT_BAR_DELAY_INDICATION
void nicUniEventDelayBar(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_DELAY_BAR);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;
	uint8_t i;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_DELAY_BAR_INFO_TAG: {

			struct UNI_EVENT_DELAY_BAR_INFO *prDelayBarInfo =
			    (struct UNI_EVENT_DELAY_BAR_INFO *)tag;
			struct EVENT_BAR_DELAY legacy = {0};

			legacy.ucEvtVer = prDelayBarInfo->ucEvtVer;
			legacy.ucBaNum = prDelayBarInfo->ucBaNum;

			if (sizeof(struct UNI_STORED_BAR_INFO) !=
				sizeof(struct EVENT_STORED_BAR_INFO)) {
				DBGLOG(NIC, DEBUG,
					"skip due to struct size invalid.\n");
				break;
			}

			if (prDelayBarInfo->ucBaNum >
				BAR_DELAY_INDICATION_BA_MAX) {
				DBGLOG(NIC, DEBUG,
					"skip due to BaNum invalid.\n");
				break;
			}

			for (i = 0; i < prDelayBarInfo->ucBaNum; i++) {
				kalMemCopy(&(legacy.arBAR[i]),
					&(prDelayBarInfo->arBAR[i]),
					sizeof(struct UNI_STORED_BAR_INFO));

				/*
				 * Connac 3.x FW naming issue:
				 * ucStaRecIdx => wlanIdx in Connac 3.x new FW
				 * architecture
				 */
				legacy.arBAR[i].ucStaRecIdx =
					secGetStaIdxByWlanIdx(ad,
						legacy.arBAR[i].ucStaRecIdx);
			}

			RUN_RX_EVENT_HANDLER(
				EVENT_ID_DELAY_BAR,
				&legacy
			);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < UNI_EVENT_DELAY_BAR_TAG_NUM)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */

void nicUniEventFastPath(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
#if CFG_FAST_PATH_SUPPORT
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_ID_FAST_PATH);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint8_t fail_cnt = 0;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_FAST_PATH_PROCESS: {
			struct UNI_EVENT_FAST_PATH_PROCESS_T
				*prFastPathProcess =
			    (struct UNI_EVENT_FAST_PATH_PROCESS_T *)tag;
			struct EVENT_FAST_PATH legacy = {0};

			legacy.u2Mic = prFastPathProcess->u2Mic;
			legacy.ucKeynum = prFastPathProcess->ucKeynum;
			legacy.ucKeyBitmapMatchStatus =
				prFastPathProcess->u4KeybitmapMatchStatus;

			RUN_RX_EVENT_HANDLER(
				EVENT_ID_FAST_PATH,
				&legacy
			);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
#endif
}

#if CFG_SUPPORT_PKT_OFLD
void nicUniEventQueryOfldInfo(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_EVENT_PKT_OFLD *evt =
		(struct UNI_EVENT_PKT_OFLD *)uni_evt->aucBuffer;
	struct UNI_CMD_PKT_OFLD_GENERAL_OP *tag =
		(struct UNI_CMD_PKT_OFLD_GENERAL_OP *) evt->aucTlvBuffer;

	struct CMD_OFLD_INFO legacy;

	legacy.ucType = tag->ucType;
	legacy.ucOp = tag->ucOp;
	legacy.ucFragNum = tag->ucFragNum;
	legacy.ucFragSeq = tag->ucFragSeq;
	legacy.u4TotalLen = tag->u4TotalLen;
	legacy.u4BufLen = tag->u4BufLen;

	DBGLOG(REQ, DEBUG,
		"ucType[%d] ucOp[%d] ucFragNum[%d] ucFragSeq[%d] u4TotalLen[%d] u4BufLen[%d]\n",
		legacy.ucType, legacy.ucOp, legacy.ucFragNum,
		legacy.ucFragSeq, legacy.u4TotalLen, tag->u4BufLen);

	if (tag->u4TotalLen > 0 &&
			tag->u4BufLen > 0 &&
			tag->u4BufLen <= PKT_OFLD_BUF_SIZE) {
		kalMemCopy(legacy.aucBuf, tag->aucBuf, tag->u4BufLen);

	} else {
		DBGLOG(REQ, DEBUG,
			"Invalid query result, length: %d Buf size: %d.\n",
				tag->u4TotalLen, tag->u4BufLen);
	}

	nicCmdEventQueryOfldInfo(prAdapter, prCmdInfo, (uint8_t *)&legacy);
}
#endif

void nicUniEventThermalProtect(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_THERMAL);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;

	struct UNI_EVENT_THERMAL_RSP *rsp;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_THERMAL_EVENT_THERMAL_PROTECT_DUTY_UPDATE: {
			struct EXT_EVENT_THERMAL_PROTECT_DUTY_NOTIFY *info;

			rsp = (struct UNI_EVENT_THERMAL_RSP *)tag;
			info = (struct EXT_EVENT_THERMAL_PROTECT_DUTY_NOTIFY *)
					rsp->aucBuffer;

			DBGLOG(NIC, TRACE,
				"Duty update, B[%d] L[%d] D[%d] T[%d] P[%d]\n",
				info->u1BandIdx, info->u1LevelIdx,
				info->u1DutyPercent, info->i4Temp,
				info->eProtectActType);
		}
			break;
		case UNI_THERMAL_EVENT_THERMAL_PROTECT_RADIO_UPDATE: {
			struct EXT_EVENT_THERMAL_PROTECT_RADIO_NOTIFY *info;

			rsp = (struct UNI_EVENT_THERMAL_RSP *)tag;
			info = (struct EXT_EVENT_THERMAL_PROTECT_RADIO_NOTIFY *)
					rsp->aucBuffer;

			DBGLOG(NIC, TRACE,
				"Radio update, B[%d] L[%d] T[%d] P[%d]\n",
				info->u1BandIdx, info->u1LevelIdx,
				info->i4Temp, info->eProtectActType);
		}
			break;
		case UNI_THERMAL_EVENT_THERMAL_PROTECT_MECH_INFO: {
			struct UNI_EVENT_THERMAL_PROTECT_MECH_INFO *info;
			uint8_t i = 0;

			rsp = (struct UNI_EVENT_THERMAL_RSP *)tag;
			info = (struct UNI_EVENT_THERMAL_PROTECT_MECH_INFO *)
					rsp->aucBuffer;

			for (i = UNI_THERMAL_PROTECT_TYPE_NTX_CTRL;
				i < UNI_THERMAL_PROTECT_TYPE_NUM; i++) {

				DBGLOG(NIC, TRACE,
					"MechInfo[%d/%d/%d/%d/%d/%d/%d]",
					info->ucProtectionType[i],
					info->ucTriggerType[i],
					info->i4TriggerTemp[i],
					info->i4RestoreTemp[i],
					info->u2RecheckTime[i],
					info->ucState[i],
					info->fgEnable[i]);
			}

		}
			break;
		case UNI_THERMAL_EVENT_THERMAL_PROTECT_DUTY_INFO: {
			struct UNI_EVENT_THERMAL_PROTECT_DUTY_INFO *info;

			rsp = (struct UNI_EVENT_THERMAL_RSP *)tag;
			info = (struct UNI_EVENT_THERMAL_PROTECT_DUTY_INFO *)
					rsp->aucBuffer;

			DBGLOG(NIC, TRACE,
				"Duty Info, Band[%d] Duty[%d/%d/%d/%d]\n",
				info->ucBandIdx, info->ucDuty0, info->ucDuty1,
				info->ucDuty2, info->ucDuty3);

		}
			break;

		default:
			fail_cnt++;
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

void nicUniEventEfuseAccess(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_EFUSE_CONTROL *evt =
		(struct UNI_EVENT_EFUSE_CONTROL *)uni_evt->aucBuffer;

	struct UNI_EVENT_EFUSE_ACCESS *prEfuseStatus;
	struct PARAM_CUSTOM_ACCESS_EFUSE *prQueryBuffer;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen;

	prEfuseStatus = (struct UNI_EVENT_EFUSE_ACCESS *) evt->aucTlvBuffer;

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;
		prQueryBuffer = (struct PARAM_CUSTOM_ACCESS_EFUSE *)
				prCmdInfo->pvInformationBuffer;

		prQueryBuffer->u4Address = prEfuseStatus->u4Address;
		prQueryBuffer->u4Valid = prEfuseStatus->u4Valid;
		kalMemCopy(prQueryBuffer->aucData, prEfuseStatus->aucData,
			   BUFFER_ACCESS_CONTENT_MAX);

		u4QueryInfoLen = sizeof(struct UNI_EVENT_EFUSE_ACCESS);

		/* Update Query Information Length */
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, WLAN_STATUS_SUCCESS);
	}
}

void nicUniEventEfuseFreeBlock(struct ADAPTER
	  *prAdapter, struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	struct UNI_EVENT_EFUSE_CONTROL *evt =
		(struct UNI_EVENT_EFUSE_CONTROL *)uni_evt->aucBuffer;

	struct UNI_EVENT_EFUSE_FREE_BLOCK *prEfuseStatus;
	struct PARAM_CUSTOM_EFUSE_FREE_BLOCK *prQueryBuffer;
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4QueryInfoLen = 0;
	struct UNI_EVENT_EFUSE_FREE_BLOCK *prBankInfo;
	uint8_t i, efuse_bank_num, efuse_ctrl_tlv_offset;
	uint16_t total_tlv_len;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_EFUSE_CONTROL);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(uni_evt);
	uint32_t ret = WLAN_STATUS_SUCCESS;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		ret = WLAN_STATUS_FAILURE;
		goto efuse_chk_err;
	}

	/* calculate aucTlvBuffer member offset */
	efuse_ctrl_tlv_offset =
		offsetof(struct UNI_EVENT_EFUSE_CONTROL, aucTlvBuffer);

	total_tlv_len = data_len - efuse_ctrl_tlv_offset;

	/* check if total_tlv_len reasonable */
	if (total_tlv_len % sizeof(struct UNI_EVENT_EFUSE_FREE_BLOCK)) {
		DBGLOG(NIC, ERROR, "Invalid total_tlv_len:%d\n",
			total_tlv_len);
		ret = WLAN_STATUS_FAILURE;
		goto efuse_chk_err;
	}

	/* calculate total bank number based on TLV entry number */
	efuse_bank_num = total_tlv_len /
		sizeof(struct UNI_EVENT_EFUSE_FREE_BLOCK);

	DBGLOG(NIC, LOUD, "total_tlv_len %d efuse_bank_num %d\n",
			total_tlv_len, efuse_bank_num);

	if (efuse_bank_num > MAX_EFUSE_BANK_NUM) {
		DBGLOG(NIC, ERROR, "efuse_bank_num %d exceed %d\n",
			efuse_bank_num, MAX_EFUSE_BANK_NUM);
		ret = WLAN_STATUS_FAILURE;
		goto efuse_chk_err;
	}

	prEfuseStatus =
		(struct UNI_EVENT_EFUSE_FREE_BLOCK *)evt->aucTlvBuffer;

	if (prEfuseStatus == NULL) {
		DBGLOG(NIC, ERROR, "TAG error!\n");
		ret = WLAN_STATUS_FAILURE;
		goto efuse_chk_err;
	}

efuse_chk_err:
	if (ret != WLAN_STATUS_SUCCESS) {
		if (prCmdInfo->fgIsOid) {
			prGlueInfo = prAdapter->prGlueInfo;

			/* Update Query Information Length */
			kalOidComplete(prGlueInfo, prCmdInfo,
				u4QueryInfoLen, ret);
		}
		return;
	}

	if (prCmdInfo->fgIsOid) {
		prGlueInfo = prAdapter->prGlueInfo;

		prQueryBuffer = (struct PARAM_CUSTOM_EFUSE_FREE_BLOCK *)
				prCmdInfo->pvInformationBuffer;

		DBGLOG(NIC, LOUD, "query bank %d info:\n",
				prQueryBuffer->ucDieIdx);

		for (i = 0; i < efuse_bank_num; i++) {
			prBankInfo =
			(struct UNI_EVENT_EFUSE_FREE_BLOCK *)
				(prEfuseStatus + i);

			if (prBankInfo == NULL) {
				/* DO error handling */
				DBGLOG(NIC, ERROR, "query bank %d info error\n",
					prQueryBuffer->ucDieIdx);
				ret = WLAN_STATUS_FAILURE;
				break;
			}

			DBGLOG(NIC, LOUD,
				"bank %d Len %d free %d total %d ver %d\n",
				i,
				prBankInfo->u2Length,
				prBankInfo->ucGetFreeBlock,
				prBankInfo->ucTotalBlockNum,
				prBankInfo->ucVersion);

			if (i == prQueryBuffer->ucDieIdx) {
				prQueryBuffer->ucGetFreeBlock =
					prBankInfo->ucGetFreeBlock;
				prQueryBuffer->ucGetTotalBlock =
					prBankInfo->ucTotalBlockNum;
				u4QueryInfoLen =
					sizeof(struct UNI_EVENT_EFUSE_ACCESS);
				break;
			}
		}

		/* Update Query Information Length */
		kalOidComplete(prGlueInfo, prCmdInfo,
			       u4QueryInfoLen, ret);
	}
}

#if (CFG_HW_DETECT_REPORT == 1)
void nicUniEventHwDetectReport(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_HW_DETECT_REPORT);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;

	if (!ad->rWifiVar.fgHwDetectReportEn)
		return;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		switch (TAG_ID(tag)) {
		case UNI_EVENT_HW_DETECT_REPORT_BASIC: {
			struct UNI_EVENT_HW_DETECT_REPORT_PARAM
				*hw_detect_report =
				(struct UNI_EVENT_HW_DETECT_REPORT_PARAM *)tag;

			struct EVENT_HW_DETECT_REPORT legacy;

			legacy.fgIsReportNode =
				hw_detect_report->fgIsReportNode;
			legacy.aucReserved[0] =
				hw_detect_report->aucReserved[0];
			legacy.aucReserved[1] =
				hw_detect_report->aucReserved[1];
			legacy.aucReserved[2] =
				hw_detect_report->aucReserved[2];
			kalMemCopy(legacy.aucStrBuffer,
				hw_detect_report->aucStrBuffer,
				HW_DETECT_REPORT_STR_MAX_LEN);

			RUN_RX_EVENT_HANDLER(EVENT_ID_HW_DETECT_REPROT,
				&legacy);
		}
			break;
		default:
			fail_cnt++;
			ASSERT(fail_cnt < MAX_UNI_EVENT_FAIL_TAG_COUNT)
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif /* CFG_HW_DETECT_REPORT */

/*
 * \. Descrption : UNI_CMD UNI_CMD_ID_RX_HDR_TRAN
 * \. Parameters:
 *	UNI_CMD_RX_HDR_TRAN_ENABLE
 *	- enable : Enables RX header translation
 *	TRUE	Enable HW header translation
 *	FALSE	Disable HW header translation
 *	- chkBssid : Enables RX HDRT BSSID check
 *	TRUE	Only when BSSID of frames (meeting RX translation rule)
 *		matches own BSSID will RX header translation be performed.
 *	FALSE	No need to check BSSID field of frames
 *			(meeting RX translation rule)
 *	- ucTranslationMode : Selects RX header translation mode
 *	0: 802.11 to 802.3/ethernet
 *	1: 802.11 QoS data to 802.11 non-QoS data.
 *	The translation will remove QoS control field and HT control field.
 *
 *	UNI_CMD_RX_HDR_TRAN_VLAN
 *	- insVlan : RX inserts VLAN
 *	TRUE	RX header translation will insert new VLAN field
 *	FALSE	RX header translation will NOT insert new VLAN field
 *	- rmVlan : RX removes VLAN
 *	TRUE	RX header translation will remove original VLAN field
 *	FALSE	RX header translation does NOT remove original VLAN field
 *	- fgUseQosTid : Selects RX PCP
 *	TRUE: RX header translation uses QoS_TID as VLAN TCI.PCP
 *	FALSE: RX header translation uses firmware assigned VLAN TCI.PCP
 *
 *	UNI_CMD_RX_HDR_TRAN_BLOCKLIST_CONFIG : TBD
 *	- listCnt : max ~7
 *	- list : Ether-type blocklist for RX header translation
 *	- en : en/disable for list
 */
uint32_t nicUniCmdRxHdrTransUpdate(struct ADAPTER *ad,
	struct UNI_CMD_RX_HDR_TRAN_PARM *param)
{
	struct UNI_CMD_RX_HDR_TRAN *uni_cmd = NULL;
	struct UNI_CMD_RX_HDR_TRAN_ENABLE *rx_hdr_tran_enable = NULL;
	struct UNI_CMD_RX_HDR_TRAN_VLAN *rx_hdr_tran_vlan = NULL;
	uint32_t status = 0;
	uint32_t max_cmd_len = 0;
	uint8_t *pos = NULL;

	max_cmd_len = sizeof(*uni_cmd) + sizeof(*rx_hdr_tran_enable)
		+ sizeof(*rx_hdr_tran_vlan);

	pos = cnmMemAlloc(ad, RAM_TYPE_MSG, max_cmd_len);

	if (!pos)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_RX_HDR_TRAN *) pos;

	DBGLOG(NIC, DEBUG,
		"En %u chkBssid %u TransMode %u insVlan %u rmVlan %u UseTid %u\n",
		param->fgEnable, param->fgCheckBssid, param->ucTranslationMode,
		param->fgInsertVlan, param->fgRemoveVlan, param->fgUseQosTid);

	/* 1. cmd header setting : empty header */
	pos += sizeof(*uni_cmd);
	/* 2. set RX header translation ENABLE/DISABLE */
	rx_hdr_tran_enable = (struct UNI_CMD_RX_HDR_TRAN_ENABLE *) pos;
	rx_hdr_tran_enable->u2Tag = UNI_CMD_RX_HDR_TRAN_ENABLE;
	rx_hdr_tran_enable->u2Length =
		sizeof(struct UNI_CMD_RX_HDR_TRAN_ENABLE);
	rx_hdr_tran_enable->fgEnable = param->fgEnable;
	rx_hdr_tran_enable->fgCheckBssid = param->fgCheckBssid;
	rx_hdr_tran_enable->ucTranslationMode = param->ucTranslationMode;
	pos += sizeof(*rx_hdr_tran_enable);
	/* 3. set RX header translation - VLAN */
	rx_hdr_tran_vlan = (struct UNI_CMD_RX_HDR_TRAN_VLAN *) pos;
	rx_hdr_tran_vlan->u2Tag = UNI_CMD_RX_HDR_TRAN_VLAN_CONFIG;
	rx_hdr_tran_vlan->u2Length = sizeof(struct UNI_CMD_RX_HDR_TRAN_VLAN);
	rx_hdr_tran_vlan->fgInsertVlan = param->fgInsertVlan;
	rx_hdr_tran_vlan->fgRemoveVlan = param->fgRemoveVlan;
	rx_hdr_tran_vlan->fgUseQosTid = param->fgUseQosTid;
	pos += sizeof(*rx_hdr_tran_vlan);

	/* 4. TODO: set Ether type block list */

	/* 5. Send uni cmd */
	status = wlanSendSetQueryUniCmd(ad,
			     UNI_CMD_ID_RX_HDR_TRAN,
			     TRUE,
			     TRUE,
			     FALSE,
			     nicUniCmdEventSetCommon,
			     nicUniCmdTimeoutCommon,
			     max_cmd_len,
			     (void *)uni_cmd, NULL, 0);

	cnmMemFree(ad, uni_cmd);

	return status;
}

void nicUniCmdEventLpDbgCtrl(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt = (struct WIFI_UNI_EVENT *) pucEventBuf;
	struct UNI_CMD_LP_DBG_CTRL *uni_cmd =
		(struct UNI_CMD_LP_DBG_CTRL *) GET_UNI_CMD_DATA(prCmdInfo);
	struct UNI_EVENT_LP_DBG_CTRL *evt =
		(struct UNI_EVENT_LP_DBG_CTRL *) uni_evt->aucBuffer;

	if (TAG_ID(uni_cmd->aucTlvBuffer) ==
		UNI_CMD_LP_DBG_CTRL_TAG_GET_SLP_CNT_INFO) {
		struct UNI_EVENT_LP_GET_SLP_CNT_INFO *tag =
			(struct UNI_EVENT_LP_GET_SLP_CNT_INFO *)
			evt->aucTlvBuffer;
		struct PARAM_SLEEP_CNT_INFO legacy = {0};

		legacy.au4LmacSlpCnt[ENUM_BAND_0] =
			tag->au4LmacSlpCnt[ENUM_BAND_0];
		legacy.au4LmacSlpCnt[ENUM_BAND_1] =
			tag->au4LmacSlpCnt[ENUM_BAND_1];
		legacy.u4WfsysSlpCnt = tag->u4WfsysSlpCnt;
		legacy.u4CbinfraSlpCnt = tag->u4CbinfraSlpCnt;
		legacy.u4ChipSlpCnt = tag->u4ChipSlpCnt;

		nicCmdEventGetSlpCntInfo(prAdapter, prCmdInfo,
			(uint8_t *)&legacy);
	} else if (TAG_ID(uni_cmd->aucTlvBuffer) ==
		UNI_CMD_LP_DBG_CTRL_TAG_KEEP_PWR_CTRL) {
		struct UNI_EVENT_LP_KEEP_PWR_CTRL *tag =
			(struct UNI_EVENT_LP_KEEP_PWR_CTRL *) evt->aucTlvBuffer;
		struct CMD_LP_DBG_CTRL legacy = {0};

		legacy.ucKeepPwr = tag->ucKeepPwr;
		legacy.ucRfdigStatus = tag->ucStatus;

		nicCmdEventLpKeepPwrCtrl(prAdapter, prCmdInfo,
			(uint8_t *)&legacy);
	}
}

#if CFG_SUPPORT_FW_DROP_SSN
void nicUniEventFwDropSSN(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_THERMAL);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	uint32_t fail_cnt = 0;
	uint32_t i;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_FW_DROP_SSN_INFO_TAG: {
			struct UNI_EVENT_FW_DROP_SSN_INFO *info =
				(struct UNI_EVENT_FW_DROP_SSN_INFO *)tag;

			if (info->ucDrpPktNum > FW_DROP_SSN_MAX) {
				DBGLOG(NIC, WARN,
					"skip invalid ucDrpPktNum:%u\n",
					info->ucDrpPktNum);
				break;
			}

			for (i = 0; i < info->ucDrpPktNum; i++)
				nicEventHandleFwDropSSN(ad,
					&(info->arSSN[i]));
		}
			break;
		default:
			fail_cnt++;
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#endif /* CFG_SUPPORT_FW_DROP_SSN */

#if CFG_WOW_SUPPORT
#if CFG_SUPPORT_MDNS_OFFLOAD
void nicUniEventMdnsStats(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	struct UNI_EVENT_MDNS_RECORD *mdns_record;

	mdns_record = (struct UNI_EVENT_MDNS_RECORD *)data;
	RUN_RX_EVENT_HANDLER(EVENT_ID_MDNS_RECORD, mdns_record);
}

#endif /* CFG_SUPPORT_MDNS_OFFLOAD */
#endif

void nicUniEventUpdateLp(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_UPDATE_LP);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
#if (CFG_PCIE_GEN_SWITCH == 1)
	struct GL_HIF_INFO *prHifInfo = NULL;
#endif /* CFG_PCIE_GEN_SWITCH */

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_UPDATE_LP_TX_DELAY: {
#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
			struct UNI_EVENT_UPDATE_LP_TX_DELAY_T *info =
				(struct UNI_EVENT_UPDATE_LP_TX_DELAY_T *) tag;
			struct WIFI_VAR *prWifiVar = &ad->rWifiVar;

			DBGLOG(NIC, DEBUG,
				"Set TxDelay, Scen[%d] TRx[%u,%u] Delay[%u] Pkt[%u]\n",
				info->i4Scen, info->u4Tx, info->u4Rx,
				info->ucDelay, info->u4PktCnt);

			prWifiVar->u4TxDataDelayTimeout = info->ucDelay;
			prWifiVar->u4TxDataDelayCnt = info->u4PktCnt;
#else
			DBGLOG(NIC, WARN, "not support Tx delay.\n");
#endif
		}
			break;
		case UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP: {
#if defined(CFG_SUPPORT_TWT_EXT) && (CFG_SUPPORT_TWT_EXT == 1)
			struct UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP_T *info =
			(struct UNI_EVENT_UPDATE_LP_DELAYED_WAKEUP_T *) tag;

			/* dump event content */
			DBGLOG(REQ, INFO,
				"[DW]: idx[%u] wake[%u] cnt[%u] len[%u]\n",
				info->ucBssIndex, info->ucWakeupId,
				info->ucPktCnt, info->ucDataLen);

			/* Send vendor event to upper layer */
			DelayedWakeupEventNotify(ad, info->ucBssIndex,
				info->ucWakeupId, info->ucPktCnt,
				info->ucDataLen, info->aucPktInfo);
#endif /* CFG_SUPPORT_TWT_EXT */
		}
			break;
		case UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT: {
#ifdef CFG_SUPPORT_TWT_EXT
			struct UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT_T *info =
			(struct UNI_EVENT_UPDATE_LP_LEAKY_AP_DETECT_T *) tag;
			LeakyApEvent(ad, info->ucDectionType);
#endif
		}
			break;
#if (CFG_PCIE_GEN_SWITCH == 1)
		case UNI_EVENT_UPDATE_LP_GEN_SWITCH: {
			struct UNI_EVENT_UPDATE_LP_GEN_SWITCH_T *info =
				(struct UNI_EVENT_UPDATE_LP_GEN_SWITCH_T *) tag;

			DBGLOG(NIC, DEBUG,
				"[Gen Switch] event status [%d]\n",
					info->ucGenSwitchStatus);
			g_ucReceiveGenSwitch = FALSE;
			g_ucBypassException = FALSE;

			prHifInfo = &ad->prGlueInfo->rHifInfo;

			if (prHifInfo->rErrRecoveryCtl.eErrRecovState !=
					   ERR_RECOV_STOP_IDLE) {
				DBGLOG(INIT, ERROR, "SER on-going\n");
				wlandioStopPcieStatus(ad,
					PCIE_MD_REJECT_GEN_SWITCH);
			} else {
#if CFG_MTK_MDDP_SUPPORT
				mddpNotifyMDGenSwitchStart(ad);
#else
				wlandioStopPcieStatus(ad,
					PCIE_STOP_TRANSITION_END);
#endif
			}
		}
			break;
#endif

		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}

#if (CFG_SUPPORT_FACT_CAL == 1)
void nicUniEventGetFactCalData(struct ADAPTER *prAdapter,
	struct WIFI_UNI_EVENT *uni_evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	struct UNI_EVENT_FACT_CAL_RAPID_GET_DATA *prFactCalGetData = NULL;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_FACT_CAL);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(uni_evt);
	uint8_t *data = GET_UNI_EVENT_DATA(uni_evt);

	DBGLOG(NIC, DEBUG, "EVENT_ID_ONE_TIME_CAL\n");

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;

	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_FACT_CAL_RAPID_GET_DATA_TAG: {
			prFactCalGetData =
			(struct UNI_EVENT_FACT_CAL_RAPID_GET_DATA *) tag;

			if (prFactCalGetData->u4SeqNum == 0) {
				DBGLOG(NIC, DEBUG,
				"FACT_CAL_STORE_DATA_HEAD: done %d, calType %d, Control buf Addr=0x%p, bufNum=%d\n",
				prFactCalGetData->ucDone,
				prFactCalGetData->ucCalType,
				prFactCalGetData->aucBufData,
				prFactCalGetData->u4BufDataLength/16
				);
				DBGLOG_MEM8(RLM, LOUD,
				prFactCalGetData->aucBufData,
				prFactCalGetData->u4BufDataLength);
				rlmFactCalUpdateStruct(
				prAdapter, FACT_CAL_STORE_DATA_HEAD,
				prFactCalGetData);
			} else {
				DBGLOG(NIC, DEBUG,
				"FACT_CAL_STORE_DATA: done %d, calType %d, bufData=%p, bufDataLength %d, seqNum = %d\n",
				prFactCalGetData->ucDone,
				prFactCalGetData->ucCalType,
				prFactCalGetData->aucBufData,
				prFactCalGetData->u4BufDataLength,
				prFactCalGetData->u4SeqNum);
				DBGLOG_MEM8(RLM, LOUD,
				prFactCalGetData->aucBufData,
				prFactCalGetData->u4BufDataLength);
				rlmFactCalUpdateStruct(prAdapter,
				prFactCalGetData->ucDone ==
				TRUE ? FACT_CAL_STORE_DATA_DONE :
				FACT_CAL_STORE_DATA,
				prFactCalGetData);
			}
			DBGLOG(NIC, DEBUG,
			"done %d, calType %d, bufDataLength %d, seqNum = %d\n",
			prFactCalGetData->ucDone,
			prFactCalGetData->ucCalType,
			prFactCalGetData->u4BufDataLength,
			prFactCalGetData->u4SeqNum);
			break;
		}
		default:
			break;
		}
	}
}
#endif //CFG_SUPPORT_FACT_CAL

#if CFG_MTK_MDDP_SUPPORT
void nicUniEventMddp(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_MDDP);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);
	static char aucMddpRsn[MDDP_EXP_RSN_SIZE];

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_MDDP_EXCEPTION: {
			struct UNI_EVENT_MDDP_EXCEPTION *exp =
				(struct UNI_EVENT_MDDP_EXCEPTION *) tag;

			kalScnprintf(aucMddpRsn,
				     MDDP_EXP_RSN_SIZE,
				     MDDP_EXP_RST_STR,
				     exp->u4ExceptionIdx);
			DBGLOG(NIC, DEBUG,
			       "mddp execption tag[%u] len[%u] idx[%u]\n",
			       exp->u2Tag,
			       exp->u2Length,
			       exp->u4ExceptionIdx);
#if CFG_WMT_RESET_API_SUPPORT
			glSetRstReasonString(aucMddpRsn);
			glResetWholeChipResetTrigger(aucMddpRsn);
#endif /* CFG_WMT_RESET_API_SUPPORT */
		}
			break;
		case UNI_EVENT_MDDP_FWOWN_RETRY: {
			struct UNI_EVENT_MDDP_FWOWN_RETRY *retry =
				(struct UNI_EVENT_MDDP_FWOWN_RETRY *) tag;

			DBGLOG(NIC, DEBUG,
			       "mddp retry tag[%u] len[%u] retry[%u]\n",
			       retry->u2Tag,
			       retry->u2Length,
			       retry->u4RetryCnt);

			mddpTriggerMdFwOwnByFw(ad);
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
}
#endif /* CFG_MTK_MDDP_SUPPORT */

void nicUniEventTxPower(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_TXPOWER);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
			case UNI_EVENT_TXPOWER_POWER_LIMIT_EMI_STATUS: {
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
				rlmDomainPowerLimitEmiEvent(ad, TAG_DATA(tag));
#endif
			}
				break;
			default: {
				DBGLOG(NIC, WARN, "invalid tag = %d\n",
					TAG_ID(tag));
			}
				break;
		}
	}

}

#if (CFG_SUPPORT_802_11AX == 1)
static void nicEventHandleOmi(struct ADAPTER *prAdapter,
	struct UNI_EVENT_NOTIFY_OMI_RX_T *prOmiEvent)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	uint16_t u2HeRxMcsMapAssoc;
	uint8_t ucMaxBwAllowed;
	uint8_t ucStaRecIdx;
	uint8_t ucMacRxNss;

	if (!prAdapter || !prOmiEvent)
		return;

	ucStaRecIdx =
		secGetStaIdxByWlanIdx(prAdapter, prOmiEvent->u2StaRecIndex);
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(NIC, WARN, "Can not find prStaRec\n");
		return;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(NIC, WARN, "Can not find prBssInfo\n");
		return;
	}

	u2HeRxMcsMapAssoc = prStaRec->u2HeRxMcsMapBW80Assoc;
	ucMaxBwAllowed =
		cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);

	if (prOmiEvent->ucRxNss == VHT_OP_MODE_NSS_2) {
		/* CFG_SUPPORT_802_11AX */
		prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
			(~(VHT_CAP_INFO_MCS_1SS_MASK |
			VHT_CAP_INFO_MCS_2SS_MASK));

		prStaRec->u2VhtRxMcsMap |=
			(prStaRec->u2VhtRxMcsMapAssoc &
			(VHT_CAP_INFO_MCS_1SS_MASK |
			VHT_CAP_INFO_MCS_2SS_MASK));

		prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
			(~(HE_CAP_INFO_MCS_1SS_MASK |
			HE_CAP_INFO_MCS_2SS_MASK));

		prStaRec->u2HeRxMcsMapBW80 |=
			(u2HeRxMcsMapAssoc &
			(HE_CAP_INFO_MCS_1SS_MASK |
			HE_CAP_INFO_MCS_2SS_MASK));

		if (ucMaxBwAllowed >= MAX_BW_160MHZ)
			u2HeRxMcsMapAssoc = prStaRec->u2HeRxMcsMapBW160Assoc;

		if (ucMaxBwAllowed >= MAX_BW_160MHZ) {
			prStaRec->u2HeRxMcsMapBW160 =
				BITS(0, 15) &
				(~(HE_CAP_INFO_MCS_1SS_MASK |
				HE_CAP_INFO_MCS_2SS_MASK));

			prStaRec->u2HeRxMcsMapBW160 |=
				(u2HeRxMcsMapAssoc &
				(HE_CAP_INFO_MCS_1SS_MASK |
				HE_CAP_INFO_MCS_2SS_MASK));
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		prStaRec->u2He6gBandCapInfo |=
			HE_6G_CAP_INFO_SM_POWER_SAVE;
#endif

	} else {
		/* CFG_SUPPORT_802_11AX */
		prStaRec->u2VhtRxMcsMap = BITS(0, 15) &
			(~VHT_CAP_INFO_MCS_1SS_MASK);

		prStaRec->u2VhtRxMcsMap |=
			(prStaRec->u2VhtRxMcsMapAssoc &
			VHT_CAP_INFO_MCS_1SS_MASK);

		prStaRec->u2HeRxMcsMapBW80 = BITS(0, 15) &
			(~HE_CAP_INFO_MCS_1SS_MASK);

		prStaRec->u2HeRxMcsMapBW80 |=
			(u2HeRxMcsMapAssoc &
			HE_CAP_INFO_MCS_1SS_MASK);

		if (ucMaxBwAllowed >= MAX_BW_160MHZ)
			u2HeRxMcsMapAssoc = prStaRec->u2HeRxMcsMapBW160Assoc;

		if (ucMaxBwAllowed >= MAX_BW_160MHZ) {
			prStaRec->u2HeRxMcsMapBW160 =
				BITS(0, 15) &
				(~HE_CAP_INFO_MCS_1SS_MASK);

			prStaRec->u2HeRxMcsMapBW160 |=
				(u2HeRxMcsMapAssoc &
				HE_CAP_INFO_MCS_1SS_MASK);
		}

#if (CFG_SUPPORT_WIFI_6G == 1)
		prStaRec->u2He6gBandCapInfo &=
			~HE_6G_CAP_INFO_SM_POWER_SAVE;
#endif
	}

	DBGLOG(NIC, STATE,
		"RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
		prStaRec->u2VhtRxMcsMap,
		prStaRec->u2VhtRxMcsMapAssoc);
	DBGLOG(NIC, STATE,
		"HeBw80 RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
		prStaRec->u2HeRxMcsMapBW80,
		prStaRec->u2HeRxMcsMapBW80Assoc);
	DBGLOG(NIC, STATE,
		", HeBW160 RxMcsMap:0x%x, McsMapAssoc:0x%x\n",
		prStaRec->u2HeRxMcsMapBW160,
		prStaRec->u2HeRxMcsMapBW160Assoc);
#if (CFG_SUPPORT_WIFI_6G == 1)
	DBGLOG(NIC, STATE,
		"He6gBandCapInfo:0x%x\n",
		prStaRec->u2He6gBandCapInfo);
#endif

	ucMacRxNss = prOmiEvent->ucRxNss + 1;
	if (ucMacRxNss > wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex))
		ucMacRxNss =
			wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);

#if (CFG_SUPPORT_802_11BE == 1)
	if (prOmiEvent->ucBWExt > 0 && prOmiEvent->ucBW == 0) {
		STAREC_SET_EHT_RX_320MHZ_MCS0_9_NSS(prStaRec, ucMacRxNss);
		STAREC_SET_EHT_RX_320MHZ_MCS10_11_NSS(prStaRec, ucMacRxNss);
		STAREC_SET_EHT_RX_320MHZ_MCS12_13_NSS(prStaRec, ucMacRxNss);
	}
	DBGLOG(NIC, STATE,
		"McsMap320MHz[0]: %u, McsMap320MHz[1]: %u, McsMap320MHz[2]: %u\n",
		prStaRec->aucMcsMap320MHz[0],
		prStaRec->aucMcsMap320MHz[1],
		prStaRec->aucMcsMap320MHz[2]);

	if (prOmiEvent->ucBW >= VHT_OP_MODE_CHANNEL_WIDTH_160_80P80
		|| prOmiEvent->ucBWExt > 0) {
		STAREC_SET_EHT_RX_160MHZ_MCS0_9_NSS(prStaRec, ucMacRxNss);
		STAREC_SET_EHT_RX_160MHZ_MCS10_11_NSS(prStaRec, ucMacRxNss);
		STAREC_SET_EHT_RX_160MHZ_MCS12_13_NSS(prStaRec, ucMacRxNss);
	}

	DBGLOG(NIC, STATE,
		"McsMap160MHz[0]: %u, McsMap160MHz[1]: %u, McsMap160MHz[2]: %u\n",
		prStaRec->aucMcsMap160MHz[0],
		prStaRec->aucMcsMap160MHz[1],
		prStaRec->aucMcsMap160MHz[2]);
#endif /* CFG_SUPPORT_802_11BE */

	cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
	cnmDumpStaRec(prAdapter, prStaRec->ucIndex);
}
#endif

#if (CFG_SUPPORT_802_11AX == 1)
void nicUniEventOmi(struct ADAPTER *ad, struct WIFI_UNI_EVENT *evt)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_UPDATE_OMI);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(evt);
	uint8_t *data = GET_UNI_EVENT_DATA(evt);

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_NOTIFY_OMI_RX: {
			struct UNI_EVENT_NOTIFY_OMI_RX_T *omi_rx =
				(struct UNI_EVENT_NOTIFY_OMI_RX_T *) tag;
			DBGLOG(NIC, STATE,
				"omi_rx tag[%u] len[%u] Idx[%u] Rx[%u] Tx[%u] Bw[%u]\n",
				omi_rx->u2Tag, omi_rx->u2Length,
				omi_rx->u2StaRecIndex, omi_rx->ucRxNss,
				omi_rx->ucTxNsts, omi_rx->ucBW);
#if (CFG_SUPPORT_802_11BE == 1)
			DBGLOG(NIC, STATE,
				"RxNssExt[%u] BWExt[%u] TxNstsExt[%u]\n",
				omi_rx->ucRxNssExt, omi_rx->ucBWExt,
				omi_rx->ucTxNstsExt);
#endif
			nicEventHandleOmi(ad, omi_rx);
		}
			break;
		default:
			DBGLOG(NIC, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}
}
#endif

#if CFG_SUPPORT_FIPS
void nicUniEventFips(struct ADAPTER *prAdapter, struct CMD_INFO *prCmdInfo,
		     uint8_t *pucEventBuf)
{
	uint16_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint16_t fixed_len = sizeof(struct UNI_EVENT_ID_FIPS);
	uint16_t data_len = GET_UNI_EVENT_DATA_LEN(pucEventBuf);
	uint8_t *data = GET_UNI_EVENT_DATA(pucEventBuf);
	struct FIPS_PARAM *prFipsParam =
		(struct FIPS_PARAM *)prCmdInfo->pvInformationBuffer;

	/* underflow check */
	if (data_len < fixed_len) {
		DBGLOG(NIC, ERROR, "Invalid event data length:%d\n",
			data_len);
		return;
	}

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(CNM, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
		case UNI_EVENT_FIPS_STATUS: {
			struct UNI_EVENT_FIPS_STATUS *status =
				(struct UNI_EVENT_FIPS_STATUS *) tag;

			DBGLOG(NIC, INFO, "status:%u, fragTotal:%u\n",
			       status->ucStatus, status->ucFragTotal);
			prFipsParam->ucStatus = status->ucStatus;
			prFipsParam->ucFragTotal = status->ucFragTotal;
		}
			break;
		case UNI_EVENT_FIPS_RESULT: {
			struct UNI_EVENT_FIPS_RESULT *res =
				(struct UNI_EVENT_FIPS_RESULT *) tag;

			DBGLOG(NIC, INFO,
			       "fragTotal:%u, u2TRTotalLen:%u, u2TRBufferLen:%u\n",
			       res->ucFragTotal, res->u2TRTotalLen,
			       res->u2TRBufferLen);
			prFipsParam->ucFragTotal = res->ucFragTotal;
			prFipsParam->u2TRTotalLen = res->u2TRTotalLen;
			prFipsParam->u2TRBufferLen = res->u2TRBufferLen;
			kalMemCopy(prFipsParam->aucTRBuffer, res->aucTRBuffer,
				   res->u2TRBufferLen);
			DBGLOG(NIC, INFO, "dump result\n");
			DBGLOG_MEM8(NIC, INFO, res->aucTRBuffer,
				    res->u2TRBufferLen);
		}
			break;
		default:
			DBGLOG(CNM, WARN, "invalid tag = %d\n", TAG_ID(tag));
			break;
		}
	}

	if (tags_len != offset)
		DBGLOG(NIC, ERROR, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

	if (prCmdInfo->fgIsOid) {
		kalOidComplete(prAdapter->prGlueInfo, prCmdInfo,
			       sizeof(struct FIPS_PARAM), WLAN_STATUS_SUCCESS);
	}
}
#endif /* CFG_SUPPORT_FIPS */

#if (CFG_PCIE_GEN_SWITCH == 1)
uint32_t nicUniCmdUpdateLowPowerParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_UPDATA_LP_PARAM *cmd;
	struct UNI_CMD_UPDATE_LP *uni_cmd;
	struct UNI_CMD_UPDATE_LP_GEN_SWITCH_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_UPDATE_LP) +
		sizeof(struct UNI_CMD_UPDATE_LP_GEN_SWITCH_PARAM);

	if (info->ucCID != CMD_ID_UPDATE_LP ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_UPDATA_LP_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_UPDATE_LP,
		max_cmd_len, NULL, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_UPDATE_LP *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_UPDATE_LP_GEN_SWITCH_PARAM *)
		uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_UPDATE_LP_TAG_GEN_SWITCH_PARAM;
	tag->u2Length = sizeof(*tag);
	tag->ucPcieTransitionStatus = cmd->ucPcieTransitionStatus;
	DBGLOG(NIC, WARN, "[Gen_Switch] cmd pcie status = %d\n",
		cmd->ucPcieTransitionStatus);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}
#endif

#if (CFG_EAP_PCIE_GEN_SWITCH == 1)
uint32_t nicUniCmdUpdatePcieParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_UPDATE_PCIE_PARAM *cmd;
	struct UNI_CMD_UPDATE_PCIE *uni_cmd;
	struct UNI_CMD_UPDATE_PCIE_PARAM *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_UPDATE_PCIE) +
		sizeof(struct UNI_CMD_UPDATE_PCIE_PARAM);

	if (info->ucCID != CMD_ID_UPDATE_PCIE ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_UPDATE_PCIE_PARAM *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_UPDATE_PCIE,
		max_cmd_len, info->pfCmdDoneHandler, info->pfCmdTimeoutHandler);

	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_UPDATE_PCIE *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_UPDATE_PCIE_PARAM *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_UPDATE_LP_TAG_PCIE_PARAM;
	tag->u2Length = sizeof(*tag);

	tag->ucGen = cmd->ucGen;
	tag->ucLane = cmd->ucLane;
	DBGLOG(NIC, STATE, "[GEN_SWITCH] ucGen = %d ucLane =%d\n",
		cmd->ucGen, cmd->ucLane);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_EAP_PCIE_GEN_SWITCH */

#if (CFG_SUPPORT_TSF_SYNC == 1)
void nicUniCmdEventTsfSyncDone(struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo, uint8_t *pucEventBuf)
{
	struct WIFI_UNI_EVENT *uni_evt;
	struct UNI_EVENT_MAC_IFNO *evt;
	struct UNI_EVENT_MAC_INFO_TSF_SYNC *tag;
	struct CMD_TSF_SYNC legacy = {0};

	uni_evt = (struct WIFI_UNI_EVENT *)pucEventBuf;
	if (!uni_evt)
		return;
	evt = (struct UNI_EVENT_MAC_IFNO *)uni_evt->aucBuffer;
	if (!evt)
		return;
	tag = (struct UNI_EVENT_MAC_INFO_TSF_SYNC *) evt->aucTlvBuffer;
	if (!tag)
		return;

	if (tag->u2Tag != UNI_EVENT_MAC_INFO_TAG_TSF_SYNC ||
	    tag->u2Length != sizeof(struct UNI_EVENT_MAC_INFO_TSF_SYNC))
		return;

	legacy.u8TsfValue = tag->u8TsfValue;
	legacy.ucBssIndex = tag->ucBssIndex;

	nicCmdEventLatchTSF(prAdapter, prCmdInfo, (uint8_t *)(&legacy));
}

uint32_t nicUniCmdUpdateTsfSyncParam(struct ADAPTER *ad,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct CMD_TSF_SYNC *cmd;
	struct UNI_CMD_GET_MAC_INFO *uni_cmd;
	struct UNI_CMD_MAC_INFO_TSF_SYNC *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint32_t max_cmd_len = sizeof(struct UNI_CMD_GET_MAC_INFO) +
				sizeof(struct UNI_CMD_MAC_INFO_TSF_SYNC);

	if (info == NULL ||
	    info->ucCID != CMD_ID_BEACON_TSF_SYNC ||
	    info->u4SetQueryInfoLen != sizeof(*cmd))
		return WLAN_STATUS_NOT_ACCEPTED;

	cmd = (struct CMD_TSF_SYNC *) info->pucInfoBuffer;
	entry = nicUniCmdAllocEntry(ad, UNI_CMD_ID_GET_MAC_INFO, max_cmd_len,
			nicUniCmdEventTsfSyncDone, NULL);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_GET_MAC_INFO *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_MAC_INFO_TSF_SYNC *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_MAC_INFO_TAG_TSF_SYNC;
	tag->u2Length = sizeof(*tag);
	tag->fgIsLatch = cmd->fgIsLatch;
	tag->ucBssIndex = cmd->ucBssIndex;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	return WLAN_STATUS_SUCCESS;
}

#endif

#if (CFG_SUPPORT_WF_DUMP_BT_COREDUMP == 1)
uint32_t nicUniCmdBtCtrl(struct ADAPTER *prAdapter,
			    struct WIFI_UNI_SETQUERY_INFO *prInfo)
{
	struct EXT_CMD_BT_CTRL *prLegacyCmd;
	struct UNI_CMD_BT_CTRL *prUniCmd;
	struct UNI_CMD_BT_CTRL_HEAD *prTag;
	struct WIFI_UNI_CMD_ENTRY *prEntry;
	uint32_t u4MaxSize = sizeof(struct UNI_CMD_BT_CTRL) +
		sizeof(struct UNI_CMD_BT_CTRL_HEAD);
	uint16_t u2TagId;

	if (!prInfo || !prInfo->pucInfoBuffer)
		return WLAN_STATUS_INVALID_DATA;

	if (prInfo->ucCID != CMD_ID_LAYER_0_EXT_MAGIC_NUM ||
		prInfo->ucExtCID != EXT_CMD_ID_BT_CTRL)
		return WLAN_STATUS_NOT_ACCEPTED;

	prLegacyCmd = (struct EXT_CMD_BT_CTRL *) prInfo->pucInfoBuffer;

	if (prLegacyCmd->ucAction == CMD_BT_CTRL_GET_COREDUMP_HEADER)
		u2TagId = UNI_CMD_BT_CTRL_HEAD;
	else if (prLegacyCmd->ucAction == CMD_BT_CTRL_GET_COREDUMP_DATA)
		u2TagId = UNI_CMD_BT_CTRL_DATA;
	else
		return WLAN_STATUS_NOT_ACCEPTED;

	prEntry = nicUniCmdAllocEntry(prAdapter, UNI_CMD_ID_BT_CTRL,
			u4MaxSize, prInfo->pfCmdDoneHandler,
			prInfo->pfCmdTimeoutHandler);

	if (!prEntry)
		return WLAN_STATUS_RESOURCES;

	prUniCmd = (struct UNI_CMD_BT_CTRL *) prEntry->pucInfoBuffer;
	/* UNI_CMD_BT_CTRL_HEAD struct == UNI_CMD_BT_CTRL_DATA struct */
	prTag = (struct UNI_CMD_BT_CTRL_HEAD *) prUniCmd->aucTlvBuffer;
	prTag->u2Tag = u2TagId;
	prTag->u2Length = sizeof(*prTag);

	LINK_INSERT_TAIL(&prInfo->rUniCmdList, &prEntry->rLinkEntry);
	return WLAN_STATUS_SUCCESS;
}

void nicUniCmdEventQueryBtCtrl(struct ADAPTER *prAdapter,
			       struct WIFI_UNI_EVENT *prEvt)
{
	int32_t s4TagsLen;
	uint8_t *prTag;
	uint16_t u2Offset = 0;

	s4TagsLen = GET_UNI_EVENT_DATA_LEN(prEvt) -
			sizeof(struct UNI_EVENT_BT_CTRL);
	prTag = GET_UNI_EVENT_DATA(prEvt) +
			sizeof(struct UNI_EVENT_BT_CTRL);

	if (s4TagsLen <= 0) {
		DBGLOG(NIC, WARN, "the evt tags len is wrong (%d)\n",
			s4TagsLen);
		return;
	}

	TAG_FOR_EACH(prTag, s4TagsLen, u2Offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n",
			TAG_ID(prTag), TAG_LEN(prTag));

		if (TAG_ID(prTag) == UNI_EVENT_BT_CTRL_GET_COREDUMP_HEAD) {
			struct UNI_EVENT_BT_CTRL_HEAD *prBtCtrlTag =
				(struct UNI_EVENT_BT_CTRL_HEAD *) prTag;
			struct EXT_EVENT_BT_CTRL rEvtBtCtrl;

			rEvtBtCtrl.u4Addr = prBtCtrlTag->addr;
			rEvtBtCtrl.u4Length = prBtCtrlTag->length;
			rEvtBtCtrl.u4Round = prBtCtrlTag->round;
			rEvtBtCtrl.u4DumpLeave = prBtCtrlTag->dumpLeave;
			rEvtBtCtrl.u4CurrentRound = prBtCtrlTag->currentRound;
			rEvtBtCtrl.u4Done = prBtCtrlTag->done;

			nicCmdEventQueryBtCtrl(prAdapter, &rEvtBtCtrl);
		}
	}
}

#endif /* CFG_SUPPORT_WF_DUMP_BT_COREDUMP */

#if CFG_SUPPORT_FIPS
uint32_t nicUniCmdFipsTc(struct ADAPTER *prAdapter,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	uint8_t *pucPayload;
	uint16_t u2TotalPayloadSize;
	struct UNI_CMD_FIPS *uni_cmd;
	struct UNI_CMD_FIPS_TC_TAG *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint16_t max_cmd_len = CFG_TX_MAX_PKT_SIZE -
			prChipInfo->u2UniCmdTxHdrSize; /* whole entry size */
	uint16_t u2CmdHeaderLen = sizeof(struct WIFI_UNI_CMD_ENTRY) +
		sizeof(struct UNI_CMD_FIPS) +
		sizeof(struct UNI_CMD_FIPS_TC_TAG); /* non-payload size */
	uint16_t u2MaxPayloadSize = max_cmd_len - u2CmdHeaderLen;
	uint16_t ucFragNum, ucFragTotal;
	uint16_t offset = 0;

	if (info == NULL || info->ucCID != CMD_ID_FIPS_TC)
		return WLAN_STATUS_NOT_ACCEPTED;

	pucPayload = (uint8_t *) info->pucInfoBuffer;
	u2TotalPayloadSize = info->u4SetQueryInfoLen;
	ucFragTotal = (u2TotalPayloadSize / u2MaxPayloadSize) + 1;
	DBGLOG(NIC, INFO,
	       "total payload size:%u, total fragment num:%u, max_cmd_len:%u, buf size:%u\n",
	       u2TotalPayloadSize, ucFragTotal, max_cmd_len,
	       CFG_TX_MAX_PKT_SIZE);

	for (ucFragNum = 1; ucFragNum <= ucFragTotal; ++ucFragNum) {
		uint16_t u2CmdSize, u2PayloadSize;
		uint16_t u2RemainPayloadSize = u2TotalPayloadSize - offset;

		if (u2RemainPayloadSize > u2MaxPayloadSize)
			u2CmdSize = max_cmd_len;
		else
			u2CmdSize = u2RemainPayloadSize + u2CmdHeaderLen;
		u2PayloadSize = u2CmdSize - u2CmdHeaderLen;

		entry = nicUniCmdAllocEntry(prAdapter, UNI_CMD_ID_FIPS,
				u2CmdSize, nicUniCmdEventSetCommon,
				nicUniCmdTimeoutCommon);
		if (!entry)
			return WLAN_STATUS_RESOURCES;

		uni_cmd = (struct UNI_CMD_FIPS *) entry->pucInfoBuffer;
		tag = (struct UNI_CMD_FIPS_TC_TAG *) uni_cmd->aucTlvBuffer;
		tag->u2Tag = UNI_CMD_FIPS_TC;
		tag->u2Length = sizeof(*tag) + u2PayloadSize;
		tag->ucFragNum = ucFragNum;
		tag->ucFragTotal = ucFragTotal;
		tag->u2TCTotalLen = u2TotalPayloadSize;
		tag->u2TCBufferLen = u2PayloadSize;
		kalMemCopy(tag->aucTCBuffer, pucPayload + offset,
			   u2PayloadSize);

		offset += u2PayloadSize;
		LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

		DBGLOG(NIC, INFO, "[%u/%u] dump uni tag header\n",
		       ucFragNum, ucFragTotal);
		DBGLOG_MEM8(NIC, INFO, tag, sizeof(*tag));
		DBGLOG(NIC, INFO, "[%u/%u] dump uni tag buf\n",
		       ucFragNum, ucFragTotal);
		DBGLOG_MEM8(NIC, INFO, tag->aucTCBuffer, tag->u2TCBufferLen);
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdFipsGetStatus(struct ADAPTER *prAdapter,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_FIPS *uni_cmd;
	struct UNI_CMD_FIPS_GET_STATUS_TAG *tag;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint16_t u2CmdSize = sizeof(struct WIFI_UNI_CMD_ENTRY) +
		sizeof(struct UNI_CMD_FIPS) +
		sizeof(struct UNI_CMD_FIPS_GET_STATUS_TAG);

	if (info == NULL || info->ucCID != CMD_ID_FIPS_GET_STATUS)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(prAdapter, UNI_CMD_ID_FIPS,
			u2CmdSize, nicUniEventFips,
			nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	uni_cmd = (struct UNI_CMD_FIPS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_FIPS_GET_STATUS_TAG *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_FIPS_GET_STATUS;
	tag->u2Length = sizeof(*tag);

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	DBGLOG(NIC, INFO, "dump uni tag\n");
	DBGLOG_MEM8(NIC, INFO, tag, tag->u2Length);

	return WLAN_STATUS_SUCCESS;
}

uint32_t nicUniCmdFipsGetResult(struct ADAPTER *prAdapter,
		struct WIFI_UNI_SETQUERY_INFO *info)
{
	struct UNI_CMD_FIPS *uni_cmd;
	struct UNI_CMD_FIPS_GET_RESULT_TAG *tag;
	struct FIPS_PARAM *prFipsParam = NULL;
	struct WIFI_UNI_CMD_ENTRY *entry;
	uint16_t u2CmdSize = sizeof(struct WIFI_UNI_CMD_ENTRY) +
		sizeof(struct UNI_CMD_FIPS) +
		sizeof(struct UNI_CMD_FIPS_GET_RESULT_TAG);

	if (info == NULL || info->ucCID != CMD_ID_FIPS_GET_RESULT)
		return WLAN_STATUS_NOT_ACCEPTED;

	entry = nicUniCmdAllocEntry(prAdapter, UNI_CMD_ID_FIPS,
			u2CmdSize, nicUniEventFips,
			nicUniCmdTimeoutCommon);
	if (!entry)
		return WLAN_STATUS_RESOURCES;

	prFipsParam = (struct FIPS_PARAM *)info->pucInfoBuffer;
	uni_cmd = (struct UNI_CMD_FIPS *) entry->pucInfoBuffer;
	tag = (struct UNI_CMD_FIPS_GET_RESULT_TAG *) uni_cmd->aucTlvBuffer;
	tag->u2Tag = UNI_CMD_FIPS_GET_RESULT;
	tag->u2Length = sizeof(*tag);
	tag->ucFragNum = prFipsParam->ucFragNum;

	LINK_INSERT_TAIL(&info->rUniCmdList, &entry->rLinkEntry);

	DBGLOG(NIC, INFO, "dump uni tag\n");
	DBGLOG_MEM8(NIC, INFO, tag, tag->u2Length);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_FIPS */

#if (CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1)
void nicUniEventShrAntSwch(struct ADAPTER *prAdapter,
	struct WIFI_UNI_EVENT *prWifiEvent)
{
	int32_t tags_len;
	uint8_t *tag;
	uint16_t offset = 0;
	uint32_t fixed_len = sizeof(struct UNI_EVENT_SHR_ANT_SWCH);
	uint32_t data_len = GET_UNI_EVENT_DATA_LEN(prWifiEvent);
	uint8_t *data = GET_UNI_EVENT_DATA(prWifiEvent);
	struct CMD_INFO *prCmdInfo = NULL;

	tags_len = data_len - fixed_len;
	tag = data + fixed_len;
	TAG_FOR_EACH(tag, tags_len, offset) {
		DBGLOG(NIC, TRACE, "Tag(%d, %d)\n", TAG_ID(tag), TAG_LEN(tag));

		switch (TAG_ID(tag)) {
			case UNI_EVENT_SHR_ANT_SWCH_UPDATE: {
				struct UNI_EVENT_SHR_ANT_SWCH_UPDATE_T *exp =
				(struct UNI_EVENT_SHR_ANT_SWCH_UPDATE_T *) tag;
				cnmUpdateSharedAntennaSwitch(prAdapter, exp);
			}
				break;
			default: {
				DBGLOG(NIC, WARN, "invalid tag = %d\n",
					TAG_ID(tag));
			}
				break;
		}
	}

	/* command response handling */
	prCmdInfo = nicGetPendingCmdInfo(prAdapter, prWifiEvent->ucSeqNum);

	if (prCmdInfo != NULL) {
		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prWifiEvent->aucBuffer);
		else if (prCmdInfo->fgIsOid)
			kalOidComplete(prAdapter->prGlueInfo,
				       prCmdInfo,
				       0, WLAN_STATUS_SUCCESS);
		/* return prCmdInfo */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
	}
}
#endif /* CFG_WIFI_RAM_COEX_SPDT_SHR_ANT_CTRL == 1 */
