// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#include <linux/suspend.h>
#include <linux/workqueue.h>

#include "precomp.h"
#include "gl_coredump.h"
#include "gl_fw_log.h"
#include "gl_rst.h"
#include "wlan_pinctrl.h"

#if CFG_MTK_MDDP_SUPPORT
#include "mddp.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */



/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
u_int8_t fgSimplifyResetFlow;
uint64_t u8ResetTime;

#if CFG_CHIP_RESET_HANG
u_int8_t fgIsResetHangState = SER_L0_HANG_RST_NONE;
#endif

#if CFG_WMT_RESET_API_SUPPORT
wait_queue_head_t g_waitq_rst;
struct completion g_RstOffComp;
struct completion g_RstOnComp;
struct completion g_triggerComp;
KAL_WAKE_LOCK_T *g_IntrWakeLock;
struct task_struct *wlan_reset_thread;
u_int8_t g_IsWholeChipRst = FALSE;
u_int8_t g_SubsysRstCnt;
int g_SubsysRstTotalCnt;
int g_WholeChipRstTotalCnt;
u_int8_t g_IsSubsysRstOverThreshold = FALSE;
u_int8_t g_IsWfsysBusHang = FALSE;
char *g_reason;
char *g_WholeChipRstReason;
u_int8_t g_IsWfsysRstDone = TRUE;
u_int8_t g_fgRstRecover = FALSE;
uint8_t g_WholeChipRstType;
#endif

char * const apucRstReason[RST_REASON_MAX] = {
	"RST_UNKNOWN",
	"RST_PROCESS_ABNORMAL_INT",
	"RST_DRV_OWN_FAIL",
	"RST_FW_ASSERT",
	"RST_FW_ASSERT_TIMEOUT",
	"RST_BT_TRIGGER",
	"RST_OID_TIMEOUT",
	"RST_CMD_TRIGGER",
	"RST_REQ_CHL_FAIL",
	"RST_FW_DL_FAIL",
	"RST_SER_TIMEOUT",
	"RST_SLP_PROT_TIMEOUT",
	"RST_REG_READ_DEADFEED",
	"RST_P2P_CHNL_GRANT_INVALID_TYPE",
	"RST_P2P_CHNL_GRANT_INVALID_STATE",
	"RST_SCAN_RECOVERY",
	"RST_ACCESS_REG_FAIL",
	"[Wi-Fi On] nicpmSetDriverOwn() failed!",
	"[Wi-Fi] [Read WCIR_WLAN_READY fail!]",
	"[Wi-Fi Off] Allocate CMD_INFO_T ==> FAILED.",
	"RST_SDIO_RX_ERROR",
	"RST_WHOLE_CHIP_TRIGGER",
	"RST_MDDP_EXCEPTION",
	"RST_MDDP_MD_TRIGGER_EXCEPTION",
	"RST_FWK_TRIGGER",
	"RST_SER_L1_FAIL",
	"RST_SER_L0P5_FAIL",
	"RST_CMD_EVT_FAIL",
	"RST_WDT",
	"RST_SUBSYS_BUS_HANG",
	"RST_SMC_CMD_FAIL",
	"RST_DEVAPC",
	"RST_PCIE_NOT_READY",
	"Chip reset by AER",
	"Chip reset by AER - MalfTLP",
	"Chip reset by AER - RxErr",
	"Chip reset by AER - SDES",
	"RST_MMIO_READ",
	"RST_WFDMA_RX_HANG",
	"RST_MAWD_WAKEUP_FAIL",
	"RST_RFB_FAIL",
};

static const char *const apucRstAction[] = {
	"RST_FLAG_DO_CORE_DUMP",
	"RST_FLAG_PREVENT_POWER_OFF",
	"RST_FLAG_DO_WHOLE_RESET",
	"RST_FLAG_DO_L0P5_RESET",
	"RST_FLAG_DO_L1_RESET",
};

u_int8_t g_IsNeedWaitCoredump = FALSE;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static enum _ENUM_CHIP_RESET_REASON_TYPE_T eResetReason;

#if CFG_CHIP_RESET_SUPPORT
static struct RESET_STRUCT wifi_rst;
u_int8_t fgIsResetting;
u_int8_t fgIsL0Resetting;
u_int8_t fgIsResetOnEnd;
u_int8_t fgIsDrvTriggerWholeChipReset;
enum COREDUMP_SOURCE_TYPE g_Coredump_source;
u_int8_t fgIsRstPreventFwOwn;
static uint32_t u4ProbeCount;
#if CFG_CHIP_RESET_KO_SUPPORT
static uint32_t u4RstCount;
static uint32_t u4PowerOffCount;
static u_int8_t fgIsPendingForReady;
#endif
enum ENUM_COREDUMP_BY_CHIP_RESET_TYPE_T g_Coredump_type;
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT
static void mtk_wifi_reset(struct work_struct *work);
static void mtk_wifi_reset_main(struct RESET_STRUCT *rst,
					u_int8_t fgL0Reset);

#if CFG_WMT_RESET_API_SUPPORT
#if CFG_SUPPORT_CONNAC1X
static void mtk_wifi_trigger_reset(struct work_struct *work);
static void glResetCallback(enum _ENUM_WMTDRV_TYPE_T eSrcType,
			     enum _ENUM_WMTDRV_TYPE_T eDstType,
			     enum _ENUM_WMTMSG_TYPE_T eMsgType, void *prMsgBody,
			     unsigned int u4MsgLength);
#endif /* CFG_SUPPORT_CONNAC1X */
#ifdef CONFIG_PM
static int wlan_pm_notifier_call(struct notifier_block *notifier,
	unsigned long pm_event, void *unused);
#endif
static void reset_dump_pending_req(struct reset_pending_req *req);
static uint32_t reset_handle_pending_req(void);
static uint32_t reset_store_pending_req(struct RESET_STRUCT *rst,
	const uint32_t flag,
	const uint8_t *file,
	const uint32_t line,
	const u_int8_t fw_acked);
#else /* CFG_WMT_RESET_API_SUPPORT */
#if (CFG_CHIP_RESET_KO_SUPPORT == 0)
static u_int8_t is_bt_exist(void);
static u_int8_t rst_L0_notify_step1(void);
static void wait_core_dump_end(void);
#endif /* CFG_CHIP_RESET_KO_SUPPORT */
#endif /* CFG_WMT_RESET_API_SUPPORT */
#endif /* CFG_CHIP_RESET_SUPPORT */

#if (CFG_SUPPORT_SER_DEBUGFS == 1)
int32_t resetCreateSerDbgFs(struct GLUE_INFO *prGlueInfo);
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
void glSetIsNeedWaitCoredumpFlag(uint8_t status)
{
	g_IsNeedWaitCoredump = status;
	DBGLOG(INIT, TRACE, "isNeedWaitCoredump: %u\n", g_IsNeedWaitCoredump);
}

void glSetRstReason(enum _ENUM_CHIP_RESET_REASON_TYPE_T
		    eReason)
{
	if (kalIsResetting())
		return;

	u8ResetTime = sched_clock();
	eResetReason = eReason;
}

int glGetRstReason(void)
{
	return eResetReason;
}

