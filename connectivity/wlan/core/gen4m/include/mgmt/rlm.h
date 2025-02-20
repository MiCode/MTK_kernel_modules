/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm.h"
 *    \brief
 */


#ifndef _RLM_H
#define _RLM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
extern u_int8_t g_bIcapEnable;
extern u_int8_t g_bCaptureDone;
extern uint16_t g_u2DumpIndex;
#if CFG_SUPPORT_QA_TOOL
extern uint32_t g_au4Offset[2][2];
extern uint32_t g_au4IQData[256];
#endif

#if (CFG_SUPPORT_802_11AX == 1)
extern uint8_t  g_fgSigmaCMDHt;
extern uint8_t  g_ucHtSMPSCapValue;
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define ELEM_EXT_CAP_DEFAULT_VAL \
	(ELEM_EXT_CAP_20_40_COEXIST_SUPPORT /*| ELEM_EXT_CAP_PSMP_CAP*/)

#if CFG_SUPPORT_RX_STBC
#define FIELD_HT_CAP_INFO_RX_STBC   HT_CAP_INFO_RX_STBC_1_SS
#else
#define FIELD_HT_CAP_INFO_RX_STBC   HT_CAP_INFO_RX_STBC_NO_SUPPORTED
#endif

#if CFG_SUPPORT_RX_SGI
#define FIELD_HT_CAP_INFO_SGI_20M   HT_CAP_INFO_SHORT_GI_20M
#define FIELD_HT_CAP_INFO_SGI_40M   HT_CAP_INFO_SHORT_GI_40M
#else
#define FIELD_HT_CAP_INFO_SGI_20M   0
#define FIELD_HT_CAP_INFO_SGI_40M   0
#endif

#if CFG_SUPPORT_RX_HT_GF
#define FIELD_HT_CAP_INFO_HT_GF     HT_CAP_INFO_HT_GF
#else
#define FIELD_HT_CAP_INFO_HT_GF     0
#endif

#define HT_CAP_INFO_DEFAULT_VAL \
	(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_DSSS_CCK_IN_40M \
		| HT_CAP_INFO_SM_POWER_SAVE)

#if (CFG_SUPPORT_CONNAC3X_SMALL_PKT == 1)
#define AMPDU_PARAM_DEFAULT_VAL \
	(AMPDU_PARAM_MAX_AMPDU_LEN_64K | AMPDU_PARAM_MSS_1_US)
#else
#define AMPDU_PARAM_DEFAULT_VAL \
	(AMPDU_PARAM_MAX_AMPDU_LEN_64K | AMPDU_PARAM_MSS_NO_RESTRICIT)
#endif

#define SUP_MCS_TX_DEFAULT_VAL \
	SUP_MCS_TX_SET_DEFINED	/* TX defined and TX/RX equal (TBD) */

#if CFG_SUPPORT_MFB
#define FIELD_HT_EXT_CAP_MFB    HT_EXT_CAP_MCS_FEEDBACK_BOTH
#else
#define FIELD_HT_EXT_CAP_MFB    HT_EXT_CAP_MCS_FEEDBACK_NO_FB
#endif

#if CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_RDR    HT_EXT_CAP_RD_RESPONDER
#else
#define FIELD_HT_EXT_CAP_RDR    0
#endif

#if CFG_SUPPORT_MFB || CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_HTC    HT_EXT_CAP_HTC_SUPPORT
#else
#define FIELD_HT_EXT_CAP_HTC    0
#endif

#define HT_EXT_CAP_DEFAULT_VAL \
	(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE | \
	 FIELD_HT_EXT_CAP_MFB | FIELD_HT_EXT_CAP_HTC | \
	 FIELD_HT_EXT_CAP_RDR)

#define TX_BEAMFORMING_CAP_DEFAULT_VAL        0

