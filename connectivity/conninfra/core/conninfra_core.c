/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
/*! \file
*    \brief  Declaration of library functions
*
*    Any definitions in this file will be shared among GLUE Layer and internal Driver Stack.
*/

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include "consys_hw.h"
#include "conninfra_core.h"
#include "msg_thread.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CONNINFRA_EVENT_TIMEOUT 3000

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/delay.h>

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int opfunc_power_on(struct msg_op_data *op);
static int opfunc_power_off(struct msg_op_data *op);
static int opfunc_chip_rst(struct msg_op_data *op);
static int opfunc_pre_cal(struct msg_op_data *op);
static int opfunc_therm_ctrl(struct msg_op_data *op);


static int opfunc_subdrv_pre_reset(struct msg_op_data *op);
static int opfunc_subdrv_post_reset(struct msg_op_data *op);
static int opfunc_subdrv_cal_pwr_on(struct msg_op_data *op);
static int opfunc_subdrv_cal_do_cal(struct msg_op_data *op);
static int opfunc_subdrv_therm_ctrl(struct msg_op_data *op);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct conninfra_ctx g_conninfra_ctx;

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static const msg_opid_func conninfra_core_opfunc[] = {
	[CONNINFRA_OPID_PWR_ON] = opfunc_power_on,
	[CONNINFRA_OPID_PWR_OFF] = opfunc_power_off,
	[CONNINFRA_OPID_THERM_CTRL] = opfunc_therm_ctrl,
};

static const msg_opid_func conninfra_core_cb_opfunc[] = {
	[CONNINFRA_CB_OPID_CHIP_RST] = opfunc_chip_rst,
	[CONNINFRA_CB_OPID_PRE_CAL] = opfunc_pre_cal,
};


/* subsys ops */
static char *drv_thread_name[] = {
	[CONNDRV_TYPE_BT] = "sub_bt_thrd",
	[CONNDRV_TYPE_FM] = "sub_fm_thrd",
	[CONNDRV_TYPE_GPS] = "sub_gps_thrd",
	[CONNDRV_TYPE_WIFI] = "sub_wifi_thrd",
};

typedef enum {
	INFRA_SUBDRV_OPID_PRE_RESET	= 0,
	INFRA_SUBDRV_OPID_POST_RESET	= 1,
	INFRA_SUBDRV_OPID_CAL_PWR_ON	= 2,
	INFRA_SUBDRV_OPID_CAL_DO_CAL	= 3,
	INFRA_SUBDRV_OPID_THERM_CTRL	= 4,

	INFRA_SUBDRV_OPID_MAX
} infra_subdrv_op;


static const msg_opid_func infra_subdrv_opfunc[] = {
	[INFRA_SUBDRV_OPID_PRE_RESET] = opfunc_subdrv_pre_reset,
	[INFRA_SUBDRV_OPID_POST_RESET] = opfunc_subdrv_post_reset,
	[INFRA_SUBDRV_OPID_CAL_PWR_ON] = opfunc_subdrv_cal_pwr_on,
	[INFRA_SUBDRV_OPID_CAL_DO_CAL] = opfunc_subdrv_cal_do_cal,
	[INFRA_SUBDRV_OPID_THERM_CTRL] = opfunc_subdrv_therm_ctrl,
};

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static int opfunc_power_on_internal(unsigned int drv_type)
{
	int ret;

	/* Check abnormal type */
	if (drv_type >= CONNDRV_TYPE_MAX) {
		pr_err("abnormal Fun(%d)\n", drv_type);
		return -1;
	}

	/* Check abnormal state */
	if ((g_conninfra_ctx.drv_inst[drv_type].drv_status < DRV_STS_POWER_OFF)
	    || (g_conninfra_ctx.drv_inst[drv_type].drv_status >= DRV_STS_MAX)) {
		pr_err("func(%d) status[0x%x] abnormal\n", drv_type,
				g_conninfra_ctx.drv_inst[drv_type].drv_status);
		return -2;
	}

	/* check if func already on */
	if (g_conninfra_ctx.drv_inst[drv_type].drv_status == DRV_STS_POWER_ON) {
		pr_warn("func(%d) already on\n", drv_type);
		return 0;
	}

	if (g_conninfra_ctx.infra_drv_status == DRV_STS_POWER_OFF) {
		ret = consys_hw_pwr_on();
		if (ret) {
			pr_err("Conninfra power on fail. drv(%d) ret=(%d)\n",
				drv_type, ret);
			return -3;
		}
		/* POWER ON SEQUENCE */
		g_conninfra_ctx.infra_drv_status = DRV_STS_POWER_ON;
	}

	g_conninfra_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_ON;

	/* VCNx enable */
	switch (drv_type) {
	case CONNDRV_TYPE_BT:
		consys_hw_bt_power_ctl(1);
		break;
	case CONNDRV_TYPE_FM:
		consys_hw_fm_power_ctl(1);
		break;
	case CONNDRV_TYPE_GPS:
		consys_hw_gps_power_ctl(1);
		break;
	case CONNDRV_TYPE_WIFI:
		consys_hw_wifi_power_ctl(1);
		break;
	default:
		break;
	}

	return 0;
}

