// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Fish Wu <fish.wu@mediatek.com>
 *
 */

#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/remoteproc.h>
#include <linux/device.h>
#include <linux/version.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-mem2mem.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-v4l2.h>
#include <linux/suspend.h>
#include <linux/rtc.h>
#include <soc/mediatek/smi.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include "cmdq-sec.h"
#include "mtk_aie.h"
#include "mem/aie_videobuf2-dma-contig.h"
#include "iommu_debug.h"
#include "mtk_notify_aov.h"
#include "mtk_aie-trace.h"

#define FLD_UT 0
#define FLD_SMALL_MODEL 1
#if FLD_SMALL_MODEL
#include "./fldmodel_small/all_header.h"
#if FLD_UT
#include "./fld_pattern_small/fld_pattern_header.h"
#endif	/* #endif FLD_UT */
#else
#include "./fldmodel/all_header.h"
#if FLD_UT
#include "./fld_pattern/fld_pattern_header.h"
#endif	/* #endif FLD_UT */
#endif	/* #endif FLD_SMALL_MODEL */

#define REDUCE_MEM_NETWORK 0
#define AOV_GOLDEN 0
#define AOV_RESEULT 0
#define fd_loop_num_aov 29
#define kernel_RDMA_RA_num 2
#define FLD
#define AIE_QOS_MAX 4
#define AIE_QOS_RA_IDX 0
#define AIE_QOS_RB_IDX 1
#define AIE_QOS_WA_IDX 2
#define AIE_QOS_WB_IDX 3
#define AIE_READ0_AVG_BW 511
#define AIE_READ1_AVG_BW 255
#define AIE_WRITE2_AVG_BW 255
#define AIE_WRITE3_AVG_BW 127
#define CHECK_SERVICE_0 0
#define CHECK_SERVICE_1 1
#define AOV_NOTIFY_AIE_AVAIL 1

#if REDUCE_MEM_NETWORK
#include <aie_mp_fw_reduce/kernel/dma_def.h>
#include <aie_mp_fw_reduce/all_header.h>
#include "./aie_aov_config_reduce/dma_def.h"
#include "./aie_aov_config_reduce/aov_fd_confi_frame01.h"
#include "./aie_aov_config_reduce/aov_rs_confi_frame01.h"
#include "./aie_aov_config_reduce/aov_yuv2rgb_confi_frame01.h"
#else
#include "aie_mp_fw/kernel/dma_def.h"
#include "aie_mp_fw/all_header.h"
#include "./aie_aov_config/dma_def.h"
#include "./aie_aov_config/aov_fd_confi_frame01.h"
#include "./aie_aov_config/aov_rs_confi_frame01.h"
#include "./aie_aov_config/aov_yuv2rgb_confi_frame01.h"
#endif

#if (AOV_GOLDEN || AOV_RESEULT)
#if REDUCE_MEM_NETWORK
#include "aie_golden_reduce/fmap/dma_def.h"
#include "aie_golden_reduce/yuv2rgb/dma_def.h"
#include "aie_golden_reduce/rs/dma_def.h"
#include "aie_golden_reduce/all_header.h"
#else
#include "aie_golden/fmap/dma_def.h"
#include "aie_golden/yuv2rgb/dma_def.h"
#include "aie_golden/rs/dma_def.h"
#include "aie_golden/all_header.h"
#endif
#endif

int aie_log_level_value;
int aie_cg_debug_open_en;
int aie_cg_debug_perframe_en;

module_param(aie_log_level_value, int, 0644);
module_param(aie_cg_debug_open_en, int, 0644);
module_param(aie_cg_debug_perframe_en, int, 0644);

aov_notify m_aov_notify = NULL;
mtk_aie_register_tf_cb m_aie_reg_tf_cb = NULL;

struct mtk_aie_user_para g_user_param;
static struct device *aie_pm_dev;
static struct platform_device *gaov_dev;
static const struct v4l2_pix_format_mplane mtk_aie_img_fmts[] = {
	{
		.pixelformat = V4L2_PIX_FMT_NV16M, .num_planes = 2,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV61M, .num_planes = 2,
	},
	{
		.pixelformat = V4L2_PIX_FMT_YUYV, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_YVYU, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_UYVY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_VYUY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_GREY, .num_planes = 1,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV12M, .num_planes = 2,
	},
	{
		.pixelformat = V4L2_PIX_FMT_NV12, .num_planes = 1,
	},
};

#if REDUCE_MEM_NETWORK
static const unsigned int fd_ker_rdma_size[fd_loop_num_aov][kernel_RDMA_RA_num] = {
	{240, 240},   {1168, 1168}, {1168, 1168}, {272, 272},   {1168, 1168},
	{784, 784}, {272, 272}, {1168, 1168}, {784, 784}, {2320, 2320},
	{1168, 1168}, {1040, 1040}, {272, 272}, {1168, 1168}, {1168, 1168},
	{400, 400}, {1168, 1168}, {1168, 1168}, {2080, 2080}, {1040, 1040},
	{1040, 1040}, {528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080},
	{2080, 2080}, {2080, 2080}, {1040, 1040}, {0, 0}};
#else
static const unsigned int fd_ker_rdma_size[fd_loop_num_aov][kernel_RDMA_RA_num] = {
	{240, 240},   {1168, 1168}, {1168, 1168}, {272, 272},   {2320, 2320},
	{2080, 2080}, {1040, 1040}, {4624, 4624}, {3104, 3104}, {9232, 9232},
	{4624, 4624}, {4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624},
	{1552, 1552}, {4624, 4624}, {4624, 4624}, {4128, 4128}, {1040, 1040},
	{1040, 1040}, {528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080},
	{2080, 2080}, {2080, 2080}, {1040, 1040}, {0, 0}};
#endif

struct aie_data {
	struct clk_bulk_data *clks;
	unsigned int clk_num;
	const struct mtk_aie_drv_ops *drv_ops;
	bool larb_clk_ready;
	struct aie_reg_map *reg_map;
	unsigned int reg_map_num;
	bool is_cmdq_polling;
};

static struct clk_bulk_data ipesys_isp7s_aie_clks[] = {
	{ .id = "VCORE_GALS" },
	{ .id = "IPE_FDVT" },
	{ .id = "IPE_SMI_LARB12" },
	{ .id = "IMG_IPE" },
};

static struct clk_bulk_data isp7sp_aie_clks[] = {
	{ .id = "VCORE_MAIN_CON_0" },
	{ .id = "VCORE_GALS_DISP_CON_0" },
	{ .id = "VCORE_SUB1_CON_0" },
	{ .id = "VCORE_SUB0_CON_0" },
	{ .id = "IMG_IPE" },
	{ .id = "IPE_FDVT" },
	{ .id = "IPE_SMI_LARB12" },
};

static struct clk_bulk_data isp7sp_1_aie_clks[] = {
	{ .id = "VCORE_MAIN" },
	{ .id = "VCORE_GALS_DISP" },
	{ .id = "VCORE_SUB1" },
	{ .id = "VCORE_SUB0" },
	{ .id = "IPE" },
	{ .id = "SUB_COMMON2" },
	{ .id = "SUB_COMMON3" },
	{ .id = "GALS_RX_IPE0" },
	{ .id = "GALS_TX_IPE0" },
	{ .id = "GALS_RX_IPE1" },
	{ .id = "GALS_TX_IPE1" },
	{ .id = "GALS" },
	{ .id = "FDVT" },
	{ .id = "LARB12" },
};

static struct clk_bulk_data isp7sp_2_aie_clks[] = {
	{ .id = "VCORE_MAIN_CON_0" },
	{ .id = "VCORE_SUB0_CON_0" },
	{ .id = "IMG_IPE" },
	{ .id = "SUB_COMMON3" },
	{ .id = "IPE_FDVT" },
	{ .id = "IPE_SMI_LARB12" },
};

static struct aie_reg_map isp7sp_aie_reg_map[] = {
	{ .base = 0x15000000, .offset = 0x1000},  /* isp_main */
	{ .base = 0x15780000, .offset = 0x1000},  /* isp_vcore */
	{ .base = 0x1c001000, .offset = 0x1000},  /* sys_spm */
	{ .base = 0x10000000, .offset = 0x1000},  /* top_ck_gen */
	{ .base = 0x1000c000, .offset = 0x1000},  /* apmixedsys_clk */
};

static struct aie_data data_isp7s = {
	.clks = ipesys_isp7s_aie_clks,
	.clk_num = ARRAY_SIZE(ipesys_isp7s_aie_clks),
	.drv_ops = &aie_ops_isp7s,
	.larb_clk_ready = true,
	.reg_map = NULL,
	.reg_map_num = 0,
	.is_cmdq_polling = false,
};

static struct aie_data data_isp7sp = {
	.clks = isp7sp_aie_clks,
	.clk_num = ARRAY_SIZE(isp7sp_aie_clks),
	.drv_ops = &aie_ops_isp7sp,
	.larb_clk_ready = true,
	.reg_map = isp7sp_aie_reg_map,
	.reg_map_num = ARRAY_SIZE(isp7sp_aie_reg_map),
	.is_cmdq_polling = false,
};

static struct aie_data data_isp7sp_1 = {
	.clks = isp7sp_1_aie_clks,
	.clk_num = ARRAY_SIZE(isp7sp_1_aie_clks),
	.drv_ops = &aie_ops_isp7sp_1,
	.larb_clk_ready = true,
	.reg_map = isp7sp_aie_reg_map,
	.reg_map_num = ARRAY_SIZE(isp7sp_aie_reg_map),
	.is_cmdq_polling = true,
};

static struct aie_data data_isp7sp_2 = {
	.clks = isp7sp_2_aie_clks,
	.clk_num = ARRAY_SIZE(isp7sp_2_aie_clks),
	.drv_ops = &aie_ops_isp7sp,
	.larb_clk_ready = true,
	.reg_map = isp7sp_aie_reg_map,
	.reg_map_num = ARRAY_SIZE(isp7sp_aie_reg_map),
	.is_cmdq_polling = false,
};

void aie_get_time(long long *tv, unsigned int idx)
{
	if (idx < MAX_DEBUG_TIMEVAL)
		tv[idx] = ktime_get_boottime_ns();
	else
		pr_info("[%s] over max debug timeval (%d/%d)\n",
			__func__, idx, MAX_DEBUG_TIMEVAL);
}

void aov_notify_register(aov_notify aov_notify_fn)
{
	m_aov_notify = aov_notify_fn;
}
EXPORT_SYMBOL(aov_notify_register);

