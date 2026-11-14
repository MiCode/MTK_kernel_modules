// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/*! \file   "nanRescheduler.c"
 *  \brief  This file defines the procedures for
 *          handling extended 6g rescheduling
 *          requirement from STD+
 *
 *    Detail description.
 */



/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if (CFG_SUPPORT_NAN == 1)

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_os.h"
#include "gl_kal.h"
#include "gl_rst.h"
#if (CFG_EXT_VERSION == 1)
#include "gl_sys.h"
#endif
#include "wlan_lib.h"
#include "debug.h"
#include "wlan_oid.h"
#include <linux/rtc.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>

#include "nanRescheduler.h"

#if (CFG_SUPPORT_NAN_RESCHEDULE  == 1)

/*sync with enum RESCHEDULE_SOURCE */
static const char * const RESCHEDULE_SRC[] = {
	[RESCHEDULE_NULL] = "RESCHEDULE_NULL",
	[AIS_CONNECTED] = "AIS_CONNECTED",
	[AIS_DISCONNECTED] = "AIS_DISCONNECTED",
	[NEW_NDL] = "NEW_NDL",
	[REMOVE_NDL] = "REMOVE_NDL",
	[P2P_CONNECTED] = "P2P_CONNECTED",
	[P2P_DISCONNECTED] = "P2P_DISCONNECTED",
};

static int GetActiveNDPNum(const struct _NAN_NDL_INSTANCE_T *prNDL);
static struct _NAN_RESCHEDULE_TOKEN_T *
GenReScheduleToken(struct ADAPTER *prAdapter,
		   uint8_t fgRescheduleInputNDL,
		   enum RESCHEDULE_SOURCE event,
		   const struct _NAN_NDL_INSTANCE_T *prNDL);

static struct _NAN_RESCHEDULE_TOKEN_T *
getOngoing_RescheduleToken(struct ADAPTER *prAdapter);
static struct _NAN_RESCHED_NDL_INFO*
getOngoing_RescheduleNDL(struct ADAPTER *prAdapter);
static struct _NAN_RESCHED_NDL_INFO*
getNewState_RescheduleNDL(struct ADAPTER *prAdapter);
static struct _NAN_RESCHED_NDL_INFO*
getNewState_RescheduleNDL_reorder(struct ADAPTER *prAdapter);
static void
FreeReScheduleToken(struct ADAPTER *prAdapter,
		    struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken);
static void FreeReScheduleNdlInfo(struct ADAPTER *prAdapter,
	struct _NAN_RESCHED_NDL_INFO *reSchedNdlInfo);

static int
GetActiveNDPNum(const struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint8_t i = 0;
	uint8_t activeNdpNum = 0;

	if (prNDL == NULL)
		return 0;
	for (i = 0; i < prNDL->ucNDPNum; i++) {
		if (prNDL->arNDP[i].fgNDPActive == TRUE)
			activeNdpNum++;
	}
	return activeNdpNum;
}

static uint8_t ChkEnqCond(enum _ENUM_NDL_MGMT_STATE_T e)
{
	if (e == NDL_REQUEST_SCHEDULE_NDP ||
		e == NDL_REQUEST_SCHEDULE_NDL ||
		e == NDL_SCHEDULE_SETUP ||
		e == NDL_SCHEDULE_ESTABLISHED)
		return TRUE;
	return FALSE;
}

static uint8_t ChkDeqCond(enum _ENUM_NDL_MGMT_STATE_T e)
{
	if (e == NDL_SCHEDULE_ESTABLISHED)
		return TRUE;
	return FALSE;
}

static uint8_t ChkShrinkCond(struct _NAN_RESCHED_NDL_INFO *ReSchedNdlInfo)
{
	uint8_t shrinkCondition = FALSE;

	if (ReSchedNdlInfo == NULL)
		return FALSE;
	shrinkCondition =
		(ReSchedNdlInfo) &&
		(ReSchedNdlInfo->eNdlRescheduleState ==
			NDL_RESCHEDULE_STATE_NEW) &&
		(!ChkDeqCond(ReSchedNdlInfo->prNDL->eCurrentNDLMgmtState));
	return shrinkCondition;
}

static struct _NAN_RESCHEDULE_TOKEN_T *
GenReScheduleToken(struct ADAPTER *prAdapter,
		   uint8_t fgRescheduleInputNDL,
		   enum RESCHEDULE_SOURCE event,
		   const struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint8_t ucNdlIndex;
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo =
					&prAdapter->rDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prReschedNDL;
	static uint8_t uReSchedTokenID;
	struct _NAN_RESCHED_NDL_INFO *prReSchedNdlInfo;

	prReScheduleToken = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					sizeof(*prReScheduleToken));
	if (!prReScheduleToken)
		return NULL;

	uReSchedTokenID = uReSchedTokenID % 100;
	prReScheduleToken->ucTokenID = uReSchedTokenID++;
	DBGLOG(NAN, DEBUG,
	       "---[RESCHEDULE_TRACE] LVL2: TOKEN(ucTokenID:%u) INFO :)\n",
	       prReScheduleToken->ucTokenID);
	prReScheduleToken->ucEvent = event;
