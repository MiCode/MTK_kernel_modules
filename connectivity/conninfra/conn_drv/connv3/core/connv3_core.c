// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include "conninfra_conf.h"
#include "connv3.h"
#include "connv3_hw.h"
#include "connv3_core.h"
#include "msg_thread.h"
#include "osal_dbg.h"
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
#include <connectivity_build_in_adapter.h>
#endif

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
/* Control pmic and ext32k switch behavior when all radio off
 * 1: pmic/ext32k would be turn off
 * 0: pmic/ext32k keeps on
 */
#define CONNV3_PWR_OFF_MODE_PMIC_OFF			0

#define CONNV3_EVENT_TIMEOUT				3000
#define CONNV3_RESET_TIMEOUT				500
#define CONNV3_PRE_CAL_TIMEOUT				500
#define CONNV3_FMD_TIMEOUT				500
#define CONNV3_MAX_PRE_CAL_BLOCKING_TIME		60000
#define CONNV3_PRE_CAL_OP_TIMEOUT			60000 /* 60 sec */

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/delay.h>
#include <linux/ratelimit.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

enum connv3_pwr_dump_type {
	CONNV3_PWR_INFO_RESET = 0,
	CONNV3_PWR_INFO_DUMP,
	CONNV3_PWR_INFO_DUMP_AND_RESET,
	CONNV3_PWR_INFO_MAX,
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int opfunc_power_on(struct msg_op_data *op);
static int opfunc_power_off(struct msg_op_data *op);
static int opfunc_power_on_done(struct msg_op_data *op);
static int opfunc_chip_rst(struct msg_op_data *op);
static int opfunc_pre_cal(struct msg_op_data *op);
static int opfunc_pre_cal_prepare(struct msg_op_data *op);
static int opfunc_pre_cal_check(struct msg_op_data *op);
static int opfunc_ext_32k_on(struct msg_op_data *op);
static int opfunc_reset_power_state(struct msg_op_data *op);
static int opfunc_dump_power_state(struct msg_op_data *op);
static int opfunc_reset_and_dump_power_state(struct msg_op_data *op);
static int opfunc_enter_fmd_mode(struct msg_op_data *op);

static int opfunc_subdrv_pre_reset(struct msg_op_data *op);
static int opfunc_subdrv_post_reset(struct msg_op_data *op);
static int opfunc_subdrv_cal_pre_on(struct msg_op_data *op);
static int opfunc_subdrv_cal_pwr_on(struct msg_op_data *op);
static int opfunc_subdrv_cal_do_cal(struct msg_op_data *op);
static int opfunc_subdrv_pre_pwr_on(struct msg_op_data *op);
static int opfunc_subdrv_pwr_on_notify(struct msg_op_data *op);
static int opfunc_subdrv_efuse_on(struct msg_op_data *op);
static int opfunc_subdrv_pre_cal_fail(struct msg_op_data *op);
static int opfunc_subdrv_pwr_down_notify(struct msg_op_data *op);
static int opfunc_subdrv_post_reset_on(struct msg_op_data *op);
static int opfunc_subdrv_fmd_pre_cb(struct msg_op_data *op);
static int opfunc_subdrv_fmd_post_cb(struct msg_op_data *op);

static void _connv3_core_update_rst_status(enum chip_rst_status status);

static void connv3_core_wake_lock_get(void);
static void connv3_core_wake_lock_put(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct connv3_ctx g_connv3_ctx;
static struct osal_wake_lock g_connv3_wake_lock;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static const msg_opid_func connv3_core_opfunc[] = {
	[CONNV3_OPID_PWR_ON] = opfunc_power_on,
	[CONNV3_OPID_PWR_OFF] = opfunc_power_off,
	[CONNV3_OPID_PWR_ON_DONE] = opfunc_power_on_done,
	[CONNV3_OPID_EXT_32K_ON] = opfunc_ext_32k_on,
	/* Pre-cal */
	[CONNV3_OPID_PRE_CAL_PREPARE] = opfunc_pre_cal_prepare,
	[CONNV3_OPID_PRE_CAL_CHECK] = opfunc_pre_cal_check,
	/* Power dump */
	[CONNV3_OPID_RESET_POWER_STATE] = opfunc_reset_power_state,
	[CONNV3_OPID_DUMP_POWER_STATE] = opfunc_dump_power_state,
	[CONNV3_OPID_RESET_AND_DUMP_POWER_STATE] = opfunc_reset_and_dump_power_state,
};

static const msg_opid_func connv3_core_cb_opfunc[] = {
	[CONNV3_CB_OPID_CHIP_RST] = opfunc_chip_rst,
	[CONNV3_CB_OPID_PRE_CAL] = opfunc_pre_cal,
	[CONNV3_CB_OPID_FMD_MODE] = opfunc_enter_fmd_mode,
};


/* subsys ops */
static char *connv3_drv_thread_name[] = {
	[CONNV3_DRV_TYPE_BT] = "sub_bt_thrd",
	[CONNV3_DRV_TYPE_WIFI] = "sub_wifi_thrd",
	[CONNV3_DRV_TYPE_MODEM] = "sub_md_thrd",
	[CONNV3_DRV_TYPE_CONNV3] = "connv3_thrd",
};

static char *connv3_drv_name[] = {
	[CONNV3_DRV_TYPE_BT] = "BT",
	[CONNV3_DRV_TYPE_WIFI] = "WIFI",
	[CONNV3_DRV_TYPE_MODEM] = "MODEM",
	[CONNV3_DRV_TYPE_CONNV3] = "CONNV3",
};

typedef enum {
	CONNV3_SUBDRV_OPID_PRE_RESET	= 0,
	CONNV3_SUBDRV_OPID_POST_RESET	= 1,
	CONNV3_SUBDRV_OPID_CAL_PRE_ON	= 2,
	CONNV3_SUBDRV_OPID_CAL_PWR_ON	= 3,
	CONNV3_SUBDRV_OPID_CAL_DO_CAL	= 4,
	CONNV3_SUBDRV_OPID_PRE_PWR_ON	= 5,
	CONNV3_SUBDRV_OPID_PWR_ON_NOTIFY= 6,
	CONNV3_SUBDRV_OPID_CAL_EFUSE_ON = 7,
	CONNV3_SUBDRV_OPID_PRE_CAL_FAIL = 8,
	CONNV3_SUBDRV_OPID_POWER_OFF_NOTIFY = 9,
	CONNV3_SUBDRV_OPID_POST_RST_ON  = 10,
	CONNV3_SUBDRV_OPID_FMD_PRE_CB	= 11,
	CONNV3_SUBDRV_OPID_FMD_POST_CB	= 12,
	CONNV3_SUBDRV_OPID_MAX
} connv3_subdrv_op;


static const msg_opid_func connv3_subdrv_opfunc[] = {
	[CONNV3_SUBDRV_OPID_PRE_RESET] = opfunc_subdrv_pre_reset,
	[CONNV3_SUBDRV_OPID_POST_RESET] = opfunc_subdrv_post_reset,
	[CONNV3_SUBDRV_OPID_CAL_PRE_ON] = opfunc_subdrv_cal_pre_on,
	[CONNV3_SUBDRV_OPID_CAL_PWR_ON] = opfunc_subdrv_cal_pwr_on,
	[CONNV3_SUBDRV_OPID_CAL_DO_CAL] = opfunc_subdrv_cal_do_cal,
	[CONNV3_SUBDRV_OPID_PRE_PWR_ON] = opfunc_subdrv_pre_pwr_on,
	[CONNV3_SUBDRV_OPID_PWR_ON_NOTIFY] = opfunc_subdrv_pwr_on_notify,
	[CONNV3_SUBDRV_OPID_CAL_EFUSE_ON] = opfunc_subdrv_efuse_on,
	[CONNV3_SUBDRV_OPID_PRE_CAL_FAIL] = opfunc_subdrv_pre_cal_fail,
	[CONNV3_SUBDRV_OPID_POWER_OFF_NOTIFY] = opfunc_subdrv_pwr_down_notify,
	[CONNV3_SUBDRV_OPID_POST_RST_ON] = opfunc_subdrv_post_reset_on,
	[CONNV3_SUBDRV_OPID_FMD_PRE_CB] = opfunc_subdrv_fmd_pre_cb,
	[CONNV3_SUBDRV_OPID_FMD_POST_CB] = opfunc_subdrv_fmd_post_cb,
};

enum pre_cal_type {
	PRE_CAL_MODE_DEFAULT = 0,
	PRE_CAL_ALL_ENABLED = 1,
	PRE_CAL_ALL_DISABLED = 2,
	PRE_CAL_PWR_ON_DISABLED = 3,
	PRE_CAL_SCREEN_ON_DISABLED = 4
};

static unsigned int g_pre_cal_mode = PRE_CAL_SCREEN_ON_DISABLED;

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static void reset_chip_rst_trg_data(void)
{
	memset(g_connv3_ctx.trg_reason, '\0', CONNV3_CHIP_RST_SOURCE_MAX*CHIP_RST_REASON_MAX_LEN);
}

static unsigned long timespec64_to_ms(struct timespec64 *begin, struct timespec64 *end)
{
	unsigned long time_diff;

	time_diff = (end->tv_sec - begin->tv_sec) * MSEC_PER_SEC;
	time_diff += (end->tv_nsec - begin->tv_nsec) / NSEC_PER_MSEC;

	return time_diff;
}

static unsigned int opfunc_get_current_status(void)
{
	unsigned int ret = 0;
	unsigned int i;

	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++)
		if (g_connv3_ctx.drv_inst[i].drv_status == DRV_STS_POWER_ON ||
			g_connv3_ctx.drv_inst[i].drv_status == DRV_STS_PRE_POWER_ON)
			ret |= (0x1 << i);

	return ret;
}

static void dump_curr_status(char *tag)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;

	if (tag == NULL)
		tag = "Connv3 Pwr status";

	pr_info("[%s] core_status=[%d] BT=[%d] WF=[%d] MD=[%d]",
		tag, ctx->core_status,
		ctx->drv_inst[CONNV3_DRV_TYPE_BT].drv_status,
		ctx->drv_inst[CONNV3_DRV_TYPE_WIFI].drv_status,
		ctx->drv_inst[CONNV3_DRV_TYPE_MODEM].drv_status);
}

static int opfunc_power_down_notify(void)
{
	unsigned int drv_type;
	struct subsys_drv_inst *drv_inst;
	int ret;

	pr_info("[%s] called\n", __func__);
	for (drv_type = 0; drv_type < CONNV3_DRV_TYPE_MAX; drv_type++) {
		drv_inst = &g_connv3_ctx.drv_inst[drv_type];
		ret = msg_thread_send_1(
			&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_POWER_OFF_NOTIFY, drv_type);
	}

	return 0;
}


