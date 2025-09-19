// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

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
#include <linux/msi.h>

#if CFG_SUPPORT_RX_PAGE_POOL
#if KERNEL_VERSION(6, 6, 0) > LINUX_VERSION_CODE
#include <net/page_pool.h>
#else
#include <net/page_pool/helpers.h>
#endif
#endif

#include "mt66xx_reg.h"
#include "wlan_pinctrl.h"

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif

#if (CFG_PCIE_GEN_SWITCH == 1 || CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
#include "mddp.h"
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
#ifdef CFG_COMBO_SLT_GOLDEN
#define NIC6653_PCIe_DEVICE_ID1 0xE653 /*used for SLT golden */
#else
#define NIC6653_PCIe_DEVICE_ID1 0x6653
#endif
#define NIC6655_PCIe_DEVICE_ID1 0x3107
#define NIC6655_PCIe_DEVICE_ID2 0x6655
#define NIC7990_PCIe_DEVICE_ID 0x7990
#define NIC7927_PCIe_DEVICE_ID 0x7927
#define NIC7925_PCIe_DEVICE_ID 0x7925
#define NIC7935_PCIe_DEVICE_ID1 0x3107 /* used for FPGA */
#ifdef CFG_COMBO_SLT_GOLDEN
#define NIC7935_PCIe_DEVICE_ID2 0xF935 /* used for SLT golden */
#else
#define NIC7935_PCIe_DEVICE_ID2 0x7935 /* used for asic & FPGA */
#endif

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
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7927_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639},
#endif /* MT6639 */
#ifdef MT6653
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6653_PCIe_DEVICE_ID1),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6653},
#endif /* MT6653 */
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
#ifdef MT7925
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7925_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7925},
#endif /* MT7925 */
#ifdef MT7935
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7935_PCIe_DEVICE_ID1),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7935},
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7935_PCIe_DEVICE_ID2),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7935},
#endif /* MT7935 */
	{ /* end: all zeroes */ },
};

static const struct platform_device_id mtk_wifi_ids[] = {
	{	.name = "CONNAC",
#if defined(BELLWETHER)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_bellwether
#elif defined(MT6639)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639
#elif defined(MT6653)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6653
#elif defined(MT6655)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6655
#elif defined(MT7935)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7935
#endif
	},

	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(pci, mtk_pci_ids);

#ifdef CONFIG_OF
const struct of_device_id mtk_wifi_of_ids[] = {
	{.compatible = "mediatek,wifi",},
	{}
};

#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
const struct of_device_id mtk_wifi_misc_of_ids[] = {
	{.compatible = "mediatek,wifi_misc",},
	{}
};
#endif

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
const struct of_device_id mtk_wifi_tx_cma_of_ids[] = {
	{.compatible = "mediatek,wifi_tx_cma",},
	{}
};
#endif

#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
const struct of_device_id mtk_wifi_tx_cma_non_cache_of_ids[] = {
	{.compatible = "mediatek,wifi_tx_cma",},
	{}
};
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
#endif

#define HIF_WFDMA_INT_BIT	0
#define HIF_MAWD_INT_BIT	1
#define HIF_WED_INT_BIT		2

#if (CFG_PCIE_GEN_SWITCH == 1)
#define CHECK_RX_TIMEOUT (1000*50)
#define GEN_SWITCH_TIMEOUT (1000*100)
#define DEFAULT_IDLE	0
#define WF_RX_IDLE	1
#define FW_RX_IDLE	2
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
#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
static remove_card pfWlanShutdown;
#endif
#if CFG_MTK_WIFI_AER_RESET
static u_int8_t g_AERRstTriggered;
static u_int8_t g_AERL05Rst;
static uint32_t g_u4AERDumpInfo;
#endif

static struct platform_driver mtk_wifi_driver = {
	.driver = {
		.name = "wlan",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_wifi_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.id_table = mtk_wifi_ids,
	.probe = NULL,
	.remove = NULL,
	.shutdown = NULL,
};

#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
static struct platform_driver mtk_wifi_misc_driver = {
	.driver = {
		.name = "wlan_misc",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_wifi_misc_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.id_table = mtk_wifi_ids,
	.probe = NULL,
	.remove = NULL,
};
#endif

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
static struct platform_driver mtk_wifi_tx_cma_driver = {
	.driver = {
		.name = "wlan_tx_cma",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_wifi_tx_cma_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.id_table = mtk_wifi_ids,
	.probe = NULL,
	.remove = NULL,
};
#endif

#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
static struct platform_driver mtk_wifi_tx_cma_non_cache_driver = {
	.driver = {
		.name = "wifi_tx_cma",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_wifi_tx_cma_non_cache_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.id_table = mtk_wifi_ids,
	.probe = NULL,
	.remove = NULL,
};
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */

#if CFG_MTK_WIFI_AER_RESET
static pci_ers_result_t mtk_pci_error_detected(struct pci_dev *pdev,
	pci_channel_state_t state);
static pci_ers_result_t mtk_pci_error_slot_reset(struct pci_dev *pdev);
static void mtk_pci_error_resume(struct pci_dev *pdev);

static const struct pci_error_handlers mtk_pci_err_handler = {
	.error_detected = mtk_pci_error_detected,
	.slot_reset     = mtk_pci_error_slot_reset,
	.resume         = mtk_pci_error_resume,
};
#endif

static struct pci_driver mtk_pci_driver = {
	.name = KBUILD_MODNAME,
	.id_table = mtk_pci_ids,
	.probe = NULL,
	.remove = NULL,
#if CFG_MTK_WIFI_AER_RESET
	.err_handler = &mtk_pci_err_handler,
#endif
};

static struct GLUE_INFO *g_prGlueInfo;
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
static u64 g_u8CsrOffset;
static u32 g_u4CsrSize;
#endif
static u_int8_t g_fgDriverProbed = FALSE;
static struct pci_dev *g_prDev;

#if (CFG_PCIE_GEN_SWITCH == 1)
u_int8_t g_ucReceiveGenSwitch;
u_int8_t g_ucBypassException;
#endif


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

static irqreturn_t mtk_wifi_isr(int irq, void *dev_instance);
static irqreturn_t mtk_wifi_isr_thread(int irq, void *dev_instance);
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
	struct pcie_msi_info *prMsiInfo;
#if HIF_INT_TIME_DEBUG
	struct BUS_INFO *prBusInfo = NULL;
#endif
#if PCIE_ISR_DEBUG_LOG
	static DEFINE_RATELIMIT_STATE(_rs, 2 * HZ, 1);
#endif

	prGlueInfo = (struct GLUE_INFO *)dev_instance;
	if (!prGlueInfo) {
		DBGLOG_LIMITED(HAL, INFO, "No glue info in %s(%d, %d)\n",
			       __func__, irq, msi_idx);
		enable_irq(irq);
		return NULL;
	}

	/* Only record int time for data */
	if (msi_idx == -1 || msi_idx == PCIE_MSI_RX_DATA_BAND0 ||
		msi_idx == PCIE_MSI_RX_DATA_BAND1)
		prGlueInfo->u8HifIntTime = sched_clock();

	prAdapter = prGlueInfo->prAdapter;
	prMsiInfo = &prAdapter->chip_info->bus_info->pcie_msi_info;
	if (msi_idx >= 0 && msi_idx < PCIE_MSI_NUM)
		GLUE_INC_REF_CNT(prAdapter->rHifStats.u4MsiIsrCount[msi_idx]);
	else
		GLUE_INC_REF_CNT(prAdapter->rHifStats.u4HwIsrCount);

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
		DBGLOG_LIMITED(HAL, INFO, "GLUE_FLAG_HALT skip INT(%d, %d)\n",
			       irq, msi_idx);
		if (msi_idx >= 0 && msi_idx < PCIE_MSI_NUM)
			KAL_SET_BIT(msi_idx, prMsiInfo->ulEnBits);
		else
			KAL_SET_BIT(HIF_WFDMA_INT_BIT,
				    prGlueInfo->rHifInfo.ulHifIntEnBits);
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

static void mtk_pci_msi_mask_irq(uint32_t u4IrqNum)
{
	struct irq_data *data;

	data = irq_get_irq_data(u4IrqNum);
	if (data)
		pci_msi_mask_irq(data);
}

static void mtk_pci_msi_unmask_irq(uint32_t u4IrqNum)
{
	struct irq_data *data;

	data = irq_get_irq_data(u4IrqNum);
	if (data)
		pci_msi_unmask_irq(data);
}

void mtk_pci_msi_enable_irq(uint32_t u4Irq, uint32_t u4Bit)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo;

	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;

	if (prBusInfo->pcieMsiUnmaskIrq)
		prBusInfo->pcieMsiUnmaskIrq(u4Irq, u4Bit);
	else if (prBusInfo->is_en_drv_ctrl_pci_msi_irq)
		mtk_pci_msi_unmask_irq(u4Irq);
	else
		enable_irq(u4Irq);
}

void mtk_pci_msi_disable_irq(uint32_t u4Irq, uint32_t u4Bit)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo;

	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;

	if (prBusInfo->pcieMsiMaskIrq)
		prBusInfo->pcieMsiMaskIrq(u4Irq, u4Bit);
	else if (prBusInfo->is_en_drv_ctrl_pci_msi_irq)
		mtk_pci_msi_mask_irq(u4Irq);
	else
		disable_irq_nosync(u4Irq);
}

u_int8_t mtk_pci_is_wfdma_ready(struct GLUE_INFO *prGlueInfo)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct HIF_STATS *prHifStats = &prGlueInfo->prAdapter->rHifStats;

	glGetChipInfo((void **)&prChipInfo);
	if (prChipInfo->isWfdmaRxReady &&
	    !prChipInfo->isWfdmaRxReady(prGlueInfo->prAdapter)) {
		GLUE_INC_REF_CNT(prHifStats->u4EmptyIntCount);
		return FALSE;
	}

	return TRUE;
}

u_int8_t mtk_pci_is_int_ready(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct pcie_msi_info *prMsiInfo;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMsiInfo = &prGlueInfo->prAdapter->chip_info->bus_info->pcie_msi_info;

	return prMsiInfo->ulEnBits == 0 &&
		GLUE_GET_REF_CNT(prHifInfo->u4IntBitSetCnt) == 0;
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
	struct GLUE_INFO *prGlueInfo;
	struct GL_HIF_INFO *prHifInfo;
	struct pcie_msi_info *prMsiInfo;
	struct pcie_msi_layout *prMsiLayout;
	irqreturn_t irqret = IRQ_WAKE_THREAD;
	int i;

	prGlueInfo = (struct GLUE_INFO *)dev_instance;
	if (!prGlueInfo) {
		DBGLOG_LIMITED(HAL, INFO, "No glue info(%d)\n", irq);
		disable_irq_nosync(irq);
		return IRQ_NONE;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
	prMsiInfo = &prGlueInfo->prAdapter->chip_info->bus_info->pcie_msi_info;

	GLUE_INC_REF_CNT(prHifInfo->u4IntBitSetCnt);
	if (!prMsiInfo || !prMsiInfo->fgMsiEnabled) {
		if (KAL_TEST_BIT(HIF_WFDMA_INT_BIT,
				 prHifInfo->ulHifIntEnBits)) {
			irqret = IRQ_NONE;
			goto exit;
		}

		disable_irq_nosync(irq);
		KAL_SET_BIT(HIF_WFDMA_INT_BIT, prHifInfo->ulHifIntEnBits);
		goto exit;
	}

	if (!mtk_pci_is_wfdma_ready(prGlueInfo)) {
		irqret = IRQ_HANDLED;
		goto exit;
	}

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->irq_num == irq) {
			if (KAL_TEST_BIT(i, prMsiInfo->ulEnBits)) {
				irqret = IRQ_NONE;
				goto exit;
			}

			mtk_pci_msi_disable_irq(irq, i);
			KAL_SET_BIT(i, prMsiInfo->ulEnBits);
			goto exit;
		}
	}
#if CFG_SUPPORT_WED_PROXY
	if (IsWedAttached()) {
		if (KAL_TEST_BIT(HIF_WED_INT_BIT,
				 prHifInfo->ulHifIntEnBits)) {
			irqret = IRQ_NONE;
			goto exit;
		}
		disable_irq_nosync(irq);
		KAL_SET_BIT(HIF_WED_INT_BIT, prHifInfo->ulHifIntEnBits);
	}
#endif

exit:
	GLUE_DEC_REF_CNT(prHifInfo->u4IntBitSetCnt);

	return irqret;
}

irqreturn_t mtk_pci_isr_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = get_glue_info_isr(dev_instance, irq, 0);
	if (!prGlueInfo)
		return IRQ_NONE;

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

uint32_t mtk_pci_read_msi_mask(struct GLUE_INFO *prGlueInfo)
{
	struct pci_dev *dev = prGlueInfo->rHifInfo.pdev;
	uint16_t control;
	uint32_t mask;
	int pos;

	pci_read_config_word(dev, dev->msi_cap + PCI_MSI_FLAGS, &control);

	if (control & PCI_MSI_FLAGS_64BIT)
		pos = dev->msi_cap + PCI_MSI_MASK_64;
	else
		pos = dev->msi_cap + PCI_MSI_MASK_32;

	pci_read_config_dword(dev, pos, &mask);

	return mask;
}

void mtk_pci_msi_unmask_all_irq(struct GLUE_INFO *prGlueInfo)
{
	struct GL_HIF_INFO *prHifInfo;
	struct pcie_msi_info *prMsiInfo;
	struct pcie_msi_layout *prMsiLayout;
	int i;

	prHifInfo = &prGlueInfo->rHifInfo;
	prMsiInfo = &prGlueInfo->prAdapter->chip_info->bus_info->pcie_msi_info;
	if (!prMsiInfo || !prMsiInfo->fgMsiEnabled)
		return;

	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (!prMsiLayout ||
		    (!prMsiLayout->top_handler &&
		     !prMsiLayout->thread_handler) ||
		    prMsiLayout->type != AP_INT)
			continue;

		mtk_pci_msi_enable_irq(prMsiLayout->irq_num, i);
	}
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

	GLUE_INC_REF_CNT(prHifInfo->u4IntBitSetCnt);

	if (!prMsiInfo->fgMsiEnabled) {
		if (KAL_TEST_AND_CLEAR_BIT(HIF_WFDMA_INT_BIT,
					   prHifInfo->ulHifIntEnBits)) {
			enable_irq(prHifInfo->u4IrqId);
			GLUE_INC_REF_CNT(prAdapter->rHifStats.u4EnIrqCount);
		}
		goto exit;
	}
#if CFG_SUPPORT_WED_PROXY
	if (IsWedAttached()) {
		if (KAL_TEST_AND_CLEAR_BIT(HIF_WED_INT_BIT,
					   prHifInfo->ulHifIntEnBits)) {
			enable_irq(prHifInfo->u4IrqId);
			GLUE_INC_REF_CNT(prAdapter->rHifStats.u4EnIrqCount);
		}
		goto exit;
	}
#endif
	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->type != AP_INT ||
		    !prMsiLayout->irq_num)
			continue;

		if (KAL_TEST_AND_CLEAR_BIT(i, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_enable_irq(prMsiLayout->irq_num, i);
			GLUE_INC_REF_CNT(prAdapter->rHifStats.u4EnIrqCount);
		}
	}

