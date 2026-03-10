/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
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

#define NIC_BSS_MCC_MODE_TOKEN_CNT	64
#define NIC_BSS_LOW_RATE_TOKEN_CNT	256

#define NIC_IS_BSS_11B(prBssInfo) \
	(prBssInfo->ucPhyTypeSet == PHY_TYPE_SET_802_11B)

#define NIC_IS_BSS_BELOW_11AC(prBssInfo) \
	((prBssInfo->ucPhyTypeSet >> PHY_TYPE_VHT_INDEX) == 0)

#define NIC_DUMP_ICV_RXD(prAdapter, prRxStatus) \
	do { \
		u_int8_t fgRxIcvErrDbg = \
			IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgRxIcvErrDbg); \
		if (fgRxIcvErrDbg) { \
			struct mt66xx_chip_info *prChipInfo; \
			\
			prChipInfo = prAdapter->chip_info; \
			DBGLOG(NIC, INFO, "Dump RXD:\n"); \
			DBGLOG_MEM8(NIC, INFO, prRxStatus, \
				prChipInfo->rxd_size); \
		} \
	} while (0)

#define NIC_DUMP_ICV_RXP(pvPayload, u4PayloadLen) \
	do { \
		u_int8_t fgRxIcvErrDbg = \
			IS_FEATURE_ENABLED(prAdapter->rWifiVar.fgRxIcvErrDbg); \
		if (fgRxIcvErrDbg) { \
			DBGLOG(NIC, INFO, "Dump RXP:\n"); \
			if (!pvPayload) \
				break; \
			DBGLOG_MEM8(NIC, INFO, pvPayload, u4PayloadLen); \
		} \
	} while (0)

#define NIC_DUMP_TXD_HEADER(prAdapter, header) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxD) \
			DBGLOG(TX, TRACE, header); \
	} while (0)

#define NIC_DUMP_TXD(prAdapter, addr, size) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxD) { \
			DBGLOG(TX, TRACE, "Dump TXD:\n"); \
			DBGLOG_MEM8(TX, TRACE, addr, size); \
		} \
	} while (0)

#define NIC_DUMP_TXDMAD_HEADER(prAdapter, header) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxDmad) \
			DBGLOG(TX, TRACE, header); \
	} while (0)

#define NIC_DUMP_TXDMAD(prAdapter, addr, size) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxDmad) { \
			DBGLOG(TX, TRACE, "Dump TXDMAD:\n"); \
			DBGLOG_MEM8(TX, TRACE, addr, size); \
		} \
	} while (0)

#define NIC_DUMP_TXP_HEADER(prAdapter, header) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxP) \
			DBGLOG(TX, TRACE, header); \
	} while (0)

#define NIC_DUMP_TXP(prAdapter, addr, size) \
	do { \
		if (prAdapter->rWifiVar.fgDumpTxP) { \
			DBGLOG(TX, TRACE, "Dump TXP:\n"); \
			DBGLOG_MEM8(TX, TRACE, addr, size); \
		} \
	} while (0)

#define NIC_DUMP_RXD_HEADER(prAdapter, header) \
	do { \
		if (prAdapter->rWifiVar.fgDumpRxD) \
			DBGLOG(RX, TRACE, header); \
	} while (0)

#define NIC_DUMP_RXD(prAdapter, addr, size) \
	do { \
		if (prAdapter->rWifiVar.fgDumpRxD) { \
			DBGLOG(RX, TRACE, "Dump RXD:\n"); \
			DBGLOG_MEM8(RX, TRACE, addr, size); \
		} \
	} while (0)

#define NIC_DUMP_RXDMAD_HEADER(prAdapter, header) \
	do { \
		if (prAdapter->rWifiVar.fgDumpRxDmad) \
			DBGLOG(RX, TRACE, header); \
	} while (0)

#define NIC_DUMP_RXDMAD(prAdapter, addr, size) \
	do { \
		if (prAdapter->rWifiVar.fgDumpRxDmad) { \
			DBGLOG(RX, TRACE, "Dump RXDMAD:\n"); \
			DBGLOG_MEM8(RX, TRACE, addr, size); \
		} \
	} while (0)

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
	IE_UPD_METHOD_UPDATE_RANDOM = 0,
	IE_UPD_METHOD_UPDATE_ALL = 1,
	IE_UPD_METHOD_DELETE_ALL = 2,
	IE_UPD_METHOD_UPDATE_PROBE_RSP = 3,
	IE_UPD_METHOD_UNSOL_PROBE_RSP = 4,
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