#if CFG_SUPPORT_BFEE
#define TX_BEAMFORMING_CAP_BFEE \
	(TXBF_RX_NDP_CAPABLE | \
	 TXBF_EXPLICIT_COMPRESSED_FEEDBACK_IMMEDIATE_CAPABLE | \
	 TXBF_MINIMAL_GROUPING_1_2_3_CAPABLE | \
	 TXBF_COMPRESSED_TX_ANTENNANUM_4_SUPPORTED | \
	 TXBF_CHANNEL_ESTIMATION_4STS_CAPABILITY)
#else
#define TX_BEAMFORMING_CAP_BFEE        0
#endif

#if CFG_SUPPORT_BFER
#define TX_BEAMFORMING_CAP_BFER \
	(TXBF_TX_NDP_CAPABLE | \
	 TXBF_EXPLICIT_COMPRESSED_TX_CAPAB)
#else
#define TX_BEAMFORMING_CAP_BFER        0
#endif

#define ASEL_CAP_DEFAULT_VAL                        0

/* Define bandwidth from user setting */
#define CONFIG_BW_20_40M            0
#define CONFIG_BW_20M               1	/* 20MHz only */

#define RLM_INVALID_POWER_LIMIT                     -127 /* dbm */

#define RLM_MAX_TX_PWR		20	/* dbm */
#define RLM_MIN_TX_PWR		8	/* dbm */

#if CFG_SUPPORT_BFER
#define MODE_HT 2
#define MODE_VHT 4
#if (CFG_SUPPORT_802_11AX == 1)
#define MODE_HE_SU 8
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#define MODE_EHT_SU 15
#endif

#endif

#if CFG_SUPPORT_802_11AC
#if CFG_SUPPORT_BFEE
#define FIELD_VHT_CAP_INFO_BFEE \
		(VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE)
#define VHT_CAP_INFO_BEAMFORMEE_STS_CAP_MAX	3
#else
#define FIELD_VHT_CAP_INFO_BFEE     0
#endif

#if CFG_SUPPORT_BFER
#define FIELD_VHT_CAP_INFO_BFER \
		(VHT_CAP_INFO_SU_BEAMFORMER_CAPABLE| \
		VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_2_SUPPORTED)
#else
#define FIELD_VHT_CAP_INFO_BFER     0
#endif

#define VHT_CAP_INFO_DEFAULT_VAL \
	(VHT_CAP_INFO_MAX_MPDU_LEN_3K | \
	 (AMPDU_PARAM_MAX_AMPDU_LEN_1024K \
		 << VHT_CAP_INFO_MAX_AMPDU_LENGTH_OFFSET))

#define VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE			0
#define VHT_CAP_INFO_EXT_NSS_BW_CAP				BIT(13)

#endif

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
#define TX_PWR_ENV_LMT_MIN                0 /* LSB = 0.5dBm */
#define TX_PWR_ENV_BW_SHIFT_BW20          0
#define TX_PWR_ENV_BW_SHIFT_BW40          2
#define TX_PWR_ENV_BW_SHIFT_BW80          6
#define TX_PWR_ENV_BW_SHIFT_BW160        14

/* PSD to Power dBm transfer func : 10*log(BW) * 2 */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW20    26 /* 10*log( 20) * 2  = 26, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW40    32 /* 10*log( 40) * 2  = 32, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW80    38 /* 10*log( 80) * 2  = 38, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW160   44 /* 10*log(160) * 2  = 44, 0.5dBm */

#define TX_PWR_ENV_INT8_MIN             -128
#define TX_PWR_ENV_INT8_MAX              127
#endif

#define TX_PWR_REG_LMT_MIN   5  /* LSB = 1 dBm */
#define TX_PWR_REG_LMT_MAX  30  /* LSB = 1 dBm */
#define TX_PWR_MAX          63  /* LSB = 1 dBm */
#define TX_PWR_MIN         -64  /* LSB = 1 dBm */
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
typedef void (*PFN_OPMODE_NOTIFY_DONE_FUNC)(
	struct ADAPTER *, uint8_t, bool);

enum ENUM_OP_NOTIFY_TYPE_T {
	OP_NOTIFY_TYPE_VHT_NSS_BW = 0,
	OP_NOTIFY_TYPE_HT_NSS,
	OP_NOTIFY_TYPE_HT_BW,
	OP_NOTIFY_TYPE_OMI_NSS_BW,
	OP_NOTIFY_TYPE_NUM
};

