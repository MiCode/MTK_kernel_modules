// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#include "precomp.h"
#include "nan_data_engine.h"
#include "nan_base.h"

/* for action frame subtypes */
/* #include "nan_rxm.h" */
/* #include "nan_sec.h" */

/* for NAN schedule hooks */
#include "nanScheduler.h"

#include "gl_vendor_ndp.h"
#include "nan_sec.h"
#include "nan_txm.h"
#include "wpa_supp/src/crypto/sha256_i.h"

/* #if (CFG_NAN_DATAENGINE == 1) */

/*******************************************************************************
 *                   E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            C O N S T A N T S
 *******************************************************************************
 */
#define UINT8_MAX 0xFF

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

static struct _APPEND_ATTR_ENTRY_T txDataAttributeTable[] = {
	/*   ATTR-ID       fp for calc-var-len     fp for attr appending */
	{ NAN_ATTR_ID_DEVICE_CAPABILITY, nanDataEngineDevCapAttrLength,
	  nanDataEngineDevCapAttrAppend }, /* 0xF */
	{ NAN_ATTR_ID_NDP, nanDataEngineNDPAttrLength,
	  nanDataEngineNDPAttrAppend }, /* 0x10 */
	{ NAN_ATTR_ID_NAN_AVAILABILITY, nanDataEngineNanAvailAttrLength,
	  nanDataEngineNanAvailAttrAppend }, /* 0x12 */
	{ NAN_ATTR_ID_NDC, nanDataEngineNdcAttrLength,
	  nanDataEngineNdcAttrAppend }, /* 0x13 */
	{ NAN_ATTR_ID_NDL, nanDataEngineNDLAttrLength,
	  nanDataEngineNDLAttrAppend }, /* 0x14 */
	{ NAN_ATTR_ID_NDL_QOS, nanDataEngineNdlQosAttrLength,
	  nanDataEngineNdlQosAttrAppend }, /* 0x15 */
	{ NAN_ATTR_ID_UNALIGNED_SCHEDULE, nanDataEngineUnalignedAttrLength,
	  nanDataEngineUnalignedAttrAppend }, /* 0x17 */
	{ NAN_ATTR_ID_ELEMENT_CONTAINER, nanDataEngineElemContainerAttrLength,
	  nanDataEngineElemContainerAttrAppend }, /* 0x1D */
	{ NAN_ATTR_ID_CIPHER_SUITE_INFO, nanDataEngineCipherSuiteAttrLength,
	  nanDataEngineCipherSuiteAttrAppend }, /* 0x22 */
	{ NAN_ATTR_ID_SECURITY_CONTEXT_INFO, nanDataEngineSecContextAttrLength,
	  nanDataEngineSecContextAttrAppend }, /* 0x23 */
	{ NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR, nanDataEngineSharedKeyAttrLength,
	  nanDataEngineSharedKeyAttrAppend }, /* 0x24 */
	{ NAN_ATTR_ID_NDP_EXTENSION, nanDataEngineNDPEAttrLength,
	  nanDataEngineNDPEAttrAppend }, /* 0x29 */
	{ NAN_ATTR_ID_VENDOR_SPECIFIC, nanDataEngineVendorSpecificAttrLength,
	  nanDataEngineVendorSpecificAttrAppend }, /* 0xDD */
};

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
#define NAN_MC_TX_LOOKUP_TBL_NUM	(16)
struct _NAN_MC_TX_LOOKUP g_arNanMcTxLookupTbl[NAN_MC_TX_LOOKUP_TBL_NUM];
struct _NAN_MCDATA_RX_TEST_PATH g_rNanMcDataRxTestPath;
#endif

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static uint8_t *apucDebugDataMgmtState[NDL_MGMT_STATE_NUM] = {
	(uint8_t *)DISP_STRING("NDL_IDLE"),
	(uint8_t *)DISP_STRING("NDL_REQUEST_SCHEDULE_NDP"),
	(uint8_t *)DISP_STRING("NDL_REQUEST_SCHEDULE_NDL"),
	(uint8_t *)DISP_STRING("NDL_SCHEDULE_SETUP"),
	(uint8_t *)DISP_STRING("NDL_INITIATOR_TX_SCHEDULE_REQUEST"),
	(uint8_t *)DISP_STRING("NDL_RESPONDER_RX_SCHEDULE_REQUEST"),
	(uint8_t *)DISP_STRING("NDL_RESPONDER_TX_SCHEDULE_RESPONSE"),
	(uint8_t *)DISP_STRING("NDL_INITIATOR_RX_SCHEDULE_RESPONSE"),
	(uint8_t *)DISP_STRING("NDL_INITIATOR_TX_SCHEDULE_CONFIRM"),
	(uint8_t *)DISP_STRING("NDL_RESPONDER_RX_SCHEDULE_CONFIRM"),
	(uint8_t *)DISP_STRING("NDL_SCHEDULE_ESTABLISHED"),
	(uint8_t *)DISP_STRING("NDL_TEARDOWN_BY_NDP_TERMINATION"),
	(uint8_t *)DISP_STRING("NDL_TEARDOWN"),
	(uint8_t *)DISP_STRING("NDL_INITIATOR_RX_WAITFOR_SCHEDULE_RESPONSE"),
};

static uint8_t *apucDebugDataPathProtocolState[NDP_PROTOCOL_STATE_NUM] = {
	(uint8_t *)DISP_STRING("NDP_IDLE"),
	(uint8_t *)DISP_STRING("NDP_INITIATOR_TX_DP_REQUEST"),
	(uint8_t *)DISP_STRING("NDP_INITIATOR_RX_DP_RESPONSE"),
	(uint8_t *)DISP_STRING("NDP_INITIATOR_TX_DP_CONFIRM"),
	(uint8_t *)DISP_STRING("NDP_INITIATOR_RX_DP_SECURITY_INSTALL"),
	(uint8_t *)DISP_STRING("NDP_RESPONDER_WAIT_DATA_RSP"),
	(uint8_t *)DISP_STRING("NDP_RESPONDER_TX_DP_RESPONSE"),
	(uint8_t *)DISP_STRING("NDP_RESPONDER_RX_DP_CONFIRM"),
	(uint8_t *)DISP_STRING("NDP_RESPONDER_TX_DP_SECURITY_INSTALL"),
	(uint8_t *)DISP_STRING("NDP_NORMAL_TR"),
	(uint8_t *)DISP_STRING("NDP_TX_DP_TERMINATION"),
	(uint8_t *)DISP_STRING("NDP_DISCONNECT"),
};

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchEmptyNdlEntry(struct ADAPTER *prAdapter);

static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchEmptyNdpEntry(struct ADAPTER *prAdapter,
			       struct _NAN_NDL_INSTANCE_T *prNDL);

static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByNdpId(struct ADAPTER *prAdapter,
			    struct _NAN_NDL_INSTANCE_T *prNDL, uint8_t ucNdpId);

static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByNdpIdOnly(struct ADAPTER *prAdapter, uint8_t ucNdpId);

#if 0
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByPeerNDI(
	struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL,
	IN PUINT_8 pucPeerNDIAddr
);

static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByPublishId(
	struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL,
	UINT_8 ucPublishId
);
#endif
static struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchNdlByStaRec(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec);

static uint8_t nanDataUtilGenerateNdpId(struct ADAPTER *prAdapter,
				       struct _NAN_NDL_INSTANCE_T *prNDL);

static struct _NAN_NDP_INSTANCE_T *nanDataAllocateNdp(
	IN struct ADAPTER *prAdapter, IN struct _NAN_NDL_INSTANCE_T *prNDL,
	IN enum _ENUM_NAN_PROTOCOL_ROLE_T eNDPRole, IN uint8_t *pucPeerNDIAddr,
	IN uint8_t ucNDPID, IN unsigned char fgSecurityRequired);

static void nanDataFreeNdp(struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP);

static struct _NAN_NDL_INSTANCE_T *
nanDataAllocateNdl(IN struct ADAPTER *prAdapter, IN uint8_t *pucMacAddr,
		   IN enum _ENUM_NAN_PROTOCOL_ROLE_T eRole);

static void nanDataFreeNdl(IN struct ADAPTER *prAdapter,
			   struct _NAN_NDL_INSTANCE_T *prNDL);

/* callback function for NAN scheduler */
static void nanDataPathScheduleNegoGranted(struct ADAPTER *prAdapter,
					   uint8_t *pu1DevAddr,
					   enum _ENUM_NAN_NEGO_TYPE_T eType,
					   enum _ENUM_NAN_NEGO_ROLE_T eRole,
					   void *pvToken);

/* static machine operations */
static void nanNdlMgmtFsmStep(IN struct ADAPTER *prAdapter,
			      IN enum _ENUM_NDL_MGMT_STATE_T eNextState,
			      IN struct _NAN_NDL_INSTANCE_T *prNDL);

static enum _ENUM_NAN_NDP_STATUS_T
nanDataPathProtocolFsmStep(IN struct ADAPTER *prAdapter,
			   IN enum _ENUM_NDP_PROTOCOL_STATE_T eNextState,
			   IN struct _NAN_NDP_INSTANCE_T *prNDP);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void
nanDataResponseTimeout(struct ADAPTER *prAdapter, unsigned long ulParam) {
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prNDP = (struct _NAN_NDP_INSTANCE_T *)ulParam;

	if (prNDP != NULL && prNDP->fgNDPValid == TRUE) {
		prNDP->eDataPathFailReason =
			DP_REASON_USER_SPACE_RESPONSE_TIMEOUT;

		/* Because NAN security SM init only after data response
		 * accept the connection establishment, to reset
		 * NDP security required to avoid touch security SM.
		 */
		prNDP->fgSecurityRequired = FALSE;

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
nanDataProtocolTimeout(struct ADAPTER *prAdapter, unsigned long ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prNDL = (struct _NAN_NDL_INSTANCE_T *)ulParam;

	if (prNDL != NULL) {
		if (prNDL->prOperatingNDP != NULL) {
			prNDP = prNDL->prOperatingNDP;
			prNDP->eDataPathFailReason = DP_REASON_RX_TIMEOUT;
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
		} else
			nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN, prNDL);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
nanDataRetryTimeout(struct ADAPTER *prAdapter, unsigned long ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prNDL = (struct _NAN_NDL_INSTANCE_T *)ulParam;

	if (prNDL != NULL && prNDL->prOperatingNDP != NULL) {
		prNDP = prNDL->prOperatingNDP;

		prNDP->eDataPathFailReason = DP_REASON_RX_TIMEOUT;

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
nanDataSecurityTimeout(struct ADAPTER *prAdapter, unsigned long ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prNDL = (struct _NAN_NDL_INSTANCE_T *)ulParam;

	if (prNDL != NULL && prNDL->prOperatingNDP != NULL) {
		prNDP = prNDL->prOperatingNDP;

		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));

		prNDP->eDataPathFailReason = DP_REASON_SECURITY_TIMEOUT;
		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief        Callback from NAN-Scheduler for
 *               granting start of schedule negotiatio
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void
nanDataPathScheduleNegoGranted(struct ADAPTER *prAdapter, uint8_t *pu1DevAddr,
			       enum _ENUM_NAN_NEGO_TYPE_T eType,
			       enum _ENUM_NAN_NEGO_ROLE_T eRole,
			       void *pvToken) {
	struct _NAN_DATA_ENGINE_SCHEDULE_TOKEN_T *prDataEngineToken;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prDataEngineToken = (struct _NAN_DATA_ENGINE_SCHEDULE_TOKEN_T *)pvToken;

	prNDL = prDataEngineToken->prNDL;

	if (prNDL == NULL ||
	    (prNDL->eCurrentNDLMgmtState != NDL_REQUEST_SCHEDULE_NDP &&
	     prNDL->eCurrentNDLMgmtState != NDL_REQUEST_SCHEDULE_NDL)) {
		/* aborted request ?
		 * return negotiation permission back to NAN-Scheduler
		 */
		if (prNDL) {
			prNDL->fgScheduleUnderNegotiation = FALSE;
			DBGLOG(NAN, WARN,
			       "NAN Data Engine: unexpected scheduler negotiation [%s]\n",
			       apucDebugDataMgmtState
				       [prNDL->eCurrentNDLMgmtState]);

		} else {
			DBGLOG(NAN, WARN,
			       "NAN Data Engine: unexpected scheduler negotiation [%s]\n",
			       "NULL");
		}

		nanSchedNegoStop(prAdapter);

		return;
	}

	prNDL->fgScheduleUnderNegotiation = TRUE;
	nanSchedNegoAddQos(prAdapter, prNDL->ucMinimumTimeSlot,
			   prNDL->u2MaximumLatency);
	if (prNDL->eCurrentNDLMgmtState == NDL_REQUEST_SCHEDULE_NDP)
		nanNdlMgmtFsmStep(prAdapter, NDL_SCHEDULE_SETUP, prNDL);
	else if (prNDL->eCurrentNDLMgmtState == NDL_REQUEST_SCHEDULE_NDL) {
		if (prNDL->eNDLRole == NAN_PROTOCOL_RESPONDER) {
			nanNdlMgmtFsmStep(prAdapter,
					  NDL_RESPONDER_RX_SCHEDULE_REQUEST,
					  prNDL);
		} else if (prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR) {
			if (nanSchedNegoGenLocalCrbProposal(prAdapter) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, WARN, "Reject by scheduler\n");
				prNDL->eCurrentNDLMgmtState = NDL_TEARDOWN;
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			}
			/* send Data Path Request */
			nanNdlMgmtFsmStep(prAdapter,
					  NDL_INITIATOR_TX_SCHEDULE_REQUEST,
					  prNDL);
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchEmptyNdlEntry(struct ADAPTER *prAdapter) {
	uint8_t ucNdlIndex;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		if (prAdapter->rDataPathInfo.arNDL[ucNdlIndex].fgNDLValid ==
		    FALSE) {
			prNDL = &(prAdapter->rDataPathInfo.arNDL[ucNdlIndex]);
			DBGLOG(NAN, INFO, "ucNdlIndex:%d\n",
			       ucNdlIndex);
			prNDL->ucIndex = ucNdlIndex;
			break;
		}
	}

	return prNDL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchEmptyNdpEntry(struct ADAPTER *prAdapter,
			       struct _NAN_NDL_INSTANCE_T *prNDL) {
	uint8_t ucNdpIndex;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error, return NULL\n");
		return NULL;
	}

	for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
	     ucNdpIndex++) {
		if (prNDL->arNDP[ucNdpIndex].fgNDPValid == FALSE) {
			prNDP = &prNDL->arNDP[ucNdpIndex];
			DBGLOG(NAN, INFO,
			       "ucNdpIndex:%d, prInitiatorSecSmInfo:0x%p\n",
			       ucNdpIndex,
			       prNDL->arNDP[ucNdpIndex].prInitiatorSecSmInfo);
			return prNDP;
		}
	}

	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByNdpId(struct ADAPTER *prAdapter,
		struct _NAN_NDL_INSTANCE_T *prNDL, uint8_t ucNdpId) {
	uint8_t ucNdpIndex;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return NULL;
	}

	if (prNDL->fgNDLValid == TRUE) {
		for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
		     ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
			    prNDL->arNDP[ucNdpIndex].ucNDPID == ucNdpId)
				return &(prNDL->arNDP[ucNdpIndex]);
		}
	}

	/* no NDP matched */
	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByNdpIdOnly(struct ADAPTER *prAdapter, uint8_t ucNdpId) {
	uint8_t ucNdlIndex, ucNdpIndex;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error, return NULL\n");
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		if (prDataPathInfo->arNDL[ucNdlIndex].fgNDLValid == TRUE) {
			prNDL = &(prDataPathInfo->arNDL[ucNdlIndex]);

			for (ucNdpIndex = 0;
			     ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
			     ucNdpIndex++) {
				if (prNDL->arNDP[ucNdpIndex].fgNDPValid ==
					    TRUE &&
				    prNDL->arNDP[ucNdpIndex].ucNDPID == ucNdpId)
					return &(prNDL->arNDP[ucNdpIndex]);
			}
		}
	}

	return NULL;
}

#if 0
/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByPeerNDI(
	struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL,
	IN PUINT_8 pucPeerNDIAddr
)
{
	UINT_8 ucNdpIndex;

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error, return NULL\n", __func__);
		return NULL;
	}

	if (prNDL->fgNDLValid == TRUE) {
		for (ucNdpIndex = 0;
			ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM; ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
				EQUAL_MAC_ADDR(
				prNDL->arNDP[ucNdpIndex].aucPeerNDIAddr,
				pucRemoteNDIAddr))
				return &(prNDL->arNDP[ucNdpIndex]);
		}
	}

	/*no NDP matched*/
	return NULL;
}

/*----------------------------------------------------------------------------*/
/*!
* \brief
*
* \param[in]
*
* \return none
*/
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByPublishId(
	struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL,
	UINT_8 ucPublishId
)
{
	UINT_8 ucNdpIndex;

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error, return NULL\n", __func__);
		return NULL;
	}

	if (prNDL->fgNDLValid == TRUE) {
		for (ucNdpIndex = 0;
			ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM; ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
				prNDL->arNDP[ucNdpIndex].ucPublishId
				 == ucPublishId)
				return &(prNDL->arNDP[ucNdpIndex]);
		}
	}

	/* no NDP matched */
	return NULL;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchNdlByMac(struct ADAPTER *prAdapter, uint8_t *pucAddr) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint32_t u4Idx;
	uint8_t *pucMacAddr;

	uint8_t ucNdlIndex; /* ucNdpIndex */
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Search %02x:%02x:%02x:%02x:%02x:%02x\n",
	       pucAddr[0], pucAddr[1], pucAddr[2], pucAddr[3],
	       pucAddr[4], pucAddr[5]);
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error, return NULL\n");
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		if (prDataPathInfo->arNDL[ucNdlIndex].fgNDLValid == TRUE) {
			prNDL = &(prDataPathInfo->arNDL[ucNdlIndex]);
			DBGLOG(NAN, LOUD,
			       "NDL Peer %02x:%02x:%02x:%02x:%02x:%02x\n",
			       prNDL->aucPeerMacAddr[0],
			       prNDL->aucPeerMacAddr[1],
			       prNDL->aucPeerMacAddr[2],
			       prNDL->aucPeerMacAddr[3],
			       prNDL->aucPeerMacAddr[4],
			       prNDL->aucPeerMacAddr[5]);
			if (EQUAL_MAC_ADDR(prNDL->aucPeerMacAddr, pucAddr))
				return prNDL;

			for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_NUM;
			     u4Idx++) {
				if (prNDL->arNDP[u4Idx].fgNDPValid == FALSE)
					continue;
				pucMacAddr = prNDL->arNDP[u4Idx].aucPeerNDIAddr;
				DBGLOG(NAN, LOUD,
				       "NDP#%d Peer %02x:%02x:%02x:%02x:%02x:%02x\n",
				       u4Idx, pucMacAddr[0], pucMacAddr[1],
				       pucMacAddr[2], pucMacAddr[3],
				       pucMacAddr[4], pucMacAddr[5]);
				if (EQUAL_MAC_ADDR(pucMacAddr, pucAddr))
					return prNDL;
			}
		}
	}

	return NULL;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchNdlByStaRec(struct ADAPTER *prAdapter,
			     struct STA_RECORD *prStaRec) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint8_t ucNdlIndex; /*ucNdpIndex*/
	uint8_t ucNdpCxtIdx;
	struct _NAN_NDP_CONTEXT_T *prNdpCxt;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error, return NULL\n");
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		prNDL = &prDataPathInfo->arNDL[ucNdlIndex];
		if (prNDL->fgNDLValid == FALSE)
			continue;

		for (ucNdpCxtIdx = 0; ucNdpCxtIdx < NAN_MAX_SUPPORT_NDP_CXT_NUM;
		     ucNdpCxtIdx++) {
			prNdpCxt = &prNDL->arNdpCxt[ucNdpCxtIdx];
			if (prNdpCxt->fgValid == FALSE)
				continue;

			if (prNdpCxt->prNanStaRec == prStaRec)
				return prNDL;
		}
	}

	return NULL;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static uint8_t