enum ENUM_ECO_VER {
	ECO_VER_1 = 1,
	ECO_VER_2,
	ECO_VER_3
};

enum ENUM_REMOVE_BY_MSDU_TPYE {
	MSDU_REMOVE_BY_WLAN_INDEX = 0,
	MSDU_REMOVE_BY_BSS_INDEX,
	MSDU_REMOVE_BY_ALL,
	ENUM_REMOVE_BY_MSDU_TPYE_NUM
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
	uint16_t u2Count;
	uint16_t u2TimePerHundred;
	OS_SYSTIME rStartTime;
};
#endif /* fos_change end */

#if CFG_SUPPORT_PKT_OFLD

enum RX_DATA_MODE {
	RX_DATA_MODE_TO_HOST,
	RX_DATA_MODE_SUSPEND_TO_FW,
	RX_DATA_MODE_CFG_SUSPEND_TO_FW,
	RX_DATA_MODE_FORCE_TO_FW,
	RX_DATA_MODE_NUM
};

struct ABNORMAL_WAKEUP_STATISTIC {
	uint16_t u2Count;
	OS_SYSTIME rStartTime;
};
#endif

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

#define NETWORK_BSSID_MASK BITS(0, 5)
#define NETWORK_BSSID_OFFSET 0
#define NETWORK_LINKID_MASK BITS(6, 7)
#define NETWORK_LINKID_OFFSET 6
#define NETWORK_ID(_bss_idx, _link_idx) \
	((((uint8_t)_link_idx) << NETWORK_LINKID_OFFSET) | \
	(((uint8_t)_bss_idx) & NETWORK_BSSID_MASK))
#define NETWORK_BSS_ID(_network_idx) \
	(((uint8_t)_network_idx) & NETWORK_BSSID_MASK)
#define NETWORK_LINK_ID(_network_idx) \
	((((uint8_t)_network_idx) & NETWORK_LINKID_MASK) >> \
	 NETWORK_LINKID_OFFSET)

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if (CFG_TWT_SMART_STA == 1)
extern struct _TWT_SMART_STA_T g_TwtSmartStaCtrl;
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
uint32_t nicAllocateAdapterMemory(struct ADAPTER
				  *prAdapter);

void nicLinkStatsFreeCacheBuffer(struct ADAPTER *prAdapter);

void nicReleaseAdapterMemory(struct ADAPTER *prAdapter);

void nicDisableInterrupt(struct ADAPTER *prAdapter);

void nicEnableInterrupt(struct ADAPTER *prAdapter);

uint32_t nicProcessIST(struct ADAPTER *prAdapter);

uint32_t nicProcessISTWithSpecifiedCount(struct ADAPTER *prAdapter,
	uint32_t u4HifIstLoopCount);

uint32_t nicProcessIST_impl(struct ADAPTER *prAdapter,
			    uint32_t u4IntStatus);

uint32_t nicInitializeAdapter(struct ADAPTER *prAdapter);

void nicMCRInit(struct ADAPTER *prAdapter);

u_int8_t nicVerifyChipID(struct ADAPTER *prAdapter);

void nicpmWakeUpWiFi(struct ADAPTER *prAdapter);

u_int8_t nicpmSetDriverOwn(struct ADAPTER *prAdapter);

void nicpmSetFWOwn(struct ADAPTER *prAdapter,
		   u_int8_t fgEnableGlobalInt);

u_int8_t nicpmSetAcpiPowerD0(struct ADAPTER *prAdapter);

void nicTriggerAHDBG(struct ADAPTER *prAdapter,
	uint32_t u4mod, uint32_t u4reason,
	uint32_t u4BssIdx, uint32_t u4wlanIdx);

u_int8_t nicpmSetAcpiPowerD3(struct ADAPTER *prAdapter);

#if defined(_HIF_SPI)
void nicRestoreSpiDefMode(struct ADAPTER *prAdapter);
#endif

void nicProcessSoftwareInterruptEx(struct ADAPTER
				 *prAdapter);

void nicProcessSoftwareInterrupt(struct ADAPTER
				 *prAdapter);

void nicProcessAbnormalInterrupt(struct ADAPTER
				 *prAdapter);

void nicSetSwIntr(struct ADAPTER *prAdapter,
		  uint32_t u4SwIntrBitmap);

