// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */
#include "precomp.h"

struct wifi_iface_limit ap[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP),
	},
};

struct wifi_iface_limit sta_sta[] = {
	{
		.max_limit = 2,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
};

struct wifi_iface_limit ap_ap[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP_BRIDGED),
	},
};

struct wifi_iface_limit sta_ap[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP),
	},
};

struct wifi_iface_limit sta_p2p[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_P2P),
	},
};

struct wifi_iface_limit sta_nan[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_NAN),
	},
};

struct wifi_iface_limit sta_sta_sta[] = {
	{
		.max_limit = 3,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
};

struct wifi_iface_limit sta_p2p_p2p[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 2,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_P2P),
	},
};

struct wifi_iface_limit sta_ap_p2p[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_P2P),
	},
};

struct wifi_iface_limit sta_ap_nan[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_NAN),
	},
};

struct wifi_iface_limit sta_p2p_nan[] = {
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_P2P),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_NAN),
	},
};

struct wifi_iface_limit sta_sta_p2p[] = {
	{
		.max_limit = 2,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_P2P),
	},
};

struct wifi_iface_limit sta_sta_ap[] = {
	{
		.max_limit = 2,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_STA),
	},
	{
		.max_limit = 1,
		.iface_mask = BIT(WIFI_INTERFACE_TYPE_AP),
	},
};

/******************************************************************************
 *		      P R O J E C T    C O M B I N A T I O N S
 ******************************************************************************
 */

#if (CFG_SUPPORT_CONNAC1X == 1) /* Legacy */ || defined(MT6631) /* Rayas */
struct mtk_wifi_iface_combination mtk_ifaces_combinations[] = {
	{
		.max_ifaces = 1,
		.num_iface_limits = ARRAY_SIZE(ap),
		.iface_limits = ap,
	},
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_ap),
		.iface_limits = sta_ap,
	},
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_p2p),
		.iface_limits = sta_p2p,
	},
#if (CFG_SUPPORT_NAN == 1)
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_nan),
		.iface_limits = sta_nan,
	},
#endif
};
#elif (defined(MT6653) && (CFG_WIFI_DX4 == 1) && \
	(CONFIG_BAND_NUM == 2)) /* DX4 2G2A */ || \
	defined(MT6639) /* DX2/DX3 */ || \
	(CFG_SUPPORT_CONNAC2X == 1) /* Legacy */
struct mtk_wifi_iface_combination mtk_ifaces_combinations[] = {
#if (KAL_AIS_NUM != 1)
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_sta),
		.iface_limits = sta_sta,
	},
#endif
	{
		.max_ifaces = 1,
		.num_iface_limits = ARRAY_SIZE(ap_ap),
		.iface_limits = ap_ap,
	},
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_ap),
		.iface_limits = sta_ap,
	},
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_p2p),
		.iface_limits = sta_p2p,
	},
#if (CFG_SUPPORT_NAN == 1)
#if (CFG_NAN_CONCURRENCY == 1) && (CFG_EXT_FEATURE == 1)
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_ap_nan),
		.iface_limits = sta_ap_nan,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_p2p_nan),
		.iface_limits = sta_p2p_nan,
	},
#else
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_nan),
		.iface_limits = sta_nan,
	},
#endif /* (CFG_NAN_CONCURRENCY == 1) */
#endif /* (CFG_SUPPORT_NAN == 1) */
};
#elif defined(MT6653) && (CFG_WIFI_DX4 == 1) && \
	(CONFIG_BAND_NUM == 3) /* DX4 triband */
struct mtk_wifi_iface_combination mtk_ifaces_combinations[] = {
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_sta_sta),
		.iface_limits = sta_sta_sta,
	},
	{
		.max_ifaces = 1,
		.num_iface_limits = ARRAY_SIZE(ap_ap),
		.iface_limits = ap_ap,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_p2p_p2p),
		.iface_limits = sta_p2p_p2p,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_ap_p2p),
		.iface_limits = sta_ap_p2p,
	},
#if (CFG_SUPPORT_NAN == 1)
	{
		.max_ifaces = 2,
		.num_iface_limits = ARRAY_SIZE(sta_nan),
		.iface_limits = sta_nan,
	},
#endif
};
#else /* New chip default setting (from CONNAC3 DX5) */
struct mtk_wifi_iface_combination mtk_ifaces_combinations[] = {
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_sta_sta),
		.iface_limits = sta_sta_sta,
	},
	{
		.max_ifaces = 1,
		.num_iface_limits = ARRAY_SIZE(ap_ap),
		.iface_limits = ap_ap,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_p2p_p2p),
		.iface_limits = sta_p2p_p2p,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_ap_p2p),
		.iface_limits = sta_ap_p2p,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_sta_p2p),
		.iface_limits = sta_sta_p2p,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_sta_ap),
		.iface_limits = sta_sta_ap,
	},
