/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __MTK_CAM_UT_SENINF_H
#define __MTK_CAM_UT_SENINF_H

enum tm_pix_mode {
	tm_pix_mode_8 = 0,
	tm_pix_mode_16,
};

enum tg_enum {
	camsv_tg_0,
	camsv_tg_1,
	camsv_tg_2,
	camsv_tg_3,
	camsv_tg_4,
	camsv_tg_5,
	raw_tg_0,
	raw_tg_1,
	raw_tg_2,
	pdp_tg_0,
	pdp_tg_1,
	pdp_tg_2,
	pdp_tg_3,
	uisp_tg_0,
	camsys_tg_max,
};

/* align mt6899 dts */
enum tg_enum_remap {
	camsv_tg_0_remap,
	camsv_tg_1_remap,
	camsv_tg_2_remap,
	camsv_tg_3_remap,
	camsv_tg_4_remap,
	raw_tg_0_remap,
	raw_tg_1_remap,
	raw_tg_2_remap,
	pdp_tg_0_remap,
	pdp_tg_1_remap,
	pdp_tg_2_remap,
	camsys_tg_max_remap,
};

enum testmdl_exp_no {
	testmdl_exp1 = 0,
	testmdl_exp2,
	testmdl_exp3,
};

enum tag_enum {
	tag_0 = 0,
	tag_1,
	tag_2,
	tag_3,
	tag_4,
	tag_5,
	tag_6,
	tag_7,
	tag_max,
};


struct mtk_cam_ut_tm_para {
	enum tg_enum tg_idx;
	enum testmdl_exp_no exp_no;
	enum tag_enum tag;
	enum tm_pix_mode pixmode;
};

#endif /* __MTK_CAM_UT_SENINF_H */
