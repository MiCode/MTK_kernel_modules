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

#include "mt66xx_reg.h"
#include <linux/irq.h>
#include <linux/platform_device.h>

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
#define NIC7933_PCIe_DEVICE_ID	0x0789
#define NIC7922_PCIe_DEVICE_ID	0x7922
#define NICSOC5_0_PCIe_DEVICE_ID  0x0789
#define NIC7902_PCIe_DEVICE_ID	0x7902
#define NIC7926_PCIe_DEVICE_ID	0x7926
#define NIC6639_PCIe_DEVICE_ID 0x3107

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
#ifdef CONNAC2X2
	{	PCI_DEVICE(CONNAC_PCI_VENDOR_ID, CONNAC_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_connac2x2},
#endif /* CONNAC */
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
#ifdef MT7922
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7922_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7922},
#endif /* MT7922 */
#ifdef MT7902
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7902_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7902},
#endif /* MT7902 */
#ifdef MT7926
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7926_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7926},
#endif /* MT7926 */
#ifdef MT7933
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC7933_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7933},
#endif /* MT7933 */
#ifdef SOC5_0
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NICSOC5_0_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc5_0},
#endif /* SOC5_0 */
#ifdef MT6639
	{	PCI_DEVICE(MTK_PCI_VENDOR_ID, NIC6639_PCIe_DEVICE_ID),
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt6639},
#endif /* MT6639 */
	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(pci, mtk_pci_ids);


#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
static const struct platform_device_id mtk_wifi_ids[] = {
	{	.name = "CONNAC",
#if defined(MT7961)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7961,
#elif defined(MT7902)
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_mt7902,
#endif
	},

	{ /* end: all zeroes */ },
};

#ifdef CONFIG_OF
const struct of_device_id mtk_wifi_of_ids[] = {
#ifndef CFG_MTK_WIFI_DTS_COMPATIBLE
	{.compatible = "mediatek,wifi",},
#else
	{.compatible = CFG_MTK_WIFI_DTS_COMPATIBLE,},
#endif
	{}
};
#endif  /* CONFIG_OF */
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */

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

#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
static struct platform_driver mtk_wifi_driver = {
	.driver = {
		.name = "wlan_rsvmem",
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
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */

static struct pci_driver mtk_pci_driver = {
	.name = "wlan",
	.id_table = mtk_pci_ids,
	.probe = NULL,
	.remove = NULL,
};

static u_int8_t g_fgDriverProbed = FALSE;
static uint32_t g_u4DmaMask = 32;
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
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

	ret = wifiDmaSetup(pdev, prDriverData);
	if (ret) {
		DBGLOG(INIT, ERROR, "wifiDmaSetup fail, ret: %d\n", ret);
		goto exit;
	}

	ret = halAllocHifMem(pdev, prDriverData);
	if (ret)
		DBGLOG(INIT, ERROR, "halAllocHifMem fail, ret: %d\n", ret);

exit:
	DBGLOG(INIT, INFO, "mtk_wifi_probe() done, ret: %d\n", ret);
	return ret;
}

static int mtk_wifi_remove(struct platform_device *pdev)
{
	halFreeHifMem(pdev, WIFI_RSV_MEM_WFDMA);
	platform_set_drvdata(pdev, NULL);
	return 0;
}
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a PCIE interrupt callback function
 *
 * \param[in] func  pointer to PCIE handle
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
#if CFG_SUPPORT_MULTI_CARD
static void *CSRBaseAddress[CFG_MAX_WLAN_DEVICES];
#else
static void *CSRBaseAddress;
#endif
#if defined(CONFIG_SMP)
static int mtk_pci_irq_set_smp_affinity(struct pci_dev *pdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct ADAPTER *prAdapter = NULL;
	uint32_t ucAffinity = 0;
	struct cpumask *onlinemask = NULL;
	struct cpumask *mask = NULL;
	int ret = -1;

	prGlueInfo = (struct GLUE_INFO *)pci_get_drvdata(pdev);
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "pci_get_drvdata fail!\n");
		goto out;
	}

	prAdapter = prGlueInfo->prAdapter;
	if (!prAdapter) {
		DBGLOG(HAL, ERROR, "prAdapter is NULL!\n");
		goto out;
	}

	ucAffinity = prAdapter->rWifiVar.u4PciIrqSMPAffinity;
	if (!ucAffinity) {
		DBGLOG(HAL, INFO, "No need set affinity\n");
		goto out;
	}

	onlinemask = kalMemAlloc(sizeof(struct cpumask), VIR_MEM_TYPE);
	if (onlinemask == NULL) {
		DBGLOG(HAL, INFO, "No need set affinity\n");
		goto out;
	}
	kalMemZero(onlinemask, sizeof(struct cpumask));

	mask = kalMemAlloc(sizeof(struct cpumask), VIR_MEM_TYPE);
	if (mask == NULL) {
		DBGLOG(HAL, INFO, "No need set affinity\n");
		goto out;
	}
	kalMemZero(mask, sizeof(struct cpumask));

	cpumask_copy(onlinemask, cpu_online_mask);


	if (ucAffinity & onlinemask->bits[0]) {
		mask->bits[0] = ucAffinity & onlinemask->bits[0];
	} else {
		DBGLOG(HAL, ERROR, "The affinity is out of cpu online mask!\n");
		mask->bits[0] = onlinemask->bits[0] & 0xfe;
	}
