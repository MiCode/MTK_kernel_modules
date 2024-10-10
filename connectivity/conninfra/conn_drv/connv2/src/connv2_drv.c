/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/version.h>

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
#include <connectivity_build_in_adapter.h>
#include "conn_power_throttling.h"
#include "wmt_build_in_adapter.h"
#endif

#include "osal.h"
#include "conninfra_core.h"
#include "conninfra_dbg.h"
#include "consys_hw.h"
#include "connsys_debug_utility.h"
#include "coredump_mng.h"
#include "conn_adaptor.h"
#include "emi_mng.h"

#if IS_ENABLED(CONFIG_DEVICE_MODULES_MTK_DEVAPC)
#include <devapc_public.h>
#endif

#ifdef CFG_CONNINFRA_UT_SUPPORT
#include "conninfra_test.h"
#endif

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

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/

extern const struct of_device_id apconninfra_of_ids[];

static int mtk_conninfra_probe(struct platform_device *pdev);
static int mtk_conninfra_remove(struct platform_device *pdev);

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

struct platform_device *g_drv_dev;

/* PMIC */
struct conninfra_pmic_work {
	unsigned int id;
	unsigned int event;
	struct work_struct pmic_work;
};

/* For PMIC callback */
static struct conninfra_pmic_work g_conninfra_pmic_work;

static int conninfra_dev_pmic_event_cb(unsigned int, unsigned int);
static struct conninfra_dev_cb g_conninfra_dev_cb = {
	.conninfra_pmic_event_notifier = conninfra_dev_pmic_event_cb,
};

/* suspend/resume */
struct work_struct g_ap_resume_work;
void connv2_suspend_notify(void);
void connv2_resume_notify(void);

/* adaptor */
void connv2_set_coredump_mode(int mode);
u32 connv2_detect_adie_chipid(u32 drv_type);

/* screen on/off */
void connv2_power_on_off_notify(int on_off);

/* thermal query for Linux thermal framework */
static int g_temp_thermal_value;
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static int conninfra_thermal_get_temp_cb(struct thermal_zone_device *tz,
		int *temp);
static const struct thermal_zone_device_ops tz_conninfra_thermal_ops = {
	.get_temp = conninfra_thermal_get_temp_cb,
};
#else
static int conninfra_thermal_get_temp_cb(void *data, int *temp);
static const struct thermal_zone_of_device_ops tz_conninfra_thermal_ops = {
	.get_temp = conninfra_thermal_get_temp_cb,
};
#endif

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
/* thermal for legacy MTK thermal platform */
static int last_thermal_value;
static int conninfra_thermal_query_cb(void);

/* clk fail */
static void conninfra_clock_fail_dump_cb(void);

/* readable & bus_hang */
static int conninfra_conn_reg_readable(void);
static int conninfra_conn_is_bus_hang(void);
#ifdef SSPM_DEBUG_DUMP
static int conninfra_conn_bus_dump(void);
#endif
#endif

/* DEVAPC */
#ifdef CONFIG_FPGA_EARLY_PORTING
/* Disable DEVAPC on FPGA */
#define CFG_CONNINFRA_DEVAPC_SUPPORT	0
#else
#if IS_ENABLED(CONFIG_DEVICE_MODULES_MTK_DEVAPC)
#define CFG_CONNINFRA_DEVAPC_SUPPORT	1
#else
#define CFG_CONNINFRA_DEVAPC_SUPPORT	0
#endif
#endif

#if CFG_CONNINFRA_DEVAPC_SUPPORT
static void conninfra_devapc_violation_cb(void);
static void conninfra_register_devapc_callback(void);
/* For DEVAPC callback */
static struct work_struct g_conninfra_devapc_work;
static struct devapc_vio_callbacks conninfra_devapc_handle = {
	.id = INFRA_SUBSYS_CONN,
	.debug_dump = conninfra_devapc_violation_cb,
};
#endif
ssize_t connv2_coredump_emi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
int connv2_dump_power_state(uint8_t *buf, u32 buf_sz);

struct conn_adaptor_drv_gen_cb g_connv2_drv_gen = {
	.drv_radio_support = 0x7,

	.get_chip_id = consys_hw_chipid_get,
	.get_adie_id = connv2_detect_adie_chipid,

	/* suspend/resume */
	.plat_suspend_notify = connv2_suspend_notify,
	.plat_resume_notify = connv2_resume_notify,

	/* on/off */
	.power_on_off_notify = connv2_power_on_off_notify,

	/* coredump */
	.set_coredump_mode = connv2_set_coredump_mode,
	.coredump_emi_read = connv2_coredump_emi_read,

