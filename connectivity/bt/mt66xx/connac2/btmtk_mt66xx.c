/*
 *  Copyright (c) 2016,2017 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

//#include "btmtk_drv.h"
#include <linux/kthread.h>
#include "btmtk_chip_if.h"
#include "btmtk_mt66xx_reg.h"
#include "btmtk_define.h"
#include "btmtk_main.h"

#include "conninfra.h"
#include "connsys_debug_utility.h"


/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/
#define WMT_CMD_HDR_LEN (4)

#define EMI_BGFSYS_MCU_START    0
#define EMI_BGFSYS_MCU_SIZE     0x100000 /* 1024*1024 */
#define EMI_BT_START            (EMI_BGFSYS_MCU_START + EMI_BGFSYS_MCU_SIZE)
#define EMI_BT_SIZE             0x129400 /* 1189*1024 */

#define POS_POLLING_RTY_LMT     10
#define LPCR_POLLING_RTY_LMT    1200 /* 200*n */
/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
struct fw_patch_emi_hdr {
	uint8_t date_time[16];
	uint8_t plat[4];
	uint16_t hw_ver;
	uint16_t sw_ver;
	uint8_t emi_addr[4];
	uint32_t subsys_id;
	uint8_t reserved[16];
};

/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
static const uint8_t WMT_OVER_HCI_CMD_HDR[] = { 0x01, 0x6F, 0xFC, 0x00 };


/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/
static uint8_t g_mcu_rom_patch_name[] = "soc3_0_ram_mcu_e1_hdr.bin";
static uint8_t g_bt_ram_code_name[] = "soc3_0_ram_bt_1_1_hdr.bin";


/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
int32_t bgfsys_fw_own_clr(void)
{
	uint32_t lpctl_cr;
	int32_t retry = LPCR_POLLING_RTY_LMT;
	uint32_t delay_us = 500;

	do {
		if ((retry % 200) == 0) {
			CONN_INFRA_REG_WRITEL(BGF_LPCTL, BGF_HOST_CLR_FW_OWN_B);
			if (retry < LPCR_POLLING_RTY_LMT)
				BTMTK_WARN("FW own clear failed in %d us, write again\n",
					       200*delay_us);
		}

		lpctl_cr = CONN_INFRA_REG_READL(BGF_LPCTL);
		BTMTK_WARN("lpctl_cr = 0x%08x\n", lpctl_cr);
		if (!(lpctl_cr & BGF_OWNER_STATE_SYNC_B))
			break;

		usleep_range(delay_us, 2*delay_us);
		retry --;
	} while (retry > 0);

	if (retry == 0) {
		BTMTK_ERR("FW own clear (Wakeup) failed!\n");
		return -1;
	}

	BTMTK_DBG("FW own clear (Wakeup) succeed\n");
	return 0;
}

int32_t bgfsys_fw_own_set(void)
{
	uint32_t irqstat_cr;
	int32_t retry = LPCR_POLLING_RTY_LMT;
	uint32_t delay_us = 500;

	do {
		if ((retry % 200) == 0) {
			CONN_INFRA_REG_WRITEL(BGF_LPCTL, BGF_HOST_SET_FW_OWN_B);
			if (retry < LPCR_POLLING_RTY_LMT)
				BTMTK_WARN("FW own set failed in %d us, write again\n",
					       200*delay_us);
		}

		/*
		 * As current H/W design, BGF_LPCTL.OWNER_STATE_SYNC bit will be automatically
		 * asserted by H/W once driver set FW own, not waiting for F/W's ack,
		 * which means F/W may still transmit data to host at that time.
		 * So we cannot check this bit for FW own done identity, use BGF_IRQ_FW_OWN_SET
		 * bit for instead, even on polling mode and the interrupt is masked.
		 *
		 * This is a work-around, H/W will change design on ECO.
		 */
		irqstat_cr = CONN_INFRA_REG_READL(BGF_IRQ_STAT2);
		if (irqstat_cr & BGF_IRQ_FW_OWN_SET_B) {
			/* Write 1 to clear IRQ */
			CONN_INFRA_REG_WRITEL(BGF_IRQ_STAT2, BGF_IRQ_FW_OWN_SET_B);
			break;
		}

		usleep_range(delay_us, 2*delay_us);
		retry --;
	} while (retry > 0);

	if (retry == 0) {
		BTMTK_ERR("FW own set (Sleep) failed!\n");
		return -1;
	}

	BTMTK_DBG("FW own set (Sleep) succeed\n");
	return 0;
}

