/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 *
 * Author: Yuhsuan.chang <yuhsuan.chang@mediatek.com>
 *
 */

#ifndef IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_HWQOS_DATA_H_
#define IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_HWQOS_DATA_H_

#include <linux/bitfield.h>
#include <linux/bits.h>

#include "mtk_imgsys-hwqos-reg.h"

#define FLOAT2FIX(x, bits) ((u32)((x) * (1 << (bits))))

/* BWR register field */
#define BWR_IMG_RPT_START                   0
#define BWR_IMG_RPT_END                     1
#define BWR_IMG_RPT_RST                     2

/* BWR register field */
#define BWR_IMG_RPT_IDLE                    0
#define BWR_IMG_RPT_WAIT                    1
#define BWR_IMG_RPT_SEND                    2

/* Fill in the value */
// original setting BWR_IMG_PROTOCOL0_OFT:  0x2100020
#define BWR_IMG_PROTOCOL0                   0x2110120
// original setting BWR_IMG_PROTOCOL1_OFT:  0x140412
#define BWR_IMG_PROTOCOL1                   0x150512

#define OCC_FACTOR                          1.2890625  // 1.25 * (1 + 1/32) (with TCU BW)
#define BUS_URATE                           1.421875
#define BW_RAT                              0.9765625
#define BLS_UNIT                            16  // byte

#define BWR_OCC_FACTOR_POINT                7
#define BWR_BW_RAT_POINT                    7
#define BWR_BW_POINT                        3

#define BWR_IMG_RPT_TIMER                   2600  // 100 us
#define BWR_IMG_DBC_CYC                     260   // 10 us
#define BWR_IMG_OCC_FACTOR                  FLOAT2FIX(OCC_FACTOR, BWR_OCC_FACTOR_POINT)
#define BWR_IMG_BUS_URATE                   FLOAT2FIX(BUS_URATE,  BWR_OCC_FACTOR_POINT)
#define BWR_IMG_TTL_DVFS_FREQ               1     // 64 MB/s
#define BWR_IMG_RW_DVFS_FREQ                4     // 16 MB/s

#define BWR_IMG_SRT_ENG_BW_RAT              FLOAT2FIX(BW_RAT, BWR_BW_RAT_POINT)  // 0x7D
#define BWR_IMG_SRT_ENG_MAX_RW_ENG_BW       FLOAT2FIX(2200, BWR_BW_POINT)
#define BWR_IMG_SRT_ENG_MAX_TTL_BW          FLOAT2FIX(11236,  BWR_BW_POINT)     // 0x15F20

/* Enable engine 0~8 for BWR: 0b111111111 */
#define BWR_IMG_ENG_EN                      GENMASK(8, 0)

/* Enable engine for each core & sub-common for BWR */
#define BWR_IMG_TTL_ENG_EN                  (GENMASK(5, 0) | BIT(7))
#define BWR_IMG_CORE0_SC0_ENG_EN            (BIT(0) | BIT(3) | BIT(4))
#define BWR_IMG_CORE0_SC3_ENG_EN            (BIT(3) | BIT(4) | BIT(7))
#define BWR_IMG_CORE1_SC1_ENG_EN            (BIT(1) | BIT(4) | BIT(5))
#define BWR_IMG_CORE1_SC4_ENG_EN            (BIT(2) | BIT(4) | BIT(5))
#define BWR_IMG_CORE2_SC2_ENG_EN            (BIT(7) | BIT(8))
#define BWR_IMG_SW_MODE_EN                  0x0

/* OSTDL setting */
#define OSTDL_EN_REG_MASK                   GENMASK(14, 14)
#define OSTDL_EN                            1

#define OSTDL_R_REG_H 22
#define OSTDL_R_REG_L 16
#define OSTDL_R_REG_MASK                    GENMASK(OSTDL_R_REG_H, OSTDL_R_REG_L)

#define OSTDL_W_REG_H 30
#define OSTDL_W_REG_L 24
#define OSTDL_W_REG_MASK                    GENMASK(OSTDL_W_REG_H, OSTDL_W_REG_L)

/* IMG BW setting */
#define IMG_BW_REG_MASK0                    GENMASK(9, 0)
#define IMG_BW_REG_MASK1                    GENMASK(19, 10)
#define IMG_BW_REG_MASK2                    GENMASK(29, 20)