#if KERNEL_VERSION(5, 4, 0) <= CFG80211_VERSION_CODE
	ret = irq_set_affinity_hint(pdev->irq, mask);

	if (ret) {
		DBGLOG(HAL, INFO, "irq_set_affinity_hint failed(%d)\n", ret);
		goto out;
	}
#endif


	ret = 0;

out:
	if (onlinemask)
		kalMemFree(onlinemask, VIR_MEM_TYPE, sizeof(struct cpumask));

	if (mask)
		kalMemFree(mask, VIR_MEM_TYPE, sizeof(struct cpumask));

	return ret;


}
#endif

static irqreturn_t mtk_pci_interrupt(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;

	prGlueInfo = (struct GLUE_INFO *) dev_instance;
	if (!prGlueInfo) {
		DBGLOG(HAL, INFO, "No glue info in mtk_pci_interrupt()\n");
		return IRQ_NONE;
	}

	halDisableInterrupt(prGlueInfo->prAdapter);

	if (prGlueInfo->ulFlag & GLUE_FLAG_HALT) {
		DBGLOG(HAL, INFO, "GLUE_FLAG_HALT skip INT\n");
		return IRQ_NONE;
	}

	kalSetIntEvent(prGlueInfo);

	return IRQ_HANDLED;
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
	int ret = 0;
	struct mt66xx_chip_info *prChipInfo;
#if defined(CONFIG_SMP)
	int retSetIrq = 0;
#endif

	ASSERT(pdev);
	ASSERT(id);

	ret = pci_enable_device(pdev);

	if (ret) {
		DBGLOG(INIT, INFO, "pci_enable_device failed!\n");
		goto out;
	}

#if defined(SOC3_0)
	if ((void *)&mt66xx_driver_data_soc3_0 == (void *)id->driver_data)
		DBGLOG(INIT, INFO,
			"[MJ]&mt66xx_driver_data_soc3_0 == id->driver_data\n");
#endif

	DBGLOG(INIT, INFO, "pci_enable_device done!\n");

	prChipInfo = ((struct mt66xx_hif_driver_data *)
				id->driver_data)->chip_info;
	prChipInfo->pdev = (void *)pdev;

#if (CFG_POWER_ON_DOWNLOAD_EMI_ROM_PATCH == 1)
		g_fgDriverProbed = TRUE;
		g_u4DmaMask = prChipInfo->bus_info->u4DmaMask;
#else
	if (pfWlanProbe((void *) pdev,
		(void *) id->driver_data) != WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, INFO, "pfWlanProbe fail!call pfWlanRemove()\n");
		pfWlanRemove((void *) pdev);
		ret = -1;
	} else {
		g_fgDriverProbed = TRUE;
		g_u4DmaMask = prChipInfo->bus_info->u4DmaMask;
#if defined(CONFIG_SMP)
		retSetIrq = mtk_pci_irq_set_smp_affinity(pdev);
		if (retSetIrq)
			DBGLOG(INIT, INFO,
				"mtk_pci_irq_set_smp_affinity failed\n");
#endif
	}
