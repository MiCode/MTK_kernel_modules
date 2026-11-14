// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 * Id: @(#) gl_fmcw.c@@
 */

/*! \file   gl_fmcw.c
 *    \brief  Main routines of Linux driver interface for Fmcw
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

#if (CFG_SUPPORT_FMCW_VNI == 1)

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define FMCW_INF_NAME "rdr%d"

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
static uint8_t *fmcwifname = FMCW_INF_NAME;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
/* Net Device Hooks */
static int fmcwOpen(struct net_device *prDev);

static int fmcwStop(struct net_device *prDev);

const struct net_device_ops fmcw_netdev_ops = {
	.ndo_open = fmcwOpen,
	.ndo_stop = fmcwStop
};

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

static int
fmcwOpen(struct net_device *prDev)
{
	if (!prDev) {
		DBGLOG(RDR, ERROR, "prDev error!\n");
		return -EFAULT;
	}

	if (!netif_carrier_ok(prDev))
		netif_carrier_on(prDev);

	return 0; /* success */
}

static int
fmcwStop(struct net_device *prDev)
{
	if (!prDev) {
		DBGLOG(RDR, ERROR, "prDev error!\n");
		return -EFAULT;
	}

	if (netif_carrier_ok(prDev))
		netif_carrier_off(prDev);

	return 0; /* success */
}

u_int8_t fmcwFreeInfo(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (prGlueInfo->prFmcwDevInfo != NULL) {
		kalMemFree(prGlueInfo->prFmcwDevInfo, VIR_MEM_TYPE,
			   sizeof(struct _GL_FMCW_INFO_T *));
		prGlueInfo->prFmcwDevInfo = NULL;
	}

	return TRUE;
}

u_int8_t fmcwNetUnregister(struct GLUE_INFO *prGlueInfo)
{
	struct net_device *prDevHandler = NULL;
	u_int8_t fgDoUnregister = FALSE;

	GLUE_SPIN_LOCK_DECLARATION();

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prAdapter) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}
	if (!prGlueInfo->prFmcwDevInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prFmcwDevInfo error\n");
		return FALSE;
	}

	prDevHandler = prGlueInfo->prFmcwDevInfo->prDevHandler;

	if (!prDevHandler) {
		DBGLOG(RDR, ERROR,
			"prGlueInfo->prFmcwDevInfo->prDevHandler error\n");
		return FALSE;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	switch (prGlueInfo->prAdapter->rFmcwNetRegState) {
	case ENUM_NET_REG_STATE_UNREGISTERED:
		DBGLOG(RDR, INFO, "Fmcw Net is already Unregistered\n");
		break;

	case ENUM_NET_REG_STATE_REGISTERED:
		prGlueInfo->prAdapter->rFmcwNetRegState =
			ENUM_NET_REG_STATE_UNREGISTERING;
		fgDoUnregister = TRUE;
		break;

	default:
		DBGLOG(RDR, ERROR,
			"FMCW Net is not REGISTERED when doing Unregister\n");
		break;
	}

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoUnregister)
		return FALSE;

	if (netif_carrier_ok(prDevHandler))
		netif_carrier_off(prDevHandler);

	netif_tx_stop_all_queues(prDevHandler);

	unregister_netdev(prDevHandler);

	prGlueInfo->prAdapter->rFmcwNetRegState =
		ENUM_NET_REG_STATE_UNREGISTERED;

	return TRUE;
}

u_int8_t fmcwNetRegister(struct GLUE_INFO *prGlueInfo)
{
	struct net_device *prDevHandler = NULL;
	unsigned char fgDoRegister = FALSE;
	unsigned char ret;

	GLUE_SPIN_LOCK_DECLARATION();

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}
	if (!prGlueInfo->prAdapter) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}
	if (!prGlueInfo->prFmcwDevInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prFmcwDevInfo error\n");
		return FALSE;
	}

	prDevHandler = prGlueInfo->prFmcwDevInfo->prDevHandler;

	if (!prDevHandler) {
		DBGLOG(RDR, ERROR,
			"prGlueInfo->prFmcwDevInfo->prDevHandler error\n");
		return FALSE;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	switch (prGlueInfo->prAdapter->rFmcwNetRegState) {
	case ENUM_NET_REG_STATE_REGISTERED:
		DBGLOG(RDR, INFO, "FMCW Net is already Registered\n");
		break;

	case ENUM_NET_REG_STATE_UNREGISTERED:
		prGlueInfo->prAdapter->rFmcwNetRegState =
			ENUM_NET_REG_STATE_REGISTERING;
		fgDoRegister = TRUE;
		break;

	default:
		DBGLOG(RDR, ERROR,
			"FMCW Net is not UNREGISTERED when doing Register\n");
		break;
	}

	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!fgDoRegister)
		return FALSE;

	ret = TRUE;
	/* net device initialize */
	netif_carrier_off(prDevHandler);
	netif_tx_stop_all_queues(prDevHandler);

	/* register for net device */
	if (register_netdev(
		prDevHandler) < 0) {
		DBGLOG(INIT, WARN,
			"unable to register netdevice for fmcw\n");
		/* trunk doesn't do free_netdev here */
		free_netdev(prDevHandler);

		ret = FALSE;
	} else {
		prGlueInfo->prAdapter->rFmcwNetRegState =
			ENUM_NET_REG_STATE_REGISTERED;
		ret = TRUE;
	}

	return ret;
}

