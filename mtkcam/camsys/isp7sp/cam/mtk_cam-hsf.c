// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Louis Kuo <louis.kuo@mediatek.com>
 */
#include <linux/component.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>

#include <linux/platform_data/mtk_ccd.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <linux/version.h>

#include <media/videobuf2-v4l2.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-mc.h>
#include <media/v4l2-subdev.h>
#include "mtk_cam.h"
#include "mtk_cam-engine.h"
#include "mtk_heap.h"
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/arm-smccc.h>
#include "iommu_debug.h"

struct dma_buf *mtk_cam_dmabuf_alloc(struct mtk_cam_ctx *ctx, unsigned int size)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct dma_heap *heap_type;
	struct dma_buf *dbuf;

	heap_type = dma_heap_find("mtk_prot_region");

	if (!heap_type) {
		dev_info(cam->dev, "heap type error\n");
		return NULL;
	}

	dbuf = dma_heap_buffer_alloc(heap_type, size,
		O_CLOEXEC | O_RDWR, DMA_HEAP_VALID_HEAP_FLAGS);

	dma_heap_put(heap_type);
	if (IS_ERR(dbuf)) {
		dev_info(cam->dev, "region-based hsf buffer allocation fail\n");
		return NULL;
	}
	dev_info(cam->dev, "%s done dbuf = 0x%pad\n", __func__, (unsigned int *)dbuf);
	return dbuf;
}

int mtk_cam_dmabuf_get_iova(struct mtk_cam_ctx *ctx,
	struct device *dev, struct mtk_cam_dma_map *dmap)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct dma_buf_attachment *attach;
	struct sg_table *table;
#if IS_ENABLED(CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM)
	uint64_t handle;

	handle = dmabuf_to_secure_handle(dmap->dbuf);

	dev_info(cam->dev, "get dbuf dmabuf_to_secure_handle done\n");
	if (!handle) {
		dev_info(cam->dev, "get hsf handle failed\n");
		return -1;
	}
#endif
	attach = dma_buf_attach(dmap->dbuf, dev);
	dev_info(cam->dev, "get dbuf dma_buf_attach done(%s)\n",
		smmu_v3_enabled() ? "SMMU" : "IOMMU");
	if (IS_ERR(attach)) {
		dev_info(cam->dev, "dma_buf_attach failed\n");
		return -1;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	table = dma_buf_map_attachment_unlocked(attach, DMA_TO_DEVICE);
#else
	table = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
#endif
	dev_info(cam->dev, "get dbuf dma_buf_map_attachment done\n");

	if (IS_ERR(table)) {
		dev_info(cam->dev, "dma_buf_map_attachment failed\n");
		dma_buf_detach(dmap->dbuf, attach);
		return -1;
	}
#if IS_ENABLED(CONFIG_MTK_TRUSTED_MEMORY_SUBSYSTEM)
	dmap->hsf_handle = handle;
#endif
	dmap->attach = attach;
	dmap->table = table;
	dmap->dma_addr = sg_dma_address(table->sgl);
	dev_info(cam->dev, "dma_addr:0x%llx\n", dmap->dma_addr);
	return 0;
}

