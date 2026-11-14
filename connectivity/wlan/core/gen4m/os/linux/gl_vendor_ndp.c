// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * gl_vendor_ndp.c
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
#include "gl_vendor_ndp.h"
#include "debug.h"
#include "gl_cfg80211.h"
#include "gl_os.h"
#include "gl_vendor.h"
#include "gl_wext.h"
#include "nan_data_engine.h"
#include "nan_sec.h"
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
uint8_t g_InitiatorMacAddr[6];

const struct nla_policy
	mtk_wlan_vendor_ndp_policy[MTK_WLAN_VENDOR_ATTR_NDP_PARAMS_MAX + 1] = {
			[MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID] = {
				.type = NLA_U16 },
			[MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR] = {
				.type = NLA_NUL_STRING,
				.len = IFNAMSIZ - 1 },
			[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_INSTANCE_ID] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_CHANNEL] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR] = {
				.type = NLA_BINARY,
				.len = MAC_ADDR_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_SECURITY] = {
				.type = NLA_NESTED },
			[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO] = {
				.type = NLA_BINARY,
				.len = NDP_APP_INFO_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_RESPONSE_CODE] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_NDI_MAC_ADDR] = {
				.type = NLA_BINARY,
				.len = MAC_ADDR_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY] = {
				.type = NLA_BINARY,
				.len = NDP_NUM_INSTANCE_ID },
			[MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_CHANNEL_CONFIG] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_PMK] = {
				.type = NLA_BINARY,
				.len = NDP_PMK_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_SCID] = {
				.type = NLA_BINARY,
				.len = NDP_SCID_BUF_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_CSID] = {
				.type = NLA_U32 },
			[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE] = {
				.type = NLA_BINARY,
				.len = NAN_PASSPHRASE_MAX_LEN },
			[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_NAME] = {
				.type = NLA_BINARY,
				.len = NAN_MAX_SERVICE_NAME_LEN },
	};

#define NAN_MAX_CHANNEL_INFO_SUPPORTED (4)

/* NAN Channel Info */
struct NanChannelInfo {
	u32 channel;
	u32 bandwidth;
	u32 nss;
};

void
nanGetChannelInfo(
	struct ADAPTER *ad,
	struct _NAN_NDP_INSTANCE_T *prNDP,
	struct NanChannelInfo *info,
	uint32_t *num_info)
{
	uint32_t u4Idx = 0, u4Idx1 = 0, u4Idx2 = 0;
	uint32_t i = 0;
	struct _NAN_NDL_INSTANCE_T *prNDL = NULL;
	struct _NAN_PEER_SCH_DESC_T *p = NULL;
	struct _NAN_AVAILABILITY_DB_T *d = NULL;
	struct _NAN_AVAILABILITY_TIMELINE_T *t = NULL;
	union _NAN_BAND_CHNL_CTRL chctrl;
	uint32_t pch = 0;
	uint32_t opc = 0;

	if (!prNDP)
		return;

	prNDL =
		&ad->rDataPathInfo.arNDL[prNDP->ucNdlIndex];
	if (!prNDL)
		return;

	p = nanSchedSearchPeerSchDescByNmi(
		ad,
		prNDL->aucPeerMacAddr);
	if (!p) {
		DBGLOG(NAN, WARN,
			"PeerSchDesc for " MACSTR " not found\n",
		MAC2STR(prNDL->aucPeerMacAddr));
		return;
	}

	*num_info = 0;

	for (u4Idx = 0; u4Idx < NAN_NUM_AVAIL_DB; u4Idx++) {
		d = &p->arAvailAttr[u4Idx];
		if (d->ucMapId == NAN_INVALID_MAP_ID)
			continue;

		for (u4Idx1 = 0;
			u4Idx1 < NAN_NUM_AVAIL_TIMELINE;
			u4Idx1++) {
			t = &d->arAvailEntryList[u4Idx1];
			if (t->fgActive == FALSE)
				continue;

			if (t->arBandChnlCtrl[0].u4Type ==
				NAN_BAND_CH_ENTRY_LIST_TYPE_BAND)
				continue;

			for (u4Idx2 = 0;
				u4Idx2 < t->ucNumBandChnlCtrl;
				u4Idx2++) {
				chctrl = t->arBandChnlCtrl[u4Idx2];
				pch = chctrl.u4PrimaryChnl;
				opc = chctrl.u4OperatingClass;
				info[i].channel = pch;
				info[i].bandwidth = nanRegGetBw(opc);
				info[i].nss = 2;
				DBGLOG(NAN, DEBUG,
					"[%u][%u]Map:%d,Bw:%d,Ch:%d\n",
					u4Idx, u4Idx1,
					d->ucMapId,
					info[i].bandwidth,
					info[i].channel);
				i++;
				*num_info = i;
				if (i >=
					NAN_MAX_CHANNEL_INFO_SUPPORTED)
					return;
			}
		}
	}
}

uint32_t nanOidDataRequest(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct _NAN_CMD_DATA_REQUEST *prNanCmdDataRequest;
	int32_t rStatus = WLAN_STATUS_SUCCESS;
	struct NanDataReqReceive rDataRcv;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	*pu4SetInfoLen = u4SetBufferLen;
	prNanCmdDataRequest =
		(struct _NAN_CMD_DATA_REQUEST *) pvSetBuffer;

	if (u4SetBufferLen <
		sizeof(struct _NAN_CMD_DATA_REQUEST))
		return WLAN_STATUS_INVALID_DATA;

	nanBackToNormal(prAdapter);

	rStatus = nanCmdDataRequest(prAdapter,
		prNanCmdDataRequest,
		&rDataRcv.ndpid,
		rDataRcv.initiator_data_addr);

	DBGLOG(NAN, DEBUG,
	       "Initiator request to peer " MACSTR ", status = %d\n",
	       MAC2STR(prNanCmdDataRequest->aucResponderDataAddress),
	       rStatus);

	return rStatus;
}


