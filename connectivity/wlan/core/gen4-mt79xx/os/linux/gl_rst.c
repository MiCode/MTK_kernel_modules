// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 ** Id: @(#) gl_rst.c@@
 */

/*! \file   gl_rst.c
 *    \brief  Main routines for supporintg MT6620 whole-chip reset mechanism
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
#include <linux/kernel.h>
#include <linux/workqueue.h>

#include "precomp.h"
#include "gl_rst.h"

#if CFG_CHIP_RESET_SUPPORT
/*******************************************************************************
 *                              D A T A   T Y P E S
 *******************************************************************************
 */
enum ENUM_BT_RESET_RETURN_STATE {
	BT_RESET_OK = 0,
	BT_RESET_NOT_OK,
	BT_RESET_NOT_FOUND
};


/*******************************************************************************
 *                              C O N S T A N T S
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
#if (CFG_SUPPORT_MULTI_CARD == 0)
static struct CHIP_RESET_INFO rChipResetInfo;
#endif


/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#if CFG_SUPPORT_MULTI_CARD
#define P_CHIP_RESET_INFO(prGlueInfo)  \
	((struct CHIP_RESET_INFO *)(&((prGlueInfo)->rChipResetInfo)))
#else
#define P_CHIP_RESET_INFO(prGlueInfo)  \
	((struct CHIP_RESET_INFO *)(&rChipResetInfo))
#endif


/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
static void mtk_wifi_reset_main(struct GLUE_INFO *prGlueInfo);

static void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state);
static void WfsysResetHdlr(struct work_struct *work);

static u_int8_t is_bt_exist(void);
static u_int8_t rst_L0_notify_step1(struct GLUE_INFO *prGlueInfo);
static void wait_core_dump_end(struct GLUE_INFO *prGlueInfo);

#endif  /* CFG_CHIP_RESET_SUPPORT */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for checking if connectivity chip is resetting
 * @param   prGlueInfo
 * @retval  TRUE / FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsResetting(struct GLUE_INFO *prGlueInfo)
{
#if CFG_CHIP_RESET_SUPPORT
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return FALSE;
#endif

	return P_CHIP_RESET_INFO(prGlueInfo)->fgIsResetting;
#else
	return FALSE;
#endif  /* CFG_CHIP_RESET_SUPPORT */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for setting connectivity chip is resetting
 * @param   prGlueInfo
 * @param   value: TRUE / FALSE
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glSetResettingFlag(struct GLUE_INFO *prGlueInfo, u_int8_t value)
{
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return;
#endif

	P_CHIP_RESET_INFO(prGlueInfo)->fgIsResetting = value;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for checking if connectivity chip is resetting
 * @param   prGlueInfo
 * @retval  TRUE / FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalGetSimplifyResetFlowFlag(struct GLUE_INFO *prGlueInfo)
{
#if CFG_CHIP_RESET_SUPPORT
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return FALSE;
#endif

	return P_CHIP_RESET_INFO(prGlueInfo)->fgSimplifyResetFlow;
#else
	return FALSE;
#endif  /* CFG_CHIP_RESET_SUPPORT */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for setting connectivity chip is in SimplifyResetFlow
 * @param   prGlueInfo
 * @param   value: TRUE / FALSE
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glSetSimplifyResetFlowFlag(struct GLUE_INFO *prGlueInfo, u_int8_t value)
{
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return;
#endif

	P_CHIP_RESET_INFO(prGlueInfo)->fgSimplifyResetFlow = value;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for checking if it is necessary to disable FW OWN due to the
 *        L0.5 reset.
 * @param   prGlueInfo
 * @retval  TRUE / FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsRstPreventFwOwn(struct GLUE_INFO *prGlueInfo)
{
#if CFG_CHIP_RESET_SUPPORT
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return FALSE;
#endif
	return P_CHIP_RESET_INFO(prGlueInfo)->fgIsRstPreventFwOwn;
#else
	return FALSE;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for setting a flag to indicate that a L0.5 reset is in progress
 *        and avoid FW OWN.
 * @param   prGlueInfo
 * @param   value: TRUE / FALSE
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glSetRstPreventFwOwn(struct GLUE_INFO *prGlueInfo, u_int8_t value)
{
#if CFG_SUPPORT_MULTI_CARD
	if (!prGlueInfo)
		return;
#endif

	P_CHIP_RESET_INFO(prGlueInfo)->fgIsRstPreventFwOwn = value;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Called for getting the timestamp of the most recent reset occurrence.
 * @param   prGlueInfo
 * @retval  u8ResetTime
 */
/*----------------------------------------------------------------------------*/
uint64_t glGetRstTIme(struct GLUE_INFO *prGlueInfo)
{
	return P_CHIP_RESET_INFO(prGlueInfo)->u8ResetTime;
}


#if CFG_CHIP_RESET_SUPPORT

/*----------------------------------------------------------------------------*/
/*!
 * @brief set cruuent reset reason
 * @param   prGlueInfo
 * @param   eReason
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glSetRstReason(struct ADAPTER *prAdapter,
		    enum _ENUM_CHIP_RESET_REASON_TYPE_T eReason)
{
	struct GLUE_INFO *prGlueInfo;

	if (prAdapter == NULL) {
#if CFG_SUPPORT_MULTI_CARD
		DBGLOG(INIT, ERROR, "prAdapter is null, skip reset!\n");
		return;
#else
		/* if no multi card support,
		 * we can't get default glue info from global gprWdev[0]
		 */
		WIPHY_PRIV(wlanGetWiphy(gprWdev[0]), prGlueInfo);
#endif
	} else {
		prGlueInfo = prAdapter->prGlueInfo;
	}

	if (kalIsResetting(prGlueInfo))
		return;
	P_CHIP_RESET_INFO(prGlueInfo)->eResetReason = eReason;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief get cruuent reset reason
 * @param   prGlueInfo
 * @retval  eReason
 */
/*----------------------------------------------------------------------------*/
int glGetRstReason(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;

	if (prAdapter == NULL) {
#if CFG_SUPPORT_MULTI_CARD
		DBGLOG(INIT, ERROR, "prAdapter is null, skip reset!\n");
		return (int)(RST_CMD_TRIGGER);
#else
		/* if no multi card support,
		 * we can't get default glue info from global gprWdev[0]
		 */
		WIPHY_PRIV(wlanGetWiphy(gprWdev[0]), prGlueInfo);
#endif
	} else {
		prGlueInfo = prAdapter->prGlueInfo;
	}

	return (int)(P_CHIP_RESET_INFO(prGlueInfo)->eResetReason);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief init wake lock for chip reset
 * @param   prGlueInfo
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
static void glResetInitWakeLock(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	char wakelock_name[32];
	char *phy_name;
	KAL_WAKE_LOCK_T *lock =
			P_CHIP_RESET_INFO(prGlueInfo)->prWlanChipResetWakeLock;

	if (prGlueInfo->prDevHandler)
		phy_name = prGlueInfo->prDevHandler->name;
	else
		phy_name = "unknown";

	snprintf(wakelock_name, sizeof(wakelock_name),
		 "WLAN_ChipReset_%s", phy_name);

	KAL_WAKE_LOCK_INIT(NULL, lock, wakelock_name);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief hold wake lock for chip reset
 * @param   prGlueInfo
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
static void glResetWakeLock(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *lock =
			P_CHIP_RESET_INFO(prGlueInfo)->prWlanChipResetWakeLock;

	/* directily call __pm_stay_awake to ignore halIsHifStateReady check */
	if ((lock) && (!KAL_WAKE_LOCK_ACTIVE(NULL, lock)))
		__pm_stay_awake(lock);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief release wake lock for chip reset
 * @param   prGlueInfo
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
static void glResetWakeUnLock(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *lock =
			P_CHIP_RESET_INFO(prGlueInfo)->prWlanChipResetWakeLock;

	if (KAL_WAKE_LOCK_ACTIVE(NULL, lock))
		KAL_WAKE_UNLOCK(NULL, lock);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief destroy wake lock for chip reset
 * @param   prGlueInfo
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
static void glResetDestroyWakeLock(struct GLUE_INFO *prGlueInfo)
{
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *lock =
			P_CHIP_RESET_INFO(prGlueInfo)->prWlanChipResetWakeLock;

	if (KAL_WAKE_LOCK_ACTIVE(NULL, lock))
		KAL_WAKE_UNLOCK(NULL, lock);
	KAL_WAKE_LOCK_DESTROY(NULL, lock);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. register wifi reset callback
 *        2. initialize wifi reset work
 *
 * @param prGlueInfo
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetInit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_CHIP_RESET_KO_SUPPORT
	P_CHIP_RESET_INFO(prGlueInfo)->u4RstCount = 0;
	P_CHIP_RESET_INFO(prGlueInfo)->u4PowerOffCount = 0;
	P_CHIP_RESET_INFO(prGlueInfo)->fgIsPendingForReady = FALSE;
#endif
	P_CHIP_RESET_INFO(prGlueInfo)->prGlueInfo = prGlueInfo;
	P_CHIP_RESET_INFO(prGlueInfo)->u4ProbeCount = 0;

	glSetRstPreventFwOwn(prGlueInfo, FALSE);
	glSetResettingFlag(prGlueInfo, FALSE);
	glSetSimplifyResetFlowFlag(prGlueInfo, FALSE);
	glResetInitWakeLock(prGlueInfo);

	P_CHIP_RESET_INFO(prGlueInfo)->fgIsInitialized = TRUE;
}

void glReseProbeRemoveDone(struct GLUE_INFO *prGlueInfo, int32_t i4Status,
			   u_int8_t fgIsProbe)
{
	uint32_t bus_id = 0;

	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "HifInfo is NULL\n");
		return;
	}

#if CFG_SUPPORT_MULTI_CARD
#if defined(_HIF_USB)
	bus_id = glGetBusId(prGlueInfo->rHifInfo.intf);
#endif
#if defined(_HIF_PCIE)
	bus_id = glGetBusId(prGlueInfo->rHifInfo.pdev);
#endif
#endif  /* CFG_SUPPORT_MULTI_CARD */

	if (fgIsProbe) {
		/* probe done, release wake lock */
		glResetWakeUnLock(prGlueInfo);

		P_CHIP_RESET_INFO(prGlueInfo)->u4ProbeCount++;
		DBGLOG(INIT, WARN,
			"[SER][L0] %s: busid %08x, probe count %d, status %d\n",
			__func__, bus_id,
			P_CHIP_RESET_INFO(prGlueInfo)->u4ProbeCount,
			i4Status);
#if CFG_CHIP_RESET_KO_SUPPORT
		if (i4Status == WLAN_STATUS_SUCCESS) {
			glSendResetEvent(bus_id,
					 RESET_MODULE_TYPE_WIFI,
					 RFSM_EVENT_PROBED);
#if defined(_HIF_SDIO)
			update_hif_info(HIF_INFO_SDIO_HOST,
					prGlueInfo->rHifInfo.func);
#endif
		}
	} else {
		glSendResetEvent(bus_id,
				 RESET_MODULE_TYPE_WIFI, RFSM_EVENT_REMOVED);
	}

	if (P_CHIP_RESET_INFO(prGlueInfo)->fgIsPendingForReady) {
		P_CHIP_RESET_INFO(prGlueInfo)->fgIsPendingForReady = FALSE;
		glSetResettingFlag(prGlueInfo, TRUE);
		glSendResetEvent(bus_id,
				 RESET_MODULE_TYPE_WIFI, RFSM_EVENT_READY);
#endif  /* CFG_CHIP_RESET_KO_SUPPORT */
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. deregister wifi reset callback
 *
 * @param prGlueInfo
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetUninit(struct GLUE_INFO *prGlueInfo)
{
	P_CHIP_RESET_INFO(prGlueInfo)->fgIsInitialized = FALSE;

	glResetDestroyWakeLock(prGlueInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief For CE project, decide reset action according to trigger reset reason.
 *
 * @param   prAdapter
 *
 * @retval  reset action like RST_FLAG_DO_CORE_DUMP,
 *          or RST_FLAG_PREVENT_POWER_OFF, ..., etc.
 */
/*----------------------------------------------------------------------------*/
uint32_t glResetSelectActionCe(IN struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4RstFlag = 0;

	prChipInfo = prAdapter->chip_info;

	switch (glGetRstReason(prAdapter)) {
	case RST_PROCESS_ABNORMAL_INT:
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
		u4RstFlag = RST_FLAG_DO_L1_RESET;
#elif defined(_HIF_SDIO)
		u4RstFlag = RST_FLAG_DO_L0P5_RESET;
#else
		/* print log but don't execute SER */
		DBGLOG(INIT, WARN,
		       "USB shall not happen abnormal interrupt\n");
#endif
		break;

	case RST_DRV_OWN_FAIL:
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_SDIO)
		u4RstFlag = RST_FLAG_DO_L0P5_RESET;
#else
		/* print log but don't execute SER */
		DBGLOG(INIT, WARN,
		       "USB shall not happen set drv own fail\n");
#endif
		break;

	case RST_FW_ASSERT_DONE:
		/* If L0.5 is supported, then L0.5 reset will be triggered
		 * automatically by RST_WDT. Otherwise, execute L0 reset.
		 */
		if (!prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
		break;

	case RST_RX_ERROR:
#if defined(_HIF_SDIO)
		u4RstFlag = RST_FLAG_DO_L0P5_RESET;
#else
		/* When 20201225, HIF other than SDIO doesn't have this error.
		 * If this isn't TRUE in the future, then we need to decide
		 * the appropriate reset at that time.
		 */
#endif
		break;

	case RST_FW_ASSERT_TIMEOUT:
	case RST_OID_TIMEOUT:
	case RST_CMD_EVT_FAIL:
	case RST_WDT:
	case RST_SER_L1_FAIL:
		u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		break;

	case RST_BT_TRIGGER:
	case RST_CR_ACCESS_FAIL:
	case RST_CMD_TRIGGER:
	case RST_SER_L0P5_FAIL:
	default:
		u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
		break;
	}

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	if ((u4RstFlag & RST_FLAG_DO_L1_RESET) &&
	    prAdapter->rWifiVar.eEnableSerL1 != FEATURE_OPT_SER_ENABLE) {
		DBGLOG(INIT, INFO,
		       "[SER][L1] Bypass L1 reset due to wifi.cfg\n");

		if (prChipInfo->fgIsSupportL0p5Reset) {
			DBGLOG(INIT, INFO,
			       "[SER][L1] Try L0.5 reset instead\n");

			u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		} else {
			DBGLOG(INIT, INFO,
			       "[SER][L1] Try L0 reset instead\n");

			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
		}
	}
#endif

	if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
	    prAdapter->rWifiVar.eEnableSerL0p5 != FEATURE_OPT_SER_ENABLE) {
		DBGLOG(INIT, INFO,
		       "[SER][L0.5] Bypass L0.5 reset due to wifi.cfg\n");
		DBGLOG(INIT, INFO,
		       "[SER][L0.5] Try L0 reset instead\n");

		u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
	} else if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
		prChipInfo->fgIsSupportL0p5Reset != TRUE) {
		DBGLOG(INIT, INFO,
			"[SER][L0.5] Bypass L0.5 reset due to chip setting\n");
		u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
	} else if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
		HAL_TEST_FLAG(prAdapter, ADAPTER_FLAG_HW_ERR)) {
		DBGLOG(INIT, INFO,
			"[SER][L0.5] change to L0 RST when hw err flag has been set\n");
		u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
	}

	if ((u4RstFlag & RST_FLAG_DO_WHOLE_RESET) &&
	    !prAdapter->rWifiVar.fgEnableSerL0) {
		DBGLOG(INIT, INFO,
		       "[SER][L0] Bypass L0 reset due to wifi.cfg\n");

		u4RstFlag = 0;
	}

	return u4RstFlag;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Reset trigger CE flow
 *
 * @param   prAddapter
 *          u4RstFlag  specify reset option
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
static void glResetTriggerCe(IN struct ADAPTER *prAdapter,
			     IN uint32_t u4RstFlag)
{
	struct GLUE_INFO *prGlueInfo;
#if CFG_CHIP_RESET_KO_SUPPORT
	uint32_t bus_id = 0;
#endif

	if (!prAdapter) {
#if CFG_SUPPORT_MULTI_CARD
		return;
#else
		WIPHY_PRIV(wlanGetWiphy(gprWdev[0]), prGlueInfo);
#endif
	} else {
		prGlueInfo = prAdapter->prGlueInfo;
	}
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "GlueInfo or HifInfo is NULL\n");
		return;
	}

	if ((prAdapter == NULL) ||
	    (u4RstFlag & RST_FLAG_DO_WHOLE_RESET)) {
		glResetWakeLock(prGlueInfo);
		glSetResettingFlag(prGlueInfo, TRUE);
		glSetSimplifyResetFlowFlag(prGlueInfo, FALSE);
#if CFG_CHIP_RESET_KO_SUPPORT
#if defined(_HIF_USB) && (CFG_SUPPORT_MULTI_CARD == 1)
		bus_id = glGetBusId(prGlueInfo->rHifInfo.intf);
#endif
		if (glSendResetEvent(bus_id,
				     RESET_MODULE_TYPE_WIFI,
				     RFSM_EVENT_TRIGGER_RESET) !=
		    RESET_RETURN_STATUS_SUCCESS)
#endif  /* CFG_CHIP_RESET_KO_SUPPORT */
		{
			mtk_wifi_reset_main(prGlueInfo);
		}
	} else if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
		   prAdapter->chip_info->fgIsSupportL0p5Reset) {
		spin_lock_bh(&prAdapter->rWfsysResetLock);

		if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_IDLE) {
#if CFG_SUPPORT_NAN
			mtk_cfg80211_vendor_event_nan_chip_reset(prAdapter,
								TRUE);
#endif
			glSetResettingFlag(prGlueInfo, TRUE);
			glSetRstPreventFwOwn(prGlueInfo, TRUE);
			spin_unlock_bh(&prAdapter->rWfsysResetLock);

			glSetWfsysResetState(prAdapter,
					     WFSYS_RESET_STATE_DETECT);

			schedule_work(
			       &P_CHIP_RESET_INFO(prGlueInfo)->rWfsysResetWork);
		} else
			spin_unlock_bh(&prAdapter->rWfsysResetLock);
	} else if (u4RstFlag & RST_FLAG_DO_L1_RESET) {
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
				 SER_SET_L1_RECOVER, 0);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Reset trigger common flow for both MOBILE and CE
 *
 * @param   prAddapter
 *          u4RstFlag  specify reset option
 *          pucFile  reset is triggered at which file
 *          u4Line  reset is triggered at which line
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
static void glResetTriggerCommon(struct ADAPTER *prAdapter, uint32_t u4RstFlag,
			  const uint8_t *pucFile, uint32_t u4Line)
{
	struct CHIP_DBG_OPS *prChipDbg;
	uint16_t u2FwOwnVersion;
	uint16_t u2FwPeerVersion;
	static const char *const apcReason[RST_REASON_MAX] = {
		"RST_UNKNOWN",
		"RST_PROCESS_ABNORMAL_INT",
		"RST_DRV_OWN_FAIL",
		"RST_FW_ASSERT_DONE",
		"RST_FW_ASSERT_TIMEOUT",
		"RST_BT_TRIGGER",
		"RST_OID_TIMEOUT",
		"RST_CMD_TRIGGER",
		"RST_CR_ACCESS_FAIL",
		"RST_CMD_EVT_FAIL",
		"RST_GROUP4_NULL",
		"RST_TX_ERROR",
		"RST_RX_ERROR",
		"RST_WDT",
		"RST_SER_L1_FAIL",
		"RST_SER_L0P5_FAIL",
	};
	static const char *const apcAction[] = {
		"RST_FLAG_DO_CORE_DUMP",
		"RST_FLAG_PREVENT_POWER_OFF",
		"RST_FLAG_DO_WHOLE_RESET",
		"RST_FLAG_DO_L0P5_RESET",
		"RST_FLAG_DO_L1_RESET",
	};
	uint32_t i;
	u_int8_t fgDrvOwn;
	enum _ENUM_CHIP_RESET_REASON_TYPE_T reason = glGetRstReason(prAdapter);

	if (!prAdapter) {
		if ((uint32_t)reason < RST_REASON_MAX)
			DBGLOG(INIT, ERROR,
			       "Trigger reset in %s line %u reason %s, but prAdapter is null\n",
			       pucFile, u4Line,
			       apcReason[(uint32_t)reason]);
		else
			DBGLOG(INIT, ERROR,
			       "Trigger reset in %s line %u unsupported reason %d and prAdapter is null\n",
			       pucFile, u4Line, (uint32_t)reason);
		return;
	}
	prChipDbg = prAdapter->chip_info->prDebugOps;
	u2FwOwnVersion = prAdapter->rVerInfo.u2FwOwnVersion;
	u2FwPeerVersion = prAdapter->rVerInfo.u2FwPeerVersion;
	fgDrvOwn = TRUE;
	HAL_LP_OWN_RD(prAdapter, &fgDrvOwn);

	if ((uint32_t)reason < RST_REASON_MAX)
		DBGLOG(INIT, ERROR,
		       "Trigger reset in %s line %u reason %s\n",
		       pucFile, u4Line, apcReason[(uint32_t)reason]);
	else
		DBGLOG(INIT, ERROR,
		       "Trigger reset in %s line %u but unsupported reason %d\n",
		       pucFile, u4Line, reason);
	for (i = 0; i < sizeof(apcAction) / sizeof(char *); i++)
		if (u4RstFlag & BIT(i))
			DBGLOG(INIT, ERROR, "action %s\n", apcAction[i]);
	if (!u4RstFlag)
		DBGLOG(INIT, ERROR, "no action\n");

	DBGLOG(INIT, ERROR,
	       "Chip[%04X E%u] FW Ver DEC[%u.%u] HEX[%x.%x]\n",
	       prAdapter->chip_info->chip_id,
	       wlanGetEcoVersion(prAdapter),
	       (uint16_t)(u2FwOwnVersion >> 8),
	       (uint16_t)(u2FwOwnVersion & BITS(0, 7)),
	       (uint16_t)(u2FwOwnVersion >> 8),
	       (uint16_t)(u2FwOwnVersion & BITS(0, 7)));
	DBGLOG(INIT, ERROR,
	       "Driver Ver[%u.%u]\n",
		(uint16_t)(u2FwPeerVersion >> 8),
		(uint16_t)(u2FwPeerVersion & BITS(0, 7)));

	prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;

#if defined(_HIF_USB) || defined(_HIF_SDIO)
	/* sdio_claim_hosth and usb_control_msg may cause sleep,
	 * so they cannot be executed in interrupt context.
	 */
	if (in_interrupt()) {
		DBGLOG(INIT, ERROR, "in_interrupt(), skip show hif dbg info\n");
		return;
	}
#endif

	if (fgDrvOwn) {
		halPrintHifDbgInfo(prAdapter);
		if ((u4RstFlag & RST_FLAG_DO_CORE_DUMP)
			&& (prChipDbg->show_mcu_debug_info != NULL)) {
			prChipDbg->show_mcu_debug_info(prAdapter, NULL, 0,
				DBG_MCU_DBG_ALL, NULL);
		}
	} else
		DBGLOG(INIT, WARN, "bypass debug msg when fw own\n");

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Reset trigger entry point.
 *
 * @param   prAddapter
 *          u4RstFlag  specify reset option
 *          pucFile  reset is triggered at which file
 *          u4Line  reset is triggered at which line
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
void glResetTrigger(struct ADAPTER *prAdapter, uint32_t u4RstFlag,
		    const uint8_t *pucFile, uint32_t u4Line)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t bus_id = 0;

	if (prAdapter == NULL) {
#if CFG_SUPPORT_MULTI_CARD
		DBGLOG(INIT, ERROR, "prAdapter is null, skip reset!\n");
		return;
#else
		/* if no multi card support,
		 * we can't get default glue info from global gprWdev[0]
		 */
		WIPHY_PRIV(wlanGetWiphy(gprWdev[0]), prGlueInfo);
#endif
	} else {
		prGlueInfo = prAdapter->prGlueInfo;
#if defined(_HIF_USB) && (CFG_SUPPORT_MULTI_CARD == 1)
		bus_id = glGetBusId(prGlueInfo->rHifInfo.udev);
#endif
	}
	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "prGlueInfo is null, skip reset!\n");
		return;
	}

	if (P_CHIP_RESET_INFO(prGlueInfo)->fgIsInitialized != TRUE)
		return;

	if (kalIsResetting(prGlueInfo)
#if CFG_DC_USB_WOW_CALLBACK
	    || prGlueInfo->rHifInfo.fgUsbShutdown
#endif
	)
		return;

	if (wlanIsProbingOrRemoving(FALSE, bus_id)) {
		DBGLOG(INIT, ERROR, "wlanRemove in process, skip reset!\n");
		return;
	}

	P_CHIP_RESET_INFO(prGlueInfo)->u8ResetTime = sched_clock();

	if (prAdapter &&
	    ((u4RstFlag & RST_FLAG_DO_WHOLE_RESET) ||
	     (u4RstFlag & RST_FLAG_DO_L0P5_RESET))) {
		spin_lock_bh(&prAdapter->rWfsysResetLock);
		if (prAdapter->eWfsysResetState != WFSYS_RESET_STATE_IDLE) {
			spin_unlock_bh(&prAdapter->rWfsysResetLock);
			DBGLOG(INIT, ERROR, "skip reset during L0.5\n");
			return;
		}
		spin_unlock_bh(&prAdapter->rWfsysResetLock);
	}
	glSetResettingFlag(prGlueInfo, TRUE);

	dump_stack();

	glResetTriggerCommon(prAdapter, u4RstFlag, pucFile, u4Line);
	glResetTriggerCe(prAdapter, u4RstFlag);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Set L0.5 reset state
 *
 * @param   state
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
static void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state)
{
	if (state >= WFSYS_RESET_STATE_MAX) {
		DBGLOG(INIT, WARN,
			"[SER][L0.5] unsupported wfsys reset state\n");
	} else {
		spin_lock_bh(&prAdapter->rWfsysResetLock);

		prAdapter->eWfsysResetState = state;

		spin_unlock_bh(&prAdapter->rWfsysResetLock);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief init work for L0.5 reset
 * @param   prGlueInfo
 * @param   func
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glResetInitL0p5Work(struct GLUE_INFO *prGlueInfo)
{
	spin_lock_init(&prGlueInfo->prAdapter->rWfsysResetLock);
	glSetWfsysResetState(prGlueInfo->prAdapter, WFSYS_RESET_STATE_IDLE);
	INIT_WORK(&P_CHIP_RESET_INFO(prGlueInfo)->rWfsysResetWork,
		  WfsysResetHdlr);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief flush L0.5 work, this function will block until L0.5 work is completed
 * @param   prGlueInfo
 * @retval  void
 */
/*----------------------------------------------------------------------------*/
void glResetWaitL0p5WorkDone(struct GLUE_INFO *prGlueInfo)
{
	flush_work(&P_CHIP_RESET_INFO(prGlueInfo)->rWfsysResetWork);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Re-schedule L0.5 reset if it's in WFSYS_POSTPONE state.
 *
 * @param   prAdapter
 *
 * @retval  TRUE   if re-schedule L0.5 reset is done
 *          FALSE   otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t glResetReSchL0p5Work(struct GLUE_INFO *prGlueInfo)
{
	u_int8_t fgReSch = FALSE;
	struct ADAPTER *prAdapter = prGlueInfo->prAdapter;

	spin_lock_bh(&prAdapter->rWfsysResetLock);

	if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_POSTPONE) {
		fgReSch = TRUE;
		schedule_work(&P_CHIP_RESET_INFO(prGlueInfo)->rWfsysResetWork);
	}

	spin_unlock_bh(&prAdapter->rWfsysResetLock);

	return fgReSch;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief L0.5 reset workhorse.
 *
 * @param   work
 *
 * @retval  none
 */
/*----------------------------------------------------------------------------*/
static void WfsysResetHdlr(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct mt66xx_hif_driver_data *prHifDrvData;
	struct net_device *prNetDev;
	struct CHIP_RESET_INFO *prChipResetInfo;

	prChipResetInfo =
		container_of(work, struct CHIP_RESET_INFO, rWfsysResetWork);
	if (!prChipResetInfo) {
		DBGLOG(INIT, ERROR, "fail to get CHIP_RESET_INFO\n");
		return;
	}
	prGlueInfo = prChipResetInfo->prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "fail to get prGlueInfo\n");
		return;
	}
	prAdapter = prGlueInfo->prAdapter;
	prNetDev  = prGlueInfo->prDevHandler;
	if (prAdapter == NULL || prNetDev == NULL) {
		DBGLOG(INIT, ERROR, "fail to get prAdapter or prNetDev\n");
		return;
	}

#if defined(_HIF_USB)
	if (glUsbGetState(&prGlueInfo->rHifInfo) == USB_STATE_LINK_DOWN) {
		glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);
		DBGLOG(INIT, ERROR, "Usb is disconnectting\n\n");
		return;
	}
#endif

	prHifDrvData = container_of(&prAdapter->chip_info,
				    struct mt66xx_hif_driver_data, chip_info);

	DBGLOG(INIT, STATE, "[SER][L0.5] Reset triggered eWfsysResetState=%d\n",
						prAdapter->eWfsysResetState);
	spin_lock_bh(&prAdapter->rWfsysResetLock);

	if (prAdapter->eWfsysResetState != WFSYS_RESET_STATE_POSTPONE) {
		spin_unlock_bh(&prAdapter->rWfsysResetLock);

		if (prAdapter->u2WfsysResetCnt < 0xFFFF)
			prAdapter->u2WfsysResetCnt++;

		glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_RESET);

		HAL_CANCEL_TX_RX(prAdapter);

#if defined(_HIF_SDIO)
		/* please refer to 7961 sdio ser L0.5 program guide */
		if (prAdapter->chip_info->asicSetNoBTFwOwnEn != NULL)
			if (prAdapter->chip_info->asicSetNoBTFwOwnEn(TRUE)
								== FALSE)
				goto FAIL;
#endif

		if (wlanOffAtReset(prNetDev) != WLAN_STATUS_SUCCESS)
			goto FAIL;

		glSetResettingFlag(prAdapter->prGlueInfo, FALSE);
		prAdapter->fgIsFwDownloaded = FALSE;

		if (HAL_TOGGLE_WFSYS_RST(prAdapter) != WLAN_STATUS_SUCCESS)
			goto FAIL;

	} else
		spin_unlock_bh(&prAdapter->rWfsysResetLock);

	if (GL_GET_WFSYS_RESET_POSTPONE(prAdapter))
		goto POSTPONE;

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_REINIT);

	HAL_RESUME_TX_RX(prAdapter);

	if (wlanOnAtReset(prNetDev) != WLAN_STATUS_SUCCESS)
		goto FAIL;

#if defined(_HIF_SDIO)
	/* please refer to 7961 sdio ser L0.5 program guide */
	if (prAdapter->chip_info->asicSetNoBTFwOwnEn != NULL)
		if (prAdapter->chip_info->asicSetNoBTFwOwnEn(FALSE) == FALSE)
			goto FAIL;
#endif

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);
#if CFG_SUPPORT_NAN
	mtk_cfg80211_vendor_event_nan_chip_reset(prAdapter, FALSE);
#endif

#if CFG_WIFI_TESTMODE_FW_REDOWNLOAD
	prGlueInfo->fgTestL0P5Done = TRUE;
	wake_up_interruptible(&prGlueInfo->waitQTestFwDl);
#endif

	DBGLOG(INIT, INFO, "[SER][L0.5] Reset done\n");

	DBGLOG(INIT, INFO, "[SER][L0.5] eWfsysResetState = %d\n",
					prAdapter->eWfsysResetState);

	DBGLOG(INIT, INFO, "[SER][L0.5] ucSerState = %d\n",
					prAdapter->ucSerState);

	DBGLOG(INIT, INFO, "[SER][L0.5] i4PendingFwdFrameCount=%d\n",
		prAdapter->rTxCtrl.i4PendingFwdFrameCount);

	DBGLOG(INIT, INFO, "[SER][L0.5] i4TxPendingFrameNum=%d\n",
		prAdapter->prGlueInfo->i4TxPendingFrameNum);

	DBGLOG(INIT, INFO, "[SER][L0.5] au4PseCtrlEnMap=%d\n",
		prAdapter->rTxCtrl.rTc.au4PseCtrlEnMap);

	return;

POSTPONE:
	DBGLOG(INIT, WARN, "[SER][L0.5] reset postpone\n");

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_POSTPONE);

	return;

FAIL:
	DBGLOG(INIT, ERROR, "\n[SER][L0.5] Reset fail !!!!\n\n");

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);
	glSetResettingFlag(prAdapter->prGlueInfo, FALSE);
	glSetSimplifyResetFlowFlag(prGlueInfo, FALSE);
	GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SER_L0P5_FAIL);

	return;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for wifi reset
 *
 * @param   skb
 *          info
 *
 * @retval  0
 *          nonzero
 */