#if (CFG_SUPPORT_NAN_11BE == 1)
	prReScheduleToken->fgIsEhtReschedule =
		prNDL ? prNDL->fgIsEhtReschedule : FALSE;
#endif
	LINK_INITIALIZE(&(prReScheduleToken->rReSchedNdlList));
	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
			ucNdlIndex++) {
		prReschedNDL = &prDataPathInfo->arNDL[ucNdlIndex];

		if (prReschedNDL->fgNDLValid == FALSE ||
		    ChkEnqCond(prReschedNDL->eCurrentNDLMgmtState) == FALSE)
			continue;

		if (prNDL && !fgRescheduleInputNDL && prNDL == prReschedNDL) {
			/*
			 * this is NDL triggered this reschedule
			 * (eg., newly connected device.
			 * not existing device)
			 * skip this NDL.
			 */
			continue;
		}
		prReSchedNdlInfo = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
					sizeof(*prReSchedNdlInfo));
		if (prReSchedNdlInfo) {
			prReSchedNdlInfo->eNdlRescheduleState =
				NDL_RESCHEDULE_STATE_NEW;
			prReSchedNdlInfo->prNDL = prReschedNDL;
			LINK_INSERT_TAIL(&prReScheduleToken->rReSchedNdlList,
					 &prReSchedNdlInfo->rLinkEntry);
			DBGLOG(NAN, DEBUG,
			       "---[RESCHEDULE_TRACE] LVL2: ucNdlIndex#%u:NDL(MAC:"
			       MACSTR")\n",
			       ucNdlIndex,
			       prReSchedNdlInfo->prNDL->aucPeerMacAddr);
		} else {
			DBGLOG(NAN, ERROR,
			       "Failed to generate a prReSchedNdlInfo.");
		}
	}
	DBGLOG(NAN, DEBUG,
	       "---[RESCHEDULE_TRACE] LVL2: ---------------------------\n");

	return prReScheduleToken;
}

uint32_t
ReleaseNanSlotsForSchedulePrep(struct ADAPTER *prAdapter,
			       enum RESCHEDULE_SOURCE event,
			       u_int8_t fgIsEhtRescheduleNewNDL)
{
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	union _NAN_BAND_CHNL_CTRL rAisChnlInfo = {0};
	uint32_t u4SlotBitmap = 0;
	uint8_t ucAisPhyTypeSet;
	enum ENUM_BAND eAisBand;
	const size_t sz2gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_2G4);
	const size_t sz5gTimeLineIdx = nanGetTimelineMgmtIndexByBand(prAdapter,
								     BAND_5G);
	union _NAN_BAND_CHNL_CTRL arDwChnl[BAND_NUM] = {
		[BAND_NULL] = g_rNullChnl,
		[BAND_2G4] = g_r2gDwChnl,
		[BAND_5G] = g_r5gDwChnl,
#if (CFG_SUPPORT_WIFI_6G == 1)
		[BAND_6G] = g_r5gDwChnl,
#endif
	};

	if (event == AIS_CONNECTED) {
		/* FIXME: per band trigger,
		 * otherwise 2G NAN == AIS, but 5G NAN != AIS,
		 * both 2G/5G will be released
		 */
		for (eAisBand = BAND_2G4; eAisBand < BAND_NUM; eAisBand++) {
			if (nanSchedGetConnChnlUsage(prAdapter,
				     NETWORK_TYPE_AIS, eAisBand,
				     &rAisChnlInfo, &u4SlotBitmap,
				     &ucAisPhyTypeSet) == WLAN_STATUS_SUCCESS &&
			    nanSchedChkConcurrOp(arDwChnl[eAisBand],
						 rAisChnlInfo) ==
				    CNM_CH_CONCURR_MCC) {

				nanSchedReleaseReschedCommitSlot(prAdapter,
					NAN_SLOT_MASK_TYPE_AIS,
					nanGetTimelineMgmtIndexByBand(prAdapter,
					eAisBand));
			}
		}
	} else if (event == AIS_DISCONNECTED) {
		nanSchedReleaseReschedCommitSlot(prAdapter,
			NAN_SLOT_MASK_TYPE_AIS, sz2gTimeLineIdx);
		nanSchedReleaseReschedCommitSlot(prAdapter,
			NAN_SLOT_MASK_TYPE_AIS, sz5gTimeLineIdx);
	} else if (event == NEW_NDL) {
		uint32_t u4ReschedSlot = 0;

#if (CFG_SUPPORT_NAN_11BE == 1)
		u4ReschedSlot = fgIsEhtRescheduleNewNDL ?
			NAN_SLOT_MASK_TYPE_AIS | nanGetNdlSlots(prAdapter) :
			nanGetNdlSlots(prAdapter);
#else
		u4ReschedSlot = nanGetNdlSlots(prAdapter);
#endif

		nanSchedReleaseReschedCommitSlot(prAdapter,
			u4ReschedSlot, sz5gTimeLineIdx);
	} else if (event == REMOVE_NDL) {
/* Only need release if REMOVE_NDL condition recover to customer requirement */
#ifdef NAN_UNUSED
		nanSchedReleaseReschedCommitSlot(prAdapter,
			nanGetNdlSlots(prAdapter), sz5gTimeLineIdx);
#else
		DBGLOG(NAN, WARN, "Not release slot when REMOVE NDL\n");
		/* Not release committed forcely,
		 * but still need to release unused commit slot
		 */
		nanSchedReleaseUnusedCommitSlot(prAdapter);
		nanSchedCmdUpdateAvailability(prAdapter);
#endif
	} else if (event == P2P_CONNECTED) {
		/* Move to AIS+NAN+P2P or NAN+P2P */
		for (eAisBand = BAND_2G4; eAisBand < BAND_NUM; eAisBand++) {
			/* TODO: distinguish P2P from SAP? */
			if (nanSchedGetConnChnlUsage(prAdapter,
				     NETWORK_TYPE_P2P, eAisBand,
				     &rAisChnlInfo, &u4SlotBitmap,
				     &ucAisPhyTypeSet) == WLAN_STATUS_SUCCESS) {
				nanSchedReleaseReschedCommitSlot(prAdapter,
					NAN_T1_SLOT_MASK_CONCURRENT_FULL,
					nanGetTimelineMgmtIndexByBand(prAdapter,
								eAisBand));
			}
		}
	} else if (event == P2P_DISCONNECTED) {
		/* Go back to AIS+NAN or NAN only */
		DBGLOG(NAN, WARN, "Not release slot when P2P disconnected\n");
		/* Not release committed forcely,
		 * but still need to release unused commit slot
		 */
		nanSchedReleaseUnusedCommitSlot(prAdapter);
		nanSchedCmdUpdateAvailability(prAdapter);
	}
	return rRetStatus;
}