static void bgfsys_cal_data_backup(
	uint32_t start_addr,
	uint8_t *cal_data,
	uint16_t data_len)
{
	uint32_t start_offset;

	/*
	 * The calibration data start address and ready bit address returned in WMT RF cal event
	 * should be 0x7C05XXXX from F/W's point of view (locate at ConnInfra sysram region),
	 * treat the low 3 bytes as offset to our remapped virtual base.
	 */
	start_offset = start_addr & 0x00FFFFFF;
	memcpy_fromio(cal_data, p_conn_infra_base_addr + start_offset, data_len);
}

static void bgfsys_cal_data_restore(uint32_t start_addr,
					 	uint32_t ready_addr,
						uint8_t *cal_data,
						uint16_t data_len)
{
	uint32_t start_offset, ready_offset;

	start_offset = start_addr & 0x00FFFFFF;
	ready_offset = ready_addr & 0x00FFFFFF;
	memcpy_toio(p_conn_infra_base_addr + start_offset, cal_data, data_len);
	/* Write 1 to ready bit address (32 bits) to mark calibration data is ready */
	/* Firmware will not do calibration again when BT func on */
	CONN_INFRA_REG_WRITEL(ready_offset, 1);
}


static int32_t bgfsys_power_on(void)
{
	uint32_t value;
	int32_t retry = POS_POLLING_RTY_LMT;
	uint32_t delay_us = 100;
	uint32_t mcu_idle;;

	/* reset n10 cpu core */
	CONN_INFRA_CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST,
			   BGF_CPU_SW_RST_B);
	/* power on bgfsys */
	CONN_INFRA_SET_BIT(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL,
			   BGF_PWR_CTL_B);
#ifndef __SIMFEX__
	/* enable bt function en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0,
			   BT_FUNC_EN_B);
#endif
	/* enable bt function en */
	CONN_INFRA_SET_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0,
			   BT_FUNC_EN_B);

	/* polling bgfsys power ack bits until they are asserted */
	do {
		value = BGF_PWR_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_ACK_ST);
		BTMTK_INFO("bgfsys power ack = 0x%08x\n", value);
		retry--;
	} while (value != BGF_PWR_ACK_B && retry > 0);

	if (0 == retry)
		return -1;

	/* disable conn2bt slp_prot rx en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL,
			   CONN2BT_SLP_PROT_RX_EN_B);

	/* polling conn2bt slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
		BTMTK_INFO("conn2bt slp_prot rx ack = 0x%08x\n", value);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry)
		return -1;

	/* disable conn2bt slp_prot tx en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL,
			   CONN2BT_SLP_PROT_TX_EN_B);
	/* polling conn2bt slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
		BTMTK_INFO("conn2bt slp_prot tx ack = 0x%08x\n", value);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry)
		return -1;

	/* disable bt2conn slp_prot rx en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL,
			   BT2CONN_SLP_PROT_RX_EN_B);
	/* polling bt2conn slp_prot rx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_INFO("bt2conn slp_prot rx ack = 0x%08x\n", value);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry)
		return -1;

	/* disable bt2conn slp_prot tx en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL,
			   BT2CONN_SLP_PROT_TX_EN_B);
	/* polling bt2conn slp_prot tx ack until it is cleared */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_INFO("bt2conn slp_prot tx ack = 0x%08x\n", value);
		retry--;
	} while (value != 0 && retry > 0);

	if (0 == retry)
		return -1;

	usleep_range(200, 220);

	/* read bgfsys hw_version */
	value = BGFSYS_REG_READL(BGF_HW_VERSION);
	BTMTK_INFO("bgfsys hw version id = 0x%08x\n", value);
	if (value != BGF_HW_VER_ID)
		return -1;

	/* read bgfsys fw_version */
	value = BGFSYS_REG_READL(BGF_FW_VERSION);
	BTMTK_INFO("bgfsys fw version id = 0x%08x\n", value);
	if (value != BGF_FW_VER_ID)
		return -1;

	/* read bgfsys hw_code */
	value = BGFSYS_REG_READL(BGF_HW_CODE);
	BTMTK_INFO("bgfsys hw code = 0x%08x\n", value);
	if (value != BGF_HW_CODE_ID)
		return -1;

	/* read and check bgfsys version id */
	value = BGFSYS_REG_READL(BGF_IP_VERSION);
	BTMTK_INFO("bgfsys version id = 0x%08x\n", value);
	if (value != BGF_IP_VER_ID)
		return -1;

	/* clear con_cr_ahb_auto_dis */
	BGFSYS_CLR_BIT(BGF_MCCR, BGF_CON_CR_AHB_AUTO_DIS);
	/* set con_cr_ahb_stop */
	BGFSYS_REG_WRITEL(BGF_MCCR_SET, BGF_CON_CR_AHB_STOP);

	/* Todo: ?
	 * 1. Enable A-die tip_ck_en (use common API)
	 * 2. Set A-die Batch CMD & clock buffer setting as full mode (use common API)
	 */

	/* release n10 cpu core */
	CONN_INFRA_SET_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST, BGF_CPU_SW_RST_B);

	/*
	 * polling BGFSYS MCU sw_dbg_ctl cr to wait it becomes 0x1D1E,
	 * which indicates that the power-on part of ROM is completed.
	 */
	retry = POS_POLLING_RTY_LMT;
	do {
		mcu_idle = BGFSYS_REG_READL(BGF_MCU_CFG_SW_DBG_CTL);
		BTMTK_INFO("MCU sw_dbg_ctl = 0x%08x\n", mcu_idle);
		if (0x1D1E == mcu_idle)
			break;

		usleep_range(delay_us, 2*delay_us);
		retry --;
	} while (retry > 0);

	if (retry == 0)
		return -1;

	return 0;
}

