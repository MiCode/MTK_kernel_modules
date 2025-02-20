/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __MTK_CAM_DVFS_QOS_RAW_H
#define __MTK_CAM_DVFS_QOS_RAW_H

#include "mtk_cam-defs.h"

enum SMI_RAW_MERGE_PORT_ID {
	/* raw domain */
	SMI_PORT_CQI_R1 = 0,
	SMI_PORT_RAWI_R2,
	SMI_PORT_RAWI_R3,
	SMI_PORT_RAWI_R5,
	SMI_PORT_IMGO_R1,
	SMI_PORT_FPRI_R1,
	SMI_PORT_BPCI_R1,
	SMI_PORT_BPCI_R4,
	SMI_PORT_LSCI_R1,
	SMI_PORT_UFEO_R1,
	SMI_PORT_LTMSO_R1,
	SMI_PORT_DRZB2NO_R1,
	SMI_PORT_AFO_R1,
	SMI_PORT_AAO_R1,
	SMI_PORT_RAW_NUM,
};

enum SMI_YUV_MERGE_PORT_ID {
	/* yuv domain */
	SMI_PORT_YUVO_R1 = 0,
	SMI_PORT_YUVO_R3,
	SMI_PORT_YUVO_R2,
	SMI_PORT_YUVO_R5,
	SMI_PORT_RGBWI_R1,
	SMI_PORT_TCYSO_R1,
	SMI_PORT_DRZHNO_R3,
	SMI_PORT_YUV_NUM,
};

enum PORT_DOMAIN {
	RAW_DOMAIN,
	RAW_W_DOMAIN,
	YUV_DOMAIN,
};

enum STATS_DMA_PORT {
	PORT_UNKNOWN = 0,
	PORT_CQI,
	PORT_CACI,
	PORT_BPCI,
	PORT_PDI,
	PORT_PDO,
	PORT_AAO,
	PORT_AAHO,
	PORT_TSFSO,
	PORT_LTMSO,
	PORT_LTMSHO,
	PORT_FLKO,
	PORT_TCYSO,
	PORT_AFO,
};

/* for yuv */
enum UFBC_TYPE {
	UFBC_BITSTREAM_0 = 1,
	UFBC_BITSTREAM_1,
	UFBC_TABLE_0,
	UFBC_TABLE_1,
};

struct qos_dma_desc {
	const char *dma_name;
	u8 domain;
	u8 src_port;
	u8 dst_port;
	u8 ufbc_type;
	u8 exp_num;
};

struct mtkcam_qos_desc {
	u8 id;
	u8 desc_size;

	struct qos_dma_desc *dma_desc;
};

static struct qos_dma_desc stats_cfg_dmas[] = {
	{
		.dma_name = "cqi_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_CQI,
		.dst_port = SMI_PORT_CQI_R1,
	},
	{
		.dma_name = "caci_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_CACI,
		.dst_port = SMI_PORT_FPRI_R1,
	},
	{
		.dma_name = "bpci_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_BPCI,
		.dst_port = SMI_PORT_BPCI_R1,
	},
	{
		.dma_name = "bpci_r2",
		.domain = RAW_DOMAIN,
		.src_port = PORT_BPCI,
		.exp_num = 2,
		.dst_port = SMI_PORT_BPCI_R1,
	},
	{
		.dma_name = "bpci_r3",
		.domain = RAW_DOMAIN,
		.src_port = PORT_BPCI,
		.exp_num = 3,
		.dst_port = SMI_PORT_BPCI_R4,
	},
	{
		.dma_name = "pdi_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_PDI,
		.dst_port = SMI_PORT_LSCI_R1,
	},
};

static struct qos_dma_desc stats_0_dmas[] = {
	{
		.dma_name = "aao_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_AAO,
		.dst_port = SMI_PORT_AAO_R1,
	},
	{
		.dma_name = "aaho_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_AAHO,
		.dst_port = SMI_PORT_AAO_R1,
	},
	{
		.dma_name = "tsfso_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_TSFSO,
		.dst_port = SMI_PORT_AFO_R1,
	},
	{
		.dma_name = "ltmso_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_LTMSO,
		.dst_port = SMI_PORT_LTMSO_R1,
	},
	{
		.dma_name = "ltmsho_r1",
		.src_port = PORT_LTMSHO,
		.dst_port = SMI_PORT_LTMSO_R1,
	},
	{
		.dma_name = "flko_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_FLKO,
		.dst_port = SMI_PORT_UFEO_R1,
	},
	{
		.dma_name = "tcyso_r1",
		.domain = YUV_DOMAIN,
		.src_port = PORT_TCYSO,
		.dst_port = SMI_PORT_TCYSO_R1,
	},
	{
		.dma_name = "pdo_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_PDO,
		.dst_port = SMI_PORT_UFEO_R1,
	},
};

