// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/delay.h>

#include "connv3.h"
#include "connv3_hw.h"
#include "connv3_hw_dbg.h"

#include "mt6653_dbg.h"

#define MT6653_CONN_INFRA_CLK_DETECT		0x20023000
#define MT6653_CONN_INFRA_VERSION_ID_REG	0x7c011000
#define MT6653_CONN_INFRA_VERSION_ID		0x03040001
#define MT6653_CONN_INFRA_OFF_IRQ_REG		0x20023400

#define MT6653_VERSION_ID_FAIL_A2C	0xdead0a2c
#define MT6653_VERSION_ID_FAIL_ZERO	0xdead0000

#define CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT_CTL	0x7C001020
#define CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT		0x7C001024
#define CONN_INFRA_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT	0x7C00102C
#define CONN_INFRA_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT	0x7C001028
#define CONN_INFRA_CFG_ON_CONN_INFRA_SLP_TIMER		0x7C00104C
#define CONN_INFRA_CFG_ON_CONN_INFRA_SLP_COUNTER	0x7C001050
#define CONN_INFRA_CFG_ON_WF_SLP_TIMER			0x7C001054
#define CONN_INFRA_CFG_ON_WF_SLP_COUNTER		0x7C001058
#define CONN_INFRA_CFG_ON_BT_SLP_TIMER			0x7C00105C
#define CONN_INFRA_CFG_ON_BT_SLP_COUNTER		0x7C001060

#define CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR	0x7c0601a0

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int connv3_conninfra_bus_dump_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);
static int connv3_conninfra_power_info_dump_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size);
static int connv3_conninfra_power_info_reset_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);


/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
const struct connv3_platform_dbg_ops g_connv3_hw_dbg_mt6653 = {
	.dbg_bus_dump = connv3_conninfra_bus_dump_mt6653,
	.dbg_power_info_dump = connv3_conninfra_power_info_dump_mt6653,
	.dbg_power_info_reset = connv3_conninfra_power_info_reset_mt6653,
};

const struct connv3_dbg_command mt6653_extra_bt[] = {
	/* Write, addr, mask, value, Read, addr*/
	/* A01 */ {false, 0, 0, 0, true, 0x81025200},
	/* A02 */ {false, 0, 0, 0, true, 0x80000000},
	/* A03 */ {false, 0, 0, 0, true, 0x81030130},
};

const struct connv3_dump_list mt6653_dmp_list_extra_bt = {
	"extra_bt", NULL,
	3, sizeof(mt6653_extra_bt)/sizeof(struct connv3_dbg_command),
	mt6653_extra_bt,
};

#define POWER_STATE_DUMP_DATA_SIZE 25
unsigned long mt6653_power_state_dump_data[POWER_STATE_DUMP_DATA_SIZE];

#define POWER_STATE_BUF_SIZE 256
static char temp_buf[POWER_STATE_BUF_SIZE];

/*******************************************************************************
 *                              F U N C T I O N S
 ********************************************************************************
 */

static int connv3_bus_check_ap2conn_off_mt6653(struct connv3_cr_cb *cb)
{
	unsigned int value;
	int i, ret;
	void *data = cb->priv_data;

	/* AP2CONN_INFRA OFF */
	/* 1.Check "AP2CONN_INFRA ON step is ok" (X)
	 * 2. Check conn_infra off bus clock
	 * (Need to polling 4 times to confirm the correctness and polling every 1ms)
	 * - write 0x1 to 0x1802_3000[0], reset clock detect
	 * - 0x1802_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * - 0x1802_3000[3] lp_osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * - Read 0x1801_1000 = 0x03030002
	 * 4. Check conn_infra off domain bus hang irq status
	 * - 0x1802_3400[9:0], should be 10'b0, or means conn_infra off bus timeout
	 */
	for (i = 0; i < 4; i++) {
		ret = cb->write_mask(data, MT6653_CONN_INFRA_CLK_DETECT, 0x1, 0x1);
		if (ret) {
			pr_notice("[%s] clock detect write fail, ret = %d", __func__, ret);
			break;
		}
		ret = cb->read(data, MT6653_CONN_INFRA_CLK_DETECT, &value);
		if (ret) {
			pr_notice("[%s] clock detect read fail, ret = %d", __func__, ret);
			break;
		}
		if ((value & 0xa) == 0xa)
			break;
		udelay(1000);
	}
	if (ret)
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	if ((value & 0xa) != 0xa) {
		pr_notice("[%s] clock detect fail, get: [0x%08x]", __func__, value);
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}

	/* Read IP version */
	ret = cb->read(data, MT6653_CONN_INFRA_VERSION_ID_REG, &value);
	if (ret) {
		pr_notice("[%s] get conn_infra version fail, ret=[%d]", __func__, ret);
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}
	if (value != MT6653_CONN_INFRA_VERSION_ID) {
		pr_notice("[%s] get conn_infra version fail, expect:[0x%08x], get:0x%08x",
			__func__, MT6653_CONN_INFRA_VERSION_ID, value);
		if (value == 0xdead0a2c)
			return MT6653_VERSION_ID_FAIL_A2C;
		if (value == 0x0)
			return MT6653_VERSION_ID_FAIL_ZERO;
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}

	/* Check bus timeout irq */
	ret = cb->read(data, MT6653_CONN_INFRA_OFF_IRQ_REG, &value);
	if (ret) {
		pr_notice("[%s] read irq status fail, ret=[%d]", __func__, ret);
		return CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ;
	}
	if ((value & 0x3ff) != 0x0) {
		pr_notice("[%s] bus time out irq detect, get:0x%08x", __func__, value);
		return CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ;
	}

	return 0;
}

