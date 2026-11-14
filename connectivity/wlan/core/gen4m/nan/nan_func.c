// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nan_func.c"
 *  \brief
 */

#if (CFG_SUPPORT_NAN == 1)

#include "precomp.h"
#include "nan/nan_func.h"
#include "nan_sec.h"

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

void __weak
nanSetFlashCommunication(struct ADAPTER *prAdapter, u_int8_t fgEnable)
{
	DBGLOG(NAN, TRACE, "Set Flash Communication %u\n", fgEnable);
	/* TODO:
	 * 1. Set 2.4GHz/5GHz bitmap respectively
	 * 2. Call reconfigure function to update customized timeline
	 * 3. Send command to FW to set the bitmap, reuse Instant communication?
	 */
}

void __weak
nanExtEnableReq(struct ADAPTER *prAdapter)
{
	nanSetFlashCommunication(prAdapter, TRUE);
	nanInstantCommModeOnHandler(prAdapter);
}

void __weak
nanExtDisableReq(struct ADAPTER *prAdapter)
{
}

void __weak
nanExtClearCustomNdpFaw(uint8_t ucIndex)
{
}

void __weak
nanPeerReportEhtEvent(struct ADAPTER *prAdapter,
					 uint8_t enable)
{
}

void __weak
nanEnableEhtMode(struct ADAPTER *prAdapter, uint8_t mode)
{
}

void __weak nanEnableEht(struct ADAPTER *prAdapter, uint8_t enable)
{
}

u_int8_t __weak
nanExtHoldNdl(struct _NAN_NDL_INSTANCE_T *prNDL)
{
	return FALSE;
}

void __weak
nanExtBackToNormal(struct ADAPTER *prAdapter)
{
	nanInstantCommModeBackToNormal(prAdapter);
}

void __weak
nanExtResetNdlConfig(struct _NAN_NDL_INSTANCE_T *prNDL)
{
}

struct _NAN_NDL_INSTANCE_T * __weak
nanExtGetReusedNdl(struct ADAPTER *prAdapter)
{
	return NULL;
}

u32 __weak
wlanoidNANExtCmd(struct ADAPTER *prAdapter, void *pvSetBuffer,
		     uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	return WLAN_STATUS_NOT_SUPPORTED;
}

u32 __weak
wlanoidNANExtCmdRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
			uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	return WLAN_STATUS_NOT_SUPPORTED;
}

void __weak
nanExtComposeBeaconTrack(struct ADAPTER *prAdapter,
				struct _NAN_EVENT_REPORT_BEACON *prFwEvt)
{
}

void __weak
nanExtComposeClusterEvent(struct ADAPTER *prAdapter,
			       struct NAN_DE_EVENT *prDeEvt)
{
}

uint32_t __weak
nanSchedGetVendorEhtAttr(struct ADAPTER *prAdapter,
				  uint8_t **ppucVendorAttr,
				  uint32_t *pu4VendorAttrLength)
{
	return 0;
}

uint32_t __weak
nanSchedGetVendorAttr(struct ADAPTER *prAdapter,
			       uint8_t **ppucVendorAttr,
			       uint32_t *pu4VendorAttrLength)
{
	return 0;
}


uint16_t __weak
nanDataEngineVendorAttrLength(struct ADAPTER *prAdapter,
				 struct _NAN_NDL_INSTANCE_T *prNDL,
				 struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint16_t u2VSAttrLen = 0;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return 0;
	}

	DBGLOG(NAN, INFO,
		"NanCustomAttr = %d\n",
		prAdapter->rNanCustomAttr.length);

	if ((prNDL == NULL) || (prNDP == NULL))
		return 0;

	u2VSAttrLen = prAdapter->rNanCustomAttr.length;

	if (u2VSAttrLen > NAN_CUSTOM_ATTRIBUTE_MAX_SIZE)
		return 0;

	return u2VSAttrLen;
}

void __weak
nanDataEngineVendorAttrAppend(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      struct _NAN_NDL_INSTANCE_T *prNDL,
				      struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint8_t *pucVSAttrBuf = NULL;
	uint16_t u2VSAttrLen = 0;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return;
	}

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return;
	}

	if ((prNDL == NULL) || (prNDP == NULL))
		return;

	pucVSAttrBuf = prAdapter->rNanCustomAttr.data;
	u2VSAttrLen = prAdapter->rNanCustomAttr.length;

	if (u2VSAttrLen > NAN_CUSTOM_ATTRIBUTE_MAX_SIZE)
		return;

	kalMemCopy(((uint8_t *)prMsduInfo->prPacket) +
		   prMsduInfo->u2FrameLength,
		   pucVSAttrBuf, u2VSAttrLen);

	prMsduInfo->u2FrameLength += u2VSAttrLen;
}

