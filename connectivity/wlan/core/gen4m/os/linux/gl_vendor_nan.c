// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * gl_vendor_nan.c
 */

#if (CFG_SUPPORT_NAN == 1)
/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "gl_cfg80211.h"
#include "gl_os.h"
#include "debug.h"
#include "gl_vendor.h"
#include "gl_wext.h"
#include "wlan_lib.h"
#include "wlan_oid.h"
#include <linux/can/netlink.h>
#include <net/cfg80211.h>
#include <net/netlink.h>

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
uint8_t g_enableNAN = TRUE;
uint8_t g_disableNAN = FALSE;
uint8_t g_deEvent;
uint8_t g_aucNanServiceName[NAN_MAX_SERVICE_NAME_LEN];
uint8_t g_aucNanServiceId[6];
uint8_t g_ucNanLowPowerMode = FALSE;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void
nanAbortOngoingScan(struct ADAPTER *prAdapter)
{
	struct SCAN_INFO *prScanInfo;

	if (!prAdapter)
		return;

	prScanInfo = &(prAdapter->rWifiVar.rScanInfo);

	if (!prScanInfo || (prScanInfo->eCurrentState != SCAN_STATE_SCANNING))
		return;

	if (IS_BSS_INDEX_AIS(prAdapter, prScanInfo->rScanParam.ucBssIndex))
		aisFsmStateAbort_SCAN(prAdapter,
			prScanInfo->rScanParam.ucBssIndex);
	else if (prScanInfo->rScanParam.ucBssIndex ==
			prAdapter->ucP2PDevBssIdx)
		p2pDevFsmRunEventScanAbort(prAdapter,
			prAdapter->ucP2PDevBssIdx);
}

uint32_t nanOidAbortOngoingScan(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_FAILURE;
	}

	nanAbortOngoingScan(prAdapter);

	DBGLOG(NAN, TRACE, "After\n");

	return WLAN_STATUS_SUCCESS;
}

void
nanNdpAbortScan(struct ADAPTER *prAdapter)
{
	uint32_t rStatus = 0;
	uint32_t u4SetInfoLen = 0;

	if (!prAdapter->rWifiVar.fgNanOnAbortScan)
		return;

	rStatus = kalIoctl(
		prAdapter->prGlueInfo,
		nanOidAbortOngoingScan, NULL, 0,
		&u4SetInfoLen);
}

uint32_t nanOidDissolveReq(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	struct _NAN_NDP_INSTANCE_T *prNDP;
	uint8_t i, j;
	u_int8_t found = FALSE;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, TRACE, "Before\n");

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (i = 0; i < NAN_MAX_SUPPORT_NDL_NUM; i++) {
		if (!prDataPathInfo->arNDL[i].fgNDLValid)
			continue;

		for (j = 0; j < NAN_MAX_SUPPORT_NDP_NUM; j++) {
			prNDP = &prDataPathInfo->arNDL[i].arNDP[j];

			if (prNDP->eCurrentNDPProtocolState !=
				NDP_NORMAL_TR)
				continue;
			prNDP->eLastNDPProtocolState =
				NDP_NORMAL_TR;
			prNDP->eCurrentNDPProtocolState =
				NDP_TX_DP_TERMINATION;
			nanNdpUpdateTypeStatus(prAdapter, prNDP);
			nanNdpSendDataPathTermination(prAdapter,
				prNDP);

			found = TRUE;
		}

		if (i >= prAdapter->rWifiVar.ucNanMaxNdpDissolve)
			break;
	}

	/* Make the frame send to FW ASAP. */
#if !CFG_SUPPORT_MULTITHREAD
	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter,
		DRV_OWN_SRC_NAN_REQ);
#endif
	wlanProcessCommandQueue(prAdapter,
		&prAdapter->prGlueInfo->rCmdQueue);
#if !CFG_SUPPORT_MULTITHREAD
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE, DRV_OWN_SRC_NAN_REQ);
#endif

	if (!found) {
		DBGLOG(NAN, TRACE,
			"Dissolve: Complete NAN\n");
		complete(&prAdapter->prGlueInfo->rNanDissolveComp);
	}

	DBGLOG(NAN, TRACE, "After\n");

	return WLAN_STATUS_SUCCESS;
}

void
nanNdpDissolve(struct ADAPTER *prAdapter,
	uint32_t u4Timeout)
{
	uint32_t waitRet = 0;
	uint32_t rStatus = 0;
	uint32_t u4SetInfoLen = 0;

	reinit_completion(&prAdapter->prGlueInfo->rNanDissolveComp);

	if (prAdapter->rWifiVar.fgNanDissolveAbortScan)
		rStatus = kalIoctl(
			prAdapter->prGlueInfo,
			nanOidAbortOngoingScan, NULL, 0,
			&u4SetInfoLen);

	rStatus = kalIoctl(
		prAdapter->prGlueInfo,
		nanOidDissolveReq, NULL, 0,
		&u4SetInfoLen);

	waitRet = wait_for_completion_timeout(
		&prAdapter->prGlueInfo->rNanDissolveComp,
		MSEC_TO_JIFFIES(
		u4Timeout));
	if (!waitRet)
		DBGLOG(NAN, TRACE, "Disconnect timeout.\n");
	else
		DBGLOG(NAN, TRACE, "Disconnect complete.\n");
}

/* Helper function to Write and Read TLV called in indication as well as
 * request
 */
u16
nanWriteTlv(struct _NanTlv *pInTlv, u8 *pOutTlv)
{
	u16 writeLen = 0;
	u16 i;

	if (!pInTlv) {
		DBGLOG(NAN, ERROR, "NULL pInTlv\n");
		return writeLen;
	}

	if (!pOutTlv) {
		DBGLOG(NAN, ERROR, "NULL pOutTlv\n");
		return writeLen;
	}

	*pOutTlv++ = pInTlv->type & 0xFF;
	*pOutTlv++ = (pInTlv->type & 0xFF00) >> 8;
	writeLen += 2;

	DBGLOG(NAN, LOUD, "Write TLV type %u, writeLen %u\n", pInTlv->type,
	       writeLen);

	*pOutTlv++ = pInTlv->length & 0xFF;
	*pOutTlv++ = (pInTlv->length & 0xFF00) >> 8;
	writeLen += 2;

	DBGLOG(NAN, LOUD, "Write TLV length %u, writeLen %u\n", pInTlv->length,
	       writeLen);

	for (i = 0; i < pInTlv->length; ++i)
		*pOutTlv++ = pInTlv->value[i];

	writeLen += pInTlv->length;
	DBGLOG(NAN, LOUD, "Write TLV value, writeLen %u\n", writeLen);
	return writeLen;
}

u16
nan_read_tlv(u8 *pInTlv, struct _NanTlv *pOutTlv)
{
	u16 readLen = 0;

	if (!pInTlv)
		return readLen;

	if (!pOutTlv)
		return readLen;

	pOutTlv->type = *pInTlv++;
	pOutTlv->type |= *pInTlv++ << 8;
	readLen += 2;

	pOutTlv->length = *pInTlv++;
	pOutTlv->length |= *pInTlv++ << 8;
	readLen += 2;

	if (pOutTlv->length) {
		pOutTlv->value = pInTlv;
		readLen += pOutTlv->length;
	} else {
		pOutTlv->value = NULL;
	}
	return readLen;
}

u8 *
nanAddTlv(u16 type, u16 length, u8 *value, u8 *pOutTlv)
{
	struct _NanTlv nanTlv;
	u16 len;

	nanTlv.type = type;
	nanTlv.length = length;
	nanTlv.value = (u8 *)value;

	len = nanWriteTlv(&nanTlv, pOutTlv);
	return (pOutTlv + len);
}

u16
nanMapPublishReqParams(u16 *pIndata, struct NanPublishRequest *pOutparams)
{
	u16 readLen = 0;
	u32 *pPublishParams = NULL;

	DBGLOG(NAN, DEBUG, "Enter\n");

	/* Get value of ttl(time to live) */
	pOutparams->ttl = *pIndata;

	/* Get value of ttl(time to live) */
	pIndata++;
	readLen += 2;

	/* Get value of period */
	pOutparams->period = *pIndata;

	/* Assign default value */
	if (pOutparams->period == 0)
		pOutparams->period = 1;

	pIndata++;
	readLen += 2;

	pPublishParams = (u32 *)pIndata;
	dumpMemory32(pPublishParams, 4);
	pOutparams->recv_indication_cfg =
		(u8)(GET_PUB_REPLY_IND_FLAG(*pPublishParams) |
		     GET_PUB_FOLLOWUP_RX_IND_DISABLE_FLAG(*pPublishParams) |
		     GET_PUB_MATCH_EXPIRED_IND_DISABLE_FLAG(*pPublishParams) |
		     GET_PUB_TERMINATED_IND_DISABLE_FLAG(*pPublishParams));
	pOutparams->publish_type = GET_PUB_PUBLISH_TYPE(*pPublishParams);
	pOutparams->tx_type = GET_PUB_TX_TYPE(*pPublishParams);
	pOutparams->rssi_threshold_flag =
		(u8)GET_PUB_RSSI_THRESHOLD_FLAG(*pPublishParams);
	pOutparams->publish_match_indicator =
		GET_PUB_MATCH_ALG(*pPublishParams);
	pOutparams->publish_count = (u8)GET_PUB_COUNT(*pPublishParams);
	pOutparams->connmap = (u8)GET_PUB_CONNMAP(*pPublishParams);
	readLen += 4;

	DBGLOG(NAN, INFO,
	       "[Publish Req] ttl: %u, period: %u, recv_indication_cfg: %x, publish_type: %u,tx_type: %u, rssi_threshold_flag: %u, publish_match_indicator: %u, publish_count:%u, connmap:%u, readLen:%u\n",
	       pOutparams->ttl, pOutparams->period,
	       pOutparams->recv_indication_cfg, pOutparams->publish_type,
	       pOutparams->tx_type, pOutparams->rssi_threshold_flag,
	       pOutparams->publish_match_indicator, pOutparams->publish_count,
	       pOutparams->connmap, readLen);

	return readLen;
}

u16
nanMapSubscribeReqParams(u16 *pIndata, struct NanSubscribeRequest *pOutparams)
{
	u16 readLen = 0;
	u32 *pSubscribeParams = NULL;

	DBGLOG(NAN, TRACE, "Enter\n");

	pOutparams->ttl = *pIndata;
	pIndata++;
	readLen += 2;

	pOutparams->period = *pIndata;
	pIndata++;
	readLen += 2;

	pSubscribeParams = (u32 *)pIndata;

	pOutparams->subscribe_type = GET_SUB_SUBSCRIBE_TYPE(*pSubscribeParams);
	pOutparams->serviceResponseFilter = GET_SUB_SRF_ATTR(*pSubscribeParams);
	pOutparams->serviceResponseInclude =
		GET_SUB_SRF_INCLUDE(*pSubscribeParams);
	pOutparams->useServiceResponseFilter =
		GET_SUB_SRF_SEND(*pSubscribeParams);
	pOutparams->ssiRequiredForMatchIndication =
		GET_SUB_SSI_REQUIRED(*pSubscribeParams);
	pOutparams->subscribe_match_indicator =
		GET_SUB_MATCH_ALG(*pSubscribeParams);
	pOutparams->subscribe_count = (u8)GET_SUB_COUNT(*pSubscribeParams);
	pOutparams->rssi_threshold_flag =
		(u8)GET_SUB_RSSI_THRESHOLD_FLAG(*pSubscribeParams);
	pOutparams->recv_indication_cfg =
		(u8)GET_SUB_FOLLOWUP_RX_IND_DISABLE_FLAG(*pSubscribeParams) |
		GET_SUB_MATCH_EXPIRED_IND_DISABLE_FLAG(*pSubscribeParams) |
		GET_SUB_TERMINATED_IND_DISABLE_FLAG(*pSubscribeParams);

	DBGLOG(NAN, INFO,
	       "[Subscribe Req] ttl: %u, period: %u, subscribe_type: %u, ssiRequiredForMatchIndication: %u, subscribe_match_indicator: %x, rssi_threshold_flag: %u\n",
	       pOutparams->ttl, pOutparams->period,
	       pOutparams->subscribe_type,
	       pOutparams->ssiRequiredForMatchIndication,
	       pOutparams->subscribe_match_indicator,
	       pOutparams->rssi_threshold_flag);
	pOutparams->connmap = (u8)GET_SUB_CONNMAP(*pSubscribeParams);
	readLen += 4;
	DBGLOG(NAN, LOUD, "Subscribe readLen : %d\n", readLen);
	return readLen;
}

u16
nanMapFollowupReqParams(u32 *pIndata,
			struct NanTransmitFollowupRequest *pOutparams)
{
	u16 readLen = 0;
	u32 *pXmitFollowupParams = NULL;

	pOutparams->requestor_instance_id = *pIndata;
	pIndata++;
	readLen += 4;

	pXmitFollowupParams = pIndata;

	pOutparams->priority = GET_FLWUP_PRIORITY(*pXmitFollowupParams);
	pOutparams->dw_or_faw = GET_FLWUP_WINDOW(*pXmitFollowupParams);
	pOutparams->recv_indication_cfg =
		GET_FLWUP_TX_RSP_DISABLE_FLAG(*pXmitFollowupParams);
	readLen += 4;

	DBGLOG(NAN, DEBUG,
	       "[%s]priority: %u, dw_or_faw: %u, recv_indication_cfg: %u\n",
	       __func__, pOutparams->priority, pOutparams->dw_or_faw,
	       pOutparams->recv_indication_cfg);

	return readLen;
}

void
nanMapSdeaCtrlParams(u32 *pIndata,
		     struct NanSdeaCtrlParams *prNanSdeaCtrlParms)
{
	prNanSdeaCtrlParms->config_nan_data_path =
		GET_SDEA_DATA_PATH_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->ndp_type = GET_SDEA_DATA_PATH_TYPE(*pIndata);
	prNanSdeaCtrlParms->security_cfg = GET_SDEA_SECURITY_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->ranging_state = GET_SDEA_RANGING_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->range_report = GET_SDEA_RANGE_REPORT(*pIndata);
	prNanSdeaCtrlParms->fgFSDRequire = GET_SDEA_FSD_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->fgGAS = GET_SDEA_FSD_WITH_GAS(*pIndata);
	prNanSdeaCtrlParms->fgQoS = GET_SDEA_QOS_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->fgRangeLimit =
		GET_SDEA_RANGE_LIMIT_PRESENT(*pIndata);

	DBGLOG(NAN, DEBUG,
	       "config_nan_data_path: %u, ndp_type: %u, security_cfg: %u\n",
	       prNanSdeaCtrlParms->config_nan_data_path,
	       prNanSdeaCtrlParms->ndp_type, prNanSdeaCtrlParms->security_cfg);
	DBGLOG(NAN, DEBUG,
	       "ranging_state: %u, range_report: %u, fgFSDRequire: %u, fgGAS: %u, fgQoS: %u, fgRangeLimit: %u\n",
	       prNanSdeaCtrlParms->ranging_state,
	       prNanSdeaCtrlParms->range_report,
	       prNanSdeaCtrlParms->fgFSDRequire,
	       prNanSdeaCtrlParms->fgGAS, prNanSdeaCtrlParms->fgQoS,
	       prNanSdeaCtrlParms->fgRangeLimit);
}

void
nanMapRangingConfigParams(u32 *pIndata, struct NanRangingCfg *prNanRangingCfg)
{
	struct NanFWRangeConfigParams *prNanFWRangeCfgParams;

	prNanFWRangeCfgParams = (struct NanFWRangeConfigParams *)pIndata;

	prNanRangingCfg->ranging_resolution =
		prNanFWRangeCfgParams->range_resolution;
	prNanRangingCfg->ranging_interval_msec =
		prNanFWRangeCfgParams->range_interval;
	prNanRangingCfg->config_ranging_indications =
		prNanFWRangeCfgParams->ranging_indication_event;

	if (prNanRangingCfg->config_ranging_indications &
	    NAN_RANGING_INDICATE_INGRESS_MET_MASK)
		prNanRangingCfg->distance_ingress_cm =
			prNanFWRangeCfgParams->geo_fence_threshold
				.inner_threshold /
			10;
	if (prNanRangingCfg->config_ranging_indications &
	    NAN_RANGING_INDICATE_EGRESS_MET_MASK)
		prNanRangingCfg->distance_egress_cm =
			prNanFWRangeCfgParams->geo_fence_threshold
				.outer_threshold /
			10;

	DBGLOG(NAN, DEBUG,
	       "[%s]ranging_resolution: %u, ranging_interval_msec: %u, config_ranging_indications: %u\n",
	       __func__, prNanRangingCfg->ranging_resolution,
	       prNanRangingCfg->ranging_interval_msec,
	       prNanRangingCfg->config_ranging_indications);
	DBGLOG(NAN, DEBUG, "[%s]distance_egress_cm: %u\n", __func__,
	       prNanRangingCfg->distance_egress_cm);
}

void
nanMapNan20RangingReqParams(struct ADAPTER *prAdapter, u32 *pIndata,
			    struct NanRangeResponseCfg *prNanRangeRspCfgParms)
{
	struct NanFWRangeReqMsg *pNanFWRangeReqMsg;

	pNanFWRangeReqMsg = (struct NanFWRangeReqMsg *)pIndata;

