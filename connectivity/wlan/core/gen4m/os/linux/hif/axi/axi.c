// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/******************************************************************************
 *[File]             axi.c
 *[Version]          v1.0
 *[Revision Date]    2010-03-01
 *[Author]
 *[Description]
 *    The program provides AXI HIF driver
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
#include <linux/platform_device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#ifndef CONFIG_X86
#include <asm/memory.h>
#endif
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>

#include "mt66xx_reg.h"

#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of.h>

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

static const struct platform_device_id mtk_axi_ids[] = {
	{	.name = "CONNAC",
#ifdef CONNAC
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_connac},
#endif /* CONNAC */
#ifdef SOC2_1X1
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc2_1x1},
#endif /* SOC2_1X1 */
#ifdef SOC2_2X2
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc2_2x2},
#endif /* SOC2_2X2 */
#ifdef SOC3_0
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc3_0},
#endif /* SOC3_0 */
#ifdef SOC5_0
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc5_0},
#endif /* SOC5_0 */
#ifdef SOC7_0
		.driver_data = (kernel_ulong_t)&mt66xx_driver_data_soc7_0},
#endif /* SOC7_0 */

	{ /* end: all zeroes */ },
};

MODULE_DEVICE_TABLE(axi, mtk_axi_ids);

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
static remove_card pfWlanShutdown;

static struct platform_driver mtk_axi_driver = {
	.driver = {
		.name = "wlan",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = mtk_axi_of_ids,
#endif
		.probe_type = PROBE_FORCE_SYNCHRONOUS,
	},
	.id_table = mtk_axi_ids,
	.probe = NULL,
	.remove = NULL,
	.shutdown = NULL,
};

static struct GLUE_INFO *g_prGlueInfo;
static void *CSRBaseAddress;
static u64 g_u8CsrOffset;
static u32 g_u4CsrSize;
static u_int8_t g_fgDriverProbed = FALSE;

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

struct mt66xx_hif_driver_data *get_platform_driver_data(void)
{
	return (struct mt66xx_hif_driver_data *) mtk_axi_ids[0].driver_data;
}

static int hifAxiProbe(void)
{
	int ret = 0;
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	struct platform_device *pdev;

	prDriverData = get_platform_driver_data();
	prChipInfo = prDriverData->chip_info;
	pdev = prChipInfo->platform_device;
	if (!pdev) {
		DBGLOG(INIT, ERROR, "pdev is NULL\n");
		ret = -1;
		goto out;
	}

	if (pfWlanProbe((void *) pdev, (void *) prDriverData) !=
			WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR, "pfWlanProbe fail!\n");
		ret = -1;
		goto out;
	}
	g_fgDriverProbed = TRUE;

out:
	DBGLOG(INIT, TRACE, "hifAxiProbe() done(%d)\n", ret);

	return ret;
}

int hifAxiRemove(void)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;

	prDriverData = get_platform_driver_data();
	prChipInfo = prDriverData->chip_info;

	if (g_fgDriverProbed) {
		pfWlanRemove();
		DBGLOG(INIT, TRACE, "pfWlanRemove done\n");
	}

	g_fgDriverProbed = FALSE;

	DBGLOG(INIT, TRACE, "hifAxiRemove() done\n");
	return 0;
}

