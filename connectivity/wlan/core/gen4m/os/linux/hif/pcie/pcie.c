/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/******************************************************************************
 *[File]             pcie.c
 *[Version]          v1.0
 *[Revision Date]    2010-03-01
 *[Author]
 *[Description]
 *    The program provides PCIE HIF driver
 *[Copyright]
 *    Copyright (C) 2010 MediaTek Incorporation. All Rights Reserved.
 ******************************************************************************/


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

#include "gl_os.h"
#include "hif_pdma.h"
#include "precomp.h"
#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>

#if CFG_SUPPORT_RX_PAGE_POOL
#include <net/page_pool.h>
#endif

#include "mt66xx_reg.h"
#include "wlan_pinctrl.h"

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#define MTK_PCI_VENDOR_ID	0x14C3
#define NIC6632_PCIe_DEVICE_ID	0x6632
#define NIC7668_PCIe_DEVICE_ID	0x7668
#define MT7663_PCI_PFGA2_VENDOR_ID	0x0E8D
#define NIC7663_PCIe_DEVICE_ID	0x7663
#define CONNAC_PCI_VENDOR_ID	0x0E8D
#define CONNAC_PCIe_DEVICE_ID	0x3280
#define NIC7915_PCIe_DEVICE_ID	0x7915
#define NICSOC3_0_PCIe_DEVICE_ID  0x0789
#define NIC7961_PCIe_DEVICE_ID	0x7961
#define NICSOC5_0_PCIe_DEVICE_ID  0x0789
#define NICSOC7_0_PCIe_DEVICE_ID  0x0789
#define NICBELLWETHER_PCIe_DEVICE_ID1 0x3107 /* used for FPGA */
#ifdef CFG_COMBO_SLT_GOLDEN
#define NICBELLWETHER_PCIe_DEVICE_ID2 0xF922 /* used for SLT golden */
#else
#define NICBELLWETHER_PCIe_DEVICE_ID2 0x7902 /* used for asic & FPGA */
#endif
#define NIC6639_PCIe_DEVICE_ID1 0x3107
#ifdef CFG_COMBO_SLT_GOLDEN
#define NIC6639_PCIe_DEVICE_ID2 0xE639 /*used for SLT golden */
#else
#define NIC6639_PCIe_DEVICE_ID2 0x6639
#endif
#define NIC6655_PCIe_DEVICE_ID1 0x3107
#define NIC6655_PCIe_DEVICE_ID2 0x6655
#define NIC7990_PCIe_DEVICE_ID 0x7990

static const struct pci_device_id mtk_pci_ids[] = {
#ifdef MT6632
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6632_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6632},
#endif /* MT6632 */
#ifdef MT7668
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7668_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7668},
#endif /* MT7668 */
#ifdef MT7663
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7663_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7663},
	/* For FPGA2 temparay */
	{	PCI_DEVICE(MT7663_PCI_PFGA2_VENDOR_ID, NIC7663_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7663},
#endif /* MT7663 */
#ifdef CONNAC
	{	PCI_DEVICE(CONNAC_PCI_VENDOR_ID, CONNAC_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_connac},
#endif /* CONNAC */
#ifdef SOC2_1X1
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc2_1x1},
#endif /* SOC2_1X1 */
#ifdef SOC2_2X2
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc2_2x2},
#endif /* SOC2_2X2 */
#ifdef MT7915
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7915_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7915},
#endif /* MT7915 */
#ifdef SOC3_0
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICSOC3_0_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc3_0 },
#endif /* SOC3_0 */
#ifdef MT7961
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7961_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7961},
#endif /* MT7961 */
#ifdef SOC5_0
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICSOC5_0_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc5_0},
#endif /* SOC5_0 */
#ifdef SOC7_0
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICSOC7_0_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc7_0},
#endif /* SOC7_0 */
#ifdef BELLWETHER
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICBELLWETHER_PCIe_DEVICE_ID1),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_bellwether},
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICBELLWETHER_PCIe_DEVICE_ID2),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_bellwether},
#endif /* BELLWETHER */
#ifdef MT6639
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6639_PCIe_DEVICE_ID1),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639},
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6639_PCIe_DEVICE_ID2),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639},
#endif /* MT6639 */
#ifdef MT6655
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6655_PCIe_DEVICE_ID1),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6655},
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6655_PCIe_DEVICE_ID2),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6655},
#endif /* MT6655 */
#ifdef MT7990
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7990_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7990},
#endif /* MT7990 */
	{ /* end: all zeroes */ },
};

static const struct platform_device_id mtk_axi_ids[] = {
	{	.name = "CONNAC",
#if defined(BELLWETHER)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_bellwether
#elif defined(MT6639)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639
#elif defined(MT6655)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6655
#endif
	},

	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(pci, mtk_pci_ids);

#ifdef CONFIG_OF
const struct of_device_id mtk_axi_of_ids[] = {
	{.compatible = "mediatek,wifi",},
	{}
};
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
static probe_card pfWlanProbe;
static remove_card pfWlanRemove;
static u_int8_t g_AERRstTriggered;
static u_int8_t g_AERL05Rst;

static struct platform_driver mtk_axi_driver = {
	.driver = {
		.name = "wlan",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_axi_of_ids,
#endif
	},
	.id_table = mtk_axi_ids,
	.probe = NULL,
	.remove = NULL,
};

static pci_ers_result_t mtk_pci_error_detected(struct pci_dev *pdev,
	pci_channel_state_t state);
static pci_ers_result_t mtk_pci_error_slot_reset(struct pci_dev *pdev);
static void mtk_pci_error_resume(struct pci_dev *pdev);

static const struct pci_error_handlers mtk_pci_err_handler = {
	.error_detected = mtk_pci_error_detected,
	.slot_reset     = mtk_pci_error_slot_reset,
	.resume         = mtk_pci_error_resume,
};

static struct pci_driver mtk_pci_driver = {
	.name = KBUILD_MODNAME,
	.id_table = mtk_pci_ids,
	.probe = NULL,
	.remove = NULL,
	.err_handler = &mtk_pci_err_handler,
};

static struct GLUE_INFO *g_prGlueInfo;
static void *CSRBaseAddress;
static u64 g_u8CsrOffset;
static u32 g_u4CsrSize;
static u_int8_t g_fgDriverProbed = FALSE;
struct pci_dev *g_prDev;

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static void halPciePreSuspendCmd(struct ADAPTER *prAdapter);
static void halPcieResumeCmd(struct ADAPTER *prAdapter);

static irqreturn_t mtk_axi_isr(int irq, void *dev_instance);
static irqreturn_t mtk_axi_isr_thread(int irq, void *dev_instance);

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

struct mt66xx_hif_driver_data *get_platform_driver_data(void)
{
	return (struct mt66xx_hif_driver_data *) mtk_pci_ids[0].driver_data;
}

struct GLUE_INFO *get_glue_info_isr(void *dev_instance, int irq, int msi_idx)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter;
#if PCIE_ISR_DEBUG_LOG
	static DEFINE_RATELIMIT_STATE(_rs, 2 * HZ, 1);
#endif

	prGlueInfo = (struct GLUE_INFO *)dev_instance;
	if (!prGlueInfo) {
		DBGLOG(HAL, INFO, "No glue info in %s(%d, %d)\n",
		       __func__, irq, msi_idx);
		enable_irq(irq);
		return NULL;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (msi_idx >= 0 && msi_idx < PCIE_MSI_NUM)
		GLUE_INC_REF_CNT(prAdapter->rHifStats.u4MsiIsrCount[msi_idx]);
	else
		GLUE_INC_REF_CNT(prAdapter->rHifStats.u4HwIsrCount);

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG(HAL, INFO, "GLUE_FLAG_HALT skip INT(%d, %d)\n",
		       irq, msi_idx);
		enable_irq(irq);
		return NULL;
	}

#if HIF_INT_TIME_DEBUG
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	if (!prBusInfo->u4EnHifIntTs) {
		ktime_get_ts64(&prBusInfo->rHifIntTs);
		prBusInfo->u4EnHifIntTs = 1;
	}
	prBusInfo->u4HifIntTsCnt++;
#endif

#if PCIE_ISR_DEBUG_LOG
	if (__ratelimit(&_rs))
		pr_info("[wlan] In HIF ISR(%d).\n", irq);
#endif

	return prGlueInfo;
}