void mtk_aie_aov_memcpy(char *buffer)
{
	char *tmp = buffer;

	memcpy(tmp, &aov_yuv2rgb_confi_frame01[0], aov_yuv2rgb_confi_frame01_size);
	tmp += aov_yuv2rgb_confi_frame01_size;
	memcpy(tmp, &aov_fd_confi_frame01[0], aov_fd_confi_frame01_size);
	tmp += aov_fd_confi_frame01_size;

	/*0~10*/
	memcpy(tmp, &fdvt_kernel_bias_loop00_0_frame01[0], fd_ker_rdma_size[0][0]);
	tmp += fd_ker_rdma_size[0][0];
	memcpy(tmp, &fdvt_kernel_bias_loop00_1_frame01[0], fd_ker_rdma_size[0][1]);
	tmp += fd_ker_rdma_size[0][1];

	memcpy(tmp, &fdvt_kernel_bias_loop01_0_frame01[0], fd_ker_rdma_size[1][0]);
	tmp += fd_ker_rdma_size[1][0];
	memcpy(tmp, &fdvt_kernel_bias_loop01_1_frame01[0], fd_ker_rdma_size[1][1]);
	tmp += fd_ker_rdma_size[1][1];

	memcpy(tmp, &fdvt_kernel_bias_loop02_0_frame01[0], fd_ker_rdma_size[2][0]);
	tmp += fd_ker_rdma_size[2][0];
	memcpy(tmp, &fdvt_kernel_bias_loop02_1_frame01[0], fd_ker_rdma_size[2][1]);
	tmp += fd_ker_rdma_size[2][1];

	memcpy(tmp, &fdvt_kernel_bias_loop03_0_frame01[0], fd_ker_rdma_size[3][0]);
	tmp += fd_ker_rdma_size[3][0];
	memcpy(tmp, &fdvt_kernel_bias_loop03_1_frame01[0], fd_ker_rdma_size[3][1]);
	tmp += fd_ker_rdma_size[3][1];

	memcpy(tmp, &fdvt_kernel_bias_loop04_0_frame01[0], fd_ker_rdma_size[4][0]);
	tmp += fd_ker_rdma_size[4][0];
	memcpy(tmp, &fdvt_kernel_bias_loop04_1_frame01[0], fd_ker_rdma_size[4][1]);
	tmp += fd_ker_rdma_size[4][1];

	memcpy(tmp, &fdvt_kernel_bias_loop05_0_frame01[0], fd_ker_rdma_size[5][0]);
	tmp += fd_ker_rdma_size[5][0];
	memcpy(tmp, &fdvt_kernel_bias_loop05_1_frame01[0], fd_ker_rdma_size[5][1]);
	tmp += fd_ker_rdma_size[5][1];

	memcpy(tmp, &fdvt_kernel_bias_loop06_0_frame01[0], fd_ker_rdma_size[6][0]);
	tmp += fd_ker_rdma_size[6][0];
	memcpy(tmp, &fdvt_kernel_bias_loop06_1_frame01[0], fd_ker_rdma_size[6][1]);
	tmp += fd_ker_rdma_size[6][1];

	memcpy(tmp, &fdvt_kernel_bias_loop07_0_frame01[0], fd_ker_rdma_size[7][0]);
	tmp += fd_ker_rdma_size[7][0];
	memcpy(tmp, &fdvt_kernel_bias_loop07_1_frame01[0], fd_ker_rdma_size[7][1]);
	tmp += fd_ker_rdma_size[7][1];
	memcpy(tmp, &fdvt_kernel_bias_loop08_0_frame01[0], fd_ker_rdma_size[8][0]);
	tmp += fd_ker_rdma_size[8][0];
	memcpy(tmp, &fdvt_kernel_bias_loop08_1_frame01[0], fd_ker_rdma_size[8][1]);
	tmp += fd_ker_rdma_size[8][1];
	memcpy(tmp, &fdvt_kernel_bias_loop09_0_frame01[0], fd_ker_rdma_size[9][0]);
	tmp += fd_ker_rdma_size[9][0];
	memcpy(tmp, &fdvt_kernel_bias_loop09_1_frame01[0], fd_ker_rdma_size[9][1]);
	tmp += fd_ker_rdma_size[9][1];

	memcpy(tmp, &fdvt_kernel_bias_loop10_0_frame01[0], fd_ker_rdma_size[10][0]);
	tmp += fd_ker_rdma_size[10][0];
	memcpy(tmp, &fdvt_kernel_bias_loop10_1_frame01[0], fd_ker_rdma_size[10][1]);
	tmp += fd_ker_rdma_size[10][1];
	memcpy(tmp, &fdvt_kernel_bias_loop11_0_frame01[0], fd_ker_rdma_size[11][0]);
	tmp += fd_ker_rdma_size[11][0];
	memcpy(tmp, &fdvt_kernel_bias_loop11_1_frame01[0], fd_ker_rdma_size[11][1]);
	tmp += fd_ker_rdma_size[11][1];
	memcpy(tmp, &fdvt_kernel_bias_loop12_0_frame01[0], fd_ker_rdma_size[12][0]);
	tmp += fd_ker_rdma_size[12][0];
	memcpy(tmp, &fdvt_kernel_bias_loop12_1_frame01[0], fd_ker_rdma_size[12][1]);
	tmp += fd_ker_rdma_size[12][1];
	memcpy(tmp, &fdvt_kernel_bias_loop13_0_frame01[0], fd_ker_rdma_size[13][0]);
	tmp += fd_ker_rdma_size[13][0];
	memcpy(tmp, &fdvt_kernel_bias_loop13_1_frame01[0], fd_ker_rdma_size[13][1]);
	tmp += fd_ker_rdma_size[13][1];

	memcpy(tmp, &fdvt_kernel_bias_loop14_0_frame01[0], fd_ker_rdma_size[14][0]);
	tmp += fd_ker_rdma_size[14][0];
	memcpy(tmp, &fdvt_kernel_bias_loop14_1_frame01[0], fd_ker_rdma_size[14][1]);
	tmp += fd_ker_rdma_size[14][1];
	memcpy(tmp, &fdvt_kernel_bias_loop15_0_frame01[0], fd_ker_rdma_size[15][0]);
	tmp += fd_ker_rdma_size[15][0];
	memcpy(tmp, &fdvt_kernel_bias_loop15_1_frame01[0], fd_ker_rdma_size[15][1]);
	tmp += fd_ker_rdma_size[15][1];
	memcpy(tmp, &fdvt_kernel_bias_loop16_0_frame01[0], fd_ker_rdma_size[16][0]);
	tmp += fd_ker_rdma_size[16][0];
	memcpy(tmp, &fdvt_kernel_bias_loop16_1_frame01[0], fd_ker_rdma_size[16][1]);
	tmp += fd_ker_rdma_size[16][1];
	memcpy(tmp, &fdvt_kernel_bias_loop17_0_frame01[0], fd_ker_rdma_size[17][0]);
	tmp += fd_ker_rdma_size[17][0];
	memcpy(tmp, &fdvt_kernel_bias_loop17_1_frame01[0], fd_ker_rdma_size[17][1]);
	tmp += fd_ker_rdma_size[17][1];
	memcpy(tmp, &fdvt_kernel_bias_loop18_0_frame01[0], fd_ker_rdma_size[18][0]);
	tmp += fd_ker_rdma_size[18][0];
	memcpy(tmp, &fdvt_kernel_bias_loop18_1_frame01[0], fd_ker_rdma_size[18][1]);
	tmp += fd_ker_rdma_size[18][1];
	memcpy(tmp, &fdvt_kernel_bias_loop19_0_frame01[0], fd_ker_rdma_size[19][0]);
	tmp += fd_ker_rdma_size[19][0];
	memcpy(tmp, &fdvt_kernel_bias_loop19_1_frame01[0], fd_ker_rdma_size[19][1]);
	tmp += fd_ker_rdma_size[19][1];
	memcpy(tmp, &fdvt_kernel_bias_loop20_0_frame01[0], fd_ker_rdma_size[20][0]);
	tmp += fd_ker_rdma_size[20][0];
	memcpy(tmp, &fdvt_kernel_bias_loop20_1_frame01[0], fd_ker_rdma_size[20][1]);
	tmp += fd_ker_rdma_size[20][1];
	memcpy(tmp, &fdvt_kernel_bias_loop21_0_frame01[0], fd_ker_rdma_size[21][0]);
	tmp += fd_ker_rdma_size[21][0];
	memcpy(tmp, &fdvt_kernel_bias_loop21_1_frame01[0], fd_ker_rdma_size[21][1]);
	tmp += fd_ker_rdma_size[21][1];
	memcpy(tmp, &fdvt_kernel_bias_loop22_0_frame01[0], fd_ker_rdma_size[22][0]);
	tmp += fd_ker_rdma_size[22][0];
	memcpy(tmp, &fdvt_kernel_bias_loop22_1_frame01[0], fd_ker_rdma_size[22][1]);
	tmp += fd_ker_rdma_size[22][1];
	memcpy(tmp, &fdvt_kernel_bias_loop23_0_frame01[0], fd_ker_rdma_size[23][0]);
	tmp += fd_ker_rdma_size[23][0];
	memcpy(tmp, &fdvt_kernel_bias_loop23_1_frame01[0], fd_ker_rdma_size[23][1]);
	tmp += fd_ker_rdma_size[23][1];
	memcpy(tmp, &fdvt_kernel_bias_loop24_0_frame01[0], fd_ker_rdma_size[24][0]);
	tmp += fd_ker_rdma_size[24][0];
	memcpy(tmp, &fdvt_kernel_bias_loop24_1_frame01[0], fd_ker_rdma_size[24][1]);
	tmp += fd_ker_rdma_size[24][1];
	memcpy(tmp, &fdvt_kernel_bias_loop25_0_frame01[0], fd_ker_rdma_size[25][0]);
	tmp += fd_ker_rdma_size[25][0];
	memcpy(tmp, &fdvt_kernel_bias_loop25_1_frame01[0], fd_ker_rdma_size[25][1]);
	tmp += fd_ker_rdma_size[25][1];
	memcpy(tmp, &fdvt_kernel_bias_loop26_0_frame01[0], fd_ker_rdma_size[26][0]);
	tmp += fd_ker_rdma_size[26][0];
	memcpy(tmp, &fdvt_kernel_bias_loop26_1_frame01[0], fd_ker_rdma_size[26][1]);
	tmp += fd_ker_rdma_size[26][1];
	memcpy(tmp, &fdvt_kernel_bias_loop27_0_frame01[0], fd_ker_rdma_size[27][0]);
	tmp += fd_ker_rdma_size[27][0];
	memcpy(tmp, &fdvt_kernel_bias_loop27_1_frame01[0], fd_ker_rdma_size[27][1]);

#if AOV_RESEULT
	tmp += fd_ker_rdma_size[27][1];
	memcpy(tmp, &fdvt_fdvt_in1_frame01[0], fdvt_fdvt_in1_frame01_size);
	tmp += fdvt_fdvt_in1_frame01_size;
	memcpy(tmp, &fdvt_fdvt_in2_frame01[0], fdvt_fdvt_in2_frame01_size);
	tmp += fdvt_fdvt_in2_frame01_size;
	memcpy(tmp, &fdvt_fd_out_loop28_0[0], fdvt_fd_out_loop28_0_size);
#endif

#if AOV_GOLDEN
	tmp += fd_ker_rdma_size[27][1];
	memcpy(tmp, &fdvt_fdvt_in1_frame01[0], fdvt_fdvt_in1_frame01_size);
	tmp += fdvt_fdvt_in1_frame01_size;
	memcpy(tmp, &fdvt_fdvt_in2_frame01[0], fdvt_fdvt_in2_frame01_size);
	tmp += fdvt_fdvt_in2_frame01_size;

	memcpy(tmp, &fdvt_rs_out_frame01_scale00_r[0], fdvt_rs_out_frame01_scale00_r_size);
	tmp += fdvt_rs_out_frame01_scale00_r_size;
	memcpy(tmp, &fdvt_rs_out_frame01_scale00_g[0], fdvt_rs_out_frame01_scale00_g_size);
	tmp += fdvt_rs_out_frame01_scale00_g_size;
	memcpy(tmp, &fdvt_rs_out_frame01_scale00_b[0], fdvt_rs_out_frame01_scale00_b_size);
	tmp += fdvt_rs_out_frame01_scale00_b_size;


	memcpy(tmp, &fdvt_fd_out_loop00_0[0], fdvt_fd_out_loop00_0_size);
	tmp += fdvt_fd_out_loop00_0_size;

	memcpy(tmp, &fdvt_fd_out_loop01_0[0], fdvt_fd_out_loop01_0_size);
	tmp += fdvt_fd_out_loop01_0_size;

	memcpy(tmp, &fdvt_fd_out_loop01_2[0], fdvt_fd_out_loop01_2_size);
	tmp += fdvt_fd_out_loop01_2_size;
	memcpy(tmp, &fdvt_fd_out_loop02_0[0], fdvt_fd_out_loop02_0_size);
	tmp += fdvt_fd_out_loop02_0_size;

	memcpy(tmp, &fdvt_fd_out_loop02_2[0], fdvt_fd_out_loop02_2_size);
	tmp += fdvt_fd_out_loop02_2_size;
	memcpy(tmp, &fdvt_fd_out_loop03_0[0], fdvt_fd_out_loop03_0_size);
	tmp += fdvt_fd_out_loop03_0_size;
	memcpy(tmp, &fdvt_fd_out_loop04_0[0], fdvt_fd_out_loop04_0_size);
	tmp += fdvt_fd_out_loop04_0_size;
#if (!REDUCE_MEM_NETWORK)
	memcpy(tmp, &fdvt_fd_out_loop04_1[0], fdvt_fd_out_loop04_1_size);
	tmp += fdvt_fd_out_loop04_1_size;
#endif
	memcpy(tmp, &fdvt_fd_out_loop04_2[0], fdvt_fd_out_loop04_2_size);
	tmp += fdvt_fd_out_loop04_2_size;
#if (!REDUCE_MEM_NETWORK)
	memcpy(tmp, &fdvt_fd_out_loop04_3[0], fdvt_fd_out_loop04_3_size);
	tmp += fdvt_fd_out_loop04_3_size;
#endif
	memcpy(tmp, &fdvt_fd_out_loop05_0[0], fdvt_fd_out_loop05_0_size);
	tmp += fdvt_fd_out_loop05_0_size;
	memcpy(tmp, &fdvt_fd_out_loop05_1[0], fdvt_fd_out_loop05_1_size);
	tmp += fdvt_fd_out_loop05_1_size;
	memcpy(tmp, &fdvt_fd_out_loop05_2[0], fdvt_fd_out_loop05_2_size);
	tmp += fdvt_fd_out_loop05_2_size;
	memcpy(tmp, &fdvt_fd_out_loop05_3[0], fdvt_fd_out_loop05_3_size);
	tmp += fdvt_fd_out_loop05_3_size;

	memcpy(tmp, &fdvt_fd_out_loop06_0[0], fdvt_fd_out_loop06_0_size);
	tmp += fdvt_fd_out_loop06_0_size;
	memcpy(tmp, &fdvt_fd_out_loop07_0[0], fdvt_fd_out_loop07_0_size);
	tmp += fdvt_fd_out_loop07_0_size;
	memcpy(tmp, &fdvt_fd_out_loop07_2[0], fdvt_fd_out_loop07_2_size);
	tmp += fdvt_fd_out_loop07_2_size;
	memcpy(tmp, &fdvt_fd_out_loop08_0[0], fdvt_fd_out_loop08_0_size);
	tmp += fdvt_fd_out_loop08_0_size;

	memcpy(tmp, &fdvt_fd_out_loop08_1[0], fdvt_fd_out_loop08_1_size);
	tmp += fdvt_fd_out_loop08_1_size;
	memcpy(tmp, &fdvt_fd_out_loop09_0[0], fdvt_fd_out_loop09_0_size);
	tmp += fdvt_fd_out_loop09_0_size;
	memcpy(tmp, &fdvt_fd_out_loop10_0[0], fdvt_fd_out_loop10_0_size);
	tmp += fdvt_fd_out_loop10_0_size;
	memcpy(tmp, &fdvt_fd_out_loop10_2[0], fdvt_fd_out_loop10_2_size);
	tmp += fdvt_fd_out_loop10_2_size;
	memcpy(tmp, &fdvt_fd_out_loop11_0[0], fdvt_fd_out_loop11_0_size);
	tmp += fdvt_fd_out_loop11_0_size;
	memcpy(tmp, &fdvt_fd_out_loop11_1[0], fdvt_fd_out_loop11_1_size);
	tmp += fdvt_fd_out_loop11_1_size;
	memcpy(tmp, &fdvt_fd_out_loop12_0[0], fdvt_fd_out_loop12_0_size);
	tmp += fdvt_fd_out_loop12_0_size;
	memcpy(tmp, &fdvt_fd_out_loop13_0[0], fdvt_fd_out_loop13_0_size);
	tmp += fdvt_fd_out_loop13_0_size;
	memcpy(tmp, &fdvt_fd_out_loop14_0[0], fdvt_fd_out_loop14_0_size);
	tmp += fdvt_fd_out_loop14_0_size;
	memcpy(tmp, &fdvt_fd_out_loop15_0[0], fdvt_fd_out_loop15_0_size);
	tmp += fdvt_fd_out_loop15_0_size;
	memcpy(tmp, &fdvt_fd_out_loop16_0[0], fdvt_fd_out_loop16_0_size);
	tmp += fdvt_fd_out_loop16_0_size;
	memcpy(tmp, &fdvt_fd_out_loop17_0[0], fdvt_fd_out_loop17_0_size);
	tmp += fdvt_fd_out_loop17_0_size;
	memcpy(tmp, &fdvt_fd_out_loop18_0[0], fdvt_fd_out_loop18_0_size);
	tmp += fdvt_fd_out_loop18_0_size;
	memcpy(tmp, &fdvt_fd_out_loop18_1[0], fdvt_fd_out_loop18_1_size);
	tmp += fdvt_fd_out_loop18_1_size;

	memcpy(tmp, &fdvt_fd_out_loop19_0[0], fdvt_fd_out_loop19_0_size);
	tmp += fdvt_fd_out_loop19_0_size;
	memcpy(tmp, &fdvt_fd_out_loop19_1[0], fdvt_fd_out_loop19_1_size);
	tmp += fdvt_fd_out_loop19_1_size;

	memcpy(tmp, &fdvt_fd_out_loop20_0[0], fdvt_fd_out_loop20_0_size);
	tmp += fdvt_fd_out_loop20_0_size;
	memcpy(tmp, &fdvt_fd_out_loop20_1[0], fdvt_fd_out_loop20_1_size);
	tmp += fdvt_fd_out_loop20_1_size;
	memcpy(tmp, &fdvt_fd_out_loop21_0[0], fdvt_fd_out_loop21_0_size);
	tmp += fdvt_fd_out_loop21_0_size;
	memcpy(tmp, &fdvt_fd_out_loop22_0[0], fdvt_fd_out_loop22_0_size);
	tmp += fdvt_fd_out_loop22_0_size;
	memcpy(tmp, &fdvt_fd_out_loop22_1[0], fdvt_fd_out_loop22_1_size);
	tmp += fdvt_fd_out_loop22_1_size;
	memcpy(tmp, &fdvt_fd_out_loop22_2[0], fdvt_fd_out_loop22_2_size);
	tmp += fdvt_fd_out_loop22_2_size;
	memcpy(tmp, &fdvt_fd_out_loop22_3[0], fdvt_fd_out_loop22_3_size);
	tmp += fdvt_fd_out_loop22_3_size;
	memcpy(tmp, &fdvt_fd_out_loop23_0[0], fdvt_fd_out_loop23_0_size);
	tmp += fdvt_fd_out_loop23_0_size;
	memcpy(tmp, &fdvt_fd_out_loop23_1[0], fdvt_fd_out_loop23_1_size);
	tmp += fdvt_fd_out_loop23_1_size;
	memcpy(tmp, &fdvt_fd_out_loop23_2[0], fdvt_fd_out_loop23_2_size);
	tmp += fdvt_fd_out_loop23_2_size;
	memcpy(tmp, &fdvt_fd_out_loop23_3[0], fdvt_fd_out_loop23_3_size);
	tmp += fdvt_fd_out_loop23_3_size;
	memcpy(tmp, &fdvt_fd_out_loop24_0[0], fdvt_fd_out_loop24_0_size);
	tmp += fdvt_fd_out_loop24_0_size;
	memcpy(tmp, &fdvt_fd_out_loop24_1[0], fdvt_fd_out_loop24_1_size);
	tmp += fdvt_fd_out_loop24_1_size;
	memcpy(tmp, &fdvt_fd_out_loop25_0[0], fdvt_fd_out_loop25_0_size);
	tmp += fdvt_fd_out_loop25_0_size;

	memcpy(tmp, &fdvt_fd_out_loop25_1[0], fdvt_fd_out_loop25_1_size);
	tmp += fdvt_fd_out_loop25_1_size;
	memcpy(tmp, &fdvt_fd_out_loop26_0[0], fdvt_fd_out_loop26_0_size);
	tmp += fdvt_fd_out_loop26_0_size;
	memcpy(tmp, &fdvt_fd_out_loop26_1[0], fdvt_fd_out_loop26_1_size);
	tmp += fdvt_fd_out_loop26_1_size;
	memcpy(tmp, &fdvt_fd_out_loop27_0[0], fdvt_fd_out_loop27_0_size);
	tmp += fdvt_fd_out_loop27_0_size;

	memcpy(tmp, &fdvt_fd_out_loop28_0[0], fdvt_fd_out_loop28_0_size);

#endif
}
EXPORT_SYMBOL(mtk_aie_aov_memcpy);