	prNanRangeRspCfgParms->requestor_instance_id =
		pNanFWRangeReqMsg->range_id;
	memcpy(&prNanRangeRspCfgParms->peer_addr,
	       &pNanFWRangeReqMsg->range_mac_addr, NAN_MAC_ADDR_LEN);
	if (pNanFWRangeReqMsg->ranging_accept == 1)
		prNanRangeRspCfgParms->ranging_response_code =
			NAN_RANGE_REQUEST_ACCEPT;
	else if (pNanFWRangeReqMsg->ranging_reject == 1)
		prNanRangeRspCfgParms->ranging_response_code =
			NAN_RANGE_REQUEST_REJECT;
	else
		prNanRangeRspCfgParms->ranging_response_code =
			NAN_RANGE_REQUEST_CANCEL;

	DBGLOG(NAN, DEBUG,
	       "[%s]requestor_instance_id: %u, ranging_response_code:%u\n",
	       __func__, prNanRangeRspCfgParms->requestor_instance_id,
	       prNanRangeRspCfgParms->ranging_response_code);
	DBGFWLOG(NAN, INFO, prAdapter,
		"[%s] addr=>%02x:%02x:%02x:%02x:%02x:%02x\n",
		 __func__, prNanRangeRspCfgParms->peer_addr[0],
		 prNanRangeRspCfgParms->peer_addr[1],
		 prNanRangeRspCfgParms->peer_addr[2],
		 prNanRangeRspCfgParms->peer_addr[3],
		 prNanRangeRspCfgParms->peer_addr[4],
		 prNanRangeRspCfgParms->peer_addr[5]);
}

