// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_p2p.c
 *    \brief  Main routines of Linux driver interface for Wi-Fi Direct
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

#include <linux/poll.h>

#include <linux/kmod.h>


#include "gl_os.h"
#include "debug.h"
#include "wlan_lib.h"
#include "gl_wext.h"

/* #include <net/cfg80211.h> */
#include "gl_p2p_ioctl.h"

#include "precomp.h"
#include "gl_vendor.h"
#include "gl_cfg80211.h"
#include "mddp.h"
/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

#if CFG_ENABLE_WIFI_DIRECT
#define ARGV_MAX_NUM        (4)

/*For CFG80211 - wiphy parameters*/
#define MAX_SCAN_LIST_NUM   (1)
#define MAX_SCAN_IE_LEN     (512)

#if 0
#define RUNNING_P2P_MODE 0
#define RUNNING_AP_MODE 1
#define RUNNING_DUAL_AP_MODE 2
#endif
/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

#define	SKIP_ROLE_NONE 0
#define	SKIP_ROLE_ALL 1
#define	SKIP_ROLE_EXCEPT_MAIN 2

/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */


/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************
 */


static const struct iw_priv_args rP2PIwPrivTable[] = {
	{
	 .cmd = IOC_P2P_CFG_DEVICE,
	 .set_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_cfg_device_type),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_CFG_DEVICE"}
	,
	{
	 .cmd = IOC_P2P_START_STOP_DISCOVERY,
	 .set_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_req_device_type),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_DISCOVERY"}
	,
	{
	 .cmd = IOC_P2P_DISCOVERY_RESULTS,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_RESULT"}
	,
	{
	 .cmd = IOC_P2P_WSC_BEACON_PROBE_RSP_IE,
	 .set_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_hostapd_param),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_WSC_IE"}
	,
	{
	 .cmd = IOC_P2P_CONNECT_DISCONNECT,
	 .set_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_connect_device),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_CONNECT"}
	,
	{
	 .cmd = IOC_P2P_PASSWORD_READY,
	 .set_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_password_ready),
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_PASSWD_RDY"}
	,
	{
	 .cmd = IOC_P2P_GET_STRUCT,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = 256,
	 .name = "P2P_GET_STRUCT"}
	,
	{
	 .cmd = IOC_P2P_SET_STRUCT,
	 .set_args = 256,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "P2P_SET_STRUCT"}
	,
	{
	 .cmd = IOC_P2P_GET_REQ_DEVICE_INFO,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = IW_PRIV_TYPE_BYTE
				| (__u16) sizeof(struct iw_p2p_device_req),
	 .name = "P2P_GET_REQDEV"}
	,
	{
	 /* SET STRUCT sub-ioctls commands */
	 .cmd = PRIV_CMD_OID,
	 .set_args = 256,
	 .get_args = IW_PRIV_TYPE_NONE,
	 .name = "set_oid"}
	,
	{
	 /* GET STRUCT sub-ioctls commands */
	 .cmd = PRIV_CMD_OID,
	 .set_args = IW_PRIV_TYPE_NONE,
	 .get_args = 256,
	 .name = "get_oid"}
};

#ifdef CONFIG_PM
static const struct wiphy_wowlan_support mtk_p2p_wowlan_support = {
	.flags = WIPHY_WOWLAN_DISCONNECT | WIPHY_WOWLAN_ANY,
};
#endif

static const struct ieee80211_iface_limit mtk_p2p_sta_go_limits[] = {
	{
		.max = 3,
		.types = BIT(NL80211_IFTYPE_STATION),
	},

	{
		.max = 3,
		.types = BIT(NL80211_IFTYPE_P2P_GO)
				| BIT(NL80211_IFTYPE_P2P_CLIENT),
	},
};

#if (CFG_SUPPORT_DFS_MASTER == 1)
#if (KERNEL_VERSION(3, 17, 0) > CFG80211_VERSION_CODE)

static const struct ieee80211_iface_limit mtk_ap_limits[] = {
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_AP),
	},
};
#endif
#endif

static const struct ieee80211_iface_combination
mtk_iface_combinations_sta[] = {
	{
#ifdef CFG_NUM_DIFFERENT_CHANNELS_STA
		.num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_STA,
#else
		.num_different_channels = 2,
#endif /* CFG_NUM_DIFFERENT_CHANNELS_STA */
		.max_interfaces = 3,
		/*.beacon_int_infra_match = true,*/
		.limits = mtk_p2p_sta_go_limits,
		.n_limits = 1, /* include p2p */
	},
};

static const struct ieee80211_iface_combination
mtk_iface_combinations_p2p[] = {
	{
#if defined(CFG_NUM_DIFFERENT_CHANNELS_P2P)
		.num_different_channels = CFG_NUM_DIFFERENT_CHANNELS_P2P,
#else
		.num_different_channels = 4,
#endif /* CFG_NUM_DIFFERENT_CHANNELS_P2P */
		.max_interfaces = 4,
		/*.beacon_int_infra_match = true,*/
		.limits = mtk_p2p_sta_go_limits,
		.n_limits = ARRAY_SIZE(mtk_p2p_sta_go_limits), /* include p2p */
	},
#if (CFG_SUPPORT_DFS_MASTER == 1)
#if (KERNEL_VERSION(3, 17, 0) > CFG80211_VERSION_CODE)
	/* ONLY for passing checks in cfg80211_can_use_iftype_chan
	 * before linux-3.17.0
	 */
	{
		.num_different_channels = 2,
		.max_interfaces = 2,
		.limits = mtk_ap_limits,
		.n_limits = ARRAY_SIZE(mtk_ap_limits),
		.radar_detect_widths = BIT(NL80211_CHAN_WIDTH_20_NOHT) |
				       BIT(NL80211_CHAN_WIDTH_20) |
				       BIT(NL80211_CHAN_WIDTH_40) |
				       BIT(NL80211_CHAN_WIDTH_80) |
				       BIT(NL80211_CHAN_WIDTH_80P80),
	},
#endif
#endif
};


const struct ieee80211_iface_combination
	*p_mtk_iface_combinations_sta = mtk_iface_combinations_sta;
const int32_t mtk_iface_combinations_sta_num =
	ARRAY_SIZE(mtk_iface_combinations_sta);

const struct ieee80211_iface_combination
	*p_mtk_iface_combinations_p2p = mtk_iface_combinations_p2p;
const int32_t mtk_iface_combinations_p2p_num =
	ARRAY_SIZE(mtk_iface_combinations_p2p);

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/* Net Device Hooks */
static int p2pOpen(struct net_device *prDev);

static int p2pStop(struct net_device *prDev);

static struct net_device_stats *p2pGetStats(struct net_device *prDev);

static void p2pSetMulticastList(struct net_device *prDev);
static void p2pSetMulticastListWorkQueue(struct work_struct *work);

static netdev_tx_t p2pHardStartXmit(struct sk_buff *prSkb,
		struct net_device *prDev);

static int p2pSetMACAddress(struct net_device *prDev, void *addr);

static int p2pDoIOCTL(struct net_device *prDev,
		struct ifreq *prIFReq,
		int i4Cmd);

#if KERNEL_VERSION(5, 15, 0) <= CFG80211_VERSION_CODE
static int p2pDoPrivIOCTL(struct net_device *prDev, struct ifreq *prIfReq,
		void __user *prData, int i4Cmd);
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->init
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0         The execution of wlanInit succeeds.
 * \retval -ENXIO    No such device.
 */
/*---------------------------------------------------------------------------*/
static int p2pInit(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv;

	if (!prDev)
		return -ENXIO;
#if CFG_SUPPORT_RX_GRO
	kalRxGroInit(prDev);
#endif /* CFG_SUPPORT_RX_GRO */

	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDev);
	INIT_WORK(&(prNetDevPriv->workq), p2pSetMulticastListWorkQueue);

	return 0;		/* success */
}				/* end of p2pInit() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief A function for prDev->uninit
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
static void p2pUninit(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv;

	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDev);
	cancel_work_sync(&(prNetDevPriv->workq));
}				/* end of p2pUninit() */

const struct net_device_ops p2p_netdev_ops = {
	.ndo_open = p2pOpen,
	.ndo_stop = p2pStop,
	.ndo_set_mac_address = p2pSetMACAddress,
	.ndo_set_rx_mode = p2pSetMulticastList,
	.ndo_get_stats = p2pGetStats,
	.ndo_do_ioctl = p2pDoIOCTL,
#if KERNEL_VERSION(5, 15, 0) <= LINUX_VERSION_CODE
	.ndo_siocdevprivate = p2pDoPrivIOCTL,
#endif
	.ndo_start_xmit = p2pHardStartXmit,
	/* .ndo_select_queue       = p2pSelectQueue, */
	.ndo_select_queue = wlanSelectQueue,
	.ndo_init = p2pInit,
	.ndo_uninit = p2pUninit,
};

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

