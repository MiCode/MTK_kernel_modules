// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2021 MediaTek Inc.

#include <linux/arm-smccc.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <uapi/linux/dma-heap.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>
#include <linux/uaccess.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/iommu.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/fdtable.h>
#include <linux/interrupt.h>
#include <linux/remoteproc/mtk_ccu.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <soc/mediatek/smi.h>
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
#include <mt-plat/aee.h>
#endif
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
#include <mt-plat/mrdump.h>
#endif
#include <linux/version.h>

#include "mtk_ccu_isp71.h"
#include "mtk_ccu_common.h"
#include "mtk-interconnect.h"
#include "remoteproc_internal.h"
#include <mtk-smmu-v3.h>
#include <mtk_heap.h>

#define CCU_SET_MMQOS
/* #define CCU1_DEVICE */
#define MTK_CCU_MB_RX_TIMEOUT_SPEC    1000  /* 10ms */
#define ERROR_DESC_LEN	160

#define MTK_CCU_TAG "[ccu_rproc]"
#define LOG_DBG(format, args...)
#define LOG_DBG_MUST(format, args...) \
	pr_info(MTK_CCU_TAG "[%s] " format, __func__, ##args)

static char *buf_name = "CCU_LOG_DBGDUMP";

struct mtk_ccu_clk_name ccu_clk_name_isp71[] = {
	{true, "CLK_TOP_CCUSYS_SEL"},
	{true, "CLK_TOP_CCU_AHB_SEL"},
	{true, "CLK_CCU_LARB"},
	{true, "CLK_CCU_AHB"},
	{true, "CLK_CCUSYS_CCU0"},
	{true, "CAM_LARB14"},
	{true, "CAM_MM1_GALS"},
	{false, ""}};

struct mtk_ccu_clk_name ccu_clk_name_isp7s[] = {
	{true, "CLK_TOP_CCUSYS_SEL"},
	{true, "CLK_TOP_CCU_AHB_SEL"},
	{true, "CLK_CCU_LARB"},
	{true, "CLK_CCU_AHB"},
	{true, "CLK_CCUSYS_CCU0"},
	{true, "CAM_CCUSYS"},
	{true, "CAM_CAM2SYS_GALS"},
	{true, "CAM_CAM2MM1_GALS"},
	{true, "CAM_CAM2MM0_GALS"},
	{true, "CAM_MRAW"},
	{true, "CAM_CG"},
	{false, ""}};

struct mtk_ccu_clk_name ccu_clk_name_isp7sp[] = {
	{true, "TOP_CAM"},
	{true, "TOP_CCU_AHB"},
	{true, "VLP_CCUSYS"},
	{true, "VLP_CCUTM"},
	{true, "CCU2MM0_GALS"},
	{true, "CCU_LARB"},
	{true, "CCU_AHB"},
	{true, "CCUSYS_CCU0"},
	{true, "CAM_CG"},
	{true, "CAM_VCORE_CG"},
	{false, ""}};

struct mtk_ccu_clk_name ccu_clk_name_isp7spl[] = {
	{true, "TOP_CAM"},
	{true, "TOP_CCU_AHB"},
	{true, "TOP_CCUSYS"},
	{true, "TOP_CCUTM"},
	{true, "CCU2MM0_GALS"},
	{true, "CCU_LARB"},
	{true, "CCU_AHB"},
	{true, "CCUSYS_CCU0"},
	{true, "CAM_CG"},
	{true, "CAM_VCORE_CG"},
	{false, ""}};

struct mtk_ccu_clk_name ccu_clk_name_isp8[] = {
	{true, "VLP_CCUSYS"},
	{true, "VLP_CCUTM"},
	{true, "CCU2MM0_GALS"},
	{true, "CCU_LARB"},
	{true, "CCU_INFRA"},
	{true, "CCUSYS_CCU0"},
	{true, "CAM_VCORE_CG"},
	{false, ""}};

struct mtk_ccu_clk_name ccu_clk_name_isp8l[] = {
	{true, "TOP_CCU_AHB"},
	{true, "TOP_CCUSYS"},
	{true, "TOP_CCUTM"},
	{true, "CCU2MM0_GALS"},
	{true, "CCU_LARB"},
	{true, "CCU_INFRA"},
	{true, "CCUSYS_CCU0"},
	{true, "CAM_VCORE_CG"},
	{false, ""}};

#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
static struct mtk_ccu *dev_ccu;
#endif
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
extern struct rproc *ccu_rproc_ipifuzz; /* IPIFuzz, to export symbol. */
struct rproc *ccu_rproc_ipifuzz; /* IPIFuzz */
#endif
static int mtk_ccu_probe(struct platform_device *dev);
static int mtk_ccu_remove(struct platform_device *dev);
static int mtk_ccu_read_platform_info_from_dt(struct device_node
	*node, struct mtk_ccu *ccu);
static int mtk_ccu_get_power(struct mtk_ccu *ccu, struct device *dev);
static void mtk_ccu_put_power(struct mtk_ccu *ccu, struct device *dev);
static int mtk_ccu_stopx(struct rproc *rproc, bool normal_stop);

static int
mtk_ccu_deallocate_mem(struct device *dev, struct mtk_ccu_mem_handle *memHandle, bool smmu)
{
	if ((!dev) || (!memHandle))
		return -EINVAL;

	if (smmu)
		goto dealloc_with_smmu;

	dma_free_attrs(dev, memHandle->meminfo.size, memHandle->meminfo.va,
		memHandle->meminfo.mva, DMA_ATTR_WRITE_COMBINE);

	goto dealloc_end;

dealloc_with_smmu:
	if ((memHandle->dmabuf) && (memHandle->meminfo.va)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		dma_buf_vunmap_unlocked(memHandle->dmabuf, &memHandle->map);
#else
		dma_buf_vunmap(memHandle->dmabuf, &memHandle->map);
#endif
		memHandle->meminfo.va = NULL;
	}
	if ((memHandle->attach) && (memHandle->sgt)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
		dma_buf_unmap_attachment_unlocked(memHandle->attach, memHandle->sgt,
			DMA_FROM_DEVICE);
#else
		dma_buf_unmap_attachment(memHandle->attach, memHandle->sgt,
			DMA_FROM_DEVICE);
#endif
		memHandle->sgt = NULL;
	}
	if ((memHandle->dmabuf) && (memHandle->attach)) {
		dma_buf_detach(memHandle->dmabuf, memHandle->attach);
		memHandle->attach = NULL;
	}
	if (memHandle->dmabuf) {
		dma_heap_buffer_free(memHandle->dmabuf);
		memHandle->dmabuf = NULL;
	}

dealloc_end:
	memset(memHandle, 0, sizeof(struct mtk_ccu_mem_handle));

	return 0;
}

static int
mtk_ccu_allocate_mem(struct device *dev, struct mtk_ccu_mem_handle *memHandle, bool smmu)
{
	struct dma_heap *dmaheap;
	struct device *sdev;
	int ret;

	if ((!dev) || (!memHandle))
		return -EINVAL;

	if (memHandle->meminfo.size <= 0)
		return -EINVAL;

	if (smmu)
		goto alloc_with_smmu;

	/* get buffer virtual address */
	memHandle->meminfo.va = dma_alloc_attrs(dev, memHandle->meminfo.size,
		&memHandle->meminfo.mva, GFP_KERNEL, DMA_ATTR_WRITE_COMBINE);

	if (memHandle->meminfo.va == NULL) {
		dev_err(dev, "fail to get buffer kernel virtual address");
		return -EINVAL;
	}

	goto alloc_show;

alloc_with_smmu:
	LOG_DBG_MUST("alloc by DMA-BUF\n");

	/* dma_heap_find() will fail if mtk_ccu.ko is in ramdisk. */
	dmaheap = dma_heap_find("mtk_mm-uncached");
	if (!dmaheap) {
		dev_err(dev, "fail to find dma_heap");
		return -EINVAL;
	}

	memHandle->dmabuf = dma_heap_buffer_alloc(dmaheap, memHandle->meminfo.size,
		O_RDWR | O_CLOEXEC, DMA_HEAP_VALID_HEAP_FLAGS);
	dma_heap_put(dmaheap);
	if (IS_ERR(memHandle->dmabuf)) {
		dev_err(dev, "fail to alloc dma_buf");
		memHandle->dmabuf = NULL;
		goto err_out;
	}

	mtk_dma_buf_set_name(memHandle->dmabuf, buf_name);

	sdev = mtk_smmu_get_shared_device(dev);
	if (!sdev) {
		dev_err(dev, "fail to mtk_smmu_get_shared_device()");
		goto err_out;
	}

	memHandle->attach = dma_buf_attach(memHandle->dmabuf, sdev);
	if (IS_ERR(memHandle->attach)) {
		dev_err(dev, "fail to attach dma_buf");
		memHandle->attach = NULL;
		goto err_out;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	memHandle->sgt = dma_buf_map_attachment_unlocked(memHandle->attach, DMA_FROM_DEVICE);
#else
	memHandle->sgt = dma_buf_map_attachment(memHandle->attach, DMA_FROM_DEVICE);
#endif
	if (IS_ERR(memHandle->sgt)) {
		dev_err(dev, "fail to map attachment");
		memHandle->sgt = NULL;
		goto err_out;
	}

	memHandle->meminfo.mva = sg_dma_address(memHandle->sgt->sgl);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	ret = dma_buf_vmap_unlocked(memHandle->dmabuf, &memHandle->map);
#else
	ret = dma_buf_vmap(memHandle->dmabuf, &memHandle->map);
#endif
	memHandle->meminfo.va = memHandle->map.vaddr;
	if ((ret) || (memHandle->meminfo.va == NULL)) {
		dev_err(dev, "fail to map va");
		memHandle->meminfo.va = NULL;
		goto err_out;
	}

alloc_show:
	dev_info(dev, "success: size(0x%x), va(0x%lx), mva(%pad)\n",
		memHandle->meminfo.size, (unsigned long)memHandle->meminfo.va,
		&memHandle->meminfo.mva);

	return 0;

err_out:
	mtk_ccu_deallocate_mem(dev, memHandle, smmu);
	return -EINVAL;
}

static void mtk_ccu_set_log_memory_address(struct mtk_ccu *ccu)
{
	struct mtk_ccu_mem_info *meminfo;
	int offset;

	if (ccu->ext_buf.meminfo.va != 0) {
		meminfo = &ccu->ext_buf.meminfo;
		offset = 0;
	} else {
		dev_info(ccu->dev, "no log buf setting\n");
		return;
	}

	/* log chunk1 */
	ccu->log_info[0].fd = meminfo->fd;
	ccu->log_info[0].size = MTK_CCU_DRAM_LOG_BUF_SIZE;
	ccu->log_info[0].offset = offset;
	ccu->log_info[0].mva = meminfo->mva + offset;
	ccu->log_info[0].va = meminfo->va + offset;
	*((uint32_t *)(ccu->log_info[0].va)) = LOG_ENDEND;

	/* log chunk2 */
	ccu->log_info[1].fd = meminfo->fd;
	ccu->log_info[1].size = MTK_CCU_DRAM_LOG_BUF_SIZE;
	ccu->log_info[1].offset = offset + MTK_CCU_DRAM_LOG_BUF_SIZE;
	ccu->log_info[1].mva = ccu->log_info[0].mva + MTK_CCU_DRAM_LOG_BUF_SIZE;
	ccu->log_info[1].va = ccu->log_info[0].va + MTK_CCU_DRAM_LOG_BUF_SIZE;
	*((uint32_t *)(ccu->log_info[1].va)) = LOG_ENDEND;

	/* sram log */
	ccu->log_info[2].fd = meminfo->fd;
	ccu->log_info[2].size = MTK_CCU_SRAM_LOG_INDRAM_BUF_SIZE;
	ccu->log_info[2].offset = offset + MTK_CCU_DRAM_LOG_BUF_SIZE * 2;
	ccu->log_info[2].mva = ccu->log_info[1].mva + MTK_CCU_DRAM_LOG_BUF_SIZE;
	ccu->log_info[2].va = ccu->log_info[1].va + MTK_CCU_DRAM_LOG_BUF_SIZE;

	ccu->log_info[3].fd = meminfo->fd;
	ccu->log_info[3].size = MTK_CCU_DRAM_LOG_BUF_SIZE * 2;
	ccu->log_info[3].offset = offset;
	ccu->log_info[3].mva = ccu->log_info[0].mva;
	ccu->log_info[3].va = ccu->log_info[0].va;
}

uint32_t read_ccu_info_regd(struct mtk_ccu *ccu, uint32_t addr)
{
	uint8_t *dmem_base = (uint8_t *)ccu->dmem_base;

	if (addr > MTK_CCU_SPARE_REG31)
		return 0;

	return readl(dmem_base + addr + SPARE_REG_OFFSET_AP);
}

int mtk_ccu_sw_hw_reset(struct mtk_ccu *ccu)
{
	uint32_t duration = 0;
	uint32_t ccu_status;
	uint8_t *ccu_base = (uint8_t *)ccu->ccu_base;
	/* ISP_7SPL with RV33 use HALT_MASK_RV55. */
	uint32_t halt_mask = (ccu->ccu_version >= CCU_VER_ISP7SP) ?
				HALT_MASK_RV55 : HALT_MASK_RV33;

	/* check halt is up */
	ccu_status = readl(ccu_base + MTK_CCU_MON_ST);
	LOG_DBG_MUST("polling CCU halt(0x%08x)\n", ccu_status);
	duration = 0;
	while ((ccu_status & halt_mask) != halt_mask) {
		duration++;
		if (duration > 1000) {
			dev_err(ccu->dev,
			"polling CCU halt, timeout: (0x%08x)\n", ccu_status);
			mtk_smi_dbg_hang_detect("CCU");
			break;
		}
		udelay(10);
		ccu_status = readl(ccu_base + MTK_CCU_MON_ST);
	}
	LOG_DBG_MUST("polling CCU halt done(0x%08x)\n", ccu_status);

	return true;
}

struct platform_device *mtk_ccu_get_pdev(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *ccu_node;
	struct platform_device *ccu_pdev;

	ccu_node = of_parse_phandle(dev->of_node, "mediatek,ccu_rproc", 0);
	if (!ccu_node) {
		dev_err(dev, "failed to get ccu node\n");
		return NULL;
	}

	ccu_pdev = of_find_device_by_node(ccu_node);
	if (WARN_ON(!ccu_pdev)) {
		dev_err(dev, "failed to get ccu pdev\n");
		of_node_put(ccu_node);
		return NULL;
	}

	return ccu_pdev;
}
EXPORT_SYMBOL_GPL(mtk_ccu_get_pdev);

static int mtk_ccu_run(struct mtk_ccu *ccu)
{
	int32_t timeout = 100;
	uint32_t init_status_reg;
#if !defined(SECURE_CCU)
	uint8_t	*ccu_base = (uint8_t *)ccu->ccu_base;
#endif
	uint8_t	*ccu_spare_base = (uint8_t *)ccu->ccu_spare_base;
#if defined(SECURE_CCU)
	struct arm_smccc_res res;
#else
	struct mtk_ccu_mem_info *bin_mem =
		mtk_ccu_get_meminfo(ccu, MTK_CCU_DDR);
	dma_addr_t remapOffset;

	if (!bin_mem) {
		dev_err(ccu->dev, "get binary memory info failed\n");
		return -EINVAL;
	}
#endif

	LOG_DBG("+\n");

	/*1. Set CCU remap offset & log level*/
#if !defined(SECURE_CCU)
	remapOffset = bin_mem->mva - MTK_CCU_CACHE_BASE;
	writel(remapOffset >> 4, ccu_base + MTK_CCU_REG_AXI_REMAP);
#endif
	writel(ccu->log_level, ccu_spare_base + MTK_CCU_SPARE_REG04);
	writel(ccu->log_taglevel, ccu_spare_base + MTK_CCU_SPARE_REG05);

#if defined(SECURE_CCU)
	if (ccu->compact_ipc)
		writel(CCU_GO_TO_RUN, ccu_spare_base + MTK_CCU_SPARE_REG30);
	else
		writel(CCU_GO_TO_RUN, ccu_spare_base + MTK_CCU_SPARE_REG06);
#ifdef CONFIG_ARM64
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u64) CCU_SMC_REQ_RUN, 0, 0, 0, 0, 0, 0, &res);
#endif
#ifdef CONFIG_ARM_PSCI
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u32) CCU_SMC_REQ_RUN, 0, 0, 0, 0, 0, 0, &res);
#endif
#else  /* !defined(SECURE_CCU) */
	/*2. Set CCU_RESET. CCU_HW_RST=0*/
	writel(0x0, ccu_base + MTK_CCU_REG_RESET);