enum ENUM_OP_CHANGE_STATUS_T {
	OP_CHANGE_STATUS_INVALID = 0, /* input invalid */
	/* input valid, but no need to change */
	OP_CHANGE_STATUS_VALID_NO_CHANGE,
	/* process callback done before function return */
	OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE,
	/* wait next INT to call callback */
	OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_WAIT,
	OP_CHANGE_STATUS_NUM
};

enum ENUM_OP_CHANGE_SEND_ACT_T {
	/* Do not send action frame */
	OP_CHANGE_SEND_ACT_DISABLE = 0,
	/* Send action frame if change */
	OP_CHANGE_SEND_ACT_DEFAULT = 1,
	/* Send action frame w/wo change */
	OP_CHANGE_SEND_ACT_FORCE = 2,
	OP_CHANGE_SEND_ACT_NUM
};

struct SUB_ELEMENT_LIST {
	struct SUB_ELEMENT_LIST *prNext;
	struct SUB_ELEMENT rSubIE;
};

#if CFG_SUPPORT_DFS
enum ENUM_CHNL_SWITCH_MODE {
	MODE_ALLOW_TX,
	MODE_DISALLOW_TX,
	MODE_NUM
};

struct SWITCH_CH_AND_BAND_PARAMS {
	enum ENUM_BAND eCsaBand;
	uint8_t ucCsaNewCh;
	uint8_t ucCsaCount;
	uint8_t ucVhtS1;
	uint8_t ucVhtS2;
	uint8_t ucVhtBw;
	enum ENUM_CHNL_EXT eSco;
	uint8_t ucBssIndex;
	uint8_t fgHasStopTx;
	uint8_t fgIsCrossBand;
	enum ENUM_CHNL_SWITCH_MODE ucCsaMode;
	uint32_t u4MaxSwitchTime;
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

/* It is used for RLM module to judge if specific network is valid
 * Note: Ad-hoc mode of AIS is not included now. (TBD)
 */
#define RLM_NET_PARAM_VALID(_prBssInfo) \
	(IS_BSS_ACTIVE(_prBssInfo) && \
	 ((_prBssInfo)->eConnectionState == MEDIA_STATE_CONNECTED || \
	  (_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT || \
	  (_prBssInfo)->eCurrentOPMode == OP_MODE_IBSS || \
	  IS_BSS_BOW(_prBssInfo)) \
	)

#define RLM_NET_IS_11N(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11N)
#define RLM_NET_IS_11GN(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11GN)

#if CFG_SUPPORT_802_11AC
#define RLM_NET_IS_11AC(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
#endif
#if (CFG_SUPPORT_802_11AX == 1)
#define RLM_NET_IS_11AX(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#define RLM_NET_IS_11BE(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11BE)
#endif

#if CFG_SUPPORT_DFS
#define MAX_CSA_COUNT 255
#define HAS_CH_SWITCH_PARAMS(prCSAParams) (prCSAParams->ucCsaNewCh > 0)
#define HAS_SCO_PARAMS(prCSAParams) (prCSAParams->eSco > 0)
#define HAS_WIDE_BAND_PARAMS(prCSAParams) \
	(prCSAParams->ucVhtBw > 0 || \
	 prCSAParams->ucVhtS1 > 0 || \
	 prCSAParams->ucVhtS2 > 0)
#define SHOULD_CH_SWITCH(current, prCSAParams) \
	(HAS_CH_SWITCH_PARAMS(prCSAParams) && \
	 (current < prCSAParams->ucCsaCount))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void rlmFsmEventInit(struct ADAPTER *prAdapter);

void rlmFsmEventUninit(struct ADAPTER *prAdapter);

void rlmReqGenerateHtCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGeneratePowerCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGenerateSupportedChIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGenerateExtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateHtCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateExtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateHtOpIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo);

void rlmGeneratePwrConstraintIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateErpIE(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo);

uint8_t rlmCheckMtkOuiChipCap(uint8_t *pucIe, uint64_t u8ChipCap);

uint32_t rlmCalculateMTKOuiIELen(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void rlmGenerateMTKOuiIE(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo);
uint16_t rlmGenerateMTKChipCapIE(uint8_t *pucBuf, uint16_t u2FrameLength,
	uint8_t fgNeedOui, uint64_t u8ChipCap);

u_int8_t rlmParseCheckMTKOuiIE(struct ADAPTER *prAdapter,
			const uint8_t *pucBuf,  struct STA_RECORD *prStaRec);

#if CFG_SUPPORT_RXSMM_ALLOWLIST
u_int8_t rlmParseCheckRxsmmOuiIE(struct ADAPTER *prAdapter,
		const uint8_t *pucBuf, u_int8_t *pfgRxsmmEnable);
#endif
#if CFG_ENABLE_WIFI_DIRECT
uint32_t rlmCalculateCsaIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec);

void rlmGenerateCsaIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);
#endif
void rlmProcessBcn(struct ADAPTER *prAdapter,
		   struct SW_RFB *prSwRfb, uint8_t *pucIE,
		   uint16_t u2IELength);