uint32_t nanOidDataResponse(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct _NAN_CMD_DATA_RESPONSE *prNanCmdDataResponse;
	int32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	*pu4SetInfoLen = u4SetBufferLen;
	prNanCmdDataResponse =
		(struct _NAN_CMD_DATA_RESPONSE *) pvSetBuffer;

	if (u4SetBufferLen <
		sizeof(struct _NAN_CMD_DATA_RESPONSE))
		return WLAN_STATUS_INVALID_DATA;

	nanBackToNormal(prAdapter);

	rStatus = nanCmdDataResponse(prAdapter, prNanCmdDataResponse);

	DBGLOG(NAN, DEBUG,
	   "Responder response to peer " MACSTR ", status = %d\n",
	   MAC2STR(prNanCmdDataResponse->aucInitiatorDataAddress),
	   rStatus);

	return rStatus;
}

uint32_t nanOidEndReq(
	struct ADAPTER *prAdapter,
	void *pvSetBuffer,
	uint32_t u4SetBufferLen,
	uint32_t *pu4SetInfoLen)
{
	struct _NAN_CMD_DATA_END *prNanCmdDataEnd;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);
	ASSERT(pvSetBuffer);

	*pu4SetInfoLen = u4SetBufferLen;
	prNanCmdDataEnd = (struct _NAN_CMD_DATA_END *) pvSetBuffer;

	if (u4SetBufferLen <
		sizeof(struct _NAN_CMD_DATA_END))
		return WLAN_STATUS_INVALID_DATA;

	return nanCmdDataEnd(prAdapter, prNanCmdDataEnd);
}