#endif

	init_status_reg = (ccu->compact_ipc) ? MTK_CCU_SPARE_REG31 : MTK_CCU_SPARE_REG08;

	/*3. Pulling CCU init done spare register*/
	while ((readl(ccu_spare_base + init_status_reg)
		!= CCU_STATUS_INIT_DONE) && (timeout >= 0)) {
		usleep_range(50, 100);
		timeout = timeout - 1;
	}
	if (timeout <= 0) {
		dev_err(ccu->dev, "CCU init timeout\n");
		dev_err(ccu->dev, "ccu initial debug info: %x\n",
			(ccu->compact_ipc) ?
			read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG17) :
			readl(ccu_spare_base + MTK_CCU_SPARE_REG17));
		return -ETIMEDOUT;
	}

	/*5. Get mailbox address in CCU's sram */
	if (ccu->compact_ipc) {
		ccu->mb_compact = (struct mtk_ccu_mailbox_compact *)(uintptr_t)(ccu->ccu_spare_base +
			read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG00));
		ccu->mb = (struct mtk_ccu_mailbox *)ccu->mb_compact;
		ccu->ccu_sram_log_offset = read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG25) & 0xFFFF;
		LOG_DBG_MUST("ccu initial debug mb_ccu2ap: %x, sram_log_offset %x\n",
			read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG00),
			ccu->ccu_sram_log_offset);
	} else {
		ccu->mb = (ccu->ccu_version >= CCU_VER_ISP8L) ?
			(struct mtk_ccu_mailbox *)(uintptr_t)(ccu->ccu_exch_base +
			readl(ccu_spare_base + MTK_CCU_SPARE_REG00)) :
			(struct mtk_ccu_mailbox *)(uintptr_t)(ccu->dmem_base +
			readl(ccu_spare_base + MTK_CCU_SPARE_REG00));
		ccu->ccu_sram_log_offset = readl(ccu_spare_base + MTK_CCU_SPARE_REG25) & 0xFFFF;
		LOG_DBG_MUST("ccu initial debug mb_ccu2ap: %x, sram_log_offset %x\n",
			readl(ccu_spare_base + MTK_CCU_SPARE_REG00),
			ccu->ccu_sram_log_offset);
	}

	mtk_ccu_rproc_ipc_init(ccu);

	mtk_ccu_ipc_register(ccu->pdev, MTK_CCU_MSG_TO_APMCU_FLUSH_LOG,
		mtk_ccu_ipc_log_handle, ccu);
	mtk_ccu_ipc_register(ccu->pdev, MTK_CCU_MSG_TO_APMCU_CCU_ASSERT,
		mtk_ccu_ipc_assert_handle, ccu);
	mtk_ccu_ipc_register(ccu->pdev, MTK_CCU_MSG_TO_APMCU_CCU_WARNING,
		mtk_ccu_ipc_warning_handle, ccu);

	/*tell ccu that driver has initialized mailbox*/
	writel(0, ccu_spare_base + init_status_reg);

	timeout = 10;
	while ((readl(ccu_spare_base + init_status_reg)
		!= CCU_STATUS_INIT_DONE_2) && (timeout >= 0)) {
		udelay(100);
		timeout = timeout - 1;
	}

	if (timeout <= 0) {
		dev_err(ccu->dev, "CCU init timeout 2\n");
		dev_err(ccu->dev, "ccu initial debug info 2: %x\n",
			(ccu->compact_ipc) ?
			read_ccu_info_regd(ccu, MTK_CCU_SPARE_REG17) :
			readl(ccu_spare_base + MTK_CCU_SPARE_REG17));
		return -ETIMEDOUT;
	}

	if (ccu->compact_ipc)
		writel(0, ccu_spare_base + init_status_reg);

	LOG_DBG("-\n");

	return 0;
}