u_int8_t kalIsRstPreventFwOwn(void)
{
#if CFG_CHIP_RESET_SUPPORT
	return fgIsRstPreventFwOwn;
#else
	return FALSE;
#endif

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for checking if connectivity chip is
 *        reset on ended
 * @param   None
 *
 * @retval  TRUE
 *          FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsResetOnEnd(void)
{
#if CFG_CHIP_RESET_SUPPORT
	return fgIsResetOnEnd;
#else
	return FALSE;
#endif
}

void glResetOnEndUpdateFlag(u_int8_t reset_on_end)
{
	DBGLOG(INIT, TRACE, "reset_on_end: %d\n", reset_on_end);
	fgIsResetOnEnd = reset_on_end;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for checking if connectivity chip is resetting
 *
 * @param   None
 *
 * @retval  TRUE
 *          FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalIsResetting(void)
{
#if CFG_CHIP_RESET_SUPPORT
	return fgIsResetting;
#else
	return FALSE;
#endif
}

void glResetUpdateFlag(u_int8_t reset)
{
	DBGLOG(INIT, TRACE, "reset: %d\n", reset);
	fgIsResetting = reset;
#if CFG_MTK_ANDROID_WMT && !CFG_SUPPORT_CONNAC1X
	update_driver_reset_status(fgIsResetting);
#endif
}

void glResetCleanResetFlag(void)
{
	glResetUpdateFlag(FALSE);
	glResetOnEndUpdateFlag(FALSE);
#if CFG_WMT_RESET_API_SUPPORT
	g_IsWfsysBusHang = FALSE;
#endif
}

u_int8_t glIsL0Resetting(void)
{
#if CFG_CHIP_RESET_SUPPORT
	return fgIsL0Resetting;
#else
	return FALSE;
#endif
}

void glResetUpdateL0Flag(u_int8_t status)
{
	DBGLOG(INIT, TRACE, "L0 reset flag: %d\n", status);
	fgIsL0Resetting = status;
#if CFG_MTK_ANDROID_WMT && !CFG_SUPPORT_CONNAC1X
	update_whole_chip_rst_status(fgIsL0Resetting);
#endif
}

#if CFG_CHIP_RESET_SUPPORT
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. register wifi reset callback
 *        2. initialize wifi reset work
 *
 * @param none
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetInit(struct GLUE_INFO *prGlueInfo)
{
#if CFG_WMT_RESET_API_SUPPORT && defined(CONFIG_PM)
	int ret = 0;
#endif

#if CFG_WMT_RESET_API_SUPPORT
#if CFG_SUPPORT_CONNAC1X
	/* 1. Register reset callback */
	mtk_wcn_wmt_msgcb_reg(WMTDRV_TYPE_WIFI, glResetCallback);
	/* 2. Initialize reset work */
	INIT_WORK(&(wifi_rst.rst_trigger_work),
		  mtk_wifi_trigger_reset);
#endif
#endif
	INIT_WORK(&(wifi_rst.rst_work), mtk_wifi_reset);
	fgSimplifyResetFlow = FALSE;
	fgIsDrvTriggerWholeChipReset = FALSE;
	glResetCleanResetFlag();

	fgIsRstPreventFwOwn = FALSE;
	wifi_rst.prGlueInfo = prGlueInfo;
	u4ProbeCount = 0;

#if CFG_TESTMODE_L0P5_FWDL_SUPPORT
	init_waitqueue_head(&prGlueInfo->waitQTestFwDl);
#endif
#if CFG_CHIP_RESET_KO_SUPPORT
	u4RstCount = 0;
	u4PowerOffCount = 0;
	fgIsPendingForReady = FALSE;
#endif
#if CFG_WMT_RESET_API_SUPPORT
	init_completion(&wifi_rst.halt_comp);
	KAL_WAKE_LOCK_INIT(NULL, g_IntrWakeLock, "WLAN Reset");
	init_waitqueue_head(&g_waitq_rst);
	init_completion(&g_RstOffComp);
	init_completion(&g_RstOnComp);
	init_completion(&g_triggerComp);
	wlan_reset_thread = kthread_run(wlan_reset_thread_main,
					&wifi_rst,
					"wlan_rst_thread");
#ifdef CONFIG_PM
	wifi_rst.pm_nb.notifier_call = wlan_pm_notifier_call;
	ret = register_pm_notifier(&wifi_rst.pm_nb);
	if (ret)
		DBGLOG(INIT, WARN,
			"register pm notifier failed\n");
#endif
#endif
	wifi_coredump_init(prGlueInfo);
#if (CFG_SUPPORT_SER_DEBUGFS == 1)
	resetCreateSerDbgFs(prGlueInfo);
#endif
	wifi_rst.fgIsInitialized = TRUE;
}

void glReseProbeRemoveDone(struct GLUE_INFO *prGlueInfo, int32_t i4Status,
			   u_int8_t fgIsProbe)
{
	if (!prGlueInfo)
		return;

	if (fgIsProbe) {
		u4ProbeCount++;
		DBGLOG(INIT, WARN,
			"[SER][L0] %s: probe count %d, status %d\n",
			__func__, u4ProbeCount, i4Status);
#if CFG_CHIP_RESET_KO_SUPPORT
		if (i4Status == WLAN_STATUS_SUCCESS) {
			send_reset_event(RESET_MODULE_TYPE_WIFI,
					 RFSM_EVENT_PROBED);
#if defined(_HIF_SDIO)
			update_hif_info(HIF_INFO_SDIO_HOST,
					prGlueInfo->rHifInfo.func);
#endif
		}
	} else {
		send_reset_event(RESET_MODULE_TYPE_WIFI, RFSM_EVENT_REMOVED);
	}

	if (fgIsPendingForReady) {
		fgIsPendingForReady = FALSE;
		glResetUpdateFlag(TRUE);
		send_reset_event(RESET_MODULE_TYPE_WIFI, RFSM_EVENT_READY);
#endif
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is responsible for
 *        1. deregister wifi reset callback
 *
 * @param none
 *
 * @retval none
 */
/*----------------------------------------------------------------------------*/
void glResetUninit(void)
{
#if CFG_WMT_RESET_API_SUPPORT
	struct RESET_STRUCT *rst = &wifi_rst;
#endif

	wifi_rst.fgIsInitialized = FALSE;
	wifi_coredump_deinit();

#if CFG_WMT_RESET_API_SUPPORT
#ifdef CONFIG_PM
	unregister_pm_notifier(&wifi_rst.pm_nb);
#endif
	if (rst->pending_req) {
		kalMemFree(rst->pending_req, VIR_MEM_TYPE,
			sizeof(*rst->pending_req));
		rst->pending_req = NULL;
	}
	set_bit(GLUE_FLAG_HALT_BIT, &rst->ulFlag);
	wake_up_interruptible(&g_waitq_rst);
	wait_for_completion_interruptible(&rst->halt_comp);
#if CFG_SUPPORT_CONNAC1X
	mtk_wcn_wmt_msgcb_unreg(WMTDRV_TYPE_WIFI);
#endif
#endif
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is called for generating reset request to WMT
 *
 * @param   None
 *
 * @retval  None
 */
/*----------------------------------------------------------------------------*/
void glSendResetRequest(void)
{
#if CFG_WMT_RESET_API_SUPPORT

	/* WMT thread would trigger whole chip reset itself */
#endif
}

#if CFG_WMT_RESET_API_SUPPORT
#ifdef CONFIG_PM
static uint8_t *pm_evt_to_str(const unsigned long pm_event)
{
	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return "PM_HIBERNATION_PREPARE";
	case PM_POST_HIBERNATION:
		return "PM_POST_HIBERNATION";
	case PM_SUSPEND_PREPARE:
		return "PM_SUSPEND_PREPARE";
	case PM_POST_SUSPEND:
		return "PM_POST_SUSPEND";
	case PM_RESTORE_PREPARE:
		return "PM_RESTORE_PREPARE";
	case PM_POST_RESTORE:
		return "PM_POST_RESTORE";
	default:
		return "PM_UNKNOWN";
	}
}

static int wlan_pm_notifier_call(struct notifier_block *notifier,
	unsigned long pm_event, void *unused)
{
	struct RESET_STRUCT *rst;

	rst = CONTAINER_OF(notifier, struct RESET_STRUCT, pm_nb);

	DBGLOG(REQ, INFO, "pm_event: %lu %s\n",
		pm_event, pm_evt_to_str(pm_event));

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		rst->is_suspend = TRUE;
		set_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
			&rst->prGlueInfo->fgIsInSuspend);
		set_bit(SUSPEND_FLAG_FOR_WAKEUP_REASON,
			&rst->prGlueInfo->fgIsInSuspend);
		break;
	case PM_POST_SUSPEND:
		rst->is_suspend = FALSE;
		clear_bit(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
			&rst->prGlueInfo->fgIsInSuspend);
		reset_handle_pending_req();
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}
#endif

/* Separate function definitions by CFG_WMT_RESET_API_SUPPORT
 * into 2 categories : mobile and ce.
 * If any merge or refactor is possible, then feel free to do it.
 * The following definition is of mobile.
 */
static void reset_dump_pending_req(struct reset_pending_req *req)
{
	if (!req)
		return;

	DBGLOG(INIT, INFO, "====== DUMP PENDING RESET REQ ======\n");

	DBGLOG(INIT, INFO, "\treq: 0x%p\n", req);
	DBGLOG(INIT, INFO, "\tflag: 0x%x\n", req->flag);
	DBGLOG(INIT, INFO, "\tfile: %s#%d\n", req->file, req->line);
	DBGLOG(INIT, INFO, "\tfw_acked: %d\n", req->fw_acked);

	DBGLOG(INIT, INFO, "============= DUMP END ===========\n");
}

static uint32_t reset_handle_pending_req(void)
{
	struct RESET_STRUCT *rst = &wifi_rst;
	struct reset_pending_req *req = rst->pending_req;

	if (!req) {
		return WLAN_STATUS_INVALID_DATA;
	}

	if (rst->is_suspend) {
		DBGLOG(INIT, ERROR,
			"Pending reset request only handled in resume mode.\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	DBGLOG(INIT, INFO, "\n");

	reset_dump_pending_req(req);

	kalSetRstEvent(req->fw_acked);

	kalMemFree(req, VIR_MEM_TYPE, sizeof(*req));
	rst->pending_req = NULL;

	return WLAN_STATUS_SUCCESS;
}

static uint32_t reset_store_pending_req(struct RESET_STRUCT *rst,
	const uint32_t flag,
	const uint8_t *file,
	const uint32_t line,
	const u_int8_t fw_acked)
{
	struct reset_pending_req *req = rst->pending_req;
	uint32_t status = WLAN_STATUS_SUCCESS;

	if (!IS_ENABLED(_HIF_PCIE)) {
		status = WLAN_STATUS_NOT_ACCEPTED;
		goto exit;
	}

	if (!rst->is_suspend) {
		DBGLOG(INIT, ERROR,
			"Pending reset request only accepted in suspend mode.\n");
		status = WLAN_STATUS_NOT_ACCEPTED;
		goto exit;
	}

	if (req) {
		DBGLOG(INIT, WARN,
			"pending reset request NOT handled.\n");
		reset_dump_pending_req(req);
		kalMemFree(req, VIR_MEM_TYPE, sizeof(*req));
		rst->pending_req = NULL;
	}

	req = kalMemZAlloc(sizeof(*req), VIR_MEM_TYPE);
	if (!req) {
		DBGLOG(INIT, ERROR,
			"Alloc pending reset req failed.\n");
		status = WLAN_STATUS_RESOURCES;
		goto exit;
	}

	req->flag = flag;
	req->fw_acked = fw_acked;
	kalSnprintf(req->file, sizeof(req->file), "%s", file);
	req->line = line;
	req->fw_acked = fw_acked;

	DBGLOG(INIT, INFO, "Store pending req: 0x%p.\n", req);
	rst->pending_req = req;

exit:
	return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Decide reset action according to trigger reset reason.
 *
 * @param   prAdapter
 *
 * @retval  reset action like RST_FLAG_DO_CORE_DUMP,
 *          or RST_FLAG_PREVENT_POWER_OFF, ..., etc.
 */
/*----------------------------------------------------------------------------*/
uint32_t glResetSelectAction(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4RstFlag = RST_FLAG_CHIP_RESET;

	if (prAdapter == NULL)
		prAdapter = wifi_rst.prGlueInfo->prAdapter;

	prChipInfo = prAdapter->chip_info;

	switch (glGetRstReason()) {
	case RST_PROCESS_ABNORMAL_INT:
	case RST_SDIO_RX_ERROR:
		u4RstFlag = RST_FLAG_DO_CORE_DUMP;
		break;

	case RST_DRV_OWN_FAIL:
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#if CFG_SUPPORT_CONNAC1X
		u4RstFlag = RST_FLAG_CHIP_RESET;
#else
		u4RstFlag = RST_FLAG_WF_RESET;
#endif
#elif defined(_HIF_SDIO)
		u4RstFlag = RST_FLAG_DO_CORE_DUMP;
#endif
		break;

	case RST_FW_DL_FAIL:
	case RST_WIFI_ON_DRV_OWN_FAIL:
	case RST_WHOLE_CHIP_TRIGGER:
	case RST_FWK_TRIGGER:
	case RST_SMC_CMD_FAIL:
	case RST_PCIE_NOT_READY:
	case RST_REQ_CHL_FAIL:
	case RST_P2P_CHNL_GRANT_INVALID_TYPE:
	case RST_P2P_CHNL_GRANT_INVALID_STATE:
	case RST_SCAN_RECOVERY:
	case RST_CMD_EVT_FAIL:
	case RST_RFB_FAIL:
#if CFG_SUPPORT_CONNAC1X
		u4RstFlag = RST_FLAG_CHIP_RESET;
#else
		u4RstFlag = RST_FLAG_WF_RESET;
#endif
		break;

	case RST_SER_TIMEOUT:
#if CFG_SUPPORT_CONNAC1X
		u4RstFlag = RST_FLAG_CHIP_RESET;
#else
		u4RstFlag = RST_FLAG_DO_CORE_DUMP;
#endif
		break;

	case RST_ACCESS_REG_FAIL:
	case RST_CHECK_READY_BIT_TIMEOUT:
		u4RstFlag = RST_FLAG_DO_CORE_DUMP | RST_FLAG_PREVENT_POWER_OFF;
		break;

	case RST_BT_TRIGGER:
	case RST_OID_TIMEOUT:
	case RST_CMD_TRIGGER:
	case RST_SLP_PROT_TIMEOUT:
	case RST_REG_READ_DEADFEED:
	default:
		u4RstFlag = RST_FLAG_CHIP_RESET;
		break;
	}

	return u4RstFlag;
}

uint32_t glResetTriggerImpl(struct ADAPTER *prAdapter,
		uint32_t u4RstFlag, const uint8_t *pucFile, uint32_t u4Line)
{
#define UPGRATE_TO_L0_PATTERN " - Upgrade to L0"

	struct RESET_STRUCT *rst = &wifi_rst;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps = NULL;
#if CFG_SUPPORT_PCIE_ASPM
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
#endif
	uint32_t rst_evt_send = WLAN_STATUS_NOT_ACCEPTED;
#if !CFG_SUPPORT_CONNAC1X
	int ret = 0;
#endif

	if (kalIsResetting() || kalIsResetOnEnd()) {
		DBGLOG(INIT, INFO, "already in reset\n");
		goto exit;
	}

	if (prAdapter) {
		prChipInfo = prAdapter->chip_info;

		/* L0.5 upgrade to L0 */
		if (g_IsWholeChipRst == FALSE &&
			g_IsWfsysBusHang == FALSE &&
			prChipInfo->isUpgradeWholeChipReset) {
			if (prChipInfo->isUpgradeWholeChipReset(prAdapter)) {
				char reason[64] = {0};

				u4RstFlag |= RST_FLAG_WHOLE_RESET;
				if (eResetReason == RST_CMD_TRIGGER) {
					kalSnprintf(&reason, sizeof(reason),
						"%s%s",
						apucRstReason[eResetReason],
						UPGRATE_TO_L0_PATTERN);
					glSetRstReasonString(reason);
				} else
					glSetRstReasonString(
						apucRstReason[eResetReason]);
			}
		}
	}

#if CFG_MTK_MDDP_SUPPORT
#if (CFG_PCIE_GEN_SWITCH == 1)
	pcie_gen_switch_recover(prAdapter);
	mddpNotifyMDGenSwitchEnd(prAdapter);
#endif /* CFG_PCIE_GEN_SWITCH */
#endif /* CFG_MTK_MDDP_SUPPORT */

#if WLAN_INCLUDE_SYS
	sysResetTrigger();
#endif
#if CFG_MTK_ANDROID_WMT && !CFG_SUPPORT_CONNAC1X

	glResetUpdateFlag(TRUE);
	glResetOnEndUpdateFlag(TRUE);

	/* Avoid doing reset triggered by CMD when WIFI write is processing */
	if (get_wifi_process_status() &&
	   (eResetReason == RST_CMD_TRIGGER ||
	    eResetReason == RST_FWK_TRIGGER)) {
		DBGLOG(INIT, INFO, "cmd trigger in write processing\n");
		glResetUpdateFlag(FALSE);
		glResetCleanResetFlag();
		goto exit;
	}
#endif

#if CFG_MTK_MDDP_SUPPORT
	mddpNotifyWifiReset();
#endif

	if (kalGetShutdownState()) {
		DBGLOG(INIT, INFO, "skip in shutdown\n");
		glResetCleanResetFlag();
		goto exit;
	}

#if CFG_SUPPORT_CONNAC1X
	if (eResetReason != RST_BT_TRIGGER)
		DBGLOG(INIT, STATE, "[SER][L0] wifi trigger eResetReason=%d\n",
								eResetReason);
	else
		DBGLOG(INIT, STATE, "[SER][L0] BT trigger\n");
#endif

#if CFG_WMT_RESET_API_SUPPORT
	if (u4RstFlag & RST_FLAG_DO_CORE_DUMP)
		if (glIsWmtCodeDump())
			DBGLOG(INIT, WARN, "WMT is code dumping !\n");
#endif

	dump_stack();
	DBGLOG(INIT, ERROR,
		"Trigger chip reset in %s#%u, bus[%d] flag[0x%x] reason[%s]\n",
		pucFile,
		u4Line,
		g_IsWfsysBusHang,
		u4RstFlag,
		apucRstReason[eResetReason]);

	if (prAdapter) {
		prDbgOps = prChipInfo->prDebugOps;

		if (prDbgOps && prDbgOps->dumpBusHangCr)
			prDbgOps->dumpBusHangCr(prAdapter);

		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		halPrintHifDbgInfo(prAdapter);

		/* fix AER in debug sop dump, need upgrade to L0 */
		if (g_IsWholeChipRst == FALSE &&
		    fgIsBusAccessFailed == TRUE) {
			u4RstFlag |= RST_FLAG_WHOLE_RESET;
			if (eResetReason == RST_CMD_TRIGGER) {
				char reason[64] = {0};

				kalSnprintf(&reason, sizeof(reason),
					"%s%s",
					apucRstReason[eResetReason],
					UPGRATE_TO_L0_PATTERN);
				glSetRstReasonString(reason);
			} else
				glSetRstReasonString(
					apucRstReason[eResetReason]);
		}
	}

#if CFG_SUPPORT_CONNAC1X
	rst->rst_trigger_flag = u4RstFlag;
	schedule_work(&rst->rst_trigger_work);
#else
	if (u4RstFlag & RST_FLAG_DO_CORE_DUMP)
		g_fgRstRecover = FALSE;
	else
		g_fgRstRecover = TRUE;

	if (u4RstFlag & RST_FLAG_DO_WHOLE_RESET) {
		glResetWholeChipResetTrigger(g_reason);
		glResetCleanResetFlag();
		goto exit;
	}

	g_Coredump_source = COREDUMP_SOURCE_WF_DRIVER;
	if (!prAdapter || !prChipInfo->trigger_fw_assert) {
		DBGLOG(INIT, ERROR,
			"No impl. of trigger_fw_assert API\n");
		glResetCleanResetFlag();
		goto exit;
	}

	reinit_completion(&g_triggerComp);
	ret = prChipInfo->trigger_fw_assert(prAdapter);
	if (ret == -EBUSY) {
		glResetCleanResetFlag();
		goto exit;
	}

#if CFG_MTK_WIFI_PCIE_SUPPORT
	/* Check MCU off */
	if (prAdapter->chip_info->checkmcuoff) {
		kalMdelay(500);
		prAdapter->chip_info->checkmcuoff(prAdapter);
	}
#endif

	if (rst->is_suspend) {
		uint32_t status;

		status = reset_store_pending_req(rst,
						 u4RstFlag,
						 pucFile,
						 u4Line,
						 ret != -ETIMEDOUT);
		if (status == WLAN_STATUS_SUCCESS) {
			rst_evt_send = WLAN_STATUS_SUCCESS;
			goto exit;
		}
	}

	if (ret == -ETIMEDOUT)
		kalSetRstEvent(FALSE);
	else
		kalSetRstEvent(TRUE);
	rst_evt_send = WLAN_STATUS_SUCCESS;
#endif
exit:
#if CFG_SUPPORT_PCIE_ASPM
	if (prAdapter) {
		prGlueInfo = prAdapter->prGlueInfo;
		if (prGlueInfo && prChipInfo &&
			prChipInfo->bus_info) {
			prBusInfo = prChipInfo->bus_info;
			if (prBusInfo->configPcieAspm)
				prBusInfo->configPcieAspm(prGlueInfo,
					TRUE, 3);
		}
	}
#endif
	fgIsMcuOff = FALSE;
	return rst_evt_send;
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
uint32_t glResetTrigger(struct ADAPTER *prAdapter,
		uint32_t u4RstFlag, const uint8_t *pucFile, uint32_t u4Line)
{
	if (g_IsWholeChipRst &&
		eResetReason != RST_WHOLE_CHIP_TRIGGER) {
		DBGLOG(INIT, INFO, "whole chip rst on-going, skip %s\n",
			apucRstReason[eResetReason]);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	return glResetTriggerImpl(prAdapter, u4RstFlag, pucFile, u4Line);
}
#else
/* The following definition is of ce. */

/*----------------------------------------------------------------------------*/
/*!
 * @brief Decide reset action according to trigger reset reason.
 *
 * @param   prAdapter
 *
 * @retval  reset action like RST_FLAG_DO_CORE_DUMP,
 *          or RST_FLAG_PREVENT_POWER_OFF, ..., etc.
 */
/*----------------------------------------------------------------------------*/
uint32_t glResetSelectAction(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4RstFlag = 0;

	if (prAdapter == NULL)
		prAdapter = wifi_rst.prGlueInfo->prAdapter;

	prChipInfo = prAdapter->chip_info;

	switch (glGetRstReason()) {
	case RST_PROCESS_ABNORMAL_INT:
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
		u4RstFlag = RST_FLAG_DO_L1_RESET;
#elif defined(_HIF_SDIO)
		if (prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		else
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
#else
		/* print log but don't execute SER */
		DBGLOG(INIT, WARN,
		       "USB shall not happen abnormal interrupt\n");
#endif
		break;

	case RST_DRV_OWN_FAIL:
#if defined(_HIF_PCIE) || defined(_HIF_AXI) || defined(_HIF_SDIO)
		if (prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		else
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
#else
		/* print log but don't execute SER */
		DBGLOG(INIT, WARN,
			   "USB shall not happen set drv own fail\n");
#endif
		break;

	case RST_SDIO_RX_ERROR:
#if defined(_HIF_SDIO)
		if (prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		else
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
#else
		/* When 20201225, HIF other than SDIO doesn't have this error.
		 * If this isn't TRUE in the future, then we need to decide
		 * the appropriate reset at that time.
		 */
#endif
		break;

	case RST_FW_ASSERT:
		/* If L0.5 is supported, then L0.5 reset will be triggered
		 * automatically by RST_WDT. Otherwise, execute L0 reset.
		 */
		if (!prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
		break;

	case RST_FW_ASSERT_TIMEOUT:
	case RST_OID_TIMEOUT:
	case RST_SER_TIMEOUT:
	case RST_SER_L1_FAIL:
	case RST_CMD_EVT_FAIL:
	case RST_WDT:
	case RST_SUBSYS_BUS_HANG:
		if (prChipInfo->fgIsSupportL0p5Reset)
			u4RstFlag = RST_FLAG_DO_L0P5_RESET;
		else
			u4RstFlag = RST_FLAG_DO_WHOLE_RESET;
		break;

	case RST_BT_TRIGGER:
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
uint32_t glResetTrigger(struct ADAPTER *prAdapter, uint32_t u4RstFlag,
		    const uint8_t *pucFile, uint32_t u4Line)
{
	uint16_t i;
	struct CHIP_DBG_OPS *prChipDbg;
	uint16_t u2FwOwnVersion;
	uint16_t u2FwPeerVersion;
	u_int8_t fgDrvOwn;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	if (wifi_rst.fgIsInitialized != TRUE)
		return rStatus;
	if (kalIsResetting()
#if CFG_DC_USB_WOW_CALLBACK
	|| prAdapter->prGlueInfo->rHifInfo.fgUsbShutdown
#endif
	)
		return rStatus;
	if ((u4RstFlag & RST_FLAG_DO_WHOLE_RESET) ||
	    (u4RstFlag & RST_FLAG_DO_L0P5_RESET))
		glResetUpdateFlag(TRUE);
	dump_stack();

	if (eResetReason >= 0 && eResetReason < RST_REASON_MAX)
		DBGLOG(INIT, ERROR,
		       "Trigger reset in %s line %u reason %s\n",
		       pucFile, u4Line, apucRstReason[eResetReason]);
	else
		DBGLOG(INIT, ERROR,
		       "Trigger reset in %s line %u but unsupported reason %d\n",
		       pucFile, u4Line, eResetReason);
	for (i = 0; i < ARRAY_SIZE(apucRstAction); i++)
		if (u4RstFlag & BIT(i))
			DBGLOG(INIT, ERROR, "action %s\n", apucRstAction[i]);

	if (!u4RstFlag) {
		DBGLOG(INIT, ERROR, "no action\n");
		return rStatus;
	}

#if CFG_CHIP_RESET_KO_SUPPORT
	if (u4RstFlag & RST_FLAG_DO_WHOLE_RESET) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		if (prAdapter->eWfsysResetState != WFSYS_RESET_STATE_IDLE) {
			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);
			DBGLOG(INIT, ERROR, "Ignore L0 during L0.5\n");
			return rStatus;
		}
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);

		glResetUpdateFlag(TRUE);
		fgSimplifyResetFlow = FALSE;
		send_reset_event(RESET_MODULE_TYPE_WIFI,
				 RFSM_EVENT_TRIGGER_RESET);

		return rStatus;
	}
#endif

	if (prAdapter == NULL)
		prAdapter = wifi_rst.prGlueInfo->prAdapter;
	prChipDbg = prAdapter->chip_info->prDebugOps;
	u2FwOwnVersion = prAdapter->rVerInfo.u2FwOwnVersion;
	u2FwPeerVersion = prAdapter->rVerInfo.u2FwPeerVersion;
	fgDrvOwn = TRUE;

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

	if (prChipDbg->show_mcu_debug_info) {
		HAL_LP_OWN_RD(prAdapter, &fgDrvOwn);
		if (fgDrvOwn)
			prChipDbg->show_mcu_debug_info(prAdapter, NULL, 0,
						       DBG_MCU_DBG_ALL, NULL);
		else
			DBGLOG(INIT, INFO,
			       "[SER] not drv own, cannot get mcu info\n");
	}

	if (u4RstFlag & RST_FLAG_DO_WHOLE_RESET) {
		glResetUpdateFlag(TRUE);
		fgSimplifyResetFlow = FALSE;
		wifi_rst.prGlueInfo = prAdapter->prGlueInfo;
		schedule_work(&(wifi_rst.rst_work));
	} else if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
	      prAdapter->chip_info->fgIsSupportL0p5Reset) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		glResetUpdateFlag(TRUE);
		fgIsRstPreventFwOwn = TRUE;

		if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_IDLE) {
			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);
			glSetWfsysResetState(prAdapter,
					     WFSYS_RESET_STATE_DETECT);

			schedule_work(&prAdapter->prGlueInfo->rWfsysResetWork);
		} else
			KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
				SPIN_LOCK_WFSYS_RESET);
	} else if (u4RstFlag & RST_FLAG_DO_L1_RESET) {
		wlanoidSerExtCmd(prAdapter, SER_ACTION_RECOVER,
					     SER_SET_L1_RECOVER, 0);
	}
	return rStatus;
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
void glSetWfsysResetState(struct ADAPTER *prAdapter,
			  enum ENUM_WFSYS_RESET_STATE_TYPE_T state)
{
	if (state >= WFSYS_RESET_STATE_MAX) {
		DBGLOG(INIT, WARN,
			"[SER][L0.5] unsupported wfsys reset state\n");
	} else {
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		prAdapter->eWfsysResetState = state;
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
	}
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
u_int8_t glReSchWfsysReset(struct ADAPTER *prAdapter)
{
	u_int8_t fgReSch = FALSE;

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
		SPIN_LOCK_WFSYS_RESET);

	if (prAdapter->eWfsysResetState == WFSYS_RESET_STATE_POSTPONE) {
		fgReSch = TRUE;
		schedule_work(&prAdapter->prGlueInfo->rWfsysResetWork);
	}

	KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
		SPIN_LOCK_WFSYS_RESET);

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
void WfsysResetHdlr(struct work_struct *work)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;
	struct mt66xx_hif_driver_data *prHifDrvData;

	prGlueInfo = CONTAINER_OF(work, struct GLUE_INFO, rWfsysResetWork);
	prAdapter = prGlueInfo->prAdapter;
	prHifDrvData = CONTAINER_OF(&prAdapter->chip_info,
				    struct mt66xx_hif_driver_data, chip_info);

	DBGLOG(INIT, STATE, "[SER][L0.5] Reset triggered eWfsysResetState=%d\n",
				prAdapter->eWfsysResetState);

	KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
		SPIN_LOCK_WFSYS_RESET);

	if (prAdapter->eWfsysResetState != WFSYS_RESET_STATE_POSTPONE) {
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);

		if (prAdapter->u2WfsysResetCnt < 0xFFFF)
			prAdapter->u2WfsysResetCnt++;

		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		halPrintHifDbgInfo(prAdapter);

		glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_RESET);
#if CFG_SUPPORT_WED_PROXY
		wedHwRecoveryFromError(prAdapter, WIFI_ERR_RECOV_DETACH);
#endif
		HAL_CANCEL_TX_RX(prAdapter);

		if (wlanOffAtReset() != WLAN_STATUS_SUCCESS)
			goto FAIL;

		glResetUpdateFlag(FALSE);
		prAdapter->fgIsFwDownloaded = FALSE;

		if (HAL_TOGGLE_WFSYS_RST(prAdapter) != WLAN_STATUS_SUCCESS)
			goto FAIL;
	} else
		KAL_RELEASE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);

	if (kalCheckWfsysResetPostpone(prGlueInfo))
		goto POSTPONE;

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_REINIT);

	HAL_RESUME_TX_RX(prAdapter);

	if (wlanOnAtReset() != WLAN_STATUS_SUCCESS)
		goto FAIL;
#if CFG_SUPPORT_WED_PROXY
	wedHwRecoveryFromError(prAdapter, WIFI_ERR_RECOV_ATTACH);
#endif
	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);

#if CFG_TESTMODE_L0P5_FWDL_SUPPORT
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
	DBGLOG(INIT, ERROR, "[SER][L0.5] Reset fail !!!!\n");

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);
	glResetUpdateFlag(FALSE);

	GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_SER_L0P5_FAIL);

	return;
}
#endif /* CFG_WMT_RESET_API_SUPPORT */

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
static void mtk_wifi_reset_main(struct RESET_STRUCT *rst,
	u_int8_t fgL0Reset)
{
	u_int8_t fgResult = FALSE;
	int32_t ret = 0;

	if (rst == NULL) {
		DBGLOG(INIT, ERROR, "input value rst is NULL.\n");
		return;
	}

#if CFG_WMT_RESET_API_SUPPORT
#if CFG_MTK_ANDROID_WMT
	ret = wifi_reset_end(rst->rst_data);
#else
	if (fgL0Reset)
		ret = wlanFuncOnImpl();
	else
		ret = wlanFuncOn();
#endif
#if !CFG_SUPPORT_CONNAC1X
	g_IsWfsysBusHang = FALSE;
	if (g_IsWholeChipRst == TRUE) {
		g_IsWholeChipRst = FALSE;
		glResetUpdateL0Flag(FALSE);
		complete(&g_RstOnComp);
	}
#endif
#else
#if (CFG_CHIP_RESET_KO_SUPPORT == 0)
	fgResult = rst_L0_notify_step1();

	wait_core_dump_end();

	if (rst->prGlueInfo != NULL &&
		rst->prGlueInfo->prAdapter != NULL &&
		rst->prGlueInfo->prAdapter->chip_info != NULL &&
		rst->prGlueInfo->prAdapter->chip_info
			->rst_L0_notify_step2 != NULL)
		fgResult = rst->prGlueInfo->prAdapter
			->chip_info->rst_L0_notify_step2();

#if CFG_CHIP_RESET_HANG
	if (fgIsResetHangState == SER_L0_HANG_RST_NONE)
		fgIsResetHangState = SER_L0_HANG_RST_TRGING;
#endif

	if (is_bt_exist() == FALSE)
		kalRemoveProbe(rst->prGlueInfo);
#endif
#endif

	if (fgSimplifyResetFlow) {
		DBGLOG(INIT, INFO, "Force down the reset flag.\n");
		fgSimplifyResetFlow = FALSE;
	}

	if (mtk_cfg80211_vendor_event_reset_triggered(
					(uint32_t) eResetReason) != 0)
		DBGLOG(INIT, ERROR, "Send WIFI_EVENT_RESET_TRIGGERED Error!\n");

	DBGLOG(INIT, STATE, "[SER][L0] flow end, fgResult=%d, ret: %d\n",
		fgResult, ret);
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
static void mtk_wifi_reset(struct work_struct *work)
{
	struct RESET_STRUCT *rst =
		CONTAINER_OF(work, struct RESET_STRUCT, rst_work);
	struct ADAPTER *prAdapter;
	struct CHIP_DBG_OPS *prChipDbg;
	u_int8_t fgDrvOwn;

	prAdapter = rst->prGlueInfo->prAdapter;
	prChipDbg = prAdapter->chip_info->prDebugOps;
	fgDrvOwn = TRUE;

	/* Although we've already executed show_mcu_debug_info() in
	 * glResetTriggerCommon(), it may not be a successful operation at that
	 * time in USB mode due to the context is soft_irq which forbids the
	 * execution of usb_control_msg(). Since we don't prefer to refactor it
	 * at this stage, this is a workaround by performing it again here
	 * where the context is kernel thread from workqueue.
	 */
	if (prChipDbg->show_mcu_debug_info) {
		HAL_LP_OWN_RD(prAdapter, &fgDrvOwn);
		if (fgDrvOwn)
			prChipDbg->show_mcu_debug_info(prAdapter, NULL, 0,
						       DBG_MCU_DBG_ALL, NULL);
		else
			DBGLOG(INIT, INFO,
			       "[SER][L0] not drv own, cannot get mcu info\n");
	}
	mtk_wifi_reset_main(rst, FALSE);

}

#if CFG_CHIP_RESET_KO_SUPPORT
void resetkoNotifyFunc(unsigned int event, void *data)
{
	DBGLOG(INIT, INFO, "%s: %d\n", __func__, event);
	if (wifi_rst.fgIsInitialized == FALSE) {
		DBGLOG(INIT, WARN, "%s: reset deinited\n", __func__);
		return;
	}
	if (event == MODULE_NOTIFY_PRE_POWER_OFF) {
		if (wlanIsProbing() || wlanIsRemoving()) {
			fgIsPendingForReady = TRUE;
		} else {
			fgSimplifyResetFlow = FALSE;
			glResetUpdateFlag(TRUE);
			send_reset_event(RESET_MODULE_TYPE_WIFI,
					 RFSM_EVENT_READY);
		}
	} else if (event == MODULE_NOTIFY_RESET_DONE) {
		u4RstCount++;
		DBGLOG(INIT, INFO, "%s: reset count %d\n",
			__func__, u4RstCount);
	} else if (event == MODULE_NOTIFY_POWER_OFF_DONE) {
		u4PowerOffCount++;
		DBGLOG(INIT, INFO, "%s: power off count %d\n",
			__func__, u4PowerOffCount);
	}
}

void resetkoReset(void)
{
	DBGLOG(INIT, WARN, "%s\n", __func__);
	kalRemoveProbe(wifi_rst.prGlueInfo);
}
#endif
#endif

#if CFG_WMT_RESET_API_SUPPORT
#if CFG_SUPPORT_CONNAC1X

static void mtk_wifi_trigger_reset(struct work_struct *work)
{
	u_int8_t fgResult = FALSE;
	struct RESET_STRUCT *rst =
		CONTAINER_OF(work, struct RESET_STRUCT, rst_trigger_work);

	glResetUpdateFlag(TRUE);
	/* Set the power off flag to FALSE in WMT to prevent chip power off
	 * after wlanProbe return failure, because we need to do core dump
	 * afterward.
	 */
	if (rst->rst_trigger_flag & RST_FLAG_PREVENT_POWER_OFF)
		mtk_wcn_set_connsys_power_off_flag(FALSE);

	fgResult = mtk_wcn_wmt_assert_keyword(
		WMTDRV_TYPE_WIFI, apucRstReason[eResetReason]);
	DBGLOG(INIT, INFO, "reset result %d, trigger flag 0x%x\n",
				fgResult, rst->rst_trigger_flag);
}
#endif
/* Weak reference for those platform doesn't support wmt functions */

int32_t __weak mtk_wcn_stp_coredump_start_get(void)
{
	return FALSE;
}

/*0= f/w assert flag is not set, others=f/w assert flag is set */
int32_t glIsWmtCodeDump(void)
{
	return mtk_wcn_stp_coredump_start_get();
}

static void triggerHifDumpIfNeed(void)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter;

	if (fgIsResetting)
		return;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag || !prGlueInfo->prAdapter)
		return;

	prAdapter = prGlueInfo->prAdapter;
	prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;

	kalSetHifDbgEvent(prAdapter->prGlueInfo);
	/* wait for hif_thread finish dump */
	kalMsleep(100);
}

