/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_QOF_H
#define __MTK_CAM_QOF_H

struct mtk_raw_device;
struct mtk_cam_ctx;
struct mtk_cam_device;

struct qof_voter_handle {
	int used_raw;
};

#define qof_dump_ctx(ctx, func) \
	do { \
		int i; \
		for (i = 0; i < ARRAY_SIZE(ctx->hw_raw); i++) { \
			if (ctx->hw_raw[i]) { \
				struct mtk_raw_device *raw_dev = \
					dev_get_drvdata(ctx->hw_raw[i]); \
				func(raw_dev); \
			} \
		} \
	} while (0)

int qof_reset(struct mtk_raw_device *dev);

void qof_setup_ctrl(struct mtk_raw_device *dev, int on);
void qof_sof_src_sel(struct mtk_raw_device *dev,
	int exp_num, bool with_tg, int sv_last_tag);
void qof_init_timer_freq(struct mtk_raw_device *dev);
void qof_setup_hw_timer(struct mtk_raw_device *dev, u32 interval_us);
void qof_setup_rtc(struct mtk_raw_device *dev);
int qof_setup_twin(struct mtk_raw_device *dev, bool is_master, bool next_raw);
void qof_set_cq_start_max(struct mtk_raw_device *dev, int scq_ms);

bool qof_is_enabled(struct mtk_raw_device *dev);
int qof_enable(struct mtk_raw_device *dev, bool enable);
int qof_enable_cq_trigger_by_qof(struct mtk_raw_device *dev, bool enable);

int __qof_mtcmos_voter(struct mtk_cam_engines *eng,
	unsigned int used_engine, bool enable,
	const char *caller);
#define qof_mtcmos_voter(eng, used_raw, enable) \
	__qof_mtcmos_voter(eng, used_raw, enable, __func__)

int __qof_mtcmos_raw_voter(struct mtk_raw_device *raw, bool enable, const char *caller);
#define qof_mtcmos_raw_voter(raw, enable) \
	__qof_mtcmos_raw_voter(raw, enable, __func__)

void __qof_mtcmos_voter_handle(struct mtk_cam_engines *eng,
	unsigned int used_raw, struct qof_voter_handle *handle,
	const char *caller);
#define qof_mtcmos_voter_handle(eng, used_raw, handle) \
	__qof_mtcmos_voter_handle(eng, used_raw, handle, __func__)

int qof_reset_mtcmos_voter(struct mtk_cam_ctx *ctx);
int qof_reset_mtcmos_raw_voter(struct mtk_raw_device *raw);
void qof_ddren_setting(struct mtk_raw_device *raw, int frm_time_us);

u32 qof_on_off_cnt(struct mtk_raw_device *raw);

void qof_dump_trigger_cnt(struct mtk_raw_device *dev);
void qof_dump_voter(struct mtk_raw_device *dev);
void qof_dump_power_state(struct mtk_raw_device *raw);
void qof_dump_hw_timer(struct mtk_raw_device *raw);
void qof_dump_cq_addr(struct mtk_raw_device *raw);
void qof_dump_ctrl(struct mtk_raw_device *raw);
void qof_dump_qoftop_status(struct mtk_raw_device *raw);
void qof_dump_int_en_addr(struct mtk_raw_device *raw);

void qof_set_force_dump(struct mtk_raw_device *raw, bool en);
void qof_force_dump_all(struct mtk_raw_device *raw);

u32 qof_get_mtcmos_margin(void);

void mtk_cam_enable_itc(struct mtk_raw_device *raw);
void mtk_cam_reset_itc(struct mtk_cam_device *cam);

#endif /*__MTK_CAM_QOF_H */