/*----------------------------------------------------------------------------*/
/*!
* \brief After NDI interface create, send create response to wifi hal
*
* \param[in] prNDP: NDP info
*
* \return WLAN_STATUS
*/
/*----------------------------------------------------------------------------*/
uint32_t
nanNdiCreateRspEvent(struct ADAPTER *prAdapter,
		struct NdiIfaceCreate rNdiInterfaceCreate) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2CreateRspLen;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}


	DBGLOG(NAN, DEBUG, "Send NDI Create Rsp event\n");

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2CreateRspLen = (3 * sizeof(uint32_t)) + sizeof(uint16_t) +
			 (4 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2CreateRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_INTERFACE_CREATE) <
		     0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* Transaction ID */
	if (unlikely(nla_put_u16(skb, MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
				rNdiInterfaceCreate.u2NdpTransactionId) < 0)){
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* NMI(same as NDI) */
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR,
				strlen(rNdiInterfaceCreate.pucIfaceName) + 1,
				rNdiInterfaceCreate.pucIfaceName) < 0)){
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* prNDP no NanInternalStatusType field,
	 * set NAN_I_STATUS_SUCCESS as workaround
	 */
	if (unlikely(nla_put_u32(
			     skb,
			     MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			     NAN_I_STATUS_SUCCESS) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 DP_REASON_SUCCESS) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief After NDI interface delete, send delete response to wifi hal
 *
 * \param[in] prNDP: NDP info
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdiDeleteRspEvent(struct ADAPTER *prAdapter,
		struct NdiIfaceDelete rNdiInterfaceDelete) {
	struct sk_buff *skb = NULL;
	struct net_device *prNetDevice = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	uint16_t u2CreateRspLen;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDI Delete Rsp event\n");

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	if (wiphy == NULL) {
		DBGLOG(NAN, ERROR, "[%s] wiphy is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	prNetDevice = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (prNetDevice == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNetDevice is NULL\n", __func__);
		return -EFAULT;
	}
	wdev = prNetDevice->ieee80211_ptr;
	u2CreateRspLen = (3 * sizeof(uint32_t)) + sizeof(uint16_t) +
			 (4 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2CreateRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_INTERFACE_DELETE) <
					0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* Transaction ID */
	if (unlikely(nla_put_u16(skb, MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
				rNdiInterfaceDelete.u2NdpTransactionId) < 0)){
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* NMI(same as NDI) */
	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR,
				strlen(rNdiInterfaceDelete.pucIfaceName) + 1,
				rNdiInterfaceDelete.pucIfaceName) < 0)){
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}
	/* prNDP no NanInternalStatusType field,
	 * set NAN_I_STATUS_SUCCESS as workaround
	 */
	if (unlikely(nla_put_u32(
			     skb,
			     MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			     NAN_I_STATUS_SUCCESS) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 DP_REASON_SUCCESS) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief After Tx initiator req NAF TxDone, send initiator response to wifi hal
 *
 * \param[in] prNDP: NDP info
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpInitiatorRspEvent(struct ADAPTER *prAdapter,
			struct _NAN_NDP_INSTANCE_T *prNDP,
			uint32_t rTxDoneStatus) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2InitiatorRspLen;
	uint32_t u4Id = 0;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDP is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "[%s] Send NDP Initiator Rsp event\n", __func__);

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2InitiatorRspLen = (4 * sizeof(uint32_t)) + (1 * sizeof(uint16_t)) +
			    (5 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2InitiatorRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_INITIATOR_RESPONSE) <
		     0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u16(skb, MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
				 prNDP->u2TransId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (nanGetFeatureIsSigma(prAdapter))
		u4Id = prNDP->ucNDPID;
	else
		u4Id = prNDP->ndp_instance_id;

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID,
				 u4Id) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* prNDP no NanInternalStatusType field,
	 * set NAN_I_STATUS_SUCCESS and NAN_I_STATUS_TIMEOUT as workaround
	 */
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		if (unlikely(nla_put_u32(skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_SUCCESS) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	} else {
		if (unlikely(nla_put_u32(skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_TIMEOUT) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 prNDP->eDataPathFailReason) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief After Tx responder req NAF TxDone, send responder response to wifi hal
 *
 * \param[in] prNDP: NDP info
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpResponderUserTimeoutEvent(struct ADAPTER *prAdapter,
				uint32_t ndp_instance_id,
				uint16_t u2TransId)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2ResponderRspLen;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDP Response event NdpId(%u) trans=%u\n",
	       ndp_instance_id, u2TransId);

	if (u2TransId == 0) {
		DBGLOG(NAN, ERROR,
		       "Invalid transaction id NdpId(%u) trans=%u\n",
		       ndp_instance_id, u2TransId);
		return WLAN_STATUS_INVALID_DATA;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2ResponderRspLen = (3 * sizeof(uint32_t)) + sizeof(uint16_t) +
			    (4 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2ResponderRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb,
			MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
			MTK_WLAN_VENDOR_ATTR_NDP_RESPONDER_RESPONSE) <
		     0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u16(skb,
			MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
			u2TransId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb,
		    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
		    NAN_I_STATUS_TIMEOUT) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb,
			MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
			DP_REASON_USER_SPACE_RESPONSE_TIMEOUT) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

uint32_t
nanNdpResponderRspEvent(struct ADAPTER *prAdapter,
			struct _NAN_NDP_INSTANCE_T *prNDP,
			uint32_t rTxDoneStatus) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2ResponderRspLen;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDP is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDP Response event NdpId(%u) trans=%u\n",
	       prNDP->ndp_instance_id, prNDP->u2TransId);

	if (prNDP->u2TransId == 0) {
		DBGLOG(NAN, ERROR,
		       "Invalid transaction id NdpId(%u) trans=%u\n",
		       prNDP->ndp_instance_id, prNDP->u2TransId);
		return WLAN_STATUS_INVALID_DATA;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2ResponderRspLen = (3 * sizeof(uint32_t)) + sizeof(uint16_t) +
			    (4 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2ResponderRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_RESPONDER_RESPONSE) <
		     0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u16(skb, MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
				 prNDP->u2TransId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* prNDP no NanInternalStatusType field,
	 * set NAN_I_STATUS_SUCCESS and NAN_I_STATUS_TIMEOUT as workaround
	 */
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		if (unlikely(nla_put_u32(skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_SUCCESS) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	} else {
		if (unlikely(nla_put_u32(skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_TIMEOUT) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 prNDP->eDataPathFailReason) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief After Tx termination req NAF TxDone, send end response to wifi hal
 *
 * \param[in] prNDP: NDP info
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t
nanNdpEndRspEvent(struct ADAPTER *prAdapter,
	enum _ENUM_DP_PROTOCOL_REASON_CODE_T eReason,
	uint16_t u2TransId,
	uint32_t rTxDoneStatus)
{
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2EndRspLen;

	if (prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prAdapter is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDI End Rsp event\n");

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2EndRspLen = (3 * sizeof(uint32_t)) + sizeof(uint16_t) +
		      (4 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2EndRspLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_END_RESPONSE) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	/* prNDP no NanInternalStatusType field,
	 * set NAN_I_STATUS_SUCCESS and NAN_I_STATUS_TIMEOUT as workaround
	 */
	if (rTxDoneStatus == WLAN_STATUS_SUCCESS) {
		if (unlikely(nla_put_u32(
			    skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_SUCCESS) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	} else {
		if (unlikely(nla_put_u32(skb,
			    MTK_WLAN_VENDOR_ATTR_NDP_DRV_RESPONSE_STATUS_TYPE,
			    NAN_I_STATUS_TIMEOUT) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 eReason) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}
	if (unlikely(nla_put_u16(skb, MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID,
				 u2TransId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle NAN interface create request vendor cmd.
 *
 * \param[in] prGlueInfo: Pointer to glue info structure.
 *
 * \param[in] tb: NDP vendor cmd attributes.
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
int32_t nanNdiCreateHandler(struct GLUE_INFO *prGlueInfo, struct nlattr **tb)
{
	/* Need implement */
	struct ADAPTER *prAdapter = NULL;
	struct NdiIfaceCreate rNdiInterfaceCreate;

	kalMemZero(&rNdiInterfaceCreate, sizeof(struct NdiIfaceCreate));

	if (prGlueInfo == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prGlueInfo is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Get transaction ID */
	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]) {
		DBGLOG(NAN, ERROR, "Get NDP Transaction ID error!\n");
		return -EINVAL;
	}
	rNdiInterfaceCreate.u2NdpTransactionId =
		nla_get_u16(tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]);

	/* Get interface name */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR]) {
		rNdiInterfaceCreate.pucIfaceName =
			nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR]);
		DBGLOG(NAN, DEBUG,
			"[%s] Transaction ID: %d Interface name: %s\n",
			__func__, rNdiInterfaceCreate.u2NdpTransactionId,
			rNdiInterfaceCreate.pucIfaceName);
	}

	/* Send event to wifi hal */
	prAdapter = prGlueInfo->prAdapter;
	nanNdiCreateRspEvent(prAdapter, rNdiInterfaceCreate);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle NAN interface delete request vendor cmd.
 *
 * \param[in] prGlueInfo: Pointer to glue info structure.
 *
 * \param[in] tb: NDP vendor cmd attributes.
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
int32_t nanNdiDeleteHandler(struct GLUE_INFO *prGlueInfo, struct nlattr **tb)
{
	/* Need implement */
	struct ADAPTER *prAdapter = NULL;
	struct NdiIfaceDelete rNdiInterfaceDelete;

	kalMemZero(&rNdiInterfaceDelete, sizeof(struct NdiIfaceDelete));

	if (prGlueInfo == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prGlueInfo is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "NAN interface delete request, need implement!\n");

	/* Get transaction ID */
	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]) {
		DBGLOG(NAN, ERROR, "Get NDP Transaction ID error!\n");
		return -EINVAL;
	}
	rNdiInterfaceDelete.u2NdpTransactionId =
		nla_get_u16(tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]);

	/* Get interface name */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR]) {
		rNdiInterfaceDelete.pucIfaceName =
			nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_IFACE_STR]);
		DBGLOG(NAN, DEBUG,
			"[%s] Delete transaction ID: %d Interface name: %s\n",
			__func__, rNdiInterfaceDelete.u2NdpTransactionId,
			rNdiInterfaceDelete.pucIfaceName);
	}

	/* Workaround: send event to wifi hal */
	prAdapter = prGlueInfo->prAdapter;
	nanNdiDeleteRspEvent(prAdapter, rNdiInterfaceDelete);
	return WLAN_STATUS_SUCCESS;
}



/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle NDP initiator request vendor cmd.
 *
 * \param[in] prGlueInfo: Pointer to glue info structure.
 *
 * \param[in] tb: NDP vendor cmd attributes.
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
int32_t nanNdpInitiatorReqHandler(struct GLUE_INFO *prGlueInfo,
		struct nlattr **tb)
{
	struct _NAN_CMD_DATA_REQUEST rNanCmdDataRequest;
	int32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t aucPassphrase[64] = {0};
	uint8_t aucSalt[] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint32_t u4BufLen;

	kalMemZero(&rNanCmdDataRequest, sizeof(rNanCmdDataRequest));

	/* Instance ID */
	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_INSTANCE_ID]) {
		DBGLOG(NAN, ERROR, "Get NDP Instance ID unavailable!\n");
		return -EINVAL;
	}

	/* Get transaction ID */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]) {
		rNanCmdDataRequest.u2NdpTransactionId = nla_get_u32(
			tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]);
		DBGLOG(NAN, ERROR, "Get NDP Transaction ID = %d\n",
			rNanCmdDataRequest.u2NdpTransactionId);
	}

	/* Peer mac Addr */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]) {
		kalMemCopy(rNanCmdDataRequest.aucResponderDataAddress,
		nla_data(
		tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]),
		MAC_ADDR_LEN);
	}
	rNanCmdDataRequest.ucPublishID =
		nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_INSTANCE_ID]);
	if (rNanCmdDataRequest.ucPublishID == 0)
		rNanCmdDataRequest.ucPublishID = g_u2IndPubId;

	/* Security */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_SECURITY]) {
		if (tb[MTK_WLAN_VENDOR_ATTR_NDP_CSID])
			rNanCmdDataRequest.ucSecurity =
				nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_CSID]);

		if (rNanCmdDataRequest.ucSecurity &&
		    tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]) {
			kalMemCopy(rNanCmdDataRequest.aucPMK,
				   nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]),
				   nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]));