	/* dump power state */
	.dump_power_state = connv2_dump_power_state,
};

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
struct wmt_platform_bridge g_plat_bridge = {
	.thermal_query_cb = conninfra_thermal_query_cb,
	.clock_fail_dump_cb  = conninfra_clock_fail_dump_cb,
	.conninfra_reg_readable_cb = conninfra_conn_reg_readable,
#ifdef SSPM_DEBUG_DUMP
	.conninfra_reg_is_bus_hang_cb = conninfra_conn_is_bus_hang,
	.conninfra_reg_is_bus_hang_no_lock_cb = conninfra_conn_bus_dump,
#else
	.conninfra_reg_is_bus_hang_cb = conninfra_conn_is_bus_hang,
#endif
#if CONNINFRA_DBG_SUPPORT
	//.debug_cb = conninfra_dbg_write,
#endif
};
#endif

static atomic_t g_connv2_hw_init_done = ATOMIC_INIT(0);

static const u32 drv_convertor[CONN_ADAPTOR_DRV_SIZE] = {CONNDRV_TYPE_WIFI,
				CONNDRV_TYPE_BT, CONNDRV_TYPE_GPS, CONNDRV_TYPE_FM};
static int _conn_adp_drv_2_connv2_drv(u32 adp_drv_type) {
	if (adp_drv_type >= CONN_ADAPTOR_DRV_SIZE)
		return -1;
	return drv_convertor[adp_drv_type];
}

/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/
u32 connv2_chipid_get(void)
{
	return consys_hw_chipid_get();
}

void connv2_set_coredump_mode(int mode)
{
	connsys_coredump_set_dump_mode(mode);
}



u32 connv2_detect_adie_chipid(u32 drv_type)
{
	int v2_drv_type = _conn_adp_drv_2_connv2_drv(drv_type);
	if (v2_drv_type < 0) {
		pr_notice("[%s] input error: %d\n", __func__, drv_type);
		return 0;
	}
	return consys_hw_detect_adie_chipid(v2_drv_type);
}

static int conninfra_dev_pmic_event_cb(unsigned int id, unsigned int event)
{
	g_conninfra_pmic_work.id = id;
	g_conninfra_pmic_work.event = event;
	schedule_work(&g_conninfra_pmic_work.pmic_work);

	return 0;
}

static void conninfra_dev_pmic_event_handler(struct work_struct *work)
{
	unsigned int id, event;
	struct conninfra_pmic_work *pmic_work =
		container_of(work, struct conninfra_pmic_work, pmic_work);

	if (pmic_work) {
		id = pmic_work->id;
		event = pmic_work->event;
		conninfra_core_pmic_event_cb(id, event);
	} else
		pr_info("[%s] pmic_work is null", __func__);

}

void connv2_suspend_notify(void)
{
	connsys_dedicated_log_set_ap_state(0);
	conninfra_core_reset_power_state();
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	conn_pwr_suspend();
#endif
}

void connv2_resume_notify(void)
{
	schedule_work(&g_ap_resume_work);
}

static void consys_hw_ap_resume_handler(struct work_struct *work)
{
	conninfra_core_dump_power_state(NULL, 0);
	connsys_dedicated_log_set_ap_state(1);
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	conn_pwr_resume();
#endif
}


void connv2_power_on_off_notify(int on_off)
{
	pr_info("[%s] on_off=[%d]", __func__, on_off);
	if (on_off == 1)
		conninfra_core_screen_on();
	else
		conninfra_core_screen_off();
}

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
static int conninfra_thermal_query_cb(void)
{
	int ret;

	/* if rst is ongoing, return thermal val got from last time */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, return last temp ", __func__);
		return last_thermal_value;
	}
	pr_info("[%s] query thermal", __func__);
	ret = conninfra_core_thermal_query(&g_temp_thermal_value);
	if (ret == 0)
		last_thermal_value = g_temp_thermal_value;
	else if (ret == CONNINFRA_ERR_WAKEUP_FAIL)
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "Query thermal wakeup fail");

	return last_thermal_value;
}
#endif

/* for Linux thermal framework */
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
static int conninfra_thermal_get_temp_cb(struct thermal_zone_device *tz,
		int *temp)
