// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Ming-Hsuan.Chiang <Ming-Hsuan.Chiang@mediatek.com>
 *
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/dma-heap.h>
#include <uapi/linux/dma-heap.h>
#include <linux/pm_runtime.h>
#include <linux/suspend.h>
#include <linux/vmalloc.h>

#include <linux/device.h>
#include <linux/version.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include "iommu_debug.h"
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include "cmdq-sec.h"
#include "mtk_heap.h"

#include "mtk-mae.h"
#include "mtk_notify_aov.h"
#include "./mae_mm16_fd/fdvt_FPGA_coef.h"
#include "./mae_mm16_fd/fdvt_FPGA_config.h"
#include "./mae_mm16_fld/fld_FPGA_coef.h"
#include "./mae_mm16_fld/fld_FPGA_config.h"
//#define FLD_GOLDEN
//#define GOLDEN

#if IS_ENABLED(CONFIG_MTK_SLBC)
#include <slbc_ops.h>
#endif

#ifdef GOLDEN
#include "./mae_mm16_fd/fdvt_FPGA_DMA0_outer0_input.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA0_outer0_input1.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA0_outer0_output.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA1_outer0_output.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA2_outer0_output.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA3_outer0_output.h"
#include "./mae_mm16_fd/fdvt_FPGA_DMA4_outer0_output.h"
#endif

#ifdef FLD_GOLDEN
#include "./mae_mm16_fld/fld_FPGA_DMA0_outer0_input.h"
#include "./mae_mm16_fld/fld_FPGA_DMA0_outer0_input1.h"
#include "./mae_mm16_fld/fld_FPGA_DMA0_outer0_output.h"
#include "./mae_mm16_fld/fld_FPGA_DMA1_outer0_output.h"
#include "./mae_mm16_fld/fld_FPGA_DMA2_outer0_output.h"
#include "./mae_mm16_fld/fld_FPGA_DMA3_outer0_output.h"
#include "./mae_mm16_fld/fld_FPGA_DMA4_outer0_output.h"
#include "./mae_mm16_fld/fld_FPGA_DMA5_outer0_output.h"
#endif

// DEBUG_ONLY
#include <linux/delay.h>

#define M2M_ENABLE 1
#define AOV_NOTIFY_AIE_AVAIL 1

/*
 * MAE Debug level:
 * MAE_INFO = 0
 * MAE_DEBUG = 1
 */
int mae_log_level_value;
int delay_time;
int dump_reg_en;
int irq_handler_en = 1;
int umap_debug;
int cmdq_profiling_result;

#if IS_ENABLED(CONFIG_MTK_SLBC)
int mae_slc_dbg_en = 1;
static int mae_gid;
static struct slbc_gid_data *mae_slbc_gid_data;

module_param(mae_slc_dbg_en, int, 0644);
#endif
module_param(mae_log_level_value, int, 0644);
module_param(delay_time, int, 0644);
module_param(dump_reg_en, int, 0644);
module_param(irq_handler_en, int, 0644);
module_param(umap_debug, int, 0644);
module_param(cmdq_profiling_result, int, 0644);

static struct device *mae_pm_dev;
aov_notify m_aov_notify = NULL;

mtk_mae_register_tf_cb m_mae_reg_tf_cb;

#if M2M_ENABLE
#define V4L2_META_FMT_MTFD_RESULT  v4l2_fourcc('M', 'T', 'f', 'd')
#endif
#define MTK_MAE_OUTPUT_MIN_WIDTH 0U
#define MTK_MAE_OUTPUT_MIN_HEIGHT 0U
#define MTK_MAE_OUTPUT_MAX_WIDTH 4096U
#define MTK_MAE_OUTPUT_MAX_HEIGHT 4096U

static const struct v4l2_pix_format_mplane mtk_mae_img_fmts[] = {
	{
		.pixelformat = V4L2_PIX_FMT_YUYV, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_YVYU, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_UYVY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_VYUY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_GREY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV12M, .num_planes = 2,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV12, .num_planes = 1,
	},
};
#define NUM_FORMATS ARRAY_SIZE(mtk_mae_img_fmts)

#define call_mae_drv_ops(dev, ops, op, args...)		\
({													\
	int ret = 0;									\
	if ((ops) && (ops)->op)							\
		ret = (ops)->op(args);						\
	else if (ops)									\
		mae_dev_info(dev, "%s is Null\n", #op);		\
	else											\
		mae_dev_info(dev, "%s is Null\n", #ops);	\
	ret;											\
})

static struct mae_plat_data g_mae_data;

void mtk_mae_set_data(const struct mae_plat_data *plat_data)
{
	g_mae_data.drv_ops = plat_data->drv_ops;
	g_mae_data.clks = plat_data->clks;
	g_mae_data.clk_num = plat_data->clk_num;
	g_mae_data.data = plat_data->data;
}
EXPORT_SYMBOL(mtk_mae_set_data);

void aov_notify_register(aov_notify aov_notify_fn)
{
	m_aov_notify = aov_notify_fn;
}
EXPORT_SYMBOL(aov_notify_register);

void register_mtk_mae_reg_tf_cb(mtk_mae_register_tf_cb mtk_mae_register_tf_cb_fn)
{
	m_mae_reg_tf_cb = mtk_mae_register_tf_cb_fn;
}
EXPORT_SYMBOL(register_mtk_mae_reg_tf_cb);

void mtk_aie_aov_memcpy(char *buffer)
{
	char *tmp = buffer;

	memcpy(tmp, &fdvt_FPGA_coef_frame01[0], sizeof(fdvt_FPGA_coef_frame01));
	tmp += sizeof(fdvt_FPGA_coef_frame01);

	memcpy(tmp, &fdvt_FPGA_config_frame01[0], sizeof(fdvt_FPGA_config_frame01));
#ifdef GOLDEN
	tmp += sizeof(fdvt_FPGA_config_frame01);
	memcpy(tmp, &fdvt_FPGA_DMA0_outer0_input_frame01[0], sizeof(fdvt_FPGA_DMA0_outer0_input_frame01));
	tmp += sizeof(fdvt_FPGA_DMA0_outer0_input_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA0_outer0_input1_frame01[0], sizeof(fdvt_FPGA_DMA0_outer0_input1_frame01));
	tmp += sizeof(fdvt_FPGA_DMA0_outer0_input1_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA0_outer0_output_frame01[0],  sizeof(fdvt_FPGA_DMA0_outer0_output_frame01));
	tmp += sizeof(fdvt_FPGA_DMA0_outer0_output_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA1_outer0_output_frame01[0],  sizeof(fdvt_FPGA_DMA1_outer0_output_frame01));
	tmp += sizeof(fdvt_FPGA_DMA1_outer0_output_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA2_outer0_output_frame01[0],  sizeof(fdvt_FPGA_DMA2_outer0_output_frame01));
	tmp += sizeof(fdvt_FPGA_DMA2_outer0_output_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA3_outer0_output_frame01[0],  sizeof(fdvt_FPGA_DMA3_outer0_output_frame01));
	tmp += sizeof(fdvt_FPGA_DMA3_outer0_output_frame01);

	memcpy(tmp, &fdvt_FPGA_DMA4_outer0_output_frame01[0],  sizeof(fdvt_FPGA_DMA4_outer0_output_frame01));
#endif
}
EXPORT_SYMBOL(mtk_aie_aov_memcpy);

void mtk_fld_aov_memcpy(char *buffer)
{
	char *tmp = buffer;

	memcpy(tmp, &fld_FPGA_coef_frame01[0], sizeof(fld_FPGA_coef_frame01));
	tmp += sizeof(fld_FPGA_coef_frame01);

	memcpy(tmp, &fld_FPGA_config_frame01[0], sizeof(fld_FPGA_config_frame01));
#ifdef FLD_GOLDEN
	tmp += sizeof(fld_FPGA_config_frame01);
	memcpy(tmp, &fld_FPGA_DMA0_outer0_input_frame01[0], sizeof(fld_FPGA_DMA0_outer0_input_frame01));
	tmp += sizeof(fld_FPGA_DMA0_outer0_input_frame01);

	memcpy(tmp, &fld_FPGA_DMA0_outer0_input1_frame01[0], sizeof(fld_FPGA_DMA0_outer0_input1_frame01));
	tmp += sizeof(fld_FPGA_DMA0_outer0_input1_frame01);

	memcpy(tmp, &fld_FPGA_DMA0_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA0_outer0_output_frame01));
	tmp += sizeof(fld_FPGA_DMA0_outer0_output_frame01);

	memcpy(tmp, &fld_FPGA_DMA1_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA1_outer0_output_frame01));
	tmp += sizeof(fld_FPGA_DMA1_outer0_output_frame01);

	memcpy(tmp, &fld_FPGA_DMA2_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA2_outer0_output_frame01));
	tmp += sizeof(fld_FPGA_DMA2_outer0_output_frame01);

	memcpy(tmp, &fld_FPGA_DMA3_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA3_outer0_output_frame01));
	tmp += sizeof(fld_FPGA_DMA3_outer0_output_frame01);

	memcpy(tmp, &fld_FPGA_DMA4_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA4_outer0_output_frame01));
	tmp += sizeof(fld_FPGA_DMA4_outer0_output_frame01);

	memcpy(tmp, &fld_FPGA_DMA5_outer0_output_frame01[0],  sizeof(fld_FPGA_DMA5_outer0_output_frame01));
#endif
}
EXPORT_SYMBOL(mtk_fld_aov_memcpy);

enum MAE_BUF_TYPE {
	SECURE_BUF,
	CACHED_BUF,
	UNCACHED_BUF
};

