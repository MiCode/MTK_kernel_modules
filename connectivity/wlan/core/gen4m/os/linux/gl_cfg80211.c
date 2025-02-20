// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_cfg80211.c
 *    \brief  Main routines for supporintg MT6620 cfg80211 control interface
 *
 *    This file contains the support routines of Linux driver for MediaTek Inc.
 *    802.11 Wireless LAN Adapters.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"
#include "precomp.h"
#include <linux/can/netlink.h>
#include <net/netlink.h>
#include <net/cfg80211.h>
#include "gl_cfg80211.h"
#include "gl_vendor.h"
#include "gl_p2p_os.h"
#include "wlan_lib.h"
#include "gl_cmd_validate.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

extern const struct net_device_ops wlan_netdev_ops;

#if CFG_SUPPORT_WAPI
#define KEY_BUF_SIZE	1024
#endif

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
static const uint32_t arBwCfg80211Table[] = {
	RATE_INFO_BW_20,
	RATE_INFO_BW_40,
	RATE_INFO_BW_80,
	RATE_INFO_BW_160,
#if (CFG_MTK_ANDROID_WMT == 1 && \
		KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE) || \
	KERNEL_VERSION(5, 18, 0) <= LINUX_VERSION_CODE
	RATE_INFO_BW_320
#endif
};

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

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for change STA type between
 *        1. Infrastructure Client (Non-AP STA)
 *        2. Ad-Hoc IBSS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int
mtk_cfg80211_change_iface(struct wiphy *wiphy,
			  struct net_device *ndev, enum nl80211_iftype type,
			  u32 *flags, struct vif_params *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_OP_MODE rOpMode;
	uint32_t u4BufLen;
	struct GL_WPA_INFO *prWpaInfo;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	if (type == NL80211_IFTYPE_STATION)
		rOpMode.eOpMode = NET_TYPE_INFRA;
	else if (type == NL80211_IFTYPE_ADHOC)
		rOpMode.eOpMode = NET_TYPE_IBSS;
	else
		return -EINVAL;
	rOpMode.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode,
		(void *)&rOpMode, sizeof(struct PARAM_OP_MODE), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "set infrastructure mode error:%x\n",
		       rStatus);

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		ucBssIndex);

	/* reset wpa info */
	prWpaInfo->u4WpaVersion =
		IW_AUTH_WPA_VERSION_DISABLED;
	prWpaInfo->u4KeyMgmt = 0;
	prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_NONE;
	prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_NONE;
	prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
#if CFG_SUPPORT_802_11W
	prWpaInfo->u4Mfp = RSN_AUTH_MFP_DISABLED;
	prWpaInfo->ucRSNMfpCap = 0;
#endif

	ndev->ieee80211_ptr->iftype = type;

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for adding key
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int
mtk_cfg80211_add_key(struct wiphy *wiphy,
		     struct net_device *ndev, int link_id,
		     u8 key_index, bool pairwise, const u8 *mac_addr,
		     struct key_params *params)
{
	struct PARAM_KEY rKey;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Rslt = -EINVAL;
	uint32_t u4BufLen = 0;
	uint8_t tmp1[8], tmp2[8];
	uint8_t ucBssIndex = 0;
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	/* const UINT_8 aucZeroMacAddr[] = NULL_MAC_ADDR; */
#if CFG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD
	struct PARAM_KEY rDupKey;
	struct BSS_INFO *prBssInfo;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

#if 1
	if (mac_addr) {
		DBGLOG(RSN, INFO,
		       "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
		       key_index, pairwise, MAC2STR(mac_addr));
	} else {
		DBGLOG(RSN, INFO, "keyIdx = %d pairwise = %d null mac\n",
		       key_index, pairwise);
	}
	DBGLOG(RSN, INFO, "Cipher = %x\n", params->cipher);
	DBGLOG_MEM8(RSN, INFO, params->key, params->key_len);
#endif

	kalMemZero(&rKey, sizeof(struct PARAM_KEY));

	rKey.u4KeyIndex = key_index;

	if (params->cipher) {
		switch (params->cipher) {
		case WLAN_CIPHER_SUITE_WEP40:
			rKey.ucCipher = CIPHER_SUITE_WEP40;
			break;
		case WLAN_CIPHER_SUITE_WEP104:
			rKey.ucCipher = CIPHER_SUITE_WEP104;
			break;
#if 0
		case WLAN_CIPHER_SUITE_WEP128:
			rKey.ucCipher = CIPHER_SUITE_WEP128;
			break;
#endif
		case WLAN_CIPHER_SUITE_TKIP:
			rKey.ucCipher = CIPHER_SUITE_TKIP;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			rKey.ucCipher = CIPHER_SUITE_CCMP;
			break;
		case WLAN_CIPHER_SUITE_GCMP:
			rKey.ucCipher = CIPHER_SUITE_GCMP_128;
			break;
#if 0
		case WLAN_CIPHER_SUITE_CCMP_256:
			rKey.ucCipher = CIPHER_SUITE_CCMP256;
			break;
#endif
		case WLAN_CIPHER_SUITE_SMS4:
			rKey.ucCipher = CIPHER_SUITE_WPI;
			break;
		case WLAN_CIPHER_SUITE_AES_CMAC:
			rKey.ucCipher = CIPHER_SUITE_BIP_CMAC_128;
			break;
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		case WLAN_CIPHER_SUITE_GCMP_256:
			rKey.ucCipher = CIPHER_SUITE_GCMP_256;
			break;
		case WLAN_CIPHER_SUITE_BIP_GMAC_256:
			DBGLOG(RSN, INFO,
				"[BIP-GMAC-256] save IGTK and handle integrity check ...\n");
			rKey.ucCipher = CIPHER_SUITE_BIP_GMAC_256;
			break;
#endif
		default:
			ASSERT(FALSE);
		}
	}

	if (pairwise) {
		ASSERT(mac_addr);
		rKey.u4KeyIndex |= BIT(31);
		rKey.u4KeyIndex |= BIT(30);
		COPY_MAC_ADDR(rKey.arBSSID, mac_addr);
	} else {		/* Group key */
		COPY_MAC_ADDR(rKey.arBSSID, aucBCAddr);
	}

	if (params->key) {
		if (params->key_len > sizeof(rKey.aucKeyMaterial))
			return -EINVAL;

		kalMemCopy(rKey.aucKeyMaterial, params->key,
			   params->key_len);
		if (rKey.ucCipher == CIPHER_SUITE_TKIP) {
			kalMemCopy(tmp1, &params->key[16], 8);
			kalMemCopy(tmp2, &params->key[24], 8);
			kalMemCopy(&rKey.aucKeyMaterial[16], tmp2, 8);
			kalMemCopy(&rKey.aucKeyMaterial[24], tmp1, 8);
		}
	}

	rKey.ucBssIdx = ucBssIndex;
	rKey.i4LinkId = link_id;
	rKey.u4KeyLength = params->key_len;
	rKey.u4Length = OFFSET_OF(struct PARAM_KEY, aucKeyMaterial)
				+ rKey.u4KeyLength;

	if (params->seq_len) {
		DBGLOG(RSN, INFO, "Dump IPN if given\n");
		DBGLOG_MEM8(RSN, INFO, params->seq, params->seq_len);
		if (params->seq_len == 6) /* IGTK Package Number */
			kalMemCopy(rKey.aucKeyPn, params->seq, params->seq_len);
	}

#if CFG_SUPPORT_DUAL_WTBL_GTK_REKEY_OFFLOAD
	 prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
			ucBssIndex);

	DBGLOG(RSN, INFO, "ucBMCWlanIndexSUsed[1]=%u [2]=%u\n",
		prBssInfo->ucBMCWlanIndexSUsed[1],
		prBssInfo->ucBMCWlanIndexSUsed[2]);

	if (pairwise == FALSE
			&& prBssInfo->ucBMCWlanIndexSUsed[1] == FALSE
			&& prBssInfo->ucBMCWlanIndexSUsed[2] == FALSE) {
		/* if this is the first time add gtk */
		if ((rKey.u4KeyIndex == 1) || (rKey.u4KeyIndex == 2)) {
			kalMemCopy(&rDupKey, &rKey, sizeof(struct PARAM_KEY));
			rDupKey.u4KeyIndex = rKey.u4KeyIndex ^ 0x03;
			rStatus = kalIoctl(prGlueInfo, wlanoidSetAddKey,
				&rDupKey,
				rDupKey.u4Length,
				&u4BufLen);
		} else {
			DBGLOG(RSN, ERROR, "GTK key id is %u\n",
				rKey.u4KeyIndex);
		}
	}
#endif

	rStatus = kalIoctl(prGlueInfo, wlanoidSetAddKey, &rKey,
			   rKey.u4Length, &u4BufLen);

	if (rStatus == WLAN_STATUS_SUCCESS)
		i4Rslt = 0;

	return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for getting key for specified STA
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int
mtk_cfg80211_get_key(struct wiphy *wiphy,
		     struct net_device *ndev, int link_id,
		     u8 key_index,
		     bool pairwise,
		     const u8 *mac_addr, void *cookie,
		     void (*callback)(void *cookie, struct key_params *)
		    )
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

#if 1
	DBGLOG(INIT, INFO, "--> %s()\n", __func__);
#endif

	/* not implemented */

	return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for removing key for specified STA
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_del_key(struct wiphy *wiphy,
			 struct net_device *ndev, int link_id,
			 u8 key_index, bool pairwise,
			 const u8 *mac_addr)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BSS_INFO *prBssInfo;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct PARAM_REMOVE_KEY rRemoveKey;
	uint32_t u4BufLen = 0;
	int32_t i4Rslt = -EINVAL;
	uint8_t ucBssIndex = 0;
	uint32_t waitRet = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (g_u4HaltFlag) {
		DBGLOG_LIMITED(RSN, WARN, "wlan is halt, skip key deletion\n");
		return WLAN_STATUS_FAILURE;
	}

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	if (mac_addr) {
		DBGLOG(RSN, TRACE,
		       "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
		       key_index, pairwise, MAC2STR(mac_addr));
	} else {
		DBGLOG(RSN, TRACE,
			"keyIdx = %d pairwise = %d null mac\n",
		       key_index, pairwise);
	}

	kalMemZero(&rRemoveKey, sizeof(struct PARAM_REMOVE_KEY));
	rRemoveKey.u4KeyIndex = key_index;
	rRemoveKey.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	if (mac_addr) {
		COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
		rRemoveKey.u4KeyIndex |= BIT(30);
	}

	if (prGlueInfo->prAdapter == NULL)
		return i4Rslt;

	rRemoveKey.ucBssIdx = ucBssIndex;
	rRemoveKey.i4LinkId = link_id;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
		ucBssIndex);
#if CFG_SUPPORT_802_11W
	/* if encrypted deauth frame is in process, pending remove key */
	if (IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex) &&
	    aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex)
				->encryptedDeauthIsInProcess == TRUE) {
		waitRet = wait_for_completion_timeout(
			&aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex)
								->rDeauthComp,
			MSEC_TO_JIFFIES(1000));
		if (!waitRet) {
			DBGLOG(RSN, WARN, "timeout\n");
			aisGetAisFsmInfo(prGlueInfo->prAdapter, ucBssIndex)
					->encryptedDeauthIsInProcess = FALSE;
		} else
			DBGLOG(RSN, INFO, "complete\n");
	}
#endif
	rStatus = kalIoctl(prGlueInfo, wlanoidSetRemoveKey, &rRemoveKey,
			rRemoveKey.u4Length, &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG_LIMITED(RSN, WARN, "remove key error:%x\n", rStatus);
	else
		i4Rslt = 0;

	return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting default key on an interface
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int
mtk_cfg80211_set_default_key(struct wiphy *wiphy,
		     struct net_device *ndev, int link_id,
		     u8 key_index, bool unicast,
		     bool multicast)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_DEFAULT_KEY rDefaultKey;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Rst = -EINVAL;
	uint32_t u4BufLen = 0;
	u_int8_t fgDef = FALSE, fgMgtDef = FALSE;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* For STA, should wep set the default key !! */
 #if DBG
	DBGLOG(RSN, INFO,
	       "keyIdx = %d unicast = %d multicast = %d\n", key_index,
	       unicast, multicast);
#endif

	rDefaultKey.ucKeyID = key_index;
	rDefaultKey.ucUnicast = unicast;
	rDefaultKey.ucMulticast = multicast;
	if (rDefaultKey.ucUnicast && !rDefaultKey.ucMulticast)
		return WLAN_STATUS_SUCCESS;

	if (rDefaultKey.ucUnicast && rDefaultKey.ucMulticast)
		fgDef = TRUE;

	if (!rDefaultKey.ucUnicast && rDefaultKey.ucMulticast)
		fgMgtDef = TRUE;

	rDefaultKey.ucBssIdx = ucBssIndex;
	rDefaultKey.i4LinkId = link_id;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetDefaultKey, &rDefaultKey,
				sizeof(struct PARAM_DEFAULT_KEY), &u4BufLen);
	if (rStatus == WLAN_STATUS_SUCCESS)
		i4Rst = 0;

	return i4Rst;
}

#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for getting tx rate from LLS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
static uint32_t wlanGetTxRateFromLinkStats(
	struct GLUE_INFO *prGlueInfo, uint32_t *pu4TxRate,
	uint32_t *pu4TxBw, uint8_t ucBssIndex)
{
	uint32_t rStatus = WLAN_STATUS_NOT_SUPPORTED;
	uint32_t u4MaxTxRate, u4Nss;
	union {
		struct CMD_GET_STATS_LLS cmd;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
		struct UNI_EVENT_BSS_TX_RATE arTlv[MAX_BSSID_NUM];
#else
		struct EVENT_STATS_LLS_TX_RATE_INFO rate_info;
#endif
	} query = {0};
	uint32_t u4QueryBufLen;
	uint32_t u4QueryInfoLen;
	struct _STATS_LLS_TX_RATE_INFO *target;
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	uint16_t offset = 0;
	uint8_t *tag = NULL;
#endif

	if (unlikely(ucBssIndex >= MAX_BSSID_NUM))
		return WLAN_STATUS_FAILURE;

	kalMemZero(&query, sizeof(query));
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	query.cmd.u4Tag = STATS_LLS_TAG_BSS_CURRENT_TX_RATE;
#else
	query.cmd.u4Tag = STATS_LLS_TAG_CURRENT_TX_RATE;
#endif
	u4QueryBufLen = sizeof(query);
	u4QueryInfoLen = sizeof(query.cmd);

	rStatus = kalIoctl(prGlueInfo,
			wlanQueryLinkStats,
			&query,
			u4QueryBufLen,
			&u4QueryInfoLen);
	DBGLOG(REQ, INFO, "kalIoctl=%x, %u bytes",
				rStatus, u4QueryInfoLen);

	if (unlikely(rStatus != WLAN_STATUS_SUCCESS)) {
		DBGLOG(REQ, INFO, "wlanQueryLinkStats return fail\n");
		return rStatus;
	}

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	if (unlikely(u4QueryInfoLen > (sizeof(struct UNI_EVENT_BSS_TX_RATE)
		* MAX_BSSID_NUM))) {
		DBGLOG(REQ, INFO, "wlanQueryLinkStats return len unexpected\n");
		return WLAN_STATUS_FAILURE;
	}

	tag = (uint8_t *) query.arTlv;
	TAG_FOR_EACH(tag, u4QueryInfoLen, offset) {
		DBGLOG_HEX(REQ, INFO, tag, TAG_LEN(tag));
		switch (TAG_ID(tag)) {
		case UNI_EVENT_STATISTICS_TAG_BSS_CURRENT_TX_RATE: {
			struct UNI_EVENT_BSS_TX_RATE *tlv =
				(struct UNI_EVENT_BSS_TX_RATE *)tag;
			if (tlv->ucBssIdx == ucBssIndex)
				target = &tlv->rTxRateInfo;
			break;
		}
		default:
			DBGLOG(REQ, WARN, "invalid tag:%u", TAG_ID(tag));
			break;
		}
	}
#else
	if (unlikely(u4QueryInfoLen != sizeof(
		struct EVENT_STATS_LLS_TX_RATE_INFO))) {
		DBGLOG(REQ, INFO, "wlanQueryLinkStats return len unexpected\n");
		return WLAN_STATUS_FAILURE;
	}

	target = &query.rate_info.arTxRateInfo[ucBssIndex];
#endif
	if (!target)
		return WLAN_STATUS_FAILURE;

	if (target->bw >= ARRAY_SIZE(arBwCfg80211Table)) {
		DBGLOG(REQ, WARN, "wrong tx bw!");
		return WLAN_STATUS_FAILURE;
	}

	*pu4TxBw = arBwCfg80211Table[target->bw];
	target->nsts += 1;
	if (target->nsts == 1)
		u4Nss = target->nsts;
	else
		u4Nss = target->stbc ?
			(target->nsts >> 1)
			: target->nsts;

	wlanQueryRateByTable(target->mode,
		target->rate, target->bw, 0,
		u4Nss, pu4TxRate, &u4MaxTxRate);
	DBGLOG(REQ, INFO, "rate=%u mode=%u nss=%u stbc=%u bw=%u linkspeed=%u\n",
		target->rate, target->mode,
		u4Nss, target->stbc,
		*pu4TxBw, *pu4TxRate);

	return rStatus;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for getting station information such as
 *        RSSI
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_get_station(struct wiphy *wiphy,
			     struct net_device *ndev, const u8 *mac,
			     struct station_info *sinfo)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	uint32_t rStatus;
	uint32_t u4BufLen = 0, u4TxRate = 0, u4RxRate = 0, u4RxBw = 0;
	int32_t i4Rssi = 0;

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	struct PARAM_GET_STATS_ONE_CMD rParam;
	uint32_t u4QueryInfoLen;
	struct LINK_SPEED_EX_ *prLq;
#else
	struct PARAM_LINK_SPEED_EX rLinkSpeed = {0};
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	uint32_t u4TxBw = 0;
#endif
#endif
	struct PARAM_GET_STA_STATISTICS *prGetStaStats;
	uint32_t u4TotalError;
	uint32_t u4FcsError = 0;
	struct net_device_stats *prDevStats;
	uint8_t ucBssIndex = 0;
	struct PARAM_LINK_BSS_INFO rLinkBss = {0};
	struct BSS_INFO *prBssInfo;
	uint8_t ucBandIdx = 0;
	struct MIB_INFO_STAT *prMibInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;

	ucBssIndex = wlanGetBssIdx(ndev);
	if (unlikely(ucBssIndex >= MAX_BSSID_NUM ||
	    !IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)))
		return -EINVAL;

	rLinkBss.ucBssIndex = ucBssIndex;
	COPY_MAC_ADDR(rLinkBss.aucMacAddr, mac);

	/* get the link bssIdx if mac is one of the AIS MLO link */
	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidQueryLinkBssInfo,
			&rLinkBss, sizeof(rLinkBss), &u4BufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS || u4BufLen != sizeof(rLinkBss))
		return -ENOENT;

	ucBssIndex = rLinkBss.ucBssIndex;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return -EINVAL;

	ucBandIdx = prBssInfo->eHwBandIdx;
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	prGetStaStats = &prAdapter->rQueryStaStatistics[ucBssIndex];
	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];
	/* no need to COPY_MAC_ADDR here
	 * because main thread will traverse all BSS index
	 */
#else
	prGetStaStats = &(
		prAdapter->rQueryStaStatistics);
	COPY_MAC_ADDR(prGetStaStats->aucMacAddr, mac);
#endif
	prGetStaStats->ucReadClear = TRUE;
	if (ucBandIdx < ENUM_BAND_NUM) {
		prMibInfo = &g_arMibInfo[ucBandIdx];
	}
	/* 2. fill TX/RX rate */
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(REQ, WARN, "not yet connected\n");
		return -EINVAL;
	}

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	rParam.u4Period = CFG_STATS_ONE_CMD_PERIOD;
	/* query linkspeed and sta_statistics in one unified cmd */
	rStatus = kalIoctlByBssIdx(prGlueInfo,
		   wlanoidQueryStatsOneCmd, &rParam,
		   sizeof(rParam), &u4QueryInfoLen, ucBssIndex);
	DBGLOG(REQ, TRACE, "kalIoctlByBssIdx()=%u, prGlueInfo=%p",
		rStatus, prGlueInfo);
#else
	DBGLOG(REQ, TRACE, "Call LinkSpeed=%p, size=%zu, &u4BufLen=%p",
		&rLinkSpeed, sizeof(rLinkSpeed), &u4BufLen);
	rStatus = kalIoctlByBssIdx(prGlueInfo,
				   wlanoidQueryLinkSpeed, &rLinkSpeed,
				   sizeof(rLinkSpeed),
				   &u4BufLen, ucBssIndex);
	DBGLOG(REQ, TRACE, "kalIoctlByBssIdx()=%u, prGlueInfo=%p, u4BufLen=%u",
		rStatus, prGlueInfo, u4BufLen);
#endif

#if CFG_REPORT_MAX_TX_RATE
	/*rewrite LinkSpeed with Max LinkSpeed*/
	rStatus = kalIoctlByBssIdx(prGlueInfo,
			       wlanoidQueryMaxLinkSpeed,
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
			       &prAdapter->rLinkQuality,
#else
			       &rLinkSpeed,
#endif
			       sizeof(struct PARAM_LINK_SPEED_EX),
			       &u4BufLen, ucBssIndex);
#endif /* CFG_REPORT_MAX_TX_RATE */

	if (rStatus == WLAN_STATUS_SUCCESS) {
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
		u4TxRate = prLq->u2TxLinkSpeed;
		u4RxRate = prLq->u2RxLinkSpeed;
		i4Rssi = prLq->cRssi;
		u4RxBw = prLq->u4RxBw;
#else
		u4TxRate = rLinkSpeed.rLq[ucBssIndex].u2TxLinkSpeed;
		u4RxRate = rLinkSpeed.rLq[ucBssIndex].u2RxLinkSpeed;
		i4Rssi = rLinkSpeed.rLq[ucBssIndex].cRssi;
		u4RxBw = rLinkSpeed.rLq[ucBssIndex].u4RxBw;
#endif
		if (unlikely(u4RxBw >= ARRAY_SIZE(arBwCfg80211Table))) {
			DBGLOG(REQ, WARN, "wrong u4RxBw!");
			u4RxBw = 0;
		}
	}
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
	sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE);
	sinfo->filled |= BIT(NL80211_STA_INFO_RX_BITRATE);
	sinfo->filled |= BIT(NL80211_STA_INFO_SIGNAL);
#else
	sinfo->filled |= STATION_INFO_TX_BITRATE;
	sinfo->filled |= STATION_INFO_RX_BITRATE;
	sinfo->filled |= STATION_INFO_SIGNAL;
#endif

	/* change to unit of 100 kbps */
	/*
	 *  FW report in 500kbps because u2TxLinkSpeed is 16 bytes
	 *  TODO:
	 *    driver and fw should change u2TxLinkSpeed to u4
	 *    because it will overflow in wifi7
	 */
	if ((rStatus != WLAN_STATUS_SUCCESS) || (u4TxRate == 0) ||
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
		!prLq->fgIsLinkRateValid) {
#else
		!rLinkSpeed.rLq[ucBssIndex].fgIsLinkRateValid) {
#endif
		/* unable to retrieve link speed */
		DBGLOG(REQ, WARN, "last Tx link speed\n");
	} else {
		/* convert from 100bps to 100kbps */
		prGlueInfo->u4TxLinkSpeedCache[ucBssIndex] = u4TxRate / 1000;
	}

	if ((rStatus != WLAN_STATUS_SUCCESS) || (u4RxRate == 0) ||
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
		!prLq->fgIsLinkRateValid) {
#else
		!rLinkSpeed.rLq[ucBssIndex].fgIsLinkRateValid) {
#endif
		/* unable to retrieve link speed */
		DBGLOG(REQ, WARN, "last Rx link speed\n");
	} else {
		/* convert from 100bps to 100kbps */
		prGlueInfo->u4RxLinkSpeedCache[ucBssIndex] = u4RxRate / 1000;
		prGlueInfo->u4RxBwCache[ucBssIndex] =
			arBwCfg80211Table[u4RxBw];
	}

	/* if there is no valid RSSI from fw when we
	 * query the RSSI for the first time after
	 * connection, use the scan result
	 */
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	if (!prLq->fgIsLinkRateValid) {
#else
	if (!rLinkSpeed.rLq[ucBssIndex].fgIsLinkRateValid) {
#endif
		/* use the scan RSSI */
		struct BSS_DESC *prBssDesc =
			scanSearchBssDescByBssid(prAdapter, (uint8_t *)mac);

		if (prBssDesc) {
			i4Rssi = RCPI_TO_dBm(prBssDesc->ucRCPI);
			DBGLOG(REQ, WARN,
				"LR invalid, use scan result:%d\n", i4Rssi);
		}
	}

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN,
			"Query RSSI failed, use last RSSI %d\n",
			prGlueInfo->i4RssiCache[ucBssIndex]);
		sinfo->signal = prGlueInfo->i4RssiCache[ucBssIndex] ?
			prGlueInfo->i4RssiCache[ucBssIndex] :
			PARAM_WHQL_RSSI_INITIAL_DBM;
	} else if (i4Rssi <= PARAM_WHQL_RSSI_MIN_DBM ||
			i4Rssi >= PARAM_WHQL_RSSI_MAX_DBM) {
		DBGLOG(REQ, WARN,
			"RSSI abnormal %d, use last RSSI %d\n",
			i4Rssi,
			prGlueInfo->i4RssiCache[ucBssIndex]);
		sinfo->signal = prGlueInfo->i4RssiCache[ucBssIndex] ?
			prGlueInfo->i4RssiCache[ucBssIndex] : i4Rssi;
	} else {
		sinfo->signal = i4Rssi;	/* dBm */
		prGlueInfo->i4RssiCache[ucBssIndex] = i4Rssi;
	}

#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	sinfo->txrate.bw =
		arBwCfg80211Table[prGlueInfo->u4TxBwCache[ucBssIndex]];
#else
	rStatus = wlanGetTxRateFromLinkStats(prGlueInfo, &u4TxRate,
			&u4TxBw, ucBssIndex);
	if (rStatus == WLAN_STATUS_SUCCESS)
		prGlueInfo->u4TxBwCache[ucBssIndex] = u4TxBw;
	sinfo->txrate.bw =
		prGlueInfo->u4TxBwCache[ucBssIndex];
#endif
#endif

	sinfo->txrate.legacy =
		prGlueInfo->u4TxLinkSpeedCache[ucBssIndex];
	sinfo->rxrate.legacy =
		prGlueInfo->u4RxLinkSpeedCache[ucBssIndex];

	sinfo->rxrate.bw =
		prGlueInfo->u4RxBwCache[ucBssIndex];

	/* Get statistics from net_dev */
	prDevStats = (struct net_device_stats *)kalGetStats(ndev);

	if (prDevStats) {
		/* 4. fill RX_PACKETS */
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT(NL80211_STA_INFO_RX_PACKETS);
		sinfo->filled |= BIT(NL80211_STA_INFO_RX_BYTES64);
#else
		sinfo->filled |= STATION_INFO_RX_PACKETS;
		sinfo->filled |= NL80211_STA_INFO_RX_BYTES64;
#endif
		sinfo->rx_packets = prDevStats->rx_packets;
		sinfo->rx_bytes = prDevStats->rx_bytes;

		/* 5. fill TX_PACKETS */
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT(NL80211_STA_INFO_TX_PACKETS);
		sinfo->filled |= BIT(NL80211_STA_INFO_TX_BYTES64);
#else
		sinfo->filled |= STATION_INFO_TX_PACKETS;
		sinfo->filled |= NL80211_STA_INFO_TX_BYTES64;
#endif

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
		sinfo->tx_packets = prGetStaStats->u4TxDataCount;
#else
		sinfo->tx_packets = prDevStats->tx_packets;
#endif
		sinfo->tx_bytes = prDevStats->tx_bytes;

		/* 6. fill TX_FAILED */
#if (CFG_SUPPORT_STATS_ONE_CMD == 0)
		rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidQueryStaStatistics,
				prGetStaStats,
				sizeof(*prGetStaStats),
				&u4BufLen, ucBssIndex);
#endif

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN,
			       "link speed=%u, rssi=%d, unable to retrieve link speed,status=%u\n",
			       sinfo->txrate.legacy, sinfo->signal, rStatus);
		} else {
			if (prMibInfo)
				u4FcsError = prMibInfo->u4FcsError;

			u4TotalError = prGetStaStats->u4TxFailCount +
				       prGetStaStats->u4TxLifeTimeoutCount;
			prDevStats->tx_errors = u4TotalError;

#define TEMP_LOG_TEMPLATE \
	"link speed=%u/%u, bw=%u/%u, rssi=%d, BSSID:[" MACSTR "], idx=%u," \
	"TxFail=%u, TxTimeOut=%u, TxOK=%u, RxOK=%u, FcsErr=%u\n"
			DBGLOG(REQ, INFO,
				TEMP_LOG_TEMPLATE,
				sinfo->txrate.legacy, sinfo->rxrate.legacy,
				sinfo->txrate.bw, sinfo->rxrate.bw,
				sinfo->signal,
				MAC2STR(mac),
				ucBssIndex,
				prGetStaStats->u4TxFailCount,
				prGetStaStats->u4TxLifeTimeoutCount,
				sinfo->tx_packets, sinfo->rx_packets,
				u4FcsError
			);
#undef TEMP_LOG_TEMPLATE
		}
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT(NL80211_STA_INFO_TX_FAILED);
#else
		sinfo->filled |= STATION_INFO_TX_FAILED;
#endif
		sinfo->tx_failed = prDevStats->tx_errors;
#if KERNEL_VERSION(4, 20, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT_ULL(NL80211_STA_INFO_FCS_ERROR_COUNT);
		sinfo->fcs_err_count = u4FcsError;
#endif
	}

	return 0;
}
#else
int mtk_cfg80211_get_station(struct wiphy *wiphy,
			     struct net_device *ndev, u8 *mac,
			     struct station_info *sinfo)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint8_t arBssid[PARAM_MAC_ADDR_LEN];
	uint32_t u4BufLen, u4Rate;
	int32_t i4Rssi;
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
	struct PARAM_LINK_SPEED_EX rLinkSpeed;
	uint32_t u4TotalError;
	struct net_device_stats *prDevStats;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		return -EINVAL;

	kalMemZero(arBssid, MAC_ADDR_LEN);
	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidQueryBssid,
			arBssid, sizeof(arBssid), &u4BufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS || u4BufLen != MAC_ADDR_LEN)
		return -EINVAL;

	/* 1. check BSSID */
	if (UNEQUAL_MAC_ADDR(arBssid, mac)) {
		/* wrong MAC address */
		DBGLOG(REQ, WARN,
		       "incorrect BSSID: [" MACSTR
		       "] currently connected BSSID["
		       MACSTR "]\n",
		       MAC2STR(mac), MAC2STR(arBssid));
		return -ENOENT;
	}

	/* 2. fill TX rate */
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(REQ, WARN, "not yet connected\n");
	} else {
#if CFG_REPORT_MAX_TX_RATE
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidQueryMaxLinkSpeed,
				&u4Rate, sizeof(u4Rate),
				&u4BufLen, ucBssIndex);
#else
		DBGLOG(REQ, TRACE, "Call LinkSpeed=%p sizeof=%zu &u4BufLen=%p",
			&rLinkSpeed, sizeof(rLinkSpeed), &u4BufLen);
		rStatus = kalIoctlByBssIdx(prGlueInfo,
					   wlanoidQueryLinkSpeed,
					   &rLinkSpeed, sizeof(rLinkSpeed),
					   &u4BufLen, ucBssIndex);
		DBGLOG(REQ, TRACE, "rStatus=%u, prGlueInfo=%p, u4BufLen=%u",
			rStatus, prGlueInfo, u4BufLen);
		if (ucBssIndex < MAX_BSSID_NUM)
			u4Rate = rLinkSpeed.rLq[ucBssIndex].u2TxLinkSpeed;