/*---------------------------------------------------------------------------*/
/*!
 * \brief Allocate memory for P2P_DEV_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2PAllocInfo(struct GLUE_INFO *prGlueInfo, uint8_t ucIdex)
{
	struct ADAPTER *prAdapter = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	/* UINT_32 u4Idx = 0; */

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	prWifiVar = &(prAdapter->rWifiVar);

	ASSERT(prAdapter);
	ASSERT(prWifiVar);

	do {
		if (prGlueInfo->prP2PInfo[ucIdex] == NULL) {
			/*alloc memory for p2p info */
			prGlueInfo->prP2PInfo[ucIdex] =
				kalMemAlloc(sizeof(struct GL_P2P_INFO),
					VIR_MEM_TYPE);

			if (prGlueInfo->prP2PDevInfo == NULL) {
				prGlueInfo->prP2PDevInfo =
					kalMemAlloc(
						sizeof(struct GL_P2P_DEV_INFO),
						VIR_MEM_TYPE);
				if (prGlueInfo->prP2PDevInfo) {
					kalMemZero(prGlueInfo->prP2PDevInfo,
						sizeof(struct GL_P2P_DEV_INFO));
				}
			}

			if (prAdapter->prP2pInfo == NULL) {
				prAdapter->prP2pInfo =
					kalMemAlloc(sizeof(struct P2P_DEV_INFO),
						    VIR_MEM_TYPE);
				if (prAdapter->prP2pInfo) {
					kalMemZero(prAdapter->prP2pInfo,
						   sizeof(struct P2P_DEV_INFO));
				}
			}

			if (prWifiVar->prP2pDevFsmInfo == NULL) {
				/* Don't only create P2P device for ucIdex 0.
				 * Avoid the exception that mtk_init_ap_role
				 * called without p2p0.
				 */
				prWifiVar->prP2pDevFsmInfo =
					kalMemAlloc(
						sizeof(struct P2P_DEV_FSM_INFO),
						VIR_MEM_TYPE);
				if (prWifiVar->prP2pDevFsmInfo) {
					kalMemZero(prWifiVar->prP2pDevFsmInfo,
						sizeof(struct
							P2P_DEV_FSM_INFO));
				}
			}

			prWifiVar->prP2PConnSettings[ucIdex] =
				kalMemAlloc(
					sizeof(struct P2P_CONNECTION_SETTINGS),
					VIR_MEM_TYPE);
			prWifiVar->prP2pSpecificBssInfo[ucIdex] =
				kalMemAlloc(
					sizeof(struct P2P_SPECIFIC_BSS_INFO),
					VIR_MEM_TYPE);
#if CFG_ENABLE_PER_STA_STATISTICS_LOG
			prWifiVar->prP2pQueryStaStatistics[ucIdex] =
				kalMemAlloc(
					sizeof(struct PARAM_GET_STA_STATISTICS),
					VIR_MEM_TYPE);
#endif
			/* TODO: It can be moved
			 * to the interface been created.
			 */
#if 0
			for (u4Idx = 0; u4Idx < BSS_P2P_NUM; u4Idx++) {
				prWifiVar->aprP2pRoleFsmInfo[u4Idx] =
				kalMemAlloc(sizeof(struct P2P_ROLE_FSM_INFO),
					VIR_MEM_TYPE);
			}
#endif
		} else {
			ASSERT(prAdapter->prP2pInfo != NULL);
			ASSERT(prWifiVar->prP2PConnSettings[ucIdex] != NULL);
			/* ASSERT(prWifiVar->prP2pFsmInfo != NULL); */
			ASSERT(prWifiVar->prP2pSpecificBssInfo[ucIdex] != NULL);
		}
		/*MUST set memory to 0 */
		kalMemZero(prGlueInfo->prP2PInfo[ucIdex],
			sizeof(struct GL_P2P_INFO));
		kalMemZero(prWifiVar->prP2PConnSettings[ucIdex],
			sizeof(struct P2P_CONNECTION_SETTINGS));
/* kalMemZero(prWifiVar->prP2pFsmInfo, sizeof(P2P_FSM_INFO_T)); */
		kalMemZero(prWifiVar->prP2pSpecificBssInfo[ucIdex],
			sizeof(struct P2P_SPECIFIC_BSS_INFO));
#if CFG_ENABLE_PER_STA_STATISTICS_LOG
		if (prWifiVar->prP2pQueryStaStatistics[ucIdex])
			kalMemZero(prWifiVar->prP2pQueryStaStatistics[ucIdex],
				sizeof(struct PARAM_GET_STA_STATISTICS));
#endif
		init_completion(&prGlueInfo->prP2PInfo[ucIdex]->rStopApComp);
		init_completion(&prGlueInfo->prP2PInfo[ucIdex]->rDisconnComp);
		init_completion(&prGlueInfo->prP2PInfo[ucIdex]->rDelStaComp);

#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
		init_completion(
			&prGlueInfo->prP2PInfo[ucIdex]->rSuspendStopApComp);
		prGlueInfo->prP2PInfo[ucIdex]->ulSuspendStopAp = 0;
#endif
	} while (FALSE);

	if (!prGlueInfo->prP2PDevInfo)
		DBGLOG(P2P, ERROR, "prP2PDevInfo error\n");
	else
		DBGLOG(P2P, TRACE, "prP2PDevInfo ok\n");

	if (!prGlueInfo->prP2PInfo[ucIdex])
		DBGLOG(P2P, ERROR, "prP2PInfo error\n");
	else
		DBGLOG(P2P, TRACE, "prP2PInfo ok\n");



	/* chk if alloc successful or not */
	if (prGlueInfo->prP2PInfo[ucIdex] &&
		prGlueInfo->prP2PDevInfo &&
		prAdapter->prP2pInfo &&
		prWifiVar->prP2PConnSettings[ucIdex] &&
/* prWifiVar->prP2pFsmInfo && */
	    prWifiVar->prP2pSpecificBssInfo[ucIdex])
		return TRUE;


	DBGLOG(P2P, ERROR, "[fail!]p2PAllocInfo :fail\n");

	if (prWifiVar->prP2pSpecificBssInfo[ucIdex]) {
		kalMemFree(prWifiVar->prP2pSpecificBssInfo[ucIdex],
			VIR_MEM_TYPE,
			sizeof(struct P2P_SPECIFIC_BSS_INFO));

		prWifiVar->prP2pSpecificBssInfo[ucIdex] = NULL;
	}

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
	if (prWifiVar->prP2pQueryStaStatistics[ucIdex]) {
		kalMemFree(prWifiVar->prP2pQueryStaStatistics[ucIdex],
			VIR_MEM_TYPE,
			sizeof(struct PARAM_GET_STA_STATISTICS));
		prWifiVar->prP2pQueryStaStatistics[ucIdex] = NULL;
	}
#endif

/* if (prWifiVar->prP2pFsmInfo) { */
/* kalMemFree(prWifiVar->prP2pFsmInfo,
 * VIR_MEM_TYPE, sizeof(P2P_FSM_INFO_T));
 */

/* prWifiVar->prP2pFsmInfo = NULL; */
/* } */
	if (prWifiVar->prP2PConnSettings[ucIdex]) {
		kalMemFree(prWifiVar->prP2PConnSettings[ucIdex],
			VIR_MEM_TYPE, sizeof(struct P2P_CONNECTION_SETTINGS));

		prWifiVar->prP2PConnSettings[ucIdex] = NULL;
	}
	if (prGlueInfo->prP2PDevInfo) {
		kalMemFree(prGlueInfo->prP2PDevInfo,
			VIR_MEM_TYPE, sizeof(struct GL_P2P_DEV_INFO));

		prGlueInfo->prP2PDevInfo = NULL;
	}
	if (prGlueInfo->prP2PInfo[ucIdex]) {
		kalMemFree(prGlueInfo->prP2PInfo[ucIdex],
			VIR_MEM_TYPE, sizeof(struct GL_P2P_INFO));

		prGlueInfo->prP2PInfo[ucIdex] = NULL;
	}
	if (prAdapter->prP2pInfo) {
		kalMemFree(prAdapter->prP2pInfo,
			VIR_MEM_TYPE, sizeof(struct P2P_DEV_INFO));

		prAdapter->prP2pInfo = NULL;
	}
	return FALSE;

}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Free memory for P2P_DEV_INFO, GL_P2P_INFO, P2P_CONNECTION_SETTINGS
 *                                          P2P_SPECIFIC_BSS_INFO, P2P_FSM_INFO
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *	[in] ucIdx	     The BSS with the idx will be freed.
 *			     "ucIdx == 0xff" will free all BSSs.
 *			     Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2PFreeInfo(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx)
{
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;
	struct GL_P2P_INFO *prGlP2pInfo;
	struct WIFI_VAR *prWifiVar;
	struct P2P_PENDING_MGMT_INFO *prPendingMgmtInfo = NULL;

	ASSERT(prGlueInfo);
	ASSERT(prAdapter);

	if (ucIdx >= KAL_P2P_NUM) {
		DBGLOG(P2P, ERROR, "ucIdx=%d is invalid\n", ucIdx);
		return FALSE;
	}

	/* Expect that prAdapter->prP2pInfo must be existing. */
	if (prAdapter->prP2pInfo == NULL) {
		DBGLOG(P2P, ERROR, "prAdapter->prP2pInfo is NULL\n");
		return FALSE;
	}

	prWifiVar = &prAdapter->rWifiVar;
	prGlP2pInfo = prGlueInfo->prP2PInfo[ucIdx];

	/* TODO: how can I sure that the specific P2P device can be freed?
	 * The original check is that prGlueInfo->prAdapter->fgIsP2PRegistered.
	 * For one wiphy feature, this func may be called without
	 * (fgIsP2PRegistered == FALSE) condition.
	 */

	if (prGlP2pInfo != NULL) {
		p2pFreeMemSafe(prGlueInfo,
			(void **)&prWifiVar->prP2PConnSettings[ucIdx],
			sizeof(struct P2P_CONNECTION_SETTINGS));

		p2pFreeMemSafe(prGlueInfo,
			(void **)&prWifiVar->prP2pSpecificBssInfo[ucIdx],
			sizeof(struct P2P_SPECIFIC_BSS_INFO));

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
		p2pFreeMemSafe(prGlueInfo,
			(void **)&prWifiVar->prP2pQueryStaStatistics[ucIdx],
			sizeof(struct PARAM_GET_STA_STATISTICS));
#endif
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_P2P_PENDING_MGMT_TX);
		while (!LINK_IS_EMPTY(&prGlP2pInfo->rWaitTxDoneLink)) {
			LINK_REMOVE_HEAD(
				&prGlP2pInfo->rWaitTxDoneLink,
				prPendingMgmtInfo,
				struct P2P_PENDING_MGMT_INFO *);
			DBGLOG(P2P, INFO,
				"Free pending mgmt link[%u] cookie: 0x%llx\n",
				ucIdx,
				prPendingMgmtInfo->u8PendingMgmtCookie);
			cnmMemFree(prAdapter, prPendingMgmtInfo);
		}
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_P2P_PENDING_MGMT_TX);

		p2pFreeMemSafe(prGlueInfo,
			(void **)&prGlP2pInfo,
			sizeof(struct GL_P2P_INFO));
		prGlueInfo->prP2PInfo[ucIdx] = NULL;
		prAdapter->prP2pInfo->u4DeviceNum--;
	}

	if (prAdapter->prP2pInfo->u4DeviceNum == 0) {
		/* all prP2PInfo are freed, and free the general part now */

		p2pFreeMemSafe(prGlueInfo,
			(void **)&prAdapter->prP2pInfo,
			sizeof(struct P2P_DEV_INFO));

		if (prGlueInfo->prP2PDevInfo) {
			p2pFreeMemSafe(prGlueInfo,
				(void **)&prGlueInfo->prP2PDevInfo,
				sizeof(struct GL_P2P_DEV_INFO));
		}
		if (prAdapter->rWifiVar.prP2pDevFsmInfo) {
			p2pFreeMemSafe(prGlueInfo,
				(void **)&prWifiVar->prP2pDevFsmInfo,
				sizeof(struct P2P_DEV_FSM_INFO));
		}
	}

	return TRUE;
}