static int32_t bgfsys_power_off(void)
{
	uint32_t value = 0;
	int32_t retry = POS_POLLING_RTY_LMT;

	/* enable bt2conn slp_prot tx en */
	CONN_INFRA_SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL,
			   BT2CONN_SLP_PROT_TX_EN_B);
	/* polling bt2conn slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_TX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_DBG("bt2conn slp_prot tx ack = 0x%08x\n", value);
		retry--;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		return -1;

	/* enable bt2conn slp_prot rx en */
	CONN_INFRA_SET_BIT(CONN_INFRA_BT2CONN_GALS_SLP_CTL,
			   BT2CONN_SLP_PROT_RX_EN_B);
	/* polling bt2conn slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = BT2CONN_SLP_PROT_RX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_DBG("bt2conn slp_prot rx ack = 0x%08x\n", value);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		return -1;


	/* enable conn2bt slp_prot tx en */
	CONN_INFRA_SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL,
			   CONN2BT_SLP_PROT_TX_EN_B);
	/* polling conn2bt slp_prot tx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_TX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
		BTMTK_DBG("conn2bt slp_prot tx ack = 0x%08x\n", value);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		return -1;


	/* enable conn2bt slp_prot rx en */
	CONN_INFRA_SET_BIT(CONN_INFRA_CONN2BT_GALS_SLP_CTL,
			   CONN2BT_SLP_PROT_RX_EN_B);
	/* polling conn2bt slp_prot rx ack until it is asserted */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN2BT_SLP_PROT_RX_ACK_B &
			CONN_INFRA_REG_READL(CONN_INFRA_CONN2BT_GALS_SLP_CTL);
		BTMTK_DBG("conn2bt slp_prot rx ack = 0x%08x\n", value);
		retry --;
	} while (value == 0 && retry > 0);

	if (retry == 0)
		return -1;

	/* Todo: ?
	 * 1. Enable A-die tip_ck_en (use common API)
	 * 2. Set A-die Batch CMD & clock buffer setting as full mode/BT only (use common API)
	 * 3. Disable A-die tip_ck_en (use common API)
	 */

	/* disable bt function en */
	CONN_INFRA_CLR_BIT(CONN_INFRA_CFG_BT_PWRCTLCR0,
			   BT_FUNC_EN_B);

	/* power off bgfsys */
	CONN_INFRA_CLR_BIT(CONN_INFRA_RGU_BGFSYS_ON_TOP_PWR_CTL,
			   BGF_PWR_CTL_B);

	/* reset n10 cpu core */
	CONN_INFRA_CLR_BIT(CONN_INFRA_RGU_BGFSYS_CPU_SW_RST,
			   BGF_CPU_SW_RST_B);

	return 0;
}