#endif /* CFG_REPORT_MAX_TX_RATE */

		sinfo->filled |= STATION_INFO_TX_BITRATE;

		if ((rStatus != WLAN_STATUS_SUCCESS) || (u4Rate == 0)) {
			/* unable to retrieve link speed */
			DBGLOG(REQ, WARN, "last link speed\n");
			sinfo->txrate.legacy =
				prGlueInfo->u4TxLinkSpeedCache[ucBssIndex];
		} else {
			/* convert from 100bps to 100kbps */
			sinfo->txrate.legacy = u4Rate / 1000;
			prGlueInfo->u4TxLinkSpeedCache[ucBssIndex] =
				u4Rate / 1000;
		}
	}

	/* 3. fill RSSI */
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(REQ, WARN, "not yet connected\n");
	} else {
		rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidQueryRssi,
				&rLinkSpeed, sizeof(rLinkSpeed),
				&u4BufLen, ucBssIndex);
		if (ucBssIndex < MAX_BSSID_NUM)
			i4Rssi = rLinkSpeed.rLq[ucBssIndex].cRssi;

		sinfo->filled |= STATION_INFO_SIGNAL;

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN,
				"Query RSSI failed, use last RSSI %d\n",
				prGlueInfo->i4RssiCache[ucBssIndex]);
			sinfo->signal = prGlueInfo->i4RssiCache[ucBssIndex] ?
				prGlueInfo->i4RssiCache[ucBssIndex] :
				PARAM_WHQL_RSSI_INITIAL_DBM;
		} else if (i4Rssi == PARAM_WHQL_RSSI_MIN_DBM ||
			i4Rssi == PARAM_WHQL_RSSI_MAX_DBM) {
			DBGLOG(REQ, WARN,
				"RSSI abnormal, use last RSSI %d\n",
				prGlueInfo->i4RssiCache[ucBssIndex]);
			sinfo->signal = prGlueInfo->i4RssiCache[ucBssIndex] ?
				prGlueInfo->i4RssiCache[ucBssIndex] : i4Rssi;
		} else {
			sinfo->signal = i4Rssi;	/* dBm */
			prGlueInfo->i4RssiCache[ucBssIndex] = i4Rssi;
		}
	}

	/* Get statistics from net_dev */
	prDevStats = (struct net_device_stats *)kalGetStats(ndev);
	if (prDevStats) {
		/* 4. fill RX_PACKETS */
		sinfo->filled |= STATION_INFO_RX_PACKETS;
		sinfo->rx_packets = prDevStats->rx_packets;

		/* 5. fill TX_PACKETS */
		sinfo->filled |= STATION_INFO_TX_PACKETS;
		sinfo->tx_packets = prDevStats->tx_packets;

		/* 6. fill TX_FAILED */
		kalMemZero(&rQueryStaStatistics,
			   sizeof(rQueryStaStatistics));
		COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr, arBssid);
		rQueryStaStatistics.ucReadClear = TRUE;

		rStatus = kalIoctl(prGlueInfo, wlanoidQueryStaStatistics,
				   &rQueryStaStatistics,
				   sizeof(rQueryStaStatistics), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN,
			       "link speed=%u, rssi=%d, unable to get sta statistics: status=%u\n",
			       sinfo->txrate.legacy, sinfo->signal, rStatus);
		} else {
			DBGLOG(REQ, INFO,
			       "link speed=%u, rssi=%d, BSSID=[" MACSTR
			       "], TxFailCount=%d, LifeTimeOut=%d\n",
			       sinfo->txrate.legacy, sinfo->signal,
			       MAC2STR(arBssid),
			       rQueryStaStatistics.u4TxFailCount,
			       rQueryStaStatistics.u4TxLifeTimeoutCount);

			u4TotalError = rQueryStaStatistics.u4TxFailCount +
				       rQueryStaStatistics.u4TxLifeTimeoutCount;
			prDevStats->tx_errors = u4TotalError;
		}
		sinfo->filled |= STATION_INFO_TX_FAILED;
		sinfo->tx_failed = prDevStats->tx_errors;
	}

	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to do a scan
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_scan(struct wiphy *wiphy,
		      struct cfg80211_scan_request *request)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t i, j = 0, u4BufLen;
	struct PARAM_SCAN_REQUEST_ADV *prScanRequest;
	uint32_t num_ssid = 0;
	uint32_t old_num_ssid = 0;
	uint32_t u4ValidIdx = 0;
	uint32_t wildcard_flag = 0;
#if (CFG_SUPPORT_QA_TOOL == 1) || (CFG_SUPPORT_LOWLATENCY_MODE == 1)
	struct ADAPTER *prAdapter = NULL;
#endif
	uint8_t ucBssIndex = 0;
#if (CFG_SUPPORT_QA_TOOL == 1)
	GLUE_SPIN_LOCK_DECLARATION();
#endif

	if (kalIsResetting())
		return -EBUSY;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "prGlueInfo is NULL");
		return -EINVAL;
	}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	if (prGlueInfo->fgIsEnableMon)
		return -EINVAL;
#endif
	ucBssIndex = wlanGetBssIdx(request->wdev->netdev);
	if (!IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		return -EINVAL;

#if (CFG_SUPPORT_QA_TOOL == 1) || (CFG_SUPPORT_LOWLATENCY_MODE == 1)
	prAdapter = prGlueInfo->prAdapter;
	if (prGlueInfo->prAdapter == NULL) {
		DBGLOG(REQ, ERROR, "prGlueInfo->prAdapter is NULL");
		return -EINVAL;
	}
#endif

	if (wlanIsChipAssert(prGlueInfo->prAdapter))
		return -EBUSY;

#if CFG_SUPPORT_RTT
	if (rttIsRunning(prAdapter)) {
		DBGLOG(REQ, ERROR, "RTT is running\n");
		return -EBUSY;
	}
#endif

#if (CFG_CE_ASSERT_DUMP == 1)
	if (prGlueInfo->prAdapter->fgN9AssertDumpOngoing)
		return -EBUSY;
#endif

#if CFG_SUPPORT_QA_TOOL
	if (prAdapter->fgTestMode) {
		DBGLOG(REQ, ERROR,
			"directly return scan done, TestMode running\n");

		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		kalCfg80211ScanDone(request, FALSE);
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		return 0;
	}
	if (prAdapter->rIcapInfo.eIcapState != ICAP_STATE_INIT) {
		DBGLOG(REQ, ERROR, "skip scan, ICAP In State(%d)\n",
			prAdapter->rIcapInfo.eIcapState);
		return -EBUSY;
	}
#endif

	kalScanReqLog(request);

	/* check if there is any pending scan/sched_scan not yet finished */
	if (prGlueInfo->prScanRequest != NULL) {
		DBGLOG(REQ, ERROR, "prGlueInfo->prScanRequest != NULL\n");
		return -EBUSY;
	}

#if CFG_SUPPORT_LOWLATENCY_MODE
	if (!prGlueInfo->prAdapter->fgEnCfg80211Scan
#if CFG_SUPPORT_SCAN_EXT_FLAG
		&& (prGlueInfo->u4ScanExtFlag != TRUE)
#endif
	    && MEDIA_STATE_CONNECTED
	    == kalGetMediaStateIndicated(prGlueInfo, ucBssIndex)) {
		DBGLOG(REQ, INFO,
		       "mtk_cfg80211_scan LowLatency reject scan\n");
		return -EBUSY;
	}
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */
#if CFG_SUPPORT_SCAN_EXT_FLAG
	prGlueInfo->u4ScanExtFlag = 0;
#endif
#if CFG_SUPPORT_SCAN_CACHE_RESULT
	prGlueInfo->scanCache.prGlueInfo = prGlueInfo;
	prGlueInfo->scanCache.prRequest = request;
	prGlueInfo->scanCache.n_channels = (uint32_t) request->n_channels;
	prGlueInfo->scanCache.ucBssIndex = ucBssIndex;
	prGlueInfo->scanCache.u4Flags = request->flags;
	if (isScanCacheDone(&prGlueInfo->scanCache) == TRUE)
		return 0;
#endif /* CFG_SUPPORT_SCAN_CACHE_RESULT */

	prScanRequest = kalMemAlloc(sizeof(struct PARAM_SCAN_REQUEST_ADV),
			VIR_MEM_TYPE);
	if (prScanRequest == NULL) {
		DBGLOG(REQ, ERROR, "alloc scan request fail\n");
		return -ENOMEM;

	}
	kalMemZero(prScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV));

	if (request->n_ssids == 0) {
		prScanRequest->u4SsidNum = 0;
		prScanRequest->ucScanType = SCAN_TYPE_PASSIVE_SCAN;
	} else if ((request->ssids) && (request->n_ssids > 0)
		   && (request->n_ssids <= (CFG_SCAN_SSID_MAX_NUM + 1))) {
		num_ssid = (uint32_t)request->n_ssids;
		old_num_ssid = (uint32_t)request->n_ssids;
		u4ValidIdx = 0;
		for (i = 0; i < request->n_ssids; i++) {
			if ((request->ssids[i].ssid[0] == 0)
			    || (request->ssids[i].ssid_len == 0)) {
				/* remove if this is a wildcard scan */
				num_ssid--;
				wildcard_flag |= (1 << i);
				DBGLOG(REQ, TRACE, "i=%d, wildcard scan\n", i);
				continue;
			}
			COPY_SSID(prScanRequest->rSsid[u4ValidIdx].aucSsid,
				prScanRequest->rSsid[u4ValidIdx].u4SsidLen,
				request->ssids[i].ssid,
				request->ssids[i].ssid_len);
			if (prScanRequest->rSsid[u4ValidIdx].u4SsidLen >
				ELEM_MAX_LEN_SSID) {
				prScanRequest->rSsid[u4ValidIdx].u4SsidLen =
				ELEM_MAX_LEN_SSID;
			}
			DBGLOG(REQ, STATE,
			       "i=%d, u4ValidIdx=%d, Ssid=%s, SsidLen=%d\n",
			       i, u4ValidIdx,
			       HIDE(prScanRequest->rSsid[u4ValidIdx].aucSsid),
			       prScanRequest->rSsid[u4ValidIdx].u4SsidLen);

			u4ValidIdx++;
			if (u4ValidIdx == CFG_SCAN_SSID_MAX_NUM) {
				DBGLOG(REQ, STATE, "CFG_SCAN_SSID_MAX_NUM\n");
				break;
			}
		}
		/* real SSID number to firmware */
		prScanRequest->u4SsidNum = u4ValidIdx;
		prScanRequest->ucScanType = SCAN_TYPE_ACTIVE_SCAN;
	} else {
		DBGLOG(REQ, ERROR, "request->n_ssids:%d\n",
		       request->n_ssids);
		kalMemFree(prScanRequest,
			   sizeof(struct PARAM_SCAN_REQUEST_ADV), VIR_MEM_TYPE);
		return -EINVAL;
	}

	/* 6G only need to scan PSC channel, transform channel list first*/
	for (i = 0; i < request->n_channels; i++) {
		uint32_t u4channel =
		nicFreq2ChannelNum(request->channels[i]->center_freq *
								1000);
		if (u4channel == 0) {
			DBGLOG(REQ, WARN, "Wrong Channel[%d] freq=%u\n",
			       i, request->channels[i]->center_freq);
			continue;
		}
		prScanRequest->arChannel[j].ucChannelNum = u4channel;
		switch ((request->channels[i])->band) {
		case KAL_BAND_2GHZ:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.
				fgEnOnlyScan6g))
				continue;
#endif
			prScanRequest->arChannel[j].eBand = BAND_2G4;
			break;
		case KAL_BAND_5GHZ:
#if (CFG_SUPPORT_WIFI_6G == 1)
			if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.
				fgEnOnlyScan6g))
				continue;
#endif
			prScanRequest->arChannel[j].eBand = BAND_5G;
			break;
#if (CFG_SUPPORT_WIFI_6G == 1)
		case KAL_BAND_6GHZ:
			/* 6g only scan PSC channel if OnlyScan6g not enabled */
			if (IS_FEATURE_DISABLED(prAdapter->rWifiVar.
				fgEnOnlyScan6g))
				if (((u4channel - 5) % 16) != 0)
					continue;

#if WLAN_INCLUDE_SYS
			/* Special case: cmd to block 6G */
			if (!prGlueInfo->prAdapter->fgIsHwSupport6G)
				continue;
#endif

			prScanRequest->arChannel[j].eBand = BAND_6G;
			break;
#endif
		default:
			DBGLOG(REQ, WARN, "UNKNOWN Band %d(chnl=%u)\n",
			       request->channels[i]->band,
			       u4channel);
			prScanRequest->arChannel[j].eBand = BAND_NULL;
			break;
		}
		j++;
	}
	prScanRequest->u4ChannelNum = j;

	/* Check if channel list > MAX support number */
	if (prScanRequest->u4ChannelNum > MAXIMUM_OPERATION_CHANNEL_LIST) {
		prScanRequest->u4ChannelNum = 0;
		DBGLOG(REQ, INFO,
		       "Channel list (%u->%u) exceed maximum support.\n",
		       request->n_channels,
		       prScanRequest->u4ChannelNum);
	}

	if (kalScanParseRandomMac(request->wdev->netdev,
		request, prScanRequest->aucRandomMac)) {
		prScanRequest->ucScnFuncMask |= ENUM_SCN_RANDOM_MAC_EN;
	}

	if (request->ie_len > 0 && request->ie_len < MAX_IE_LENGTH) {
		prScanRequest->u4IELength = request->ie_len;
		kalMemCopy(prScanRequest->aucIEBuf,
			(uint8_t *) request->ie, request->ie_len);
	}

#define TEMP_LOG_TEMPLATE "n_ssid=(%u->%u) n_channel(%u==>%u) " \
	"wildcard=0x%X flag=0x%x random_mac=" MACSTR "\n"
	DBGLOG(REQ, INFO, TEMP_LOG_TEMPLATE,
		request->n_ssids, num_ssid, request->n_channels,
		prScanRequest->u4ChannelNum, wildcard_flag,
		request->flags,
		MAC2STR(prScanRequest->aucRandomMac));
#undef TEMP_LOG_TEMPLATE

	prScanRequest->ucBssIndex = ucBssIndex;
	prScanRequest->u4Flags = request->flags;

	prGlueInfo->prScanRequest = request;
	rStatus = kalIoctl(prGlueInfo, wlanoidSetBssidListScanAdv,
			   prScanRequest, sizeof(struct PARAM_SCAN_REQUEST_ADV),
			   &u4BufLen);

	kalMemFree(prScanRequest, VIR_MEM_TYPE,
		   sizeof(struct PARAM_SCAN_REQUEST_ADV));
	if (rStatus != WLAN_STATUS_SUCCESS) {
		prGlueInfo->prScanRequest = NULL;
		DBGLOG(REQ, WARN, "scan error:%x\n", rStatus);
		return -EINVAL;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for abort an ongoing scan. The driver
 *        shall indicate the status of the scan through cfg80211_scan_done()
 *
 * @param wiphy - pointer of wireless hardware description
 *        wdev - pointer of  wireless device state
 *
 */
/*----------------------------------------------------------------------------*/
void mtk_cfg80211_abort_scan(struct wiphy *wiphy,
			     struct wireless_dev *wdev)
{
	uint32_t u4SetInfoLen = 0;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return;

	scanlog_dbg(LOG_SCAN_ABORT_REQ_K2D, INFO, "mtk_cfg80211_abort_scan\n");

	rStatus = kalIoctlByBssIdx(prGlueInfo,
			   wlanoidAbortScan,
			   NULL, 1, &u4SetInfoLen,
			   ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, ERROR, "wlanoidAbortScan fail 0x%x\n", rStatus);
}

int wlanParseAkmSuites(uint32_t *au4AkmSuites, uint32_t u4AkmSuitesCount,
	uint32_t u4WpaVersion, enum ENUM_PARAM_AUTH_MODE *prAuthMode,
	uint32_t *pu4AkmSuite, struct IEEE_802_11_MIB *prMib)
{
	enum ENUM_PARAM_AUTH_MODE eOriAuthMode = *prAuthMode;
	uint8_t i, j;
	struct DOT11_RSNA_CONFIG_AUTHENTICATION_SUITES_ENTRY *prEntry;

	for (i = 0;
#ifdef CFG80211_MAX_NUM_AKM_SUITES
		i < u4AkmSuitesCount && i < CFG80211_MAX_NUM_AKM_SUITES; i++) {
#else
		i < u4AkmSuitesCount && i < NL80211_MAX_NR_AKM_SUITES; i++) {
#endif
		uint32_t u4AkmSuite = 0;
		enum ENUM_PARAM_AUTH_MODE eAuthMode;

		if (u4WpaVersion == IW_AUTH_WPA_VERSION_DISABLED) {
			switch (au4AkmSuites[i]) {
			case WLAN_AKM_SUITE_OSEN:
				u4AkmSuite = RSN_AKM_SUITE_OSEN;
				break;
			default:
				break;
			}
		} else if (u4WpaVersion == IW_AUTH_WPA_VERSION_WPA ||
			u4WpaVersion == IW_AUTH_WPA_VERSION_WPA2) {
			switch (au4AkmSuites[i]) {
			case WLAN_AKM_SUITE_8021X:
				if (u4WpaVersion == IW_AUTH_WPA_VERSION_WPA)
					u4AkmSuite = WPA_AKM_SUITE_802_1X;
				else
					u4AkmSuite = RSN_AKM_SUITE_802_1X;
				break;
			case WLAN_AKM_SUITE_PSK:
				if (u4WpaVersion == IW_AUTH_WPA_VERSION_WPA)
					u4AkmSuite = WPA_AKM_SUITE_PSK;
				else
					u4AkmSuite = RSN_AKM_SUITE_PSK;
				break;
#if CFG_SUPPORT_802_11R
			case WLAN_AKM_SUITE_FT_8021X:
				u4AkmSuite = RSN_AKM_SUITE_FT_802_1X;
				break;
			case WLAN_AKM_SUITE_FT_PSK:
				u4AkmSuite = RSN_AKM_SUITE_FT_PSK;
				break;
			case WLAN_AKM_SUITE_FT_OVER_SAE:
				u4AkmSuite = RSN_AKM_SUITE_FT_OVER_SAE;
				break;
			case WLAN_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED:
				u4AkmSuite =
				    RSN_AKM_SUITE_FT_802_1X_SHA384_UNRESTRICTED;
				break;
			case WLAN_AKM_SUITE_FT_SAE_EXT_KEY:
				u4AkmSuite = RSN_AKM_SUITE_FT_SAE_EXT_KEY;
				break;
#endif
#if CFG_SUPPORT_802_11W
			/* Notice:: Need kernel patch!! */
			case WLAN_AKM_SUITE_8021X_SHA256:
				u4AkmSuite = RSN_AKM_SUITE_802_1X_SHA256;
				break;
			case WLAN_AKM_SUITE_PSK_SHA256:
				u4AkmSuite = RSN_AKM_SUITE_PSK_SHA256;
				break;
#endif
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
			case WLAN_AKM_SUITE_FILS_SHA256:
				u4AkmSuite = RSN_AKM_SUITE_FILS_SHA256;
				break;
			case WLAN_AKM_SUITE_FILS_SHA384:
				u4AkmSuite = RSN_AKM_SUITE_FILS_SHA384;
				break;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
#if CFG_SUPPORT_PASSPOINT
			case WLAN_AKM_SUITE_OSEN:
				u4AkmSuite = RSN_AKM_SUITE_OSEN;
				break;
#endif
			case WLAN_AKM_SUITE_SAE:
				u4AkmSuite = RSN_AKM_SUITE_SAE;
				break;
			case WLAN_AKM_SUITE_SAE_EXT_KEY:
				u4AkmSuite = RSN_AKM_SUITE_SAE_EXT_KEY;
				break;
			case WLAN_AKM_SUITE_OWE:
				u4AkmSuite = RSN_AKM_SUITE_OWE;
				break;
#if CFG_SUPPORT_DPP
			case WLAN_AKM_SUITE_DPP:
				u4AkmSuite = RSN_AKM_SUITE_DPP;
				break;
#endif
			case WLAN_AKM_SUITE_8021X_SUITE_B_192:
				u4AkmSuite = RSN_AKM_SUITE_8021X_SUITE_B_192;
				break;
			default:
				DBGLOG(REQ, WARN, "invalid Akm Suite (0x%x)\n",
				       au4AkmSuites[i]);
				return -EINVAL;
			}
		}

		eAuthMode = rsnKeyMgmtToAuthMode(
			eOriAuthMode, u4WpaVersion, u4AkmSuite);

		/* Enable the specific AKM suite only. */
		for (j = 0; j < MAX_NUM_SUPPORTED_AKM_SUITES; j++) {
			prEntry =
			    &prMib->dot11RSNAConfigAuthenticationSuitesTable[j];

			if (prEntry->dot11RSNAConfigAuthenticationSuite !=
				u4AkmSuite)
				continue;

			prEntry->dot11RSNAConfigAuthenticationSuiteEnabled =
				TRUE;

			prMib->dot11RSNAConfigAkm |=
				rsnKeyMgmtToBit(u4AkmSuite);
		}

		if (i == 0) {
			*prAuthMode = eAuthMode;
			*pu4AkmSuite = u4AkmSuite;
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to connect to
 *        the ESS with the specified parameters
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_connect(struct wiphy *wiphy,
			 struct net_device *ndev,
			 struct cfg80211_connect_params *sme)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	enum ENUM_WEP_STATUS eEncStatus;
	enum ENUM_PARAM_AUTH_MODE eAuthMode = AUTH_MODE_OPEN;
	uint32_t cipher;
	struct PARAM_CONNECT rNewSsid;
	struct PARAM_OP_MODE rOpMode;
	uint32_t u4AkmSuite = 0;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;

	struct GL_WPA_INFO *prWpaInfo;
	struct IEEE_802_11_MIB *prMib;
#if CFG_SUPPORT_PASSPOINT
	struct HS20_INFO *prHS20Info;
#endif
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		return -EINVAL;

	DBGLOG(REQ, INFO,
	       "[wlan] mtk_cfg80211_connect %p %zu auth_type=%d flags=0x%x\n",
	       sme->ie, sme->ie_len, sme->auth_type, sme->flags);
	prConnSettings = aisGetConnSettings(prGlueInfo->prAdapter, ucBssIndex);
	/* init to prevent returning status success due to no valid ap. */
	prConnSettings->u2JoinStatus = WLAN_STATUS_AUTH_TIMEOUT;
	prConnSettings->u4ConnFlags = sme->flags;
	if (prConnSettings->eOPMode > NET_TYPE_AUTO_SWITCH)
		rOpMode.eOpMode = NET_TYPE_AUTO_SWITCH;
	else
		rOpMode.eOpMode = prConnSettings->eOPMode;
	rOpMode.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, wlanoidSetInfrastructureMode,
		(void *)&rOpMode, sizeof(struct PARAM_OP_MODE),
		&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO,
		       "wlanoidSetInfrastructureMode fail 0x%x\n", rStatus);
		return -EFAULT;
	}
	/* after set operation mode, key table are cleared */

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		ucBssIndex);
	/* <1> Reset WPA info */
	prWpaInfo->u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;
	prWpaInfo->u4KeyMgmt = 0;
	prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_NONE;
	prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_NONE;
	prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
	prWpaInfo->fgPrivacyInvoke = FALSE;
#if CFG_SUPPORT_802_11W
	prWpaInfo->u4Mfp = RSN_AUTH_MFP_DISABLED;
	prWpaInfo->ucRSNMfpCap = RSN_AUTH_MFP_DISABLED;
	prWpaInfo->u4CipherGroupMgmt = RSN_CIPHER_SUITE_BIP_CMAC_128;
#endif
	aisInitializeConnectionRsnInfo(prGlueInfo->prAdapter, ucBssIndex);
	prMib = aisGetMib(prGlueInfo->prAdapter, ucBssIndex);

	if (sme->crypto.wpa_versions & NL80211_WPA_VERSION_1)
		prWpaInfo->u4WpaVersion = IW_AUTH_WPA_VERSION_WPA;
	else if ((sme->crypto.wpa_versions & NL80211_WPA_VERSION_2)
#if KERNEL_VERSION(5, 4, 0) <= LINUX_VERSION_CODE
		|| (sme->crypto.wpa_versions & NL80211_WPA_VERSION_3)
#endif
		)
		prWpaInfo->u4WpaVersion = IW_AUTH_WPA_VERSION_WPA2;
	else
		prWpaInfo->u4WpaVersion = IW_AUTH_WPA_VERSION_DISABLED;

	DBGLOG(REQ, INFO,
	       "sme->auth_type=%x, sme->crypto.wpa_versions=%x",
		sme->auth_type,	sme->crypto.wpa_versions);

	switch (sme->auth_type) {
	case NL80211_AUTHTYPE_OPEN_SYSTEM:
		prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
		eAuthMode = AUTH_MODE_OPEN;
		break;
	case NL80211_AUTHTYPE_SHARED_KEY:
		prWpaInfo->u4AuthAlg = IW_AUTH_ALG_SHARED_KEY;
		eAuthMode = AUTH_MODE_SHARED;
		break;
	case NL80211_AUTHTYPE_FT:
		prWpaInfo->u4AuthAlg = IW_AUTH_ALG_FT;
		eAuthMode = AUTH_MODE_OPEN;
		break;
	case NL80211_AUTHTYPE_SAE:
		prWpaInfo->u4AuthAlg = IW_AUTH_ALG_SAE;
		/* To prevent FWKs asks connect without AKM Suite */
		eAuthMode = AUTH_MODE_WPA3_SAE;
		u4AkmSuite = RSN_AKM_SUITE_SAE;
		break;
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	case NL80211_AUTHTYPE_FILS_SK:
		prWpaInfo->u4AuthAlg = IW_AUTH_ALG_FILS_SK;
		break;
	case NL80211_AUTHTYPE_FILS_SK_PFS:
	case NL80211_AUTHTYPE_FILS_PK:
		DBGLOG(INIT, INFO,
			"Only support fils share key authentication without PFS (auth_type=%d)\n",
			sme->auth_type);
		return -EFAULT;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
	default:
		/* NL80211 only set the Tx wep key while connect */
		if (sme->key_len != 0) {
			prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM |
					       IW_AUTH_ALG_SHARED_KEY;
			eAuthMode = AUTH_MODE_AUTO_SWITCH;
		} else {
			prWpaInfo->u4AuthAlg = IW_AUTH_ALG_OPEN_SYSTEM;
			eAuthMode = AUTH_MODE_OPEN;
		}
		break;
	}

	if (sme->crypto.n_akm_suites) {
		DBGLOG(REQ, INFO, "n_akm_suites=%x, akm_suites=%x",
			sme->crypto.n_akm_suites,
			sme->crypto.akm_suites[0]);
		if (wlanParseAkmSuites(sme->crypto.akm_suites,
			sme->crypto.n_akm_suites, prWpaInfo->u4WpaVersion,
			&eAuthMode, &u4AkmSuite, prMib) < 0) {
			return -EINVAL;
		}
	}

	if (sme->crypto.n_ciphers_pairwise) {
		DBGLOG(RSN, INFO, "cipher pairwise (0x%x)\n",
		       sme->crypto.ciphers_pairwise[0]);
		prMib->dot11RSNAConfigPairwiseCipher =
			SWAP32(sme->crypto.ciphers_pairwise[0]);
		switch (sme->crypto.ciphers_pairwise[0]) {
		case WLAN_CIPHER_SUITE_WEP40:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_WEP40;
			break;
		case WLAN_CIPHER_SUITE_WEP104:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_WEP104;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_TKIP;
#if (CFG_TC10_FEATURE == 1)
			if (eAuthMode == AUTH_MODE_WPA_PSK ||
			    eAuthMode == AUTH_MODE_WPA2_PSK)
				prWpaInfo->u4CipherPairwise |=
					IW_AUTH_CIPHER_CCMP;
#endif
			break;
		case WLAN_CIPHER_SUITE_CCMP:
		case WLAN_CIPHER_SUITE_AES_CMAC:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_CCMP;
#if (CFG_TC10_FEATURE == 1)
			if (eAuthMode == AUTH_MODE_WPA_PSK ||
			    eAuthMode == AUTH_MODE_WPA2_PSK)
				prWpaInfo->u4CipherPairwise |=
					IW_AUTH_CIPHER_TKIP;
#endif
			break;
#if CFG_SUPPORT_WAPI
		case WLAN_CIPHER_SUITE_SMS4:
			break;
#endif
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		case WLAN_CIPHER_SUITE_GCMP_256:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_GCMP256;
			break;
#endif
		case WLAN_CIPHER_SUITE_GCMP:
			prWpaInfo->u4CipherPairwise = IW_AUTH_CIPHER_GCMP128;
			break;
		case WLAN_CIPHER_SUITE_NO_GROUP_ADDR:
			DBGLOG(REQ, INFO, "WLAN_CIPHER_SUITE_NO_GROUP_ADDR\n");
			break;
		default:
			DBGLOG(REQ, WARN, "invalid cipher pairwise (%d)\n",
			       sme->crypto.ciphers_pairwise[0]);
			return -EINVAL;
		}
	}

	if (sme->crypto.cipher_group) {
		DBGLOG(RSN, INFO, "cipher group (0x%x)\n",
		       sme->crypto.cipher_group);
		prMib->dot11RSNAConfigGroupCipher =
			SWAP32(sme->crypto.cipher_group);
		switch (sme->crypto.cipher_group) {
		case WLAN_CIPHER_SUITE_WEP40:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_WEP40;
			break;
		case WLAN_CIPHER_SUITE_WEP104:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_WEP104;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_TKIP;
#if (CFG_TC10_FEATURE == 1)
			if (eAuthMode == AUTH_MODE_WPA_PSK ||
			    eAuthMode == AUTH_MODE_WPA2_PSK)
				prWpaInfo->u4CipherGroup |=
					IW_AUTH_CIPHER_CCMP;
#endif
			break;
		case WLAN_CIPHER_SUITE_CCMP:
		case WLAN_CIPHER_SUITE_AES_CMAC:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_CCMP;
#if (CFG_TC10_FEATURE == 1)
			if (eAuthMode == AUTH_MODE_WPA_PSK ||
			    eAuthMode == AUTH_MODE_WPA2_PSK)
				prWpaInfo->u4CipherGroup |=
					IW_AUTH_CIPHER_TKIP;
#endif
			break;
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		case WLAN_CIPHER_SUITE_GCMP_256:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_GCMP256;
			break;
#endif
		case WLAN_CIPHER_SUITE_GCMP:
			prWpaInfo->u4CipherGroup = IW_AUTH_CIPHER_GCMP128;
			break;
#if CFG_SUPPORT_WAPI
		case WLAN_CIPHER_SUITE_SMS4:
			break;
#endif
		case WLAN_CIPHER_SUITE_NO_GROUP_ADDR:
			break;
		default:
			DBGLOG(REQ, WARN, "invalid cipher group (%d)\n",
			       sme->crypto.cipher_group);
			return -EINVAL;
		}
	}

	DBGLOG(REQ, INFO,
		"u4WpaVersion=%d, u4AuthAlg=%d, eAuthMode=%d, u4AkmSuite=0x%x, u4CipherGroup=0x%x, u4CipherPairwise=0x%x\n",
		prWpaInfo->u4WpaVersion, prWpaInfo->u4AuthAlg,
		eAuthMode, u4AkmSuite, prWpaInfo->u4CipherGroup,
		prWpaInfo->u4CipherPairwise);

	prWpaInfo->fgPrivacyInvoke = sme->privacy;
	prConnSettings->fgWpsActive = FALSE;

#if CFG_SUPPORT_PASSPOINT
	prHS20Info = aisGetHS20Info(prGlueInfo->prAdapter,
		ucBssIndex);
	prHS20Info->fgConnectHS20AP = FALSE;
#endif /* CFG_SUPPORT_PASSPOINT */

	prConnSettings->non_wfa_vendor_ie_len = 0;
	if (sme->ie && sme->ie_len > 0) {
		uint32_t rStatus;
		uint32_t u4BufLen;
		uint8_t *prDesiredIE = NULL;
		uint8_t *pucIEStart = (uint8_t *)sme->ie;
#if CFG_SUPPORT_WAPI
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetWapiAssocInfo,
				pucIEStart, sme->ie_len,
				&u4BufLen,
				ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, TRACE,
				"[wapi] wapi not support due to set wapi assoc info error:%x\n",
				rStatus);
#endif
#if CFG_SUPPORT_PASSPOINT
		if (wextSrchDesiredHS20IE(pucIEStart, sme->ie_len,
					  (uint8_t **) &prDesiredIE)) {
			rStatus = kalIoctlByBssIdx(prGlueInfo,
					   wlanoidSetHS20Info,
					   prDesiredIE, IE_SIZE(prDesiredIE),
					   &u4BufLen, ucBssIndex);
		}
#endif /* CFG_SUPPORT_PASSPOINT */
		if (wextSrchDesiredWPAIE(pucIEStart, sme->ie_len, ELEM_ID_RSN,
					 (uint8_t **) &prDesiredIE)) {
			struct RSN_INFO rRsnInfo;

			if (rsnParseRsnIE(prGlueInfo->prAdapter,
			    (struct RSN_INFO_ELEM *)prDesiredIE, &rRsnInfo)) {
				prWpaInfo->u4CipherGroupMgmt =
					rRsnInfo.u4GroupMgmtCipherSuite;
				DBGLOG(RSN, INFO,
					"RSN: group mgmt cipher suite 0x%x\n",
					prWpaInfo->u4CipherGroupMgmt);
#if CFG_SUPPORT_802_11W
				if (rRsnInfo.u2RsnCap & ELEM_WPA_CAP_MFPC) {
					prWpaInfo->ucRSNMfpCap =
							RSN_AUTH_MFP_OPTIONAL;
					if (rRsnInfo.u2RsnCap &
					    ELEM_WPA_CAP_MFPR)
						prWpaInfo->
						ucRSNMfpCap =
							RSN_AUTH_MFP_REQUIRED;
				} else
					prWpaInfo->ucRSNMfpCap =
							RSN_AUTH_MFP_DISABLED;
#endif
			}
		}
		if (wextSrchDesiredWPAIE(pucIEStart, sme->ie_len, ELEM_ID_RSNX,
					 (uint8_t **) &prDesiredIE)) {
			struct RSNX_INFO rRsnxeInfo;

			if (rsnParseRsnxIE(prGlueInfo->prAdapter,
				(struct RSNX_INFO_ELEM *)prDesiredIE,
					&rRsnxeInfo)) {
				prWpaInfo->u2RSNXCap = rRsnxeInfo.u2Cap;
				if (prWpaInfo->u2RSNXCap &
					BIT(WLAN_RSNX_CAPAB_SAE_H2E)) {
					DBGLOG(RSN, INFO,
						"SAE-H2E is supported, RSNX ie: 0x%x\n",
						prWpaInfo->u2RSNXCap);
				}
			}
		}
	}

	/* Fill WPA info - mfp setting */
	/* Must put after paring RSNE from upper layer
	* for prWpaInfo->ucRSNMfpCap assignment
	*/