struct _NAN_RESCHED_NDL_INFO*
getOngoing_RescheduleNDL(struct ADAPTER *prAdapter)
{
	struct _NAN_RESCHED_NDL_INFO *ReSchedNdlInfo = NULL;
	struct _NAN_RESCHEDULE_TOKEN_T *reschedToken =
		getOngoing_RescheduleToken(prAdapter);
	if (reschedToken) {
		ReSchedNdlInfo = LINK_PEEK_HEAD(
				&(reschedToken->rReSchedNdlList),
				struct _NAN_RESCHED_NDL_INFO,
				rLinkEntry);
	}
	if (ReSchedNdlInfo &&
		ReSchedNdlInfo->eNdlRescheduleState ==
		NDL_RESCHEDULE_STATE_NEGO_ONGOING)
		return ReSchedNdlInfo;
	else
		return NULL;
}

struct _NAN_RESCHED_NDL_INFO*
getNewState_RescheduleNDL(struct ADAPTER *prAdapter)
{
	struct _NAN_RESCHED_NDL_INFO *ReSchedNdlInfo = NULL;
	struct _NAN_RESCHEDULE_TOKEN_T *reschedToken =
		getOngoing_RescheduleToken(prAdapter);
	if (reschedToken) {
		ReSchedNdlInfo = LINK_PEEK_HEAD(
				&(reschedToken->rReSchedNdlList),
				struct _NAN_RESCHED_NDL_INFO,
				rLinkEntry);
	}
	if (ReSchedNdlInfo &&
		ReSchedNdlInfo->eNdlRescheduleState ==
		NDL_RESCHEDULE_STATE_NEW)
		return ReSchedNdlInfo;
	else
		return NULL;
}

/*
 * getNewState_RescheduleNDL_reorder filters
 * ndl info which is (not started reschedule yet)
 * AND (NDL not yet established)
 */
struct _NAN_RESCHED_NDL_INFO*
getNewState_RescheduleNDL_reorder(struct ADAPTER *prAdapter)
{
	struct _NAN_RESCHED_NDL_INFO *ReSchedNdlInfo = NULL;
	struct _NAN_RESCHED_NDL_INFO *prNDLInfo;
	struct _NAN_RESCHEDULE_TOKEN_T *reschedToken =
		getOngoing_RescheduleToken(prAdapter);

	if (reschedToken) {
		ReSchedNdlInfo = LINK_PEEK_HEAD(
			&(reschedToken->rReSchedNdlList),
			struct _NAN_RESCHED_NDL_INFO,
			rLinkEntry);
		while (ChkShrinkCond(ReSchedNdlInfo)) {
			LINK_REMOVE_HEAD(&(reschedToken->rReSchedNdlList),
				prNDLInfo, struct _NAN_RESCHED_NDL_INFO*);
			FreeReScheduleNdlInfo(prAdapter, prNDLInfo);
			ReSchedNdlInfo = LINK_PEEK_HEAD(
					&(reschedToken->rReSchedNdlList),
					struct _NAN_RESCHED_NDL_INFO,
					rLinkEntry);
		}
	}
	return getNewState_RescheduleNDL(prAdapter);
}

struct _NAN_RESCHEDULE_TOKEN_T *
getOngoing_RescheduleToken(struct ADAPTER *prAdapter)
{
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken = NULL;
	prReScheduleToken =
	LINK_PEEK_HEAD(
		&(prAdapter->rDataPathInfo.rReScheduleTokenList),
		struct _NAN_RESCHEDULE_TOKEN_T, rLinkEntry);
	return prReScheduleToken;
}

