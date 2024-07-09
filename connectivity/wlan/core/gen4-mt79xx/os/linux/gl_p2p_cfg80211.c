/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */
/*
 ** Id: @(#) gl_p2p_cfg80211.c@@
 */

/*! \file   gl_p2p_cfg80211.c
 *    \brief  Main routines of Linux driver interface for Wi-Fi Direct
 *	    using cfg80211 interface
 *
 *    This file contains the main routines of Linux driver
 *    for MediaTek Inc. 802.11 Wireless LAN Adapters.
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

#include "config.h"

#if CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/wireless.h>
#include <linux/ieee80211.h>
#include <net/cfg80211.h>

#include "precomp.h"
#include "gl_cfg80211.h"
#include "gl_p2p_os.h"

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wformat"
#endif

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

#if CFG_ENABLE_UNIFY_WIPHY
#define P2P_WIPHY_PRIV(_wiphy, _priv) \
	(_priv = (struct GLUE_INFO *) wiphy_priv(_wiphy))
#else
#define P2P_WIPHY_PRIV(_wiphy, _priv) \
	(_priv = *((struct GLUE_INFO **) wiphy_priv(_wiphy)))
#endif

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

int32_t mtk_Netdev_To_RoleIdx(struct GLUE_INFO *prGlueInfo,
		struct net_device *ndev, uint8_t *pucRoleIdx)
{
	int32_t i4Ret = -1;
	uint32_t u4Idx = 0;

	if ((pucRoleIdx == NULL) || (ndev == NULL))
		return i4Ret;
#if 0
	for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
		if (prGlP2pInfo->aprRoleHandler[u4Idx] == ndev) {
			*pucRoleIdx = (uint8_t) u4Idx;
			i4Ret = 0;
		}
	}
#if  1
	i4Ret = 0;
	*pucRoleIdx = 0;
#endif
#else
	/* The prP2PInfo[0] may be removed and prP2PInfo[1] is existing
	 * under cfg80211 operation. So that check all KAL_P2P_NUM not only
	 * prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum.
	 */
	for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
		if ((prGlueInfo->prP2PInfo[u4Idx] != NULL) &&
		    (prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler != NULL) &&
		    (prGlueInfo->prP2PInfo[u4Idx]->aprRoleHandler == ndev)) {
			*pucRoleIdx = (uint8_t) u4Idx;
			i4Ret = 0;
		}
	}
#endif

	return i4Ret;

}				/* mtk_Netdev_To_RoleIdx */

static void mtk_vif_destructor(struct net_device *dev)
{
	struct wireless_dev *prWdev = NULL;
	uint32_t u4Idx = 0;

	DBGLOG(P2P, INFO, "mtk_vif_destructor\n");
	if (dev) {
		prWdev = dev->ieee80211_ptr;
		free_netdev(dev);
		/* Expect that the gprP2pWdev isn't freed here */
		if ((prWdev) && (prWdev != gprP2pWdev)) {
			/* Role[i] and Dev share the same wdev by default */
			for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
				if (prWdev != gprP2pRoleWdev[u4Idx])
					continue;
				/* In the initWlan gprP2pRoleWdev[0] is equal to
				 * gprP2pWdev. And other gprP2pRoleWdev[] should
				 * be NULL, if the 2nd P2P dev isn't created.
				 */
				if (u4Idx == 0)
					gprP2pRoleWdev[u4Idx] = gprP2pWdev;
				else
					gprP2pRoleWdev[u4Idx] = NULL;
				break;
			}
			kfree(prWdev);
		}
	}
}

#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
struct wireless_dev *mtk_p2p_cfg80211_add_iface(struct wiphy *wiphy,
		const char *name, unsigned char name_assign_type,
		enum nl80211_iftype type, u32 *flags, struct vif_params *params)
#else
struct wireless_dev *mtk_p2p_cfg80211_add_iface(struct wiphy *wiphy,
		const char *name,
		enum nl80211_iftype type, u32 *flags, struct vif_params *params)
#endif
{
	/* 2 TODO: Fit kernel 3.10 modification */
	struct ADAPTER *prAdapter;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct net_device *prNewNetDevice = NULL;
	uint32_t u4Idx = 0;
	struct GL_P2P_INFO *prP2pInfo = NULL;
	struct GL_HIF_INFO *prHif = NULL;
	struct MSG_P2P_SWITCH_OP_MODE *prSwitchModeMsg = NULL;
	struct wireless_dev *prWdev = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = NULL;
	uint8_t rMacAddr[PARAM_MAC_ADDR_LEN];
	struct MSG_P2P_ACTIVE_DEV_BSS *prMsgActiveBss = NULL;
	struct mt66xx_chip_info *prChipInfo;
	struct wireless_dev *prOrigWdev = NULL;

	do {
		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (prGlueInfo == NULL)
			return ERR_PTR(-EINVAL);

		prAdapter = prGlueInfo->prAdapter;
		prChipInfo = prAdapter->chip_info;

		for (u4Idx = 0; u4Idx < KAL_P2P_NUM; u4Idx++) {
			prP2pInfo = prGlueInfo->prP2PInfo[u4Idx];
			/* Expect that only create the new dev with the p2p0 */
			if (prP2pInfo == NULL)
				continue;
			if (prP2pInfo->aprRoleHandler ==
					prP2pInfo->prDevHandler)
				break;
			if (prP2pInfo->aprRoleHandler == NULL) {
				p2pRoleFsmInit(prGlueInfo->prAdapter, u4Idx);
				init_completion(&prP2pInfo->rStopApComp);
				break;
			}
		}

		/*u4Idx = 0;*/
		DBGLOG(P2P, TRACE, "%s: u4Idx=%d\n", __func__, u4Idx);

		if (u4Idx == KAL_P2P_NUM) {
			/* Role port full. */
			return ERR_PTR(-EINVAL);
		}

		prP2pInfo = prGlueInfo->prP2PInfo[u4Idx];

		/* Alloc all resource here to avoid do unregister_netdev for
		 * error case (kernel exception).
		 */
#if KERNEL_VERSION(3, 18, 0) <= CFG80211_VERSION_CODE
		prNewNetDevice = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), name,
			NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);
#else
		prNewNetDevice = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), name,
			ether_setup, CFG_MAX_TXQ_NUM);
#endif

		if (prNewNetDevice == NULL) {
			DBGLOG(P2P, ERROR, "can't alloc prNewNetDevice\n");
			break;
		}

		prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
		if (prWdev == NULL) {
			DBGLOG(P2P, ERROR, "can't alloc prWdev\n");
			break;
		}

		prSwitchModeMsg = (struct MSG_P2P_SWITCH_OP_MODE *) cnmMemAlloc(
					prGlueInfo->prAdapter, RAM_TYPE_MSG,
					sizeof(struct MSG_P2P_SWITCH_OP_MODE));
		if (prSwitchModeMsg == NULL) {
			DBGLOG(P2P, ERROR, "can't alloc prSwitchModeMsg\n");
			break;
		}

		prMsgActiveBss = (struct MSG_P2P_ACTIVE_DEV_BSS *) cnmMemAlloc(
					prGlueInfo->prAdapter, RAM_TYPE_MSG,
					sizeof(struct MSG_P2P_ACTIVE_DEV_BSS));

		if (prMsgActiveBss == NULL) {
			DBGLOG(P2P, ERROR, "can't alloc prMsgActiveBss\n");
			break;
		}

		DBGLOG(P2P, INFO, "type: %d, name = %s, netdev: 0x%p\n",
				type, name, prNewNetDevice);

		prP2pInfo->aprRoleHandler = prNewNetDevice;
		*((struct GLUE_INFO **) netdev_priv(prNewNetDevice)) =
			prGlueInfo;
		prNewNetDevice->needed_headroom =
			NIC_TX_DESC_AND_PADDING_LENGTH +
			prChipInfo->txd_append_size;
		prNewNetDevice->netdev_ops = &p2p_netdev_ops;

		prHif = &prGlueInfo->rHifInfo;
		ASSERT(prHif);

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
		SET_NETDEV_DEV(prNewNetDevice, &(prHif->func->dev));
#endif
#endif

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
		kalMemCopy(prWdev, gprP2pWdev, sizeof(struct wireless_dev));
		prWdev->netdev = prNewNetDevice;
		prWdev->iftype = type;
		prNewNetDevice->ieee80211_ptr = prWdev;
		/* register destructor function for virtual interface */
#if KERNEL_VERSION(4, 14, 0) <= CFG80211_VERSION_CODE
		prNewNetDevice->priv_destructor = mtk_vif_destructor;
#else
		prNewNetDevice->destructor = mtk_vif_destructor;
#endif
		/* The prOrigWdev is used to do error handle. If return fail,
		 * set the gprP2pRoleWdev[u4Idx] to original value.
		 * Expect that the gprP2pRoleWdev[0] = gprP2pWdev, and the
		 * other is NULL.
		 */
		prOrigWdev = gprP2pRoleWdev[u4Idx];
		gprP2pRoleWdev[u4Idx] = prWdev;
		/*prP2pInfo->prRoleWdev[0] = prWdev;*//* TH3 multiple P2P */
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
		/* set HW checksum offload */
		if (prAdapter->fgIsSupportCsumOffload)
			prNewNetDevice->features |= NETIF_F_IP_CSUM
				| NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM;
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

		kalResetStats(prNewNetDevice);
		/* net device initialize */

		/* register for net device */
		if (register_netdevice(prP2pInfo->aprRoleHandler) < 0) {
			DBGLOG(INIT, WARN,
				"unable to register netdevice for p2p\n");
			break;

		} else {
			DBGLOG(P2P, TRACE, "register_netdev OK\n");
			prGlueInfo->prAdapter->rP2PNetRegState =
				ENUM_NET_REG_STATE_REGISTERED;

			netif_carrier_off(prP2pInfo->aprRoleHandler);
			netif_tx_stop_all_queues(prP2pInfo->aprRoleHandler);
		}
		prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[u4Idx];

		/* 13. bind netdev pointer to netdev index */
		wlanBindBssIdxToNetInterface(prGlueInfo,
			prP2pRoleFsmInfo->ucBssIndex,
			(void *) prP2pInfo->aprRoleHandler);
		prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prP2pInfo->aprRoleHandler);
		prNetDevPriv->prGlueInfo = prGlueInfo;
		prNetDevPriv->ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;
#if CFG_ENABLE_UNIFY_WIPHY
		/* Expect that only P2P device uses the cfg80211_add_iface */
		prNetDevPriv->ucIsP2p = TRUE;
#endif

		/* 4.2 fill hardware address */
		COPY_MAC_ADDR(rMacAddr, prAdapter->rMyMacAddr);
		if (prGlueInfo->prAdapter->rWifiVar.ucP2pShareMacAddr &&
			(type == NL80211_IFTYPE_P2P_CLIENT
			|| type == NL80211_IFTYPE_P2P_GO)) {
			rMacAddr[0] = gPrP2pDev[0]->dev_addr[0];
		} else {
			/* change to local administrated address */
			rMacAddr[0] |= 0x2;
			if (u4Idx > 0)
				rMacAddr[0] ^= u4Idx << 2;
			else
				rMacAddr[0] ^=
					prGlueInfo->prAdapter
						->prP2pInfo->u4DeviceNum << 2;
		}
		kal_eth_hw_addr_set(prNewNetDevice, rMacAddr);
		kalMemCopy(prNewNetDevice->perm_addr, rMacAddr, ETH_ALEN);

		DBGLOG(P2P, TRACE,
			"mtk_p2p_cfg80211_add_iface ucBssIdx=%d\n",
			prNetDevPriv->ucBssIdx);

		/* Switch OP MOde. */
		prSwitchModeMsg->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
		prSwitchModeMsg->ucRoleIdx = 0;
		switch (type) {
		case NL80211_IFTYPE_P2P_CLIENT:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_CLIENT.\n");
			prSwitchModeMsg->eIftype = IFTYPE_P2P_CLIENT;
			prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
			kalP2PSetRole(prGlueInfo, 1, u4Idx);
			break;
		case NL80211_IFTYPE_STATION:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_STATION.\n");
			prSwitchModeMsg->eIftype = IFTYPE_STATION;
			prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
			kalP2PSetRole(prGlueInfo, 1, u4Idx);
			break;
		case NL80211_IFTYPE_AP:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_AP.\n");
			prSwitchModeMsg->eIftype = IFTYPE_AP;
			prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
			kalP2PSetRole(prGlueInfo, 2, u4Idx);
			break;
		case NL80211_IFTYPE_P2P_GO:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_GO not AP.\n");
			prSwitchModeMsg->eIftype = IFTYPE_P2P_GO;
			prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
			kalP2PSetRole(prGlueInfo, 2, u4Idx);
			break;
		default:
			DBGLOG(P2P, TRACE, "Other type :%d .\n", type);
			prSwitchModeMsg->eIftype = IFTYPE_P2P_DEVICE;
			prSwitchModeMsg->eOpMode = OP_MODE_P2P_DEVICE;
			kalP2PSetRole(prGlueInfo, 0, u4Idx);
			break;
		}
		mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prSwitchModeMsg,
			MSG_SEND_METHOD_BUF);

		/* Send Msg to DevFsm and active P2P dev BSS */
		prMsgActiveBss->rMsgHdr.eMsgId = MID_MNY_P2P_ACTIVE_BSS;
		mboxSendMsg(prGlueInfo->prAdapter, MBOX_ID_0,
			(struct MSG_HDR *) prMsgActiveBss, MSG_SEND_METHOD_BUF);
		/* Success */
		return prWdev;
	} while (FALSE);

	/* Start Error Handle */

	if (prNewNetDevice != NULL) {
		free_netdev(prNewNetDevice);
		prP2pInfo->aprRoleHandler = NULL;
	}

	if (prWdev != NULL) {
		kfree(prWdev);

		if ((gprP2pRoleWdev[u4Idx] != NULL) &&
		    (gprP2pRoleWdev[u4Idx] != gprP2pWdev)) {
			gprP2pRoleWdev[u4Idx] = prOrigWdev;
		}
	}

	if (prSwitchModeMsg != NULL)
		cnmMemFree(prAdapter, prSwitchModeMsg);

	if (prMsgActiveBss != NULL)
		cnmMemFree(prAdapter, prMsgActiveBss);

	return ERR_PTR(-ENOMEM);
}				/* mtk_p2p_cfg80211_add_iface */

int mtk_p2p_cfg80211_del_iface(struct wiphy *wiphy, struct wireless_dev *wdev)
{
#if 0
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct MSG_P2P_DEL_IFACE *prP2pDelIfaceMsg =
		(struct MSG_P2P_DEL_IFACE *) NULL;
	struct MSG_P2P_ACTIVE_DEV_BSS *prMsgActiveBss =
		(struct MSG_P2P_ACTIVE_DEV_BSS *) NULL;

	prGlueInfo = *((struct GLUE_INFO **) wiphy_priv(wiphy));
	if (prGlueInfo == NULL)
		return -EINVAL;

	prP2pDelIfaceMsg = (struct MSG_P2P_DEL_IFACE *)
		cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
		sizeof(struct MSG_P2P_DEL_IFACE));

	if (prP2pDelIfaceMsg == NULL) {
		ASSERT(FALSE);
		DBGLOG(INIT, WARN, "unable to alloc msg\n");
	} else {
		prP2pDelIfaceMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DEL_IFACE;
		prP2pDelIfaceMsg->ucRoleIdx = 0;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pDelIfaceMsg,
			MSG_SEND_METHOD_BUF);

		/* Send Msg to DevFsm and Deactive P2P dev BSS */
		prMsgActiveBss = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG,
			sizeof(struct MSG_P2P_ACTIVE_DEV_BSS));
		prMsgActiveBss->rMsgHdr.eMsgId = MID_MNY_P2P_ACTIVE_BSS;
		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgActiveBss,
			MSG_SEND_METHOD_BUF);
	}

#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct MSG_P2P_DEL_IFACE *prP2pDelIfaceMsg =
		(struct MSG_P2P_DEL_IFACE *) NULL;
	struct ADAPTER *prAdapter;
	struct GL_P2P_INFO *prP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	struct GL_P2P_DEV_INFO *prP2pGlueDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;
	struct net_device *UnregRoleHander = (struct net_device *)NULL;
	unsigned char ucBssIdx = 0;
	struct BSS_INFO *prP2pBssInfo = NULL;
#if 1
	struct cfg80211_scan_request *prScanRequest = NULL;
