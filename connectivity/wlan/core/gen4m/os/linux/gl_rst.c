/*******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/
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
#include "gl_coredump.h"
#include "gl_fw_log.h"
#include "gl_rst.h"

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
u_int8_t fgSimplifyResetFlow = FALSE;
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
u_int8_t g_IsWfsysResetOnFail = FALSE;
u_int8_t g_IsWfsysRstDone = TRUE;
u_int8_t g_fgRstRecover = FALSE;
#endif

uint8_t *apucRstReason[RST_REASON_MAX] = {
	(uint8_t *) DISP_STRING("RST_UNKNOWN"),
	(uint8_t *) DISP_STRING("RST_PROCESS_ABNORMAL_INT"),
	(uint8_t *) DISP_STRING("RST_DRV_OWN_FAIL"),
	(uint8_t *) DISP_STRING("RST_FW_ASSERT"),
	(uint8_t *) DISP_STRING("RST_FW_ASSERT_TIMEOUT"),
	(uint8_t *) DISP_STRING("RST_BT_TRIGGER"),
	(uint8_t *) DISP_STRING("RST_OID_TIMEOUT"),
	(uint8_t *) DISP_STRING("RST_CMD_TRIGGER"),
	(uint8_t *) DISP_STRING("RST_REQ_CHL_FAIL"),
	(uint8_t *) DISP_STRING("RST_FW_DL_FAIL"),
	(uint8_t *) DISP_STRING("RST_SER_TIMEOUT"),
	(uint8_t *) DISP_STRING("RST_SLP_PROT_TIMEOUT"),
	(uint8_t *) DISP_STRING("RST_REG_READ_DEADFEED"),
	(uint8_t *) DISP_STRING("RST_P2P_CHNL_GRANT_INVALID_TYPE"),
	(uint8_t *) DISP_STRING("RST_P2P_CHNL_GRANT_INVALID_STATE"),
	(uint8_t *) DISP_STRING("RST_SCAN_RECOVERY"),
	(uint8_t *) DISP_STRING("RST_ACCESS_REG_FAIL"),
	(uint8_t *) DISP_STRING("[Wi-Fi On] nicpmSetDriverOwn() failed!"),
	(uint8_t *) DISP_STRING("[Wi-Fi] [Read WCIR_WLAN_READY fail!]"),
	(uint8_t *) DISP_STRING("[Wi-Fi Off] Allocate CMD_INFO_T ==> FAILED."),
	(uint8_t *) DISP_STRING("RST_SDIO_RX_ERROR"),
	(uint8_t *) DISP_STRING("RST_WHOLE_CHIP_TRIGGER"),
	(uint8_t *) DISP_STRING("RST_MDDP_MD_TRIGGER_EXCEPTION"),
	(uint8_t *) DISP_STRING("RST_FWK_TRIGGER"),
	(uint8_t *) DISP_STRING("RST_SER_L1_FAIL"),
	(uint8_t *) DISP_STRING("RST_SER_L0P5_FAIL"),
	(uint8_t *) DISP_STRING("RST_CMD_EVT_FAIL"),
	(uint8_t *) DISP_STRING("RST_WDT"),
	(uint8_t *) DISP_STRING("RST_SMC_CMD_FAIL"),
	(uint8_t *) DISP_STRING("RST_PCIE_NOT_READY"),
	(uint8_t *) DISP_STRING("RST_DEVAPC"),
	(uint8_t *) DISP_STRING("Chip reset by AER"),
	(uint8_t *) DISP_STRING("RST_USER_CMD_TRIGGER")
};

const uint8_t *apucRstAction[] = {
	DISP_STRING("RST_FLAG_DO_CORE_DUMP"),
	DISP_STRING("RST_FLAG_PREVENT_POWER_OFF"),
	DISP_STRING("RST_FLAG_DO_WHOLE_RESET"),
	DISP_STRING("RST_FLAG_DO_L0P5_RESET"),
	DISP_STRING("RST_FLAG_DO_L1_RESET")
};

u_int8_t g_IsNeedWaitCoredump = FALSE;

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static enum _ENUM_CHIP_RESET_REASON_TYPE_T eResetReason;

#if CFG_CHIP_RESET_SUPPORT
static struct RESET_STRUCT wifi_rst;
u_int8_t fgIsResetting = FALSE;
u_int8_t fgIsDrvTriggerWholeChipReset = FALSE;
enum COREDUMP_SOURCE_TYPE g_Coredump_source;
u_int8_t fgIsRstPreventFwOwn = FALSE;
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
#if CFG_CHIP_RESET_SUPPORT

#if CFG_WMT_RESET_API_SUPPORT
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
static void mtk_wifi_reset(struct work_struct *work);
static void mtk_wifi_trigger_reset(struct work_struct *work);
static void glResetCallback(enum _ENUM_WMTDRV_TYPE_T eSrcType,
			     enum _ENUM_WMTDRV_TYPE_T eDstType,
			     enum _ENUM_WMTMSG_TYPE_T eMsgType, void *prMsgBody,
			     unsigned int u4MsgLength);
#endif
static void reset_dump_pending_req(struct reset_pending_req *req);
static uint32_t reset_handle_pending_req(void);
static uint32_t reset_store_pending_req(struct RESET_STRUCT *rst,
	const uint32_t flag,
	const uint8_t *file,
	const uint32_t line,
	const u_int8_t fw_acked);
#else
#ifndef CFG_CHIP_RESET_KO_SUPPORT
static u_int8_t is_bt_exist(void);
static u_int8_t rst_L0_notify_step1(void);
static void wait_core_dump_end(void);
#endif
#endif
#endif

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
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
#if CFG_MTK_ANDROID_WMT && !IS_ENABLED(CFG_SUPPORT_CONNAC1X)
	update_driver_reset_status(fgIsResetting);
#endif
}

#if CFG_CHIP_RESET_SUPPORT
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

	rst = (struct RESET_STRUCT *)container_of(notifier,
		struct RESET_STRUCT, pm_nb);

	DBGLOG(REQ, INFO, "pm_event: %lu %s\n",
		pm_event, pm_evt_to_str(pm_event));

	switch (pm_event) {
	case PM_SUSPEND_PREPARE:
		rst->is_suspend = TRUE;
		break;
	case PM_POST_SUSPEND:
		rst->is_suspend = FALSE;
		reset_handle_pending_req();
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}
#endif

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
#if CFG_WMT_RESET_API_SUPPORT
	/* 1. Register reset callback */
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
	mtk_wcn_wmt_msgcb_reg(WMTDRV_TYPE_WIFI, glResetCallback);
	/* 2. Initialize reset work */
	INIT_WORK(&(wifi_rst.rst_trigger_work),
		  mtk_wifi_trigger_reset);
	INIT_WORK(&(wifi_rst.rst_work), mtk_wifi_reset);
