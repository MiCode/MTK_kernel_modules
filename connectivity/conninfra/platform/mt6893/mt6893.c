// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/memblock.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/types.h>

#include <connectivity_build_in_adapter.h>

#include "osal.h"
#include "conninfra.h"
#include "conninfra_conf.h"
#include "consys_hw.h"
#include "consys_reg_mng.h"
#include "consys_reg_util.h"
#include "mt6893.h"
#include "emi_mng.h"
#include "mt6893_emi.h"
#include "mt6893_consys_reg.h"
#include "mt6893_consys_reg_offset.h"
#include "mt6893_pos.h"
#include "clock_mng.h"

#if COMMON_KERNEL_CLK_SUPPORT
#include <linux/pm_runtime.h>
#include <linux/pm_wakeup.h>
#else
#include <mtk_clkbuf_ctl.h>
#endif

#include "mt6893_connsyslog.h"
#include "coredump_mng.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CONSYS_PWR_SPM_CTRL 1
#define PLATFORM_SOC_CHIP 0x6893

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

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int consys_clk_get_from_dts(struct platform_device *pdev);
static int consys_clk_detach(void);
static int consys_clock_buffer_ctrl(unsigned int enable);
static unsigned int consys_soc_chipid_get(void);
static unsigned int consys_get_hw_ver(void);
static void consys_clock_fail_dump(void);
static int consys_thermal_query(void);
static int consys_power_state(char *buf, unsigned int size);
static int consys_bus_clock_ctrl(enum consys_drv_type, unsigned int, int);
static unsigned long long consys_soc_timestamp_get(void);
static unsigned int consys_adie_detection_mt6893(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
struct consys_hw_ops_struct g_consys_hw_ops_mt6893 = {
	/* load from dts */
	/* TODO: mtcmos should move to a independent module */
	.consys_plt_clk_get_from_dts = consys_clk_get_from_dts,
	.consys_plt_clk_detach = consys_clk_detach,

	/* clock */
	.consys_plt_clock_buffer_ctrl = consys_clock_buffer_ctrl,
	.consys_plt_co_clock_type = consys_co_clock_type_mt6893,

	/* POS */
	.consys_plt_conninfra_on_power_ctrl = consys_conninfra_on_power_ctrl_mt6893,
	.consys_plt_set_if_pinmux = consys_set_if_pinmux_mt6893,

	.consys_plt_polling_consys_chipid = consys_polling_chipid_mt6893,
	.consys_plt_d_die_cfg = connsys_d_die_cfg_mt6893,
	.consys_plt_spi_master_cfg = connsys_spi_master_cfg_mt6893,
	.consys_plt_a_die_cfg = connsys_a_die_cfg_mt6893,
	.consys_plt_afe_wbg_cal = connsys_afe_wbg_cal_mt6893,
	.consys_plt_subsys_pll_initial = connsys_subsys_pll_initial_mt6893,
	.consys_plt_low_power_setting = connsys_low_power_setting_mt6893,
	.consys_plt_soc_chipid_get = consys_soc_chipid_get,
	.consys_plt_conninfra_wakeup = consys_conninfra_wakeup_mt6893,
	.consys_plt_conninfra_sleep = consys_conninfra_sleep_mt6893,
	.consys_plt_is_rc_mode_enable = consys_is_rc_mode_enable_mt6893,

	/* debug */
	.consys_plt_clock_fail_dump = consys_clock_fail_dump,
	.consys_plt_get_hw_ver = consys_get_hw_ver,

	.consys_plt_spi_read = consys_spi_read_mt6893,
	.consys_plt_spi_write = consys_spi_write_mt6893,
	.consys_plt_spi_update_bits = consys_spi_update_bits_mt6893,
	.consys_plt_adie_top_ck_en_on = consys_adie_top_ck_en_on_mt6893,
	.consys_plt_adie_top_ck_en_off = consys_adie_top_ck_en_off_mt6893,
	.consys_plt_spi_clock_switch = consys_spi_clock_switch_mt6893,
	.consys_plt_subsys_status_update = consys_subsys_status_update_mt6893,

	.consys_plt_thermal_query = consys_thermal_query,
	.consys_plt_power_state = consys_power_state,
	.consys_plt_config_setup = consys_config_setup_mt6893,
	.consys_plt_bus_clock_ctrl = consys_bus_clock_ctrl,
	.consys_plt_soc_timestamp_get = consys_soc_timestamp_get,
	.consys_plt_adie_detection = consys_adie_detection_mt6893,
};

#if (!COMMON_KERNEL_CLK_SUPPORT)
static struct clk *clk_scp_conn_main;	/*ctrl conn_power_on/off */
#endif

static struct consys_plat_thermal_data_mt6893 g_consys_plat_therm_data;

extern struct consys_hw_ops_struct g_consys_hw_ops_mt6893;
extern struct consys_reg_mng_ops g_dev_consys_reg_ops_mt6893;
extern struct consys_platform_emi_ops g_consys_platform_emi_ops_mt6893;
extern struct consys_platform_pmic_ops g_consys_platform_pmic_ops_mt6893;
extern struct consys_platform_coredump_ops g_consys_platform_coredump_ops_mt6893;

struct conninfra_plat_data mt6893_plat_data = {
	.chip_id = PLATFORM_SOC_CHIP,
	.consys_hw_version = CONN_HW_VER,
	.hw_ops = &g_consys_hw_ops_mt6893,
	.reg_ops = &g_dev_consys_reg_ops_mt6893,
	.platform_emi_ops = &g_consys_platform_emi_ops_mt6893,
	.platform_pmic_ops = &g_consys_platform_pmic_ops_mt6893,
	.platform_coredump_ops = &g_consys_platform_coredump_ops_mt6893,
	.connsyslog_config = &g_connsyslog_config,
};

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

/* mtcmos contorl */
int consys_clk_get_from_dts(struct platform_device *pdev)
{
#if COMMON_KERNEL_CLK_SUPPORT
	pm_runtime_enable(&pdev->dev);
	dev_pm_syscore_device(&pdev->dev, true);
#else
	clk_scp_conn_main = devm_clk_get(&pdev->dev, "conn");
	if (IS_ERR(clk_scp_conn_main)) {
		pr_err("[CCF]cannot get clk_scp_conn_main clock.\n");
		return PTR_ERR(clk_scp_conn_main);
	}
	pr_debug("[CCF]clk_scp_conn_main=%p\n", clk_scp_conn_main);
#endif
	return 0;
}


int consys_clk_detach(void)
{
#if COMMON_KERNEL_CLK_SUPPORT
	struct platform_device *pdev = get_consys_device();

	if (pdev == NULL)
		return 0;
	pm_runtime_disable(&pdev->dev);
#endif
	return 0;
}


int consys_platform_spm_conn_ctrl_mt6893(unsigned int enable)
{
	int ret = 0;
#if COMMON_KERNEL_CLK_SUPPORT
	struct platform_device *pdev = get_consys_device();

	if (!pdev) {
		pr_info("get_consys_device fail.\n");
		return -1;
	}
#endif

	if (enable) {
#if COMMON_KERNEL_CLK_SUPPORT
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
#else
		ret = clk_prepare_enable(clk_scp_conn_main);
#endif
		if (ret) {
			pr_info("Turn on conn_infra power fail. Ret=%d\n", ret);
			return -1;
		}
	} else {
#if COMMON_KERNEL_CLK_SUPPORT
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
#else
		clk_disable_unprepare(clk_scp_conn_main);

#endif
	}

	return ret;
}

int consys_clock_buffer_ctrl(unsigned int enable)
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

int consys_co_clock_type_mt6893(void)
{
	const struct conninfra_conf *conf;

	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (NULL == conf) {
		pr_err("[%s] Get conf fail", __func__);
		return -1;
	}
	/* TODO: for co-clock mode, there are two case: 26M and 52M. Need something to distinguish it. */
	if (conf->tcxo_gpio != 0)
		return CONNSYS_CLOCK_SCHEMATIC_26M_EXTCXO;
	else
		return CONNSYS_CLOCK_SCHEMATIC_26M_COTMS;
}

unsigned int consys_soc_chipid_get(void)
{
	return PLATFORM_SOC_CHIP;
}

unsigned int consys_get_hw_ver(void)
{
	return CONN_HW_VER;
}

void consys_clock_fail_dump(void)
{
	pr_info("[%s]", __func__);
}


void update_thermal_data_mt6893(struct consys_plat_thermal_data_mt6893* input)
{
	memcpy(&g_consys_plat_therm_data, input, sizeof(struct consys_plat_thermal_data_mt6893));
	/* Special factor, not in POS */
	/* THERMCR1 [16:17]*/
	CONSYS_REG_WRITE(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERMCR1,
			(CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERMCR1) |
				(0x3 << 16)));

}

