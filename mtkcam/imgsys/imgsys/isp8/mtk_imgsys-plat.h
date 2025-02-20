/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018 MediaTek Inc.
 *
 * Author: Christopher Chen <christopher.chen@mediatek.com>
 *
 */

#ifndef _MTK_IMGSYS_PLAT_H_
#define _MTK_IMGSYS_PLAT_H_

#include <linux/clk.h>

struct clk_bulk_data imgsys_isp8_clks_mt6991[] = {
	{
		.id = "VCORE_GALS",
	},
	{
		.id = "VCORE_MAIN",
	},
	{
		.id = "VCORE_SUB0",
	},
	{
		.id = "VCORE_SUB1",
	},
	{
		.id = "IMG_LARB9",
	},
	{
		.id = "IMG_TRAW0",
	},
	{
		.id = "IMG_TRAW1",
	},
	{
		.id = "IMG_DIP0",
	},
	{
		.id = "IMG_WPE0",
	},
	{
		.id = "IMG_IPE",
	},
	{
		.id = "IMG_WPE1",
	},
	{
		.id = "IMG_WPE2",
	},
	{
		.id = "IMG_ADL_LARB",
	},
	{
		.id = "IMG_ADLRD",
	},
	{
		.id = "IMG_ADLWR0",
	},
	{
		.id = "IMG_AVS",
	},
	{
		.id = "IMG_IPS",
	},
	{
		.id = "IMG_ADLWR1",
	},
	{
		.id = "IMG_ROOTCQ",
	},
	{
		.id = "IMG_BLS",
	},
	{
		.id = "IMG_SUB_COMMON0",
	},
	{
		.id = "IMG_SUB_COMMON1",
	},
	{
		.id = "IMG_SUB_COMMON2",
	},
	{
		.id = "IMG_SUB_COMMON3",
	},
	{
		.id = "IMG_SUB_COMMON4",
	},
	{
		.id = "IMG_GALS_RX_DIP0",
	},
	{
		.id = "IMG_GALS_RX_DIP1",
	},
	{
		.id = "IMG_GALS_RX_TRAW0",
	},
	{
		.id = "IMG_GALS_RX_WPE0",
	},
	{
		.id = "IMG_GALS_RX_WPE1",
	},
	{
		.id = "IMG_GALS_RX_WPE2",
	},
	{
		.id = "IMG_GALS_TRX_IPE0",
	},
	{
		.id = "IMG_GALS_TRX_IPE1",
	},
	{
		.id = "IMG26",
	},
	{
		.id = "IMG_BWR",
	},

	{
		.id = "IMG_GALS",
	},
	{
		.id = "IMG_ME",
	},
	{
		.id = "IMG_MMG",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS0",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS1",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS2",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS3",
	},
	{
		.id = "DIP_TOP_DIP1_LARB10",
	},
	{
		.id = "DIP_TOP_DIP1_LARB15",
	},
	{
		.id = "DIP_TOP_DIP1_LARB38",
	},
	{
		.id = "DIP_TOP_DIP1_LARB39",
	},
	{
		.id = "DIP_NR1_DIP1_LARB",
	},
	{
		.id = "DIP_NR1_DIP1_DIP_NR1",
	},
	{
		.id = "DIP_NR2_DIP1_DIP_NR",
	},
	{
		.id = "DIP_NR2_DIP1_LARB15",
	},
	{
		.id = "DIP_NR2_DIP1_LARB39",
	},
	{
		.id = "WPE1_DIP1_LARB11",
	},
	{
		.id = "WPE1_DIP1_WPE",
	},
	{
		.id = "WPE1_DIP1_GALS0",
	},
	{
		.id = "WPE2_DIP1_LARB11",
	},
	{
		.id = "WPE2_DIP1_WPE",
	},
	{
		.id = "WPE2_DIP1_GALS0",
	},
	{
		.id = "WPE3_DIP1_LARB11",
	},
	{
		.id = "WPE3_DIP1_WPE",
	},
	{
		.id = "WPE3_DIP1_GALS0",
	},
	{
		.id = "TRAW_DIP1_LARB28",
	},
	{
		.id = "TRAW_DIP1_LARB40",
	},
	{
		.id = "TRAW_DIP1_TRAW",
	},
	{
		.id = "TRAW_DIP1_GALS",
	},
	{
		.id = "TRAW_CAP_DIP1_TRAW_CAP",
	},
};