int connv3_conninfra_dump_von_mt6653(struct connv3_cr_cb *cb)
{
	int ret = 0;

	/* Dump host side CR */
	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_conn_infra_bus_a, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_conn_infra_bus_a error(%d)\n", __func__, ret);
	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_conn_infra_top_a, cb);
	if (ret)
		pr_notice("[%s] mt6653_conn_infra_top_a error(%d)\n", __func__, ret);
	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_connsys_power_b, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_connsys_power_b error(%d)\n", __func__, ret);

	return 0;
}

static int __make_conninfra_sleep(struct connv3_cr_cb *cb)
{
	int ret = 0;

	ret = cb->write(cb->priv_data, CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR, 0x0);
	if (ret) {
		pr_notice("[%s] make conninfra sleep fail, ret=[%d]\n", __func__, ret);
		return -1;
	}
	return 0;
}


static int __wakeup_conninfra(struct connv3_cr_cb *cb)
{
	int ret = 0, count = 0;
	unsigned int value;
	void *data = cb->priv_data;

	ret = cb->write(data, CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR, 0x1);
	if (ret) {
		pr_notice("[%s] write 0x%x fail, ret = %d\n",
			__func__, CONN_HOST_CSR_TOP_CONN_INFRA_WAKEPU_TOP_ADDR, ret);
		return -1;
	}

	udelay(5000);

	for (count = 0; count < 10; count++) {
		ret = cb->read(data, MT6653_CONN_INFRA_VERSION_ID_REG, &value);
		if (ret) {
			pr_notice("[%s] get conn_infra version fail, ret=[%d]", __func__, ret);
			__make_conninfra_sleep(cb);
			return -1;
		}
		if (value == MT6653_CONN_INFRA_VERSION_ID) {
			pr_info("[%s] get 0x%x after %d times polling\n", __func__, value, count);
			return 0;
		}
		udelay(1000);
	}

	pr_notice("[%s] wakeup fail\n", __func__);
	__make_conninfra_sleep(cb);

	return -1;
}


int connv3_conninfra_bus_dump_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	int ret = 0, func_ret = 0;
	unsigned int value = 0;

	/* Print version */
	pr_info("[V3_BUS][PSOP_1_1] version=%s\n", MT6653_CONNINFRA_DEBUGSOP_DUMP_VERSION);

	/* Dump host side CR */
	connv3_conninfra_dump_von_mt6653(cb);

	/* AP2CONN_INFRA OFF check */
	func_ret = connv3_bus_check_ap2conn_off_mt6653(cb);
	if (func_ret == CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR)
		return func_ret;

	/* Special case for more dump */
	if (func_ret == MT6653_VERSION_ID_FAIL_A2C) {
		if (drv_type == CONNV3_DRV_TYPE_WIFI) {
			/* Dump von again */
			pr_info("[V3_BUS][PSOP_1_1] version=%s\n", MT6653_CONNINFRA_DEBUGSOP_DUMP_VERSION);
			/* Dump host side CR */
			connv3_conninfra_dump_von_mt6653(cb);
		}
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}
	if (func_ret == MT6653_VERSION_ID_FAIL_ZERO) {
		if (drv_type == CONNV3_DRV_TYPE_BT) {
			ret = connv3_hw_dbg_dump_utility(&mt6653_dmp_list_extra_bt, cb);
			if (ret)
				pr_notice("[%s] mt6653_extra_bt dump err=[%d]", __func__, ret);
		}
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}


	/* Dump conninfra off */
	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_conn_infra_bus_b, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_conn_infra_bus_b error(%d)\n", __func__, ret);
	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_conn_infra_bus_c, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_conn_infra_bus_c error(%d)\n", __func__, ret);

	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_conn_infra_top_b, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_conn_infra_top_b error(%d)\n", __func__, ret);

	ret = connv3_hw_dbg_unify_dump_utility(
		&mt6653_dump_list_connsys_power_c, cb);
	if (ret)
		pr_notice("[%s] mt6653_dump_list_connsys_power_c error(%d)\n", __func__, ret);

	if (func_ret == CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ) {
		/* Check bus timeout irq */
		ret = cb->read(cb->priv_data, MT6653_CONN_INFRA_OFF_IRQ_REG, &value);
		if (ret) {
			pr_notice("[%s] read irq status fail, ret=[%d]", __func__, ret);
		} else {
			/* Check [7:12] */
			if ((value & 0x1F80) != 0x0) {
				pr_info("[%s] timeout irq = %x\n", __func__, value);
				/* Force wakeup and dump */
				ret = __wakeup_conninfra(cb);
				if (ret)
					return 0;
				ret = connv3_hw_dbg_unify_dump_utility(
					&mt6653_dump_list_conn_infra_bus_d, cb);
				if (ret)
					pr_notice("[%s] mt6653_dump_list_conn_infra_bus_d error(%d)\n", __func__, ret);
				__make_conninfra_sleep(cb);

			}
		}
	}

	return 0;
}