#endif
#endif
	glResetUpdateFlag(FALSE);
	fgIsRstPreventFwOwn = FALSE;
	wifi_rst.prGlueInfo = prGlueInfo;

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
	register_pm_notifier(&wifi_rst.pm_nb);
#endif
#endif
	wifi_coredump_init(prGlueInfo);
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
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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

	if (!req)
		return WLAN_STATUS_INVALID_DATA;

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

	if (!rst->is_suspend) {
		DBGLOG(INIT, ERROR,
			"Pending reset request only accepted in suspend mode.\n");
		return WLAN_STATUS_NOT_ACCEPTED;
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
		return WLAN_STATUS_RESOURCES;
	}

	req->flag = flag;
	req->fw_acked = fw_acked;
	kalSnprintf(req->file, sizeof(req->file), "%s", file);
	req->line = line;
	req->fw_acked = fw_acked;

	DBGLOG(INIT, INFO, "Store pending req: 0x%p.\n", req);
	rst->pending_req = req;

	return WLAN_STATUS_SUCCESS;
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
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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
	case RST_CMD_EVT_FAIL:
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
		u4RstFlag = RST_FLAG_CHIP_RESET;
#else
		u4RstFlag = RST_FLAG_WF_RESET;
#endif
		break;

	case RST_SER_TIMEOUT:
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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
	case RST_SCAN_RECOVERY:
	default:
		u4RstFlag = RST_FLAG_CHIP_RESET;
		break;
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
void glResetTrigger(struct ADAPTER *prAdapter,
		uint32_t u4RstFlag, const uint8_t *pucFile, uint32_t u4Line)
{
	struct RESET_STRUCT *rst = &wifi_rst;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps = NULL;
#if !IS_ENABLED(CFG_SUPPORT_CONNAC1X)
	int ret = 0;
#endif

	if (kalIsResetting() || !prAdapter)
		goto exit;

#if CFG_MTK_MDDP_SUPPORT
	mddpNotifyWifiReset();
#endif

	glResetUpdateFlag(TRUE);

#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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

	prChipInfo = prAdapter->chip_info;
	prDbgOps = prChipInfo->prDebugOps;

	dump_stack();
	DBGLOG(INIT, ERROR,
		"Trigger chip reset in %s#%u, bus[%d] flag[0x%x] reason[%s]\n",
		pucFile,
		u4Line,
		g_IsWfsysBusHang,
		u4RstFlag,
		apucRstReason[eResetReason]);

	prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
	halPrintHifDbgInfo(prAdapter);

	if (glGetRstReason() != RST_USER_CMD_TRIGGER &&
		prDbgOps && prDbgOps->dumpBusHangCr)
		prDbgOps->dumpBusHangCr(prAdapter);

#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
	rst->rst_trigger_flag = u4RstFlag;
	schedule_work(&rst->rst_trigger_work);
#else
	if (u4RstFlag & RST_FLAG_DO_CORE_DUMP)
		g_fgRstRecover = FALSE;
	else
		g_fgRstRecover = TRUE;

	/* check if whole chip reset is triggered */
	if (g_IsWfsysBusHang)
		goto exit;

	if (u4RstFlag & RST_FLAG_DO_WHOLE_RESET) {
		glResetWholeChipResetTrigger(g_reason);
		goto exit;
	}

	g_Coredump_source = COREDUMP_SOURCE_WF_DRIVER;
	if (!prChipInfo->trigger_fw_assert) {
		DBGLOG(INIT, ERROR,
			"No impl. of trigger_fw_assert API\n");
		goto exit;
	}
	if (prAdapter->fgIsFwOwn &&
		glGetRstReason() == RST_USER_CMD_TRIGGER) {
		kalSetRstEvent(FALSE);
		goto exit;
	}
	ret = prChipInfo->trigger_fw_assert(prAdapter);
	if (ret == -EBUSY)
		goto exit;

	if (rst->is_suspend) {
		uint32_t status;

		status = reset_store_pending_req(rst,
						 u4RstFlag,
						 pucFile,
						 u4Line,
						 ret != -ETIMEDOUT);
		if (status == WLAN_STATUS_SUCCESS)
			goto exit;
	}

	if (ret == -ETIMEDOUT)
		kalSetRstEvent(FALSE);
	else
		kalSetRstEvent(TRUE);
#endif
exit:
	fgIsMcuOff = FALSE;
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
void glResetTrigger(struct ADAPTER *prAdapter, uint32_t u4RstFlag,
		    const uint8_t *pucFile, uint32_t u4Line)
{
	struct CHIP_DBG_OPS *prChipDbg;
	uint16_t u2FwOwnVersion;
	uint16_t u2FwPeerVersion;
	uint16_t i;
	u_int8_t fgDrvOwn;

	dump_stack();
	if (kalIsResetting())
		return;

	if (prAdapter == NULL)
		prAdapter = wifi_rst.prGlueInfo->prAdapter;

	prChipDbg = prAdapter->chip_info->prDebugOps;
	u2FwOwnVersion = prAdapter->rVerInfo.u2FwOwnVersion;
	u2FwPeerVersion = prAdapter->rVerInfo.u2FwPeerVersion;
	fgDrvOwn = TRUE;

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
	halPrintHifDbgInfo(prAdapter);

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
		fgIsResetting = TRUE;
		fgSimplifyResetFlow = FALSE;
		wifi_rst.prGlueInfo = prAdapter->prGlueInfo;
		schedule_work(&(wifi_rst.rst_work));
	} else if ((u4RstFlag & RST_FLAG_DO_L0P5_RESET) &&
	      prAdapter->chip_info->fgIsSupportL0p5Reset) {
		KAL_ACQUIRE_SPIN_LOCK_BH(prAdapter,
			SPIN_LOCK_WFSYS_RESET);
		fgIsResetting = TRUE;
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

	prGlueInfo = container_of(work, struct GLUE_INFO, rWfsysResetWork);
	prAdapter = prGlueInfo->prAdapter;
	prHifDrvData = container_of(&prAdapter->chip_info,
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

		glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_RESET);

		HAL_CANCEL_TX_RX(prAdapter);

		if (wlanOffAtReset() != WLAN_STATUS_SUCCESS)
			goto FAIL;

		fgIsResetting = FALSE;
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

	glSetWfsysResetState(prAdapter, WFSYS_RESET_STATE_IDLE);

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

	fgIsResetting = FALSE;

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
	char uevent[64] = {0};

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
#if !IS_ENABLED(CFG_SUPPORT_CONNAC1X)
	if (g_IsWholeChipRst == TRUE) {
		g_IsWholeChipRst = FALSE;
		g_IsWfsysBusHang = FALSE;
		complete(&g_RstOnComp);
	}
#endif
#else
#ifdef CFG_CHIP_RESET_KO_SUPPORT
#if defined(_HIF_USB)
	rstNotifyWholeChipRstStatus(RST_MODULE_WIFI,
				RST_MODULE_STATE_PRERESET, NULL);
#endif
#if defined(_HIF_SDIO)
	rstNotifyWholeChipRstStatus(RST_MODULE_WIFI,
				RST_MODULE_STATE_PRERESET,
				rst->prGlueInfo->rHifInfo.func);
#endif
#else /* CFG_CHIP_RESET_KO_SUPPORT */
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

#endif  /* CFG_CHIP_RESET_KO_SUPPORT */

	if (fgSimplifyResetFlow) {
		DBGLOG(INIT, INFO, "Force down the reset flag.\n");
		fgSimplifyResetFlow = FALSE;
	}

	if (mtk_cfg80211_vendor_event_reset_triggered(
					(uint32_t) eResetReason) != 0)
		DBGLOG(INIT, ERROR, "Send WIFI_EVENT_RESET_TRIGGERED Error!\n");

	DBGLOG(INIT, STATE, "[SER][L0] flow end, fgResult=%d, ret: %d\n",
		fgResult, ret);

	if (glGetRstReason() == RST_USER_CMD_TRIGGER &&
		g_IsWholeChipRst == FALSE) {
		kalSnprintf(uevent, sizeof(uevent),
			"recoveryNotify=reset:%d,reason:%d", fgL0Reset,
			(BIT(glGetRstReason()) & RST_REASON_FW) > 0 ? 1:0);
		kalSendUevent(uevent);
	}

}
#endif

#if CFG_WMT_RESET_API_SUPPORT
#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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
	struct RESET_STRUCT *rst = container_of(work,
						struct RESET_STRUCT, rst_work);
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

static void mtk_wifi_trigger_reset(struct work_struct *work)
{
	u_int8_t fgResult = FALSE;
	struct RESET_STRUCT *rst = container_of(work,
					struct RESET_STRUCT, rst_trigger_work);

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
	struct CHIP_DBG_OPS *debug_ops;

	if (fgIsResetting)
		return;

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	if (!prGlueInfo || !prGlueInfo->u4ReadyFlag || !prGlueInfo->prAdapter)
		return;

	prAdapter = prGlueInfo->prAdapter;
	prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
	debug_ops = prAdapter->chip_info->prDebugOps;

	if (debug_ops && debug_ops->dumpBusHangCr)
		debug_ops->dumpBusHangCr(prAdapter);

	kalSetHifDbgEvent(prAdapter->prGlueInfo);
	/* wait for hif_thread finish dump */
	kalMsleep(100);
}

#if IS_ENABLED(CFG_SUPPORT_CONNAC1X)
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
		break;

	case ENUM_RST_MSG_L04_START:
	case ENUM_RST_MSG_L05_START:
		DBGLOG(INIT, WARN, "WF chip reset start!\n");
#ifdef CONFIG_MTK_CONNSYS_DEDICATED_LOG_PATH
		fw_log_handler();
#endif
		glResetUpdateFlag(TRUE);
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
	while (get_wifi_process_status() == 1) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		return bRet;
	}

	triggerHifDumpIfNeed();

	g_Coredump_source = coredump_conn_type_to_src(type);
	g_WholeChipRstReason = reason;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetting()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			msleep(100);
		}
		while ((!prGlueInfo) ||
			(prGlueInfo->u4ReadyFlag == 0) ||
			(g_IsWfsysRstDone == FALSE)) {
			WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
			DBGLOG(REQ, WARN, "wifi driver is not ready\n");
			if (g_IsWfsysResetOnFail == TRUE) {
				DBGLOG(REQ, WARN,
					"wifi driver reset fail, need whole chip reset.\n");
				g_IsWholeChipRst = TRUE;
				return bRet;
			}
			msleep(100);
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

		while (kalIsResetting() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			msleep(100);
		}
		fgIsDrvTriggerWholeChipReset = FALSE;
		g_IsWholeChipRst = TRUE;

		if (!prGlueInfo->u4ReadyFlag)
			g_IsNeedWaitCoredump = TRUE;

		kalSetRstEvent(FALSE);
	}
	wait_for_completion(&g_RstOffComp);
	DBGLOG(INIT, INFO, "Wi-Fi is off successfully.\n");

	return bRet;
}

