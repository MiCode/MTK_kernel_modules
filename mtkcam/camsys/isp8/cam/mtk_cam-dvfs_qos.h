/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_CAM_DVFS_QOS_H
#define __MTK_CAM_DVFS_QOS_H

#include <linux/clk.h>
#include <linux/interconnect.h>

#include "mtk_cam-dvfs_qos_raw.h"
#include "mtk_cam-dvfs_qos_sv.h"
#include "mtk_cam-dvfs_qos_mraw.h"

struct device;
struct regulator;

struct dvfs_stream_info;
struct mtk_camsys_dvfs {
	struct device *dev;

	unsigned int opp_num;
	struct camsys_opp_table {
		unsigned int freq_hz;
		unsigned int volt_uv;
	} opp[8];

	struct clk *mmdvfs_clk;

	int max_stream_num;
	struct dvfs_stream_info *stream_infos;

	struct mutex dvfs_lock;
	int cur_opp_idx;
};

int mtk_cam_dvfs_probe(struct device *dev,
		       struct mtk_camsys_dvfs *dvfs, int max_stream_num);
int mtk_cam_dvfs_remove(struct mtk_camsys_dvfs *dvfs);

void mtk_cam_dvfs_reset_runtime_info(struct mtk_camsys_dvfs *dvfs);

unsigned int mtk_cam_dvfs_query(struct mtk_camsys_dvfs *dvfs, int opp_idx);

int mtk_cam_dvfs_update(struct mtk_camsys_dvfs *dvfs, int stream_id,
			unsigned int target_freq_hz, bool boostable);

int mtk_cam_dvfs_switch_begin(struct mtk_camsys_dvfs *dvfs, int stream_id,
			      unsigned int target_freq_hz, bool boostable);
int mtk_cam_dvfs_switch_end(struct mtk_camsys_dvfs *dvfs, int stream_id);

static inline
int mtk_cam_dvfs_get_opp_table(struct mtk_camsys_dvfs *dvfs,
			       const struct camsys_opp_table **tbl)
{
	*tbl = dvfs->opp;
	return dvfs->opp_num;
}

struct mtk_camsys_qos_path;
struct mtk_camsys_qos {
	int n_path;
	struct mtk_camsys_qos_path *cam_path;
};

int mtk_cam_qos_probe(struct device *dev,
		      struct mtk_camsys_qos *qos, int qos_num);
int mtk_cam_qos_remove(struct mtk_camsys_qos *qos);

static inline u32 to_qos_icc(unsigned long Bps)
{
	return kBps_to_icc(Bps / 1024);
}

static inline u32 KBps_to_bwr(unsigned long KBps)
{
	return KBps / 1024;
}

static inline int is_w_merge_port(int id, enum PORT_DOMAIN domain)
{
	if (domain == RAW_DOMAIN) {
		switch(id) {
		case SMI_PORT_CQI_R1:
		case SMI_PORT_CQI_R2:
		case SMI_PORT_RAWI_R2:
		case SMI_PORT_RAWI_R3:
		case SMI_PORT_RAWI_R4:
		case SMI_PORT_RAWI_R5:
		case SMI_PORT_BPCI_R1:
		case SMI_PORT_BPCI_R3:
		case SMI_PORT_GMGI_R1:
		case SMI_PORT_LSCI_R1:
		case SMI_PORT_IPUI_I1:
			return 0;
		default:
			return 1;
		}
	} else
		return 1;
}

static inline int is_adl_port(int id)
{
	if (id >= SMI_PORT_ADL_START)
		return 1;
	else
		return 0;
}

struct mtk_cam_job;
struct mtk_cam_engines;
struct req_buffer_helper;
void mtk_cam_fill_qos(struct req_buffer_helper *helper);

int mtk_cam_apply_qos(struct mtk_cam_job *job);

int mtk_cam_reset_qos(struct device *dev, struct mtk_camsys_qos *qos);

#endif /* __MTK_CAM_DVFS_QOS_H */
