// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Ming-Hsuan.Chiang <Ming-Hsuan.Chiang@mediatek.com>
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>

#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include "cmdq-sec.h"
#include "cmdq-sec-iwc-common.h"
#include <linux/videodev2.h>
#include <soc/mediatek/smi.h>

#include "mtk-mae-isp8.h"

#ifdef MAE_TF_DUMP_8
#include <dt-bindings/memory/mt6991-larb-port.h>
#endif
#include "iommu_debug.h"

#define CHECK_BASE_ADDR(ADDR) (ADDR % MAE_BASE_ADDR_ALIGN != 0)

#define MAE_DUMP_REG(MAE_DEV, REG)								\
		mae_dev_info(mae_dev->dev, "%s [0x%08X %08X]\n",			\
					#REG, (uint32_t)REG,				\
					(uint32_t)readl(MAE_DEV->mae_base + REG))

#define MAE_CMDQ_WRITE_REG(PKT, MAE_REG_OFFSET, VALUE)			\
	do {								\
		cmdq_pkt_write(PKT, NULL, g_data->data->base_address + MAE_REG_OFFSET,	\
						VALUE, CMDQ_REG_MASK);	\
	} while(0)

#define DIV_CEIL_POS(X,Y)	((((X)) - (int)((X) / (Y)) * (Y)) > 0 ? (int)(((X) / (Y)) + 1) : (int)((X) / (Y)))
// #define CEIL_POS(X)	((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))
// #define CEIL_NEG(X)	(int)(X)
// #define CEIL(X)		( ((X) > 0) ? CEIL_POS(X) : CEIL_NEG(X) )

// #define FLOOR_POS(X)		(int)(X)
// #define FLOOR_NEG(X)		((X-(int)(X)) < 0 ? (int)(X-1) : (int)(X))
// #define FLOOR(X)		( ((X) > 0) ? FLOOR_POS(X) : FLOOR_NEG(X) )

#define MIN(X,Y)		(((X) > (Y)) ? (Y) : (X))
#define MAX(X,Y)		(((X) > (Y)) ? (X) : (Y))
#define ABS(X)			(((X) > 0) ? (X) : -(X))
#define ROUND(X,Y)		((int)((2*(X))+(Y))/(2*(Y)))
#define BUFFER_NAME_LEN		50
#define CMDQ_GPR_R03_IDX 11
#define AIE_POLL_TIME_INFINI	(0xFFFF)

#define MAE_READ_BACK_REG_EN 1

static struct clk_bulk_data mae_clks_isp8[] = {
	{ .id = "VCORE_GALS_DISP" },
	{ .id = "VCORE_MAIN" },
	{ .id = "VCORE_SUB0_CAMERA_P2" },
	{ .id = "VCORE_SUB1_CAMERA_P2" },
	{ .id = "FDVT_CAMERA_P2" },
	{ .id = "LARB12_CAMERA_P2" },
	{ .id = "IPE_CAMERA_P2" },
	{ .id = "SUB_COMMON2_CAMERA_P2" },
	{ .id = "SUB_COMMON3_CAMERA_P2" },
	{ .id = "GALS_TRX_IPE0_CAMERA_P2" },
	{ .id = "GALS_TRX_IPE1_CAMERA_P2" },
	{ .id = "GALS_CAMERA_P2" },
};

static struct clk_bulk_data mae_clks_isp8_m6899[] = {
	{ .id = "VCORE_GALS_DISP_CAMERA_MAE" },
	{ .id = "VCORE_MAIN_CAMERA_MAE" },
	{ .id = "VCORE_SUB0_CAMERA_MAE" },
	{ .id = "VCORE_SUB1_CAMERA_MAE" },
	{ .id = "FDVT_CAMERA_MAE" },
	{ .id = "LARB12_CAMERA_MAE" },
	{ .id = "IPE_CAMERA_MAE" },
	{ .id = "SUB_COMMON2_CAMERA_MAE" },
	{ .id = "SUB_COMMON3_CAMERA_MAE" },
	{ .id = "GALS_TRX_IPE0_CAMERA_MAE" },
	{ .id = "GALS_TRX_IPE1_CAMERA_MAE" },
	{ .id = "GALS_CAMERA_MAE" },
};

static struct mae_data mae_data_isp8 = {
	.internal_buffer_size = 3 * 512 * 1024,
	.base_address = 0x34310000,
	.fd_fpn_threshold = 0xAA,
};

static struct mae_data mae_data_isp8_mt6899 = {
	.internal_buffer_size = 3 * 512 * 1024,
	.base_address = 0x15310000,
	.fd_fpn_threshold = 0x9B,
};

struct mae_priv_data priv_data_isp8 = {
	.fd_v0_config_info = {
		{
			.size = 1004,
			.rotate_offset = 2043,
			.rotate_size = 1004,
		},
		{
			.size = 968,
			.rotate_offset = 1431,
			.rotate_size = 968,
		},
		{
			.size = 671,
			.rotate_offset = 1090,
			.rotate_size = 671,
		},
		{
			.size = 543,
			.rotate_offset = 917,
			.rotate_size = 543,
		}
	},
	.fd_v0_coef_info = {
		{
			.size = 1309,
		},
		{
			.size = 5583,
		},
		{
			.size = 5583,
		},
		{
			.size = 9540,
		}
	},
	.attr_v0_config_info = {
		.size = 558,
		.rotate_offset = 1000,
		.rotate_size = 558,
	},
	.attr_v0_coef_info = {
		.size = 5180,
	},
	.v0_fd_coef_offset = {
		V0_FD_640_480_COEF_PAT_OFFSET,
		V0_FD_480_360_COEF_PAT_OFFSET,
		V0_FD_240_180_COEF_PAT_OFFSET,
		V0_FD_120_90_COEF_PAT_OFFSET
	},
	.v0_fd_config_offset = {
		V0_FD_640_480_CONFIG_PAT_OFFSET,
		V0_FD_480_360_CONFIG_PAT_OFFSET,
		V0_FD_240_180_CONFIG_PAT_OFFSET,
		V0_FD_120_90_CONFIG_PAT_OFFSET
	},
	.fd_v1_ipn_config_info = {
		{
			.size = 1004,
			.rotate_offset = 3968,
			.rotate_size = 1005,
		},
		{
			.size = 968,
			.rotate_offset = 3338,
			.rotate_size = 968,
		},
		{
			.size = 671,
			.rotate_offset = 2971,
			.rotate_size = 671,
		},
		{
			.size = 543,
			.rotate_offset = 2413,
			.rotate_size = 544,
		}
	},
	.fd_v1_ipn_coef_info = {
		{
			.size = 1309,
		},
		{
			.size = 5583,
		},
		{
			.size = 5583,
		},
		{
			.size = 9540,
		}
	},
	.fac_v1_config_info = {
		.size = 588,
		.rotate_offset = 2206,
		.rotate_size = 588,
	},
	.fac_v1_coef_info = {
		.size = 7262,
	},
	.fd_v1_fpn_config_info = {
		.size = 777,
		.rotate_offset = 4091,
		.rotate_size = 777,
	},
	.fd_v1_fpn_coef_info = {
		.size = 6449,
	},
	.v1_fd_ipn_coef_offset = {
		V1_FD_IPN_640_480_COEF_PAT_OFFSET,
		V1_FD_IPN_480_360_COEF_PAT_OFFSET,
		V1_FD_IPN_240_180_COEF_PAT_OFFSET,
		V1_FD_IPN_120_90_COEF_PAT_OFFSET
	},
	.v1_fd_ipn_config_offset = {
		V1_FD_IPN_640_480_CONFIG_PAT_OFFSET,
		V1_FD_IPN_480_360_CONFIG_PAT_OFFSET,
		V1_FD_IPN_240_180_CONFIG_PAT_OFFSET,
		V1_FD_IPN_120_90_CONFIG_PAT_OFFSET
	},
	.v1_fd_ipn_config_single_size = {
		V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE,
		V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE,
		V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE,
		V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE
	},
	.cmdq_polling_option = 2,
};

struct mae_priv_data priv_data_isp8_mt6899 = {
	.fd_v1_ipn_config_info = {
		{
			.size = 451,
			.rotate_offset = 3977,
			.rotate_size = 451,
		},
		{
			.size = 435,
			.rotate_offset = 3566,
			.rotate_size = 435,
		},
		{
			.size = 470,
			.rotate_offset = 3154,
			.rotate_size = 470,
		},
		{
			.size = 436,
			.rotate_offset = 3118,
			.rotate_size = 436,
		}
	},
	.fd_v1_ipn_coef_info = {
		{
			.size = 289,
		},
		{
			.size = 289,
		},
		{
			.size = 2179,
		},
		{
			.size = 1309,
		}
	},
	.fac_v1_config_info = {
		.size = 428,
		.rotate_offset = 3184,
		.rotate_size = 433,
	},
	.fac_v1_coef_info = {
		.size = 1599,
	},
	.fd_v1_fpn_config_info = {
		.size = 448,
		.rotate_offset = 6278,
		.rotate_size = 453,
	},
	.fd_v1_fpn_coef_info = {
		.size = 2306,
	},
	.v1_fd_ipn_coef_offset = {
		V1_FD_IPN_640_480_COEF_PAT_OFFSET_mt6899,
		V1_FD_IPN_480_360_COEF_PAT_OFFSET_mt6899,
		V1_FD_IPN_240_180_COEF_PAT_OFFSET_mt6899,
		V1_FD_IPN_120_90_COEF_PAT_OFFSET_mt6899
	},
	.v1_fd_ipn_config_offset = {
		V1_FD_IPN_640_480_CONFIG_PAT_OFFSET_mt6899,
		V1_FD_IPN_480_360_CONFIG_PAT_OFFSET_mt6899,
		V1_FD_IPN_240_180_CONFIG_PAT_OFFSET_mt6899,
		V1_FD_IPN_120_90_CONFIG_PAT_OFFSET_mt6899
	},
	.v1_fd_ipn_config_single_size = {
		V1_FD_IPN_640_480_CONFIG_SINGLE_SIZE_mt6899,
		V1_FD_IPN_480_360_CONFIG_SINGLE_SIZE_mt6899,
		V1_FD_IPN_240_180_CONFIG_SINGLE_SIZE_mt6899,
		V1_FD_IPN_120_90_CONFIG_SINGLE_SIZE_mt6899
	},
	.cmdq_polling_option = 1,
};

const struct mae_plat_data *g_data;

/*
 * MAE Debug level:
 * MAE_INFO = 0
 * MAE_DEBUG = 1
 */
int mae_log_level_value;
int mae_trigger_cmdq_timeout;
int fld_debug_1;
int mae_dbf_on;
int set_default_value = 1;
int fld_reset_en = 1;
int cmdq_polling_en = 1;
int cmdq_profiling_en;
uint32_t mae_preultra_read;
uint32_t mae_preultra_write;
uint32_t mae_rdma_debug_sel;
uint32_t mae_read_back_val;

module_param(mae_log_level_value, int, 0644);
module_param(mae_trigger_cmdq_timeout, int, 0644);
module_param(fld_debug_1, int, 0644);
module_param(mae_dbf_on, int, 0644);
module_param(set_default_value, int, 0644);
module_param(fld_reset_en, int, 0644);
module_param(cmdq_polling_en, int, 0644);
module_param(cmdq_profiling_en, int, 0644);
module_param(mae_preultra_read, uint, 0644);
module_param(mae_preultra_write, uint, 0644);
module_param(mae_rdma_debug_sel, uint, 0644);
module_param(mae_read_back_val, uint, 0644);

static bool mtk_mae_dump(struct mtk_mae_dev *mae_dev);
static void mtk_mae_fld_reset(struct mtk_mae_dev *mae_dev);
static void mtk_mae_dump_param(struct mtk_mae_dev *mae_dev);
static void mtk_mae_dump_buf_iova(struct mtk_mae_dev *mae_dev);
static void mtk_mae_reg_dump_to_buffer(struct mtk_mae_dev *mae_dev);

#ifdef MAE_TF_DUMP_8
static int MAE_M4U_TranslationFault_callback_8(int port,
							dma_addr_t mva,
							void *data)
{
	struct mtk_mae_dev *mae_dev;

	if (data == NULL) {
		pr_info("MAE TF CB Data is NULL\n");
		return 0;
	}

	mae_dev = (struct mtk_mae_dev *)data;

	mae_dev_info(mae_dev->dev ,"[MAE_M4U]fault call port=%d, mva=0x%llx", port, mva);

	mtk_mae_dump_param(mae_dev);

	mtk_mae_dump_buf_iova(mae_dev);

	return 1;
}

void mtk_mae_register_tf_cb_8(void *data)
{
	mtk_iommu_register_fault_callback(SMMU_L12_P0_FDVT_RDA_0,
		(mtk_iommu_fault_callback_t)MAE_M4U_TranslationFault_callback_8,
		data, false);
	mtk_iommu_register_fault_callback(SMMU_L12_P1_FDVT_WRA_0,
		(mtk_iommu_fault_callback_t)MAE_M4U_TranslationFault_callback_8,
		data, false);
}
#endif

static void MAECmdqCB(struct cmdq_cb_data data)
{
	struct mtk_mae_dev *mae_dev = (struct mtk_mae_dev *)data.data;
	bool is_hw_hang = (data.err == 0) ? false : true;

	mae_dev->is_hw_hang = is_hw_hang;

	if (is_hw_hang) {
		mae_dev_info(mae_dev->dev, "MAE HW Hang");
		mtk_mae_dump(mae_dev);
	}

	queue_work(mae_dev->frame_done_wq, &mae_dev->req_work.work);
}

static int mtk_mae_fd_core_sel(struct mtk_mae_dev *mae_dev,
								struct EnqueParam *param,
								uint32_t loop)
{
	int crop_w, crop_h;
	int pyramid_w, pyramid_h;

	if (param->image[loop].enRoi) {
		crop_w = param->image[loop].roi.x2 - param->image[loop].roi.x1;
		crop_h = param->image[loop].roi.y2 - param->image[loop].roi.y1;
	} else {
		crop_w = param->image[loop].imgWidth;
		crop_h = param->image[loop].imgHeight;
	}

	pyramid_w = param->image[loop].resizeWidth;
	pyramid_h = (crop_h * pyramid_w) / crop_w;

	if (pyramid_w > fd_pattern_width[loop] || pyramid_h > fd_pattern_height[loop]) {
		mae_dev_info(mae_dev->dev, "[%s] over pyramid size limit, too large w(%d/%d), h(%d/%d)",
					__func__, pyramid_w, fd_pattern_width[loop],
					pyramid_h, fd_pattern_height[loop]);
		// force to 640x480
		// return -1;
		return 0;
	} else if (pyramid_w > fd_pattern_width[1] || pyramid_h > fd_pattern_height[1]) {
		return 0;
	} else if (pyramid_w > fd_pattern_width[2] || pyramid_h > fd_pattern_height[2]) {
		return 1;
	} else if (pyramid_w > fd_pattern_width[3] || pyramid_h > fd_pattern_height[3]) {
		return 2;
	} else if (pyramid_w >= FD_PYRAMID_MIN_WIDTH || pyramid_h >= FD_PYRAMID_MIN_HEIGHT) {
		return 3;
	} else {
		mae_dev_info(mae_dev->dev, "[%s] over pyramid size limit, too small w(%d/%d), h(%d/%d)",
					__func__, pyramid_w, FD_PYRAMID_MIN_WIDTH, pyramid_h, FD_PYRAMID_MIN_HEIGHT);
		return -1;
	}
}

