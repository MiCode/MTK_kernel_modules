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
#include "connv3_mt6989_pmic.h"
#include "consys_reg_util.h"

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
/* pmic for antenna */
static struct regulator *g_reg_VANT18 = NULL;

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

int connv3_plt_pmic_initial_setting_mt6989(struct platform_device *pdev, struct connv3_dev_cb* dev_cb);
int connv3_plt_pmic_common_power_ctrl_mt6989(u32 enable);
int connv3_plt_pmic_vsel_ctrl_mt6989(u32 enable);
int connv3_plt_pmic_parse_state_mt6989(char *buffer, int buf_sz);
int connv3_plt_pmic_get_connsys_chip_info_mt6989(char *connsys_ecid, int connsys_ecid_size);
int connv3_plt_pmic_get_pmic_chip_info_mt6989(char *pmic_ecid, int pmic_ecid_size);

const struct connv3_platform_pmic_ops g_connv3_platform_pmic_ops_mt6989 = {
	.pmic_initial_setting = connv3_plt_pmic_initial_setting_mt6989,
	.pmic_common_power_ctrl = connv3_plt_pmic_common_power_ctrl_mt6989,
	.pmic_vsel_ctrl = connv3_plt_pmic_vsel_ctrl_mt6989,
	.pmic_parse_state = connv3_plt_pmic_parse_state_mt6989,
	.pmic_get_connsys_chip_info = connv3_plt_pmic_get_connsys_chip_info_mt6989,
	.pmic_get_pmic_chip_info = connv3_plt_pmic_get_pmic_chip_info_mt6989,
};

struct work_struct g_pmic_faultb_work_mt6989;
unsigned int g_pmic_excep_irq_num_mt6989 = 0;
unsigned int g_spurious_pmic_exception_mt6989 = 1;
int g_faultb_gpio_mt6989 = 0, g_pmic_en_gpio_mt6989 = 0;
static irqreturn_t pmic_fault_handler(int irq, void * arg)
{
	if (g_spurious_pmic_exception_mt6989) {
		pr_info("[%s] g_spurious_pmic_exception_mt6989\n", __func__);
		return IRQ_HANDLED;
	}

	pr_err("[%s] Get PMIC FaultB interrupt\n", __func__);
	schedule_work(&g_pmic_faultb_work_mt6989);

	return IRQ_HANDLED;
}

static void check_faultb_status(struct work_struct *work)
{
	unsigned int faultb_level;

	mdelay(10);

	/* check FaultB level to avoid noise trigger */
	faultb_level = gpio_get_value(g_faultb_gpio_mt6989);
	pr_info("[%s] PMIC_EN=%d, faultb=%d\n",
		__func__, gpio_get_value(g_pmic_en_gpio_mt6989), faultb_level);
	if (faultb_level == 1)
		return;

	if (g_dev_cb != NULL && g_dev_cb->connv3_pmic_event_notifier != NULL)
		g_dev_cb->connv3_pmic_event_notifier(0, 1);
}


static void __iomem *vir_0x1000_5000 = NULL; /* GPIO */
static void __iomem *vir_0x11b3_0000 = NULL; /* IOCFG_TR */

static void _dump_pmic_gpio_state(void)
{
#define GET_BIT(V, INDEX) ((V & (0x1U << INDEX)) >> INDEX)
	unsigned int aux, dir, pu, pd, dout;

	if (vir_0x1000_5000 == NULL)
		vir_0x1000_5000 = ioremap(0x10005000, 0x1000);

	if (vir_0x11b3_0000 == NULL)
		vir_0x11b3_0000 = ioremap(0x11b30000, 0x1000);

	if (vir_0x1000_5000 == NULL || vir_0x11b3_0000 == NULL) {
		pr_notice("[%s] vir_0x1000_5000=%lx vir_0x11b3_0000=%lx",
			__func__, (unsigned long)vir_0x1000_5000, (unsigned long)vir_0x11b3_0000);
		return;
	}

	aux = CONSYS_REG_READ(vir_0x1000_5000 + 0x0430);
	dir = CONSYS_REG_READ(vir_0x1000_5000 + 0x0040);
	dout = CONSYS_REG_READ(vir_0x1000_5000 + 0x0140);

	pd = CONSYS_REG_READ(vir_0x11b3_0000 + 0x0080);
	pu = CONSYS_REG_READ(vir_0x11b3_0000 + 0x00a0);

	pr_info("[%s] GPIO 152= \taux=[%d]\tdir=[%s]\tDOUT=[%d] PD/PU=[%d/%d]", __func__,
		((aux & 0x7) >> 0), (GET_BIT(dir, 24)? "OUT" : "IN"), GET_BIT(dout, 24),
		GET_BIT(pd, 4), GET_BIT(pu, 4));
}