static int opfunc_power_on(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];

	return opfunc_power_on_internal(drv_type);
}

static int opfunc_power_off_internal(unsigned int drv_type)
{
	int i, ret;
	bool try_power_off = true;

	/* Check abnormal type */
	if (drv_type >= CONNDRV_TYPE_MAX) {
		pr_err("abnormal Fun(%d)\n", drv_type);
		return -1;
	}

	/* Check abnormal state */
	if ((g_conninfra_ctx.drv_inst[drv_type].drv_status < DRV_STS_POWER_OFF)
	    || (g_conninfra_ctx.drv_inst[drv_type].drv_status >= DRV_STS_MAX)) {
		pr_err("func(%d) status[0x%x] abnormal\n", drv_type,
			g_conninfra_ctx.drv_inst[drv_type].drv_status);
		return -2;
	}

	/* check if func already on */
	if (g_conninfra_ctx.drv_inst[drv_type].drv_status
				== DRV_STS_POWER_OFF) {
		pr_warn("func(%d) already on\n", drv_type);
		return 0;
	}

	/* VCNx disable */
	switch (drv_type) {
	case CONNDRV_TYPE_BT:
		consys_hw_bt_power_ctl(0);
		break;
	case CONNDRV_TYPE_FM:
		consys_hw_fm_power_ctl(0);
		break;
	case CONNDRV_TYPE_GPS:
		consys_hw_gps_power_ctl(0);
		break;
	case CONNDRV_TYPE_WIFI:
		consys_hw_wifi_power_ctl(0);
		break;
	default:
		break;
	}

	g_conninfra_ctx.drv_inst[drv_type].drv_status = DRV_STS_POWER_OFF;

	/* is there subsys on ? */
	for (i = 0; i < CONNDRV_TYPE_MAX; i++)
		if (g_conninfra_ctx.drv_inst[i].drv_status == DRV_STS_POWER_ON)
			try_power_off = false;

	if (try_power_off) {
		/* POWER OFF SEQUENCE */
		ret = consys_hw_pwr_off();
		if (ret) {
			pr_err("Conninfra power on fail. drv(%d) ret=(%d)\n",
				drv_type, ret);
			return -3;
		}
		g_conninfra_ctx.infra_drv_status = DRV_STS_POWER_OFF;
	}

	return 0;
}

static int opfunc_power_off(struct msg_op_data *op)
{
	unsigned int drv_type = op->op_data[0];

	return opfunc_power_off_internal(drv_type);
}

