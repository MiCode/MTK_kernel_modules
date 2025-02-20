/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_UT_H
#define __MTK_CAM_UT_H

#include <linux/cdev.h>
#include <linux/rpmsg.h>
#include <linux/wait.h>

#include "camsys/isp8/cam/mtk_cam-ipi.h"
#include "mtk_cam_ut-event.h"
#include "mtk_cam_ut-seninf.h"
#include "mtk_cam_ut-utils.h"

#define IPI_FRAME_BUF_SIZE		ALIGN(sizeof(struct mtkcam_ipi_frame_param), SZ_1K)
#define IPI_FRAME_BUF_NUM		3
#define IPI_FRAME_BUF_TOTAL_SIZE	(IPI_FRAME_BUF_SIZE * IPI_FRAME_BUF_NUM)

#define CAM_MAX_DMA (CAM_MAX_IMAGE_OUTPUT * CAM_MAX_PLANENUM \
			+ CAM_MAX_META_OUTPUT + CAM_MAX_PIPE_USED)

// align TestMdlMode in testPlan.h
enum testmdl_enum {
	testmdl_disable		= 0,
	testmdl_normal		= 1,
	testmdl_stagger_1exp	= 2,
	testmdl_stagger_2exp	= 3,
	testmdl_stagger_3exp	= 4,
	testmdl_rgbw		= 5,
};

enum streaming_enum {
	streaming_off		= 0,
	streaming_vf		= 1,
	streaming_active	= 2,
};

enum isp_hardware_enum { //sync with TestPlan.h
	WITH_NONE		= 0x0,
	WITH_RAW		= 0x1,
	SINGLE_SV		= 0x2,
	SINGLE_MRAW		= 0x4,
};

struct mtk_cam_ut_mem_obj {
	int fd;
	dma_addr_t iova;
	uint32_t size;
	void *va;

	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct dma_buf *dma_buf;
};

struct mtk_cam_msg_buf {
	void *va;
	int size;
};

struct mtk_cam_ut_buf_entry {
	unsigned int frame_seq_no;
	struct mtk_cam_ut_mem_obj cq_buf;
	struct mtk_cam_ut_mem_obj dma_buf[CAM_MAX_DMA];
	struct mtk_cam_msg_buf msg_buffer;
	struct list_head list_entry;
	unsigned int  cq_offset;
	unsigned int  sub_cq_size;
	unsigned int  sub_cq_offset;
};

struct mtk_cam_ut_buf_list {
	struct list_head list;
	u32 cnt;
	spinlock_t lock;
};

struct dma_debug_item {
	unsigned int	debug_sel;
	const char	*msg;
};

struct mtk_cam_ut;
struct mtk_cam_ut_event_handler {
	int (*on_ipi_composed)(struct mtk_cam_ut *ut);

	int (*on_isr_sof)(struct mtk_cam_ut *ut);
	int (*on_isr_sv_sof)(struct mtk_cam_ut *ut);
	int (*on_isr_mraw_sof)(struct mtk_cam_ut *ut);
	int (*on_isr_cq_done)(struct mtk_cam_ut *ut);
	int (*on_isr_frame_done)(struct mtk_cam_ut *ut);
};

struct mtk_cam_ut_vcore_device {
	struct device *dev;
	struct clk **clks;
	unsigned int num_clks;
};

struct mtk_cam_ut {
	struct device *dev;

	dev_t devno;
	struct cdev cdev;
	struct class *class;

	int num_raw;
	int num_yuv;
	int num_rms;
	int num_larb;
	int num_camsv;
	int num_mraw;

	struct device *seninf;
	struct device **raw;
	struct device **yuv;
	struct device **rms;
	struct device **larb;
	struct device **camsv;
	struct device **mraw;

	struct mtk_cam_ut_mem_obj *mem;
	struct mtk_cam_ut_mem_obj *msg_mem;
	void __iomem *base;
	void __iomem *adlrd_base;

	phandle rproc_phandle;
	struct rproc *rproc_handle;
	struct rpmsg_channel_info rpmsg_channel;
	struct mtk_rpmsg_device *rpmsg_dev;

	struct mtk_cam_ut_buf_list enque_list;
	struct mtk_cam_ut_buf_list processing_list;
	struct mtk_cam_ut_buf_list deque_list;
	wait_queue_head_t done_wq;

	struct ut_event_listener listener;
	struct clk **clks;

	/* config related */
	int with_testmdl;
	int subsample;
	int hardware_scenario;
	int main_rawi;
	enum isp_hardware_enum isp_hardware;
	int raw_module;
	unsigned int num_clks;

	struct mtk_cam_ut_event_handler hdl;

	spinlock_t spinlock_irq;
	int m2m_available;
};

struct mtk_cam_ut_data {
	const char		platform[8];
};

#endif /* __MTK_CAM_UT_H */
