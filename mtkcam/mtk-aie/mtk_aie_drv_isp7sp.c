// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020 MediaTek Inc.
 *
 * Author: Ming-Hsuan.Chiang <Ming-Hsuan.Chiang@mediatek.com>
 *
 */

#include "mtk_aie.h"
#include "mtk_aie_reg_7sp.h"
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/device.h>
#include <linux/dma-heap.h>
#include <linux/soc/mediatek/mtk_sip_svc.h>
#include <linux/arm-smccc.h>
#include <linux/version.h>
#include "mtk_heap.h"
#include <uapi/linux/dma-heap.h>
#include <linux/scatterlist.h>
#include <linux/soc/mediatek/mtk-cmdq-ext.h>
#include "cmdq-sec.h"
#include "cmdq-sec-iwc-common.h"
#include "iommu_debug.h"

#include "aie_mp_fw_7sp_def.h"
#include "mtk_aie-trace.h"

#define FDVT_USE_GCE 1
#define FLD
#define FLD_ALIGN 128
#define CHECK_SERVICE_0 0
#define BUFTAG "AIE"
#define MAX_PYRAMID_WIDTH 640
#define MAX_PYRAMID_HEIGHT 640

#define AIE_ALIGN32(x) round_up(x, 32)

enum AIE_BUF_TYPE {
	SECURE_BUF,
	CACHED_BUF,
	UNCACHED_BUF
};

static struct cmdq_pkt *g_sec_pkt;
static const unsigned int fd_wdma_en[fd_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1},
	{1, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1},
	{1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0},
	{1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 0, 0, 0},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0} };

static const unsigned int out_stride_size[fd_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 0, 0}, {1, 0, 2, 0}, {1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2},
	{1, 1, 2, 2}, {1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {3, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2}, {1, 1, 2, 2},
	{1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {3, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0},
	{1, 0, 2, 0}, {1, 0, 0, 0}, {1, 1, 2, 2}, {1, 1, 2, 2}, {1, 0, 0, 0},
	{1, 0, 2, 0}, {1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 2, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 1, 1, 1}, {1, 1, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}, {1, 1, 0, 0},
	{1, 0, 0, 0}, {3, 0, 0, 0} };

static const unsigned int fd_ker_rdma_size[fd_loop_num][kernel_RDMA_RA_num] = {
	{240, 240},   {1168, 1168}, {1168, 1168}, {272, 272},   {2320, 2320},
	{2080, 2080}, {1040, 1040}, {4624, 4624}, {3104, 3104}, {9232, 9232},
	{4624, 4624}, {4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624},
	{1552, 1552}, {4624, 4624}, {4624, 4624}, {4128, 4128}, {1040, 1040},
	{1040, 1040}, {528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080},
	{2080, 2080}, {2080, 2080}, {1040, 1040}, {0, 0},       {240, 240},
	{1168, 1168}, {1168, 1168}, {272, 272},   {2320, 2320}, {2080, 2080},
	{1040, 1040}, {4624, 4624}, {3104, 3104}, {9232, 9232}, {4624, 4624},
	{4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624}, {1552, 1552},
	{4624, 4624}, {4624, 4624}, {4128, 4128}, {1040, 1040}, {1040, 1040},
	{528, 528},   {4160, 4160}, {4160, 4160}, {2080, 2080}, {2080, 2080},
	{2080, 2080}, {1040, 1040}, {0, 0},       {240, 240},   {1168, 1168},
	{1168, 1168}, {272, 272},   {2320, 2320}, {2080, 2080}, {1040, 1040},
	{4624, 4624}, {3104, 3104}, {9232, 9232}, {4624, 4624}, {4128, 4128},
	{1040, 1040}, {4624, 4624}, {4624, 4624}, {1552, 1552}, {4624, 4624},
	{4624, 4624}, {4128, 4128}, {1040, 1040}, {1040, 1040}, {528, 528},
	{4160, 4160}, {4160, 4160}, {2080, 2080}, {2080, 2080}, {2080, 2080},
	{1040, 1040}, {0, 0} };
static unsigned int fd_ker_rdma_size_aligned[fd_loop_num][kernel_RDMA_RA_num];

static const unsigned int fd_out_stride2_in[fd_loop_num] = {
	0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int fd_stride[fd_loop_num] = {
	2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

static const unsigned int fd_maxpool[fd_loop_num] = {
	0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int out_2size[fd_loop_num] = {
	0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const unsigned int in_ch_pack[fd_loop_num] = {
	1, 16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 0,  1,  16, 16, 16, 16, 16, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 0,  1,  16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 0};

static const unsigned int outlayer[fd_loop_num] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};
static const unsigned int out_ch_pack[fd_loop_num] = {
	16, 16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0,  16,
	16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0,  16, 16,
	16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 16, 16, 16, 32, 32, 32, 32, 32, 32, 0};

static const unsigned int anchor_en_num[fd_loop_num] = {
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5,  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};

/* [loop][ch][output_index] */
static const signed int fd_rdma_en[fd_loop_num][input_WDMA_WRA_num][2] = {
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{0, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 0}, {2, 0}, {-1, -1}, {-1, -1} },
	{{3, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{1, 2}, {2, 2}, {4, 2}, {4, 3} },
	{{5, 0}, {5, 1}, {-1, -1}, {-1, -1} },
	{{6, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{5, 0}, {5, 1}, {7, 0}, {-1, -1} },
	{{8, 0}, {8, 1}, {-1, -1}, {-1, -1} },
	{{9, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{5, 2}, {5, 3}, {7, 2}, {10, 2} },
	{{11, 0}, {11, 1}, {-1, -1}, {-1, -1} },
	{{12, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{13, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{11, 0}, {11, 1}, {14, 0}, {-1, -1} },
	{{15, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{16, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{11, 0}, {11, 1}, {14, 0}, {17, 0} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {18, 1}, {-1, -1}, {-1, -1} },
	{{19, 0}, {22, 0}, {22, 1}, {25, 0} },
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{29, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 0}, {31, 0}, {-1, -1}, {-1, -1} },
	{{32, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{30, 2}, {31, 2}, {33, 2}, {33, 3} },
	{{34, 0}, {34, 1}, {-1, -1}, {-1, -1} },
	{{35, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{34, 0}, {34, 1}, {36, 0}, {-1, -1} },
	{{37, 0}, {37, 1}, {-1, -1}, {-1, -1} },
	{{38, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{34, 2}, {34, 3}, {36, 2}, {39, 2} },
	{{40, 0}, {40, 1}, {-1, -1}, {-1, -1} },
	{{41, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{42, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{40, 0}, {40, 1}, {43, 0}, {-1, -1} },
	{{44, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{45, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{40, 0}, {40, 1}, {43, 0}, {46, 0} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{47, 0}, {47, 1}, {-1, -1}, {-1, -1} },
	{{48, 0}, {51, 0}, {51, 1}, {54, 0} },
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{58, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 0}, {60, 0}, {-1, -1}, {-1, -1} },
	{{61, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{59, 2}, {60, 2}, {62, 2}, {62, 3} },
	{{63, 0}, {63, 1}, {-1, -1}, {-1, -1} },
	{{64, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{63, 0}, {63, 1}, {65, 0}, {-1, -1} },
	{{66, 0}, {66, 1}, {-1, -1}, {-1, -1} },
	{{67, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{63, 2}, {63, 3}, {65, 2}, {68, 2} },
	{{69, 0}, {69, 1}, {-1, -1}, {-1, -1} },
	{{70, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{71, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{69, 0}, {69, 1}, {72, 0}, {-1, -1} },
	{{73, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{74, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{69, 0}, {69, 1}, {72, 0}, {75, 0} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{76, 0}, {76, 1}, {-1, -1}, {-1, -1} },
	{{77, 0}, {80, 0}, {80, 1}, {83, 0} } };

static const unsigned int attr_wdma_en[attr_loop_num][output_WDMA_WRA_num] = {
	{1, 0, 1, 0}, {1, 0, 1, 0}, {1, 0, 0, 0}, {1, 1, 1, 1}, {1, 1, 1, 1},
	{1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 1, 0}, {1, 1, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 1, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
	{1, 0, 0, 0} };
static const unsigned int
	attr_ker_rdma_size[attr_loop_num][kernel_RDMA_RA_num] = {
		{240, 240},   {1168, 1168}, {272, 272},   {2320, 2320},
		{2080, 2080}, {9232, 9232}, {3104, 3104}, {9232, 9232},
		{4128, 4128}, {1040, 1040}, {4624, 4624}, {4624, 4624},
		{1552, 1552}, {4624, 4624}, {4624, 4624}, {4128, 4128},
		{9232, 9232}, {272, 272},   {9232, 9232}, {2320, 2320},
		{144, 144},   {9232, 9232}, {272, 272},   {9232, 9232},
		{2320, 2320}, {144, 144} };
static unsigned int
	attr_ker_rdma_aligned_size[attr_loop_num][kernel_RDMA_RA_num];
static const unsigned int attr_out_stride2_as_in[attr_loop_num] = {
	0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned int attr_fd_stride[attr_loop_num] = {/* H */
							   2, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1, 1, 1,
							   1, 1, 1, 1, 1};
static const unsigned int attr_fd_maxpool[attr_loop_num] = {/* L */
							    1, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0, 0, 0,
							    0, 0, 0, 0, 0};
static const unsigned int attr_out_2size[attr_loop_num] = {/* O */
							   1, 1, 0, 1, 1, 1, 0,
							   1, 0, 0, 0, 0, 0, 0,
							   0, 0, 0, 0, 0, 0, 0,
							   0, 0, 0, 0, 0};
static const unsigned int attr_input_ch_pack[attr_loop_num] = {
	1,  16, 16, 16, 16, 32, 32, 32, 32, 32, 32, 32, 32,
	32, 32, 32, 32, 32, 32, 32, 16, 32, 32, 32, 32, 16};
/* [loop][ch][output_index] */
static const signed int attr_rdma_en[attr_loop_num][input_WDMA_WRA_num][2] = {
	{{99, 99}, {99, 99}, {99, 99}, {-1, -1} },
	{{0, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{0, 0}, {1, 0}, {-1, -1}, {-1, -1} },
	{{2, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{0, 2}, {1, 2}, {3, 2}, {3, 3} },
	{{4, 0}, {4, 1}, {-1, -1}, {-1, -1} },
	{{4, 0}, {4, 1}, {5, 0}, {-1, -1} },
	{{6, 0}, {6, 1}, {-1, -1}, {-1, -1} },
	{{4, 2}, {4, 3}, {5, 2}, {7, 2} },
	{{8, 0}, {8, 1}, {-1, -1}, {-1, -1} },
	{{9, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{10, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{8, 0}, {8, 1}, {11, 0}, {-1, -1} },
	{{12, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{13, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{8, 0}, {8, 1}, {11, 0}, {14, 0} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{16, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{18, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{19, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{21, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{15, 0}, {15, 1}, {-1, -1}, {-1, -1} },
	{{23, 0}, {-1, -1}, {-1, -1}, {-1, -1} },
	{{24, 0}, {-1, -1}, {-1, -1}, {-1, -1} } };

static const unsigned int attr_wdma_size[attr_loop_num][output_WDMA_WRA_num] = {
	{16384, 0, 4096, 0},
	{16384, 0, 4096, 0},
	{16384, 0, 0, 0},
	{16384, 16384, 4096, 4096},
	{8192, 8192, 2048, 2048},
	{8192, 0, 2048, 0},
	{8192, 8192, 0, 0},
	{8192, 0, 2048, 0},
	{2048, 2048, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 0, 0, 0},
	{2048, 2048, 0, 0},
	{2048, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{1024, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{0, 0, 0, 0},
	{2048, 0, 0, 0},
	{1024, 0, 0, 0},
	{0, 0, 0, 0} };
static const unsigned int debug_info_sel[input_WDMA_WRA_num] = {
	0x00000000, 0x11111111, 0x22222222, 0x33333333};

enum aie_smc_control {
	AIE_SET_DOMAIN
};

static unsigned int attr_wdma_aligned_size[attr_loop_num][output_WDMA_WRA_num];
/* (128-bits ALIGN work-around)*/
#define fld_blink_weight_size 6416
#define fld_blink_weight_size_non_align 6416
#define fld_cv_size 1072
#define fld_cv_size_00 1072
#define fld_cv_size_00_non_align 1072
#define fld_fp_size 5344
#define fld_fp_size_non_align 5344
#define fld_leafnode_size 307200
#define fld_tree_size 16000
#define fld_tree_size_non_align 16000
#define fld_shape_size 192
#define fld_shape_size_non_align 192
#define fld_result_size 112
#define fld_forest 14
#define fld_point 500
#define fld_cur_landmark 11
#define CHECK_SERVICE_IF_0 0

/* For reducing AIE memory usage, we reuse the memory of different fd loop
 * with the method of arranging wdma buffer which always choose the first big
 * enough memory section to allocate or increase fd dma buffer size if there
 * is no such memory section. This method is implemented by simulation tool, and
 * fd dma buffer size and wdma offset are generated by the tool as follows:
 */
#define wdma_offset_0_0 0
#define wdma_offset_1_0 102400
#define wdma_offset_1_2 128000
#define wdma_offset_2_0 0
#define wdma_offset_2_2 25600
#define wdma_offset_3_0 32000
#define wdma_offset_4_0 0
#define wdma_offset_4_1 57600
#define wdma_offset_4_2 83200
#define wdma_offset_4_3 89600
#define wdma_offset_5_0 0
#define wdma_offset_5_1 12800
#define wdma_offset_5_2 32000
#define wdma_offset_5_3 35200
#define wdma_offset_6_0 38400
#define wdma_offset_7_0 51200
#define wdma_offset_7_2 25600
#define wdma_offset_8_0 38400
#define wdma_offset_8_1 64000
#define wdma_offset_9_0 0
#define wdma_offset_10_0 12800
#define wdma_offset_10_2 28800
#define wdma_offset_11_0 0
#define wdma_offset_11_1 3200
#define wdma_offset_12_0 6400
#define wdma_offset_13_0 9600
#define wdma_offset_14_0 6400
#define wdma_offset_15_0 9600
#define wdma_offset_16_0 12800
#define wdma_offset_17_0 9600
#define wdma_offset_18_0 12800
#define wdma_offset_18_1 16000
#define wdma_offset_19_0 19200
#define wdma_offset_22_0 35200
#define wdma_offset_22_1 67200
#define wdma_offset_25_0 99200
#define wdma_offset_29_0 0
#define wdma_offset_30_0 409600
#define wdma_offset_30_2 512000
#define wdma_offset_31_0 0
#define wdma_offset_31_2 102400
#define wdma_offset_32_0 128000
#define wdma_offset_33_0 0
#define wdma_offset_33_1 230400
#define wdma_offset_33_2 332800
#define wdma_offset_33_3 358400
#define wdma_offset_34_0 0
#define wdma_offset_34_1 51200
#define wdma_offset_34_2 128000
#define wdma_offset_34_3 140800
#define wdma_offset_35_0 153600
#define wdma_offset_36_0 204800
#define wdma_offset_36_2 102400
#define wdma_offset_37_0 153600
#define wdma_offset_37_1 256000
#define wdma_offset_38_0 0
#define wdma_offset_39_0 51200
#define wdma_offset_39_2 115200
#define wdma_offset_40_0 0
#define wdma_offset_40_1 12800
#define wdma_offset_41_0 25600
#define wdma_offset_42_0 38400
#define wdma_offset_43_0 25600
#define wdma_offset_44_0 38400
#define wdma_offset_45_0 51200
#define wdma_offset_46_0 38400
#define wdma_offset_47_0 51200
#define wdma_offset_47_1 64000
#define wdma_offset_48_0 76800
#define wdma_offset_51_0 140800
#define wdma_offset_51_1 268800
#define wdma_offset_54_0 396800
#define wdma_offset_58_0 0
#define wdma_offset_59_0 1638400
#define wdma_offset_59_2 2048000
#define wdma_offset_60_0 0
#define wdma_offset_60_2 409600
#define wdma_offset_61_0 512000
#define wdma_offset_62_0 0
#define wdma_offset_62_1 921600
#define wdma_offset_62_2 1331200
#define wdma_offset_62_3 1433600
#define wdma_offset_63_0 0
#define wdma_offset_63_1 204800
#define wdma_offset_63_2 512000
#define wdma_offset_63_3 563200
#define wdma_offset_64_0 614400
#define wdma_offset_65_0 819200
#define wdma_offset_65_2 409600
#define wdma_offset_66_0 614400
#define wdma_offset_66_1 1024000
#define wdma_offset_67_0 0
#define wdma_offset_68_0 204800
#define wdma_offset_68_2 460800
#define wdma_offset_69_0 0
#define wdma_offset_69_1 51200
#define wdma_offset_70_0 102400
#define wdma_offset_71_0 153600
#define wdma_offset_72_0 102400
#define wdma_offset_73_0 153600
#define wdma_offset_74_0 204800
#define wdma_offset_75_0 153600
#define wdma_offset_76_0 204800
#define wdma_offset_76_1 256000
#define wdma_offset_77_0 307200
#define wdma_offset_80_0 563200
#define wdma_offset_80_1 1075200
#define wdma_offset_83_0 1587200
#define FD_DMA_BUFFER_SIZE round_up(2150400, 32)

static unsigned int g_fd_rs_config_offset;
static unsigned int g_fd_yuv2rgb_config_offset;
static unsigned int g_fd_fd_config_offset;

static void aie_irqhandle(struct mtk_aie_dev *fd);

static void aie_arrange_config(struct mtk_aie_dev *fd);
static void aie_arrange_network(struct mtk_aie_dev *fd);

static void aie_dump_cg_reg(struct mtk_aie_dev *fd);

static void FDVT_DumpDRAMOut(struct mtk_aie_dev *fd, unsigned int *hw, unsigned int size)
{
	unsigned int i;
	unsigned int comparetimes = size / 4;

	for (i = 0; i < comparetimes; i += 4) {
		aie_dev_info(fd->dev, "[%d] 0x%08x, 0x%08x, 0x%08x, 0x%08x", i, hw[i],
						hw[i + 1], hw[i + 2], hw[i + 3]);
	}
	aie_dev_info(fd->dev, "Dump End");
}

static void aie_fdvt_dump_reg(struct mtk_aie_dev *fd)
{
	//int fld_face_num = fd->aie_cfg->fld_face_num;
	unsigned int loop_num = 1;
	int i = 0;

	if (fd->is_shutdown) {
		aie_dev_info(fd->dev, "%s: skip for shutdown", __func__);
		return;
	}

	aie_dump_cg_reg(fd);

	aie_dev_info(fd->dev, "%s result result1: %x, %x, %x", __func__,
		 readl(fd->fd_base + AIE_RESULT_0_REG),
		 readl(fd->fd_base + AIE_RESULT_1_REG),
		 readl(fd->fd_base + AIE_DMA_CTL_REG));

	aie_dev_info(fd->dev, "%s interrupt enable: %x", __func__,
		 readl(fd->fd_base + AIE_INT_EN_REG));

	writel(0x11, fd->fd_base + AIE_INT_EN_REG);
	aie_dev_info(fd->dev, "%s interrupt status: %x", __func__,
		 readl(fd->fd_base + AIE_INT_REG));

	if (fd->aie_cfg->sel_mode == ATTRIBUTEMODE) {
		aie_dev_info(fd->dev, "[ATTRMODE] w_idx = %d, r_idx = %d\n",
			 fd->attr_para->w_idx, fd->attr_para->r_idx);
	} else if (fd->aie_cfg->sel_mode == FLDMODE) {
		aie_dev_info(fd->dev, "Blink Addr: %llx\n", fd->dma_para->fld_blink_weight_pa);
		for (i = 0; i < 15; i++) {
			aie_dev_info(fd->dev, "[%d]CV Addr: %llx\n", i, fd->dma_para->fld_cv_pa[i]);
			aie_dev_info(fd->dev, "[%d]LEAFNODE Addr: %llx\n", i,
						fd->dma_para->fld_leafnode_pa[i]);
			aie_dev_info(fd->dev, "[%d]FP Addr: %llx\n", i, fd->dma_para->fld_fp_pa[i]);
			aie_dev_info(fd->dev, "[%d]Tree02 Addr: %llx\n", i,
						fd->dma_para->fld_tree02_pa[i]);
			//aie_dev_info(fd->dev, "[%d]Tree03 Addr: %x\n", i,
			//			fd->dma_para->fld_tree13_pa[i]);
		}
		aie_dev_info(fd->dev, "OUT Addr: %llx\n", fd->dma_para->fld_output_pa);

		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_START_REG,
					(unsigned int)readl(fd->fd_base + AIE_START_REG));

		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_DMA_CTL_REG,
					(unsigned int)readl(fd->fd_base + AIE_DMA_CTL_REG));

		aie_dev_info(fd->dev, "FLD_IMG_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_IMG_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_IMG_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_MS_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_MS_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_MS_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_FP_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_FP_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_FP_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_TR_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_TR_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_TR_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_SH_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_SH_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_SH_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_CV_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_CV_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_CV_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_BS_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_BS_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_BS_BASE_ADDR));
		aie_dev_info(fd->dev, "FLD_PP_BASE_ADDR[0x%08X %08X]\n",
				(unsigned int) FLD_PP_BASE_ADDR,
				(unsigned int) readl(fd->fd_base + FLD_PP_BASE_ADDR));

		aie_dev_info(fd->dev, "FLD_FP_FORT_OFST[0x%08X %08X]\n",
				(unsigned int) FLD_FP_FORT_OFST,
				(unsigned int) readl(fd->fd_base + FLD_FP_FORT_OFST));
		aie_dev_info(fd->dev, "FLD_TR_FORT_OFST[0x%08X %08X]\n",
				(unsigned int) FLD_TR_FORT_OFST,
				(unsigned int) readl(fd->fd_base + FLD_TR_FORT_OFST));
		aie_dev_info(fd->dev, "FLD_SH_FORT_OFST[0x%08X %08X]\n",
				(unsigned int) FLD_SH_FORT_OFST,
				(unsigned int) readl(fd->fd_base + FLD_SH_FORT_OFST));
		aie_dev_info(fd->dev, "FLD_CV_FORT_OFST[0x%08X %08X]\n",
				(unsigned int) FLD_CV_FORT_OFST,
				(unsigned int) readl(fd->fd_base + FLD_CV_FORT_OFST));

		aie_dev_info(fd->dev, "FLD face Info:\n");
		for (i = 0; i < fd->aie_cfg->fld_face_num; i++) {
			aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)fld_face_info_idx_0[i],
					(unsigned int)readl(fd->fd_base + fld_face_info_idx_0[i]));

			aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)fld_face_info_idx_1[i],
					(unsigned int)readl(fd->fd_base + fld_face_info_idx_1[i]));
		}

		aie_dev_info(fd->dev, "FLD_NUM_CONFIG_0[0x%08X %08X]\n",
				(unsigned int) FLD_NUM_CONFIG_0,
				(unsigned int) readl(fd->fd_base + FLD_NUM_CONFIG_0));
		aie_dev_info(fd->dev, "FLD_FACE_NUM[0x%08X %08X]\n", (unsigned int)FLD_FACE_NUM,
					(unsigned int) readl(fd->fd_base + FLD_FACE_NUM));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_0[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_MEAN_SCALE_0,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_0));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_1[0x%08X %08X]\n",
				(unsigned int)FLD_PCA_MEAN_SCALE_1,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_1));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_2[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_MEAN_SCALE_2,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_2));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_3[0x%08X %08X]\n",
				(unsigned int)FLD_PCA_MEAN_SCALE_3,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_3));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_4[0x%08X %08X]\n",
				(unsigned int)FLD_PCA_MEAN_SCALE_4,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_4));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_5[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_MEAN_SCALE_5,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_5));
		aie_dev_info(fd->dev, "FLD_PCA_MEAN_SCALE_6[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_MEAN_SCALE_6,
				(unsigned int) readl(fd->fd_base + FLD_PCA_MEAN_SCALE_6));

		aie_dev_info(fd->dev, "FLD_PCA_VEC_0[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_0,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_0));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_1[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_1,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_1));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_2[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_2,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_2));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_3[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_3,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_3));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_4[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_4,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_4));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_5[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_5,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_5));
		aie_dev_info(fd->dev, "FLD_PCA_VEC_6[0x%08X %08X]\n",
				(unsigned int) FLD_PCA_VEC_6,
				(unsigned int) readl(fd->fd_base + FLD_PCA_VEC_6));

		aie_dev_info(fd->dev, "FLD_CV_BIAS_FR_0[0x%08X %08X]\n",
				(unsigned int) FLD_CV_BIAS_FR_0,
				(unsigned int) readl(fd->fd_base + FLD_CV_BIAS_FR_0));
		aie_dev_info(fd->dev, "FLD_CV_BIAS_PF_0[0x%08X %08X]\n",
				(unsigned int) FLD_CV_BIAS_PF_0,
				(unsigned int) readl(fd->fd_base + FLD_CV_BIAS_PF_0));
		aie_dev_info(fd->dev, "FLD_CV_RANGE_FR_0[0x%08X %08X]\n",
				(unsigned int) FLD_CV_RANGE_FR_0,
				(unsigned int) readl(fd->fd_base + FLD_CV_RANGE_FR_0));
		aie_dev_info(fd->dev, "FLD_CV_RANGE_FR_1[0x%08X %08X]\n",
				(unsigned int) FLD_CV_RANGE_FR_1,
				(unsigned int) readl(fd->fd_base + FLD_CV_RANGE_FR_1));
		aie_dev_info(fd->dev, "FLD_CV_RANGE_PF_0[0x%08X %08X]\n",
				(unsigned int) FLD_CV_RANGE_PF_0,
				(unsigned int) readl(fd->fd_base + FLD_CV_RANGE_PF_0));
		aie_dev_info(fd->dev, "FLD_CV_RANGE_PF_1[0x%08X %08X]\n",
				(unsigned int) FLD_CV_RANGE_PF_1,
				(unsigned int) readl(fd->fd_base + FLD_CV_RANGE_PF_1));
		aie_dev_info(fd->dev, "FLD_PP_COEF[0x%08X %08X]\n",
				(unsigned int) FLD_PP_COEF,
				(unsigned int) readl(fd->fd_base + FLD_PP_COEF));

		aie_dev_info(fd->dev, "FLD_BS_CONFIG0[0x%08X %08X]\n",
				(unsigned int) FLD_BS_CONFIG0,
				(unsigned int) readl(fd->fd_base + FLD_BS_CONFIG0));
		aie_dev_info(fd->dev, "FLD_BS_CONFIG1[0x%08X %08X]\n",
				(unsigned int) FLD_BS_CONFIG1,
				(unsigned int) readl(fd->fd_base + FLD_BS_CONFIG1));
		aie_dev_info(fd->dev, "FLD_BS_CONFIG2[0x%08X %08X]\n",
				(unsigned int) FLD_BS_CONFIG2,
				(unsigned int) readl(fd->fd_base + FLD_BS_CONFIG2));

		aie_dev_info(fd->dev, "FLD_SRC_SIZE[0x%08X %08X]\n", (unsigned int)FLD_SRC_SIZE,
					(unsigned int) readl(fd->fd_base + FLD_SRC_SIZE));
		aie_dev_info(fd->dev, "FLD busy[0x%08X %08X]\n", (unsigned int)FLD_FLD_BUSY,
					(unsigned int) readl(fd->fd_base + FLD_FLD_BUSY));
		aie_dev_info(fd->dev, "FLD done[0x%08X %08X]\n", (unsigned int)FLD_FLD_DONE,
					(unsigned int) readl(fd->fd_base + FLD_FLD_DONE));

		aie_dev_info(fd->dev, "FLD_FLD_STM_DBG_DATA0[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_STM_DBG_DATA0,
				(unsigned int) readl(fd->fd_base + FLD_FLD_STM_DBG_DATA0));
		aie_dev_info(fd->dev, "FLD_FLD_STM_DBG_DATA1[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_STM_DBG_DATA1,
				(unsigned int) readl(fd->fd_base + FLD_FLD_STM_DBG_DATA1));
		aie_dev_info(fd->dev, "FLD_FLD_STM_DBG_DATA2[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_STM_DBG_DATA2,
				(unsigned int) readl(fd->fd_base + FLD_FLD_STM_DBG_DATA2));
		aie_dev_info(fd->dev, "FLD_FLD_STM_DBG_DATA3[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_STM_DBG_DATA3,
				(unsigned int) readl(fd->fd_base + FLD_FLD_STM_DBG_DATA3));

		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA0[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA0,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA0));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA1[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA1,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA1));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA2[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA2,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA2));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA3[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA3,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA3));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA4[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA4,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA4));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA5[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA5,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA5));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA6[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA6,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA6));
		aie_dev_info(fd->dev, "FLD_FLD_SH_DBG_DATA7[0x%08X %08X]\n",
				(unsigned int) FLD_FLD_SH_DBG_DATA7,
				(unsigned int) readl(fd->fd_base + FLD_FLD_SH_DBG_DATA7));
		aie_dev_info(fd->dev, "FDVT Check if AIE is on\n");
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)0x118,
			(unsigned int)readl(fd->fd_base + 0x118));
	} else {
		aie_dev_info(fd->dev, "- E.");
		aie_dev_info(fd->dev, "FDVT Config Info\n");
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_START_REG,
		(unsigned int)readl(fd->fd_base + AIE_START_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_ENABLE_REG,
			(unsigned int)readl(fd->fd_base + AIE_ENABLE_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_LOOP_REG,
			(unsigned int)readl(fd->fd_base + AIE_LOOP_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_INT_EN_REG,
			(unsigned int)readl(fd->fd_base + AIE_INT_EN_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_SRC_WD_HT,
			(unsigned int)readl(fd->fd_base + FDVT_SRC_WD_HT));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_DES_WD_HT,
			(unsigned int)readl(fd->fd_base + FDVT_DES_WD_HT));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_DEBUG_INFO_0,
			(unsigned int)readl(fd->fd_base + FDVT_DEBUG_INFO_0));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_DEBUG_INFO_1,
			(unsigned int)readl(fd->fd_base + FDVT_DEBUG_INFO_1));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_YUV2RGB_CON,
			(unsigned int)readl(fd->fd_base + FDVT_YUV2RGB_CON));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_RS_CON_BASE_ADR_REG,
			(unsigned int)readl(fd->fd_base + AIE_RS_CON_BASE_ADR_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_FD_CON_BASE_ADR_REG,
			(unsigned int)readl(fd->fd_base + AIE_FD_CON_BASE_ADR_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)AIE_YUV2RGB_CON_BASE_ADR_REG,
			(unsigned int)readl(fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_REG));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_IN_BASE_ADR_0,
			(unsigned int)readl(fd->fd_base + FDVT_IN_BASE_ADR_0));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_IN_BASE_ADR_1,
			(unsigned int)readl(fd->fd_base + FDVT_IN_BASE_ADR_1));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_IN_BASE_ADR_2,
			(unsigned int)readl(fd->fd_base + FDVT_IN_BASE_ADR_2));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_IN_BASE_ADR_3,
			(unsigned int)readl(fd->fd_base + FDVT_IN_BASE_ADR_3));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_OUT_BASE_ADR_0,
			(unsigned int)readl(fd->fd_base + FDVT_OUT_BASE_ADR_0));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_OUT_BASE_ADR_1,
			(unsigned int)readl(fd->fd_base + FDVT_OUT_BASE_ADR_1));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_OUT_BASE_ADR_2,
			(unsigned int)readl(fd->fd_base + FDVT_OUT_BASE_ADR_2));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_OUT_BASE_ADR_3,
			(unsigned int)readl(fd->fd_base + FDVT_OUT_BASE_ADR_3));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_KERNEL_BASE_ADR_0,
			(unsigned int)readl(fd->fd_base + FDVT_KERNEL_BASE_ADR_0));
		aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)FDVT_KERNEL_BASE_ADR_1,
			(unsigned int)readl(fd->fd_base + FDVT_KERNEL_BASE_ADR_1));
		aie_dev_info(fd->dev,
			"fdmode_fdvt_yuv2rgb_config:	0x%llx, fdmode_fdvt_yuv2rgb_config_size:	%d",
			(u64)fd->base_para->fd_yuv2rgb_cfg_va, fd->fd_yuv2rgb_cfg_size);
		FDVT_DumpDRAMOut(fd, (u32 *)fd->base_para->fd_yuv2rgb_cfg_va,
								fd->fd_yuv2rgb_cfg_size);


		if (fd->attr_para->w_idx == 0) {
			aie_dev_info(fd->dev,
				"attr_yuv2rgb_config:	0x%llx, attr_yuv2rgb_cfg_aligned_size:	%d",
				(u64)fd->base_para->attr_yuv2rgb_cfg_va[MAX_ENQUE_FRAME_NUM - 1],
				fd->attr_yuv2rgb_cfg_data_size);
			FDVT_DumpDRAMOut(fd,
				(u32 *)fd->base_para->attr_yuv2rgb_cfg_va[MAX_ENQUE_FRAME_NUM - 1],
				fd->attr_yuv2rgb_cfg_data_size);
		} else {
			aie_dev_info(fd->dev,
				"attr_yuv2rgb_config:	0x%llx, attr_yuv2rgb_cfg_aligned_size:	%d",
				(u64)fd->base_para->attr_yuv2rgb_cfg_va[fd->attr_para->w_idx - 1],
				fd->attr_yuv2rgb_cfg_data_size);
			FDVT_DumpDRAMOut(fd,
				(u32 *)fd->base_para->attr_yuv2rgb_cfg_va[fd->attr_para->w_idx - 1],
				fd->attr_yuv2rgb_cfg_data_size);
	}

		aie_dev_info(fd->dev,
			"fdmode_fdvt_rs_config:	  0x%llx, fdmode_fdvt_rs_config_size:	 %d",
			(u64)fd->base_para->fd_rs_cfg_va, fd->fd_rs_cfg_size);
		FDVT_DumpDRAMOut(fd, (u32 *)fd->base_para->fd_rs_cfg_va, fd->fd_rs_cfg_size);

		loop_num = (unsigned int)readl(fd->fd_base + FDVT_DEBUG_INFO_0) & 0xFF;

		aie_dev_info(fd->dev,
			"fdmode_fdvt_fd_config:	0x%llx, fdmode_fdvt_fd_config_size:	%d",
			(u64)fd->base_para->fd_fd_cfg_va,
			((fd->fd_fd_cfg_aligned_size)/87) * loop_num);
		FDVT_DumpDRAMOut(fd,
			(u32 *)fd->base_para->fd_fd_cfg_va
				+ sizeof(u32) * FD_CONFIG_SIZE * (fd_loop_num / 3)*
				(3 - fd->aie_cfg->number_of_pyramid),
			((fd->fd_fd_cfg_aligned_size)/fd_loop_num) * (loop_num + 1));

		aie_dev_info(fd->dev, "FDVT DMA Debug Info\n");
		aie_dev_info(fd->dev, "[FDVT_DMA_RDMA_0_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_RDMA_0_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_RDMA_0_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_RDMA_1_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_RDMA_1_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_RDMA_1_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_RDMA_2_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_RDMA_2_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_RDMA_2_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_RDMA_3_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_RDMA_3_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_RDMA_3_CHECK_SUM));

		aie_dev_info(fd->dev, "[FDVT_DMA_WDMA_0_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_WDMA_0_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_WDMA_0_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_WDMA_1_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_WDMA_1_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_WDMA_1_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_WDMA_2_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_WDMA_2_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_WDMA_2_CHECK_SUM));
		aie_dev_info(fd->dev, "[FDVT_DMA_WDMA_3_CHECK_SUM]: 0x%08X %08X\n",
		  FDVT_DMA_WDMA_3_CHECK_SUM,
		  (unsigned int)readl(fd->fd_base + FDVT_DMA_WDMA_3_CHECK_SUM));

		for (i = 0; i < input_WDMA_WRA_num; i++) {
			writel((unsigned int)debug_info_sel[i], fd->fd_base + FDVT_DMA_DEBUG_SEL);
			aie_dev_info(fd->dev, "[sel = %d][FDVT_DMA_DEBUG_SEL]: 0x%08X %08X\n",
			  i, FDVT_DMA_DEBUG_SEL,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_SEL));

			/*  read dma status  */
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_0_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_0_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_0_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_0_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_0_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_0_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_0_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_0_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_0_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_0_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_0_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_0_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_1_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_1_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_1_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_1_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_1_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_1_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_1_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_1_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_1_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_1_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_1_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_1_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_2_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_2_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_2_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_2_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_2_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_2_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_2_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_2_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_2_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_2_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_2_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_2_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_3_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_3_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_3_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_3_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_3_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_3_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_3_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_3_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_3_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_RDMA_3_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_RDMA_3_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_RDMA_3_3));

			/*  write dma status  */
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_0_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_0_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_0_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_0_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_0_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_0_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_0_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_0_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_0_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_0_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_0_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_0_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_1_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_1_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_1_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_1_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_1_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_1_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_1_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_1_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_1_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_1_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_1_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_1_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_2_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_2_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_2_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_2_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_2_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_2_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_2_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_2_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_2_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_2_3]: 0x%08X %08X\n",
			  (FDVT_DMA_DEBUG_DATA_WDMA_2_3),
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_2_3));

			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_3_0]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_3_0,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_3_0));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_3_1]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_3_1,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_3_1));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_3_2]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_3_2,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_3_2));
			aie_dev_info(fd->dev, "[FDVT_DMA_DEBUG_DATA_WDMA_3_3]: 0x%08X %08X\n",
			  FDVT_DMA_DEBUG_DATA_WDMA_3_3,
			  (unsigned int)readl(fd->fd_base + FDVT_DMA_DEBUG_DATA_WDMA_3_3));
			aie_dev_info(fd->dev, "FDVT Check if AIE is on\n");
			aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)0x118,
				(unsigned int)readl(fd->fd_base + 0x118));
			aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)0x11c,
				(unsigned int)readl(fd->fd_base + 0x11c));
			aie_dev_info(fd->dev, "[0x%08X %08X]\n", (unsigned int)0x158,
				(unsigned int)readl(fd->fd_base + 0x158));
		}
	}
}