void mtk_pci_disable_device(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return;

	pci_disable_device(prGlueInfo->rHifInfo.pdev);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a PCIE interrupt callback function
 *
 * \param[in] func  pointer to PCIE handle
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
irqreturn_t mtk_pci_isr(int irq, void *dev_instance)
{
	disable_irq_nosync(irq);
	return IRQ_WAKE_THREAD;
}

irqreturn_t mtk_pci_isr_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo;
	struct pcie_msi_layout *prMsiLayout;
	int i;

	prGlueInfo = get_glue_info_isr(dev_instance, irq, 0);
	if (!prGlueInfo)
		return IRQ_NONE;

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo || !prMsiInfo->fgMsiEnabled) {
		KAL_SET_BIT(0, prHifInfo->ulHifIntEnBits);
		goto exit;
	}

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->irq_num == irq) {
			KAL_SET_BIT(i, prMsiInfo->ulEnBits);
			break;
		}
	}
exit:
	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

void mtk_pci_enable_irq(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;
	struct pcie_msi_layout *prMsiLayout;
	int i;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo->fgMsiEnabled) {
		enable_irq(prHifInfo->u4IrqId);
		return;
	}

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->type != AP_INT)
			continue;

		if (test_and_clear_bit(i, &prMsiInfo->ulEnBits))
			enable_irq(prMsiLayout->irq_num);
	}
}

void mtk_pci_disable_irq(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;
	struct pcie_msi_layout *prMsiLayout;
	int i;

	prAdapter = prGlueInfo->prAdapter;
	prChipInfo = prAdapter->chip_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!prMsiInfo->fgMsiEnabled) {
		disable_irq_nosync(prHifInfo->u4IrqId);
		return;
	}

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->type != AP_INT)
			continue;

		if (!test_bit(i, &prMsiInfo->ulEnBits)) {
			disable_irq_nosync(prMsiLayout->irq_num);
			KAL_SET_BIT(i, prMsiInfo->ulEnBits);
		}
	}
}

irqreturn_t pcie_sw_int_top_handler(int irq, void *dev_instance)
{
	return IRQ_WAKE_THREAD;
}

irqreturn_t pcie_sw_int_thread_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;

	prGlueInfo = (struct GLUE_INFO *)dev_instance;
	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(HAL, WARN, "NULL prAdapter.\n");
		goto exit;
	}

	GLUE_INC_REF_CNT(prAdapter->rHifStats.u4SwIsrCount);

#if (CFG_SUPPORT_CONNAC3X == 1)
	asicConnac3xSwIntHandler(prAdapter);
#endif

exit:
	return IRQ_HANDLED;
}

irqreturn_t pcie_fw_log_top_handler(int irq, void *dev_instance)
{
	return IRQ_WAKE_THREAD;
}

irqreturn_t pcie_fw_log_thread_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;

	prGlueInfo = (struct GLUE_INFO *)dev_instance;

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG(HAL, WARN, "GLUE_FLAG_HALT skip INT\n");
		return IRQ_NONE;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(HAL, WARN, "NULL prAdapter.\n");
		return IRQ_NONE;
	}

	GLUE_INC_REF_CNT(prAdapter->rHifStats.u4SwIsrCount);

#if (CFG_SUPPORT_CONNAC3X == 1)
	fw_log_handler();
#endif

	return IRQ_HANDLED;
}

irqreturn_t pcie_drv_own_top_handler(int irq, void *dev_instance)
{
	return IRQ_WAKE_THREAD;
}

