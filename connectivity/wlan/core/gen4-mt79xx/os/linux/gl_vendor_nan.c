// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * gl_vendor_nan.c
 */

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
static uint16_t u16NanFollowupID;
uint8_t g_enableNAN = TRUE;
uint8_t g_disableNAN = TRUE;
uint8_t g_deEvent = FALSE;
uint8_t g_aucNanServiceName[NAN_MAX_SERVICE_NAME_LEN + 1];
uint8_t g_aucNanServiceId[NAN_SERVICE_HASH_LENGTH];
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define NAN_EVT_BCN_RX_DBG_STRING1 "<Drv> band: %u, RSSI:%d, beacon_len: %u, " \
				"tx_mode: %u, rate: %u, hw_ch: %u, bw: %u, " \
				"tsf0: %u, tsf1: %u\n"
#define NAN_EVT_BCN_RX_DBG_STRING2 " Cl:" MACSTR ", Src:" MACSTR ", rssi:%d, " \
				"chnl:%d, rate:%d, TsfL:0x%x\n"
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
const struct nla_policy mtk_wlan_vendor_nan_policy[NL80211_ATTR_MAX + 1] = {
#if KERNEL_VERSION(5, 9, 0) <= CFG80211_VERSION_CODE
	[NL80211_ATTR_VENDOR_DATA] = NLA_POLICY_MIN_LEN(0),
#elif KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	[NL80211_ATTR_VENDOR_DATA] = {.type = NLA_MIN_LEN, .len = 0 },
#else
	[NL80211_ATTR_VENDOR_DATA] = {.type = NLA_BINARY},
#endif
};

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

	DBGLOG(REQ, INFO, "Into nanMapPublishReqParams\n");

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
		(u8)((!GET_PUB_REPLY_IND_FLAG(*pPublishParams)) |
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

	DBGLOG(REQ, INFO,
	       "[Publish Req] ttl: %u, period: %u, recv_indication_cfg: %x, publish_type: %u,tx_type: %u, rssi_threshold_flag: %u, publish_match_indicator: %u, publish_count:%u, connmap:%u\n",
	       pOutparams->ttl, pOutparams->period,
	       pOutparams->recv_indication_cfg, pOutparams->publish_type,
	       pOutparams->tx_type, pOutparams->rssi_threshold_flag,
	       pOutparams->publish_match_indicator, pOutparams->publish_count,
	       pOutparams->connmap);

	DBGLOG(REQ, INFO, " readLen : %d\n", readLen);
	return readLen;
}

