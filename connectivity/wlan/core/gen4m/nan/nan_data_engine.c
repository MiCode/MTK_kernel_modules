// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if (CFG_SUPPORT_NAN == 1)

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
#include "wpa_supp/src/crypto/sha256_i.h"
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
#include "nanRescheduler.h"
#endif

/* #if (CFG_NAN_DATAENGINE == 1) */

/*******************************************************************************
 *                   E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
uint8_t g_ucNdpId;

static struct _APPEND_ATTR_ENTRY_T txDataAttributeTable[] = {
	/*   ATTR-ID       fp for calc-var-len     fp for attr appending */
	{ NAN_ATTR_ID_NDP, nanDataEngineNDPAttrLength,
	  nanDataEngineNDPAttrAppend },
	{ NAN_ATTR_ID_NDP_EXTENSION, nanDataEngineNDPEAttrLength,
	  nanDataEngineNDPEAttrAppend },
	{ NAN_ATTR_ID_DEVICE_CAPABILITY, nanDataEngineDevCapAttrLength,
	  nanDataEngineDevCapAttrAppend },
#if (CFG_SUPPORT_NAN_6G == 1)
	{ NAN_ATTR_ID_DEVICE_CAPABILITY_EXT, nanDataEngineDevCapExtAttrLength,
	  nanDataEngineDevCapExtAttrAppend },
#endif
	{ NAN_ATTR_ID_NAN_AVAILABILITY, nanDataEngineNanAvailAttrLength,
	  nanDataEngineNanAvailAttrAppend },
	{ NAN_ATTR_ID_NDC, nanDataEngineNdcAttrLength,
	  nanDataEngineNdcAttrAppend },
	{ NAN_ATTR_ID_NDL, nanDataEngineNDLAttrLength,
	  nanDataEngineNDLAttrAppend },
	{ NAN_ATTR_ID_NDL_QOS, nanDataEngineNdlQosAttrLength,
	  nanDataEngineNdlQosAttrAppend },
	{ NAN_ATTR_ID_ELEMENT_CONTAINER, nanDataEngineElemContainerAttrLength,
	  nanDataEngineElemContainerAttrAppend },
	{ NAN_ATTR_ID_UNALIGNED_SCHEDULE, nanDataEngineUnalignedAttrLength,
	  nanDataEngineUnalignedAttrAppend },
	{ NAN_ATTR_ID_CIPHER_SUITE_INFO, nanDataEngineCipherSuiteAttrLength,
	  nanDataEngineCipherSuiteAttrAppend },
	{ NAN_ATTR_ID_SECURITY_CONTEXT_INFO, nanDataEngineSecContextAttrLength,
	  nanDataEngineSecContextAttrAppend },
	{ NAN_ATTR_ID_SHARED_KEY_DESCRIPTOR, nanDataEngineSharedKeyAttrLength,
	  nanDataEngineSharedKeyAttrAppend },
	{ NAN_ATTR_ID_VENDOR_SPECIFIC, nanDataEngineVendorAttrLength,
	  nanDataEngineVendorAttrAppend },
#if (CFG_SUPPORT_NAN_11BE == 1)
	{ NAN_ATTR_ID_VENDOR_SPECIFIC, nanDataEngineVendorEhtAttrLength,
	  nanDataEngineVendorEhtAttrAppend },
#endif
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

static const char * const apucDebugDataMgmtState[NDL_MGMT_STATE_NUM] = {
	"NDL_IDLE",
	"NDL_REQUEST_SCHEDULE_NDP",
	"NDL_REQUEST_SCHEDULE_NDL",
	"NDL_SCHEDULE_SETUP",
	"NDL_INITIATOR_TX_SCHEDULE_REQUEST",
	"NDL_RESPONDER_RX_SCHEDULE_REQUEST",
	"NDL_RESPONDER_TX_SCHEDULE_RESPONSE",
	"NDL_INITIATOR_RX_SCHEDULE_RESPONSE",
	"NDL_INITIATOR_TX_SCHEDULE_CONFIRM",
	"NDL_RESPONDER_RX_SCHEDULE_CONFIRM",
	"NDL_SCHEDULE_ESTABLISHED",
	"NDL_TEARDOWN_BY_NDP_TERMINATION",
	"NDL_TEARDOWN",
	"NDL_INITIATOR_RX_WAITFOR_SCHEDULE_RESPONSE",
};

static const char * const
		apucDebugDataPathProtocolState[NDP_PROTOCOL_STATE_NUM] = {
	"NDP_IDLE",
	"NDP_INITIATOR_TX_DP_REQUEST",
	"NDP_INITIATOR_RX_DP_RESPONSE",
	"NDP_INITIATOR_TX_DP_CONFIRM",
	"NDP_INITIATOR_RX_DP_SECURITY_INSTALL",
	"NDP_RESPONDER_WAIT_DATA_RSP",
	"NDP_RESPONDER_TX_DP_RESPONSE",
	"NDP_RESPONDER_RX_DP_CONFIRM",
	"NDP_RESPONDER_TX_DP_SECURITY_INSTALL",
	"NDP_NORMAL_TR",
	"NDP_TX_DP_TERMINATION",
	"NDP_DISCONNECT",
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
	PUINT_8 pucPeerNDIAddr
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
	struct ADAPTER *prAdapter, struct _NAN_NDL_INSTANCE_T *prNDL,
	enum _ENUM_NAN_PROTOCOL_ROLE_T eNDPRole, uint8_t *pucPeerNDIAddr,
	uint8_t ucNDPID, unsigned char fgSecurityRequired);

static void nanDataFreeNdp(struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP);

static struct _NAN_NDL_INSTANCE_T *
nanDataAllocateNdl(struct ADAPTER *prAdapter, uint8_t *pucMacAddr,
		   enum _ENUM_NAN_PROTOCOL_ROLE_T eRole);

static struct _NAN_NDL_INSTANCE_T *
nanDataUtilGetNdl(struct ADAPTER *prAdapter,
		  struct _NAN_NDP_INSTANCE_T *prNDP);

static void nanDataFreeNdl(struct ADAPTER *prAdapter,
			   struct _NAN_NDL_INSTANCE_T *prNDL);

/* callback function for NAN scheduler */
static void nanDataPathScheduleNegoGranted(struct ADAPTER *prAdapter,
					   uint8_t *pu1DevAddr,
					   enum _ENUM_NAN_NEGO_TYPE_T eType,
					   enum _ENUM_NAN_NEGO_ROLE_T eRole,
					   void *pvToken);

/* static machine operations */
static void nanNdlMgmtFsmStep(struct ADAPTER *prAdapter,
			      enum _ENUM_NDL_MGMT_STATE_T eNextState,
			      struct _NAN_NDL_INSTANCE_T *prNDL);

static enum _ENUM_NAN_NDP_STATUS_T
nanDataPathProtocolFsmStep(struct ADAPTER *prAdapter,
			   enum _ENUM_NDP_PROTOCOL_STATE_T eNextState,
			   struct _NAN_NDP_INSTANCE_T *prNDP);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void
nanDataResponseTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam) {
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	prNDP = (struct _NAN_NDP_INSTANCE_T *)ulParam;

	if (prNDP != NULL && prNDP->fgNDPValid == TRUE) {
		DBGLOG(NAN, ERROR,
			"NdpId(%d)\n", prNDP->ucNDPID);

		nanNdpResponderRspEvent(prAdapter,
			prNDP,
			WLAN_STATUS_FAILURE);

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
nanDataProtocolTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	prNDL = (struct _NAN_NDL_INSTANCE_T *)ulParam;

	if (prNDL != NULL) {
		/* If peer ignore our schedule request,
		 * recover to nego slot and state before reschedule
		 */
		if (prNDL->eCurrentNDLMgmtState ==
		    NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE ||
		    prNDL->eCurrentNDLMgmtState ==
		    NDL_RESPONDER_RX_SCHEDULE_CONFIRM) {
			DBGLOG(NAN, WARN,
			       "Prevent NDL teardown for availability!\n");
			nanSchedNegoUpdateNegoResult(prAdapter);

			nanNdlMgmtFsmStep(prAdapter,
					  NDL_SCHEDULE_ESTABLISHED,
					  prNDL);

			return;
		}

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
nanDataRetryTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	prNDL = (struct _NAN_NDL_INSTANCE_T *)ulParam;

	if (prNDL != NULL && prNDL->prOperatingNDP != NULL) {
		prNDP = prNDL->prOperatingNDP;

		prNDP->eDataPathFailReason = DP_REASON_RX_TIMEOUT;

		if (prNDP->eCurrentNDPProtocolState ==
			NDP_RESPONDER_TX_DP_RESPONSE)
			nanNdpResponderRspEvent(prAdapter, prNDP,
				WLAN_STATUS_FAILURE);

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	} else if (prNDL != NULL &&
		(prNDL->eCurrentNDLMgmtState ==
		NDL_INITIATOR_TX_SCHEDULE_REQUEST)) {
		DBGLOG(NAN, WARN,
			"Prevent NDL teardown for availability!\n");
		nanSchedNegoUpdateNegoResult(prAdapter);
		nanNdlMgmtFsmStep(prAdapter,
				  NDL_SCHEDULE_ESTABLISHED,
				  prNDL);
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
nanDataSecurityTimeout(struct ADAPTER *prAdapter, uintptr_t ulParam) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
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
			if (nanSchedNegoGenLocalCrbProposal(prAdapter)
				!= WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, WARN, "[%s] Reject by scheduler\n",
				       __func__);
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
				prNDL->eCurrentNDLMgmtState = NDL_TEARDOWN;
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
#else
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
#endif
			}
			/* send Data Path Request */
			nanNdlMgmtFsmStep(prAdapter,
					  NDL_INITIATOR_TX_SCHEDULE_REQUEST,
					  prNDL);
		}
	}
}

