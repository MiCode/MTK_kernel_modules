// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   gl_p2p_init.c
 *    \brief  init and exit routines of Linux driver interface for Wi-Fi Direct
 *
 *    This file contains the main routines
 *    of Linux driver for MediaTek Inc. 802.11
 *    Wireless LAN Adapters.
 */

/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

#include "precomp.h"

#if CFG_ENABLE_WIFI_DIRECT
/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

#define P2P_INF_NAME "p2p%d"

#if CFG_TC10_FEATURE
#define AP_INF_NAME  "swlan%d"
#else
#define AP_INF_NAME  "ap%d"
#endif

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
static uint8_t *ifname = P2P_INF_NAME;
static uint8_t *ifname2 = P2P_INF_NAME;
static uint16_t mode = RUNNING_P2P_MODE;


/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */

/******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 *                              F U N C T I O N S
 ******************************************************************************
 */

void p2pSetSuspendMode(struct GLUE_INFO *prGlueInfo, u_int8_t fgEnable)
{
	struct net_device *prDev = NULL;
	struct GL_P2P_INFO *prP2PInfo = NULL;

	if (!prGlueInfo)
		return;

	if (!prGlueInfo->prAdapter->fgIsP2PRegistered ||
		(prGlueInfo->prAdapter->rP2PNetRegState !=
			ENUM_NET_REG_STATE_REGISTERED)) {
		DBGLOG(INIT, DEBUG,
		       "%s: P2P is not enabled, SKIP!\n", __func__);
		return;
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
	prP2PInfo = prGlueInfo->prP2PInfo[0];
	if ((prP2PInfo->aprRoleHandler != NULL) &&
		(prP2PInfo->aprRoleHandler != prP2PInfo->prDevHandler)) {
		prDev = prP2PInfo->aprRoleHandler;
	} else {
		prDev = prP2PInfo->prDevHandler;
		/* Skip p2p dev for dev mode */
		if (p2pGetMode() == RUNNING_P2P_DEV_MODE &&
			!prP2PInfo->aprRoleHandler) {
			DBGLOG(INIT, LOUD, "P2P dev SKIP!\n");
			return;
		}
	}

	if (!prDev) {
		DBGLOG(INIT, DEBUG,
			"%s: P2P dev is not available, SKIP!\n", __func__);
		return;
	}

	kalSetNetAddressFromInterface(prGlueInfo, prDev, fgEnable);
	wlanNotifyFwSuspend(prGlueInfo, prDev, fgEnable);

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
	/*
	 * For case P2pRoleFsmGetStatisticsTimer is stopped by system suspend.
	 * We need to recall P2pRoleFsmGetStatisticsTimer when system resumes.
	 */
	p2pResumeStatisticsTimer(prGlueInfo, prDev);
#endif
}

#if CFG_ENABLE_PER_STA_STATISTICS_LOG
void p2pResumeStatisticsTimer(struct GLUE_INFO *prGlueInfo,
	struct net_device *prNetDev)
{
	struct BSS_INFO *prP2pBssInfo = (struct BSS_INFO *) NULL;
	struct P2P_ROLE_FSM_INFO *prP2pRoleFsmInfo =
		(struct P2P_ROLE_FSM_INFO *) NULL;
	uint8_t ucBssIndex = 0;

	if (!prGlueInfo)
		return;

	ucBssIndex = wlanGetBssIdx(prNetDev);

	if (!IS_BSS_INDEX_VALID(ucBssIndex)) {
		DBGLOG(P2P, ERROR,
			"StatisticsTimer resume failed. ucBssIndex is invalid\n");
		return;
	}

	prP2pBssInfo = GET_BSS_INFO_BY_INDEX(prGlueInfo->prAdapter, ucBssIndex);
	if (!prP2pBssInfo) {
		DBGLOG(P2P, ERROR,
			"StatisticsTimer resume failed. prP2pBssInfo is NULL\n");
		return;
	}

	prP2pRoleFsmInfo =
		P2P_ROLE_INDEX_2_ROLE_FSM_INFO(prGlueInfo->prAdapter,
			(uint8_t) prP2pBssInfo->u4PrivateData);

	if (!prP2pRoleFsmInfo) {
		DBGLOG(P2P, ERROR,
			"StatisticsTimer resume failed. prP2pRoleFsmInfo is NULL\n");
		return;
	}
#if CFG_SUPPORT_WFD
	if (prGlueInfo->prAdapter->rWifiVar.rWfdConfigureSettings.ucWfdEnable &&
		!prGlueInfo->fgIsInSuspendMode) {
		cnmTimerStartTimer(prGlueInfo->prAdapter,
			&(prP2pRoleFsmInfo->rP2pRoleFsmGetStatisticsTimer),
			P2P_ROLE_GET_STATISTICS_TIME);
	}
#endif
}
#endif

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *       run p2p init procedure, glue register p2p and set p2p registered flag
 *
 * \retval 1     Success
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pLaunch(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter = NULL;
	enum ENUM_P2P_REG_STATE eP2PRegState;
	enum ENUM_NET_REG_STATE eP2PNetRegState;

	GLUE_SPIN_LOCK_DECLARATION();

	prAdapter = prGlueInfo->prAdapter;

	ASSERT(prGlueInfo);
	ASSERT(prAdapter);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->rP2PRegState != ENUM_P2P_REG_STATE_UNREGISTERED) {
		eP2PRegState = prAdapter->rP2PRegState;
		eP2PNetRegState = prAdapter->rP2PNetRegState;
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		DBGLOG(P2P, INFO, "skip launch, p2p_state=%d, net_state=%d\n",
			eP2PRegState, eP2PNetRegState);
		return FALSE;
	}

	prAdapter->rP2PRegState = ENUM_P2P_REG_STATE_REGISTERING;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	if (!glRegisterP2P(prGlueInfo, ifname, ifname2, mode)) {
		DBGLOG(P2P, ERROR, "Launch failed\n");
		prAdapter->rP2PRegState = ENUM_P2P_REG_STATE_UNREGISTERED;
		return FALSE;
	}

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prAdapter->fgIsP2PRegistered = TRUE;
	prAdapter->p2p_scan_report_all_bss = CFG_P2P_SCAN_REPORT_ALL_BSS;
	prAdapter->rP2PRegState = ENUM_P2P_REG_STATE_REGISTERED;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	DBGLOG(P2P, TRACE, "Launch success, fgIsP2PRegistered TRUE\n");

	return TRUE;
}