static void FreeReScheduleToken(struct ADAPTER *prAdapter,
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken)
{
	struct _NAN_RESCHED_NDL_INFO *prReSchedNdlInfo = NULL;

	if (prReScheduleToken == NULL)
		return;
	while (!LINK_IS_EMPTY(&prReScheduleToken->rReSchedNdlList)) {
		LINK_REMOVE_HEAD(
			&prReScheduleToken->rReSchedNdlList,
			prReSchedNdlInfo,
			struct _NAN_RESCHED_NDL_INFO *);
		cnmMemFree(prAdapter, prReSchedNdlInfo);
	}
	cnmMemFree(prAdapter, prReScheduleToken);
}

static void FreeReScheduleNdlInfo(struct ADAPTER *prAdapter,
	struct _NAN_RESCHED_NDL_INFO *reSchedNdlInfo)
{
	cnmMemFree(prAdapter, reSchedNdlInfo);
}


static void handleAisP2pConnected(struct ADAPTER *prAdapter,
				  enum RESCHEDULE_SOURCE event,
				  struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct LINK *prReScheduleTokenList;
	struct _NAN_RESCHED_NDL_INFO *prOngoingNDLInfo;
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;
	struct _NAN_RESCHED_NDL_INFO *prNextNDLInfo;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	prReScheduleTokenList = &prDataPathInfo->rReScheduleTokenList;
	prOngoingNDLInfo = getOngoing_RescheduleNDL(prAdapter);

	DBGLOG(NAN, INFO,
	       "->[RESCHEDULE_TRACE] LVL1:EVENT=%s:CHECKING RESCHEDULE NEEDED\n",
	       RESCHEDULE_SRC[event]);

	if (!nanCheckIsNeedReschedule(prAdapter, event, NULL)) {
		DBGLOG(NAN, DEBUG,
		       "<-[RESCHEDULE_TRACE] LVL1:RESCHEDULE NOT NEEDED\n");
		return;
	}

	/* nanCheckIsNeedReschedule() returns TRUE */
	prReScheduleToken = GenReScheduleToken(prAdapter, FALSE, event, NULL);
	if (!prReScheduleToken) {
		DBGLOG(NAN, ERROR, "Failed to generate a ReScheduleToken.");
		return;
	}

	if (LINK_IS_EMPTY(&prReScheduleToken->rReSchedNdlList)) {
		DBGLOG(NAN, DEBUG,
		       "<--[RESCHEDULE_TRACE] LVL2:NO NDL in TOKEN(ucTokenID:%u, source:%s)\n",
		       prReScheduleToken->ucTokenID,
		       RESCHEDULE_SRC[event]);
		FreeReScheduleToken(prAdapter, prReScheduleToken);
		return;
	}

	DBGLOG(NAN, DEBUG,
	       "-->[RESCHEDULE_TRACE] LVL2:GENERATE TOKEN(ucTokenID:%u, source:%s)\n",
	       prReScheduleToken->ucTokenID,
	       RESCHEDULE_SRC[event]);

	ReleaseNanSlotsForSchedulePrep(prAdapter, event, FALSE);

	LINK_INSERT_TAIL(prReScheduleTokenList,
			 &prReScheduleToken->rLinkEntry);

	prNextNDLInfo = getNewState_RescheduleNDL_reorder(prAdapter);
	/*
	 * if there is ongoing reschedule ahead,
	 * it just push it back and processes
	 * it later. else case handle it directly
	 */
	if (prNextNDLInfo) {
		prNextNDLInfo->eNdlRescheduleState =
			NDL_RESCHEDULE_STATE_NEGO_ONGOING;
		DBGLOG(NAN, INFO,
		       "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE(MAC:"
		       MACSTR") START!\n",
		       MAC2STR(prNextNDLInfo->prNDL->aucPeerMacAddr));
		nanUpdateNdlScheduleV2(prAdapter, prNextNDLInfo->prNDL);
	}
}