#endif

	GLUE_SPIN_LOCK_DECLARATION();

	DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_del_iface\n");

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	if (prGlueInfo == NULL)
		return -EINVAL;

	prAdapter = prGlueInfo->prAdapter;
	prP2pInfo = prGlueInfo->prP2PInfo[0];
	prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

	if ((prP2pInfo == NULL) ||
	    (prP2pInfo->aprRoleHandler == NULL) ||
	    (prP2pInfo->aprRoleHandler == prP2pInfo->prDevHandler)) {
		/* This iface isn't added. */
		return -EINVAL;
	}

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_DEL_INF);

	prP2pRoleFsmInfo = prAdapter->rWifiVar.aprP2pRoleFsmInfo[0];
	if (prP2pRoleFsmInfo == NULL) {
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_DEL_INF);
		return -EINVAL;
	}

	ucBssIdx = prP2pRoleFsmInfo->ucBssIndex;
	wlanBindBssIdxToNetInterface(prGlueInfo, ucBssIdx,
		(void *) prGlueInfo->prP2PInfo[0]->prDevHandler);

	UnregRoleHander = prP2pInfo->aprRoleHandler;

	/* fix that the kernel warning occures when the GC is connected */
	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if ((prP2pBssInfo != NULL) &&
	    (prP2pBssInfo->eConnectionState == MEDIA_STATE_CONNECTED) &&
	    (wdev->iftype == NL80211_IFTYPE_P2P_CLIENT)) {
#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 2, 0) <= CFG80211_VERSION_CODE)
		cfg80211_disconnected(UnregRoleHander, 0, NULL, 0, TRUE,
					GFP_KERNEL);
#else
		cfg80211_disconnected(UnregRoleHander, 0, NULL, 0, GFP_KERNEL);
#endif
	}

	/* Wait for kalSendCompleteAndAwakeQueue() complete */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prP2pInfo->aprRoleHandler = prP2pInfo->prDevHandler;
#if 1
	prScanRequest = prP2pGlueDevInfo->prScanRequest;
	if ((prScanRequest != NULL) &&
	    (prScanRequest->wdev == UnregRoleHander->ieee80211_ptr)) {
		kalCfg80211ScanDone(prScanRequest, TRUE);
		prP2pGlueDevInfo->prScanRequest = NULL;
	}
#else
	/* 2017/12/18: This part is for error case that p2p-p2p0-0 which is
	 * deleted when doing scan causes some exception for scan done action.
	 * The newest driver doesn't observed this error case, so just do the
	 * normal scan done process (use prP2pGlueDevInfo->prScanRequest, not
	 * prP2pGlueDevInfo->rBackupScanRequest). Keep this part for the
	 * reference, if the exception case occurs again.
	 * Can reference the related part in the mtk_p2p_cfg80211_scan.
	 */
	if (prP2pGlueDevInfo->prScanRequest != NULL) {
		/* Check the wdev with backup scan req due to */
		/* the kernel will free this request by error handling */
		if (prP2pGlueDevInfo->rBackupScanRequest.wdev
			== UnregRoleHander->ieee80211_ptr) {
			kalCfg80211ScanDone(
				&(prP2pGlueDevInfo->rBackupScanRequest), TRUE);
			/* clear the request to avoid the Role FSM
			 * calls the scan_done again
			 */
			prP2pGlueDevInfo->prScanRequest = NULL;
		}
	}
#endif
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	/* Wait for wlanSchedCfg80211WorkQueue() complete */
#if CFG_SUPPORT_CFG80211_QUEUE
	flush_delayed_work(&cfg80211_workq);
#endif

	/* prepare for removal */
	if (netif_carrier_ok(UnregRoleHander))
		netif_carrier_off(UnregRoleHander);

	netif_tx_stop_all_queues(UnregRoleHander);

	/* Here are functions which need rtnl_lock */
	unregister_netdevice(UnregRoleHander);
	/* free is called at destructor */
	/* free_netdev(UnregRoleHander); */

	KAL_RELEASE_MUTEX(prAdapter, MUTEX_DEL_INF);

	prP2pDelIfaceMsg = (struct MSG_P2P_DEL_IFACE *)
		cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
			sizeof(struct MSG_P2P_DEL_IFACE));

	if (prP2pDelIfaceMsg == NULL) {
		DBGLOG(INIT, WARN, "unable to alloc msg\n");
	} else {
		prP2pDelIfaceMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DEL_IFACE;
		prP2pDelIfaceMsg->ucRoleIdx = 0;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pDelIfaceMsg,
			MSG_SEND_METHOD_BUF);
	}
#endif
	return 0;
}				/* mtk_p2p_cfg80211_del_iface */

int mtk_p2p_cfg80211_add_key(struct wiphy *wiphy,
		 struct net_device *ndev,
		 u8 key_index, bool pairwise,
		 const u8 *mac_addr,
		 struct key_params *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int32_t i4Rslt = -EINVAL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	struct P2P_PARAM_KEY rKey;
	uint8_t ucRoleIdx = 0;
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	/* const UINT_8 aucZeroMacAddr[] = NULL_MAC_ADDR; */

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0)
		return -EINVAL;

	DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_add_key\n");
#if DBG
	if (mac_addr) {
		DBGLOG(RSN, INFO,
			"keyIdx = %d pairwise = %d mac = " MACSTR "\n",
			key_index, pairwise, MAC2STR(mac_addr));
	} else {
		DBGLOG(RSN, INFO,
			"keyIdx = %d pairwise = %d null mac\n",
			key_index, pairwise);
	}
	DBGLOG(RSN, TRACE, "Cipher = %x\n", params->cipher);
	DBGLOG_MEM8(RSN, TRACE, params->key, params->key_len);
#endif

	if (params->key_len > 32) {
		DBGLOG(RSN, WARN, "key_len [%d] is invalid!\n",
			params->key_len);
		return -EINVAL;
	}

	/* Todo:: By Cipher to set the key */

	kalMemZero(&rKey, sizeof(struct P2P_PARAM_KEY));

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &rKey.ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;

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
#if 0
		case WLAN_CIPHER_SUITE_GCMP:
			rKey.ucCipher = CIPHER_SUITE_GCMP;
			break;
		case WLAN_CIPHER_SUITE_CCMP_256:
			rKey.ucCipher = CIPHER_SUITE_CCMP256;
			break;
#endif
		case WLAN_CIPHER_SUITE_SMS4:
			rKey.ucCipher = CIPHER_SUITE_WPI;
			break;
		case WLAN_CIPHER_SUITE_AES_CMAC:
			rKey.ucCipher = CIPHER_SUITE_BIP;
			break;
		default:
			ASSERT(FALSE);
		}
	}

	/* For BC addr case: ex: AP mode,
	 * driver_nl80211 will not have mac_addr
	 */
	if (pairwise) {
		rKey.u4KeyIndex |= BIT(31);	/* Tx */
		rKey.u4KeyIndex |= BIT(30);	/* Pairwise */
		COPY_MAC_ADDR(rKey.arBSSID, mac_addr);
	} else {
		COPY_MAC_ADDR(rKey.arBSSID, aucBCAddr);
	}

	/* Check if add key under AP mode */
	if (kalP2PGetRole(prGlueInfo, ucRoleIdx) == 2)
		rKey.u4KeyIndex |= BIT(28); /* authenticator */


	if (params->key)
		kalMemCopy(rKey.aucKeyMaterial, params->key, params->key_len);
	rKey.u4KeyLength = params->key_len;
	rKey.u4Length = ((unsigned long)
		&(((struct P2P_PARAM_KEY *) 0)->aucKeyMaterial))
		+ rKey.u4KeyLength;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidSetAddKey,
		&rKey, rKey.u4Length, FALSE, FALSE, TRUE, &u4BufLen);

	if (rStatus == WLAN_STATUS_SUCCESS)
		i4Rslt = 0;

	return i4Rslt;
}

int mtk_p2p_cfg80211_get_key(struct wiphy *wiphy,
		struct net_device *ndev,
		u8 key_index,
		bool pairwise,
		const u8 *mac_addr, void *cookie,
		void (*callback)
			(void *cookie, struct key_params *))
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	/* not implemented yet */
	DBGLOG(RSN, INFO, "not support this func\n");

	return -EINVAL;
}

int mtk_p2p_cfg80211_del_key(struct wiphy *wiphy,
		struct net_device *ndev,
		u8 key_index, bool pairwise, const u8 *mac_addr)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_REMOVE_KEY rRemoveKey;
	int32_t i4Rslt = -EINVAL;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;
	uint8_t ucRoleIdx = 0;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);
	DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_del_key\n");

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) < 0)
		return -EINVAL;
#if DBG
	if (mac_addr) {
		DBGLOG(RSN, TRACE,
		       "keyIdx = %d pairwise = %d mac = " MACSTR "\n",
		       key_index, pairwise, MAC2STR(mac_addr));
	} else {
		DBGLOG(RSN, TRACE,
			"keyIdx = %d pairwise = %d null mac\n",
			key_index, pairwise);
	}
#endif

	kalMemZero(&rRemoveKey, sizeof(struct PARAM_REMOVE_KEY));

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &rRemoveKey.ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	if (mac_addr)
		COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
	rRemoveKey.u4KeyIndex = key_index;
	rRemoveKey.u4Length = sizeof(struct PARAM_REMOVE_KEY);
	if (mac_addr) {
		COPY_MAC_ADDR(rRemoveKey.arBSSID, mac_addr);
		rRemoveKey.u4KeyIndex |= BIT(30);
	}

	rStatus = kalIoctl(prGlueInfo,
			wlanoidSetRemoveKey,
			&rRemoveKey, rRemoveKey.u4Length,
			FALSE, FALSE, TRUE, &u4BufLen);

	if (rStatus == WLAN_STATUS_SUCCESS)
		i4Rslt = 0;

	return i4Rslt;
}

int
mtk_p2p_cfg80211_set_default_key(struct wiphy *wiphy,
		struct net_device *netdev,
		u8 key_index, bool unicast, bool multicast)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct PARAM_DEFAULT_KEY rDefaultKey;
	uint8_t ucRoleIdx = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4Rst = -EINVAL;
	uint32_t u4BufLen = 0;
	u_int8_t fgDef = FALSE, fgMgtDef = FALSE;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);
	DBGLOG(RSN, TRACE, "mtk_p2p_cfg80211_set_default_key\n");

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, netdev, &ucRoleIdx) != 0)
		return -EINVAL;
#if DBG
	DBGLOG(RSN, TRACE,
		"keyIdx = %d unicast = %d multicast = %d\n",
		key_index, unicast, multicast);
#endif


	/* For wep case, this set the key for tx */
	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &rDefaultKey.ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;


	rDefaultKey.ucKeyID = key_index;
	rDefaultKey.ucUnicast = unicast;
	rDefaultKey.ucMulticast = multicast;
	if (rDefaultKey.ucUnicast && !rDefaultKey.ucMulticast)
		return WLAN_STATUS_SUCCESS;

	if (rDefaultKey.ucUnicast && rDefaultKey.ucMulticast)
		fgDef = TRUE;

	if (!rDefaultKey.ucUnicast && rDefaultKey.ucMulticast)
		fgMgtDef = TRUE;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidSetDefaultKey,
		&rDefaultKey,
		sizeof(struct PARAM_DEFAULT_KEY),
		FALSE, FALSE, TRUE, &u4BufLen);


	if (rStatus == WLAN_STATUS_SUCCESS)
		i4Rst = 0;


	return i4Rst;
}

/*---------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for setting the default mgmt key index
 *
 * @param
 *
 * @retval 0:       successful
 *         others:  failure
 */
/*---------------------------------------------------------------------------*/
int mtk_p2p_cfg80211_set_mgmt_key(struct wiphy *wiphy,
		struct net_device *dev, u8 key_index)
{
	DBGLOG(RSN, INFO, "mtk_p2p_cfg80211_set_mgmt_key, kid:%d\n", key_index);

	return 0;
}

#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_get_station(struct wiphy *wiphy,
	struct net_device *ndev,
	const u8 *mac, struct station_info *sinfo)
#else
int mtk_p2p_cfg80211_get_station(struct wiphy *wiphy,
	struct net_device *ndev,
	u8 *mac, struct station_info *sinfo)
#endif
{
	int32_t i4RetRslt = -EINVAL;
	int32_t i4Rssi = 0;
	uint8_t ucRoleIdx = 0;
	uint8_t ucBssIdx = 0;
	uint32_t u4Rate = 0;
	uint32_t u4BufLen = 0;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct GL_P2P_INFO *prP2pGlueInfo = (struct GL_P2P_INFO *) NULL;
	struct P2P_STATION_INFO rP2pStaInfo;
	struct BSS_INFO *prBssInfo;
	struct PARAM_LINK_SPEED_EX rLinkSpeed;

	kalMemZero(&rLinkSpeed, sizeof(struct PARAM_LINK_SPEED_EX));

	ASSERT(wiphy);

	do {
		if ((wiphy == NULL) || (ndev == NULL)
			|| (sinfo == NULL) || (mac == NULL))
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_get_station\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0)
			return -EINVAL;

		prP2pGlueInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		/* Get station information. */
		/* 1. Inactive time? */
		p2pFuncGetStationInfo(prGlueInfo->prAdapter,
			(uint8_t *)mac, &rP2pStaInfo);

		/* Inactive time. */
#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT(NL80211_STA_INFO_INACTIVE_TIME);
#else
		sinfo->filled |= STATION_INFO_INACTIVE_TIME;
#endif
		sinfo->inactive_time = rP2pStaInfo.u4InactiveTime;
		sinfo->generation = prP2pGlueInfo->i4Generation;

		/* 2. fill TX rate */
		if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
			ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS)
			return -EINVAL;
		prBssInfo =
			GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIdx);
		if (!prBssInfo) {
			DBGLOG(P2P, WARN, "bss is not active\n");
			return -EINVAL;
		}
		if (prBssInfo->eConnectionState
			!= MEDIA_STATE_CONNECTED) {
			/* not connected */
			DBGLOG(P2P, WARN, "not yet connected\n");
			return 0;
		}

		rStatus = kalIoctlByBssIdx(prGlueInfo,
				 wlanoidQueryLinkSpeedEx,
				 &rLinkSpeed, sizeof(rLinkSpeed),
				 TRUE, FALSE, TRUE,
				 &u4BufLen, ucBssIdx);
		if (rStatus == WLAN_STATUS_SUCCESS
			&& ucBssIdx < BSSID_NUM) {
			u4Rate = rLinkSpeed.rLq[ucBssIdx].u2LinkSpeed;
			i4Rssi = rLinkSpeed.rLq[ucBssIdx].cRssi;
		}

		if (i4Rssi > PARAM_WHQL_RSSI_MAX_DBM)
			i4Rssi = PARAM_WHQL_RSSI_MAX_DBM;
		else if (i4Rssi < PARAM_WHQL_RSSI_MIN_DBM)
			i4Rssi = PARAM_WHQL_RSSI_MIN_DBM;
		/* convert from 100bps to 100kbps */
		sinfo->txrate.legacy = u4Rate / 1000;
		sinfo->signal = i4Rssi;

		DBGLOG(P2P, INFO,
			"ucRoleIdx = %d, rate = %u, signal = %d\n",
			ucRoleIdx,
			sinfo->txrate.legacy,
			sinfo->signal);

#if KERNEL_VERSION(4, 0, 0) <= CFG80211_VERSION_CODE
		sinfo->filled |= BIT(NL80211_STA_INFO_TX_BITRATE);
		sinfo->filled |= BIT(NL80211_STA_INFO_SIGNAL);
#else
		sinfo->filled |= STATION_INFO_TX_BITRATE;
		sinfo->filled |= STATION_INFO_SIGNAL;
#endif

		i4RetRslt = 0;
	} while (FALSE);

	return i4RetRslt;
}