static void aie_free_dmabuf(struct mtk_aie_dev *fd, struct imem_buf_info *bufinfo)
{
	if (bufinfo->dmabuf) {
		dma_heap_buffer_free(bufinfo->dmabuf);
		bufinfo->dmabuf = NULL;
	}
}

static void aie_free_iova(struct mtk_aie_dev *fd, struct imem_buf_info *bufinfo)
{
	if (bufinfo->pa) {
		/*free iova*/
#ifdef AIE_DMA_BUF_UNLOCK_API
		dma_buf_unmap_attachment_unlocked(bufinfo->attach, bufinfo->sgt, DMA_BIDIRECTIONAL);
#else
		dma_buf_unmap_attachment(bufinfo->attach, bufinfo->sgt, DMA_BIDIRECTIONAL);
#endif
		dma_buf_detach(bufinfo->dmabuf, bufinfo->attach);
		bufinfo->pa = 0;
	}
}

static void aie_free_va(struct mtk_aie_dev *fd, struct imem_buf_info *bufinfo)
{
	if (bufinfo->va) {
#ifdef AIE_DMA_BUF_UNLOCK_API
		dma_buf_vunmap_unlocked(bufinfo->dmabuf, &bufinfo->map);
#else
		dma_buf_vunmap(bufinfo->dmabuf, &bufinfo->map);
#endif
		bufinfo->va = NULL;
	}
}

static struct dma_buf *aie_imem_sec_alloc(struct mtk_aie_dev *fd,
						u32 size,
						enum AIE_BUF_TYPE buf_type)
{
	struct dma_heap *dma_heap;
	struct dma_buf *my_dma_buf;

	switch (buf_type) {
	case SECURE_BUF:
		dma_heap = dma_heap_find("mtk_prot_region");
		break;
	case CACHED_BUF:
		dma_heap = dma_heap_find("mtk_mm");
		break;
	case UNCACHED_BUF:
		dma_heap = dma_heap_find("mtk_mm-uncached");
		break;
	default:
		dma_heap = dma_heap_find("mtk_mm-uncached");
		break;
	}

	if (!dma_heap) {
		aie_dev_info(fd->dev, "heap find fail\n");
		return NULL;
	}

	my_dma_buf = dma_heap_buffer_alloc(dma_heap, size, O_RDWR |
		O_CLOEXEC, DMA_HEAP_VALID_HEAP_FLAGS);
	if (IS_ERR(my_dma_buf)) {
		aie_dev_info(fd->dev, "buffer alloc fail\n");
		dma_heap_put(dma_heap);
		return NULL;
	}

	return my_dma_buf;
}

static unsigned long long aie_get_sec_iova(struct mtk_aie_dev *fd, struct dma_buf *my_dma_buf,
			  struct imem_buf_info *bufinfo)
{
	struct dma_buf_attachment *attach;
	unsigned long long iova = 0;
	struct sg_table *sgt;

	attach = dma_buf_attach(my_dma_buf, fd->smmu_dev);
	if (IS_ERR(attach)) {
		aie_dev_info(fd->dev, "attach fail, return\n");
		return 0;
	}
	bufinfo->attach = attach;

#ifdef AIE_DMA_BUF_UNLOCK_API
	sgt = dma_buf_map_attachment_unlocked(attach, DMA_BIDIRECTIONAL);
#else
	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
#endif
	if (IS_ERR(sgt)) {
		aie_dev_info(fd->dev, "map failed, detach and return\n");
		dma_buf_detach(my_dma_buf, attach);
		return 0;
	}
	bufinfo->sgt = sgt;

	iova = sg_dma_address(sgt->sgl);

	return iova;
}

static void *aie_get_va(struct mtk_aie_dev *fd, struct dma_buf *my_dma_buf,
			  struct imem_buf_info *bufinfo)
{
	void *buf_ptr = NULL;
	int ret = 0;

#ifdef AIE_DMA_BUF_UNLOCK_API
	ret = dma_buf_vmap_unlocked(my_dma_buf, &bufinfo->map);
#else
	ret = dma_buf_vmap(my_dma_buf, &bufinfo->map);
#endif
	if (ret) {
		aie_dev_info(fd->dev, "%s, map kernel va failed\n", __func__);
		return NULL;
	}

	buf_ptr = bufinfo->map.vaddr;

	if (!buf_ptr) {
		aie_dev_info(fd->dev, "map failed\n");
		return NULL;
	}
	return buf_ptr;
}
#if CHECK_SERVICE_IF_0
static int aie_imem_alloc(struct mtk_aie_dev *fd, u32 size,
			  struct imem_buf_info *bufinfo)
{
	struct device *dev = fd->dev;
	void *va;
	dma_addr_t dma_handle;

	va = dma_alloc_coherent(dev, size, &dma_handle, GFP_KERNEL);
	if (!va)
		return -ENOMEM;

	bufinfo->va = va;
	bufinfo->pa = dma_handle;
	bufinfo->size = size;

	aie_dev_info(fd->dev, "%s: vAddr(0x%p)(0x%llx), pAddr(0x%pad), size(%d)\n",
		__func__, va, (u64 *)va, &dma_handle, size);

	return 0;
}

static void aie_imem_free(struct mtk_aie_dev *fd, struct imem_buf_info *bufinfo)
{
	aie_dev_info(fd->dev, "%s: vAddr(0x%p)(0x%llx), pAddr(0x%p), size(%d)\n",
		__func__, bufinfo->va, (u64 *)bufinfo->va, bufinfo->pa,
		bufinfo->size);

	if (bufinfo->size != 0) {
		dma_free_coherent(fd->dev, bufinfo->size, bufinfo->va, bufinfo->pa);
		bufinfo->size = 0;
	}
}
#endif
static void aie_init_table(struct mtk_aie_dev *fd, u16 pym_width,
			   u16 pym_height)
{
	int i;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	pstv->img_width[pym2_start_loop] = pym_width / 4;
	pstv->img_height[pym2_start_loop] = pym_height / 4;

	pstv->img_width[pym1_start_loop] = pym_width / 2;
	pstv->img_height[pym1_start_loop] = pym_height / 2;

	pstv->img_width[pym0_start_loop] = pym_width;
	pstv->img_height[pym0_start_loop] = pym_height;

	for (i = 0; i < fd_loop_num; i++) {
		if (i != pym2_start_loop && i != pym1_start_loop &&
		    i != pym0_start_loop) {
			if (fd_out_stride2_in[i] == 1) {
				pstv->img_width[i] =
					pstv->stride2_out_width[i - 1];
				pstv->img_height[i] =
					pstv->stride2_out_height[i - 1];
			} else {
				pstv->img_width[i] = pstv->out_width[i - 1];
				pstv->img_height[i] = pstv->out_height[i - 1];
			}
		}

		if (fd_maxpool[i] == 1 && fd_stride[i] == 1) {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
				(2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] = (pstv->img_height[i] - 1) /
					      (2 * fd_maxpool[i]) + 1;
		} else {
			pstv->out_width[i] =
				(pstv->img_width[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
			pstv->out_height[i] =
				(pstv->img_height[i] - 1) /
					(fd_stride[i] + 2 * fd_maxpool[i]) + 1;
		}

		pstv->stride2_out_width[i] =
			((pstv->out_width[i] - 1) / 2 + 1) * out_2size[i];
		pstv->stride2_out_height[i] =
			((pstv->out_height[i] - 1) / 2 + 1) * out_2size[i];

		if (outlayer[i] == 1) {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i] * 2;
			pstv->out_stride[i] = round_up(
				pstv->out_xsize_plus_1[i] * anchor_en_num[i],
				16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * 2 * out_2size[i];
		} else {
			pstv->out_xsize_plus_1[i] =
				pstv->out_width[i] * out_ch_pack[i];
			pstv->out_stride[i] =
				round_up(pstv->out_xsize_plus_1[i], 16);
			pstv->out_xsize_plus_1_stride2[i] =
				((pstv->out_width[i] - 1) / 2 + 1) *
				out_ch_pack[i] * out_2size[i];
		}

		pstv->out_stride_stride2[i] =
			round_up(pstv->out_xsize_plus_1_stride2[i], 16);

		if (out_2size[i] == 1)
			pstv->out_ysize_plus_1_stride2[i] =
				(pstv->out_height[i] - 1) / 2 + 1;
		else
			pstv->out_ysize_plus_1_stride2[i] = pstv->out_height[i];

		if (fd_wdma_en[i][0]) {
			if (i == rpn2_loop_num || i == rpn1_loop_num ||
			    i == rpn0_loop_num) {
				pstv->fd_wdma_size[i][0] = AIE_ALIGN32(result_size);
			} else {
				pstv->fd_wdma_size[i][0] = AIE_ALIGN32(pstv->out_height[i] *
							   pstv->out_stride[i]);
			}
		}

		if (outlayer[i] == 1) {
			if (fd_wdma_en[i][1])
				pstv->fd_wdma_size[i][1] =
					pstv->fd_wdma_size[i][0];
			if (fd_wdma_en[i][2])
				pstv->fd_wdma_size[i][2] =
					pstv->fd_wdma_size[i][0];
			if (fd_wdma_en[i][3])
				pstv->fd_wdma_size[i][3] =
					pstv->fd_wdma_size[i][0];
		} else if (i == rpn2_loop_num || i == rpn1_loop_num ||
			   i == rpn0_loop_num) {
			pstv->fd_wdma_size[i][0] = AIE_ALIGN32(result_size);
		} else {
			if (fd_wdma_en[i][1])
				pstv->fd_wdma_size[i][1] = AIE_ALIGN32(pstv->out_height[i] *
							   pstv->out_stride[i]);
			if (fd_wdma_en[i][2])
				pstv->fd_wdma_size[i][2] =
					AIE_ALIGN32(pstv->out_ysize_plus_1_stride2[i] *
					pstv->out_stride_stride2[i]);
			if (fd_wdma_en[i][3])
				pstv->fd_wdma_size[i][3] =
					AIE_ALIGN32(pstv->out_ysize_plus_1_stride2[i] *
					pstv->out_stride_stride2[i]);
		}

		if (in_ch_pack[i] == 1)
			pstv->input_xsize_plus_1[i] =
				round_up(pstv->img_width[i], 8);
		else
			pstv->input_xsize_plus_1[i] =
				pstv->img_width[i] * in_ch_pack[i];
	}
}

static void aie_get_data_size(struct mtk_aie_dev *fd, u16 max_img_width,
			      u16 max_img_height)
{
	u8 i, j;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	fd->base_para->max_img_width = max_img_width;
	fd->base_para->max_img_height = max_img_height;
	fd->fd_fd_kernel_size = 0;
	fd->fd_attr_kernel_size = 0;
	fd->fd_attr_dma_max_size = 0;
	fd->fd_attr_dma_rst_max_size = 0;

	/* FDMODE Dram Buffer Size */
	fd->fd_rs_cfg_size = fd_rs_confi_size;
	fd->fd_fd_cfg_data_size = fd_fd_confi_size;
	fd->fd_fd_cfg_aligned_size = AIE_ALIGN32(fd_fd_confi_size);
	fd->fd_yuv2rgb_cfg_size = fd_yuv2rgb_confi_size;
	fd->fd_yuv2rgb_cfg_aligned_size = AIE_ALIGN32(fd_yuv2rgb_confi_size);

	/* ATTRMODE Dram Buffer Size */
	fd->attr_fd_cfg_data_size = attr_fd_confi_size;
	fd->attr_fd_cfg_aligned_size = AIE_ALIGN32(fd->attr_fd_cfg_data_size);
	fd->attr_yuv2rgb_cfg_data_size = attr_yuv2rgb_confi_size;
	fd->attr_yuv2rgb_cfg_aligned_size = AIE_ALIGN32(fd->attr_yuv2rgb_cfg_data_size);

	/* HW Output Buffer Size */
	fd->rs_pym_out_size[0] = AIE_ALIGN32(fd->base_para->max_pyramid_width *
				 fd->base_para->max_pyramid_height);
	fd->rs_pym_out_size[1] = AIE_ALIGN32(fd->rs_pym_out_size[0] / 4);
	fd->rs_pym_out_size[2] = AIE_ALIGN32(fd->rs_pym_out_size[0] / 16);

	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j])
				fd_ker_rdma_size_aligned[i][j]
						= AIE_ALIGN32(fd_ker_rdma_size[i][j]);
		}
	}

	for (i = 0; i < fd_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j])
				fd->fd_fd_kernel_size += fd_ker_rdma_size_aligned[i][j];
		}
	}

	/* ATTRMODE Dram Buffer Size */
	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++)
			attr_wdma_aligned_size[i][j] = AIE_ALIGN32(attr_wdma_size[i][j]);
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				if ((i == age_out_rgs || i == gender_out_rgs ||
				     i == indian_out_rgs || i == race_out_rgs) &&
				    (j == 0)) {
					fd->fd_attr_dma_rst_max_size +=
						ATTR_OUT_SIZE * MAX_ENQUE_FRAME_NUM;
				} else {
					fd->fd_attr_dma_max_size += attr_wdma_aligned_size[i][j];
				}
			}
		}
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++)
			attr_ker_rdma_aligned_size[i][j] = AIE_ALIGN32(attr_ker_rdma_size[i][j]);
	}

	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < kernel_RDMA_RA_num; j++)
			fd->fd_attr_kernel_size += attr_ker_rdma_aligned_size[i][j];
	}

}

static int aie_alloc_output_buf(struct mtk_aie_dev *fd)
{
	int ret = 0;
	u32 alloc_size = 0;
	int i, j, pa_off = 0, va_off = 0;
	struct dma_buf *ret_buf = NULL;
	unsigned long long iova = 0;
	void *va = NULL;

	for (i = 0; i < PYM_NUM; i++)
		alloc_size += fd->rs_pym_out_size[i] * 3;

	if (g_user_param.is_secure) {
		ret_buf = aie_imem_sec_alloc(fd, alloc_size, SECURE_BUF);
		if (!ret_buf)
			return -1;

		mtk_dma_buf_set_name(ret_buf, "rs_pym_out_buf");
		fd->rs_output_hw.size = alloc_size;
		fd->rs_output_hw.dmabuf = ret_buf;
		iova = aie_get_sec_iova(fd, ret_buf, &fd->rs_output_hw);
		if (!iova)
			return -1;

		fd->rs_output_hw.pa = iova;
	} else {
		ret_buf = aie_imem_sec_alloc(fd, alloc_size, CACHED_BUF);
		if (!ret_buf)
			return -1;

		mtk_dma_buf_set_name(ret_buf, "rs_pym_out_buf");
		fd->rs_output_hw.size = alloc_size;
		fd->rs_output_hw.dmabuf = ret_buf;
		iova = aie_get_sec_iova(fd, ret_buf, &fd->rs_output_hw);
		if (!iova)
			return -1;

		fd->rs_output_hw.pa = iova;
		va = aie_get_va(fd, ret_buf, &fd->rs_output_hw);
		if (!va)
			return -1;

		fd->rs_output_hw.va = va;
	}

	for (i = 0; i < PYM_NUM; i++) {
		for (j = 0; j < COLOR_NUM; j++) {
			fd->base_para->rs_pym_rst_pa[i][j] =
				fd->rs_output_hw.pa + pa_off;
			pa_off += fd->rs_pym_out_size[i];

			fd->base_para->rs_pym_rst_va[i][j] =
				fd->rs_output_hw.va + va_off;
			va_off += fd->rs_pym_out_size[i];
		}
	}

	return ret;
}