static int axiDmaSetup(struct platform_device *pdev,
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


static bool axiCsrIoremap(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
	struct resource res;
	signed int ret;
	uint32_t idx = 0;

	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi");
	if (!node) {
		DBGLOG(INIT, ERROR, "WIFI-OF: get wifi device node fail\n");
		return false;
	}

	ret = of_property_read_u32(node, "remap-idx", &idx);
	if (ret < 0)
		DBGLOG(INIT, TRACE, "use default remap-idx:%u\n", idx);
	else
		DBGLOG(INIT, TRACE, "remap-idx:%u\n", idx);

	if (of_address_to_resource(node, idx, &res)) {
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
	prChipInfo = prDriverData->chip_info;

	if (CSRBaseAddress) {
		DBGLOG(INIT, ERROR, "CSRBaseAddress not iounmap!\n");
		return false;
	}

	request_mem_region(g_u8CsrOffset, g_u4CsrSize, axi_name(pdev));

	/* map physical address to virtual address for accessing register */
#ifdef CONFIG_OF
	CSRBaseAddress = of_iomap(node, idx);
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

	prChipInfo->CSRBaseAddress = CSRBaseAddress;
	prChipInfo->u8CsrOffset = g_u8CsrOffset & BITS(0, 31);

	/* This Base Address is aim for PCIE (for MAWD) */
	prChipInfo->HostCSRBaseAddress = NULL;
	prChipInfo->u4HostCsrOffset = 0;
	prChipInfo->u4HostCsrSize = 0;

	DBGLOG(INIT, INFO,
	       "CSRBaseAddress:0x%llX ioremap idx:%u region 0x%X @ 0x%llX\n",
	       (uint64_t)CSRBaseAddress, idx, g_u4CsrSize, g_u8CsrOffset);

	return true;
}

static void axiCsrIounmap(struct platform_device *pdev,
	struct mt66xx_chip_info *prChipInfo)
{
	if (!CSRBaseAddress)
		return;

	prChipInfo->CSRBaseAddress = NULL;

	/* Unmap CSR base address */
	iounmap(CSRBaseAddress);
	release_mem_region(g_u8CsrOffset, g_u4CsrSize);

	CSRBaseAddress = NULL;
	g_u8CsrOffset = 0;
	g_u4CsrSize = 0;
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
static irqreturn_t mtk_axi_interrupt(int irq, void *dev_instance)
{
	struct GLUE_INFO *prGlueInfo = NULL;
#if AXI_ISR_DEBUG_LOG
	static DEFINE_RATELIMIT_STATE(_rs, 2 * HZ, 1);
#endif

	prGlueInfo = (struct GLUE_INFO *)dev_instance;
	if (!prGlueInfo) {
#if AXI_ISR_DEBUG_LOG
		DBGLOG(HAL, INFO, "No glue info in mtk_axi_interrupt()\n");
#endif
		return IRQ_NONE;
	}

	prGlueInfo->u8HifIntTime = sched_clock();
	GLUE_INC_REF_CNT(prGlueInfo->prAdapter->rHifStats.u4HwIsrCount);
	halDisableInterrupt(prGlueInfo->prAdapter);

	if (test_bit(GLUE_FLAG_HALT_BIT, &prGlueInfo->ulFlag)) {
#if AXI_ISR_DEBUG_LOG
		DBGLOG(HAL, INFO, "GLUE_FLAG_HALT skip INT\n");
#endif
		return IRQ_NONE;
	}

	kalSetIntEvent(prGlueInfo);
#if AXI_ISR_DEBUG_LOG
	if (__ratelimit(&_rs))
		LOG_FUNC("In HIF ISR.\n");
#endif

	return IRQ_HANDLED;
}
#if (CFG_SUPPORT_CONNINFRA == 1)
static irqreturn_t mtk_sw_int_top_handler(int irq, void *dev_instance)
{
	struct ADAPTER *prAdapter = (struct ADAPTER *)dev_instance;
	struct GL_HIF_INFO *prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	disable_irq_nosync(prHifInfo->u4IrqId_1);
	return IRQ_WAKE_THREAD;
}

static irqreturn_t mtk_sw_int_thread_handler(int irq, void *dev_instance)
{
	struct ADAPTER *prAdapter;
	struct GL_HIF_INFO *prHifInfo;
	bool enable_int = true;

	prAdapter = (struct ADAPTER *)dev_instance;
	if (!prAdapter) {
		DBGLOG(HAL, WARN, "NULL prAdapter.\n");
		goto exit;
	}
	prHifInfo = &prAdapter->prGlueInfo->rHifInfo;

	GLUE_INC_REF_CNT(prAdapter->rHifStats.u4SwIsrCount);
	enable_int = asicConnac2xSwIntHandler(prAdapter);
	if (enable_int)
		enable_irq(prHifInfo->u4IrqId_1);

exit:
	return IRQ_HANDLED;
}
#endif

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

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a AXI probe function
 *
 * \param[in] func   pointer to AXI handle
 * \param[in] id     pointer to AXI device id table
 *
 * \return void
 */
/*----------------------------------------------------------------------------*/
static int mtk_axi_probe(struct platform_device *pdev)
{
	struct mt66xx_hif_driver_data *prDriverData;
	struct mt66xx_chip_info *prChipInfo;
	int ret = 0;

	prDriverData = (struct mt66xx_hif_driver_data *)
			mtk_axi_ids[0].driver_data;
	prChipInfo = prDriverData->chip_info;
	prChipInfo->pdev = (void *) pdev;
	prChipInfo->platform_device = (void *) pdev;

	platform_set_drvdata(pdev, (void *) prDriverData);

	axiSetupFwFlavor(pdev, prDriverData);

	if (!axiCsrIoremap(pdev))
		goto exit;

	ret = axiDmaSetup(pdev, prDriverData);
	if (ret)
		goto exit;

#if (CFG_MTK_ANDROID_WMT == 1)
	emi_mem_init(prChipInfo, pdev);
	ret = halAllocHifMem(pdev, prDriverData);
	if (ret)
		goto exit;
#endif

exit:
	DBGLOG(INIT, INFO, "mtk_axi_probe() done, ret: %d\n", ret);
	return ret;
}

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
static int mtk_axi_remove(struct platform_device *pdev)
#else
static void mtk_axi_remove(struct platform_device *pdev)
#endif
{
	struct mt66xx_hif_driver_data *prDriverData =
		platform_get_drvdata(pdev);
	struct mt66xx_chip_info *prChipInfo = prDriverData->chip_info;

	axiCsrIounmap(pdev, prChipInfo);
#if (CFG_MTK_ANDROID_WMT == 1)
	halFreeHifMem(pdev, WIFI_RSV_MEM_WFDMA);
#endif
	emi_mem_uninit(prChipInfo, pdev);
	platform_set_drvdata(pdev, NULL);

#if (KERNEL_VERSION(6, 11, 0) > LINUX_VERSION_CODE)
	return 0;
#endif
}

static void mtk_axi_shutdown(struct platform_device *pdev)
{
	DBGLOG(INIT, INFO, "enter shutdown\n");
	wfsys_lock();
	if (g_fgDriverProbed && pfWlanShutdown) {
		DBGLOG(INIT, INFO, "do shutdown\n");
		pfWlanShutdown();
		g_fgDriverProbed = FALSE;
	}
	wfsys_unlock();
}

static int mtk_axi_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	return 0;
}

int mtk_axi_resume(struct platform_device *pdev)
{
	return 0;
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
uint32_t glRegisterShutdownCB(remove_card pfShutdown)
{
	ASSERT(pfShutdown);
	pfWlanShutdown = pfShutdown;

	mtk_axi_driver.shutdown = mtk_axi_shutdown;
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

	mtk_axi_driver.probe = mtk_axi_probe;
	mtk_axi_driver.remove = mtk_axi_remove;

	mtk_axi_driver.suspend = mtk_axi_suspend;
	mtk_axi_driver.resume = mtk_axi_resume;

	ret = (platform_driver_register(&mtk_axi_driver) == 0) ?
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
void glUnregisterBus(remove_card pfRemove)
{
	if (g_fgDriverProbed) {
		pfRemove();
		g_fgDriverProbed = FALSE;
	}
	platform_driver_unregister(&mtk_axi_driver);
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

	g_prGlueInfo = prGlueInfo;
	prHif = &prGlueInfo->rHifInfo;
	glGetChipInfo((void **)&prChipInfo);
	prBusInfo = prChipInfo->bus_info;
	prMemOps = &prHif->rMemOps;

	prHif->pdev = (struct platform_device *)ulCookie;
	prHif->prDmaDev = &prHif->pdev->dev;

	SET_NETDEV_DEV(prGlueInfo->prDevHandler, &prHif->pdev->dev);

	prGlueInfo->u4InfType = MT_DEV_INF_AXI;

	prHif->fgIsPowerOff = true;
	prHif->fgIsDumpLog = false;

	prHif->u4RxDataRingSize = prBusInfo->rx_data_ring_size;
	prHif->u4RxEvtRingSize = prBusInfo->rx_evt_ring_size;

	prMemOps->allocTxDesc = halCopyPathAllocTxDesc;
	prMemOps->allocRxDesc = halCopyPathAllocRxDesc;
	prMemOps->allocTxCmdBuf = halCopyPathAllocTxCmdBuf;
	prMemOps->allocTxDataBuf = halCopyPathAllocTxDataBuf;
	prMemOps->allocRxEvtBuf = halCopyPathAllocRxBuf;
	prMemOps->allocRxDataBuf = halCopyPathAllocRxBuf;
	prMemOps->allocRuntimeMem = NULL;
	prMemOps->copyCmd = halCopyPathCopyCmd;
	prMemOps->copyEvent = halCopyPathCopyEvent;
	prMemOps->copyTxData = halCopyPathCopyTxData;
	prMemOps->copyRxData = halCopyPathCopyRxData;
	prMemOps->mapTxDataBuf = NULL;
	prMemOps->mapTxCmdBuf = NULL;
	prMemOps->mapRxBuf = NULL;
	prMemOps->unmapTxDataBuf = NULL;
	prMemOps->unmapTxCmdBuf = NULL;
	prMemOps->unmapRxBuf = NULL;
	prMemOps->freeDesc = NULL;
	prMemOps->freeCmdBuf = NULL;
	prMemOps->freeDataBuf = NULL;
	prMemOps->freePacket = NULL;
	prMemOps->dumpTx = halCopyPathDumpTx;
	prMemOps->dumpRx = halCopyPathDumpRx;
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
	ASSERT(pvData);

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
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct platform_device *pdev = NULL;
#ifdef CONFIG_OF
	struct device_node *node = NULL;
#endif
	int ret = 0, en_wake_ret = 0;

	ASSERT(pvData);
	if (!pvData)
		return -1;

	prNetDevice = (struct net_device *)pvData;
	prGlueInfo = (struct GLUE_INFO *)pvCookie;
	ASSERT(prGlueInfo);
	if (!prGlueInfo)
		return -1;

	prHifInfo = &prGlueInfo->rHifInfo;
	pdev = prHifInfo->pdev;

	prHifInfo->u4IrqId = AXI_WLAN_IRQ_NUMBER;
#ifdef CONFIG_OF
	node = of_find_compatible_node(NULL, NULL, "mediatek,wifi");
	if (node) {
		prHifInfo->u4IrqId = irq_of_parse_and_map(node, 0);
#if (CFG_SUPPORT_CONNINFRA == 1)
		prHifInfo->u4IrqId_1 = irq_of_parse_and_map(node, 1);
#endif
	}
	else
		DBGLOG(INIT, ERROR,
			"WIFI-OF: get wifi device node fail\n");
#endif
#if (CFG_SUPPORT_CONNINFRA == 1)
	DBGLOG(INIT, INFO, "glBusSetIrq: request_irq num(%d), num(%d)\n",
	       prHifInfo->u4IrqId, prHifInfo->u4IrqId_1);
#else
	DBGLOG(INIT, INFO, "glBusSetIrq: request_irq num(%d)\n",
	       prHifInfo->u4IrqId);
#endif /*end of CFG_SUPPORT_CONNINFRA == 1*/
	ret = request_irq(prHifInfo->u4IrqId, mtk_axi_interrupt, IRQF_SHARED,
			  prNetDevice->name, prGlueInfo);
	if (ret != 0) {
		DBGLOG(INIT, INFO, "request_irq(%u) ERROR(%d)\n",
				prHifInfo->u4IrqId, ret);
		goto exit;
	}
	en_wake_ret = enable_irq_wake(prHifInfo->u4IrqId);
	if (en_wake_ret)
		DBGLOG(INIT, INFO, "enable_irq_wake(%u) ERROR(%d)\n",
				prHifInfo->u4IrqId, en_wake_ret);
#if (CFG_SUPPORT_CONNINFRA == 1)
	ret = request_threaded_irq(prHifInfo->u4IrqId_1,
		mtk_sw_int_top_handler,
		mtk_sw_int_thread_handler,
		IRQF_SHARED,
		prNetDevice->name,
		prGlueInfo->prAdapter);
	if (ret != 0) {
		DBGLOG(INIT, INFO, "request_irq(%u) ERROR(%d)\n",
				prHifInfo->u4IrqId_1, ret);
		goto exit;
	}

	en_wake_ret = enable_irq_wake(prHifInfo->u4IrqId_1);
	if (en_wake_ret)
		DBGLOG(INIT, INFO, "enable_irq_wake(%u) ERROR(%d)\n",
				prHifInfo->u4IrqId_1, en_wake_ret);
#endif

exit:
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
	struct platform_device *pdev = NULL;

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

	synchronize_irq(prHifInfo->u4IrqId);
	irq_set_affinity_hint(prHifInfo->u4IrqId, NULL);
	free_irq(prHifInfo->u4IrqId, prGlueInfo);
#if (CFG_SUPPORT_CONNINFRA == 1)
	synchronize_irq(prHifInfo->u4IrqId_1);
	irq_set_affinity_hint(prHifInfo->u4IrqId_1, NULL);
	free_irq(prHifInfo->u4IrqId_1, prGlueInfo->prAdapter);
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
	*dev = &((struct platform_device *)ctx)->dev;
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

int32_t glBusFuncOn(void)
{
	return hifAxiProbe();
}

void glBusFuncOff(void)
{
	hifAxiRemove();
	g_fgDriverProbed = FALSE;
}