void mtk_mae_get_kernel_time(struct mtk_mae_dev *mae_dev,
		struct EnqueParam *param,
		uint32_t idx)
{
	if (!param){
		mae_dev_info(mae_dev->dev, "[%s] param is null\n", __func__);
		return;
	}

	if (idx < MAE_TIME_INTERVAL_MAX) {
		param->mae_ktime[idx].requestNum = param->requestNum;
		param->mae_ktime[idx].ktime = ktime_get_boottime_ns();
		param->mae_ktime[idx].maeMode = param->maeMode;
	} else {
		mae_dev_info(mae_dev->dev, "[%s] over max debug timeval (%d/%d)\n",
			__func__, idx, MAE_TIME_INTERVAL_MAX);
	}
}

static int mtk_mae_dev_larb_init(struct mtk_mae_dev *mae_dev)
{
	struct device_node *node;
	struct platform_device *pdev;
	struct device_link *link;

	node = of_parse_phandle(mae_dev->dev->of_node, "mediatek,larb", 0);
	if (!node)
		return -EINVAL;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -EINVAL;
	}
	of_node_put(node);

	mae_dev->larb = &pdev->dev;

	link = device_link_add(mae_dev->dev, &pdev->dev,
					DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (!link) {
		dev_info(mae_dev->dev, "unable to link SMI LARB idx\n");
		return -EINVAL;
	}

	return 0;
}

static void mtk_mae_cmdq_alloc_buf(struct cmdq_client *clt, u32 **buf_va, dma_addr_t *buf_pa)
{
	int *p;

	if (!clt) {
		pr_err("param is NULL");
		return;
	}

	*buf_va = cmdq_mbox_buf_alloc(clt, buf_pa);
	if (*buf_va) {
		p = (int *)*buf_va;
		*p = 0;
	} else {
		pr_err("%s: cmdq mbox buf alloc fail\n", __func__);
	}
}

static int mtk_mae_ccf_enable(struct device *dev)
{
	struct mtk_mae_dev *mae_dev = dev_get_drvdata(dev);
	int ret;

	ret = clk_bulk_prepare_enable(g_mae_data.clk_num,
			g_mae_data.clks);
	if (ret) {
		dev_info(mae_dev->dev, "failed to enable mae clock:%d\n", ret);
		return ret;
	}

	return 0;
}

static void mtk_mae_ccf_disable(struct device *dev)
{
	clk_bulk_disable_unprepare(g_mae_data.clk_num,
			g_mae_data.clks);
}

static struct dma_buf *mae_imem_sec_alloc(struct mtk_mae_dev *mae_dev,
						uint32_t size,
						enum MAE_BUF_TYPE buf_type)
{
	struct dma_heap *dma_heap;
	struct dma_buf *my_dma_buf;

	switch (buf_type) {
	case SECURE_BUF:
		dma_heap = dma_heap_find("mtk_prot_region");
		break;
	case CACHED_BUF:
		dma_heap = dma_heap_find("mtk_mm");
		break;
	case UNCACHED_BUF:
		dma_heap = dma_heap_find("mtk_mm-uncached");
		break;
	default:
		dma_heap = dma_heap_find("mtk_mm-uncached");
		break;
	}

	if (!dma_heap) {
		mae_dev_info(mae_dev->dev, "heap find fail\n");
		return NULL;
	}

	my_dma_buf = dma_heap_buffer_alloc(dma_heap, size, O_RDWR |
		O_CLOEXEC, DMA_HEAP_VALID_HEAP_FLAGS);
	if (IS_ERR(my_dma_buf)) {
		mae_dev_info(mae_dev->dev, "buffer alloc fail\n");
		dma_heap_put(dma_heap);
		return NULL;
	}

	return my_dma_buf;
}

static int mtk_mae_set_dmabuf_info(struct mtk_mae_dev *mae_dev,
							int fd,
							struct dmabuf_info *info,
							enum MAE_ADDR_TYPE addr_type)
{
	int ret;

	info->dmabuf = dma_buf_get(fd);
	if (IS_ERR(info->dmabuf) || info->dmabuf == NULL) {
		mae_dev_info(mae_dev->dev, "%s, dma_buf_get failed fd(%d)\n",
					__func__, fd);
		return -ENOMEM;
	}

	if (addr_type == GET_VA || addr_type == GET_BOTH) {
#ifdef MAE_DMA_BUF_UNLOCK_API
		ret = dma_buf_vmap_unlocked(info->dmabuf, &info->map);
#else
		ret = dma_buf_vmap(info->dmabuf, &info->map);
#endif
		if (ret) {
			mae_dev_info(mae_dev->dev, "%s, map kernel va failed fd(%d)\n",
					__func__, fd);
				ret = -ENOMEM;
				goto ERROR_DMA_BUF_VMAP_FAIL;
		}
		info->kva = (uint64_t)info->map.vaddr;

		info->is_map = true;
	}

	if (addr_type == GET_PA || addr_type == GET_BOTH) {
		info->attach =
			dma_buf_attach(info->dmabuf, mae_dev->smmu_dev);
		if (IS_ERR(info->attach)) {
			mae_dev_info(mae_dev->dev, "%s, dmabuf attach fail fd(%d)\n",
					__func__, fd);
			ret = -ENOMEM;
			goto ERROR_DMA_BUF_ATTACH_FAIL;
		}

#ifdef MAE_DMA_BUF_UNLOCK_API
		info->sg_table =
			dma_buf_map_attachment_unlocked(info->attach, DMA_BIDIRECTIONAL);
#else
		info->sg_table =
			dma_buf_map_attachment(info->attach, DMA_BIDIRECTIONAL);
#endif
		if (IS_ERR(info->sg_table)) {
			mae_dev_info(mae_dev->dev, "%s, dmabuf map attach fail fd(%d)\n",
					__func__, fd);
			ret = -ENOMEM;
			goto ERROR_DMA_BUF_MAP_ATTACHMENT_FAIL;
		}
		info->pa = sg_dma_address(info->sg_table->sgl);

		info->is_attach = true;
	}

	return 0;

ERROR_DMA_BUF_MAP_ATTACHMENT_FAIL:
	dma_buf_detach(info->dmabuf, info->attach);

ERROR_DMA_BUF_ATTACH_FAIL:
	if (addr_type == GET_BOTH)
		return ret;

ERROR_DMA_BUF_VMAP_FAIL:
	dma_buf_put(info->dmabuf);

	return ret;
}

static int mtk_mae_acquire_cache(struct mtk_mae_dev *mae_dev,
			struct list_head *list,
			s32 fd,
			struct dmabuf_info_cache **cache,
			enum MAE_ADDR_TYPE addr_type)
{
	int ret;
	bool dmabuf_found = false;
	struct dmabuf_info_cache *tmp;

	list_for_each_entry(tmp, list, list_entry) {
		if (fd == tmp->fd) {
			dmabuf_found = true;
			*cache = tmp;
			break;
		}
	}
	if (!dmabuf_found) {
		tmp = vzalloc(sizeof(**cache));
		if (tmp == NULL)
			return -ENOMEM;
		tmp->fd = fd;
		ret = mtk_mae_set_dmabuf_info(mae_dev,
					tmp->fd,
					&tmp->info,
					addr_type);
		if (ret) {
			vfree(tmp);
			mae_dev_dbg(mae_dev->dev, "%s, set dmabuf info fail\n",
				__func__);
			return ret;
		}
		list_add_tail(&tmp->list_entry, list);
		mae_dev_dbg(mae_dev->dev, "%s, new fd: %d\n",
			__func__, tmp->fd);
		*cache = tmp;
	}
	return 0;
}

static void mtk_mae_hw_done(struct mtk_mae_dev *mae_dev,
			enum vb2_buffer_state vb_state)
{
#if M2M_ENABLE
	struct mtk_mae_ctx *ctx;
	struct vb2_v4l2_buffer *src_vbuf = NULL, *dst_vbuf = NULL;

	ctx = v4l2_m2m_get_curr_priv(mae_dev->m2m_dev);
	if (ctx == NULL)
		return;

	src_vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	if (src_vbuf == NULL)
		return;

	dst_vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	if (dst_vbuf == NULL)
		return;

	v4l2_m2m_buf_copy_metadata(src_vbuf, dst_vbuf,
				   V4L2_BUF_FLAG_TSTAMP_SRC_MASK);
	v4l2_m2m_buf_done(src_vbuf, vb_state);
	v4l2_m2m_buf_done(dst_vbuf, vb_state);
	v4l2_m2m_job_finish(mae_dev->m2m_dev, ctx->fh.m2m_ctx);

	complete_all(&mae_dev->mae_job_finished);
	wake_up(&mae_dev->flushing_waitq);

#else
	// MAE_TO_DO
#endif
}

static void mtk_mae_frame_done_worker(struct work_struct *work)
{
	struct mtk_mae_req_work *req_work = (struct mtk_mae_req_work *)work;
	struct mtk_mae_dev *mae_dev = (struct mtk_mae_dev *)req_work->mae_dev;
	struct EnqueParam *param =
		(struct EnqueParam *)mae_dev->map_table->param_dmabuf_info[0].kva;
	uint32_t *dump;

	mutex_lock(&mae_dev->mae_stream_lock);

	// MAE_TO_DO: support multiple users by multiple works
	if (mae_dev->mae_stream_count != 0)
		mtk_mae_get_kernel_time(mae_dev, param, MAE_CMDQ_PKT_WAIT_COMPLETE_START);

	cmdq_pkt_wait_complete(mae_dev->pkt[0]);

	if (mae_dev->mae_stream_count != 0)
		mtk_mae_get_kernel_time(mae_dev, param, MAE_CMDQ_PKT_DESTROY_START);

	cmdq_pkt_destroy(mae_dev->pkt[0]);

	if (mae_dev->mae_stream_count != 0)
		mtk_mae_get_kernel_time(mae_dev, param, MAE_CMDQ_PKT_DESTROY_END);

	if (mae_dev->is_hw_hang) {
		mtk_mae_hw_done(mae_dev, VB2_BUF_STATE_ERROR);
	} else {
		if (delay_time != 0)
			msleep(delay_time);

		if (cmdq_profiling_result)
			mae_dev_info(mae_dev->dev, "mode(%d) hw time: %u us\n", param->maeMode,
				(*(mae_dev->mae_time_ed_va) - *(mae_dev->mae_time_st_va)) / 26);

		dump = (uint32_t *)mae_dev->map_table->output_dmabuf_info[0][0].kva;

		mae_dev_dbg(mae_dev->dev, "%s, output 0x%llx (0x%x_%x)(0x%x_%x)\n",
			__func__, (uint64_t)dump, *(dump), *(dump+1), *(dump+2), *(dump+3));


		if (!mae_dev->is_shutdown && mae_dev->mae_stream_count != 0) {
			switch (param->maeMode) {
			case FD_V0:
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					get_fd_v0_result,
					mae_dev, 0);
				break;
			case FD_V1_IPN:
			case FD_V1_FPN:
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					get_fd_v1_result,
					mae_dev, 0);
				break;
			default:
				break;
			}

			if (dump_reg_en)
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					dump_reg,
					mae_dev);

			if (irq_handler_en)
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					irq_handle,
					mae_dev);
		} else {
			mae_dev_info(mae_dev->dev, "%s, skip read hw reg, is_shutdown(%d), stream_count(%d)\n",
				__func__, mae_dev->is_shutdown, mae_dev->mae_stream_count);
		}

		mtk_mae_hw_done(mae_dev, VB2_BUF_STATE_DONE);
	}

	if (mae_dev->mae_stream_count != 0)
		mtk_mae_get_kernel_time(mae_dev, param, MAE_FRAME_DONE_WORKER_END);

	mutex_unlock(&mae_dev->mae_stream_lock);
}

