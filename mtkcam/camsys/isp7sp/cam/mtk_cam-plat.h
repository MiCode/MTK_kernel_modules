/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_PLAT_H
#define __MTK_CAM_PLAT_H

#include <linux/types.h>
#include <linux/media.h>
#include <media/v4l2-subdev.h>
#include "mtk_cam-raw_pads.h"

enum mraw_dmao_id {
	imgo_m1 = 0,
	imgbo_m1,
	cpio_m1,
	mraw_dmao_num
};

enum camsv_module_id {
	CAMSV_START = 0,
	CAMSV_0 = CAMSV_START,
	CAMSV_1 = 1,
	CAMSV_2 = 2,
	CAMSV_3 = 3,
	CAMSV_4 = 4,
	CAMSV_5 = 5,
	CAMSV_END
};

struct mraw_stats_cfg_param {
	s8  mqe_en;
	s8  mobc_en;
	s8  plsc_en;
	s8  lm_en;
	s8  dbg_en;

	u32 crop_width;
	u32 crop_height;

	u32 mqe_mode;

	u32 mbn_hei;
	u32 mbn_pow;
	u32 mbn_dir;
	u32 mbn_spar_hei;
	u32 mbn_spar_pow;
	u32 mbn_spar_fac;
	u32 mbn_spar_con1;
	u32 mbn_spar_con0;

	u32 cpi_th;
	u32 cpi_pow;
	u32 cpi_dir;
	u32 cpi_spar_hei;
	u32 cpi_spar_pow;
	u32 cpi_spar_fac;
	u32 cpi_spar_con1;
	u32 cpi_spar_con0;

	u32 img_sel;
	u32 imgo_sel;

	u32 lm_mode_ctrl;
};

struct sv_dma_th_setting {
	u32 urgent_th;
	u32 ultra_th;
	u32 pultra_th;
	u32 dvfs_th;

	u32 urgent_th2;
	u32 ultra_th2;
	u32 pultra_th2;
	u32 dvfs_th2;

	u32 urgent_len1_th;
	u32 ultra_len1_th;
	u32 pultra_len1_th;
	u32 dvfs_len1_th;

	u32 urgent_len2_th;
	u32 ultra_len2_th;
	u32 pultra_len2_th;
	u32 dvfs_len2_th;

	u32 cq1_fifo_size;
	u32 cq1_urgent_th;
	u32 cq1_ultra_th;
	u32 cq1_pultra_th;
	u32 cq1_dvfs_th;

	u32 cq2_fifo_size;
	u32 cq2_urgent_th;
	u32 cq2_ultra_th;
	u32 cq2_pultra_th;
	u32 cq2_dvfs_th;
};

struct mraw_dma_th_setting {
	u32 urgent_th;
	u32 ultra_th;
	u32 pultra_th;
	u32 dvfs_th;
	u32 fifo_size;
};

struct mraw_cq_th_setting {
	u32 cq1_fifo_size;
	u32 cq1_urgent_th;
	u32 cq1_ultra_th;
	u32 cq1_pultra_th;
	u32 cq1_dvfs_th;

	u32 cq2_fifo_size;
	u32 cq2_urgent_th;
	u32 cq2_ultra_th;
	u32 cq2_pultra_th;
	u32 cq2_dvfs_th;
};

struct dma_info {
	u32 width;
	u32 height;
	u32 xsize;
	u32 stride;
};

struct set_meta_stats_info_param {
	unsigned int cfg_dataformat;
	void *meta_cfg;
	size_t meta_cfg_size;

	/* raw input size */
	unsigned int width;
	unsigned int height;

	int bin_ratio; /* 1/2/3/... */

	int rgbw;
};

struct plat_v4l2_data {
	int raw_pipeline_num;
	int camsv_pipeline_num;
	int mraw_pipeline_num;

	unsigned int meta_major;
	unsigned int meta_minor;

	int meta_cfg_size;
	int meta_cfg_size_rgbw;
	int meta_stats0_size;
	int meta_stats0_size_rgbw;
	int meta_stats1_size;
	int meta_stats1_size_rgbw;
	int meta_sv_ext_size;
	int meta_mraw_ext_size;