int glRstwlanPostWholeChipReset(void)
{
	while (get_wifi_process_status() == 1) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
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
	char *reason)
{
	struct GLUE_INFO *prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	struct BUS_INFO *prBusInfo = NULL;

	DBGLOG(INIT, INFO,
		"drv: %d, reason: %s\n",
		drv, reason);

	while (get_wifi_process_status() == 1) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}

	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		return 0;
	}

	WIPHY_PRIV(wlanGetWiphy(), prGlueInfo);
	prAdapter = prGlueInfo->prAdapter;
	prBusInfo = prAdapter->chip_info->bus_info;

	if (drv == CONNV3_DRV_TYPE_CONNV3) {
		if (prGlueInfo->u4ReadyFlag &&
		    kalStrnCmp(reason, "PMIC Fault", 10) == 0) {
			fgIsBusAccessFailed = TRUE;
			g_IsWfsysBusHang = TRUE;
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
			if (prBusInfo->disableDevice)
				prBusInfo->disableDevice(prGlueInfo);
#endif
		}
	}

	if (!fgIsBusAccessFailed && drv != CONNV3_DRV_TYPE_WIFI)
		triggerHifDumpIfNeed();

	g_Coredump_source = coredump_connv3_type_to_src(drv);
	g_WholeChipRstReason = reason;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetting()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			msleep(100);
		}

		g_IsWholeChipRst = TRUE;

		GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
					 RST_WHOLE_CHIP_TRIGGER);
	} else {
		while (kalIsResetting() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			msleep(100);
		}
		fgIsDrvTriggerWholeChipReset = FALSE;
		g_IsWholeChipRst = TRUE;

		kalSetRstEvent(TRUE);
	}

	wait_for_completion(&g_RstOffComp);
	DBGLOG(INIT, INFO, "Wi-Fi is off successfully.\n");

	return 0;
}