irqreturn_t pcie_drv_own_thread_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	DBGLOG(HAL, TRACE, "driver own INT\n");

	prGlueInfo = (struct GLUE_INFO *)dev_instance;

	if (prGlueInfo) {
		ktime_get_ts64(&prGlueInfo->u4DrvOwnIntTick);
		set_bit(GLUE_FLAG_DRV_OWN_INT_BIT, &prGlueInfo->ulFlag);
	} else {
		DBGLOG(HAL, WARN, "NULL prGlueInfo.\n");
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

#if (CFG_MTK_MDDP_SUPPORT || CFG_MTK_CCCI_SUPPORT)
irqreturn_t mtk_md_dummy_pci_interrupt(int irq, void *dev_instance)
{
	return IRQ_HANDLED;
}
#endif

static pci_ers_result_t mtk_pci_error_detected(struct pci_dev *pdev,
	pci_channel_state_t state)
{
	pci_ers_result_t res = PCI_ERS_RESULT_NONE;
	uint32_t dump = 0;
	u_int8_t fgNeedReset = FALSE;

	DBGLOG(HAL, INFO,
		"mtk_pci_error_detected state: %d, resetting: %d %d\n",
		state, g_AERRstTriggered, kalIsResetting());

	if (!pci_is_enabled(pdev)) {
		DBGLOG(HAL, INFO, "pcie is disable\n");
		goto exit;
	}

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	dump = mtk_pcie_dump_link_info(0);
#endif

	if (g_AERRstTriggered || kalIsResetting())
		goto exit;

	if (state == pci_channel_io_normal) {
		uint16_t vnd_id = 0;

		/* bit[6]: Completion timeout status */
		if (dump & BIT(6)) {
			fgNeedReset = TRUE;
			pci_read_config_word(pdev, PCI_VENDOR_ID, &vnd_id);
			if (vnd_id == 0) {
				DBGLOG(HAL, WARN, "PCIE link down\n");
				fgIsBusAccessFailed = TRUE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
				fgTriggerDebugSop = TRUE;
#endif
			} else {
#if CFG_MTK_WIFI_AER_L05_RESET
				g_AERL05Rst = TRUE;
#endif
			}
		/* bit[7]: RxErr */
		} else if (dump & BIT(7)) {
			/* block PCIe access */
#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
			mtk_pcie_disable_data_trans(0);
#endif
			fgNeedReset = TRUE;
			fgIsBusAccessFailed = TRUE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
			fgTriggerDebugSop = TRUE;
#endif
		}
	} else {
		pci_disable_device(pdev);
		fgNeedReset = TRUE;
		fgIsBusAccessFailed = TRUE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		fgTriggerDebugSop = TRUE;
#endif
	}

exit:
	if (fgNeedReset) {
		if (g_AERRstTriggered || kalIsResetting())
			res = PCI_ERS_RESULT_CAN_RECOVER;
		else
			res = PCI_ERS_RESULT_NEED_RESET;

		g_AERRstTriggered = TRUE;
	}

	return res;
}

static pci_ers_result_t mtk_pci_error_slot_reset(struct pci_dev *pdev)
{
#define AER_RST_STR		"Whole chip reset by AER"

	struct GLUE_INFO *prGlueInfo = g_prGlueInfo;

	DBGLOG(HAL, INFO, "mtk_pci_error_slot_reset, L05_rst: %d\n",
		g_AERL05Rst);

	if (g_AERL05Rst) {
		GL_USER_DEFINE_RESET_TRIGGER(prGlueInfo->prAdapter,
			RST_AER, RST_FLAG_WF_RESET);
	} else {
		glSetRstReasonString(AER_RST_STR);
		glResetWholeChipResetTrigger(AER_RST_STR);
	}

	return PCI_ERS_RESULT_DISCONNECT;
}

static void mtk_pci_error_resume(struct pci_dev *pdev)
{
	DBGLOG(HAL, INFO, "mtk_pci_error_resume\n");
}

static int32_t setupPlatDevIrq(struct platform_device *pdev, uint32_t *pu4IrqId)
{
	uint32_t u4IrqId = 0;
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	int en_wake_ret = 0;
#endif

	if (!pdev)
		return -1;

#ifdef CONFIG_OF
	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi");
	if (node)
		u4IrqId = irq_of_parse_and_map(node, 0);
	else
		DBGLOG(INIT, ERROR,
		       "WIFI-OF: get wifi device node fail\n");

	if (!u4IrqId) {
		DBGLOG(INIT, INFO, "no irq\n");
		goto exit;
	}

	DBGLOG(INIT, INFO, "request_irq num(%d)\n", u4IrqId);

	ret = devm_request_threaded_irq(
		&pdev->dev,
		u4IrqId,
		mtk_axi_isr,
		mtk_axi_isr_thread,
		IRQF_SHARED,
		mtk_axi_driver.driver.name,
		platform_get_drvdata(pdev));
	if (ret != 0) {
		DBGLOG(INIT, INFO, "request_irq(%u) ERROR(%d)\n",
		       u4IrqId, ret);
		goto exit;
	}

	en_wake_ret = enable_irq_wake(u4IrqId);
	if (en_wake_ret)
		DBGLOG(INIT, INFO, "enable_irq_wake(%u) ERROR(%d)\n",
		       u4IrqId, en_wake_ret);
#endif

exit:
	*pu4IrqId = u4IrqId;
	return ret;
}

void freePlatDevIrq(struct platform_device *pdev, uint32_t u4IrqId)
{
	if (!pdev || !u4IrqId)
		return;

	synchronize_irq(u4IrqId);
	irq_set_affinity_hint(u4IrqId, NULL);
	devm_free_irq(&pdev->dev, u4IrqId, platform_get_drvdata(pdev));
}

static int axiDmaSetup(struct platform_device *pdev,
		struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	u64 dma_mask;
	int ret = 0;

	prChipInfo = prDriverData->chip_info;

	ret = halInitResvMem(pdev);
	if (ret)
		goto exit;
	ret = of_reserved_mem_device_init(&pdev->dev);
	if (ret) {
		DBGLOG(INIT, ERROR, "of_reserved_mem_device_init failed(%d).\n",
				ret);
		goto exit;
	}

	dma_mask = DMA_BIT_MASK(prChipInfo->bus_info->u4DmaMask);
	ret = dma_set_mask_and_coherent(&pdev->dev, dma_mask);
	if (ret) {
		DBGLOG(INIT, ERROR, "dma_set_mask_and_coherent failed(%d)\n",
			ret);
		goto exit;
	}

exit:
	return ret;
}

static bool axiCsrIoremap(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	struct resource res;

	DBGLOG(INIT, INFO, "%s: start\n", __func__);

	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi");
	if (!node) {
		DBGLOG(INIT, ERROR, "WIFI-OF: get wifi device node fail\n");
		return false;
	}

	if (of_address_to_resource(node, 0, &res)) {
		DBGLOG(INIT, ERROR, "WIFI-OF: of_address_to_resource fail\n");
		of_node_put(node);
		return false;
	}
	of_node_put(node);

	g_u8CsrOffset = (u64)res.start;
	g_u4CsrSize = resource_size(&res);
#else
	g_u8CsrOffset = axi_resource_start(pdev, 0);
	g_u4CsrSize = axi_resource_len(pdev, 0);
#endif

	prDriverData = get_platform_driver_data();
	if (!prDriverData) {
		DBGLOG(INIT, ERROR, "driver data is NULL\n");
		return false;
	}
	prChipInfo = prDriverData->chip_info;

	if (CSRBaseAddress) {
		DBGLOG(INIT, ERROR, "CSRBaseAddress not iounmap!\n");
		return false;
	}

	request_mem_region(g_u8CsrOffset, g_u4CsrSize, axi_name(pdev));

	/* map physical address to virtual address for accessing register */
#ifdef CONFIG_OF
	CSRBaseAddress = of_iomap(node, 0);
#else
	CSRBaseAddress = ioremap(g_u8CsrOffset, g_u4CsrSize);
#endif

	if (!CSRBaseAddress) {
		DBGLOG(INIT, INFO,
			"ioremap failed for device %s, region 0x%X @ 0x%lX\n",
			axi_name(pdev), g_u4CsrSize, g_u8CsrOffset);
		release_mem_region(g_u8CsrOffset, g_u4CsrSize);
		return false;
	}

	prChipInfo->HostCSRBaseAddress = CSRBaseAddress;
	prChipInfo->u4HostCsrOffset = (uint32_t)g_u8CsrOffset;
	prChipInfo->u4HostCsrSize = g_u4CsrSize;

	DBGLOG(INIT, INFO, "CSRBaseAddress:0x%lX ioremap region 0x%X @ 0x%lX\n",
	       CSRBaseAddress, g_u4CsrSize, g_u8CsrOffset);

	return true;
}

static void axiCsrIounmap(struct platform_device *pdev)
{
	if (!CSRBaseAddress)
		return;

	/* Unmap CSR base address */
	iounmap(CSRBaseAddress);
	release_mem_region(g_u8CsrOffset, g_u4CsrSize);

	CSRBaseAddress = NULL;
	g_u8CsrOffset = 0;
	g_u4CsrSize = 0;
}

static void axiSetupFwFlavor(struct platform_device *pdev,
	struct mt66xx_hif_driver_data *driver_data)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;

	if (of_property_read_string(node,
				    FW_BIN_FLAVOR_KEY,
				    &driver_data->fw_flavor))
		return;

	DBGLOG(HAL, INFO, "fw_flavor: %s\n", driver_data->fw_flavor);
}

static int mtk_axi_probe(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	int ret = 0;

	g_prPlatDev = pdev;
	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_axi_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;

	platform_set_drvdata(pdev, (void *)prDriverData);

	axiSetupFwFlavor(pdev, prDriverData);

	if (!axiCsrIoremap(pdev))
		goto exit;

	ret = axiDmaSetup(pdev, prDriverData);
	if (ret)
		goto exit;

#if (CFG_MTK_ANDROID_WMT == 1)
	emi_mem_init(prChipInfo, pdev);
#endif

	ret = halAllocHifMem(pdev, prDriverData);
	if (ret)
		goto exit;

#if (CFG_SUPPORT_RX_PAGE_POOL == 1) && (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 0)
	kalCreateHifSkbList(prChipInfo);
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	ret = wlan_pinctrl_init(prChipInfo);
	if (ret)
		goto exit;
#endif

#if CFG_SUPPORT_THERMAL_QUERY
	thermal_cbs_register(pdev);
#endif

exit:
	DBGLOG(INIT, INFO, "mtk_axi_probe() done, ret: %d\n", ret);
	return ret;
}

static int mtk_axi_remove(struct platform_device *pdev)
{
#if (CFG_MTK_ANDROID_WMT == 1)
	struct mt66xx_hif_driver_data *prDriverData =
		platform_get_drvdata(pdev);
	struct mt66xx_chip_info *prChipInfo = prDriverData->chip_info;
#endif

#if CFG_SUPPORT_THERMAL_QUERY
	thermal_cbs_unregister(pdev);
#endif
	halFreeHifMem(pdev);
#if (CFG_MTK_ANDROID_WMT == 1)
	emi_mem_uninit(prChipInfo, pdev);
#endif
#if (CFG_SUPPORT_RX_PAGE_POOL == 1) && (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 0)
	kalReleaseHifSkbList();
#endif
	platform_set_drvdata(pdev, NULL);
	return 0;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a PCIE probe function
 *
 * \param[in] func   pointer to PCIE handle
 * \param[in] id     pointer to PCIE device id table
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
static int mtk_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;
	uint32_t u4MaxMsiNum;
	int ret = 0;
#if CFG_SUPPORT_PCIE_ASPM
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
#endif

	ASSERT(pdev);
	ASSERT(id);

	prDriverData = (struct mt66xx_hif_driver_data *)id->driver_data;
	prChipInfo = prDriverData->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	ret = pcim_enable_device(pdev);
	if (ret) {
		DBGLOG(INIT, INFO,
			"pci_enable_device failed, ret=%d\n", ret);
		goto out;
	}

	ret = pcim_iomap_regions(pdev, BIT(0), pci_name(pdev));
	if (ret) {
		DBGLOG(INIT, INFO,
			"pcim_iomap_regions failed, ret=%d\n", ret);
		goto out;
	}
	fgIsBusAccessFailed = FALSE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	fgTriggerDebugSop = FALSE;
#endif
	g_AERRstTriggered = FALSE;
	g_AERL05Rst = FALSE;
	pci_set_master(pdev);

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_MSI_SUPPORT)
	u4MaxMsiNum = prMsiInfo->u4MaxMsiNum ?
		prMsiInfo->u4MaxMsiNum : 1;
#else
	u4MaxMsiNum = 1;
#endif
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	ret = pci_alloc_irq_vectors(pdev, 1,
				    u4MaxMsiNum,
				    PCI_IRQ_MSI);