void mtk_cam_dmabuf_free_iova(struct mtk_cam_ctx *ctx, struct mtk_cam_dma_map *dmap)
{
	if (dmap->attach == NULL || dmap->table == NULL) {
		//dev_info(cam->dev, "dmap is null\n");
		return;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	dma_buf_unmap_attachment_unlocked(dmap->attach, dmap->table, DMA_TO_DEVICE);
#else
	dma_buf_unmap_attachment(dmap->attach, dmap->table, DMA_TO_DEVICE);
#endif
	dma_buf_detach(dmap->dbuf, dmap->attach);
	dma_heap_buffer_free(dmap->dbuf);
}

#ifdef USING_CCU
static unsigned int get_ccu_device(struct mtk_cam_hsf_ctrl *handle_inst)
{
	int ret = 0;
	phandle handle;
	struct device_node *node = NULL, *rproc_np = NULL;

    /* clear data */
	handle_inst->ccu_pdev = NULL;
	handle_inst->ccu_handle = 0;
	node = of_find_compatible_node(NULL, NULL, "mediatek,camera-camsys-ccu");
	if (!node) {
		pr_info("error: find mediatek,camera-camsys-ccu failed!!!\n");
		return 1;
	}
	ret = of_property_read_u32(node, "mediatek,ccu-rproc", &handle);
	if (ret < 0) {
		pr_info("error: ccu-rproc of_property_read_u32:%d\n", ret);
		return ret;
	}
	rproc_np = of_find_node_by_phandle(handle);
	if (rproc_np) {
		handle_inst->ccu_pdev = of_find_device_by_node(rproc_np);
		pr_info("handle_inst.ccu_pdev = 0x%pad\n", handle_inst->ccu_pdev);
		if (!handle_inst->ccu_pdev) {
			pr_info("error: failed to find ccu rproc pdev\n");
			handle_inst->ccu_pdev = NULL;
			return ret;
		}
		/* keep for rproc_get_by_phandle() using */
		handle_inst->ccu_handle = handle;
		pr_info("get ccu proc pdev successfully\n");
	}
	return ret;
}

static void mtk_cam_power_on_ccu(struct mtk_cam_hsf_ctrl *handle_inst, unsigned int flag)
{
	int ret = 0;
	struct rproc *ccu_rproc = NULL;

	if (!handle_inst->ccu_pdev)
		get_ccu_device(handle_inst);

	ccu_rproc = rproc_get_by_phandle(handle_inst->ccu_handle);
	if (!ccu_rproc) {
		pr_info("error: get ccu rproc failed!\n");
		return;
	}

	if (flag > 0) {
	/* boot up ccu */
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		ret = rproc_bootx(ccu_rproc, RPROC_UID_SC);
#else
		ret = rproc_boot(ccu_rproc);
#endif
		if (ret != 0) {
			pr_info("error: ccu rproc_boot failed!\n");
			return;
		}

		handle_inst->power_on_cnt++;
		pr_info("camsys power on ccu, cnt:%d\n", handle_inst->power_on_cnt);
	} else {
		/* shutdown ccu */
#if IS_ENABLED(CONFIG_MTK_CCU_DEBUG)
		rproc_shutdownx(ccu_rproc, RPROC_UID_SC);
#else
		rproc_shutdown(ccu_rproc);
#endif

		handle_inst->power_on_cnt--;
		pr_info("camsys power off ccu, cnt:%d\n", handle_inst->power_on_cnt);
	}
}
#endif

void ccu_stream_on(struct mtk_cam_ctx *ctx, int on)
{
	int ret = 0;
	struct mtk_cam_hsf_ctrl *hsf_config = NULL;
	struct mtk_cam_hsf_info *share_buf = NULL;
	struct raw_info pData;
	int raw_id, raw_engine;

#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time;

	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

	raw_engine = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);
	raw_id = find_first_bit_set(raw_engine);
	pr_info("%s used_engine:%d raw_engine:%d raw_id:%d\n", __func__, ctx->used_engine,
		raw_engine, raw_id);
	if (raw_id < 0) {
		pr_info("%s error: raw_id is not found\n", __func__);
		return;
	}

	hsf_config = ctx->hsf;
	if (hsf_config == NULL) {
		pr_info("%s error: hsf_config is NULL pointer check hsf initial\n", __func__);
		return;
	}

	share_buf = hsf_config->share_buf;
	if (share_buf == NULL) {
		pr_info("%s hsf config fail share buffer not alloc\n", __func__);
		return;
	}

	pData.tg_idx = raw_id;
	pData.vf_en = on;
	pData.chunk_iova = share_buf->chunk_iova;
	pData.hsf_status = 0;

#ifdef USING_CCU
	ret = mtk_ccu_rproc_ipc_send(
	hsf_config->ccu_pdev,
	MTK_CCU_FEATURE_CAMSYS,
	MSG_TO_CCU_STREAM_ON,
	(void *)&pData, sizeof(struct raw_info));
#endif

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	pr_info("%s time %d us\n", __func__, ms);
#endif

	if (ret != 0 || pData.hsf_status == 0)
		pr_info("%s TG(%d) VF(%d) fail, hsf_status:(%d)\n",
			__func__, pData.tg_idx, on, pData.hsf_status);
	else
		pr_info("%s TG(%d) VF(%d) success\n", __func__, pData.tg_idx, on);
}