u32
wlanoidGetNANCapabilitiesRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen)
{
	struct NanCapabilitiesRspMsg nanCapabilitiesRsp;
	struct NanCapabilitiesRspMsg *pNanCapabilitiesRsp =
		(struct NanCapabilitiesRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	nanCapabilitiesRsp.fwHeader.msgVersion = 1;
	nanCapabilitiesRsp.fwHeader.msgId = NAN_MSG_ID_CAPABILITIES_RSP;
	nanCapabilitiesRsp.fwHeader.msgLen =
		sizeof(struct NanCapabilitiesRspMsg);
	nanCapabilitiesRsp.fwHeader.transactionId =
		pNanCapabilitiesRsp->fwHeader.transactionId;
	nanCapabilitiesRsp.status = 0;
	nanCapabilitiesRsp.max_concurrent_nan_clusters = 1;
	nanCapabilitiesRsp.max_service_name_len = NAN_MAX_SERVICE_NAME_LEN;
	nanCapabilitiesRsp.max_match_filter_len = NAN_FW_MAX_MATCH_FILTER_LEN;
	nanCapabilitiesRsp.max_service_specific_info_len =
		NAN_MAX_SERVICE_SPECIFIC_INFO_LEN;
	nanCapabilitiesRsp.max_sdea_service_specific_info_len =
		NAN_FW_MAX_FOLLOW_UP_SDEA_LEN;
	nanCapabilitiesRsp.max_scid_len = NAN_MAX_SCID_BUF_LEN;
	nanCapabilitiesRsp.max_total_match_filter_len =
		(NAN_FW_MAX_MATCH_FILTER_LEN * 2);
	nanCapabilitiesRsp.cipher_suites_supported =
		NAN_CIPHER_SUITE_SHARED_KEY_128_MASK;
	nanCapabilitiesRsp.max_ndi_interfaces = 1;
	nanCapabilitiesRsp.max_publishes = NAN_MAX_PUBLISH_NUM;
	nanCapabilitiesRsp.max_subscribes = NAN_MAX_SUBSCRIBE_NUM;
	nanCapabilitiesRsp.max_ndp_sessions =
		prAdapter->rWifiVar.ucNanMaxNdpSession;
	nanCapabilitiesRsp.max_app_info_len = NAN_DP_MAX_APP_INFO_LEN;
	nanCapabilitiesRsp.max_queued_transmit_followup_msgs =
		NAN_MAX_QUEUE_FOLLOW_UP;
	nanCapabilitiesRsp.max_subscribe_address =
		NAN_MAX_SUBSCRIBE_MAX_ADDRESS;
	nanCapabilitiesRsp.is_pairing_supported = FALSE;
	nanCapabilitiesRsp.is_instant_mode_supported = FALSE;
	nanCapabilitiesRsp.is_set_cluster_id_supported = FALSE;
	nanCapabilitiesRsp.is_suspension_supported = FALSE;
#if (CFG_SUPPORT_802_11AX == 1)
	nanCapabilitiesRsp.is_he_supported = TRUE;
#else
	nanCapabilitiesRsp.is_he_supported = FALSE;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	nanCapabilitiesRsp.is_6g_supported =
		!prAdapter->rWifiVar.ucDisallowBand6G;
#else
	nanCapabilitiesRsp.is_6g_supported = FALSE;
#endif

	/*  Fill values of nanCapabilitiesRsp */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					  sizeof(struct NanCapabilitiesRspMsg) +
						  NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanCapabilitiesRspMsg),
			     &nanCapabilitiesRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANEnableRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
		    uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct NanEnableRspMsg nanEnableRsp;
	struct NanEnableRspMsg *pNanEnableRsp =
		(struct NanEnableRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	nanSchedUpdateP2pAisMcc(prAdapter);
	nanExtEnableReq(prAdapter);

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	nanEnableRsp.fwHeader.msgVersion = 1;
	nanEnableRsp.fwHeader.msgId = NAN_MSG_ID_ENABLE_RSP;
	nanEnableRsp.fwHeader.msgLen = sizeof(struct NanEnableRspMsg);
	nanEnableRsp.fwHeader.transactionId =
		pNanEnableRsp->fwHeader.transactionId;
	nanEnableRsp.status = 0;
	nanEnableRsp.value = 0;

	/*  Fill values of nanCapabilitiesRsp */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev, sizeof(struct NanEnableRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanEnableRspMsg),
			     &nanEnableRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	g_disableNAN = TRUE;

	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANDisableRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
		     uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct NanDisableRspMsg nanDisableRsp;
	struct NanDisableRspMsg *pNanDisableRsp =
		(struct NanDisableRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	nanExtDisableReq(prAdapter);

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	nanDisableRsp.fwHeader.msgVersion = 1;
	nanDisableRsp.fwHeader.msgId = NAN_MSG_ID_DISABLE_RSP;
	nanDisableRsp.fwHeader.msgLen = sizeof(struct NanDisableRspMsg);
	nanDisableRsp.fwHeader.transactionId =
		pNanDisableRsp->fwHeader.transactionId;
	nanDisableRsp.status = 0;

	/*  Fill values of nanCapabilitiesRsp */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev, sizeof(struct NanDisableRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanDisableRspMsg),
			     &nanDisableRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANConfigRsp(struct ADAPTER *prAdapter,
			      void *pvSetBuffer, uint32_t u4SetBufferLen,
			      uint32_t *pu4SetInfoLen)
{
	struct NanConfigRspMsg nanConfigRsp;
	struct NanConfigRspMsg *pNanConfigRsp =
		(struct NanConfigRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	nanConfigRsp.fwHeader.msgVersion = 1;
	nanConfigRsp.fwHeader.msgId = NAN_MSG_ID_CONFIGURATION_RSP;
	nanConfigRsp.fwHeader.msgLen = sizeof(struct NanConfigRspMsg);
	nanConfigRsp.fwHeader.transactionId =
		pNanConfigRsp->fwHeader.transactionId;
	nanConfigRsp.status = 0;
	nanConfigRsp.value = 0;

	/*  Fill values of nanCapabilitiesRsp */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev, sizeof(struct NanConfigRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanConfigRspMsg),
			     &nanConfigRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNanPublishRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
		     uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct NanPublishServiceRspMsg nanPublishRsp;
	struct NanPublishServiceRspMsg *pNanPublishRsp =
		(struct NanPublishServiceRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	kalMemZero(&nanPublishRsp, sizeof(struct NanPublishServiceRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	/* Prepare publish response header*/
	nanPublishRsp.fwHeader.msgVersion = 1;
	nanPublishRsp.fwHeader.msgId = NAN_MSG_ID_PUBLISH_SERVICE_RSP;
	nanPublishRsp.fwHeader.msgLen = sizeof(struct NanPublishServiceRspMsg);
	nanPublishRsp.fwHeader.handle = pNanPublishRsp->fwHeader.handle;
	nanPublishRsp.fwHeader.transactionId =
		pNanPublishRsp->fwHeader.transactionId;
	nanPublishRsp.value = 0;

	if (nanPublishRsp.fwHeader.handle != 0)
		nanPublishRsp.status = NAN_I_STATUS_SUCCESS;
	else
		nanPublishRsp.status = NAN_I_STATUS_INVALID_HANDLE;

	DBGLOG(NAN, DEBUG, "publish ID:%u, msgId:%u, msgLen:%u, tranID:%u\n",
	       nanPublishRsp.fwHeader.handle, nanPublishRsp.fwHeader.msgId,
	       nanPublishRsp.fwHeader.msgLen,
	       nanPublishRsp.fwHeader.transactionId);

	/*  Fill values of nanPublishRsp */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev,
		sizeof(struct NanPublishServiceRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanPublishServiceRspMsg),
			     &nanPublishRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	/* Free the memory due to no use anymore */
	kfree(pvSetBuffer);

	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANCancelPublishRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
			   uint32_t u4SetBufferLen,
			   uint32_t *pu4SetInfoLen)
{
	struct NanPublishServiceCancelRspMsg nanPublishCancelRsp;
	struct NanPublishServiceCancelRspMsg *pNanPublishCancelRsp =
		(struct NanPublishServiceCancelRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	kalMemZero(&nanPublishCancelRsp,
		   sizeof(struct NanPublishServiceCancelRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	DBGLOG(NAN, DEBUG, "Enter\n");

	nanPublishCancelRsp.fwHeader.msgVersion = 1;
	nanPublishCancelRsp.fwHeader.msgId =
		NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_RSP;
	nanPublishCancelRsp.fwHeader.msgLen =
		sizeof(struct NanPublishServiceCancelRspMsg);
	nanPublishCancelRsp.fwHeader.handle =
		pNanPublishCancelRsp->fwHeader.handle;
	nanPublishCancelRsp.fwHeader.transactionId =
		pNanPublishCancelRsp->fwHeader.transactionId;
	nanPublishCancelRsp.value = 0;
	nanPublishCancelRsp.status = pNanPublishCancelRsp->status;

	DBGLOG(NAN, DEBUG, "[%s] nanPublishCancelRsp.fwHeader.handle = %d\n",
	       __func__, nanPublishCancelRsp.fwHeader.handle);
	DBGLOG(NAN, DEBUG,
	       "[%s] nanPublishCancelRsp.fwHeader.transactionId = %d\n",
	       __func__, nanPublishCancelRsp.fwHeader.transactionId);

	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev,
		sizeof(struct NanPublishServiceCancelRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanPublishServiceCancelRspMsg),
			     &nanPublishCancelRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNanSubscribeRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
		       uint32_t u4SetBufferLen,
		       uint32_t *pu4SetInfoLen)
{
	struct NanSubscribeServiceRspMsg nanSubscribeRsp;
	struct NanSubscribeServiceRspMsg *pNanSubscribeRsp =
		(struct NanSubscribeServiceRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	kalMemZero(&nanSubscribeRsp, sizeof(struct NanSubscribeServiceRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	DBGLOG(NAN, DEBUG, "Enter\n");

	nanSubscribeRsp.fwHeader.msgVersion = 1;
	nanSubscribeRsp.fwHeader.msgId = NAN_MSG_ID_SUBSCRIBE_SERVICE_RSP;
	nanSubscribeRsp.fwHeader.msgLen =
		sizeof(struct NanSubscribeServiceRspMsg);
	nanSubscribeRsp.fwHeader.handle = pNanSubscribeRsp->fwHeader.handle;
	nanSubscribeRsp.fwHeader.transactionId =
		pNanSubscribeRsp->fwHeader.transactionId;
	nanSubscribeRsp.value = 0;
	if (nanSubscribeRsp.fwHeader.handle != 0)
		nanSubscribeRsp.status = NAN_I_STATUS_SUCCESS;
	else
		nanSubscribeRsp.status = NAN_I_STATUS_INVALID_HANDLE;

	/*  Fill values of nanSubscribeRsp */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev,
		sizeof(struct NanSubscribeServiceRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanSubscribeServiceRspMsg),
			     &nanSubscribeRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	DBGLOG(NAN, INFO, "handle:%u,transactionId:%u\n",
	       nanSubscribeRsp.fwHeader.handle,
	       nanSubscribeRsp.fwHeader.transactionId);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANCancelSubscribeRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
			     uint32_t u4SetBufferLen,
			     uint32_t *pu4SetInfoLen)
{
	struct NanSubscribeServiceCancelRspMsg nanSubscribeCancelRsp;
	struct NanSubscribeServiceCancelRspMsg *pNanSubscribeCancelRsp =
		(struct NanSubscribeServiceCancelRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	kalMemZero(&nanSubscribeCancelRsp,
		   sizeof(struct NanSubscribeServiceCancelRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	DBGLOG(NAN, DEBUG, "Enter\n");

	nanSubscribeCancelRsp.fwHeader.msgVersion = 1;
	nanSubscribeCancelRsp.fwHeader.msgId =
		NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_RSP;
	nanSubscribeCancelRsp.fwHeader.msgLen =
		sizeof(struct NanSubscribeServiceCancelRspMsg);
	nanSubscribeCancelRsp.fwHeader.handle =
		pNanSubscribeCancelRsp->fwHeader.handle;
	nanSubscribeCancelRsp.fwHeader.transactionId =
		pNanSubscribeCancelRsp->fwHeader.transactionId;
	nanSubscribeCancelRsp.value = 0;
	nanSubscribeCancelRsp.status = pNanSubscribeCancelRsp->status;

	/*  Fill values of NanSubscribeServiceCancelRspMsg */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev,
		sizeof(struct NanSubscribeServiceCancelRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanSubscribeServiceCancelRspMsg),
			     &nanSubscribeCancelRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(NAN, ERROR, "handle:%u, transactionId:%u\n",
	       nanSubscribeCancelRsp.fwHeader.handle,
	       nanSubscribeCancelRsp.fwHeader.transactionId);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANFollowupRsp(struct ADAPTER *prAdapter, void *pvSetBuffer,
		      uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	struct NanTransmitFollowupRspMsg nanXmitFollowupRsp;
	struct NanTransmitFollowupRspMsg *pNanXmitFollowupRsp =
		(struct NanTransmitFollowupRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct net_device *prDev;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	kalMemZero(&nanXmitFollowupRsp,
		   sizeof(struct NanTransmitFollowupRspMsg));

	DBGLOG(NAN, DEBUG, "Enter\n");

	/* Prepare Transmit Follow up response */
	nanXmitFollowupRsp.fwHeader.msgVersion = 1;
	nanXmitFollowupRsp.fwHeader.msgId = NAN_MSG_ID_TRANSMIT_FOLLOWUP_RSP;
	nanXmitFollowupRsp.fwHeader.msgLen =
		sizeof(struct NanTransmitFollowupRspMsg);
	nanXmitFollowupRsp.fwHeader.handle =
		pNanXmitFollowupRsp->fwHeader.handle;
	nanXmitFollowupRsp.fwHeader.transactionId =
		pNanXmitFollowupRsp->fwHeader.transactionId;
	nanXmitFollowupRsp.status = pNanXmitFollowupRsp->status;
	nanXmitFollowupRsp.value = 0;

	/*  Fill values of NanSubscribeServiceCancelRspMsg */
	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev,
		sizeof(struct NanTransmitFollowupRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanTransmitFollowupRspMsg),
			     &nanXmitFollowupRsp) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE

void nan_wiphy_unlock(struct wiphy *wiphy)
{
	if (!wiphy) {
		log_dbg(NAN, ERROR, "wiphy is null\n");
		return;
	}
	wiphy_unlock(wiphy);
}

void nan_wiphy_lock(struct wiphy *wiphy)
{
	if (!wiphy) {
		log_dbg(NAN, ERROR, "wiphy is null\n");
		return;
	}
	wiphy_lock(wiphy);
}
#endif

struct NanDataPathInitiatorNDPE g_ndpReqNDPE;
int mtk_cfg80211_vendor_nan(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sk_buff *skb = NULL;
	struct ADAPTER *prAdapter;
	struct _NAN_SCHEDULER_T *prNanScheduler;

	struct _NanMsgHeader nanMsgHdr;
	struct _NanTlv outputTlv;
	u16 readLen = 0;
	u32 u4BufLen;
	u32 i4Status = -EINVAL;
	u32 u4DelayIdx;
	int ret = 0;
	int remainingLen;
	u32 waitRet = 0;

	if (data_len < sizeof(struct _NanMsgHeader)) {
		DBGLOG(NAN, ERROR, "data_len error!\n");
		return -EINVAL;
	}
	remainingLen = (data_len - (sizeof(struct _NanMsgHeader)));

	if (!wiphy) {
		DBGLOG(NAN, ERROR, "wiphy error!\n");
		return -EINVAL;
	}
	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev error!\n");
		return -EINVAL;
	}

	if (data == NULL || data_len <= 0) {
		DBGLOG(NAN, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(NAN, ERROR, "prGlueInfo error!\n");
		return -EINVAL;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return -EINVAL;
	}

	prAdapter->fgIsNANfromHAL = TRUE;
	DBGLOG(NAN, LOUD, "NAN fgIsNANfromHAL set %u\n",
		prAdapter->fgIsNANfromHAL);

	dumpMemory8((uint8_t *)data, data_len);
	DBGLOG(NAN, TRACE, "DATA len from user %d, lock(%d)\n",
		data_len,
		rtnl_is_locked());

	memcpy(&nanMsgHdr, (struct _NanMsgHeader *)data,
		sizeof(struct _NanMsgHeader));
	data += sizeof(struct _NanMsgHeader);

	dumpMemory8((uint8_t *)data, remainingLen);
	DBGLOG(NAN, INFO, "nanMsgHdr.length %u, nanMsgHdr.msgId %d\n",
		nanMsgHdr.msgLen, nanMsgHdr.msgId);

	switch (nanMsgHdr.msgId) {
	case NAN_MSG_ID_ENABLE_REQ: {
		struct NanEnableRequest nanEnableReq;
		struct NanEnableRspMsg nanEnableRsp;
#if KERNEL_VERSION(5, 12, 0) > CFG80211_VERSION_CODE
		uint8_t fgRollbackRtnlLock = FALSE;
#endif

		nanNdpAbortScan(prAdapter);

		kalMemZero(&nanEnableReq, sizeof(struct NanEnableRequest));
		kalMemZero(&nanEnableRsp, sizeof(struct NanEnableRspMsg));

		memcpy(&nanEnableRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanEnableRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanEnableRspMsg),
					   &nanEnableRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		ret = cfg80211_vendor_cmd_reply(skb);

		if (prAdapter->fgIsNANRegistered) {
			DBGLOG(NAN, WARN, "NAN is already enabled\n");
			goto skip_enable;
		}

#if KERNEL_VERSION(3, 13, 0) <= CFG80211_VERSION_CODE
		kal_reinit_completion(
			&prAdapter->prGlueInfo->rNanHaltComp);
#if (CFG_SUPPORT_MLO_STA_NAN_FALLBACK == 1)
		kal_reinit_completion(
			&prAdapter->prGlueInfo->rNanAisComp);
#endif
#else
		prAdapter->prGlueInfo->rNanHaltComp.done = 0;
#if (CFG_SUPPORT_MLO_STA_NAN_FALLBACK == 1)
		prAdapter->prGlueInfo->rNanAisComp.done = 0;
#endif
#endif

		for (u4DelayIdx = 0; u4DelayIdx < 5; u4DelayIdx++) {
			if (g_enableNAN == TRUE) {
				g_enableNAN = FALSE;
				break;
			}
			msleep(1000);
		}
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		nan_wiphy_unlock(wiphy);
#else
		/* to avoid re-enter rtnl lock during
		 * register_netdev/unregister_netdev NAN/P2P
		 * we take away lock first and return later
		 */
		if (rtnl_is_locked()) {
			fgRollbackRtnlLock = TRUE;
			rtnl_unlock();
		}
#endif

#if (CFG_SUPPORT_MLO_STA_NAN_FALLBACK == 1)
		if (aisGetLinkNum(
			aisGetDefaultAisInfo(prAdapter)) > 1 &&
			nanIsSapOrP2pActive(prAdapter)) {
			prAdapter->fgIsNANStartWaiting = TRUE;
			aisBssBeaconTimeout_impl(prAdapter,
			BEACON_TIMEOUT_REASON_NUM,
			FALSE,
			aisGetDefaultLinkBssIndex(prAdapter));
			waitRet = wait_for_completion_timeout(
				&prAdapter->prGlueInfo->rNanAisComp,
				MSEC_TO_JIFFIES(2*1000));
			prAdapter->fgIsNANStartWaiting = FALSE;
		}
#endif

		DBGLOG(NAN, TRACE,
			"[DBG] NAN enable enter set_nan_handler, lock(%d)\n",
			rtnl_is_locked());
		set_nan_handler(wdev->netdev, 1, FALSE);
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		nan_wiphy_lock(wiphy);
#else
		if (fgRollbackRtnlLock)
			rtnl_lock();
#endif

		g_deEvent = 0;

		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_CONFIG_DISCOVERY_INDICATIONS:
					nanEnableReq.discovery_indication_cfg =
						*outputTlv.value;
				break;
			case NAN_TLV_TYPE_CLUSTER_ID_LOW:
				if (outputTlv.length >
					sizeof(nanEnableReq.cluster_low)) {
					DBGLOG(NAN, ERROR,
						"type%d outputTlv.length is invalid!\n",
						outputTlv.type);
					return -EFAULT;
				}
				memcpy(&nanEnableReq.cluster_low,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_CLUSTER_ID_HIGH:
				if (outputTlv.length >
					sizeof(nanEnableReq.cluster_high)) {
					DBGLOG(NAN, ERROR,
						"type%d outputTlv.length is invalid!\n",
						outputTlv.type);
					return -EFAULT;
				}
				memcpy(&nanEnableReq.cluster_high,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_MASTER_PREFERENCE:
				if (outputTlv.length >
					sizeof(nanEnableReq.master_pref)) {
					DBGLOG(NAN, ERROR,
						"type%d outputTlv.length is invalid!\n",
						outputTlv.type);
					return -EFAULT;
				}
				memcpy(&nanEnableReq.master_pref,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_ENABLE_INSTANT_MODE:
				if (outputTlv.length != sizeof(uint32_t)) {
					DBGLOG(NAN, ERROR,
						"type%d outputTlv.length %u is invalid!\n",
						outputTlv.type,
						outputTlv.length);
					continue;
				}
				nanEnableReq.fgNanInstantMode =
					!!(*(uint32_t *)outputTlv.value);
				DBGLOG(NAN, INFO,
				       "Set fgNanInstantMode=%u\n",
				       nanEnableReq.fgNanInstantMode);
				break;
			case NAN_TLV_TYPE_ENABLE_INSTANT_MODE_CHANNEL:
				if (outputTlv.length != sizeof(uint32_t)) {
					DBGLOG(NAN, ERROR,
						"type%d outputTlv.length %u is invalid!\n",
						outputTlv.type,
						outputTlv.length);
				}
				memcpy(&nanEnableReq.u4NanInstantModeChannel,
				       outputTlv.value, outputTlv.length);
				DBGLOG(NAN, INFO,
				       "Set u4NanInstantModeChannel=%u\n",
				       nanEnableReq.u4NanInstantModeChannel);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		nanEnableReq.enable_log_slot_statistics =
			prAdapter->rWifiVar.ucNanLogSlotStatistics;

		nanEnableReq.master_pref = prAdapter->rWifiVar.ucMasterPref;
		nanEnableReq.config_random_factor_force = 0;
		nanEnableReq.random_factor_force_val = 0;
		nanEnableReq.config_hop_count_force = 0;
		nanEnableReq.hop_count_force_val = 0;
		nanEnableReq.config_5g_channel =
			prAdapter->rWifiVar.ucConfig5gChannel;
		if (rlmDomainIsLegalChannel(prAdapter,
					    BAND_5G,
					    NAN_5G_LOW_DISC_CHANNEL))
			nanEnableReq.channel_5g_val |= BIT(0);
		if (rlmDomainIsLegalChannel(prAdapter,
					    BAND_5G,
					    NAN_5G_HIGH_DISC_CHANNEL))
			nanEnableReq.channel_5g_val |= BIT(1);

		prAdapter->fgIsNANCSAWaiting = TRUE;
		/* Wait DBDC enable here, then send Nan neable request */
		waitRet = wait_for_completion_timeout(
			&prAdapter->prGlueInfo->rNanHaltComp,
			MSEC_TO_JIFFIES(4*1000));

		if (waitRet == 0) {
			DBGLOG(NAN, ERROR,
				"wait event timeout!\n");
			return FALSE;
		}

		prAdapter->fgIsNANCSAWaiting = FALSE;
		nanEnableRsp.status = nanDevEnableRequest(prAdapter,
							  &nanEnableReq);

		for (u4DelayIdx = 0; u4DelayIdx < 50; u4DelayIdx++) {
			if (g_deEvent == NAN_BSS_INDEX_NUM) {
				g_deEvent = 0;
				break;
			}
			msleep(100);
		}

		prNanScheduler = nanGetScheduler(prAdapter);
		prNanScheduler->fgNanInstantMode = FALSE;
		prNanScheduler->u4NanInstantModeChannel = 0;
		prNanScheduler->u4NanInstantModeBitmap = NAN_ICM_DEFAULT_BITMAP;

		prNanScheduler->fgNanInstantMode =
			nanEnableReq.fgNanInstantMode;
		prNanScheduler->u4NanInstantModeChannel =
			nanEnableReq.u4NanInstantModeChannel;
		/* TODO: set bitmap according to the value in request */
skip_enable:
		i4Status = kalIoctl(prGlueInfo, wlanoidNANEnableRsp,
				    (void *)&nanEnableRsp,
				    sizeof(struct NanEnableRequest), &u4BufLen);

		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}
		break;
	}
	case NAN_MSG_ID_DISABLE_REQ: {
		struct NanDisableRspMsg nanDisableRsp;
#if KERNEL_VERSION(5, 12, 0) > CFG80211_VERSION_CODE
		uint8_t fgRollbackRtnlLock = FALSE;
#endif

		kalMemZero(&nanDisableRsp, sizeof(struct NanDisableRspMsg));

		memcpy(&nanDisableRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanDisableRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanDisableRspMsg),
					   &nanDisableRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		ret = cfg80211_vendor_cmd_reply(skb);

		if (!prAdapter->fgIsNANRegistered) {
			DBGLOG(NAN, WARN, "NAN is already disabled\n");
			goto skip;
		}

		for (u4DelayIdx = 0; u4DelayIdx < 5; u4DelayIdx++) {
			/* Do not block to disable if not enable */
			if (g_disableNAN == TRUE || g_enableNAN == TRUE) {
				g_disableNAN = FALSE;
				break;
			}
			msleep(1000);
		}

		if (!wlanIsDriverReady(prGlueInfo,
			WLAN_DRV_READY_CHECK_WLAN_ON |
			WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
			DBGLOG(NAN, WARN, "driver is not ready\n");
			return -EFAULT;
		}

		if (prAdapter->rWifiVar.ucNanMaxNdpDissolve)
			nanNdpDissolve(prAdapter,
				prAdapter->rWifiVar.u4NanDissolveTimeout);

		nanDisableRsp.status =
			nanDevDisableRequest(prGlueInfo->prAdapter);

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		nan_wiphy_unlock(wiphy);
#else
		/* to avoid re-enter rtnl lock during
		 * register_netdev/unregister_netdev NAN/P2P
		 * we take away lock first and return later
		 */
		if (rtnl_is_locked()) {
			fgRollbackRtnlLock = TRUE;
			rtnl_unlock();
		}
#endif
		DBGLOG(NAN, TRACE,
			"[DBG] NAN disable, enter set_nan_handler, lock(%d)\n",
			rtnl_is_locked());
		set_nan_handler(wdev->netdev, 0, FALSE);
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
		nan_wiphy_lock(wiphy);
#else
		if (fgRollbackRtnlLock)
			rtnl_lock();
#endif

skip:

		i4Status = kalIoctl(prGlueInfo, wlanoidNANDisableRsp,
				    (void *)&nanDisableRsp,
				    sizeof(struct NanDisableRspMsg), &u4BufLen);

		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}

		break;
	}
	case NAN_MSG_ID_CONFIGURATION_REQ: {
		struct NanConfigRequest nanConfigReq;
		struct NanConfigRspMsg nanConfigRsp;

		kalMemZero(&nanConfigReq, sizeof(struct NanConfigRequest));
		kalMemZero(&nanConfigRsp, sizeof(struct NanConfigRspMsg));

		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_MASTER_PREFERENCE:
				if (outputTlv.length >
					sizeof(nanConfigReq.master_pref)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					return -EFAULT;
				}
				memcpy(&nanConfigReq.master_pref,
				       outputTlv.value, outputTlv.length);
				nanDevSetMasterPreference(
					prGlueInfo->prAdapter,
					nanConfigReq.master_pref);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		nanConfigRsp.status = 0;

		memcpy(&nanConfigRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanConfigRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanConfigRspMsg),
					   &nanConfigRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		i4Status = kalIoctl(prGlueInfo, wlanoidNANConfigRsp,
			(void *)&nanConfigRsp, sizeof(struct NanConfigRspMsg),
			&u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);
		break;
	}
	case NAN_MSG_ID_CAPABILITIES_REQ: {
		struct NanCapabilitiesRspMsg nanCapabilitiesRsp;

		memcpy(&nanCapabilitiesRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanCapabilitiesRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb,
					   sizeof(struct NanCapabilitiesRspMsg),
					   &nanCapabilitiesRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		i4Status = kalIoctl(prGlueInfo, wlanoidGetNANCapabilitiesRsp,
				    (void *)&nanCapabilitiesRsp,
				    sizeof(struct NanCapabilitiesRspMsg),
				    &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}
		DBGLOG(NAN, DEBUG, "i4Status = %u\n", i4Status);
		ret = cfg80211_vendor_cmd_reply(skb);

		break;
	}
	case NAN_MSG_ID_PUBLISH_SERVICE_REQ: {
		struct NanPublishRequest *pNanPublishReq = NULL;
		struct NanPublishServiceRspMsg *pNanPublishRsp = NULL;
		uint16_t publish_id = 0;
		uint8_t ucCipherType = 0;

		DBGLOG(NAN, INFO, "IN case NAN_MSG_ID_PUBLISH_SERVICE_REQ\n");

		pNanPublishReq =
			kmalloc(sizeof(struct NanPublishRequest), GFP_ATOMIC);

		if (!pNanPublishReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		pNanPublishRsp = kmalloc(sizeof(struct NanPublishServiceRspMsg),
					 GFP_ATOMIC);

		if (!pNanPublishRsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanPublishReq);
			return -ENOMEM;
		}

		kalMemZero(pNanPublishReq, sizeof(struct NanPublishRequest));
		kalMemZero(pNanPublishRsp,
			   sizeof(struct NanPublishServiceRspMsg));

		/* Mapping publish req related parameters */
		readLen = nanMapPublishReqParams((u16 *)data, pNanPublishReq);
		remainingLen -= readLen;
		data += readLen;

		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_SERVICE_NAME:
				if (outputTlv.length >
					NAN_MAX_SERVICE_NAME_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memset(g_aucNanServiceName, 0,
					NAN_MAX_SERVICE_NAME_LEN);
				memcpy(pNanPublishReq->service_name,
				       outputTlv.value, outputTlv.length);
				memcpy(g_aucNanServiceName,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->service_name_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"type:SERVICE_NAME:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				break;
			case NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memcpy(pNanPublishReq->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->service_specific_info_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"type:SERVICE_SPECIFIC_INFO:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				break;
			case NAN_TLV_TYPE_RX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_FW_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length %d is invalid!\n",
						outputTlv.length);
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memcpy(pNanPublishReq->rx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->rx_match_filter_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"type:RX_MATCH_FILTER:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				dumpMemory8(
					(uint8_t *)
						pNanPublishReq->rx_match_filter,
					pNanPublishReq->rx_match_filter_len);
				break;
			case NAN_TLV_TYPE_TX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_FW_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length %d is invalid!\n",
						outputTlv.length);
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memcpy(pNanPublishReq->tx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->tx_match_filter_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"type:TX_MATCH_FILTER:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				dumpMemory8(
					(uint8_t *)
						pNanPublishReq->tx_match_filter,
					pNanPublishReq->tx_match_filter_len);
				break;
			case NAN_TLV_TYPE_NAN_SERVICE_ACCEPT_POLICY:
				pNanPublishReq->service_responder_policy =
					*(outputTlv.value);
				DBGLOG(NAN, DEBUG,
					"type:SERVICE_ACCEPT_POLICY:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				break;
			case NAN_TLV_TYPE_NAN_CSID:
				pNanPublishReq->cipher_type =
					*(outputTlv.value);
				break;
			case NAN_TLV_TYPE_NAN_PMK:
				if (outputTlv.length >
					NAN_PMK_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memcpy(pNanPublishReq->key_info.body.pmk_info
					       .pmk,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->key_info.body.pmk_info.pmk_len =
					outputTlv.length;
				break;
			case NAN_TLV_TYPE_NAN_PASSPHRASE:
				if (outputTlv.length >
					NAN_SECURITY_MAX_PASSPHRASE_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				memcpy(pNanPublishReq->key_info.body
					       .passphrase_info.passphrase,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->key_info.body.passphrase_info
					.passphrase_len = outputTlv.length;
				break;
			case NAN_TLV_TYPE_SDEA_CTRL_PARAMS:
				nanMapSdeaCtrlParams(
					(u32 *)outputTlv.value,
					&pNanPublishReq->sdea_params);
				DBGLOG(NAN, DEBUG,
					"type:_SDEA_CTRL_PARAMS:%u Len:%u\n",
					outputTlv.type, outputTlv.length);

				break;
			case NAN_TLV_TYPE_NAN_RANGING_CFG:
				nanMapRangingConfigParams(
					(u32 *)outputTlv.value,
					&pNanPublishReq->ranging_cfg);
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				pNanPublishReq->sdea_service_specific_info[0] =
					*(outputTlv.value);
				pNanPublishReq->sdea_service_specific_info_len =
					outputTlv.length;
				break;
			case NAN_TLV_TYPE_NAN20_RANGING_REQUEST:
				nanMapNan20RangingReqParams(
					prAdapter,
					(u32 *)outputTlv.value,
					&pNanPublishReq->range_response_cfg);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		/* Publish response message */
		memcpy(&pNanPublishRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanPublishServiceRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR,
				"Allocate skb failed\n");
			kfree(pNanPublishRsp);
			kfree(pNanPublishReq);
			return -ENOMEM;
		}

		if (unlikely(nla_put_nohdr(
			skb,
			sizeof(struct NanPublishServiceRspMsg),
			pNanPublishRsp) < 0)) {
			kfree_skb(skb);
			kfree(pNanPublishRsp);
			kfree(pNanPublishReq);
			DBGLOG(NAN, ERROR, "Fail send reply\n");
			return -EFAULT;
		}
		/* WIFI HAL will set nanMsgHdr.handle to 0xFFFF
		 * if publish id is 0. (means new publish) Otherwise set
		 * to previous publish id.
		 */
		if (nanMsgHdr.handle != 0xFFFF)
			pNanPublishReq->publish_id = nanMsgHdr.handle;

		/* return publish ID */
		publish_id = (uint16_t)nanPublishRequest(prGlueInfo->prAdapter,
							pNanPublishReq);
		/* NAN_CHK_PNT log message */
		if (nanMsgHdr.handle == 0xFFFF)
			nanLogPublish(publish_id);

		pNanPublishRsp->fwHeader.handle = publish_id;
		DBGLOG(NAN, INFO,
			"pNanPublishRsp->fwHeader.handle %u, publish_id : %u\n",
			pNanPublishRsp->fwHeader.handle, publish_id);

		if (pNanPublishReq->sdea_params.security_cfg &&
			publish_id != 0) {
			/* Fixme: supply a cipher suite list */
			ucCipherType = pNanPublishReq->cipher_type;
			nanCmdAddCsid(
				prGlueInfo->prAdapter,
				publish_id,
				1,
				&ucCipherType);
			nanSetPublishPmkid(
				prGlueInfo->prAdapter,
				pNanPublishReq);
			if (pNanPublishReq->scid_len) {
				if (pNanPublishReq->scid_len
						> NAN_SCID_DEFAULT_LEN)
					pNanPublishReq->scid_len
						= NAN_SCID_DEFAULT_LEN;
				nanCmdManageScid(
					prGlueInfo->prAdapter,
					TRUE,
					publish_id,
					pNanPublishReq->scid);
			}
		}

		nanExtTerminateApNanEndLegacy(prAdapter);

		i4Status = kalIoctl(prGlueInfo, wlanoidNanPublishRsp,
				    (void *)pNanPublishRsp,
				    sizeof(struct NanPublishServiceRspMsg),
				    &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			kfree_skb(skb);
			kfree(pNanPublishReq);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanPublishReq);
		break;
	}
	case NAN_MSG_ID_PUBLISH_SERVICE_CANCEL_REQ: {
		uint32_t rStatus;
		struct NanPublishCancelRequest *pNanPublishCancelReq = NULL;
		struct NanPublishServiceCancelRspMsg *pNanPublishCancelRsp =
			NULL;

		pNanPublishCancelReq = kmalloc(
			sizeof(struct NanPublishCancelRequest), GFP_ATOMIC);

		if (!pNanPublishCancelReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		pNanPublishCancelRsp =
			kmalloc(sizeof(struct NanPublishServiceCancelRspMsg),
				GFP_ATOMIC);

		if (!pNanPublishCancelRsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanPublishCancelReq);
			return -ENOMEM;
		}

		DBGLOG(NAN, DEBUG, "Enter CANCEL Publish Request\n");
		pNanPublishCancelReq->publish_id = nanMsgHdr.handle;

		DBGLOG(NAN, DEBUG,
		       "PID %d\n", pNanPublishCancelReq->publish_id);
		rStatus = nanCancelPublishRequest(prGlueInfo->prAdapter,
						  pNanPublishCancelReq);

		/* Prepare for command reply */
		memcpy(&pNanPublishCancelRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanPublishServiceCancelRspMsg));
		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			kfree(pNanPublishCancelReq);
			kfree(pNanPublishCancelRsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb, sizeof(struct
						 NanPublishServiceCancelRspMsg),
				     pNanPublishCancelRsp) < 0)) {
			kfree_skb(skb);
			kfree(pNanPublishCancelReq);
			kfree(pNanPublishCancelRsp);
			return -EFAULT;
		}

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, DEBUG,
			       "CANCEL Publish Error %x\n", rStatus);
			pNanPublishCancelRsp->status = NAN_I_STATUS_DE_FAILURE;
		} else {
			DBGLOG(NAN, DEBUG, "CANCEL Publish Success %x\n",
			       rStatus);
			pNanPublishCancelRsp->status = NAN_I_STATUS_SUCCESS;
		}

		i4Status =
			kalIoctl(prGlueInfo, wlanoidNANCancelPublishRsp,
				 (void *)pNanPublishCancelRsp,
				 sizeof(struct NanPublishServiceCancelRspMsg),
				 &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			kfree_skb(skb);
			kfree(pNanPublishCancelReq);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanPublishCancelReq);
		break;
	}
	case NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ: {
		struct NanSubscribeRequest *pNanSubscribeReq = NULL;
		struct NanSubscribeServiceRspMsg *pNanSubscribeRsp = NULL;
		bool fgRangingCFG = FALSE;
		bool fgRangingREQ = FALSE;
		uint16_t Subscribe_id = 0;
		int i = 0;

		DBGLOG(NAN, DEBUG, "In NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ\n");

		pNanSubscribeReq =
			kmalloc(sizeof(struct NanSubscribeRequest), GFP_ATOMIC);

		if (!pNanSubscribeReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanSubscribeRsp = kmalloc(
			sizeof(struct NanSubscribeServiceRspMsg), GFP_ATOMIC);

		if (!pNanSubscribeRsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanSubscribeReq);
			return -ENOMEM;
		}
		kalMemZero(pNanSubscribeReq,
			   sizeof(struct NanSubscribeRequest));
		kalMemZero(pNanSubscribeRsp,
			   sizeof(struct NanSubscribeServiceRspMsg));

		/* WIFI HAL will set nanMsgHdr.handle to 0xFFFF
		 * if subscribe_id is 0. (means new subscribe)
		 */
		if (nanMsgHdr.handle != 0xFFFF)
			pNanSubscribeReq->subscribe_id = nanMsgHdr.handle;

		/* Mapping subscribe req related parameters */
		readLen =
			nanMapSubscribeReqParams((u16 *)data, pNanSubscribeReq);
		remainingLen -= readLen;
		data += readLen;
		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_SERVICE_NAME:
				if (outputTlv.length >
					NAN_MAX_SERVICE_NAME_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memset(g_aucNanServiceName, 0,
					NAN_MAX_SERVICE_NAME_LEN);
				memcpy(pNanSubscribeReq->service_name,
				       outputTlv.value, outputTlv.length);
				memcpy(g_aucNanServiceName,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->service_name_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"SERVICE_NAME type:%u len:%u SRV_name:%s\n",
					outputTlv.type,
					outputTlv.length,
					pNanSubscribeReq->service_name);
				break;
			case NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->service_specific_info_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"SERVICE_SPECIFIC_INFO type:%u len:%u SRV_spec_info:%s\n",
					outputTlv.type,
					outputTlv.length,
					pNanSubscribeReq
					->service_specific_info);
				break;
			case NAN_TLV_TYPE_RX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_FW_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length %d is invalid!\n",
						outputTlv.length);
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq->rx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->rx_match_filter_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"RX_MATCH_FILTER type:%u len:%u rx_match_filter:%s\n",
					outputTlv.type,
					outputTlv.length,
					pNanSubscribeReq->rx_match_filter);
				dumpMemory8((uint8_t *)pNanSubscribeReq
						    ->rx_match_filter,
					    outputTlv.length);
				break;
			case NAN_TLV_TYPE_TX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_FW_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length %d is invalid!\n",
						outputTlv.length);
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq->tx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->tx_match_filter_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"TX_MATCH_FILTERtype:%u len:%u tx_match_filter:%s\n",
					outputTlv.type,
					outputTlv.length,
					pNanSubscribeReq->tx_match_filter);
				dumpMemory8((uint8_t *)pNanSubscribeReq
						    ->tx_match_filter,
					    outputTlv.length);
				break;
			case NAN_TLV_TYPE_MAC_ADDRESS:
				if (outputTlv.length >
					sizeof(uint8_t)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				if (i < NAN_MAX_SUBSCRIBE_MAX_ADDRESS) {
					/* Get column neumbers */
					memcpy(pNanSubscribeReq->intf_addr[i],
					     outputTlv.value, outputTlv.length);
					i++;
				}
				break;
			case NAN_TLV_TYPE_NAN_CSID:
				pNanSubscribeReq->cipher_type =
					*(outputTlv.value);
				DBGLOG(NAN, DEBUG, "NAN_CSID type:%u len:%u\n",
				       outputTlv.type, outputTlv.length);
				break;
			case NAN_TLV_TYPE_NAN_PMK:
				if (outputTlv.length >
					NAN_PMK_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq->key_info.body.pmk_info
					       .pmk,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->key_info.body.pmk_info
					.pmk_len = outputTlv.length;
				break;
			case NAN_TLV_TYPE_NAN_PASSPHRASE:
				if (outputTlv.length >
					NAN_SECURITY_MAX_PASSPHRASE_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq->key_info.body
					       .passphrase_info.passphrase,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->key_info.body.passphrase_info
					.passphrase_len = outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"NAN_PASSPHRASE type:%u len:%u\n",
					outputTlv.type,
					outputTlv.length);
				break;
			case NAN_TLV_TYPE_SDEA_CTRL_PARAMS:
				nanMapSdeaCtrlParams(
					(u32 *)outputTlv.value,
					&pNanSubscribeReq->sdea_params);
				DBGLOG(NAN, DEBUG,
					"SDEA_CTRL_PARAMS type:%u len:%u\n",
					outputTlv.type,
					outputTlv.length);

				break;
			case NAN_TLV_TYPE_NAN_RANGING_CFG:
				fgRangingCFG = TRUE;
				DBGLOG(NAN, DEBUG, "fgRangingCFG %d\n",
					fgRangingCFG);
				nanMapRangingConfigParams(
					(u32 *)outputTlv.value,
					&pNanSubscribeReq->ranging_cfg);
				pNanSubscribeReq->ranging_enabled = TRUE;
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_MAX_SDEA_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				memcpy(pNanSubscribeReq
					->sdea_service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq
					->sdea_service_specific_info_len =
					outputTlv.length;
				DBGLOG(NAN, DEBUG,
					"SDEA_SERVICE_SPECIFIC_INFO type:%u len:%u\n",
					outputTlv.type,
					outputTlv.length);

				break;
			case NAN_TLV_TYPE_NAN20_RANGING_REQUEST:
				fgRangingREQ = TRUE;
				DBGLOG(NAN, DEBUG, "fgRangingREQ %d\n",
					fgRangingREQ);
				nanMapNan20RangingReqParams(
					prAdapter,
					(u32 *)outputTlv.value,
					&pNanSubscribeReq->range_response_cfg);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		/* Prepare command reply of Subscriabe response */
		memcpy(&pNanSubscribeRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanSubscribeServiceRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			kfree(pNanSubscribeReq);
			kfree(pNanSubscribeRsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct NanSubscribeServiceRspMsg),
				     pNanSubscribeRsp) < 0)) {
			kfree(pNanSubscribeReq);
			kfree(pNanSubscribeRsp);
			kfree_skb(skb);
			return -EFAULT;
		}

		prAdapter->fgIsNANfromHAL = TRUE;

		/* return subscribe ID */
		Subscribe_id = (uint16_t)nanSubscribeRequest(
			prGlueInfo->prAdapter, pNanSubscribeReq);
		/* NAN_CHK_PNT log message */
		if (nanMsgHdr.handle == 0xFFFF)
			nanLogSubscribe(Subscribe_id);

		pNanSubscribeRsp->fwHeader.handle = Subscribe_id;

		DBGLOG(NAN, INFO,
		       "Subscribe_id:%u, pNanSubscribeRsp->fwHeader.handle:%u\n",
		       Subscribe_id, pNanSubscribeRsp->fwHeader.handle);
		i4Status = kalIoctl(prGlueInfo, wlanoidNanSubscribeRsp,
				    (void *)pNanSubscribeRsp,
				    sizeof(struct NanSubscribeServiceRspMsg),
				    &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			kfree(pNanSubscribeReq);
			kfree_skb(skb);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanSubscribeReq);
		break;
	}
	case NAN_MSG_ID_SUBSCRIBE_SERVICE_CANCEL_REQ: {
		uint32_t rStatus;
		struct NanSubscribeCancelRequest *pNanSubscribeCancelReq = NULL;
		struct NanSubscribeServiceCancelRspMsg *pNanSubscribeCancelRsp =
			NULL;

		pNanSubscribeCancelReq = kmalloc(
			sizeof(struct NanSubscribeCancelRequest), GFP_ATOMIC);
		if (!pNanSubscribeCancelReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		pNanSubscribeCancelRsp =
			kmalloc(sizeof(struct NanSubscribeServiceCancelRspMsg),
				GFP_ATOMIC);
		if (!pNanSubscribeCancelRsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanSubscribeCancelReq);
			return -ENOMEM;
		}
		kalMemZero(pNanSubscribeCancelReq,
			   sizeof(struct NanSubscribeCancelRequest));
		kalMemZero(pNanSubscribeCancelRsp,
			   sizeof(struct NanSubscribeServiceCancelRspMsg));

		DBGLOG(NAN, DEBUG, "Enter CANCEL Subscribe Request\n");
		pNanSubscribeCancelReq->subscribe_id = nanMsgHdr.handle;

		DBGLOG(NAN, DEBUG, "PID %d\n",
		       pNanSubscribeCancelReq->subscribe_id);
		rStatus = nanCancelSubscribeRequest(prGlueInfo->prAdapter,
						    pNanSubscribeCancelReq);

		/* Prepare Cancel Subscribe command reply message */
		memcpy(&pNanSubscribeCancelRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanSubscribeServiceCancelRspMsg));
		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			kfree(pNanSubscribeCancelReq);
			kfree(pNanSubscribeCancelRsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct
					    NanSubscribeServiceCancelRspMsg),
				     pNanSubscribeCancelRsp) < 0)) {
			kfree(pNanSubscribeCancelReq);
			kfree(pNanSubscribeCancelRsp);
			kfree_skb(skb);
			return -EFAULT;
		}

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "CANCEL Subscribe Error %X\n",
			       rStatus);
			pNanSubscribeCancelRsp->status =
				NAN_I_STATUS_DE_FAILURE;
		} else {
			DBGLOG(NAN, DEBUG, "CANCEL Subscribe Success %X\n",
			       rStatus);
			pNanSubscribeCancelRsp->status = NAN_I_STATUS_SUCCESS;
		}

		i4Status =
			kalIoctl(prGlueInfo, wlanoidNANCancelSubscribeRsp,
				 (void *)pNanSubscribeCancelRsp,
				 sizeof(struct NanSubscribeServiceCancelRspMsg),
				 &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			kfree(pNanSubscribeCancelReq);
			kfree_skb(skb);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanSubscribeCancelReq);
		break;
	}
	case NAN_MSG_ID_TRANSMIT_FOLLOWUP_REQ: {
		uint32_t rStatus;
		struct NanTransmitFollowupRequest *pNanXmitFollowupReq = NULL;
		struct NanTransmitFollowupRspMsg *pNanXmitFollowupRsp = NULL;

		pNanXmitFollowupReq = kmalloc(
			sizeof(struct NanTransmitFollowupRequest), GFP_ATOMIC);

		if (!pNanXmitFollowupReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		pNanXmitFollowupRsp = kmalloc(
			sizeof(struct NanTransmitFollowupRspMsg), GFP_ATOMIC);
		if (!pNanXmitFollowupRsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanXmitFollowupReq);
			return -ENOMEM;
		}
		kalMemZero(pNanXmitFollowupReq,
			   sizeof(struct NanTransmitFollowupRequest));
		kalMemZero(pNanXmitFollowupRsp,
			   sizeof(struct NanTransmitFollowupRspMsg));

		DBGLOG(NAN, INFO, "Enter Transmit follow up Request\n");

		/* Mapping publish req related parameters */
		readLen = nanMapFollowupReqParams((u32 *)data,
						  pNanXmitFollowupReq);
		remainingLen -= readLen;
		data += readLen;
		pNanXmitFollowupReq->publish_subscribe_id = nanMsgHdr.handle;

		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_MAC_ADDRESS:
				if (outputTlv.length >
					NAN_MAC_ADDR_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanXmitFollowupReq);
					kfree(pNanXmitFollowupRsp);
					return -EFAULT;
				}
				memcpy(pNanXmitFollowupReq->addr,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_MAX_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanXmitFollowupReq);
					kfree(pNanXmitFollowupRsp);
					return -EFAULT;
				}
				memcpy(pNanXmitFollowupReq
					       ->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanXmitFollowupReq->service_specific_info_len =
					outputTlv.length;
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_FW_MAX_FOLLOW_UP_SDEA_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanXmitFollowupReq);
					kfree(pNanXmitFollowupRsp);
					return -EFAULT;
				}
				memcpy(pNanXmitFollowupReq
					       ->sdea_service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanXmitFollowupReq
					->sdea_service_specific_info_len =
					outputTlv.length;
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		/* Follow up Command reply message */
		memcpy(&pNanXmitFollowupRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));

		pNanXmitFollowupReq->transaction_id =
			pNanXmitFollowupRsp->fwHeader.transactionId;

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanTransmitFollowupRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			kfree(pNanXmitFollowupReq);
			kfree(pNanXmitFollowupRsp);
			return -ENOMEM;
		}

		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct NanTransmitFollowupRspMsg),
				     pNanXmitFollowupRsp) < 0)) {
			kfree(pNanXmitFollowupReq);
			kfree(pNanXmitFollowupRsp);
			kfree_skb(skb);
			DBGLOG(NAN, ERROR, "Fail send reply\n");
			return -EFAULT;
		}

		rStatus = nanTransmitRequest(prGlueInfo->prAdapter,
					     pNanXmitFollowupReq);
		if (rStatus != WLAN_STATUS_SUCCESS)
			pNanXmitFollowupRsp->status = NAN_I_STATUS_DE_FAILURE;
		else
			pNanXmitFollowupRsp->status = NAN_I_STATUS_SUCCESS;

		nanExtTerminateApNanEndLegacy(prAdapter);

		i4Status = kalIoctl(prGlueInfo, wlanoidNANFollowupRsp,
				    (void *)pNanXmitFollowupRsp,
				    sizeof(struct NanTransmitFollowupRspMsg),
				    &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "kalIoctl failed\n");
			kfree(pNanXmitFollowupReq);
			kfree_skb(skb);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanXmitFollowupReq);
		break;
	}
	case NAN_MSG_ID_BEACON_SDF_REQ: {
		u16 vsa_length = 0;
		u32 *pXmitVSAparms = NULL;
		struct NanTransmitVendorSpecificAttribute *pNanXmitVSAttrReq =
			NULL;
		struct NanBeaconSdfPayloadRspMsg *pNanBcnSdfVSARsp = NULL;

		DBGLOG(NAN, DEBUG, "Enter Beacon SDF Request.\n");

		pNanXmitVSAttrReq = kmalloc(
			sizeof(struct NanTransmitVendorSpecificAttribute),
			GFP_ATOMIC);

		if (!pNanXmitVSAttrReq) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanBcnSdfVSARsp = kmalloc(
			sizeof(struct NanBeaconSdfPayloadRspMsg), GFP_ATOMIC);

		if (!pNanBcnSdfVSARsp) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			kfree(pNanXmitVSAttrReq);
			return -ENOMEM;
		}

		while ((remainingLen >= 4) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_VENDOR_SPECIFIC_ATTRIBUTE_TRANSMIT:
				pXmitVSAparms = (u32 *)outputTlv.value;
				pNanXmitVSAttrReq->payload_transmit_flag =
					(u8)(*pXmitVSAparms & BIT(0));
				pNanXmitVSAttrReq->tx_in_discovery_beacon =
					(u8)(*pXmitVSAparms & BIT(1));
				pNanXmitVSAttrReq->tx_in_sync_beacon =
					(u8)(*pXmitVSAparms & BIT(2));
				pNanXmitVSAttrReq->tx_in_service_discovery =
					(u8)(*pXmitVSAparms & BIT(3));
				pNanXmitVSAttrReq->vendor_oui =
					*pXmitVSAparms & BITS(8, 31);
				outputTlv.value += 4;

				vsa_length = outputTlv.length - sizeof(u32);
				if (vsa_length >
					NAN_MAX_VSA_DATA_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanXmitVSAttrReq);
					kfree(pNanBcnSdfVSARsp);
					return -EFAULT;
				}
				memcpy(pNanXmitVSAttrReq->vsa, outputTlv.value,
				       vsa_length);
				pNanXmitVSAttrReq->vsa_len = vsa_length;
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		/* To be implement
		 * Beacon SDF VSA request.................................
		 * rStatus = ;
		 */

		/* Prepare Beacon Sdf Payload Response */
		memcpy(&pNanBcnSdfVSARsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		pNanBcnSdfVSARsp->fwHeader.msgId = NAN_MSG_ID_BEACON_SDF_RSP;
		pNanBcnSdfVSARsp->fwHeader.msgLen =
			sizeof(struct NanBeaconSdfPayloadRspMsg);

		pNanBcnSdfVSARsp->status = NAN_I_STATUS_SUCCESS;

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanBeaconSdfPayloadRspMsg));

		if (!skb) {
			DBGLOG(NAN, ERROR, "Allocate skb failed\n");
			kfree(pNanXmitVSAttrReq);
			kfree(pNanBcnSdfVSARsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct NanBeaconSdfPayloadRspMsg),
				     pNanBcnSdfVSARsp) < 0)) {
			kfree(pNanXmitVSAttrReq);
			kfree(pNanBcnSdfVSARsp);
			kfree_skb(skb);
			return -EFAULT;
		}

		ret = cfg80211_vendor_cmd_reply(skb);

		kfree(pNanXmitVSAttrReq);
		kfree(pNanBcnSdfVSARsp);

		break;
	}
	case NAN_MSG_ID_TESTMODE_REQ:
	{
		struct NanDebugParams *pNanDebug = NULL;

		pNanDebug = kmalloc(sizeof(struct NanDebugParams), GFP_ATOMIC);
		if (!pNanDebug) {
			DBGLOG(NAN, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		kalMemZero(pNanDebug, sizeof(struct NanDebugParams));
		DBGLOG(NAN, DEBUG, "NAN_MSG_ID_TESTMODE_REQ\n");

		while ((remainingLen >= 4) &&
			(0 != (readLen = nan_read_tlv((u8 *)data,
			&outputTlv)))) {
			DBGLOG(NAN, DEBUG, "outputTlv.type= %d\n",
				outputTlv.type);
			if (outputTlv.type ==
				NAN_TLV_TYPE_TESTMODE_GENERIC_CMD) {
				if (outputTlv.length >
					sizeof(
					struct NanDebugParams
					)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanDebug);
					return -EFAULT;
				}
				memcpy(pNanDebug, outputTlv.value,
					outputTlv.length);
				switch (pNanDebug->cmd) {
				case NAN_TEST_MODE_CMD_DISABLE_NDPE:
					g_ndpReqNDPE.fgEnNDPE = TRUE;
					g_ndpReqNDPE.ucNDPEAttrPresent =
						pNanDebug->
						debug_cmd_data[0];
					DBGLOG(NAN, DEBUG,
						"NAN_TEST_MODE_CMD_DISABLE_NDPE: fgEnNDPE = %d\n",
						g_ndpReqNDPE.fgEnNDPE);
					break;
				default:
					break;
				}
			} else {
				DBGLOG(NAN, ERROR,
					"Testmode invalid TLV type\n");
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		kfree(pNanDebug);
		return 0;
	}
	case NAN_MSG_ID_GET_COUNTRY_CODE:
	{
		struct NanGetCountryCodeRspMsg rCountryCode;
		uint32_t u4CountryCode = 0;
		char acCountryStr[MAX_COUNTRY_CODE_LEN + 1] = {0};

		kalMemZero(&rCountryCode,
			sizeof(struct NanGetCountryCodeRspMsg));
		kalMemCopy(&rCountryCode.fwHeader, &nanMsgHdr,
			sizeof(struct _NanMsgHeader));

		u4CountryCode = rlmDomainGetCountryCode(prAdapter);
		rlmDomainU32ToAlpha(u4CountryCode, acCountryStr);

		rCountryCode.countryCode[0] = acCountryStr[0];
		rCountryCode.countryCode[1] = acCountryStr[1];

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanGetCountryCodeRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb,
			sizeof(struct NanGetCountryCodeRspMsg),
					&rCountryCode) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		break;
	}
	case NAN_MSG_ID_GET_INFRA_BSSID:
	{
		struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;
		struct NanGetInfraBssidRspMsg rInfraBssid;

		prAisBssInfo = aisGetAisBssInfo(prAdapter, AIS_DEFAULT_INDEX);
		if (prAisBssInfo == NULL) {
			DBGLOG(REQ, ERROR, "prAisBssInfo is null\n");
			return -EFAULT;
		}

		kalMemCopy(&rInfraBssid.fwHeader, &nanMsgHdr,
			sizeof(struct _NanMsgHeader));
		if (prAisBssInfo->eConnectionState ==
			MEDIA_STATE_CONNECTED) {
			COPY_MAC_ADDR(rInfraBssid.MacAddr,
				prAisBssInfo->aucBSSID);
		} else {
			kalMemSet(rInfraBssid.MacAddr, 0, MAC_ADDR_LEN);
		}

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanGetInfraBssidRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb,
			sizeof(struct NanGetInfraBssidRspMsg),
					   &rInfraBssid) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		break;
	}
	case NAN_MSG_ID_GET_INFRA_CHANNEL:
	{
		struct BSS_INFO *prAisBssInfo = (struct BSS_INFO *) NULL;
		struct NanGetInfraChannelRspMsg rInfraCh;

		prAisBssInfo = aisGetAisBssInfo(prAdapter, AIS_DEFAULT_INDEX);
		if (prAisBssInfo == NULL) {
			DBGLOG(REQ, ERROR, "prAisBssInfo is null\n");
			return -EFAULT;
		}

		kalMemCopy(&rInfraCh.fwHeader, &nanMsgHdr,
			sizeof(struct _NanMsgHeader));
		if (prAisBssInfo->eConnectionState ==
			MEDIA_STATE_CONNECTED) {
			rInfraCh.Channel = prAisBssInfo->ucPrimaryChannel;
			rInfraCh.Flag = 0;
			/* Bandwidth */
			if (prAisBssInfo->ucVhtChannelWidth ==
				VHT_OP_CHANNEL_WIDTH_20_40) {
				if (prAisBssInfo->eBssSCO ==
					CHNL_EXT_SCN) {
					rInfraCh.Flag |=
						NAN_C_FLAG_20MHZ;
				} else if (prAisBssInfo->eBssSCO ==
				CHNL_EXT_SCA) {
					rInfraCh.Flag |=
						NAN_C_FLAG_40MHZ;
					rInfraCh.Flag |=
						NAN_C_FLAG_EXTENSION_ABOVE;
				} else if (prAisBssInfo->eBssSCO ==
				CHNL_EXT_SCB) {
					rInfraCh.Flag |=
						NAN_C_FLAG_40MHZ;
				}
			} else if (prAisBssInfo->ucVhtChannelWidth ==
				VHT_OP_CHANNEL_WIDTH_80) {
				rInfraCh.Flag |=
					NAN_C_FLAG_80MHZ;
			} else if (prAisBssInfo->ucVhtChannelWidth ==
				VHT_OP_CHANNEL_WIDTH_160) {
				rInfraCh.Flag |=
					NAN_C_FLAG_160MHZ;
			}
			/* Band */
			if (prAisBssInfo->eBand == BAND_2G4)
				rInfraCh.Flag |= NAN_C_FLAG_2GHZ;
			else if (prAisBssInfo->eBand == BAND_5G)
				rInfraCh.Flag |= NAN_C_FLAG_5GHZ;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prAisBssInfo->eBand == BAND_6G)
				rInfraCh.Flag |= NAN_C_FLAG_6GHZ;
#endif /* CFG_SUPPORT_WIFI_6G */
		} else {
			rInfraCh.Channel = 0;
			rInfraCh.Flag = 0;
		}

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanGetInfraChannelRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb,
			sizeof(struct NanGetInfraChannelRspMsg),
					   &rInfraCh) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		break;
	}
	case NAN_MSG_ID_GET_DRIVER_CAPABILITIES:
	{
		struct NanDriverCapabilitiesRspMsg rDriverCapabilities;

		kalMemZero(&rDriverCapabilities,
			sizeof(struct NanDriverCapabilitiesRspMsg));
		kalMemCopy(&rDriverCapabilities.fwHeader, &nanMsgHdr,
			sizeof(struct _NanMsgHeader));

		rDriverCapabilities.capabilities =
			WFPAL_WIFI_DRIVER_SUPPORTS_NAN |
			WFPAL_WIFI_DRIVER_SUPPORTS_DUAL_BAND;
		/* nonDBDC case shall be 0 */
		rDriverCapabilities.capabilities |=
			(prAdapter->rWifiVar.eDbdcMode > 0) ?
			WFPAL_WIFI_DRIVER_SUPPORTS_SIMULTANEOUS_DUAL_BAND : 0;

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanDriverCapabilitiesRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb,
			sizeof(struct NanDriverCapabilitiesRspMsg),
					&rDriverCapabilities) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		break;
	}

	case NAN_MSG_ID_SET_COMMITTED_AVAILABILITY:
	{
		struct _NanCommittedAvailability *prCommittedAvailability;
		struct _NanChannelAvailabilityEntry *prChnlEntry;
		union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};
		union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl = {0};
		uint8_t ucMapId, ucNumMaps, ucNumChnlEntries;
		uint8_t ucMapIdx, ucChnlEntryIdx;
		//size_t i, j;
		size_t message_len = 0;
		unsigned char fgNonContinuousBw = FALSE;
		uint16_t u2TimeBitmapControl = 0;
		uint32_t au4AvailMap[NAN_TOTAL_DW] = {0};

		DBGLOG(REQ, INFO,
			"GET NAN_MSG_ID_SET_COMMITTED_AVAILABILITY\n");

		message_len = sizeof(struct _NanCommittedAvailability);
		prCommittedAvailability = kalMemAlloc(message_len,
			VIR_MEM_TYPE);

		if (!prCommittedAvailability) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		kalMemZero(prCommittedAvailability, message_len);
		kalMemCopy(prCommittedAvailability, (u8 *)data, message_len);