exit:
	GLUE_DEC_REF_CNT(prHifInfo->u4IntBitSetCnt);
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
		if (!KAL_TEST_BIT(HIF_WFDMA_INT_BIT,
				  prHifInfo->ulHifIntEnBits)) {
			disable_irq_nosync(prHifInfo->u4IrqId);
			KAL_SET_BIT(HIF_WFDMA_INT_BIT,
				  prHifInfo->ulHifIntEnBits);
		}
		return;
	}
#if CFG_SUPPORT_WED_PROXY
	if (IsWedAttached()) {
		if (!KAL_TEST_BIT(HIF_WED_INT_BIT,
				  prHifInfo->ulHifIntEnBits)) {
			disable_irq_nosync(prHifInfo->u4IrqId);
			KAL_SET_BIT(HIF_WED_INT_BIT, prHifInfo->ulHifIntEnBits);
		}
		return;
	}
#endif
	for (i = 0; i < prMsiInfo->u4MsiNum; i++) {
		prMsiLayout = &prMsiInfo->prMsiLayout[i];
		if (prMsiLayout->type != AP_INT ||
		    !prMsiLayout->irq_num)
			continue;

		if (!KAL_TEST_BIT(i, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_disable_irq(prMsiLayout->irq_num, i);
			KAL_SET_BIT(i, prMsiInfo->ulEnBits);
		}
	}
}

uint8_t pcie_backup_config_space_settings(
	struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = NULL;
	int i;
	uint32_t ret = 0;

	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "adapter is NULL\n");
		return -1;
	}

	prBusInfo = prAdapter->chip_info->bus_info;

	for (i = 0; i < PCIE_EP_CONFIG_SPACE_SIZE; i++) {
		ret = glReadPcieCfgSpace(i * 4,
			&prBusInfo->u4ConfigSpace[i]);
		if (ret != 0) {
			prBusInfo->ucConfigSpaceBkDone = 0;
			DBGLOG(HAL, ERROR, "cfg space bk failed\n");
			return -1;
		}
	}
	prBusInfo->ucConfigSpaceBkDone = 1;
	DBGLOG(HAL, INFO, "cfg space bk pass\n");

	return 0;
}