#if (ENABLE_SEC_UT_LOG == 1)
			DBGLOG(NAN, DEBUG, "PMK from APP\n");
			dumpMemory8(nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]),
				    nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]));
#endif
		}

		if (rNanCmdDataRequest.ucSecurity) {
			if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]) {
				DBGLOG(NAN, DEBUG,
				"[%s] PASSPHRASE\n",
				__func__);
				kalMemCopy(
				aucPassphrase,
				nla_data(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]),
				nla_len(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]));
				kalMemCopy(aucSalt + 2,
				g_aucNanServiceId, 6);
				dumpMemory8(
				g_aucNanServiceId, 6);
				kalMemCopy(aucSalt + 8,
				rNanCmdDataRequest.aucResponderDataAddress,
				6);
				dumpMemory8(
				aucPassphrase, sizeof(aucPassphrase));
				dumpMemory8(
				aucSalt, sizeof(aucSalt));
				PKCS5_PBKDF2_HMAC(
				(unsigned char *)aucPassphrase,
				sizeof(aucPassphrase) - 1,
				(unsigned char *)aucSalt, sizeof(aucSalt),
				4096, 32,
				(unsigned char *)rNanCmdDataRequest.aucPMK
				);

				dumpMemory8(rNanCmdDataRequest.aucPMK, 32);
			}
			if (tb[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_NAME]) {
				DBGLOG(NAN, DEBUG,
					"[%s] pmkid(vendor cmd)\n",
					__func__);
				nanSetNdpPmkid(
				prGlueInfo->prAdapter,
				&rNanCmdDataRequest,
				nla_data(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_NAME]
				));
			} else {
				DBGLOG(NAN, DEBUG,
					"[%s] pmkid(local)\n",
					__func__);
				nanSetNdpPmkid(
				prGlueInfo->prAdapter,
				&rNanCmdDataRequest,
				g_aucNanServiceName);
				dumpMemory8(
				g_aucNanServiceName, NAN_MAX_SERVICE_NAME_LEN);
#ifdef NAN_UNUSED
				memset(g_aucNanServiceName, 0,
				NAN_MAX_SERVICE_NAME_LEN);
#endif
			}
		}
	}

	/* QoS: Default set to false for testing */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS]) {
		rNanCmdDataRequest.ucRequireQOS = 0;
		/* rNanCmdDataRequest.ucRequireQOS = */
		/* nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS]); */
	}

	/* Peer mac Addr */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]) {
		kalMemCopy(rNanCmdDataRequest.aucResponderDataAddress,
			nla_data(
			tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]),
			MAC_ADDR_LEN);
	}

	rNanCmdDataRequest.fgNDPE = g_ndpReqNDPE.fgEnNDPE;
	/* APP Info */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]) {
		rNanCmdDataRequest.u2SpecificInfoLength =
			nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]);
		kalMemCopy(rNanCmdDataRequest.aucSpecificInfo,
			nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]),
			nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]));

		DBGLOG(NAN, DEBUG, "[%s] AppInfoLen = %d\n",
			__func__, rNanCmdDataRequest.u2SpecificInfoLength);
	}

	/* Ipv6 */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_IPV6_ADDR]) {
		rNanCmdDataRequest.fgCarryIpv6 = 1;
		kalMemCopy(rNanCmdDataRequest.aucIPv6Addr, nla_data(
		tb[MTK_WLAN_VENDOR_ATTR_NDP_IPV6_ADDR]), IPV6MACLEN);
	}

	/* NDPE */
	DBGLOG(NAN, DEBUG, "[%s] NDPEenable = %d\n",
		__func__, g_ndpReqNDPE.fgEnNDPE);

	/* Send cmd request */
	rStatus =  kalIoctl(prGlueInfo,
		nanOidDataRequest,
		&rNanCmdDataRequest,
		sizeof(struct _NAN_CMD_DATA_REQUEST),
		&u4BufLen);

	/* Return status */
	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle NDP reponder request vendor cmd.
 *
 * \param[in] prGlueInfo: Pointer to glue info structure.
 *
 * \param[in] tb: NDP vendor cmd attributes.
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
int32_t nanNdpResponderReqHandler(struct GLUE_INFO *prGlueInfo,
		struct nlattr **tb)
{
	struct _NAN_CMD_DATA_RESPONSE rNanCmdDataResponse;
	int32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t aucPassphrase[64] = {0};
	uint8_t aucSalt[] = { 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	struct BSS_INFO *prBssInfo;
	struct _NAN_SPECIFIC_BSS_INFO_T *prNanSpecificBssInfo;
	uint32_t u4BufLen;

	if (prGlueInfo->prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter is null\n");
		return -EINVAL;
	}

	/* Get BSS info */
	prNanSpecificBssInfo = nanGetSpecificBssInfo(
		prGlueInfo->prAdapter,
		NAN_BSS_INDEX_BAND0);
	if (prNanSpecificBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prNanSpecificBssInfo is null\n");
		return -EINVAL;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(
			prGlueInfo->prAdapter,
			prNanSpecificBssInfo->ucBssIndex);
	if (prBssInfo == NULL) {
		DBGLOG(NAN, ERROR, "prBssInfo is null\n");
		return -EINVAL;
	}

	kalMemZero(&rNanCmdDataResponse, sizeof(rNanCmdDataResponse));
	/* Decision status */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_RESPONSE_CODE]) {
		if (nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_RESPONSE_CODE])
				== NAN_DP_REQUEST_AUTO)
			rNanCmdDataResponse.ucDecisionStatus =
				NAN_DP_REQUEST_ACCEPT;
		else
			rNanCmdDataResponse.ucDecisionStatus =
				nla_get_u32(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_RESPONSE_CODE]);
	}

	/* Instance ID */
	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID]) {
		DBGLOG(NAN, ERROR, "Get NDP Instance ID unavailable!\n");
		return -EINVAL;
	}

	if (nanGetFeatureIsSigma(prGlueInfo->prAdapter)) {
		rNanCmdDataResponse.ucNDPId =
			nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID]);
		DBGLOG(NAN, DEBUG, "[Data Resp] RespID:%d\n",
	       rNanCmdDataResponse.ucNDPId);
		if (rNanCmdDataResponse.ucNDPId == 0)
			rNanCmdDataResponse.ucNDPId = g_u2IndPubId;
	} else {
		rNanCmdDataResponse.ndp_instance_id =
			nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID]);
		DBGLOG(NAN, DEBUG, "[Data Resp] InstanceRespID:%d\n",
			rNanCmdDataResponse.ndp_instance_id);
	}

	/* Get transaction ID */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]) {
		rNanCmdDataResponse.u2NdpTransactionId = nla_get_u32(
			tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]);
		DBGLOG(NAN, ERROR, "Get NDP Transaction ID =%d\n",
			rNanCmdDataResponse.u2NdpTransactionId);
	}

	/* QoS: Default set to false for testing */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS]) {
		rNanCmdDataResponse.ucRequireQOS = 0;
		/* rNanCmdDataResponse.ucRequireQOS =
		 * nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_CONFIG_QOS]);
		 */
	}

	/* Security type */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_CSID])
		rNanCmdDataResponse.ucSecurity =
			nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_CSID]);

	/* App Info */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]) {
		rNanCmdDataResponse.u2SpecificInfoLength =
			nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]);
		kalMemCopy(rNanCmdDataResponse.aucSpecificInfo,
			nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]),
			nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO]));

		DBGLOG(NAN, ERROR, "[%s] appInfoLen= %d\n",
			__func__,
			rNanCmdDataResponse.u2SpecificInfoLength);
	}

	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_IPV6_ADDR]) {
		kalMemCopy(rNanCmdDataResponse.aucIPv6Addr,
			nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_IPV6_ADDR]),
			IPV6MACLEN);
		rNanCmdDataResponse.fgCarryIpv6 = 1;
	}

	if (nanGetFeatureIsSigma(prGlueInfo->prAdapter)) {
		/* PortNum: vendor cmd did not fill this attribute,
		 * default set to 9000
		 */
		rNanCmdDataResponse.u2PortNum = 9000;

		/* Service protocol type:
		 * vendor cmd did not fill this attribute,
		 * default set to 0xFF
		 */
		rNanCmdDataResponse.ucServiceProtocolType = IP_PRO_TCP;
	}

	/* Peer mac addr */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]) {
		kalMemCopy(
			rNanCmdDataResponse.aucInitiatorDataAddress,
			nla_data(
			tb[MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR]),
			MAC_ADDR_LEN);
	} else {
		kalMemZero(rNanCmdDataResponse.aucInitiatorDataAddress,
			MAC_ADDR_LEN);
	}
	DBGLOG(NAN, DEBUG, "[%s] aucInitiatorDataAddress = " MACSTR "\n",
	       __func__, MAC2STR(rNanCmdDataResponse.aucInitiatorDataAddress));
	/* PMK */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]) {
		kalMemCopy(rNanCmdDataResponse.aucPMK,
			   nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]),
			   nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]));
