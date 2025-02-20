/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2018 MediaTek Inc.
 *
 */
#ifndef MTK_IMGSYS_FRM_SYNC_H
#define MTK_IMGSYS_FRM_SYNC_H

#include <linux/cdev.h>
#include "mtk_imgsys_frm_sync_event.h"

/*token layout definition*/
struct group {
	int algo_group_id;
	int hw_group_id;
};


enum hw_module {
	imgsys_engine = 1,
	dpe_engine,
	mae_engine
};

enum algo_module {
	algo_mcnr = 0,
	algo_vsdof,
	algo_aiseg,
	algo_all
};

enum token_type {
	imgsys_token_none = -1,
	imgsys_token_set = 0,
	imgsys_token_wait = 1
};

#define STOKEN_NUM 16

struct token_info_k {
	uint32_t token_value;
	enum token_type type;
};
struct imgsys_control_meta_token_kernel {
	uint32_t mSyncTokenNum;
	struct token_info_k mSyncTokenList[16];
	//uint32_t mSyncTokenNotifyList[16];
	//uint32_t mSyncTokenWaitList[16];
};


struct imgsys_in_data {
	uint64_t frm_owner;
	uint64_t imgstm_inst;
	uint32_t req_fd;
	uint32_t req_no;
	uint32_t frm_no;
	uint32_t stage;
	uint32_t r_idx;
	uint32_t is_vss;
	uint32_t is_capture;
	uint32_t is_smvr;
	uint32_t hw_comb;
	bool sync_next;
	bool sync_prev;
	bool is_first_frm;
	bool is_last_frm;
	uint32_t sync_id;
	uint32_t gp_id;
	uint32_t stoken_num;
	struct imgsys_control_meta_token_kernel token_list;
	uint32_t evthistb_idx;
	struct sw_info_t *sw_info;
	uint32_t frm_sync_token_list;
};

struct imgsys_out_data {
	uint32_t event_id;
	uint32_t sw_ridx;
};

struct imgsys_deque_done_in_data {
	uint32_t sw_ridx;
};

struct dpe_in_data {
	uint64_t frm_owner;
	uint64_t imgstm_inst;
	uint32_t req_fd;
	uint32_t req_no;
	uint32_t frm_no;
	struct imgsys_control_meta_token_kernel token_info;
};

struct dpe_out_data {
	uint32_t event_id;
	uint32_t req_fd;
};

struct dpe_deque_done_in_data {
	uint32_t req_fd;
};

struct mae_in_data {
	uint64_t frm_owner;
	uint64_t imgstm_inst;
	uint32_t req_fd;
	uint32_t req_no;
	uint32_t frm_no;
	struct imgsys_control_meta_token_kernel token_info;
};

struct mae_out_data {
	uint32_t event_id;
};

struct mae_deque_done_in_data {
	uint32_t sw_ridx;
};

struct mtk_img_frm_sync {
	struct device *dev;
	dev_t frm_sync_devno;
	struct cdev frm_sync_cdev;
	struct class *frm_sync_class;
	struct device *frm_sync_device;
	const char *frm_sync_name;
	const struct mtk_img_frm_sync_data *data;
	bool   is_open;
};

int mtk_imgsys_frm_sync_init(struct platform_device *pdev, struct group group);

int mtk_imgsys_frm_sync_uninit(struct platform_device *pdev, struct group group);

int Handler_frame_token_sync_imgsys(struct platform_device *pdev, struct imgsys_in_data *in_data,
	struct imgsys_out_data *out_data);

int Handler_frame_token_sync_DPE(struct platform_device *pdev, struct dpe_in_data *in_data,
	struct dpe_out_data *out_data);

int Handler_frame_token_sync_MAE(struct platform_device *pdev, struct mae_in_data *in_data,
	struct mae_out_data *out_data);

int release_frame_token_imgsys(struct platform_device *pdev,
	struct imgsys_deque_done_in_data *in_data);

int release_frame_token_DPE(struct platform_device *pdev, struct dpe_deque_done_in_data *in_data);

int release_frame_token_MAE(struct platform_device *pdev, struct mae_deque_done_in_data *in_data);

int clear_token_user(struct platform_device *pdev, unsigned long frm_owner,
	unsigned long imgstm_inst);

int mtk_imgsys_frm_sync_timeout(struct platform_device *pdev, int gce_event_id);

struct mtk_img_frm_sync_data {
	int (*open)(void);
	int (*init)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group group);
	int (*uninit)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev, struct group group);
	int (*Handler_frame_token_sync_imgsys)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct imgsys_in_data *in_data, struct imgsys_out_data *out_data) ;
	int (*Handler_frame_token_sync_DPE)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct dpe_in_data *in_data, struct dpe_out_data *out_data);
	int (*Handler_frame_token_sync_MAE)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct mae_in_data *in_data, struct mae_out_data *out_data);
	int (*release_frame_token_imgsys)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct imgsys_deque_done_in_data *in_data);
	int (*release_frame_token_DPE)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct dpe_deque_done_in_data *in_data);
	int (*release_frame_token_MAE)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		struct mae_deque_done_in_data *in_data);
	int (*clear_token_user)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		unsigned long frm_owner, unsigned long imgstm_inst);
	int (*frm_sync_timeout)(struct mtk_img_frm_sync *mtk_img_frm_sync_dev,
		int gce_event_id);
};

#endif /* MTK_IMGSYS_FRM_SYNC_H */