static void nanDataEngineDisconnectByNdl(struct ADAPTER *prAdapter,
					 struct _NAN_NDL_INSTANCE_T *prNDL,
					 u_int8_t fgDisconnectByKeepAlive)
{
	uint8_t i = 0;

	if (prNDL == NULL) {
		DBGLOG(NAN, DEBUG, "Not found the NDL\n");
		return;
	}

	for (i = 0; i < NAN_MAX_SUPPORT_NDP_NUM; i++) {
		if (!prNDL->arNDP[i].fgNDPValid)
			continue;

		if (prNDL->arNDP[i].eCurrentNDPProtocolState == NDP_NORMAL_TR &&
		    fgDisconnectByKeepAlive == FALSE) {
			nanDataPathProtocolFsmStep(prAdapter,
						   NDP_DISCONNECT,
						   &prNDL->arNDP[i]);
		} else {
			nanDataPathProtocolFsmStep(prAdapter,
						   NDP_TX_DP_TERMINATION,
						   &prNDL->arNDP[i]);
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
nanDataUtilSearchEmptyNdlEntry(struct ADAPTER *prAdapter)
{
	uint8_t ucNdlIndex;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDL_INSTANCE_T *prReuseNDL = NULL;
	uint8_t ucNumNDL = kal_min_t(uint8_t, NAN_MAX_SUPPORT_NDL_NUM,
				     prAdapter->rWifiVar.ucNanMaxNdpSession);

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif



	for (ucNdlIndex = 0; ucNdlIndex < ucNumNDL; ucNdlIndex++) {
		prNDL = &prAdapter->rDataPathInfo.arNDL[ucNdlIndex];

		if (prNDL->fgNDLValid)
			continue;

		DBGLOG(NAN, DEBUG, "found empty ucNdlIndex:%d\n", ucNdlIndex);
		prNDL->ucIndex = ucNdlIndex;
		break;
	}

	if (prNDL == NULL) {
		prReuseNDL = nanExtGetReusedNdl(prAdapter);
		if (prReuseNDL)
			nanDataEngineDisconnectByNdl(prAdapter, prReuseNDL,
						     FALSE);
		return prReuseNDL;
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error, return NULL\n", __func__);
		return NULL;
	}

	for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
	     ucNdpIndex++) {
		if (prNDL->arNDP[ucNdpIndex].fgNDPValid == FALSE) {
			prNDP = &prNDL->arNDP[ucNdpIndex];
			DBGLOG(NAN, DEBUG,
			       "[%s] ucNdpIndex:%d, prInitiatorSecSmInfo:0x%p\n",
			       __func__, ucNdpIndex,
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
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

static void
nanDataGenerateNdpInstanceId(
	struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL,
	struct _NAN_NDP_INSTANCE_T *prNDP)
{
	if (!prNDL || !prNDP)
		return;

	prNDP->ndp_instance_id =
		(prNDP->ucNDPID * 100) +
		(prNDP->ucNdlIndex) * NAN_MAX_SUPPORT_NDP_NUM +
		(prNDP - prNDL->arNDP);
}

struct _NAN_NDP_INSTANCE_T *
nanDataUtilSearchNdpByNdpInstanceId(
	struct ADAPTER *prAdapter,
	uint32_t u4NdpInstanceId)
{
	uint8_t ucNdlIndex, ucNdpIndex;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	uint32_t u4Id = u4NdpInstanceId % 100;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
		return NULL;
	}

	ucNdlIndex = u4Id / NAN_MAX_SUPPORT_NDP_NUM;
	ucNdpIndex = u4Id % NAN_MAX_SUPPORT_NDP_NUM;
	if (ucNdlIndex >= NAN_MAX_SUPPORT_NDL_NUM ||
		ucNdpIndex >= NAN_MAX_SUPPORT_NDP_NUM) {
		DBGLOG(NAN, ERROR,
			"[%s] Invalid NDP instance ID = %d\n",
			__func__, u4NdpInstanceId);
		return NULL;
	}

	DBGLOG(NAN, DEBUG, "NdpId %u [%d,%d]\n",
		u4NdpInstanceId,
		ucNdlIndex,
		ucNdpIndex);

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	prNDL = &(prDataPathInfo->arNDL[ucNdlIndex]);
	if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
		prNDL->arNDP[ucNdpIndex].ndp_instance_id == u4NdpInstanceId)
		return &(prNDL->arNDP[ucNdpIndex]);

	DBGLOG(NAN, ERROR,
		"[%s] Invalid NDP (fg = %d, id = %d), instance ID = %d\n",
		__func__,
		prNDL->arNDP[ucNdpIndex].fgNDPValid,
		prNDL->arNDP[ucNdpIndex].ndp_instance_id,
		u4NdpInstanceId);

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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
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
	PUINT_8 pucPeerNDIAddr
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
 * \brief Find an NDL in prAdapter->rDataPathInfo by matching MAC address
 *        pucAddr
 *
 * \param[in]
 *
 * \return return pointer to matched NDL if
 *	a) prNDL->aucPeerMacAddr equals pucAddr, or
 *	b) any valid NDP prNDL->arNDP[u4Idx].aucPeerNDIAddr equals pucAddr
 */
/*----------------------------------------------------------------------------*/
struct _NAN_NDL_INSTANCE_T *
nanDataUtilSearchNdlByMac(struct ADAPTER *prAdapter, uint8_t *pucAddr)
{
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint32_t u4Idx;
	uint8_t *pucMacAddr;

	uint8_t ucNdlIndex; /* ucNdpIndex */
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Search %02x:%02x:%02x:%02x:%02x:%02x\n",
	       __func__, pucAddr[0], pucAddr[1], pucAddr[2], pucAddr[3],
	       pucAddr[4], pucAddr[5]);
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (ucNdlIndex = 0; ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
	     ucNdlIndex++) {
		if (!prDataPathInfo->arNDL[ucNdlIndex].fgNDLValid)
			continue;

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

		for (u4Idx = 0; u4Idx < NAN_MAX_SUPPORT_NDP_NUM; u4Idx++) {
			if (!prNDL->arNDP[u4Idx].fgNDPValid)
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
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

			if (nanGetStaRecExist(
				prNdpCxt,
				prStaRec))
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
	uint8_t ucNdpId = 0;
	uint8_t ucNdpIdTmp;
#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return 0;
	}

	if (prNDL->fgNDLValid != TRUE) {
		DBGLOG(NAN, ERROR, "[%s] fgNDLValid not valid\n", __func__);
		return 0;
	}

	if (prAdapter->rWifiVar.fgEnableRandNdpid == FALSE)
		ucNdpId = prNDL->ucSeqNum;

	/* ID conflict check */
	do {
		fgConflict = FALSE;

		if (prAdapter->rWifiVar.fgEnableRandNdpid == FALSE) {
			/* incremental */
			if (ucNdpId < 255)
				ucNdpId++;
			else
				ucNdpId = 1;
				/* keep 0 as reserved purposes ... */
		} else {
			/* incremental */
			if (g_ucNdpId < 255)
				g_ucNdpId++;
			else
				g_ucNdpId = (uint8_t)kalRandomNumber();
		}

		if (prAdapter->rWifiVar.fgEnableRandNdpid == FALSE) {
			ucNdpIdTmp = ucNdpId;
		} else {
			ucNdpIdTmp = g_ucNdpId;
		}

		for (ucNdpIndex = 0; ucNdpIndex < NAN_MAX_SUPPORT_NDP_NUM;
		     ucNdpIndex++) {
			if (prNDL->arNDP[ucNdpIndex].fgNDPValid == TRUE &&
			    prNDL->arNDP[ucNdpIndex].ucNDPID == ucNdpIdTmp) {
				fgConflict = TRUE;
				break;
			}
		}

		if (fgConflict == FALSE) /* non-conflict */
			break;
	} while (fgConflict == TRUE);

	if (prAdapter->rWifiVar.fgEnableRandNdpid == FALSE) {
		/* update to local tracker */
		prNDL->ucSeqNum = ucNdpId;
		DBGLOG(NAN, DEBUG, "gen NdpId(%d)\n", ucNdpId);
		return ucNdpId;
	}

	DBGLOG(NAN, DEBUG, "gen g_NdpId(%d)\n", g_ucNdpId);
	return g_ucNdpId;
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
		   struct _NAN_NDL_INSTANCE_T *prNDL,
		   enum _ENUM_NAN_PROTOCOL_ROLE_T eNDPRole,
		   uint8_t *pucPeerNDIAddr, uint8_t ucNDPID,
		   unsigned char fgSecurityRequired) {
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
		return NULL;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error, return NULL\n", __func__);
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	/* NDP-ID duplication check */
	if (nanDataUtilSearchNdpByNdpId(prAdapter, prNDL, ucNDPID) != NULL)
		return NULL;

	prNDP = nanDataUtilSearchEmptyNdpEntry(prAdapter, prNDL);
	if (prNDP == NULL)
		return NULL;

	/* fill data in NDP */
	prNDP->eNDPRole = eNDPRole;
	if (eNDPRole == NAN_PROTOCOL_RESPONDER)
		prNDP->ucNDPID = ucNDPID;
	else if (eNDPRole == NAN_PROTOCOL_INITIATOR)
		prNDP->ucNDPID = nanDataUtilGenerateNdpId(prAdapter, prNDL);
	else {
		DBGLOG(NAN, ERROR, "[%s] eNDPRole invalid\n", __func__);
		return NULL;
	}

	COPY_MAC_ADDR(prNDP->aucPeerNDIAddr, pucPeerNDIAddr);
	prNDP->fgSecurityRequired = fgSecurityRequired;
	nanDataUpdateNdpLocalNDI(prAdapter, prNDP);
	prNDP->fgNDPActive = FALSE;
	prNDP->u2TransId = 0;
	prNDP->eDataPathFailReason = 0;
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

	cnmTimerInitTimer(prAdapter, &(prNDP->rNDPUserSpaceResponseTimer),
			  nanDataResponseTimeout,
			  (uintptr_t)prNDP);

	if (prNDL->ucNDPNum < UINT8_MAX)
		prNDL->ucNDPNum++;

	nanDataGenerateNdpInstanceId(prAdapter, prNDL, prNDP);

	DBGLOG(NAN, DEBUG, "Create NDP ID [%d,%d]\n",
		prNDP->ucNDPID,
		prNDP->ndp_instance_id);

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
nanDataUpdateNdpPeerNDI(struct ADAPTER *prAdapter,
			struct _NAN_NDP_INSTANCE_T *prNDP,
			uint8_t *pucPeerNDIAddr) {

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
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
nanDataUpdateNdpLocalNDI(struct ADAPTER *prAdapter,
			 struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct BSS_INFO *prBssInfo;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return;
	}

	prBssInfo = prAdapter->aprBssInfo[nanGetSpecificBssInfo(
						  prAdapter,
						  NAN_BSS_INDEX_BAND0)
						  ->ucBssIndex];

	if (UNEQUAL_MAC_ADDR(prNDP->aucLocalNDIAddr, prBssInfo->aucOwnMacAddr))
		COPY_MAC_ADDR(prNDP->aucLocalNDIAddr, prBssInfo->aucOwnMacAddr);
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);
	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return;
	}

	cnmTimerStopTimer(prAdapter,
		&(prNDP->rNDPUserSpaceResponseTimer));

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
nanDataAllocateNdl(struct ADAPTER *prAdapter, uint8_t *pucMacAddr,
		   enum _ENUM_NAN_PROTOCOL_ROLE_T eRole) {
	/* UINT_8 ucNdlIndex; */
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	uint8_t ucNdpCxtIdx;

	uint32_t u4SetTarPerfLevel;
	uint32_t u4SetBoostCpuTh;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	size_t i;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR,
			"[%s] prAdapter error, return NULL\n", __func__);
		return NULL;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	prNDL = nanDataUtilSearchEmptyNdlEntry(prAdapter);

	if (prNDL == NULL)
		return NULL;

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
#if (CFG_SUPPORT_802_11AX == 1)
	kalMemZero(&(prNDL->aucIeHeCap), sizeof(prNDL->aucIeHeCap));
#endif
#if (CFG_SUPPORT_NAN_11BE == 1)
	kalMemZero(&(prNDL->aucIeEhtCap), sizeof(prNDL->aucIeEhtCap));
#endif

	nanExtResetNdlConfig(prNDL);

	for (ucNdpCxtIdx = 0; ucNdpCxtIdx < NAN_MAX_SUPPORT_NDP_CXT_NUM;
	     ucNdpCxtIdx++)
		prNDL->arNdpCxt[ucNdpCxtIdx].fgValid = FALSE;

	prNDL->ucNDLSetupCurrentStatus = NAN_ATTR_NDL_STATUS_CONTINUED;
	prNDL->ucReasonCode = NAN_REASON_CODE_RESERVED;

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1 && CFG_SUPPORT_NAN_11BE == 1)
	prNDL->fgIsEhtReschedule = FALSE;
#endif

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	prNDL->fgTriggerReschedNewNDL = FALSE;
	prNDL->fgIs3rd6GNewNDL = FALSE;
#endif
	prNDL->eNdcParseAction = NDC_PARSE_NORMAL;

	/* timer initialization sequence */
	cnmTimerInitTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer),
			  nanDataProtocolTimeout,
			  (uintptr_t)prNDL);

	for (i = 0; i < NAN_PROTOCOL_ROLE_NUM; i++) {
		cnmTimerInitTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer[i],
				  nanDataRetryTimeout,
				  (uintptr_t)prNDL);
	}
	for (i = 0; i < NAN_MAX_SUPPORT_NDP_NUM; i++) {
		cnmTimerInitTimer(prAdapter, &prNDL->arNDPProtocolRetryTimer[i],
				  nanDataRetryTimeout,
				  (uintptr_t)prNDL);
	}

	cnmTimerInitTimer(prAdapter, &(prNDL->rNDPSecurityExpireTimer),
			  nanDataSecurityTimeout,
			  (uintptr_t)prNDL);

	LINK_INITIALIZE(&(prNDL->rPendingReqList));

	prDataPathInfo->ucNDLNum++;
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (prDataPathInfo->ucNDLNum == 2) {
		struct _NAN_SCHEDULER_T *prNanScheduler;

		prNanScheduler = nanGetScheduler(prAdapter);
		cnmTimerStopTimer(prAdapter,
				  &prNanScheduler->rResumeRescheduleTimer);
	}
#endif

	/* BoostCPU for QuickShare */
	u4SetTarPerfLevel = u4SetBoostCpuTh = prWifiVar->u4NanBoostLevel;
	prAdapter->rPerMonitor.u4NanBoostCpu = prWifiVar->u4NanBoostInit;
	if (prWifiVar->u4NanBoostInit & 1) { /* Odd number to fast boost */
		DBGLOG(SW4, DEBUG, "kalBoostCpu(%u, %u)\n",
		       u4SetTarPerfLevel, u4SetBoostCpuTh);
		kalBoostCpu(prAdapter, u4SetTarPerfLevel, u4SetBoostCpuTh);
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
static struct _NAN_NDL_INSTANCE_T *
nanDataUtilGetNdl(struct ADAPTER *prAdapter,
		  struct _NAN_NDP_INSTANCE_T *prNDP) {
#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error, return NULL\n", __func__);
		return NULL;
	}

	if (prNDP->ucNdlIndex >= NAN_MAX_SUPPORT_NDL_NUM) {
		DBGLOG(NAN, ERROR,
			"[%s] NdlIndex out of range, return NULL\n",
			__func__);
		return NULL;
	}

	return &(prAdapter->rDataPathInfo.arNDL[prNDP->ucNdlIndex]);
}

/* reverse of nanDataGenerateNdpInstanceId() */
#define NDP_IDX(_prNDP) \
	(_prNDP->ndp_instance_id - (_prNDP->ucNDPID * 100) -		\
			(_prNDP->ucNdlIndex) * NAN_MAX_SUPPORT_NDP_NUM)



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
nanDataFreeNdl(struct ADAPTER *prAdapter, struct _NAN_NDL_INSTANCE_T *prNDL) {
	uint8_t ucNdpIndex;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_SCHEDULER_T *prNanScheduler = nanGetScheduler(prAdapter);

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return;
	}

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
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

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (prDataPathInfo->ucNDLNum == 1) {
		cnmTimerStartTimer(prAdapter,
				   &prNanScheduler->rResumeRescheduleTimer,
				   NAN_RESUME_RESCHEDULE_TIMEOUT);
	} else if (prDataPathInfo->ucNDLNum == 0) {
		cnmTimerStopTimer(prAdapter,
				  &prNanScheduler->rResumeRescheduleTimer);
	}
#endif

#if CFG_SUPPORT_NAN_EXT
	if (!prDataPathInfo->ucNDLNum) {
		/* Resume CPU level for when no NDL exist */
		prAdapter->rPerMonitor.u4NanBoostCpu = 0;
		prAdapter->rPerMonitor.fgNanBoostCpuOff = TRUE;
#if (CFG_SUPPORT_NAN_11BE == 1)
		if (nanIsEhtSupport(prAdapter) &&
			!nanIsEhtEnable(prAdapter))
			nanEnableEht(prAdapter, TRUE);
#endif /* CFG_SUPPORT_NAN_11BE */
	}
#endif /* CFG_SUPPORT_NAN_EXT */
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
nanDataEngineInit(struct ADAPTER *prAdapter, uint8_t *pu1NMIAddress) {
	uint8_t i, j;
	uint32_t u4offset;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	enum NAN_BSS_ROLE_INDEX eRole = NAN_BSS_INDEX_BAND0;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	kalMemZero(prDataPathInfo, sizeof(struct _NAN_DATA_PATH_INFO_T));

	prDataPathInfo->ucNDLNum = 0;
	COPY_MAC_ADDR(prDataPathInfo->aucLocalNMIAddr, pu1NMIAddress);

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
			prDataPathInfo->arNDL[i].arNDP[j].prResponderSecSmInfo =
				nanSecGetResponderSm(
					(uint8_t)(u4offset));
		}

		/* initialize token for NAN scheduler */
		prDataPathInfo->arNDL[i].rToken.prNDL =
			&(prDataPathInfo->arNDL[i]);
		prDataPathInfo->arNDL[i].rToken.prAdapter = prAdapter;
	}

	prDataPathInfo->fgIsECSet = FALSE;
	kalMemZero(prDataPathInfo->aucECAttr,
		   sizeof(prDataPathInfo->aucECAttr));
	prDataPathInfo->prLocalHtCap = NULL;
	prDataPathInfo->prLocalVhtCap = NULL;
#if (CFG_SUPPORT_802_11AX == 1)
	prDataPathInfo->prLocalHeCap = NULL;
#endif
#if (CFG_SUPPORT_NAN_11BE == 1)
	prDataPathInfo->prLocalEhtCap = NULL;
#endif

	atomic_set(&(prDataPathInfo->NetDevRefCount[eRole]), 0);
	g_ndpReqNDPE.fgEnNDPE = FALSE;
	prDataPathInfo->fgAutoHandleDPRequest = FALSE;
	prDataPathInfo->ucDPResponseDecisionStatus =
		NAN_DATA_RESP_DECISION_ACCEPT;

	if (prAdapter->rWifiVar.fgEnableRandNdpid == TRUE)
		g_ucNdpId = (uint8_t)kalRandomNumber();

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (prAdapter->rWifiVar.ucNanEnable6gReschedInit == 1)
		nanRescheduleInit(prAdapter);
#endif
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
	enum NAN_BSS_ROLE_INDEX eRole = NAN_BSS_INDEX_BAND0;
	struct _NAN_NDP_INSTANCE_T *prNDP;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return;
	}

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (atomic_read(&(prDataPathInfo->NetDevRefCount[eRole])) > 0) {
		/* netif_carrier_off */
		kalNanIndicateStatusAndComplete(
			prAdapter->prGlueInfo,
			WLAN_STATUS_MEDIA_DISCONNECT, eRole);
	}
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (prAdapter->rWifiVar.ucNanEnable6gReschedInit == 1)
		nanRescheduleDeInit(prAdapter);
