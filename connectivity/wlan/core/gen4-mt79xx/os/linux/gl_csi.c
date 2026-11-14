// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Id: @(#) gl_csi.c@@
 */

/*! \file   gl_csi.c
 *    \brief  Main routines of Linux driver interface for CSI
 *
 *    This file contains the main routines of Linux driver for MediaTek Inc.
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

#include <linux/poll.h>

#include <linux/kmod.h>

#include "precomp.h"
#include "debug.h"
#include "gl_os.h"
#include "gl_wext.h"
#include "wlan_lib.h"

#include "gl_cfg80211.h"
#include "gl_vendor.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CSI_INF_NAME "csi%d"

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
static uint8_t *csiifname = CSI_INF_NAME;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/* Net Device Hooks */
static int csiOpen(struct net_device *prDev);

static int csiStop(struct net_device *prDev);

static struct net_device_stats *csiGetStats(struct net_device *prDev);

static int csiInit(struct net_device *prDev);

static void csiUninit(struct net_device *prDev);

const struct net_device_ops csi_netdev_ops = {
	.ndo_open = csiOpen,
	.ndo_stop = csiStop,
	.ndo_get_stats = csiGetStats,
	.ndo_init = csiInit,
	.ndo_uninit = csiUninit,
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static int
csiOpen(struct net_device *prDev)
{
	if (!prDev) {
		DBGLOG(CSI, ERROR, "prDev error!\n");
		return -EFAULT;
	}

	if (!netif_carrier_ok(prDev))
		netif_carrier_on(prDev);

	return 0; /* success */
}

static int
csiStop(struct net_device *prDev)
{
	if (!prDev) {
		DBGLOG(CSI, ERROR, "prDev error!\n");
		return -EFAULT;
	}

	if (netif_carrier_ok(prDev))
		netif_carrier_off(prDev);

	return 0; /* success */
}

struct net_device_stats *csiGetStats(struct net_device *prDev)
{
	return (struct net_device_stats *)kalGetStats(prDev);
}				/* end of csiGetStats() */

static int
csiInit(struct net_device *prDev)
{
	if (!prDev)
		return -ENXIO;

	return 0; /* success */
}

static void
csiUninit(struct net_device *prDev)
{
}

u_int8_t csiFreeInfo(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (prGlueInfo->prCSIDevInfo != NULL) {
		kalMemFree(prGlueInfo->prCSIDevInfo, VIR_MEM_TYPE,
			   sizeof(struct _GL_CSI_INFO_T *));
		prGlueInfo->prCSIDevInfo = NULL;
	}

	return TRUE;
}

u_int8_t csiNetUnregister(struct GLUE_INFO *prGlueInfo)
{
	u_int8_t fgDoUnregister = FALSE;

	GLUE_SPIN_LOCK_DECLARATION();

	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prAdapter) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}
	if (!prGlueInfo->prCSIDevInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prCSIDevInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prCSIDevInfo->prDevHandler) {
		DBGLOG(CSI, ERROR,
			"prGlueInfo->prCSIDevInfo->prDevHandler error\n");
		return FALSE;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	switch (prGlueInfo->prAdapter->rCsiNetRegState) {
	case ENUM_NET_REG_STATE_UNREGISTERED:
		DBGLOG(CSI, INFO, "CSI Net is already Unregistered\n");
		break;

	case ENUM_NET_REG_STATE_REGISTERED:
		prGlueInfo->prAdapter->rCsiNetRegState =
			ENUM_NET_REG_STATE_UNREGISTERING;
		fgDoUnregister = TRUE;
		break;

	default:
		DBGLOG(CSI, ERROR,
			"CSI Net is not REGISTERED when doing Unregister\n");
		break;
	}

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoUnregister)
		return FALSE;

	if (netif_carrier_ok(prGlueInfo->prCSIDevInfo->prDevHandler))
		netif_carrier_off(prGlueInfo->prCSIDevInfo->prDevHandler);

	netif_tx_stop_all_queues(prGlueInfo->prCSIDevInfo->prDevHandler);

	unregister_netdev(prGlueInfo->prCSIDevInfo->prDevHandler);
	DBGLOG(INIT, INFO, "unregister csidev\n");

	prGlueInfo->prAdapter->rCsiNetRegState =
		ENUM_NET_REG_STATE_UNREGISTERED;

	return TRUE;
}