int mtk_p2p_cfg80211_scan(struct wiphy *wiphy,
		struct cfg80211_scan_request *request)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct GL_P2P_INFO *prP2pGlueInfo = (struct GL_P2P_INFO *) NULL;
	struct GL_P2P_DEV_INFO *prP2pGlueDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;
	struct MSG_P2P_SCAN_REQUEST *prMsgScanRequest = (
		struct MSG_P2P_SCAN_REQUEST *) NULL;
	uint32_t u4MsgSize = 0, u4Idx = 0;
	int32_t i4RetRslt = -EINVAL;
	struct RF_CHANNEL_INFO *prRfChannelInfo =
		(struct RF_CHANNEL_INFO *) NULL;
	struct P2P_SSID_STRUCT *prSsidStruct = (struct P2P_SSID_STRUCT *) NULL;
	struct ieee80211_channel *prChannel = NULL;
	struct cfg80211_ssid *prSsid = NULL;
	uint8_t ucBssIdx = 0;
	u_int8_t fgIsFullChanScan = FALSE;

	/* [-----Channel-----] [-----SSID-----][-----IE-----] */

	do {
		if ((wiphy == NULL) || (request == NULL))
			break;

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (wlanIsChipAssert(prGlueInfo->prAdapter))
			break;

		prP2pGlueInfo = prGlueInfo->prP2PInfo[0];
		prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;

		if ((prP2pGlueInfo == NULL) || (prP2pGlueDevInfo == NULL)) {
			ASSERT(FALSE);
			break;
		}

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_scan.\n");

		if (prP2pGlueDevInfo->prScanRequest != NULL) {
			/* There have been a scan request
			 * on-going processing.
			 */
			DBGLOG(P2P, TRACE,
				"There have been a scan request on-going processing.\n");
			break;
		}

		prP2pGlueDevInfo->prScanRequest = request;

		/* Should find out why the n_channels so many? */
		if (request->n_channels > MAXIMUM_OPERATION_CHANNEL_LIST) {
			request->n_channels = MAXIMUM_OPERATION_CHANNEL_LIST;
			fgIsFullChanScan = TRUE;
			DBGLOG(P2P, TRACE,
				"Channel list exceed the maximun support.\n");
		}
		/* TODO: */
		/* Find a way to distinct DEV port scan & ROLE port scan.
		 */
		ucBssIdx = prGlueInfo->prAdapter->ucP2PDevBssIdx;
		DBGLOG(P2P, TRACE, "Device Port Scan.\n");

		u4MsgSize = sizeof(struct MSG_P2P_SCAN_REQUEST) +
		    (request->n_channels * sizeof(struct RF_CHANNEL_INFO)) +
		    (request->n_ssids * sizeof(struct PARAM_SSID)) +
		    request->ie_len;

		prMsgScanRequest = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, u4MsgSize);

		if (prMsgScanRequest == NULL) {
			i4RetRslt = -ENOMEM;
			break;
		}

		DBGLOG(P2P, TRACE, "Generating scan request message.\n");

		prMsgScanRequest->rMsgHdr.eMsgId = MID_MNY_P2P_DEVICE_DISCOVERY;
		prMsgScanRequest->eScanType = SCAN_TYPE_ACTIVE_SCAN;
		prMsgScanRequest->ucBssIdx = ucBssIdx;

		DBGLOG(P2P, INFO,
			"Requesting channel number:%d.\n", request->n_channels);

		for (u4Idx = 0; u4Idx < request->n_channels; u4Idx++) {
			/* Translate Freq from MHz to channel number. */
			prRfChannelInfo =
				&(prMsgScanRequest->arChannelListInfo[u4Idx]);
			prChannel = request->channels[u4Idx];

			prRfChannelInfo->ucChannelNum =
				nicFreq2ChannelNum(
					prChannel->center_freq * 1000);

			DBGLOG(P2P, TRACE,
			       "Scanning Channel:%d,  freq: %d\n",
			       prRfChannelInfo->ucChannelNum,
			       prChannel->center_freq);

			switch (prChannel->band) {
			case KAL_BAND_2GHZ:
				prRfChannelInfo->eBand = BAND_2G4;
				break;
			case KAL_BAND_5GHZ:
				prRfChannelInfo->eBand = BAND_5G;
				break;
#if (CFG_SUPPORT_WIFI_6G == 1)
			case KAL_BAND_6GHZ:
				prRfChannelInfo->eBand = BAND_6G;
				break;
#endif
			default:
				DBGLOG(P2P, TRACE,
					"UNKNOWN Band info from supplicant\n");
				prRfChannelInfo->eBand = BAND_NULL;
				break;
			}

			/* Iteration. */
			prRfChannelInfo++;
		}
		prMsgScanRequest->u4NumChannel = request->n_channels;
		if (fgIsFullChanScan) {
			prMsgScanRequest->u4NumChannel =
				SCN_P2P_FULL_SCAN_PARAM;
			DBGLOG(P2P, INFO,
				"request->n_channels = SCN_P2P_FULL_SCAN_PARAM\n");
		}
		DBGLOG(P2P, TRACE, "Finish channel list.\n");

		/* SSID */
		prSsid = request->ssids;
		prSsidStruct = (struct P2P_SSID_STRUCT *) prRfChannelInfo;
		if (request->n_ssids) {
			ASSERT((unsigned long) prSsidStruct
				== (unsigned long)
				&(prMsgScanRequest->arChannelListInfo[u4Idx]));

			prMsgScanRequest->prSSID = prSsidStruct;
		}

		for (u4Idx = 0; u4Idx < request->n_ssids; u4Idx++) {
			COPY_SSID(prSsidStruct->aucSsid,
				  prSsidStruct->ucSsidLen,
				  request->ssids->ssid,
				  request->ssids->ssid_len);

			prSsidStruct++;
			prSsid++;
		}

		prMsgScanRequest->i4SsidNum = request->n_ssids;

		DBGLOG(P2P, TRACE, "Finish SSID list:%d.\n", request->n_ssids);

		/* IE BUFFERS */
		prMsgScanRequest->pucIEBuf = (uint8_t *) prSsidStruct;
		if (request->ie_len) {
			kalMemCopy(prMsgScanRequest->pucIEBuf,
				request->ie, request->ie_len);
			prMsgScanRequest->u4IELen = request->ie_len;
		} else {
			prMsgScanRequest->u4IELen = 0;
		}

		DBGLOG(P2P, TRACE, "Finish IE Buffer.\n");

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgScanRequest,
			MSG_SEND_METHOD_BUF);

		/* Backup scan request structure */
		/* The purpose of this backup is due
		 * to the kernel free the scan req
		 * when the wpa supplicant down the iface before down,
		 * and it will free the original scan request structure
		 * In this case, the scan resoure could be locked by kernel,
		 * and driver needs this work around to clear the state
		 */
#if 0
		kalMemCopy(&(prP2pGlueDevInfo->rBackupScanRequest),
			prP2pGlueDevInfo->prScanRequest,
			sizeof(struct cfg80211_scan_request));
#endif
		i4RetRslt = 0;
	} while (FALSE);

	return i4RetRslt;
}				/* mtk_p2p_cfg80211_scan */

void mtk_p2p_cfg80211_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev)
{
	uint32_t u4SetInfoLen = 0;
	uint32_t u4Value = 0;
	uint32_t rStatus;
	struct GLUE_INFO *prGlueInfo = NULL;

	DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_abort_scan\n");

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	rStatus = kalIoctl(prGlueInfo,
		wlanoidAbortP2pScan,
		&u4Value, sizeof(u4Value),
		FALSE, FALSE, TRUE, &u4SetInfoLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(REQ, ERROR,
			"mtk_p2p_cfg80211_abort_scan fail 0x%x\n",
			rStatus);
}

int mtk_p2p_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = NULL;

	do {
		if (wiphy == NULL)
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_wiphy_params\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (changed & WIPHY_PARAM_RETRY_SHORT) {
			/* TODO: */
			DBGLOG(P2P, TRACE,
				"The RETRY short param is changed.\n");
		}

		if (changed & WIPHY_PARAM_RETRY_LONG) {
			/* TODO: */
			DBGLOG(P2P, TRACE,
				"The RETRY long param is changed.\n");
		}

		if (changed & WIPHY_PARAM_FRAG_THRESHOLD) {
			/* TODO: */
			DBGLOG(P2P, TRACE,
				"The RETRY fragmentation threshold is changed.\n");
		}

		if (changed & WIPHY_PARAM_RTS_THRESHOLD) {
			/* TODO: */
			DBGLOG(P2P, TRACE,
				"The RETRY RTS threshold is changed.\n");
		}

		if (changed & WIPHY_PARAM_COVERAGE_CLASS) {
			/* TODO: */
			DBGLOG(P2P, TRACE,
				"The coverage class is changed???\n");
		}

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_set_wiphy_params */

int mtk_p2p_cfg80211_join_ibss(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_ibss_params *params)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, INFO, "not support now\n");
	/* not implemented yet */

	return -EINVAL;
}

int mtk_p2p_cfg80211_leave_ibss(struct wiphy *wiphy, struct net_device *dev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, INFO, "not support now\n");
	/* not implemented yet */

	return -EINVAL;
}

int mtk_p2p_cfg80211_set_txpower(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		enum nl80211_tx_power_setting type, int mbm)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(wiphy);

	DBGLOG(P2P, INFO, "%s: not support now\n", __func__);
	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	/* not implemented yet */

	return -EINVAL;
}

int mtk_p2p_cfg80211_get_txpower(struct wiphy *wiphy,
		struct wireless_dev *wdev, int *dbm)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	ASSERT(wiphy);

	DBGLOG(P2P, TRACE, "%s: not support now\n", __func__);
	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	/* not implemented yet */

	return -EINVAL;
}

int mtk_p2p_cfg80211_set_power_mgmt(struct wiphy *wiphy,
		struct net_device *ndev, bool enabled, int timeout)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	enum PARAM_POWER_MODE ePowerMode;
	struct PARAM_POWER_MODE_ rPowerMode;
	uint32_t rStatus;
	uint32_t u4Leng;
	uint8_t ucRoleIdx;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	if (prGlueInfo->prAdapter->rWifiVar.ucDisP2pPs)
		return 0;

	if (enabled)
		ePowerMode = Param_PowerModeFast_PSP;
	else
		ePowerMode = Param_PowerModeCAM;

	DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_power_mgmt ps=%d.\n", enabled);

	if (mtk_Netdev_To_RoleIdx(prGlueInfo,
		ndev, &ucRoleIdx) != 0)
		return -EINVAL;

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &rPowerMode.ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	rPowerMode.ePowerMode = ePowerMode;

	/* p2p_set_power_save */
	rStatus = kalIoctl(prGlueInfo,
		wlanoidSet802dot11PowerSaveProfile,
		&rPowerMode, sizeof(rPowerMode),
		FALSE, FALSE, TRUE, &u4Leng);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, WARN, "set_power_mgmt error:%x\n", rStatus);
		return -EFAULT;
	}

	return 0;
}

int mtk_p2p_cfg80211_start_ap(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_ap_settings *settings)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_BEACON_UPDATE *prP2pBcnUpdateMsg =
		(struct MSG_P2P_BEACON_UPDATE *) NULL;
	struct MSG_P2P_START_AP *prP2pStartAPMsg =
		(struct MSG_P2P_START_AP *) NULL;
	uint8_t *pucBuffer = (uint8_t *) NULL;
	uint8_t ucRoleIdx = 0;
	struct cfg80211_chan_def *chandef;
	struct RF_CHANNEL_INFO rRfChnlInfo;

	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	/* RF_CHANNEL_INFO_T rRfChnlInfo; */
/* P_IE_SSID_T prSsidIE = (P_IE_SSID_T)NULL; */

	do {
		if ((wiphy == NULL) || (settings == NULL))
			break;

		DBGLOG(P2P, INFO, "inactivity_timeout: %d\n",
				settings->inactivity_timeout);
		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		/*DFS todo 20161220_DFS*/
		netif_carrier_on(dev);
		netif_tx_start_all_queues(dev);

		chandef = &settings->chandef;

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		if (chandef) {
			kalChannelFormatSwitch(chandef, chandef->chan,
					&rRfChnlInfo);

			DBGLOG(P2P, INFO, "pri: %d, s1: %d, s2: %d, bw: %d\n",
				rRfChnlInfo.u2PriChnlFreq,
				rRfChnlInfo.u4CenterFreq1,
				rRfChnlInfo.u4CenterFreq2,
				rRfChnlInfo.ucChnlBw);

			/* Follow the channel info from wifi.cfg
			 * prior to hostapd.conf
			 */
			{
				struct ADAPTER *prAdapter =
					(struct ADAPTER *) NULL;
				struct WIFI_VAR *prWifiVar =
					(struct WIFI_VAR *)NULL;

				prAdapter = prGlueInfo->prAdapter;
				prWifiVar = &prAdapter->rWifiVar;

				if ((prWifiVar->ucApChannel != 0) &&
					(prWifiVar->ucApChnlDefFromCfg != 0) &&
					(prWifiVar->ucApChannel !=
					rRfChnlInfo.ucChannelNum)) {
					rRfChnlInfo.ucChannelNum =
						prWifiVar->ucApChannel;
					rRfChnlInfo.eBand =
						(rRfChnlInfo.ucChannelNum <= 14)
						? BAND_2G4 : BAND_5G;
					/* [TODO][20160829]If we will set SCO
					 * by nl80211_channel_type afterward,
					 * to check if we need to modify SCO
					 * by wifi.cfg here
					 */
				}
			}

			p2pFuncSetChannel(prGlueInfo->prAdapter,
				ucRoleIdx, &rRfChnlInfo);
		} else
			DBGLOG(P2P, INFO, "!!! no CH def!!!\n");

		prP2pBcnUpdateMsg = (struct MSG_P2P_BEACON_UPDATE *)
			cnmMemAlloc(prGlueInfo->prAdapter,
			    RAM_TYPE_MSG,
			    (sizeof(struct MSG_P2P_BEACON_UPDATE)
			     +
			     settings->beacon.head_len +
			     settings->beacon.tail_len +
			     settings->beacon.assocresp_ies_len
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
				 + settings->beacon.proberesp_ies_len
#endif
			     ));

		if (prP2pBcnUpdateMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
		prP2pBcnUpdateMsg->rMsgHdr.eMsgId = MID_MNY_P2P_BEACON_UPDATE;
		pucBuffer = prP2pBcnUpdateMsg->aucBuffer;

#if (CFG_SUPPORT_DFS_MASTER == 1)
		if (p2pFuncGetDfsState() == DFS_STATE_DETECTED)
			p2pFuncSetDfsState(DFS_STATE_INACTIVE);
#endif
		if (settings->beacon.head_len != 0) {
			kalMemCopy(pucBuffer,
				settings->beacon.head,
				settings->beacon.head_len);

			prP2pBcnUpdateMsg->u4BcnHdrLen =
				settings->beacon.head_len;

			prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

			pucBuffer += settings->beacon.head_len;
		} else {
			prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

			prP2pBcnUpdateMsg->pucBcnHdr = NULL;
		}

		if (settings->beacon.tail_len != 0) {
			prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
			kalMemCopy(pucBuffer,
				settings->beacon.tail,
				settings->beacon.tail_len);

			prP2pBcnUpdateMsg->u4BcnBodyLen =
				settings->beacon.tail_len;

			pucBuffer += settings->beacon.tail_len;
		} else {
			prP2pBcnUpdateMsg->u4BcnBodyLen = 0;

			prP2pBcnUpdateMsg->pucBcnBody = NULL;
		}

		if ((settings->crypto.cipher_group ==
			 WLAN_CIPHER_SUITE_WEP40) ||
			(settings->crypto.cipher_group ==
			 WLAN_CIPHER_SUITE_WEP104))
			prP2pBcnUpdateMsg->fgIsWepCipher = TRUE;
		else
			prP2pBcnUpdateMsg->fgIsWepCipher = FALSE;

		if (settings->beacon.assocresp_ies_len != 0
			&& settings->beacon.assocresp_ies != NULL) {
			prP2pBcnUpdateMsg->pucAssocRespIE = pucBuffer;
			kalMemCopy(pucBuffer,
				settings->beacon.assocresp_ies,
				settings->beacon.assocresp_ies_len);
			prP2pBcnUpdateMsg->u4AssocRespLen =
				settings->beacon.assocresp_ies_len;
		} else {
			prP2pBcnUpdateMsg->u4AssocRespLen = 0;
			prP2pBcnUpdateMsg->pucAssocRespIE = NULL;
		}

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
		if (settings->beacon.proberesp_ies_len != 0
			&& settings->beacon.proberesp_ies != NULL) {
			prP2pBcnUpdateMsg->pucProbeRespIE = pucBuffer;
			kalMemCopy(pucBuffer,
				settings->beacon.proberesp_ies,
				settings->beacon.proberesp_ies_len);
			prP2pBcnUpdateMsg->u4ProbeRespLen =
				settings->beacon.proberesp_ies_len;
		} else {
			prP2pBcnUpdateMsg->u4ProbeRespLen = 0;
			prP2pBcnUpdateMsg->pucProbeRespIE = NULL;
		}
#endif

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pBcnUpdateMsg,
			MSG_SEND_METHOD_BUF);

		prP2pStartAPMsg = (struct MSG_P2P_START_AP *)
			cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG, sizeof(struct MSG_P2P_START_AP));

		if (prP2pStartAPMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prP2pStartAPMsg->rMsgHdr.eMsgId = MID_MNY_P2P_START_AP;

		prP2pStartAPMsg->fgIsPrivacy = settings->privacy;

		prP2pStartAPMsg->u4BcnInterval = settings->beacon_interval;

		prP2pStartAPMsg->u4DtimPeriod = settings->dtim_period;

		/* Copy NO SSID. */
		prP2pStartAPMsg->ucHiddenSsidType = settings->hidden_ssid;

		prP2pStartAPMsg->ucRoleIdx = ucRoleIdx;

		kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

		COPY_SSID(prP2pStartAPMsg->aucSsid,
			prP2pStartAPMsg->u2SsidLen,
			settings->ssid, settings->ssid_len);

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pStartAPMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;

	} while (FALSE);

	return i4Rslt;