static int calculate_thermal_temperature(int y)
{
	struct consys_plat_thermal_data_mt6893 *data = &g_consys_plat_therm_data;
	int t;
	int const_offset = 25;

	/*
	 *    MT6635 E1 : read 0x02C = 0x66358A00
	 *    MT6635 E2 : read 0x02C = 0x66358A10
	 *    MT6635 E3 : read 0x02C = 0x66358A11
	 */
	if (conn_hw_env.adie_hw_version == 0x66358A10 ||
		conn_hw_env.adie_hw_version == 0x66358A11)
		const_offset = 28;

	/* temperature = (y-b)*slope + (offset) */
	/* TODO: offset + 25 : this is only for E1, E2 is 28 */
	t = (y - (data->thermal_b == 0 ? 0x36 : data->thermal_b)) *
			((data->slop_molecule + 209) / 100) + (data->offset + const_offset);

	pr_info("y=[%d] b=[%d] constOffset=[%d] [%d] [%d] => t=[%d]\n",
			y, data->thermal_b, const_offset, data->slop_molecule, data->offset,
			t);

	return t;
}

int consys_thermal_query(void)
{
#define THERMAL_DUMP_NUM	11
#define LOG_TMP_BUF_SZ		256
#define TEMP_SIZE		13
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

	consys_adie_top_ck_en_on_mt6893(CONNSYS_ADIE_CTL_HOST_CONNINFRA);

	/* Hold Semaphore, TODO: may not need this, because
		thermal cr seperate for different  */
	if (consys_sema_acquire_timeout_mt6893(CONN_SEMA_THERMAL_INDEX, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL) {
		pr_err("[THERM QRY] Require semaphore fail\n");
		consys_adie_top_ck_en_off_mt6893(CONNSYS_ADIE_CTL_HOST_CONNINFRA);
		iounmap(addr);
		return -1;
	}

	/* therm cal en */
	CONSYS_REG_WRITE(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN,
			(CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN) |
				(0x1 << 19)));
	/* GPT2 En */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_THERMAL_EN,
			(CONSYS_REG_READ(addr + CONN_GPT2_CTRL_THERMAL_EN) |
			0x1));

	/* thermal trigger */
	CONSYS_REG_WRITE(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN,
			(CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN) |
				(0x1 << 18)));
	udelay(500);
	/* get thermal value */
	cal_val = CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN);
	cal_val = (cal_val >> 8) & 0x7f;

	/* thermal debug dump */
	efuse0 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_0);
	efuse1 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_1);
	efuse2 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_2);
	efuse3 = CONSYS_REG_READ(CONN_INFRA_SYSRAM_BASE_ADDR + CONN_INFRA_SYSRAM_SW_CR_A_DIE_EFUSE_DATA_3);
	for (i = 0; i < THERMAL_DUMP_NUM; i++) {
		if (snprintf(
			tmp, TEMP_SIZE, "[0x%08x]",
			CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + thermal_dump_crs[i])) > 0)
			strncat(tmp_buf, tmp, strlen(tmp));
	}
	pr_info("[%s] efuse:[0x%08x][0x%08x][0x%08x][0x%08x] thermal dump: %s",
		__func__, efuse0, efuse1, efuse2, efuse3, tmp_buf);

	res = calculate_thermal_temperature(cal_val);

	/* GPT2 disable, no effect on 6893 */
	CONSYS_REG_WRITE(addr + CONN_GPT2_CTRL_THERMAL_EN,
			(CONSYS_REG_READ(addr + CONN_GPT2_CTRL_THERMAL_EN) &
				~(0x1)));

	/* disable */
	CONSYS_REG_WRITE(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN,
			(CONSYS_REG_READ(CONN_TOP_THERM_CTL_ADDR + CONN_TOP_THERM_CTL_THERM_CAL_EN) &
				~(0x1 << 19)));

	consys_sema_release_mt6893(CONN_SEMA_THERMAL_INDEX);
	consys_adie_top_ck_en_off_mt6893(CONNSYS_ADIE_CTL_HOST_CONNINFRA);

	iounmap(addr);

	return res;
}