static int mtk_ccu_clk_prepare(struct mtk_ccu *ccu)
{
	int ret;
	int i = 0;
	struct device *dev = ccu->dev;

	LOG_DBG_MUST("Power on CCU0.\n");
	ret = mtk_ccu_get_power(ccu, dev);
	if (ret)
		return ret;

#if defined(CCU1_DEVICE)
	LOG_DBG_MUST("Power on CCU1\n");
	ret = mtk_ccu_get_power(ccu, &ccu->pdev1->dev);
	if (ret)
		goto ERROR_poweroff_ccu;
#endif

	LOG_DBG_MUST("Clock on CCU(%d)\n", ccu->clock_num);
	for (i = 0; (i < ccu->clock_num) && (i < MTK_CCU_CLK_PWR_NUM); ++i) {
		ret = clk_prepare_enable(ccu->ccu_clk_pwr_ctrl[i]);
		if (ret) {
			dev_err(dev, "failed to enable CCU clocks #%d %s\n",
				i, ccu->clock_name[i].name);
			goto ERROR;
		}
	}

	return 0;

ERROR:
	for (--i; i >= 0 ; --i)
		clk_disable_unprepare(ccu->ccu_clk_pwr_ctrl[i]);

#if defined(CCU1_DEVICE)
	mtk_ccu_put_power(ccu, &ccu->pdev1->dev);
ERROR_poweroff_ccu:
#endif
	mtk_ccu_put_power(ccu, dev);

	return ret;
}

static void mtk_ccu_clk_unprepare(struct mtk_ccu *ccu)
{
	int i;

	LOG_DBG_MUST("Clock off CCU(%d)\n", ccu->clock_num);
	for (i = 0; (i < ccu->clock_num) && (i < MTK_CCU_CLK_PWR_NUM); ++i)
		clk_disable_unprepare(ccu->ccu_clk_pwr_ctrl[i]);
	mtk_ccu_put_power(ccu, ccu->dev);
#if defined(CCU1_DEVICE)
	mtk_ccu_put_power(ccu, &ccu->pdev1->dev);
#endif
}