int connv3_plt_pmic_initial_setting_mt6989(struct platform_device *pdev, struct connv3_dev_cb* dev_cb)
{
#ifndef CONFIG_FPGA_EARLY_PORTING
#if COMMON_KERNEL_PMIC_SUPPORT
	#define SRCLKENRC_BASE_ADDR                 0x1c00d000
	#define SRCLKENRC_RC_CENTRAL_CFG1_OFFSET    0x4
	bool is_rc_mode = false;
	mapped_addr vir_addr_srclkenrc_base = NULL;
#endif
#endif
	struct pinctrl_state *pinctrl_faultb_init;
	int ret = 0;
	unsigned int irq_num = 0;

	g_dev_cb = dev_cb;
	g_faultb_gpio_mt6989 = of_get_named_gpio(pdev->dev.of_node, "mt6376-gpio", 0);
	g_pmic_en_gpio_mt6989 = of_get_named_gpio(pdev->dev.of_node, "mt6376-gpio", 1);

	INIT_WORK(&g_pmic_faultb_work_mt6989, check_faultb_status);

	g_reg_VANT18 = devm_regulator_get(&pdev->dev, "mt6373_vant18");
	if (IS_ERR(g_reg_VANT18)) {
		pr_notice("Regulator_get VANT18 fail\n");
		g_reg_VANT18 = NULL;
	}

	g_pinctrl_ptr = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(g_pinctrl_ptr)) {
		pr_err("[%s] Get pinctrl fail, %ld", __func__, PTR_ERR(g_pinctrl_ptr));
		return -1;
	}

	_dump_pmic_gpio_state();

	pinctrl_faultb_init = pinctrl_lookup_state(
			g_pinctrl_ptr, "connsys-pin-pmic-faultb-default");
	if (!IS_ERR(pinctrl_faultb_init)) {
		ret = pinctrl_select_state(g_pinctrl_ptr, pinctrl_faultb_init);
		if (ret) {
			pr_err("[%s] faultb init fail, %d", __func__, ret);
			return -1;
		}
	} else {
		pr_err("[%s] fail to get \"connsys-pin-pmic-faultb-default\"",  __func__);
		return -1;
	}

	irq_num = irq_of_parse_and_map(pdev->dev.of_node, 0);
	pr_info("[%s][%d], irqNum of CONNSYS = %d", __func__, __LINE__, irq_num);

	ret = devm_request_threaded_irq(&pdev->dev, irq_num, NULL,
				pmic_fault_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"MT6376_FAULT", platform_get_drvdata(pdev));
	if (ret) {
		pr_err("[%s][%d], request irq fail with irq_num=%d\n", __func__, __LINE__, irq_num);
		return ret;
	}
	g_pmic_excep_irq_num_mt6989 = irq_num;

#ifndef CONFIG_FPGA_EARLY_PORTING
#if COMMON_KERNEL_PMIC_SUPPORT
	/* Set RC VANT18 */
	vir_addr_srclkenrc_base = ioremap(SRCLKENRC_BASE_ADDR, 0x100);

	if (!vir_addr_srclkenrc_base) {
		pr_notice("[%s] vir_addr_srclkenrc_base(0x%x) ioremap fail\n", __func__,
			SRCLKENRC_BASE_ADDR);
	} else {
		is_rc_mode = (bool)CONSYS_REG_READ_BIT(vir_addr_srclkenrc_base +
			SRCLKENRC_RC_CENTRAL_CFG1_OFFSET, 0x1);
		if (is_rc_mode && g_connv3_regmap_mt6373) {
			/* Full HW signal control, only need config it. */
			/* 1. set PMIC VANT18 LDO PMIC HW mode control by PMRC_EN[10]
			 * 1.1. set PMIC VANT18 LDO op_mode = 0
			 * 1.2. set PMIC VANT18 LDO  HW_OP_EN = 1, HW_OP_CFG = 0
			 */
			regmap_update_bits(g_connv3_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_MODE_ADDR, 1 << 2, 0 << 2);
			regmap_update_bits(g_connv3_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_EN_ADDR,   1 << 2, 1 << 2);
			regmap_update_bits(g_connv3_regmap_mt6373,
				MT6373_RG_LDO_VANT18_RC10_OP_CFG_ADDR,  1 << 2, 0 << 2);
			pr_info("[%s] set rc vant18 done\n", __func__);
		}

		iounmap(vir_addr_srclkenrc_base);
	}