/* /////////////////////// */
/**
 * struct cfg80211_ap_settings - AP configuration
 *
 * Used to configure an AP interface.
 *
 * @beacon: beacon data
 * @beacon_interval: beacon interval
 * @dtim_period: DTIM period
 * @ssid: SSID to be used in the BSS (note: may be %NULL if not provided from
 *      user space)
 * @ssid_len: length of @ssid
 * @hidden_ssid: whether to hide the SSID in Beacon/Probe Response frames
 * @crypto: crypto settings
 * @privacy: the BSS uses privacy
 * @auth_type: Authentication type (algorithm)
 * @inactivity_timeout: time in seconds to determine station's inactivity.
 */
/* struct cfg80211_ap_settings { */
/* struct cfg80211_beacon_data beacon; */
/*  */
/* int beacon_interval, dtim_period; */
/* const u8 *ssid; */
/* size_t ssid_len; */
/* enum nl80211_hidden_ssid hidden_ssid; */
/* struct cfg80211_crypto_settings crypto; */
/* bool privacy; */
/* enum nl80211_auth_type auth_type; */
/* int inactivity_timeout; */
/* }; */
/* ////////////////// */
}				/* mtk_p2p_cfg80211_start_ap */

#if (CFG_SUPPORT_DFS_MASTER == 1)

static int mtk_p2p_cfg80211_start_radar_detection_impl(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_chan_def *chandef,
		unsigned int cac_time_ms)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_DFS_CAC *prP2pDfsCacMsg =
		(struct MSG_P2P_DFS_CAC *) NULL;
	uint8_t ucRoleIdx = 0;
	struct RF_CHANNEL_INFO rRfChnlInfo;

	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	do {
		if ((wiphy == NULL) || (chandef == NULL))
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_start_radar_detection.\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
			prGlueInfo->prP2PInfo[ucRoleIdx]->chandef =
				(struct cfg80211_chan_def *)
					cnmMemAlloc(prGlueInfo->prAdapter,
					RAM_TYPE_BUF,
					sizeof(struct cfg80211_chan_def));
			if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
				i4Rslt = -ENOMEM;
				break;
			}
			prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan =
				(struct ieee80211_channel *)
					cnmMemAlloc(prGlueInfo->prAdapter,
					RAM_TYPE_BUF,
					sizeof(struct ieee80211_channel));
			if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan
				== NULL) {
				i4Rslt = -ENOMEM;
				break;
			}
		}

		/* Copy chan def to local buffer*/
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->center_freq1 = chandef->center_freq1;
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->center_freq2 = chandef->center_freq2;
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->width = chandef->width;
		memcpy(prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan,
			chandef->chan, sizeof(struct ieee80211_channel));
		prGlueInfo->prP2PInfo[ucRoleIdx]->cac_time_ms = cac_time_ms;

		if (chandef) {
			kalChannelFormatSwitch(chandef, chandef->chan,
					&rRfChnlInfo);

			p2pFuncSetChannel(prGlueInfo->prAdapter,
				ucRoleIdx, &rRfChnlInfo);
		}

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_start_radar_detection.(role %d)\n",
			ucRoleIdx);

		p2pFuncSetDfsState(DFS_STATE_INACTIVE);

		prP2pDfsCacMsg = (struct MSG_P2P_DFS_CAC *)
			cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG, sizeof(*prP2pDfsCacMsg));

		if (prP2pDfsCacMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prP2pDfsCacMsg->rMsgHdr.eMsgId = MID_MNY_P2P_DFS_CAC;

		switch (chandef->width) {
		case NL80211_CHAN_WIDTH_20_NOHT:
		case NL80211_CHAN_WIDTH_20:
		case NL80211_CHAN_WIDTH_40:
			prP2pDfsCacMsg->eChannelWidth = CW_20_40MHZ;
			break;

		case NL80211_CHAN_WIDTH_80:
			prP2pDfsCacMsg->eChannelWidth = CW_80MHZ;
			break;

		case NL80211_CHAN_WIDTH_80P80:
			prP2pDfsCacMsg->eChannelWidth = CW_80P80MHZ;
			break;

		default:
			DBGLOG(P2P, ERROR,
				"mtk_p2p_cfg80211_start_radar_detection. !!!Bandwidth do not support!!!\n");
			ASSERT(FALSE);
			break;
		}

		prP2pDfsCacMsg->ucRoleIdx = ucRoleIdx;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pDfsCacMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;

	} while (FALSE);

	return i4Rslt;
}

#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_start_radar_detection(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_chan_def *chandef, unsigned int cac_time_ms)
{
	return mtk_p2p_cfg80211_start_radar_detection_impl(
			wiphy, dev, chandef, cac_time_ms);
}
#else
int mtk_p2p_cfg80211_start_radar_detection(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_chan_def *chandef)
{
	return mtk_p2p_cfg80211_start_radar_detection_impl(
			wiphy, dev, chandef, IEEE80211_DFS_MIN_CAC_TIME_MS);
}
#endif

#if KERNEL_VERSION(3, 13, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_channel_switch(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_csa_settings *params)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_BEACON_UPDATE *prP2pBcnUpdateMsg =
		(struct MSG_P2P_BEACON_UPDATE *) NULL;
	struct MSG_P2P_SET_NEW_CHANNEL *prP2pSetNewChannelMsg =
		(struct MSG_P2P_SET_NEW_CHANNEL *) NULL;
	uint8_t *pucBuffer = (uint8_t *) NULL;
	uint8_t ucRoleIdx = 0;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	uint8_t ucBssIdx = 0;
	uint32_t u4Len = 0;

	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	do {
		if ((wiphy == NULL) || (params == NULL))
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_channel_switch.\n");
		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
			DBGLOG(P2P, ERROR, "get role index fail.\n");
			break;
		} else {
			ASSERT(ucRoleIdx < KAL_P2P_NUM);
			/* Role Interface. */
			if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
				ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS) {
				DBGLOG(P2P, ERROR,
					"get bss index fail by role(%d).\n",
					ucRoleIdx);
				break;
			}
		}

		if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
			prGlueInfo->prP2PInfo[ucRoleIdx]->chandef =
				(struct cfg80211_chan_def *)
				cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_BUF, sizeof(struct cfg80211_chan_def));
			if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef == NULL) {
				i4Rslt = -ENOMEM;
				break;
			}
			prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan =
				(struct ieee80211_channel *)
				cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_BUF, sizeof(struct ieee80211_channel));
			if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan
				== NULL) {
				i4Rslt = -ENOMEM;
				break;
			}
		}
		/* Copy chan def to local buffer*/
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->center_freq1 = params->chandef.center_freq1;
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->center_freq2 = params->chandef.center_freq2;
		prGlueInfo->prP2PInfo[ucRoleIdx]
			->chandef->width = params->chandef.width;
		memcpy(prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan,
			params->chandef.chan,
			sizeof(struct ieee80211_channel));

		if (params) {
			kalChannelFormatSwitch(&params->chandef,
					params->chandef.chan, &rRfChnlInfo);

			p2pFuncSetChannel(prGlueInfo->prAdapter,
				ucRoleIdx, &rRfChnlInfo);
		}

		DBGLOG(P2P, INFO, "ucRoleIdx: %d, ucBssIdx: %d\n",
				ucRoleIdx, ucBssIdx);

		if (prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan->
			dfs_state == NL80211_DFS_AVAILABLE
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
			&& prGlueInfo->prP2PInfo[ucRoleIdx]->chandef->chan->
			dfs_cac_ms != 0
#endif
			)
			p2pFuncSetDfsState(DFS_STATE_ACTIVE);
		else
			p2pFuncSetDfsState(DFS_STATE_INACTIVE);

		/* Set CSA IE parameters */
		prGlueInfo->prAdapter->rWifiVar.ucChannelSwitchMode = 1;
		prGlueInfo->prAdapter->rWifiVar.ucNewChannelNumber =
			nicFreq2ChannelNum(
				params->chandef.chan->center_freq * 1000);
		prGlueInfo->prAdapter->rWifiVar.ucChannelSwitchCount =
			params->count;

		/* To prevent race condition, we have to set CSA flags
		* after all CSA parameters are updated. In this way,
		* we can guarantee that CSA IE will be and only be
		* reported once in the beacon
		*/
		prGlueInfo->prAdapter->rWifiVar.fgCsaInProgress = TRUE;

		/* Set new channel parameters */
		prP2pSetNewChannelMsg = (struct MSG_P2P_SET_NEW_CHANNEL *)
			cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(*prP2pSetNewChannelMsg));

		if (prP2pSetNewChannelMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prP2pSetNewChannelMsg->rMsgHdr.eMsgId =
			MID_MNY_P2P_SET_NEW_CHANNEL;

		switch (params->chandef.width) {
		case NL80211_CHAN_WIDTH_20_NOHT:
		case NL80211_CHAN_WIDTH_20:
		case NL80211_CHAN_WIDTH_40:
			prP2pSetNewChannelMsg->eChannelWidth = CW_20_40MHZ;
			break;

		case NL80211_CHAN_WIDTH_80:
			prP2pSetNewChannelMsg->eChannelWidth = CW_80MHZ;
			break;

		case NL80211_CHAN_WIDTH_80P80:
			prP2pSetNewChannelMsg->eChannelWidth = CW_80P80MHZ;
			break;

		default:
			DBGLOG(P2P, ERROR,
				"mtk_p2p_cfg80211_channel_switch. !!!Bandwidth do not support!!!\n");
			ASSERT(FALSE);
			break;
		}

		prP2pSetNewChannelMsg->ucRoleIdx = ucRoleIdx;
		prP2pSetNewChannelMsg->ucBssIndex = ucBssIdx;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pSetNewChannelMsg,
			MSG_SEND_METHOD_BUF);

		/* Update beacon */
		if ((params->beacon_csa.head_len != 0)
			|| (params->beacon_csa.tail_len != 0)) {
			u4Len = (sizeof(struct MSG_P2P_BEACON_UPDATE)
				+ params->beacon_csa.head_len
				+ params->beacon_csa.tail_len);

			prP2pBcnUpdateMsg = (struct MSG_P2P_BEACON_UPDATE *)
				cnmMemAlloc(prGlueInfo->prAdapter,
					RAM_TYPE_MSG,
					u4Len);

			if (prP2pBcnUpdateMsg == NULL) {
				i4Rslt = -ENOMEM;
				break;
			}

			kalMemZero(prP2pBcnUpdateMsg, u4Len);

			prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
			prP2pBcnUpdateMsg->rMsgHdr.eMsgId =
				MID_MNY_P2P_BEACON_UPDATE;
			pucBuffer = prP2pBcnUpdateMsg->aucBuffer;

			if (params->beacon_csa.head_len != 0) {
				kalMemCopy(pucBuffer,
					params->beacon_csa.head,
					params->beacon_csa.head_len);

				prP2pBcnUpdateMsg->u4BcnHdrLen =
					params->beacon_csa.head_len;

				prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

				pucBuffer = (uint8_t *) ((unsigned long)
					pucBuffer
					+ (unsigned long)
					params->beacon_csa.head_len);
			} else {
				prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

				prP2pBcnUpdateMsg->pucBcnHdr = NULL;
			}

			if (params->beacon_csa.tail_len != 0) {
				prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
				kalMemCopy(pucBuffer,
					params->beacon_csa.tail,
					params->beacon_csa.tail_len);

				prP2pBcnUpdateMsg->u4BcnBodyLen =
					params->beacon_csa.tail_len;
			} else {
				prP2pBcnUpdateMsg->u4BcnBodyLen = 0;
				prP2pBcnUpdateMsg->pucBcnBody = NULL;
			}

			kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

			mboxSendMsg(prGlueInfo->prAdapter,
				MBOX_ID_0,
				(struct MSG_HDR *) prP2pBcnUpdateMsg,
				MSG_SEND_METHOD_BUF);

			i4Rslt = 0; /* Return Success */
		}

	} while (FALSE);

	return i4Rslt;
}
#endif
#endif

#if 0
struct cfg80211_beacon_data {
	const u8 *head, *tail;
	const u8 *beacon_ies;
	const u8 *proberesp_ies;
	const u8 *assocresp_ies;
	const u8 *probe_resp;

	size_t head_len, tail_len;
	size_t beacon_ies_len;
	size_t proberesp_ies_len;
	size_t assocresp_ies_len;
	size_t probe_resp_len;
};
#endif

int mtk_p2p_cfg80211_change_beacon(struct wiphy *wiphy,
		struct net_device *dev, struct cfg80211_beacon_data *info)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_BEACON_UPDATE *prP2pBcnUpdateMsg =
		(struct MSG_P2P_BEACON_UPDATE *) NULL;
	uint8_t *pucBuffer = (uint8_t *) NULL;
	uint8_t ucRoleIdx = 0;
	uint32_t u4Len = 0;

	do {
		if ((wiphy == NULL) || (info == NULL))
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_change_beacon.\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		if ((info->head_len != 0) || (info->tail_len != 0)) {
			u4Len = (sizeof(struct MSG_P2P_BEACON_UPDATE)
				+ info->head_len
				+ info->tail_len
				+ info->assocresp_ies_len
#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
				+ info->proberesp_ies_len
#endif
				);

			prP2pBcnUpdateMsg = (struct MSG_P2P_BEACON_UPDATE *)
			    cnmMemAlloc(prGlueInfo->prAdapter,
					RAM_TYPE_MSG,
					u4Len);

			if (prP2pBcnUpdateMsg == NULL) {
				i4Rslt = -ENOMEM;
				break;
			}

			kalMemZero(prP2pBcnUpdateMsg, u4Len);

			prP2pBcnUpdateMsg->ucRoleIndex = ucRoleIdx;
			prP2pBcnUpdateMsg->rMsgHdr.eMsgId =
				MID_MNY_P2P_BEACON_UPDATE;
			pucBuffer = prP2pBcnUpdateMsg->aucBuffer;

			if (info->head_len != 0) {
				kalMemCopy(pucBuffer,
					info->head,
					info->head_len);

				prP2pBcnUpdateMsg->u4BcnHdrLen = info->head_len;

				prP2pBcnUpdateMsg->pucBcnHdr = pucBuffer;

				pucBuffer += info->head_len;
			} else {
				prP2pBcnUpdateMsg->u4BcnHdrLen = 0;

				prP2pBcnUpdateMsg->pucBcnHdr = NULL;
			}

			if (info->tail_len != 0) {
				prP2pBcnUpdateMsg->pucBcnBody = pucBuffer;
				kalMemCopy(pucBuffer,
					info->tail,
					info->tail_len);

				prP2pBcnUpdateMsg->u4BcnBodyLen =
					info->tail_len;

				pucBuffer += info->tail_len;
			} else {
				prP2pBcnUpdateMsg->u4BcnBodyLen = 0;
				prP2pBcnUpdateMsg->pucBcnBody = NULL;
			}

			if (info->assocresp_ies_len != 0
				&& info->assocresp_ies != NULL) {

				prP2pBcnUpdateMsg->pucAssocRespIE = pucBuffer;
				kalMemCopy(pucBuffer,
					info->assocresp_ies,
					info->assocresp_ies_len);
				prP2pBcnUpdateMsg->u4AssocRespLen =
					info->assocresp_ies_len;
			} else {
				prP2pBcnUpdateMsg->u4AssocRespLen = 0;
				prP2pBcnUpdateMsg->pucAssocRespIE = NULL;
			}

#if CFG_SUPPORT_P2P_GO_OFFLOAD_PROBE_RSP
			if (info->proberesp_ies_len != 0
				&& info->proberesp_ies != NULL) {

				prP2pBcnUpdateMsg->pucProbeRespIE = pucBuffer;
				kalMemCopy(pucBuffer,
					info->proberesp_ies,
					info->proberesp_ies_len);
				prP2pBcnUpdateMsg->u4ProbeRespLen =
					info->proberesp_ies_len;
			} else {
				prP2pBcnUpdateMsg->u4ProbeRespLen = 0;
				prP2pBcnUpdateMsg->pucProbeRespIE = NULL;
			}
#endif

			kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);

			mboxSendMsg(prGlueInfo->prAdapter,
				MBOX_ID_0,
				(struct MSG_HDR *) prP2pBcnUpdateMsg,
				MSG_SEND_METHOD_BUF);

			i4Rslt = 0; /* Return Success */
		}

		/* TODO: Probe Rsp, Assoc Rsp, Beacon IE update. */

/* ////////////////////////// */
/**
 * struct cfg80211_beacon_data - beacon data
 * @head: head portion of beacon (before TIM IE)
 *     or %NULL if not changed
 * @tail: tail portion of beacon (after TIM IE)
 *     or %NULL if not changed
 * @head_len: length of @head
 * @tail_len: length of @tail
 * @beacon_ies: extra information element(s) to add into Beacon frames or %NULL
 * @beacon_ies_len: length of beacon_ies in octets
 * @proberesp_ies: extra information element(s) to add into Probe Response
 *      frames or %NULL
 * @proberesp_ies_len: length of proberesp_ies in octets
 * @assocresp_ies: extra information element(s) to add into (Re)Association
 *      Response frames or %NULL
 * @assocresp_ies_len: length of assocresp_ies in octets
 * @probe_resp_len: length of probe response template (@probe_resp)
 * @probe_resp: probe response template (AP mode only)
 */