enum BWR_CTRL {
	BWR_START = 0,
	BWR_STOP,
};

enum BLS_CTRL {
	BLS_INIT = 0,
	BLS_STOP,
	BLS_TRIG,
};

struct reg {
	const uint32_t offset;
	const uint32_t value;
};

struct reg bls_init_data[] = {
	{BLS_IMG_RID_EN_OFT,          0x1},
	{BLS_IMG_WID_EN_OFT,          0x1},
};

struct reg bwr_init_data[] = {
	{BWR_IMG_PROTOCOL_SET_EN_OFT,        1},
	{BWR_IMG_PROTOCOL0_OFT,              BWR_IMG_PROTOCOL0},
	{BWR_IMG_PROTOCOL1_OFT,              BWR_IMG_PROTOCOL1},
	{BWR_IMG_RPT_TIMER_OFT,              BWR_IMG_RPT_TIMER},
	{BWR_IMG_DBC_CYC_OFT,                BWR_IMG_DBC_CYC},
	{BWR_IMG_SRT_TTL_OCC_FACTOR_OFT,     BWR_IMG_OCC_FACTOR},
	{BWR_IMG_SRT_RW_OCC_FACTOR_OFT,      BWR_IMG_BUS_URATE},
	{BWR_IMG_SRT_TTL_DVFS_FREQ_OFT,      BWR_IMG_TTL_DVFS_FREQ},
	{BWR_IMG_SRT_RW_DVFS_FREQ_OFT,       BWR_IMG_RW_DVFS_FREQ},
};

uint32_t bls_base_array[] = {
	BLS_IMG_E1A_BASE, BLS_IMG_E2A_BASE,
	BLS_IMG_E3A_BASE, BLS_IMG_E4A_BASE,
	BLS_IMG_E5A_BASE, BLS_IMG_E6A_BASE,
	BLS_IMG_E7A_BASE, BLS_IMG_E8A_BASE,
	BLS_IMG_E9A_BASE, BLS_IMG_E10A_BASE,
	BLS_IMG_E11A_BASE, BLS_IMG_E12A_BASE,
	BLS_IMG_E13A_BASE, BLS_IMG_E14A_BASE,
	BLS_IMG_E15A_BASE,
};

uint32_t bwr_base_array[] = {
	BWR_IMG_E1A_BASE,
};


struct bwr_ctrl_reg {
	/* HW MODE: 1, SW MODE: 0 */
	const uint32_t qos_sel_offset;
	/* HW MODE, SW trigger */
	const uint32_t qos_trig_offset;
	/* SW MODE, SW enable */
	const uint32_t qos_en_offset;
	/* ENGINE enable */
	const uint32_t eng_en_value;
};

