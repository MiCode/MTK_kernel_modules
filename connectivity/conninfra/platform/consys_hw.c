/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/suspend.h>

#include <connectivity_build_in_adapter.h>

#include "osal.h"
#include "consys_hw.h"
#include "emi_mng.h"
#include "pmic_mng.h"
#include "clock_mng.h"
#include "consys_reg_mng.h"
#include "connsys_debug_utility.h"
#include "coredump_mng.h"
#include "conn_power_throttling.h"
#include "aee.h"

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/


/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
enum conninfra_pwr_on_rollback_type
{
	CONNINFRA_PWR_ON_PMIC_ON_FAIL,
	CONNINFRA_PWR_ON_CONNINFRA_HW_POWER_FAIL,
	CONNINFRA_PWR_ON_POLLING_CHIP_ID_FAIL,
	CONNINFRA_PWR_ON_A_DIE_FAIL,
	CONNINFRA_PWR_ON_MAX
};
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

static int mtk_conninfra_probe(struct platform_device *pdev);
static int mtk_conninfra_remove(struct platform_device *pdev);
static int mtk_conninfra_suspend(void);
static int mtk_conninfra_resume(void);
static int get_consys_platform_ops(struct platform_device *pdev);

static int _consys_hw_conninfra_wakeup(void);
static void _consys_hw_conninfra_sleep(void);
/* drv_type: who want to raise voltage
 * raise: upgrade(1) or downgrad(0) voltage
 * onoff: the request is send during power on/off duration
 * 	1: Yes, the request is send during power on/off
 * 	0: No, the request is from sub-radio
 */
static int _consys_hw_raise_voltage(enum consys_drv_type drv_type, bool raise, bool onoff);
static void _consys_hw_conninfra_print_wakeup_record(void);

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

extern const struct of_device_id apconninfra_of_ids[];

static struct platform_driver mtk_conninfra_dev_drv = {
	.probe = mtk_conninfra_probe,
	.remove = mtk_conninfra_remove,
	.driver = {
		   .name = "mtk_conninfra",
		   .owner = THIS_MODULE,
#ifdef CONFIG_OF
		   .of_match_table = apconninfra_of_ids,
#endif
		   .probe_type = PROBE_FORCE_SYNCHRONOUS,
		   },
};


struct consys_hw_env conn_hw_env;

const struct consys_hw_ops_struct *consys_hw_ops;
struct platform_device *g_pdev;

static int g_conninfra_wakeup_ref_cnt;

#define CONNINFRA_WAKEUP_RECORD_NUM 6
static int g_conninfra_wakeup_rec_idx;
static unsigned long long g_conninfra_wakeup_rec_sec[CONNINFRA_WAKEUP_RECORD_NUM];
static unsigned long g_conninfra_wakeup_rec_nsec[CONNINFRA_WAKEUP_RECORD_NUM];
static int g_conninfra_wakeup_rec_cnt[CONNINFRA_WAKEUP_RECORD_NUM];

struct work_struct ap_resume_work;

struct conninfra_dev_cb *g_conninfra_dev_cb;
const struct conninfra_plat_data *g_conninfra_plat_data = NULL;

struct pinctrl *g_conninfra_pinctrl_ptr = NULL;

static unsigned int g_adie_chipid = 0;
static OSAL_SLEEPABLE_LOCK g_adie_chipid_lock;

/* this config is defined by each platform, used to change setting by dbg command. */
/* MT6983: for sleep mode control */
static int g_platform_config;

static struct notifier_block conninfra_pm_notifier;

static atomic_t g_hw_init_done = ATOMIC_INIT(0);
/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/

struct platform_device *get_consys_device(void)
{
	return g_pdev;
}

int consys_hw_get_clock_schematic(void)
{
	if (consys_hw_ops->consys_plt_co_clock_type)
		return consys_hw_ops->consys_plt_co_clock_type();
	else
		pr_err("consys_hw_ops->consys_co_clock_type not supported\n");

	return -1;
}