#endif

out:
	DBGLOG(INIT, INFO, "mtk_pci_probe() done(%d)\n", ret);

	return ret;
}

static void mtk_pci_remove(struct pci_dev *pdev)
{
#if CFG_SUPPORT_MULTI_CARD
	uint32_t u4Idx, u4CSRIdx;
	struct GLUE_INFO *prGlueInfo = NULL;
	void *prCSRBaseAddress       = NULL;
	struct device *prDev         = NULL;
#endif

	ASSERT(pdev);

#if CFG_SUPPORT_MULTI_CARD
	glGetDev(pdev, &prDev);

	u4CSRIdx = wlanSearchDevIdx(prDev);

	if (u4CSRIdx < CFG_MAX_WLAN_DEVICES) {
		prGlueInfo = *((struct GLUE_INFO **)
			netdev_priv(arWlanDevInfo[u4CSRIdx].prDev));
		prCSRBaseAddress = prGlueInfo->rHifInfo.CSRBaseAddress;
	}
#endif

	if (g_fgDriverProbed)
		pfWlanRemove((void *) pdev);
	DBGLOG(INIT, INFO, "pfWlanRemove done\n");

	/* Unmap CSR base address */
#if CFG_SUPPORT_MULTI_CARD
	if (prCSRBaseAddress)
		iounmap(CSRBaseAddress[u4CSRIdx]);
	else {
		for (u4Idx = 0; u4Idx < CFG_MAX_WLAN_DEVICES; u4Idx++)
			iounmap(CSRBaseAddress[u4Idx]);
	}
#else
	iounmap(CSRBaseAddress);
#endif

	/* release memory region */
	pci_release_regions(pdev);

	pci_disable_device(pdev);
#if CFG_SUPPORT_MULTI_CARD
	if (prCSRBaseAddress)
		CSRBaseAddress[u4CSRIdx] = NULL;
	else
		for (u4Idx = 0; u4Idx < CFG_MAX_WLAN_DEVICES; u4Idx++)
			CSRBaseAddress[u4Idx] = NULL;
#endif
	DBGLOG(INIT, INFO, "mtk_pci_remove() done\n");
}

static int mtk_pci_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo;
	uint32_t count = 0;
	int wait = 0;
	struct ADAPTER *prAdapter = NULL;
	uint8_t drv_own_fail = FALSE;
	struct ERR_RECOVERY_CTRL_T *prErrRecoveryCtrl;
	uint32_t u4Status = 0;
	uint32_t u4Tick;
#if CFG_SUPPORT_MULTITHREAD
	struct task_struct *prHifThread = NULL;
#endif

	DBGLOG(HAL, STATE, "mtk_pci_suspend()\n");

	prGlueInfo = (struct GLUE_INFO *)pci_get_drvdata(pdev);
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "pci_get_drvdata fail!\n");
		return -1;
	}

	prAdapter = prGlueInfo->prAdapter;
	prErrRecoveryCtrl = &prGlueInfo->rHifInfo.rErrRecoveryCtl;
#if CFG_SUPPORT_MULTITHREAD
	prHifThread = prGlueInfo->hif_thread;
#endif

	ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);

	/* Stop upper layers calling the device hard_start_xmit routine. */
	netif_tx_stop_all_queues(prGlueInfo->prDevHandler);

#if CFG_ENABLE_WAKE_LOCK
	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_SUSPEND_ENTERING;
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow)) {
		DBGLOG(HAL, STATE, ">> compile flag CFG_ENABLE_WAKE_LOCK\n");
		aisPreSuspendFlow(prGlueInfo->prAdapter);
		p2pRoleProcessPreSuspendFlow(prGlueInfo->prAdapter);
	}
#else
	/* wait wiphy device do cfg80211 suspend done, then start hif suspend */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanWaitCfg80211SuspendDone(prGlueInfo);
