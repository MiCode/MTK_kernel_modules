// SPDX-License-Identifier: GPL-2.0
//
// Copyright (c) 2019 MediaTek Inc.

#include <linux/module.h>

#include "mtk_cam-plat.h"
#include "mtk_cam-raw_regs.h"
#include "mtk_cam-meta-mt6985.h"
#include "mtk_cam-ipi.h"

#define RAW_STATS_CFG_SIZE \
	ALIGN(sizeof(struct mtk_cam_uapi_meta_raw_stats_cfg), SZ_1K)

#define RAW_STATS_CFG_SIZE_RGBW \
	ALIGN(sizeof(struct mtk_cam_uapi_meta_raw_stats_rgbw_cfg), SZ_1K)

#define RAW_STAT_0_BUF_SIZE_RGBW_BAYER \
			(MTK_CAM_UAPI_AAO_MAX_BUF_SIZE + \
			MTK_CAM_UAPI_AAHO_MAX_BUF_SIZE + \
			MTK_CAM_UAPI_LTMSO_SIZE + \
			MTK_CAM_UAPI_LTMSHO_SIZE + \
			MTK_CAM_UAPI_TSFSO_SIZE * 2 + \
			MTK_CAM_UAPI_TCYSO_SIZE)

#define RAW_STAT_0_BUF_SIZE_RGBW_W \
			(MTK_CAM_UAPI_AAO_MAX_BUF_SIZE + \
			MTK_CAM_UAPI_AAHO_MAX_BUF_SIZE + \
			MTK_CAM_UAPI_LTMSO_SIZE + \
			MTK_CAM_UAPI_LTMSHO_SIZE + \
			MTK_CAM_UAPI_FLK_MAX_BUF_SIZE + \
			MTK_CAM_UAPI_TSFSO_SIZE * 2 + \
			MTK_CAM_UAPI_TCYSO_SIZE)

#define RAW_STAT_0_STRUCT_SIZE \
	ALIGN(sizeof(struct mtk_cam_uapi_meta_raw_stats_0), SZ_1K)

/* meta out max size include 1k meta info and dma buffer size */
#define RAW_STATS_0_SIZE \
	ALIGN(RAW_STAT_0_STRUCT_SIZE + \
			RAW_STAT_0_BUF_SIZE_RGBW_BAYER + \
			MTK_CAM_UAPI_FLK_MAX_BUF_SIZE \
			, (4 * SZ_1K))

#define RAW_STATS_0_SIZE_RGBW \
	ALIGN(RAW_STAT_0_STRUCT_SIZE + \
			RAW_STAT_0_BUF_SIZE_RGBW_BAYER + \
			RAW_STAT_0_BUF_SIZE_RGBW_W \
			, (4 * SZ_1K))

#define RAW_STATS_1_SIZE_UNALIGNED \
	(ALIGN(sizeof(struct mtk_cam_uapi_meta_raw_stats_1), SZ_1K) + \
			MTK_CAM_UAPI_AFO_MAX_BUF_SIZE)

#define RAW_STATS_1_SIZE \
	ALIGN(RAW_STATS_1_SIZE_UNALIGNED, (4 * SZ_1K))

#define RAW_STATS_1_SIZE_RGBW \
	ALIGN(RAW_STATS_1_SIZE_UNALIGNED * 2, (4 * SZ_1K))

#define SV_STATS_0_SIZE \
	sizeof(struct mtk_cam_uapi_meta_camsv_stats_0)

#define MRAW_STATS_0_SIZE \
	sizeof(struct mtk_cam_uapi_meta_mraw_stats_0)

#define DYNAMIC_SIZE

static void set_payload(struct mtk_cam_uapi_meta_hw_buf *buf,
			unsigned int size, size_t *offset)
{
	buf->offset = *offset;
	buf->size = size;
	*offset += size;
}