#else
static int conninfra_thermal_get_temp_cb(void *data, int *temp)
#endif
{
	int ret;

	if (!temp)
		return 0;

	/* if rst is ongoing, return THERMAL_TEMP_INVALID */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, return invalid value ", __func__);
		*temp = THERMAL_TEMP_INVALID;
		return 0;
	}

	ret = conninfra_core_thermal_query(&g_temp_thermal_value);
	if (ret == 0)
		*temp = g_temp_thermal_value * 1000;
	else if (ret == CONNINFRA_ERR_WAKEUP_FAIL) {
		conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "Query thermal wakeup fail");
		*temp = THERMAL_TEMP_INVALID;
	} else
		*temp = THERMAL_TEMP_INVALID;

	return 0;
}

static void conninfra_register_thermal_callback(void)
{
	struct thermal_zone_device *tz;
	struct platform_device *pdev = get_consys_device();
	int ret;

	if (!pdev) {
		pr_info("get_consys_device return NULL\n");
		return;
	}

	/* register thermal zone */
#if (KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE)
	tz = devm_thermal_of_zone_register(
		&pdev->dev, 0, NULL, &tz_conninfra_thermal_ops);
#else
	tz = devm_thermal_zone_of_sensor_register(
		&pdev->dev, 0, NULL, &tz_conninfra_thermal_ops);
#endif

	if (IS_ERR(tz)) {
		ret = PTR_ERR(tz);
		pr_info("Failed to register thermal zone device %d\n", ret);
		return;
	}
	pr_info("Register thermal zone device.\n");
}

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
static void conninfra_clock_fail_dump_cb(void)
{
	conninfra_core_clock_fail_dump_cb();
}
#endif

ssize_t connv2_coredump_emi_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct consys_emi_addr_info* addr_info = emi_mng_get_phy_addr();
	unsigned int start_offset = 0, end_offset = 0;
	unsigned long size = 0;
	unsigned int mcif_emi_size = 0;
	phys_addr_t emi_dump_addr = 0;
	phys_addr_t mcif_emi_dump_addr = 0;
	void __iomem *emi_virt_addr_base;
	void __iomem *mcif_emi_virt_addr_base;
	char* tmp_buf = NULL;
	unsigned long retval;

	if (addr_info == NULL)
		return -EINVAL;
	mcif_emi_size = addr_info->md_emi_size;
	emi_dump_addr = addr_info->emi_ap_phy_addr;
	mcif_emi_dump_addr = addr_info->md_emi_phy_addr;

	coredump_mng_get_emi_dump_offset(&start_offset, &end_offset);
	size = end_offset - start_offset;

	if (count != (size + mcif_emi_size) || end_offset < start_offset) {
		pr_notice("[%s][0x%x,0x%x] connsys emi_size=0x%x, mcif emi size=0x%x but buf_size=0x%x",
			__func__, start_offset, end_offset,
			size, mcif_emi_size, count);
		return 0;
	} else {
		pr_info("[%s] base=[0x%x] [0x%x~0x%x] size=0x%x", __func__, emi_dump_addr, start_offset, end_offset, size);
		emi_virt_addr_base = ioremap(emi_dump_addr + start_offset, size);
		if (emi_virt_addr_base == NULL) {
			pr_notice("[%s] remap fail, addr=0x%x size=%d",
				__func__, emi_dump_addr + start_offset, size);
			return 0;
		}
		tmp_buf = (char*)osal_malloc(sizeof(char) * size);
		if (tmp_buf == NULL) {
			pr_notice("[%s] allocate buf fail, size = %d",
				__func__, size);
			iounmap(emi_virt_addr_base);
			return 0;
		}
		memcpy_fromio(tmp_buf, emi_virt_addr_base, size);
		retval = copy_to_user(buf, tmp_buf, size);
		if (retval != 0)
			pr_notice("[%s] copy to user fail: %d", __func__, retval);
		iounmap(emi_virt_addr_base);
		osal_free(tmp_buf);

		if (mcif_emi_size == 0)
			return (size - retval);

		pr_info("[%s] base=[0x%x] [0x%x~0x%x] size=0x%x", __func__, mcif_emi_dump_addr, 0, mcif_emi_size, mcif_emi_size);
		mcif_emi_virt_addr_base = ioremap(mcif_emi_dump_addr, mcif_emi_size);
		if (mcif_emi_virt_addr_base == NULL) {
			pr_notice("[%s] remap fail, addr=0x%x size=%d",
				__func__, mcif_emi_dump_addr, mcif_emi_size);
			return size;
		}
		tmp_buf = (char*)osal_malloc(sizeof(char) * mcif_emi_size);
		if (tmp_buf == NULL) {
			pr_notice("[%s] allocate buf fail, size = %d",
				__func__, mcif_emi_size);
			iounmap(mcif_emi_virt_addr_base);
			return size;
		}
		memcpy_fromio(tmp_buf, mcif_emi_virt_addr_base, mcif_emi_size);
		retval = copy_to_user(buf + size, tmp_buf, mcif_emi_size);
		if (retval != 0)
			pr_notice("[%s] copy to user fail: %d", __func__, retval);
		iounmap(mcif_emi_virt_addr_base);

		osal_free(tmp_buf);

		return (size + mcif_emi_size - retval);
	}
}