#endif

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

	/* Make sure any pending or on-going L1 reset is done before HIF
	 * suspend.
	 */
	u4Tick = kalGetTimeTick();
	while (1) {
		u4Status = prBusInfo->getSoftwareInterrupt ?
			prBusInfo->getSoftwareInterrupt(prAdapter) : 0;
		if (((u4Status & ERROR_DETECT_MASK) == 0) &&
		   (prErrRecoveryCtrl->eErrRecovState == ERR_RECOV_STOP_IDLE)) {
			DBGLOG(HAL, INFO, "[SER][L1] no pending L1 reset\n");
			break;
		}

		kalMsleep(40);

		if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Tick,
			       MSEC_TO_SYSTIME(WIFI_SER_L1_RST_DONE_TIMEOUT))) {

			DBGLOG(HAL, ERROR,
			       "[SER][L1] reset timeout before suspend\n");
			break;
		}
	}

	halDisableInterrupt(prAdapter);

	/* FW own */
	/* Set FW own directly without waiting sleep notify */
	prGlueInfo->rHifInfo.fgForceFwOwn = TRUE;
	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	/* Wait for
	*  1. The other unfinished ownership handshakes
	*  2. FW own back
	*/
	while (wait < 500) {
		if ((prAdapter->u4PwrCtrlBlockCnt == 0) &&
		    (prAdapter->fgIsFwOwn == TRUE) &&
#if CFG_SUPPORT_MULTITHREAD
			(!prHifThread ||
#if KERNEL_VERSION(5, 14, 0) <= CFG80211_VERSION_CODE
			prHifThread->__state & TASK_NORMAL) &&
#else
			prHifThread->state & TASK_NORMAL) &&
#endif
#endif
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

	prGlueInfo->rHifInfo.eSuspendtate = PCIE_STATE_SUSPEND;
	pci_save_state(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));

	DBGLOG(HAL, STATE, "mtk_pci_suspend() done!\n");

	/* pending cmd will be kept in queue and no one to handle it after HIF resume.
	 * In STR, it will result in cmd buf full and then cmd buf alloc fail .
	 */
	if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWifiVar.ucWow))
		wlanReleaseAllTxCmdQueue(prGlueInfo->prAdapter);

	return 0;
}


int mtk_pci_resume(struct pci_dev *pdev)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct BUS_INFO *prBusInfo;
	struct CHIP_DBG_OPS *prDebugOps;
	uint32_t count = 0;

	DBGLOG(HAL, STATE, "mtk_pci_resume()\n");

	prGlueInfo = (struct GLUE_INFO *)pci_get_drvdata(pdev);
	if (!prGlueInfo) {
		DBGLOG(HAL, ERROR, "pci_get_drvdata fail!\n");
		return -1;
	}

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;
	prDebugOps = prGlueInfo->prAdapter->chip_info->prDebugOps;

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

	prGlueInfo->rHifInfo.fgForceFwOwn = FALSE;
	halPcieResumeCmd(prGlueInfo->prAdapter);

	while (prGlueInfo->rHifInfo.eSuspendtate != PCIE_STATE_PRE_RESUME_DONE) {
		if (count > 500) {
			DBGLOG(HAL, ERROR, "pre_resume timeout\n");

			if (prDebugOps) {
				if (prDebugOps->show_mcu_debug_info)
					prDebugOps->show_mcu_debug_info(prGlueInfo->prAdapter, NULL, 0,
						DBG_MCU_DBG_ALL, NULL);
#if (CFG_SUPPORT_DEBUG_SOP == 1)
				if (prDebugOps->show_debug_sop_info)
					prDebugOps->show_debug_sop_info(prGlueInfo->prAdapter,
						SLAVENORESP);
#endif
				if (prDebugOps->showCsrInfo)
					prDebugOps->showCsrInfo(prGlueInfo->prAdapter);
			}

			break;
		}

		kalUsleep_range(2000,3000);
		count++;
	}

	wlanResumePmHandle(prGlueInfo);

	/* FW own */
	RECLAIM_POWER_CONTROL_TO_PM(prGlueInfo->prAdapter, FALSE);

	/* Allow upper layers to call the device hard_start_xmit routine. */
	netif_tx_wake_all_queues(prGlueInfo->prDevHandler);

	DBGLOG(HAL, STATE, "mtk_pci_resume() done!\n");

	return 0;
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