#if CFG_SUPPORT_802_11W
	switch (sme->mfp) {
	case NL80211_MFP_NO:
		prWpaInfo->u4Mfp = RSN_AUTH_MFP_DISABLED;
		/* Change Mfp parameter from DISABLED to OPTIONAL
		* if upper layer set MFPC = 1 in RSNE
		* since upper layer can't bring MFP OPTIONAL information
		* to driver by sme->mfp
		*/
		if (prWpaInfo->ucRSNMfpCap == RSN_AUTH_MFP_OPTIONAL)
			prWpaInfo->u4Mfp = RSN_AUTH_MFP_OPTIONAL;
		else if (prWpaInfo->ucRSNMfpCap == RSN_AUTH_MFP_REQUIRED)
			DBGLOG(REQ, WARN,
				"mfp parameter(DISABLED) conflict with mfp cap(REQUIRED)\n");
		break;
	case NL80211_MFP_REQUIRED:
		prWpaInfo->u4Mfp = RSN_AUTH_MFP_REQUIRED;
		break;
	default:
		prWpaInfo->u4Mfp = RSN_AUTH_MFP_DISABLED;
		break;
	}
	/* DBGLOG(REQ, INFO, ("MFP=%d\n", prWpaInfo->u4Mfp)); */
#endif

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetAuthMode, &eAuthMode,
			sizeof(eAuthMode), &u4BufLen,
			ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "set auth mode error:%x\n", rStatus);

	cipher = prWpaInfo->u4CipherGroup | prWpaInfo->u4CipherPairwise;

	if (1 /* prWpaInfo->fgPrivacyInvoke */) {
		if (cipher & (IW_AUTH_CIPHER_GCMP256 |
			      IW_AUTH_CIPHER_GCMP128)) {
			eEncStatus = ENUM_ENCRYPTION4_ENABLED;
		} else if (cipher & IW_AUTH_CIPHER_CCMP) {
			eEncStatus = ENUM_ENCRYPTION3_ENABLED;
		} else if (cipher & IW_AUTH_CIPHER_TKIP) {
			eEncStatus = ENUM_ENCRYPTION2_ENABLED;
		} else if (cipher & (IW_AUTH_CIPHER_WEP104 |
				     IW_AUTH_CIPHER_WEP40)) {
			eEncStatus = ENUM_ENCRYPTION1_ENABLED;
		} else if (cipher & IW_AUTH_CIPHER_NONE) {
			if (prWpaInfo->fgPrivacyInvoke)
				eEncStatus = ENUM_ENCRYPTION1_ENABLED;
			else
				eEncStatus = ENUM_ENCRYPTION_DISABLED;
		} else {
			eEncStatus = ENUM_ENCRYPTION_DISABLED;
		}
	} else {
		eEncStatus = ENUM_ENCRYPTION_DISABLED;
	}

	rStatus = kalIoctlByBssIdx(prGlueInfo,
			wlanoidSetEncryptionStatus, &eEncStatus,
			sizeof(eEncStatus), &u4BufLen,
			ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "set encryption mode error:%x\n",
		       rStatus);

	if (sme->key_len != 0
	    && prWpaInfo->u4WpaVersion ==
	    IW_AUTH_WPA_VERSION_DISABLED) {
		/* NL80211 only set the Tx wep key while connect, the max 4 wep
		 * key set prior via add key cmd
		 */
		uint8_t wepBuf[48];
		struct PARAM_WEP *prWepKey = (struct PARAM_WEP *) wepBuf;

		kalMemZero(prWepKey, sizeof(struct PARAM_WEP));
		prWepKey->u4Length = OFFSET_OF(struct PARAM_WEP,
					       aucKeyMaterial) + sme->key_len;
		prWepKey->u4KeyLength = (uint32_t) sme->key_len;
		prWepKey->u4KeyIndex = (uint32_t) sme->key_idx;
		prWepKey->u4KeyIndex |= IS_TRANSMIT_KEY;
		if (prWepKey->u4KeyLength > MAX_KEY_LEN) {
			DBGLOG(REQ, WARN, "Too long key length (%u)\n",
			       prWepKey->u4KeyLength);
			return -EINVAL;
		}
		kalMemCopy(prWepKey->aucKeyMaterial, sme->key,
			   prWepKey->u4KeyLength);

		rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidAbortScan,
				NULL, 1, &u4BufLen,
				ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, ERROR, "wlanoidAbortScan fail 0x%x\n",
				rStatus);

		rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidSetAddWep, prWepKey,
				prWepKey->u4Length,
				&u4BufLen, ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO, "wlanoidSetAddWep fail 0x%x\n",
				rStatus);
			return -EFAULT;
		}
	}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (sme->fils_erp_rrk) {
		struct PARAM_FILS rFils;

		rFils.pucErpUsername = sme->fils_erp_username;
		rFils.u2ErpUsernameLen = sme->fils_erp_username_len;
		rFils.pucErpRealm = sme->fils_erp_realm;
		rFils.pucErpRealmLen = sme->fils_erp_realm_len;
		rFils.u4ErpNextSeqNum = sme->fils_erp_next_seq_num;
		rFils.pucErpRrk = sme->fils_erp_rrk;
		rFils.u2ErpRrkLen = sme->fils_erp_rrk_len;
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetFilsConnInfo,
				&rFils, sizeof(rFils), &u4BufLen, ucBssIndex);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO,
				"FILS conn info error:%x\n", rStatus);
			return -EFAULT;
		}
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

	/* Avoid dangling pointer, set defatul all zero */
	kalMemZero(&rNewSsid, sizeof(rNewSsid));
	if (sme->channel)
		rNewSsid.u4CenterFreq = sme->channel->center_freq;
	else if (sme->channel_hint)
		rNewSsid.u4CenterFreq = sme->channel_hint->center_freq;
	rNewSsid.pucBssid = (uint8_t *)sme->bssid;
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
	rNewSsid.pucBssidHint = (uint8_t *)sme->bssid_hint;
#endif
	rNewSsid.pucSsid = (uint8_t *)sme->ssid;
	rNewSsid.u4SsidLen = sme->ssid_len;
	rNewSsid.pucIEs = (uint8_t *)sme->ie;
	rNewSsid.u4IesLen = sme->ie_len;
	rNewSsid.ucBssIdx = ucBssIndex;
	rNewSsid.u2LinkIdBitmap = 0xFFFF;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetConnect,
			   (void *)&rNewSsid, sizeof(struct PARAM_CONNECT),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);
		return -EINVAL;
	}

#if (CFG_SUPPORT_CONN_LOG == 1)
	connLogConnect(prGlueInfo->prAdapter,
		ucBssIndex,
		sme);
#endif

	return 0;
}

#if CFG_SUPPORT_WPA3
int mtk_cfg80211_external_auth(struct wiphy *wiphy,
			 struct net_device *ndev,
			 struct cfg80211_external_auth_params *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_EXTERNAL_AUTH auth;
	uint8_t rBuf[256] = {0};
	uint32_t rStatus;
	uint32_t u4BufLen;
	int32_t i4Written = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(REQ, WARN,
		       "SAE-confirm failed with invalid prGlueInfo\n");
		return -EFAULT;
	}

	i4Written += kalSnprintf(rBuf + i4Written,
				 sizeof(rBuf) - i4Written,
				 "%s: action=%d bssid="MACSTR
				 " ssid=[%u %s] key_mgmt=0x%x status=%u",
				 ndev->name,
				 params->action,
				 MAC2STR(params->bssid),
				 params->ssid.ssid_len,
				 params->ssid.ssid,
				 params->key_mgmt_suite,
				 params->status);
#if (KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE)
	i4Written += kalSnprintf(rBuf + i4Written,
				 sizeof(rBuf) - i4Written,
				 " mld_addr="MACSTR,
				 MAC2STR(params->mld_addr));
#endif

	DBGLOG(REQ, INFO, "%s\n", rBuf);
#if (KERNEL_VERSION(5, 1, 0) <= CFG80211_VERSION_CODE)
	if (params->pmkid)
		DBGLOG(REQ, LOUD, "PMKID="PMKSTR"\n",
			params->pmkid[0], params->pmkid[1],
			params->pmkid[2], params->pmkid[3],
			params->pmkid[4], params->pmkid[5],
			params->pmkid[6], params->pmkid[7],
			params->pmkid[8], params->pmkid[9],
			params->pmkid[10], params->pmkid[11],
			params->pmkid[12] + params->pmkid[13],
			params->pmkid[14], params->pmkid[15]);
#endif

	COPY_MAC_ADDR(auth.bssid, params->bssid);
	auth.status = params->status;
	auth.ucBssIdx = wlanGetBssIdx(ndev);
	rStatus = kalIoctl(prGlueInfo, wlanoidExternalAuthDone, (void *)&auth,
			   sizeof(auth), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(OID, INFO, "SAE-confirm failed with: %d\n", rStatus);

	return 0;
}
#endif

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_CONTROL_PORT_OVER_NL80211 == 1)
int mtk_cfg80211_tx_control_port(struct wiphy *wiphy, struct net_device *dev,
				 const u8 *buf, size_t len,
				 const u8 *dest, __be16 proto, bool unencrypted,
				 int link_id, u64 *cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct sk_buff *prSkb;
	struct ethhdr *prEthHdr;
	struct BSS_INFO *prBssInfo;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
	struct MLD_STA_RECORD *prMldSta;
#endif
	uint32_t u4SkbSize, u4TxHeadRoomSize = 0;
	uint16_t u2QueIdx;
	uint8_t ucBssIndex;
	int ret = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
			       WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		ret = -EFAULT;
		goto exit;
	}

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(dev);
	ucBssIndex = prNetDevPrivate->ucBssIdx;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(REQ, ERROR, "Null Bss by idx(%u)\n", ucBssIndex);
		ret = -EINVAL;
		goto exit;
	}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = mldBssGetByBss(prGlueInfo->prAdapter, prBssInfo);
	prMldSta = mldStarecGetByMldAddr(prGlueInfo->prAdapter, prMldBss,
					 dest);
	if (link_id != -1 && IS_MLD_BSSINFO_MULTI(prMldBss)) {
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;
		u_int8_t fgFound = FALSE;

		prBssList = &prMldBss->rBssList;
		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			if (prTempBss->ucLinkIndex != link_id)
				continue;

			ucBssIndex = prTempBss->ucBssIndex;
			fgFound = TRUE;
			break;
		}
		if (fgFound == FALSE)
			DBGLOG(REQ, WARN, "link not found(%d)\n", link_id);
	}
#endif

	u4TxHeadRoomSize = NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;
	u4SkbSize = u4TxHeadRoomSize + sizeof(struct ethhdr) + len;
	prSkb = dev_alloc_skb(u4SkbSize);
	if (!prSkb) {
		DBGLOG(REQ, ERROR, "Alloc skb failed, size=%u\n", u4SkbSize);
		ret = -ENOMEM;
		goto exit;
	}
	kmemleak_not_leak(prSkb); /* Omit memleak check */

	kalResetPacket(prGlueInfo, prSkb);
	skb_reserve(prSkb, u4TxHeadRoomSize + sizeof(struct ethhdr));
	skb_put_data(prSkb, buf, len);

	prEthHdr = skb_push(prSkb, sizeof(struct ethhdr));
	kalMemCopy(prEthHdr->h_dest, dest, ETH_ALEN);
	if (link_id == -1)
		COPY_MAC_ADDR(prEthHdr->h_source, dev->dev_addr);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	else if (prMldSta)
		COPY_MAC_ADDR(prEthHdr->h_source, prMldBss->aucOwnMldAddr);
	else
		COPY_MAC_ADDR(prEthHdr->h_source, prBssInfo->aucOwnMacAddr);
#endif
	prEthHdr->h_proto = proto;

	prSkb->dev = dev;
	prSkb->protocol = proto;
	u2QueIdx = wlanSelectQueue(dev, prSkb, NULL);
	skb_set_queue_mapping(prSkb, u2QueIdx);

	GLUE_SET_PKT_TX_COOKIE(prSkb, (uint32_t)prGlueInfo->u8Cookie++);
	*cookie = (uint64_t)GLUE_GET_PKT_TX_COOKIE(prSkb);
	GLUE_SET_PKT_CONTROL_PORT_TX(prSkb);

	DBGLOG(REQ, INFO,
		"%s: [%u] dest="MACSTR" src="MACSTR
		" proto=0x%x unencrypted=%d link_id=%d cookie=0x%llx\n",
		dev->name, ucBssIndex, MAC2STR(prEthHdr->h_dest),
		MAC2STR(prEthHdr->h_source), proto, unencrypted,
		link_id, *cookie);
	DBGLOG_MEM8(REQ, LOUD, buf, len);

	kalHardStartXmit(prSkb, dev, prGlueInfo, ucBssIndex);

exit:
	return ret;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to disconnect from
 *        currently connected ESS
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_disconnect(struct wiphy *wiphy,
			    struct net_device *ndev, u16 reason_code)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	DBGLOG(REQ, INFO, "ucBssIndex = %d\n", ucBssIndex);
	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetDisassociate, NULL,
			   0, &u4BufLen, ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "disassociate error:%x\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to join an IBSS group
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_join_ibss(struct wiphy *wiphy,
			   struct net_device *ndev,
			   struct cfg80211_ibss_params *params)
{
	struct PARAM_SSID rNewSsid;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4ChnlFreq;	/* Store channel or frequency information */
	uint32_t u4BufLen = 0, u4SsidLen = 0;
	uint32_t rStatus;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* set channel */
	if (params->channel_fixed) {
		u4ChnlFreq = params->chandef.center_freq1;

		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetFrequency,
				&u4ChnlFreq, sizeof(u4ChnlFreq),
				&u4BufLen, ucBssIndex);
		if (rStatus != WLAN_STATUS_SUCCESS)
			return -EFAULT;
	}

	/* set SSID */
	if (params->ssid_len > PARAM_MAX_LEN_SSID)
		u4SsidLen = PARAM_MAX_LEN_SSID;
	else
		u4SsidLen = params->ssid_len;

	kalMemCopy(rNewSsid.aucSsid, params->ssid,
		   u4SsidLen);
	rStatus = kalIoctlByBssIdx(prGlueInfo,
				wlanoidSetSsid, (void *)&rNewSsid,
				sizeof(struct PARAM_SSID),
				&u4BufLen, ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "set SSID:%x\n", rStatus);
		return -EFAULT;
	}

	return 0;

	return -EINVAL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to leave from IBSS group
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_leave_ibss(struct wiphy *wiphy,
			    struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetDisassociate, NULL,
			   0, &u4BufLen, ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "disassociate error:%x\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to configure
 *        WLAN power managemenet
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_power_mgmt(struct wiphy *wiphy,
			struct net_device *ndev, bool enabled, int timeout)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct PARAM_POWER_MODE_ rPowerMode;
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!prGlueInfo)
		return -EFAULT;

	if (prGlueInfo->prAdapter->fgEnDbgPowerMode) {
		DBGLOG(REQ, WARN,
			"Force power mode enabled, ignore: %d\n", enabled);
		return 0;
	}

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter,
		ucBssIndex);
	if (!prBssInfo)
		return -EINVAL;

	DBGLOG(REQ, INFO, "%d: enabled=%d, timeout=%d, fgTIMPresend=%d\n",
	       ucBssIndex, enabled, timeout,
	       prBssInfo->fgTIMPresent);

	if (enabled) {
		if (prBssInfo->eConnectionState
			== MEDIA_STATE_CONNECTED &&
		    !prBssInfo->fgTIMPresent)
			return -EFAULT;

		if (timeout == -1)
			rPowerMode.ePowerMode = Param_PowerModeFast_PSP;
		else
			rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
	} else {
		rPowerMode.ePowerMode = Param_PowerModeCAM;
	}

	rPowerMode.ucBssIdx = ucBssIndex;

	rStatus = kalIoctl(prGlueInfo, wlanoidSet802dot11PowerSaveProfile,
			   &rPowerMode, sizeof(struct PARAM_POWER_MODE_),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "set_power_mgmt error:%x\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

void wlanParsePmksa(struct cfg80211_pmksa *pmksa,
	struct PARAM_PMKID *param, uint8_t ucBssIndex)
{
	kalMemZero(param, sizeof(*param));
	kalMemCopy(param->arPMKID, pmksa->pmkid, IW_PMKID_LEN);

	if (pmksa->bssid)
		COPY_MAC_ADDR(param->arBSSID, pmksa->bssid);

	if (pmksa->pmk && pmksa->pmk_len) {
		if (pmksa->pmk_len > sizeof(param->arPMK)) {
			DBGLOG(REQ, WARN, "pmk len=%d too big\n",
				(int)pmksa->pmk_len);
		} else {
			kalMemCopy(param->arPMK, pmksa->pmk,
				pmksa->pmk_len);
			param->u2PMKLen = pmksa->pmk_len;
		}
	}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (pmksa->cache_id && pmksa->ssid && pmksa->ssid_len) {
		param->fgFilsCacheIdSet = TRUE;
		kalMemCopy(param->arFilsCacheId, pmksa->cache_id, 2);
		COPY_SSID(param->rSsid.aucSsid, param->rSsid.u4SsidLen,
			pmksa->ssid, pmksa->ssid_len);
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */

	param->ucBssIdx = ucBssIndex;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cache
 *        a PMKID for a BSSID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_pmksa(struct wiphy *wiphy,
		   struct net_device *ndev, struct cfg80211_pmksa *pmksa)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct PARAM_PMKID pmkid = {0};
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	DBGLOG(REQ, TRACE, "mtk_cfg80211_set_pmksa " MACSTR " pmk\n",
		MAC2STR(pmksa->bssid));

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex) || !pmksa->pmkid)
		return -EINVAL;

	wlanParsePmksa(pmksa, &pmkid, ucBssIndex);
	rStatus = kalIoctl(prGlueInfo, wlanoidSetPmkid, &pmkid,
			   sizeof(struct PARAM_PMKID),
			   &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "add pmkid error:%x\n", rStatus);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to remove
 *        a cached PMKID for a BSSID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_del_pmksa(struct wiphy *wiphy,
			struct net_device *ndev, struct cfg80211_pmksa *pmksa)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct PARAM_PMKID pmkid = {0};
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	DBGLOG(REQ, TRACE, "mtk_cfg80211_del_pmksa " MACSTR "\n",
		MAC2STR(pmksa->bssid));

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex) || !pmksa->pmkid)
		return -EINVAL;

	wlanParsePmksa(pmksa, &pmkid, ucBssIndex);
	rStatus = kalIoctl(prGlueInfo, wlanoidDelPmkid, &pmkid,
			   sizeof(struct PARAM_PMKID),
			   &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "add pmkid error:%x\n", rStatus);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to flush
 *        all cached PMKID
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_flush_pmksa(struct wiphy *wiphy,
			     struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidFlushPmkid, NULL, 0,
			   &u4BufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "flush pmkid error:%x\n", rStatus);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting the rekey data
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_set_rekey_data(struct wiphy *wiphy,
				struct net_device *dev,
				struct cfg80211_gtk_rekey_data *data)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen;
	struct PARAM_GTK_REKEY_DATA *prGtkData;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Rslt = -EINVAL;
	struct GL_WPA_INFO *prWpaInfo;
	uint8_t ucBssIndex = 0;
	struct BSS_INFO *prBssInfo;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(dev);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (!prBssInfo)
		return -EINVAL;

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter, ucBssIndex);

	/* if ucEapolSuspendOffload 1 => suspend rekey offload */
	/* So we store key data here */
	/* and enable rekey when enter system suspend wow */
	if (prGlueInfo->prAdapter->rWifiVar.ucEapolSuspendOffload) {
		kalMemZero(prWpaInfo->aucKek, NL80211_KEK_LEN);
		kalMemZero(prWpaInfo->aucKck, NL80211_KCK_LEN);
		kalMemZero(prWpaInfo->aucReplayCtr, NL80211_REPLAY_CTR_LEN);
		kalMemCopy(prWpaInfo->aucKek, data->kek, NL80211_KEK_LEN);
		kalMemCopy(prWpaInfo->aucKck, data->kck, NL80211_KCK_LEN);
		kalMemCopy(prWpaInfo->aucReplayCtr,
			data->replay_ctr, NL80211_REPLAY_CTR_LEN);

		return 0;
	}

	prGtkData =
		(struct PARAM_GTK_REKEY_DATA *) kalMemAlloc(sizeof(
				struct PARAM_GTK_REKEY_DATA), VIR_MEM_TYPE);

	if (!prGtkData)
		return 0;

	DBGLOG(RSN, INFO, "ucBssIndex = %d, size(%d)\n",
		ucBssIndex,
		(uint32_t) sizeof(struct cfg80211_gtk_rekey_data));

	DBGLOG(RSN, TRACE, "kek\n");
	DBGLOG_MEM8(RSN, TRACE, (uint8_t *)data->kek,
		    NL80211_KEK_LEN);
	DBGLOG(RSN, TRACE, "kck\n");
	DBGLOG_MEM8(RSN, TRACE, (uint8_t *)data->kck,
		    NL80211_KCK_LEN);
	DBGLOG(RSN, TRACE, "replay count\n");
	DBGLOG_MEM8(RSN, TRACE, (uint8_t *)data->replay_ctr,
		    NL80211_REPLAY_CTR_LEN);


#if 0
	kalMemCopy(prGtkData, data, sizeof(*data));
#else
	kalMemCopy(prGtkData->aucKek, data->kek, NL80211_KEK_LEN);
	kalMemCopy(prGtkData->aucKck, data->kck, NL80211_KCK_LEN);
	kalMemCopy(prGtkData->aucReplayCtr, data->replay_ctr,
		   NL80211_REPLAY_CTR_LEN);
#endif

	prGtkData->ucBssIndex = ucBssIndex;
#if (CFG_REKEY_OFFLOAD == 0)
	prGtkData->ucRekeyMode = GTK_REKEY_CMD_MODE_OFLOAD_OFF;
#else
	prGtkData->ucRekeyMode = GTK_REKEY_CMD_MODE_OFFLOAD_ON;
#endif

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		ucBssIndex);

	prGtkData->u4Proto = NL80211_WPA_VERSION_2;
	if (prWpaInfo->u4WpaVersion == IW_AUTH_WPA_VERSION_WPA)
		prGtkData->u4Proto = NL80211_WPA_VERSION_1;

	if (GET_SELECTOR_TYPE(prBssInfo->u4RsnSelectedPairwiseCipher) ==
			    CIPHER_SUITE_TKIP)
		prGtkData->u4PairwiseCipher = BIT(3);
	else if (GET_SELECTOR_TYPE(prBssInfo->u4RsnSelectedPairwiseCipher) ==
			    CIPHER_SUITE_CCMP)
		prGtkData->u4PairwiseCipher = BIT(4);
	else {
		kalMemFree(prGtkData, VIR_MEM_TYPE,
			   sizeof(struct PARAM_GTK_REKEY_DATA));
		return 0;
	}

	if (GET_SELECTOR_TYPE(prBssInfo->u4RsnSelectedGroupCipher) ==
			    CIPHER_SUITE_TKIP)
		prGtkData->u4GroupCipher = BIT(3);
	else if (GET_SELECTOR_TYPE(prBssInfo->u4RsnSelectedGroupCipher) ==
			    CIPHER_SUITE_CCMP)
		prGtkData->u4GroupCipher = BIT(4);
	else {
		kalMemFree(prGtkData, VIR_MEM_TYPE,
			   sizeof(struct PARAM_GTK_REKEY_DATA));
		return 0;
	}

	prGtkData->u4KeyMgmt = prBssInfo->u4RsnSelectedAKMSuite;
	prGtkData->u4MgmtGroupCipher = 0;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetGtkRekeyData, prGtkData,
				sizeof(struct PARAM_GTK_REKEY_DATA),
				&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "set GTK rekey data error:%x\n",
		       rStatus);
	else
		i4Rslt = 0;

	kalMemFree(prGtkData, VIR_MEM_TYPE,
		   sizeof(struct PARAM_GTK_REKEY_DATA));

	return i4Rslt;
}

void mtk_cfg80211_mgmt_frame_register(struct wiphy *wiphy,
				      struct wireless_dev *wdev,
				      u16 frame_type,
				      bool reg)
{
#if 0
	struct MSG_P2P_MGMT_FRAME_REGISTER *prMgmtFrameRegister =
		(struct MSG_P2P_MGMT_FRAME_REGISTER *) NULL;
#endif
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;

	do {
		if ((wiphy == NULL) || (wdev == NULL))
			break;

		DBGLOG(INIT, TRACE, "netdev: 0x%p, frame_type: 0x%x, reg: %d\n",
				wdev->netdev, frame_type, reg);

		WIPHY_PRIV(wiphy, prGlueInfo);

		/* prepare private netdev */
		prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(wdev->netdev);

		switch (frame_type) {
		case MAC_FRAME_PROBE_REQ:
			if (reg) {
				prNetDevPrivate->u4OsMgmtFrameFilter |=
					PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(INIT, TRACE,
					"Open packet filer probe request\n");
			} else {
				prNetDevPrivate->u4OsMgmtFrameFilter &=
					~PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(INIT, TRACE,
					"Close packet filer probe request\n");
			}
			break;
		case MAC_FRAME_ACTION:
			if (reg) {
				prNetDevPrivate->u4OsMgmtFrameFilter |=
					PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(INIT, TRACE,
					"Open packet filer action frame.\n");
			} else {
				prNetDevPrivate->u4OsMgmtFrameFilter &=
					~PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(INIT, TRACE,
					"Close packet filer action frame.\n");
			}
			break;
		default:
			DBGLOG(INIT, TRACE,
				"Ask frog to add code for mgmt:%x\n",
				frame_type);
			break;
		}

#if 0

		prMgmtFrameRegister =
			(struct MSG_P2P_MGMT_FRAME_REGISTER *) cnmMemAlloc(
				prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_MGMT_FRAME_REGISTER));

		if (prMgmtFrameRegister == NULL) {
			ASSERT(FALSE);
			break;
		}

		prMgmtFrameRegister->rMsgHdr.eMsgId =
			MID_MNY_P2P_MGMT_FRAME_REGISTER;

		prMgmtFrameRegister->u2FrameType = frame_type;
		prMgmtFrameRegister->fgIsRegister = reg;

		mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
			    (struct MSG_HDR *) prMgmtFrameRegister,
			    MSG_SEND_METHOD_BUF);

#endif

	} while (FALSE);

}				/* mtk_cfg80211_mgmt_frame_register */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to stay on a
 *        specified channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_remain_on_channel(struct wiphy *wiphy,
		   struct wireless_dev *wdev,
		   struct ieee80211_channel *chan, unsigned int duration,
		   u64 *cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_REMAIN_ON_CHANNEL *prMsgChnlReq =
		(struct MSG_REMAIN_ON_CHANNEL *) NULL;
	uint8_t ucBssIndex = 0;

	do {
		if ((wiphy == NULL)
		    || (wdev == NULL)
		    || (chan == NULL)
		    || (cookie == NULL)) {
			break;
		}

		WIPHY_PRIV(wiphy, prGlueInfo);
		ASSERT(prGlueInfo);

		ucBssIndex = wlanGetBssIdx(wdev->netdev);
		if (!IS_BSS_INDEX_VALID(ucBssIndex))
			return -EINVAL;

		*cookie = prGlueInfo->u8Cookie++;

		prMsgChnlReq = cnmMemAlloc(prGlueInfo->prAdapter,
			   RAM_TYPE_MSG, sizeof(struct MSG_REMAIN_ON_CHANNEL));

		if (prMsgChnlReq == NULL) {
			ASSERT(FALSE);
			i4Rslt = -ENOMEM;
			break;
		}

		prMsgChnlReq->rMsgHdr.eMsgId =
			MID_MNY_AIS_REMAIN_ON_CHANNEL;
		prMsgChnlReq->u8Cookie = *cookie;
		prMsgChnlReq->u4DurationMs = duration;
		prMsgChnlReq->eReqType = CH_REQ_TYPE_ROC;
		prMsgChnlReq->ucChannelNum = nicFreq2ChannelNum(
				chan->center_freq * 1000);

		switch (chan->band) {
		case KAL_BAND_2GHZ:
			prMsgChnlReq->eBand = BAND_2G4;
			break;
		case KAL_BAND_5GHZ:
			prMsgChnlReq->eBand = BAND_5G;
			break;
#if (CFG_SUPPORT_WIFI_6G == 1)
		case KAL_BAND_6GHZ:
			prMsgChnlReq->eBand = BAND_6G;
			break;
#endif
		default:
			prMsgChnlReq->eBand = BAND_2G4;
			break;
		}

		prMsgChnlReq->eSco = CHNL_EXT_SCN;

		prMsgChnlReq->ucBssIdx = ucBssIndex;

		mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prMsgChnlReq, MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cancel staying
 *        on a specified channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_cancel_remain_on_channel(
	struct wiphy *wiphy, struct wireless_dev *wdev, u64 cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_CANCEL_REMAIN_ON_CHANNEL *prMsgChnlAbort =
		(struct MSG_CANCEL_REMAIN_ON_CHANNEL *) NULL;
	uint8_t ucBssIndex = 0;

	do {
		if ((wiphy == NULL)
		    || (wdev == NULL)
		   ) {
			break;
		}

		WIPHY_PRIV(wiphy, prGlueInfo);
		ASSERT(prGlueInfo);

		ucBssIndex = wlanGetBssIdx(wdev->netdev);
		if (!IS_BSS_INDEX_VALID(ucBssIndex))
			return -EINVAL;

		prMsgChnlAbort =
			cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
			    sizeof(struct MSG_CANCEL_REMAIN_ON_CHANNEL));

		if (prMsgChnlAbort == NULL) {
			ASSERT(FALSE);
			i4Rslt = -ENOMEM;
			break;
		}

		prMsgChnlAbort->rMsgHdr.eMsgId =
			MID_MNY_AIS_CANCEL_REMAIN_ON_CHANNEL;
		prMsgChnlAbort->u8Cookie = cookie;

		prMsgChnlAbort->ucBssIdx = ucBssIndex;

		mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prMsgChnlAbort, MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}

#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
int _mtk_cfg80211_mgmt_tx_via_data_path(
		struct GLUE_INFO *prGlueInfo,
		struct wireless_dev *wdev,
		const u8 *buf,
		size_t len, u64 u8GlCookie)
{
	int32_t i4Rslt = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct sk_buff *prSkb = NULL;
	uint8_t *pucRecvBuff = NULL;
	uint8_t ucBssIndex = 0;

	DBGLOG(P2P, INFO, "len[%d], cookie: 0x%llx.\n", len, u8GlCookie);
	prSkb = kalPacketAlloc(prGlueInfo, len, TRUE, &pucRecvBuff);
	if (prSkb) {
		kalMemCopy(pucRecvBuff, buf, len);
		skb_put(prSkb, len);
		prSkb->dev = wdev->netdev;
		GLUE_SET_PKT_FLAG(prSkb, ENUM_PKT_802_11_MGMT);
		GLUE_SET_PKT_COOKIE(prSkb, u8GlCookie);
		ucBssIndex = wlanGetBssIdx(wdev->netdev);
		if (!IS_BSS_INDEX_VALID(ucBssIndex))
			return -EINVAL;
		rStatus = kalHardStartXmit(prSkb,
			wdev->netdev,
			prGlueInfo,
			ucBssIndex);
		if (rStatus != WLAN_STATUS_SUCCESS)
			i4Rslt = -EINVAL;
	} else
		i4Rslt = -ENOMEM;

	return i4Rslt;
}
#endif
int _mtk_cfg80211_mgmt_tx(struct wiphy *wiphy,
		struct wireless_dev *wdev, struct ieee80211_channel *chan,
		bool offchan, unsigned int wait, const u8 *buf, size_t len,
		bool no_cck, bool dont_wait_for_ack, u64 *cookie)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_MGMT_TX_REQUEST *prMsgTxReq =
			(struct MSG_MGMT_TX_REQUEST *) NULL;
	struct MSDU_INFO *prMgmtFrame = (struct MSDU_INFO *) NULL;
	uint8_t *pucFrameBuf = (uint8_t *) NULL;
	uint64_t *pu8GlCookie = (uint64_t *) NULL;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
	uint16_t u2MgmtTxMaxLen = 0;
#endif

	do {
		if ((wiphy == NULL) || (wdev == NULL) || (cookie == NULL))
			break;

		WIPHY_PRIV(wiphy, prGlueInfo);
		ASSERT(prGlueInfo);

		*cookie = prGlueInfo->u8Cookie++;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
		u2MgmtTxMaxLen = prGlueInfo->prAdapter
				->chip_info->cmd_max_pkt_size
						- prGlueInfo->prAdapter
						->chip_info->u2CmdTxHdrSize;
#if defined(_HIF_USB)
		u2MgmtTxMaxLen -= LEN_USB_UDMA_TX_TERMINATOR;
#endif
		/* to fix WFDMA entry size limitation, >1600 MGMT frame
		*  send into data flow and bypass MCU
		*/
		if (len > u2MgmtTxMaxLen)
			return _mtk_cfg80211_mgmt_tx_via_data_path(
				prGlueInfo, wdev, buf,
				len, *cookie);
#endif

		prMsgTxReq = cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_MGMT_TX_REQUEST));

		if (prMsgTxReq == NULL) {
			ASSERT(FALSE);
			i4Rslt = -ENOMEM;
			break;
		}

		if (offchan) {
			prMsgTxReq->fgIsOffChannel = TRUE;

			kalChannelFormatSwitch(NULL, chan,
					&prMsgTxReq->rChannelInfo);
			kalChannelScoSwitch(NL80211_CHAN_NO_HT,
					&prMsgTxReq->eChnlExt);
		} else {
			prMsgTxReq->fgIsOffChannel = FALSE;
		}

		if (wait)
			prMsgTxReq->u4Duration = wait;
		else
			prMsgTxReq->u4Duration = 0;

		if (no_cck)
			prMsgTxReq->fgNoneCckRate = TRUE;
		else
			prMsgTxReq->fgNoneCckRate = FALSE;

		if (dont_wait_for_ack)
			prMsgTxReq->fgIsWaitRsp = FALSE;
		else
			prMsgTxReq->fgIsWaitRsp = TRUE;

		prMgmtFrame = cnmMgtPktAlloc(prGlueInfo->prAdapter,
				(int32_t) (len + sizeof(uint64_t)
				+ MAC_TX_RESERVED_FIELD));
		prMsgTxReq->prMgmtMsduInfo = prMgmtFrame;
		if (prMsgTxReq->prMgmtMsduInfo == NULL) {
			/* ASSERT(FALSE); */
			i4Rslt = -ENOMEM;
			break;
		}

		prMsgTxReq->u8Cookie = *cookie;
		prMsgTxReq->rMsgHdr.eMsgId = MID_MNY_AIS_MGMT_TX;
		prMsgTxReq->ucBssIdx = wlanGetBssIdx(wdev->netdev);

		pucFrameBuf =
			(uint8_t *)
			((unsigned long) prMgmtFrame->prPacket
			+ MAC_TX_RESERVED_FIELD);
		pu8GlCookie =
			(uint64_t *)
			((unsigned long) prMgmtFrame->prPacket
			+ (unsigned long) len
			+ MAC_TX_RESERVED_FIELD);

		kalMemCopy(pucFrameBuf, buf, len);

		*pu8GlCookie = *cookie;

		prMgmtFrame->u2FrameLength = len;
		prMgmtFrame->ucBssIndex = wlanGetBssIdx(wdev->netdev);

#define TEMP_LOG_TEMPLATE "bssIdx: %d, band: %d, chan: %d, offchan: %d, " \
		"wait: %d, len: %d, no_cck: %d, dont_wait_for_ack: %d, " \
		"cookie: 0x%llx\n"
		DBGLOG(P2P, INFO, TEMP_LOG_TEMPLATE,
				prMsgTxReq->ucBssIdx,
				prMsgTxReq->rChannelInfo.eBand,
				prMsgTxReq->rChannelInfo.ucChannelNum,
				prMsgTxReq->fgIsOffChannel,
				prMsgTxReq->u4Duration,
				prMsgTxReq->prMgmtMsduInfo->u2FrameLength,
				prMsgTxReq->fgNoneCckRate,
				prMsgTxReq->fgIsWaitRsp,
				prMsgTxReq->u8Cookie);
#undef TEMP_LOG_TEMPLATE

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgTxReq,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	if ((i4Rslt != 0) && (prMsgTxReq != NULL)) {
		if (prMsgTxReq->prMgmtMsduInfo != NULL)
			cnmMgtPktFree(prGlueInfo->prAdapter,
				prMsgTxReq->prMgmtMsduInfo);

		cnmMemFree(prGlueInfo->prAdapter, prMsgTxReq);
	}

	return i4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to send a management frame
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_mgmt_tx(struct wiphy *wiphy,
			struct wireless_dev *wdev,
			struct cfg80211_mgmt_tx_params *params,
			u64 *cookie)
{
	if (params == NULL)
		return -EINVAL;

	return _mtk_cfg80211_mgmt_tx(wiphy, wdev, params->chan,
			params->offchan, params->wait, params->buf, params->len,
			params->no_cck, params->dont_wait_for_ack, cookie);
}
#else
int mtk_cfg80211_mgmt_tx(struct wiphy *wiphy,
		struct wireless_dev *wdev, struct ieee80211_channel *channel,
		bool offchan, unsigned int wait, const u8 *buf, size_t len,
		bool no_cck, bool dont_wait_for_ack, u64 *cookie)
{
	return _mtk_cfg80211_mgmt_tx(wiphy, wdev, channel, offchan, wait, buf,
			len, no_cck, dont_wait_for_ack, cookie);
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for requesting to cancel the wait time
 *        from transmitting a management frame on another channel
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy,
		struct wireless_dev *wdev, u64 cookie)
{
	int32_t i4Rslt;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct MSG_CANCEL_TX_WAIT_REQUEST *prMsgCancelTxWait =
			(struct MSG_CANCEL_TX_WAIT_REQUEST *) NULL;
	uint8_t ucBssIndex = 0;

	do {
		ASSERT(wiphy);

		WIPHY_PRIV(wiphy, prGlueInfo);
		ASSERT(prGlueInfo);

		ucBssIndex = wlanGetBssIdx(wdev->netdev);
		if (!IS_BSS_INDEX_VALID(ucBssIndex))
			return -EINVAL;

		DBGLOG(P2P, INFO, "cookie: 0x%llx, ucBssIndex = %d\n",
			cookie, ucBssIndex);


		prMsgCancelTxWait = cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_CANCEL_TX_WAIT_REQUEST));

		if (prMsgCancelTxWait == NULL) {
			ASSERT(FALSE);
			i4Rslt = -ENOMEM;
			break;
		}

		prMsgCancelTxWait->rMsgHdr.eMsgId =
				MID_MNY_AIS_MGMT_TX_CANCEL_WAIT;
		prMsgCancelTxWait->u8Cookie = cookie;
		prMsgCancelTxWait->ucBssIdx = ucBssIndex;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgCancelTxWait,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}