u_int8_t fmcwAllocInfo(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	if (prGlueInfo->prFmcwDevInfo == NULL) {
		/* alloc memory for FMCWDEV info */
		prGlueInfo->prFmcwDevInfo = kalMemAlloc(
			sizeof(struct _GL_FMCW_INFO_T), VIR_MEM_TYPE);
		if (prGlueInfo->prFmcwDevInfo) {
			kalMemZero(prGlueInfo->prFmcwDevInfo,
				   sizeof(struct _GL_FMCW_INFO_T));
		} else {
			DBGLOG(RDR, INFO, "alloc prFmcwDevInfo fail\n");
			goto err_alloc;
		}
	}
	return TRUE;

err_alloc:

	if (prGlueInfo->prFmcwDevInfo) {
		kalMemFree(prGlueInfo->prFmcwDevInfo, VIR_MEM_TYPE,
		   sizeof(struct _GL_FMCW_INFO_T));

		prGlueInfo->prFmcwDevInfo = NULL;
	}

	return FALSE;
}

int glSetupFmcw(struct GLUE_INFO *prGlueInfo, struct wireless_dev *prFmcwWdev,
	   struct net_device *prFmcwDev)
{
	struct _GL_FMCW_INFO_T *prFmcwInfo = NULL;
	struct NETDEV_PRIVATE_GLUE_INFO *prNetDevPriv = NULL;
#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	struct GL_HIF_INFO *prHif = NULL;
#endif
#endif

	DBGLOG(INIT, INFO, "setup the fmcw dev\n");

	if ((prGlueInfo == NULL) || (prFmcwWdev == NULL) ||
	    (prFmcwWdev->wiphy == NULL) || (prFmcwDev == NULL)) {
		DBGLOG(RDR, ERROR, "parameter is NULL!!\n");
		return -1;
	}

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	prHif = &prGlueInfo->rHifInfo;

	if (prHif == NULL) {
		DBGLOG(RDR, ERROR, "prHif is NULL!!\n");
		return -1;
	}
#endif
#endif

	/* 0. allocate fmcwinfo */
	if (fmcwAllocInfo(prGlueInfo) != TRUE) {
		DBGLOG(RDR, ERROR, "Allocate memory for fmcw FAILED\n");
		return -1;
	}

	prFmcwInfo = prGlueInfo->prFmcwDevInfo;

	/* setup netdev */
	/* Point to shared glue structure */
	prNetDevPriv = (struct NETDEV_PRIVATE_GLUE_INFO *)
					netdev_priv(prFmcwDev);
	prNetDevPriv->prGlueInfo = prGlueInfo;

	prFmcwWdev->iftype = NL80211_IFTYPE_STATION;

	/* register callback functions */
	prFmcwDev->needed_headroom += NIC_TX_HEAD_ROOM;
	prFmcwDev->netdev_ops = &fmcw_netdev_ops;

#if defined(_HIF_SDIO)
#if (MTK_WCN_HIF_SDIO == 0)
	SET_NETDEV_DEV(prFmcwDev, &(prHif->func->dev));
#endif
#endif
	prFmcwDev->ieee80211_ptr = prFmcwWdev;
	prFmcwWdev->netdev = prFmcwDev;

	kalResetStats(prFmcwDev);

	/* finish
	 * bind netdev pointer to netdev index
	 */
	prFmcwInfo->prDevHandler = prFmcwDev;

	return 0;
}

u_int8_t glFmcwCreateWirelessDevice(struct GLUE_INFO *prGlueInfo)
{
	/* whsu, KAL_AIS_NUM at gprWdev */
	struct wireless_dev **pprOrigWdev = wlanGetWirelessDevice(prGlueInfo);
	struct wiphy *prWiphy = wlanGetWiphy(*pprOrigWdev);
	struct wireless_dev *prWdev = NULL;

	if (!prWiphy) {
		DBGLOG(INIT, ERROR, "unable to allocate wiphy for fmcw\n");
		return FALSE;
	}

	prWdev = kzalloc(sizeof(struct wireless_dev), GFP_KERNEL);
	if (!prWdev) {
		DBGLOG(INIT, ERROR, "allocate fmcw wdev fail, no memory\n");
		return FALSE;
	}

	/* set priv as pointer to glue structure */
	prWdev->wiphy = prWiphy;

	prGlueInfo->prFmcwRoleWdev = prWdev;
	DBGLOG(INIT, INFO, "glfmcwCreateWirelessDevice (%x)\n",
	       prGlueInfo->prFmcwRoleWdev->wiphy);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Unregister Net Device for Fmcw
 *
 * \param[in] prGlueInfo      Pointer to glue info
 *
 * \return   TRUE
 *           FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t glUnregisterFmcw(struct GLUE_INFO *prGlueInfo)
{
	struct _GL_FMCW_INFO_T *prFmcwInfo = NULL;

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error!\n");
		return FALSE;
	}

	/* 4 <3> Free Wiphy & netdev */
	prFmcwInfo = prGlueInfo->prFmcwDevInfo;
	if (prFmcwInfo == NULL)
		return TRUE;

	/* 4 <4> Free Fmcw internal memory */
	if (!fmcwFreeInfo(prGlueInfo)) {
		DBGLOG(INIT, ERROR, "fmcwFreeInfo FAILED\n");
		return FALSE;
	}

	return TRUE;
}

