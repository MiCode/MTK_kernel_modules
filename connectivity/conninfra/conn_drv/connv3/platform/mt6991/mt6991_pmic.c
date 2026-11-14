// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME "@(%s:%d) " fmt, __func__, __LINE__

#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/jiffies.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/vmalloc.h>

#include "osal_dbg.h"
#include "connv3.h"
#include "conn_adaptor.h"
#include "connv3_hw.h"
#include "connv3_pmic_mng.h"
#include "consys_reg_util.h"
#include "connv3_mt6991_pmic.h"
#include "consys_reg_util.h"
#include "mt6376.h"

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
static struct connv3_dev_cb* g_dev_cb;
static struct pinctrl *g_pinctrl_ptr;
static struct pinctrl_state *g_pin_por_reset_trigger = NULL;
static struct pinctrl_state *g_pin_por_reset_done = NULL;
static bool g_por_reset_pin_init_done = false;
#define CHIP_ECIP_INFO_LENGTH  128
static char connsys_chip_ecid[CHIP_ECIP_INFO_LENGTH];
static bool connsys_chip_ecid_ready = false;
static char connsys_pmic_ecid[CHIP_ECIP_INFO_LENGTH];
static bool connsys_pmic_ecid_ready = false;
static char g_connsys_adie_chip_info[CHIP_ECIP_INFO_LENGTH];
static bool g_connsys_adie_chip_info_ready = false;
int fmd_flag = false;

struct pmic_addr_mask_value
{
    const u32 addr;
    const u32 mask;
    const u32 value;
    u32 bk_value;
};

struct pmic_addr_mask_value mt6688_BLE[] =
{
	{0x0004, 0xFF, 0x88},
	{0x0410, 0xFF, 0x10},
	{0x0606, 0xFF, 0x00},
	{0x0626, 0xFF, 0x00},
	{0x060E, 0xFF, 0x02},
	{0x0613, 0xFF, 0x02},
	{0x062E, 0xFF, 0x02},
	{0x0633, 0xFF, 0x02},
	{0x0609, 0xFF, 0x05},
	{0x0629, 0xFF, 0x05},
};

/* #define PMIC_EN1_USING_GPIO_MODE */
struct pmic_addr_mask_value mt6661AP_BLE[] =
{
	{0xA2F ,0xFF ,0x29}, // unlock cpskey
	{0xA30 ,0xFF ,0x47}, // unlock cpskey
	{0x3B4 ,0xFF ,0x9E}, // tma key
	{0x3B5 ,0xFF ,0x99}, // tma key
	{0x1D ,0x1F ,0x1F},
	{0x51 ,0xC0 ,0x40},
	{0x8E ,0x20 ,0x20},
	{0x94 ,0x20 ,0x20},
	{0xAD ,0x38 ,0x8},
	{0xB0 ,0x38 ,0x8},
	#ifndef PMIC_EN1_USING_GPIO_MODE
	{0xB6 ,0x38 ,0x18},  // Set AGPIO2 as PMIC_EN1 function
	#else
	{0xB6 ,0x38 ,0x00},  // Set AGPIO2 as GPIO function
	{0x8B ,0x02 ,0x02},  // Set AGPIO2 as OUTPUT
	{0xA3 ,0x02 ,0x02},  // Set AGPIO2 as HIGH
	#endif
	{0xC3 ,0x1 ,0x1},
	{0xC4 ,0x1 ,0x1},
	{0xCF ,0xC0 ,0x40},
	{0xD0 ,0xC ,0x4}, /* should remove after PTPL move this setting to PMIC initial setting */
	#if 0 /* move to PMIC initial setting, set as PORSTB */
	#ifndef PMIC_EN1_USING_GPIO_MODE
	{0xD1 ,0xC ,0x4},    // Set AGPIO2 mode reset as btrstb+wdtrstb
	#endif
	#endif
	{0xD2 ,0xC0 ,0x40},
	{0xD3 ,0xC ,0x4}, /* should remove after PTPL move this setting to PMIC initial setting */
	#if 0 /* move to PMIC initial setting, set as PORSTB */
	#ifndef PMIC_EN1_USING_GPIO_MODE
	{0xD4 ,0xC ,0x4},    // Set AGPIO2 cell reset as btrstb+wdtrstb
	#endif
	#endif
	{0x10F ,0x10 ,0x10},
	{0x112 ,0x4 ,0x4},
	{0x11B ,0x10 ,0x0},
	{0x120 ,0xC ,0x0},
	{0x12D ,0x55 ,0x0},
	{0x12E ,0x5 ,0x0},
	{0x147 ,0x1 ,0x0},
	{0x151 ,0x70 ,0x70},
	{0x187 ,0x6 ,0x6},
	{0x248 ,0xFF ,0x0},
	{0x923 ,0x1 ,0x1},
	{0x924 ,0x3 ,0x1},
	{0x925 ,0x10 ,0x10},
	{0x926 ,0x80 ,0x80},
	{0x92C ,0x3 ,0x3},
	{0x92D ,0x10 ,0x10},
	{0x92E ,0x80 ,0x80},
	{0x93E ,0x1 ,0x1},
	{0x94D ,0xFF ,0xFE},
	{0x956 ,0xFF ,0xFE},
	{0x95D ,0xFF ,0xFE},
	{0x95E ,0xFF ,0xFE},
	{0xA33 ,0x1 ,0x1},
	{0xA34 ,0x10 ,0x10},
	{0xA35 ,0x80 ,0x80},
	{0xA45 ,0x2 ,0x2},
	{0xA48 ,0x8 ,0x8},
	{0xB23 ,0xFF ,0x63},
	{0xB28 ,0xFF ,0x63},
	{0xB31 ,0xFF ,0x4F},
	{0xB39 ,0xFF ,0x64},
	{0xB3E ,0xFF ,0x0},
	{0xB47 ,0xFF ,0x14},
	{0x140B ,0x7 ,0x0},
	{0x140E ,0xF ,0xF},
	{0x1F87 ,0x1 ,0x0},
	{0xA2F ,0xFF ,0x0}, // lock cpskey
	{0xA30 ,0xFF ,0x0}, // lock cpskey
	{0x3B4 ,0xFF ,0x0}, // tma key
	{0x3B5 ,0xFF ,0x0}, // tma key
};