nanDataUtilGenerateNdpId(struct ADAPTER *prAdapter,
			 struct _NAN_NDL_INSTANCE_T *prNDL) {
	unsigned char fgConflict;
	uint8_t ucNdpIndex;
	uint8_t ucNdpId;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return 0;
	}

	if (prNDL->fgNDLValid != TRUE) {
		DBGLOG(NAN, ERROR, "fgNDLValid not valid\n");
		return 0;
	}

	ucNdpId = prNDL->ucSeqNum;

	/* ID conflict check */
	do {
		fgConflict = FALSE;

		/* incremental */
		if (ucNdpId < 255)
			ucNdpId++;
		else
			ucNdpId = 1; /* keep 0 as reserved purposes ... */

		for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
		     ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
			    prNDL->arNDP[ucNdpIndex].ucNDPID == ucNdpId) {
				fgConflict = TRUE;
				break;
			}
		}

		if (fgConflict == FALSE) /* non-conflict */
			break;
	} while (fgConflict == TRUE);

	/* update to local tracker */
	prNDL->ucSeqNum = ucNdpId;

	return ucNdpId;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in] prNDL
 *       [in] eNDPRole
 *       [in] ucNDPID - only valid when eNDPRole == RESPONDER
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDP_INSTANCE_T *
nanDataAllocateNdp(struct ADAPTER *prAdapter,
		   IN struct _NAN_NDL_INSTANCE_T *prNDL,
		   IN enum _ENUM_NAN_PROTOCOL_ROLE_T eNDPRole,
		   IN uint8_t *pucPeerNDIAddr, IN uint8_t ucNDPID,
		   IN unsigned char fgSecurityRequired) {
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error, return NULL\n");
		return NULL;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error, return NULL\n");
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	/* NDP-ID duplication check */
	if (nanDataUtilSearchNdpByNdpId(prAdapter, prNDL, ucNDPID) != NULL) {
		DBGLOG(NAN, ERROR, "NDP-ID duplication\n");
		return NULL;
	}

	prNDP = nanDataUtilSearchEmptyNdpEntry(prAdapter, prNDL);
	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return NULL;
	}

	/* fill data in NDP */
	prNDP->eNDPRole = eNDPRole;
	if (eNDPRole == NAN_PROTOCOL_RESPONDER)
		prNDP->ucNDPID = ucNDPID;
	else if (eNDPRole == NAN_PROTOCOL_INITIATOR)
		prNDP->ucNDPID = nanDataUtilGenerateNdpId(prAdapter, prNDL);
	else {
		DBGLOG(NAN, ERROR, "eNDPRole invalid\n");
		return NULL;
	}

	COPY_MAC_ADDR(prNDP->aucPeerNDIAddr, pucPeerNDIAddr);
	prNDP->fgSecurityRequired = fgSecurityRequired;
	nanDataUpdateNdpLocalNDI(prAdapter, prNDP);
	prNDP->fgNDPActive = FALSE;
	prNDP->ucTxRetryCounter = 0;
	prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_CONTINUED;
	prNDP->ucReasonCode = NAN_REASON_CODE_RESERVED;
	prNDP->fgQoSRequired = FALSE;
	prNDP->fgConfirmRequired = FALSE;
	prNDP->fgRejectPending = FALSE;
	prNDP->fgSupportNDPE =
		nanGetFeaturePeerNDPE(prAdapter, prNDL->aucPeerMacAddr);
	prNDP->fgCarryIPV6 = FALSE;
	prNDP->ucCipherType = 0;
	/* Need security module data structure */
	prNDP->fgNDPValid = TRUE;
	/* prNDP->rkey_info = ; */
	prNDP->u2KdeLen = 0;
	prNDP->pucKdeInfo = NULL;

	prNDP->u2AppInfoLen = 0;
	prNDP->pucAppInfo = NULL;
	prNDP->u2PortNum = 0;
	prNDP->ucProtocolType = 0xFF;
	prNDP->ucServiceProtocolType = NAN_SERVICE_PROTOCOL_TYPE_GENERIC;

	prNDP->u2OtherAppInfoLen = 0;
	prNDP->pucOtherAppInfo = NULL;

	prNDP->u2AttrListLength = 0;
	prNDP->pucAttrList = NULL;

	/* prNDP->aucNanMulticastAddr = NULL; */
	prNDP->eCurrentNDPProtocolState = NDP_IDLE;
	prNDP->eLastNDPProtocolState = NDP_IDLE;
	prNDP->fgNDPEstablish = FALSE;
	prNDP->prContext = NULL;

	/* Need security module data structure
	 * wpa_sm p_wpa_sm;
	 * sta_info prStaInfo;
	 */
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	prNDP->fgTxGtkRequired = FALSE;
	prNDP->ucTxSecurityCap = 0;
	prNDP->ucTxGtkCipherType = NAN_CIPHER_SUITE_ID_NONE;
	prNDP->fgRxGtkRequired = FALSE;
	prNDP->ucRxSecurityCap = 0;
	prNDP->ucRxGtkCipherType = NAN_CIPHER_SUITE_ID_NONE;
	prNDP->ucGroupSAUsed = FALSE;
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

	if (!timerPendingTimer(&(prNDP->rNDPUserSpaceResponseTimer))) {
		cnmTimerInitTimer(prAdapter,
			  &(prNDP->rNDPUserSpaceResponseTimer),
			  (PFN_MGMT_TIMEOUT_FUNC)nanDataResponseTimeout,
			  (unsigned long)prNDP,
			  TIMER_WAKELOCK_AUTO);
	}
	if (prNDL->ucNDPNum < UINT8_MAX)
		prNDL->ucNDPNum++;
	DBGLOG(NAN, INFO, "Create NDP ID [%u]\n", prNDP->ucNDPID);
	return prNDP;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
nanDataUpdateNdpPeerNDI(IN struct ADAPTER *prAdapter,
			struct _NAN_NDP_INSTANCE_T *prNDP,
			IN uint8_t *pucPeerNDIAddr) {

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	if (UNEQUAL_MAC_ADDR(prNDP->aucPeerNDIAddr, pucPeerNDIAddr))
		COPY_MAC_ADDR(prNDP->aucPeerNDIAddr, pucPeerNDIAddr);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
nanDataUpdateNdpLocalNDI(IN struct ADAPTER *prAdapter,
			 struct _NAN_NDP_INSTANCE_T *prNDP) {

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	if (UNEQUAL_MAC_ADDR(prNDP->aucLocalNDIAddr,
		prAdapter->rDataPathInfo.aucLocalNDIAddr))
		COPY_MAC_ADDR(prNDP->aucLocalNDIAddr,
			prAdapter->rDataPathInfo.aucLocalNDIAddr);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void
nanDataFreeNdp(struct ADAPTER *prAdapter, struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);
	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return;
	}

	if (prNDP->u2KdeLen) {
		prNDP->pucKdeInfo = NULL;
		prNDP->u2KdeLen = 0;
	}

	if (prNDP->pucAppInfo != NULL) {
		cnmMemFree(prAdapter, prNDP->pucAppInfo);
		prNDP->u2AppInfoLen = 0;
		prNDP->pucAppInfo = NULL;
	}

	if (prDataPathInfo->pucAppInfo != NULL) {
		cnmMemFree(prAdapter, prDataPathInfo->pucAppInfo);
		prDataPathInfo->u2AppInfoLen = 0;
		prDataPathInfo->pucAppInfo = NULL;
	}

	if (prNDP->pucPeerAppInfo != NULL) {
		cnmMemFree(prAdapter, prNDP->pucPeerAppInfo);
		prNDP->u2PeerAppInfoLen = 0;
		prNDP->pucPeerAppInfo = NULL;
	}

	if (prNDP->pucAttrList != NULL) {
		/* TODO_CJ: free crash*/
		cnmMemFree(prAdapter, prNDP->pucAttrList);
		prNDP->u2AttrListLength = 0;
		prNDP->pucAttrList = NULL;
	}

	prNDP->fgNDPValid = FALSE;

	if (prNDL->ucNDPNum > 0)
		prNDL->ucNDPNum--;

	nanDataEngineUnrollNDPContext(prAdapter, prNDL, prNDP);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static struct _NAN_NDL_INSTANCE_T *
nanDataAllocateNdl(struct ADAPTER *prAdapter, IN uint8_t *pucMacAddr,
		   IN enum _ENUM_NAN_PROTOCOL_ROLE_T eRole) {
	/* UINT_8 ucNdlIndex; */
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	uint8_t ucNdpCxtIdx;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error, return NULL\n");
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	prNDL = nanDataUtilSearchEmptyNdlEntry(prAdapter);

	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return NULL;
	}

	prNDL->fgNDLValid = TRUE;

	/* fill data in NDL*/
	COPY_MAC_ADDR(prNDL->aucPeerMacAddr, pucMacAddr);
	prNDL->eNDLRole = eRole;
	prNDL->eCurrentNDLMgmtState = NDL_IDLE;
	prNDL->eLastNDLMgmtState = NDL_IDLE;
	prNDL->u2MaximumIdlePeriod = 0;
	prNDL->prOperatingNDP = NULL;
	prNDL->ucSeqNum = 0;
	prNDL->fgPagingRequired = FALSE;
	prNDL->fgScheduleEstablished = FALSE;
	prNDL->fgRejectPending = FALSE;
	prNDL->fgCarryImmutableSchedule = FALSE;
	prNDL->fgIsCounter = FALSE;

	prNDL->ucMinimumTimeSlot = NAN_INVALID_QOS_MIN_SLOTS;
	prNDL->u2MaximumLatency = NAN_INVALID_QOS_MAX_LATENCY;

	prNDL->ucTxRetryCounter = 0;

	prNDL->ucPhyTypeSet = 0;
	kalMemZero(&(prNDL->rIeHtCap), sizeof(struct IE_HT_CAP));
	kalMemZero(&(prNDL->rIeVhtCap), sizeof(struct IE_VHT_CAP));
#if (CFG_SUPPORT_NAN_R4 == 1) && (CFG_SUPPORT_802_11AX == 1)
	kalMemZero(&(prNDL->aucIeHeCap), sizeof(prNDL->aucIeHeCap));
#endif

	for (ucNdpCxtIdx = 0; ucNdpCxtIdx < NAN_MAX_SUPPORT_NDP_CXT_NUM;
	     ucNdpCxtIdx++)
		prNDL->arNdpCxt[ucNdpCxtIdx].fgValid = FALSE;

	prNDL->ucNDLSetupCurrentStatus = NAN_ATTR_NDL_STATUS_CONTINUED;
	prNDL->ucReasonCode = NAN_REASON_CODE_RESERVED;

	/* timer initialization sequence */
	cnmTimerInitTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer),
			  (PFN_MGMT_TIMEOUT_FUNC)nanDataProtocolTimeout,
			  (unsigned long)prNDL,
			  TIMER_WAKELOCK_AUTO);

	cnmTimerInitTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer),
			  (PFN_MGMT_TIMEOUT_FUNC)nanDataRetryTimeout,
			  (unsigned long)prNDL,
			  TIMER_WAKELOCK_AUTO);

	cnmTimerInitTimer(prAdapter, &(prNDL->rNDPSecurityExpireTimer),
			  (PFN_MGMT_TIMEOUT_FUNC)nanDataSecurityTimeout,
			  (unsigned long)prNDL,
			  TIMER_WAKELOCK_AUTO);

	LINK_INITIALIZE(&(prNDL->rPendingReqList));

	prDataPathInfo->ucNDLNum++;

	return prNDL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct _NAN_NDL_INSTANCE_T *
nanDataUtilGetNdl(struct ADAPTER *prAdapter,
		  struct _NAN_NDP_INSTANCE_T *prNDP)
{
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error, return NULL\n");
		return NULL;
	}

	return &(prAdapter->rDataPathInfo.arNDL[prNDP->ucNdlIndex]);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
