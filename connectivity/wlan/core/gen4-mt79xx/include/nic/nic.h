/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: //Department/DaVinci/BRANCHES/
 *      MT6620_WIFI_DRIVER_V2_3/include/nic/nic.h#1
 */

/*! \file   "nic.h"
 *    \brief  The declaration of nic functions
 *
 *    Detail description.
 */


#ifndef _NIC_H
#define _NIC_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define PS_SYNC_WITH_FW		BIT(31)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

struct REG_ENTRY {
	uint32_t u4Offset;
	uint32_t u4Value;
};

struct _TABLE_ENTRY_T {
	struct REG_ENTRY *pu4TablePtr;
	uint16_t u2Size;
};

/*! INT status to event map */
struct INT_EVENT_MAP {
	uint32_t u4Int;
	uint32_t u4Event;
};

struct ECO_INFO {
	uint8_t ucHwVer;
	uint8_t ucRomVer;
	uint8_t ucFactoryVer;
	uint8_t ucEcoVer;
};

enum ENUM_INT_EVENT_T {
	INT_EVENT_ABNORMAL,
	INT_EVENT_SW_INT,
	INT_EVENT_TX,
	INT_EVENT_RX,
	INT_EVENT_NUM
};

enum ENUM_IE_UPD_METHOD {
	IE_UPD_METHOD_UPDATE_RANDOM,
	IE_UPD_METHOD_UPDATE_ALL,
	IE_UPD_METHOD_DELETE_ALL,
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
	IE_UPD_METHOD_UPDATE_PROBE_RSP,
#endif
};

enum ENUM_SER_STATE {
	SER_IDLE_DONE,       /* SER is idle or done */
	SER_STOP_HOST_TX,    /* Host HIF Tx is stopped */
	SER_STOP_HOST_TX_RX, /* Host HIF Tx/Rx is stopped */
	SER_REINIT_HIF,      /* Host HIF is reinit */

	SER_STATE_NUM
};

/* The bit map for the caller to set the power save flag */
enum POWER_SAVE_CALLER {
	PS_CALLER_COMMON = 0,
	PS_CALLER_CTIA,
	PS_CALLER_SW_WRITE,
	PS_CALLER_CTIA_CAM,
	PS_CALLER_P2P,
	PS_CALLER_CAMCFG,
	PS_CALLER_GPU,
	PS_CALLER_TP,
	PS_CALLER_NO_TIM,
	PS_CALLER_WOW,
	PS_CALLER_MAX_NUM = 24
};

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
enum WAKEUP_TYPE {
	ABNORMAL_INT,
	SOFTWARE_INT,
	TX_INT,
	RX_DATA_INT,
	RX_EVENT_INT,
	RX_MGMT_INT,
	RX_OTHERS_INT,
	WAKEUP_TYPE_NUM
};
struct WAKEUP_STATISTIC {
	uint32_t u4Count;
	uint32_t u4TimePerHundred;
	OS_SYSTIME rStartTime;
};
#endif /* fos_change end */

enum ENUM_ECO_VER {
	ECO_VER_1 = 1,
	ECO_VER_2,
	ECO_VER_3
};

enum ENUM_REMOVE_BY_MSDU_TPYE {
	MSDU_REMOVE_BY_WLAN_INDEX = 0,
	MSDU_REMOVE_BY_BSS_INDEX,
	ENUM_REMOVE_BY_MSDU_TPYE_NUM
};

/* Test mode bitmask of disable flag */
#define TEST_MODE_DISABLE_ONLINE_SCAN  BIT(0)
#define TEST_MODE_DISABLE_ROAMING      BIT(1)
#define TEST_MODE_FIXED_CAM_MODE       BIT(2)
#define TEST_MODE_DISABLE_BCN_LOST_DET BIT(3)
#define TEST_MODE_NONE                0
#define TEST_MODE_THROUGHPUT \
		(TEST_MODE_DISABLE_ONLINE_SCAN | TEST_MODE_DISABLE_ROAMING | \
		TEST_MODE_FIXED_CAM_MODE | TEST_MODE_DISABLE_BCN_LOST_DET)
#define TEST_MODE_SIGMA_AC_N_PMF \
		(TEST_MODE_DISABLE_ONLINE_SCAN | TEST_MODE_FIXED_CAM_MODE)
#define TEST_MODE_SIGMA_WMM_PS (TEST_MODE_DISABLE_ONLINE_SCAN)

#define PS_CALLER_ACTIVE \
		(BITS(0, (PS_CALLER_MAX_NUM - 1)) & (~BIT(PS_CALLER_WOW)))

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if defined(_HIF_USB)
extern struct TIMER rSerSyncTimer;
#endif