#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
	mtk_wifi_driver.probe = mtk_wifi_probe;
	mtk_wifi_driver.remove = mtk_wifi_remove;

	if (platform_driver_register(&mtk_wifi_driver))
		DBGLOG(HAL, ERROR, "platform_driver_register fail\n");
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */

	ret = (pci_register_driver(&mtk_pci_driver) == 0) ?
		WLAN_STATUS_SUCCESS : WLAN_STATUS_FAILURE;

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
void glUnregisterBus(remove_card pfRemove, struct device *prDev)
{
	struct pci_dev *pdev;

	if (!prDev)
		goto RETURN;
	pdev = container_of(prDev, struct pci_dev, dev);

#if CFG_SUPPORT_MULTI_CARD
	if (g_fgDriverProbed)
		pfRemove((void *) pdev);

	if (u4WlanDevNum == 0) {
		g_fgDriverProbed = FALSE;
		pci_unregister_driver(&mtk_pci_driver);
	}
#else
	if (g_fgDriverProbed) {
		pfRemove((void *) pdev);
		g_fgDriverProbed = FALSE;
	}
	pci_unregister_driver(&mtk_pci_driver);
#endif

RETURN:
#if (CFG_SUPPORT_WIFI_RSV_MEM == 1)
	platform_driver_unregister(&mtk_wifi_driver);
#endif  /* CFG_SUPPORT_WIFI_RSV_MEM */
	return;
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
	struct GL_HIF_INFO *prHif = NULL;
	struct HIF_MEM_OPS *prMemOps;

	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;

	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_pci_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;

	prHif = &prGlueInfo->rHifInfo;

	prHif->pdev = (struct pci_dev *)ulCookie;
	prMemOps = &prHif->rMemOps;
	prHif->prDmaDev = prHif->pdev;

#if CFG_SUPPORT_MULTI_CARD
	prHif->CSRBaseAddress = CSRBaseAddress[u4WlanDevNum];
#else
	prHif->CSRBaseAddress = CSRBaseAddress;
#endif

	pci_set_drvdata(prHif->pdev, prGlueInfo);

	SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHif->pdev->dev);

	prGlueInfo->u4InfType = MT_DEV_INF_PCIE;

	prHif->rErrRecoveryCtl.eErrRecovState = ERR_RECOV_STOP_IDLE;
	prHif->rErrRecoveryCtl.u4Status = 0;

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	timer_setup(&prHif->rSerTimer, halHwRecoveryTimeout, 0);
#else
	init_timer(&prHif->rSerTimer);
	prHif->rSerTimer.function = halHwRecoveryTimeout;
	prHif->rSerTimer.data = (unsigned long)prGlueInfo;
	prHif->rSerTimer.expires =
		jiffies + HIF_SER_TIMEOUT * HZ / MSEC_PER_SEC;
#endif

	INIT_LIST_HEAD(&prHif->rTxCmdQ);
	INIT_LIST_HEAD(&prHif->rTxDataQ);
	prHif->u4TxDataQLen = 0;

	prHif->fgIsPowerOff = true;
	prHif->fgIsDumpLog = false;

	halSetMemOps(prChipInfo->platform_device, prMemOps);
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
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct list_head *prCur, *prNext;
	struct TX_CMD_REQ *prTxCmdReq;
	struct TX_DATA_REQ *prTxDataReq;

	del_timer_sync(&prHifInfo->rSerTimer);

	halUninitMsduTokenInfo(prGlueInfo->prAdapter);
	halWpdmaFreeRing(prGlueInfo);

	list_for_each_safe(prCur, prNext, &prHifInfo->rTxCmdQ) {
		prTxCmdReq = list_entry(prCur, struct TX_CMD_REQ, list);
		list_del(prCur);
		kfree(prTxCmdReq);
	}

	list_for_each_safe(prCur, prNext, &prHifInfo->rTxDataQ) {
		prTxDataReq = list_entry(prCur, struct TX_DATA_REQ, list);
		list_del(prCur);
		prHifInfo->u4TxDataQLen--;
	}
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
	int ret = 0;
	struct pci_dev *pdev = NULL;

	ASSERT(pvData);

	pdev = (struct pci_dev *)pvData;

	ret = KAL_DMA_SET_MASK(pdev, DMA_BIT_MASK(g_u4DmaMask));
	if (ret != 0) {
		DBGLOG(INIT, INFO, "set DMA mask failed!errno=%d\n", ret);
		return FALSE;
	}

	ret = pci_request_regions(pdev, pci_name(pdev));
	if (ret != 0) {
		DBGLOG(INIT, INFO,
			"Request PCI resource failed, errno=%d!\n", ret);
	}

	/* map physical address to virtual address for accessing register */