struct pmic_addr_mask_value mt6661BP_BLE[] =
{
	{0xA2F ,0xFF ,0x29}, // unlock cpskey
	{0xA30 ,0xFF ,0x47}, // unlock cpskey
	{0x3B4 ,0xFF ,0x9E}, // tma key
	{0x3B5 ,0xFF ,0x99}, // tma key
	{0x1D, 0x1F, 0x1F},
	{0xAD, 0x38, 0x8},
	{0xB6, 0x38, 0x10},
	{0xC3, 0x1, 0x1},
	{0xC4, 0x1, 0x1},
	{0xCF, 0xC0, 0x40},
	{0xD1, 0xC, 0x4},
	{0xD2, 0xC0, 0x40},
	{0xD4, 0xC, 0x4},
	{0x10F, 0x10, 0x10},
	{0x112, 0x4, 0x4},
	{0x11B, 0x10, 0x0},
	{0x120, 0xC, 0x0},
	{0x187, 0x6, 0x6},
	{0x248, 0xFF, 0x0},
	{0x249, 0xFF, 0x0},
	{0x24A, 0xFF, 0x0},
	{0x24B, 0xFF, 0x0},
	{0x24E, 0xFF, 0x0},
	{0x923, 0x1, 0x1},
	{0x924, 0x3, 0x1},
	{0x925, 0x10, 0x10},
	{0x927, 0x8, 0x8},
	{0x92C, 0x3, 0x3},
	{0x92D, 0x10, 0x10},
	{0x92F, 0x8, 0x8},
	{0x93E, 0x1, 0x1},
	{0x94D, 0xFF, 0xFE},
	{0x95A, 0xFF, 0xFE},
	{0xA33, 0x1, 0x1},
	{0xA34, 0x10, 0x10},
	{0xA36, 0x8, 0x8},
	{0xA48, 0x8, 0x8},
	{0xB23, 0xFF, 0x63},
	{0xB28, 0xFF, 0x63},
	{0xB35, 0xFF, 0x4F},
	{0xB39, 0xFF, 0x64},
	{0xB3E, 0xFF, 0x0},
	{0xB4B, 0xFF, 0x14},
	{0x140B, 0x7, 0x0},
	{0x140E, 0xF, 0xF},
	{0x1F87, 0x1, 0x0},
	{0x1FC7, 0x1, 0x0},
	{0x2047, 0x1, 0x0},
	{0x2087, 0x1, 0x0},
	{0x2107, 0x1, 0x0},
	{0xA2F ,0xFF ,0x0}, // lock cpskey
	{0xA30 ,0xFF ,0x0}, // lock cpskey
	{0x3B4 ,0xFF ,0x0}, // tma key
	{0x3B5 ,0xFF ,0x0}, // tma key
};