static void mtk_mae_set_default_value(struct mtk_mae_dev *mae_dev, struct cmdq_pkt *pkt)
{
	uint32_t i;

	for (i = 0; i < RSZ_NUM; i++) {
		MAE_CMDQ_WRITE_REG(pkt, REG_0004_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0008_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0008);
		MAE_CMDQ_WRITE_REG(pkt, REG_000C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0010_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0008);
		MAE_CMDQ_WRITE_REG(pkt, REG_001C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0020_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0101);
		MAE_CMDQ_WRITE_REG(pkt, REG_0024_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x8000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0028_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0100);

		MAE_CMDQ_WRITE_REG(pkt, REG_002C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0101);
		MAE_CMDQ_WRITE_REG(pkt, REG_0034_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0003);
		MAE_CMDQ_WRITE_REG(pkt, REG_005C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0060_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0064_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0068_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0080_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0780);
		MAE_CMDQ_WRITE_REG(pkt, REG_0084_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0f00);
		MAE_CMDQ_WRITE_REG(pkt, REG_00A0_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x8780);
		MAE_CMDQ_WRITE_REG(pkt, REG_00A4_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x8438);
		MAE_CMDQ_WRITE_REG(pkt, REG_00A8_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0F00);
		MAE_CMDQ_WRITE_REG(pkt, REG_00AC_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0870);
		MAE_CMDQ_WRITE_REG(pkt, REG_00C0_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0090);

		MAE_CMDQ_WRITE_REG(pkt, REG_00C4_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_00C8_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_00CC_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x02C0);
		MAE_CMDQ_WRITE_REG(pkt, REG_00D0_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x02C0);
		MAE_CMDQ_WRITE_REG(pkt, REG_00D4_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_00D8_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x01E0);

		MAE_CMDQ_WRITE_REG(pkt, REG_0104_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0022);
		MAE_CMDQ_WRITE_REG(pkt, REG_0108_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_010C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_0110_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x02C0);
		MAE_CMDQ_WRITE_REG(pkt, REG_0114_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x02C6);
		MAE_CMDQ_WRITE_REG(pkt, REG_0118_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, REG_011C_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x01E0);
		MAE_CMDQ_WRITE_REG(pkt, REG_0120_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x01E0);
		MAE_CMDQ_WRITE_REG(pkt, REG_0180_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0005);
		MAE_CMDQ_WRITE_REG(pkt, REG_0184_RSZ1 + RSZ_BASE_ADDR_OFFSET * i, 0x0002);

		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_X_OFFSET_0 + COMMON_REG_SIZE * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_Y_OFFSET_0 + COMMON_REG_SIZE * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_MMFD_O_SCALE_0 + COMMON_REG_SIZE * i, 0x0AA8);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_SIZE0 + COMMON_REG_SIZE * i, 0x0280);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_SIZE0 + COMMON_REG_SIZE * i, 0x01E0);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MIN0 + COMMON_REG_SIZE * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MIN0 + COMMON_REG_SIZE * i, 0x0000);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_SCORE_TH0 + COMMON_REG_SIZE * i, 0x012C);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MAX0 + COMMON_REG_SIZE * i, 0x01F4);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MAX0 + COMMON_REG_SIZE * i, 0x01F4);
	}

	MAE_CMDQ_WRITE_REG(pkt, MAE_0174_MAISR, 0xc000);
}

static bool select_outer_loop_by_mae_mode(struct mtk_mae_dev *mae_dev,
											const struct EnqueParam *param,
											uint32_t *loop_num)
{
	switch (param->maeMode) {
	case FD_V0:
	case FD_V1_IPN:
		*loop_num = param->pyramidNumber;
		break;
	case ATTR_V0:
	case FAC_V1:
		*loop_num = param->attrFaceNumber;
		break;
	case FLD_V0:
	case FD_V1_FPN:
	case AISEG:
		*loop_num = 1;
		break;
	default:
		mae_dev_dbg(mae_dev->dev, "unsupport mode(%d)", param->maeMode);
		return false;
	}

	return true;
}

static bool mtk_mae_config_dma(struct mtk_mae_dev *mae_dev, uint32_t idx)
{
	struct EnqueParam *param =
		(struct EnqueParam*)mae_dev->map_table->param_dmabuf_info[idx].kva;
	uint64_t addr = 0;
	uint64_t config_addr = 0;
	uint64_t coef_addr = 0;
	uint32_t config_offset = 0;
	uint32_t config_rt_offset = 0;
	uint32_t config_size = 0;
	uint32_t coef_offset = 0;
	uint32_t coef_size = 0;
	uint32_t loop = 0;
	uint32_t i = 0;
	uint32_t wdma_base_addr_reg_offset = 0;
	struct ModelTable *model_table = (struct ModelTable *)mae_dev->map_table->model_table_dmabuf_info.kva;
	struct mae_priv_data *priv_data = g_data->priv_data;

	mae_dev_dbg(mae_dev->dev, "%s+", __func__);
	if (set_default_value)
		mtk_mae_set_default_value(mae_dev, mae_dev->pkt[idx]);

	if (!select_outer_loop_by_mae_mode(mae_dev, param, &mae_dev->outer_loop[idx]))
		return false;

	if (mae_dev->outer_loop[idx] > MAX_OUTER_LOOP_NUM) {
		mae_dev_info(mae_dev->dev, "[%s] outer_loop num(%d) is over limitation",
					__func__, mae_dev->outer_loop[idx]);
		return false;
	}

	mae_dev_dbg(mae_dev->dev, "[%s] outer_loop num is %d",
					__func__, mae_dev->outer_loop[idx]);

	// config the base address of internal buffer
	addr = mae_dev->map_table->internal_dmabuf_info.pa;
	if (CHECK_BASE_ADDR(addr) || addr == 0) {
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned",
				"internal buffer", addr ,MAE_BASE_ADDR_ALIGN);
		return false;
	}

	mae_dev_dbg(mae_dev->dev, "internal base (0x%llx)", addr);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_INTRN_BASE_0_W, LSB_ADDR(addr));
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_INTRN_BASE_1_W, MSB_ADDR(addr));
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_INTRN_BASE_0_R, LSB_ADDR(addr));
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_INTRN_BASE_1_R, MSB_ADDR(addr));

	for (loop = 0; loop < mae_dev->outer_loop[idx]; loop++) {
		if (!param->image[loop].enRoi) {
			param->image[loop].roi.x1 = 0;
			param->image[loop].roi.y1 = 0;
			param->image[loop].roi.x2 = 0;
			param->image[loop].roi.y2 = 0;
		}

		if (!param->image[loop].enPadding) {
			param->image[loop].padding.left = 0;
			param->image[loop].padding.right = 0;
			param->image[loop].padding.up = 0;
			param->image[loop].padding.down = 0;
		}

		// config the base address of input buffer
		addr = mae_dev->map_table->image_dmabuf_info[idx].pa;

		if (param->maeMode == AISEG) {
			if (param->image[loop].imgWidth % 8 != 0) {
				mae_dev_info(mae_dev->dev, "Loop %d: image width(%d) should be 8-aligned",
						loop, param->image[loop].imgWidth);
				return false;
			}
		} else {
			if (param->image[loop].imgWidth % 16 != 0) {
				mae_dev_info(mae_dev->dev, "Loop %d: image width(%d) should be 16-aligned",
						loop, param->image[loop].imgWidth);
				return false;
			}
		}

		if (param->image[loop].enRoi) {
				addr += (MAX(param->image[loop].roi.y1, 0) / 2 * 2) *
						param->image[loop].imgWidth +
						(MAX(param->image[loop].roi.x1, 0) / 16) * 16;
		}

		if (CHECK_BASE_ADDR(addr) || addr == 0)
			mae_dev_info(mae_dev->dev, "Loop %d: %s(0x%llx) is not %d-aligned or zero",
					loop, "image0", addr, MAE_BASE_ADDR_ALIGN);

		mae_dev_dbg(mae_dev->dev, "Loop %d: %s(0x%llx)", loop, "image0", addr);

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_EXTRN_BASE0_00_0_R + loop * BASE_ADDR_REG_SIZE,
				LSB_ADDR(addr));
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_EXTRN_BASE0_00_1_R + loop * BASE_ADDR_REG_SIZE,
				MSB_ADDR(addr));

		addr = mae_dev->map_table->image_dmabuf_info[idx].pa +
				(uint64_t)param->image[loop].imgWidth * param->image[loop].imgHeight;

		if (param->image[loop].enRoi) {
				addr += (MAX(param->image[loop].roi.y1, 0) / 2 * 2) *
						(param->image[loop].imgWidth / 2) +
						(MAX(param->image[loop].roi.x1, 0) / 16) * 16;
		}

		if (CHECK_BASE_ADDR(addr) || addr == 0)
			mae_dev_info(mae_dev->dev, "Loop %d: %s(0x%llx) is not %d-aligned or zero",
										loop, "image1", addr, MAE_BASE_ADDR_ALIGN);

		mae_dev_dbg(mae_dev->dev, "Loop %d: %s(0x%llx)", loop, "image1", addr);
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_EXTRN_BASE1_00_0_R + loop * BASE_ADDR_REG_SIZE,
				LSB_ADDR(addr));
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_EXTRN_BASE1_00_1_R + loop * BASE_ADDR_REG_SIZE,
				MSB_ADDR(addr));

		// config the base address of output buffer
		if (param->maeMode == AISEG) {
			for (i = 0; i < param->outputNum; i++) {
				addr = mae_dev->map_table->aiseg_output_dmabuf_info[idx][i].pa +
					model_table->aisegOutput[i].offset;
				if (CHECK_BASE_ADDR(addr) || addr == 0)
					mae_dev_info(mae_dev->dev, "Loop %d: %s %d (0x%llx) is not %d-aligned",
						loop, "aiseg output", i, addr, MAE_BASE_ADDR_ALIGN);

				mae_dev_dbg(mae_dev->dev, "Loop %d: %s %d (0x%llx)", loop, "aiseg output", i, addr);
				mae_dev_dbg(mae_dev->dev, "aiseg offset (0x%lx)", model_table->aisegOutput[i].offset);

				wdma_base_addr_reg_offset = BASE_ADDR_REG_SIZE * i;

				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
					MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
					LSB_ADDR(addr));

				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
					MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
					MSB_ADDR(addr));
			}
		} else {
			addr = mae_dev->map_table->output_dmabuf_info[idx][loop].pa;
			if (CHECK_BASE_ADDR(addr) || addr == 0)
				mae_dev_info(mae_dev->dev, "Loop %d: %s (0x%llx) is not %d-aligned",
					loop, "output", addr, MAE_BASE_ADDR_ALIGN);
			mae_dev_dbg(mae_dev->dev, "Loop %d: %s (0x%llx)", loop, "output", addr);

			switch (param->maeMode) {
			case FD_V0:
				wdma_base_addr_reg_offset = BASE_ADDR_REG_SIZE * FD_V0_WDMA_NUM * loop;

				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
					MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
					LSB_ADDR(addr));

				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
					MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
					MSB_ADDR(addr));
				break;
			case FD_V1_IPN:
				for (i = 0; i < FD_V1_IPN_WDMA_NUM; i++) {
					wdma_base_addr_reg_offset =
						BASE_ADDR_REG_SIZE * (FD_V1_IPN_WDMA_NUM * loop + i);

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
						LSB_ADDR(addr));

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
						MSB_ADDR(addr));

					addr += FD_V1_IPN_WDMA_SIZE;
				}
				break;
			case FD_V1_FPN:
				for (i = 0; i < FD_V1_FPN_WDMA_NUM; i++) {
					wdma_base_addr_reg_offset =
						BASE_ADDR_REG_SIZE * (FD_V1_FPN_WDMA_NUM * loop + i);

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
						LSB_ADDR(addr));

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
						MSB_ADDR(addr));

					addr += FD_V1_FPN_WDMA_SIZE;
				}
				break;
			case ATTR_V0:
				for (i = 0; i < ATTR_V0_WDMA_NUM; i++) {
					wdma_base_addr_reg_offset =
						BASE_ADDR_REG_SIZE * (ATTR_V0_WDMA_NUM * loop + i);

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
						LSB_ADDR(addr));

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
						MSB_ADDR(addr));

					addr += ATTR_V0_WDMA_SIZE * WDMA_DATA_UNIT;
				}

				if (loop == 0) {
					// follow model footprint document
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_00_W, 1);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_01_W, 1);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_02_W, 1);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_03_W, 1);
				}
				break;
			case FAC_V1:
				for (i = 0; i < FAC_V1_WDMA_NUM; i++) {
					wdma_base_addr_reg_offset =
						BASE_ADDR_REG_SIZE * (FAC_V1_WDMA_NUM * loop + i);

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_0_W + wdma_base_addr_reg_offset,
						LSB_ADDR(addr));

					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
						MAE_REG_EXTRN_BASE0_00_1_W + wdma_base_addr_reg_offset,
						MSB_ADDR(addr));

					addr += FAC_V1_WDMA_SIZE * WDMA_DATA_UNIT;
				}

				if (loop == 0) {
					// follow model footprint document
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_00_W, 54);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_01_W, 2);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_02_W, 2);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_03_W, 2);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_04_W, 2);
					MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_LN_OFFSET_05_W, 2);
				}
				break;
			default:
				mae_dev_info(mae_dev->dev, "[%s] unsupport mode(%d)",
					__func__, param->maeMode);
				return false;
			}
		}

		// calculate the offset and size of binary file
		if (param->maeMode == FD_V0 || param->maeMode == FD_V1_IPN) {
			mae_dev->core_sel[idx][loop] = mtk_mae_fd_core_sel(mae_dev, param, loop);
			if (mae_dev->core_sel[idx][loop] < 0)
				return false;
		}

		switch (param->maeMode) {
		case FD_V0:
			config_offset = priv_data->v0_fd_config_offset[mae_dev->core_sel[idx][loop]];
			coef_offset = priv_data->v0_fd_coef_offset[mae_dev->core_sel[idx][loop]];

			if (param->fdInputDegree == DEGREE_90 ||
				param->fdInputDegree == DEGREE_180) {
				config_rt_offset =
					priv_data->fd_v0_config_info[mae_dev->core_sel[idx][loop]].rotate_offset;
				config_size =
					priv_data->fd_v0_config_info[mae_dev->core_sel[idx][loop]].rotate_size;
			} else {
				config_rt_offset = 0;
				config_size =
					priv_data->fd_v0_config_info[mae_dev->core_sel[idx][loop]].size;
			}

			coef_size = priv_data->fd_v0_coef_info[mae_dev->core_sel[idx][loop]].size;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_FD_V0].pa + config_offset;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FD_V0].pa + coef_offset;
			break;
		case FD_V1_IPN:
			config_offset =
				priv_data->v1_fd_ipn_config_offset[mae_dev->core_sel[idx][loop]] +
				loop * priv_data->v1_fd_ipn_config_single_size[mae_dev->core_sel[idx][loop]];
			coef_offset = priv_data->v1_fd_ipn_coef_offset[mae_dev->core_sel[idx][loop]];

			if (param->fdInputDegree == DEGREE_90 ||
				param->fdInputDegree == DEGREE_180) {
				config_rt_offset =
					priv_data->fd_v1_ipn_config_info[mae_dev->core_sel[idx][loop]].rotate_offset;
				config_size =
					priv_data->fd_v1_ipn_config_info[mae_dev->core_sel[idx][loop]].rotate_size;
			} else {
				config_rt_offset = 0;
				config_size =
					priv_data->fd_v1_ipn_config_info[mae_dev->core_sel[idx][loop]].size;
			}

			coef_size = priv_data->fd_v1_ipn_coef_info[mae_dev->core_sel[idx][loop]].size;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_FD_V1_IPN].pa + config_offset;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FD_V1_IPN].pa + coef_offset;

			param->coef_dump_offset[loop] = coef_offset;
			param->coef_dump_size[loop] = coef_size * MAE_BASE_ADDR_ALIGN;
			param->config_dump_offset[loop] = config_offset + config_rt_offset * MAE_BASE_ADDR_ALIGN;
			param->config_dump_size[loop] = config_size * MAE_BASE_ADDR_ALIGN;
			break;
		case FD_V1_FPN:
			config_offset = 0;
			coef_offset = 0;

			if (param->fdInputDegree == DEGREE_90 ||
				param->fdInputDegree == DEGREE_180) {
				config_rt_offset = priv_data->fd_v1_fpn_config_info.rotate_offset;
				config_size = priv_data->fd_v1_fpn_config_info.rotate_size;
			} else {
				config_rt_offset = 0;
				config_size = priv_data->fd_v1_fpn_config_info.size;
			}
			coef_size = priv_data->fd_v1_fpn_coef_info.size;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_FD_V1_FPN].pa + config_offset;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FD_V1_FPN].pa + coef_offset;
			break;
		case ATTR_V0:
			config_offset = 0;
			coef_offset = 0;

			if (param->attrInputDegree[loop] == DEGREE_90 ||
				param->attrInputDegree[loop] == DEGREE_180) {
				// MAE_TO_CHECK: rotate_offset is 16B align but offset is not
				config_rt_offset = priv_data->attr_v0_config_info.rotate_offset;
				config_size = priv_data->attr_v0_config_info.rotate_size;
			} else {
				config_rt_offset = 0;
				config_size = priv_data->attr_v0_config_info.size;
			}

			coef_size = priv_data->attr_v0_config_info.size;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_FLD_FAC_V0].pa +
				config_offset;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FLD_FAC_V0].pa +
				coef_offset;
			break;
		case FAC_V1:
			config_offset = 0;
			coef_offset = 0;

			if (param->attrInputDegree[loop] == DEGREE_90 ||
				param->attrInputDegree[loop] == DEGREE_180) {
				// MAE_TO_CHECK: rotate_offset is 16B align but offset is not
				config_rt_offset = priv_data->fac_v1_config_info.rotate_offset;
				config_size = priv_data->fac_v1_config_info.rotate_size;
			} else {
				config_rt_offset = 0;
				config_size = priv_data->fac_v1_config_info.size;
			}

			coef_size = priv_data->fac_v1_coef_info.size;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_FLD_FAC_V1].pa +
				config_offset;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FLD_FAC_V1].pa +
				coef_offset;

			param->coef_dump_offset[loop] = coef_offset;
			param->coef_dump_size[loop] = coef_size * MAE_BASE_ADDR_ALIGN;
			param->config_dump_offset[loop] = config_offset + config_rt_offset * MAE_BASE_ADDR_ALIGN;
			param->config_dump_size[loop] = config_size * MAE_BASE_ADDR_ALIGN;
			break;
		case AISEG:
			config_rt_offset = 0;
			config_offset =
				model_table->configTable[MODEL_TYPE_AISEG].offset;
			coef_offset =
				model_table->coefTable[MODEL_TYPE_AISEG].offset;

			config_size =
				model_table->configTable[MODEL_TYPE_AISEG].size >> LSB_ADDR_SHIFT_BITS;
			coef_size =
				model_table->coefTable[MODEL_TYPE_AISEG].size >> LSB_ADDR_SHIFT_BITS;

			config_addr =
				mae_dev->map_table->config_dmabuf_info[MODEL_TYPE_AISEG].pa +
				config_offset ;
			coef_addr =
				mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_AISEG].pa +
				coef_offset;
			break;
		default:
			mae_dev_info(mae_dev->dev, "[%s] unsupport mode(%d)",
						__func__, param->maeMode);
			break;
		}

		mae_dev_dbg(mae_dev->dev,
			"Loop %d: sel: %d, deg: %d, config_offset: %d (B), ",
			loop, mae_dev->core_sel[idx][loop], param->fdInputDegree, config_offset);
		mae_dev_dbg(mae_dev->dev,
			"config_rt_offset: %d (16B), config_size: %d (16 B), ",
			config_rt_offset, config_size);
		mae_dev_dbg(mae_dev->dev,
			"coef_offset: %d (B), coef_size: %d (16 B) , config_addr: 0x%llx , coef_addr: 0x%llx",
			coef_offset, coef_size, config_addr, coef_addr);

		// config the base address of config buffer
		if (CHECK_BASE_ADDR(config_addr) || config_addr == 0)
			mae_dev_info(mae_dev->dev, "Loop %d: %s(0x%llx) is not %d-aligned",
					loop, "config0", config_addr, MAE_BASE_ADDR_ALIGN);

		mae_dev_dbg(mae_dev->dev, "Loop %d: %s(0x%llx)", loop, "config", config_addr);

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_CONFIG_BASE_00_0 + loop * BASE_ADDR_REG_SIZE,
				LSB_ADDR(config_addr + (config_rt_offset << LSB_ADDR_SHIFT_BITS)));
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_CONFIG_BASE_00_1 + loop * BASE_ADDR_REG_SIZE,
				MSB_ADDR(config_addr + (config_rt_offset << LSB_ADDR_SHIFT_BITS)));

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_CONFIG_SIZE_00 + loop * COMMON_REG_SIZE,
				config_size);

		// config the base address of coef buffer
		if (CHECK_BASE_ADDR(coef_addr) || coef_addr == 0)
			mae_dev_info(mae_dev->dev, "Loop %d: %s(0x%llx) is not %d-aligned",
					loop, "coef0", coef_addr, MAE_BASE_ADDR_ALIGN);

		mae_dev_dbg(mae_dev->dev, "Loop %d: %s(0x%llx)", loop, "coef0", coef_addr);

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_COEF_BASE_00_0 + loop * BASE_ADDR_REG_SIZE,
				LSB_ADDR(coef_addr));
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_COEF_BASE_00_1 + loop * BASE_ADDR_REG_SIZE,
				MSB_ADDR(coef_addr));

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_COEF_SIZE_00 + loop * COMMON_REG_SIZE,
				coef_size);
	}

	mae_dev_dbg(mae_dev->dev, "%s-", __func__);

	return true;
}