#if CFG_SUPPORT_WAKEUP_STATISTICS
extern struct WAKEUP_STATISTIC g_arWakeupStatistic[WAKEUP_TYPE_NUM];
extern uint32_t g_wake_event_count[EVENT_ID_END];
#endif

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
/*----------------------------------------------------------------------------*/
/* Routines in nic.c                                                          */
/*----------------------------------------------------------------------------*/
uint32_t nicAllocateAdapterMemory(IN struct ADAPTER
				  *prAdapter);

void nicReleaseAdapterMemory(IN struct ADAPTER *prAdapter);

void nicDisableInterrupt(IN struct ADAPTER *prAdapter);

void nicEnableInterrupt(IN struct ADAPTER *prAdapter);

uint32_t nicProcessIST(IN struct ADAPTER *prAdapter);

uint32_t nicProcessIST_impl(IN struct ADAPTER *prAdapter,
			    IN uint32_t u4IntStatus);

uint32_t nicInitializeAdapter(IN struct ADAPTER *prAdapter);

void nicMCRInit(IN struct ADAPTER *prAdapter);

u_int8_t nicVerifyChipID(IN struct ADAPTER *prAdapter);

void nicpmWakeUpWiFi(IN struct ADAPTER *prAdapter);

u_int8_t nicpmSetDriverOwn(IN struct ADAPTER *prAdapter);

void nicpmSetFWOwn(IN struct ADAPTER *prAdapter,
		   IN u_int8_t fgEnableGlobalInt);

u_int8_t nicpmSetAcpiPowerD0(IN struct ADAPTER *prAdapter);

u_int8_t nicpmSetAcpiPowerD3(IN struct ADAPTER *prAdapter);

#if defined(_HIF_SPI)
void nicRestoreSpiDefMode(IN struct ADAPTER *prAdapter);
#endif

void nicProcessSoftwareInterrupt(IN struct ADAPTER
				 *prAdapter);

void nicProcessAbnormalInterrupt(IN struct ADAPTER
				 *prAdapter);

void nicSetSwIntr(IN struct ADAPTER *prAdapter,
		  IN uint32_t u4SwIntrBitmap);

struct CMD_INFO *nicGetPendingCmdInfo(IN struct ADAPTER
				      *prAdapter, IN uint8_t ucSeqNum);

struct MSDU_INFO *nicGetPendingTxMsduInfo(
	IN struct ADAPTER *prAdapter, IN uint8_t ucWlanIndex,
	IN uint8_t ucSeqNum);

void nicFreePendingTxMsduInfo(IN struct ADAPTER *prAdapter,
	IN uint8_t ucIndex, IN enum ENUM_REMOVE_BY_MSDU_TPYE ucFreeType);

uint8_t nicIncreaseCmdSeqNum(IN struct ADAPTER *prAdapter);

uint8_t nicIncreaseTxSeqNum(IN struct ADAPTER *prAdapter);

/* Media State Change */
uint32_t
nicMediaStateChange(IN struct ADAPTER *prAdapter,
		    IN uint8_t ucBssIndex,
		    IN struct EVENT_CONNECTION_STATUS *prConnectionStatus);

uint32_t nicMediaJoinFailure(IN struct ADAPTER *prAdapter,
			     IN uint8_t ucBssIndex, IN uint32_t rStatus);

/* Utility function for channel number conversion */
uint32_t nicChannelNum2Freq(IN uint32_t u4ChannelNum,
				IN enum ENUM_BAND eBand);

uint32_t nicFreq2ChannelNum(IN uint32_t u4FreqInKHz);

uint8_t nicGetVhtS1(IN uint8_t ucPrimaryChannel,
		    IN uint8_t ucBandwidth);

#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetHe6gS1(IN uint8_t ucPrimaryChannel,
		    IN uint8_t ucBandwidth);
#endif

/* firmware command wrapper */
/* NETWORK (WIFISYS) */
uint32_t nicActivateNetwork(IN struct ADAPTER *prAdapter,
			    IN uint8_t ucBssIndex);

uint32_t nicDeactivateNetwork(IN struct ADAPTER *prAdapter,
			      IN uint8_t ucBssIndex);

/* BSS-INFO */
uint32_t nicUpdateBss(IN struct ADAPTER *prAdapter,
		      IN uint8_t ucBssIndex);

/* BSS-INFO Indication (PM) */
uint32_t nicPmIndicateBssCreated(IN struct ADAPTER
				 *prAdapter, IN uint8_t ucBssIndex);

uint32_t nicPmIndicateBssConnected(IN struct ADAPTER
				   *prAdapter, IN uint8_t ucBssIndex);