void ccu_hsf_config(struct mtk_cam_ctx *ctx, unsigned int En)
{
	int ret = 0;
	struct raw_info pData;
	struct mtk_cam_hsf_ctrl *hsf_config = NULL;
	struct mtk_cam_hsf_info *share_buf = NULL;
#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time;

	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif
	if (ctx == NULL) {
		pr_info("%s error: ctx is NULL pointer check initial\n", __func__);
		return;
	}
	hsf_config = ctx->hsf;

	if (hsf_config == NULL) {
		pr_info("%s error: hsf_config is NULL pointer check hsf initial\n", __func__);
		return;
	}

	share_buf = hsf_config->share_buf;
	pData.tg_idx = share_buf->cam_tg;
	pData.chunk_iova = share_buf->chunk_iova;
	pData.cq_iova = share_buf->cq_dst_iova;
	pData.enable_raw = share_buf->enable_raw;
	pData.Hsf_en = En;

	pr_info("%s tg = %d  chunk_iova = 0x%llx  cq_iova = 0x%llx pData.Hsf_en = 0x%x\n", __func__,
		pData.tg_idx, pData.chunk_iova, pData.cq_iova, pData.Hsf_en);
	ret = mtk_ccu_rproc_ipc_send(
		hsf_config->ccu_pdev,
		MTK_CCU_FEATURE_CAMSYS,
		MSG_TO_CCU_HSF_CONFIG,
		(void *)&pData, sizeof(struct raw_info));

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	pr_info("%s time %d us\n", __func__, ms);
#endif

	if (ret != 0)
		pr_info("%s En(%d) Tg(%d) error\n", __func__, En, pData.tg_idx);
	else
		pr_info("%s En(%d) Tg(%d) success\n", __func__, En, pData.tg_idx);
}



void ccu_apply_cq(struct mtk_cam_job *job, unsigned long raw_engines, dma_addr_t cq_addr,
	unsigned int cq_size, unsigned int cq_offset,
	unsigned int sub_cq_size, unsigned int sub_cq_offset)
{
	int ret = 0;
	struct mtk_cam_hsf_ctrl *hsf_config = NULL;
	struct mtk_cam_hsf_info *share_buf = NULL;
	struct cq_info pData;

	struct mtk_cam_device *cam = job->src_ctx->cam;
	struct mtk_raw_device *raw_dev;
	int raw_id;
	//struct apply_cq_ref *ref = &job->cq_ref;
#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time;

	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

	raw_id = find_first_bit_set(raw_engines);
	pr_info("%s raw_engines:%lu raw_id:%d\n", __func__, raw_engines, raw_id);
	if (raw_id < 0) {
		pr_info("%s error: raw_id is not found\n", __func__);
		return;
	}
	raw_dev = dev_get_drvdata(cam->engines.raw_devs[raw_id]);

	/* note: apply cq with size = 0, will cause cq hang */
	if (WARN_ON(!cq_size || !sub_cq_size))
		return;

	//if (WARN_ON(assign_apply_cq_ref(&raw_dev->cq_ref, ref)))
	//	return;

	hsf_config = job->src_ctx->hsf;
	if (hsf_config == NULL) {
		pr_info("%s error: hsf_config is NULL pointer check hsf initial\n", __func__);
		return;
	}

	share_buf = hsf_config->share_buf;
	if (share_buf == NULL) {
		pr_info("%s hsf config fail share buffer not alloc\n", __func__);
		return;
	}
#ifdef USING_CCU
	if (!(hsf_config->ccu_pdev))
		get_ccu_device(hsf_config);
#endif

    /* call CCU to trigger CQ*/
	pData.tg = raw_id;
	pData.dst_addr = share_buf->cq_dst_iova;
	pData.chunk_iova = share_buf->chunk_iova;
	pData.src_addr = cq_addr;
	pData.cq_size = cq_size;
	pData.init_value = 0;
	pData.cq_offset = cq_offset;
	pData.sub_cq_size = sub_cq_size;
	pData.sub_cq_offset = sub_cq_offset;
	pData.ipc_status = 0;

	pr_info("CCU trigger CQ. tg:%d cq_src:0x%llx cq_dst:0x%llx cq_addr = 0x%llx init_value = %d\n",
		pData.tg, pData.src_addr, pData.dst_addr, cq_addr, pData.init_value);
#ifdef USING_CCU
	pr_info("ccu_pdev = 0x%pad\n", hsf_config->ccu_pdev);
	ret = mtk_ccu_rproc_ipc_send(
		hsf_config->ccu_pdev,
		MTK_CCU_FEATURE_CAMSYS,
		MSG_TO_CCU_HSF_APPLY_CQ,
		(void *)&pData, sizeof(struct cq_info));
#endif

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	pr_info("%s time %d us\n", __func__, ms);
#endif

	if (ret != 0)
		pr_info("error: CCU trigger CQ failure. tg:%d cq_src:0x%llx cq_dst:0x%llx cq_size:%d\n",
		pData.tg, pData.src_addr, pData.dst_addr, pData.cq_size);
	else
		pr_info("after CCU trigger CQ. tg:%d cq_src:0x%llx cq_dst:0x%llx initial_value = %d\n",
		pData.tg, pData.src_addr, pData.dst_addr, pData.init_value);

	if (pData.ipc_status != 0)
		pr_info("error:CCU trrigger CQ Sensor initial fail 0x%x\n",
		pData.init_value);

}
int mtk_cam_hsf_init(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_hsf_ctrl *hsf_config;
	int Buf_s = 0;
#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time;

	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

	hsf_config = ctx->hsf = kmalloc(sizeof(struct mtk_cam_hsf_info), GFP_KERNEL);
	if (ctx->hsf == NULL) {
		dev_info(cam->dev, "ctx->hsf == NULL\n");
		return -1;
	}

	Buf_s = sizeof(struct mtk_cam_hsf_info);
	Buf_s = ((Buf_s) + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1); //align Page size
	hsf_config->share_buf = kmalloc(Buf_s, GFP_KERNEL);
	if (hsf_config->share_buf == NULL) {
		dev_info(cam->dev, "share_buf kmalloc fail\n");
		return -1;
	}
	memset(hsf_config->share_buf, 0, Buf_s); /*init data */

	hsf_config->cq_buf = kmalloc(sizeof(struct mtk_cam_dma_map), GFP_KERNEL);
	if (hsf_config->cq_buf == NULL)
		return -1;

	hsf_config->chk_buf = kmalloc(sizeof(struct mtk_cam_dma_map), GFP_KERNEL);
	if (hsf_config->chk_buf == NULL)
		return -1;

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "hsf alloc time %d us\n", ms);
#endif