// follow DE crop formula
static bool mtk_mae_crop(struct mtk_mae_dev *mae_dev,
		const struct crop_setting_in *in,
		struct crop_setting_out *out)
{
	int32_t crop_x_size;
	int32_t crop_y_size;
	int32_t even_end_y;
	int32_t even_start_y;
	int32_t reg_outer_src_hsize;
	int32_t reg_outer_src_vsize;
	int32_t crop_right_x;
	int32_t crop_left_x;
	int32_t crop_down_y;
	int32_t crop_up_y;


	mae_dev_dbg(mae_dev->dev, "[%s] s_x(%d), e_x(%d), s_y(%d), e_y(%d), input_h_size(%d), input_v_size(%d)\n",
		__func__, in->start_x, in->end_x, in->start_y, in->end_y,
		in->input_h_size, in->input_v_size);

	crop_x_size = in->end_x - in->start_x;
	crop_y_size = in->end_y - in->start_y;

	if (crop_x_size <= 0 || crop_y_size <= 0 || in->input_h_size <= 0 || in->input_v_size <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] error input height/width(%d/%d), (x1,x2,y1,y2)=(%d,%d,%d,%d)",
				__func__, in->input_v_size, in->input_h_size,
				in->start_x, in->end_x, in->start_y, in->end_y);
		return false;
	}

	even_end_y = DIV_CEIL_POS(MIN(in->end_y, in->input_v_size), 2) * 2;
	even_start_y = MAX(in->start_y, 0) / 2 * 2;

	reg_outer_src_hsize = DIV_CEIL_POS(MIN(in->end_x, in->input_h_size), 16) * 16
		- MIN(MAX(in->start_x, 0), in->input_h_size) / 16 * 16;
	reg_outer_src_vsize = even_end_y - even_start_y;

	// base0_shift_offset = MAX(in->start_x, 0) / 16;
	// base0_shift = even_start_y * 40 + base0_shift_offset;
	// base1_shift = even_start_y * 20 + base0_shift_offset;

	crop_right_x = MIN(in->end_x, in->input_h_size)
		- DIV_CEIL_POS(MIN(in->end_x, in->input_h_size), 16) * 16;
	crop_left_x =  MAX(in->start_x, 0)
		- MIN(MAX(in->start_x, 0), in->input_h_size) / 16 * 16;

	crop_down_y = even_end_y - MIN(in->end_y, in->input_v_size);
	crop_up_y = even_start_y - MAX(in->start_y, 0);

	out->reg_pre_crop_h_st = ABS(crop_left_x);
	out->reg_pre_crop_h_length =
		reg_outer_src_hsize - ABS(crop_right_x) - ABS(crop_left_x);
	out->reg_pre_crop_hfde_size = reg_outer_src_hsize;

	out->reg_pre_crop_v_st = ABS(crop_up_y);
	out->reg_pre_crop_v_length =
		reg_outer_src_vsize - ABS(crop_down_y) - ABS(crop_up_y);
	out->reg_pre_crop_vfde_size = reg_outer_src_vsize;

	out->reg_ins_path = 1;
	out->reg_pre_crop_h_crop_en = 1;
	out->reg_pre_crop_v_crop_en = 1;

	mae_dev_dbg(mae_dev->dev, "[%s] reg_pre_crop_h_st(%d), reg_pre_crop_h_length(%d), ",
			__func__,
			out->reg_pre_crop_h_st,
			out->reg_pre_crop_h_length);
	mae_dev_dbg(mae_dev->dev, "reg_pre_crop_hfde_size(%d), reg_pre_crop_v_st(%d), reg_pre_crop_v_length(%d), ",
			out->reg_pre_crop_hfde_size,
			out->reg_pre_crop_v_st,
			out->reg_pre_crop_v_length);
	mae_dev_dbg(mae_dev->dev, "pre_crop_vfde_size(%d), ins_path(%d), pre_crop_h_crop_en(%d), pre_crop_v_crop_en(%d)\n",
			out->reg_pre_crop_vfde_size,
			out->reg_ins_path,
			out->reg_pre_crop_h_crop_en,
			out->reg_pre_crop_v_crop_en);

	return true;
}

// follow DE crop formula
static bool mtk_mae_padding(struct mtk_mae_dev *mae_dev,
			const struct padding_setting_in *in,
			struct padding_setting_out *out)
{
	int32_t pad_left_x = -in->left;
	int32_t pad_right_x = -in->right;
	int32_t pad_down_y = -in->down;
	int32_t pad_up_y = -in->up;

	mae_dev_dbg(mae_dev->dev, "[%s] l(%d), r(%d), d(%d), u(%d), crop_output_h_size(%d), crop_output_v_size(%d)\n",
		__func__,
		in->left, in->right, in->down, in->up,
		in->crop_output_h_size,
		in->crop_output_v_size);

	if (in->crop_output_h_size <= 0 || in->crop_output_v_size <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] error input height/width(%d/%d)",
			__func__, in->crop_output_v_size, in->crop_output_h_size);
		return false;
	}

	out->reg_post_ins_blk_hpre = ABS(pad_left_x);
	out->reg_post_ins_h_length = DIV_CEIL_POS(in->crop_output_h_size, 4) * 4;
	out->reg_post_ins_hfde_size =
		DIV_CEIL_POS((out->reg_post_ins_h_length + ABS(pad_right_x) + ABS(pad_left_x)), 4) * 4;

	out->reg_post_ins_blk_vpre = ABS(pad_up_y);
	out->reg_post_ins_v_length = in->crop_output_v_size;
	out->reg_post_ins_vfde_size = in->crop_output_v_size + ABS(pad_up_y) + ABS(pad_down_y);

	out->reg_h_size = out->reg_post_ins_h_length + ABS(pad_right_x) + ABS(pad_left_x);
	out->reg_v_size = out->reg_post_ins_vfde_size;
	out->reg_post_ins_hv_insert_en = 1;

	if (out->reg_post_ins_blk_vpre > 0)
		if (out->reg_post_ins_blk_hpre < 4 &&
		(out->reg_post_ins_hfde_size - out->reg_post_ins_h_length - out->reg_post_ins_blk_hpre) < 4) {
			out->reg_post_ins_hfde_size = out->reg_post_ins_hfde_size + 4;
			out->reg_h_size = out->reg_post_ins_h_length + ABS(pad_right_x) + ABS(pad_left_x) + 4;
		}

	mae_dev_dbg(mae_dev->dev, "[%s] reg_post_ins_blk_hpre(%d), reg_post_ins_h_length(%d), reg_post_ins_hfde_size(%d), ",
			__func__,
			out->reg_post_ins_blk_hpre,
			out->reg_post_ins_h_length,
			out->reg_post_ins_hfde_size);
	mae_dev_dbg(mae_dev->dev, "reg_post_ins_blk_vpre(%d), reg_post_ins_v_length(%d), reg_post_ins_vfde_size(%d), ",
			out->reg_post_ins_blk_vpre,
			out->reg_post_ins_v_length,
			out->reg_post_ins_vfde_size);
	mae_dev_dbg(mae_dev->dev, "reg_h_size(%d), reg_v_size(%d), reg_post_ins_hv_insert_en(%d)\n",
			out->reg_h_size,
			out->reg_v_size,
			out->reg_post_ins_hv_insert_en);

	return true;
}

// follow DE resize formula
static bool mtk_mae_resize(struct mtk_mae_dev *mae_dev,
		const struct rsz_setting_in *in,
		struct rsz_setting_out *out)
{
	int32_t inputHeight_used = in->rsz_input_v_size;
	int32_t inputWidth_used = in->rsz_input_h_size;

	mae_dev_dbg(mae_dev->dev, "[%s] rsz_input_h_size(%d), "
			"rsz_input_v_size(%d), "
			"rsz_output_h_size(%d), "
			"rsz_output_v_size(%d), "
			"rsz_input_ch(%d)\n",
			__func__,
			in->rsz_input_h_size,
			in->rsz_input_v_size,
			in->rsz_output_h_size,
			in->rsz_output_v_size,
			in->rsz_input_ch);

	if (inputHeight_used <= 0 || inputWidth_used <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] input height/width(%d/%d) should not be negative",
				__func__, inputHeight_used, inputWidth_used);
		return false;
	}

	out->reg_1p_path_en = 1;
	out->reg_h_size_usr_md = 1;
	out->reg_v_size_usr_md = 1;
	out->reg_scale_ve_en = 1;
	out->reg_scale_ho_en = 1;
	out->reg_rsz_u2s = 1;

	if (in->rsz_input_h_size < in->rsz_output_h_size)
		out->reg_order = 1;
	else
	 	out->reg_order = 0;

	if (in->rsz_input_h_size > in->rsz_output_h_size &&
		in->rsz_input_v_size > in->rsz_output_v_size) {
		// cb mode
		out->reg_rsz_mode_ho = 1;
		out->reg_rsz_mode_ve = 1;
		out->reg_cb_factor_ho = (int32_t)((in->rsz_output_h_size) << 20) / inputWidth_used;
		out->reg_cb_factor_ve = (int32_t)((in->rsz_output_v_size) << 20) / inputHeight_used;

		if ((out->reg_cb_factor_ho - ((out->reg_cb_factor_ho >> 8) << 8)) > 0)
			out->reg_cb_factor_ho = out->reg_cb_factor_ho + 1;

		if ((out->reg_cb_factor_ve - ((out->reg_cb_factor_ve >> 8) << 8)) > 0)
			out->reg_cb_factor_ve = out->reg_cb_factor_ve + 1;
		// confirm with DE
		out->reg_mode_c_ve = 1;
		out->reg_mode_c_ho = 1;
	} else {
		// bilinear mode
		out->reg_scale_factor_ve = (int32_t)((((inputHeight_used - 1) << 20) +
			((in->rsz_output_v_size - 1) >> 1)) / (in->rsz_output_v_size - 1));
		out->reg_v_shift_mode_en = 0;
		out->reg_ini_factor_ve = 0;

		out->reg_scale_factor_ho = (int32_t)((((inputWidth_used - 1) << 20) +
			((in->rsz_output_h_size - 1) >> 1)) / (in->rsz_output_h_size - 1));
		out->reg_h_shift_mode_en = 0;
		out->reg_ini_factor_ho = 0;

		out->reg_mode_c_ve = 1;
		out->reg_mode_c_ho = 1;
	}

	mae_dev_dbg(mae_dev->dev, "[%s] reg_scale_factor_ve(%d), reg_v_shift_mode_en(%d), reg_ini_factor_ve(%d),",
			__func__,
			out->reg_scale_factor_ve,
			out->reg_v_shift_mode_en,
			out->reg_ini_factor_ve);
	mae_dev_dbg(mae_dev->dev, "[%s]  reg_scale_factor_ho(%d), reg_h_shift_mode_en(%d), reg_ini_factor_ho(%d), ",
			__func__,
			out->reg_scale_factor_ho,
			out->reg_h_shift_mode_en,
			out->reg_ini_factor_ho);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_1p_path_en(%d), reg_order(%d), reg_h_size_usr_md(%d), ",
			__func__,
			out->reg_1p_path_en,
			out->reg_order,
			out->reg_h_size_usr_md);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_v_size_usr_md(%d), reg_scale_ve_en(%d), reg_scale_ho_en(%d), ",
			__func__,
			out->reg_v_size_usr_md,
			out->reg_scale_ve_en,
			out->reg_scale_ho_en);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_mode_c_ve(%d), reg_mode_c_ho(%d), reg_rsz_mode_ho(%d), ",
			__func__,
			out->reg_mode_c_ve,
			out->reg_mode_c_ho,
			out->reg_rsz_mode_ho);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_rsz_mode_ve(%d), reg_cb_factor_ho(%d), reg_cb_factor_ve(%d), rsz_u2s(%d)\n",
			__func__,
			out->reg_rsz_mode_ve,
			out->reg_cb_factor_ho,
			out->reg_cb_factor_ve,
			out->reg_rsz_u2s);

	return true;
}

static bool mtk_mae_resize_fld(struct mtk_mae_dev *mae_dev,
		const struct rsz_setting_in *in,
		struct rsz_setting_out *out)
{
	int32_t inputHeight_used = in->rsz_input_v_size;
	int32_t inputWidth_used = in->rsz_input_h_size;

	mae_dev_dbg(mae_dev->dev, "[%s] rsz_input_h_size(%d), rsz_input_v_size(%d)\n",
			__func__,
			in->rsz_input_h_size,
			in->rsz_input_v_size);
	mae_dev_dbg(mae_dev->dev, "[%s] rsz_output_h_size(%d), rsz_output_v_size(%d), rsz_input_ch(%d)\n",
			__func__,
			in->rsz_output_h_size,
			in->rsz_output_v_size,
			in->rsz_input_ch);