uint32_t nicPmIndicateBssAbort(IN struct ADAPTER *prAdapter,
			       IN uint8_t ucBssIndex);

/* Beacon Template Update */
uint32_t
nicUpdateBeaconIETemplate(IN struct ADAPTER *prAdapter,
			  IN enum ENUM_IE_UPD_METHOD eIeUpdMethod,
			  IN uint8_t ucBssIndex, IN uint16_t u2Capability,
			  IN uint8_t *aucIe, IN uint16_t u2IELen);

uint32_t nicQmUpdateWmmParms(IN struct ADAPTER *prAdapter,
			     IN uint8_t ucBssIndex);

#if (CFG_SUPPORT_802_11AX == 1)
uint32_t nicQmUpdateMUEdcaParams(IN struct ADAPTER *prAdapter,
	IN uint8_t ucBssIndex);
uint32_t nicRlmUpdateSRParams(IN struct ADAPTER *prAdapter,
	IN uint8_t ucBssIndex);
#endif

uint32_t nicSetAutoTxPower(IN struct ADAPTER *prAdapter,
			   IN struct CMD_AUTO_POWER_PARAM *prAutoPwrParam);

#if (CFG_SUPPORT_WIFI_6G == 1)
/* RXD relative */
void nicRxdChNumTranslate(
	IN enum ENUM_BAND eBand, IN uint8_t *pucHwChannelNum);
#endif

/*----------------------------------------------------------------------------*/
/* Calibration Control                                                        */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdateTxPower(IN struct ADAPTER *prAdapter,
			  IN struct CMD_TX_PWR *prTxPwrParam);

uint32_t nicUpdate5GOffset(IN struct ADAPTER *prAdapter,
			   IN struct CMD_5G_PWR_OFFSET *pr5GPwrOffset);

uint32_t nicUpdateDPD(IN struct ADAPTER *prAdapter,
		      IN struct CMD_PWR_PARAM *prDpdCalResult);

/*----------------------------------------------------------------------------*/
/* PHY configuration                                                          */
/*----------------------------------------------------------------------------*/
void nicSetAvailablePhyTypeSet(IN struct ADAPTER
			       *prAdapter);

/*----------------------------------------------------------------------------*/
/* MGMT and System Service Control                                            */
/*----------------------------------------------------------------------------*/
void nicInitSystemService(IN struct ADAPTER *prAdapter,
				   IN const u_int8_t bAtResetFlow);

void nicResetSystemService(IN struct ADAPTER *prAdapter);

void nicUninitSystemService(IN struct ADAPTER *prAdapter);

void nicInitMGMT(IN struct ADAPTER *prAdapter,
		IN struct REG_INFO *prRegInfo);

void nicUninitMGMT(IN struct ADAPTER *prAdapter);

void
nicPowerSaveInfoMap(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN enum PARAM_POWER_MODE ePowerMode,
		IN enum POWER_SAVE_CALLER ucCaller);

uint32_t
nicConfigPowerSaveProfile(IN struct ADAPTER *prAdapter,
		IN uint8_t ucBssIndex, IN enum PARAM_POWER_MODE ePwrMode,
		IN u_int8_t fgEnCmdEvent, IN enum POWER_SAVE_CALLER ucCaller);

uint32_t
nicConfigProcSetCamCfgWrite(IN struct ADAPTER *prAdapter,
		IN u_int8_t enabled, IN uint8_t ucBssIndex);