struct pmic_addr_mask_value mt6661AP_UDS[] =
{
	{0xA2F ,0xFF ,0x29}, // unlock cpskey
	{0xA30 ,0xFF ,0x47}, // unlock cpskey
	{0x3B4 ,0xFF ,0x9E}, // tma key
	{0x3B5 ,0xFF ,0x99}, // tma key
	{0x1D, 0x1F, 0x1F},
	{0x51, 0xC0, 0x40},
	{0x8E, 0x20, 0x20},
	{0x94, 0x20, 0x20},
	{0xAD, 0x38, 0x8},
	{0xC3, 0x1, 0x1},
	{0xC4, 0x1, 0x1},
	{0xCF, 0xC0, 0x40},
	{0xD2, 0xC0, 0x40},
	{0x10F, 0x10, 0x10},
	{0x112, 0x4, 0x4},
	{0x120, 0xC, 0x0},
	{0x12D, 0x55, 0x0},
	{0x12E, 0x5, 0x0},
	{0x147, 0x1, 0x0},
	{0x151, 0x70, 0x70},
	{0x187, 0x6, 0x6},
	{0x923, 0x1, 0x1},
	{0x924, 0x3, 0x1},
	{0x925, 0x10, 0x10},
	{0x926, 0x80, 0x80},
	{0xB23, 0xFF, 0x63},
	{0xB28, 0xFF, 0x63},
	{0xB31, 0xFF, 0x4F},
	{0xB39, 0xFF, 0x64},
	{0xB3E, 0xFF, 0x0},
	{0xB47, 0xFF, 0x14},
	{0x140B, 0x7, 0x0},
	{0x140E, 0xF, 0xF},
	{0x1454, 0x1, 0x0},
	{0x1455, 0x3, 0x2},
	{0x169C, 0x1F, 0x9},
	{0x16A1, 0x2, 0x0},
	{0x16A4, 0x14, 0x10},
	{0x16A7, 0x2, 0x0},
	{0x16AA, 0x14, 0x14},
	{0x16AD, 0x2, 0x0},
	{0x16B0, 0x14, 0x14},
	{0x1E52, 0x3, 0x2},
	{0x1E53, 0x1, 0x0},
	{0x2047, 0x3, 0x1},
	{0x204A, 0xC, 0x0},
	{0x204B, 0x80, 0x0},
	{0x206F, 0x90, 0x90},
	{0x2075, 0x10, 0x10},
	{0x207B, 0x10, 0x10},
	{0xA2F ,0xFF ,0x0}, // lock cpskey
	{0xA30 ,0xFF ,0x0}, // lock cpskey
	{0x3B4 ,0xFF ,0x0}, // tma key
	{0x3B5 ,0xFF ,0x0}, // tma key
};

struct pmic_addr_mask_value mt6661BP_UDS[] =
{
	{0xA2F ,0xFF ,0x29}, // unlock cpskey
	{0xA30 ,0xFF ,0x47}, // unlock cpskey
	{0x3B4 ,0xFF ,0x9E}, // tma key
	{0x3B5 ,0xFF ,0x99}, // tma key
	{0x1D, 0x1F, 0x1F},
	{0xAD, 0x38, 0x8},
	{0xC3, 0x1, 0x1},
	{0xC4, 0x1, 0x1},
	{0xCF, 0xC0, 0x40},
	{0xD2, 0xC0, 0x40},
	{0x10F, 0x10, 0x10},
	{0x112, 0x4, 0x4},
	{0x120, 0xC, 0x0},
	{0x187, 0x6, 0x6},
	{0x923, 0x1, 0x1},
	{0x924, 0x3, 0x1},
	{0x925, 0x10, 0x10},
	{0x927, 0x8, 0x8},
	{0xB23, 0xFF, 0x63},
	{0xB28, 0xFF, 0x63},
	{0xB35, 0xFF, 0x4F},
	{0xB39, 0xFF, 0x64},
	{0xB3E, 0xFF, 0x0},
	{0xB4B, 0xFF, 0x14},
	{0x140B, 0x7, 0x0},
	{0x140E, 0xF, 0xF},
	{0x1454, 0x1, 0x0},
	{0x1455, 0x3, 0x2},
	{0x169C, 0x1F, 0x9},
	{0x16A1, 0x3, 0x0},
	{0x16A4, 0x14, 0x10},
	{0x16A7, 0x3, 0x1},
	{0x16AA, 0x14, 0x14},
	{0x16AD, 0x3, 0x1},
	{0x16B0, 0x14, 0x14},
	{0x19AF, 0x2, 0x2},
	{0x1E52, 0x3, 0x2},
	{0x1E53, 0x1, 0x0},
	{0x2147, 0x3, 0x1},
	{0x214A, 0xC, 0x4},
	{0x214B, 0x80, 0x0},
	{0x216F, 0x90, 0x90},
	{0x2175, 0x10, 0x10},
	{0x217B, 0x10, 0x10},
	{0xA2F ,0xFF ,0x0}, // lock cpskey
	{0xA30 ,0xFF ,0x0}, // lock cpskey
	{0x3B4 ,0xFF ,0x0}, // tma key
	{0x3B5 ,0xFF ,0x0}, // tma key
};