unsigned int consys_hw_chipid_get(void)
{
	if (consys_hw_ops->consys_plt_soc_chipid_get)
		return consys_hw_ops->consys_plt_soc_chipid_get();
	else
		pr_err("consys_plt_soc_chipid_get not supported\n");

	return 0;
}

unsigned int consys_hw_get_hw_ver(void)
{
	if (consys_hw_ops->consys_plt_get_hw_ver)
		return consys_hw_ops->consys_plt_get_hw_ver();
	return 0;
}


int consys_hw_reg_readable(void)
{
	return consys_reg_mng_reg_readable();
}

int consys_hw_reg_readable_for_coredump(void)
{
	return consys_reg_mng_reg_readable_for_coredump();
}

int consys_hw_is_connsys_reg(phys_addr_t addr)
{
	return consys_reg_mng_is_connsys_reg(addr);
}

int consys_hw_is_bus_hang(void)
{
	return consys_reg_mng_is_bus_hang();
}

int consys_hw_dump_bus_status(void)
{
	return consys_reg_mng_dump_bus_status();
}

int consys_hw_dump_cpupcr(enum conn_dump_cpupcr_type dump_type, int times, unsigned long interval_us)
{
	return consys_reg_mng_dump_cpupcr(dump_type, times, interval_us);
}

int consys_hw_pwr_off(unsigned int curr_status, unsigned int off_radio)
{
	unsigned int next_status = curr_status & ~(0x1 << off_radio);
	int ret = 0;

	if (next_status == 0) {
		pr_info("Last power off: %d, Power off CONNSYS PART 1\n", off_radio);
		consys_hw_raise_voltage(off_radio, false, true);
		if (consys_hw_ops->consys_plt_conninfra_on_power_ctrl)
			consys_hw_ops->consys_plt_conninfra_on_power_ctrl(0);
		pr_info("Power off CONNSYS PART 2\n");
		if (consys_hw_ops->consys_plt_set_if_pinmux)
			consys_hw_ops->consys_plt_set_if_pinmux(0);
		if (consys_hw_ops->consys_plt_clock_buffer_ctrl)
			consys_hw_ops->consys_plt_clock_buffer_ctrl(0);
		ret = pmic_mng_common_power_ctrl(0);
		if (ret)
			pr_info("Power off a-die power, ret=%d\n", ret);
	} else {
		pr_info("[%s] Part 0: only subsys (%d) off (curr_status=0x%x, next_status = 0x%x)\n",
			__func__, off_radio, curr_status, next_status);
		ret = _consys_hw_conninfra_wakeup();
		if (consys_hw_ops->consys_plt_subsys_status_update)
			consys_hw_ops->consys_plt_subsys_status_update(false, off_radio);
		_consys_hw_raise_voltage(off_radio, false, true);
		if (consys_hw_ops->consys_plt_spi_master_cfg)
			consys_hw_ops->consys_plt_spi_master_cfg(next_status);
		if (consys_hw_ops->consys_plt_low_power_setting)
			consys_hw_ops->consys_plt_low_power_setting(curr_status, next_status);
		if (ret == 0)
			_consys_hw_conninfra_sleep();
	}

	return 0;
}

int _consys_hw_pwr_on_rollback(enum conninfra_pwr_on_rollback_type type)
{
	int ret;

	switch (type) {
		case CONNINFRA_PWR_ON_A_DIE_FAIL:
		case CONNINFRA_PWR_ON_POLLING_CHIP_ID_FAIL:
			ret = consys_hw_is_bus_hang();
			consys_hw_clock_fail_dump();
			if (ret)
				pr_info("Conninfra bus error, code=%d", ret);
		case CONNINFRA_PWR_ON_CONNINFRA_HW_POWER_FAIL:
			if (consys_hw_ops->consys_plt_conninfra_on_power_ctrl) {
				ret = consys_hw_ops->consys_plt_conninfra_on_power_ctrl(0);
				if (ret)
					pr_err("[%s] turn off hw power fail, ret=%d\n", __func__, ret);
			}
		case CONNINFRA_PWR_ON_PMIC_ON_FAIL:
			ret = pmic_mng_common_power_ctrl(0);
			if (ret)
				pr_err("[%s] turn off VCN control fail, ret=%d\n", __func__, ret);
			break;
		default:
			pr_notice("[%s] wrong type: %u", __func__, type);
			break;
	}
	return 0;
}