#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
		nanSchedResetCommitedAvailability(prAdapter);
#endif

		ucNumMaps = prCommittedAvailability->num_maps_ids;

		for (ucMapIdx = 0; ucMapIdx < ucNumMaps; ucMapIdx++) {
			ucMapId =
				prCommittedAvailability
				->schedule[ucMapIdx].map_id;
			ucNumChnlEntries =
				prCommittedAvailability
				->schedule[ucMapIdx].num_entries;

			DBGLOG(NAN, INFO, "MapId:%d, MapNum:%d, EntryNumb:%d\n",
					ucMapId, ucNumMaps, ucNumChnlEntries);

			if ((ucMapIdx >= NAN_MAX_MAP_IDS) ||
				(ucMapIdx >= NAN_TIMELINE_MGMT_SIZE)) {
				DBGLOG(NAN, ERROR,
					"Map number (%d, %d) exceed cap\n",
					ucMapIdx, ucNumMaps);
				kalMemFree(
					prCommittedAvailability,
					VIR_MEM_TYPE,
					message_len);
				return -EINVAL;
			}

			for (ucChnlEntryIdx = 0;
				ucChnlEntryIdx < ucNumChnlEntries;
				ucChnlEntryIdx++) {
				if ((ucChnlEntryIdx >=
				NAN_MAX_AVAILABILITY_CHANNEL_ENTRIES) ||
				(ucChnlEntryIdx >=
				NAN_TIMELINE_MGMT_CHNL_LIST_NUM)) {
					DBGLOG(NAN, ERROR,
					  "Chnl number (%d, %d) exceed cap\n",
					  ucChnlEntryIdx,
					  ucNumChnlEntries);
					kalMemFree(
						prCommittedAvailability,
						VIR_MEM_TYPE,
						message_len);
					return -EINVAL;
				}

				prChnlEntry =
					&(prCommittedAvailability
					->schedule[ucMapIdx]
					.channel_entries[ucChnlEntryIdx]);

				/* convert to _NAN_BAND_CHNL_CTRL */
				if (prChnlEntry->auxiliary_channel_bitmap !=
					0) {
					fgNonContinuousBw = TRUE;
					rChnlInfo.u4AuxCenterChnl =
					  nanRegGetChannelByOrder(
					    prChnlEntry->op_class,
					    &prChnlEntry
					    ->auxiliary_channel_bitmap);
				}
				rChnlInfo.u4Type =
					NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
				rChnlInfo.u4OperatingClass =
					prChnlEntry->op_class;
				rChnlInfo.u4PrimaryChnl =
					nanRegGetPrimaryChannelByOrder(
						prChnlEntry->op_class,
						&prChnlEntry->op_class_bitmap,
						fgNonContinuousBw,
						prChnlEntry
						->primary_channel_bitmap);

				/* convert to _NAN_AVAIL_ENTRY_CTRL */
				rEntryCtrl.b3Type =
				NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT;
				rEntryCtrl.b2Preference =
					prChnlEntry->usage_preference;
				rEntryCtrl.b3Util =
					prChnlEntry->utilization;
				rEntryCtrl.b4RxNss =
					prChnlEntry->rx_nss;
				rEntryCtrl.b1TimeMapAvail =
					((prChnlEntry
					->time_bitmap.time_bitmap_length
					== 0) ? 0 : 1);

				/* convert to au4AvailMap */
				kalMemZero(au4AvailMap, sizeof(au4AvailMap));
				if (rEntryCtrl.b1TimeMapAvail == 0) {
					kalMemSet(au4AvailMap, 0xFF,
						sizeof(au4AvailMap));
				} else {
					u2TimeBitmapControl =
					((prChnlEntry->time_bitmap.bitDuration
					<< NAN_TIME_BITMAP_CTRL_DURATION_OFFSET)
					& NAN_TIME_BITMAP_CTRL_DURATION) |
					((prChnlEntry->time_bitmap.period
					<< NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET)
					& NAN_TIME_BITMAP_CTRL_PERIOD) |
					((prChnlEntry->time_bitmap.offset
					<<
					NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET)
					& NAN_TIME_BITMAP_CTRL_STARTOFFSET);

					nanParserInterpretTimeBitmapField(
						prAdapter, u2TimeBitmapControl,
						prChnlEntry
						->time_bitmap
						.time_bitmap_length,
						prChnlEntry
						->time_bitmap.time_bitmap,
						au4AvailMap);
				}

#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
				nanSchedConfigCommitedAvailability(
					prAdapter,
					ucMapId,
					rChnlInfo,
					rEntryCtrl,
					au4AvailMap);
#endif
			}
		}
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
		/* Sync setting to firmware */
		nanSchedNegoSyncSchUpdateFsmStep(
			prAdapter, ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE);