	if (inputHeight_used <= 0 || inputWidth_used <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] input height/width(%d/%d) should not be negative",
				__func__, inputHeight_used, inputWidth_used);
		return false;
	}

	out->reg_1p_path_en = 1;
	out->reg_h_size_usr_md = 1;
	out->reg_v_size_usr_md = 1;
	out->reg_scale_ve_en = 1;
	out->reg_scale_ho_en = 1;
	out->reg_rsz_u2s = 1;
	out->reg_order = 0;

	if (in->rsz_input_h_size > in->rsz_output_h_size) {
		// cb mode
		out->reg_rsz_mode_ho = 1;

		out->reg_cb_factor_ho = (int32_t)((in->rsz_output_h_size) << 20) / inputWidth_used;

		if ((out->reg_cb_factor_ho - ((out->reg_cb_factor_ho >> 8) << 8)) > 0)
			out->reg_cb_factor_ho = out->reg_cb_factor_ho + 1;

		out->reg_scale_factor_ho = out->reg_cb_factor_ho;
	} else {
		// bilinear mode
		out->reg_rsz_mode_ho = 0;

		out->reg_scale_factor_ho = (int32_t)(((inputWidth_used << 20) +
			(in->rsz_output_h_size >> 1)) / in->rsz_output_h_size);

		if (in->rsz_output_h_size > in->rsz_input_h_size) {
			out->reg_h_shift_mode_en = 1;
			out->reg_ini_factor_ho =
				(int32_t)((((in->rsz_input_h_size << 20) / in->rsz_output_h_size) + (1 << 20)) / 2);
		} else if (in->rsz_output_h_size == in->rsz_input_h_size) {
			out->reg_h_shift_mode_en = 0;
			out->reg_ini_factor_ho = 0;
		}

		out->reg_mode_c_ho = 1;
	}

	if (in->rsz_input_v_size > in->rsz_output_v_size) {
		// cb mode
		out->reg_rsz_mode_ve = 1;

		out->reg_cb_factor_ve = (int32_t)((in->rsz_output_v_size) << 20) / inputHeight_used;

		if ((out->reg_cb_factor_ve - ((out->reg_cb_factor_ve >> 8) << 8)) > 0)
			out->reg_cb_factor_ve = out->reg_cb_factor_ve + 1;

		out->reg_scale_factor_ve = out->reg_cb_factor_ve;
	} else {
		// bilinear mode
		out->reg_rsz_mode_ve = 0;

		out->reg_scale_factor_ve = (int32_t)(((inputHeight_used << 20) +
			(in->rsz_output_v_size >> 1)) / in->rsz_output_v_size);

		if (in->rsz_output_v_size > in->rsz_input_v_size) {
			out->reg_v_shift_mode_en = 1;
			out->reg_ini_factor_ve =
				(int32_t)((((in->rsz_input_v_size << 20) / in->rsz_output_v_size) + (1 << 20)) / 2);
		} else if (in->rsz_output_v_size == in->rsz_input_v_size) {
			out->reg_v_shift_mode_en = 0;
			out->reg_ini_factor_ve = 0;
		}

		out->reg_mode_c_ve = 1;
	}

	mae_dev_dbg(mae_dev->dev, "[%s] reg_scale_factor_ve(%d), reg_v_shift_mode_en(%d), reg_ini_factor_ve(%d),",
			__func__,
			out->reg_scale_factor_ve,
			out->reg_v_shift_mode_en,
			out->reg_ini_factor_ve);
	mae_dev_dbg(mae_dev->dev, "[%s]  reg_scale_factor_ho(%d), reg_h_shift_mode_en(%d), reg_ini_factor_ho(%d), ",
			__func__,
			out->reg_scale_factor_ho,
			out->reg_h_shift_mode_en,
			out->reg_ini_factor_ho);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_1p_path_en(%d), reg_order(%d), reg_h_size_usr_md(%d), ",
			__func__,
			out->reg_1p_path_en,
			out->reg_order,
			out->reg_h_size_usr_md);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_v_size_usr_md(%d), reg_scale_ve_en(%d), reg_scale_ho_en(%d), ",
			__func__,
			out->reg_v_size_usr_md,
			out->reg_scale_ve_en,
			out->reg_scale_ho_en);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_mode_c_ve(%d), reg_mode_c_ho(%d), reg_rsz_mode_ho(%d), ",
			__func__,
			out->reg_mode_c_ve,
			out->reg_mode_c_ho,
			out->reg_rsz_mode_ho);
	mae_dev_dbg(mae_dev->dev, "[%s] reg_rsz_mode_ve(%d), reg_cb_factor_ho(%d), reg_cb_factor_ve(%d), rsz_u2s(%d)\n",
			__func__,
			out->reg_rsz_mode_ve,
			out->reg_cb_factor_ho,
			out->reg_cb_factor_ve,
			out->reg_rsz_u2s);

	return true;
}

static bool mtk_mae_config_rsz(struct mtk_mae_dev *mae_dev,
				struct EnqueParam *param,
				struct cmdq_pkt *pkt,
				uint32_t rsz_offset,
				uint32_t loop,
				uint32_t idx)
{
	struct crop_setting_in crop_in = {0};
	struct crop_setting_out crop_out = {0};
	struct padding_setting_in padding_in = {0};
	struct padding_setting_out padding_out = {0};
	struct rsz_setting_in rsz_in = {0};
	struct rsz_setting_out rsz_out = {0};
	uint32_t i;

	if (mae_dev->core_sel[idx][loop] < 0 || mae_dev->core_sel[idx][loop] >= FD_PATTERN_NUM)
		return false;

	// crop
	if (param->image[loop].enRoi) {
		crop_in.start_x = param->image[loop].roi.x1;
		crop_in.end_x = param->image[loop].roi.x2;
		crop_in.start_y = param->image[loop].roi.y1;
		crop_in.end_y = param->image[loop].roi.y2;
	} else {
		crop_in.start_x = 0;
		crop_in.end_x = param->image[loop].imgWidth;
		crop_in.start_y = 0;
		crop_in.end_y = param->image[loop].imgHeight;
	}
	crop_in.input_h_size = param->image[loop].imgWidth;
	crop_in.input_v_size = param->image[loop].imgHeight;

	if (!mtk_mae_crop(mae_dev, &crop_in, &crop_out))
		return false;

	// padding
	if (param->maeMode == FD_V0 || param->maeMode == FD_V1_IPN) {
		padding_in.left = 0;
		padding_in.up = 0;

		if (param->image[loop].enRoi) {
			padding_in.crop_output_h_size = param->image[loop].roi.x2 - param->image[loop].roi.x1;
			padding_in.crop_output_v_size = param->image[loop].roi.y2 - param->image[loop].roi.y1;
		} else {
			padding_in.crop_output_h_size = param->image[loop].imgWidth;
			padding_in.crop_output_v_size = param->image[loop].imgHeight;
		}

		padding_in.right =
			DIV_CEIL_POS(fd_pattern_width[mae_dev->core_sel[idx][loop]] * padding_in.crop_output_h_size,
				param->image[loop].resizeWidth) - padding_in.crop_output_h_size;
		padding_in.down =
			DIV_CEIL_POS(fd_pattern_height[mae_dev->core_sel[idx][loop]] * padding_in.crop_output_h_size,
				param->image[loop].resizeWidth) - padding_in.crop_output_v_size;

		if (padding_in.right < 0 || padding_in.down < 0) {
			mae_dev_dbg(mae_dev->dev,
				"pad neg val r:%d d:%d m:%d py:%d l:%d img:%d,%d rsz:%d,%d roi:%d,%d,%d,%d,%d",
				padding_in.right, padding_in.down,
				param->maeMode, param->pyramidNumber, loop,
				param->image[0].imgWidth, param->image[0].imgHeight,
				param->image[0].resizeWidth, param->image[0].resizeHeight,
				param->image[0].enRoi, param->image[0].roi.x1,
				param->image[0].roi.y1, param->image[0].roi.x2,
				param->image[0].roi.y2);
			for (i = 0; i < param->pyramidNumber; i++)
				mae_dev_dbg(mae_dev->dev,
					"py(%d) img(%d,%d) rsz(%d,%d) roi(%d)(%d,%d -> %d,%d) pad(%d)(%d,%d,%d,%d)",
					i, param->image[i].imgWidth, param->image[i].imgHeight,
					param->image[i].resizeWidth, param->image[i].resizeHeight,
					param->image[i].enRoi, param->image[i].roi.x1,
					param->image[i].roi.y1, param->image[i].roi.x2,
					param->image[i].roi.y2, param->image[i].enPadding,
					param->image[i].padding.left, param->image[i].padding.right,
					param->image[i].padding.down, param->image[i].padding.up);
			// force to 640x480
			// return false;
			padding_in.right = 0;
			padding_in.down = 0;
		}
	} else if (param->maeMode == FD_V1_FPN) {
		padding_in.left = 0;
		padding_in.up = 0;

		if (param->image[loop].enRoi) {
			padding_in.crop_output_h_size = param->image[loop].roi.x2 - param->image[loop].roi.x1;
			padding_in.crop_output_v_size = param->image[loop].roi.y2 - param->image[loop].roi.y1;
		} else {
			padding_in.crop_output_h_size = param->image[loop].imgWidth;
			padding_in.crop_output_v_size = param->image[loop].imgHeight;
		}

		padding_in.right =
			DIV_CEIL_POS(FPN_PYRAMID_WIDTH * padding_in.crop_output_h_size,
				param->image[loop].resizeWidth) - padding_in.crop_output_h_size;
		padding_in.down =
			DIV_CEIL_POS(FPN_PYRAMID_HEIGHT * padding_in.crop_output_h_size,
				param->image[loop].resizeWidth) - padding_in.crop_output_v_size;

		if (padding_in.right < 0 || padding_in.down < 0) {
			mae_dev_dbg(mae_dev->dev,
				"pad neg val r:%d d:%d m:%d py:%d l:%d img:%d,%d rsz:%d,%d roi:%d,%d,%d,%d,%d",
				padding_in.right, padding_in.down,
				param->maeMode, param->pyramidNumber, loop,
				param->image[0].imgWidth, param->image[0].imgHeight,
				param->image[0].resizeWidth, param->image[0].resizeHeight,
				param->image[0].enRoi, param->image[0].roi.x1,
				param->image[0].roi.y1, param->image[0].roi.x2,
				param->image[0].roi.y2);
			for (i = 0; i < param->pyramidNumber; i++)
				mae_dev_dbg(mae_dev->dev,
					"py(%d) img(%d,%d) rsz(%d,%d) roi(%d)(%d,%d -> %d,%d) pad(%d)(%d,%d,%d,%d)",
					i, param->image[i].imgWidth, param->image[i].imgHeight,
					param->image[i].resizeWidth, param->image[i].resizeHeight,
					param->image[i].enRoi, param->image[i].roi.x1,
					param->image[i].roi.y1, param->image[i].roi.x2,
					param->image[i].roi.y2, param->image[i].enPadding,
					param->image[i].padding.left, param->image[i].padding.right,
					param->image[i].padding.down, param->image[i].padding.up);
			// force to 640x480
			// return false;
			padding_in.right = 0;
			padding_in.down = 0;
		}
	} else {
		padding_in.left = param->image[loop].padding.left;
		padding_in.right = param->image[loop].padding.right;
		padding_in.down = param->image[loop].padding.down;
		padding_in.up = param->image[loop].padding.up;
		if (param->image[loop].enRoi) {
			padding_in.crop_output_h_size = param->image[loop].roi.x2 - param->image[loop].roi.x1;
			padding_in.crop_output_v_size = param->image[loop].roi.y2 - param->image[loop].roi.y1;
		} else {
			padding_in.crop_output_h_size = param->image[loop].imgWidth;
			padding_in.crop_output_v_size = param->image[loop].imgHeight;
		}
	}

	if (!mtk_mae_padding(mae_dev, &padding_in, &padding_out))
		return false;

	rsz_in.rsz_input_h_size =
		padding_in.crop_output_h_size + padding_in.left + padding_in.right;
	rsz_in.rsz_input_v_size =
		padding_in.crop_output_v_size + padding_in.down + padding_in.up;

	switch (param->maeMode) {
	case FD_V0:
	case FD_V1_IPN:
		rsz_in.rsz_output_h_size = fd_pattern_width[mae_dev->core_sel[idx][loop]];
		rsz_in.rsz_output_v_size = fd_pattern_height[mae_dev->core_sel[idx][loop]];
		break;
	case FD_V1_FPN:
		rsz_in.rsz_output_h_size = FPN_PYRAMID_WIDTH;
		rsz_in.rsz_output_v_size = FPN_PYRAMID_HEIGHT;
		break;
	case ATTR_V0:
	case FAC_V1:
		rsz_in.rsz_output_h_size = param->image[loop].resizeWidth;
		rsz_in.rsz_output_v_size = param->image[loop].resizeHeight;
		break;
	default:
		break;
	}
	rsz_in.rsz_input_ch = 8;

	if (param->maeMode == FAC_V1) {
		if (!mtk_mae_resize_fld(mae_dev, &rsz_in, &rsz_out))
			return false;
	} else {
		if (!mtk_mae_resize(mae_dev, &rsz_in, &rsz_out))
			return false;
	}

	MAE_CMDQ_WRITE_REG(pkt, REG_00C8_RSZ1 + rsz_offset,
			REG_RANGE(crop_out.reg_pre_crop_h_st, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00CC_RSZ1 + rsz_offset,
			REG_RANGE(crop_out.reg_pre_crop_h_length, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00D0_RSZ1 + rsz_offset,
			REG_RANGE(crop_out.reg_pre_crop_hfde_size, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00D4_RSZ1 + rsz_offset,
			REG_RANGE(crop_out.reg_pre_crop_v_st, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00D8_RSZ1 + rsz_offset,
			REG_RANGE(crop_out.reg_pre_crop_v_length, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00C0_RSZ1 + rsz_offset,
			0x80 +
			(REG_RANGE(crop_out.reg_pre_crop_v_crop_en, 0, 0) << 1) +
			REG_RANGE(crop_out.reg_pre_crop_h_crop_en, 0, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_010C_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_blk_hpre, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0110_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_h_length, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0114_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_hfde_size, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0118_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_blk_vpre, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_011C_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_v_length, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0120_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_vfde_size, 13, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00A0_RSZ1 + rsz_offset,
			(1 << 15) + REG_RANGE(padding_out.reg_h_size, 13, 0));

	// reg_v_size_usr_md_1[15] = 1
	MAE_CMDQ_WRITE_REG(pkt,
			REG_00A4_RSZ1 + rsz_offset,
			(1 << 15) + REG_RANGE(padding_out.reg_v_size, 13, 0));

	// reg_post_ins_boundary_md_1[0] = 0, reg_post_ins_de_start_trig_md_1 = 0
	MAE_CMDQ_WRITE_REG(pkt,
			REG_0104_RSZ1 + rsz_offset,
			REG_RANGE(padding_out.reg_post_ins_hv_insert_en, 0, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0004_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_ini_factor_ho, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0008_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_ini_factor_ho, 27, 16));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_000C_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_ini_factor_ve, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0010_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_ini_factor_ve, 27, 16));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_001C_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_scale_factor_ho, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0020_RSZ1 + rsz_offset,
			(REG_RANGE(rsz_out.reg_scale_factor_ho, 27, 24) << 12) +
			(REG_RANGE(rsz_out.reg_h_shift_mode_en, 0, 0) << 9) +
			(REG_RANGE(rsz_out.reg_scale_ho_en, 0, 0) << 8) +
			(REG_RANGE(rsz_out.reg_scale_factor_ho, 23, 16)));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0024_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_scale_factor_ve, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0028_RSZ1 + rsz_offset,
			(REG_RANGE(rsz_out.reg_scale_factor_ve, 27, 24) << 12) +
			(REG_RANGE(rsz_out.reg_v_shift_mode_en, 0, 0) << 9) +
			(REG_RANGE(rsz_out.reg_scale_ve_en, 0, 0) << 8) +
			(REG_RANGE(rsz_out.reg_scale_factor_ve, 23, 16)));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_002C_RSZ1 + rsz_offset,
			(REG_RANGE(rsz_out.reg_mode_c_ve, 1, 0) << 8) +
			(REG_RANGE(rsz_out.reg_mode_c_ho, 1, 0)));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0034_RSZ1 + rsz_offset,
			(REG_RANGE(rsz_out.reg_rsz_u2s, 0, 0) << 1) + 0);

	MAE_CMDQ_WRITE_REG(pkt,
			REG_005C_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_cb_factor_ho, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0060_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_cb_factor_ho, 19, 16));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0064_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_cb_factor_ve, 15, 0));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0068_RSZ1 + rsz_offset,
			REG_RANGE(rsz_out.reg_cb_factor_ve, 19, 16));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00A8_RSZ1 + rsz_offset,
			rsz_in.rsz_output_h_size);

	MAE_CMDQ_WRITE_REG(pkt,
			REG_00AC_RSZ1 + rsz_offset,
			rsz_in.rsz_output_v_size);

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0180_RSZ1 + rsz_offset,
			(REG_RANGE(rsz_out.reg_order, 0, 0) << 2) +
			(REG_RANGE(rsz_out.reg_rsz_mode_ho, 0, 0) << 4) +
			(REG_RANGE(rsz_out.reg_rsz_mode_ve, 0, 0) << 8));

	MAE_CMDQ_WRITE_REG(pkt,
			REG_0184_RSZ1 + rsz_offset,
			0xb);

	return true;
}