void pmic_backup_and_write_array(struct regmap *regmap, struct pmic_addr_mask_value *setting_array, u32 array_size)
{
	int i;

	for (i = 0; i < array_size ; i++) {
		regmap_read(regmap, (setting_array+i)->addr, &((setting_array+i)->bk_value));
		regmap_update_bits(regmap, (setting_array+i)->addr, (setting_array+i)->mask, (setting_array+i)->value);
	}

	for(i = 0; 4*i+3 < array_size; i++) {
		pr_info("%s[%d], Register[%d]: 0x%04x=0x%04x, 0x%04x=0x%04x, 0x%04x=0x%04x, 0x%04x=0x%04x]\n", __func__, __LINE__, i
			, (setting_array+4*i)->addr, (setting_array+4*i)->bk_value
			, (setting_array+4*i+1)->addr, (setting_array+4*i+1)->bk_value
			, (setting_array+4*i+2)->addr, (setting_array+4*i+2)->bk_value
			, (setting_array+4*i+3)->addr, (setting_array+4*i+3)->bk_value);
	}
}

void pmic_backup_array_read_back(struct regmap *regmap, struct pmic_addr_mask_value *setting_array, u32 array_size)
{
	int i = 0;

	for (i = 0; i < array_size ; i++) {
		regmap_write(regmap, (setting_array+i)->addr, (setting_array+i)->bk_value);
	}
}


/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int connv3_plt_pmic_initial_setting_mt6991(struct platform_device *pdev, struct connv3_dev_cb* dev_cb);
int connv3_plt_pmic_common_power_ctrl_mt6991(u32 enable);
int connv3_plt_pmic_vsel_ctrl_mt6991(u32 enable);
int connv3_plt_pmic_parse_state_mt6991(char *buffer, int buf_sz);
int connv3_plt_pmic_get_connsys_chip_info_mt6991(char *connsys_ecid, int connsys_ecid_size);
int connv3_plt_pmic_get_connsys_adie_chip_info_mt6991(char *connsys_adie_chip_info, int connsys_adie_chip_info_size);
int connv3_plt_pmic_get_pmic_chip_info_mt6991(char *pmic_ecid, int pmic_ecid_size);
int connv3_plt_pmic_fmd_setting_mt6991(u32 enable);
int connv3_plt_pmic_pwr_rst_mt6991(void);
int connv3_plt_pmic_set_pmic_en0_mt6991(int enable);
int connv3_plt_pmic_set_pmic_en1_mt6991(int enable);

int connv3_plt_pmic_initial_setting_mt6991_mt6661(struct platform_device *pdev, struct connv3_dev_cb* dev_cb);
int connv3_plt_pmic_common_power_ctrl_mt6991_mt6661(u32 enable);

const struct connv3_platform_pmic_ops g_connv3_platform_pmic_ops_mt6991 = {
	.pmic_initial_setting = connv3_plt_pmic_initial_setting_mt6991,
	.pmic_common_power_ctrl = connv3_plt_pmic_common_power_ctrl_mt6991,
	.pmic_vsel_ctrl = connv3_plt_pmic_vsel_ctrl_mt6991,
	.pmic_parse_state = connv3_plt_pmic_parse_state_mt6991,
	.pmic_get_connsys_chip_info = connv3_plt_pmic_get_connsys_chip_info_mt6991,
	.pmic_get_connsys_adie_chip_info = connv3_plt_pmic_get_connsys_adie_chip_info_mt6991,
	.pmic_get_pmic_chip_info = connv3_plt_pmic_get_pmic_chip_info_mt6991,
	.pmic_pwr_rst = connv3_plt_pmic_pwr_rst_mt6991,
	.set_pmic_en0 = connv3_plt_pmic_set_pmic_en0_mt6991,
	.set_pmic_en1 = connv3_plt_pmic_set_pmic_en1_mt6991,
};