static int opfunc_power_on_internal(unsigned int drv_type)
{
	int ret, i;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	u32 cur_pre_on_state;
	const unsigned int subdrv_all_done = (0x1 << CONNV3_DRV_TYPE_MAX) - 1;
	unsigned int subdrv_preon_done;
	struct subsys_drv_inst *drv_inst;

	/* Check abnormal type */
	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_err("abnormal Fun(%d)\n", drv_type);
		return -EINVAL;
	}

	/* Check abnormal state */
	if ((g_connv3_ctx.drv_inst[drv_type].drv_status < DRV_STS_POWER_OFF)
	    || (g_connv3_ctx.drv_inst[drv_type].drv_status >= DRV_STS_MAX)) {
		pr_err("func(%d) status[0x%x] abnormal\n", drv_type,
				g_connv3_ctx.drv_inst[drv_type].drv_status);
		return -EINVAL;
	}

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("core_lock fail!!");
		return ret;
	}

	/* check if func already on */
	if (g_connv3_ctx.drv_inst[drv_type].drv_status == DRV_STS_POWER_ON) {
		pr_warn("func(%d) already on\n", drv_type);
		osal_unlock_sleepable_lock(&ctx->core_lock);
		return 0;
	}

	/* check if platform ready */
	ret = connv3_hw_check_status();
	if (ret != CONNV3_PLT_STATE_READY) {
		pr_notice("[CONNV3_PWR_ON] ret = %d\n", ret);
		osal_unlock_sleepable_lock(&ctx->core_lock);
		return CONNV3_ERR_CLOCK_NOT_READY;
	}

	/* g_connv3_ctx.core_status meaning
	 * - DRV_STS_POWER_OFF: all radio is off.
	 * 	(pmic_en is 0 or 1 for power off uds mode)
	 * - DRV_STS_RESET: pmic_en is 1 and POR_RST has been happened.
	 * - DRV_STS_POWER_ON: pmic_en is 1 and at least one radio has called connv3_pwr_on.
	 */
	if (g_connv3_ctx.core_status == DRV_STS_POWER_OFF ||
	    g_connv3_ctx.core_status == DRV_STS_RESET) {
		if (g_connv3_ctx.core_status == DRV_STS_POWER_OFF) {
			/* power recycle */
			ret = connv3_hw_pwr_off(0, CONNV3_DRV_TYPE_MAX, NULL);
			if (ret) {
				pr_err("[%s] connv3 power recycle fail. drv=[%d] ret=[%d]\n",
					__func__, drv_type, ret);
				osal_unlock_sleepable_lock(&ctx->core_lock);
				return ret;
			}
		}

		/* pre_power_on flow */
		atomic_set(&g_connv3_ctx.pre_pwr_state, 0);
		sema_init(&g_connv3_ctx.pre_pwr_sema, 1);

		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			drv_inst = &g_connv3_ctx.drv_inst[i];
			ret = msg_thread_send_1(&drv_inst->msg_ctx,
					CONNV3_SUBDRV_OPID_PRE_PWR_ON, i);
		}

		pr_info("[CONNV3_PWR_ON] pre vvvvvvvvvvvvv");
		while (atomic_read(&g_connv3_ctx.pre_pwr_state) != subdrv_all_done) {
			ret = down_timeout(&g_connv3_ctx.pre_pwr_sema, msecs_to_jiffies(CONNV3_RESET_TIMEOUT));
			if (ret == 0)
				continue;
			cur_pre_on_state = atomic_read(&g_connv3_ctx.pre_pwr_state);
			pr_info("cur_rst state =[%d]", cur_pre_on_state);
			for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
				if ((cur_pre_on_state & (0x1 << i)) == 0) {
					pr_info("[pre_pwr_on] [%s] pre-callback is not back", connv3_drv_thread_name[i]);
					drv_inst = &g_connv3_ctx.drv_inst[i];
					osal_thread_show_stack(&drv_inst->msg_ctx.thread);
				}
			}
		}


		/* POWER ON SEQUENCE */
		connv3_core_wake_lock_get();
		ret = connv3_hw_pwr_on(opfunc_get_current_status(), drv_type);
		connv3_core_wake_lock_put();

		if (ret) {
			pr_err("Connv3 power on fail. drv(%d) ret=(%d)\n",
				drv_type, ret);
			osal_unlock_sleepable_lock(&ctx->core_lock);
			return ret;
		}
		g_connv3_ctx.core_status = DRV_STS_POWER_ON;

		/* power on notify */
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			if (i == drv_type)
				continue;
			drv_inst = &g_connv3_ctx.drv_inst[i];
			ret = msg_thread_send_1(&drv_inst->msg_ctx,
					CONNV3_SUBDRV_OPID_PWR_ON_NOTIFY, i);
			if (drv_inst->ops_cb.pwr_on_cb.power_on_notify)
				drv_inst->drv_status = DRV_STS_PRE_POWER_ON;
		}
	} else {
		/* second/third radio power on */
		drv_inst = &g_connv3_ctx.drv_inst[drv_type];
		/* pre_power_on flow */
		atomic_set(&g_connv3_ctx.pre_pwr_state, 0);
		sema_init(&g_connv3_ctx.pre_pwr_sema, 1);

		ret = msg_thread_send_1(&drv_inst->msg_ctx, CONNV3_SUBDRV_OPID_PRE_PWR_ON, drv_type);
		if (ret)
			pr_notice("[%s] drv(%d) pre_on fail, ret=%d", __func__, drv_type, ret);
		subdrv_preon_done = (0x1 << drv_type);

		pr_info("[CONNV3_PWR_ON][%s] pre vvvvvvvvvvvvv", connv3_drv_name[drv_type]);
		while (atomic_read(&g_connv3_ctx.pre_pwr_state) != subdrv_preon_done) {
			ret = down_timeout(&g_connv3_ctx.pre_pwr_sema, msecs_to_jiffies(CONNV3_RESET_TIMEOUT));
			if (ret == 0)
				continue;
			cur_pre_on_state = atomic_read(&g_connv3_ctx.pre_pwr_state);
			pr_info("[pre_pwr_on] [%s] pre-callback is not back", connv3_drv_thread_name[drv_type]);
			osal_thread_show_stack(&drv_inst->msg_ctx.thread);
		}

		/* Set VANT18 when second radio WiFi is on */
		connv3_core_wake_lock_get();
		ret = connv3_hw_pwr_on(opfunc_get_current_status(), drv_type);
		connv3_core_wake_lock_put();

		if (ret) {
			pr_err("Connv3 second radio power on fail. drv(%d) ret=(%d)\n", drv_type, ret);
			osal_unlock_sleepable_lock(&ctx->core_lock);
			return ret;
		}
	}

	g_connv3_ctx.drv_inst[drv_type].drv_status = DRV_STS_PRE_POWER_ON;

	dump_curr_status("Connv3 Pwr On");
	osal_unlock_sleepable_lock(&ctx->core_lock);

	return 0;
}

static int opfunc_power_on(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];

	return opfunc_power_on_internal(drv_type);
}

static int opfunc_power_on_done(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret;

	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_err("abnormal Fun(%d)\n", drv_type);
		return -EINVAL;
	}

	/* Check abnormal state */
	if (g_connv3_ctx.drv_inst[drv_type].drv_status != DRV_STS_PRE_POWER_ON) {
		pr_err("func(%d) status[0x%x] abnormal\n", drv_type,
				g_connv3_ctx.drv_inst[drv_type].drv_status);
		return -EINVAL;
	}

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] core_lock fail!!", __func__);
		return ret;
	}

	pr_info("[%s] type=[%d]", __func__, drv_type);

	/* GPIO control */
	ret = connv3_hw_pwr_on_done(drv_type);
	if (ret) {
		pr_err("[%s] fail, ret=%d", __func__, ret);
	} else {
		g_connv3_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_ON;

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
		/* notification */
		if (drv_type == CONNV3_DRV_TYPE_BT)
			connectivity_export_conap_scp_state_change(conn_bt_on);
		else if (drv_type == CONNV3_DRV_TYPE_WIFI)
			connectivity_export_conap_scp_state_change(conn_wifi_on);
#endif
	}
	dump_curr_status("Connv3 Pwr done");

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return 0;
}

static int opfunc_power_off_internal(unsigned int drv_type)
{
	int i, ret;
	bool try_power_off = true;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	unsigned int curr_status = opfunc_get_current_status();
	unsigned int pmic_state = 0;

	/* Check abnormal type */
	/* Special case: use CONNV3_DRV_TYPE_MAX for force off (turn off pmic_en directly */
	if (drv_type > CONNV3_DRV_TYPE_MAX) {
		pr_err("abnormal Fun(%d)\n", drv_type);
		return -EINVAL;
	}

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("core_lock fail!!");
		return ret;
	}

	if (drv_type == CONNV3_DRV_TYPE_MAX) {
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++)
			g_connv3_ctx.drv_inst[i].drv_status = DRV_STS_POWER_OFF;
		curr_status = 0;
	} else {
		/* Check abnormal state */
		if ((g_connv3_ctx.drv_inst[drv_type].drv_status < DRV_STS_POWER_OFF)
		    || (g_connv3_ctx.drv_inst[drv_type].drv_status >= DRV_STS_MAX)) {
			pr_err("func(%d) status[0x%x] abnormal\n", drv_type,
			g_connv3_ctx.drv_inst[drv_type].drv_status);
			osal_unlock_sleepable_lock(&ctx->core_lock);
			return -2;
		}

		/* check if func already off */
		if (g_connv3_ctx.drv_inst[drv_type].drv_status
					== DRV_STS_POWER_OFF) {
			pr_warn("func(%d) already off\n", drv_type);
			osal_unlock_sleepable_lock(&ctx->core_lock);
			return 0;
		}

		g_connv3_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_OFF;

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
		/* notification */
		if (drv_type == CONNV3_DRV_TYPE_BT)
			connectivity_export_conap_scp_state_change(conn_bt_off);
		else if (drv_type == CONNV3_DRV_TYPE_WIFI)
			connectivity_export_conap_scp_state_change(conn_wifi_off);
#endif
	}
	/* is there subsys on ? */
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++)
		if (g_connv3_ctx.drv_inst[i].drv_status == DRV_STS_POWER_ON ||
		    g_connv3_ctx.drv_inst[i].drv_status == DRV_STS_PRE_POWER_ON)
			try_power_off = false;

	connv3_core_wake_lock_get();
	ret = connv3_hw_pwr_off(curr_status, drv_type, &pmic_state);
	/* Notify subsys driver that pmic has been shutdown */
	if (pmic_state == 1)
		opfunc_power_down_notify();
	connv3_core_wake_lock_put();

	if (ret) {
		pr_err("Connv3 power off fail. drv(%d) ret=(%d)\n",
				drv_type, ret);
		osal_unlock_sleepable_lock(&ctx->core_lock);
		return -3;
	}

	if (try_power_off)
		g_connv3_ctx.core_status = DRV_STS_POWER_OFF;

	dump_curr_status("Connv3 Pwr Off");

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return 0;
}

static int opfunc_power_off(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];

	return opfunc_power_off_internal(drv_type);
}

static const char* __chip_rst_get_type_name(unsigned int type)
{
	static const char *rst_type_string[3] = {
		[CONNV3_CHIP_RST_TYPE_LEGACY_MODE] = "Legacy",
		[CONNV3_CHIP_RST_TYPE_PMIC_FAULT_B] = "PMIC_FAULTB",
		[CONNV3_CHIP_RST_TYPE_DFD_DUMP] = "DFD_DUMP",
	};

	if (type > CONNV3_CHIP_RST_TYPE_DFD_DUMP)
		return "UNKNOWN";

	return rst_type_string[type];
}