void p2pFreeMemSafe(struct GLUE_INFO *prGlueInfo,
		void **pprMemInfo, uint32_t size)
{
	void *prTmpMemInfo = NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prTmpMemInfo = *pprMemInfo;
	*pprMemInfo = NULL;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	kalMemFree(prTmpMemInfo, VIR_MEM_TYPE, size);
}

/*---------------------------------------------------------------------------*/
/*!
 * \param
 *  [in] prGlueInfo		Pointer to glue info
 *  [in] fgIsRtnlLockAcquired	Is the rtnl lock already hold or not.
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pNetRegister(struct GLUE_INFO *prGlueInfo,
		uint8_t fgIsRtnlLockAcquired)
{
	struct ADAPTER *prAdapter = NULL;
	struct net_device *prDevHandler = NULL;
	struct net_device **pprP2pDev = NULL;
	uint32_t i;
	int32_t i4RetReg;
	u_int8_t ret = TRUE, fgDoRegister = FALSE;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	pprP2pDev = prGlueInfo->prP2pDev;

	ASSERT(prAdapter);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_UNREGISTERED &&
		prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED) {
		prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERING;
		fgDoRegister = TRUE;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoRegister) {
		DBGLOG(P2P, WARN,
			"skip register, p2p_state=%d, net_state=%d\n",
			prAdapter->rP2PRegState,
			prAdapter->rP2PNetRegState);
		return TRUE;
	}

	for (i = 0; i < prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum; i++) {
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		prDevHandler = prGlueInfo->prP2PInfo[i] ?
			prGlueInfo->prP2PInfo[i]->prDevHandler :
			NULL;

		/* Check NETREG_RELEASED for the case that free_netdev
		 * is called but not set to NULL yet.
		 */
		if (prDevHandler == NULL ||
		    prDevHandler->reg_state == NETREG_RELEASED) {
			prAdapter->rP2PNetRegState =
				ENUM_NET_REG_STATE_UNREGISTERED;
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			ret = FALSE;
			goto fail;
		}

		/* net device initialize */
		netif_carrier_off(prDevHandler);
		netif_tx_stop_all_queues(prDevHandler);

		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		if (fgIsRtnlLockAcquired) {
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
			i4RetReg = cfg80211_register_netdevice(prDevHandler);
#else
			i4RetReg = register_netdevice(prDevHandler);
#endif
		} else {
			i4RetReg = register_netdev(prDevHandler);
		}

		DBGLOG(P2P, TRACE,
			"P2P interface %d %s ifindex=%d reg=%d\n",
			i, prDevHandler->name, prDevHandler->ifindex,
			i4RetReg);

		if (i4RetReg) {
			ret = FALSE;
			goto fail;
		} else {
			GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			pprP2pDev[i] = prDevHandler;
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		}
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_REGISTERED;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	goto exit;
fail:
	for (i = 0;
	     i < prGlueInfo->prAdapter->prP2pInfo->u4DeviceNum;
	     i++) {
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		prDevHandler = prGlueInfo->prP2PInfo[i] ?
			prGlueInfo->prP2PInfo[i]->prDevHandler :
			NULL;
		pprP2pDev[i] = NULL;
		prGlueInfo->prP2PInfo[i]->prDevHandler = NULL;
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		if (!prDevHandler)
			continue;

		if (prDevHandler->reg_state == NETREG_REGISTERED) {
			if (fgIsRtnlLockAcquired) {
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
				cfg80211_unregister_netdevice(prDevHandler);
#else
				unregister_netdevice(prDevHandler);
#endif
			} else {
				unregister_netdev(prDevHandler);
			}
#if KERNEL_VERSION(4, 11, 9) > CFG80211_VERSION_CODE
			free_netdev(prDevHandler);
#endif
		} else {
			free_netdev(prDevHandler);
		}
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
exit:
	return ret;
}

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
static void p2pNetUnregisterMldLinks(struct GLUE_INFO *prGlueInfo,
	struct net_device *prRoleDev,
	uint8_t ucRoleIdx)
{
	uint8_t ucIdx = 0;

	/* Only mlo sap is supported by kernel add intf link API */
	if (prRoleDev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP)
		return;

	for (ucIdx = ucRoleIdx + 1; ucIdx < KAL_P2P_NUM; ucIdx++) {
		struct GL_P2P_INFO *prP2PInfo;

		prP2PInfo = prGlueInfo->prP2PInfo[ucIdx];
		if (prRoleDev == prP2PInfo->aprRoleHandler)
			prP2PInfo->aprRoleHandler = NULL;
	}
}
#endif

u_int8_t p2pNetUnregister(struct GLUE_INFO *prGlueInfo,
		uint8_t fgIsRtnlLockAcquired,
		u_int8_t fgIsWiphyLockHeld)
{
	u_int8_t fgDoUnregister = FALSE;
	uint8_t ucRoleIdx;
	struct ADAPTER *prAdapter = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = NULL;
	struct GL_P2P_INFO *prP2PInfo = NULL;
	struct BSS_INFO *prP2pBssInfo = NULL;
	int iftype = 0;
	struct net_device *prRoleDev = NULL;
	struct GL_P2P_DEV_INFO *prGlueP2pDevInfo =
		(struct GL_P2P_DEV_INFO *) NULL;
	struct cfg80211_p2p_roc_request *prP2pRocRequest =
		(struct cfg80211_p2p_roc_request *) NULL;

	GLUE_SPIN_LOCK_DECLARATION();

	prAdapter = prGlueInfo->prAdapter;

	ASSERT(prGlueInfo);
	ASSERT(prAdapter);

	prGlueP2pDevInfo = prGlueInfo->prP2PDevInfo;
	if (prGlueP2pDevInfo) {
		prP2pRocRequest = &(prGlueP2pDevInfo->rP2pRocRequest);
		if (prP2pRocRequest->wdev) {
			kalP2PIndicateChannelReady(
				prAdapter->prGlueInfo,
				prP2pRocRequest->u8Cookie,
				prP2pRocRequest->ucReqChnlNum,
				prP2pRocRequest->eBand,
				prP2pRocRequest->eChnlSco,
				prP2pRocRequest->u4MaxInterval);
			kalP2PIndicateChannelExpired(
				prAdapter->prGlueInfo,
				prP2pRocRequest->u8Cookie,
				prP2pRocRequest->ucReqChnlNum,
				prP2pRocRequest->eBand,
				prP2pRocRequest->eChnlSco);
			/* sleep 10 ms to wait for supplicant cancel
			 * ungoing radio work before freeing netdev
			 */
			kalMsleep(10);
		}
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_REGISTERED &&
		prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED) {
		prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERING;
		fgDoUnregister = TRUE;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoUnregister) {
		DBGLOG(P2P, WARN,
			"skip unregister, p2p_state=%d, net_state=%d\n",
			prAdapter->rP2PRegState,
			prAdapter->rP2PNetRegState);
		return TRUE;
	}

	for (ucRoleIdx = 0; ucRoleIdx < KAL_P2P_NUM; ucRoleIdx++) {
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];
		if (prP2PInfo == NULL) {
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			continue;
		}

		if (prP2PInfo->prDevHandler == NULL) {
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			continue;
		}

		/* don't unregister the dev that share with the AIS */
		if (wlanIsAisDev(prP2PInfo->prDevHandler)) {
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			continue;
		}

		prRoleDev = prP2PInfo->aprRoleHandler;
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		if (prRoleDev != NULL) {
			/* info cfg80211 disconnect */
			prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
				netdev_priv(prRoleDev);
			iftype = prRoleDev->ieee80211_ptr->iftype;
			prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
							prNetDevPriv->ucBssIdx);

			/* FIXME: The p2pRoleFsmUninit may call the
			 * cfg80211_disconnected.
			 * p2pRemove()->glUnregisterP2P->p2pRoleFsmUninit(),
			 * it may be too late.
			 */
			if ((prP2pBssInfo != NULL) &&
			    (prP2pBssInfo->eConnectionState ==
				MEDIA_STATE_CONNECTED) &&
			    ((iftype == NL80211_IFTYPE_P2P_CLIENT) ||
			     (iftype == NL80211_IFTYPE_STATION))) {
				p2pChangeMediaState(prAdapter,
					prP2pBssInfo,
					MEDIA_STATE_DISCONNECTED);

#if CFG_WPS_DISCONNECT || (KERNEL_VERSION(4, 2, 0) <= CFG80211_VERSION_CODE)
				cfg80211_disconnected(prRoleDev, 0, NULL, 0,
							TRUE, GFP_KERNEL);
#else
				cfg80211_disconnected(prRoleDev, 0, NULL, 0,
							GFP_KERNEL);
#endif
			}

			if (prRoleDev != prP2PInfo->prDevHandler) {
				if (netif_carrier_ok(prRoleDev))
					netif_carrier_off(prRoleDev);

				netif_tx_stop_all_queues(prRoleDev);
			}
		}

		if (netif_carrier_ok(prP2PInfo->prDevHandler))
			netif_carrier_off(prP2PInfo->prDevHandler);

		netif_tx_stop_all_queues(prP2PInfo->prDevHandler);

		/* Here are the functions which need rtnl_lock */
		if ((prRoleDev) && (prP2PInfo->prDevHandler != prRoleDev)) {
			DBGLOG(INIT, DEBUG, "unregister p2p[%d]\n", ucRoleIdx);

#if (KERNEL_VERSION(6, 0, 0) <= CFG80211_VERSION_CODE) && \
	(CFG_SUPPORT_802_11BE_MLO == 1)
			p2pNetUnregisterMldLinks(prGlueInfo, prRoleDev,
						 ucRoleIdx);
#endif

			if (prRoleDev->reg_state == NETREG_REGISTERED) {
				/* Kernel may lock wiphy again if UP flag still
				 * raised, force unset to avoid deadlock.
				 */
				if (prRoleDev->flags & IFF_UP) {
					DBGLOG(P2P, TRACE,
					       "unset role dev flag UP\n");
					if (!fgIsRtnlLockAcquired)
						rtnl_lock();
#if KERNEL_VERSION(5, 0, 0) <= CFG80211_VERSION_CODE
					dev_change_flags(prRoleDev,
					      prRoleDev->flags ^ IFF_UP, NULL);
#else
					dev_change_flags(prRoleDev,
					      prRoleDev->flags ^ IFF_UP);
#endif
					if (!fgIsRtnlLockAcquired)
						rtnl_unlock();
				}

				if (fgIsRtnlLockAcquired) {
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
					struct wireless_dev *ptr =
						prRoleDev->ieee80211_ptr;

					if (!fgIsWiphyLockHeld)
						wiphy_lock(ptr->wiphy);
					cfg80211_unregister_netdevice(
							prRoleDev);
					if (!fgIsWiphyLockHeld)
						wiphy_unlock(ptr->wiphy);
#else
					unregister_netdevice(prRoleDev);
#endif
				} else {
					unregister_netdev(prRoleDev);
				}
			}
			/* This ndev is created in mtk_p2p_cfg80211_add_iface(),
			 * and unregister_netdev will also free the ndev.
			 */
			prP2PInfo->aprRoleHandler = NULL;
		}

		DBGLOG(INIT, DEBUG, "unregister p2pdev[%d]\n", ucRoleIdx);
		if (prP2PInfo->prDevHandler->reg_state == NETREG_REGISTERED) {
			struct net_device *prDev;

			prDev = prP2PInfo->prDevHandler;
			prP2PInfo->prDevHandler = NULL;
			if (prDev == prRoleDev) {
				DBGLOG(INIT, DEBUG,
					"set p2p role as NULL too\n");
				prP2PInfo->aprRoleHandler = NULL;
			}

			/* Kernel may lock wiphy again if UP flag still
			 * raised, force unset to avoid deadlock.
			 */
			if (prDev->flags & IFF_UP) {
				DBGLOG(P2P, TRACE, "unset p2pdev flag UP\n");
				if (!fgIsRtnlLockAcquired)
					rtnl_lock();
#if KERNEL_VERSION(5, 0, 0) <= CFG80211_VERSION_CODE
				dev_change_flags(prDev, prDev->flags ^ IFF_UP,
						 NULL);
#else
				dev_change_flags(prDev, prDev->flags ^ IFF_UP);
#endif
				if (!fgIsRtnlLockAcquired)
					rtnl_unlock();
			}

			if (fgIsRtnlLockAcquired) {
#if KERNEL_VERSION(5, 12, 0) <= CFG80211_VERSION_CODE
				struct wireless_dev *ptr = prDev->ieee80211_ptr;

				if (!fgIsWiphyLockHeld)
					wiphy_lock(ptr->wiphy);
				cfg80211_unregister_netdevice(prDev);
				if (!fgIsWiphyLockHeld)
					wiphy_unlock(ptr->wiphy);
#else
				unregister_netdevice(prDev);
#endif
			} else {
				unregister_netdev(prDev);
			}

#if KERNEL_VERSION(4, 11, 9) > CFG80211_VERSION_CODE
			free_netdev(prDev);
#endif
		}
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	return TRUE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Setup the P2P device information
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *       [in] prP2pWdev       Pointer to the wireless device
 *       [in] prP2pDev        Pointer to the net device
 *       [in] u4Idx           The P2P Role index (max : (KAL_P2P_NUM-1))
 *       [in] fgIsApMode      Indicate that this device is AP Role or not
 *
 * \return    0	Success
 *           -1	Failure
 */