#endif
	if (ret < 0) {
		DBGLOG(INIT, INFO,
			"pci_alloc_irq_vectors(1, %d) failed, ret=%d\n",
			u4MaxMsiNum,
			ret);
		goto err_free_iomap;
	}

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_MSI_SUPPORT)
	if (u4MaxMsiNum > 1 && ret == prMsiInfo->u4MaxMsiNum) {
		prMsiInfo->fgMsiEnabled = TRUE;
		prMsiInfo->u4MsiNum = ret;
	} else {
		prMsiInfo->fgMsiEnabled = FALSE;
		prMsiInfo->u4MsiNum = 1;
	}
#else
	prMsiInfo->fgMsiEnabled = FALSE;
	prMsiInfo->u4MsiNum = 1;
#endif
	prMsiInfo->ulEnBits = 0;
	DBGLOG(INIT, INFO, "ret=%d, fgMsiEnabled=%d, u4MsiNum=%d\n",
		ret, prMsiInfo->fgMsiEnabled, prMsiInfo->u4MsiNum);

	ret = pci_set_dma_mask(pdev,
		DMA_BIT_MASK(prChipInfo->bus_info->u4DmaMask));
	if (ret != 0) {
		DBGLOG(INIT, INFO,
			"pci_set_dma_mask failed, ret=%d\n", ret);
		goto err_free_irq_vectors;
	}

	g_prDev = pdev;
	prChipInfo->pdev = (void *)pdev;
	prChipInfo->CSRBaseAddress = pcim_iomap_table(pdev) ?
		pcim_iomap_table(pdev)[0] : NULL;

	DBGLOG(INIT, INFO, "ioremap for device %s, region 0x%lX @ 0x%lX\n",
		pci_name(pdev), (unsigned long) pci_resource_len(pdev, 0),
		(unsigned long) pci_resource_start(pdev, 0));

	pci_set_drvdata(pdev, (void *)id->driver_data);

#if (CFG_MTK_ANDROID_WMT == 0)
	emi_mem_init(prChipInfo, pdev);
#endif

	if (pfWlanProbe((void *) pdev,
		(void *) id->driver_data) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "pfWlanProbe fail!\n");
		ret = -1;
		goto err_free_irq_vectors;
	}

#if CFG_CONTROL_ASPM_BY_FW
#if CFG_SUPPORT_PCIE_ASPM
	glBusConfigASPM(pdev,
			DISABLE_ASPM_L1);
	glBusConfigASPML1SS(pdev,
		PCI_L1PM_CTR1_ASPM_L12_EN |
		PCI_L1PM_CTR1_ASPM_L11_EN);
	glBusConfigASPM(pdev,
			ENABLE_ASPM_L1);

	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL!\n");
	} else {
			prHifInfo = &prGlueInfo->rHifInfo;
			prAdapter = prGlueInfo->prAdapter;
			if (prAdapter->rWifiVar.fgPcieEnableL1ss == 0) {
				glBusConfigASPM(prHifInfo->pdev,
					DISABLE_ASPM_L1);
				DBGLOG(INIT, INFO, "PCIE keep L0\n");
			} else
				DBGLOG(INIT, INFO, "PCIE allow enter L1.2\n");
	}
#endif
#endif

	g_fgDriverProbed = TRUE;
	goto out;

err_free_irq_vectors:
#if (CFG_MTK_ANDROID_WMT == 0)
	emi_mem_uninit(prChipInfo, pdev);
#endif
	pci_set_drvdata(pdev, NULL);
	prChipInfo->CSRBaseAddress = NULL;
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	pci_free_irq_vectors(pdev);
#endif

err_free_iomap:
	pcim_iounmap_regions(pdev, BIT(0));

out:
	DBGLOG(INIT, INFO, "mtk_pci_probe() done(%d)\n", ret);

	return ret;
}

static void mtk_pci_remove(struct pci_dev *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData = pci_get_drvdata(pdev);
	struct mt66xx_chip_info *prChipInfo = prDriverData->chip_info;

	ASSERT(pdev);

	if (g_fgDriverProbed) {
		pfWlanRemove();
		g_fgDriverProbed = FALSE;
		DBGLOG(INIT, INFO, "pfWlanRemove done\n");
	}
#if (CFG_MTK_ANDROID_WMT == 0)
	emi_mem_uninit(prChipInfo, pdev);
#endif
	pci_set_drvdata(pdev, NULL);
	prChipInfo->CSRBaseAddress = NULL;
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	pci_free_irq_vectors(pdev);
#endif
	pcim_iounmap_regions(pdev, BIT(0));
}

static int mtk_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct GLUE_INFO *prGlueInfo = NULL;

#if (CFG_DEVICE_SUSPEND_BY_MOBILE == 1)
	struct device *dev = &pdev->dev;

	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return -1;
	}

	prGlueInfo->fgIsInSuspend = TRUE;
	dev->power.driver_flags = DPM_FLAG_SMART_SUSPEND;
	dev->power.runtime_status = RPM_SUSPENDED;
	pdev->skip_bus_pm = true;
	return 0;
#else
	struct BUS_INFO *prBusInfo;
	uint32_t count = 0;
	int wait = 0;
	struct ADAPTER *prAdapter = NULL;
	uint8_t drv_own_fail = FALSE;

	DBGLOG(HAL, STATE, "mtk_pci_suspend()\n");

	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return -1;
	}

	prAdapter = prGlueInfo->prAdapter;
	prGlueInfo->fgIsInSuspendMode = TRUE;

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);

	/* Stop upper layers calling the device hard_start_xmit routine. */
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);

#if CFG_ENABLE_WAKE_LOCK
	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_SUSPEND_ENTERING;
#endif

	/* wait wiphy device do cfg80211 suspend done, then start hif suspend */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanWaitCfg80211SuspendDone(prGlueInfo);

	wlanSuspendPmHandle(prGlueInfo);

#if !CFG_ENABLE_WAKE_LOCK
	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_PRE_SUSPEND_WAITING;
#endif

	halPciePreSuspendCmd(prAdapter);

	while (prGlueInfo->rHifInfo.eSuspendtate !=
		PCIE_STATE_PRE_SUSPEND_DONE) {
		if (count > 500) {
			DBGLOG(HAL, ERROR, "pcie pre_suspend timeout\n");
			return -EAGAIN;
		}
		kalMsleep(2);
		count++;
	}
	DBGLOG(HAL, ERROR, "pcie pre_suspend done\n");

	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_SUSPEND;

	/* Polling until HIF side PDMAs are all idle */
	prBusInfo = prAdapter->chip_info->bus_info;
	if (prBusInfo->pdmaPollingIdle) {
		if (prBusInfo->pdmaPollingIdle(prGlueInfo) != TRUE)
			return -EAGAIN;
	} else
		DBGLOG(HAL, ERROR, "PDMA polling idle API didn't register\n");

	/* Disable HIF side PDMA TX/RX */
	if (prBusInfo->pdmaStop)
		prBusInfo->pdmaStop(prGlueInfo, TRUE);
	else
		DBGLOG(HAL, ERROR, "PDMA config API didn't register\n");

	halDisableInterrupt(prAdapter);

	/* FW own */
	/* Set FW own directly without waiting sleep notify */
	prAdapter->fgWiFiInSleepyState = TRUE;
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	/* Wait for
	*  1. The other unfinished ownership handshakes
	*  2. FW own back
	*/
	while (wait < 500) {
		if ((prAdapter->u4PwrCtrlBlockCnt == 0) &&
		    (prAdapter->fgIsFwOwn == TRUE) &&
		    (drv_own_fail == FALSE)) {
			DBGLOG(HAL, STATE, "*********************\n");
			DBGLOG(HAL, STATE, "* Enter PCIE Suspend *\n");
			DBGLOG(HAL, STATE, "*********************\n");
			DBGLOG(HAL, INFO, "wait = %d\n\n", wait);
			break;
		}

		ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
		/* Prevent that suspend without FW Own:
		 * Set Drv own has failed,
		 * and then Set FW Own is skipped
		 */
		if (prAdapter->fgIsFwOwn == FALSE)
			drv_own_fail = FALSE;
		else
			drv_own_fail = TRUE;
		/* For single core CPU */
		/* let hif_thread can be completed */
		usleep_range(1000, 3000);
		RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

		wait++;
	}

	if (wait >= 500) {
		DBGLOG(HAL, ERROR, "Set FW Own Timeout !!\n");
		return -EAGAIN;
	}

	pci_save_state(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));

	DBGLOG(HAL, STATE, "mtk_pci_suspend() done!\n");

	/* pending cmd will be kept in queue and no one to handle it after HIF resume.
	 * In STR, it will result in cmd buf full and then cmd buf alloc fail .
	 */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanReleaseAllTxCmdQueue(prGlueInfo->prAdapter);

	return 0;