#if CFG_SUPPORT_MULTI_CARD
	CSRBaseAddress[u4WlanDevNum] =
		ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
#else
	CSRBaseAddress = ioremap(pci_resource_start(pdev, 0),
		pci_resource_len(pdev, 0));
#endif
	DBGLOG(INIT, INFO, "ioremap for device %s, region 0x%lX @ 0x%lX\n",
		pci_name(pdev), (unsigned long) pci_resource_len(pdev, 0),
		(unsigned long) pci_resource_start(pdev, 0));
#if CFG_SUPPORT_MULTI_CARD
	if (!CSRBaseAddress[u4WlanDevNum]) {
#else
	if (!CSRBaseAddress) {
#endif
		DBGLOG(INIT, INFO,
			"ioremap failed for device %s, region 0x%lX @ 0x%lX\n",
			pci_name(pdev),
			(unsigned long) pci_resource_len(pdev, 0),
			(unsigned long) pci_resource_start(pdev, 0));
		goto err_out_free_res;
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
#endif
#endif

	/* Set DMA master */
	pci_set_master(pdev);

	return TRUE;

err_out_free_res:
	pci_release_regions(pdev);

	pci_disable_device(pdev);

	return FALSE;
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
	struct BUS_INFO *prBusInfo;
	struct net_device *prNetDevice = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct pci_dev *pdev = NULL;
	int ret = 0;

	ASSERT(pvData);
	if (!pvData)
		return -1;

	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *)pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo)
		return -1;

	prBusInfo = prGlueInfo->prAdapter->chip_info->bus_info;

	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

#if KERNEL_VERSION(4, 8, 0) < LINUX_VERSION_CODE
	DBGLOG(INIT, INFO, "alloc pci irq vectors\n");
	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0) {
		DBGLOG(INIT, ERROR, "alloc_irq_vectors fail(%d)\n", ret);
		return ret;
	}
#endif

	prHifInfo->u4IrqId = pdev->irq;
	ret = request_irq(prHifInfo->u4IrqId, mtk_pci_interrupt,
		IRQF_SHARED, prNetDevice->name, prGlueInfo);
	if (ret != 0) {
#if KERNEL_VERSION(4, 8, 0) < LINUX_VERSION_CODE
		pci_free_irq_vectors(pdev);
#endif
		DBGLOG(INIT, INFO,
			"glBusSetIrq: request_irq  ERROR(%d)\n", ret);
	}
	else if (prBusInfo->initPcieInt)
		prBusInfo->initPcieInt(prGlueInfo);
	return ret;
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
	struct net_device *prNetDevice = NULL;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct pci_dev *pdev = NULL;

	ASSERT(pvData);
	if (!pvData) {
		DBGLOG(INIT, INFO, "%s null pvData\n", __func__);
		return;
	}
	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *) pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo) {
		DBGLOG(INIT, INFO, "%s no glue info\n", __func__);
		return;
	}

	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

	synchronize_irq(pdev->irq);
	free_irq(pdev->irq, prGlueInfo);
#if KERNEL_VERSION(4, 8, 0) < LINUX_VERSION_CODE
	pci_free_irq_vectors(pdev);
#endif
}

u_int8_t glIsReadClearReg(uint32_t u4Address)
{
	return TRUE;
}

void glSetPowerState(IN struct GLUE_INFO *prGlueInfo, IN uint32_t ePowerMode)
{
}

void glGetDev(void *ctx, struct device **dev)
{

	*dev = &((struct pci_dev *)ctx)->dev;
}

void glGetHifDev(struct GL_HIF_INFO *prHif, struct device **dev)
{
	*dev = &(prHif->pdev->dev);
}