static const struct v4l2_pix_format_mplane *mtk_mae_find_fmt(u32 format)
{
	unsigned int i;

	for (i = 0; i < NUM_FORMATS; i++) {
		if (mtk_mae_img_fmts[i].pixelformat == format)
			return &mtk_mae_img_fmts[i];
	}

	return NULL;
}

#if M2M_ENABLE
static void mtk_mae_device_run(void *priv)
{
	struct mtk_mae_ctx *ctx = priv;
	struct mtk_mae_dev *mae_dev = ctx->mae_dev;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct EnqueParam *param;
	uint32_t idx;

	src_buf = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	if (src_buf == NULL)
		return;

	dst_buf = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	if (dst_buf == NULL)
		return;

	idx = src_buf->vb2_buf.index;
	param = (struct EnqueParam*)mae_dev->map_table->param_dmabuf_info[idx].kva;

	// mae_dev->mae_out = vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);
	// plane_vaddr = vb2_plane_vaddr(&dst_buf->vb2_buf, 0);

	if (mae_dev->is_shutdown)
		return;

	mtk_mae_get_kernel_time(mae_dev, param, MAE_CMDQ_PKT_CREATE_START);
	mae_dev->pkt[idx] = cmdq_pkt_create(mae_dev->mae_clt);
	mtk_mae_get_kernel_time(mae_dev, param, MAE_CMDQ_PKT_CREATE_END);

	reinit_completion(&mae_dev->mae_job_finished);
	mae_dev->is_hw_hang = false;

	if (param->maeMode == FLD_V0) {
		call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
			config_fld,
			mae_dev, idx);
	} else {
		mtk_mae_get_kernel_time(mae_dev, param, MAE_SET_DMA_ADDRESS_START);

		if (!call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					set_dma_address,
					mae_dev, idx)) {
			mae_dev_info(mae_dev->dev, "set dma address fail\n");
			return;
		}

		mtk_mae_get_kernel_time(mae_dev, param, MAE_CONFIG_HW_START);

		if (!call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					config_hw,
					mae_dev, idx))
			mae_dev_info(mae_dev->dev, "config hw fail\n");

		mtk_mae_get_kernel_time(mae_dev, param, MAE_CONFIG_HW_END);
	}
}

static struct v4l2_m2m_ops mae_m2m_ops = {
	.device_run = mtk_mae_device_run,
};
#endif

static const struct media_device_ops mae_m2m_media_ops = {
	.req_validate = vb2_request_validate,
	.req_queue = v4l2_m2m_request_queue,
};

static void mtk_mae_fill_pixfmt_mp(struct v4l2_pix_format_mplane *dfmt,
				const struct v4l2_pix_format_mplane *sfmt)
{
	dfmt->field = V4L2_FIELD_NONE;
	dfmt->colorspace = V4L2_COLORSPACE_BT2020;
	dfmt->num_planes = sfmt->num_planes;
	dfmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	dfmt->quantization = V4L2_QUANTIZATION_DEFAULT;
	dfmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(dfmt->colorspace);

	/* Keep user setting as possible */
	dfmt->width = clamp(dfmt->width, MTK_MAE_OUTPUT_MIN_WIDTH,
				MTK_MAE_OUTPUT_MAX_WIDTH);
	dfmt->height = clamp(dfmt->height, MTK_MAE_OUTPUT_MIN_HEIGHT,
				 MTK_MAE_OUTPUT_MAX_HEIGHT);

	if (sfmt->num_planes == 2) {
		dfmt->plane_fmt[0].sizeimage =
			dfmt->height * dfmt->plane_fmt[0].bytesperline;
		dfmt->plane_fmt[1].sizeimage =
			dfmt->height * dfmt->plane_fmt[1].bytesperline;
		if (sfmt->pixelformat == V4L2_PIX_FMT_NV12M)
			dfmt->plane_fmt[1].sizeimage =
				dfmt->height * dfmt->plane_fmt[1].bytesperline /
				2;
	} else {
		dfmt->plane_fmt[0].sizeimage =
			dfmt->height * dfmt->plane_fmt[0].bytesperline;
		if (sfmt->pixelformat == V4L2_PIX_FMT_NV12)
			dfmt->plane_fmt[0].sizeimage =
				dfmt->height * dfmt->plane_fmt[0].bytesperline *
				3 / 2;
	}
}

static void mtk_mae_init_v4l2_fmt(struct mtk_mae_ctx *ctx)
{
	struct v4l2_pix_format_mplane *src_fmt = &ctx->src_fmt;
	struct v4l2_meta_format *dst_fmt = &ctx->dst_fmt;

	/* Initialize source fmt */
	src_fmt->width = MTK_MAE_OUTPUT_MAX_WIDTH;
	src_fmt->height = MTK_MAE_OUTPUT_MAX_HEIGHT;
	mtk_mae_fill_pixfmt_mp(src_fmt, &mtk_mae_img_fmts[0]);

#if M2M_ENABLE
	/* Initialize destination fmt */
	dst_fmt->buffersize = sizeof(struct mtk_mae_enq_info);
	dst_fmt->dataformat = V4L2_META_FMT_MTFD_RESULT;
#endif

}

/*
 * vb2_ops: queue_setup
 */
static int mtk_mae_vb2_queue_setup(struct vb2_queue *vq,
				unsigned int *num_buffers,
				unsigned int *num_planes,
				unsigned int sizes[],
				struct device *alloc_devs[])
{
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vq);
	unsigned int size[2];
	unsigned int plane;

	switch (vq->type) {
#if M2M_ENABLE
	case V4L2_BUF_TYPE_META_CAPTURE:
		size[0] = ctx->dst_fmt.buffersize;
		break;
#endif

	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		size[0] = ctx->src_fmt.plane_fmt[0].sizeimage;
		size[1] = ctx->src_fmt.plane_fmt[1].sizeimage;
		break;
	}

	if (*num_planes > 2)
		return -EINVAL;
	if (*num_planes == 0) {
#if M2M_ENABLE
		if (vq->type == V4L2_BUF_TYPE_META_CAPTURE) {
			sizes[0] = ctx->dst_fmt.buffersize;
			*num_planes = 1;
			return 0;
		}
#endif

		*num_planes = ctx->src_fmt.num_planes;
		if (*num_planes > 2)
			return -EINVAL;
		for (plane = 0; plane < *num_planes; plane++) {
			sizes[plane] = ctx->src_fmt.plane_fmt[plane].sizeimage;
		}
		return 0;
	}

	return 0;
}

/*
 * vb2_ops: buf_out_validate
 */
static int mtk_mae_vb2_buf_out_validate(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	if (v4l2_buf->field == V4L2_FIELD_ANY)
		v4l2_buf->field = V4L2_FIELD_NONE;
	if (v4l2_buf->field != V4L2_FIELD_NONE)
		return -EINVAL;

	return 0;
}

/*
 * vb2_ops: buf_prepare
 */
static int mtk_mae_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vb2_queue *vq = vb->vb2_queue;
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vq);
	struct device *dev = ctx->dev;
	struct v4l2_pix_format_mplane *pixfmt;

	switch (vq->type) {
#if M2M_ENABLE
	case V4L2_BUF_TYPE_META_CAPTURE:
		if (vb2_plane_size(vb, 0) < ctx->dst_fmt.buffersize) {
			mae_dev_info(dev, "meta size %lu is too small\n",
						 vb2_plane_size(vb, 0));
			return -EINVAL;
		}
		break;
#endif
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		pixfmt = &ctx->src_fmt;

		if (vbuf->field == V4L2_FIELD_ANY)
			vbuf->field = V4L2_FIELD_NONE;

		if (vb->num_planes > 2 || vbuf->field != V4L2_FIELD_NONE) {
			mae_dev_info(dev, "plane %d or field %d not supported\n",
						 vb->num_planes, vbuf->field);
			return -EINVAL;
		}

		if (vb2_plane_size(vb, 0) < pixfmt->plane_fmt[0].sizeimage) {
			mae_dev_info(dev, "plane 0 %lu is too small than %x\n",
						 vb2_plane_size(vb, 0),
						 pixfmt->plane_fmt[0].sizeimage);
			return -EINVAL;
		}

		if (pixfmt->num_planes == 2 && (vb2_plane_size(vb, 1) < pixfmt->plane_fmt[1].sizeimage)) {
			mae_dev_info(dev, "plane 1 %lu is too small than %x\n",
							vb2_plane_size(vb, 1),
							pixfmt->plane_fmt[1].sizeimage);
			return -EINVAL;
		}
		break;
	}

	return 0;
}

