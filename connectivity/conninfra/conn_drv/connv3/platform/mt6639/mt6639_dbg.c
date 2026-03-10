// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/delay.h>

#include "connv3.h"
#include "connv3_hw.h"
#include "connv3_hw_dbg.h"

#include "mt6639_dbg.h"

#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL		0x7c060380
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP	0x7c060384
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER		0x7c060388
#define CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER		0x7c06038c

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int connv3_conninfra_bus_dump_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);
static int connv3_conninfra_power_info_dump_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size);
static int connv3_conninfra_power_info_reset_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb);


/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
const struct connv3_platform_dbg_ops g_connv3_hw_dbg_mt6639 = {
	.dbg_bus_dump = connv3_conninfra_bus_dump_mt6639,
	.dbg_power_info_dump = connv3_conninfra_power_info_dump_mt6639,
	.dbg_power_info_reset = connv3_conninfra_power_info_reset_mt6639,
};

#define POWER_STATE_DUMP_DATA_SIZE 25
unsigned long mt6639_power_state_dump_data[POWER_STATE_DUMP_DATA_SIZE];

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

static int connv3_bus_check_ap2conn_off(struct connv3_cr_cb *cb)
{
	unsigned int value;
	int ret, i;
	void *data = cb->priv_data;

	/* AP2CONN_INFRA OFF
	 * 1.Check "AP2CONN_INFRA ON step is ok"
	 * 2. Check conn_infra off bus clock
	 *    (Need to polling 4 times to confirm the correctness and polling every 1ms)
	 * - write 0x1 to 0x1802_3000[0], reset clock detect
	 * - 0x1802_3000[1] conn_infra off bus clock (should be 1'b1 if clock exist)
	 * - 0x1802_3000[2] osc clock (should be 1'b1 if clock exist)
	 * 3. Read conn_infra IP version
	 * - Read 0x1801_1000 = 0x03010001
	 * 4. Check conn_infra off domain bus hang irq status
	 * - 0x1802_3400[2:0], should be 3'b000, or means conn_infra off bus might hang
	 */

	/* Clock detect
	 */
	for (i = 0; i < 4; i++) {
		ret = cb->write_mask(data, MT6639_CONN_INFRA_CLK_DETECT, 0x1, 0x1);
		if (ret) {
			pr_notice("[%s] clock detect write fail, ret = %d", __func__, ret);
			break;
		}
		ret = cb->read(data, MT6639_CONN_INFRA_CLK_DETECT, &value);
		if (ret) {
			pr_notice("[%s] clock detect read fail, ret = %d", __func__, ret);
			break;
		}
		if ((value & 0x6) == 0x6)
			break;
		udelay(1000);
	}
	if (ret)
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	if ((value & 0x6) != 0x6) {
		pr_notice("[%s] clock detect fail, get: [0x%08x]", __func__, value);
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}

	/* Read IP version
	 */
	ret = cb->read(data, MT6639_CONN_INFRA_VERSION_ID_REG, &value);
	if (ret) {
		pr_notice("[%s] get conn_infra version fail, ret=[%d]", __func__, ret);
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}
	if (value != MT6639_CONN_INFRA_VERSION_ID &&
	    value != MT6639_CONN_INFRA_VERSION_ID_E2) {
		pr_notice("[%s] get conn_infra version fail, expect:[0x%08x or 0x%08x], get:0x%08x",
			__func__,
			MT6639_CONN_INFRA_VERSION_ID, MT6639_CONN_INFRA_VERSION_ID_E2,
			value);
		return CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR;
	}

	/* Check bus timeout irq
	 */
	ret = cb->read(data, MT6639_CONN_INFRA_OFF_IRQ_REG, &value);
	if (ret) {
		pr_notice("[%s] read irq status fail, ret=[%d]", __func__, ret);
		return CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ;
	}
	if (value != 0x0) {
		pr_notice("[%s] bus time out irq detect, get:0x%08x", __func__, value);
		return CONNV3_BUS_CONN_INFRA_BUS_HANG_IRQ;
	}

	return 0;
}

