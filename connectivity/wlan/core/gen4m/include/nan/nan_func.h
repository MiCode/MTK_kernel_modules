/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_FUNC_H
#define _NAN_FUNC_H

void
nanSetFlashCommunication(struct ADAPTER *prAdapter, u_int8_t fgEnable);

void
nanExtEnableReq(struct ADAPTER *prAdapter);

void
nanExtDisableReq(struct ADAPTER *prAdapter);

void
nanExtClearCustomNdpFaw(uint8_t ucIndex);

void
nanPeerReportEhtEvent(struct ADAPTER *prAdapter,
					 uint8_t enable);

void
nanEnableEhtMode(struct ADAPTER *prAdapter, uint8_t mode);

void
nanEnableEht(struct ADAPTER *prAdapter, uint8_t enable);

void
nanExtBackToNormal(struct ADAPTER *prAdapter);

void
nanExtResetNdlConfig(struct _NAN_NDL_INSTANCE_T *prNDL);

struct _NAN_NDL_INSTANCE_T *
nanExtGetReusedNdl(struct ADAPTER *prAdapter);

u32
wlanoidNANExtCmd(struct ADAPTER *prAdapter, void *pvSetBuffer,
		     uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

u32
wlanoidNANExtCmdRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
			uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen);

void
nanExtComposeBeaconTrack(struct ADAPTER *prAdapter,
				struct _NAN_EVENT_REPORT_BEACON *prFwEvt);

void
nanExtComposeClusterEvent(struct ADAPTER *prAdapter,
			       struct NAN_DE_EVENT *prDeEvt);

uint32_t
nanSchedGetVendorEhtAttr(struct ADAPTER *prAdapter,
				  uint8_t **ppucVendorAttr,
				  uint32_t *pu4VendorAttrLength);

uint32_t
nanSchedGetVendorAttr(struct ADAPTER *prAdapter,
			       uint8_t **ppucVendorAttr,
			       uint32_t *pu4VendorAttrLength);

uint16_t
nanDataEngineVendorAttrLength(struct ADAPTER *prAdapter,
				 struct _NAN_NDL_INSTANCE_T *prNDL,
				 struct _NAN_NDP_INSTANCE_T *prNDP);

void
nanDataEngineVendorAttrAppend(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      struct _NAN_NDL_INSTANCE_T *prNDL,
				      struct _NAN_NDP_INSTANCE_T *prNDP);

uint16_t
nanDataEngineVendorEhtAttrLength(struct ADAPTER *prAdapter,
				 struct _NAN_NDL_INSTANCE_T *prNDL,
				 struct _NAN_NDP_INSTANCE_T *prNDP);

void
nanDataEngineVendorEhtAttrAppend(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      struct _NAN_NDL_INSTANCE_T *prNDL,
				      struct _NAN_NDP_INSTANCE_T *prNDP);

uint32_t
nanGetFcSlots(struct ADAPTER *prAdapter);

uint32_t
nanGetTimelineFcSlots(struct ADAPTER *prAdapter, size_t szTimelineIdx,
			       size_t szSlotIdx);

u_int8_t
nanIsChnlSwitchSlot(struct ADAPTER *prAdapter,
			     unsigned char fgPrintLog,
			     size_t szTimelineIdx,
			     size_t szSlotIdx);

void
nanExtTerminateApNan(struct ADAPTER *prAdapter, uint8_t ucReason);

void
nanExtTerminateApNanEndPs(struct ADAPTER *prAdapter);

void
nanExtTerminateApNanEndLegacy(struct ADAPTER *prAdapter);

void
nanExtRxAuthHandler(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

void
nanExtAisConnectHandler(struct ADAPTER *prAdapter);

uint32_t
nanExtSetCountryCodeHandler(struct ADAPTER *prAdapter);

uint32_t
nanExtAisAssocDoneHandler(
		struct ADAPTER *prAdapter,
		uint32_t rJoinStatus,
		struct STA_RECORD *prStaRec);

uint32_t
nanExtScanStartHandler(
		struct ADAPTER *prAdapter,
		struct cfg80211_scan_request *request);

uint32_t
nanExtAisChangeHandler(
		struct ADAPTER *prAdapter);

uint32_t
nanExtScanCompleteHandler(
		struct ADAPTER *prAdapter,
		uint8_t ucStatus);

uint32_t
nanExtUniEventHandler(struct ADAPTER *prAdapter,
		uint32_t u4SubEvent,
		uint8_t *pucBuf);

uint32_t
nanExtEventHandler(struct ADAPTER *prAdapter,
		uint32_t u4SubEvent,
		uint8_t *pucBuf);

uint32_t
nanExtProcessRsvdFrame(struct ADAPTER *prAdapter,
		 struct SW_RFB *prSwRfb);

uint32_t
nanExtAisAssocStartHandler(
		struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		uint8_t ucChannelNum,
		uint8_t ucChnlBw,
		enum ENUM_CHNL_EXT eSco);

uint32_t
nanExtRxAssocHandler(
		struct ADAPTER *prAdapter,
		uint8_t *buf);
#endif /* _NAN_FUNC_H */