static int mtk_ccu_start(struct rproc *rproc)
{
	struct mtk_ccu *ccu = (struct mtk_ccu *)rproc->priv;
	uint8_t	*ccu_spare_base = (uint8_t *)ccu->ccu_spare_base;
	int ret;
#ifndef REQUEST_IRQ_IN_INIT
	int rc;
#endif

	/*1. Set CCU log memory address from user space*/
	writel((uint32_t)((ccu->log_info[0].mva) >> 8), ccu_spare_base + MTK_CCU_SPARE_REG02);
	writel((uint32_t)((ccu->log_info[1].mva) >> 8), ccu_spare_base + MTK_CCU_SPARE_REG03);
	writel((uint32_t)((ccu->log_info[2].mva) >> 8), ccu_spare_base + MTK_CCU_SPARE_REG07);

#if defined(CCU_SET_MMQOS)
	if (ccu->ccu_version < CCU_VER_ISP7SP) {
		mtk_icc_set_bw(ccu->path_ccuo, MBps_to_icc(20), MBps_to_icc(30));
		mtk_icc_set_bw(ccu->path_ccui, MBps_to_icc(10), MBps_to_icc(30));
		mtk_icc_set_bw(ccu->path_ccug, MBps_to_icc(30), MBps_to_icc(30));
	} else if (ccu->ccu_version >= CCU_VER_ISP8) {
		mtk_icc_set_bw(ccu->path_ccuo, MBps_to_icc(50), 0);
		mtk_icc_set_bw(ccu->path_ccui, MBps_to_icc(40), 0);
	} else {
		mtk_icc_set_bw(ccu->path_ccuo, MBps_to_icc(50), MBps_to_icc(60));
		mtk_icc_set_bw(ccu->path_ccui, MBps_to_icc(40), MBps_to_icc(60));
	}
#endif

	LOG_DBG_MUST("LogBuf_mva[](0x%pad)(0x%x<<8),(0x%pad)(0x%x<<8),(0x%pad)(0x%x<<8)\n",
		&ccu->log_info[0].mva, readl(ccu_spare_base + MTK_CCU_SPARE_REG02),
		&ccu->log_info[1].mva, readl(ccu_spare_base + MTK_CCU_SPARE_REG03),
		&ccu->log_info[2].mva, readl(ccu_spare_base + MTK_CCU_SPARE_REG07));
	ccu->g_LogBufIdx = 0;

	spin_lock(&ccu->ccu_poweron_lock);
	ccu->poweron = true;
	spin_unlock(&ccu->ccu_poweron_lock);

	ccu->disirq = false;

#ifdef REQUEST_IRQ_IN_INIT
	enable_irq(ccu->irq_num);
#else
	rc = devm_request_threaded_irq(ccu->dev, ccu->irq_num, NULL,
		mtk_ccu_isr_handler, IRQF_ONESHOT, "ccu_rproc", ccu);
	if (rc) {
		dev_err(ccu->dev, "fail to request ccu irq(%d, %d)!\n", ccu->irq_num, rc);
		return -ENODEV;
	}
#endif

	/*1. Set CCU run*/
	ret = mtk_ccu_run(ccu);

	if (ret)
		mtk_ccu_stopx(rproc, false);

	return ret;
}

void *mtk_ccu_da_to_va(struct rproc *rproc, u64 da, size_t len, bool *is_iomem)
{
	struct mtk_ccu *ccu = (struct mtk_ccu *)rproc->priv;
	struct device *dev = ccu->dev;
	int offset = 0;
#if !defined(SECURE_CCU)
	struct mtk_ccu_mem_info *bin_mem = mtk_ccu_get_meminfo(ccu, MTK_CCU_DDR);
#endif

	if (da < ccu->ccu_sram_offset) {
		offset = da;
		if (offset >= 0 && (offset + len) <= ccu->ccu_sram_size)
			return ccu->pmem_base + offset;
	} else if (da < MTK_CCU_CACHE_BASE) {
		offset = da - ccu->ccu_sram_offset;
		if (offset >= 0 && (offset + len) <= ccu->ccu_sram_size)
			return ccu->dmem_base + offset;
	}
#if !defined(SECURE_CCU)
	else {
		offset = da - MTK_CCU_CACHE_BASE;
		if (!bin_mem) {
			dev_err(dev, "get binary memory info failed\n");
			return NULL;
		}
		if (offset >= 0 && (offset + len) <= MTK_CCU_CACHE_SIZE)
			return bin_mem->va + offset;
	}
#endif

	dev_err(dev, "failed lookup da(0x%llx) len(0x%zx) to va, offset(%x)\n",
		da, len, offset);
	return NULL;
}
EXPORT_SYMBOL_GPL(mtk_ccu_da_to_va);

static bool mtk_ccu_mb_rx_empty(struct mtk_ccu *ccu)
{
	if ((!ccu) || (!(ccu->mb)))
		return true;

	return (readl(&ccu->mb->rear) == readl(&ccu->mb->front));
}

static int mtk_ccu_stopx(struct rproc *rproc, bool normal_stop)
{
	struct mtk_ccu *ccu = (struct mtk_ccu *)rproc->priv;
	/* struct device *dev = &rproc->dev; */
	int ret, i;
#if defined(SECURE_CCU)
	struct arm_smccc_res res;
#else
	int ccu_reset;
	uint8_t *ccu_base = (uint8_t *)ccu->ccu_base;
#endif

	/* notify CCU to shutdown*/
	if (normal_stop) {
		writel(SYSCTRL_IPC_MAGICNO ^ 3, ccu->ccu_spare_base +
			((ccu->compact_ipc) ? MTK_CCU_SPARE_REG31 : MTK_CCU_SPARE_REG24));
		ret = mtk_ccu_rproc_ipc_send(ccu->pdev, MTK_CCU_FEATURE_SYSCTRL,
			3, NULL, 0);
		if (ret)
			dev_notice(ccu->dev, "stop CCU, IPC failed (%d).\n", ret);
	}

	mtk_ccu_rproc_ipc_uninit(ccu);
	mtk_ccu_sw_hw_reset(ccu);

	ccu->disirq = true;

#if !defined(SECURE_CCU)
	ret = mtk_ccu_deallocate_mem(ccu->dev,
		&ccu->buffer_handle[MTK_CCU_DDR], ccu->smmu_enabled);
	if (ret)
		dev_notice(ccu->dev, "CCU deallocate mem error (%d).\n", ret);
#endif

#if defined(SECURE_CCU)
	if (ccu->compact_ipc)
		writel(CCU_GO_TO_STOP, ccu->ccu_spare_base + MTK_CCU_SPARE_REG30);
	else
		writel(CCU_GO_TO_STOP, ccu->ccu_spare_base + MTK_CCU_SPARE_REG06);
#ifdef CONFIG_ARM64
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u64) CCU_SMC_REQ_STOP,
		0, 0, 0, 0, 0, 0, &res);
#endif
#ifdef CONFIG_ARM_PSCI
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u32) CCU_SMC_REQ_STOP,
		0, 0, 0, 0, 0, 0, &res);
#endif
	if (res.a0 != 0)
		dev_err(ccu->dev, "stop CCU failed (%lu).\n", res.a0);
	else
		LOG_DBG_MUST("stop CCU OK\n");
#else
	ccu_reset = readl(ccu_base + MTK_CCU_REG_RESET);
	writel(ccu_reset|MTK_CCU_HW_RESET_BIT, ccu_base + MTK_CCU_REG_RESET);
#endif

	if (normal_stop) {
		for (i = 0; i <= MTK_CCU_MB_RX_TIMEOUT_SPEC; ++i) {
			if (mtk_ccu_mb_rx_empty(ccu))
				break;
			if (i < MTK_CCU_MB_RX_TIMEOUT_SPEC)
				udelay(10);
		}

		if (i > MTK_CCU_MB_RX_TIMEOUT_SPEC)
			LOG_DBG_MUST("mb_rx_empty timeout.\n");
	}

#if defined(CCU_SET_MMQOS)
	mtk_icc_set_bw(ccu->path_ccuo, MBps_to_icc(0), MBps_to_icc(0));
	mtk_icc_set_bw(ccu->path_ccui, MBps_to_icc(0), MBps_to_icc(0));
	if (ccu->ccu_version < CCU_VER_ISP7SP)
		mtk_icc_set_bw(ccu->path_ccug, MBps_to_icc(0), MBps_to_icc(0));
#endif

#ifdef REQUEST_IRQ_IN_INIT
	spin_lock(&ccu->ccu_irq_lock);
	if (ccu->disirq) {
		ccu->disirq = false;
		spin_unlock(&ccu->ccu_irq_lock);
		disable_irq(ccu->irq_num);
	} else
		spin_unlock(&ccu->ccu_irq_lock);
#else
	devm_free_irq(ccu->dev, ccu->irq_num, ccu);
#endif

	spin_lock(&ccu->ccu_poweron_lock);
	ccu->poweron = false;
	spin_unlock(&ccu->ccu_poweron_lock);

	mtk_ccu_clk_unprepare(ccu);
	return 0;
}