static int32_t __download_patch_to_emi(
		uint8_t *patch_name,
		uint32_t emi_start,
		uint32_t emi_size
	)
{
#if 1
	int32_t ret = 0;
	struct fw_patch_emi_hdr *p_patch_hdr = NULL;
	uint8_t *p_buf = NULL;
	uint32_t patch_size;
	uint8_t c_data_time[16];
	uint16_t hw_ver = 0;
	uint16_t sw_ver = 0;
	uint32_t subsys_id = 0;
	uint32_t patch_emi_offset = 0;
	phys_addr_t emi_ap_phy_base;
	uint8_t *p_patch_addr;

	/*  Read Firmware patch content */
	btmtk_load_code_from_bin(&p_buf, patch_name, NULL, &patch_size);
	if (patch_size < sizeof(struct fw_patch_emi_hdr)) {
		BTMTK_ERR("patch size %u error\n", patch_size);
		ret = -EINVAL;
		goto done;
	}

	/* Patch binary format:
	 * |<-EMI header: 48 Bytes->|<-BT/MCU header: 48 Bytes ->||<-patch body: X Bytes ----->|
	 */
	/* Check patch header information */
	p_patch_hdr = (struct fw_patch_emi_hdr *)p_buf;

	hw_ver = p_patch_hdr->hw_ver;
	sw_ver = p_patch_hdr->sw_ver;
	subsys_id = p_patch_hdr->subsys_id;
	strncpy(c_data_time, p_patch_hdr->date_time, sizeof(p_patch_hdr->date_time));
	c_data_time[15] = '\0';
	BTMTK_INFO(
		"[Patch]BTime=%s,HVer=0x%04x,SVer=0x%04x,Plat=%c%c%c%c,Addr=0x%02x%02x%02x%02x,Type=%x\n",
		c_data_time,
		((hw_ver & 0x00ff) << 8) | ((hw_ver & 0xff00) >> 8),
		((sw_ver & 0x00ff) << 8) | ((sw_ver & 0xff00) >> 8),
		p_patch_hdr->plat[0], p_patch_hdr->plat[1],
		p_patch_hdr->plat[2], p_patch_hdr->plat[3],
		p_patch_hdr->emi_addr[3], p_patch_hdr->emi_addr[2],
		p_patch_hdr->emi_addr[1], p_patch_hdr->emi_addr[0],
		subsys_id);

	/* Remove EMI header:
	 * |<-BT/MCU header: 48 Bytes ->||<-patch body: X Bytes ----->|
	 */
	patch_size -= sizeof(struct fw_patch_emi_hdr);
	p_buf += sizeof(struct fw_patch_emi_hdr);

	/*
	 * The EMI entry address given in patch header should be 0xFXXXXXXX
	 * from F/W's point of view, treat the middle 2 bytes as offset,
	 * the actual phy base should be dynamically allocated and provided
	 * by conninfra driver.
	 *
	 */
	patch_emi_offset = (p_patch_hdr->emi_addr[2] << 16) |
		(p_patch_hdr->emi_addr[1] << 8);

	conninfra_get_phy_addr((uint32_t*)&emi_ap_phy_base, NULL);
	emi_ap_phy_base &= 0xFFFFFFFF;

	if ((patch_emi_offset >= emi_start) &&
	    (patch_emi_offset + patch_size < emi_start + emi_size)) {
		p_patch_addr = ioremap_nocache(emi_ap_phy_base + patch_emi_offset, patch_size);
		if (p_patch_addr) {
			memcpy_toio(p_patch_addr, p_buf, patch_size);
			iounmap(p_patch_addr);
		} else {
			BTMTK_ERR("ioremap_nocache fail!\n");
			ret = -EFAULT;
		}
	} else {
		BTMTK_ERR("emi_start =0x%x size=0x%x\n", emi_start, emi_size);
		BTMTK_ERR("Patch overflow on EMI, offset=0x%x size=0x%x\n",
			      patch_emi_offset, patch_size);
		ret = -EINVAL;
	}

done:
	kfree(p_buf);
	return ret;
#else
	return 0;
#endif
}

static int32_t bgfsys_mcu_rom_patch_dl(void)
{
	uint32_t value = 0, retry = 0;

	/* wake up conn_infra off */
	CONN_INFRA_SET_BIT(CONN_INFRA_WAKEUP_BT, BIT(0));

	/* check ap2conn slpprot_rdy */
	retry = POS_POLLING_RTY_LMT;
	do {
		value = CONN_INFRA_ON2OFF_SLP_PROT_ACK &
			CONN_INFRA_REG_READL(CONN_INFRA_BT2CONN_GALS_SLP_CTL);
		BTMTK_DBG("ap2conn slpprot_rdy = 0x%08x", value);
		usleep_range(1000, 1100);
		retry--;
	} while (value != 0 && retry > 0);

	if (retry == 0)
		return -1;

	/* check conn_infra off ID */
	value = CONN_INFRA_REG_READL(CONN_INFRA_CFG_VERSION);
	BTMTK_DBG("connifra cfg version = 0x%08x", value);
	if (value != CONN_INFRA_CFG_ID)
		return -1;

	return __download_patch_to_emi(g_mcu_rom_patch_name,
				      EMI_BGFSYS_MCU_START,
				      EMI_BGFSYS_MCU_SIZE);
}

static int32_t bgfsys_bt_ram_code_dl(void)
{
	return __download_patch_to_emi(g_bt_ram_code_name, EMI_BT_START, EMI_BT_SIZE);
}