void mtk_fld_aov_memcpy(char *buffer)
{
	char *tmp = buffer;

	memcpy(tmp, &fdvt_fld_blink_weight_forest14[0],
		fdvt_fld_blink_weight_forest14_size);
	tmp += fdvt_fld_blink_weight_forest14_size;
	memcpy(tmp, &fdvt_fld_tree_forest00_cv_weight[0],
		fdvt_fld_tree_forest00_cv_weight_size);
	tmp += fdvt_fld_tree_forest00_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest01_cv_weight[0],
		fdvt_fld_tree_forest01_cv_weight_size);
	tmp += fdvt_fld_tree_forest01_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest02_cv_weight[0],
		fdvt_fld_tree_forest02_cv_weight_size);
	tmp += fdvt_fld_tree_forest02_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest03_cv_weight[0],
		fdvt_fld_tree_forest03_cv_weight_size);
	tmp += fdvt_fld_tree_forest03_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest04_cv_weight[0],
		fdvt_fld_tree_forest04_cv_weight_size);
	tmp += fdvt_fld_tree_forest04_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest05_cv_weight[0],
		fdvt_fld_tree_forest05_cv_weight_size);
	tmp += fdvt_fld_tree_forest05_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest06_cv_weight[0],
		fdvt_fld_tree_forest06_cv_weight_size);
	tmp += fdvt_fld_tree_forest06_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest07_cv_weight[0],
		fdvt_fld_tree_forest07_cv_weight_size);
	tmp += fdvt_fld_tree_forest07_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest08_cv_weight[0],
		fdvt_fld_tree_forest08_cv_weight_size);
	tmp += fdvt_fld_tree_forest08_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest09_cv_weight[0],
		fdvt_fld_tree_forest09_cv_weight_size);
	tmp += fdvt_fld_tree_forest09_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest10_cv_weight[0],
		fdvt_fld_tree_forest10_cv_weight_size);
	tmp += fdvt_fld_tree_forest10_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest11_cv_weight[0],
		fdvt_fld_tree_forest11_cv_weight_size);
	tmp += fdvt_fld_tree_forest11_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest12_cv_weight[0],
		fdvt_fld_tree_forest12_cv_weight_size);
	tmp += fdvt_fld_tree_forest12_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest13_cv_weight[0],
		fdvt_fld_tree_forest13_cv_weight_size);
	tmp += fdvt_fld_tree_forest13_cv_weight_size;
	memcpy(tmp, &fdvt_fld_tree_forest14_cv_weight[0],
		fdvt_fld_tree_forest14_cv_weight_size);
	tmp += fdvt_fld_tree_forest14_cv_weight_size;

	memcpy(tmp, &fdvt_fld_fp_forest00_om45[0],
		fdvt_fld_fp_forest00_om45_size);
	tmp += fdvt_fld_fp_forest00_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest01_om45[0],
		fdvt_fld_fp_forest01_om45_size);
	tmp += fdvt_fld_fp_forest01_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest02_om45[0],
		fdvt_fld_fp_forest02_om45_size);
	tmp += fdvt_fld_fp_forest02_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest03_om45[0],
		fdvt_fld_fp_forest03_om45_size);
	tmp += fdvt_fld_fp_forest03_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest04_om45[0],
		fdvt_fld_fp_forest04_om45_size);
	tmp += fdvt_fld_fp_forest04_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest05_om45[0],
		fdvt_fld_fp_forest05_om45_size);
	tmp += fdvt_fld_fp_forest05_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest06_om45[0],
		fdvt_fld_fp_forest06_om45_size);
	tmp += fdvt_fld_fp_forest06_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest07_om45[0],
		fdvt_fld_fp_forest07_om45_size);
	tmp += fdvt_fld_fp_forest07_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest08_om45[0],
		fdvt_fld_fp_forest08_om45_size);
	tmp += fdvt_fld_fp_forest08_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest09_om45[0],
		fdvt_fld_fp_forest09_om45_size);
	tmp += fdvt_fld_fp_forest09_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest10_om45[0],
		fdvt_fld_fp_forest10_om45_size);
	tmp += fdvt_fld_fp_forest10_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest11_om45[0],
		fdvt_fld_fp_forest11_om45_size);
	tmp += fdvt_fld_fp_forest11_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest12_om45[0],
		fdvt_fld_fp_forest12_om45_size);
	tmp += fdvt_fld_fp_forest12_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest13_om45[0],
		fdvt_fld_fp_forest13_om45_size);
	tmp += fdvt_fld_fp_forest13_om45_size;
	memcpy(tmp, &fdvt_fld_fp_forest14_om45[0],
		fdvt_fld_fp_forest14_om45_size);
	tmp += fdvt_fld_fp_forest14_om45_size;

	memcpy(tmp, &fdvt_fld_leafnode_forest00[0],
		fdvt_fld_leafnode_forest00_size);
	tmp += fdvt_fld_leafnode_forest00_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest01[0],
		fdvt_fld_leafnode_forest01_size);
	tmp += fdvt_fld_leafnode_forest01_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest02[0],
		fdvt_fld_leafnode_forest02_size);
	tmp += fdvt_fld_leafnode_forest02_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest03[0],
		fdvt_fld_leafnode_forest03_size);
	tmp += fdvt_fld_leafnode_forest03_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest04[0],
		fdvt_fld_leafnode_forest04_size);
	tmp += fdvt_fld_leafnode_forest04_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest05[0],
		fdvt_fld_leafnode_forest05_size);
	tmp += fdvt_fld_leafnode_forest05_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest06[0],
		fdvt_fld_leafnode_forest06_size);
	tmp += fdvt_fld_leafnode_forest06_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest07[0],
		fdvt_fld_leafnode_forest07_size);
	tmp += fdvt_fld_leafnode_forest07_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest08[0],
		fdvt_fld_leafnode_forest08_size);
	tmp += fdvt_fld_leafnode_forest08_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest09[0],
		fdvt_fld_leafnode_forest09_size);
	tmp += fdvt_fld_leafnode_forest09_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest10[0],
		fdvt_fld_leafnode_forest10_size);
	tmp += fdvt_fld_leafnode_forest10_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest11[0],
		fdvt_fld_leafnode_forest11_size);
	tmp += fdvt_fld_leafnode_forest11_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest12[0],
		fdvt_fld_leafnode_forest12_size);
	tmp += fdvt_fld_leafnode_forest12_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest13[0],
		fdvt_fld_leafnode_forest13_size);
	tmp += fdvt_fld_leafnode_forest13_size;
	memcpy(tmp, &fdvt_fld_leafnode_forest14[0],
		fdvt_fld_leafnode_forest14_size);
	tmp += fdvt_fld_leafnode_forest14_size;

	memcpy(tmp, &fdvt_fld_tree_forest00_init_shape[0],
		fdvt_fld_tree_forest00_init_shape_size);
	tmp += fdvt_fld_tree_forest00_init_shape_size;

	memcpy(tmp, &fdvt_fld_tree_forest00_tree_node[0],
		fdvt_fld_tree_forest00_tree_node_size);
	tmp += fdvt_fld_tree_forest00_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest01_tree_node[0],
		fdvt_fld_tree_forest01_tree_node_size);
	tmp += fdvt_fld_tree_forest01_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest02_tree_node[0],
		fdvt_fld_tree_forest02_tree_node_size);
	tmp += fdvt_fld_tree_forest02_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest03_tree_node[0],
		fdvt_fld_tree_forest03_tree_node_size);
	tmp += fdvt_fld_tree_forest03_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest04_tree_node[0],
		fdvt_fld_tree_forest04_tree_node_size);
	tmp += fdvt_fld_tree_forest04_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest05_tree_node[0],
		fdvt_fld_tree_forest05_tree_node_size);
	tmp += fdvt_fld_tree_forest05_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest06_tree_node[0],
		fdvt_fld_tree_forest06_tree_node_size);
	tmp += fdvt_fld_tree_forest06_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest07_tree_node[0],
		fdvt_fld_tree_forest07_tree_node_size);
	tmp += fdvt_fld_tree_forest07_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest08_tree_node[0],
		fdvt_fld_tree_forest08_tree_node_size);
	tmp += fdvt_fld_tree_forest08_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest09_tree_node[0],
		fdvt_fld_tree_forest09_tree_node_size);
	tmp += fdvt_fld_tree_forest09_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest10_tree_node[0],
		fdvt_fld_tree_forest10_tree_node_size);
	tmp += fdvt_fld_tree_forest10_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest11_tree_node[0],
		fdvt_fld_tree_forest11_tree_node_size);
	tmp += fdvt_fld_tree_forest11_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest12_tree_node[0],
		fdvt_fld_tree_forest12_tree_node_size);
	tmp += fdvt_fld_tree_forest12_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest13_tree_node[0],
		fdvt_fld_tree_forest13_tree_node_size);
	tmp += fdvt_fld_tree_forest13_tree_node_size;
	memcpy(tmp, &fdvt_fld_tree_forest14_tree_node[0],
		fdvt_fld_tree_forest14_tree_node_size);

#if FLD_UT
#if FLD_SMALL_MODEL
	tmp += fdvt_fld_tree_forest14_tree_node_size;
	memcpy(tmp, &fdvt_fld_img_frame_00[0], fdvt_fld_img_frame_00_size);
	tmp += fdvt_fld_img_frame_00_size;
	memcpy(tmp, &fdvt_fld_result_frame00[0], fdvt_fld_result_frame00_size);
	tmp += fdvt_fld_result_frame00_size;
	memcpy(tmp, &fdvt_fld_img_frame_01[0], fdvt_fld_img_frame_01_size);
	tmp += fdvt_fld_img_frame_01_size;
	memcpy(tmp, &fdvt_fld_result_frame01[0], fdvt_fld_result_frame01_size);
#else
	tmp += fdvt_fld_tree_forest14_tree_node_size;
	memcpy(tmp, &fdvt_fld_img_frame_02[0], fdvt_fld_img_frame_02_size);
	tmp += fdvt_fld_img_frame_02_size;
	memcpy(tmp, &fdvt_fld_result_frame02[0], fdvt_fld_result_frame02_size);
	tmp += fdvt_fld_result_frame02_size;
	memcpy(tmp, &fdvt_fld_img_frame_07[0], fdvt_fld_img_frame_07_size);
	tmp += fdvt_fld_img_frame_07_size;
	memcpy(tmp, &fdvt_fld_result_frame07[0], fdvt_fld_result_frame07_size);