#define __BWR_CTRL_REG(rw, x, y)  \
	{.qos_sel_offset  = BWR_IMG_SRT_ ## rw ## x ## _BW_QOS_SEL ## y ## _OFT, \
	.qos_trig_offset = BWR_IMG_SRT_ ## rw ## x ## _SW_QOS_TRIG ## y ## _OFT,\
	.qos_en_offset   = BWR_IMG_SRT_ ## rw ## x ## _SW_QOS_EN ## y ## _OFT,  \
	.eng_en_value    = BWR_IMG_CORE ## x ## _SC ## y ## _ENG_EN}
#define _BWR_CTRL_REG(rw, x, y) __BWR_CTRL_REG(rw, x, y)
#define BWR_CTRL_REG(rw, x, y) _BWR_CTRL_REG(rw, x, y)

struct bwr_ctrl_reg bwr_ctrl_data[] = {
	{.qos_sel_offset  = BWR_IMG_SRT_TTL_BW_QOS_SEL_OFT,
	 .qos_trig_offset = BWR_IMG_SRT_TTL_SW_QOS_TRIG_OFT,
	 .qos_en_offset   = BWR_IMG_SRT_TTL_SW_QOS_EN_OFT,
	 .eng_en_value    = BWR_IMG_TTL_ENG_EN},
	BWR_CTRL_REG(R, 0, 0),
	BWR_CTRL_REG(W, 0, 0),
	BWR_CTRL_REG(R, 0, 3),
	BWR_CTRL_REG(W, 0, 3),
	BWR_CTRL_REG(R, 1, 1),
	BWR_CTRL_REG(W, 1, 1),
	BWR_CTRL_REG(R, 1, 4),
	BWR_CTRL_REG(W, 1, 4),
	BWR_CTRL_REG(R, 2, 2),
	BWR_CTRL_REG(W, 2, 2),
};

struct qos_map {
	const uint32_t bls_base;
	const uint8_t  core;
	const uint8_t  sub_common;
	const uint8_t  engine;
	const uint8_t  larb;
	const uint32_t bwr_r_offset;
	const uint32_t bwr_r_rat_offset;
	const uint32_t bwr_w_offset;
	const uint32_t bwr_w_rat_offset;
};

#define __BLS_REG(x)  \
	.bls_base            = BLS_IMG_E ## x ## A_BASE
#define _BLS_REG(x) __BLS_REG(x)
#define BLS_REG(x) _BLS_REG(x)

#define __BWR_BW_REG(x, y, z, l)  \
	.core             = x, \
	.sub_common       = y, \
	.engine           = z, \
	.larb             = l, \
	.bwr_r_offset     = BWR_IMG_SRT_R ## x ## _ENG_BW ## y ## _ ## z ## _OFT,     \
	.bwr_r_rat_offset = BWR_IMG_SRT_R ## x ## _ENG_BW_RAT ## y ## _ ## z ## _OFT, \
	.bwr_w_offset     = BWR_IMG_SRT_W ## x ## _ENG_BW ## y ## _ ## z ## _OFT,     \
	.bwr_w_rat_offset = BWR_IMG_SRT_W ## x ## _ENG_BW_RAT ## y ## _ ## z ## _OFT
#define _BWR_BW_REG(x, y, z, l) __BWR_BW_REG(x, y, z, l)
#define BWR_BW_REG(x, y, z, l) _BWR_BW_REG(x, y, z, l)

struct qos_map qos_map_data[] = {
	{BLS_REG(2), BWR_BW_REG(0, 0, 0, 11)},	//0
	{BLS_REG(3), BWR_BW_REG(0, 0, 3, 28)},	//1
	{BLS_REG(4), BWR_BW_REG(0, 0, 4, 10)},	//2
	{BLS_REG(5), BWR_BW_REG(1, 1, 4, 15)},	//3
	{BLS_REG(6), BWR_BW_REG(1, 1, 1, 22)},	//4
	// {BLS_REG(7), BWR_BW_REG(1, 1, 5, 18)},
	// {BLS_REG(8), BWR_BW_REG(2, 2, 7, 12)},
	// {BLS_REG(9), BWR_BW_REG(2, 2, 8, 18)},
	{BLS_REG(10), BWR_BW_REG(0, 3, 4, 38)},	//5
	{BLS_REG(11), BWR_BW_REG(0, 3, 7, 12)},	//6
	{BLS_REG(12), BWR_BW_REG(0, 3, 3, 40)},	//7
	{BLS_REG(13), BWR_BW_REG(1, 4, 4, 39)},	//8
	{BLS_REG(14), BWR_BW_REG(1, 4, 2, 23)},	//9
	{BLS_REG(15), BWR_BW_REG(1, 4, 5, 9)},	//10
};

uint32_t img_ostdl_array[] = {
	OSTDL_IMG_COMM0_ADDR,
	OSTDL_IMG_COMM1_ADDR
};

struct reg_addr_mask {
	const uint32_t addr;
	const uint32_t mask;
};

struct reg_addr_mask bwr_ttl_bw_array[] = {
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW0_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW1_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW2_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW3_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW4_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW5_OFT, GENMASK(31, 3)},
	{BWR_IMG_E1A_BASE + BWR_IMG_SRT_TTL_ENG_BW7_OFT, GENMASK(31, 3)},
};

struct reg_addr_mask img_hw_bw_reg_array[] = {
	{HFRP_DVFSRC_VMM_IMG_HW_BW_1, IMG_BW_REG_MASK1},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_1, IMG_BW_REG_MASK2},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_2, IMG_BW_REG_MASK2},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_3, IMG_BW_REG_MASK0},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_6, IMG_BW_REG_MASK2},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_7, IMG_BW_REG_MASK0},
	{HFRP_DVFSRC_VMM_IMG_HW_BW_SRT, IMG_BW_REG_MASK0},
};

#endif  // IMGSYS_PLATFORMS_ISP8_MTK_IMGSYS_HWQOS_DATA_H_