/* struct cfg80211_beacon_data { */
/* const u8 *head, *tail; */
/* const u8 *beacon_ies; */
/* const u8 *proberesp_ies; */
/* const u8 *assocresp_ies; */
/* const u8 *probe_resp; */

/* size_t head_len, tail_len; */
/* size_t beacon_ies_len; */
/* size_t proberesp_ies_len; */
/* size_t assocresp_ies_len; */
/* size_t probe_resp_len; */
/* }; */

/* ////////////////////////// */

	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_change_beacon */

int mtk_p2p_cfg80211_stop_ap(struct wiphy *wiphy, struct net_device *dev)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_STOP_AP *prP2pStopApMsg =
			(struct MSG_P2P_STOP_AP *) NULL;
	uint8_t ucRoleIdx = 0;
	struct GL_P2P_INFO *prP2PInfo;
	uint32_t waitRet = 0;

	do {
		if (wiphy == NULL)
			break;

		DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_stop_ap.\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

#if (CFG_SUPPORT_DFS_MASTER == 1)
		if (dev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP) {
			netif_carrier_off(dev);
			netif_tx_stop_all_queues(dev);
		}
#endif

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		prP2pStopApMsg = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_P2P_STOP_AP));

		if (prP2pStopApMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
#if KERNEL_VERSION(3, 13, 0) <= CFG80211_VERSION_CODE
		reinit_completion(&prP2PInfo->rStopApComp);
#else
		prP2PInfo->rStopApComp.done = 0;
#endif

		prP2pStopApMsg->rMsgHdr.eMsgId = MID_MNY_P2P_STOP_AP;
		prP2pStopApMsg->ucRoleIdx = ucRoleIdx;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prP2pStopApMsg,
			MSG_SEND_METHOD_BUF);

		waitRet = wait_for_completion_timeout(
			&prP2PInfo->rStopApComp,
			MSEC_TO_JIFFIES(P2P_DEAUTH_TIMEOUT_TIME_MS));
		if (!waitRet)
			DBGLOG(P2P, WARN, "timeout\n");
		else
			DBGLOG(P2P, INFO, "complete\n");

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_stop_ap */

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
int mtk_p2p_cfg80211_auth(struct wiphy *wiphy,
	struct net_device *ndev, struct cfg80211_auth_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct P2P_CONNECTION_SETTINGS *prP2pConnSettings = NULL;
	struct cfg80211_connect_params connect;
	struct cfg80211_connect_params *sme = &connect;
	const struct cfg80211_bss_ies *ies;
	const uint8_t *ssidie = NULL;
	uint8_t ssid_len = 0;
	uint8_t ucRoleIdx = 0;

	DBGLOG(REQ, INFO,
		"auth to  BSS [" MACSTR "]\n",
		MAC2STR((uint8_t *)req->bss->bssid));
	DBGLOG(REQ, INFO, "auth_type:%d\n", req->auth_type);

	prGlueInfo = (struct GLUE_INFO *) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	memset(&connect, 0, sizeof(connect));
	sme->bssid = req->bss->bssid;
	if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev,
			&ucRoleIdx) < 0)
		return -EINVAL;

	DBGLOG(REQ, INFO, "ucRoleIndex = %d\n", ucRoleIdx);

	prP2pConnSettings = prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[ucRoleIdx];

	ies = rcu_access_pointer(req->bss->ies);
	if (!ies)
		return false;

	ssidie = cfg80211_find_ie(WLAN_EID_SSID, ies->data, ies->len);
	if (!ssidie)
		return false;

	ssid_len = ssidie[1];
	sme->ssid = ssidie + 2;
	sme->ssid_len = ssid_len;
	COPY_SSID(prP2pConnSettings->aucSSID, prP2pConnSettings->ucSSIDLen,
		sme->ssid, sme->ssid_len);
	prP2pConnSettings->fgIsSendAssoc = FALSE;

	DBGLOG(REQ, INFO, "ssid_len %lu ssid %s\n", sme->ssid_len, sme->ssid);

	return mtk_p2p_cfg80211_connect(wiphy, ndev, sme);
}

int mtk_p2p_cfg80211_assoc(struct wiphy *wiphy,
	       struct net_device *ndev, struct cfg80211_assoc_request *req)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucRoleIdx = 0;
	struct P2P_CONNECTION_SETTINGS *prP2pConnSettings = NULL;
	struct STA_RECORD *prStaRec = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
				(struct P2P_ROLE_FSM_INFO *) NULL;
	struct P2P_CONNECTION_REQ_INFO *prConnReqInfo =
				(struct P2P_CONNECTION_REQ_INFO *) NULL;

	prGlueInfo = (struct GLUE_INFO *) wiphy_priv(wiphy);
	ASSERT(prGlueInfo);

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev,
			&ucRoleIdx) < 0)
		return -EINVAL;

	prP2pConnSettings = prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[ucRoleIdx];

	/* [todo]temp use for indicate rx assoc resp, may need to be modified */
	/* The BSS from cfg80211_ops.assoc must give back to
	* cfg80211_send_rx_assoc() or to cfg80211_assoc_timeout().
	* To ensure proper refcounting,
	* new association requests while already associating
	* must be rejected.
	*/
	if (prP2pConnSettings->bss)
		return -ENOENT;
	prP2pConnSettings->bss = req->bss;

	DBGLOG(REQ, INFO, "ucRoleIndex = %d\n", ucRoleIdx);

	/*[TODO]may to check if assoc parameters change as cfg80211_auth*/
	prP2pConnSettings->fgIsSendAssoc = TRUE;
	/* skip join initial flow when it has been completed*/
	prP2pRoleFsmInfo = P2P_ROLE_INDEX_2_ROLE_FSM_INFO(
						prGlueInfo->prAdapter,
						ucRoleIdx);
	prStaRec = prP2pRoleFsmInfo->rJoinInfo.prTargetStaRec;
	prConnReqInfo = &(prP2pRoleFsmInfo->rConnReqInfo);
	kalMemCopy(prConnReqInfo->aucIEBuf,
						req->ie, req->ie_len);
	prConnReqInfo->u4BufLength = req->ie_len;

	/* set crypto */
	kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_NONE,
						ucRoleIdx);
	DBGLOG(REQ, INFO,
					"n_ciphers_pairwise %d, ciphers_pairwise[0] %#x\n",
					req->crypto.n_ciphers_pairwise,
					req->crypto.ciphers_pairwise[0]);

	if (req->crypto.n_ciphers_pairwise) {
		switch (req->crypto.ciphers_pairwise[0]) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			kalP2PSetCipher(prGlueInfo,
						IW_AUTH_CIPHER_WEP40,
						ucRoleIdx);
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			kalP2PSetCipher(prGlueInfo,
						IW_AUTH_CIPHER_TKIP,
						ucRoleIdx);
			break;
		case WLAN_CIPHER_SUITE_CCMP:
		case WLAN_CIPHER_SUITE_AES_CMAC:
			kalP2PSetCipher(prGlueInfo,
						IW_AUTH_CIPHER_CCMP,
						ucRoleIdx);
			break;
		default:
			DBGLOG(REQ, WARN,
						"invalid cipher pairwise (%d)\n",
						req->crypto
						.ciphers_pairwise[0]);
					/* do cfg80211_put_bss before return */
					return -EINVAL;
		}
	}
	/* end	*/

	if (prStaRec)
		saaSendAuthAssoc(prGlueInfo->prAdapter, prStaRec);
	else
		DBGLOG(REQ, WARN,
					"can't send auth since can't find StaRec\n");

	return 0;
}
#endif

/* TODO: */
int mtk_p2p_cfg80211_deauth(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_deauth_request *req)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	int32_t i4Rslt = 0;
#endif

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	/* not implemented yet */
	DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_deauth.\n");

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
	i4Rslt = mtk_p2p_cfg80211_disconnect(wiphy, dev,
						   req->reason_code);
	return i4Rslt;
#else
	return -EINVAL;
#endif
}				/* mtk_p2p_cfg80211_deauth */

/* TODO: */
int mtk_p2p_cfg80211_disassoc(struct wiphy *wiphy,
		struct net_device *dev,
		struct cfg80211_disassoc_request *req)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_disassoc.\n");

	/* not implemented yet */

	return -EINVAL;
}				/* mtk_p2p_cfg80211_disassoc */

int mtk_p2p_cfg80211_remain_on_channel(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		struct ieee80211_channel *chan,
		unsigned int duration, u64 *cookie)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct GL_P2P_DEV_INFO *prGlueP2pDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;
	struct MSG_P2P_CHNL_REQUEST *prMsgChnlReq =
		(struct MSG_P2P_CHNL_REQUEST *) NULL;

	DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_remain_on_channel\n");

	do {
		if ((wiphy == NULL) ||
		    /* (dev == NULL) || */
		    (chan == NULL) || (cookie == NULL)) {
			break;
		}

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		prGlueP2pDevInfo = prGlueInfo->prP2PDevInfo;

		*cookie = prGlueP2pDevInfo->u8Cookie++;

		prMsgChnlReq = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_P2P_CHNL_REQUEST));

		if (prMsgChnlReq == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		DBGLOG(P2P, INFO,
			"Remain on channel, cookie: 0x%llx\n",
			*cookie);

		prMsgChnlReq->rMsgHdr.eMsgId = MID_MNY_P2P_CHNL_REQ;
		prMsgChnlReq->u8Cookie = *cookie;
		prMsgChnlReq->u4Duration = duration;
		prMsgChnlReq->eChnlReqType = CH_REQ_TYPE_ROC;

		kalChannelFormatSwitch(NULL, chan,
			&prMsgChnlReq->rChannelInfo);
		kalChannelScoSwitch(NL80211_CHAN_NO_HT,
			&prMsgChnlReq->eChnlSco);

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChnlReq,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}

/* mtk_p2p_cfg80211_remain_on_channel */

int mtk_p2p_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy,
		struct wireless_dev *wdev, u64 cookie)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct GL_P2P_INFO *prGlueP2pInfo = (struct GL_P2P_INFO *) NULL;
	struct MSG_P2P_CHNL_ABORT *prMsgChnlAbort =
		(struct MSG_P2P_CHNL_ABORT *) NULL;

	do {
		if (wiphy == NULL /* || (dev == NULL) */)
			break;

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		prGlueP2pInfo = prGlueInfo->prP2PInfo[0];

		prMsgChnlAbort = cnmMemAlloc(prGlueInfo->prAdapter,
			RAM_TYPE_MSG, sizeof(struct MSG_P2P_CHNL_ABORT));

		if (prMsgChnlAbort == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		DBGLOG(P2P, INFO,
			"Cancel remain on channel, cookie: 0x%llx\n", cookie);

		prMsgChnlAbort->rMsgHdr.eMsgId = MID_MNY_P2P_CHNL_ABORT;
		prMsgChnlAbort->u8Cookie = cookie;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgChnlAbort,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_cancel_remain_on_channel */

int _mtk_p2p_cfg80211_mgmt_tx(struct wiphy *wiphy,
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
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	struct net_device *dev = NULL;

	do {
		if ((wiphy == NULL) || (wdev == NULL) || (cookie == NULL))
			break;

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		dev = wdev->netdev;

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0) {
			/* Device Interface. */
			ucBssIdx = prGlueInfo->prAdapter->ucP2PDevBssIdx;
		} else {
			ASSERT(ucRoleIdx < KAL_P2P_NUM);
			/* Role Interface. */
			if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
				ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS) {
				/* Can't find BSS index. */
				break;
			}
		}

		*cookie = prGlueInfo->prP2PDevInfo->u8Cookie++;

		prMsgTxReq = cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_MGMT_TX_REQUEST));

		if (prMsgTxReq == NULL) {
			i4Rslt = -ENOMEM;
			DBGLOG(P2P, ERROR, "Allocate TX req msg fails.\n");
			break;
		}

		if (offchan) {
			prMsgTxReq->fgIsOffChannel = TRUE;
			memset(&prMsgTxReq->rChannelInfo, 0,
					sizeof(struct RF_CHANNEL_INFO));
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
			DBGLOG(P2P, ERROR, "Allocate TX packet fails.\n");
			break;
		}

		prMsgTxReq->u8Cookie = *cookie;
		prMsgTxReq->rMsgHdr.eMsgId = MID_MNY_P2P_MGMT_TX;
		prMsgTxReq->ucBssIdx = ucBssIdx;

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

#define TEMP_LOG_TEMPLATE "netdev: %p, bssIdx: %d, band: %d, chan: %d, " \
		"offchan: %d, wait: %d, len: %d, no_cck: %d, " \
		"dont_wait_for_ack: %d, cookie: 0x%llx\n"
		DBGLOG(P2P, INFO, TEMP_LOG_TEMPLATE,
				dev,
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

#if KERNEL_VERSION(3, 14, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_mgmt_tx(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		struct cfg80211_mgmt_tx_params *params,
		u64 *cookie)
{
	if (params == NULL)
		return -EINVAL;

	return _mtk_p2p_cfg80211_mgmt_tx(wiphy, wdev, params->chan,
			params->offchan, params->wait, params->buf,
			params->len, params->no_cck, params->dont_wait_for_ack,
			cookie);
}				/* mtk_p2p_cfg80211_mgmt_tx */
#else
int mtk_p2p_cfg80211_mgmt_tx(struct wiphy *wiphy,
		struct wireless_dev *wdev,
		struct ieee80211_channel *chan, bool offchan,
		unsigned int wait, const u8 *buf, size_t len,
		bool no_cck, bool dont_wait_for_ack, u64 *cookie)
{
	return _mtk_p2p_cfg80211_mgmt_tx(wiphy, wdev, chan, offchan, wait, buf,
			len, no_cck, dont_wait_for_ack, cookie);
}				/* mtk_p2p_cfg80211_mgmt_tx */
#endif

int mtk_p2p_cfg80211_mgmt_tx_cancel_wait(struct wiphy *wiphy,
		struct wireless_dev *wdev, u64 cookie)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	uint8_t ucRoleIdx = 0, ucBssIdx = 0;
	struct MSG_CANCEL_TX_WAIT_REQUEST *prMsgCancelTxWait =
			(struct MSG_CANCEL_TX_WAIT_REQUEST *) NULL;

	do {
		ASSERT(wiphy);

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo,
			wdev->netdev, &ucRoleIdx) < 0) {
			/* Device Interface. */
			ucBssIdx = prGlueInfo->prAdapter->ucP2PDevBssIdx;
		} else {
			ASSERT(ucRoleIdx < KAL_P2P_NUM);
			/* Role Interface. */
			if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
				ucRoleIdx,
				&ucBssIdx) != WLAN_STATUS_SUCCESS) {
				/* Can't find BSS index. */
				break;
			}
		}

		DBGLOG(P2P, INFO, "bssIdx: %d, cookie: 0x%llx\n",
				ucBssIdx,
				cookie);

		prMsgCancelTxWait = cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_CANCEL_TX_WAIT_REQUEST));

		if (prMsgCancelTxWait == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prMsgCancelTxWait->rMsgHdr.eMsgId =
				MID_MNY_P2P_MGMT_TX_CANCEL_WAIT;
		prMsgCancelTxWait->u8Cookie = cookie;
		prMsgCancelTxWait->ucBssIdx = ucBssIdx;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMsgCancelTxWait,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_mgmt_tx_cancel_wait */

int mtk_p2p_cfg80211_change_bss(struct wiphy *wiphy,
		struct net_device *dev,
		struct bss_parameters *params)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt;

	ASSERT(wiphy);

	DBGLOG(P2P, INFO, "%s\n", __func__);
	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	switch (params->use_cts_prot) {
	case -1:
		DBGLOG(P2P, TRACE, "CTS protection no change\n");
		break;
	case 0:
		DBGLOG(P2P, TRACE, "CTS protection disable.\n");
		break;
	case 1:
		DBGLOG(P2P, TRACE, "CTS protection enable\n");
		break;
	default:
		DBGLOG(P2P, TRACE, "CTS protection unknown\n");
		break;
	}

	switch (params->use_short_preamble) {
	case -1:
		DBGLOG(P2P, TRACE, "Short prreamble no change\n");
		break;
	case 0:
		DBGLOG(P2P, TRACE, "Short prreamble disable.\n");
		break;
	case 1:
		DBGLOG(P2P, TRACE, "Short prreamble enable\n");
		break;
	default:
		DBGLOG(P2P, TRACE, "Short prreamble unknown\n");
		break;
	}

#if 0
	/* not implemented yet */
	p2pFuncChangeBssParam(prGlueInfo->prAdapter,
			      prBssInfo->fgIsProtection,
			      prBssInfo->fgIsShortPreambleAllowed,
			      prBssInfo->fgUseShortSlotTime,
			      /* Basic rates */
			      /* basic rates len */
			      /* ap isolate */
			      /* ht opmode. */
	    );
#else
	i4Rslt = 0;
#endif

	return i4Rslt;
}				/* mtk_p2p_cfg80211_change_bss */