#endif /* #endif FLD_SMALL_MODEL */
#endif /* #endif FLD_UT */
}
EXPORT_SYMBOL(mtk_fld_aov_memcpy);

void register_mtk_aie_reg_tf_cb(mtk_aie_register_tf_cb mtk_aie_register_tf_cb_fn)
{
	m_aie_reg_tf_cb = mtk_aie_register_tf_cb_fn;
}
EXPORT_SYMBOL(register_mtk_aie_reg_tf_cb);

static int mtk_aie_suspend(struct device *dev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(dev);
	int ret, num;

	if (fd->larb_clk_ready && !fd->is_shutdown)
		if (pm_runtime_suspended(dev))
			return 0;


	num = atomic_read(&fd->num_composing);
	aie_dev_dbg(dev, "%s: suspend aie job start, num(%d)\n", __func__, num);

	ret = wait_event_timeout
		(fd->flushing_waitq,
		 !(num = atomic_read(&fd->num_composing)),
		 msecs_to_jiffies(MTK_FD_HW_TIMEOUT));
	if (!ret && num) {
		aie_dev_info(dev, "%s: flushing aie job timeout, num(%d)\n",
			__func__, num);

		return -EBUSY;
	}

	aie_dev_dbg(dev, "%s: suspend aie job end, num(%d)\n", __func__, num);

	if (fd->larb_clk_ready && !fd->is_shutdown) {
		ret = pm_runtime_put_sync(dev);
		if (ret) {
			aie_dev_info(dev, "%s: pm_runtime_put_sync failed:(%d)\n",
				__func__, ret);
			return ret;
		}
	}

	return 0;
}

static int mtk_aie_resume(struct device *dev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(dev);
	int ret;

	aie_dev_dbg(dev, "%s: resume aie job start)\n", __func__);

	if (fd->larb_clk_ready && !fd->is_shutdown) {
		if (pm_runtime_suspended(dev)) {
			aie_dev_dbg(dev, "%s: pm_runtime_suspended is true, no action\n",
				__func__);
			return 0;
		}
	}

	if (m_aov_notify != NULL)
		m_aov_notify(gaov_dev, AOV_NOTIFY_AIE_AVAIL, 0); //unavailable: 0 available: 1

	if (fd->larb_clk_ready && !fd->is_shutdown) {
		ret = pm_runtime_get_sync(dev);
		if (ret) {
			aie_dev_info(dev, "%s: pm_runtime_get_sync failed:(%d)\n",
				__func__, ret);
			return ret;
		}
	}

	aie_dev_dbg(dev, "%s: resume aie job end)\n", __func__);

	return 0;
}

#if IS_ENABLED(CONFIG_PM)
static int aie_pm_event(struct notifier_block *notifier,
			unsigned long pm_event, void *unused)
{
	struct timespec64 ts;
	struct rtc_time tm;

	ktime_get_ts64(&ts);
	rtc_time64_to_tm(ts.tv_sec, &tm);

	switch (pm_event) {
	case PM_HIBERNATION_PREPARE:
		return NOTIFY_DONE;
	case PM_RESTORE_PREPARE:
		return NOTIFY_DONE;
	case PM_POST_HIBERNATION:
		return NOTIFY_DONE;
	case PM_SUSPEND_PREPARE: /*enter suspend*/
		mtk_aie_suspend(aie_pm_dev);
		return NOTIFY_DONE;
	case PM_POST_SUSPEND:    /*after resume*/
		mtk_aie_resume(aie_pm_dev);
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block aie_notifier_block = {
	.notifier_call = aie_pm_event,
	.priority = 0,
};
#endif

#define NUM_FORMATS ARRAY_SIZE(mtk_aie_img_fmts)

static inline struct mtk_aie_ctx *fh_to_ctx(struct v4l2_fh *fh)
{
	return container_of(fh, struct mtk_aie_ctx, fh);
}
#if CHECK_SERVICE_0
static void mtk_aie_mmdvfs_init(struct mtk_aie_dev *fd)
{
	struct mtk_aie_dvfs *dvfs_info = &fd->dvfs_info;
	u64 freq = 0;
	int ret = 0, opp_num = 0, opp_idx = 0, idx = 0, volt;
	struct device_node *np, *child_np = NULL;
	struct of_phandle_iterator it;

	memset((void *)dvfs_info, 0x0, sizeof(struct mtk_aie_dvfs));
	dvfs_info->dev = fd->dev;
	ret = dev_pm_opp_of_add_table(dvfs_info->dev);
	if (ret < 0) {
		aie_dev_info(dvfs_info->dev, "fail to init opp table: %d\n", ret);
		return;
	}
	dvfs_info->reg = devm_regulator_get(dvfs_info->dev, "dvfsrc-vcore");
	if (IS_ERR(dvfs_info->reg)) {
		aie_dev_info(dvfs_info->dev, "can't get dvfsrc-vcore\n");
		return;
	}

	opp_num = regulator_count_voltages(dvfs_info->reg);
	of_for_each_phandle(
		&it, ret, dvfs_info->dev->of_node, "operating-points-v2", NULL, 0) {
		np = of_node_get(it.node);
		if (!np) {
			aie_dev_info(dvfs_info->dev, "of_node_get fail\n");
			return;
		}

		do {
			child_np = of_get_next_available_child(np, child_np);
			if (child_np) {
				of_property_read_u64(child_np, "opp-hz", &freq);
				dvfs_info->clklv[opp_idx][idx] = freq;
				of_property_read_u32(child_np, "opp-microvolt", &volt);
				dvfs_info->voltlv[opp_idx][idx] = volt;
				idx++;
			}
		} while (child_np);
		dvfs_info->clklv_num[opp_idx] = idx;
		dvfs_info->clklv_target[opp_idx] = dvfs_info->clklv[opp_idx][0];
		dvfs_info->clklv_idx[opp_idx] = 0;
		idx = 0;
		opp_idx++;
		of_node_put(np);
	}

	opp_num = opp_idx;
	for (opp_idx = 0; opp_idx < opp_num; opp_idx++) {
		for (idx = 0; idx < dvfs_info->clklv_num[opp_idx]; idx++) {
			aie_dev_dbg(dvfs_info->dev, "[%s] opp=%d, idx=%d, clk=%d volt=%d\n",
				__func__, opp_idx, idx, dvfs_info->clklv[opp_idx][idx],
				dvfs_info->voltlv[opp_idx][idx]);
		}
	}
	dvfs_info->cur_volt = 0;

}

static void mtk_aie_mmdvfs_uninit(struct mtk_aie_dev *fd)
{
	struct mtk_aie_dvfs *dvfs_info = &fd->dvfs_info;
	int volt = 0, ret = 0;

	dvfs_info->cur_volt = volt;

	ret = regulator_set_voltage(dvfs_info->reg, volt, INT_MAX);

}

static void mtk_aie_mmdvfs_set(struct mtk_aie_dev *fd,
				bool isSet, unsigned int level)
{
	struct mtk_aie_dvfs *dvfs_info = &fd->dvfs_info;
	int volt = 0, ret = 0, idx = 0, opp_idx = 0;

	if (isSet) {
		if (level < dvfs_info->clklv_num[opp_idx])
			idx = level;
	}
	volt = dvfs_info->voltlv[opp_idx][idx];

	if (dvfs_info->cur_volt != volt) {
		aie_dev_dbg(dvfs_info->dev, "[%s] volt change opp=%d, idx=%d, clk=%d volt=%d\n",
			__func__, opp_idx, idx, dvfs_info->clklv[opp_idx][idx],
			dvfs_info->voltlv[opp_idx][idx]);
		ret = regulator_set_voltage(dvfs_info->reg, volt, INT_MAX);
		dvfs_info->cur_volt = volt;
	}
}


static void mtk_aie_mmqos_init(struct mtk_aie_dev *fd)
{
	struct mtk_aie_qos *qos_info = &fd->qos_info;
	//struct icc_path *path;
	int idx = 0;

	memset((void *)qos_info, 0x0, sizeof(struct mtk_aie_qos));
	qos_info->dev = fd->dev;
	qos_info->qos_path = aie_qos_path;

	for (idx = 0; idx < AIE_QOS_MAX; idx++) {

		qos_info->qos_path[idx].path =
			of_mtk_icc_get(qos_info->dev, qos_info->qos_path[idx].dts_name);

		qos_info->qos_path[idx].bw = 0;
		aie_dev_dbg(qos_info->dev, "[%s] idx=%d, path=%p, name=%s, bw=%d\n",
			__func__, idx,
			qos_info->qos_path[idx].path,
			qos_info->qos_path[idx].dts_name,
			qos_info->qos_path[idx].bw);
	}
}

static void mtk_aie_mmqos_uninit(struct mtk_aie_dev *fd)
{
	struct mtk_aie_qos *qos_info = &fd->qos_info;
	int idx = 0;

	for (idx = 0; idx < AIE_QOS_MAX; idx++) {
		if (qos_info->qos_path[idx].path == NULL) {
			aie_dev_dbg(qos_info->dev, "[%s] path of idx(%d) is NULL\n", __func__, idx);
			continue;
		}
		aie_dev_dbg(qos_info->dev, "[%s] idx=%d, path=%p, bw=%d\n",
			__func__, idx,
			qos_info->qos_path[idx].path,
			qos_info->qos_path[idx].bw);
		qos_info->qos_path[idx].bw = 0;

		mtk_icc_set_bw(qos_info->qos_path[idx].path, 0, 0);
	}
}

static void mtk_aie_mmqos_set(struct mtk_aie_dev *fd,
				bool isSet)
{
	struct mtk_aie_qos *qos_info = &fd->qos_info;
	int r0_bw = 0, r1_bw = 0;
	int w2_bw = 0, w3_bw = 0;
	int idx = 0;

	if (isSet) {
		r0_bw = AIE_READ0_AVG_BW;
		r1_bw = AIE_READ1_AVG_BW;
		w2_bw = AIE_WRITE2_AVG_BW;
		w3_bw = AIE_WRITE3_AVG_BW;
	}

	for (idx = 0; idx < AIE_QOS_MAX; idx++) {
		if (qos_info->qos_path[idx].path == NULL) {
			aie_dev_info(qos_info->dev, "[%s] path of idx(%d) is NULL\n",
				 __func__, idx);
			continue;
		}

		if (idx == AIE_QOS_RA_IDX &&
		    qos_info->qos_path[idx].bw != r0_bw) {
			aie_dev_info(qos_info->dev, "[%s] idx=%d, path=%p, bw=%d/%d,\n",
				__func__, idx,
				qos_info->qos_path[idx].path,
				qos_info->qos_path[idx].bw, r0_bw);
			qos_info->qos_path[idx].bw = r0_bw;

			mtk_icc_set_bw(qos_info->qos_path[idx].path,
					MBps_to_icc(qos_info->qos_path[idx].bw), 0);
		}

		if (idx == AIE_QOS_RB_IDX &&
		    qos_info->qos_path[idx].bw != r1_bw) {
			aie_dev_info(qos_info->dev, "[%s] idx=%d, path=%p, bw=%d/%d,\n",
				__func__, idx,
				qos_info->qos_path[idx].path,
				qos_info->qos_path[idx].bw, r1_bw);
			qos_info->qos_path[idx].bw = r1_bw;

			mtk_icc_set_bw(qos_info->qos_path[idx].path,
					MBps_to_icc(qos_info->qos_path[idx].bw), 0);
		}

		if (idx == AIE_QOS_WA_IDX &&
		    qos_info->qos_path[idx].bw != w2_bw) {
			aie_dev_info(qos_info->dev, "[%s] idx=%d, path=%p, bw=%d/%d,\n",
				__func__, idx,
				qos_info->qos_path[idx].path,
				qos_info->qos_path[idx].bw, w2_bw);
			qos_info->qos_path[idx].bw = w2_bw;

			mtk_icc_set_bw(qos_info->qos_path[idx].path,
					MBps_to_icc(qos_info->qos_path[idx].bw), 0);
		}

		if (idx == AIE_QOS_WB_IDX &&
		    qos_info->qos_path[idx].bw != w3_bw) {
			aie_dev_info(qos_info->dev, "[%s] idx=%d, path=%p, bw=%d/%d,\n",
				__func__, idx,
				qos_info->qos_path[idx].path,
				qos_info->qos_path[idx].bw, w3_bw);
			qos_info->qos_path[idx].bw = w3_bw;

			mtk_icc_set_bw(qos_info->qos_path[idx].path,
					MBps_to_icc(qos_info->qos_path[idx].bw), 0);
		}

	}
}


static void mtk_aie_fill_init_param(struct mtk_aie_dev *fd,
				    struct user_init *user_init,
				    struct v4l2_ctrl_handler *hdl)
{
	struct v4l2_ctrl *ctrl;

	ctrl = v4l2_ctrl_find(hdl, V4L2_CID_MTK_AIE_INIT);
	if (ctrl) {
		memcpy(user_init, ctrl->p_new.p_u32, sizeof(struct user_init));
		aie_dev_dbg(fd->dev, "init param : max w:%d, max h:%d",
			user_init->max_img_width, user_init->max_img_height);
		aie_dev_dbg(fd->dev, "init param : p_w:%d, p_h:%d, f thread:%d",
			user_init->pyramid_width, user_init->pyramid_height,
			user_init->feature_thread);
	}
}
#endif

static int mtk_aie_hw_enable(struct mtk_aie_dev *fd)
{
	if (m_aie_reg_tf_cb) {
		aie_dev_info(fd->dev, "AIE register tf callback\n");
		m_aie_reg_tf_cb(fd);
	}

	if(aie_cg_debug_open_en && fd->drv_ops->dump_cg_reg)
		fd->drv_ops->dump_cg_reg(fd);

	return fd->drv_ops->init(fd);
}

static void mtk_aie_hw_job_finish(struct mtk_aie_dev *fd,
				  enum vb2_buffer_state vb_state)
{
	struct mtk_aie_ctx *ctx;
	struct vb2_v4l2_buffer *src_vbuf = NULL, *dst_vbuf = NULL;

	ctx = v4l2_m2m_get_curr_priv(fd->m2m_dev);
	if (ctx == NULL)
		return;

	src_vbuf = v4l2_m2m_src_buf_remove(ctx->fh.m2m_ctx);
	if (src_vbuf == NULL)
		return;

	dst_vbuf = v4l2_m2m_dst_buf_remove(ctx->fh.m2m_ctx);
	if (dst_vbuf == NULL)
		return;

	v4l2_m2m_buf_copy_metadata(src_vbuf, dst_vbuf,
				   V4L2_BUF_FLAG_TSTAMP_SRC_MASK);
	v4l2_m2m_buf_done(src_vbuf, vb_state);
	v4l2_m2m_buf_done(dst_vbuf, vb_state);
	v4l2_m2m_job_finish(fd->m2m_dev, ctx->fh.m2m_ctx);
	complete_all(&fd->fd_job_finished);
}

static void mtk_aie_hw_done(struct mtk_aie_dev *fd,
			    enum vb2_buffer_state vb_state)
{

	aie_get_time(fd->tv, 6);

	mtk_aie_hw_job_finish(fd, vb_state);
	atomic_dec(&fd->num_composing);
	wake_up(&fd->flushing_waitq);
}

static int mtk_aie_ccf_enable(struct device *dev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(dev);
	int ret;

	ret = clk_bulk_prepare_enable(fd->aie_clk.clk_num, fd->aie_clk.clks);
	if (ret) {
		aie_dev_info(fd->dev, "failed to enable AIE clock:%d\n", ret);
		return ret;
	}

	return 0;
}

static int mtk_aie_ccf_disable(struct device *dev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(dev);

	clk_bulk_disable_unprepare(fd->aie_clk.clk_num,
			fd->aie_clk.clks);

	return 0;
}

static int mtk_aie_hw_connect(struct mtk_aie_dev *fd)
{
	int ret = 0;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	if (m_aov_notify != NULL)
		m_aov_notify(gaov_dev, AOV_NOTIFY_AIE_AVAIL, 0); //unavailable: 0 available: 1

	if (fd->larb_clk_ready && !fd->is_shutdown)
		pm_runtime_get_sync((fd->dev));

	fd->fd_stream_count++;
	if (fd->fd_stream_count == 1) {
		if (!fd->is_shutdown) {
			cmdq_mbox_enable(fd->fdvt_clt->chan);
			cmdq_clear_event(fd->fdvt_clt->chan, fd->fdvt_event_id);
		}

		ret = mtk_aie_hw_enable(fd);
		if (ret)
			return -EINVAL;
		//mtk_aie_mmdvfs_set(fd, 1, 0);
		//mtk_aie_mmqos_set(fd, 1);

		fd->map_count = 0;
	}

	AIE_SYSTRACE_END();

	return 0;
}

static void mtk_aie_hw_disconnect(struct mtk_aie_dev *fd)
{
	if (g_user_param.is_secure == 1 &&
		fd->fd_stream_count == 1 &&
		!fd->is_shutdown) {
		aie_disable_secure_domain(fd);
#if CMDQ_SEC_READY
		cmdq_sec_mbox_stop(fd->fdvt_secure_clt);
#endif
	}

	fd->drv_ops->irq_handle(fd);

	if (fd->larb_clk_ready && !fd->is_shutdown)
		pm_runtime_put_sync(fd->dev);

	aie_dev_info(fd->dev, "[%s] stream_count:%d map_count%d\n", __func__,
			fd->fd_stream_count, fd->map_count);
	fd->fd_stream_count--;
	if (fd->fd_stream_count == 0) { //have hw_connect
		//mtk_aie_mmqos_set(fd, 0);
		if (!fd->is_shutdown)
			cmdq_mbox_disable(fd->fdvt_clt->chan);

		//mtk_aie_mmdvfs_set(fd, 0, 0);
		if (fd->map_count == 1) { //have qbuf + map memory
#ifdef AIE_DMA_BUF_UNLOCK_API
			dma_buf_vunmap_unlocked(fd->para_dmabuf, &fd->para_map);
#else
			dma_buf_vunmap(fd->para_dmabuf, &fd->para_map);
#endif
			dma_buf_end_cpu_access(fd->para_dmabuf, DMA_BIDIRECTIONAL);
			dma_buf_put(fd->para_dmabuf);

#ifdef AIE_DMA_BUF_UNLOCK_API
			dma_buf_unmap_attachment_unlocked(fd->config_attach,
				fd->config_sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(fd->config_dmabuf, fd->config_attach);
			dma_buf_vunmap_unlocked(fd->config_dmabuf, &fd->config_map);
#else
			dma_buf_unmap_attachment(fd->config_attach,
				fd->config_sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(fd->config_dmabuf, fd->config_attach);
			dma_buf_vunmap(fd->config_dmabuf, &fd->config_map);
#endif
			dma_buf_put(fd->config_dmabuf);

#ifdef AIE_DMA_BUF_UNLOCK_API
			dma_buf_unmap_attachment_unlocked(fd->model_attach,
				fd->model_sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(fd->model_dmabuf, fd->model_attach);
			dma_buf_vunmap_unlocked(fd->model_dmabuf, &fd->model_map);
#else
			dma_buf_unmap_attachment(fd->model_attach,
				fd->model_sgt, DMA_BIDIRECTIONAL);
			dma_buf_detach(fd->model_dmabuf, fd->model_attach);
			dma_buf_vunmap(fd->model_dmabuf, &fd->model_map);
#endif

			dma_buf_put(fd->model_dmabuf);
			fd->map_count--;
			aie_dev_info(fd->dev, "[%s] stream_count:%d map_count%d\n", __func__,
					fd->fd_stream_count, fd->map_count);
		}
		fd->drv_ops->uninit(fd);
	}
}

static int mtk_aie_hw_job_exec(struct mtk_aie_dev *fd,
			       struct fd_enq_param *fd_param)
{
	reinit_completion(&fd->fd_job_finished);

	return 0;
}

static int mtk_aie_vb2_buf_out_validate(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *v4l2_buf = to_vb2_v4l2_buffer(vb);

	if (v4l2_buf->field == V4L2_FIELD_ANY)
		v4l2_buf->field = V4L2_FIELD_NONE;
	if (v4l2_buf->field != V4L2_FIELD_NONE)
		return -EINVAL;

	return 0;
}

static int mtk_aie_vb2_buf_prepare(struct vb2_buffer *vb)
{
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct vb2_queue *vq = vb->vb2_queue;
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vq);
	struct device *dev = ctx->dev;
	struct v4l2_pix_format_mplane *pixfmt;

	switch (vq->type) {
	case V4L2_BUF_TYPE_META_CAPTURE:
		if (vb2_plane_size(vb, 0) < ctx->dst_fmt.buffersize) {
			aie_dev_info(dev, "meta size %lu is too small\n",
				 vb2_plane_size(vb, 0));
			return -EINVAL;
		}
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		pixfmt = &ctx->src_fmt;

		if (vbuf->field == V4L2_FIELD_ANY)
			vbuf->field = V4L2_FIELD_NONE;

		if (vb->num_planes > 2 || vbuf->field != V4L2_FIELD_NONE) {
			aie_dev_info(dev, "plane %d or field %d not supported\n",
				 vb->num_planes, vbuf->field);
			return -EINVAL;
		}

		if (vb2_plane_size(vb, 0) < pixfmt->plane_fmt[0].sizeimage) {
			aie_dev_info(dev, "plane 0 %lu is too small than %x\n",
				 vb2_plane_size(vb, 0),
				 pixfmt->plane_fmt[0].sizeimage);
			return -EINVAL;
		}

		if (pixfmt->num_planes == 2 &&
		    vb2_plane_size(vb, 1) < pixfmt->plane_fmt[1].sizeimage) {
			aie_dev_info(dev, "plane 1 %lu is too small than %x\n",
				 vb2_plane_size(vb, 1),
				 pixfmt->plane_fmt[1].sizeimage);
			return -EINVAL;
		}
		break;
	}

	return 0;
}

static void mtk_aie_vb2_buf_queue(struct vb2_buffer *vb)
{
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);

	v4l2_m2m_buf_queue(ctx->fh.m2m_ctx, vbuf);
}

static int mtk_aie_vb2_queue_setup(struct vb2_queue *vq,
				   unsigned int *num_buffers,
				   unsigned int *num_planes,
				   unsigned int sizes[],
				   struct device *alloc_devs[])
{
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vq);
	unsigned int size[2];
	unsigned int plane;

	switch (vq->type) {
	case V4L2_BUF_TYPE_META_CAPTURE:
		size[0] = ctx->dst_fmt.buffersize;
		break;
	case V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE:
		size[0] = ctx->src_fmt.plane_fmt[0].sizeimage;
		size[1] = ctx->src_fmt.plane_fmt[1].sizeimage;
		break;
	}

	if (*num_planes > 2)
		return -EINVAL;
	if (*num_planes == 0) {
		if (vq->type == V4L2_BUF_TYPE_META_CAPTURE) {
			sizes[0] = ctx->dst_fmt.buffersize;
			*num_planes = 1;
			return 0;
		}

		*num_planes = ctx->src_fmt.num_planes;
		if (*num_planes > 2)
			return -EINVAL;
		for (plane = 0; plane < *num_planes; plane++)
			sizes[plane] = ctx->src_fmt.plane_fmt[plane].sizeimage;

		return 0;
	}

	return 0;
}

static int mtk_aie_vb2_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vq);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		return mtk_aie_hw_connect(ctx->fd_dev);

	return 0;
}