unsigned int consys_hw_get_ic_info(enum connsys_ic_info_type type)
{
	if (type == CONNSYS_SOC_CHIPID) {
		return consys_hw_chipid_get();
	} else if (type == CONNSYS_HW_VER) {
		return consys_hw_get_hw_ver();
	} else if (type == CONNSYS_ADIE_CHIPID) {
		return g_adie_chipid;
	}

	return 0;
}

unsigned int consys_hw_detect_adie_chipid(void)
{
	int chipid = 0;

	if (osal_lock_sleepable_lock(&g_adie_chipid_lock))
		return 0;

	/* detect a-die only once */
	if (g_adie_chipid) {
		osal_unlock_sleepable_lock(&g_adie_chipid_lock);
		return g_adie_chipid;
	}

	if (consys_hw_ops->consys_plt_adie_detection) {
		chipid = consys_hw_ops->consys_plt_adie_detection();

		if (chipid > 0) {
			g_adie_chipid = chipid;
			pr_info("A-die chipid detection done, found chipid=[%x]\n", chipid);
		} else
			pr_info("Fail to detect a-die chipid, found chipid=[%x]\n", chipid);
	}

	osal_unlock_sleepable_lock(&g_adie_chipid_lock);

	return g_adie_chipid;
}

int consys_hw_pwr_on(unsigned int curr_status, unsigned int on_radio)
{
	int ret;
	unsigned int next_status = (curr_status | (0x1 << on_radio));

	/* first power on */
	if (curr_status == 0) {
		/* POS PART 1:
		 * Set PMIC to turn on the power that AFE WBG circuit in D-die,
		 * OSC or crystal component, and A-die need.
		 */
		ret = pmic_mng_common_power_ctrl(1);
		if (consys_hw_ops->consys_plt_clock_buffer_ctrl)
			consys_hw_ops->consys_plt_clock_buffer_ctrl(1);
		if (ret) {
			pr_err("[%s] turn on PMIC error, ret=%d\n", __func__, ret);
			_consys_hw_pwr_on_rollback(CONNINFRA_PWR_ON_PMIC_ON_FAIL);
			return CONNINFRA_POWER_ON_D_DIE_FAIL;
		}

		/* POS PART 2:
		 * 1. Pinmux setting
		 * 2. Turn on MTCMOS
		 * 3. Enable AXI bus (AP2CONN slpprot)
		 */
		if (consys_hw_ops->consys_plt_set_if_pinmux)
			consys_hw_ops->consys_plt_set_if_pinmux(1);

		if (consys_hw_ops->consys_plt_conninfra_on_power_ctrl)
			ret = consys_hw_ops->consys_plt_conninfra_on_power_ctrl(1);
		if (ret) {
			pr_err("[%s] Conninfra HW power on fail, ret=%d\n", __func__, ret);
			_consys_hw_pwr_on_rollback(CONNINFRA_PWR_ON_CONNINFRA_HW_POWER_FAIL);
			return CONNINFRA_POWER_ON_D_DIE_FAIL;
		}

		if (consys_hw_ops->consys_plt_polling_consys_chipid)
			ret = consys_hw_ops->consys_plt_polling_consys_chipid();
		if (ret) {
			pr_err("[%s] polling d-die id fail, ret=%d\n", __func__, ret);
			_consys_hw_pwr_on_rollback(CONNINFRA_PWR_ON_POLLING_CHIP_ID_FAIL);
			return CONNINFRA_POWER_ON_D_DIE_FAIL;
		}

		/* POS PART 3:
		 * 1. Set connsys EMI mapping
		 * 2. d_die_cfg
		 * 3. spi_master_cfg
		 * 4. a_die_cfg
		 * 5. afe_wbg_cal
		 * 6. patch default value
		 * 7. CONN_INFRA low power setting (srcclken wait time, mtcmos HW ctl...)
		 */
		emi_mng_set_remapping_reg();
		if (consys_hw_ops->consys_plt_d_die_cfg)
			consys_hw_ops->consys_plt_d_die_cfg();
		if (consys_hw_ops->consys_plt_spi_master_cfg)
			consys_hw_ops->consys_plt_spi_master_cfg(next_status);
		if (consys_hw_ops->consys_plt_a_die_cfg)
			ret = consys_hw_ops->consys_plt_a_die_cfg();
		if (ret) {
			pr_err("[%s] a-die config error, ret=%d\n", __func__, ret);
			_consys_hw_pwr_on_rollback(CONNINFRA_PWR_ON_A_DIE_FAIL);
			return CONNINFRA_POWER_ON_A_DIE_FAIL;
		}
		if (consys_hw_ops->consys_plt_afe_sw_patch)
			consys_hw_ops->consys_plt_afe_sw_patch();
		if (consys_hw_ops->consys_plt_afe_wbg_cal)
			consys_hw_ops->consys_plt_afe_wbg_cal();
		if (consys_hw_ops->consys_plt_subsys_pll_initial)
			consys_hw_ops->consys_plt_subsys_pll_initial();
		/* Record SW status on shared sysram */
		if (consys_hw_ops->consys_plt_subsys_status_update)
			consys_hw_ops->consys_plt_subsys_status_update(true, on_radio);
		_consys_hw_raise_voltage(on_radio, true, true);
		if (consys_hw_ops->consys_plt_low_power_setting)
			consys_hw_ops->consys_plt_low_power_setting(curr_status, next_status);
		/* Enable low power mode */
		pmic_mng_common_power_low_power_mode(1);
	} else {
		ret = _consys_hw_conninfra_wakeup();
		if (ret) {
			pr_err("[%s] wakeup conninfra fail, ret=%d\n", __func__, ret);
			return CONNINFRA_POWER_ON_CONFIG_FAIL;
		}
		/* Record SW status on shared sysram */
		if (consys_hw_ops->consys_plt_subsys_status_update)
			consys_hw_ops->consys_plt_subsys_status_update(true, on_radio);
		if (consys_hw_ops->consys_plt_spi_master_cfg)
			consys_hw_ops->consys_plt_spi_master_cfg(next_status);
		_consys_hw_raise_voltage(on_radio, true, true);
		if (consys_hw_ops->consys_plt_low_power_setting)
			consys_hw_ops->consys_plt_low_power_setting(curr_status, next_status);

		_consys_hw_conninfra_sleep();
	}
	return 0;
}