/*---------------------------------------------------------------------------*/
int glSetupP2P(struct GLUE_INFO *prGlueInfo, struct wireless_dev *prP2pWdev,
	struct net_device *prP2pDev, uint8_t u4Idx, u_int8_t fgIsApMode,
	u_int8_t fgSkipRole, uint8_t aucIntfMac[])
{
	struct ADAPTER *prAdapter = NULL;
	struct GL_P2P_INFO *prP2PInfo = NULL;
	struct GL_HIF_INFO *prHif = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = NULL;
	struct MSG_P2P_SWITCH_OP_MODE *prSwitchModeMsg;
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *prMldBss;
#endif
	uint8_t ucGroupMldId = MLD_GROUP_NONE;
	enum nl80211_iftype type;
	uint8_t ucBssIndex;

	GLUE_SPIN_LOCK_DECLARATION();

	DBGLOG(INIT, TRACE, "setup the p2p dev\n");

	if ((prGlueInfo == NULL) ||
	    (prP2pWdev == NULL) ||
	    (prP2pWdev->wiphy == NULL) ||
	    (prP2pDev == NULL)) {
		DBGLOG(INIT, ERROR, "parameter is NULL!!\n");
		return -1;
	}

	prHif = &prGlueInfo->rHifInfo;
	prAdapter = prGlueInfo->prAdapter;

	if ((prAdapter == NULL) ||
	    (prHif == NULL)) {
		DBGLOG(INIT, ERROR, "prAdapter/prHif is NULL!!\n");
		return -1;
	}

	/* FIXME: check KAL_P2P_NUM in trunk? */
	if (u4Idx >= KAL_P2P_NUM) {
		DBGLOG(INIT, ERROR, "u4Idx(%d) is out of range!!\n", u4Idx);
		return -1;
	}

	/*0. allocate p2pinfo */
	if (p2PAllocInfo(prGlueInfo, u4Idx) != TRUE) {
		DBGLOG(INIT, WARN, "Allocate memory for p2p FAILED\n");
		return -1;
	}

	prP2PInfo = prGlueInfo->prP2PInfo[u4Idx];

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	/* fill wiphy parameters */
	prP2PInfo->prWdev = prP2pWdev;
#endif /* CFG_ENABLE_WIFI_DIRECT_CFG_80211 */

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	/* setup netdev */
	/* Point to shared glue structure */
	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prP2pDev);
	prNetDevPriv->prGlueInfo = prGlueInfo;

	/* set ucIsP2p for P2P function device */
	if (fgIsApMode == TRUE) {
		prP2pWdev->iftype = NL80211_IFTYPE_AP;
		prNetDevPriv->ucIsP2p = FALSE;
#if CFG_MTK_MDDP_SUPPORT
		prNetDevPriv->ucMddpSupport = TRUE;
#else
		prNetDevPriv->ucMddpSupport = FALSE;
#endif
	} else {
		prP2pWdev->iftype = NL80211_IFTYPE_P2P_CLIENT;
		prNetDevPriv->ucIsP2p = TRUE;
		prNetDevPriv->ucMddpSupport = FALSE;
	}

	/* register callback functions */
	prP2pDev->needed_headroom = wlanGetTxNeededHeadRoom(prAdapter);
	prP2pDev->netdev_ops = &p2p_netdev_ops;

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	SET_NETDEV_DEV(prP2pDev, &(prHif->func->dev));
#endif
#endif

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	prP2pDev->ieee80211_ptr = prP2pWdev;
	prP2pWdev->netdev = prP2pDev;
#endif

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	/* set HW checksum offload */
	if (prAdapter->fgIsSupportCsumOffload) {
		prP2pDev->features |= NETIF_F_IP_CSUM |
				     NETIF_F_IPV6_CSUM |
				     NETIF_F_RXCSUM;
	}
#endif /* CFG_TCP_IP_CHKSUM_OFFLOAD */

	kalResetStats(prP2pDev);

	/* finish */
	/* bind netdev pointer to netdev index */
	prP2PInfo->prDevHandler = prP2pDev;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	/* XXX: All the P2P/AP devices do p2pDevFsmInit in the original code */
	ucBssIndex = p2pDevFsmInit(prAdapter, aucIntfMac);
	if (IS_BSS_INDEX_VALID(ucBssIndex)) {
		prNetDevPriv->ucBssIdx = ucBssIndex;
	} else {
		DBGLOG(INIT, WARN,
			"p2pDev bssindex=%d invalid\n",
			ucBssIndex);
		return -1;
	}

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_P2P_PENDING_MGMT_TX);
	LINK_INITIALIZE(&prP2PInfo->rWaitTxDoneLink);
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_P2P_PENDING_MGMT_TX);

	if ((fgSkipRole == SKIP_ROLE_ALL) ||
		((fgSkipRole == SKIP_ROLE_EXCEPT_MAIN) && u4Idx))
		goto exit;

	prP2PInfo->aprRoleHandler = prP2PInfo->prDevHandler;

	DBGLOG(P2P, INFO,
		"check prDevHandler = %p, aprRoleHandler = %p\n",
		prP2PInfo->prDevHandler, prP2PInfo->aprRoleHandler);

	/* setup running mode */
	p2pFuncInitConnectionSettings(prAdapter,
		prAdapter->rWifiVar.prP2PConnSettings[u4Idx], fgIsApMode);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prMldBss = p2pMldBssInit(prGlueInfo->prAdapter, aucIntfMac,
				 fgIsApMode);
	if (!prMldBss) {
		DBGLOG(P2P, ERROR, "Null prMldBss, fgIsApMode=%d\n",
			fgIsApMode);
		return -1;
	}
	ucGroupMldId = prMldBss->ucGroupMldId;
#endif
	prNetDevPriv->ucBssIdx = p2pRoleFsmInit(prAdapter, u4Idx,
		ucGroupMldId, aucIntfMac);
	if (prNetDevPriv->ucBssIdx == MAX_BSSID_NUM) {
		DBGLOG(P2P, ERROR, "p2pRoleFsmInit failed.\n");
#if (CFG_SUPPORT_802_11BE_MLO == 1)
		p2pMldBssUninit(prAdapter, prMldBss);
#endif
		return -1;
	}
	/* Currently wpasupplicant can't support create interface. */
	/* so initial the corresponding data structure here. */
	wlanBindBssIdxToNetInterface(prGlueInfo, prNetDevPriv->ucBssIdx,
					(void *) prP2PInfo->aprRoleHandler);
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	prNetDevPriv->ucMldBssIdx = prMldBss->ucGroupMldId;
#endif

	/* bind netdev pointer to netdev index */
#if 0
	wlanBindNetInterface(prGlueInfo, NET_DEV_P2P_IDX,
				(void *)prGlueInfo->prP2PInfo->prDevHandler);