uint8_t pcie_restore_config_space_settings(
	struct ADAPTER *prAdapter)
{
	struct BUS_INFO *prBusInfo = NULL;
	int i;
	uint32_t ret = 0;

	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "adapter is NULL\n");
		return -1;
	}

	prBusInfo = prAdapter->chip_info->bus_info;

	for (i = 0; i < PCIE_EP_CONFIG_SPACE_SIZE; i++) {
		ret = glWritePcieCfgSpace(i * 4,
			prBusInfo->u4ConfigSpace[i]);
		if (ret != 0) {
			DBGLOG(HAL, ERROR, "cfg space rs failed\n");
			return -1;
		}
	}
	DBGLOG(HAL, INFO, "cfg space rs pass\n");

	return 0;
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

#if CFG_MTK_WIFI_FW_LOG_MMIO || CFG_MTK_WIFI_FW_LOG_EMI
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
#endif

irqreturn_t pcie_drv_own_top_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = (struct GLUE_INFO *)dev_instance;

	if (prGlueInfo)
		set_bit(GLUE_FLAG_DRV_OWN_INT_BIT, &prGlueInfo->ulFlag);

	return IRQ_WAKE_THREAD;
}

irqreturn_t pcie_drv_own_thread_handler(int irq, void *dev_instance)
{
	DBGLOG(HAL, TRACE, "driver own IRQ handled.\n");
	return IRQ_HANDLED;
}

#if (CFG_MTK_MDDP_SUPPORT || CFG_MTK_CCCI_SUPPORT)
irqreturn_t mtk_md_dummy_pci_interrupt(int irq, void *dev_instance)
{
	return IRQ_HANDLED;
}
#endif

u_int8_t pcie_check_status_is_linked(void)
{
	uint32_t vnd_id = 0;

	if (glReadPcieCfgSpace(PCI_VENDOR_ID, &vnd_id) == WLAN_STATUS_FAILURE)
		return FALSE;

	if (vnd_id == 0 || vnd_id == 0xffffffff) {
		DBGLOG(HAL, WARN, "PCIE link down\n");
		return FALSE;
	}
	DBGLOG_LIMITED(HAL, INFO, "PCI_VENDOR_ID: 0x%X\n", vnd_id);
	return TRUE;
}

#if CFG_MTK_WIFI_AER_RESET
static pci_ers_result_t mtk_pci_error_detected(struct pci_dev *pdev,
	pci_channel_state_t state)
{
	pci_ers_result_t res = PCI_ERS_RESULT_NONE;
	uint32_t dump = 0;
	u_int8_t fgNeedReset = FALSE;

	DBGLOG(HAL, INFO,
		"mtk_pci_error_detected state: %d, resetting: %d %d\n",
		state, g_AERRstTriggered, kalIsResetting());

	kalDumpPlatGPIOStat();

	if (!pci_is_enabled(pdev)) {
		DBGLOG(HAL, INFO, "pcie is disable\n");
		goto exit;
	}

#if CFG_MTK_WIFI_PCIE_SUPPORT
	dump = mtk_pcie_dump_link_info(0);
	g_u4AERDumpInfo = dump;
#endif

	if (fgIsPcieDataTransDisabled == FALSE &&
		state == pci_channel_io_normal &&
		dump & BIT(6) &&
		pcie_check_status_is_linked() == FALSE) {
		DBGLOG(HAL, WARN, "PCIE link down\n");
		/* block PCIe access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
		mtk_pcie_disable_data_trans(0);
		fgIsPcieDataTransDisabled = TRUE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
		wlanUpdateBusAccessStatus(TRUE);
#ifdef CFG_MTK_WIFI_CONNV3_SUPPORT
		fgTriggerDebugSop = TRUE;
#endif
	}

	if (g_AERRstTriggered)
		goto exit;

	if (state == pci_channel_io_normal) {
		/* bit[6]: Completion timeout status */
		if (dump & BIT(6)) {
			fgNeedReset = TRUE;
			wlanUpdateBusAccessStatus(TRUE);
			if (pcie_check_status_is_linked() == TRUE) {
#if CFG_MTK_WIFI_AER_L05_RESET
				g_AERL05Rst = TRUE;
#endif
			}
		}

		/* bit[7]: RxErr */
		else if (dump & BIT(7)) {
			/* block PCIe access */
#if CFG_MTK_WIFI_PCIE_SUPPORT
			mtk_pcie_disable_data_trans(0);
#endif
			fgNeedReset = TRUE;
			wlanUpdateBusAccessStatus(TRUE);
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
			fgTriggerDebugSop = TRUE;
#endif
		}
	} else {
		pci_disable_device(pdev);
		fgNeedReset = TRUE;
		wlanUpdateBusAccessStatus(TRUE);
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		fgTriggerDebugSop = TRUE;
#endif

		/* SDES trigger reset directly */
		if (dump & BIT(10)) {
			DBGLOG(HAL, ERROR, "pcie SDES detected.\n");
			g_AERRstTriggered = TRUE;
			kalSetHifAerResetEvent(g_prGlueInfo);
		}
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
#define AER_RST_STR			"Whole chip reset by AER"
#define AER_RST_STR_MALFTLP		"Whole chip reset by AER - MalfTLP"
#define AER_RST_STR_SDES		"Whole chip reset by AER - SDES"
#define AER_RST_STR_RXERR		"Whole chip reset by AER - RxErr"
#define AER_RSN_SIZE			50

	struct GLUE_INFO *prGlueInfo = g_prGlueInfo;
	static char aucAerRsn[AER_RSN_SIZE];
	uint32_t pos = 0;
	char *reason = NULL;
	enum _ENUM_CHIP_RESET_REASON_TYPE_T eReason;

	DBGLOG(HAL, INFO, "mtk_pci_error_slot_reset, L05_rst: %d\n",
		g_AERL05Rst);

	kalMemZero(aucAerRsn, AER_RSN_SIZE);
	pos = kalScnprintf(aucAerRsn, AER_RSN_SIZE, AER_RST_STR);
	pos += kalScnprintf(aucAerRsn + pos, AER_RSN_SIZE - pos,
			    " [0x%x]", g_u4AERDumpInfo);

	if (g_u4AERDumpInfo & BIT(7)) {
		reason = AER_RST_STR_RXERR;
		eReason = RST_AER_RXERR;
	} else if (g_u4AERDumpInfo & BIT(8)) {
		reason = AER_RST_STR_MALFTLP;
		eReason = RST_AER_MALFTLP;
	} else if (g_u4AERDumpInfo & BIT(10)) {
		reason = AER_RST_STR_SDES;
		eReason = RST_AER_SDES;
	} else {
		reason = AER_RST_STR;
		eReason = RST_AER;
	}

	if (g_AERL05Rst) {
		GL_USER_DEFINE_RESET_TRIGGER(prGlueInfo->prAdapter,
			eReason, RST_FLAG_WF_RESET);
	} else {
		glSetRstReasonString(reason);
		glResetWholeChipResetTrigger(reason);
	}

	DBGLOG(HAL, INFO, "%s\n", aucAerRsn);

	return PCI_ERS_RESULT_DISCONNECT;
}

static void mtk_pci_error_resume(struct pci_dev *pdev)
{
	struct GLUE_INFO *prGlueInfo = g_prGlueInfo;

	DBGLOG(HAL, INFO, "mtk_pci_error_resume\n");

	if (!prGlueInfo || !prGlueInfo->prAdapter)
		return;

	/* trigger driver SER after AER */
	prGlueInfo->prAdapter->u4HifChkFlag |= HIF_DRV_SER;
	kalSetHifDbgEvent(prGlueInfo);
}
#endif

void mtk_trigger_aer_slot_reset(void)
{
#if CFG_MTK_WIFI_AER_RESET
	mtk_pci_error_slot_reset(NULL);
#endif /* CFG_MTK_WIFI_AER_RESET */
}


u_int8_t mtk_get_aer_triggered(void)
{
#if CFG_MTK_WIFI_AER_RESET
	return g_AERRstTriggered;
#else
	return 0;
#endif
}

static int32_t setupPlatDevIrq(
	struct platform_device *pdev,
	struct GLUE_INFO *prGlueInfo,
	uint32_t *pu4IrqId)
{
	uint32_t u4IrqId = 0;
	int ret = 0;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	int en_wake_ret = 0;
#endif

	if (!pdev || !prGlueInfo)
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
		mtk_wifi_isr,
		mtk_wifi_isr_thread,
		IRQF_SHARED,
		mtk_wifi_driver.driver.name,
		&prGlueInfo->rHifInfo);
	if (ret != 0) {
		DBGLOG(INIT, INFO, "request_irq(%u) ERROR(%d)\n",
		       u4IrqId, ret);
		goto exit;
	}

	en_wake_ret = enable_irq_wake(u4IrqId);
	if (en_wake_ret)
		DBGLOG(INIT, INFO, "enable_irq_wake(%u) ERROR(%d)\n",
		       u4IrqId, en_wake_ret);

exit:
#endif
	*pu4IrqId = u4IrqId;
	return ret;
}

void freePlatDevIrq(
	struct platform_device *pdev,
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4IrqId)
{
	if (!pdev || !prGlueInfo || !u4IrqId)
		return;

	synchronize_irq(u4IrqId);
	irq_set_affinity_hint(u4IrqId, NULL);
	devm_free_irq(&pdev->dev, u4IrqId, &prGlueInfo->rHifInfo);
}

