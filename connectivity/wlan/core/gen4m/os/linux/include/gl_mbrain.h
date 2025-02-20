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

#define MBR_TXTIMEOUT_QUE_CNT_MAX	50
#define MBR_TXTIMEOUT_INTERVAL		20000 /* 20s*/

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
 * u4ExpdLen: expected data length
 * pfnHandler: pointer to the function which should copy to the buffer
 * pfnGetDataNum: pointer to the function which should
 *                specify the amount of valid data
 */
struct wifi2mbr_handler {
	enum wifi2mbr_tag eTag;
	uint32_t u4ExpdLen;
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
};

struct MBRAIN_TXTIMEOUT_ENTRY {
	struct QUE_ENTRY rQueEntry;
	struct wifi2mbr_TxTimeoutInfo rTxTimeoutInfo;
};

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


/* get tag total data num */
uint16_t mbr_wifi_lls_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

uint16_t mbr_wifi_lp_get_total_data_num(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

uint16_t mbrWifiTxTimeoutGetTotalDataNum(
	struct ADAPTER *prAdapter, enum wifi2mbr_tag eTag);

void mmbrTxTimeoutEnqueue(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, struct timespec64 rTimeoutTs,
	uint32_t u4AvgIdleSlot);

struct MBRAIN_TXTIMEOUT_ENTRY *mbrTxTimeoutDequeue(struct ADAPTER *prAdapter);

void mbrIsTxTimeout(struct ADAPTER *prAdapter,
	uint32_t u4TokenId, uint32_t u4TxTimeoutDuration);

#endif /* CFG_SUPPORT_MBRAIN */
#endif /* _GL_MBRAIN_H */
