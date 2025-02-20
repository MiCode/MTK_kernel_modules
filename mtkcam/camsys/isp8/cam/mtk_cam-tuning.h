/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_TUNING_H
#define __MTK_CAM_TUNING_H

#define CAM_TUNING_BEGIN_F_RATIO 50
#define CAM_TUNING_DELAY_NS      0
#define CAM_TUNING_DEADLINE_NS   2000000

#define MTK_CAM_LSCI_TABLE_SIZE 32768

struct mtk_cam_tuning {
	/* sensor */
	u32 sensor_mode;
	u32 width;
	u32 height;
	u64 readout_ns;
	u64 exp_time_ns;
	u64 sof_boottime_ns;

	/* lsc */
	u32 x_num;
	u32 y_num;
	u32 block_width;
	u32 block_height;

	/* shading table */
	unsigned int *shading_tbl;
	struct vb2_buffer *meta_cfg_vb2_buf;

	/* NDD */
	int seq_num;
	bool normal_dump_enabled;

	/* timestamp */
	u64 begin_ts_ns;
	u64 end_ts_ns;
};


void mtk_cam_tuning_probe(void);

void mtk_cam_tuning_init(struct mtk_cam_tuning *param);

void mtk_cam_tuning_update(struct mtk_cam_tuning *param);

#endif /*__MTK_CAM_TUNING_H*/