int consys_hw_wifi_power_ctl(unsigned int enable)
{
	return pmic_mng_wifi_power_ctrl(enable);
}

int consys_hw_bt_power_ctl(unsigned int enable)
{
	return pmic_mng_bt_power_ctrl(enable);
}

int consys_hw_gps_power_ctl(unsigned int enable)
{
	return pmic_mng_gps_power_ctrl(enable);
}

int consys_hw_fm_power_ctl(unsigned int enable)
{
	return pmic_mng_fm_power_ctrl(enable);
}


int consys_hw_therm_query(int *temp_ptr)
{
	int ret = 0;

	/* wake/sleep conninfra */
	if (consys_hw_ops && consys_hw_ops->consys_plt_thermal_query) {
		ret = _consys_hw_conninfra_wakeup();
		if (ret)
			return CONNINFRA_ERR_WAKEUP_FAIL;
		*temp_ptr = consys_hw_ops->consys_plt_thermal_query();
		_consys_hw_conninfra_sleep();
	} else
		ret = -1;

	return ret;
}

void consys_hw_clock_fail_dump(void)
{
	if (consys_hw_ops && consys_hw_ops->consys_plt_clock_fail_dump)
		consys_hw_ops->consys_plt_clock_fail_dump();
}

int consys_hw_enable_power_dump(void)
{
	/* If not supported (no implement), assume it works fine. */
	int ret = 0;
	if (consys_hw_ops && consys_hw_ops->consys_plt_enable_power_dump)
		ret = consys_hw_ops->consys_plt_enable_power_dump();
	return ret;
}