static int32_t bt_hw_and_mcu_on(void)
{
	int ret = -1;

	/*
	 * Firmware patch download (ROM patch, RAM code...)
	 * start MCU to enter idle loop after patch ready
	 */
	ret = bgfsys_mcu_rom_patch_dl();
	if (ret)
		goto power_on_error;

	ret = bgfsys_bt_ram_code_dl();
	if (ret)
		goto power_on_error;


	/* BGFSYS hardware power on */
	if (bgfsys_power_on()) {
		BTMTK_ERR("BGFSYS power on failed!\n");
		ret = -EIO;
		goto power_on_error;
	}

	/* Register all needed IRQs by MCU */
	ret = bt_request_irq(BGF2AP_BTIF_WAKEUP_IRQ);
	if (ret)
		goto request_irq_error;

	//bt_disable_irq(BGF2AP_BTIF_WAKEUP_IRQ);

	ret = bt_request_irq(BGF2AP_SW_IRQ);
	if (ret)
		goto request_irq_error2;

	//bt_disable_irq(BGF2AP_SW_IRQ);

	btmtk_reset_init();

	if (btmtk_btif_open()) {
		ret = -EIO;
		goto bus_operate_error;
	}
	return 0;


bus_operate_error:
	bt_free_irq(BGF2AP_SW_IRQ);

request_irq_error2:
	bt_free_irq(BGF2AP_BTIF_WAKEUP_IRQ);

request_irq_error:
	bgfsys_power_off();

power_on_error:

	return ret;
}

static void bt_hw_and_mcu_off(void)
{
	BTMTK_INFO("%s", __func__);
	/* Close hardware bus interface */
	btmtk_btif_close();

	/* Free all registered IRQs */
	bt_free_irq(BGF2AP_SW_IRQ);
	bt_free_irq(BGF2AP_BTIF_WAKEUP_IRQ);

	/* BGFSYS hardware power off */
	bgfsys_power_off();

	/* release resource */
	// bt_hw_ctrl_relax();
}

int32_t btmtk_send_wmt_reset_cmd(struct hci_dev *hdev)
{
	/* Support MT66xx*/
	uint8_t cmd[] =  {0x01, 0x5B, 0xFD, 0x00};

	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

#if 0
int32_t btmtk_send_utc_sync_cmd(struct hci_dev *hdev)
{
	uint8_t cmd[] =  {0x01, 0xF0, 0x09, 0x00, 0x02
			  0x00, 0x00, 0x00, 0x00,	/* UTC time second unit */
			  0x00, 0x00, 0x00, 0x00};	/* UTC time microsecond unit*/
	uint32_t sec, usec;
	/* uint8_t evt[] = {0x04, 0xE4, 0x06, 0x02, 0xF0, 0x02, 0x00, 0x02, 0x00}; */

	connsys_log_get_utc_time(&sec, &usec);
	memcpy(cmd + 5, &sec, sizeof(uint32_t));
	memcpy(cmd + 5 + sizeof(uint32_t), &usec, sizeof(uint32_t));
	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);
	BTMTK_INFO("%s done", __func__);
	return 0;
}

int32_t btmtk_send_blank_status_cmd(struct hci_dev *hdev)
{
	uint8_t cmd[] =  { 0x01, 0xF0, 0x02, 0x00, 0x03, 0x00 };
	/* uint8_t evt[] = {0x04, 0xE4, 0x06, 0x02, 0xF0, 0x02, 0x00, 0x03, 0x00}; */

	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);
	BTMTK_INFO("%s done", __func__);
	return 0;
}
#endif

#if 0
static int btmtk_send_gating_disable_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x03, 0x01, 0x00, 0x04 };
	/* To-Do, for event check */
	/* u8 event[] = { 0x04, 0xE4, 0x06, 0x02, 0x03, 0x02, 0x00, 0x00, 0x04 }; */
	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}

static int btmtk_send_read_conn_cmd(struct hci_dev *hdev)
{
	u8 cmd[] = { 0x01, 0x6F, 0xFC, 0x05, 0x01, 0x04, 0x01, 0x00, 0x07 };
	/* To-Do, for event check */
	/* u8 event[] = { 0x04, 0xE4, 0x0A, 0x02, 0x04, 0x06, 0x00, 0x00, 0x07, 0x00, 0x8a, 0x00, 0x00 }; */
	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}
#endif