static void
nanDataFreeNdl(struct ADAPTER *prAdapter, struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint8_t ucNdpIndex;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return;
	}

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	nanDataEngineFlushRequest(prAdapter, prNDL);

	/* 1. resource releasing by state machine */
	if (prNDL->eCurrentNDLMgmtState != NDL_IDLE) {
		/* TODO: do some thing to release resource hold
		 *       by state machine
		 * TODO: free CRB
		 */
	}

	/* 2. also invalidate remaining NDPs belongs to this NDL */
	if (prNDL->ucNDPNum) {
		for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
		     ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE) {
				prNDP = &(prNDL->arNDP[ucNdpIndex]);

				/* send data termination indication
				 * to host layer
				 */
				nanNdpSendDataTerminationEvent(prAdapter,
							       prNDP);

				if (prNDP->fgSecurityRequired)
					nanSecNotify4wayTerminate(prNDP);

				nanDataFreeNdp(prAdapter, prNDP);
			}
		}
	}

	/* 3. invalidate NDL */
	prNDL->fgNDLValid = FALSE;
	prNDL->fgIsCounter = FALSE;
	prDataPathInfo->ucNDLNum--;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanDataEngineInit(struct ADAPTER *prAdapter, IN uint8_t *pu1NMIAddress) {
	uint8_t i, j;
	uint32_t u4offset;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	enum NAN_BSS_ROLE_INDEX eRole = NAN_BSS_INDEX_MAIN;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	kalMemZero(prDataPathInfo, sizeof(struct _NAN_DATA_PATH_INFO_T));

	prDataPathInfo->ucNDLNum = 0;
	COPY_MAC_ADDR(prDataPathInfo->aucLocalNMIAddr, pu1NMIAddress);
	COPY_MAC_ADDR(prDataPathInfo->aucLocalNDIAddr, pu1NMIAddress);

	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		for (j = 0; j < NAN_MAX_SUPPORT_NDP_NUM; j++) {
			/* zero-ize */
			kalMemZero(&(prDataPathInfo->arNDL[i].arNDP[j]),
				   sizeof(struct _NAN_NDP_INSTANCE_T));
			prDataPathInfo->arNDL[i].arNDP[j].ucNdlIndex = i;
			u4offset = i * NAN_MAX_SUPPORT_NDP_NUM + j;
			prDataPathInfo->arNDL[i].arNDP[j].prInitiatorSecSmInfo =
				nanSecGetInitiatorSm(
					(uint8_t)(u4offset));
			nanInitSecInitiatorSm(
				prDataPathInfo->arNDL[i].arNDP[j]
				.prInitiatorSecSmInfo);
			prDataPathInfo->arNDL[i].arNDP[j].prResponderSecSmInfo =
				nanSecGetResponderSm(
					(uint8_t)(u4offset));
			nanInitSecResponderSm(
				prDataPathInfo->arNDL[i].arNDP[j]
				.prResponderSecSmInfo);
		}

		/* initialize token for NAN scheduler */
		prDataPathInfo->arNDL[i].rToken.prNDL =
			&(prDataPathInfo->arNDL[i]);
		prDataPathInfo->arNDL[i].rToken.prAdapter = prAdapter;
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
	/* initialize MC Rx Data wtbl idx */
	kalMemZero(g_arNanMcRxWtbl, sizeof(g_arNanMcRxWtbl));
	for (i = 0; i < NAN_MAX_MC_RX_WTBL_NUM; i++)
		g_arNanMcRxWtbl[i].u2WtblIdx = WTBL_RESERVED_ENTRY;

	kalMemZero(g_arNanMcTxLookupTbl, sizeof(g_arNanMcTxLookupTbl));
#endif

	kalMemZero(prDataPathInfo->aucECAttr,
		   sizeof(prDataPathInfo->aucECAttr));
	prDataPathInfo->prLocalHtCap = NULL;
	prDataPathInfo->prLocalVhtCap = NULL;
#if (CFG_SUPPORT_NAN_R4 == 1) && (CFG_SUPPORT_802_11AX == 1)
	prDataPathInfo->prLocalHeCap = NULL;
#endif

	atomic_set(&(prDataPathInfo->NetDevRefCount[eRole]), 0);
	g_ndpReqNDPE.fgEnNDPE = FALSE;
	prDataPathInfo->fgAutoHandleDPRequest = FALSE;
	prDataPathInfo->ucDPResponseDecisionStatus =
		NAN_DATA_RESP_DECISION_ACCEPT;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanDataEngineUninit(struct ADAPTER *prAdapter) {
	uint8_t i, j;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	enum NAN_BSS_ROLE_INDEX eRole = NAN_BSS_INDEX_MAIN;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (atomic_read(&(prDataPathInfo->NetDevRefCount[eRole])) > 0) {
		/* netif_carrier_off */
		kalNanIndicateStatusAndComplete(
			prAdapter->prGlueInfo,
			WLAN_STATUS_MEDIA_DISCONNECT, eRole);
	}

	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		if (prDataPathInfo->arNDL[i].fgNDLValid == TRUE) {
			for (j = 0; j < NAN_MAX_SUPPORT_NDP_NUM; j++) {
				prNDP = &prDataPathInfo->arNDL[i].arNDP[j];
				if (prNDP->fgNDPValid == TRUE) {
					if (prNDP->fgSecurityRequired)
						nanSecNotify4wayTerminate(
							prNDP);

					nanDataFreeNdp(prAdapter, prNDP);
				}
			}

			nanDataFreeNdl(prAdapter, &(prDataPathInfo->arNDL[i]));
		}
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanDataPathSetupSuccess(struct ADAPTER *prAdapter,
			IN struct _NAN_NDP_INSTANCE_T *prNDP) {
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	prNDP->fgNDPActive = TRUE;

	if (prAdapter->fgIsNANfromHAL == FALSE) {
		/* send dataConfirm() to host */
		nanNdpSendDataConfirmEvent(prAdapter, prNDP);
	} else {
		/* vendor cmd path */
		nanNdpDataConfirmEvent(prAdapter, prNDP);
	}
	/* Update success unicast SCID to FW Discovery */
	nanCmdManageScid(prAdapter, TRUE, prNDP->ucPublishId, prNDP->au1Scid);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
static uint32_t
nanNdpAutoReplyDataRequest(struct ADAPTER *prAdapter,
			   struct _NAN_NDL_INSTANCE_T *prNDL,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_ATTR_NDL_T *prAttrNDL;

	uint8_t au1TestPmk[] = {
		0xf0, 0x4e, 0x41, 0x4e, 0x4e, 0x41, 0x4e, 0x4e,
		0x41, 0x4e, 0x4e, 0x41, 0x4e, 0x4e, 0x41, 0x4e,
		0x4e, 0x41, 0x4e, 0x4e, 0x41, 0x4e, 0x4e, 0x41,
		0x4e, 0x4e, 0x41, 0x4e, 0x4e, 0x41, 0x4e, 0x0f
	};

	DBGLOG(NAN, INFO, "Enter\n");

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}
	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prDataPathInfo) {
		DBGLOG(NAN, ERROR, "prDataPathInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength,
		NAN_ATTR_ID_NDP_EXTENSION);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength, NAN_ATTR_ID_NDL);

	nanUtilDump(prAdapter, "Peer NMI", prNDL->aucPeerMacAddr, MAC_ADDR_LEN);

	if (prDataPathInfo->fgAutoHandleDPRequest == TRUE) {
		prDataPathInfo->fgAutoHandleDPRequest = FALSE;

		/* Workaround for NDP termination */
		if (prAttrNDP != NULL) {
			kalMemCopy(prAdapter->rDataPathInfo.aucRemoteAddr,
				   prAttrNDP->aucInitiatorNDI, MAC_ADDR_LEN);
		} else if (prAttrNDPE != NULL) {
			kalMemCopy(prAdapter->rDataPathInfo.aucRemoteAddr,
				   prAttrNDPE->aucInitiatorNDI, MAC_ADDR_LEN);
		}

		/* always update in cases CMD Data Response asks
		 * for security policy change
		 */
		nanDataUpdateNdpLocalNDI(prAdapter, prNDP);

		/* Transaction Id */
		prNDP->u2TransId = prAdapter->rDataPathInfo.u2TransId;

		if (prNDP->fgConfirmRequired == TRUE ||
		    prNDP->fgSecurityRequired == TRUE)
			prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_CONTINUED;

		else
			prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_ACCEPTED;

		if (prNDP->fgSecurityRequired == TRUE) {
			/* Trigger NAN SEC: Responder */
			nanSecSetPmk(prNDP, NAN_PMK_INFO_LEN, au1TestPmk);
			nanSecSetCipherType(prNDP,
					NAN_CIPHER_SUITE_ID_NCS_SK_CCM_128);
			nanSecNotify4wayBegin(prNDP);
			nan_sec_wpa_sm_rx_eapol(
					prNDP->prInitiatorSecSmInfo,
					prNDP->prResponderSecSmInfo,
					prNDP->aucPeerNDIAddr);
		}

		/* If Support NAN R3 NDPE IE */
		if (nanGetFeatureNDPE(prAdapter)) {
			/* Carry Ipv6 */
			if (prDataPathInfo->fgCarryIPV6) {
				prNDP->fgCarryIPV6 = TRUE;
				kalMemCopy(prNDP->aucRspInterfaceId,
					prDataPathInfo->aucIPv6Addr,
					IPV6MACLEN);
			}

			/* Carry App info */
			if (prDataPathInfo->u2AppInfoLen > 0) {
				nanDataEngineUpdateAppInfo(prAdapter,
					prNDP,
					NAN_SERVICE_PROTOCOL_TYPE_GENERIC,
					prDataPathInfo->u2AppInfoLen,
					prDataPathInfo->pucAppInfo);

				DBGLOG(NAN, INFO, "App InfoLen = %d:\n",
					prNDP->u2AppInfoLen);
			}

			/* Carry port number */
			if (prDataPathInfo->u2PortNum > 0)
				prNDP->u2PortNum = prDataPathInfo->u2PortNum;

			/* Carry Transport protocol type:
			 * TCP(0x06) / UDP(0x11)
			 */
			if (prDataPathInfo->ucProtocolType != 0xFF)
				prNDP->ucProtocolType =
					prDataPathInfo->ucProtocolType;
		}

		if (prDataPathInfo->ucDPResponseDecisionStatus ==
		    NAN_DATA_RESP_DECISION_ACCEPT) {
			if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
			    prNDL->eCurrentNDLMgmtState ==
				    NDL_SCHEDULE_ESTABLISHED) {
				prNDL->prOperatingNDP = prNDP;
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_REQUEST_SCHEDULE_NDP,
						  prNDL);
			} else {
				/* insert into queue for later handling */
				nanDataEngineInsertRequest(
					prAdapter, prNDL,
					NAN_DATA_ENGINE_REQUEST_NDP_SETUP,
					prNDL->eNDLRole, prNDP);
			}
		} else {
			nanNdpSendDataPathResponse(
				prAdapter, NULL, prNDL->aucPeerMacAddr,
				prAttrNDP, NAN_REASON_CODE_NDP_REJECTED,
				prAttrNDPE, NAN_REASON_CODE_NDP_REJECTED,
				prAttrNDL, NAN_REASON_CODE_NDP_REJECTED);

			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
		}

		return WLAN_STATUS_SUCCESS;
	} else {
		DBGLOG(NAN, ERROR, "fail for AutoHandleDPRequest\n");
		return WLAN_STATUS_FAILURE;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Data Request
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpProcessDataRequest(struct ADAPTER *prAdapter, IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	unsigned char fgAllocNDL = FALSE;
	uint8_t ucReasonCode;
	uint8_t aucInitiatorNDI[MAC_ADDR_LEN];
	uint8_t ucNDPID;
	unsigned char fgSupportNDPE = FALSE;
	unsigned char fgSecurityRequired = FALSE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. retrieve NDP-ID from frame */
	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDL);

	if (prAttrNDP != NULL) {
		COPY_MAC_ADDR(aucInitiatorNDI, prAttrNDP->aucInitiatorNDI);
		ucNDPID = prAttrNDP->ucNDPID;
		fgSecurityRequired =
			prAttrNDP->ucNDPControl &
					NAN_ATTR_NDP_CTRL_SECURITY_PRESENT
				? TRUE
				: FALSE;
	} else if (prAttrNDPE != NULL) {
		COPY_MAC_ADDR(aucInitiatorNDI, prAttrNDPE->aucInitiatorNDI);
		ucNDPID = prAttrNDPE->ucNDPID;
		fgSecurityRequired =
			prAttrNDPE->ucNDPEControl &
					NAN_ATTR_NDPE_CTRL_SECURITY_PRESENT
				? TRUE
				: FALSE;

		fgSupportNDPE = TRUE;
	} else {
		/* invalid NAF as NAN_ACTION_DATA_PATH_REQUEST
		 * should carry NDP/NDPE attribute
		 */
		return WLAN_STATUS_FAILURE;
	}

	/* 2. create NDP on-the-fly - using specified peer-specified NDPID */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL) {
		if (prNDL->prOperatingNDP == NULL) {
			prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
							    ucNDPID);

			if (prNDP == NULL)
				prNDP = nanDataAllocateNdp(
					prAdapter, prNDL,
					NAN_PROTOCOL_RESPONDER, aucInitiatorNDI,
					ucNDPID, fgSecurityRequired);
		} else if (prNDL->prOperatingNDP->ucNDPID == ucNDPID)
			prNDP = prNDL->prOperatingNDP;
	} else {
		prNDL = nanDataAllocateNdl(prAdapter, prNaf->aucSrcAddr,
					   NAN_PROTOCOL_RESPONDER);
		if (prNDL) {
			fgAllocNDL = TRUE;
			prNDP = nanDataAllocateNdp(
				prAdapter, prNDL, NAN_PROTOCOL_RESPONDER,
				aucInitiatorNDI, ucNDPID, fgSecurityRequired);
		}
	}

	/* 2.1 Check NDP has been created or not */
	if (prNDL == NULL || prNDP == NULL) {
		if (prNDL) {
			if (nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
							ucNDPID) != NULL)
				ucReasonCode = NAN_REASON_CODE_INVALID_PARAMS;
			else
				ucReasonCode =
					NAN_REASON_CODE_RESOURCE_LIMITATION;

			if (fgAllocNDL == TRUE)
				nanDataFreeNdl(prAdapter, prNDL);
		} else
			ucReasonCode = NAN_REASON_CODE_RESOURCE_LIMITATION;

		DBGLOG(NAN, WARN, "reject by reason code [%d]\n",
		       ucReasonCode);

		/* reply with reject */
		nanNdpSendDataPathResponse(prAdapter, NULL, prNaf->aucSrcAddr,
					   prAttrNDP, ucReasonCode, prAttrNDPE,
					   ucReasonCode, prAttrNDL,
					   ucReasonCode);
	} else {

		/* 3. buffer necessary data and roll the state machine */
		if (prNDP->eCurrentNDPProtocolState == NDP_IDLE) {
			prNDP->ucTxRetryCounter = 0;

			/* Workaround: save Rx SwRfb Category pointer */
			prNDP->u2RxMsgLen =
				prSwRfb->u2PacketLen -
				OFFSET_OF(struct _NAN_ACTION_FRAME_T,
					  ucCategory);
			prNDP->pucRxMsgBuf = (uint8_t *)(&prNaf->ucCategory);

			/* update parameters through attribute parsing */
			if (nanNdpParseAttributes(
				    prAdapter, NAN_ACTION_DATA_PATH_REQUEST,
				    pucAttrList, u2AttrListLength, prNDL,
				    prNDP) == WLAN_STATUS_SUCCESS) {
				if ((nanDataEngineNDPECheck(
					     prAdapter, prNDP->fgSupportNDPE) ==
					     TRUE &&
				     prAttrNDPE == NULL) ||
				    (nanDataEngineNDPECheck(
					     prAdapter, prNDP->fgSupportNDPE) ==
					     FALSE &&
				     prAttrNDP == NULL)) {
					ucReasonCode =
						NAN_REASON_CODE_INVALID_PARAMS;
					DBGLOG(NAN, ERROR,
					       "weird condition reject by reason code [%d]\n",
					       ucReasonCode);
					/* reply with reject */
					nanNdpSendDataPathResponse(
						prAdapter, NULL,
						prNaf->aucSrcAddr, prAttrNDP,
						ucReasonCode, prAttrNDPE,
						ucReasonCode, prAttrNDL,
						ucReasonCode);
					if (fgAllocNDL == TRUE)
						nanDataFreeNdl(prAdapter,
							       prNDL);
					return WLAN_STATUS_FAILURE;
				}
				prNDP->ucRCPI = nicRxGetRcpiValueFromRxv(
					prAdapter, RCPI_MODE_WF0, prSwRfb);

				if (nanNdpAutoReplyDataRequest(prAdapter, prNDL,
							       prNDP) !=
				    WLAN_STATUS_SUCCESS)
				/* if (prNDP->fgSecurityRequired == TRUE) */
				/* TODO_CJ */
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_RESPONDER_WAIT_DATA_RSP,
						prNDP);
			} else {
				DBGLOG(NAN, WARN,
				       "reject by invalid parameters\n");

				/* in cases unexpected type/status received */
				nanNdpSendDataPathResponse(
					prAdapter, NULL, prNaf->aucSrcAddr,
					prAttrNDP,
					NAN_REASON_CODE_INVALID_PARAMS,
					prAttrNDPE,
					NAN_REASON_CODE_INVALID_PARAMS,
					prAttrNDL,
					NAN_REASON_CODE_INVALID_PARAMS);
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
			}
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Data Response
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpProcessDataResponse(struct ADAPTER *prAdapter,
			  IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	uint32_t u4RejectCode = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, INFO,
		       "Unexpected Data Response NAF from " MACSTR "\n",
		       MAC2STR(prNaf->aucSrcAddr));

		return WLAN_STATUS_FAILURE;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);

	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		/* invalid NAF as NAN_ACTION_DATA_PATH_RSP should carry
		 * NDP/NDPE attribute
		 */
		DBGLOG(NAN, ERROR, "Invalid NAF\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		DBGLOG(NAN, ERROR, "Unknown NDPID\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Special condition: DPReqTXDone comes later than DataPath Rsp NAF */
	if (prNDP->eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_REQUEST)
		nanDataPathProtocolFsmStep(prAdapter,
					   NDP_INITIATOR_RX_DP_RESPONSE, prNDP);

	if (prNDP->eCurrentNDPProtocolState == NDP_INITIATOR_RX_DP_RESPONSE) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));

		/* Workaround: save Rx SwRfb Category pointer */
		prNDP->u2RxMsgLen =
			prSwRfb->u2PacketLen -
			OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		prNDP->pucRxMsgBuf = (uint8_t *)(&prNaf->ucCategory);

		/* update parameters through attribute parsing */
		if (nanNdpParseAttributes(prAdapter,
					  NAN_ACTION_DATA_PATH_RESPONSE,
					  pucAttrList, u2AttrListLength, prNDL,
					  prNDP) == WLAN_STATUS_SUCCESS) {
			if ((prAttrNDP != NULL && prAttrNDPE != NULL) ||
			    (nanDataEngineNDPECheck(
				     prAdapter, prNDP->fgSupportNDPE) == TRUE &&
			     prAttrNDPE == NULL) ||
			    (nanDataEngineNDPECheck(prAdapter,
						    prNDP->fgSupportNDPE) ==
				     FALSE &&
			     prAttrNDP == NULL)) {
				DBGLOG(NAN, ERROR,
				       "Receive the werid Packet.\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
				prNDP->ucNDPSetupStatus =
					NAN_ATTR_NDP_STATUS_REJECTED;
				if (prNDP->fgConfirmRequired == TRUE)
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_INITIATOR_TX_DP_CONFIRM,
						prNDP);
				else
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_TX_DP_TERMINATION, prNDP);
				return WLAN_STATUS_FAILURE;
			}
			/* Query scheduler for checking availbility */
			if (prNDL->fgScheduleEstablished == FALSE)
				rStatus = nanSchedNegoChkRmtCrbProposal(
					prAdapter, &u4RejectCode);
			else
				rStatus = WLAN_STATUS_SUCCESS;
			if (rStatus == WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, INFO,
				       "nanSchedNegoChkRmtCrbProposal: accept\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_ACCEPTED;
				prNDP->ucNDPSetupStatus =
					NAN_ATTR_NDP_STATUS_ACCEPTED;
				if (prNDP->fgSecurityRequired == TRUE) {
					/* force for SEC */
					prNDP->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_CONTINUED;
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_INITIATOR_TX_DP_CONFIRM,
						prNDP);
				} else if (prNDP->fgConfirmRequired == TRUE) {
					prNDP->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_ACCEPTED;
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_INITIATOR_TX_DP_CONFIRM,
						prNDP);
				} else {
					prNDP->ucRCPI =
						nicRxGetRcpiValueFromRxv(
							prAdapter,
							RCPI_MODE_WF0, prSwRfb);
					nanDataPathProtocolFsmStep(
						prAdapter, NDP_NORMAL_TR,
						prNDP);
				}
			} else if (rStatus == WLAN_STATUS_NOT_ACCEPTED) {
				DBGLOG(NAN, INFO,
				       "nanSchedNegoChkRmtCrbProposal: counter\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_CONTINUED;
				prNDP->ucNDPSetupStatus =
					NAN_ATTR_NDP_STATUS_CONTINUED;
				if (prNDP->fgConfirmRequired == TRUE)
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_INITIATOR_TX_DP_CONFIRM,
						prNDP);
				else
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_TX_DP_TERMINATION, prNDP);
			} else {
				DBGLOG(NAN, INFO,
				       "nanSchedNegoChkRmtCrbProposal: reject\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
				prNDP->ucNDPSetupStatus =
					NAN_ATTR_NDP_STATUS_REJECTED;
				if (prNDP->fgConfirmRequired == TRUE)
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_INITIATOR_TX_DP_CONFIRM,
						prNDP);
				else
					nanDataPathProtocolFsmStep(
						prAdapter,
						NDP_TX_DP_TERMINATION, prNDP);
			}

		} else if (prNDP->fgRejectPending == TRUE) {
			if (prNDP->fgConfirmRequired == TRUE) {
				/* use Data Path Confirm to carry REJECT
				 * in NAN_ATTR_NDP
				 */
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_INITIATOR_TX_DP_CONFIRM,
					prNDP);
			} else {
				/* in cases peer think connection has been
				 * established
				 * use Data Path Termination to REJECT
				 */
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			}
		} else {
			/* disconnect directly */
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Data Confirm
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpProcessDataConfirm(struct ADAPTER *prAdapter, IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	uint32_t u4RejectCode = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR,
		       "Unexpected Data Response NAF from " MACSTR "\n",
		       MAC2STR(prNaf->aucSrcAddr));

		return WLAN_STATUS_FAILURE;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);

	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		/* invalid NAF as NAN_ACTION_DATA_PATH_CONFIRM should carry
		 * NDP/NDPE attribute
		 */
		DBGLOG(NAN, ERROR, "Invalid NAF\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		DBGLOG(NAN, ERROR, "Unknown NDPID\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Special condition: DPRespTxDone comes later than DataPath Confirm */
	if (prNDP->eCurrentNDPProtocolState == NDP_RESPONDER_TX_DP_RESPONSE &&
	    prNDP->ucNDPSetupStatus == NAN_ATTR_NDP_STATUS_CONTINUED)
		nanDataPathProtocolFsmStep(prAdapter,
					   NDP_RESPONDER_RX_DP_CONFIRM, prNDP);

	if (prNDP->eCurrentNDPProtocolState == NDP_RESPONDER_RX_DP_CONFIRM) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));

		/* Workaround: save Rx SwRfb Category pointer */
		prNDP->u2RxMsgLen =
			prSwRfb->u2PacketLen -
			OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		prNDP->pucRxMsgBuf = (uint8_t *)(&prNaf->ucCategory);

		/* update parameters through attribute parsing */
		if (nanNdpParseAttributes(prAdapter,
					  NAN_ACTION_DATA_PATH_CONFIRM,
					  pucAttrList, u2AttrListLength, prNDL,
					  prNDP) == WLAN_STATUS_SUCCESS) {

			if ((prAttrNDP != NULL && prAttrNDPE != NULL) ||
			    (nanDataEngineNDPECheck(
				     prAdapter, prNDP->fgSupportNDPE) == TRUE &&
			     prAttrNDPE == NULL) ||
			    (nanDataEngineNDPECheck(prAdapter,
						    prNDP->fgSupportNDPE) ==
				     FALSE &&
			     prAttrNDP == NULL)) {
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			}

			if (prNDL->fgScheduleEstablished == FALSE)
				rStatus = nanSchedNegoChkRmtCrbProposal(
					prAdapter, &u4RejectCode);
			else
				rStatus = WLAN_STATUS_SUCCESS;
			if (rStatus == WLAN_STATUS_SUCCESS) {
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_ACCEPTED;
				if (prNDP->fgSecurityRequired == TRUE) {
					prNDP->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_ACCEPTED;
					nanDataPathProtocolFsmStep(
					  prAdapter,
					  NDP_RESPONDER_TX_DP_SECURITY_INSTALL,
					  prNDP);
				} else {
					prNDP->ucRCPI =
						nicRxGetRcpiValueFromRxv(
							prAdapter,
							RCPI_MODE_WF0, prSwRfb);
					nanDataPathProtocolFsmStep(
						prAdapter, NDP_NORMAL_TR,
						prNDP);
				}
			} else {
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			}
		} else if (prNDP->fgRejectPending == TRUE) {
			if (prNDP->fgSecurityRequired == TRUE) {
				/* use Data Path Security Install to carry
				 * REJECT in NAN_ATTR_NDP
				 */
				nanDataPathProtocolFsmStep(
					prAdapter,
					NDP_RESPONDER_TX_DP_SECURITY_INSTALL,
					prNDP);
			} else {
				/* in cases peer think connection has been
				 * established,
				 * use Data Path Termination to REJECT
				 */
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			}
		} else {
			/* disconnect directly */
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Data Path Key Install
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpProcessDataKeyInstall(struct ADAPTER *prAdapter,
			    IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR,
		       "Unexpected Data Response NAF from " MACSTR "\n",
		       MAC2STR(prNaf->aucSrcAddr));

		return WLAN_STATUS_FAILURE;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);

	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		/* invalid NAF as NAN_ACTION_DATA_PATH_KEY_INSTALL
		 * should carry NDP/NDPE attribute
		 */
		DBGLOG(NAN, ERROR, "Invalid NAF\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		DBGLOG(NAN, ERROR, "Unknown NDPID\n");
		return WLAN_STATUS_FAILURE;
	}

	/* Special condition: DPConfirmTxDone comes later than DataPath
	 * Security Install
	 */
	if (prNDP->eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_CONFIRM &&
	    prNDP->ucNDPSetupStatus == NAN_ATTR_NDP_STATUS_CONTINUED)
		nanDataPathProtocolFsmStep(
			prAdapter, NDP_INITIATOR_RX_DP_SECURITY_INSTALL, prNDP);

	/* update parameters through attribute parsing */
	if (prNDP->eCurrentNDPProtocolState ==
	    NDP_INITIATOR_RX_DP_SECURITY_INSTALL) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));

		/* Workaround: save Rx SwRfb Category pointer */
		prNDP->u2RxMsgLen =
			prSwRfb->u2PacketLen -
			OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		prNDP->pucRxMsgBuf = (uint8_t *)(&prNaf->ucCategory);

		if (nanNdpParseAttributes(prAdapter,
					  NAN_ACTION_DATA_PATH_KEY_INSTALLMENT,
					  pucAttrList, u2AttrListLength, prNDL,
					  prNDP) == WLAN_STATUS_SUCCESS) {
			if ((prAttrNDP != NULL && prAttrNDPE != NULL) ||
			    (nanDataEngineNDPECheck(
				     prAdapter, prNDP->fgSupportNDPE) == TRUE &&
			     prAttrNDPE == NULL) ||
			    (nanDataEngineNDPECheck(prAdapter,
						    prNDP->fgSupportNDPE) ==
				     FALSE &&
			     prAttrNDP == NULL))
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			else {
				prNDP->ucRCPI = nicRxGetRcpiValueFromRxv(
					prAdapter, RCPI_MODE_WF0, prSwRfb);
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_NORMAL_TR, prNDP);
			}
		} else if (prNDP->fgRejectPending == TRUE) {
			/* in cases peer think connection has been established
			 * use Data Path Termination to REJECT
			 */
			nanDataPathProtocolFsmStep(
				prAdapter, NDP_TX_DP_TERMINATION, prNDP);
		} else {
			/* in cases reject status received */
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Data Path Termination
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpProcessDataTermination(struct ADAPTER *prAdapter,
			     IN struct SW_RFB *prSwRfb) {

	/* enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentState; */
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR,
		       "Unexpected Data Response NAF from " MACSTR "\n",
		       MAC2STR(prNaf->aucSrcAddr));

		return WLAN_STATUS_FAILURE;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);

	if (prAttrNDP != NULL) {
		DBGLOG(NAN, LOUD, "prAttrNDP->ucNDPID= %d\n",
			prAttrNDP->ucNDPID);
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	} else if (prAttrNDPE != NULL) {
		DBGLOG(NAN, LOUD, "prAttrNDPE->ucNDPID= %d\n",
			prAttrNDPE->ucNDPID);

		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	} else {
		/* invalid NAF as NAN_ACTION_DATA_PATH_TERMINATION
		 * should carry NDP/NDPE attribute
		 */
		DBGLOG(NAN, ERROR, "prAttrNDP/prAttrNDPE null!\n");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		DBGLOG(NAN, ERROR, "prNDP null!\n");
		return WLAN_STATUS_FAILURE;
	}
	DBGLOG(NAN, LOUD, "Check return success\n");
	/* update parameters through attribute parsing */
	if (nanNdpParseAttributes(prAdapter, NAN_ACTION_DATA_PATH_TERMINATION,
				  pucAttrList, u2AttrListLength, prNDL,
				  prNDP) == WLAN_STATUS_SUCCESS) {
		/* roll state machine for resource recycling */
		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Schedule Request
 *
 * \param[in]
 *
 * \return Status WLAN_STATUS_SUCCESS - accepted
 *                WLAN_STATUS_FAILURE - reject or ignore
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlProcessScheduleRequest(struct ADAPTER *prAdapter,
			     IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDL);

	if (prAttrNDL == NULL)
		return WLAN_STATUS_FAILURE;

	/* 1. create NDL on the fly */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL != NULL) {
		prNDL->eNDLRole = NAN_PROTOCOL_RESPONDER;

		nanSchedPeerPrepareNegoState(prAdapter, prNaf->aucSrcAddr);

		/* 2. parse NAN attributes for NDL */
		if (nanNdlParseAttributes(
			    prAdapter, NAN_ACTION_SCHEDULE_REQUEST, pucAttrList,
			    u2AttrListLength, prNDL) == WLAN_STATUS_SUCCESS) {
			if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
			    prNDL->eCurrentNDLMgmtState ==
				    NDL_SCHEDULE_ESTABLISHED) {
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_REQUEST_SCHEDULE_NDL,
						  prNDL);
			} else {
/* insert into queue for later handling */
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
				nanDataEngineInsertRequest(prAdapter,
						prNDL,
						NDL_REQUEST_SCHEDULE_NDL,
						prNDL->eNDLRole,
						NULL);
#endif
			}
			rStatus = WLAN_STATUS_SUCCESS;
		} else {
			nanSchedPeerCompleteNegoState(prAdapter,
						      prNaf->aucSrcAddr);

			nanNdlSendScheduleResponse(prAdapter, NULL,
						   prNaf->aucSrcAddr, prAttrNDL,
						   prNDL->ucReasonCode);
			if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
			    prNDL->eCurrentNDLMgmtState ==
				    NDL_SCHEDULE_ESTABLISHED) {
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			} else {
/* insert into queue for later handling */
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
				nanDataEngineInsertRequest(prAdapter,
						prNDL,
						NDL_TEARDOWN,
						prNDL->eNDLRole,
						NULL);
#endif
			}

			rStatus = WLAN_STATUS_FAILURE;
		}
	} else {
		/* allocate a new one */
		prNDL = nanDataAllocateNdl(prAdapter, prNaf->aucSrcAddr,
					   NAN_PROTOCOL_RESPONDER);
		if (prNDL == NULL) {
			nanNdlSendScheduleResponse(
				prAdapter, NULL, prNaf->aucSrcAddr, prAttrNDL,
				NAN_REASON_CODE_RESOURCE_LIMITATION);

			rStatus = WLAN_STATUS_FAILURE;
		} else {
			nanSchedPeerPrepareNegoState(prAdapter,
						     prNaf->aucSrcAddr);

			/* 2. parse NAN attributes for NDL */
			if (nanNdlParseAttributes(
				    prAdapter, NAN_ACTION_SCHEDULE_REQUEST,
				    pucAttrList, u2AttrListLength,
				    prNDL) == WLAN_STATUS_SUCCESS) {
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_REQUEST_SCHEDULE_NDL,
						  prNDL);

				rStatus = WLAN_STATUS_SUCCESS;
			} else {
				nanSchedPeerCompleteNegoState(
					prAdapter, prNaf->aucSrcAddr);

				nanNdlSendScheduleResponse(
					prAdapter, NULL, prNaf->aucSrcAddr,
					prAttrNDL, prNDL->ucReasonCode);

				/* free allocate NDL */
				nanDataFreeNdl(prAdapter, prNDL);

				rStatus = WLAN_STATUS_FAILURE;
			}
		}
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Schedule Response
 *
 * \param[in]
 *
 * \return Status WLAN_STATUS_SUCCESS - accepted
 *                WLAN_STATUS_FAILURE - reject or ignore
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlProcessScheduleResponse(struct ADAPTER *prAdapter,
			      IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDL);

	if (prAttrNDL == NULL)
		return WLAN_STATUS_FAILURE;

	/* 1. search for previously created NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL) {
		/* special condition - Schedule Response earlier than
		 *  ScheduleReqTxDone
		 */
		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_TX_SCHEDULE_REQUEST)
			nanNdlMgmtFsmStep(
				prAdapter,
				NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE,
				prNDL);

		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE) {
			/* 2. retrieve ATTR-NDL from payload */

			if (nanNdlParseAttributes(
				    prAdapter, NAN_ACTION_SCHEDULE_RESPONSE,
				    pucAttrList, u2AttrListLength,
				    prNDL) == WLAN_STATUS_SUCCESS) {
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_INITIATOR_RX_SCHEDULE_RESPONSE,
					prNDL);
				rStatus = WLAN_STATUS_SUCCESS;
			} else {
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			}
		} else {
			/* schedule response received in unexpected
			 * state - ignore
			 */
		}
	} else {
		/* unknown peer - ignore */
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame - Schedule Confirm
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlProcessScheduleConfirm(struct ADAPTER *prAdapter,
			     IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDL);

	if (prAttrNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_FAILURE;
	}

	/* 1. search for previously created NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL) {
		/* special condition - Schedule Confirm earlier
		 * than ScheduleRespTxDone
		 */
		if (prNDL->eCurrentNDLMgmtState ==
			    NDL_RESPONDER_TX_SCHEDULE_RESPONSE &&
		    prNDL->ucNDLSetupCurrentStatus ==
			    NAN_ATTR_NDL_STATUS_CONTINUED)
			nanNdlMgmtFsmStep(prAdapter,
					  NDL_INITIATOR_RX_SCHEDULE_RESPONSE,
					  prNDL);

		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_RESPONDER_RX_SCHEDULE_CONFIRM) {
			/* 2. retrieve ATTR-NDL from payload */
			if (nanNdlParseAttributes(
				    prAdapter, NAN_ACTION_SCHEDULE_CONFIRM,
				    pucAttrList, u2AttrListLength,
				    prNDL) == WLAN_STATUS_SUCCESS) {
				if (prNDL->ucNDLSetupCurrentStatus ==
				    NAN_ATTR_NDL_STATUS_ACCEPTED) {
					nanNdlMgmtFsmStep(
						prAdapter,
						NDL_SCHEDULE_ESTABLISHED,
						prNDL);
				} else /* REJECT */
					nanNdlMgmtFsmStep(prAdapter,
							  NDL_TEARDOWN, prNDL);

				rStatus = WLAN_STATUS_SUCCESS;
			} else {
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			}
		} else {
			/* unexpected schedule confirm NAF received - ignore */
		}
	} else {
		/* unknown peer - ignore */
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame
 *  - Schedule Update Notification
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlProcessScheduleUpdateNotification(struct ADAPTER *prAdapter,
					IN struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. search for previously created NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL) {
		DBGLOG(NAN, INFO, "ucNDLSetupCurrentStatus:%d\n",
		       prNDL->ucNDLSetupCurrentStatus);
		if (prNDL->eCurrentNDLMgmtState == NDL_SCHEDULE_ESTABLISHED) {
			/* 2. retrieve ATTR-NDL from payload */
			if (nanNdlParseAttributes(
				    prAdapter,
				    NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION,
				    pucAttrList, u2AttrListLength,
				    prNDL) == WLAN_STATUS_SUCCESS) {
				if (prNDL->ucNDLSetupCurrentStatus ==
				    NAN_ATTR_NDL_STATUS_ACCEPTED)
					nanNdlMgmtFsmStep(
						prAdapter,
						NDL_SCHEDULE_ESTABLISHED,
						prNDL);

				else /* REJECT */
					nanNdlMgmtFsmStep(prAdapter,
							  NDL_TEARDOWN, prNDL);

				rStatus = WLAN_STATUS_SUCCESS;
			} else {
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			}
		} else {
			/* TODO: unexpected schedule update notification
			 * received
			 */
		}
	} else {
		/* unknown peer ? ignore it */
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Incoming frame handlers for NAN Action Frame
 *  - Reserved
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlProcessOob(struct ADAPTER *prAdapter,
		 struct SW_RFB *prSwRfb,
		 uint8_t u1FrameType)
{
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint8_t ucRcpi, ucRssi;
	struct RX_DESC_OPS_T *prRxDescOps;
	struct RF_CHANNEL_INFO rVendorChInfo;
	uint8_t ucHwBand, ucHwChannelNum;
	struct HW_MAC_RX_STS_GROUP_3_V2 *prGroup3;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "prSwRfb error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prSwRfb->u2PacketLen < prSwRfb->u2HeaderLen) {
		DBGLOG(NAN, ERROR, "Invalid packet length\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "prNaf error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* RSSI */
	ucRcpi = nicRxGetRcpiValueFromRxv(prAdapter, RCPI_MODE_MAX, prSwRfb);
	ucRssi = RCPI_TO_dBm(ucRcpi);

	/* Channel */
	prRxDescOps = prAdapter->chip_info->prRxDescOps;
	RX_STATUS_GET(prRxDescOps, ucHwBand, get_rf_band, prSwRfb->prRxStatus);
	RX_STATUS_GET(prRxDescOps, ucHwChannelNum, get_ch_num,
			prSwRfb->prRxStatus);
#if (CFG_SUPPORT_WIFI_6G == 1)
	nicRxdChNumTranslate((enum ENUM_BAND) ucHwBand, &ucHwChannelNum);
#endif

	prGroup3 = (struct HW_MAC_RX_STS_GROUP_3_V2 *)
			prSwRfb->prRxStatusGroup3;
	rVendorChInfo.ucChannelNum = ucHwChannelNum;
	rVendorChInfo.eBand = (enum ENUM_BAND) ucHwBand;
	rVendorChInfo.ucChnlBw = CONNAC2X_HAL_RX_VECTOR_GET_BW(prGroup3);

	rStatus = mtk_cfg80211_vendor_event_nan_oob_af_rx(
		prAdapter,
		prNaf->aucDestAddr,
		prNaf->aucSrcAddr,
		prNaf->aucClusterID,
		&rVendorChInfo,
		ucRssi,
		(prSwRfb->u2PacketLen - prSwRfb->u2HeaderLen),
		&prNaf->ucCategory,
		u1FrameType
		);

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
static enum _ENUM_NAN_NDP_STATUS_T
nanDataPathProtocolFsmStep(IN struct ADAPTER *prAdapter,
			   IN enum _ENUM_NDP_PROTOCOL_STATE_T eNextState,
			   IN struct _NAN_NDP_INSTANCE_T *prNDP) {
	IN struct _NAN_NDL_INSTANCE_T *prNDL;
	enum _ENUM_NDP_PROTOCOL_STATE_T eLastState;
	enum _ENUM_NAN_NDP_STATUS_T eNdpConnectionStatus;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return NAN_NDP_DISCONNECT;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return NAN_NDP_DISCONNECT;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return NAN_NDP_DISCONNECT;
	}

	eNdpConnectionStatus = NAN_NDP_DISCONNECT;

	do {

		/* TODO: Define your own dbg level */
		DBGLOG(NAN, STATE, "NDP mgmt STATE_%d: [%s] -> [%s]\n",
		       prNDP->ucNDPID,
		       apucDebugDataPathProtocolState
			       [prNDP->eCurrentNDPProtocolState],
		       apucDebugDataPathProtocolState[eNextState]);

		prNDP->eLastNDPProtocolState = prNDP->eCurrentNDPProtocolState;
		prNDP->eCurrentNDPProtocolState = eNextState;
		eLastState = eNextState;

		switch (eNextState) {
		case NDP_IDLE:
			/* stable state */
			break;

		case NDP_INITIATOR_TX_DP_REQUEST:
			prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_CONTINUED;
			nanNdpUpdateTypeStatus(prAdapter, prNDP);

			/* generate new dialog token */
			if (prNDL->fgScheduleEstablished == FALSE)
				nanNdlGenerateDialogToken(prAdapter, prNDL);
			nanNdpGenerateDialogToken(prAdapter, prNDP);

			/* send Data Path Request NAF */
			nanNdpSendDataPathRequest(prAdapter, prNDP);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);

			break;

		case NDP_INITIATOR_RX_DP_RESPONSE:
			/* no need to do special thing
			 * but reset timer and wait for incoming
			 * Data Path Response
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_PROTOCOL_TIMEOUT);
			break;

		case NDP_INITIATOR_RX_DP_SECURITY_INSTALL:
			/* no need to do special thing
			 * but reset timer and wait for incoming Data Path
			 * Security Install
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_PROTOCOL_TIMEOUT);
			break;

		case NDP_INITIATOR_TX_DP_CONFIRM:
			nanNdpUpdateTypeStatus(prAdapter, prNDP);
			nanNdpSendDataPathConfirm(prAdapter, prNDP);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);

			break;

		case NDP_RESPONDER_WAIT_DATA_RSP:
			/* send data indication to host layer */
			if (prAdapter->fgIsNANfromHAL == FALSE) {
				/* 1. IOCtrl */
				nanNdpSendDataIndicationEvent(prAdapter, prNDP);
			} else {
				/* 2. vendor cmd */
				nanNdpDataIndEvent(prAdapter, prNDP, prNDL);
			}
			cnmTimerStopTimer(prAdapter,
					  &(prNDP->rNDPUserSpaceResponseTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDP->rNDPUserSpaceResponseTimer),
					   NAN_PROTOCOL_TIMEOUT);

			break;

		case NDP_RESPONDER_TX_DP_RESPONSE:
			nanNdpUpdateTypeStatus(prAdapter, prNDP);

			nanNdpSendDataPathResponse(
				prAdapter, prNDP, NULL, NULL,
				NAN_REASON_CODE_RESERVED, NULL,
				NAN_REASON_CODE_RESERVED, NULL,
				NAN_REASON_CODE_RESERVED);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);

			break;

		case NDP_RESPONDER_RX_DP_CONFIRM:
			/* no need to do special thing
			 * but reset timer and wait for incoming
			 * Data Path Confirm
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_PROTOCOL_TIMEOUT);
			break;

		case NDP_RESPONDER_TX_DP_SECURITY_INSTALL:
			nanNdpUpdateTypeStatus(prAdapter, prNDP);

			nanNdpSendDataPathKeyInstall(prAdapter, prNDP);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);

			break;

		case NDP_NORMAL_TR:
			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter, prNDL);

			/* Allocate STA-REC */
			if (nanDataEngineEnrollNDPContext(prAdapter, prNDL,
							  prNDP) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, ERROR,
				       "NDP_NORMAL_TR: STA-REC allocation failure\n");
				prNDP->fgNDPEstablish = FALSE;
				eNextState = NDP_TX_DP_TERMINATION;
			} else {
				/* send data confirm indication to host layer */
				prNDP->fgNDPEstablish = TRUE;
				nanDataPathSetupSuccess(prAdapter, prNDP);

				/* roll NDL state machine to schedule
				 * estabilshed
				 */
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			}

			break;

		case NDP_TX_DP_TERMINATION:
			nanNdpUpdateTypeStatus(prAdapter, prNDP);
			nanNdpSendDataPathTermination(prAdapter, prNDP);

			break;

		case NDP_DISCONNECT:
			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter, prNDL);

			/* send data termination indication to host layer */
			if (prAdapter->fgIsNANfromHAL == FALSE) {
				/* 1. IOCtrl */
				nanNdpSendDataTerminationEvent(prAdapter,
							       prNDP);
			} else {
				/* 2. vendor cmd path */
				nanNdpDataTerminationEvent(prAdapter, prNDP);
			}
			if (prNDL->prOperatingNDP == prNDP) {
				/* handsahking failed, clear operating
				 * NDP pointer
				 */
				prNDL->prOperatingNDP = NULL;
			}

			/* terminate NAN SEC sm */
			if (prNDP->fgNDPValid && prNDP->fgSecurityRequired)
				nanSecNotify4wayTerminate(prNDP);

			/* free NDP */
			nanDataFreeNdp(prAdapter, prNDP);

			/* roll NDL state machine to schedule teardown
			 * if needed
			 */
			nanNdlMgmtFsmStep(prAdapter,
					  NDL_TEARDOWN_BY_NDP_TERMINATION,
					  prNDL);

			break;

		default:
			break;
		}

	} while (eLastState != eNextState);

	return eNdpConnectionStatus;
}