#ifdef CONFIG_NL80211_TESTMODE
#if CFG_SUPPORT_WAPI
int mtk_cfg80211_testmode_set_key_ext(struct wiphy
				      *wiphy,
		struct wireless_dev *wdev,
		void *data, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_SET_KEY_EXTS *prParams =
		(struct NL80211_DRIVER_SET_KEY_EXTS *) NULL;
	struct iw_encode_exts *prIWEncExt = (struct iw_encode_exts
					     *)NULL;
	uint32_t rstatus = WLAN_STATUS_SUCCESS;
	int fgIsValid = 0;
	uint32_t u4BufLen = 0;
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucBssIndex = 0;

	struct PARAM_KEY *prWpiKey;
	uint8_t *keyStructBuf;

	ASSERT(wiphy);
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (len < sizeof(struct NL80211_DRIVER_SET_KEY_EXTS)) {
		DBGLOG(REQ, ERROR, "len [%d] is invalid!\n", len);
		return -EINVAL;
	}
	if (data == NULL || len == 0) {
		DBGLOG(INIT, TRACE, "%s data or len is invalid\n", __func__);
		return -EINVAL;
	}

	keyStructBuf = kalMemZAlloc(KEY_BUF_SIZE, VIR_MEM_TYPE);
	if (keyStructBuf == NULL) {
		DBGLOG(REQ, ERROR, "alloc key buffer fail\n");
		return -ENOMEM;
	}
	prWpiKey = (struct PARAM_KEY *) keyStructBuf;

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		ucBssIndex = 0;

	prParams = (struct NL80211_DRIVER_SET_KEY_EXTS *) data;
	prIWEncExt = (struct iw_encode_exts *)&prParams->ext;

	if (prIWEncExt->alg == IW_ENCODE_ALG_SMS4) {
		/* KeyID */
		prWpiKey->u4KeyIndex = prParams->key_index;
		prWpiKey->u4KeyIndex--;
		if (prWpiKey->u4KeyIndex > 1) {
			fgIsValid = -EINVAL;
			goto freeBuf;
		}

		if (prIWEncExt->key_len != 32) {
			fgIsValid = -EINVAL;
			goto freeBuf;
		}
		prWpiKey->u4KeyLength = prIWEncExt->key_len;

		if (prIWEncExt->ext_flags & IW_ENCODE_EXT_SET_TX_KEY &&
		    !(prIWEncExt->ext_flags & IW_ENCODE_EXT_GROUP_KEY)) {
			/* WAI seems set the STA group key with
			 * IW_ENCODE_EXT_SET_TX_KEY !!!!
			 * Ignore the group case
			 */
			prWpiKey->u4KeyIndex |= BIT(30);
			prWpiKey->u4KeyIndex |= BIT(31);
			/* BSSID */
			memcpy(prWpiKey->arBSSID, prIWEncExt->addr, 6);
		} else {
			COPY_MAC_ADDR(prWpiKey->arBSSID, aucBCAddr);
		}

		/* PN */
		/* memcpy(prWpiKey->rKeyRSC, prIWEncExt->tx_seq,
		 * IW_ENCODE_SEQ_MAX_SIZE * 2);
		 */

		memcpy(prWpiKey->aucKeyMaterial, prIWEncExt->key, 32);

		prWpiKey->u4Length = sizeof(struct PARAM_KEY);
		prWpiKey->ucBssIdx = ucBssIndex;
		prWpiKey->ucCipher = CIPHER_SUITE_WPI;

		rstatus = kalIoctl(prGlueInfo, wlanoidSetAddKey, prWpiKey,
				sizeof(struct PARAM_KEY),
				&u4BufLen);

		if (rstatus != WLAN_STATUS_SUCCESS) {
			fgIsValid = -EFAULT;
		}

	}

freeBuf:
	if (keyStructBuf)
		kalMemFree(keyStructBuf, VIR_MEM_TYPE, KEY_BUF_SIZE);
	return fgIsValid;
}
#endif

int
mtk_cfg80211_testmode_get_sta_statistics(struct wiphy
		*wiphy, void *data, int len,
		struct GLUE_INFO *prGlueInfo)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen;
	uint32_t u4LinkScore;
	uint32_t u4TotalError;
	uint32_t u4TxExceedThresholdCount;
	uint32_t u4TxTotalCount;

	struct NL80211_DRIVER_GET_STA_STATISTICS_PARAMS *prParams =
			NULL;
	struct PARAM_GET_STA_STATISTICS rQueryStaStatistics;
	struct sk_buff *skb;

	ASSERT(wiphy);
	ASSERT(prGlueInfo);

	if (data && len)
		prParams = (struct NL80211_DRIVER_GET_STA_STATISTICS_PARAMS
			    *) data;

	if (prParams == NULL) {
		DBGLOG(QM, ERROR, "prParams is NULL, data=%p, len=%d\n",
		       data, len);
		return -EINVAL;
	} /* else if (prParams->aucMacAddr == NULL) {
		DBGLOG(QM, ERROR,
		       "prParams->aucMacAddr is NULL, data=%p, len=%d\n",
		       data, len);
		return -EINVAL;
	}
	*/

	skb = cfg80211_testmode_alloc_reply_skb(wiphy,
				sizeof(struct PARAM_GET_STA_STATISTICS) + 1);
	if (!skb) {
		DBGLOG(QM, ERROR, "allocate skb failed:%x\n", rStatus);
		return -ENOMEM;
	}

	DBGLOG(QM, TRACE, "Get [" MACSTR "] STA statistics\n",
	       MAC2STR(prParams->aucMacAddr));

	kalMemZero(&rQueryStaStatistics,
		   sizeof(rQueryStaStatistics));
	COPY_MAC_ADDR(rQueryStaStatistics.aucMacAddr,
		      prParams->aucMacAddr);
	rQueryStaStatistics.ucReadClear = TRUE;

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryStaStatistics,
			   &rQueryStaStatistics, sizeof(rQueryStaStatistics),
			   &u4BufLen);

	/* Calcute Link Score */
	u4TxExceedThresholdCount =
		rQueryStaStatistics.u4TxExceedThresholdCount;
	u4TxTotalCount = rQueryStaStatistics.u4TxTotalCount;
	u4TotalError = rQueryStaStatistics.u4TxFailCount +
		       rQueryStaStatistics.u4TxLifeTimeoutCount;

	/* u4LinkScore 10~100 , ExceedThreshold ratio 0~90 only
	 * u4LinkScore 0~9    , Drop packet ratio 0~9 and all packets exceed
	 * threshold
	 */
	if (u4TxTotalCount) {
		if (u4TxExceedThresholdCount <= u4TxTotalCount)
			u4LinkScore = (90 - ((u4TxExceedThresholdCount * 90)
							/ u4TxTotalCount));
		else
			u4LinkScore = 0;
	} else {
		u4LinkScore = 90;
	}

	u4LinkScore += 10;

	if (u4LinkScore == 10) {
		if (u4TotalError <= u4TxTotalCount)
			u4LinkScore = (10 - ((u4TotalError * 10)
							/ u4TxTotalCount));
		else
			u4LinkScore = 0;

	}

	if (u4LinkScore > 100)
		u4LinkScore = 100;
	{
		u8 __tmp = 0;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_INVALID, sizeof(u8),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u8 __tmp = NL80211_DRIVER_TESTMODE_VERSION;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_VERSION, sizeof(u8),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_MAC, MAC_ADDR_LEN,
	    prParams->aucMacAddr) < 0))
		goto nla_put_failure;
	{
		u32 __tmp = u4LinkScore;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_LINK_SCORE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.u4Flag;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_FLAG, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4EnqueueCounter;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_ENQUEUE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4DequeueCounter;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_DEQUEUE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4EnqueueStaCounter;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_STA_ENQUEUE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4DequeueStaCounter;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_STA_DEQUEUE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.IsrCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.IsrPassCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_ISR_PASS_CNT,
		    sizeof(u32), &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.TaskIsrCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_TASK_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.IsrAbnormalCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_AB_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.IsrSoftWareCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_SW_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.IsrTxCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_TX_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	{
		u32 __tmp = rQueryStaStatistics.IsrRxCnt;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_IRQ_RX_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	/* FW part STA link status */
	{
		u8 __tmp = rQueryStaStatistics.ucPer;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_PER, sizeof(u8),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u8 __tmp = rQueryStaStatistics.ucRcpi;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_RSSI, sizeof(u8),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4PhyMode;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_PHY_MODE, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u16 __tmp = rQueryStaStatistics.u2LinkSpeed;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_TX_RATE, sizeof(u16),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxFailCount;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_FAIL_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxLifeTimeoutCount;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_TIMEOUT_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxAverageAirTime;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_AVG_AIR_TIME, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}

	/* Driver part link status */
	{
		u32 __tmp = rQueryStaStatistics.u4TxTotalCount;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_TOTAL_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxExceedThresholdCount;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_THRESHOLD_CNT, sizeof(u32),
		    &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxAverageProcessTime;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_AVG_PROCESS_TIME,
		    sizeof(u32), &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxMaxTime;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_MAX_PROCESS_TIME,
		    sizeof(u32), &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxAverageHifTime;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_AVG_HIF_PROCESS_TIME,
		    sizeof(u32), &__tmp) < 0))
			goto nla_put_failure;
	}
	{
		u32 __tmp = rQueryStaStatistics.u4TxMaxHifTime;

		if (unlikely(nla_put(skb,
		    NL80211_TESTMODE_STA_STATISTICS_MAX_HIF_PROCESS_TIME,
		    sizeof(u32), &__tmp) < 0))
			goto nla_put_failure;
	}

	/* Network counter */
	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_TC_EMPTY_CNT_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcResourceEmptyCount),
	    rQueryStaStatistics.au4TcResourceEmptyCount) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_NO_TC_ARRAY,
	    sizeof(rQueryStaStatistics.au4DequeueNoTcResource),
	    rQueryStaStatistics.au4DequeueNoTcResource) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_RB_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcResourceBackCount),
	    rQueryStaStatistics.au4TcResourceBackCount) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_USED_TC_PGCT_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcResourceUsedPageCount),
	    rQueryStaStatistics.au4TcResourceUsedPageCount) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_WANTED_TC_PGCT_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcResourceWantedPageCount),
	    rQueryStaStatistics.au4TcResourceWantedPageCount) < 0))
		goto nla_put_failure;

	/* Sta queue length */
	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_TC_QUE_LEN_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcQueLen),
	    rQueryStaStatistics.au4TcQueLen) < 0))
		goto nla_put_failure;

	/* Global QM counter */
	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_TC_AVG_QUE_LEN_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcAverageQueLen),
	    rQueryStaStatistics.au4TcAverageQueLen) < 0))
		goto nla_put_failure;

	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_TC_CUR_QUE_LEN_ARRAY,
	    sizeof(rQueryStaStatistics.au4TcCurrentQueLen),
	    rQueryStaStatistics.au4TcCurrentQueLen) < 0))
		goto nla_put_failure;

	/* Reserved field */
	if (unlikely(nla_put(skb,
	    NL80211_TESTMODE_STA_STATISTICS_RESERVED_ARRAY,
	    sizeof(rQueryStaStatistics.au4Reserved),
	    rQueryStaStatistics.au4Reserved) < 0))
		goto nla_put_failure;

	return cfg80211_testmode_reply(skb);

nla_put_failure:
	/* nal_put_skb_fail */
	kfree_skb(skb);
	return -EFAULT;
}

int
mtk_cfg80211_testmode_get_link_detection(struct wiphy
		*wiphy,
		struct wireless_dev *wdev,
		void *data, int len,
		struct GLUE_INFO *prGlueInfo)
{

	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Status = -EINVAL;
	uint32_t u4BufLen;
	uint8_t u1buf = 0;
	uint32_t i = 0;
	uint32_t arBugReport[sizeof(struct EVENT_BUG_REPORT)];
	struct PARAM_802_11_STATISTICS_STRUCT rStatistics;
	struct EVENT_BUG_REPORT *prBugReport;
	struct sk_buff *skb;

	ASSERT(wiphy);
	ASSERT(prGlueInfo);

	prBugReport = (struct EVENT_BUG_REPORT *) kalMemAlloc(
			      sizeof(struct EVENT_BUG_REPORT), VIR_MEM_TYPE);
	if (!prBugReport) {
		DBGLOG(QM, TRACE, "%s allocate prBugReport failed\n",
		       __func__);
		return -ENOMEM;
	}
	skb = cfg80211_testmode_alloc_reply_skb(wiphy,
			sizeof(struct PARAM_802_11_STATISTICS_STRUCT) +
			sizeof(struct EVENT_BUG_REPORT) + 1);

	if (!skb) {
		kalMemFree(prBugReport, VIR_MEM_TYPE,
			   sizeof(struct EVENT_BUG_REPORT));
		DBGLOG(QM, TRACE, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	kalMemZero(&rStatistics, sizeof(rStatistics));
	kalMemZero(prBugReport, sizeof(struct EVENT_BUG_REPORT));
	kalMemZero(arBugReport, sizeof(uint32_t)*sizeof(struct EVENT_BUG_REPORT));

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryStatistics,
			   &rStatistics, sizeof(rStatistics),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "query statistics error:%x\n", rStatus);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidQueryBugReport,
			   prBugReport, sizeof(struct EVENT_BUG_REPORT),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, INFO, "query statistics error:%x\n", rStatus);

	kalMemCopy(arBugReport, prBugReport,
		   sizeof(struct EVENT_BUG_REPORT));

	rStatistics.u4RstReason = glGetRstReason();
	rStatistics.u8RstTime = u8ResetTime;
	rStatistics.u4RoamFailCnt = prGlueInfo->u4RoamFailCnt;
	rStatistics.u8RoamFailTime = prGlueInfo->u8RoamFailTime;
	rStatistics.u2TxDoneDelayIsARP =
		prGlueInfo->fgTxDoneDelayIsARP;
	rStatistics.u4ArriveDrvTick = prGlueInfo->u4ArriveDrvTick;
	rStatistics.u4EnQueTick = prGlueInfo->u4EnQueTick;
	rStatistics.u4DeQueTick = prGlueInfo->u4DeQueTick;
	rStatistics.u4LeaveDrvTick = prGlueInfo->u4LeaveDrvTick;
	rStatistics.u4CurrTick = prGlueInfo->u4CurrTick;
	rStatistics.u8CurrTime = prGlueInfo->u8CurrTime;

	if (!NLA_PUT_U8(skb, NL80211_TESTMODE_LINK_INVALID, &u1buf))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_TX_FAIL_CNT,
			 &rStatistics.rFailedCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_TX_RETRY_CNT,
			 &rStatistics.rRetryCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb,
			 NL80211_TESTMODE_LINK_TX_MULTI_RETRY_CNT,
			 &rStatistics.rMultipleRetryCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_ACK_FAIL_CNT,
			 &rStatistics.rACKFailureCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_FCS_ERR_CNT,
			 &rStatistics.rFCSErrorCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_TX_CNT,
			 &rStatistics.rTransmittedFragmentCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_RX_CNT,
			 &rStatistics.rReceivedFragmentCount.QuadPart))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_RST_REASON,
			 &rStatistics.u4RstReason))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_RST_TIME,
			 &rStatistics.u8RstTime))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_ROAM_FAIL_TIMES,
			 &rStatistics.u4RoamFailCnt))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_ROAM_FAIL_TIME,
			 &rStatistics.u8RoamFailTime))
		goto nla_put_failure;

	if (!NLA_PUT_U8(skb,
			NL80211_TESTMODE_LINK_TX_DONE_DELAY_IS_ARP,
			&rStatistics.u2TxDoneDelayIsARP))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_ARRIVE_DRV_TICK,
			 &rStatistics.u4ArriveDrvTick))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_ENQUE_TICK,
			 &rStatistics.u4EnQueTick))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_DEQUE_TICK,
			 &rStatistics.u4DeQueTick))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_LEAVE_DRV_TICK,
			 &rStatistics.u4LeaveDrvTick))
		goto nla_put_failure;

	if (!NLA_PUT_U32(skb, NL80211_TESTMODE_LINK_CURR_TICK,
			 &rStatistics.u4CurrTick))
		goto nla_put_failure;

	if (!NLA_PUT_U64(skb, NL80211_TESTMODE_LINK_CURR_TIME,
			 &rStatistics.u8CurrTime))
		goto nla_put_failure;

	for (i = 0;
	     i < sizeof(struct EVENT_BUG_REPORT) / sizeof(uint32_t);
	     i++) {
		if (!NLA_PUT_U32(skb, i + NL80211_TESTMODE_LINK_DETECT_NUM,
				 &arBugReport[i]))
			goto nla_put_failure;
	}

	i4Status = cfg80211_testmode_reply(skb);
	kalMemFree(prBugReport, VIR_MEM_TYPE,
		   sizeof(struct EVENT_BUG_REPORT));
	return i4Status;

nla_put_failure:
	/* nal_put_skb_fail */
	kfree_skb(skb);
	kalMemFree(prBugReport, VIR_MEM_TYPE,
		   sizeof(struct EVENT_BUG_REPORT));
	return -EFAULT;
}

int mtk_cfg80211_testmode_sw_cmd(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		void *data, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_SW_CMD_PARAMS *prParams =
		(struct NL80211_DRIVER_SW_CMD_PARAMS *) NULL;
	uint32_t rstatus = WLAN_STATUS_SUCCESS;
	int fgIsValid = 0;
	uint32_t u4SetInfoLen = 0;

	ASSERT(wiphy);

	WIPHY_PRIV(wiphy, prGlueInfo);

#if 0
	DBGLOG(INIT, INFO, "--> %s()\n", __func__);
#endif

	if (len < sizeof(struct NL80211_DRIVER_SW_CMD_PARAMS)) {
		DBGLOG(REQ, ERROR, "len [%d] is invalid!\n", len);
		return -EINVAL;
	}

	if (data && len)
		prParams = (struct NL80211_DRIVER_SW_CMD_PARAMS *) data;

	if (prParams) {
		if (prParams->set == 1) {
			rstatus = kalIoctl(prGlueInfo,
				   (PFN_OID_HANDLER_FUNC) wlanoidSetSwCtrlWrite,
				   &prParams->adr, (uint32_t) 8,
				   &u4SetInfoLen);
		}
	}

	if (rstatus != WLAN_STATUS_SUCCESS)
		fgIsValid = -EFAULT;

	return fgIsValid;
}

static int mtk_wlan_cfg_testmode_cmd(struct wiphy *wiphy,
					 struct wireless_dev *wdev,
				     void *data, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_TEST_MODE_PARAMS *prParams = NULL;
	int32_t i4Status = 0;

	ASSERT(wiphy);

	if (len < sizeof(struct NL80211_DRIVER_TEST_MODE_PARAMS)) {
		DBGLOG(REQ, ERROR, "len [%d] is invalid!\n", len);
		return -EINVAL;
	}
	if (!data || !len) {
		DBGLOG(REQ, ERROR, "mtk_cfg80211_testmode_cmd null data\n");
		return -EINVAL;
	}

	if (!wiphy) {
		DBGLOG(REQ, ERROR,
		       "mtk_cfg80211_testmode_cmd null wiphy\n");
		return -EINVAL;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	prParams = (struct NL80211_DRIVER_TEST_MODE_PARAMS *)data;

	/* Clear the version byte */
	prParams->index = prParams->index & ~BITS(24, 31);
	DBGLOG(INIT, TRACE, "params index=%x\n", prParams->index);

	switch (prParams->index) {
	case TESTMODE_CMD_ID_SW_CMD:	/* SW cmd */
		i4Status = mtk_cfg80211_testmode_sw_cmd(wiphy,
				wdev, data, len);
		break;
	case TESTMODE_CMD_ID_WAPI:	/* WAPI */
#if CFG_SUPPORT_WAPI
		i4Status = mtk_cfg80211_testmode_set_key_ext(wiphy,
				wdev, data, len);
#endif
		break;
	case 0x10:
		i4Status = mtk_cfg80211_testmode_get_sta_statistics(wiphy,
				data, len, prGlueInfo);
		break;
	case 0x20:
		i4Status = mtk_cfg80211_testmode_get_link_detection(wiphy,
				wdev, data, len, prGlueInfo);
		break;
	case TESTMODE_CMD_ID_STR_CMD:
		i4Status = mtk_cfg80211_process_str_cmd(wiphy,
				wdev, data, len);
		break;

	default:
		i4Status = -EINVAL;
		break;
	}

	if (i4Status != 0)
		DBGLOG(REQ, TRACE, "prParams->index=%d, status=%d\n",
		       prParams->index, i4Status);

	return i4Status;
}

#if KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_testmode_cmd(struct wiphy *wiphy,
			      struct wireless_dev *wdev,
			      void *data, int len)
{
	ASSERT(wdev);
	return mtk_wlan_cfg_testmode_cmd(wiphy, wdev, data, len);
}
#else
int mtk_cfg80211_testmode_cmd(struct wiphy *wiphy,
			      void *data, int len)
{
	return mtk_wlan_cfg_testmode_cmd(wiphy, NULL, data, len);
}
#endif
#endif

#if CFG_SUPPORT_SCHED_SCAN
int mtk_cfg80211_sched_scan_start(struct wiphy *wiphy,
			  struct net_device *ndev,
			  struct cfg80211_sched_scan_request *request)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t i, j = 0, u4BufLen;
	struct PARAM_SCHED_SCAN_REQUEST *prSchedScanRequest;
	uint32_t num = 0;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (prGlueInfo->prAdapter == NULL) {
		DBGLOG(REQ, ERROR, "prGlueInfo->prAdapter is NULL");
		return -EINVAL;
	}

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_AIS(prGlueInfo->prAdapter, ucBssIndex))
		return -EINVAL;

	if (likely(request)) {
		scanlog_dbg(LOG_SCHED_SCAN_REQ_START_K2D, INFO, "ssid(%d)match(%d)ch(%u)f(%u)rssi(%d)\n",
		       request->n_ssids, request->n_match_sets,
		       request->n_channels, request->flags,
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
		       request->min_rssi_thold);
#else
		       request->rssi_thold);
#endif
	} else
		scanlog_dbg(LOG_SCHED_SCAN_REQ_START_K2D, INFO, "--> %s()\n",
			__func__);

#if CFG_SUPPORT_LOWLATENCY_MODE
	if (!prGlueInfo->prAdapter->fgEnCfg80211Scan
	    && MEDIA_STATE_CONNECTED
	    == kalGetMediaStateIndicated(prGlueInfo, ucBssIndex)) {
		DBGLOG(REQ, INFO,
		       "sched_scan_start LowLatency reject scan\n");
		return -EBUSY;
	}
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

	if (prGlueInfo->prSchedScanRequest != NULL) {
		DBGLOG(SCN, ERROR,
		       "GlueInfo->prSchedScanRequest != NULL\n");
		return -EBUSY;
	} else if (request == NULL) {
		DBGLOG(SCN, ERROR, "request == NULL\n");
		return -EINVAL;
	} else if (!request->n_match_sets) {
		/* invalid scheduled scan request */
		DBGLOG(SCN, ERROR,
		       "No match sets. No need to do sched scan\n");
		return -EINVAL;
	} else if (request->n_match_sets >
		   CFG_SCAN_SSID_MATCH_MAX_NUM) {
		DBGLOG(SCN, WARN, "request->n_match_sets(%d) > %d\n",
		       request->n_match_sets,
		       CFG_SCAN_SSID_MATCH_MAX_NUM);
		return -EINVAL;
	} else if (request->n_ssids >
		   CFG_SCAN_HIDDEN_SSID_MAX_NUM) {
		DBGLOG(SCN, WARN, "request->n_ssids(%d) > %d\n",
		       request->n_ssids, CFG_SCAN_HIDDEN_SSID_MAX_NUM);
		return -EINVAL;
	}

	prSchedScanRequest = (struct PARAM_SCHED_SCAN_REQUEST *)
		     kalMemAlloc(sizeof(struct PARAM_SCHED_SCAN_REQUEST),
								 VIR_MEM_TYPE);
	if (prSchedScanRequest == NULL) {
		DBGLOG(SCN, ERROR, "prSchedScanRequest kalMemAlloc fail\n");
		return -ENOMEM;
	}
	kalMemZero(prSchedScanRequest,
		   sizeof(struct PARAM_SCHED_SCAN_REQUEST));

	/* passed in the probe_reqs in active scans */
	if (request->ssids) {
		for (i = 0; i < request->n_ssids; i++) {
			DBGLOG(SCN, TRACE, "ssids : (%d)[%s]\n",
			       i, request->ssids[i].ssid);
			/* driver ignored the null ssid */
			if (request->ssids[i].ssid_len == 0
			    || request->ssids[i].ssid[0] == 0)
				DBGLOG(SCN, TRACE, "ignore null ssid(%d)\n", i);
			else {
				struct PARAM_SSID *prSsid;

				prSsid = &(prSchedScanRequest->arSsid[num]);
				COPY_SSID(prSsid->aucSsid, prSsid->u4SsidLen,
					  request->ssids[i].ssid,
					  request->ssids[i].ssid_len);
				num++;
			}
		}
	}
	prSchedScanRequest->u4SsidNum = num;
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
	prSchedScanRequest->i4MinRssiThold =
		request->min_rssi_thold;
#else
	prSchedScanRequest->i4MinRssiThold = request->rssi_thold;
#endif

	num = 0;
	if (request->match_sets) {
		for (i = 0; i < request->n_match_sets; i++) {
			DBGLOG(SCN, TRACE, "match : (%d)[%s]\n", i,
			       request->match_sets[i].ssid.ssid);
			/* driver ignored the null ssid */
			if (request->match_sets[i].ssid.ssid_len == 0
			    || request->match_sets[i].ssid.ssid[0] == 0)
				DBGLOG(SCN, TRACE, "ignore null ssid(%d)\n", i);
			else {
				struct PARAM_SSID *prSsid =
					&(prSchedScanRequest->arMatchSsid[num]);

				COPY_SSID(prSsid->aucSsid,
					  prSsid->u4SsidLen,
					  request->match_sets[i].ssid.ssid,
					  request->match_sets[i].ssid.ssid_len);
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
				prSchedScanRequest->ai4RssiThold[i] =
					request->match_sets[i].rssi_thold;
#else
				prSchedScanRequest->ai4RssiThold[i] =
					request->rssi_thold;
#endif
				num++;
			}
		}
	}
	prSchedScanRequest->u4MatchSsidNum = num;

	if (kalSchedScanParseRandomMac(ndev, request,
		prSchedScanRequest->aucRandomMac,
		prSchedScanRequest->aucRandomMacMask)) {
		prSchedScanRequest->ucScnFuncMask |= ENUM_SCN_RANDOM_MAC_EN;
	}

	prSchedScanRequest->u4IELength = request->ie_len;
	if (request->ie_len > 0) {
		prSchedScanRequest->pucIE =
			kalMemAlloc(request->ie_len, VIR_MEM_TYPE);
		if (prSchedScanRequest->pucIE == NULL) {
			DBGLOG(SCN, ERROR, "pucIE kalMemAlloc fail\n");
		} else {
			kalMemZero(prSchedScanRequest->pucIE, request->ie_len);
			kalMemCopy(prSchedScanRequest->pucIE,
				   (uint8_t *)request->ie, request->ie_len);
		}
	}

#if KERNEL_VERSION(4, 4, 0) <= CFG80211_VERSION_CODE
	prSchedScanRequest->u2ScanInterval =
		(uint16_t) (request->scan_plans->interval);