struct CMD_INFO *nicGetPendingCmdInfo(struct ADAPTER *prAdapter,
					uint8_t ucSeqNum);
void removeDuplicatePendingCmd(struct ADAPTER *prAdapter,
				struct CMD_INFO *prCmdInfo);

struct MSDU_INFO *nicGetPendingTxMsduInfo(struct ADAPTER *prAdapter,
		uint8_t ucWlanIndex, uint8_t ucPID, uint8_t ucTID);

void nicFreePendingTxMsduInfo(struct ADAPTER *prAdapter,
	uint8_t ucIndex, enum ENUM_REMOVE_BY_MSDU_TPYE ucFreeType);

uint8_t nicIncreaseCmdSeqNum(struct ADAPTER *prAdapter);

uint8_t nicIncreaseTxSeqNum(struct ADAPTER *prAdapter);

/* Media State Change */
uint32_t
nicMediaStateChange(struct ADAPTER *prAdapter,
		    uint8_t ucBssIndex,
		    struct EVENT_CONNECTION_STATUS *prConnectionStatus);

uint32_t nicMediaJoinFailure(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex, uint32_t rStatus);

/* Utility function for channel number conversion */
uint32_t nicChannelNum2Freq(uint32_t u4ChannelNum, enum ENUM_BAND eBand);

uint32_t nicFreq2ChannelNum(uint32_t u4FreqInKHz);

uint32_t nicGetS1Freq(enum ENUM_BAND eBand,
			uint8_t ucPrimaryChannel,
			uint8_t ucBandwidth);

#if (CFG_SUPPORT_802_11BE == 1)
uint8_t nicGetEhtS1(enum ENUM_BAND eBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth);
uint8_t nicGetEht6gS1(uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth);
uint8_t nicGetEhtS2(enum ENUM_BAND eBand,
	uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth);
uint8_t nicGetEht6gS2(uint8_t ucPrimaryChannel,
	uint8_t ucBandwidth);
#endif

/* Utility to get S1, S2 */
uint8_t nicGetS1(enum ENUM_BAND eBand,
		uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth);
uint8_t nicGetS2(enum ENUM_BAND eBand,
		uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth,
		uint8_t ucS1);
uint8_t nicGetVhtS1(uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth);
#if (CFG_SUPPORT_WIFI_6G == 1)
uint8_t nicGetHe6gS1(uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth);
uint8_t nicGetHe6gS2(uint8_t ucPrimaryChannel,
		uint8_t ucBandwidth,
		uint8_t ucS1);
uint8_t nicGetHe6gS1BW40(uint8_t ucPrimaryChannel);
#endif

/* firmware command wrapper */
/* NETWORK (WIFISYS) */
uint32_t nicActivateNetwork(struct ADAPTER *prAdapter,
			    uint8_t ucNetworkIndex);
uint32_t nicActivateNetworkEx(struct ADAPTER *prAdapter,
			    uint8_t ucNetworkIndex,
			    uint8_t fgReset40mBw);
uint32_t nicDeactivateNetwork(struct ADAPTER *prAdapter,
				uint8_t ucNetworkIndex);
uint32_t nicDeactivateNetworkEx(struct ADAPTER *prAdapter,
				uint8_t ucNetworkIndex,
				uint8_t fgClearStaRec);

void nicUpdateNetifTxThByBssId(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint32_t u4StopTh, uint32_t u4StartTh);

/* BSS-INFO */
uint32_t nicUpdateBss(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex);

uint32_t nicUpdateDscb(struct ADAPTER *prAdapter,
			uint8_t		ucBssIndex,
			uint16_t	u2PreDscBitmap,
			uint16_t	u2NewDscBitmap);

uint32_t nicUpdateBssEx(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			uint8_t fgClearStaRec);
/* BSS-INFO Indication (PM) */
uint32_t nicPmIndicateBssCreated(struct ADAPTER
				 *prAdapter, uint8_t ucBssIndex);

uint32_t nicPmIndicateBssConnected(struct ADAPTER
				   *prAdapter, uint8_t ucBssIndex);

uint32_t nicPmIndicateBssAbort(struct ADAPTER *prAdapter,
			       uint8_t ucBssIndex);

/* Beacon Template Update */
uint32_t
nicUpdateBeaconIETemplate(struct ADAPTER *prAdapter,
			  enum ENUM_IE_UPD_METHOD eIeUpdMethod,
			  uint8_t ucBssIndex, uint16_t u2Capability,
			  uint8_t *aucIe, uint16_t u2IELen);