#endif
		kalMemFree(prCommittedAvailability, VIR_MEM_TYPE, message_len);

		return 0;
	}
	case NAN_MSG_ID_SET_POTENTIAL_AVAILABILITY:
	{
		struct _NanPotentialAvailability *
			prPotentialAvailability = NULL;
		struct _NanChannelAvailabilityEntry *
			prChnlEntry = NULL;
		union _NAN_BAND_CHNL_CTRL rChnlInfo = {0};
		union _NAN_AVAIL_ENTRY_CTRL rEntryCtrl = {0};
		uint8_t ucNumMaps = 0, ucNumChnlEntries = 0;
		uint8_t ucMapIdx = 0, ucNumBandEntries = 0;
		uint8_t ucChnlEntryIdx = 0, ucBandEntryIdx = 0;
		unsigned char fgNonContinuousBw = FALSE;
		uint16_t u2TimeBitmapControl = 0;
		uint32_t au4AvailMap[NAN_TOTAL_DW] = {0};
		uint8_t ucBandId = 0, ucMapId = 0;
		size_t message_len = 0;
		uint32_t u4DurOf = NAN_TIME_BITMAP_CTRL_DURATION_OFFSET;
		uint32_t u4Dur = NAN_TIME_BITMAP_CTRL_DURATION;
		uint32_t u4PeOf = NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET;
		uint32_t u4CtPe = NAN_TIME_BITMAP_CTRL_PERIOD;
		uint32_t u4StaOf = NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET;
		uint32_t u4Sta = NAN_TIME_BITMAP_CTRL_STARTOFFSET;

		DBGLOG(REQ, INFO,
			"GET NAN_MSG_ID_SET_POTENTIAL_AVAILABILITY\n");

		message_len = sizeof(struct _NanPotentialAvailability);
		prPotentialAvailability =
			kalMemAlloc(message_len, VIR_MEM_TYPE);

		if (!prPotentialAvailability) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		kalMemZero(prPotentialAvailability, message_len);
		kalMemCopy(prPotentialAvailability, (u8 *)data, message_len);

#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
		nanSchedResetPotentialAvailability(prAdapter);
#endif

		ucNumMaps = prPotentialAvailability->num_maps_ids;

		for (ucMapIdx = 0; ucMapIdx < ucNumMaps; ucMapIdx++) {
			ucNumBandEntries =
				prPotentialAvailability
				->potential[ucMapIdx]
				.num_band_entries;
			ucNumChnlEntries =
				prPotentialAvailability
				->potential[ucMapIdx]
				.num_entries;
			ucMapId =
				prPotentialAvailability
				->potential[ucMapIdx]
				.map_id;

			if ((ucMapIdx >= NAN_MAX_MAP_IDS) ||
				(ucMapIdx >= NAN_TIMELINE_MGMT_SIZE)) {
				DBGLOG(NAN, ERROR,
					"Map number (%d, %d) exceed cap\n",
					ucMapIdx, ucNumMaps);
				kalMemFree(prPotentialAvailability,
					VIR_MEM_TYPE,
					message_len);
				return -EINVAL;
			}

			DBGLOG(NAN, INFO,
				"MapId: %u Band number: %d, chnl entry number: %d\n",
				ucMapId, ucNumBandEntries, ucNumChnlEntries);

			if (ucNumBandEntries) {
				for (ucBandEntryIdx = 0;
					ucBandEntryIdx < ucNumBandEntries;
					ucBandEntryIdx++) {
					ucBandId =
					  prPotentialAvailability
					  ->potential[ucMapIdx]
					  .band_ids[ucBandEntryIdx];

					rChnlInfo.u4Type =
					  NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
					rChnlInfo.u4BandIdMask |=
					  BIT(ucBandId);
				}
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
				nanSchedConfigPotentialAvailability(
					prAdapter,
					ucMapId,
					rChnlInfo,
					rEntryCtrl,
					au4AvailMap);
#endif
				if (ucBandEntryIdx > NAN_MAX_BAND_IDS) {
					DBGLOG(NAN, ERROR,
					  "Band number (%d, %d) exceed cap\n",
					  ucBandEntryIdx,
					  ucNumBandEntries);
					kalMemFree(prPotentialAvailability,
					  VIR_MEM_TYPE,
					  message_len);
					return -EINVAL;
				}

			} else if (ucNumChnlEntries) {
				for (ucChnlEntryIdx = 0;
					ucChnlEntryIdx < ucNumChnlEntries;
					ucChnlEntryIdx++) {

					if ((ucChnlEntryIdx >=
					  NAN_MAX_AVAILABILITY_CHANNEL_ENTRIES)
					  || (ucChnlEntryIdx
					  >= NAN_MAX_POTENTIAL_CHNL_LIST)) {
						DBGLOG(NAN, ERROR,
						  "Chnl number (%d, %d) exceed cap\n",
						  ucChnlEntryIdx,
						  ucNumChnlEntries);
						kalMemFree(
						  prPotentialAvailability,
						  VIR_MEM_TYPE,
						  message_len);
						return -EINVAL;
					}

					prChnlEntry =
					  &(prPotentialAvailability
					  ->potential[ucMapIdx]
					  .channel_entries[ucChnlEntryIdx]);
					/* convert to _NAN_BAND_CHNL_CTRL */
					if (prChnlEntry
					->auxiliary_channel_bitmap !=
					0) {
						fgNonContinuousBw = TRUE;
						rChnlInfo
						.u4AuxCenterChnl =
						  nanRegGetChannelByOrder(
						    prChnlEntry->op_class,
						    &prChnlEntry
						    ->auxiliary_channel_bitmap);
					}
					rChnlInfo.u4Type =
					  NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
					rChnlInfo.u4OperatingClass =
						prChnlEntry->op_class;
					rChnlInfo.u4PrimaryChnl =
						nanRegGetPrimaryChannelByOrder(
						  prChnlEntry->op_class,
						  &prChnlEntry
						  ->op_class_bitmap,
						  fgNonContinuousBw,
						  prChnlEntry
						  ->primary_channel_bitmap);

					/* convert to _NAN_AVAIL_ENTRY_CTRL*/
					rEntryCtrl.b3Type =
					  NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN;
					rEntryCtrl.b2Preference =
					  prChnlEntry->usage_preference;
					rEntryCtrl.b3Util =
					  prChnlEntry->utilization;
					rEntryCtrl.b4RxNss =
					  prChnlEntry->rx_nss;
					rEntryCtrl.b1TimeMapAvail =
					  ((prChnlEntry
					  ->time_bitmap
					  .time_bitmap_length ==
						0) ? 0 : 1);

					/* convert to au4AvailMap */
					if (rEntryCtrl
					.b1TimeMapAvail == 1) {
						u2TimeBitmapControl =
						((prChnlEntry
						->time_bitmap
						.bitDuration
						<< u4DurOf)
						& u4Dur) |
						((prChnlEntry
						->time_bitmap
						.period
						<< u4PeOf)
						& u4CtPe) |
						((prChnlEntry
						->time_bitmap
						.offset
						<< u4StaOf)
						& u4Sta);

					  nanParserInterpretTimeBitmapField
					  (prAdapter,
					  u2TimeBitmapControl,
					  prChnlEntry
					  ->time_bitmap
					  .time_bitmap_length,
					  prChnlEntry
					  ->time_bitmap
					  .time_bitmap,
					  au4AvailMap);
					}
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
					nanSchedConfigPotentialAvailability(
						prAdapter, ucMapId, rChnlInfo,
						rEntryCtrl, au4AvailMap);
#endif
				}
			}
		}

		/* Sync setting to firmware */
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
		nanSchedCmdUpdatePotentialChnlAvail(prAdapter);
#endif
		kalMemFree(prPotentialAvailability, VIR_MEM_TYPE,
			message_len);

		return 0;
	}
	case NAN_MSG_ID_SET_DATA_CLUSTER_AVAILABILITY:
	{
		struct _NanDataClusterAvailability *prNdcAvailability = NULL;
		struct _NanDataClusterAvailabilityParams *prNdcParam = NULL;
		uint8_t ucMapId = 0, ucNumMaps = 0;
		uint8_t ucMapIdx = 0;
		uint16_t u2TimeBitmapControl = 0;
		uint32_t au4AvailMap[NAN_TOTAL_DW] = {0};
		size_t message_len = 0;

		DBGLOG(REQ, INFO,
			"GET NAN_MSG_ID_SET_DATA_CLUSTER_AVAILABILITY\n");

		message_len = sizeof(struct _NanDataClusterAvailability);
		prNdcAvailability = kalMemAlloc(message_len, VIR_MEM_TYPE);

		if (!prNdcAvailability) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		kalMemZero(prNdcAvailability, message_len);
		kalMemCopy(prNdcAvailability, (u8 *)data, message_len);

		ucNumMaps = prNdcAvailability->num_maps_ids;

		for (ucMapIdx = 0; ucMapIdx < ucNumMaps; ucMapIdx++) {
			prNdcParam = &(prNdcAvailability->ndc[ucMapIdx]);
			ucMapId = prNdcParam->map_id;

			DBGLOG(NAN, INFO,
				"MapNum:%d, MapId:%d, sel:%d\n",
				ucNumMaps, ucMapId, prNdcParam->selected);
			if ((ucMapIdx >= NAN_MAX_MAP_IDS) ||
				(ucMapIdx >= NAN_TIMELINE_MGMT_SIZE)) {
				DBGLOG(NAN, ERROR,
					"Map number (%d, %d) exceed cap\n",
					ucMapIdx, ucNumMaps);
				kalMemFree(prNdcAvailability, VIR_MEM_TYPE,
					message_len);
				return -EINVAL;
			}

			if (prNdcParam->selected) {
				/* convert to au4AvailMap */
				u2TimeBitmapControl =
				((prNdcParam->time_bitmap.bitDuration
				<< NAN_TIME_BITMAP_CTRL_DURATION_OFFSET)
				& NAN_TIME_BITMAP_CTRL_DURATION) |
				((prNdcParam->time_bitmap.period
				<< NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET)
				& NAN_TIME_BITMAP_CTRL_PERIOD) |
				((prNdcParam->time_bitmap.offset
				<< NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET)
				& NAN_TIME_BITMAP_CTRL_STARTOFFSET);

				kalMemZero(au4AvailMap, sizeof(au4AvailMap));
				nanParserInterpretTimeBitmapField(
					prAdapter, u2TimeBitmapControl,
					prNdcParam->time_bitmap
					.time_bitmap_length,
					prNdcParam->time_bitmap
					.time_bitmap,
					au4AvailMap);
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
				nanSchedConfigNdcAvailability(
					prAdapter, ucMapId, au4AvailMap,
					prNdcParam->ndc_id.octet);
#endif
			}
			DBGLOG(NAN, ERROR,
				"sel: %d, MapId:%d, BitmapCtrl:0x%x\n",
				prNdcParam->selected,
				ucMapId,
				u2TimeBitmapControl);
			DBGLOG(NAN, INFO, "NDC ID\n");
			dumpMemory8((uint8_t *)prNdcParam->ndc_id.octet,
						MAC_ADDR_LEN);
		}

		kalMemFree(prNdcAvailability, VIR_MEM_TYPE,
			message_len);

		return 0;
	}

	case NAN_MSG_ID_FORCED_BEACON_TRANSMISSION:
	{
		struct _NanForcedDiscBeaconTransmission
					*prNanDiscBcnTrans = NULL;
		struct _NanForcedDiscBeaconTxAvailability *prAvail = NULL;
		struct _NanForcedDiscBeaconTxAvailabilityParams
					*prParams = NULL;
		struct _NAN_CMD_EVENT_SET_DISC_BCN_T rNanSetDiscBcn = {};
		uint32_t rStatus = 0;
		uint16_t u2TimeBitmapControl = 0;
		size_t message_len = 0, i = 0;
		uint32_t au4AvailMap[NAN_TOTAL_DW] = {0};

		DBGLOG(REQ, INFO,
			"GET NAN_MSG_ID_SET_DATA_CLUSTER_AVAILABILITY\n");

		message_len = sizeof(struct _NanForcedDiscBeaconTransmission);
		prNanDiscBcnTrans = kalMemAlloc(message_len, VIR_MEM_TYPE);

		if (!prNanDiscBcnTrans) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		kalMemZero(prNanDiscBcnTrans, message_len);
		kalMemCopy(prNanDiscBcnTrans, (u8 *)data, message_len);

		prAvail = &prNanDiscBcnTrans->availability;

		if (!prNanDiscBcnTrans->enable)
			return 0;

		DBGLOG(NAN, INFO,
			"[FastDisc] Enable: %d, num_maps_ids: %d, BcnItv: %d\n",
			prNanDiscBcnTrans->enable,
			prAvail->num_maps_ids,
			prNanDiscBcnTrans->beacon_interval);

		if ((prAvail->num_maps_ids == 0 &&
		     prNanDiscBcnTrans->beacon_interval == 0) ||
		    (prAvail->num_maps_ids != 0 &&
		     prNanDiscBcnTrans->beacon_interval != 0)) {
			DBGLOG(NAN, ERROR, "[FastDisc] Wrong Input\n");
			return -EINVAL;
		}

		kalMemZero(&rNanSetDiscBcn,
			   sizeof(struct _NAN_CMD_EVENT_SET_DISC_BCN_T));
		if (prNanDiscBcnTrans->beacon_interval != 0) {
			DBGLOG(NAN, INFO, "[FastDisc] Periodic based\n");

			rNanSetDiscBcn.ucDiscBcnType = ENUM_DISC_BCN_PERIOD;
			rNanSetDiscBcn.ucDiscBcnPeriod =
					prNanDiscBcnTrans->beacon_interval;
		} else if (prAvail->num_maps_ids != 0) {
			DBGLOG(NAN, INFO, "[FastDisc] Slot based\n");

			rNanSetDiscBcn.ucDiscBcnType = ENUM_DISC_BCN_SLOT;
			rNanSetDiscBcn.ucDiscBcnPeriod = 0;

			for (i = 0; i < NAN_TIMELINE_MGMT_SIZE; i++) {
				prParams = &prAvail->slots[i];

				if (i >= prAvail->num_maps_ids) {
					DBGLOG(NAN, WARN,
						"Skip Tid, %d, NumMap, %d\n",
						i, prAvail->num_maps_ids);
					rNanSetDiscBcn.rDiscBcnTimeline[i]
						.ucMapId = NAN_INVALID_MAP_ID;
					break;
				}

				rNanSetDiscBcn.rDiscBcnTimeline[i].ucMapId =
							prParams->map_id;
				/* convert to au4AvailMap */
				u2TimeBitmapControl =
				    ((prParams->time_bitmap.bitDuration
				     << NAN_TIME_BITMAP_CTRL_DURATION_OFFSET)
				     & NAN_TIME_BITMAP_CTRL_DURATION) |
				    ((prParams->time_bitmap.period
				     << NAN_TIME_BITMAP_CTRL_PERIOD_OFFSET)
				     & NAN_TIME_BITMAP_CTRL_PERIOD) |
				    ((prParams->time_bitmap.offset
				     << NAN_TIME_BITMAP_CTRL_STARTOFFSET_OFFSET)
				     & NAN_TIME_BITMAP_CTRL_STARTOFFSET);

				kalMemZero(au4AvailMap, sizeof(au4AvailMap));
				nanParserInterpretTimeBitmapField(
				       prAdapter, u2TimeBitmapControl,
				       prParams->time_bitmap.time_bitmap_length,
				       prParams->time_bitmap.time_bitmap,
				       au4AvailMap);

				kalMemCopy(rNanSetDiscBcn.rDiscBcnTimeline[i]
						.au4AvailMap,
						au4AvailMap,
						sizeof(au4AvailMap));
			}
		}
		rStatus = nanDevSetDiscBcn(prAdapter, &rNanSetDiscBcn);

		if (rStatus != NAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR,
				"[FastDisc] Set Disc Bcn Period Error !!\n");
			return -EFAULT;
		}

		return 0;
	}
	case NAN_MSG_ID_UPDATE_DFSP_CONFIG:
		{
			struct _NanDfspConfig *prNanDfspCfg = NULL;

			prNanDfspCfg = (struct _NanDfspConfig *)data;
			nanUpdateDfspConfig(prAdapter,
				(struct _NAN_CMD_DFSP_CONFIG *)prNanDfspCfg);
			return 0;
		}
	case NAN_MSG_ID_UPDATE_CUSTOM_ATTRIBUTE:
	{
		struct NanCustomAttribute *prNanCustomAttr = NULL;

		prNanCustomAttr = (struct NanCustomAttribute *)data;

		prAdapter->rNanCustomAttr.length =
			prNanCustomAttr->length;
#ifdef NAN_TODO /* T.B.D Unify NAN-Display */
		kalMemCpyS(prAdapter->rNanCustomAttr.data,
			sizeof(prAdapter->rNanCustomAttr.data),
			prNanCustomAttr->data,
			prNanCustomAttr->length);
#endif

		/* Update custom vendor specific attribute to FW */
		nanDiscSetCustomAttribute(prAdapter, prNanCustomAttr);

		return 0;
	}
	case NAN_MSG_ID_PRIV_CMD:
	{
		struct NanDrvPrivCmd *prNanPrivCmd = NULL;
		struct NanDrvPrivCmdWork *prWork = NULL;

		prNanPrivCmd = (struct NanDrvPrivCmd *)data;

		prWork = kalMemAlloc(sizeof(struct NanDrvPrivCmdWork),
				PHY_MEM_TYPE);
		if (!prWork) {
			DBGLOG(REQ, ERROR, "Cannot allocate prWork\n");
			return -ENOMEM;
		}

		kalMemZero(prWork, sizeof(struct NanDrvPrivCmdWork));
		kalMemCopy(prWork->cmd,
			prNanPrivCmd->cmd,
			NAN_PRIV_CMD_MAX_SIZE - 1);
		prWork->prGlueInfo = prGlueInfo;

		INIT_WORK(&prWork->work, kalNanPrivWork);

		if (!prGlueInfo->prNANPrivCmdWorkQueue) {
			DBGLOG(REQ, ERROR, "prNANPrivCmdWorkQueue is NULL\n");
			kalMemFree(prWork, PHY_MEM_TYPE,
				sizeof(struct NanDrvPrivCmdWork));
			return -EFAULT;
		}

		queue_work(prGlueInfo->prNANPrivCmdWorkQueue, &prWork->work);

		return 0;
	}

	default:
		return -EOPNOTSUPP;
	}

	return ret;
}

