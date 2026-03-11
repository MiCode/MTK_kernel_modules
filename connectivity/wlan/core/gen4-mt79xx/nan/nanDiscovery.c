// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "precomp.h"
#include "nanDiscovery.h"
#include "wpa_supp/src/crypto/sha256.h"
#include "wpa_supp/src/crypto/sha256_i.h"
#include "wpa_supp/src/utils/common.h"

uint8_t g_ucInstanceID;
struct _NAN_DISC_ENGINE_T g_rNanDiscEngine;

struct _NAN_INSTANCE_T *
nanDiscInstanceSearch(struct ADAPTER *prAdapter,
	uint8_t ucID)
{
	uint8_t idx;

	for (idx = 0; idx < g_rNanDiscEngine.ucNumInstance; idx++) {
		if (g_rNanDiscEngine.arNanInstanceTbl[idx].ucInstanceID == ucID)
			return &g_rNanDiscEngine.arNanInstanceTbl[idx];
	}

	return NULL;
}

uint32_t
nanDiscInstanceCount(struct ADAPTER *prAdapter,
	enum NAN_INSTANCE_TYPE eType)
{
	uint8_t idx;
	uint32_t count = 0;

	for (idx = 0; idx < g_rNanDiscEngine.ucNumInstance; idx++) {
		if (g_rNanDiscEngine.arNanInstanceTbl[idx].eType == eType)
			count++;
	}

	return count;
}

int32_t
nanDiscInstanceAdd(struct ADAPTER *prAdapter,
	uint8_t ucID, enum NAN_INSTANCE_TYPE eType,
	uint8_t *pucServiceHashName, uint8_t *pucServiceName,
	uint8_t ucServiceNameLength)
{
	struct _NAN_INSTANCE_T *prInstance;

	prInstance = nanDiscInstanceSearch(prAdapter, ucID);
	if (!prInstance) {
		if (g_rNanDiscEngine.ucNumInstance >=
			NUM_OF_NAN_INSTANCE) {
			DBGLOG(NAN, ERROR,
				"[DISC] num of SD instance overflow\n");
			return -1;
		}

		prInstance = &g_rNanDiscEngine.arNanInstanceTbl[
				g_rNanDiscEngine.ucNumInstance];
		g_rNanDiscEngine.ucNumInstance++;
	} else {
		DBGLOG(NAN, INFO, "[DISC] override CID:%u, Type:%u\n",
			prInstance->ucInstanceID, prInstance->eType);
	}

	prInstance->ucInstanceID = ucID;
	prInstance->eType = eType;
	kalMemCopy(prInstance->aucServiceHash, pucServiceHashName,
		NAN_SERVICE_HASH_LENGTH);
	prInstance->ucServiceNameLength = ucServiceNameLength;
	if (prInstance->ucServiceNameLength > NAN_MAX_SERVICE_NAME_LEN)
		prInstance->ucServiceNameLength = NAN_MAX_SERVICE_NAME_LEN;
	kalMemCopy(prInstance->aucServiceName, pucServiceName,
		ucServiceNameLength);
	prInstance->aucServiceName[prInstance->ucServiceNameLength] = '\0';

	return 0;
}

int32_t
nanDiscInstanceDel(struct ADAPTER *prAdapter, uint8_t ucID)
{
	uint8_t idx;

	for (idx = 0; idx < g_rNanDiscEngine.ucNumInstance; idx++) {
		if (g_rNanDiscEngine.arNanInstanceTbl[idx].ucInstanceID ==
			ucID) {
			g_rNanDiscEngine.ucNumInstance--;
			kalMemCopy(&g_rNanDiscEngine.arNanInstanceTbl[idx],
				&g_rNanDiscEngine.arNanInstanceTbl[
					g_rNanDiscEngine.ucNumInstance],
				sizeof(struct _NAN_INSTANCE_T));
			break;
		}
	}

	return 0;
}

void
nanDiscInstanceResetAll(struct ADAPTER *prAdapter)
{
	g_rNanDiscEngine.ucNumInstance = 0;
	kalMemSet(g_rNanDiscEngine.arNanInstanceTbl, 0,
		sizeof(g_rNanDiscEngine.arNanInstanceTbl));
}

void
nanDiscInstanceQueryServiceList(struct ADAPTER *prAdapter,
	enum NAN_INSTANCE_TYPE eType,
	uint8_t *pucList, uint16_t *u2MaxListSize)
{
	struct _NAN_INSTANCE_T *prInstance;
	uint8_t idx;
	uint8_t num;

	if (pucList == NULL || u2MaxListSize == NULL)
		return;

	for (idx = 0, num = 0;
		idx < g_rNanDiscEngine.ucNumInstance &&
			num <= *u2MaxListSize; idx++) {
		prInstance = &g_rNanDiscEngine.arNanInstanceTbl[idx];
		if (prInstance->eType == eType) {
			pucList[num] = prInstance->ucInstanceID;
			num++;
		}
	}

	*u2MaxListSize = num;
}

void
nanConvertMatchFilter(uint8_t *pucFilterDst, size_t szFilterDstTotalLen,
		uint8_t *pucFilterSrc, uint8_t ucFilterSrcLen,
		uint16_t *pucFilterDstLen) {
	uint32_t u4Idx = 0;
	uint8_t ucLen = 0;
	uint16_t ucFilterLen = 0;
	uint8_t *pucFilterDstEndPos = pucFilterDst + szFilterDstTotalLen;

	if (ucFilterSrcLen == 0) {
		*pucFilterDstLen = 0;
		return;
	}

	for (u4Idx = 0; u4Idx < ucFilterSrcLen;) {
		/* skip comma */
		if (*pucFilterSrc == ',') {
			u4Idx++;
			pucFilterSrc++;
		}
		DBGLOG(INIT, INFO, "nan: filter[%d] =  %d\n", u4Idx,
		       pucFilterSrc);
		ucLen = atoi(*pucFilterSrc);
		if (ucLen == 0)
			continue;
		*pucFilterDst = ucLen;
		pucFilterDst++;
		u4Idx++;
		pucFilterSrc++;
		ucFilterLen++;
		/* skip comma */
		if (*pucFilterSrc == ',') {
			DBGLOG(INIT, INFO, "nan: skip comma%d\n", u4Idx);
			u4Idx++;
			pucFilterSrc++;
		}
		kalMemCpyS(pucFilterDst,
			(size_t)(pucFilterDstEndPos - pucFilterDst),
			pucFilterSrc, ucLen);
		pucFilterDst += ucLen;
		pucFilterSrc += ucLen;
		u4Idx += ucLen;
		ucFilterLen += ucLen;
	}

	*pucFilterDstLen = ucFilterLen;
}