static enum _ENUM_NDL_MGMT_STATE_T
nanNdlMgmtFsmNdlScheduleSetup(IN struct ADAPTER *prAdapter,
			      IN struct _NAN_NDL_INSTANCE_T *prNDL) {
	enum _ENUM_NDL_MGMT_STATE_T eNextState = NDL_SCHEDULE_SETUP;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4RejectCode = 0;

	do {
		if (prNDL->prOperatingNDP) {
			if (prNDL->prOperatingNDP->eNDPRole ==
				    NAN_PROTOCOL_INITIATOR &&
			    prNDL->fgScheduleEstablished == FALSE) {
				/* Query scheduler for gen availiblity */
				rStatus = nanSchedNegoGenLocalCrbProposal(
					prAdapter);
				if (rStatus != WLAN_STATUS_SUCCESS) {
					DBGLOG(NAN, WARN,
						"Reject by scheduler,status:0x%x\n",
						rStatus);
					eNextState = NDL_TEARDOWN;
					break;
				}
				/* send Data Path Request */
				nanDataPathProtocolFsmStep(
					prAdapter,
					NDP_INITIATOR_TX_DP_REQUEST,
					prNDL->prOperatingNDP);
			} else if (prNDL->prOperatingNDP->eNDPRole ==
					   NAN_PROTOCOL_INITIATOR &&
				   prNDL->fgScheduleEstablished == TRUE) {
				/* send Data Path Request */
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_INITIATOR_TX_DP_REQUEST,
					prNDL->prOperatingNDP);
			} else if (prNDL->prOperatingNDP->eNDPRole ==
					   NAN_PROTOCOL_RESPONDER &&
				   prNDL->fgScheduleEstablished == FALSE) {
				/* Query scheduler for checking availbility */
				rStatus = nanSchedNegoChkRmtCrbProposal(
					prAdapter, &u4RejectCode);
				if (rStatus == WLAN_STATUS_SUCCESS) {
					DBGLOG(NAN, INFO,
					       "NegoChkRmtCrbProposal:accept\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_ACCEPTED;
					if ((prNDL->prOperatingNDP
							->fgConfirmRequired ==
						TRUE) ||
						(prNDL->prOperatingNDP
							->fgSecurityRequired ==
					     TRUE)) {
						/* force for SEC */
						prNDL->prOperatingNDP
						    ->ucNDPSetupStatus =
						  NAN_ATTR_NDP_STATUS_CONTINUED;
					} else
					    prNDL->prOperatingNDP
						->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_ACCEPTED;
				} else if (rStatus ==
					   WLAN_STATUS_NOT_ACCEPTED) {
					DBGLOG(NAN, INFO,
					       "NegoChkRmtCrbProposal:counter\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_CONTINUED;
					prNDL->prOperatingNDP
						->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_CONTINUED;
				} else {
					DBGLOG(NAN, INFO,
					       "NegoChkRmtCrbProposal:reject\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_REJECTED;
					prNDL->prOperatingNDP
						->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_REJECTED;
				}
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_RESPONDER_TX_DP_RESPONSE,
					prNDL->prOperatingNDP);
			} else {
				/* nanCmdDataResponse() setup initial
				 * status code
				 */
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_RESPONDER_TX_DP_RESPONSE,
					prNDL->prOperatingNDP);
			}
		} else {
			/* strange condition occurred - error handling */
			DBGLOG(NAN, ERROR, "unexpected - no prOperatingNDP\n");

			/* return schedule negotiation permit */
			prNDL->fgScheduleUnderNegotiation = FALSE;
			nanSchedNegoStop(prAdapter);

			if (prNDL->fgScheduleEstablished == TRUE)
				eNextState = NDL_SCHEDULE_ESTABLISHED;

			else
				eNextState = NDL_IDLE;
		}
	} while (FALSE);

	return eNextState;
}
static enum _ENUM_NDL_MGMT_STATE_T
nanNdlGetNextRequire(IN struct ADAPTER *prAdapter,
		     IN struct _NAN_NDL_INSTANCE_T *prNDL,
		     IN enum _ENUM_NDL_MGMT_STATE_T eCurrentState) {
	enum _ENUM_NDL_MGMT_STATE_T eNextState;
	struct _NAN_DATA_ENGINE_REQUEST_T *prPendingReq;

	prPendingReq = nanDataEngineGetNextRequest(prAdapter, prNDL);

	eNextState = eCurrentState;
	if (prPendingReq == NULL) {
		/* stable state */
		return eNextState;

	} else if (prPendingReq->eRequestType ==
		   NAN_DATA_ENGINE_REQUEST_NDL_SETUP) {
		prNDL->eNDLRole = prPendingReq->eNDLRole;
		eNextState = NDL_REQUEST_SCHEDULE_NDL;

	} else if (prPendingReq->eRequestType ==
		   NAN_DATA_ENGINE_REQUEST_NDP_SETUP) {
		prNDL->eNDLRole = prPendingReq->eNDLRole;
		prNDL->prOperatingNDP = prPendingReq->prNDP;
		eNextState = NDL_REQUEST_SCHEDULE_NDP;

	} else
		DBGLOG(NAN, ERROR, "unknown pending request type (%d)\n",
		       (uint8_t)(prPendingReq->eRequestType));

	if (prPendingReq)
		cnmMemFree(prAdapter, prPendingReq);
	return eNextState;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
static void
nanNdlMgmtFsmStep(IN struct ADAPTER *prAdapter,
		  IN enum _ENUM_NDL_MGMT_STATE_T eNextState,
		  IN struct _NAN_NDL_INSTANCE_T *prNDL) {

	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4RejectCode = 0;
	enum _ENUM_NDL_MGMT_STATE_T eLastState;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return;
	}

	do {
		DBGLOG(NAN, STATE, "NDL mgmt STATE_%d: [%s] -> [%s]\n",
		       prNDL->ucIndex /* prNDL->ucPeerID */,
		       apucDebugDataMgmtState[prNDL->eCurrentNDLMgmtState],
		       apucDebugDataMgmtState[eNextState]);

		/* state tracking */
		prNDL->eLastNDLMgmtState = prNDL->eCurrentNDLMgmtState;
		prNDL->eCurrentNDLMgmtState = eNextState;

		eLastState = eNextState;

		switch (eNextState) {
		case NDL_IDLE:
			/* stable state */
			eNextState = nanNdlGetNextRequire(prAdapter, prNDL,
							  eNextState);
			break;

		case NDL_REQUEST_SCHEDULE_NDP:
			/* Call NAN-Scheduler API and wait for callback
			 * to be invoked
			 */
			if (prNDL->fgScheduleEstablished == TRUE) {
				eNextState = NDL_SCHEDULE_SETUP;
			} else {
				nanSchedNegoStart(
					prAdapter, prNDL->aucPeerMacAddr,
					ENUM_NAN_NEGO_DATA_LINK,
					prNDL->eNDLRole ==
							NAN_PROTOCOL_INITIATOR
						? ENUM_NAN_NEGO_ROLE_INITIATOR
						: ENUM_NAN_NEGO_ROLE_RESPONDER,
					nanDataPathScheduleNegoGranted,
					(void *) &(prNDL->rToken));
			}
			break;

		case NDL_REQUEST_SCHEDULE_NDL:
			/* Call NAN-Scheduler API and wait for callback
			 * to be invoked
			 */
			nanSchedNegoStart(
				prAdapter, prNDL->aucPeerMacAddr,
				ENUM_NAN_NEGO_DATA_LINK,
				prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR
					? ENUM_NAN_NEGO_ROLE_INITIATOR
					: ENUM_NAN_NEGO_ROLE_RESPONDER,
				nanDataPathScheduleNegoGranted,
				(void *) &(prNDL->rToken));
			nanSchedPeerCompleteNegoState(prAdapter,
						      prNDL->aucPeerMacAddr);
			break;

		case NDL_SCHEDULE_SETUP:
			eNextState =
				nanNdlMgmtFsmNdlScheduleSetup(prAdapter, prNDL);
			break;

		case NDL_INITIATOR_TX_SCHEDULE_REQUEST:
			prNDL->ucNDLSetupCurrentStatus =
				NAN_ATTR_NDL_STATUS_CONTINUED;

			nanNdlGenerateDialogToken(prAdapter, prNDL);

			nanNdlSendScheduleRequest(prAdapter, prNDL);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);
			break;

		case NDL_RESPONDER_RX_SCHEDULE_REQUEST:
			prNDL->ucTxRetryCounter = 0;

			/* Query scheduler for checking availbility */
			rStatus = nanSchedNegoChkRmtCrbProposal(prAdapter,
								&u4RejectCode);
			if (rStatus == WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:accept\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_ACCEPTED;
			} else if (rStatus == WLAN_STATUS_NOT_ACCEPTED) {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:counter\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_CONTINUED;
			} else {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:reject\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
			}
			eNextState = NDL_RESPONDER_TX_SCHEDULE_RESPONSE;

			break;

		case NDL_RESPONDER_TX_SCHEDULE_RESPONSE:
			nanNdlSendScheduleResponse(prAdapter, prNDL, NULL, NULL,
						   NAN_REASON_CODE_RESERVED);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);
			break;
		case NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE:
			/* but reset timer and wait for incoming
			 * Schedule Request
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_PROTOCOL_TIMEOUT);
			break;
		case NDL_INITIATOR_RX_SCHEDULE_RESPONSE:

			/* Query scheduler for checking availbility */
			rStatus = nanSchedNegoChkRmtCrbProposal(prAdapter,
								&u4RejectCode);
			if (rStatus == WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:accept\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_ACCEPTED;
				eNextState = NDL_SCHEDULE_ESTABLISHED;
			} else if (rStatus == WLAN_STATUS_NOT_ACCEPTED) {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:counter\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_CONTINUED;
				eNextState = NDL_INITIATOR_TX_SCHEDULE_CONFIRM;
			} else {
				DBGLOG(NAN, INFO,
				       "NegoChkRmtCrbProposal:reject NDL Teardown\n");
				eNextState = NDL_TEARDOWN;
			}
			break;

		case NDL_INITIATOR_TX_SCHEDULE_CONFIRM:
			nanNdlSendScheduleConfirm(prAdapter, prNDL);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolRetryTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolRetryTimer),
					   NAN_DATA_RETRY_TIMEOUT);
			break;

		case NDL_RESPONDER_RX_SCHEDULE_CONFIRM:
			/* no need to do special thing
			 * but reset timer and wait for incoming
			 *  Schedule Request
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_PROTOCOL_TIMEOUT);
			break;

		case NDL_SCHEDULE_ESTABLISHED:
			prNDL->fgScheduleEstablished = TRUE;

			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter, prNDL);

			if (prNDL->fgScheduleUnderNegotiation == TRUE) {
				/* return schedule negotiation permit */
				prNDL->fgScheduleUnderNegotiation = FALSE;
				nanSchedNegoStop(prAdapter);
			}

			if (prNDL->prOperatingNDP != NULL)
				prNDL->prOperatingNDP = NULL;
			eNextState = nanNdlGetNextRequire(prAdapter, prNDL,
							  eNextState);

			break;

		case NDL_TEARDOWN_BY_NDP_TERMINATION:
		case NDL_TEARDOWN:

			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter, prNDL);

			if (prNDL->fgScheduleUnderNegotiation == TRUE) {
				/* return schedule negotiation permit */
				prNDL->fgScheduleUnderNegotiation = FALSE;
				nanSchedNegoStop(prAdapter);
			}

			if (prNDL->ucNDPNum == 0 ||
			    prNDL->eCurrentNDLMgmtState == NDL_TEARDOWN) {
				prNDL->fgScheduleEstablished = FALSE;

				nanDataFreeNdl(prAdapter, prNDL);

				/* return resource to NAN scheduler */
				nanSchedDropResources(prAdapter,
						      prNDL->aucPeerMacAddr,
						      ENUM_NAN_NEGO_DATA_LINK);

				eNextState = NDL_IDLE;
			} else {
				/* still having NDP,
				 * fall back to Schedule Established
				 */
				eNextState = NDL_SCHEDULE_ESTABLISHED;
			}

			break;

		default:
			break;
		}
	} while (eLastState != eNextState);
}