static int mtk_aie_job_wait_finish(struct mtk_aie_dev *fd)
{
	return wait_for_completion_timeout(&fd->fd_job_finished, msecs_to_jiffies(1000));
}

static void mtk_aie_vb2_stop_streaming(struct vb2_queue *vq)
{
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vq);
	struct mtk_aie_dev *fd = ctx->fd_dev;
	struct vb2_v4l2_buffer *vb;
	struct v4l2_m2m_ctx *m2m_ctx = ctx->fh.m2m_ctx;
	struct v4l2_m2m_queue_ctx *queue_ctx;
	int ret;

	aie_dev_info(fd->dev, "STREAM STOP\n");

	ret = mtk_aie_job_wait_finish(fd);
	if (!ret)
		aie_dev_info(fd->dev, "wait job finish timeout\n");

	queue_ctx = V4L2_TYPE_IS_OUTPUT(vq->type) ? &m2m_ctx->out_q_ctx
						  : &m2m_ctx->cap_q_ctx;
	while ((vb = v4l2_m2m_buf_remove(queue_ctx)))
		v4l2_m2m_buf_done(vb, VB2_BUF_STATE_ERROR);

	if (vq->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)
		mtk_aie_hw_disconnect(fd);
}

static void mtk_aie_vb2_request_complete(struct vb2_buffer *vb)
{
	struct mtk_aie_ctx *ctx = vb2_get_drv_priv(vb->vb2_queue);

	v4l2_ctrl_request_complete(vb->req_obj.req, &ctx->hdl);
}

static int mtk_aie_querycap(struct file *file, void *fh,
			    struct v4l2_capability *cap)
{
	struct mtk_aie_dev *fd = video_drvdata(file);
	struct device *dev = fd->dev;
	int ret = 0;

	strscpy(cap->driver, dev_driver_string(dev), sizeof(cap->driver));
	strscpy(cap->card, dev_driver_string(dev), sizeof(cap->card));
	ret = snprintf(cap->bus_info, sizeof(cap->bus_info), "platform:%s",
		 dev_name(fd->dev));
	if (ret < 0)
		return ret;

	return 0;
}

static int mtk_aie_enum_fmt_out_mp(struct file *file, void *fh,
				   struct v4l2_fmtdesc *f)
{
	if (f->index >= NUM_FORMATS)
		return -EINVAL;

	f->pixelformat = mtk_aie_img_fmts[f->index].pixelformat;
	return 0;
}

static void mtk_aie_fill_pixfmt_mp(struct v4l2_pix_format_mplane *dfmt,
				   const struct v4l2_pix_format_mplane *sfmt)
{
	dfmt->field = V4L2_FIELD_NONE;
	dfmt->colorspace = V4L2_COLORSPACE_BT2020;
	dfmt->num_planes = sfmt->num_planes;
	dfmt->ycbcr_enc = V4L2_YCBCR_ENC_DEFAULT;
	dfmt->quantization = V4L2_QUANTIZATION_DEFAULT;
	dfmt->xfer_func = V4L2_MAP_XFER_FUNC_DEFAULT(dfmt->colorspace);

	/* Keep user setting as possible */
	dfmt->width = clamp(dfmt->width, MTK_FD_OUTPUT_MIN_WIDTH,
			    MTK_FD_OUTPUT_MAX_WIDTH);
	dfmt->height = clamp(dfmt->height, MTK_FD_OUTPUT_MIN_HEIGHT,
			     MTK_FD_OUTPUT_MAX_HEIGHT);

	if (sfmt->num_planes == 2) {
		dfmt->plane_fmt[0].sizeimage =
			dfmt->height * dfmt->plane_fmt[0].bytesperline;
		dfmt->plane_fmt[1].sizeimage =
			dfmt->height * dfmt->plane_fmt[1].bytesperline;
		if (sfmt->pixelformat == V4L2_PIX_FMT_NV12M)
			dfmt->plane_fmt[1].sizeimage =
				dfmt->height * dfmt->plane_fmt[1].bytesperline /
				2;
	} else {
		dfmt->plane_fmt[0].sizeimage =
			dfmt->height * dfmt->plane_fmt[0].bytesperline;
		if (sfmt->pixelformat == V4L2_PIX_FMT_NV12)
			dfmt->plane_fmt[0].sizeimage =
				dfmt->height * dfmt->plane_fmt[0].bytesperline *
				3 / 2;
	}
}

static const struct v4l2_pix_format_mplane *mtk_aie_find_fmt(u32 format)
{
	unsigned int i;

	for (i = 0; i < NUM_FORMATS; i++) {
		if (mtk_aie_img_fmts[i].pixelformat == format)
			return &mtk_aie_img_fmts[i];
	}

	return NULL;
}

static int mtk_aie_try_fmt_out_mp(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	struct v4l2_pix_format_mplane *pix_mp = &f->fmt.pix_mp;
	const struct v4l2_pix_format_mplane *fmt;

	fmt = mtk_aie_find_fmt(pix_mp->pixelformat);
	if (!fmt)
		fmt = &mtk_aie_img_fmts[0]; /* Get default img fmt */

	mtk_aie_fill_pixfmt_mp(pix_mp, fmt);
	return 0;
}