static int32_t _send_wmt_power_cmd(struct hci_dev *hdev, u_int8_t is_on)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct wmt_pkt wmt_cmd;
	uint8_t buffer[HCI_MAX_FRAME_SIZE];
	uint16_t param_len, pkt_len;
	struct bt_internal_cmd *p_inter_cmd = &bdev->internal_cmd;
	int ret;

	BTMTK_INFO("%s", __func__);
	wmt_cmd.hdr.dir = WMT_PKT_DIR_HOST_TO_CHIP;

	/* Support Connac 2.0 */
	wmt_cmd.hdr.opcode = WMT_OPCODE_FUNC_CTRL;
	wmt_cmd.params.u.func_ctrl_cmd.subsys = 0;
	wmt_cmd.params.u.func_ctrl_cmd.on = is_on ? 1 : 0;
	param_len = sizeof(wmt_cmd.params.u.func_ctrl_cmd);

	wmt_cmd.hdr.param_len[0] = (uint8_t)(param_len & 0xFF);
	wmt_cmd.hdr.param_len[1] = (uint8_t)((param_len >> 8) & 0xFF);

	pkt_len = HCI_CMD_HDR_LEN + WMT_CMD_HDR_LEN + param_len;
	memcpy(buffer, WMT_OVER_HCI_CMD_HDR, HCI_CMD_HDR_LEN);
	buffer[3] = WMT_CMD_HDR_LEN + param_len;
	memcpy(buffer + HCI_CMD_HDR_LEN, &wmt_cmd, WMT_CMD_HDR_LEN + param_len);

	bdev->event_intercept = TRUE;
	p_inter_cmd->waiting_event = 0xE4;
	p_inter_cmd->pending_cmd_opcode = 0xFC6F;
	p_inter_cmd->wmt_opcode = WMT_OPCODE_FUNC_CTRL;
	p_inter_cmd->result = 0;

	btmtk_main_send_cmd(hdev, buffer, pkt_len, BTMTKUART_TX_WAIT_VND_EVT);
	bdev->event_intercept = FALSE;
	BTMTK_INFO("WMT func ctrl event return with %s\n",
		       (p_inter_cmd->result) ? "success" : "failure");
		ret = (p_inter_cmd->result) ? 0 : -EIO;

	return ret;
}

int32_t btmtk_send_wmt_power_on_cmd(struct hci_dev *hdev)
{
	int ret;
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	ret = _send_wmt_power_cmd(hdev, TRUE);
	if (!ret) {
		BTMTK_INFO("BT Func on success");
		bdev->bt_state = FUNC_ON;
	} else
		BTMTK_ERR("BT Func on fail");

	return ret;
}

int32_t btmtk_send_wmt_power_off_cmd(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	int ret = 0;

	BTMTK_INFO("%s", __func__);
	/* Do not send CMD to connsys while connsys is resetting */
	if (bdev->bt_state != RESET_START)
		ret = _send_wmt_power_cmd(hdev, FALSE);

	bdev->bt_state = FUNC_OFF;
	BTMTK_INFO("%s: Done", __func__);
	return ret;
}

static int32_t btmtk_get_cal_data_ref(
	struct hci_dev *hdev,
	uint32_t *p_start_addr,
	uint32_t *p_ready_addr,
	uint16_t *p_data_len
)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	struct wmt_pkt wmt_cmd;
	uint8_t buffer[HCI_MAX_FRAME_SIZE];
	uint16_t param_len, pkt_len;
	struct bt_internal_cmd *p_inter_cmd = &bdev->internal_cmd;
	int ret;

	BTMTK_INFO("%s", __func__);
	wmt_cmd.hdr.dir = WMT_PKT_DIR_HOST_TO_CHIP;
	wmt_cmd.hdr.opcode = WMT_OPCODE_RF_CAL;
	wmt_cmd.params.u.rf_cal_cmd.subop = 0x03;
	param_len = sizeof(wmt_cmd.params.u.rf_cal_cmd);


	wmt_cmd.hdr.param_len[0] = (uint8_t)(param_len & 0xFF);
	wmt_cmd.hdr.param_len[1] = (uint8_t)((param_len >> 8) & 0xFF);

	pkt_len = HCI_CMD_HDR_LEN + WMT_CMD_HDR_LEN + param_len;
	memcpy(buffer, WMT_OVER_HCI_CMD_HDR, HCI_CMD_HDR_LEN);
	buffer[3] = WMT_CMD_HDR_LEN + param_len;
	memcpy(buffer + HCI_CMD_HDR_LEN, &wmt_cmd, WMT_CMD_HDR_LEN + param_len);

	/* Save the necessary information to internal_cmd struct */
	bdev->event_intercept = TRUE;
	p_inter_cmd->waiting_event = 0xE4;
	p_inter_cmd->pending_cmd_opcode = 0xFC6F;
	p_inter_cmd->wmt_opcode = WMT_OPCODE_RF_CAL;
	p_inter_cmd->result = 0;

	ret = btmtk_main_send_cmd(hdev, buffer, pkt_len, BTMTKUART_TX_WAIT_VND_EVT);

	if (ret <= 0) {
		BTMTK_ERR("Unable to get calibration event in time\n");
		return -1;
	}

	BTMTK_INFO("WMT RF cal backup event return with %s\n",
		       (p_inter_cmd->result == 1) ? "success" : "failure");
	if (p_inter_cmd->result) {
		*p_start_addr = (p_inter_cmd->wmt_event_params.u.rf_cal_evt.start_addr[3] << 24) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.start_addr[2] << 16) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.start_addr[1] << 8) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.start_addr[0]);
		*p_ready_addr = (p_inter_cmd->wmt_event_params.u.rf_cal_evt.ready_addr[3] << 24) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.ready_addr[2] << 16) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.ready_addr[1] << 8) |
				(p_inter_cmd->wmt_event_params.u.rf_cal_evt.ready_addr[0]);
		*p_data_len = (p_inter_cmd->wmt_event_params.u.rf_cal_evt.data_len[1] << 8) |
			      (p_inter_cmd->wmt_event_params.u.rf_cal_evt.data_len[0]);
		ret = 0;
	} else
		ret = -EIO;

	/* Whether succeed or not, no one is waiting, reset the flag here */
	bdev->event_intercept = FALSE;
	return ret;
}

