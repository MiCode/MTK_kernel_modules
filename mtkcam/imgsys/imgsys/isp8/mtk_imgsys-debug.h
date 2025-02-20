/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Christopher Chen <christopher.chen@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_DEBUG_H_
#define _MTK_IMGSYS_DEBUG_H_

#include "mtk_imgsys-dev.h"
#include "mtk_imgsys-cmdq.h"
#include "mtk_imgsys-module.h"
#include "mtk_imgsys-engine-isp8.h"
#include "mtk_imgsys-isp-debug.h"
/**
 * enum IMGSYS_DL_PATH_E
 *
 * Definition about supported direct link path
 */
enum IMGSYS_DL_PATH_E {
	IMGSYS_DL_WPE_EIS_TO_TRAW_LTRAW = 0,
	IMGSYS_DL_WPE_EIS_TO_DIP = 1,
	IMGSYS_DL_TRAW_TO_DIP = 4,
	IMGSYS_DL_LTRAW_TO_DIP = 5,
	IMGSYS_DL_DIP_TO_PQDIP_A = 6,
	IMGSYS_DL_DIP_TO_PQDIP_B = 7,
	IMGSYS_DL_PQDIP_A_TO_ADLWR_0 = 8,
	IMGSYS_DL_PQDIP_B_TO_ADLWR_1 = 9, /* ADLWR1 isn't used anymore */
	IMGSYS_DL_OMC_TNR_TO_TRAW_LTRAW = 10,
	IMGSYS_DL_OMC_TNR_TO_DIP = 11,
	IMGSYS_DL_WPE_LITE_TO_TRAW_LTRAW = 12,
	IMGSYS_DL_WPE_LITE_TO_ADLWR_0 = 13,
	IMGSYS_DL_OMC_LITE_TO_TRAW_LTRAW = 14,
	IMGSYS_DL_WPE_LITE_TO_PQDIP_A_B = 15,
	IMGSYS_DL_WPE_EIS_TO_TRAW  = 22,
	IMGSYS_DL_OMC_TNR_TO_TRAW = 23,
	IMGSYS_DL_WPE_LITE_TO_TRAW = 24,

	IMGSYS_DL_ADLRD_EVEN_TO_TRAW = 25,
	IMGSYS_DL_ADLRD_ODD_TO_TRAW = 26,
	IMGSYS_DL_ADLRD_0_TO_TRAW = 27,
	IMGSYS_DL_OMC_LITE_TO_TRAW = 28,

  IMGSYS_DL_NO_CHECK_SUM_DUMP = 0x80,
};

#define IMGSYS_ENG_NAME_LEN 16

struct imgsys_dbg_engine_t {
	enum mtk_imgsys_engine eng_e;
	char eng_name[IMGSYS_ENG_NAME_LEN];
};

void imgsys_dl_debug_dump(struct mtk_imgsys_dev *imgsys_dev, unsigned int hw_comb);
void imgsys_debug_dump_routine(struct mtk_imgsys_dev *imgsys_dev,
	const struct module_ops *imgsys_modules, int imgsys_module_num,
	unsigned int hw_comb);
void imgsys_main_init(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_main_set_init(struct mtk_imgsys_dev *imgsys_dev);
void imgsys_main_cmdq_set_init(struct mtk_imgsys_dev *imgsys_dev, void *pkt, int hw_idx);
void imgsys_main_uninit(struct mtk_imgsys_dev *imgsys_dev);

bool imgsys_dip_7sp_dbg_enable(void);
bool imgsys_traw_7sp_dbg_enable(void);
bool imgsys_wpe_7sp_dbg_enable(void);
bool imgsys_omc_8_dbg_enable(void);
bool imgsys_pqdip_7sp_dbg_enable(void);
bool imgsys_me_7sp_dbg_enable(void);

#endif /* _MTK_IMGSYS_DEBUG_H_ */