uint32_t nicEnterCtiaMode(IN struct ADAPTER *prAdapter,
		u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterTPTestMode(IN struct ADAPTER *prAdapter,
		IN uint8_t ucFuncMask);

uint32_t nicEnterCtiaModeOfScan(IN struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfRoaming(IN struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfCAM(IN struct ADAPTER *prAdapter,
		u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfBCNTimeout(IN struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfAutoTxPower(IN struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfFIFOFullNoAck(IN struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);
/*----------------------------------------------------------------------------*/
/* Scan Result Processing                                                     */
/*----------------------------------------------------------------------------*/
void
nicAddScanResult(IN struct ADAPTER *prAdapter,
		 IN uint8_t rMacAddr[PARAM_MAC_ADDR_LEN],
		 IN struct PARAM_SSID *prSsid,
		 IN uint16_t u2CapInfo,
		 IN int32_t rRssi,
		 IN enum ENUM_PARAM_NETWORK_TYPE eNetworkType,
		 IN struct PARAM_802_11_CONFIG *prConfiguration,
		 IN enum ENUM_PARAM_OP_MODE eOpMode,
		 IN uint8_t rSupportedRates[PARAM_MAX_LEN_RATES_EX],
		 IN uint16_t u2IELength, IN uint8_t *pucIEBuf);

void nicFreeScanResultIE(IN struct ADAPTER *prAdapter,
			 IN uint32_t u4Idx);

/*----------------------------------------------------------------------------*/
/* Fixed Rate Hacking                                                         */
/*----------------------------------------------------------------------------*/
uint32_t
nicUpdateRateParams(IN struct ADAPTER *prAdapter,
		    IN enum ENUM_REGISTRY_FIXED_RATE eRateSetting,
		    IN uint8_t *pucDesiredPhyTypeSet,
		    IN uint16_t *pu2DesiredNonHTRateSet,
		    IN uint16_t *pu2BSSBasicRateSet,
		    IN uint8_t *pucMcsSet, IN uint8_t *pucSupMcs32,
		    IN uint16_t *u2HtCapInfo);

/*----------------------------------------------------------------------------*/
/* Write registers                                                            */
/*----------------------------------------------------------------------------*/
uint32_t nicWriteMcr(IN struct ADAPTER *prAdapter,
		     IN uint32_t u4Address, IN uint32_t u4Value);

/*----------------------------------------------------------------------------*/
/* Update auto rate                                                           */
/*----------------------------------------------------------------------------*/
uint32_t
nicRlmArUpdateParms(IN struct ADAPTER *prAdapter,
		    IN uint32_t u4ArSysParam0,
		    IN uint32_t u4ArSysParam1, IN uint32_t u4ArSysParam2,
		    IN uint32_t u4ArSysParam3);

/*----------------------------------------------------------------------------*/
/* Link Quality Updating                                                      */
/*----------------------------------------------------------------------------*/
void
nicUpdateLinkQuality(IN struct ADAPTER *prAdapter,
		     IN uint8_t ucBssIndex,
		     IN struct EVENT_LINK_QUALITY *prEventLinkQuality);

void nicUpdateRSSI(IN struct ADAPTER *prAdapter,
		   IN uint8_t ucBssIndex, IN int8_t cRssi,
		   IN int8_t cLinkQuality);

void nicUpdateLinkSpeed(IN struct ADAPTER *prAdapter,
			IN uint8_t ucBssIndex, IN uint16_t u2LinkSpeed);

#if CFG_SUPPORT_RDD_TEST_MODE
uint32_t nicUpdateRddTestMode(IN struct ADAPTER *prAdapter,
			      IN struct CMD_RDD_CH *prRddChParam);
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
uint32_t nicSetCoalescingInt(IN struct ADAPTER *prAdapter,
				  IN u_int8_t fgPktThEn, IN u_int8_t fgTmrThEn);
#endif

/*----------------------------------------------------------------------------*/
/* Address Setting Apply                                                      */
/*----------------------------------------------------------------------------*/
uint32_t nicApplyNetworkAddress(IN struct ADAPTER
				*prAdapter);

/*----------------------------------------------------------------------------*/
/* ECO Version                                                                */
/*----------------------------------------------------------------------------*/
uint8_t nicGetChipSwVer(void);
uint8_t nicGetChipEcoVer(IN struct ADAPTER *prAdapter);
u_int8_t nicIsEcoVerEqualTo(IN struct ADAPTER *prAdapter,
			    uint8_t ucEcoVer);
u_int8_t nicIsEcoVerEqualOrLaterTo(IN struct ADAPTER
				   *prAdapter, uint8_t ucEcoVer);
uint8_t nicSetChipHwVer(uint8_t value);
uint8_t nicSetChipSwVer(uint8_t value);
uint8_t nicSetChipFactoryVer(uint8_t value);

#if defined(_HIF_USB)
void nicSerTimerHandler(IN struct ADAPTER *prAdapter,
			IN unsigned long plParamPtr);
#endif /* _HIF_USB */
void nicSerStopTxRx(IN struct ADAPTER *prAdapter);
void nicSerStopTx(IN struct ADAPTER *prAdapter);
void nicSerStartTxRx(IN struct ADAPTER *prAdapter);
u_int8_t nicSerIsWaitingReset(IN struct ADAPTER *prAdapter);
u_int8_t nicSerIsTxStop(IN struct ADAPTER *prAdapter);
u_int8_t nicSerIsRxStop(IN struct ADAPTER *prAdapter);
void nicSerReInitBeaconFrame(IN struct ADAPTER *prAdapter);
void nicSerInit(IN struct ADAPTER *prAdapter, IN const u_int8_t bAtResetFlow);
void nicSerDeInit(IN struct ADAPTER *prAdapter);

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
void nicUpdateWakeupStatistics(IN struct ADAPTER *prAdapter,
	IN enum WAKEUP_TYPE intType);
#endif /* fos_change end */

#endif /* _NIC_H */