int wlan_post_whole_chip_rst_v3(void)
{
	DBGLOG(INIT, INFO, "wlan_post_whole_chip_rst_v3\n");

	fgIsBusAccessFailed = FALSE;
	glRstSetRstEndEvent();

	return 0;
}

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

	while (get_wifi_process_status() == 1) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}

	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		return 0;
	}

	triggerHifDumpIfNeed();

	g_Coredump_source = coredump_conn_type_to_src(drv);
	g_WholeChipRstReason = reason;

	if (glRstCheckRstCriteria()) {
		while (kalIsResetting()) {
			DBGLOG(REQ, WARN, "wifi driver is resetting\n");
			msleep(100);
		}

		g_IsWholeChipRst = TRUE;

		GL_DEFAULT_RESET_TRIGGER(prGlueInfo->prAdapter,
					 RST_WHOLE_CHIP_TRIGGER);
	} else {
		while (kalIsResetting() &&
				fgIsDrvTriggerWholeChipReset == FALSE) {
			DBGLOG(REQ, WARN, "Wi-Fi driver is resetting\n");
			msleep(100);
		}
		fgIsDrvTriggerWholeChipReset = FALSE;
		g_IsWholeChipRst = TRUE;

		kalSetRstEvent(TRUE);
	}

	wait_for_completion(&g_RstOffComp);
	DBGLOG(INIT, INFO, "Wi-Fi is off successfully.\n");

	return 0;
}