#endif
#endif

	return 0;
}

int connv3_plt_pmic_common_power_ctrl_mt6989(u32 enable)
{
	struct pinctrl_state *pinctrl_set;
	struct pinctrl_state *faultb_set;
	int ret = 0;

	if (enable) {
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

		g_spurious_pmic_exception_mt6989 = 0;
	} else {
		g_spurious_pmic_exception_mt6989 = 1;

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
	}

	pr_info("[%s] enable=[%d]", __func__, enable);

	return ret;
}

int connv3_plt_pmic_vsel_ctrl_mt6989(u32 enable)
{
	int ret = 0;
	struct pinctrl_state *vsel_set;

	if (enable) {
		vsel_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-vsel-low"); // vsel=L (3.85V)
		if (!IS_ERR(vsel_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, vsel_set);
			if (ret)
				pr_err("[%s] vsel low fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-vsel-low\"", __func__);
		}
	} else {
		vsel_set = pinctrl_lookup_state(
				g_pinctrl_ptr, "connsys-pin-pmic-vsel-high"); // vsel=H (3.45V)
		if (!IS_ERR(vsel_set)) {
			ret = pinctrl_select_state(g_pinctrl_ptr, vsel_set);
			if (ret)
				pr_err("[%s] vsel high fail, %d", __func__, ret);
		} else {
			pr_err("[%s] fail to get \"connsys-pin-pmic-vsel-high\"", __func__);
		}
	}

	pr_info("[%s] enable=[%d]", __func__, enable);
	return ret;
}

/*
 * need discuss how to get from WIFI driver
 *
 * Format:
 * register_dump[0~17]: slave: PMIC
 * register_dump[0] : 0x18[1,2,4,5]: OC of [BuckD, BuckIO, BuckR, BuckVB]
 * register_dump[1] : 0x1A[0:7]: OC of LDOs
 * register_dump[2] : 0x19[1,2,4,5]: PG of [BuckD, BuckIO, BuckR, BuckVB]
 * register_dump[3] : 0x1B[0:7]: PG of LDOs
 * register_dump[4] : 0x17[5:7]: Chip OTP, Chip OV, Chip UV
 *
 * register_dump[5] : 0xA0[3:4]: PG/OC mode of RFLDO
 * register_dump[6] : 0xA8[3:4]: PG/OC mode of HIOLDO
 * register_dump[7] : 0xB0[3:4]: PG/OC mode of PHYLDO
 * register_dump[8] : 0xB8[3:4]: PG/OC mode of IOLDO
 * register_dump[9] : 0xC0[3:4]: PG/OC mode of ALDO
 * register_dump[10]: 0xC8[3:4]: PG/OC mode of MLDO
 * register_dump[11]: 0xD0[3:4]: PG/OC mode of ANALDO
 * register_dump[12]: 0xD8[3:4]: PG/OC mode of PALDO
 *
 * register_dump[13]: 0x4A[0]: on UDS mode
 * register_dump[14]: 0x44: switch of i2c recording feature
 * register_dump[15]: 0x45: slave id: 0: slave PMIC, 1: slave BUCK, 2: slave TEST
 * register_dump[16]: 0x46: last ADDR
 * register_dump[17]: 0x47: last WDATA
 *
 * register_dump[18~20]: slave: BUCK
 * register_dump[18]: 0x1C[5:6]: PG/OC mode of BuckD
 * register_dump[19]: 0x24[5:6]: PG/OC mode of BuckIO
 * register_dump[20]: 0x2C[5:6]: PG/OC mode of BuckR
 */
#define PMIC_DUMP_REGISTER_SIZE 48

/* slave: PMIC, IC status */
#define PMIC_OTP_EVT		(0x1U << 5)
#define PMIC_SYSOV_EVT		(0x1U << 6)
#define PMIC_SYSUV_EVT		(0x1U << 7)
#define PMIC_STAT_FAIL		(PMIC_OTP_EVT | PMIC_SYSOV_EVT | PMIC_SYSUV_EVT)