int consys_hw_reset_power_state(void)
{
	/* If not supported (no implement), assume it works fine. */
	int ret = 0;
	if (consys_hw_ops && consys_hw_ops->consys_plt_reset_power_state)
		ret = consys_hw_ops->consys_plt_reset_power_state();
	return ret;
}

int consys_hw_dump_power_state(char *buf, unsigned int size)
{
	if (consys_hw_ops && consys_hw_ops->consys_plt_power_state)
		consys_hw_ops->consys_plt_power_state(buf, size);
	return 0;
}

int consys_hw_spi_read(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int *data)
{
	if (consys_hw_ops->consys_plt_spi_read)
		return consys_hw_ops->consys_plt_spi_read(subsystem, addr, data);
	return -1;
}

int consys_hw_spi_write(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data)
{
	if (consys_hw_ops->consys_plt_spi_write)
		return consys_hw_ops->consys_plt_spi_write(subsystem, addr, data);
	return -1;
}

int consys_hw_spi_update_bits(enum sys_spi_subsystem subsystem, unsigned int addr, unsigned int data, unsigned int mask)
{
	if (consys_hw_ops->consys_plt_spi_update_bits)
		return consys_hw_ops->consys_plt_spi_update_bits(subsystem, addr, data, mask);
	return -1;
}

int consys_hw_adie_top_ck_en_on(enum consys_adie_ctl_type type)
{
	if (consys_hw_ops->consys_plt_adie_top_ck_en_on)
		return consys_hw_ops->consys_plt_adie_top_ck_en_on(type);
	return -1;
}

int consys_hw_adie_top_ck_en_off(enum consys_adie_ctl_type type)
{
	if (consys_hw_ops->consys_plt_adie_top_ck_en_off)
		return consys_hw_ops->consys_plt_adie_top_ck_en_off(type);
	return -1;
}

int _consys_hw_raise_voltage(enum consys_drv_type drv_type, bool raise, bool onoff) {
	return pmic_mng_raise_voltage(drv_type, raise, onoff);
}

int consys_hw_raise_voltage(enum consys_drv_type drv_type, bool raise, bool onoff)
{
	int ret;

	ret = _consys_hw_conninfra_wakeup();
	if (ret == 0) {
		ret = _consys_hw_raise_voltage(drv_type, raise, onoff);
		_consys_hw_conninfra_sleep();
	} else {
		pr_err("[%s] wake up fail. drv={%d] raise=[%d] ret=[%d]\n",
			__func__, drv_type, raise, ret);
	}
	return 0;
}

static void _consys_hw_conninfra_add_wakeup_record(int count)
{
	unsigned long long sec = 0;
	unsigned long nsec = 0;

	osal_get_local_time(&sec, &nsec);

	if (g_conninfra_wakeup_rec_idx >= CONNINFRA_WAKEUP_RECORD_NUM)
		g_conninfra_wakeup_rec_idx = 0;

	g_conninfra_wakeup_rec_cnt[g_conninfra_wakeup_rec_idx] = count;
	g_conninfra_wakeup_rec_sec[g_conninfra_wakeup_rec_idx] = sec;
	g_conninfra_wakeup_rec_nsec[g_conninfra_wakeup_rec_idx] = nsec;
	g_conninfra_wakeup_rec_idx++;

	if (g_conninfra_wakeup_rec_idx == CONNINFRA_WAKEUP_RECORD_NUM)
		_consys_hw_conninfra_print_wakeup_record();
}