static int aie_alloc_fddma_buf(struct mtk_aie_dev *fd)
{
	u32 alloc_size;
	struct dma_buf *ret_buf = NULL;
	unsigned long long iova = 0;
	void *va = NULL;


	alloc_size = FD_DMA_BUFFER_SIZE;
	ret_buf = aie_imem_sec_alloc(fd, alloc_size, CACHED_BUF);
	if (!ret_buf)
		return -1;

	mtk_dma_buf_set_name(ret_buf, "fd_dma_buf");
	fd->fd_dma_hw.dmabuf = ret_buf;
	fd->fd_dma_hw.size = alloc_size;
	iova = aie_get_sec_iova(fd, ret_buf, &fd->fd_dma_hw);
	if (!iova)
		return -1;

	fd->fd_dma_hw.pa = iova;
	va = aie_get_va(fd, ret_buf, &fd->fd_dma_hw);
	if (!va)
		return -1;

	fd->fd_dma_hw.va = va;

	alloc_size = fd->fd_attr_dma_max_size;
	ret_buf = aie_imem_sec_alloc(fd, alloc_size, CACHED_BUF);
	if (!ret_buf)
		return -1;

	mtk_dma_buf_set_name(ret_buf, "attr_dma_buf");
	fd->fd_attr_dma_hw.dmabuf = ret_buf;
	fd->fd_attr_dma_hw.size = alloc_size;
	iova = aie_get_sec_iova(fd, ret_buf, &fd->fd_attr_dma_hw);
	if (!iova)
		return -1;

	fd->fd_attr_dma_hw.pa = iova;
	va = aie_get_va(fd, ret_buf, &fd->fd_attr_dma_hw);
	if (!va)
		return -1;

	fd->fd_attr_dma_hw.va = va;
	return 0;
}

static void aie_arrange_fddma_buf(struct mtk_aie_dev *fd)
{
	// dma_addr_t currentPA;
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	/* fd loop0 wdma arrange */
	fd->dma_para->fd_out_hw_pa[0][0] = fd->fd_dma_hw.pa + wdma_offset_0_0;

	/* fd loop1 wdma arrange */
	fd->dma_para->fd_out_hw_pa[1][0] = fd->fd_dma_hw.pa + wdma_offset_1_0;
	fd->dma_para->fd_out_hw_pa[1][2] = fd->fd_dma_hw.pa + wdma_offset_1_2;

	/* fd loop2 wdma arrange */
	fd->dma_para->fd_out_hw_pa[2][0] = fd->fd_dma_hw.pa + wdma_offset_2_0;
	fd->dma_para->fd_out_hw_pa[2][2] = fd->fd_dma_hw.pa + wdma_offset_2_2;

	/* fd loop3 wdma arrange */
	fd->dma_para->fd_out_hw_pa[3][0] = fd->fd_dma_hw.pa + wdma_offset_3_0;

	/* fd loop4 wdma arrange */
	fd->dma_para->fd_out_hw_pa[4][0] = fd->fd_dma_hw.pa + wdma_offset_4_0;
	fd->dma_para->fd_out_hw_pa[4][1] = fd->fd_dma_hw.pa + wdma_offset_4_1;
	fd->dma_para->fd_out_hw_pa[4][2] = fd->fd_dma_hw.pa + wdma_offset_4_2;
	fd->dma_para->fd_out_hw_pa[4][3] = fd->fd_dma_hw.pa + wdma_offset_4_3;

	/* fd loop5 wdma arrange */
	fd->dma_para->fd_out_hw_pa[5][0] = fd->fd_dma_hw.pa + wdma_offset_5_0;
	fd->dma_para->fd_out_hw_pa[5][1] = fd->fd_dma_hw.pa + wdma_offset_5_1;
	fd->dma_para->fd_out_hw_pa[5][2] = fd->fd_dma_hw.pa + wdma_offset_5_2;
	fd->dma_para->fd_out_hw_pa[5][3] = fd->fd_dma_hw.pa + wdma_offset_5_3;

	/* fd loop6 wdma arrange */
	fd->dma_para->fd_out_hw_pa[6][0] = fd->fd_dma_hw.pa + wdma_offset_6_0;

	/* fd loop7 wdma arrange */
	fd->dma_para->fd_out_hw_pa[7][0] = fd->fd_dma_hw.pa + wdma_offset_7_0;
	fd->dma_para->fd_out_hw_pa[7][2] = fd->fd_dma_hw.pa + wdma_offset_7_2;

	/* fd loop8 wdma arrange */
	fd->dma_para->fd_out_hw_pa[8][0] = fd->fd_dma_hw.pa + wdma_offset_8_0;
	fd->dma_para->fd_out_hw_pa[8][1] = fd->fd_dma_hw.pa + wdma_offset_8_1;

	/* fd loop9 wdma arrange */
	fd->dma_para->fd_out_hw_pa[9][0] = fd->fd_dma_hw.pa + wdma_offset_9_0;

	/* fd loop10 wdma arrange */
	fd->dma_para->fd_out_hw_pa[10][0] = fd->fd_dma_hw.pa + wdma_offset_10_0;
	fd->dma_para->fd_out_hw_pa[10][2] = fd->fd_dma_hw.pa + wdma_offset_10_2;

	/* fd loop11 wdma arrange */
	fd->dma_para->fd_out_hw_pa[11][0] = fd->fd_dma_hw.pa + wdma_offset_11_0;
	fd->dma_para->fd_out_hw_pa[11][1] = fd->fd_dma_hw.pa + wdma_offset_11_1;

	/* fd loop12 wdma arrange */
	fd->dma_para->fd_out_hw_pa[12][0] = fd->fd_dma_hw.pa + wdma_offset_12_0;

	/* fd loop13 wdma arrange */
	fd->dma_para->fd_out_hw_pa[13][0] = fd->fd_dma_hw.pa + wdma_offset_13_0;

	/* fd loop14 wdma arrange */
	fd->dma_para->fd_out_hw_pa[14][0] = fd->fd_dma_hw.pa + wdma_offset_14_0;

	/* fd loop15 wdma arrange */
	fd->dma_para->fd_out_hw_pa[15][0] = fd->fd_dma_hw.pa + wdma_offset_15_0;

	/* fd loop16 wdma arrange */
	fd->dma_para->fd_out_hw_pa[16][0] = fd->fd_dma_hw.pa + wdma_offset_16_0;

	/* fd loop17 wdma arrange */
	fd->dma_para->fd_out_hw_pa[17][0] = fd->fd_dma_hw.pa + wdma_offset_17_0;

	/* fd loop18 wdma arrange */
	fd->dma_para->fd_out_hw_pa[18][0] = fd->fd_dma_hw.pa + wdma_offset_18_0;
	fd->dma_para->fd_out_hw_pa[18][1] = fd->fd_dma_hw.pa + wdma_offset_18_1;

	/* fd loop19 wdma arrange */
	fd->dma_para->fd_out_hw_pa[19][0] = fd->fd_dma_hw.pa + wdma_offset_19_0;
	fd->dma_para->fd_out_hw_pa[19][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + 1 * pstv->out_xsize_plus_1[19];

	/* fd loop20 wdma arrange */
	fd->dma_para->fd_out_hw_pa[20][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + 2 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[20][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + 3 * pstv->out_xsize_plus_1[20];

	/* fd loop21 wdma arrange */
	fd->dma_para->fd_out_hw_pa[21][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + 4 * pstv->out_xsize_plus_1[21];

	/* fd loop22 wdma arrange */
	fd->dma_para->fd_out_hw_pa[22][0] = fd->fd_dma_hw.pa + wdma_offset_22_0;
	fd->dma_para->fd_out_hw_pa[22][1] = fd->fd_dma_hw.pa + wdma_offset_22_1;
	fd->dma_para->fd_out_hw_pa[22][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + 1 * pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[22][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + 1 * pstv->out_xsize_plus_1[22];

	/* fd loop23 wdma arrange */
	fd->dma_para->fd_out_hw_pa[23][0] =
		fd->dma_para->fd_out_hw_pa[22][0] + 2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][1] =
		fd->dma_para->fd_out_hw_pa[22][1] + 2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + 3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + 3 * pstv->out_xsize_plus_1[23];

	/* fd loop24 wdma arrange */
	fd->dma_para->fd_out_hw_pa[24][0] =
		fd->dma_para->fd_out_hw_pa[22][0] + 4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[24][1] =
		fd->dma_para->fd_out_hw_pa[22][1] + 4 * pstv->out_xsize_plus_1[24];

	/* fd loop25 wdma arrange */
	fd->dma_para->fd_out_hw_pa[25][0] = fd->fd_dma_hw.pa + wdma_offset_25_0;
	fd->dma_para->fd_out_hw_pa[25][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + 1 * pstv->out_xsize_plus_1[25];

	/* fd loop26 wdma arrange */
	fd->dma_para->fd_out_hw_pa[26][0] =
		fd->dma_para->fd_out_hw_pa[25][0] + 2 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[26][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + 3 * pstv->out_xsize_plus_1[26];

	/* fd loop27 wdma arrange */
	fd->dma_para->fd_out_hw_pa[27][0] =
		fd->dma_para->fd_out_hw_pa[25][0] + 4 * pstv->out_xsize_plus_1[27];

	/* fd loop29 wdma arrange */
	fd->dma_para->fd_out_hw_pa[29][0] = fd->fd_dma_hw.pa + wdma_offset_29_0;

	/* fd loop30 wdma arrange */
	fd->dma_para->fd_out_hw_pa[30][0] = fd->fd_dma_hw.pa + wdma_offset_30_0;
	fd->dma_para->fd_out_hw_pa[30][2] = fd->fd_dma_hw.pa + wdma_offset_30_2;

	/* fd loop31 wdma arrange */
	fd->dma_para->fd_out_hw_pa[31][0] = fd->fd_dma_hw.pa + wdma_offset_31_0;
	fd->dma_para->fd_out_hw_pa[31][2] = fd->fd_dma_hw.pa + wdma_offset_31_2;

	/* fd loop32 wdma arrange */
	fd->dma_para->fd_out_hw_pa[32][0] = fd->fd_dma_hw.pa + wdma_offset_32_0;

	/* fd loop33 wdma arrange */
	fd->dma_para->fd_out_hw_pa[33][0] = fd->fd_dma_hw.pa + wdma_offset_33_0;
	fd->dma_para->fd_out_hw_pa[33][1] = fd->fd_dma_hw.pa + wdma_offset_33_1;
	fd->dma_para->fd_out_hw_pa[33][2] = fd->fd_dma_hw.pa + wdma_offset_33_2;
	fd->dma_para->fd_out_hw_pa[33][3] = fd->fd_dma_hw.pa + wdma_offset_33_3;

	/* fd loop34 wdma arrange */
	fd->dma_para->fd_out_hw_pa[34][0] = fd->fd_dma_hw.pa + wdma_offset_34_0;
	fd->dma_para->fd_out_hw_pa[34][1] = fd->fd_dma_hw.pa + wdma_offset_34_1;
	fd->dma_para->fd_out_hw_pa[34][2] = fd->fd_dma_hw.pa + wdma_offset_34_2;
	fd->dma_para->fd_out_hw_pa[34][3] = fd->fd_dma_hw.pa + wdma_offset_34_3;

	/* fd loop35 wdma arrange */
	fd->dma_para->fd_out_hw_pa[35][0] = fd->fd_dma_hw.pa + wdma_offset_35_0;

	/* fd loop36 wdma arrange */
	fd->dma_para->fd_out_hw_pa[36][0] = fd->fd_dma_hw.pa + wdma_offset_36_0;
	fd->dma_para->fd_out_hw_pa[36][2] = fd->fd_dma_hw.pa + wdma_offset_36_2;

	/* fd loop37 wdma arrange */
	fd->dma_para->fd_out_hw_pa[37][0] = fd->fd_dma_hw.pa + wdma_offset_37_0;
	fd->dma_para->fd_out_hw_pa[37][1] = fd->fd_dma_hw.pa + wdma_offset_37_1;

	/* fd loop38 wdma arrange */
	fd->dma_para->fd_out_hw_pa[38][0] = fd->fd_dma_hw.pa + wdma_offset_38_0;

	/* fd loop39 wdma arrange */
	fd->dma_para->fd_out_hw_pa[39][0] = fd->fd_dma_hw.pa + wdma_offset_39_0;
	fd->dma_para->fd_out_hw_pa[39][2] = fd->fd_dma_hw.pa + wdma_offset_39_2;

	/* fd loop40 wdma arrange */
	fd->dma_para->fd_out_hw_pa[40][0] = fd->fd_dma_hw.pa + wdma_offset_40_0;
	fd->dma_para->fd_out_hw_pa[40][1] = fd->fd_dma_hw.pa + wdma_offset_40_1;

	/* fd loop41 wdma arrange */
	fd->dma_para->fd_out_hw_pa[41][0] = fd->fd_dma_hw.pa + wdma_offset_41_0;

	/* fd loop42 wdma arrange */
	fd->dma_para->fd_out_hw_pa[42][0] = fd->fd_dma_hw.pa + wdma_offset_42_0;

	/* fd loop43 wdma arrange */
	fd->dma_para->fd_out_hw_pa[43][0] = fd->fd_dma_hw.pa + wdma_offset_43_0;

	/* fd loop44 wdma arrange */
	fd->dma_para->fd_out_hw_pa[44][0] = fd->fd_dma_hw.pa + wdma_offset_44_0;

	/* fd loop45 wdma arrange */
	fd->dma_para->fd_out_hw_pa[45][0] = fd->fd_dma_hw.pa + wdma_offset_45_0;

	/* fd loop46 wdma arrange */
	fd->dma_para->fd_out_hw_pa[46][0] = fd->fd_dma_hw.pa + wdma_offset_46_0;

	/* fd loop47 wdma arrange */
	fd->dma_para->fd_out_hw_pa[47][0] = fd->fd_dma_hw.pa + wdma_offset_47_0;
	fd->dma_para->fd_out_hw_pa[47][1] = fd->fd_dma_hw.pa + wdma_offset_47_1;

	/* fd loop48 wdma arrange */
	fd->dma_para->fd_out_hw_pa[48][0] = fd->fd_dma_hw.pa + wdma_offset_48_0;
	fd->dma_para->fd_out_hw_pa[48][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + 1 * pstv->out_xsize_plus_1[48];

	/* fd loop49 wdma arrange */
	fd->dma_para->fd_out_hw_pa[49][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + 2 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[49][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + 3 * pstv->out_xsize_plus_1[49];

	/* fd loop50 wdma arrange */
	fd->dma_para->fd_out_hw_pa[50][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + 4 * pstv->out_xsize_plus_1[50];

	/* fd loop51 wdma arrange */
	fd->dma_para->fd_out_hw_pa[51][0] = fd->fd_dma_hw.pa + wdma_offset_51_0;
	fd->dma_para->fd_out_hw_pa[51][1] = fd->fd_dma_hw.pa + wdma_offset_51_1;
	fd->dma_para->fd_out_hw_pa[51][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + 1 * pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[51][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + 1 * pstv->out_xsize_plus_1[51];

	/* fd loop52 wdma arrange */
	fd->dma_para->fd_out_hw_pa[52][0] =
		fd->dma_para->fd_out_hw_pa[51][0] + 2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][1] =
		fd->dma_para->fd_out_hw_pa[51][1] + 2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + 3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + 3 * pstv->out_xsize_plus_1[52];

	/* fd loop53 wdma arrange */
	fd->dma_para->fd_out_hw_pa[53][0] =
		fd->dma_para->fd_out_hw_pa[51][0] + 4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[53][1] =
		fd->dma_para->fd_out_hw_pa[51][1] + 4 * pstv->out_xsize_plus_1[53];

	/* fd loop54 wdma arrange */
	fd->dma_para->fd_out_hw_pa[54][0] = fd->fd_dma_hw.pa + wdma_offset_54_0;
	fd->dma_para->fd_out_hw_pa[54][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + 1 * pstv->out_xsize_plus_1[54];

	/* fd loop55 wdma arrange */
	fd->dma_para->fd_out_hw_pa[55][0] =
		fd->dma_para->fd_out_hw_pa[54][0] + 2 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[55][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + 3 * pstv->out_xsize_plus_1[55];

	/* fd loop56 wdma arrange */
	fd->dma_para->fd_out_hw_pa[56][0] =
		fd->dma_para->fd_out_hw_pa[54][0] + 4 * pstv->out_xsize_plus_1[56];

	/* fd loop58 wdma arrange */
	fd->dma_para->fd_out_hw_pa[58][0] = fd->fd_dma_hw.pa + wdma_offset_58_0;

	/* fd loop59 wdma arrange */
	fd->dma_para->fd_out_hw_pa[59][0] = fd->fd_dma_hw.pa + wdma_offset_59_0;
	fd->dma_para->fd_out_hw_pa[59][2] = fd->fd_dma_hw.pa + wdma_offset_59_2;

	/* fd loop60 wdma arrange */
	fd->dma_para->fd_out_hw_pa[60][0] = fd->fd_dma_hw.pa + wdma_offset_60_0;
	fd->dma_para->fd_out_hw_pa[60][2] = fd->fd_dma_hw.pa + wdma_offset_60_2;

	/* fd loop61 wdma arrange */
	fd->dma_para->fd_out_hw_pa[61][0] = fd->fd_dma_hw.pa + wdma_offset_61_0;

	/* fd loop62 wdma arrange */
	fd->dma_para->fd_out_hw_pa[62][0] = fd->fd_dma_hw.pa + wdma_offset_62_0;
	fd->dma_para->fd_out_hw_pa[62][1] = fd->fd_dma_hw.pa + wdma_offset_62_1;
	fd->dma_para->fd_out_hw_pa[62][2] = fd->fd_dma_hw.pa + wdma_offset_62_2;
	fd->dma_para->fd_out_hw_pa[62][3] = fd->fd_dma_hw.pa + wdma_offset_62_3;

	/* fd loop63 wdma arrange */
	fd->dma_para->fd_out_hw_pa[63][0] = fd->fd_dma_hw.pa + wdma_offset_63_0;
	fd->dma_para->fd_out_hw_pa[63][1] = fd->fd_dma_hw.pa + wdma_offset_63_1;
	fd->dma_para->fd_out_hw_pa[63][2] = fd->fd_dma_hw.pa + wdma_offset_63_2;
	fd->dma_para->fd_out_hw_pa[63][3] = fd->fd_dma_hw.pa + wdma_offset_63_3;

	/* fd loop64 wdma arrange */
	fd->dma_para->fd_out_hw_pa[64][0] = fd->fd_dma_hw.pa + wdma_offset_64_0;

	/* fd loop65 wdma arrange */
	fd->dma_para->fd_out_hw_pa[65][0] = fd->fd_dma_hw.pa + wdma_offset_65_0;
	fd->dma_para->fd_out_hw_pa[65][2] = fd->fd_dma_hw.pa + wdma_offset_65_2;

	/* fd loop66 wdma arrange */
	fd->dma_para->fd_out_hw_pa[66][0] = fd->fd_dma_hw.pa + wdma_offset_66_0;
	fd->dma_para->fd_out_hw_pa[66][1] = fd->fd_dma_hw.pa + wdma_offset_66_1;

	/* fd loop67 wdma arrange */
	fd->dma_para->fd_out_hw_pa[67][0] = fd->fd_dma_hw.pa + wdma_offset_67_0;

	/* fd loop68 wdma arrange */
	fd->dma_para->fd_out_hw_pa[68][0] = fd->fd_dma_hw.pa + wdma_offset_68_0;
	fd->dma_para->fd_out_hw_pa[68][2] = fd->fd_dma_hw.pa + wdma_offset_68_2;

	/* fd loop69 wdma arrange */
	fd->dma_para->fd_out_hw_pa[69][0] = fd->fd_dma_hw.pa + wdma_offset_69_0;
	fd->dma_para->fd_out_hw_pa[69][1] = fd->fd_dma_hw.pa + wdma_offset_69_1;

	/* fd loop70 wdma arrange */
	fd->dma_para->fd_out_hw_pa[70][0] = fd->fd_dma_hw.pa + wdma_offset_70_0;

	/* fd loop71 wdma arrange */
	fd->dma_para->fd_out_hw_pa[71][0] = fd->fd_dma_hw.pa + wdma_offset_71_0;

	/* fd loop72 wdma arrange */
	fd->dma_para->fd_out_hw_pa[72][0] = fd->fd_dma_hw.pa + wdma_offset_72_0;

	/* fd loop73 wdma arrange */
	fd->dma_para->fd_out_hw_pa[73][0] = fd->fd_dma_hw.pa + wdma_offset_73_0;

	/* fd loop74 wdma arrange */
	fd->dma_para->fd_out_hw_pa[74][0] = fd->fd_dma_hw.pa + wdma_offset_74_0;

	/* fd loop75 wdma arrange */
	fd->dma_para->fd_out_hw_pa[75][0] = fd->fd_dma_hw.pa + wdma_offset_75_0;

	/* fd loop76 wdma arrange */
	fd->dma_para->fd_out_hw_pa[76][0] = fd->fd_dma_hw.pa + wdma_offset_76_0;
	fd->dma_para->fd_out_hw_pa[76][1] = fd->fd_dma_hw.pa + wdma_offset_76_1;

	/* fd loop77 wdma arrange */
	fd->dma_para->fd_out_hw_pa[77][0] = fd->fd_dma_hw.pa + wdma_offset_77_0;
	fd->dma_para->fd_out_hw_pa[77][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + 1 * pstv->out_xsize_plus_1[77];

	/* fd loop78 wdma arrange */
	fd->dma_para->fd_out_hw_pa[78][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + 2 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[78][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + 3 * pstv->out_xsize_plus_1[78];

	/* fd loop79 wdma arrange */
	fd->dma_para->fd_out_hw_pa[79][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + 4 * pstv->out_xsize_plus_1[79];

	/* fd loop80 wdma arrange */
	fd->dma_para->fd_out_hw_pa[80][0] = fd->fd_dma_hw.pa + wdma_offset_80_0;
	fd->dma_para->fd_out_hw_pa[80][1] = fd->fd_dma_hw.pa + wdma_offset_80_1;
	fd->dma_para->fd_out_hw_pa[80][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + 1 * pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[80][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + 1 * pstv->out_xsize_plus_1[80];

	/* fd loop81 wdma arrange */
	fd->dma_para->fd_out_hw_pa[81][0] =
		fd->dma_para->fd_out_hw_pa[80][0] + 2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][1] =
		fd->dma_para->fd_out_hw_pa[80][1] + 2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + 3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + 3 * pstv->out_xsize_plus_1[81];

	/* fd loop82 wdma arrange */
	fd->dma_para->fd_out_hw_pa[82][0] =
		fd->dma_para->fd_out_hw_pa[80][0] + 4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[82][1] =
		fd->dma_para->fd_out_hw_pa[80][1] + 4 * pstv->out_xsize_plus_1[82];

	/* fd loop83 wdma arrange */
	fd->dma_para->fd_out_hw_pa[83][0] = fd->fd_dma_hw.pa + wdma_offset_83_0;
	fd->dma_para->fd_out_hw_pa[83][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + 1 * pstv->out_xsize_plus_1[83];

	/* fd loop84 wdma arrange */
	fd->dma_para->fd_out_hw_pa[84][0] =
		fd->dma_para->fd_out_hw_pa[83][0] + 2 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[84][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + 3 * pstv->out_xsize_plus_1[84];

	/* fd loop85 wdma arrange */
	fd->dma_para->fd_out_hw_pa[85][0] =
		fd->dma_para->fd_out_hw_pa[83][0] + 4 * pstv->out_xsize_plus_1[85];
}

static void aie_arrange_attrdma_buf(struct mtk_aie_dev *fd)
{
	void *currentVA = NULL;
	dma_addr_t currentPA;
	u8 i, j;

	currentPA = fd->fd_attr_dma_hw.pa;
	currentVA = fd->fd_attr_dma_hw.va;

	/* attribute mode */
	for (i = 0; i < attr_loop_num; i++) {
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				fd->dma_para->attr_out_hw_pa[i][j] = currentPA;
				fd->dma_para->attr_out_hw_va[i][j] = currentVA;
				currentPA += attr_wdma_aligned_size[i][j];
				currentVA += attr_wdma_aligned_size[i][j];
			}
		}
	}
}

static void aie_update_fddma_buf(struct mtk_aie_dev *fd)
{
	struct aie_static_info *pstv;

	pstv = &fd->st_info;

	/* fd loop19 wdma arrange */
	fd->dma_para->fd_out_hw_pa[19][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + 1 * pstv->out_xsize_plus_1[19];

	/* fd loop20 wdma arrange */
	fd->dma_para->fd_out_hw_pa[20][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + 2 * pstv->out_xsize_plus_1[20];
	fd->dma_para->fd_out_hw_pa[20][1] =
		fd->dma_para->fd_out_hw_pa[19][0] + 3 * pstv->out_xsize_plus_1[20];

	/* fd loop21 wdma arrange */
	fd->dma_para->fd_out_hw_pa[21][0] =
		fd->dma_para->fd_out_hw_pa[19][0] + 4 * pstv->out_xsize_plus_1[21];

	/* fd loop22 wdma arrange */
	fd->dma_para->fd_out_hw_pa[22][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + 1 * pstv->out_xsize_plus_1[22];
	fd->dma_para->fd_out_hw_pa[22][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + 1 * pstv->out_xsize_plus_1[22];

	/* fd loop23 wdma arrange */
	fd->dma_para->fd_out_hw_pa[23][0] =
		fd->dma_para->fd_out_hw_pa[22][0] + 2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][1] =
		fd->dma_para->fd_out_hw_pa[22][1] + 2 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][2] =
		fd->dma_para->fd_out_hw_pa[22][0] + 3 * pstv->out_xsize_plus_1[23];
	fd->dma_para->fd_out_hw_pa[23][3] =
		fd->dma_para->fd_out_hw_pa[22][1] + 3 * pstv->out_xsize_plus_1[23];

	/* fd loop24 wdma arrange */
	fd->dma_para->fd_out_hw_pa[24][0] =
		fd->dma_para->fd_out_hw_pa[22][0] + 4 * pstv->out_xsize_plus_1[24];
	fd->dma_para->fd_out_hw_pa[24][1] =
		fd->dma_para->fd_out_hw_pa[22][1] + 4 * pstv->out_xsize_plus_1[24];

	/* fd loop25 wdma arrange */
	fd->dma_para->fd_out_hw_pa[25][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + 1 * pstv->out_xsize_plus_1[25];

	/* fd loop26 wdma arrange */
	fd->dma_para->fd_out_hw_pa[26][0] =
		fd->dma_para->fd_out_hw_pa[25][0] + 2 * pstv->out_xsize_plus_1[26];
	fd->dma_para->fd_out_hw_pa[26][1] =
		fd->dma_para->fd_out_hw_pa[25][0] + 3 * pstv->out_xsize_plus_1[26];

	/* fd loop27 wdma arrange */
	fd->dma_para->fd_out_hw_pa[27][0] =
		fd->dma_para->fd_out_hw_pa[25][0] + 4 * pstv->out_xsize_plus_1[27];

	/* fd loop48 wdma arrange */
	fd->dma_para->fd_out_hw_pa[48][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + 1 * pstv->out_xsize_plus_1[48];

	/* fd loop49 wdma arrange */
	fd->dma_para->fd_out_hw_pa[49][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + 2 * pstv->out_xsize_plus_1[49];
	fd->dma_para->fd_out_hw_pa[49][1] =
		fd->dma_para->fd_out_hw_pa[48][0] + 3 * pstv->out_xsize_plus_1[49];

	/* fd loop50 wdma arrange */
	fd->dma_para->fd_out_hw_pa[50][0] =
		fd->dma_para->fd_out_hw_pa[48][0] + 4 * pstv->out_xsize_plus_1[50];

	/* fd loop51 wdma arrange */
	fd->dma_para->fd_out_hw_pa[51][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + 1 * pstv->out_xsize_plus_1[51];
	fd->dma_para->fd_out_hw_pa[51][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + 1 * pstv->out_xsize_plus_1[51];

	/* fd loop52 wdma arrange */
	fd->dma_para->fd_out_hw_pa[52][0] =
		fd->dma_para->fd_out_hw_pa[51][0] + 2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][1] =
		fd->dma_para->fd_out_hw_pa[51][1] + 2 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][2] =
		fd->dma_para->fd_out_hw_pa[51][0] + 3 * pstv->out_xsize_plus_1[52];
	fd->dma_para->fd_out_hw_pa[52][3] =
		fd->dma_para->fd_out_hw_pa[51][1] + 3 * pstv->out_xsize_plus_1[52];

	/* fd loop53 wdma arrange */
	fd->dma_para->fd_out_hw_pa[53][0] =
		fd->dma_para->fd_out_hw_pa[51][0] + 4 * pstv->out_xsize_plus_1[53];
	fd->dma_para->fd_out_hw_pa[53][1] =
		fd->dma_para->fd_out_hw_pa[51][1] + 4 * pstv->out_xsize_plus_1[53];

	/* fd loop54 wdma arrange */
	fd->dma_para->fd_out_hw_pa[54][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + 1 * pstv->out_xsize_plus_1[54];

	/* fd loop55 wdma arrange */
	fd->dma_para->fd_out_hw_pa[55][0] =
		fd->dma_para->fd_out_hw_pa[54][0] + 2 * pstv->out_xsize_plus_1[55];
	fd->dma_para->fd_out_hw_pa[55][1] =
		fd->dma_para->fd_out_hw_pa[54][0] + 3 * pstv->out_xsize_plus_1[55];

	/* fd loop56 wdma arrange */
	fd->dma_para->fd_out_hw_pa[56][0] =
		fd->dma_para->fd_out_hw_pa[54][0] + 4 * pstv->out_xsize_plus_1[56];

	/* fd loop77 wdma arrange */
	fd->dma_para->fd_out_hw_pa[77][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + 1 * pstv->out_xsize_plus_1[77];

	/* fd loop78 wdma arrange */
	fd->dma_para->fd_out_hw_pa[78][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + 2 * pstv->out_xsize_plus_1[78];
	fd->dma_para->fd_out_hw_pa[78][1] =
		fd->dma_para->fd_out_hw_pa[77][0] + 3 * pstv->out_xsize_plus_1[78];

	/* fd loop79 wdma arrange */
	fd->dma_para->fd_out_hw_pa[79][0] =
		fd->dma_para->fd_out_hw_pa[77][0] + 4 * pstv->out_xsize_plus_1[79];

	/* fd loop80 wdma arrange */
	fd->dma_para->fd_out_hw_pa[80][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + 1 * pstv->out_xsize_plus_1[80];
	fd->dma_para->fd_out_hw_pa[80][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + 1 * pstv->out_xsize_plus_1[80];

	/* fd loop81 wdma arrange */
	fd->dma_para->fd_out_hw_pa[81][0] =
		fd->dma_para->fd_out_hw_pa[80][0] + 2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][1] =
		fd->dma_para->fd_out_hw_pa[80][1] + 2 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][2] =
		fd->dma_para->fd_out_hw_pa[80][0] + 3 * pstv->out_xsize_plus_1[81];
	fd->dma_para->fd_out_hw_pa[81][3] =
		fd->dma_para->fd_out_hw_pa[80][1] + 3 * pstv->out_xsize_plus_1[81];

	/* fd loop82 wdma arrange */
	fd->dma_para->fd_out_hw_pa[82][0] =
		fd->dma_para->fd_out_hw_pa[80][0] + 4 * pstv->out_xsize_plus_1[82];
	fd->dma_para->fd_out_hw_pa[82][1] =
		fd->dma_para->fd_out_hw_pa[80][1] + 4 * pstv->out_xsize_plus_1[82];

	/* fd loop83 wdma arrange */
	fd->dma_para->fd_out_hw_pa[83][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + 1 * pstv->out_xsize_plus_1[83];

	/* fd loop84 wdma arrange */
	fd->dma_para->fd_out_hw_pa[84][0] =
		fd->dma_para->fd_out_hw_pa[83][0] + 2 * pstv->out_xsize_plus_1[84];
	fd->dma_para->fd_out_hw_pa[84][1] =
		fd->dma_para->fd_out_hw_pa[83][0] + 3 * pstv->out_xsize_plus_1[84];

	/* fd loop85 wdma arrange */
	fd->dma_para->fd_out_hw_pa[85][0] =
		fd->dma_para->fd_out_hw_pa[83][0] + 4 * pstv->out_xsize_plus_1[85];
}


static void aie_free_sec_buf(struct mtk_aie_dev *fd)
{
	aie_free_iova(fd, &fd->rs_output_hw);
	aie_free_dmabuf(fd, &fd->rs_output_hw);
}

static void aie_free_output_buf(struct mtk_aie_dev *fd)
{
	aie_free_iova(fd, &fd->rs_output_hw);
	aie_free_va(fd, &fd->rs_output_hw);
	aie_free_dmabuf(fd, &fd->rs_output_hw);
}

static void aie_free_fddma_buf(struct mtk_aie_dev *fd)
{
	aie_free_iova(fd, &fd->fd_dma_hw);
	aie_free_va(fd, &fd->fd_dma_hw);
	aie_free_dmabuf(fd, &fd->fd_dma_hw);

	aie_free_iova(fd, &fd->fd_attr_dma_hw);
	aie_free_va(fd, &fd->fd_attr_dma_hw);
	aie_free_dmabuf(fd, &fd->fd_attr_dma_hw);
}

#if CHECK_SERVICE_0
static int aie_copy_fw(struct mtk_aie_dev *fd, const char *name, void *buf,
		       unsigned int size)
{
	int ret = 0;
	const struct firmware *fw = NULL;

	ret = request_firmware(&fw, name, fd->dev);
	if (ret) {
		aie_dev_info(fd->dev, "%s: fail to load firmware %s\n", __func__,
			 name);
		return ret;
	}

	if (size < fw->size) {
		release_firmware(fw);
		return -EINVAL;
	}

	memcpy(buf, fw->data, fw->size);
	release_firmware(fw);

	return ret;
}
#endif

#if  CHECK_SERVICE_0
static void aie_reset_output_buf(struct mtk_aie_dev *fd,
				 struct aie_enq_info *aie_cfg)
{
	if (aie_cfg->sel_mode == 0) {
		memset(fd->rs_output_hw.va, 0, fd->rs_output_hw.size);

		memset(fd->dma_para->fd_out_hw_va[rpn0_loop_num][0], 0,
		       result_size);
		memset(fd->dma_para->fd_out_hw_va[rpn1_loop_num][0], 0,
		       result_size);
		memset(fd->dma_para->fd_out_hw_va[rpn2_loop_num][0], 0,
		       result_size);
	} else if (aie_cfg->sel_mode == 1) {
		memset(fd->base_para->rs_pym_rst_va[0][0], 0,
		       fd->rs_pym_out_size[0]);
		memset(fd->base_para->rs_pym_rst_va[0][1], 0,
		       fd->rs_pym_out_size[0]);
		memset(fd->base_para->rs_pym_rst_va[0][2], 0,
		       fd->rs_pym_out_size[0]);
	}
}
#endif
static int aie_update_cfg(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int crop_width;
	int crop_height;

	crop_width = aie_cfg->src_img_width;
	crop_height = aie_cfg->src_img_height;

	if (aie_cfg->en_roi) {
		crop_width = aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 + 1;
		crop_height = aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1 + 1;
	}

	if (crop_width == 0 || crop_height == 0) {
		aie_dev_info(fd->dev, "AIE error:crop size is wrong");
		return -EINVAL;
	}

	if (aie_cfg->en_padding) {
		crop_width = crop_width + aie_cfg->src_padding.right +
			     aie_cfg->src_padding.left;
		crop_height = crop_height + aie_cfg->src_padding.up +
			      aie_cfg->src_padding.down;
	}

	if (aie_cfg->sel_mode == FDMODE) {
		fd->base_para->sel_mode = aie_cfg->sel_mode;
		fd->base_para->crop_width = crop_width;
		fd->base_para->crop_height = crop_height;
		fd->base_para->src_img_addr = aie_cfg->src_img_addr;
		fd->base_para->src_img_addr_uv = aie_cfg->src_img_addr_uv;
		fd->base_para->img_width = aie_cfg->src_img_width;
		fd->base_para->img_height = aie_cfg->src_img_height;
		fd->base_para->src_img_fmt = aie_cfg->src_img_fmt;
		fd->base_para->rotate_degree = aie_cfg->rotate_degree;
	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		fd->attr_para->sel_mode[fd->attr_para->w_idx] =
			aie_cfg->sel_mode;
		fd->attr_para->crop_width[fd->attr_para->w_idx] = crop_width;
		fd->attr_para->crop_height[fd->attr_para->w_idx] = crop_height;
		fd->attr_para->src_img_addr[fd->attr_para->w_idx] =
			aie_cfg->src_img_addr;
		fd->attr_para->src_img_addr_uv[fd->attr_para->w_idx] =
			aie_cfg->src_img_addr_uv;
		fd->attr_para->img_width[fd->attr_para->w_idx] =
			aie_cfg->src_img_width;
		fd->attr_para->img_height[fd->attr_para->w_idx] =
			aie_cfg->src_img_height;
		fd->attr_para->src_img_fmt[fd->attr_para->w_idx] =
			aie_cfg->src_img_fmt;
		fd->attr_para->rotate_degree[fd->attr_para->w_idx] =
			aie_cfg->rotate_degree;
	}

	return 0;
}

static u32 aie_combine_u16(u16 low, u16 high)
{
	return ((u32)high << 16) | low;
}

static u32 aie_combine_stride(u16 low, u16 high)
{
	return ((u32)high << 16) | (low & 0x000F);
}

static int aie_config_y2r(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg,
			  int mode)
{
	u32 img_addr = 0;
	u32 img_addr_UV = 0;
	u32 img_off = 0;
	u32 img_off_uv = 0;
	u32 *yuv2rgb_cfg = NULL;
	u32 srcbuf = 0, srcbuf_UV = 0;
	u16 xmag_0 = 0, ymag_0 = 0;
	u16 pym0_out_w = 0;
	u16 pym0_out_h = 0;
	u16 stride_pym0_out_w = 0;
	u16 src_crop_w = 0;
	u16 src_crop_h = 0;
	unsigned int msb_bit_0 = 0, msb_bit_1 = 0, msb_bit_2 = 0;
	u32 flush_offset = 0;
	u32 flush_len = 0;

	if (aie_cfg->en_roi == false) {
		img_off = 0;
		img_off_uv = 0;
	} else {
		if (aie_cfg->src_img_fmt == FMT_MONO ||
		    aie_cfg->src_img_fmt == FMT_YUV_2P ||
		    aie_cfg->src_img_fmt == FMT_YVU_2P) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
			img_off_uv =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
		} else if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
			   aie_cfg->src_img_fmt == FMT_YUV420_1P) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1;
			img_off_uv = aie_cfg->src_img_stride *
				     aie_cfg->src_roi.y1 / 2 +
				     aie_cfg->src_roi.x1;
		} else if (aie_cfg->src_img_fmt == FMT_YUYV ||
			   aie_cfg->src_img_fmt == FMT_YVYU ||
			   aie_cfg->src_img_fmt == FMT_UYVY ||
			   aie_cfg->src_img_fmt == FMT_VYUY) {
			img_off =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1 * 2;
			img_off_uv =
				aie_cfg->src_img_stride * aie_cfg->src_roi.y1 +
				aie_cfg->src_roi.x1 * 2;
		} else {
			aie_dev_info(fd->dev,
				 "AIE error: Unsupport input format %d",
				 aie_cfg->src_img_fmt);
			return -EINVAL;
		}
	}

	img_addr = aie_cfg->src_img_addr + img_off;
	img_addr_UV = aie_cfg->src_img_addr_uv + img_off_uv;

	srcbuf = img_addr;
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P ||
	    aie_cfg->src_img_fmt == FMT_YUV_2P ||
	    aie_cfg->src_img_fmt == FMT_YVU_2P)
		srcbuf_UV = img_addr_UV;
	else
		srcbuf_UV = 0;

	if (mode == FDMODE) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
		yuv2rgb_cfg = (u32 *)fd->base_para->fd_yuv2rgb_cfg_va;
		pym0_out_w = fd->base_para->pyramid_width;
		flush_offset = g_fd_yuv2rgb_config_offset;
		flush_len = AIE_ALIGN32(fdvt_yuv2rgb_confi_frame01_size);
	} else if (mode == ATTRIBUTEMODE) {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
		yuv2rgb_cfg =
			(u32 *)fd->base_para
				->attr_yuv2rgb_cfg_va[fd->attr_para->w_idx];
		pym0_out_w = ATTR_MODE_PYRAMID_WIDTH;
		flush_offset = g_fd_yuv2rgb_config_offset +
			AIE_ALIGN32(fdvt_yuv2rgb_confi_frame01_size) +
			AIE_ALIGN32(attr_yuv2rgb_confi_frame01_size) * fd->attr_para->w_idx;
		flush_len = AIE_ALIGN32(attr_yuv2rgb_confi_frame01_size);
	} else {
		aie_dev_info(fd->dev,
				"YUV2RGB not support %d", mode);
		return -EINVAL;
	}

	if (src_crop_w)
		pym0_out_h = pym0_out_w * src_crop_h / src_crop_w;

	if (pym0_out_w != 0) {
		xmag_0 = 512 * src_crop_w / pym0_out_w;
		ymag_0 = xmag_0;
	} else {
		xmag_0 = 0;
		ymag_0 = 0;
	}

	yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] =
		(yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] & 0xFFFFFFF8) |
		((aie_cfg->src_img_fmt) & 0x7);
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P) { /* for match patten */
		yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] =
			(yuv2rgb_cfg[Y2R_SRC_DST_FORMAT] & 0xFFFFFFF8) |
			((0x3) & 0x7);
	}
	yuv2rgb_cfg[Y2R_IN_W_H] = (yuv2rgb_cfg[Y2R_IN_W_H] & 0xF800F800) |
				  ((src_crop_w << 16) & 0x7FF0000) |
				  (src_crop_h & 0x7FF);
	yuv2rgb_cfg[Y2R_OUT_W_H] = (yuv2rgb_cfg[Y2R_OUT_W_H] & 0xF800F800) |
				   ((pym0_out_w << 16) & 0x7FF0000) |
				   (pym0_out_h & 0x7FF);

	if (aie_cfg->src_img_fmt == FMT_YUV_2P ||
	    aie_cfg->src_img_fmt == FMT_YVU_2P) { /* 2 plane */
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x11;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x1;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x1;
	} else if (aie_cfg->src_img_fmt == FMT_MONO) {
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x01;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
	} else if (aie_cfg->src_img_fmt == FMT_YUYV ||
		   aie_cfg->src_img_fmt == FMT_YVYU ||
		   aie_cfg->src_img_fmt == FMT_UYVY ||
		   aie_cfg->src_img_fmt == FMT_VYUY) { /* 1 plane */
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x1;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				2 * (aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 +
				     1) -
					1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				2 * (aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1 +
				     1) -
					1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				2 * src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				2 * src_crop_w - 1, src_crop_h - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x3;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x3;
	}

	/* AIE3.0 */
	if (aie_cfg->src_img_fmt == FMT_YUV420_2P ||
	    aie_cfg->src_img_fmt == FMT_YUV420_1P) {
		yuv2rgb_cfg[Y2R_RA0_RA1_EN] =
			(yuv2rgb_cfg[Y2R_RA0_RA1_EN] & 0xFFFFFFEE) | 0x11;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1,
				(aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1) /
					2);
		} else {
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE0] =
				aie_combine_u16(src_crop_w - 1, src_crop_h - 1);
			yuv2rgb_cfg[Y2R_IN_X_Y_SIZE1] = aie_combine_u16(
				src_crop_w - 1, src_crop_h / 2 - 1);
		}
		yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE0_BUS_SIZE0] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;
		yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] =
			(yuv2rgb_cfg[Y2R_IN_STRIDE1_BUS_SIZE1] & 0xFFF0) |
			((aie_cfg->src_img_stride << 16) & 0xFFFF0000) | 0x0;

		yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] =
			(yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] & 0xFFFFFFFE) | 0x01;
		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] = aie_combine_u16(
				0, aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] = aie_combine_u16(
				0, aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] =
				aie_combine_u16(0, src_crop_w - 1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] =
				aie_combine_u16(0, src_crop_h - 1);
		}
	} else {
		yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] =
			(yuv2rgb_cfg[Y2R_CO2_FMT_MODE_EN] & 0xFFFFFFFE);

		if (aie_cfg->en_roi) {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] = aie_combine_u16(
				0, aie_cfg->src_roi.x2 - aie_cfg->src_roi.x1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] = aie_combine_u16(
				0, aie_cfg->src_roi.y2 - aie_cfg->src_roi.y1);
		} else {
			yuv2rgb_cfg[Y2R_CO2_CROP_X] =
				aie_combine_u16(0, src_crop_w - 1);
			yuv2rgb_cfg[Y2R_CO2_CROP_Y] =
				aie_combine_u16(0, src_crop_h - 1);
		}
	}

	stride_pym0_out_w = round_up(pym0_out_w, 8);

	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE0] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE0_BUS_SIZE0] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE0_BUS_SIZE0], stride_pym0_out_w);
	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE1] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE1_BUS_SIZE1] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE1_BUS_SIZE1], stride_pym0_out_w);
	yuv2rgb_cfg[Y2R_OUT_X_Y_SIZE2] =
		aie_combine_u16(pym0_out_w - 1, pym0_out_h - 1);
	yuv2rgb_cfg[Y2R_OUT_STRIDE2_BUS_SIZE2] = aie_combine_u16(
		yuv2rgb_cfg[Y2R_OUT_STRIDE2_BUS_SIZE2], stride_pym0_out_w);

	if (aie_cfg->en_padding) {
		yuv2rgb_cfg[Y2R_PADDING_EN_UP_DOWN] =
			1 | ((aie_cfg->src_padding.up << 4) & 0x1FF0) |
			((aie_cfg->src_padding.down << 16) & 0x01FF0000);
		yuv2rgb_cfg[Y2R_PADDING_RIGHT_LEFT] =
			(aie_cfg->src_padding.right & 0x01FF) |
			((aie_cfg->src_padding.left << 16) & 0x01FF0000);
	} else {
		yuv2rgb_cfg[Y2R_PADDING_EN_UP_DOWN] = 0;
		yuv2rgb_cfg[Y2R_PADDING_RIGHT_LEFT] = 0;
	}

	yuv2rgb_cfg[Y2R_IN_0] = srcbuf;
	yuv2rgb_cfg[Y2R_IN_1] = srcbuf_UV;


	//yuv2rgb_cfg[POS_Y2RCON_IN_BA_MSB] = (u32)0x00000303; //for UT
	yuv2rgb_cfg[POS_Y2RCON_IN_BA_MSB] = (u32)(fd->img_msb_y | fd->img_msb_uv << 8);
	msb_bit_0 = (fd->base_para->rs_pym_rst_pa[0][0] &
						0xf00000000) >> 32;
	msb_bit_1 = (fd->base_para->rs_pym_rst_pa[0][1] &
						0xf00000000) >> 32;
	msb_bit_2 = (fd->base_para->rs_pym_rst_pa[0][2] &
						0xf00000000) >> 32;

	yuv2rgb_cfg[POS_Y2RCON_OUT_BA_MSB] = (u32)(msb_bit_0 | msb_bit_1 << 8 |
						msb_bit_2 << 16);//0x00030303

	yuv2rgb_cfg[Y2R_OUT_0] = (u32)fd->base_para->rs_pym_rst_pa[0][0];
	yuv2rgb_cfg[Y2R_OUT_1] = (u32)fd->base_para->rs_pym_rst_pa[0][1];
	yuv2rgb_cfg[Y2R_OUT_2] = (u32)fd->base_para->rs_pym_rst_pa[0][2];

	yuv2rgb_cfg[Y2R_X_Y_MAG] =
		(xmag_0 & 0x3FFF) | ((ymag_0 << 16) & 0x3FFF0000);

	if (src_crop_w >= pym0_out_w) { /* down scale AIE1.0 by FRZ */
		yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] =
			(yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] & 0x00100070) |
			FDRZ_BIT;
		yuv2rgb_cfg[Y2R_SRZ_HORI_STEP] = 0;
		yuv2rgb_cfg[Y2R_SRZ_VERT_STEP] = 0;
	} else { /* SRZ */
		/* 0: FDRZ for down scaling */
		/* 1: SRZ for up scaling */
		yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] =
			(yuv2rgb_cfg[Y2R_RS_SEL_SRZ_EN] & 0x00100070) | SRZ_BIT;
		yuv2rgb_cfg[Y2R_SRZ_HORI_STEP] =
			((src_crop_w - 1) << 15) / (pym0_out_w - 1);
		yuv2rgb_cfg[Y2R_SRZ_VERT_STEP] =
			((src_crop_h - 1) << 15) / (pym0_out_h - 1);
	}

	dma_buf_end_cpu_access_partial(fd->config_dmabuf,
		DMA_BIDIRECTIONAL, flush_offset, flush_len);

	return 0;
}