int wlan_post_whole_chip_rst_v2(void)
{
	while (get_wifi_process_status() == 1) {
		DBGLOG(REQ, WARN,
			"Wi-Fi on/off process is ongoing, wait here.\n");
		msleep(100);
	}
	if (!get_wifi_powered_status()) {
		DBGLOG(REQ, WARN, "wifi driver is off now\n");
		return 0;
	}
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
	KAL_GET_PTIME_OF_USEC_OR_NSEC(rNowTs) = 0;
	rLastTs->tv_sec = 0;
	KAL_GET_PTIME_OF_USEC_OR_NSEC(rLastTs) = 0;
}

bool IsOverRstTimeThreshold(
	struct timespec64 *rNowTs, struct timespec64 *rLastTs)
{
#if (CFG_SUPPORT_CONNINFRA == 1)
	struct timespec64 rTimeout, rTime = {0};
	bool fgIsTimeout = FALSE;

	rTimeout.tv_sec = 30;
	KAL_GET_TIME_OF_USEC_OR_NSEC(rTimeout) = 0;
	ktime_get_ts64(rNowTs);
	DBGLOG(INIT, INFO,
		"Reset happen time :%d.%d, last happen time :%d.%d\n",
		rNowTs->tv_sec,
		KAL_GET_PTIME_OF_USEC_OR_NSEC(rNowTs),
		rLastTs->tv_sec,
		KAL_GET_PTIME_OF_USEC_OR_NSEC(rLastTs));
	if (rLastTs->tv_sec != 0) {
		if (halGetDeltaTime(rNowTs, rLastTs, &rTime)) {
			if (halTimeCompare(&rTime, &rTimeout) >= 0)
				fgIsTimeout = TRUE;
			else
				fgIsTimeout = FALSE;
		}
		DBGLOG(INIT, INFO,
			"Reset rTimeout :%d.%ld, calculate time :%d.%ld\n",
			rTimeout.tv_sec,
			KAL_GET_TIME_OF_USEC_OR_NSEC(rTimeout),
			rTime.tv_sec,
			KAL_GET_TIME_OF_USEC_OR_NSEC(rTime));
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
	prDebugOps = prAdapter->chip_info->prDebugOps;

	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (prGlueInfo->u4ReadyFlag && dumpViaBt) {
		if (prDebugOps && prDebugOps->dumpBusHangCr)
			prDebugOps->dumpBusHangCr(prAdapter);
	}
#endif

#if (CFG_SUPPORT_CONNINFRA == 1)
	ret = conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_WIFI, pcReason);
#elif IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	ret = connv3_trigger_whole_chip_rst(CONNV3_DRV_TYPE_WIFI, pcReason);
#else
	DBGLOG(INIT, WARN, "whole chip reset NOT support\n");
#endif