static void handleRemoveNDL(struct ADAPTER *prAdapter,
		enum RESCHEDULE_SOURCE event,
		struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken = NULL;
	struct _NAN_RESCHED_NDL_INFO *prNextNDLInfo;
	struct _NAN_NDL_INSTANCE_T *prNextNDL;
	struct _NAN_RESCHED_NDL_INFO *prOngoingNDLInfo;
	struct _NAN_RESCHEDULE_TOKEN_T *prOngoingToken = NULL;
	struct _NAN_RESCHEDULE_TOKEN_T *prNextToken = NULL;

	prOngoingNDLInfo = getOngoing_RescheduleNDL(prAdapter);
	prOngoingToken = getOngoing_RescheduleToken(prAdapter);
	if (prNDL && prOngoingNDLInfo && prNDL == prOngoingNDLInfo->prNDL) {
		DBGLOG(NAN, DEBUG,
		       "<---[RESCHEDULE_TRACE] LVL3:RESCHEDULE(MAC:"
		       MACSTR") is FAILED.\n",
		       MAC2STR(prNDL->aucPeerMacAddr));
		LINK_REMOVE_HEAD(
			&(prOngoingToken->rReSchedNdlList),
			prOngoingNDLInfo,
			struct _NAN_RESCHED_NDL_INFO*);
		FreeReScheduleNdlInfo(prAdapter, prOngoingNDLInfo);
		if (LINK_IS_EMPTY(&prOngoingToken->rReSchedNdlList) ==
		    FALSE)	{
			/* get next NDL */
			prNextNDLInfo =
				getNewState_RescheduleNDL_reorder(prAdapter);
			if (prNextNDLInfo) {
				prNextNDL = prNextNDLInfo->prNDL;

				prNextNDLInfo->eNdlRescheduleState =
					NDL_RESCHEDULE_STATE_NEGO_ONGOING;
				DBGLOG(NAN, DEBUG,
				       "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE next NDL(MAC:"
				       MACSTR") START!\n",
				       MAC2STR(prNextNDL->aucPeerMacAddr));
				nanUpdateNdlScheduleV2(prAdapter, prNextNDL);
			}
		} else {
			LINK_REMOVE_HEAD(
				&prAdapter->rDataPathInfo.rReScheduleTokenList,
				prNextToken,
				struct _NAN_RESCHEDULE_TOKEN_T *);
			if (prNextToken != NULL)
				DBGLOG(NAN, DEBUG,
				       "<--[RESCHEDULE_TRACE] LVL2:Reschedule for Token(%u)->event(%s) is ALL DONE\n",
				       prNextToken->ucTokenID,
				       RESCHEDULE_SRC[prNextToken->ucEvent]);
			FreeReScheduleToken(prAdapter,
				prOngoingToken);
			if (LINK_IS_EMPTY(
				&prAdapter->rDataPathInfo.rReScheduleTokenList)
				    == FALSE) {
				prOngoingToken =
					getOngoing_RescheduleToken(prAdapter);
				prNextNDLInfo =
					getNewState_RescheduleNDL_reorder(
						  prAdapter);
				if (prNextNDLInfo) {
					prNextNDL = prNextNDLInfo->prNDL;

					prNextNDLInfo->eNdlRescheduleState =
					    NDL_RESCHEDULE_STATE_NEGO_ONGOING;
					if (prOngoingToken)
						DBGLOG(NAN, DEBUG,
						      "-->[RESCHEDULE_TRACE] LVL2:dequeue next TOKEN(ucTokenID:%u, source:%s)\n",
						      prOngoingToken->ucTokenID,
						      RESCHEDULE_SRC[
						      prOngoingToken->ucEvent]);
					DBGLOG(NAN, DEBUG,
					    "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE next NDL(MAC:"
					    MACSTR") START!\n",
					    MAC2STR(prNextNDL->aucPeerMacAddr));
					nanUpdateNdlScheduleV2(prAdapter,
							       prNextNDL);
				}
			} else {
				DBGLOG(NAN, DEBUG,
				       "<--[RESCHEDULE_TRACE] LVL2:No more token. END\n");
			}

		}
	} else {
		DBGLOG(NAN, INFO,
		       "->[RESCHEDULE_TRACE] LVL1:EVENT=%s:CHECKING RESCHEDULE NEEDED\n",
		       RESCHEDULE_SRC[event]);

		if (!nanCheckIsNeedReschedule(prAdapter, event, NULL)) {
			DBGLOG(NAN, DEBUG,
			       "<-[RESCHEDULE_TRACE] LVL1:RESCHEDULE NOT NEEDED\n");
			return;
		}

		/* create reschedToken with event */
		prReScheduleToken =
			GenReScheduleToken(prAdapter, FALSE, event, NULL);

		if (!prReScheduleToken) {
			DBGLOG(NAN, ERROR,
			       "Failed to generate a ReScheduleToken.");
			return;
		}

		DBGLOG(NAN, DEBUG,
		       "-->[RESCHEDULE_TRACE] LVL2:GENERATE TOKEN(ucTokenID:%u, source:%s)\n",
		       prReScheduleToken->ucTokenID,
		       RESCHEDULE_SRC[event]);

		if (LINK_IS_EMPTY(&prReScheduleToken->rReSchedNdlList)) {
			DBGLOG(NAN, DEBUG,
			       "<--[RESCHEDULE_TRACE] LVL2:NO NDL in TOKEN(ucTokenID:%u, source:%s)\n",
			       prReScheduleToken->ucTokenID,
			       RESCHEDULE_SRC[event]);
			FreeReScheduleToken(prAdapter, prReScheduleToken);
			return;
		}

		ReleaseNanSlotsForSchedulePrep(prAdapter, event, FALSE);
		/* enqueue NDLs to be rescheduled */
		LINK_INSERT_TAIL(&prAdapter->rDataPathInfo.rReScheduleTokenList,
				 &prReScheduleToken->rLinkEntry);
		prNextNDLInfo = getNewState_RescheduleNDL_reorder(prAdapter);
		if (prNextNDLInfo) {
			prNextNDL = prNextNDLInfo->prNDL;

			prNextNDLInfo->eNdlRescheduleState =
				NDL_RESCHEDULE_STATE_NEGO_ONGOING;
			DBGLOG(NAN, INFO,
			       "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE(MAC:"
			       MACSTR") START!\n",
			       MAC2STR(prNextNDL->aucPeerMacAddr));
			nanUpdateNdlScheduleV2(prAdapter, prNextNDL);
		}
	}
}