void nanSetNdpPmkid(
	IN struct ADAPTER *prAdapter,
	IN struct _NAN_CMD_DATA_REQUEST *prNanCmdDataRequest,
	IN uint8_t *puServiceName
){
	u8 pmkid[32];
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	struct BSS_INFO *prBssInfo;

	/* Get BSS info */
	prNanSpecificBssInfo = nanGetSpecificBssInfo(
		prAdapter,
		NAN_BSS_INDEX_MAIN);
	if (prNanSpecificBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prNanSpecificBssInfo is null\n");
		return;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(
		prAdapter,
		prNanSpecificBssInfo->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prBssInfo is null\n");
		return;
	}

	calculate_pmkid(
		prNanCmdDataRequest->aucPMK,
		prBssInfo->aucOwnMacAddr,
		prNanCmdDataRequest->aucResponderDataAddress,
		puServiceName, NULL, pmkid);
	nanUtilDump(prAdapter, "[publish] SCID=>",
						pmkid, NAN_SCID_DEFAULT_LEN);
	kalMemCopy(prNanCmdDataRequest->aucScid, pmkid, 16);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanCmdDataRequest(IN struct ADAPTER *prAdapter,
		  IN struct _NAN_CMD_DATA_REQUEST *prNanCmdDataRequest,
		  OUT uint8_t *pu1NdpId, OUT uint8_t *au1InitiatorDataAddr) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	unsigned char fgAllocNDL = FALSE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanCmdDataRequest) {
		DBGLOG(NAN, ERROR,
			"prNanCmdDataRequest error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Workaround for NDP termination */
	kalMemCopy(prAdapter->rDataPathInfo.aucRemoteAddr,
		   prNanCmdDataRequest->aucResponderDataAddress, MAC_ADDR_LEN);

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO,
		"ucPublishID:%d, ucRequireQOS:%d, ucSecurity%d, u2SpecificInfoLength:%d\n",
		prNanCmdDataRequest->ucPublishID,
		prNanCmdDataRequest->ucRequireQOS,
		prNanCmdDataRequest->ucSecurity,
		prNanCmdDataRequest->u2SpecificInfoLength);
	dumpMemory8(prNanCmdDataRequest->aucResponderDataAddress, MAC_ADDR_LEN);
#endif

	/* check for existing NDL where peer address exists or not */
	prNDL = nanDataUtilSearchNdlByMac(
		prAdapter, prNanCmdDataRequest->aucResponderDataAddress);
	if (prNDL) {
		/* had GAS further service discovery */
		prNDP = nanDataAllocateNdp(
			prAdapter, prNDL, NAN_PROTOCOL_INITIATOR,
			prNanCmdDataRequest->aucResponderDataAddress, 0,
			prNanCmdDataRequest->ucSecurity ==
					NAN_CIPHER_SUITE_ID_NONE
				? FALSE
				: TRUE);
	} else {
		prNDL = nanDataAllocateNdl(
			prAdapter, prNanCmdDataRequest->aucResponderDataAddress,
			NAN_PROTOCOL_INITIATOR);
		if (prNDL) {
			fgAllocNDL = TRUE;
			prNDP = nanDataAllocateNdp(
				prAdapter, prNDL, NAN_PROTOCOL_INITIATOR,
				prNanCmdDataRequest->aucResponderDataAddress, 0,
				prNanCmdDataRequest->ucSecurity ==
						NAN_CIPHER_SUITE_ID_NONE
					? FALSE
					: TRUE);
		}
	}

	if (prNDL == NULL || prNDP == NULL) {
		/* no available entry can be allocated */
		if (prNDL && fgAllocNDL == TRUE)
			nanDataFreeNdl(prAdapter, prNDL);

		/* @TODO: return a unsuccessful indication to host side
		 * cnmPktFree((P_PKT_INFO_T)prMsduInfo, TRUE);
		 * free OID?
		 */
		return WLAN_STATUS_RESOURCES;
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
	if (IS_NAN_CAPABILITY_ENABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA) &&
		IS_BMCAST_MAC_ADDR(
			prNanCmdDataRequest->aucMulticastAddress)) {
		kalMemCpyS(prNDP->aucNanMulticastAddr,
			NAN_MAC_ADDR_LEN,
			prNanCmdDataRequest->aucMulticastAddress,
			NAN_MAC_ADDR_LEN);
	} else {
		kalMemZero(prNDP->aucNanMulticastAddr, NAN_MAC_ADDR_LEN);
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */

	/* fill NDP parameters from initiator request */
	prNDP->ucPublishId = prNanCmdDataRequest->ucPublishID;

	/* Fill transaction Id assign by framework */
	prNDP->u2TransId = prNanCmdDataRequest->u2NdpTransactionId;

	if (prNanCmdDataRequest->ucRequireQOS & NAN_DATAREQ_REQUIRE_QOS_UNICAST)
		prNDP->fgQoSRequired = TRUE;

	else
		prNDP->fgQoSRequired = FALSE;

	if (prNDL->ucNDPNum > 1) {
		prNDP->fgQoSRequired = FALSE;
		DBGLOG(NAN, INFO,
		       "Use the same NDL (Qos no need to update)\n");
	}

	if (prNanCmdDataRequest->ucSecurity) {
		prNDP->ucCipherType = prNanCmdDataRequest->ucSecurity;
		kalMemCopy(prNDP->au1Scid, prNanCmdDataRequest->aucScid,
			   NAN_SCID_DEFAULT_LEN);
		kalMemCopy(prNDP->aucPMK, prNanCmdDataRequest->aucPMK,
			   NAN_PMK_INFO_LEN);

		/* Trigger NAN SEC */
		nanSecApSmBufReset(prNDP->prInitiatorSecSmInfo);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
		/* Reset wpa_sm for M4 group key process */
		nanSecStaSmBufReset(prNDP->prResponderSecSmInfo);
#endif
		nanSecSetPmk(prNDP, NAN_PMK_INFO_LEN,
			     prNanCmdDataRequest->aucPMK);
		nanSecSetCipherType(prNDP, prNanCmdDataRequest->ucSecurity);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
		if (nanSecIsDevSupportGroupSecurity(prAdapter)) {
			prNDP->fgTxGtkRequired =
				(prNanCmdDataRequest->ucGtkCipher) ?
				TRUE : FALSE;
			prNDP->ucTxGtkCipherType =
				prNanCmdDataRequest->ucGtkCipher;
			prNDP->ucTxSecurityCap =
				nanSecGetGroupSecurityCap(prAdapter, prNDP);

			nan_sec_ndi_wpa_auth_init(NAN_NDI_INDEX_0,
				prNDP->ucTxGtkCipherType);

			nanSecSetGroupCipherType(prNDP, TRUE);
		} else
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */
		{
			nan_sec_ndi_wpa_auth_init(NAN_NDI_INDEX_0,
				0);
		}
		nanSecNotify4wayBegin(prNDP);
	}

	/* R3 capability */
		/* AppInfo */
		if (prNanCmdDataRequest->u2SpecificInfoLength > 0)
			nanDataEngineUpdateAppInfo(prAdapter,
					prNDP,
					NAN_SERVICE_PROTOCOL_TYPE_GENERIC,
					prNanCmdDataRequest->
					u2SpecificInfoLength,
					prNanCmdDataRequest->aucSpecificInfo);
		else
			prNDP->u2AppInfoLen = 0;

	/* Ipv6 */
	prNDP->fgCarryIPV6 = prNanCmdDataRequest->fgCarryIpv6;
	if (prNDP->fgCarryIPV6 == TRUE) {
		prNDP->fgIsInitiator = TRUE;
		kalMemCopy(prNDP->aucInterfaceId,
			prNanCmdDataRequest->aucIPv6Addr,
			IPV6MACLEN);
	}

	/* transport protocol and port num */
	if (nanGetFeatureIsSigma(prAdapter) && g_ndpReqNDPE.fgEnNDPE) {
		prNDP->u2PortNum = 9000;
		prNDP->ucProtocolType = IP_PRO_TCP;
	}

	if (prNanCmdDataRequest->ucRequireQOS == TRUE) {
		prNDL->ucMinimumTimeSlot = prNanCmdDataRequest->ucMinTimeSlot;
		prNDL->u2MaximumLatency = prNanCmdDataRequest->u2MaxLatency;
	}
	DBGLOG(NAN, INFO, "Update App info len = %d and QoS:%d\n",
		prNDP->u2AppInfoLen,
		prNanCmdDataRequest->ucRequireQOS);

	/* roll NDL state machine for permission request */
	if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
	    prNDL->eCurrentNDLMgmtState == NDL_SCHEDULE_ESTABLISHED) {
		prNDL->prOperatingNDP = prNDP;
		nanNdlMgmtFsmStep(prAdapter, NDL_REQUEST_SCHEDULE_NDP, prNDL);
	} else {
/* insert into queue for later handling */
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
		nanDataEngineInsertRequest(prAdapter,
				prNDL,
				NAN_DATA_ENGINE_REQUEST_NDP_SETUP,
				prNDL->eNDLRole,
				prNDP);
#endif
	}

	/* Fill-in return values */
	*pu1NdpId = prNDP->ucNDPID;
	kalMemCopy(au1InitiatorDataAddr, prNDP->aucLocalNDIAddr, MAC_ADDR_LEN);

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Done\n");
#endif

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanCmdDataResponse(struct ADAPTER *prAdapter,
		   struct _NAN_CMD_DATA_RESPONSE *prNanCmdDataResponse) {
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	const uint8_t aucNULLAddr[] = NULL_MAC_ADDR;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	struct _GL_NAN_INFO_T *prNANInfo = (struct _GL_NAN_INFO_T *)NULL;
#endif

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}
	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (!prNanCmdDataResponse) {
		DBGLOG(NAN, ERROR,
			"prNanCmdDataResponse error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO,
	       "ucNDPId:%d, aucInitiatorDataAddress=>%02x:%02x:%02x:%02x:%02x:%02x\n",
	       prNanCmdDataResponse->ucNDPId,
	       prNanCmdDataResponse->aucInitiatorDataAddress[0],
	       prNanCmdDataResponse->aucInitiatorDataAddress[1],
	       prNanCmdDataResponse->aucInitiatorDataAddress[2],
	       prNanCmdDataResponse->aucInitiatorDataAddress[3],
	       prNanCmdDataResponse->aucInitiatorDataAddress[4],
	       prNanCmdDataResponse->aucInitiatorDataAddress[5]);
	DBGLOG(NAN, INFO,
	       "ucRequireQOS:%d, ucSecurity:%d(cipher), u2SpecificInfoLength:%d, ucDecisionStatus:%d\n",
	       prNanCmdDataResponse->ucRequireQOS,
	       prNanCmdDataResponse->ucSecurity,
	       prNanCmdDataResponse->u2SpecificInfoLength,
	       prNanCmdDataResponse->ucDecisionStatus);
#endif

	if (UNEQUAL_MAC_ADDR(prNanCmdDataResponse->aucInitiatorDataAddress,
			     aucBCAddr) &&
	    UNEQUAL_MAC_ADDR(prNanCmdDataResponse->aucInitiatorDataAddress,
			     aucNULLAddr)) {
		prNDL = nanDataUtilSearchNdlByMac(
			prAdapter,
			prNanCmdDataResponse->aucInitiatorDataAddress);
		if (prNDL != NULL)
			prNDP = nanDataUtilSearchNdpByNdpId(
				prAdapter, prNDL,
				prNanCmdDataResponse->ucNDPId);
	} else {
		prNDP = nanDataUtilSearchNdpByNdpIdOnly(
			prAdapter, prNanCmdDataResponse->ucNDPId);
		if (prNDP != NULL)
			prNDL = nanDataUtilGetNdl(prAdapter, prNDP);
	}

	if (prNDL == NULL || prNDP == NULL) {
		if (prNanCmdDataResponse->ucNDPId == 0) {
			prDataPathInfo->fgAutoHandleDPRequest = TRUE;
			prDataPathInfo->ucDPResponseDecisionStatus =
				prNanCmdDataResponse->ucDecisionStatus;
			prDataPathInfo->u2TransId =
				prNanCmdDataResponse->u2NdpTransactionId;

			/* For NAN R3, Support NDPE,
			 * should define in wifi.cfg
			 */
			if (nanGetFeatureNDPE(prAdapter)) {
				/* Add Ipv6 in App info */
				if (prNanCmdDataResponse->fgCarryIpv6 == 1) {
					prDataPathInfo->fgCarryIPV6 = TRUE;
					kalMemCopy(prDataPathInfo->aucIPv6Addr,
						prNanCmdDataResponse->
						aucIPv6Addr,
						IPV6MACLEN);
				}

				/* Add App info */
				if (prNanCmdDataResponse->
					u2SpecificInfoLength > 0) {
					DBGLOG(NAN, INFO,
						"Enter parse AppInfo");
					prDataPathInfo->u2AppInfoLen =
						prNanCmdDataResponse->
						u2SpecificInfoLength;
					prDataPathInfo->pucAppInfo =
						cnmMemAlloc(prAdapter,
						RAM_TYPE_BUF,
						(uint32_t)prDataPathInfo->
						u2AppInfoLen);
					if (!prDataPathInfo->pucAppInfo) {
						DBGLOG(NAN, ERROR,
						"AppInfo is null\n");
						return WLAN_STATUS_FAILURE;
					}
					kalMemCopy(prDataPathInfo->pucAppInfo,
						prNanCmdDataResponse->
						aucSpecificInfo,
						prDataPathInfo->u2AppInfoLen);
				}
				if (nanGetFeatureIsSigma(prAdapter)) {
				prDataPathInfo->u2PortNum =
					prNanCmdDataResponse->u2PortNum;
				prDataPathInfo->ucProtocolType =
					prNanCmdDataResponse->
					ucServiceProtocolType;
				}

				DBGLOG(NAN, INFO,
					"AppInfoLen %d\n",
					prDataPathInfo->u2AppInfoLen);
			}
			return WLAN_STATUS_SUCCESS;
		}

		/* no matching NDL/NDP - it should have been created when NAF
		 * - Data Path Request is received
		 */

		/* @TODO: return a unsuccessful indication to host */
		DBGLOG(NAN, ERROR, "no matching NDL/NDP\n");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
	if (IS_NAN_CAPABILITY_ENABLED(prAdapter,
		WLAN_CAPABILITIES_NAN_MC_DATA) &&
		IS_BMCAST_MAC_ADDR(
			prNanCmdDataResponse->aucMulticastAddress)) {
		kalMemCpyS(prNDP->aucNanMulticastAddr,
			NAN_MAC_ADDR_LEN,
			prNanCmdDataResponse->aucMulticastAddress,
			NAN_MAC_ADDR_LEN);
	} else {
		kalMemZero(prNDP->aucNanMulticastAddr, NAN_MAC_ADDR_LEN);
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA */
	/* Transaction Id */
	prNDP->u2TransId =  prNanCmdDataResponse->u2NdpTransactionId;

	if (prNanCmdDataResponse->ucRequireQOS == TRUE) {
		prNDL->ucMinimumTimeSlot = prNanCmdDataResponse->ucMinTimeSlot;
		prNDL->u2MaximumLatency = prNanCmdDataResponse->u2MaxLatency;
	}

	/* stop timer first */
	cnmTimerStopTimer(prAdapter, &(prNDP->rNDPUserSpaceResponseTimer));

	/* If data response doesn't compliant data request's
	 * security requirement
	 */
	if ((prNDP->fgSecurityRequired &&
	     (prNanCmdDataResponse->ucSecurity != prNDP->ucCipherType)) ||
	    (!prNDP->fgSecurityRequired && prNanCmdDataResponse->ucSecurity)) {
		DBGLOG(NAN, ERROR, "Reject due to security mismatch!");
		DBGLOG(NAN, ERROR,
			"gSecurityRequired[%u]ucSecurity[%u]ucCipherType[%u]\n",
			prNDP->fgSecurityRequired,
			prNanCmdDataResponse->ucSecurity,
			prNDP->ucCipherType);
		prNanCmdDataResponse->ucDecisionStatus =
			NAN_DATA_RESP_DECISION_REJECT;

		/* Because NAN security SM init only after data response
		 * accept the connection establishment, to
		 * reset NDP security required to avoid touch security SM.
		 */
		prNDP->fgSecurityRequired = FALSE;
	}

#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
	/* Check GTK cipher */
	prNANInfo = prAdapter->prGlueInfo->aprNANDevInfo[NAN_NDI_INDEX_0];
	if (prNDP->fgSecurityRequired) {
		prNDP->fgTxGtkRequired =
			(prNanCmdDataResponse->ucGtkCipher) ? TRUE : FALSE;
		if (!nanSecIsDevSupportGroupSecurity(prAdapter))
			prNDP->fgTxGtkRequired = FALSE;

		if ((prNDP->fgTxGtkRequired != prNDP->fgRxGtkRequired) &&
			(prNANInfo->fgRejectNdpByGtkReq)) {
			prNanCmdDataResponse->ucDecisionStatus =
				NAN_DATA_RESP_DECISION_REJECT;
			DBGLOG(NAN, ERROR,
						   "Gtk required mismatch, %u, %u\n",
						   prNDP->fgTxGtkRequired,
						   prNDP->fgRxGtkRequired);
		} else if (prNDP->fgTxGtkRequired) {
			if (nanSecIsGroupCipherSupported(
				prNanCmdDataResponse->ucGtkCipher) == FALSE) {
				DBGLOG(NAN, ERROR,
					"Reject Gtk_cypher[%u]\n",
					prNanCmdDataResponse->ucGtkCipher);
				prNanCmdDataResponse->ucDecisionStatus =
					NAN_DATA_RESP_DECISION_REJECT;
			}
		}
	}
#endif /* CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT */

	if (prNanCmdDataResponse->ucDecisionStatus ==
	    NAN_DATA_RESP_DECISION_ACCEPT) {
		/* fill NDP parameters from response request */
		if (prNanCmdDataResponse->ucRequireQOS &
		    NAN_DATAREQ_REQUIRE_QOS_UNICAST)
			prNDP->fgQoSRequired = TRUE;

		else
			prNDP->fgQoSRequired = FALSE;

		if (prNanCmdDataResponse->ucSecurity) {
			DBGLOG(NAN, ERROR,
				"[Data Resp] ucSecurity: 1\n");
			kalMemCopy(prNDP->aucPMK, prNanCmdDataResponse->aucPMK,
				   NAN_PMK_INFO_LEN);

			/* Trigger NAN SEC: Responder
			 * nanSecStaSmBufReset(prNDP->prResponderSecSmInfo);
			 */
			nanSecSetPmk(prNDP, NAN_PMK_INFO_LEN,
				     prNanCmdDataResponse->aucPMK);
			nanSecSetCipherType(prNDP,
					    prNanCmdDataResponse->ucSecurity);
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_FRAME_PROT == 1)
			if (nanSecIsDevSupportGroupSecurity(prAdapter)) {
				prNDP->ucTxGtkCipherType =
					prNanCmdDataResponse->ucGtkCipher;
				prNDP->ucTxSecurityCap =
					nanSecGetGroupSecurityCap(prAdapter,
						prNDP);

				nan_sec_ndi_wpa_auth_init(NAN_NDI_INDEX_0,
					prNDP->ucTxGtkCipherType);

				nanSecSetGroupCipherType(prNDP, TRUE);
			} else
#endif
			{
				nan_sec_ndi_wpa_auth_init(NAN_NDI_INDEX_0,
					0);
			}
			nanSecNotify4wayBegin(prNDP);
			rStatus = nan_sec_wpa_sm_rx_eapol(
				prNDP->prInitiatorSecSmInfo,
				prNDP->prResponderSecSmInfo,
				prNDP->aucPeerNDIAddr);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, ERROR,
				       "ERROR! Process Rx share key attribute failed! rStatus:0x%x\n",
				       rStatus);
				return rStatus;
			}

		} else {
			DBGLOG(NAN, ERROR,
				"[Data Resp] ucSecurity: open\n");
			prNDP->fgSecurityRequired = FALSE;
		}

		/* always update in cases CMD Data Response asks for
		 * security policy change
		 */
		nanDataUpdateNdpLocalNDI(prAdapter, prNDP);

		if (prNDP->fgConfirmRequired == TRUE ||
		    prNDP->fgSecurityRequired == TRUE)
			prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_CONTINUED;

		else
			prNDP->ucNDPSetupStatus = NAN_ATTR_NDP_STATUS_ACCEPTED;

		if (prNanCmdDataResponse->u2SpecificInfoLength > 0)
			nanDataEngineUpdateAppInfo(
				prAdapter, prNDP,
				NAN_SERVICE_PROTOCOL_TYPE_GENERIC,
				prNanCmdDataResponse->u2SpecificInfoLength,
				prNanCmdDataResponse->aucSpecificInfo);
		else
			prNDP->u2AppInfoLen = 0;

		prNDP->fgCarryIPV6 = prNanCmdDataResponse->fgCarryIpv6;
		if (prNDP->fgCarryIPV6 == TRUE) {
			prNDP->fgIsInitiator = FALSE;
			kalMemCopy(prNDP->aucRspInterfaceId,
				   prNanCmdDataResponse->aucIPv6Addr,
				   IPV6MACLEN);
		}

		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_RESPONDER_WAIT_DATA_RSP) {
			/* roll the state machine .. */
			if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
			    prNDL->eCurrentNDLMgmtState ==
				    NDL_SCHEDULE_ESTABLISHED) {
				prNDL->prOperatingNDP = prNDP;
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_REQUEST_SCHEDULE_NDP,
						  prNDL);
#if (CFG_NAN_P2P_CONCURRENT_SUPPORT == 1)
				/* Send P2P data mutual exclusive */
				nanSchedNotifyExclusiveMode(prAdapter);
#endif /* CFG_NAN_P2P_CONCURRENT_SUPPORT */
			} else {
/* insert into queue for later handling */
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
				nanDataEngineInsertRequest(prAdapter,
					prNDL,
					NAN_DATA_ENGINE_REQUEST_NDP_SETUP,
					prNDL->eNDLRole,
					prNDP);
#endif
			}
		}
	} else {
		/* 1. retrieve NDP-ID from frame */
		prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
			prNDP->pucAttrList, prNDP->u2AttrListLength,
			NAN_ATTR_ID_NDP);
		prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
			prNDP->pucAttrList, prNDP->u2AttrListLength,
			NAN_ATTR_ID_NDP_EXTENSION);
		prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
			prNDP->pucAttrList, prNDP->u2AttrListLength,
			NAN_ATTR_ID_NDL);

		/* send DATA PATH RESPONSE frame with reject */
		nanNdpSendDataPathResponse(
			prAdapter, NULL, prNDL->aucPeerMacAddr, prAttrNDP,
			NAN_REASON_CODE_NDP_REJECTED, prAttrNDPE,
			NAN_REASON_CODE_NDP_REJECTED, prAttrNDL,
			NAN_REASON_CODE_NDP_REJECTED);

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanCmdDataEnd(IN struct ADAPTER *prAdapter,
	      struct _NAN_CMD_DATA_END *prNanCmdDataEnd) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanCmdDataEnd) {
		DBGLOG(NAN, ERROR, "prNanCmdDataEnd error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "ucNDPId:%d\n", prNanCmdDataEnd->ucNDPId);
#endif

	prNDP = nanDataUtilSearchNdpByNdpIdOnly(
		prAdapter, prNanCmdDataEnd->ucNDPId);

	if (!prNDP) {
		DBGLOG(NAN, WARN, "prNDP NULL, might already terminate\n");
		return WLAN_STATUS_FAILURE;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, WARN, "prNDL NULL, might already terminate\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, INFO, "NDP End with Peer["MACSTR"]\n",
		MAC2STR(prNDP->aucPeerNDIAddr));

	/* Transaction Id */
	prNDP->u2TransId = prNanCmdDataEnd->u2NdpTransactionId;

	if (prNDP->eCurrentNDPProtocolState == NDP_NORMAL_TR) {
		/* roll the state machine for disconnection handling */
		nanDataPathProtocolFsmStep(
			prAdapter, NDP_TX_DP_TERMINATION, prNDP);
	} else {
		if (prNDP->eCurrentNDPProtocolState == NDP_IDLE)
			nanDataEngineRemovePendingRequests(
				prAdapter, prNDL,
				NAN_DATA_ENGINE_REQUEST_NDP_SETUP,
				prNDP);

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
					   prNDP);
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanCmdDataUpdate(IN struct ADAPTER *prAdapter,
		 struct _NAN_PARAMETER_NDL_SCH *prNanUpdateSchParam) {
	return nanUpdateNdlSchedule(prAdapter, prNanUpdateSchParam);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanUpdateNdlSchedule(IN struct ADAPTER *prAdapter,
		     struct _NAN_PARAMETER_NDL_SCH *prNanUpdateSchParam) {
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
#endif
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanUpdateSchParam) {
		DBGLOG(NAN, ERROR,
			"prNanUpdateSchParam error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilSearchNdlByMac(
		prAdapter, prNanUpdateSchParam->aucPeerDataAddress);
#if 0
	if (prNDL != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter,
				prNDL, prNanCmdDataUpdate->ucNDPId);
#endif
	if (prNDL == NULL ||
	    prNDL->eCurrentNDLMgmtState != NDL_SCHEDULE_ESTABLISHED) {
		/* no matching NDL/NDP - it should have been created before */

		/* @TODO: return a unsuccessful indication to host side */
		rStatus = WLAN_STATUS_FAILURE;
	} else {
		prNDL->eNDLRole = NAN_PROTOCOL_INITIATOR;

		if (prNanUpdateSchParam->ucRequireQOS == TRUE) {
			prNDL->ucMinimumTimeSlot =
				prNanUpdateSchParam->ucMinTimeSlot;
			prNDL->u2MaximumLatency =
				prNanUpdateSchParam->u2MaxLatency;
		}

		nanSchedPeerPrepareNegoState(
			prAdapter, prNanUpdateSchParam->aucPeerDataAddress);
		/* roll NDL state machine for permission request */
		if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
		    prNDL->eCurrentNDLMgmtState == NDL_SCHEDULE_ESTABLISHED) {

			DBGLOG(NAN, INFO,
			       "Update SCH MinTimeSlot %d MaxLT %d\n",
			       prNDL->ucMinimumTimeSlot,
			       prNDL->u2MaximumLatency);
			nanNdlMgmtFsmStep(prAdapter, NDL_REQUEST_SCHEDULE_NDL,
					  prNDL);

		} else {
/* insert into queue for later handling */
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
			nanDataEngineInsertRequest(prAdapter,
					prNDL,
					NDL_REQUEST_SCHEDULE_NDL,
					prNDL->eNDLRole,
					prNDP);
#endif
		}

		rStatus = WLAN_STATUS_SUCCESS;
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    Send Data Indication event to host
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanNdpSendDataIndicationEvent(IN struct ADAPTER *prAdapter,
			      struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathRequestInd *prDataReqInd = NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	prDataReqInd = kalMemAlloc(
		 sizeof(*prDataReqInd),
		VIR_MEM_TYPE);

	if (!prDataReqInd) {
		DBGLOG(NAN, ERROR, "Alloc mem failed for Data path req\n");
		return;
	}
	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		kalMemFree(prDataReqInd,
			VIR_MEM_TYPE, sizeof(*prDataReqInd));
		return;
	}

	prDataReqInd->eventID = ENUM_NAN_DATA_INDICATION;
	prDataReqInd->service_instance_id = prNDP->ucPublishId;
	prDataReqInd->ndp_instance_id = prNDP->ucNDPID;
	prDataReqInd->ndp_cfg.qos_cfg = prNDP->fgQoSRequired ? 1 : 0;
	prDataReqInd->ndp_cfg.security_cfg = prNDP->fgSecurityRequired ? 1 : 0;
	DBGLOG(NAN, INFO, "Incoming Data Indication, PID:%d, NDPID:%d\n",
		prDataReqInd->service_instance_id,
		prDataReqInd->ndp_instance_id);
	if (nanDataEngineNDPECheck(prAdapter, prNDP->fgSupportNDPE) == TRUE) {
		prDataReqInd->fgSupportNDPE = TRUE;
		if (prNDP->fgIsInitiator == TRUE)
			kalMemCopy(prDataReqInd->aucIPv6Addr,
				   prNDP->aucRspInterfaceId, IPV6MACLEN);
		else
			kalMemCopy(prDataReqInd->aucIPv6Addr,
				   prNDP->aucInterfaceId, IPV6MACLEN);
	}
	kalMemCopy(prDataReqInd->peer_disc_mac_addr, prNDL->aucPeerMacAddr,
		   MAC_ADDR_LEN);

	if (prNDP->fgSecurityRequired) {
		kalMemCopy(&prDataReqInd->aucSCID[0], &prNDP->au1Scid[0],
			   NAN_SCID_DEFAULT_LEN);
		prDataReqInd->uCipher = prNDP->ucCipherType;

		DBGLOG(NAN, INFO, "uCipher:%d\n",
		       prDataReqInd->uCipher);
		dumpMemory8(prDataReqInd->aucSCID, NAN_SCID_DEFAULT_LEN);
	}

	kalIndicateNetlink2User(prAdapter->prGlueInfo, prDataReqInd,
				sizeof(struct NanDataPathRequestInd));
	kalMemFree(prDataReqInd,
		VIR_MEM_TYPE, sizeof(*prDataReqInd));

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief    Send Data Confirm event to host
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanNdpSendDataConfirmEvent(IN struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathConfirmInd *prDataConfirmInd = NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	prDataConfirmInd = kalMemAlloc(
				sizeof(*prDataConfirmInd),
				VIR_MEM_TYPE);

	if (!prDataConfirmInd) {
		DBGLOG(NAN, ERROR, "Alloc mem failed for Data path confirm\n");
		return;
	}
	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	prDataConfirmInd->eventID = ENUM_NAN_DATA_CONFIRM;
	prDataConfirmInd->ndp_instance_id = prNDP->ucNDPID;
	prDataConfirmInd->rsp_code =
		((prNDP->ucTxNextTypeStatus & NAN_ATTR_NDP_STATUS_MASK) >>
		 NAN_ATTR_NDP_STATUS_OFFSET);
	prDataConfirmInd->reason_code = prNDP->ucReasonCode;

	kalMemCopy(prDataConfirmInd->peer_ndi_mac_addr, prNDP->aucPeerNDIAddr,
		   MAC_ADDR_LEN);

	prDataConfirmInd->fgIsSec = prNDP->fgSecurityRequired;
	if (nanDataEngineNDPECheck(prAdapter, prNDP->fgSupportNDPE) == TRUE) {
		prDataConfirmInd->fgSupportNDPE = TRUE;
		if (prNDP->fgIsInitiator == TRUE)
			kalMemCopy(prDataConfirmInd->aucIPv6Addr,
				   prNDP->aucRspInterfaceId, IPV6MACLEN);
		else
			kalMemCopy(prDataConfirmInd->aucIPv6Addr,
				   prNDP->aucInterfaceId, IPV6MACLEN);
		prDataConfirmInd->u2Port = prNDP->u2PortNum;
		prDataConfirmInd->ucProtocol = prNDP->ucProtocolType;
	}
	kalIndicateNetlink2User(prAdapter->prGlueInfo, prDataConfirmInd,
				sizeof(struct NanDataPathConfirmInd));
	kalMemFree(prDataConfirmInd,
		VIR_MEM_TYPE, sizeof(*prDataConfirmInd));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
void
nanNdpSendDataTerminationEvent(IN struct ADAPTER *prAdapter,
			       struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathEndInd rDataEndInd = {0};

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	rDataEndInd.eventID = ENUM_NAN_DATA_TERMINATE;
	rDataEndInd.num_ndp_instances = 1;
	rDataEndInd.ndp_instance_id = prNDP->ucNDPID;
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rDataEndInd,
				sizeof(struct NanDataPathEndInd));
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Data Path Request
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpSendDataPathRequest(IN struct ADAPTER *prAdapter,
			  struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint32_t i = 0;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucPeerAddr = NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, prNDP);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_DATA_PATH_REQUEST,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	/* Save M1 for SEC auth token */
	if (prNDP->fgSecurityRequired) {
		/* The body of Mx beginning from the Category */
		pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
			      OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		u1TxMsgLen = u2EstimatedFrameLen -
			     OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		nanSecNotifyMsgBodyRdy(prNDP, NAN_SEC_M1, u1TxMsgLen,
				       pu1TxMsgBuf);
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDPReqTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Data Path Response
 *
 * \param[in]        Parameter 3~9 only valid when prNDP == NULL
 *                   When prNDP == NULL, always send Data Path Response with
 *                   StatusCode == REJECT (2)
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanNdpSendDataPathResponse(
	IN struct ADAPTER *prAdapter, struct _NAN_NDP_INSTANCE_T *prNDP,

	/* below are optional params, only valid when prNDP == NULL */
	IN uint8_t *pucDestMacAddr, IN struct _NAN_ATTR_NDP_T *prPeerAttrNDP,
	IN uint8_t ucNDPReasonCode, IN struct _NAN_ATTR_NDPE_T *prPeerAttrNDPE,
	IN uint8_t ucNDPEReasonCode, IN struct _NAN_ATTR_NDL_T *prPeerAttrNDL,
	IN uint8_t ucNDLReasonCode) {
	uint32_t i = 0;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucPeerAddr = NULL;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (prNDP)
		prNDL = nanDataUtilGetNdl(prAdapter, prNDP);
	else
		prNDL = NULL;

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, prNDP);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	if (pucDestMacAddr)
		pucPeerAddr = pucDestMacAddr;
	else if (prNDL)
		pucPeerAddr = prNDL->aucPeerMacAddr;
	else {
		DBGLOG(NAN, ERROR, "pucDestMacAddr error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_DATA_PATH_RESPONSE,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	if (prNDP) {
		/* fill NAN attributes */
		for (i = 0; i < sizeof(txDataAttributeTable) /
					sizeof(struct _APPEND_ATTR_ENTRY_T);
		     i++) {
			if (txDataAttributeTable[i]
				    .pfnCalculateVariableAttrLen &&
			    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
				    prAdapter, prNDL, prNDP) != 0) {
				if (txDataAttributeTable[i].pfnAppendAttr)
					txDataAttributeTable[i].pfnAppendAttr(
						prAdapter, prMsduInfo, prNDL,
						prNDP);
			}
		}

		/* For MIC calulation */
		if (prNDP->fgSecurityRequired) {
			/* The body of Mx beginning from the Category */
			pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
				      OFFSET_OF(struct _NAN_ACTION_FRAME_T,
						ucCategory);
			u1TxMsgLen = u2EstimatedFrameLen -
				     OFFSET_OF(struct _NAN_ACTION_FRAME_T,
					       ucCategory);

			rStatus = nanSecNotifyMsgBodyRdy(
				prNDP, NAN_SEC_M2, u1TxMsgLen, pu1TxMsgBuf);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, ERROR,
				       "ERROR! MIC calulation failed!\n");
				return rStatus;
			}
		}
	} else { /* prNDP == NULL */
		if (prPeerAttrNDP)
			nanDataEngineNDPAttrAppendImpl(
				prAdapter, prMsduInfo, NULL, NULL,
				prPeerAttrNDP,
				NAN_ATTR_NDP_TYPE_RESPONSE |
					(NAN_ATTR_NDP_STATUS_REJECTED
					 << NAN_ATTR_NDP_STATUS_OFFSET),
				ucNDPReasonCode);

		if (prPeerAttrNDPE)
			nanDataEngineNDPEAttrAppendImpl(
				prAdapter, prMsduInfo, NULL, NULL,
				prPeerAttrNDPE,
				NAN_ATTR_NDPE_TYPE_RESPONSE |
					(NAN_ATTR_NDPE_STATUS_REJECTED
					 << NAN_ATTR_NDPE_STATUS_OFFSET),
				ucNDPEReasonCode);

		if (prPeerAttrNDL)
			nanDataEngineNDLAttrAppendImpl(
				prAdapter, prMsduInfo, NULL, prPeerAttrNDL,
				NAN_ATTR_NDL_TYPE_RESPONSE |
					(NAN_ATTR_NDL_STATUS_REJECTED
					 << NAN_ATTR_NDL_STATUS_OFFSET),
				ucNDLReasonCode);
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDPRespTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Data Path Confirm
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpSendDataPathConfirm(IN struct ADAPTER *prAdapter,
			  struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint32_t i = 0;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucPeerAddr = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, prNDP);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_DATA_PATH_CONFIRM,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	/* For MIC calulation */
	if (prNDP->fgSecurityRequired) {
		/* The body of Mx beginning from the Category */
		pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
			      OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		u1TxMsgLen = u2EstimatedFrameLen -
			     OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);

		rStatus = nanSecNotifyMsgBodyRdy(prNDP, NAN_SEC_M3, u1TxMsgLen,
						 pu1TxMsgBuf);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
			       "ERROR! MIC calulation failed!\n");
			return rStatus;
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDPConfirmTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Data Path Key Installment
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpSendDataPathKeyInstall(IN struct ADAPTER *prAdapter,
			     struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint32_t i = 0;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct STA_RECORD *prStaRec = NULL;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr = NULL;
	uint8_t *pucPeerAddr = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, prNDP);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_DATA_PATH_KEY_INSTALLMENT,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	/* For MIC calulation */
	if (prNDP->fgSecurityRequired) {
		/* The body of Mx beginning from the Category */
		pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
			      OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		u1TxMsgLen = u2EstimatedFrameLen -
			     OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);

		rStatus = nanSecNotifyMsgBodyRdy(prNDP, NAN_SEC_M4, u1TxMsgLen,
						 pu1TxMsgBuf);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
			       "ERROR! MIC calulation failed!\n");
			return rStatus;
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDPSecurityInstallTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Data Path Termination
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpSendDataPathTermination(IN struct ADAPTER *prAdapter,
		struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, prNDP);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_DATA_PATH_TERMINATION,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDPTerminationTxDone, prStaRec);
}

#if CFG_NAN_SUPPORT_OOB_AF_SLOT_BASE
/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAN OOB action frame TX Done event handler
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanOobActionFrameTxDoneSlotBase(struct ADAPTER *prAdapter,
		uint8_t *pcuEvtBuf)
{
	struct _NAN_OOB_ACTION_TXS_EVT_T *prOobTxDone = NULL;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, INFO, "[OobAF] Enter, buf:%p\n", pcuEvtBuf);
	prOobTxDone = (struct _NAN_OOB_ACTION_TXS_EVT_T *)pcuEvtBuf;

	DBGLOG(NAN, INFO, "[OobAF] Token %d, sts: %08X\n",
		prOobTxDone->u2Token, prOobTxDone->u4TxStatus);

	mtk_cfg80211_vendor_event_nan_oob_af_tx(prAdapter,
		prOobTxDone->u2Token,
		prOobTxDone->u4TxStatus);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send OOB Action frame byt command
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanCmdSendNanOobActionAuthFrameSlotBase(struct ADAPTER *prAdapter,
		struct _NAN_CMD_OOB_ACTION *prNanCmdOOBAction)
{
	uint8_t *pucLocalAddr = NULL;
	struct _NAN_ACTION_FRAME_T *prOobNAF = NULL;
	struct WLAN_AUTH_FRAME *prOobAuthFrame = NULL;
	uint16_t u2EstimatedFrameLen = 0;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;

	/* for CMD_TX_OOB_ACTION_FRAME */
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	void *prCmdBuffer = NULL;
	size_t szCmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_OOB_ACTION_AUTH_FRAME_T *
		prCmdNanOobActAuthFrame = NULL;
	uint64_t u8CorrectedBitmap = 0;

	if (!prNanCmdOOBAction) {
		DBGLOG(NAN, ERROR, "prNanCmdOOBAction is NULL\n");
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	/* payload len */
	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory) +
		prNanCmdOOBAction->u2PayloadLen;

	/* Creater Cmd, need to copy txd and payload into  */
	szCmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_OOB_ACTION_AUTH_FRAME_T) +
			 u2EstimatedFrameLen;

	DBGLOG(NAN, INFO, "[OobAF] framelen: %d cmd buf: %lu\n",
		u2EstimatedFrameLen, szCmdBufferLen);

	prCmdBuffer = cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, (uint32_t)szCmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_TX_OOB_ACTION_FRAME,
		sizeof(struct _NAN_CMD_OOB_ACTION_AUTH_FRAME_T) +
			u2EstimatedFrameLen,
		(uint32_t)szCmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	prCmdNanOobActAuthFrame =
		(struct _NAN_CMD_OOB_ACTION_AUTH_FRAME_T *)
		prTlvElement->aucbody;

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;

	if (prNanCmdOOBAction->ucFrameType == OOB_TYPE_ACTION) {
		if (kalMemCmp(prNanCmdOOBAction->aucDestAddress,
			aucZeroMacAddr, MAC_ADDR_LEN) == 0) {
			DBGLOG(NAN, ERROR, "null mac error\n");
			cnmMemFree(prAdapter, prCmdBuffer);
			return NAN_STATUS_INTERNAL_FAILURE;
		}

		prOobNAF = (struct _NAN_ACTION_FRAME_T *)
			&(prCmdNanOobActAuthFrame->aucOobActionAuthFrame);

		/* MAC header */
		COPY_MAC_ADDR(prOobNAF->aucDestAddr,
			prNanCmdOOBAction->aucDestAddress);
		COPY_MAC_ADDR(prOobNAF->aucSrcAddr,
			pucLocalAddr);
		COPY_MAC_ADDR(prOobNAF->aucClusterID,
			nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_MAIN)
				->aucClusterId);
		WLAN_SET_FIELD_16(&(prOobNAF->u2FrameCtrl), MAC_FRAME_ACTION);
		prOobNAF->u2SeqCtrl = 0;

		/* Copy payload */
		kalMemCpyS(&(prOobNAF->ucCategory),
			prNanCmdOOBAction->u2PayloadLen,
			prNanCmdOOBAction->aucPayload,
			prNanCmdOOBAction->u2PayloadLen);
	} else {
		prOobAuthFrame = (struct WLAN_AUTH_FRAME *)
			&(prCmdNanOobActAuthFrame->aucOobActionAuthFrame);

		/* MAC header */
		COPY_MAC_ADDR(prOobAuthFrame->aucDestAddr,
			prNanCmdOOBAction->aucDestAddress);
		COPY_MAC_ADDR(prOobAuthFrame->aucSrcAddr,
			pucLocalAddr);
		COPY_MAC_ADDR(prOobAuthFrame->aucBSSID,
			nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_MAIN)
				->aucClusterId);
		WLAN_SET_FIELD_16(&(prOobAuthFrame->u2FrameCtrl),
			MAC_FRAME_AUTH);
		/* Copy payload */
		kalMemCpyS((uint8_t *)&(prOobAuthFrame->u2AuthAlgNum),
			prNanCmdOOBAction->u2PayloadLen,
			prNanCmdOOBAction->aucPayload,
			prNanCmdOOBAction->u2PayloadLen);
	}

	prCmdNanOobActAuthFrame->u2FrameLen = u2EstimatedFrameLen;
	prCmdNanOobActAuthFrame->ucBitmapId = prNanCmdOOBAction->ucMapId;
	prCmdNanOobActAuthFrame->u2Timeout = prNanCmdOOBAction->u2Timeout;
	prCmdNanOobActAuthFrame->ucSecType = prNanCmdOOBAction->ucSecurity;
	prCmdNanOobActAuthFrame->u2Token = prNanCmdOOBAction->u2Token;
	u8CorrectedBitmap = prNanCmdOOBAction->u8BitMap;
	nanSchedGetPeerOobAvailableBitmap(
		prAdapter, prNanCmdOOBAction->ucMapId,
		&u8CorrectedBitmap,
		prNanCmdOOBAction->aucDestAddress);

	/* OOB may fail due to map mismatch with peer */
	if (u8CorrectedBitmap == 0)
		DBGLOG(NAN, ERROR,
			"[OobNAF] A mismacth between local bitmap and peer's\n");

	prCmdNanOobActAuthFrame->u4Bitmap[0] =
		u8CorrectedBitmap & BITS(0, 31);
	prCmdNanOobActAuthFrame->u4Bitmap[1] =
		(u8CorrectedBitmap >> 32) & BITS(0, 31);
	DBGLOG(NAN, INFO, "[OobNAF] DstAddr: "MACSTR", Type:%u, Len:%u\n",
		MAC2STR(prNanCmdOOBAction->aucDestAddress),
		prNanCmdOOBAction->ucFrameType,
		u2EstimatedFrameLen);
	dumpMemory8(
		(uint8_t *)&prCmdNanOobActAuthFrame->aucOobActionAuthFrame[0],
		u2EstimatedFrameLen);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
					FALSE, FALSE, nanDevCommonSetCb,
					nicCmdTimeoutCommon,
					(uint32_t)szCmdBufferLen,
					(uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	return (rStatus != WLAN_STATUS_FAILURE) ?
		NAN_STATUS_SUCCESS : NAN_STATUS_INTERNAL_FAILURE;
}
#endif /* CFG_NAN_SUPPORT_OOB_AF_SLOT_BASE */