#ifdef USING_CCU
#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif
	get_ccu_device(hsf_config);
	mtk_cam_power_on_ccu(hsf_config, 1);
#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "ccu power on %d us\n", ms);
#endif
#endif
	dev_info(cam->dev, "hsf initial ready\n");
	return 0;
}

int mtk_cam_hsf_config(struct mtk_cam_ctx *ctx, unsigned int raw_id)
{
#define CQ_SIZE 0x1000
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_hsf_ctrl *hsf_config = NULL;
	struct mtk_cam_hsf_info *share_buf = NULL;
	struct mtk_cam_dma_map *dma_map_cq = NULL;
	struct mtk_cam_dma_map *dma_map_chk = NULL;
	struct device *dev_to_attach;
	//struct mtk_raw_device *raw_dev;
	struct arm_smccc_res res;

#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time;

	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

	if (ctx == NULL) {
		pr_info("%s error: ctx is NULL pointer\n", __func__);
		return -1;
	}

	hsf_config = ctx->hsf;
	if (hsf_config == NULL) {
		pr_info("%s error: hsf_config is NULL pointer check hsf initial\n", __func__);
		return -1;
	}

	share_buf = hsf_config->share_buf;
	if (share_buf == NULL) {
		pr_info("%s hsf config fail share buffer not alloc\n", __func__);
		return -1;
	}

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "hsf initial time %d us\n", ms);
#endif

	share_buf->cq_size = CQ_SIZE;
	share_buf->enable_raw = bit_map_subset_of(MAP_HW_RAW, ctx->used_engine);
	share_buf->cam_module = raw_id;
	share_buf->cam_tg = raw_id;

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

	//FIXME:
	dma_map_cq = hsf_config->cq_buf;
	dma_map_chk = hsf_config->chk_buf;

	//TEST: secure iova map to camsys device.
	dma_map_cq->dbuf = mtk_cam_dmabuf_alloc(ctx, CQ_SIZE);
	if (!dma_map_cq->dbuf) {
		dev_info(cam->dev, "mtk_cam_dmabuf_alloc fail\n");
		return -1;
	}

	dev_info(cam->dev, "mtk_cam_dmabuf_alloc Done\n");
	if (smmu_v3_enabled())
		dev_to_attach = ctx->cam->smmu_dev;
	else
		dev_to_attach = &(hsf_config->ccu_pdev->dev);

	if (mtk_cam_dmabuf_get_iova(ctx, dev_to_attach, dma_map_cq) != 0) {
		dev_info(cam->dev, "Get cq iova failed\n");
		return -1;
	}
	share_buf->cq_dst_iova = dma_map_cq->dma_addr;
	dev_info(cam->dev, "dma_map_cq->dma_addr:0x%llx dma_map_cq->hsf_handle:0x%llx\n",
		dma_map_cq->dma_addr, dma_map_cq->hsf_handle);

	//TEST: secure map to ccu devcie.
	dma_map_chk->dbuf = mtk_cam_dmabuf_alloc(ctx, CQ_SIZE);
	if (!dma_map_chk->dbuf) {
		dev_info(cam->dev, "mtk_cam_dmabuf_alloc dma_map_chk fail\n");
		return -1;
	}

	dev_info(cam->dev, "mtk_cam_dmabuf_alloc ccu Done\n");
	if (mtk_cam_dmabuf_get_iova(ctx, dev_to_attach, dma_map_chk) != 0) {
		dev_info(cam->dev, "Get chk iova failed\n");
		return -1;
	}

	dev_info(cam->dev, "dma_map_chk->dma_addr:0x%llx dma_map_chk->hsf_handle:0x%llx\n",
		dma_map_chk->dma_addr, dma_map_chk->hsf_handle);

	share_buf->chunk_hsfhandle = dma_map_chk->hsf_handle;
	share_buf->chunk_iova =  dma_map_chk->dma_addr;

	arm_smccc_smc(MTK_SIP_KERNEL_DAPC_CAM_CONTROL, 1, 0, 0, 0, 0, 0, 0, &res);

	ccu_hsf_config(ctx, 1);