#else
	prSchedScanRequest->u2ScanInterval = (uint16_t) (
			request->interval);
#endif
	if (request->n_channels > MAXIMUM_OPERATION_CHANNEL_LIST) {
		prSchedScanRequest->ucChnlNum = 0;
		DBGLOG(REQ, INFO,
		       "Channel list (%u->%u) exceed maximum support.\n",
		       request->n_channels, prSchedScanRequest->ucChnlNum);
	} else {
		for (i = 0; i < request->n_channels; i++) {
			uint32_t u4channel =
				nicFreq2ChannelNum(request->channels[i]->
							center_freq * 1000);
			if (u4channel == 0) {
				DBGLOG(REQ, WARN, "Wrong Channel[%d] freq=%u\n",
				       i, request->channels[i]->center_freq);
				continue;
			}
			prSchedScanRequest->aucChannel[j].ucChannelNum =
								u4channel;
			switch ((request->channels[i])->band) {
			case KAL_BAND_2GHZ:
				prSchedScanRequest->aucChannel[j].ucBand =
								BAND_2G4;
				break;
			case KAL_BAND_5GHZ:
				prSchedScanRequest->aucChannel[j].ucBand =
								BAND_5G;
				break;
#if (CFG_SUPPORT_WIFI_6G == 1)
			case KAL_BAND_6GHZ:
				prSchedScanRequest->aucChannel[j].ucBand =
								BAND_6G;
				break;
#endif
			default:
				DBGLOG(REQ, WARN, "UNKNOWN Band %d(chnl=%u)\n",
				       request->channels[i]->band, u4channel);
				prSchedScanRequest->aucChannel[j].ucBand =
								BAND_NULL;
				break;
			}
			j++;
		}
		prSchedScanRequest->ucChnlNum = j;
	}
	prSchedScanRequest->ucBssIndex = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, wlanoidSetStartSchedScan,
			   prSchedScanRequest,
			   sizeof(struct PARAM_SCHED_SCAN_REQUEST),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "scheduled scan error:%x\n", rStatus);
		kalMemFree(prSchedScanRequest->pucIE,
			VIR_MEM_TYPE, request->ie_len);
		kalMemFree(prSchedScanRequest,
			VIR_MEM_TYPE, sizeof(struct PARAM_SCHED_SCAN_REQUEST));
		return -EINVAL;
	}

	/* prSchedScanRequest is owned by oid now, don't free it */

	return 0;
}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_sched_scan_stop(struct wiphy *wiphy,
				 struct net_device *ndev,
				 u64 reqid)
#else
int mtk_cfg80211_sched_scan_stop(struct wiphy *wiphy,
				 struct net_device *ndev)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	/* check if there is any pending scan/sched_scan not yet finished */
	if (prGlueInfo->prSchedScanRequest == NULL)
		return -EPERM; /* Operation not permitted */

	rStatus = kalIoctl(prGlueInfo, wlanoidSetStopSchedScan,
			   NULL, 0, &u4BufLen);

	if (rStatus == WLAN_STATUS_FAILURE) {
		DBGLOG(REQ, WARN, "scheduled scan error in IoCtl:%x\n",
		       rStatus);
		return 0;
	} else if (rStatus == WLAN_STATUS_RESOURCES) {
		DBGLOG(REQ, WARN, "scheduled scan error in Driver:%x\n",
		       rStatus);
		return -EINVAL;
	}

	return 0;
}
#endif /* CFG_SUPPORT_SCHED_SCAN */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for handling association request
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_assoc(struct wiphy *wiphy,
	       struct net_device *ndev, struct cfg80211_assoc_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t arBssid[PARAM_MAC_ADDR_LEN];
#if CFG_SUPPORT_PASSPOINT
	uint8_t *prDesiredIE = NULL;
#endif /* CFG_SUPPORT_PASSPOINT */
	uint32_t rStatus;
	uint32_t u4BufLen = 0;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	kalMemZero(arBssid, MAC_ADDR_LEN);
	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidQueryBssid,
			arBssid, sizeof(arBssid), &u4BufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS || u4BufLen != MAC_ADDR_LEN)
		return -EINVAL;

	/* 1. check BSSID */
	if (UNEQUAL_MAC_ADDR(arBssid, req->bss->bssid)) {
		/* wrong MAC address */
		DBGLOG(REQ, WARN,
		       "incorrect BSSID: [" MACSTR
		       "] currently connected BSSID["
		       MACSTR "]\n",
		       MAC2STR(req->bss->bssid), MAC2STR(arBssid));
		return -ENOENT;
	}

	if (req->ie && req->ie_len > 0) {
#if CFG_SUPPORT_PASSPOINT
		if (wextSrchDesiredHS20IE((uint8_t *) req->ie, req->ie_len,
					  (uint8_t **) &prDesiredIE)) {
			rStatus = kalIoctlByBssIdx(prGlueInfo,
					   wlanoidSetHS20Info,
					   prDesiredIE, IE_SIZE(prDesiredIE),
					   &u4BufLen, ucBssIndex);
			if (rStatus != WLAN_STATUS_SUCCESS) {
				/* DBGLOG(REQ, TRACE,
				 * ("[HS20] set HS20 assoc info error:%x\n",
				 * rStatus));
				 */
			}
		}
#endif /* CFG_SUPPORT_PASSPOINT */
	}

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetBssid,
			(void *)req->bss->bssid, MAC_ADDR_LEN,
			&u4BufLen, ucBssIndex);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "set BSSID:%x\n", rStatus);
		return -EINVAL;
	}

	return 0;
}

#if CFG_SUPPORT_NFC_BEAM_PLUS

int mtk_cfg80211_testmode_get_scan_done(struct wiphy
					*wiphy, void *data, int len,
					struct GLUE_INFO *prGlueInfo)
{
	int32_t i4Status = -EINVAL;

#ifdef CONFIG_NL80211_TESTMODE
#define NL80211_TESTMODE_P2P_SCANDONE_INVALID 0
#define NL80211_TESTMODE_P2P_SCANDONE_STATUS 1

	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t READY_TO_BEAM = 0;

	struct sk_buff *skb = NULL;

	ASSERT(wiphy);
	ASSERT(prGlueInfo);

	skb = cfg80211_testmode_alloc_reply_skb(wiphy,
						sizeof(uint32_t));

	/* READY_TO_BEAM =
	 * (UINT_32)(prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo
	 * .fgIsGOInitialDone)
	 * &(!prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo
	 * .fgIsScanRequest);
	 */
	READY_TO_BEAM = 1;
	/* DBGLOG(QM, TRACE,
	 * "NFC:GOInitialDone[%d] and P2PScanning[%d]\n",
	 * prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo
	 * .fgIsGOInitialDone,
	 * prGlueInfo->prAdapter->rWifiVar.prP2pFsmInfo->rScanReqInfo
	 * .fgIsScanRequest));
	 */

	if (!skb) {
		DBGLOG(QM, TRACE, "%s allocate skb failed:%x\n", __func__,
		       rStatus);
		return -ENOMEM;
	}
	{
		u8 __tmp = 0;

		if (unlikely(nla_put(skb, NL80211_TESTMODE_P2P_SCANDONE_INVALID,
		    sizeof(u8), &__tmp) < 0)) {
			kfree_skb(skb);
			return -EINVAL;
		}
	}
	{
		u32 __tmp = READY_TO_BEAM;

		if (unlikely(nla_put(skb, NL80211_TESTMODE_P2P_SCANDONE_STATUS,
		    sizeof(u32), &__tmp) < 0)) {
			kfree_skb(skb);
			return -EINVAL;
		}
	}

	i4Status = cfg80211_testmode_reply(skb);
#else
	DBGLOG(QM, WARN, "CONFIG_NL80211_TESTMODE not enabled\n");
#endif
	return i4Status;
}

#endif

#if CFG_SUPPORT_TDLS

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for changing a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int
mtk_cfg80211_change_station(struct wiphy *wiphy,
			    struct net_device *ndev, const u8 *mac,
			    struct station_parameters *params)
{

	/* return 0; */

	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CMD_PEER_UPDATE rCmdUpdate;
	uint32_t rStatus;
	uint32_t u4BufLen, u4Temp;
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex = 0;
#if (CFG_ADVANCED_80211_MLO == 1) || \
	KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE
	struct link_station_parameters *prLinkParams =
			&(params->link_sta_params);
#else
	struct station_parameters *prLinkParams = params;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* make up command */

	prAdapter = prGlueInfo->prAdapter;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		ucBssIndex);
	if (!prBssInfo || !params || !mac)
		return -EINVAL;

	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_AUTHORIZED))) {
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetAuthorized,
			(void *) mac, MAC_ADDR_LEN, &u4BufLen, ucBssIndex);

		DBGLOG(REQ, INFO, "rStatus: %x", rStatus);
	}

	if (prLinkParams->supported_rates == NULL)
		return 0;

	/* init */
	kalMemZero(&rCmdUpdate, sizeof(rCmdUpdate));
	kalMemCopy(rCmdUpdate.aucPeerMac, mac, 6);

	if (prLinkParams->supported_rates != NULL) {

		u4Temp = prLinkParams->supported_rates_len;
		if (u4Temp > CMD_PEER_UPDATE_SUP_RATE_MAX)
			u4Temp = CMD_PEER_UPDATE_SUP_RATE_MAX;
		kalMemCopy(rCmdUpdate.aucSupRate, prLinkParams->supported_rates,
			   u4Temp);
		rCmdUpdate.u2SupRateLen = u4Temp;
	}

	/*
	 * In supplicant, only recognize WLAN_EID_QOS 46, not 0xDD WMM
	 * So force to support UAPSD here.
	 */
	rCmdUpdate.UapsdBitmap = 0x0F;	/*params->uapsd_queues; */
	rCmdUpdate.UapsdMaxSp = 0;	/*params->max_sp; */

	rCmdUpdate.u2Capability = params->capability;

	if (params->ext_capab != NULL) {

		u4Temp = params->ext_capab_len;
		if (u4Temp > CMD_PEER_UPDATE_EXT_CAP_MAXLEN)
			u4Temp = CMD_PEER_UPDATE_EXT_CAP_MAXLEN;
		kalMemCopy(rCmdUpdate.aucExtCap, params->ext_capab, u4Temp);
		rCmdUpdate.u2ExtCapLen = u4Temp;
	}

	if (prLinkParams->ht_capa != NULL) {

		rCmdUpdate.rHtCap.u2CapInfo = prLinkParams->ht_capa->cap_info;
		rCmdUpdate.rHtCap.ucAmpduParamsInfo =
			prLinkParams->ht_capa->ampdu_params_info;
		rCmdUpdate.rHtCap.u2ExtHtCapInfo =
			prLinkParams->ht_capa->extended_ht_cap_info;
		rCmdUpdate.rHtCap.u4TxBfCapInfo =
			prLinkParams->ht_capa->tx_BF_cap_info;
		rCmdUpdate.rHtCap.ucAntennaSelInfo =
			prLinkParams->ht_capa->antenna_selection_info;
		kalMemCopy(rCmdUpdate.rHtCap.rMCS.arRxMask,
			   prLinkParams->ht_capa->mcs.rx_mask,
			   sizeof(rCmdUpdate.rHtCap.rMCS.arRxMask));

		rCmdUpdate.rHtCap.rMCS.u2RxHighest =
			prLinkParams->ht_capa->mcs.rx_highest;
		rCmdUpdate.rHtCap.rMCS.ucTxParams =
			prLinkParams->ht_capa->mcs.tx_params;
		rCmdUpdate.fgIsSupHt = TRUE;
	}

	/* vht */
	if (prLinkParams->vht_capa != NULL) {
		rCmdUpdate.rVHtCap.u4CapInfo =
				prLinkParams->vht_capa->vht_cap_info;
		rCmdUpdate.rVHtCap.rVMCS.u2RxMcsMap =
				prLinkParams->vht_capa->supp_mcs.rx_mcs_map;
		rCmdUpdate.rVHtCap.rVMCS.u2RxHighest =
				prLinkParams->vht_capa->supp_mcs.rx_highest;
		rCmdUpdate.rVHtCap.rVMCS.u2TxMcsMap =
				prLinkParams->vht_capa->supp_mcs.tx_mcs_map;
		rCmdUpdate.rVHtCap.rVMCS.u2TxHighest =
				prLinkParams->vht_capa->supp_mcs.tx_highest;
		rCmdUpdate.fgIsSupVht = TRUE;
	}

	/* he */
#if KERNEL_VERSION(4, 19, 0) <= CFG80211_VERSION_CODE
#if CFG_SUPPORT_TDLS_11AX
	if (prLinkParams->he_capa != NULL) {
		kalMemCopy(rCmdUpdate.rHeCap.ucHeMacCapInfo,
			prLinkParams->he_capa->mac_cap_info,
			kal_min_t(uint32_t, HE_MAC_CAP_BYTE_NUM,
			sizeof(prLinkParams->he_capa->mac_cap_info)));
		kalMemCopy(rCmdUpdate.rHeCap.ucHePhyCapInfo,
			prLinkParams->he_capa->phy_cap_info,
			kal_min_t(uint32_t, HE_PHY_CAP_BYTE_NUM,
			sizeof(prLinkParams->he_capa->phy_cap_info)));
		rCmdUpdate.fgIsSupHe = TRUE;
	}
#endif
#endif

	/* update a TDLS peer record */
	/* sanity check */
	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_TDLS_PEER)))
		rCmdUpdate.eStaType = STA_TYPE_DLS_PEER;
	rCmdUpdate.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, cnmPeerUpdate, &rCmdUpdate,
			   sizeof(struct CMD_PEER_UPDATE),
			   &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -EINVAL;
	/* for Ch Sw AP prohibit case */
	if (prBssInfo->fgTdlsIsChSwProhibited) {
		/* disable TDLS ch sw function */

		rStatus = kalIoctl(prGlueInfo,
				   TdlsSendChSwControlCmd,
				   &TdlsSendChSwControlCmd,
				   sizeof(struct CMD_TDLS_CH_SW),
				   &u4BufLen);

		DBGLOG(REQ, INFO, "rStatus: %x", rStatus);
	}

	return 0;
}
#else
int
mtk_cfg80211_change_station(struct wiphy *wiphy,
			    struct net_device *ndev, u8 *mac,
			    struct station_parameters *params)
{

	/* return 0; */

	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CMD_PEER_UPDATE rCmdUpdate;
	uint32_t rStatus;
	uint32_t u4BufLen, u4Temp;
	struct ADAPTER *prAdapter;
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* make up command */

	prAdapter = prGlueInfo->prAdapter;
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		ucBssIndex);
	if (!prBssInfo || !params || !mac)
		return -EINVAL;

	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_AUTHORIZED))) {
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetAuthorized,
			(void *) mac, MAC_ADDR_LEN,
			FALSE, &u4BufLen, ucBssIndex);
	}

	if (params->supported_rates == NULL)
		return 0;

	/* init */
	kalMemZero(&rCmdUpdate, sizeof(rCmdUpdate));
	kalMemCopy(rCmdUpdate.aucPeerMac, mac, 6);

	if (params->supported_rates != NULL) {

		u4Temp = params->supported_rates_len;
		if (u4Temp > CMD_PEER_UPDATE_SUP_RATE_MAX)
			u4Temp = CMD_PEER_UPDATE_SUP_RATE_MAX;
		kalMemCopy(rCmdUpdate.aucSupRate, params->supported_rates,
			   u4Temp);
		rCmdUpdate.u2SupRateLen = u4Temp;
	}

	/*
	 * In supplicant, only recognize WLAN_EID_QOS 46, not 0xDD WMM
	 * So force to support UAPSD here.
	 */
	rCmdUpdate.UapsdBitmap = 0x0F;	/*params->uapsd_queues; */
	rCmdUpdate.UapsdMaxSp = 0;	/*params->max_sp; */

	rCmdUpdate.u2Capability = params->capability;

	if (params->ext_capab != NULL) {

		u4Temp = params->ext_capab_len;
		if (u4Temp > CMD_PEER_UPDATE_EXT_CAP_MAXLEN)
			u4Temp = CMD_PEER_UPDATE_EXT_CAP_MAXLEN;
		kalMemCopy(rCmdUpdate.aucExtCap, params->ext_capab, u4Temp);
		rCmdUpdate.u2ExtCapLen = u4Temp;
	}

	if (params->ht_capa != NULL) {

		rCmdUpdate.rHtCap.u2CapInfo = params->ht_capa->cap_info;
		rCmdUpdate.rHtCap.ucAmpduParamsInfo =
			params->ht_capa->ampdu_params_info;
		rCmdUpdate.rHtCap.u2ExtHtCapInfo =
			params->ht_capa->extended_ht_cap_info;
		rCmdUpdate.rHtCap.u4TxBfCapInfo =
			params->ht_capa->tx_BF_cap_info;
		rCmdUpdate.rHtCap.ucAntennaSelInfo =
			params->ht_capa->antenna_selection_info;
		kalMemCopy(rCmdUpdate.rHtCap.rMCS.arRxMask,
			   params->ht_capa->mcs.rx_mask,
			   sizeof(rCmdUpdate.rHtCap.rMCS.arRxMask));

		rCmdUpdate.rHtCap.rMCS.u2RxHighest =
			params->ht_capa->mcs.rx_highest;
		rCmdUpdate.rHtCap.rMCS.ucTxParams =
			params->ht_capa->mcs.tx_params;
		rCmdUpdate.fgIsSupHt = TRUE;
	}
	/* vht */

	if (params->vht_capa != NULL) {
		/* rCmdUpdate.rVHtCap */
		/* rCmdUpdate.rVHtCap */
	}

	/* update a TDLS peer record */
	/* sanity check */
	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_TDLS_PEER)))
		rCmdUpdate.eStaType = STA_TYPE_DLS_PEER;
	rCmdUpdate.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, cnmPeerUpdate, &rCmdUpdate,
			   sizeof(struct CMD_PEER_UPDATE), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	/* for Ch Sw AP prohibit case */
	if (prBssInfo->fgTdlsIsChSwProhibited) {
		/* disable TDLS ch sw function */

		rStatus = kalIoctl(prGlueInfo,
				   TdlsSendChSwControlCmd,
				   &TdlsSendChSwControlCmd,
				   sizeof(struct CMD_TDLS_CH_SW),
				   &u4BufLen);
	}

	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for adding a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_add_station(struct wiphy *wiphy,
			     struct net_device *ndev,
			     const u8 *mac, struct station_parameters *params)
{
	/* return 0; */

	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CMD_PEER_ADD rCmdCreate;
	struct ADAPTER *prAdapter;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* make up command */

	prAdapter = prGlueInfo->prAdapter;

	/* init */
	kalMemZero(&rCmdCreate, sizeof(rCmdCreate));
	kalMemCopy(rCmdCreate.aucPeerMac, mac, 6);

	/* create a TDLS peer record */
	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_TDLS_PEER))) {
		rCmdCreate.eStaType = STA_TYPE_DLS_PEER;
		rCmdCreate.ucBssIdx = ucBssIndex;
		rStatus = kalIoctl(prGlueInfo, cnmPeerAdd, &rCmdCreate,
				   sizeof(struct CMD_PEER_ADD),
				   &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -EINVAL;
	}

	return 0;
}
#else
int mtk_cfg80211_add_station(struct wiphy *wiphy,
			     struct net_device *ndev, u8 *mac,
			     struct station_parameters *params)
{
	/* return 0; */

	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */
	struct GLUE_INFO *prGlueInfo = NULL;
	struct CMD_PEER_ADD rCmdCreate;
	struct ADAPTER *prAdapter;
	uint32_t rStatus;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* make up command */

	prAdapter = prGlueInfo->prAdapter;

	/* init */
	kalMemZero(&rCmdCreate, sizeof(rCmdCreate));
	kalMemCopy(rCmdCreate.aucPeerMac, mac, 6);

	/* create a TDLS peer record */
	if ((params->sta_flags_set & BIT(
		     NL80211_STA_FLAG_TDLS_PEER))) {
		rCmdCreate.eStaType = STA_TYPE_DLS_PEER;
		rCmdCreate.ucBssIdx = ucBssIndex;
		rStatus = kalIoctl(prGlueInfo, cnmPeerAdd, &rCmdCreate,
				   sizeof(struct CMD_PEER_ADD), &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			return -EINVAL;
	}

	return 0;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for deleting a station information
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 *
 * @other
 *		must implement if you have add_station().
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
#if KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
static const u8 bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int mtk_cfg80211_del_station(struct wiphy *wiphy,
			     struct net_device *ndev,
			     struct station_del_parameters *params)
{
	/* fgIsTDLSlinkEnable = 0; */

	/* return 0; */
	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */

	const u8 *mac = params->mac ? params->mac : bcast_addr;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct STA_RECORD *prStaRec;
	u8 deleteMac[MAC_ADDR_LEN];
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	prAdapter = prGlueInfo->prAdapter;

	/* For kernel 3.18 modification, we trasfer to local buff to query
	 * sta
	 */
	memset(deleteMac, 0, MAC_ADDR_LEN);
	memcpy(deleteMac, mac, MAC_ADDR_LEN);

	prStaRec = cnmGetStaRecByAddress(prAdapter,
		 (uint8_t) ucBssIndex, deleteMac);

	if (prStaRec != NULL)
		cnmStaRecFree(prAdapter, prStaRec);

	return 0;
}
#else
int mtk_cfg80211_del_station(struct wiphy *wiphy,
			     struct net_device *ndev, const u8 *mac)
{
	/* fgIsTDLSlinkEnable = 0; */

	/* return 0; */
	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */

	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct STA_RECORD *prStaRec;
	u8 deleteMac[MAC_ADDR_LEN];
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	prAdapter = prGlueInfo->prAdapter;

	/* For kernel 3.18 modification, we trasfer to local buff to query
	 * sta
	 */
	memset(deleteMac, 0, MAC_ADDR_LEN);
	memcpy(deleteMac, mac, MAC_ADDR_LEN);

	prStaRec = cnmGetStaRecByAddress(prAdapter,
		 (uint8_t) ucBssIndex, deleteMac);

	if (prStaRec != NULL)
		cnmStaRecFree(prAdapter, prStaRec);

	return 0;
}
#endif
#else
int mtk_cfg80211_del_station(struct wiphy *wiphy,
			     struct net_device *ndev, u8 *mac)
{
	/* fgIsTDLSlinkEnable = 0; */

	/* return 0; */
	/* from supplicant -- wpa_supplicant_tdls_peer_addset() */

	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct STA_RECORD *prStaRec;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	prAdapter = prGlueInfo->prAdapter;

	prStaRec = cnmGetStaRecByAddress(prAdapter,
			 (uint8_t) ucBssIndex, mac);

	if (prStaRec != NULL)
		cnmStaRecFree(prAdapter, prStaRec);

	return 0;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to transmit a TDLS data frame from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(6, 4, 0) <= CFG80211_VERSION_CODE
int
mtk_cfg80211_tdls_mgmt(struct wiphy *wiphy,
		       struct net_device *dev, const u8 *peer,
		       int link_id, u8 action_code, u8 dialog_token,
		       u16 status_code, u32 peer_capability,
		       bool initiator, const u8 *buf, size_t len)
{
	struct GLUE_INFO *prGlueInfo;
	struct TDLS_CMD_LINK_MGT rCmdMgt;
	uint32_t u4BufLen;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* sanity check */
	if ((wiphy == NULL) || (peer == NULL) || (buf == NULL))
		return -EINVAL;

	/* init */
	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	kalMemZero(&rCmdMgt, sizeof(rCmdMgt));
	rCmdMgt.u2StatusCode = status_code;
	rCmdMgt.u4SecBufLen = len;
	rCmdMgt.ucDialogToken = dialog_token;
	rCmdMgt.ucActionCode = action_code;
	kalMemCopy(&(rCmdMgt.aucPeer), peer, 6);

	if  (len > TDLS_SEC_BUF_LENGTH) {
		DBGLOG(REQ, WARN, "%s:len > TDLS_SEC_BUF_LENGTH\n", __func__);
		return -EINVAL;
	}

	kalMemCopy(&(rCmdMgt.aucSecBuf), buf, len);
	rCmdMgt.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, TdlsexLinkMgt, &rCmdMgt,
		 sizeof(struct TDLS_CMD_LINK_MGT),
		 &u4BufLen);

	DBGLOG(REQ, INFO, "rStatus: %x", rStatus);

	if (rStatus == WLAN_STATUS_SUCCESS)
		return 0;
	else
		return -EINVAL;
}
#elif KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
int
mtk_cfg80211_tdls_mgmt(struct wiphy *wiphy,
		       struct net_device *dev,
		       const u8 *peer, u8 action_code, u8 dialog_token,
		       u16 status_code, u32 peer_capability,
		       bool initiator, const u8 *buf, size_t len)
{
	struct GLUE_INFO *prGlueInfo;
	struct TDLS_CMD_LINK_MGT rCmdMgt;
	uint32_t u4BufLen;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* sanity check */
	if ((wiphy == NULL) || (peer == NULL) || (buf == NULL))
		return -EINVAL;

	/* init */
	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	kalMemZero(&rCmdMgt, sizeof(rCmdMgt));
	rCmdMgt.u2StatusCode = status_code;
	rCmdMgt.u4SecBufLen = len;
	rCmdMgt.ucDialogToken = dialog_token;
	rCmdMgt.ucActionCode = action_code;
	kalMemCopy(&(rCmdMgt.aucPeer), peer, 6);

	if  (len > TDLS_SEC_BUF_LENGTH) {
		DBGLOG(REQ, WARN, "%s:len > TDLS_SEC_BUF_LENGTH\n", __func__);
		return -EINVAL;
	}

	kalMemCopy(&(rCmdMgt.aucSecBuf), buf, len);
	rCmdMgt.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, TdlsexLinkMgt, &rCmdMgt,
		 sizeof(struct TDLS_CMD_LINK_MGT),
		 &u4BufLen);

	DBGLOG(REQ, INFO, "rStatus: %x", rStatus);

	if (rStatus == WLAN_STATUS_SUCCESS)
		return 0;
	else
		return -EINVAL;
}
#elif KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int
mtk_cfg80211_tdls_mgmt(struct wiphy *wiphy,
		       struct net_device *dev,
		       const u8 *peer, u8 action_code, u8 dialog_token,
		       u16 status_code, u32 peer_capability,
		       const u8 *buf, size_t len)
{
	struct GLUE_INFO *prGlueInfo;
	struct TDLS_CMD_LINK_MGT rCmdMgt;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* sanity check */
	if ((wiphy == NULL) || (peer == NULL) || (buf == NULL))
		return -EINVAL;

	/* init */
	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	kalMemZero(&rCmdMgt, sizeof(rCmdMgt));
	rCmdMgt.u2StatusCode = status_code;
	rCmdMgt.u4SecBufLen = len;
	rCmdMgt.ucDialogToken = dialog_token;
	rCmdMgt.ucActionCode = action_code;
	kalMemCopy(&(rCmdMgt.aucPeer), peer, 6);

	if  (len > TDLS_SEC_BUF_LENGTH) {
		DBGLOG(REQ, WARN, "%s:len > TDLS_SEC_BUF_LENGTH\n", __func__);
		return -EINVAL;
	}

	kalMemCopy(&(rCmdMgt.aucSecBuf), buf, len);
	rCmdMgt.ucBssIdx = ucBssIndex;
	kalIoctl(prGlueInfo, TdlsexLinkMgt, &rCmdMgt,
		 sizeof(struct TDLS_CMD_LINK_MGT),
		 &u4BufLen);
	return 0;

}

#else
int
mtk_cfg80211_tdls_mgmt(struct wiphy *wiphy,
		       struct net_device *dev,
		       u8 *peer, u8 action_code, u8 dialog_token,
		       u16 status_code, const u8 *buf, size_t len)
{
	struct GLUE_INFO *prGlueInfo;
	struct TDLS_CMD_LINK_MGT rCmdMgt;
	uint32_t u4BufLen;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	/* sanity check */
	if ((wiphy == NULL) || (peer == NULL) || (buf == NULL))
		return -EINVAL;

	/* init */
	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	kalMemZero(&rCmdMgt, sizeof(rCmdMgt));
	rCmdMgt.u2StatusCode = status_code;
	rCmdMgt.u4SecBufLen = len;
	rCmdMgt.ucDialogToken = dialog_token;
	rCmdMgt.ucActionCode = action_code;
	kalMemCopy(&(rCmdMgt.aucPeer), peer, 6);

	if (len > TDLS_SEC_BUF_LENGTH) {
		DBGLOG(REQ, WARN,
		       "In mtk_cfg80211_tdls_mgmt , len > TDLS_SEC_BUF_LENGTH, please check\n");
		return -EINVAL;
	}

	kalMemCopy(&(rCmdMgt.aucSecBuf), buf, len);
	rCmdMgt.ucBssIdx = ucBssIndex;
	kalIoctl(prGlueInfo, TdlsexLinkMgt, &rCmdMgt,
		 sizeof(struct TDLS_CMD_LINK_MGT),
		 &u4BufLen);
	return 0;

}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to hadel TDLS link from nl80211.
 *
 * \param[in] pvAdapter Pointer to the Adapter structure.
 * \param[in]
 * \param[in]
 * \param[in] buf includes RSN IE + FT IE + Lifetimeout IE
 *
 * \retval WLAN_STATUS_SUCCESS
 * \retval WLAN_STATUS_INVALID_LENGTH
 */
/*----------------------------------------------------------------------------*/
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg80211_tdls_oper(struct wiphy *wiphy,
			   struct net_device *dev,
			   const u8 *peer, enum nl80211_tdls_operation oper)
{

	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen;
	struct ADAPTER *prAdapter;
	struct TDLS_CMD_LINK_OPER rCmdOper;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(REQ, INFO, "ucBssIndex = %d, oper=%d",
		ucBssIndex, oper);

	ASSERT(prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;

	kalMemZero(&rCmdOper, sizeof(rCmdOper));
	kalMemCopy(rCmdOper.aucPeerMac, peer, 6);

	rCmdOper.oper = (enum ENUM_TDLS_LINK_OPER)oper;
	rCmdOper.ucBssIdx = ucBssIndex;
	rStatus = kalIoctl(prGlueInfo, TdlsexLinkOper, &rCmdOper,
			sizeof(struct TDLS_CMD_LINK_OPER),
			&u4BufLen);

	DBGLOG(REQ, INFO, "rStatus: %x", rStatus);

	if (rStatus == WLAN_STATUS_SUCCESS)
		return 0;
	else
		return -EINVAL;
}
#else
int mtk_cfg80211_tdls_oper(struct wiphy *wiphy,
			   struct net_device *dev, u8 *peer,
			   enum nl80211_tdls_operation oper)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4BufLen;
	struct ADAPTER *prAdapter;
	struct TDLS_CMD_LINK_OPER rCmdOper;
	uint8_t ucBssIndex = 0;

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	DBGLOG(REQ, INFO, "ucBssIndex = %d, oper=%d",
		ucBssIndex, oper);

	prAdapter = prGlueInfo->prAdapter;

	kalMemZero(&rCmdOper, sizeof(rCmdOper));
	kalMemCopy(rCmdOper.aucPeerMac, peer, 6);

	rCmdOper.oper = oper;
	rCmdOper.ucBssIdx = ucBssIndex;

	kalIoctl(prGlueInfo, TdlsexLinkOper, &rCmdOper,
			sizeof(struct TDLS_CMD_LINK_OPER),
			&u4BufLen);
	return 0;
}
#endif
#endif

#if (CFG_SUPPORT_SINGLE_SKU == 1)

#if (CFG_BUILT_IN_DRIVER == 1)
/* in kernel-x.x/net/wireless/reg.c */
#else
bool is_world_regdom(const char *alpha2)
{
	if (!alpha2)
		return false;

	return (alpha2[0] == '0') && (alpha2[1] == '0');
}
#endif

void
mtk_reg_notify(struct wiphy *pWiphy,
	       struct regulatory_request *pRequest)
{
	struct GLUE_INFO *prGlueInfo = rlmDomainGetGlueInfo();
	struct ADAPTER *prAdapter;
	uint32_t u4CountryCode = 0;

	if (g_u4HaltFlag) {
		DBGLOG(RLM, ERROR, "wlan is halt, skip reg callback\n");
		return;
	}

	if (!rlmDomainCountryCodeUpdateSanity(
		prGlueInfo, &prAdapter)) {
		DBGLOG(RLM, ERROR, "sanity check failed, skip!\n");
		return;
	}

	DBGLOG(RLM, INFO,
		"request->alpha2=%s, initiator=%x, intersect=%d\n",
		pRequest->alpha2, pRequest->initiator, pRequest->intersect);

	if (rlmDomainIsSameCountryCode(pRequest->alpha2, 2)) {
		char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};

		rlmDomainU32ToAlpha(
			rlmDomainGetCountryCode(), acCountryCodeStr);
		DBGLOG(RLM, WARN,
			"Same as current country %s, skip!\n",
			acCountryCodeStr);
		return;
	}

	rlmDomainSetCountryCode(pRequest->alpha2, 2);

	u4CountryCode = rlmDomainAlpha2ToU32(pRequest->alpha2, 2);

	rlmDomainCountryCodeUpdate(prAdapter, u4CountryCode, 0);

	rlmDomainSetDfsRegion((u8)pRequest->dfs_region);
}