struct clk_bulk_data imgsys_isp8_clks_mt6899[] = {
	{
		.id = "VCORE_GALS",
	},
	{
		.id = "VCORE_MAIN",
	},
	{
		.id = "VCORE_SUB0",
	},
	{
		.id = "VCORE_SUB1",
	},
	{
		.id = "IMG_LARB9",
	},
	{
		.id = "IMG_TRAW0",
	},
	{
		.id = "IMG_TRAW1",
	},
	{
		.id = "IMG_DIP0",
	},
	{
		.id = "IMG_WPE0",
	},
	{
		.id = "IMG_IPE",
	},
	{
		.id = "IMG_WPE1",
	},
	{
		.id = "IMG_WPE2",
	},
	{
		.id = "IMG_ADL_LARB",
	},
	{
		.id = "IMG_ADLRD",
	},
	{
		.id = "IMG_ADLWR0",
	},
	{
		.id = "IMG_AVS",
	},
	{
		.id = "IMG_IPS",
	},
	{
		.id = "IMG_ADLWR1",
	},
	{
		.id = "IMG_ROOTCQ",
	},
	{
		.id = "IMG_BLS",
	},
	{
		.id = "IMG_SUB_COMMON0",
	},
	{
		.id = "IMG_SUB_COMMON1",
	},
	{
		.id = "IMG_SUB_COMMON2",
	},
	{
		.id = "IMG_SUB_COMMON3",
	},
	{
		.id = "IMG_SUB_COMMON4",
	},
	{
		.id = "IMG_GALS_RX_DIP0",
	},
	{
		.id = "IMG_GALS_RX_DIP1",
	},
	{
		.id = "IMG_GALS_RX_TRAW0",
	},
	{
		.id = "IMG_GALS_RX_WPE0",
	},
	{
		.id = "IMG_GALS_RX_WPE1",
	},
	{
		.id = "IMG_GALS_RX_WPE2",
	},
	{
		.id = "IMG_GALS_TRX_IPE0",
	},
	{
		.id = "IMG_GALS_TRX_IPE1",
	},
	{
		.id = "IMG26",
	},
	{
		.id = "IMG_BWR",
	},

	{
		.id = "IMG_GALS",
	},
	{
		.id = "IMG_ME",
	},
	{
		.id = "IMG_MMG",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS0",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS1",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS2",
	},
	{
		.id = "DIP_TOP_DIP1_DIP_TOP_GALS3",
	},
	{
		.id = "DIP_TOP_DIP1_LARB10",
	},
	{
		.id = "DIP_TOP_DIP1_LARB15",
	},
	{
		.id = "DIP_TOP_DIP1_LARB38",
	},
	{
		.id = "DIP_TOP_DIP1_LARB39",
	},
	{
		.id = "DIP_NR1_DIP1_LARB",
	},
	{
		.id = "DIP_NR1_DIP1_DIP_NR1",
	},
	{
		.id = "DIP_NR2_DIP1_DIP_NR",
	},
	{
		.id = "DIP_NR2_DIP1_LARB15",
	},
	{
		.id = "DIP_NR2_DIP1_LARB39",
	},
	{
		.id = "WPE1_DIP1_LARB11",
	},
	{
		.id = "WPE1_DIP1_WPE",
	},
	{
		.id = "WPE1_DIP1_GALS0",
	},
	{
		.id = "WPE2_DIP1_LARB11",
	},
	{
		.id = "WPE2_DIP1_WPE",
	},
	{
		.id = "WPE2_DIP1_GALS0",
	},
	{
		.id = "WPE3_DIP1_LARB11",
	},
	{
		.id = "WPE3_DIP1_WPE",
	},
	{
		.id = "WPE3_DIP1_GALS0",
	},
	{
		.id = "TRAW_DIP1_LARB28",
	},
	{
		.id = "TRAW_DIP1_LARB40",
	},
	{
		.id = "TRAW_DIP1_TRAW",
	},
	{
		.id = "TRAW_DIP1_GALS",
	},
	{
		.id = "TRAW_CAP_DIP1_TRAW_CAP",
	},
};

#define MTK_IMGSYS_CLK_NUM_MT6991	ARRAY_SIZE(imgsys_isp8_clks_mt6991)
#define MTK_IMGSYS_CLK_NUM_MT6899	ARRAY_SIZE(imgsys_isp8_clks_mt6899)

#endif /* _MTK_IMGSYS_PLAT_H_ */