#if CFG_SUPPORT_CONNAC1X
static void dumpWlanThreadsIfNeed(void)
{
	struct GLUE_INFO *prGlueInfo;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag || !prGlueInfo->prAdapter)
		return;

	if (fgIsResetting)
		return;

	DBGLOG(INIT, INFO, "prGlueInfo->ulFlag: 0x%lx\n", prGlueInfo->ulFlag);

	if (prGlueInfo->main_thread) {
		DBGLOG(INIT, INFO, "Show backtrace of main_thread.\n");
		kal_show_stack(prGlueInfo->prAdapter, prGlueInfo->main_thread,
				NULL);
	}
	if (prGlueInfo->rx_thread) {
		DBGLOG(INIT, INFO, "Show backtrace of rx_thread.\n");
		kal_show_stack(prGlueInfo->prAdapter, prGlueInfo->rx_thread,
				NULL);
	}
	if (prGlueInfo->hif_thread) {
		DBGLOG(INIT, INFO, "Show backtrace of hif_thread.\n");
		kal_show_stack(prGlueInfo->prAdapter, prGlueInfo->hif_thread,
				NULL);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This routine is invoked when there is reset messages indicated
 *
 * @param   eSrcType
 *          eDstType
 *          eMsgType
 *          prMsgBody
 *          u4MsgLength
 *
 * @retval
 */
/*----------------------------------------------------------------------------*/
static void glResetCallback(enum _ENUM_WMTDRV_TYPE_T eSrcType,
			     enum _ENUM_WMTDRV_TYPE_T eDstType,
			     enum _ENUM_WMTMSG_TYPE_T eMsgType, void *prMsgBody,
			     unsigned int u4MsgLength)
{
	switch (eMsgType) {
	case WMTMSG_TYPE_RESET:
		if (u4MsgLength == sizeof(enum _ENUM_WMTRSTMSG_TYPE_T)) {
			enum _ENUM_WMTRSTMSG_TYPE_T *prRstMsg =
				(enum _ENUM_WMTRSTMSG_TYPE_T *) prMsgBody;

			switch (*prRstMsg) {
			case WMTRSTMSG_RESET_START:
				DBGLOG(INIT, WARN, "Whole chip reset start!\n");
				dumpWlanThreadsIfNeed();
				triggerHifDumpIfNeed();
				glResetUpdateFlag(TRUE);
				fgSimplifyResetFlow = TRUE;
#if CFG_MTK_ANDROID_WMT
				wifi_reset_start();
#endif
				break;

			case WMTRSTMSG_RESET_END:
				DBGLOG(INIT, WARN, "Whole chip reset end!\n");
				wifi_rst.rst_data = RESET_SUCCESS;
				glResetUpdateFlag(FALSE);
				mtk_wifi_reset_main(&wifi_rst, TRUE);
#if WLAN_INCLUDE_SYS
				sysHangRecoveryReport();
#endif
				break;

			case WMTRSTMSG_RESET_END_FAIL:
				DBGLOG(INIT, WARN, "Whole chip reset fail!\n");
				glResetUpdateFlag(FALSE);
				wifi_rst.rst_data = RESET_FAIL;
				mtk_wifi_reset_main(&wifi_rst, TRUE);
				break;

			default:
				break;
			}
		}
		break;

	default:
		break;
	}
}
#endif

void glSetRstReasonString(char *reason)
{
	g_reason = reason;
}

static u_int8_t glResetMsgHandler(enum ENUM_RST_MSG MsgBody)
{
	switch (MsgBody) {
	case ENUM_RST_MSG_L0_START:
		DBGLOG(INIT, INFO, "Whole chip reset start!\n");
#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
		fw_log_handler();
#endif
		glResetUpdateFlag(TRUE);
		glResetOnEndUpdateFlag(TRUE);
#if (CFG_SUPPORT_CONNINFRA == 1)
		fgSimplifyResetFlow = TRUE;
#endif
#if CFG_MTK_ANDROID_WMT
		wifi_reset_start();
#endif
		wfsys_lock();
		wlanFuncOffImpl();
		wfsys_unlock();
		complete(&g_RstOffComp);
		break;
	case ENUM_RST_MSG_L0_END:
		DBGLOG(INIT, INFO, "Whole chip reset end!\n");
		glResetUpdateFlag(FALSE);
		wifi_rst.rst_data = RESET_SUCCESS;
		mtk_wifi_reset_main(&wifi_rst, TRUE);
		glResetOnEndUpdateFlag(FALSE);
#if WLAN_INCLUDE_SYS
		sysHangRecoveryReport();
#endif
		break;

	case ENUM_RST_MSG_L04_START:
	case ENUM_RST_MSG_L05_START:
		DBGLOG(INIT, WARN, "WF chip reset start!\n");
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
		fw_log_handler();
#endif
		glResetUpdateFlag(TRUE);
		glResetOnEndUpdateFlag(TRUE);
#if (CFG_SUPPORT_CONNINFRA == 1)
		fgSimplifyResetFlow = TRUE;
#endif
#if CFG_MTK_ANDROID_WMT
		wifi_reset_start();
#endif
		wfsys_lock();
		wlanFuncOff();
		wfsys_unlock();
		break;

	case ENUM_RST_MSG_L04_END:
	case ENUM_RST_MSG_L05_END:
		DBGLOG(INIT, WARN, "WF chip reset end!\n");
		glResetUpdateFlag(FALSE);
		wifi_rst.rst_data = RESET_SUCCESS;
		mtk_wifi_reset_main(&wifi_rst, FALSE);
		glResetOnEndUpdateFlag(FALSE);
#if WLAN_INCLUDE_SYS
		sysHangRecoveryReport();
#endif
		break;

	default:
		break;
	}

	return TRUE;
}

bool glRstCheckRstCriteria(void)
{
	/*
	 * for those cases which need to trigger whole chip reset
	 * when fgIsResetting = TRUE
	 */
	if (g_IsSubsysRstOverThreshold || g_IsWfsysBusHang)
		return FALSE;
	else
		return TRUE;
}

void glRstWholeChipRstParamInit(void)
{
	g_IsSubsysRstOverThreshold = FALSE;
	g_SubsysRstCnt = 0;
	g_WholeChipRstTotalCnt++;
}

#if (CFG_SUPPORT_CONNINFRA == 1)
int glRstwlanPreWholeChipReset(enum consys_drv_type type, char *reason)
{
	bool bRet = 0;
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter = NULL;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;

	DBGLOG(INIT, INFO,
			"Enter glRstwlanPreWholeChipReset.\n");
	glResetUpdateL0Flag(TRUE);
	while (get_wifi_process_status()) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	wfsys_lock();
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		wfsys_unlock();
		goto exit;
	}
	wfsys_unlock();

	triggerHifDumpIfNeed();

	g_Coredump_source = coredump_conn_type_to_src(type);
	g_WholeChipRstReason = reason;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetOnEnd()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			msleep(100);
		}
		if (!get_wifi_powered_status()) {
			DBGLOG(REQ, INFO, "wifi driver is off, skip reset\n");
			goto exit;
		}
		g_IsWholeChipRst = TRUE;
		DBGLOG(INIT, INFO,
				"Wi-Fi Driver processes whole chip reset start.\n");
		GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
					 RST_WHOLE_CHIP_TRIGGER);
	} else {
		if (g_IsSubsysRstOverThreshold)
			DBGLOG(INIT, INFO, "Reach subsys reset threshold!!!\n");
		else if (g_IsWfsysBusHang)
			DBGLOG(INIT, INFO, "WFSYS bus hang!!!\n");

		while (kalIsResetOnEnd() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			msleep(100);
		}
		g_IsWholeChipRst = TRUE;

		if (!prGlueInfo->u4ReadyFlag)
			glSetIsNeedWaitCoredumpFlag(TRUE);

		kalSetRstEvent(FALSE);
	}
	wait_for_completion(&g_RstOffComp);
	DBGLOG(INIT, INFO, "Wi-Fi is off successfully.\n");