int32_t btmtk_send_calibration_cmd(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
	uint32_t cal_data_start_addr = 0;
	uint32_t cal_data_ready_addr = 0;
	uint16_t cal_data_len = 0;
	uint8_t *p_cal_data = NULL;

	if (bdev->cal_data.p_cache_buf)
		return 0;

	/*
	 * In case that we did not have a successful pre-calibration and no
	 * cached data to restore, the first BT func on will trigger calibration,
	 * but it is most likely to be failed due to Wi-Fi is off.
	 *
	 * If BT func on return success and we get here, anyway try to backup
	 * the calibration data, any failure during backup should be ignored.
	 */

	/* Get calibration data reference for backup */
	if (btmtk_get_cal_data_ref(hdev, &cal_data_start_addr,
				    &cal_data_ready_addr,
				    &cal_data_len))
		BTMTK_ERR("Get cal data ref failed!\n");
	else {
		BTMTK_INFO(
			"Get cal data ref: saddr(0x%08x) raddr(0x%08x) len(%d)\n",
			cal_data_start_addr, cal_data_ready_addr, cal_data_len);

		/* Allocate a cache buffer to backup the calibration data in driver */
		if (cal_data_len) {
			p_cal_data = kmalloc(cal_data_len, GFP_KERNEL);
			if (p_cal_data) {
				bgfsys_cal_data_backup(cal_data_start_addr, p_cal_data, cal_data_len);
				bdev->cal_data.p_cache_buf = p_cal_data;
				bdev->cal_data.cache_len = cal_data_len;
				bdev->cal_data.start_addr = cal_data_start_addr;
				bdev->cal_data.ready_addr = cal_data_ready_addr;
			} else
				BTMTK_ERR("Failed to allocate cache buffer for backup!\n");
		} else
			BTMTK_ERR(
				"Abnormal cal data length! something error with F/W!\n");
	}
	return 0;
}


int32_t btmtk_set_power_on(struct hci_dev *hdev)
{
	int ret;
#if 0
	phys_addr_t emi_ap_phy_base;
#endif
#if SUPPORT_BT_THREAD
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);
#endif

	if (bdev->bt_state != FUNC_OFF) {
		BTMTK_ERR("BT is not at off state, uanble to on [%d]!\n",
							bdev->bt_state);
		return -EIO;
	}

	bdev->bt_state = TURNING_ON;
	/*
	 *  1. ConnInfra hardware power on (Must be the first step)
	 *
	 *  We will block in this step if conninfra driver find that the pre-calibration
	 *  has not been performed, conninfra driver should guarantee to trigger
	 *  pre-calibration procedure first:
	 *    - call BT/Wi-Fi registered pwr_on_cb and do_cal_cb
	 *  then return from this API after 2.4G calibration done.
	 */
	if (conninfra_pwr_on(CONNDRV_TYPE_BT)) {
		BTMTK_ERR("ConnInfra power on failed!\n");
		ret = -EIO;
		goto conninfra_error;
	}

	/* 2. BGFSYS hardware and MCU bring up, BT RAM code ready */
	ret = bt_hw_and_mcu_on();
	if (ret) {
		BTMTK_ERR("BT hardware and MCU on failed!\n");
		goto mcu_error;
	}

	bdev->rx_ind = FALSE;
	bdev->psm.state = PSM_ST_SLEEP;
	bdev->psm.sleep_flag = FALSE;
	bdev->psm.wakeup_flag = FALSE;
	bdev->psm.result = 0;
	bdev->psm.quick_ps = TRUE;