const struct connv3_platform_pmic_ops g_connv3_platform_pmic_ops_mt6991_mt6661 = {
	.pmic_initial_setting = connv3_plt_pmic_initial_setting_mt6991_mt6661,
	.pmic_common_power_ctrl = connv3_plt_pmic_common_power_ctrl_mt6991_mt6661,
	.pmic_vsel_ctrl = connv3_plt_pmic_vsel_ctrl_mt6991,
	.pmic_parse_state = connv3_plt_pmic_parse_state_mt6991,
	.pmic_get_connsys_chip_info = connv3_plt_pmic_get_connsys_chip_info_mt6991,
	.pmic_get_pmic_chip_info = connv3_plt_pmic_get_pmic_chip_info_mt6991,
	.pmic_fmd_setting = connv3_plt_pmic_fmd_setting_mt6991,
	.pmic_pwr_rst = connv3_plt_pmic_pwr_rst_mt6991,
};

struct work_struct g_pmic_faultb_work_mt6991;
//unsigned int g_pmic_excep_irq_num_mt6989 = 0;
unsigned int g_spurious_pmic_exception_mt6991 = 1;
int g_faultb_gpio_mt6991 = -1, g_pmic_en_gpio_mt6991 = -1;
static irqreturn_t pmic_fault_handler(int irq, void * arg)
{
#define PMIC_EXCEPTION_STRING_LEN	40
	static int pmic_exception_count = 0;
	char pmic_exception_string[PMIC_EXCEPTION_STRING_LEN];

	if (g_spurious_pmic_exception_mt6991) {
		pr_info("[%s] g_spurious_pmic_exception_mt6991\n", __func__);
		return IRQ_HANDLED;
	}

	pr_err("[%s] Get PMIC FaultB interrupt\n", __func__);
	if (snprintf(pmic_exception_string, PMIC_EXCEPTION_STRING_LEN,
		"[connv3][pmic]pmic_exception_count=%d\n", ++pmic_exception_count)) {
		conn_dbg_add_log(0, pmic_exception_string);
	} else {
		pr_info("[%s] save pmic_exception_string fail\n", __func__);
	}
	schedule_work(&g_pmic_faultb_work_mt6991);

	return IRQ_HANDLED;
}

static void check_faultb_status(struct work_struct *work)
{
	unsigned int faultb_level;

	mdelay(10);

	/* check FaultB level to avoid noise trigger */
	if ((g_faultb_gpio_mt6991 != -1) && (g_pmic_en_gpio_mt6991 != -1)) {
		faultb_level = gpio_get_value(g_faultb_gpio_mt6991);
		pr_info("[%s] PMIC_EN=%d, faultb=%d\n",
			__func__, gpio_get_value(g_pmic_en_gpio_mt6991), faultb_level);
		if (faultb_level == 1)
			return;
	}

	if (g_dev_cb != NULL && g_dev_cb->connv3_pmic_event_notifier != NULL)
		g_dev_cb->connv3_pmic_event_notifier(1, 1);
}


static int parse_pmic_register_once;
int connv3_plt_pmic_common_power_ctrl_mt6991(u32 enable)
{
	struct pinctrl_state *pinctrl_set;
	struct pinctrl_state *faultb_set;
	static int pmic_enabled = 0;
	int ret = 0;

	if (enable == 1 && pmic_enabled == 0) {
		pinctrl_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-en-set");
		if (!IS_ERR(pinctrl_set)) {
			mdelay(30);
			ret = pinctrl_select_state(g_pinctrl_ptr, pinctrl_set);
			if (ret)
				pr_err("[%s] pinctrl on fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-en-set\"",  __func__);
		}
		mdelay(20);

		faultb_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-faultb-enable");
		if (!IS_ERR(faultb_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, faultb_set);
			if (ret)
				pr_err("[%s] faultb on fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-faultb-enable\"",  __func__);
		}

		g_spurious_pmic_exception_mt6991 = 0;
		pmic_enabled = 1;
		pr_info("[%s] enable=[%d] Done\n", __func__, enable);
	} else if (enable == 0 && pmic_enabled == 1){
		if (fmd_flag == true) {
			pr_info("%s[%d] Don't turn off PMIC_EN0 in co-clock FMD device\n", __func__, __LINE__);
			return 0;
		}

		/* Wait BT/WIFI FW off */
		mdelay(210);

		g_spurious_pmic_exception_mt6991 = 1;
		parse_pmic_register_once = 0;

		faultb_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-faultb-default");
		if (!IS_ERR(faultb_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, faultb_set);
			if (ret)
				pr_err("[%s] faultb off fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-faultb-default\"",  __func__);
		}

		pinctrl_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-en-clr");
		if (!IS_ERR(pinctrl_set)) {
			/* Add a delay before pmic_en = 0 to make sure reset task is completed.
			 * According to experiment, 1 ms is enough.
			 * Use 20ms because the api is not accurate.
			 */
			mdelay(20);
			ret = pinctrl_select_state(g_pinctrl_ptr, pinctrl_set);
			if (ret)
				pr_err("[%s] pinctrl on fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-en-clr\"",	__func__);
		}

		pmic_enabled = 0;
		pr_info("[%s] enable=[%d] Done\n", __func__, enable);
	}

	return ret;
}

