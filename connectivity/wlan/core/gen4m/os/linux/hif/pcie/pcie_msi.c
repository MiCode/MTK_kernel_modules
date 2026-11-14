// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2022 MediaTek Inc.
 */
/*! \file   pcie_msi.c
*/

#include "gl_os.h"

#include "hif_pdma.h"

#include "precomp.h"

#include <linux/mm.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif

#include <linux/irq.h>

u_int8_t mtk_is_wfdma_ready(struct GLUE_INFO *prGlueInfo, uint8_t ucRingNum)
{
	struct WIFI_VAR *prWifiVar;

	if (!prGlueInfo->prAdapter)
		return FALSE;

	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;
	if (!IS_FEATURE_ENABLED(prWifiVar->fgEnWfdmaNoMmioRead))
		return TRUE;
#if CFG_SUPPORT_WED_PROXY
	if (IsWedAttached() == TRUE)
		return TRUE;
#endif

	return halIsWfdmaRxRingReady(prGlueInfo, ucRingNum);
}

irqreturn_t mtk_pci_isr_tx_data0_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = get_glue_info_isr(
		dev_instance, irq, PCIE_MSI_TX_DATA_BAND0);
	if (!prGlueInfo)
		return IRQ_NONE;

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_tx_data1_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = get_glue_info_isr(
		dev_instance, irq, PCIE_MSI_TX_DATA_BAND1);
	if (!prGlueInfo)
		return IRQ_NONE;

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_tx_free_done_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;

	prGlueInfo = get_glue_info_isr(
		dev_instance, irq, PCIE_MSI_TX_FREE_DONE);
	if (!prGlueInfo)
		return IRQ_NONE;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!mtk_is_wfdma_ready(prGlueInfo, RX_RING_TXDONE0)) {
		if (KAL_TEST_AND_CLEAR_BIT(
			    PCIE_MSI_TX_FREE_DONE, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_enable_irq(
				prGlueInfo, irq, PCIE_MSI_TX_FREE_DONE);
			GLUE_INC_REF_CNT(
				prGlueInfo->prAdapter->rHifStats.u4EnIrqCount);
		}
		return IRQ_HANDLED;
	}

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_rx_data0_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;

	prGlueInfo = get_glue_info_isr(
		dev_instance, irq, PCIE_MSI_RX_DATA_BAND0);
	if (!prGlueInfo)
		return IRQ_NONE;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!mtk_is_wfdma_ready(prGlueInfo, RX_RING_DATA0)) {
		if (KAL_TEST_AND_CLEAR_BIT(
			    PCIE_MSI_RX_DATA_BAND0, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_enable_irq(
				prGlueInfo, irq, PCIE_MSI_RX_DATA_BAND0);
			GLUE_INC_REF_CNT(
				prGlueInfo->prAdapter->rHifStats.u4EnIrqCount);
		}
		return IRQ_HANDLED;
	}

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_rx_data1_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;

	prGlueInfo = get_glue_info_isr(
		dev_instance, irq, PCIE_MSI_RX_DATA_BAND1);
	if (!prGlueInfo)
		return IRQ_NONE;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!mtk_is_wfdma_ready(prGlueInfo, RX_RING_DATA1)) {
		if (KAL_TEST_AND_CLEAR_BIT(
			    PCIE_MSI_RX_DATA_BAND1, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_enable_irq(
				prGlueInfo, irq, PCIE_MSI_RX_DATA_BAND1);
			GLUE_INC_REF_CNT(
				prGlueInfo->prAdapter->rHifStats.u4EnIrqCount);
		}
		return IRQ_HANDLED;
	}

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_rx_event_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;
	struct BUS_INFO *prBusInfo;
	struct pcie_msi_info *prMsiInfo;

	prGlueInfo = get_glue_info_isr(dev_instance, irq, PCIE_MSI_EVENT);
	if (!prGlueInfo)
		return IRQ_NONE;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prMsiInfo = &prBusInfo->pcie_msi_info;

	if (!mtk_is_wfdma_ready(prGlueInfo, RX_RING_EVT)) {
		if (KAL_TEST_AND_CLEAR_BIT(
			    PCIE_MSI_EVENT, prMsiInfo->ulEnBits)) {
			mtk_pci_msi_enable_irq(
				prGlueInfo, irq, PCIE_MSI_EVENT);
			GLUE_INC_REF_CNT(
				prGlueInfo->prAdapter->rHifStats.u4EnIrqCount);
		}
		return IRQ_HANDLED;
	}

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_tx_cmd_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = get_glue_info_isr(dev_instance, irq, PCIE_MSI_CMD);
	if (!prGlueInfo)
		return IRQ_NONE;

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}

irqreturn_t mtk_pci_isr_lump_thread(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo;

	prGlueInfo = get_glue_info_isr(dev_instance, irq, PCIE_MSI_LUMP);
	if (!prGlueInfo)
		return IRQ_NONE;

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
}