/*
 * vb2_ops: buf_queue
 */
static void mtk_mae_vb2_buf_queue(struct vb2_buffer *vb)
{
#if M2M_ENABLE
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
#else
	// MAE_TO_DO: add buffer to ready queue
	pr_info("%s: +\n", __func__);
#endif
}

static int mtk_mae_hw_connect(struct mtk_mae_dev *mae_dev)
{
	struct dmabuf_info *buf_info = &mae_dev->map_table->internal_dmabuf_info;
	struct device *dev = mae_dev->dev;
	int ret;

	mutex_lock(&mae_dev->mae_stream_lock);
	mae_dev_info(mae_dev->dev, "%s+ count(%d)", __func__, mae_dev->mae_stream_count);

	mae_dev->mae_stream_count++;
	if (mae_dev->mae_stream_count == 1) {
		/* unavailable: 0 available: 1 */
		if (m_aov_notify != NULL)
			m_aov_notify(mae_dev->aov_pdev, AOV_NOTIFY_AIE_AVAIL, 0);

		memset(mae_dev->map_table, 0, sizeof(*mae_dev->map_table));
		if (m_mae_reg_tf_cb) {
			mae_dev_info(mae_dev->dev, "MAE register tf callback\n");
			m_mae_reg_tf_cb(mae_dev);
		}

		/* power on */
		if (!mae_dev->is_shutdown) {
			ret = pm_runtime_get_sync(dev);
			if (ret) {
				mae_dev_info(dev, "%s: pm_runtime_get_sync failed:(%d)\n",
					__func__, ret);
				return ret;
			}

			mtk_mae_ccf_enable(dev);

			cmdq_mbox_enable(mae_dev->mae_clt->chan);
			cmdq_clear_event(mae_dev->mae_clt->chan, mae_dev->mae_event_id);
		}

		buf_info->dmabuf =
			mae_imem_sec_alloc(mae_dev, g_mae_data.data->internal_buffer_size, CACHED_BUF);
		if (IS_ERR(buf_info->dmabuf) || buf_info->dmabuf == NULL) {
			mae_dev_info(mae_dev->dev, "%s, internal buffer alloc failed\n", __func__);
			mutex_unlock(&mae_dev->mae_stream_lock);
			return -ENOMEM;
		}

		buf_info->attach =
			dma_buf_attach(buf_info->dmabuf, mae_dev->smmu_dev);
		if (IS_ERR(buf_info->attach)) {
			mae_dev_info(mae_dev->dev, "%s, internal buf attach fail\n", __func__);
			ret = -ENOMEM;
			goto ERROR_DMA_BUF_ATTACH_FAIL;
		}

#ifdef MAE_DMA_BUF_UNLOCK_API
		buf_info->sg_table =
			dma_buf_map_attachment_unlocked(buf_info->attach, DMA_BIDIRECTIONAL);
#else
		buf_info->sg_table =
			dma_buf_map_attachment(buf_info->attach, DMA_BIDIRECTIONAL);
#endif
		if (IS_ERR(buf_info->sg_table)) {
			mae_dev_info(mae_dev->dev, "%s, dmabuf map attach fail\n", __func__);
			ret = -ENOMEM;
			goto ERROR_DMA_BUF_MAP_ATTACHMENT_FAIL;
		}
		buf_info->pa = sg_dma_address(buf_info->sg_table->sgl);

		buf_info->is_attach = true;

		// initialize
		mae_dev->is_first_qbuf = true;
		mae_dev->is_secure = false;

	#if IS_ENABLED(CONFIG_MTK_SLBC)
		/* register slc debug */
		if (mae_slc_dbg_en) {
			mae_gid = -1;
			mae_slbc_gid_data = vzalloc(sizeof(struct slbc_gid_data));
			mae_slbc_gid_data->sign = SLC_DATA_MAGIC;
			ret = slbc_gid_request(ID_MAE, &mae_gid, mae_slbc_gid_data);
			if (ret)
				dev_info(mae_dev->dev, "slc request fail");
			ret = slbc_validate(ID_MAE, mae_gid);
			if (ret)
				dev_info(mae_dev->dev, "slc validate fail");
		}
	#endif

		/* for profiling hw time */
		mtk_mae_cmdq_alloc_buf(mae_dev->mae_clt,
			&(mae_dev->mae_time_st_va),
			&(mae_dev->mae_time_st_pa));
		mtk_mae_cmdq_alloc_buf(mae_dev->mae_clt,
			&(mae_dev->mae_time_ed_va),
			&(mae_dev->mae_time_ed_pa));

		atomic_inc(&mae_dev->num_composing);
	}


	mutex_unlock(&mae_dev->mae_stream_lock);

	return 0;

ERROR_DMA_BUF_MAP_ATTACHMENT_FAIL:
	dma_buf_detach(buf_info->dmabuf, buf_info->attach);

ERROR_DMA_BUF_ATTACH_FAIL:
	dma_buf_put(buf_info->dmabuf);

	mae_dev->mae_stream_count--;

	mutex_unlock(&mae_dev->mae_stream_lock);

	return ret;
}
/*
 * vb2_ops: start_streaming
 */
static int mtk_mae_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vq);

#if M2M_ENABLE
	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return mtk_mae_hw_connect(ctx->mae_dev);
#else
	return mtk_mae_hw_connect(ctx->mae_dev);
#endif

	return 0;
}

static void mtk_mae_umap_detach(struct mtk_mae_dev *mae_dev,
								struct dmabuf_info *info)
{
	if (info->is_map) {
#ifdef MAE_DMA_BUF_UNLOCK_API
		dma_buf_vunmap_unlocked(info->dmabuf, &info->map);
#else
		dma_buf_vunmap(info->dmabuf, &info->map);
#endif
		info->is_map = false;
	}

	if (info->is_attach) {
#ifdef MAE_DMA_BUF_UNLOCK_API
		dma_buf_unmap_attachment_unlocked(info->attach,
			info->sg_table, DMA_BIDIRECTIONAL);
#else
		dma_buf_unmap_attachment(info->attach,
			info->sg_table, DMA_BIDIRECTIONAL);
#endif
		dma_buf_detach(info->dmabuf, info->attach);

		info->is_attach = false;
	}

	if (!IS_ERR(info->dmabuf) && info->dmabuf)
		dma_buf_put(info->dmabuf);

	info->kva = 0;
	info->pa = 0;
}

static void mtk_mae_release_cache(struct mtk_mae_dev *mae_dev,
			struct list_head *list)
{
	struct dmabuf_info_cache *cache, *tmp;

	list_for_each_entry_safe(cache, tmp, list, list_entry) {
		mae_dev_dbg(mae_dev->dev, "fd: %d\n", cache->fd);
		mtk_mae_umap_detach(mae_dev, &cache->info);
		list_del(&cache->list_entry);
		vfree(cache);
	}
}

static void mtk_mae_release_all_caches(struct mtk_mae_dev *mae_dev)
{
	mtk_mae_release_cache(mae_dev, &mae_dev->aiseg_config_cache_list);
	mtk_mae_release_cache(mae_dev, &mae_dev->aiseg_coef_cache_list);
	mtk_mae_release_cache(mae_dev, &mae_dev->aiseg_output_cache_list);
}

static void mtk_mae_hw_disconnect(struct mtk_mae_dev *mae_dev)
{
	uint32_t i;
	int ret;

	mutex_lock(&mae_dev->mae_stream_lock);

	// DEBUG_ONLY
	mae_dev_info(mae_dev->dev, "%s+ count(%d)", __func__, mae_dev->mae_stream_count);

	mae_dev->mae_stream_count--;
	if (mae_dev->mae_stream_count == 0) {
		if (mae_dev->is_secure && !mae_dev->is_shutdown) {
			call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					secure_disable,
					mae_dev);
#if MAE_CMDQ_SEC_READY
			cmdq_sec_mbox_stop(mae_dev->mae_secure_clt);
#endif
		}

		if (!mae_dev->is_shutdown) {
			cmdq_mbox_disable(mae_dev->mae_clt->chan);
			mtk_mae_ccf_disable(mae_dev->dev);
			ret = pm_runtime_put_sync(mae_dev->dev);
			if (ret)
				mae_dev_info(mae_dev->dev, "%s: pm_runtime_put_sync failed:(%d)\n",
					__func__, ret);

			/* unavailable: 0 available: 1 */
			if (m_aov_notify != NULL)
				m_aov_notify(mae_dev->aov_pdev, AOV_NOTIFY_AIE_AVAIL, 1);
		}

		// MAE_TO_DO: unmap buffer
		mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->model_table_dmabuf_info);
		mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->image_dmabuf_info[0]);
		mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->param_dmabuf_info[0]);
		mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->internal_dmabuf_info);
		mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->debug_dmabuf_info[0]);

		for (i = 0; i < MAX_PYRAMID_NUM; i++)
			mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->output_dmabuf_info[0][i]);

		for (i = 0; i < MODEL_TYPE_MAX; i++) {
			if (i == MODEL_TYPE_AISEG)
				continue;

			mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->config_dmabuf_info[i]);
			mtk_mae_umap_detach(mae_dev, &mae_dev->map_table->coef_dmabuf_info[i]);
		}

		mtk_mae_release_all_caches(mae_dev);

		/*slc uninit API*/
	#if IS_ENABLED(CONFIG_MTK_SLBC)
		if (mae_slc_dbg_en) {
			ret = slbc_invalidate(ID_MAE, mae_gid);
			if (ret)
				dev_info(mae_dev->dev, "slc invalidate fail");

			ret = slbc_gid_release(ID_MAE, mae_gid);
			if (ret)
				dev_info(mae_dev->dev, "slc release fail");

			vfree(mae_slbc_gid_data);
		}
	#endif

		/* for profiling hw time */
		cmdq_mbox_buf_free(mae_dev->mae_clt,
			mae_dev->mae_time_st_va,
			mae_dev->mae_time_st_pa);
		cmdq_mbox_buf_free(mae_dev->mae_clt,
			mae_dev->mae_time_ed_va,
			mae_dev->mae_time_ed_pa);

		atomic_dec(&mae_dev->num_composing);
	}


	mutex_unlock(&mae_dev->mae_stream_lock);
}