exit:
	fgIsDrvTriggerWholeChipReset = FALSE;

	return bRet;
}

int glRstwlanPostWholeChipReset(void)
{
	while (get_wifi_process_status()) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		glResetUpdateL0Flag(FALSE);
		return 0;
	}
	glRstSetRstEndEvent();
	DBGLOG(INIT, INFO, "Wait Wi-Fi state recover.\n");
	wait_for_completion(&g_RstOnComp);

	DBGLOG(INIT, INFO,
		"Leave glRstwlanPostWholeChipReset (%d).\n",
		g_IsWholeChipRst);
	return 0;
}
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
int wlan_pre_whole_chip_rst_v3(enum connv3_drv_type drv,
	char *reason, unsigned int reset_type)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct mt66xx_chip_info *chip = NULL;
	struct CHIP_DBG_OPS *dbg_ops = NULL;

	DBGLOG(INIT, INFO,
		"drv:%d, reason:%s, reset_type:%d\n",
		drv, reason, reset_type);

#if CFG_MTK_ANDROID_WMT
	glResetUpdateL0Flag(TRUE);
	while (get_wifi_process_status()) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		kalMsleep(100);
	}

	while (kalIsResetOnEnd()) {
		DBGLOG(REQ, WARN, "wifi driver is resetting\n");
		kalMsleep(100);
	}

	g_IsWholeChipRst = TRUE;

	wfsys_lock();
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		glResetOnEndUpdateFlag(TRUE);
		wfsys_unlock();
		goto exit;
	}
	wfsys_unlock();