static int aie_config_rs(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	u32 *rs_cfg = NULL;
	u32 *rs_tbl[2] = {NULL, NULL};
	u16 xmag_0 = 0, ymag_0 = 0;
	u16 pym_out_w[3] = {0, 0, 0};
	u16 pym_out_h[3] = {0, 0, 0};
	u16 round_w = 0;
	u16 src_crop_w = 0;
	u16 src_crop_h = 0;
	int i = 0, msb_bit_0 = 0, msb_bit_1 = 0, msb_bit_2 = 0;

	if (aie_cfg->sel_mode == FDMODE) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
	}

	rs_cfg = (u32 *)fd->base_para->fd_rs_cfg_va;

	pym_out_w[0] = fd->base_para->pyramid_width;
	pym_out_w[1] = pym_out_w[0] >> 1;
	pym_out_w[2] = pym_out_w[1] >> 1;

	if (src_crop_w)
		pym_out_h[0] = pym_out_w[0] * src_crop_h / src_crop_w;

	pym_out_h[1] = pym_out_h[0] >> 1;
	pym_out_h[2] = pym_out_h[1] >> 1;

	for (i = 0; i < 2; i++) {
		rs_tbl[i] = rs_cfg + RS_CONFIG_SIZE * i;

		msb_bit_0 = (fd->base_para->rs_pym_rst_pa[i][0] &
							0xf00000000) >> 32;
		msb_bit_1 = (fd->base_para->rs_pym_rst_pa[i][1] &
							0xf00000000) >> 32;
		msb_bit_2 = (fd->base_para->rs_pym_rst_pa[i][2] &
							0xf00000000) >> 32;

		rs_tbl[i][POS_RSCON_IN_BA_MSB] = (u32)(msb_bit_0 | msb_bit_1 << 8 |
							msb_bit_2 << 16); //0x00030303

		rs_tbl[i][RS_IN_0] = (u32)fd->base_para->rs_pym_rst_pa[i][0];
		rs_tbl[i][RS_IN_1] = (u32)fd->base_para->rs_pym_rst_pa[i][1];
		rs_tbl[i][RS_IN_2] = (u32)fd->base_para->rs_pym_rst_pa[i][2];

		msb_bit_0 = (fd->base_para->rs_pym_rst_pa[i + 1][0] &
							0xf00000000) >> 32;
		msb_bit_1 = (fd->base_para->rs_pym_rst_pa[i + 1][1] &
							0xf00000000) >> 32;
		msb_bit_2 = (fd->base_para->rs_pym_rst_pa[i + 1][2] &
							0xf00000000) >> 32;

		rs_tbl[i][POS_RSCON_OUT_BA_MSB] = (u32)(msb_bit_0 | msb_bit_1 << 8 |
							msb_bit_2 << 16); //0x00030303

		rs_tbl[i][RS_OUT_0] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][0];
		rs_tbl[i][RS_OUT_1] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][1];
		rs_tbl[i][RS_OUT_2] =
			(u32)fd->base_para->rs_pym_rst_pa[i + 1][2];

		rs_tbl[i][RS_INPUT_W_H] =
			(rs_tbl[i][RS_INPUT_W_H] & 0xF800F800) |
			(pym_out_h[i] & 0x7FF) |
			((pym_out_w[i] << 16) & 0x7FF0000);
		rs_tbl[i][RS_OUTPUT_W_H] =
			(rs_tbl[i][RS_OUTPUT_W_H] & 0xF800F800) |
			(pym_out_h[i + 1] & 0x7FF) |
			((pym_out_w[i + 1] << 16) & 0x7FF0000);
		rs_tbl[i][RS_IN_X_Y_SIZE0] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);
		rs_tbl[i][RS_IN_X_Y_SIZE1] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);
		rs_tbl[i][RS_IN_X_Y_SIZE2] =
			aie_combine_u16(pym_out_w[i] - 1, pym_out_h[i] - 1);

		round_w = round_up(pym_out_w[i], 8);
		rs_tbl[i][RS_IN_STRIDE0] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE0], round_w);
		rs_tbl[i][RS_IN_STRIDE1] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE1], round_w);
		rs_tbl[i][RS_IN_STRIDE2] =
			aie_combine_u16(rs_tbl[i][RS_IN_STRIDE2], round_w);
		rs_tbl[i][RS_OUT_X_Y_SIZE0] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);
		rs_tbl[i][RS_OUT_X_Y_SIZE1] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);
		rs_tbl[i][RS_OUT_X_Y_SIZE2] = aie_combine_u16(
			pym_out_w[i + 1] - 1, pym_out_h[i + 1] - 1);


		round_w = round_up(pym_out_w[i + 1], 8);

		rs_tbl[i][RS_OUT_STRIDE0] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE0], round_w);
		rs_tbl[i][RS_OUT_STRIDE1] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE1], round_w);
		rs_tbl[i][RS_OUT_STRIDE2] =
			aie_combine_u16(rs_tbl[i][RS_OUT_STRIDE2], round_w);

		xmag_0 = 512 * pym_out_w[i] / pym_out_w[i + 1];
		ymag_0 = xmag_0;

		rs_tbl[i][RS_X_Y_MAG] =
			(xmag_0 & 0x3FFF) | ((ymag_0 << 16) & 0x3FFF0000);
	}

	dma_buf_end_cpu_access_partial(fd->config_dmabuf,
		DMA_BIDIRECTIONAL, g_fd_rs_config_offset,
		AIE_ALIGN32(fdvt_rs_confi_frame01_size));

	return 0;
}