static void _consys_hw_conninfra_print_wakeup_record(void)
{
	unsigned long long *sec = g_conninfra_wakeup_rec_sec;
	unsigned long *nsec = g_conninfra_wakeup_rec_nsec;
	int *cnt = g_conninfra_wakeup_rec_cnt;

	pr_info("conn_wakeup:%llu.%06lu:%d; %llu.%06lu:%d; %llu.%06lu:%d; %llu.%06lu:%d; %llu.%06lu:%d; %llu.%06lu:%d",
		sec[0], nsec[0], cnt[0], sec[1], nsec[1], cnt[1], sec[2], nsec[2], cnt[2],
		sec[3], nsec[3], cnt[3], sec[4], nsec[4], cnt[4], sec[5], nsec[5], cnt[5]);
}

static int _consys_hw_conninfra_wakeup(void)
{
	bool wakeup = false, ret;

	if (consys_hw_ops->consys_plt_conninfra_wakeup) {
		if (g_conninfra_wakeup_ref_cnt == 0)  {
			ret = consys_hw_ops->consys_plt_conninfra_wakeup();
			if (ret) {
				pr_err("wakeup fail!! ret=[%d]", ret);
				return ret;
			}
			wakeup = true;
		}
		g_conninfra_wakeup_ref_cnt++;
		_consys_hw_conninfra_add_wakeup_record(g_conninfra_wakeup_ref_cnt);
	}
	return 0;
}

static void _consys_hw_conninfra_sleep(void)
{
	bool sleep = false;

	if (consys_hw_ops->consys_plt_conninfra_sleep &&
		--g_conninfra_wakeup_ref_cnt == 0) {
		sleep = true;
		consys_hw_ops->consys_plt_conninfra_sleep();
	}

	if (g_conninfra_wakeup_ref_cnt < 0) {
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
		aee_kernel_exception("conninfra", "%s count %d is unexpected.", __func__, g_conninfra_wakeup_ref_cnt);
#else
		pr_notice("%s count %d is unexpected.", __func__, g_conninfra_wakeup_ref_cnt);
#endif
		_consys_hw_conninfra_print_wakeup_record();
	} else
		_consys_hw_conninfra_add_wakeup_record(g_conninfra_wakeup_ref_cnt);
}

int consys_hw_force_conninfra_wakeup(void)
{
	return _consys_hw_conninfra_wakeup();
}

int consys_hw_force_conninfra_sleep(void)
{
	_consys_hw_conninfra_sleep();
	return 0;
}

int consys_hw_spi_clock_switch(enum connsys_spi_speed_type type)
{
	if (consys_hw_ops->consys_plt_spi_clock_switch)
		return consys_hw_ops->consys_plt_spi_clock_switch(type);
	return -1;
}

void consys_hw_config_setup(void)
{
	if (consys_hw_ops->consys_plt_config_setup)
		consys_hw_ops->consys_plt_config_setup();
}

int consys_hw_pmic_event_cb(unsigned int id, unsigned int event)
{
	pmic_mng_event_cb(id, event);
	return 0;
}

int consys_hw_bus_clock_ctrl(enum consys_drv_type drv_type, unsigned int bus_clock, int status)
{
	if (consys_hw_ops->consys_plt_bus_clock_ctrl)
		return consys_hw_ops->consys_plt_bus_clock_ctrl(drv_type, bus_clock, status);
	else
		return -1;
}

int get_consys_platform_ops(struct platform_device *pdev)
{
	g_conninfra_plat_data = (const struct conninfra_plat_data*)of_device_get_match_data(&pdev->dev);
	if (g_conninfra_plat_data == NULL) {
		pr_err("[%s] Get platform data fail.", __func__);
		return -1;
	}
	if (consys_hw_ops == NULL)
		consys_hw_ops = (const struct consys_hw_ops_struct*)g_conninfra_plat_data->hw_ops;

	if (consys_hw_ops == NULL) {
		pr_err("Get HW op fail");
		return -1;
	}
	return 0;
}