/*
 * vb2_ops: stop_streaming
 */
static void mtk_mae_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_mae_dev *mae_dev = ctx->mae_dev;
#if M2M_ENABLE
	struct vb2_v4l2_buffer *vb;
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct v4l2_m2m_queue_ctx *queue_ctx;
#else
	struct vb2_buffer *b;
#endif
	// MAE_TO_DO: wait job finish before stopping streaming
	// int ret;

	mae_dev_info(mae_dev->dev, "STREAM STOP\n");

	// MAE_TO_DO: wait job finish before stopping streaming
	// ret = mtk_aie_job_wait_finish(fd);
	// if (!ret)
	// 	aie_dev_info(fd->dev, "wait job finish timeout\n");

	if(!wait_for_completion_timeout(&mae_dev->mae_job_finished, msecs_to_jiffies(1000)))
		mae_dev_info(mae_dev->dev, "%s: wait job finish timeout\n", __func__);


#if M2M_ENABLE
	queue_ctx = V4L2_TYPE_IS_OUTPUT(vq->type) ? &m2m_ctx->out_q_ctx
						  : &m2m_ctx->cap_q_ctx;
	while ((vb = v4l2_m2m_buf_remove(queue_ctx)))
		v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		mtk_mae_hw_disconnect(mae_dev);
#else
	list_for_each_entry(b, &vq->queued_list, queued_entry)
		vb2_buffer_done(b, VB2_BUF_STATE_ERROR);

	// MAE_TO_DO: stopping stream flow
	// mtk_aie_hw_disconnect(fd);
#endif
}

/*
 * vb2_ops: buf_request_complete
 */
static void mtk_mae_vb2_request_complete(struct vb2_buffer *vb)
{
	struct mtk_mae_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_ctrl_request_complete(vb->req_obj.req, &ctx->hdl);
}

static const struct vb2_ops mtk_mae_vb2_ops = {
	.queue_setup = mtk_mae_vb2_queue_setup,
	.buf_out_validate = mtk_mae_vb2_buf_out_validate,
	.buf_prepare = mtk_mae_vb2_buf_prepare,
	.buf_queue = mtk_mae_vb2_buf_queue,
	.start_streaming = mtk_mae_vb2_start_streaming,
	.stop_streaming = mtk_mae_vb2_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.buf_request_complete = mtk_mae_vb2_request_complete,
};

#if M2M_ENABLE
static int mtk_mae_queue_init(void *priv, struct vb2_queue *src_vq,
				  struct vb2_queue *dst_vq)
{
	struct mtk_mae_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->supports_requests = true;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_mae_vb2_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->mae_dev->vdev_lock;
	src_vq->dev = ctx->mae_dev->smmu_dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_META_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_mae_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->mae_dev->vdev_lock;
	dst_vq->dev = ctx->mae_dev->smmu_dev;

	return vb2_queue_init(dst_vq);
}
#else
static int mtk_mae_queue_init(struct mtk_mae_ctx *ctx)
{
	int ret;
	struct vb2_queue *vq = ctx->vq;

	// DEBUG_ONLY
	pr_info("%s+", __func__);

	vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	vq->io_modes = VB2_MMAP | VB2_DMABUF;
	vq->supports_requests = true;
	vq->drv_priv = ctx;
	vq->ops = &mtk_mae_vb2_ops;
	vq->mem_ops = &vb2_dma_contig_memops;
	vq->buf_struct_size = sizeof(struct vb2_v4l2_buffer);
	vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	vq->lock = &ctx->mae_dev->vdev_lock;
	vq->dev = ctx->mae_dev->smmu_dev;

	ret = vb2_queue_init(vq);
	if (ret)
		pr_info("%s: queue init fail\n", __func__);

	return ret;
}
#endif

/*
 * V4L2 file operations: open
 */
static int mtk_mae_video_device_open(struct file *filp)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(filp);
	struct video_device *vdev = video_devdata(filp);
	struct mtk_mae_ctx *ctx = mae_dev->ctx;
	int ret;

	mutex_lock(&mae_dev->mae_device_lock);

	mae_dev_info(mae_dev->dev, "%s+ open_video_device_cnt(%d)\n",
				__func__, mae_dev->open_video_device_cnt);


	if (mae_dev->open_video_device_cnt == 0) {
		v4l2_fh_init(&ctx->fh, vdev);
		mtk_mae_init_v4l2_fmt(ctx);
#if M2M_ENABLE
		ctx->fh.m2m_ctx =
			v4l2_m2m_ctx_init(mae_dev->m2m_dev, ctx, &mtk_mae_queue_init);
		if (IS_ERR(ctx->fh.m2m_ctx)) {
			ret = PTR_ERR(ctx->fh.m2m_ctx);
			goto err_free_ctrl_handler;
		}
#else
		ctx->vq = kzalloc(sizeof(*ctx->vq), GFP_KERNEL);
		ret = mtk_mae_queue_init(ctx);
		if (ret)
			goto err_free_ctrl_handler;
#endif
		v4l2_fh_add(&ctx->fh);
	}

	filp->private_data = &ctx->fh;
	mae_dev->open_video_device_cnt++;

	mutex_unlock(&mae_dev->mae_device_lock);

	return 0;

err_free_ctrl_handler:
	v4l2_fh_exit(&ctx->fh);
	mutex_unlock(&mae_dev->mae_device_lock);

	return ret;
}

/*
 * V4L2 file operations: release
 */
static int mtk_mae_video_device_release(struct file *filp)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(filp);
	struct mtk_mae_ctx *ctx = mae_dev->ctx;

	mutex_lock(&mae_dev->mae_device_lock);

	if (mae_dev->open_video_device_cnt - 1 < 0) {
		mae_dev_info(mae_dev->dev, "open_video_device_cnt(%d): release null device\n",
			mae_dev->open_video_device_cnt);
		return -ENXIO;
	}

	mae_dev->open_video_device_cnt--;

	mae_dev_info(mae_dev->dev, "%s+ open_video_device_cnt(%d)\n",
				__func__, mae_dev->open_video_device_cnt);

	if (mae_dev->open_video_device_cnt == 0) {
#if M2M_ENABLE
		v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);
#endif

		v4l2_fh_del(&ctx->fh);
		v4l2_fh_exit(&ctx->fh);

#if (M2M_ENABLE == 0)
		vb2_queue_release(ctx->vq);
		kfree(ctx->vq);
#endif
	}

	mutex_unlock(&mae_dev->mae_device_lock);

	return 0;
}

/*
 * V4L2 file operations: poll
 */
static __poll_t mtk_mae_video_device_poll(struct file *file, poll_table *wait)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(file);

	if(!wait_for_completion_timeout(&mae_dev->mae_job_finished, msecs_to_jiffies(MTK_FD_HW_TIMEOUT))) {
		mae_dev_info(mae_dev->dev, "%s: wait job finish timeout\n", __func__);
		return EPOLLERR;
	}

	if (mae_dev->is_hw_hang) {
		mae_dev_info(mae_dev->dev, "%s: hw timeout\n", __func__);
		return EPOLLERR;
	}
#if M2M_ENABLE
	return v4l2_m2m_fop_poll(file, wait);
#else
	return vb2_fop_poll(file, wait);
#endif
}

static const struct v4l2_file_operations mae_video_fops = {
	.owner = THIS_MODULE,
	.open = mtk_mae_video_device_open,
	.release = mtk_mae_video_device_release,
	.poll = mtk_mae_video_device_poll,
	.unlocked_ioctl = video_ioctl2,
#if M2M_ENABLE
	.mmap = v4l2_m2m_fop_mmap,
#else
	.mmap = vb2_fop_mmap,
#endif
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = v4l2_compat_ioctl32,
#endif

};

/*
 * V4L2 ioctl operations: vidioc_querycap
 */
static int mtk_mae_querycap(struct file *file, void *fh,
				struct v4l2_capability *cap)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(file);
	struct device *dev = mae_dev->dev;
	int ret = 0;

	strscpy(cap->driver, dev_driver_string(dev), sizeof(cap->driver));
	strscpy(cap->card, dev_driver_string(dev), sizeof(cap->card));
	ret = snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(mae_dev->dev));
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * V4L2 ioctl operations: vidioc_enum_fmt_vid_out
 */
static int mtk_mae_enum_fmt_out_mp(struct file *file, void *fh,
				   struct v4l2_fmtdesc *f)
{
	if (f->index >= NUM_FORMATS)
		return -EINVAL;

	f->pixelformat = mtk_mae_img_fmts[f->index].pixelformat;
	return 0;
}

/*
 * V4L2 ioctl operations: vidioc_g_fmt_vid_out_mplane
 */
static int mtk_mae_g_fmt_out_mp(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(file);
	struct mtk_mae_ctx *ctx = mae_dev->ctx;

	if (ctx == NULL)
		return -1;

	f->fmt.pix_mp = ctx->src_fmt;

	return 0;
}

/*
 * V4L2 ioctl operations: vidioc_try_fmt_vid_out_mplane
 */
static int mtk_mae_try_fmt_out_mp(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct v4l2_pix_format_mplane *fmt;

	fmt = mtk_mae_find_fmt(pix_mp->pixelformat);
	if (!fmt)
		fmt = &mtk_mae_img_fmts[0]; /* Get default img fmt */

	mtk_mae_fill_pixfmt_mp(pix_mp, fmt);

	return 0;
}

/*
 * V4L2 ioctl operations: vidioc_s_fmt_vid_out_mplane
 */
static int mtk_mae_s_fmt_out_mp(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_mae_dev *mae_dev = video_drvdata(file);
	struct mtk_mae_ctx *ctx = mae_dev->ctx;
	struct vb2_queue *vq;

	if (ctx == NULL)
		return -1;

#if M2M_ENABLE
	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vq == NULL)
		return -1;