static int aie_config_network(struct mtk_aie_dev *fd,
			      struct aie_enq_info *aie_cfg)
{
	u16 conv_width = 0;
	u16 conv_height = 0;
	u8 i = 0;
	u8 j = 0;
	u8 uch = 0;
	u8 uloop = 0;
	u16 fd_xsize[4] = {0, 0, 0, 0};
	void *fd_cfg = NULL;
	u32 *fd_cur_cfg = NULL;
	u32 *fd_cur_set = NULL;
	u16 pyramid0_out_w = 0;
	u16 pyramid0_out_h = 0;
	u16 pyramid1_out_h = 0;
	u16 pyramid2_out_h = 0;
	u16 input_height = 0;
	u16 out_height = 0;
	u16 out_ysize_plus_1 = 0;
	u16 out_ysize_plus_1_stride2 = 0;
	u32 src_crop_w  = 0;
	u32 src_crop_h = 0;
	u32 flush_offset  = g_fd_fd_config_offset;
	u32 flush_len = AIE_ALIGN32(fdvt_fd_confi_frame01_size);
	struct aie_static_info *pstv = NULL;
	int msb_bit_0 = 0, msb_bit_1 = 0, msb_bit_2 = 0, msb_bit_3 = 0;

	pstv = &fd->st_info;

	if (aie_cfg->sel_mode == FDMODE) {
		src_crop_w = fd->base_para->crop_width;
		src_crop_h = fd->base_para->crop_height;
	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
		src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];
	}

	pyramid0_out_w = fd->base_para->pyramid_width;
	if (src_crop_w)
		pyramid0_out_h = pyramid0_out_w * src_crop_h / src_crop_w;

	pyramid1_out_h = pyramid0_out_h / 2;
	pyramid2_out_h = pyramid1_out_h / 2;

	fd_cfg = fd->base_para->fd_fd_cfg_va;

	switch (fd->base_para->number_of_pyramid) {
	case 1:
		i = rpn1_loop_num + 1;
		break;
	case 2:
		i = rpn2_loop_num + 1;
		break;
	case 3:
		i = 0;
		break;
	default:
		aie_dev_info(fd->dev, "pyramid size error: %d",
					fd->base_para->number_of_pyramid);
		return -1;
	}

	for (; i < fd_loop_num; i++) {
		msb_bit_0 = 0;
		msb_bit_1 = 0;
		msb_bit_2 = 0;
		msb_bit_3 = 0;
		fd_cur_cfg = (u32 *)fd_cfg + FD_CONFIG_SIZE * i;
		fd_cur_cfg[FD_INPUT_ROTATE] =
			(fd_cur_cfg[FD_INPUT_ROTATE] & 0xFFFF0FFF) |
			((aie_cfg->rotate_degree << 12) & 0x3000);

		if (i == 0) {
			input_height = pyramid2_out_h;
		} else if (i == (rpn2_loop_num + 1)) {
			input_height = pyramid1_out_h;
		} else if (i == (rpn1_loop_num + 1)) {
			input_height = pyramid0_out_h;
		} else {
			if (fd_out_stride2_in[i] == 0)
				input_height = out_height;
			else
				input_height = (out_height + 1) / 2;
		}
		if (i == rpn0_loop_num)
			fd->pose_height = input_height;

		if (fd_maxpool[i] == 1 && fd_stride[i] == 1)
			out_height =
				DIV_ROUND_UP(input_height, 2 * fd_maxpool[i]);
		else
			out_height = DIV_ROUND_UP(
				input_height, fd_stride[i] + 2 * fd_maxpool[i]);

		if (i == rpn0_loop_num || i == rpn1_loop_num ||
		    i == rpn2_loop_num) {
			conv_width = fd->base_para->img_width;
			conv_height = fd->base_para->img_height;
			fd_xsize[0] =
				pstv->img_width[i] * 2 * 16 * anchor_en_num[i] -
				1;
			fd_xsize[1] = fd_xsize[2] = fd_xsize[3] =
				pstv->img_width[i] * 2 * 32 * anchor_en_num[i] -
				1;
		} else {
			conv_width =
				DIV_ROUND_UP(pstv->img_width[i], fd_stride[i]);
			conv_height = DIV_ROUND_UP(input_height, fd_stride[i]);

			fd_xsize[0] = fd_xsize[1] = fd_xsize[2] = fd_xsize[3] =
				pstv->input_xsize_plus_1[i] - 1;
		}

		fd_cur_cfg[FD_CONV_WIDTH_MOD6] =
			(fd_cur_cfg[FD_CONV_WIDTH_MOD6] & 0xFF8FFFFF) |
			(((conv_width % 6) << 20) & 0x00700000);
		fd_cur_cfg[FD_CONV_IMG_W_H] =
			aie_combine_u16(conv_height, conv_width);

		fd_cur_cfg[FD_IN_IMG_W_H] =
			aie_combine_u16(input_height, pstv->img_width[i]);
		fd_cur_cfg[FD_OUT_IMG_W_H] =
			aie_combine_u16(out_height, pstv->out_width[i]);

		if (fd_rdma_en[i][0][0] != -1) {
			for (j = 0; j < 4; j++) {
				fd_cur_cfg[FD_IN_X_Y_SIZE0 + 2 * j] =
					aie_combine_u16(fd_xsize[j],
							input_height - 1);

				fd_cur_cfg[FD_IN_STRIDE0_BUS_SIZE0 + 2 * j] =
					aie_combine_stride(
						fd_cur_cfg
							[FD_IN_STRIDE0_BUS_SIZE0 +
							2 * j],
						fd_xsize[j] + 1);
			}
		}

		out_ysize_plus_1 = out_height - 1;
		out_ysize_plus_1_stride2 = (out_height + 1) / 2 - 1;

		for (j = 0; j < output_WDMA_WRA_num; j++) {
			fd_cur_set = fd_cur_cfg + 2 * j;
			if (!fd_wdma_en[i][j])
				continue;

			if (out_stride_size[i][j] == 1) {
				fd_cur_set[FD_OUT_X_Y_SIZE0] = aie_combine_u16(
					pstv->out_xsize_plus_1[i] - 1,
					out_ysize_plus_1);
				fd_cur_set[FD_OUT_STRIDE0_BUS_SIZE0] =
					aie_combine_stride(
						fd_cur_set
							[FD_OUT_STRIDE0_BUS_SIZE0],
						pstv->out_stride[i]);
			} else if (out_stride_size[i][j] == 2) {
				fd_cur_set[FD_OUT_X_Y_SIZE0] = aie_combine_u16(
					pstv->out_xsize_plus_1_stride2[i] - 1,
					out_ysize_plus_1_stride2);
				fd_cur_set[FD_OUT_STRIDE0_BUS_SIZE0] =
					aie_combine_stride(
						fd_cur_set
							[FD_OUT_STRIDE0_BUS_SIZE0],
						pstv->out_stride_stride2[i]);
			}
		}

		if (i == rpn0_loop_num || i == rpn1_loop_num || i == rpn2_loop_num) {

			fd_cur_cfg[FD_RPN_SET] =
				aie_combine_u16(fd_cur_cfg[FD_RPN_SET],
						fd->base_para->rpn_anchor_thrd);
				fd_cur_cfg[FD_IN_CHANNEL_PACK] = fd_cur_cfg[Y2R_SRC_DST_FORMAT] |
									0x30000000;
		}

		if (i == rpn0_loop_num) {
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				(((src_crop_w * 512 /
				(int)fd->base_para->pyramid_width)
				<< 4) &
				0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		} else if (i == rpn1_loop_num) {
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				(((src_crop_w * 512 /
				(int)fd->base_para->pyramid_width * 2)
				<< 4) &
				0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		} else if (i == rpn2_loop_num) {
			fd_cur_cfg[FD_IMAGE_COORD] =
				(fd_cur_cfg[FD_IMAGE_COORD] & 0xF) |
				(((src_crop_w * 512/
				(int)fd->base_para->pyramid_width * 4)
				<< 4) &
				0x7FFF0);
			fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] = 0;
			if (aie_cfg->en_roi) {
				fd_cur_cfg[FD_IMAGE_COORD_XY_OFST] =
				(aie_cfg->src_roi.x1 - aie_cfg->src_padding.left) |
				(aie_cfg->src_roi.y1 - aie_cfg->src_padding.up) << 16;
			}
		}

		/* IN_FM_BASE_ADR */
		if (i == 0) {
			msb_bit_0 = (fd->base_para->rs_pym_rst_pa[2][0] &
								0xf00000000) >> 32;
			msb_bit_1 = (fd->base_para->rs_pym_rst_pa[2][1] &
								0xf00000000) >> 32;
			msb_bit_2 = (fd->base_para->rs_pym_rst_pa[2][2] &
								0xf00000000) >> 32;

			fd_cur_cfg[POS_FDCON_IN_BA_MSB] = (u32)(msb_bit_0 |
				msb_bit_1 << 8 | msb_bit_2 << 16);
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[2][2]);
		} else if (i == (rpn2_loop_num + 1)) {
			msb_bit_0 = (fd->base_para->rs_pym_rst_pa[1][0] &
								0xf00000000) >> 32;
			msb_bit_1 = (fd->base_para->rs_pym_rst_pa[1][1] &
								0xf00000000) >> 32;
			msb_bit_2 = (fd->base_para->rs_pym_rst_pa[1][2] &
								0xf00000000) >> 32;

			fd_cur_cfg[POS_FDCON_IN_BA_MSB] = (u32)(msb_bit_0 |
				msb_bit_1 << 8 |  msb_bit_2 << 16);

			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[1][2]);
		} else if (i == (rpn1_loop_num + 1)) {
			msb_bit_0 = (fd->base_para->rs_pym_rst_pa[0][0] &
								0xf00000000) >> 32;
			msb_bit_1 = (fd->base_para->rs_pym_rst_pa[0][1] &
								0xf00000000) >> 32;
			msb_bit_2 = (fd->base_para->rs_pym_rst_pa[0][2] &
								0xf00000000) >> 32;

			fd_cur_cfg[POS_FDCON_IN_BA_MSB] = (u32)(msb_bit_0 |
						msb_bit_1 << 8 |  msb_bit_2 << 16);
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][2]);
		} else {
			for (j = 0; j < input_WDMA_WRA_num; j++) {
				if (fd_rdma_en[i][j][0] != -1) {
					uloop = fd_rdma_en[i][j][0];
					uch = fd_rdma_en[i][j][1];
					if (j == 0) {
						msb_bit_0 = (fd->dma_para->fd_out_hw_pa[uloop][uch]
								& 0xf00000000) >> 32;
					} else if (j == 1) {
						msb_bit_1 = (fd->dma_para->fd_out_hw_pa[uloop][uch]
								& 0xf00000000) >> 32;
					} else if (j == 2) {
						msb_bit_2 = (fd->dma_para->fd_out_hw_pa[uloop][uch]
								& 0xf00000000) >> 32;
					} else if (j == 3) {
						msb_bit_3 = (fd->dma_para->fd_out_hw_pa[uloop][uch]
								& 0xf00000000) >> 32;
					}
					fd_cur_cfg[FD_IN_0 + j] = (u32)(
						fd->dma_para
							->fd_out_hw_pa[uloop]
								      [uch]);
				}
			}
			fd_cur_cfg[POS_FDCON_IN_BA_MSB] = (u32)((msb_bit_3 << 24) |
						(msb_bit_2 << 16) | (msb_bit_1 << 8) | (msb_bit_0));
		}

		/* OUT_FM_BASE_ADR */
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (fd_wdma_en[i][j]) {
				if (j == 0) {
					msb_bit_0 = (fd->dma_para->fd_out_hw_pa[i][j] &
								0xf00000000) >> 32;
				} else if (j == 1) {
					msb_bit_1 = (fd->dma_para->fd_out_hw_pa[i][j] &
								0xf00000000) >> 32;
				} else if (j == 2) {
					msb_bit_2 = (fd->dma_para->fd_out_hw_pa[i][j] &
								0xf00000000) >> 32;
				} else if (j == 3) {
					msb_bit_3 = (fd->dma_para->fd_out_hw_pa[i][j] &
								0xf00000000) >> 32;
				}
				fd_cur_cfg[FD_OUT_0 + j] =
					(u32)(fd->dma_para->fd_out_hw_pa[i][j]);
			}
		}
		fd_cur_cfg[POS_FDCON_OUT_BA_MSB] = (u32)((msb_bit_3 << 24)
					| (msb_bit_2 << 16) | (msb_bit_1 << 8) | (msb_bit_0));

		/* KERNEL_BASE_ADR */
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (fd_ker_rdma_size[i][j]) {
				if (j == 0) {
					msb_bit_0 = (fd->dma_para->fd_kernel_pa[i][j] &
								0xf00000000) >> 32;
				} else if (j == 1) {
					msb_bit_1 = (fd->dma_para->fd_kernel_pa[i][j] &
								0xf00000000) >> 32;
				}
				fd_cur_cfg[FD_KERNEL_0 + j] =
					(u32)(fd->dma_para->fd_kernel_pa[i][j]);
			}
		}
		fd_cur_cfg[POS_FDCON_KERNEL_BA_MSB] = (u32)((msb_bit_1 << 8) | (msb_bit_0));
	}

	dma_buf_end_cpu_access_partial(fd->config_dmabuf,
		DMA_BIDIRECTIONAL, flush_offset, flush_len);

	return 0;
}

static int aie_config_attr_network(struct mtk_aie_dev *fd,
				   struct aie_enq_info *aie_cfg)
{
	bool isRegressionLoop = false;
	void *fd_cfg;
	u32 *fd_cur_cfg;
	u16 fd_input_ht, fd_output_ht = 0x0;
	u16 fd_out_y[4];
	u8 i, j;
	u8 uloop, uch, uidx;
	u16 pyramid0_out_w, pyramid0_out_h;
	int fd_conv_ht;
	u16 src_crop_w;
	u16 src_crop_h;
	int msb_bit_0 = 0, msb_bit_1 = 0, msb_bit_2 = 0, msb_bit_3 = 0;
	u32 flush_offset;
	u32 flush_len;

	flush_offset = g_fd_fd_config_offset + AIE_ALIGN32(fdvt_fd_confi_frame01_size) +
		AIE_ALIGN32(attr_fd_confi_frame01_size) * fd->attr_para->w_idx;
	flush_len = AIE_ALIGN32(attr_fd_confi_frame01_size);

	src_crop_w = fd->attr_para->crop_width[fd->attr_para->w_idx];
	src_crop_h = fd->attr_para->crop_height[fd->attr_para->w_idx];

	pyramid0_out_w = ATTR_MODE_PYRAMID_WIDTH;
	pyramid0_out_h = pyramid0_out_w * src_crop_h / src_crop_w;

	fd_cfg = fd->base_para->attr_fd_cfg_va[fd->attr_para->w_idx];

	for (i = 0; i < attr_loop_num; i++) {
		fd_cur_cfg = (u32 *)fd_cfg + FD_CONFIG_SIZE * i;
		fd_cur_cfg[FD_INPUT_ROTATE] =
			(fd_cur_cfg[FD_INPUT_ROTATE] & 0xFFFF0FFF) |
			((aie_cfg->rotate_degree << 12) & 0x3000);

		if (i == 0) {
			fd_input_ht = pyramid0_out_h;
		} else {
			if (attr_out_stride2_as_in[i] == 0)
				fd_input_ht = fd_output_ht;
			else if (attr_out_stride2_as_in[i] == 1)
				fd_input_ht = (fd_output_ht + 1) / 2;
		}
		fd_output_ht = DIV_ROUND_UP(fd_input_ht,
					    attr_fd_stride[i] +
						    2 * attr_fd_maxpool[i]);
		fd_conv_ht = DIV_ROUND_UP(fd_input_ht, attr_fd_stride[i]);

		fd_cur_cfg[FD_CONV_IMG_W_H] =
			(fd_cur_cfg[FD_CONV_IMG_W_H] & 0xFFFF0000) |
			(fd_conv_ht & 0xFFFF);
		fd_cur_cfg[FD_IN_IMG_W_H] =
			(fd_cur_cfg[FD_IN_IMG_W_H] & 0xFFFF0000) |
			(fd_input_ht & 0xFFFF);
		fd_cur_cfg[FD_OUT_IMG_W_H] =
			(fd_cur_cfg[FD_OUT_IMG_W_H] & 0xFFFF0000) |
			(fd_output_ht & 0xFFFF);
		fd_cur_cfg[FD_IN_X_Y_SIZE0] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE0], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE1] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE1], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE2] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE2], fd_input_ht - 1);
		fd_cur_cfg[FD_IN_X_Y_SIZE3] = aie_combine_u16(
			fd_cur_cfg[FD_IN_X_Y_SIZE3], fd_input_ht - 1);

		isRegressionLoop = (i == age_out_rgs || i == gender_out_rgs ||
				    i == indian_out_rgs || i == race_out_rgs);

		if (isRegressionLoop) {
			fd_out_y[0] = 0;
			fd_out_y[1] = 0;
			fd_out_y[2] = 0;
			fd_out_y[3] = 0;
		} else {
			fd_out_y[0] = fd_output_ht - 1;
			fd_out_y[1] = fd_output_ht - 1;
			if (attr_out_2size[i] == 0) {
				fd_out_y[2] = fd_output_ht - 1;
				fd_out_y[3] = fd_output_ht - 1;
			} else {
				fd_out_y[2] = (fd_output_ht + 1) / 2 - 1;
				fd_out_y[3] = (fd_output_ht + 1) / 2 - 1;
			}
		}

		for (j = 0; j < 4; j++)
			fd_cur_cfg[FD_OUT_X_Y_SIZE0 + 2 * j] = aie_combine_u16(
				fd_cur_cfg[FD_OUT_X_Y_SIZE0 + 2 * j],
				fd_out_y[j]);

		/* IN_FM_BASE_ADR */
		if (i == 0) {
			msb_bit_0 = (fd->base_para->rs_pym_rst_pa[0][0] &
								0xf00000000) >> 32;
			msb_bit_1 = (fd->base_para->rs_pym_rst_pa[0][1] &
								0xf00000000) >> 32;
			msb_bit_2 = (fd->base_para->rs_pym_rst_pa[0][2] &
								0xf00000000) >> 32;

			fd_cur_cfg[POS_FDCON_IN_BA_MSB] = (u32)(msb_bit_0 |
							msb_bit_1 << 8 |
							msb_bit_2 << 16);
			fd_cur_cfg[FD_IN_0] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][0]);
			fd_cur_cfg[FD_IN_1] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][1]);
			fd_cur_cfg[FD_IN_2] =
				(u32)(fd->base_para->rs_pym_rst_pa[0][2]);
		} else {
			for (j = 0; j < input_WDMA_WRA_num; j++) {

				if (attr_rdma_en[i][j][0] != -1) {
					uloop = attr_rdma_en[i][j][0];
					uch = attr_rdma_en[i][j][1];
					if (j == 0) {
						msb_bit_0 =
						(fd->dma_para->attr_out_hw_pa[uloop][uch] &
								0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_IN_BA_MSB] |= (u32)(msb_bit_0);
					} else if (j == 1) {
						msb_bit_1 =
						(fd->dma_para->attr_out_hw_pa[uloop][uch] &
								0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_IN_BA_MSB] |=
								(u32)(msb_bit_1 << 8);
					} else if (j == 2) {
						msb_bit_2 =
						(fd->dma_para->attr_out_hw_pa[uloop][uch] &
								0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_IN_BA_MSB] |=
								(u32)(msb_bit_2 << 16);
					} else if (j == 3) {
						msb_bit_3 =
						(fd->dma_para->attr_out_hw_pa[uloop][uch] &
								0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_IN_BA_MSB] |=
								(u32)(msb_bit_3 << 24);
					}

					fd_cur_cfg[FD_IN_0 + j] = (u32)(
						fd->dma_para
							->attr_out_hw_pa[uloop]
									[uch]);
				}
			}
		}

		/* OUT_FM_BASE_ADR */
		for (j = 0; j < output_WDMA_WRA_num; j++) {
			if (attr_wdma_en[i][j]) {
				uidx = fd->attr_para->w_idx;
				if (i == age_out_rgs && j == 0) {
					msb_bit_0 = (fd->dma_para->age_out_hw_pa[uidx] &
									0xf00000000) >> 32;
					fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |= (u32)msb_bit_0;
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->age_out_hw_pa[uidx]);
				} else if (i == gender_out_rgs && j == 0) {
					msb_bit_0 = (fd->dma_para->gender_out_hw_pa[uidx] &
									0xf00000000) >> 32;
					fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |= (u32)msb_bit_0;
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para->gender_out_hw_pa
							[uidx]);
				} else if (i == indian_out_rgs && j == 0) {
					msb_bit_0 = (fd->dma_para->isIndian_out_hw_pa[uidx] &
									0xf00000000) >> 32;
					fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |= (u32)msb_bit_0;

					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para->isIndian_out_hw_pa
							[uidx]);
				} else if (i == race_out_rgs && j == 0) {
					msb_bit_0 = (fd->dma_para->race_out_hw_pa[uidx] &
									0xf00000000) >> 32;
					fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |= (u32)msb_bit_0;
					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->race_out_hw_pa[uidx]);
				} else {
					if (j == 0) {
						msb_bit_0 = (fd->dma_para->attr_out_hw_pa[i][j] &
									0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |=
									(u32)(msb_bit_0);
					} else if (j == 1) {
						msb_bit_1 = (fd->dma_para->attr_out_hw_pa[i][j] &
									0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |=
									(u32)(msb_bit_1 << 8);
					} else if (j == 2) {
						msb_bit_2 = (fd->dma_para->attr_out_hw_pa[i][j] &
									0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |=
									(u32)(msb_bit_2 << 16);
					} else if (j == 3) {
						msb_bit_3 = (fd->dma_para->attr_out_hw_pa[i][j] &
									0xf00000000) >> 32;
						fd_cur_cfg[POS_FDCON_OUT_BA_MSB] |=
									(u32)(msb_bit_3 << 24);
					}

					fd_cur_cfg[FD_OUT_0 + j] = (u32)(
						fd->dma_para
							->attr_out_hw_pa[i][j]);
			}
		}
		}

		/* KERNEL_BASE_ADR */
		for (j = 0; j < kernel_RDMA_RA_num; j++) {
			if (j == 0) {
				msb_bit_0 = (fd->dma_para->attr_kernel_pa[i][j] &
								0xf00000000) >> 32;
				fd_cur_cfg[POS_FDCON_KERNEL_BA_MSB] |= (u32)(msb_bit_0);
			} else if (j == 1) {
				msb_bit_1 = (fd->dma_para->attr_kernel_pa[i][j] &
								0xf00000000) >> 32;
				fd_cur_cfg[POS_FDCON_KERNEL_BA_MSB] |= (u32)(msb_bit_1 << 8);
			}
			fd_cur_cfg[FD_KERNEL_0 + j] =
				(u32)(fd->dma_para->attr_kernel_pa[i][j]);
		}
	}

	dma_buf_end_cpu_access_partial(fd->config_dmabuf,
		DMA_BIDIRECTIONAL, flush_offset, flush_len);

	return 0;
}

static int aie_config_dram(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int ret = 0;

	if (aie_cfg->sel_mode == FDMODE) { /* FDMODE */
		ret = aie_config_y2r(fd, aie_cfg, aie_cfg->sel_mode);
		if (ret)
			return ret;

		ret = aie_config_rs(fd, aie_cfg);
		if (ret)
			return ret;

		ret = aie_config_network(fd, aie_cfg);
		if (ret)
			return ret;

	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) { /* ATTRIBUTEMODE */
		ret = aie_config_y2r(fd, aie_cfg, aie_cfg->sel_mode);
		if (ret)
			return ret;

		ret = aie_config_attr_network(fd, aie_cfg);
		if (ret)
			return ret;
	}

	return ret;
}

static void aie_reset(struct mtk_aie_dev *fd)
{
	unsigned int ret = 0, counter = 0;

	if (fd->is_shutdown) {
		aie_dev_info(fd->dev, "%s: skip for shutdown", __func__);
		return;
	}

	writel(0x20000, fd->fd_base + AIE_START_REG);
	ret = (unsigned int)readl(fd->fd_base + AIE_START_REG);
	while (!((ret & 0X20000) == 0) && counter < 10) {
		ret = (unsigned int)readl(fd->fd_base + AIE_START_REG);
		aie_dev_info(fd->dev, "SW Reset... 0x%08X\n", ret);
		counter++;
	}
	writel(0x10000, fd->fd_base + AIE_START_REG);
	writel(0x0, fd->fd_base + AIE_START_REG);
}

static int aie_alloc_aie_buf(struct mtk_aie_dev *fd)
{
	int ret = -1;
	int err_tag = 0;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	aie_reset(fd);

	memset(&fd->st_info, 0, sizeof(fd->st_info));

	if (fd->base_para->max_pyramid_width > MAX_PYRAMID_WIDTH
		|| fd->base_para->max_pyramid_height > MAX_PYRAMID_HEIGHT) {
		aie_dev_info(fd->dev, "max pyramid size too big: w: %d, h: %d",
			fd->base_para->max_pyramid_width,
			fd->base_para->max_pyramid_height);
		goto max_pyramid_size_too_large;
	}

	aie_init_table(fd, fd->base_para->max_pyramid_width,
		       fd->base_para->max_pyramid_height);

	aie_get_data_size(fd, fd->base_para->max_img_width,
				      fd->base_para->max_img_height);

	ret = aie_alloc_output_buf(fd); //pyramid
	if (ret)
		goto output_fail;

	ret = aie_alloc_fddma_buf(fd); //inter-production
	if (ret)
		goto fddma_fail;

	aie_arrange_config(fd);
	aie_arrange_network(fd);

	aie_dev_info(fd->dev,
	"c(%llx/%llx/%llx)o(%llx/%llx/%llx/%llx)f(%llx/%llx/%llx/%llx/%llx/%llx)\n",
		fd->base_para->fd_rs_cfg_pa, fd->base_para->fd_fd_cfg_pa,
		fd->base_para->fd_yuv2rgb_cfg_pa, fd->rs_output_hw.pa, fd->fd_dma_hw.pa,
		fd->dma_para->fd_kernel_pa[0][0], fd->fd_attr_dma_hw.pa,
		fd->dma_para->fld_cv_pa[0], fd->dma_para->fld_fp_pa[0],
		fd->dma_para->fld_leafnode_pa[0], fd->dma_para->fld_tree02_pa[0],
		fd->dma_para->fld_shape_pa[0], fd->dma_para->fld_blink_weight_pa
	);
	aie_arrange_fddma_buf(fd);
	aie_arrange_attrdma_buf(fd);

	AIE_SYSTRACE_END();
	return ret;

fddma_fail:
	aie_free_output_buf(fd);
	err_tag++;

output_fail:
	err_tag++;

max_pyramid_size_too_large:
	kfree(fd->dma_para);
	fd->dma_para = NULL;
	aie_dev_info(fd->dev, "Failed to alloc aie buf: %d\n", err_tag);

	AIE_SYSTRACE_END();

	return ret;

}