#endif
}


int mtk_pci_resume(struct pci_dev *pdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;

#if (CFG_DEVICE_SUSPEND_BY_MOBILE == 1)
	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return -1;
	}

	prGlueInfo->fgIsInSuspend = FALSE;
	return 0;
#else
	struct BUS_INFO *prBusInfo;

	DBGLOG(HAL, STATE, "mtk_pci_resume()\n");

	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return -1;
	}

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);

	/* Driver own */
	/* Include restore PDMA settings */
	ACQUIRE_POWER_CONTROL_FROM_PM(prGlueInfo->prAdapter);

	if (prBusInfo->initPcieInt)
		prBusInfo->initPcieInt(prGlueInfo);

	halEnableInterrupt(prGlueInfo->prAdapter);

	/* Enable HIF side PDMA TX/RX */
	if (prBusInfo->pdmaStop)
		prBusInfo->pdmaStop(prGlueInfo, FALSE);
	else
		DBGLOG(HAL, ERROR, "PDMA config API didn't register\n");

	halPcieResumeCmd(prGlueInfo->prAdapter);

	wlanResumePmHandle(prGlueInfo);

	/* FW own */
	RECLAIM_POWER_CONTROL_TO_PM(prGlueInfo->prAdapter, FALSE);

	prGlueInfo->fgIsInSuspendMode = FALSE;
	/* Allow upper layers to call the device hard_start_xmit routine. */
	netif_tx_wake_all_queues(prGlueInfo->prDevHandler);

	DBGLOG(HAL, STATE, "mtk_pci_resume() done!\n");

	return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will register pci bus to the os
 *
 * \param[in] pfProbe    Function pointer to detect card
 * \param[in] pfRemove   Function pointer to remove card
 *
 * \return The result of registering pci bus
 */