void
cfg80211_regd_set_wiphy(struct wiphy *prWiphy)
{
	/*
	 * register callback
	 */
	prWiphy->reg_notifier = mtk_reg_notify;

	/*
	 * clear REGULATORY_CUSTOM_REG flag
	 */
#if KERNEL_VERSION(3, 14, 0) > CFG80211_VERSION_CODE
	/*tells kernel that assign WW as default*/
	prWiphy->flags &= ~(WIPHY_FLAG_CUSTOM_REGULATORY);
#elif (KERNEL_VERSION(5, 5, 0) > CFG80211_VERSION_CODE) || \
	(CFG_SUPPORT_SINGLE_SKU_FORCE_CUSTOM_REG == 1)
	prWiphy->regulatory_flags &= ~(REGULATORY_CUSTOM_REG);
#else /* KERNEL_VERSION(5, 5, 0) <= CFG80211_VERSION_CODE */
	prWiphy->regulatory_flags &= ~(REGULATORY_WIPHY_SELF_MANAGED);
#endif /* CFG80211_VERSION_CODE */

	/*
	 * set other regulatory_flags
	 */
#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
	/*ignore the hint from IE*/
	prWiphy->regulatory_flags |= REGULATORY_COUNTRY_IE_IGNORE;
#ifdef CFG_SUPPORT_DISABLE_BCN_HINTS
	/*disable beacon hint to avoid channel flag be changed*/
	prWiphy->regulatory_flags |= REGULATORY_DISABLE_BEACON_HINTS;
#endif /* CFG_SUPPORT_DISABLE_BCN_HINTS */
#endif /* CFG80211_VERSION_CODE */

	/*
	 * set REGULATORY_CUSTOM_REG flag
	 */
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)

#if KERNEL_VERSION(3, 14, 0) > CFG80211_VERSION_CODE
	/*tells kernel that assign WW as default*/
	prWiphy->flags |= (WIPHY_FLAG_CUSTOM_REGULATORY);
#elif (KERNEL_VERSION(5, 5, 0) > CFG80211_VERSION_CODE) || \
	(CFG_SUPPORT_SINGLE_SKU_FORCE_CUSTOM_REG == 1)
	prWiphy->regulatory_flags |= (REGULATORY_CUSTOM_REG);
	/* assigned a defautl one */
	if (rlmDomainGetLocalDefaultRegd())
		wiphy_apply_custom_regulatory(prWiphy,
		(const struct ieee80211_regdomain *)
			rlmDomainGetLocalDefaultRegd());
#else /* KERNEL_VERSION(5, 5, 0) <= CFG80211_VERSION_CODE */
	prWiphy->regulatory_flags |= (REGULATORY_WIPHY_SELF_MANAGED);
	/* To prevent wiphy registration failure, and kernel would set
	 * these flags for self managed wiphy when registration.
	 */
	prWiphy->regulatory_flags &= ~(REGULATORY_COUNTRY_IE_IGNORE);
	prWiphy->regulatory_flags &= ~(REGULATORY_DISABLE_BEACON_HINTS);
#endif /* CFG80211_VERSION_CODE */

#endif /* CFG_SUPPORT_SINGLE_SKU_LOCAL_DB */

	/*
	 * Initialize regd control information
	 */
	rlmDomainResetCtrlInfo(FALSE);
}
#else
void
cfg80211_regd_set_wiphy(struct wiphy *prWiphy)
{
}
#endif

int testmode_disable_tdls_ps(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
#if CFG_SUPPORT_TDLS
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Ret = -1;
	uint32_t rStatus;
	uint8_t ucIsEnablePs = 0;
	uint32_t u4SetInfoLen = 0;

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	i4Ret = kalkStrtou8(apcArgv[1], 0, &ucIsEnablePs);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "Parse ucIsEnablePs error: %d\n",
		       i4Ret);
		return WLAN_STATUS_INVALID_DATA;
	}

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidDisableTdlsPs,
			   &ucIsEnablePs, sizeof(ucIsEnablePs),
			   &u4SetInfoLen);
	return rStatus;
#else
	DBGLOG(REQ, WARN, "not support tdls\n");
	return -EOPNOTSUPP;
#endif
}

int testmode_neighbor_request(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t rStatus;
	uint8_t *pucSSID = NULL;
	uint32_t u4SSIDLen = 0;
	uint8_t ucBssIndex = 0;
	uint32_t u4SetInfoLen = 0;

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (kalStrniCmp(apcArgv[1], "SSID=", 5) == 0) {
		pucSSID = apcArgv[1] + 5;
		u4SSIDLen = kalStrLen(apcArgv[1]) - 5;
		DBGLOG(REQ, INFO, "cmd=%s, ssid len=%u, ssid=%s\n",
			pcCommand, u4SSIDLen, HIDE(pucSSID));
	}

	rStatus = kalIoctlByBssIdx(prGlueInfo,
			   wlanoidSendNeighborRequest,
			   (void *)pucSSID, u4SSIDLen,
			   &u4SetInfoLen, ucBssIndex);

	return rStatus;
}

int testmode_bss_tran_query(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint32_t rStatus;
	uint8_t *pucReason = NULL;
	uint32_t u4ReasonLen = 0;
	uint8_t ucBssIndex = 0;
	uint32_t u4SetInfoLen = 0;

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (kalStrniCmp(apcArgv[1], "reason=", 7) == 0) {
		pucReason = apcArgv[1] + 7;
		u4ReasonLen = kalStrLen(apcArgv[1]) - 7;
		DBGLOG(REQ, INFO, "cmd=%s, reason len=%u, reason=%s\n",
			pcCommand, u4ReasonLen, pucReason);
	}

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSendBTMQuery,
			   (void *)pucReason, u4ReasonLen,
			   &u4SetInfoLen, ucBssIndex);

	return rStatus;
}

int testmode_osharemod(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
#if CFG_SUPPORT_OSHARE
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Ret = -1;
	uint32_t rStatus;
	struct OSHARE_MODE_T cmdBuf;
	struct OSHARE_MODE_T *pCmdHeader = NULL;
	struct OSHARE_MODE_SETTING_V1_T *pCmdData = NULL;
	uint8_t ucBssIndex = 0;
	uint32_t u4SetInfoLen = 0;

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	kalMemZero(&cmdBuf, sizeof(cmdBuf));

	pCmdHeader = &cmdBuf;
	pCmdHeader->cmdVersion = OSHARE_MODE_CMD_V1;
	pCmdHeader->cmdType = 1; /*1-set   0-query*/
	pCmdHeader->magicCode = OSHARE_MODE_MAGIC_CODE;
	pCmdHeader->cmdBufferLen = MAX_OSHARE_MODE_LENGTH;

	pCmdData = (struct OSHARE_MODE_SETTING_V1_T *) &
		   (pCmdHeader->buffer[0]);
	i4Ret = kalkStrtou8(apcArgv[1], 0, &pCmdData->osharemode);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "Parse osharemode error: %d\n", i4Ret);
		return WLAN_STATUS_INVALID_DATA;
	}

	DBGLOG(REQ, INFO, "cmd=%s, osharemode=%u\n",
		pcCommand, pCmdData->osharemode);

	rStatus = kalIoctl(prGlueInfo,
			   wlanoidSetOshareMode,
			   &cmdBuf,
			   sizeof(struct OSHARE_MODE_T),
			   &u4SetInfoLen);

	if (rStatus == WLAN_STATUS_SUCCESS)
		prGlueInfo->prAdapter->fgEnOshareMode
			= pCmdData->osharemode;

	return rStatus;
#else
	DBGLOG(REQ, WARN, "not support OSHAREMOD\n");
	return -EOPNOTSUPP;
#endif
}

int testmode_cmd_example(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	char tmp[] = "CMD_RESPONSE";

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	return mtk_cfg80211_process_str_cmd_reply(wiphy, tmp, sizeof(tmp));
}

int testmode_reassoc(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	uint32_t u4FreqInfo = 0;
	uint32_t u4SetInfoLen = 0;
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	int32_t i4Ret = -1;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_CONNECT rNewSsid;
	struct CONNECTION_SETTINGS *prConnSettings = NULL;
	uint8_t bssid[MAC_ADDR_LEN];
	uint8_t ucSSIDLen;
	uint8_t aucSSID[ELEM_MAX_LEN_SSID] = {0};
	uint8_t ucBssIndex = 0;
	uint16_t u2LinkIdBitmap = 0xFFFF;

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	prConnSettings = aisGetConnSettings(prGlueInfo->prAdapter, ucBssIndex);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 3) {
		DBGLOG(REQ, TRACE, "argc is %i, cmd is %s, %s\n", i4Argc,
		       apcArgv[1], apcArgv[2]);
		i4Ret = kalkStrtou32(apcArgv[2], 0, &u4FreqInfo);
		if (i4Ret) {
			DBGLOG(REQ, ERROR, "parse u4Param error %d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}
		/* copy ssd to local instead of assigning
		 * prConnSettings->aucSSID to rNewSsid.pucSsid because
		 * wlanoidSetConnect will copy ssid from rNewSsid again
		 */
		COPY_SSID(aucSSID, ucSSIDLen,
			prConnSettings->aucSSID, prConnSettings->ucSSIDLen);

		wlanHwAddrToBin(apcArgv[1], bssid);
		if (i4Argc >= 4)
			i4Ret = kalkStrtou16(apcArgv[3], 0, &u2LinkIdBitmap);
		if (i4Ret) {
			DBGLOG(REQ, ERROR,
				"parse u2LinkIdBitmap error %d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}

		kalMemZero(&rNewSsid, sizeof(rNewSsid));
		rNewSsid.u4CenterFreq = u4FreqInfo;
		rNewSsid.pucBssid = NULL;
		rNewSsid.pucBssidHint = bssid;
		rNewSsid.pucSsid = aucSSID;
		rNewSsid.u4SsidLen = ucSSIDLen;
		rNewSsid.ucBssIdx = ucBssIndex;
		rNewSsid.fgTestMode = TRUE;
		rNewSsid.u2LinkIdBitmap = u2LinkIdBitmap;

		DBGLOG(INIT, INFO,
		   "Reassoc ssid=%s(%d) bssid= " MACSTR
		   " freq=%d LinkIdBitmap=%d\n",
		   rNewSsid.pucSsid, rNewSsid.u4SsidLen,
		   MAC2STR(bssid), u4FreqInfo, u2LinkIdBitmap);

		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetConnect,
			   (void *)&rNewSsid, sizeof(struct PARAM_CONNECT),
			   &u4SetInfoLen, ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR,
			       "reassoc fail 0x%x\n", rStatus);
		else
			DBGLOG(INIT, TRACE,
			       "reassoc successed\n");
	} else {
		DBGLOG(REQ, ERROR, "reassoc failed\n");
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;
}

int testmode_set_ax_blocklist(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus;
	uint32_t u4BufLen;
	struct PARAM_AX_BLOCKLIST rBlocklist = { 0 };
	int32_t i4BytesWritten = -1;
	uint8_t ucType = 0;
	uint8_t i = 0;
	uint8_t aucMacAddr[MAC_ADDR_LEN] = { 0 };
	uint8_t index = 0;
	int32_t i4Ret = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc >= 2) {
		DBGLOG(REQ, TRACE, "argc %i, cmd [%s]\n", i4Argc, apcArgv[1]);
		i4BytesWritten = kalkStrtou8(apcArgv[1], 0, &ucType);
		if (i4BytesWritten)
			DBGLOG(REQ, ERROR, "parse ucType error %d\n",
					i4BytesWritten);


		rBlocklist.ucType = ucType;
		rBlocklist.ucCount = (i4Argc - 2);
		for (i = 2; i < i4Argc; i++) {
			DBGLOG(REQ, TRACE,
				"argc %i, cmd [%s]\n", i4Argc, apcArgv[i]);
			i4Ret = wlanHwAddrToBin(apcArgv[i], aucMacAddr);
			if (i4Ret != 17) {
				DBGLOG(REQ, WARN,
				    "BSSID format is wrong! i4Ret=%d\n", i4Ret);
				continue;
			}

			if (index >= ARRAY_SIZE(rBlocklist.aucList)) {
				DBGLOG(REQ, WARN,
				    "Could only set %d BSSID in blocklist!\n",
				    ARRAY_SIZE(rBlocklist.aucList));
				rBlocklist.ucCount =
					ARRAY_SIZE(rBlocklist.aucList);
				break;
			}
			COPY_MAC_ADDR(&rBlocklist.aucList[index],
					aucMacAddr);
			index++;
		}
		rStatus = kalIoctl(prGlueInfo, wlanoidSetAxBlocklist,
			&rBlocklist, sizeof(struct PARAM_AX_BLOCKLIST),
			&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR, "fail 0x%x\n", rStatus);

	} else {
		DBGLOG(REQ, ERROR, "fail invalid data\n");
		rStatus = WLAN_STATUS_INVALID_DATA;
	}
	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief For blocklist customization, Usage:
 *        SET_CUS_BLOCKLIST "0xa" "SSID" "BSSID" "Freq""Band" "Reason"
 *        "Type" "Timeout"
 *
 *        0xa: Limit rule, BIT(0): SSID, BIT(1): BSSID, BIT(2): Freq,
 *             BIT(3): Band, the following four parameters should be
 *             assigned individual values.
 *        Reason: Reserved.
 *        Type: Limit type, BIT(0): first connection, BIT(1): roaming.
 *        Timeout: Remove timeout (Unit: s).
 * @param
 *
 * @retval WLAN_STATUS_SUCCESS:  successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/

int testmode_set_cus_blocklist(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0, i4Ret = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t rStatus, u4BufLen;
	uint8_t ucType, ucBand;
	struct PARAM_CUS_BLOCKLIST rCusBlocklist = { 0 };

	DBGLOG(INIT, TRACE, "command is %s\n", pcCommand);
	WIPHY_PRIV(wiphy, prGlueInfo);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 9) {
		/* Parse customized blocklist type */
		i4Ret = kalkStrtou8(apcArgv[1], 0, &ucType);
		if (i4Ret) {
			DBGLOG(REQ, ERROR, "Parse ucType fail! %d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}
		rCusBlocklist.ucType = ucType;

		if (ucType & BIT(CUS_BLOCKLIST_TYPE_SSID)) {
			COPY_SSID(rCusBlocklist.rSSID.aucSsid,
				  rCusBlocklist.rSSID.u4SsidLen,
				  apcArgv[2],
				  kalStrLen(apcArgv[2]));
		}

		if (ucType & BIT(CUS_BLOCKLIST_TYPE_BSSID)) {
			i4Ret = wlanHwAddrToBin(apcArgv[3],
						rCusBlocklist.aucBSSID);
			if (i4Ret != 17) {
				DBGLOG(REQ, WARN,
					"Parse BSSID fail! i4Ret=%d\n", i4Ret);
				return WLAN_STATUS_INVALID_DATA;
			}
		}

		if (ucType & BIT(CUS_BLOCKLIST_TYPE_FREQUENCY)) {
			i4Ret = kalkStrtou32(apcArgv[4], 0,
						&rCusBlocklist.u4Frequency);
			if (i4Ret) {
				DBGLOG(REQ, WARN,
				    "Parse FREQUENCY fail! i4Ret=%d\n", i4Ret);
				return WLAN_STATUS_INVALID_DATA;
			}
		}

		if (ucType & BIT(CUS_BLOCKLIST_TYPE_BAND)) {
			i4Ret = kalkStrtou8(apcArgv[5], 0, &ucBand);
			if (i4Ret) {
				DBGLOG(REQ, WARN,
					"Parse BAND fail! i4Ret=%d\n", i4Ret);
				return WLAN_STATUS_INVALID_DATA;
			}
			rCusBlocklist.eBand = ucBand < BAND_NUM ?
							ucBand : BAND_NULL;
		}

		i4Ret = kalkStrtou8(apcArgv[6], 0,
					&rCusBlocklist.ucLimitReason);
		if (i4Ret) {
			DBGLOG(REQ, WARN,
				"Parse limitReason fail! i4Ret=%d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}

		i4Ret = kalkStrtou8(apcArgv[7], 0, &rCusBlocklist.ucLimitType);
		if (i4Ret) {
			DBGLOG(REQ, WARN,
				"Parse limitType fail! i4Ret=%d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}

		i4Ret = kalkStrtou32(apcArgv[8], 0,
					&rCusBlocklist.u4LimitTimeout);
		if (i4Ret) {
			DBGLOG(REQ, WARN,
				"Parse limitTimeout fail! i4Ret=%d\n", i4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}

		rStatus = kalIoctl(prGlueInfo, wlanoidSetCusBlocklist,
			  &rCusBlocklist, sizeof(struct PARAM_CUS_BLOCKLIST),
			  &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR, "fail 0x%x\n", rStatus);

	} else {
		DBGLOG(REQ, ERROR, "fail invalid data\n");
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;
}

int testmode_set_report_vendor_specified(struct wiphy *wiphy,
		struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0, i4BytesWritten = -1;
	uint32_t u4SetInfoLen = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = {0};
	uint8_t ucParam = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	DBGLOG(INIT, TRACE, "Report vendor specified frame: %s (%d)(%s)\n",
		pcCommand, i4Argc, apcArgv[1]);

	if (i4Argc == 2) {
		i4BytesWritten = kalkStrtou8(apcArgv[1], 0, &ucParam);
		if (i4BytesWritten) {
			DBGLOG(REQ, ERROR, "Parsing failed(%d)\n",
			       i4BytesWritten);
			i4BytesWritten = -1;
		} else {
			rStatus = kalIoctl(prGlueInfo,
					   wlanoidEnableVendorSpecifiedRpt,
					   &ucParam, sizeof(uint8_t),
					   &u4SetInfoLen);

			if (rStatus != WLAN_STATUS_SUCCESS)
				DBGLOG(INIT, ERROR,
				       "Set report VS failed 0x%x\n", rStatus);
			else
				DBGLOG(INIT, TRACE,
				       "Set report VS successed\n");
		}
	}

	return rStatus;
}

static int testmode_force_stbc_mrc(struct GLUE_INFO *prGlueInfo,
	uint8_t ucBssIndex, uint8_t ucType, char *pcCommand, int i4TotalLen)
{
	int32_t i4Argc = 0;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	DBGLOG(REQ, INFO, "[bss = %d] command is %s\n", ucBssIndex, pcCommand);
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);

	if (i4Argc == 2)  {
		struct PARAM_STBC_MRC rParam;
		uint32_t u4Ret, u4BufLen;

		rParam.ucType = ucType;
		rParam.ucBssIndex = ucBssIndex;

		u4Ret = kalkStrtou8(apcArgv[1], 0, &(rParam.fgEnable));
		if (u4Ret) {
			DBGLOG(REQ, ERROR, "parse ucEnable error %d\n", u4Ret);
			return WLAN_STATUS_INVALID_DATA;
		}

		rStatus = kalIoctl(prGlueInfo, wlanoidForceStbcMrc,
			(void *)&rParam, sizeof(struct PARAM_STBC_MRC),
			&u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, ERROR, "fail 0x%x\n", rStatus);

	} else {
		DBGLOG(REQ, ERROR, "fail invalid data\n");
		rStatus = WLAN_STATUS_INVALID_DATA;
	}

	return rStatus;

}

int testmode_force_stbc(struct wiphy *wiphy,
	struct wireless_dev *wdev, char *cmd, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	return testmode_force_stbc_mrc(prGlueInfo, ucBssIndex, 0, cmd, len);
}

int testmode_force_mrc(struct wiphy *wiphy,
	struct wireless_dev *wdev, char *cmd, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucBssIndex = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo == NULL)
		return -EINVAL;

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	return testmode_force_stbc_mrc(prGlueInfo, ucBssIndex, 1, cmd, len);
}

#if CFG_SUPPORT_LLW_SCAN
uint32_t wlanoidSetScanParam(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen)
{
	struct PARAM_SCAN *param;
	struct AIS_FSM_INFO *ais;
	uint8_t ucBssIndex = 0;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pvSetBuffer) {
		DBGLOG(REQ, ERROR, "pvGetBuffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	param = (struct PARAM_SCAN *) pvSetBuffer;

	if (ais->ucLatencyCrtDataMode == 3) {
		DBGLOG(OID, INFO,
			"LATENCY_CRT_DATA = 3, not apply SET_DWELL_TIME\n");
		return WLAN_STATUS_SUCCESS;
	}

	/* To mitigate switch ch overhead,
	 * we reduce dwell time 20ms, MIN is 20ms
	 */
	if (param->ucDfsChDwellTimeMs != 0)
		ais->ucDfsChDwellTimeMs = (param->ucDfsChDwellTimeMs >= 40)
			? param->ucDfsChDwellTimeMs - 20 : 20;
	else
		ais->ucDfsChDwellTimeMs = 0;

	if (param->ucNonDfsChDwellTimeMs != 0)
		ais->ucNonDfsChDwellTimeMs =
			(param->ucNonDfsChDwellTimeMs >= 40)
			? param->ucNonDfsChDwellTimeMs - 20 : 20;
	else
		ais->ucNonDfsChDwellTimeMs = 0;

	ais->u2OpChStayTimeMs = param->u2OpChStayTimeMs;

	if (param->ucNonDfsChDwellTimeMs != 0) {
		ais->ucPerScanChannelCnt =
			param->u2OpChAwayTimeMs / param->ucNonDfsChDwellTimeMs;
	} else
		ais->ucPerScanChannelCnt = 0;

	DBGLOG(OID, INFO,
		"DFS(%d->%d), non-DFS(%d->%d), OpChTime(%d %d), PerScanCh(%d)\n",
		param->ucDfsChDwellTimeMs,
		ais->ucDfsChDwellTimeMs,
		param->ucNonDfsChDwellTimeMs,
		ais->ucNonDfsChDwellTimeMs,
		ais->u2OpChStayTimeMs,
		param->u2OpChAwayTimeMs,
		ais->ucPerScanChannelCnt);

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanoidSetLatencyCrtData(struct ADAPTER *prAdapter,
			    void *pvSetBuffer,
			    uint32_t u4SetBufferLen,
			    uint32_t *pu4SetInfoLen)
{
	uint32_t *pu4Mode;
	struct AIS_FSM_INFO *ais;
	uint8_t ucBssIndex = 0;

	if (!prAdapter) {
		DBGLOG(REQ, ERROR, "prAdapter is NULL\n");
		return WLAN_STATUS_ADAPTER_NOT_READY;
	}

	if (!pvSetBuffer) {
		DBGLOG(REQ, ERROR, "pvGetBuffer is NULL\n");
		return WLAN_STATUS_INVALID_DATA;
	}

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	ais = aisGetAisFsmInfo(prAdapter, ucBssIndex);
	pu4Mode = (uint32_t *) pvSetBuffer;

	ais->ucLatencyCrtDataMode = 0;
	/*
	 * Mode 2: Restrict full roam scan triggered by Firmware
	 *          due to low RSSI.
	 * Mode 3: Restrict off channel time due to full scan to < 40ms
	 */
	ais->ucLatencyCrtDataMode = *pu4Mode;

	if (ais->ucLatencyCrtDataMode == 3) {
		ais->ucDfsChDwellTimeMs = 20;
		ais->ucNonDfsChDwellTimeMs = 35;
		ais->u2OpChStayTimeMs = 0;
		ais->ucPerScanChannelCnt = 1;
	} else if (ais->ucLatencyCrtDataMode == 0) {
		ais->ucDfsChDwellTimeMs = 0;
		ais->ucNonDfsChDwellTimeMs = 0;
		ais->u2OpChStayTimeMs = 0;
		ais->ucPerScanChannelCnt = 0;
	}

	return WLAN_STATUS_SUCCESS;
}

int testmode_set_scan_param(struct wiphy *wiphy,
	struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4SetInfoLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_SCAN param;

	WIPHY_PRIV(wiphy, prGlueInfo);

	/*ex: wpa_cli driver SET_DWELL_TIME W X Y Z*/
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	if (i4Argc != 5) {
		DBGLOG(REQ, ERROR,
			"Error input parameters(%d):%s\n", i4Argc, pcCommand);
		return WLAN_STATUS_INVALID_DATA;
	}

	kalMemZero(&param, sizeof(struct PARAM_SCAN));

	if (kalkStrtou8(apcArgv[1], 0, &param.ucDfsChDwellTimeMs)) {
		DBGLOG(REQ, LOUD, "DfsDwellTime parse %s err\n", apcArgv[1]);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (kalkStrtou16(apcArgv[2], 0, &param.u2OpChStayTimeMs)) {
		DBGLOG(REQ, LOUD, "OpChStayTimeMs parse %s err\n", apcArgv[2]);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (kalkStrtou8(apcArgv[3], 0, &param.ucNonDfsChDwellTimeMs)) {
		DBGLOG(REQ, LOUD,
			"NonDfsDwellTimeMs parse %s err\n", apcArgv[3]);
		return WLAN_STATUS_INVALID_DATA;
	}

	if (kalkStrtou16(apcArgv[4], 0, &param.u2OpChAwayTimeMs)) {
		DBGLOG(REQ, LOUD, "OpChAwayTimeMs parse %s err\n", apcArgv[4]);
		return WLAN_STATUS_INVALID_DATA;
	}

	rStatus = kalIoctl(prGlueInfo, wlanoidSetScanParam,
			&param, sizeof(struct PARAM_SCAN),
			&u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR,
		       "SET_SCAN_PARAM fail 0x%x\n", rStatus);
	else
		DBGLOG(INIT, TRACE,
		       "SET_SCAN_PARAM pass\n");

	return rStatus;
}

int testmode_set_latency_crt_data(struct wiphy *wiphy,
	struct wireless_dev *wdev, char *pcCommand, int i4TotalLen)
{
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX] = { 0 };
	int32_t i4Argc = 0, i4Ret = -1;
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4SetInfoLen = 0, u4Mode = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	/*ex: wpa_cli driver SET_LATENCY_CRT_DATA X */
	wlanCfgParseArgument(pcCommand, &i4Argc, apcArgv);
	if (i4Argc != 2) {
		DBGLOG(REQ, ERROR,
			"Error input parameters(%d):%s\n", i4Argc, pcCommand);
		return WLAN_STATUS_INVALID_DATA;
	}

	i4Ret = kalkStrtou32(apcArgv[1], 0, &u4Mode);
	if (i4Ret) {
		DBGLOG(REQ, ERROR, "Set Latency crt mode parse error %d\n",
			i4Ret);
		return WLAN_STATUS_FAILURE;
	}

	/* Do further scan handling for mode 2 and mode 3,
	 * reset if u4Mode == 0
	 */
	if (u4Mode >= 2 || u4Mode == 0) {
		if (u4Mode == 0)
			wlanChipConfigWithType(prGlueInfo->prAdapter,
				pcCommand, 22, CHIP_CONFIG_TYPE_WO_RESPONSE);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetLatencyCrtData,
			&u4Mode, sizeof(uint32_t),
			&u4SetInfoLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, ERROR,
				"SET_CRT_DATA fail 0x%x\n", rStatus);
		else
			DBGLOG(INIT, TRACE,
				"SET_CRT_DATA pass\n");
	} else {
		/* for mode 1 */
		wlanChipConfigWithType(prGlueInfo->prAdapter,
			pcCommand, 22, CHIP_CONFIG_TYPE_WO_RESPONSE);
	}

	return rStatus;
}
#endif

int32_t mtk_cfg80211_process_str_cmd_reply(
	struct wiphy *wiphy, char *data, int len)
{
	struct sk_buff *skb;
	int32_t i4Err = 0;

	skb = cfg80211_vendor_cmd_alloc_reply_skb(wiphy, len);

	if (!skb) {
		DBGLOG(REQ, INFO, "%s allocate skb failed\n", __func__);
		return -ENOMEM;
	}

	i4Err = nla_put_nohdr(skb, len, data);
	if (i4Err) {
		kfree_skb(skb);
		return i4Err;
	}

	return cfg80211_vendor_cmd_reply(skb);
}

int32_t mtk_cfg80211_process_str_cmd(struct wiphy *wiphy,
		struct wireless_dev *wdev, uint8_t *data, int32_t len)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t *cmd = data;
	struct GLUE_INFO *prGlueInfo = NULL;
	STR_CMD_FUNCTION pfHandler = NULL;

	if (data == NULL || len == 0) {
		DBGLOG(INIT, TRACE, "%s data or len is invalid\n", __func__);
		return -EINVAL;
	}

	DBGLOG(REQ, INFO, "cmd: %s, len: %d\n", cmd, len);
	if (kalIsResetOnEnd() == TRUE) {
		DBGLOG(INIT, WARN, "WiFi is resetting\n");
		return -EBUSY;
	}

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
	    WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	pfHandler = get_str_cmd_handler(cmd, len);
	if (pfHandler != NULL) {
		rStatus = pfHandler(wiphy, wdev, cmd, len);
		return rStatus;
	}

#if CFG_EXT_FEATURE
	pfHandler = get_str_cmd_ext_handler(cmd, len);
	if (pfHandler != NULL) {
		rStatus = pfHandler(wiphy, wdev, cmd, len);
		return rStatus;
	}
#endif

	return -EOPNOTSUPP;
}

int mtk_cfg80211_suspend(struct wiphy *wiphy,
			 struct cfg80211_wowlan *wow)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
#if !CFG_ENABLE_WAKE_LOCK
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen;
	GLUE_SPIN_LOCK_DECLARATION();
#endif

	DBGLOG(REQ, TRACE, "mtk_cfg80211_suspend\n");

#if (CFG_SUPPORT_STATISTICS == 1)
	wlanWakeDumpRes();
#endif
	if (kalHaltTryLock())
		return 0;

	if (kalIsHalted() || !wiphy)
		goto end;

	WIPHY_PRIV(wiphy, prGlueInfo);


	if (prGlueInfo && prGlueInfo->prAdapter) {
		prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

#if !CFG_ENABLE_WAKE_LOCK

		if (IS_FEATURE_ENABLED(prWifiVar->ucWow)) {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			if (prGlueInfo->prScanRequest) {
				kalCfg80211ScanDone(prGlueInfo->prScanRequest,
					TRUE);
				prGlueInfo->prScanRequest = NULL;
			}
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

			/* AIS flow: disassociation if wow_en=0 */
			DBGLOG(REQ, INFO, "Enter AIS pre-suspend\n");
			rStatus = kalIoctl(prGlueInfo, wlanoidAisPreSuspend,
					NULL, 0, &u4BufLen);

			/* TODO: p2pProcessPreSuspendFlow
			 * In current design, only support AIS connection during suspend only.
			 * It need to add flow to deactive P2P (GC/GO) link during suspend flow.
			 * Otherwise, MT7668 would fail to enter deep sleep.
			 */
		}
#endif

		set_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON,
			&prGlueInfo->prAdapter->ulSuspendFlag);
		set_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
			&prGlueInfo->prAdapter->ulSuspendFlag);
		halSetSuspendFlagToFw(prGlueInfo->prAdapter, TRUE);
	}
end:
	kalHaltUnlock();

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief cfg80211 resume callback, will be invoked in wiphy_resume.
 *
 * @param wiphy: pointer to wiphy
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*----------------------------------------------------------------------------*/
int mtk_cfg80211_resume(struct wiphy *wiphy)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t rStatus, u4InfoLen;

	DBGLOG(REQ, TRACE, "mtk_cfg80211_resume\n");

	if (kalHaltTryLock())
		return 0;

	if (kalIsHalted() || !wiphy)
		goto end;

	WIPHY_PRIV(wiphy, prGlueInfo);
	if (prGlueInfo)
		prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL)
		goto end;

	clear_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON,
		  &prAdapter->ulSuspendFlag);

	clear_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
		  &prAdapter->ulSuspendFlag);

	rStatus = kalIoctl(prGlueInfo, wlanoidIndicateBssInfo,
			(void *) NULL, 0, &u4InfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, WARN, "ScanResultLog error:%x\n",
		       rStatus);
	halSetSuspendFlagToFw(prGlueInfo->prAdapter, FALSE);
	fw_log_handler();
end:
	kalHaltUnlock();

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check the net device is P2P net device (P2P GO/GC, AP), or not.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : the net device
 *
 * @retval 0:  AIS device (STA/IBSS)
 *         1:  P2P GO/GC, AP
 */
/*----------------------------------------------------------------------------*/
int mtk_IsP2PNetDevice(struct GLUE_INFO *prGlueInfo,
		       struct net_device *ndev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	int iftype = 0;
	int ret = 1;

	if (ndev == NULL) {
		DBGLOG(REQ, WARN, "ndev is NULL\n");
		return -1;
	}

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			  netdev_priv(ndev);
	iftype = ndev->ieee80211_ptr->iftype;

	/* P2P device/GO/GC always return 1 */
	if (prNetDevPrivate->ucIsP2p == TRUE)
		ret = 1;
	else if (iftype == NL80211_IFTYPE_STATION)
		ret = 0;
	else if (iftype == NL80211_IFTYPE_ADHOC)
		ret = 0;
#if CFG_SUPPORT_NAN
	else if (prNetDevPrivate->ucIsNan == TRUE)
		ret = 0;
#endif

	return ret;
}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
int mtk_IsP2PGoNetDevice(
	struct GLUE_INFO *prGlueInfo,
	struct net_device *dev)
{
#if CFG_SUPPORT_TDLS_P2P
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo = NULL;

	do {
		if (dev == NULL) {
			DBGLOG(REQ, WARN, "ndev is NULL\n");
			break;
		}

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
			break;

		prBssInfo =
			GET_BSS_INFO_BY_INDEX(
			prGlueInfo->prAdapter,
			ucBssIdx);

		if (prBssInfo && IS_BSS_APGO(prBssInfo))
			return 1;
	} while (FALSE);

	return 0;
#else
	return mtk_IsP2PNetDevice(prGlueInfo, dev);
#endif
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the AIS related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : the net device
 *        ucBssIdx   : the AIS BSS index adssigned by the driver (wlanProbe)
 *
 * @retval 0
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_init_sta_role(struct ADAPTER *prAdapter,
		      struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo;
	uint8_t ucBssIndex = 0;

	if ((prAdapter == NULL) || (ndev == NULL))
		return -1;

	prGlueInfo = prAdapter->prGlueInfo;
	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex)) {
		if (ndev != prGlueInfo->prDevHandler)
			return -1;
		/* use default idx for wlan0 */
		ucBssIndex = AIS_DEFAULT_INDEX;
	}

	/* init AIS FSM */
	aisFsmInit(prAdapter, NULL, AIS_INDEX(prAdapter, ucBssIndex));

	ndev->netdev_ops = wlanGetNdevOps();
	ndev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Uninitialize the AIS related FSM and data.
 *
 * @param prAdapter : the driver private data
 *
 * @retval 0
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_uninit_sta_role(struct ADAPTER *prAdapter,
			struct net_device *ndev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNdevPriv = NULL;
	uint8_t ucBssIndex = 0;

	if ((prAdapter == NULL) || (ndev == NULL))
		return -1;

	ucBssIndex = wlanGetBssIdx(ndev);
	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		return -1;

	/* uninit AIS FSM */
	aisFsmUninit(prAdapter, ucBssIndex);

	/* set the ucBssIdx to the illegal value */
	prNdevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(ndev);
	prNdevPriv->ucBssIdx = 0xff;

	return 0;
}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
void mtk_init_monitor_role(struct wiphy *wiphy,
		      struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo;
	uint8_t i = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if ((prGlueInfo == NULL) || (ndev == NULL))
		return;

	DBGLOG(INIT, INFO, "[type:iftype]:[%d:%d]=>[%d:%d]\n",
		ndev->type,
		ndev->ieee80211_ptr->iftype,
		ARPHRD_IEEE80211_RADIOTAP,
		NL80211_IFTYPE_MONITOR);
	ndev->type = ARPHRD_IEEE80211_RADIOTAP;
	ndev->ieee80211_ptr->iftype = NL80211_IFTYPE_MONITOR;
	prGlueInfo->fgIsEnableMon = TRUE;
	prGlueInfo->ucBandIdx = 0xFF;
	prGlueInfo->fgDropFcsErrorFrame = TRUE;
	prGlueInfo->u2Aid = 0;
	for (i = 0; i < CFG_MONITOR_BAND_NUM; i++) {
		prGlueInfo->aucBandIdxEn[i] = 0;
		prGlueInfo->u4AmpduRefNum[i] = 0;
	}
	DBGLOG(INIT, INFO, "enable sniffer mode\n");
}

void mtk_uninit_monitor_role(struct wiphy *wiphy,
			struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo;
	uint8_t i = 0, ucBandIdx;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if ((prGlueInfo == NULL) || (ndev == NULL))
		return;

	if (prGlueInfo->fgIsEnableMon) {
		prGlueInfo->fgIsEnableMon = FALSE;
		/* store band idx */
		ucBandIdx = prGlueInfo->ucBandIdx;

		for (i = 0; i < CFG_MONITOR_BAND_NUM; i++) {
			if (ucBandIdx == 0xFF ||
				prGlueInfo->aucBandIdxEn[i]) {
				/* ucBandIdx will be used to disable monitor */
				prGlueInfo->ucBandIdx = i;
				mtk_cfg80211_set_monitor_channel(wiphy,
					NULL);
			}
		}
		prGlueInfo->ucBandIdx = 0xFF;
	}

	ndev->type = ARPHRD_ETHER;
	ndev->ieee80211_ptr->iftype = NL80211_IFTYPE_STATION;
	DBGLOG(INIT, INFO, "disable sniffer mode\n");
}
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */

#if CFG_ENABLE_WIFI_DIRECT
/*----------------------------------------------------------------------------*/
/*!
 * @brief Initialize the AP (P2P) related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : net device
 *
 * @retval 0      : success
 *         others : can't alloc and setup the AP FSM & data
 *
 */
/*----------------------------------------------------------------------------*/
int mtk_init_ap_role(struct GLUE_INFO *prGlueInfo,
		     struct net_device *ndev)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	uint8_t rMacAddr[PARAM_MAC_ADDR_LEN];
	uint8_t u4Idx = 0;

	GLUE_SPIN_LOCK_DECLARATION();

	for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
		if (gprP2pRoleWdev[u4Idx] == NULL)
			break;
	}

	if (u4Idx >= KAL_P2P_NUM) {
		DBGLOG(INIT, ERROR, "There is no free gprP2pRoleWdev.\n");
		return -ENOMEM;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if ((u4Idx == 0) || (prAdapter == NULL) ||
		(prAdapter->rP2PNetRegState != ENUM_NET_REG_STATE_REGISTERED)) {
		DBGLOG(INIT, ERROR,
		       "The wlan0 can't set to AP without p2p0\n");
		/* System will crash, if p2p0 isn't existing. */
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		return -EFAULT;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	COPY_MAC_ADDR(rMacAddr,
		prAdapter->rWifiVar.aucP2pInterfaceAddress[u4Idx]);

	/* reference from the glRegisterP2P() */
	gprP2pRoleWdev[u4Idx] = ndev->ieee80211_ptr;
	if (glSetupP2P(prGlueInfo, gprP2pRoleWdev[u4Idx], ndev,
		u4Idx, TRUE, TRUE, rMacAddr)) {
		DBGLOG(INIT, ERROR, "glSetupP2P failed\n");
		gprP2pRoleWdev[u4Idx] = NULL;
		return -EFAULT;
	}

	prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum++;

	/* reference from p2pNetRegister() */
	/* The ndev doesn't need register_netdev, only reassign the gPrP2pDev.*/
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	gPrP2pDev[u4Idx] = ndev;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Unnitialize the AP (P2P) related FSM and data.
 *
 * @param prGlueInfo : the driver private data
 *        ndev       : net device
 *
 * @retval 0      : success
 *         others : can't find the AP information by the ndev
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
mtk_oid_uninit_ap_role(struct ADAPTER *prAdapter, void *pvSetBuffer,
	uint32_t u4SetBufferLen, uint32_t *pu4SetInfoLen)
{
	unsigned char u4Idx = 0;
	struct GLUE_INFO *prGlueInfo = NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	if ((prAdapter == NULL) || (pvSetBuffer == NULL)
		|| (pu4SetInfoLen == NULL))
		return WLAN_STATUS_FAILURE;

	prGlueInfo = prAdapter->prGlueInfo;

	/* init */
	*pu4SetInfoLen = sizeof(unsigned char);
	if (u4SetBufferLen < sizeof(unsigned char))
		return WLAN_STATUS_INVALID_LENGTH;

	ASSERT(pvSetBuffer);
	u4Idx = *(unsigned char *) pvSetBuffer;

	DBGLOG(INIT, INFO, "ucRoleIdx = %d\n", u4Idx);
	if (u4Idx >= KAL_P2P_NUM)
		return WLAN_STATUS_FAILURE;

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->rP2PNetRegState != ENUM_NET_REG_STATE_REGISTERED) {
		DBGLOG(INIT, ERROR, "p2p net is already unregistered?\n");
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		return -EFAULT;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	glUnregisterP2P(prAdapter->prGlueInfo, u4Idx, FALSE);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	gPrP2pDev[u4Idx] = NULL;
	gprP2pRoleWdev[u4Idx] = NULL;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	return 0;

}

int mtk_uninit_ap_role(struct GLUE_INFO *prGlueInfo,
		       struct net_device *ndev)
{
	unsigned char u4Idx;
	uint32_t rStatus;
	uint32_t u4BufLen;

	if (!prGlueInfo) {
		DBGLOG(INIT, WARN, "prGlueInfo is NULL\n");
		return -EINVAL;
	}
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &u4Idx) != 0) {
		DBGLOG(INIT, WARN,
		       "can't find the matched dev to uninit AP\n");
		return -EFAULT;
	}

	rStatus = kalIoctl(prGlueInfo, mtk_oid_uninit_ap_role, &u4Idx,
				sizeof(unsigned char), &u4BufLen);
	if (rStatus != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	return 0;
}
#endif /* CFG_ENABLE_WIFI_DIRECT */

#if (CFG_SUPPORT_DFS_MASTER == 1)
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_start_radar_detection(struct wiphy *wiphy,
				  struct net_device *dev,
				  struct cfg80211_chan_def *chandef,
				  unsigned int cac_time_ms)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}

	return mtk_p2p_cfg80211_start_radar_detection(wiphy,
						      dev,
						      chandef,
						      cac_time_ms);
#else
	return 0;
#endif
}
#else
int mtk_cfg_start_radar_detection(struct wiphy *wiphy,
				  struct net_device *dev,
				  struct cfg80211_chan_def *chandef)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}

	return mtk_p2p_cfg80211_start_radar_detection(wiphy, dev, chandef);
#else
	return 0;
#endif
}