#if SUPPORT_BT_THREAD
	/* 3. Create TX thread with PS state machine */
	init_waitqueue_head(&bdev->tx_waitq);
	bdev->tx_thread = kthread_run(btmtk_tx_thread, bdev, "bt_tx_ps");
	if (IS_ERR(bdev->tx_thread)) {
		BTMTK_DBG("bt_tx_worker_thread failed to start!\n");
		ret = PTR_ERR(bdev->tx_thread);
		goto thread_create_error;
	}
#endif

	/*
	 * 4. Calibration data restore
	 * If we have a cache of calibration data, restore it to Firmware sysram,
	 * otherwise, BT func on will trigger calibration again.
	 */
	if (bdev->cal_data.p_cache_buf)
		bgfsys_cal_data_restore(bdev->cal_data.start_addr,
					bdev->cal_data.ready_addr,
					bdev->cal_data.p_cache_buf,
					bdev->cal_data.cache_len);

#if 0
	/* init core dump to get handle */
	conninfra_get_phy_addr((uint32_t*)&emi_ap_phy_base, NULL);
	emi_ap_phy_base &= 0xFFFFFFFF;

	bdev->coredump_handle =	connsys_coredump_init(CONN_DEBUG_TYPE_BT,
					emi_ap_phy_base + BT_EMI_BASE_OFFSET,
					BT_EMI_LENGTH, NULL);
#endif

	/* clear reset count if power on success */
	bdev->rst_level = RESET_LEVEL_NONE;
	bdev->rst_count = 0;

	return 0;

thread_create_error:
	bt_hw_and_mcu_off();

mcu_error:
	conninfra_pwr_off(CONNDRV_TYPE_BT);

conninfra_error:
	bdev->bt_state = FUNC_OFF;
	return ret;
}

int32_t btmtk_set_power_off(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev = hci_get_drvdata(hdev);

	BTMTK_INFO("%s", __func__);
	/* 1. Stop TX thread */
#if SUPPORT_BT_THREAD
	skb_queue_purge(&bdev->tx_queue);

	if (bdev->tx_thread) {
		wake_up_interruptible(&bdev->tx_waitq);
		kthread_stop(bdev->tx_thread);
	}
	bdev->tx_thread = NULL;
#endif
	/* 2. deinit coredump */
	//connsys_coredump_deinit(bdev->coredump_handle);

	/* 3. BGFSYS hardware and MCU shut down */
	bt_hw_and_mcu_off();
	BTMTK_INFO("BT hardware and MCU off!\n");

	/* 4. ConnInfra hardware power off */
	conninfra_pwr_off(CONNDRV_TYPE_BT);
	BTMTK_INFO("ConnInfra power off!\n");

	return 0;
}

int32_t btmtk_set_sleep(struct hci_dev *hdev)
{
	struct btmtk_dev *bdev =  hci_get_drvdata(hdev);

#if SUPPORT_BT_THREAD
	bdev->psm.sleep_flag = TRUE;
	wake_up_interruptible(&bdev->tx_waitq);
	if (!wait_for_completion_timeout(&bdev->psm.comp, msecs_to_jiffies(1000))) {
		BTMTK_ERR("[PSM]Timeout for BGFSYS to enter sleep!\n");
		bdev->psm.sleep_flag = FALSE;
		return -1;
	}

	BTMTK_DBG("[PSM]sleep return %s, sleep(%d), wakeup(%d)\n",
		      (bdev->psm.result == 0) ? "success" : "failure",
		      bdev->psm.sleep_flag, bdev->psm.wakeup_flag);

	return 0;
#else
	BTMTK_ERR("[PSM] Doesn't support non-thread mode !\n");
	return -1;
#endif


}

int32_t btmtk_set_wakeup(struct hci_dev *hdev)
{
	return 0;
}

int32_t btmtk_send_thermal_query_cmd(struct hci_dev *hdev)
{
	uint8_t cmd[] = { 0x01, 0x91, 0xFD, 0x00 };
	/* To-Do, for event check */
	/* u8 event[] = { 0x04, 0x0E, 0x08, 0x01, 0x91, 0xFD, 0x00, 0x00, 0x00, 0x00, 0x00 }; */
	btmtk_main_send_cmd(hdev, cmd, sizeof(cmd), BTMTKUART_TX_WAIT_VND_EVT);

	BTMTK_INFO("%s done", __func__);
	return 0;
}