static int mtk_aie_g_fmt_out_mp(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_aie_ctx *ctx;

	ctx = fh_to_ctx(fh);
	if (ctx == NULL)
		return -1;

	f->fmt.pix_mp = ctx->src_fmt;

	return 0;
}

static int mtk_aie_s_fmt_out_mp(struct file *file, void *fh,
				struct v4l2_format *f)
{
	struct mtk_aie_ctx *ctx;
	struct vb2_queue *vq;

	ctx = fh_to_ctx(fh);
	if (ctx == NULL)
		return -1;

	vq = v4l2_m2m_get_vq(ctx->fh.m2m_ctx, f->type);
	if (vq == NULL)
		return -1;

	/* Change not allowed if queue is streaming. */
	if (vb2_is_streaming(vq)) {
		aie_dev_info(ctx->dev, "Failed to set format, vb2 is busy\n");
		return -EBUSY;
	}

	mtk_aie_try_fmt_out_mp(file, fh, f);
	ctx->src_fmt = f->fmt.pix_mp;

	return 0;
}

static int mtk_aie_enum_fmt_meta_cap(struct file *file, void *fh,
				     struct v4l2_fmtdesc *f)
{
	if (f->index)
		return -EINVAL;

	strscpy(f->description, "Face detection result",
		sizeof(f->description));

	f->pixelformat = V4L2_META_FMT_MTFD_RESULT;
	f->flags = 0;

	return 0;
}

static int mtk_aie_g_fmt_meta_cap(struct file *file, void *fh,
				  struct v4l2_format *f)
{
	f->fmt.meta.dataformat = V4L2_META_FMT_MTFD_RESULT;
	f->fmt.meta.buffersize = sizeof(struct aie_enq_info);

	return 0;
}

int mtk_aie_vidioc_qbuf(struct file *file, void *priv,
				  struct v4l2_buffer *buf)
{
	struct mtk_aie_dev *fd;
	int ret = 0, idx = 0;

	if (file == NULL || buf == NULL) {
		pr_info("%s, AIE TF Dump param is NULL\n", __func__);
		return -EFAULT;
	}

	fd = video_drvdata(file);

	AIE_SYSTRACE_BEGIN("%s", __func__);

	if (buf->type == V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) { /*IMG & data*/
		if (!fd->map_count) {

			idx = buf->length - 3;
			if (idx < 0) {
				aie_dev_info(fd->dev, "%s, buffer index out of range(%d, %d)\n",
					__func__, buf->length, idx);
				return -1;
			}
			fd->para_dmabuf = dma_buf_get(buf->m.planes[idx].m.fd);
			if (IS_ERR(fd->para_dmabuf) || fd->para_dmabuf == NULL) {
				aie_dev_info(fd->dev, "%s, dma_buf_getad failed\n", __func__);
				return -ENOMEM;
			}

			ret = dma_buf_begin_cpu_access(fd->para_dmabuf, DMA_BIDIRECTIONAL);
			if (ret < 0) {
				aie_dev_info(fd->dev, "%s, begin_cpu_access failed\n", __func__);
				ret = -ENOMEM;
				goto ERROR_PARA_PUTBUF;
			}

#ifdef AIE_DMA_BUF_UNLOCK_API
			ret = (u64)dma_buf_vmap_unlocked(fd->para_dmabuf, &fd->para_map);
#else
			ret = (u64)dma_buf_vmap(fd->para_dmabuf, &fd->para_map);
#endif
			if (ret) {
				aie_dev_info(fd->dev, "%s, map kernel va failed\n", __func__);
				ret = -ENOMEM;
				goto ERROR_END_CPU_ACCESS;
			}
			fd->para_kva = (unsigned long long)fd->para_map.vaddr;

			memcpy((char *)&g_user_param, (char *)fd->para_kva, sizeof(g_user_param));
			fd->base_para->rpn_anchor_thrd = (signed short)
						(g_user_param.feature_threshold & 0x0000FFFF);
			fd->base_para->max_img_height = (signed short)
						(g_user_param.max_img_height & 0x0000FFFF);
			fd->base_para->max_img_width = (signed short)
						(g_user_param.max_img_width & 0x0000FFFF);
			fd->base_para->pyramid_width = (signed short)
						(g_user_param.pyramid_width & 0x0000FFFF);
			fd->base_para->pyramid_height = (signed short)
						(g_user_param.pyramid_height & 0x0000FFFF);
			fd->base_para->max_pyramid_width = (signed short)
						(g_user_param.pyramid_width & 0x0000FFFF);
			fd->base_para->max_pyramid_height = (signed short)
						(g_user_param.pyramid_height & 0x0000FFFF);

			if (g_user_param.is_secure && !fd->is_shutdown) {
				aie_dev_info(fd->dev, "AIE SECURE MODE INIT!\n");
				config_aie_cmdq_secure_init(fd);
				aie_enable_secure_domain(fd);
			}

			idx = buf->length - 2;
			if (idx < 0) {
				aie_dev_info(fd->dev, "%s, buffer index out of range(%d, %d)\n",
					__func__, buf->length, idx);
				return -1;
			}
			fd->config_dmabuf =
				dma_buf_get(buf->m.planes[idx].m.fd);
			if (IS_ERR(fd->config_dmabuf) ||
				fd->config_dmabuf == NULL) {
				aie_dev_info(fd->dev, "%s, config dmabuf get failed\n",
					__func__);
				ret = -ENOMEM;
				goto ERROR_PARA_UMAP;
			}

#ifdef AIE_DMA_BUF_UNLOCK_API
			ret = (u64)dma_buf_vmap_unlocked(fd->config_dmabuf,
				&fd->config_map);
#else
			ret = (u64)dma_buf_vmap(fd->config_dmabuf,
				&fd->config_map);
#endif
			if (ret) {
				aie_dev_info(fd->dev, "%s, config map va failed\n",
					__func__);
				ret = -ENOMEM;
				goto ERROR_PUT_CONFIG_BUFFER;
			}
			fd->config_kva =
				(unsigned long long)fd->config_map.vaddr;

			fd->config_attach =
				dma_buf_attach(fd->config_dmabuf, fd->smmu_dev);
			if (IS_ERR(fd->config_attach)) {
				aie_dev_info(fd->dev, "config_dmabuf attach fail\n");
				ret = -ENOMEM;
				goto ERROR_CONFIG_UMAP;
			}

#ifdef AIE_DMA_BUF_UNLOCK_API
			fd->config_sgt =
				dma_buf_map_attachment_unlocked(fd->config_attach, DMA_BIDIRECTIONAL);
#else
			fd->config_sgt =
				dma_buf_map_attachment(fd->config_attach, DMA_BIDIRECTIONAL);
#endif
			if (IS_ERR(fd->config_sgt)) {
				aie_dev_info(fd->dev, "config_dmabuf attach fail\n");
				ret = -ENOMEM;
				goto ERROR_CONFIG_DETACH;
			}
			fd->config_pa = sg_dma_address(fd->config_sgt->sgl);

			idx = buf->length - 1;
			if (idx < 0) {
				aie_dev_info(fd->dev, "%s, buffer index out of range(%d, %d)\n",
					__func__, buf->length, idx);
				return -1;
			}
			fd->model_dmabuf =
				dma_buf_get(buf->m.planes[idx].m.fd);
			if (IS_ERR(fd->model_dmabuf) ||
				fd->model_dmabuf == NULL) {
				aie_dev_info(fd->dev, "%s, model dmabuf get failed\n",
					__func__);
				ret = -ENOMEM;
				goto ERROR_CONFIG_UMAP_ATTACHMENT;
			}

#ifdef AIE_DMA_BUF_UNLOCK_API
			ret = (u64)dma_buf_vmap_unlocked(fd->model_dmabuf,
				&fd->model_map);
#else
			ret = (u64)dma_buf_vmap(fd->model_dmabuf,
				&fd->model_map);
#endif
			if (ret) {
				aie_dev_info(fd->dev, "%s, model map va failed\n",
					__func__);
				ret = -ENOMEM;
				goto ERROR_PUT_MODEL_BUFFER;
			}
			fd->model_kva =
				(unsigned long long)fd->model_map.vaddr;

			fd->model_attach =
				dma_buf_attach(fd->model_dmabuf, fd->smmu_dev);
			if (IS_ERR(fd->model_attach)) {
				aie_dev_info(fd->dev, "model_dmabuf attach fail\n");
				ret = -ENOMEM;
				goto ERROR_MODEL_UMAP;
			}

#ifdef AIE_DMA_BUF_UNLOCK_API
			fd->model_sgt =
				dma_buf_map_attachment_unlocked(fd->model_attach, DMA_BIDIRECTIONAL);
#else
			fd->model_sgt =
				dma_buf_map_attachment(fd->model_attach, DMA_BIDIRECTIONAL);
#endif
			if (IS_ERR(fd->model_sgt)) {
				aie_dev_info(fd->dev, "model_dmabuf attach fail\n");
				ret = -ENOMEM;
				goto ERROR_MODEL_DETACH;
			}
			fd->model_pa = sg_dma_address(fd->model_sgt->sgl);

			ret = fd->drv_ops->alloc_buf(fd);
			if (ret)
				return ret;
			fd->map_count++;
		} else {
			memcpy((char *)&g_user_param, (char *)fd->para_kva, sizeof(g_user_param));
		}

	}

	AIE_SYSTRACE_END();

	return v4l2_m2m_ioctl_qbuf(file, priv, buf);

ERROR_MODEL_DETACH:
	dma_buf_detach(fd->model_dmabuf, fd->model_attach);

ERROR_MODEL_UMAP:
#ifdef AIE_DMA_BUF_UNLOCK_API
	dma_buf_vunmap_unlocked(fd->model_dmabuf, &fd->model_map);
#else
	dma_buf_vunmap(fd->model_dmabuf, &fd->model_map);
#endif

ERROR_PUT_MODEL_BUFFER:
		dma_buf_put(fd->model_dmabuf);

ERROR_CONFIG_UMAP_ATTACHMENT:
#ifdef AIE_DMA_BUF_UNLOCK_API
	dma_buf_unmap_attachment_unlocked(fd->config_attach,
		fd->config_sgt, DMA_BIDIRECTIONAL);
#else
	dma_buf_unmap_attachment(fd->config_attach,
		fd->config_sgt, DMA_BIDIRECTIONAL);
#endif

ERROR_CONFIG_DETACH:
	dma_buf_detach(fd->config_dmabuf, fd->config_attach);

ERROR_CONFIG_UMAP:
#ifdef AIE_DMA_BUF_UNLOCK_API
	dma_buf_vunmap_unlocked(fd->config_dmabuf, &fd->config_map);
#else
	dma_buf_vunmap(fd->config_dmabuf, &fd->config_map);
#endif

ERROR_PUT_CONFIG_BUFFER:
	dma_buf_put(fd->config_dmabuf);

ERROR_PARA_UMAP:
#ifdef AIE_DMA_BUF_UNLOCK_API
	dma_buf_vunmap_unlocked(fd->para_dmabuf, &fd->para_map);
#else
	dma_buf_vunmap(fd->para_dmabuf, &fd->para_map);
#endif

ERROR_END_CPU_ACCESS:
	dma_buf_end_cpu_access(fd->para_dmabuf, DMA_BIDIRECTIONAL);

ERROR_PARA_PUTBUF:
	dma_buf_put(fd->para_dmabuf);

	AIE_SYSTRACE_END();

	return ret;
}

static const struct vb2_ops mtk_aie_vb2_ops = {
	.queue_setup = mtk_aie_vb2_queue_setup,
	.buf_out_validate = mtk_aie_vb2_buf_out_validate,
	.buf_prepare = mtk_aie_vb2_buf_prepare,
	.buf_queue = mtk_aie_vb2_buf_queue,
	.start_streaming = mtk_aie_vb2_start_streaming,
	.stop_streaming = mtk_aie_vb2_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
	.buf_request_complete = mtk_aie_vb2_request_complete,
};

static const struct v4l2_ioctl_ops mtk_aie_v4l2_video_out_ioctl_ops = {
	.vidioc_querycap = mtk_aie_querycap,
	.vidioc_enum_fmt_vid_out = mtk_aie_enum_fmt_out_mp,
	.vidioc_g_fmt_vid_out_mplane = mtk_aie_g_fmt_out_mp,
	.vidioc_s_fmt_vid_out_mplane = mtk_aie_s_fmt_out_mp,
	.vidioc_try_fmt_vid_out_mplane = mtk_aie_try_fmt_out_mp,
	.vidioc_enum_fmt_meta_cap = mtk_aie_enum_fmt_meta_cap,
	.vidioc_g_fmt_meta_cap = mtk_aie_g_fmt_meta_cap,
	.vidioc_s_fmt_meta_cap = mtk_aie_g_fmt_meta_cap,
	.vidioc_try_fmt_meta_cap = mtk_aie_g_fmt_meta_cap,
	.vidioc_reqbufs = v4l2_m2m_ioctl_reqbufs,
	.vidioc_create_bufs = v4l2_m2m_ioctl_create_bufs,
	.vidioc_expbuf = v4l2_m2m_ioctl_expbuf,
	.vidioc_prepare_buf = v4l2_m2m_ioctl_prepare_buf,
	.vidioc_querybuf = v4l2_m2m_ioctl_querybuf,
	.vidioc_qbuf = mtk_aie_vidioc_qbuf,
	.vidioc_dqbuf = v4l2_m2m_ioctl_dqbuf,
	.vidioc_streamon = v4l2_m2m_ioctl_streamon,
	.vidioc_streamoff = v4l2_m2m_ioctl_streamoff,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
};

static int mtk_aie_queue_init(void *priv, struct vb2_queue *src_vq,
			      struct vb2_queue *dst_vq)
{
	struct mtk_aie_ctx *ctx = priv;
	int ret;

	src_vq->type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
	src_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	src_vq->supports_requests = true;
	src_vq->drv_priv = ctx;
	src_vq->ops = &mtk_aie_vb2_ops;
	src_vq->mem_ops = &vb2_dma_contig_memops;
	src_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	src_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	src_vq->lock = &ctx->fd_dev->vfd_lock;
	src_vq->dev = ctx->fd_dev->smmu_dev;