u16
nanMapSubscribeReqParams(u16 *pIndata, struct NanSubscribeRequest *pOutparams)
{
	u16 readLen = 0;
	u32 *pSubscribeParams = NULL;

	DBGLOG(NAN, INFO, "IN %s\n", __func__);

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

	DBGLOG(REQ, INFO,
	       "[Subscribe Req] ttl: %u, period: %u, subscribe_type: %u, ssiRequiredForMatchIndication: %u, subscribe_match_indicator: %x, rssi_threshold_flag: %u\n",
	       pOutparams->ttl, pOutparams->period,
	       pOutparams->subscribe_type,
	       pOutparams->ssiRequiredForMatchIndication,
	       pOutparams->subscribe_match_indicator,
	       pOutparams->rssi_threshold_flag);
	pOutparams->connmap = (u8)GET_SUB_CONNMAP(*pSubscribeParams);
	readLen += 4;
	DBGLOG(REQ, LOUD, "Subscribe readLen : %d\n", readLen);
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

	DBGLOG(NAN, INFO,
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
	prNanSdeaCtrlParms->eServUpdateInd =
		GET_SDEA_SERVICE_UPDATE_IND_PRESENT(*pIndata);
	prNanSdeaCtrlParms->fgFSDRequire = GET_SDEA_FSD_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->fgGAS = GET_SDEA_FSD_WITH_GAS(*pIndata);
	prNanSdeaCtrlParms->fgQoS = GET_SDEA_QOS_REQUIRED(*pIndata);
	prNanSdeaCtrlParms->fgRangeLimit =
		GET_SDEA_RANGE_LIMIT_PRESENT(*pIndata);

	DBGLOG(NAN, INFO,
	       "[%s]config_nan_data_path: %u, ndp_type: %u, security_cfg: %u\n",
	       __func__, prNanSdeaCtrlParms->config_nan_data_path,
	       prNanSdeaCtrlParms->ndp_type, prNanSdeaCtrlParms->security_cfg);
	DBGLOG(NAN, INFO,
	       "[%s]ranging_state: %u, eServUpdateInd: %u, fgFSDRequire: %u\n",
	       __func__, prNanSdeaCtrlParms->ranging_state,
	       prNanSdeaCtrlParms->eServUpdateInd,
	       prNanSdeaCtrlParms->fgFSDRequire);
	DBGLOG(NAN, INFO, "[%s]fgGAS: %u, fgQoS: %u, fgRangeLimit: %u\n",
	       __func__, prNanSdeaCtrlParms->fgGAS, prNanSdeaCtrlParms->fgQoS,
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

	DBGLOG(NAN, INFO,
	       "[%s]ranging_resolution: %u, ranging_interval_msec: %u, config_ranging_indications: %u\n",
	       __func__, prNanRangingCfg->ranging_resolution,
	       prNanRangingCfg->ranging_interval_msec,
	       prNanRangingCfg->config_ranging_indications);
	DBGLOG(NAN, INFO, "[%s]distance_egress_cm: %u\n", __func__,
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
	kalMemCopy(&prNanRangeRspCfgParms->peer_addr,
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

	DBGLOG(NAN, INFO,
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
wlanoidGetNANCapabilitiesRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
			     IN uint32_t u4SetBufferLen,
			     OUT uint32_t *pu4SetInfoLen)
{
	struct NanCapabilitiesRspMsg nanCapabilitiesRsp;
	struct NanCapabilitiesRspMsg *pNanCapabilitiesRsp =
		(struct NanCapabilitiesRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	nanCapabilitiesRsp.fwHeader.msgVersion = 1;
	nanCapabilitiesRsp.fwHeader.msgId = NAN_MSG_ID_CAPABILITIES_RSP;
	nanCapabilitiesRsp.fwHeader.msgLen =
		sizeof(struct NanCapabilitiesRspMsg);
	nanCapabilitiesRsp.fwHeader.transactionId =
		pNanCapabilitiesRsp->fwHeader.transactionId;
	nanCapabilitiesRsp.status = 0;
	nanCapabilitiesRsp.max_concurrent_nan_clusters = 1;
	nanCapabilitiesRsp.max_service_name_len = NAN_MAX_SERVICE_NAME_LEN;
	nanCapabilitiesRsp.max_match_filter_len = NAN_MAX_MATCH_FILTER_LEN;
	nanCapabilitiesRsp.max_service_specific_info_len =
		NAN_MAX_SERVICE_SPECIFIC_INFO_LEN;
	nanCapabilitiesRsp.max_sdea_service_specific_info_len =
		NAN_MAX_SDEA_SERVICE_SPECIFIC_INFO_LEN;
	nanCapabilitiesRsp.max_scid_len = NAN_MAX_SCID_BUF_LEN;
	nanCapabilitiesRsp.max_total_match_filter_len =
		256; /* only to pass VTS testing, need > 255 */
	nanCapabilitiesRsp.cipher_suites_supported =
		NAN_CIPHER_SUITE_SHARED_KEY_128_MASK;
	nanCapabilitiesRsp.max_ndi_interfaces = 1;
	nanCapabilitiesRsp.max_publishes = NAN_MAX_PUBLISH_NUM;
	nanCapabilitiesRsp.max_subscribes = NAN_MAX_SUBSCRIBE_NUM;
	nanCapabilitiesRsp.max_ndp_sessions = NAN_MAX_NDP_SESSIONS;
	nanCapabilitiesRsp.max_app_info_len = NAN_DP_MAX_APP_INFO_LEN;
	nanCapabilitiesRsp.max_queued_transmit_followup_msgs = 2;
	nanCapabilitiesRsp.max_subscribe_address = 1;

	/*  Fill values of nanCapabilitiesRsp */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
				  sizeof(struct NanCapabilitiesRspMsg) +
					  NLMSG_HDRLEN,
				  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanCapabilitiesRspMsg),
			     &nanCapabilitiesRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANEnableRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		    IN uint32_t u4SetBufferLen, OUT uint32_t *pu4SetInfoLen)
{
	struct NanEnableRspMsg nanEnableRsp;
	struct NanEnableRspMsg *pNanEnableRsp =
		(struct NanEnableRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

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
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanEnableRspMsg),
			     &nanEnableRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	g_disableNAN = TRUE;
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANDisableRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		     IN uint32_t u4SetBufferLen, OUT uint32_t *pu4SetInfoLen)
{
	struct NanDisableRspMsg nanDisableRsp;
	struct NanDisableRspMsg *pNanDisableRsp =
		(struct NanDisableRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

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
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanDisableRspMsg),
			     &nanDisableRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANConfigRsp(IN struct ADAPTER *prAdapter,
			      IN void *pvSetBuffer, IN uint32_t u4SetBufferLen,
			      OUT uint32_t *pu4SetInfoLen)
{
	struct NanConfigRspMsg nanConfigRsp;
	struct NanConfigRspMsg *pNanConfigRsp =
		(struct NanConfigRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

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
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanConfigRspMsg),
			     &nanConfigRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNanPublishRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		     IN uint32_t u4SetBufferLen, OUT uint32_t *pu4SetInfoLen)
{
	struct NanPublishServiceRspMsg nanPublishRsp;
	struct NanPublishServiceRspMsg *pNanPublishRsp =
		(struct NanPublishServiceRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	kalMemZero(&nanPublishRsp, sizeof(struct NanPublishServiceRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	DBGLOG(REQ, INFO, "%s\n", __func__);

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

	DBGLOG(REQ, INFO, "publish ID:%u, msgId:%u, msgLen:%u, tranID:%u\n",
	       nanPublishRsp.fwHeader.handle, nanPublishRsp.fwHeader.msgId,
	       nanPublishRsp.fwHeader.msgLen,
	       nanPublishRsp.fwHeader.transactionId);

	/*  Fill values of nanPublishRsp */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		sizeof(struct NanPublishServiceRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanPublishServiceRspMsg),
			     &nanPublishRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	/* Free the memory due to no use anymore */
	kfree(pvSetBuffer);

	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANCancelPublishRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
			   IN uint32_t u4SetBufferLen,
			   OUT uint32_t *pu4SetInfoLen)
{
	struct NanPublishServiceCancelRspMsg nanPublishCancelRsp;
	struct NanPublishServiceCancelRspMsg *pNanPublishCancelRsp =
		(struct NanPublishServiceCancelRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	kalMemZero(&nanPublishCancelRsp,
		   sizeof(struct NanPublishServiceCancelRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	DBGLOG(REQ, INFO, "%s\n", __func__);

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

	DBGLOG(REQ, INFO, "[%s] nanPublishCancelRsp.fwHeader.handle = %d\n",
	       __func__, nanPublishCancelRsp.fwHeader.handle);
	DBGLOG(REQ, INFO,
	       "[%s] nanPublishCancelRsp.fwHeader.transactionId = %d\n",
	       __func__, nanPublishCancelRsp.fwHeader.transactionId);

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		sizeof(struct NanPublishServiceCancelRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanPublishServiceCancelRspMsg),
			     &nanPublishCancelRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNanSubscribeRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		       IN uint32_t u4SetBufferLen,
		       OUT uint32_t *pu4SetInfoLen)
{
	struct NanSubscribeServiceRspMsg nanSubscribeRsp;
	struct NanSubscribeServiceRspMsg *pNanSubscribeRsp =
		(struct NanSubscribeServiceRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	kalMemZero(&nanSubscribeRsp, sizeof(struct NanSubscribeServiceRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	DBGLOG(REQ, INFO, "%s\n", __func__);

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
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		sizeof(struct NanSubscribeServiceRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanSubscribeServiceRspMsg),
			     &nanSubscribeRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);

	DBGLOG(REQ, INFO, "handle:%u,transactionId:%u\n",
	       nanSubscribeRsp.fwHeader.handle,
	       nanSubscribeRsp.fwHeader.transactionId);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANCancelSubscribeRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
			     IN uint32_t u4SetBufferLen,
			     OUT uint32_t *pu4SetInfoLen)
{
	struct NanSubscribeServiceCancelRspMsg nanSubscribeCancelRsp;
	struct NanSubscribeServiceCancelRspMsg *pNanSubscribeCancelRsp =
		(struct NanSubscribeServiceCancelRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	kalMemZero(&nanSubscribeCancelRsp,
		   sizeof(struct NanSubscribeServiceCancelRspMsg));
	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	DBGLOG(REQ, INFO, "%s\n", __func__);

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
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		sizeof(struct NanSubscribeServiceCancelRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanSubscribeServiceCancelRspMsg),
			     &nanSubscribeCancelRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	DBGLOG(REQ, ERROR, "handle:%u, transactionId:%u\n",
	       nanSubscribeCancelRsp.fwHeader.handle,
	       nanSubscribeCancelRsp.fwHeader.transactionId);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

u32
wlanoidNANFollowupRsp(IN struct ADAPTER *prAdapter, IN void *pvSetBuffer,
		      IN uint32_t u4SetBufferLen, OUT uint32_t *pu4SetInfoLen)
{
	struct NanTransmitFollowupRspMsg nanXmitFollowupRsp;
	struct NanTransmitFollowupRspMsg *pNanXmitFollowupRsp =
		(struct NanTransmitFollowupRspMsg *)pvSetBuffer;
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	wdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;
	kalMemZero(&nanXmitFollowupRsp,
		   sizeof(struct NanTransmitFollowupRspMsg));

	DBGLOG(REQ, INFO, "%s\n", __func__);

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

	u16NanFollowupID = nanXmitFollowupRsp.fwHeader.transactionId;

	/*  Fill values of NanSubscribeServiceCancelRspMsg */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev,
		sizeof(struct NanTransmitFollowupRspMsg) + NLMSG_HDRLEN,
		WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
			     sizeof(struct NanTransmitFollowupRspMsg),
			     &nanXmitFollowupRsp) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	kfree(pvSetBuffer);
	return WLAN_STATUS_SUCCESS;
}

static struct wfpal_channel sunrise_to_wfpal_channel(
		struct RF_CHANNEL_INFO sunrise_channel,
		enum ENUM_CHNL_EXT eSco)
{
	struct wfpal_channel c = {0};
	uint32_t defaultBw = 0;

	c.channel = sunrise_channel.ucChannelNum;
	switch (sunrise_channel.eBand) {
	case BAND_2G4:
		c.flags = NAN_C_FLAG_2GHZ;
		defaultBw = NAN_C_FLAG_20MHZ;
		break;
	case BAND_5G:
		c.flags = NAN_C_FLAG_5GHZ;
		defaultBw = NAN_C_FLAG_80MHZ;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case BAND_6G:
		c.flags = NAN_C_FLAG_6GHZ;
		defaultBw = NAN_C_FLAG_80MHZ;
		break;
#endif /* CFG_SUPPORT_WIFI_6G */
	default:
		c.flags = NAN_C_FLAG_2GHZ;
		defaultBw = NAN_C_FLAG_20MHZ;
		break;
	}
	switch (sunrise_channel.ucChnlBw) {
	case CW_20_40MHZ:
		if (eSco == CHNL_EXT_SCN) {
			c.flags |= NAN_C_FLAG_20MHZ;
		} else if (eSco == CHNL_EXT_SCA) {
			c.flags |= NAN_C_FLAG_40MHZ;
			c.flags |= NAN_C_FLAG_EXTENSION_ABOVE;
		} else if (eSco == CHNL_EXT_SCB) {
			c.flags |= NAN_C_FLAG_40MHZ;
		}
		break;
	case CW_80MHZ:
		c.flags |= NAN_C_FLAG_80MHZ;
		break;
	case CW_160MHZ:
		c.flags |= NAN_C_FLAG_160MHZ;
		break;
	default:
		DBGLOG(REQ, ERROR, "%s: Unknown Channel Bw: %d\n",
			__func__, sunrise_channel.ucChnlBw);
		c.flags |= defaultBw;
		break;
	}
	return c;
}

struct NanDataPathInitiatorNDPE g_ndpReqNDPE;

int mtk_cfg80211_vendor_nan(struct wiphy *wiphy,
	struct wireless_dev *wdev, const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sk_buff *skb = NULL;
	struct ADAPTER *prAdapter;
	struct _NanMsgHeader nanMsgHdr;
	struct _NanTlv outputTlv;
	u16 readLen = 0;
	u32 u4BufLen;
	int32_t i4Status = -EINVAL;
	u32 u4DelayIdx;

	int remainingLen;

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
		log_dbg(REQ, ERROR, "data error(len=%d)\n", data_len);
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

	DBGLOG(INIT, INFO, "DATA len from user %d\n", data_len);

	kalMemCopy(&nanMsgHdr, (struct _NanMsgHeader *)data,
		sizeof(struct _NanMsgHeader));
	data += sizeof(struct _NanMsgHeader);

	DBGLOG(INIT, INFO, "nanMsgHdr.length %u, nanMsgHdr.msgId %d\n",
		nanMsgHdr.msgLen, nanMsgHdr.msgId);

	switch (nanMsgHdr.msgId) {
	case NAN_MSG_ID_ENABLE_REQ: {
		struct NanEnableRequest nanEnableReq;
		struct NanEnableRspMsg nanEnableRsp;

		if (prAdapter->fgNanEnable) {
			DBGLOG(INIT, WARN, "NAN Enable Already\n");
			return -EPERM;
		}

		for (u4DelayIdx = 0; u4DelayIdx < 5; u4DelayIdx++) {
			if (g_enableNAN == TRUE) {
				g_enableNAN = FALSE;
				break;
			}
			kalMsleep(1000);
		}

		kalMemZero(&nanEnableReq, sizeof(struct NanEnableRequest));
		kalMemZero(&nanEnableRsp, sizeof(struct NanEnableRspMsg));
		while ((remainingLen > 0) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_CONFIG_DISCOVERY_INDICATIONS:
				if (outputTlv.length >
					sizeof(
					nanEnableReq.discovery_indication_cfg
					)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					return -EFAULT;
				}
				kalMemCopy(&nanEnableReq
					.discovery_indication_cfg,
					outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_CLUSTER_ID_LOW:
				if (outputTlv.length >
					sizeof(nanEnableReq.cluster_low)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					return -EFAULT;
				}
				kalMemCopy(&nanEnableReq.cluster_low,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_CLUSTER_ID_HIGH:
				if (outputTlv.length >
					sizeof(nanEnableReq.cluster_high)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					return -EFAULT;
				}
				kalMemCopy(&nanEnableReq.cluster_high,
				       outputTlv.value, outputTlv.length);
				break;
			case NAN_TLV_TYPE_MASTER_PREFERENCE:
				if (outputTlv.length >
					sizeof(nanEnableReq.master_pref)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					return -EFAULT;
				}
				kalMemCopy(&nanEnableReq.master_pref,
				       outputTlv.value, outputTlv.length);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		nanDevBssActivate(prAdapter);
		nanDevEnable(prAdapter);

		kalMemZero(&nanEnableReq, sizeof(struct NanEnableRequest));
		nanEnableReq.master_pref = prAdapter->rWifiVar.ucMasterPref;
		nanEnableReq.config_random_factor_force = 0;
		nanEnableReq.random_factor_force_val = 0;
		nanEnableReq.config_hop_count_force = 0;
		nanEnableReq.hop_count_force_val = 0;
		nanEnableReq.config_5g_channel =
			prGlueInfo->prAdapter->rWifiVar.ucConfig5gChannel;
		nanEnableReq.channel_5g_val =
			prGlueInfo->prAdapter->rWifiVar.ucChannel5gVal;

		/*
		 * Send request to CNM module
		 * hold on request until DBDC switch done.
		 */
		nanEnableRsp.status = nanDevSendEnableRequestToCnm(prAdapter);

		kalMemCopy(&nanEnableRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanEnableRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanEnableRspMsg),
					   &nanEnableRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}

		for (u4DelayIdx = 0; u4DelayIdx < 5; u4DelayIdx++) {
			if (g_deEvent == TRUE) {
				g_deEvent = FALSE;
				break;
			}
			kalMsleep(1000);
		}
		i4Status = kalIoctl(prGlueInfo, wlanoidNANEnableRsp,
				    (void *)&nanEnableRsp,
				    sizeof(struct NanEnableRequest), FALSE,
				    FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}
		prAdapter->fgNanEnable = TRUE;
		break;
	}
	case NAN_MSG_ID_DISABLE_REQ: {
		struct NanDisableRspMsg nanDisableRsp;

		if (!prAdapter->fgNanEnable) {
			DBGLOG(INIT, WARN, "NAN Not Enable yet\n");
			return -EPERM;
		}

		for (u4DelayIdx = 0; u4DelayIdx < 5; u4DelayIdx++) {
			if (g_disableNAN == TRUE) {
				g_disableNAN = FALSE;
				break;
			}
			kalMsleep(1000);
		}

		kalMemZero(&nanDisableRsp, sizeof(struct NanDisableRspMsg));
		nanDisableRsp.status =
			nanDisableHandler(prAdapter);

		kalMemCopy(&nanDisableRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanDisableRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanDisableRspMsg),
					   &nanDisableRsp) < 0)) {
			kfree_skb(skb);
			return -EFAULT;
		}
		i4Status = kalIoctl(prGlueInfo, wlanoidNANDisableRsp,
				    (void *)&nanDisableRsp,
				    sizeof(struct NanDisableRspMsg), FALSE,
				    FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}
		break;
	}
	case NAN_MSG_ID_CONFIGURATION_REQ: {
		struct NanConfigRequest *prNanConfigReq = NULL;
		struct NanConfigRspMsg nanConfigRsp;

		prNanConfigReq = kalMemAlloc(
				 sizeof(*prNanConfigReq),
				VIR_MEM_TYPE);

		if (!prNanConfigReq) {
			DBGLOG(REQ, ERROR,
				"Alloc mem failed for Nan config request\n");
			return -ENOMEM;
		}

		kalMemZero(prNanConfigReq, sizeof(*prNanConfigReq));
		kalMemZero(&nanConfigRsp, sizeof(struct NanConfigRspMsg));

		while ((remainingLen > 0) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_MASTER_PREFERENCE:
				if (outputTlv.length >
					sizeof(prNanConfigReq->master_pref)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					i4Status = -EFAULT;
					goto exit;
				}
				kalMemCopy(&prNanConfigReq->master_pref,
				       outputTlv.value, outputTlv.length);
				nanDevSetMasterPreference(
					prGlueInfo->prAdapter,
					prNanConfigReq->master_pref);
				break;
			default:
				break;
			}
			remainingLen -= readLen;
			data += readLen;
			memset(&outputTlv, 0, sizeof(outputTlv));
		}

		nanConfigRsp.status = 0;

		kalMemCopy(&nanConfigRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanConfigRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			i4Status = -ENOMEM;
			goto exit;
		}
		if (unlikely(nla_put_nohdr(skb, sizeof(struct NanConfigRspMsg),
				&nanConfigRsp) < 0)) {
			kfree_skb(skb);
			i4Status = -EFAULT;
			goto exit;
		}
		i4Status = kalIoctl(prGlueInfo, wlanoidNANConfigRsp,
			(void *)&nanConfigRsp, sizeof(struct NanConfigRspMsg),
			FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			i4Status = -EFAULT;
			goto exit;
		}
exit:
		kalMemFree(prNanConfigReq,
			VIR_MEM_TYPE,
			sizeof(*prNanConfigReq));
		if (i4Status != WLAN_STATUS_SUCCESS)
			return i4Status;
		break;
	}
	case NAN_MSG_ID_CAPABILITIES_REQ: {
		struct NanCapabilitiesRspMsg nanCapabilitiesRsp;

		kalMemCopy(&nanCapabilitiesRsp.fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanCapabilitiesRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
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
				    sizeof(struct NanCapabilitiesRspMsg), FALSE,
				    FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			return -EFAULT;
		}
		DBGLOG(INIT, INFO, "i4Status = %u\n", i4Status);

		break;
	}
	case NAN_MSG_ID_PUBLISH_SERVICE_REQ: {
		struct NanPublishRequest *pNanPublishReq = NULL;
		struct NanPublishServiceRspMsg *pNanPublishRsp = NULL;
		uint16_t publish_id = 0;
		uint8_t ucCipherType = 0;

		DBGLOG(REQ, INFO, "IN case NAN_MSG_ID_PUBLISH_SERVICE_REQ\n");

		pNanPublishReq =
			kmalloc(sizeof(struct NanPublishRequest), GFP_ATOMIC);

		if (!pNanPublishReq) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanPublishRsp = kmalloc(sizeof(struct NanPublishServiceRspMsg),
					 GFP_ATOMIC);

		if (!pNanPublishRsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
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

		while ((remainingLen > 0) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
			       outputTlv.type);
			DBGLOG(REQ, INFO, "outputTlv.length:%u\n",
			       outputTlv.length);

			switch (outputTlv.type) {
			case NAN_TLV_TYPE_SERVICE_NAME:
				if (outputTlv.length >
					NAN_FW_MAX_SERVICE_NAME_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				kalMemCopy(pNanPublishReq->service_name,
				       outputTlv.value, outputTlv.length);
				kalMemCopy(g_aucNanServiceName,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->service_name_len =
					outputTlv.length;
				DBGLOG(INIT, INFO,
				       "pNanPublishReq->service_name_len:%u\n",
				       pNanPublishReq->service_name_len);
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
				kalMemCopy(pNanPublishReq
					->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->service_specific_info_len =
					outputTlv.length;
				DBGLOG(REQ, INFO,
				       "pNanPublishReq->service_specific_info_len:%u\n",
				       pNanPublishReq
					       ->service_specific_info_len);
				break;
			case NAN_TLV_TYPE_RX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				kalMemCopy(pNanPublishReq->rx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->rx_match_filter_len =
					outputTlv.length;
				DBGLOG(REQ, INFO,
				       "pNanPublishReq->rx_match_filter_len:%u\n",
				       pNanPublishReq->rx_match_filter_len);
				dumpMemory8(
					(uint8_t *)
						pNanPublishReq->rx_match_filter,
					pNanPublishReq->rx_match_filter_len);
				break;
			case NAN_TLV_TYPE_TX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				kalMemCopy(pNanPublishReq->tx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->tx_match_filter_len =
					outputTlv.length;
				DBGLOG(REQ, INFO,
				       "pNanPublishReq->tx_match_filter_len:%u\n",
				       pNanPublishReq->tx_match_filter_len);
				dumpMemory8(
					(uint8_t *)
						pNanPublishReq->tx_match_filter,
					pNanPublishReq->tx_match_filter_len);
				break;
			case NAN_TLV_TYPE_NAN_SERVICE_ACCEPT_POLICY:
				pNanPublishReq->service_responder_policy =
					*(outputTlv.value);
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
				kalMemCopy(pNanPublishReq
					->key_info.body.pmk_info
					.pmk, outputTlv.value,
					outputTlv.length);
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
				kalMemCopy(pNanPublishReq->key_info.body
					       .passphrase_info.passphrase,
				       outputTlv.value, outputTlv.length);
				pNanPublishReq->key_info.body.passphrase_info
					.passphrase_len = outputTlv.length;
				break;
			case NAN_TLV_TYPE_SDEA_CTRL_PARAMS:
				nanMapSdeaCtrlParams(
					(u32 *)outputTlv.value,
					&pNanPublishReq->sdea_params);
				/* Fixme: support it when TLV for
				 * service update indicator is ready
				 */
				pNanPublishReq->sdea_params.eServUpdateInd =
					NAN_SERV_UPDATE_IND_ABSENT;
				break;
			case NAN_TLV_TYPE_NAN_RANGING_CFG:
				nanMapRangingConfigParams(
					(u32 *)outputTlv.value,
					&pNanPublishReq->ranging_cfg);
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanPublishRsp);
					kfree(pNanPublishReq);
					return -EFAULT;
				}
				kalMemCopy(pNanPublishReq
					->sdea_service_specific_info,
					outputTlv.value, outputTlv.length);
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
		kalMemCopy(&pNanPublishRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanPublishServiceRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
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
			DBGLOG(REQ, ERROR, "Fail send reply\n");
			return -EFAULT;
		}

		/* publish ID from MsgHdr, 0xFFFF means new publish*/
		if (nanMsgHdr.handle != 0xFFFF)
			pNanPublishReq->publish_id = nanMsgHdr.handle;

		/* return publish ID */
		publish_id = (uint16_t)nanPublishRequest(prGlueInfo->prAdapter,
							pNanPublishReq);
		pNanPublishRsp->fwHeader.handle = publish_id;
		DBGLOG(REQ, INFO,
		       "pNanPublishRsp->fwHeader.handle %u, publish_id : %u\n",
		       pNanPublishRsp->fwHeader.handle, publish_id);
		if (publish_id != 0) {
			nanDiscInstanceAdd(prAdapter, publish_id, NAN_PUBLISH,
				pNanPublishReq->aucServiceHash,
				pNanPublishReq->service_name,
				pNanPublishReq->service_name_len);
		}

		if (pNanPublishReq->sdea_params.security_cfg
				&& publish_id != 0) {
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

		i4Status = kalIoctl(prGlueInfo, wlanoidNanPublishRsp,
				    (void *)pNanPublishRsp,
				    sizeof(struct NanPublishServiceRspMsg),
				    FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			kfree_skb(skb);
			kfree(pNanPublishReq);
			return -EFAULT;
		}

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
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanPublishCancelRsp =
			kmalloc(sizeof(struct NanPublishServiceCancelRspMsg),
				GFP_ATOMIC);

		if (!pNanPublishCancelRsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			kfree(pNanPublishCancelReq);
			return -ENOMEM;
		}

		DBGLOG(REQ, INFO, "Enter CANCEL Publish Request\n");
		pNanPublishCancelReq->publish_id = nanMsgHdr.handle;

		DBGLOG(REQ, INFO, "PID %d\n", pNanPublishCancelReq->publish_id);
		rStatus = nanCancelPublishRequest(prGlueInfo->prAdapter,
						  pNanPublishCancelReq);

		/* Prepare for command reply */
		kalMemCopy(&pNanPublishCancelRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanPublishServiceCancelRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
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
			DBGLOG(REQ, INFO, "CANCEL Publish Error %x\n", rStatus);
			pNanPublishCancelRsp->status = NAN_I_STATUS_DE_FAILURE;
		} else {
			DBGLOG(REQ, INFO, "CANCEL Publish Success %x\n",
			       rStatus);
			pNanPublishCancelRsp->status = NAN_I_STATUS_SUCCESS;
		}

		i4Status =
			kalIoctl(prGlueInfo, wlanoidNANCancelPublishRsp,
				 (void *)pNanPublishCancelRsp,
				 sizeof(struct NanPublishServiceCancelRspMsg),
				 FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			kfree_skb(skb);
			kfree(pNanPublishCancelReq);
			return -EFAULT;
		}

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

		DBGLOG(REQ, INFO, "In NAN_MSG_ID_SUBSCRIBE_SERVICE_REQ\n");

		pNanSubscribeReq =
			kmalloc(sizeof(struct NanSubscribeRequest), GFP_ATOMIC);
		if (!pNanSubscribeReq) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanSubscribeRsp = kmalloc(
			sizeof(struct NanSubscribeServiceRspMsg), GFP_ATOMIC);
		if (!pNanSubscribeRsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			kalMemFree(pNanSubscribeReq, PHY_MEM_TYPE,
				sizeof(struct NanSubscribeRequest));
			return -ENOMEM;
		}

		kalMemZero(pNanSubscribeReq,
			   sizeof(struct NanSubscribeRequest));
		kalMemZero(pNanSubscribeRsp,
			   sizeof(struct NanSubscribeServiceRspMsg));
		/* Mapping subscribe req related parameters */
		readLen =
			nanMapSubscribeReqParams((u16 *)data, pNanSubscribeReq);
		remainingLen -= readLen;
		data += readLen;
		while ((remainingLen > 0) &&
		       (0 !=
			(readLen = nan_read_tlv((u8 *)data, &outputTlv)))) {
			switch (outputTlv.type) {
			case NAN_TLV_TYPE_SERVICE_NAME:
				if (outputTlv.length >
					NAN_FW_MAX_SERVICE_NAME_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				kalMemCopy(pNanSubscribeReq->service_name,
				       outputTlv.value, outputTlv.length);
				kalMemCopy(g_aucNanServiceName,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->service_name_len =
					outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				DBGLOG(REQ, INFO, "outputTlv.value: %s\n",
				       outputTlv.value);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->service_name_len: %u\n",
				       pNanSubscribeReq->service_name_len);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->service_name:%s\n",
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
				kalMemCopy(pNanSubscribeReq
					->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->service_specific_info_len =
					outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				DBGLOG(REQ, INFO, "outputTlv.value: %s\n",
				       outputTlv.value);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->service_specific_info_len: %u\n",
				       pNanSubscribeReq
					       ->service_specific_info_len);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->service_specific_info:%s\n",
				       pNanSubscribeReq->service_specific_info);
				break;
			case NAN_TLV_TYPE_RX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				kalMemCopy(pNanSubscribeReq->rx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->rx_match_filter_len =
					outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				DBGLOG(REQ, INFO, "outputTlv.value: %s\n",
				       outputTlv.value);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->rx_match_filter_len: %u\n",
				       pNanSubscribeReq->rx_match_filter_len);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->rx_match_filter:%s\n",
				       pNanSubscribeReq->rx_match_filter);
				dumpMemory8((uint8_t *)pNanSubscribeReq
						    ->rx_match_filter,
					    outputTlv.length);
				break;
			case NAN_TLV_TYPE_TX_MATCH_FILTER:
				if (outputTlv.length >
					NAN_MAX_MATCH_FILTER_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				kalMemCopy(pNanSubscribeReq->tx_match_filter,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->tx_match_filter_len =
					outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				DBGLOG(REQ, INFO, "outputTlv.value: %s\n",
				       outputTlv.value);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->tx_match_filter_len: %u\n",
				       pNanSubscribeReq->tx_match_filter_len);
				DBGLOG(REQ, INFO,
				       "pNanSubscribeReq->tx_match_filter:%s\n",
				       pNanSubscribeReq->tx_match_filter);
				dumpMemory8((uint8_t *)pNanSubscribeReq
						    ->tx_match_filter,
					    outputTlv.length);
				break;
			case NAN_TLV_TYPE_MAC_ADDRESS:
				if ((outputTlv.length >
					NAN_MAC_ADDR_LEN) ||
					(i >
					NAN_MAX_SUBSCRIBE_MAX_ADDRESS - 1)) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				/* Get column neumbers */
				kalMemCopy(pNanSubscribeReq->intf_addr[i],
				       outputTlv.value, outputTlv.length);
				i++;
				break;
			case NAN_TLV_TYPE_NAN_CSID:
				pNanSubscribeReq->cipher_type =
					*(outputTlv.value);
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
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
				kalMemCopy(pNanSubscribeReq
					->key_info.body.pmk_info
					.pmk, outputTlv.value,
					outputTlv.length);
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
				kalMemCopy(pNanSubscribeReq->key_info.body
					       .passphrase_info.passphrase,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq->key_info.body.passphrase_info
					.passphrase_len = outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				break;
			case NAN_TLV_TYPE_SDEA_CTRL_PARAMS:
				nanMapSdeaCtrlParams(
					(u32 *)outputTlv.value,
					&pNanSubscribeReq->sdea_params);
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				pNanSubscribeReq->sdea_params.eServUpdateInd =
					NAN_SERV_UPDATE_IND_ABSENT;
				break;
			case NAN_TLV_TYPE_NAN_RANGING_CFG:
				fgRangingCFG = TRUE;
				DBGLOG(NAN, INFO, "fgRangingCFG %d \n",
					fgRangingCFG);
				nanMapRangingConfigParams(
					(u32 *)outputTlv.value,
					&pNanSubscribeReq->ranging_cfg);
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanSubscribeReq);
					kfree(pNanSubscribeRsp);
					return -EFAULT;
				}
				kalMemCopy(pNanSubscribeReq
					->sdea_service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanSubscribeReq
					->sdea_service_specific_info_len =
					outputTlv.length;
				DBGLOG(REQ, INFO, "outputTlv.type:%u\n",
				       outputTlv.type);
				DBGLOG(REQ, INFO, "outputTlv.length: %u\n",
				       outputTlv.length);
				break;
			case NAN_TLV_TYPE_NAN20_RANGING_REQUEST:
				fgRangingREQ = TRUE;
				DBGLOG(NAN, INFO, "fgRangingREQ %d \n",
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
		kalMemCopy(&pNanSubscribeRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanSubscribeServiceRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			kfree(pNanSubscribeReq);
			kfree(pNanSubscribeRsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct NanSubscribeServiceRspMsg),
				     pNanSubscribeRsp) < 0)) {
			kfree_skb(skb);
			kfree(pNanSubscribeReq);
			kfree(pNanSubscribeRsp);
			return -EFAULT;
		}
		/* Ranging */
		if (fgRangingCFG && fgRangingREQ) {

			struct NanRangeRequest *rgreq = NULL;
			uint16_t rgId = 0;
			uint32_t rStatus;

			rgreq = kmalloc(sizeof(struct NanRangeRequest), GFP_ATOMIC);
			if (!rgreq) {
				DBGLOG(REQ, ERROR, "Allocate failed\n");
				kalMemFree(pNanSubscribeReq, PHY_MEM_TYPE,
					sizeof(
					struct NanSubscribeRequest));
				kalMemFree(pNanSubscribeRsp, PHY_MEM_TYPE,
					sizeof(
					struct NanSubscribeServiceRspMsg));
				kfree_skb(skb);
				return -ENOMEM;
			}

			kalMemZero(rgreq, sizeof(struct NanRangeRequest));

			kalMemCopy(&rgreq->peer_addr,
				&pNanSubscribeReq->range_response_cfg.peer_addr,
				NAN_MAC_ADDR_LEN);
			kalMemCopy(&rgreq->ranging_cfg,
				&pNanSubscribeReq->ranging_cfg,
				sizeof(struct NanRangingCfg));
			rgreq->range_id =
				pNanSubscribeReq->range_response_cfg.requestor_instance_id;
			DBGLOG(NAN, INFO, MACSTR
				" id %d reso %d intev %d indicat %d ING CM %d ENG CM %d\n",
				MAC2STR(rgreq->peer_addr),
				rgreq->range_id,
				rgreq->ranging_cfg.ranging_resolution,
				rgreq->ranging_cfg.ranging_interval_msec,
				rgreq->ranging_cfg.config_ranging_indications,
				rgreq->ranging_cfg.distance_ingress_cm,
				rgreq->ranging_cfg.distance_egress_cm);
			rStatus = nanRangingRequest(prGlueInfo->prAdapter, &rgId, rgreq);

			pNanSubscribeRsp->fwHeader.handle = rgId;
			i4Status = kalIoctl(prGlueInfo, wlanoidNanSubscribeRsp,
				       (void *)pNanSubscribeRsp,
				       sizeof(struct NanSubscribeServiceRspMsg),
				       FALSE, FALSE, FALSE, &u4BufLen);
			if (i4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, ERROR, "kalIoctl failed\n");
				kfree(pNanSubscribeReq);
				kfree(rgreq);
				kfree_skb(skb);
				return -EFAULT;
			}
			kfree(rgreq);
			kfree(pNanSubscribeReq);
			break;

		}

		prAdapter->fgIsNANfromHAL = TRUE;

		/* return subscribe ID */
		Subscribe_id = (uint16_t)nanSubscribeRequest(
			prGlueInfo->prAdapter, pNanSubscribeReq);
		pNanSubscribeRsp->fwHeader.handle = Subscribe_id;
		if (Subscribe_id != 0) {
			nanDiscInstanceAdd(prAdapter, Subscribe_id,
				NAN_SUBSCRIBE,
				pNanSubscribeReq->aucServiceHash,
				pNanSubscribeReq->service_name,
				pNanSubscribeReq->service_name_len);
		}

		DBGLOG(REQ, INFO,
		       "Subscribe_id:%u, pNanSubscribeRsp->fwHeader.handle:%u\n",
		       Subscribe_id, pNanSubscribeRsp->fwHeader.handle);
		i4Status = kalIoctl(prGlueInfo, wlanoidNanSubscribeRsp,
				    (void *)pNanSubscribeRsp,
				    sizeof(struct NanSubscribeServiceRspMsg),
				    FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			kfree(pNanSubscribeReq);
			kfree_skb(skb);
			return -EFAULT;
		}

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
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		pNanSubscribeCancelRsp =
			kmalloc(sizeof(struct NanSubscribeServiceCancelRspMsg),
				GFP_ATOMIC);
		if (!pNanSubscribeCancelRsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			kalMemFree(pNanSubscribeCancelReq, PHY_MEM_TYPE,
				sizeof(struct NanSubscribeCancelRequest));
			return -ENOMEM;
		}
		kalMemZero(pNanSubscribeCancelReq,
			   sizeof(struct NanSubscribeCancelRequest));
		kalMemZero(pNanSubscribeCancelRsp,
			   sizeof(struct NanSubscribeServiceCancelRspMsg));

		DBGLOG(REQ, INFO, "Enter CANCEL Subscribe Request\n");
		pNanSubscribeCancelReq->subscribe_id = nanMsgHdr.handle;

		DBGLOG(REQ, INFO, "PID %d\n",
		       pNanSubscribeCancelReq->subscribe_id);
		rStatus = nanCancelSubscribeRequest(prGlueInfo->prAdapter,
						    pNanSubscribeCancelReq);

		/* Prepare Cancel Subscribe command reply message */
		kalMemCopy(&pNanSubscribeCancelRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanSubscribeServiceCancelRspMsg));
		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
			kfree(pNanSubscribeCancelReq);
			kfree(pNanSubscribeCancelRsp);
			return -ENOMEM;
		}
		if (unlikely(nla_put_nohdr(
				     skb,
				     sizeof(struct
					    NanSubscribeServiceCancelRspMsg),
				     pNanSubscribeCancelRsp) < 0)) {
			kfree_skb(skb);
			kfree(pNanSubscribeCancelReq);
			kfree(pNanSubscribeCancelRsp);
			return -EFAULT;
		}

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(NAN, ERROR, "CANCEL Subscribe Error %X\n",
			       rStatus);
			pNanSubscribeCancelRsp->status =
				NAN_I_STATUS_DE_FAILURE;
		} else {
			DBGLOG(NAN, INFO, "CANCEL Subscribe Success %X\n",
			       rStatus);
			pNanSubscribeCancelRsp->status = NAN_I_STATUS_SUCCESS;
		}

		i4Status =
			kalIoctl(prGlueInfo, wlanoidNANCancelSubscribeRsp,
				 (void *)pNanSubscribeCancelRsp,
				 sizeof(struct NanSubscribeServiceCancelRspMsg),
				 FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			kfree_skb(skb);
			kfree(pNanSubscribeCancelReq);
			kfree(pNanSubscribeCancelRsp);
			return -EFAULT;
		}

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
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanXmitFollowupRsp = kmalloc(
			sizeof(struct NanTransmitFollowupRspMsg), GFP_ATOMIC);
		if (!pNanXmitFollowupRsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			kfree(pNanXmitFollowupReq);
			return -ENOMEM;
		}

		kalMemZero(pNanXmitFollowupReq,
			   sizeof(struct NanTransmitFollowupRequest));
		kalMemZero(pNanXmitFollowupRsp,
			   sizeof(struct NanTransmitFollowupRspMsg));

		DBGLOG(REQ, INFO, "Enter Transmit follow up Request\n");

		/* Mapping publish req related parameters */
		readLen = nanMapFollowupReqParams((u32 *)data,
						  pNanXmitFollowupReq);
		remainingLen -= readLen;
		data += readLen;
		pNanXmitFollowupReq->publish_subscribe_id = nanMsgHdr.handle;

		while ((remainingLen > 0) &&
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
				kalMemCopy(pNanXmitFollowupReq->addr,
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
				kalMemCopy(pNanXmitFollowupReq
					       ->service_specific_info,
				       outputTlv.value, outputTlv.length);
				pNanXmitFollowupReq->service_specific_info_len =
					outputTlv.length;
				break;
			case NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO:
				if (outputTlv.length >
					NAN_SDEA_SERVICE_SPECIFIC_INFO_LEN) {
					DBGLOG(NAN, ERROR,
						"outputTlv.length is invalid!\n");
					kfree(pNanXmitFollowupReq);
					kfree(pNanXmitFollowupRsp);
					return -EFAULT;
				}
				kalMemCopy(pNanXmitFollowupReq
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
		kalMemCopy(&pNanXmitFollowupRsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanTransmitFollowupRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
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
			DBGLOG(REQ, ERROR, "Fail send reply\n");
			return -EFAULT;
		}

		rStatus = nanTransmitRequest(prGlueInfo->prAdapter,
					     pNanXmitFollowupReq);
		if (rStatus != WLAN_STATUS_SUCCESS)
			pNanXmitFollowupRsp->status = NAN_I_STATUS_DE_FAILURE;
		else
			pNanXmitFollowupRsp->status = NAN_I_STATUS_SUCCESS;

		i4Status = kalIoctl(prGlueInfo, wlanoidNANFollowupRsp,
				    (void *)pNanXmitFollowupRsp,
				    sizeof(struct NanTransmitFollowupRspMsg),
				    FALSE, FALSE, FALSE, &u4BufLen);
		if (i4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, ERROR, "kalIoctl failed\n");
			kfree(pNanXmitFollowupReq);
			kfree_skb(skb);
			return -EFAULT;
		}

		kfree(pNanXmitFollowupReq);
		break;
	}
	case NAN_MSG_ID_BEACON_SDF_REQ: {
		u16 vsa_length = 0;
		u32 *pXmitVSAparms = NULL;
		struct NanTransmitVendorSpecificAttribute *pNanXmitVSAttrReq =
			NULL;
		struct NanBeaconSdfPayloadRspMsg *pNanBcnSdfVSARsp = NULL;

		DBGLOG(NAN, INFO, "Enter Beacon SDF Request.\n");

		pNanXmitVSAttrReq = kmalloc(
			sizeof(struct NanTransmitVendorSpecificAttribute),
			GFP_ATOMIC);

		if (!pNanXmitVSAttrReq) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}

		pNanBcnSdfVSARsp = kmalloc(
			sizeof(struct NanBeaconSdfPayloadRspMsg), GFP_ATOMIC);

		if (!pNanBcnSdfVSARsp) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			kfree(pNanXmitVSAttrReq);
			return -ENOMEM;
		}

		while ((remainingLen > 0) &&
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
				kalMemCopy(pNanXmitVSAttrReq->vsa,
					outputTlv.value,
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
		kalMemCopy(&pNanBcnSdfVSARsp->fwHeader, &nanMsgHdr,
		       sizeof(struct _NanMsgHeader));
		pNanBcnSdfVSARsp->fwHeader.msgId = NAN_MSG_ID_BEACON_SDF_RSP;
		pNanBcnSdfVSARsp->fwHeader.msgLen =
			sizeof(struct NanBeaconSdfPayloadRspMsg);

		pNanBcnSdfVSARsp->status = NAN_I_STATUS_SUCCESS;

		skb = cfg80211_vendor_cmd_alloc_reply_skb(
			wiphy, sizeof(struct NanBeaconSdfPayloadRspMsg));

		if (!skb) {
			DBGLOG(REQ, ERROR, "Allocate skb failed\n");
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

		kfree(pNanXmitVSAttrReq);
		kfree(pNanBcnSdfVSARsp);

		break;
	}
	case NAN_MSG_ID_TESTMODE_REQ:
	{
		struct NanDebugParams *pNanDebug = NULL;

		pNanDebug = kmalloc(sizeof(struct NanDebugParams), GFP_ATOMIC);
		if (!pNanDebug) {
			DBGLOG(REQ, ERROR, "Allocate failed\n");
			return -ENOMEM;
		}
		kalMemZero(pNanDebug, sizeof(struct NanDebugParams));
		DBGLOG(REQ, INFO, "NAN_MSG_ID_TESTMODE_REQ\n");

		while ((remainingLen > 0) &&
			(0 != (readLen = nan_read_tlv((u8 *)data,
			&outputTlv)))) {
			DBGLOG(REQ, INFO, "outputTlv.type= %d\n",
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
				kalMemCopy(pNanDebug, outputTlv.value,
					outputTlv.length);
				switch (pNanDebug->cmd) {
				case NAN_TEST_MODE_CMD_DISABLE_NDPE:
					g_ndpReqNDPE.fgEnNDPE = TRUE;
					g_ndpReqNDPE.ucNDPEAttrPresent =
						pNanDebug->
						debug_cmd_data[0];
					DBGLOG(REQ, INFO,
						"NAN_TEST_MODE_CMD_DISABLE_NDPE: fgEnNDPE = %d\n",
						g_ndpReqNDPE.fgEnNDPE);
					break;
				default:
					break;
				}
			} else {
				DBGLOG(REQ, ERROR,
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

#if (CFG_SUPPORT_NAN_CUSTOMIZATION_VERSION == 0)
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

		nanSchedResetCommitedAvailability(prAdapter);

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
					rChnlInfo.rChannel.u4AuxCenterChnl =
					  nanRegGetChannelByOrder(
					    prChnlEntry->op_class,
					    &prChnlEntry
					    ->auxiliary_channel_bitmap);
				}
				rChnlInfo.rChannel.u4Type =
					NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
				rChnlInfo.rChannel.u4OperatingClass =
					prChnlEntry->op_class;
				rChnlInfo.rChannel.u4PrimaryChnl =
					nanRegGetPrimaryChannelByOrder(
						prChnlEntry->op_class,
						&prChnlEntry->op_class_bitmap,
						fgNonContinuousBw,
						prChnlEntry
						->primary_channel_bitmap);

				/* convert to _NAN_AVAIL_ENTRY_CTRL */
				rEntryCtrl.rField.u2Type =
				NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_COMMIT;
				rEntryCtrl.rField.u2Preference =
					prChnlEntry->usage_preference;
				rEntryCtrl.rField.u2Util =
					prChnlEntry->utilization;
				rEntryCtrl.rField.u2RxNss =
					prChnlEntry->rx_nss;
				rEntryCtrl.rField.u2TimeMapAvail =
					((prChnlEntry
					->time_bitmap.time_bitmap_length
					== 0) ? 0 : 1);

				/* convert to au4AvailMap */
				kalMemZero(au4AvailMap, sizeof(au4AvailMap));
				if (rEntryCtrl.rField.u2TimeMapAvail == 0) {
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
				nanSchedConfigCommitedAvailability(
					prAdapter,
					ucMapId,
					rChnlInfo,
					rEntryCtrl,
					au4AvailMap);
			}
		}
		/* Sync setting to firmware */
		nanSchedNegoSyncSchUpdateFsmStep(
			prAdapter, ENUM_NAN_SYNC_SCH_UPDATE_STATE_IDLE);
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

		nanSchedResetPotentialAvailability(prAdapter);

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
				"Band number: %d, chnl entry number: %d\n",
				ucNumBandEntries, ucNumChnlEntries);

			if (ucNumBandEntries) {
				for (ucBandEntryIdx = 0;
					ucBandEntryIdx < ucNumBandEntries;
					ucBandEntryIdx++) {
					ucBandId =
					  prPotentialAvailability
					  ->potential[ucMapIdx]
					  .band_ids[ucBandEntryIdx];

					rChnlInfo.rBand.u4Type =
					  NAN_BAND_CH_ENTRY_LIST_TYPE_BAND;
					rChnlInfo.rBand.u4BandIdMask |=
					  BIT(ucBandId);
				}

				nanSchedConfigPotentialAvailability(
					prAdapter,
					ucMapId,
					rChnlInfo,
					rEntryCtrl,
					au4AvailMap);

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
						rChnlInfo.rChannel
						.u4AuxCenterChnl =
						  nanRegGetChannelByOrder(
						    prChnlEntry->op_class,
						    &prChnlEntry
						    ->auxiliary_channel_bitmap);
					}
					rChnlInfo.rChannel.u4Type =
					  NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL;
					rChnlInfo.rChannel.u4OperatingClass =
						prChnlEntry->op_class;
					rChnlInfo.rChannel.u4PrimaryChnl =
						nanRegGetPrimaryChannelByOrder(
						  prChnlEntry->op_class,
						  &prChnlEntry
						  ->op_class_bitmap,
						  fgNonContinuousBw,
						  prChnlEntry
						  ->primary_channel_bitmap);

					/* convert to _NAN_AVAIL_ENTRY_CTRL*/
					rEntryCtrl.rField.u2Type =
					  NAN_AVAIL_ENTRY_CTRL_AVAIL_TYPE_POTN;
					rEntryCtrl.rField.u2Preference =
					  prChnlEntry->usage_preference;
					rEntryCtrl.rField.u2Util =
					  prChnlEntry->utilization;
					rEntryCtrl.rField.u2RxNss =
					  prChnlEntry->rx_nss;
					rEntryCtrl.rField.u2TimeMapAvail =
					  ((prChnlEntry
					  ->time_bitmap
					  .time_bitmap_length ==
						0) ? 0 : 1);

					/* convert to au4AvailMap */
					if (rEntryCtrl
					.rField.u2TimeMapAvail == 1) {
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

					nanSchedConfigPotentialAvailability(
						prAdapter, ucMapId, rChnlInfo,
						rEntryCtrl, au4AvailMap);

				}
			}
		}

		/* Sync setting to firmware */
		nanSchedCmdUpdatePotentialChnlAvail(prAdapter);
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
				nanSchedConfigNdcAvailability(
					prAdapter, ucMapId, au4AvailMap,
					prNdcParam->ndc_id.octet);
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
#endif /* CFG_SUPPORT_NAN_CUSTOMIZATION_VERSION == 0 */
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

			prNanDfspCfg =
				(struct _NanDfspConfig *)data;
			nicUpdateDfspConfig(prAdapter,
				(struct CMD_DFSP_CONFIG *)prNanDfspCfg);
			return 0;
		}
	case NAN_MSG_ID_UPDATE_CUSTOM_ATTRIBUTE:
	{
		struct NanCustomAttribute *prNanCustomAttr = NULL;

		prNanCustomAttr = (struct NanCustomAttribute *)data;

		prAdapter->rNanCustomAttr.length = prNanCustomAttr->length;
		kalMemCpyS(prAdapter->rNanCustomAttr.data,
				sizeof(prAdapter->rNanCustomAttr.data),
				prNanCustomAttr->data,
				prNanCustomAttr->length);

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

	return cfg80211_vendor_cmd_reply(skb);
}

/* Indication part */
uint32_t
mtk_cfg80211_vendor_event_nan_event_indication(IN struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanEventIndMsg *prNanEventInd;
	struct NAN_DE_EVENT *prDeEvt;
	uint16_t u2EventType;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prDeEvt = (struct NAN_DE_EVENT *) pcuEvtBuf;

	/*Final length includes all TLVs*/
	message_len = sizeof(struct _NanMsgHeader) +
		SIZEOF_TLV_HDR + MAC_ADDR_LEN;

	prNanEventInd = kalMemAlloc(message_len, VIR_MEM_TYPE);
	if (prNanEventInd == NULL) {
		DBGLOG(REQ, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	prNanEventInd->fwHeader.msgVersion = 1;
	prNanEventInd->fwHeader.msgId = NAN_MSG_ID_DE_EVENT_IND;
	prNanEventInd->fwHeader.msgLen = message_len;
	prNanEventInd->fwHeader.handle = 0;
	prNanEventInd->fwHeader.transactionId = 0;

	tlvs = prNanEventInd->ptlv;

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

	/* Add TLV datas */
	tlvs = nanAddTlv(u2EventType, MAC_ADDR_LEN, prDeEvt->addr, tlvs);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanEventInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	if (prDeEvt->ucEventType == NAN_EVENT_ID_STARTED_CLUSTER ||
		prDeEvt->ucEventType == NAN_EVENT_ID_JOINED_CLUSTER)
		g_deEvent = TRUE;

	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanEventInd)
		kalMemFree(prNanEventInd, VIR_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t mtk_cfg80211_vendor_event_nan_disable_indication(
		IN struct ADAPTER *prAdapter, uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanDisableIndMsg *prNanDisableInd;
	struct NAN_DISABLE_EVENT *prDisableEvt;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prDisableEvt = (struct NAN_DISABLE_EVENT *) pcuEvtBuf;

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	/*Final length includes all TLVs*/
	message_len = sizeof(struct _NanMsgHeader) +
			sizeof(prNanDisableInd->reason) +
			sizeof(prNanDisableInd->reserved);

	prNanDisableInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prNanDisableInd) {
		DBGLOG(REQ, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	prNanDisableInd->fwHeader.msgVersion = 1;
	prNanDisableInd->fwHeader.msgId = NAN_MSG_ID_DISABLE_IND;
	prNanDisableInd->fwHeader.msgLen = message_len;
	prNanDisableInd->fwHeader.handle = 0;
	prNanDisableInd->fwHeader.transactionId = 0;

	prNanDisableInd->reason = 0;

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
		message_len, prNanDisableInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	g_enableNAN = TRUE;

	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanDisableInd)
		kalMemFree(prNanDisableInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

/* Indication part */
uint32_t
mtk_cfg80211_vendor_event_nan_replied_indication(IN struct ADAPTER *prAdapter,
						 uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NAN_REPLIED_EVENT *prRepliedEvt = NULL;
	struct NanPublishRepliedIndMsg *prNanPubRepliedInd;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prRepliedEvt = (struct NAN_REPLIED_EVENT *)pcuEvtBuf;

	/* Final length includes all TLVs */
	message_len = sizeof(struct _NanMsgHeader) +
		      sizeof(struct _NanPublishRepliedIndParams) +
		      ((SIZEOF_TLV_HDR) + MAC_ADDR_LEN) +
		      ((SIZEOF_TLV_HDR) + sizeof(prRepliedEvt->ucRssi_value));

	prNanPubRepliedInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (prNanPubRepliedInd == NULL) {
		DBGLOG(REQ, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prNanPubRepliedInd, message_len);

	DBGLOG(NAN, INFO, "[%s] message_len : %d\n", __func__, message_len);
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
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanPubRepliedInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanPubRepliedInd)
		kalMemFree(prNanPubRepliedInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_match_indication(IN struct ADAPTER *prAdapter,
					       uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NAN_DISCOVERY_EVENT *prDiscEvt;
	struct NanMatchIndMsg *prNanMatchInd;
	struct NanSdeaCtrlParams peer_sdea_params;
	struct NanFWSdeaCtrlParams nanPeerSdeaCtrlarms;
	size_t message_len = 0;
	uint8_t *tlvs = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
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

	prNanMatchInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prNanMatchInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
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
	DBGLOG(NAN, INFO, "[%s] :NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO %u\n",
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

	tlvs = nanAddTlv(NAN_TLV_TYPE_SDEA_CTRL_PARAMS,
			 sizeof(struct NanFWSdeaCtrlParams),
			 (u8 *)&nanPeerSdeaCtrlarms, tlvs);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanMatchInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanMatchInd)
		kalMemFree(prNanMatchInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_publish_terminate(IN struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NAN_PUBLISH_TERMINATE_EVENT *prPubTerEvt;
	struct NanPublishTerminatedIndMsg nanPubTerInd;
	size_t message_len = 0;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prPubTerEvt = (struct NAN_PUBLISH_TERMINATE_EVENT *)pcuEvtBuf;

	kalMemZero(&nanPubTerInd, sizeof(struct NanPublishTerminatedIndMsg));

	message_len = sizeof(struct NanPublishTerminatedIndMsg);

	nanPubTerInd.fwHeader.msgVersion = 1;
	nanPubTerInd.fwHeader.msgId = NAN_MSG_ID_PUBLISH_TERMINATED_IND;
	nanPubTerInd.fwHeader.msgLen = message_len;
	nanPubTerInd.fwHeader.handle = prPubTerEvt->u2Pubid;
	/* Indication doesn't have transactionId, don't care */
	nanPubTerInd.fwHeader.transactionId = 0;
	/* For all user should be success. */
	nanPubTerInd.reason = prPubTerEvt->ucReasonCode;

	DBGLOG(NAN, INFO, "[%s] Cancel Pub ID = %d\n",
	       nanPubTerInd.fwHeader.handle);

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return WLAN_STATUS_RESOURCES;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     &nanPubTerInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return WLAN_STATUS_FAILURE;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
mtk_cfg80211_vendor_event_nan_subscribe_terminate(IN struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NAN_SUBSCRIBE_TERMINATE_EVENT *prSubTerEvt;
	struct NanSubscribeTerminatedIndMsg nanSubTerInd;
	size_t message_len = 0;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prSubTerEvt = (struct NAN_SUBSCRIBE_TERMINATE_EVENT *)pcuEvtBuf;

	kalMemZero(&nanSubTerInd, sizeof(struct NanSubscribeTerminatedIndMsg));

	message_len = sizeof(struct NanSubscribeTerminatedIndMsg);

	nanSubTerInd.fwHeader.msgVersion = 1;
	nanSubTerInd.fwHeader.msgId = NAN_MSG_ID_SUBSCRIBE_TERMINATED_IND;
	nanSubTerInd.fwHeader.msgLen = message_len;
	nanSubTerInd.fwHeader.handle = prSubTerEvt->u2Subid;
	/* Indication doesn't have transactionId, don't care */
	nanSubTerInd.fwHeader.transactionId = 0;
	/* For all user should be success. */
	nanSubTerInd.reason = prSubTerEvt->ucReasonCode;

	/* Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return WLAN_STATUS_RESOURCES;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     &nanSubTerInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return WLAN_STATUS_FAILURE;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);

	return WLAN_STATUS_SUCCESS;
}

uint32_t
mtk_cfg80211_vendor_event_nan_followup_indication(IN struct ADAPTER *prAdapter,
						  uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanFollowupIndMsg *prNanFollowupInd;
	struct NAN_FOLLOW_UP_EVENT *prFollowupEvt;
	uint8_t *tlvs = NULL;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prFollowupEvt = (struct NAN_FOLLOW_UP_EVENT *)pcuEvtBuf;

	message_len =
		sizeof(struct _NanMsgHeader) +
		sizeof(struct _NanFollowupIndParams) +
		(SIZEOF_TLV_HDR + MAC_ADDR_LEN) +
		(SIZEOF_TLV_HDR + prFollowupEvt->service_specific_info_len) +
		(SIZEOF_TLV_HDR +
			prFollowupEvt->sdea_service_specific_info_len);

	prNanFollowupInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prNanFollowupInd) {
		DBGLOG(REQ, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}
	kalMemZero(prNanFollowupInd, message_len);

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

	DBGLOG(NAN, LOUD, "[%s] matchHandle: %d, window:%d\n", __func__,
	       prNanFollowupInd->followupIndParams.matchHandle,
	       prNanFollowupInd->followupIndParams.window);

	tlvs = prNanFollowupInd->ptlv;
	/* Add TLV datas */
	tlvs = nanAddTlv(NAN_TLV_TYPE_MAC_ADDRESS, MAC_ADDR_LEN,
			 prFollowupEvt->addr, tlvs);

	tlvs = nanAddTlv(NAN_TLV_TYPE_SERVICE_SPECIFIC_INFO,
			 prFollowupEvt->service_specific_info_len,
			 prFollowupEvt->service_specific_info, tlvs);

	tlvs = nanAddTlv(NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO,
			 prFollowupEvt->sdea_service_specific_info_len,
			 prFollowupEvt->sdea_service_specific_info, tlvs);

	/* Ranging report
	 * To be implement. NAN_TLV_TYPE_SDEA_SERVICE_SPECIFIC_INFO
	 */

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prNanFollowupInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanFollowupInd)
		kalMemFree(prNanFollowupInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_seldflwup_indication(
		IN struct ADAPTER *prAdapter,
		uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanFollowupIndMsg *prNanFollowupInd;
	struct NAN_FOLLOW_UP_EVENT *prFollowupEvt;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
				->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prFollowupEvt = (struct NAN_FOLLOW_UP_EVENT *) pcuEvtBuf;

	message_len = sizeof(struct _NanMsgHeader) +
				sizeof(struct _NanFollowupIndParams);

	prNanFollowupInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prNanFollowupInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prNanFollowupInd, message_len);

	prNanFollowupInd->fwHeader.msgVersion = 1;
	prNanFollowupInd->fwHeader.msgId =
			NAN_MSG_ID_SELF_TRANSMIT_FOLLOWUP_IND;
	prNanFollowupInd->fwHeader.msgLen = message_len;
	prNanFollowupInd->fwHeader.handle = prFollowupEvt->publish_subscribe_id;
	/* Indication doesn't have transition ID */
	prNanFollowupInd->fwHeader.transactionId = u16NanFollowupID;

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
				message_len, prNanFollowupInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prNanFollowupInd)
		kalMemFree(prNanFollowupInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_changed_indication(
		IN struct ADAPTER *prAdapter)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	size_t message_len = 0;
	struct _NanMsgHeader *prMsgHdr;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len =
		sizeof(struct _NanMsgHeader);

	prMsgHdr = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsgHdr) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsgHdr, message_len);

	prMsgHdr->msgVersion = 1;
	prMsgHdr->msgId = NAN_MSG_ID_INFRA_CHANGED_IND;
	prMsgHdr->msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsgHdr->transactionId = 0;

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsgHdr) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsgHdr)
		kalMemFree(prMsgHdr, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_dfsp_csa(IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanDfspCsaIndMsg *prDfspCsaInd;
	struct DFSP_EVENT_CSA_T *prDfspCsaEvt;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
				->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prDfspCsaEvt = (struct DFSP_EVENT_CSA_T *)pcuEvtBuf;

	message_len = sizeof(struct NanDfspCsaIndMsg) +
				sizeof(struct DFSP_COMBINED_EVENT_DATA_T *);

	prDfspCsaInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prDfspCsaInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prDfspCsaInd, message_len);

	prDfspCsaInd->fwHeader.msgVersion = 1;
	prDfspCsaInd->fwHeader.msgId =
		NAN_MSG_ID_UPDATE_DFSP_CSA_IND;
	prDfspCsaInd->fwHeader.msgLen = message_len;
	prDfspCsaInd->fwHeader.handle = 0;
	/* Indication doesn't have transition ID */
	prDfspCsaInd->fwHeader.transactionId = 0;

	/* CSA body */
	prDfspCsaInd->csaInd.flags = prDfspCsaEvt->flags;
	prDfspCsaInd->csaInd.length = prDfspCsaEvt->length;
	kalMemCopy(prDfspCsaInd->csaInd.dfs_tlv_data,
		prDfspCsaEvt->dfs_tlv_data,
		sizeof(struct DFSP_COMBINED_EVENT_DATA_T *));

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
				message_len, prDfspCsaInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prDfspCsaInd)
		kalMemFree(prDfspCsaInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_dfsp_csa_complete(
		IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanDfspCsaCompleteIndMsg *prDfspCsaCompleteInd;
	struct DFSP_EVENT_CSA_COMPLETE_T *prDfspCsaCompleteEvt;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
				->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prDfspCsaCompleteEvt = (struct DFSP_EVENT_CSA_COMPLETE_T *)pcuEvtBuf;

	message_len = sizeof(struct NanDfspCsaCompleteIndMsg);

	prDfspCsaCompleteInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prDfspCsaCompleteInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prDfspCsaCompleteInd, message_len);

	prDfspCsaCompleteInd->fwHeader.msgVersion = 1;
	prDfspCsaCompleteInd->fwHeader.msgId =
		NAN_MSG_ID_UPDATE_DFSP_CSA_COMPLETE_IND;
	prDfspCsaCompleteInd->fwHeader.msgLen = message_len;
	prDfspCsaCompleteInd->fwHeader.handle = 0;
	/* Indication doesn't have transition ID */
	prDfspCsaCompleteInd->fwHeader.transactionId = 0;

	/* CSA_Complete body */
	prDfspCsaCompleteInd->csaCompleteInd.ucNewChannelNum =
	prDfspCsaCompleteEvt->ucNewChannelNum;

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
				message_len, prDfspCsaCompleteInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prDfspCsaCompleteInd)
		kalMemFree(prDfspCsaCompleteInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_dfsp_suspend_resume(
		IN struct ADAPTER *prAdapter,
		IN uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct NanDfspSusResIndMsg *prDfspSusResInd;
	struct DFSP_SUSPEND_RESUME_T *prDfspSusResEvt;
	size_t message_len = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
				->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prDfspSusResEvt = (struct DFSP_SUSPEND_RESUME_T *)pcuEvtBuf;

	message_len = sizeof(struct NanDfspSusResIndMsg);

	prDfspSusResInd = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prDfspSusResInd) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prDfspSusResInd, message_len);

	prDfspSusResInd->fwHeader.msgVersion = 1;
	prDfspSusResInd->fwHeader.msgId =
		NAN_MSG_ID_UPDATE_DFSP_SUSPEND_RESUME_IND;
	prDfspSusResInd->fwHeader.msgLen = message_len;
	prDfspSusResInd->fwHeader.handle = 0;
	/* Indication doesn't have transition ID */
	prDfspSusResInd->fwHeader.transactionId = 0;

	/* Suspend / Resume body */
	prDfspSusResInd->susResInd.flags =
		prDfspSusResEvt->flags;
	prDfspSusResInd->susResInd.suspended =
		prDfspSusResEvt->suspended;
	prDfspSusResInd->susResInd.resumed =
		prDfspSusResEvt->resumed;

	/*  Fill skb and send to kernel by nl80211*/
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					message_len + NLMSG_HDRLEN,
					WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN,
				message_len, prDfspSusResInd) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prDfspSusResInd)
		kalMemFree(prDfspSusResInd, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_assoc_st_ind(
		struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand,
		uint8_t ucChannelNum,
		uint8_t ucChnlBw,
		enum ENUM_CHNL_EXT eSco)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraAssocStartIndMsg *prMsg = NULL;
	struct RF_CHANNEL_INFO rChannel = {0};
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(INIT, INFO,
		"eBand: %u, ucChannelNum: %u, ucChnlBw: %u, eSco: %u\n",
		eBand, ucChannelNum, ucChnlBw, eSco);
	message_len =
		sizeof(struct NanInfraAssocStartIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_ASSOC_START_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	rChannel.eBand = eBand;
	rChannel.ucChannelNum = ucChannelNum;
	rChannel.ucChnlBw = ucChnlBw;

	prMsg->channel = sunrise_to_wfpal_channel(rChannel, eSco);

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_auth_rx_indication(
	struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraAuthRxIndMsg *prMsg = NULL;
	struct WLAN_AUTH_FRAME *prAuthFrame = NULL;
	struct AIS_FSM_INFO *prAisFsmInfo = NULL;
	struct PMKID_ENTRY *prPmkidEntry = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!prSwRfb) {
		DBGLOG(REQ, ERROR, "prSwRfb is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prAuthFrame = (struct WLAN_AUTH_FRAME *) prSwRfb->pvHeader;

	message_len = sizeof(struct NanInfraAuthRxIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);

	if (!prMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_AUTH_RX_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	prMsg->status = (prAuthFrame->aucAuthData[3] << 8) +
					prAuthFrame->aucAuthData[2];
#else
	prMsg->status = prAuthFrame->u2StatusCode;
#endif
	prMsg->reason = 0xFF; /* IEEE_REASON_RESERVED */

	kalMemCopy(prMsg->peer_mac, prAuthFrame->aucSrcAddr, MAC_ADDR_LEN);

	/* get pmkid */
	prAisFsmInfo = aisGetAisFsmInfo(prAdapter, AIS_DEFAULT_INDEX);
	prPmkidEntry = rsnSearchPmkidEntry(prAdapter, prAuthFrame->aucBSSID,
				AIS_DEFAULT_INDEX);

	prMsg->pmk_len = 0; /* We don't have pmk information.*/

	if (prPmkidEntry) {
		kalMemCopy(prMsg->pmkid,
			prPmkidEntry->rBssidInfo.arPMKID,
			IW_PMKID_LEN);
		prMsg->pmkid_len = IW_PMKID_LEN;
	}

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_assoc_done_indication(
		struct ADAPTER *prAdapter,
		uint32_t rJoinStatus,
		struct STA_RECORD *prStaRec)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraAssocDoneIndMsg *prMsg = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	uint16_t u2StatusCode = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!prStaRec) {
		DBGLOG(REQ, ERROR, "prStaRec is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanInfraAssocDoneIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_ASSOC_DONE_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, prStaRec->ucBssIndex);

	kalMemCopy(prMsg->join_address, prStaRec->aucMacAddr, MAC_ADDR_LEN);
	prMsg->return_val = 0; /* Not Sure */
	prMsg->extension_return_val = 0; /* Not Sure */
	kalMemCopy(prMsg->substate_info[0].bssid, prBssInfo->aucBSSID,
		MAC_ADDR_LEN);

	if (rJoinStatus == WLAN_STATUS_SUCCESS) {
		prMsg->ieee_status = STATUS_CODE_SUCCESSFUL;

	} else if (rJoinStatus == WLAN_STATUS_FAILURE) {
		if (prStaRec->u2StatusCode)
			u2StatusCode = prStaRec->u2StatusCode;
		else
			u2StatusCode =
				(uint16_t)STATUS_CODE_UNSPECIFIED_FAILURE;

		prMsg->ieee_status = u2StatusCode;

		if (prStaRec->eAuthAssocSent <= AA_SENT_AUTH3) {
			prMsg->substate_info[0].flags |= BIT(0);
			prMsg->substate_info[0].auth_status = u2StatusCode;
		} else {
			prMsg->substate_info[0].flags |= BIT(1);
			prMsg->substate_info[0].assoc_status = u2StatusCode;
		}
	}

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_assoc_rx_ind(
		struct ADAPTER *prAdapter,
		uint8_t *buf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraAssocReceivedIndMsg *prMsg = NULL;
	struct WLAN_ASSOC_RSP_FRAME *prAssocRspFrame = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!buf) {
		DBGLOG(REQ, ERROR, "buf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanInfraAssocReceivedIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_ASSOC_RECEIVED_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	prAssocRspFrame = (struct WLAN_ASSOC_RSP_FRAME *) buf;
	prMsg->status = (uint32_t)prAssocRspFrame->u2StatusCode;
	 /* Reserved */
	prMsg->reason = REASON_CODE_RESERVED;

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_assoc_ready_indication(
		struct ADAPTER *prAdapter)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	struct net_device *prDev = NULL;
	struct in_device *prInDev = NULL;
	struct in_ifaddr *prIfa = NULL;
	size_t message_len = 0;
	struct NanInfraAssocReadyIndMsg *prMsg = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prDev = wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX);
	/* 4 <1> Sanity check of netDevice */
	if (!prDev) {
		DBGLOG(INIT, INFO, "prDev is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = prDev->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanInfraAssocReceivedIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_ASSOC_READY_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	prMsg->status = 1;
	prMsg->is_ipv6 = 0;

	/* Get IPv4 address */
	prInDev = (struct in_device *)(prDev->ip_ptr);

	if (!prInDev || !(prInDev->ifa_list)) {
		DBGLOG(REQ, WARN, "Cannot get ip address\n");
	} else {
		prIfa = prInDev->ifa_list;
		kalMemCopy(prMsg->addressv4, &prIfa->ifa_local, IPV4_ADDR_LEN);
	}

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_scan_start_indication(
		struct ADAPTER *prAdapter,
		struct cfg80211_scan_request *request)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraScanStartIndMsg *prMsg = NULL;
	uint32_t u4channel = 0, u4Idx = 0;
	size_t i = 0, cnum = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!request) {
		DBGLOG(REQ, ERROR, "request is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanInfraScanStartIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_SCAN_START_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	cnum = request->n_channels;

	if (cnum > WFPAL_MAX_CHANNELS)
		cnum = WFPAL_MAX_CHANNELS;

	for (i = 0; i < cnum; i++) {
		u4channel = nicFreq2ChannelNum(request->channels[i]->
							center_freq * 1000);
		switch ((request->channels[i])->band) {
		case KAL_BAND_2GHZ:
			prMsg->num_of_24G_channels += 1;
			prMsg->channel_list[u4Idx] = u4channel;
			u4Idx++;
			break;
		case KAL_BAND_5GHZ:
			prMsg->num_of_5G_channels += 1;
			prMsg->channel_list[u4Idx] = u4channel;
			u4Idx++;
			break;
#if (CFG_SUPPORT_WIFI_6G == 1)
		case KAL_BAND_6GHZ: /* Not support yet*/
			break;
#endif
		default:
			DBGLOG(REQ, WARN, "UNKNOWN Band %d(chnl=%u)\n",
			       request->channels[i]->band,
			       u4channel);
			break;
		}
	}


	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_infra_scan_complete_indication(
		struct ADAPTER *prAdapter,
		uint8_t ucStatus)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanInfraScanCompleteIndMsg *prMsg = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanInfraScanCompleteIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_INFRA_SCAN_COMPLETE_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	prMsg->status = ucStatus;

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_report_dw_start(struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanDwStartIndMsg *prMsg = NULL;
	struct NAN_EVENT_REPORT_DW_T *prNanReportDwEvt = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanDwStartIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);
	if (!prMsg) {
		DBGLOG(NAN, ERROR, "Allocate failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_DW_START_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	prNanReportDwEvt = (struct NAN_EVENT_REPORT_DW_T *)pcuEvtBuf;

	if (prNanReportDwEvt->channel == 6) {
		prMsg->channel.channel = 6;
		prMsg->channel.flags = 0xA;
	} else if (prNanReportDwEvt->channel == 149) {
		prMsg->channel.channel = 149;
		prMsg->channel.flags = 0x410;
	}

	prMsg->expected_tsf_l = prNanReportDwEvt->expected_tsf_l;
	prMsg->expected_tsf_h = prNanReportDwEvt->expected_tsf_h;
	prMsg->actual_tsf_l = prNanReportDwEvt->actual_tsf_l;
	prMsg->actual_tsf_h = prNanReportDwEvt->actual_tsf_h;

	prMsg->dw_num = (uint8_t)prNanReportDwEvt->dw_num;

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_report_dw_end(struct ADAPTER *prAdapter,
						uint8_t *pcuEvtBuf)
{
	return WLAN_STATUS_SUCCESS;
}

uint32_t
mtk_cfg80211_vendor_event_nan_role_changed_received(struct ADAPTER *prAdapter,
							uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanRoleChangedIndMsg *prMsg = NULL;
	struct NAN_EVENT_DEVICE_ROLE_T *prNanRoleRecvEvt = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(*prMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);

	if (!prMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_ROLE_CHANGED_IND;
	prMsg->fwHeader.msgLen = message_len;

	prNanRoleRecvEvt = (struct NAN_EVENT_DEVICE_ROLE_T *)pcuEvtBuf;

	DBGLOG(NAN, LOUD, "role:%u\n", prNanRoleRecvEvt->ucNanDeviceRole);
	DBGLOG(NAN, LOUD, "hop_count:%u\n", prNanRoleRecvEvt->ucHopCount);

	prMsg->role = prNanRoleRecvEvt->ucNanDeviceRole;
	prMsg->hopcount = prNanRoleRecvEvt->ucHopCount;


	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_oob_af_rx(struct ADAPTER *prAdapter,
					uint8_t *ucDestAddr,
					uint8_t *ucSrcAddr,
					uint8_t *ucBssid,
					struct RF_CHANNEL_INFO *prSunriseChnl,
					uint8_t ucRssi,
					uint16_t u2DataLen,
					uint8_t *ucData)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanOobAfRxIndMsg *prMsg = NULL;
	struct RF_CHANNEL_INFO sunriseChannel = {0};
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanOobAfRxIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);

	if (!prMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_OOB_AF_RX_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	sunriseChannel.ucChannelNum = prSunriseChnl->ucChannelNum;
	sunriseChannel.eBand = prSunriseChnl->eBand;
	sunriseChannel.ucChnlBw = prSunriseChnl->ucChnlBw;

	kalMemCopy(prMsg->dst_addr, ucDestAddr, MAC_ADDR_LEN);
	kalMemCopy(prMsg->src_addr, ucSrcAddr, MAC_ADDR_LEN);
	kalMemCopy(prMsg->bssid, ucBssid, MAC_ADDR_LEN);

	prMsg->channel = sunrise_to_wfpal_channel(sunriseChannel, CHNL_EXT_SCN);
	prMsg->rx_rssi = ucRssi;
	prMsg->length = u2DataLen;

	kalMemCopy(prMsg->data, ucData, prMsg->length);

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_chip_reset(struct ADAPTER *prAdapter,
					uint8_t fgIsReset)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct net_device *prNetDev = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanChipRstIndMsg *prMsg = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prNetDev = wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX);

	if (!prNetDev) {
		DBGLOG(REQ, ERROR, "Cannot get netdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = prNetDev->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	message_len = sizeof(struct NanChipRstIndMsg);

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);

	if (!prMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_CHIP_RST_IND;
	prMsg->fwHeader.msgLen = message_len;

	/* Indication doesn't have transition ID */
	prMsg->fwHeader.transactionId = 0;

	if (fgIsReset) {
		prMsg->state = 1; /* IN_PROGRESS */
	} else {
		g_enableNAN = TRUE;
		prMsg->state = 2; /* COMPLETED */
	}

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_report_beacon(struct ADAPTER *prAdapter,
							uint8_t *pcuEvtBuf)
{
	struct sk_buff *skb = NULL;
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	size_t message_len = 0;
	struct NanBcnRxIndMsg *prMsg = NULL;
	struct NAN_REPORT_BEACON_EVENT *prNanBcnFrameEvt = NULL;
	struct RF_CHANNEL_INFO sunriseChannel = {0};
	uint32_t u4PhyRateIn100Kbps = 0;
	struct WLAN_BEACON_FRAME *prWlanBeaconFrame = NULL;
	uint32_t rStatus = WLAN_STATUS_FAILURE;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	if (!pcuEvtBuf) {
		DBGLOG(REQ, ERROR, "pcuEvtBuf is null\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = (wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX))
		       ->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prNanBcnFrameEvt = (struct NAN_REPORT_BEACON_EVENT *)pcuEvtBuf;
	prWlanBeaconFrame =
		(struct WLAN_BEACON_FRAME *) prNanBcnFrameEvt->aucBeaconFrame;

	DBGLOG(NAN, LOUD, NAN_EVT_BCN_RX_DBG_STRING1,
		prNanBcnFrameEvt->eRfBand,
		prNanBcnFrameEvt->i4Rssi,
		prNanBcnFrameEvt->u2BeaconLength,
		prNanBcnFrameEvt->u2TxMode,
		prNanBcnFrameEvt->ucRate,
		prNanBcnFrameEvt->ucHwChnl,
		prNanBcnFrameEvt->ucBw,
		prNanBcnFrameEvt->au4LocalTsf[0],
		prNanBcnFrameEvt->au4LocalTsf[1]);


	message_len = sizeof(struct NanBcnRxIndMsg) +
		prNanBcnFrameEvt->u2BeaconLength;

	prMsg = kalMemAlloc(message_len, PHY_MEM_TYPE);

	if (!prMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prMsg, message_len);

	prMsg->fwHeader.msgVersion = 1;
	prMsg->fwHeader.msgId = NAN_MSG_ID_BEACON_RX_IND;
	prMsg->fwHeader.msgLen = message_len;

	prMsg->rssi = prNanBcnFrameEvt->i4Rssi;

	sunriseChannel.ucChannelNum = prNanBcnFrameEvt->ucHwChnl;
	sunriseChannel.eBand =
		SCN_GET_EBAND_BY_CH_NUM(prNanBcnFrameEvt->ucHwChnl);
	sunriseChannel.ucChnlBw = prNanBcnFrameEvt->ucBw;
	prMsg->channel = sunrise_to_wfpal_channel(sunriseChannel, CHNL_EXT_SCN);

	/* rate */
	if (((prNanBcnFrameEvt->u2TxMode << RATE_TX_MODE_OFFSET) ==
		TX_MODE_OFDM) || (
		(prNanBcnFrameEvt->u2TxMode << RATE_TX_MODE_OFFSET) ==
		TX_MODE_CCK)) {
		/* The hardware rate is defined in units of 500 Kbps.
		 * To convert it to units of 100 Kbps, we multiply by 5.
		 */
		u4PhyRateIn100Kbps = (nicGetHwRateByPhyRate(
				prNanBcnFrameEvt->ucRate & BITS(0, 3))) * 5;
	}
	prMsg->rate = (uint8_t)(u4PhyRateIn100Kbps/10);

	/* TSF */
	prMsg->local_tsf_offset_h = prNanBcnFrameEvt->au4LocalTsf[1];
	prMsg->local_tsf_offset_l = prNanBcnFrameEvt->au4LocalTsf[0];

	prMsg->length = prNanBcnFrameEvt->u2BeaconLength;
	kalMemCopy(prMsg->frame,
		prWlanBeaconFrame,
		prNanBcnFrameEvt->u2BeaconLength);


	DBGLOG(NAN, LOUD, NAN_EVT_BCN_RX_DBG_STRING2,
		MAC2STR(prWlanBeaconFrame->aucBSSID),
		MAC2STR(prWlanBeaconFrame->aucSrcAddr),
		prMsg->rssi,
		sunriseChannel.ucChannelNum,
		prMsg->rate,
		prMsg->local_tsf_offset_l);

	/*  Fill skb and send to kernel by nl80211 */
	skb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  message_len + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus =  WLAN_STATUS_RESOURCES;
		goto out;
	}
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NAN, message_len,
			     prMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		rStatus =  WLAN_STATUS_FAILURE;
		goto out;
	}
	cfg80211_vendor_event(skb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prMsg)
		kalMemFree(prMsg, PHY_MEM_TYPE, message_len);

	return rStatus;
}

uint32_t
mtk_cfg80211_vendor_event_nan_country_chng_ind(struct ADAPTER *prAdapter)
{
	struct sk_buff *prSkb;
	struct wiphy *prWiphy;
	struct wireless_dev *prWdev;
	struct net_device *prNetDev;
	size_t szMsgLen = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4Band_idx;
	uint32_t u4Ch_idx;
	uint32_t u4Ch_count;
	uint16_t u2Flags;
	struct ieee80211_supported_band *prSband;
	struct ieee80211_channel *prChan;
	struct NanCountryCodeChangedIndMsg *prCountryCodeChangedIndMsg = NULL;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdpter is NULL\n");
		return WLAN_STATUS_FAILURE;
	}

	prNetDev = wlanGetNetDev(prAdapter->prGlueInfo, AIS_DEFAULT_INDEX);

	if (!prNetDev) {
		DBGLOG(REQ, ERROR, "Cannot get netdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	prWiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prWdev = prNetDev->ieee80211_ptr;

	if (!prWiphy || !prWdev) {
		DBGLOG(REQ, ERROR, "Cannot get wiphy/wdev.\n");
		return WLAN_STATUS_FAILURE;
	}

	DBGLOG(NAN, INFO, "IN\n");

	szMsgLen = sizeof(struct NanCountryCodeChangedIndMsg);

	prCountryCodeChangedIndMsg = kalMemAlloc(szMsgLen, PHY_MEM_TYPE);

	if (!prCountryCodeChangedIndMsg) {
		DBGLOG(REQ, ERROR, "Allocate Msg failed\n");
		return WLAN_STATUS_RESOURCES;
	}

	kalMemZero(prCountryCodeChangedIndMsg, szMsgLen);

	u4Ch_count = 0;
	for (u4Band_idx = 0; u4Band_idx <= KAL_BAND_5GHZ; u4Band_idx++) {
		prSband = prWiphy->bands[u4Band_idx];
		if (!prSband)
			continue;

		for (u4Ch_idx = 0; u4Ch_idx < prSband->n_channels; u4Ch_idx++) {
			prChan = &prSband->channels[u4Ch_idx];

			if (prChan->flags & IEEE80211_CHAN_DISABLED) {
				DBGLOG(NAN, INFO,
				       "Disabled channels[%d][%d]: ch%d (freq = %d) flags=0x%x\n",
				    u4Band_idx, u4Ch_idx, prChan->hw_value,
				    prChan->center_freq, prChan->flags);
				continue;
			}

			/* Allowable channel */
			if (u4Ch_count == NAN_CHANNEL_REPORT_MAX_SIZE) {
				DBGLOG(NAN, ERROR,
				       "%s(): no buffer to store channel information.\n",
				       __func__);
				break;
			}

			DBGLOG(NAN, INFO,
			       "channels[%d][%d]: ch%d (freq = %d) flgs=0x%x\n",
				u4Band_idx, u4Ch_idx, prChan->hw_value,
				prChan->center_freq, prChan->flags);

			prCountryCodeChangedIndMsg
				->channel[u4Ch_count].channel
				= prChan->hw_value;

			if (u4Band_idx == KAL_BAND_5GHZ) {
				u2Flags = NAN_C_FLAG_5GHZ | NAN_C_FLAG_20MHZ;
				if (!(prChan->flags & IEEE80211_CHAN_NO_80MHZ))
					u2Flags |= NAN_C_FLAG_80MHZ;
				if (!(
					(prChan->flags
						& IEEE80211_CHAN_NO_HT40MINUS)
					&&
					(prChan->flags
						& IEEE80211_CHAN_NO_HT40PLUS)
					)
				)
					u2Flags |= NAN_C_FLAG_40MHZ;
				if ((prChan->flags & IEEE80211_CHAN_RADAR))
					u2Flags |= NAN_C_FLAG_DFS;
				if ((prChan->flags & IEEE80211_CHAN_NO_IR))
					u2Flags |= NAN_C_PASSIVE_MODE;

				prCountryCodeChangedIndMsg
					->channel[u4Ch_count].flags
					= u2Flags;
			} else {
				u2Flags = NAN_C_FLAG_2GHZ | NAN_C_FLAG_20MHZ;
				if (!(
					(prChan->flags
						& IEEE80211_CHAN_NO_HT40MINUS)
					&&
					(prChan->flags
						& IEEE80211_CHAN_NO_HT40PLUS)
					)
				)
					u2Flags |= NAN_C_FLAG_40MHZ;

				prCountryCodeChangedIndMsg
					->channel[u4Ch_count].flags
					= u2Flags;
			}

			u4Ch_count += 1;
		}

	}

	prCountryCodeChangedIndMsg->fwHeader.msgVersion = 1;
	prCountryCodeChangedIndMsg->fwHeader.msgId
		= NAN_MSG_ID_COUNTRY_CODE_CHANGED;
	prCountryCodeChangedIndMsg->fwHeader.msgLen = szMsgLen;

	/* Indication doesn't have transition ID */
	prCountryCodeChangedIndMsg->fwHeader.transactionId = 0;

	prCountryCodeChangedIndMsg->channel_num = u4Ch_count;
	rlmDomainU32ToAlpha(rlmDomainGetCountryCode(prAdapter)
		, prCountryCodeChangedIndMsg->countryCode);
	DBGLOG(NAN, INFO,
			"Set country code [%c%c], Total CH[%d]\n"
			, prCountryCodeChangedIndMsg->countryCode[0]
			, prCountryCodeChangedIndMsg->countryCode[1]
			, prCountryCodeChangedIndMsg->channel_num);

	/*  Fill skb and send to kernel by nl80211 */
	prSkb = kalCfg80211VendorEventAlloc(prWiphy, prWdev,
					  szMsgLen + NLMSG_HDRLEN,
					  WIFI_EVENT_SUBCMD_NAN, GFP_KERNEL);

	if (!prSkb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		rStatus = WLAN_STATUS_RESOURCES;
		goto out;
	}

	if (unlikely(nla_put(prSkb, MTK_WLAN_VENDOR_ATTR_NAN, szMsgLen,
			     prCountryCodeChangedIndMsg) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(prSkb);
		rStatus = WLAN_STATUS_FAILURE;
		goto out;
	}

	cfg80211_vendor_event(prSkb, GFP_KERNEL);
	rStatus = WLAN_STATUS_SUCCESS;
out:
	if (prCountryCodeChangedIndMsg)
		kalMemFree(prCountryCodeChangedIndMsg, PHY_MEM_TYPE, szMsgLen);

	return rStatus;
}