static void mtk_mae_fd_post(struct mtk_mae_dev *mae_dev,
			struct EnqueParam *param,
			struct cmdq_pkt *pkt,
			uint32_t core_offset,
			int loop,
			MAE_MODE mode)
{
	struct EnqueImage *image = &param->image[loop];

	// formula from algo
	MAE_CMDQ_WRITE_REG(pkt, MAE_REG_X_OFFSET_0 + core_offset,
		(image->enRoi) ? (image->roi.x1) : 0);
	mae_dev_dbg(mae_dev->dev, "[%s] 0x%x = 0x%x",
		__func__, MAE_BASE + MAE_REG_X_OFFSET_0 + core_offset,
		(image->enRoi) ? (image->roi.x1) : 0);

	MAE_CMDQ_WRITE_REG(pkt, MAE_REG_Y_OFFSET_0 + core_offset,
		(image->enRoi) ? (image->roi.y1) : 0);
	mae_dev_dbg(mae_dev->dev, "[%s] 0x%x = 0x%x",
		__func__, MAE_BASE + MAE_REG_Y_OFFSET_0 + core_offset,
		(image->enRoi) ? (image->roi.y1) : 0);

	MAE_CMDQ_WRITE_REG(pkt, MAE_REG_MMFD_O_SCALE_0 + core_offset,
		(image->enRoi) ?
		ROUND(((image->roi.x2 - image->roi.x1) << 9), image->resizeWidth) :
		ROUND((image->imgWidth << 9), image->resizeWidth));
	mae_dev_dbg(mae_dev->dev, "[%s] 0x%x = 0x%x",
		__func__, MAE_BASE + MAE_REG_MMFD_O_SCALE_0 + core_offset,
		(image->enRoi) ?
		ROUND(((image->roi.x2 - image->roi.x1) << 9), image->resizeWidth) :
		ROUND((image->imgWidth << 9), image->resizeWidth));

	MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_SIZE0 + core_offset,
		(uint32_t)image->imgWidth);
	mae_dev_dbg(mae_dev->dev, "[%s] 0x%x = 0x%x",
		__func__, MAE_BASE + MAE_REG_H_SIZE0 + core_offset,
		(uint32_t)image->imgWidth);

	MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_SIZE0 + core_offset,
		(uint32_t)image->imgHeight);
	mae_dev_dbg(mae_dev->dev, "[%s] 0x%x = 0x%x",
		__func__, MAE_BASE + MAE_REG_V_SIZE0 + core_offset,
		(uint32_t)image->imgHeight);

	if (mode == FD_V1_IPN) {
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MIN0 + core_offset, 0x0);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MIN0 + core_offset, 0x0);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MAX0 + core_offset,
				(uint32_t)image->imgWidth);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MAX0 + core_offset,
				(uint32_t)image->imgHeight);

		// 0x3E2 is representation of -30 by 2's complement
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_SCORE_TH0 + core_offset, 0x3E2); // -30
	} else if (mode == FD_V1_FPN) {
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MIN0 + core_offset, 0x0);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MIN0 + core_offset, 0x0);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_H_MAX0 + core_offset,
				(uint32_t)image->imgWidth);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_V_MAX0 + core_offset,
				(uint32_t)image->imgHeight);
		MAE_CMDQ_WRITE_REG(pkt, MAE_REG_SCORE_TH0 + core_offset, g_data->data->fd_fpn_threshold);
	}
}

static void mtk_mae_config_aiseg_post(struct mtk_mae_dev *mae_dev,
				struct EnqueParam *param,
				struct cmdq_pkt *pkt)
{
	uint32_t i, j;
	uint32_t reg_addr;

	for (i = 0; i < AISEG_POP_GROUP_SIZE; i++) {
		reg_addr = REG_01A0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET;

		for (j = 0; j < SEMANTIC_MERGE_NUM / 2; j++) {
			MAE_CMDQ_WRITE_REG(pkt, reg_addr,
				(param->semanticMerge[i][2*j+1] << 8) + param->semanticMerge[i][2*j]);
			reg_addr += COMMON_REG_SIZE;
		}

		for (j = 0; j < PERSON_MERGE_NUM / 2; j++) {
			MAE_CMDQ_WRITE_REG(pkt, reg_addr,
				(param->personMerge[i][2*j+1] << 8) + param->personMerge[i][2*j]);
			reg_addr += COMMON_REG_SIZE;
		}

		for (j = 0; j < MERGE_CONFIDENCE_NUM / 2; j++) {
			MAE_CMDQ_WRITE_REG(pkt, reg_addr,
				(param->mergeConfidence[i][2*j+1] << 8) + param->mergeConfidence[i][2*j]);
			reg_addr += COMMON_REG_SIZE;
		}
	}
}

static void mtk_mae_aiseg_crop(struct mtk_mae_dev *mae_dev,
				uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
				uint32_t feature_map_size, uint32_t output_size_x, uint32_t output_size_y,
				uint32_t shift_bit, struct aiseg_crop_setting_out *out)
{
	// Variables
	uint32_t output_map_size_x, output_map_size_y;
	uint32_t scale_x_0, scale_x_1, scale_y_0, scale_y_1;
	uint32_t scale_factor, crop_st, crop_length, init_factor, rounding;

	// Default setting
	out->reg_mode_c_ho = 1;
	out->reg_mode_c_ve = 1;
	out->reg_1p_path_en = 1;
	out->reg_h_size_usr_md = 1;
	out->reg_v_size_usr_md = 1;
	out->reg_scale_ve_en = 1;
	out->reg_scale_ho_en = 1;
	out->reg_rsz_s2u = 1;
	out->reg_order = 1;
	out->reg_postproc_en = 1;

	mae_dev_dbg(mae_dev->dev, "[%s] crop(%d,%d -> %d,%d) map(%d) output x/y(%d/%d) shift(%d)\n",
		__func__, x0, y0, x1, y1, feature_map_size, output_size_x, output_size_y, shift_bit);

	mae_dev_dbg(mae_dev->dev, "=== Default Setting ===\n");
	mae_dev_dbg(mae_dev->dev, "reg_mode_c_ho = %d, reg_mode_c_ve = %d\n",
		out->reg_mode_c_ho, out->reg_mode_c_ve);
	mae_dev_dbg(mae_dev->dev, "reg_1p_path_en = %d, reg_h_size_usr_md = %d, reg_v_size_usr_md = %d\n",
		out->reg_1p_path_en, out->reg_h_size_usr_md, out->reg_v_size_usr_md);
	mae_dev_dbg(mae_dev->dev, "reg_scale_ve_en = %d, reg_scale_ho_en = %d\n",
		out->reg_scale_ve_en, out->reg_scale_ho_en);
	mae_dev_dbg(mae_dev->dev, "reg_rsz_s2u = %d, reg_order = %d, reg_postproc_en = %d\n",
		out->reg_rsz_s2u, out->reg_order, out->reg_postproc_en);

	// Horizontal resizer
	output_map_size_x = output_size_x - 1;
	output_map_size_y = output_size_y - 1;

	scale_x_0 = x0 * (feature_map_size - 1);
	scale_x_1 = x1 * (feature_map_size - 1);

	scale_factor = (scale_x_1 - scale_x_0) * (1 << (20 - shift_bit)) / output_map_size_x ;

	crop_st = (scale_x_0 >> shift_bit) << shift_bit;
	(scale_x_1 & 0x3FF) ? (rounding = 1) : (rounding = 0);
	crop_length = (feature_map_size << shift_bit) - crop_st
		- (((feature_map_size - 1) << shift_bit) - (((scale_x_1 >> shift_bit) + rounding) << shift_bit));
	init_factor = (scale_x_0 - crop_st) * ((uint32_t)1 << (20 - shift_bit));

	out->reg_pre_crop_hfde_size =  DIV_CEIL_POS(feature_map_size, 4) * 4; // 4 align
	out->reg_nve_op_attr_auto_mode_ve = 0;
	out->reg_nve_op_attr_auto_mode_ho = 0;
	out->reg_pre_crop_h_crop_en = 1;
	out->reg_pre_crop_h_st = crop_st >> shift_bit;
	out->reg_pre_crop_h_length = (crop_length + (1 << (shift_bit - 1))) >> shift_bit;
	out->reg_h_size = out->reg_pre_crop_h_length;
	out->reg_scale_factor_ho_0 = scale_factor & 0xFFFF;
	out->reg_scale_factor_ho_1 = (scale_factor >> 16) & 0xFF;
	out->reg_ini_factor_ho_0 = init_factor & 0xFFFF;
	out->reg_ini_factor_ho_1 = (init_factor >> 16) & 0xFFF;

	mae_dev_dbg(mae_dev->dev, "=== Horizontal Resizer ===\n");
	mae_dev_dbg(mae_dev->dev, "reg_nve_op_attr_auto_mode_ho = %d, reg_pre_crop_h_crop_en = %d\n",
		out->reg_nve_op_attr_auto_mode_ho, out->reg_pre_crop_h_crop_en);
	mae_dev_dbg(mae_dev->dev, "reg_pre_crop_h_st = %d, reg_pre_crop_h_length = %d, reg_h_size = %d\n",
		out->reg_pre_crop_h_st, out->reg_pre_crop_h_length, out->reg_h_size);
	mae_dev_dbg(mae_dev->dev, "reg_scale_factor_ho_0 = 0x%x, reg_scale_factor_ho_1 = 0x%x\n",
		out->reg_scale_factor_ho_0, out->reg_scale_factor_ho_1);
	mae_dev_dbg(mae_dev->dev, "reg_ini_factor_ho_0 = 0x%x, reg_ini_factor_ho_1 = 0x%x reg_pre_crop_hfde_size=%d\n",
		out->reg_ini_factor_ho_0, out->reg_ini_factor_ho_1, out->reg_pre_crop_hfde_size);

	// Veritical resizer
	scale_y_0 = y0 * (feature_map_size - 1);
	scale_y_1 = y1 * (feature_map_size - 1);
	scale_factor = (scale_y_1 - scale_y_0) * (1 << (20 - shift_bit)) / output_map_size_y;

	crop_st = (scale_y_0 >> shift_bit) << shift_bit;
	(scale_y_1 & 0x3FF) ? (rounding = 1) : (rounding = 0);
	crop_length = (feature_map_size << shift_bit) - crop_st -
		(((feature_map_size - 1) << shift_bit) - (((scale_y_1 >> shift_bit) + rounding) << shift_bit));
	init_factor = (scale_y_0 - crop_st) * ((uint32_t)1 << (20 - shift_bit));

	out->reg_pre_crop_v_crop_en = 1;
	out->reg_pre_crop_v_st = crop_st >> shift_bit;
	out->reg_pre_crop_v_length = (crop_length + 512) >> shift_bit;
	out->reg_v_size = out->reg_pre_crop_v_length;
	out->reg_scale_factor_ve_0 = scale_factor & 0xFFFF;
	out->reg_scale_factor_ve_1 = (scale_factor >> 16) & 0xFF;
	out->reg_ini_factor_ve_0 = init_factor & 0xFFFF;
	out->reg_ini_factor_ve_1 = (init_factor >> 16) & 0xFFF;

	mae_dev_dbg(mae_dev->dev, "=== Vertial Resizer ===\n");
	mae_dev_dbg(mae_dev->dev, "reg_nve_op_attr_auto_mode_ve = %d, reg_pre_crop_v_crop_en = %d\n",
		out->reg_nve_op_attr_auto_mode_ve, out->reg_pre_crop_v_crop_en);
	mae_dev_dbg(mae_dev->dev, "reg_pre_crop_v_st = %d, reg_pre_crop_v_length = %d, reg_v_size = %d\n",
		out->reg_pre_crop_v_st, out->reg_pre_crop_v_length, out->reg_v_size);
	mae_dev_dbg(mae_dev->dev, "reg_scale_factor_ve_0 = 0x%x, reg_scale_factor_ve_1 = 0x%x\n",
		out->reg_scale_factor_ve_0, out->reg_scale_factor_ve_1);
	mae_dev_dbg(mae_dev->dev, "reg_ini_factor_ve_0 = 0x%x, reg_ini_factor_ve_1 = 0x%x\n",
		out->reg_ini_factor_ve_0, out->reg_ini_factor_ve_1);
}

static void mtk_mae_config_aiseg_crop(struct mtk_mae_dev *mae_dev,
				struct EnqueParam *param,
				struct cmdq_pkt *pkt)
{
	uint32_t i;
	struct aiseg_crop_setting_out crop_reg = {0};

	for (i = 0; i < AISEG_CROP_NUM; i++) {
		mtk_mae_aiseg_crop(mae_dev, param->aisegCrop[i].x0, param->aisegCrop[i].y0,
				param->aisegCrop[i].x1, param->aisegCrop[i].y1,
				param->aisegCrop[i].featureMapSize, param->aisegCrop[i].outputSizeX,
				param->aisegCrop[i].outputSizeY, param->aisegCrop[i].shiftBit,
				&crop_reg);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0004_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				crop_reg.reg_ini_factor_ho_0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0008_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				crop_reg.reg_ini_factor_ho_1);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_000C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				crop_reg.reg_ini_factor_ve_0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0010_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				crop_reg.reg_ini_factor_ve_1);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_001C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_scale_factor_ho_0, 15, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0020_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(REG_RANGE(crop_reg.reg_scale_factor_ho_1, 11, 8) << 12) +
				(REG_RANGE(crop_reg.reg_scale_ho_en, 0, 0) << 8) +
				(REG_RANGE(crop_reg.reg_scale_factor_ho_1, 7, 0)));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0024_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_scale_factor_ve_0, 15, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0028_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(REG_RANGE(crop_reg.reg_scale_factor_ve_1, 11, 8) << 12) +
				(REG_RANGE(crop_reg.reg_scale_ve_en, 0, 0) << 8) +
				(REG_RANGE(crop_reg.reg_scale_factor_ve_1, 7, 0)));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_002C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(REG_RANGE(crop_reg.reg_mode_c_ve, 1, 0) << 8) +
				(REG_RANGE(crop_reg.reg_mode_c_ho, 1, 0)));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0034_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_rsz_s2u, 0, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00A0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(crop_reg.reg_h_size_usr_md << 15) +
				REG_RANGE(crop_reg.reg_h_size, 13, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00A4_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(crop_reg.reg_v_size_usr_md << 15) +
				REG_RANGE(crop_reg.reg_v_size, 13, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00A8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				param->aisegCrop[i].outputSizeX);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00AC_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				param->aisegCrop[i].outputSizeY);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00C0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x80 +
				(REG_RANGE(crop_reg.reg_pre_crop_v_crop_en, 0, 0) << 1) +
				REG_RANGE(crop_reg.reg_pre_crop_h_crop_en, 0, 0));

		MAE_CMDQ_WRITE_REG(pkt, REG_00C8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_pre_crop_h_st, 13, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00CC_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_pre_crop_h_length, 13, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00D0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				crop_reg.reg_pre_crop_hfde_size);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00D4_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_pre_crop_v_st, 13, 0));

		MAE_CMDQ_WRITE_REG(pkt,
				REG_00D8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				REG_RANGE(crop_reg.reg_pre_crop_v_length, 13, 0));

		// reg_post_ins_de_start_trig_md_1[0] = 0, reg_post_ins_boundary_md_1 = 1
		MAE_CMDQ_WRITE_REG(pkt,
				REG_0104_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0002);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0110_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0114_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0118_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_011C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0120_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x0);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0180_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				0x4);

		MAE_CMDQ_WRITE_REG(pkt,
				REG_0184_RSZ1 + i * RSZ_BASE_ADDR_OFFSET,
				(REG_RANGE(crop_reg.reg_postproc_en, 0, 0) << 2) +
				(REG_RANGE(crop_reg.reg_1p_path_en, 0, 0) << 1) +
				1);
	}
}