/*----------------------------------------------------------------------------*/
static void mtk_wifi_reset_main(struct GLUE_INFO *prGlueInfo)
{
	u_int8_t fgResult = FALSE;

	if (prGlueInfo == NULL) {
		DBGLOG(INIT, ERROR, "input value prGlueInfo is NULL.\n");
		return;
	}

	/* If CFG_CHIP_RESET_KO_SUPPORT = 1,
	 * this function will only be invoked when the reset action fails to
	 * execute through resetko. In the case of USB HIF, if resetko fails
	 * to execute, the wifi driver can directly pull the reset pin.
	 */
#if defined(_HIF_USB) || (0 == CFG_CHIP_RESET_KO_SUPPORT)
	fgResult = rst_L0_notify_step1(prGlueInfo);

	if (fgResult == BT_RESET_NOT_OK) {
		DBGLOG(INIT, ERROR, "BT not ready to reset, skip reset.\n");
		glSetResettingFlag(prGlueInfo, FALSE);
		return;
	}
	wait_core_dump_end(prGlueInfo);

	if (prGlueInfo != NULL &&
		prGlueInfo->prAdapter != NULL &&
		prGlueInfo->prAdapter->chip_info != NULL &&
		prGlueInfo->prAdapter->chip_info
			->rst_L0_notify_step2 != NULL)
		fgResult = prGlueInfo->prAdapter
			->chip_info->rst_L0_notify_step2();

	if (is_bt_exist() == FALSE)
		kalRemoveProbe(prGlueInfo);

#endif

	if (kalGetSimplifyResetFlowFlag(prGlueInfo)) {
		DBGLOG(INIT, INFO, "Force down the reset flag.\n");
		glSetSimplifyResetFlowFlag(prGlueInfo, FALSE);
	}

	DBGLOG(INIT, STATE, "[SER][L0] flow end, fgResult=%d\n", fgResult);
}


