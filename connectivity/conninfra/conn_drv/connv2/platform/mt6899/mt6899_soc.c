// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of_gpio.h>
#include "../../../../conf/include/conninfra_conf.h"
#include "../../../../include/conninfra.h"
#include "../include/clock_mng.h"
#include "../include/consys_hw.h"
#include "../include/consys_reg_util.h"
#include "../include/plat_library.h"
#include "include/mt6899.h"
#include "include/mt6899_consys_reg_offset.h"
#include "include/mt6899_pos_gen.h"
#include "include/mt6899_soc.h"
#include "include/mt6899_consys_reg.h"

int consys_get_co_clock_type_mt6899(void)
{
	const struct conninfra_conf *conf;
	/* For 6899, clock for connsys is always 52M (RFCK2B).
	 * We don't need to read clock ic register to identify clock rate.
	 */
	int clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_COTMS;
	unsigned char tcxo_gpio = 0;

	/* Default solution */
	conf = conninfra_conf_get_cfg();
	if (NULL == conf)
		pr_notice("[%s] Get conf fail", __func__);
	else
		tcxo_gpio = conf->tcxo_gpio;

	if (tcxo_gpio != 0 || conn_hw_env.tcxo_support)
		clock_type = CONNSYS_CLOCK_SCHEMATIC_52M_EXTCXO;
	pr_info("[%s] conf->tcxo_gpio=%d conn_hw_env.tcxo_support=%d, %s",
		__func__, tcxo_gpio, conn_hw_env.tcxo_support,
		clock_mng_get_schematic_name(clock_type));

	return clock_type;
}

int consys_clk_get_from_dts_mt6899(struct platform_device *pdev)
{
	pm_runtime_enable(&pdev->dev);
	dev_pm_syscore_device(&pdev->dev, true);

	return 0;
}

int consys_clock_buffer_ctrl_mt6899(unsigned int enable)
{
	mapped_addr vir_addr_consys_gen_cksys_reg_base = NULL;

	if (enable) {
	vir_addr_consys_gen_cksys_reg_base =
		ioremap(CONSYS_GEN_CKSYS_REG_BASE_ADDR, 0x1F8);

	if (!vir_addr_consys_gen_cksys_reg_base) {
		pr_notice("vir_addr_consys_gen_cksys_reg_base(%x) ioremap fail\n",
			CONSYS_GEN_CKSYS_REG_BASE_ADDR);
		return -1;
	}

	/* Turn off conn_von_top_host_ck_req_en to disable host_cg_ck always on state */
	CONSYS_SET_BIT(vir_addr_consys_gen_cksys_reg_base +
		0x1F8, (0x1U << 16));
	}

	return 0;
}

int consys_platform_spm_conn_ctrl_mt6899(unsigned int enable)
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

unsigned int consys_soc_chipid_get_mt6899(void)
{
	return PLATFORM_SOC_CHIP;
}

unsigned long long consys_soc_timestamp_get_mt6899(void)
{
#define TICK_PER_MS	(13000)
	void __iomem *addr = NULL;
	u32 tick_h = 0, tick_l = 0, tmp_h = 0;
	u64 timestamp = 0;

	/* 0x1cc1_0000	sys_timer@13M (VLPSYS)
	 * - 0x0008	CNTCV_L	32	System counter count value low
	 * - 0x000C	CNTCV_H	32	System counter count value high
	 */
	addr = ioremap(0x1cc10000, 0x10);
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

	timestamp = ((((u64)tick_h << 32) & 0xFFFFFFFF00000000)
		    | ((u64)tick_l & 0x00000000FFFFFFFF));
	do_div(timestamp, TICK_PER_MS);

	return timestamp;
}

int consys_conninfra_on_power_ctrl_mt6899(unsigned int enable)
{
	int ret = 0;

#if MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE
	ret = consys_platform_spm_conn_ctrl_mt6899(enable);
	mdelay(5);
#else
	ret = consys_conninfra_on_power_ctrl_mt6899_gen(enable);
#endif /* MTK_CONNINFRA_CLOCK_BUFFER_API_AVAILABLE */

	return ret;
}