static int wifiDmaSetup(struct platform_device *pdev,
		struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	u64 dma_mask;
	int ret = 0;

	prChipInfo = prDriverData->chip_info;

	ret = halInitResvMem(pdev, WIFI_RSV_MEM_WFDMA);
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

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
static bool wifiCsrIoremap(struct platform_device *pdev)
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
	g_u8CsrOffset = wifi_resource_start(pdev, 0);
	g_u4CsrSize = wifi_resource_len(pdev, 0);
#endif

	prDriverData = get_platform_driver_data();
	if (!prDriverData) {
		DBGLOG(INIT, ERROR, "driver data is NULL\n");
		return false;
	}
	prChipInfo = prDriverData->chip_info;

	if (prChipInfo->HostCSRBaseAddress) {
		DBGLOG(INIT, ERROR, "HostCSRBaseAddress not iounmap!\n");
		return false;
	}

	request_mem_region(g_u8CsrOffset, g_u4CsrSize, wifi_name(pdev));

	/* map physical address to virtual address for accessing register */
#ifdef CONFIG_OF
	prChipInfo->HostCSRBaseAddress = of_iomap(node, 0);
#else
	prChipInfo->HostCSRBaseAddress = ioremap(g_u8CsrOffset, g_u4CsrSize);
#endif

	if (!prChipInfo->HostCSRBaseAddress) {
		DBGLOG(INIT, INFO,
			"ioremap failed for device %s, region 0x%X @ 0x%lX\n",
			wifi_name(pdev), g_u4CsrSize, g_u8CsrOffset);
		release_mem_region(g_u8CsrOffset, g_u4CsrSize);
		return false;
	}

	prChipInfo->u4HostCsrOffset = (uint32_t)g_u8CsrOffset;
	prChipInfo->u4HostCsrSize = g_u4CsrSize;

	DBGLOG(INIT, INFO,
	       "HostCSRBaseAddress:0x%llX ioremap region 0x%X @ 0x%llX\n",
	       (uint64_t)prChipInfo->HostCSRBaseAddress,
	       g_u4CsrSize, g_u8CsrOffset);

	return true;
}

static void wifiCsrIounmap(struct platform_device *pdev)
{
	struct mt66xx_chip_info *prChipInfo;

	glGetChipInfo((void **)&prChipInfo);

	if (!prChipInfo || !prChipInfo->HostCSRBaseAddress)
		return;

	/* Unmap CSR base address */
	iounmap(prChipInfo->HostCSRBaseAddress);
	release_mem_region(g_u8CsrOffset, g_u4CsrSize);

	prChipInfo->HostCSRBaseAddress = NULL;
	g_u8CsrOffset = 0;
	g_u4CsrSize = 0;
}
#endif /* CFG_SUPPORT_HOST_OFFLOAD */