#else
	vq = ctx->vq
#endif
	/* Change not allowed if queue is streaming. */
	if (vb2_is_streaming(vq)) {
		mae_dev_info(ctx->dev, "Failed to set format, vb2 is busy\n");
		return -EBUSY;
	}

	mtk_mae_try_fmt_out_mp(file, fh, f);
	ctx->src_fmt = f->fmt.pix_mp;

	return 0;
}

#if M2M_ENABLE
/*
 * V4L2 ioctl operations: vidioc_enum_fmt_meta_cap
 */
static int mtk_mae_enum_fmt_meta_cap(struct file *file, void *fh,
					 struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;

	strscpy(f->description, "Face detection result",
		sizeof(f->description));

	f->pixelformat = V4L2_META_FMT_MTFD_RESULT;
	f->flags = 0;

	return 0;
}
#endif

/*
 * V4L2 ioctl operations:
 * vidioc_g_fmt_meta_cap,
 * vidioc_s_fmt_meta_cap,
 * vidioc_try_fmt_meta_cap
 */
static int mtk_mae_g_fmt_meta_cap(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	f->fmt.meta.dataformat = V4L2_META_FMT_MTFD_RESULT;
	f->fmt.meta.buffersize = sizeof(struct mtk_mae_enq_info);

	return 0;
}

/*
 * V4L2 ioctl operations: vidioc_dqbuf
 */
int mtk_mae_vioctl_dqbuf(struct file *file, void *priv,
			       struct v4l2_buffer *buf)
{
	struct mtk_mae_dev *mae_dev;

	if (file == NULL) {
		pr_info("%s: file is NULL\n", __func__);
		return -EFAULT;
	}

	if (buf == NULL) {
		pr_info("%s: buf is NULL\n", __func__);
		return -EFAULT;
	}

	if (file->private_data == NULL) {
		pr_info("%s: private data is NULL\n", __func__);
		return -EFAULT;
	}

	mae_dev = video_drvdata(file);

	if (mae_dev->mae_stream_count <= 0) {
		pr_info("%s: deque but not stream on\n", __func__);
		return -EPERM;
	}

	return v4l2_m2m_ioctl_dqbuf(file, priv, buf);
}

/*
 * V4L2 ioctl operations: vidioc_qbuf
 */
int mtk_mae_vidioc_qbuf(struct file *file, void *priv,
			struct v4l2_buffer *buf)
{
	struct mtk_mae_dev *mae_dev;
	struct mtk_mae_map_table *map_table;
	uint32_t idx;
	int ret;
	struct ModelTable *model_table;
	struct EnqueParam *param;
	uint32_t i;
	struct dmabuf_info_cache *cache = NULL;

	if (file == NULL || buf == NULL)
		return -EFAULT;

#if M2M_ENABLE
	if (buf->type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return v4l2_m2m_ioctl_qbuf(file, priv, buf);
#endif

	idx = buf->index;

	mae_dev = video_drvdata(file);

	map_table = mae_dev->map_table;

	if (buf->length < MAX_PLANE) {
		mae_dev_info(mae_dev->dev, "%s, buf length is too small (%d/%d)\n",
			__func__, buf->length, MAX_PLANE);
		return -EINVAL;
	}

	// MAE_TO_DO: lock for shared variable
	// get va of model table
	if (!map_table->model_table_dmabuf_info.is_map) {
		ret = mtk_mae_set_dmabuf_info(mae_dev,
					buf->m.planes[MODEL_TABLE_PLANE].m.fd,
					&map_table->model_table_dmabuf_info,
					GET_VA);
		if (ret) {
			mae_dev_info(mae_dev->dev, "%s, set model table dmabuf info fail\n",
					__func__);
			return ret;
		}
	}

	ret = dma_buf_begin_cpu_access(map_table->model_table_dmabuf_info.dmabuf,
									DMA_BIDIRECTIONAL);
	if (ret < 0) {
		mae_dev_info(mae_dev->dev, "%s, begin_cpu_access (%d,%d) failed\n",
					__func__, idx, MODEL_TABLE_PLANE);
		return -ENOMEM;
	}
	model_table = (struct ModelTable *)map_table->model_table_dmabuf_info.kva;

	if (model_table->clearCache) {
		mtk_mae_release_all_caches(mae_dev);
		model_table->clearCache = false;
	}

	// get va of param plane
	if (!map_table->param_dmabuf_info[idx].is_map) {
		ret = mtk_mae_set_dmabuf_info(mae_dev,
					buf->m.planes[PARAM_PLANE].m.fd,
					&map_table->param_dmabuf_info[idx],
					GET_VA);
		if (ret) {
			mae_dev_info(mae_dev->dev, "%s, set param dmabuf info fail\n",
					__func__);
			return ret;
		}
	}

	ret = dma_buf_begin_cpu_access(map_table->param_dmabuf_info[idx].dmabuf,
									DMA_BIDIRECTIONAL);
	if (ret < 0) {
		mae_dev_info(mae_dev->dev, "%s, begin_cpu_access (%d,%d) failed\n",
					__func__, idx, PARAM_PLANE);
		return -ENOMEM;
	}
	param = (struct EnqueParam *)map_table->param_dmabuf_info[idx].kva;

	mtk_mae_get_kernel_time(mae_dev, param, MAE_QBUF_START);

	mae_dev_dbg(mae_dev->dev, "%s, [check param] user(%d), imgMaxWidth(%d)",
				__func__, param->user, param->imgMaxWidth);
	mae_dev_dbg(mae_dev->dev, "imgMaxHeight(%d), isSecure(%d), FDModelSel(%d), FACModelSel(%d), ",
				param->imgMaxHeight, param->isSecure, param->FDModelSel, param->FACModelSel);
	mae_dev_dbg(mae_dev->dev, "attrFaceNumber(%d), aisegInputDegree(%d), maeMode(%d), requestNum(%d)",
				param->attrFaceNumber, param->aisegInputDegree, param->maeMode, param->requestNum);

	switch (param->maeMode) {
	case FD_V0:
	case FD_V1_IPN:
	case FD_V1_FPN:
		mae_dev_dbg(mae_dev->dev, "pyramidNumber(%d), fdInputDegree(%d), ",
					param->pyramidNumber, param->fdInputDegree);
		for (i = 0; i < param->pyramidNumber; i++) {
			mae_dev_dbg(mae_dev->dev, "Image %d: srcImgFmt(%d), imgWidth(%d), imgHeight(%d), ",
				i, param->image[i].srcImgFmt, param->image[i].imgWidth, param->image[i].imgHeight);
			mae_dev_dbg(mae_dev->dev, "enRoi(%d), (%d, %d) -> (%d, %d)\n",
				param->image[i].enRoi, param->image[i].roi.x1, param->image[i].roi.y1,
				param->image[i].roi.x2, param->image[i].roi.y2);
			mae_dev_dbg(mae_dev->dev, "enResize(%d), resizeWidth(%d), resizeHeight(%d)\n",
				param->image[i].enResize, param->image[i].resizeWidth, param->image[i].resizeHeight);
			mae_dev_dbg(mae_dev->dev, "enPadding(%d), (l, r, d, u) = (%d, %d, %d, %d)\n",
				param->image[i].enPadding, param->image[i].padding.left, param->image[i].padding.right,
				param->image[i].padding.down, param->image[i].padding.up);
		}
		break;
	case ATTR_V0:
	case FAC_V1:
		for (i = 0; i < param->attrFaceNumber; i++) {
			mae_dev_dbg(mae_dev->dev, "Image %d: attrInputDegree(%d), ",
				i, param->attrInputDegree[i]);
			mae_dev_dbg(mae_dev->dev, "srcImgFmt(%d), imgWidth(%d), imgHeight(%d), ",
				param->image[i].srcImgFmt, param->image[i].imgWidth, param->image[i].imgHeight);
			mae_dev_dbg(mae_dev->dev, "enRoi(%d), (%d, %d) -> (%d, %d)\n",
				param->image[i].enRoi, param->image[i].roi.x1, param->image[i].roi.y1,
				param->image[i].roi.x2, param->image[i].roi.y2);
			mae_dev_dbg(mae_dev->dev, ".enResize(%d), resizeWidth(%d), resizeHeight(%d)\n",
				param->image[i].enResize, param->image[i].resizeWidth, param->image[i].resizeHeight);
			mae_dev_dbg(mae_dev->dev, "enPadding(%d), (l, r, d, u) = (%d, %d, %d, %d)\n",
				param->image[i].enPadding, param->image[i].padding.left, param->image[i].padding.right,
				param->image[i].padding.down, param->image[i].padding.up);
		}
		break;
	case AISEG:
		mae_dev_dbg(mae_dev->dev, "srcImgFmt(%d), imgWidth(%d), imgHeight(%d), ",
			param->image[0].srcImgFmt, param->image[0].imgWidth, param->image[0].imgHeight);
		mae_dev_dbg(mae_dev->dev, "enResize(%d), resizeWidth(%d), resizeHeight(%d)\n",
			param->image[0].enResize, param->image[0].resizeWidth, param->image[0].resizeHeight);
		for (i = 0; i < AISEG_CROP_NUM; i++)
			mae_dev_dbg(mae_dev->dev, "(%d,%d->%d,%d), Map(%d), output(%d/%d), shiftBit(%d)\n",
				param->aisegCrop[i].x0, param->aisegCrop[i].y0,
				param->aisegCrop[i].x1, param->aisegCrop[i].y1,
				param->aisegCrop[i].featureMapSize, param->aisegCrop[i].outputSizeX,
				param->aisegCrop[i].outputSizeY, param->aisegCrop[i].shiftBit);
		break;
	default:
		break;
	}


	if (mae_dev->is_first_qbuf) {
		if (param->isSecure) {
			mae_dev_info(mae_dev->dev, "MAE SECURE MODE INIT!\n");

			mae_dev->is_secure = true;
			if (!mae_dev->is_shutdown) {
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					secure_init,
					mae_dev);
				call_mae_drv_ops(mae_dev->dev, g_mae_data.drv_ops,
					secure_enable,
					mae_dev);
			}
		}

		mae_dev->is_first_qbuf = false;
	}