static void meta_state0_reset_all(struct mtk_cam_uapi_meta_raw_stats_0 *stats)
{
	size_t offset = sizeof(*stats);

	set_payload(&stats->ae_awb_stats.aao_buf, 0, &offset);
	set_payload(&stats->ae_awb_stats.aaho_buf, 0, &offset);
	set_payload(&stats->ltm_stats.ltmso_buf, 0, &offset);
	set_payload(&stats->ltm_stats.ltmsho_buf, 0, &offset);
	set_payload(&stats->flk_stats.flko_buf, 0, &offset);
	set_payload(&stats->tsf_stats.tsfo_r1_buf, 0, &offset);
	set_payload(&stats->tsf_stats.tsfo_r2_buf, 0, &offset);
	set_payload(&stats->tcys_stats.tcyso_buf, 0, &offset);
	set_payload(&stats->pde_stats.pdo_buf, 0, &offset);

	set_payload(&stats->ae_awb_stats_w.aao_buf, 0, &offset);
	set_payload(&stats->ae_awb_stats_w.aaho_buf, 0, &offset);
	set_payload(&stats->ltm_stats_w.ltmso_buf, 0, &offset);
	set_payload(&stats->ltm_stats_w.ltmsho_buf, 0, &offset);
	set_payload(&stats->flk_stats_w.flko_buf, 0, &offset);
	set_payload(&stats->tsf_stats_w.tsfo_r1_buf, 0, &offset);
	set_payload(&stats->tsf_stats_w.tsfo_r2_buf, 0, &offset);
	set_payload(&stats->tcys_stats_w.tcyso_buf, 0, &offset);
	set_payload(&stats->pde_stats_w.pdo_buf, 0, &offset);
}

static int set_meta_stat0_info(struct mtk_cam_uapi_meta_raw_stats_0 *stats,
			       size_t size,
			       const struct set_meta_stats_info_param *p)
{
	size_t offset = sizeof(*stats);
	unsigned int flko_size;

	if (!p->meta_cfg || !p->meta_cfg_size) {
		meta_state0_reset_all(stats);
		return 0;
	}

#ifdef DYNAMIC_SIZE
	flko_size = (p->height / p->bin_ratio) *
		MTK_CAM_UAPI_FLK_BLK_SIZE * MTK_CAM_UAPI_FLK_MAX_STAT_BLK_NUM;
#else
	flko_size = MTK_CAM_UAPI_FLK_MAX_BUF_SIZE;
#endif

	set_payload(&stats->ae_awb_stats.aao_buf,
		    MTK_CAM_UAPI_AAO_MAX_BUF_SIZE, &offset);
	set_payload(&stats->ae_awb_stats.aaho_buf,
		    (p->rgbw) ? MTK_CAM_UAPI_AAHO_HIST_SIZE :
				MTK_CAM_UAPI_AAHO_MAX_BUF_SIZE,
			&offset);
	set_payload(&stats->ltm_stats.ltmso_buf,
		    MTK_CAM_UAPI_LTMSO_SIZE, &offset);
	set_payload(&stats->ltm_stats.ltmsho_buf,
		    MTK_CAM_UAPI_LTMSHO_SIZE, &offset);
	set_payload(&stats->flk_stats.flko_buf,
		    (p->rgbw) ? 0 : flko_size, &offset);
	set_payload(&stats->tsf_stats.tsfo_r1_buf,
		    MTK_CAM_UAPI_TSFSO_SIZE, &offset);
	set_payload(&stats->tsf_stats.tsfo_r2_buf,
		    MTK_CAM_UAPI_TSFSO_SIZE, &offset);
	set_payload(&stats->tcys_stats.tcyso_buf,
		    MTK_CAM_UAPI_TCYSO_SIZE, &offset);
	set_payload(&stats->pde_stats.pdo_buf, 0, &offset); /* FIXME */

	/* TODO: rgbw */
	set_payload(&stats->ae_awb_stats_w.aao_buf,
				(p->rgbw) ? MTK_CAM_UAPI_AAO_MAX_BUF_SIZE : 0,
				&offset);
	set_payload(&stats->ae_awb_stats_w.aaho_buf,
				(p->rgbw) ? MTK_CAM_UAPI_AAHO_HIST_SIZE : 0,
				&offset);
	set_payload(&stats->ltm_stats_w.ltmso_buf,
				(p->rgbw) ? MTK_CAM_UAPI_LTMSO_SIZE : 0,
				&offset);
	set_payload(&stats->ltm_stats_w.ltmsho_buf,
				(p->rgbw) ? MTK_CAM_UAPI_LTMSHO_SIZE : 0,
				&offset);
	set_payload(&stats->flk_stats_w.flko_buf,
				(p->rgbw) ? flko_size : 0,
				&offset);
	set_payload(&stats->tsf_stats_w.tsfo_r1_buf,
				(p->rgbw) ? MTK_CAM_UAPI_TSFSO_SIZE : 0,
				&offset);
	set_payload(&stats->tsf_stats_w.tsfo_r2_buf,
				(p->rgbw) ? MTK_CAM_UAPI_TSFSO_SIZE : 0,
				&offset);
	set_payload(&stats->tcys_stats_w.tcyso_buf,
				(p->rgbw) ? MTK_CAM_UAPI_TCYSO_SIZE : 0,
				&offset);
	set_payload(&stats->pde_stats_w.pdo_buf, 0, &offset);

	if (offset > size) {
		pr_info("%s: required %zu > buffer size %zu\n",
			__func__, offset, size);
		return -1;
	}

	return 0;
}