static void handleNewNDL(struct ADAPTER *prAdapter,
			 enum RESCHEDULE_SOURCE event,
			 struct _NAN_NDL_INSTANCE_T *prNDL)
{

	struct LINK *prReScheduleTokenList;
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;
	struct _NAN_RESCHED_NDL_INFO *prNextNDLInfo;
	struct _NAN_NDL_INSTANCE_T *prNextNDL;
	struct _NAN_RESCHED_NDL_INFO *prOngoingNDLInfo;
	struct _NAN_RESCHEDULE_TOKEN_T *prOngoingToken;
	struct _NAN_RESCHEDULE_TOKEN_T *prNextToken = NULL;
	uint8_t ucNDPNum;

	ucNDPNum = GetActiveNDPNum(prNDL);
	prOngoingNDLInfo = getOngoing_RescheduleNDL(prAdapter);
	prOngoingToken = getOngoing_RescheduleToken(prAdapter);

	prReScheduleTokenList = &prAdapter->rDataPathInfo.rReScheduleTokenList;

	if (ucNDPNum > 0 && prOngoingNDLInfo &&
	    prOngoingNDLInfo->prNDL  == prNDL &&
	    prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR) {
		/*
		 * In this case : input prNDL is result from
		 * "NEW_NDL initiated NDL reschedule".
		 * so do not generate reScheduleToken again.
		 * but process Ongoing reScheduleToken.
		 */
		DBGLOG(NAN, INFO,
		       "<---[RESCHEDULE_TRACE] LVL3:RESCHEDULE(MAC:"
		       MACSTR") is DONE.\n",
		       MAC2STR(prNDL->aucPeerMacAddr));
		prOngoingNDLInfo->eNdlRescheduleState =
			NDL_RESCHEDULE_STATE_ESTABLISHED;
		/* dequeue completed NDL from rReSchedNdlList. */
		LINK_REMOVE_HEAD(&(prOngoingToken->rReSchedNdlList),
			prNextNDLInfo, struct _NAN_RESCHED_NDL_INFO*);
		FreeReScheduleNdlInfo(prAdapter, prOngoingNDLInfo);

		/* reschedule next NDL from reschedule queue(rReSchedNdlList) */
		if (LINK_IS_EMPTY(&prOngoingToken->rReSchedNdlList) ==
			FALSE) {
			/* -> get next NDL */
			prNextNDLInfo =
				getNewState_RescheduleNDL_reorder(prAdapter);
			if (prNextNDLInfo) {
				prNextNDL = prNextNDLInfo->prNDL;

				prNextNDLInfo->eNdlRescheduleState =
					NDL_RESCHEDULE_STATE_NEGO_ONGOING;
				DBGLOG(NAN, DEBUG,
				       "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE next NDL(MAC:"
				       MACSTR") START!\n",
				       MAC2STR(prNextNDL->aucPeerMacAddr));
				nanUpdateNdlScheduleV2(prAdapter, prNextNDL);
			}
		} else {
			LINK_REMOVE_HEAD(prReScheduleTokenList, prNextToken,
					 struct _NAN_RESCHEDULE_TOKEN_T *);
			if (prNextToken != NULL)
				DBGLOG(NAN, DEBUG,
				       "<--[RESCHEDULE_TRACE] LVL2:Reschedule for Token(%u)->event(%s) is ALL DONE\n",
				       prNextToken->ucTokenID,
				       RESCHEDULE_SRC[prNextToken->ucEvent]);
			FreeReScheduleToken(prAdapter,
					    prOngoingToken);
			/* 1. dequeue next ReschedToken from
			 * &prAdapter->rDataPathInfo.rReScheduleTokenList.
			 * And reschedule new NDL.
			 */
			if (LINK_IS_EMPTY(prReScheduleTokenList) == FALSE) {
				prNextToken =
					getOngoing_RescheduleToken(prAdapter);
				prNextNDLInfo =
					getNewState_RescheduleNDL_reorder(
							  prAdapter);
				if (prNextNDLInfo) {
					prNextNDL = prNextNDLInfo->prNDL;

					prNextNDLInfo->eNdlRescheduleState =
					      NDL_RESCHEDULE_STATE_NEGO_ONGOING;
					if (prNextToken)
						DBGLOG(NAN, DEBUG,
						       "-->[RESCHEDULE_TRACE] LVL2:dequeue next TOKEN(ucTokenID:%u, source:%s)\n",
						       prNextToken->ucTokenID,
						       RESCHEDULE_SRC[
						       prNextToken->ucEvent]);
					DBGLOG(NAN, DEBUG,
					    "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE next NDL(MAC:"
					    MACSTR") START!\n",
					    MAC2STR(prNextNDL->aucPeerMacAddr));
					nanUpdateNdlScheduleV2(prAdapter,
							       prNextNDL);
				}
			} else {
				DBGLOG(NAN, DEBUG,
				       "<--[RESCHEDULE_TRACE] LVL2:No more token. END\n");
			}

		}
	} else if (ucNDPNum > 0 && prOngoingNDLInfo &&
		   prOngoingNDLInfo && prOngoingNDLInfo->prNDL != prNDL &&
		   prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR) {
		DBGLOG(NAN, DEBUG,
		       "[RESCHEDULE_TRACE] INFO: it is not NDL reschedule(req) from this module.\n");
	} else if (ucNDPNum > 0 &&  prOngoingNDLInfo == NULL &&
		   prNDL->eNDLRole == NAN_PROTOCOL_RESPONDER) {
		/*
		 * In this case : input prNDL is result from
		 * peer initiated Schedule update
		 */
		DBGLOG(NAN, DEBUG,
		       "[RESCHEDULE_TRACE] INFO: peer requested reschedule.\n");
	} else if (ucNDPNum == 0) {
		/*
		 * in this case : this is real NDL creation
		 * from new connection(not by reschedule)
		 */
		DBGLOG(NAN, INFO,
		       "->[RESCHEDULE_TRACE] LVL1:EVENT=%s:CHECKING PENDING RESCHEDULE TOKEN\n",
		       RESCHEDULE_SRC[event]);
		prNextNDLInfo = getNewState_RescheduleNDL_reorder(prAdapter);
		if (prNextNDLInfo != NULL) {
			prNextNDL = prNextNDLInfo->prNDL;

			prReScheduleToken =
				getOngoing_RescheduleToken(prAdapter);
			if (prReScheduleToken &&
			    LINK_IS_EMPTY(&prReScheduleToken->rReSchedNdlList)
			    == FALSE) {
				prNextNDLInfo->eNdlRescheduleState =
					NDL_RESCHEDULE_STATE_NEGO_ONGOING;
				DBGLOG(NAN, INFO,
				       "--->[RESCHEDULE_TRACE] LVL3:RESCHEDULE(MAC:"
				       MACSTR") START!\n",
				       MAC2STR(prNextNDL->aucPeerMacAddr));
				nanUpdateNdlScheduleV2(prAdapter, prNextNDL);
			}
		} else {
			DBGLOG(NAN, INFO,
			       "<-[RESCHEDULE_TRACE] LVL1:NO PENDING RESCHEDULE TOKEN\n");
		}
	} /* ucNDPNum == 0 */
}