int connv3_conninfra_power_info_dump_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size)
{
#define CONN_32K_TICKS_PER_SEC (32768)
#define CONN_TICK_TO_SEC(TICK) (TICK / CONN_32K_TICKS_PER_SEC)
	static u64 round;
	static u64 t_conninfra_sleep_cnt, t_conninfra_sleep_time;
	static u64 t_wf_sleep_cnt, t_wf_sleep_time;
	static u64 t_bt_sleep_cnt, t_bt_sleep_time;
	unsigned int conninfra_sleep_cnt = 0, conninfra_sleep_time = 0;
	unsigned int wf_sleep_cnt = 0, wf_sleep_time = 0;
	unsigned int bt_sleep_cnt = 0, bt_sleep_time = 0;
	int ret = 0;
	char *buf_p = temp_buf;
	int buf_sz = POWER_STATE_BUF_SIZE;
	void *data = cb->priv_data;

	/* 1. Stop counter
	 *    conninfra: 0x1800_1024[0] = 1'b1
	 *    wifi: 0x1800_1028[0] = 1'b1
	 *    bt: 0x1800_102c[0] = 1'b1
	 * 2. Read timer
	 *    conninfra: 0x1800_104c
	 *    wifi: 0x1800_1054
	 *    bt: 0x1800_1058
	 * 3. Read counter
	 *    conninfra: 0x1800_1050
	 *    wifi: 0x1800_1054
	 *    bt: 0x1800_1060
	 */
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT, 0x1, 0x1);
	ret += cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT, 0x1, 0x1);
	ret += cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT, 0x1, 0x1);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] stop counter error", drv_type);
		goto print_partial_log;
	}

	ret = cb->read(data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_COUNTER, &conninfra_sleep_cnt);
	ret += cb->read(data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_TIMER, &conninfra_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] dump conninfra error\n", drv_type);
		goto print_partial_log;
	}
	ret = cb->read(data, CONN_INFRA_CFG_ON_WF_SLP_COUNTER, &wf_sleep_cnt);
	ret += cb->read(data, CONN_INFRA_CFG_ON_WF_SLP_TIMER, &wf_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] wifi dump error", drv_type);
		goto print_partial_log;
	}
	ret = cb->read(data, CONN_INFRA_CFG_ON_BT_SLP_COUNTER, &bt_sleep_cnt);
	ret += cb->read(data, CONN_INFRA_CFG_ON_BT_SLP_TIMER, &bt_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] bt dump error", drv_type);
		goto print_partial_log;
	}

	t_conninfra_sleep_cnt += conninfra_sleep_cnt;
	t_conninfra_sleep_time += conninfra_sleep_time;
	t_wf_sleep_cnt += wf_sleep_cnt;
	t_wf_sleep_time += wf_sleep_time;
	t_bt_sleep_cnt += bt_sleep_cnt;
	t_bt_sleep_time += bt_sleep_time;

	mt6653_power_state_dump_data[0] = round;
	mt6653_power_state_dump_data[1] = CONN_TICK_TO_SEC(conninfra_sleep_time);
	mt6653_power_state_dump_data[2] =
		CONN_TICK_TO_SEC((conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[3] = conninfra_sleep_cnt;
	mt6653_power_state_dump_data[4] = CONN_TICK_TO_SEC(wf_sleep_time);
	mt6653_power_state_dump_data[5] =
		CONN_TICK_TO_SEC((wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[6] = wf_sleep_cnt;
	mt6653_power_state_dump_data[7] = CONN_TICK_TO_SEC(bt_sleep_time);
	mt6653_power_state_dump_data[8] =
		CONN_TICK_TO_SEC((bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[9] = bt_sleep_cnt;

	mt6653_power_state_dump_data[13] = CONN_TICK_TO_SEC(t_conninfra_sleep_time);
	mt6653_power_state_dump_data[14] =
		CONN_TICK_TO_SEC((t_conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[15] = t_conninfra_sleep_cnt;
	mt6653_power_state_dump_data[16] = CONN_TICK_TO_SEC(t_wf_sleep_time);
	mt6653_power_state_dump_data[17] =
		CONN_TICK_TO_SEC((t_wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[18] = t_wf_sleep_cnt;
	mt6653_power_state_dump_data[19] = CONN_TICK_TO_SEC(t_bt_sleep_time);
	mt6653_power_state_dump_data[20] =
		CONN_TICK_TO_SEC((t_bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[21] = t_bt_sleep_cnt;
	round++;

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	ret = snprintf(buf_p, buf_sz,
		"[connv3_power_state][round:%lu]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;[total]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;",
		mt6653_power_state_dump_data[0],
		mt6653_power_state_dump_data[1],
		mt6653_power_state_dump_data[2],
		mt6653_power_state_dump_data[3],
		mt6653_power_state_dump_data[4],
		mt6653_power_state_dump_data[5],
		mt6653_power_state_dump_data[6],
		mt6653_power_state_dump_data[7],
		mt6653_power_state_dump_data[8],
		mt6653_power_state_dump_data[9],
		mt6653_power_state_dump_data[13],
		mt6653_power_state_dump_data[14],
		mt6653_power_state_dump_data[15],
		mt6653_power_state_dump_data[16],
		mt6653_power_state_dump_data[17],
		mt6653_power_state_dump_data[18],
		mt6653_power_state_dump_data[19],
		mt6653_power_state_dump_data[20],
		mt6653_power_state_dump_data[21]);
	if (ret)
		pr_info("%s", buf_p);

	return 0;

print_partial_log:
	mt6653_power_state_dump_data[13] = CONN_TICK_TO_SEC(t_conninfra_sleep_time);
	mt6653_power_state_dump_data[14] =
		CONN_TICK_TO_SEC((t_conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[15] = t_conninfra_sleep_cnt;
	mt6653_power_state_dump_data[16] = CONN_TICK_TO_SEC(t_wf_sleep_time);
	mt6653_power_state_dump_data[17] =
		CONN_TICK_TO_SEC((t_wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[18] = t_wf_sleep_cnt;
	mt6653_power_state_dump_data[19] = CONN_TICK_TO_SEC(t_bt_sleep_time);
	mt6653_power_state_dump_data[20] =
		CONN_TICK_TO_SEC((t_bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6653_power_state_dump_data[21] = t_bt_sleep_cnt;

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	ret = snprintf(buf_p, buf_sz,
		"[connv3_power_state][total]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;",
		mt6653_power_state_dump_data[13],
		mt6653_power_state_dump_data[14],
		mt6653_power_state_dump_data[15],
		mt6653_power_state_dump_data[16],
		mt6653_power_state_dump_data[17],
		mt6653_power_state_dump_data[18],
		mt6653_power_state_dump_data[19],
		mt6653_power_state_dump_data[20],
		mt6653_power_state_dump_data[21]);

	if (ret)
		pr_info("%s", buf_p);

	return 0;
}

int connv3_conninfra_power_info_reset_mt6653(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	int ret;
	void *data = cb->priv_data;

	/* 1. Enable slp counter function
	 *    0x1800_1020[0] = 1b'1
	 * 2. Release stop register
	 *    conninfra: 0x1800_1024[0] = 1b'0
	 *    wifi: 0x1800_1028[0] = 1b'0
	 *    bt: 0x1800_102c[0] = 1b'0
	 * 3. Clear
	 *    a. Set clr = 1b'1
	 *       conninfra:0x1800_1024[1]=1b'1
	 *       wifi: 0x1800_1028[1]=1b'1
	 *       bt: 0x1800_102c[1]=1b'1
	 *    b. wait 150us
	 *    c. Set clr = 1b'0
	 *       conninfra:0x1800_1024[1]=1b'0
	 *       wifi: 0x1800_1028[1]=1b'0
	 *       bt: 0x1800_102c[1]=1b'0
	 */
	/* Step 1 */
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT_CTL, 0x1, 0x1);

	/* Step 2 */
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT, 0x1, 0x0);
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT, 0x1, 0x0);
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT, 0x1, 0x0);

	/* Step 3 */
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT, (0x1U << 1), (0x1U << 1));
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT, (0x1U << 1), (0x1U << 1));
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT, (0x1U << 1), (0x1U << 1));
	udelay(150);
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_SLP_CNT, (0x1U << 1), 0x0);
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_WFSYS_SLP_CNT, (0x1U << 1), 0x0);
	ret = cb->write_mask(
		data, CONN_INFRA_CFG_ON_CONN_INFRA_BGFSYS_SLP_CNT, (0x1U << 1), 0x0);

	return 0;
}
