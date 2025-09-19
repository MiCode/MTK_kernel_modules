// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <linux/io.h>
#include "consys_reg_mng.h"
#include "consys_reg_util.h"

const struct consys_reg_mng_ops* g_consys_reg_ops = NULL;

int consys_reg_mng_print_log(int level)
{
	int ret = -1;

	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_platform_log)
		g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_platform_log();
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_pmic_log)
		g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_pmic_log();
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_log)
		ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_log(level);
	return ret;
}

/*
 * Get register status
 * Return:
 * 1: Readable
 * 0: Not readable
 * Print log if it is not readable
 */
int consys_reg_mng_reg_readable(void)
{
	int ret;

	if (g_consys_reg_ops == NULL) {
		pr_err("%s not implement", __func__);
		BUG_ON(1);
	}

	/* Case of implement all checks in single platform function */
	if (g_consys_reg_ops->consys_reg_mng_check_reable)
		return g_consys_reg_ops->consys_reg_mng_check_reable();

	/* Case of implement all checks by individual functions */
	/* Check Power ON domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status == NULL) {
		pr_err("%s not implement conninfra on domain check\n", __func__);
		BUG_ON(1);
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY);
		return 0;
	}
	/* Check Power OFF domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status == NULL) {
		pr_err("%s not implement conninfra off domain check\n", __func__);
		BUG_ON(1);
	}
	/* Separate bus_clock_check from conninfra_off_status_check */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status) {
		ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status();
		if (ret != 0) {
			consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);
			return 0;
		}
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);
		return 0;
	}
	/* Check IRQ status */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq == NULL) {
		pr_err("%s not implement conninfra irq check\n", __func__);
		BUG_ON(1);
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF);
		return 0;
	}
	return 1;
}

/*
 * Get register status for coredump
 * Return:
 * 1: Connsys Power on domain and Power off domain are both enabled
 * 0: Connsys Power on domain and Power off domain are both disabled
 * Print log if it is not readable
 */
int consys_reg_mng_reg_readable_for_coredump(void)
{
	int ret;

	if (g_consys_reg_ops == NULL) {
		pr_err("%s not implement", __func__);
		BUG_ON(1);
	}

	/* Case of implement all checks in single platform function */
	if (g_consys_reg_ops->consys_reg_mng_check_reable_for_coredump)
		return g_consys_reg_ops->consys_reg_mng_check_reable_for_coredump();

	/* Case of implement all checks by individual functions */
	/* Check Power On domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status == NULL) {
		pr_err("%s not implement conninfra on domain check\n", __func__);
		BUG_ON(1);
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY);
		return 0;
	}
	/* Check Power OFF domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status == NULL) {
		pr_err("%s not implement conninfra off domain check\n", __func__);
		BUG_ON(1);
	}
	/* Separate bus_clock_check from conninfra_off_statsu_check */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status) {
		ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status();
		if (ret != 0) {
			consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);
			return 0;
		}
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);
		return 0;
	}
	/* Check IRQ status */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq == NULL) {
		pr_err("%s not implement conninfra irq check\n", __func__);
		BUG_ON(1);
	}
	ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq();
	if (ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF);
	}
	return 1;
}

int consys_reg_mng_is_connsys_reg(phys_addr_t addr)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_is_consys_reg)
		return g_consys_reg_ops->consys_reg_mng_is_consys_reg(addr);
	return -1;
}

/*
 * Get bus hang status
 * Return:
 * Error Code: Bus hang reason
 * 0: bus is okay
 * Always print log with bus status
 */
