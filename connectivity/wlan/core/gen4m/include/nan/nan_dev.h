/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __NAN_DEV_H__
#define __NAN_DEV_H__

#if CFG_SUPPORT_NAN

#define NAN_DEFAULT_INDEX (0)

enum ENUM_MODULE {
	ENUM_NAN_DATA_MODULE,
	ENUM_NAN_RANGE_MODULE,
	ENUM_NAN_MODULE_NUM
};

extern uint8_t g_ucNanWmmQueIdx;

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint8_t nanDevInit(struct ADAPTER *prAdapter, uint8_t ucIdx);

void nanDevFsmUninit(struct ADAPTER *prAdapter, uint8_t ucIdx);
struct _NAN_SPECIFIC_BSS_INFO_T *
nanGetSpecificBssInfo(struct ADAPTER *prAdapter,
		      uint8_t eIndex);
uint8_t
nanGetBssIdxbyBand(struct ADAPTER *prAdapter,
		      enum ENUM_BAND eBand);

void nanDevSetMasterPreference(struct ADAPTER *prAdapter,
			       uint8_t ucMasterPreference);

enum NanStatusType nanDevEnableRequest(struct ADAPTER *prAdapter,
				       struct NanEnableRequest *prEnableReq);
enum NanStatusType nanDevDisableRequest(struct ADAPTER *prAdapter);
void nanDevMasterIndEvtHandler(struct ADAPTER *prAdapter,
			       uint8_t *pcuEvtBuf);
uint32_t nanDevGetMasterIndAttr(struct ADAPTER *prAdapter,
				uint8_t *pucMasterIndAttrBuf,
				uint32_t *pu4MasterIndAttrLength);
void nanDevClusterIdEvtHandler(struct ADAPTER *prAdapter,
		uint8_t *pcuEvtBuf);
uint32_t nanDevGetClusterId(struct ADAPTER *prAdapter,
		uint8_t *pucClusterId);
uint32_t nanDevSendEnableRequestToCnm(struct ADAPTER *prAdapter);
uint32_t nanDevSendAbortRequestToCnm(struct ADAPTER *prAdapter);
void nanDevSendEnableRequest(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);
/*========================= FUNCTIONs ============================*/
#endif
#endif /* __NAN_DEV_H__ */