#endif

	prSwitchModeMsg = (struct MSG_P2P_SWITCH_OP_MODE *) cnmMemAlloc(
				prGlueInfo->prAdapter, RAM_TYPE_MSG,
				sizeof(struct MSG_P2P_SWITCH_OP_MODE));
	if (prSwitchModeMsg) {
		prSwitchModeMsg->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
		prSwitchModeMsg->ucRoleIdx = u4Idx;
		type = prP2pWdev->iftype;
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
	} else {
		DBGLOG(P2P, ERROR, "Alloc prSwitchModeMsg msg failed.\n");
	}

exit:

	return 0;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Register for cfg80211 for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
u_int8_t glRegisterP2P(struct GLUE_INFO *prGlueInfo, const char *prDevName,
		const char *prDevName2, uint8_t ucApMode)
{
	struct ADAPTER *prAdapter = NULL;
	uint8_t rMacAddr[PARAM_MAC_ADDR_LEN];
	u_int8_t fgIsApMode = FALSE;
	uint8_t  ucRegisterNum = 1, i = 0;
	struct wireless_dev *prP2pWdev = NULL;
	struct net_device *prP2pDev = NULL;
	struct wireless_dev **pprP2pWdev = NULL;
	struct wiphy *prWiphy = NULL;
	const char *prSetDevName;
	u_int8_t fgSkipRole = SKIP_ROLE_NONE;

	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	if (ucApMode == RUNNING_P2P_NO_GROUP_MODE)
		fgSkipRole = SKIP_ROLE_EXCEPT_MAIN;
	else if (ucApMode == RUNNING_P2P_DEV_MODE)
		fgSkipRole = SKIP_ROLE_ALL;

	if ((ucApMode == RUNNING_DUAL_AP_MODE) ||
	    (ucApMode == RUNNING_P2P_AP_MODE) ||
	    (ucApMode == RUNNING_DUAL_P2P_MODE) ||
	    (ucApMode == RUNNING_P2P_DEV_MODE) ||
	    (ucApMode == RUNNING_P2P_NO_GROUP_MODE)) {
		ucRegisterNum = KAL_P2P_NUM;
	}

	do {
		if (ucApMode == RUNNING_P2P_AP_MODE) {
			if (i == 0) {
				prSetDevName = prDevName;
				fgIsApMode = FALSE;
			} else {
				prSetDevName = prDevName2;
				fgIsApMode = TRUE;
			}
		} else {
			/* RUNNING_AP_MODE
			 * RUNNING_DUAL_AP_MODE
			 * RUNNING_P2P_MODE
			 * RUNNING_DUAL_P2P_MODE
			 * RUNNING_P2P_DEV_MODE
			 * RUNNING_P2P_NO_GROUP_MODE
			 */
			prSetDevName = prDevName;

			if (ucApMode == RUNNING_P2P_MODE ||
				ucApMode == RUNNING_DUAL_P2P_MODE ||
				ucApMode == RUNNING_P2P_DEV_MODE ||
				ucApMode == RUNNING_P2P_NO_GROUP_MODE)
				fgIsApMode = FALSE;
			else
				fgIsApMode = TRUE;
		}

		pprP2pWdev = prGlueInfo->prP2pWdev;

		if (!pprP2pWdev[i])
			glP2pCreateWirelessDevice(prGlueInfo);

		if (!pprP2pWdev[i]) {
			DBGLOG(P2P, ERROR, "pprP2pWdev[%d] is NULL\n", i);
			return FALSE;
		}

		prP2pWdev = pprP2pWdev[i];

		/* Reset prP2pWdev for the issue that the prP2pWdev doesn't
		 * reset when the usb unplug/plug.
		 */
		prWiphy = prP2pWdev->wiphy;
		memset(prP2pWdev, 0, sizeof(struct wireless_dev));
		prP2pWdev->wiphy = prWiphy;

		/* allocate netdev */
#if KERNEL_VERSION(3, 17, 0) <= CFG80211_VERSION_CODE
		prP2pDev = alloc_netdev_mq(
					sizeof(struct NETDEV_PRIVATE_GLUE_INFO),
					prSetDevName, NET_NAME_PREDICTABLE,
					ether_setup, CFG_MAX_TXQ_NUM);
#else
		prP2pDev = alloc_netdev_mq(
					sizeof(struct NETDEV_PRIVATE_GLUE_INFO),
					prSetDevName,
					ether_setup, CFG_MAX_TXQ_NUM);
#endif
		if (!prP2pDev) {
			DBGLOG(INIT, WARN, "unable to allocate ndev for p2p\n");
			goto err_alloc_netdev;
		}

		if (fgSkipRole == SKIP_ROLE_ALL ||
		    (fgSkipRole == SKIP_ROLE_EXCEPT_MAIN && i != 0))
			COPY_MAC_ADDR(rMacAddr, prAdapter->rWifiVar
				.aucP2pDeviceAddress[i]);
		else
			COPY_MAC_ADDR(rMacAddr, prAdapter->rWifiVar
				.aucP2pInterfaceAddress[i]);

		DBGLOG(INIT, DEBUG,
			"Set p2p[%d] mac to " MACSTR " fgIsApMode(%d)\n",
			i, MAC2STR(rMacAddr), fgIsApMode);

#if KERNEL_VERSION(4, 11, 9) <= CFG80211_VERSION_CODE
		prP2pDev->needs_free_netdev = true;
#endif

#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		eth_hw_addr_set(prP2pDev, rMacAddr);
#else
		kalMemCopy(prP2pDev->dev_addr, rMacAddr, ETH_ALEN);
#endif
		kalMemCopy(prP2pDev->perm_addr, prP2pDev->dev_addr, ETH_ALEN);

		if (glSetupP2P(prGlueInfo, prP2pWdev, prP2pDev, i,
			       fgIsApMode, fgSkipRole, rMacAddr) != 0) {
			DBGLOG(INIT, WARN, "glSetupP2P[%u] FAILED\n", i);
			free_netdev(prP2pDev);
			return FALSE;
		}

		i++;
		/* prP2pInfo is alloc at glSetupP2P()->p2PAllocInfo() */
		prAdapter->prP2pInfo->u4DeviceNum++;

	} while (i < ucRegisterNum);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	/* set p2p net device register state */
	/* p2pNetRegister() will check prAdapter->rP2PNetRegState. */
	prAdapter->rP2PNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	return TRUE;

err_alloc_netdev:
	return FALSE;
}				/* end of glRegisterP2P() */

u_int8_t glP2pCreateWirelessDevice(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct wiphy *prWiphy = NULL;
	struct wireless_dev *prWdev = NULL;
	struct wireless_dev **pprOrigWdev = NULL;
	struct wireless_dev **pprP2pRoleWdev = NULL;
	struct wireless_dev **pprP2pWdev = NULL;
	uint8_t	i = 0;

	if (!prGlueInfo) {
		DBGLOG(P2P, ERROR, "prGlueInfo is NULL\n");
		return FALSE;
	}

	pprOrigWdev = wlanGetWirelessDevice(prGlueInfo);
	prWiphy = wlanGetWiphyByWdev(*pprOrigWdev);

	if (!prWiphy) {
		DBGLOG(P2P, ERROR, "unable to allocate wiphy for p2p\n");
		return FALSE;
	}

	pprP2pWdev = prGlueInfo->prP2pWdev;
	pprP2pRoleWdev = prGlueInfo->prP2pRoleWdev;

	for (i = 0 ; i < KAL_P2P_NUM; i++) {
		if (!pprP2pRoleWdev[i])
			break;
	}

	if (i >= KAL_P2P_NUM) {
		DBGLOG(INIT, WARN, "fail to register wiphy to driver\n");
		return FALSE;
	}

	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(P2P, ERROR, "allocate p2p wdev fail, no memory\n");
		return FALSE;
	}

	/* set priv as pointer to glue structure */
	prWdev->wiphy = prWiphy;

	pprP2pRoleWdev[i] = prWdev;
	DBGLOG(INIT, TRACE, "glP2pCreateWirelessDevice (%p)\n",
			pprP2pRoleWdev[i]->wiphy);

	/* P2PDev and P2PRole[0] share the same Wdev */
	pprP2pWdev[i] = pprP2pRoleWdev[i];

	return TRUE;
#else
	return FALSE;
#endif
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief Unregister Net Device for Wi-Fi Direct
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *	[in] ucIdx	     The BSS with the idx will be freed.
 *			     "ucIdx == 0xff" will free all BSSs.
 *			     Only has meaning for "CFG_ENABLE_UNIFY_WIPHY == 1"
 *
 * \return   TRUE
 *           FALSE
 */
