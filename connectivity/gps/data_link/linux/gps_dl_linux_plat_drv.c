/*
 * Copyright (C) 2019 MediaTek Inc.
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
#include "gps_dl_config.h"

#if GPS_DL_HAS_PLAT_DRV
#include <linux/pinctrl/consumer.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/poll.h>

#include <linux/io.h>
#include <asm/io.h>
#include <sync_write.h>

#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>

#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_isr.h"
#include "gps_each_device.h"

/* #ifdef CONFIG_OF */
const struct of_device_id gps_dl_of_ids[] = {
	{ .compatible = "mediatek,mt6885-gps", },
	{}
};
/* #endif */
#define GPS_DL_IOMEM_NUM 2

struct gps_dl_iomem_addr_map_entry g_gps_dl_iomem_arrary[GPS_DL_IOMEM_NUM];
struct gps_dl_iomem_addr_map_entry g_gps_dl_status_dummy_cr;
struct gps_dl_iomem_addr_map_entry g_gps_dl_tia_gps;

void __iomem *gps_dl_host_addr_to_virt(unsigned int host_addr)
{
	int i;
	int offset;
	struct gps_dl_iomem_addr_map_entry *p;

	for (i = 0; i < GPS_DL_IOMEM_NUM; i++) {
		p = &g_gps_dl_iomem_arrary[i];

		if (p->length == 0)
			continue;

		offset = host_addr - p->host_phys_addr;
		if (offset >= 0 && offset < p->length)
			return p->host_virt_addr + offset;
	}

	return (void __iomem *)0;
}

void gps_dl_update_status_for_md_blanking(bool gps_is_on)
{
	void __iomem *p = g_gps_dl_status_dummy_cr.host_virt_addr;
	unsigned int val = (gps_is_on ? 1 : 0);
	unsigned int val_old, val_new;

	if (p != NULL) {
		val_old = __raw_readl(p);
		mt_reg_sync_writel(val, p);
		val_new = __raw_readl(p);
		GDL_LOGI_INI("dummy cr updated: %d -> %d, due to on = %d",
			val_old, val_new, gps_is_on);
	} else
		GDL_LOGW_INI("dummy cr addr is invalid, can not update (on = %d)", gps_is_on);
}

void gps_dl_tia_gps_ctrl(bool gps_is_on)
{
	void __iomem *p = g_gps_dl_tia_gps.host_virt_addr;
	unsigned int tia_gps_on, tia_gps_ctrl, tia_temp;
	unsigned int tia_gps_on1, tia_gps_ctrl1, tia_temp1;

	if (p == NULL) {
		GDL_LOGW_INI("on = %d, tia_gps addr is null", gps_is_on);
		return;
	}

	tia_gps_on = __raw_readl(p);
	tia_gps_ctrl = __raw_readl(p + 4);
	tia_temp = __raw_readl(p + 8);

	if (gps_is_on) {
		/* 0x1001C018[0] = 1 (GPS on) */
		mt_reg_sync_writel(tia_gps_on | 1UL, p);

		/* 0x1001C01C[11:0] = 100 (~3ms update period, 1/32k = 0.03125ms)
		 * 0x1001C01C[12] = 1 (enable TSX)
		 * 0x1001C01C[13] = 1 (enable DCXO)
		 */
		/* 20190923 period changed to 196 (0xC4, 6ms) */
		mt_reg_sync_writel((196UL | (1UL << 12) | (1UL << 13)), p + 4);
	} else {
		/* 0x1001C018[0] = 0 (GPS off) */
		mt_reg_sync_writel(tia_gps_on & ~1UL, p);
	}

	tia_gps_on1 = __raw_readl(p);
	tia_gps_ctrl1 = __raw_readl(p + 4);
	tia_temp1 = __raw_readl(p + 8);

	GDL_LOGI_INI(
		"on = %d, tia_gps_on = 0x%08x/0x%08x, ctrl = 0x%08x/0x%08x, temp = 0x%08x/0x%08x",
		gps_is_on, tia_gps_on, tia_gps_on1,
		tia_gps_ctrl, tia_gps_ctrl1,
		tia_temp, tia_temp1);
}