static void wifiSetupFwFlavor(struct platform_device *pdev,
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

static int mtk_wifi_probe(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	int ret = 0;

	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_wifi_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;
	prChipInfo->platform_device = (void *) pdev;

	platform_set_drvdata(pdev, (void *)prDriverData);

	wifiSetupFwFlavor(pdev, prDriverData);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	if (!wifiCsrIoremap(pdev))
		goto exit;
#endif

	ret = wifiDmaSetup(pdev, prDriverData);
	if (ret)
		goto exit;

#if (CFG_MTK_ANDROID_WMT == 1)
	emi_mem_init(prChipInfo, pdev);
#endif
#if CFG_SUPPORT_WIFI_RSV_MEM
	ret = halAllocHifMem(pdev, prDriverData);
	if (ret)
		goto exit;
#endif

#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 1) && (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 0)
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
	DBGLOG(INIT, INFO, "mtk_wifi_probe() done, ret: %d\n", ret);
	return ret;
}

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
static int mtk_wifi_remove(struct platform_device *pdev)
#else
static void mtk_wifi_remove(struct platform_device *pdev)
#endif
{
#if (CFG_MTK_ANDROID_WMT == 1)
	struct mt66xx_hif_driver_data *prDriverData =
		platform_get_drvdata(pdev);
	struct mt66xx_chip_info *prChipInfo = prDriverData->chip_info;
#endif

#if CFG_SUPPORT_THERMAL_QUERY
	thermal_cbs_unregister(pdev);
#endif
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	wifiCsrIounmap(pdev);
#endif
#if CFG_SUPPORT_WIFI_RSV_MEM
	halFreeHifMem(pdev, WIFI_RSV_MEM_WFDMA);
#endif
#if (CFG_MTK_ANDROID_WMT == 1)
	emi_mem_uninit(prChipInfo, pdev);
#endif
#if (CFG_SUPPORT_PAGE_POOL_USE_CMA == 1) && (CFG_SUPPORT_DYNAMIC_PAGE_POOL == 0)
	kalReleaseHifSkbList();
#endif
	platform_set_drvdata(pdev, NULL);

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
	return 0;
#endif
}

#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
static void mtk_wifi_shutdown(struct platform_device *pdev)
{
	wfsys_lock();
	if (g_fgDriverProbed && pfWlanShutdown) {
		DBGLOG(INIT, INFO, "do shutdown\n");
		pfWlanShutdown();
		g_fgDriverProbed = FALSE;
	}
	wfsys_unlock();
}
#endif

#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
static int wifiMiscDmaSetup(struct platform_device *pdev,
		struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	u64 dma_mask;
	int ret = 0;

	prChipInfo = prDriverData->chip_info;

	ret = halInitResvMem(pdev, WIFI_RSV_MEM_WIFI_MISC);
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


static int mtk_wifi_misc_probe(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	struct device_node *node = NULL;
	int ret = 0;

	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_wifi_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;

	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi_misc");
	if (!node) {
		DBGLOG(INIT, ERROR,
		       "WIFI-OF: get wifi_misc device node fail\n");
		return false;
	}
	of_node_put(node);

	ret = wifiMiscDmaSetup(pdev, prDriverData);
	if (ret)
		goto exit;

	ret = halAllocHifMemForWiFiMisc(pdev, prDriverData);
	if (ret)
		goto exit;

exit:
	DBGLOG(INIT, INFO, "%s() done, ret: %d\n", __func__, ret);

	return 0;
}

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
static int mtk_wifi_misc_remove(struct platform_device *pdev)
#else
static void mtk_wifi_misc_remove(struct platform_device *pdev)
#endif
{
#if (CFG_MTK_ANDROID_WMT == 1)
	halFreeHifMem(pdev, WIFI_RSV_MEM_WIFI_MISC);
#endif
	platform_set_drvdata(pdev, NULL);
#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
	return 0;
#endif
}
#endif

#if (CFG_CONTROL_ASPM_BY_FW == 1) && (CFG_SUPPORT_PCIE_ASPM == 1)
static void mtk_pci_setup_aspm(struct pci_dev *pdev)
{
	u_int8_t fgKeepL0 = FALSE;

	if (g_prGlueInfo &&
	    g_prGlueInfo->prAdapter &&
	    g_prGlueInfo->prAdapter->rWifiVar.fgPcieEnableL1ss == 0)
		fgKeepL0 = TRUE;

	glBusConfigASPM(pdev, DISABLE_ASPM_L1);
	if (fgKeepL0) {
		DBGLOG(INIT, INFO, "PCIE keep L0\n");
		return;
	}

	glBusConfigASPML1SS(pdev,
		PCI_L1PM_CTR1_ASPM_L12_EN |
		PCI_L1PM_CTR1_ASPM_L11_EN);
	glBusConfigASPM(pdev, ENABLE_ASPM_L1);
	DBGLOG(INIT, INFO, "PCIE allow enter L1.2\n");
}
#endif

static int mtk_pcie_setup_msi(struct pci_dev *pdev,
			      struct mt66xx_chip_info *prChipInfo)
{
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;
	uint32_t u4MaxMsiNum;
	int ret = 0;

	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

#if CFG_MTK_WIFI_PCIE_MSI_SUPPORT
	u4MaxMsiNum = prMsiInfo->u4MaxMsiNum ?
		prMsiInfo->u4MaxMsiNum : 1;
#else
	u4MaxMsiNum = 1;
#endif
#if KERNEL_VERSION(4, 8, 0) <= LINUX_VERSION_CODE
	ret = pci_alloc_irq_vectors(
		pdev, 1, u4MaxMsiNum, PCI_IRQ_MSI);
#endif
	if (ret < 0) {
		DBGLOG(INIT, INFO,
			"pci_alloc_irq_vectors(1, %d) failed, ret=%d\n",
			u4MaxMsiNum,
			ret);
		return ret;
	}

#if CFG_MTK_WIFI_PCIE_MSI_SUPPORT
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

	return 0;
}

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
static int wifiTxCmaSetup(struct platform_device *pdev)
{
#ifdef CONFIG_OF
	uint32_t ret = 0;

	ret = halInitTxCmaMem(pdev);
#else
	DBGLOG(INIT, ERROR, "%s: kernel option CONFIG_OF not enabled.");
#endif

	return ret;
}

static int mtk_wifi_tx_cma_probe(struct platform_device *pdev)
{
	struct device_node *node = NULL;
	int ret = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi_tx_cma");
	if (!node) {
		DBGLOG(INIT, ERROR,
		       "WIFI-OF: get wifi_tx_cma device node fail\n");
		return false;
	}
	of_node_put(node);

	ret = wifiTxCmaSetup(pdev);

	if (ret == 0)
		DBGLOG(INIT, INFO, "%s() done, ret: %d\n", __func__, ret);
	else
		DBGLOG(INIT, INFO, "%s() fail, ret: %d\n", __func__, ret);

	return 0;
}

static int mtk_wifi_tx_cma_remove(struct platform_device *pdev)
{
	halFreeTxCmaMem(pdev);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
static int wifiTxCmaNonCacheSetup(
		struct platform_device *pdev,
		struct mt66xx_hif_driver_data *prDriverData)
{
	struct mt66xx_chip_info *prChipInfo;
	u64 dma_mask;
	int ret = 0;

	prChipInfo = prDriverData->chip_info;

	ret = halInitTxCmaNonCacheMem(pdev);

	dma_mask = DMA_BIT_MASK(prChipInfo->bus_info->u4DmaMask);
	ret = dma_set_mask_and_coherent(&pdev->dev, dma_mask);
	if (ret) {
		DBGLOG(INIT, ERROR, "dma_set_mask_and_coherent failed(%d)\n",
			ret);
		goto exit;
	}

	ret = halInitResvMem(pdev, WIFI_RSV_MEM_WIFI_CMA_NON_CACHE);
	if (ret)
		goto exit;

	ret = of_reserved_mem_device_init_by_idx(&pdev->dev,
		pdev->dev.of_node, 0);

	if (pdev->dev.cma_area == NULL) {
		DBGLOG(INIT, ERROR,
			"Failed to set cma, user Kernel mem dma_coherent: %u\n",
			pdev->dev.dma_coherent);
		goto exit;
	}

exit:
	return ret;
}
static int mtk_wifi_tx_cma_non_cache_probe(
	struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	struct device_node *node = NULL;
	int ret = 0;

	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_wifi_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;

	node = of_find_compatible_node(NULL, NULL,
		"mediatek,wifi_tx_cma");

	if (!node) {
		DBGLOG(INIT, ERROR,
		       "WIFI-OF: get wifi_tx_cma_non_cache device node fail\n");
		return false;
	}
	of_node_put(node);

	ret = wifiTxCmaNonCacheSetup(pdev, prDriverData);
	if (ret)
		goto exit;

exit:
	DBGLOG(INIT, INFO, "%s() done, ret: %d\n", __func__, ret);

	return 0;
}

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
static int mtk_wifi_tx_cma_non_cache_remove(
#else
static void mtk_wifi_tx_cma_non_cache_remove(
#endif
	struct platform_device *pdev)
{
#if (CFG_MTK_ANDROID_WMT == 1)
	halFreeHifMem(pdev, WIFI_RSV_MEM_WIFI_CMA_NON_CACHE);
#endif
	platform_set_drvdata(pdev, NULL);
#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
	return 0;
#endif
}
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */

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
	int ret = 0, i;

	ASSERT(pdev);
	ASSERT(id);

	prDriverData = (struct mt66xx_hif_driver_data *)id->driver_data;
	prChipInfo = prDriverData->chip_info;

	ret = pcim_enable_device(pdev);
	if (ret) {
		DBGLOG(INIT, INFO,
			"pci_enable_device failed, ret=%d\n", ret);
		goto out;
	}

	for (i = 0; i <= PCI_STD_RESOURCE_END; i++) {
		ret = pcim_iomap_regions(pdev, BIT(i), pci_name(pdev));
		if (ret == 0)
			break;
	}
	if (i > PCI_STD_RESOURCE_END) {
		DBGLOG(INIT, INFO,
		       "pcim_iomap_regions failed, ret=%d\n", ret);
		goto out;
	}

	wlanUpdateBusAccessStatus(FALSE);
#if CFG_MTK_WIFI_PCIE_SUPPORT
	fgIsPcieDataTransDisabled = FALSE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	fgTriggerDebugSop = FALSE;
#endif
#if CFG_MTK_WIFI_AER_RESET
	g_AERRstTriggered = FALSE;
	g_AERL05Rst = FALSE;
#endif
#if CFG_MTK_WIFI_PCIE_SR
	fgIsL2Finished = FALSE;
#endif
	pci_set_master(pdev);

	ret = mtk_pcie_setup_msi(pdev, prChipInfo);
	if (ret < 0)
		goto err_free_iomap;

	ret = dma_set_mask(&pdev->dev,
		DMA_BIT_MASK(prChipInfo->bus_info->u4DmaMask));
	if (ret != 0) {
		DBGLOG(INIT, INFO,
			"dma_set_mask failed, ret=%d\n", ret);
		goto err_free_irq_vectors;
	}

	g_prDev = pdev;
	prChipInfo->pdev = (void *)pdev;
	prChipInfo->CSRBaseAddress = pcim_iomap_table(pdev) ?
		pcim_iomap_table(pdev)[i] : NULL;
	prChipInfo->u8CsrOffset = pci_resource_start(pdev, i);

	DBGLOG(INIT, INFO, "ioremap for device %s[%d], region 0x%lX @ 0x%lX\n",
	       pci_name(pdev), i, (unsigned long) pci_resource_len(pdev, i),
	       (unsigned long) pci_resource_start(pdev, i));

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

#if (CFG_CONTROL_ASPM_BY_FW == 1) && (CFG_SUPPORT_PCIE_ASPM == 1)
	mtk_pci_setup_aspm(pdev);
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

	kalDumpPlatGPIOStat();

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
	pcim_iounmap_regions(pdev, BIT(0));
}

static int mtk_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
#if (CFG_DEVICE_SUSPEND_BY_MOBILE == 1)
#if (KERNEL_VERSION(5, 2, 0) <= CFG80211_VERSION_CODE)
	struct device *dev = &pdev->dev;

	dev->power.driver_flags = DPM_FLAG_SMART_SUSPEND;
	dev->power.runtime_status = RPM_SUSPENDED;
	pdev->skip_bus_pm = true;
#endif
	return 0;

#else
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo;
	uint32_t count = 0;
	int wait = 0;
	struct ADAPTER *prAdapter = NULL;
	uint8_t drv_own_fail = FALSE;
	int ret;

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
			ret = -EAGAIN;
			goto SUSPEND_PRESUSPEND_FAIL;
		}
		kalMsleep(2);
		count++;
	}
	DBGLOG(HAL, ERROR, "pcie pre_suspend done\n");

	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_SUSPEND;

	/* Polling until HIF side PDMAs are all idle */
	prBusInfo = prAdapter->chip_info->bus_info;
	if (prBusInfo->pdmaPollingIdle) {
		if (prBusInfo->pdmaPollingIdle(prGlueInfo) != TRUE) {
			ret = -EAGAIN;
			goto SUSPEND_POLL_IDLE_FAIL;
		}
	} else
		DBGLOG(HAL, ERROR, "PDMA polling idle API didn't register\n");

#if CFG_SUPPORT_WED_PROXY
	kalIoctl(prGlueInfo, wlanoidWedSuspend, NULL, 0, &ret);
#endif

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

#if (CFG_MTK_SUPPORT_LIGHT_MDDP == 1)
	if (mddpIsMdDrvOwnAcquired())
		DBGLOG(HAL, STATE, "MD acqurie DrvOwn, Skip Check.\n");
	else
#endif /* CFG_MTK_SUPPORT_LIGHT_MDDP == 1 */
		if (wait >= 500) {
			DBGLOG(HAL, ERROR, "Set FW Own Timeout !!\n");
			ret = -EAGAIN;
			goto SUSPEND_FW_OWN_FAIL;
		}

#if (CFG_SUPPORT_PCIE_ASPM == 1) && (CFG_SUPPORT_ASPM_IN_CE_PCI_SUSPEND == 1)
	DBGLOG(HAL, STATE, "not switch D-state due to ASPM enable!\n");
#else
	pci_save_state(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
#endif

	DBGLOG(HAL, STATE, "mtk_pci_suspend() done!\n");

	/* pending cmd will be kept in queue and no one to handle it after HIF resume.
	 * In STR, it will result in cmd buf full and then cmd buf alloc fail .
	 */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanReleaseAllTxCmdQueue(prGlueInfo->prAdapter);

	return 0;

SUSPEND_FW_OWN_FAIL:
	halEnableInterrupt(prGlueInfo->prAdapter);

	/* Enable HIF side PDMA TX/RX */
	if (prBusInfo->pdmaStop)
		prBusInfo->pdmaStop(prGlueInfo, FALSE);
	else
		DBGLOG(HAL, ERROR, "PDMA config API didn't register\n");

#if CFG_SUPPORT_WED_PROXY
	kalIoctl(prGlueInfo, wlanoidWedResume, NULL, 0, &ret);
#endif

SUSPEND_POLL_IDLE_FAIL:
SUSPEND_PRESUSPEND_FAIL:
	halPcieResumeCmd(prGlueInfo->prAdapter);

	return ret;
#endif
}


int mtk_pci_resume(struct pci_dev *pdev)
{
#if (CFG_DEVICE_SUSPEND_BY_MOBILE == 1)
	struct device *dev = &pdev->dev;

	dev->power.runtime_status = RPM_ACTIVE;
	return 0;
#else
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo;
#if CFG_SUPPORT_WED_PROXY
	uint32_t ret;
#endif

	DBGLOG(HAL, STATE, "mtk_pci_resume()\n");

	prGlueInfo = g_prGlueInfo;
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "prGlueInfo is NULL!\n");
		return -1;
	}

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

