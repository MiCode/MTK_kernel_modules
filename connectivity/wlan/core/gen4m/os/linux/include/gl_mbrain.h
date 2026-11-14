/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_MBRAIN_H
#define _GL_MBRAIN_H

#if CFG_SUPPORT_MBRAIN
#include "bridge/mbraink_bridge.h"
#include "gl_kal.h"

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MBR_TRX_PERF_QUE_CNT_MAX	200
#define MBR_TRX_PERF_TIMEOUT_INTERVAL	5000 /* 5s*/

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
#define TXPWR_MBRAIN_ANT_NUM 2
#endif
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*
 * this macro is used to get specific field from mbrain emi
 * ex. GET_MBR_EMI_FIELD(prAdapter, rStatus, u4Mbr_test2, val);
 * caller should check rStatus:
 *  WLAN_STATUS_SUCCESS: read emi success
 *  WLAN_STATUS_FAILURE: read emi fail
 */
#define GET_MBR_EMI_FIELD(_prAdapter, _status, _field, _ele) \
	do { \
		if (!_prAdapter->prMbrEmiData) { \
			_status = WLAN_STATUS_FAILURE; \
			break; \
		} \
		kalMemCopy(&(_ele), &(_prAdapter->prMbrEmiData->_field), \
			sizeof(_prAdapter->prMbrEmiData->_field)); \
		_status = WLAN_STATUS_SUCCESS; \
	} while (0)

/*
 * this macro is used to set specific field to mbrain emi
 * ex. SET_MBR_EMI_FIELD(prAdapter, rStatus, u4Mbr_test2, &u4SetVal,
 *						 sizeof(u4SetVal));
 * caller should check _status:
 *  WLAN_STATUS_SUCCESS: copy to emi success
 *  WLAN_STATUS_FAILURE: copy to emi fail, might be emi init fail
 */
#define SET_MBR_EMI_FIELD(_prAdapter, _status, _field, _src, _len) \
	do { \
		void *ptr = (_prAdapter->prMbrEmiData ? \
			&(_prAdapter->prMbrEmiData->_field) : NULL); \
		if (!ptr || _len > sizeof(_prAdapter->prMbrEmiData->_field)) { \
			_status = WLAN_STATUS_FAILURE; \
			break; \
		} \
		DBGLOG_LIMITED(REQ, LOUD, \
			"MBR update ptr: 0x%p len:%u caller:%pS\n", \
			ptr, _len, KAL_TRACE); \
		kalMemCopy(ptr, _src, _len); \
		_status = WLAN_STATUS_SUCCESS; \
	} while (0)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/* function pointer to copy data to mbrain */
typedef enum wifi2mbr_status (*PFN_WIFI2MBR_HANDLER) (struct ADAPTER*,
	enum wifi2mbr_tag, uint16_t, void *, uint16_t *);

/* function pointer to specify the amount of valid data */
typedef uint16_t (*PFN_WIFI2MBR_DATA_NUM) (struct ADAPTER*,
	enum wifi2mbr_tag);

/*
 * eTag: data tag
 * ucExpdLen: expected data length
 * pfnHandler: pointer to the function which should copy to the buffer
 * pfnGetDataNum: pointer to the function which should
 *                specify the amount of valid data
 */
struct wifi2mbr_handler {
	enum wifi2mbr_tag eTag;
	uint16_t ucExpdLen;
	PFN_WIFI2MBR_HANDLER pfnHandler;
	PFN_WIFI2MBR_DATA_NUM pfnGetDataNum;
};


#if CFG_SUPPORT_WIFI_ICCM
struct ICCM_POWER_STATE_T {
	uint32_t u4TxTime;
	uint32_t u4RxTime;
	uint32_t u4RxListenTime;
	uint32_t u4SleepTime;
};

struct ICCM_T {
	uint32_t u4TotalTime;
	struct ICCM_POWER_STATE_T u4BandRatio[5];
};
#endif /* CFG_SUPPORT_WIFI_ICCM */

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
struct TXPWR_MBRAIN_RPT_COEX_INFO_T {
	bool fgBtOn;
	bool fgLteOn;
	uint8_t ucReserved[2];
	uint32_t u4BtProfile;
	uint32_t u4PtaGrant;
	uint32_t u4PtaReq;
	uint32_t u4CurrOpMode;
};

struct TXPWR_MBRAIN_RPT_D_DIE_INFO_T {
	int32_t i4Delta;
	int8_t icTargetPwr;
	uint8_t ucCompGrp;
	uint8_t ucFeGainMode;
	uint8_t ucReserved[5];
};

struct TXPWR_MBRAIN_RPT_INFO_T {
	bool fgEpaSupport;
	uint8_t ucCalTpye;
	uint8_t ucCenterCh;
	uint8_t ucMccIdx;
	uint32_t u4RfBand;
	int32_t i4Temp;
	uint32_t u4Antsel;
	struct TXPWR_MBRAIN_RPT_COEX_INFO_T rCoex;
	struct TXPWR_MBRAIN_RPT_D_DIE_INFO_T rDdieInfo;
};

struct TXPWR_MBRAIN_RPT_T {
	uint8_t u1Ver;
	uint8_t ucRptType;
	uint8_t ucMaxBnNum;
	uint8_t ucMaxAntNum;
	struct TXPWR_MBRAIN_RPT_INFO_T
		arInfo[ENUM_BAND_NUM][TXPWR_MBRAIN_ANT_NUM];
};
#endif