void
nanConvertUccMatchFilter(uint8_t *pucFilterDst, size_t szFilterDstTotalLen,
		uint8_t *pucFilterSrc, uint8_t ucFilterSrcLen,
		uint16_t *pucFilterDstLen) {
	char *const delim = ",";
	uint8_t *pucfilter = NULL;
	uint32_t u4FilterLen = 0;
	uint32_t u4TotalLen = 0;
	uint8_t *pucFilterDstEndPos = pucFilterDst + szFilterDstTotalLen;

	if (ucFilterSrcLen == 0) {
		*pucFilterDstLen = 0;
		return;
	}

	DBGLOG(INIT, INFO, "ucFilterSrcLen %d\n", ucFilterSrcLen);
	if ((ucFilterSrcLen == 1) && (*pucFilterSrc == *delim)) {
		*pucFilterDstLen = 1;
		*pucFilterDst = 0;
		return;
	}

	/* skip last */
	if ((*(pucFilterSrc + ucFilterSrcLen - 1) == *delim) &&
	    (ucFilterSrcLen >= 2)) {
		DBGLOG(INIT, INFO, " erase last ','\n");
		*(pucFilterSrc + ucFilterSrcLen - 1) = 0;
	}
	while ((pucfilter = kalStrSep((char **)&pucFilterSrc, delim)) != NULL) {
		if (*pucfilter == '*') {
			DBGLOG(INIT, INFO, "met *, wildcard filter\n");
			*pucFilterDst = 0;
			u4TotalLen += 1;
			pucFilterDst += 1;
		} else {
			DBGLOG(INIT, INFO, "%s\n", pucfilter);
			u4FilterLen = kalStrLen(pucfilter);
			u4TotalLen += (u4FilterLen + 1);
			*(pucFilterDst) = u4FilterLen;
			kalMemCpyS((pucFilterDst + 1),
				(size_t)(pucFilterDstEndPos -
				(pucFilterDst + 1)),
				pucfilter, u4FilterLen);
			DBGLOG(INIT, INFO, "u4FilterLen: %d\n", u4FilterLen);
			dumpMemory8((uint8_t *)pucFilterDst, u4FilterLen + 1);
			pucFilterDst += (u4FilterLen + 1);
		}
	}
	*pucFilterDstLen = u4TotalLen;
}

uint32_t
nanCancelPublishRequest(struct ADAPTER *prAdapter,
			struct NanPublishCancelRequest *msg) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint16_t *pu2CancelPubID;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(uint16_t);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_CANCEL_PUBLISH, sizeof(uint16_t),
				      u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	pu2CancelPubID = (uint16_t *)prTlvElement->aucbody;
	*pu2CancelPubID = msg->publish_id;

	nanDiscInstanceDel(prAdapter, msg->publish_id);
	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
		    CMD_ID_NAN_EXT_CMD,   /* ucCID */
		    TRUE,		  /* fgSetQuery */
		    FALSE,		  /* fgNeedResp */
		    FALSE,		  /* fgIsOid */
		    NULL,		  /* pfCmdDoneHandler */
		    NULL,		  /* pfCmdTimeoutHandler */
		    u4CmdBufferLen,       /* u4SetQueryInfoLen */
		    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
		    NULL,		  /* pvSetQueryBuffer */
		    0 /* u4SetQueryBufferLen */);
	cnmMemFree(prAdapter, prCmdBuffer);
	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanUpdatePublishRequest(struct ADAPTER *prAdapter,
			struct NanPublishRequest *msg) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWPublishRequest *prPublishReq = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWPublishRequest);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_UPDATE_PUBLISH,
				      sizeof(struct NanFWPublishRequest),
				      u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prPublishReq = (struct NanFWPublishRequest *)prTlvElement->aucbody;
	kalMemZero(prPublishReq, sizeof(struct NanFWPublishRequest));
	prPublishReq->publish_id = msg->publish_id;

	DBGLOG(INIT, INFO, "nan: service_name_len = %d\n",
	       msg->service_name_len);
	prPublishReq->service_name_len = msg->service_name_len;
	kalMemCopy(prPublishReq->service_name, msg->service_name,
		   msg->service_name_len);

	DBGLOG(INIT, INFO, "nan: service_specific_info_len = %d\n",
	       prPublishReq->service_specific_info_len);
	prPublishReq->service_specific_info_len =
		msg->service_specific_info_len;
	if (prPublishReq->service_specific_info_len >
	    NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN)
		prPublishReq->service_specific_info_len =
			NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN;
	kalMemCopy(prPublishReq->service_specific_info,
		   msg->service_specific_info,
		   prPublishReq->service_specific_info_len);

	DBGLOG(INIT, INFO, "nan: sdea_service_specific_info_len = %d\n",
	       prPublishReq->sdea_service_specific_info_len);
	prPublishReq->sdea_service_specific_info_len =
		msg->sdea_service_specific_info_len;
	if (prPublishReq->sdea_service_specific_info_len >
	    NAN_FW_SDEA_SPECIFIC_INFO_LEN)
		prPublishReq->sdea_service_specific_info_len =
			NAN_FW_SDEA_SPECIFIC_INFO_LEN;
	kalMemCopy(prPublishReq->sdea_service_specific_info,
		   msg->sdea_service_specific_info,
		   prPublishReq->sdea_service_specific_info_len);

	/* send command to fw */
	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);
	cnmMemFree(prAdapter, prCmdBuffer);
	return WLAN_STATUS_SUCCESS;
}