int connv2_dump_power_state(uint8_t *buf, u32 buf_sz)
{
#define CONN_DUMP_STATE_BUF_SIZE 1024
	int ret = 0, len;
	char tmp_buf[CONN_DUMP_STATE_BUF_SIZE];

#if 0
	ret = osal_lock_sleepable_lock(&g_dump_lock);
	if (ret) {
		pr_notice("dump_lock fail!!");
		return ret;
	}
#endif

	memset(tmp_buf, '\0', CONN_DUMP_STATE_BUF_SIZE);
	ret = conninfra_core_dump_power_state(tmp_buf, CONN_DUMP_STATE_BUF_SIZE);
	if (ret) {
		//osal_unlock_sleepable_lock(&g_dump_lock);
		return ret;
	}

	len = strlen(tmp_buf);
	if (len > 0 && len < CONN_DUMP_STATE_BUF_SIZE) {
		if (snprintf(buf, buf_sz, "%s", tmp_buf, len) < 0)
			pr_notice("[%s] snprintf fail", __func__);
	} else
		return -1;

	//osal_unlock_sleepable_lock(&g_dump_lock);
	return len;
}

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
/*  THIS FUNCTION IS ONLY FOR AUDIO DRIVER                 */
/* this function go through consys_hw, skip conninfra_core */
/* there is no lock and skip consys power on check         */
/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
static int conninfra_conn_reg_readable(void)
{
	return consys_hw_reg_readable();
	/*return conninfra_core_reg_readable(); */
}

static int conninfra_conn_is_bus_hang(void)
{
	/* if rst is ongoing, don't dump */
	if (conninfra_core_is_rst_locking()) {
		pr_info("[%s] rst is locking, skip dump", __func__);
		return CONNINFRA_ERR_RST_ONGOING;
	}
	return conninfra_core_is_bus_hang();
}

/* For sspm debug dump
 * To dump connsys bus status when sspm ipi timeout
 */
#ifdef SSPM_DEBUG_DUMP
static int conninfra_conn_bus_dump(void)
{
	return conninfra_core_conn_bus_dump();
}
#endif
#endif

#if CFG_CONNINFRA_DEVAPC_SUPPORT
static void conninfra_devapc_violation_cb(void)
{
	schedule_work(&g_conninfra_devapc_work);
}

static void conninfra_devapc_handler(struct work_struct *work)
{
	conninfra_trigger_whole_chip_rst(CONNDRV_TYPE_CONNINFRA, "CONNSYS DEVAPC");
}

static void conninfra_register_devapc_callback(void)
{
	INIT_WORK(&g_conninfra_devapc_work, conninfra_devapc_handler);
	register_devapc_vio_callback(&conninfra_devapc_handle);
}
#endif /* CFG_CONNINFRA_DEVAPC_SUPPORT */

static void conninfra_register_power_throttling_callback(void)
{
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	struct conn_pwr_plat_info pwr_info;
	int ret;

	pwr_info.chip_id = consys_hw_chipid_get();
	/* default wifi bucause it usually has the highest power consumption */
	pwr_info.adie_id = consys_hw_detect_adie_chipid(CONNDRV_TYPE_WIFI);
	pwr_info.get_temp = conninfra_core_thermal_query;
	ret = conn_pwr_init(&pwr_info);
	if (ret < 0)
		pr_info("conn_pwr_init is failed %d.", ret);
#endif
}

int mtk_conninfra_probe(struct platform_device *pdev)
{
	int ret = -1;
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	struct conn_pwr_plat_info pwr_info;
#endif

	if (pdev == NULL) {
		pr_err("[%s] invalid input", __func__);
		return -1;
	}

	pr_info("[%s] --- [%x][%x] of_node[%s][%s]", __func__,
				pdev->dev.driver->of_match_table, apconninfra_of_ids,
				(pdev->dev.of_node != NULL ? pdev->dev.of_node->name : ""),
				(pdev->dev.of_node != NULL ? pdev->dev.of_node->full_name : ""));

	ret = consys_hw_init(pdev, &g_conninfra_dev_cb);
	if (ret) {
		return -2;
	}

	g_drv_dev = pdev;

	/* TODO */
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	/* power throttling */
	pwr_info.chip_id = consys_hw_chipid_get();
	/* default wifi bucause it usually has the highest power consumption */
	pwr_info.adie_id = consys_hw_detect_adie_chipid(CONNDRV_TYPE_WIFI);
	pwr_info.get_temp = consys_hw_therm_query;
	ret = conn_pwr_init(&pwr_info);
	if (ret < 0)
		pr_info("conn_pwr_init is failed %d.", ret);
#endif

	atomic_set(&g_connv2_hw_init_done, 1);

	pr_info("[%s] init done", __func__);

	return 0;
}

