/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 * Author: Jenny Hsu <jenny.hsu@mediatek.com>
 */

#ifndef __MTK_CAM_UT_UTILS_H__
#define __MTK_CAM_UT_UTILS_H__

#include "camsys/isp7sp/cam/mtk_cam-defs.h"

static inline int is_on_the_fly(const int hw_scenario);
static inline int is_direct_couple(const int hw_scenario);
static inline int is_stagger(const int hw_scenario);
static inline int is_mstream(const int hw_scenario);
static inline int is_dcif_required(const int hw_scenario);
static inline int is_tg_required(const int hw_scenario);
static inline int is_rawi_triggered_by_raw_sof(const int hw_scenario);
static inline int is_raw_sel_from_rawi(const int hw_scenario);

/* implementation */
static inline int is_on_the_fly(const int hw_scenario)
{
	return HWPATH_VARIANT(hw_scenario) == MTKCAM_IPI_FLOW_OTF;
}

static inline int is_direct_couple(const int hw_scenario)
{
	return HWPATH_VARIANT(hw_scenario) == MTKCAM_IPI_FLOW_DC;
}

static inline int is_offline(const int hw_scenario)
{
	return HWPATH_VARIANT(hw_scenario) == MTKCAM_IPI_FLOW_OFFLINE;
}

// stagger sensor (multi-exposure)
static inline int is_stagger(const int hw_scenario)
{
	return HWPATH_FLOW(hw_scenario) == MTKCAM_IPI_FLOW_STAGGER;
}

static inline int is_mstream(const int hw_scenario)
{
	return HWPATH_FLOW(hw_scenario) == MTKCAM_IPI_FLOW_MSTREAM;
}

static inline int is_dcif_required(const int hw_scenario)
{
	if (is_direct_couple(hw_scenario))
		return 1;

	return hw_scenario == MTKCAM_IPI_HW_PATH_STAGGER ||
		hw_scenario == MTKCAM_IPI_HW_PATH_OTF_RGBW_DOL;
}

static inline int is_tg_required(const int hw_scenario)
{
	return is_on_the_fly(hw_scenario);
}

// rawi is triggered by CAM SOF
static inline int is_rawi_triggered_by_raw_sof(const int hw_scenario)
{
	return hw_scenario == MTKCAM_IPI_HW_PATH_MSTREAM ||
		hw_scenario == MTKCAM_IPI_HW_PATH_STAGGER ||
		hw_scenario == MTKCAM_IPI_HW_PATH_OTF_RGBW_DOL;
}

static inline int is_raw_sel_from_rawi(const int hw_scenario)
{
	if (HWPATH_FLOW(hw_scenario) == MTKCAM_IPI_FLOW_ADL)
		return 0;

	return is_direct_couple(hw_scenario) || is_offline(hw_scenario);
}


#endif