void nanRescheduleNdlIfNeeded(struct ADAPTER *prAdapter,
	enum RESCHEDULE_SOURCE event,
	struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_RESCHED_NDL_INFO *prOngoingNDLInfo;
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);

	prDataPathInfo = &prAdapter->rDataPathInfo;
	prOngoingNDLInfo = getOngoing_RescheduleNDL(prAdapter);

	DBGLOG(NAN, DEBUG, "Enter event=%u\n", event);

	if (prNanScheduler) {
		if (nanGetPowerSavingPeerNum(prAdapter) == 0) {
			prNanScheduler->eRescheduleEventInPS = RESCHEDULE_NULL;
			prNanScheduler->prRescheduleNdlInPS = NULL;
		} else { /* trigger later after resumed */
			prNanScheduler->eRescheduleEventInPS = event;
			prNanScheduler->prRescheduleNdlInPS = prNDL;
		}
	}

	if (prDataPathInfo->ucNDLNum == 0 && prOngoingNDLInfo == NULL) {
		DBGLOG(NAN, INFO,
		       "->[RESCHEDULE_TRACE] LVL1:EVENT=%s:RESCHEDULE REQUESTED but NO NDL. SKIP\n",
		       RESCHEDULE_SRC[event]);
		return;
	}
	switch (event) {
	case AIS_CONNECTED:
	case P2P_CONNECTED:
		handleAisP2pConnected(prAdapter, event, prNDL);
		break;

	case REMOVE_NDL:
		handleRemoveNDL(prAdapter, event, prNDL);
		break;

	case NEW_NDL:
		handleNewNDL(prAdapter, event, prNDL);
		break;

	case P2P_DISCONNECTED: /* not handleed temporarily */
	case AIS_DISCONNECTED: /* not handleed temporarily */
	default:
		break;
	}
}

void nanRescheduleEnqueueNewToken(struct ADAPTER *prAdapter,
				  enum RESCHEDULE_SOURCE event,
				  struct _NAN_NDL_INSTANCE_T *prNDL)
{
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;
	struct LINK *prReScheduleTokenList;

	DBGLOG(NAN, INFO,
	       "->[RESCHEDULE_TRACE] LVL1:EVENT=%s:EXTERNAL RESCHEDULE REQUEST\n",
	       RESCHEDULE_SRC[event]);
	prReScheduleToken = GenReScheduleToken(prAdapter, FALSE, event, prNDL);

	if (!prReScheduleToken) {
		DBGLOG(NAN, ERROR, "Failed to generate a ReScheduleToken.");
		return;
	}

	DBGLOG(NAN, DEBUG,
	       "-->[RESCHEDULE_TRACE] LVL2:GENERATE TOKEN(ucTokenID:%u, source:%s)\n",
	       prReScheduleToken->ucTokenID, RESCHEDULE_SRC[event]);

