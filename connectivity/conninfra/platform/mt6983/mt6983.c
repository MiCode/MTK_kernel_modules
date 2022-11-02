// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/of.h>
#include <linux/types.h>
#include <mtk_clkbuf_ctl.h>
#include <linux/pm_runtime.h>

#include <connectivity_build_in_adapter.h>

#include "osal.h"
#include "conninfra.h"
#include "conninfra_conf.h"
#include "consys_hw.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"

#include "mt6983.h"
#include "mt6983_pos.h"
#include "mt6983_consys_reg.h"
#include "mt6983_consys_reg_offset.h"
#include "mt6983_connsyslog.h"
#include "clock_mng.h"
#include "coredump_mng.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define PLATFORM_SOC_CHIP 0x6983
#define PRINT_THERMAL_LOG_THRESHOLD 60

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

struct rf_cr_backup_data {
	unsigned int addr;
	unsigned int value1;
	unsigned int value2;
};

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
static int consys_clk_get_from_dts_mt6983(struct platform_device *pdev);
static int consys_clock_buffer_ctrl_mt6983(unsigned int enable);
static unsigned int consys_soc_chipid_get_mt6983(void);
static void consys_clock_fail_dump_mt6983(void);
static unsigned int consys_get_hw_ver_mt6983(void);
static int consys_thermal_query_mt6983(void);
/* Power state relative */
static int consys_enable_power_dump_mt6983(void);
static int consys_reset_power_state_mt6983(void);
static int consys_reset_power_state(void);
static int consys_power_state_dump_mt6983(char *buf, unsigned int size);

static unsigned long long consys_soc_timestamp_get_mt6983(void);

static unsigned int consys_adie_detection_mt6983(void);
static void consys_set_mcu_control_mt6983(int type, bool onoff);

static int consys_pre_cal_backup_mt6983(unsigned int offset, unsigned int size);
static int consys_pre_cal_clean_data_mt6983(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

struct consys_hw_ops_struct g_consys_hw_ops_mt6983 = {
	/* load from dts */
	/* TODO: mtcmos should move to a independent module */
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts_mt6983,

	/* clock */
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl_mt6983,
	.consys_plt_co_clock_type = consys_co_clock_type_mt6983,

	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6983,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6983,

	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6983,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6983,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6983,
	.consys_plt_a_die_cfg = connsys_a_die_cfg_mt6983,
	.consys_plt_afe_wbg_cal = connsys_afe_wbg_cal_mt6983,
	.consys_plt_afe_sw_patch = connsys_afe_sw_patch_mt6983,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6983,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6983,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get_mt6983,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6983,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6983,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6983,

	/* debug */
	.consys_plt_clock_fail_dump = consys_clock_fail_dump_mt6983,
	.consys_plt_get_hw_ver = consys_get_hw_ver_mt6983,

	.consys_plt_spi_read = consys_spi_read_mt6983,
	.consys_plt_spi_write = consys_spi_write_mt6983,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6983,
	.consys_plt_spi_clock_switch = consys_spi_clock_switch_mt6983,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6983,

	.consys_plt_thermal_query = consys_thermal_query_mt6983,
	.consys_plt_enable_power_dump = consys_enable_power_dump_mt6983,
	.consys_plt_reset_power_state = consys_reset_power_state_mt6983,
	.consys_plt_power_state = consys_power_state_dump_mt6983,
	.consys_plt_soc_timestamp_get = consys_soc_timestamp_get_mt6983,
	.consys_plt_adie_detection = consys_adie_detection_mt6983,
	.consys_plt_set_mcu_control = consys_set_mcu_control_mt6983,

	.consys_plt_pre_cal_backup = consys_pre_cal_backup_mt6983,
	.consys_plt_pre_cal_clean_data = consys_pre_cal_clean_data_mt6983,
};

extern struct consys_hw_ops_struct g_consys_hw_ops_mt6983;
extern struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6983;
extern struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6983;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6983;
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6983;

const struct conninfra_plat_data mt6983_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_consys_hw_ops_mt6983,
	.reg_ops = &g_dev_consys_reg_ops_mt6983,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6983,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6983,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6983,
	.connsyslog_config = &g_connsyslog_config,
};

static struct consys_plat_thermal_data_mt6983 g_consys_plat_therm_data;