#if (CFG_NAN_CONCURRENCY == 1)
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_ap_nan),
		.iface_limits = sta_ap_nan,
	},
	{
		.max_ifaces = 3,
		.num_iface_limits = ARRAY_SIZE(sta_p2p_nan),
		.iface_limits = sta_p2p_nan,
	},
#endif /* (CFG_NAN_CONCURRENCY == 1) */
};
#endif

struct mtk_wifi_iface_concurrency_matrix mtk_ifaces_matrix = {
	.num_iface_combinations = ARRAY_SIZE(mtk_ifaces_combinations),
	.iface_combinations = mtk_ifaces_combinations,
};

int mtk_cfg80211_vendor_get_chip_concurrency_matrix(struct wiphy *wiphy,
		struct wireless_dev *wdev, const void *data, int data_len)
{
#define DEBUG_BUFFER_SZ		512

	struct mtk_wifi_iface_concurrency_matrix *src = &mtk_ifaces_matrix;
	struct wifi_iface_concurrency_matrix *dest = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct sk_buff *skb;
	uint8_t i, j;
	uint8_t buffer[DEBUG_BUFFER_SZ];
	int32_t written = 0;
	static const uint32_t WLAN_DRV_READY =
		WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND |
		WLAN_DRV_READY_CHECK_RESET;

	if (!wiphy || !wdev) {
		DBGLOG(REQ, ERROR,
			"wiphy=0x%p, wdev=0x%p\n",
			wiphy, wdev);
		return -EINVAL;
	}

	prGlueInfo = wlanGetGlueInfo();
	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY)) {
		DBGLOG(REQ, ERROR, "driver is not ready\n");
		return -EFAULT;
	}

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(*dest));

	if (!skb) {
		DBGLOG(REQ, ERROR,
			"cfg80211_vendor_cmd_alloc_reply_skb failed(%zu)\n",
			sizeof(*dest));
		goto nla_put_failure;
	}

	dest = (struct wifi_iface_concurrency_matrix *)
		kalMemAlloc(sizeof(*dest), VIR_MEM_TYPE);
	if (!dest) {
		DBGLOG(REQ, ERROR,
			"alloc matrix failed(%zu)\n",
			sizeof(*dest));
		goto nla_put_failure;
	}
	kalMemZero(dest, sizeof(*dest));
	kalMemZero(buffer, sizeof(buffer));

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	if (!prChipInfo) {
		DBGLOG(REQ, ERROR, "prChipInfo=0x%p\n",	prChipInfo);
		goto nla_put_failure;
	}

	dest->num_iface_combinations = src->num_iface_combinations;
	written += kalSnprintf(buffer + written,
			       sizeof(buffer) - written,
			       "combinations=%u\n",
			       dest->num_iface_combinations);
	for (i = 0; i < src->num_iface_combinations; i++) {
		struct mtk_wifi_iface_combination *src_comb =
			&src->iface_combinations[i];
		struct wifi_iface_combination *dest_comb =
			&dest->iface_combinations[i];

		dest_comb->max_ifaces = src_comb->max_ifaces;
		dest_comb->num_iface_limits = src_comb->num_iface_limits;
		written += kalSnprintf(buffer + written,
				       sizeof(buffer) - written,
				       "\t[%u] max=%u limits=%u,",
				       i,
				       dest_comb->max_ifaces,
				       dest_comb->num_iface_limits);
		for (j = 0; j < src_comb->num_iface_limits; j++) {
			struct wifi_iface_limit *src_limit =
				&src_comb->iface_limits[j];
			struct wifi_iface_limit *dest_limit =
				&dest_comb->iface_limits[j];

			dest_limit->max_limit = src_limit->max_limit;
			dest_limit->iface_mask = src_limit->iface_mask;
			written += kalSnprintf(buffer + written,
					       sizeof(buffer) - written,
					       " [%u, max=%u mask=%u]",
					       j,
					       dest_limit->max_limit,
					       dest_limit->iface_mask);
		}
		written += kalSnprintf(buffer + written,
				       sizeof(buffer) - written,
				       "\n");
	}
	DBGLOG(REQ, DEBUG, "%s", buffer);

	if (unlikely(nla_put(skb, WIFI_ATTRIBUTE_CONCURRENCY_MATRIX,
			      sizeof(*dest), (void *)dest) < 0))
		goto nla_put_failure;

	kalMemFree(dest, VIR_MEM_TYPE, sizeof(*dest));

	return cfg80211_vendor_cmd_reply(skb);

nla_put_failure:
	if (skb)
		kfree_skb(skb);

	if (dest)
		kalMemFree(dest, VIR_MEM_TYPE, sizeof(*dest));

	return -EINVAL;
}