/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Schedule Request
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlSendScheduleRequest(IN struct ADAPTER *prAdapter,
			  struct _NAN_NDL_INSTANCE_T *prNDL) {
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, NULL);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_SCHEDULE_REQUEST, pucLocalAddr,
				pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDataEngineScheduleReqTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Schedule Response
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlSendScheduleResponse(
	IN struct ADAPTER *prAdapter, IN struct _NAN_NDL_INSTANCE_T *prNDL,

	/* below are optional params, only valid when prNDL == NULL */
	IN uint8_t *pucDestMacAddr, IN struct _NAN_ATTR_NDL_T *prPeerAttrNDL,
	IN uint8_t ucReasonCode) {
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	/* struct _NAN_ACTION_FRAME_T* prNAF; */
	/* struct _NAN_ATTR_NDL_T* prAttrNDL; */
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, NULL);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	if (pucDestMacAddr)
		pucPeerAddr = pucDestMacAddr;
	else if (prNDL)
		pucPeerAddr = prNDL->aucPeerMacAddr;
	else {
		DBGLOG(NAN, ERROR, "aucPeerMacAddr error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_SCHEDULE_RESPONSE,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	if (prNDL) {
		/* fill NAN attributes */
		for (i = 0; i < sizeof(txDataAttributeTable) /
					sizeof(struct _APPEND_ATTR_ENTRY_T);
		     i++) {
			if (txDataAttributeTable[i]
				    .pfnCalculateVariableAttrLen &&
			    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
				    prAdapter, prNDL, NULL) != 0) {
				if (txDataAttributeTable[i].pfnAppendAttr)
					txDataAttributeTable[i].pfnAppendAttr(
						prAdapter, prMsduInfo, prNDL,
						NULL);
			}
		}
	} else { /* prNDL == NULL */
		if (prPeerAttrNDL)
			nanDataEngineNDLAttrAppendImpl(
				prAdapter, prMsduInfo, NULL, prPeerAttrNDL,
				NAN_ATTR_NDL_TYPE_RESPONSE |
					(NAN_ATTR_NDL_STATUS_REJECTED
					 << NAN_ATTR_NDL_STATUS_OFFSET),
				ucReasonCode);
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDataEngineScheduleRespTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Schedule Confirm
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanNdlSendScheduleConfirm(IN struct ADAPTER *prAdapter,
			  struct _NAN_NDL_INSTANCE_T *prNDL) {
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, NULL);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_SCHEDULE_CONFIRM, pucLocalAddr,
				pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)nanDataEngineScheduleConfirmTxDone,
		prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Schedule Update Notify
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlSendScheduleUpdateNotify(IN struct ADAPTER *prAdapter,
			       struct _NAN_NDL_INSTANCE_T *prNDL) {
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen) {
			u2EstimatedFrameLen +=
				txDataAttributeTable[i]
					.pfnCalculateVariableAttrLen(
						prAdapter, prNDL, NULL);
		}
	}

	/* allocate MSDU_INFO_T */
	prMsduInfo = cnmMgtPktAlloc(prAdapter, u2EstimatedFrameLen);
	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN,
		       "NAN Data Engine: packet allocation failure\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero((uint8_t *)prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION,
				pucLocalAddr, pucPeerAddr,
				nanSecIsPMFApply(prAdapter,
					pucLocalAddr, pucPeerAddr, NULL));

	/* fill NAN attributes */
	for (i = 0; i < sizeof(txDataAttributeTable) /
				sizeof(struct _APPEND_ATTR_ENTRY_T);
	     i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	prStaRec = nanDataEngineSearchStaRec(prAdapter, prNDL, pucLocalAddr,
						 pucPeerAddr);

	return nanDataEngineSendNAF(
		prAdapter, prMsduInfo, prMsduInfo->u2FrameLength,
		(PFN_TX_DONE_HANDLER)
			nanDataEngineScheduleUpdateNotificationTxDone,
		prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Done Callback - for NDP negotiation (DP Request)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDPReqTxDone(IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo,
	       IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* search for NDP-ID */
	pucAttrList = (uint8_t *)(prNAF->aucInfoContent);
	u2AttrListLength =
		prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);

	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		DBGLOG(NAN, ERROR, "Not found the NDPID\n");
		return WLAN_STATUS_SUCCESS;
	}
	if (prNDP == NULL) {
		/* weird condition */
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		/* Notify SEC */
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M1);

		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_INITIATOR_TX_DP_REQUEST)
			nanDataPathProtocolFsmStep(
				prAdapter, NDP_INITIATOR_RX_DP_RESPONSE, prNDP);
	} else {
		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_INITIATOR_TX_DP_REQUEST) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_INITIATOR_TX_DP_REQUEST,
					prNDP);
			else
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
		}
	}

	/* Send rsp event to wifi hal */
	nanNdpInitiatorRspEvent(prAdapter, prNDP, rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Done Callback - for NDP negotiation (DP Response)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDPRespTxDone(IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo,
		IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* search for NDP-ID */
	pucAttrList = (uint8_t *)(prNAF->aucInfoContent);
	u2AttrListLength =
		prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);

	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);

	if (prAttrNDP != NULL) {
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);

		DBGLOG(NAN, INFO, "prAttrNDP exist\n");
	} else if (prAttrNDPE != NULL) {
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);

		DBGLOG(NAN, INFO, "prAttrNDPE exist\n");
	} else {
		DBGLOG(NAN, ERROR, "Not found the NDPID\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (prNDP == NULL) {
		/* weird condition */
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* Send rsp event to wifi hal */
	nanNdpResponderRspEvent(prAdapter, prNDP, rTxDoneStatus);

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		/* Notify SEC */
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M2);

		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_RESPONDER_TX_DP_RESPONSE) {
			switch (prNDP->ucNDPSetupStatus) {
			case NAN_ATTR_NDP_STATUS_CONTINUED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_RESPONDER_RX_DP_CONFIRM,
					prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_ACCEPTED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_NORMAL_TR, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_REJECTED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
				break;
			}
		}
	} else {
		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_RESPONDER_TX_DP_RESPONSE) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_RESPONDER_TX_DP_RESPONSE,
					prNDP);
			else
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Done Callback - for NDP negotiation (DP Confirm)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDPConfirmTxDone(IN struct ADAPTER *prAdapter,
		   IN struct MSDU_INFO *prMsduInfo,
		   IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* search for NDP-ID */
	pucAttrList = (uint8_t *)(prNAF->aucInfoContent);
	u2AttrListLength =
		prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		DBGLOG(NAN, ERROR, "Not found the NDPID\n");
		return WLAN_STATUS_SUCCESS;
	}
	if (prNDP == NULL) {
		/* weird condition */
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		/* Notify SEC */
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M3);

		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_INITIATOR_TX_DP_CONFIRM) {
			switch (prNDP->ucNDPSetupStatus) {
			case NAN_ATTR_NDP_STATUS_CONTINUED:
				nanDataPathProtocolFsmStep(
					prAdapter,
					NDP_INITIATOR_RX_DP_SECURITY_INSTALL,
					prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_ACCEPTED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_NORMAL_TR, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_REJECTED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
				break;
			}
		}
	} else {
		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_INITIATOR_TX_DP_CONFIRM) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_INITIATOR_TX_DP_CONFIRM,
					prNDP);
			else
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief       NAF TX Done Callback - for NDP negotiation (DP Security Install)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDPSecurityInstallTxDone(IN struct ADAPTER *prAdapter,
			   IN struct MSDU_INFO *prMsduInfo,
			   IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* search for NDP-ID */
	pucAttrList = (uint8_t *)(prNAF->aucInfoContent);
	u2AttrListLength =
		prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		DBGLOG(NAN, ERROR, "Not found the NDPID\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (prNDP == NULL) {
		/* weird condition */
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		/* Notify SEC */
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M4);

		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_RESPONDER_TX_DP_SECURITY_INSTALL) {
			switch (prNDP->ucNDPSetupStatus) {
			case NAN_ATTR_NDP_STATUS_ACCEPTED:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_NORMAL_TR, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_REJECTED:
			default:
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
				break;
			}
		}
	} else {
		if (prNDP->eCurrentNDPProtocolState ==
		    NDP_RESPONDER_TX_DP_SECURITY_INSTALL) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanDataPathProtocolFsmStep(
					prAdapter,
					NDP_RESPONDER_TX_DP_SECURITY_INSTALL,
					prNDP);
			else
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Done Callback - for NDP negotiation (DP Termination)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDPTerminationTxDone(IN struct ADAPTER *prAdapter,
		       IN struct MSDU_INFO *prMsduInfo,
		       IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* search for NDP-ID */
	pucAttrList = (uint8_t *)(prNAF->aucInfoContent);
	u2AttrListLength =
		prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		pucAttrList, u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	if (prAttrNDP != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDP->ucNDPID);
	else if (prAttrNDPE != NULL)
		prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, prNDL,
						    prAttrNDPE->ucNDPID);
	else {
		prNDP = NULL;
		DBGLOG(NAN, ERROR, "Not found the NDPID\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (prNDP == NULL) {
		/* weird condition */
		DBGLOG(NAN, ERROR, "NDP is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		if (prNDP->eCurrentNDPProtocolState == NDP_TX_DP_TERMINATION)
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
	} else {
		if (prNDP->eCurrentNDPProtocolState == NDP_TX_DP_TERMINATION) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			else
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
		}
	}

	/* Send rsp event to wifi hal*/
	nanNdpEndRspEvent(prAdapter, prNDP, rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief          NAF TX Done Callback - for NDL negotiation (Schedule Request)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineScheduleReqTxDone(IN struct ADAPTER *prAdapter,
			       IN struct MSDU_INFO *prMsduInfo,
			       IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_TX_SCHEDULE_REQUEST)
			nanNdlMgmtFsmStep(
				prAdapter,
				NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE,
				prNDL);
		/* else
		 * unexpected state - ignore
		 */
	} else {
		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_TX_SCHEDULE_REQUEST) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_INITIATOR_TX_SCHEDULE_REQUEST,
					prNDL);
			else
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
		} else {
			/* unexpected state - ignore */
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brie          NAF TX Done Callback - for NDL negotiation (Schedule Response)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineScheduleRespTxDone(IN struct ADAPTER *prAdapter,
				IN struct MSDU_INFO *prMsduInfo,
				IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_RESPONDER_TX_SCHEDULE_RESPONSE) {
			if (prNDL->ucNDLSetupCurrentStatus ==
			    NAN_ATTR_NDL_STATUS_CONTINUED)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_RESPONDER_RX_SCHEDULE_CONFIRM,
					prNDL);

			else if (prNDL->ucNDLSetupCurrentStatus ==
				 NAN_ATTR_NDL_STATUS_ACCEPTED) {
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			} else if (prNDL->ucNDLSetupCurrentStatus ==
				   NAN_ATTR_NDL_STATUS_REJECTED)
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);

			else {
				DBGLOG(NAN, ERROR,
					"ucNDLSetupCurrentStatus invalid\n");
				return WLAN_STATUS_INVALID_DATA;
			}
		}
	} else {
		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_RESPONDER_TX_SCHEDULE_RESPONSE) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_RESPONDER_TX_SCHEDULE_RESPONSE,
					prNDL);

			else
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
		} else {
			/* unexpected state - ignore */
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief          NAF TX Done Callback - for NDL negotiation (Schedule Confirm)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineScheduleConfirmTxDone(IN struct ADAPTER *prAdapter,
				   IN struct MSDU_INFO *prMsduInfo,
				   IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));

		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_TX_SCHEDULE_CONFIRM) {
			if (prNDL->ucNDLSetupCurrentStatus ==
			    NAN_ATTR_NDL_STATUS_ACCEPTED) {
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			} else if (prNDL->ucNDLSetupCurrentStatus ==
				   NAN_ATTR_NDL_STATUS_REJECTED)
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
			else {
				DBGLOG(NAN, ERROR,
					"ucNDLSetupCurrentStatus invalid\n");
				return WLAN_STATUS_INVALID_DATA;
			}
		}
	} else {
		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_TX_SCHEDULE_CONFIRM) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_INITIATOR_TX_SCHEDULE_CONFIRM,
					prNDL);

			else
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
		} else {
			/* unexpected state - ignore */
		}
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Done Callback - for NDL negotiation -
 *                                            (Schedule Update Notification)
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineScheduleUpdateNotificationTxDone(
	IN struct ADAPTER *prAdapter, IN struct MSDU_INFO *prMsduInfo,
	IN enum ENUM_TX_RESULT_CODE rTxDoneStatus) {
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter, Status:%x\n", rTxDoneStatus);
#endif

	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "prMsduInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "NDL is null\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));
		nanNdlMgmtFsmStep(prAdapter, NDL_SCHEDULE_ESTABLISHED, prNDL);
	} else {
		prNDL->ucTxRetryCounter++;

		if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
			nanNdlSendScheduleUpdateNotify(prAdapter, prNDL);

		else
			nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN, prNDL);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAF TX Wrapper Function
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineSendNAF(IN struct ADAPTER *prAdapter,
		     IN struct MSDU_INFO *prMsduInfo, IN uint16_t u2FrameLength,
		     IN PFN_TX_DONE_HANDLER pfTxDoneHandler,
		     IN struct STA_RECORD *prSelectStaRec) {
	struct _NAN_ACTION_FRAME_T *prNAF = NULL;
	uint8_t ucWtblEntry = 0;
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	/* 4 <3> Update information of MSDU_INFO_T */
	TX_SET_MMPDU(
		prAdapter, prMsduInfo,
		(prSelectStaRec != NULL)
			? (prSelectStaRec->ucBssIndex)
			: nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_MAIN)
				  ->ucBssIndex,
		(prSelectStaRec != NULL) ? (prSelectStaRec->ucIndex)
					 : (STA_REC_INDEX_NOT_FOUND),
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory),
		u2FrameLength, pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	prMsduInfo->ucTxToNafQueFlag = TRUE;

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;
	if (!prAdapter->rWifiVar.fgNoPmf &&
		nanSecIsPMFApply(prAdapter,
			prNAF->aucSrcAddr,
			prNAF->aucDestAddr,
			&ucWtblEntry)) {
		nicTxConfigPktOption(prMsduInfo, MSDU_OPT_PROTECTED_FRAME,
				     TRUE);
		nicTxSetWtblEntry(prMsduInfo, ucWtblEntry);
		DBGLOG(NAN, INFO,
			"Tx PMF, OUItype:%d, OUISubtype:%d option[%x]\n",
			prNAF->ucOUItype,
			prNAF->ucOUISubtype,
			prMsduInfo->u4Option);
		if (prSelectStaRec)
			DBGLOG(NAN, INFO,
		       "StaIdx:%d, MAC=>%02x:%02x:%02x:%02x:%02x:%02x\n",
		       prSelectStaRec->ucIndex, prSelectStaRec->aucMacAddr[0],
		       prSelectStaRec->aucMacAddr[1],
		       prSelectStaRec->aucMacAddr[2],
		       prSelectStaRec->aucMacAddr[3],
		       prSelectStaRec->aucMacAddr[4],
		       prSelectStaRec->aucMacAddr[5]);
	}

	/* 4 <6> Enqueue the frame to send this NAF frame. */
	nicTxEnqueueMsdu(prAdapter, prMsduInfo);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDP TX Type Status Generation
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpUpdateTypeStatus(IN struct ADAPTER *prAdapter,
		       IN struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint8_t ucType = 0;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	switch (prNDP->eCurrentNDPProtocolState) {
	case NDP_INITIATOR_TX_DP_REQUEST:
		ucType = NAN_ATTR_NDP_TYPE_REQUEST;
		break;

	case NDP_INITIATOR_TX_DP_CONFIRM:
		ucType = NAN_ATTR_NDP_TYPE_CONFIRM;
		break;

	case NDP_RESPONDER_TX_DP_RESPONSE:
		ucType = NAN_ATTR_NDP_TYPE_RESPONSE;
		break;

	case NDP_RESPONDER_TX_DP_SECURITY_INSTALL:
		ucType = NAN_ATTR_NDP_TYPE_SEC_INSTALL;
		break;

	case NDP_TX_DP_TERMINATION:
		ucType = NAN_ATTR_NDP_TYPE_TERMINATE;
		break;

	default:
		DBGLOG(NAN, ERROR,
			"eCurrentNDPProtocolState invalid\n");
		return WLAN_STATUS_INVALID_DATA;
		/* for other state, no need to generate Type Status */
	}

	prNDP->ucTxNextTypeStatus =
		(((prNDP->ucNDPSetupStatus << NAN_ATTR_NDP_STATUS_OFFSET) &
		  NAN_ATTR_NDP_STATUS_MASK) |
		 (ucType & NAN_ATTR_NDP_TYPE_MASK));

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDP Dialog Token Generation
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpGenerateDialogToken(IN struct ADAPTER *prAdapter,
			  IN struct _NAN_NDP_INSTANCE_T *prNDP) {
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP->eNDPRole != NAN_PROTOCOL_INITIATOR) {
		DBGLOG(NAN, ERROR, "eNDPRole error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP->ucDialogToken < UINT8_MAX)
		prNDP->ucDialogToken++;

	else
		prNDP->ucDialogToken = 1; /* always non-zero */

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL Dialog Token Generation
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlGenerateDialogToken(IN struct ADAPTER *prAdapter,
			  IN struct _NAN_NDL_INSTANCE_T *prNDL) {
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDL->eNDLRole != NAN_PROTOCOL_INITIATOR) {
		DBGLOG(NAN, ERROR, "eNDLRole error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDL->ucDialogToken < UINT8_MAX)
		prNDL->ucDialogToken++;

	else
		prNDL->ucDialogToken = 1; /* always non-zero */

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDP/NDPE APP-INFO Buffer/Update Function
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineUpdateSSI(IN struct ADAPTER *prAdapter,
		IN struct _NAN_NDP_INSTANCE_T *prNDP,
		IN uint8_t ucServiceProtocolType, IN uint16_t u2ContextLen,
		IN uint8_t *pucContext) {
	uint32_t rStatus;
	uint16_t u2Length = 0;
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u2ContextLen <= 0) {
		DBGLOG(NAN, ERROR,
			"prNu2ContextLenDL invalid\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	rStatus = WLAN_STATUS_SUCCESS;
	if (ucServiceProtocolType == NAN_SERVICE_PROTOCOL_TYPE_GENERIC) {
		struct _NAN_ATTR_HDR_T *prSubNanAttr;

		while (u2Length < u2ContextLen) {
			prSubNanAttr = (struct _NAN_ATTR_HDR_T *)pucContext;
			switch (prSubNanAttr->ucAttrId) {
			case NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_TRANSPORT_PORT: {
				kalMemCopy(&prNDP->u2PortNum,
					prSubNanAttr->aucAttrBody,
					sizeof(uint16_t));
				DBGLOG(NAN, INFO, "PortNum %d\n",
				       prNDP->u2PortNum);
				break;
			}
			case NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_PROTOCOL: {
				prNDP->ucProtocolType =
					prSubNanAttr->aucAttrBody[0];
				DBGLOG(NAN, INFO, "ucProtocolType %d\n",
				       prNDP->ucProtocolType);
				break;
			}
			case NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_SPECINFO: {
				if (prNDP->pucAppInfo) {
					/* already exist ? free it first */
					cnmMemFree(prAdapter,
						   prNDP->pucAppInfo);
					prNDP->pucAppInfo = NULL;
					prNDP->u2AppInfoLen = 0;
				}

				prNDP->pucAppInfo = cnmMemAlloc(
					prAdapter, RAM_TYPE_BUF,
					(uint32_t)prSubNanAttr->u2Length);
				if (prNDP->pucAppInfo) {
					kalMemCopy(prNDP->pucAppInfo,
						   prSubNanAttr->aucAttrBody,
						   prSubNanAttr->u2Length);
					prNDP->u2AppInfoLen =
						prSubNanAttr->u2Length;
				}
				DBGLOG(NAN, INFO, "SPECIFIC INFO %d\n",
				       prNDP->u2AppInfoLen);
				break;
			}
			default: {
				DBGLOG(NAN, INFO,
				       "Sub attribute type not know %x\n",
				       prSubNanAttr->ucAttrId);
				rStatus = WLAN_STATUS_FAILURE;
			}
			}
			u2Length +=
				OFFSET_OF(struct _NAN_ATTR_NDPE_GENERAL_TLV_T,
					  aucValue) +
				prSubNanAttr->u2Length;
			pucContext +=
				OFFSET_OF(struct _NAN_ATTR_NDPE_GENERAL_TLV_T,
					  aucValue) +
				prSubNanAttr->u2Length;
		}
	}
	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDP/NDPE APP-INFO Buffer/Update Function
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineUpdateAppInfo(IN struct ADAPTER *prAdapter,
			   IN struct _NAN_NDP_INSTANCE_T *prNDP,
			   IN uint8_t ucServiceProtocolType,
			   IN uint16_t u2AppInfoLen, IN uint8_t *pucAppInfo) {
	uint32_t rStatus;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u2AppInfoLen <= 0) {
		DBGLOG(NAN, ERROR, "u2AppInfoLen invalid\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pucAppInfo) {
		DBGLOG(NAN, ERROR, "pucAppInfo error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDP->ucServiceProtocolType = ucServiceProtocolType;

	if (prNDP->pucAppInfo != NULL) {
		/* already exist ? free it first */
		cnmMemFree(prAdapter, prNDP->pucAppInfo);
		prNDP->pucAppInfo = NULL;
		prNDP->u2AppInfoLen = 0;
	}

	prNDP->pucAppInfo =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, (uint32_t)u2AppInfoLen);
	if (prNDP->pucAppInfo) {
		kalMemCopy(prNDP->pucAppInfo, pucAppInfo, u2AppInfoLen);
		prNDP->u2AppInfoLen = u2AppInfoLen;

		rStatus = WLAN_STATUS_SUCCESS;
	} else {
		DBGLOG(NAN, ERROR, "AppInfo allocation failure (%d)\n",
		       u2AppInfoLen);

		prNDP->u2AppInfoLen = 0;

		rStatus = WLAN_STATUS_RESOURCES;
	}

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDPE OTHER APP-INFO Buffer/Update Function
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineUpdateOtherAppInfo(
	IN struct ADAPTER *prAdapter, IN struct _NAN_NDP_INSTANCE_T *prNDP,
	IN struct _NAN_ATTR_NDPE_SVC_INFO_TLV_T *prAppInfoTLV) {
	uint16_t u2OtherAppInfoLen;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "prNDP error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prAppInfoTLV) {
		DBGLOG(NAN, ERROR, "prAppInfoTLV error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prAppInfoTLV->u2Length < VENDOR_OUI_LEN) {
		DBGLOG(NAN, ERROR, "prAppInfoTLV len invalid\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP->pucOtherAppInfo) {
		/* already exist ? free it first */
		cnmMemFree(prAdapter, prNDP->pucOtherAppInfo);
		prNDP->pucOtherAppInfo = NULL;
		prNDP->u2OtherAppInfoLen = 0;
	}

	kalMemCopy(prNDP->aucOtherAppInfoOui, prAppInfoTLV->aucOui,
		   VENDOR_OUI_LEN);
	u2OtherAppInfoLen = prAppInfoTLV->u2Length - VENDOR_OUI_LEN;

	if (u2OtherAppInfoLen > 0) {
		prNDP->pucOtherAppInfo = cnmMemAlloc(
			prAdapter, RAM_TYPE_BUF, (uint32_t)u2OtherAppInfoLen);
		if (prNDP->pucOtherAppInfo) {
			kalMemCopy(prNDP->pucOtherAppInfo,
				   prAppInfoTLV->aucBody, u2OtherAppInfoLen);
			prNDP->u2OtherAppInfoLen = u2OtherAppInfoLen;
		} else {
			DBGLOG(NAN, WARN,
			       "OtherAppInfo allocation failure (%d)\n",
			       u2OtherAppInfoLen);

			prNDP->u2OtherAppInfoLen = 0;

			return WLAN_STATUS_RESOURCES;
		}
	} else
		prNDP->u2OtherAppInfoLen = 0;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAN_ATTR
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineGetECAttr(struct ADAPTER *prAdapter, uint8_t **ppucECAttr,
		       uint16_t *pu2ECAttrLength) {
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *)NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!ppucECAttr) {
		DBGLOG(NAN, ERROR, "ppucECAttr error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pu2ECAttrLength) {
		DBGLOG(NAN, ERROR, "pu2ECAttrLength error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = prAdapter->aprBssInfo[nanGetSpecificBssInfo(
		prAdapter, NAN_BSS_INDEX_5G_BAND)->ucBssIndex];

	return nanDataEngineGetECAttrImpl(prAdapter, ppucECAttr,
					  pu2ECAttrLength, prBssInfo, NULL);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NAN_ATTR
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineGetECAttrImpl(struct ADAPTER *prAdapter,
		uint8_t **ppucECAttr, uint16_t *pu2ECAttrLength,
		struct BSS_INFO *prBssInfo,
		struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct _NAN_ATTR_ELEMENT_CONTAINER_T *prAttrEC;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint16_t u2AttrLength;
#if (CFG_SUPPORT_NAN_R4 == 1) && (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeCapLen, ucHeOpLen;
#endif

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!ppucECAttr) {
		DBGLOG(NAN, ERROR, "ppucECAttr error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pu2ECAttrLength) {
		DBGLOG(NAN, ERROR, "pu2ECAttrLength error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrLength =
		OFFSET_OF(struct _NAN_ATTR_ELEMENT_CONTAINER_T, aucElements);
	u2AttrLength += (ELEM_HDR_LEN + ELEM_MAX_LEN_HT_CAP);
#if CFG_SUPPORT_802_11AC
	u2AttrLength += ELEM_HDR_LEN + ELEM_MAX_LEN_VHT_CAP;
#endif

	prAttrEC = (struct _NAN_ATTR_ELEMENT_CONTAINER_T *)
			   prAdapter->rDataPathInfo.aucECAttr;

	prAttrEC->ucAttrId = NAN_ATTR_ID_ELEMENT_CONTAINER;
	prAttrEC->ucMapID = 0;

	/* HT-CAP IE - leverage RLM */
	rlmFillNANHTCapIE(prAdapter, prBssInfo,
			  &(prAttrEC->aucElements[0]));

	prAdapter->rDataPathInfo.prLocalHtCap =
		(struct IE_HT_CAP *)&(prAttrEC->aucElements[0]);

#if CFG_SUPPORT_802_11AC
	if (prWifiVar->fgEnNanVHT == TRUE) {
		DBGLOG(NAN, INFO, "Bring VHT IE\n");
		/* VHT-CAP IE -  leverage RLM */
		rlmFillNANVHTCapIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
						ELEM_MAX_LEN_HT_CAP]));
		prAdapter->rDataPathInfo
			.prLocalVhtCap = (struct IE_VHT_CAP *)&(
			prAttrEC->aucElements[ELEM_HDR_LEN +
					      ELEM_MAX_LEN_HT_CAP]);
	}
#endif

#if (CFG_SUPPORT_NAN_R4 == 1) && (CFG_SUPPORT_802_11AX == 1)
	if (prWifiVar->fgEnNanHE == TRUE) {
		DBGLOG(NAN, INFO, "Bring HE IE\n");
		ucHeCapLen = heRlmFillNANHECapIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
				ELEM_MAX_LEN_HT_CAP +
				ELEM_HDR_LEN +
				ELEM_MAX_LEN_VHT_CAP]));
			prAdapter->rDataPathInfo.prLocalHeCap =
				(struct _IE_HE_CAP_T *)&(prAttrEC->aucElements
				[ELEM_HDR_LEN +
				ELEM_MAX_LEN_HT_CAP +
				ELEM_HDR_LEN +
				ELEM_MAX_LEN_VHT_CAP]);

		u2AttrLength += ucHeCapLen;
		DBGLOG(NAN, INFO, "Len HE Cap IE (%d)\n", ucHeCapLen);

		ucHeOpLen = heRlmFillNANHeOpIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
				ELEM_MAX_LEN_HT_CAP +
				ELEM_HDR_LEN +
				ELEM_MAX_LEN_VHT_CAP +
				ucHeCapLen]));

		u2AttrLength += ucHeOpLen;
		DBGLOG(NAN, INFO, "Len HE OP IE (%d)\n", ucHeOpLen);
	}
#endif

	prAttrEC->u2Length =
			u2AttrLength -
			OFFSET_OF(struct _NAN_ATTR_HDR_T,
			aucAttrBody);
	DBGLOG(NAN, INFO, "Len EC (%d)\n", prAttrEC->u2Length);

	*ppucECAttr = prAdapter->rDataPathInfo.aucECAttr;
	*pu2ECAttrLength = u2AttrLength;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL timer stopping
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdlDeactivateTimers(IN struct ADAPTER *prAdapter,
		       IN struct _NAN_NDL_INSTANCE_T *prNDL) {

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* stop all timers for handsahking */
	cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));
	cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolRetryTimer));
	cnmTimerStopTimer(prAdapter, &(prNDL->rNDPSecurityExpireTimer));

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL Request Management
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineInsertRequest(IN struct ADAPTER *prAdapter,
			   IN struct _NAN_NDL_INSTANCE_T *prNDL,
			   IN enum _NAN_DATA_ENGINE_REQUEST_TYPE_T eRequestType,
			   IN enum _ENUM_NAN_PROTOCOL_ROLE_T eNDLRole,
			   IN struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prReq;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	prReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			    sizeof(struct _NAN_DATA_ENGINE_REQUEST_T));

	if (prReq == NULL)
		return WLAN_STATUS_RESOURCES;

	prReq->eRequestType = eRequestType;
	prReq->prNDP = prNDP;
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
	prReq->eRequestType = eNDLRole;
#endif
	LINK_INSERT_TAIL(&(prNDL->rPendingReqList), &(prReq->rLinkEntry));

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL Request Management
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanDataEngineFlushRequest(IN struct ADAPTER *prAdapter,
			  IN struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prReq;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	while ((prReq = nanDataEngineGetNextRequest(prAdapter, prNDL)) != NULL)
		cnmMemFree(prAdapter, prReq);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL Request Management
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
struct _NAN_DATA_ENGINE_REQUEST_T *
nanDataEngineGetNextRequest(IN struct ADAPTER *prAdapter,
			    IN struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prPendingReq;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return NULL;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return NULL;
	}

	LINK_REMOVE_HEAD(&(prNDL->rPendingReqList), prPendingReq,
			 struct _NAN_DATA_ENGINE_REQUEST_T *);

	return prPendingReq;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            NDL Request Management
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanDataEngineRemovePendingRequests(
	IN struct ADAPTER *prAdapter, IN struct _NAN_NDL_INSTANCE_T *prNDL,
	IN enum _NAN_DATA_ENGINE_REQUEST_TYPE_T eRequestType,
	IN struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prPendingReq, *prPendingReqNext;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "prNDL error\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	/* traverse through pending request list */
	LINK_FOR_EACH_ENTRY_SAFE(prPendingReq, prPendingReqNext,
				 &(prNDL->rPendingReqList), rLinkEntry,
				 struct _NAN_DATA_ENGINE_REQUEST_T) {
		/* check for specified type */
		if (prPendingReq->eRequestType == eRequestType) {
			LINK_REMOVE_KNOWN_ENTRY(&(prNDL->rPendingReqList),
						&(prPendingReq->rLinkEntry));

			cnmMemFree(prAdapter, prPendingReq);
		}
	}

	return WLAN_STATUS_SUCCESS;
}

void
nanDataEngineDisconnectByStaIdx(IN struct ADAPTER *prAdapter,
		uint8_t ucStaIdx) {
	struct STA_RECORD *prStaRec = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint8_t i = 0;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaIdx);
	if (prStaRec == NULL) {
		DBGLOG(NAN, INFO, "Station Record Not Found\n");
		return;
	}
	prNDL = nanDataUtilSearchNdlByStaRec(prAdapter, prStaRec);
	if (prNDL != NULL) {
		for (i = 0; i < NAN_MAX_SUPPORT_NDP_NUM; i++) {
			if (prNDL->arNDP[i].fgNDPValid == TRUE)
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					&prNDL->arNDP[i]);
		}
	} else {
		DBGLOG(NAN, INFO, "Not found the NDL\n");
	}
}

void
nanDataEngingDisconnectEvt(IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf) {
	struct _NAN_SCHED_EVENT_NDL_DISCONN_T *prNDLDisconn;

	prNDLDisconn = (struct _NAN_SCHED_EVENT_NDL_DISCONN_T *)pcuEvtBuf;
	DBGLOG(NAN, INFO, "NDL Timeout, Sta:%d\n",
	       prNDLDisconn->ucStaIdx);
	nanDataEngineDisconnectByStaIdx(prAdapter, prNDLDisconn->ucStaIdx);
}
#if (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief Get _NAN_NDP_INSTANCE_T by Multicast MAC Address
 *
 * @param[in] pucNanMulticastAddr      Given Multicast MAC Address.
 * @param[in] pucLocalMacAddr          Given Local NDI MAC Address.
 *
 * @retval   Pointer to _NAN_NDP_INSTANCE_T, if found. NULL, if not found
 */
/*----------------------------------------------------------------------------*/
struct _NAN_NDP_INSTANCE_T *
nanFindNdpByGroupAddr(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucNanMulticastAddr,
	IN uint8_t *pucLocalMacAddr)
{
	size_t szNdlIndex = 0;
	size_t szNdpIdx = 0;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDP_INSTANCE_T *prTargetNDP = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter is null\n");
		return NULL;
	}
	if ((pucNanMulticastAddr == NULL) || (pucLocalMacAddr == NULL)) {
		DBGLOG(NAN, ERROR, "mac address is null\n");
		return NULL;
	}

	for (szNdlIndex = 0;
		szNdlIndex < NAN_MAX_SUPPORT_NDL_NUM; szNdlIndex++) {

		if (prAdapter->rDataPathInfo.arNDL[szNdlIndex].fgNDLValid ==
			FALSE) {
			continue;
		}
		prNDL = &(prAdapter->rDataPathInfo.arNDL[szNdlIndex]);
		for (szNdpIdx = 0; szNdpIdx < NAN_MAX_SUPPORT_NDP_NUM;
			szNdpIdx++) {

			prNDP = &prNDL->arNDP[szNdpIdx];
			if (prNDP->fgNDPValid == FALSE)
				continue;

			if (EQUAL_MAC_ADDR(prNDP->aucLocalNDIAddr,
				pucLocalMacAddr)) {
				if (prTargetNDP == NULL)
					prTargetNDP = prNDP;

				if (EQUAL_MAC_ADDR(
					prTargetNDP->aucNanMulticastAddr,
					pucNanMulticastAddr))
					return prNDP;
			}
		}
	}

	return prTargetNDP;
}