	ret = vb2_queue_init(src_vq);
	if (ret)
		return ret;

	dst_vq->type = V4L2_BUF_TYPE_META_CAPTURE;
	dst_vq->io_modes = VB2_MMAP | VB2_DMABUF;
	dst_vq->drv_priv = ctx;
	dst_vq->ops = &mtk_aie_vb2_ops;
	dst_vq->mem_ops = &vb2_dma_contig_memops;
	dst_vq->buf_struct_size = sizeof(struct v4l2_m2m_buffer);
	dst_vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	dst_vq->lock = &ctx->fd_dev->vfd_lock;
	dst_vq->dev = ctx->fd_dev->smmu_dev;

	return vb2_queue_init(dst_vq);
}

static void init_ctx_fmt(struct mtk_aie_ctx *ctx)
{
	struct v4l2_pix_format_mplane *src_fmt = &ctx->src_fmt;
	struct v4l2_meta_format *dst_fmt = &ctx->dst_fmt;

	/* Initialize M2M source fmt */
	src_fmt->width = MTK_FD_OUTPUT_MAX_WIDTH;
	src_fmt->height = MTK_FD_OUTPUT_MAX_HEIGHT;
	mtk_aie_fill_pixfmt_mp(src_fmt, &mtk_aie_img_fmts[0]);

	/* Initialize M2M destination fmt */
	dst_fmt->buffersize = sizeof(struct aie_enq_info);
	dst_fmt->dataformat = V4L2_META_FMT_MTFD_RESULT;
}

/*
 * V4L2 file operations.
 */
static int mtk_vfd_open(struct file *filp)
{
	struct mtk_aie_dev *fd = video_drvdata(filp);
	struct video_device *vdev = video_devdata(filp);
	struct mtk_aie_ctx *ctx;
	int ret;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->fd_dev = fd;
	ctx->dev = fd->dev;
	fd->ctx = ctx;

	v4l2_fh_init(&ctx->fh, vdev);
	filp->private_data = &ctx->fh;

	init_ctx_fmt(ctx);
	ctx->fh.m2m_ctx =
		v4l2_m2m_ctx_init(fd->m2m_dev, ctx, &mtk_aie_queue_init);
	if (IS_ERR(ctx->fh.m2m_ctx)) {
		ret = PTR_ERR(ctx->fh.m2m_ctx);
		goto err_free_ctrl_handler;
	}

	v4l2_fh_add(&ctx->fh);

	return 0;

err_free_ctrl_handler:
	v4l2_fh_exit(&ctx->fh);
	kfree(ctx);

	return ret;
}

static int mtk_vfd_release(struct file *filp)
{
	struct mtk_aie_ctx *ctx =
		container_of(filp->private_data, struct mtk_aie_ctx, fh);

	v4l2_m2m_ctx_release(ctx->fh.m2m_ctx);

	v4l2_fh_del(&ctx->fh);
	v4l2_fh_exit(&ctx->fh);

	kfree(ctx);

	return 0;
}

static __poll_t mtk_vfd_fop_poll(struct file *file, poll_table *wait)
{
	struct mtk_aie_ctx *ctx =
		container_of(file->private_data, struct mtk_aie_ctx, fh);
	int ret;

	ret = mtk_aie_job_wait_finish(ctx->fd_dev);
	if (!ret) {
		aie_dev_info(ctx->dev, "wait job finish timeout\n");
		return EPOLLERR;
	}

	return v4l2_m2m_fop_poll(file, wait);
}

static const struct v4l2_file_operations fd_video_fops = {
	.owner = THIS_MODULE,
	.open = mtk_vfd_open,
	.release = mtk_vfd_release,
	.poll = mtk_vfd_fop_poll,
	.unlocked_ioctl = video_ioctl2,
	.mmap = v4l2_m2m_fop_mmap,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = v4l2_compat_ioctl32,
#endif

};

static void mtk_aie_device_run(void *priv)
{
	struct mtk_aie_ctx *ctx = priv;
	struct mtk_aie_dev *fd = ctx->fd_dev;
	struct vb2_v4l2_buffer *src_buf, *dst_buf;
	struct fd_enq_param fd_param;
	void *plane_vaddr;
	int ret = 0;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	if(aie_cg_debug_perframe_en && fd->drv_ops->dump_cg_reg)
		fd->drv_ops->dump_cg_reg(fd);

	src_buf = v4l2_m2m_next_src_buf(ctx->fh.m2m_ctx);
	if (src_buf == NULL)
		return;

	dst_buf = v4l2_m2m_next_dst_buf(ctx->fh.m2m_ctx);
	if (dst_buf == NULL)
		return;

	fd->img_y = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 0);
	fd->img_msb_y = (fd->img_y & 0Xf00000000) >> 32;
	fd_param.src_img[0].dma_addr = fd->img_y & 0xffffffff;

	if (ctx->src_fmt.num_planes == 2) {
		fd->img_uv = vb2_dma_contig_plane_dma_addr(&src_buf->vb2_buf, 1);
		fd_param.src_img[1].dma_addr = fd->img_uv & 0xffffffff;
	}

	fd->out_fd = vb2_dma_contig_plane_dma_addr(&dst_buf->vb2_buf, 0);

	plane_vaddr = vb2_plane_vaddr(&dst_buf->vb2_buf, 0);
	if (plane_vaddr == NULL) {
		aie_dev_info(fd->dev, "vb2 plane vaddr is null");
		return;
	}

	fd->aie_cfg = (struct aie_enq_info *)plane_vaddr;
	fd->aie_cfg->sel_mode = g_user_param.user_param.fd_mode;
	fd->aie_cfg->src_img_fmt = g_user_param.user_param.src_img_fmt;
	fd->aie_cfg->src_img_width = g_user_param.user_param.src_img_width;
	fd->aie_cfg->src_img_height = g_user_param.user_param.src_img_height;
	fd->aie_cfg->src_img_stride = g_user_param.user_param.src_img_stride;
	fd->aie_cfg->pyramid_base_width = g_user_param.user_param.pyramid_base_width;
	fd->aie_cfg->pyramid_base_height = g_user_param.user_param.pyramid_base_height;
	fd->aie_cfg->number_of_pyramid = g_user_param.user_param.number_of_pyramid;
	fd->aie_cfg->rotate_degree = g_user_param.user_param.rotate_degree;
	fd->aie_cfg->en_roi = g_user_param.user_param.en_roi;
	fd->aie_cfg->src_roi.x1 = g_user_param.user_param.src_roi_x1;
	fd->aie_cfg->src_roi.y1 = g_user_param.user_param.src_roi_y1;
	fd->aie_cfg->src_roi.x2 = g_user_param.user_param.src_roi_x2;
	fd->aie_cfg->src_roi.y2 = g_user_param.user_param.src_roi_y2;
	fd->aie_cfg->en_padding = g_user_param.user_param.en_padding;
	fd->aie_cfg->src_padding.left = g_user_param.user_param.src_padding_left;
	fd->aie_cfg->src_padding.right = g_user_param.user_param.src_padding_right;
	fd->aie_cfg->src_padding.down = g_user_param.user_param.src_padding_down;
	fd->aie_cfg->src_padding.up = g_user_param.user_param.src_padding_up;
	fd->aie_cfg->freq_level = g_user_param.user_param.freq_level;

	aie_dev_dbg(fd->dev, "fd->aie_cfg->sel_mode(%d), fd->aie_cfg->src_img_fmt(%d), "
		"fd->aie_cfg->src_img_width(%d), fd->aie_cfg->src_img_height(%d), "
		"fd->aie_cfg->src_img_stride(%d), fd->aie_cfg->pyramid_base_width(%d), "
		"fd->aie_cfg->pyramid_base_height(%d), fd->aie_cfg->number_of_pyramid(%d), "
		"fd->aie_cfg->rotate_degree(%d), fd->aie_cfg->en_roi(%d),(%d,%d,%d,%d), "
		"fd->aie_cfg->en_padding(%d),(%d,%d,%d,%d),(%d)\n",
		fd->aie_cfg->sel_mode, fd->aie_cfg->src_img_fmt,
		fd->aie_cfg->src_img_width, fd->aie_cfg->src_img_height, fd->aie_cfg->src_img_stride,
		fd->aie_cfg->pyramid_base_width, fd->aie_cfg->pyramid_base_height,
		fd->aie_cfg->number_of_pyramid, fd->aie_cfg->rotate_degree, fd->aie_cfg->en_roi,
		fd->aie_cfg->src_roi.x1, fd->aie_cfg->src_roi.y1, fd->aie_cfg->src_roi.x2, fd->aie_cfg->src_roi.y2,
		fd->aie_cfg->en_padding, fd->aie_cfg->src_padding.left, fd->aie_cfg->src_padding.right,
		fd->aie_cfg->src_padding.down, fd->aie_cfg->src_padding.up, fd->aie_cfg->freq_level);

	if (fd->aie_cfg->sel_mode == FDMODE) {
		fd->dma_para->fd_out_hw_pa[rpn2_loop_num][0]
				= fd->out_fd + offsetof(struct aie_enq_info, fd_out)
					+ offsetof(struct fd_result, rpn31_rlt);
		fd->dma_para->fd_out_hw_pa[rpn1_loop_num][0]
				= fd->out_fd + offsetof(struct aie_enq_info, fd_out)
					+ offsetof(struct fd_result, rpn63_rlt);
		fd->dma_para->fd_out_hw_pa[rpn0_loop_num][0]
				= fd->out_fd + offsetof(struct aie_enq_info, fd_out)
					+ offsetof(struct fd_result, rpn95_rlt);
	} else if (fd->aie_cfg->sel_mode == ATTRIBUTEMODE) {
		fd->dma_para->age_out_hw_pa[fd->attr_para->r_idx]
				= fd->out_fd + offsetof(struct aie_enq_info, attr_out)
					+ offsetof(struct attr_result, rpn17_rlt);
		fd->dma_para->gender_out_hw_pa[fd->attr_para->r_idx]
				= fd->out_fd + offsetof(struct aie_enq_info, attr_out)
					+ offsetof(struct attr_result, rpn20_rlt);
		fd->dma_para->isIndian_out_hw_pa[fd->attr_para->r_idx]
				= fd->out_fd + offsetof(struct aie_enq_info, attr_out)
					+ offsetof(struct attr_result, rpn22_rlt);
		fd->dma_para->race_out_hw_pa[fd->attr_para->r_idx]
				= fd->out_fd + offsetof(struct aie_enq_info, attr_out)
					+ offsetof(struct attr_result, rpn25_rlt);
	} else if (fd->aie_cfg->sel_mode == FLDMODE) {
		fd->dma_para->fld_output_pa
				= fd->out_fd + offsetof(struct aie_enq_info, fld_raw_out);
	}

	if (fd->aie_cfg->sel_mode == FLDMODE) {
		fd->aie_cfg->fld_face_num = g_user_param.user_param.fld_face_num;
		memcpy(fd->aie_cfg->fld_input, g_user_param.user_param.fld_input,
		sizeof(struct FLD_CROP_RIP_ROP)*g_user_param.user_param.fld_face_num);
	} else if (fd->aie_cfg->src_img_fmt == FMT_YUV420_1P) {
		/*FLD Just have Y*/
		fd_param.src_img[1].dma_addr = fd_param.src_img[0].dma_addr +
			g_user_param.user_param.src_img_stride *
			g_user_param.user_param.src_img_height;
		fd->img_msb_uv = ((fd->img_y +
			g_user_param.user_param.src_img_stride *
			g_user_param.user_param.src_img_height) &
			0Xf00000000) >> 32;
	}

	fd->aie_cfg->src_img_addr = fd_param.src_img[0].dma_addr;
	fd->aie_cfg->src_img_addr_uv = fd_param.src_img[1].dma_addr;

	if (fd->aie_cfg->sel_mode == FLDMODE) {
		fd->drv_ops->config_fld_buf_reg(fd);

		fd->fld_para->sel_mode = fd->aie_cfg->sel_mode;
		fd->fld_para->img_height = fd->aie_cfg->src_img_height;
		fd->fld_para->img_width = fd->aie_cfg->src_img_width;
		fd->fld_para->face_num = fd->aie_cfg->fld_face_num;
		fd->fld_para->src_img_addr = fd->aie_cfg->src_img_addr;
		memcpy(fd->fld_para->fld_input, fd->aie_cfg->fld_input,
			sizeof(struct FLD_CROP_RIP_ROP) * fd->aie_cfg->fld_face_num);
	} else
		ret = fd->drv_ops->prepare(fd, fd->aie_cfg);

	/* mmdvfs */
	//mtk_aie_mmdvfs_set(fd, 1, fd->aie_cfg->freq_level);

	atomic_inc(&fd->num_composing);

	if (!fd->is_shutdown)
		mtk_aie_hw_job_exec(fd, &fd_param);

	aie_get_time(fd->tv, 0);

	if (ret) {
		aie_dev_info(fd->dev, "Failed to prepare aie setting\n");
		return;
	}
	if (!fd->is_shutdown)
		fd->drv_ops->execute(fd, fd->aie_cfg);

	AIE_SYSTRACE_END();
}

static struct v4l2_m2m_ops fd_m2m_ops = {
	.device_run = mtk_aie_device_run,
};

static const struct media_device_ops fd_m2m_media_ops = {
	.req_validate = vb2_request_validate,
	.req_queue = v4l2_m2m_request_queue,
};

static int mtk_aie_video_device_register(struct mtk_aie_dev *fd)
{
	struct video_device *vfd = &fd->vfd;
	struct v4l2_m2m_dev *m2m_dev = fd->m2m_dev;
	struct device *dev = fd->dev;
	int ret;

	vfd->fops = &fd_video_fops;
	vfd->release = video_device_release;
	vfd->lock = &fd->vfd_lock;
	vfd->v4l2_dev = &fd->v4l2_dev;
	vfd->vfl_dir = VFL_DIR_M2M;
	vfd->device_caps = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE |
			   V4L2_CAP_META_CAPTURE;
	vfd->ioctl_ops = &mtk_aie_v4l2_video_out_ioctl_ops;

	strscpy(vfd->name, dev_driver_string(dev), sizeof(vfd->name));

	video_set_drvdata(vfd, fd);

	ret = video_register_device(vfd, VFL_TYPE_VIDEO, 0);
	if (ret) {
		aie_dev_info(dev, "Failed to register video device\n");
		return ret;
	}

	ret = v4l2_m2m_register_media_controller(
		m2m_dev, vfd, MEDIA_ENT_F_PROC_VIDEO_STATISTICS);
	if (ret) {
		aie_dev_info(dev, "Failed to init mem2mem media controller\n");
		goto err_unreg_video;
	}
	return 0;

err_unreg_video:
	video_unregister_device(vfd);
	return ret;
}