void
nanSetPublishPmkid(struct ADAPTER *prAdapter, struct NanPublishRequest *msg) {

	int i = 0;
	char *I_mac = "ff:ff:ff:ff:ff:ff";
	u8 pmkid[32];
	u8 IMac[6];
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	struct BSS_INFO *prBssInfo;

	/* Get BSS info */
	prNanSpecificBssInfo = nanGetSpecificBssInfo(
		prAdapter, NAN_BSS_INDEX_MAIN);
	if (prNanSpecificBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prNanSpecificBssInfo is null\n");
		return;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(
		prAdapter, prNanSpecificBssInfo->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prBssInfo is null\n");
		return;
	}

	hwaddr_aton(I_mac, IMac);
	caculate_pmkid(msg->key_info.body.pmk_info.pmk,
		IMac, prBssInfo->aucOwnMacAddr,
		msg->service_name, pmkid);
	DBGLOG(NAN, LOUD, "[publish] SCID=>");
	for (i = 0 ; i < 15; i++)
		DBGLOG(NAN, LOUD, "%X:", pmkid[i]);

	DBGLOG(NAN, LOUD, "%X\n", pmkid[i]);
	msg->scid_len = 16;
	kalMemCopy(msg->scid, pmkid, 16);
}

uint32_t
nanPublishRequestExt(struct ADAPTER *prAdapter,
		uint8_t ucPublishId, struct NanPublishRequest *msg)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWPublishRequestExt *prPublishReqExt = NULL;

	/* Extension CMD for Publish */
	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWPublishRequestExt);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_PUBLISH_EXT,
				      sizeof(struct NanFWPublishRequestExt),
				      u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prPublishReqExt = (struct NanFWPublishRequestExt *)
		prTlvElement->aucbody;
	kalMemZero(prPublishReqExt, sizeof(struct NanFWPublishRequestExt));

	prPublishReqExt->ucVersion =
		NAN_VENDOR_REQUEST_VERSION_V1;
	prPublishReqExt->ucPublishId = ucPublishId;

	prPublishReqExt->fgAnnouncePeriodValid =
		msg->fgAnnouncePeriodValid;
	prPublishReqExt->u4AnnouncePeriod =
		msg->u4AnnouncePeriod;

	prPublishReqExt->fgDiscoveryRangeValid =
		msg->fgDiscoveryRangeValid;
	prPublishReqExt->i4DiscoveryRange =
		msg->i4DiscoveryRange;

	prPublishReqExt->fgSrvUpdateIndicatorValid =
		msg->fgSrvUpdateIndicatorValid;
	prPublishReqExt->ucSrvUpdateIndicator =
		msg->ucSrvUpdateIndicator;

	prPublishReqExt->fgMatchFilterTypeValid =
		msg->fgMatchFilterTypeValid;
	prPublishReqExt->ucMatchFilterType =
		msg->ucMatchFilterType;

	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);

	cnmMemFree(prAdapter, prCmdBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanPublishRequest(struct ADAPTER *prAdapter, struct NanPublishRequest *msg) {

	uint32_t rStatus = WLAN_STATUS_FAILURE;
	void *prCmdBuffer = NULL;
	uint32_t u4CmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWPublishRequest *prPublishReq = NULL;
	char aucServiceName[256] = {0};
	struct sha256_state r_SHA_256_state = {0};
	uint8_t auc_tk[32] = {0};
	uint32_t u4Idx = 0;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWPublishRequest);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_PUBLISH,
				      sizeof(struct NanFWPublishRequest),
				      u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prPublishReq = (struct NanFWPublishRequest *)prTlvElement->aucbody;
	kalMemZero(prPublishReq, sizeof(struct NanFWPublishRequest));

	if (nanDiscInstanceCount(prAdapter, NAN_PUBLISH) <
		NAN_MAX_PUBLISH_NUM) {
		if (msg->publish_id == 0)
			prPublishReq->publish_id = ++g_ucInstanceID;
		else
			prPublishReq->publish_id = msg->publish_id;
	} else {
		DBGLOG(NAN, INFO, "Exceed max number, allocate fail\n");
		prPublishReq->publish_id = 0;
		return prPublishReq->publish_id;
	}
	if (g_ucInstanceID == 255)
		g_ucInstanceID = 0;

	prPublishReq->tx_type = msg->tx_type;
	prPublishReq->publish_type = msg->publish_type;
	prPublishReq->cipher_type = msg->cipher_type;
	prPublishReq->ttl = msg->ttl;
	prPublishReq->rssi_threshold_flag = msg->rssi_threshold_flag;
	prPublishReq->recv_indication_cfg = msg->recv_indication_cfg;

	if (msg->service_name_len > NAN_FW_MAX_SERVICE_NAME_LEN) {
		DBGLOG(NAN, ERROR,
				"Service name length error:%d\n",
				msg->service_name_len);
		msg->service_name_len = NAN_FW_MAX_SERVICE_NAME_LEN;
	}

	prPublishReq->service_name_len = msg->service_name_len;
	kalMemCopy(prPublishReq->service_name, msg->service_name,
		   msg->service_name_len);

	if (!msg->fgServiceHashValid) {
		kalMemZero(aucServiceName, sizeof(aucServiceName));
		kalMemCopy(aucServiceName,
			   msg->service_name,
			   msg->service_name_len);
		aucServiceName[msg->service_name_len] = '\0';
		for (u4Idx = 0; u4Idx < kalStrLen(aucServiceName); u4Idx++) {
			if ((aucServiceName[u4Idx] >= 'A') &&
			    (aucServiceName[u4Idx] <= 'Z'))
				aucServiceName[u4Idx] =
					aucServiceName[u4Idx] + 32;
		}
		sha256_init(&r_SHA_256_state);
		sha256_process(&r_SHA_256_state, aucServiceName,
			       kalStrLen(aucServiceName));
		kalMemZero(auc_tk, sizeof(auc_tk));
		sha256_done(&r_SHA_256_state, auc_tk);
		kalMemCopy(prPublishReq->service_name_hash, auc_tk,
			   NAN_SERVICE_HASH_LENGTH);
		kalMemCopy(msg->aucServiceHash, prPublishReq->service_name_hash,
			   NAN_SERVICE_HASH_LENGTH);
		msg->fgServiceHashValid = 1;
	} else {
		kalMemCopy(prPublishReq->service_name_hash, msg->aucServiceHash,
			   NAN_SERVICE_HASH_LENGTH);
	}
	kalMemCopy(g_aucNanServiceId,
		   prPublishReq->service_name_hash, NAN_SERVICE_HASH_LENGTH);
	nanUtilDump(prAdapter, "service hash",
		    g_aucNanServiceId, NAN_SERVICE_HASH_LENGTH);

	prPublishReq->service_specific_info_len =
		msg->service_specific_info_len;
	if (prPublishReq->service_specific_info_len >
	    NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN)
		prPublishReq->service_specific_info_len =
			NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN;
	kalMemCopy(prPublishReq->service_specific_info,
		   msg->service_specific_info,
		   prPublishReq->service_specific_info_len);

	DBGLOG(NAN, INFO, "nan: service_specific_info_len = %d\n",
	       prPublishReq->service_specific_info_len);

	prPublishReq->sdea_service_specific_info_len =
		msg->sdea_service_specific_info_len;
	if (prPublishReq->sdea_service_specific_info_len >
	    NAN_FW_SDEA_SPECIFIC_INFO_LEN)
		prPublishReq->sdea_service_specific_info_len =
			NAN_FW_SDEA_SPECIFIC_INFO_LEN;
	kalMemCopy(prPublishReq->sdea_service_specific_info,
		   msg->sdea_service_specific_info,
		   prPublishReq->sdea_service_specific_info_len);

	DBGLOG(NAN, INFO, "nan: sdea_service_specific_info_len = %d\n",
	       prPublishReq->sdea_service_specific_info_len);

	prPublishReq->sdea_params.config_nan_data_path =
		msg->sdea_params.config_nan_data_path;
	prPublishReq->sdea_params.ndp_type = msg->sdea_params.ndp_type;
	prPublishReq->sdea_params.security_cfg = msg->sdea_params.security_cfg;
	prPublishReq->period = msg->period;
	prPublishReq->sdea_params.ranging_state =
		msg->sdea_params.ranging_state;
	prPublishReq->sdea_params.fgFSDRequire = msg->sdea_params.fgFSDRequire;
	//prPublishReq->sdea_params.fgFSDRequire = TRUE;
	//prPublishReq->sdea_params.security_cfg = TRUE;

	if (prAdapter->fgIsNANfromHAL == FALSE) {
		nanConvertUccMatchFilter(prPublishReq->tx_match_filter,
			sizeof_field(struct NanFWPublishRequest,
				tx_match_filter),
					 msg->tx_match_filter,
					 msg->tx_match_filter_len,
					 &prPublishReq->tx_match_filter_len);
		nanUtilDump(prAdapter, "tx match filter",
			    prPublishReq->tx_match_filter,
			    prPublishReq->tx_match_filter_len);
		nanConvertUccMatchFilter(prPublishReq->rx_match_filter,
			sizeof_field(struct NanFWPublishRequest,
				rx_match_filter),
					 msg->rx_match_filter,
					 msg->rx_match_filter_len,
					 &prPublishReq->rx_match_filter_len);

		nanUtilDump(prAdapter, "rx match filter",
			    prPublishReq->rx_match_filter,
			    prPublishReq->rx_match_filter_len);
	} else {
		/* fgIsNANfromHAL, then no need to convert match filter */
		prPublishReq->tx_match_filter_len = msg->tx_match_filter_len;
		kalMemCopy(prPublishReq->tx_match_filter, msg->tx_match_filter,
			   prPublishReq->tx_match_filter_len);
		nanUtilDump(prAdapter, "tx match filter",
			    prPublishReq->tx_match_filter,
			    prPublishReq->tx_match_filter_len);

		prPublishReq->rx_match_filter_len = msg->rx_match_filter_len;
		kalMemCopy(prPublishReq->rx_match_filter, msg->rx_match_filter,
			   prPublishReq->rx_match_filter_len);
		nanUtilDump(prAdapter, "rx match filter",
			    prPublishReq->rx_match_filter,
			    prPublishReq->rx_match_filter_len);
	}

	if (msg->fgNeedExtCmd)
		nanPublishRequestExt(prAdapter, prPublishReq->publish_id, msg);

	nanDiscServiceStart(prAdapter, prPublishReq->publish_id, TRUE);
	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);

	cnmMemFree(prAdapter, prCmdBuffer);

	return prPublishReq->publish_id;
}