static int aie_init(struct mtk_aie_dev *fd)
{
	int err_tag = 0;

	fd->fd_state = STATE_NA;

	fd->base_para = kmalloc(sizeof(struct aie_para), GFP_KERNEL);
	if (fd->base_para == NULL)
		return -ENOMEM;

	fd->attr_para = kmalloc(sizeof(struct aie_attr_para), GFP_KERNEL);
	if (fd->attr_para == NULL)
		goto attr_para_fail;
#ifdef FLD
	fd->fld_para = kmalloc(sizeof(struct aie_fld_para), GFP_KERNEL);
	if (fd->fld_para == NULL)
		goto fld_para_fail;
#endif
	fd->dma_para = kmalloc(sizeof(struct aie_fd_dma_para), GFP_KERNEL);
	if (fd->dma_para == NULL)
		goto dma_para_fail;

	fd->attr_para->r_idx = 0;
	fd->attr_para->w_idx = 0;

	fd->fd_state = STATE_INIT;

	return 0;

dma_para_fail:
	kfree(fd->attr_para);
	fd->attr_para = NULL;
	err_tag++;
#ifdef FLD
fld_para_fail:
	kfree(fd->fld_para);
	fd->fld_para = NULL;
	err_tag++;
#endif
attr_para_fail:
	kfree(fd->base_para);
	fd->base_para = NULL;
	err_tag++;

	aie_dev_info(fd->dev, "Failed to init aie: %d\n", err_tag);

	return -ENOMEM;
}

static void aie_uninit(struct mtk_aie_dev *fd)
{
	fd->fd_state = STATE_NA;

	aie_free_fddma_buf(fd);

	if (g_user_param.is_secure)
		aie_free_sec_buf(fd);
	else
		aie_free_output_buf(fd);

	if (fd->base_para != NULL) {
		kfree(fd->base_para);
		fd->base_para = NULL;
	}
	if (fd->attr_para != NULL) {
		kfree(fd->attr_para);
		fd->attr_para = NULL;
	}
	if (fd->dma_para != NULL) {
		kfree(fd->dma_para);
		fd->dma_para = NULL;
	}
#ifdef FLD
	if (fd->fld_para != NULL) {
		kfree(fd->fld_para);
		fd->fld_para = NULL;
	}
#endif
}

static int aie_prepare(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	int ret = 0;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	if (fd->fd_state != STATE_INIT) {
		aie_dev_info(fd->dev, "%s fd state fail: %d\n",
			 __func__, fd->fd_state);
		return -EINVAL;
	}

	memset(&fd->reg_cfg, 0, sizeof(fd->reg_cfg));

	if (aie_cfg->pyramid_base_width == 0) {
		fd->base_para->pyramid_width =
			fd->base_para->max_pyramid_width;
		fd->base_para->pyramid_height =
			fd->base_para->max_pyramid_height;
		fd->base_para->number_of_pyramid = 3;
	} else {
		if (aie_cfg->pyramid_base_width >
			fd->base_para->max_pyramid_width ||
		    aie_cfg->pyramid_base_height >
			fd->base_para->max_pyramid_height ||
		    aie_cfg->number_of_pyramid > 3 ||
		    aie_cfg->number_of_pyramid <= 0) {
			aie_dev_info(fd->dev, "err: base w: %d, h: %d, num: %d\n",
			    aie_cfg->pyramid_base_width,
			    aie_cfg->pyramid_base_height,
			    aie_cfg->number_of_pyramid);
			aie_dev_info(fd->dev, "err: max w: %d, h: %d\n",
			    fd->base_para->max_pyramid_width,
			    fd->base_para->max_pyramid_height);

			return -EINVAL;
		}

		fd->base_para->pyramid_height =
			fd->base_para->max_pyramid_height;
		fd->base_para->number_of_pyramid =
			aie_cfg->number_of_pyramid;
		if (aie_cfg->pyramid_base_width !=
			fd->base_para->pyramid_width) {
			aie_dev_dbg(fd->dev, "pre: %d, cur: %d, num: %d\n",
				fd->base_para->pyramid_width,
				aie_cfg->pyramid_base_width,
				fd->base_para->number_of_pyramid);
			fd->base_para->pyramid_width =
				aie_cfg->pyramid_base_width;

			aie_init_table(fd, fd->base_para->pyramid_width,
					fd->base_para->pyramid_height);
			aie_update_fddma_buf(fd);
		}
	}

	if ((aie_cfg->src_img_width > fd->base_para->max_img_width) ||
	    (aie_cfg->src_img_height > fd->base_para->max_img_height)) {
		aie_dev_info(
			fd->dev,
			"AIE error: Enque Size error, Src_WD: %d, Src_HT: %d\n",
			aie_cfg->src_img_width, aie_cfg->src_img_height);

		aie_dev_info(fd->dev, "AIE error: MAX_Src_WD: %d, MAX_Src_HT: %d\n",
			 fd->base_para->max_img_width,
			 fd->base_para->max_img_height);
		return -EINVAL;
	}

	//aie_reset_output_buf(fd, aie_cfg);

	fd->reg_cfg.fd_mode = aie_cfg->sel_mode;
	if (aie_cfg->sel_mode == FDMODE) {
		fd->reg_cfg.rs_adr = (u32)fd->base_para->fd_rs_cfg_pa;
		fd->reg_cfg.yuv2rgb_adr = (u32)fd->base_para->fd_yuv2rgb_cfg_pa;
		fd->reg_cfg.fd_adr = (u32)fd->base_para->fd_fd_cfg_pa +
			FD_CONFIG_SIZE * 4 * fd_loop_num /
			3 * (3 - aie_cfg->number_of_pyramid);

	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		fd->reg_cfg.yuv2rgb_adr =
			(u32)fd->base_para
				->attr_yuv2rgb_cfg_pa[fd->attr_para->w_idx];
		fd->reg_cfg.fd_adr =
			(u32)fd->base_para
				->attr_fd_cfg_pa[fd->attr_para->w_idx];
	} else {
		aie_dev_info(fd->dev, "AIE error, Mode: %d", aie_cfg->sel_mode);
		return -EINVAL;
	}

	ret = aie_update_cfg(fd, aie_cfg);
	if (ret)
		return ret;

	ret = aie_config_dram(fd, aie_cfg);
	if (ret)
		return ret;

	if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		fd->attr_para->w_idx =
			(fd->attr_para->w_idx + 1) % MAX_ENQUE_FRAME_NUM;
	}

	AIE_SYSTRACE_END();

	return ret;
}

#ifdef FDVT_USE_GCE
static void mtk_aie_job_timeout_work(struct mtk_aie_dev *fd)
{
	unsigned int i;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	aie_dev_info(fd->dev, "FD Job timeout!");

	aie_dev_info(fd->dev, "AIE mode:%d fmt:%d w:%d h:%d s:%d pw%d ph%d np%d dg%d",
		 fd->aie_cfg->sel_mode,
		 fd->aie_cfg->src_img_fmt,
		 fd->aie_cfg->src_img_width,
		 fd->aie_cfg->src_img_height,
		 fd->aie_cfg->src_img_stride,
		 fd->aie_cfg->pyramid_base_width,
		 fd->aie_cfg->pyramid_base_height,
		 fd->aie_cfg->number_of_pyramid,
		 fd->aie_cfg->rotate_degree);
	aie_dev_info(fd->dev, "roi%d x1:%d y1:%d x2:%d y2:%d pad%d l%d r%d d%d u%d f%d",
		 fd->aie_cfg->en_roi,
		 fd->aie_cfg->src_roi.x1,
		 fd->aie_cfg->src_roi.y1,
		 fd->aie_cfg->src_roi.x2,
		 fd->aie_cfg->src_roi.y2,
		 fd->aie_cfg->en_padding,
		 fd->aie_cfg->src_padding.left,
		 fd->aie_cfg->src_padding.right,
		 fd->aie_cfg->src_padding.down,
		 fd->aie_cfg->src_padding.up,
		 fd->aie_cfg->freq_level);

	aie_get_time(fd->tv, 7);
	aie_fdvt_dump_reg(fd);

	for (i = 0; i < MAX_DEBUG_TIMEVAL; i++)
		aie_dev_info(fd->dev, "tv[%d], %lld.%lld s",
			i, fd->tv[i] / 1000000000, fd->tv[i] % 1000000000);

	aie_irqhandle(fd);
	aie_reset(fd);

	AIE_SYSTRACE_END();
}

static void AIECmdqCB(struct cmdq_cb_data data)
{
	struct mtk_aie_dev *fd = (struct mtk_aie_dev *)data.data;
	bool isHwHang = (data.err == 0) ? false : true;

	fd->isHwHang = isHwHang;

	if (isHwHang)
		mtk_aie_job_timeout_work(fd);

	queue_work(fd->frame_done_wq, &fd->req_work.work);
}

static void AIECmdqSecCB(struct cmdq_cb_data data)
{
	struct mtk_aie_dev *fd = (struct mtk_aie_dev *)data.data;

	aie_dev_info(fd->dev, "AIE SEC CMDQ CB\n");
}


static void AieSecPktCB(struct cmdq_cb_data data)
{
	struct cmdq_pkt *sec_pkt = (struct cmdq_pkt *)data.data;

	cmdq_pkt_destroy(sec_pkt);
	g_sec_pkt = NULL;

}

void config_aie_cmdq_secure_init(struct mtk_aie_dev *fd)
{
	g_sec_pkt = cmdq_pkt_create(fd->fdvt_secure_clt);

#if CMDQ_SEC_READY
	cmdq_sec_pkt_set_data(g_sec_pkt, 0, 0, CMDQ_SEC_DEBUG, CMDQ_METAEX_TZMP);
	cmdq_sec_pkt_set_mtee(g_sec_pkt, true);
#endif

	cmdq_pkt_finalize_loop(g_sec_pkt);
	cmdq_pkt_flush_threaded(g_sec_pkt, AieSecPktCB, (void *)g_sec_pkt);
}

void aie_enable_secure_domain(struct mtk_aie_dev *fd)
{
	struct cmdq_pkt *pkt = NULL;

	pkt = cmdq_pkt_create(fd->fdvt_clt);
	cmdq_pkt_set_event(pkt, fd->fdvt_sec_wait);
	cmdq_pkt_wfe(pkt, fd->fdvt_sec_set);
	cmdq_pkt_flush_async(pkt, AIECmdqSecCB, (void *)fd);	/* flush and destry in cmdq*/
	cmdq_pkt_wait_complete(pkt);
	cmdq_pkt_destroy(pkt);
}

void aie_disable_secure_domain(struct mtk_aie_dev *fd)
{
	struct cmdq_pkt *pkt = NULL;

	pkt = cmdq_pkt_create(fd->fdvt_clt);
	cmdq_pkt_set_event(pkt, fd->fdvt_sec_wait);
	cmdq_pkt_wfe(pkt, fd->fdvt_sec_set);
	cmdq_pkt_flush_async(pkt, AIECmdqSecCB, (void *)fd);/* flush and destry in cmdq*/
	cmdq_pkt_wait_complete(pkt);
	cmdq_pkt_destroy(pkt);
}

static void config_aie_cmdq_hw(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	struct cmdq_pkt *pkt = NULL;
	unsigned int loop_num = 0;
	unsigned int loop_reg_val = 0;

	AIE_SYSTRACE_BEGIN("%s", __func__);

	aie_get_time(fd->tv, 1);
	pkt = cmdq_pkt_create(fd->fdvt_clt);
	aie_get_time(fd->tv, 2);

	fd->pkt = pkt;

	/*for early porting*/
	if (aie_cfg->sel_mode == FDMODE) {
		cmdq_pkt_write(pkt, NULL, FDVT_ENABLE_HW, 0x00000111,
			CMDQ_REG_MASK);
		loop_num = fd_loop_num / 3 * (aie_cfg->number_of_pyramid);
		loop_reg_val = (loop_num << 8) |
			(aie_cfg->number_of_pyramid - 1);
		cmdq_pkt_write(pkt, NULL, FDVT_LOOP_HW, loop_reg_val, CMDQ_REG_MASK);

		/* write clear irq */
		cmdq_pkt_write(pkt, NULL, FDVT_INT_EN_HW, 0x11, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_INT_HW, 0x1, CMDQ_REG_MASK);

		cmdq_pkt_write(pkt, NULL, FDVT_RS_CON_BASE_ADR_HW,
				fd->reg_cfg.rs_adr, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_FD_CON_BASE_ADR_HW,
				fd->reg_cfg.fd_adr, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_YUV2RGB_CON_BASE_ADR_HW,
				fd->reg_cfg.yuv2rgb_adr, CMDQ_REG_MASK);

		cmdq_pkt_write(pkt, NULL, FDVT_START_HW, 0x1, CMDQ_REG_MASK);

		if (fd->is_cmdq_polling)
			cmdq_pkt_poll_timeout(pkt, 0x1, SUBSYS_NO_SUPPORT,
				FDVT_INT_HW, 0x1, AIE_POLL_TIME_INFINI,
				CMDQ_GPR_R03 + CMDQ_GPR_R03_IDX);
		else
			cmdq_pkt_wfe(pkt, fd->fdvt_event_id);

		/*cmdqRecWait(handle, CMDQ_EVENT_IPE_EVENT_TX_FRAME_DONE_0);*/
		cmdq_pkt_write(pkt, NULL, FDVT_START_HW, 0x0, CMDQ_REG_MASK);

	} else if (aie_cfg->sel_mode == ATTRIBUTEMODE) {
		cmdq_pkt_write(pkt, NULL, FDVT_ENABLE_HW, 0x00000101,
			       CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_LOOP_HW, 0x00001A00,
			       CMDQ_REG_MASK);

		/* write clear irq */
		cmdq_pkt_write(pkt, NULL, FDVT_INT_EN_HW, 0x11, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_INT_HW, 0x1, CMDQ_REG_MASK);

		cmdq_pkt_write(pkt, NULL, FDVT_RS_CON_BASE_ADR_HW,
			       fd->reg_cfg.rs_adr,
			       CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_FD_CON_BASE_ADR_HW,
			       fd->reg_cfg.fd_adr,
			       CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_YUV2RGB_CON_BASE_ADR_HW,
			       fd->reg_cfg.yuv2rgb_adr,
			       CMDQ_REG_MASK);

		cmdq_pkt_write(pkt, NULL, FDVT_START_HW, 0x1, CMDQ_REG_MASK);

		if (fd->is_cmdq_polling)
			cmdq_pkt_poll_timeout(pkt, 0x1, SUBSYS_NO_SUPPORT,
				FDVT_INT_HW, 0x1, AIE_POLL_TIME_INFINI,
				CMDQ_GPR_R03 + CMDQ_GPR_R03_IDX);
		else
			cmdq_pkt_wfe(pkt, fd->fdvt_event_id);
		/*cmdqRecWait(handle, CMDQ_EVENT_IPE_EVENT_TX_FRAME_DONE_0);*/
		cmdq_pkt_write(pkt, NULL, FDVT_START_HW, 0x0, CMDQ_REG_MASK);

	} else if (aie_cfg->sel_mode == FLDMODE) {
		int i = 0;

		cmdq_pkt_write(pkt, NULL, FDVT_BASE_HW + AIE_START_REG, 0x10, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_DMA_CTL_HW, 0x00011111, CMDQ_REG_MASK);

		for (i = 0; i < aie_cfg->fld_face_num; i++) {
			cmdq_pkt_write(pkt, NULL, fld_face_info_cmdq_idx_0[i],
					(aie_cfg->fld_input[i].fld_in_crop.y1 << 12) |
					aie_cfg->fld_input[i].fld_in_crop.x1, CMDQ_REG_MASK);
			cmdq_pkt_write(pkt, NULL, fld_face_info_cmdq_idx_1[i],
					(aie_cfg->fld_input[i].fld_in_rop << 28) |
					(aie_cfg->fld_input[i].fld_in_rip << 24) |
					(aie_cfg->fld_input[i].fld_in_crop.y2 << 12) |
					aie_cfg->fld_input[i].fld_in_crop.x2, CMDQ_REG_MASK);
		}

		cmdq_pkt_write(pkt, NULL, FLD_CMDQ_FACE_NUM,
				(fld_point << 8) | aie_cfg->fld_face_num, CMDQ_REG_MASK);

		/* Buffer resolution */
		cmdq_pkt_write(pkt, NULL, FLD_CMDQ_SRC_SIZE,
				(aie_cfg->src_img_height << 16) | aie_cfg->src_img_width,
				CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FLD_CMDQ_SRC_PITCH,
				aie_cfg->src_img_stride,
				CMDQ_REG_MASK);

		/* write clear irq */
		cmdq_pkt_write(pkt, NULL, FDVT_INT_EN_HW, 0x11, CMDQ_REG_MASK);
		cmdq_pkt_write(pkt, NULL, FDVT_INT_HW, 0x1, CMDQ_REG_MASK);

		/*fld mode + trigger start*/
		cmdq_pkt_write(pkt, NULL, FDVT_BASE_HW + AIE_START_REG, 0x11, CMDQ_REG_MASK);

		if (fd->is_cmdq_polling)
			cmdq_pkt_poll_timeout(pkt, 0x1, SUBSYS_NO_SUPPORT,
				FDVT_INT_HW, 0x1, AIE_POLL_TIME_INFINI,
				CMDQ_GPR_R03 + CMDQ_GPR_R03_IDX);
		else
			cmdq_pkt_wfe(pkt, fd->fdvt_event_id);
		/*cmdqRecWait(handle, CMDQ_EVENT_IPE_EVENT_TX_FRAME_DONE_0);*/
		cmdq_pkt_write(pkt, NULL, FDVT_START_HW, 0x0, CMDQ_REG_MASK);
	}

	aie_get_time(fd->tv, 3);
	cmdq_pkt_flush_async(pkt, AIECmdqCB, (void *)fd);	/* flush and destry in cmdq*/
	aie_get_time(fd->tv, 4);

	AIE_SYSTRACE_END();
}
#endif

static void aie_execute(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
#ifndef FDVT_USE_GCE

	unsigned int loop_num = 0;
	unsigned int loop_reg_val = 0;

	if (aie_cfg->sel_mode == 0) {
		writel(0x00000111, fd->fd_base + AIE_ENABLE_REG);
		loop_num = fd_loop_num / 3 * (aie_cfg->number_of_pyramid);
		loop_reg_val = (loop_num << 8) |
			(aie_cfg->number_of_pyramid - 1);
		writel(loop_reg_val, fd->fd_base + AIE_LOOP_REG);
		writel(0x1, fd->fd_base + AIE_INT_EN_REG);
		writel(fd->reg_cfg.rs_adr,
			fd->fd_base + AIE_RS_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.fd_adr,
			fd->fd_base + AIE_FD_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.yuv2rgb_adr,
			fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_REG);
		writel(0x1, fd->fd_base + AIE_START_REG);
	} else if (aie_cfg->sel_mode == 1) {
		writel(0x00000101, fd->fd_base + AIE_ENABLE_REG);
		writel(0x00001A00, fd->fd_base + AIE_LOOP_REG);
		writel(0x1, fd->fd_base + AIE_INT_EN_REG);
		writel(fd->reg_cfg.rs_adr,
		       fd->fd_base + AIE_RS_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.fd_adr,
		       fd->fd_base + AIE_FD_CON_BASE_ADR_REG);
		writel(fd->reg_cfg.yuv2rgb_adr,
		       fd->fd_base + AIE_YUV2RGB_CON_BASE_ADR_REG);
		writel(0x1, fd->fd_base + AIE_START_REG);
	} else if (aie_cfg->sel_mode == 3) {
		int i = 0;

		writel(0x10, fd->fd_base + AIE_START_REG);
		writel(0x00011111, fd->fd_base + AIE_DMA_CTL_REG);
		writel(0x01111111, fd->fd_base + FLD_EN);
		for (i = 0; i < aie_cfg->fld_face_num; i++) {
			writel(aie_cfg->src_img_addr, fd->fd_base + FLD_BASE_ADDR_FACE_0 + i * 0x4);
			writel((aie_cfg->fld_input[i].fld_in_crop.x1 << 16) |
			aie_cfg->fld_input[i].fld_in_crop.y1,
						fd->fd_base + fld_face_info_0[i]);
			writel((aie_cfg->fld_input[i].fld_in_crop.x2 << 16) |
						aie_cfg->fld_input[i].fld_in_crop.y2,
						fd->fd_base + fld_face_info_1[i]);
			writel(aie_cfg->fld_input[i].fld_in_rip << 4 |
						aie_cfg->fld_input[i].fld_in_rop,
						fd->fd_base + fld_face_info_2[i]);
		}


		writel((fld_forest << 16) | (aie_cfg->fld_face_num << 28) | fld_point,
								fd->fd_base + FLD_MODEL_PARA1);
		writel((0xd << 16) | 0xfe9, fd->fd_base + FLD_MODEL_PARA14);

		/*fld kernel model pa setting*/
		for (i = 0; i < FLD_MAX_INPUT; i++) {
			writel(fd->dma_para->fld_tree02_pa[i], fd->fd_base + fld_pl_in_addr_0[i]);
			writel(fd->dma_para->fld_tree13_pa[i], fd->fd_base + fld_pl_in_addr_1[i]);
			writel(fd->dma_para->fld_cv_pa[i], fd->fd_base + fld_pl_in_addr_2[i]);
			writel(fd->dma_para->fld_fp_pa[i], fd->fd_base + fld_pl_in_addr_3[i]);
			writel(fd->dma_para->fld_leafnode_pa[i], fd->fd_base + fld_sh_in_addr[i]);
		}
		writel(fd->dma_para->fld_blink_weight_pa, fd->fd_base + FLD_BS_IN_BASE_ADDR_14);

		writel((aie_cfg->src_img_width << 16) |
					aie_cfg->src_img_height, fd->fd_base + FLD_SRC_WD_HT);

		/*input settings*/
		writel(0x007c003f, fd->fd_base + FLD_PL_IN_SIZE_0);
		writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_0);
		writel(0x007c003f, fd->fd_base + FLD_PL_IN_SIZE_1);
		writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_1);
		writel(0x0016003f, fd->fd_base + FLD_PL_IN_SIZE_2_0);
		writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_0);
		writel(0x0013003f, fd->fd_base + FLD_PL_IN_SIZE_2_1);
		writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_1);
		writel(0x0013003f, fd->fd_base + FLD_PL_IN_SIZE_2_2);
		writel(0x0040000f, fd->fd_base + FLD_PL_IN_STRIDE_2_2);
		writel(0x00a6001f, fd->fd_base + FLD_PL_IN_SIZE_3);
		writel(0x0020000f, fd->fd_base + FLD_PL_IN_STRIDE_3);

		/*output setting*/
		writel((2400 * aie_cfg->fld_face_num - 1) << 16 | 127,
						fd->fd_base + FLD_SH_IN_SIZE_0);
		writel(0x0010000f, fd->fd_base + FLD_SH_IN_STRIDE_0);
		writel(fd->dma_para->fld_output_pa, fd->fd_base + FLD_TR_OUT_BASE_ADDR_0);
		writel((aie_cfg->fld_face_num - 1) << 16 | 0x6f, fd->fd_base + FLD_TR_OUT_SIZE_0);
		writel(0x0070000f, fd->fd_base + FLD_TR_OUT_STRIDE_0);
		writel(fd->dma_para->fld_output_pa, fd->fd_base + FLD_PP_OUT_BASE_ADDR_0);
		writel((aie_cfg->fld_face_num - 1) << 16 | 0x6f, fd->fd_base + FLD_PP_OUT_SIZE_0);
		writel(0x0070000f, fd->fd_base + FLD_PP_OUT_STRIDE_0);

		/*cv score*/
		writel(0x00000001, fd->fd_base + FLD_BS_BIAS);
		writel(0x0000b835, fd->fd_base + FLD_CV_FM_RANGE_0); //8E8
		writel(0xffff5cba, fd->fd_base + FLD_CV_FM_RANGE_1); //8EC
		writel(0x00005ed5, fd->fd_base + FLD_CV_PM_RANGE_0); //8F0
		writel(0xffff910d, fd->fd_base + FLD_CV_PM_RANGE_1); //8F4 //temp 310
		writel(0x0000031e, fd->fd_base + FLD_BS_RANGE_0); //8F8
		writel(0xfffffcae, fd->fd_base + FLD_BS_RANGE_1); //8FC

		/*fld mode + trigger start*/
		writel(0x11, fd->fd_base + AIE_START_REG);
	}
#else
	config_aie_cmdq_hw(fd, aie_cfg);
#endif
}

static void aie_irqhandle(struct mtk_aie_dev *fd)
{
	if (fd->is_shutdown) {
		aie_dev_info(fd->dev, "%s: skip for shutdown", __func__);
		return;
	}

	writel(0x0, fd->fd_base + AIE_START_REG);

	/* write clear irq */
	writel(0x11, fd->fd_base + AIE_INT_EN_REG);
	writel(0x1, fd->fd_base + AIE_INT_REG);
}

/* return aie_cfg to user space */
static void aie_get_fd_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	u32 fd_result_hw, fd_result_1_hw;
	u32 fd_total_num;
	u32 fd_pyramid_num[PYM_NUM];

	aie_cfg->sel_mode = fd->base_para->sel_mode;
	aie_cfg->rotate_degree = fd->base_para->rotate_degree;
	aie_cfg->src_img_addr = fd->base_para->src_img_addr;
	aie_cfg->src_img_addr_uv = fd->base_para->src_img_addr_uv;
	aie_cfg->src_img_width = fd->base_para->img_width;
	aie_cfg->src_img_height = fd->base_para->img_height;
	aie_cfg->src_img_fmt = fd->base_para->src_img_fmt;
	aie_cfg->fd_version = FD_VERSION;
	aie_cfg->attr_version = ATTR_VERSION;

	if (!fd->is_shutdown) {
		fd->reg_cfg.hw_result = readl(fd->fd_base + AIE_RESULT_0_REG);
		fd->reg_cfg.hw_result1 = readl(fd->fd_base + AIE_RESULT_1_REG);
	}

	fd_result_hw = fd->reg_cfg.hw_result;
	fd_result_1_hw = fd->reg_cfg.hw_result1;
	fd_total_num = fd_result_hw & 0xFFF;
	fd_pyramid_num[0] = (fd_result_hw & 0xFFF0000) >> 16;
	fd_pyramid_num[1] = fd_result_1_hw & 0xFFF;
	fd_pyramid_num[2] = (fd_result_1_hw & 0xFFF0000) >> 16;

	aie_cfg->fd_out.fd_total_num = fd_total_num;
	aie_cfg->fd_out.fd_pyramid0_num = fd_pyramid_num[0];
	aie_cfg->fd_out.fd_pyramid1_num = fd_pyramid_num[1];
	aie_cfg->fd_out.fd_pyramid2_num = fd_pyramid_num[2];

}