#endif


#if KERNEL_VERSION(3, 13, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_channel_switch(struct wiphy *wiphy,
			   struct net_device *dev,
			   struct cfg80211_csa_settings *params)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}

	return mtk_p2p_cfg80211_channel_switch(wiphy, dev, params);
#else
	return 0;
#endif
}
#endif
#endif

static void mtk_vif_destructor(struct net_device *dev)
{
	struct wireless_dev *prWdev = NULL;
	uint32_t u4Idx = 0;
	if (dev) {
		DBGLOG(AIS, INFO, "netdev=%p, wdev=%p\n",
			dev, dev->ieee80211_ptr);
		prWdev = dev->ieee80211_ptr;
		if (prWdev)
			prWdev->netdev = NULL;
		free_netdev(dev);
		if (prWdev) {
			for (u4Idx = 0; u4Idx < KAL_AIS_NUM; u4Idx++) {
				if (prWdev == gprWdev[u4Idx]) {
					gprWdev[u4Idx] = NULL;
					kfree(prWdev);
					break;
				}
			}
		}
		DBGLOG(AIS, INFO, "done\n");
	}
}

#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
struct wireless_dev *mtk_cfg80211_add_iface(struct wiphy *wiphy,
		const char *name, unsigned char name_assign_type,
		enum nl80211_iftype type, u32 *flags, struct vif_params *params)
#else
struct wireless_dev *mtk_cfg80211_add_iface(struct wiphy *wiphy,
		const char *name,
		enum nl80211_iftype type, u32 *flags, struct vif_params *params)
#endif
{
	struct ADAPTER *prAdapter;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct net_device *prDevHandler = NULL;
	struct wireless_dev *prWdev = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	uint8_t ucBssIdx = 0;
	uint32_t rStatus;
	uint8_t ucAisIndex;
	uint32_t u4SetInfoLen;
	struct sockaddr MacAddr;
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
	u8 addr[ETH_ALEN];
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (prGlueInfo == NULL)
		return ERR_PTR(-EINVAL);

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;

	for (ucAisIndex = 0; ucAisIndex < KAL_AIS_NUM; ucAisIndex++) {
		if (gprWdev[ucAisIndex] == NULL)
			break;
	}

	DBGLOG(AIS, TRACE, "%s: u4Idx=%d\n", __func__, ucAisIndex);

	if (ucAisIndex >= KAL_AIS_NUM) {
		DBGLOG(INIT, ERROR, "wdev num reaches limit\n");
		goto fail;
	}

	/* prepare wireless dev */
	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(INIT, ERROR,
			"Allocating memory to wireless_dev context failed\n");
		goto fail;
	}

	memset(prWdev, 0, sizeof(struct wireless_dev));
	prWdev->iftype = NL80211_IFTYPE_STATION;
	prWdev->wiphy = wiphy;

	/* prepare netdev */
	prDevHandler = alloc_netdev_mq(
		sizeof(struct NETDEV_PRIVATE_GLUE_INFO), name,
#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
		NET_NAME_PREDICTABLE,
#endif
		ether_setup, CFG_MAX_TXQ_NUM);


	if (!prDevHandler) {
		DBGLOG(INIT, ERROR,
			"Allocating memory to net_device context failed\n");
		goto fail;
	}

#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
	prDevHandler->priv_destructor = mtk_vif_destructor;
#else
	prDevHandler->destructor = mtk_vif_destructor;
#endif

	/* Device can help us to save at most 3000 packets,
	 * after we stopped queue
	 */
	prDevHandler->tx_queue_len = 3000;
	DBGLOG(INIT, INFO, "net_device prDev(0x%p) allocated\n", prDevHandler);

	prDevHandler->needed_headroom =
		NIC_TX_DESC_AND_PADDING_LENGTH +
		prChipInfo->txd_append_size;
	prDevHandler->netdev_ops = &wlan_netdev_ops;
#ifdef CONFIG_WIRELESS_EXT
	prDevHandler->wireless_handlers = &wext_handler_def;
#endif
	netif_carrier_off(prDevHandler);
	netif_tx_stop_all_queues(prDevHandler);
	kalResetStats(prDevHandler);

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (prAdapter->fgIsSupportCsumOffload)
		prDevHandler->features |=
			NETIF_F_IP_CSUM |
			NETIF_F_IPV6_CSUM |
			NETIF_F_RXCSUM;
#endif

	if (prAdapter->rWifiVar.u4MTU > 0 &&
	    prAdapter->rWifiVar.u4MTU <= ETH_DATA_LEN) {
		prDevHandler->mtu = prAdapter->rWifiVar.u4MTU;
	}

	/* 4 <3.1.3> co-relate net device & prDev */
	prDevHandler->ieee80211_ptr = prWdev;
	prWdev->netdev = prDevHandler;
	SET_NETDEV_DEV(prDevHandler, wiphy_dev(prWdev->wiphy));

	/* prepare private netdev */
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDevHandler);
	prNetDevPrivate->prGlueInfo = prGlueInfo;

	u4SetInfoLen = ucAisIndex;
	rStatus = kalIoctl(prGlueInfo, wlanoidQueryCurrentAddr,
			&MacAddr.sa_data, PARAM_MAC_ADDR_LEN,
			&u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, WARN, "set MAC%d addr fail 0x%x\n",
			ucAisIndex, rStatus);
	} else {
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		ether_addr_copy(addr, MacAddr.sa_data);
		eth_hw_addr_set(prDevHandler, addr);
#else
		kalMemCopy(prDevHandler->dev_addr,
			&MacAddr.sa_data, ETH_ALEN);
#endif
		kalMemCopy(prDevHandler->perm_addr,
			prDevHandler->dev_addr, ETH_ALEN);
#if CFG_SHOW_MACADDR_SOURCE
		DBGLOG(INIT, INFO, "MAC%d address: " MACSTR, ucAisIndex,
		MAC2STR(&MacAddr.sa_data));
#endif
	}

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
	if (cfg80211_register_netdevice(prDevHandler) < 0) {
#else
	if (register_netdevice(prDevHandler) < 0) {
#endif
		DBGLOG(INIT, ERROR, "Register netdev %d failed\n", ucAisIndex);
		goto fail;
	}

	/* netdev and wdev are ready */
	gprWdev[ucAisIndex] = prWdev;

	/* prepare aisfsm/bssinfo */
	kalIoctl(prGlueInfo, wlanoidInitAisFsm, &ucAisIndex, 1, &u4SetInfoLen);

	/* BssIdx should not be 0 if add successfully */
	ucBssIdx = wlanGetBssIdxByNetInterface(prGlueInfo,
					       gprWdev[ucAisIndex]->netdev);
	if (ucBssIdx != AIS_DEFAULT_INDEX && ucBssIdx != MAX_BSSID_NUM)
		return prWdev;

	/* Do uninit flow since wlanoidInitAisFsm failed */
	gprWdev[ucAisIndex] = NULL;
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
	cfg80211_unregister_netdevice(prDevHandler);
#else
	unregister_netdevice(prDevHandler);
#endif
	/* Don't free netdev and wdev manually here!
	 * Netdev and wdev will be free after kernel invoke
	 * netdev priv_destructor/desctructor.
	 * (after unregister netdev and unlock rtnl lock)
	 */
	prDevHandler = NULL;
	prWdev = NULL;
fail:
	if (prDevHandler != NULL)
		free_netdev(prDevHandler);

	if (prWdev != NULL)
		kfree(prWdev);

	return ERR_PTR(-EINVAL);
}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
struct wireless_dev *mtk_cfg_add_iface(struct wiphy *wiphy,
				       const char *name,
				       unsigned char name_assign_type,
				       enum nl80211_iftype type,
				       struct vif_params *params)
#elif KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
struct wireless_dev *mtk_cfg_add_iface(struct wiphy *wiphy,
				       const char *name,
				       unsigned char name_assign_type,
				       enum nl80211_iftype type,
				       u32 *flags,
				       struct vif_params *params)
#else
struct wireless_dev *mtk_cfg_add_iface(struct wiphy *wiphy,
				       const char *name,
				       enum nl80211_iftype type,
				       u32 *flags,
				       struct vif_params *params)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	u32 *flags = NULL;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return ERR_PTR(-EFAULT);
	}

	if (type == NL80211_IFTYPE_STATION)
#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
		return mtk_cfg80211_add_iface(wiphy, name,
						  name_assign_type, type,
						  flags, params);
#else	/* KERNEL_VERSION > (4, 1, 0) */
		return mtk_cfg80211_add_iface(wiphy, name, type, flags,
						  params);
#endif	/* KERNEL_VERSION */

	/* TODO: error handele for the non-P2P interface */

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 == 0)
	DBGLOG(REQ, WARN, "P2P is not supported\n");
	return ERR_PTR(-EINVAL);
#else	/* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
	return mtk_p2p_cfg80211_add_iface(wiphy, name,
					  name_assign_type, type,
					  flags, params);
#else	/* KERNEL_VERSION > (4, 1, 0) */
	return mtk_p2p_cfg80211_add_iface(wiphy, name, type, flags,
					  params);
#endif	/* KERNEL_VERSION */
#endif  /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
}

int mtk_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
	struct net_device *prDevHandler = NULL;
	struct wireless_dev *prWdev = NULL;
	uint32_t u4DisconnectReason = DISCONNECT_REASON_CODE_DEL_IFACE;
	uint32_t rStatus;
	uint8_t ucBssIndex = 0, ucIdx;
	uint8_t ucAisIndex = 0;
	uint32_t u4SetInfoLen;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	ucBssIndex = wlanGetBssIdx(wdev->netdev);
	prAdapter = prGlueInfo->prAdapter;

	if (!IS_BSS_INDEX_VALID(ucBssIndex) ||
	    !IS_BSS_INDEX_AIS(prAdapter, ucBssIndex))
		return -EINVAL;

	ucAisIndex = AIS_INDEX(prAdapter, ucBssIndex);
	if (ucAisIndex == AIS_DEFAULT_INDEX ||
		!wlanGetAisNetDev(prGlueInfo, ucAisIndex)) {
		DBGLOG(REQ, INFO, "bss = %d, ais=%d no netdev\n",
			ucBssIndex, ucAisIndex);
		return -EINVAL;
	}

	if (KAL_AIS_NUM > 1) {
		/* exclude wlan0 (index 0) since its netdev is NOT created by
		 * cfg80211, and wlan0's netdev life cycle must be the same
		 * as driver on/off
		 */
		for (ucIdx = 1; ucIdx < KAL_AIS_NUM; ucIdx++) {
			if (gprWdev[ucIdx] == wdev)
				break;
		}
		if (ucIdx >= KAL_AIS_NUM) {
			DBGLOG(REQ, WARN,
				"can NOT find matching wireless dev.\n");
			return -EINVAL;
		}
	}

	prWdev = gprWdev[ucAisIndex];
	prDevHandler = prWdev->netdev;

	/* make sure netdev is disconnected */
	DBGLOG(REQ, INFO, "ucBssIndex = %d\n", ucBssIndex);
	if (!kalIsResetting()) {
		/* Clear pending request (AIS). */
		aisFsmFlushRequest(prAdapter, ucBssIndex);

		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetDisassociate,
				&u4DisconnectReason, sizeof(u4DisconnectReason),
				&u4SetInfoLen, ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, WARN, "disassociate error:%x\n", rStatus);

		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidUninitAisFsm,
				&ucAisIndex, 1, &u4SetInfoLen, ucBssIndex);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(REQ, WARN, "uninit ais error:%x\n", rStatus);
	} else {
		/* Invoke directly since ioctl will be invalid during reset */
		if (kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex) ==
		    MEDIA_STATE_CONNECTED)
			kalIndicateStatusAndComplete(prAdapter->prGlueInfo,
				     WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY, NULL,
				     0, ucBssIndex);

		aisFsmUninit(prAdapter, AIS_INDEX(prAdapter, ucBssIndex));
	}

	/* prepare for removal */
	if (netif_carrier_ok(prDevHandler))
		netif_carrier_off(prDevHandler);

	netif_tx_stop_all_queues(prDevHandler);

#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
	cfg80211_unregister_netdevice(prDevHandler);
#else
	unregister_netdevice(prDevHandler);
#endif

	DBGLOG(REQ, INFO, "ucBssIndex = %d done\n", ucBssIndex);
	/* netdev and wdev will be freed at mtk_vif_destructor */
	return 0;
}

int mtk_cfg_del_iface(struct wiphy *wiphy,
		      struct wireless_dev *wdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		return mtk_p2p_cfg80211_del_iface(wiphy, wdev);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_del_iface(wiphy, wdev);
}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_change_iface(struct wiphy *wiphy,
			 struct net_device *ndev,
			 enum nl80211_iftype type,
			 struct vif_params *params)
#else
int mtk_cfg_change_iface(struct wiphy *wiphy,
			 struct net_device *ndev,
			 enum nl80211_iftype type, u32 *flags,
			 struct vif_params *params)
#endif
{
#if CFG_ENABLE_WIFI_DIRECT
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetdevPriv = NULL;
	struct P2P_INFO *prP2pInfo = NULL;
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	u32 *flags = NULL;
#endif
	GLUE_SPIN_LOCK_DECLARATION();

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (!ndev) {
		DBGLOG(REQ, WARN, "ndev is NULL\n");
		return -EINVAL;
	}

	prNetdevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
		       netdev_priv(ndev);

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211)
	/* for p2p0(GO/GC) & ap0(SAP): mtk_p2p_cfg80211_change_iface
	 * for wlan0 (STA/SAP): the following mtk_cfg_change_iface process
	 */
	if (!wlanIsAisDev(ndev)) {
		return mtk_p2p_cfg80211_change_iface(wiphy, ndev, type,
						     flags, params);
	}
#endif /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */

	DBGLOG(P2P, INFO, "ndev=%p, new type=%d\n", ndev, type);

	prAdapter = prGlueInfo->prAdapter;

	if (ndev->ieee80211_ptr->iftype == type) {
		DBGLOG(REQ, INFO, "ndev type is not changed (%d)\n", type);
		return 0;
	}

	netif_carrier_off(ndev);
	/* stop ap will stop all queue, and kalIndicateStatusAndComplete only do
	 * netif_carrier_on. So that, the following STA can't send 4-way M2 to
	 * AP.
	 */
	netif_tx_start_all_queues(ndev);

	/* flush scan */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if ((prGlueInfo->prScanRequest != NULL) &&
	    (prGlueInfo->prScanRequest->wdev == ndev->ieee80211_ptr)) {
		kalCfg80211ScanDone(prGlueInfo->prScanRequest, TRUE);
		prGlueInfo->prScanRequest = NULL;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	/* only AP/STA/Monitor will be handled here (excluding IBSS) */

	DBGLOG(INIT, INFO, "[before][type:iftype]:[%d:%d] => [?:%d]\n",
		ndev->type,
		ndev->ieee80211_ptr->iftype,
		type);

	if (type == NL80211_IFTYPE_AP) {
		/* STA mode change to AP mode */
		prP2pInfo = prAdapter->prP2pInfo;

		if (prP2pInfo == NULL) {
			DBGLOG(INIT, ERROR, "prP2pInfo is NULL\n");
			return -EFAULT;
		}

		if (prP2pInfo->u4DeviceNum >= KAL_P2P_NUM) {
			DBGLOG(INIT, ERROR, "resource invalid, u4DeviceNum=%d\n"
			       , prP2pInfo->u4DeviceNum);
			return -EFAULT;
		}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		if (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_MONITOR)
			mtk_uninit_monitor_role(wiphy, ndev);
		else
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */
		if (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION)
			mtk_uninit_sta_role(prAdapter, ndev);

		if (mtk_init_ap_role(prGlueInfo, ndev) != 0) {
			DBGLOG(INIT, ERROR, "mtk_init_ap_role FAILED\n");

			/* Only AP/P2P resource has the failure case.	*/
			/* So, just re-init AIS.			*/
			mtk_init_sta_role(prAdapter, ndev);
			return -EFAULT;
		}
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
	} else if (type == NL80211_IFTYPE_MONITOR) {
		if (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_STATION)
			mtk_uninit_sta_role(prAdapter, ndev);
		else if (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP)
			mtk_uninit_ap_role(prGlueInfo, ndev);

		mtk_init_monitor_role(wiphy, ndev);
#endif
	} else {
		if (ndev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP) {
			/* AP mode change to STA mode */
			if (mtk_uninit_ap_role(prGlueInfo, ndev) != 0) {
				DBGLOG(INIT, ERROR,
					"mtk_uninit_ap_role FAILED\n");
				return -EFAULT;
			}
		}
#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
		else if (ndev->ieee80211_ptr->iftype ==
				NL80211_IFTYPE_MONITOR)
			mtk_uninit_monitor_role(wiphy, ndev);
#endif /* CFG_SUPPORT_SNIFFER_RADIOTAP */

		mtk_init_sta_role(prAdapter, ndev);

		/* continue the mtk_cfg80211_change_iface() process */
		mtk_cfg80211_change_iface(wiphy, ndev, type, flags, params);
	}

	DBGLOG(INIT, INFO, "[after][type:iftype]:[%d:%d]\n",
		ndev->type,
		ndev->ieee80211_ptr->iftype);

#endif /* CFG_ENABLE_WIFI_DIRECT */
	return 0;
}

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
int mtk_cfg_add_intf_link(struct wiphy *wiphy,
	struct wireless_dev *wdev, unsigned int link_id)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}

	return mtk_p2p_cfg80211_add_intf_link(wiphy, wdev, link_id);
#else
	return 0;
#endif
}

void mtk_cfg_del_intf_link(struct wiphy *wiphy,
	struct wireless_dev *wdev, unsigned int link_id)
{
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return;
	}

	mtk_p2p_cfg80211_del_intf_link(wiphy, wdev, link_id);
#endif
}
#endif

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_add_key(struct wiphy *wiphy,
		    struct net_device *ndev, int link_id, u8 key_index,
		    bool pairwise, const u8 *mac_addr,
		    struct key_params *params)
#else
int mtk_cfg_add_key(struct wiphy *wiphy,
		    struct net_device *ndev, u8 key_index,
		    bool pairwise, const u8 *mac_addr,
		    struct key_params *params)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_add_key(wiphy, ndev, i4LinkId,
			key_index, pairwise, mac_addr, params);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_add_key(wiphy, ndev, i4LinkId, key_index,
				    pairwise,
				    mac_addr, params);
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_get_key(struct wiphy *wiphy,
		    struct net_device *ndev, int link_id, u8 key_index,
		    bool pairwise, const u8 *mac_addr, void *cookie,
		    void (*callback)(void *cookie, struct key_params *))
#else
int mtk_cfg_get_key(struct wiphy *wiphy,
		    struct net_device *ndev, u8 key_index,
		    bool pairwise, const u8 *mac_addr, void *cookie,
		    void (*callback)(void *cookie, struct key_params *))
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_get_key(wiphy, ndev, i4LinkId,
			key_index, pairwise, mac_addr, cookie, callback);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_get_key(wiphy, ndev, i4LinkId, key_index,
				    pairwise, mac_addr, cookie, callback);
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_del_key(struct wiphy *wiphy,
		    struct net_device *ndev, int link_id, u8 key_index,
		    bool pairwise, const u8 *mac_addr)
#else
int mtk_cfg_del_key(struct wiphy *wiphy,
		    struct net_device *ndev, u8 key_index,
		    bool pairwise, const u8 *mac_addr)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_del_key(wiphy, ndev, i4LinkId,
			key_index, pairwise, mac_addr);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_del_key(wiphy, ndev, i4LinkId, key_index,
				    pairwise, mac_addr);
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_set_default_key(struct wiphy *wiphy,
			    struct net_device *ndev, int link_id,
			    u8 key_index, bool unicast, bool multicast)
#else
int mtk_cfg_set_default_key(struct wiphy *wiphy,
			    struct net_device *ndev,
			    u8 key_index, bool unicast, bool multicast)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_set_default_key(wiphy, ndev, i4LinkId,
						key_index, unicast, multicast);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_set_default_key(wiphy, ndev, i4LinkId,
					    key_index, unicast, multicast);
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_set_default_mgmt_key(struct wiphy *wiphy,
		struct net_device *ndev, int link_id, u8 key_index)
#else
int mtk_cfg_set_default_mgmt_key(struct wiphy *wiphy,
		struct net_device *ndev, u8 key_index)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_set_mgmt_key(wiphy, ndev,
			i4LinkId, key_index);
#endif
	/* STA Mode */
	DBGLOG(REQ, WARN, "STA don't support this function\n");
	return -EFAULT;
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_set_default_beacon_key(struct wiphy *wiphy,
		struct net_device *ndev, int link_id, u8 key_index)
#else
int mtk_cfg_set_default_beacon_key(struct wiphy *wiphy,
		struct net_device *ndev, u8 key_index)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int i4LinkId = MLD_LINK_ID_NONE;

#if (KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
	i4LinkId = link_id;
#endif

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_set_beacon_key(wiphy, ndev,
			i4LinkId, key_index);
	/* STA Mode */
	DBGLOG(REQ, WARN, "STA don't support this function\n");
	return -EFAULT;
}

#if (CFG_ADVANCED_80211_MLO == 1) || \
	(KERNEL_VERSION(6, 1, 0) <= CFG80211_VERSION_CODE)
int mtk_cfg_get_channel(struct wiphy *wiphy,
			struct wireless_dev *wdev,
			unsigned int link_id,
			struct cfg80211_chan_def *chandef)
{
	return -EINVAL;
}
#endif /* (CFG_ADVANCED_80211_MLO == 1) || K(6.1) <= CFG80211_VERSION_CODE) */

#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_get_station(struct wiphy *wiphy,
			struct net_device *ndev,
			const u8 *mac, struct station_info *sinfo)
#else
int mtk_cfg_get_station(struct wiphy *wiphy,
			struct net_device *ndev,
			u8 *mac, struct station_info *sinfo)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_get_station(wiphy, ndev, mac,
						    sinfo);
#endif
	/* STA Mode */
	return mtk_cfg80211_get_station(wiphy, ndev, mac, sinfo);
}

#if CFG_SUPPORT_TDLS
#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_change_station(struct wiphy *wiphy,
			   struct net_device *ndev,
			   const u8 *mac, struct station_parameters *params)
#else
int mtk_cfg_change_station(struct wiphy *wiphy,
			   struct net_device *ndev,
			   u8 *mac, struct station_parameters *params)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_change_station(
			wiphy, ndev, mac, params);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_change_station(wiphy, ndev, mac,
					   params);
}

#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_add_station(struct wiphy *wiphy,
			struct net_device *ndev,
			const u8 *mac, struct station_parameters *params)
#else
int mtk_cfg_add_station(struct wiphy *wiphy,
			struct net_device *ndev,
			u8 *mac, struct station_parameters *params)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_add_station(wiphy, ndev, mac);
#endif
	/* STA Mode */
	return mtk_cfg80211_add_station(wiphy, ndev, mac, params);
}

#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_tdls_oper(struct wiphy *wiphy,
		      struct net_device *ndev,
		      const u8 *peer, enum nl80211_tdls_operation oper)
#else
int mtk_cfg_tdls_oper(struct wiphy *wiphy,
		      struct net_device *ndev,
		      u8 *peer, enum nl80211_tdls_operation oper)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_tdls_oper(wiphy, ndev, peer, oper);
}

#if KERNEL_VERSION(6, 3, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_tdls_mgmt(struct wiphy *wiphy,
		      struct net_device *dev, const u8 *peer,
		      int link_id, u8 action_code, u8 dialog_token,
		      u16 status_code, u32 peer_capability,
		      bool initiator, const u8 *buf, size_t len)
#elif KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_tdls_mgmt(struct wiphy *wiphy,
		      struct net_device *dev,
		      const u8 *peer, u8 action_code, u8 dialog_token,
		      u16 status_code, u32 peer_capability,
		      bool initiator, const u8 *buf, size_t len)
#elif KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_tdls_mgmt(struct wiphy *wiphy,
		      struct net_device *dev,
		      const u8 *peer, u8 action_code, u8 dialog_token,
		      u16 status_code, u32 peer_capability,
		      const u8 *buf, size_t len)
#else
int mtk_cfg_tdls_mgmt(struct wiphy *wiphy,
		      struct net_device *dev,
		      u8 *peer, u8 action_code, u8 dialog_token,
		      u16 status_code,
		      const u8 *buf, size_t len)