int mtk_conninfra_remove(struct platform_device *pdev)
{
	atomic_set(&g_connv2_hw_init_done, 0);
	consys_hw_deinit();
	if (g_drv_dev)
		g_drv_dev = NULL;

	return 0;
}


int connv2_drv_init(void)
{
	unsigned int connv2_radio_support = 0;
	unsigned int drv_enum_converter[CONNDRV_TYPE_MAX] =
		{CONN_SUPPOPRT_DRV_BT_TYPE_BIT, CONN_SUPPOPRT_DRV_FM_TYPE_BIT,
		CONN_SUPPOPRT_DRV_GPS_TYPE_BIT, CONN_SUPPOPRT_DRV_WIFI_TYPE_BIT,
		0x0, 0x0};
	int i = 0;
	unsigned int adaptor_radio_support = 0;
	int iret = 0, retry = 0;
	static DEFINE_RATELIMIT_STATE(_rs, HZ, 1);

	ratelimit_set_flags(&_rs, RATELIMIT_MSG_ON_RELEASE);


	iret = platform_driver_probe(&mtk_conninfra_dev_drv, mtk_conninfra_probe);
	//iret = platform_driver_register(&mtk_conninfra_dev_drv);
	if (iret) {
		pr_err("Conninfra platform driver registered failed(%d)\n", iret);
		return -1;
	} else {
		while (atomic_read(&g_connv2_hw_init_done) == 0) {
			osal_sleep_ms(50);
			retry++;
			if (__ratelimit(&_rs))
				pr_info("g_connv2_hw_init_done = 0, retry = %d", retry);
		}
	}
#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	wmt_export_platform_bridge_register(&g_plat_bridge);
#endif

	INIT_WORK(&g_conninfra_pmic_work.pmic_work, conninfra_dev_pmic_event_handler);

	/* init core */
	iret = conninfra_core_init(consys_hw_get_support_drv());
	if (iret) {
		pr_err("conninfra init fail");
		return -3;
	}

	/* init dbg device node */
	conninfra_dev_dbg_init();

	/* ap resume worker */
	INIT_WORK(&g_ap_resume_work, consys_hw_ap_resume_handler);

	connv2_radio_support = consys_hw_get_support_drv();
	for (i = 0; i < CONN_ADAPTOR_DRV_SIZE; i++) {
		if (connv2_radio_support & (0x1 << i))
			adaptor_radio_support |= drv_enum_converter[i];
	}
	pr_info("[%s] radio support convert 0x%x => 0x%x",
		__func__, connv2_radio_support, adaptor_radio_support);

	g_connv2_drv_gen.drv_radio_support = adaptor_radio_support;
	iret = conn_adaptor_register_drv_gen(CONN_ADAPTOR_DRV_GEN_CONNAC_2, &g_connv2_drv_gen);
	if (iret)
		pr_notice("Register to conn_adap fail, ret = %d", iret);

#if CFG_CONNINFRA_DEVAPC_SUPPORT
	conninfra_register_devapc_callback();
#endif
	conninfra_register_thermal_callback();
	conninfra_register_power_throttling_callback();
#ifdef CFG_CONNINFRA_UT_SUPPORT
	iret = conninfra_test_setup();
	if (iret)
		pr_notice("init conninfra_test fail, ret = %d\n", iret);
#endif
	pr_info("[%s] init successfully", __func__);
	return 0;
}

int connv2_drv_deinit(void)
{
	int ret;

#ifdef CFG_CONNINFRA_UT_SUPPORT
	ret = conninfra_test_remove();
#endif

#if defined(CONNINFRA_PLAT_ALPS) && CONNINFRA_PLAT_ALPS
	conn_pwr_deinit();
#endif
	ret = conn_adaptor_unregister_drv_gen(CONN_ADAPTOR_DRV_GEN_CONNAC_2);

	ret = conninfra_dev_dbg_deinit();

	ret = conninfra_core_deinit();

	ret = consys_hw_deinit();

	platform_driver_unregister(&mtk_conninfra_dev_drv);
	return 0;
}