int connv3_plt_pmic_vsel_ctrl_mt6991(u32 enable)
{
	return 0;
}

int connv3_plt_pmic_parse_state_mt6991(char *buffer, int buf_sz)
{
#define TMP_LOG_SIZE 128
#define PMIC_DUMP_REGISTER_SIZE	240
	int i;
	char tmp[4];
	char log_buf[TMP_LOG_SIZE];
	int remain_size = TMP_LOG_SIZE - 1;
	int ret;
	unsigned int adie_chip_id = 0, ddie_chip_id = 0, ddie_hw_ver = 0;

	if (!buffer){
		pr_err("[%s] PMIC dump register is NULL\n", __func__);
		return -1;
	}

	if (buf_sz != PMIC_DUMP_REGISTER_SIZE) {
		pr_notice("[%s] buf_sz=%d, expect: %d\n", __func__, buf_sz, PMIC_DUMP_REGISTER_SIZE);
		return -1;
	}

	log_buf[0] = '\0';
	for (i = 0; i < PMIC_DUMP_REGISTER_SIZE - 4; i++) {
		if (snprintf(tmp, 4, "%02X ", buffer[4 + i]) < 0)
			pr_notice("[%s] snprintf error", __func__);

		strncat(log_buf, tmp, remain_size);
		remain_size -= 3;
		if (i > 0 && (i % 24 == 23)) {
			pr_info("[MT6376-State] %s", log_buf);
			log_buf[0] = '\0';
			remain_size = TMP_LOG_SIZE;
		}
	}
	if (strlen(log_buf) > 0)
		pr_info("[MT6376-State] %s", log_buf);

	if (connsys_pmic_ecid_ready == false) {
		ret = snprintf(connsys_pmic_ecid, CHIP_ECIP_INFO_LENGTH, "[MT6376P][%02X][%02X, %02X]", buffer[27], buffer[25], buffer[26]);
		if (ret <= 0)
			pr_notice("%s snprintf fail", __func__);
		else
			connsys_pmic_ecid_ready = true;
	}
	if (connsys_chip_ecid_ready == false) {
		ddie_chip_id = ((buffer[97] << 8) | (buffer[96]));
		ddie_hw_ver = ((buffer[99] << 8) | (buffer[98]));
		ret = snprintf(connsys_chip_ecid, CHIP_ECIP_INFO_LENGTH, "[MT6653][%04X, %04X][%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X]",
			ddie_chip_id, ddie_hw_ver,
			buffer[32], buffer[33], buffer[34], buffer[35], buffer[36],buffer[37], buffer[38], buffer[39]);
		if (ret <= 0)
			pr_notice("%s snprintf fail", __func__);
		else
			connsys_chip_ecid_ready = true;
	}
	if (g_connsys_adie_chip_info_ready == false) {
		adie_chip_id = ((buffer[103] << 24) | buffer[102] << 16 | (buffer[101] << 8) | buffer[100]);
		ret = snprintf(
			g_connsys_adie_chip_info, CHIP_ECIP_INFO_LENGTH -1,
			"[MT6653_ADIE][%08X][%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X]",
			adie_chip_id,
			buffer[88], buffer[89], buffer[90], buffer[91], buffer[92],buffer[93], buffer[94], buffer[95]);
		if (ret <= 0)
			pr_notice("[%s] snprintf adie info fail, ret = %d", __func__, ret);
		else
			g_connsys_adie_chip_info_ready = true;
	}

	if (parse_pmic_register_once == 1)
		return 0;

	connv3_pmic_parse_state_mt6376(buffer, buf_sz);
	parse_pmic_register_once = 1;

	return 0;
}