#if (ENABLE_SEC_UT_LOG == 1)
		DBGLOG(NAN, DEBUG, "PMK from APP\n");
		dumpMemory8(nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]),
			    nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_PMK]));
#endif
	}
	/* PASSPHRASE */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]) {
		DBGLOG(NAN, DEBUG, "[%s] PASSPHRASE\n", __func__);
		kalMemCopy(aucPassphrase,
			   nla_data(tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]),
			   nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_PASSPHRASE]));
		kalMemCopy(aucSalt + 2,
				g_aucNanServiceId, 6);
		kalMemCopy(aucSalt + 8,
		prBssInfo->aucOwnMacAddr,
		6);
		dumpMemory8(aucPassphrase, sizeof(aucPassphrase));
		dumpMemory8(aucSalt, sizeof(aucSalt));
		PKCS5_PBKDF2_HMAC(
			  (unsigned char *)aucPassphrase,
			  sizeof(aucPassphrase) - 1,
			  (unsigned char *)aucSalt,
			  sizeof(aucSalt),
			  4096, 32,
			  (unsigned char *)rNanCmdDataResponse.aucPMK);

		dumpMemory8(rNanCmdDataResponse.aucPMK, 32);
	}

	/* Send data response */
	rStatus =  kalIoctl(prGlueInfo,
		nanOidDataResponse,
		&rNanCmdDataResponse,
		sizeof(struct _NAN_CMD_DATA_RESPONSE),
		&u4BufLen);

	/* Return */
	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Handle NDP end vendor cmd.
 *
 * \param[in] prGlueInfo: Pointer to glue info structure.
 *
 * \param[in] tb: NDP vendor cmd attributes.
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
int32_t nanNdpEndReqHandler(struct GLUE_INFO *prGlueInfo, struct nlattr **tb)
{
	struct _NAN_CMD_DATA_END rNanCmdDataEnd;
	int32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t instanceIdNum;
	uint32_t i;

	kalMemZero(&rNanCmdDataEnd, sizeof(rNanCmdDataEnd));

	/* trial run! */
	DBGLOG(NAN, DEBUG, "NDP end request\n");

	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY]) {
		DBGLOG(NAN, ERROR, "Get NDP Instance ID unavailable!\n");
		return -EINVAL;
	}
	instanceIdNum =
		nla_len(tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY]) /
		sizeof(uint32_t);
	if (instanceIdNum <= 0) {
		DBGLOG(NAN, ERROR, "No NDP Instance ID!\n");
		return -EINVAL;
	}

	/* Get transaction ID */
	if (tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]) {
		rNanCmdDataEnd.u2NdpTransactionId = nla_get_u32(
			tb[MTK_WLAN_VENDOR_ATTR_NDP_TRANSACTION_ID]);
		DBGLOG(NAN, ERROR, "Get NDP Transaction ID =%d\n",
			rNanCmdDataEnd.u2NdpTransactionId);
	}

	for (i = 0; i < instanceIdNum; i++) {
		uint32_t u4BufLen;

		if (nanGetFeatureIsSigma(prGlueInfo->prAdapter))
			rNanCmdDataEnd.ucNDPId = nla_get_u32(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY] +
				i * sizeof(uint32_t));
		else
			rNanCmdDataEnd.ndp_instance_id = nla_get_u32(
				tb[MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY] +
				i * sizeof(uint32_t));
		rStatus =  kalIoctl(prGlueInfo,
			nanOidEndReq,
			&rNanCmdDataEnd,
			sizeof(struct _NAN_CMD_DATA_END),
			&u4BufLen);
	}
	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send data indication event to hal.
 *
 * \param[in] prNDP: NDP info attribute
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanNdpDataIndEvent(struct ADAPTER *prAdapter,
		   struct _NAN_NDP_INSTANCE_T *prNDP,
		   struct _NAN_NDL_INSTANCE_T *prNDL) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint16_t u2IndiEventLen;
	uint32_t u4Id = 0;

	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDP is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (prNDL == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDL is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDP Data Indication event\n");

	if (prAdapter->rNanNetRegState == ENUM_NET_REG_STATE_UNREGISTERED) {
		DBGLOG(NAN, ERROR, "Net device for NAN unregistered\n");
		return WLAN_STATUS_FAILURE;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2IndiEventLen = (3 * sizeof(uint32_t)) + (2 * MAC_ADDR_LEN) +
			 prNDP->u2AppInfoLen + NAN_SCID_DEFAULT_LEN +
			 (6 * NLA_HDRLEN) + NLMSG_HDRLEN;

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2IndiEventLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_REQUEST_IND) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb,
				 MTK_WLAN_VENDOR_ATTR_NDP_SERVICE_INSTANCE_ID,
				 prNDP->ucPublishId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_NDI_MAC_ADDR,
			     MAC_ADDR_LEN, prNDP->aucPeerNDIAddr) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}
	kalMemCopy(g_InitiatorMacAddr, prNDP->aucPeerNDIAddr, MAC_ADDR_LEN);
	DBGLOG(NAN, DEBUG, "[%s] gInitiatorMacAddr = " MACSTR "\n", __func__,
	       MAC2STR(g_InitiatorMacAddr));

	if (unlikely(nla_put(skb,
			     MTK_WLAN_VENDOR_ATTR_NDP_PEER_DISCOVERY_MAC_ADDR,
			     MAC_ADDR_LEN, prNDL->aucPeerMacAddr) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (nanGetFeatureIsSigma(prAdapter))
		u4Id = prNDP->ucNDPID;
	else
		u4Id = prNDP->ndp_instance_id;

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID,
				 u4Id) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (prNDP->u2PeerAppInfoLen) {
		if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO,
				     prNDP->u2PeerAppInfoLen,
				     prNDP->pucPeerAppInfo) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	}