enum gps_dl_pinctrl_state_enum {
	GPS_DL_L1_LNA_DISABLE,
	GPS_DL_L1_LNA_DSP_CTRL,
	GPS_DL_L1_LNA_ENABLE,
	GPS_DL_L5_LNA_DISABLE,
	GPS_DL_L5_LNA_DSP_CTRL,
	GPS_DL_L5_LNA_ENABLE,
	GPS_DL_PINCTRL_STATE_CNT
};

const char *const gps_dl_pinctrl_state_name_list[GPS_DL_PINCTRL_STATE_CNT] = {
	"gps_l1_lna_disable",
	"gps_l1_lna_dsp_ctrl",
	"gps_l1_lna_enable",
	"gps_l5_lna_disable",
	"gps_l5_lna_dsp_ctrl",
	"gps_l5_lna_enable",
};

struct pinctrl_state *g_gps_dl_pinctrl_state_struct_list[GPS_DL_PINCTRL_STATE_CNT];
struct pinctrl *g_gps_dl_pinctrl_ptr;

void gps_dl_pinctrl_show_info(void)
{
	enum gps_dl_pinctrl_state_enum state_id;
	const char *p_name;
	struct pinctrl_state *p_state;

	GDL_LOGD_INI("pinctrl_ptr = 0x%p", g_gps_dl_pinctrl_ptr);

	for (state_id = 0; state_id < GPS_DL_PINCTRL_STATE_CNT; state_id++) {
		p_name = gps_dl_pinctrl_state_name_list[state_id];
		p_state = g_gps_dl_pinctrl_state_struct_list[state_id];
		GDL_LOGD_INI("state id = %d, ptr = 0x%p, name = %s",
			state_id, p_state, p_name);
	}
}

void gps_dl_pinctrl_context_init(void)
{
	enum gps_dl_pinctrl_state_enum state_id;
	const char *p_name;
	struct pinctrl_state *p_state;

	if (IS_ERR(g_gps_dl_pinctrl_ptr)) {
		GDL_LOGE_INI("pinctrl is error");
		return;
	}

	for (state_id = 0; state_id < GPS_DL_PINCTRL_STATE_CNT; state_id++) {
		p_name = gps_dl_pinctrl_state_name_list[state_id];
		p_state = pinctrl_lookup_state(g_gps_dl_pinctrl_ptr, p_name);

		if (IS_ERR(p_state)) {
			GDL_LOGE_INI("lookup fail: state id = %d, name = %s", state_id, p_name);
			g_gps_dl_pinctrl_state_struct_list[state_id] = NULL;
			continue;
		}

		g_gps_dl_pinctrl_state_struct_list[state_id] = p_state;
		GDL_LOGW_INI("lookup okay: state id = %d, name = %s", state_id, p_name);
	}
}

void gps_dl_lna_pin_ctrl(enum gps_dl_link_id_enum link_id, bool dsp_is_on, bool force_en)
{
	struct pinctrl_state *p_state = NULL;
	int ret;

	ASSERT_LINK_ID(link_id, GDL_VOIDF());

	if (GPS_DATA_LINK_ID0 == link_id) {
		if (dsp_is_on && force_en)
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L1_LNA_ENABLE];
		else if (dsp_is_on)
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L1_LNA_DSP_CTRL];
		else
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L1_LNA_DISABLE];
	}

	if (GPS_DATA_LINK_ID1 == link_id) {
		if (dsp_is_on && force_en)
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L5_LNA_ENABLE];
		else if (dsp_is_on)
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L5_LNA_DSP_CTRL];
		else
			p_state = g_gps_dl_pinctrl_state_struct_list[GPS_DL_L5_LNA_DISABLE];
	}

	if (p_state == NULL) {
		GDL_LOGXW(link_id, "on = %d, force = %d, state is null", dsp_is_on, force_en);
		return;
	}

	ret = pinctrl_select_state(g_gps_dl_pinctrl_ptr, p_state);
	if (ret != 0)
		GDL_LOGXW(link_id, "on = %d, force = %d, select ret = %d", dsp_is_on, force_en, ret);
	else
		GDL_LOGXD(link_id, "on = %d, force = %d, select ret = %d", dsp_is_on, force_en, ret);
}