static int opfunc_chip_rst(struct msg_op_data *op)
{
	int i, ret;
	struct subsys_drv_inst *drv_inst;
	unsigned int drv_pwr_state[CONNDRV_TYPE_MAX];

	if (g_conninfra_ctx.infra_drv_status == DRV_STS_POWER_OFF) {
		pr_info("No subsys on, just return");
		return 0;
	}

	atomic_set(&g_conninfra_ctx.rst_state, 0);
	sema_init(&g_conninfra_ctx.rst_sema, 1);
	/* pre */
	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		drv_inst = &g_conninfra_ctx.drv_inst[i];
		drv_pwr_state[i] = drv_inst->drv_status;
		pr_info("subsys %d is %d", i, drv_inst->drv_status);
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				INFRA_SUBDRV_OPID_PRE_RESET, i);
	}
	while (atomic_read(&g_conninfra_ctx.rst_state) < CONNDRV_TYPE_MAX)
		ret = down_interruptible(&g_conninfra_ctx.rst_sema);

	/*******************************************************/
	/* reset */
	/* call consys_hw */
	/*******************************************************/
	/* Turn off subsys one-by-one  */
	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		if (drv_pwr_state[i]) {
			ret = opfunc_power_off_internal(i);
			pr_info("Call subsys(%d) power off ret=%d", i, ret);
		}
	}
	pr_info("conninfra status should be power off. Status=%d", g_conninfra_ctx.infra_drv_status);
	/* Turn on subsys */
	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		if (drv_pwr_state[i]) {
			ret = opfunc_power_on_internal(i);
			pr_info("Call subsys(%d) power on ret=%d", i, ret);
		}
	}
	pr_info("conninfra status should be power on. Status=%d", g_conninfra_ctx.infra_drv_status);

	/* post */
	atomic_set(&g_conninfra_ctx.rst_state, 0);
	sema_init(&g_conninfra_ctx.rst_sema, 1);
	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		drv_inst = &g_conninfra_ctx.drv_inst[i];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				INFRA_SUBDRV_OPID_POST_RESET, i);
	}
	while (atomic_read(&g_conninfra_ctx.rst_state) < CONNDRV_TYPE_MAX)
		ret = down_interruptible(&g_conninfra_ctx.rst_sema);

	return 0;
}

static int opfunc_pre_cal(struct msg_op_data *op)
{
#define CAL_DRV_COUNT 2
	int cal_drvs[CAL_DRV_COUNT] = {CONNDRV_TYPE_BT, CONNDRV_TYPE_WIFI};
	int i, ret;
	struct subsys_drv_inst *drv_inst;

	/* power on subsys */
	atomic_set(&g_conninfra_ctx.pre_cal_state, 0);
	sema_init(&g_conninfra_ctx.pre_cal_sema, 1);

	for (i = 0; i < CAL_DRV_COUNT; i++) {
		drv_inst = &g_conninfra_ctx.drv_inst[cal_drvs[i]];
		ret = msg_thread_send_1(&drv_inst->msg_ctx,
				INFRA_SUBDRV_OPID_CAL_PWR_ON, cal_drvs[i]);
		if (ret)
			pr_warn("driver [%d] power on fail\n", cal_drvs[i]);
	}
	while (atomic_read(&g_conninfra_ctx.pre_cal_state) < CAL_DRV_COUNT)
		down_interruptible(&g_conninfra_ctx.pre_cal_sema);
	pr_info(">>>>>>> power on DONE!!");

	/* Do Calibration */
	drv_inst = &g_conninfra_ctx.drv_inst[CONNDRV_TYPE_BT];
	ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			INFRA_SUBDRV_OPID_CAL_DO_CAL, 0, CONNDRV_TYPE_BT);
	if (ret)
		pr_warn("driver [%d] calibration fail\n", CONNDRV_TYPE_BT);
	pr_info(">>>>>>>> BT do cal done");
	drv_inst = &g_conninfra_ctx.drv_inst[CONNDRV_TYPE_WIFI];
	ret = msg_thread_send_wait_1(&drv_inst->msg_ctx,
			INFRA_SUBDRV_OPID_CAL_DO_CAL, 0, CONNDRV_TYPE_WIFI);
	if (ret)
		pr_warn("driver [%d] calibration fail\n", CONNDRV_TYPE_WIFI);

	pr_info(">>>>>>>> WF do cal done");

	return 0;
}