static struct qos_dma_desc stats_1_dmas[] = {
	{
		.dma_name = "afo_r1",
		.domain = RAW_DOMAIN,
		.src_port = PORT_AFO,
		.dst_port = SMI_PORT_AFO_R1,
	},
};

#define MTKCAM_IPI_STATS_NUM 3
static  struct mtkcam_qos_desc mmqos_stats_table[MTKCAM_IPI_STATS_NUM] = {
	{
		.id = MTKCAM_IPI_RAW_META_STATS_CFG,
		.dma_desc = stats_cfg_dmas,
		.desc_size = ARRAY_SIZE(stats_cfg_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_META_STATS_0,
		.dma_desc = stats_0_dmas,
		.desc_size = ARRAY_SIZE(stats_0_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_META_STATS_1,
		.dma_desc = stats_1_dmas,
		.desc_size = ARRAY_SIZE(stats_1_dmas),
	},
};

static struct qos_dma_desc rawi_2_dmas[] = {
	{
		.dma_name = "rawi_r2",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R2,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r2",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R2,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc rawi_3_dmas[] = {
	{
		.dma_name = "rawi_r3",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R3,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r3",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R3,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc rawi_5_dmas[] = {
	{
		.dma_name = "rawi_r5",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R5,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r5",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R5,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc imgo_dmas[] = {
	{
		.dma_name = "imgo_r1",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_IMGO_R1,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufeo_r1",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_UFEO_R1,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc yuvo_1_dmas[] = {
	{
		.dma_name = "yuvo_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R1,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "yuvbo_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R1,
		.ufbc_type = UFBC_BITSTREAM_1,
	},
	{
		.dma_name = "yuvco_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R1,
		.ufbc_type = UFBC_TABLE_0,
	},
	{
		.dma_name = "yuvdo_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R1,
		.ufbc_type = UFBC_TABLE_1,
	},
};

static struct qos_dma_desc yuvo_2_dmas[] = {
	{
		.dma_name = "yuvo_r2",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R2,
	},
	{
		.dma_name = "yuvbo_r2",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R2,
	},
};

static struct qos_dma_desc yuvo_3_dmas[] = {
	{
		.dma_name = "yuvo_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R3,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "yuvbo_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R3,
		.ufbc_type = UFBC_BITSTREAM_1,
	},
	{
		.dma_name = "yuvco_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R3,
		.ufbc_type = UFBC_TABLE_0,
	},
	{
		.dma_name = "yuvdo_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R3,
		.ufbc_type = UFBC_TABLE_1,
	},
};

static struct qos_dma_desc yuvo_4_dmas[] = {
	{
		.dma_name = "yuvo_r4",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R2,
	},
	{
		.dma_name = "yuvbo_r4",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R2,
	},
};

static struct qos_dma_desc yuvo_5_dmas[] = {
	{
		.dma_name = "yuvo_r5",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R5,
	},
	{
		.dma_name = "yuvbo_r5",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R5,
	},
};

static struct qos_dma_desc rzh1n2to_2_dmas[] = {
	{
		.dma_name = "rzh1n2to_r2",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_TCYSO_R1,
	},
};

static struct qos_dma_desc drzs4no_1_dmas[] = {
	{
		.dma_name = " drzs4no_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_TCYSO_R1,
	},
	{
		.dma_name = " drzh2no_r8",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_TCYSO_R1,
	},
};

static struct qos_dma_desc drzs4no_3_dmas[] = {
	{
		.dma_name = " drzs4no_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_DRZHNO_R3,
	},
};

static struct qos_dma_desc rzh1n2to_3_dmas[] = {
	{
		.dma_name = "rzh1n2to_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_DRZHNO_R3,
	},
	{
		.dma_name = "rzh1n2tbo_r3",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R5,
	},
};

static struct qos_dma_desc rzh1n2to_1_dmas[] = {
	{
		.dma_name = "rzh1n2to_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_DRZHNO_R3,
	},
	{
		.dma_name = "rzh1n2tbo_r1",
		.domain = YUV_DOMAIN,
		.dst_port = SMI_PORT_YUVO_R5,
	},
};

static struct qos_dma_desc drzb2no_1_dmas[] = {
	{
		.dma_name = "drzb2no_r1",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_DRZB2NO_R1,
	},
	{
		.dma_name = "drzb2nbo_r1",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_DRZB2NO_R1,
	},
	{
		.dma_name = " drzb2nco_r1",
		.domain = RAW_DOMAIN,
		.dst_port = SMI_PORT_DRZB2NO_R1,
	},
};

static struct qos_dma_desc imgo_w_dmas[] = {
	{
		.dma_name = "imgo_r1_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_IMGO_R1,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufeo_r1_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_UFEO_R1,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc rawi_r2_w_dmas[] = {
	{
		.dma_name = "rawi_r2_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R2,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r2_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R2,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc rawi_r3_w_dmas[] = {
	{
		.dma_name = "rawi_r3_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R3,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r2_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R2,
		.ufbc_type = UFBC_TABLE_0,
	},
};

static struct qos_dma_desc rawi_r5_w_dmas[] = {
	{
		.dma_name = "rawi_r5_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R5,
		.ufbc_type = UFBC_BITSTREAM_0,
	},
	{
		.dma_name = "ufdi_r5_w",
		.domain = RAW_W_DOMAIN,
		.dst_port = SMI_PORT_RAWI_R5,
		.ufbc_type = UFBC_TABLE_0,
	},
};

#define MTKCAM_IPI_RAW_NUM 21
static struct mtkcam_qos_desc mmqos_img_table[MTKCAM_IPI_RAW_NUM] = {
	{
		.id = MTKCAM_IPI_RAW_RAWI_2,
		.dma_desc = rawi_2_dmas,
		.desc_size = ARRAY_SIZE(rawi_2_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RAWI_3,
		.dma_desc = rawi_3_dmas,
		.desc_size = ARRAY_SIZE(rawi_3_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RAWI_5,
		.dma_desc = rawi_5_dmas,
		.desc_size = ARRAY_SIZE(rawi_5_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_IPUI,
		.desc_size = 0,
	},
	{
		.id = MTKCAM_IPI_RAW_IMGO,
		.dma_desc = imgo_dmas,
		.desc_size = ARRAY_SIZE(imgo_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_YUVO_1,
		.dma_desc = yuvo_1_dmas,
		.desc_size = ARRAY_SIZE(yuvo_1_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_YUVO_2,
		.dma_desc = yuvo_2_dmas,
		.desc_size = ARRAY_SIZE(yuvo_2_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_YUVO_3,
		.dma_desc = yuvo_3_dmas,
		.desc_size = ARRAY_SIZE(yuvo_2_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_YUVO_4,
		.dma_desc = yuvo_4_dmas,
		.desc_size = ARRAY_SIZE(yuvo_4_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_YUVO_5,
		.dma_desc = yuvo_5_dmas,
		.desc_size = ARRAY_SIZE(yuvo_5_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RZH1N2TO_2,
		.dma_desc = rzh1n2to_2_dmas,
		.desc_size = ARRAY_SIZE(rzh1n2to_2_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_DRZS4NO_1,
		.dma_desc = drzs4no_1_dmas,
		.desc_size = ARRAY_SIZE(drzs4no_1_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_DRZS4NO_3,
		.dma_desc = drzs4no_3_dmas,
		.desc_size = ARRAY_SIZE(drzs4no_3_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RZH1N2TO_3,
		.dma_desc = rzh1n2to_3_dmas,
		.desc_size = ARRAY_SIZE(rzh1n2to_3_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RZH1N2TO_1,
		.dma_desc = rzh1n2to_1_dmas,
		.desc_size = ARRAY_SIZE(rzh1n2to_1_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_DRZB2NO_1,
		.dma_desc = drzb2no_1_dmas,
		.desc_size = ARRAY_SIZE(drzb2no_1_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_IPUO,
		.desc_size = 0,
	},
	{
		.id = MTKCAM_IPI_RAW_IMGO_W,
		.dma_desc = imgo_w_dmas,
		.desc_size = ARRAY_SIZE(imgo_w_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RAWI_2_W,
		.dma_desc = rawi_r2_w_dmas,
		.desc_size = ARRAY_SIZE(rawi_r2_w_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RAWI_3_W,
		.dma_desc = rawi_r3_w_dmas,
		.desc_size = ARRAY_SIZE(rawi_r3_w_dmas),
	},
	{
		.id = MTKCAM_IPI_RAW_RAWI_5_W,
		.dma_desc = rawi_r5_w_dmas,
		.desc_size = ARRAY_SIZE(rawi_r5_w_dmas),
	},
};

#endif /* __MTK_CAM_DVFS_QOS_RAW_H */