#if CFG_SUPPORT_PCIE_MBRAIN
struct PCIE_T {
	uint32_t u4UpdateTimeUtcSec;
	uint32_t u4UpdateTimeUtcUsec;
	uint32_t u4ReqRecoveryCount;
	uint32_t u4L0TimeS;
	uint32_t u4L0TimeUs;
	uint32_t u4L1TimeS;
	uint32_t u4L1TimeUs;
	uint32_t u4L1ssTimeS;
	uint32_t u4L1ssTimeUs;
};
#endif /* CFG_SUPPORT_PCIE_MBRAIN */

struct mbrain_emi_data {
	/*
	 * this struct should be the same as the struct defined in fw
	 * example:
	 * uint32_t u4Mbr_test;
	 * uint32_t u4Mbr_test2;
	 */
#if CFG_SUPPORT_WIFI_ICCM
	struct ICCM_T rMbrIccmData;
#endif /* CFG_SUPPORT_WIFI_ICCM */
#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
	struct TXPWR_MBRAIN_RPT_T rMbrTxPwrRpt;
#endif /* CFG_SUPPORT_MBRAIN_TXPWR_RPT */
#if CFG_SUPPORT_PCIE_MBRAIN
	struct PCIE_T rMbrPcieData;
#endif /* CFG_SUPPORT_PCIE_MBRAIN */
#if CFG_SUPPORT_MBRAIN_BIGDATA
	struct BIG_DATA_BSS_CNT_T arBssStatCnt[MAX_BSSID_NUM];
	struct BIG_DATA_ABT_CNT arAbtCnt[ENUM_BAND_NUM];
	struct BIG_DATA_PHY_CNT arPhyCnt[ENUM_BAND_NUM];
	struct BIG_DATA_STA_INFO arStaInfo[BIG_DATA_MAX_STA_NUM];
#endif /* CFG_SUPPORT_MBRAIN_BIGDATA */
};

#if CFG_SUPPORT_MBRAIN_TRX_PERF
struct MBRAIN_TRXPERF_ENTRY {
	struct QUE_ENTRY rQueEntry;
	struct wifi2mbr_TRxPerfInfo rTRxPerfInfo;
};
#endif /* CFG_SUPPORT_MBRAIN_TRX_PERF */

#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
struct MBR_WIFI_WKUPRSN_ENTRY {
	struct QUE_ENTRY rQueEntry;
	struct wifi2mbr_WiFiWkUpRsnInfo rWiFiWkUpRsnInfo;
};

enum MBR_WIFI_WKUPRSN_ACTION {
	MBR_WKUP_RSN_UPDATE_RESUME_TIME,
	MBR_WKUP_RSN_UPDATE_SUSPEND_TIME,
	MBR_WKUP_RSN_UPDATE_WIFI_WKUP_TIME,
};
#endif
/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void glRegCbsToMbraink(struct ADAPTER *prAdapter);
void glUnregCbsToMbraink(void);

enum wifi2mbr_status mbraink2wifi_get_data(void *priv,
						enum mbr2wifi_reason reason,
						enum wifi2mbr_tag tag,
						void *buf,
						uint16_t *pu2Len);

/* tag handler */
enum wifi2mbr_status mbr_wifi_lls_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

enum wifi2mbr_status mbr_wifi_lp_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

enum wifi2mbr_status mbrWifiTxTimeoutHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

enum wifi2mbr_status mbr_wifi_txpwr_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

enum wifi2mbr_status mbrWifiPcieHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

/* get tag total data num */
uint16_t mbr_wifi_lls_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

uint16_t mbr_wifi_lp_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

uint16_t mbr_wifi_txpwr_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

#if (CFG_SUPPORT_MBRAIN_WIFI_WKUP_HOST == 1)
enum wifi2mbr_status mbr_wifi_wkup_rsn_handler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);
uint16_t mbr_wifi_wkup_rsn_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);
void mbr_wifi_wkup_rsn_action(struct ADAPTER *prAdapter,
			      enum MBR_WIFI_WKUPRSN_ACTION eAction,
			      enum enum_mbr_wifi_wkup_reason eReason,
			      uint32_t u4WkUpInfo);
void mbr_wifi_wkup_rsn_clear_queue(struct ADAPTER *prAdapter);
#endif

uint16_t mbrWifiPcieGetTotalDataNum(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

void mbrIsTxTimeout(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, uint32_t u4TxTimeoutDuration);

#if CFG_SUPPORT_MBRAIN_TRX_PERF
void mbrTRxPerfEnqueue(struct ADAPTER *prAdapter);

struct MBRAIN_TRXPERF_ENTRY *mbrTRxPerfDequeue(struct ADAPTER *prAdapter);

enum wifi2mbr_status mbrWifiTRxPerfHandler(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag, uint16_t u2CurLoopIdx,
	void *buf, uint16_t *pu2Len);

uint16_t mbrWifiTRxPerfGetTotalDataNum(struct ADAPTER *prAdapter,
	enum wifi2mbr_tag eTag);

void mbrWifiTRxPerfFlushQueue(struct ADAPTER *prAdapter);

#endif /* CFG_SUPPORT_MBRAIN_TRX_PERF */

#if CFG_SUPPORT_MBRAIN_TXPWR_RPT
enum wifi2mbr_status mbr_wifi_txpwr_uni_event_handler(struct ADAPTER *prAdapter,
	struct TXPWR_MBRAIN_RPT_T *prMbrRpt
);
#endif

#endif /* CFG_SUPPORT_MBRAIN */
#endif /* _GL_MBRAIN_H */