#if (CFG_SUPPORT_PCIE_ASPM == 1) && (CFG_SUPPORT_ASPM_IN_CE_PCI_SUSPEND == 1)
	DBGLOG(HAL, STATE, "not switch D-state due to ASPM enable!\n");
#else
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
#endif

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

#if CFG_SUPPORT_WED_PROXY
	kalIoctl(prGlueInfo, wlanoidWedResume, NULL, 0, &ret);
#endif

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
 * \brief This function will register shutdownCB
 *
 * \param[in] pfProbe    Function pointer to remove card when shutdown
 *
 * \return The result of registering pci bus
 */
/*----------------------------------------------------------------------------*/
#if CFG_MTK_ANDROID_WMT && CFG_WIFI_PLAT_SHUTDOWN_SUPPORT
uint32_t glRegisterShutdownCB(remove_card pfShutdown)
{
	int ret = 0;

	ASSERT(pfShutdown);
	pfWlanShutdown = pfShutdown;

	mtk_wifi_driver.shutdown = mtk_wifi_shutdown;
	return ret;
}
#endif

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

	mtk_wifi_driver.probe = mtk_wifi_probe;
	mtk_wifi_driver.remove = mtk_wifi_remove;

	if (platform_driver_register(&mtk_wifi_driver))
		DBGLOG(HAL, ERROR, "platform_driver_register fail\n");

#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
	mtk_wifi_misc_driver.probe = mtk_wifi_misc_probe;
	mtk_wifi_misc_driver.remove = mtk_wifi_misc_remove;

	if (platform_driver_register(&mtk_wifi_misc_driver))
		DBGLOG(HAL, ERROR, "page pool platform_driver_register fail\n");
#endif

#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	mtk_wifi_tx_cma_driver.probe = mtk_wifi_tx_cma_probe;
	mtk_wifi_tx_cma_driver.remove = mtk_wifi_tx_cma_remove;

	if (platform_driver_register(&mtk_wifi_tx_cma_driver))
		DBGLOG(HAL, ERROR,
			"Wi-Fi tx cma platform_driver_register fail\n");
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */

#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	DBGLOG(HAL, ERROR, "CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE\n");
	mtk_wifi_tx_cma_non_cache_driver.probe =
		mtk_wifi_tx_cma_non_cache_probe;
	mtk_wifi_tx_cma_non_cache_driver.remove =
		mtk_wifi_tx_cma_non_cache_remove;

	if (platform_driver_register(&mtk_wifi_tx_cma_non_cache_driver))
		DBGLOG(HAL, ERROR,
			"tx cma non cache platform_driver_register fail\n");
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */

#if (CFG_MTK_WIFI_PCIE_SUPPORT) && (CFG_PRELOADER_PMIC_EN_PU == 1)
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
	platform_driver_unregister(&mtk_wifi_driver);
#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
	platform_driver_unregister(&mtk_wifi_misc_driver);
#endif
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
	platform_driver_unregister(&mtk_wifi_tx_cma_driver);
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	platform_driver_unregister(&mtk_wifi_tx_cma_non_cache_driver);
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
}

static void glPopulateMemOps(struct mt66xx_chip_info *prChipInfo,
			     struct HIF_MEM_OPS *prMemOps)
{
	halSetMemOps(prChipInfo->platform_device, prMemOps);
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
	struct platform_device *pdev;

	prHif = &prGlueInfo->rHifInfo;
	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;
	prMemOps = &prHif->rMemOps;

	prHif->pdev = (struct pci_dev *)ulCookie;
	prHif->prDmaDev = prHif->pdev;

	g_prGlueInfo = prGlueInfo;

	pdev = prChipInfo->platform_device;
	if (pdev)
		SET_NETDEV_DEV(prGlueInfo->prDevHandler, &pdev->dev);
	else
		SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHif->pdev->dev);

	prGlueInfo->u4InfType = MT_DEV_INF_PCIE;

#if (CFG_SUPPORT_RX_ZERO_COPY == 1)
	prHif->u4RxDataRingSize = prBusInfo->rx_data_ring_size;
#else
	/* TODO: Need to check correct value of RxDataRingSize */
	prHif->u4RxDataRingSize = prBusInfo->rx_data_ring_size;
#endif
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


/* same as pci_msi_set_enable function in kernel */
static void glBusSetMsiEnable(struct pci_dev *dev, int enable)
{
	uint16_t u2Control;

	pci_read_config_word(dev, dev->msi_cap + PCI_MSI_FLAGS, &u2Control);
	u2Control &= ~PCI_MSI_FLAGS_ENABLE;
	if (enable)
		u2Control |= PCI_MSI_FLAGS_ENABLE;
	pci_write_config_word(dev, dev->msi_cap + PCI_MSI_FLAGS, u2Control);
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
	if (buf)
		kalMemZero(buf, BUF_SIZE);

	/* During SER L0.5, the wlanOffAtReset function is executed first,
	 * where glBusFreeIrq sets pdev->msi_enabled to 0. Then, wlanOnAtReset
	 * is executed. Since pcie probe is not performed, it is necessary to
	 * additionally execute mtk_pcie_setup_msi to set pdev->msi_enabled back
	 * to 1, in order to prevent the failure of glBusSetMsiIrq.
	 */
	if (pdev->msi_enabled == 0)
		mtk_pcie_setup_msi(pdev, prGlueInfo->prAdapter->chip_info);

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
			IRQF_SHARED | IRQF_NO_SUSPEND,
			KBUILD_MODNAME,
			prGlueInfo);

#if CFG_SUPPORT_WED_PROXY
		if (prMsiLayout->type == AP_INT) {
			struct irq_data *data;
			struct msi_desc *entry;

			data = irq_get_irq_data(irqn);
			if (data) {
				entry = irq_data_get_msi_desc(data);
				if (entry) {
					DBGLOG(INIT, INFO,
					      "messages address [0x%x, 0x%x]\n",
					      entry->msg.address_lo,
					      entry->msg.address_hi);
					prMsiInfo->address_lo =
							entry->msg.address_lo;
					prMsiInfo->address_hi =
							entry->msg.address_hi;
				}
			}
		}
#endif

#if CFG_MTK_MDDP_SUPPORT
		if (prMsiLayout->type == MDDP_INT) {
			struct irq_data *data;

			data = irq_get_irq_data(irqn);
			if (data)
				irq_chip_mask_parent(data);
		}
#endif
#if CFG_MTK_CCCI_SUPPORT
		if (prMsiLayout->type == CCIF_INT) {
			struct irq_data *data;

			data = irq_get_irq_data(irqn);
			if (data) {
				irq_chip_mask_parent(data);
#if CFG_MTK_WIFI_PCIE_SUPPORT
				mtk_msi_unmask_to_other_mcu(data, 1);
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
			}
		}
#endif

		en_wake_ret = enable_irq_wake(irqn);
		if (buf && ((BUF_SIZE - pos) > 0)) {
			pos += kalSnprintf(buf + pos, BUF_SIZE - pos,
				"irqn:%d, ret:%d, en_wake_ret:%d%s",
				irqn, ret, en_wake_ret,
				i == prMsiInfo->u4MsiNum - 1 ? "\n" : "; ");
		} else {
			DBGLOG(INIT, INFO, "request_irq(%d %s %d %d)\n",
				irqn, prMsiLayout->name, ret, en_wake_ret);
		}
		if (ret)
			goto err;
	}

	if (buf) {
		DBGLOG(HAL, INFO, "request_irq info: %s", buf);
		kalMemFree(buf, VIR_MEM_TYPE, BUF_SIZE);
	}

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

#if CFG_MTK_WIFI_PCIE_SUPPORT
	mtk_pcie_dump_link_info(0);
#endif

	return ret;