	DBGLOG(INIT, INFO, "ret: %d\n", ret);
	if (ret == 0) {
		dump_stack();
		fgIsDrvTriggerWholeChipReset = TRUE;
	}
}

void glResetSubsysRstProcedure(struct RESET_STRUCT *rst,
	struct timespec64 *rNowTs,
	struct timespec64 *rLastTs)
{
	struct GLUE_INFO *prGlueInfo = rst->prGlueInfo;
	struct ADAPTER *prAdapter = NULL;
	bool fgIsTimeout;

	if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
		struct WIFI_VAR *prWifiVar = NULL;

		prAdapter = prGlueInfo->prAdapter;
		prWifiVar = &prAdapter->rWifiVar;
		if (prWifiVar->fgRstRecover == 1)
			g_fgRstRecover = TRUE;
	}

	fgIsTimeout = IsOverRstTimeThreshold(rNowTs, rLastTs);
	if (g_IsWfsysBusHang == TRUE && prAdapter) {
		struct CHIP_DBG_OPS *debug_ops = prAdapter->chip_info->prDebugOps;

		if (prGlueInfo && prGlueInfo->u4ReadyFlag) {
			/* dump host cr */
			if (debug_ops && debug_ops->dumpBusHangCr)
				debug_ops->dumpBusHangCr(prAdapter);
			fgIsDrvTriggerWholeChipReset = TRUE;
			glSetRstReasonString(
				"fw detect bus hang");
			glResetWholeChipResetTrigger(g_reason);
		} else {
#if (CFG_SUPPORT_CONNINFRA == 1)
			if (conninfra_reg_readable_for_coredump() == 1 &&
			    debug_ops &&
			    debug_ops->dumpBusHangCr)
				debug_ops->dumpBusHangCr(prAdapter);
#endif
			DBGLOG(INIT, INFO,
				"Don't trigger whole chip reset due to driver is not ready\n");
		}
		return;
	}

	if (g_SubsysRstCnt > 3) {
		if (fgIsTimeout == TRUE) {
		/*
		 * g_SubsysRstCnt > 3, > 30 sec,
		 * need to update rLastTs, still do wfsys reset
		 */
			if (eResetReason >= RST_REASON_MAX)
				eResetReason = 0;

			if (g_fgRstRecover == TRUE)
				g_fgRstRecover = FALSE;
			else if (glGetRstReason() != RST_USER_CMD_TRIGGER)
				wifi_coredump_start(
					g_Coredump_source,
					apucRstReason[eResetReason],
					rst->force_dump);

			g_IsNeedWaitCoredump = FALSE;

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
				glResetUpdateFlag(FALSE);
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
		}
	} else {
		if (eResetReason >= RST_REASON_MAX)
			eResetReason = 0;

		if (g_fgRstRecover == TRUE)
			g_fgRstRecover = FALSE;
		else if (glGetRstReason() != RST_USER_CMD_TRIGGER)
			wifi_coredump_start(
				g_Coredump_source,
				apucRstReason[eResetReason],
				rst->force_dump);

		g_IsNeedWaitCoredump = FALSE;

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
			glResetUpdateFlag(FALSE);
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
		rLastTs->tv_sec = rNowTs->tv_sec;
		KAL_GET_PTIME_OF_USEC_OR_NSEC(rLastTs) =
			KAL_GET_PTIME_OF_USEC_OR_NSEC(rNowTs);
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

#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
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
				if (glGetRstReason() != RST_USER_CMD_TRIGGER) {
					wifi_coredump_start(
						g_Coredump_source,
						g_WholeChipRstReason,
						rst->force_dump);
					rst->force_dump = FALSE;
					g_IsNeedWaitCoredump = FALSE;
				}

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
					glResetUpdateFlag(FALSE);
				}
			} else {
				/*wfsys reset start*/
				g_IsWfsysRstDone = FALSE;
				g_SubsysRstCnt++;
				DBGLOG(INIT, INFO,
					"WF reset count = %d.\n",
					g_SubsysRstCnt);
				glResetSubsysRstProcedure(rst,
							 &rNowTs,
							 &rLastTs);
				/*wfsys reset done*/
				g_IsWfsysRstDone = TRUE;
			}
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