/* slave: PMIC, BUCK OC info */
#define PMIC_BUCK_D_OC_EVT	(0x1U << 1)
#define PMIC_BUCK_IO_OC_EVT	(0x1U << 2)
#define PMIC_BUCK_R_OC_EVT	(0x1U << 4)
#define PMIC_PSW_VB_OC_EVT	(0x1U << 5)
#define PMIC_BUCK_OC		(PMIC_BUCK_D_OC_EVT | PMIC_BUCK_IO_OC_EVT | PMIC_BUCK_R_OC_EVT | PMIC_PSW_VB_OC_EVT)

/* slave: PMIC, LDO OC info */
#define PMIC_RFLDO_OC_EVT	(0x1U << 0)
#define PMIC_HIOLDO_OC_EVT	(0x1U << 1)
#define PMIC_PHYLDO_OC_EVT	(0x1U << 2)
#define PMIC_IOLDO_OC_EVT	(0x1U << 3)
#define PMIC_ALDO_OC_EVT	(0x1U << 4)
#define PMIC_MLDO_OC_EVT	(0x1U << 5)
#define PMIC_ANALDO_OC_EVT	(0x1U << 6)
#define PMIC_PALDO_OC_EVT	(0x1U << 7)
#define PMIC_LDO_OC		(PMIC_RFLDO_OC_EVT | PMIC_HIOLDO_OC_EVT | PMIC_PHYLDO_OC_EVT | PMIC_IOLDO_OC_EVT | PMIC_ALDO_OC_EVT | PMIC_MLDO_OC_EVT | PMIC_ANALDO_OC_EVT | PMIC_PALDO_OC_EVT)

/* slave: PMIC, BUCK PG info */
#define PMIC_BUCK_PG_EVT_ADDR 0x19

#define PMIC_BUCK_D_PGB_EVT	(0x1U << 1)
#define PMIC_BUCK_IO_PGB_EVT	(0x1U << 2)
#define PMIC_BUCK_R_PGB_EVT	(0x1U << 4)
#define PMIC_BUCK_PG		(PMIC_BUCK_D_PGB_EVT | PMIC_BUCK_IO_PGB_EVT | PMIC_BUCK_R_PGB_EVT)

/* slave: PMIC, LDO PG info */
#define PMIC_LDO_PG_EVT_ADDR 0x1B

#define PMIC_RFLDO_PGB_EVT	(0x1U << 0)
#define PMIC_HIOLDO_PGB_EVT	(0x1U << 1)
#define PMIC_PHYLDO_PGB_EVT	(0x1U << 2)
#define PMIC_IOLDO_PGB_EVT	(0x1U << 3)
#define PMIC_ALDO_PGB_EVT	(0x1U << 4)
#define PMIC_MLDO_PGB_EVT	(0x1U << 5)
#define PMIC_ANALDO_PGB_EVT	(0x1U << 6)
#define PMIC_PALDO_PGB_EVT	(0x1U << 7)
#define PMIC_LDO_PG		(PMIC_RFLDO_PGB_EVT | PMIC_HIOLDO_PGB_EVT | PMIC_PHYLDO_PGB_EVT | PMIC_IOLDO_PGB_EVT | PMIC_ALDO_PGB_EVT | PMIC_MLDO_PGB_EVT | PMIC_ANALDO_PGB_EVT | PMIC_PALDO_PGB_EVT)

/* slave: BUCK, BUCK op mode info */
#define BUCK_BUCK_OC_MODE_DBGFLAG	(0x1U << 6)
#define BUCK_BUCK_PG_MODE_DBGFLAG	(0x1U << 5)

/* slave: PMIC, LDO op mode info */
#define PMIC_LDO_OC_MODE_DBGFLAG	(0x1U << 4)
#define PMIC_LDO_PG_MODE_DBGFLAG	(0x1U << 3)

/* slave: PMIC, UDS enable status */
#define PMIC_UDS_EN_STAT_BIT		(0x1U << 0)

#define PMIC_STAT_SIZE  21

#define CHIP_ECIP_INFO_LENGTH  128
char connsys_chip_ecid[CHIP_ECIP_INFO_LENGTH];
bool connsys_chip_ecid_ready = false;
char connsys_pmic_ecid[CHIP_ECIP_INFO_LENGTH];
bool connsys_pmic_ecid_ready = false;