static bool mtk_mae_config_hw(struct mtk_mae_dev *mae_dev, uint32_t idx)
{
	struct EnqueParam *param =
		(struct EnqueParam*)mae_dev->map_table->param_dmabuf_info[idx].kva;
	uint32_t rsz_offset = 0;
	uint32_t core_offset = 0;
	uint32_t loop = 0;
	uint32_t i = 0;
	uint32_t val;
	struct mae_priv_data *priv_data = g_data->priv_data;

	if (mae_dev->core_sel[idx][loop] < 0 || mae_dev->core_sel[idx][loop] >= FD_PATTERN_NUM)
		return false;

	if (mae_read_back_val) {
		val = (uint32_t)readl(mae_dev->mae_base + MAE_REG_01C0_UDMA_R);
		writel(val | mae_read_back_val, mae_dev->mae_base + MAE_REG_01C0_UDMA_R);
	}

	if (mae_rdma_debug_sel) {
		val = (uint32_t)readl(mae_dev->mae_base + MAE_REG_0178_MAE_DRV_R);
		writel(val | mae_rdma_debug_sel, mae_dev->mae_base + MAE_REG_0178_MAE_DRV_R);
	}

	mae_dev_dbg(mae_dev->dev, "%s+", __func__);
	mae_dev_dbg(mae_dev->dev, "adb: mae_trigger_cmdq_timeout(%d)\n", mae_trigger_cmdq_timeout);
	mae_dev_dbg(mae_dev->dev, "adb: fld_debug_1(%d)\n", fld_debug_1);
	mae_dev_dbg(mae_dev->dev, "adb: mae_dbf_on(%d)\n", mae_dbf_on);
	mae_dev_dbg(mae_dev->dev, "adb: set_default_value(%d)\n", set_default_value);
	mae_dev_dbg(mae_dev->dev, "adb: fld_reset_en(%d)\n", fld_reset_en);
	mae_dev_dbg(mae_dev->dev, "adb: cmdq_polling_en(%d)\n", cmdq_polling_en);
	mae_dev_dbg(mae_dev->dev, "adb: cmdq_profiling_en(%d)\n", cmdq_profiling_en);
	mae_dev_dbg(mae_dev->dev, "adb: mae_preultra_read(%d)\n", mae_preultra_read);
	mae_dev_dbg(mae_dev->dev, "adb: mae_preultra_write(%d)\n", mae_preultra_write);
	mae_dev_dbg(mae_dev->dev, "adb: mae_rdma_debug_sel(%d)\n", mae_rdma_debug_sel);
	mae_dev_dbg(mae_dev->dev, "adb: mae_read_back_val(%d)\n", mae_read_back_val);

	for (loop = 0; loop < mae_dev->outer_loop[idx]; loop++) {
		if (param->image[loop].srcImgFmt == NV12 &&
			param->image[loop].imgHeight % 2 != 0) {
			mae_dev_info(mae_dev->dev, "imgHeight(%d) should be 2 pixel aligned in NV12",
							param->image[loop].imgHeight);
			return false;
		}

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
							MAE_REG_EXTRN_LN_OFFSET_00_R + loop * COMMON_REG_SIZE,
							param->image[loop].imgWidth);

		if (param->image[loop].enRoi) {
			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_SRC_HSIZE_00 + loop * 2 * COMMON_REG_SIZE,
				DIV_CEIL_POS(MIN(param->image[loop].roi.x2, param->image[loop].imgWidth), 16)
				* 16 -
				MIN(MAX(param->image[loop].roi.x1, 0), param->image[loop].imgWidth) / 16 * 16);
			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
				MAE_REG_OUTER_SRC_VSIZE_00 + loop * 2 * COMMON_REG_SIZE,
				DIV_CEIL_POS(MIN(param->image[loop].roi.y2, param->image[loop].imgHeight), 2)
				* 2 -
				(param->image[loop].roi.y1 / 2) * 2);
		} else {
			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
							MAE_REG_OUTER_SRC_HSIZE_00 + loop * 2 * COMMON_REG_SIZE,
							param->image[loop].imgWidth);
			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
							MAE_REG_OUTER_SRC_VSIZE_00 + loop * 2 * COMMON_REG_SIZE,
							param->image[loop].imgHeight);
		}

		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_SYS_SHADOW_CTRL, mae_dev->outer_loop[idx] - 1);

		switch (param->maeMode) {
		case FD_V0:
			switch (mae_dev->core_sel[idx][loop]) {
			case 0:
			case 1:
				rsz_offset = 0;
				core_offset = 0;
				break;
			case 2:
				rsz_offset = 1 * RSZ_BASE_ADDR_OFFSET;
				core_offset = 1 * COMMON_REG_SIZE;
				break;
			case 3:
				rsz_offset = 2 * RSZ_BASE_ADDR_OFFSET;
				core_offset = 2 * COMMON_REG_SIZE;
				break;
			default:
				break;
			}

			if (param->image[loop].srcImgFmt == NV12) {
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_MEM_CONFIG, 0x000C);
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_RESERVE, 0x0007);
			} else {
				mae_dev_info(mae_dev->dev, "wrong img fmt(%d) for fd mode(%d)\n",
					param->image[loop].srcImgFmt, param->maeMode);
				return false;
			}

			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_COEF_ROTATE, param->fdInputDegree);

			mtk_mae_fd_post(mae_dev, param, mae_dev->pkt[idx],
							core_offset, loop, param->maeMode);

			if (!mtk_mae_config_rsz(mae_dev, param, mae_dev->pkt[idx], rsz_offset, loop, idx)) {
				mae_dev_info(mae_dev->dev, "mae mode(%d) config rsz fail\n", param->maeMode);
				// dump param
				return false;
			}

			break;
		case FD_V1_IPN:
			rsz_offset = loop * RSZ_BASE_ADDR_OFFSET;
			core_offset = loop * COMMON_REG_SIZE;

			if (param->image[loop].srcImgFmt == NV12) {
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_MEM_CONFIG, 0x000C);
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_RESERVE, 0x0007);
			} else {
				mae_dev_info(mae_dev->dev, "wrong img fmt(%d) for fd mode(%d)\n",
					param->image[loop].srcImgFmt, param->maeMode);
				return false;
			}

			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_COEF_ROTATE, param->fdInputDegree);

			mtk_mae_fd_post(mae_dev, param, mae_dev->pkt[idx],
							core_offset, loop, param->maeMode);

			if (!mtk_mae_config_rsz(mae_dev, param, mae_dev->pkt[idx], rsz_offset, loop, idx)) {
				mae_dev_info(mae_dev->dev, "mae mode(%d) config rsz fail\n", param->maeMode);
				// dump param
				return false;
			}

			break;
		case FD_V1_FPN:
			rsz_offset = 0;
			core_offset = 0;

			if (param->image[loop].srcImgFmt == NV12) {
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_MEM_CONFIG, 0x000C);
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_RESERVE, 0x0007);
			} else {
				mae_dev_info(mae_dev->dev, "wrong img fmt(%d) for fd mode(%d)\n",
					param->image[loop].srcImgFmt, param->maeMode);
				return false;
			}

			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_COEF_ROTATE, param->fdInputDegree);

			mtk_mae_fd_post(mae_dev, param, mae_dev->pkt[idx], core_offset, loop, param->maeMode);

			if (!mtk_mae_config_rsz(mae_dev, param, mae_dev->pkt[idx], rsz_offset, loop, idx)) {
				mae_dev_info(mae_dev->dev, "mae mode(%d) config rsz fail\n", param->maeMode);
				// dump param
				return false;
			}

			break;
		case ATTR_V0:
		case FAC_V1:
			rsz_offset = 0;
			core_offset = 0;

			if (param->image[loop].srcImgFmt == NV12) {
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_MEM_CONFIG, 0x000C);
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_RESERVE, 0x0007);
			} else {
				mae_dev_info(mae_dev->dev,
					"wrong img fmt(%d) for attr_v0\n", param->image[loop].srcImgFmt);
				return false;
			}

			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_COEF_ROTATE, param->attrInputDegree[idx]);

			mtk_mae_fd_post(mae_dev, param, mae_dev->pkt[idx], core_offset, loop, param->maeMode);

			if (!mtk_mae_config_rsz(mae_dev, param, mae_dev->pkt[idx], rsz_offset, loop, idx)) {
				mae_dev_info(mae_dev->dev, "mae mode(%d) config rsz fail\n", param->maeMode);
				// dump param
				return false;
			}

			break;
		case AISEG:
			if (param->image[loop].srcImgFmt == YUYV) {
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_EXTRN_MEM_CONFIG, 0x0005);
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_RESERVE, 0x0006);
			} else {
				mae_dev_info(mae_dev->dev,
					"wrong img fmt(%d) for attr_v0\n", param->image[loop].srcImgFmt);
				return false;
			}

			if (param->outputNum > AISEG_MAP_NUM) {
				mae_dev_info(mae_dev->dev,
					"Aiseg output is too much (%d/%d)\n",
					param->outputNum, AISEG_MAP_NUM);
				return false;
			}

			for (i = 0; i < param->outputNum; i++)
				MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx],
					MAE_REG_EXTRN_LN_OFFSET_00_W + i * COMMON_REG_SIZE,
					param->lnOffset[i]);

			MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_COEF_ROTATE, param->aisegInputDegree);

			mtk_mae_config_aiseg_crop(mae_dev, param, mae_dev->pkt[idx]);

			mtk_mae_config_aiseg_post(mae_dev, param, mae_dev->pkt[idx]);

			break;
		default:
			break;
		}
	}

	// preultra
	if (mae_preultra_write)
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_0054_UDMA_W, mae_preultra_write);
	else
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_0054_UDMA_W, 0x1428);

	if (mae_preultra_read)
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_0054_UDMA_R, mae_preultra_read);
	else
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_0054_UDMA_R, 0x1428);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_REG_0004_MAE_RDMA_5, 0x6221);
	if (mae_dbf_on == 1)
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_IRQ_DDREN_CMDQ_CTRL, 0x2200);
	else
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_IRQ_DDREN_CMDQ_CTRL, 0x2202);

	/* gce profiling start */
	if (cmdq_profiling_en)
		cmdq_pkt_write_indriect(mae_dev->pkt[idx], NULL,
			mae_dev->mae_time_st_pa, CMDQ_TPR_ID, ~0);

	if (mae_trigger_cmdq_timeout == 0)
		// sw trigger
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_TRIG_RST_CTRL, 0x8000);

	if (cmdq_polling_en) {
		if (priv_data->cmdq_polling_option == 1)
		cmdq_pkt_poll_timeout(mae_dev->pkt[idx], 0x1, SUBSYS_NO_SUPPORT,
				g_data->data->base_address + MAE_IRQ_CTRL1, 0x1, AIE_POLL_TIME_INFINI,
			CMDQ_GPR_R03 + CMDQ_GPR_R03_IDX);
		else if (priv_data->cmdq_polling_option == 2)
		cmdq_pkt_poll_sleep(mae_dev->pkt[idx], MAE_IRQ_STATUS_VALUE,
				g_data->data->base_address + MAE_IRQ_CTRL1, MAE_IRQ_MASK);
	} else {
		cmdq_pkt_wfe(mae_dev->pkt[idx], mae_dev->mae_event_id);
	}

	/* gce profiling end */
	if (cmdq_profiling_en) {
		cmdq_pkt_write_indriect(mae_dev->pkt[idx], NULL, mae_dev->mae_time_ed_pa, CMDQ_TPR_ID, ~0);
		cmdq_pkt_read(mae_dev->pkt[idx], NULL, mae_dev->mae_time_st_pa, CMDQ_THR_SPR_IDX2);
		cmdq_pkt_read(mae_dev->pkt[idx], NULL, mae_dev->mae_time_ed_pa, CMDQ_THR_SPR_IDX3);
	}

	cmdq_pkt_flush_async(mae_dev->pkt[idx], MAECmdqCB, (void *)mae_dev);

	// DEBUG_ONLY
	mae_dev_dbg(mae_dev->dev, "%s-", __func__);

	return true;
}

static void mtk_mae_reg_dump_to_buffer(struct mtk_mae_dev *mae_dev)
{
	uint8_t *debug_buffer = (uint8_t *)mae_dev->map_table->debug_dmabuf_info[0].kva;
	uint32_t i;
	int ret;

	mae_dev_info(mae_dev->dev, "%s +\n", __func__);

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0120_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0120_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0120_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0120_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0124_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0124_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0124_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0124_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0128_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0128_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0128_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0128_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_012C_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_012C_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_012C_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_012C_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0130_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0130_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0130_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0130_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0180_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0180_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0180_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0180_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0184_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0184_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0184_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0184_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01A0_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01A0_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01A0_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01A0_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01A4_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01A4_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01A4_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01A4_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01C0_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C0_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01C0_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C0_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01C4_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C4_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01C4_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C4_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01C8_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C8_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01C8_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01C8_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_01D4_UDMA_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_01D4_UDMA_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_01D4_UDMA_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_01D4_UDMA_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_0178_MAE_DRV_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_0178_MAE_DRV_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_0178_MAE_DRV_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_0178_MAE_DRV_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + MAE_REG_017C_MAE_DRV_R,
			(uint32_t)readl(mae_dev->mae_base + MAE_REG_017C_MAE_DRV_R));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + MAE_REG_017C_MAE_DRV_R,
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_017C_MAE_DRV_R));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + 0x59d0,
			(uint32_t)readl(mae_dev->mae_base + 0x59d0));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + 0x59d0,
				(uint32_t)readl(mae_dev->mae_base + 0x59d0));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x",
			g_data->data->base_address + 0x59d4,
			(uint32_t)readl(mae_dev->mae_base + 0x59d4));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
				"\n[0x%08x] 0x%08x",
				g_data->data->base_address + 0x59d4,
				(uint32_t)readl(mae_dev->mae_base + 0x59d4));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	// 21 line
	for (i = 0; i < FDVT_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + FDVT_START + i,
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + FDVT_START + i,
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + FDVT_START + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 96 line
	for (i = 0; i < MAE_MAISR_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_MAISR_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE+ i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_MAISR_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 48 line
	for (i = 0; i < MAE_CTRL_CENTER_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_CTRL_CENTER_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_CTRL_CENTER_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_CTRL_CENTER_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 31 line
	for (i = 0; i < MAE_RSZ0_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_RSZ0_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_RSZ0_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_RSZ0_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 24 line
	for (i = 0; i < MAE_CMP_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_CMP_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_CMP_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_CMP_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 32 line
	for (i = 0; i < MAE_RDMA_5_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_RDMA_5_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_RDMA_5_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_RDMA_5_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 31 line
	for (i = 0; i < RSZ1_BASE_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + RSZ1_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + RSZ1_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ1_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 31 line
	for (i = 0; i < RSZ2_BASE_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + RSZ2_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + RSZ2_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ2_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 31 line
	for (i = 0; i < RSZ3_BASE_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + RSZ3_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + RSZ3_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + RSZ3_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 1 line
	ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
			"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
			g_data->data->base_address + MAE_MAISR_APB_BASE,
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0x4),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0x8),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0xc));
	if (ret <= 0) {
		mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
		return;
	}
	mae_dev_info(mae_dev->dev,
			"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
			g_data->data->base_address + MAE_MAISR_APB_BASE,
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0x4),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0x8),
			(uint32_t)readl(mae_dev->mae_base + MAE_MAISR_APB_BASE + 0xc));
	debug_buffer += DEBUG_BUFFER_LINE_LEN;

	// 17 line
	for (i = 0; i < MMFD_POST_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MMFD_POST_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MMFD_POST_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MMFD_POST_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 22 line
	for (i = 0; i < MAE_DRV_W_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_DRV_W_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_DRV_W_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_W_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}

	// 31 line
	for (i = 0; i < MAE_DRV_R_LEN; i += 0x10) {
		ret = snprintf(debug_buffer, DEBUG_BUFFER_LINE_LEN,
				"\n[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x",
				g_data->data->base_address + MAE_DRV_R_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0xc));
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mae_dev_info(mae_dev->dev,
				"[0x%08x] 0x%08x 0x%08x 0x%08x 0x%08x\n",
				g_data->data->base_address + MAE_DRV_R_BASE + i,
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0x4),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0x8),
				(uint32_t)readl(mae_dev->mae_base + MAE_DRV_R_BASE + i + 0xc));
		debug_buffer += DEBUG_BUFFER_LINE_LEN;
	}
}

static void mtk_mae_print_iova(struct mtk_mae_dev *mae_dev,
						struct dmabuf_info *buf_info,
						const char *buf_name)
{
	if (buf_info->is_attach)
		mae_dev_info(mae_dev->dev, "Buffer %s IOVA is 0x%llx\n",
			buf_name, buf_info->pa);
}