#else
	return -EOPNOTSUPP;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a WIFI interrupt callback function
 *
 * \param[in] func  pointer to WIFI handle
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
static irqreturn_t mtk_wifi_isr(int irq, void *dev_instance)
{
	struct GL_HIF_INFO *prHifInfo = (struct GL_HIF_INFO *)dev_instance;

	if (!prHifInfo)
		return IRQ_NONE;

	if (KAL_TEST_BIT(HIF_MAWD_INT_BIT, prHifInfo->ulHifIntEnBits))
		return IRQ_NONE;

	disable_irq_nosync(irq);
	KAL_SET_BIT(HIF_MAWD_INT_BIT, prHifInfo->ulHifIntEnBits);

	return IRQ_WAKE_THREAD;
}

static irqreturn_t mtk_wifi_isr_thread(int irq, void *dev_instance)
{
	struct GL_HIF_INFO *prHifInfo = (struct GL_HIF_INFO *)dev_instance;
	struct GLUE_INFO *prGlueInfo;

	if (!prHifInfo)
		return IRQ_NONE;

	prGlueInfo = CONTAINER_OF(prHifInfo, struct GLUE_INFO, rHifInfo);
	prGlueInfo = get_glue_info_isr((void *)prGlueInfo, irq, -1);
	if (!prGlueInfo)
		return IRQ_NONE;

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
	struct mt66xx_chip_info *prChipInfo;
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

	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;
	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

	prHifInfo->u4IrqId = pdev->irq;

#if CFG_SUPPORT_WED_PROXY
	prHifInfo->irq_handler = mtk_pci_isr;
	prHifInfo->irq_handler_thread = mtk_pci_isr_thread;
#endif

	if (prMsiInfo && prMsiInfo->fgMsiEnabled)
		ret = glBusSetMsiIrq(pdev, prGlueInfo, prBusInfo);
	else
		ret = glBusSetLegacyIrq(pdev, prGlueInfo, prBusInfo);

	if (ret)
		goto exit;

	if (prBusInfo->initPcieInt)
		prBusInfo->initPcieInt(prGlueInfo);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	setupPlatDevIrq(prChipInfo->platform_device, prGlueInfo,
			&prHifInfo->u4IrqId_1);
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
				       "[%d] %u, ",
				       irqn,
				       (uint32_t)KAL_GET_TIME_INTERVAL());
	}
	KAL_REC_TIME_END();

	DBGLOG(INIT, INFO,
		"Total: %llu us, %s\n",
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
	struct mt66xx_chip_info *prChipInfo;
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
	prChipInfo = prGlueInfo->prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;
	pdev = prHifInfo->pdev;

	glBusSetMsiEnable(pdev, 0);

	if (prMsiInfo->fgMsiEnabled)
		glBusFreeMsiIrq(pdev, prGlueInfo, prBusInfo);
	else
		glBusFreeLegacyIrq(pdev, prGlueInfo, prBusInfo);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	freePlatDevIrq(prChipInfo->platform_device, prGlueInfo,
			prHifInfo->u4IrqId_1);
	prHifInfo->u4IrqId_1 = 0;
#endif /* CFG_SUPPORT_HOST_OFFLOAD */

#if CFG_SUPPORT_WED_PROXY
	if (IsWedAttached())
		return;
#endif

#if KERNEL_VERSION(4, 8, 0) <= CFG80211_VERSION_CODE
	pci_free_irq_vectors(pdev);
#endif

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

	if (!pos) {
		DBGLOG(INIT, INFO, "L1 PM Substate capability is not found!\n");
		return;
	}

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
		if ((i4BitMap & PCI_L1PM_CAP_ASPM_L12) &&
				(!(u4Reg & PCI_L1PM_CAP_ASPM_L12))) {
			DBGLOG(INIT, INFO, "not support ASPM L1.2!\n");
			return FALSE;
		}
		if ((i4BitMap & PCI_L1PM_CAP_ASPM_L11) &&
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
			DBGLOG(INIT, TRACE, "ASPM STATUS %d\n", i4Enable);
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
			DBGLOG(INIT, TRACE, "Config ASPM-L1SS\n");
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
#if CFG_MTK_WIFI_PCIE_SUPPORT
	uint8_t voteResult = TRUE;
	int32_t u4VoteState = 0;
	int32_t err = 0;

	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "adapter null\n");
		return;
	}

	if (u4WifiUser >= PCIE_VOTE_USER_NUM) {
		DBGLOG(HAL, ERROR,
			"vote user undefined[%d]\n", u4WifiUser);
		return;
	}

	spin_lock_bh(
		&prAdapter->prGlueInfo->rSpinLock[SPIN_LOCK_PCIE_VOTE]);

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
		"enable[%d], user[%d], Vote state[0x%08X]\n",
		enable, u4WifiUser,
		prAdapter->prGlueInfo->rHifInfo.u4VoteState);

	u4VoteState = prAdapter->prGlueInfo->rHifInfo.u4VoteState;

	/* if all bits are 0's, means all vote to enable hw ctrl */
	if (!u4VoteState)
		voteResult = TRUE;
	else
		voteResult = FALSE;

	/* vote to enable/disable hw mode */
	err = mtk_pcie_hw_control_vote(0, voteResult, 1);
	if (err) {
		DBGLOG(HAL, ERROR,
			"hw control mode err[%d]\n", err);
		wlanUpdateBusAccessStatus(TRUE);
		GL_DEFAULT_RESET_TRIGGER(prAdapter,
			RST_PCIE_NOT_READY);
	}

	spin_unlock_bh(
		&prAdapter->prGlueInfo->rSpinLock[SPIN_LOCK_PCIE_VOTE]);
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
}