static int opfunc_chip_rst(struct msg_op_data *op)
{
	int i, ret, cur_rst_state;
	struct subsys_drv_inst *drv_inst;
	unsigned int drv_pwr_state[CONNV3_DRV_TYPE_MAX];
	const unsigned int subdrv_all_done = (0x1 << CONNV3_DRV_TYPE_MAX) - 1;
	struct timespec64 rst_begin, pre_begin, pre_end, post_reset_end, reset_end, done_end;
	unsigned int rst_type_support;
	enum connv3_reset_source rst_source;
	enum connv3_drv_type trg_drv;
	/* 0: legacy mode (default)
	 * 1: PMIC falut_b
	 * 2: DFD dump (CONN_RST or PMIC_IRQB)
	 */
	unsigned int rst_type = CONNV3_CHIP_RST_TYPE_LEGACY_MODE;
	bool need_pmic_toggle = true;
	enum connv3_drv_status bt_status = g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT].drv_status;
	enum connv3_drv_status wifi_status = g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI].drv_status;

	if (g_connv3_ctx.core_status == DRV_STS_POWER_OFF) {
		pr_info("No subsys on, just return");
		_connv3_core_update_rst_status(CHIP_RST_NONE);
		return 0;
	}

	trg_drv = op->op_data[0];
	rst_source = op->op_data[1];
	osal_gettimeofday(&rst_begin);

	atomic_set(&g_connv3_ctx.rst_state, 0);
	sema_init(&g_connv3_ctx.rst_sema, 1);

	rst_type_support = connv3_hw_get_reset_type_support();
	/* 1: support POR_RST */
	if (rst_type_support == 1) {
		if (rst_source == CONNV3_CHIP_RST_SOURCE_NORMAL) {
			rst_type = CONNV3_CHIP_RST_TYPE_DFD_DUMP;
			need_pmic_toggle = false;
		} else if (rst_source == CONNV3_CHIP_RST_SOURCE_PMIC_IRQ_B) {
			/* GO DFD flow */
			rst_type = CONNV3_CHIP_RST_TYPE_DFD_DUMP;
		} else if (rst_source == CONNV3_CHIP_RST_SOURCE_PMIC_FAULT_B) {
			/* GO legacy reset */
			rst_type = CONNV3_CHIP_RST_TYPE_PMIC_FAULT_B;
		}
	}

	if (rst_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
		_connv3_core_update_rst_status(CHIP_RST_DFD_SETUP);
		/* PU DFD_EN pin */
		ret = connv3_hw_dfd_trigger(true);
		if (ret)
			pr_notice("[%s] connv3_hw_dfd_en error, ret = %d\n", __func__, ret);

		_connv3_core_update_rst_status(CHIP_RST_DFD_PRE_DUMP);
		/* Call conn_scp API to do DFD dump and wait */
	#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
		ret = connectivity_export_conap_scp_trigger_dfd_cmd(
			((bt_status > 0? CONAP_SCP_DFD_DRV_BT: 0) | (wifi_status > 0? CONAP_SCP_DFD_DRV_WF : 0)),
			0, 0);
		pr_info("[chip_rst] dfd dump done, ret = %d\n", ret);
	#else
		pr_warn("[chip_rst] not internal, don't support DFD dump\n");
	#endif
	}

	osal_gettimeofday(&pre_begin);
	_connv3_core_update_rst_status(CHIP_RST_PRE_CB);

	/* pre */
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		drv_inst = &g_connv3_ctx.drv_inst[i];
		drv_pwr_state[i] = drv_inst->drv_status;
		pr_info("subsys %d is %d", i, drv_inst->drv_status);
		ret = msg_thread_send_4(&drv_inst->msg_ctx,
				CONNV3_SUBDRV_OPID_PRE_RESET, i, trg_drv, rst_source, rst_type);
	}

	pr_info("[chip_rst] pre vvvvvvvvvvvvv");
	while (atomic_read(&g_connv3_ctx.rst_state) != subdrv_all_done) {
		ret = down_timeout(&g_connv3_ctx.rst_sema, msecs_to_jiffies(CONNV3_RESET_TIMEOUT));
		if (ret == 0)
			continue;
		cur_rst_state = atomic_read(&g_connv3_ctx.rst_state);
		pr_info("cur_rst state =[%d]", cur_rst_state);
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			if ((cur_rst_state & (0x1 << i)) == 0) {
				pr_info("[chip_rst] [%s] pre-callback is not back", connv3_drv_thread_name[i]);
				drv_inst = &g_connv3_ctx.drv_inst[i];
				osal_thread_show_stack(&drv_inst->msg_ctx.thread);
			}
		}
	}

	osal_gettimeofday(&pre_end);

	if (rst_type == CONNV3_CHIP_RST_TYPE_DFD_DUMP) {
	#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
		/* Clean DFD dump */
		ret = connectivity_export_conap_scp_clr_dfd_buffer();
		pr_info("[%s] clean DFD dump done, ret = %d\n", __func__, ret);
	#else
		pr_warn("[chip_rst] Not internal platform\n");
	#endif

		/* Reset flow
		 * 1. PD DFD_EN pin
		 * 2. Call conn_scp to turn off uart function
		 * 3. CONN_RST toggle
		 */
		/* 1. PD DFD_EN pin */
		ret = connv3_hw_dfd_trigger(false);
		if (ret)
			pr_notice("[%s] connv3_hw_dfd_trigger(0) error, ret = %d\n", __func__, ret);

		/* 2. Call conn_scp to turn off uart function */
	#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
		if (g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT].drv_status == DRV_STS_POWER_ON)
			connectivity_export_conap_scp_state_change(conn_bt_off);
		if (g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI].drv_status == DRV_STS_POWER_ON)
			connectivity_export_conap_scp_state_change(conn_wifi_off);
	#endif
		/* 3. CONN_RST toggle */
		ret = connv3_hw_pwr_rst();
		if (ret)
			pr_notice("[%s] connv3_hw_pwr_rst fail, ret = %d\n", __func__, ret);

		/* post_rst_on_cb */
		atomic_set(&g_connv3_ctx.rst_state, 0);
		sema_init(&g_connv3_ctx.rst_sema, 1);
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			drv_inst = &g_connv3_ctx.drv_inst[i];
			ret = msg_thread_send_2(&drv_inst->msg_ctx,
				CONNV3_SUBDRV_OPID_POST_RST_ON, i, need_pmic_toggle);
		}
		while (atomic_read(&g_connv3_ctx.rst_state) != subdrv_all_done) {
			ret = down_timeout(&g_connv3_ctx.rst_sema, msecs_to_jiffies(CONNV3_RESET_TIMEOUT));
			if (ret == 0)
				continue;
			cur_rst_state = atomic_read(&g_connv3_ctx.rst_state);
			for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
				if ((cur_rst_state & (0x1 << i)) == 0) {
					pr_info("[chip_rst] [%s] post_reset_on callback is not back", connv3_drv_thread_name[i]);
					drv_inst = &g_connv3_ctx.drv_inst[i];
					osal_thread_show_stack(&drv_inst->msg_ctx.thread);
				}
			}
		}
	}

	osal_gettimeofday(&post_reset_end);

	if (need_pmic_toggle) {
		_connv3_core_update_rst_status(CHIP_RST_RESET);
		pr_info("[chip_rst] reset ++++++++++++");
		/*******************************************************/
		/* reset */
		/* call consys_hw */
		/*******************************************************/
		/* Special power-off function, turn off connsys directly */
		ret = opfunc_power_off_internal(CONNV3_DRV_TYPE_MAX);
		pr_info("Force connv3 power off, ret=%d. Status should be off. Status=%d\n",
			ret, g_connv3_ctx.core_status);
	} else {
		/* Don't toggle pmic, need to update driver status one-by-one */
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			if (g_connv3_ctx.drv_inst[i].drv_status != DRV_STS_POWER_OFF) {
				g_connv3_ctx.drv_inst[i].drv_status = DRV_STS_PRE_POWER_ON;
			}
		}
		/* core_status DRV_STS_RESET means that POR_RST trigger and pmic_en is on
		 * need to patch download again
		 */
		g_connv3_ctx.core_status = DRV_STS_RESET;
		dump_curr_status("PMIC_POR_RST");
	}

	osal_gettimeofday(&reset_end);

	_connv3_core_update_rst_status(CHIP_RST_POST_CB);

	/* post */
	atomic_set(&g_connv3_ctx.rst_state, 0);
	sema_init(&g_connv3_ctx.rst_sema, 1);
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		drv_inst = &g_connv3_ctx.drv_inst[i];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				CONNV3_SUBDRV_OPID_POST_RESET, i);
	}

	while (atomic_read(&g_connv3_ctx.rst_state) != subdrv_all_done) {
		ret = down_timeout(&g_connv3_ctx.rst_sema, msecs_to_jiffies(CONNV3_RESET_TIMEOUT));
		if (ret == 0)
			continue;
		cur_rst_state = atomic_read(&g_connv3_ctx.rst_state);
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			if ((cur_rst_state & (0x1 << i)) == 0) {
				pr_info("[chip_rst] [%s] post-callback is not back", connv3_drv_thread_name[i]);
				drv_inst = &g_connv3_ctx.drv_inst[i];
				osal_thread_show_stack(&drv_inst->msg_ctx.thread);
			}
		}
	}
	pr_info("[chip_rst] post ^^^^^^^^^^^^^^");

	reset_chip_rst_trg_data();
	_connv3_core_update_rst_status(CHIP_RST_NONE);
	osal_gettimeofday(&done_end);

	pr_info("[chip_rst][%s] summary total=[%lu] pre-dfd=[%lu] pre-cb=[%lu] post-dfd=[%lu] off=[%lu] post-cb=[%lu]",
				__chip_rst_get_type_name(rst_type),
				timespec64_to_ms(&rst_begin, &done_end),      /* total */
				timespec64_to_ms(&rst_begin, &pre_begin),     /* pre-dfd */
				timespec64_to_ms(&pre_begin, &pre_end),       /* pre-cb */
				timespec64_to_ms(&pre_end, &post_reset_end),  /* post-dfd */
				timespec64_to_ms(&post_reset_end, &reset_end),/* power off */
				timespec64_to_ms(&reset_end, &done_end));     /* post-cb */

	return 0;
}

static int pre_cal_drv_onoff_internal(
	enum connv3_drv_type drv_type,
	bool on, unsigned int *pmic_state)
{
	int ret;
	unsigned int status;

	ret = osal_lock_sleepable_lock(&g_connv3_ctx.core_lock);
	if (ret) {
		pr_notice("[%s][%d][%d] core_lock fail!!, ret = %d",
			__func__, drv_type, on, ret);
		return ret;
	}
	if (drv_type == CONNV3_DRV_TYPE_MAX) {
		if (on) {
			pr_notice("[%s] CONNV3_DRV_TYPE_MAX only for power off\n", __func__);
			goto PRE_CAL_ONOFF_END;
		} else {
			connv3_core_wake_lock_get();
			ret = connv3_hw_pwr_off(0, CONNV3_DRV_TYPE_MAX, pmic_state);
			connv3_core_wake_lock_put();
			if (ret)
				pr_notice("[%s] CONNV3_DRV_TYPE_MAX off fail, ret = %d\n",
					__func__, ret);
			g_connv3_ctx.core_status = DRV_STS_POWER_OFF;
		}
	} else {
		if (on) {
			g_connv3_ctx.core_status = DRV_STS_POWER_ON;
			if (g_connv3_ctx.drv_inst[drv_type].drv_status == DRV_STS_POWER_ON) {
				pr_notice("[%s] drv(%d) already on", __func__, drv_type);
				goto PRE_CAL_ONOFF_END;
			}
			connv3_core_wake_lock_get();
			ret = connv3_hw_pwr_on(opfunc_get_current_status(), drv_type);
			connv3_core_wake_lock_put();
			g_connv3_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_ON;
		} else {
			if (g_connv3_ctx.drv_inst[drv_type].drv_status == DRV_STS_POWER_OFF) {
				pr_notice("[%s] drv(%d) already off", __func__, drv_type);
				goto PRE_CAL_ONOFF_END;
			}
			connv3_core_wake_lock_get();
			ret = connv3_hw_pwr_off(opfunc_get_current_status(), drv_type, pmic_state);
			connv3_core_wake_lock_put();
			g_connv3_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_OFF;
			status = opfunc_get_current_status();
			if (status == 0)
				g_connv3_ctx.core_status = DRV_STS_POWER_OFF;
		}
	}

PRE_CAL_ONOFF_END:
	osal_unlock_sleepable_lock(&g_connv3_ctx.core_lock);
	return 0;
}