static int mtk_ccu_stop(struct rproc *rproc)
{
	return mtk_ccu_stopx(rproc, true);
}

#if !defined(SECURE_CCU)
static int
ccu_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct mtk_ccu *ccu = rproc->priv;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	int i, ret = 0;
	int timeout = 10;
	unsigned int status;
	const u8 *elf_data = fw->data;
	uint8_t *ccu_base = (uint8_t *)ccu->ccu_base;
	bool is_iomem;

	/* 1. Halt CCU HW before load binary */
	writel(((ccu->ccu_version >= CCU_VER_ISP7SP)) ?
		MTK_CCU_HW_RESET_BIT_ISP7SP : MTK_CCU_HW_RESET_BIT,
		ccu_base + MTK_CCU_REG_RESET);
	udelay(10);

	/* 2. Polling CCU HW status until ready */
	status = readl(ccu_base + MTK_CCU_REG_RESET);
	while ((status & 0x3) != 0x3) {
		status = readl(ccu_base + MTK_CCU_REG_RESET);
		udelay(300);
		if (timeout < 0 && ((status & 0x3) != 0x3)) {
			dev_err(dev, "wait ccu halt before load bin, timeout");
			return -EFAULT;
		}
		timeout--;
	}

	ehdr = (struct elf32_hdr *)elf_data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);
	/* 3. go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		u32 da = phdr->p_paddr;
		u32 memsz = phdr->p_memsz;
		u32 filesz = phdr->p_filesz;
		u32 offset = phdr->p_offset;
		void *ptr;

		if (phdr->p_type != PT_LOAD)
			continue;

		LOG_DBG("phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
			phdr->p_type, da, memsz, filesz);

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, da, memsz, &is_iomem);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz)
			mtk_ccu_memcpy(ptr, elf_data + phdr->p_offset, filesz);

		/*
		 * Zero out remaining memory for this segment.
		 */
		if (memsz > filesz) {
			mtk_ccu_memclr(ptr + ((filesz + 0x3) & (~0x3)),
				memsz - filesz);
		}
	}

	return ret;
}
#endif

static int mtk_ccu_load(struct rproc *rproc, const struct firmware *fw)
{
	struct mtk_ccu *ccu = rproc->priv;
	int ret;
#if defined(SECURE_CCU)
	unsigned int clks;
	struct arm_smccc_res res;
	int rc, i;
	char error_desc[ERROR_DESC_LEN];
#endif

	/*1. prepare CCU's clks & power*/
	ret = mtk_ccu_clk_prepare(ccu);
	if (ret) {
		dev_err(ccu->dev, "failed to prepare ccu clocks\n");
		return ret;
	}

	LOG_DBG_MUST("Load CCU binary start\n");
#if defined(SECURE_CCU)
	if (ccu->compact_ipc)
		writel(CCU_GO_TO_LOAD, ccu->ccu_spare_base + MTK_CCU_SPARE_REG30);
	else
		writel(CCU_GO_TO_LOAD, ccu->ccu_spare_base + MTK_CCU_SPARE_REG06);
#ifdef CONFIG_ARM64
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u64) CCU_SMC_REQ_LOAD,
		0, 0, 0, 0, 0, 0, &res);
#endif
#ifdef CONFIG_ARM_PSCI
	arm_smccc_smc(MTK_SIP_KERNEL_CCU_CONTROL, (u32) CCU_SMC_REQ_LOAD,
		0, 0, 0, 0, 0, 0, &res);
#endif
	ret = (int)(res.a0);
	if (ret != 0) {
		for (i = 0, clks = 0; i < ccu->clock_num; ++i)
			if (__clk_is_enabled(ccu->ccu_clk_pwr_ctrl[i]))
				clks |= (1 << i);
		rc = snprintf(error_desc, ERROR_DESC_LEN,
			"load CCU binary fail(%d,0x%lx,0x%lx,0x%lx), clock: 0x%x",
			ret, res.a1, res.a2, res.a3, clks);
		if (rc < 0) {
			strncpy(error_desc, "load CCU binary fail", ERROR_DESC_LEN);
			error_desc[ERROR_DESC_LEN - 1] = 0;
		}
		dev_err(ccu->dev, "%s\n", error_desc);
#if IS_ENABLED(CONFIG_MTK_AEE_FEATURE)
		aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_DEFAULT, "CCU",
			       error_desc);
#else
		WARN_ON(1);
#endif
		goto ccu_load_err;
	} else
		LOG_DBG_MUST("load CCU binary OK\n");
#else
	/*2. allocate CCU's dram memory if needed*/
	ccu->buffer_handle[MTK_CCU_DDR].meminfo.size = MTK_CCU_CACHE_SIZE;
	ccu->buffer_handle[MTK_CCU_DDR].meminfo.cached	= false;
	ret = mtk_ccu_allocate_mem(ccu->dev, &ccu->buffer_handle[MTK_CCU_DDR],
		ccu->smmu_enabled);
	if (ret) {
		dev_err(ccu->dev, "alloc mem failed\n");
		goto ccu_load_err;
	}

	/*3. load binary*/
	ret = ccu_elf_load_segments(rproc, fw);
	if (ret) {
		mtk_ccu_deallocate_mem(ccu->dev, &ccu->buffer_handle[MTK_CCU_DDR],
			ccu->smmu_enabled);
		goto ccu_load_err;
	}
#endif

	return ret;

ccu_load_err:
	mtk_ccu_clk_unprepare(ccu);
	return ret;
}

static int
mtk_ccu_sanity_check(struct rproc *rproc, const struct firmware *fw)
{
#if !defined(SECURE_CCU)
	struct mtk_ccu *ccu = rproc->priv;
	const char *name = rproc->firmware;
	struct elf32_hdr *ehdr;
	char class;

	if (!fw) {
		dev_err(ccu->dev, "failed to load %s\n", name);
		return -EINVAL;
	}

	if (fw->size < sizeof(struct elf32_hdr)) {
		dev_err(ccu->dev, "Image is too small\n");
		return -EINVAL;
	}

	ehdr = (struct elf32_hdr *)fw->data;

	/* We only support ELF32 at this point */
	class = ehdr->e_ident[EI_CLASS];
	if (class != ELFCLASS32) {
		dev_err(ccu->dev, "Unsupported class: %d\n", class);
		return -EINVAL;
	}

	/* We assume the firmware has the same endianness as the host */
# ifdef __LITTLE_ENDIAN
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
# else /* BIG ENDIAN */
	if (ehdr->e_ident[EI_DATA] != ELFDATA2MSB) {
# endif
		dev_err(ccu->dev, "Unsupported firmware endianness\n");
		return -EINVAL;
	}

	if (fw->size < ehdr->e_shoff + sizeof(struct elf32_shdr)) {
		dev_err(ccu->dev, "Image is too small\n");
		return -EINVAL;
	}

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		dev_err(ccu->dev, "Image is corrupted (bad magic)\n");
		return -EINVAL;
	}

	if (ehdr->e_phnum == 0) {
		dev_err(ccu->dev, "No loadable segments\n");
		return -EINVAL;
	}

	if (ehdr->e_phoff > fw->size) {
		dev_err(ccu->dev, "Firmware size is too small\n");
		return -EINVAL;
	}
#endif

	return 0;
}