#if KERNEL_VERSION(3, 16, 0) <= CFG80211_VERSION_CODE
#if KERNEL_VERSION(3, 19, 0) <= CFG80211_VERSION_CODE
static const u8 bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int mtk_p2p_cfg80211_del_station(struct wiphy *wiphy,
		struct net_device *dev,
		struct station_del_parameters *params)
{
	const u8 *mac = params->mac ? params->mac : bcast_addr;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnectMsg =
		(struct MSG_P2P_CONNECTION_ABORT *) NULL;
	uint8_t aucBcMac[] = BC_MAC_ADDR;
	uint8_t ucRoleIdx = 0;

	do {
		if ((wiphy == NULL) || (dev == NULL))
			break;

		if (mac == NULL)
			mac = aucBcMac;

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_del_station " MACSTR ". reason: %d\n",
			MAC2STR(mac), params->reason_code);

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;
		/* prDisconnectMsg = (struct MSG_P2P_CONNECTION_ABORT *)
		 * kalMemAlloc(sizeof(struct MSG_P2P_CONNECTION_ABORT),
		 * VIR_MEM_TYPE);
		 */

		prDisconnectMsg = (struct MSG_P2P_CONNECTION_ABORT *)
		    cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_CONNECTION_ABORT));

		if (prDisconnectMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prDisconnectMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
		prDisconnectMsg->ucRoleIdx = ucRoleIdx;
		COPY_MAC_ADDR(prDisconnectMsg->aucTargetID, mac);
		prDisconnectMsg->u2ReasonCode = params->reason_code;
		prDisconnectMsg->fgSendDeauth = TRUE;


		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prDisconnectMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;

}				/* mtk_p2p_cfg80211_del_station */
#else
int mtk_p2p_cfg80211_del_station(struct wiphy *wiphy,
		struct net_device *dev, const u8 *mac)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnectMsg =
		(struct MSG_P2P_CONNECTION_ABORT *) NULL;
	uint8_t aucBcMac[] = BC_MAC_ADDR;
	uint8_t ucRoleIdx = 0;

	do {
		if ((wiphy == NULL) || (dev == NULL))
			break;

		if (mac == NULL)
			mac = aucBcMac;

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_del_station " MACSTR ".\n",
			MAC2STR(mac));

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;
		/* prDisconnectMsg = (struct MSG_P2P_CONNECTION_ABORT *)
		 * kalMemAlloc(sizeof(struct MSG_P2P_CONNECTION_ABORT),
		 * VIR_MEM_TYPE);
		 */

		prDisconnectMsg = (struct MSG_P2P_CONNECTION_ABORT *)
			cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_CONNECTION_ABORT));

		if (prDisconnectMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prDisconnectMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
		prDisconnectMsg->ucRoleIdx = ucRoleIdx;
		COPY_MAC_ADDR(prDisconnectMsg->aucTargetID, mac);
		prDisconnectMsg->u2ReasonCode = REASON_CODE_UNSPECIFIED;
		prDisconnectMsg->fgSendDeauth = TRUE;


		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prDisconnectMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;

}				/* mtk_p2p_cfg80211_del_station */
#endif
#else
int mtk_p2p_cfg80211_del_station(struct wiphy *wiphy,
		struct net_device *dev, u8 *mac)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnectMsg =
		(struct MSG_P2P_CONNECTION_ABORT *) NULL;
	uint8_t aucBcMac[] = BC_MAC_ADDR;
	uint8_t ucRoleIdx = 0;

	do {
		if ((wiphy == NULL) || (dev == NULL))
			break;

		if (mac == NULL)
			mac = aucBcMac;

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_del_station " MACSTR ".\n",
			MAC2STR(mac));

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;
		/* prDisconnectMsg = (struct MSG_P2P_CONNECTION_ABORT *)
		 * kalMemAlloc(sizeof(struct MSG_P2P_CONNECTION_ABORT),
		 * VIR_MEM_TYPE);
		 */

		prDisconnectMsg =
		    (struct MSG_P2P_CONNECTION_ABORT *)
		    cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_CONNECTION_ABORT));

		if (prDisconnectMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prDisconnectMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
		prDisconnectMsg->ucRoleIdx = ucRoleIdx;
		COPY_MAC_ADDR(prDisconnectMsg->aucTargetID, mac);
		prDisconnectMsg->u2ReasonCode = REASON_CODE_UNSPECIFIED;
		prDisconnectMsg->fgSendDeauth = TRUE;


		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prDisconnectMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;

}				/* mtk_p2p_cfg80211_del_station */
#endif

int mtk_p2p_cfg80211_connect(struct wiphy *wiphy,
		struct net_device *dev, struct cfg80211_connect_params *sme)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MSG_P2P_CONNECTION_REQUEST *prConnReqMsg =
		(struct MSG_P2P_CONNECTION_REQUEST *) NULL;
	uint8_t ucRoleIdx = 0;
	const u8 *bssid = NULL;
	struct ieee80211_channel *channel = NULL;
	struct cfg80211_bss *bss = NULL;

	do {
		if ((wiphy == NULL) || (dev == NULL) || (sme == NULL))
			break;

		if (sme->bssid)
			bssid = sme->bssid;
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
		else if (sme->bssid_hint)
			bssid = sme->bssid_hint;
#endif
		if (sme->channel)
			channel = sme->channel;
#if KERNEL_VERSION(3, 15, 0) <= CFG80211_VERSION_CODE
		else if (sme->channel_hint)
			channel = sme->channel_hint;
#endif

		if ((bssid == NULL) || (channel == NULL)) {
#if KERNEL_VERSION(4, 1, 0) <= CFG80211_VERSION_CODE
			bss = cfg80211_get_bss(wiphy, NULL, NULL,
				sme->ssid, sme->ssid_len,
				IEEE80211_BSS_TYPE_ESS, IEEE80211_PRIVACY_ANY);
#else
			bss = cfg80211_get_bss(wiphy, NULL, NULL,
				sme->ssid, sme->ssid_len,
				WLAN_CAPABILITY_ESS, WLAN_CAPABILITY_ESS);
#endif
			if (bss == NULL) {
				DBGLOG(P2P, ERROR,
				"Reject connect without bssid/channel/bss.\n");
				break;
			}

			bssid = bss->bssid;
			channel = bss->channel;

			if ((bssid == NULL) || (channel == NULL)) {
				DBGLOG(P2P, ERROR,
				"Reject connect: no bssid/channel in bss.\n");
				break;
			}
		}

		DBGLOG(P2P, INFO,
			"bssid: " MACSTR ", band: %d, freq: %d.\n",
			MAC2STR(bssid), channel->band, channel->center_freq);

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		prConnReqMsg =
		    (struct MSG_P2P_CONNECTION_REQUEST *)
		    cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				(sizeof(struct MSG_P2P_CONNECTION_REQUEST)
				+ sme->ie_len));

		if (prConnReqMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prConnReqMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_REQ;
		prConnReqMsg->ucRoleIdx = ucRoleIdx;

		COPY_SSID(prConnReqMsg->rSsid.aucSsid,
			prConnReqMsg->rSsid.ucSsidLen,
			sme->ssid, sme->ssid_len);

		COPY_MAC_ADDR(prConnReqMsg->aucBssid, bssid);
		COPY_MAC_ADDR(prConnReqMsg->aucSrcMacAddr, dev->dev_addr);

		DBGLOG(P2P, TRACE,
			"Assoc Req IE Buffer Length:%zu\n", sme->ie_len);

		kalMemCopy(prConnReqMsg->aucIEBuf, sme->ie, sme->ie_len);
		prConnReqMsg->u4IELen = sme->ie_len;

		kalP2PSetCipher(prGlueInfo, IW_AUTH_CIPHER_NONE, ucRoleIdx);

		if (sme->crypto.n_ciphers_pairwise) {
			switch (sme->crypto.ciphers_pairwise[0]) {
			case WLAN_CIPHER_SUITE_WEP40:
			case WLAN_CIPHER_SUITE_WEP104:
				kalP2PSetCipher(prGlueInfo,
					IW_AUTH_CIPHER_WEP40, ucRoleIdx);
				break;
			case WLAN_CIPHER_SUITE_TKIP:
				kalP2PSetCipher(prGlueInfo,
					IW_AUTH_CIPHER_TKIP, ucRoleIdx);
				break;
			case WLAN_CIPHER_SUITE_CCMP:
			case WLAN_CIPHER_SUITE_AES_CMAC:
				kalP2PSetCipher(prGlueInfo,
					IW_AUTH_CIPHER_CCMP, ucRoleIdx);
				break;
			default:
				cnmMemFree(prGlueInfo->prAdapter, prConnReqMsg);
				DBGLOG(REQ, WARN,
					"invalid cipher pairwise (%d)\n",
					sme->crypto.ciphers_pairwise[0]);
				/* do cfg80211_put_bss before return */
				if (bss)
					cfg80211_put_bss(wiphy, bss);
				return -EINVAL;
			}
		}

		kalChannelFormatSwitch(NULL, channel,
				&prConnReqMsg->rChannelInfo);
		kalChannelScoSwitch(NL80211_CHAN_NO_HT,
				&prConnReqMsg->eChnlSco);

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prConnReqMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	/* do cfg80211_put_bss before return */
	if (bss)
		cfg80211_put_bss(wiphy, bss);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_connect */

int mtk_p2p_cfg80211_disconnect(struct wiphy *wiphy,
		struct net_device *dev, u16 reason_code)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct MSG_P2P_CONNECTION_ABORT *prDisconnMsg =
		(struct MSG_P2P_CONNECTION_ABORT *) NULL;
	uint8_t aucBCAddr[] = BC_MAC_ADDR;
	uint8_t ucRoleIdx = 0;

	do {
		if ((wiphy == NULL) || (dev == NULL))
			break;

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_disconnect reason: %d.\n",
			reason_code);

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

#if (CFG_SUPPORT_SUPPLICANT_SME == 1)
		if (prGlueInfo->prAdapter->rWifiVar.
			prP2PConnSettings[ucRoleIdx]->bss) {
			cfg80211_assoc_timeout(dev,
				prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings[ucRoleIdx]->bss);
			DBGLOG(P2P, EVENT, "assoc timeout notify[%d]\n", ucRoleIdx);
			prGlueInfo->prAdapter->rWifiVar.
				prP2PConnSettings[ucRoleIdx]->bss = NULL;
		}
#endif
/* prDisconnMsg = (P_MSG_P2P_CONNECTION_ABORT_T)
 * MemAlloc(sizeof(P_MSG_P2P_CONNECTION_ABORT_T), VIR_MEM_TYPE);
 */
		prDisconnMsg =
		    (struct MSG_P2P_CONNECTION_ABORT *)
		    cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_CONNECTION_ABORT));

		if (prDisconnMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prDisconnMsg->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_ABORT;
		prDisconnMsg->ucRoleIdx = ucRoleIdx;
		prDisconnMsg->u2ReasonCode = reason_code;
		prDisconnMsg->fgSendDeauth = TRUE;
		COPY_MAC_ADDR(prDisconnMsg->aucTargetID, aucBCAddr);

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prDisconnMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_disconnect */

int
mtk_p2p_cfg80211_change_iface(IN struct wiphy *wiphy,
		IN struct net_device *ndev,
		IN enum nl80211_iftype type,
		IN u32 *flags,
		IN struct vif_params *params)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	int32_t i4Rslt = -EINVAL;
	struct MSG_P2P_SWITCH_OP_MODE *prSwitchModeMsg =
		(struct MSG_P2P_SWITCH_OP_MODE *) NULL;
	uint8_t ucRoleIdx = 0;

	do {
		if ((wiphy == NULL) || (ndev == NULL)) {
			DBGLOG(P2P, ERROR, "wiphy=%p, ndev=%p.\n", wiphy, ndev);
			break;
		}

		DBGLOG(P2P, INFO,
			"mtk_p2p_cfg80211_change_iface, type: %d\n", type);

		if (ndev->ieee80211_ptr)
			ndev->ieee80211_ptr->iftype = type;

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, ndev, &ucRoleIdx) != 0) {
			DBGLOG(P2P, TRACE,
				"Device Interface no need to change interface type.\n");
			return 0;
		}
		/* Switch OP MOde. */
		prSwitchModeMsg =
		    (struct MSG_P2P_SWITCH_OP_MODE *)
		    cnmMemAlloc(prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_SWITCH_OP_MODE));

		if (prSwitchModeMsg == NULL) {
			i4Rslt = -ENOMEM;
			break;
		}

		prSwitchModeMsg->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
		prSwitchModeMsg->ucRoleIdx = ucRoleIdx;

		switch (type) {
		case NL80211_IFTYPE_P2P_CLIENT:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_P2P_CLIENT.\n");
			prSwitchModeMsg->eIftype = IFTYPE_P2P_CLIENT;
			/* This case need to fall through */
			kal_fallthrough;
		case NL80211_IFTYPE_STATION:
			if (type == NL80211_IFTYPE_STATION) {
				DBGLOG(P2P, TRACE, "NL80211_IFTYPE_STATION.\n");
				prSwitchModeMsg->eIftype = IFTYPE_STATION;
			}
			prSwitchModeMsg->eOpMode = OP_MODE_INFRASTRUCTURE;
			kalP2PSetRole(prGlueInfo, 1, ucRoleIdx);
			break;
		case NL80211_IFTYPE_AP:
			DBGLOG(P2P, TRACE, "NL80211_IFTYPE_AP.\n");
			kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);
			prSwitchModeMsg->eIftype = IFTYPE_AP;
			/* This case need to fall through */
			kal_fallthrough;
		case NL80211_IFTYPE_P2P_GO:
			if (type == NL80211_IFTYPE_P2P_GO) {
				DBGLOG(P2P, TRACE,
					"NL80211_IFTYPE_P2P_GO not AP.\n");
				prSwitchModeMsg->eIftype = IFTYPE_P2P_GO;
			}
			prSwitchModeMsg->eOpMode = OP_MODE_ACCESS_POINT;
			kalP2PSetRole(prGlueInfo, 2, ucRoleIdx);
			break;
		default:
			DBGLOG(P2P, TRACE, "Other type :%d .\n", type);
			prSwitchModeMsg->eOpMode = OP_MODE_P2P_DEVICE;
			kalP2PSetRole(prGlueInfo, 0, ucRoleIdx);
			prSwitchModeMsg->eIftype = IFTYPE_P2P_DEVICE;
			break;
		}

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prSwitchModeMsg,
			MSG_SEND_METHOD_BUF);

		i4Rslt = 0;

	} while (FALSE);

	return i4Rslt;

}				/* mtk_p2p_cfg80211_change_iface */

int mtk_p2p_cfg80211_set_channel(IN struct wiphy *wiphy,
		struct cfg80211_chan_def *chandef)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	struct RF_CHANNEL_INFO rRfChnlInfo;
	uint8_t ucRoleIdx = 0;
	struct net_device *dev = NULL;

	kalMemZero(&rRfChnlInfo, sizeof(struct RF_CHANNEL_INFO));

	if ((wiphy == NULL) || (chandef == NULL))
		return i4Rslt;

	dev = (struct net_device *) wiphy_dev(wiphy);

	do {
		DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_set_channel.\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		kalChannelFormatSwitch(chandef, chandef->chan, &rRfChnlInfo);

		if (mtk_Netdev_To_RoleIdx(prGlueInfo, dev, &ucRoleIdx) < 0)
			break;

		p2pFuncSetChannel(prGlueInfo->prAdapter,
			ucRoleIdx, &rRfChnlInfo);

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;

}

/* mtk_p2p_cfg80211_set_channel */

int
mtk_p2p_cfg80211_set_bitrate_mask(IN struct wiphy *wiphy,
		IN struct net_device *dev,
		IN const u8 *peer,
		IN const struct cfg80211_bitrate_mask *mask)
{
	int32_t i4Rslt = -EINVAL;
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;

	do {
		if ((wiphy == NULL) || (dev == NULL) || (mask == NULL))
			break;

		DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_set_bitrate_mask\n");

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		/* TODO: Set bitrate mask of the peer? */

		i4Rslt = 0;
	} while (FALSE);

	return i4Rslt;
}				/* mtk_p2p_cfg80211_set_bitrate_mask */