static int opfunc_pre_cal_efuse_on(void)
{
	int pre_cal_done_state = (0x1 << CONNV3_DRV_TYPE_WIFI);
	int ret = 0;
	struct timespec64 efuse_begin, efuse_pre_on, efuse_on, efuse_end;
	struct subsys_drv_inst *drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];

	/* force power off */
	pr_info("[pre_cal][efuse_on] force power off");
	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_MAX, false, NULL);
	if (ret)
		pr_notice("[%s] force Connv3 power off fail, ret(%d)", __func__, ret);

	/* efuse pwoer on */
	atomic_set(&g_connv3_ctx.pre_cal_state, 0);
	sema_init(&g_connv3_ctx.pre_cal_sema, 1);

	osal_gettimeofday(&efuse_begin);
	pr_info("[pre_cal][efuse_on] wifi on");
	ret = msg_thread_send_1(&drv_inst->msg_ctx,
		CONNV3_SUBDRV_OPID_CAL_PRE_ON, CONNV3_DRV_TYPE_WIFI);
	if (ret)
		pr_notice("wifi pre_on for efuse fail, ret=%d", ret);

	while (atomic_read(&g_connv3_ctx.pre_cal_state) != pre_cal_done_state) {
		ret = down_timeout(&g_connv3_ctx.pre_cal_sema, msecs_to_jiffies(CONNV3_PRE_CAL_TIMEOUT));
		if (ret == 0)
			continue;
		pr_info("[pre_cal][efuse_on] wifi pre_on is not back");
	}
	pr_info("[pre_cal][efuse_on] wifi on done");
	osal_gettimeofday(&efuse_pre_on);

	/* Do HW on directly. Don't call core function. */
	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_WIFI, true, NULL);
	if (ret) {
		pr_notice("[%s] Connv3 power on fail, ret=(%d)", __func__, ret);
		ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_PRE_CAL_FAIL, 0, CONNV3_DRV_TYPE_WIFI);
		return -1;
	}

	atomic_set(&g_connv3_ctx.pre_cal_state, 0);
	sema_init(&g_connv3_ctx.pre_cal_sema, 1);

	ret = msg_thread_send_1(&drv_inst->msg_ctx,
		CONNV3_SUBDRV_OPID_CAL_EFUSE_ON, CONNV3_DRV_TYPE_WIFI);
	if (ret)
		pr_notice("wifi efuse on fail, ret=%d", ret);
	while (atomic_read(&g_connv3_ctx.pre_cal_state) != pre_cal_done_state) {
		ret = down_timeout(&g_connv3_ctx.pre_cal_sema, msecs_to_jiffies(CONNV3_PRE_CAL_TIMEOUT));
		if (ret == 0)
			continue;
		pr_info("[pre_cal][efuse_on] wifi efuse_on_cb is not back");
	}
	pr_info("[pre_cal][efuse_on] efuse_on_cb done");
	osal_gettimeofday(&efuse_on);

	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_WIFI, false, NULL);
	if (ret)
		pr_notice("[efuse_on] power off wifi fail, ret = %d", ret);

	/* use CONNV3_DRV_TYPE_MAX to trigger PMIC en off */
	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_MAX, false, NULL);
	if (ret)
		pr_notice("[%s] Connv3 power off fail, ret(%d)", __func__, ret);
	if (opfunc_get_current_status() != 0)
		pr_notice("[pre_cal][efuse_on] all radio should be off, but get 0x%x",
			opfunc_get_current_status());

	osal_gettimeofday(&efuse_end);

	pr_info("[efuse_on] summary pre_on=[%lu] pwr=[%lu] pwr off=[%lu]",
		timespec64_to_ms(&efuse_begin, &efuse_pre_on),
		timespec64_to_ms(&efuse_pre_on, &efuse_on),
		timespec64_to_ms(&efuse_on, &efuse_end));

	return 0;
}

static int opfunc_pre_cal(struct msg_op_data *op)
{
#define CAL_DRV_COUNT 2
#define PRE_CAL_PLATFORM_CHECK_TIMEOUT_MS	(2 * 60 * 1000) // 2 minutes
#define PRE_CAL_PLATFOEM_CHECK_DURATION		1000 // 1 sec
	int cal_drvs[CAL_DRV_COUNT] = {CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI};
	int i, ret, cur_state;
	int bt_cal_ret, wf_cal_ret;
	struct subsys_drv_inst *drv_inst;
	int pre_cal_done_state = (0x1 << CONNV3_DRV_TYPE_BT) | (0x1 << CONNV3_DRV_TYPE_WIFI);
	struct timespec64 plt_check, efuse_on_start, begin, pwr_on_begin, bt_cal_begin, wf_cal_begin, end;
	int pmic_state;
	unsigned int check_total_time = 0;

	/* Check BT/WIFI status again */
	ret = osal_lock_sleepable_lock(&g_connv3_ctx.core_lock);
	if (ret) {
		pr_err("[%s] core_lock fail!!", __func__);
		return ret;
	}
	/* check if func already on */
	for (i = 0; i < CAL_DRV_COUNT; i++) {
		if (g_connv3_ctx.drv_inst[cal_drvs[i]].drv_status == DRV_STS_POWER_ON) {
			pr_warn("[%s] %s already on\n", __func__, connv3_drv_name[cal_drvs[i]]);
			osal_unlock_sleepable_lock(&g_connv3_ctx.core_lock);
			return 0;
		}
	}
	osal_unlock_sleepable_lock(&g_connv3_ctx.core_lock);

	osal_gettimeofday(&plt_check);
	/* Check platform status */
	while (check_total_time <= PRE_CAL_PLATFORM_CHECK_TIMEOUT_MS) {
		ret = connv3_hw_check_status();
		if (ret == CONNV3_PLT_STATE_READY) {
			break;
		}
		check_total_time += PRE_CAL_PLATFOEM_CHECK_DURATION;
		msleep(PRE_CAL_PLATFOEM_CHECK_DURATION);
	}
	if (ret != CONNV3_PLT_STATE_READY) {
		pr_notice("[%s] pre-cal check platform ready fail, ret: %d\n", __func__, ret);
		return -1;
	}

	osal_gettimeofday(&efuse_on_start);
	ret = opfunc_pre_cal_efuse_on();
	if (ret) {
		pr_notice("[%s] break pre-cal flow and return, ret = %d", __func__, ret);
		return -1;
	}

	osal_gettimeofday(&begin);
	/* power on subsys */
	atomic_set(&g_connv3_ctx.pre_cal_state, 0);
	sema_init(&g_connv3_ctx.pre_cal_sema, 1);

	for (i = 0; i < CAL_DRV_COUNT; i++) {
		drv_inst = &g_connv3_ctx.drv_inst[cal_drvs[i]];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_CAL_PRE_ON, cal_drvs[i]);
		if (ret)
			pr_warn("driver [%d] cal pre on fail\n", cal_drvs[i]);
	}
	while (atomic_read(&g_connv3_ctx.pre_cal_state) != pre_cal_done_state) {
		ret = down_timeout(&g_connv3_ctx.pre_cal_sema, msecs_to_jiffies(CONNV3_PRE_CAL_TIMEOUT));
		if (ret == 0)
			continue;
		cur_state = atomic_read(&g_connv3_ctx.pre_cal_state);
		pr_info("[pre_cal][pre_on] cur state =[%d]", cur_state);
		if ((cur_state & (0x1 << CONNV3_DRV_TYPE_BT)) == 0) {
			pr_info("[pre_cal][pre_on] BT pre_on_cb is not back");
			drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
			osal_thread_show_stack(&drv_inst->msg_ctx.thread);
		}
		if ((cur_state & (0x1 << CONNV3_DRV_TYPE_WIFI)) == 0) {
			pr_info("[pre_cal][pre_on] WIFI pre_on_cb is not back");
			drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
			osal_thread_show_stack(&drv_inst->msg_ctx.thread);
		}
	}
	pr_info("[pre_cal] >>>>>>> pre on DONE!!");
	osal_gettimeofday(&pwr_on_begin);

	/* Common part POS */
	bt_cal_ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_BT, true, NULL);
	wf_cal_ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_WIFI, true, NULL);
	/* Pre-cal fail, inform subsys driver and rollback to power off state */
	if (bt_cal_ret || wf_cal_ret) {
		pr_notice("[%s] Connv3 power on fail. ret=(%d, %d)\n",
			__func__, bt_cal_ret, wf_cal_ret);

		drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
		ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_PRE_CAL_FAIL, 0, CONNV3_DRV_TYPE_BT);
		bt_cal_ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_BT, false, NULL);
		pr_notice("[%s] inform bt pre-cal fail, ret=(%d, %d)\n",
			__func__, ret, bt_cal_ret);

		drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
		ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_PRE_CAL_FAIL, 0, CONNV3_DRV_TYPE_WIFI);
		wf_cal_ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_WIFI, false, NULL);
		pr_notice("[%s] inform wifi pre-cal fail, ret=(%d, %d)\n",
			__func__, ret, wf_cal_ret);
		return -1;
	}

	/* Subsys power on */
	atomic_set(&g_connv3_ctx.pre_cal_state, 0);
	sema_init(&g_connv3_ctx.pre_cal_sema, 1);
	for (i = 0; i < CAL_DRV_COUNT; i++) {
		drv_inst = &g_connv3_ctx.drv_inst[cal_drvs[i]];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				CONNV3_SUBDRV_OPID_CAL_PWR_ON, cal_drvs[i]);
		if (ret)
			pr_warn("driver [%d] power on fail\n", cal_drvs[i]);
	}

	while (atomic_read(&g_connv3_ctx.pre_cal_state) != pre_cal_done_state) {
		ret = down_timeout(&g_connv3_ctx.pre_cal_sema, msecs_to_jiffies(CONNV3_PRE_CAL_TIMEOUT));
		if (ret == 0)
			continue;
		cur_state = atomic_read(&g_connv3_ctx.pre_cal_state);
		pr_info("[pre_cal] cur state =[%d]", cur_state);
		if ((cur_state & (0x1 << CONNV3_DRV_TYPE_BT)) == 0) {
			pr_info("[pre_cal] BT pwr_on callback is not back");
			drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
			osal_thread_show_stack(&drv_inst->msg_ctx.thread);
		}
		if ((cur_state & (0x1 << CONNV3_DRV_TYPE_WIFI)) == 0) {
			pr_info("[pre_cal] WIFI pwr_on callback is not back");
			drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
			osal_thread_show_stack(&drv_inst->msg_ctx.thread);
		}
	}
	pr_info("[pre_cal] >>>>>>> power on DONE!!");

	osal_gettimeofday(&bt_cal_begin);

	/* Do Calibration */
	drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
	bt_cal_ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_CAL_DO_CAL, 0, CONNV3_DRV_TYPE_BT);

	pr_info("[pre_cal] driver [%s] calibration %s, ret=[%d]\n", connv3_drv_name[CONNV3_DRV_TYPE_BT],
			(bt_cal_ret == CONNV3_CB_RET_CAL_FAIL) ? "fail" : "success",
			bt_cal_ret);
	pr_info("[pre_cal] >>>>>>>> BT do cal done");

	osal_gettimeofday(&wf_cal_begin);

	drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
	wf_cal_ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			CONNV3_SUBDRV_OPID_CAL_DO_CAL, 0, CONNV3_DRV_TYPE_WIFI);

	pr_info("[pre_cal] driver [%s] calibration %s, ret=[%d]\n", connv3_drv_name[CONNV3_DRV_TYPE_WIFI],
			(wf_cal_ret == CONNV3_CB_RET_CAL_FAIL) ? "fail" : "success",
			wf_cal_ret);
	pr_info(">>>>>>>> WF do cal done");

	/* Power off */
	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_BT, false, NULL);
	if (ret)
		pr_notice("[pre_cal] power off bt fail, ret = %d", ret);
	ret = pre_cal_drv_onoff_internal(CONNV3_DRV_TYPE_WIFI, false, &pmic_state);
	if (ret)
		pr_notice("[pre_cal] power off wifi fail, ret = %d", ret);

	/* Check radio status */
	ret = opfunc_get_current_status();
	if (ret != 0)
		pr_notice("[pre_cal] all radio should be off, but get 0x%x", ret);

	pr_info(">>>>>>>> Power off bt/wifi done");

	osal_gettimeofday(&end);

	pr_info("[pre_cal] summary plt_check=[%lu] efuse_on=[%lu] pre_on=[%lu] pwr=[%lu] bt_cal=[%d][%lu] wf_cal=[%d][%lu]",
			timespec64_to_ms(&plt_check, &efuse_on_start),
			timespec64_to_ms(&efuse_on_start, &begin),
			timespec64_to_ms(&begin, &pwr_on_begin),
			timespec64_to_ms(&pwr_on_begin, &bt_cal_begin),
			bt_cal_ret, timespec64_to_ms(&bt_cal_begin, &wf_cal_begin),
			wf_cal_ret, timespec64_to_ms(&wf_cal_begin, &end));

	if (pmic_state == 1) {
		opfunc_power_down_notify();
	}

	return 0;
}