int consys_hw_tcxo_parser(struct platform_device *pdev)
{
	int ret = 0;
	int pinmux = 0;
	struct device_node *pinctrl_node;
	struct device_node *pins_node;
	unsigned int pin_num = 0xffffffff;
	const char* tcxo_support;

	/* Get tcxo status */
	/* Set default value */
	conn_hw_env.tcxo_support = false;
	ret = of_property_read_string(pdev->dev.of_node, "tcxo_support", &tcxo_support);
	if (!ret) {
		if (strcmp(tcxo_support, "true") == 0) {
			conn_hw_env.tcxo_support = true;
			pr_info("[%s] Support TCXO", __func__);
		}
	} else {
		pr_warn("Get tcxo property fail: %d", ret);
	}

	/* Set up gpio for TCXO */
	g_conninfra_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);

	if (IS_ERR(g_conninfra_pinctrl_ptr)) {
		pr_err("Fail to get conninfra pinctrl");
	} else {
		pinctrl_node = of_parse_phandle(pdev->dev.of_node, "pinctrl-1", 0);

		if (pinctrl_node) {
			pins_node = of_get_child_by_name(pinctrl_node, "pins_cmd_dat");

			if (pins_node) {
				ret = of_property_read_u32(pins_node, "pinmux", &pinmux);

				if (ret == 0) {
					pin_num = (pinmux >> 8) & 0xff;
					pr_info("Conninfra gpio pin number[%d]\n", pin_num);
				} else {
					pr_err("Fail to get conninfra gpio pin number");
				}
			}
		}
	}

	pr_info("[%s] tcxo=%d pintctrl=%p", __func__,
		conn_hw_env.tcxo_support, g_conninfra_pinctrl_ptr);
	return 0;
}

u64 consys_hw_soc_timestamp_get(void)
{
	if (consys_hw_ops->consys_plt_soc_timestamp_get)
		return consys_hw_ops->consys_plt_soc_timestamp_get();
	return 0;
}

int consys_hw_pre_cal_backup(unsigned int offset, unsigned int size)
{
	if (consys_hw_ops->consys_plt_pre_cal_backup)
		return consys_hw_ops->consys_plt_pre_cal_backup(offset, size);
	return 0;
}

int consys_hw_pre_cal_clean_data(void)
{
	if (consys_hw_ops->consys_plt_pre_cal_clean_data)
		return consys_hw_ops->consys_plt_pre_cal_clean_data();
	return 0;
}

int mtk_conninfra_probe(struct platform_device *pdev)
{
	int ret = -1;
	struct consys_emi_addr_info* emi_info = NULL;

	if (pdev == NULL) {
		pr_err("[%s] invalid input", __func__);
		return -1;
	}

	ret = get_consys_platform_ops(pdev);
	if (ret) {
		pr_err("[%s] get platform ops fail", __func__);
		return -2;
	}

	/* Read device node */
	if (consys_reg_mng_init(pdev, g_conninfra_plat_data) != 0) {
		pr_err("consys_plt_read_reg_from_dts fail");
		return -3;
	}

	if (consys_hw_ops->consys_plt_clk_get_from_dts)
		consys_hw_ops->consys_plt_clk_get_from_dts(pdev);
	else {
		pr_err("consys_plt_clk_get_from_dtsfail");
		return -4;
	}

	/* emi mng init */
	ret = emi_mng_init(pdev, g_conninfra_plat_data);
	if (ret) {
		pr_err("emi_mng init fail, %d\n", ret);
		return -5;
	}

	ret = pmic_mng_init(pdev, g_conninfra_dev_cb, g_conninfra_plat_data);
	if (ret) {
		pr_err("pmic_mng init fail, %d\n", ret);
		return -6;
	}

	/* Setup connsys log emi base */
	emi_info = emi_mng_get_phy_addr();
	if (emi_info) {
		connsys_dedicated_log_path_apsoc_init((phys_addr_t)emi_info->emi_ap_phy_addr,
                                        g_conninfra_plat_data->connsyslog_config);
	} else {
		pr_err("Connsys log didn't init because EMI is invalid\n");
	}

	consys_hw_tcxo_parser(pdev);

	coredump_mng_init(g_conninfra_plat_data);
	g_pdev = pdev;

	osal_sleepable_lock_init(&g_adie_chipid_lock);

	atomic_set(&g_hw_init_done, 1);
	return 0;
}