#if CFG_CHIP_RESET_KO_SUPPORT
enum ReturnStatus glSendResetEvent(uint32_t bus_id,
				   enum ModuleType module,
				   enum ResetFsmEvent event)
{
#if defined(_HIF_USB) && (CFG_SUPPORT_MULTI_CARD == 1)
	return send_reset_event(bus_id, module, event);
#else
	return send_reset_event(module, event);
#endif
}

void resetkoNotifyFunc(unsigned int event, void *data)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ModuleMsg *msg;
#if defined(_HIF_SDIO)
	bool *ret;
#endif
#if defined(_HIF_USB) && (CFG_SUPPORT_MULTI_CARD == 1)
	uint32_t bus_id, tmp_bus_id;
	int i;

	bus_id = (event == MODULE_NOTIFY_MESSAGE) ?
		       ((struct ModuleMsg *)data)->bus_id : (*(uint32_t *)data);

	for (i = 0; i < CFG_MAX_WLAN_DEVICES; i++) {
		if (!arWlanDevInfo[i].prDev)
			continue;

		prDev = arWlanDevInfo[i].prDev
		prGlueInfo = *((struct GLUE_INFO **)netdev_priv(prDev));

		if (prGlueInfo != NULL &&
		    prGlueInfo->rHifInfo.intf != NULL &&
		    prGlueInfo->rHifInfo.udev != NULL &&
		    prGlueInfo->rHifInfo.udev->bus != NULL) {
			tmp_bus_id = glGetBusId(prGlueInfo->rHifInfo.intf);

			if (tmp_bus_id == bus_id)
				break;
		}
		prGlueInfo = NULL;
	}