struct _NAN_NDC_CTRL_T *
nanFindNdcForNdp(
		IN struct ADAPTER *prAdapter,
		IN struct _NAN_NDP_INSTANCE_T *prNDP)
{
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

	if ((prAdapter == NULL) || (prNDP == NULL)) {
		DBGLOG(NAN, ERROR,
			"prAdapter or prNDP is null\n");
		return NULL;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR,
			"prNDL is null\n");
		return NULL;
	}

	return prNDL->prNdcCtrl;
}

struct _NAN_MC_TX_LOOKUP *
nanLookUpMcTxTbl(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucNanMulticastAddr,
	IN uint8_t *pucLocalNdiAddr
)
{
	size_t szMcTxTblIdx = 0;
	struct _NAN_MC_TX_LOOKUP *prMcTxLookUpTbl = NULL;
	struct _NAN_MC_TX_LOOKUP *prTargetMcTxLookUpTbl = NULL;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter is null\n");
		return NULL;
	}
	if ((pucNanMulticastAddr == NULL) || (pucLocalNdiAddr == NULL)) {
		DBGLOG(NAN, ERROR, "mac address is null\n");
		return NULL;
	}

	for (szMcTxTblIdx = 0; szMcTxTblIdx < NAN_MC_TX_LOOKUP_TBL_NUM;
		szMcTxTblIdx++) {

		prMcTxLookUpTbl = &g_arNanMcTxLookupTbl[szMcTxTblIdx];

		if (prMcTxLookUpTbl->prNdcMgmt == NULL)
			continue;

		if (EQUAL_MAC_ADDR(prMcTxLookUpTbl->aucLocalNdiAddr,
			pucLocalNdiAddr)) {
			if (prTargetMcTxLookUpTbl == NULL)
				prTargetMcTxLookUpTbl = prMcTxLookUpTbl;

			if (EQUAL_MAC_ADDR(prMcTxLookUpTbl->aucMulticastAddr,
				pucNanMulticastAddr))
				return prMcTxLookUpTbl;
		}
	}

	return prTargetMcTxLookUpTbl;
}