#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
void get_ccu_mrdump_buffer(unsigned long *vaddr, unsigned long *size)
{
	if ((!dev_ccu) || (!dev_ccu->mrdump_buf))
		return;

	*((uint32_t *)(dev_ccu->mrdump_buf)) = LOG_ENDEND;
	*((uint32_t *)(dev_ccu->mrdump_buf +
		(MTK_CCU_SRAM_LOG_BUF_SIZE / 2))) = LOG_ENDEND;

	if (spin_trylock(&dev_ccu->ccu_poweron_lock)) {
		if (dev_ccu->poweron) {
			memcpy(dev_ccu->mrdump_buf,
				dev_ccu->dmem_base + dev_ccu->ccu_sram_log_offset,
				MTK_CCU_SRAM_LOG_BUF_SIZE);
			memcpy(dev_ccu->mrdump_buf + MTK_CCU_SRAM_LOG_BUF_SIZE,
				dev_ccu->ccu_base,
				MTK_CCU_REG_LOG_BUF_SIZE - MTK_CCU_EXTRA_REG_LOG_BUF_SIZE);
			memcpy(dev_ccu->mrdump_buf + MTK_CCU_MRDUMP_SRAM_BUF_SIZE
				- MTK_CCU_EXTRA_REG_LOG_BUF_SIZE,
				(dev_ccu->ccu_version < CCU_VER_ISP8) ?
				dev_ccu->ccu_base + MTK_CCU_EXTRA_REG_OFFSET :
				dev_ccu->ccu_exch_base,
				MTK_CCU_EXTRA_REG_LOG_BUF_SIZE);
		}

		spin_unlock(&dev_ccu->ccu_poweron_lock);
	}

	memcpy(dev_ccu->mrdump_buf + MTK_CCU_MRDUMP_SRAM_BUF_SIZE,
		dev_ccu->log_info[0].va, MTK_CCU_MRDUMP_BUF_DRAM_SIZE);
	memcpy(dev_ccu->mrdump_buf + MTK_CCU_MRDUMP_SRAM_BUF_SIZE + MTK_CCU_MRDUMP_BUF_DRAM_SIZE,
		dev_ccu->log_info[1].va, MTK_CCU_MRDUMP_BUF_DRAM_SIZE);

	*vaddr = (unsigned long)dev_ccu->mrdump_buf;
	*size = MTK_CCU_MRDUMP_BUF_SIZE;
}
#endif

static const struct rproc_ops ccu_ops = {
	.start = mtk_ccu_start,
	.stop  = mtk_ccu_stop,
	.da_to_va = mtk_ccu_da_to_va,
	.load = mtk_ccu_load,
	.sanity_check = mtk_ccu_sanity_check,
};

static int mtk_ccu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct mtk_ccu *ccu;
	struct rproc *rproc;
	struct device_node *smi_node;
	struct platform_device *smi_pdev;
	uint32_t clki;
	int ret = 0;
	uint32_t phy_addr;
	uint32_t phy_size;
	static struct lock_class_key ccu_lock_key;
	const char *ccu_lock_name = "ccu_lock_class";
	struct device_link *link;
	struct device_node *node_cammainpwr;
	phandle phandle_cammainpwr;
#if defined(CCU1_DEVICE)
	struct device_node *node1;
	phandle ccu_rproc1_phandle;