#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "hsf config time %d us\n", ms);
#endif
#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif

#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "atf devapc enable time %d us\n", ms);
#endif

	return 0;
}

int mtk_cam_hsf_uninit(struct mtk_cam_ctx *ctx)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct mtk_cam_hsf_ctrl *hsf_config = NULL;
	int ret = 0;
	struct arm_smccc_res res;
#ifdef PERFORMANCE_HSF
	int ms_0 = 0, ms_1 = 0, ms = 0;
	struct timeval time
;
	do_gettimeofday(&time);
	ms_0 = time.tv_sec + time.tv_usec;
#endif
	hsf_config = ctx->hsf;
	if (hsf_config == NULL) {
		pr_info("%s error: hsf_config is NULL pointer\n", __func__);
		return -1;
	}

	if (ret != 0) {
		dev_info(cam->dev, "HSF camera unint fail ret = %d\n", ret);
		return -1;
	}

	ccu_hsf_config(ctx, 0);
	arm_smccc_smc(MTK_SIP_KERNEL_DAPC_CAM_CONTROL, 0, 0, 0, 0, 0, 0, 0, &res);
	mtk_cam_dmabuf_free_iova(ctx, hsf_config->cq_buf);
	mtk_cam_dmabuf_free_iova(ctx, hsf_config->chk_buf);

#ifdef USING_CCU
	mtk_cam_power_on_ccu(hsf_config, 0);
#endif
#ifdef PERFORMANCE_HSF
	do_gettimeofday(&time);
	ms_1 = time.tv_sec + time.tv_usec;
	ms = ms_1 - ms_0;
	dev_info(cam->dev, "%s %d us\n", __func__, ms);
#endif
	kfree(hsf_config->share_buf);
	kfree(hsf_config->cq_buf);
	kfree(hsf_config->chk_buf);
	kfree(ctx->hsf);
	ctx->hsf = NULL;

	ctx->enable_hsf_raw = 0;

	return 0;
}

int mtk_cam_hsf_aid(struct mtk_cam_ctx *ctx, unsigned int enable,
		    unsigned int feature, unsigned int used_engine)
{
	struct mtk_cam_device *cam = ctx->cam;
	struct aid_info pData;
	int ret = 0;

	pData.enable = enable;
	pData.feature = feature;
	pData.used_engine = used_engine;
	dev_info(cam->dev, "%s: set AID, enable:%d feature:%d eng:0x%x\n",
		 __func__, enable, feature, used_engine);

	if (mtk_cam_power_ctrl_ccu(cam->dev, 1))
		return -1;

	if (WARN_ON(!cam->ccu_pdev)) {
		ret = -1;
		goto FAILED;
	}

	ret = mtk_ccu_rproc_ipc_send(
		cam->ccu_pdev,
		MTK_CCU_FEATURE_CAMSYS,
		MSG_TO_CCU_AID,
		(void *)&pData, sizeof(struct aid_info));

	if (ret != 0)
		dev_info(cam->dev, "set AID fail, enable:%d feature:%d, ret = %d\n",
			 enable, feature, ret);

FAILED:
	mtk_cam_power_ctrl_ccu(cam->dev, 0);
	return ret;
}