int consys_reg_mng_is_bus_hang(void)
{
	int fp_ret;
	int ret = 0;
	int wakeup_conninfra = 0;

	if (g_consys_reg_ops == NULL) {
		pr_err("%s not implement", __func__);
		BUG_ON(1);
	}

	/* Case of implement all checks in single platform function */
	if (g_consys_reg_ops->consys_reg_mng_is_bus_hang)
		return g_consys_reg_ops->consys_reg_mng_is_bus_hang();

	/* Case of implement all checks by individual functions */
	/* Check Power On domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status == NULL) {
		pr_err("%s not implement conninfra on domain check\n", __func__);
		BUG_ON(1);
	}
	fp_ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_on_status();
	if (fp_ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_HOST_ONLY);
		return fp_ret;
	}
	/* Check Power OFF domain */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status == NULL) {
		pr_err("%s not implement conninfra off domain check\n", __func__);
		BUG_ON(1);
	}
	/* Separate bus_clock_check from conninfra_off_statsu_check */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status) {
		ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_bus_clock_status();
		if (ret != 0) {
			consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);
			return ret;
		}
	}
	fp_ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_off_status();
	if (fp_ret != 0) {
		consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_ON);

		if (consys_hw_force_conninfra_wakeup() == 0)
			wakeup_conninfra = 1;
		else
			return fp_ret;
		ret = fp_ret;
	}
	/* Check IRQ status */
	if (g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq == NULL) {
		pr_err("%s not implement conninfra irq check\n", __func__);
		BUG_ON(1);
	}
	fp_ret = g_consys_reg_ops->consys_reg_mng_check_readable_conninfra_irq();
	ret |= fp_ret;
	consys_reg_mng_print_log(CONNINFRA_BUS_LOG_LEVEL_CONNINFRA_OFF);

	if (wakeup_conninfra)
		consys_hw_force_conninfra_sleep();

	return ret;
}

int consys_reg_mng_dump_bus_status(void)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_dump_bus_status)
		return g_consys_reg_ops->consys_reg_mng_dump_bus_status();
	return -1;
}

int consys_reg_mng_dump_conninfra_status(void)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_dump_conninfra_status)
		return g_consys_reg_ops->consys_reg_mng_dump_conninfra_status();
	return -1;
}

int consys_reg_mng_dump_cpupcr(enum conn_dump_cpupcr_type dump_type, int times, unsigned long interval_us)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_dump_cpupcr)
		return g_consys_reg_ops->consys_reg_mng_dump_cpupcr(dump_type, times, interval_us);
	return -1;
}

int consys_reg_mng_init(struct platform_device *pdev, const struct conninfra_plat_data* plat_data)
{
	int ret = 0;
	if (g_consys_reg_ops == NULL)
		g_consys_reg_ops = (const struct consys_reg_mng_ops*)plat_data->reg_ops;

	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_init) {
		ret = g_consys_reg_ops->consys_reg_mng_init(pdev);
		if (g_consys_reg_ops->consys_reg_mng_debug_init)
			g_consys_reg_ops->consys_reg_mng_debug_init();
	} else
		ret = EFAULT;

	return ret;
}

int consys_reg_mng_deinit(void)
{
	if (g_consys_reg_ops&&
		g_consys_reg_ops->consys_reg_mng_deinit) {
		g_consys_reg_ops->consys_reg_mng_deinit();
		if (g_consys_reg_ops->consys_reg_mng_debug_deinit)
			g_consys_reg_ops->consys_reg_mng_debug_deinit();
	}

	return 0;
}

int consys_reg_mng_reg_read(unsigned long addr, unsigned int *value, unsigned int mask)
{
	void __iomem *vir_addr = NULL;

	vir_addr = ioremap(addr, 0x100);
	if (!vir_addr) {
		pr_err("ioremap fail");
		return -1;
	}

	*value = (unsigned int)CONSYS_REG_READ(vir_addr) & mask;

	pr_info("[%x] mask=[%x]", *value, mask);

	iounmap(vir_addr);
	return 0;
}

int consys_reg_mng_reg_write(unsigned long addr, unsigned int value, unsigned int mask)
{
	void __iomem *vir_addr = NULL;

	vir_addr = ioremap(addr, 0x100);
	if (!vir_addr) {
		pr_err("ioremap fail");
		return -1;
	}

	CONSYS_REG_WRITE_MASK(vir_addr, value, mask);

	iounmap(vir_addr);
	return 0;
}


int consys_reg_mng_is_host_csr(unsigned long addr)
{
	if (g_consys_reg_ops &&
		g_consys_reg_ops->consys_reg_mng_is_host_csr)
		return g_consys_reg_ops->consys_reg_mng_is_host_csr(addr);
	return -1;
}