void mtk_p2p_cfg80211_mgmt_frame_register(IN struct wiphy *wiphy,
		struct wireless_dev *wdev,
		IN u16 frame_type, IN bool reg)
{
#if 0
	struct MSG_P2P_MGMT_FRAME_REGISTER *prMgmtFrameRegister =
		(struct MSG_P2P_MGMT_FRAME_REGISTER *) NULL;
#endif
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *) NULL;
	uint8_t ucRoleIdx = 0;
	uint32_t *pu4P2pPacketFilter = NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;

	do {
		if ((wiphy == NULL) || (wdev == NULL))
			break;

		DBGLOG(P2P, TRACE, "netdev: 0x%p, frame_type: 0x%x, reg: %d\n",
				wdev->netdev, frame_type, reg);

		P2P_WIPHY_PRIV(wiphy, prGlueInfo);

		/* since p2p device share the aprRoleHandler
		 * so needs to check DevHandler 1st
		 */
		if (prGlueInfo->prP2PInfo[0]->prDevHandler == wdev->netdev) {
			/* P2P device*/
			pu4P2pPacketFilter =
				&prGlueInfo->prP2PDevInfo->u4OsMgmtFrameFilter;
		} else {
			if (mtk_Netdev_To_RoleIdx(prGlueInfo,
				wdev->netdev, &ucRoleIdx) < 0) {
				DBGLOG(P2P, WARN, "wireless dev match fail!\n");
				break;
			} else {
				/* Non P2P device*/
				ASSERT(ucRoleIdx < KAL_P2P_NUM);
				DBGLOG(P2P, TRACE,
					"Open packet filer RoleIdx %u\n",
					ucRoleIdx);
				prP2pRoleFsmInfo =
					prGlueInfo->prAdapter
					->rWifiVar.aprP2pRoleFsmInfo[ucRoleIdx];
				pu4P2pPacketFilter =
					&prP2pRoleFsmInfo->u4P2pPacketFilter;
			}
		}
		switch (frame_type) {
		case MAC_FRAME_PROBE_REQ:
			if (reg) {
				*pu4P2pPacketFilter
					|= PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(P2P, TRACE,
					"Open packet filer probe request\n");
			} else {
				*pu4P2pPacketFilter
					&= ~PARAM_PACKET_FILTER_PROBE_REQ;
				DBGLOG(P2P, TRACE,
					"Close packet filer probe request\n");
			}
			break;
		case MAC_FRAME_ACTION:
			if (reg) {
				*pu4P2pPacketFilter
					|= PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(P2P, TRACE,
					"Open packet filer action frame.\n");
			} else {
				*pu4P2pPacketFilter
					&= ~PARAM_PACKET_FILTER_ACTION_FRAME;
				DBGLOG(P2P, TRACE,
					"Close packet filer action frame.\n");
			}
			break;
#if CFG_SUPPORT_SOFTAP_WPA3
		case MAC_FRAME_AUTH:
			if (reg) {
				*pu4P2pPacketFilter
					|= PARAM_PACKET_FILTER_AUTH;
				DBGLOG(P2P, TRACE,
					"Open packet filer auth request\n");
			} else {
				*pu4P2pPacketFilter
					&= ~PARAM_PACKET_FILTER_AUTH;
				DBGLOG(P2P, TRACE,
					"Close packet filer auth request\n");
			}
			break;
		case MAC_FRAME_ASSOC_REQ:
			if (reg) {
				*pu4P2pPacketFilter
					|= PARAM_PACKET_FILTER_ASSOC_REQ;
				DBGLOG(P2P, TRACE,
					"Open packet filer assoc request\n");
			} else {
				*pu4P2pPacketFilter
					&= ~PARAM_PACKET_FILTER_ASSOC_REQ;
				DBGLOG(P2P, TRACE,
					"Close packet filer assoc request\n");
			}
			break;
#endif
		default:
			DBGLOG(P2P, ERROR,
				"Ask frog to add code for mgmt:%x\n",
				frame_type);
			break;
		}

		set_bit(GLUE_FLAG_FRAME_FILTER_BIT, &prGlueInfo->ulFlag);

		/* wake up main thread */
		wake_up_interruptible(&prGlueInfo->waitq);

		if (in_interrupt())
			DBGLOG(P2P, TRACE, "It is in interrupt level\n");
#if 0

		prMgmtFrameRegister =
		    (struct MSG_P2P_MGMT_FRAME_REGISTER *)
		    cnmMemAlloc(prGlueInfo->prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_MGMT_FRAME_REGISTER));

		if (prMgmtFrameRegister == NULL) {
			ASSERT(FALSE);
			break;
		}

		prMgmtFrameRegister->rMsgHdr.eMsgId =
			MID_MNY_P2P_MGMT_FRAME_REGISTER;

		prMgmtFrameRegister->u2FrameType = frame_type;
		prMgmtFrameRegister->fgIsRegister = reg;

		mboxSendMsg(prGlueInfo->prAdapter,
			MBOX_ID_0,
			(struct MSG_HDR *) prMgmtFrameRegister,
			MSG_SEND_METHOD_BUF);

#endif

	} while (FALSE);

}				/* mtk_p2p_cfg80211_mgmt_frame_register */

#ifdef CONFIG_NL80211_TESTMODE

#if KERNEL_VERSION(3, 12, 0) <= CFG80211_VERSION_CODE
int mtk_p2p_cfg80211_testmode_cmd(struct wiphy *wiphy,
		struct wireless_dev *wdev, void *data,
		int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_TEST_PARAMS *prParams =
		(struct NL80211_DRIVER_TEST_PARAMS *) NULL;
	int32_t i4Status = -EINVAL;

	ASSERT(wiphy);
	ASSERT(wdev);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_testmode_cmd\n");

	if (len < sizeof(struct NL80211_DRIVER_TEST_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	if (data && len) {
		prParams = (struct NL80211_DRIVER_TEST_PARAMS *) data;
	} else {
		DBGLOG(P2P, ERROR,
			"mtk_p2p_cfg80211_testmode_cmd, data is NULL\n");
		return i4Status;
	}
	if (prParams->index >> 24 == 0x01) {
		/* New version */
		prParams->index = prParams->index & ~BITS(24, 31);
	} else {
		/* Old version */
		mtk_p2p_cfg80211_testmode_p2p_sigma_pre_cmd(wiphy, data, len);
		i4Status = 0;
		return i4Status;
	}

	/* Clear the version byte */
	prParams->index = prParams->index & ~BITS(24, 31);

	if (prParams) {
		switch (prParams->index) {
		case 1:	/* P2P Simga */
#if CFG_SUPPORT_HOTSPOT_OPTIMIZATION
			{
				struct NL80211_DRIVER_SW_CMD_PARAMS
					*prParamsCmd;

				prParamsCmd =
					(struct NL80211_DRIVER_SW_CMD_PARAMS *)
					data;

				if ((prParamsCmd->adr & 0xffff0000)
					== 0xffff0000) {
					i4Status =
					mtk_p2p_cfg80211_testmode_sw_cmd(
						wiphy, data, len);
					break;
				}
			}
#endif
			i4Status = mtk_p2p_cfg80211_testmode_p2p_sigma_cmd(
				wiphy, data, len);
			break;
		case 2:	/* WFD */
#if CFG_SUPPORT_WFD
			/* use normal driver command wifi_display */
			/* i4Status =
			 * mtk_p2p_cfg80211_testmode_wfd_update_cmd(
			 * wiphy, data, len);
			 */
#endif
			break;
		case 3:	/* Hotspot Client Management */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
			i4Status =
			mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd(
				wiphy, data, len);
#endif
			break;
		case 0x10:
			i4Status =
				mtk_cfg80211_testmode_get_sta_statistics(
					wiphy, data, len, prGlueInfo);
			break;
#if CFG_SUPPORT_NFC_BEAM_PLUS
		case 0x11:	/*NFC Beam + Indication */
			if (data && len) {
				struct NL80211_DRIVER_SET_NFC_PARAMS *prParams =
					(struct NL80211_DRIVER_SET_NFC_PARAMS *)
					data;

				DBGLOG(P2P, INFO,
					"NFC: BEAM[%d]\n",
					prParams->NFC_Enable);
			}
			break;
		case 0x12:	/*NFC Beam + Indication */
			DBGLOG(P2P, INFO, "NFC: Polling\n");
			i4Status =
				mtk_cfg80211_testmode_get_scan_done(
					wiphy, data, len, prGlueInfo);
			break;
#endif
		case TESTMODE_CMD_ID_HS_CONFIG:
			i4Status =
				mtk_p2p_cfg80211_testmode_hotspot_config_cmd(
					wiphy, data, len);
			break;

		default:
			i4Status = -EINVAL;
			break;
		}
	}

	return i4Status;

}
#else
int mtk_p2p_cfg80211_testmode_cmd(struct wiphy *wiphy, void *data, int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_TEST_PARAMS *prParams =
		(struct NL80211_DRIVER_TEST_PARAMS *) NULL;
	int32_t i4Status = -EINVAL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, TRACE, "mtk_p2p_cfg80211_testmode_cmd\n");

	if (len < sizeof(struct NL80211_DRIVER_TEST_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	if (data && len) {
		prParams = (struct NL80211_DRIVER_TEST_PARAMS *) data;
	} else {
		DBGLOG(P2P, ERROR, "data is NULL\n");
		return i4Status;
	}
	if (prParams->index >> 24 == 0x01) {
		/* New version */
		prParams->index = prParams->index & ~BITS(24, 31);
	} else {
		/* Old version */
		mtk_p2p_cfg80211_testmode_p2p_sigma_pre_cmd(wiphy, data, len);
		i4Status = 0;
		return i4Status;
	}

	/* Clear the version byte */
	prParams->index = prParams->index & ~BITS(24, 31);

	if (prParams) {
		switch (prParams->index) {
		case 1:	/* P2P Simga */
#if CFG_SUPPORT_HOTSPOT_OPTIMIZATION
			{
				struct NL80211_DRIVER_SW_CMD_PARAMS
					*prParamsCmd;

				prParamsCmd =
					(struct NL80211_DRIVER_SW_CMD_PARAMS *)
					data;

				if ((prParamsCmd->adr & 0xffff0000)
					== 0xffff0000) {
					i4Status =
					mtk_p2p_cfg80211_testmode_sw_cmd(
						wiphy, data, len);
					break;
				}
			}
#endif
			i4Status = mtk_p2p_cfg80211_testmode_p2p_sigma_cmd(
				wiphy, data, len);
			break;
		case 2:	/* WFD */
#if CFG_SUPPORT_WFD
			/* use normal driver command wifi_display */
			/* i4Status = mtk_p2p_cfg80211_testmode_wfd_update_cmd(
			 * wiphy, data, len);
			 */
#endif
			break;
		case 3:	/* Hotspot Client Management */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
			i4Status =
			mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd(
				wiphy, data, len);
#endif
			break;
		case 0x10:
			i4Status =
				mtk_cfg80211_testmode_get_sta_statistics(
					wiphy, data, len, prGlueInfo);
			break;
#if CFG_SUPPORT_NFC_BEAM_PLUS
		case 0x11:	/*NFC Beam + Indication */
			if (data && len) {
				struct NL80211_DRIVER_SET_NFC_PARAMS *prParams =
					(struct NL80211_DRIVER_SET_NFC_PARAMS *)
					data;

				DBGLOG(P2P, INFO,
					"NFC: BEAM[%d]\n",
					prParams->NFC_Enable);
			}
			break;
		case 0x12:	/*NFC Beam + Indication */
			DBGLOG(P2P, INFO, "NFC: Polling\n");
			i4Status =
				mtk_cfg80211_testmode_get_scan_done(
					wiphy, data, len, prGlueInfo);
			break;
#endif
		case TESTMODE_CMD_ID_HS_CONFIG:
			i4Status =
				mtk_p2p_cfg80211_testmode_hotspot_config_cmd(
					wiphy, data, len);
			break;

		default:
			i4Status = -EINVAL;
			break;
		}
	}

	return i4Status;

}
#endif

int mtk_p2p_cfg80211_testmode_hotspot_config_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_HOTSPOT_CONFIG_PARAMS *prParams =
		(struct NL80211_DRIVER_HOTSPOT_CONFIG_PARAMS *) NULL;
	uint32_t index;
	uint32_t value;
	uint32_t i;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	if (len < sizeof(struct NL80211_DRIVER_HOTSPOT_CONFIG_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	if (data && len) {
		prParams = (struct NL80211_DRIVER_HOTSPOT_CONFIG_PARAMS *) data;
	} else {
		DBGLOG(P2P, ERROR, "data is NULL or len is 0\n");
		return -EINVAL;
	}

	index = prParams->idx;
	value = prParams->value;

	DBGLOG(P2P, INFO, "NL80211_ATTR_TESTDATA, idx=%d value=%d\n",
			(uint32_t) prParams->idx, (uint32_t) prParams->value);

	switch (index) {
	case 1:		/* Max Clients */
		for (i = 0; i < KAL_P2P_NUM; i++)
			if (p2pFuncIsAPMode(prGlueInfo->prAdapter
				->rWifiVar.prP2PConnSettings[i]))
				kalP2PSetMaxClients(prGlueInfo, value, i);
		break;
	default:
		break;
	}

	return 0;
}

int mtk_p2p_cfg80211_testmode_p2p_sigma_pre_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_TEST_PRE_PARAMS rParams;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	/* P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings =
	 * (P_P2P_CONNECTION_SETTINGS_T)NULL;
	 */
	uint32_t index_mode;
	uint32_t index;
	int32_t value;
	int status = 0;
	uint32_t u4Leng;
	uint8_t ucBssIdx;

	if (len > sizeof(struct NL80211_DRIVER_TEST_PRE_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	kalMemZero(&rParams, sizeof(struct NL80211_DRIVER_TEST_PRE_PARAMS));

	prP2pSpecificBssInfo =
		prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[0];
	/* prP2pConnSettings =
	 * prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings;
	 */

	if (data && len)
		memcpy(&rParams, data, len);

	DBGLOG(P2P, TRACE,
		"NL80211_ATTR_TESTDATA, idx_mode=%d idx=%d value=%u\n",
		rParams.idx_mode,
		rParams.idx,
		rParams.value);

	index_mode = rParams.idx_mode;
	index = rParams.idx;
	value = rParams.value;

	/* 3 FIX ME: Add p2p role index selection */
	if (p2pFuncRoleToBssIdx(
		prGlueInfo->prAdapter, 0, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	switch (index) {
	case 0:		/* Listen CH */
		break;
	case 1:		/* P2p mode */
		break;
	case 4:		/* Noa duration */
		prP2pSpecificBssInfo->rNoaParam.u4NoaDurationMs = value;
		/* only to apply setting when setting NOA count */
		/* status =
		 * mtk_p2p_wext_set_noa_param(prDev,
		 * info, wrqu, (char *)&prP2pSpecificBssInfo->rNoaParam);
		 */
		break;
	case 5:		/* Noa interval */
		prP2pSpecificBssInfo->rNoaParam.u4NoaIntervalMs = value;
		/* only to apply setting when setting NOA count */
		/* status =
		 * mtk_p2p_wext_set_noa_param(prDev,
		 * info, wrqu, (char *)&prP2pSpecificBssInfo->rNoaParam);
		 */
		break;
	case 6:		/* Noa count */
		prP2pSpecificBssInfo->rNoaParam.u4NoaCount = value;
		/* status =
		 * mtk_p2p_wext_set_noa_param(prDev,
		 * info, wrqu, (char *)&prP2pSpecificBssInfo->rNoaParam);
		 */
		break;
	case 100:		/* Oper CH */
		/* 20110920 - frog:
		 * User configurations are placed in ConnSettings.
		 */
		/* prP2pConnSettings->ucOperatingChnl = value; */
		break;
	case 101:		/* Local config Method, for P2P SDK */
		/* prP2pConnSettings->u2LocalConfigMethod = value; */
		break;
	case 102:		/* Sigma P2p reset */
		/* kalMemZero(prP2pConnSettings->aucTargetDevAddr,
		 * MAC_ADDR_LEN);
		 */
		/* prP2pConnSettings->eConnectionPolicy =
		 * ENUM_P2P_CONNECTION_POLICY_AUTO;
		 */
		/* p2pFsmUninit(prGlueInfo->prAdapter); */
		/* p2pFsmInit(prGlueInfo->prAdapter); */
		break;
	case 103:		/* WPS MODE */
		kalP2PSetWscMode(prGlueInfo, value);
		break;
	case 104:		/* P2p send persence, duration */
		break;
	case 105:		/* P2p send persence, interval */
		break;
	case 106:		/* P2P set sleep  */
		{
			struct PARAM_POWER_MODE_ rPowerMode;
			uint32_t rStatus;

			rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
			rPowerMode.ucBssIdx = ucBssIdx;

			rStatus = kalIoctl(prGlueInfo,
				wlanoidSet802dot11PowerSaveProfile,
				&rPowerMode,
				sizeof(rPowerMode),
				FALSE, FALSE, TRUE, &u4Leng);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN,
					"set_power_mgmt error:%x\n", rStatus);
				return -EFAULT;
			}
		}
		break;
	case 107:		/* P2P set opps, CTWindowl */
		prP2pSpecificBssInfo->rOppPsParam.u4CTwindowMs = value;
		/* status = mtk_p2p_wext_set_oppps_param(prDev, info, wrqu,
		 * (char *)&prP2pSpecificBssInfo->rOppPsParam);
		 */
		break;
	case 108:		/* p2p_set_power_save */
		{
			struct PARAM_POWER_MODE_ rPowerMode;
			uint32_t rStatus;

			rPowerMode.ePowerMode = value;
			rPowerMode.ucBssIdx = ucBssIdx;

			rStatus = kalIoctl(prGlueInfo,
				wlanoidSet802dot11PowerSaveProfile,
				&rPowerMode,
				sizeof(rPowerMode),
				FALSE, FALSE, TRUE, &u4Leng);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN,
					"set_power_mgmt error:%x\n", rStatus);
				return -EFAULT;
			}
		}
		break;
	default:
		break;
	}

	return status;

}

int mtk_p2p_cfg80211_testmode_p2p_sigma_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_P2P_SIGMA_PARAMS *prParams =
		(struct NL80211_DRIVER_P2P_SIGMA_PARAMS *) NULL;
	struct P2P_SPECIFIC_BSS_INFO *prP2pSpecificBssInfo =
		(struct P2P_SPECIFIC_BSS_INFO *) NULL;
	/* P_P2P_CONNECTION_SETTINGS_T prP2pConnSettings =
	 * (P_P2P_CONNECTION_SETTINGS_T)NULL;
	 */
	uint32_t rStatus;
	uint32_t index;
	int32_t value;
	int status = 0;
	uint32_t u4Leng;
	uint8_t ucBssIdx;
	uint32_t i;
	struct NL80211_DRIVER_P2P_NOA_PARAMS {
		struct NL80211_DRIVER_TEST_PARAMS hdr;
		uint32_t idx;
		uint32_t value; /* should not be used in this case */
		uint32_t count;
		uint32_t interval;
		uint32_t duration;
	};
	struct NL80211_DRIVER_P2P_NOA_PARAMS *prNoaParams = NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	prP2pSpecificBssInfo =
		prGlueInfo->prAdapter->rWifiVar.prP2pSpecificBssInfo[0];
	/* prP2pConnSettings =
	 * prGlueInfo->prAdapter->rWifiVar.prP2PConnSettings;
	 */

	if (len < sizeof(struct NL80211_DRIVER_P2P_SIGMA_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	if (data && len)
		prParams = (struct NL80211_DRIVER_P2P_SIGMA_PARAMS *) data;
	else {
		DBGLOG(P2P, ERROR, "data is NULL\n");
		return -EINVAL;
	}

	index = (int32_t) prParams->idx;
	value = (int32_t) prParams->value;

	DBGLOG(P2P, INFO, "NL80211_ATTR_TESTDATA, idx=%u value=%u\n",
		prParams->idx, prParams->value);

	/* 3 FIX ME: Add p2p role index selection */
	if (p2pFuncRoleToBssIdx(
		prGlueInfo->prAdapter, 0, &ucBssIdx) != WLAN_STATUS_SUCCESS)
		return -EINVAL;

	switch (index) {
	case 0:		/* Listen CH */
		break;
	case 1:		/* P2p mode */
		break;
	case 4:		/* Noa duration */
		prNoaParams = data;
		prP2pSpecificBssInfo->rNoaParam.u4NoaCount = prNoaParams->count;
		prP2pSpecificBssInfo->rNoaParam.u4NoaIntervalMs =
			prNoaParams->interval;
		prP2pSpecificBssInfo->rNoaParam.u4NoaDurationMs =
			prNoaParams->duration;
		prP2pSpecificBssInfo->rNoaParam.ucBssIdx =
			ucBssIdx;
		DBGLOG(P2P, INFO,
			"SET NOA[%d]: %d %d %d\n",
			ucBssIdx,
			prNoaParams->count,
			prNoaParams->interval,
			prNoaParams->duration);

		/* only to apply setting when setting NOA count */
		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetNoaParam,
			&prP2pSpecificBssInfo->rNoaParam,
			sizeof(struct PARAM_CUSTOM_NOA_PARAM_STRUCT),
			FALSE, FALSE, TRUE, &u4Leng);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN, "set_noa error:%x\n", rStatus);
			return -EFAULT;
		}
		break;
	case 5:		/* Noa interval */
		prP2pSpecificBssInfo->rNoaParam.u4NoaIntervalMs = value;
		/* only to apply setting when setting NOA count */
		/* status =
		 * mtk_p2p_wext_set_noa_param(prDev,
		 * info, wrqu, (char *)&prP2pSpecificBssInfo->rNoaParam);
		 */
		break;
	case 6:		/* Noa count */
		prP2pSpecificBssInfo->rNoaParam.u4NoaCount = value;
		/* status =
		 * mtk_p2p_wext_set_noa_param(prDev,
		 * info, wrqu, (char *)&prP2pSpecificBssInfo->rNoaParam);
		 */
		break;
	case 100:		/* Oper CH */
		/* 20110920 - frog:
		 * User configurations are placed in ConnSettings.
		 */
		/* prP2pConnSettings->ucOperatingChnl = value; */
		break;
	case 101:		/* Local config Method, for P2P SDK */
		/* prP2pConnSettings->u2LocalConfigMethod = value; */
		break;
	case 102:		/* Sigma P2p reset */
		/* kalMemZero(prP2pConnSettings->aucTargetDevAddr,
		 * MAC_ADDR_LEN);
		 */
		/* prP2pConnSettings->eConnectionPolicy =
		 * ENUM_P2P_CONNECTION_POLICY_AUTO;
		 */
		break;
	case 103:		/* WPS MODE */
		kalP2PSetWscMode(prGlueInfo, value);
		break;
	case 104:		/* P2p send persence, duration */
		break;
	case 105:		/* P2p send persence, interval */
		break;
	case 106:		/* P2P set sleep  */
		{
			struct PARAM_POWER_MODE_ rPowerMode;

			rPowerMode.ePowerMode = Param_PowerModeMAX_PSP;
			rPowerMode.ucBssIdx = ucBssIdx;

			rStatus = kalIoctl(prGlueInfo,
				wlanoidSet802dot11PowerSaveProfile,
				&rPowerMode,
				sizeof(rPowerMode),
				FALSE, FALSE, TRUE, &u4Leng);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN,
					"set_power_mgmt error:%x\n", rStatus);
				return -EFAULT;
			}
		}
		break;
	case 107:		/* P2P set opps, CTWindowl */
		prP2pSpecificBssInfo->rOppPsParam.u4CTwindowMs = value;
		prP2pSpecificBssInfo->rOppPsParam.ucBssIdx = ucBssIdx;
		DBGLOG(P2P, INFO, "SET OPPS[%d]: %d\n", ucBssIdx, value);
		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetOppPsParam,
			&prP2pSpecificBssInfo->rOppPsParam,
			sizeof(struct PARAM_CUSTOM_OPPPS_PARAM_STRUCT),
			FALSE, FALSE, TRUE, &u4Leng);
		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN,
				"set_opps error:%x\n", rStatus);
			return -EFAULT;
		}
		break;
	case 108:		/* p2p_set_power_save */
		{
			struct PARAM_POWER_MODE_ rPowerMode;

			rPowerMode.ePowerMode = value;
			rPowerMode.ucBssIdx = ucBssIdx;

			rStatus = kalIoctl(prGlueInfo,
				wlanoidSet802dot11PowerSaveProfile,
				&rPowerMode, sizeof(rPowerMode),
				FALSE, FALSE, TRUE, &u4Leng);

			if (rStatus != WLAN_STATUS_SUCCESS) {
				DBGLOG(REQ, WARN,
					"set_power_mgmt error:%x\n", rStatus);
				return -EFAULT;
			}
		}
		break;
	case 109:		/* Max Clients */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
		for (i = 0; i < KAL_P2P_NUM; i++)
			if (p2pFuncIsAPMode(prGlueInfo->prAdapter
				->rWifiVar.prP2PConnSettings[i]))
				kalP2PSetMaxClients(prGlueInfo, value, i);
