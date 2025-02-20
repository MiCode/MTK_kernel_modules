// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "platform.c"
 *    \brief  This file including the protocol layer privacy function.
 *
 *    This file provided the macros and functions library support for the
 *    protocol layer security setting from wlan_oid.c and for parse.c and
 *    rsn.c and nic_privacy.c
 *
 */

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/uaccess.h>
#include "precomp.h"
#include "gl_os.h"

#if CFG_ENABLE_EARLY_SUSPEND
#include <linux/earlysuspend.h>
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define WIFI_NVRAM_FILE_NAME   "/data/nvram/APCFG/APRDEB/WIFI"
#define WIFI_NVRAM_CUSTOM_NAME "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM"

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
static const char *const apucDebugNetdevState[] = {
	"NETDEV_UNKNOWN",
	"NETDEV_UP",
	"NETDEV_DOWN",
	"NETDEV_REBOOT",
	"NETDEV_CHANGE",
	"NETDEV_REGISTER",
	"NETDEV_UNREGISTER",
	"NETDEV_CHANGEMTU",
	"NETDEV_CHANGEADDR",
#if (KERNEL_VERSION(5, 0, 0) <= CFG80211_VERSION_CODE)
	"NETDEV_PRE_CHANGEADDR",
#endif
	"NETDEV_GOING_DOWN",
	"NETDEV_CHANGENAME",
	"NETDEV_FEAT_CHANGE",
	"NETDEV_BONDING_FAILOVER",
	"NETDEV_PRE_UP",
	"NETDEV_PRE_TYPE_CHANGE",
	"NETDEV_POST_TYPE_CHANGE",
	"NETDEV_POST_INIT",
#if (KERNEL_VERSION(4, 17, 0) > CFG80211_VERSION_CODE)
	"NETDEV_UNREGISTER_FINAL",
#endif
	"NETDEV_RELEASE",
	"NETDEV_NOTIFY_PEERS",
	"NETDEV_JOIN",
	"NETDEV_CHANGEUPPER",
	"NETDEV_RESEND_IGMP",
	"NETDEV_PRECHANGEMTU",
	"NETDEV_CHANGEINFODATA",
	"NETDEV_BONDING_INFO",
	"NETDEV_PRECHANGEUPPER",
	"NETDEV_CHANGELOWERSTATE",
	"NETDEV_UDP_TUNNEL_PUSH_INFO",
	"NETDEV_UNKNOWN",
	"NETDEV_CHANGE_TX_QUEUE_LEN",
#if (KERNEL_VERSION(4, 17, 0) <= CFG80211_VERSION_CODE)
	"NETDEV_CVLAN_FILTER_PUSH_INFO",
	"NETDEV_CVLAN_FILTER_DROP_INFO",
	"NETDEV_SVLAN_FILTER_PUSH_INFO",
	"NETDEV_SVLAN_FILTER_DROP_INFO",
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
static int wlan_netdev_notifier_call(struct notifier_block *nb,
		unsigned long state, void *ndev);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if 1
static int netdev_event(struct notifier_block *nb,
			unsigned long notification, void *ptr)
{
	struct in_ifaddr *ifa = (struct in_ifaddr *)ptr;
	struct net_device *prDev = ifa->ifa_dev->dev;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (prDev == NULL) {
		/* DBGLOG(REQ, INFO, ("netdev_event: device is empty.\n")); */
		return NOTIFY_DONE;
	}

	if ((strncmp(prDev->name, "p2p", 3) != 0)
#ifdef CFG_COMBO_SLT_GOLDEN
	    && (strncmp(prDev->name, "ra", 2) != 0)
#endif
#if CFG_SUPPORT_NAN
		&& (strncmp(prDev->name, "aware", 5) != 0)
#endif
	    && (strncmp(prDev->name, "wlan", 4) != 0)) {
		/* DBGLOG(REQ, INFO, ("netdev_event: xxx\n")); */
		return NOTIFY_DONE;
	}
#if 0				/* CFG_SUPPORT_PASSPOINT */
	{
		prGlueInfo->fgIsDad = FALSE;
	}
#endif /* CFG_SUPPORT_PASSPOINT */

	prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	if (prGlueInfo == NULL) {
		DBGLOG(REQ, INFO, "netdev_event: prGlueInfo is empty.\n");
		return NOTIFY_DONE;
	}

	if (prGlueInfo->fgIsInSuspendMode == FALSE) {
		/* DBGLOG(REQ, INFO,
		 *  ("netdev_event: MEDIA_STATE_DISCONNECTED. (%d)\n",
		 * prGlueInfo->eParamMediaStateIndicated));
		 */
		/* return NOTIFY_DONE; */
	}

	DBGLOG(REQ, INFO, "netdev_event: set net addr\n");
	kalSetNetAddressFromInterface(prGlueInfo, prDev, TRUE);

	return NOTIFY_DONE;

}
#endif
#if 0				/* CFG_SUPPORT_PASSPOINT */
static int net6dev_event(struct notifier_block *nb,
			 unsigned long notification, void *ptr)
{
	struct inet6_ifaddr *ifa = (struct inet6_ifaddr *)ptr;
	struct net_device *prDev = ifa->idev->dev;
	struct GLUE_INFO *prGlueInfo = NULL;

	if (prDev == NULL) {
		DBGLOG(REQ, INFO, "net6dev_event: device is empty.\n");
		return NOTIFY_DONE;
	}

	if ((strncmp(prDev->name, "p2p", 3) != 0)
#ifdef CFG_COMBO_SLT_GOLDEN
	    && (strncmp(prDev->name, "ra", 2) != 0)
#endif
	    && (strncmp(prDev->name, "wlan", 4) != 0)) {
		DBGLOG(REQ, INFO, "net6dev_event: xxx\n");
		return NOTIFY_DONE;
	}

	if (strncmp(prDev->name, "p2p", 3) == 0) {
		/* because we store the address of prGlueInfo in p2p's private
		 * date of net device
		 */
		/* *((P_GLUE_INFO_T *) netdev_priv(
		 *        prGlueInfo->prP2PInfo[0]->prDevHandler)) = prGlueInfo;
		 */
		prGlueInfo = *((struct GLUE_INFO **) netdev_priv(prDev));
	} else {		/* wlan0 */
		prGlueInfo = (struct GLUE_INFO *) netdev_priv(prDev);
	}

	if (prGlueInfo == NULL) {
		DBGLOG(REQ, INFO, "netdev_event: prGlueInfo is empty.\n");
		return NOTIFY_DONE;
	}

	prGlueInfo->fgIs6Dad = FALSE;

	return NOTIFY_DONE;
}
#endif /* CFG_SUPPORT_PASSPOINT */

#if 1       /* unused  */
static struct notifier_block inetaddr_notifier = {
	.notifier_call = netdev_event,
};
#endif

#if 0				/* CFG_SUPPORT_PASSPOINT */
static struct notifier_block inet6addr_notifier = {
	.notifier_call = net6dev_event,
};
#endif /* CFG_SUPPORT_PASSPOINT */

void wlanRegisterInetAddrNotifier(void)
{
#if CFG_ENABLE_NET_DEV_NOTIFY

	register_inetaddr_notifier(&inetaddr_notifier);
#if 0				/* CFG_SUPPORT_PASSPOINT */
	register_inet6addr_notifier(&inet6addr_notifier);
#endif /* CFG_SUPPORT_PASSPOINT */

#endif
}

void wlanUnregisterInetAddrNotifier(void)
{
#if CFG_ENABLE_NET_DEV_NOTIFY

	unregister_inetaddr_notifier(&inetaddr_notifier);
#if 0				/* CFG_SUPPORT_PASSPOINT */
	unregister_inetaddr_notifier(&inet6addr_notifier);
#endif /* CFG_SUPPORT_PASSPOINT */

#endif
}

#if CFG_ENABLE_EARLY_SUSPEND
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will register platform driver to os
 *
 * \param[in] wlanSuspend    Function pointer to platform suspend function
 * \param[in] wlanResume   Function pointer to platform resume   function
 *
 * \return The result of registering earlysuspend
 */
/*----------------------------------------------------------------------------*/

int glRegisterEarlySuspend(struct early_suspend *prDesc,
			   early_suspend_callback wlanSuspend,
			   late_resume_callback wlanResume)
{
	int ret = 0;

	if (wlanSuspend != NULL)
		prDesc->suspend = wlanSuspend;
	else {
		DBGLOG(REQ, INFO,
		       "glRegisterEarlySuspend wlanSuspend ERROR.\n");
		ret = -1;
	}

	if (wlanResume != NULL)
		prDesc->resume = wlanResume;
	else {
		DBGLOG(REQ, INFO,
		       "glRegisterEarlySuspend wlanResume ERROR.\n");
		ret = -1;
	}

	register_early_suspend(prDesc);
	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will un-register platform driver to os
 *
 * \return The result of un-registering earlysuspend
 */
/*----------------------------------------------------------------------------*/

int glUnregisterEarlySuspend(struct early_suspend *prDesc)
{
	int ret = 0;

	unregister_early_suspend(prDesc);

	prDesc->suspend = NULL;
	prDesc->resume = NULL;

	return ret;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief API for reading data on NVRAM with flexible length.
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 *           len
 * \param[out]
 *           pu2Data
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCfgDataRead(struct GLUE_INFO *prGlueInfo,
			uint32_t u4Offset,
			ssize_t len, uint16_t *pu2Data)
{
	if (pu2Data == NULL)
		return FALSE;

	if (u4Offset + len > MAX_CFG_FILE_WIFI_REC_SIZE)
		return FALSE;

	kalMemCopy(pu2Data, &g_aucNvram[u4Offset], len);
	return TRUE;
#if 0
	if (nvram_read(WIFI_NVRAM_FILE_NAME,
		       (char *)pu2Data, len, u4Offset) != len) {
		return FALSE;
	} else {
		return TRUE;
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief API for reading data on NVRAM with 2 bytes fixed length.
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 * \param[out]
 *           pu2Data
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCfgDataRead16(struct GLUE_INFO *prGlueInfo,
			  uint32_t u4Offset, uint16_t *pu2Data)
{
	if (pu2Data == NULL)
		return FALSE;

	if (u4Offset + sizeof(unsigned short) > MAX_CFG_FILE_WIFI_REC_SIZE)
		return FALSE;

	kalMemCopy(pu2Data, &g_aucNvram[u4Offset],
		sizeof(unsigned short));
	return TRUE;
#if 0
	if (nvram_read(WIFI_NVRAM_FILE_NAME,
		       (char *)pu2Data, sizeof(unsigned short),
		       u4Offset) != sizeof(unsigned short)) {
		return FALSE;
	} else {
		return TRUE;
	}
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief API for writing data on NVRAM with 2 bytes fixed length.
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 *           u2Data
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCfgDataWrite16(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Offset, uint16_t u2Data)
{
	if (u4Offset + sizeof(unsigned short) > MAX_CFG_FILE_WIFI_REC_SIZE)
		return FALSE;

	kalMemCopy(&g_aucNvram[u4Offset], &u2Data,
		sizeof(unsigned short));
	return TRUE;
#if 0
	if (nvram_write(WIFI_NVRAM_FILE_NAME,
			(char *)&u2Data, sizeof(unsigned short),
			u4Offset) != sizeof(unsigned short)) {
		return FALSE;
	} else {
		return TRUE;
	}
#endif
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief API for writing data on NVRAM with 1 bytes fixed length.
 *
 * \param[in]
 *           prGlueInfo
 *           u4Offset
 *           u1Data
 * \return
 *           TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/

u_int8_t kalCfgDataWrite8(struct GLUE_INFO *prGlueInfo,
			   uint32_t u4Offset, uint8_t u1Data)
{
	if (u4Offset + sizeof(unsigned char) > MAX_CFG_FILE_WIFI_REC_SIZE)
		return FALSE;

	kalMemCopy(&g_aucNvram[u4Offset], &u1Data,
		sizeof(unsigned char));
	return TRUE;

}

static int wlan_netdev_notifier_call(struct notifier_block *nb,
		unsigned long state, void *ndev)
{
#if KERNEL_VERSION(3, 11, 0) <= CFG80211_VERSION_CODE
	struct netdev_notifier_info *dev_notif_info = ndev;
	struct net_device *dev = dev_notif_info != NULL ?
			dev_notif_info->dev : NULL;
#else
	struct net_device *dev = ndev;
#endif

	if (!dev)
		return NOTIFY_DONE;

	if ((strncmp(dev->name, "wlan", 4) != 0) &&
#ifdef CFG_COMBO_SLT_GOLDEN
			(strncmp(dev->name, "ra", 2) != 0) &&
#endif
#if CFG_SUPPORT_NAN
			(strncmp(dev->name, "aware", 5) != 0) &&
#endif
			(strncmp(dev->name, "p2p", 3) != 0) &&
			(strncmp(dev->name, "ap", 2) != 0)) {
		return NOTIFY_DONE;
	}

	DBGLOG(REQ, TRACE, "%s's new state: %lu %s.\n",
			dev->name, state, apucDebugNetdevState[state]);

	return NOTIFY_DONE;
}

static struct notifier_block wlan_netdev_notifier = {
	.notifier_call = wlan_netdev_notifier_call,
};

void wlanRegisterNetdevNotifier(void)
{
	register_netdevice_notifier(&wlan_netdev_notifier);
}

void wlanUnregisterNetdevNotifier(void)
{
	unregister_netdevice_notifier(&wlan_netdev_notifier);
}