static void mtk_mae_print_cache(struct mtk_mae_dev *mae_dev,
			struct list_head *list,
			const char *buf_name)
{
	struct dmabuf_info_cache *cache, *tmp;

	list_for_each_entry_safe(cache, tmp, list, list_entry) {
		mae_dev_info(mae_dev->dev, "[%s] fd: %d, pa: 0x%llx\n",
			buf_name, cache->fd, cache->info.pa);
	}
}

static void mtk_mae_dump_buf_iova(struct mtk_mae_dev *mae_dev)
{
	struct mtk_mae_map_table *map_table = mae_dev->map_table;
	uint32_t i = 0;
	uint8_t buffer_name[BUFFER_NAME_LEN];
	int ret;

	for (i = 0; i < MODEL_TYPE_MAX; i++) {
		ret = snprintf(buffer_name, BUFFER_NAME_LEN, "config_%d", i);
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mtk_mae_print_iova(mae_dev, &map_table->config_dmabuf_info[i], buffer_name);

		ret = snprintf(buffer_name, BUFFER_NAME_LEN, "coef_%d", i);
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mtk_mae_print_iova(mae_dev, &map_table->coef_dmabuf_info[i], buffer_name);
	}

	mtk_mae_print_iova(mae_dev, &map_table->image_dmabuf_info[0], "image");

	for (i = 0; i < MAX_PYRAMID_NUM; i++) {
		ret = snprintf(buffer_name, BUFFER_NAME_LEN, "output_%d", i);
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mtk_mae_print_iova(mae_dev, &map_table->output_dmabuf_info[0][i], buffer_name);
	}

	for (i = 0; i < AISEG_MAP_NUM; i++) {
		ret = snprintf(buffer_name, BUFFER_NAME_LEN, "aiseg_output_%d", i);
		if (ret <= 0) {
			mae_dev_info(mae_dev->dev, "[%s] snprintf fail (%d)\n", __func__, ret);
			return;
		}
		mtk_mae_print_iova(mae_dev, &map_table->aiseg_output_dmabuf_info[0][i], buffer_name);
	}

	mtk_mae_print_iova(mae_dev, &map_table->internal_dmabuf_info, "internal");

	for (i = 0; i < AISEG_MAP_NUM; i++)
		mae_dev_info(mae_dev->dev, "aiseg output_%d pa: 0x%llx\n",
			i, map_table->aiseg_output_dmabuf_info[0][i].pa);

	mae_dev_info(mae_dev->dev, "aiseg config pa: 0x%llx\n",
		map_table->config_dmabuf_info[MODEL_TYPE_AISEG].pa);

	mae_dev_info(mae_dev->dev, "aiseg coef pa: 0x%llx\n",
		map_table->coef_dmabuf_info[MODEL_TYPE_AISEG].pa);

	mae_dev_info(mae_dev->dev, "print MAE cache\n");
	mtk_mae_print_cache(mae_dev, &mae_dev->aiseg_config_cache_list, "aiseg_config");
	mtk_mae_print_cache(mae_dev, &mae_dev->aiseg_coef_cache_list, "aiseg_coef");
	mtk_mae_print_cache(mae_dev, &mae_dev->aiseg_output_cache_list, "aiseg_output");
}

static void mtk_mae_dump_param(struct mtk_mae_dev *mae_dev)
{
	struct EnqueParam *param =
		(struct EnqueParam *)mae_dev->map_table->param_dmabuf_info[0].kva;
	uint32_t i;
	struct ModelTable *model_table =
		(struct ModelTable *)mae_dev->map_table->model_table_dmabuf_info.kva;

	mae_dev_info(mae_dev->dev, "Dump user setting, user(%d)\n", param->user);

	if (param->user == USER_FD || param->user == USER_SUPER)
		mae_dev_info(mae_dev->dev, "Max W/H(%d/%d) Sec(%d) FD/FAC Model SEL(%d/%d)\n",
			param->imgMaxWidth, param->imgMaxHeight ,param->isSecure,
			param->FDModelSel, param->FACModelSel);

	mae_dev_info(mae_dev->dev, "mae mode(%d) request number(%d)\n",
		param->maeMode, param->requestNum);

	switch (param->maeMode) {
	case FD_V0:
	case FD_V1_IPN:
	case FD_V1_FPN:
		// core_sel
		mae_dev_info(mae_dev->dev, "pyramid number(%d), fd degree(%d)\n",
			param->pyramidNumber, param->fdInputDegree);
		for (i = 0; i < param->pyramidNumber; i++) {
			mae_dev_info(mae_dev->dev, "fmt(%d), img W/H(%d/%d), roi(%d)(%d,%d->%d,%d), rsz W/H(%d/%d)\n",
				param->image[i].srcImgFmt, param->image[i].imgWidth, param->image[i].imgHeight,
				param->image[i].enRoi, param->image[i].roi.x1, param->image[i].roi.y1,
				param->image[i].roi.x2, param->image[i].roi.y2, param->image[i].resizeWidth,
				param->image[i].resizeHeight);
			mae_dev_info(mae_dev->dev, "pad(%d) (l,r,d,u)=(%d,%d->%d,%d)\n", param->image[i].enPadding,
				param->image[i].padding.left, param->image[i].padding.right,
				param->image[i].padding.down, param->image[i].padding.up);
		}
		break;
	case ATTR_V0:
	case FAC_V1:
		mae_dev_info(mae_dev->dev, "attr face number(%d)\n", param->attrFaceNumber);
		for (i = 0; i < param->attrFaceNumber; i++) {
			mae_dev_info(mae_dev->dev, "attr rotate(%d)\n", param->attrInputDegree[i]);
			mae_dev_info(mae_dev->dev, "fmt(%d), img W/H(%d/%d), roi(%d)(%d,%d->%d,%d), rsz W/H(%d/%d)\n",
				param->image[i].srcImgFmt, param->image[i].imgWidth, param->image[i].imgHeight,
				param->image[i].enRoi, param->image[i].roi.x1, param->image[i].roi.y1,
				param->image[i].roi.x2, param->image[i].roi.y2, param->image[i].resizeWidth,
				param->image[i].resizeHeight);
			mae_dev_info(mae_dev->dev, "pad(%d) (l,r,d,u)=(%d,%d->%d,%d)\n", param->image[i].enPadding,
				param->image[i].padding.left, param->image[i].padding.right,
				param->image[i].padding.down, param->image[i].padding.up);
		}
		break;
	case AISEG:
		mae_dev_info(mae_dev->dev, "aiseg rotate(%d)\n", param->aisegInputDegree);
		mae_dev_info(mae_dev->dev, "fmt(%d), img W/H(%d/%d)\n",
			param->image[0].srcImgFmt, param->image[0].imgWidth, param->image[0].imgHeight);

		for (i = 0; i < AISEG_CROP_NUM; i++) {
			mae_dev_info(mae_dev->dev, "corp[%d] (%d,%d -> %d,%d) featureMapSize(%d) outputSizeX(%d)\n",
					i, param->aisegCrop[i].x0, param->aisegCrop[i].y0,
					param->aisegCrop[i].x1, param->aisegCrop[i].y1,
					param->aisegCrop[i].featureMapSize,
					param->aisegCrop[i].outputSizeX);
			mae_dev_info(mae_dev->dev, "outputSizeY(%d) shiftBit(%d)\n",
					param->aisegCrop[i].outputSizeY,
					param->aisegCrop[i].shiftBit);
		}

		mae_dev_info(mae_dev->dev, "outputNum(%d)\n", param->outputNum);
		for (i = 0; i < param->outputNum; i++) {
			mae_dev_info(mae_dev->dev, "lnOffset[%d] = (%d)\n",
				i, param->lnOffset[i]);
			mae_dev_info(mae_dev->dev, "output fd(%d) size(%ld) offset(%ld)",
				model_table->aisegOutput[i].fd,
				model_table->aisegOutput[i].size,
				model_table->aisegOutput[i].offset);
		}
		break;
	case FLD_V0:
		mae_dev_info(mae_dev->dev, "fmt(%d) fldFaceNum(%d) img W/H(%d/%d)\n",
			param->image[0].srcImgFmt, param->fldConfig.fldFaceNum,
			param->image[0].imgWidth, param->image[0].imgHeight);
		for (i = 0; i < MAX_FLD_V0_FACE_NUM; i++) {
			mae_dev_info(mae_dev->dev, "roi(%d,%d->%d,%d), rip(%d), rop(%d)\n",
				param->fldConfig.fldSetting[i].roi.x1, param->fldConfig.fldSetting[i].roi.y1,
				param->fldConfig.fldSetting[i].roi.x2, param->fldConfig.fldSetting[i].roi.y2,
				param->fldConfig.fldSetting[i].rip, param->fldConfig.fldSetting[i].rop);
		}
		break;
	default:
		mae_dev_info(mae_dev->dev, "No support MAE mode (%d)\n", param->maeMode);
		break;
	}
}

static bool mtk_mae_dump(struct mtk_mae_dev *mae_dev)
{
	struct EnqueParam *param =
		(struct EnqueParam *)mae_dev->map_table->param_dmabuf_info[0].kva;
	uint32_t i, j;
	uint32_t reg_addr;
	uint32_t val;

	mae_dev_info(mae_dev->dev, "%s +\n", __func__);

	// reset driver domain for read back compiler domain reg
	val = (uint32_t)readl(mae_dev->mae_base + 0x6b70);
	writel(val | (0x1 << 3), mae_dev->mae_base + 0x6b70);
	writel(val & ~(0x1 << 3), mae_dev->mae_base + 0x6b70);

	mtk_mae_dump_param(mae_dev);

	mtk_mae_dump_buf_iova(mae_dev);

	mtk_mae_reg_dump_to_buffer(mae_dev);

	mtk_smi_dbg_hang_detect("mae driver");

	mae_dev_info(mae_dev->dev, "Dump reg\n");

	if (param->maeMode == FLD_V0) {
		MAE_DUMP_REG(mae_dev, FLD_IMG_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_MS_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_FP_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_TR_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_SH_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_CV_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_BS_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_PP_BASE_ADDR);
		MAE_DUMP_REG(mae_dev, FLD_FP_FORT_OFST);
		MAE_DUMP_REG(mae_dev, FLD_TR_FORT_OFST);
		MAE_DUMP_REG(mae_dev, FLD_SH_FORT_OFST);
		MAE_DUMP_REG(mae_dev, FLD_CV_FORT_OFST);

		MAE_DUMP_REG(mae_dev, FLD_FACE_0_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_0_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_1_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_1_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_2_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_2_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_3_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_3_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_4_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_4_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_5_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_5_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_6_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_6_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_7_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_7_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_8_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_8_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_9_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_9_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_10_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_10_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_11_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_11_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_12_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_12_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_13_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_13_INFO_1);
		MAE_DUMP_REG(mae_dev, FLD_FACE_14_INFO_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_14_INFO_1);

		MAE_DUMP_REG(mae_dev, FLD_NUM_CONFIG_0);
		MAE_DUMP_REG(mae_dev, FLD_FACE_NUM);

		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_0);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_1);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_2);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_3);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_4);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_5);
		MAE_DUMP_REG(mae_dev, FLD_PCA_MEAN_SCALE_6);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_0);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_1);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_2);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_3);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_4);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_5);
		MAE_DUMP_REG(mae_dev, FLD_PCA_VEC_6);
		MAE_DUMP_REG(mae_dev, FLD_CV_BIAS_FR_0);
		MAE_DUMP_REG(mae_dev, FLD_CV_BIAS_PF_0);
		MAE_DUMP_REG(mae_dev, FLD_CV_RANGE_FR_0);
		MAE_DUMP_REG(mae_dev, FLD_CV_RANGE_FR_1);
		MAE_DUMP_REG(mae_dev, FLD_CV_RANGE_PF_0);
		MAE_DUMP_REG(mae_dev, FLD_CV_RANGE_PF_1);
		MAE_DUMP_REG(mae_dev, FLD_PP_COEF);
		MAE_DUMP_REG(mae_dev, FLD_SRC_SIZE);
		MAE_DUMP_REG(mae_dev, FLD_CMDQ_SRC_SIZE);
		MAE_DUMP_REG(mae_dev, FLD_SRC_PITCH);
		MAE_DUMP_REG(mae_dev, FLD_BS_CONFIG0);
		MAE_DUMP_REG(mae_dev, FLD_BS_CONFIG1);
		MAE_DUMP_REG(mae_dev, FLD_BS_CONFIG2);
	} else {
		mae_dev_info(mae_dev->dev, "Dump MAE_RDMA_5\n");
		MAE_DUMP_REG(mae_dev, MAE_REG_0004_MAE_RDMA_5);

		mae_dev_info(mae_dev->dev, "Dump RSZ1_BASE\n");
		for (i = 0; i < RSZ_NUM; i++) {
			MAE_DUMP_REG(mae_dev, REG_0004_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0008_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_000C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0010_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_001C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0020_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0024_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0028_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_002C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0034_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_005C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0060_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0064_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0068_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0080_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0084_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00A0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00A4_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00A8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00AC_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00C0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00C4_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00C8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00CC_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00D0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00D4_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_00D8_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0104_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0108_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_010C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0110_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0114_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0118_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_011C_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0120_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0180_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
			MAE_DUMP_REG(mae_dev, REG_0184_RSZ1 + i * RSZ_BASE_ADDR_OFFSET);
		}

		MAE_DUMP_REG(mae_dev, MAE_COEF_ROTATE);

		mae_dev_info(mae_dev->dev, "Dump MMFD_POST\n");
		MAE_DUMP_REG(mae_dev, MAE_REG_X_OFFSET_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_X_OFFSET_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_X_OFFSET_2);
		MAE_DUMP_REG(mae_dev, MAE_REG_Y_OFFSET_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_Y_OFFSET_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_Y_OFFSET_2);

		MAE_DUMP_REG(mae_dev, MAE_REG_MMFD_O_SCALE_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_MMFD_O_SCALE_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_MMFD_O_SCALE_2);

		MAE_DUMP_REG(mae_dev, MAE_REG_H_SIZE0);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_SIZE1);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_SIZE2);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_SIZE0);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_SIZE1);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_SIZE2);

		MAE_DUMP_REG(mae_dev, MAE_REG_H_MIN0);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_MIN1);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_MIN2);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MIN0);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MIN1);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MIN2);

		MAE_DUMP_REG(mae_dev, MAE_REG_SCORE_TH0);
		MAE_DUMP_REG(mae_dev, MAE_REG_SCORE_TH1);
		MAE_DUMP_REG(mae_dev, MAE_REG_SCORE_TH2);

		MAE_DUMP_REG(mae_dev, MAE_REG_H_MAX0);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_MAX1);
		MAE_DUMP_REG(mae_dev, MAE_REG_H_MAX2);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MAX0);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MAX1);
		MAE_DUMP_REG(mae_dev, MAE_REG_V_MAX2);

		mae_dev_info(mae_dev->dev, "Dump MAE_DRV\n");
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_00_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_00_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_01_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_01_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_02_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_02_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_03_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_03_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_04_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_04_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_05_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_05_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_06_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_06_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_07_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_07_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_08_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_08_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_09_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_09_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_10_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_10_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_11_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_11_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_12_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_12_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_13_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_13_1_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_14_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_14_1_W);

		MAE_DUMP_REG(mae_dev, MAE_REG_INTRN_BASE_0_W);
		MAE_DUMP_REG(mae_dev, MAE_REG_INTRN_BASE_1_W);

		mae_dev_info(mae_dev->dev, "Dump MAE_DRV_R\n");
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_00_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_00_1_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_01_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_01_1_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_02_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE0_02_1_R);

		MAE_DUMP_REG(mae_dev, MAE_REG_INTRN_BASE_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_INTRN_BASE_1_R);

		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_00_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_00_1_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_01_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_01_1_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_02_0_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_BASE1_02_1_R);

		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_LN_OFFSET_00_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_LN_OFFSET_01_R);
		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_LN_OFFSET_02_R);

		MAE_DUMP_REG(mae_dev, MAE_REG_EXTRN_MEM_CONFIG);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_HSIZE_00);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_VSIZE_00);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_HSIZE_01);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_VSIZE_01);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_HSIZE_02);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_SRC_VSIZE_02);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_00_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_00_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_01_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_01_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_02_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_BASE_02_1);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_00_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_00_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_01_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_01_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_02_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_OFFSET_02_1);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_SIZE_00);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_SIZE_01);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_CONFIG_SIZE_02);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_00_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_00_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_01_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_01_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_02_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_BASE_02_1);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_00_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_00_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_01_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_01_1);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_02_0);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_OFFSET_02_1);

		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_SIZE_00);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_SIZE_01);
		MAE_DUMP_REG(mae_dev, MAE_REG_OUTER_COEF_SIZE_02);

		MAE_DUMP_REG(mae_dev, MAE_REG_RESERVE);

		MAE_DUMP_REG(mae_dev, MAE_IRQ_CTRL1);

	}

	if (param->maeMode == AISEG) {
		mae_dev_info(mae_dev->dev, "Dump AISEG POST\n");

		for (i = 0; i < AISEG_POP_GROUP_SIZE; i++) {
			reg_addr = REG_01A0_RSZ1 + i * RSZ_BASE_ADDR_OFFSET;
			for (j = 0; j < SEMANTIC_MERGE_NUM / 2; j++) {
				MAE_DUMP_REG(mae_dev, reg_addr);
				reg_addr += COMMON_REG_SIZE;
			}

			for (j = 0; j < PERSON_MERGE_NUM / 2; j++) {
				MAE_DUMP_REG(mae_dev, reg_addr);
				reg_addr += COMMON_REG_SIZE;
			}

			for (j = 0; j < AISEG_POP_GROUP_SIZE / 2; j++) {
				MAE_DUMP_REG(mae_dev, reg_addr);
				reg_addr += COMMON_REG_SIZE;
			}
		}
	}

	mae_dev_info(mae_dev->dev, "%s -\n", __func__);
	return true;
}

