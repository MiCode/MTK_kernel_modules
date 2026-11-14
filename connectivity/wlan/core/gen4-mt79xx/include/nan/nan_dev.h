 /* SPDX-License-Identifier: BSD-2-Clause */
 /*
  * Copyright (c) 2021 MediaTek Inc.
  */

#if CFG_SUPPORT_NAN

enum ENUM_MODULE {
	ENUM_NAN_DATA_MODULE,
	ENUM_NAN_RANGE_MODULE,
	ENUM_NAN_MODULE_NUM
};

enum ENUM_NAN_DISC_BCN_TYPE {
	ENUM_DISC_BCN_PERIOD = 0,
	ENUM_DISC_BCN_SLOT
};

struct _NAN_CMD_EVENT_SET_DISC_BCN_T {
	uint8_t ucDiscBcnType;
	uint8_t ucDiscBcnPeriod;
	uint8_t aucReserved[2];
	struct _NAN_SCHEDULE_TIMELINE_T
		rDiscBcnTimeline[NAN_TIMELINE_MGMT_SIZE];
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

uint8_t nanBssInit(IN struct ADAPTER *prAdapter, uint8_t ucIdx);

void nanDevFsmUninit(IN struct ADAPTER *prAdapter);
void nanDevDumpBssStatus(struct ADAPTER *prAdapter);
void nanDevBssActivate(struct ADAPTER *prAdapter);
void nanDevBssDeactivate(IN struct ADAPTER *prAdapter, bool fgFreeBss);
void nanDevEnable(struct ADAPTER *prAdapter);
void nanDevDisable(struct ADAPTER *prAdapter);

struct _NAN_SPECIFIC_BSS_INFO_T *
nanGetSpecificBssInfo(IN struct ADAPTER *prAdapter,
		      enum NAN_BSS_ROLE_INDEX eIndex);
uint8_t
nanGetBssIdxbyBand(IN struct ADAPTER *prAdapter,
		      enum ENUM_BAND eBand);
void
nanDevCommonSetCb(IN struct ADAPTER *prAdapter, IN struct CMD_INFO *prCmdInfo,
		  IN uint8_t *pucEventBuf);
u_int8_t
nanIsRegistered(struct ADAPTER *prAdapter);

void nanDevSetMasterPreference(IN struct ADAPTER *prAdapter,
			       uint8_t ucMasterPreference);

enum NanStatusType
nanDevSetDiscBcn(IN struct ADAPTER *prAdapter,
		 struct _NAN_CMD_EVENT_SET_DISC_BCN_T *prNanSetDiscBcn);
void
nanDevDiscBcnPeriodEvtHandler(IN struct ADAPTER *prAdapter,
			      IN uint8_t *pcuEvtBuf);

enum NanStatusType nanDevEnableRequest(IN struct ADAPTER *prAdapter,
				       struct NanEnableRequest *prEnableReq);
enum NanStatusType nanDevDisableRequest(IN struct ADAPTER *prAdapter);

void nanDevUpdateBss(IN struct ADAPTER *prAdapter, int u4Idx);
enum NanStatusType nanDevSetNmiAddress(IN struct ADAPTER *prAdapter,
					   uint8_t *macAddress);
enum NanStatusType nanDevSetNdiAddress(IN struct ADAPTER *prAdapter,
					   uint8_t *macAddress);
void nanDevGetNdiAddress(struct ADAPTER *prAdapter,
		uint8_t ucNdiIdx, uint8_t *pucNdiAddr);
uint8_t nanGetBssIdxByRole(IN struct ADAPTER *prAdapter, size_t szRole);

void nanDevMasterIndEvtHandler(IN struct ADAPTER *prAdapter,
			       IN uint8_t *pcuEvtBuf);
uint32_t nanDevGetMasterIndAttr(IN struct ADAPTER *prAdapter,
				uint8_t *pucMasterIndAttrBuf,
				uint32_t *pu4MasterIndAttrLength);
void nanDevClusterIdEvtHandler(IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf);
uint32_t nanDevGetClusterId(IN struct ADAPTER *prAdapter,
		uint8_t *pucClusterId);
uint32_t nanDevSendEnableRequestToCnm(IN struct ADAPTER *prAdapter);
uint32_t nanDevSendAbortRequestToCnm(IN struct ADAPTER *prAdapter);
void nanDevSendEnableRequest(struct ADAPTER *prAdapter,
				struct MSG_HDR *prMsgHdr);
void nanDevSetConfig(IN struct ADAPTER *prAdapter);
void nanDevSetOwnMacIdx(struct ADAPTER *prAdapter, uint8_t *ucOwnMacIdx);
uint32_t nanDevSetSigmaConfig(IN struct ADAPTER *prAdapter);
uint32_t
nanDevGetDeviceInfo(IN struct ADAPTER *prAdapter,
		    IN void *pvQueryBuffer, IN uint32_t u4QueryBufferLen,
		    OUT uint32_t *pu4QueryInfoLen);
void nanDevEventQueryDeviceInfo(IN struct ADAPTER
			*prAdapter, IN struct CMD_INFO *prCmdInfo,
			IN uint8_t *pucEventBuf);
void
nanDevCommonSetCb(IN struct ADAPTER *prAdapter, IN struct CMD_INFO *prCmdInfo,
		  IN uint8_t *pucEventBuf);
void
nanDevGetDeviceCapabilities(struct ADAPTER *prAdapter,
	struct NanDeviceCapabilities *prNanDvcCapabilities);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
enum NanStatusType nanDevEnableGtkReqChecker(struct ADAPTER *prAdapter,
			uint8_t ucNdiIdx, uint8_t fgEnable);
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
#endif