static int mtk_aie_dev_larb_init(struct mtk_aie_dev *fd)
{
	struct device_node *node;
	struct platform_device *pdev;
	struct device_link *link;

	node = of_parse_phandle(fd->dev->of_node, "mediatek,larb", 0);
	if (!node)
		return -EINVAL;
	pdev = of_find_device_by_node(node);
	if (WARN_ON(!pdev)) {
		of_node_put(node);
		return -EINVAL;
	}
	of_node_put(node);

	fd->larb = &pdev->dev;

	link = device_link_add(fd->dev, &pdev->dev,
					DL_FLAG_PM_RUNTIME | DL_FLAG_STATELESS);
	if (!link) {
		aie_dev_info(fd->dev, "unable to link SMI LARB idx\n");
		return -EINVAL;
	}

	return 0;
}

static int mtk_aie_dev_v4l2_init(struct mtk_aie_dev *fd)
{
	struct media_device *mdev = &fd->mdev;
	struct device *dev = fd->dev;
	int ret;

	ret = v4l2_device_register(dev, &fd->v4l2_dev);
	if (ret) {
		aie_dev_info(dev, "Failed to register v4l2 device\n");
		return ret;
	}

	fd->m2m_dev = v4l2_m2m_init(&fd_m2m_ops);
	if (IS_ERR(fd->m2m_dev)) {
		aie_dev_info(dev, "Failed to init mem2mem device\n");
		ret = PTR_ERR(fd->m2m_dev);
		goto err_unreg_v4l2_dev;
	}

	mdev->dev = dev;
	ret = strscpy(mdev->model, dev_driver_string(dev), sizeof(mdev->model));
	if (ret < 0)
		goto err_unreg_v4l2_dev;

	ret = snprintf(mdev->bus_info, sizeof(mdev->bus_info), "platform:%s",
		 dev_name(dev));
	if (ret < 0)
		goto err_unreg_v4l2_dev;

	media_device_init(mdev);
	mdev->ops = &fd_m2m_media_ops;
	fd->v4l2_dev.mdev = mdev;

	ret = mtk_aie_video_device_register(fd);
	if (ret) {
		aie_dev_info(dev, "Failed to register video device\n");
		goto err_cleanup_mdev;
	}

	ret = media_device_register(mdev);
	if (ret) {
		aie_dev_info(dev, "Failed to register mem2mem media device\n");
		goto err_unreg_vdev;
	}

	return 0;

err_unreg_vdev:
	v4l2_m2m_unregister_media_controller(fd->m2m_dev);
	video_unregister_device(&fd->vfd);
err_cleanup_mdev:
	media_device_cleanup(mdev);
	v4l2_m2m_release(fd->m2m_dev);
err_unreg_v4l2_dev:
	v4l2_device_unregister(&fd->v4l2_dev);
	return ret;
}

static void mtk_aie_dev_v4l2_release(struct mtk_aie_dev *fd)
{
	v4l2_m2m_unregister_media_controller(fd->m2m_dev);
	video_unregister_device(&fd->vfd);
	media_device_cleanup(&fd->mdev);
	v4l2_m2m_release(fd->m2m_dev);
	v4l2_device_unregister(&fd->v4l2_dev);
}

static void mtk_aie_frame_done_worker(struct work_struct *work)
{
	struct mtk_aie_req_work *req_work = (struct mtk_aie_req_work *)work;
	struct mtk_aie_dev *fd = (struct mtk_aie_dev *)req_work->fd_dev;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	cmdq_pkt_wait_complete(fd->pkt);
	cmdq_pkt_destroy(fd->pkt);

	if (fd->isHwHang) {
		mtk_aie_hw_done(fd, VB2_BUF_STATE_ERROR);
	} else {
		aie_get_time(fd->tv, 5);
		switch (fd->aie_cfg->sel_mode) {
		case FDMODE:
			fd->drv_ops->get_fd_result(fd, fd->aie_cfg);
		break;
		case ATTRIBUTEMODE:
			fd->drv_ops->get_attr_result(fd, fd->aie_cfg);
		break;
		case FLDMODE:
			fd->drv_ops->get_fld_result(fd, fd->aie_cfg);
		break;
		default:
		break;
		}

		mtk_aie_hw_done(fd, VB2_BUF_STATE_DONE);
	}

	AIE_SYSTRACE_END();
}

static int mtk_aie_probe(struct platform_device *pdev)
{
	struct mtk_aie_dev *fd;
	struct device *dev = &pdev->dev;

	struct resource *res;
	int ret, i;
	const struct aie_data *data;
	struct aie_reg_map *reg_map;

	aie_dev_info(dev, "probe start\n");

	fd = devm_kzalloc(&pdev->dev, sizeof(*fd), GFP_KERNEL);
	if (!fd) {
		aie_dev_info(dev, "devm_kzalloc fail!\n");
		return -ENOMEM;
	}
	memset(fd, 0, sizeof(*fd));

	data = of_device_get_match_data(&pdev->dev);
	if (!data) {
		aie_dev_info(dev, "match data is NULL\n");
		return PTR_ERR(data);
	}
	fd->drv_ops = data->drv_ops;
	fd->larb_clk_ready = data->larb_clk_ready;
	fd->reg_map_num = data->reg_map_num;
	fd->is_cmdq_polling = data->is_cmdq_polling;
	reg_map = data->reg_map;
	fd->is_shutdown = false;

	if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(34)))
		aie_dev_info(dev, "%s: No suitable DMA available\n", __func__);

	if (!dev->dma_parms) {
		dev->dma_parms =
			devm_kzalloc(dev, sizeof(*dev->dma_parms), GFP_KERNEL);
		if (!dev->dma_parms)
			return -ENOMEM;
	}

	if (dev->dma_parms) {
		ret = dma_set_max_seg_size(dev, UINT_MAX);
		if (ret)
			aie_dev_info(dev, "Failed to set DMA segment size\n");
	}

	dev_set_drvdata(dev, fd);
	fd->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	fd->fd_base = devm_ioremap_resource(dev, res);
	if (IS_ERR(fd->fd_base)) {
		aie_dev_info(dev, "Failed to get fd reg base\n");
		return PTR_ERR(fd->fd_base);
	}


	if (fd->reg_map_num > MAX_REG_BASE) {
		aie_dev_info(dev, "reg_map_num is over the limit(%d/%d)\n",
			fd->reg_map_num, MAX_REG_BASE);
		return -1;
	}

	for (i = 0; i < fd->reg_map_num; i++) {
		fd->reg_base[i] = ioremap(reg_map[i].base, reg_map[i].offset);
		if (IS_ERR(fd->reg_base[i])) {
			aie_dev_info(dev, "Failed to get reg base(%i)\n", i);
			return PTR_ERR(fd->reg_base[i]);
		}
	}

	if (fd->larb_clk_ready) {
		ret = mtk_aie_dev_larb_init(fd);
		if (ret) {
			aie_dev_info(dev, "Failed to init larb : %d\n", ret);
			return ret;
		}
	}

	if (fd->larb_clk_ready) {
		fd->aie_clk.clk_num = data->clk_num;
		fd->aie_clk.clks = data->clks;
		ret = devm_clk_bulk_get(dev,
				fd->aie_clk.clk_num,
				fd->aie_clk.clks);
		if (ret) {
			aie_dev_info(dev, "Failed to get clks: %d\n", ret);
			return ret;
		}
	}

	fd->fdvt_clt = cmdq_mbox_create(dev, 0);
	if (!fd->fdvt_clt)
		aie_dev_info(dev, "cmdq mbox create fail\n");
	else
		aie_dev_info(dev, "cmdq mbox create done\n");

#if CMDQ_SEC_READY
	fd->fdvt_secure_clt = cmdq_mbox_create(dev, 1);

	if (!fd->fdvt_secure_clt)
		aie_dev_info(dev, "cmdq mbox create fail\n");
	else
		aie_dev_info(dev, "cmdq mbox create done\n");
#endif

	of_property_read_u32(pdev->dev.of_node, "fdvt-frame-done", &(fd->fdvt_event_id));

#if CMDQ_SEC_READY
	of_property_read_u32(pdev->dev.of_node, "sw-sync-token-tzmp-aie-wait",
				&(fd->fdvt_sec_wait));
	aie_dev_info(dev, "fdvt_sec_wait is %d\n", fd->fdvt_sec_wait);
	of_property_read_u32(pdev->dev.of_node, "sw-sync-token-tzmp-aie-set",
				&(fd->fdvt_sec_set));
	aie_dev_info(dev, "fdvt_sec_set is %d\n", fd->fdvt_sec_set);
#endif

	mutex_init(&fd->vfd_lock);
	init_completion(&fd->fd_job_finished);
	init_waitqueue_head(&fd->flushing_waitq);
	atomic_set(&fd->num_composing, 0);

	fd->frame_done_wq =
			alloc_ordered_workqueue(dev_name(fd->dev),
						WQ_HIGHPRI | WQ_FREEZABLE);
	if (!fd->frame_done_wq) {
		aie_dev_info(fd->dev, "failed to alloc frame_done workqueue\n");
		mutex_destroy(&fd->vfd_lock);
		return -ENOMEM;
	}

	INIT_WORK(&fd->req_work.work, mtk_aie_frame_done_worker);
	fd->req_work.fd_dev = fd;

	//mtk_aie_mmdvfs_init(fd);
	//mtk_aie_mmqos_init(fd);
	fd->aov_pdev = pdev;
	gaov_dev = pdev;

	if (fd->larb_clk_ready)
		pm_runtime_enable(dev);

	ret = mtk_aie_dev_v4l2_init(fd);
	if (ret) {
		aie_dev_info(dev, "Failed to init v4l2 device: %d\n", ret);
		goto err_destroy_mutex;
	}

	aie_pm_dev = &pdev->dev;
#if IS_ENABLED(CONFIG_PM)
	ret = register_pm_notifier(&aie_notifier_block);
	if (ret) {
		aie_dev_info(dev, "failed to register notifier block.\n");
		return ret;
	}
#endif

	fd->smmu_dev = mtk_smmu_get_shared_device(dev);
	if (!fd->smmu_dev) {
		aie_dev_info(dev,
			"%s: failed to get aie smmu device\n",
			__func__);
		return -EINVAL;
	}

	aie_dev_info(dev, "AIE : Success to %s\n", __func__);

	return 0;

err_destroy_mutex:
	pm_runtime_disable(fd->dev);
	//mtk_aie_mmdvfs_uninit(fd);
	//mtk_aie_mmqos_uninit(fd);
	destroy_workqueue(fd->frame_done_wq);
	mutex_destroy(&fd->vfd_lock);

	return ret;
}

static int mtk_aie_remove(struct platform_device *pdev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(&pdev->dev);

	mtk_aie_dev_v4l2_release(fd);

	pm_runtime_disable(&pdev->dev);

	//mtk_aie_mmdvfs_uninit(fd);
	//mtk_aie_mmqos_uninit(fd);
	destroy_workqueue(fd->frame_done_wq);
	fd->frame_done_wq = NULL;
	mutex_destroy(&fd->vfd_lock);

	return 0;
}

static int mtk_aie_runtime_suspend(struct device *dev)
{
	aie_dev_info(dev, "%s: runtime suspend aie job)\n", __func__);
	mtk_aie_ccf_disable(dev);

	if (m_aov_notify != NULL)
		m_aov_notify(gaov_dev, AOV_NOTIFY_AIE_AVAIL, 1); //unavailable: 0 available: 1

	return 0;
}

static int mtk_aie_runtime_resume(struct device *dev)
{
	struct mtk_aie_dev *fd = dev_get_drvdata(dev);

	aie_dev_info(dev, "%s: empty runtime resume aie job)\n", __func__);
	mtk_aie_ccf_enable(dev);

	if (fd->drv_ops->enable_ddren)
		fd->drv_ops->enable_ddren(fd);

	return 0;
}

static void mtk_aie_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct mtk_aie_dev *fd = dev_get_drvdata(&pdev->dev);

	fd->is_shutdown = true;
	if (fd->fdvt_clt)
		cmdq_mbox_stop(fd->fdvt_clt);
	else
		aie_dev_info(dev, "%s: fdvt cmdq client is NULL\n", __func__);


	aie_dev_info(dev, "%s: aie shutdown ready(%d)\n",
		__func__, fd->is_shutdown);
}

static const struct dev_pm_ops mtk_aie_pm_ops = {
		SET_RUNTIME_PM_OPS(mtk_aie_runtime_suspend,
				   mtk_aie_runtime_resume, NULL)};

static const struct of_device_id mtk_aie_of_ids[] = {
	{
		.compatible = "mediatek,aie-isp7s",
		.data = &data_isp7s,
	},
	{
		.compatible = "mediatek,aie-isp7sp",
		.data = &data_isp7sp,
	},
	{
		.compatible = "mediatek,aie-isp7sp-1",
		.data = &data_isp7sp_1,
	},
	{
		.compatible = "mediatek,aie-isp7sp-2",
		.data = &data_isp7sp_2,
	},
	{} };
MODULE_DEVICE_TABLE(of, mtk_aie_of_ids);

static struct platform_driver mtk_aie_driver = {
	.probe = mtk_aie_probe,
	.remove = mtk_aie_remove,
	.shutdown = mtk_aie_shutdown,
	.driver = {
		.name = "mtk-aie-5.3",
		.of_match_table = of_match_ptr(mtk_aie_of_ids),
		.pm = &mtk_aie_pm_ops,
	} };

module_platform_driver(mtk_aie_driver);
MODULE_AUTHOR("Fish Wu <fish.wu@mediatek.com>");
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(DMA_BUF);
MODULE_DESCRIPTION("Mediatek AIE driver");