void rlmProcessAssocRsp(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb, const uint8_t *pucIE,
			uint16_t u2IELength);

void rlmProcessHtAction(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb);

#if CFG_SUPPORT_NAN
uint32_t rlmFillNANVHTCapIE(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);
uint32_t rlmFillNANHTCapIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);
#endif

#if CFG_SUPPORT_802_11AC
void rlmProcessVhtAction(struct ADAPTER *prAdapter,
			 struct SW_RFB *prSwRfb);
#endif

void rlmFillSyncCmdParam(struct CMD_SET_BSS_RLM_PARAM
			 *prCmdBody, struct BSS_INFO *prBssInfo);

void rlmSyncOperationParams(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo);

void rlmSyncAntCtrl(struct ADAPTER *prAdapter, uint8_t txNss, uint8_t rxNss);

void rlmBssInitForAPandIbss(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo);

void rlmProcessAssocReq(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb, uint8_t *pucIE,
			uint16_t u2IELength);

void rlmBssAborted(struct ADAPTER *prAdapter,
		   struct BSS_INFO *prBssInfo);

#if CFG_SUPPORT_TDLS
uint32_t
rlmFillHtCapIEByParams(u_int8_t fg40mAllowed,
		       u_int8_t fgShortGIDisabled,
		       uint8_t u8SupportRxSgi20,
		       uint8_t u8SupportRxSgi40, uint8_t u8SupportRxGf,
		       enum ENUM_OP_MODE eCurrentOPMode, uint8_t *pOutBuf);

uint32_t rlmFillHtCapIEByAdapter(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);

uint32_t rlmFillVhtCapIEByAdapter(struct ADAPTER *prAdapter,
				  struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);

#endif

#if CFG_SUPPORT_802_11AC
void rlmReqGenerateVhtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtOpIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmFillVhtOpIE(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prBssInfo, struct MSDU_INFO *prMsduInfo);

void rlmGenerateVhtTPEIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtOpNotificationIE(struct ADAPTER
			       *prAdapter, struct MSDU_INFO *prMsduInfo);
void rlmReqGenerateVhtOpNotificationIE(struct ADAPTER
			       *prAdapter, struct MSDU_INFO *prMsduInfo);




#endif
#if CFG_SUPPORT_802_11D
void rlmGenerateCountryIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo);
#endif
#if CFG_SUPPORT_DFS
void rlmProcessExCsaIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams,
	uint8_t ucChannelSwitchMode, uint8_t ucNewOperatingClass,
	uint8_t ucNewChannelNum, uint8_t ucChannelSwitchCount);

void rlmProcessSpecMgtAction(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb);

void rlmProcessPublicAction(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb);

void rlmResetCSAParams(struct BSS_INFO *prBssInfo, uint8_t fgClearAll);

void rlmCsaTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr);

void rlmCsaDoneTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr);
#endif

uint32_t rlmUpdateStbcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable, uint8_t notify);

uint32_t rlmUpdateMrcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable);

uint32_t
rlmSendOpModeFrameByType(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucOpChangeType,
	uint8_t ucChannelWidth,
	uint8_t ucRxNss, uint8_t ucTxNss);

uint32_t
rlmSendOpModeNotificationFrame(struct ADAPTER *prAdapter,
			       struct STA_RECORD *prStaRec,
			       uint8_t ucChannelWidth, uint8_t ucNss);

uint32_t
rlmSendSmPowerSaveFrame(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec, uint8_t ucNss);

uint32_t
rlmSendNotifyChannelWidthFrame(
		struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		uint8_t ucChannelWidth);

#if (CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1)
uint32_t
rlmSendOMIDataFrame(struct ADAPTER *prAdapter,
		    struct STA_RECORD *prStaRec,
		    uint8_t ucChannelWidth,
		    uint8_t ucOpRxNss,
		    uint8_t ucOpTxNss,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler);
#endif

void rlmReqGenerateOMIIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

void
rlmSendChannelSwitchFrame(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

uint16_t
rlmOpClassToBandwidth(uint8_t ucOpClass);

uint32_t
rlmNotifyVhtOpModeTxDone(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyOMIOpModeTxDone(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyApGoOmiOpModeTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmSmPowerSaveTxDone(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo,
		     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyChannelWidthtTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint8_t
rlmGetBssOpBwByVhtAndHtOpInfo(struct BSS_INFO *prBssInfo);

uint8_t
rlmGetBssOpBwByOwnAndPeerCapability(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

uint8_t rlmGetBssOpBwByChannelWidth(enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChannelWidth);

uint8_t
rlmGetVhtOpBwByBssOpBw(uint8_t ucBssOpBw);

uint8_t rlmGetVhtOpBw320ByS1(uint8_t ucS1);

void
rlmFillVhtOpInfoByBssOpBw(struct ADAPTER *prAdapter,
			  struct BSS_INFO *prBssInfo,
			  uint8_t ucChannelWidth);

enum ENUM_OP_CHANGE_STATUS_T
rlmChangeOperationMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucChannelWidth,
	uint8_t ucOpRxNss,
	uint8_t ucOpTxNss,
	enum ENUM_OP_CHANGE_SEND_ACT_T ucSendAct,
	PFN_OPMODE_NOTIFY_DONE_FUNC pfOpChangeHandler
);

void
rlmDummyChangeOpHandler(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, bool fgIsChangeSuccess);

#if CFG_SUPPORT_BFER
void
rlmBfStaRecPfmuUpdate(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec);

void
rlmETxBfTriggerPeriodicSounding(struct ADAPTER *prAdapter);

bool
rlmClientSupportsVhtETxBF(struct STA_RECORD *prStaRec);

uint8_t
rlmClientSupportsVhtBfeeStsCap(struct STA_RECORD *prStaRec);

bool
rlmClientSupportsHtETxBF(struct STA_RECORD *prStaRec);
#endif

void rlmModifyVhtBwPara(uint8_t *pucVhtChannelFrequencyS1,
			uint8_t *pucVhtChannelFrequencyS2,
			uint8_t ucHtChannelFrequencyS3,
			uint8_t *pucVhtChannelWidth);

#if (CFG_SUPPORT_WIFI_6G == 1)
void rlmTransferHe6gOpInfor(struct ADAPTER *prAdapter,
	uint8_t ucChannelNum,
	uint8_t ucChannelWidth,
	uint8_t *pucChannelWidth,
	uint8_t *pucCenterFreqS1,
	uint8_t *pucCenterFreqS2,
	enum ENUM_CHNL_EXT *peSco);

void rlmModifyHE6GBwPara(struct ADAPTER *prAdapter,
	uint8_t ucHe6gChannelWidth,
	uint8_t ucHe6gPrimaryChannel,
	uint8_t *pucHe6gChannelFrequencyS1,
	uint8_t *pucHe6gChannelFrequencyS2);
#endif

void rlmReviseMaxBw(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_CHNL_EXT *peExtend,
	enum ENUM_CHANNEL_WIDTH *peChannelWidth,
	uint8_t *pucS1,
	uint8_t *pucPrimaryCh);

enum ENUM_CHNL_EXT rlmReviseSco(
	enum ENUM_CHANNEL_WIDTH eChannelWidth,
	uint8_t ucPrimaryCh,
	uint8_t ucS1,
	enum ENUM_CHNL_EXT eScoOrigin,
	uint8_t ucMaxBandwidth);

void rlmRevisePreferBandwidthNss(struct ADAPTER *prAdapter,
					uint8_t ucBssIndex,
					struct STA_RECORD *prStaRec);

void rlmSetMaxTxPwrLimit(struct ADAPTER *prAdapter, int8_t cLimit,
			 uint8_t ucEnable);

void rlmSyncExtCapIEwithSupplicant(uint8_t *aucCapabilities,
	const uint8_t *supExtCapIEs, size_t IElen);

int32_t rlmGetOpClassForChannel(int32_t channel,
	enum ENUM_BAND band, enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChBw, uint16_t u2Country);

#if (CFG_SUPPORT_802_11AX == 1)
void rlmSetSrControl(struct ADAPTER *prAdapter, bool fgIsEnableSr);
#endif

#if CFG_AP_80211K_SUPPORT
void rlmMulAPAgentGenerateApRRMEnabledCapIE(
				struct ADAPTER *prAdapter,
				struct MSDU_INFO *prMsduInfo);
void rlmMulAPAgentTxMeasurementRequest(
				struct ADAPTER *prAdapter,
				struct STA_RECORD *prStaRec,
				struct SUB_ELEMENT_LIST *prSubIEs);

void rlmMulAPAgentProcessRadioMeasurementResponse(
		struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);
#endif /* CFG_AP_80211K_SUPPORT */

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief Init Transmit Power Envelope max TxPower limit to max TxPower
 *
 * \param[in] picMaxTxPwr : Pointer of max TxPower limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrInit(int8_t *picMaxTxPwr);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use send Tranmit Power Envelope max TxPower limit to FW
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] eBand : RF Band index
 * \param[in] ucPriCh : Primary Channel
 * \param[in] picTxPwrEnvMaxPwr : Pointer to Tranmit Power Envelope max TxPower
 *                                limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrSend(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucPwrLmtNum,
	int8_t *picTxPwrEnvMaxPwr,
	uint8_t fgPwrLmtEnable);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update TxPwr Envelope if need.
 *        1. It will translate IE content if the content is represent as PSD
 *        2. Update the TxPwr Envelope TxPower limit to BSS_DESC if the IE
 *           content is smaller than the current exit setting.
 *        3. If the TxPower limit have update, it will send cmd to FW to reset
 *           TxPower limit
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prTxPwrEnvIE : Pointer of TxPwer Envelope IE
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmTxPwrEnvMaxPwrUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE);
#endif /* CFG_SUPPORT_TX_PWR_ENV */

#if CFG_SUPPORT_802_11K
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update Regulatory TxPower limit if need.
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prCountryIE : Pointer of Country IE
 * \param[in] ucPowerConstraint : TxPower constraint value from Power
 *                                Constrait IE
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmRegTxPwrLimitUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_COUNTRY *prCountryIE,
	uint8_t ucPowerConstraint);
#endif /* CFG_SUPPORT_802_11K */

uint32_t rlmCalculateTpeIELen(struct ADAPTER *prAdapter,
			      uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void rlmGenerateTpeIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

enum ENUM_MAX_BANDWIDTH_SETTING
rlmVhtBw2Bw(uint8_t ucVhtBw, enum ENUM_CHNL_EXT eSco);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#ifndef _lint
static __KAL_INLINE__ void rlmDataTypeCheck(void)
{
}
#endif /* _lint */

#endif /* _RLM_H */