u_int8_t glRegisterFmcw(struct GLUE_INFO *prGlueInfo, const char *prDevName)
{
	struct ADAPTER *prAdapter = NULL;
	struct wireless_dev *prFmcwWdev = NULL;
	struct net_device *prFmcwDev = NULL;
	struct wiphy *prWiphy = NULL;
	const char *prSetDevName;

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error!\n");
		return FALSE;
	}
	if (!prDevName) {
		DBGLOG(RDR, ERROR, "prDevName error!\n");
		return FALSE;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(RDR, ERROR, "prAdapter error!\n");
		return FALSE;
	}

	glFmcwCreateWirelessDevice(prGlueInfo);

	if (!prGlueInfo->prFmcwRoleWdev) {
		DBGLOG(INIT, ERROR, "prGlueInfo->prFmcwRoleWdev is NULL\n");
		return FALSE;
	}

	DBGLOG(INIT, INFO, "prFmcwWdev\n");
	prFmcwWdev = prGlueInfo->prFmcwRoleWdev;

	/* Reset prFmcwWdev for the issue that the prFmcwWdev doesn't
	 * reset when the usb unplug/plug.
	 */
	prWiphy = prFmcwWdev->wiphy;
	memset(prFmcwWdev, 0, sizeof(struct wireless_dev));
	prFmcwWdev->wiphy = prWiphy;

	prSetDevName = prDevName;

    /* allocate netdev */
#if KERNEL_VERSION(3, 17, 0) <= CFG80211_VERSION_CODE
	prFmcwDev = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
			NET_NAME_PREDICTABLE, ether_setup, CFG_MAX_TXQ_NUM);
#else
	prFmcwDev = alloc_netdev_mq(
			sizeof(struct NETDEV_PRIVATE_GLUE_INFO), prSetDevName,
			ether_setup, CFG_MAX_TXQ_NUM);
#endif
	if (!prFmcwDev) {
		DBGLOG(INIT, WARN, "unable to allocate ndev for fmcw\n");
		goto err_alloc_netdev;
	}

	if (glSetupFmcw(prGlueInfo, prFmcwWdev, prFmcwDev) != 0) {
		DBGLOG(INIT, WARN, "glSetupFmcw FAILED\n");
		free_netdev(prFmcwDev);
		return FALSE;
	}

	/* set fmcw net device register state */
	/* fmcwNetRegister() will check prAdapter->rFmcwNetRegState. */
	prAdapter->rFmcwNetRegState = ENUM_NET_REG_STATE_UNREGISTERED;

	return TRUE;
err_alloc_netdev:
	return FALSE;
} /* end of glRegisterFmcw() */

u_int8_t fmcwLaunch(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = NULL;

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}

	if (prAdapter->fgIsFmcwRegistered == TRUE) {
		DBGLOG(RDR, ERROR, "RDR is already registered\n");
		return FALSE;
	}

	if (!glRegisterFmcw(prGlueInfo, fmcwifname)) {
		DBGLOG(RDR, ERROR, "Launch failed\n");
		return FALSE;
	}

	prAdapter->fgIsFmcwRegistered = TRUE;

	return TRUE;
}

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *  run fmcw exit procedure, glue unregister fmcw
 *  and set fmcw registered flag
 *
 * \retval 1     Success
 */
/*---------------------------------------------------------------------------*/
u_int8_t fmcwRemove(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = NULL;

	if (!prGlueInfo) {
		DBGLOG(RDR, ERROR, "prGlueInfo error\n");
		return FALSE;
	}

	prAdapter = prGlueInfo->prAdapter;

	if (!prAdapter) {
		DBGLOG(RDR, ERROR, "prGlueInfo->prAdapter error\n");
		return FALSE;
	}

	if (prAdapter->fgIsFmcwRegistered == FALSE) {
		DBGLOG(RDR, INFO, "fmcw is not registered\n");
		return FALSE;
	}

	DBGLOG(RDR, INFO, "fgIsFmcwRegistered FALSE\n");
	prAdapter->fgIsFmcwRegistered = FALSE;

	glUnregisterFmcw(prGlueInfo);

	/* Release fmcw wdev. */
	if (prGlueInfo->prFmcwRoleWdev == NULL)
		return TRUE;

	DBGLOG(INIT, INFO, "Unregister prGlueInfo->prFmcwRoleWdev\n");

	kfree(prGlueInfo->prFmcwRoleWdev);
	prGlueInfo->prFmcwRoleWdev = NULL;

	return TRUE;


}
#endif /* CFG_SUPPORT_FMCW_VNI */