uint32_t glGetBusId(void *ctx)
{
	uint32_t u4Bus, u4Dev, u4Func;
	struct pci_dev *pdev = (struct pci_dev *)ctx;

	if (pdev == NULL || pdev->bus == NULL) {
		DBGLOG(HAL, ERROR, "fail to find pcie bus\n");
		return 0;
	}
	u4Bus = (pdev->bus->number) & 0xFF;
	u4Dev = PCI_SLOT(pdev->devfn);
	u4Func = PCI_FUNC(pdev->devfn);

	return (u4Bus << 8) | (u4Dev << 3) | u4Func;
}

#if CFG_CHIP_RESET_SUPPORT
void kalRemoveProbe(IN struct GLUE_INFO *prGlueInfo)
{
	DBGLOG(INIT, WARN, "[SER][L0] not support...\n");
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

void halPciePreSuspendCmd(IN struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl;
	uint32_t rStatus;

	kalMemZero(&rCmdHifCtrl, sizeof(struct CMD_HIF_CTRL));

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

void halPcieResumeCmd(IN struct ADAPTER *prAdapter)
{
	struct CMD_HIF_CTRL rCmdHifCtrl;
	uint32_t rStatus;

	kalMemZero(&rCmdHifCtrl, sizeof(struct CMD_HIF_CTRL));

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
	IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo,
	IN uint8_t *pucEventBuf)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.eSuspendtate =
		PCIE_STATE_PRE_SUSPEND_DONE;
}

void halPciePreSuspendTimeout(
	IN struct ADAPTER *prAdapter,
	IN struct CMD_INFO *prCmdInfo)
{
	ASSERT(prAdapter);

	prAdapter->prGlueInfo->rHifInfo.eSuspendtate =
		PCIE_STATE_PRE_SUSPEND_FAIL;
}

#if CFG_SUPPORT_MULTI_CARD
uint32_t wlanDevCfgParseEntry(struct ADAPTER *prAdapter,
		      int8_t **pprArgs, int8_t *prArgv_size, int32_t i4Nargs)
{
	uint32_t u4Ret = 0;
	uint32_t u4Bus = 0, u4Dev = 0, u4Func = 0;
	uint32_t u4WlanNameLen = 0;
	struct GLUE_INFO *prGlueInfo = NULL;
	struct pci_dev *pdev = NULL;

	if (!prAdapter || !prAdapter->prGlueInfo)
		return WLAN_STATUS_FAILURE;

	if (i4Nargs < DEV_CFG_ARGS_NUM) {
		DBGLOG(INIT, ERROR, "i4Nargs Fail\n");
		return WLAN_STATUS_FAILURE;
	}

	prGlueInfo = prAdapter->prGlueInfo;
	pdev = prGlueInfo->rHifInfo.pdev;

	u4Ret = kalkStrtou32(pprArgs[DEV_CFG_BUS_NUM], 0, &u4Bus);
	if (u4Ret)
		return WLAN_STATUS_FAILURE;

	u4Ret = kalkStrtou32(pprArgs[DEV_CFG_DEV_NUM], 0, &u4Dev);
	if (u4Ret)
		return WLAN_STATUS_FAILURE;

	u4Ret = kalkStrtou32(pprArgs[DEV_CFG_FUNC_NUM], 0, &u4Func);
	if (u4Ret)
		return WLAN_STATUS_FAILURE;

	if (u4Bus  != pdev->bus->number ||
		u4Dev  != PCI_SLOT(pdev->devfn) ||
		u4Func != PCI_FUNC(pdev->devfn)) {
		return WLAN_STATUS_FAILURE;
	}

	u4WlanNameLen = kalStrLen(pprArgs[DEV_CFG_WLAN_NAME]) + 1;

	kalMemCopy(
		prGlueInfo->aucDevCfgPath,
		pprArgs[DEV_CFG_WLAN_NAME], u4WlanNameLen);

	DBGLOG(INIT, INFO, "Wlan Config Dir Name=%s\n",
			pprArgs[DEV_CFG_WLAN_NAME]);

	return WLAN_STATUS_SUCCESS;
}
#endif /* CFG_SUPPORT_MULTI_CARD */