int connv3_plt_pmic_parse_state_mt6989(char *buffer, int buf_sz)
{
#define TMP_LOG_SIZE 128
#define MT6376_REG_SIZE 8
	u8 *register_dump;
	u8 pmic_stat = 0;
	u8 buck_oc_stat = 0, ldo_oc_stat = 0;
	u8 buck_pg_stat = 0, ldo_pg_stat = 0;
	u8 uds_status;
	u8 i2c_last_dev, i2c_last_addr, i2c_last_wdata;
	u8 log_len = 0;
	int i;
	u32 data_size = 0;
	char tmp[4];
	char log_buf[TMP_LOG_SIZE];
	int remain_size = TMP_LOG_SIZE - 1;
	static int g_first_dump = 1;
	int ret;
	const char *pmic_exception_type_str[MT6376_REG_SIZE] = {"", "", "", "", "", "OverTemperature ", "OverVoltage ", "UVLO "};
	const char *buck_name_str[MT6376_REG_SIZE] = {"", "BUCK_D ", "BUCK_IO ", "", "BUCK_R ", "PSW_VB ", "", ""};
	const char *ldo_name_str[MT6376_REG_SIZE] = {"RFLDO ", "HIOLDO ", "PHYLDO ", "IOLDO ", "ALDO ", "MLDO ", "ANALDO ", "PALDO "};

	if (!buffer){
		pr_err("[%s] PMIC dump register is NULL\n", __func__);
		return -1;
	}

	if (buf_sz < 4) {
		pr_err("[%s] PMIC dump register size = %d (!= %d)\n", __func__, buf_sz, PMIC_DUMP_REGISTER_SIZE);
		return -1;
	}

	/* 4 byte to describe number of PMIC registers */
	memcpy(&data_size, buffer, 4);
	pr_info("[%s] data size=[%d]", __func__, data_size);

	if (data_size != PMIC_STAT_SIZE) {
		pr_notice("[MT6376] incorrect state size=[%d]", data_size);
		return -1;
	}

	log_buf[0] = '\0';
	for (i = 0; i < PMIC_DUMP_REGISTER_SIZE - 4; i++) {
		if (snprintf(tmp, 4, "%02X ", buffer[i+4]) < 0)
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
		ret = snprintf(connsys_pmic_ecid, CHIP_ECIP_INFO_LENGTH, "[MT6376P_ECID][%02X, %02X]", buffer[26], buffer[27]);
		if (ret <= 0)
			pr_notice("%s snprintf fail", __func__);
		else
			connsys_pmic_ecid_ready = true;
	}
	if (connsys_chip_ecid_ready == false) {
		ret = snprintf(connsys_chip_ecid, CHIP_ECIP_INFO_LENGTH, "[MT6639A_ECID][%02X, %02X, %02X, %02X, %02X, %02X, %02X, %02X]",
			buffer[28], buffer[29], buffer[30], buffer[31], buffer[32],buffer[33], buffer[34], buffer[35]);
		if (ret <= 0)
			pr_notice("%s snprintf fail", __func__);
		else
			connsys_chip_ecid_ready = true;
	}

	register_dump = buffer + 4;

	/* 1. OT/OV/UV status */
	pmic_stat = register_dump[4] & (PMIC_STAT_FAIL);

	/* 2. check BUCK/LDO OC status */
	buck_oc_stat = register_dump[0] & PMIC_BUCK_OC;
	ldo_oc_stat = register_dump[1] & PMIC_LDO_OC;

	/* 3. check BUCK/LDO PG status */
	buck_pg_stat = register_dump[2] & PMIC_BUCK_PG;
	ldo_pg_stat = register_dump[3] & PMIC_LDO_PG;

	/* 4. UDS status */
	uds_status = register_dump[13] & (PMIC_UDS_EN_STAT_BIT);

	/* 5. check last i2c write command */
	/* i2c_last_dev: 0x00: write to PMIC, 0x01: write to BUCK, 0x10: write to TM */
	i2c_last_dev = register_dump[15];
	i2c_last_addr = register_dump[16];
	i2c_last_wdata = register_dump[17];

	if (g_first_dump && pmic_stat == PMIC_SYSUV_EVT
		&& buck_oc_stat == 0 && ldo_oc_stat == 0
		&& buck_pg_stat == 0 && ldo_pg_stat == 0) {
		pr_info("[%s] 1st time enable PMIC, UVLO happen before reboot.\n", __func__);
	} else if (pmic_stat
		|| buck_oc_stat || ldo_oc_stat
		|| buck_pg_stat || ldo_pg_stat) {

		pr_notice("[MT6376] EXCEPTION pmic=[%02X] oc=[%02X][%02X] pg=[%02X][%02X]",
				pmic_stat, buck_oc_stat, ldo_oc_stat, buck_pg_stat, ldo_pg_stat);

		/* 2.1. check "op mode when OC happen" */
		/* 3.1. check "op mode when PG happen" */
		if (buck_oc_stat || ldo_oc_stat || buck_pg_stat || ldo_pg_stat) {
			pr_notice("[MT6376] EXCEPTION op=%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
					(register_dump[18] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[19] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[20] & (BUCK_BUCK_OC_MODE_DBGFLAG | BUCK_BUCK_PG_MODE_DBGFLAG)),
					(register_dump[5] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[6] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[7] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[8] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[9] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[10] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[11] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)),
					(register_dump[12] & (PMIC_LDO_OC_MODE_DBGFLAG | PMIC_LDO_PG_MODE_DBGFLAG)));
		}

		if (pmic_stat) {
			log_buf[0] = '\0';
			for (i = 5; i <= 7; i++) {
				if (pmic_stat & (0x1 << i))
					strncat(log_buf, pmic_exception_type_str[i], strlen(pmic_exception_type_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION PMIC exception: %s", log_buf);
		}
		if (buck_oc_stat) {
			log_buf[0] = '\0';
			for (i = 1; i < 6; i++) {
				if (buck_oc_stat & (0x1 << i))
					strncat(log_buf, buck_name_str[i], strlen(buck_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION BUCK OC: %s", log_buf);
		}
		if (ldo_oc_stat) {
			log_buf[0] = '\0';
			for (i = 0; i < MT6376_REG_SIZE; i++) {
				if (ldo_oc_stat & (0x1 << i))
					strncat(log_buf, ldo_name_str[i], strlen(ldo_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION LDO OC: %s", log_buf);
		}
		if (buck_pg_stat) {
			log_buf[0] = '\0';
			/* There is no PSW_VB PG */
			for (i = 1; i < 5; i++) {
				if (buck_pg_stat & (0x1 << i))
					strncat(log_buf, buck_name_str[i], strlen(buck_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION BUCK PG: %s", log_buf);
		}
		if (ldo_pg_stat) {
			log_buf[0] = '\0';
			for (i = 0; i < MT6376_REG_SIZE; i++) {
				if (ldo_pg_stat & (0x1 << i))
					strncat(log_buf, ldo_name_str[i], strlen(ldo_name_str[i]));
			}
			pr_notice("[MT6376] EXCEPTION LDO PG: %s", log_buf);
		}

		pr_notice("[MT6376] EXCEPTION UDS=[%d], Last i2c write to %s, write 0x%x by 0x%x\n",
			uds_status, (i2c_last_dev == 0x0)?"PMIC":"BUCK", i2c_last_addr, i2c_last_wdata);

		log_buf[0] = '\0';
		log_len = snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "[MT6376] EXCEPTION: ");
		if (pmic_stat) {
			if (pmic_stat & PMIC_OTP_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "OT ");
			if (pmic_stat & PMIC_SYSOV_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "OV ");
			if (pmic_stat & PMIC_SYSUV_EVT)
				log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "UV ");
		}
		if (buck_oc_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "BUCK_OC %02X ", buck_oc_stat);

		if (ldo_oc_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "LDO_OC %02X ", ldo_oc_stat);

		if (buck_pg_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "BUCK_PG %02X ", buck_pg_stat);

		if (ldo_pg_stat)
			log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "LDO_PG %02X ", ldo_pg_stat);
		log_len += snprintf(log_buf + log_len, TMP_LOG_SIZE - log_len, "\n");

		osal_dbg_kernel_exception("Connv3", "%s", log_buf);
	}
	g_first_dump = 0;

	/* print UDS and I2C command for more info */
	pr_info("[%s] UDS status=[%d], Last i2c write to %s, write 0x%x by 0x%x\n",
			__func__, uds_status, (i2c_last_dev == 0x0)?"PMIC":"BUCK", i2c_last_addr, i2c_last_wdata);

	return 0;
}

int connv3_plt_pmic_get_connsys_chip_info_mt6989(char *connsys_ecid, int connsys_ecid_size)
{
	int ret = 0;

	if (connsys_chip_ecid_ready == true)
		strncpy(connsys_ecid, connsys_chip_ecid, connsys_ecid_size);

	return ret;
}

int connv3_plt_pmic_get_pmic_chip_info_mt6989(char *pmic_ecid, int pmic_ecid_size)
{
	int ret = 0;

	if (connsys_pmic_ecid_ready == true)
		strncpy(pmic_ecid, connsys_pmic_ecid, pmic_ecid_size);

	return ret;
}