static bool mtk_mae_irq_handle(struct mtk_mae_dev *mae_dev)
{
	writel(0x1, mae_dev->mae_base + MAE_IRQ_CTRL0);
	writel(0x0, mae_dev->mae_base + MAE_IRQ_CTRL0);

	return true;
}

static bool mtk_mae_config_fld_v0(struct mtk_mae_dev *mae_dev, uint32_t idx)
{
	struct EnqueParam *param =
		(struct EnqueParam*)mae_dev->map_table->param_dmabuf_info[idx].kva;
	uint64_t addr = 0;
	uint8_t i;

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FDVT_ENABLE, 0x4000000);	// [26] ddren set for v0 fld

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_SYS_SHADOW_CTRL, 0x00001040);	// [6] 1: fld done event

	// follow fld pattern setting which is necessary
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], 0x0600, 0x00000010);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FDVT_DMA_CTL, 0x00011111);

	// fill fld base address
	addr = mae_dev->map_table->image_dmabuf_info[idx].pa;
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"fld input image", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_IMG_BASE_ADDR, addr >> 4);
	mae_dev_dbg(mae_dev->dev, "%s(0x%llx)", "fld input image", addr);

	addr = mae_dev->map_table->coef_dmabuf_info[MODEL_TYPE_FLD_FAC_V0].pa +
			round_up(V0_ATTR_128_128_COEF_SIZE, MAE_BASE_ADDR_ALIGN);
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_BS_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_BS_BASE_ADDR, addr >> 4);	// 0x418

	addr += round_up(fdvt_fld_blink_weight_forest14_size, MAE_BASE_ADDR_ALIGN);
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_FP_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_FP_BASE_ADDR, addr >> 4);	// 0x408

	addr += round_up(fdvt_fld_fp_forest00_om45_size, MAE_BASE_ADDR_ALIGN) * 15;
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_SH_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_SH_BASE_ADDR, addr >> 4);	// 0x410

	addr += round_up(fdvt_fld_leafnode_forest00_size, MAE_BASE_ADDR_ALIGN) * 15;
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_CV_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_BASE_ADDR, addr >> 4);	// 0x414

	addr += round_up(fdvt_fld_tree_forest00_cv_weight_size, MAE_BASE_ADDR_ALIGN) * 15;
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_MS_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_MS_BASE_ADDR, addr >> 4);	// 0x404

	addr += round_up(fdvt_fld_tree_forest00_init_shape_size, MAE_BASE_ADDR_ALIGN);
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"FLD_TR_BASE_ADDR", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_TR_BASE_ADDR, addr >> 4);	// 0x40C

	addr = mae_dev->map_table->output_dmabuf_info[idx][0].pa;
	if (CHECK_BASE_ADDR(addr) || addr == 0)
		mae_dev_info(mae_dev->dev, "%s(0x%llx) is not %d-aligned or zero",
									"fld output image", addr, MAE_BASE_ADDR_ALIGN);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PP_BASE_ADDR, addr >> 4);
	mae_dev_dbg(mae_dev->dev, "%s(0x%llx)", "fld output image", addr);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_FP_FORT_OFST,
		round_up(fdvt_fld_fp_forest00_om45_size, MAE_BASE_ADDR_ALIGN) >> 4);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_TR_FORT_OFST,
		round_up(fdvt_fld_tree_forest00_tree_node_size, MAE_BASE_ADDR_ALIGN) >> 4);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_SH_FORT_OFST,
		round_up(fdvt_fld_leafnode_forest00_size, MAE_BASE_ADDR_ALIGN) >> 4);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_FORT_OFST,
		round_up(fdvt_fld_tree_forest00_cv_weight_size, MAE_BASE_ADDR_ALIGN) >> 4);


	for (i = 0; i < param->fldConfig.fldFaceNum; i++) {
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], fld_face_info_idx_0[i],
			(param->fldConfig.fldSetting[i].roi.y1 << 12) |
			param->fldConfig.fldSetting[i].roi.x1);
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], fld_face_info_idx_1[i],
			(param->fldConfig.fldSetting[i].rop << 28) |
			(param->fldConfig.fldSetting[i].rip << 24) |
			(param->fldConfig.fldSetting[i].roi.y2 << 12) |
			param->fldConfig.fldSetting[i].roi.x2);
	}

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_FACE_NUM,
		(FLD_V0_POINT << 8) | param->fldConfig.fldFaceNum);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_SRC_SIZE,
		(param->image[0].imgHeight << 16) | param->image[0].imgWidth);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_SRC_PITCH,
		param->image[0].imgWidth);

	// MAE_TO_DO: maybe write once
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_NUM_CONFIG_0, 0x00b0c80f);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_0, 0x6C004800);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_1, 0x6c007c00);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_2, 0x6c00b800);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_3, 0x6c00ec00);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_4, 0xb0009800);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_5, 0xdc006800);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_MEAN_SCALE_6, 0xdc00cc00);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_0, 0x00fdefd3);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_1, 0x00fef095);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_2, 0x00011095);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_3, 0x00022fd3);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_4, 0x000003e6);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_5, 0x0000dfe9);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PCA_VEC_6, 0x00ff3fe9);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_BIAS_FR_0, 0x00000008);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_BIAS_PF_0, 0x00000003);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_RANGE_FR_0, 0x0000b835);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_RANGE_FR_1, 0xFFFF5cba);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_RANGE_PF_0, 0x00005ed5);
	if (fld_debug_1)
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_RANGE_PF_1, 0xFFFF910d);
	else
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_CV_RANGE_PF_1, 0xFFFF310d);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_PP_COEF, 0xe8242184);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_BS_CONFIG0, 0x00000001);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_BS_CONFIG1, 0x0000031e);
	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FLD_BS_CONFIG2, 0xfffffcae);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_IRQ_DDREN_CMDQ_CTRL, 0x2200);

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FDVT_INT_EN, 0x00000011);

	// sw trigger
	if (mae_trigger_cmdq_timeout == 0) {
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], MAE_TRIG_RST_CTRL, 0x8000);
		MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FDVT_START, 0x00000011);  // [4] AIE_MODE, [0] START

	}

	MAE_CMDQ_WRITE_REG(mae_dev->pkt[idx], FDVT_ENABLE, 0x8000000);	// [27] ddren clr for v0 fld

	if (fld_reset_en)
		mtk_mae_fld_reset(mae_dev);

	cmdq_pkt_wfe(mae_dev->pkt[idx], mae_dev->mae_event_id);
	cmdq_pkt_flush_async(mae_dev->pkt[idx], MAECmdqCB, (void *)mae_dev);

	return true;
}

static bool mtk_mae_get_fd_v0_result(struct mtk_mae_dev *mae_dev, uint32_t idx)
{
	struct EnqueParam *param =
		(struct EnqueParam *)mae_dev->map_table->param_dmabuf_info[idx].kva;
	uint32_t i;

	for (i = 0; i < param->pyramidNumber; i++) {
		switch (mae_dev->core_sel[idx][i]) {
		case 0:
		case 1:
			param->faceNum[i][0] =
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_FACE_NUM0);
			mae_dev_dbg(mae_dev->dev, "%s + face_num(%d)\n", __func__, param->faceNum[i][0]);
			break;
		case 2:
			param->faceNum[i][0] =
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_FACE_NUM1);
			mae_dev_dbg(mae_dev->dev, "%s + face_num(%d)\n", __func__, param->faceNum[i][0]);
			break;
		case 3:
			param->faceNum[i][0] =
				(uint32_t)readl(mae_dev->mae_base + MAE_REG_FACE_NUM2);
			mae_dev_dbg(mae_dev->dev, "%s + face_num(%d)\n", __func__, param->faceNum[i][0]);
			break;
		default:
			mae_dev_info(mae_dev->dev, "[%s] unsupport core_sel(%d)",
						__func__, mae_dev->core_sel[idx][i]);
			break;
		}
	}
	return true;
}

static bool mtk_mae_get_fd_v1_result(struct mtk_mae_dev *mae_dev, uint32_t idx)
{
	struct EnqueParam *param =
		(struct EnqueParam *)mae_dev->map_table->param_dmabuf_info[idx].kva;
	uint32_t i, j;
	uint32_t reg_base = 0;

	for (i = 0; i < param->pyramidNumber; i++) {
		reg_base = MAE_REG_FACE_NUM0 + i * COMMON_REG_SIZE;

		for (j = 0; j < FD_V1_IPN_WDMA_NUM; j++)
			param->faceNum[i][j] =
					(uint32_t)readl(mae_dev->mae_base + reg_base + j * FACE_NUM_REG_OFFSET);
	}

	return true;
}

static void mtk_mae_fld_reset(struct mtk_mae_dev *mae_dev)
{
	uint32_t value;

	value = (uint32_t)readl(mae_dev->mae_base + MAE_TRIG_RST_CTRL);
	writel(value | (0x1 << 12), mae_dev->mae_base + MAE_TRIG_RST_CTRL);

	value = (uint32_t)readl(mae_dev->mae_base + MAE_TRIG_RST_CTRL);
	writel(value & ~(0x1 << 12), mae_dev->mae_base + MAE_TRIG_RST_CTRL);

	value = (uint32_t)readl(mae_dev->mae_base + MAE_TRIG_RST_CTRL);
	writel(value | (0x1 << 13), mae_dev->mae_base + MAE_TRIG_RST_CTRL);

	value = (uint32_t)readl(mae_dev->mae_base + MAE_TRIG_RST_CTRL);
	writel(value & ~(0x1 << 13), mae_dev->mae_base + MAE_TRIG_RST_CTRL);
}

#if MAE_CMDQ_SEC_READY
static void mtk_mae_sec_cmdq_cb(struct cmdq_cb_data data)
{
	struct mtk_mae_dev *mae_dev = (struct mtk_mae_dev *)data.data;

	mae_dev_info(mae_dev->dev, "MAE SEC CMDQ CB\n");
}

static void mtk_mae_sec_pkt_cb(struct cmdq_cb_data data)
{
	struct mtk_mae_dev *mae_dev = (struct mtk_mae_dev *)data.data;

	cmdq_pkt_destroy(mae_dev->sec_pkt);
	mae_dev->sec_pkt = NULL;
}

static bool mtk_mae_secure_cmdq_init(struct mtk_mae_dev *mae_dev)
{
	mae_dev->sec_pkt = cmdq_pkt_create(mae_dev->mae_secure_clt);
	cmdq_sec_pkt_set_data(mae_dev->sec_pkt, 0, 0, CMDQ_SEC_DEBUG, CMDQ_METAEX_TZMP);
	cmdq_sec_pkt_set_mtee(mae_dev->sec_pkt, true);
	cmdq_pkt_finalize_loop(mae_dev->sec_pkt);
	cmdq_pkt_flush_threaded(mae_dev->sec_pkt, mtk_mae_sec_pkt_cb, (void *)mae_dev);

	return true;
}

static bool mtk_mae_enable_secure_domain(struct mtk_mae_dev *mae_dev)
{
	struct cmdq_pkt *pkt = cmdq_pkt_create(mae_dev->mae_clt);

	cmdq_pkt_set_event(pkt, mae_dev->mae_sec_wait);
	cmdq_pkt_wfe(pkt, mae_dev->mae_sec_set);
	cmdq_pkt_flush_async(pkt, mtk_mae_sec_cmdq_cb, (void *)mae_dev);
	cmdq_pkt_wait_complete(pkt);
	cmdq_pkt_destroy(pkt);

	return true;
}

static bool mtk_mae_disable_secure_domain(struct mtk_mae_dev *mae_dev)
{
	struct cmdq_pkt *pkt = cmdq_pkt_create(mae_dev->mae_clt);

	cmdq_pkt_set_event(pkt, mae_dev->mae_sec_wait);
	cmdq_pkt_wfe(pkt, mae_dev->mae_sec_set);
	cmdq_pkt_flush_async(pkt, mtk_mae_sec_cmdq_cb, (void *)mae_dev);
	cmdq_pkt_wait_complete(pkt);
	cmdq_pkt_destroy(pkt);

	return true;
}
#endif

const struct mtk_mae_drv_ops mae_ops_isp8 = {
	.set_dma_address = mtk_mae_config_dma,
	.config_hw = mtk_mae_config_hw,
	.config_fld = mtk_mae_config_fld_v0,
	.get_fd_v0_result = mtk_mae_get_fd_v0_result,
	.get_fd_v1_result = mtk_mae_get_fd_v1_result,
	.irq_handle = mtk_mae_irq_handle,
	.dump_reg = mtk_mae_dump,
#if MAE_CMDQ_SEC_READY
	.secure_init = mtk_mae_secure_cmdq_init,
	.secure_enable = mtk_mae_enable_secure_domain,
	.secure_disable = mtk_mae_disable_secure_domain,
#endif
};

int mtk_mae_isp8_probe(struct platform_device *pdev)
{
	int ret;

	dev_info(&pdev->dev ,"%s +", __func__);

	g_data = of_device_get_match_data(&pdev->dev);
	if (!g_data) {
		dev_info(&pdev->dev, "match data is NULL\n");
		return PTR_ERR(g_data);
	}

	mtk_mae_set_data(g_data);

	ret = devm_clk_bulk_get(&pdev->dev,
			g_data->clk_num,
			g_data->clks);
	if (ret) {
		dev_info(&pdev->dev, "Failed to get clks: %d\n", ret);
		return ret;
	}

#ifdef MAE_TF_DUMP_8
	dev_info(&pdev->dev , "register MAE isp8 tf cb");
	register_mtk_mae_reg_tf_cb(mtk_mae_register_tf_cb_8);
#endif

	dev_info(&pdev->dev ,"%s -", __func__);

	return 0;
}


int mtk_mae_isp8_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev ,"%s +-", __func__);
	return 0;
}

static struct mae_plat_data mae_plat_data_isp8 = {
	.clks = mae_clks_isp8,
	.clk_num = ARRAY_SIZE(mae_clks_isp8),
	.drv_ops = &mae_ops_isp8,
	.data = &mae_data_isp8,
	.priv_data = &priv_data_isp8,
};

static struct mae_plat_data mae_plat_data_isp8_mt6899 = {
	.clks = mae_clks_isp8_m6899,
	.clk_num = ARRAY_SIZE(mae_clks_isp8_m6899),
	.drv_ops = &mae_ops_isp8,
	.data = &mae_data_isp8_mt6899,
	.priv_data = &priv_data_isp8_mt6899,
};

static const struct of_device_id of_match_mtk_mae_isp8_drv[] = {
	{
		.compatible = "mediatek,mtk-mae-plat",
		.data = &mae_plat_data_isp8,
	}, {
		.compatible = "mediatek,mtk-mae-plat-isp8-mt6899",
		.data = &mae_plat_data_isp8_mt6899,
	},{
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, of_match_mtk_mae_isp8_drv);

static struct platform_driver mtk_mae_isp8_drv = {
	.probe = mtk_mae_isp8_probe,
	.remove = mtk_mae_isp8_remove,
	.driver = {
		.name = "mtk-mae-plat",
		.of_match_table = of_match_mtk_mae_isp8_drv,
	},
};

module_platform_driver(mtk_mae_isp8_drv);
MODULE_AUTHOR("Ming-Hsuan Chaing <ming-hsuan.chiang@mediatek.com>");
MODULE_LICENSE("GPL v2");