static int opfunc_pre_cal_prepare(struct msg_op_data *op)
{
	int ret = 0, rst_status, num = 0;
	unsigned long flag;
	struct pre_cal_info *cal_info = &g_connv3_ctx.cal_info;
	struct subsys_drv_inst *bt_drv = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
	struct subsys_drv_inst *wifi_drv = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
	enum pre_cal_status cur_status;

	spin_lock_irqsave(&g_connv3_ctx.infra_lock, flag);

	if (bt_drv->ops_cb.pre_cal_cb.do_cal_cb == NULL ||
		wifi_drv->ops_cb.pre_cal_cb.do_cal_cb == NULL) {
		pr_info("[%s] [pre_cal] [%p][%p]", __func__,
			bt_drv->ops_cb.pre_cal_cb.do_cal_cb,
			wifi_drv->ops_cb.pre_cal_cb.do_cal_cb);
		spin_unlock_irqrestore(&g_connv3_ctx.infra_lock, flag);
		return 0;
	}
	spin_unlock_irqrestore(&g_connv3_ctx.infra_lock, flag);

	spin_lock_irqsave(&g_connv3_ctx.rst_lock, flag);
	rst_status = g_connv3_ctx.rst_status;
	spin_unlock_irqrestore(&g_connv3_ctx.rst_lock, flag);

	if (rst_status > CHIP_RST_NONE) {
		pr_info("rst is ongoing, skip pre_cal");
		return 0;
	}

	/* non-zero means lock got, zero means not */

	while (!ret) {
		ret = osal_trylock_sleepable_lock(&cal_info->pre_cal_lock);
		if (ret == 0) {
			if (num >= 10) {
				/* Another pre-cal should be on progress */
				/* Skip to prevent block core thread */
				pr_notice("[%s] fail to get pre_cal_lock\n", __func__);
				break;
			}
			/* sleep time is short to make sure get lock easier than */
			/* conninfra_core_pre_cal_blocking */
			osal_sleep_ms(10);
			num++;
			continue;
		}

		cur_status = cal_info->status;

		if ((cur_status == PRE_CAL_NOT_INIT || cur_status == PRE_CAL_NEED_RESCHEDULE) &&
			bt_drv->drv_status == DRV_STS_POWER_OFF &&
			wifi_drv->drv_status == DRV_STS_POWER_OFF) {
			cal_info->status = PRE_CAL_SCHEDULED;
			cal_info->caller = op->op_data[0];
			pr_info("[pre_cal] BT&WIFI is off, schedule pre-cal from status=[%d] to new status[%d]\n",
				cur_status, cal_info->status);
			schedule_work(&cal_info->pre_cal_work);
		} else {
			pr_info("[%s] [pre_cal] bt=[%d] wf=[%d] status=[%d]", __func__,
				bt_drv->drv_status, wifi_drv->drv_status, cur_status);
		}
		osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
	}

	return 0;
}

static int opfunc_pre_cal_check(struct msg_op_data *op)
{
	int ret;
	struct pre_cal_info *cal_info = &g_connv3_ctx.cal_info;
	struct subsys_drv_inst *bt_drv = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
	struct subsys_drv_inst *wifi_drv = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
	enum pre_cal_status cur_status;

	/* non-zero means lock got, zero means not */
	ret = osal_trylock_sleepable_lock(&cal_info->pre_cal_lock);
	if (ret) {
		cur_status = cal_info->status;

		pr_info("[%s] [pre_cal] bt=[%d] wf=[%d] status=[%d]", __func__,
			bt_drv->drv_status, wifi_drv->drv_status,
			cur_status);
		if (cur_status == PRE_CAL_DONE &&
			bt_drv->drv_status == DRV_STS_POWER_OFF &&
			wifi_drv->drv_status == DRV_STS_POWER_OFF) {
			pr_info("[pre_cal] reset pre-cal");
			cal_info->status = PRE_CAL_NEED_RESCHEDULE;
		}
		osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
	}
	return 0;
}

static int opfunc_subdrv_pre_reset(struct msg_op_data *op)
{
	int ret, cur_rst_state;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;
	enum connv3_drv_type trg_drv = op->op_data[1];
	enum connv3_reset_source rst_source = op->op_data[2];
	unsigned int rst_type = op->op_data[3];

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.rst_cb.pre_whole_chip_rst) {

		pr_info("[%s][%s] trg_reason=[%s] type=[%d]",
			__func__, connv3_drv_thread_name[drv_type],
			g_connv3_ctx.trg_reason[rst_source], rst_type);
		ret = drv_inst->ops_cb.rst_cb.pre_whole_chip_rst(trg_drv,
					g_connv3_ctx.trg_reason[rst_source],
					rst_type);
		if (ret)
			pr_notice("[%s] fail [%d]", __func__, ret);
	}

	atomic_add(0x1 << drv_type, &g_connv3_ctx.rst_state);
	cur_rst_state = atomic_read(&g_connv3_ctx.rst_state);

	pr_info("[%s] rst_state=[%d]", connv3_drv_thread_name[drv_type], cur_rst_state);

	up(&g_connv3_ctx.rst_sema);
	return 0;
}

static bool __power_dump(
	enum connv3_drv_type drv_type,
	struct subsys_drv_inst *drv_inst,
	enum connv3_pwr_dump_type dump_type,
	char *buf, unsigned int size, bool force_dump)
{
	static bool is_start = false;
	bool cb_ok = false;
	struct connv3_power_dump_cb *pwr_dump_cb;
	struct connv3_cr_cb *cr_cb = &drv_inst->ops_cb.cr_cb;

	pwr_dump_cb = &drv_inst->ops_cb.pwr_dump_cb;
	/* Check dump function first */
	if (pwr_dump_cb != NULL &&
	    pwr_dump_cb->power_dump_start != NULL &&
	    pwr_dump_cb->power_dump_end != NULL) {
		if (cr_cb->read != NULL &&
		    cr_cb->write != NULL &&
		    cr_cb->write_mask != NULL)
			cb_ok = true;
	}

	if (!cb_ok) {
		pr_info("[%s][%s] not support\n", __func__, connv3_drv_name[drv_type]);
		return false;
	}

	if (pwr_dump_cb->power_dump_start(cr_cb->priv_data, force_dump) == 0) {
		if (dump_type == CONNV3_PWR_INFO_DUMP) {
			if (is_start)
				connv3_hw_power_info_dump(drv_type, cr_cb, buf, size);
			else
				pr_notice("[CONNV3] power dump not start\n");
		} else if (dump_type == CONNV3_PWR_INFO_RESET) {
			connv3_hw_power_info_reset(drv_type, cr_cb);
			is_start = true;
		} else if (dump_type == CONNV3_PWR_INFO_DUMP_AND_RESET) {
			if (is_start)
				connv3_hw_power_info_dump(drv_type, cr_cb, buf, size);
			else
				pr_notice("[CONNV3] power dump not start\n");
			connv3_hw_power_info_reset(drv_type, cr_cb);
			is_start = true;
		}
		pwr_dump_cb->power_dump_end(cr_cb->priv_data);
	} else {
		pr_info("[%s][%s] force_dump=%d reject\n", __func__, connv3_drv_name[drv_type], force_dump);
		return false;
	}

	return true;
}

static int opfunc_power_dump_internal(enum connv3_pwr_dump_type dump_type, struct msg_op_data *op)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret;
	bool dump_done = false;
	struct subsys_drv_inst *drv_inst;
        bool force_dump = (bool)op->op_data[0];
	char *buf = (char *)op->op_data[1];
	unsigned int buf_sz = op->op_data[2];

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_notice("core_lock fail!!");
		return ret;
	}
	/* Check BT first */
	drv_inst = &ctx->drv_inst[CONNV3_DRV_TYPE_BT];
	if (drv_inst->drv_status == DRV_STS_POWER_ON) {
		/* if BT is on, check bt first */
		dump_done = __power_dump(
			CONNV3_DRV_TYPE_BT, drv_inst, dump_type, buf, buf_sz, force_dump);
	}

	drv_inst = &ctx->drv_inst[CONNV3_DRV_TYPE_WIFI];
	if (!dump_done && drv_inst->drv_status == DRV_STS_POWER_ON) {
		dump_done = __power_dump(
			CONNV3_DRV_TYPE_WIFI, drv_inst, dump_type, buf, buf_sz, force_dump);
	}

	osal_unlock_sleepable_lock(&ctx->core_lock);
	return 0;
}

static int opfunc_reset_power_state(struct msg_op_data *op)
{
	return opfunc_power_dump_internal(CONNV3_PWR_INFO_RESET, op);
}

static int opfunc_dump_power_state(struct msg_op_data *op)
{
	return opfunc_power_dump_internal(CONNV3_PWR_INFO_DUMP, op);
}

static int opfunc_reset_and_dump_power_state(struct msg_op_data *op)
{
	return opfunc_power_dump_internal(CONNV3_PWR_INFO_DUMP_AND_RESET, op);
}

static int opfunc_subdrv_post_reset_on(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;
	unsigned int type = op->op_data[1];

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.rst_cb.post_reset_on) {
		ret = drv_inst->ops_cb.rst_cb.post_reset_on(type);
		if (ret)
			pr_notice("[%s][%s][type=%d] fail, ret=%d\n",
				__func__, connv3_drv_name[drv_type], type, ret);
	} else
		pr_notice("[%s][%s][type=%d] not support\n",
			__func__, connv3_drv_name[drv_type], type);

	atomic_add(0x1 << drv_type, &g_connv3_ctx.rst_state);
	up(&g_connv3_ctx.rst_sema);

	return 0;
}


static int opfunc_subdrv_post_reset(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.rst_cb.post_whole_chip_rst) {
		ret = drv_inst->ops_cb.rst_cb.post_whole_chip_rst();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	atomic_add(0x1 << drv_type, &g_connv3_ctx.rst_state);
	up(&g_connv3_ctx.rst_sema);
	return 0;
}

static int opfunc_subdrv_cal_pre_on(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.pre_cal_cb.pre_on_cb) {
		ret = drv_inst->ops_cb.pre_cal_cb.pre_on_cb();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	atomic_add(0x1 << drv_type, &g_connv3_ctx.pre_cal_state);
	up(&g_connv3_ctx.pre_cal_sema);

	pr_info("[pre_cal][%s] [%s] DONE", __func__, connv3_drv_thread_name[drv_type]);
	return 0;
}

static int opfunc_subdrv_cal_pwr_on(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.pre_cal_cb.pwr_on_cb) {
		ret = drv_inst->ops_cb.pre_cal_cb.pwr_on_cb();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	atomic_add(0x1 << drv_type, &g_connv3_ctx.pre_cal_state);
	up(&g_connv3_ctx.pre_cal_sema);

	pr_info("[pre_cal][%s] [%s] DONE", __func__, connv3_drv_thread_name[drv_type]);
	return 0;
}

static int opfunc_subdrv_cal_do_cal(struct msg_op_data *op)
{
	int ret = 0;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.pre_cal_cb.do_cal_cb) {
		ret = drv_inst->ops_cb.pre_cal_cb.do_cal_cb();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	pr_info("[pre_cal][%s] [%s] DONE", __func__, connv3_drv_thread_name[drv_type]);
	return ret;
}

static int opfunc_subdrv_pre_cal_fail(struct msg_op_data *op)
{
	int ret = 0;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.pre_cal_cb.pre_cal_error) {
		ret = drv_inst->ops_cb.pre_cal_cb.pre_cal_error();
		if (ret)
			pr_notice("[%s] fail [%d]", __func__, ret);
	}

	pr_info("[pre_cal][%s] [%s] DONE", __func__, connv3_drv_thread_name[drv_type]);
	return ret;
}


int opfunc_subdrv_pre_pwr_on(struct msg_op_data *op)
{
	int ret, cur_rst_state;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;


	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.pwr_on_cb.pre_power_on) {

		ret = drv_inst->ops_cb.pwr_on_cb.pre_power_on();
		if (ret)
			pr_err("[%s] fail [%d]", __func__, ret);
	}

	atomic_add(0x1 << drv_type, &g_connv3_ctx.pre_pwr_state);
	cur_rst_state = atomic_read(&g_connv3_ctx.pre_pwr_state);

	pr_info("[%s] [%s] pwr_state_state=[%d]", __func__, connv3_drv_thread_name[drv_type], cur_rst_state);

	up(&g_connv3_ctx.pre_pwr_sema);

	return 0;
}

int opfunc_subdrv_pwr_on_notify(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);

	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid type=[%d]", __func__, drv_type);
		return -EINVAL;
	}

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.pwr_on_cb.power_on_notify)
		(drv_inst->ops_cb.pwr_on_cb.power_on_notify)();

	return 0;
}


int opfunc_ext_32k_on(struct msg_op_data *op)
{
	int ret;

	ret = connv3_hw_ext_32k_onoff(true);
	return 0;
}