#endif
	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(REQ, WARN, "adapter null, return\n");
		goto exit;
	}

	prBusInfo = prAdapter->chip_info->bus_info;

	if (drv == CONNV3_DRV_TYPE_CONNV3) {
		if (prGlueInfo->u4ReadyFlag &&
		    kalStrnCmp(reason, "PMIC Fault", 10) == 0) {
			fgIsBusAccessFailed = TRUE;
			g_IsWfsysBusHang = TRUE;
			DBGLOG(REQ, INFO,
				"Get PMIC Fault\n");
#if defined(_HIF_PCIE)
			if (prBusInfo->disableDevice)
				prBusInfo->disableDevice(prGlueInfo);
#endif
		}
	}

	if (!fgIsBusAccessFailed && drv != CONNV3_DRV_TYPE_WIFI)
		triggerHifDumpIfNeed();

	g_Coredump_source = coredump_connv3_type_to_src(drv);
	g_WholeChipRstReason = reason;
	g_Coredump_type = reset_type;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetOnEnd()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			kalMsleep(100);
		}

		/* If wifi is off, skip off flow after previous reset end */
		if (!get_wifi_powered_status()) {
			DBGLOG(INIT, INFO,
				"wifi driver is off now, skip reset off.\n");
			goto exit;
		}

		glSetRstReason(RST_WHOLE_CHIP_TRIGGER);
		if (glResetTriggerImpl(prGlueInfo->prAdapter,
			glResetSelectAction(prGlueInfo->prAdapter),
			(const uint8_t *)__FILE__, __LINE__) !=
			WLAN_STATUS_SUCCESS)
			goto exit;
	} else {
		while (kalIsResetOnEnd() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			kalMsleep(100);
		}
		fgIsDrvTriggerWholeChipReset = FALSE;

		dbg_ops = prAdapter->chip_info->prDebugOps;
		if (dbg_ops && dbg_ops->dumpBusHangCr)
			dbg_ops->dumpBusHangCr(prAdapter);

		if (drv != CONNV3_DRV_TYPE_WIFI)
			glSetRstReason(RST_WHOLE_CHIP_TRIGGER);

		kalSetRstEvent(TRUE);
	}

	DBGLOG(INIT, INFO, "g_RstOffComp.done= %d\n",
		g_RstOffComp.done);
	if (g_RstOffComp.done != 0)
		kalSendAeeWarning("WLAN", "reset off failed\n");

	wait_for_completion(&g_RstOffComp);