/*---------------------------------------------------------------------------*/
u_int8_t glUnregisterP2P(struct GLUE_INFO *prGlueInfo, uint8_t ucIdx,
	uint8_t fgIsRtnlLockAcquired)
{
	uint8_t ucRoleIdx;
	struct ADAPTER *prAdapter;
	struct GL_P2P_INFO *prP2PInfo = NULL;
	struct wireless_dev **pprP2pRoleWdev = NULL;
	uint8_t ucStart = 0;
	uint8_t ucEnd = 0;
	GLUE_SPIN_LOCK_DECLARATION();

	ASSERT(prGlueInfo);

	if (ucIdx == 0xff) {
		ucStart = 0;
		ucEnd = BSS_P2P_NUM;
	} else if (ucIdx < BSS_P2P_NUM) {
		ucStart = ucIdx;
		ucEnd = ucIdx + 1;
	} else {
		DBGLOG(INIT, WARN, "The ucIdx (%d) is a wrong value\n", ucIdx);
		return FALSE;
	}

	prAdapter = prGlueInfo->prAdapter;
	pprP2pRoleWdev = prGlueInfo->prP2pRoleWdev;

	/* 4 <1> Uninit P2P dev FSM */
	/* Uninit P2P device FSM */
	/* only do p2pDevFsmUninit, when unregister all P2P device */
	if (ucIdx == 0xff)
		p2pDevFsmUninit(prAdapter);

	/* 4 <2> Uninit P2P role FSM */
	for (ucRoleIdx = ucStart; ucRoleIdx < ucEnd; ucRoleIdx++) {
		if (P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prAdapter, ucRoleIdx)) {
			/* FIXME: The cfg80211_XXX() is following the
			 * p2pRoleFsmUninit() sub-progress.
			 * ex: The cfg80211_del_sta() is called in the
			 *     kalP2PGOStationUpdate().
			 * But the netdev had be unregistered at
			 * p2pNetUnregister(). EXCEPTION!!
			 */
			p2pRoleFsmUninit(prAdapter, ucRoleIdx);
		}
	}

	/* 4 <3> Free Wiphy & netdev */
	for (ucRoleIdx = ucStart; ucRoleIdx < ucEnd; ucRoleIdx++) {
		GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIdx];

		if (prP2PInfo == NULL) {
			GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
			continue;
		}

		/* For P2P interfaces, prDevHandler points to the net_device of
		 * p2p0 interface. And aprRoleHandler points to the net_device
		 * of p2p virtual interface (i.e., p2p1) when it was created.
		 * And when p2p virtual interface is deleted, aprRoleHandler
		 * will change to point to prDevHandler. Hence, when
		 * aprRoleHandler & prDevHandler are pointing to different
		 * addresses, it means vif p2p1 exists. Otherwise it means p2p1
		 * was already deleted.
		 */
		if ((prP2PInfo->aprRoleHandler != NULL) &&
		    (prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)) {
			/* This device is added by the P2P, and use
			 * ndev->destructor to free. The p2pDevFsmUninit() use
			 * prP2PInfo->aprRoleHandler to do some check.
			 */
			prP2PInfo->aprRoleHandler = NULL;
			DBGLOG(P2P, INFO, "aprRoleHandler idx %d set NULL\n",
					ucRoleIdx);

			/* Expect that pprP2pRoleWdev[ucRoleIdx] has been reset
			 * as pprP2pWdev or NULL in p2pNetUnregister
			 * (unregister_netdev).
			 */
		}

		if (prP2PInfo->prDevHandler) {
			struct net_device *prDev;

			prDev = prP2PInfo->prDevHandler;
			prP2PInfo->prDevHandler = NULL;
			if (prDev == prP2PInfo->aprRoleHandler) {
				DBGLOG(INIT, INFO,
					"set p2p role as NULL too\n");
				prP2PInfo->aprRoleHandler = NULL;
			}

			/* don't free the dev that share with the AIS */
			if (wlanIsAisDev(prDev))
				pprP2pRoleWdev[ucRoleIdx] = NULL;
			else {
				if (prAdapter->rP2PNetRegState ==
					ENUM_NET_REG_STATE_REGISTERED) {
					DBGLOG(INIT, WARN,
						"Force unregister netdev\n");
					prAdapter->rP2PNetRegState =
					    ENUM_NET_REG_STATE_UNREGISTERING;
					GLUE_RELEASE_SPIN_LOCK(prGlueInfo,
						SPIN_LOCK_NET_DEV);
					if (fgIsRtnlLockAcquired)
						unregister_netdevice(prDev);
					else
						unregister_netdev(prDev);
					GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo,
						SPIN_LOCK_NET_DEV);
					prAdapter->rP2PNetRegState =
						ENUM_NET_REG_STATE_UNREGISTERED;

#if KERNEL_VERSION(4, 11, 9) > CFG80211_VERSION_CODE
					free_netdev(prDev);
#endif
				} else if (prAdapter->rP2PNetRegState !=
					ENUM_NET_REG_STATE_UNREGISTERED) {
					DBGLOG(P2P, WARN,
						"p2p dev[%u] not unregister done. net_state=%d\n",
						ucRoleIdx,
						prAdapter->rP2PNetRegState);
				}
			}
		}
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

		/* 4 <4> Free P2P internal memory */
		if (!p2PFreeInfo(prGlueInfo, ucRoleIdx)) {
			/* FALSE: (fgIsP2PRegistered!=FALSE)||(ucRoleIdx err) */
			DBGLOG(INIT, ERROR, "p2PFreeInfo FAILED\n");
			return FALSE;
		}

	}

	return TRUE;
}				/* end of glUnregisterP2P() */

/* Net Device Hooks */
/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device open (ifup)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pOpen(struct net_device *prDev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
#if CFG_SUPPORT_WED_PROXY
	uint32_t u4BufLen = 0;
#endif

	ASSERT(prDev);
	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	ASSERT(prGlueInfo);

#if CFG_SUPPORT_WED_PROXY
	kalIoctlByBssIdx(prGlueInfo, wlanoidWedAttachWarp, prDev,
		sizeof(struct net_device *), &u4BufLen, wlanGetBssIdx(prDev));
#endif

#if CFG_TX_GSO
	kalTxGsoInit(prDev);
#endif /* CFG_TX_GSO */

#if CFG_SW_TSO
	kalTxTsoSwInit(prDev);
#endif /* CFG_SW_TSO */

	/* 2. carrier on & start TX queue */
	/*DFS todo 20161220_DFS*/
#if (CFG_SUPPORT_DFS_MASTER == 1)
	if (prDev->ieee80211_ptr->iftype != NL80211_IFTYPE_AP) {
		/*netif_carrier_on(prDev);*/
		netif_tx_start_all_queues(prDev);
	}
#else
	/*netif_carrier_on(prDev);*/
	netif_tx_start_all_queues(prDev);
#endif

	/* init acs information */
	kalMemZero(&(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo),
		sizeof(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo));
	wlanInitChnLoadInfoChannelList(prGlueInfo->prAdapter);

#ifdef CONFIG_WIRELESS_EXT
	prDev->wireless_handlers = &wext_handler_def;
#endif

	return 0;		/* success */
}				/* end of p2pOpen() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A function for net_device stop (ifdown)
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \retval 0     The execution succeeds.
 * \retval < 0   The execution failed.
 */
/*----------------------------------------------------------------------------*/
static int p2pStop(struct net_device *prDev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	struct GL_P2P_DEV_INFO *prP2pGlueDevInfo = NULL;
/* P_MSG_P2P_FUNCTION_SWITCH_T prFuncSwitch; */

	GLUE_SPIN_LOCK_DECLARATION();
#endif
#if CFG_SUPPORT_WED_PROXY
	uint32_t u4BufLen = 0;
#endif

	ASSERT(prDev);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	/* XXX: The p2pStop may be triggered after the wlanRemove.	*/
	/*      And prGlueInfo->prP2PDevInfo is freed in p2PFreeInfo.	*/
	if (!prAdapter || !prAdapter->fgIsP2PRegistered)
		return -EFAULT;

#if CFG_ENABLE_WIFI_DIRECT_CFG_80211
	prP2pGlueDevInfo = prGlueInfo->prP2PDevInfo;
	ASSERT(prP2pGlueDevInfo);

	/* 0. Do the scan done and set parameter to abort if the scan pending */
	/*DBGLOG(INIT, DEBUG, "p2pStop and ucRoleIdx = %u\n", ucRoleIdx);*/
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if ((prP2pGlueDevInfo->prScanRequest != NULL) &&
	    (prP2pGlueDevInfo->prScanRequest->wdev == prDev->ieee80211_ptr)) {
		DBGLOG(INIT, DEBUG, "%s and abort scan!!\n", __func__);
		kalCfg80211ScanDone(prP2pGlueDevInfo->prScanRequest, TRUE);
		prP2pGlueDevInfo->prScanRequest = NULL;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
#endif
	/* zero clear old acs information */
	kalMemZero(&(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo),
		sizeof(prGlueInfo->prAdapter->rWifiVar.rChnLoadInfo));
	wlanInitChnLoadInfoChannelList(prGlueInfo->prAdapter);

	/* 1. stop TX queue */
	netif_tx_stop_all_queues(prDev);
#if 0
	/* 2. switch P2P-FSM off */
	/* 2.1 allocate for message */
	prFuncSwitch = (P_MSG_P2P_FUNCTION_SWITCH_T) cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG, sizeof(MSG_P2P_FUNCTION_SWITCH_T));

	if (!prFuncSwitch) {
		ASSERT(0);	/* Can't trigger P2P FSM */
		return -ENOMEM;
	}

	/* 2.2 fill message */
	prFuncSwitch->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
	prFuncSwitch->fgIsFuncOn = FALSE;

	/* 2.3 send message */
	mboxSendMsg(prAdapter,
		MBOX_ID_0,
		(struct MSG_HDR *) prFuncSwitch,
		MSG_SEND_METHOD_BUF);
#endif
	/* 3. stop queue and turn off carrier */
	/* TH3 multiple P2P */
	/*prGlueInfo->prP2PInfo[0]->eState = MEDIA_STATE_DISCONNECTED;*/

	netif_tx_stop_all_queues(prDev);
	if (netif_carrier_ok(prDev))
		netif_carrier_off(prDev);

#ifdef CONFIG_WIRELESS_EXT
	prDev->wireless_handlers = NULL;
#endif

#if CFG_SUPPORT_WED_PROXY
	if (kalIsHalted(prGlueInfo) == FALSE)
		kalIoctlByBssIdx(prGlueInfo, wlanoidWedDetachWarp, prDev,
			sizeof(struct net_device *), &u4BufLen,
			wlanGetBssIdx(prDev));
	else
		wlanoidWedDetachWarp(prGlueInfo->prAdapter, prDev,
			sizeof(struct net_device *), &u4BufLen);
#endif

	return 0;
}				/* end of p2pStop() */

/*---------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device,
 *        to get the network interface statistical
 *        information.
 *
 * Whenever an application needs to get statistics for the interface,
 * this method is called.
 * This happens, for example, when ifconfig or netstat -i is run.
 *
 * \param[in] prDev      Pointer to struct net_device.
 *
 * \return net_device_stats buffer pointer.
 */
/*---------------------------------------------------------------------------*/
struct net_device_stats *p2pGetStats(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
			netdev_priv(prDev);
	kalMemCopy(&prNetDevPrivate->stats, &prDev->stats,
			sizeof(struct net_device_stats));
#if CFG_MTK_MDDP_SUPPORT
	mddpGetMdStats(prDev);
#endif

	return (struct net_device_stats *) &prNetDevPrivate->stats;
}				/* end of p2pGetStats() */

static void p2pSetMulticastList(struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv;
	struct GLUE_INFO *prGlueInfo;

	if (!prDev)
		return;

	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDev);
	prGlueInfo = prNetDevPriv->prGlueInfo;
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag) {
		DBGLOG(REQ, WARN, "driver is not ready\n");
		return;
	}

	schedule_work(&(prNetDevPriv->workq));
}				/* p2pSetMulticastList */