int mtk_conninfra_remove(struct platform_device *pdev)
{
	atomic_set(&g_hw_init_done, 0);

	if (consys_hw_ops->consys_plt_clk_detach)
		consys_hw_ops->consys_plt_clk_detach();
	else
		pr_info("consys_plt_clk_detach is null");

	if (g_pdev)
		g_pdev = NULL;

	osal_sleepable_lock_deinit(&g_adie_chipid_lock);
	return 0;
}

static int mtk_conninfra_suspend(void)
{
	int ret = 0;

	/* suspend callback is in atomic context */
	if (g_conninfra_dev_cb && g_conninfra_dev_cb->conninfra_suspend_cb)
		ret = (*g_conninfra_dev_cb->conninfra_suspend_cb)();

	conn_pwr_suspend();

	return ret;
}

static int mtk_conninfra_resume(void)
{
	/* suspend callback is in atomic context */
	schedule_work(&ap_resume_work);
	return 0;
}


static void consys_hw_ap_resume_handler(struct work_struct *work)
{
	if (g_conninfra_dev_cb && g_conninfra_dev_cb->conninfra_resume_cb)
		(*g_conninfra_dev_cb->conninfra_resume_cb)();

	conn_pwr_resume();
}

/* this function is used by conninfra_dbg. */
int consys_hw_set_platform_config(int value)
{
	g_platform_config = value;
	return 0;
}

int consys_hw_get_platform_config(void)
{
	return g_platform_config;
}

static int conninfra_pm_notifier_callback(struct notifier_block *nb,
		unsigned long event, void *dummy)
{
	switch (event) {
		case PM_SUSPEND_PREPARE:
			mtk_conninfra_suspend();
			break;
		case PM_POST_SUSPEND:
			mtk_conninfra_resume();
			break;
		default:
			break;
	}
	return NOTIFY_DONE;
}

void consys_hw_set_mcu_control(int type, bool onoff)
{
	if (consys_hw_ops->consys_plt_set_mcu_control)
		consys_hw_ops->consys_plt_set_mcu_control(type, onoff);
	else
		pr_notice("consys_plt_set_mcu_control not supported\n");
}

int consys_hw_init(struct conninfra_dev_cb *dev_cb)
{
	int iRet = 0, ret = 0, retry = 0;
	phys_addr_t emi_addr = 0;
	unsigned int emi_size = 0;
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);
	g_conninfra_dev_cb = dev_cb;

	pmic_mng_register_device();
	clock_mng_register_device();

	iRet = platform_driver_register(&mtk_conninfra_dev_drv);
	if (iRet)
		pr_err("Conninfra platform driver registered failed(%d)\n", iRet);
	else {
		while (atomic_read(&g_hw_init_done) == 0) {
			osal_sleep_ms(50);
			retry++;
			if (__ratelimit(&_rs))
				pr_info("g_hw_init_done = 0, retry = %d", retry);
		}
	}

	conninfra_get_phy_addr(&emi_addr, &emi_size);
	connectivity_export_conap_scp_init(consys_hw_get_ic_info(CONNSYS_SOC_CHIPID), emi_addr);

	INIT_WORK(&ap_resume_work, consys_hw_ap_resume_handler);

	conninfra_pm_notifier.notifier_call = conninfra_pm_notifier_callback;
	ret = register_pm_notifier(&conninfra_pm_notifier);
	if (ret < 0)
		pr_notice("%s register_pm_notifier fail %d\n", __func__, ret);

	pr_info("[consys_hw_init] result [%d]\n", iRet);

	return iRet;
}

int consys_hw_deinit(void)
{
	platform_driver_unregister(&mtk_conninfra_dev_drv);
	unregister_pm_notifier(&conninfra_pm_notifier);
	g_conninfra_dev_cb = NULL;
	return 0;
}