#if defined(CONFIG_ANDROID) && (CFG_ENABLE_WAKE_LOCK)
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prWlanRstThreadWakeLock))
		KAL_WAKE_UNLOCK(NULL, prWlanRstThreadWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL,
			      prWlanRstThreadWakeLock);
#endif

	DBGLOG(INIT, TRACE, "%s:%u stopped!\n",
	       KAL_GET_CURRENT_THREAD_NAME(), KAL_GET_CURRENT_THREAD_ID());
	complete(&rst->halt_comp);

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
	uint32_t waitRet = 0, timeout = 0;

	if (glGetRstReason() != RST_USER_CMD_TRIGGER)
		timeout = WIFI_TRIGGER_ASSERT_TIMEOUT;
	else
		timeout = WIFI_USER_TRIGGER_ASSERT_TIMEOUT;

	waitRet = wait_for_completion_timeout(&g_triggerComp,
		MSEC_TO_JIFFIES(timeout));
	if (waitRet > 0) {
		/* Case 1: No timeout. */
		DBGLOG(INIT, INFO, "Trigger assert successfully.\n");
	} else {
		/* Case 2: timeout */
		DBGLOG(INIT, ERROR,
			"Trigger assert more than %d ms, need to trigger rst self\n",
			timeout);
	}

	return waitRet > 0 ? 0 : -ETIMEDOUT;
}

void reset_done_trigger_completion(void)
{
	complete(&g_triggerComp);
}
#else
#ifndef CFG_CHIP_RESET_KO_SUPPORT
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