/*----------------------------------------------------------------------------*/
uint32_t glRegisterBus(probe_card pfProbe, remove_card pfRemove)
{
	int ret = 0;

	ASSERT(pfProbe);
	ASSERT(pfRemove);

	pfWlanProbe = pfProbe;
	pfWlanRemove = pfRemove;

	mtk_pci_driver.probe = mtk_pci_probe;
	mtk_pci_driver.remove = mtk_pci_remove;
	mtk_pci_driver.suspend = mtk_pci_suspend;
	mtk_pci_driver.resume = mtk_pci_resume;

	mtk_axi_driver.probe = mtk_axi_probe;
	mtk_axi_driver.remove = mtk_axi_remove;

	if (platform_driver_register(&mtk_axi_driver))
		DBGLOG(HAL, ERROR, "platform_driver_register fail\n");

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	mtk_pcie_remove_port(0);
#endif

	return ret;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will unregister pci bus to the os
 *
 * \param[in] pfRemove Function pointer to remove card
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void glUnregisterBus(remove_card pfRemove)
{
	if (g_fgDriverProbed) {
		pfRemove();
		g_fgDriverProbed = FALSE;
	}
	platform_driver_unregister(&mtk_axi_driver);
}

static void glPopulateMemOps(struct mt66xx_chip_info *prChipInfo,
			     struct HIF_MEM_OPS *prMemOps)
{
	prMemOps->allocTxDesc = halZeroCopyPathAllocDesc;
	prMemOps->allocRxDesc = halZeroCopyPathAllocDesc;
	prMemOps->allocExtBuf = halZeroCopyPathAllocExtBuf;
	prMemOps->allocTxCmdBuf = NULL;
	prMemOps->allocTxDataBuf = halZeroCopyPathAllocTxDataBuf;
	prMemOps->allocRuntimeMem = halZeroCopyPathAllocRuntimeMem;
	prMemOps->copyCmd = halZeroCopyPathCopyCmd;
	prMemOps->copyEvent = halZeroCopyPathCopyEvent;
	prMemOps->copyTxData = halZeroCopyPathCopyTxData;
	prMemOps->copyRxData = halZeroCopyPathCopyRxData;
	prMemOps->mapTxBuf = halZeroCopyPathMapTxBuf;
	prMemOps->mapRxBuf = halZeroCopyPathMapRxBuf;
	prMemOps->unmapTxBuf = halZeroCopyPathUnmapTxBuf;
	prMemOps->unmapRxBuf = halZeroCopyPathUnmapRxBuf;
	prMemOps->freeDesc = halZeroCopyPathFreeDesc;
	prMemOps->freeExtBuf = halZeroCopyPathFreeDesc;
	prMemOps->freeBuf = halZeroCopyPathFreeBuf;
	prMemOps->allocRxEvtBuf = halZeroCopyPathAllocRxBuf;
#if CFG_SUPPORT_RX_PAGE_POOL
	prMemOps->allocRxDataBuf = halZeroCopyPathAllocPagePoolRxBuf;
	prMemOps->freePacket = halZeroCopyPathFreePagePoolPacket;
#else
	prMemOps->allocRxDataBuf = halZeroCopyPathAllocRxBuf;
	prMemOps->freePacket = halZeroCopyPathFreePacket;
#endif /* CFG_SUPPORT_RX_PAGE_POOL */
#if CFG_MTK_WIFI_SW_EMI_RING
	prMemOps->getRsvEmi = halGetRsvEmi;
#endif
#if 0
	prMemOps->dumpTx = halZeroCopyPathDumpTx;
	prMemOps->dumpRx = halZeroCopyPathDumpRx;
#endif

	if (g_prPlatDev) {
		DBGLOG(HAL, TRACE, "Use pre-alloc mem ops instead.\n");
		prMemOps->allocTxDesc = halCopyPathAllocTxDesc;
		prMemOps->allocRxDesc = halCopyPathAllocRxDesc;
		prMemOps->allocExtBuf = halCopyPathAllocExtBuf;
		prMemOps->allocTxCmdBuf = halCopyPathAllocTxCmdBuf;
		prMemOps->allocTxDataBuf = halCopyPathAllocTxDataBuf;
		prMemOps->allocRxEvtBuf = halCopyPathAllocRxBuf;
		prMemOps->allocRuntimeMem = NULL;
		prMemOps->copyCmd = halCopyPathCopyCmd;
		prMemOps->copyEvent = halCopyPathCopyEvent;
		prMemOps->copyTxData = halCopyPathCopyTxData;
		prMemOps->mapTxBuf = NULL;
		prMemOps->unmapTxBuf = NULL;
		prMemOps->freeDesc = NULL;
		prMemOps->freeExtBuf = halCopyPathFreeExtBuf;
		prMemOps->freeBuf = NULL;
		prMemOps->dumpTx = halCopyPathDumpTx;

#if (CFG_SUPPORT_RX_ZERO_COPY == 1)
		prMemOps->dumpRx = halZeroCopyPathDumpRx;
#else
		glUpdateRxCopyMemOps(prMemOps)
#endif /* CFG_SUPPORT_RX_ZERO_COPY == 1 */
	}
}

void glUpdateRxCopyMemOps(struct HIF_MEM_OPS *prMemOps)
{
	prMemOps->copyRxData = halCopyPathCopyRxData;
	prMemOps->mapRxBuf = NULL;
	prMemOps->unmapRxBuf = NULL;
	prMemOps->freePacket = NULL;
	prMemOps->dumpRx = halCopyPathDumpRx;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function stores hif related info, which is initialized before.
 *
 * \param[in] prGlueInfo Pointer to glue info structure
 * \param[in] u4Cookie   Pointer to UINT_32 memory base variable for _HIF_HPI
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void glSetHifInfo(struct GLUE_INFO *prGlueInfo, unsigned long ulCookie)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHif = NULL;
	struct BUS_INFO *prBusInfo;
	struct HIF_MEM_OPS *prMemOps;

	prHif = &prGlueInfo->rHifInfo;
	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;
	prMemOps = &prHif->rMemOps;

	prHif->pdev = (struct pci_dev *)ulCookie;
	prHif->prDmaDev = prHif->pdev;

	g_prGlueInfo = prGlueInfo;

	prHif->CSRBaseAddress = prChipInfo->CSRBaseAddress;

	if (g_prPlatDev)
		SET_NETDEV_DEV(prGlueInfo->prDevHandler, &g_prPlatDev->dev);
	else
		SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHif->pdev->dev);

	prGlueInfo->u4InfType = MT_DEV_INF_PCIE;

	prHif->fgIsPowerOn = true;
	prHif->fgForceReadWriteReg = false;
	prHif->u4RxDataRingSize = prBusInfo->rx_data_ring_size;
	prHif->u4RxEvtRingSize = prBusInfo->rx_evt_ring_size;

	glPopulateMemOps(prChipInfo, prMemOps);

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	if (!kalCreateHifSkbList(prChipInfo)) {
		DBGLOG(HAL, INFO,
		       "Rx data ring using copy path. size[%u]->[%u].\n",
		       prBusInfo->rx_data_ring_size,
		       prBusInfo->rx_data_ring_prealloc_size);

		glUpdateRxCopyMemOps(prMemOps);
		prHif->u4RxDataRingSize =
			prBusInfo->rx_data_ring_prealloc_size;
	}
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function clears hif related info.
 *
 * \param[in] prGlueInfo Pointer to glue info structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void glClearHifInfo(struct GLUE_INFO *prGlueInfo)
{
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
	kalReleaseHifSkbList();
#endif
}

/*----------------------------------------------------------------------------*/
/*!
* \brief This function reset necessary hif related info when chip reset.
*
* \param[in] prGlueInfo Pointer to glue info structure
*
* \return (none)
*/
/*----------------------------------------------------------------------------*/
void glResetHifInfo(struct GLUE_INFO *prGlueInfo)
{
	ASSERT(prGlueInfo);
} /* end of glResetHifInfo() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize bus operation and hif related information, request
 *        resources.
 *
 * \param[out] pvData    A pointer to HIF-specific data type buffer.
 *                       For eHPI, pvData is a pointer to UINT_32 type and
 *                       stores a mapped base address.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t glBusInit(void *pvData)
{
	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Stop bus operation and release resources.
 *
 * \param[in] pvData A pointer to struct net_device.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void glBusRelease(void *pvData)
{
}

static int32_t glBusSetMsiIrq(struct pci_dev *pdev,
	struct GLUE_INFO *prGlueInfo,
	struct BUS_INFO *prBusInfo)
{
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	struct pcie_msi_info *prMsiInfo = &prBusInfo->pcie_msi_info;
	uint8_t i = 0;
	int ret = 0;
	uint32_t pos = 0;
	char *buf;

#define BUF_SIZE 1024U
	buf = (char *) kalMemAlloc(BUF_SIZE, VIR_MEM_TYPE);
	if (!buf) {
		DBGLOG(HAL, ERROR, "buf is NULL");
		goto err;
	}
	kalMemZero(buf, BUF_SIZE);

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		struct pcie_msi_layout *prMsiLayout =
			&prMsiInfo->prMsiLayout[i];
		int irqn = pci_irq_vector(pdev, i);
		int en_wake_ret = 0;

		if (prMsiLayout && !prMsiLayout->top_handler &&
		    !prMsiLayout->thread_handler)
			continue;

		prMsiLayout->irq_num = irqn;
		ret = devm_request_threaded_irq(&pdev->dev,
			prMsiLayout->irq_num,
			prMsiLayout->top_handler,
			prMsiLayout->thread_handler,
			IRQF_SHARED,
			KBUILD_MODNAME,
			prGlueInfo);

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
#if CFG_MTK_MDDP_SUPPORT
		if (prMsiLayout->type == MDDP_INT) {
			struct irq_data *data;

			data = irq_get_irq_data(irqn);
			irq_chip_mask_parent(data);
		}
#endif
#if CFG_MTK_CCCI_SUPPORT
		if (prMsiLayout->type == CCIF_INT) {
			struct irq_data *data;

			data = irq_get_irq_data(irqn);
			irq_chip_mask_parent(data);
			mtk_msi_unmask_to_other_mcu(data, 1);
		}
#endif
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
		en_wake_ret = enable_irq_wake(irqn);
		if ((BUF_SIZE - pos) > 0) {
			pos += kalSnprintf(buf + pos, BUF_SIZE - pos,
				"irqn:%d, ret:%d, en_wake_ret:%d%s",
				irqn, ret, en_wake_ret,
				i == prMsiInfo->u4MsiNum - 1 ? "\n" : "; ");
		}

		if (ret)
			goto err;
	}
	DBGLOG(HAL, INFO, "request_irq info: %s", buf);

	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, BUF_SIZE);

	return 0;

err:
	while (i--) {
		struct pcie_msi_layout *prMsiLayout =
			&prMsiInfo->prMsiLayout[i];
		int irqn = pci_irq_vector(pdev, i);

		if (prMsiLayout && !prMsiLayout->top_handler &&
		    !prMsiLayout->thread_handler)
			continue;

		devm_free_irq(&pdev->dev, irqn, prGlueInfo);
	}

	if (buf)
		kalMemFree(buf, VIR_MEM_TYPE, BUF_SIZE);

	return ret;
#else
	return -EOPNOTSUPP;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a AXI interrupt callback function
 *
 * \param[in] func  pointer to AXI handle
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
static irqreturn_t mtk_axi_isr(int irq, void *dev_instance)
{
	disable_irq_nosync(irq);
	return IRQ_HANDLED;
}

static irqreturn_t mtk_axi_isr_thread(int irq, void *dev_instance)
{
	struct ADAPTER *prAdapter;
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;

	prAdapter = (struct ADAPTER *)dev_instance;
	if (!prAdapter) {
		DBGLOG(HAL, WARN, "NULL prAdapter.\n");
		return IRQ_HANDLED;
	}

	prGlueInfo = get_glue_info_isr(prAdapter->prGlueInfo, irq, -1);
	if (!prGlueInfo)
		return IRQ_NONE;

	prHifInfo = &prGlueInfo->rHifInfo;
	KAL_SET_BIT(1, prHifInfo->ulHifIntEnBits);
	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

static int32_t glBusSetLegacyIrq(struct pci_dev *pdev,
	struct GLUE_INFO *prGlueInfo,
	struct BUS_INFO *prBusInfo)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	prHifInfo = &prGlueInfo->rHifInfo;

	return devm_request_threaded_irq(
		&pdev->dev,
		prHifInfo->u4IrqId,
		mtk_pci_isr,
		mtk_pci_isr_thread,
		IRQF_SHARED,
		KBUILD_MODNAME,
		prGlueInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Setup bus interrupt operation and interrupt handler for os.
 *
 * \param[in] pvData     A pointer to struct net_device.
 * \param[in] pfnIsr     A pointer to interrupt handler function.
 * \param[in] pvCookie   Private data for pfnIsr function.
 *
 * \retval WLAN_STATUS_SUCCESS   if success
 *         NEGATIVE_VALUE   if fail
 */
/*----------------------------------------------------------------------------*/
int32_t glBusSetIrq(void *pvData, void *pfnIsr, void *pvCookie)
{
	struct net_device *prNetDevice = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;
	struct pci_dev *pdev = NULL;
	int ret = 0;

	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *)pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo)
		return -1;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

	prHifInfo->u4IrqId = pdev->irq;
	if (prMsiInfo && prMsiInfo->fgMsiEnabled)
		ret = glBusSetMsiIrq(pdev, prGlueInfo, prBusInfo);
	else
		ret = glBusSetLegacyIrq(pdev, prGlueInfo, prBusInfo);

	if (ret)
		goto exit;

	if (prBusInfo->initPcieInt)
		prBusInfo->initPcieInt(prGlueInfo);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	setupPlatDevIrq(g_prPlatDev, &prHifInfo->u4IrqId_1);
#endif /* CFG_SUPPORT_HOST_OFFLOAD */

exit:
	return ret;
}

static void glBusFreeMsiIrq(struct pci_dev *pdev,
	struct GLUE_INFO *prGlueInfo,
	struct BUS_INFO *prBusInfo)
{
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	struct pcie_msi_info *prMsiInfo = NULL;
	uint8_t dbg[512];
	uint32_t written = 0;
	uint8_t i = 0;