static int set_meta_stat1_info(struct mtk_cam_uapi_meta_raw_stats_1 *stats,
			       size_t size,
			       const struct set_meta_stats_info_param *p)
{
	size_t offset = sizeof(*stats);

	set_payload(&stats->af_stats.afo_buf,
		MTK_CAM_UAPI_AFO_MAX_BUF_SIZE, &offset);

	/* w part */
	set_payload(&stats->af_stats_w.afo_buf,
			(p->rgbw) ? MTK_CAM_UAPI_AFO_MAX_BUF_SIZE : 0,
			&offset);

	if (offset > size) {
		pr_info("%s: required %zu > buffer size %zu\n",
			__func__, offset, size);
		return -1;
	}

	return 0;
}

static int set_meta_stats_info(int ipi_id, void *addr, size_t size,
			       const struct set_meta_stats_info_param *p)
{
	if (WARN_ON(!addr))
		return -1;

	switch (ipi_id) {
	case MTKCAM_IPI_RAW_META_STATS_0:
		if (WARN_ON(size < sizeof(struct mtk_cam_uapi_meta_raw_stats_0)))
			return -1;

		return set_meta_stat0_info(addr, size, p);
	case MTKCAM_IPI_RAW_META_STATS_1:
		if (WARN_ON(size < sizeof(struct mtk_cam_uapi_meta_raw_stats_1)))
			return -1;

		return set_meta_stat1_info(addr, size, p);
	default:
		pr_info("%s: %s: not supported: %d\n",
			__FILE__, __func__, ipi_id);
		break;
	}
	return -1;
}

static int set_sv_meta_stats_info(
	int ipi_id, void *addr, struct dma_info *info)
{
	struct mtk_cam_uapi_meta_camsv_stats_0 *sv_stats0;
	unsigned long offset;
	unsigned int size;

	switch (ipi_id) {
	case MTKCAM_IPI_CAMSV_MAIN_OUT:
		sv_stats0 = (struct mtk_cam_uapi_meta_camsv_stats_0 *)addr;
		size = info->stride * info->height;
		/* calculate offset for 16-alignment limitation */
		offset = ((((dma_addr_t)sv_stats0 + SV_STATS_0_SIZE + 15) >> 4) << 4)
			- (dma_addr_t)sv_stats0;
		set_payload(&sv_stats0->pd_stats.pdo_buf, size, &offset);
		sv_stats0->pd_stats_enabled = 1;
		sv_stats0->pd_stats.stats_src.width = info->width;
		sv_stats0->pd_stats.stats_src.height = info->height;
		sv_stats0->pd_stats.stride = info->stride;
		break;
	default:
		pr_info("%s: %s: not supported: %d\n",
			__FILE__, __func__, ipi_id);
		break;
	}

	return 0;
}