int consys_power_state(char *buf, unsigned int size)
{
	const char* osc_str[] = {
		"fm ", "gps ", "bgf ", "wf ", "ap2conn ", "conn_thm ", "conn_pta ", "conn_infra_bus "
	};
	char temp_buf[256] = {'\0'};
	int r = CONSYS_REG_READ(CON_REG_HOST_CSR_ADDR + CONN_HOST_CSR_DBG_DUMMY_2);
	int i;

	for (i = 0; i < 8; i++) {
		if ((r & (0x1 << (18 + i))) > 0)
			strncat(temp_buf, osc_str[i], strlen(osc_str[i]));
	}
	pr_info("[%s] [0x%x] %s", __func__, r, temp_buf);
	return 0;
}

int consys_bus_clock_ctrl(enum consys_drv_type drv_type, unsigned int bus_clock, int status)
{
	static unsigned int conninfra_bus_clock_wpll_state = 0;
	static unsigned int conninfra_bus_clock_bpll_state = 0;
	unsigned int wpll_state = conninfra_bus_clock_wpll_state;
	unsigned int bpll_state = conninfra_bus_clock_bpll_state;
	bool wpll_switch = false, bpll_switch = false;
	int check;

	if (status) {
		/* Turn on */
		/* Enable BPLL */
		if (bus_clock & CONNINFRA_BUS_CLOCK_BPLL) {

			if (conninfra_bus_clock_bpll_state == 0) {
				bpll_switch = true;
			}
			conninfra_bus_clock_bpll_state |= (0x1 << drv_type);
		}
		/* Enable WPLL */
		if (bus_clock & CONNINFRA_BUS_CLOCK_WPLL) {
			if (conninfra_bus_clock_wpll_state == 0) {
				wpll_switch = true;
			}
			conninfra_bus_clock_wpll_state |= (0x1 << drv_type);
		}

		if (bpll_switch || wpll_switch) {
			while (consys_sema_acquire_timeout_mt6893(CONN_SEMA_BUS_CONTROL, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL);

			if (bpll_switch) {
				CONSYS_SET_BIT(CONN_AFE_CTL_BASE_ADDR + CONN_AFE_CTL_RG_DIG_EN_03, (0x1 << 21));
				udelay(30);
			}

			if (wpll_switch) {
				CONSYS_SET_BIT(CONN_AFE_CTL_BASE_ADDR + CONN_AFE_CTL_RG_DIG_EN_03, (0x1 << 20));
				udelay(50);
			}

			consys_sema_release_mt6893(CONN_SEMA_BUS_CONTROL);
		}

		pr_info("drv=[%d] conninfra_bus_clock_wpll=[%u]->[%u] %s conninfra_bus_clock_bpll=[%u]->[%u] %s",
			drv_type,
			wpll_state, conninfra_bus_clock_wpll_state, (wpll_switch ? "enable" : ""),
			bpll_state, conninfra_bus_clock_bpll_state, (bpll_switch ? "enable" : ""));
	} else {
		/* Turn off */
		/* Turn off WPLL */
		if (bus_clock & CONNINFRA_BUS_CLOCK_WPLL) {
			conninfra_bus_clock_wpll_state &= ~(0x1<<drv_type);
			if (conninfra_bus_clock_wpll_state == 0) {
				wpll_switch = true;
			}
		}
		/* Turn off BPLL */
		if (bus_clock & CONNINFRA_BUS_CLOCK_BPLL) {
			conninfra_bus_clock_bpll_state &= ~(0x1<<drv_type);
			if (conninfra_bus_clock_bpll_state == 0) {
				bpll_switch = true;
			}
		}

		if (bpll_switch || wpll_switch) {
			while (consys_sema_acquire_timeout_mt6893(CONN_SEMA_BUS_CONTROL, CONN_SEMA_TIMEOUT) == CONN_SEMA_GET_FAIL);

			if (wpll_switch) {
				CONSYS_CLR_BIT(CONN_AFE_CTL_BASE_ADDR + CONN_AFE_CTL_RG_DIG_EN_03, (0x1 << 20));
			}

			if (bpll_switch) {
				CONSYS_CLR_BIT(CONN_AFE_CTL_BASE_ADDR + CONN_AFE_CTL_RG_DIG_EN_03, (0x1 << 21));
			}

			consys_sema_release_mt6893(CONN_SEMA_BUS_CONTROL);
		}

		pr_info("drv=[%d] conninfra_bus_clock_wpll=[%u]->[%u] %s conninfra_bus_clock_bpll=[%u]->[%u] %s",
			drv_type,
			wpll_state, conninfra_bus_clock_wpll_state, (wpll_switch ? "disable" : ""),
			bpll_state, conninfra_bus_clock_bpll_state, (bpll_switch ? "disable" : ""));
		if (consys_reg_mng_reg_readable() == 0) {
			check = consys_reg_mng_is_bus_hang();
			pr_info("[%s] not readable, bus hang check=[%d]", __func__, check);
		}
	}
	return 0;
}

static unsigned long long consys_soc_timestamp_get(void)
{
#define TICK_PER_MS	(13000)
	void __iomem *addr = NULL;
	u32 tick_h = 0, tick_l = 0, tmp_h = 0;
	u64 timestamp = 0;

	/* 0x1001_7000	sys_timer@13M
	 * - 0x0008	CNTCV_L	32	System counter count value low
	 * - 0x000C	CNTCV_H	32	System counter count value high
	 */
	addr = ioremap(0x10017000, 0x10);
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

static unsigned int consys_adie_detection_mt6893(void)
{
	return 0x6635;
}