	prReScheduleTokenList = &prAdapter->rDataPathInfo.rReScheduleTokenList;
	if (LINK_IS_EMPTY(&prReScheduleToken->rReSchedNdlList) == FALSE) {
		LINK_INSERT_TAIL(prReScheduleTokenList,
				 &prReScheduleToken->rLinkEntry);
	} else {
		DBGLOG(NAN, DEBUG,
		       "<--[RESCHEDULE_TRACE] LVL2:NO NDL in TOKEN(ucTokenID:%u, source:%s)\n",
		       prReScheduleToken->ucTokenID,
		       RESCHEDULE_SRC[event]);
		FreeReScheduleToken(prAdapter, prReScheduleToken);
	}
}

enum RESCHEDULE_SOURCE
nanRescheduleGetOngoingTokenEvent(struct ADAPTER *prAdapter)
{
	struct _NAN_RESCHEDULE_TOKEN_T *reschedToken = NULL;

	reschedToken = getOngoing_RescheduleToken(prAdapter);

	if (reschedToken)
		return reschedToken->ucEvent;
	else
		return RESCHEDULE_NULL;
}

void
nanRescheduleInit(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

	DBGLOG(NAN, DEBUG, "[RESCHEDULE_TRACE] (INIT RESCHEDULER)\n");
	prDataPathInfo = &(prAdapter->rDataPathInfo);
	LINK_INITIALIZE(&(prDataPathInfo->rReScheduleTokenList));
	nanSchedRegisterReschedInf(getOngoing_RescheduleToken);
}

void
nanRescheduleDeInit(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_RESCHEDULE_TOKEN_T *prReScheduleToken;

	prDataPathInfo = &(prAdapter->rDataPathInfo);
	DBGLOG(NAN, DEBUG, "[RESCHEDULE_TRACE] (DEINIT RESCHEDULER)\n");
	while (!LINK_IS_EMPTY(&prDataPathInfo->rReScheduleTokenList)) {
		LINK_REMOVE_HEAD(
		&prDataPathInfo->rReScheduleTokenList, prReScheduleToken,
		struct _NAN_RESCHEDULE_TOKEN_T *);
		FreeReScheduleToken(prAdapter, prReScheduleToken);
	}
	nanSchedUnRegisterReschedInf();
}

void nanResumeRescheduleTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam)
{
	/* handleRemoveNDL(prAdapter, REMOVE_NDL, NULL); */
}

u_int8_t nanReschedIsOngoing(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Idx = 0;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDL_NUM; u4Idx++) {
		prNDL = &prDataPathInfo->arNDL[u4Idx];
		if (!prNDL->fgNDLValid)
			continue;

		switch (prNDL->eCurrentNDLMgmtState) {
		case NDL_REQUEST_SCHEDULE_NDL:
		case NDL_INITIATOR_TX_SCHEDULE_REQUEST:
		case NDL_RESPONDER_RX_SCHEDULE_REQUEST:
		case NDL_RESPONDER_TX_SCHEDULE_RESPONSE:
		case NDL_INITIATOR_RX_SCHEDULE_RESPONSE:
			return TRUE;
		default:
			break;
		}
	}

	return FALSE;
}

uint32_t nanReschedGetNdlNdc(struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
	struct _NAN_SCHEDULE_TIMELINE_T rNdcTimeline;
	uint32_t u4SlotIdx = NAN_INVALID_SLOT_INDEX;
	uint32_t u4NdlIdx = 0;
	uint32_t u4Idx = 0;

	prDataPathInfo = &prAdapter->rDataPathInfo;
	for (u4NdlIdx = 0; u4NdlIdx < NAN_MAX_SUPPORT_NDL_NUM; u4NdlIdx++) {
		prNDL = &prDataPathInfo->arNDL[u4NdlIdx];
		if (!prNDL->fgNDLValid)
			continue;

		switch (prNDL->eCurrentNDLMgmtState) {
		case NDL_REQUEST_SCHEDULE_NDL:
		case NDL_INITIATOR_TX_SCHEDULE_REQUEST:
		case NDL_RESPONDER_RX_SCHEDULE_REQUEST:
		case NDL_RESPONDER_TX_SCHEDULE_RESPONSE:
		case NDL_INITIATOR_RX_SCHEDULE_RESPONSE:
			prPeerSchDesc = nanSchedSearchPeerSchDescByNmi(
					prAdapter, prNDL->aucPeerMacAddr);

			if (prPeerSchDesc == NULL) {
				DBGLOG(NAN, DEBUG, "PeerSchDesc not found\n");
				return NAN_INVALID_SLOT_INDEX;
			}

			for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
				rNdcTimeline = prPeerSchDesc->rSelectedNdcCtrl
						.arTimeline[u4Idx];
				if (rNdcTimeline.ucMapId == NAN_INVALID_MAP_ID)
					continue;

				u4SlotIdx = nanGetNdcSlotIdxFromAvailMap(
						rNdcTimeline.au4AvailMap[0]);
				if (u4SlotIdx != NAN_INVALID_SLOT_INDEX)
					break;
			}
			DBGLOG(NAN, INFO, "Current ReschedNDL NDC SlotIdx=%u\n",
					u4SlotIdx);
			break;
		default:
			break;
		}
	}

	return u4SlotIdx;
}
#endif /* CFG_SUPPORT_NAN_RESCHEDULE  */

#endif /* CFG_SUPPORT_NAN */