void consys_set_if_pinmux_mt6899(unsigned int enable,
            unsigned int curr_status, unsigned int next_status)
{
	if (enable) {
		if (curr_status != 0)
			return;
		consys_set_if_pinmux_mt6899_gen(1);
	} else {
		if (next_status != 0)
			return;
		consys_set_if_pinmux_mt6899_gen(0);
	}
}

int consys_is_consys_reg_mt6899(unsigned int addr)
{
	if (addr >= 0x18000000 && addr < 0x19000000)
		return 1;
	return 0;
}

static const char *consys_base_addr_index_to_str[CONSYS_BASE_ADDR_MAX] = {
	"conn_infra_rgu_on",
	"infracfg_ao",
	"GPIO",
	"IOCFG_RT",
	"conn_infra_cfg_on",
	"conn_wt_slp_ctl_reg",
	"conn_infra_bus_cr_on",
	"conn_infra_cfg",
	"conn_infra_clkgen_top",
	"conn_von_bus_bcrm",
	"conn_dbg_ctl",
	"conn_infra_on_bus_bcrm",
	"conn_therm_ctl",
	"conn_afe_ctl",
	"conn_rf_spi_mst_reg",
	"conn_infra_bus_cr",
	"conn_infra_off_debug_ctrl_ao",
	"conn_infra_off_bus_bcrm",
	"conn_infra_sysram_sw_cr",
	"conn_host_csr_top",
	"conn_semaphore",
	"spm",
	"top_rgu",
	"ccif_wf2ap_swirq",
	"ccif_bgf2ap_swirq",
};

int consys_reg_init_mt6899(struct platform_device *pdev)
{
	int ret = -1;
	struct device_node *node = NULL;
	struct consys_reg_base_addr *base_addr = NULL;
	struct resource res;
	int flag, i = 0;

	node = pdev->dev.of_node;
	pr_info("[%s] node=[%p]\n", __func__, node);
	if (node) {
		for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
			base_addr = &conn_reg_mt6899.reg_base_addr[i];

			ret = of_address_to_resource(node, i, &res);
			if (ret) {
				pr_err("Get Reg Index(%d-%s) failed",
						i, consys_base_addr_index_to_str[i]);
				continue;
			}

			base_addr->phy_addr = res.start;
			base_addr->vir_addr =
				(unsigned long) of_iomap(node, i);
			of_get_address(node, i, &(base_addr->size), &flag);

			pr_info("Get Index(%d-%s) phy(0x%zx) baseAddr=(0x%zx) size=(0x%zx)",
				i, consys_base_addr_index_to_str[i], base_addr->phy_addr,
				base_addr->vir_addr, base_addr->size);
		}

	} else {
		pr_err("[%s] can't find CONSYS compatible node\n", __func__);
		return ret;
	}

	return 0;

}

int consys_reg_deinit_mt6899(void)
{
	int i = 0;

	for (i = 0; i < CONSYS_BASE_ADDR_MAX; i++) {
		if (conn_reg_mt6899.reg_base_addr[i].vir_addr) {
			pr_info("[%d] Unmap %s (0x%zx)",
				i, consys_base_addr_index_to_str[i],
				conn_reg_mt6899.reg_base_addr[i].vir_addr);
			iounmap((void __iomem *)conn_reg_mt6899.reg_base_addr[i].vir_addr);
			conn_reg_mt6899.reg_base_addr[i].vir_addr = 0;
		}
	}

	return 0;
}