exit:
	DBGLOG(INIT, INFO, "Wi-Fi is off successfully\n");
	fgIsDrvTriggerWholeChipReset = FALSE;

	if (reset_type == ENUM_COREDUMP_BY_CHIP_RST_DFD_DUMP) {
		glGetChipInfo((void **)&chip);
		if (!chip)
			DBGLOG(HAL, ERROR, "NULL chip info pwr on.\n");
		else
			wlan_pinctrl_action(chip, WLAN_PINCTRL_MSG_FUNC_ON);
	}
	return 0;
}

int wlan_post_whole_chip_rst_v3(void)
{
	DBGLOG(INIT, INFO, "wlan_post_whole_chip_rst_v3\n");

	fgIsBusAccessFailed = FALSE;
#if CFG_MTK_WIFI_PCIE_SUPPORT
	fgIsPcieDataTransDisabled = FALSE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
	glRstSetRstEndEvent();

	return 0;
}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
int wlan_post_reset_on_v3(unsigned int type)
{
	int ret = 0;

	DBGLOG(INIT, INFO, "type: %d\n", type);

	/* 0: CONNV3_CHIP_RST_POST_ACTION_NOTHING
	 * 1: CONNV3_CHIP_RST_POST_ACTION_PMIC_SHUTDOWN
	 */
	if (type > 1)
		goto exit;

	ret = wlanFuncPreOnImpl();

exit:
	return 0;
}
#endif

int wlan_pre_whole_chip_rst_v2(enum consys_drv_type drv,
	char *reason)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter = NULL;

	DBGLOG(INIT, INFO,
		"drv: %d, reason: %s\n",
		drv, reason);

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;

#if CFG_MTK_ANDROID_WMT
	glResetUpdateL0Flag(TRUE);
	while (get_wifi_process_status()) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		kalMsleep(100);
	}

	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		goto exit;
	}
#endif
	triggerHifDumpIfNeed();

	g_Coredump_source = coredump_conn_type_to_src(drv);
	g_WholeChipRstReason = reason;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetOnEnd()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			kalMsleep(100);
		}

		g_IsWholeChipRst = TRUE;

		GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
					 RST_WHOLE_CHIP_TRIGGER);
	} else {
		while (kalIsResetOnEnd() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			kalMsleep(100);
		}
		g_IsWholeChipRst = TRUE;

		kalSetRstEvent(TRUE);
	}

	wait_for_completion(&g_RstOffComp);
exit:
	fgIsDrvTriggerWholeChipReset = FALSE;

	DBGLOG(INIT, INFO, "Wi-Fi is off successfully.\n");

	return 0;
}

int wlan_post_whole_chip_rst_v2(void)
{
#if CFG_MTK_ANDROID_WMT
	while (get_wifi_process_status()) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		glResetUpdateL0Flag(FALSE);
		return 0;
	}
#endif
	glRstSetRstEndEvent();

	DBGLOG(INIT, INFO,
		"Leave glRstwlanPostWholeChipReset (%d).\n",
		g_IsWholeChipRst);
	return 0;
}
#endif

u_int8_t kalIsWholeChipResetting(void)
{
#if CFG_CHIP_RESET_SUPPORT
	return g_IsWholeChipRst;
#else
	return FALSE;
#endif
}

void glReset_timeinit(struct timespec64 *rNowTs, struct timespec64 *rLastTs)
{
	rNowTs->tv_sec = 0;
	rNowTs->tv_nsec = 0;
	rLastTs->tv_sec = 0;
	rLastTs->tv_nsec = 0;
}

bool IsOverRstTimeThreshold(
	struct timespec64 *rNowTs, struct timespec64 *rLastTs)
{
#if (CFG_SUPPORT_CONNINFRA == 1)
	struct timespec64 rTimeout, rTime = {0};
	bool fgIsTimeout = TRUE;

	rTimeout.tv_sec = 30;
	rTimeout.tv_nsec = 0;
	ktime_get_ts64(rNowTs);
	DBGLOG(INIT, INFO,
		"Reset happen time :%ld.%09ld, last happen time :%ld.%09ld\n",
		rNowTs->tv_sec, rNowTs->tv_nsec,
		rLastTs->tv_sec, rLastTs->tv_nsec);
	if (rLastTs->tv_sec != 0) {
		if (kalGetDeltaTime(rNowTs, rLastTs, &rTime)) {
			if (kalTimeCompare(&rTime, &rTimeout) >= 0)
				fgIsTimeout = TRUE;
			else
				fgIsTimeout = FALSE;
		}
		DBGLOG(INIT, INFO,
			"Reset rTimeout :%ld.%ld, calculate time :%ld.%ld\n",
			rTimeout.tv_sec,
			rTimeout.tv_nsec,
			rTime.tv_sec,
			rTime.tv_nsec);
	}
	return fgIsTimeout;
#else
	return TRUE;
#endif
}

void glResetWholeChipResetTrigger(char *pcReason)
{
	int ret = -ENOTSUPP;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct RESET_STRUCT *rst = &wifi_rst;
	struct GLUE_INFO *prGlueInfo = rst->prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter != NULL && prAdapter->chip_info != NULL)
		prDebugOps = prAdapter->chip_info->prDebugOps;

	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt(prAdapter);

	if (prGlueInfo->u4ReadyFlag && dumpViaBt) {
		if (prDebugOps && prDebugOps->dumpBusHangCr)
			prDebugOps->dumpBusHangCr(prAdapter);
	}
#endif

	glResetUpdateL0Flag(TRUE);

#if (CFG_SUPPORT_CONNINFRA == 1)
	ret = conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_WIFI, pcReason);
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_WIFI, pcReason);
#else
	DBGLOG(INIT, WARN, "whole chip reset NOT support\n");
#endif

	DBGLOG(INIT, INFO, "ret:%d, reason:%s\n", ret, pcReason);
#if (CFG_SUPPORT_CONNINFRA == 1) || IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (ret == 0) {
		dump_stack();
		fgIsDrvTriggerWholeChipReset = TRUE;
	} else {
		glResetUpdateL0Flag(FALSE);
	}
#endif
}

void glResetSubsysRstProcedure(struct RESET_STRUCT *rst,
	struct timespec64 *rNowTs,
	struct timespec64 *rLastTs,
	enum COREDUMP_SOURCE_TYPE coredump_source,
	enum _ENUM_CHIP_RESET_REASON_TYPE_T resetReason)
{
	struct GLUE_INFO *prGlueInfo = rst->prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	bool fgIsTimeout;

	if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
		struct WIFI_VAR *prWifiVar = NULL;