uint32_t nicQmUpdateWmmParms(struct ADAPTER *prAdapter,
			     uint8_t ucBssIndex);

#if (CFG_SUPPORT_802_11AX == 1)
uint32_t nicQmUpdateMUEdcaParams(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
uint32_t nicRlmUpdateSRParams(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);
#endif

uint32_t nicSetAutoTxPower(struct ADAPTER *prAdapter,
			   struct CMD_AUTO_POWER_PARAM *prAutoPwrParam);

/* RXD relative */
void nicRxdChNumTranslate(
	enum ENUM_BAND eBand, uint8_t *pucHwChannelNum);

/*----------------------------------------------------------------------------*/
/* Calibration Control                                                        */
/*----------------------------------------------------------------------------*/
uint32_t nicUpdateTxPower(struct ADAPTER *prAdapter,
			  struct CMD_TX_PWR *prTxPwrParam);

uint32_t nicUpdate5GOffset(struct ADAPTER *prAdapter,
			   struct CMD_5G_PWR_OFFSET *pr5GPwrOffset);

uint32_t nicUpdateDPD(struct ADAPTER *prAdapter,
		      struct CMD_PWR_PARAM *prDpdCalResult);

/*----------------------------------------------------------------------------*/
/* PHY configuration                                                          */
/*----------------------------------------------------------------------------*/
void nicSetAvailablePhyTypeSet(struct ADAPTER
			       *prAdapter);

/*----------------------------------------------------------------------------*/
/* MGMT and System Service Control                                            */
/*----------------------------------------------------------------------------*/
void nicInitSystemService(struct ADAPTER *prAdapter,
				   const u_int8_t bAtResetFlow);

void nicResetSystemService(struct ADAPTER *prAdapter);

void nicUninitSystemService(struct ADAPTER *prAdapter);

void nicInitMGMT(struct ADAPTER *prAdapter,
		struct REG_INFO *prRegInfo);

void nicUninitMGMT(struct ADAPTER *prAdapter);

void
nicPowerSaveInfoMap(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, enum PARAM_POWER_MODE ePowerMode,
		enum POWER_SAVE_CALLER ucCaller);

uint32_t
nicConfigPowerSaveProfile(struct ADAPTER *prAdapter,
		uint8_t ucBssIndex, enum PARAM_POWER_MODE ePwrMode,
		u_int8_t fgEnCmdEvent, enum POWER_SAVE_CALLER ucCaller);

uint32_t
nicConfigProcSetCamCfgWrite(struct ADAPTER *prAdapter,
		u_int8_t enabled, uint8_t ucBssIndex);

uint32_t nicEnterCtiaMode(struct ADAPTER *prAdapter,
		u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterTPTestMode(struct ADAPTER *prAdapter,
		uint8_t ucFuncMask);

uint32_t nicEnterCtiaModeOfScan(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfRoaming(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfCAM(struct ADAPTER *prAdapter,
		u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfBCNTimeout(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfAutoTxPower(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);

uint32_t nicEnterCtiaModeOfFIFOFullNoAck(struct ADAPTER
		*prAdapter, u_int8_t fgEnterCtia, u_int8_t fgEnCmdEvent);
/*----------------------------------------------------------------------------*/
/* Scan Result Processing                                                     */
/*----------------------------------------------------------------------------*/
void
nicAddScanResult(struct ADAPTER *prAdapter,
		 uint8_t rMacAddr[PARAM_MAC_ADDR_LEN],
		 struct PARAM_SSID *prSsid,
		 uint16_t u2CapInfo,
		 int32_t rRssi,
		 enum ENUM_PARAM_NETWORK_TYPE eNetworkType,
		 struct PARAM_802_11_CONFIG *prConfiguration,
		 enum ENUM_PARAM_OP_MODE eOpMode,
		 uint8_t rSupportedRates[PARAM_MAX_LEN_RATES_EX],
		 uint16_t u2IELength, uint8_t *pucIEBuf);

void nicFreeScanResultIE(struct ADAPTER *prAdapter,
			 uint32_t u4Idx);

/*----------------------------------------------------------------------------*/
/* Fixed Rate Hacking                                                         */
/*----------------------------------------------------------------------------*/
uint32_t
nicUpdateRateParams(struct ADAPTER *prAdapter,
		    enum ENUM_REGISTRY_FIXED_RATE eRateSetting,
		    uint8_t *pucDesiredPhyTypeSet,
		    uint16_t *pu2DesiredNonHTRateSet,
		    uint16_t *pu2BSSBasicRateSet,
		    uint8_t *pucMcsSet, uint8_t *pucSupMcs32,
		    uint16_t *u2HtCapInfo);

/*----------------------------------------------------------------------------*/
/* Write registers                                                            */
/*----------------------------------------------------------------------------*/
uint32_t nicWriteMcr(struct ADAPTER *prAdapter,
		     uint32_t u4Address, uint32_t u4Value);

/*----------------------------------------------------------------------------*/
/* Update auto rate                                                           */
/*----------------------------------------------------------------------------*/
uint32_t
nicRlmArUpdateParms(struct ADAPTER *prAdapter,
		    uint32_t u4ArSysParam0,
		    uint32_t u4ArSysParam1, uint32_t u4ArSysParam2,
		    uint32_t u4ArSysParam3);

/*----------------------------------------------------------------------------*/
/* Link Quality Updating                                                      */
/*----------------------------------------------------------------------------*/
void
nicUpdateLinkQuality(struct ADAPTER *prAdapter,
		     uint8_t ucBssIndex,
		     struct EVENT_LINK_QUALITY *prEventLinkQuality);

void nicUpdateRSSI(struct ADAPTER *prAdapter,
		   uint8_t ucBssIndex, int8_t cRssi,
		   int8_t cLinkQuality);

void nicUpdateLinkSpeed(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex, uint16_t u2TxLinkSpeed);

#if CFG_SUPPORT_RDD_TEST_MODE
uint32_t nicUpdateRddTestMode(struct ADAPTER *prAdapter,
			      struct CMD_RDD_CH *prRddChParam);
#endif

#if (CFG_COALESCING_INTERRUPT == 1)
uint32_t nicSetCoalescingInt(struct ADAPTER *prAdapter,
				  u_int8_t fgPktThEn, u_int8_t fgTmrThEn);
#endif

/*----------------------------------------------------------------------------*/
/* Address Setting Apply                                                      */
/*----------------------------------------------------------------------------*/
uint32_t nicApplyNetworkAddress(struct ADAPTER
				*prAdapter);
void nicApplyLinkAddress(struct ADAPTER *prAdapter,
	uint8_t *pucSrcMAC,
	uint8_t *pucDestMAC,
	uint8_t ucLinkIdx);

/*----------------------------------------------------------------------------*/
/* ECO Version                                                                */
/*----------------------------------------------------------------------------*/
uint8_t nicGetChipSwVer(void);
uint8_t nicGetChipEcoVer(struct ADAPTER *prAdapter);
u_int8_t nicIsEcoVerEqualTo(struct ADAPTER *prAdapter,
			    uint8_t ucEcoVer);
u_int8_t nicIsEcoVerEqualOrLaterTo(struct ADAPTER
				   *prAdapter, uint8_t ucEcoVer);
uint8_t nicSetChipHwVer(uint8_t value);
uint8_t nicSetChipSwVer(uint8_t value);
uint8_t nicSetChipFactoryVer(uint8_t value);

void nicSerStopTxRx(struct ADAPTER *prAdapter);
void nicSerStopTx(struct ADAPTER *prAdapter);
void nicSerStartTxRx(struct ADAPTER *prAdapter);
u_int8_t nicSerIsWaitingReset(struct ADAPTER *prAdapter);
u_int8_t nicSerIsTxStop(struct ADAPTER *prAdapter);
u_int8_t nicSerIsRxStop(struct ADAPTER *prAdapter);
void nicSerReInitBeaconFrame(struct ADAPTER *prAdapter);
void nicSerInit(struct ADAPTER *prAdapter, const u_int8_t bAtResetFlow);
void nicSerDeInit(struct ADAPTER *prAdapter);

/* fos_change begin */
#if CFG_SUPPORT_WAKEUP_STATISTICS
void nicUpdateWakeupStatistics(struct ADAPTER *prAdapter,
	enum WAKEUP_TYPE intType);
#endif /* fos_change end */

void nicDumpMsduInfo(struct MSDU_INFO *prMsduInfo);

uint8_t nicGetActiveTspec(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex);

#if CFG_SUPPORT_PKT_OFLD
void nicAbnormalWakeupHandler(struct ADAPTER *prAdapter);
void nicAbnormalWakeupMonReset(struct ADAPTER *prAdapter);
#endif
#endif /* _NIC_H */