int connv3_conninfra_bus_dump_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	int ret = 0, func_ret = 0;

	pr_info("[V3_BUS] version=%s\n", MT6639_CONN_INFRA_BUS_DUMP_VERSION);

	/* Dump after conn_infra_on is ready
	 * - Connsys power debug - dump list
	 * - Conninfra bus debug - status result
	 * - conn_infra_cfg_clk - dump_list
	 */
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_pwr_b, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_pwr_b dump err=[%d]", __func__, ret);
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_bus_a, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_bus_a dump err=[%d]", __func__, ret);
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_cfg_clk_a, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_cfg_clk_a dump err=[%d]", __func__, ret);

	/* AP2CONN_INFRA OFF check */
	func_ret = connv3_bus_check_ap2conn_off(cb);
	if (func_ret == CONNV3_BUS_CONN_INFRA_OFF_CLK_ERR)
		return func_ret;

	/* Dump conninfra off CR even timeout irq is triggerred. */
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_pwr_c, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_pwr_c dump err=[%d]", __func__, ret);
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_bus_b, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_bus_b dump err=[%d]", __func__, ret);
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_cfg_clk_b, cb);
	if (ret)
		pr_notice("[%s] mt6639_dmp_list_cfg_clk_b dump err=[%d]", __func__, ret);

	/* Extra information dump */
	ret = connv3_hw_dbg_dump_utility(&mt6639_dmp_list_bus_extra, cb);
	if (ret)
		pr_notice("[%s] &mt6639_dmp_list_bus_extra dump err=[%d]", __func__, ret);

	return func_ret;
}

#if 0
static int connv3_26m_dump(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
#define POWER_STATE_BUFF_LENGTH	256
	unsigned int i, str_len;
	unsigned int buf_len = 0;
	unsigned int r;
	static const char *const osc_str[] = {
		/* 0 */
		"", "fm ", "gps ", "bgf ",
		/* 4 */
		"wf ", "conn_infra_bus ", " ", "ap2conn ", " ",
		/* 8 */
		" ", " ", " ", " ",
		/* 12 */
		"conn_pta ", "conn_spi ", " ", "conn_thm "};
	char buf[POWER_STATE_BUFF_LENGTH] = {'\0'};
	int ret;
	void *data = cb->priv_data;

	/* w	0x1806_015c[2:0]=3b'000
	 * r	0x1806_0a04
	 */
	ret = cb->write_mask(data, 0x7c06015c, 0x7, 0x0);
	if (ret)
		return ret;
	ret = cb->read(data, 0x7c060a04, &r);
	if (ret)
		return ret;

	for (i = 0; i < 16; i++) {
		str_len = strlen(osc_str[i]);

		if ((r & (0x1 << (1 + i))) > 0 && (buf_len + str_len < POWER_STATE_BUFF_LENGTH)) {
			strncat(buf, osc_str[i], str_len);
			buf_len += str_len;
		}
	}
	pr_info("[%s] [0x%x] %s", __func__, r, buf);

	return 0;
}
#endif


static inline void __sleep_count_trigger_read(struct connv3_cr_cb *cb)
{
	int ret;
	void *data = cb->priv_data;

	/* 0x7c06_0380[4] = 0
	 * udelay 150
	 * 0x7c06_0380[4] = 1
	 */
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL, (0x1U << 4), 0);
	udelay(150);
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL, (0x1U << 4), (0x1U << 4));
}

#define POWER_STATE_BUF_SIZE 256
static char temp_buf[POWER_STATE_BUF_SIZE];