static void aie_get_attr_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	aie_cfg->sel_mode = fd->attr_para->sel_mode[fd->attr_para->r_idx];
	aie_cfg->rotate_degree =
		fd->attr_para->rotate_degree[fd->attr_para->r_idx];
	aie_cfg->src_img_addr =
		fd->attr_para->src_img_addr[fd->attr_para->r_idx];
	aie_cfg->src_img_addr_uv =
		fd->attr_para->src_img_addr_uv[fd->attr_para->r_idx];
	aie_cfg->src_img_width = fd->attr_para->img_width[fd->attr_para->r_idx];
	aie_cfg->src_img_height =
		fd->attr_para->img_height[fd->attr_para->r_idx];
	aie_cfg->src_img_fmt = fd->attr_para->src_img_fmt[fd->attr_para->r_idx];
	aie_cfg->fd_version = FD_VERSION;
	aie_cfg->attr_version = ATTR_VERSION;

	fd->attr_para->r_idx = (fd->attr_para->r_idx + 1) % MAX_ENQUE_FRAME_NUM;
}

static void aie_get_fld_result(struct mtk_aie_dev *fd, struct aie_enq_info *aie_cfg)
{
	aie_cfg->sel_mode = fd->fld_para->sel_mode;
	aie_cfg->src_img_width = fd->fld_para->img_width;
	aie_cfg->src_img_height = fd->fld_para->img_height;
	aie_cfg->fd_version = FD_VERSION;
	aie_cfg->attr_version = ATTR_VERSION;
	aie_cfg->src_img_addr = fd->fld_para->src_img_addr;
	aie_cfg->fld_face_num = fd->fld_para->face_num;

	memcpy((char *)&(aie_cfg->fld_input[0]), (char *)fd->fld_para->fld_input,
		sizeof(struct FLD_CROP_RIP_ROP) * aie_cfg->fld_face_num);
}

static void aie_config_fld_buf_reg(struct mtk_aie_dev *fd)
{
	if (fd->is_shutdown) {
		aie_dev_info(fd->dev, "%s: skip for shutdown", __func__);
		return;
	}

	writel(
		AIE_IOVA(fd->img_y),
		fd->fd_base + FLD_IMG_BASE_ADDR);
	writel(AIE_IOVA(fd->dma_para->fld_output_pa),
		fd->fd_base + FLD_PP_BASE_ADDR);
}

static void aie_arrange_config(struct mtk_aie_dev *fd)
{
	unsigned char *va = (unsigned char *)fd->config_kva;
	unsigned long long pa = fd->config_pa;
	unsigned int i;
	unsigned int msb_bit = 0;
	g_fd_rs_config_offset = 0;

	/* arrange iova */
	/* rs config */
	fd->base_para->fd_rs_cfg_pa = pa;
	msb_bit = (pa & 0Xf00000000) >> 32;
	if (!fd->is_shutdown)
		writel(msb_bit, fd->fd_base + FDVT_RS_CON_BASE_ADR_MSB);

	pa += AIE_ALIGN32(fdvt_rs_confi_frame01_size);
	g_fd_yuv2rgb_config_offset =
		g_fd_rs_config_offset + AIE_ALIGN32(fdvt_rs_confi_frame01_size);

	/* yuv2rgb config */
	fd->base_para->fd_yuv2rgb_cfg_pa = pa;
	msb_bit = (pa & 0Xf00000000) >> 32;
	if (!fd->is_shutdown)
		writel(msb_bit, fd->fd_base + FDVT_YUV2RGB_CON_BASE_ADR_MSB);

	pa += AIE_ALIGN32(fdvt_yuv2rgb_confi_frame01_size);

	for (i = 0; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_yuv2rgb_cfg_pa[i] = pa;
		pa += AIE_ALIGN32(attr_yuv2rgb_confi_frame01_size);
	}
	g_fd_fd_config_offset =
		g_fd_yuv2rgb_config_offset +
		AIE_ALIGN32(fdvt_yuv2rgb_confi_frame01_size) +
		MAX_ENQUE_FRAME_NUM * AIE_ALIGN32(attr_yuv2rgb_confi_frame01_size);

	/* fd config */
	fd->base_para->fd_fd_cfg_pa = pa;
	msb_bit = (pa & 0Xf00000000) >> 32;
	if (!fd->is_shutdown)
		writel(msb_bit, fd->fd_base + FDVT_FD_CON_BASE_ADR_MSB);

	pa += AIE_ALIGN32(fdvt_fd_confi_frame01_size);

	for (i = 0; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_fd_cfg_pa[i] = pa;
		pa += AIE_ALIGN32(attr_fd_confi_frame01_size);
	}

	/* arrange va */
	/* rs config */
	fd->base_para->fd_rs_cfg_va = va;
	va += AIE_ALIGN32(fdvt_rs_confi_frame01_size);

	/* yuv2rgb config */
	fd->base_para->fd_yuv2rgb_cfg_va = va;
	va += AIE_ALIGN32(fdvt_yuv2rgb_confi_frame01_size);

	for (i = 0; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_yuv2rgb_cfg_va[i] = va;
		va += AIE_ALIGN32(attr_yuv2rgb_confi_frame01_size);
	}

	/* fd config */
	fd->base_para->fd_fd_cfg_va = va;
	va += AIE_ALIGN32(fdvt_fd_confi_frame01_size);

	for (i = 0; i < MAX_ENQUE_FRAME_NUM; i++) {
		fd->base_para->attr_fd_cfg_va[i] = va;
		va += AIE_ALIGN32(attr_fd_confi_frame01_size);
	}
}