int consys_factory_testcase_mt6899(char *buf, unsigned int size)
{
	int mode_bk = 0;
	int val_bk = 0;
	int val = 0;
	int ret = 0;
	int num_written;
	char temp_buf[FACTORY_TC_SIZE];
	char *buf_p = temp_buf;
	int buf_sz = FACTORY_TC_SIZE;

	if (buf != NULL && size > 0) {
		buf_p = buf;
		buf_sz = size;
	}

	/* Check conninfra driver status */
	if (consys_check_conninfra_on_domain_status_mt6899() != 0) {
		num_written = snprintf(buf_p, buf_sz, "test fail");
		pr_info("[%s] connsys is power on\n", __func__);
		if (num_written < 0 || num_written >= buf_sz)
			pr_info("[%s] snprintf failed\n", __func__);
		return -1;
	}

	#ifndef CONFIG_FPGA_EARLY_PORTING
		/* Backup GPIO mode */
		mode_bk = CONSYS_REG_READ(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_MODE24_OFFSET_ADDR);

		/* Set CONN_HRST_B(GPIO194) output value high */
		CONSYS_SET_BIT(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_DOUT6_OFFSET_ADDR, (0x1U << 2));
		CONSYS_SET_BIT(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_DIR6_OFFSET_ADDR, (0x1U << 2));

		/* CONN_HRST_B(GPIO194) swtich to GPIO mode */
		CONSYS_REG_WRITE_MASK(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_MODE24_OFFSET_ADDR, 0x0, 0x700);

		/* Turn on SPM clock */
		/* (apply this for SPM's CONNSYS power control related CR accessing) */
		CONSYS_REG_WRITE_MASK(SPM_REG_BASE +
			CONSYS_GEN_POWERON_CONFIG_EN_OFFSET_ADDR, 0xB160001, 0xFFFF0001);

		/* Backup 0x1C001014 value */
		val_bk = CONSYS_REG_READ(SPM_REG_BASE + 0x14);

		/* Set 0x1C001014[0] = 0 */
		CONSYS_REG_WRITE_MASK(SPM_REG_BASE + 0x14, 0x0, 0x1);

		/* Set vcore_io_latch_enb = 1 */
		CONSYS_SET_BIT(SPM_REG_BASE + 0x4, (0x1U << 26));

		/* Set vcore_io_iso = 1 */
		CONSYS_SET_BIT(SPM_REG_BASE + 0x4, (0x1U << 27));

		/* Set CONN_HRST_B(GPIO194) output value low */
		CONSYS_CLR_BIT(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_DOUT6_OFFSET_ADDR, (0x1U << 2));
		CONSYS_CLR_BIT(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_DIR6_OFFSET_ADDR, (0x1U << 2));

		/* Check CONN_HRST_B(GPIO194) should be high*/
		val = CONSYS_REG_READ(GPIO_REG_BASE +0x260);
		pr_info("[%s] CONN_HRST_B = %d\n", __func__, val);
		if (!(val & (0x1 << 2))) {
			pr_info("[%s] factory testcase failed, gpio_hrst_b = 0\n", __func__);
			ret = -1;
		}

		/* Set vcore_io_latch_enb = 0 */
		CONSYS_CLR_BIT(SPM_REG_BASE + 0x4, (0x1U << 26));

		/* Set vcore_io_iso = 0 */
		CONSYS_CLR_BIT(SPM_REG_BASE + 0x4, (0x1U << 27));

		/* Restore the 0x1C001014 setting */
		CONSYS_REG_WRITE(SPM_REG_BASE + 0x14, val_bk);

		/* Restore GPIO mode */
		CONSYS_REG_WRITE_MASK(GPIO_REG_BASE +
			CONSYS_GEN_GPIO_MODE24_OFFSET_ADDR, mode_bk, 0x700);
	#endif

	if (ret < 0)
		num_written = snprintf(buf_p, buf_sz, "test fail");
	else
		num_written = snprintf(buf_p, buf_sz, "test pass");

	if (num_written < 0 || num_written >= buf_sz) {
		pr_info("[%s] snprintf failed\n", __func__);
		return -1;
	}

	pr_info("[%s] factory testcase done\n", __func__);

	return 0;
}