#else
	uint32_t bus_id = 0;

	WIPHY_PRIV(wlanGetWiphy(gprWdev[0]), prGlueInfo);
#endif

	DBGLOG(INIT, INFO, "%s: %d\n", __func__, event);
	if (prGlueInfo == NULL || (prGlueInfo != NULL &&
	    P_CHIP_RESET_INFO(prGlueInfo)->fgIsInitialized == FALSE)) {
		DBGLOG(INIT, WARN, "%s: prGlueInfo not ready\n",
			__func__);
		return;
	}

	if (event == MODULE_NOTIFY_MESSAGE) {
		msg = (struct ModuleMsg *)data;
		if (msg == NULL || msg->input == NULL || msg->output == NULL)
			return;
#if defined(_HIF_SDIO)
		if (msg->msgId == BT_TO_WIFI_SET_WIFI_DRIVER_OWN) {
			ret = (bool *)(msg->output);
			glSetRstPreventFwOwn(prGlueInfo,
					     *((u_int8_t *)(msg->input)));
			*ret = TRUE;
		}
#endif
	} else if (event == MODULE_NOTIFY_PRE_POWER_OFF) {
		if (wlanIsProbingOrRemoving(TRUE, bus_id) ||
		    wlanIsProbingOrRemoving(FALSE, bus_id)) {
			P_CHIP_RESET_INFO(prGlueInfo)->fgIsPendingForReady =
									   TRUE;
		} else {
			glSetResettingFlag(prGlueInfo, TRUE);
			glSetSimplifyResetFlowFlag(prGlueInfo, FALSE);
			glSendResetEvent(bus_id,
					 RESET_MODULE_TYPE_WIFI,
					 RFSM_EVENT_READY);
		}
	} else if (event == MODULE_NOTIFY_RESET_DONE) {
		P_CHIP_RESET_INFO(prGlueInfo)->u4RstCount++;
		DBGLOG(INIT, INFO, "%s: reset count %d\n",
			__func__, P_CHIP_RESET_INFO(prGlueInfo)->u4RstCount);
	} else if (event == MODULE_NOTIFY_POWER_OFF_DONE) {
		P_CHIP_RESET_INFO(prGlueInfo)->u4PowerOffCount++;
		DBGLOG(INIT, INFO, "%s: power off count %d\n",
		      __func__, P_CHIP_RESET_INFO(prGlueInfo)->u4PowerOffCount);
	}
}