int connv3_plt_pmic_get_connsys_chip_info_mt6991(char *connsys_ecid, int connsys_ecid_size)
{
	if (connsys_chip_ecid_ready == true)
		strncpy(connsys_ecid, connsys_chip_ecid, connsys_ecid_size);

	return 0;
}

int connv3_plt_pmic_get_connsys_adie_chip_info_mt6991(char *connsys_adie_chip_info, int connsys_adie_chip_info_size)
{
	if (g_connsys_adie_chip_info_ready == true)
		strncpy(connsys_adie_chip_info, g_connsys_adie_chip_info, connsys_adie_chip_info_size);
	return 0;
}

int connv3_plt_pmic_get_pmic_chip_info_mt6991(char *pmic_ecid, int pmic_ecid_size)
{
	if (connsys_pmic_ecid_ready == true)
		strncpy(pmic_ecid, connsys_pmic_ecid, pmic_ecid_size);

	return 0;
}

int connv3_plt_pmic_fmd_setting_mt6991(u32 enable)
{
	int ret = 0;

	/* MT6688 setting on 20241015 */
	pmic_backup_and_write_array(g_connv3_regmap_mt6688, mt6688_BLE, sizeof(mt6688_BLE) / sizeof(mt6688_BLE[0]));

	/* mt6661AP setting on 20241009 */
	pmic_backup_and_write_array(g_connv3_regmap_mt6661_4, mt6661AP_BLE, sizeof(mt6661AP_BLE) / sizeof(mt6661AP_BLE[0]));

	/* mt6661AP UDS part */
	pmic_backup_and_write_array(g_connv3_regmap_mt6661_4, mt6661AP_UDS, sizeof(mt6661AP_UDS) / sizeof(mt6661AP_UDS[0]));

	/* mt6661BP setting on 20241009 */
	pmic_backup_and_write_array(g_connv3_regmap_mt6661_3, mt6661BP_BLE, sizeof(mt6661BP_BLE) / sizeof(mt6661BP_BLE[0]));

	/* mt6661BP UDS part */
	pmic_backup_and_write_array(g_connv3_regmap_mt6661_3, mt6661BP_UDS, sizeof(mt6661BP_UDS) / sizeof(mt6661BP_UDS[0]));

	fmd_flag = true;

	return ret;
}

int connv3_plt_pmic_pwr_rst_mt6991(void)
{
	int ret;

	if (fmd_flag == true) {
		pr_info("%s[%d] restore PMIC from BLE setting back to initial setting\n", __func__, __LINE__);
		pmic_backup_array_read_back(g_connv3_regmap_mt6688, mt6688_BLE, sizeof(mt6688_BLE) / sizeof(mt6688_BLE[0]));
		pmic_backup_array_read_back(g_connv3_regmap_mt6661_4, mt6661AP_BLE, sizeof(mt6661AP_BLE) / sizeof(mt6661AP_BLE[0]));
		pmic_backup_array_read_back(g_connv3_regmap_mt6661_4, mt6661AP_UDS, sizeof(mt6661AP_UDS) / sizeof(mt6661AP_UDS[0]));
		pmic_backup_array_read_back(g_connv3_regmap_mt6661_3, mt6661BP_BLE, sizeof(mt6661BP_BLE) / sizeof(mt6661BP_BLE[0]));
		pmic_backup_array_read_back(g_connv3_regmap_mt6661_3, mt6661BP_UDS, sizeof(mt6661BP_UDS) / sizeof(mt6661BP_UDS[0]));
		fmd_flag = false;
	}

	if (!g_por_reset_pin_init_done) {
		pr_notice("[%s] not init\n", __func__);
		return -1;
	}

	ret = pinctrl_select_state(g_pinctrl_ptr, g_pin_por_reset_trigger);
	if (ret)
		pr_notice("[%s] trigger fail, ret = %d\n", __func__, ret);
	else
		pr_info("[%s] trigger done\n", __func__);

	mdelay(30);
	ret = pinctrl_select_state(g_pinctrl_ptr, g_pin_por_reset_done);
	if (ret)
		pr_notice("[%s] recover fail, ret = %d\n", __func__, ret);
	else
		pr_info("[%s] recover done\n", __func__);

	return 0;
}