static int opfunc_subdrv_efuse_on(struct msg_op_data *op) {
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);
	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid type=[%d]", __func__, drv_type);
		return -EINVAL;
	}

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.pre_cal_cb.efuse_on_cb)
		(drv_inst->ops_cb.pre_cal_cb.efuse_on_cb)();
	atomic_add(0x1 << drv_type, &g_connv3_ctx.pre_cal_state);
	up(&g_connv3_ctx.pre_cal_sema);

	pr_info("[pre_cal][%s] [%s] DONE", __func__, connv3_drv_thread_name[drv_type]);

	return 0;
}

int opfunc_subdrv_pwr_down_notify(struct msg_op_data *op) {
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;
	int ret;

	if (drv_type >= CONNV3_DRV_TYPE_MAX) {
		pr_notice("[%s] invalid type=[%d]", __func__, drv_type);
		return -EINVAL;
	}

	ret = osal_lock_sleepable_lock(&g_connv3_ctx.subsys_op_lock);
	if (ret) {
		pr_notice("[%s][drv=%s] get subsys_op_lock fail, ret = %d\n",
			__func__, connv3_drv_thread_name[drv_type], ret);
		return -1;
	}

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.pwr_on_cb.chip_power_down_notify) {
		pr_info("[%s] drv=[%s]", __func__, connv3_drv_thread_name[drv_type]);
		(drv_inst->ops_cb.pwr_on_cb.chip_power_down_notify)(0);
	}

	osal_unlock_sleepable_lock(&g_connv3_ctx.subsys_op_lock);
	return 0;
}

static int opfunc_subdrv_fmd_pre_cb(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.fmd_cb.pre_fmd_cb) {
		ret = drv_inst->ops_cb.fmd_cb.pre_fmd_cb();
		if (ret)
			pr_notice("[%s][%s] fail, ret=%d\n",
				__func__, connv3_drv_name[drv_type], ret);
	} else
		pr_notice("[%s][%s] not support\n",
			__func__, connv3_drv_name[drv_type]);

	atomic_add(0x1 << drv_type, &g_connv3_ctx.fmd_state);
	up(&g_connv3_ctx.fmd_sema);
	return 0;
}

static int opfunc_subdrv_fmd_post_cb(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	drv_inst = &g_connv3_ctx.drv_inst[drv_type];
	if (drv_inst->ops_cb.fmd_cb.post_fmd_cb) {
		ret = drv_inst->ops_cb.fmd_cb.post_fmd_cb();
		if (ret)
			pr_notice("[%s][%s] fail, ret=%d\n",
				__func__, connv3_drv_name[drv_type], ret);
	} else
		pr_notice("[%s][%s] not support\n",
			__func__, connv3_drv_name[drv_type]);

	atomic_add(0x1 << drv_type, &g_connv3_ctx.fmd_state);
	up(&g_connv3_ctx.fmd_sema);
	return 0;
}

int opfunc_enter_fmd_mode(struct msg_op_data *op)
{
	int ret, fmd_ready_state;
	struct subsys_drv_inst *drv_inst;
	struct timespec64 fmd_begin, pre_wifi_end, pre_bt_end, pwr_off_end, post_cb_end;
	int i, cur_rst_state;
	const unsigned int subdrv_all_done = (0x1 << CONNV3_DRV_TYPE_MAX) - 1;

	if (g_connv3_ctx.core_status == DRV_STS_POWER_OFF) {
		pr_info("No subsys on, just return");
		_connv3_core_update_rst_status(CHIP_RST_NONE);
		return 0;
	}

	ret =  osal_lock_sleepable_lock(&g_connv3_ctx.core_lock);
	if (ret) {
 		pr_notice("[FMD] core_lock fail!!\n");
 		return ret;
 	}

	atomic_set(&g_connv3_ctx.fmd_mode_trigger, 1);
	osal_gettimeofday(&fmd_begin);
	pr_info("[FMD] pre_cb - wifi\n");
	atomic_set(&g_connv3_ctx.fmd_state, 0);
	sema_init(&g_connv3_ctx.fmd_sema, 1);
	drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_WIFI];
	ret = msg_thread_send_1(&drv_inst->msg_ctx,
		CONNV3_SUBDRV_OPID_FMD_PRE_CB, CONNV3_DRV_TYPE_WIFI);
	if (ret)
		pr_notice("[FMD] pre_cb - wifi fail, ret = %d\n", ret);

	fmd_ready_state = (0x1 << CONNV3_DRV_TYPE_WIFI);
	while (atomic_read(&g_connv3_ctx.fmd_state) != fmd_ready_state) {
		ret = down_timeout(&g_connv3_ctx.fmd_sema, msecs_to_jiffies(CONNV3_PRE_CAL_TIMEOUT));
		if (ret == 0)
			continue;
		pr_info("[FMD] pre_cb - wifi is not back");
	}
	pr_info("[FMD][PRE] wifi done\n");
	osal_gettimeofday(&pre_wifi_end);

	pr_info("[FMD] pre_cb - bt\n");
	atomic_set(&g_connv3_ctx.fmd_state, 0);
	sema_init(&g_connv3_ctx.fmd_sema, 1);
	drv_inst = &g_connv3_ctx.drv_inst[CONNV3_DRV_TYPE_BT];
	ret = msg_thread_send_1(&drv_inst->msg_ctx,
		CONNV3_SUBDRV_OPID_FMD_PRE_CB, CONNV3_DRV_TYPE_BT);
	if (ret)
		pr_notice("[FMD] pre_cb - bt fail, ret = %d", ret);

	fmd_ready_state = (0x1 << CONNV3_DRV_TYPE_BT);
	while (atomic_read(&g_connv3_ctx.fmd_state) != fmd_ready_state) {
		ret = down_timeout(&g_connv3_ctx.fmd_sema, msecs_to_jiffies(CONNV3_FMD_TIMEOUT));
		if (ret == 0)
			continue;
		pr_info("[FMD] pre_cb - bt is not back\n");
	}
	pr_info("[FMD][PRE] bt done\n");
	osal_gettimeofday(&pre_bt_end);

	/* Power off common resource */
	connv3_core_wake_lock_get();
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		ret = connv3_hw_pwr_off(opfunc_get_current_status(), i, NULL);
		if (ret)
			pr_notice("[FMD] power off %s fail, ret = %d\n", connv3_drv_name[i], ret);
		g_connv3_ctx.drv_inst[i].drv_status = DRV_STS_POWER_OFF;
	}
	g_connv3_ctx.core_status = DRV_STS_POWER_OFF;
	connv3_core_wake_lock_put();

	/* Check radio status */
	ret = opfunc_get_current_status();
	if (ret != 0)
		pr_notice("[FMD] all radio should be off, but get 0x%x\n", ret);
	osal_gettimeofday(&pwr_off_end);
	pr_info("[FMD] Power off done\n");

	/* Post callback */
	atomic_set(&g_connv3_ctx.fmd_state, 0);
	sema_init(&g_connv3_ctx.fmd_sema, 1);
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		drv_inst = &g_connv3_ctx.drv_inst[i];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				CONNV3_SUBDRV_OPID_FMD_POST_CB, i);
	}
	while (atomic_read(&g_connv3_ctx.fmd_state) != subdrv_all_done) {
		ret = down_timeout(&g_connv3_ctx.fmd_sema, msecs_to_jiffies(CONNV3_FMD_TIMEOUT));
		if (ret == 0)
			continue;
		cur_rst_state = atomic_read(&g_connv3_ctx.fmd_state);
		for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
			if ((cur_rst_state & (0x1 << i)) == 0) {
				pr_info("[FMD][%s] post-callback is not back", connv3_drv_thread_name[i]);
				drv_inst = &g_connv3_ctx.drv_inst[i];
				osal_thread_show_stack(&drv_inst->msg_ctx.thread);
			}
		}
	}
	osal_gettimeofday(&post_cb_end);
	pr_info("[FMD] post-callback end\n");

	osal_unlock_sleepable_lock(&g_connv3_ctx.core_lock);
	atomic_set(&g_connv3_ctx.fmd_mode_trigger, 0);

	pr_info("[FMD] summary total=[%lu] pre_wifi_cb=[%lu] pre_bt_cb=[%lu] pwr_off=[%lu] post_cb=[%lu]\n",
		timespec64_to_ms(&fmd_begin, &post_cb_end),
		timespec64_to_ms(&fmd_begin, &pre_wifi_end),
		timespec64_to_ms(&pre_wifi_end, &pre_bt_end),
		timespec64_to_ms(&pre_bt_end, &pwr_off_end),
		timespec64_to_ms(&pwr_off_end, &post_cb_end));

	return 0;
}

int connv3_core_is_fmd_locking(void)
{
	return atomic_read(&g_connv3_ctx.fmd_mode_trigger);
}

/*
 * CONNv3 API
 */
int connv3_core_power_on(enum connv3_drv_type type)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send_wait_1(&ctx->msg_ctx,
				CONNV3_OPID_PWR_ON, 0, type);
	if (ret) {
		pr_err("[%s] fail, ret = %d\n", __func__, ret);
		return ret;
	}
	return 0;
}

int connv3_core_power_on_done(enum connv3_drv_type type)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send_wait_1(&ctx->msg_ctx,
				CONNV3_OPID_PWR_ON_DONE, 0, type);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}
	return 0;
}

int connv3_core_power_off(enum connv3_drv_type type)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send_wait_1(&ctx->msg_ctx,
				CONNV3_OPID_PWR_OFF, 0, type);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}
	return 0;
}

int connv3_core_ext_32k_on(void)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send_wait(&ctx->msg_ctx,
		CONNV3_OPID_EXT_32K_ON, 0);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}
	return 0;
}

int connv3_core_pre_cal_start(void)
{
	int ret = 0;
	bool skip = false;
	enum pre_cal_caller caller;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	struct pre_cal_info *cal_info = &ctx->cal_info;

	ret = osal_lock_sleepable_lock(&cal_info->pre_cal_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	caller = cal_info->caller;
	pr_info("[%s] [pre_cal] Caller = %u", __func__, caller);

	/* Handle different pre_cal_mode */
	switch (g_pre_cal_mode) {
		case PRE_CAL_ALL_DISABLED:
			pr_info("[%s] [pre_cal] Skip all pre-cal", __func__);
			skip = true;
			cal_info->status = PRE_CAL_DONE;
			break;
		case PRE_CAL_PWR_ON_DISABLED:
			if (caller == PRE_CAL_BY_SUBDRV_REGISTER) {
				pr_info("[%s] [pre_cal] Skip pre-cal triggered by subdrv register", __func__);
				skip = true;
				cal_info->status = PRE_CAL_NOT_INIT;
			}
			break;
		case PRE_CAL_SCREEN_ON_DISABLED:
			if (caller == PRE_CAL_BY_SCREEN_ON) {
				pr_info("[%s] [pre_cal] Skip pre-cal triggered by screen on", __func__);
				skip = true;
				cal_info->status = PRE_CAL_DONE;
			}
			break;
		default:
			pr_info("[%s] [pre_cal] Begin pre-cal, g_pre_cal_mode: %u",
				__func__, g_pre_cal_mode);
			break;
	}

	if (skip) {
		pr_info("[%s] [pre_cal] Reset status to %d", __func__, cal_info->status);
		osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
		return -2;
	}

	cal_info->status = PRE_CAL_EXECUTING;
	ret = msg_thread_send_wait(&ctx->cb_ctx,
				CONNV3_CB_OPID_PRE_CAL, CONNV3_PRE_CAL_OP_TIMEOUT);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
	}

	cal_info->status = PRE_CAL_DONE;
	osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
	return 0;
}

int connv3_core_screen_on(void)
{
#if 0
	int ret = 0, rst_status;
	unsigned long flag;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	spin_lock_irqsave(&ctx->rst_lock, flag);
	rst_status = g_connv3_ctx.rst_status;
	spin_unlock_irqrestore(&ctx->rst_lock, flag);

	if (rst_status > CHIP_RST_NONE) {
		pr_info("rst is ongoing, skip pre_cal");
		return 0;
	}

	ret = msg_thread_send_1(&ctx->msg_ctx,
			CONNV3_OPID_PRE_CAL_PREPARE, PRE_CAL_BY_SCREEN_ON);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}