uint16_t __weak
nanDataEngineVendorEhtAttrLength(struct ADAPTER *prAdapter,
				 struct _NAN_NDL_INSTANCE_T *prNDL,
				 struct _NAN_NDP_INSTANCE_T *prNDP)
{
	return 0;
}

void __weak
nanDataEngineVendorEhtAttrAppend(struct ADAPTER *prAdapter,
				      struct MSDU_INFO *prMsduInfo,
				      struct _NAN_NDL_INSTANCE_T *prNDL,
				      struct _NAN_NDP_INSTANCE_T *prNDP)
{
}

uint32_t __weak
nanGetFcSlots(struct ADAPTER *prAdapter)
{
	uint32_t u4Bitmap = 0;

	DBGLOG(NAN, TEMP,
	       "FC slots: %02x-%02x-%02x-%02x\n",
	       ((uint8_t *)&u4Bitmap)[0], ((uint8_t *)&u4Bitmap)[1],
	       ((uint8_t *)&u4Bitmap)[2], ((uint8_t *)&u4Bitmap)[3]);

	return u4Bitmap;
}

uint32_t __weak
nanGetTimelineFcSlots(struct ADAPTER *prAdapter, size_t szTimelineIdx,
			       size_t szSlotIdx)
{
	uint32_t u4Bitmap = 0;

	NAN_DW_DBGLOG(NAN, DEBUG, TRUE, szSlotIdx,
		      "Timeline %u FC slots: %02x-%02x-%02x-%02x\n",
		      szTimelineIdx,
		      ((uint8_t *)&u4Bitmap)[0], ((uint8_t *)&u4Bitmap)[1],
		      ((uint8_t *)&u4Bitmap)[2], ((uint8_t *)&u4Bitmap)[3]);

	return u4Bitmap;
}

/*
 * @szSlotIdx: slot index [0..512) representing the range in 0~8192 TU
 */
u_int8_t __weak
nanIsChnlSwitchSlot(struct ADAPTER *prAdapter,
			     unsigned char fgPrintLog,
			     size_t szTimelineIdx,
			     size_t szSlotIdx)
{
	return FALSE;
}

void __weak
nanExtTerminateApNan(struct ADAPTER *prAdapter, uint8_t ucReason)
{

}

void __weak
nanExtTerminateApNanEndPs(struct ADAPTER *prAdapter)
{

}

void __weak
nanExtTerminateApNanEndLegacy(struct ADAPTER *prAdapter)
{

}

void __weak
nanExtRxAuthHandler(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb)
{

}

void __weak
nanExtAisConnectHandler(struct ADAPTER *prAdapter)
{

}
uint32_t __weak
nanExtSetCountryCodeHandler(struct ADAPTER *prAdapter)
{
	return 0;
}

uint32_t __weak
nanExtAisAssocDoneHandler(
		struct ADAPTER *prAdapter,
		uint32_t rJoinStatus,
		struct STA_RECORD *prStaRec)
{
	return 0;
}

uint32_t __weak
nanExtScanStartHandler(
		struct ADAPTER *prAdapter,
		struct cfg80211_scan_request *request)
{
	return 0;
}

uint32_t __weak
nanExtAisChangeHandler(
		struct ADAPTER *prAdapter)
{
	return 0;
}

uint32_t __weak
nanExtScanCompleteHandler(
		struct ADAPTER *prAdapter,
		uint8_t ucStatus)
{
	return 0;
}

uint32_t __weak
nanExtUniEventHandler(struct ADAPTER *prAdapter,
		uint32_t u4SubEvent,
		uint8_t *pucBuf)
{
	return 0;
}

uint32_t __weak
nanExtEventHandler(struct ADAPTER *prAdapter,
		uint32_t u4SubEvent,
		uint8_t *pucBuf)
{
	return 0;
}

uint32_t __weak
nanExtProcessRsvdFrame(struct ADAPTER *prAdapter,
		 struct SW_RFB *prSwRfb)
{
	return 0;
}

uint32_t __weak
nanExtAisAssocStartHandler(
		struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		uint8_t ucChannelNum,
		uint8_t ucChnlBw,
		enum ENUM_CHNL_EXT eSco)
{
	return 0;
}

uint32_t __weak
nanExtRxAssocHandler(
		struct ADAPTER *prAdapter,
		uint8_t *buf)
{
	return 0;
}
#endif /* CFG_SUPPORT_NAN == 1 */