u_int8_t csiNetRegister(struct GLUE_INFO *prGlueInfo)
{
	unsigned char fgDoRegister = FALSE;
	unsigned char ret;

	GLUE_SPIN_LOCK_DECLARATION();

	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prAdapter) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}
	if (!prGlueInfo->prCSIDevInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prCSIDevInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prCSIDevInfo->prDevHandler) {
		DBGLOG(CSI, ERROR,
			"prGlueInfo->prCSIDevInfo->prDevHandler error\n");
		return FALSE;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	switch (prGlueInfo->prAdapter->rCsiNetRegState) {
	case ENUM_NET_REG_STATE_REGISTERED:
		DBGLOG(CSI, INFO, "CSI Net is already Registered\n");
		break;

	case ENUM_NET_REG_STATE_UNREGISTERED:
		prGlueInfo->prAdapter->rCsiNetRegState =
			ENUM_NET_REG_STATE_REGISTERING;
		fgDoRegister = TRUE;
		break;

	default:
		DBGLOG(CSI, ERROR,
			"CSI Net is not UNREGISTERED when doing Register\n");
		break;
	}

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoRegister)
		return FALSE;

	ret = TRUE;
	/* net device initialize */
	netif_carrier_off(
		prGlueInfo->prCSIDevInfo->prDevHandler);
	netif_tx_stop_all_queues(
		prGlueInfo->prCSIDevInfo->prDevHandler);

	/* register for net device */
	if (register_netdev(
		prGlueInfo->prCSIDevInfo->prDevHandler) < 0) {
		DBGLOG(INIT, WARN,
			"unable to register netdevice for csi\n");
		/* trunk doesn't do free_netdev here */
		free_netdev(
			prGlueInfo->prCSIDevInfo->prDevHandler);

		ret = FALSE;
	} else {
		prGlueInfo->prAdapter->rCsiNetRegState =
			ENUM_NET_REG_STATE_REGISTERED;
		ret = TRUE;
	}

	return ret;
}

u_int8_t csiAllocInfo(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (prGlueInfo->prCSIDevInfo == NULL) {
		/* alloc memory for CSIDEV info */
		prGlueInfo->prCSIDevInfo = kalMemAlloc(
			sizeof(struct _GL_CSI_INFO_T), VIR_MEM_TYPE);
		if (prGlueInfo->prCSIDevInfo) {
			kalMemZero(prGlueInfo->prCSIDevInfo,
				   sizeof(struct _GL_CSI_INFO_T));
		} else {
			DBGLOG(CSI, INFO, "alloc prCSIDevInfo fail\n");
			goto err_alloc;
		}
	}
	return TRUE;

err_alloc:

	if (prGlueInfo->prCSIDevInfo) {
		kalMemFree(prGlueInfo->prCSIDevInfo, VIR_MEM_TYPE,
		   sizeof(struct _GL_CSI_INFO_T));

		prGlueInfo->prCSIDevInfo = NULL;
	}

	return FALSE;
}

int glSetupCSI(struct GLUE_INFO *prGlueInfo, struct wireless_dev *prCsiWdev,
	   struct net_device *prCsiDev)
{
	struct _GL_CSI_INFO_T *prCSIInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = NULL;
#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	struct GL_HIF_INFO *prHif = NULL;
#endif
#endif

	DBGLOG(INIT, INFO, "setup the csi dev\n");

	if ((prGlueInfo == NULL) || (prCsiWdev == NULL) ||
	    (prCsiWdev->wiphy == NULL) || (prCsiDev == NULL)) {
		DBGLOG(INIT, ERROR, "parameter is NULL!!\n");
		return -1;
	}

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	prHif = &prGlueInfo->rHifInfo;

	if (prHif == NULL) {
		DBGLOG(INIT, ERROR, "prHif is NULL!!\n");
		return -1;
	}
#endif
#endif

	/* 0. allocate csiinfo */
	if (csiAllocInfo(prGlueInfo) != TRUE) {
		DBGLOG(INIT, WARN, "Allocate memory for csi FAILED\n");
		return -1;
	}

	prCSIInfo = prGlueInfo->prCSIDevInfo;

	/* setup netdev */
	/* Point to shared glue structure */
	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)netdev_priv(prCsiDev);
	prNetDevPriv->prGlueInfo = prGlueInfo;

	/* set ucCSI for CSI function device */
	prCsiWdev->iftype = NL80211_IFTYPE_STATION;

	prNetDevPriv->fgIsCsi = TRUE;
	/* register callback functions */
	prCsiDev->needed_headroom += NIC_TX_HEAD_ROOM;
	prCsiDev->netdev_ops = &csi_netdev_ops;

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	SET_NETDEV_DEV(prCsiDev, &(prHif->func->dev));
#endif
#endif
	prCsiDev->ieee80211_ptr = prCsiWdev;
	prCsiWdev->netdev = prCsiDev;

	kalResetStats(prCsiDev);

	/* finish
	 * bind netdev pointer to netdev index
	 */
	prCSIInfo->prDevHandler = prCsiDev;
	DBGLOG(INIT, INFO, "setup the csi dev\n");

	return 0;
}