static int set_mraw_meta_stats_info(
	int ipi_id, void *addr, struct dma_info *info)
{
	struct mtk_cam_uapi_meta_mraw_stats_0 *mraw_stats0;
	unsigned long offset;
	unsigned int size;

	switch (ipi_id) {
	case MTKCAM_IPI_MRAW_META_STATS_0:
		mraw_stats0 = (struct mtk_cam_uapi_meta_mraw_stats_0 *)addr;
		/* imgo */
		size = info[imgo_m1].stride * info[imgo_m1].height;
		/* calculate offset for 16-alignment limitation */
		offset = ((((dma_addr_t)mraw_stats0 + MRAW_STATS_0_SIZE + 15) >> 4) << 4)
			- (dma_addr_t)mraw_stats0;
		set_payload(&mraw_stats0->pdp_0_stats.pdo_buf, size, &offset);
		mraw_stats0->pdp_0_stats_enabled = 1;
		mraw_stats0->pdp_0_stats.stats_src.width = info[imgo_m1].width;
		mraw_stats0->pdp_0_stats.stats_src.height = info[imgo_m1].height;
		mraw_stats0->pdp_0_stats.stride = info[imgo_m1].stride;
		/* imgbo */
		size = info[imgbo_m1].stride * info[imgbo_m1].height;
		/* calculate offset for 16-alignment limitation */
		offset = ((((dma_addr_t)mraw_stats0 + offset + 15) >> 4) << 4)
			- (dma_addr_t)mraw_stats0;
		set_payload(&mraw_stats0->pdp_1_stats.pdo_buf, size, &offset);
		mraw_stats0->pdp_1_stats_enabled = 1;
		mraw_stats0->pdp_1_stats.stats_src.width = info[imgbo_m1].width;
		mraw_stats0->pdp_1_stats.stats_src.height = info[imgbo_m1].height;
		mraw_stats0->pdp_1_stats.stride = info[imgbo_m1].stride;
		/* cpio */
		size = info[cpio_m1].stride * info[cpio_m1].height;
		/* calculate offset for 16-alignment limitation */
		offset = ((((dma_addr_t)mraw_stats0 + offset + 15) >> 4) << 4)
			- (dma_addr_t)mraw_stats0;
		set_payload(&mraw_stats0->cpi_stats.cpio_buf, size, &offset);
		mraw_stats0->cpi_stats_enabled = 1;
		mraw_stats0->cpi_stats.stats_src.width = info[cpio_m1].width;
		mraw_stats0->cpi_stats.stats_src.height = info[cpio_m1].height;
		mraw_stats0->cpi_stats.stride = info[cpio_m1].stride;
		break;
	default:
		pr_info("%s: %s: not supported: %d\n",
			__FILE__, __func__, ipi_id);
		break;
	}

	return 0;
}