bool gps_dl_get_iomem_by_name(struct platform_device *pdev, const char *p_name,
	struct gps_dl_iomem_addr_map_entry *p_entry)
{
	struct resource *regs;
	bool okay;

	regs = platform_get_resource_byname(pdev, IORESOURCE_MEM, p_name);
	if (regs != NULL) {
		p_entry->length = resource_size(regs);
		p_entry->host_phys_addr = regs->start;
		p_entry->host_virt_addr = devm_ioremap(&pdev->dev, p_entry->host_phys_addr, p_entry->length);
		okay = true;
	} else {
		p_entry->length = 0;
		p_entry->host_phys_addr = 0;
		p_entry->host_virt_addr = 0;
		okay = false;
	}

	GDL_LOGW_INI("phy_addr = 0x%08x, vir_addr = 0x%p, ok = %d, size = 0x%x, name = %s",
		p_entry->host_phys_addr, p_entry->host_virt_addr, okay, p_entry->length, p_name);

	return okay;
}

static int gps_dl_probe(struct platform_device *pdev)
{
	struct resource *irq;
	struct gps_each_device *p_each_dev0 = gps_dl_device_get(0);
	struct gps_each_device *p_each_dev1 = gps_dl_device_get(1);
	int i;
	bool okay;

	gps_dl_get_iomem_by_name(pdev, "conn_infra_base", &g_gps_dl_iomem_arrary[0]);
	gps_dl_get_iomem_by_name(pdev, "conn_gps_base", &g_gps_dl_iomem_arrary[1]);

	okay = gps_dl_get_iomem_by_name(pdev, "status_dummy_cr", &g_gps_dl_status_dummy_cr);
	if (okay)
		gps_dl_update_status_for_md_blanking(false);

	gps_dl_get_iomem_by_name(pdev, "tia_gps", &g_gps_dl_tia_gps);

	for (i = 0; i < GPS_DL_IRQ_NUM; i++) {
		irq = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (irq == NULL) {
			GDL_LOGE_INI("irq idx = %d, ptr = NULL!", i);
			continue;
		}

		GDL_LOGW_INI("irq idx = %d, start = %lld, end = %lld, name = %s, flag = 0x%x",
			i, irq->start, irq->end, irq->name, irq->flags);
		gps_dl_irq_set_id(i, irq->start);
	}

	g_gps_dl_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(g_gps_dl_pinctrl_ptr))
		GDL_LOGE_INI("devm_pinctrl_get fail");
	else {
		gps_dl_pinctrl_context_init();
		gps_dl_pinctrl_show_info();
	}

	GDL_LOGW_INI("do gps_dl_probe");
	platform_set_drvdata(pdev, p_each_dev0);
	p_each_dev0->private_data = (struct device *)&pdev->dev;
	p_each_dev1->private_data = (struct device *)&pdev->dev;

	gps_dl_device_context_init();

	return 0;
}

static int gps_dl_remove(struct platform_device *pdev)
{
	struct gps_each_device *p_each_dev = gps_dl_device_get(0);

	GDL_LOGW_INI("do gps_dl_remove");
	platform_set_drvdata(pdev, NULL);
	p_each_dev->private_data = NULL;
	return 0;
}

static int gps_dl_drv_suspend(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);
	pm_message_t state = PMSG_SUSPEND;

	return mtk_btif_suspend(pdev, state);
#endif
	return 0;
}

static int gps_dl_drv_resume(struct device *dev)
{
#if 0
	struct platform_device *pdev = to_platform_device(dev);

	return mtk_btif_resume(pdev);
#endif
	return 0;
}

