// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "nanInstantCommMode.c"
 *    \brief  This file including NAN Instant Communication Mode functions.
 *
 *    This file provided the macros and functions library support for NAN R4
 *    Instant Communication mode features
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#include "precomp.h"
#include "nan_base.h"


/*******************************************************************************
 *                   E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            C O N S T A N T S
 *******************************************************************************
 */
static const char * const pcCustomIcmTag = "ICM";

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

__KAL_ATTRIB_PACKED_FRONT__ __KAL_ATTRIB_ALIGNED_FRONT__(4)
struct _NAN_CMD_ICM_T {
	uint8_t ucEnabled;
	uint32_t u4Bitmap2g;
	uint32_t u4Bitmap5g;
	uint8_t ucChannel2g;
	uint8_t ucChannel5g;
} __KAL_ATTRIB_PACKED__ __KAL_ATTRIB_ALIGNED__(4);

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
static u_int8_t NAN_BAND_IN_SAME_TIMELINE(enum ENUM_BAND eBand1,
						 enum ENUM_BAND eBand2)
{
	if (eBand1 == eBand2)
		return TRUE;

#if (CFG_SUPPORT_NAN_6G == 1)
	if (eBand1 == BAND_5G && eBand2 == BAND_6G)
		return TRUE;

	if (eBand1 == BAND_6G && eBand2 == BAND_5G)
		return TRUE;
#endif

	return FALSE;
}

static void nanExcludeConcurrencySlots(struct ADAPTER *prAdapter,
				       enum ENUM_BAND eCustBand,
				       uint32_t *u4Bitmap)
{
	enum ENUM_BAND eAisBand;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo;
	uint32_t u4SlotBitmap;
	uint8_t ucPhyTypeSet;

	if (nanSchedGetConnChnlUsage(prAdapter,
		NETWORK_TYPE_AIS,
		eCustBand,
		&rAisChnlInfo,
		&u4SlotBitmap,
		&ucPhyTypeSet) !=
		WLAN_STATUS_SUCCESS)
		return;

	eAisBand = nanRegGetNanChnlBand(rAisChnlInfo);

	DBGLOG(NAN, INFO,
		"Check band:%u vs. AIS: chnl:%u b:%u butmap:0x%08x\n",
		eCustBand, rAisChnlInfo.u4PrimaryChnl,
		eAisBand, u4SlotBitmap);

	if (nanGetTimelineNum() == 1 ||
	    NAN_BAND_IN_SAME_TIMELINE(eAisBand, eCustBand)) {
		DBGLOG(NAN, INFO, "Update 0x%08x => 0x%08x\n", *u4Bitmap,
			   *u4Bitmap & ~u4SlotBitmap);
		*u4Bitmap &= ~u4SlotBitmap;
	}
}

static void nanSetInstantCommModeCmd(struct ADAPTER *prAdapter,
				     u_int8_t fgEnable,
				     uint8_t ucChannel2g,
				     uint32_t u4Bitmap2g,
				     uint8_t ucChannel5g,
				     uint32_t u4Bitmap5g)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_ICM_T *prCmd = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_ICM_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;
	rStatus = nicNanAddNewTlvElement(NAN_CMD_INSTANT_COMM_MODE,
					 sizeof(struct _NAN_CMD_ICM_T),
					 u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicNanGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmd = (struct _NAN_CMD_ICM_T *)prTlvElement->aucbody;
	kalMemZero(prCmd, sizeof(*prCmd));

	prCmd->ucEnabled = fgEnable;

	if (fgEnable) {
		prCmd->u4Bitmap2g = u4Bitmap2g;
		prCmd->u4Bitmap5g = u4Bitmap5g;
		prCmd->ucChannel2g = ucChannel2g;
		prCmd->ucChannel5g = ucChannel5g;
	}

	rStatus = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_NAN_EXT_CMD,
				      TRUE, FALSE, FALSE,
				      NULL, nicCmdTimeoutCommon,
				      u4CmdBufferLen, (uint8_t *)prCmdBuffer,
				      NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

u_int8_t nanIsInstantCommModeSupport(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return FALSE;

	return prAdapter->rWifiVar.fgNanInstantCommMode;
}

void nanEnableInstantCommMode(struct ADAPTER *prAdapter, u_int8_t fgEnable)
{
	struct WIFI_VAR *prWifiVar;
	struct _NAN_SCHEDULER_T *prNanScheduler;
	uint8_t uc2Gchannel = 0;
	uint8_t uc5Gchannel = 0;
	uint32_t u4Bitmap2g = 0;
	uint32_t u4Bitmap5g = 0;
	enum ENUM_BAND eBand;
	uint32_t *pu4Bitmap = NULL;
	uint8_t ucChannel;

	if (!prAdapter)
		return;

	prWifiVar = &prAdapter->rWifiVar;
	prNanScheduler = nanGetScheduler(prAdapter);

	/* ReconfigureCustFaw */
	nanSchedNegoCustFawRemoveEntry(prAdapter, pcCustomIcmTag);
	if (fgEnable) {
		if (prNanScheduler->u4NanInstantModeChannel) {
			cnmFreqToChnl(prNanScheduler->u4NanInstantModeChannel,
				      &ucChannel, &eBand);
		} else if (prNanScheduler->fgEn5gH || prNanScheduler->fgEn5gL) {
			eBand = BAND_5G;
			ucChannel = g_r5gDwChnl.u4PrimaryChnl;
			uc5Gchannel = ucChannel;
		} else {
			eBand = BAND_2G4;
			ucChannel = g_r2gDwChnl.u4PrimaryChnl;
			uc2Gchannel = ucChannel;
		}

		if (eBand == BAND_5G)
			pu4Bitmap = &u4Bitmap5g;
		else
			pu4Bitmap = &u4Bitmap2g;

		*pu4Bitmap = prNanScheduler->u4NanInstantModeBitmap;
		nanExcludeConcurrencySlots(prAdapter, eBand, pu4Bitmap);
		nanSchedNegoCustFawAddEntry(prAdapter,
					    &(struct _NAN_CUST_FAW_ENTRY){
					    .pcTag = pcCustomIcmTag,
					    .ucOpChannel = ucChannel,
					    .eBand = eBand,
					    .u4Bitmap = *pu4Bitmap});
	}
	nanSchedNegoCustFawReconfigure(prAdapter);

	/* Set to FW */
	nanSetInstantCommModeCmd(prAdapter, fgEnable,
				 uc2Gchannel, u4Bitmap2g,
				 uc5Gchannel, u4Bitmap5g);
}

void nanInstantCommModeOnHandler(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prNanScheduler;

	prNanScheduler = nanGetScheduler(prAdapter);
	if (prNanScheduler && prAdapter->rWifiVar.fgNanInstantCommMode &&
	    prNanScheduler->fgNanInstantMode) { /* framework requests to run */
		nanEnableInstantCommMode(prAdapter,
					 prNanScheduler->fgNanInstantMode);
	}
}

void nanInstantCommModeBackToNormal(struct ADAPTER *prAdapter)
{
	struct _NAN_SCHEDULER_T *prNanScheduler;

	prNanScheduler = nanGetScheduler(prAdapter);
	if (prNanScheduler && prAdapter->rWifiVar.fgNanInstantCommMode &&
	    prNanScheduler->fgNanInstantMode) { /* running */
		nanEnableInstantCommMode(prAdapter, FALSE);

		prNanScheduler->fgNanInstantMode = FALSE;
		/* TODO: notify framework of the status update */
	}
}