static int connv3_plt_pmic_init_por_rst_pin(void)
{
	int ret;

	/* check pinctrl */
	if (IS_ERR(g_pinctrl_ptr)) {
		pr_notice("[%s] Get pinctrl fail, %ld", __func__, PTR_ERR(g_pinctrl_ptr));
		return -1;
	}

	g_pin_por_reset_trigger = pinctrl_lookup_state(
		g_pinctrl_ptr, "connsys-pin-por-reset-trigger");
	g_pin_por_reset_done = pinctrl_lookup_state(
		g_pinctrl_ptr, "connsys-pin-por-reset-done");

	if (IS_ERR_OR_NULL(g_pin_por_reset_trigger) || IS_ERR_OR_NULL(g_pin_por_reset_done)) {
		pr_notice("[%s] g_pin_por_reset_trigger=%p g_pin_por_reset_done=%p\n",
			__func__, g_pin_por_reset_trigger, g_pin_por_reset_done);
		return -1;
	}

	g_por_reset_pin_init_done = true;

	/* PD por reset */
	ret = pinctrl_select_state(g_pinctrl_ptr, g_pin_por_reset_done);
	if (ret)
		pr_notice("[%s] POR_RST PD fail, ret = %d\n", __func__, ret);
	else
		pr_info("[%s] POR_RST PD\n", __func__);

	return 0;
}

int connv3_plt_pmic_set_pmic_en0_mt6991(int enable)
{
	struct pinctrl_state *pinctrl_set;
	int ret = 0;

	if (enable == 1) {
		pinctrl_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-en-set");
		if (!IS_ERR(pinctrl_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, pinctrl_set);
			if (ret)
				pr_info("[%s] pinctrl on fail, %d", __func__, ret);
		} else {
			pr_info("[%s] fail to get \"connsys-pin-pmic-en-set\"",  __func__);
		}

		pr_info("[%s] enable=[%d] Done\n", __func__, enable);
	} else if (enable == 0){
		pinctrl_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-en-clr");
		if (!IS_ERR(pinctrl_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, pinctrl_set);
			if (ret)
				pr_info("[%s] pinctrl on fail, %d", __func__, ret);
		} else {
			pr_info("[%s] fail to get \"connsys-pin-pmic-en-clr\"",	__func__);
		}

		pr_info("[%s] enable=[%d] Done\n", __func__, enable);
	}

	return ret;
}

int connv3_plt_pmic_set_pmic_en1_mt6991(int enable)
{
	// preserve this function, add log for debug
	pr_info("%s[%d], Do nothing\n", __func__, __LINE__);
	return 0;
}

int connv3_plt_pmic_initial_setting_mt6991(
	struct platform_device *pdev, struct connv3_dev_cb* dev_cb)
{
	int ret;
	unsigned int irq_num = 0;

	g_dev_cb = dev_cb;

	INIT_WORK(&g_pmic_faultb_work_mt6991, check_faultb_status);
	g_faultb_gpio_mt6991 = of_get_named_gpio(pdev->dev.of_node, "mt6376-gpio", 0);
	g_pmic_en_gpio_mt6991 = of_get_named_gpio(pdev->dev.of_node, "mt6376-gpio", 1);

	g_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(g_pinctrl_ptr)) {
		pr_notice("[%s] Get pinctrl fail, %ld", __func__, PTR_ERR(g_pinctrl_ptr));
		return -1;
	}

	/* Init por_rst pin */
	connv3_plt_pmic_init_por_rst_pin();

	irq_num = irq_of_parse_and_map(pdev->dev.of_node, 0);
	pr_info("[%s][%d], irqNum of CONNSYS = %d", __func__, __LINE__, irq_num);

	ret = devm_request_threaded_irq(&pdev->dev, irq_num, NULL,
				pmic_fault_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"MT6376_FAULT", platform_get_drvdata(pdev));
	if (ret) {
		pr_notice("[%s][%d], request irq fail with irq_num=%d\n", __func__, __LINE__, irq_num);
		return ret;
	}

	return 0;
}

int connv3_plt_pmic_common_power_ctrl_mt6991_mt6661(u32 enable)
{
	int ret;

	ret = connv3_plt_pmic_common_power_ctrl_mt6991(enable);

	return ret;
}


int connv3_plt_pmic_initial_setting_mt6991_mt6661(
	struct platform_device *pdev, struct connv3_dev_cb* dev_cb)
{
	int ret;

	ret = connv3_plt_pmic_initial_setting_mt6991(pdev, dev_cb);

	return ret;
}