#endif

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
			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter,
				&(prDataPathInfo->arNDL[i]));
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
			struct _NAN_NDP_INSTANCE_T *prNDP) {
#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
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

	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}
	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prDataPathInfo) {
		DBGLOG(NAN, ERROR, "[%s] prDataPathInfo error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength, NAN_ATTR_ID_NDP);
	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength,
		NAN_ATTR_ID_NDP_EXTENSION);
	prAttrNDL = (struct _NAN_ATTR_NDL_T *)nanRetrieveAttrById(
		prNDP->pucAttrList, prNDP->u2AttrListLength, NAN_ATTR_ID_NDL);

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
			nan_sec_wpa_sm_rx_eapol(prNDP->prResponderSecSmInfo,
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

				DBGLOG(NAN, DEBUG, "[%s] App InfoLen = %d:\n",
					__func__, prNDP->u2AppInfoLen);
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
	} else
		return WLAN_STATUS_FAILURE;
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
nanNdpProcessDataRequest(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb) {
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
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
		/* NAN_CHK_PNT log message */
		nanLogFailRxReqStr("no_ndp_and_ndpe_attr");
		return WLAN_STATUS_FAILURE;
	}

	/* 2. create NDP on-the-fly - using specified peer-specified NDPID */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);

	if (prNDL &&
		prNDL->prOperatingNDP &&
		(prNDL->eNDLRole == NAN_PROTOCOL_INITIATOR) &&
		(ucNDPID > prNDL->prOperatingNDP->ucNDPID)) {
		DBGLOG(NAN, WARN,
			"Cross sending to free NDL with small NDPID %u (< %u)\n",
			prNDL->prOperatingNDP->ucNDPID,
			ucNDPID);

		if (prNDL->prOperatingNDP->eCurrentNDPProtocolState ==
			NDP_INITIATOR_TX_DP_REQUEST)
			nanNdpInitiatorRspEvent(prAdapter,
				prNDL->prOperatingNDP,
				WLAN_STATUS_FAILURE);

		if (prNDL->prOperatingNDP->eCurrentNDPProtocolState ==
			NDP_INITIATOR_RX_DP_RESPONSE)
			nanNdpDataTerminationEvent(prAdapter,
						   prNDL->prOperatingNDP);


		prNDL->eCurrentNDLMgmtState = NDL_TEARDOWN;
		nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
				  prNDL);
		prNDL->prOperatingNDP = NULL;
		prNDL = NULL;
	}

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

		DBGLOG(NAN, WARN, "%s(): reject by reason code [%d]\n",
		       __func__, ucReasonCode);

		/* NAN_CHK_PNT log message */
		nanLogFailRxReq(ucReasonCode);

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
			if (nanNdpParseAttributes(prAdapter,
						  NAN_ACTION_DATA_PATH_REQUEST,
						  pucAttrList, u2AttrListLength,
						  prNDL, prNDP) ==
						    WLAN_STATUS_SUCCESS) {
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
					DBGLOG(NAN, WARN,
					       "weird condition reject by reason code [%d]\n",
					       ucReasonCode);
					/* NAN_CHK_PNT log message */
					nanLogFailRxReq(ucReasonCode);
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
				       "%s(): reject by invalid parameters\n",
				       __func__);

				/* NAN_CHK_PNT log message */
				nanLogFailRxReq(NAN_REASON_CODE_INVALID_PARAMS);

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
			  struct SW_RFB *prSwRfb) {
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
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, DEBUG,
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
		/* NAN_CHK_PNT log message */
		nanLogFailRxRespStr("no_ndp_and_ndpe_attr");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		return WLAN_STATUS_FAILURE;
	}

	/* Special case: nanDPReqTxDone cnSecTxKdeAttrDoneomes later than
	 * DP Response
	 */
	if (prNDP->eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_REQUEST) {
		DBGLOG(NAN, DEBUG,
		       "Received DP Response earlier than DP Req TX Done");
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M1);

		nanDataPathProtocolFsmStep(prAdapter,
					   NDP_INITIATOR_RX_DP_RESPONSE, prNDP);
		nanNdpInitiatorRspEvent(prAdapter, prNDP, WLAN_STATUS_SUCCESS);
	}

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
				DBGLOG(NAN, DEBUG,
				       "Receive the werid Packet.\n");

				/* NAN_CHK_PNT log message */
				nanLogFailRxRespStr(
					"invalid_ndp_or_ndpe_parameters");

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
				DBGLOG(NAN, DEBUG,
				       "[%s] nanSchedNegoChkRmtCrbProposal: accept\n",
				       __func__);
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
				DBGLOG(NAN, DEBUG,
				       "[%s] nanSchedNegoChkRmtCrbProposal: counter\n",
				       __func__);
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
				DBGLOG(NAN, DEBUG,
				       "[%s] nanSchedNegoChkRmtCrbProposal: reject\n",
				       __func__);

				/* NAN_CHK_PNT log message */
				nanLogFailRxRespStr(
					"check_crb_proposal_failed");

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
			/* NAN_CHK_PNT log message */
			nanLogFailRxRespStr("parse_attr_failed");

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
			/* NAN_CHK_PNT log message */
			nanLogFailRxRespStr("parse_attr_failed");

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
nanNdpProcessDataConfirm(struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb) {
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, DEBUG,
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
		/* NAN_CHK_PNT log message */
		nanLogFailRxConfmStr("no_ndp_and_ndpe_attr");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		return WLAN_STATUS_FAILURE;
	}

	/* Special case: nanDPRespTxDone comes later than DP Confirm */
	if (prNDP->eCurrentNDPProtocolState == NDP_RESPONDER_TX_DP_RESPONSE &&
	    prNDP->ucNDPSetupStatus == NAN_ATTR_NDP_STATUS_CONTINUED) {
		DBGLOG(NAN, DEBUG,
		       "Received DP Confirm earlier than DP Resp TX Done");
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M2);

		nanDataPathProtocolFsmStep(prAdapter,
					   NDP_RESPONDER_RX_DP_CONFIRM, prNDP);
		nanNdpResponderRspEvent(prAdapter, prNDP, WLAN_STATUS_SUCCESS);
	}

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

				/* NAN_CHK_PNT log message */
				nanLogFailRxConfmStr(
					"invalid_ndp_or_ndpe_parameters");

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
				/* NAN_CHK_PNT log message */
				nanLogFailRxConfmStr(
					"check_crb_proposal_failed");

				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			}
		} else if (prNDP->fgRejectPending == TRUE) {
			/* NAN_CHK_PNT log message */
			nanLogFailRxConfmStr("parse_attr_failed");

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
			/* NAN_CHK_PNT log message */
			nanLogFailRxConfmStr("parse_attr_failed");

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
			    struct SW_RFB *prSwRfb) {
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, DEBUG,
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
		/* NAN_CHK_PNT log message */
		nanLogFailRxKeyInstlStr("no_ndp_and_ndpe_attr");
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		return WLAN_STATUS_FAILURE;
	}

	/* Special case: nanDPConfirmTxDone comes later than DP Key Install */
	if (prNDP->eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_CONFIRM &&
	    prNDP->ucNDPSetupStatus == NAN_ATTR_NDP_STATUS_CONTINUED) {
		DBGLOG(NAN, DEBUG,
		       "Received DP Key Install earlier than DP Confirm TX Done");
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M3);

		nanDataPathProtocolFsmStep(prAdapter,
				NDP_INITIATOR_RX_DP_SECURITY_INSTALL, prNDP);
	}

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
			     prAttrNDP == NULL)) {

				/* NAN_CHK_PNT log message */
				nanLogFailRxKeyInstlStr(
					"invalid_ndp_or_ndpe_parameters");

				nanDataPathProtocolFsmStep(
					prAdapter, NDP_TX_DP_TERMINATION,
					prNDP);
			} else {
				prNDP->ucRCPI = nicRxGetRcpiValueFromRxv(
					prAdapter, RCPI_MODE_WF0, prSwRfb);
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_NORMAL_TR, prNDP);
			}
		} else if (prNDP->fgRejectPending == TRUE) {
			/* NAN_CHK_PNT log message */
			nanLogFailRxKeyInstlStr("parse_attr_failed");

			/* in cases peer think connection has been established
			 * use Data Path Termination to REJECT
			 */
			nanDataPathProtocolFsmStep(
				prAdapter, NDP_TX_DP_TERMINATION, prNDP);
		} else {
			/* NAN_CHK_PNT log message */
			nanLogFailRxKeyInstlStr("parse_attr_failed");

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
			     struct SW_RFB *prSwRfb) {

	/* enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentState; */
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP = NULL;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. extract NDL from address */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL == NULL) {
		DBGLOG(NAN, DEBUG,
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
		/* invalid NAF as NAN_ACTION_DATA_PATH_TERMINATION
		 * should carry NDP/NDPE attribute
		 */
		return WLAN_STATUS_FAILURE;
	}

	if (prNDP == NULL) {
		/* unknown NDP-ID, ignore it - DoS Attack ? */
		return WLAN_STATUS_FAILURE;
	}

	/* Workaround for R2 cert 5.5.4 and 5.5.5 */
	if (nanGetFeatureIsSigma(prAdapter) &&
		nanIsTxKeyReady(prNDP->prContext)) {
		/* NAN Todo: Not HW_MAC_RX_DESC here */
#if (CFG_SUPPORT_CONNAC3X == 1)
		if (HAL_MAC_CONNAC3X_RX_STATUS_IS_CIPHER_MISMATCH(
		    (struct HW_MAC_CONNAC3X_RX_DESC *)prSwRfb->prRxStatus)
		    == TRUE) {
#elif (CFG_SUPPORT_CONNAC2X == 1)
		if (HAL_MAC_CONNAC2X_RX_STATUS_IS_CIPHER_MISMATCH(
		    (struct HW_MAC_CONNAC2X_RX_DESC *)prSwRfb->prRxStatus)
		    == TRUE) {
#else
		if (HAL_RX_STATUS_IS_CIPHER_MISMATCH(
		    (struct HW_MAC_RX_DESC *)prSwRfb->prRxStatus)
		    == TRUE) {
#endif
			DBGLOG(NAN, DEBUG,
			   "[PMF] Rx NON-PROTECT NAF, Wtbl:%d\n",
			   prSwRfb->ucWlanIdx);

			return WLAN_STATUS_FAILURE;
		}
	}

	/* update parameters through attribute parsing */
	if (nanNdpParseAttributes(prAdapter, NAN_ACTION_DATA_PATH_TERMINATION,
				  pucAttrList, u2AttrListLength, prNDL,
				  prNDP) == WLAN_STATUS_SUCCESS) {
		/* NAN_CHK_PNT log message */
		nanLogEndReason("recv_ndp_termination_frame");
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
u_int8_t nanIsNdpSetupOngoing(
	struct ADAPTER *prAdapter)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	uint8_t i, j;

	if (!prAdapter)
		return FALSE;

	prDataPathInfo = &(prAdapter->rDataPathInfo);
	if (!prDataPathInfo)
		return FALSE;

	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		if (!prDataPathInfo->arNDL[i].fgNDLValid)
			continue;

		for (j = 0; j < NAN_MAX_SUPPORT_NDP_NUM; j++) {
			enum _ENUM_NDP_PROTOCOL_STATE_T s;

			prNDP = &prDataPathInfo->arNDL[i].arNDP[j];

			if (!prNDP || !prNDP->fgNDPValid)
				continue;

			s = prNDP->eCurrentNDPProtocolState;

			if ((s > NDP_IDLE) &&
				(s < NDP_NORMAL_TR)) {
				DBGLOG(NAN, DEBUG,
					"NdpId(%d)\n",
					prNDP->ucNDPID);
				return TRUE;
			}
		}
	}

	return FALSE;
}

uint32_t
nanNdlProcessScheduleRequest(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
#if (CFG_SUPPORT_NAN_EXT == 1)
#if (CFG_SUPPORT_NAN_11BE == 1)
	struct _NAN_PEER_SCH_DESC_T *prPeerSchDesc = NULL;
#endif /* CFG_SUPPORT_NAN_11BE */
#endif /* CFG_SUPPORT_NAN_EXT */

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
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

	if (IS_FEATURE_FORCE_ENABLED(
		prAdapter->rWifiVar.fgNanNdpSkipSchedule)) {
		DBGLOG(NAN, ERROR, "Skip req\n");
		return WLAN_STATUS_FAILURE;
	} else if (prAdapter->rWifiVar.fgNanNdpSkipSchedule &&
		nanIsNdpSetupOngoing(prAdapter)) {
		prNDL = nanDataUtilSearchNdlByMac(prAdapter,
			prNaf->aucSrcAddr);
		if (prNDL != NULL) {
			/* Update FAW */
			nanNdlParseAttributes(
				prAdapter,
				NAN_ACTION_SCHEDULE_REQUEST,
				pucAttrList,
				u2AttrListLength,
				prNDL);
		}
		nanNdlSendScheduleResponse(
			prAdapter, NULL,
			prNaf->aucSrcAddr,
			prAttrNDL,
			NAN_REASON_CODE_RESOURCE_LIMITATION);
		return WLAN_STATUS_FAILURE;
	}

	/* 1. create NDL on the fly */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL != NULL) {
		if (prNDL->ucDialogToken == prAttrNDL->ucDialogToken) {
			if (prNDL->eCurrentNDLMgmtState ==
			    NDL_SCHEDULE_ESTABLISHED) {
				DBGLOG(NAN, DEBUG,
				       "RX Request in same dialog token");
			} else {
				DBGLOG(NAN, DEBUG, "Ignore same dialog token");
				return WLAN_STATUS_FAILURE;
			}
		}
		COPY_MAC_ADDR(prNDL->aucTxRespAddr, prNaf->aucSrcAddr);
		prNDL->eNDLRole = NAN_PROTOCOL_RESPONDER;

		nanSchedPeerPrepareNegoState(prAdapter, prNaf->aucSrcAddr);

		/* 2. parse NAN attributes for NDL */
		if (nanNdlParseAttributes(
			    prAdapter, NAN_ACTION_SCHEDULE_REQUEST, pucAttrList,
			    u2AttrListLength, prNDL) == WLAN_STATUS_SUCCESS) {
			if (prNDL->eCurrentNDLMgmtState == NDL_IDLE ||
			    prNDL->eCurrentNDLMgmtState ==
				    NDL_SCHEDULE_ESTABLISHED) {
#if (CFG_SUPPORT_NAN_EXT == 1)
#if (CFG_SUPPORT_NAN_11BE == 1)
				uint8_t fgEhtSlotExist = FALSE;
				/* if peer origin nego is EHT and now is not,
				 * switch mode from EHT to HE
				 */
				prPeerSchDesc =
					nanSchedAcquirePeerSchDescByNmi(
						prAdapter,
						prNDL->aucPeerMacAddr);

				fgEhtSlotExist =
					nanSchedCheckEHTSlotExist(prAdapter);

				DBGLOG(NAN, DEBUG, "EHT slot exist: %u\n",
				       fgEhtSlotExist);

				if (prPeerSchDesc)
					DBGLOG(NAN, DEBUG,
					       "Check peer EHT: %u\n",
					       prPeerSchDesc->fgEht);

				if (fgEhtSlotExist &&
				    prPeerSchDesc && !prPeerSchDesc->fgEht &&
				    nanIsEhtSupport(prAdapter)) {

					DBGLOG(NAN, DEBUG,
					       "Switch mode due to peer reschedule\n");
					nanEnableEht(prAdapter, FALSE);
				}
#endif /* CFG_SUPPORT_NAN_11BE */
#endif /* CFG_SUPPORT_NAN_EXT */

				nanNdlMgmtFsmStep(prAdapter,
						  NDL_REQUEST_SCHEDULE_NDL,
						  prNDL);
			} else {
				/* insert into queue for later handling */
				nanDataEngineInsertRequest(prAdapter,
					prNDL,
					NAN_DATA_ENGINE_REQUEST_NDL_SETUP,
					prNDL->eNDLRole,
					NULL);
			}
			rStatus = WLAN_STATUS_SUCCESS;
		} else {
			nanSchedPeerCompleteNegoState(prAdapter,
						      prNaf->aucSrcAddr);

			nanNdlSendScheduleResponse(prAdapter, NULL,
						   prNaf->aucSrcAddr, prAttrNDL,
						   prNDL->ucReasonCode);
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
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
#else
			DBGLOG(NAN, WARN,
			       "Prevent NDL teardown for availability!\n");
			rStatus = WLAN_STATUS_SUCCESS;
#endif
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
			      struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
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
		 * ScheduleReqTxDone
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
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
#else
				/* If peer schedule resp let us tear down,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				       "Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);

				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
#endif
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
			     struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4RejectCode;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
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
			if (nanNdlParseAttributes(prAdapter,
				    NAN_ACTION_SCHEDULE_CONFIRM,
				    pucAttrList, u2AttrListLength, prNDL) ==
			    WLAN_STATUS_SUCCESS) {
				rStatus = nanSchedNegoChkRmtCrbProposal(
					prAdapter, &u4RejectCode);
				DBGLOG(NAN, INFO, "Status=%u\n", rStatus);
				if (prNDL->ucNDLSetupCurrentStatus ==
				    NAN_ATTR_NDL_STATUS_ACCEPTED &&
				    rStatus == WLAN_STATUS_SUCCESS) {
					nanNdlMgmtFsmStep(prAdapter,
						NDL_SCHEDULE_ESTABLISHED,
						prNDL);
				} else /* REJECT */
					nanNdlMgmtFsmStep(prAdapter,
							  NDL_TEARDOWN, prNDL);

				rStatus = WLAN_STATUS_SUCCESS;
			} else {
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
#else
				/* If peer schedule confirm let us tear down,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				       "Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);

				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
#endif
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
					struct SW_RFB *prSwRfb) {
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ACTION_FRAME_T *prNaf = NULL;
	uint8_t *pucAttrList = NULL;
	uint16_t u2AttrListLength;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prSwRfb) {
		DBGLOG(NAN, ERROR, "[%s] prSwRfb error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNaf = (struct _NAN_ACTION_FRAME_T *)(prSwRfb->pvHeader);
	if (!prNaf) {
		DBGLOG(NAN, ERROR, "[%s] prNaf error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2AttrListLength =
		prSwRfb->u2PacketLen -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);
	pucAttrList = (uint8_t *)(prNaf->aucInfoContent);

	/* 1. search for previously created NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNaf->aucSrcAddr);
	if (prNDL) {
		DBGLOG(NAN, DEBUG, "ucNDLSetupCurrentStatus:%d\n",
		       prNDL->ucNDLSetupCurrentStatus);
		if (prNDL->eCurrentNDLMgmtState == NDL_SCHEDULE_ESTABLISHED) {
			/* 2. retrieve ATTR-NDL from payload */
			if (nanNdlParseAttributes(prAdapter,
					NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION,
					pucAttrList, u2AttrListLength,
					prNDL) == WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, DEBUG, "NDL status=%u\n",
						prNDL->ucNDLSetupCurrentStatus);

				/* NAN_ATTR_NDL_STATUS_REJECTED:
				 * there shall no this status, log status code.
				 * NAN_ATTR_NDL_STATUS_ACCEPTED:
				 * NAN_ATTR_NDL_STATUS_CONTINUED (rarely):
				 * normal case, stay in NDL_SCHEDULE_ESTABLISHED
				 */

				rStatus = WLAN_STATUS_SUCCESS;
			} else {
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
				/* reject condition - destroy */
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
#else
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
#endif
			}
		} else {
			DBGLOG(NAN, DEBUG,
			       "received unexpected schedule update notification in state=%u",
			       prNDL->eCurrentNDLMgmtState);
		}
	} else {
		DBGLOG(NAN, TRACE, "unknown peer, ignore it");
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
static enum _ENUM_NAN_NDP_STATUS_T
nanDataPathProtocolFsmStep(struct ADAPTER *prAdapter,
			   enum _ENUM_NDP_PROTOCOL_STATE_T eNextState,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	enum _ENUM_NDP_PROTOCOL_STATE_T eLastState;
	enum _ENUM_NAN_NDP_STATUS_T eNdpConnectionStatus;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return NAN_NDP_DISCONNECT;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return NAN_NDP_DISCONNECT;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return NAN_NDP_DISCONNECT;
	}

	eNdpConnectionStatus = NAN_NDP_DISCONNECT;

	do {

		/* TODO: Define your own dbg level */
		DBGLOG(NAN, INFO,
		       "NDP mgmt STATE_%u:(" MACSTR ") [%s] -> [%s]\n",
		       prNDP->ndp_instance_id,
		       MAC2STR(prNDP->aucPeerNDIAddr),
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

			if (!prNDP->prRetryMsduInfo) {
				/* generate new dialog token */
				if (prNDL->fgScheduleEstablished == FALSE)
					nanNdlGenerateDialogToken(prAdapter,
								  prNDL);
				nanNdpGenerateDialogToken(prAdapter, prNDP);
			}

			/* send Data Path Request NAF */
			nanNdpSendDataPathRequest(prAdapter, prNDP);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);
			cnmTimerStartTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)],
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
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);
			cnmTimerStartTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)],
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
				prAdapter->rWifiVar.u4NanRespTimeout);

			break;

		case NDP_RESPONDER_TX_DP_RESPONSE:
			/* stop timer first */
			cnmTimerStopTimer(prAdapter,
				&(prNDP->rNDPUserSpaceResponseTimer));

			nanNdpUpdateTypeStatus(prAdapter, prNDP);

			nanNdpSendDataPathResponse(
				prAdapter, prNDP, NULL, NULL,
				NAN_REASON_CODE_RESERVED, NULL,
				NAN_REASON_CODE_RESERVED, NULL,
				NAN_REASON_CODE_RESERVED);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);
			cnmTimerStartTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)],
			       prAdapter->rWifiVar.u4NanConfirmTimeout);

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
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);
			cnmTimerStartTimer(prAdapter,
			       &prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)],
			       NAN_DATA_RETRY_TIMEOUT);

			break;

		case NDP_NORMAL_TR:
			/* stop all timers for handsahking */
			nanNdlDeactivateTimers(prAdapter, prNDL);

			/* Allocate STA-REC */
			if (nanDataEngineEnrollNDPContext(prAdapter, prNDL,
							  prNDP) !=
			    WLAN_STATUS_SUCCESS) {

				/* NAN_CHK_PNT log message */
				nanLogNdpFinlConfm("fail", prNDL, prNDP);

				DBGLOG(NAN, ERROR,
				       "NDP_NORMAL_TR: STA-REC allocation failure\n");
				prNDP->fgNDPEstablish = FALSE;
				eNextState = NDP_TX_DP_TERMINATION;
			} else {

				/* NAN_CHK_PNT log message */
				nanLogNdpFinlConfm("success", prNDL, prNDP);

				/* NAN_CHK_PNT log message */
				nanSchedDbgDumpCommittedSlotAndChannel(
					prAdapter,
					"ndp_setup_complete");
				nanSchedDbgDumpPeerCommittedSlotAndChannel(
					prAdapter,
					prNDL->aucPeerMacAddr,
					"ndp_setup_complete");

				/* send data confirm indication to host layer */
				prNDP->fgNDPEstablish = TRUE;

				/* roll NDL state machine to schedule
				 * estabilshed
				 */
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
				nanDataPathSetupSuccess(prAdapter, prNDP);
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
nanNdlMgmtFsmNdlScheduleSetup(struct ADAPTER *prAdapter,
			      struct _NAN_NDL_INSTANCE_T *prNDL) {
	enum _ENUM_NDL_MGMT_STATE_T eNextState = NDL_SCHEDULE_SETUP;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4RejectCode = 0;

	do {
		if (prNDL->prOperatingNDP) {
			if (prNDL->prOperatingNDP->eNDPRole ==
				    NAN_PROTOCOL_INITIATOR &&
			    prNDL->fgScheduleEstablished == FALSE) {
				/* Query scheduler for gen availiblity */
				rStatus =
				nanSchedNegoGenLocalCrbProposal(prAdapter);
				if (rStatus != WLAN_STATUS_SUCCESS) {
					DBGLOG(NAN, WARN,
						"[%s] Reject by scheduler,status:0x%x\n",
					       __func__, rStatus);
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
					DBGLOG(NAN, DEBUG,
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
					DBGLOG(NAN, DEBUG,
					       "NegoChkRmtCrbProposal:counter\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_CONTINUED;
					prNDL->prOperatingNDP
						->ucNDPSetupStatus =
						NAN_ATTR_NDP_STATUS_CONTINUED;
				} else {
					DBGLOG(NAN, DEBUG,
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
nanNdlGetNextRequire(struct ADAPTER *prAdapter,
		     struct _NAN_NDL_INSTANCE_T *prNDL,
		     enum _ENUM_NDL_MGMT_STATE_T eCurrentState) {
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
nanNdlMgmtFsmStep(struct ADAPTER *prAdapter,
		  enum _ENUM_NDL_MGMT_STATE_T eNextState,
		  struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4RejectCode = 0;
	enum _ENUM_NDL_MGMT_STATE_T eLastState;
	u_int8_t fgLooping = TRUE;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return;
	}

	do {
		DBGLOG(NAN, STATE,
		       "NDL mgmt STATE_%d:(" MACSTR ") [%s] -> [%s]\n",
		       prNDL->ucIndex, /* prNDL->ucPeerID */
		       MAC2STR(prNDL->aucPeerMacAddr),
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
#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
				if (prAdapter->rWifiVar.ucNanEnable6gReschedInit
				    == 1 && nanCheckIsNeedReschedule(prAdapter,
				    NEW_NDL, prNDL->aucPeerMacAddr)) {
					prNDL->fgTriggerReschedNewNDL = TRUE;

					nanRescheduleEnqueueNewToken(
						prAdapter,
						NEW_NDL,
						prNDL);
				}
#endif
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
					  &prNDL->arNDLProtocolReschRetryTimer
					  [NAN_PROTOCOL_INITIATOR]);
			cnmTimerStartTimer(prAdapter,
					   &prNDL->arNDLProtocolReschRetryTimer
					   [NAN_PROTOCOL_INITIATOR],
					   prAdapter->rWifiVar.u4NanSchTimeout);
			break;

		case NDL_RESPONDER_RX_SCHEDULE_REQUEST:
			prNDL->ucTxRetryCounter = 0;

			/* Query scheduler for checking availbility */
			rStatus =
				nanSchedNegoChkRmtCrbProposal(prAdapter,
							      &u4RejectCode);
			if (rStatus == WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, DEBUG,
				       "NegoChkRmtCrbProposal:accept\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_ACCEPTED;
			} else if (rStatus == WLAN_STATUS_NOT_ACCEPTED) {
				DBGLOG(NAN, DEBUG,
				       "NegoChkRmtCrbProposal:counter\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_CONTINUED;
			} else {
				DBGLOG(NAN, DEBUG,
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
					  &prNDL->arNDLProtocolReschRetryTimer
					  [NAN_PROTOCOL_RESPONDER]);
			cnmTimerStartTimer(prAdapter,
					   &prNDL->arNDLProtocolReschRetryTimer
					   [NAN_PROTOCOL_RESPONDER],
					   prAdapter->rWifiVar.u4NanSchTimeout);
			break;

		case NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE:
			/* but reset timer and wait for incoming
			 * Schedule Request
			 */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));
			cnmTimerStartTimer(prAdapter,
					   &(prNDL->rNDPProtocolExpireTimer),
					   NAN_SECURITY_TIMEOUT);
			break;

		case NDL_INITIATOR_RX_SCHEDULE_RESPONSE:
			/* Query scheduler for checking availbility */
			cnmTimerStopTimer(prAdapter,
					  &(prNDL->rNDPProtocolExpireTimer));

			rStatus =
			nanSchedNegoChkRmtCrbProposal(prAdapter, &u4RejectCode);
			if (rStatus == WLAN_STATUS_SUCCESS) {
				if (prNDL->fgIsCounter) {
					DBGLOG(NAN, DEBUG,
					       "NDL status continue\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_ACCEPTED;
					eNextState =
					    NDL_INITIATOR_TX_SCHEDULE_CONFIRM;
				} else {
					DBGLOG(NAN, DEBUG,
					       "NegoChkRmtCrbProposal:accept\n");
					prNDL->ucNDLSetupCurrentStatus =
						NAN_ATTR_NDL_STATUS_ACCEPTED;
					eNextState = NDL_SCHEDULE_ESTABLISHED;
				}
			} else if (rStatus == WLAN_STATUS_NOT_ACCEPTED) {
				DBGLOG(NAN, DEBUG,
				       "NegoChkRmtCrbProposal:counter\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_CONTINUED;
				eNextState = NDL_INITIATOR_TX_SCHEDULE_CONFIRM;
			} else {
				DBGLOG(NAN, DEBUG,
				       "NegoChkRmtCrbProposal:reject NDL reschedule\n");
				prNDL->ucNDLSetupCurrentStatus =
					NAN_ATTR_NDL_STATUS_REJECTED;
				eNextState = NDL_INITIATOR_TX_SCHEDULE_CONFIRM;
			}
			break;

		case NDL_INITIATOR_TX_SCHEDULE_CONFIRM:
			nanNdlSendScheduleConfirm(prAdapter, prNDL);

			/* Timer setup for retry */
			cnmTimerStopTimer(prAdapter,
					  &prNDL->arNDLProtocolReschRetryTimer
					  [NAN_PROTOCOL_INITIATOR]);
			cnmTimerStartTimer(prAdapter,
					   &prNDL->arNDLProtocolReschRetryTimer
					   [NAN_PROTOCOL_INITIATOR],
					   prAdapter->rWifiVar.u4NanSchTimeout);
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
					   NAN_SECURITY_TIMEOUT);
			break;

		case NDL_SCHEDULE_ESTABLISHED:
			if (prNDL->eLastNDLMgmtState ==
						NDL_SCHEDULE_ESTABLISHED) {
				DBGLOG(NAN, DEBUG,
				       "Stay in NDL_SCHEDULE_ESTABLISHED\n");
				break;
			}

			prNDL->fgScheduleEstablished = TRUE;
			nanSchedSetLocalCommittedBand(prAdapter);

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

			/* Prefer band might be switched */
			nanUpdateStaRecIdx(prAdapter, prNDL);

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
				 * 1. fall back to Schedule Established
				 * 2. In the case 2 NDP exist and one NDP
				 *    sends schedule request followed by
				 *    teardown immediately.
				 *    the state transition:
				 *    NDL_SCHEDULE_ESTABLISHED
				 *    -> NDL_REQUEST_SCHEDULE_NDL
				 *    (NDP_NORMAL_TR -> NDP_DISCONNECT)
				 *    -> NDL_TEARDOWN_BY_NDP_TERMINATION
				 *    -> "NDL_REQUEST_SCHEDULE_NDL"
				 *    stay in NDL_REQUEST_SCHEDULE_NDL and
				 *    wait for nanSchedNegoDispatchTimeout to
				 *    proceed the state
				 *    else:
				 *    -> "NDL_SCHEDULE_ESTABLISHED"
				 */
				if (prNDL->eLastNDLMgmtState ==
				    NDL_REQUEST_SCHEDULE_NDL) {
					eNextState = prNDL->eLastNDLMgmtState;
					fgLooping = FALSE;
				} else {
					eNextState = NDL_SCHEDULE_ESTABLISHED;
				}
			}
			break;

		default:
			break;
		}
	} while (eLastState != eNextState && fgLooping);

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
	if (prAdapter->rWifiVar.ucNanEnable6gReschedInit == 1) {
		switch (eLastState) {
		case NDL_IDLE:
			nanRescheduleNdlIfNeeded(prAdapter, REMOVE_NDL, prNDL);
			break;
		case NDL_SCHEDULE_ESTABLISHED:
			nanRescheduleNdlIfNeeded(prAdapter, NEW_NDL, prNDL);
			break;
		default:
			break;
		}
	}
#endif
}

void nanSetNdpPmkid(
	struct ADAPTER *prAdapter,
	struct _NAN_CMD_DATA_REQUEST *prNanCmdDataRequest,
	uint8_t *puServiceName
){
	int i = 0;
	u8 pmkid[32];
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	struct BSS_INFO *prBssInfo;

	/* Get BSS info */
	prNanSpecificBssInfo = nanGetSpecificBssInfo(
		prAdapter,
		NAN_BSS_INDEX_BAND0);
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
	caculate_pmkid(
		prNanCmdDataRequest->aucPMK,
		prBssInfo->aucOwnMacAddr,
		prNanCmdDataRequest->aucResponderDataAddress,
		puServiceName, pmkid);
	DBGLOG(NAN, LOUD, "[publish] SCID=>");
	for (i = 0 ; i < 15; i++)
		DBGLOG(NAN, LOUD, "%X:", pmkid[i]);

	DBGLOG(NAN, LOUD, "%X\n", pmkid[i]);
	kalMemCopy(prNanCmdDataRequest->aucScid, pmkid, 16);
	dumpMemory8(
		prNanCmdDataRequest->aucScid, NAN_SCID_DEFAULT_LEN);
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
int32_t nanCmdDataRequest(struct ADAPTER *prAdapter,
		  struct _NAN_CMD_DATA_REQUEST *prNanCmdDataRequest,
		  uint8_t *pu1NdpId, uint8_t *au1InitiatorDataAddr)
{
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	unsigned char fgAllocNDL = FALSE;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanCmdDataRequest) {
		DBGLOG(NAN, ERROR,
			"[%s] prNanCmdDataRequest error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Workaround for NDP termination */
	kalMemCopy(prAdapter->rDataPathInfo.aucRemoteAddr,
		   prNanCmdDataRequest->aucResponderDataAddress, MAC_ADDR_LEN);

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
	DBGLOG(NAN, INFO,
		"[%s] ucPublishID:%d, ucRequireQOS:%d, ucSecurity%d, u2SpecificInfoLength:%d\n",
		__func__, prNanCmdDataRequest->ucPublishID,
		prNanCmdDataRequest->ucRequireQOS,
		prNanCmdDataRequest->ucSecurity,
		prNanCmdDataRequest->u2SpecificInfoLength);
	dumpMemory8(prNanCmdDataRequest->aucResponderDataAddress, MAC_ADDR_LEN);
#endif

	/* check for existing NDL where peer address exists or not */
	prNDL = nanDataUtilSearchNdlByMac(
		prAdapter, prNanCmdDataRequest->aucResponderDataAddress);
	if (prNDL) {
		if (prNDL->prOperatingNDP) {
			DBGLOG(NAN, WARN, "EAGAIN for OperatingNDP exists\n");
			return -EAGAIN;
		}

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
		DBGLOG(NAN, DEBUG,
		       "[%s] Use the same NDL (Qos no need to update)\n",
		       __func__);
	}

	if (prNanCmdDataRequest->ucSecurity) {
		prNDP->ucCipherType = prNanCmdDataRequest->ucSecurity;
		kalMemCopy(prNDP->au1Scid, prNanCmdDataRequest->aucScid,
			   NAN_SCID_DEFAULT_LEN);
		kalMemCopy(prNDP->aucPMK, prNanCmdDataRequest->aucPMK,
			   NAN_PMK_INFO_LEN);

		/* Trigger NAN SEC */
		nanSecApSmBufReset(prNDP->prInitiatorSecSmInfo);
		nanSecSetPmk(prNDP, NAN_PMK_INFO_LEN,
			     prNanCmdDataRequest->aucPMK);
		nanSecSetCipherType(prNDP, prNanCmdDataRequest->ucSecurity);
		nanSecNotify4wayBegin(prNDP);
	}

	/* R3 capability */
	/* AppInfo */
	if (prNanCmdDataRequest->u2SpecificInfoLength > 0)
		nanDataEngineUpdateAppInfo(prAdapter,
		prNDP,
		NAN_SERVICE_PROTOCOL_TYPE_GENERIC,
		prNanCmdDataRequest->u2SpecificInfoLength,
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
	DBGLOG(NAN, DEBUG, "[%s] Update App info len = %d and QoS:%d\n",
		__func__, prNDP->u2AppInfoLen,
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
	TRACE_FUNC(NAN, DEBUG, "[%s] Done\n");

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
int32_t nanCmdDataResponse(struct ADAPTER *prAdapter,
		   struct _NAN_CMD_DATA_RESPONSE *prNanCmdDataResponse)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_ATTR_NDL_T *prAttrNDL;
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	const uint8_t aucNULLAddr[] = NULL_MAC_ADDR;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}
	prDataPathInfo = &(prAdapter->rDataPathInfo);

	if (!prNanCmdDataResponse) {
		DBGLOG(NAN, ERROR,
			"[%s] prNanCmdDataResponse error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
	DBGLOG(NAN, DEBUG,
	       "ucNDPId:%d, aucInitiatorDataAddress=>%02x:%02x:%02x:%02x:%02x:%02x\n",
	       prNanCmdDataResponse->ucNDPId,
	       prNanCmdDataResponse->aucInitiatorDataAddress[0],
	       prNanCmdDataResponse->aucInitiatorDataAddress[1],
	       prNanCmdDataResponse->aucInitiatorDataAddress[2],
	       prNanCmdDataResponse->aucInitiatorDataAddress[3],
	       prNanCmdDataResponse->aucInitiatorDataAddress[4],
	       prNanCmdDataResponse->aucInitiatorDataAddress[5]);
	DBGLOG(NAN, DEBUG,
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
		if (prNanCmdDataResponse->ndp_instance_id)
			prNDP = nanDataUtilSearchNdpByNdpInstanceId(
				prAdapter,
				prNanCmdDataResponse->ndp_instance_id);
		else
			prNDP = nanDataUtilSearchNdpByNdpIdOnly(
				prAdapter,
				prNanCmdDataResponse->ucNDPId);
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
					DBGLOG(NAN, DEBUG,
						"[%s] Enter parse AppInfo\n",
						__func__);
					prDataPathInfo->u2AppInfoLen =
						prNanCmdDataResponse->
						u2SpecificInfoLength;
					prDataPathInfo->pucAppInfo =
						cnmMemAlloc(prAdapter,
						RAM_TYPE_BUF,
						(uint32_t)prDataPathInfo->
						u2AppInfoLen);
					if (!prDataPathInfo->pucAppInfo)
						return WLAN_STATUS_FAILURE;

					kalMemCopy(prDataPathInfo->pucAppInfo,
						prNanCmdDataResponse->
						aucSpecificInfo,
						prDataPathInfo->u2AppInfoLen);
				}

				if (nanGetFeatureIsSigma(prAdapter)) {
					prDataPathInfo->u2PortNum =
						prNanCmdDataResponse->u2PortNum;
					prDataPathInfo->ucProtocolType =
						prNanCmdDataResponse
						->ucServiceProtocolType;
				}

				DBGLOG(NAN, DEBUG,
					"[%s] AppInfoLen %d\n",
					__func__,
					prDataPathInfo->u2AppInfoLen);
			}

			nanNdpResponderUserTimeoutEvent(prAdapter,
				prNanCmdDataResponse->ndp_instance_id,
				prNanCmdDataResponse->u2NdpTransactionId);
			return WLAN_STATUS_SUCCESS;
		}

		/* no matching NDL/NDP - it should have been created when NAF
		 * - Data Path Request is received
		 */
		nanNdpResponderUserTimeoutEvent(prAdapter,
			prNanCmdDataResponse->ndp_instance_id,
			prNanCmdDataResponse->u2NdpTransactionId);

		return WLAN_STATUS_FAILURE;
	}

	/* Transaction Id */
	prNDP->u2TransId =  prNanCmdDataResponse->u2NdpTransactionId;

	if (prNanCmdDataResponse->ucRequireQOS == TRUE) {
		prNDL->ucMinimumTimeSlot = prNanCmdDataResponse->ucMinTimeSlot;
		prNDL->u2MaximumLatency = prNanCmdDataResponse->u2MaxLatency;
	}

	/* If data response doesn't compliant data request's
	 * security requirement
	 */
	if ((prNDP->fgSecurityRequired &&
		(prNanCmdDataResponse->ucSecurity != prNDP->ucCipherType)) ||
		(!prNDP->fgSecurityRequired &&
		prNanCmdDataResponse->ucSecurity)) {
		DBGLOG(NAN, DEBUG,
			"SecurityRequired:%d, CipherType:%d\n",
			prNDP->fgSecurityRequired, prNDP->ucCipherType);
		prNanCmdDataResponse->ucDecisionStatus =
			NAN_DATA_RESP_DECISION_REJECT;

		/* Because NAN security SM init only after data response
		 * accept the connection establishment, to
		 * reset NDP security required to avoid touch security SM.
		 */
		prNDP->fgSecurityRequired = FALSE;
	}

	if (prNanCmdDataResponse->ucDecisionStatus ==
	    NAN_DATA_RESP_DECISION_ACCEPT) {
		/* fill NDP parameters from response request */
		if (prNanCmdDataResponse->ucRequireQOS &
		    NAN_DATAREQ_REQUIRE_QOS_UNICAST)
			prNDP->fgQoSRequired = TRUE;

		else
			prNDP->fgQoSRequired = FALSE;

		if (prNanCmdDataResponse->ucSecurity) {
			kalMemCopy(prNDP->aucPMK, prNanCmdDataResponse->aucPMK,
				   NAN_PMK_INFO_LEN);

			/* Trigger NAN SEC: Responder
			 * nanSecStaSmBufReset(prNDP->prResponderSecSmInfo);
			 */
			nanSecSetPmk(prNDP, NAN_PMK_INFO_LEN,
				     prNanCmdDataResponse->aucPMK);
			nanSecSetCipherType(prNDP,
					    prNanCmdDataResponse->ucSecurity);
			nanSecNotify4wayBegin(prNDP);
			rStatus = nan_sec_wpa_sm_rx_eapol(
				prNDP->prResponderSecSmInfo,
				prNDP->aucPeerNDIAddr);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(NAN, ERROR,
				       "[%s] ERROR! Process Rx share key attribute failed! rStatus:0x%x\n",
				       __func__, rStatus);
				return rStatus;
			}

		} else
			prNDP->fgSecurityRequired = FALSE;

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
				goto exit; /* Keep timer */
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

		nanNdpResponderRspEvent(prAdapter,
			prNDP,
			WLAN_STATUS_FAILURE);

		nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT, prNDP);
	}

	/* stop timer first */
	cnmTimerStopTimer(prAdapter, &(prNDP->rNDPUserSpaceResponseTimer));
exit:

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
int32_t nanCmdDataEnd(struct ADAPTER *prAdapter,
	      struct _NAN_CMD_DATA_END *prNanCmdDataEnd)
{
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanCmdDataEnd) {
		DBGLOG(NAN, ERROR, "[%s] prNanCmdDataEnd error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	kalMemCopy(prNanCmdDataEnd->aucInitiatorDataAddress,
		   prAdapter->rDataPathInfo.aucRemoteAddr, MAC_ADDR_LEN);

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
	DBGLOG(NAN, DEBUG, "[%s] ucNDPId:%d\n", __func__,
	       prNanCmdDataEnd->ucNDPId);
	dumpMemory8(prNanCmdDataEnd->aucInitiatorDataAddress, MAC_ADDR_LEN);
#endif

	/* NAN_CHK_PNT log message */
	nanLogEndReason("framework_command");

	if (prNanCmdDataEnd->ndp_instance_id)
		prNDP = nanDataUtilSearchNdpByNdpInstanceId(prAdapter,
			prNanCmdDataEnd->ndp_instance_id);
	else
		prNDP = nanDataUtilSearchNdpByNdpIdOnly(prAdapter,
			prNanCmdDataEnd->ucNDPId);

	if (prNDP == NULL) {
		/* Send rsp event to wifi hal*/
		nanNdpEndRspEvent(prAdapter,
			DP_REASON_SUCCESS,
			prNanCmdDataEnd->u2NdpTransactionId,
			WLAN_STATUS_SUCCESS);

		/* no matching NDL/NDP - it should have been created when NAF
		 * - Data Path Request is received
		 */
		DBGLOG(NAN, DEBUG, "[%s] might already terminate\n", __func__);

		/* @TODO: return a unsuccessful indication to host side */
		rStatus = WLAN_STATUS_SUCCESS;
	} else {
		/* Transaction Id */
		prNDP->u2TransId = prNanCmdDataEnd->u2NdpTransactionId;

		if (prNDP->eCurrentNDPProtocolState == NDP_NORMAL_TR) {
			/* roll the state machine for disconnection handling */
			nanDataPathProtocolFsmStep(
				prAdapter, NDP_TX_DP_TERMINATION, prNDP);
#if !NAF_REPORT_END_RSP_EVENT_TXDONE
			/* Send rsp event to wifi hal*/
			nanNdpEndRspEvent(prAdapter,
				DP_REASON_SUCCESS,
				prNanCmdDataEnd->u2NdpTransactionId,
				WLAN_STATUS_SUCCESS);
#endif
		} else {
			if (prNDP->eCurrentNDPProtocolState == NDP_IDLE)
				nanDataEngineRemovePendingRequests(
					prAdapter, prNDL,
					NAN_DATA_ENGINE_REQUEST_NDP_SETUP,
					prNDP);

			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
			/* Send rsp event to wifi hal*/
			nanNdpEndRspEvent(prAdapter,
				DP_REASON_SUCCESS,
				prNanCmdDataEnd->u2NdpTransactionId,
				WLAN_STATUS_SUCCESS);
		}
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
nanCmdDataUpdtae(struct ADAPTER *prAdapter,
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
nanUpdateNdlSchedule(struct ADAPTER *prAdapter,
		     struct _NAN_PARAMETER_NDL_SCH *prNanUpdateSchParam) {
#if 0 /* whsu: skip for skip the calling parameter enum error !!! */
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
#endif
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNanUpdateSchParam) {
		DBGLOG(NAN, ERROR,
			"[%s] prNanUpdateSchParam error\n", __func__);
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

			DBGLOG(NAN, DEBUG,
			       "[%s] Update SCH MinTimeSlot %d MaxLT %d\n",
			       __func__, prNDL->ucMinimumTimeSlot,
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
nanNdpSendDataIndicationEvent(struct ADAPTER *prAdapter,
			      struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathRequestInd rDataReqInd = {0};

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return;
	}

	rDataReqInd.eventID = ENUM_NAN_DATA_INDICATION;
	rDataReqInd.service_instance_id = prNDP->ucPublishId;
	rDataReqInd.ndp_instance_id = prNDP->ucNDPID;
	rDataReqInd.ndp_cfg.qos_cfg = prNDP->fgQoSRequired ? 1 : 0;
	rDataReqInd.ndp_cfg.security_cfg = prNDP->fgSecurityRequired ? 1 : 0;
	DBGLOG(NAN, DEBUG, "Incoming Data Indication, PID:%d, NDPID:%d\n",
	       rDataReqInd.service_instance_id, rDataReqInd.ndp_instance_id);
	if (nanDataEngineNDPECheck(prAdapter, prNDP->fgSupportNDPE) == TRUE) {
		rDataReqInd.fgSupportNDPE = TRUE;
		if (prNDP->fgIsInitiator == TRUE)
			kalMemCopy(rDataReqInd.aucIPv6Addr,
				   prNDP->aucRspInterfaceId, IPV6MACLEN);
		else
			kalMemCopy(rDataReqInd.aucIPv6Addr,
				   prNDP->aucInterfaceId, IPV6MACLEN);
	}
	kalMemCopy(rDataReqInd.peer_disc_mac_addr, prNDL->aucPeerMacAddr,
		   MAC_ADDR_LEN);

	if (prNDP->fgSecurityRequired) {
		kalMemCopy(&rDataReqInd.aucSCID[0], &prNDP->au1Scid[0],
			   NAN_SCID_DEFAULT_LEN);
		rDataReqInd.uCipher = prNDP->ucCipherType;

		DBGLOG(NAN, DEBUG, "[%s] uCipher:%d\n", __func__,
		       rDataReqInd.uCipher);
		dumpMemory8(rDataReqInd.aucSCID, NAN_SCID_DEFAULT_LEN);
	}

	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rDataReqInd,
				sizeof(struct NanDataPathRequestInd));
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
nanNdpSendDataConfirmEvent(struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathConfirmInd rDataConfirmInd = {0};

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	rDataConfirmInd.eventID = ENUM_NAN_DATA_CONFIRM;
	rDataConfirmInd.ndp_instance_id = prNDP->ucNDPID;
	rDataConfirmInd.rsp_code =
		((prNDP->ucTxNextTypeStatus & NAN_ATTR_NDP_STATUS_MASK) >>
		 NAN_ATTR_NDP_STATUS_OFFSET);
	rDataConfirmInd.reason_code = prNDP->ucReasonCode;

	kalMemCopy(rDataConfirmInd.peer_ndi_mac_addr, prNDP->aucPeerNDIAddr,
		   MAC_ADDR_LEN);

	rDataConfirmInd.fgIsSec = prNDP->fgSecurityRequired;
	if (nanDataEngineNDPECheck(prAdapter, prNDP->fgSupportNDPE) == TRUE) {
		rDataConfirmInd.fgSupportNDPE = TRUE;
		if (prNDP->fgIsInitiator == TRUE)
			kalMemCopy(rDataConfirmInd.aucIPv6Addr,
				   prNDP->aucRspInterfaceId, IPV6MACLEN);
		else
			kalMemCopy(rDataConfirmInd.aucIPv6Addr,
				   prNDP->aucInterfaceId, IPV6MACLEN);
		rDataConfirmInd.u2Port = prNDP->u2PortNum;
		rDataConfirmInd.ucProtocol = prNDP->ucProtocolType;
	}
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rDataConfirmInd,
				sizeof(struct NanDataPathConfirmInd));
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
nanNdpSendDataTerminationEvent(struct ADAPTER *prAdapter,
			       struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct NanDataPathEndInd rDataEndInd;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	rDataEndInd.eventID = ENUM_NAN_DATA_TERMINATE;
	rDataEndInd.num_ndp_instances = 1;
	rDataEndInd.ndp_instance_id = prNDP->ucNDPID;
	kalIndicateNetlink2User(prAdapter->prGlueInfo, &rDataEndInd,
				sizeof(struct NanDataPathEndInd));
}

static struct STA_RECORD *nanGetDataPathStaRec(struct ADAPTER *prAdapter,
					struct _NAN_NDL_INSTANCE_T *prNDL,
					struct _NAN_NDP_INSTANCE_T *prNDP,
					uint8_t *pucLocalAddr,
					uint8_t **ppucPeerAddr,
					u_int8_t fgSearchFirstNdp)
{
	struct STA_RECORD *prStaRec;
	struct _NAN_NDP_CONTEXT_T *prNdpCxt;

	prStaRec = nanDataEngineSearchNDPContext(prAdapter, prNDL, pucLocalAddr,
						 *ppucPeerAddr);

	if (prStaRec && prStaRec->fgTransmitKeyExist == TRUE) {
		*ppucPeerAddr = prNDP->aucPeerNDIAddr;
		if (fgSearchFirstNdp) {
			prNdpCxt = nanDataEngineSearchFirstNDP(prAdapter, prNDL,
						pucLocalAddr, *ppucPeerAddr);
			if (prNdpCxt)
				*ppucPeerAddr = prNdpCxt->aucPeerNDIAddr;
		}
		prStaRec = nanDataEngineSearchNDPContext(prAdapter, prNDL,
						pucLocalAddr, *ppucPeerAddr);
		DBGLOG(NAN, DEBUG,
			"PeerNdiMacAddr["MACSTR"]\n", MAC2STR(*ppucPeerAddr));
	} else {
		DBGLOG(NAN, DEBUG,
			"PeerNmiMacAddr["MACSTR"]\n", MAC2STR(*ppucPeerAddr));
	}

	if (!prStaRec)
		DBGLOG(NAN, WARN, "NULL starec\n");

	return prStaRec;
}

static struct MSDU_INFO *nanCopyFromRetryMsdu(struct ADAPTER *prAdapter,
				struct MSDU_INFO *prRetryMsduInfo)
{
	struct MSDU_INFO *prMsduInfo = NULL;

	if (!prRetryMsduInfo)
		return prMsduInfo;

	prMsduInfo = cnmMgtPktAlloc(prAdapter, prRetryMsduInfo->u2FrameLength);

	if (prMsduInfo == NULL) {
		DBGLOG(NAN, WARN, "Can't alloc prMsduInfo\n");
		return prMsduInfo;
	}

	kalMemCopy(prMsduInfo->prPacket, prRetryMsduInfo->prPacket,
			   prRetryMsduInfo->u2FrameLength);
	prMsduInfo->u2FrameLength = prRetryMsduInfo->u2FrameLength;
	DBGLOG(NAN, DEBUG, "Copy %u bytes for retransmission\n",
		       prRetryMsduInfo->u2FrameLength);

	return prMsduInfo;
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
uint32_t nanNdpSendDataPathRequest(struct ADAPTER *prAdapter,
				   struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
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

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
	prStaRec = nanGetDataPathStaRec(prAdapter, prNDL, prNDP,
			     pucLocalAddr, &pucPeerAddr, TRUE);
	/* Copy from retry MSDU */
	prMsduInfo = nanCopyFromRetryMsdu(prAdapter, prNDP->prRetryMsduInfo);
	if (prMsduInfo)
		goto send;

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_DATA_PATH_REQUEST,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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

send:
	DBGLOG(NAN, DEBUG, "Estimated=%u bytes, u2FrameLengt=%u bytes\n",
	       u2EstimatedFrameLen, prMsduInfo->u2FrameLength);

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDPReqTxDone, prStaRec);
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
uint32_t nanNdpSendDataPathResponse(struct ADAPTER *prAdapter,
	struct _NAN_NDP_INSTANCE_T *prNDP,
	/* below are optional params, only valid when prNDP == NULL */
	uint8_t *pucDestMacAddr, struct _NAN_ATTR_NDP_T *prPeerAttrNDP,
	uint8_t ucNDPReasonCode, struct _NAN_ATTR_NDPE_T *prPeerAttrNDPE,
	uint8_t ucNDPEReasonCode, struct _NAN_ATTR_NDL_T *prPeerAttrNDL,
	uint8_t ucNDLReasonCode)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;

	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (prNDP)
		prNDL = nanDataUtilGetNdl(prAdapter, prNDP);
	else
		prNDL = NULL;

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	if (pucDestMacAddr)
		pucPeerAddr = pucDestMacAddr;
	else if (prNDL)
		pucPeerAddr = prNDL->aucPeerMacAddr;
	else {
		DBGLOG(NAN, ERROR, "pucDestMacAddr error\n");
		return WLAN_STATUS_INVALID_DATA;
	}
	prStaRec = nanGetDataPathStaRec(prAdapter, prNDL, prNDP,
					pucLocalAddr, &pucPeerAddr, FALSE);
	/* Copy from retry MSDU */
	if (prNDP) {
		prMsduInfo = nanCopyFromRetryMsdu(prAdapter,
						  prNDP->prRetryMsduInfo);
		if (prMsduInfo)
			goto send;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_DATA_PATH_RESPONSE,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	if (prNDP) {
		/* fill NAN attributes */
		for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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

		/* For MIC calculation */
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
				DBGLOG(NAN, DEBUG,
				       "ERROR! MIC calculation failed!\n");
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

send:
	DBGLOG(NAN, DEBUG, "Estimated=%u bytes, u2FrameLengt=%u bytes\n",
	       u2EstimatedFrameLen, prMsduInfo->u2FrameLength);

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDPRespTxDone, prStaRec);
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
uint32_t nanNdpSendDataPathConfirm(struct ADAPTER *prAdapter,
				   struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
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

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif
	prStaRec = nanGetDataPathStaRec(prAdapter, prNDL, prNDP,
					pucLocalAddr, &pucPeerAddr, FALSE);

	/* Copy from retry MSDU */
	prMsduInfo = nanCopyFromRetryMsdu(prAdapter, prNDP->prRetryMsduInfo);
	if (prMsduInfo)
		goto send;

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_DATA_PATH_CONFIRM,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	/* For MIC calculation */
	if (prNDP->fgSecurityRequired) {
		/* The body of Mx beginning from the Category */
		pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
			      OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		u1TxMsgLen = u2EstimatedFrameLen -
			     OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);

		rStatus = nanSecNotifyMsgBodyRdy(prNDP, NAN_SEC_M3, u1TxMsgLen,
						 pu1TxMsgBuf);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, DEBUG, "ERROR! MIC calculation failed!\n");
			return rStatus;
		}
	}

send:
	DBGLOG(NAN, DEBUG, "Estimated=%u bytes, u2FrameLengt=%u bytes\n",
	       u2EstimatedFrameLen, prMsduInfo->u2FrameLength);

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDPConfirmTxDone, prStaRec);
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
uint32_t nanNdpSendDataPathKeyInstall(struct ADAPTER *prAdapter,
				      struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen = 0;
	struct MSDU_INFO *prMsduInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pu1TxMsgBuf = NULL;
	uint16_t u1TxMsgLen = 0;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
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

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif
	prStaRec = nanGetDataPathStaRec(prAdapter, prNDL, prNDP,
					pucLocalAddr, &pucPeerAddr, FALSE);

	/* Copy from retry MSDU */
	prMsduInfo = nanCopyFromRetryMsdu(prAdapter, prNDP->prRetryMsduInfo);
	if (prMsduInfo)
		goto send;

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_DATA_PATH_KEY_INSTALLMENT,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	/* For MIC calculation */
	if (prNDP->fgSecurityRequired) {
		/* The body of Mx beginning from the Category */
		pu1TxMsgBuf = ((uint8_t *)prMsduInfo->prPacket) +
			      OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);
		u1TxMsgLen = u2EstimatedFrameLen -
			     OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory);

		rStatus = nanSecNotifyMsgBodyRdy(prNDP, NAN_SEC_M4, u1TxMsgLen,
						 pu1TxMsgBuf);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, DEBUG, "ERROR! MIC calculation failed!\n");
			return rStatus;
		}
	}

send:
	DBGLOG(NAN, DEBUG, "Estimated=%u bytes, u2FrameLengt=%u bytes\n",
	       u2EstimatedFrameLen, prMsduInfo->u2FrameLength);

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDPSecurityInstallTxDone, prStaRec);
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
uint32_t nanNdpSendDataPathTermination(struct ADAPTER *prAdapter,
				       struct _NAN_NDP_INSTANCE_T *prNDP)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL = nanDataUtilGetNdl(prAdapter, prNDP);

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

#if CFG_NAN_ACTION_FRAME_ADDR
	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;
#else
	pucLocalAddr = prNDP->aucLocalNDIAddr;
	pucPeerAddr = prNDP->aucPeerNDIAddr;
#endif

	prStaRec = nanDataEngineSearchNDPContext(
		prAdapter, prNDL, pucLocalAddr, pucPeerAddr);

	if (prStaRec && prStaRec->fgTransmitKeyExist == TRUE) {
		DBGLOG(NAN, DEBUG,
			"PeerNdiMacAddr["MACSTR"]\n",
			MAC2STR(prNDP->aucPeerNDIAddr));
		pucPeerAddr = prNDP->aucPeerNDIAddr;
		prStaRec = nanDataEngineSearchNDPContext(
			prAdapter, prNDL, pucLocalAddr,
			pucPeerAddr);
	} else {
		DBGLOG(NAN, DEBUG,
			"PeerNmiMacAddr["MACSTR"]\n",
			MAC2STR(pucPeerAddr));
	}

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_DATA_PATH_TERMINATION,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, prNDP) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, prNDP);
		}
	}

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDPTerminationTxDone, prStaRec);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief            Send NAF - Schedule Request
 *
 * \param[in]
 *
 * \return Status
 */
/*----------------------------------------------------------------------------*/
uint32_t nanNdlSendScheduleRequest(struct ADAPTER *prAdapter,
				   struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	prStaRec = nanDataEngineSearchNDPContext(
		prAdapter, prNDL, pucLocalAddr,
		pucPeerAddr);

	DBGLOG(NAN, DEBUG, "PeerMacAddr["MACSTR"]\n",
		MAC2STR(pucPeerAddr));

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_SCHEDULE_REQUEST, pucLocalAddr,
				      pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDataEngineScheduleReqTxDone, prStaRec);
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
uint32_t nanNdlSendScheduleResponse(struct ADAPTER *prAdapter,
				    struct _NAN_NDL_INSTANCE_T *prNDL,
	/* below are optional params, only valid when prNDL == NULL */
				    uint8_t *pucDestMacAddr,
				    struct _NAN_ATTR_NDL_T *prPeerAttrNDL,
				    uint8_t ucReasonCode)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	/* struct _NAN_ACTION_FRAME_T* prNAF; */
	/* struct _NAN_ATTR_NDL_T* prAttrNDL; */
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	if (prNDL && !EQUAL_MAC_ADDR(prNDL->aucTxRespAddr, aucZeroMacAddr)) {
		pucPeerAddr = prNDL->aucTxRespAddr;
	} else if (pucDestMacAddr) {
		pucPeerAddr = pucDestMacAddr;
	} else if (prNDL) {
		pucPeerAddr = prNDL->aucPeerMacAddr;
	} else {
		DBGLOG(NAN, ERROR, "[%s] aucPeerMacAddr error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prStaRec = nanDataEngineSearchNDPContext(
		prAdapter, prNDL, pucLocalAddr,
		pucPeerAddr);

	DBGLOG(NAN, DEBUG,
		"[%s] PeerMacAddr["MACSTR"]\n",
		__func__, MAC2STR(pucPeerAddr));

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_SCHEDULE_RESPONSE,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	if (prNDL) {
		/* fill NAN attributes */
		for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDataEngineScheduleRespTxDone, prStaRec);
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

uint32_t nanNdlSendScheduleConfirm(struct ADAPTER *prAdapter,
				   struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	prStaRec = nanDataEngineSearchNDPContext(
		prAdapter, prNDL, pucLocalAddr,
		pucPeerAddr);

	DBGLOG(NAN, DEBUG,
		"[%s] PeerMacAddr["MACSTR"]\n",
		__func__, MAC2STR(pucPeerAddr));

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_SCHEDULE_CONFIRM, pucLocalAddr,
				      pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	return nanDataEngineSendNAF(prAdapter,
				    prMsduInfo, prMsduInfo->u2FrameLength,
				    nanDataEngineScheduleConfirmTxDone,
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
uint32_t nanNdlSendScheduleUpdateNotify(struct ADAPTER *prAdapter,
					struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint32_t i;
	uint16_t u2EstimatedFrameLen;
	struct MSDU_INFO *prMsduInfo;
	struct STA_RECORD *prStaRec;
	uint8_t *pucLocalAddr;
	uint8_t *pucPeerAddr;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	u2EstimatedFrameLen =
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	/* estimate total length of NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
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
		       "NAN Data Engine: packet allocation failure: %s()\n",
		       __func__);
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prMsduInfo->prPacket, u2EstimatedFrameLen);

	pucLocalAddr = prAdapter->rDataPathInfo.aucLocalNMIAddr;
	pucPeerAddr = prNDL->aucPeerMacAddr;

	prStaRec = nanDataEngineSearchNDPContext(prAdapter, prNDL, pucLocalAddr,
						pucPeerAddr);

	nanDataEngineComposeNAFHeader(prAdapter, prMsduInfo,
				      NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION,
				      pucLocalAddr, pucPeerAddr, prStaRec);

	/* fill NAN attributes */
	for (i = 0; i < ARRAY_SIZE(txDataAttributeTable); i++) {
		if (txDataAttributeTable[i].pfnCalculateVariableAttrLen &&
		    txDataAttributeTable[i].pfnCalculateVariableAttrLen(
			    prAdapter, prNDL, NULL) != 0) {
			if (txDataAttributeTable[i].pfnAppendAttr)
				txDataAttributeTable[i].pfnAppendAttr(
					prAdapter, prMsduInfo, prNDL, NULL);
		}
	}

	return nanDataEngineSendNAF(prAdapter,
			    prMsduInfo, prMsduInfo->u2FrameLength,
			    nanDataEngineScheduleUpdateNotificationTxDone,
			    prStaRec);
}

static struct _NAN_NDL_INSTANCE_T *nanGetNdlByTxDoneActionFrame(
				struct ADAPTER *prAdapter,
				struct MSDU_INFO *prMsduInfo,
				uint32_t *u4Status,
				const char *func)
{
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;

	*u4Status = WLAN_STATUS_SUCCESS;
	if (!prMsduInfo) {
		DBGLOG(NAN, ERROR, "[%s] prMsduInfo error\n", __func__);
		*u4Status = WLAN_STATUS_INVALID_DATA;
		goto done;
	}

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for matching NDL */
	prNDL = nanDataUtilSearchNdlByMac(prAdapter, prNAF->aucDestAddr);

done:
	return prNDL;
}

static struct _NAN_NDP_INSTANCE_T *nanGetNdpByTxDoneActionFrame(
				struct ADAPTER *prAdapter,
				struct MSDU_INFO *prMsduInfo,
				struct _NAN_NDL_INSTANCE_T **prNDL,
				uint32_t *u4Status,
				const char *func)
{
	uint8_t *pucAttrList;
	uint16_t u2AttrListLength;
	struct _NAN_ATTR_NDP_T *prAttrNDP;
	struct _NAN_ATTR_NDPE_T *prAttrNDPE;
	struct _NAN_ACTION_FRAME_T *prNAF;
	struct _NAN_NDP_INSTANCE_T *prNDP = NULL;
	uint8_t ucNDPID;

	*u4Status = WLAN_STATUS_SUCCESS;

	*prNDL = nanGetNdlByTxDoneActionFrame(prAdapter, prMsduInfo, u4Status,
					      func);
	if (!*prNDL)
		goto done;

	prNAF = (struct _NAN_ACTION_FRAME_T *)prMsduInfo->prPacket;

	/* search for NDP-ID */
	pucAttrList = prNAF->aucInfoContent;
	u2AttrListLength = prMsduInfo->u2FrameLength -
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, aucInfoContent);

	prAttrNDP = (struct _NAN_ATTR_NDP_T *)nanRetrieveAttrById(pucAttrList,
				u2AttrListLength, NAN_ATTR_ID_NDP);

	prAttrNDPE = (struct _NAN_ATTR_NDPE_T *)nanRetrieveAttrById(pucAttrList,
				u2AttrListLength, NAN_ATTR_ID_NDP_EXTENSION);
	if (prAttrNDP) {
		ucNDPID = prAttrNDP->ucNDPID;
		DBGLOG(NAN, DEBUG, "[%s] prAttrNDP exist, NDPID=%u\n",
		       func, ucNDPID);
	} else if (prAttrNDPE) {
		ucNDPID = prAttrNDPE->ucNDPID;
		DBGLOG(NAN, DEBUG, "[%s] prAttrNDPE exist, NDPID=%u\n",
		       func, ucNDPID);
	} else {
		DBGLOG(NAN, ERROR, "[%s] Not found the NDPID\n", func);
		goto done;
	}
	prNDP = nanDataUtilSearchNdpByNdpId(prAdapter, *prNDL, ucNDPID);

done:
	return prNDP;
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
uint32_t nanDPReqTxDone(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentNDPProtocolState;
	uint32_t u4Status;
	u_int8_t fgNeedNotify = FALSE;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_DATA_PATH_REQUEST, rTxDoneStatus);

	prNDP = nanGetNdpByTxDoneActionFrame(prAdapter, prMsduInfo, &prNDL,
					     &u4Status, __func__);
	if (!prNDP)
		return u4Status;

	eCurrentNDPProtocolState = prNDP->eCurrentNDPProtocolState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
			&prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);
		fgNeedNotify = TRUE;

		if (eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_REQUEST) {
			if (prNDP->fgSecurityRequired) /* Notify SEC */
				nanSecTxKdeAttrDone(prNDP, NAN_SEC_M1);

			nanDataPathProtocolFsmStep(prAdapter,
					NDP_INITIATOR_RX_DP_RESPONSE, prNDP);
		}
	} else {
		if (eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_REQUEST) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT) {
				prNDP->prRetryMsduInfo = prMsduInfo;
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_INITIATOR_TX_DP_REQUEST, prNDP);
				prNDP->prRetryMsduInfo = NULL;
			} else {
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
				fgNeedNotify = TRUE;
			}
		}
	}

	/* Send rsp event to wifi hal */
	if (fgNeedNotify) {
		nanNdpInitiatorRspEvent(prAdapter, prNDP, rTxDoneStatus);
		DBGLOG(NAN, INFO, "NDP req event: %d\n", rTxDoneStatus);
	}

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
uint32_t nanDPRespTxDone(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentNDPProtocolState;
	uint32_t u4Status;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_DATA_PATH_RESPONSE, rTxDoneStatus);

	prNDP = nanGetNdpByTxDoneActionFrame(prAdapter, prMsduInfo, &prNDL,
					     &u4Status, __func__);
	if (!prNDP)
		return u4Status;

	eCurrentNDPProtocolState = prNDP->eCurrentNDPProtocolState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
			&prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);

		/* Notify framework before setpping to next NDP step  */
		DBGLOG(NAN, DEBUG, "NDP resp event: %d\n", rTxDoneStatus);
		nanNdpResponderRspEvent(prAdapter, prNDP, rTxDoneStatus);


		if (eCurrentNDPProtocolState == NDP_RESPONDER_TX_DP_RESPONSE) {
			if (prNDP->fgSecurityRequired) /* Notify SEC */
				nanSecTxKdeAttrDone(prNDP, NAN_SEC_M2);

			switch (prNDP->ucNDPSetupStatus) {
			case NAN_ATTR_NDP_STATUS_CONTINUED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_RESPONDER_RX_DP_CONFIRM, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_ACCEPTED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_NORMAL_TR, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_REJECTED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
				break;
			}
		}
	} else {
		if (eCurrentNDPProtocolState == NDP_RESPONDER_TX_DP_RESPONSE) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT) {
				prNDP->prRetryMsduInfo = prMsduInfo;
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_RESPONDER_TX_DP_RESPONSE, prNDP);
				prNDP->prRetryMsduInfo = NULL;
			} else {
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
				DBGLOG(NAN, DEBUG, "NDP resp event: %d\n",
				       rTxDoneStatus);
				nanNdpResponderRspEvent(prAdapter, prNDP,
							rTxDoneStatus);
			}
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
uint32_t nanDPConfirmTxDone(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo,
			    enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentNDPProtocolState;
	uint32_t u4Status;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, INFO, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_DATA_PATH_CONFIRM, rTxDoneStatus);

	prNDP = nanGetNdpByTxDoneActionFrame(prAdapter, prMsduInfo, &prNDL,
					     &u4Status, __func__);
	if (!prNDP)
		return u4Status;

	eCurrentNDPProtocolState = prNDP->eCurrentNDPProtocolState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
			&prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);

		if (eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_CONFIRM) {
			if (prNDP->fgSecurityRequired) /* Notify SEC */
				nanSecTxKdeAttrDone(prNDP, NAN_SEC_M3);

			switch (prNDP->ucNDPSetupStatus) {
			case NAN_ATTR_NDP_STATUS_CONTINUED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_INITIATOR_RX_DP_SECURITY_INSTALL,
					prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_ACCEPTED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_NORMAL_TR, prNDP);
				break;

			case NAN_ATTR_NDP_STATUS_REJECTED:
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
				break;
			}
		}
	} else {
		if (eCurrentNDPProtocolState == NDP_INITIATOR_TX_DP_CONFIRM) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT) {
				prNDP->prRetryMsduInfo = prMsduInfo;
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_INITIATOR_TX_DP_CONFIRM, prNDP);
				prNDP->prRetryMsduInfo = NULL;
			} else {
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
			}
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
nanDPSecurityInstallTxDone(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo,
			   enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	enum _ENUM_NDP_PROTOCOL_STATE_T eCurrentNDPProtocolState;
	uint32_t u4Status;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_DATA_PATH_KEY_INSTALLMENT, rTxDoneStatus);

	prNDP = nanGetNdpByTxDoneActionFrame(prAdapter, prMsduInfo, &prNDL,
					     &u4Status, __func__);
	if (!prNDP)
		return u4Status;

	eCurrentNDPProtocolState = prNDP->eCurrentNDPProtocolState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
			&prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);

		/* Notify SEC */
		if (prNDP->fgSecurityRequired)
			nanSecTxKdeAttrDone(prNDP, NAN_SEC_M4);

		if (eCurrentNDPProtocolState ==
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
		if (eCurrentNDPProtocolState ==
		    NDP_RESPONDER_TX_DP_SECURITY_INSTALL) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT) {
				prNDP->prRetryMsduInfo = prMsduInfo;
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_RESPONDER_TX_DP_SECURITY_INSTALL,
					prNDP);
				prNDP->prRetryMsduInfo = NULL;
			} else {
				nanDataPathProtocolFsmStep(
					prAdapter, NDP_DISCONNECT, prNDP);
			}
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
nanDPTerminationTxDone(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo,
		       enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	uint32_t u4Status;
#if NAF_REPORT_END_RSP_EVENT_TXDONE
	uint8_t fgSendNdpEndRsp = TRUE;
#endif

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_DATA_PATH_TERMINATION, rTxDoneStatus);

	prNDP = nanGetNdpByTxDoneActionFrame(prAdapter, prMsduInfo, &prNDL,
					     &u4Status, __func__);
	if (!prNDP)
		return u4Status;

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
			&prNDL->arNDPProtocolRetryTimer[NDP_IDX(prNDP)]);

		if (prNDP->eCurrentNDPProtocolState == NDP_TX_DP_TERMINATION)
			nanDataPathProtocolFsmStep(prAdapter, NDP_DISCONNECT,
						   prNDP);
	} else {
		if (prNDP->eCurrentNDPProtocolState == NDP_TX_DP_TERMINATION) {
			prNDP->ucTxRetryCounter++;

			if (prNDP->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT) {
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_TX_DP_TERMINATION, prNDP);
#if NAF_REPORT_END_RSP_EVENT_TXDONE
				fgSendNdpEndRsp = FALSE;
#endif
			} else {
				nanDataPathProtocolFsmStep(prAdapter,
					NDP_DISCONNECT, prNDP);
			}
		}
	}

	/* Send rsp event to wifi hal*/