static void aie_arrange_network(struct mtk_aie_dev *fd)
{
	unsigned char *va = (unsigned char *)fd->model_kva;
	unsigned long long pa = fd->model_pa;
	unsigned int i;

	/* arrange iova */
	/* fd kernel model */
	fd->dma_para->fd_kernel_pa[0][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop00_0_frame01_size);

	fd->dma_para->fd_kernel_pa[0][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop00_1_frame01_size);

	fd->dma_para->fd_kernel_pa[1][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop01_0_frame01_size);

	fd->dma_para->fd_kernel_pa[1][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop01_1_frame01_size);

	fd->dma_para->fd_kernel_pa[2][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop02_0_frame01_size);

	fd->dma_para->fd_kernel_pa[2][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop02_1_frame01_size);

	fd->dma_para->fd_kernel_pa[3][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop03_0_frame01_size);

	fd->dma_para->fd_kernel_pa[3][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop03_1_frame01_size);

	fd->dma_para->fd_kernel_pa[4][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop04_0_frame01_size);

	fd->dma_para->fd_kernel_pa[4][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop04_1_frame01_size);

	fd->dma_para->fd_kernel_pa[5][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop05_0_frame01_size);

	fd->dma_para->fd_kernel_pa[5][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop05_1_frame01_size);

	fd->dma_para->fd_kernel_pa[6][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop06_0_frame01_size);

	fd->dma_para->fd_kernel_pa[6][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop06_1_frame01_size);

	fd->dma_para->fd_kernel_pa[7][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop07_0_frame01_size);

	fd->dma_para->fd_kernel_pa[7][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop07_1_frame01_size);

	fd->dma_para->fd_kernel_pa[8][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop08_0_frame01_size);

	fd->dma_para->fd_kernel_pa[8][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop08_1_frame01_size);

	fd->dma_para->fd_kernel_pa[9][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop09_0_frame01_size);

	fd->dma_para->fd_kernel_pa[9][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop09_1_frame01_size);

	fd->dma_para->fd_kernel_pa[10][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop10_0_frame01_size);

	fd->dma_para->fd_kernel_pa[10][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop10_1_frame01_size);

	fd->dma_para->fd_kernel_pa[11][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop11_0_frame01_size);

	fd->dma_para->fd_kernel_pa[11][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop11_1_frame01_size);

	fd->dma_para->fd_kernel_pa[12][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop12_0_frame01_size);

	fd->dma_para->fd_kernel_pa[12][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop12_1_frame01_size);

	fd->dma_para->fd_kernel_pa[13][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop13_0_frame01_size);

	fd->dma_para->fd_kernel_pa[13][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop13_1_frame01_size);

	fd->dma_para->fd_kernel_pa[14][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop14_0_frame01_size);

	fd->dma_para->fd_kernel_pa[14][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop14_1_frame01_size);

	fd->dma_para->fd_kernel_pa[15][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop15_0_frame01_size);

	fd->dma_para->fd_kernel_pa[15][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop15_1_frame01_size);

	fd->dma_para->fd_kernel_pa[16][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop16_0_frame01_size);

	fd->dma_para->fd_kernel_pa[16][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop16_1_frame01_size);

	fd->dma_para->fd_kernel_pa[17][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop17_0_frame01_size);

	fd->dma_para->fd_kernel_pa[17][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop17_1_frame01_size);

	fd->dma_para->fd_kernel_pa[18][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop18_0_frame01_size);

	fd->dma_para->fd_kernel_pa[18][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop18_1_frame01_size);

	fd->dma_para->fd_kernel_pa[19][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop19_0_frame01_size);

	fd->dma_para->fd_kernel_pa[19][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop19_1_frame01_size);

	fd->dma_para->fd_kernel_pa[20][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop20_0_frame01_size);

	fd->dma_para->fd_kernel_pa[20][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop20_1_frame01_size);

	fd->dma_para->fd_kernel_pa[21][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop21_0_frame01_size);

	fd->dma_para->fd_kernel_pa[21][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop21_1_frame01_size);

	fd->dma_para->fd_kernel_pa[22][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop22_0_frame01_size);

	fd->dma_para->fd_kernel_pa[22][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop22_1_frame01_size);

	fd->dma_para->fd_kernel_pa[23][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop23_0_frame01_size);

	fd->dma_para->fd_kernel_pa[23][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop23_1_frame01_size);

	fd->dma_para->fd_kernel_pa[24][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop24_0_frame01_size);

	fd->dma_para->fd_kernel_pa[24][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop24_1_frame01_size);

	fd->dma_para->fd_kernel_pa[25][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop25_0_frame01_size);

	fd->dma_para->fd_kernel_pa[25][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop25_1_frame01_size);

	fd->dma_para->fd_kernel_pa[26][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop26_0_frame01_size);

	fd->dma_para->fd_kernel_pa[26][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop26_1_frame01_size);

	fd->dma_para->fd_kernel_pa[27][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop27_0_frame01_size);

	fd->dma_para->fd_kernel_pa[27][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop27_1_frame01_size);

	fd->dma_para->fd_kernel_pa[29][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop29_0_frame01_size);

	fd->dma_para->fd_kernel_pa[29][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop29_1_frame01_size);

	fd->dma_para->fd_kernel_pa[30][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop30_0_frame01_size);

	fd->dma_para->fd_kernel_pa[30][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop30_1_frame01_size);

	fd->dma_para->fd_kernel_pa[31][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop31_0_frame01_size);

	fd->dma_para->fd_kernel_pa[31][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop31_1_frame01_size);

	fd->dma_para->fd_kernel_pa[32][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop32_0_frame01_size);

	fd->dma_para->fd_kernel_pa[32][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop32_1_frame01_size);

	fd->dma_para->fd_kernel_pa[33][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop33_0_frame01_size);

	fd->dma_para->fd_kernel_pa[33][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop33_1_frame01_size);

	fd->dma_para->fd_kernel_pa[34][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop34_0_frame01_size);

	fd->dma_para->fd_kernel_pa[34][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop34_1_frame01_size);

	fd->dma_para->fd_kernel_pa[35][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop35_0_frame01_size);

	fd->dma_para->fd_kernel_pa[35][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop35_1_frame01_size);

	fd->dma_para->fd_kernel_pa[36][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop36_0_frame01_size);

	fd->dma_para->fd_kernel_pa[36][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop36_1_frame01_size);

	fd->dma_para->fd_kernel_pa[37][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop37_0_frame01_size);

	fd->dma_para->fd_kernel_pa[37][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop37_1_frame01_size);

	fd->dma_para->fd_kernel_pa[38][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop38_0_frame01_size);

	fd->dma_para->fd_kernel_pa[38][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop38_1_frame01_size);

	fd->dma_para->fd_kernel_pa[39][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop39_0_frame01_size);

	fd->dma_para->fd_kernel_pa[39][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop39_1_frame01_size);

	fd->dma_para->fd_kernel_pa[40][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop40_0_frame01_size);

	fd->dma_para->fd_kernel_pa[40][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop40_1_frame01_size);

	fd->dma_para->fd_kernel_pa[41][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop41_0_frame01_size);

	fd->dma_para->fd_kernel_pa[41][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop41_1_frame01_size);

	fd->dma_para->fd_kernel_pa[42][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop42_0_frame01_size);

	fd->dma_para->fd_kernel_pa[42][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop42_1_frame01_size);

	fd->dma_para->fd_kernel_pa[43][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop43_0_frame01_size);

	fd->dma_para->fd_kernel_pa[43][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop43_1_frame01_size);

	fd->dma_para->fd_kernel_pa[44][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop44_0_frame01_size);

	fd->dma_para->fd_kernel_pa[44][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop44_1_frame01_size);

	fd->dma_para->fd_kernel_pa[45][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop45_0_frame01_size);

	fd->dma_para->fd_kernel_pa[45][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop45_1_frame01_size);

	fd->dma_para->fd_kernel_pa[46][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop46_0_frame01_size);

	fd->dma_para->fd_kernel_pa[46][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop46_1_frame01_size);

	fd->dma_para->fd_kernel_pa[47][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop47_0_frame01_size);

	fd->dma_para->fd_kernel_pa[47][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop47_1_frame01_size);

	fd->dma_para->fd_kernel_pa[48][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop48_0_frame01_size);

	fd->dma_para->fd_kernel_pa[48][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop48_1_frame01_size);

	fd->dma_para->fd_kernel_pa[49][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop49_0_frame01_size);

	fd->dma_para->fd_kernel_pa[49][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop49_1_frame01_size);

	fd->dma_para->fd_kernel_pa[50][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop50_0_frame01_size);

	fd->dma_para->fd_kernel_pa[50][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop50_1_frame01_size);

	fd->dma_para->fd_kernel_pa[51][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop51_0_frame01_size);

	fd->dma_para->fd_kernel_pa[51][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop51_1_frame01_size);

	fd->dma_para->fd_kernel_pa[52][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop52_0_frame01_size);

	fd->dma_para->fd_kernel_pa[52][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop52_1_frame01_size);

	fd->dma_para->fd_kernel_pa[53][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop53_0_frame01_size);

	fd->dma_para->fd_kernel_pa[53][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop53_1_frame01_size);

	fd->dma_para->fd_kernel_pa[54][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop54_0_frame01_size);

	fd->dma_para->fd_kernel_pa[54][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop54_1_frame01_size);

	fd->dma_para->fd_kernel_pa[55][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop55_0_frame01_size);

	fd->dma_para->fd_kernel_pa[55][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop55_1_frame01_size);

	fd->dma_para->fd_kernel_pa[56][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop56_0_frame01_size);

	fd->dma_para->fd_kernel_pa[56][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop56_1_frame01_size);

	fd->dma_para->fd_kernel_pa[58][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop58_0_frame01_size);

	fd->dma_para->fd_kernel_pa[58][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop58_1_frame01_size);

	fd->dma_para->fd_kernel_pa[59][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop59_0_frame01_size);

	fd->dma_para->fd_kernel_pa[59][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop59_1_frame01_size);

	fd->dma_para->fd_kernel_pa[60][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop60_0_frame01_size);

	fd->dma_para->fd_kernel_pa[60][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop60_1_frame01_size);

	fd->dma_para->fd_kernel_pa[61][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop61_0_frame01_size);

	fd->dma_para->fd_kernel_pa[61][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop61_1_frame01_size);

	fd->dma_para->fd_kernel_pa[62][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop62_0_frame01_size);

	fd->dma_para->fd_kernel_pa[62][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop62_1_frame01_size);

	fd->dma_para->fd_kernel_pa[63][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop63_0_frame01_size);

	fd->dma_para->fd_kernel_pa[63][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop63_1_frame01_size);

	fd->dma_para->fd_kernel_pa[64][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop64_0_frame01_size);

	fd->dma_para->fd_kernel_pa[64][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop64_1_frame01_size);

	fd->dma_para->fd_kernel_pa[65][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop65_0_frame01_size);

	fd->dma_para->fd_kernel_pa[65][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop65_1_frame01_size);

	fd->dma_para->fd_kernel_pa[66][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop66_0_frame01_size);

	fd->dma_para->fd_kernel_pa[66][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop66_1_frame01_size);

	fd->dma_para->fd_kernel_pa[67][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop67_0_frame01_size);

	fd->dma_para->fd_kernel_pa[67][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop67_1_frame01_size);

	fd->dma_para->fd_kernel_pa[68][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop68_0_frame01_size);

	fd->dma_para->fd_kernel_pa[68][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop68_1_frame01_size);

	fd->dma_para->fd_kernel_pa[69][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop69_0_frame01_size);

	fd->dma_para->fd_kernel_pa[69][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop69_1_frame01_size);

	fd->dma_para->fd_kernel_pa[70][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop70_0_frame01_size);

	fd->dma_para->fd_kernel_pa[70][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop70_1_frame01_size);

	fd->dma_para->fd_kernel_pa[71][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop71_0_frame01_size);

	fd->dma_para->fd_kernel_pa[71][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop71_1_frame01_size);

	fd->dma_para->fd_kernel_pa[72][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop72_0_frame01_size);

	fd->dma_para->fd_kernel_pa[72][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop72_1_frame01_size);

	fd->dma_para->fd_kernel_pa[73][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop73_0_frame01_size);

	fd->dma_para->fd_kernel_pa[73][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop73_1_frame01_size);

	fd->dma_para->fd_kernel_pa[74][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop74_0_frame01_size);

	fd->dma_para->fd_kernel_pa[74][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop74_1_frame01_size);

	fd->dma_para->fd_kernel_pa[75][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop75_0_frame01_size);

	fd->dma_para->fd_kernel_pa[75][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop75_1_frame01_size);

	fd->dma_para->fd_kernel_pa[76][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop76_0_frame01_size);

	fd->dma_para->fd_kernel_pa[76][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop76_1_frame01_size);

	fd->dma_para->fd_kernel_pa[77][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop77_0_frame01_size);

	fd->dma_para->fd_kernel_pa[77][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop77_1_frame01_size);

	fd->dma_para->fd_kernel_pa[78][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop78_0_frame01_size);

	fd->dma_para->fd_kernel_pa[78][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop78_1_frame01_size);

	fd->dma_para->fd_kernel_pa[79][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop79_0_frame01_size);

	fd->dma_para->fd_kernel_pa[79][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop79_1_frame01_size);

	fd->dma_para->fd_kernel_pa[80][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop80_0_frame01_size);

	fd->dma_para->fd_kernel_pa[80][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop80_1_frame01_size);

	fd->dma_para->fd_kernel_pa[81][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop81_0_frame01_size);

	fd->dma_para->fd_kernel_pa[81][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop81_1_frame01_size);

	fd->dma_para->fd_kernel_pa[82][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop82_0_frame01_size);

	fd->dma_para->fd_kernel_pa[82][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop82_1_frame01_size);

	fd->dma_para->fd_kernel_pa[83][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop83_0_frame01_size);

	fd->dma_para->fd_kernel_pa[83][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop83_1_frame01_size);

	fd->dma_para->fd_kernel_pa[84][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop84_0_frame01_size);

	fd->dma_para->fd_kernel_pa[84][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop84_1_frame01_size);

	fd->dma_para->fd_kernel_pa[85][0] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop85_0_frame01_size);

	fd->dma_para->fd_kernel_pa[85][1] = pa;
	pa += AIE_ALIGN32(fdvt_kernel_bias_loop85_1_frame01_size);

	/* attribute kernel model */
	fd->dma_para->attr_kernel_pa[0][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop00_0_frame01_size);

	fd->dma_para->attr_kernel_pa[0][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop00_1_frame01_size);

	fd->dma_para->attr_kernel_pa[1][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop01_0_frame01_size);

	fd->dma_para->attr_kernel_pa[1][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop01_1_frame01_size);

	fd->dma_para->attr_kernel_pa[2][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop02_0_frame01_size);

	fd->dma_para->attr_kernel_pa[2][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop02_1_frame01_size);

	fd->dma_para->attr_kernel_pa[3][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop03_0_frame01_size);

	fd->dma_para->attr_kernel_pa[3][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop03_1_frame01_size);

	fd->dma_para->attr_kernel_pa[4][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop04_0_frame01_size);

	fd->dma_para->attr_kernel_pa[4][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop04_1_frame01_size);

	fd->dma_para->attr_kernel_pa[5][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop05_0_frame01_size);

	fd->dma_para->attr_kernel_pa[5][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop05_1_frame01_size);

	fd->dma_para->attr_kernel_pa[6][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop06_0_frame01_size);

	fd->dma_para->attr_kernel_pa[6][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop06_1_frame01_size);

	fd->dma_para->attr_kernel_pa[7][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop07_0_frame01_size);

	fd->dma_para->attr_kernel_pa[7][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop07_1_frame01_size);

	fd->dma_para->attr_kernel_pa[8][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop08_0_frame01_size);

	fd->dma_para->attr_kernel_pa[8][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop08_1_frame01_size);

	fd->dma_para->attr_kernel_pa[9][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop09_0_frame01_size);

	fd->dma_para->attr_kernel_pa[9][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop09_1_frame01_size);

	fd->dma_para->attr_kernel_pa[10][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop10_0_frame01_size);

	fd->dma_para->attr_kernel_pa[10][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop10_1_frame01_size);

	fd->dma_para->attr_kernel_pa[11][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop11_0_frame01_size);

	fd->dma_para->attr_kernel_pa[11][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop11_1_frame01_size);

	fd->dma_para->attr_kernel_pa[12][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop12_0_frame01_size);

	fd->dma_para->attr_kernel_pa[12][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop12_1_frame01_size);

	fd->dma_para->attr_kernel_pa[13][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop13_0_frame01_size);

	fd->dma_para->attr_kernel_pa[13][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop13_1_frame01_size);

	fd->dma_para->attr_kernel_pa[14][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop14_0_frame01_size);

	fd->dma_para->attr_kernel_pa[14][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop14_1_frame01_size);

	fd->dma_para->attr_kernel_pa[15][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop15_0_frame01_size);

	fd->dma_para->attr_kernel_pa[15][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop15_1_frame01_size);

	fd->dma_para->attr_kernel_pa[16][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop16_0_frame01_size);

	fd->dma_para->attr_kernel_pa[16][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop16_1_frame01_size);

	fd->dma_para->attr_kernel_pa[17][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop17_0_frame01_size);

	fd->dma_para->attr_kernel_pa[17][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop17_1_frame01_size);

	fd->dma_para->attr_kernel_pa[18][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop18_0_frame01_size);

	fd->dma_para->attr_kernel_pa[18][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop18_1_frame01_size);

	fd->dma_para->attr_kernel_pa[19][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop19_0_frame01_size);

	fd->dma_para->attr_kernel_pa[19][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop19_1_frame01_size);

	fd->dma_para->attr_kernel_pa[20][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop20_0_frame01_size);

	fd->dma_para->attr_kernel_pa[20][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop20_1_frame01_size);

	fd->dma_para->attr_kernel_pa[21][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop21_0_frame01_size);

	fd->dma_para->attr_kernel_pa[21][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop21_1_frame01_size);

	fd->dma_para->attr_kernel_pa[22][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop22_0_frame01_size);

	fd->dma_para->attr_kernel_pa[22][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop22_1_frame01_size);

	fd->dma_para->attr_kernel_pa[23][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop23_0_frame01_size);

	fd->dma_para->attr_kernel_pa[23][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop23_1_frame01_size);

	fd->dma_para->attr_kernel_pa[24][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop24_0_frame01_size);

	fd->dma_para->attr_kernel_pa[24][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop24_1_frame01_size);

	fd->dma_para->attr_kernel_pa[25][0] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop25_0_frame01_size);

	fd->dma_para->attr_kernel_pa[25][1] = pa;
	pa += AIE_ALIGN32(gender_kernel_bias_loop25_1_frame01_size);

	/* fld blink weight forest model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_BS_BASE_ADDR);
	fd->dma_para->fld_blink_weight_pa = pa;
	pa += AIE_ALIGN32(fdvt_fld_blink_weight_forest14_size);

	/* fld fp forest model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_FP_BASE_ADDR);
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_fp_pa[i] = pa;
		pa += AIE_ALIGN32(fdvt_fld_fp_forest00_om45_size);
	}

	/* fld leafnode forest model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_SH_BASE_ADDR);
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_leafnode_pa[i] = pa;
		pa += AIE_ALIGN32(fdvt_fld_leafnode_forest00_size);
	}

	/* fld tree forest cv weight model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_CV_BASE_ADDR);
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_cv_pa[i] = pa;
		pa += AIE_ALIGN32(fdvt_fld_tree_forest00_cv_weight_size);
	}

	/* fld tree forest init shape model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_MS_BASE_ADDR);
	fd->dma_para->fld_shape_pa[0] = pa;
	pa += AIE_ALIGN32(fdvt_fld_tree_forest00_init_shape_size);

	/* fld tree forest tree node model */
	if (!fd->is_shutdown)
		writel(AIE_IOVA(pa), fd->fd_base + FLD_TR_BASE_ADDR);
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_tree02_pa[i] = pa;
		pa += AIE_ALIGN32(fdvt_fld_tree_forest00_tree_node_size);
	}

	/* arrange va */
	/* fd kernel model */
	fd->dma_para->fd_kernel_va[0][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop00_0_frame01_size);

	fd->dma_para->fd_kernel_va[0][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop00_1_frame01_size);

	fd->dma_para->fd_kernel_va[1][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop01_0_frame01_size);

	fd->dma_para->fd_kernel_va[1][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop01_1_frame01_size);

	fd->dma_para->fd_kernel_va[2][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop02_0_frame01_size);

	fd->dma_para->fd_kernel_va[2][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop02_1_frame01_size);

	fd->dma_para->fd_kernel_va[3][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop03_0_frame01_size);

	fd->dma_para->fd_kernel_va[3][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop03_1_frame01_size);

	fd->dma_para->fd_kernel_va[4][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop04_0_frame01_size);

	fd->dma_para->fd_kernel_va[4][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop04_1_frame01_size);

	fd->dma_para->fd_kernel_va[5][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop05_0_frame01_size);

	fd->dma_para->fd_kernel_va[5][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop05_1_frame01_size);

	fd->dma_para->fd_kernel_va[6][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop06_0_frame01_size);

	fd->dma_para->fd_kernel_va[6][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop06_1_frame01_size);

	fd->dma_para->fd_kernel_va[7][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop07_0_frame01_size);

	fd->dma_para->fd_kernel_va[7][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop07_1_frame01_size);

	fd->dma_para->fd_kernel_va[8][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop08_0_frame01_size);

	fd->dma_para->fd_kernel_va[8][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop08_1_frame01_size);

	fd->dma_para->fd_kernel_va[9][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop09_0_frame01_size);

	fd->dma_para->fd_kernel_va[9][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop09_1_frame01_size);

	fd->dma_para->fd_kernel_va[10][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop10_0_frame01_size);

	fd->dma_para->fd_kernel_va[10][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop10_1_frame01_size);

	fd->dma_para->fd_kernel_va[11][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop11_0_frame01_size);

	fd->dma_para->fd_kernel_va[11][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop11_1_frame01_size);

	fd->dma_para->fd_kernel_va[12][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop12_0_frame01_size);

	fd->dma_para->fd_kernel_va[12][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop12_1_frame01_size);

	fd->dma_para->fd_kernel_va[13][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop13_0_frame01_size);

	fd->dma_para->fd_kernel_va[13][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop13_1_frame01_size);

	fd->dma_para->fd_kernel_va[14][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop14_0_frame01_size);

	fd->dma_para->fd_kernel_va[14][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop14_1_frame01_size);

	fd->dma_para->fd_kernel_va[15][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop15_0_frame01_size);

	fd->dma_para->fd_kernel_va[15][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop15_1_frame01_size);

	fd->dma_para->fd_kernel_va[16][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop16_0_frame01_size);

	fd->dma_para->fd_kernel_va[16][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop16_1_frame01_size);

	fd->dma_para->fd_kernel_va[17][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop17_0_frame01_size);

	fd->dma_para->fd_kernel_va[17][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop17_1_frame01_size);

	fd->dma_para->fd_kernel_va[18][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop18_0_frame01_size);

	fd->dma_para->fd_kernel_va[18][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop18_1_frame01_size);

	fd->dma_para->fd_kernel_va[19][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop19_0_frame01_size);

	fd->dma_para->fd_kernel_va[19][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop19_1_frame01_size);

	fd->dma_para->fd_kernel_va[20][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop20_0_frame01_size);

	fd->dma_para->fd_kernel_va[20][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop20_1_frame01_size);

	fd->dma_para->fd_kernel_va[21][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop21_0_frame01_size);

	fd->dma_para->fd_kernel_va[21][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop21_1_frame01_size);

	fd->dma_para->fd_kernel_va[22][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop22_0_frame01_size);

	fd->dma_para->fd_kernel_va[22][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop22_1_frame01_size);

	fd->dma_para->fd_kernel_va[23][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop23_0_frame01_size);

	fd->dma_para->fd_kernel_va[23][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop23_1_frame01_size);

	fd->dma_para->fd_kernel_va[24][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop24_0_frame01_size);

	fd->dma_para->fd_kernel_va[24][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop24_1_frame01_size);

	fd->dma_para->fd_kernel_va[25][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop25_0_frame01_size);

	fd->dma_para->fd_kernel_va[25][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop25_1_frame01_size);

	fd->dma_para->fd_kernel_va[26][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop26_0_frame01_size);

	fd->dma_para->fd_kernel_va[26][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop26_1_frame01_size);

	fd->dma_para->fd_kernel_va[27][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop27_0_frame01_size);

	fd->dma_para->fd_kernel_va[27][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop27_1_frame01_size);

	fd->dma_para->fd_kernel_va[29][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop29_0_frame01_size);

	fd->dma_para->fd_kernel_va[29][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop29_1_frame01_size);

	fd->dma_para->fd_kernel_va[30][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop30_0_frame01_size);

	fd->dma_para->fd_kernel_va[30][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop30_1_frame01_size);

	fd->dma_para->fd_kernel_va[31][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop31_0_frame01_size);

	fd->dma_para->fd_kernel_va[31][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop31_1_frame01_size);

	fd->dma_para->fd_kernel_va[32][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop32_0_frame01_size);

	fd->dma_para->fd_kernel_va[32][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop32_1_frame01_size);

	fd->dma_para->fd_kernel_va[33][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop33_0_frame01_size);

	fd->dma_para->fd_kernel_va[33][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop33_1_frame01_size);

	fd->dma_para->fd_kernel_va[34][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop34_0_frame01_size);

	fd->dma_para->fd_kernel_va[34][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop34_1_frame01_size);

	fd->dma_para->fd_kernel_va[35][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop35_0_frame01_size);

	fd->dma_para->fd_kernel_va[35][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop35_1_frame01_size);

	fd->dma_para->fd_kernel_va[36][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop36_0_frame01_size);

	fd->dma_para->fd_kernel_va[36][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop36_1_frame01_size);

	fd->dma_para->fd_kernel_va[37][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop37_0_frame01_size);

	fd->dma_para->fd_kernel_va[37][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop37_1_frame01_size);

	fd->dma_para->fd_kernel_va[38][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop38_0_frame01_size);

	fd->dma_para->fd_kernel_va[38][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop38_1_frame01_size);

	fd->dma_para->fd_kernel_va[39][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop39_0_frame01_size);

	fd->dma_para->fd_kernel_va[39][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop39_1_frame01_size);

	fd->dma_para->fd_kernel_va[40][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop40_0_frame01_size);

	fd->dma_para->fd_kernel_va[40][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop40_1_frame01_size);

	fd->dma_para->fd_kernel_va[41][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop41_0_frame01_size);

	fd->dma_para->fd_kernel_va[41][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop41_1_frame01_size);

	fd->dma_para->fd_kernel_va[42][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop42_0_frame01_size);

	fd->dma_para->fd_kernel_va[42][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop42_1_frame01_size);

	fd->dma_para->fd_kernel_va[43][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop43_0_frame01_size);

	fd->dma_para->fd_kernel_va[43][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop43_1_frame01_size);

	fd->dma_para->fd_kernel_va[44][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop44_0_frame01_size);

	fd->dma_para->fd_kernel_va[44][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop44_1_frame01_size);

	fd->dma_para->fd_kernel_va[45][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop45_0_frame01_size);

	fd->dma_para->fd_kernel_va[45][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop45_1_frame01_size);

	fd->dma_para->fd_kernel_va[46][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop46_0_frame01_size);

	fd->dma_para->fd_kernel_va[46][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop46_1_frame01_size);

	fd->dma_para->fd_kernel_va[47][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop47_0_frame01_size);

	fd->dma_para->fd_kernel_va[47][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop47_1_frame01_size);

	fd->dma_para->fd_kernel_va[48][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop48_0_frame01_size);

	fd->dma_para->fd_kernel_va[48][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop48_1_frame01_size);

	fd->dma_para->fd_kernel_va[49][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop49_0_frame01_size);

	fd->dma_para->fd_kernel_va[49][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop49_1_frame01_size);

	fd->dma_para->fd_kernel_va[50][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop50_0_frame01_size);

	fd->dma_para->fd_kernel_va[50][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop50_1_frame01_size);

	fd->dma_para->fd_kernel_va[51][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop51_0_frame01_size);

	fd->dma_para->fd_kernel_va[51][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop51_1_frame01_size);

	fd->dma_para->fd_kernel_va[52][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop52_0_frame01_size);

	fd->dma_para->fd_kernel_va[52][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop52_1_frame01_size);

	fd->dma_para->fd_kernel_va[53][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop53_0_frame01_size);

	fd->dma_para->fd_kernel_va[53][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop53_1_frame01_size);

	fd->dma_para->fd_kernel_va[54][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop54_0_frame01_size);

	fd->dma_para->fd_kernel_va[54][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop54_1_frame01_size);

	fd->dma_para->fd_kernel_va[55][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop55_0_frame01_size);

	fd->dma_para->fd_kernel_va[55][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop55_1_frame01_size);

	fd->dma_para->fd_kernel_va[56][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop56_0_frame01_size);

	fd->dma_para->fd_kernel_va[56][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop56_1_frame01_size);

	fd->dma_para->fd_kernel_va[58][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop58_0_frame01_size);

	fd->dma_para->fd_kernel_va[58][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop58_1_frame01_size);

	fd->dma_para->fd_kernel_va[59][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop59_0_frame01_size);

	fd->dma_para->fd_kernel_va[59][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop59_1_frame01_size);

	fd->dma_para->fd_kernel_va[60][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop60_0_frame01_size);

	fd->dma_para->fd_kernel_va[60][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop60_1_frame01_size);

	fd->dma_para->fd_kernel_va[61][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop61_0_frame01_size);

	fd->dma_para->fd_kernel_va[61][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop61_1_frame01_size);

	fd->dma_para->fd_kernel_va[62][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop62_0_frame01_size);

	fd->dma_para->fd_kernel_va[62][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop62_1_frame01_size);

	fd->dma_para->fd_kernel_va[63][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop63_0_frame01_size);

	fd->dma_para->fd_kernel_va[63][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop63_1_frame01_size);

	fd->dma_para->fd_kernel_va[64][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop64_0_frame01_size);

	fd->dma_para->fd_kernel_va[64][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop64_1_frame01_size);

	fd->dma_para->fd_kernel_va[65][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop65_0_frame01_size);

	fd->dma_para->fd_kernel_va[65][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop65_1_frame01_size);

	fd->dma_para->fd_kernel_va[66][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop66_0_frame01_size);

	fd->dma_para->fd_kernel_va[66][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop66_1_frame01_size);

	fd->dma_para->fd_kernel_va[67][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop67_0_frame01_size);

	fd->dma_para->fd_kernel_va[67][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop67_1_frame01_size);

	fd->dma_para->fd_kernel_va[68][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop68_0_frame01_size);

	fd->dma_para->fd_kernel_va[68][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop68_1_frame01_size);

	fd->dma_para->fd_kernel_va[69][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop69_0_frame01_size);

	fd->dma_para->fd_kernel_va[69][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop69_1_frame01_size);

	fd->dma_para->fd_kernel_va[70][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop70_0_frame01_size);

	fd->dma_para->fd_kernel_va[70][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop70_1_frame01_size);

	fd->dma_para->fd_kernel_va[71][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop71_0_frame01_size);

	fd->dma_para->fd_kernel_va[71][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop71_1_frame01_size);

	fd->dma_para->fd_kernel_va[72][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop72_0_frame01_size);

	fd->dma_para->fd_kernel_va[72][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop72_1_frame01_size);

	fd->dma_para->fd_kernel_va[73][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop73_0_frame01_size);

	fd->dma_para->fd_kernel_va[73][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop73_1_frame01_size);

	fd->dma_para->fd_kernel_va[74][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop74_0_frame01_size);

	fd->dma_para->fd_kernel_va[74][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop74_1_frame01_size);

	fd->dma_para->fd_kernel_va[75][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop75_0_frame01_size);

	fd->dma_para->fd_kernel_va[75][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop75_1_frame01_size);

	fd->dma_para->fd_kernel_va[76][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop76_0_frame01_size);

	fd->dma_para->fd_kernel_va[76][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop76_1_frame01_size);

	fd->dma_para->fd_kernel_va[77][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop77_0_frame01_size);

	fd->dma_para->fd_kernel_va[77][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop77_1_frame01_size);

	fd->dma_para->fd_kernel_va[78][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop78_0_frame01_size);

	fd->dma_para->fd_kernel_va[78][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop78_1_frame01_size);

	fd->dma_para->fd_kernel_va[79][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop79_0_frame01_size);

	fd->dma_para->fd_kernel_va[79][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop79_1_frame01_size);

	fd->dma_para->fd_kernel_va[80][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop80_0_frame01_size);

	fd->dma_para->fd_kernel_va[80][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop80_1_frame01_size);

	fd->dma_para->fd_kernel_va[81][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop81_0_frame01_size);

	fd->dma_para->fd_kernel_va[81][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop81_1_frame01_size);

	fd->dma_para->fd_kernel_va[82][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop82_0_frame01_size);

	fd->dma_para->fd_kernel_va[82][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop82_1_frame01_size);

	fd->dma_para->fd_kernel_va[83][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop83_0_frame01_size);

	fd->dma_para->fd_kernel_va[83][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop83_1_frame01_size);

	fd->dma_para->fd_kernel_va[84][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop84_0_frame01_size);

	fd->dma_para->fd_kernel_va[84][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop84_1_frame01_size);

	fd->dma_para->fd_kernel_va[85][0] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop85_0_frame01_size);

	fd->dma_para->fd_kernel_va[85][1] = va;
	va += AIE_ALIGN32(fdvt_kernel_bias_loop85_1_frame01_size);

	/* attribute kernel model */
	fd->dma_para->attr_kernel_va[0][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop00_0_frame01_size);

	fd->dma_para->attr_kernel_va[0][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop00_1_frame01_size);

	fd->dma_para->attr_kernel_va[1][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop01_0_frame01_size);

	fd->dma_para->attr_kernel_va[1][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop01_1_frame01_size);

	fd->dma_para->attr_kernel_va[2][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop02_0_frame01_size);

	fd->dma_para->attr_kernel_va[2][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop02_1_frame01_size);

	fd->dma_para->attr_kernel_va[3][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop03_0_frame01_size);

	fd->dma_para->attr_kernel_va[3][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop03_1_frame01_size);

	fd->dma_para->attr_kernel_va[4][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop04_0_frame01_size);

	fd->dma_para->attr_kernel_va[4][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop04_1_frame01_size);

	fd->dma_para->attr_kernel_va[5][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop05_0_frame01_size);

	fd->dma_para->attr_kernel_va[5][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop05_1_frame01_size);

	fd->dma_para->attr_kernel_va[6][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop06_0_frame01_size);

	fd->dma_para->attr_kernel_va[6][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop06_1_frame01_size);

	fd->dma_para->attr_kernel_va[7][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop07_0_frame01_size);

	fd->dma_para->attr_kernel_va[7][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop07_1_frame01_size);

	fd->dma_para->attr_kernel_va[8][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop08_0_frame01_size);

	fd->dma_para->attr_kernel_va[8][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop08_1_frame01_size);

	fd->dma_para->attr_kernel_va[9][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop09_0_frame01_size);

	fd->dma_para->attr_kernel_va[9][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop09_1_frame01_size);

	fd->dma_para->attr_kernel_va[10][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop10_0_frame01_size);

	fd->dma_para->attr_kernel_va[10][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop10_1_frame01_size);

	fd->dma_para->attr_kernel_va[11][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop11_0_frame01_size);

	fd->dma_para->attr_kernel_va[11][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop11_1_frame01_size);

	fd->dma_para->attr_kernel_va[12][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop12_0_frame01_size);

	fd->dma_para->attr_kernel_va[12][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop12_1_frame01_size);

	fd->dma_para->attr_kernel_va[13][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop13_0_frame01_size);

	fd->dma_para->attr_kernel_va[13][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop13_1_frame01_size);

	fd->dma_para->attr_kernel_va[14][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop14_0_frame01_size);

	fd->dma_para->attr_kernel_va[14][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop14_1_frame01_size);

	fd->dma_para->attr_kernel_va[15][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop15_0_frame01_size);

	fd->dma_para->attr_kernel_va[15][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop15_1_frame01_size);

	fd->dma_para->attr_kernel_va[16][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop16_0_frame01_size);

	fd->dma_para->attr_kernel_va[16][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop16_1_frame01_size);

	fd->dma_para->attr_kernel_va[17][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop17_0_frame01_size);

	fd->dma_para->attr_kernel_va[17][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop17_1_frame01_size);

	fd->dma_para->attr_kernel_va[18][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop18_0_frame01_size);

	fd->dma_para->attr_kernel_va[18][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop18_1_frame01_size);

	fd->dma_para->attr_kernel_va[19][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop19_0_frame01_size);

	fd->dma_para->attr_kernel_va[19][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop19_1_frame01_size);

	fd->dma_para->attr_kernel_va[20][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop20_0_frame01_size);

	fd->dma_para->attr_kernel_va[20][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop20_1_frame01_size);

	fd->dma_para->attr_kernel_va[21][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop21_0_frame01_size);

	fd->dma_para->attr_kernel_va[21][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop21_1_frame01_size);

	fd->dma_para->attr_kernel_va[22][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop22_0_frame01_size);

	fd->dma_para->attr_kernel_va[22][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop22_1_frame01_size);

	fd->dma_para->attr_kernel_va[23][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop23_0_frame01_size);

	fd->dma_para->attr_kernel_va[23][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop23_1_frame01_size);

	fd->dma_para->attr_kernel_va[24][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop24_0_frame01_size);

	fd->dma_para->attr_kernel_va[24][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop24_1_frame01_size);

	fd->dma_para->attr_kernel_va[25][0] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop25_0_frame01_size);

	fd->dma_para->attr_kernel_va[25][1] = va;
	va += AIE_ALIGN32(gender_kernel_bias_loop25_1_frame01_size);

	/* fld blink weight forest model */
	fd->dma_para->fld_blink_weight_va = va;
	va += AIE_ALIGN32(fdvt_fld_blink_weight_forest14_size);

	/* fld fp forest model */
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_fp_va[i] = va;
		va += AIE_ALIGN32(fdvt_fld_fp_forest00_om45_size);
	}

	/* fld leafnode forest model */
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_leafnode_va[i] = va;
		va += AIE_ALIGN32(fdvt_fld_leafnode_forest00_size);
	}

	/* fld tree forest cv weight model */
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_cv_va[i] = va;
		va += AIE_ALIGN32(fdvt_fld_tree_forest00_cv_weight_size);
	}

	/* fld tree forest init shape model */
	fd->dma_para->fld_shape_va[0] = va;
	va += AIE_ALIGN32(fdvt_fld_tree_forest00_init_shape_size);

	/* fld tree forest tree node model */
	for (i = 0; i < FLD_MAX_INPUT; i++) {
		fd->dma_para->fld_tree02_va[i] = va;
		va += AIE_ALIGN32(fdvt_fld_tree_forest00_tree_node_size);
	}

	if (!fd->is_shutdown) {
		/* Set up kernel offset */
		writel(AIE_IOVA(AIE_ALIGN32(fdvt_fld_fp_forest00_om45_size)),
			fd->fd_base + FLD_FP_FORT_OFST);
		writel(AIE_IOVA(AIE_ALIGN32(fdvt_fld_tree_forest00_tree_node_size)),
			fd->fd_base + FLD_TR_FORT_OFST);
		writel(AIE_IOVA(AIE_ALIGN32(fdvt_fld_leafnode_forest00_size)),
			fd->fd_base + FLD_SH_FORT_OFST);
		writel(AIE_IOVA(AIE_ALIGN32(fdvt_fld_tree_forest00_cv_weight_size)),
			fd->fd_base + FLD_CV_FORT_OFST);

		/* Model related configuration */
		writel(0x00b0c80f, fd->fd_base + FLD_NUM_CONFIG_0);

		writel(0x6C004800, fd->fd_base + FLD_PCA_MEAN_SCALE_0);
		writel(0x6c007c00, fd->fd_base + FLD_PCA_MEAN_SCALE_1);
		writel(0x6c00b800, fd->fd_base + FLD_PCA_MEAN_SCALE_2);
		writel(0x6c00ec00, fd->fd_base + FLD_PCA_MEAN_SCALE_3);
		writel(0xb0009800, fd->fd_base + FLD_PCA_MEAN_SCALE_4);
		writel(0xdc006800, fd->fd_base + FLD_PCA_MEAN_SCALE_5);
		writel(0xdc00cc00, fd->fd_base + FLD_PCA_MEAN_SCALE_6);

		writel(0x00fdefd3, fd->fd_base + FLD_PCA_VEC_0);
		writel(0x00fef095, fd->fd_base + FLD_PCA_VEC_1);
		writel(0x00011095, fd->fd_base + FLD_PCA_VEC_2);
		writel(0x00022fd3, fd->fd_base + FLD_PCA_VEC_3);
		writel(0x000003e6, fd->fd_base + FLD_PCA_VEC_4);
		writel(0x0000dfe9, fd->fd_base + FLD_PCA_VEC_5);
		writel(0x00ff3fe9, fd->fd_base + FLD_PCA_VEC_6);

		writel(0x00000008, fd->fd_base + FLD_CV_BIAS_FR_0);
		writel(0x00000003, fd->fd_base + FLD_CV_BIAS_PF_0);
		writel(0x0000b835, fd->fd_base + FLD_CV_RANGE_FR_0);
		writel(0xFFFF5cba, fd->fd_base + FLD_CV_RANGE_FR_1);
		writel(0x00005ed5, fd->fd_base + FLD_CV_RANGE_PF_0);
		writel(0xFFFF910d, fd->fd_base + FLD_CV_RANGE_PF_1);
		writel(0xe8242184, fd->fd_base + FLD_PP_COEF);

		writel(0x00000001, fd->fd_base + FLD_BS_CONFIG0);
		writel(0x0000031e, fd->fd_base + FLD_BS_CONFIG1);
		writel(0xfffffcae, fd->fd_base + FLD_BS_CONFIG2);
	}

}

static void aie_dump_cg_reg(struct mtk_aie_dev *fd)
{
	void __iomem *isp_main_reg = fd->reg_base[0];
	void __iomem *isp_vcore_reg = fd->reg_base[1];
	void __iomem *sys_spm_reg = fd->reg_base[2];
	void __iomem *top_ck_gen_reg = fd->reg_base[3];
	void __iomem *apmixedsys_clk_reg = fd->reg_base[4];

	if (fd->is_shutdown) {
		aie_dev_info(fd->dev, "%s: skip for shutdown", __func__);
		return;
	}

	aie_dev_info(fd->dev, "Dump AIE CG/PG:\n");

	if (isp_main_reg != NULL) {
		aie_dev_info(fd->dev, "[0x15000000] 0x%08X\n",
			(unsigned int)ioread32((void *)(isp_main_reg)));
		aie_dev_info(fd->dev, "[0x15000050] 0x%08X\n",
			(unsigned int)ioread32((void *)(isp_main_reg + 0x50)));
	}

	if (isp_vcore_reg != NULL)
		aie_dev_info(fd->dev, "[0x15780000] 0x%08X\n",
			(unsigned int)ioread32((void *)(isp_vcore_reg)));

	if (sys_spm_reg != NULL) {
		aie_dev_info(fd->dev, "[0x1c001e48] 0x%08X\n",
			(unsigned int)ioread32((void *)(sys_spm_reg + 0xe48)));
		aie_dev_info(fd->dev, "[0x1c001e4c] 0x%08X\n",
			(unsigned int)ioread32((void *)(sys_spm_reg + 0xe4c)));
	}

	if (top_ck_gen_reg != NULL)
		aie_dev_info(fd->dev, "[0x10000840] 0x%08X\n",
			(unsigned int)ioread32((void *)(top_ck_gen_reg + 0x840)));

	if (apmixedsys_clk_reg != NULL) {
		aie_dev_info(fd->dev, "[0x1000c914] 0x%08X\n",
			(unsigned int)ioread32((void *)(apmixedsys_clk_reg + 0x914)));
		aie_dev_info(fd->dev, "[0x1000c920] 0x%08X\n",
			(unsigned int)ioread32((void *)(apmixedsys_clk_reg + 0x920)));
	}
}

static void aie_enable_ddren_7sp_1(struct mtk_aie_dev *fd)
{
	void __iomem *isp_vcore_reg = 0L;
	uint32_t value = 0;
	int count = 0;

	isp_vcore_reg = fd->reg_base[1];
	if (isp_vcore_reg != NULL) {
		value = ioread32((void *)(isp_vcore_reg + 0x10));
		aie_dev_info(fd->dev, "[%s]  R(0x%x): 0x%x\n\n",
				__func__, (uint32_t)(uint64_t)(isp_vcore_reg + 0x10), value);
		value |= 0x1000;
		iowrite32(value, (void *)(isp_vcore_reg + 0x10));

		count = 0;
		while (count < 1000000) {
			value = ioread32((void *)(isp_vcore_reg + 0x14));
			if ((value & 0x2) == 0x2)
				break;
			count++;
		}

		if (count >= 1000000)
			aie_dev_info(fd->dev, "[%s] APSRC ACK count(%d)\n",
				__func__, count);

		value = ioread32((void *)(isp_vcore_reg + 0x10));
		value |= 0x100;
		iowrite32(value, (isp_vcore_reg + 0x10));
		count = 0;
		while (count < 1000000) {
			value = ioread32((void *)(isp_vcore_reg + 0x14));
			if ((value & 0x1) == 0x1)
				break;
			count++;
		}

		if (count >= 1000000)
			aie_dev_info(fd->dev, "[%s] DDREN ACK count(%d)\n",
				__func__, count);

		value = ioread32((void *)(isp_vcore_reg + 0x10));
		aie_dev_info(fd->dev, "[%s]  R(0x%x): 0x%x\n\n",
				__func__, (uint32_t)(uint64_t)(isp_vcore_reg + 0x10), value);
	} else {
		aie_dev_info(fd->dev, "[%s] Failed to enable ddren\n", __func__);
	}
}

const struct mtk_aie_drv_ops aie_ops_isp7sp = {
	.reset = aie_reset,
	.alloc_buf = aie_alloc_aie_buf,
	.init = aie_init,
	.uninit = aie_uninit,
	.prepare = aie_prepare,
	.execute = aie_execute,
	.get_fd_result = aie_get_fd_result,
	.get_attr_result = aie_get_attr_result,
	.get_fld_result = aie_get_fld_result,
	.irq_handle = aie_irqhandle,
	.config_fld_buf_reg = aie_config_fld_buf_reg,
	.fdvt_dump_reg = aie_fdvt_dump_reg,
	.dump_cg_reg = aie_dump_cg_reg,
};

const struct mtk_aie_drv_ops aie_ops_isp7sp_1 = {
	.reset = aie_reset,
	.alloc_buf = aie_alloc_aie_buf,
	.init = aie_init,
	.uninit = aie_uninit,
	.prepare = aie_prepare,
	.execute = aie_execute,
	.get_fd_result = aie_get_fd_result,
	.get_attr_result = aie_get_attr_result,
	.get_fld_result = aie_get_fld_result,
	.irq_handle = aie_irqhandle,
	.config_fld_buf_reg = aie_config_fld_buf_reg,
	.fdvt_dump_reg = aie_fdvt_dump_reg,
	.dump_cg_reg = aie_dump_cg_reg,
	.enable_ddren = aie_enable_ddren_7sp_1,
};

MODULE_IMPORT_NS(DMA_BUF);