static int opfunc_therm_ctrl(struct msg_op_data *op)
{
	return 0;
}


static int opfunc_subdrv_pre_reset(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_conninfra_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.rst_cb.pre_whole_chip_rst) {

		ret = drv_inst->ops_cb.rst_cb.pre_whole_chip_rst();
		if (ret)
			pr_err("[%s] fail [%d]", __func__, ret);
	}
	atomic_inc(&g_conninfra_ctx.rst_state);
	up(&g_conninfra_ctx.rst_sema);
	return 0;
}

static int opfunc_subdrv_post_reset(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_conninfra_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.rst_cb.post_whole_chip_rst) {
		ret = drv_inst->ops_cb.rst_cb.post_whole_chip_rst();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	atomic_inc(&g_conninfra_ctx.rst_state);
	up(&g_conninfra_ctx.rst_sema);
	return 0;
}

static int opfunc_subdrv_cal_pwr_on(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, drv_thread_name[drv_type]);

	/* TODO: should be locked, to avoid cb was reset */
	drv_inst = &g_conninfra_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.pre_cal_cb.pwr_on_cb) {
		ret = drv_inst->ops_cb.pre_cal_cb.pwr_on_cb();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	atomic_inc(&g_conninfra_ctx.pre_cal_state);
	up(&g_conninfra_ctx.pre_cal_sema);

	pr_info("[%s] DONE", __func__);
	return 0;
}

static int opfunc_subdrv_cal_do_cal(struct msg_op_data *op)
{
	int ret;
	unsigned int drv_type = op->op_data[0];
	struct subsys_drv_inst *drv_inst;

	pr_info("[%s] drv=[%s]", __func__, drv_thread_name[drv_type]);

	drv_inst = &g_conninfra_ctx.drv_inst[drv_type];
	if (/*drv_inst->drv_status == DRV_ST_POWER_ON &&*/
			drv_inst->ops_cb.pre_cal_cb.do_cal_cb) {
		ret = drv_inst->ops_cb.pre_cal_cb.do_cal_cb();
		if (ret)
			pr_warn("[%s] fail [%d]", __func__, ret);
	}

	pr_info("[%s]", __func__);
	return 0;
}

static int opfunc_subdrv_therm_ctrl(struct msg_op_data *op)
{
	return 0;
}


/*
 * CONNINFRA API
 */
int conninfra_core_power_on(enum consys_drv_type type)
{
	int ret = 0;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	ret = msg_thread_send_wait_1(&infra_ctx->msg_ctx,
				CONNINFRA_OPID_PWR_ON, 0, type);
	if (ret) {
		pr_err("[%s] fail", __func__, ret);
		return -1;
	}
	return 0;
}

int conninfra_core_power_off(enum consys_drv_type type)
{
	int ret = 0;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	ret = msg_thread_send_wait_1(&infra_ctx->msg_ctx,
				CONNINFRA_OPID_PWR_OFF, 0, type);
	if (ret) {
		pr_err("[%s] send msg fail", __func__, ret);
		return -1;
	}
	return 0;
}

int conninfra_core_pre_cal_start(void)
{
	int ret = 0;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	ret = msg_thread_send(&infra_ctx->cb_ctx,
				CONNINFRA_CB_OPID_PRE_CAL);
	if (ret) {
		pr_err("[%s] send msg fail", __func__, ret);
		return -1;
	}
	return 0;
}

int conninfra_core_reg_readable(void)
{
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;
	if (infra_ctx->infra_drv_status == DRV_STS_POWER_ON)
		return consys_hw_reg_readable();
	return 0;
}