#endif

	rproc = rproc_alloc(dev, node->name, &ccu_ops,
		CCU_FW_NAME, sizeof(*ccu));
	if ((!rproc) || (!rproc->priv)) {
		dev_err(dev, "rproc or rproc->priv is NULL.\n");
		return -EINVAL;
	}
	lockdep_set_class_and_name(&rproc->lock, &ccu_lock_key, ccu_lock_name);
	ccu = (struct mtk_ccu *)rproc->priv;
	ccu->pdev = pdev;
	ccu->dev = &pdev->dev;
	ccu->rproc = rproc;
	ccu->log_level = LOG_DEFAULT_LEVEL;
	ccu->log_taglevel = LOG_DEFAULT_TAG;
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	ccu_rproc_ipifuzz = rproc;	/* IPIFuzz */
#endif

	platform_set_drvdata(pdev, ccu);
	ret = mtk_ccu_read_platform_info_from_dt(node, ccu);
	if (ret) {
		dev_err(ccu->dev, "Get CCU DT info fail.\n");
		return ret;
	}

	/*remap ccu_base*/
	phy_addr = ccu->ccu_hw_base;
	phy_size = ccu->ccu_hw_size;
	ccu->ccu_base = devm_ioremap(dev, phy_addr, phy_size);
	LOG_DBG_MUST("ccu_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
	LOG_DBG_MUST("ccu_base va: 0x%llx\n", (uint64_t)ccu->ccu_base);

	/* remap ccu_exch_base */
	if (ccu->ccu_version < CCU_VER_ISP8) {
		phy_addr = ccu->ccu_hw_base + CCU_EXCH_OFFSET;
		phy_size = CCU_EXCH_SIZE;
		ccu->ccu_exch_base = ccu->ccu_base + CCU_EXCH_OFFSET;
		ccu->ccu_spare_base = ccu->ccu_exch_base + 0x20;
	} else {
		phy_addr = ccu->ccu_exch_pa;
		phy_size = (ccu->ccu_version >= CCU_VER_ISP8L) ? CCU_EXCH_SIZE_IPC : CCU_EXCH_SIZE;
		ccu->ccu_exch_base = devm_ioremap(dev, phy_addr, phy_size);
		ccu->ccu_spare_base = ccu->ccu_exch_base;
	}
	LOG_DBG_MUST("ccu_exch_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
	LOG_DBG_MUST("ccu_exch_base va: 0x%llx\n", (uint64_t)ccu->ccu_exch_base);

	/*remap dmem_base*/
	phy_addr = (ccu->ccu_hw_base & MTK_CCU_BASE_MASK) + ccu->ccu_sram_offset;
	phy_size = ccu->ccu_sram_size;
	ccu->dmem_base = devm_ioremap(dev, phy_addr, phy_size);
	LOG_DBG_MUST("dmem_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
	LOG_DBG_MUST("dmem_base va: 0x%llx\n", (uint64_t)ccu->dmem_base);

	/*remap pmem_base*/
	phy_addr = ccu->ccu_hw_base & MTK_CCU_BASE_MASK;
	phy_size = ccu->ccu_sram_size;
	ccu->pmem_base = devm_ioremap(dev, phy_addr, phy_size);
	LOG_DBG_MUST("pmem_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
	LOG_DBG_MUST("pmem_base va: 0x%llx\n", (uint64_t)ccu->pmem_base);

	/*remap spm_base*/
	if (ccu->ccu_version >= CCU_VER_ISP7SP) {
		phy_addr = (ccu->ccu_version == CCU_VER_ISP8) ? SPM_BASE_ISP8 : SPM_BASE;
		phy_size = SPM_SIZE;
		ccu->spm_base = devm_ioremap(dev, phy_addr, phy_size);
		LOG_DBG_MUST("spm_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
		LOG_DBG_MUST("spm_base va: 0x%llx\n", (uint64_t)ccu->spm_base);
	}

	/*remap mmpc_base*/
	if (ccu->ccu_version == CCU_VER_ISP8) {
		phy_addr = MMPC_BASE;
		phy_size = MMPC_SIZE;
		ccu->mmpc_base = devm_ioremap(dev, phy_addr, phy_size);
		LOG_DBG_MUST("mmpc_base pa: 0x%x, size: 0x%x\n", phy_addr, phy_size);
		LOG_DBG_MUST("mmpc_base va: 0x%llx\n", (uint64_t)ccu->mmpc_base);
	}

	/* Get other power node if needed. */
	if (((ccu->ccu_version >= CCU_VER_ISP7SP) && (ccu->ccu_version <= CCU_VER_ISP7SPL))
		|| (ccu->ccu_version == CCU_VER_ISP8L)) {
		ret = of_property_read_u32(node, "mediatek,cammainpwr",
			&phandle_cammainpwr);
		node_cammainpwr = of_find_node_by_phandle(phandle_cammainpwr);
		if (node_cammainpwr)
			ccu->pdev_cammainpwr = of_find_device_by_node(node_cammainpwr);
		if (WARN_ON(!ccu->pdev_cammainpwr))
			dev_err(ccu->dev, "failed to get ccu cammainpwr pdev\n");
		else
			ccu->dev_cammainpwr = &ccu->pdev_cammainpwr->dev;
		of_node_put(node_cammainpwr);
	}
	/* get Clock control from device tree.  */
	/*
	 * SMI definition is usually not ready at bring-up stage of new platform.
	 * Continue initialization if SMI is not defined.
	 */
	smi_node = of_parse_phandle(node, "mediatek,larbs", 0);
	if (!smi_node) {
		dev_err(ccu->dev, "get smi larb from DTS fail!\n");
		/* return -ENODEV; */
	} else {
		smi_pdev = of_find_device_by_node(smi_node);
		if (WARN_ON(!smi_pdev)) {
			of_node_put(smi_node);
			return -ENODEV;
		}
		of_node_put(smi_node);

		link = device_link_add(ccu->dev, &smi_pdev->dev, DL_FLAG_PM_RUNTIME |
					DL_FLAG_STATELESS);
		if (!link) {
			dev_notice(ccu->dev, "ccu_rproc Unable to link SMI LARB\n");
			return -ENODEV;
		}
	}
	pm_runtime_enable(ccu->dev);

	ccu->clock_num = 0;
	if (ccu->ccu_version == CCU_VER_ISP8L)
		ccu->clock_name = ccu_clk_name_isp8l;
	else if (ccu->ccu_version == CCU_VER_ISP8)
		ccu->clock_name = ccu_clk_name_isp8;
	else if (ccu->ccu_version == CCU_VER_ISP7SPL)
		ccu->clock_name = ccu_clk_name_isp7spl;
	else if (ccu->ccu_version == CCU_VER_ISP7SP)
		ccu->clock_name = ccu_clk_name_isp7sp;
	else if (ccu->ccu_version == CCU_VER_ISP7S)
		ccu->clock_name = ccu_clk_name_isp7s;
	else
		ccu->clock_name = ccu_clk_name_isp71;

	for (clki = 0; clki < MTK_CCU_CLK_PWR_NUM; ++clki) {
		if (!ccu->clock_name[clki].enable)
			break;
		ccu->ccu_clk_pwr_ctrl[clki] = devm_clk_get(ccu->dev,
			ccu->clock_name[clki].name);
		if (IS_ERR(ccu->ccu_clk_pwr_ctrl[clki])) {
			dev_err(ccu->dev, "Get %s fail.\n", ccu->clock_name[clki].name);
			return PTR_ERR(ccu->ccu_clk_pwr_ctrl[clki]);
		}
	}

	if (clki >= MTK_CCU_CLK_PWR_NUM) {
		dev_err(ccu->dev, "ccu_clk_pwr_ctrl[] too small(%d).\n", clki);
		return -EINVAL;
	}

	ccu->clock_num = clki;
	LOG_DBG_MUST("CCU got %d clocks\n", clki);

#if defined(CCU_SET_MMQOS)
	if (ccu->ccu_version < CCU_VER_ISP7SP)
		ccu->path_ccug = of_mtk_icc_get(ccu->dev, "ccu_g");
	ccu->path_ccuo = of_mtk_icc_get(ccu->dev, "ccu_o");
	ccu->path_ccui = of_mtk_icc_get(ccu->dev, "ccu_i");
#endif

#if defined(CCU1_DEVICE)
	ret = of_property_read_u32(node, "mediatek,ccu_rproc1",
		&ccu_rproc1_phandle);
	node1 = of_find_node_by_phandle(ccu_rproc1_phandle);
	if (node1)
		ccu->pdev1 = of_find_device_by_node(node1);
	if (WARN_ON(!ccu->pdev1)) {
		dev_err(ccu->dev, "failed to get ccu rproc1 pdev\n");
		of_node_put(node1);
	}
#endif

	/* get irq from device irq*/
	ccu->irq_num = irq_of_parse_and_map(node, 0);

	LOG_DBG_MUST("ccu_probe irq_num: %d\n", ccu->irq_num);

	/*prepare mutex & log's waitqueuehead*/
	mutex_init(&ccu->ipc_desc_lock);
	spin_lock_init(&ccu->ipc_send_lock);
	spin_lock_init(&ccu->ccu_poweron_lock);
	spin_lock_init(&ccu->ccu_irq_lock);
	init_waitqueue_head(&ccu->WaitQueueHead);

#ifdef REQUEST_IRQ_IN_INIT
	ccu->disirq = true;
	ret = devm_request_threaded_irq(ccu->dev, ccu->irq_num, NULL,
		mtk_ccu_isr_handler, IRQF_ONESHOT, "ccu_rproc", ccu);
	if (ret) {
		dev_err(ccu->dev, "fail to request ccu irq(%d,%d)!\n", ccu->irq_num, ret);
		return -ENODEV;
	}
	spin_lock(&ccu->ccu_irq_lock);
	if (ccu->disirq) {
		ccu->disirq = false;
		spin_unlock(&ccu->ccu_irq_lock);
		disable_irq(ccu->irq_num);
	} else
		spin_unlock(&ccu->ccu_irq_lock);
#endif

#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	/*register char dev for log ioctl*/
	ret = mtk_ccu_reg_chardev(ccu);
	if (ret)
		dev_err(ccu->dev, "failed to regist char dev");
#endif

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34));

	ccu->smmu_enabled = smmu_v3_enabled();
	LOG_DBG_MUST("smmu_enabled : %d\n", (ccu->smmu_enabled) ? 1 : 0);

	ccu->ext_buf.meminfo.size = MTK_CCU_DRAM_LOG_DBG_BUF_SIZE;
	ccu->ext_buf.meminfo.cached = false;
	ret = mtk_ccu_allocate_mem(ccu->dev, &ccu->ext_buf, ccu->smmu_enabled);
	if (ret) {
		dev_err(ccu->dev, "alloc mem failed\n");
		return ret;
	}

	mtk_ccu_set_log_memory_address(ccu);

	rproc->auto_boot = false;

#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
	ccu->mrdump_buf = kmalloc(MTK_CCU_MRDUMP_BUF_SIZE, GFP_ATOMIC);
	if (!ccu->mrdump_buf) {
		mtk_ccu_deallocate_mem(ccu->dev, &ccu->ext_buf, ccu->smmu_enabled);
		return -EINVAL;
	}

	dev_ccu = ccu;
	mrdump_set_extra_dump(AEE_EXTRA_FILE_CCU, get_ccu_mrdump_buffer);
#endif

	ret = rproc_add(rproc);
	return ret;
}

static int mtk_ccu_remove(struct platform_device *pdev)
{
	struct mtk_ccu *ccu = platform_get_drvdata(pdev);

	/*
	 * WARNING:
	 * With mrdump, remove CCU module will cause access violation
	 * at KE/SystemAPI.
	 */
#if IS_ENABLED(CONFIG_MTK_AEE_IPANIC)
	mrdump_set_extra_dump(AEE_EXTRA_FILE_CCU, NULL);
	kfree(ccu->mrdump_buf);
#endif
	mtk_ccu_deallocate_mem(ccu->dev, &ccu->ext_buf, ccu->smmu_enabled);
#if defined(REQUEST_IRQ_IN_INIT)
	devm_free_irq(ccu->dev, ccu->irq_num, ccu);
#endif
	rproc_del(ccu->rproc);
	rproc_free(ccu->rproc);
	if (((ccu->ccu_version >= CCU_VER_ISP7SP) && (ccu->ccu_version <= CCU_VER_ISP7SPL))
		|| (ccu->ccu_version == CCU_VER_ISP8L))
		pm_runtime_disable(ccu->dev_cammainpwr);
	pm_runtime_disable(ccu->dev);
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
	mtk_ccu_unreg_chardev(ccu);
#endif
	return 0;
}

static int mtk_ccu_read_platform_info_from_dt(struct device_node
		*node, struct mtk_ccu *ccu)
{
	uint32_t reg[4] = {0, 0, 0, 0};
	int ret = 0;

	ret = of_property_read_u32_array(node, "reg", reg, 4);

	ccu->ccu_hw_base = reg[1];
	ccu->ccu_hw_size = reg[3];

	ret = of_property_read_u32(node, "ccu_version", reg);
	ccu->ccu_version = (ret < 0) ? CCU_VER_ISP71 : reg[0];

	ret = of_property_read_u32(node, "ccu_sramSize", reg);
	ccu->ccu_sram_size = (ret < 0) ? MTK_CCU_PMEM_SIZE : reg[0];

	ret = of_property_read_u32(node, "ccu_sramOffset", reg);
	ccu->ccu_sram_offset = (ret < 0) ?
		((ccu->ccu_version >= CCU_VER_ISP7SP) ?
		MTK_CCU_CORE_DMEM_BASE_ISP7SP : MTK_CCU_CORE_DMEM_BASE) : reg[0];

	if (ccu->ccu_version >= CCU_VER_ISP7SP) {
		ret = of_property_read_u32(node, "ccu-sramcon-offset", reg);
		ccu->ccu_sram_con_offset = (ret < 0) ? CCU_SLEEP_SRAM_CON : reg[0];
	}

	/* mt6899, SPM enable CCU resource mask at system bootup. */
	if (ccu->ccu_version == CCU_VER_ISP8) {
		ret = of_property_read_u32(node, "ccu-resource-offset", reg);
		ccu->ccu_resource_offset = (ret < 0) ? CCU_RESOURCE_OFFSET : reg[0];
		ret = of_property_read_u32(node, "ccu-resource-bits", reg);
		ccu->ccu_resource_bits = (ret < 0) ? CCU_RESOURCE_BITS : reg[0];
	}

	ret = of_property_read_u32(node, "ccu-exch-base", reg);
	ccu->ccu_exch_pa = (ret < 0) ? ccu->ccu_hw_base + CCU_EXCH_OFFSET : reg[0];

	ret = of_property_read_u32(node, "compact-ipc", reg);
	ccu->compact_ipc = (ret < 0) ? false : (reg[0] != 0);
	ccu_mailbox_max = (ccu->compact_ipc) ?
		MTK_CCU_MAILBOX_QUEUE_COMPACT_SIZE : MTK_CCU_MAILBOX_QUEUE_SIZE;

	ret = of_property_read_u32(node, "systick-freq", reg);
	ccu->systick_freq = (ret < 0) ? SYSTICK_FREQ_LEGACY : reg[0];

	return 0;
}

static int mtk_ccu_cam_main_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_enable(dev);
	return 0;
}

static int mtk_ccu_cam_main_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);
	return 0;
}