uint32_t
nanTransmitRequestExt(struct ADAPTER *prAdapter,
		   struct NanTransmitFollowupRequest *msg)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWTransmitFollowupRequestExt *prTransmitReqExt = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWTransmitFollowupRequestExt);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(
		NAN_CMD_TRANSMIT_EXT,
		sizeof(struct NanFWTransmitFollowupRequestExt),
		u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTransmitReqExt =
		(struct NanFWTransmitFollowupRequestExt *)prTlvElement->aucbody;
	kalMemZero(prTransmitReqExt,
		sizeof(struct NanFWTransmitFollowupRequestExt));

	prTransmitReqExt->ucVersion = NAN_VENDOR_REQUEST_VERSION_V1;
	prTransmitReqExt->u2PublishSubscribeId = msg->publish_subscribe_id;
	prTransmitReqExt->u4RequestorInstanceId = msg->requestor_instance_id;

	prTransmitReqExt->fgFollowUpTokenValid = msg->fgFollowUpTokenValid;
	prTransmitReqExt->u2FollowUpToken = msg->u2FollowUpToken;

	/* send command to fw */
	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);

	cnmMemFree(prAdapter, prCmdBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanTransmitRequest(struct ADAPTER *prAdapter,
		   struct NanTransmitFollowupRequest *msg) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWTransmitFollowupRequest *prTransmitReq = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWTransmitFollowupRequest);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(
		NAN_CMD_TRANSMIT, sizeof(struct NanFWTransmitFollowupRequest),
		u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTransmitReq =
		(struct NanFWTransmitFollowupRequest *)prTlvElement->aucbody;
	prTransmitReq->publish_subscribe_id = msg->publish_subscribe_id;
	prTransmitReq->requestor_instance_id = msg->requestor_instance_id;
	kalMemCopy(prTransmitReq->addr, msg->addr, MAC_ADDR_LEN);

	prTransmitReq->service_specific_info_len =
		msg->service_specific_info_len;
	if (prTransmitReq->service_specific_info_len >
	    NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN)
		prTransmitReq->service_specific_info_len =
			NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN;
	kalMemCopy(prTransmitReq->service_specific_info,
		   msg->service_specific_info,
		   prTransmitReq->service_specific_info_len);

	prTransmitReq->sdea_service_specific_info_len =
		MIN(sizeof(prTransmitReq->sdea_service_specific_info),
		msg->sdea_service_specific_info_len);

	kalMemCopy(prTransmitReq->sdea_service_specific_info,
		   msg->sdea_service_specific_info,
		   prTransmitReq->sdea_service_specific_info_len);

	DBGLOG(CNM, INFO,
	       "[%s]: publish_subscribe_id: %d, requestor_instance_id: %d\n",
	       __func__, prTransmitReq->publish_subscribe_id,
	       prTransmitReq->requestor_instance_id);
	DBGLOG(NAN, INFO,
	       "TransmitReq->addr=> " MACSTR "\n",
	       MAC2STR(prTransmitReq->addr));

	if (msg->fgNeedExtCmd)
		nanTransmitRequestExt(prAdapter, msg);

	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);

	cnmMemFree(prAdapter, prCmdBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanCancelSubscribeRequest(struct ADAPTER *prAdapter,
			  struct NanSubscribeCancelRequest *msg) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	uint16_t *pu2CancelSubID;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(uint16_t);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_CANCEL_SUBSCRIBE,
			sizeof(uint16_t), u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	pu2CancelSubID = (uint16_t *)prTlvElement->aucbody;
	*pu2CancelSubID = msg->subscribe_id;
	nanDiscInstanceDel(prAdapter, msg->subscribe_id);
	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
		    CMD_ID_NAN_EXT_CMD,   /* ucCID */
		    TRUE,		  /* fgSetQuery */
		    FALSE,		  /* fgNeedResp */
		    FALSE,		  /* fgIsOid */
		    NULL,		  /* pfCmdDoneHandler */
		    NULL,		  /* pfCmdTimeoutHandler */
		    u4CmdBufferLen,       /* u4SetQueryInfoLen */
		    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
		    NULL,		  /* pvSetQueryBuffer */
		    0 /* u4SetQueryBufferLen */);
	cnmMemFree(prAdapter, prCmdBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSubscribeRequestExt(struct ADAPTER *prAdapter,
		    uint8_t ucSubscribeId, struct NanSubscribeRequest *msg)
{
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWSubscribeRequestExt *prSubscribeReqExt = NULL;

	/* Extension CMD for Subscribe */
	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWSubscribeRequestExt);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_SUBSCRIBE_EXT,
				      sizeof(struct NanFWSubscribeRequestExt),
				      u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}

	prSubscribeReqExt = (struct NanFWSubscribeRequestExt *)
		prTlvElement->aucbody;
	kalMemZero(prSubscribeReqExt,
		sizeof(struct NanFWPublishRequestExt));

	prSubscribeReqExt->ucVersion = NAN_VENDOR_REQUEST_VERSION_V1;
	prSubscribeReqExt->ucSubscribeId = ucSubscribeId;

	prSubscribeReqExt->fgQueryPeriodValid = msg->fgQueryPeriodValid;
	prSubscribeReqExt->u4QueryPeriod = msg->u4QueryPeriod;

	prSubscribeReqExt->fgDiscoveryRangeValid = msg->fgDiscoveryRangeValid;
	prSubscribeReqExt->i4DiscoveryRange = msg->i4DiscoveryRange;

	prSubscribeReqExt->fgMatchFilterTypeValid = msg->fgMatchFilterTypeValid;
	prSubscribeReqExt->ucMatchFilterType = msg->ucMatchFilterType;

	prSubscribeReqExt->u4BloomFilterLength = msg->u4BloomFilterLength;
	prSubscribeReqExt->ucBloomIdx = msg->ucBloomIdx;
	kalMemCopy(prSubscribeReqExt->aucBloomFilter,
		msg->aucBloomFilter,
		prSubscribeReqExt->u4BloomFilterLength);

	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);
	cnmMemFree(prAdapter, prCmdBuffer);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanSubscribeRequest(struct ADAPTER *prAdapter,
		    struct NanSubscribeRequest *msg) {
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	void *prCmdBuffer = NULL;
	uint32_t u4CmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct NanFWSubscribeRequest *prSubscribeReq = NULL;

	char aucServiceName[256] = {0};
	struct sha256_state r_SHA_256_state = {0};
	uint8_t auc_tk[32] = {0};
	uint32_t u4Idx = 0;
	uint32_t u4subscribeId = 0;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct NanFWSubscribeRequest);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_SUBSCRIBE,
				      sizeof(struct NanFWSubscribeRequest),
				      u4CmdBufferLen, prCmdBuffer);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(TX, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);

	if (prTlvElement == NULL) {
		DBGLOG(TX, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return WLAN_STATUS_FAILURE;
	}
	prSubscribeReq = (struct NanFWSubscribeRequest *)prTlvElement->aucbody;
	if (nanDiscInstanceCount(prAdapter, NAN_SUBSCRIBE) <
		NAN_MAX_SUBSCRIBE_NUM) {
		if (msg->subscribe_id == 0)
			prSubscribeReq->subscribe_id = ++g_ucInstanceID;
		else
			prSubscribeReq->subscribe_id = msg->subscribe_id;
	} else {
		prSubscribeReq->subscribe_id = 0;
		return prSubscribeReq->subscribe_id;
	}
	if (g_ucInstanceID == 255)
		g_ucInstanceID = 0;

	prSubscribeReq->subscribe_type = msg->subscribe_type;
	prSubscribeReq->ttl = msg->ttl;
	prSubscribeReq->rssi_threshold_flag = msg->rssi_threshold_flag;
	prSubscribeReq->recv_indication_cfg = msg->recv_indication_cfg;
	prSubscribeReq->period = msg->period;

	if (msg->service_name_len > NAN_FW_MAX_SERVICE_NAME_LEN) {
		DBGLOG(NAN, ERROR,
				"Service name length error:%d\n",
				msg->service_name_len);
		msg->service_name_len = NAN_FW_MAX_SERVICE_NAME_LEN;
	}
	prSubscribeReq->service_name_len = msg->service_name_len;
	kalMemCopy(prSubscribeReq->service_name, msg->service_name,
		   msg->service_name_len);

	if (!msg->fgServiceHashValid) {
		kalMemZero(aucServiceName, sizeof(aucServiceName));
		kalMemCopy(aucServiceName,
			   msg->service_name,
			   msg->service_name_len);
		aucServiceName[msg->service_name_len] = '\0';
		for (u4Idx = 0; u4Idx < kalStrLen(aucServiceName); u4Idx++) {
			if ((aucServiceName[u4Idx] >= 'A') &&
			    (aucServiceName[u4Idx] <= 'Z'))
				aucServiceName[u4Idx] =
					aucServiceName[u4Idx] + 32;
		}
		sha256_init(&r_SHA_256_state);
		sha256_process(&r_SHA_256_state, aucServiceName,
			       kalStrLen(aucServiceName));
		kalMemZero(auc_tk, sizeof(auc_tk));
		sha256_done(&r_SHA_256_state, auc_tk);
		kalMemCopy(prSubscribeReq->service_name_hash, auc_tk,
			   NAN_SERVICE_HASH_LENGTH);
		kalMemCopy(msg->aucServiceHash,
			prSubscribeReq->service_name_hash,
			   NAN_SERVICE_HASH_LENGTH);
		msg->fgServiceHashValid = 1;
	} else {
		kalMemCopy(prSubscribeReq->service_name_hash,
			msg->aucServiceHash,
			NAN_SERVICE_HASH_LENGTH);
	}
	kalMemCopy(g_aucNanServiceId,
		   prSubscribeReq->service_name_hash, NAN_SERVICE_HASH_LENGTH);
	nanUtilDump(prAdapter, "service hash",
		g_aucNanServiceId, NAN_SERVICE_HASH_LENGTH);

	prSubscribeReq->service_specific_info_len =
		msg->service_specific_info_len;
	if (prSubscribeReq->service_specific_info_len >
	    NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN)
		prSubscribeReq->service_specific_info_len =
			NAN_FW_MAX_SERVICE_SPECIFIC_INFO_LEN;
	kalMemCopy(prSubscribeReq->service_specific_info,
		   msg->service_specific_info,
		   prSubscribeReq->service_specific_info_len);

	prSubscribeReq->sdea_service_specific_info_len =
		msg->sdea_service_specific_info_len;
	if (prSubscribeReq->sdea_service_specific_info_len >
	    NAN_FW_SDEA_SPECIFIC_INFO_LEN)
		prSubscribeReq->sdea_service_specific_info_len =
			NAN_FW_SDEA_SPECIFIC_INFO_LEN;
	DBGLOG(INIT, INFO, "nan: sdea_service_specific_info_len = %d\n",
	       prSubscribeReq->sdea_service_specific_info_len);
	kalMemCopy(prSubscribeReq->sdea_service_specific_info,
		   msg->sdea_service_specific_info,
		   prSubscribeReq->sdea_service_specific_info_len);

	if (prAdapter->fgIsNANfromHAL == FALSE) {
		nanConvertUccMatchFilter(prSubscribeReq->tx_match_filter,
			sizeof_field(struct NanFWSubscribeRequest,
				tx_match_filter),
					 msg->tx_match_filter,
					 msg->tx_match_filter_len,
					 &prSubscribeReq->tx_match_filter_len);
		nanUtilDump(prAdapter, "tx match filter",
			    prSubscribeReq->tx_match_filter,
			    prSubscribeReq->tx_match_filter_len);

		nanConvertUccMatchFilter(prSubscribeReq->rx_match_filter,
			sizeof_field(struct NanFWSubscribeRequest,
				rx_match_filter),
					 msg->rx_match_filter,
					 msg->rx_match_filter_len,
					 &prSubscribeReq->rx_match_filter_len);
		nanUtilDump(prAdapter, "rx match filter",
			    prSubscribeReq->rx_match_filter,
			    prSubscribeReq->rx_match_filter_len);
	} else {
		/* fgIsNANfromHAL, then no need to convert match filter */
		prSubscribeReq->tx_match_filter_len = msg->tx_match_filter_len;
		kalMemCopy(prSubscribeReq->tx_match_filter,
			   msg->tx_match_filter,
			   prSubscribeReq->tx_match_filter_len);
		nanUtilDump(prAdapter, "tx match filter",
			    prSubscribeReq->tx_match_filter,
			    prSubscribeReq->tx_match_filter_len);

		prSubscribeReq->rx_match_filter_len = msg->rx_match_filter_len;
		kalMemCopy(prSubscribeReq->rx_match_filter,
			   msg->rx_match_filter,
			   prSubscribeReq->rx_match_filter_len);
		nanUtilDump(prAdapter, "rx match filter",
			    prSubscribeReq->rx_match_filter,
			    prSubscribeReq->rx_match_filter_len);
	}

	prSubscribeReq->serviceResponseFilter = msg->serviceResponseFilter;
	prSubscribeReq->serviceResponseInclude = msg->serviceResponseInclude;
	prSubscribeReq->useServiceResponseFilter =
		msg->useServiceResponseFilter;
	prSubscribeReq->num_intf_addr_present = msg->num_intf_addr_present;
	kalMemCopy(prSubscribeReq->intf_addr, msg->intf_addr,
		   msg->num_intf_addr_present * MAC_ADDR_LEN);

	nanDiscServiceStart(prAdapter, prSubscribeReq->subscribe_id, FALSE);

	if (msg->fgNeedExtCmd)
		nanSubscribeRequestExt(prAdapter,
			prSubscribeReq->subscribe_id, msg);

	wlanSendSetQueryCmd(prAdapter,		  /* prAdapter */
			    CMD_ID_NAN_EXT_CMD,   /* ucCID */
			    TRUE,		  /* fgSetQuery */
			    FALSE,		  /* fgNeedResp */
			    FALSE,		  /* fgIsOid */
			    NULL,		  /* pfCmdDoneHandler */
			    NULL,		  /* pfCmdTimeoutHandler */
			    u4CmdBufferLen,       /* u4SetQueryInfoLen */
			    (uint8_t *)prCmdBuffer, /* pucInfoBuffer */
			    NULL,		  /* pvSetQueryBuffer */
			    0 /* u4SetQueryBufferLen */);
	u4subscribeId = prSubscribeReq->subscribe_id;

	cnmMemFree(prAdapter, prCmdBuffer);

	return u4subscribeId;
}