#endif
{
	struct GLUE_INFO *prGlueInfo;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (!TdlsEnabled(prGlueInfo->prAdapter))
		return -EINVAL;

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, dev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif

#if KERNEL_VERSION(6, 4, 0) <= CFG80211_VERSION_CODE
	return mtk_cfg80211_tdls_mgmt(wiphy, dev, peer, link_id, action_code,
			dialog_token, status_code, peer_capability, initiator,
			buf, len);
#elif KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
	return mtk_cfg80211_tdls_mgmt(wiphy, dev, peer, action_code,
			dialog_token, status_code, peer_capability, initiator,
			buf, len);
#elif KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
	return mtk_cfg80211_tdls_mgmt(wiphy, dev, peer, action_code,
			dialog_token, status_code, peer_capability,
			buf, len);
#else
	return mtk_cfg80211_tdls_mgmt(wiphy, dev, peer, action_code,
				      dialog_token, status_code,
				      buf, len);
#endif
}

#if CFG_SUPPORT_TDLS_OFFCHANNEL
int mtk_tdls_channel_switch(struct wiphy *wiphy,
		      struct net_device *dev,
		      const u8 *addr, u8 oper_class,
		      struct cfg80211_chan_def *chandef)
{
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rConfig;
	uint32_t u4BufLen;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;
	uint8_t cmd[128] = {0};
	uint8_t strLen = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo,
		WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, dev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	strLen = kalSnprintf(cmd, sizeof(cmd),
		"tdls %d " MACSTR " %d %d %d %d",
		1, /* fgIsChSwEnabled */
		MAC2STR(addr), /* aucPeerMac */
		ieee80211_frequency_to_channel(chandef->chan->center_freq),
		/* ucTargetChan */
		oper_class, /* ucRegClass */
		(cfg80211_get_chandef_type(chandef) == NL80211_CHAN_HT40PLUS)
		? IEEE80211_HT_PARAM_CHA_SEC_ABOVE
		: IEEE80211_HT_PARAM_CHA_SEC_BELOW, /* ucSecChanOff */
		ucBssIndex);

	DBGLOG(TDLS, INFO,
		"[%d] Notify FW %s, strlen=%d",
		ucBssIndex, cmd, strLen);

	kalMemZero(&rConfig, sizeof(rConfig));
	rConfig.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rConfig.u2MsgSize = strLen;
	kalStrnCpy(rConfig.aucCmd, cmd, strLen);
	rStatus = kalIoctl(prGlueInfo,
		wlanoidSetChipConfig,
		&rConfig, sizeof(rConfig),
		&u4BufLen);

	DBGLOG(REQ, LOUD, "[%d] rStatus: %x",
		ucBssIndex,
		rStatus);

	if (rStatus == WLAN_STATUS_SUCCESS)
		return 0;
	else
		return -EINVAL;
}

void mtk_tdls_cancel_channel_switch(struct wiphy *wiphy,
		      struct net_device *dev,
		       const u8 *addr)
{
	struct GLUE_INFO *prGlueInfo;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rConfig;
	uint32_t u4BufLen;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint8_t ucBssIndex = 0;
	uint8_t cmd[128] = {0};
	uint8_t strLen = 0;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo,
		WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PGoNetDevice(prGlueInfo, dev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return;
	}
#endif

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return;

	strLen = kalSnprintf(cmd, sizeof(cmd),
		"tdls %d " MACSTR " %d %d %d %d",
		0, /* fgIsChSwEnabled */
		MAC2STR(addr), /* aucPeerMac */
		0, /* ucTargetChan */
		0, /* ucRegClass */
		0, /* ucSecChanOff */
		ucBssIndex);

	DBGLOG(TDLS, INFO,
		"[%d] Notify FW %s, strlen=%d",
		ucBssIndex, cmd, strLen);

	kalMemZero(&rConfig, sizeof(rConfig));
	rConfig.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rConfig.u2MsgSize = strLen;
	kalStrnCpy(rConfig.aucCmd, cmd, strLen);
	rStatus = kalIoctl(prGlueInfo,
		wlanoidSetChipConfig,
		&rConfig, sizeof(rConfig),
		&u4BufLen);

	DBGLOG(REQ, LOUD, "[%d] rStatus: %x",
		ucBssIndex,
		rStatus);
}
#endif
#endif /* CFG_SUPPORT_TDLS */

#if KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_del_station(struct wiphy *wiphy,
			struct net_device *ndev,
			struct station_del_parameters *params)
#elif KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_del_station(struct wiphy *wiphy,
			struct net_device *ndev,
			const u8 *mac)
#else
int mtk_cfg_del_station(struct wiphy *wiphy,
			struct net_device *ndev, u8 *mac)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
#if KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
		return mtk_p2p_cfg80211_del_station(wiphy, ndev, params);
#else
		return mtk_p2p_cfg80211_del_station(wiphy, ndev, mac);
#endif
	}
#endif
	/* STA Mode */
#if CFG_SUPPORT_TDLS
#if KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
	return mtk_cfg80211_del_station(wiphy, ndev, params);
#else	/* CFG80211_VERSION_CODE > KERNEL_VERSION(3, 19, 0) */
	return mtk_cfg80211_del_station(wiphy, ndev, mac);
#endif	/* CFG80211_VERSION_CODE */
#else	/* CFG_SUPPORT_TDLS == 0 */
	/* AIS only support this function when CFG_SUPPORT_TDLS */
	return -EFAULT;
#endif	/* CFG_SUPPORT_TDLS */
}

int mtk_cfg_scan(struct wiphy *wiphy,
		 struct cfg80211_scan_request *request)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo,
			       request->wdev->netdev) > 0)
		return mtk_p2p_cfg80211_scan(wiphy, request);
#endif
	/* STA Mode */
	return mtk_cfg80211_scan(wiphy, request);
}

#if KERNEL_VERSION(4, 5, 0) <= CFG80211_VERSION_CODE
void mtk_cfg_abort_scan(struct wiphy *wiphy,
			struct wireless_dev *wdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0)
		mtk_p2p_cfg80211_abort_scan(wiphy, wdev);
	else	/* STA Mode */
#endif
		mtk_cfg80211_abort_scan(wiphy, wdev);
}
#endif

#if CFG_SUPPORT_SCHED_SCAN
int mtk_cfg_sched_scan_start(struct wiphy *wiphy,
			     struct net_device *ndev,
			     struct cfg80211_sched_scan_request *request)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif

	return mtk_cfg80211_sched_scan_start(wiphy, ndev, request);

}

#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_sched_scan_stop(struct wiphy *wiphy,
			    struct net_device *ndev,
			    u64 reqid)
#else
int mtk_cfg_sched_scan_stop(struct wiphy *wiphy,
			    struct net_device *ndev)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}
#if KERNEL_VERSION(4, 12, 0) <= CFG80211_VERSION_CODE
	return mtk_cfg80211_sched_scan_stop(wiphy, ndev, reqid);
#else
	return mtk_cfg80211_sched_scan_stop(wiphy, ndev);
#endif
}
#endif /* CFG_SUPPORT_SCHED_SCAN */

int mtk_cfg_connect(struct wiphy *wiphy,
		    struct net_device *ndev,
		    struct cfg80211_connect_params *sme)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_connect(wiphy, ndev, sme);
#endif
	/* STA Mode */
	return mtk_cfg80211_connect(wiphy, ndev, sme);
}

#if (CFG_SUPPORT_ROAMING == 1)
int mtk_cfg_update_connect_params(struct wiphy *wiphy,
		  struct net_device *ndev,
		  struct cfg80211_connect_params *sme,
		  u32 changed)
{
	uint8_t ucBssIndex = 0;
	uint32_t u4BufLen;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ucBssIndex = wlanGetBssIdx(ndev);

	DBGLOG(REQ, INFO, "[bss%d] update connect %p %zu %d\n",
		ucBssIndex, sme->ie, sme->ie_len, changed);

	if (changed & UPDATE_ASSOC_IES && sme->ie && sme->ie_len) {
		struct PARAM_CONNECT rNewSsid;

		kalMemZero(&rNewSsid, sizeof(rNewSsid));
		rNewSsid.pucIEs = (uint8_t *)sme->ie;
		rNewSsid.u4IesLen = sme->ie_len;
		rNewSsid.ucBssIdx = ucBssIndex;
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidUpdateConnect,
			   (void *)&rNewSsid, sizeof(struct PARAM_CONNECT),
			   &u4BufLen, ucBssIndex);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "update SSID:%x\n", rStatus);
			return -EINVAL;
		}
	}

#if KERNEL_VERSION(4, 18, 0) <= CFG80211_VERSION_CODE
	if (changed & UPDATE_AUTH_TYPE) {
		struct GL_WPA_INFO *prWpaInfo;

		prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter, ucBssIndex);

		switch (sme->auth_type) {
#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
		case NL80211_AUTHTYPE_FILS_SK:
			DBGLOG(REQ, INFO, "FILS: auth alg 0x%x -> 0x%x",
				prWpaInfo->u4AuthAlg, IW_AUTH_ALG_FILS_SK);
			prWpaInfo->u4AuthAlg = IW_AUTH_ALG_FILS_SK;
			break;
		case NL80211_AUTHTYPE_FILS_SK_PFS:
		case NL80211_AUTHTYPE_FILS_PK:
			DBGLOG(INIT, INFO,
				"Only support fils share key authentication without PFS (auth_type=%d)\n",
				sme->auth_type);
			return -EFAULT;
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
		default:
			DBGLOG(INIT, INFO, "auth_type changed to %d??\n",
				sme->auth_type);
			break;
		}
	}

#if (CFG_SUPPORT_FILS_SK_OFFLOAD == 1)
	if (changed & UPDATE_FILS_ERP_INFO) {
		struct PARAM_FILS rFils;

		rFils.pucErpUsername = sme->fils_erp_username;
		rFils.u2ErpUsernameLen = sme->fils_erp_username_len;
		rFils.pucErpRealm = sme->fils_erp_realm;
		rFils.pucErpRealmLen = sme->fils_erp_realm_len;
		rFils.u4ErpNextSeqNum = sme->fils_erp_next_seq_num;
		rFils.pucErpRrk = sme->fils_erp_rrk;
		rFils.u2ErpRrkLen = sme->fils_erp_rrk_len;
		rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidSetFilsConnInfo,
				&rFils, sizeof(rFils), &u4BufLen, ucBssIndex);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, INFO,
				"FILS conn info error:%x\n", rStatus);
			return -EFAULT;
		}
	}
#endif /* CFG_SUPPORT_FILS_SK_OFFLOAD */
#endif

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_ROAMING */

int mtk_cfg_disconnect(struct wiphy *wiphy,
		       struct net_device *ndev,
		       u16 reason_code)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_disconnect(wiphy, ndev,
						   reason_code);
#endif
	/* STA Mode */
	return mtk_cfg80211_disconnect(wiphy, ndev, reason_code);
}

int mtk_cfg_join_ibss(struct wiphy *wiphy,
		      struct net_device *ndev,
		      struct cfg80211_ibss_params *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_join_ibss(wiphy, ndev, params);
#endif
	/* STA Mode */
	return mtk_cfg80211_join_ibss(wiphy, ndev, params);
}

int mtk_cfg_leave_ibss(struct wiphy *wiphy,
		       struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0)
		return mtk_p2p_cfg80211_leave_ibss(wiphy, ndev);
#endif
	/* STA Mode */
	return mtk_cfg80211_leave_ibss(wiphy, ndev);
}

int mtk_cfg_set_power_mgmt(struct wiphy *wiphy,
			   struct net_device *ndev,
			   bool enabled, int timeout)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		return mtk_p2p_cfg80211_set_power_mgmt(wiphy, ndev,
						       enabled, timeout);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_set_power_mgmt(wiphy, ndev, enabled,
					   timeout);
}

int mtk_cfg_set_pmksa(struct wiphy *wiphy,
		      struct net_device *ndev,
		      struct cfg80211_pmksa *pmksa)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_cfg80211_set_pmksa(wiphy, ndev, pmksa);
}

int mtk_cfg_del_pmksa(struct wiphy *wiphy,
		      struct net_device *ndev,
		      struct cfg80211_pmksa *pmksa)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_cfg80211_del_pmksa(wiphy, ndev, pmksa);
}

int mtk_cfg_flush_pmksa(struct wiphy *wiphy,
			struct net_device *ndev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, TRACE, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_cfg80211_flush_pmksa(wiphy, ndev);
}

#if CONFIG_SUPPORT_GTK_REKEY
int mtk_cfg_set_rekey_data(struct wiphy *wiphy,
			   struct net_device *dev,
			   struct cfg80211_gtk_rekey_data *data)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_cfg80211_set_rekey_data(wiphy, dev, data);
}
#endif /* CONFIG_SUPPORT_GTK_REKEY */

int mtk_cfg_suspend(struct wiphy *wiphy,
		    struct cfg80211_wowlan *wow)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "GLUE_INFO is NULL\n");
		return -EFAULT;
	}

	if (!wlanIsDriverReady(prGlueInfo,
		(WLAN_DRV_READY_CHECK_RESET | WLAN_DRV_READY_CHECK_WLAN_ON))) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return 0;
	}

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT

	/* Before cfg80211 suspend, we must make sure L0.5 is either
	 * done or postponed.
	 */
	if (prGlueInfo->prAdapter &&
	    prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prGlueInfo->prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		prGlueInfo->prAdapter->fgIsCfgSuspend = TRUE;
		KAL_RELEASE_SPIN_LOCK_BH(prGlueInfo->prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		cancel_work_sync(&prGlueInfo->rWfsysResetWork);
	}
#endif
	/* TODO: AP/P2P do not support this function, should take that case. */
	return mtk_cfg80211_suspend(wiphy, wow);
}

int mtk_cfg_resume(struct wiphy *wiphy)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!prGlueInfo) {
		DBGLOG(REQ, ERROR, "GLUE_INFO is NULL\n");
		return -EFAULT;
	}

	if (!wlanIsDriverReady(prGlueInfo,
		(WLAN_DRV_READY_CHECK_RESET | WLAN_DRV_READY_CHECK_WLAN_ON))) {
		DBGLOG(REQ, TRACE, "driver is not ready\n");
		return 0;
	}

#if CFG_CHIP_RESET_SUPPORT && !CFG_WMT_RESET_API_SUPPORT

	/* When cfg80211 resume, just reschedule L0.5 reset procedure
	 * if it is pending due to that fact that system suspend happened
	 * previously when L0.5 reset was not yet done.
	 */
	if (prGlueInfo->prAdapter &&
	    prGlueInfo->prAdapter->chip_info->fgIsSupportL0p5Reset) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prGlueInfo->prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		prGlueInfo->prAdapter->fgIsCfgSuspend = FALSE;
		KAL_RELEASE_SPIN_LOCK_BH(prGlueInfo->prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		cancel_work_sync(&prGlueInfo->rWfsysResetWork);

		if (glReSchWfsysReset(prGlueInfo->prAdapter))
			DBGLOG(REQ, WARN, "reschedule L0.5 reset procedure\n");
	}
#endif

	/* TODO: AP/P2P do not support this function, should take that case. */
	return mtk_cfg80211_resume(wiphy);
}

int mtk_cfg_assoc(struct wiphy *wiphy,
		  struct net_device *ndev,
		  struct cfg80211_assoc_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, ndev) > 0) {
		DBGLOG(REQ, WARN, "P2P/AP don't support this function\n");
		return -EFAULT;
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_assoc(wiphy, ndev, req);
}

int mtk_cfg_remain_on_channel(struct wiphy *wiphy,
			      struct wireless_dev *wdev,
			      struct ieee80211_channel *chan,
			      unsigned int duration, u64 *cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		return mtk_p2p_cfg80211_remain_on_channel(wiphy, wdev, chan,
				duration, cookie);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_remain_on_channel(wiphy, wdev, chan,
					      duration, cookie);
}

int mtk_cfg_cancel_remain_on_channel(struct wiphy *wiphy,
				     struct wireless_dev *wdev, u64 cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		return mtk_p2p_cfg80211_cancel_remain_on_channel(wiphy,
				wdev,
				cookie);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_cancel_remain_on_channel(wiphy, wdev,
			cookie);
}

#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_mgmt_tx(struct wiphy *wiphy,
		    struct wireless_dev *wdev,
		    struct cfg80211_mgmt_tx_params *params, u64 *cookie)
#else
int mtk_cfg_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
		struct ieee80211_channel *channel, bool offchan,
		unsigned int wait, const u8 *buf, size_t len, bool no_cck,
		bool dont_wait_for_ack, u64 *cookie)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0)
		return mtk_p2p_cfg80211_mgmt_tx(wiphy, wdev, params,
						cookie);
#endif
	/* STA Mode */
	return mtk_cfg80211_mgmt_tx(wiphy, wdev, params, cookie);
#else /* KERNEL_VERSION(3, 14, 0) > CFG80211_VERSION_CODE */
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		return mtk_p2p_cfg80211_mgmt_tx(wiphy, wdev, channel, offchan,
			wait, buf, len, no_cck, dont_wait_for_ack, cookie);
	}
#endif
	/* STA Mode */
	return mtk_cfg80211_mgmt_tx(wiphy, wdev, channel, offchan, wait, buf,
			len, no_cck, dont_wait_for_ack, cookie);
#endif
}

void mtk_cfg_mgmt_frame_register(struct wiphy *wiphy,
				 struct wireless_dev *wdev,
				 u16 frame_type, bool reg)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		mtk_p2p_cfg80211_mgmt_frame_register(wiphy, wdev,
						     frame_type,
						     reg);
	} else {
		mtk_cfg80211_mgmt_frame_register(wiphy, wdev, frame_type,
						 reg);
	}
#else
		mtk_cfg80211_mgmt_frame_register(wiphy, wdev, frame_type,
						 reg);

#endif

}

#if KERNEL_VERSION(5, 8, 0) <= CFG80211_VERSION_CODE
void mtk_cfg_mgmt_frame_update(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				struct mgmt_frame_regs *upd)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate = NULL;
	u_int8_t fgIsP2pNetDevice = FALSE;
	uint32_t *pu4PacketFilter = NULL;

	if ((wiphy == NULL) || (wdev == NULL) || (upd == NULL)) {
		DBGLOG(INIT, TRACE, "Invalidate params\n");
		return;
	}
	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	fgIsP2pNetDevice = mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev);
#endif
	/* prepare private netdev */
	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(wdev->netdev);

	DBGLOG(INIT, TRACE,
		"netdev(0x%p) update management frame filter: 0x%08x\n",
		wdev->netdev, upd->interface_stypes);
	do {
		if (fgIsP2pNetDevice) {
			uint8_t ucRoleIdx = 0;
			struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
				(struct P2P_ROLE_FSM_INFO *) NULL;

			if (!prGlueInfo->prAdapter->fgIsP2PRegistered ||
				(prGlueInfo->prAdapter->rP2PNetRegState !=
					ENUM_NET_REG_STATE_REGISTERED)) {
				DBGLOG(P2P, WARN,
					"p2p net dev is not registered\n");
				break;
			}

			if (prGlueInfo->prP2PInfo[0]->prDevHandler ==
				wdev->netdev) {
				pu4PacketFilter =
					&prGlueInfo->prP2PDevInfo
					->u4OsMgmtFrameFilter;
				/* Reset filters*/
				*pu4PacketFilter = 0;
			} else {
				if (mtk_Netdev_To_RoleIdx(prGlueInfo,
					wdev->netdev, &ucRoleIdx) < 0) {
					DBGLOG(P2P, WARN,
						"wireless dev match fail!\n");
					break;
				}
				/* Non P2P device*/
				if (ucRoleIdx >= KAL_P2P_NUM) {
					DBGLOG(P2P, WARN,
						"wireless dev match fail2!\n");
					break;
				}
				DBGLOG(P2P, TRACE,
					"Open packet filer RoleIdx %u\n",
					ucRoleIdx);
				prP2pRoleFsmInfo =
					prGlueInfo->prAdapter->rWifiVar
					.aprP2pRoleFsmInfo[ucRoleIdx];
				if (!prP2pRoleFsmInfo)  {
					DBGLOG(P2P, WARN,
						"prP2pRoleFsmInfo fail!\n");
					break;
				}
				pu4PacketFilter = &prP2pRoleFsmInfo
					->u4P2pPacketFilter;
				*pu4PacketFilter =
					PARAM_PACKET_FILTER_SUPPORTED;
			}
		} else {
			pu4PacketFilter = &prNetDevPrivate->u4OsMgmtFrameFilter;
			*pu4PacketFilter = 0;
		}
		if (upd->interface_stypes & MASK_MAC_FRAME_PROBE_REQ)
			*pu4PacketFilter |= PARAM_PACKET_FILTER_PROBE_REQ;

		if (upd->interface_stypes & MASK_MAC_FRAME_ACTION)
			*pu4PacketFilter |= PARAM_PACKET_FILTER_ACTION_FRAME;
#if CFG_SUPPORT_SOFTAP_WPA3
		if (upd->interface_stypes & MASK_MAC_FRAME_AUTH)
			*pu4PacketFilter |= PARAM_PACKET_FILTER_AUTH;

		if (upd->interface_stypes & MASK_MAC_FRAME_ASSOC_REQ)
			*pu4PacketFilter |= PARAM_PACKET_FILTER_ASSOC_REQ;
#endif

		if (fgIsP2pNetDevice) {
			set_bit(GLUE_FLAG_FRAME_FILTER_BIT,
				&prGlueInfo->ulFlag);

			/* wake up main thread */
			wake_up_interruptible(&prGlueInfo->waitq);
		}
	} while (FALSE);
}
#endif

#ifdef CONFIG_NL80211_TESTMODE
#if KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_testmode_cmd(struct wiphy *wiphy,
			 struct wireless_dev *wdev,
			 void *data, int len)
#else
int mtk_cfg_testmode_cmd(struct wiphy *wiphy, void *data,
			 int len)
#endif
{
#if KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wdev) {
		DBGLOG(NAN, ERROR, "wdev error!\n");
		return -EFAULT;
	}

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) > 0) {
		return mtk_p2p_cfg80211_testmode_cmd(wiphy, wdev, data,
						     len);
	}
#endif
	return mtk_cfg80211_testmode_cmd(wiphy, wdev, data, len);
#else
	/* XXX: no information can to check the mtk_IsP2PNetDevice */
	/* return mtk_p2p_cfg80211_testmode_cmd(wiphy, data, len); */
	return mtk_cfg80211_testmode_cmd(wiphy, data, len);
#endif
}
#endif	/* CONFIG_NL80211_TESTMODE */

#if (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0)
int mtk_cfg_change_bss(struct wiphy *wiphy,
		       struct net_device *dev,
		       struct bss_parameters *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_change_bss(wiphy, dev, params);
}

int mtk_cfg_mgmt_tx_cancel_wait(struct wiphy *wiphy,
				struct wireless_dev *wdev,
				u64 cookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
		return mtk_cfg80211_mgmt_tx_cancel_wait(wiphy, wdev, cookie);
	}
#endif
	return mtk_p2p_cfg80211_mgmt_tx_cancel_wait(wiphy, wdev,
			cookie);
}

int mtk_cfg_deauth(struct wiphy *wiphy,
		   struct net_device *dev,
		   struct cfg80211_deauth_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_deauth(wiphy, dev, req);
}

int mtk_cfg_disassoc(struct wiphy *wiphy,
		     struct net_device *dev,
		     struct cfg80211_disassoc_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_disassoc(wiphy, dev, req);
}

int mtk_cfg_start_ap(struct wiphy *wiphy,
		     struct net_device *dev,
		     struct cfg80211_ap_settings *settings)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_start_ap(wiphy, dev, settings);
}

#if KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE
int mtk_cfg_change_beacon(struct wiphy *wiphy,
			  struct net_device *dev,
			  struct cfg80211_ap_update *info)
#else
int mtk_cfg_change_beacon(struct wiphy *wiphy,
			  struct net_device *dev,
			  struct cfg80211_beacon_data *info)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
#if KERNEL_VERSION(6, 7, 0) <= CFG80211_VERSION_CODE
	return mtk_p2p_cfg80211_change_beacon(wiphy, dev, &info->beacon);
#else
	return mtk_p2p_cfg80211_change_beacon(wiphy, dev, info);
#endif
}

#if (KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
int mtk_cfg_stop_ap(struct wiphy *wiphy, struct net_device *dev,
	unsigned int link_id)
#else
int mtk_cfg_stop_ap(struct wiphy *wiphy, struct net_device *dev)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif

#if (KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE) || \
		(CFG_ADVANCED_80211_MLO == 1)
	return mtk_p2p_cfg80211_stop_ap(wiphy, dev, link_id);
#else
	return mtk_p2p_cfg80211_stop_ap(wiphy, dev, 0);
#endif
}

int mtk_cfg_set_wiphy_params(struct wiphy *wiphy,
			     u32 changed)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	/* TODO: AIS not support this function */
	return mtk_p2p_cfg80211_set_wiphy_params(wiphy, changed);
}

#if (KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE) || \
	(CFG_ADVANCED_80211_MLO == 1)
int mtk_cfg_set_bitrate_mask(struct wiphy *wiphy,
			     struct net_device *dev,
			     unsigned int link_id,
			     const u8 *peer,
			     const struct cfg80211_bitrate_mask *mask)
#else
int mtk_cfg_set_bitrate_mask(struct wiphy *wiphy,
			     struct net_device *dev,
			     const u8 *peer,
			     const struct cfg80211_bitrate_mask *mask)
#endif
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, dev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_set_bitrate_mask(wiphy, dev, peer,
			mask);
}

int mtk_cfg_set_txpower(struct wiphy *wiphy,
			struct wireless_dev *wdev,
			enum nl80211_tx_power_setting type, int mbm)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
		DBGLOG(REQ, WARN, "STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_set_txpower(wiphy, wdev, type, mbm);
}

int mtk_cfg_get_txpower(struct wiphy *wiphy,
			struct wireless_dev *wdev,
			int *dbm)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	WIPHY_PRIV(wiphy, prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG_LIMITED(REQ, TRACE, "driver is not ready\n");
		return -EFAULT;
	}
#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
	if (mtk_IsP2PNetDevice(prGlueInfo, wdev->netdev) <= 0) {
		DBGLOG_LIMITED(REQ, TRACE,
			"STA doesn't support this function\n");
		return -EFAULT;
	}
#endif
	return mtk_p2p_cfg80211_get_txpower(wiphy, wdev, dbm);
}
#endif /* (CFG_ENABLE_WIFI_DIRECT_CFG_80211 != 0) */

/*-----------------------------------------------------------------------*/
/*!
 * @brief This function goes through the provided ies buffer, and
 *        collects those non-wfa vendor specific ies into driver's
 *        internal buffer (non_wfa_vendor_ie_buf), to be sent in
 *        AssocReq in AIS mode.
 *        The non-wfa vendor specific ies are those with ie_id = 0xdd
 *        and ouis are different from wfa's oui. (i.e., it could be
 *        customer's vendor ie ...etc.
 *
 * @param prGlueInfo    driver's private glueinfo
 *        ies           ie buffer
 *        len           length of ie
 *
 * @retval length of the non_wfa vendor ie
 */
/*-----------------------------------------------------------------------*/
uint16_t cfg80211_get_non_wfa_vendor_ie(struct GLUE_INFO *prGlueInfo,
	uint8_t *ies, int32_t len, uint8_t ucBssIndex)
{
	const uint8_t *pos = ies, *end = ies+len;
	struct ieee80211_vendor_ie *ie;
	int32_t ie_oui = 0;
	uint16_t *ret_len, max_len;
	uint8_t *w_pos;
	struct CONNECTION_SETTINGS *prConnSettings;

	if (!prGlueInfo || !ies || !len)
		return 0;

	prConnSettings =
		aisGetConnSettings(prGlueInfo->prAdapter,
		ucBssIndex);
	if (!prConnSettings)
		return 0;

	w_pos = prConnSettings->non_wfa_vendor_ie_buf;
	ret_len = &prConnSettings->non_wfa_vendor_ie_len;
	max_len = (uint16_t)sizeof(prConnSettings->non_wfa_vendor_ie_buf);

	while (pos < end) {
		pos = cfg80211_find_ie(WLAN_EID_VENDOR_SPECIFIC, pos,
				       end - pos);
		if (!pos)
			break;

		ie = (struct ieee80211_vendor_ie *)pos;

		/* Make sure we can access ie->len */
		BUILD_BUG_ON(offsetof(struct ieee80211_vendor_ie, len) != 1);

		if (ie->len < sizeof(*ie))
			goto cont;

		ie_oui = ie->oui[0] << 16 | ie->oui[1] << 8 | ie->oui[2];
		/*
		 * If oui is other than: 0x0050f2 & 0x506f9a,
		 * we consider it is non-wfa oui.
		 */
		if (ie_oui != WLAN_OUI_MICROSOFT && ie_oui != WLAN_OUI_WFA) {
			/*
			 * If remaining buf len is capable, we copy
			 * this ie to the buf.
			 */
			if (max_len-(*ret_len) >= ie->len+2) {
				DBGLOG(AIS, TRACE,
					   "vendor ie(len=%d, oui=0x%06x)\n",
					   ie->len, ie_oui);
				memcpy(w_pos, pos, ie->len+2);
				w_pos += (ie->len+2);
				(*ret_len) += ie->len+2;
			} else {
				/* Otherwise we give an error msg
				 * and return.
				 */
				DBGLOG(AIS, ERROR,
					"Insufficient buf for vendor ie, exit!\n");
				break;
			}
		}
cont:
		pos += 2 + ie->len;
	}
	return *ret_len;
}

int mtk_cfg80211_update_ft_ies(struct wiphy *wiphy, struct net_device *dev,
				 struct cfg80211_update_ft_ies_params *ftie)
{
#if CFG_SUPPORT_802_11R
	struct GLUE_INFO *prGlueInfo = NULL;
	uint32_t u4InfoBufLen = 0;
	uint32_t rStatus;
	uint8_t ucBssIndex = 0;

	if (!wiphy)
		return -1;
	WIPHY_PRIV(wiphy, prGlueInfo);

	ucBssIndex = wlanGetBssIdx(dev);
	if (!IS_BSS_INDEX_VALID(ucBssIndex))
		return -EINVAL;

	rStatus = kalIoctlByBssIdx(prGlueInfo, wlanoidUpdateFtIes, (void *)ftie,
			   sizeof(*ftie), &u4InfoBufLen, ucBssIndex);
	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(OID, INFO, "FT: update Ft IE failed\n");
#else
	DBGLOG(OID, INFO, "FT: 802.11R is not enabled\n");
#endif

	return 0;
}

#ifdef CFG_SUPPORT_SNIFFER_RADIOTAP
int mtk_cfg80211_set_monitor_channel(struct wiphy *wiphy,
			struct cfg80211_chan_def *chandef)
{
	struct GLUE_INFO *prGlueInfo;
	uint8_t ucBand = BAND_NULL;
	uint8_t ucSco = 0;
	uint8_t ucChannelWidth = 0;
	uint8_t ucPriChannel = 0;
	uint8_t ucChannelS1 = 0;
	uint8_t ucChannelS2 = 0;
	uint32_t u4BufLen;
	uint32_t rStatus;

	WIPHY_PRIV(wiphy, prGlueInfo);
	ASSERT(prGlueInfo);

	if (!wlanIsDriverReady(prGlueInfo, WLAN_DRV_READY_CHECK_WLAN_ON |
		WLAN_DRV_READY_CHECK_HIF_SUSPEND)) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return -EFAULT;
	}

	if (chandef) {
		ucPriChannel =
		ieee80211_frequency_to_channel(chandef->chan->center_freq);
		ucChannelS1 =
		ieee80211_frequency_to_channel(chandef->center_freq1);
		ucChannelS2 =
		ieee80211_frequency_to_channel(chandef->center_freq2);

		switch (chandef->chan->band) {
		case NL80211_BAND_2GHZ:
			ucBand = BAND_2G4;
			break;
		case NL80211_BAND_5GHZ:
			ucBand = BAND_5G;
			break;
		default:
			return -EFAULT;
		}

		switch (chandef->width) {
		case NL80211_CHAN_WIDTH_80P80:
			ucChannelWidth = CW_80P80MHZ;
			break;
		case NL80211_CHAN_WIDTH_160:
			ucChannelWidth = CW_160MHZ;
			break;
		case NL80211_CHAN_WIDTH_80:
			ucChannelWidth = CW_80MHZ;
			break;
		case NL80211_CHAN_WIDTH_40:
			ucChannelWidth = CW_20_40MHZ;
			if (ucChannelS1 > ucPriChannel)
				ucSco = CHNL_EXT_SCA;
			else
				ucSco = CHNL_EXT_SCB;
			break;
		case NL80211_CHAN_WIDTH_20:
			ucChannelWidth = CW_20_40MHZ;
			break;
		default:
			return -EFAULT;
		}
	}

	prGlueInfo->ucPriChannel = ucPriChannel;
	prGlueInfo->ucChannelS1 = ucChannelS1;
	prGlueInfo->ucChannelS2 = ucChannelS2;
	prGlueInfo->ucBand = ucBand;
	prGlueInfo->ucChannelWidth = ucChannelWidth;
	prGlueInfo->ucSco = ucSco;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetMonitor, NULL, 0, &u4BufLen);

	return 0;
}
#endif