int32_t glBusFuncOn(void)
{
	int ret = 0;

#if CFG_MTK_WIFI_PCIE_SUPPORT
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

#if (CFG_PCIE_MT6989_6653 == 1)
	/* Notify RC to intercept CmpltTO during wlan probe */
	mtk_pcie_set_aer_detect(0, TRUE);
#endif
	ret = pci_register_driver(&mtk_pci_driver);
#if (CFG_PCIE_MT6989_6653 == 1)
	mtk_pcie_set_aer_detect(0, FALSE);
#endif
	if (ret == -EBUSY) {
		if (g_fgDriverProbed) {
			WARN_ON_ONCE(TRUE);
			ret = 0;
		} else
			goto exit_dump;
	} else if (ret) {
		DBGLOG(HAL, ERROR, "pci_register_driver failed, ret=%d\n",
			ret);
#if CFG_MTK_WIFI_PCIE_SUPPORT
		mtk_pcie_remove_port(0);
#endif
		return ret;
	}

	if (g_fgDriverProbed == FALSE) {
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
		struct mt66xx_chip_info *prChipInfo = NULL;
		struct CHIP_DBG_OPS *prDbgOps = NULL;

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
#endif
exit_dump:
		pci_unregister_driver(&mtk_pci_driver);
#if CFG_MTK_WIFI_PCIE_SUPPORT
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
#if CFG_MTK_WIFI_PCIE_SUPPORT
	mtk_pcie_remove_port(0);
#endif
}

#if (CFG_PCIE_GEN_SWITCH == 1)
void pcie_check_gen_switch_timeout(struct ADAPTER *prAdapter, uint32_t u4Reg)
{
	uint32_t u4Val = 0;
	struct RX_IDLE_STATE *prRxIdleState;

	if (prAdapter) {
		if (prAdapter->ucStopMMIO) {
			DBGLOG(INIT, ERROR,
			       "[Gen Switch] check start. reg[0x%08x]\n",
			       u4Reg);
			prRxIdleState = (struct RX_IDLE_STATE *)
				pcie_gen_switch_get_emi_add(prAdapter);

			if (prRxIdleState == NULL) {
				DBGLOG(OID, ERROR, "g_pu4RxDone is null\n");
				return;
			}
			while (prAdapter->ucStopMMIO) {
				udelay(1);
				u4Val++;
				if (u4Val > GEN_SWITCH_TIMEOUT) {
					prAdapter->ucStopMMIO = FALSE;
					prRxIdleState->u4FWIdle = DEFAULT_IDLE;
					prRxIdleState->u4WFIdle = DEFAULT_IDLE;
#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
					mtk_pcie_disable_cfg_dump(0);
#endif
					DBGLOG(INIT, ERROR,
						"[Gen Switch] timeout\n");
					break;
				}
			}
			DBGLOG(INIT, ERROR, "[Gen Switch] check timeout end\n");
		}
	}
}
#endif /*CFG_PCIE_GEN_SWITCH */

uint32_t glReadPcieCfgSpace(int offset, uint32_t *value)
{
	int ret = 0;
#if (CFG_PCIE_GEN_SWITCH == 1)
	struct ADAPTER *prAdapter = NULL;

	if (g_prGlueInfo) {
		prAdapter = g_prGlueInfo->prAdapter;
		if (prAdapter)
			pcie_check_gen_switch_timeout(prAdapter, offset);
	}
#endif /*CFG_PCIE_GEN_SWITCH*/

	ret = pci_read_config_dword(g_prDev, offset, value);
	if (unlikely(ret))
		DBGLOG(HAL, ERROR,
			"pci_read_config_dword() failed, ret=%d offset=0x%08x\n",
			ret, offset);
	else
		DBGLOG(HAL, LOUD, "Read 0x%08x=[0x%08x]\n", offset, *value);

	return ret == 0 ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

uint32_t glWritePcieCfgSpace(int offset, uint32_t value)
{
	int ret = 0;

	ret = pci_write_config_dword(g_prDev, offset, value);
	if (unlikely(ret))
		DBGLOG(HAL, ERROR,
			"pci_write_config_dword() failed, ret=%d offset=0x%08x\n",
			ret, offset);
	else
		DBGLOG(HAL, LOUD, "Write 0x%08x=[0x%08x]\n", offset, value);

	return ret == 0 ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;
}

void glNotifyPciePowerDown(void)
{
#if defined(CFG_MTK_WIFI_PCIE_SUPPORT) && CFG_MTK_ANDROID_WMT
	DBGLOG(HAL, INFO, "notify PCIE PD\n");
#if KERNEL_VERSION(6, 6, 0) <= CFG80211_VERSION_CODE
	mtk_pcie_pinmux_select(0, PCIE_PINMUX_PD);
#endif
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
		return -1;
	}
	pci_read_config_word(parent, pos + PCI_EXP_LNKSTA, &linksta);

	if ((linksta & PCI_EXP_LNKSTA_CLS) != speed) {
		DBGLOG(HAL, ERROR, "can't train status = 0x%x\n", linksta);
		return -1;
	}

	return 1;
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

#if (CFG_PCIE_GEN_SWITCH == 1)
void pcie_gen_switch_recover(struct ADAPTER *prAdapter)
{
#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
	mtk_pcie_disable_cfg_dump(0);
#endif
	if (prAdapter)
		prAdapter->ucStopMMIO = FALSE;

	DBGLOG(OID, ERROR, "[Gen_Switch] gen switch recover\n");
}
uint32_t *pcie_gen_switch_get_emi_add(struct ADAPTER *prAdapter)
{
	struct HIF_MEM *prMem = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct HIF_MEM_OPS *prMemOps = NULL;
	uint32_t *pu4RxDone = NULL;

	prGlueInfo = prAdapter->prGlueInfo;
	prHifInfo = &prGlueInfo->rHifInfo;
	prChipInfo = prAdapter->chip_info;
	prMemOps = &prHifInfo->rMemOps;
	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(
			prChipInfo, WIFI_MISC_MEM_BLOCK_WF_M_BRAIN);
		if (prMem && prMem->va)
			pu4RxDone = (uint32_t *)prMem->va;
	}

	return pu4RxDone;

}
void pcie_gen_switch_polling_rx_done(struct ADAPTER *prAdapter)
{
	struct pcie_msi_info *prMsiInfo;
	uint32_t u4Val = 0;
	struct RX_IDLE_STATE *prRxIdleState;

	prMsiInfo = &prAdapter->chip_info->bus_info->pcie_msi_info;

	DBGLOG(OID, INFO,
		"[Gen_Switch] check rx idle start\n");

	prRxIdleState =
		(struct RX_IDLE_STATE *)pcie_gen_switch_get_emi_add(prAdapter);
	if (prRxIdleState == NULL) {
		DBGLOG(OID, ERROR, "[Gen_Switch] g_pu4RxDone is null\n");
		return;
	}

	while (!mtk_pci_is_int_ready(prAdapter->prGlueInfo) &&
		prRxIdleState->u4WFIdle != WF_RX_IDLE) {
		udelay(1);
		u4Val++;
		if (u4Val > CHECK_RX_TIMEOUT) {
			DBGLOG(OID, ERROR, "[Gen_Switch] check timeout\n");
			break;
		}
	}

	DBGLOG(OID, INFO, "[Gen_Switch] check rx idle end u4WFIdle=%d\n",
		prRxIdleState->u4WFIdle);

}
irqreturn_t pcie_gen_switch_top_handler(int irq, void *dev_instance)
{
	return IRQ_WAKE_THREAD;
}
irqreturn_t pcie_gen_switch_thread_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct RX_IDLE_STATE *prRxIdleState;


	prGlueInfo = (struct GLUE_INFO *)dev_instance;

	if (prGlueInfo) {
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return IRQ_HANDLED;
		}
	} else {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return IRQ_HANDLED;
	}
	prAdapter->fgIsGenSwitchProcessing = TRUE;

	pcie_gen_switch_polling_rx_done(prAdapter);
#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
	mtk_pcie_enable_cfg_dump(0);
#endif

	DBGLOG(HAL, TRACE, "[Gen_Switch] start\n");
	if (g_ucBypassException) {
		DBGLOG(INIT, ERROR, "[Gen_Switch] g_u1BypassException\n");
		g_ucBypassException = FALSE;
		return IRQ_HANDLED;
	}

	prAdapter->ucStopMMIO = TRUE;
	g_ucReceiveGenSwitch = TRUE;

	DBGLOG(INIT, ERROR, "[Gen_Switch] u1StopMMIO:%u, isProcessing:%u\n",
		prAdapter->ucStopMMIO, prAdapter->fgIsGenSwitchProcessing);

	prRxIdleState =
		(struct RX_IDLE_STATE *)pcie_gen_switch_get_emi_add(prAdapter);

	if (prRxIdleState == NULL) {
		DBGLOG(OID, ERROR, "[Gen_Switch] g_pu4RxDone is null\n");
		return IRQ_HANDLED;
	}

	prRxIdleState->u4FWIdle = FW_RX_IDLE;

	return IRQ_HANDLED;
}

irqreturn_t pcie_gen_switch_end_top_handler(int irq, void *dev_instance)
{
		return IRQ_WAKE_THREAD;
}
irqreturn_t pcie_gen_switch_end_thread_handler(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	struct RX_IDLE_STATE *prRxIdleState;

	DBGLOG(HAL, TRACE, "[Gen_Switch] end\n");
#if (CFG_MTK_WIFI_PCIE_CONFIG_SPACE_ACCESS_DBG == 1)
	mtk_pcie_disable_cfg_dump(0);
#endif

	prGlueInfo = (struct GLUE_INFO *)dev_instance;

	if (prGlueInfo) {
		prAdapter = prGlueInfo->prAdapter;
		if (!prAdapter) {
			DBGLOG(INIT, ERROR, "prAdapter is NULL.\n");
			return IRQ_HANDLED;
		}
	} else {
		DBGLOG(INIT, ERROR, "prGlueInfo is NULL.\n");
		return IRQ_HANDLED;
	}

	if (g_ucReceiveGenSwitch) {
		g_ucReceiveGenSwitch = FALSE;
	} else {
		g_ucBypassException = TRUE;
		DBGLOG(INIT, ERROR, "[Gen_Switch] g_ucBypassException\n");
	}
	prAdapter->fgIsGenSwitchProcessing = FALSE;
	prAdapter->ucStopMMIO = FALSE;
	DBGLOG(INIT, ERROR, "[Gen_Switch] u1StopMMIO:%u, isProcessing:%u\n",
		prAdapter->ucStopMMIO, prAdapter->fgIsGenSwitchProcessing);

#if CFG_MTK_MDDP_SUPPORT
	mddpNotifyMDGenSwitchEnd(prAdapter);
#endif
	kalSetHifMsiRecoveryEvent(prGlueInfo);

	prRxIdleState =
		(struct RX_IDLE_STATE *)pcie_gen_switch_get_emi_add(prAdapter);

	if (prRxIdleState == NULL) {
		DBGLOG(OID, ERROR, "[Gen_Switch] prRxIdleState is null\n");
		return IRQ_HANDLED;
	}

	prRxIdleState->u4WFIdle = DEFAULT_IDLE;
	prRxIdleState->u4FWIdle = DEFAULT_IDLE;
	return IRQ_HANDLED;
}
#endif

uint8_t halPcieIsPcieProbed(void)
{
	return g_fgDriverProbed;
}

#if CFG_MTK_WIFI_PCIE_SR
int mtk_pcie_enter_L2(struct pci_dev *pdev)
{
	int state = 0;

	if (pdev == NULL)
		return -1;

	pci_save_state(pdev);
	state = mtk_pcie_soft_off(pdev->bus);
	DBGLOG(HAL, LOUD, "done\n");
	return state;
}

int mtk_pcie_exit_L2(struct pci_dev *pdev)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo;
	int state = 0;

	if (pdev == NULL)
		return -1;

	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;

	state = mtk_pcie_soft_on(pdev->bus);
	if (state)
		goto error_return;

	if (!pcie_check_status_is_linked())
		goto error_return;

	pci_restore_state(pdev);

	if (g_prGlueInfo && prBusInfo->initPcieInt)
		prBusInfo->initPcieInt(g_prGlueInfo);

	DBGLOG(HAL, LOUD, "done\n");
	return state;
error_return:
	wlanUpdateBusAccessStatus(TRUE);
#if CFG_MTK_WIFI_PCIE_SUPPORT
	mtk_pcie_dump_link_info(0);
#endif
	return -1;
}
#endif /* CFG_MTK_WIFI_PCIE_SR */