static void p2pSetMulticastListWorkQueue(struct work_struct *work)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = CONTAINER_OF(work,
		struct NETDEV_PRIVATE_GLUE_INFO, workq);
	struct GLUE_INFO *prGlueInfo = prNetDevPriv->prGlueInfo;
	struct net_device *prDev;
	uint32_t u4SetInfoLen = 0, u4McCount;

	prDev = wlanGetNetDev(prGlueInfo, prNetDevPriv->ucBssIdx);
	if (!prDev) {
		DBGLOG(INIT, ERROR,
			"prDev for Bss%d not exist.\n",
			prNetDevPriv->ucBssIdx);
		return;
	}

	DBGLOG(INIT, TRACE, "Bss[%d] set multicast list, flags=0x%x\n",
		prNetDevPriv->ucBssIdx,
		prDev->flags);

	if (prDev->flags & IFF_PROMISC)
		prGlueInfo->prP2PDevInfo->u4PacketFilter
			|= PARAM_PACKET_FILTER_PROMISCUOUS;

	if (prDev->flags & IFF_BROADCAST)
		prGlueInfo->prP2PDevInfo->u4PacketFilter
			|= PARAM_PACKET_FILTER_BROADCAST;

	u4McCount = netdev_mc_count(prDev);
	if (prDev->flags & IFF_MULTICAST) {
		if ((prDev->flags & IFF_ALLMULTI)
			|| (u4McCount > MAX_NUM_GROUP_ADDR))
			prGlueInfo->prP2PDevInfo->u4PacketFilter
				|= PARAM_PACKET_FILTER_ALL_MULTICAST;
		else
			prGlueInfo->prP2PDevInfo->u4PacketFilter
				|= PARAM_PACKET_FILTER_MULTICAST;
	}

	if (prGlueInfo->prP2PDevInfo->u4PacketFilter
		& PARAM_PACKET_FILTER_MULTICAST) {
		/* Prepare multicast address list */
		struct PARAM_MULTICAST_LIST rMcAddrList;
		struct netdev_hw_addr *ha;
		uint32_t i = 0;

		/* Avoid race condition with kernel net subsystem */
		netif_addr_lock_bh(prDev);
		kalMemZero(&rMcAddrList, sizeof(rMcAddrList));

		netdev_for_each_mc_addr(ha, prDev) {
			/* If ha is null, it will break the loop. */
			/* Check mc count before accessing to ha to
			 * prevent from kernel crash.
			 */
			if (i == u4McCount || !ha)
				break;
			if (i < MAX_NUM_GROUP_ADDR) {
				COPY_MAC_ADDR(
					&rMcAddrList.aucMcAddrList[i],
					GET_ADDR(ha));
				i++;
			}
		}

		rMcAddrList.ucBssIdx = prNetDevPriv->ucBssIdx;
		rMcAddrList.ucAddrNum = i;
		rMcAddrList.fgIsOid = TRUE;

		netif_addr_unlock_bh(prDev);

		if (i >= MAX_NUM_GROUP_ADDR)
			return;

		kalIoctlByBssIdx(prGlueInfo,
				 wlanoidSetMulticastList,
				 &rMcAddrList,
				 sizeof(struct PARAM_MULTICAST_LIST),
				 &u4SetInfoLen,
				 prNetDevPriv->ucBssIdx);
	}
}

static netdev_tx_t __p2pHardStartXmit(struct GLUE_INFO *prGlueInfo,
	struct sk_buff *prSkb,
	struct net_device *prDev,
	uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;

	prBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (prBssInfo &&
	    prBssInfo->eNetworkType == NETWORK_TYPE_P2P &&
	    prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT &&
	    prBssInfo->fgIsApGoStarted == FALSE) {
		DBGLOG(P2P, WARN,
			"AP/GO (%u) is not started, skip this frame\n",
			ucBssIndex);
		dev_kfree_skb(prSkb);
		goto exit;
	}

	kalHardStartXmit(prSkb, prDev, prGlueInfo, ucBssIndex);

	if (prBssInfo &&
	    (prBssInfo->eConnectionState == MEDIA_STATE_CONNECTED ||
	     prBssInfo->rStaRecOfClientList.u4NumElem > 0)) {
		kalPerMonStart(prGlueInfo);
	}

exit:
	return NETDEV_TX_OK;
}

#if (CFG_SUPPORT_802_11BE_MLO == 1)
static netdev_tx_t __p2pMloHardStartXmit(struct GLUE_INFO *prGlueInfo,
	struct sk_buff *prSkb,
	struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate;
	struct MLD_BSS_INFO *prMldBss = NULL;
	struct MLD_STA_RECORD *prMldSta = NULL;
	struct ETH_FRAME *prEthFrame;
	u_int8_t fgFreeSkb = FALSE;

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDev);
	prEthFrame = (struct ETH_FRAME *)prSkb->data;
	prMldBss = mldBssGetByIdx(prGlueInfo->prAdapter,
				  prNetDevPrivate->ucMldBssIdx);
	prMldSta = mldStarecGetByMldAddr(prGlueInfo->prAdapter,
					 prMldBss,
					 prEthFrame->aucDestAddr);

	if (!prMldBss) {
		DBGLOG(P2P, ERROR, "Null prMldBss %u\n",
			prNetDevPrivate->ucMldBssIdx);
		fgFreeSkb = TRUE;
		goto exit;
	}

	if (IS_BMCAST_MAC_ADDR(prEthFrame->aucDestAddr)) {
#if (CFG_SUPPORT_MLO_GRP_FRAME_XMIT == 1)
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;
		struct sk_buff *prDupSkb = NULL;
		uint16_t u2Sn;

		u2Sn = mldBssGetGrpFrameSn(prGlueInfo->prAdapter, prMldBss);
		prBssList = &prMldBss->rBssList;

		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			if (!prTempBss->fgIsApGoStarted)
				continue;

			prDupSkb = skb_copy(prSkb, GFP_ATOMIC);
			if (!prDupSkb) {
				DBGLOG(P2P, ERROR,
					"[%u] duplicate skb failed.\n",
					prTempBss->ucBssIndex);
				break;
			}

			GLUE_SET_PKT_SN(prDupSkb, u2Sn);

			__p2pHardStartXmit(prGlueInfo,
					   prDupSkb,
					   prDev,
					   prTempBss->ucBssIndex);
		}
		fgFreeSkb = TRUE;
#else
		__p2pHardStartXmit(prGlueInfo,
				   prSkb,
				   prDev,
				   prNetDevPrivate->ucBssIdx);
#endif
	} else if (prMldSta) {
		struct STA_RECORD *prStarec;

		prStarec = cnmGetStaRecByIndex(prGlueInfo->prAdapter,
			secGetStaIdxByWlanIdx(prGlueInfo->prAdapter,
				prMldSta->u2SetupWlanId));
		if (!prStarec) {
			DBGLOG(P2P, ERROR,
				"get sta failed by wlan idx(%u).\n",
				prMldSta->u2SetupWlanId);
			fgFreeSkb = TRUE;
			goto exit;
		}

		__p2pHardStartXmit(prGlueInfo, prSkb, prDev,
				   prStarec->ucBssIndex);
	} else {
		struct LINK *prBssList;
		struct BSS_INFO *prTempBss;
		u_int8_t fgMatched = FALSE;

		prBssList = &prMldBss->rBssList;
		LINK_FOR_EACH_ENTRY(prTempBss, prBssList, rLinkEntryMld,
				    struct BSS_INFO) {
			struct STA_RECORD *prStaRec;

			prStaRec = cnmGetStaRecByAddress(prGlueInfo->prAdapter,
				prTempBss->ucBssIndex,
				prEthFrame->aucDestAddr);
			if (!prStaRec)
				continue;

			__p2pHardStartXmit(prGlueInfo,
					   prSkb,
					   prDev,
					   prTempBss->ucBssIndex);
			fgMatched = TRUE;
			break;
		}

		if (!fgMatched) {
			fgFreeSkb = TRUE;
			goto exit;
		}
	}

exit:
	if (fgFreeSkb)
		dev_kfree_skb(prSkb);
	return NETDEV_TX_OK;
}
#endif

/*---------------------------------------------------------------------------*/
/*!
 *  \brief This function is TX entry point of NET DEVICE.
 *
 *  \param[in] prSkb  Pointer of the sk_buff to be sent
 *  \param[in] prDev  Pointer to struct net_device
 *
 * \retval NETDEV_TX_OK - on success.
 * \retval NETDEV_TX_BUSY - on failure, packet will be discarded by upper layer.
 */
/*---------------------------------------------------------------------------*/
netdev_tx_t p2pHardStartXmit(struct sk_buff *prSkb,
	struct net_device *prDev)
{
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPrivate =
		(struct NETDEV_PRIVATE_GLUE_INFO *) NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	uint8_t ucBssIndex;

	ASSERT(prSkb);
	ASSERT(prDev);

	prNetDevPrivate = (struct NETDEV_PRIVATE_GLUE_INFO *)
		netdev_priv(prDev);
	prGlueInfo = prNetDevPrivate->prGlueInfo;
	ucBssIndex = prNetDevPrivate->ucBssIdx;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, WARN, "prGlueInfo is NULL\n");
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}

#if CFG_CHIP_RESET_SUPPORT
	if (!wlanIsDriverReady(prGlueInfo,
		(WLAN_DRV_READY_CHECK_RESET | WLAN_DRV_READY_CHECK_WLAN_ON))) {
		DBGLOG(INIT, WARN,
		"u4ReadyFlag:%u, kalIsResetting():%d, dropping the packet\n",
		prGlueInfo->u4ReadyFlag, kalIsResetting());
		dev_kfree_skb(prSkb);
		return NETDEV_TX_OK;
	}
#endif

	kalResetPacket(prGlueInfo, (void *) prSkb);

#if (CFG_SUPPORT_802_11BE_MLO == 1) && \
	(KERNEL_VERSION(5, 19, 2) <= CFG80211_VERSION_CODE)
	if (prDev->ieee80211_ptr->iftype == NL80211_IFTYPE_AP &&
	    prDev->ieee80211_ptr->valid_links)
		return __p2pMloHardStartXmit(prGlueInfo, prSkb, prDev);