	int timestamp_buffer_ofst;
	int reserved_camsv_dev_id;

	u8 *vb2_queues_support_list;
	int vb2_queues_support_list_num;

	int (*set_meta_stats_info)(int ipi_id, void *addr, size_t size,
				   const struct set_meta_stats_info_param *p);
	int (*get_meta_stats_port_size)(int ipi_id, void *addr, int dma_port, int *size);
	int (*set_sv_meta_stats_info)(int ipi_id, void *addr, struct dma_info *info);
	int (*get_sv_dma_th_setting)(unsigned int dev_id, unsigned int fifo_img_p1,
		unsigned int fifo_img_p2, unsigned int fifo_len_p1, unsigned int fifo_len_p2,
		struct sv_dma_th_setting *th_setting);
	int (*get_sv_two_smi_setting)(int *sv_two_smi_en, int *sv_support_two_smi_out);
	int (*get_single_sv_opp_idx)(unsigned int * opp_idx);
	int (*get_mraw_dmao_common_setting)(struct mraw_dma_th_setting *mraw_th_setting,
		struct mraw_cq_th_setting *mraw_cq_setting);
	int (*set_mraw_meta_stats_info)(int ipi_id, void *addr, struct dma_info *info);
	int (*get_mraw_stats_cfg_param)(void *addr, struct mraw_stats_cfg_param *param);
};

struct plat_data_hw {
	u32 camsys_axi_mux;
	u32 platform_id;
	int cammux_id_raw_start;
	int raw_icc_path_num;
	int yuv_icc_path_num;
	int max_main_pipe_w;
	int max_main_pipe_twin_w;
	int pixel_mode_max;
	/**
	 * In 6989 and 6897, if the width < 1920, hw can't accept 2 pixel mode.
	 * pixel_mode_contraints is to enable the corrosponding protection.
	 */
	bool has_pixel_mode_contraints;

	int (*query_raw_dma_group)(int m4u_id, u32 group[4]);
	int (*query_yuv_dma_group)(int m4u_id, u32 group[4]);

	int (*query_caci_size)(int w, int h, size_t *size);
	int (*query_max_exp_support)(u32 raw_idx);

	int (*query_icc_path_idx)(int domain, int smi_port);

	bool dcif_slb_support;
};

struct camsys_platform_data {
	const char			platform[8];
	const struct plat_v4l2_data	*v4l2;
	const struct plat_data_hw	*hw;
};

extern const struct camsys_platform_data *cur_platform;
void set_platform_data(const struct camsys_platform_data *platform_data);

#define CALL_PLAT_V4L2(ops, ...) \
	((cur_platform && cur_platform->v4l2 && cur_platform->v4l2->ops) ? \
	 cur_platform->v4l2->ops(__VA_ARGS__) : -EINVAL)

#define CALL_PLAT_HW(ops, ...) \
	((cur_platform && cur_platform->hw && cur_platform->hw->ops) ? \
	 cur_platform->hw->ops(__VA_ARGS__) : -EINVAL)

#define GET_PLAT_V4L2(member) (cur_platform->v4l2->member)

#define GET_PLAT_HW(member) (cur_platform->hw->member)

/* platform data list */
#ifdef CAMSYS_ISP7SP_MT6878
extern struct camsys_platform_data mt6878_data;
#endif
#ifdef CAMSYS_ISP7SP_MT6897
extern struct camsys_platform_data mt6897_data;
#endif
#ifdef CAMSYS_ISP7SP_MT6989
extern struct camsys_platform_data mt6989_data;
#endif

#define FIFO_THRESHOLD(FIFO_SIZE, HEIGHT_RATIO, LOW_RATIO) \
	(((FIFO_SIZE * HEIGHT_RATIO) & 0xFFF) << 16 | \
	((FIFO_SIZE * LOW_RATIO) & 0xFFF))


#endif /*__MTK_CAM_PLAT_H*/