/* Todo:
 * 1. QoS currently not support.
 * 2. Need to clarify security parameter
 */
#if 0
	if (prNDP->fgQoSRequired)
		continue;

	if (prNDP->fgSecurityRequired) {
		if (unlikely(nla_put_u32(skb,
					MTK_WLAN_VENDOR_ATTR_NDP_NCS_SK_TYPE,
					prNDP->ucCipherType) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
		if (unlikely(nla_put(skb,
				     MTK_WLAN_VENDOR_ATTR_NDP_SCID,
				     NAN_SCID_DEFAULT_LEN,
				     prNDP->au1Scid) < 0)) {
			DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
			kfree_skb(skb);
			return -EFAULT;
		}
	}
#endif
	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send data confirm event to hal.
 *
 * \param[in] prNDP: NDP info attribute
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanNdpDataConfirmEvent(struct ADAPTER *prAdapter,
		       struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint32_t u2ConfirmEventLen;
	uint32_t u4Id = 0;
	struct NanChannelInfo
		info[NAN_MAX_CHANNEL_INFO_SUPPORTED];
	uint32_t num_info = 0;
	struct nlattr *array, *item;

	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDP is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDP Data Confirm event\n");

	if (prAdapter->rNanNetRegState == ENUM_NET_REG_STATE_UNREGISTERED) {
		DBGLOG(NAN, ERROR, "Net device for NAN unregistered\n");
		return WLAN_STATUS_FAILURE;
	}

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2ConfirmEventLen = (4 * sizeof(uint32_t)) + MAC_ADDR_LEN +
			    +NLMSG_HDRLEN + (6 * NLA_HDRLEN) +
			    prNDP->u2AppInfoLen;
	/* WIFI_EVENT_SUBCMD_NDP: Event Idx is 13 for kernel,
	 *  but for WifiHal is 81
	 */
	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2ConfirmEventLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_CONFIRM_IND) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (nanGetFeatureIsSigma(prAdapter))
		u4Id = prNDP->ucNDPID;
	else
		u4Id = prNDP->ndp_instance_id;

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID,
				 u4Id) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_NDI_MAC_ADDR,
			     MAC_ADDR_LEN, prNDP->aucPeerNDIAddr) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (prNDP->fgCarryIPV6 && unlikely(nla_put(skb,
		MTK_WLAN_VENDOR_ATTR_NDP_IPV6_ADDR,
		IPV6MACLEN, prNDP->aucRspInterfaceId) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (prNDP->fgCarryIPV6)
		DBGLOG(NAN, DEBUG, "[%s] fgCarryIPV6 = " IPV6STR "\n",
		__func__, IPV6TOSTR(prNDP->aucRspInterfaceId));

	if (prNDP->pucPeerAppInfo &&
	    unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_APP_INFO,
	    prNDP->u2PeerAppInfoLen,
		    prNDP->pucPeerAppInfo) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (prNDP->pucPeerAppInfo)
		DBGLOG(NAN, DEBUG, "[%s] u2PeerAppInfoLen = %d\n", __func__,
		prNDP->u2PeerAppInfoLen);

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_RESPONSE_CODE,
				 prNDP->ucReasonCode) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_DRV_RETURN_VALUE,
				 prNDP->eDataPathFailReason) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kfree_skb(skb);
		return -EFAULT;
	}

	if (!prAdapter->rWifiVar.ucNanReportChInfo)
		goto SKIP_REPORT_CHANNEL_INFO;

	nanGetChannelInfo(prAdapter,
		prNDP, info, &num_info);

	if (num_info) {
		uint32_t i = 0;

		if (unlikely(nla_put_u32(skb,
			MTK_WLAN_VENDOR_ATTR_NDP_NUM_CHANNELS,
			num_info) < 0))
			goto SKIP_REPORT_CHANNEL_INFO;

		array = nla_nest_start(skb,
			MTK_WLAN_VENDOR_ATTR_NDP_CHANNEL_INFO);
		if (!array)
			goto SKIP_REPORT_CHANNEL_INFO;

		for (i = 0; i < num_info; i++) {
			item = nla_nest_start(skb, i);
			if (!item)
				goto SKIP_REPORT_CHANNEL_INFO;

			if (unlikely(nla_put_u32(skb,
				MTK_WLAN_VENDOR_ATTR_NDP_CHANNEL,
				info[i].channel) < 0))
				goto SKIP_REPORT_CHANNEL_INFO;

			if (unlikely(nla_put_u32(skb,
				MTK_WLAN_VENDOR_ATTR_NDP_CHANNEL_WIDTH,
				info[i].bandwidth) < 0))
				goto SKIP_REPORT_CHANNEL_INFO;

			if (unlikely(nla_put_u32(skb,
				MTK_WLAN_VENDOR_ATTR_NDP_NSS,
				info[i].nss) < 0))
				goto SKIP_REPORT_CHANNEL_INFO;

			nla_nest_end(skb, item);
		}
		nla_nest_end(skb, array);
	}