void
nanCmdAddCsid(IN struct ADAPTER *prAdapter, uint8_t ucPubID, uint8_t ucNumCsid,
	      uint8_t *pucCsidList) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_DISC_CMD_ADD_CSID_T *prCmdAddCsid = NULL;
	uint8_t ucIdx;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_DISC_CMD_ADD_CSID_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(NAN_CMD_ADD_CSID,
				      sizeof(struct _NAN_DISC_CMD_ADD_CSID_T),
				      u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmdAddCsid = (struct _NAN_DISC_CMD_ADD_CSID_T *)prTlvElement->aucbody;
	prCmdAddCsid->ucPubID = ucPubID;
	if (ucNumCsid > NAN_MAX_CIPHER_SUITE_NUM)
		ucNumCsid = NAN_MAX_CIPHER_SUITE_NUM;
	prCmdAddCsid->ucNum = ucNumCsid;
	for (ucIdx = 0; ucIdx < ucNumCsid; ucIdx++) {
		prCmdAddCsid->aucSupportedCSID[ucIdx] = *pucCsidList;
		pucCsidList++;
	}

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, NULL, u4CmdBufferLen,
				      (uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

void
nanCmdManageScid(IN struct ADAPTER *prAdapter, unsigned char fgAddDelete,
		 uint8_t ucPubID, uint8_t *pucScid) {
	uint32_t rStatus;
	void *prCmdBuffer;
	uint32_t u4CmdBufferLen;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_DISC_CMD_MANAGE_SCID_T *prCmdManageScid = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_DISC_CMD_MANAGE_SCID_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);
	if (!prCmdBuffer) {
		DBGLOG(NAN, ERROR, "Memory allocation fail\n");
		return;
	}

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;
	prTlvCommon->u2TotalElementNum = 0;

	rStatus = nicAddNewTlvElement(
		NAN_CMD_MANAGE_SCID, sizeof(struct _NAN_DISC_CMD_MANAGE_SCID_T),
		u4CmdBufferLen, prCmdBuffer);
	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "Add new Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prTlvElement = nicGetTargetTlvElement(1, prCmdBuffer);
	if (prTlvElement == NULL) {
		DBGLOG(NAN, ERROR, "Get target Tlv element fail\n");
		cnmMemFree(prAdapter, prCmdBuffer);
		return;
	}

	prCmdManageScid =
		(struct _NAN_DISC_CMD_MANAGE_SCID_T *)prTlvElement->aucbody;
	prCmdManageScid->fgAddDelete = fgAddDelete;
	prCmdManageScid->ucPubID = ucPubID;
	kalMemCopy(prCmdManageScid->aucSCID, pucScid, NAN_SCID_DEFAULT_LEN);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL, NULL, u4CmdBufferLen,
				      (uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);
}

void
nanDiscResetServiceSession(struct ADAPTER *prAdapter,
			   struct _NAN_SERVICE_SESSION_T *prServiceSession) {
	prServiceSession->ucNumCSID = 0;
	prServiceSession->ucNumSCID = 0;
}

struct _NAN_SERVICE_SESSION_T *
nanDiscAcquireServiceSession(struct ADAPTER *prAdapter,
			     uint8_t *pucPublishNmiAddr, uint8_t ucPubID) {
	struct LINK *rSeviceSessionList;
	struct LINK *rFreeServiceSessionList;
	struct _NAN_SERVICE_SESSION_T *prServiceSession;
	struct _NAN_SERVICE_SESSION_T *prServiceSessionNext;
	struct _NAN_SERVICE_SESSION_T *prAgingServiceSession;
	struct _NAN_DISC_ENGINE_T *prDiscEngine = &g_rNanDiscEngine;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return NULL;
	}
	rSeviceSessionList = &prDiscEngine->rSeviceSessionList;
	rFreeServiceSessionList = &prDiscEngine->rFreeServiceSessionList;
	prAgingServiceSession = NULL;

	LINK_FOR_EACH_ENTRY_SAFE(prServiceSession, prServiceSessionNext,
				 rSeviceSessionList, rLinkEntry,
				 struct _NAN_SERVICE_SESSION_T) {
		if (EQUAL_MAC_ADDR(prServiceSession->aucPublishNmiAddr,
				   pucPublishNmiAddr) &&
		    (prServiceSession->ucPublishID == ucPubID)) {
			LINK_REMOVE_KNOWN_ENTRY(rSeviceSessionList,
						prServiceSession);

			LINK_INSERT_TAIL(rSeviceSessionList,
					 &prServiceSession->rLinkEntry);
			return prServiceSession;
		} else if (prAgingServiceSession == NULL)
			prAgingServiceSession = prServiceSession;
	}

	LINK_REMOVE_HEAD(rFreeServiceSessionList, prServiceSession,
			 struct _NAN_SERVICE_SESSION_T *);
	if (prServiceSession) {
		kalMemZero(prServiceSession,
			   sizeof(struct _NAN_SERVICE_SESSION_T));
		kalMemCopy(prServiceSession->aucPublishNmiAddr,
			   pucPublishNmiAddr, MAC_ADDR_LEN);
		nanDiscResetServiceSession(prAdapter, prServiceSession);

		LINK_INSERT_TAIL(rSeviceSessionList,
				 &prServiceSession->rLinkEntry);
	} else if (prAgingServiceSession) {
		LINK_REMOVE_KNOWN_ENTRY(rSeviceSessionList,
					prAgingServiceSession);

		kalMemZero(prAgingServiceSession,
			   sizeof(struct _NAN_SERVICE_SESSION_T));
		kalMemCopy(prAgingServiceSession->aucPublishNmiAddr,
			   pucPublishNmiAddr, MAC_ADDR_LEN);
		nanDiscResetServiceSession(prAdapter, prAgingServiceSession);

		LINK_INSERT_TAIL(rSeviceSessionList,
				 &prAgingServiceSession->rLinkEntry);
		return prAgingServiceSession;
	}

	return prServiceSession;
}