#if NAF_REPORT_END_RSP_EVENT_TXDONE
	if (fgSendNdpEndRsp)
		nanNdpEndRspEvent(prAdapter, prNDP->eDataPathFailReason,
			prNDP->u2TransId, rTxDoneStatus);
#endif

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
uint32_t nanDataEngineScheduleReqTxDone(struct ADAPTER *prAdapter,
					struct MSDU_INFO *prMsduInfo,
					enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Status;
	enum _ENUM_NDL_MGMT_STATE_T eCurrentNDLMgmtState;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_SCHEDULE_REQUEST, rTxDoneStatus);

	prNDL = nanGetNdlByTxDoneActionFrame(prAdapter, prMsduInfo, &u4Status,
					     __func__);
	if (!prNDL)
		return u4Status;

	eCurrentNDLMgmtState = prNDL->eCurrentNDLMgmtState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer
				  [NAN_PROTOCOL_INITIATOR]);

		if (eCurrentNDLMgmtState == NDL_INITIATOR_TX_SCHEDULE_REQUEST)
			nanNdlMgmtFsmStep(
				prAdapter,
				NDL_INITIATOR_WAITFOR_RX_SCHEDULE_RESPONSE,
				prNDL);
		/* else
		 * unexpected state - ignore
		 */
	} else {
		if (eCurrentNDLMgmtState == NDL_INITIATOR_TX_SCHEDULE_REQUEST) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_SCHED_REQ_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_INITIATOR_TX_SCHEDULE_REQUEST,
					prNDL);
			else {
				/* If schedule request TX fail,
				 * recover nego slot and state before resched
				 */
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			}
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
uint32_t nanDataEngineScheduleRespTxDone(struct ADAPTER *prAdapter,
					 struct MSDU_INFO *prMsduInfo,
					 enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Status;
	enum _ENUM_NDL_MGMT_STATE_T eCurrentNDLMgmtState;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_SCHEDULE_RESPONSE, rTxDoneStatus);

	prNDL = nanGetNdlByTxDoneActionFrame(prAdapter, prMsduInfo, &u4Status,
					     __func__);
	if (!prNDL)
		return u4Status;

	eCurrentNDLMgmtState = prNDL->eCurrentNDLMgmtState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer
				  [NAN_PROTOCOL_RESPONDER]);

		if (eCurrentNDLMgmtState ==
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
				   NAN_ATTR_NDL_STATUS_REJECTED) {
/* Prevent NDL_TEARDOWN for availability in quality attribute */
#ifdef NAN_UNUSED
				nanNdlMgmtFsmStep(prAdapter, NDL_TEARDOWN,
						  prNDL);
#else
				/* If we reject peer schedule req,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);

				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
#endif
			}
			else {
				DBGLOG(NAN, ERROR,
					"[%s] ucNDLSetupCurrentStatus invalid\n",
					__func__);
				return WLAN_STATUS_INVALID_DATA;
			}
		}
	} else {
		if (eCurrentNDLMgmtState ==
		    NDL_RESPONDER_TX_SCHEDULE_RESPONSE) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_RESPONDER_TX_SCHEDULE_RESPONSE,
					prNDL);

			else {
				/* If schedule resp TX fail,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			}
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
uint32_t nanDataEngineScheduleConfirmTxDone(struct ADAPTER *prAdapter,
			struct MSDU_INFO *prMsduInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Status;
	enum _ENUM_NDL_MGMT_STATE_T eCurrentNDLMgmtState;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_SCHEDULE_CONFIRM, rTxDoneStatus);

	prNDL = nanGetNdlByTxDoneActionFrame(prAdapter, prMsduInfo, &u4Status,
					     __func__);
	if (!prNDL)
		return u4Status;

	eCurrentNDLMgmtState = prNDL->eCurrentNDLMgmtState;
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer
				  [NAN_PROTOCOL_INITIATOR]);

		if (eCurrentNDLMgmtState == NDL_INITIATOR_TX_SCHEDULE_CONFIRM) {
			if (prNDL->ucNDLSetupCurrentStatus ==
			    NAN_ATTR_NDL_STATUS_ACCEPTED) {
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			} else if (prNDL->ucNDLSetupCurrentStatus ==
				   NAN_ATTR_NDL_STATUS_REJECTED) {
				/* If we reject peer schedule resp,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			}
			else {
				DBGLOG(NAN, ERROR,
					"[%s] ucNDLSetupCurrentStatus invalid\n",
					__func__);
				return WLAN_STATUS_INVALID_DATA;
			}
		}
	} else {
		if (eCurrentNDLMgmtState == NDL_INITIATOR_TX_SCHEDULE_CONFIRM) {
			prNDL->ucTxRetryCounter++;

			if (prNDL->ucTxRetryCounter < NAN_DATA_RETRY_LIMIT)
				nanNdlMgmtFsmStep(
					prAdapter,
					NDL_INITIATOR_TX_SCHEDULE_CONFIRM,
					prNDL);

			else {
				/* If schedule confirm TX fail,
				 * use nego slot and state according to last
				 * peer availability. (R4 spec 6.2.4)
				 */
				DBGLOG(NAN, WARN,
				"Prevent NDL teardown for availability!\n");
				nanSchedNegoUpdateNegoResult(prAdapter);
				nanNdlMgmtFsmStep(prAdapter,
						  NDL_SCHEDULE_ESTABLISHED,
						  prNDL);
			}
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
	struct ADAPTER *prAdapter, struct MSDU_INFO *prMsduInfo,
	enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct _NAN_NDL_INSTANCE_T *prNDL;
	uint32_t u4Status;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, Status:%x\n", __func__, rTxDoneStatus);