u_int8_t glCsiCreateWirelessDevice(struct GLUE_INFO *prGlueInfo)
{
	/* whsu, KAL_AIS_NUM at gprWdev */
	struct wireless_dev **pprOrigWdev = wlanGetWirelessDevice(prGlueInfo);
	struct wiphy *prWiphy = wlanGetWiphy(*pprOrigWdev);
	struct wireless_dev *prWdev = NULL;

	if (!prWiphy) {
		DBGLOG(CSI, ERROR, "unable to allocate wiphy for CSI\n");
		return FALSE;
	}

	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(CSI, ERROR, "allocate csi wdev fail, no memory\n");
		return FALSE;
	}

	/* set priv as pointer to glue structure */
	prWdev->wiphy = prWiphy;

	prGlueInfo->prCsiRoleWdev = prWdev;
	DBGLOG(CSI, INFO, "glCsiCreateWirelessDevice (%x)\n",
	       prGlueInfo->prCsiRoleWdev->wiphy);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Unregister Net Device for CSI
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t glUnregisterCSI(struct GLUE_INFO *prGlueInfo)
{
	struct _GL_CSI_INFO_T *prCSIInfo = NULL;

	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error!\n");
		return FALSE;
	}

	/* 4 <3> Free Wiphy & netdev */
	prCSIInfo = prGlueInfo->prCSIDevInfo;
	if (prCSIInfo == NULL)
		return TRUE;

	/* 4 <4> Free CSI internal memory */
	if (!csiFreeInfo(prGlueInfo)) {
		DBGLOG(INIT, ERROR, "csiFreeInfo FAILED\n");
		return FALSE;
	}

	return TRUE;
} /* end of glUnregisterCSI() */

u_int8_t glRegisterCSI(struct GLUE_INFO *prGlueInfo, const char *prDevName)
{
	struct ADAPTER *prAdapter = NULL;
	struct wireless_dev *prCsiWdev = NULL;
	struct net_device *prCsiDev = NULL;
	struct wiphy *prWiphy = NULL;
	const char *prSetDevName;

	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error!\n");
		return FALSE;
	}
	if (!prDevName) {
		DBGLOG(CSI, ERROR, "prDevName error!\n");
		return FALSE;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(CSI, ERROR, "prAdapter error!\n");
		return FALSE;
	}

	glCsiCreateWirelessDevice(prGlueInfo);

	if (!prGlueInfo->prCsiRoleWdev) {
		DBGLOG(INIT, ERROR, "prGlueInfo->prCsiRoleWdev is NULL\n");
		return FALSE;
	}

	DBGLOG(INIT, INFO, "prCsiWdev\n");
	prCsiWdev = prGlueInfo->prCsiRoleWdev;

	/* Reset prCsiWdev for the issue that the prCsiWdev doesn't
	 * reset when the usb unplug/plug.
	 */
	prWiphy = prCsiWdev->wiphy;
	memset(prCsiWdev, 0, sizeof(struct wireless_dev));
	prCsiWdev->wiphy = prWiphy;

	prSetDevName = prDevName;

    /* allocate netdev */
#if KERNEL_VERSION(3, 17, 0) <= CFG80211_VERSION_CODE
	prCsiDev = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
			NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);
#else
	prCsiDev = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
			ether_setup, CFG_MAX_TXQ_NUM);
#endif
	if (!prCsiDev) {
		DBGLOG(INIT, WARN, "unable to allocate ndev for csi\n");
		goto err_alloc_netdev;
	}

	if (glSetupCSI(prGlueInfo, prCsiWdev, prCsiDev) != 0) {
		DBGLOG(INIT, WARN, "glSetupCSI FAILED\n");
		free_netdev(prCsiDev);
		return FALSE;
	}

	/* set csi net device register state */
	/* csiNetRegister() will check prAdapter->rCsiNetRegState. */
	prAdapter->rCsiNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

	return TRUE;
err_alloc_netdev:
	return FALSE;
} /* end of glRegisterCSI() */

u_int8_t csiLaunch(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (!prGlueInfo->prAdapter) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}

	if (prGlueInfo->prAdapter->fgIsCsiRegistered == TRUE) {
		DBGLOG(CSI, INFO, "CSI is already registered\n");
		return FALSE;
	}

	if (!glRegisterCSI(prGlueInfo, csiifname)) {
		DBGLOG(CSI, ERROR, "Launch failed\n");
		return FALSE;
	}

	prGlueInfo->prAdapter->fgIsCsiRegistered = TRUE;

	DBGLOG(CSI, INFO, "Launch success, fgIsCsiRegistered TRUE\n");
	return TRUE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *       run csi exit procedure, glue unregister csi and set csi registered flag
 *
 * \retval 1     Success
 */
/*---------------------------------------------------------------------------*/
u_int8_t csiRemove(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(CSI, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (!prGlueInfo->prAdapter) {
		DBGLOG(CSI, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}

	if (prGlueInfo->prAdapter->fgIsCsiRegistered == FALSE) {
		DBGLOG(CSI, INFO, "csi is not registered\n");
		return FALSE;
	}

	DBGLOG(CSI, INFO, "fgIsCSIRegistered FALSE\n");
	prGlueInfo->prAdapter->fgIsCsiRegistered = FALSE;

	glUnregisterCSI(prGlueInfo);

	/* Release csi wdev. */
	if (prGlueInfo->prCsiRoleWdev == NULL)
		return TRUE;

	DBGLOG(INIT, INFO, "Unregister prGlueInfo->prCsiRoleWdev\n");

	kfree(prGlueInfo->prCsiRoleWdev);
	prGlueInfo->prCsiRoleWdev = NULL;

	return TRUE;


}