/* For calibration backup/restore */
static struct rf_cr_backup_data *mt6637_backup_data = NULL;
static unsigned int mt6637_backup_cr_number = 0;
extern phys_addr_t gConEmiPhyBase;

int consys_co_clock_type_mt6983(void)
{
	const struct conninfra_conf *conf;
	struct regmap *map = consys_clock_mng_get_regmap();
	int value = 0;
	static int clock_type = -1;
	const char *clock_name[CONNSYS_CLOCK_SCHEMATIC_MAX] = {
		"26M co-clock", "52M co-clock", "26M tcxo", "52M tcxo"};

	if (clock_type >= 0)
		return clock_type;

	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (NULL == conf) {
		pr_err("[%s] Get conf fail", __func__);
		return -1;
	}

	if (conf->tcxo_gpio != 0 || conn_hw_env.tcxo_support) {
		if (conf->co_clock_flag == 3)
			clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO;
		else
			clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO;
	} else {
		if (!map) {
			pr_notice("%s, failed to get regmap.\n", __func__);
			return -1;
		} else {
			regmap_read(map, DCXO_DIGCLK_ELR, &value);
			if (value & 0x1)
				clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_COTMS;
			else
				clock_type = CONNSYS_CLOCK_SCHEMATIC_26M_COTMS;
		}
	}
	pr_info("[%s] conf->tcxo_gpio=%d conn_hw_env.tcxo_support=%d, %s",
		__func__, conf->tcxo_gpio, conn_hw_env.tcxo_support, clock_name[clock_type]);

	return clock_type;
}

int consys_clk_get_from_dts_mt6983(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);
	dev_pm_syscore_device(&pdev->dev, true);

	return 0;
}

int consys_platform_spm_conn_ctrl_mt6983(unsigned int enable)
{
	int ret = 0;
	struct platform_device *pdev = get_consys_device();

        if (!pdev) {
                pr_info("get_consys_device fail.\n");
                return -1;
        }

	if (enable) {
		ret = pm_runtime_get_sync(&(pdev->dev));
		if (ret)
			pr_info("pm_runtime_get_sync() fail(%d)\n", ret);
		else
			pr_info("pm_runtime_get_sync() CONSYS ok\n");

		ret = device_init_wakeup(&(pdev->dev), true);
		if (ret)
			pr_info("device_init_wakeup(true) fail.\n");
		else
			pr_info("device_init_wakeup(true) CONSYS ok\n");
	} else {
		ret = device_init_wakeup(&(pdev->dev), false);
		if (ret)
			pr_info("device_init_wakeup(false) fail.\n");
		else
			pr_info("device_init_wakeup(false) CONSYS ok\n");

		ret = pm_runtime_put_sync(&(pdev->dev));
		if (ret)
			pr_info("pm_runtime_put_sync() fail.\n");
		else
			pr_info("pm_runtime_put_sync() CONSYS ok\n");
	}
	return ret;
}

int consys_clock_buffer_ctrl_mt6983(unsigned int enable)
{
	/* This function call didn't work now.
	 * clock buffer is HW controlled, not SW controlled.
	 * Keep this function call to update status.
	 */
#if (!COMMON_KERNEL_CLK_SUPPORT)
	if (enable)
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, true);	/*open XO_WCN*/
	else
		KERNEL_clk_buf_ctrl(CLK_BUF_CONN, false);	/*close XO_WCN*/
#endif
	return 0;
}

unsigned int consys_soc_chipid_get_mt6983(void)
{
	return PLATFORM_SOC_CHIP;
}

void consys_clock_fail_dump_mt6983(void)
{
#if defined(KERNEL_clk_buf_show_status_info)
	KERNEL_clk_buf_show_status_info();
#endif
}

int consys_enable_power_dump_mt6983(void)
{
	/* Return success because sleep count dump is enable on POS */
	return 0;
}