SKIP_REPORT_CHANNEL_INFO:

	DBGLOG(NAN, DEBUG, "NDP Data Confirm event, ndp instance: %d,",
		u4Id);
	DBGLOG(NAN, DEBUG, "peer MAC addr : "MACSTR "rsp reason code: %d,",
		MAC2STR(prNDP->aucPeerNDIAddr), prNDP->ucReasonCode);
	DBGLOG(NAN, DEBUG, "protocol reason code: %d\n ",
		prNDP->eDataPathFailReason);

	cfg80211_vendor_event(skb, GFP_KERNEL);
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Send data termination event to hal.
 *
 * \param[in] prNDP: NDP info attribute
 *
 * \return WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/

uint32_t
nanNdpDataTerminationEvent(struct ADAPTER *prAdapter,
			   struct _NAN_NDP_INSTANCE_T *prNDP) {
	struct sk_buff *skb = NULL;
	struct wiphy *wiphy;
	struct wireless_dev *wdev;
	struct net_device *prDevHandler = NULL;
	uint32_t u2ConfirmEventLen;
	uint32_t *pu2NDPInstance;
	uint32_t u4Id = 0;

	if (prNDP == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prNDP is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	pu2NDPInstance = kalMemAlloc(1 * sizeof(*pu2NDPInstance), VIR_MEM_TYPE);
	if (pu2NDPInstance == NULL) {
		DBGLOG(NAN, ERROR, "[%s] pu2NDPInstance is NULL\n", __func__);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(NAN, DEBUG, "Send NDP Data Termination event\n");

	wiphy = GLUE_GET_WIPHY(prAdapter->prGlueInfo);
	prDevHandler = wlanGetNetDev(prAdapter->prGlueInfo, NAN_DEFAULT_INDEX);
	if (!prDevHandler) {
		DBGLOG(INIT, ERROR, "prDevHandler is NULL\n");
		return -EFAULT;
	}
	wdev = prDevHandler->ieee80211_ptr;
	u2ConfirmEventLen = sizeof(uint32_t) + NLMSG_HDRLEN + (2 * NLA_HDRLEN) +
			    1 * sizeof(*pu2NDPInstance);

	skb = kalCfg80211VendorEventAlloc(wiphy, wdev, u2ConfirmEventLen,
					  WIFI_EVENT_SUBCMD_NDP, GFP_KERNEL);
	if (!skb) {
		DBGLOG(REQ, ERROR, "Allocate skb failed\n");
		kalMemFree(pu2NDPInstance,
				   VIR_MEM_TYPE,
				   1 * sizeof(*pu2NDPInstance));
		return -ENOMEM;
	}

	if (unlikely(nla_put_u32(skb, MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD,
				 MTK_WLAN_VENDOR_ATTR_NDP_END_IND) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kalMemFree(pu2NDPInstance,
				   VIR_MEM_TYPE,
				   1 * sizeof(*pu2NDPInstance));
		kfree_skb(skb);
		return -EFAULT;
	}

	if (nanGetFeatureIsSigma(prAdapter))
		u4Id = prNDP->ucNDPID;
	else
		u4Id = prNDP->ndp_instance_id;
	*pu2NDPInstance = (uint32_t)u4Id;

	if (unlikely(nla_put(skb, MTK_WLAN_VENDOR_ATTR_NDP_INSTANCE_ID_ARRAY,
			     1 * sizeof(*pu2NDPInstance),
			     pu2NDPInstance) < 0)) {
		DBGLOG(REQ, ERROR, "nla_put_nohdr failed\n");
		kalMemFree(pu2NDPInstance,
				   VIR_MEM_TYPE,
				   1 * sizeof(*pu2NDPInstance));
		kfree_skb(skb);
		return -EFAULT;
	}

	DBGLOG(NAN, DEBUG, "NDP Data Termination event, ndp instance: %d\n",
	       u4Id);

	cfg80211_vendor_event(skb, GFP_KERNEL);

	kalMemFree(pu2NDPInstance,
			   VIR_MEM_TYPE,
			   1 * sizeof(*pu2NDPInstance));
	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is the enter point of NDP vendor cmd.
 *
 * \param[in] wiphy: for AIS STA
 *
 * \param[in] wdev (not used here).
 *
 * \param[in] data: Content of NDP vendor cmd .
 *
 * \param[in] data_len: NDP vendor cmd length.
 *
 * \return int
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_vendor_ndp(struct wiphy *wiphy, struct wireless_dev *wdev,
			const void *data, int data_len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct nlattr *tb[MTK_WLAN_VENDOR_ATTR_NDP_PARAMS_MAX + 1];
	uint32_t u4NdpCmdType;
	int32_t rStatus;

	if (wiphy == NULL) {
		DBGLOG(NAN, ERROR, "[%s] wiphy is NULL\n", __func__);
		return -EINVAL;
	}

	if (wdev == NULL) {
		DBGLOG(NAN, ERROR, "[%s] wdev is NULL\n", __func__);
		return -EINVAL;
	}

	if (data == NULL || data_len <= 0) {
		log_dbg(REQ, ERROR, "data error(len=%d)\n", data_len);
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL) {
		DBGLOG(NAN, ERROR, "[%s] prGlueInfo is NULL\n", __func__);
		return -EINVAL;
	}

	if (prGlueInfo->prAdapter == NULL) {
		DBGLOG(NAN, ERROR, "prAdapter is null\n");
		return -EINVAL;
	}

	if (!prGlueInfo->prAdapter->fgIsNANRegistered) {
		DBGLOG(NAN, ERROR, "NAN Not Register yet!\n");
		return -EINVAL;
	}

	DBGLOG(NAN, TRACE, "DATA len from user %d\n", data_len);

	/* Parse NDP vendor cmd */
	if (NLA_PARSE(tb, MTK_WLAN_VENDOR_ATTR_NDP_PARAMS_MAX, data, data_len,
		      mtk_wlan_vendor_ndp_policy)) {
		DBGLOG(NAN, ERROR, "Parse NDP cmd fail!\n");
		return -EINVAL;
	}

	/* Get NDP command type*/
	if (!tb[MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD]) {
		DBGLOG(NAN, ERROR, "Get NDP cmd type error!\n");
		return -EINVAL;
	}
	u4NdpCmdType = nla_get_u32(tb[MTK_WLAN_VENDOR_ATTR_NDP_SUBCMD]);

	switch (u4NdpCmdType) {
	/* Command to create a NAN data path interface */
	case MTK_WLAN_VENDOR_ATTR_NDP_INTERFACE_CREATE:
		rStatus = nanNdiCreateHandler(prGlueInfo, tb);
		break;
	/* Command to delete a NAN data path interface */
	case MTK_WLAN_VENDOR_ATTR_NDP_INTERFACE_DELETE:
		rStatus = nanNdiDeleteHandler(prGlueInfo, tb);
		break;
	/* Command to initiate a NAN data path session */
	case MTK_WLAN_VENDOR_ATTR_NDP_INITIATOR_REQUEST:
		rStatus = nanNdpInitiatorReqHandler(prGlueInfo, tb);
		break;
	/* Command to respond to NAN data path session */
	case MTK_WLAN_VENDOR_ATTR_NDP_RESPONDER_REQUEST:
		rStatus = nanNdpResponderReqHandler(prGlueInfo, tb);
		break;
	/* Command to initiate a NAN data path end */
	case MTK_WLAN_VENDOR_ATTR_NDP_END_REQUEST:
		rStatus = nanNdpEndReqHandler(prGlueInfo, tb);
		break;
	default:
		return -EOPNOTSUPP;
	}

	return rStatus;
}
#endif /* CFG_SUPPORT_NAN */