#endif
	return 0;
}

int connv3_core_screen_off(void)
{
#if 0
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send(&ctx->msg_ctx,
				CONNV3_OPID_PRE_CAL_CHECK);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}
#endif
	return 0;
}

int connv3_core_lock_rst(unsigned int* rst_source)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret = 0;
	unsigned long flag;

	spin_lock_irqsave(&ctx->rst_lock, flag);

	if (rst_source)
		*rst_source = ctx->rst_source;
	ret = ctx->rst_status;
	if (ctx->rst_status > CHIP_RST_NONE &&
		ctx->rst_status < CHIP_RST_DONE) {
		/* do nothing */
	} else {
		ctx->rst_status = CHIP_RST_START;
	}
	spin_unlock_irqrestore(&ctx->rst_lock, flag);

	pr_info("[%s] ret=[%d]", __func__, ret);
	return ret;
}

int connv3_core_unlock_rst(void)
{
	unsigned long flag;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	spin_lock_irqsave(&ctx->rst_lock, flag);
	ctx->rst_status = CHIP_RST_NONE;
	spin_unlock_irqrestore(&ctx->rst_lock, flag);
	return 0;
}

int connv3_core_trg_chip_rst(enum connv3_reset_source rst_source, enum connv3_drv_type drv, char *reason)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	if (rst_source >= CONNV3_CHIP_RST_SOURCE_MAX) {
		pr_notice("[%s] rst_source(%d) invalid\n", __func__, rst_source);
		return -1;
	}

	if (snprintf(&ctx->trg_reason[rst_source][0], CHIP_RST_REASON_MAX_LEN, "%s", reason) < 0)
		pr_warn("[%s::%d] snprintf error\n", __func__, __LINE__);
	ret = msg_thread_send_2(&ctx->cb_ctx,
				CONNV3_CB_OPID_CHIP_RST, drv, rst_source);
	if (ret) {
		pr_err("[%s] send msg fail, ret = %d", __func__, ret);
		return -1;
	}
	pr_info("trg_reset DONE!");
	return 0;
}

int connv3_core_pmic_event_cb(unsigned int id, unsigned int event)
{
	int r;
	unsigned int rst_source = 0;

	/* id = 0, means 6639 project. 6639 only support pmic shutdown to reboot.
	 * id = 1, means project that support POR_RST.
	 */
	r = connv3_core_lock_rst(&rst_source);
	if (id == 0) {
		if (r >= CHIP_RST_START) {
			/* reset is ongoing */
			pr_info("[%s] r=[%d] chip rst is ongoing\n", __func__, r);
			return 1;
		}
	}
	if (id == 1) {
		if (r >= CHIP_RST_START && rst_source != CONNV3_CHIP_RST_SOURCE_NORMAL) {
			pr_info("[%s] r=[%d] source=[%d] chip rst is ongoing\n", __func__, r, rst_source);
			return 1;
		}
	}

	if (event == 1) {
		pr_info("[%s] r=[%d] source=[%u] id=[%u] event=[%u] retrigger L0\n",
			__func__, r, rst_source, id, event);
		connv3_core_trg_chip_rst(CONNV3_CHIP_RST_SOURCE_PMIC_FAULT_B, CONNV3_DRV_TYPE_CONNV3, "PMIC Fault");
	} else if (event == 2) {
		pr_info("[%s] r=[%d] source=[%u] id=[%u] event=[%u] retrigger L0\n",
			__func__, r, rst_source, id, event);
		connv3_core_trg_chip_rst(CONNV3_CHIP_RST_SOURCE_PMIC_FAULT_B, CONNV3_DRV_TYPE_CONNV3, "Co-clock error");
	}

	return 0;
}

void connv3_core_update_pmic_status(enum connv3_drv_type drv, char *buffer, int buf_sz)
{
	connv3_hw_pmic_parse_state(buffer, buf_sz);
}

int connv3_core_subsys_ops_reg(enum connv3_drv_type type,
					struct connv3_sub_drv_ops_cb *cb)
{
	unsigned long flag;
	struct subsys_drv_inst *drv_inst;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int trigger_pre_cal = 0, ret = 0;

	if (type < CONNV3_DRV_TYPE_BT || type >= CONNV3_DRV_TYPE_MAX)
		return -1;

	ret = osal_lock_sleepable_lock(&ctx->subsys_op_lock);
	if (ret) {
		pr_notice("[%s] get subsys_op_lock fail, ret = %d\n", __func__, ret);
		return -1;
	}

	spin_lock_irqsave(&g_connv3_ctx.infra_lock, flag);
	drv_inst = &g_connv3_ctx.drv_inst[type];
	memcpy(&g_connv3_ctx.drv_inst[type].ops_cb, cb,
					sizeof(struct connv3_sub_drv_ops_cb));

	pr_info("[%s] [pre_cal] type=[%s] cb rst=[%p][%p][%p] pre_cal=[%p][%p]",
			__func__, connv3_drv_name[type],
			cb->rst_cb.pre_whole_chip_rst, cb->rst_cb.post_whole_chip_rst,
			cb->rst_cb.post_reset_on,
			cb->pre_cal_cb.pwr_on_cb, cb->pre_cal_cb.do_cal_cb);

	pr_info("[%s] [pre_cal] type=[%d] bt=[%p][%p] wf=[%p][%p]", __func__, type,
			ctx->drv_inst[CONNV3_DRV_TYPE_BT].ops_cb.pre_cal_cb.pwr_on_cb,
			ctx->drv_inst[CONNV3_DRV_TYPE_BT].ops_cb.pre_cal_cb.do_cal_cb,
			ctx->drv_inst[CONNV3_DRV_TYPE_WIFI].ops_cb.pre_cal_cb.pwr_on_cb,
			ctx->drv_inst[CONNV3_DRV_TYPE_WIFI].ops_cb.pre_cal_cb.do_cal_cb);

	/* trigger pre-cal if BT and WIFI are registered */
	if (ctx->drv_inst[CONNV3_DRV_TYPE_BT].ops_cb.pre_cal_cb.do_cal_cb != NULL &&
		ctx->drv_inst[CONNV3_DRV_TYPE_WIFI].ops_cb.pre_cal_cb.do_cal_cb != NULL)
		trigger_pre_cal = 1;

	spin_unlock_irqrestore(&g_connv3_ctx.infra_lock, flag);

	if (trigger_pre_cal) {
		pr_info("[%s] [pre_cal] trigger pre-cal BT/WF are registered", __func__);
		ret = msg_thread_send_1(&ctx->msg_ctx,
				CONNV3_OPID_PRE_CAL_PREPARE, PRE_CAL_BY_SUBDRV_REGISTER);
		if (ret)
			pr_err("send pre_cal_prepare msg fail, ret = %d\n", ret);
	}

	osal_unlock_sleepable_lock(&ctx->subsys_op_lock);
	return 0;
}

int connv3_core_subsys_ops_unreg(enum connv3_drv_type type)
{
	unsigned long flag;
	int ret;

	if (type < CONNV3_DRV_TYPE_BT || type >= CONNV3_DRV_TYPE_MAX)
		return -1;

	ret = osal_lock_sleepable_lock(&g_connv3_ctx.subsys_op_lock);
	if (ret) {
		pr_notice("[%s] get subsys_op_lock fail, ret = %d\n", __func__, ret);
		return -1;
	}
	spin_lock_irqsave(&g_connv3_ctx.infra_lock, flag);
	memset(&g_connv3_ctx.drv_inst[type].ops_cb, 0,
					sizeof(struct connv3_sub_drv_ops_cb));
	spin_unlock_irqrestore(&g_connv3_ctx.infra_lock, flag);

	osal_unlock_sleepable_lock(&g_connv3_ctx.subsys_op_lock);
	return 0;
}

static int connv3_is_pre_cal_timeout_by_cb_not_registered(struct timespec64 *start)
{
	struct timespec64 now;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	void *bt_cb;
	void *wifi_cb;
	unsigned long diff;
	const char *exception_title[2] = {"combo_bt", "combo_wifi"};
	int exception_title_index;
	char exception_log[70];

	osal_gettimeofday(&now);
	diff = timespec64_to_ms(start, &now);

	if (diff > CONNV3_MAX_PRE_CAL_BLOCKING_TIME) {
		bt_cb = (void *)ctx->drv_inst[CONNV3_DRV_TYPE_BT].ops_cb.pre_cal_cb.do_cal_cb;
		wifi_cb = (void *)ctx->drv_inst[CONNV3_DRV_TYPE_WIFI].ops_cb.pre_cal_cb.do_cal_cb;
		if (bt_cb == NULL || wifi_cb == NULL) {
			pr_notice("%s [pre_cal][timeout!!] bt=[%p] wf=[%p]\n", __func__, bt_cb, wifi_cb);
			exception_title_index = (bt_cb == NULL ? 0 : 1);
			if (snprintf(exception_log, sizeof(exception_log), "pre-cal timeout. %s callback is not registered",
				exception_title[exception_title_index]) > 0) {
				osal_dbg_common_exception_api(
					exception_title[exception_title_index],
					NULL, 0,
					(const int*)exception_log, strlen(exception_log),
					exception_log, 0);
			}
			return 1;
		}
	}
	return 0;
}

void connv3_core_pre_cal_blocking(void)
{
#define BLOCKING_CHECK_MONITOR_THREAD 100
	int ret;
	struct pre_cal_info *cal_info = &g_connv3_ctx.cal_info;
	struct timespec64 start, end;
	unsigned long diff;
	static bool ever_pre_cal = false;

	if (connv3_hw_pre_cal_blocking_enable() == 0) {
		pr_info("[%s] pre-cal blocking is disable\n", __func__);
		return;
	}

	if (g_pre_cal_mode == PRE_CAL_ALL_DISABLED) {
		pr_info("g_pre_cal_mode == PRE_CAL_ALL_DISABLED\n");
		return;
	}

	osal_gettimeofday(&start);

	/* non-zero means lock got, zero means not */
	while (true) {
		// Handle PRE_CAL_PWR_ON_DISABLED case:
		// 1. Do pre-cal "only once" after bootup if BT or WIFI is default on
		// 2. Use ever_pre_cal to check if the "first" pre-cal is already
		//    triggered. If yes, skip pre-cal
		if (g_pre_cal_mode == PRE_CAL_PWR_ON_DISABLED && !ever_pre_cal) {
			struct connv3_ctx *ctx = &g_connv3_ctx;

			ret = msg_thread_send_1(&ctx->msg_ctx,
					CONNV3_OPID_PRE_CAL_PREPARE, PRE_CAL_BY_SUBDRV_PWR_ON);
			ever_pre_cal = true;
			pr_info("[%s] [pre_cal] Triggered by subdrv power on and set ever_pre_cal to true, result: %d", __func__, ret);
		}

		ret = osal_trylock_sleepable_lock(&cal_info->pre_cal_lock);
		if (ret) {
			if (cal_info->status == PRE_CAL_NOT_INIT || cal_info->status == PRE_CAL_SCHEDULED) {
				pr_info("[%s] [pre_cal] ret=[%d] status=[%d]", __func__, ret, cal_info->status);
				osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
				if (connv3_is_pre_cal_timeout_by_cb_not_registered(&start) == 1)
					break;
				osal_sleep_ms(100);
				continue;
			}
			osal_unlock_sleepable_lock(&cal_info->pre_cal_lock);
			break;
		} else {
			pr_info("[%s] [pre_cal] ret=[%d] status=[%d]", __func__, ret, cal_info->status);
			osal_sleep_ms(100);
		}
	}
	osal_gettimeofday(&end);

	diff = timespec64_to_ms(&start, &end);
	if (diff > BLOCKING_CHECK_MONITOR_THREAD)
		pr_info("blocking spent [%lu]", diff);
}


static void _connv3_core_update_rst_status(enum chip_rst_status status)
{
	unsigned long flag;

	spin_lock_irqsave(&g_connv3_ctx.rst_lock, flag);
	g_connv3_ctx.rst_status = status;
	spin_unlock_irqrestore(&g_connv3_ctx.rst_lock, flag);
}