uint8_t p2pGetMode(void)
{
	return mode;
}

void p2pSetMode(uint8_t ucAPMode)
{
	uint8_t *prAPInfName = AP_INF_NAME;
	uint8_t *prP2PInfName = P2P_INF_NAME;

#ifdef CFG_DRIVER_INF_NAME_CHANGE

	if (kalStrLen(gprifnamep2p) > 0) {
		prP2PInfName = kalStrCat(gprifnamep2p, "%d");
		DBGLOG(INIT, WARN,
			"P2P ifname customized, use %s\n", prP2PInfName);
	}

	if (kalStrLen(gprifnameap) > 0) {
		prAPInfName = kalStrCat(gprifnameap, "%d");
		DBGLOG(INIT, WARN,
			"AP ifname customized, use %s\n", prAPInfName);
	}

#endif /* CFG_DRIVER_INF_NAME_CHANGE */

	switch (ucAPMode) {
	case 0:
		mode = RUNNING_P2P_MODE;
		ifname = prP2PInfName;
		break;
	case 1:
		mode = RUNNING_AP_MODE;
		ifname = prAPInfName;
		break;
	case 2:
		mode = RUNNING_DUAL_AP_MODE;
		ifname = prAPInfName;
		break;
	case 3:
		mode = RUNNING_P2P_AP_MODE;
		ifname = prP2PInfName;
		ifname2 = prAPInfName;
		break;
	case 4:
		mode = RUNNING_DUAL_P2P_MODE;
		ifname = prP2PInfName;
		break;
	case 5:
		mode = RUNNING_P2P_DEV_MODE;
		ifname = prP2PInfName;
		break;
	case 6:
		mode = RUNNING_P2P_NO_GROUP_MODE;
		ifname = prP2PInfName;
		break;
	}
}				/* p2pSetMode */

/*---------------------------------------------------------------------------*/
/*!
 * \brief
 *       run p2p exit procedure, glue unregister p2p and set p2p registered flag
 *
 * \retval 1     Success
 */
/*---------------------------------------------------------------------------*/
u_int8_t p2pRemove(struct GLUE_INFO *prGlueInfo, uint8_t fgIsRtnlLockAcquired)
{
	struct ADAPTER *prAdapter = NULL;
	u_int32_t wait = 0;

	GLUE_SPIN_LOCK_DECLARATION();

	prAdapter = prGlueInfo->prAdapter;

	ASSERT(prGlueInfo);
	ASSERT(prAdapter);

	/* We must guarantee that all p2p net devices are unregistered with
	 * kernel before the net devices are freed. Otherwise, when p2pLaunch
	 * is invoked next time, we will get kernel exception because the old
	 * p2p net devices registered to kernel were volatile.
	 */
retry:
	wait = 0;
	while (wait < 2000) {
		/* p2p net devices are unregistered */
		if (prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED &&
			prAdapter->rP2PNetRegState ==
				ENUM_NET_REG_STATE_UNREGISTERED)
			break;

		/* p2p net devices are not unregistered yet */
		if (prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED &&
			prAdapter->rP2PNetRegState ==
				ENUM_NET_REG_STATE_REGISTERED) {
			p2pNetUnregister(prGlueInfo, fgIsRtnlLockAcquired,
					 FALSE);
			break;
		}

		kalMsleep(100);
		wait += 100;
	}

	if (wait >= 2000) {
		DBGLOG(P2P, INFO, "skip remove, p2p_state=%d, net_state=%d\n",
			prAdapter->rP2PRegState,
			prAdapter->rP2PNetRegState);
		return FALSE;
	}

	/* Make sure that p2p is in registered state and p2p net is in
	 * unregistered state before continuing the removal procedure.
	 */
	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	if (prAdapter->rP2PRegState == ENUM_P2P_REG_STATE_REGISTERED &&
		prAdapter->rP2PNetRegState == ENUM_NET_REG_STATE_UNREGISTERED)
		prAdapter->rP2PRegState = ENUM_P2P_REG_STATE_UNREGISTERING;
	else {
		/* Someone has changed p2p net register state. Try again. */
		DBGLOG(P2P, INFO, "retry remove, p2p_state=%d, net_state=%d\n",
			prAdapter->rP2PRegState,
			prAdapter->rP2PNetRegState);
		GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
		goto retry;
	}
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	prAdapter->p2p_scan_report_all_bss = FALSE;

	glUnregisterP2P(prGlueInfo, 0xff, fgIsRtnlLockAcquired);

	GLUE_ACQUIRE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);
	prAdapter->rP2PRegState = ENUM_P2P_REG_STATE_UNREGISTERED;
	prAdapter->fgIsP2PRegistered = FALSE;
	GLUE_RELEASE_SPIN_LOCK(prGlueInfo, SPIN_LOCK_NET_DEV);

	return TRUE;
}
#endif /* CFG_ENABLE_WIFI_DIRECT */