static int get_mraw_stats_cfg_param(
	void *addr, struct mraw_stats_cfg_param *param)
{
	struct mtk_cam_uapi_meta_mraw_stats_cfg *stats_cfg =
		(struct mtk_cam_uapi_meta_mraw_stats_cfg *)addr;

	param->mqe_en = stats_cfg->mqe_enable;
	param->mobc_en = stats_cfg->mobc_enable;
	param->plsc_en = stats_cfg->plsc_enable;

	param->crop_width = stats_cfg->crop_param.crop_x_end -
		stats_cfg->crop_param.crop_x_start;
	param->crop_height = stats_cfg->crop_param.crop_y_end -
		stats_cfg->crop_param.crop_y_start;

	param->mqe_mode = stats_cfg->mqe_param.mqe_mode;

	param->mbn_hei = stats_cfg->mbn_param.mbn_hei;
	param->mbn_pow = stats_cfg->mbn_param.mbn_pow;
	param->mbn_dir = stats_cfg->mbn_param.mbn_dir;
	param->mbn_spar_hei = stats_cfg->mbn_param.mbn_spar_hei;
	param->mbn_spar_pow = stats_cfg->mbn_param.mbn_spar_pow;
	param->mbn_spar_fac = stats_cfg->mbn_param.mbn_spar_fac;
	param->mbn_spar_con1 = stats_cfg->mbn_param.mbn_spar_con1;
	param->mbn_spar_con0 = stats_cfg->mbn_param.mbn_spar_con0;

	param->cpi_th = stats_cfg->cpi_param.cpi_th;
	param->cpi_pow = stats_cfg->cpi_param.cpi_pow;
	param->cpi_dir = stats_cfg->cpi_param.cpi_dir;
	param->cpi_spar_hei = stats_cfg->cpi_param.cpi_spar_hei;
	param->cpi_spar_pow = stats_cfg->cpi_param.cpi_spar_pow;
	param->cpi_spar_fac = stats_cfg->cpi_param.cpi_spar_fac;
	param->cpi_spar_con1 = stats_cfg->cpi_param.cpi_spar_con1;
	param->cpi_spar_con0 = stats_cfg->cpi_param.cpi_spar_con0;

	return 0;
}

#define RAW_M4U_PORT_NUM 12
#define YUV_M4U_PORT_NUM 7
#define DMA_GROUP_SIZE 4
static u32 raw_dma_group[RAW_M4U_PORT_NUM][DMA_GROUP_SIZE] = {
	/* port-0 */
	{0x0, 0x0, 0x0, 0x0},
	/* port-1 */
	{REG_RAWI_R2_BASE, REG_UFDI_R2_BASE, 0x0, 0x0},
	/* port-2 */
	{REG_RAWI_R3_BASE, REG_UFDI_R3_BASE, 0x0, 0x0},
	/* port-3 */
	{REG_RAWI_R5_BASE, REG_UFDI_R5_BASE, 0x0, 0x0},
	/* port-4 */
	{REG_IMGO_R1_BASE, REG_FHO_R1_BASE, 0x0, 0x0},
	/* port-5 */
	{REG_BPCI_R1_BASE, REG_BPCI_R2_BASE, REG_BPCI_R3_BASE, REG_CACI_R1_BASE},
	/* port-6 */
	{REG_LSCI_R1_BASE, REG_PDI_R1_BASE, REG_AAI_R1_BASE, 0x0},
	/* port-7 */
	{REG_UFEO_R1_BASE, REG_FLKO_R1_BASE, REG_PDO_R1_BASE, 0x0},
	/* port-8*/
	{REG_LTMSO_R1_BASE, REG_LTMSHO_R1_BASE, 0x0, 0x0},
	/* port-9  */
	{REG_DRZB2NO_R1_BASE, REG_DRZB2NBO_R1_BASE, REG_DRZB2NCO_R1_BASE, 0x0},
	/* port-10 */
	{REG_AFO_R1_BASE, REG_TSFSO_R1_BASE, REG_TSFSO_R2_BASE, 0x0},
	/* port-11 */
	{REG_AAO_R1_BASE, REG_AAHO_R1_BASE, 0x0, 0x0},
};