void
nanDiscReleaseAllServiceSession(struct ADAPTER *prAdapter) {
	struct LINK *rSeviceSessionList;
	struct LINK *rFreeServiceSessionList;
	struct _NAN_SERVICE_SESSION_T *prServiceSession;
	struct _NAN_DISC_ENGINE_T *prDiscEngine = &g_rNanDiscEngine;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return;
	}
	rSeviceSessionList = &prDiscEngine->rSeviceSessionList;
	rFreeServiceSessionList = &prDiscEngine->rFreeServiceSessionList;

	while (!LINK_IS_EMPTY(rSeviceSessionList)) {
		LINK_REMOVE_HEAD(rSeviceSessionList, prServiceSession,
				 struct _NAN_SERVICE_SESSION_T *);
		if (prServiceSession) {
			kalMemZero(prServiceSession, sizeof(*prServiceSession));
			LINK_INSERT_TAIL(rFreeServiceSessionList,
					 &prServiceSession->rLinkEntry);
		} else {
			/* should not deliver to this function */
			DBGLOG(NAN, ERROR, "prServiceSession error!\n");
			return;
		}
	}
}

struct _NAN_SERVICE_SESSION_T *
nanDiscSearchServiceSession(struct ADAPTER *prAdapter,
			    uint8_t *pucPublishNmiAddr, uint8_t ucPubID) {
	struct LINK *rSeviceSessionList;
	struct _NAN_SERVICE_SESSION_T *prServiceSession;
	struct _NAN_DISC_ENGINE_T *prDiscEngine = &g_rNanDiscEngine;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return NULL;
	}

	rSeviceSessionList = &prDiscEngine->rSeviceSessionList;

	LINK_FOR_EACH_ENTRY(prServiceSession, rSeviceSessionList, rLinkEntry,
			    struct _NAN_SERVICE_SESSION_T) {
		if (EQUAL_MAC_ADDR(prServiceSession->aucPublishNmiAddr,
				   pucPublishNmiAddr) &&
		    (prServiceSession->ucPublishID == ucPubID)) {
			return prServiceSession;
		}
	}

	return NULL;
}