	KAL_TIME_INTERVAL_DECLARATION();

	prMsiInfo = &prBusInfo->pcie_msi_info;
	kalMemZero(dbg, sizeof(dbg));

	KAL_REC_TIME_START();
	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		struct pcie_msi_layout *prMsiLayout =
			&prMsiInfo->prMsiLayout[i];
		int irqn = pci_irq_vector(pdev, i);

		KAL_TIME_INTERVAL_DECLARATION();

		if (prMsiLayout && !prMsiLayout->top_handler &&
		    !prMsiLayout->thread_handler)
			continue;

		KAL_REC_TIME_START();
		synchronize_irq(irqn);
		irq_set_affinity_hint(irqn, NULL);
		devm_free_irq(&pdev->dev, irqn, prGlueInfo);
		KAL_REC_TIME_END();

		written += kalSnprintf(dbg + written,
				       sizeof(dbg) - written,
				       "[%d] %lu, ",
				       irqn,
				       KAL_GET_TIME_INTERVAL());
	}
	KAL_REC_TIME_END();

	DBGLOG(INIT, INFO,
		"Total: %lu us, %s\n",
		KAL_GET_TIME_INTERVAL(),
		dbg);
#endif
}

static void glBusFreeLegacyIrq(struct pci_dev *pdev,
	struct GLUE_INFO *prGlueInfo,
	struct BUS_INFO *prBusInfo)
{
	struct GL_HIF_INFO *prHifInfo = NULL;

	prHifInfo = &prGlueInfo->rHifInfo;

	synchronize_irq(prHifInfo->u4IrqId);
	irq_set_affinity_hint(prHifInfo->u4IrqId, NULL);
	devm_free_irq(&pdev->dev, prHifInfo->u4IrqId, prGlueInfo);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Stop bus interrupt operation and disable interrupt handling for os.
 *
 * \param[in] pvData     A pointer to struct net_device.
 * \param[in] pvCookie   Private data for pfnIsr function.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void glBusFreeIrq(void *pvData, void *pvCookie)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	struct pcie_msi_info *prMsiInfo = NULL;
	struct pci_dev *pdev = NULL;

	prGlueInfo = (struct GLUE_INFO *) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, INFO, "%s no glue info\n", __func__);
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;
	pdev = prHifInfo->pdev;

	if (prMsiInfo->fgMsiEnabled)
		glBusFreeMsiIrq(pdev, prGlueInfo, prBusInfo);
	else
		glBusFreeLegacyIrq(pdev, prGlueInfo, prBusInfo);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	freePlatDevIrq(g_prPlatDev, prHifInfo->u4IrqId_1);
	prHifInfo->u4IrqId_1 = 0;
#endif /* CFG_SUPPORT_HOST_OFFLOAD */
}

u_int8_t glIsReadClearReg(uint32_t u4Address)
{
	return TRUE;
}

void glSetPowerState(struct GLUE_INFO *prGlueInfo, uint32_t ePowerMode)
{
}

void glGetDev(void *ctx, void **dev)
{

	*dev = &((struct pci_dev *)ctx)->dev;
}

void glGetHifDev(struct GL_HIF_INFO *prHif, struct device **dev)
{
	*dev = &(prHif->pdev->dev);
}

void glGetChipInfo(void **prChipInfo)
{
	struct mt66xx_hif_driver_data *prDriverData;

	prDriverData = get_platform_driver_data();
	if (!prDriverData)
		*prChipInfo = NULL;
	else
		*prChipInfo = (void *)prDriverData->chip_info;
}

#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(INIT, WARN, "[SER][L0] not support...\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check HIF state to determine if L0.5 reset shall be postponed.
 *
 * \param[in] prGlueInfo
 *
 * \return TRUE  if L0.5 reset shall be postponed.
 *         FALSE  otherwise
 */
/*----------------------------------------------------------------------------*/
u_int8_t kalCheckWfsysResetPostpone(struct GLUE_INFO *prGlueInfo)
{
	/* TODO */
	return FALSE;
}
#endif

#if CFG_SUPPORT_PCIE_ASPM
static void pcieSetASPML1SS(struct pci_dev *dev, int i4Enable)
{
	int pos;
	uint32_t u4Reg = 0;

	pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_L1PMSS);
	pci_read_config_dword(dev, pos + PCI_L1PMSS_CTR1, &u4Reg);
	u4Reg &= ~PCI_L1PMSS_ENABLE_MASK;
	u4Reg |= i4Enable;
	pci_write_config_dword(dev, pos + PCI_L1PMSS_CTR1, u4Reg);
}
static void pcieSetASPML1(struct pci_dev *dev, int i4Enable)
{
	uint16_t u2Reg = 0;
	int i4Pos = dev->pcie_cap;

	pci_read_config_word(dev, i4Pos + PCI_EXP_LNKCTL, &u2Reg);
	u2Reg &= ~PCI_L1PM_ENABLE_MASK;
	u2Reg |= i4Enable;
	pci_write_config_word(dev, i4Pos + PCI_EXP_LNKCTL, u2Reg);
}
static bool pcieCheckASPML1SS(struct pci_dev *dev, int i4BitMap)
{
	int i4Pos;
	uint32_t u4Reg = 0;

	i4Pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_L1PMSS);


	if (!i4Pos) {
		DBGLOG(INIT, INFO, "L1 PM Substate capability is not found!\n");
		return FALSE;
	}
	pci_read_config_dword(dev, i4Pos + PCI_L1PMSS_CAP, &u4Reg);
	if (i4BitMap != 0) {
		if ((i4BitMap & PCI_L1PM_CAP_ASPM_L12) &
				(!(u4Reg & PCI_L1PM_CAP_ASPM_L12))) {
			DBGLOG(INIT, INFO, "not support ASPM L1.2!\n");
			return FALSE;
		}
		if ((i4BitMap & PCI_L1PM_CAP_ASPM_L11) &
				(!(u4Reg & PCI_L1PM_CAP_ASPM_L11))) {
			DBGLOG(INIT, INFO, "not support ASPM L1.1!\n");
			return FALSE;
		}
	}
	return TRUE;
}
bool glBusConfigASPM(struct pci_dev *dev, int i4Enable)
{

	uint32_t u4Reg = 0;
	struct pci_dev *parent = dev->bus->self;
	int pos = parent->pcie_cap;


	pci_read_config_dword(parent, pos + PCI_EXP_LNKCAP, &u4Reg);
	if (PCIE_ASPM_CHECK_L1(u4Reg)) {
		pos = dev->pcie_cap;
		pci_read_config_dword(dev, pos + PCI_EXP_LNKCAP, &u4Reg);
		if (PCIE_ASPM_CHECK_L1(u4Reg)) {
			pcieSetASPML1(parent, i4Enable);
			pcieSetASPML1(dev, i4Enable);
			DBGLOG(INIT, INFO, "ASPM STATUS %d\n", i4Enable);
			return TRUE;
		}
	}
	return FALSE;

}
bool glBusConfigASPML1SS(struct pci_dev *dev, int i4Enable)
{
	struct pci_dev *parent = dev->bus->self;

	if (pcieCheckASPML1SS(parent, i4Enable)) {
		if (pcieCheckASPML1SS(dev, i4Enable)) {
			pcieSetASPML1SS(parent, i4Enable);
			pcieSetASPML1SS(dev, i4Enable);
			DBGLOG(INIT, INFO, "Config ASPM-L1SS\n");
			return TRUE;
		}
	}
	return FALSE;
}

#endif