		prAdapter = prGlueInfo->prAdapter;
		prWifiVar = &prAdapter->rWifiVar;
	}

	fgIsTimeout = IsOverRstTimeThreshold(rNowTs, rLastTs);
	if (g_IsWfsysBusHang == TRUE) {
		if (prAdapter) {
#if (CFG_SUPPORT_CONNINFRA == 1)
			struct CHIP_DBG_OPS *debug_ops =
				prAdapter->chip_info->prDebugOps;
#endif
			if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
				fgIsDrvTriggerWholeChipReset = TRUE;
				glSetRstReasonString(
					"fw detect bus hang");
				glResetWholeChipResetTrigger(g_reason);
				glResetCleanResetFlag();
				return;
			}
#if (CFG_SUPPORT_CONNINFRA == 1)
			if (conninfra_reg_readable_for_coredump() == 1 &&
			    debug_ops &&
			    debug_ops->dumpBusHangCr)
				debug_ops->dumpBusHangCr(prAdapter);
#endif
		}
		DBGLOG(INIT, INFO,
			"Don't trigger whole chip reset due to driver is not ready\n");
		glResetCleanResetFlag();
		return;
	}
	if (g_SubsysRstCnt > 3) {
		if (fgIsTimeout == TRUE) {
		/*
		 * g_SubsysRstCnt > 3, > 30 sec,
		 * need to update rLastTs, still do wfsys reset
		 */
			if (resetReason >= RST_REASON_MAX)
				resetReason = 0;

			if (g_fgRstRecover == TRUE)
				g_fgRstRecover = FALSE;
			else
				wifi_coredump_start(
					coredump_source,
					apucRstReason[resetReason],
					ENUM_COREDUMP_BY_CHIP_RST_LEGACY_MODE,
					rst->force_dump);

			glSetIsNeedWaitCoredumpFlag(FALSE);

#if (CFG_SUPPORT_CONNINFRA == 1)
			if (g_IsWfsysBusHang == TRUE)
				DBGLOG(INIT, INFO,
					"Detect bus hang, do whole chip reset.\n");
			else if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
#else
			if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
#endif
				glResetMsgHandler(ENUM_RST_MSG_L04_START);
				glResetMsgHandler(ENUM_RST_MSG_L04_END);
			} else {
				glResetCleanResetFlag();
				DBGLOG(INIT, INFO,
					"Don't trigger subsys reset due to driver is not ready\n");
			}
			g_SubsysRstTotalCnt++;
			g_SubsysRstCnt = 1;
		} else {
			/*g_SubsysRstCnt > 3, < 30 sec, do whole chip reset */
			g_IsSubsysRstOverThreshold = TRUE;
			/*coredump is done, no need do again*/
			fgIsDrvTriggerWholeChipReset = TRUE;
			glSetRstReasonString(
				"subsys reset more than 3 times");
			glResetWholeChipResetTrigger(g_reason);
			glResetCleanResetFlag();
		}
	} else {
		if (resetReason >= RST_REASON_MAX)
			resetReason = 0;

		if (g_fgRstRecover == TRUE)
			g_fgRstRecover = FALSE;
		else
			wifi_coredump_start(coredump_source,
				apucRstReason[resetReason],
				ENUM_COREDUMP_BY_CHIP_RST_LEGACY_MODE,
				rst->force_dump);

		glSetIsNeedWaitCoredumpFlag(FALSE);

#if (CFG_SUPPORT_CONNINFRA == 1)
		if (g_IsWfsysBusHang == TRUE)
			DBGLOG(INIT, INFO,
				"Detect bus hang, do whole chip reset.\n");
		else if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
#else
		if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
#endif
			glResetMsgHandler(ENUM_RST_MSG_L04_START);
			glResetMsgHandler(ENUM_RST_MSG_L04_END);
		} else {
			glResetCleanResetFlag();
			g_IsWfsysBusHang = FALSE;
			DBGLOG(INIT, INFO,
				"Don't trigger subsys reset due to driver is not ready\n");
		}
		g_SubsysRstTotalCnt++;
		/*g_SubsysRstCnt < 3, but >30 sec,need to update rLastTs*/
		if (fgIsTimeout == TRUE)
			g_SubsysRstCnt = 1;
	}

	if (g_SubsysRstCnt == 1) {
		rLastTs->tv_nsec = rNowTs->tv_nsec;
		rLastTs->tv_sec = rNowTs->tv_sec;
	}

	g_Coredump_source = COREDUMP_SOURCE_NUM;
	rst->force_dump = FALSE;
}

int wlan_reset_thread_main(void *data)
{
	struct RESET_STRUCT *rst = data;
	struct GLUE_INFO *prGlueInfo = rst->prGlueInfo;
	struct timespec64 rNowTs, rLastTs;
	int ret = 0;

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T *prWlanRstThreadWakeLock;

	KAL_WAKE_LOCK_INIT(NULL,
			   prWlanRstThreadWakeLock, "WLAN rst_thread");
	KAL_WAKE_LOCK(NULL, prWlanRstThreadWakeLock);
#endif

	glReset_timeinit(&rNowTs, &rLastTs);

	DBGLOG(INIT, INFO, "%s:%u starts running...\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

	while (TRUE) {
		/* Unlock wakelock if hif_thread going to idle */
		KAL_WAKE_UNLOCK(NULL, prWlanRstThreadWakeLock);
		/*
		 * sleep on waitqueue if no events occurred. Event contain
		 * (1) GLUE_FLAG_HALT (2) GLUE_FLAG_RST
		 *
		 */
		do {
			ret = wait_event_interruptible(g_waitq_rst,
				((rst->ulFlag & GLUE_FLAG_RST_PROCESS)
				!= 0));
		} while (ret != 0);
#if CFG_ENABLE_WAKE_LOCK
		if (!KAL_WAKE_LOCK_ACTIVE(NULL,
					  prWlanRstThreadWakeLock))
			KAL_WAKE_LOCK(NULL,
				      prWlanRstThreadWakeLock);
#endif

		if (test_and_clear_bit(GLUE_FLAG_RST_START_BIT, &rst->ulFlag)) {
#if CFG_ENABLE_WAKE_LOCK
			if (KAL_WAKE_LOCK_ACTIVE(NULL, g_IntrWakeLock))
				KAL_WAKE_UNLOCK(NULL, g_IntrWakeLock);
#endif

			if (g_IsWholeChipRst) {
				if (eResetReason >= RST_REASON_MAX)
					eResetReason = 0;
				wifi_coredump_start(
					g_Coredump_source,
					g_WholeChipRstReason,
					g_Coredump_type,
					rst->force_dump);
				rst->force_dump = FALSE;
				glSetIsNeedWaitCoredumpFlag(FALSE);

				if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
					glResetMsgHandler(
						ENUM_RST_MSG_L0_START);
					glRstWholeChipRstParamInit();
					glReset_timeinit(&rNowTs, &rLastTs);
				} else {
					if (!completion_done(&g_RstOffComp))
						complete(&g_RstOffComp);
					DBGLOG(INIT, INFO,
						"Don't trigger whole chip reset due to driver is not ready\n");
					glResetCleanResetFlag();
				}
				g_Coredump_type =
					ENUM_COREDUMP_BY_CHIP_RST_LEGACY_MODE;
			} else {
				/*wfsys reset start*/
				g_IsWfsysRstDone = FALSE;
				g_SubsysRstCnt++;
				DBGLOG(INIT, INFO,
					"WF reset count = %d.\n",
					g_SubsysRstCnt);
				glResetSubsysRstProcedure(rst,
							 &rNowTs,
							 &rLastTs,
							 g_Coredump_source,
							 eResetReason);
				/*wfsys reset done*/
				g_IsWfsysRstDone = TRUE;
			}
			DBGLOG(INIT, INFO,
			"Whole Chip rst count /WF reset total count = (%d)/(%d).\n",
				g_WholeChipRstTotalCnt,
				g_SubsysRstTotalCnt);
		}

		if (test_and_clear_bit(GLUE_FLAG_RST_FW_NOTIFY_L05_BIT,
			&rst->ulFlag)) {
#if CFG_ENABLE_WAKE_LOCK
			if (KAL_WAKE_LOCK_ACTIVE(NULL, g_IntrWakeLock))
				KAL_WAKE_UNLOCK(NULL, g_IntrWakeLock);
#endif
			/*wfsys reset start*/
			g_IsWfsysRstDone = FALSE;
			g_SubsysRstCnt++;
			DBGLOG(INIT, INFO,
				"WF reset count = %d.\n",
				g_SubsysRstCnt);
			glResetSubsysRstProcedure(rst,
				&rNowTs, &rLastTs,
				COREDUMP_SOURCE_WF_FW,
				RST_FW_ASSERT);
			/*wfsys reset done*/
			g_IsWfsysRstDone = TRUE;

			DBGLOG(INIT, INFO,
			"Whole Chip rst count /WF reset total count = (%d)/(%d).\n",
				g_WholeChipRstTotalCnt,
				g_SubsysRstTotalCnt);
		}

		if (test_and_clear_bit(GLUE_FLAG_RST_FW_NOTIFY_L0_BIT,
			&rst->ulFlag)) {
#if CFG_ENABLE_WAKE_LOCK
			if (KAL_WAKE_LOCK_ACTIVE(NULL, g_IntrWakeLock))
				KAL_WAKE_UNLOCK(NULL, g_IntrWakeLock);
#endif
			/*wfsys reset start*/
			g_IsWfsysRstDone = FALSE;
			g_SubsysRstCnt++;
			DBGLOG(INIT, INFO,
				"WF reset count = %d.\n",
				g_SubsysRstCnt);
			glResetSubsysRstProcedure(rst,
				&rNowTs, &rLastTs,
				COREDUMP_SOURCE_WF_FW,
				0);
			/*wfsys reset done*/
			g_IsWfsysRstDone = TRUE;

			DBGLOG(INIT, INFO,
			"Whole Chip rst count /WF reset total count = (%d)/(%d).\n",
				g_WholeChipRstTotalCnt,
				g_SubsysRstTotalCnt);
		}

		if (test_and_clear_bit(GLUE_FLAG_RST_END_BIT, &rst->ulFlag)) {
#if (CFG_ENABLE_WAKE_LOCK)
			if (KAL_WAKE_LOCK_ACTIVE(NULL, g_IntrWakeLock))
				KAL_WAKE_UNLOCK(NULL, g_IntrWakeLock);
#endif
			DBGLOG(INIT, INFO, "Whole chip reset end start\n");
			glResetMsgHandler(ENUM_RST_MSG_L0_END);
		}

		if (test_and_clear_bit(GLUE_FLAG_HALT_BIT, &rst->ulFlag)) {
			DBGLOG(INIT, INFO, "rst_thread should stop now...\n");
			break;
		}
	}

#if CFG_ENABLE_WAKE_LOCK
#if (KERNEL_VERSION(4, 9, 0) > CFG80211_VERSION_CODE)
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prWlanRstThreadWakeLock))
		KAL_WAKE_UNLOCK(NULL, prWlanRstThreadWakeLock);
#endif
	KAL_WAKE_LOCK_DESTROY(NULL,
			      prWlanRstThreadWakeLock);
#endif

	complete(&rst->halt_comp);
	DBGLOG(INIT, TRACE, "%s:%u stopped!\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());

	return 0;
}

void kalSetRstEvent(u_int8_t force_dump)
{
	struct RESET_STRUCT *rst = &wifi_rst;

	KAL_WAKE_LOCK(NULL, g_IntrWakeLock);

	rst->force_dump = force_dump;
	set_bit(GLUE_FLAG_RST_START_BIT, &rst->ulFlag);

	/* when we got interrupt, we wake up servie thread */
	wake_up_interruptible(&g_waitq_rst);
}