#endif
		break;
	case 110:		/* Hotspot WPS mode */
#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER
		rStatus = kalIoctl(prGlueInfo,
			wlanoidSetP2pWPSmode,
			&value, sizeof(value),
			FALSE, FALSE, TRUE, &u4Leng);

		if (rStatus != WLAN_STATUS_SUCCESS) {
			DBGLOG(REQ, WARN,
				"set_wps error:%x\n", rStatus);
			return -EFAULT;
		}
#endif
		break;
	default:
		break;
	}

	return status;

}

#if CFG_SUPPORT_WFD && 0
/* obsolete/decrepated */
int mtk_p2p_cfg80211_testmode_wfd_update_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_WFD_PARAMS *prParams =
		(struct NL80211_DRIVER_WFD_PARAMS *) NULL;
	int status = 0;
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		(struct WFD_CFG_SETTINGS *) NULL;
	struct MSG_WFD_CONFIG_SETTINGS_CHANGED *prMsgWfdCfgUpdate =
		(struct MSG_WFD_CONFIG_SETTINGS_CHANGED *) NULL;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	prParams = (struct NL80211_DRIVER_WFD_PARAMS *) data;

	DBGLOG(P2P, INFO, "mtk_p2p_cfg80211_testmode_wfd_update_cmd\n");

#if 1

	DBGLOG(P2P, INFO, "WFD Enable:%x\n", prParams->WfdEnable);
	DBGLOG(P2P, INFO,
		"WFD Session Available:%x\n",
		prParams->WfdSessionAvailable);
	DBGLOG(P2P, INFO,
		"WFD Couple Sink Status:%x\n",
		prParams->WfdCoupleSinkStatus);
	/* aucReserved0[2] */
	DBGLOG(P2P, INFO, "WFD Device Info:%x\n", prParams->WfdDevInfo);
	DBGLOG(P2P, INFO, "WFD Control Port:%x\n", prParams->WfdControlPort);
	DBGLOG(P2P, INFO,
		"WFD Maximum Throughput:%x\n",
		prParams->WfdMaximumTp);
	DBGLOG(P2P, INFO, "WFD Extend Capability:%x\n", prParams->WfdExtendCap);
	DBGLOG(P2P, INFO,
		"WFD Couple Sink Addr " MACSTR "\n",
		MAC2STR(prParams->WfdCoupleSinkAddress));
	DBGLOG(P2P, INFO,
		"WFD Associated BSSID " MACSTR "\n",
		MAC2STR(prParams->WfdAssociatedBssid));
	/* UINT_8 aucVideolp[4]; */
	/* UINT_8 aucAudiolp[4]; */
	DBGLOG(P2P, INFO, "WFD Video Port:%x\n", prParams->WfdVideoPort);
	DBGLOG(P2P, INFO, "WFD Audio Port:%x\n", prParams->WfdAudioPort);
	DBGLOG(P2P, INFO, "WFD Flag:%x\n", prParams->WfdFlag);
	DBGLOG(P2P, INFO, "WFD Policy:%x\n", prParams->WfdPolicy);
	DBGLOG(P2P, INFO, "WFD State:%x\n", prParams->WfdState);
	/* UINT_8 aucWfdSessionInformationIE[24*8]; */
	DBGLOG(P2P, INFO,
		"WFD Session Info Length:%x\n",
		prParams->WfdSessionInformationIELen);
	/* UINT_8 aucReserved1[2]; */
	DBGLOG(P2P, INFO,
		"WFD Primary Sink Addr " MACSTR "\n",
		MAC2STR(prParams->aucWfdPrimarySinkMac));
	DBGLOG(P2P, INFO,
		"WFD Secondary Sink Addr " MACSTR "\n",
		MAC2STR(prParams->aucWfdSecondarySinkMac));
	DBGLOG(P2P, INFO, "WFD Advanced Flag:%x\n", prParams->WfdAdvanceFlag);
	DBGLOG(P2P, INFO, "WFD Sigma mode:%x\n", prParams->WfdSigmaMode);
	/* UINT_8 aucReserved2[64]; */
	/* UINT_8 aucReserved3[64]; */
	/* UINT_8 aucReserved4[64]; */

#endif

	prWfdCfgSettings =
		&(prGlueInfo->prAdapter->rWifiVar.rWfdConfigureSettings);

	kalMemCopy(&prWfdCfgSettings->u4WfdCmdType,
		&prParams->WfdCmdType,
		sizeof(struct WFD_CFG_SETTINGS));

	prMsgWfdCfgUpdate = cnmMemAlloc(prGlueInfo->prAdapter,
		RAM_TYPE_MSG,
		sizeof(struct MSG_WFD_CONFIG_SETTINGS_CHANGED));

	if (prMsgWfdCfgUpdate == NULL) {
		return status;
	}

	prMsgWfdCfgUpdate->rMsgHdr.eMsgId = MID_MNY_P2P_WFD_CFG_UPDATE;
	prMsgWfdCfgUpdate->prWfdCfgSettings = prWfdCfgSettings;

	mboxSendMsg(prGlueInfo->prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prMsgWfdCfgUpdate,
		MSG_SEND_METHOD_BUF);

#if 0				/* Test Only */
/* prWfdCfgSettings->ucWfdEnable = 1; */
/* prWfdCfgSettings->u4WfdFlag |= WFD_FLAGS_DEV_INFO_VALID; */
	prWfdCfgSettings->u4WfdFlag |= WFD_FLAGS_DEV_INFO_VALID;
	prWfdCfgSettings->u2WfdDevInfo = 123;
	prWfdCfgSettings->u2WfdControlPort = 456;
	prWfdCfgSettings->u2WfdMaximumTp = 789;

	prWfdCfgSettings->u4WfdFlag |= WFD_FLAGS_SINK_INFO_VALID;
	prWfdCfgSettings->ucWfdCoupleSinkStatus = 0xAB;
	{
		uint8_t aucTestAddr[MAC_ADDR_LEN] = {
			0x77, 0x66, 0x55, 0x44, 0x33, 0x22 };

		COPY_MAC_ADDR(prWfdCfgSettings->aucWfdCoupleSinkAddress,
			aucTestAddr);
	}

	prWfdCfgSettings->u4WfdFlag |= WFD_FLAGS_EXT_CAPABILITY_VALID;
	prWfdCfgSettings->u2WfdExtendCap = 0xCDE;

#endif

	return status;

}
#endif /*  CFG_SUPPORT_WFD */

#if CFG_SUPPORT_HOTSPOT_WPS_MANAGER

int mtk_p2p_cfg80211_testmode_hotspot_block_list_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_hotspot_block_PARAMS *prParams =
		(struct NL80211_DRIVER_hotspot_block_PARAMS *) NULL;
	int fgIsValid = 0;
	uint32_t i;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	if (len < sizeof(struct NL80211_DRIVER_hotspot_block_PARAMS)) {
		DBGLOG(P2P, WARN, "len [%d] is invalid!\n",
			len);
		return -EINVAL;
	}

	if (data && len)
		prParams = (struct NL80211_DRIVER_hotspot_block_PARAMS *) data;
	else
		return fgIsValid;

	DBGLOG(P2P, INFO,
		"%s" MACSTR "\n",
		prParams->ucblocked?"Block":"Unblock",
		MAC2STR(prParams->aucBssid));

	for (i = 0; i < KAL_P2P_NUM; i++)
		fgIsValid |=
			kalP2PSetBlackList(prGlueInfo,
				prParams->aucBssid, prParams->ucblocked, i);

	return fgIsValid;

}

#endif

int mtk_p2p_cfg80211_testmode_sw_cmd(IN struct wiphy *wiphy,
		IN void *data, IN int len)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct NL80211_DRIVER_SW_CMD_PARAMS *prParams =
		(struct NL80211_DRIVER_SW_CMD_PARAMS *) NULL;
	uint32_t rstatus = WLAN_STATUS_SUCCESS;
	int fgIsValid = 0;
	uint32_t u4SetInfoLen = 0;

	ASSERT(wiphy);

	P2P_WIPHY_PRIV(wiphy, prGlueInfo);

	DBGLOG(P2P, TRACE, "--> %s()\n", __func__);

	if (len < sizeof(struct NL80211_DRIVER_SW_CMD_PARAMS))
		rstatus = WLAN_STATUS_INVALID_LENGTH;
	else if (!data)
		rstatus = WLAN_STATUS_INVALID_DATA;
	else {
		prParams = (struct NL80211_DRIVER_SW_CMD_PARAMS *) data;
		if (prParams->set == 1) {
			rstatus = kalIoctl(prGlueInfo,
				(PFN_OID_HANDLER_FUNC) wlanoidSetSwCtrlWrite,
				&prParams->adr, (uint32_t) 8,
				FALSE, FALSE, TRUE, &u4SetInfoLen);
		}
	}

	if (rstatus != WLAN_STATUS_SUCCESS)
		fgIsValid = -EFAULT;

	return fgIsValid;
}

#endif

#endif /* CFG_ENABLE_WIFI_DIRECT && CFG_ENABLE_WIFI_DIRECT_CFG_80211 */