#endif

	return __p2pHardStartXmit(prGlueInfo, prSkb, prDev, ucBssIndex);
}				/* end of p2pHardStartXmit() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief A method of struct net_device, a primary SOCKET interface to configure
 *        the interface lively. Handle an ioctl call on one of our devices.
 *        Everything Linux ioctl specific is done here.
 *        Then we pass the contents
 *        of the ifr->data to the request message handler.
 *
 * \param[in] prDev      Linux kernel netdevice
 *
 * \param[in] prIFReq    Our private ioctl request structure,
 *                       typed for the generic struct ifreq
 *                       so we can use ptr to function
 *
 * \param[in] cmd        Command ID
 *
 * \retval WLAN_STATUS_SUCCESS The IOCTL command is executed successfully.
 * \retval OTHER The execution of IOCTL command is failed.
 */
/*----------------------------------------------------------------------------*/

int p2pDoIOCTL(struct net_device *prDev, struct ifreq *prIfReq, int i4Cmd)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	int ret = 0;
	/* char *prExtraBuf = NULL; */
	/* UINT_32 u4ExtraSize = 0; */
	/* struct iwreq *prIwReq = (struct iwreq *)prIfReq; */
	/* struct iw_request_info rIwReqInfo; */

	ASSERT(prDev && prIfReq);

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (!prGlueInfo) {
		DBGLOG(P2P, ERROR, "prGlueInfo is NULL\n");
		return -EFAULT;
	}

	if (prGlueInfo->u4ReadyFlag == 0) {
		DBGLOG(P2P, ERROR, "Adapter is not ready\n");
		return -EINVAL;
	}

	if (i4Cmd == SIOCGIWPRIV) {
		ret = wext_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if ((i4Cmd >= SIOCIWFIRSTPRIV) && (i4Cmd < SIOCIWLASTPRIV)) {
		/* 0x8BE0 ~ 0x8BFF, private ioctl region */
		ret = priv_support_ioctl(prDev, prIfReq, i4Cmd);
	} else if (i4Cmd == SIOCDEVPRIVATE + 1) {
#ifdef CFG_ANDROID_AOSP_PRIV_CMD
		ret = android_private_support_driver_cmd(prDev, prIfReq, i4Cmd);
#else
		ret = priv_support_driver_cmd(prDev, prIfReq, i4Cmd);
#endif /* CFG_ANDROID_AOSP_PRIV_CMD */
	} else {
		DBGLOG(INIT, WARN, "Unexpected ioctl command: 0x%04x\n", i4Cmd);
		ret = -1;
	}

#if 0
	/* fill rIwReqInfo */
	rIwReqInfo.cmd = (__u16) i4Cmd;
	rIwReqInfo.flags = 0;

	switch (i4Cmd) {
	case SIOCSIWENCODEEXT:
		/* Set Encryption Material after 4-way handshaking is done */
		if (prIwReq->u.encoding.pointer) {
			u4ExtraSize = prIwReq->u.encoding.length;
			prExtraBuf = kalMemAlloc(u4ExtraSize, VIR_MEM_TYPE);

			if (!prExtraBuf) {
				ret = -ENOMEM;
				break;
			}

			if (copy_from_user(prExtraBuf,
				prIwReq->u.encoding.pointer,
				prIwReq->u.encoding.length))
				ret = -EFAULT;
		} else if (prIwReq->u.encoding.length != 0) {
			ret = -EINVAL;
			break;
		}

		if (ret == 0)
			ret = mtk_p2p_wext_set_key(prDev,
				&rIwReqInfo,
				&(prIwReq->u), prExtraBuf);

		kalMemFree(prExtraBuf, VIR_MEM_TYPE, u4ExtraSize);
		prExtraBuf = NULL;
		break;

	case SIOCSIWMLME:
		/* IW_MLME_DISASSOC used for disconnection */
		if (prIwReq->u.data.length != sizeof(struct iw_mlme)) {
			DBGLOG(INIT, DEBUG,
				"MLME buffer strange:%d\n",
				prIwReq->u.data.length);
			ret = -EINVAL;
			break;
		}

		if (!prIwReq->u.data.pointer) {
			ret = -EINVAL;
			break;
		}

		prExtraBuf = kalMemAlloc(sizeof(struct iw_mlme), VIR_MEM_TYPE);
		if (!prExtraBuf) {
			ret = -ENOMEM;
			break;
		}

		if (copy_from_user(prExtraBuf,
			prIwReq->u.data.pointer, sizeof(struct iw_mlme)))
			ret = -EFAULT;
		else
			ret = mtk_p2p_wext_mlme_handler(prDev,
				&rIwReqInfo, &(prIwReq->u), prExtraBuf);

		kalMemFree(prExtraBuf, VIR_MEM_TYPE, sizeof(struct iw_mlme));
		prExtraBuf = NULL;
		break;

	case SIOCGIWPRIV:
		/* This ioctl is used to list all IW privilege ioctls */
		ret = mtk_p2p_wext_get_priv(prDev,
			&rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case SIOCGIWSCAN:
		ret = mtk_p2p_wext_discovery_results(prDev,
			&rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case SIOCSIWAUTH:
		ret = mtk_p2p_wext_set_auth(prDev,
			&rIwReqInfo, &(prIwReq->u), NULL);
		break;

	case IOC_P2P_CFG_DEVICE:
	case IOC_P2P_PROVISION_COMPLETE:
	case IOC_P2P_START_STOP_DISCOVERY:
	case IOC_P2P_DISCOVERY_RESULTS:
	case IOC_P2P_WSC_BEACON_PROBE_RSP_IE:
	case IOC_P2P_CONNECT_DISCONNECT:
	case IOC_P2P_PASSWORD_READY:
	case IOC_P2P_GET_STRUCT:
	case IOC_P2P_SET_STRUCT:
	case IOC_P2P_GET_REQ_DEVICE_INFO:
#if 0
		ret = rP2PIwPrivHandler[i4Cmd - SIOCIWFIRSTPRIV](prDev,
				&rIwReqInfo,
				&(prIwReq->u),
				(char *)&(prIwReq->u));
#endif
		break;
#if CFG_SUPPORT_P2P_RSSI_QUERY
	case SIOCGIWSTATS:
		ret = mtk_p2p_wext_get_rssi(prDev,
			&rIwReqInfo, &(prIwReq->u), NULL);
		break;
#endif
	default:
		ret = -ENOTTY;
	}
#endif /* 0 */

	return ret;
}				/* end of p2pDoIOCTL() */

#if KERNEL_VERSION(5, 15, 0) <= CFG80211_VERSION_CODE
int p2pDoPrivIOCTL(struct net_device *prDev, struct ifreq *prIfReq,
		void __user *prData, int i4Cmd)
{
	return p2pDoIOCTL(prDev, prIfReq, i4Cmd);
}
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief To override p2p interface address
 *
 * \param[in] prDev Net device requested.
 * \param[in] addr  Pointer to address
 *
 * \retval 0 For success.
 * \retval -E2BIG For user's buffer size is too small.
 * \retval -EFAULT For fail.
 *
 */
/*---------------------------------------------------------------------------*/
int p2pSetMACAddress(struct net_device *prDev, void *addr)
{
	struct ADAPTER *prAdapter = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct sockaddr *sa = NULL;
	struct BSS_INFO *prBssInfo = NULL;
	struct BSS_INFO *prDevBssInfo = NULL;
	uint8_t ucRoleIdx = 0, ucDevIdx = 0, ucBssIdx = 0;
	u_int8_t fgIsNetDevFound = FALSE;
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
	u8 _addr[ETH_ALEN];
#endif

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	ASSERT(prGlueInfo);

	prAdapter = prGlueInfo->prAdapter;
	ASSERT(prAdapter);

	if (!prDev || !addr) {
		DBGLOG(INIT, ERROR, "Set macaddr with ndev(%d) and addr(%d)\n",
		       (prDev == NULL) ? 0 : 1, (addr == NULL) ? 0 : 1);
		return -EINVAL;
	}

	prDevBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
		prAdapter->ucP2PDevBssIdx);
	if (!prDevBssInfo) {
		DBGLOG(INIT, ERROR, "dev bss is not active\n");
		return -EINVAL;
	}

	sa = (struct sockaddr *)addr;

	if (mtk_Netdev_To_RoleIdx(prGlueInfo, prDev, &ucRoleIdx) != 0) {
		DBGLOG(INIT, WARN, "can't find the matched role\n");
		goto skip_role;
	}

	if (p2pFuncRoleToBssIdx(prGlueInfo->prAdapter,
		ucRoleIdx, &ucBssIdx) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "can't find the matched bss\n");
		goto skip_role;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(INIT, ERROR, "bss is not active\n");
		goto skip_role;
	}

	COPY_MAC_ADDR(prBssInfo->aucOwnMacAddr, sa->sa_data);
	COPY_MAC_ADDR(prAdapter->rWifiVar.aucP2pInterfaceAddress[ucRoleIdx],
		sa->sa_data);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mldBssUpdateMldAddrByMainBss(prAdapter,
		mldBssGetByBss(prAdapter, prBssInfo));
#endif
	fgIsNetDevFound = TRUE;
	DBGLOG(INIT, DEBUG,
		"[%u][%u] Set random macaddr to " MACSTR ".\n",
		ucBssIdx, ucRoleIdx,
		MAC2STR(prBssInfo->aucOwnMacAddr));

skip_role:
	if (mtk_Netdev_To_DevIdx(prGlueInfo, prDev, &ucDevIdx) == 0) {
		COPY_MAC_ADDR(prAdapter->rWifiVar.aucP2pDeviceAddress[ucDevIdx],
			sa->sa_data);
		COPY_MAC_ADDR(prDevBssInfo->aucOwnMacAddr, sa->sa_data);
		COPY_MAC_ADDR(prDevBssInfo->aucBSSID, sa->sa_data);
		fgIsNetDevFound = TRUE;
		DBGLOG(INIT, DEBUG,
			"[%d][%d] Set dev random macaddr to " MACSTR ".\n",
			ucBssIdx, ucDevIdx,
			MAC2STR(prDevBssInfo->aucOwnMacAddr));
	}

	if (fgIsNetDevFound) {
#if (KERNEL_VERSION(5, 16, 0) <= LINUX_VERSION_CODE)
		ether_addr_copy(_addr, sa->sa_data);
		eth_hw_addr_set(prDev, _addr);
#else
		COPY_MAC_ADDR(prDev->dev_addr, sa->sa_data);
#endif
	} else {
		DBGLOG(INIT, WARN,
			"Unmatch net_device %s, new " MACSTR " not set.\n",
			prDev->name, MAC2STR(sa->sa_data));
	}

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_ENABLE_WIFI_DIRECT */