static int gps_dl_plat_suspend(struct platform_device *pdev, pm_message_t state)
{
#if 0
	int i_ret = 0;
	struct _mtk_btif_ *p_btif = NULL;

	BTIF_DBG_FUNC("++\n");
	p_btif = platform_get_drvdata(pdev);
	i_ret = _btif_suspend(p_btif);
	BTIF_DBG_FUNC("--, i_ret:%d\n", i_ret);
	return i_ret;
#endif
	return 0;
}

static int gps_dl_plat_resume(struct platform_device *pdev)
{
#if 0
	int i_ret = 0;
	struct _mtk_btif_ *p_btif = NULL;

	BTIF_DBG_FUNC("++\n");
	p_btif = platform_get_drvdata(pdev);
	i_ret = _btif_resume(p_btif);
	BTIF_DBG_FUNC("--, i_ret:%d\n", i_ret);
	return i_ret;
#endif
	return 0;
}


const struct dev_pm_ops mtk_btif_drv_pm_ops = {
	.suspend = gps_dl_drv_suspend,
	.resume = gps_dl_drv_resume,
};

struct platform_driver gps_dl_dev_drv = {
	.probe = gps_dl_probe,
	.remove = gps_dl_remove,
/* #ifdef CONFIG_PM */
	.suspend = gps_dl_plat_suspend,
	.resume = gps_dl_plat_resume,
/* #endif */
	.driver = {
		.name = "gps", /* mediatek,gps */
		.owner = THIS_MODULE,
/* #ifdef CONFIG_PM */
		.pm = &mtk_btif_drv_pm_ops,
/* #endif */
/* #ifdef CONFIG_OF */
		.of_match_table = gps_dl_of_ids,
/* #endif */
	}
};

static ssize_t driver_flag_read(struct device_driver *drv, char *buf)
{
	return sprintf(buf, "gps dl driver debug level:%d\n", 1);
}

static ssize_t driver_flag_set(struct device_driver *drv,
				   const char *buffer, size_t count)
{
	GDL_LOGW_INI("buffer = %s, count = %zd", buffer, count);
	return count;
}

#define DRIVER_ATTR(_name, _mode, _show, _store) \
	struct driver_attribute driver_attr_##_name = \
	__ATTR(_name, _mode, _show, _store)
static DRIVER_ATTR(flag, 0644, driver_flag_read, driver_flag_set);


int gps_dl_linux_plat_drv_register(void)
{
	int result;
	gps_dl_wake_lock_init();

	result = platform_driver_register(&gps_dl_dev_drv);
	/* if (result) */
	GDL_LOGW_INI("platform_driver_register, ret(%d)\n", result);

	result = driver_create_file(&gps_dl_dev_drv.driver, &driver_attr_flag);
	/* if (result) */
	GDL_LOGW_INI("driver_create_file, ret(%d)\n", result);

	return 0;
}

int gps_dl_linux_plat_drv_unregister(void)
{
	driver_remove_file(&gps_dl_dev_drv.driver, &driver_attr_flag);
	platform_driver_unregister(&gps_dl_dev_drv);
	gps_dl_wake_lock_deinit();

	return 0;
}

static struct wakeup_source g_gps_dl_wake_lock;
void gps_dl_wake_lock_init(void)
{
	GDL_LOGD_INI("");
	wakeup_source_init(&g_gps_dl_wake_lock, "gpsdl_wakelock");
}

void gps_dl_wake_lock_deinit(void)
{
	GDL_LOGD_INI("");
	wakeup_source_trash(&g_gps_dl_wake_lock);
}

void gps_dl_wake_lock_hold(bool hold)
{
	GDL_LOGD_ONF("hold = %d", hold);
	if (hold)
		__pm_stay_awake(&g_gps_dl_wake_lock);
	else
		__pm_relax(&g_gps_dl_wake_lock);
}

#endif /* GPS_DL_HAS_PLAT_DRV */