int connv3_core_is_rst_locking(void)
{
	unsigned long flag;
	int ret = 0;

	spin_lock_irqsave(&g_connv3_ctx.rst_lock, flag);

	if (g_connv3_ctx.rst_status > CHIP_RST_NONE &&
		g_connv3_ctx.rst_status < CHIP_RST_POST_CB)
		ret = 1;
	spin_unlock_irqrestore(&g_connv3_ctx.rst_lock, flag);
	return ret;
}

int connv3_core_is_rst_power_off_stage(void)
{
	unsigned long flag;
	int ret = 0;

	spin_lock_irqsave(&g_connv3_ctx.rst_lock, flag);

	if (g_connv3_ctx.rst_status >= CHIP_RST_RESET)
		ret = 1;
	spin_unlock_irqrestore(&g_connv3_ctx.rst_lock, flag);
	return ret;
}

int connv3_core_bus_dump(enum connv3_drv_type drv_type)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	struct subsys_drv_inst *drv_inst = &ctx->drv_inst[drv_type];
	struct connv3_cr_cb *cb = &drv_inst->ops_cb.cr_cb;

	if (cb->read == NULL || cb->write == NULL || cb->write_mask == NULL) {
		pr_notice("[%s] %s cr_cb is imcomplete:[%p][%p][%p]\n",
			__func__, connv3_drv_name[drv_type],
			cb->read, cb->write, cb->write_mask);
		return -EINVAL;
	}

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	if (ctx->core_status == DRV_STS_POWER_ON)
		ret = connv3_hw_bus_dump(drv_type, cb);
	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}

static void connv3_core_pre_cal_work_handler(struct work_struct *work)
{
	int ret;

	/* if fail, do we need re-try? */
	ret = connv3_core_pre_cal_start();
	pr_info("[%s] [pre_cal][ret=%d] -----------", __func__, ret);
}

int connv3_core_reset_power_state(void)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send_wait(
		&ctx->msg_ctx, CONNV3_OPID_RESET_POWER_STATE, 0);
	if (ret) {
		pr_notice("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

int connv3_core_dump_power_state(char *buf, unsigned int size)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	if (buf && size > 0)
		ret = msg_thread_send_wait_2(
			&ctx->msg_ctx, CONNV3_OPID_DUMP_POWER_STATE,
			0, (size_t)buf, size);
	else
		ret = msg_thread_send_wait(
			&ctx->msg_ctx, CONNV3_OPID_DUMP_POWER_STATE, 0);
	if (ret) {
		pr_notice("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

int connv3_core_reset_and_dump_power_state(char *buf, unsigned int size, bool force_dump)
{
	int ret = 0;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	if (buf && size > 0)
		ret = msg_thread_send_wait_3(
			&ctx->msg_ctx, CONNV3_OPID_RESET_AND_DUMP_POWER_STATE,
			0, force_dump, (size_t)buf, size);
	else
		ret = msg_thread_send_wait_1(
			&ctx->msg_ctx, CONNV3_OPID_RESET_AND_DUMP_POWER_STATE, 0, force_dump);
	if (ret) {
		pr_notice("[%s] send msg fail, ret = %d\n", __func__, ret);
		return -1;
	}

	return 0;
}

static int __check_hif_dump_cb(enum connv3_drv_type to_drv)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	struct subsys_drv_inst *drv_inst = &ctx->drv_inst[to_drv];
	struct connv3_cr_cb *cb = &drv_inst->ops_cb.cr_cb;

	if (cb->read == NULL || cb->write == NULL || cb->write_mask == NULL) {
		pr_notice("[%s] %s cr_cb is imcomplete:[%p][%p][%p]\n",
			__func__, connv3_drv_name[to_drv],
			cb->read, cb->write, cb->write_mask);
		return -EINVAL;
	}

	if (drv_inst->ops_cb.hif_dump_cb.hif_dump_start == NULL ||
	    drv_inst->ops_cb.hif_dump_cb.hif_dump_end == NULL) {
		pr_notice("[%s] %s hif_dump is  imcomplete:[%p][%p]\n",
			__func__, connv3_drv_name[to_drv],
			drv_inst->ops_cb.hif_dump_cb.hif_dump_start,
			drv_inst->ops_cb.hif_dump_cb.hif_dump_end);
		return -EINVAL;
	}

	return 0;
}

int connv3_core_hif_dbg_start(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv)
{
	int ret;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	struct subsys_drv_inst *drv_inst = &ctx->drv_inst[to_drv];
	struct connv3_hif_dump_cb *cb = &drv_inst->ops_cb.hif_dump_cb;
	struct connv3_cr_cb *cr_cb = &ctx->drv_inst[to_drv].ops_cb.cr_cb;

	ret = __check_hif_dump_cb(to_drv);
	if (ret)
		return ret;

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	pr_info("[%s] from %s to %s\n", __func__, connv3_drv_name[from_drv], connv3_drv_name[to_drv]);
	ret = cb->hif_dump_start(from_drv, cr_cb->priv_data);
	if (ret)
		pr_notice("[%s] ret = %d", __func__, ret);

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}


int connv3_core_hif_dbg_end(enum connv3_drv_type from_drv, enum connv3_drv_type to_drv)
{
	int ret;
	struct connv3_ctx *ctx = &g_connv3_ctx;
	struct subsys_drv_inst *drv_inst = &ctx->drv_inst[to_drv];
	struct connv3_hif_dump_cb *cb = &drv_inst->ops_cb.hif_dump_cb;
	struct connv3_cr_cb *cr_cb = &ctx->drv_inst[to_drv].ops_cb.cr_cb;

	ret = __check_hif_dump_cb(to_drv);
	if (ret)
		return ret;

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	pr_info("[%s] from %s to %s\n", __func__, connv3_drv_name[from_drv], connv3_drv_name[to_drv]);
	ret = cb->hif_dump_end(from_drv, cr_cb->priv_data);
	if (ret)
		pr_notice("[%s] ret = %d", __func__, ret);

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}

int connv3_core_hif_dbg_read(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int *value)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret;
	struct connv3_cr_cb *cb = &ctx->drv_inst[to_drv].ops_cb.cr_cb;

	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);

	ret = __check_hif_dump_cb(to_drv);
	if (ret)
		return ret;

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	ret = cb->read(cb->priv_data, addr, value);
	if (ret && __ratelimit(&_rs))
		pr_notice("[%s] ret = %d", __func__, ret);

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}

int connv3_core_hif_dbg_write(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int value)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret;
	struct connv3_cr_cb *cb = &ctx->drv_inst[to_drv].ops_cb.cr_cb;
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);

	ret = __check_hif_dump_cb(to_drv);
	if (ret)
		return ret;

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	ret = cb->write(cb->priv_data, addr, value);
	if (ret && __ratelimit(&_rs))
		pr_notice("[%s] ret = %d", __func__, ret);

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}

int connv3_core_hif_dbg_write_mask(
	enum connv3_drv_type from_drv, enum connv3_drv_type to_drv,
	unsigned int addr, unsigned int mask, unsigned int value)
{
	struct connv3_ctx *ctx = &g_connv3_ctx;
	int ret;
	struct connv3_cr_cb *cb = &ctx->drv_inst[to_drv].ops_cb.cr_cb;
	static DEFINE_RATELIMIT_STATE(_rs, 10 * HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);

	ret = __check_hif_dump_cb(to_drv);
	if (ret)
		return ret;

	ret = osal_lock_sleepable_lock(&ctx->core_lock);
	if (ret) {
		pr_err("[%s] get lock fail, ret = %d\n",
			__func__, ret);
		return -1;
	}

	ret = cb->write_mask(cb->priv_data, addr, mask, value);
	if (ret && __ratelimit(&_rs))
		pr_notice("[%s] ret = %d", __func__, ret);

	osal_unlock_sleepable_lock(&ctx->core_lock);

	return ret;
}

int connv3_core_enter_fmd_mode(void)
{
	int ret;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	ret = msg_thread_send(&ctx->cb_ctx, CONNV3_CB_OPID_FMD_MODE);
	if (ret) {
		pr_notice("[%s] msg send failed, ret = %d\n", __func__, ret);
		return -1;
	}

	pr_info("[%s] fmd mode enter success\n", __func__);
	return 0;
}

static void connv3_core_wake_lock_get(void)
{
	osal_wake_lock(&g_connv3_wake_lock);
	pr_info("[%s] after wake_lock(%d)\n", __func__, osal_wake_lock_count(&g_connv3_wake_lock));
}

static void connv3_core_wake_lock_put(void)
{
	int count = 0;
	osal_wake_unlock(&g_connv3_wake_lock);

	count = osal_wake_lock_count(&g_connv3_wake_lock);

	if (count != 0) {
		pr_notice("[%s] osal_wake_lock_count(%d) is unexpected\n", __func__, count);
	}
}

int connv3_core_init(void)
{
	int ret = 0, i;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	// Get pre-cal mode
	const struct conninfra_conf *conf = NULL;
	conf = conninfra_conf_get_cfg();
	if (conf != NULL) {
		if (conf->pre_cal_mode == PRE_CAL_MODE_DEFAULT)
			g_pre_cal_mode = PRE_CAL_SCREEN_ON_DISABLED;
		else
			g_pre_cal_mode = conf->pre_cal_mode;
	}
	pr_info("[%s] [pre_cal] Init g_pre_cal_mode = %u", __func__, g_pre_cal_mode);

	osal_memset(&g_connv3_ctx, 0, sizeof(g_connv3_ctx));

	reset_chip_rst_trg_data();

	spin_lock_init(&ctx->infra_lock);
	osal_sleepable_lock_init(&ctx->core_lock);
	spin_lock_init(&ctx->rst_lock);
	osal_sleepable_lock_init(&ctx->subsys_op_lock);
	//spin_lock_init(&ctx->power_dump_lock);
	//atomic_set(&ctx->power_dump_enable, 0);

	ret = msg_thread_init(&ctx->msg_ctx, "connv3_cored",
				connv3_core_opfunc, CONNV3_OPID_MAX);
	if (ret) {
		pr_err("msg_thread init fail(%d)\n", ret);
		return -1;
	}

	ret = msg_thread_init(&ctx->cb_ctx, "connv3_cb",
                               connv3_core_cb_opfunc, CONNV3_CB_OPID_MAX);
	if (ret) {
		pr_err("callback msg thread init fail(%d)\n", ret);
		return -1;
	}
	/* init subsys drv state */
	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		ret += msg_thread_init(&ctx->drv_inst[i].msg_ctx,
				connv3_drv_thread_name[i], connv3_subdrv_opfunc,
				CONNV3_SUBDRV_OPID_MAX);
	}

	if (ret) {
		pr_err("subsys callback thread init fail.\n");
		return -1;
	}

	/* pre_cal */
	INIT_WORK(&ctx->cal_info.pre_cal_work, connv3_core_pre_cal_work_handler);
	osal_sleepable_lock_init(&ctx->cal_info.pre_cal_lock);

	/* wake lock */
	osal_strcpy(g_connv3_wake_lock.name, "connv3FuncCtrl");
	g_connv3_wake_lock.init_flag = 0;
	osal_wake_lock_init(&g_connv3_wake_lock);

	/* Init atomic variable */
	atomic_set(&g_connv3_ctx.fmd_mode_trigger, 0);

	return ret;
}


int connv3_core_deinit(void)
{
	int ret, i;
	struct connv3_ctx *ctx = &g_connv3_ctx;

	osal_sleepable_lock_deinit(&ctx->cal_info.pre_cal_lock);

	for (i = 0; i < CONNV3_DRV_TYPE_MAX; i++) {
		ret = msg_thread_deinit(&ctx->drv_inst[i].msg_ctx);
		if (ret)
			pr_warn("subdrv [%d] msg_thread deinit fail (%d)\n",
						i, ret);
	}

	ret = msg_thread_deinit(&ctx->msg_ctx);
	if (ret) {
		pr_err("msg_thread_deinit fail(%d)\n", ret);
		return -1;
	}

	osal_sleepable_lock_deinit(&ctx->subsys_op_lock);
	osal_sleepable_lock_deinit(&ctx->core_lock);
	osal_wake_lock_deinit(&g_connv3_wake_lock);

	//connectivity_export_conap_scp_deinit();

	return 0;
}