	if (param->maeMode == AISEG) {
		// get pa of model
		ret = mtk_mae_acquire_cache(mae_dev,
				&mae_dev->aiseg_config_cache_list,
				model_table->configTable[MODEL_TYPE_AISEG].fd,
				&cache,
				GET_PA);
		if (ret || cache == NULL) {
			mae_dev_info(mae_dev->dev, "%s, aiseg model config acquire cache fail\n",
					__func__);
			return ret;
		}
		map_table->config_dmabuf_info[MODEL_TYPE_AISEG].pa = cache->info.pa;

		ret = mtk_mae_acquire_cache(mae_dev,
				&mae_dev->aiseg_coef_cache_list,
				model_table->coefTable[MODEL_TYPE_AISEG].fd,
				&cache,
				GET_PA);
		if (ret || cache == NULL) {
			mae_dev_info(mae_dev->dev, "%s, aiseg model coef acquire cache fail\n",
					__func__);
			return ret;
		}
		map_table->coef_dmabuf_info[MODEL_TYPE_AISEG].pa = cache->info.pa;

		// get pa of aiseg output
		for (i = 0; i < AISEG_MAP_NUM; i++) {
			if (model_table->aisegOutput[i].fd > 0) {
				ret = mtk_mae_acquire_cache(mae_dev,
					&mae_dev->aiseg_output_cache_list,
					model_table->aisegOutput[i].fd,
					&cache,
					GET_PA);
				if (ret || cache == NULL) {
					mae_dev_info(mae_dev->dev, "%s, aiseg output acquire cache fail\n",
							__func__);
					return ret;
				}
				map_table->aiseg_output_dmabuf_info[idx][i].pa = cache->info.pa;
			}
		}
	} else {
		// get pa of model
		for (i = 0; i < MODEL_TYPE_MAX; i++) {
			if (i == MODEL_TYPE_AISEG)
				continue;

			if (model_table->configTable[i].fd > 0 && model_table->configTable[i].isReady == 0) {

				mtk_mae_umap_detach(mae_dev, &map_table->config_dmabuf_info[i]);
				ret = mtk_mae_set_dmabuf_info(mae_dev,
							model_table->configTable[i].fd,
							&map_table->config_dmabuf_info[i],
							GET_PA);
				if (ret) {
					mae_dev_info(mae_dev->dev, "%s, set config(%d) dmabuf info fail\n",
						__func__, i);
					return ret;
				}

				model_table->configTable[i].isReady = 1;
			}

			if (model_table->coefTable[i].fd > 0 && model_table->coefTable[i].isReady == 0) {
				mtk_mae_umap_detach(mae_dev, &map_table->coef_dmabuf_info[i]);
				ret = mtk_mae_set_dmabuf_info(mae_dev,
							model_table->coefTable[i].fd,
							&map_table->coef_dmabuf_info[i],
							GET_PA);
				if (ret) {
					mae_dev_info(mae_dev->dev, "%s, set coef(%d) dmabuf info fail\n",
						__func__, i);
					return ret;
				}

				model_table->coefTable[i].isReady = 1;
			}
		}
	}

	// get pa of image
	mtk_mae_umap_detach(mae_dev, &map_table->image_dmabuf_info[idx]);
	ret = mtk_mae_set_dmabuf_info(mae_dev,
						buf->m.planes[IMAGE_PLANE_0].m.fd,
						&map_table->image_dmabuf_info[idx],
						GET_PA);
	if (ret) {
		mae_dev_info(mae_dev->dev, "%s, set image dmabuf info fail\n",
			__func__);
		return ret;
	}

	// get pa of output
	for (i = 0; i < MAX_PYRAMID_NUM; i++)
		if (!map_table->output_dmabuf_info[idx][i].is_attach) {
			ret = mtk_mae_set_dmabuf_info(mae_dev,
							buf->m.planes[OUTPUT_PLANE + i].m.fd,
							&map_table->output_dmabuf_info[idx][i],
							GET_BOTH); // DEBUG_ONLY
			if (ret) {
				mae_dev_info(mae_dev->dev, "%s, set output dmabuf info fail\n",
					__func__);
				return ret;
			}
		}

	// debug patch
	if (!map_table->debug_dmabuf_info[idx].is_map) {
		ret = mtk_mae_set_dmabuf_info(mae_dev,
					buf->m.planes[DEBUG_PLANE].m.fd,
					&map_table->debug_dmabuf_info[idx],
					GET_VA);
		if (ret) {
			mae_dev_info(mae_dev->dev, "%s, set debug dmabuf info fail\n",
					__func__);
			return ret;
		}
	}

	mtk_mae_get_kernel_time(mae_dev, param, MAE_QBUF_END);
#if M2M_ENABLE
	return v4l2_m2m_ioctl_qbuf(file, priv, buf);
#else
	// DEBUG_ONLY
	return vb2_ioctl_qbuf(file, priv, buf);
	pr_info("%s-", __func__);
#endif
}

static const struct v4l2_ioctl_ops mtk_mae_v4l2_video_out_ioctl_ops = {
	.vidioc_querycap = mtk_mae_querycap,
	.vidioc_enum_fmt_vid_out = mtk_mae_enum_fmt_out_mp,
	.vidioc_g_fmt_vid_out_mplane = mtk_mae_g_fmt_out_mp,
	.vidioc_s_fmt_vid_out_mplane = mtk_mae_s_fmt_out_mp,
	.vidioc_try_fmt_vid_out_mplane = mtk_mae_try_fmt_out_mp,
#if M2M_ENABLE
	.vidioc_enum_fmt_meta_cap = mtk_mae_enum_fmt_meta_cap,
	.vidioc_g_fmt_meta_cap = mtk_mae_g_fmt_meta_cap,
	.vidioc_s_fmt_meta_cap = mtk_mae_g_fmt_meta_cap,
	.vidioc_try_fmt_meta_cap = mtk_mae_g_fmt_meta_cap,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_dqbuf = mtk_mae_vioctl_dqbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
#else
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_streamon = vb2_ioctl_streamon,
	.vidioc_streamoff = vb2_ioctl_streamoff,
	.vidioc_create_bufs = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf = vb2_ioctl_prepare_buf,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
#endif
	.vidioc_qbuf = mtk_mae_vidioc_qbuf,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int mtk_mae_video_device_register(struct mtk_mae_dev *mae_dev)
{
	struct video_device *vdev = &mae_dev->vdev;
#if M2M_ENABLE
	struct v4l2_m2m_dev *m2m_dev = mae_dev->m2m_dev;
#endif
	struct device *dev = mae_dev->dev;
	int ret;

	vdev->fops = &mae_video_fops;
	vdev->release = video_device_release;
	vdev->lock = &mae_dev->vdev_lock;
	vdev->v4l2_dev = &mae_dev->v4l2_dev;
#if M2M_ENABLE
	vdev->vfl_dir = VFL_DIR_M2M;
	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE |
			   V4L2_CAP_META_CAPTURE;
#else
	vdev->vfl_dir = VFL_DIR_TX;
	vdev->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE;
#endif
	vdev->ioctl_ops = &mtk_mae_v4l2_video_out_ioctl_ops;

	strscpy(vdev->name, dev_driver_string(dev), sizeof(vdev->name));

	video_set_drvdata(vdev, mae_dev);

	// ret = video_register_device(vdev, VFL_TYPE_VIDEO, 0);
	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		mae_dev_info(dev, "Failed to register video device\n");
		return ret;
	}

#if M2M_ENABLE
	ret = v4l2_m2m_register_media_controller(
		m2m_dev, vdev, MEDIA_ENT_F_PROC_VIDEO_STATISTICS);
	if (ret) {
		mae_dev_info(dev, "Failed to init mem2mem media controller\n");
		goto err_unreg_video;
	}
#endif
	return 0;

#if M2M_ENABLE
err_unreg_video:
	video_unregister_device(vdev);
	return ret;
#endif
}