static void halPciePreSuspendCmd(struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl = {0};
	uint32_t rStatus;

	rCmdHifCtrl.ucHifType = ENUM_HIF_TYPE_PCIE;
	rCmdHifCtrl.ucHifDirection = ENUM_HIF_TX;
	rCmdHifCtrl.ucHifStop = TRUE;
	rCmdHifCtrl.ucHifSuspend = TRUE;
	rCmdHifCtrl.u4WakeupHifType = ENUM_CMD_HIF_WAKEUP_TYPE_PCIE;

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_HIF_CTRL,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			sizeof(struct CMD_HIF_CTRL), /* u4SetQueryInfoLen */
			(uint8_t *)&rCmdHifCtrl, /* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
			);

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}

static void halPcieResumeCmd(struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl = {0};
	uint32_t rStatus;

	rCmdHifCtrl.ucHifType = ENUM_HIF_TYPE_PCIE;
	rCmdHifCtrl.ucHifDirection = ENUM_HIF_TX;
	rCmdHifCtrl.ucHifStop = FALSE;
	rCmdHifCtrl.ucHifSuspend = FALSE;
	rCmdHifCtrl.u4WakeupHifType = ENUM_CMD_HIF_WAKEUP_TYPE_PCIE;

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_HIF_CTRL,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			sizeof(struct CMD_HIF_CTRL), /* u4SetQueryInfoLen */
			(uint8_t *)&rCmdHifCtrl, /* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
			);

	ASSERT(rStatus == WLAN_STATUS_PENDING);
}

void halPciePreSuspendDone(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo,
	uint8_t *pucEventBuf)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.eSuspendtate =
		PCIE_STATE_PRE_SUSPEND_DONE;
}

void halPciePreSuspendTimeout(
	struct ADAPTER *prAdapter,
	struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.eSuspendtate =
		PCIE_STATE_PRE_SUSPEND_FAIL;
}

void halPcieHwControlVote(
	struct ADAPTER *prAdapter,
	uint8_t enable,
	uint32_t u4WifiUser)
{
	uint8_t voteResult = TRUE;
	int32_t u4VoteState = 0;
#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	int32_t err = 0;
#endif

	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "adapter null\n");
		return;
	}

	if (u4WifiUser >= PCIE_VOTE_USER_NUM) {
		DBGLOG(HAL, ERROR,
			"vote user undefined[%d]\n", u4WifiUser);
		return;
	}

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_WF_VOTE);

	if (enable) {
		/* set bit to 0 to vote enable */
		prAdapter->prGlueInfo->rHifInfo.u4VoteState &=
			~BIT(u4WifiUser);
	} else {
		/* set bit to 1 to vote disable */
		prAdapter->prGlueInfo->rHifInfo.u4VoteState |=
			BIT(u4WifiUser);
	}

	DBGLOG(HAL, TRACE,
		"enable[%d], user[%d], vote state[0x%08X]\n",
		enable, u4WifiUser,
		prAdapter->prGlueInfo->rHifInfo.u4VoteState);

	u4VoteState = prAdapter->prGlueInfo->rHifInfo.u4VoteState;

	/* if all bits are 0's, means all vote to enable hw ctrl */
	if (!u4VoteState)
		voteResult = TRUE;
	else
		voteResult = FALSE;

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	/* vote to enable/disable hw mode */
	if (prAdapter->prGlueInfo->fgIsSuspended) {
		err = mtk_pcie_hw_control_vote(0, voteResult, 1);
		prAdapter->prGlueInfo->fgIsSuspended = FALSE;
	}

	if (err) {
		DBGLOG(HAL, ERROR,
			"hw control mode err[%d]\n", err);
		fgIsBusAccessFailed = TRUE;
		GL_DEFAULT_RESET_TRIGGER(prAdapter,
			RST_PCIE_NOT_READY);
	}
#endif
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_WF_VOTE);
}

int32_t glBusFuncOn(void)
{
	int ret = 0;

#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct CHIP_DBG_OPS *prDbgOps = NULL;

	/*
	 * Due to connsys chip may be powered on before platform is powered on,
	 * need to remove pcie port first to ensure no resource is occupied.
	 */
	mtk_pcie_remove_port(0);
	ret = mtk_pcie_probe_port(0);
	if (ret) {
		DBGLOG(HAL, ERROR, "mtk_pcie_probe_port failed, ret=%d\n",
			ret);
		return ret;
	}
#endif

	ret = pci_register_driver(&mtk_pci_driver);
	if (ret) {
		DBGLOG(HAL, ERROR, "pci_register_driver failed, ret=%d\n",
			ret);
#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
		mtk_pcie_remove_port(0);
#endif
		return ret;
	}

	if (g_fgDriverProbed == FALSE) {
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		glGetChipInfo((void **)&prChipInfo);
		if (prChipInfo == NULL) {
			DBGLOG(HAL, ERROR, "prChipInfo in NULL\n");
			goto exit_dump;
		}

		prDbgOps = prChipInfo->prDebugOps;
		if (prDbgOps == NULL) {
			DBGLOG(HAL, ERROR, "prDebugOps in NULL\n");
			goto exit_dump;
		}

		/* Notify BT to start */
		ret = connv3_hif_dbg_start(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT);
		if (ret != 0) {
			DBGLOG(HAL, ERROR, "connv3_hif_dbg_start failed.\n");
			goto exit_dump;
		}

		if (prDbgOps->dumpPcieCr)
			prDbgOps->dumpPcieCr();

		/* Notify BT to end */
		ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT);
		if (ret != 0) {
			DBGLOG(HAL, ERROR, "connv3_hif_dbg_end failed.\n");
			goto exit_dump;
		}

exit_dump:
#endif
		pci_unregister_driver(&mtk_pci_driver);
#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
		mtk_pcie_remove_port(0);
#endif
		ret = -EINVAL;
	}

	return ret;
}

void glBusFuncOff(void)
{
	if (g_fgDriverProbed) {
		pci_unregister_driver(&mtk_pci_driver);
		g_fgDriverProbed = FALSE;
	}
#if IS_ENABLED(CFG_MTK_WIFI_PCIE_SUPPORT)
	mtk_pcie_remove_port(0);
#endif
}

#if CFG_SUPPORT_PCIE_GEN_SWITCH
int mtk_pcie_speed(struct pci_dev *dev, int speed)
{
	int ret;
	int pos, ppos;
	u16 linksta = 0, plinksta = 0, plinkctl2 = 0;
	struct pci_dev *parent;

	pos = dev->pcie_cap;
	pci_read_config_word(dev, pos + PCI_EXP_LNKSTA, &linksta);
	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &plinksta);
	DBGLOG(HAL, INFO, "[Gen_Switch] before pcie link status: 0x%x",
		plinksta);
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL2, &plinkctl2);
	if ((plinkctl2 & PCI_SPEED_MASK) != speed) {
		plinkctl2 &= ~PCI_SPEED_MASK;
		plinkctl2 |= speed;
		pci_write_config_word(parent,
			ppos + PCI_EXP_LNKCTL2, plinkctl2);
	}
	if (((linksta & PCI_EXP_LNKSTA_CLS) == speed) &&
		((plinksta & PCI_EXP_LNKSTA_CLS) == speed))
		return 0;
	ret = mtk_pcie_retrain(dev);
	if (ret) {
		DBGLOG(HAL, ERROR, "retrain fails\n");
		return ret;
	}
	pci_read_config_word(parent, pos + PCI_EXP_LNKSTA, &linksta);
	DBGLOG(HAL, INFO, "[Gen_Switch] after pcie link status: 0x%x", linksta);

	if ((linksta & PCI_EXP_LNKSTA_CLS) != speed) {
		DBGLOG(HAL, ERROR, "can't train status = 0x%x\n", linksta);
		return -1;
	}

	return 0;
}

int mtk_pcie_retrain(struct pci_dev *dev)
{
	u16 reg16 = 0;
	struct pci_dev *parent;
	int pos, ppos;
	unsigned long start_jiffies;

	parent = dev->bus->self;
	ppos = parent->pcie_cap;
	pos = dev->pcie_cap;
	/* Retrain link */
	pci_read_config_word(parent, ppos + PCI_EXP_LNKCTL, &reg16);
	reg16 |= PCI_EXP_LNKCTL_RL;
	pci_write_config_word(parent, ppos + PCI_EXP_LNKCTL, reg16);
	/* Wait for link training end. Break out after waiting for timeout */
	start_jiffies = jiffies;
	mdelay(10);
	for (;;) {
		pci_read_config_word(parent, ppos + PCI_EXP_LNKSTA, &reg16);

		if (!(reg16 & PCI_EXP_LNKSTA_LT))
			return 0;

		if (time_after(jiffies, start_jiffies + LINK_RETRAIN_TIMEOUT))
			return -1;

		mdelay(1);
	}
	DBGLOG(HAL, ERROR, "mtk_pcie_retrain\n");
	return 0;
}
#endif