uint32_t
nanDiscInit(struct ADAPTER *prAdapter) {
	uint32_t u4Idx;
	struct _NAN_DISC_ENGINE_T *prDiscEngine = &g_rNanDiscEngine;

	if (prDiscEngine->fgInit == FALSE) {
		LINK_INITIALIZE(&prDiscEngine->rSeviceSessionList);
		LINK_INITIALIZE(&prDiscEngine->rFreeServiceSessionList);
		for (u4Idx = 0; u4Idx < NAN_NUM_SERVICE_SESSION; u4Idx++) {
			kalMemZero(&prDiscEngine->arServiceSessionList[u4Idx],
				   sizeof(struct _NAN_SERVICE_SESSION_T));
			LINK_INSERT_TAIL(
				&prDiscEngine->rFreeServiceSessionList,
				&prDiscEngine->arServiceSessionList[u4Idx]
					 .rLinkEntry);
		}
	}

	prDiscEngine->fgInit = TRUE;

	nanDiscReleaseAllServiceSession(prAdapter);
	nanDiscInstanceResetAll(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanDiscUpdateSecContextInfoAttr(struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf) {
	uint8_t *pucPublishNmiAddr;
	uint8_t *pucSecContextInfoAttr;
	struct _NAN_SCHED_EVENT_NAN_ATTR_T *prEventNanAttr;
	struct _NAN_ATTR_SECURITY_CONTEXT_INFO_T *prAttrSecContextInfo;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SERVICE_SESSION_T *prServiceSession;
	uint8_t *pucSecContextList;
	int32_t i4RemainLength;
	struct _NAN_SECURITY_CONTEXT_ID_T *prSecContext;

	prEventNanAttr = (struct _NAN_SCHED_EVENT_NAN_ATTR_T *)pcuEvtBuf;
	pucPublishNmiAddr = prEventNanAttr->aucNmiAddr;
	pucSecContextInfoAttr = prEventNanAttr->aucNanAttr;

	prAttrSecContextInfo = (struct _NAN_ATTR_SECURITY_CONTEXT_INFO_T *)
		pucSecContextInfoAttr;

	pucSecContextList = prAttrSecContextInfo->aucSecurityContextIDList;
	i4RemainLength = prAttrSecContextInfo->u2Length;

	while (i4RemainLength >
	       (sizeof(struct _NAN_SECURITY_CONTEXT_ID_T) - 1)) {
		prSecContext =
			(struct _NAN_SECURITY_CONTEXT_ID_T *)pucSecContextList;
		i4RemainLength -=
			(prSecContext->u2SecurityContextIDTypeLength +
			 sizeof(struct _NAN_SECURITY_CONTEXT_ID_T) - 1);

		if (prSecContext->ucSecurityContextIDType != 1)
			continue;

		if (prSecContext->u2SecurityContextIDTypeLength !=
		    NAN_SCID_DEFAULT_LEN)
			continue;

		prServiceSession = nanDiscAcquireServiceSession(
			prAdapter, pucPublishNmiAddr,
			prSecContext->ucPublishID);
		if (prServiceSession) {
			if (prServiceSession->ucNumSCID < NAN_MAX_SCID_NUM) {
				kalMemCopy(
					&prServiceSession->aaucSupportedSCID
						 [prServiceSession->ucNumSCID]
						 [0],
					prSecContext->aucSecurityContextID,
					NAN_SCID_DEFAULT_LEN);
				prServiceSession->ucNumSCID++;
			}
		}
	}

	return rRetStatus;
}

uint32_t
nanDiscUpdateCipherSuiteInfoAttr(struct ADAPTER *prAdapter,
		uint8_t *pcuEvtBuf) {
	uint8_t *pucPublishNmiAddr;
	uint8_t *pucCipherSuiteInfoAttr;
	struct _NAN_SCHED_EVENT_NAN_ATTR_T *prEventNanAttr;
	struct _NAN_ATTR_CIPHER_SUITE_INFO_T *prAttrCipherSuiteInfo;
	uint32_t rRetStatus = WLAN_STATUS_SUCCESS;
	struct _NAN_SERVICE_SESSION_T *prServiceSession;
	struct _NAN_CIPHER_SUITE_ATTRIBUTE_T *prCipherSuite;
	uint8_t ucNumCsid;
	uint8_t ucIdx;
	struct _NAN_ATTR_HDR_T *prAttrHdr;

	prEventNanAttr = (struct _NAN_SCHED_EVENT_NAN_ATTR_T *)pcuEvtBuf;
	pucPublishNmiAddr = prEventNanAttr->aucNmiAddr;
	pucCipherSuiteInfoAttr = prEventNanAttr->aucNanAttr;

	prAttrHdr = (struct _NAN_ATTR_HDR_T *)prEventNanAttr->aucNanAttr;
	/* nanUtilDump(prAdapter, "NAN Attribute",
	 *	     (PUINT_8)prAttrHdr, (prAttrHdr->u2Length + 3));
	 */

	prAttrCipherSuiteInfo =
		(struct _NAN_ATTR_CIPHER_SUITE_INFO_T *)pucCipherSuiteInfoAttr;

	ucNumCsid = (prAttrCipherSuiteInfo->u2Length - 1) /
		    sizeof(struct _NAN_CIPHER_SUITE_ATTRIBUTE_T);

	prCipherSuite = (struct _NAN_CIPHER_SUITE_ATTRIBUTE_T *)
				prAttrCipherSuiteInfo->aucCipherSuiteList;
	for (ucIdx = 0; ucIdx < ucNumCsid; ucIdx++) {
		prServiceSession = nanDiscAcquireServiceSession(
			prAdapter, pucPublishNmiAddr,
			prCipherSuite->ucPublishID);
		if (prServiceSession) {
			if (prServiceSession->ucNumCSID <
			    NAN_MAX_CIPHER_SUITE_NUM) {
				prServiceSession->aucSupportedCipherSuite
					[prServiceSession->ucNumCSID] =
					prCipherSuite->ucCipherSuiteID;
				prServiceSession->ucNumCSID++;
			}
		}

		prCipherSuite++;
	}

	return rRetStatus;
}

void nanDiscServiceStart(struct ADAPTER *prAdapter,
			uint8_t ucID, unsigned char fgIsPubOrSub)
{
	uint8_t index;

	if (fgIsPubOrSub) {
		for (index = 0; index < NAN_MAX_PUBLISH_NUM; index++) {
			if (g_rNanDiscEngine.arPubIdList[index] == 0) {
				g_rNanDiscEngine.arPubIdList[index] = ucID;
				return;
			}
		}
	} else {
		for (index = 0; index < NAN_MAX_SUBSCRIBE_NUM; index++) {
			if (g_rNanDiscEngine.arSubIdList[index] == 0) {
				g_rNanDiscEngine.arSubIdList[index] = ucID;
				return;
			}
		}
	}
}

void nanDiscServiceTerminate(struct ADAPTER *prAdapter,
			uint8_t ucID, unsigned char fgIsPubOrSub)
{
	uint8_t index;

	if (fgIsPubOrSub) {
		for (index = 0; index < NAN_MAX_PUBLISH_NUM; index++) {
			if (g_rNanDiscEngine.arPubIdList[index] == ucID) {
				g_rNanDiscEngine.arPubIdList[index] = 0;
				return;
			}
		}
	} else {
		for (index = 0; index < NAN_MAX_SUBSCRIBE_NUM; index++) {
			if (g_rNanDiscEngine.arSubIdList[index] == ucID) {
				g_rNanDiscEngine.arSubIdList[index] = 0;
				return;
			}
		}
	}
}

enum NanStatusType
nanDiscSetCustomAttribute(struct ADAPTER *prAdapter,
			struct NanCustomAttribute *prNanCustomAttr)
{
	uint32_t rStatus;
	void *prCmdBuffer = NULL;
	uint32_t u4CmdBufferLen = 0;
	struct _CMD_EVENT_TLV_COMMOM_T *prTlvCommon = NULL;
	struct _CMD_EVENT_TLV_ELEMENT_T *prTlvElement = NULL;
	struct _NAN_CMD_UPDATE_CUSTOM_ATTR_T *prCmdNanCustomAttr = NULL;

	u4CmdBufferLen = sizeof(struct _CMD_EVENT_TLV_COMMOM_T) +
			 sizeof(struct _CMD_EVENT_TLV_ELEMENT_T) +
			 sizeof(struct _NAN_CMD_UPDATE_CUSTOM_ATTR_T);
	prCmdBuffer = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4CmdBufferLen);

	if (!prCmdBuffer) {
		DBGLOG(CNM, ERROR, "Memory allocation fail\n");
		return NAN_STATUS_INTERNAL_FAILURE;
	}

	kalMemZero(prCmdBuffer, u4CmdBufferLen);

	prTlvCommon = (struct _CMD_EVENT_TLV_COMMOM_T *)prCmdBuffer;

	rStatus = nicAddNewTlvElement(
		NAN_CMD_UPDATE_CUSTOM_ATTR, sizeof(struct NanCustomAttribute),
		u4CmdBufferLen, prCmdBuffer);

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

	prCmdNanCustomAttr =
		(struct _NAN_CMD_UPDATE_CUSTOM_ATTR_T *)prTlvElement->aucbody;

	prCmdNanCustomAttr->u2Length = prNanCustomAttr->length;

	kalMemCpyS(prCmdNanCustomAttr->aucData,
		   sizeof(prCmdNanCustomAttr->aucData),
		   prNanCustomAttr->data,
		   prNanCustomAttr->length);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_NAN_EXT_CMD, TRUE,
				      FALSE, FALSE, NULL,
				      nicCmdTimeoutCommon, u4CmdBufferLen,
				      (uint8_t *)prCmdBuffer, NULL, 0);

	cnmMemFree(prAdapter, prCmdBuffer);

	if (rStatus == WLAN_STATUS_SUCCESS || rStatus == WLAN_STATUS_PENDING)
		return NAN_STATUS_SUCCESS;
	else
		return NAN_STATUS_INTERNAL_FAILURE;
}