#endif

static u_int8_t is_bt_exist(void)
{
	char *bt_func_name = "btmtk_sdio_whole_reset";
	void *pvAddr = NULL;

#if	(CFG_ENABLE_GKI_SUPPORT != 1)
	pvAddr = GLUE_SYMBOL_GET(bt_func_name);
#endif
	if (!pvAddr)
		DBGLOG(INIT, ERROR, "[SER][L0] %s does not exist\n",
			bt_func_name);
	else {
#if	(CFG_ENABLE_GKI_SUPPORT != 1)
		GLUE_SYMBOL_PUT(bt_func_name);
#endif
		return TRUE;
	}

	return FALSE;

}

static u_int8_t rst_L0_notify_step1(struct GLUE_INFO *prGlueInfo)
{
	if (glGetRstReason(prGlueInfo->prAdapter) != RST_BT_TRIGGER) {
		typedef int (*p_bt_fun_type) (void *);
		p_bt_fun_type bt_func;
		char *bt_func_name = "btmtk_sdio_whole_reset";
		void *pvAddr = NULL;
		int ret = -1;

		DBGLOG(INIT, STATE, "[SER][L0] %s\n", bt_func_name);
#if	(CFG_ENABLE_GKI_SUPPORT != 1)
		pvAddr = GLUE_SYMBOL_GET(bt_func_name);
#endif
		if (pvAddr) {
			bt_func = (p_bt_fun_type) pvAddr;
			ret = bt_func(NULL);
#if	(CFG_ENABLE_GKI_SUPPORT != 1)
			GLUE_SYMBOL_PUT(bt_func_name);
#endif
			if (ret == 0)
				return BT_RESET_OK;
			else
				return BT_RESET_NOT_OK;
		} else {
			DBGLOG(INIT, ERROR,
				"[SER][L0] %s does not exist\n", bt_func_name);
			return BT_RESET_NOT_FOUND;
		}
	}

	return BT_RESET_OK;
}

static void wait_core_dump_end(struct GLUE_INFO *prGlueInfo)
{
#ifdef CFG_SUPPORT_CONNAC2X
	if (glGetRstReason(prGlueInfo->prAdapter) == RST_OID_TIMEOUT)
		return;
	DBGLOG(INIT, WARN, "[SER][L0] not support..\n");
#endif
}

int32_t BT_rst_L0_notify_WF_step1(int32_t reserved)
{
	glSetRstReason(NULL, RST_BT_TRIGGER);
	GL_DEFAULT_RESET_TRIGGER(NULL, RST_BT_TRIGGER);

	return 0;
}
EXPORT_SYMBOL(BT_rst_L0_notify_WF_step1);

int32_t BT_rst_L0_notify_WF_2(int32_t reserved)
{
	DBGLOG(INIT, WARN, "[SER][L0] not support...\n");

	return 0;
}
EXPORT_SYMBOL(BT_rst_L0_notify_WF_2);

#endif