static u32 yuv_dma_group[YUV_M4U_PORT_NUM][DMA_GROUP_SIZE] = {
	/* port-0 */
	{REG_YUVO_R1_BASE, REG_YUVBO_R1_BASE, REG_YUVCO_R1_BASE, REG_YUVDO_R1_BASE},
	/* port-1 */
	{REG_YUVO_R3_BASE, REG_YUVBO_R3_BASE, REG_YUVCO_R3_BASE, REG_YUVDO_R3_BASE},
	/* port-2 */
	{REG_YUVO_R2_BASE, REG_YUVBO_R2_BASE, REG_YUVO_R4_BASE, REG_YUVBO_R4_BASE},
	/* port-3 */
	{REG_YUVO_R5_BASE, REG_YUVBO_R5_BASE, REG_RZH1N2TBO_R1_BASE, REG_RZH1N2TBO_R3_BASE},
	/* port-4 */
	{REG_RGBWI_R1_BASE, 0x0, 0x0, 0x0},
	/* port-5 */
	{REG_TCYSO_R1_BASE, REG_RZH1N2TO_R2_BASE, REG_DRZS4NO_R1_BASE, REG_DRZH2NO_R8_BASE},
	/* port-6 */
	{REG_DRZS4NO_R3_BASE, REG_RZH1N2TO_R3_BASE, REG_RZH1N2TO_R1_BASE, 0x0},
};

static int query_raw_dma_group(int m4u_id, u32 *group)
{
	if (m4u_id < RAW_M4U_PORT_NUM)
		memcpy(group, raw_dma_group[m4u_id], sizeof(u32)*DMA_GROUP_SIZE);
	else
		pr_info("%s: %s: not supported: %d\n",
			__FILE__, __func__, m4u_id);

	return 0;
}

static int query_yuv_dma_group(int m4u_id, u32 *group)
{
	if (m4u_id < YUV_M4U_PORT_NUM)
		memcpy(group, yuv_dma_group[m4u_id], sizeof(u32)*DMA_GROUP_SIZE);
	else
		pr_info("%s: %s: not supported: %d\n",
			__FILE__, __func__, m4u_id);

	return 0;
}

static const struct plat_v4l2_data mt6985_v4l2_data = {
	.raw_pipeline_num = 3,
	.camsv_pipeline_num = 16,
	.mraw_pipeline_num = 4,

	.meta_major = MTK_CAM_META_VERSION_MAJOR,
	.meta_minor = MTK_CAM_META_VERSION_MINOR,

	.meta_cfg_size = RAW_STATS_CFG_SIZE,
	.meta_cfg_size_rgbw = RAW_STATS_CFG_SIZE_RGBW,
	.meta_stats0_size = RAW_STATS_0_SIZE,
	.meta_stats0_size_rgbw = RAW_STATS_0_SIZE_RGBW,
	.meta_stats1_size = RAW_STATS_1_SIZE,
	.meta_stats1_size_rgbw = RAW_STATS_1_SIZE_RGBW,
	.meta_sv_ext_size = SV_STATS_0_SIZE,
	.meta_mraw_ext_size = MRAW_STATS_0_SIZE,

	.timestamp_buffer_ofst = offsetof(struct mtk_cam_uapi_meta_raw_stats_0,
					  timestamp),

	.reserved_camsv_dev_id = 3,

	.set_meta_stats_info = set_meta_stats_info,
	.set_sv_meta_stats_info = set_sv_meta_stats_info,
	.set_mraw_meta_stats_info = set_mraw_meta_stats_info,
	.get_mraw_stats_cfg_param = get_mraw_stats_cfg_param,
};

static const struct plat_data_hw mt6985_hw_data = {
	.camsys_axi_mux = 0x3,
	.cammux_id_raw_start = 34,
	.camsys_dma_group_size = DMA_GROUP_SIZE,

	.query_raw_dma_group = query_raw_dma_group,
	.query_yuv_dma_group = query_yuv_dma_group,
};

struct camsys_platform_data mt6985_data = {
	.platform = "mt6985",
	.v4l2 = &mt6985_v4l2_data,
	.hw = &mt6985_hw_data,
};