#endif

	/* NAN_CHK_PNT log message */
	nanLogTxDone(NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION, rTxDoneStatus);

	prNDL = nanGetNdlByTxDoneActionFrame(prAdapter, prMsduInfo, &u4Status,
					     __func__);
	if (!prNDL)
		return u4Status;

	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer
				  [NAN_PROTOCOL_INITIATOR]);
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

static void nanLogTxNAF(struct ADAPTER *prAdapter,
			struct STA_RECORD *prSelectStaRec,
			uint8_t ucStaRecIndex, uint8_t ucWlanIndex,
			void *prPacket)
{
	struct _NAN_ACTION_FRAME_T *prNAF = prPacket;
	uint8_t ucOuiSubtype;

	ucOuiSubtype = prNAF->ucOUISubtype;

	if (!prAdapter->rWifiVar.fgNoPmf && (prSelectStaRec != NULL) &&
	    (prSelectStaRec->rPmfCfg.fgApplyPmf == TRUE)) {
		DBGLOG(NAN, INFO,
		       "Tx PMF, StaIdx:%d, OUItype:%d, OUISubtype:%d(%s), MAC=>"
		       MACSTR "\n",
		       prSelectStaRec->ucIndex,
		       prNAF->ucOUItype, ucOuiSubtype,
		       nanActionFrameOuiString(ucOuiSubtype),
		       MAC2STR(prSelectStaRec->aucMacAddr));
	}

	nanLogTx(prNAF); /* NAN_CHK_PNT log message */

	if (prNAF->ucOUItype == VENDOR_OUI_TYPE_NAN_NAF ||
	    prNAF->ucOUItype == VENDOR_OUI_TYPE_NAN_SDF) {
		DBGLOG(NAN, INFO,
		       "Tx NAN Pub Action, StaIdx:%d, Wtbl:%d, OUISubtype:%d(%s), Src: "
		       MACSTR " Dest: " MACSTR "\n",
		       ucStaRecIndex, ucWlanIndex,
		       ucOuiSubtype, nanActionFrameOuiString(ucOuiSubtype),
		       MAC2STR(prNAF->aucSrcAddr), MAC2STR(prNAF->aucDestAddr));
	}
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
uint32_t nanDataEngineSendNAF(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo, uint16_t u2FrameLength,
		     PFN_TX_DONE_HANDLER pfTxDoneHandler,
		     struct STA_RECORD *prSelectStaRec)
{
#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	/* 4 <3> Update information of MSDU_INFO_T */
	TX_SET_MMPDU(
		prAdapter, prMsduInfo,
		(prSelectStaRec != NULL)
			? (prSelectStaRec->ucBssIndex)
			: nanGetSpecificBssInfo(prAdapter, NAN_BSS_INDEX_BAND0)
				  ->ucBssIndex,
		(prSelectStaRec != NULL) ? (prSelectStaRec->ucIndex)
					 : (STA_REC_INDEX_NOT_FOUND),
		OFFSET_OF(struct _NAN_ACTION_FRAME_T, ucCategory),
		u2FrameLength, pfTxDoneHandler, MSDU_RATE_MODE_AUTO);

	prMsduInfo->ucTxToNafQueFlag = TRUE;

	nanLogTxNAF(prAdapter, prSelectStaRec,
		    prMsduInfo->ucStaRecIndex, prMsduInfo->ucWlanIndex,
		    prMsduInfo->prPacket);

	if (!prAdapter->rWifiVar.fgNoPmf && (prSelectStaRec != NULL) &&
	    (prSelectStaRec->rPmfCfg.fgApplyPmf == TRUE)) {
		nicTxConfigPktOption(prMsduInfo, MSDU_OPT_PROTECTED_FRAME,
				     TRUE);
	}

	nicTxSetPktRetryLimit(prMsduInfo, NAF_TX_RETRY_COUNT_LIMIT);

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
nanNdpUpdateTypeStatus(struct ADAPTER *prAdapter,
		       struct _NAN_NDP_INSTANCE_T *prNDP) {
	uint8_t ucType = 0;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
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
			"[%s] eCurrentNDPProtocolState invalid\n", __func__);
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
uint32_t nanNdpGenerateDialogToken(struct ADAPTER *prAdapter,
				   struct _NAN_NDP_INSTANCE_T *prNDP)
{
	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP->eNDPRole != NAN_PROTOCOL_INITIATOR) {
		DBGLOG(NAN, ERROR, "[%s] eNDPRole error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP->ucDialogToken < UINT8_MAX)
		prNDP->ucDialogToken++;

	else
		prNDP->ucDialogToken = 1; /* always non-zero */

	DBGLOG(NAN, DEBUG, "NDP->DialogToken=%u\n", prNDP->ucDialogToken);
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
uint32_t nanNdlGenerateDialogToken(struct ADAPTER *prAdapter,
				   struct _NAN_NDL_INSTANCE_T *prNDL)
{
	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDL->eNDLRole != NAN_PROTOCOL_INITIATOR) {
		DBGLOG(NAN, ERROR, "[%s] eNDLRole error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDL->ucDialogToken < UINT8_MAX)
		prNDL->ucDialogToken++;

	else
		prNDL->ucDialogToken = 1; /* always non-zero */

	DBGLOG(NAN, DEBUG, "NDL->DialogToken=%u\n", prNDL->ucDialogToken);
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
uint32_t nanDataEngineUpdateSSI(struct ADAPTER *prAdapter,
				struct _NAN_NDP_INSTANCE_T *prNDP,
				uint8_t ucServiceProtocolType,
				uint16_t u2ContextLen, uint8_t *pucContext)
{
	uint32_t rStatus;
	uint16_t u2Length = 0;
#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u2ContextLen <= 0) {
		DBGLOG(NAN, ERROR,
			"[%s] prNu2ContextLenDL invalid\n", __func__);
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
				DBGLOG(NAN, DEBUG, "PortNum %d\n",
				       prNDP->u2PortNum);
				break;
			}
			case NAN_ATTR_NDPE_SERVINFO_SUB_ATTR_PROTOCOL: {
				prNDP->ucProtocolType =
					prSubNanAttr->aucAttrBody[0];
				DBGLOG(NAN, DEBUG, "ucProtocolType %d\n",
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
				DBGLOG(NAN, DEBUG, "SPECIFIC INFO %d\n",
				       prNDP->u2AppInfoLen);
				break;
			}
			default: {
				DBGLOG(NAN, DEBUG,
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
nanDataEngineUpdateAppInfo(struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP,
			   uint8_t ucServiceProtocolType,
			   uint16_t u2AppInfoLen, uint8_t *pucAppInfo) {
	uint32_t rStatus;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (u2AppInfoLen <= 0) {
		DBGLOG(NAN, ERROR, "[%s] u2AppInfoLen invalid\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pucAppInfo) {
		DBGLOG(NAN, ERROR, "[%s] pucAppInfo error\n", __func__);
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
		DBGLOG(NAN, WARN, "%s(): AppInfo allocation failure (%d)\n",
		       __func__, u2AppInfoLen);

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
	struct ADAPTER *prAdapter, struct _NAN_NDP_INSTANCE_T *prNDP,
	struct _NAN_ATTR_NDPE_SVC_INFO_TLV_T *prAppInfoTLV) {
	uint16_t u2OtherAppInfoLen;

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDP) {
		DBGLOG(NAN, ERROR, "[%s] prNDP error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prAppInfoTLV) {
		DBGLOG(NAN, ERROR, "[%s] prAppInfoTLV error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prAppInfoTLV->u2Length < VENDOR_OUI_LEN) {
		DBGLOG(NAN, ERROR, "[%s] prAppInfoTLV len invalid\n", __func__);
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
			       "%s(): OtherAppInfo allocation failure (%d)\n",
			       __func__, u2OtherAppInfoLen);

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
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!ppucECAttr) {
		DBGLOG(NAN, ERROR, "[%s] ppucECAttr error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pu2ECAttrLength) {
		DBGLOG(NAN, ERROR, "[%s] pu2ECAttrLength error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = prAdapter->aprBssInfo[nanGetSpecificBssInfo(
#if (CFG_SUPPORT_NAN_DBDC == 1)
			prAdapter, NAN_BSS_INDEX_BAND1)->ucBssIndex];
#else
			prAdapter, NAN_BSS_INDEX_BAND0)->ucBssIndex];
#endif

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
#if (CFG_SUPPORT_802_11AX == 1)
	uint8_t ucHeCapLen, ucHeOpLen;
#endif
#if (CFG_SUPPORT_NAN_11BE == 1)
	uint8_t ucEhtCapLen, ucEhtOpLen;
#endif

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!ppucECAttr) {
		DBGLOG(NAN, ERROR, "[%s] ppucECAttr error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!pu2ECAttrLength) {
		DBGLOG(NAN, ERROR, "[%s] pu2ECAttrLength error\n", __func__);
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
		DBGLOG(NAN, DEBUG, "Bring VHT IE\n");
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

#if (CFG_SUPPORT_802_11AX == 1)
	DBGLOG(NAN, DEBUG, "Bring HE IE\n");
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
	DBGLOG(NAN, DEBUG, "Len HE Cap IE (%d)\n", ucHeCapLen);

	ucHeOpLen = heRlmFillNANHeOpIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
				ELEM_MAX_LEN_HT_CAP +
				ELEM_HDR_LEN +
				ELEM_MAX_LEN_VHT_CAP +
				ucHeCapLen]));

	u2AttrLength += ucHeOpLen;
	DBGLOG(NAN, DEBUG, "Len HE OP IE (%d)\n", ucHeOpLen);
#endif
#if (CFG_SUPPORT_NAN_11BE == 1)
	if (nanIsEhtEnable(prAdapter)) {
		DBGLOG(NAN, DEBUG, "Bring EHT IE\n");
		ucEhtCapLen = ehtRlmNANFillCapIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
			ELEM_MAX_LEN_HT_CAP +
			ELEM_HDR_LEN +
			ELEM_MAX_LEN_VHT_CAP +
			ucHeCapLen +
			ucHeOpLen]));
		prAdapter->rDataPathInfo.prLocalEhtCap =
			(struct IE_EHT_CAP_T *)
			&(prAttrEC->aucElements
			[ELEM_HDR_LEN +
			ELEM_MAX_LEN_HT_CAP +
			ELEM_HDR_LEN +
			ELEM_MAX_LEN_VHT_CAP +
			ucHeCapLen +
			ucHeOpLen]);

		u2AttrLength += ucEhtCapLen;
		DBGLOG(NAN, DEBUG, "Len EHT Cap IE (%d)\n",
			ucEhtCapLen);

		ucEhtOpLen = ehtRlmNANFillOpIE(
			prAdapter, prBssInfo,
			&(prAttrEC->aucElements[ELEM_HDR_LEN +
			ELEM_MAX_LEN_HT_CAP +
			ELEM_HDR_LEN +
			ELEM_MAX_LEN_VHT_CAP +
			ucHeCapLen +
			ucHeOpLen +
			ucEhtCapLen]));

		u2AttrLength += ucEhtOpLen;
		DBGLOG(NAN, DEBUG, "Len EHT OP IE (%d)\n",
			ucEhtOpLen);
	}
#endif

	prAttrEC->u2Length =
		u2AttrLength -
		OFFSET_OF(struct _NAN_ATTR_HDR_T,
				aucAttrBody);
	DBGLOG(NAN, DEBUG, "Len EC (%d)\n", prAttrEC->u2Length);

	prAdapter->rDataPathInfo.fgIsECSet = TRUE;

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
nanNdlDeactivateTimers(struct ADAPTER *prAdapter,
		       struct _NAN_NDL_INSTANCE_T *prNDL)
{

	size_t i;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* stop all timers for handsahking */
	cnmTimerStopTimer(prAdapter, &(prNDL->rNDPProtocolExpireTimer));
	for (i = 0; i < NAN_PROTOCOL_ROLE_NUM; i++) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDLProtocolReschRetryTimer[i]);
	}
	for (i = 0; i < NAN_MAX_SUPPORT_NDP_NUM; i++) {
		cnmTimerStopTimer(prAdapter,
				  &prNDL->arNDPProtocolRetryTimer[i]);
	}
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
nanDataEngineInsertRequest(struct ADAPTER *prAdapter,
			   struct _NAN_NDL_INSTANCE_T *prNDL,
			   enum _NAN_DATA_ENGINE_REQUEST_TYPE_T eRequestType,
			   enum _ENUM_NAN_PROTOCOL_ROLE_T eNDLRole,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prReq;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

#if (ENABLE_NDP_UT_LOG == 1)
	TRACE_FUNC(NAN, DEBUG, "[%s] Enter\n");
#endif

	prReq = cnmMemAlloc(prAdapter, RAM_TYPE_MSG,
			    sizeof(struct _NAN_DATA_ENGINE_REQUEST_T));

	if (prReq == NULL)
		return WLAN_STATUS_RESOURCES;

	prReq->eRequestType = eRequestType;
	prReq->prNDP = prNDP;
	prReq->eNDLRole = eNDLRole;

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
nanDataEngineFlushRequest(struct ADAPTER *prAdapter,
			  struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prReq;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
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
nanDataEngineGetNextRequest(struct ADAPTER *prAdapter,
			    struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prPendingReq;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "Enter\n");
#endif

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return NULL;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
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
	struct ADAPTER *prAdapter, struct _NAN_NDL_INSTANCE_T *prNDL,
	enum _NAN_DATA_ENGINE_REQUEST_TYPE_T eRequestType,
	struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct _NAN_DATA_ENGINE_REQUEST_T *prPendingReq, *prPendingReqNext;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL error\n", __func__);
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
nanDataEngineDisconnectByStaIdx(struct ADAPTER *prAdapter,
		uint8_t ucStaIdx,
		unsigned char fgTXNDPTermination) {
	struct STA_RECORD *prStaRec = NULL;
	struct _NAN_NDL_INSTANCE_T *prNDL;

#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "[%s] Enter, fgTXNDPTermination = %u\n",
	       __func__, fgTXNDPTermination);
#endif
	prStaRec = cnmGetStaRecByIndex(prAdapter, ucStaIdx);
	if (prStaRec == NULL) {
		DBGLOG(NAN, DEBUG, "Station Record Not Found\n");
		return;
	}
	prNDL = nanDataUtilSearchNdlByStaRec(prAdapter, prStaRec);
	nanDataEngineDisconnectByNdl(prAdapter, prNDL, fgTXNDPTermination);

}

void
nanDataEngingDisconnectEvt(struct ADAPTER *prAdapter,
		uint8_t *pcuEvtBuf) {
	struct _NAN_SCHED_EVENT_NDL_DISCONN_T *prNDLDisconn;
#if (CFG_SUPPORT_CONNAC3X == 1)
	uint8_t ucWlanIdx = 0;
#endif

	prNDLDisconn = (struct _NAN_SCHED_EVENT_NDL_DISCONN_T *)pcuEvtBuf;

#if (CFG_SUPPORT_CONNAC3X == 1)
	ucWlanIdx = prNDLDisconn->ucStaIdx;
	/* [Con3] NAN fw: sta idx == wlan idx,
	 * drv: sta idx != wlan idx
	 */
	if (wlanGetStaIdxByWlanIdx(prAdapter,
		ucWlanIdx, &prNDLDisconn->ucStaIdx)
		!= WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, DEBUG, "Station Record Not Found\n");
		return;
	}
#endif

	DBGLOG(NAN, DEBUG, "[%s] NDL Timeout, Reason:%u, Sta:%u\n", __func__,
	       prNDLDisconn->ucReason, prNDLDisconn->ucStaIdx);
	if (prNDLDisconn->ucReason == ENUM_NAN_NDL_DISCONNECT_BY_KEEP_ALIVE) {
		/* NAN_CHK_PNT log message */
		nanLogEndReason("keep_alive_timeout");
		nanDataEngineDisconnectByStaIdx(prAdapter,
						prNDLDisconn->ucStaIdx, FALSE);
	} else {
		/* NAN_CHK_PNT log message */
		nanLogEndReason("aging_timeout");
		nanDataEngineDisconnectByStaIdx(prAdapter,
						prNDLDisconn->ucStaIdx, TRUE);
	}
}

#if (CFG_SUPPORT_NAN_RESCHEDULE == 1)
uint32_t nanUpdateNdlScheduleV2(struct ADAPTER *prAdapter,
	struct _NAN_NDL_INSTANCE_T *prNDL)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
#if (ENABLE_NDP_UT_LOG == 1)
	DBGLOG(NAN, DEBUG, "Enter\n");
#endif
	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (!prNDL) {
		DBGLOG(NAN, ERROR,
			"[%s] prNDL error\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNDL->eNDLRole = NAN_PROTOCOL_INITIATOR;
	nanNdlMgmtFsmStep(prAdapter, NDL_REQUEST_SCHEDULE_NDL,
			prNDL);
	return rStatus;

}
#endif

const char *nanActionFrameOuiString(uint8_t subtype)
{
	static const char * const subtype_str[] = {
		[0] = "Reserved",
		[NAN_ACTION_RANGING_REQUEST] = "Ranging_Request",
		[NAN_ACTION_RANGING_RESPONSE] = "Ranging_Response",
		[NAN_ACTION_RANGING_TERMINATION] = "Ranging_Termination",
		[NAN_ACTION_RANGING_REPORT] = "Ranging_Report",
		[NAN_ACTION_DATA_PATH_REQUEST] = "Data_Path_Request",
		[NAN_ACTION_DATA_PATH_RESPONSE] = "Data_Path_Response",
		[NAN_ACTION_DATA_PATH_CONFIRM] = "Data_Path_Confirm",
		[NAN_ACTION_DATA_PATH_KEY_INSTALLMENT] =
			"Data_Path_Key_Install",
		[NAN_ACTION_DATA_PATH_TERMINATION] = "Data_Path_Termination",
		[NAN_ACTION_SCHEDULE_REQUEST] = "Schedule_Request",
		[NAN_ACTION_SCHEDULE_RESPONSE] = "Schedule_Response",
		[NAN_ACTION_SCHEDULE_CONFIRM] = "Schedule_Confirm",
		[NAN_ACTION_SCHEDULE_UPDATE_NOTIFICATION] =
			"Schedule_Update_Notification",
	};

	if (unlikely(subtype >= ARRAY_SIZE(subtype_str)))
		return "Reserved";
	else
		return subtype_str[subtype];
}

#endif /* CFG_SUPPORT_NAN */