int connv3_conninfra_sleep_count_dump(
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
	int ret;
	char *buf_p = temp_buf;
	int buf_sz = POWER_STATE_BUF_SIZE;
	void *data = cb->priv_data;

	/* Sleep count */
	/* 1. Setup read select: 0x1806_0380[3:1]
	 *	3'h0: conn_infra sleep counter
	 *	3'h1: wfsys sleep counter
	 *	3'h2: bgfsys sleep counter
	 *	3'h3: gpssys sleep counter
	 * 2. Dump time and count
	 *	a. Timer: 0x1806_0388
	 *	b. Count: 0x1806_038C
	 */
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL, 0xe, (0x0 << 1));
	__sleep_count_trigger_read(cb);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER, &conninfra_sleep_cnt);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER, &conninfra_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] conninfra dump error", drv_type);
		goto print_partial_log;
	}
	udelay(60);

	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL, 0xe, (0x1U << 1));
	__sleep_count_trigger_read(cb);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER, &wf_sleep_cnt);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER, &wf_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] wifi dump error", drv_type);
		goto print_partial_log;
	}
	udelay(60);

	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL, 0xe, (0x2U << 1));
	__sleep_count_trigger_read(cb);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER, &bt_sleep_cnt);
	ret += cb->read(data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER, &bt_sleep_time);
	if (ret) {
		pr_notice("[Connv3][Power dump][%d] bt dump error", drv_type);
		goto print_partial_log;
	}
	udelay(60);

	t_conninfra_sleep_cnt += conninfra_sleep_cnt;
	t_conninfra_sleep_time += conninfra_sleep_time;
	t_wf_sleep_cnt += wf_sleep_cnt;
	t_wf_sleep_time += wf_sleep_time;
	t_bt_sleep_cnt += bt_sleep_cnt;
	t_bt_sleep_time += bt_sleep_time;

	mt6639_power_state_dump_data[0] = round;
	mt6639_power_state_dump_data[1] = CONN_TICK_TO_SEC(conninfra_sleep_time);
	mt6639_power_state_dump_data[2] =
		CONN_TICK_TO_SEC((conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[3] = conninfra_sleep_cnt;
	mt6639_power_state_dump_data[4] = CONN_TICK_TO_SEC(wf_sleep_time);
	mt6639_power_state_dump_data[5] =
		CONN_TICK_TO_SEC((wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[6] = wf_sleep_cnt;
	mt6639_power_state_dump_data[7] = CONN_TICK_TO_SEC(bt_sleep_time);
	mt6639_power_state_dump_data[8] =
		CONN_TICK_TO_SEC((bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[9] = bt_sleep_cnt;

	mt6639_power_state_dump_data[13] = CONN_TICK_TO_SEC(t_conninfra_sleep_time);
	mt6639_power_state_dump_data[14] =
		CONN_TICK_TO_SEC((t_conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[15] = t_conninfra_sleep_cnt;
	mt6639_power_state_dump_data[16] = CONN_TICK_TO_SEC(t_wf_sleep_time);
	mt6639_power_state_dump_data[17] =
		CONN_TICK_TO_SEC((t_wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[18] = t_wf_sleep_cnt;
	mt6639_power_state_dump_data[19] = CONN_TICK_TO_SEC(t_bt_sleep_time);
	mt6639_power_state_dump_data[20] =
		CONN_TICK_TO_SEC((t_bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[21] = t_bt_sleep_cnt;
	round++;

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	ret = snprintf(buf_p, buf_sz,
		"[connv3_power_state][round:%lu]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;[total]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu,%lu;",
		mt6639_power_state_dump_data[0],
		mt6639_power_state_dump_data[1],
		mt6639_power_state_dump_data[2],
		mt6639_power_state_dump_data[3],
		mt6639_power_state_dump_data[4],
		mt6639_power_state_dump_data[5],
		mt6639_power_state_dump_data[6],
		mt6639_power_state_dump_data[7],
		mt6639_power_state_dump_data[8],
		mt6639_power_state_dump_data[9],
		mt6639_power_state_dump_data[13],
		mt6639_power_state_dump_data[14],
		mt6639_power_state_dump_data[15],
		mt6639_power_state_dump_data[16],
		mt6639_power_state_dump_data[17],
		mt6639_power_state_dump_data[18],
		mt6639_power_state_dump_data[19],
		mt6639_power_state_dump_data[20],
		mt6639_power_state_dump_data[21]);
	if (ret)
		pr_info("%s", buf_p);

	return 0;

print_partial_log:
	mt6639_power_state_dump_data[13] = CONN_TICK_TO_SEC(t_conninfra_sleep_time);
	mt6639_power_state_dump_data[14] =
		CONN_TICK_TO_SEC((t_conninfra_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[15] = t_conninfra_sleep_cnt;
	mt6639_power_state_dump_data[16] = CONN_TICK_TO_SEC(t_wf_sleep_time);
	mt6639_power_state_dump_data[17] =
		CONN_TICK_TO_SEC((t_wf_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[18] = t_wf_sleep_cnt;
	mt6639_power_state_dump_data[19] = CONN_TICK_TO_SEC(t_bt_sleep_time);
	mt6639_power_state_dump_data[20] =
		CONN_TICK_TO_SEC((t_bt_sleep_time % CONN_32K_TICKS_PER_SEC) * 1000);
	mt6639_power_state_dump_data[21] = t_bt_sleep_cnt;

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}
	ret = snprintf(buf_p, buf_sz,
		"[connv3_power_state][total]conninfra:%lu.%03lu,%lu;wf:%lu.%03lu,%lu;bt:%lu.%03lu;",
		mt6639_power_state_dump_data[13],
		mt6639_power_state_dump_data[14],
		mt6639_power_state_dump_data[15],
		mt6639_power_state_dump_data[16],
		mt6639_power_state_dump_data[17],
		mt6639_power_state_dump_data[18],
		mt6639_power_state_dump_data[19],
		mt6639_power_state_dump_data[20],
		mt6639_power_state_dump_data[21]);

	if (ret)
		pr_info("%s", buf_p);


	return 0;
}

int connv3_conninfra_power_info_dump_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb,
	char *buf, unsigned int size)
{
	int ret;

#if 0 // remove 26M info because the dump flow would wakeup subsys
	ret = connv3_26m_dump(drv_type, cb);
	if (ret) {
		pr_notice("[%s][%d] dump 26M fail, ret = %d", __func__, drv_type, ret);
		return -1;
	}
#endif

	ret = connv3_conninfra_sleep_count_dump(drv_type, cb, buf, size);
	if (ret) {
		pr_notice("[%s][%d] sleep count dump fail, ret = %d", __func__, drv_type, ret);
		return -1;
	}
	return 0;
}

int connv3_conninfra_power_info_reset_mt6639(
	enum connv3_drv_type drv_type, struct connv3_cr_cb *cb)
{
	unsigned int ctl_bit;
	int ret;
	void *data = cb->priv_data;

	/* switch to host side
	 * HOST_CONN_INFRA_SLP_CNT_CTL (0x18060000 + 0x380)
	 * 	HOST_SLP_COUNTER_CTL_EN[31]  - (RW) Sleep counter control enable:
	 * 	1'h0: Control by conn_infra_cfg
	 * 	1'h1: Control by conn_host_csr_top
	 */
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL,
		(0x1U << 31 | 0x1U), (0x1U << 31 | 0x1U));

	/* Clear data and disable stop to keep going */
	/* I. Clear
	 * i.	Conn_infra:	0x1806_0384[15]
	 * ii.	Wf:		0x1806_0384[9]
	 * iii.	Bt:		0x1806_0384[10]
	 */

	ctl_bit = ((0x1U << 15) | (0x1U << 9) | (0x1U << 10));
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP, ctl_bit, ctl_bit);
	udelay(150);
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP, ctl_bit, 0x0);

	ctl_bit = ((0x1U << 7) | (0x1U << 2) | (0x1U << 1));
	/* II. Stop
	 * i.	Conn_infra:	0x1806_0384[7]
	 * ii.	Wf:		0x1806_0384[1]
	 * iii.	Bt:		0x1806_0384[2]
	 */
	ret = cb->write_mask(
		data, CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP, ctl_bit, 0x0);

	return 0;
}