static int mtk_mae_dev_v4l2_init(struct mtk_mae_dev *mae_dev)
{
	struct media_device *mdev = &mae_dev->mdev;
	struct device *dev = mae_dev->dev;
	int ret;

	ret = v4l2_device_register(dev, &mae_dev->v4l2_dev);
	if (ret) {
		mae_dev_info(dev, "Failed to register v4l2 device\n");
		return ret;
	}

#if M2M_ENABLE
	mae_dev->m2m_dev = v4l2_m2m_init(&mae_m2m_ops);
	if (IS_ERR(mae_dev->m2m_dev)) {
		mae_dev_info(dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(mae_dev->m2m_dev);
		goto err_unreg_v4l2_dev;
	}
#endif

	mdev->dev = dev;
	ret = strscpy(mdev->model, dev_driver_string(dev), sizeof(mdev->model));
	if (ret < 0) {
		mae_dev_info(dev, "strscpy fail\n");
#if M2M_ENABLE
		goto err_unreg_v4l2_m2m_dev;
#else
		goto err_unreg_v4l2_dev;
#endif
	}

	ret = snprintf(mdev->bus_info, sizeof(mdev->bus_info), "platform:%s",
		 dev_name(dev));
	if (ret < 0) {
		mae_dev_info(dev, "snprintf fail\n");
#if M2M_ENABLE
		goto err_unreg_v4l2_m2m_dev;
#else
		goto err_unreg_v4l2_dev;
#endif
	}

	media_device_init(mdev);
	mdev->ops = &mae_m2m_media_ops;
	mae_dev->v4l2_dev.mdev = mdev;

	ret = mtk_mae_video_device_register(mae_dev);
	if (ret) {
		mae_dev_info(dev, "Failed to register video device\n");
		goto err_cleanup_mdev;
	}

	ret = media_device_register(mdev);
	if (ret) {
		mae_dev_info(dev, "Failed to register mem2mem media device\n");
		goto err_unreg_vdev;
	}

	return 0;

err_unreg_vdev:
#if M2M_ENABLE
	v4l2_m2m_unregister_media_controller(mae_dev->m2m_dev);
#endif
	video_unregister_device(&mae_dev->vdev);

err_cleanup_mdev:
	media_device_cleanup(mdev);

#if M2M_ENABLE
err_unreg_v4l2_m2m_dev:
	v4l2_m2m_release(mae_dev->m2m_dev);
#endif

err_unreg_v4l2_dev:
	v4l2_device_unregister(&mae_dev->v4l2_dev);
	return ret;
}



static int mtk_mae_suspend(struct device *dev)
{
	struct mtk_mae_dev *mae_dev = dev_get_drvdata(dev);
	int ret, num;

	num = atomic_read(&mae_dev->num_composing);
	mae_dev_info(dev, "%s: suspend mae job start, num(%d)\n", __func__, num);

	ret = wait_event_timeout
		(mae_dev->flushing_waitq,
		 !(num = atomic_read(&mae_dev->num_composing)),
		 msecs_to_jiffies(MTK_FD_HW_TIMEOUT));
	if (!ret && num) {
		mae_dev_info(dev, "%s: flushing mae job timeout, num(%d)\n",
			__func__, num);

		return -EBUSY;
	}

	if (!mae_dev->is_shutdown) {
		/* unavailable: 0 available: 1 */
		if (m_aov_notify != NULL)
			m_aov_notify(mae_dev->aov_pdev, AOV_NOTIFY_AIE_AVAIL, 1);
	}


	mae_dev_info(dev, "%s: suspend mae job end\n", __func__);

	return 0;
}

static int mtk_mae_resume(struct device *dev)
{
	struct mtk_mae_dev *mae_dev = dev_get_drvdata(dev);

	mae_dev_info(dev, "%s: resume mae job start\n", __func__);

	if (!mae_dev->is_shutdown) {
		if (m_aov_notify != NULL)
			m_aov_notify(mae_dev->aov_pdev, AOV_NOTIFY_AIE_AVAIL, 0);
	}

	mae_dev_info(dev, "%s: resume aie job end)\n", __func__);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int mae_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
	case PM_SUSPEND_PREPARE: /*enter suspend*/
		mtk_mae_suspend(mae_pm_dev);
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:    /*after resume*/
		mtk_mae_resume(mae_pm_dev);
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block mae_notifier_block = {
	.notifier_call = mae_pm_event,
	.priority = 0,
};
#endif

int mtk_mae_probe(struct platform_device *pdev)
{
	struct mtk_mae_dev *mae_dev;
	struct device *dev = &pdev->dev;
	int ret;
	struct resource *res;
	struct mtk_mae_ctx *ctx;
	struct mtk_mae_map_table *map_table;

	mae_dev_info(dev ,"%s+", __func__);

	mae_dev = devm_kzalloc(dev, sizeof(*mae_dev), GFP_KERNEL);
	if (!mae_dev)
		return -ENOMEM;

	memset(mae_dev, 0, sizeof(*mae_dev));

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->mae_dev = mae_dev;
	ctx->dev = mae_dev->dev;
	mae_dev->ctx = ctx;
	mae_dev->is_shutdown = false;

	map_table = devm_kzalloc(dev, sizeof(*map_table), GFP_KERNEL);
	if (!map_table)
		return -ENOMEM;

	memset(map_table, 0, sizeof(*map_table));
	mae_dev->map_table = map_table;

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		mae_dev_info(dev, "%s: No suitable DMA available\n", __func__);

	if (!dev->dma_parms) {
		dev->dma_parms =
			devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms)
			return -ENOMEM;
	}

	ret = dma_set_max_seg_size(dev, UINT_MAX);
	if (ret)
		mae_dev_info(dev, "Failed to set DMA segment size\n");

	dev_set_drvdata(dev, mae_dev);
	mae_dev->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mae_dev->mae_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(mae_dev->mae_base)) {
		mae_dev_info(dev, "Failed to get mae reg base\n");
		return PTR_ERR(mae_dev->mae_base);
	}

	/* Larb init */
	ret = mtk_mae_dev_larb_init(mae_dev);
	if (ret)
		dev_info(dev, "Failed to init larb : %d\n", ret);

	mae_dev->mae_clt = cmdq_mbox_create(dev, 0);
	if (!mae_dev->mae_clt)
		mae_dev_info(dev, "cmdq mbox create fail\n");
	else
		mae_dev_info(dev, "cmdq mbox create done\n");

#if MAE_CMDQ_SEC_READY
	mae_dev->mae_secure_clt = cmdq_mbox_create(dev, 1);

	if (!mae_dev->mae_secure_clt)
		mae_dev_info(dev, "secure cmdq mbox create fail\n");
	else
		mae_dev_info(dev, "secure cmdq mbox create done\n");

	of_property_read_u32(pdev->dev.of_node, "sw-sync-token-tzmp-aie-wait",
				&(mae_dev->mae_sec_wait));
	mae_dev_info(mae_dev->dev, "mae_sec_wait is %d\n", mae_dev->mae_sec_wait);
	of_property_read_u32(pdev->dev.of_node, "sw-sync-token-tzmp-aie-set",
				&(mae_dev->mae_sec_set));
	mae_dev_info(mae_dev->dev, "mae_sec_set is %d\n", mae_dev->mae_sec_set);
#endif

	of_property_read_u32(pdev->dev.of_node, "fdvt-frame-done",
						 &(mae_dev->mae_event_id));

	mutex_init(&mae_dev->vdev_lock);
	init_completion(&mae_dev->mae_job_finished);
	init_waitqueue_head(&mae_dev->flushing_waitq);
	atomic_set(&mae_dev->num_composing, 0);

	mutex_init(&mae_dev->mae_device_lock);
	mutex_init(&mae_dev->mae_stream_lock);
	mae_dev->open_video_device_cnt = 0;
	mae_dev->mae_stream_count = 0;

	// MAE_TO_DO: Init workqueue
	mae_dev->frame_done_wq =
			alloc_ordered_workqueue(dev_name(mae_dev->dev),
						WQ_HIGHPRI | WQ_FREEZABLE);
	if (!mae_dev->frame_done_wq) {
		mae_dev_info(mae_dev->dev, "failed to alloc frame_done workqueue\n");
		mutex_destroy(&mae_dev->vdev_lock);
		return -ENOMEM;
	}
	INIT_WORK(&mae_dev->req_work.work, mtk_mae_frame_done_worker);
	mae_dev->req_work.mae_dev = mae_dev;

	mae_dev->aov_pdev = pdev;

	pm_runtime_enable(dev);

	ret = mtk_mae_dev_v4l2_init(mae_dev);
	if (ret) {
		mae_dev_info(dev, "Failed to init v4l2 device: %d\n", ret);
		goto err_destroy_mutex;
	}

	mae_pm_dev = dev;
#if IS_ENABLED(CONFIG_PM)
	ret = register_pm_notifier(&mae_notifier_block);
	if (ret) {
		mae_dev_info(dev, "failed to register notifier block.\n");
		return ret;
	}
#endif

	mae_dev->smmu_dev = mtk_smmu_get_shared_device(dev);
	if (!mae_dev->smmu_dev) {
		mae_dev_info(dev,
			"%s: failed to get mae smmu device\n",
			__func__);
		return -EINVAL;
	}

	INIT_LIST_HEAD(&mae_dev->aiseg_config_cache_list);
	INIT_LIST_HEAD(&mae_dev->aiseg_coef_cache_list);
	INIT_LIST_HEAD(&mae_dev->aiseg_output_cache_list);

	mae_dev_info(dev ,"%s-", __func__);
	return 0;

err_destroy_mutex:
	pm_runtime_disable(dev);
	destroy_workqueue(mae_dev->frame_done_wq);
	mutex_destroy(&mae_dev->vdev_lock);

	return ret;
}

int mtk_mae_remove(struct platform_device *pdev)
{
	struct mtk_mae_dev *mae_dev = dev_get_drvdata(&pdev->dev);

	video_unregister_device(&mae_dev->vdev);
	media_device_cleanup(&mae_dev->mdev);
#if M2M_ENABLE
	v4l2_m2m_unregister_media_controller(mae_dev->m2m_dev);
	v4l2_m2m_release(mae_dev->m2m_dev);
#endif
	v4l2_device_unregister(&mae_dev->v4l2_dev);

	// MAE_TO_DO
	pm_runtime_disable(&pdev->dev);
	// fd->frame_done_wq = NULL;

	mutex_destroy(&mae_dev->vdev_lock);
	return 0;
}

static void mtk_mae_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_mae_dev *mae_dev = dev_get_drvdata(&pdev->dev);

	mae_dev->is_shutdown = true;
	if (mae_dev->mae_clt)
		cmdq_mbox_stop(mae_dev->mae_clt);
	else
		mae_dev_info(dev, "%s: mae cmdq client is NULL\n", __func__);

	mae_dev_info(dev, "%s: mae shutdown ready(%d)\n",
		__func__, mae_dev->is_shutdown);
}

static const struct of_device_id mtk_mae_of_ids[] = {
	{
		.compatible = "mediatek,mtk-mae",
	},{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, mtk_mae_of_ids);

static struct platform_driver mtk_mae_driver = {
	.probe = mtk_mae_probe,
	.remove = mtk_mae_remove,
	.shutdown = mtk_mae_shutdown,

	.driver = {
		.name = "mtk-mae",
		.of_match_table = of_match_ptr(mtk_mae_of_ids),
		// .pm = &mtk_aie_pm_ops, // no used
	}
};

module_platform_driver(mtk_mae_driver);
MODULE_AUTHOR("Ming-Hsuan Chaing <ming-hsuan.chiang@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_DESCRIPTION("Mediatek MAE driver");