int consys_reset_power_state(void)
{
	/* Clear data and disable stop */
	/* I. Clear
	 * i.	Conn_infra:	0x1806_0384[15]
	 * ii.	Wf:		0x1806_0384[9]
	 * iii.	Bt:		0x1806_0384[10]
	 * iv.	Gps:		0x1806_0384[8]
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR,
		0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_CLR,
		0x0);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR,
		0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_CLR,
		0x0);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR,
		0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_CLR,
		0x0);


	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR,
		0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_CLR,
		0x0);
	/* II. Stop
	 * i.	Conn_infra:	0x1806_0384[7]
	 * ii.	Wf:		0x1806_0384[1]
	 * iii.	Bt:		0x1806_0384[2]
	 * iv.	Gps:		0x1806_0384[0]
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_CONN_INFRA_SLEEP_CNT_STOP,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_WF_SLEEP_CNT_STOP,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_BT_SLEEP_CNT_STOP,
		0x0);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_SLP_STOP_HOST_CR_GPS_SLEEP_CNT_STOP,
		0x0);
	return 0;
}

static inline void __sleep_count_trigger_read(void)
{
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER, 0x1);
	udelay(150);
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_RD_TRIGGER, 0x0);

}

static void consys_power_state(void)
{
	unsigned int i, str_len;
	unsigned int buf_len = 0;
	unsigned int r;
	const char* osc_str[] = {
	"fm ", "gps ", "bgf ", "wf ", "conn_infra_bus ", " ", "ap2conn "," ",
	" "," "," ", "conn_pta ", "conn_spi ", " ", "conn_thm "};
	char buf[256] = {'\0'};

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL_CR_CONN_INFRA_CFG_ON_DBG_MUX_SEL,
		0x0);
	r = CONSYS_REG_READ(CONN_HOST_CSR_TOP_CONN_INFRA_CFG_ON_DBG_ADDR);

	for (i = 0; i < 15; i++) {
		str_len = strlen(osc_str[i]);
		
		if ((r & (0x1 << (1 + i))) > 0 && (buf_len + str_len < 256)) {
			strncat(buf, osc_str[i], str_len);
			buf_len += str_len;
		}
	}
	if (r & 0xFFFF)
		pr_info("[%s] [0x%x] %s", __func__, r, buf);
}

static int consys_power_state_dump(char *buf, unsigned int size, int print_log)
{
#define POWER_STATE_BUF_SIZE 256
#define CONN_32K_TICKS_PER_SEC (32768)
#define CONN_TICK_TO_SEC(TICK) (TICK / CONN_32K_TICKS_PER_SEC)
	static u64 round = 0;
	static u64 t_conninfra_sleep_cnt = 0, t_conninfra_sleep_time = 0;
	static u64 t_wf_sleep_cnt = 0, t_wf_sleep_time = 0;
	static u64 t_bt_sleep_cnt = 0, t_bt_sleep_time = 0;
	static u64 t_gps_sleep_cnt = 0, t_gps_sleep_time = 0;
	unsigned int conninfra_sleep_cnt, conninfra_sleep_time;
	unsigned int wf_sleep_cnt, wf_sleep_time;
	unsigned int bt_sleep_cnt, bt_sleep_time;
	unsigned int gps_sleep_cnt, gps_sleep_time;
	char temp_buf[POWER_STATE_BUF_SIZE];
	char* buf_p = temp_buf;
	int buf_sz = POWER_STATE_BUF_SIZE;

	/* Sleep count */
	/* 1. Setup read select: 0x1806_0380[3:1]
	 * 	3'h0: conn_infra sleep counter
	 * 	3'h1: wfsys sleep counter
	 * 	3'h2: bgfsys sleep counter
	 * 	3'h3: gpssys sleep counter
	 * 2. Dump time and count
	 * 	a. Timer: 0x1806_0388
	 * 	b. Count: 0x1806_038C
	 */
	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x0);
	__sleep_count_trigger_read();
	conninfra_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	conninfra_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_conninfra_sleep_time += conninfra_sleep_time;
	t_conninfra_sleep_cnt += conninfra_sleep_cnt;
	/* Wait 60 us to make sure the duration to next write to SLP_COUNTER_RD_TRIGGER is
	 * long enough.
	 */
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x1);
	__sleep_count_trigger_read();
	wf_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	wf_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_wf_sleep_time += wf_sleep_time;
	t_wf_sleep_cnt += wf_sleep_cnt;
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x2);
	__sleep_count_trigger_read();
	bt_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	bt_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);

	t_bt_sleep_time += bt_sleep_time;
	t_bt_sleep_cnt += bt_sleep_cnt;
	udelay(60);

	CONSYS_REG_WRITE_HW_ENTRY(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_CNT_CTL_HOST_SLP_COUNTER_SEL,
		0x3);
	__sleep_count_trigger_read();
	gps_sleep_time = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_TIMER_ADDR);
	gps_sleep_cnt = CONSYS_REG_READ(
		CONN_HOST_CSR_TOP_HOST_CONN_INFRA_SLP_COUNTER_ADDR);
	t_gps_sleep_time += gps_sleep_time;
	t_gps_sleep_cnt += gps_sleep_cnt;

	if (print_log > 0 && buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	if (print_log > 0 && snprintf(buf_p, buf_sz,"[consys_power_state][round:%llu]"
		"conninfra:%u.%03u,%u;wf:%u.%03u,%u;bt:%u.%03u,%u;gps:%u.%03u,%u;"
		"[total]conninfra:%llu.%03llu,%llu;wf:%llu.%03llu,%llu;"
		"bt:%llu.%03llu,%llu;gps:%llu.%03llu,%llu;",
		round,
		CONN_TICK_TO_SEC(conninfra_sleep_time),
		CONN_TICK_TO_SEC((conninfra_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		conninfra_sleep_cnt,
		CONN_TICK_TO_SEC(wf_sleep_time),
		CONN_TICK_TO_SEC((wf_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		wf_sleep_cnt,
		CONN_TICK_TO_SEC(bt_sleep_time),
		CONN_TICK_TO_SEC((bt_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		bt_sleep_cnt,
		CONN_TICK_TO_SEC(gps_sleep_time),
		CONN_TICK_TO_SEC((gps_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		gps_sleep_cnt,
		CONN_TICK_TO_SEC(t_conninfra_sleep_time),
		CONN_TICK_TO_SEC((t_conninfra_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		t_conninfra_sleep_cnt,
		CONN_TICK_TO_SEC(t_wf_sleep_time),
		CONN_TICK_TO_SEC((t_wf_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		t_wf_sleep_cnt,
		CONN_TICK_TO_SEC(t_bt_sleep_time),
		CONN_TICK_TO_SEC((t_bt_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		t_bt_sleep_cnt,
		CONN_TICK_TO_SEC(t_gps_sleep_time),
		CONN_TICK_TO_SEC((t_gps_sleep_time % CONN_32K_TICKS_PER_SEC)* 1000),
		t_gps_sleep_cnt) > 0) {
			pr_info("%s", buf_p);
	}

	/* Power state */
	if (print_log > 0)
		consys_power_state();

	round++;

	/* reset after sleep time is accumulated. */
	consys_reset_power_state();
	return 0;
}

int consys_reset_power_state_mt6983(void)
{
	return consys_power_state_dump(NULL, 0, 0);
}

int consys_power_state_dump_mt6983(char *buf, unsigned int size)
{
	return consys_power_state_dump(buf, size, 1);
}

unsigned int consys_get_hw_ver_mt6983(void)
{
	return CONN_HW_VER;
}

void update_thermal_data_mt6983(struct consys_plat_thermal_data_mt6983* input)
{
	memcpy(&g_consys_plat_therm_data, input, sizeof(struct consys_plat_thermal_data_mt6983));
}

static int calculate_thermal_temperature(int y)
{
	struct consys_plat_thermal_data_mt6983 *data = &g_consys_plat_therm_data;
	int t;
	int const_offset = 30;

	/*  temperature = (y-b)*slope + (offset) */
	/* Postpone division to avoid getting wrong slope becasue of integer division */
	t = (y - (data->thermal_b == 0 ? 0x38 : data->thermal_b)) *
			(data->slop_molecule + 1866) / 1000 + const_offset;

	if (t > PRINT_THERMAL_LOG_THRESHOLD)
		pr_info("y=[%d] b=[%d] constOffset=[%d] [%d] [%d] => t=[%d]\n",
			y, data->thermal_b, const_offset, data->slop_molecule, data->offset, t);
	return t;
}

int consys_thermal_query_mt6983(void)
{
#define THERMAL_DUMP_NUM	11
#define LOG_TMP_BUF_SZ		256
#define TEMP_SIZE		13
#define CONN_GPT2_CTRL_BASE	0x18007000
#define CONN_GPT2_CTRL_AP_EN	0x38

	void __iomem *addr = NULL;
	int cal_val, res = 0;
	/* Base: 0x1800_2000, CONN_TOP_THERM_CTL */
	const unsigned int thermal_dump_crs[THERMAL_DUMP_NUM] = {
		0x00, 0x04, 0x08, 0x0c,
		0x10, 0x14, 0x18, 0x1c,
		0x20, 0x24, 0x28,
	};
	char tmp[TEMP_SIZE] = {'\0'};
	char tmp_buf[LOG_TMP_BUF_SZ] = {'\0'};
	unsigned int i;
	unsigned int efuse0, efuse1, efuse2, efuse3;

	addr = ioremap(CONN_GPT2_CTRL_BASE, 0x100);
	if (addr == NULL) {
		pr_err("GPT2_CTRL_BASE remap fail");
		return -1;
	}

	connsys_adie_top_ck_en_ctl_mt6983(true);

	/* Hold Semaphore, TODO: may not need this, because
		thermal cr seperate for different  */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_THERMAL_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[THERM QRY] Require semaphore fail\n");
		connsys_adie_top_ck_en_ctl_mt6983(false);
		iounmap(addr);
		return -1;
	}

	/* therm cal en */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));
	/* GPT2 En */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0x1);

	/* thermal trigger */
	CONSYS_SET_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 18));
	udelay(500);
	/* get thermal value */
	cal_val = CONSYS_REG_READ(CONN_THERM_CTL_THERMEN3_ADDR);
	cal_val = (cal_val >> 8) & 0x7f;

	/* thermal debug dump */
	efuse0 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0);
	efuse1 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1);
	efuse2 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2);
	efuse3 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3);
	for (i = 0; i < THERMAL_DUMP_NUM; i++) {
		if (snprintf(
			tmp, TEMP_SIZE, "[0x%08x]",
			CONSYS_REG_READ(CONN_REG_CONN_THERM_CTL_ADDR + thermal_dump_crs[i])) >= 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}

	res = calculate_thermal_temperature(cal_val);
	if (res > PRINT_THERMAL_LOG_THRESHOLD)
		pr_info("[%s] efuse:[0x%08x][0x%08x][0x%08x][0x%08x] thermal dump: %s",
			__func__, efuse0, efuse1, efuse2, efuse3, tmp_buf);

	/* GPT2 disable */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_AP_EN, 0);

	/* disable */
	CONSYS_CLR_BIT(CONN_THERM_CTL_THERMEN3_ADDR, (0x1 << 19));

	consys_sema_release_mt6983(CONN_SEMA_THERMAL_INDEX);
	connsys_adie_top_ck_en_ctl_mt6983(false);

	iounmap(addr);

	return res;
}

static unsigned long long consys_soc_timestamp_get_mt6983(void)
{
#define TICK_PER_MS	(13000)
	void __iomem *addr = NULL;
	u32 tick_h = 0, tick_l = 0, tmp_h = 0;
	u64 timestamp = 0;

	/* 0x1c01_1000	sys_timer@13M (VLPSYS)
	 * - 0x0008	CNTCV_L	32	System counter count value low
	 * - 0x000C	CNTCV_H	32	System counter count value high
	 */
	addr = ioremap(0x1c011000, 0x10);
	if (addr) {
		do {
			tick_h = CONSYS_REG_READ(addr + 0x000c);
			tick_l = CONSYS_REG_READ(addr + 0x0008);
			tmp_h = CONSYS_REG_READ(addr + 0x000c);
		} while (tick_h != tmp_h);
		iounmap(addr);
	} else {
		pr_info("[%s] remap fail", __func__);
		return 0;
	}

	timestamp = ((((u64)tick_h << 32) & 0xFFFFFFFF00000000) | ((u64)tick_l & 0x00000000FFFFFFFF));
	do_div(timestamp, TICK_PER_MS);

	return timestamp;
}

static unsigned int consys_adie_detection_mt6983(void)
{
	return ADIE_6637;
}

unsigned int consys_get_adie_chipid_mt6983(void)
{
	return ADIE_6637;
}

static void consys_set_mcu_control_mt6983(int type, bool onoff)
{
	pr_info("[%s] Set mcu control type=[%d] onoff=[%d]\n", __func__, type, onoff);

	if (onoff) // Turn on
		CONSYS_SET_BIT(CONN_INFRA_SYSRAM_SW_CR_MCU_LOG_CONTROL, (0x1 << type));
	else // Turn off
		CONSYS_CLR_BIT(CONN_INFRA_SYSRAM_SW_CR_MCU_LOG_CONTROL, (0x1 << type));
}

static int consys_pre_cal_backup_mt6983(unsigned int offset, unsigned int size)
{
	void __iomem* vir_addr = 0;
	unsigned int expected_size = 0;

	pr_info("[%s] emi base=0x%x offset=0x%x size=%d", __func__, gConEmiPhyBase, offset, size);
	if ((size == 0) || ((offset & 0x3) != 0x0))
		return 1;
	if (mt6637_backup_data != NULL) {
		kfree(mt6637_backup_data);
		mt6637_backup_data = NULL;
	}

	/* Read CR number from EMI */
	vir_addr = ioremap(gConEmiPhyBase + offset, 4);
	if (vir_addr == NULL) {
		pr_err("[%s] ioremap CR number fail", __func__);
		return -ENOMEM;
	}
	mt6637_backup_cr_number = readl(vir_addr);
	iounmap(vir_addr);
	expected_size = sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number + 4;
	if (size < expected_size) {
		pr_err("[%s] cr number=%d, expected_size=0x%x size=0x%x", __func__, mt6637_backup_cr_number, expected_size, size);
		mt6637_backup_cr_number = 0;
		return 1;
	}

	mt6637_backup_data =
		kmalloc(
			sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number,
			GFP_KERNEL);
	if (mt6637_backup_data == NULL) {
		pr_err("[%s] allocate fail");
		return -ENOMEM;
	}
	vir_addr = ioremap(gConEmiPhyBase + offset + 4,
			sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number);
	if (vir_addr == NULL) {
		pr_err("[%s] ioremap data fail", __func__);
		return -ENOMEM;
	}
	memcpy_fromio(mt6637_backup_data, vir_addr,
		sizeof(struct rf_cr_backup_data)*mt6637_backup_cr_number);
	iounmap(vir_addr);

	return 0;
}

int consys_pre_cal_restore_mt6983(void)
{
	int i;

	if (mt6637_backup_cr_number == 0 || mt6637_backup_data == NULL) {
		pr_info("[%s] mt6637_backup_cr_number=%d mt6637_backup_data=%x",
			__func__, mt6637_backup_cr_number, mt6637_backup_data);
		return 1;
	}
	pr_info("[%s] mt6637_backup_cr_number=%d mt6637_backup_data=%x",
		__func__, mt6637_backup_cr_number, mt6637_backup_data);
	/* Acquire semaphore */
	if (consys_sema_acquire_timeout_mt6983(CONN_SEMA_RFSPI_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[%s] Require semaphore fail\n", __func__);
		return CONNINFRA_SPI_OP_FAIL;
	}
	/* Enable a-die top_ck en */
	connsys_adie_top_ck_en_ctl_mt6983(true);
	/* Enable WF clock
	 * ATOP 0xb04 0xfe000000
	 * ATOP 0xb08 0xe0000000
	 * ATOP 0xa04 0xffffffff
	 * ATOP 0xaf4 0xffffffff
	 */
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xb04, 0xfe000000);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xb08, 0xe0000000);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xa04, 0xffffffff);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xaf4, 0xffffffff);
	/* Write CR back, SYS_SPI_WF & SYS_SPI_WF1 */
	for (i = 0; i < mt6637_backup_cr_number; i++) {
		consys_spi_write_nolock_mt6983(
			SYS_SPI_WF,
			mt6637_backup_data[i].addr,
			mt6637_backup_data[i].value1);
	}
	for (i = 0; i < mt6637_backup_cr_number; i++) {
		consys_spi_write_nolock_mt6983(
			SYS_SPI_WF1,
			mt6637_backup_data[i].addr,
			mt6637_backup_data[i].value2);
	}
	/* Disable WF clock
	 * ATOP 0xb04 0x88000000
	 * ATOP 0xb08 0x00000000
	 * ATOP 0xa04 0x00000000
	 * ATOP 0xaf4 0x00000000
	 */
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xb04, 0x88000000);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xb08, 0x00000000);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xa04, 0x00000000);
	consys_spi_write_nolock_mt6983(SYS_SPI_TOP, 0xaf4, 0x00000000);
	/* Release semaphore */
	consys_sema_release_mt6983(CONN_SEMA_RFSPI_INDEX);
	/* Disable a-die top ck en */
	connsys_adie_top_ck_en_ctl_mt6983(false);
	return 0;
}

static int consys_pre_cal_clean_data_mt6983(void)
{

	pr_info("[%s]", __func__);
	if (mt6637_backup_data != NULL) {
		kfree(mt6637_backup_data);
		mt6637_backup_data = NULL;
	}
	mt6637_backup_cr_number = 0;

	return 0;
}