int conninfra_core_is_consys_reg(phys_addr_t addr)
{
	return consys_hw_is_connsys_reg(addr);
}


int conninfra_core_lock_rst(void)
{
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;
	int ret;

	/* non-zero means lock got, zero means not */
	ret = osal_trylock_sleepable_lock(&infra_ctx->rst_lock);
	pr_info("[%s] ret=[%d]", __func__, ret);
	return ret;
}

int conninfra_core_unlock_rst(void)
{
	int ret = osal_unlock_sleepable_lock(&g_conninfra_ctx.rst_lock);

	pr_info("[%s] ret=[%d]", __func__, ret);
	return ret;
}

int conninfra_core_trg_chip_rst(enum consys_drv_type drv, char *reason)
{
	int ret = 0;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	ret = msg_thread_send_wait_1(&infra_ctx->cb_ctx,
				CONNINFRA_CB_OPID_CHIP_RST, 0, drv);
	if (ret) {
		pr_err("[%s] send msg fail", __func__, ret);
		return -1;
	}
	return 0;
}


int conninfra_core_subsys_ops_reg(enum consys_drv_type type,
					struct sub_drv_ops_cb *cb)
{
	unsigned long flag;
	struct subsys_drv_inst *drv_inst;

	spin_lock_irqsave(&g_conninfra_ctx.infra_lock, flag);
	drv_inst = &g_conninfra_ctx.drv_inst[type];
	memcpy(&g_conninfra_ctx.drv_inst[type].ops_cb, cb,
					sizeof(struct sub_drv_ops_cb));
	spin_unlock_irqrestore(&g_conninfra_ctx.infra_lock, flag);
	return 0;
}

int conninfra_core_subsys_ops_unreg(enum consys_drv_type type)
{
	unsigned long flag;

	spin_lock_irqsave(&g_conninfra_ctx.infra_lock, flag);
	memset(&g_conninfra_ctx.drv_inst[type].ops_cb, 0,
					sizeof(struct sub_drv_ops_cb));
	spin_unlock_irqrestore(&g_conninfra_ctx.infra_lock, flag);

	return 0;
}

int conninfra_core_init(void)
{
	int ret = 0, i;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	osal_memset(&g_conninfra_ctx, 0, sizeof(g_conninfra_ctx));

	spin_lock_init(&infra_ctx->infra_lock);
	osal_sleepable_lock_init(&infra_ctx->rst_lock);


	ret = msg_thread_init(&infra_ctx->msg_ctx, "conninfra_cored",
				conninfra_core_opfunc, CONNINFRA_OPID_MAX);
	if (ret) {
		pr_err("msg_thread init fail(%d)\n", ret);
		return -1;
	}

	ret = msg_thread_init(&infra_ctx->cb_ctx, "conninfra_cb",
                               conninfra_core_cb_opfunc, CONNINFRA_CB_OPID_MAX);
	if (ret) {
		pr_err("callback msg thread init fail(%d)\n", ret);
		return -1;
	}
	/* init subsys drv state */
	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		msg_thread_init(&infra_ctx->drv_inst[i].msg_ctx,
				drv_thread_name[i],	infra_subdrv_opfunc,
				INFRA_SUBDRV_OPID_MAX);
	}

	return ret;
}


int conninfra_core_deinit(void)
{
	int ret, i;
	struct conninfra_ctx *infra_ctx = &g_conninfra_ctx;

	for (i = 0; i < CONNDRV_TYPE_MAX; i++) {
		ret = msg_thread_deinit(&infra_ctx->drv_inst[i].msg_ctx);
		if (ret)
			pr_warn("subdrv [%d] msg_thread deinit fail (%d)\n",
						i, ret);
	}

	ret = msg_thread_deinit(&infra_ctx->msg_ctx);
	if (ret) {
		pr_err("msg_thread_deinit fail(%d)\n", ret);
		return -1;
	}

	osal_sleepable_lock_deinit(&infra_ctx->rst_lock);

	return 0;
}