void kalSetRstFwNotifyL05Event(u_int8_t force_dump)
{
	struct RESET_STRUCT *rst = &wifi_rst;

	KAL_WAKE_LOCK(NULL, g_IntrWakeLock);

	rst->force_dump = force_dump;
	set_bit(GLUE_FLAG_RST_FW_NOTIFY_L05_BIT, &rst->ulFlag);

	/* when we got interrupt, we wake up servie thread */
	wake_up_interruptible(&g_waitq_rst);
}

void kalSetRstFwNotifyTriggerL0Event(u_int8_t force_dump)
{
	struct RESET_STRUCT *rst = &wifi_rst;

	KAL_WAKE_LOCK(NULL, g_IntrWakeLock);

	rst->force_dump = force_dump;
	set_bit(GLUE_FLAG_RST_FW_NOTIFY_L0_BIT, &rst->ulFlag);

	/* when we got interrupt, we wake up servie thread */
	wake_up_interruptible(&g_waitq_rst);
}

void glRstSetRstEndEvent(void)
{
	struct RESET_STRUCT *rst = &wifi_rst;

	KAL_WAKE_LOCK(NULL, g_IntrWakeLock);

	set_bit(GLUE_FLAG_RST_END_BIT, &rst->ulFlag);

	/* when we got interrupt, we wake up servie thread */
	wake_up_interruptible(&g_waitq_rst);
}

int reset_wait_for_trigger_completion(void)
{
	uint32_t waitRet = 0;

	waitRet = wait_for_completion_timeout(&g_triggerComp,
		MSEC_TO_JIFFIES(WIFI_TRIGGER_ASSERT_TIMEOUT));
	if (waitRet > 0) {
		/* Case 1: No timeout. */
		DBGLOG(INIT, INFO, "Trigger assert successfully.\n");
	} else {
		/* Case 2: timeout */
		DBGLOG(INIT, ERROR,
			"Trigger assert more than %d ms, need to trigger rst self\n",
			WIFI_TRIGGER_ASSERT_TIMEOUT);
	}

	return waitRet > 0 ? 0 : -ETIMEDOUT;
}

void reset_done_trigger_completion(void)
{
	complete(&g_triggerComp);
}
#else
#if (CFG_CHIP_RESET_KO_SUPPORT == 0)
static u_int8_t is_bt_exist(void)
{
	char *bt_func_name = "WF_rst_L0_notify_BT_step1";
	void *pvAddr = NULL;

	pvAddr = GLUE_SYMBOL_GET(bt_func_name);

	if (!pvAddr)
		DBGLOG(INIT, ERROR, "[SER][L0] %s does not exist\n",
			bt_func_name);
	else {
		GLUE_SYMBOL_PUT(bt_func_name);
		return TRUE;
	}

	return FALSE;

}

static u_int8_t rst_L0_notify_step1(void)
{
	if (eResetReason != RST_BT_TRIGGER) {
		typedef int (*p_bt_fun_type) (int);
		p_bt_fun_type bt_func;
		char *bt_func_name = "WF_rst_L0_notify_BT_step1";
		void *pvAddr = NULL;

		DBGLOG(INIT, STATE, "[SER][L0] %s\n", bt_func_name);

		pvAddr = GLUE_SYMBOL_GET(bt_func_name);
		if (pvAddr) {
			bt_func = (p_bt_fun_type) pvAddr;
			bt_func(0);
			GLUE_SYMBOL_PUT(bt_func_name);
		} else {
			DBGLOG(INIT, ERROR,
				"[SER][L0] %s does not exist\n", bt_func_name);
			return FALSE;
		}
	}

	return TRUE;
}

static void wait_core_dump_end(void)
{
#ifdef CFG_SUPPORT_CONNAC2X
	if (eResetReason == RST_OID_TIMEOUT)
		return;
	DBGLOG(INIT, WARN, "[SER][L0] not support..\n");
#endif
}

int32_t BT_rst_L0_notify_WF_step1(int32_t reserved)
{
	GL_DEFAULT_RESET_TRIGGER(NULL, RST_BT_TRIGGER);

	return 0;
}
#ifndef CFG_COMBO_SLT_GOLDEN
EXPORT_SYMBOL(BT_rst_L0_notify_WF_step1);
#endif

int32_t BT_rst_L0_notify_WF_2(int32_t reserved)
{
	DBGLOG(INIT, WARN, "[SER][L0] not support...\n");

	return 0;
}
#ifndef CFG_COMBO_SLT_GOLDEN
EXPORT_SYMBOL(BT_rst_L0_notify_WF_2);
#endif
#endif

#endif

#if (CFG_SUPPORT_SER_DEBUGFS == 1)
static struct dentry *serDbgFsDir;

static int ser_dbgfs_read_dummy(void *data, uint64_t *val)
{
	*val = (kalIsResetting() == FALSE) ? 0 : 1;
	return 0;

}

static int ser_dbgfs_L0_reset(void *data, uint64_t val)
{
	if (wifi_rst.prGlueInfo == NULL)
		return 0;
	if (val != 1)
		return 0;

	kalRemoveProbe(wifi_rst.prGlueInfo);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_L0_reset,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_L0_reset,
				     "%llu\n");

static int ser_dbgfs_power_ctl(void *data, uint64_t val)
{
	if (wifi_rst.prGlueInfo == NULL)
		return 0;

	if (val == 0)
		send_reset_event(RESET_MODULE_TYPE_WIFI,
				 RFSM_EVENT_TRIGGER_POWER_OFF);
	else
		send_reset_event(RESET_MODULE_TYPE_WIFI,
				 RFSM_EVENT_TRIGGER_POWER_ON);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_power_ctl,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_power_ctl,
				     "%llu\n");

static int ser_dbgfs_fw_assert(void *data, uint64_t val)
{
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};
	uint32_t u4BufLen = 0;

	if (wifi_rst.prGlueInfo == NULL)
		return 0;
	if (val != 1)
		return 0;

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rChipConfigInfo.u2MsgSize = kalStrnLen("assert", CHIP_CONFIG_RESP_SIZE);
	kalStrnCpy(rChipConfigInfo.aucCmd,
		   "assert", CHIP_CONFIG_RESP_SIZE - 1);
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

	kalIoctl(wifi_rst.prGlueInfo, wlanoidSetChipConfig,
		&rChipConfigInfo, sizeof(rChipConfigInfo),
		&u4BufLen);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_fw_assert,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_fw_assert,
				     "%llu\n");

static int ser_dbgfs_conninfra_hang(void *data, uint64_t val)
{
	struct CMD_ACCESS_REG rCmdAccessReg = {0};
	uint32_t u4BufLen = 0;

	if (wifi_rst.prGlueInfo == NULL)
		return 0;
	if (val != 1)
		return 0;

	rCmdAccessReg.u4Address = 0x70028438;
	rCmdAccessReg.u4Data = 0;

	kalIoctl(wifi_rst.prGlueInfo, wlanoidSetMcrWrite,
		 &rCmdAccessReg, sizeof(rCmdAccessReg),
		 &u4BufLen);

	rCmdAccessReg.u4Address = 0x70070000;
	rCmdAccessReg.u4Data = 0;
	kalIoctl(wifi_rst.prGlueInfo, wlanoidQueryMcrRead,
		 &rCmdAccessReg, sizeof(rCmdAccessReg),
		 &u4BufLen);

	return 0;

}
DEFINE_SIMPLE_ATTRIBUTE(fops_conninfra_hang,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_conninfra_hang,
				     "%llu\n");


#if defined(_HIF_USB)
static int ser_dbgfs_bus_hang(void *data, uint64_t val)
{
	struct CMD_ACCESS_REG rCmdAccessReg = {0};
	uint32_t u4BufLen = 0;

	if (wifi_rst.prGlueInfo == NULL)
		return 0;
	if (val != 1)
		return 0;

	rCmdAccessReg.u4Address = 0x74011804;
	rCmdAccessReg.u4Data = 0x00000001;

	kalIoctl(wifi_rst.prGlueInfo, wlanoidSetMcrWrite,
		 &rCmdAccessReg, sizeof(rCmdAccessReg),
		 &u4BufLen);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_bus_hang,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_bus_hang,
				     "%llu\n");

static int ser_dbgfs_bus_disconnect(void *data, uint64_t val)
{
	struct CMD_ACCESS_REG rCmdAccessReg = {0};
	uint32_t u4BufLen = 0;

	if (wifi_rst.prGlueInfo == NULL)
		return 0;
	if (val != 1)
		return 0;

	rCmdAccessReg.u4Address = 0x74013E00;
	rCmdAccessReg.u4Data = 0x00000001;

	kalIoctl(wifi_rst.prGlueInfo, wlanoidSetMcrWrite,
		 &rCmdAccessReg, sizeof(rCmdAccessReg),
		 &u4BufLen);

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_bus_disconnect,
				     ser_dbgfs_read_dummy,
				     ser_dbgfs_bus_disconnect,
				     "%llu\n");

#endif

int32_t resetCreateSerDbgFs(struct GLUE_INFO *prGlueInfo)
{
	serDbgFsDir = debugfs_create_dir("mtk_ser_dbgfs", NULL);
	if (!serDbgFsDir) {
		DBGLOG(INIT, ERROR,
			"serDbgFsDir is null for mtk_ser_dbgfs\n");
		return -1;
	}

	/* /sys/kernel/debug/mtk_ser_dbgfs/L0_reset, mode: wr */
	if (!debugfs_create_file("L0_reset",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_L0_reset))
		DBGLOG(INIT, WARN,
			"create L0_reset dgbfs fail\n");

	/* /sys/kernel/debug/mtk_ser_dbgfs/power_ctl, mode: wr */
	if (!debugfs_create_file("power_ctl",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_power_ctl))
		DBGLOG(INIT, WARN,
			"create power_ctl dgbfs fail\n");

	/* /sys/kernel/debug/mtk_ser_dbgfs/fw_assert, mode: wr */
	if (!debugfs_create_file("fw_assert",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_fw_assert))
		DBGLOG(INIT, WARN,
			"create fw_assert dgbfs fail\n");

	/* /sys/kernel/debug/mtk_ser_dbgfs/conninfra_hang, mode: wr */
	if (!debugfs_create_file("conninfra_hang",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_conninfra_hang))
		DBGLOG(INIT, WARN,
			"create conninfra_hang dgbfs fail\n");

#if defined(_HIF_USB)
	/* /sys/kernel/debug/mtk_ser_dbgfs/bus_hang, mode: wr */
	if (!debugfs_create_file("bus_hang",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_bus_hang))
		DBGLOG(INIT, WARN,
			"create bus_hang dgbfs fail\n");

	/* /sys/kernel/debug/mtk_ser_dbgfs/bus_disconnect, mode: wr */
	if (!debugfs_create_file("bus_disconnect",
				 0644, serDbgFsDir, prGlueInfo,
				 &fops_bus_disconnect))
		DBGLOG(INIT, WARN,
			"create bus_disconnect dgbfs fail\n");
#endif
	return 0;
}

#endif