struct _NAN_MC_TX_LOOKUP *
nanAquireMcTxLookUpEntry(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucNanMulticastAddr,
	IN uint8_t *pucLocalNdiAddr,
	IN struct _NAN_NDP_INSTANCE_T *prNDP)
{
	size_t szMcTxTblIdx = 0;
	struct _NAN_MC_TX_LOOKUP *prMcTxLookUpTbl = NULL;
	struct _NAN_MC_TX_LOOKUP *prRetMcTxLookUpTbl = NULL;

	if (!pucLocalNdiAddr) {
		DBGLOG(NAN, ERROR,
			"[MCDATA] MC Tx lookup entry acquire fail, lack NDI\n");
		return NULL;
	}

	for (szMcTxTblIdx = 0; szMcTxTblIdx < NAN_MC_TX_LOOKUP_TBL_NUM;
		szMcTxTblIdx++) {

		prMcTxLookUpTbl = &g_arNanMcTxLookupTbl[szMcTxTblIdx];

		if (prMcTxLookUpTbl->prNdcMgmt == NULL) {
			prRetMcTxLookUpTbl = prMcTxLookUpTbl;
		} else {
			if (!EQUAL_MAC_ADDR(prMcTxLookUpTbl->aucLocalNdiAddr,
				pucLocalNdiAddr))
				continue;

			if (pucNanMulticastAddr &&
				(!EQUAL_MAC_ADDR(
					prMcTxLookUpTbl->aucMulticastAddr,
					pucNanMulticastAddr)))
				continue;

			if (prNDP && (prMcTxLookUpTbl->prNDP != prNDP))
				continue;

			prRetMcTxLookUpTbl = prMcTxLookUpTbl;
			break;
		}
	}

	if (!prRetMcTxLookUpTbl)
		DBGLOG(NAN, ERROR,
			"[MCDATA] MC Tx lookup entry acquire fail\n");
	return prRetMcTxLookUpTbl;
}

void
nanReleaseMcTxLookUpEntry(
	IN struct ADAPTER *prAdapter,
	IN uint8_t *pucNanMulticastAddr,
	IN uint8_t *pucLocalNdiAddr,
	IN struct _NAN_NDP_INSTANCE_T *prNDP
)
{
	size_t szMcTxTblIdx = 0;
	struct _NAN_MC_TX_LOOKUP *prMcTxLookUpTbl = NULL;

	if (!pucLocalNdiAddr) {
		DBGLOG(NAN, ERROR,
			"[MCDATA] MC Tx lookup entry release fail, lack NDI\n");
		return;
	}

	for (szMcTxTblIdx = 0; szMcTxTblIdx < NAN_MC_TX_LOOKUP_TBL_NUM;
		szMcTxTblIdx++) {

		prMcTxLookUpTbl = &g_arNanMcTxLookupTbl[szMcTxTblIdx];

		if (prMcTxLookUpTbl->prNdcMgmt == NULL) {
			continue;
		} else {
			if (!EQUAL_MAC_ADDR(
				prMcTxLookUpTbl->aucLocalNdiAddr,
				pucLocalNdiAddr))
				continue;

			if (pucNanMulticastAddr &&
				(!EQUAL_MAC_ADDR(
					prMcTxLookUpTbl->aucMulticastAddr,
					pucNanMulticastAddr)))
				continue;

			if (prNDP && (prMcTxLookUpTbl->prNDP != prNDP))
				continue;

			kalMemZero(prMcTxLookUpTbl->aucMulticastAddr,
				MAC_ADDR_LEN);
			kalMemZero(prMcTxLookUpTbl->aucLocalNdiAddr,
				MAC_ADDR_LEN);
			prMcTxLookUpTbl->prNdcMgmt = NULL;
			prMcTxLookUpTbl->ucLocalNdiIdx = 0;
		}
	}
}

uint32_t
nanInitMcTxLookUpEntry(
	IN struct ADAPTER *prAdapter,
	IN struct _NAN_MC_TX_LOOKUP *prMcTxLookUpTbl,
	IN struct _NAN_NDC_MGMT_T *prNdcMgmt,
	IN uint8_t *pucNanMulticastAddr,
	IN uint8_t *pucLocalNdiAddr,
	IN struct _NAN_NDP_INSTANCE_T *prNDP
)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prMcTxLookUpTbl || !prNdcMgmt || !pucLocalNdiAddr) {
		DBGLOG(NAN, ERROR, "init MC Tx lookup fail\n");
		return rStatus;
	}

	if (prMcTxLookUpTbl->prNdcMgmt == NULL) {
		prMcTxLookUpTbl->prNdcMgmt = prNdcMgmt;

		kalMemCpyS(prMcTxLookUpTbl->aucLocalNdiAddr,
			MAC_ADDR_LEN,
			pucLocalNdiAddr, MAC_ADDR_LEN);

		if (pucNanMulticastAddr)
			kalMemCpyS(prMcTxLookUpTbl->aucMulticastAddr,
				MAC_ADDR_LEN,
				pucNanMulticastAddr, MAC_ADDR_LEN);
		else
			kalMemZero(prMcTxLookUpTbl->aucMulticastAddr,
				sizeof(prMcTxLookUpTbl->aucMulticastAddr));

		prMcTxLookUpTbl->prNDP = prNDP;
		prMcTxLookUpTbl->ucLocalNdiIdx =
			nanDataEngineGetNdiIdxByAddr(prAdapter,
				pucLocalNdiAddr);
	} else
		DBGLOG(NAN, INFO, "MC Tx lookup is already ready\n");

	rStatus = WLAN_STATUS_SUCCESS;
	return rStatus;
}

#endif /* (CFG_NAN_SUPPORT_R4_GROUP_ADDR_DATA == 1) */

uint8_t *
nanDataGetLocalNDIAddr(IN struct ADAPTER *prAdapter,
			 uint8_t ucNdiIndex)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct _GL_NAN_INFO_T *prNANDevInfo = NULL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "Enter\n");
#endif

	prGlueInfo = prAdapter->prGlueInfo;
	prNANDevInfo = prGlueInfo->aprNANDevInfo[ucNdiIndex];

	if (prNANDevInfo != NULL) {
		DBGLOG(NAN, INFO, "NDI#%u, addr(" MACSTR ")\n",
			ucNdiIndex,
			MAC2STR(prNANDevInfo->prDevHandler->dev_addr));
		return (uint8_t *)prNANDevInfo->prDevHandler->dev_addr;
	}

	DBGLOG(NAN, INFO, "NDI#%u is not enabled)\n", ucNdiIndex);
	return NULL;

}

uint8_t
nanDataEngineGetNdiIdxByAddr(struct ADAPTER *prAdapter,
			uint8_t *pucMacAddr)
{
	uint8_t i = 0;
	uint8_t *pucNdiAddr = NULL;
	uint8_t ucNdiIdx = NAN_MAX_MULTI_NDI_NUM;

	DBGLOG(NAN, INFO, "addr(" MACSTR ")\n", MAC2STR(pucMacAddr));

	for (i = NAN_NDI_INDEX_0; i < NAN_MAX_MULTI_NDI_NUM; i++) {
		pucNdiAddr = nanDataGetLocalNDIAddr(prAdapter, i);

		if (pucNdiAddr == NULL)
			continue;

		if (kalMemCmp(pucMacAddr,
			pucNdiAddr,
			MAC_ADDR_LEN) == 0) {
			ucNdiIdx = i;
			break;
		}
	}

	return ucNdiIdx;
}

u_int8_t
nanDataCheckNdpExist(struct ADAPTER *prAdapter)
{
	uint8_t ucNdlIndex = 0;
	uint8_t ucNdpIndex = 0;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;

	for (; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		if (prAdapter->rDataPathInfo.arNDL[ucNdlIndex].fgNDLValid ==
		    FALSE)
			continue;
		prNDL = &(prAdapter->rDataPathInfo.arNDL[ucNdlIndex]);
		for (; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
			ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == FALSE)
				continue;
			prNDP = &prNDL->arNDP[ucNdpIndex];
			if (prNDP->fgNDPEstablish == TRUE)
				return TRUE;
		}
	}
	return FALSE;
}

u_int8_t
nanCheckScanAbort(struct ADAPTER *prAdapter)
{
	if (!prAdapter->fgNanEnable)
		return FALSE;

	if ((aisGetCurrState(prAdapter, AIS_DEFAULT_INDEX) ==
		AIS_STATE_NORMAL_TR) &&
		(nanDataCheckNdpExist(prAdapter) == TRUE) &&
		(prAdapter->rWifiVar.fgNanOnAbortScan == TRUE))
		return TRUE;

	return FALSE;
}