/* Indication part */
int
mtk_cfg80211_vendor_event_nan_event_indication(struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NanEventIndMsg *prNanEventInd;
	struct NAN_DE_EVENT *prDeEvt;
	uint16_t u2EventType;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;

	prDeEvt = (struct NAN_DE_EVENT *) pcuEvtBuf;

	if (prDeEvt == NULL) {
		DBGLOG(NAN, ERROR, "pcuEvtBuf is null\n");
		return -EFAULT;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	/*Final length includes all TLVs*/
	message_len = sizeof(struct _NanMsgHeader) +
		SIZEOF_TLV_HDR + MAC_ADDR_LEN;

	prNanEventInd = kalMemAlloc(message_len, VIR_MEM_TYPE);
	if (!prNanEventInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}

	prNanEventInd->fwHeader.msgVersion = 1;
	prNanEventInd->fwHeader.msgId = NAN_MSG_ID_DE_EVENT_IND;
	prNanEventInd->fwHeader.msgLen = message_len;
	prNanEventInd->fwHeader.handle = 0;
	prNanEventInd->fwHeader.transactionId = 0;

	tlvs = prNanEventInd->ptlv;


	if (prDeEvt->ucEventType != NAN_EVENT_ID_DISC_MAC_ADDR) {
		DBGLOG(NAN, DEBUG, "ClusterId=%02x%02x%02x%02x%02x%02x\n",
		       prDeEvt->ucClusterId[0], prDeEvt->ucClusterId[1],
		       prDeEvt->ucClusterId[2], prDeEvt->ucClusterId[3],
		       prDeEvt->ucClusterId[4], prDeEvt->ucClusterId[5]);
		/* NAN_CHK_PNT log message */
		if (prDeEvt->ucEventType == NAN_EVENT_ID_STARTED_CLUSTER) {
			nanLogClusterMac(prDeEvt->ucOwnNmi);
			nanLogClusterId(prDeEvt->ucClusterId);
		} else if (prDeEvt->ucEventType ==
			   NAN_EVENT_ID_JOINED_CLUSTER)
			nanLogJoinCluster(prDeEvt->ucClusterId);
		DBGLOG(NAN, DEBUG,
		       "AnchorMasterRank=%02x%02x%02x%02x%02x%02x%02x%02x\n",
		       prDeEvt->aucAnchorMasterRank[0],
		       prDeEvt->aucAnchorMasterRank[1],
		       prDeEvt->aucAnchorMasterRank[2],
		       prDeEvt->aucAnchorMasterRank[3],
		       prDeEvt->aucAnchorMasterRank[4],
		       prDeEvt->aucAnchorMasterRank[5],
		       prDeEvt->aucAnchorMasterRank[6],
		       prDeEvt->aucAnchorMasterRank[7]);
		DBGLOG(NAN, DEBUG, "MyNMI=%02x%02x%02x%02x%02x%02x\n",
		       prDeEvt->ucOwnNmi[0], prDeEvt->ucOwnNmi[1],
		       prDeEvt->ucOwnNmi[2], prDeEvt->ucOwnNmi[3],
		       prDeEvt->ucOwnNmi[4], prDeEvt->ucOwnNmi[5]);
		DBGLOG(NAN, DEBUG, "MasterNMI=%02x%02x%02x%02x%02x%02x\n",
		       prDeEvt->ucMasterNmi[0], prDeEvt->ucMasterNmi[1],
		       prDeEvt->ucMasterNmi[2], prDeEvt->ucMasterNmi[3],
		       prDeEvt->ucMasterNmi[4], prDeEvt->ucMasterNmi[5]);
	}

	if (prDeEvt->ucEventType == NAN_EVENT_ID_DISC_MAC_ADDR)
		u2EventType = NAN_TLV_TYPE_EVENT_SELF_STATION_MAC_ADDRESS;
	else if (prDeEvt->ucEventType == NAN_EVENT_ID_STARTED_CLUSTER)
		u2EventType = NAN_TLV_TYPE_EVENT_STARTED_CLUSTER;
	else if (prDeEvt->ucEventType == NAN_EVENT_ID_JOINED_CLUSTER)
		u2EventType = NAN_TLV_TYPE_EVENT_JOINED_CLUSTER;
	else {
		kalMemFree(prNanEventInd, VIR_MEM_TYPE, message_len);
		return WLAN_STATUS_SUCCESS;
	}

	nanExtComposeClusterEvent(prAdapter, prDeEvt);

	/* Add TLV datas */
	tlvs = nanAddTlv(u2EventType, MAC_ADDR_LEN, prDeEvt->ucClusterId, tlvs);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kalMemFree(prNanEventInd, VIR_MEM_TYPE, message_len);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanEventInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kalMemFree(prNanEventInd, VIR_MEM_TYPE, message_len);
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kalMemFree(prNanEventInd, VIR_MEM_TYPE, message_len);

	return WLAN_STATUS_SUCCESS;
}

int mtk_cfg80211_vendor_event_nan_disable_indication(
		struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NanDisableIndMsg *prNanDisableInd;
	struct NAN_DISABLE_EVENT *prDisableEvt;
	size_t message_len = 0;

	prDisableEvt = (struct NAN_DISABLE_EVENT *) pcuEvtBuf;

	if (prDisableEvt == NULL) {
		DBGLOG(NAN, ERROR, "pcuEvtBuf is null\n");
		return -EFAULT;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	/*Final length includes all TLVs*/
	message_len = sizeof(struct _NanMsgHeader) +
			sizeof(u16) +
			sizeof(u16);

	prNanDisableInd = kalMemAlloc(message_len, VIR_MEM_TYPE);
	if (!prNanDisableInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}
	prNanDisableInd->fwHeader.msgVersion = 1;
	prNanDisableInd->fwHeader.msgId = NAN_MSG_ID_DISABLE_IND;
	prNanDisableInd->fwHeader.msgLen = message_len;
	prNanDisableInd->fwHeader.handle = 0;
	prNanDisableInd->fwHeader.transactionId = 0;

	prNanDisableInd->reason = 0;

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kalMemFree(prNanDisableInd, VIR_MEM_TYPE, message_len);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
		message_len, prNanDisableInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kalMemFree(prNanDisableInd, VIR_MEM_TYPE, message_len);
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kalMemFree(prNanDisableInd, VIR_MEM_TYPE, message_len);

	g_enableNAN = TRUE;

	return WLAN_STATUS_SUCCESS;
}

/* Indication part */
int
mtk_cfg80211_vendor_event_nan_replied_indication(struct ADAPTER *prAdapter,
						 uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NAN_REPLIED_EVENT *prRepliedEvt = NULL;
	struct NanPublishRepliedIndMsg *prNanPubRepliedInd;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	prRepliedEvt = (struct NAN_REPLIED_EVENT *)pcuEvtBuf;

	/* Final length includes all TLVs */
	message_len = sizeof(struct _NanMsgHeader) +
		      sizeof(struct _NanPublishRepliedIndParams) +
		      ((SIZEOF_TLV_HDR) + MAC_ADDR_LEN) +
		      ((SIZEOF_TLV_HDR) + sizeof(prRepliedEvt->ucRssi_value));

	prNanPubRepliedInd = kmalloc(message_len, GFP_KERNEL);
	if (prNanPubRepliedInd == NULL)
		return -ENOMEM;

	kalMemZero(prNanPubRepliedInd, message_len);

	DBGLOG(NAN, DEBUG, "[%s] message_len : %lu\n", __func__, message_len);
	prNanPubRepliedInd->fwHeader.msgVersion = 1;
	prNanPubRepliedInd->fwHeader.msgId = NAN_MSG_ID_PUBLISH_REPLIED_IND;
	prNanPubRepliedInd->fwHeader.msgLen = message_len;
	prNanPubRepliedInd->fwHeader.handle = prRepliedEvt->u2Pubid;
	prNanPubRepliedInd->fwHeader.transactionId = 0;

	prNanPubRepliedInd->publishRepliedIndParams.matchHandle =
		prRepliedEvt->u2Subid;

	tlvs = prNanPubRepliedInd->ptlv;
	/* Add TLV datas */
	tlvs = nanAddTlv(NAN_TLV_TYPE_MAC_ADDRESS, MAC_ADDR_LEN,
			 &prRepliedEvt->auAddr[0], tlvs);

	tlvs = nanAddTlv(NAN_TLV_TYPE_RECEIVED_RSSI_VALUE,
			 sizeof(prRepliedEvt->ucRssi_value),
			 &prRepliedEvt->ucRssi_value, tlvs);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kfree(prNanPubRepliedInd);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanPubRepliedInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree(prNanPubRepliedInd);
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	kfree(prNanPubRepliedInd);
	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_match_indication(struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NAN_DISCOVERY_EVENT *prDiscEvt;
	struct NanMatchIndMsg *prNanMatchInd;
	struct NanSdeaCtrlParams peer_sdea_params;
	struct NanFWSdeaCtrlParams nanPeerSdeaCtrlarms;
	size_t message_len = 0;
	uint8_t *tlvs = NULL;
#if CFG_SUPPORT_RTT
	uint16_t u2RangingId = 0;
	struct _NAN_RANGING_INSTANCE_T *prRanging = NULL;
	struct NanRangeRequest *prRangingReq = NULL;
	struct NanRangeInfo rNanRangeInfo;
	struct RTT_RESULT *prRttResult = NULL;
#endif
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	kalMemZero(&nanPeerSdeaCtrlarms, sizeof(struct NanFWSdeaCtrlParams));
	kalMemZero(&peer_sdea_params, sizeof(struct NanSdeaCtrlParams));

	prDiscEvt = (struct NAN_DISCOVERY_EVENT *)pcuEvtBuf;

	message_len = sizeof(struct _NanMsgHeader) +
		      sizeof(struct _NanMatchIndParams) +
		      (SIZEOF_TLV_HDR + MAC_ADDR_LEN) +
		      (SIZEOF_TLV_HDR + prDiscEvt->u2Service_info_len) +
		      (SIZEOF_TLV_HDR + prDiscEvt->ucSdf_match_filter_len) +
		      (SIZEOF_TLV_HDR + sizeof(struct NanFWSdeaCtrlParams));
#if CFG_SUPPORT_RTT
	prRanging = nanRangingInstanceSearchByMac(prAdapter,
						prDiscEvt->aucNanAddress);
	if (prRanging) {
		DBGLOG(NAN, INFO, "Ranging of DiscSubId=%u, Addr="MACSTR"\n",
			prDiscEvt->u2SubscribeID,
			MAC2STR(prDiscEvt->aucNanAddress));
		message_len += (SIZEOF_TLV_HDR + sizeof(struct NanRangeInfo));
	}
#endif

	prNanMatchInd = kmalloc(message_len, GFP_KERNEL);
	if (!prNanMatchInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}

	kalMemZero(prNanMatchInd, message_len);

	prNanMatchInd->fwHeader.msgVersion = 1;
	prNanMatchInd->fwHeader.msgId = NAN_MSG_ID_MATCH_IND;
	prNanMatchInd->fwHeader.msgLen = message_len;
	prNanMatchInd->fwHeader.handle = prDiscEvt->u2SubscribeID;
	prNanMatchInd->fwHeader.transactionId = 0;

	prNanMatchInd->matchIndParams.matchHandle = prDiscEvt->u2PublishID;
	prNanMatchInd->matchIndParams.matchOccuredFlag =
		0; /* means match in SDF */
	prNanMatchInd->matchIndParams.outOfResourceFlag =
		0; /* doesn't outof resource. */

	tlvs = prNanMatchInd->ptlv;
	/* Add TLV datas */
	tlvs = nanAddTlv(NAN_TLV_TYPE_MAC_ADDRESS, MAC_ADDR_LEN,
			 &prDiscEvt->aucNanAddress[0], tlvs);
	DBGLOG(NAN, DEBUG, "[%s] :NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO %u\n",
	       __func__, NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO);

	tlvs = nanAddTlv(NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
			 prDiscEvt->u2Service_info_len,
			 &prDiscEvt->aucSerive_specificy_info[0], tlvs);

	tlvs = nanAddTlv(NAN_TLV_TYPE_SDF_MATCH_FILTER,
			 prDiscEvt->ucSdf_match_filter_len,
			 prDiscEvt->aucSdf_match_filter,
			 tlvs);

	nanPeerSdeaCtrlarms.data_path_required =
		(prDiscEvt->ucDataPathParm != 0) ? 1 : 0;
	nanPeerSdeaCtrlarms.security_required =
		(prDiscEvt->aucSecurityInfo[0] != 0) ? 1 : 0;
	nanPeerSdeaCtrlarms.ranging_required =
		(prDiscEvt->ucRange_measurement != 0) ? 1 : 0;

	DBGLOG(NAN, LOUD,
	       "[%s] data_path_required : %d, security_required:%d, ranging_required:%d\n",
	       __func__, nanPeerSdeaCtrlarms.data_path_required,
	       nanPeerSdeaCtrlarms.security_required,
	       nanPeerSdeaCtrlarms.ranging_required);

	/* NAN_CHK_PNT log message */
	nanLogMatch(prDiscEvt->aucNanAddress);

	tlvs = nanAddTlv(NAN_TLV_TYPE_SDEA_CTRL_PARAMS,
			 sizeof(struct NanFWSdeaCtrlParams),
			 (u8 *)&nanPeerSdeaCtrlarms, tlvs);
#if CFG_SUPPORT_RTT
	prRangingReq = nanGetRangingReq(prAdapter, prDiscEvt->u2SubscribeID);
	if (!prRangingReq) {
		DBGLOG(NAN, ERROR,
			"RangingReq Null, id: %d\n",
			prDiscEvt->u2SubscribeID);
		goto SKIP_RTT;
	}

	if (prRanging) {
		DBGLOG(NAN, INFO, "write range_info to tlv\n");
		prRttResult = &prRanging->ranging_ctrl.rRangingRttResult;
		rNanRangeInfo.range_measurement_cm =
			prRttResult->i4DistanceMM / 1000;
		rNanRangeInfo.ranging_event_type =
			prRangingReq->ranging_cfg.config_ranging_indications;
		tlvs = nanAddTlv(NAN_TLV_TYPE_NAN20_RANGING_RESULT,
				sizeof(struct NanRangeInfo),
				(u8 *)&rNanRangeInfo, tlvs);
	} else if (nanIsSubEnableRanging(prAdapter, prDiscEvt->u2SubscribeID)) {
		DBGLOG(NAN, INFO, "store DiscEvt and trigger ranging\n");
		nanSubStoreDiscEvtForRanging(prAdapter, prDiscEvt);
		kalMemCopy(prRangingReq->peer_addr,
				prDiscEvt->aucNanAddress,
				NAN_MAC_ADDR_LEN);
		nanRangingRequest(prAdapter, &u2RangingId, prRangingReq);
		return	WLAN_STATUS_PENDING;
	}

SKIP_RTT:

#endif /* CFG_SUPPORT_RTT */

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kfree(prNanMatchInd);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanMatchInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		kfree(prNanMatchInd);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kfree(prNanMatchInd);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_publish_terminate(struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NAN_PUBLISH_TERMINATE_EVENT *prPubTerEvt;
	struct NanPublishTerminatedIndMsg nanPubTerInd;
	struct _NAN_PUBLISH_SPECIFIC_INFO_T *prPubSpecificInfo = NULL;
	size_t message_len = 0;
	uint8_t i;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	kalMemZero(&nanPubTerInd, sizeof(struct NanPublishTerminatedIndMsg));
	prPubTerEvt = (struct NAN_PUBLISH_TERMINATE_EVENT *)pcuEvtBuf;

	message_len = sizeof(struct NanPublishTerminatedIndMsg);

	nanPubTerInd.fwHeader.msgVersion = 1;
	nanPubTerInd.fwHeader.msgId = NAN_MSG_ID_PUBLISH_TERMINATED_IND;
	nanPubTerInd.fwHeader.msgLen = message_len;
	nanPubTerInd.fwHeader.handle = prPubTerEvt->u2Pubid;
	/* Indication doesn't have transactionId, don't care */
	nanPubTerInd.fwHeader.transactionId = 0;
	/* For all user should be success. */
	nanPubTerInd.reason = prPubTerEvt->ucReasonCode;
	prAdapter->rPublishInfo.ucNanPubNum--;

	DBGLOG(NAN, DEBUG, "Cancel Pub ID = %d, PubNum = %d\n",
	       nanPubTerInd.fwHeader.handle,
	       prAdapter->rPublishInfo.ucNanPubNum);

	for (i = 0; i < NAN_MAX_PUBLISH_NUM; i++) {
		prPubSpecificInfo =
			&prAdapter->rPublishInfo.rPubSpecificInfo[i];
		if (prPubSpecificInfo->ucPublishId == prPubTerEvt->u2Pubid) {
			prPubSpecificInfo->ucUsed = FALSE;
			if (prPubSpecificInfo->ucReportTerminate) {
				/* Fill skb and send to kernel by nl80211 */
				skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN,
					GFP_KERNEL);
				if (!skb) {
					DBGLOG(NAN, ERROR,
						"Allocate skb failed\n");
					return -ENOMEM;
				}
				if (unlikely(nla_put(skb,
					MTK_WLAN_VENDOR_ATTR_NAN,
					message_len,
					&nanPubTerInd) < 0)) {
					DBGLOG(NAN, ERROR,
						"nla_put_nohdr failed\n");
					kfree_skb(skb);
					return -EFAULT;
				}
				cfg80211_vendor_event(skb, GFP_KERNEL);
			}
		}
	}
	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_subscribe_terminate(struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NAN_SUBSCRIBE_TERMINATE_EVENT *prSubTerEvt;
	struct NanSubscribeTerminatedIndMsg nanSubTerInd;
	struct _NAN_SUBSCRIBE_SPECIFIC_INFO_T *prSubSpecificInfo = NULL;
	size_t message_len = 0;
	uint8_t i;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	kalMemZero(&nanSubTerInd, sizeof(struct NanSubscribeTerminatedIndMsg));
	prSubTerEvt = (struct NAN_SUBSCRIBE_TERMINATE_EVENT *)pcuEvtBuf;

	message_len = sizeof(struct NanSubscribeTerminatedIndMsg);

	nanSubTerInd.fwHeader.msgVersion = 1;
	nanSubTerInd.fwHeader.msgId = NAN_MSG_ID_SUBSCRIBE_TERMINATED_IND;
	nanSubTerInd.fwHeader.msgLen = message_len;
	nanSubTerInd.fwHeader.handle = prSubTerEvt->u2Subid;
	/* Indication doesn't have transactionId, don't care */
	nanSubTerInd.fwHeader.transactionId = 0;
	/* For all user should be success. */
	nanSubTerInd.reason = prSubTerEvt->ucReasonCode;
	prAdapter->rSubscribeInfo.ucNanSubNum--;

	DBGLOG(NAN, DEBUG, "Cancel Sub ID = %d, SubNum = %d\n",
		nanSubTerInd.fwHeader.handle,
		prAdapter->rSubscribeInfo.ucNanSubNum);

	for (i = 0; i < NAN_MAX_SUBSCRIBE_NUM; i++) {
		prSubSpecificInfo =
			&prAdapter->rSubscribeInfo.rSubSpecificInfo[i];
		if (prSubSpecificInfo->ucSubscribeId == prSubTerEvt->u2Subid) {
			prSubSpecificInfo->ucUsed = FALSE;
			if (prSubSpecificInfo->ucReportTerminate) {
				/*  Fill skb and send to kernel by nl80211*/
				skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN,
					GFP_KERNEL);
				if (!skb) {
					DBGLOG(NAN, ERROR,
						"Allocate skb failed\n");
					return -ENOMEM;
				}
				if (unlikely(nla_put(skb,
					MTK_WLAN_VENDOR_ATTR_NAN,
					message_len,
					&nanSubTerInd) < 0)) {
					DBGLOG(NAN, ERROR,
						"nla_put_nohdr failed\n");
					kfree_skb(skb);
					return -EFAULT;
				}
				cfg80211_vendor_event(skb, GFP_KERNEL);
			}
		}
	}
	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_followup_indication(struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NanFollowupIndMsg *prNanFollowupInd;
	struct NAN_FOLLOW_UP_EVENT *prFollowupEvt;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	prFollowupEvt = (struct NAN_FOLLOW_UP_EVENT *)pcuEvtBuf;

	message_len =
		sizeof(struct _NanMsgHeader) +
		sizeof(struct _NanFollowupIndParams) +
		(SIZEOF_TLV_HDR + MAC_ADDR_LEN) +
		(SIZEOF_TLV_HDR + prFollowupEvt->service_specific_info_len) +
		(SIZEOF_TLV_HDR +
		prFollowupEvt->sdea_service_specific_info_len);

	prNanFollowupInd = kmalloc(message_len, GFP_KERNEL);
	if (!prNanFollowupInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}
	kalMemZero(prNanFollowupInd, message_len);
	if (!prNanFollowupInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}

	prNanFollowupInd->fwHeader.msgVersion = 1;
	prNanFollowupInd->fwHeader.msgId = NAN_MSG_ID_FOLLOWUP_IND;
	prNanFollowupInd->fwHeader.msgLen = message_len;
	prNanFollowupInd->fwHeader.handle = prFollowupEvt->publish_subscribe_id;

	/* Indication doesn't have transition ID */
	prNanFollowupInd->fwHeader.transactionId = 0;

	/* Mapping datas */
	prNanFollowupInd->followupIndParams.matchHandle =
		prFollowupEvt->requestor_instance_id;
	prNanFollowupInd->followupIndParams.window = prFollowupEvt->dw_or_faw;

	DBGLOG(NAN, INFO,
	       "[%s] matchHandle: %d, window:%d, ServiceLen(%d,%d)\n",
	       __func__,
	       prNanFollowupInd->followupIndParams.matchHandle,
	       prNanFollowupInd->followupIndParams.window,
	       prFollowupEvt->service_specific_info_len,
	       prFollowupEvt->sdea_service_specific_info_len);

	tlvs = prNanFollowupInd->ptlv;
	/* Add TLV datas */
	tlvs = nanAddTlv(NAN_TLV_TYPE_MAC_ADDRESS, MAC_ADDR_LEN,
			 prFollowupEvt->addr, tlvs);

	if (prFollowupEvt->service_specific_info_len > 0)
		tlvs = nanAddTlv(NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
			 prFollowupEvt->service_specific_info_len,
			 prFollowupEvt->service_specific_info, tlvs);
	if (prFollowupEvt->sdea_service_specific_info_len > 0)
		tlvs = nanAddTlv(NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO,
			 prFollowupEvt->sdea_service_specific_info_len,
			 prFollowupEvt->sdea_service_specific_info, tlvs);

	DBGLOG(NAN, INFO,
		"pub/subid: %d, addr: %02x:%02x:%02x:%02x:%02x:%02x, specific_info[0]: %02x\n",
		prNanFollowupInd->fwHeader.handle,
		((uint8_t *)prFollowupEvt->addr)[0],
		((uint8_t *)prFollowupEvt->addr)[1],
		((uint8_t *)prFollowupEvt->addr)[2],
		((uint8_t *)prFollowupEvt->addr)[3],
		((uint8_t *)prFollowupEvt->addr)[4],
		((uint8_t *)prFollowupEvt->addr)[5],
		prFollowupEvt->service_specific_info[0]);

	/* NAN_CHK_PNT log message */
		nanLogRx("Follow_Up", prFollowupEvt->addr);

	/* Ranging report
	 * To be implement. NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO
	 */

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kfree(prNanFollowupInd);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanFollowupInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		kfree(prNanFollowupInd);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kfree(prNanFollowupInd);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_selfflwup_indication(
	struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NanSelfFollowupIndMsg *prNanSelfFollowupInd;
	struct NAN_FOLLOW_UP_EVENT *prFollowupEvt;
	size_t message_len = 0;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	prFollowupEvt = (struct NAN_FOLLOW_UP_EVENT *) pcuEvtBuf;

	message_len = sizeof(*prNanSelfFollowupInd);

	prNanSelfFollowupInd = kmalloc(message_len, GFP_KERNEL);
	if (!prNanSelfFollowupInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}

	kalMemZero(prNanSelfFollowupInd, message_len);

	prNanSelfFollowupInd->fwHeader.msgVersion = 1;
	prNanSelfFollowupInd->fwHeader.msgId =
			NAN_MSG_ID_SELF_TRANSMIT_FOLLOWUP_IND;
	prNanSelfFollowupInd->fwHeader.msgLen = message_len;
	prNanSelfFollowupInd->fwHeader.handle =
		prFollowupEvt->publish_subscribe_id;
	/* Indication doesn't have transition ID */
	prNanSelfFollowupInd->fwHeader.transactionId =
		prFollowupEvt->transaction_id;

	if (prFollowupEvt->tx_status == WLAN_STATUS_SUCCESS)
		prNanSelfFollowupInd->reason = NAN_I_STATUS_SUCCESS;
	else
		prNanSelfFollowupInd->reason = NAN_I_STATUS_DE_FAILURE;

	nanLogTxAndTxDoneFollowup("Follow_Up", prFollowupEvt);

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kfree(prNanSelfFollowupInd);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
		message_len, prNanSelfFollowupInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		kfree(prNanSelfFollowupInd);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kfree(prNanSelfFollowupInd);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_match_expire(struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;
	struct NAN_MATCH_EXPIRE_EVENT *prMatchExpireEvt;
	struct NanMatchExpiredIndMsg *prNanMatchExpiredInd;
	size_t message_len = 0;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev is NULL\n");
		return -ENODEV;
	}

	prMatchExpireEvt = (struct NAN_MATCH_EXPIRE_EVENT *)pcuEvtBuf;

	message_len = sizeof(struct NanMatchExpiredIndMsg);

	prNanMatchExpiredInd = kmalloc(message_len, GFP_KERNEL);
	if (!prNanMatchExpiredInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return -ENOMEM;
	}

	kalMemZero(prNanMatchExpiredInd, message_len);

	prNanMatchExpiredInd->fwHeader.msgVersion = 1;
	prNanMatchExpiredInd->fwHeader.msgId = NAN_MSG_ID_MATCH_EXPIRED_IND;
	prNanMatchExpiredInd->fwHeader.msgLen = message_len;
	prNanMatchExpiredInd->fwHeader.handle =
			prMatchExpireEvt->u2PublishSubscribeID;
	prNanMatchExpiredInd->fwHeader.transactionId = 0;

	prNanMatchExpiredInd->matchExpiredIndParams.matchHandle =
		prMatchExpireEvt->u4RequestorInstanceID;

	DBGLOG(NAN, DEBUG, "[%s] Handle:%d, matchHandle:%d\n", __func__,
		prNanMatchExpiredInd->fwHeader.handle,
		prNanMatchExpiredInd->matchExpiredIndParams.matchHandle);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		message_len + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN,
		GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		kfree(prNanMatchExpiredInd);
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb,
		MTK_WLAN_VENDOR_ATTR_NAN,
		message_len,
		prNanMatchExpiredInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree(prNanMatchExpiredInd);
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	kfree(prNanMatchExpiredInd);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_report_beacon(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf)
{
	struct _NAN_EVENT_REPORT_BEACON *prFwEvt;
	struct WLAN_BEACON_FRAME *prWlanBeaconFrame
		= (struct WLAN_BEACON_FRAME *) NULL;

	prFwEvt = (struct _NAN_EVENT_REPORT_BEACON *) pcuEvtBuf;
	prWlanBeaconFrame = (struct WLAN_BEACON_FRAME *)
		prFwEvt->aucBeaconFrame;

	DBGLOG(NAN, DEBUG,
		"Cl:" MACSTR ",Src:" MACSTR ",rssi:%d,chnl:%d,TsfL:0x%x\n",
		MAC2STR(prWlanBeaconFrame->aucBSSID),
		MAC2STR(prWlanBeaconFrame->aucSrcAddr),
		prFwEvt->i4Rssi,
		prFwEvt->ucHwChnl,
		prFwEvt->au4LocalTsf[0]);

	nanExtComposeBeaconTrack(prAdapter, prFwEvt);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_schedule_config(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf)
{
	g_deEvent++;

	nanUpdateAisBitmap(prAdapter, TRUE);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_event_nan_lowpower_ctrl(
	struct ADAPTER *prAdapter,
	uint8_t *pcuEvtBuf)
{
#define NAN_SET_TX_ALLOWED 0
	struct _NAN_EVENT_LOWPOWER_CTRL *prFwEvt;
#if (NAN_SET_TX_ALLOWED == 1)
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_NDP_CONTEXT_T *prNdpCxt;
	struct _NAN_DATA_PATH_INFO_T *prDataPathInfo;
	uint8_t ucNdlIndex;
	uint8_t ucNdpCxtIdx;
	uint32_t i = 0;
#endif

	if (!prAdapter)
		return -EFAULT;

	prFwEvt = (struct _NAN_EVENT_LOWPOWER_CTRL *) pcuEvtBuf;

	DBGLOG(NAN, DEBUG,
		"Map: 0x%x\n",
		prFwEvt->ucPeerSchRecordTxMap);

#if (NAN_SET_TX_ALLOWED == 1)
	KAL_SPIN_LOCK_DECLARATION();

	prDataPathInfo = &(prAdapter->rDataPathInfo);

	for (ucNdlIndex = 0;
		ucNdlIndex < NAN_MAX_SUPPORT_NDL_NUM;
		ucNdlIndex++) {
		prNDL = &prDataPathInfo->arNDL[ucNdlIndex];
		if (prNDL->fgNDLValid == FALSE)
			continue;

		for (ucNdpCxtIdx = 0;
			ucNdpCxtIdx < NAN_MAX_SUPPORT_NDP_CXT_NUM;
			ucNdpCxtIdx++) {
			prNdpCxt = &prNDL->arNdpCxt[ucNdpCxtIdx];
			if (prNdpCxt->fgValid == FALSE)
				continue;

			KAL_ACQUIRE_SPIN_LOCK(prAdapter,
				SPIN_LOCK_NAN_NDL_FLOW_CTRL);

			for (i = 0; i < NAN_LINK_NUM; i++) {
				if (!prNdpCxt->prNanStaRec[i])
					continue;

				qmSetStaRecTxAllowed(
					prAdapter,
					prNdpCxt->prNanStaRec[i],
					FALSE);
			}

			KAL_RELEASE_SPIN_LOCK(prAdapter,
					SPIN_LOCK_NAN_NDL_FLOW_CTRL);
		}
	}
#endif

	g_ucNanLowPowerMode = TRUE;

	return WLAN_STATUS_SUCCESS;
}

#if CFG_SUPPORT_NAN_EXT
int mtk_cfg80211_vendor_nan_ext_indication(struct ADAPTER *prAdapter,
					   u8 *data, uint16_t u2Size)
{
	struct NanExtIndMsg nanExtInd = {0};
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDev;

	wiphy = wlanGetWiphy();
	if (!wiphy) {
		DBGLOG(NAN, ERROR, "wiphy error!\n");
		return -EFAULT;
	}

	prDev = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);

	if (!prDev) {
		DBGLOG(NAN, ERROR, "prDev for Bss0 not exist.\n");
		return -ENODEV;
	}
	wdev = prDev->ieee80211_ptr;

	nanExtInd.fwHeader.msgVersion = 1;
	nanExtInd.fwHeader.msgId = NAN_MSG_ID_EXT_IND;
	nanExtInd.fwHeader.msgLen = u2Size;
	nanExtInd.fwHeader.transactionId = 0;

	skb = kalCfg80211VendorEventAlloc(
		wiphy, wdev, sizeof(struct NanExtIndMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN_EXT, GFP_KERNEL);
	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	kalMemCopy(nanExtInd.data, data, u2Size);
	DBGLOG(NAN, DEBUG, "NAN Ext Ind:\n");
	DBGLOG_HEX(NAN, DEBUG, nanExtInd.data, u2Size)

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanExtIndMsg),
			     &nanExtInd) < 0)) {
		DBGLOG(NAN, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	return WLAN_STATUS_SUCCESS;
}

int
mtk_cfg80211_vendor_nan_ext(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sk_buff *skb = NULL;
	struct ADAPTER *prAdapter;

	struct NanExtCmdMsg extCmd = {0};
	struct NanExtResponseMsg extRsp = {0};
	u32 u4BufLen;
	u32 i4Status = -EINVAL;

	if (!wiphy) {
		DBGLOG(NAN, ERROR, "wiphy error!\n");
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(NAN, ERROR, "prGlueInfo error!\n");
		return -EINVAL;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(NAN, ERROR, "prAdapter error!\n");
		return -EINVAL;
	}

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev error!\n");
		return -EINVAL;
	}

	if (data == NULL || data_len < sizeof(struct NanExtCmdMsg)) {
		DBGLOG(NAN, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	/* read ext cmd */
	kalMemCopy(&extCmd, data, sizeof(struct NanExtCmdMsg));
	extRsp.fwHeader = extCmd.fwHeader;

	/* execute ext cmd */
	i4Status = kalIoctl(prGlueInfo, wlanoidNANExtCmd, &extCmd,
				sizeof(struct NanExtCmdMsg), &u4BufLen);
	if (i4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "kalIoctl NAN Ext Cmd failed\n");
		return -EFAULT;
	}
	kalMemCopy(extRsp.data, extCmd.data, extCmd.fwHeader.msgLen);
	extRsp.fwHeader.msgLen = extCmd.fwHeader.msgLen;

	DBGLOG(NAN, TRACE, "Resp data:", extRsp.data);
	DBGLOG_HEX(NAN, TRACE, extRsp.data, extCmd.fwHeader.msgLen)
	/* reply to framework */
	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy,
				sizeof(struct NanExtResponseMsg));

	if (!skb) {
		DBGLOG(NAN, ERROR, "Allocate skb %zu bytes failed\n",
		       sizeof(struct NanExtResponseMsg));
		return -ENOMEM;
	}

	if (unlikely(
		nla_put_nohdr(skb, sizeof(struct NanExtResponseMsg),
			(void *)&extRsp) < 0)) {
		DBGLOG(NAN, ERROR, "Fail send reply\n");
		goto failure;
	}

	i4Status = kalIoctl(prGlueInfo, wlanoidNANExtCmdRsp, (void *)&extRsp,
				sizeof(struct NanExtResponseMsg), &u4BufLen);

	if (i4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(NAN, ERROR, "kalIoctl NAN Ext Cmd Rsp failed\n");
		goto failure;
	}

	return cfg80211_vendor_cmd_reply(skb);

failure:
	kfree_skb(skb);
	return -EFAULT;
}
#endif /* CFG_SUPPORT_NAN_EXT */
#endif /* CFG_SUPPORT_NAN */