#if defined(CCU1_DEVICE)
static int mtk_ccu1_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34));

	pm_runtime_enable(dev);
	return 0;
}

static int mtk_ccu1_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	pm_runtime_disable(dev);
	return 0;
}
#endif

static int mtk_ccu_get_power(struct mtk_ccu *ccu, struct device *dev)
{
	uint8_t *sram_con, *resource_con;
	int rc, ret = pm_runtime_get_sync(dev);

	if (ret < 0) {
		dev_err(dev, "pm_runtime_get_sync failed %d", ret);
		return ret;
	}

	if (((ccu->ccu_version >= CCU_VER_ISP7SP) && (ccu->ccu_version <= CCU_VER_ISP7SPL))
		|| (ccu->ccu_version == CCU_VER_ISP8L)) {
		rc = pm_runtime_get_sync(ccu->dev_cammainpwr);
		LOG_DBG_MUST("CCU power-on cammainpwr %d\n", rc);
		ccu->cammainpwr_powered = (rc >= 0);
	}

	if (ccu->ccu_version == CCU_VER_ISP8) {
		sram_con = ((uint8_t *)ccu->mmpc_base)+ccu->ccu_sram_con_offset;
		writel(readl(sram_con) & ~CCU_SLEEP_SRAM_PDN, sram_con);
	} else if (ccu->ccu_version >= CCU_VER_ISP7SP) {
		sram_con = ((uint8_t *)ccu->spm_base)+ccu->ccu_sram_con_offset;
		writel(readl(sram_con) & ~CCU_SLEEP_SRAM_PDN, sram_con);
	}

	/* mt6899 CCU resource mask enabled on SPM init. */
	if (ccu->ccu_version == CCU_VER_ISP8) {
		resource_con = ((uint8_t *)ccu->spm_base)+ccu->ccu_resource_offset;
		writel(readl(resource_con) | ccu->ccu_resource_bits, resource_con);
	}

	return ret;
}

static void mtk_ccu_put_power(struct mtk_ccu *ccu, struct device *dev)
{
	uint8_t *sram_con, *resource_con;
	int ret;

	/* mt6899 CCU resource mask enabled on SPM init. */
	if (ccu->ccu_version == CCU_VER_ISP8) {
		resource_con = ((uint8_t *)ccu->spm_base)+ccu->ccu_resource_offset;
		writel(readl(resource_con) & ~ccu->ccu_resource_bits, resource_con);
	}

	if (ccu->ccu_version == CCU_VER_ISP8) {
		sram_con = ((uint8_t *)ccu->mmpc_base)+ccu->ccu_sram_con_offset;
		writel(readl(sram_con) | CCU_SLEEP_SRAM_PDN, sram_con);
	} else if (ccu->ccu_version >= CCU_VER_ISP7SP) {
		sram_con = ((uint8_t *)ccu->spm_base)+ccu->ccu_sram_con_offset;
		writel(readl(sram_con) | CCU_SLEEP_SRAM_PDN, sram_con);
	}

	if ((((ccu->ccu_version >= CCU_VER_ISP7SP) && (ccu->ccu_version <= CCU_VER_ISP7SPL))
		|| (ccu->ccu_version == CCU_VER_ISP8L)) && (ccu->cammainpwr_powered)) {
		ret = pm_runtime_put_sync(ccu->dev_cammainpwr);
		if (ret < 0)
			dev_err(dev, "pm_runtime_put_sync cammainpwr failed %d", ret);
		ccu->cammainpwr_powered = false;
	}

	ret = pm_runtime_put_sync(dev);
	if (ret < 0)
		dev_err(dev, "pm_runtime_put_sync failed %d", ret);
}

static const struct of_device_id mtk_ccu_of_ids[] = {
	{.compatible = "mediatek,ccu_rproc", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ccu_of_ids);

static struct platform_driver ccu_rproc_driver = {
	.probe = mtk_ccu_probe,
	.remove = mtk_ccu_remove,
	.driver = {
		.name = MTK_CCU_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_ccu_of_ids),
	},
};

static const struct of_device_id mtk_ccu_cam_main_of_ids[] = {
	{.compatible = "mediatek,ccucammain", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ccu_cam_main_of_ids);

static struct platform_driver ccu_cam_main_driver = {
	.probe = mtk_ccu_cam_main_probe,
	.remove = mtk_ccu_cam_main_remove,
	.driver = {
		.name = MTK_CCU_CAM_MAIN_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_ccu_cam_main_of_ids),
	},
};

#if defined(CCU1_DEVICE)
static const struct of_device_id mtk_ccu1_of_ids[] = {
	{.compatible = "mediatek,ccu_rproc1", },
	{},
};
MODULE_DEVICE_TABLE(of, mtk_ccu1_of_ids);

static struct platform_driver ccu_rproc1_driver = {
	.probe = mtk_ccu1_probe,
	.remove = mtk_ccu1_remove,
	.driver = {
		.name = MTK_CCU1_DEV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(mtk_ccu1_of_ids),
	},
};
#endif

static int __init ccu_init(void)
{
	platform_driver_register(&ccu_cam_main_driver);
	platform_driver_register(&ccu_rproc_driver);
#if defined(CCU1_DEVICE)
	platform_driver_register(&ccu_rproc1_driver);
#endif
	return 0;
}

static void __exit ccu_exit(void)
{
	platform_driver_unregister(&ccu_cam_main_driver);
	platform_driver_unregister(&ccu_rproc_driver);
#if defined(CCU1_DEVICE)
	platform_driver_unregister(&ccu_rproc1_driver);
#endif
}

module_init(ccu_init);
module_exit(ccu_exit);

MODULE_IMPORT_NS(DMA_BUF);

MODULE_DESCRIPTION("MTK CCU Rproc Driver");
MODULE_LICENSE("GPL v2");
