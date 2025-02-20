/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef __MTK_CAM_RAW_CTRL_H
#define __MTK_CAM_RAW_CTRL_H

#include "mtk_camera-v4l2-controls-7sp.h"

static inline
bool res_raw_is_dc_mode(const struct mtk_cam_resource_raw_v2 *res_raw)
{
	return res_raw->hw_mode == MTK_CAM_HW_MODE_DIRECT_COUPLED;
}

static inline bool scen_is_normal(const struct mtk_cam_scen *scen)
{
	return scen->id == MTK_CAM_SCEN_NORMAL ||
		scen->id == MTK_CAM_SCEN_ODT_NORMAL ||
		scen->id == MTK_CAM_SCEN_M2M_NORMAL;
}

static inline bool scen_is_mstream(const struct mtk_cam_scen *scen)
{
	return scen->id == MTK_CAM_SCEN_MSTREAM ||
		scen->id == MTK_CAM_SCEN_ODT_MSTREAM;
}

static inline bool scen_is_smvr(const struct mtk_cam_scen *scen)
{
	return scen->id == MTK_CAM_SCEN_SMVR;
}

static inline bool scen_is_dcg_sensor_merge(const struct mtk_cam_scen *scen)
{
	if (scen_is_normal(scen))
		return (scen->scen.normal.stagger_type ==
				MTK_CAM_STAGGER_DCG_SENSOR_MERGE);

	return false;
}

static inline bool scen_is_dcg_ap_merge(const struct mtk_cam_scen *scen)
{
	if (scen_is_normal(scen))
		return (scen->scen.normal.stagger_type ==
				MTK_CAM_STAGGER_DCG_AP_MERGE);

	return false;
}

static inline bool scen_is_vhdr(const struct mtk_cam_scen *scen)
{
	switch (scen->id) {
	case MTK_CAM_SCEN_NORMAL:
	case MTK_CAM_SCEN_M2M_NORMAL:
	case MTK_CAM_SCEN_ODT_NORMAL:
		return (scen->scen.normal.max_exp_num > 1 ||
			scen_is_dcg_sensor_merge(scen));
	case MTK_CAM_SCEN_MSTREAM:
	case MTK_CAM_SCEN_ODT_MSTREAM:
		return 1;
	default:
		break;
	}
	return 0;
}

static inline bool scen_support_rgbw(const struct mtk_cam_scen *scen)
{
	if (scen_is_normal(scen))
		return !!(scen->scen.normal.w_chn_supported);

	return false;
}

static inline bool scen_is_rgbw(const struct mtk_cam_scen *scen)
{
	if (scen_is_normal(scen))
		return !!(scen->scen.normal.w_chn_enabled);

	return false;
}
static inline bool scen_is_extisp(const struct mtk_cam_scen *scen)
{
	return (scen->id == MTK_CAM_SCEN_EXT_ISP);
}

static inline bool scen_is_m2m(const struct mtk_cam_scen *scen)
{
	return (scen->id == MTK_CAM_SCEN_ODT_NORMAL ||
		scen->id == MTK_CAM_SCEN_ODT_MSTREAM ||
		scen->id == MTK_CAM_SCEN_M2M_NORMAL);
}

static inline bool scen_is_m2m_apu(const struct mtk_cam_scen *scen,
				   const struct mtk_cam_apu_info *apu_info)
{
	return scen->id == MTK_CAM_SCEN_M2M_NORMAL &&
		(apu_info->apu_path != APU_NONE);
}

static inline bool apu_info_is_dc(const struct mtk_cam_apu_info *apu_info)
{
	return apu_info->apu_path == APU_DC_RAW;
}

static inline bool scen_is_stagger_lbmf(const struct mtk_cam_scen *scen)
{
	if (scen_is_normal(scen) &&
		scen->scen.normal.max_exp_num > 1 &&
		scen->scen.normal.stagger_type == MTK_CAM_STAGGER_LBMF)
		return true;

	return false;
}

#define SCEN_MAX_LEN 40
static inline int scen_to_str(char *buff, size_t size,
			      const struct mtk_cam_scen *scen)
{
	int n = 0;

	if (scen_is_normal(scen)) {
		n = scnprintf(buff, size, "scen:id=%d n:exp=%d/%d stag=%d",
			     scen->id,
			     scen->scen.normal.exp_num,
			     scen->scen.normal.max_exp_num,
			     scen->scen.normal.stagger_type);

		if (scen->scen.normal.w_chn_enabled ||
		    scen->scen.normal.w_chn_supported)
			n += scnprintf(buff + n, size - n, " w=%d/%d",
				       scen->scen.normal.w_chn_enabled,
				       scen->scen.normal.w_chn_supported);

	} else if (scen_is_mstream(scen))
		n = scnprintf(buff, size, "scen:id=%d m:type=%d",
			     scen->id,
			     scen->scen.mstream.type);
	else if (scen_is_smvr(scen))
		n = scnprintf(buff, size, "scen:id=%d smvr:sub=%d%s",
			      scen->id,
			      scen->scen.smvr.subsample_num,
			      scen->scen.smvr.output_first_frame_only ? " 1st-only" : "");

	return n;
}

#define RES_RAW_MAX_LEN (SCEN_MAX_LEN + 100)
static inline int raw_res_to_str(char *buff, size_t size,
				 const struct mtk_cam_resource_raw_v2 *r)
{
	int n;

	n = scen_to_str(buff, size, &r->scen);

	n += scnprintf(buff + n, size - n,
		       " pxlmode=%d freq=%d bin=%d hwmode=%d raw=(0x%x,0x%x,%d)",
		       r->raw_pixel_mode, r->freq / 1000000,
		       r->bin, r->hw_mode,
		       r->raws, r->raws_must, r->raws_max_num);

	if (r->img_wbuf_num || r->img_wbuf_size)
		n += scnprintf(buff + n, size - n, " wbuf=%dx%d",
			       r->img_wbuf_num, r->img_wbuf_size);

	if (r->slb_size)
		n += scnprintf(buff + n, size - n, " slb_s=%u", r->slb_size);

	return n;
}

#endif /*__MTK_CAM_RAW_CTRL_H*/
