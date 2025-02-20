// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   wlan_lib.c
 *    \brief  Internal driver stack will export the required procedures here for
 *            GLUE Layer.
 *
 *    This file contains all routines which are exported from MediaTek 802.11
 *    Wireless LAN driver stack to GLUE Layer.
 */


/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"
#include "mgmt/ais_fsm.h"
#include "mddp.h"
#include "gl_kal.h"
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
#include "gl_coredump.h"
#endif
/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/* 6.1.1.2 Interpretation of priority parameter in MAC service primitives */
/* Static convert the Priority Parameter/TID(User Priority/TS Identifier) to
 * Traffic Class
 */
const uint8_t aucPriorityParam2TC[] = {
	TC1_INDEX,
	TC0_INDEX,
	TC0_INDEX,
	TC1_INDEX,
	TC2_INDEX,
	TC2_INDEX,
	TC3_INDEX,
	TC3_INDEX
};

#if (CFG_WOW_SUPPORT == 1)
/* HIF suspend should wait for cfg80211 suspend done.
 * by experience, 5ms is enough, and worst case ~= 250ms.
 * if > 250 ms --> treat as no cfg80211 suspend
 */
#define HIF_SUSPEND_MAX_WAIT_TIME 50 /* unit: 5ms */
#endif

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
struct CODE_MAPPING {
	uint32_t u4RegisterValue;
	int32_t u4TxpowerOffset;
};

struct NVRAM_TAG_FRAGMENT_GROUP {
	uint8_t u1StartTagID;
	uint8_t u1EndTagID;
};

struct NVRAM_FRAGMENT_RANGE {
	uint32_t startOfs;
	uint32_t endOfs;
};


/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
u_int8_t fgIsMcuOff;
u_int8_t fgIsBusAccessFailed = FALSE;
#if CFG_MTK_WIFI_PCIE_SUPPORT
u_int8_t fgIsPcieDataTransDisabled = FALSE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
u_int8_t fgTriggerDebugSop = FALSE;
#endif

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
static uint8_t wifi_test_mode_fwdl;
static uint8_t wifi_in_switch_mode;
#endif

/* data rate mapping table for CCK */
struct cckDataRateMappingTable_t {
	uint32_t rate[4];
} g_rCckDataRateMappingTable = {
	{10, 20, 55, 110}
};
/* data rate mapping table for OFDM */
struct ofdmDataRateMappingTable_t {
	uint32_t rate[8];
} g_rOfdmDataRateMappingTable = {
	{60, 90, 120, 180, 240, 360, 480, 540}
};
/* data rate mapping table for 802.11n and 802.11ac */
struct dataRateMappingTable_t {
	struct nsts_t {
		struct bw_t {
			struct sgi_t {
				uint32_t rate[12];
			} sgi[2];
		} bw[4];
	} nsts[3];
} g_rDataRateMappingTable = {
{ { { { { /* HT/VHT NSTS=1, 20MHz */
	{ /* no SGI */
	{65, 130, 195, 260, 390, 520, 585, 650, 780, 867, 975, 1083}
	},
	{ /* SGI */
	{72, 144, 217, 289, 433, 578, 650, 722, 867, 963, 1083, 1204}
	}
} },
{
	{ /* HT/VHT NSTS=1, 40MHz */
	{ /* no SGI */
	{135, 270, 405, 540, 810, 1080, 1215, 1350, 1620, 1800, 2025, 2250}
	},
	{ /* SGI */
	{150, 300, 450, 600, 900, 1200, 1350, 1500, 1800, 2000, 2250, 2500}
	}
} },
{
	{ /* HT/VHT NSTS=1, 80MHz */
	{ /* no SGI */
	{293, 585, 878, 1170, 1755, 2340, 2633, 2925, 3510, 3900, 4388, 4875}
	},
	{ /* SGI */
	{325, 650, 975, 1300, 1950, 2600, 2925, 3250, 3900, 4333, 4875, 5417}
	}
} },
{
	{ /* HT/VHT NSTS=1, 160MHz */
	{ /* no SGI */
	{585, 1170, 1755, 2340, 3510, 4680, 5265, 5850, 7020, 7800, 8775, 9750}
	},
	{ /* SGI */
	{650, 1300, 1950, 2600, 3900, 5200, 5850, 6500, 7800, 8667, 9750, 10833}
	}
} } } },
{ { {
	{ /* HT/VHT NSTS=2, 20MHz */
	{ /* no SGI */
	{130, 260, 390, 520, 780, 1040, 1170, 1300, 1560, 1733, 1950, 2167}
	},
	{ /* SGI */
	{144, 289, 433, 578, 867, 1156, 1300, 1444, 1733, 1927, 2167, 2407}
	}
} },
{
	{ /* HT/VHT NSTS=2, 40MHz */
	{ /* no SGI */
	{270, 540, 810, 1080, 1620, 2160, 2430, 2700, 3240, 3600, 4050, 4500}
	},
	{ /* SGI */
	{300, 600, 900, 1200, 1800, 2400, 2700, 3000, 3600, 4000, 4500, 5000}
	}
} },
{
	{ /* HT/VHT NSTS=2, 80MHz */
	{ /* no SGI */
	{585, 1170, 1755, 2340, 3510, 4680, 5265, 5850, 7020, 7800, 8775, 9750}
	},
	{ /* SGI */
	{650, 1300, 1950, 2600, 3900, 5200, 5850, 6500, 7800, 8667, 9750, 10833}
	}
} },
{
	{ /* HT/VHT NSTS=2, 160MHz */
	{ /* no SGI */
	{1170, 2340, 3510, 4680, 7020, 9360, 10530, 11700, 14040, 15600,
	 17550, 19500}
	},
	{ /* SGI */
	{1300, 2600, 3900, 5200, 7800, 10400, 11700, 13000, 15600, 17333,
	 19500, 21667}
	}
} } } },
{ { {
	{ /* HT/VHT NSTS=3, 20MHz */
	{ /* no SGI */
	{195, 390, 585, 780, 1170, 1560, 1755, 1950, 2340, 2600, 2925, 3250}
	},
	{ /* SGI */
	{217, 433, 650, 867, 1300, 1733, 1950, 2167, 2600, 2889, 3250, 3611}
	}
} },
{
	{ /* HT/VHT NSTS=3, 40MHz */
	{ /* no SGI */
	{405, 810, 1215, 1620, 2430, 3240, 3645, 4050, 4860, 5400, 6075, 6750}
	},
	{ /* SGI */
	{450, 900, 1350, 1800, 2700, 3600, 4050, 4500, 5400, 6000, 6750, 7500}
	}
} },
{
	{ /* HT/VHT NSTS=3, 80MHz */
	{ /* no SGI */
	{878, 1755, 2633, 3510, 5265, 7020, 0, 8775, 10530, 11700, 13163, 14625}
	},
	{ /* SGI */
	{975, 1950, 2925, 3900, 5850, 7800, 0, 9750, 11700, 13000, 14625, 16250}
	}
} },
{
	{ /* HT/VHT NSTS=3, 160MHz */
	{ /* no SGI */
	{1755, 3510, 5265, 7020, 10530, 14040, 15795, 17550, 21060, 23400,
	 26325, 29250}
	},
	{ /* SGI */
	{1950, 3900, 5850, 7800, 11700, 15600, 17550, 19500, 23400, 26000,
	 29250, 32500}
	}
} } } } }
};

/* data rate mapping table for ax */
struct axDataRateMappingTable_t {
	struct axNsts_t {
		struct axBw_t {
			struct axGi_t {
				uint32_t rate[16];
			} gi[3];
		} bw[5];
	} nsts[4];
} g_rAxDataRateMappingTable = {
{ { { { { /* HE/EHT NSTS=1, 20MHz */
	{ /* GI 0.8 */
	{86, 172, 258, 344, 516, 688, 774, 860, 1032, 1147, 1290, 1434,
		1549, 1721, 0, 43}
	},
	{ /* GI 1.6 */
	{81, 163, 244, 325, 488, 650, 731, 813, 975, 1083, 1219, 1354,
		1463, 1625, 0, 40}
	},
	{ /* GI 3.2 */
	{73, 146, 219, 293, 439, 585, 658, 731, 878, 975, 1097, 1219,
		1316, 1463, 0, 36}
	}
} },
{
	{ /* HE/EHT NSTS=1, 40MHz */
	{ /* GI 0.8 */
	{172, 344, 516, 688, 1032, 1376, 1549, 1721, 2065, 2294, 2581, 2868,
		3097, 3441, 0, 86}
	},
	{ /* GI 1.6 */
	{163, 325, 488, 650, 975, 1300, 1463, 1625, 1950, 2167, 2438, 2708,
		2925, 3250, 0, 81}
	},
	{ /* GI 3.2 */
	{146, 293, 439, 585, 878, 1170, 1316, 1463, 1755, 1950, 2194, 2438,
		2633, 2925, 0, 73}
	}
} },
{
	{ /* HE/EHT NSTS=1, 80MHz */
	{ /* GI 0.8 */
	{360, 721, 1081, 1441, 2162, 2882, 3243, 3603, 4324, 4804, 5404, 6005,
		6485, 7206, 0, 180}
	},
	{ /* GI 1.6 */
	{340, 681, 1021, 1361, 2042, 2722, 3063, 3403, 4083, 4537, 5104, 5671,
		6125, 6806, 0, 170}
	},
	{ /* GI 3.2 */
	{306, 613, 919, 1225, 1838, 2450, 2756, 3063, 3675, 4083, 4594, 5104,
		5513, 6125, 0, 153}
	}
} },
{
	{ /* HE/EHT NSTS=1, 160MHz */
	{ /* GI 0.8 */
	{721, 1441, 2162, 2882, 4324, 5765, 6485, 7206, 8647, 9608, 10809,
		12010, 12971, 14412, 0, 360}
	},
	{ /* GI 1.6 */
	{681, 1361, 2042, 2722, 4083, 5444, 6125, 6806, 8167, 9074, 10208,
		11343, 12250, 13611, 0, 340}
	},
	{ /* GI 3.2 */
	{613, 1225, 1838, 2450, 3675, 4900, 5513, 6125, 7350, 8167, 9188, 10208,
		11025, 12250, 0, 306}
	}
} },
{
	{ /* HE/EHT NSTS=1, 320MHz */
	{ /* GI 0.8 */
	{1441, 2882, 4324, 5765, 8647, 11529, 12971, 14412, 17294, 19215, 21618,
		24019, 25941, 28824, 0, 721}
	},
	{ /* GI 1.6 */
	{1361, 2722, 4083, 5444, 8167, 10889, 12250, 13611, 16333, 18148, 20417,
		22685, 24500, 27222, 0, 681}
	},
	{ /* GI 3.2 */
	{1225, 2450, 3675, 4900, 7350, 9800, 11025, 12250, 14700, 16333, 18375,
		20416, 22050, 24500, 0, 613}
	}
} } } },
{ { {
	{ /* HE/EHT NSTS=2, 20MHz */
	{ /* GI 0.8 */
	{172, 344, 516, 688, 1032, 1376, 1549, 1721, 2065, 2294, 2581, 2868,
		3097, 3441, 0, 86}
	},
	{ /* GI 1.6 */
	{163, 325, 488, 650, 975, 1300, 1463, 1625, 1950, 2167, 2438, 2708,
		2925, 3250, 0, 81}
	},
	{ /* GI 3.2 */
	{146, 293, 439, 585, 878, 1170, 1316, 1463, 1755, 1950, 2194, 2438,
		2633, 2925, 0, 73}
	}
} },
{
	{ /* HE/EHT NSTS=2, 40MHz */
	{ /* GI 0.8 */
	{344, 688, 1032, 1376, 2065, 2753, 3097, 3441, 4129, 4588, 5162, 5735,
		6194, 6882, 0, 172}
	},
	{ /* GI 1.6 */
	{325, 650, 975, 1300, 1950, 2600, 2925, 3250, 3900, 4333, 4875, 5417,
		5850, 6500, 0, 162}
	},
	{ /* GI 3.2 */
	{293, 585, 878, 1170, 1755, 2340, 2633, 2925, 3510, 3900, 4388, 4875,
		5326, 5850, 0, 292}
	}
} },
{
	{ /* HE/EHT NSTS=2, 80MHz */
	{ /* GI 0.8 */
	{721, 1441, 2162, 2882, 4324, 5765, 6485, 7206, 8647, 9608, 10809,
		12010, 12971, 14412, 0, 360}
	},
	{ /* GI 1.6 */
	{681, 1361, 2042, 2722, 4083, 5444, 6125, 6806, 8167, 9074, 10208,
		11343, 12250, 13611, 0, 340}
	},
	{ /* GI 3.2 */
	{613, 1225, 1838, 2450, 3675, 4900, 5513, 6125, 7350, 8167, 9188, 10208,
		11025, 12250, 0, 306}
	}
} },
{
	{ /* HE/EHT NSTS=2, 160MHz */
	{ /* GI 0.8 */
	{1441, 2882, 4324, 5765, 8647, 11529, 12971, 14412, 17294, 19215, 21618,
		24019, 25941, 28824, 0, 721}
	},
	{ /* GI 1.6 */
	{1361, 2722, 4083, 5444, 8167, 10889, 12250, 13611, 16333, 18148, 20417,
		22685, 24500, 27222, 0, 681}
	},
	{ /* GI 3.2 */
	{1225, 2450, 3675, 4900, 7350, 9800, 11025, 12250, 14700, 16333, 18375,
		20416, 22050, 24500, 0, 613}
	}
} },
{
	{ /* HE/EHT NSTS=2, 320MHz */
	{ /* GI 0.8 */
	{2882, 5765, 8647, 11529, 17294, 23059, 25941, 28824, 34588, 38431,
		43235, 48039, 51882, 57648, 0, 1442}
	},
	{ /* GI 1.6 */
	{2722, 5444, 8167, 10889, 16333, 21778, 24500, 27222, 32667, 36296,
		40833, 45370, 49000, 54444, 0, 1362}
	},
	{ /* GI 3.2 */
	{2450, 4900, 7350, 9800, 14700, 19600, 22050, 24500, 29400, 32666,
		36750, 40833, 44100, 49000, 0, 1226}
	}
} } } },
{ { {
	{ /* HE/EHT NSTS=3, 20MHz */
	{ /* GI 0.8 */
	{258, 516, 774, 1032, 1549, 2065, 2323, 2581, 3097, 3441, 3871, 4301,
		4646, 5162, 0, 129}
	},
	{ /* GI 1.6 */
	{244, 488, 731, 975, 1463, 1950, 2194, 2438, 2925, 3250, 3656, 4063,
		4388, 4875, 0, 122}
	},
	{ /* GI 3.2 */
	{219, 439, 658, 878, 1316, 1755, 1974, 2194, 2633, 2925, 3291, 3656,
		3949, 4388, 0, 109}
	}
} },
{
	{ /* HE/EHT NSTS=3, 40MHz */
	{ /* GI 0.8 */
	{516, 1032, 1549, 2065, 3097, 4129, 4646, 5162, 6194, 6882, 7743, 8603,
		9292, 10324, 0, 258}
	},
	{ /* GI 1.6 */
	{488, 975, 1463, 1950, 2925, 3900, 4388, 4875, 5850, 6500, 7313, 8125,
		8776, 9750, 0, 244}
	},
	{ /* GI 3.2 */
	{439, 878, 1316, 1755, 2633, 3510, 3949, 4388, 5265, 5850, 6581, 7313,
		7898, 8776, 0, 218}
	}
} },
{
	{ /* HE/EHT NSTS=3, 80MHz */
	{ /* GI 0.8 */
	{1081, 2162, 3243, 4324, 6485, 8647, 9728, 10809, 12971, 14412, 16213,
		18015, 19456, 21618, 0, 540}
	},
	{ /* GI 1.6 */
	{1021, 2042, 3063, 4083, 6125, 8167, 9188, 10208, 12250, 13611, 15313,
		17014, 18375, 20417, 0, 510}
	},
	{ /* GI 3.2 */
	{919, 1838, 2756, 3675, 5513, 7350, 8269, 9188, 11025, 12250, 13781,
		15313, 16538, 18375, 0, 459}
	}
} },
{
	{ /* HE/EHT NSTS=3, 160MHz */
	{ /* GI 0.8 */
	{2162, 4324, 6485, 8647, 12971, 17294, 19456, 21618, 25941, 28824,
		32426, 36029, 38912, 43236, 0, 1080}
	},
	{ /* GI 1.6 */
	{2042, 4083, 6125, 8167, 12250, 16333, 18375, 20417, 24500, 27222,
		30625, 34028, 36750, 40834, 0, 1020}
	},
	{ /* GI 3.2 */
	{1838, 3675, 5513, 7350, 11025, 14700, 16538, 18375, 22050, 24500,
		27563, 30625, 33076, 36750, 0, 918}
	}
} },
{
	{ /* HE/EHT NSTS=3, 320MHz */
	{ /* GI 0.8 */
	{4324, 8647, 12971, 17294, 25941, 34588, 38912, 43235, 51882, 57647,
		64853, 72059, 77824, 86472, 0, 2160}
	},
	{ /* GI 1.6 */
	{4083, 8167, 12250, 16333, 24500, 32667, 36750, 40833, 49000, 54444,
		61250, 68056, 73500, 81668, 0, 2040}
	},
	{ /* GI 3.2 */
	{3675, 7350, 11025, 14700, 22050, 29400, 33075, 36750, 44100, 49000,
		55125, 61250, 66152, 73500, 0, 1836}
	}
} } } },
{ { {
	{ /* HE/EHT NSTS=4, 20MHz */
	{ /* GI 0.8 */
	{344, 688, 1032, 1376, 2065, 2753, 3097, 3441, 4129, 4588, 5162, 5735,
		6194, 6882, 0, 172}
	},
	{ /* GI 1.6 */
	{325, 650, 975, 1300, 1950, 2600, 2925, 3250, 3900, 4333, 4875, 5417,
		5850, 6500, 0, 162}
	},
	{ /* GI 3.2 */
	{293, 585, 878, 1170, 1755, 2340, 2633, 2925, 3510, 3900, 4388, 4875,
		5326, 5850, 0, 146}
	}
} },
{
	{ /* HE/EHT NSTS=4, 40MHz */
	{ /* GI 0.8 */
	{688, 1376, 2065, 2753, 4129, 5506, 6194, 6882, 8259, 9176, 10324,
		11471, 12388, 13764, 0, 344}
	},
	{ /* GI 1.6 */
	{650, 1300, 1950, 2600, 3900, 5200, 5850, 6500, 7800, 8667, 9750, 10833,
		11700, 13000, 0, 324}
	},
	{ /* GI 3.2 */
	{585, 1170, 1755, 2340, 3510, 4680, 5265, 5850, 7020, 7800, 8775, 9750,
		10652, 11700, 0, 292}
	}
} },
{
	{ /* HE/EHT NSTS=4, 80MHz */
	{ /* GI 0.8 */
	{1441, 2882, 4324, 5765, 8647, 11529, 12971, 14412, 17294, 19215, 21618,
		24019, 25941, 28824, 0, 721}
	},
	{ /* GI 1.6 */
	{1361, 2722, 4083, 5444, 8167, 10889, 12250, 13611, 16333, 18148, 20417,
		22685, 24500, 27222, 0, 681}
	},
	{ /* GI 3.2 */
	{1225, 2450, 3675, 4900, 7350, 9800, 11025, 12250, 14700, 16333, 18375,
		20416, 22050, 24500, 0, 613}
	}
} },
{
	{ /* HE/EHT NSTS=4, 160MHz */
	{ /* GI 0.8 */
	{2882, 5765, 8647, 11529, 17294, 23059, 25941, 28824, 34588, 38431,
		43235, 48039, 51882, 57648, 0, 1442}
	},
	{ /* GI 1.6 */
	{2722, 5444, 8167, 10889, 16333, 21778, 24500, 27222, 32667, 36296,
		40833, 45370, 49000, 54444, 0, 1362}
	},
	{ /* GI 3.2 */
	{2450, 4900, 7350, 9800, 14700, 19600, 22050, 24500, 29400, 32666,
		36750, 40833, 44100, 49000, 0, 1226}
	}
} },
{
	{ /* HE/EHT NSTS=4, 320MHz */
	{ /* GI 0.8 */
	{5765, 11529, 17294, 23059, 34588, 46118, 51882, 57647, 69176, 76863,
		86471, 96078, 103764, 115296, 0, 2884}
	},
	{ /* GI 1.6 */
	{5444, 10889, 16333, 21778, 32667, 43556, 49000, 54444, 65333, 72592,
		81667, 90740, 98000, 108888, 0, 2724}
	},
	{ /* GI 3.2 */
	{4900, 9800, 14700, 19600, 29400, 39200, 44100, 49000, 58800, 65333,
		73500, 81666, 88200, 98000, 0, 2452}
	}
} } } } }
};

struct PARAM_CUSTOM_KEY_CFG_STRUCT g_rEmCfgBk[WLAN_CFG_REC_ENTRY_NUM_MAX];


struct PARAM_CUSTOM_KEY_CFG_STRUCT g_rDefaulteSetting[] = {
	/*format :
	*: {
	*	"firmware config parameter",
	*	"firmware config value",
	*	"Operation:default 0"
	*   }
	*/
	{"AdapScan", "0x0", WLAN_CFG_DEFAULT},
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
	/*Fill Iot AP blocklist here*/
	/*AS AX89X, OUI=0x8CFDF0, NSS=8, PHY=WiFi6, Action=2:Disable SG*/
	{"IOTAP31", "0:8CFDF0:::::8:6::2"},
#endif
#if CFG_TC3_FEATURE
	{"ScreenOnBeaconTimeoutCount", "20"},
	{"ScreenOffBeaconTimeoutCount", "10"},
	{"AgingPeriod", "0x19"},
	{"DropPacketsIPV4Low", "0x1"},
	{"DropPacketsIPV6Low", "0x1"},
	{"Sta2gBw", "1"},
#endif
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */
#define SIGNED_EXTEND(n, _sValue) \
	(((_sValue) & BIT((n)-1)) ? ((_sValue) | BITS(n, 31)) : \
	 ((_sValue) & ~BITS(n, 31)))

/* TODO: Check */
/* OID set handlers without the need to access HW register */
PFN_OID_HANDLER_FUNC apfnOidSetHandlerWOHwAccess[] = {
	wlanoidSetChannel,
	wlanoidSetBeaconInterval,
	wlanoidSetAtimWindow,
	wlanoidSetFrequency,
};

/* TODO: Check */
/* OID query handlers without the need to access HW register */
PFN_OID_HANDLER_FUNC apfnOidQueryHandlerWOHwAccess[] = {
	wlanoidQueryBssid,
	wlanoidQuerySsid,
	wlanoidQueryInfrastructureMode,
	wlanoidQueryAuthMode,
	wlanoidQueryEncryptionStatus,
	wlanoidQueryNetworkTypeInUse,
	wlanoidQueryBssidList,
	wlanoidQueryAcpiDevicePowerState,
	wlanoidQuerySupportedRates,
	wlanoidQuery802dot11PowerSaveProfile,
	wlanoidQueryBeaconInterval,
	wlanoidQueryAtimWindow,
	wlanoidQueryFrequency,
};

/* OID set handlers allowed in RF test mode */
PFN_OID_HANDLER_FUNC apfnOidSetHandlerAllowedInRFTest[] = {
#if CFG_SUPPORT_QA_TOOL
	wlanoidRftestSetTestMode,
	wlanoidRftestSetAbortTestMode,
	wlanoidRftestSetAutoTest,
#endif
	wlanoidSetMcrWrite,
	wlanoidSetEepromWrite
};

/* OID query handlers allowed in RF test mode */
PFN_OID_HANDLER_FUNC apfnOidQueryHandlerAllowedInRFTest[] = {
#if CFG_SUPPORT_QA_TOOL
	wlanoidRftestQueryAutoTest,
#endif
	wlanoidQueryMcrRead,
	wlanoidQueryEepromRead
}

;

PFN_OID_HANDLER_FUNC apfnOidWOTimeoutCheck[] = {
#if CFG_SUPPORT_QA_TOOL
	wlanoidRftestSetTestMode,
	wlanoidRftestSetAbortTestMode,
#endif
	wlanoidSetAcpiDevicePowerState,
};

#define NVRAM_TAG_HDR_SIZE  3 /*ID+Len MSB+Len LSB*/

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This is a private routine, which is used to check if HW access is
 *        needed for the OID query/ set handlers.
 *
 * \param[IN] pfnOidHandler Pointer to the OID handler.
 * \param[IN] fgSetInfo     It is a Set information handler.
 *
 * \retval TRUE This function needs HW access
 * \retval FALSE This function does not need HW access
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanIsHandlerNeedHwAccess(PFN_OID_HANDLER_FUNC
				   pfnOidHandler, u_int8_t fgSetInfo)
{
	PFN_OID_HANDLER_FUNC *apfnOidHandlerWOHwAccess;
	uint32_t i;
	uint32_t u4NumOfElem;

	if (fgSetInfo) {
		apfnOidHandlerWOHwAccess = apfnOidSetHandlerWOHwAccess;
		u4NumOfElem = ARRAY_SIZE(apfnOidSetHandlerWOHwAccess);
	} else {
		apfnOidHandlerWOHwAccess = apfnOidQueryHandlerWOHwAccess;
		u4NumOfElem = ARRAY_SIZE(apfnOidQueryHandlerWOHwAccess);
	}

	for (i = 0; i < u4NumOfElem; i++) {
		if (apfnOidHandlerWOHwAccess[i] == pfnOidHandler)
			return FALSE;
	}

	return TRUE;
} /* wlanIsHandlerNeedHwAccess */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set flag for later handling card
 *        ejected event.
 *
 * \param[in] prAdapter Pointer to the Adapter structure.
 *
 * \return (none)
 *
 * \note When surprised removal happens, Glue layer should invoke this
 *       function to notify WPDD not to do any hw access.
 */
/*----------------------------------------------------------------------------*/
void wlanCardEjected(struct ADAPTER *prAdapter)
{
	/* INITLOG(("\n")); */

	ASSERT(prAdapter);

	/* mark that the card is being ejected, NDIS will shut us down soon */
	nicTxRelease(prAdapter, FALSE);

} /* wlanCardEjected */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to check driver ready state
 *
 * \param[in] prGlueInfo Pointer to the GlueInfo structure.
 *
 * \retval TRUE Driver is ready for kernel access
 * \retval FALSE Driver is not ready
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanIsDriverReady(struct GLUE_INFO *prGlueInfo,
				  uint32_t u4Check)
{
	u_int8_t fgIsReady = TRUE;
	uint8_t u1State = 0;

	if (!prGlueInfo)
		fgIsReady = FALSE;
	else {
		if ((u4Check & WLAN_DRV_READY_CHECK_WLAN_ON) &&
			(prGlueInfo->u4ReadyFlag == FALSE))
			fgIsReady = FALSE;

		if ((u4Check & WLAN_DRV_READY_CHECK_HIF_SUSPEND) &&
			(!halIsHifStateReady(prGlueInfo, &u1State))) {
			DBGLOG(REQ, WARN, "driver state[%d]\n", u1State);
			fgIsReady = FALSE;
		}

		if ((u4Check & WLAN_DRV_READY_CHECK_RESET) &&
			kalIsResetting())
			fgIsReady = FALSE;
	}

	return fgIsReady;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Create adapter object
 *
 * \param prAdapter This routine is call to allocate the driver software
 *		    objects. If fails, return NULL.
 * \retval NULL If it fails, NULL is returned.
 * \retval NOT NULL If the adapter was initialized successfully.
 */
/*----------------------------------------------------------------------------*/
struct ADAPTER *wlanAdapterCreate(struct GLUE_INFO
				  *prGlueInfo)
{
	struct ADAPTER *prAdpater = (struct ADAPTER *) NULL;

	do {
		prAdpater = (struct ADAPTER *) kalMemAlloc(sizeof(
					struct ADAPTER), VIR_MEM_TYPE);

		if (!prAdpater) {
			DBGLOG(INIT, ERROR,
			       "Allocate ADAPTER memory ==> FAILED\n");
			break;
		}
#if QM_TEST_MODE
		g_rQM.prAdapter = prAdpater;
#endif
		kalMemZero(prAdpater, sizeof(struct ADAPTER));
		prAdpater->prGlueInfo = prGlueInfo;

	} while (FALSE);

	return prAdpater;
}				/* wlanAdapterCreate */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Destroy adapter object
 *
 * \param prAdapter This routine is call to destroy the driver software objects.
 *                  If fails, return NULL.
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanAdapterDestroy(struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return;

	scanLogCacheFlushAll(prAdapter,
		&(prAdapter->rWifiVar.rScanInfo.rScanLogCache),
		LOG_SCAN_D2D);

	kalMemFree(prAdapter, VIR_MEM_TYPE, sizeof(struct ADAPTER));
}

void wlanOnPreAllocAdapterMem(struct ADAPTER *prAdapter,
			  const u_int8_t bAtResetFlow)
{
	uint32_t i = 0, j = 0;

	DBGLOG(INIT, TRACE, "start.\n");

	if (!bAtResetFlow) {
		/* 4 <0> Reset variables in ADAPTER_T */
		/* prAdapter->fgIsFwOwn = TRUE; */
		prAdapter->fgIsEnterD3ReqIssued = FALSE;
		prAdapter->ucHwBssIdNum = HW_BSSID_NUM;
		prAdapter->ucSwBssIdNum = MAX_BSSID_NUM;
		prAdapter->ucWmmSetNum = HW_BSSID_NUM;
		prAdapter->ucP2PDevBssIdx = MAX_BSSID_NUM;
		prAdapter->ucWtblEntryNum = WTBL_SIZE;
		prAdapter->ucTxDefaultWlanIndex = prAdapter->ucWtblEntryNum - 1;

		prAdapter->u4HifDbgFlag = 0;
		prAdapter->u4HifChkFlag = 0;
		prAdapter->u4HifDbgMod = 0;
		prAdapter->u4HifDbgBss = 0;
		prAdapter->u4HifDbgReason = 0;
		prAdapter->u4HifTxHangDumpBitmap = 0;
		prAdapter->u4HifTxHangDumpIdx = 0;
		prAdapter->u4HifTxHangDumpNum = 0;
		prAdapter->ulNoMoreRfb = 0;
		prAdapter->u4WaitRecIdx = 0;
		prAdapter->u4CompRecIdx = 0;
		prAdapter->fgSetLogOnOff = true;
		prAdapter->fgSetLogLevel = true;

		/* Initialize rWlanInfo */
		kalMemSet(&prAdapter->rWlanInfo, 0, sizeof(struct WLAN_INFO));

		/* Initialize aprBssInfo[].
		 * Important: index shall be same
		 *            when mapping between aprBssInfo[]
		 *            and arBssInfoPool[].rP2pDevInfo
		 *            is indexed to final one.
		 */
		for (i = 0; i < MAX_BSSID_NUM + 1; i++)
			prAdapter->aprBssInfo[i] =
				&prAdapter->rWifiVar.arBssInfoPool[i];

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		LINK_INITIALIZE(&prAdapter->rPwrLevelHandlerList);
#endif
	} else {
		/* need to reset these values after the reset flow */
		prAdapter->ulNoMoreRfb = 0;
	}

	prAdapter->u4OwnFailedCount = 0;
	prAdapter->u4OwnFailedLogCount = 0;
	prAdapter->ucCmdSeqNum = 0;
	prAdapter->u4PwrCtrlBlockCnt = 0;
	prAdapter->fgIsPostponeTxEAPOLM3 = FALSE;
#if CFG_SUPPORT_WIFI_SLEEP_COUNT
	prAdapter->fgIsPowerDumpDrvOwn = FALSE;
#endif

	if (bAtResetFlow) {
		for (i = 0; i < (prAdapter->ucSwBssIdNum + 1); i++)
			UNSET_NET_ACTIVE(prAdapter, i);

#if CFG_CE_ASSERT_DUMP
		/* Core dump maybe not complete,so need set
		 * back to FALSE or will result can't fw own.
		 */
		prAdapter->fgN9AssertDumpOngoing = FALSE;
#endif
#if defined(_HIF_SDIO)
		prAdapter->fgMBAccessFail = FALSE;
		prAdapter->prGlueInfo->rHifInfo.fgSkipRx = FALSE;
#endif
		prAdapter->fgIsChipNoAck = FALSE;
	}

	QUEUE_INITIALIZE(&(prAdapter->rPendingCmdQueue));
#if CFG_SUPPORT_MULTITHREAD
	QUEUE_INITIALIZE(&prAdapter->rTxCmdQueue);
	QUEUE_INITIALIZE(&prAdapter->rTxCmdDoneQueue);
	for (i = 0; i < MAX_BSSID_NUM; i++)
		for (j = 0; j < TC_NUM; j++)
			QUEUE_INITIALIZE(&prAdapter->rTxPQueue[i][j]);
#if (CFG_TX_HIF_PORT_QUEUE == 1)
	for (i = 0; i < MAX_BSSID_NUM; i++)
		for (j = 0; j < TC_NUM; j++)
			QUEUE_INITIALIZE(&prAdapter->rTxHifPQueue[i][j]);
#endif
	QUEUE_INITIALIZE(&prAdapter->rRxQueue);
	QUEUE_INITIALIZE(&prAdapter->rTxDataDoneQueue);
#endif

#if (CFG_TX_MGMT_BY_DATA_Q == 1)
	QUEUE_INITIALIZE(&prAdapter->rMgmtDirectTxQueue);
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

	/* 4 <0.1> reset fgIsBusAccessFailed */
	fgIsMcuOff = FALSE;
	fgIsBusAccessFailed = FALSE;
#if CFG_MTK_WIFI_PCIE_SUPPORT
	fgIsPcieDataTransDisabled = FALSE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	fgTriggerDebugSop = FALSE;
#endif
}

void wlanOnPostNicInitAdapter(struct ADAPTER *prAdapter,
	struct REG_INFO *prRegInfo,
	const u_int8_t bAtResetFlow)
{
	DBGLOG(INIT, TRACE, "start.\n");

	/* 4 <2.1> Initialize System Service (MGMT Memory pool and
	 *	   STA_REC)
	 */
	nicInitSystemService(prAdapter, bAtResetFlow);

	if (!bAtResetFlow) {

		/* 4 <2.2> Initialize Feature Options */
		wlanInitFeatureOption(prAdapter);
#if CFG_SUPPORT_MTK_SYNERGY
#if 0 /* u2FeatureReserved is 0 on 6765 */
		if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE) {
			if (prRegInfo->prNvramSettings->u2FeatureReserved &
					BIT(MTK_FEATURE_2G_256QAM_DISABLED))
				prAdapter->rWifiVar.aucMtkFeature[0] &=
					~(MTK_SYNERGY_CAP_SUPPORT_24G_MCS89);
		}
#endif
#endif

		/* 4 <2.3> Overwrite debug level settings */
		wlanCfgSetDebugLevel(prAdapter);

		/* 4 <3> Initialize Tx */
		nicTxInitialize(prAdapter);
	} /* end of bAtResetFlow == FALSE */
#if defined(_HIF_SDIO)
	else {
		/* For SER L0.5, need reset resource before first CMD,
		 *  or maybe will get resource fail (resource exhaustion
		 * before SER).
		 */
		nicTxResetResource(prAdapter);
	}
#endif
	/* 4 <4> Initialize Rx */
	nicRxInitialize(prAdapter);
}

void wlanOnPostFirmwareReady(struct ADAPTER *prAdapter,
		struct REG_INFO *prRegInfo)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	struct WLAN_INFO *prWlanInfo;

	DBGLOG(INIT, TRACE, "start.\n");
	/* OID timeout timer initialize */
	cnmTimerInitTimer(prAdapter,
			  &prAdapter->rOidTimeoutTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) wlanReleasePendingOid,
			  (uintptr_t) NULL);

	prAdapter->ucOidTimeoutCount = 0;

	prAdapter->fgIsChipNoAck = FALSE;

	/* Return Indicated Rfb list timer */
	cnmTimerInitTimer(prAdapter,
			  &prAdapter->rPacketDelaySetupTimer,
			  (PFN_MGMT_TIMEOUT_FUNC)
				wlanReturnPacketDelaySetupTimeout,
			  (uintptr_t) NULL);

	/* Power state initialization */
	prAdapter->fgWiFiInSleepyState = FALSE;
	prAdapter->rAcpiState = ACPI_STATE_D0;

#if 0
	/* Online scan option */
	if (prRegInfo->fgDisOnlineScan == 0)
		prAdapter->fgEnOnlineScan = TRUE;
	else
		prAdapter->fgEnOnlineScan = FALSE;

	/* Beacon lost detection option */
	if (prRegInfo->fgDisBcnLostDetection != 0)
		prAdapter->fgDisBcnLostDetection = TRUE;
#else
	if (prAdapter->rWifiVar.fgDisOnlineScan == 0)
		prAdapter->fgEnOnlineScan = TRUE;
	else
		prAdapter->fgEnOnlineScan = FALSE;

	/* Beacon lost detection option */
	if (prAdapter->rWifiVar.fgDisBcnLostDetection != 0)
		prAdapter->fgDisBcnLostDetection = TRUE;
	if (prAdapter->rWifiVar.fgDisAgingLostDetection != 0)
		prAdapter->fgDisStaAgingTimeoutDetection = TRUE;
#endif

	prWlanInfo = &prAdapter->rWlanInfo;

	/* Load compile time constant */
	prWlanInfo->u2BeaconPeriod = CFG_INIT_ADHOC_BEACON_INTERVAL;
	prWlanInfo->u2AtimWindow = CFG_INIT_ADHOC_ATIM_WINDOW;

#if 1				/* set PM parameters */
	prAdapter->u4PsCurrentMeasureEn =
		prRegInfo->u4PsCurrentMeasureEn;
#if 0
	prAdapter->fgEnArpFilter = prRegInfo->fgEnArpFilter;
	prAdapter->u4UapsdAcBmp = prRegInfo->u4UapsdAcBmp;
	prAdapter->u4MaxSpLen = prRegInfo->u4MaxSpLen;
#else
	prAdapter->fgEnArpFilter =
		prAdapter->rWifiVar.fgEnArpFilter;
	prAdapter->u4UapsdAcBmp = prAdapter->rWifiVar.u4UapsdAcBmp;
	prAdapter->u4MaxSpLen = prAdapter->rWifiVar.u4MaxSpLen;
	prAdapter->u4P2pUapsdAcBmp = prAdapter->rWifiVar.u4P2pUapsdAcBmp;
	prAdapter->u4P2pMaxSpLen = prAdapter->rWifiVar.u4P2pMaxSpLen;
#endif
	DBGLOG(INIT, TRACE,
	       "[1] fgEnArpFilter:0x%x, u4UapsdAcBmp:0x%x, u4MaxSpLen:0x%x",
	       prAdapter->fgEnArpFilter, prAdapter->u4UapsdAcBmp,
	       prAdapter->u4MaxSpLen);

	prAdapter->fgEnCtiaPowerMode = FALSE;

#endif
	/* QA_TOOL and ICAP info struct */
	prAdapter->rIcapInfo.eIcapState = ICAP_STATE_INIT;
#if CFG_SUPPORT_QA_TOOL
	prAdapter->rIcapInfo.u2DumpIndex = 0;
#endif
	prAdapter->rIcapInfo.u4CapNode = 0;

	/* MGMT Initialization */
	nicInitMGMT(prAdapter, prRegInfo);

#if CFG_SUPPORT_NCHO
	wlanNchoInit(prAdapter, FALSE);
#endif

	/* Enable WZC Disassociation */
	prAdapter->rWifiVar.fgSupportWZCDisassociation = TRUE;

	/* Apply Rate Setting */
	if ((enum ENUM_REGISTRY_FIXED_RATE) (prRegInfo->u4FixedRate)
	    < FIXED_RATE_NUM)
		prAdapter->rWifiVar.eRateSetting =
				(enum ENUM_REGISTRY_FIXED_RATE)
				(prRegInfo->u4FixedRate);
	else
		prAdapter->rWifiVar.eRateSetting = FIXED_RATE_NONE;

	if (prAdapter->rWifiVar.eRateSetting == FIXED_RATE_NONE) {
		/* Enable Auto (Long/Short) Preamble */
		prAdapter->rWifiVar.ePreambleType = PREAMBLE_TYPE_AUTO;
	} else if ((prAdapter->rWifiVar.eRateSetting >=
		    FIXED_RATE_MCS0_20M_400NS &&
		    prAdapter->rWifiVar.eRateSetting <=
		    FIXED_RATE_MCS7_20M_400NS)
		   || (prAdapter->rWifiVar.eRateSetting >=
		       FIXED_RATE_MCS0_40M_400NS &&
		       prAdapter->rWifiVar.eRateSetting <=
		       FIXED_RATE_MCS32_400NS)) {
		/* Force Short Preamble */
		prAdapter->rWifiVar.ePreambleType = PREAMBLE_TYPE_SHORT;
	} else {
		/* Force Long Preamble */
		prAdapter->rWifiVar.ePreambleType = PREAMBLE_TYPE_LONG;
	}

	/* Disable Hidden SSID Join */
	prAdapter->rWifiVar.fgEnableJoinToHiddenSSID = FALSE;

	/* Enable Short Slot Time */
	prAdapter->rWifiVar.fgIsShortSlotTimeOptionEnable = TRUE;

	/* Disable skip dfs during scan*/
	prAdapter->rWifiVar.rScanInfo.fgSkipDFS = 0;

	/* Determine whether to disable Partial Scan */
	prAdapter->rWifiVar.fgDisablePartialScan = 0;

	/* configure available PHY type set */
	nicSetAvailablePhyTypeSet(prAdapter);

#if 0				/* Marked for MT6630 */
#if 1				/* set PM parameters */
		{
#if CFG_SUPPORT_PWR_MGT
			prAdapter->u4PowerMode = prRegInfo->u4PowerMode;
#if CFG_ENABLE_WIFI_DIRECT
			prWlanInfo->
			arPowerSaveMode[NETWORK_TYPE_P2P_INDEX].ucNetTypeIndex
				= NETWORK_TYPE_P2P_INDEX;
			prWlanInfo->
			arPowerSaveMode[NETWORK_TYPE_P2P_INDEX].ucPsProfile
				= ENUM_PSP_FAST_SWITCH;
#endif
#else
			prAdapter->u4PowerMode = ENUM_PSP_CONTINUOUS_ACTIVE;
#endif

			nicConfigPowerSaveProfile(prAdapter,
					  prAdapter->prAisBssInfo->ucBssIndex,
					  prAdapter->u4PowerMode, FALSE);
		}

#endif
#endif

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	/* dynamic tx power control initialization */
	/* note: call this API before loading NVRAM */
	txPwrCtrlInit(prAdapter);
#endif

	/* Check hardware 5g band support */
	if (prAdapter->fgIsHw5GBandDisabled)
		prAdapter->fgEnable5GBand = FALSE;
	else
		prAdapter->fgEnable5GBand = TRUE;

#if CFG_SUPPORT_NVRAM
	/* load manufacture data */
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) == TRUE)
		wlanLoadManufactureData(prAdapter, prRegInfo);
	else
		DBGLOG(INIT, WARN, "%s: load manufacture data fail\n",
			       __func__);
#endif

#if 0
		/* Update Auto rate parameters in FW */
		nicRlmArUpdateParms(prAdapter, prRegInfo->u4ArSysParam0,
				    prRegInfo->u4ArSysParam1,
				    prRegInfo->u4ArSysParam2,
				    prRegInfo->u4ArSysParam3);
#endif

	/* Default QM RX BA timeout */
	prAdapter->u4QmRxBaMissTimeout = prWifiVar->u4BaMissTimeoutMs;

#if CFG_SUPPORT_LOWLATENCY_MODE
	wlanAdapterStartForLowLatency(prAdapter);
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if (CFG_WIFI_GET_MCS_INFO == 1)
	prAdapter->fgIsMcsInfoValid = FALSE;
#endif
}

#if CFG_SUPPORT_XONVRAM
/*----------------------------------------------------------------------------*/
/*!
 * \brief Because each SKU will have different platform config, such as
 *        clock type. The platform config will be divided into two parts:
 *        XO NVRAM and clock type/source.
 *        XO NVRAM will be set from offset 0, while other config will be
 *        set from the end of conninfra sysram.
 *        This function should be called before patch download.
 *
 * \param[in]  prGlueInfo        Pointer to the Adapter structure.
 * \param[in]  prXo              Pointer of XO_CFG_PARAM_STRUCT.
 * \param[in]  prPlat            Pointer of platcfg_infra_sysram.
 */
/*----------------------------------------------------------------------------*/
static uint32_t
wlanCopyXonvramToSysram(struct GLUE_INFO *prGlueInfo,
	struct XO_CFG_PARAM_STRUCT *prXo, struct platcfg_infra_sysram *prPlat)
{
#if defined(_HIF_PCIE)
	ASSERT(prGlueInfo);
	ASSERT(prPlat);

	if (prXo == NULL) {
		DBGLOG(INIT, TRACE, "Unsupport xo nvram\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (prXo->u2DataLen == 0) {
		DBGLOG(INIT, TRACE, "Xo nvram length is zero\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (prPlat->size < prXo->u2DataLen) {
		DBGLOG(INIT, WARN, "Invalid length : %d, %d\n"
				, prPlat->size, prXo->u2DataLen);
		return WLAN_STATUS_FAILURE;
	}

	if (kalDevRegWriteRange(prGlueInfo, prPlat->addr,
		prXo, prXo->u2DataLen) == FALSE) {
		DBGLOG(INIT, WARN, "Fail to copy XO to infra sysram\n");
		return WLAN_STATUS_FAILURE;
	}
#endif
	return WLAN_STATUS_SUCCESS;
}
#endif

#if CFG_SUPPORT_CONNAC3X
/*----------------------------------------------------------------------------*/
/*!
 * \brief Because each SKU will have different platform config, such as
 *        clock type. The platform config will be divided into two parts:
 *        XO NVRAM and clock type/source.
 *        XO NVRAM will be set from offset 0, while other config will be
 *        set from the end of conninfra sysram.
 *        This function should be called before patch download.
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[in]  prRegInfo        Pointer of REG_INFO_T.
 */
/*----------------------------------------------------------------------------*/
static uint32_t
wlanCopyPlatCfgToSysram(struct ADAPTER *prAdapter, struct REG_INFO *prRegInfo)
{
	struct GLUE_INFO *prGlueInfo;
	struct platcfg_infra_sysram *prPlatCfg;
#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT)
	uint32_t u4Addr, u4Size;
	uint8_t *pu1Cfg;
#endif

	ASSERT(prAdapter);
	ASSERT(prRegInfo);

	prGlueInfo = prAdapter->prGlueInfo;
	prPlatCfg = &(prAdapter->chip_info->rPlatcfgInfraSysram);
	if ((prPlatCfg->size == 0) || (prPlatCfg->addr == 0)) {
		DBGLOG(INIT, TRACE, "No available infra sysram for plat cfg\n");
		return WLAN_STATUS_SUCCESS;
	}

#if CFG_SUPPORT_XONVRAM
	if (prRegInfo->prXonvCfg == NULL) {
		DBGLOG(INIT, TRACE, "Unsupport xo nvram\n");
		return WLAN_STATUS_SUCCESS;
	}

	if (wlanCopyXonvramToSysram(prGlueInfo, prRegInfo->prXonvCfg, prPlatCfg)
		!= WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, TRACE, "Fail to copy xo nvram\n");
		return WLAN_STATUS_FAILURE;
	}
#endif

#if defined(CFG_MTK_WIFI_CONNV3_SUPPORT)
	pu1Cfg = connv3_get_plat_config(&u4Size);
	if (u4Size == 0) {
		DBGLOG(INIT, TRACE, "No need to copy plat config\n");
		return WLAN_STATUS_SUCCESS;
	}

	if ((u4Size + prRegInfo->prXonvCfg->u2DataLen) > prPlatCfg->size) {
		DBGLOG(INIT, TRACE, "No enough size for plat cfg\n");
		return WLAN_STATUS_FAILURE;
	}

	u4Addr = prPlatCfg->addr + prPlatCfg->size - u4Size;
	if (kalDevRegWriteRange(prGlueInfo, u4Addr, pu1Cfg, u4Size) == FALSE) {
		DBGLOG(INIT, WARN, "Fail to copy plat cfg to infra sysram\n");
		return WLAN_STATUS_FAILURE;
	}
#endif
	return WLAN_STATUS_SUCCESS;
}
#endif /* #if CFG_SUPPORT_CONNAC3X */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize the adapter. The sequence is
 *        1. Disable interrupt
 *        2. Read adapter configuration from EEPROM and registry, verify chip
 *	     ID.
 *        3. Create NIC Tx/Rx resource.
 *        4. Initialize the chip
 *        5. Initialize the protocol
 *        6. Enable Interrupt
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanAdapterStart(struct ADAPTER *prAdapter,
					struct REG_INFO *prRegInfo,
					const u_int8_t bAtResetFlow)
{
	struct mt66xx_chip_info *prChipInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
#if CFG_MTK_WIFI_SW_WFDMA
	struct SW_WFDMA_INFO *prSwWfdmaInfo = NULL;
#endif
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	enum ENUM_ADAPTER_START_FAIL_REASON {
		ALLOC_ADAPTER_MEM_FAIL,
		DRIVER_OWN_FAIL,
		INIT_ADAPTER_FAIL,
		INIT_HIFINFO_FAIL,
		SET_CHIP_ECO_INFO_FAIL,
		COPY_CONNSYS_CFG_FAIL,
		PRE_ON_PROCESS_DONE,
		RAM_CODE_DOWNLOAD_FAIL,
		WAIT_FIRMWARE_READY_FAIL,
		FAIL_REASON_MAX
	} eFailReason;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;
	prBusInfo = prChipInfo->bus_info;
#if CFG_MTK_WIFI_SW_WFDMA
	prSwWfdmaInfo = &prBusInfo->rSwWfdmaInfo;
#endif

	eFailReason = FAIL_REASON_MAX;

	wlanOnPreAllocAdapterMem(prAdapter, bAtResetFlow);

	do {
		if (!bAtResetFlow) {
			u4Status = nicAllocateAdapterMemory(prAdapter);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
						"nicAllocateAdapterMemory Error!\n");
				u4Status = WLAN_STATUS_FAILURE;
				eFailReason = ALLOC_ADAPTER_MEM_FAIL;
				break;
			}

			prAdapter->u4OsPacketFilter
				= PARAM_PACKET_FILTER_SUPPORTED;
		}
		/* set FALSE after wifi init flow or reset (not reinit WFDMA) */
		prAdapter->fgIsFwDownloaded = FALSE;

		DBGLOG(INIT, TRACE,
		       "wlanAdapterStart(): Acquiring LP-OWN\n");
		prAdapter->fgIsWiFiOnDrvOwn = TRUE;
		ACQUIRE_POWER_CONTROL_FROM_PM(prAdapter);
		prAdapter->fgIsWiFiOnDrvOwn = FALSE;
		DBGLOG(INIT, TRACE,
		       "wlanAdapterStart(): Acquiring LP-OWN-end\n");

#if (CFG_ENABLE_FULL_PM == 0)
		nicpmSetDriverOwn(prAdapter);
#endif

#if !defined(_HIF_USB)
		if (prAdapter->fgIsFwOwn == TRUE) {
			DBGLOG(INIT, ERROR, "nicpmSetDriverOwn() failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			eFailReason = DRIVER_OWN_FAIL;
			GL_DEFAULT_RESET_TRIGGER(prAdapter,
						 RST_WIFI_ON_DRV_OWN_FAIL);
			break;
		}
#endif

#if CFG_MTK_MDDP_SUPPORT
		setMddpSupportRegister(prAdapter);
#endif
		if (!bAtResetFlow) {
			/* 4 <1> Initialize the Adapter */
			u4Status = nicInitializeAdapter(prAdapter);
			if (u4Status != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"nicInitializeAdapter failed!\n");
				u4Status = WLAN_STATUS_FAILURE;
				eFailReason = INIT_ADAPTER_FAIL;
				break;
			}
		}

		wlanOnPostNicInitAdapter(prAdapter, prRegInfo, bAtResetFlow);

#if CFG_MTK_WIFI_SW_WFDMA
		if (prSwWfdmaInfo->fgIsEnAfterFwdl) {
			if (prSwWfdmaInfo->rOps.enable)
				prSwWfdmaInfo->rOps.enable(
					prAdapter->prGlueInfo, false);
		}
#endif
		u4Status = wlanWakeUpWiFi(prAdapter);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "wlanWakeUpWiFi failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}

		/* 4 <5> HIF SW info initialize */
		if (!halHifSwInfoInit(prAdapter)) {
			DBGLOG(INIT, ERROR, "halHifSwInfoInit failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			eFailReason = INIT_HIFINFO_FAIL;
			break;
		}

#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
		if (fgIsPreOnProcessing) {
			wifi_coredump_post_start();
		}
#endif

		fw_log_init(prAdapter);

		/* 4 <6> Enable HIF cut-through to N9 mode, not visiting CR4 */
		HAL_ENABLE_FWDL(prAdapter, TRUE);

#if (CFG_MTK_WIFI_SUPPORT_IPC == 0)
		/* 4 <7> Get ECO Version */
		if (wlanSetChipEcoInfo(prAdapter) != WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "wlanSetChipEcoInfo failed!\n");
			u4Status = WLAN_STATUS_FAILURE;
			eFailReason = SET_CHIP_ECO_INFO_FAIL;
			break;
		}
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */
		/* recheck Asic capability depends on ECO version */
		wlanCheckAsicCap(prAdapter);

#if CFG_SUPPORT_CONNAC3X
		/* Copy config to infra sysram before patch download */
		if (wlanCopyPlatCfgToSysram(prAdapter, prRegInfo)
					!= WLAN_STATUS_SUCCESS) {
			DBGLOG(INIT, ERROR, "wlanCopyPlatCfgToSysram failed\n");
			u4Status = WLAN_STATUS_FAILURE;
			eFailReason = COPY_CONNSYS_CFG_FAIL;
			break;
		}
#endif

#if CFG_ENABLE_FW_DOWNLOAD
		/* 4 <8> FW/patch download */

		/* 1. disable interrupt, download is done by polling mode only
		 */
		nicDisableInterrupt(prAdapter);

		/* 2. Initialize Tx Resource to fw download state */
		nicTxInitResetResource(prAdapter);

#if CFG_WMT_RESET_API_SUPPORT
		prAdapter->fgIsSkipFWL05 = FALSE;
#endif
		u4Status = wlanDownloadFW(prAdapter);
		if (u4Status != WLAN_STATUS_SUCCESS) {
#if CFG_MTK_WIFI_DFD_DUMP_SUPPORT
			if (fgIsPreOnProcessing) {
				DBGLOG(INIT, INFO,
					"PRE_ON_PROCESS_DONE finished\n");
				eFailReason = PRE_ON_PROCESS_DONE;
				break;
			}
#endif
			eFailReason = RAM_CODE_DOWNLOAD_FAIL;
			GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_FW_DL_FAIL);
			break;
		}
#endif

#if CFG_MTK_WIFI_SW_WFDMA
		if (prSwWfdmaInfo->fgIsEnAfterFwdl) {
			if (prSwWfdmaInfo->rOps.enable)
				prSwWfdmaInfo->rOps.enable(
					prAdapter->prGlueInfo, true);
		}
#endif

#if (CFG_MTK_WIFI_SUPPORT_IPC == 0)
		DBGLOG(INIT, INFO, "Waiting for Ready bit..\n");

		/* 4 <9> check Wi-Fi FW asserts ready bit */
		u4Status = wlanCheckWifiFunc(prAdapter, TRUE);
#endif /* CFG_MTK_WIFI_SUPPORT_IPC */

		if (u4Status == WLAN_STATUS_SUCCESS) {
#if defined(_HIF_SDIO)
			uint32_t *pu4WHISR = NULL;
			uint16_t au2TxCount[SDIO_TX_RESOURCE_NUM];

			pu4WHISR = (uint32_t *)kalMemAlloc(sizeof(uint32_t),
							   PHY_MEM_TYPE);
			if (!pu4WHISR) {
				/* Every break should have a fail reason
				 * for driver clean up.
				 */
				eFailReason = RAM_CODE_DOWNLOAD_FAIL;
				DBGLOG(INIT, ERROR,
				       "Allocate pu4WHISR fail\n");
				u4Status = WLAN_STATUS_FAILURE;
				break;
			}
			/* 1. reset interrupt status */
			HAL_READ_INTR_STATUS(prAdapter, sizeof(uint32_t),
					     (uint8_t *)pu4WHISR);
			if (HAL_IS_TX_DONE_INTR(*pu4WHISR))
				HAL_READ_TX_RELEASED_COUNT(prAdapter,
							   au2TxCount);

			if (pu4WHISR)
				kalMemFree(pu4WHISR, PHY_MEM_TYPE,
					   sizeof(uint32_t));
#endif
			/* Set FW download success flag */
			prAdapter->fgIsFwDownloaded = TRUE;

#if CFG_MTK_WIFI_WFDMA_WB
			/* enable wfdma write back after fw dl */
			if (prChipInfo->enableWfdmaWb) {
				prChipInfo->enableWfdmaWb(
					prAdapter->prGlueInfo);
			}
#endif /* CFG_MTK_WIFI_WFDMA_WB */

			fw_log_start(prAdapter);

			/* 2. query & reset TX Resource for normal operation */
			wlanQueryNicResourceInformation(prAdapter);

#if (CFG_SUPPORT_NIC_CAPABILITY == 1)
			if (!bAtResetFlow) {
				/* 2.9 Workaround for Capability
				*CMD packet lost issue
				*wlanSendDummyCmd(prAdapter, TRUE);
				*/

				/* Connsys Fw log setting */
				wlanSetConnsysFwLog(prAdapter);

				/* 3. query for NIC capability */
				if (prAdapter->chip_info->isNicCapV1)
					wlanQueryNicCapability(prAdapter);

				/* 4. query for NIC capability V2 */
				u4Status = wlanQueryNicCapabilityV2(prAdapter);
				if (u4Status !=  WLAN_STATUS_SUCCESS) {
					DBGLOG(INIT, WARN,
						"wlanQueryNicCapabilityV2 failed.\n");
					RECLAIM_POWER_CONTROL_TO_PM(
						prAdapter, FALSE);
					eFailReason = WAIT_FIRMWARE_READY_FAIL;
					break;
				}
			}

			/* 5. reset TX Resource for normal operation
			 *    based on the information reported from
			 *    CMD_NicCapabilityV2
			 */
			wlanUpdateNicResourceInformation(prAdapter);

			wlanPrintVersion(prAdapter);
#endif

			/* 6. update basic configuration */
			wlanUpdateBasicConfig(prAdapter);

			if (!bAtResetFlow) {
				/* 7. Override network address */
				wlanUpdateNetworkAddress(prAdapter);

				/* 8. Apply Network Address */
				nicApplyNetworkAddress(prAdapter);
			}
		}

		if (u4Status != WLAN_STATUS_SUCCESS) {
			eFailReason = WAIT_FIRMWARE_READY_FAIL;
			break;
		}

		if (!bAtResetFlow)
			wlanOnPostFirmwareReady(prAdapter, prRegInfo);
		else {
#if CFG_SUPPORT_NVRAM
			/* load manufacture data */
			if (kalIsConfigurationExist(prAdapter->prGlueInfo)
				== TRUE)
				wlanLoadManufactureData(prAdapter, prRegInfo);
			else
				DBGLOG(INIT, WARN,
				"%s: load manufacture data fail\n", __func__);
#endif
		}

		/* restore to hardware default */
		HAL_SET_INTR_STATUS_READ_CLEAR(prAdapter);
		HAL_SET_MAILBOX_READ_CLEAR(prAdapter, FALSE);

		/* Enable interrupt */
		nicEnableInterrupt(prAdapter);

		/* init SER module */
		nicSerInit(prAdapter, bAtResetFlow);

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
		/* init thermal protection */
		if (!bAtResetFlow) {
			thrmInit(prAdapter);
		}
#endif

		RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);
	} while (FALSE);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		prAdapter->u4HifDbgFlag |= DEG_HIF_DEFAULT_DUMP;
		halPrintHifDbgInfo(prAdapter);
		DBGLOG(INIT, WARN, "Fail reason: %d\n", eFailReason);

		/* Don't do error handling in chip reset flow, leave it to
		 * coming wlanRemove for full clean
		 */
		if (!bAtResetFlow) {
			/* release allocated memory */
			switch (eFailReason) {
			case WAIT_FIRMWARE_READY_FAIL:
			case RAM_CODE_DOWNLOAD_FAIL:
			case PRE_ON_PROCESS_DONE:
			case COPY_CONNSYS_CFG_FAIL:
			case SET_CHIP_ECO_INFO_FAIL:
				fw_log_deinit(prAdapter);
				halHifSwInfoUnInit(prAdapter->prGlueInfo);
			kal_fallthrough;
			case INIT_HIFINFO_FAIL:
				nicRxUninitialize(prAdapter);
				nicTxRelease(prAdapter, FALSE);
				/* System Service Uninitialization */
				nicUninitSystemService(prAdapter);
			kal_fallthrough;
			case INIT_ADAPTER_FAIL:
			kal_fallthrough;
			case DRIVER_OWN_FAIL:
				nicReleaseAdapterMemory(prAdapter);
				break;
			case ALLOC_ADAPTER_MEM_FAIL:
			default:
				break;
			}
		}
	}
#if CFG_SUPPORT_CUSTOM_NETLINK
	glCustomGenlInit();
#endif

#if (CFG_SUPPORT_CONNAC2X == 1)
	if (prAdapter->chip_info->checkbushang)
		prAdapter->chip_info->checkbushang((void *) prAdapter, TRUE);
#endif

	return u4Status;
}				/* wlanAdapterStart */

void wlanOffClearAllQueues(struct ADAPTER *prAdapter)
{
	DBGLOG(INIT, INFO, "wlanOffClearAllQueues(): start.\n");

	/* Release all CMD/MGMT/CmdData frame in command queue */
	kalClearCommandQueue(prAdapter->prGlueInfo, TRUE);

	/* Release all CMD in pending command queue */
	wlanClearPendingCommandQueue(prAdapter);

#if CFG_SUPPORT_MULTITHREAD

	/* Flush all items in queues for multi-thread */
	wlanClearTxCommandQueue(prAdapter);

	wlanClearTxCommandDoneQueue(prAdapter);

	wlanClearDataQueue(prAdapter);

	wlanClearRxToOsQueue(prAdapter);

#endif
}

void wlanOffUninitNicModule(struct ADAPTER *prAdapter,
	const u_int8_t bAtResetFlow)
{
	DBGLOG(INIT, INFO, "wlanOffUninitNicModule(): start.\n");
	nicRxUninitialize(prAdapter);

	nicTxRelease(prAdapter, FALSE);

	if (!bAtResetFlow) {
		/* MGMT - unitialization */
		nicUninitMGMT(prAdapter);

		/* System Service Uninitialization */
		nicUninitSystemService(prAdapter);
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
		/* dynamic tx power control uninitialization */
		txPwrCtrlUninit(prAdapter);
#endif
		nicReleaseAdapterMemory(prAdapter);

#if defined(_HIF_SPI)
		/* Note: restore the SPI Mode Select from 32 bit to default */
		nicRestoreSpiDefMode(prAdapter);
#endif
	}  else {
		/* Timer Destruction */
		cnmTimerDestroy(prAdapter);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Uninitialize the adapter
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanAdapterStop(struct ADAPTER *prAdapter,
		const u_int8_t bAtResetFlow)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	wlanOffClearAllQueues(prAdapter);

	fw_log_stop(prAdapter);

	/* Hif power off wifi */
	if (prAdapter->rAcpiState == ACPI_STATE_D0 &&
		!wlanIsChipNoAck(prAdapter)
		&& !kalIsCardRemoved(prAdapter->prGlueInfo)) {
		wlanPowerOffWifi(prAdapter);
	} else {
		DBGLOG(INIT, ERROR, "Cannot WF pwr-off, release HIF TRX-res");
		HAL_CANCEL_TX_RX(prAdapter);
	}

	fw_log_deinit(prAdapter);

#if !CFG_WMT_RESET_API_SUPPORT
	/* Not free WFDMA related mem when running reset flow,
	 * in reset flow will not allocate again, just do this when rmmod.
	 */
	if (!bAtResetFlow)
		halHifSwInfoUnInit(prAdapter->prGlueInfo);
#else
	halHifSwInfoUnInit(prAdapter->prGlueInfo);
#endif
	wlanOffUninitNicModule(prAdapter, bAtResetFlow);

#if CFG_SUPPORT_CUSTOM_NETLINK
	glCustomGenlDeinit();
#endif

	fgIsMcuOff = FALSE;
	fgIsBusAccessFailed = FALSE;
#if CFG_MTK_WIFI_PCIE_SUPPORT
	fgIsPcieDataTransDisabled = FALSE;
#endif /* CFG_MTK_WIFI_PCIE_SUPPORT */
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	fgTriggerDebugSop = FALSE;
#endif

	return u4Status;
}				/* wlanAdapterStop */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called by ISR (interrupt).
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval TRUE: NIC's interrupt
 * \retval FALSE: Not NIC's interrupt
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanISR(struct ADAPTER *prAdapter,
		 u_int8_t fgGlobalIntrCtrl)
{
	ASSERT(prAdapter);

	if (fgGlobalIntrCtrl) {
		nicDisableInterrupt(prAdapter);

		/* wlanIST(prAdapter); */
	}

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called by IST (task_let).
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanIST(struct ADAPTER *prAdapter, bool fgEnInt)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	if (prAdapter->fgIsFwOwn == FALSE) {
		u4Status = nicProcessIST(prAdapter);
		if (u4Status != WLAN_STATUS_SUCCESS) {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
			GLUE_INC_REF_CNT(prAdapter->rHifStats.u4IsrNotIndCount);
#else
			DBGLOG_LIMITED(
				REQ, INFO,
				"Fail: nicProcessIST! status [%x]\n",
				u4Status);
#endif
		}
#if CFG_ENABLE_WAKE_LOCK
		if (KAL_WAKE_LOCK_ACTIVE(prAdapter,
					 prAdapter->prGlueInfo->rIntrWakeLock))
			KAL_WAKE_UNLOCK(prAdapter,
					prAdapter->prGlueInfo->rIntrWakeLock);
#endif
	}

	if (fgEnInt)
		nicEnableInterrupt(prAdapter);
}

void wlanClearPendingInterrupt(struct ADAPTER *prAdapter)
{
	uint32_t i;

	i = 0;
	while (i < CFG_IST_LOOP_COUNT
	       && nicProcessIST(prAdapter) != WLAN_STATUS_NOT_INDICATING) {
		i++;
	};
}

void wlanCheckAsicCap(struct ADAPTER *prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);

	prChipInfo = prAdapter->chip_info;

	if (prChipInfo->wlanCheckAsicCap)
		prChipInfo->wlanCheckAsicCap(prAdapter);
}

uint32_t wlanCheckWifiFunc(struct ADAPTER *prAdapter,
			   u_int8_t fgRdyChk)
{
	u_int8_t fgResult, fgTimeout;
	uint32_t u4Result = 0, u4Status, u4StartTime, u4CurTime;
	const uint32_t ready_bits =
		prAdapter->chip_info->sw_ready_bits;

	u4StartTime = kalGetTimeTick();
	fgTimeout = FALSE;

#if defined(_HIF_USB)
	if (prAdapter->prGlueInfo->rHifInfo.state ==
	    USB_STATE_LINK_DOWN)
		return WLAN_STATUS_FAILURE;
#endif

	while (TRUE) {
		DBGLOG(INIT, TRACE,
			"Check ready_bits(=0x%x)\n", ready_bits);
		if (fgRdyChk)
			HAL_WIFI_FUNC_READY_CHECK(prAdapter,
					ready_bits /* WIFI_FUNC_READY_BITS */,
					&fgResult);
		else {
			HAL_WIFI_FUNC_OFF_CHECK(prAdapter,
					ready_bits /* WIFI_FUNC_READY_BITS */,
					&fgResult);
#if defined(_HIF_USB) || defined(_HIF_SDIO)
			if (nicProcessIST(prAdapter) !=
				WLAN_STATUS_NOT_INDICATING)
				DBGLOG_LIMITED(INIT, INFO,
				       "Handle pending interrupt\n");
#endif /* _HIF_USB or _HIF_SDIO */
		}
		u4CurTime = kalGetTimeTick();

		if (CHECK_FOR_TIMEOUT(u4CurTime, u4StartTime,
				      CFG_RESPONSE_POLLING_TIMEOUT *
				      CFG_RESPONSE_POLLING_DELAY)) {

			fgTimeout = TRUE;
		}

		if (fgResult) {
			if (fgRdyChk)
				DBGLOG(INIT, INFO,
					"Ready bit asserted\n");
			else
				DBGLOG(INIT, INFO,
					"Wi-Fi power off done!\n");

			u4Status = WLAN_STATUS_SUCCESS;

			break;
		} else if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
			   || fgIsBusAccessFailed == TRUE) {
			u4Status = WLAN_STATUS_FAILURE;

			break;
		} else if (fgTimeout) {
			HAL_WIFI_FUNC_GET_STATUS(prAdapter, u4Result);
			DBGLOG(INIT, ERROR,
			       "Waiting for %s: Timeout, Status=0x%08x\n",
			       fgRdyChk ? "ready bit" : "power off", u4Result);
			GL_DEFAULT_RESET_TRIGGER(prAdapter,
						 RST_CHECK_READY_BIT_TIMEOUT);
			u4Status = WLAN_STATUS_FAILURE;
			break;
		}
		kalMsleep(CFG_RESPONSE_POLLING_DELAY);

	}

	return u4Status;
}

#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
void wlanSendIcsOffCmd(struct ADAPTER *ad, enum ENUM_MBMC_BN eBand)
{
	uint32_t u4Status;
	struct CMD_ICS_SNIFFER_INFO rIcsCmd = {0};

	rIcsCmd.ucModule = 2;
	rIcsCmd.ucAction = 0; /* turn off */
	rIcsCmd.ucCondition[0] = 2;
	rIcsCmd.ucCondition[1] = eBand;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	u4Status = wlanSendSetQueryCmdHelper(
#else
	u4Status = wlanSendSetQueryCmdAdv(
#endif
		ad, CMD_ID_SET_ICS_SNIFFER, 0,
		TRUE, FALSE, TRUE,
		nicCmdEventSetCommon, nicOidCmdTimeoutCommon,
		sizeof(struct CMD_ICS_SNIFFER_INFO),
		(uint8_t *)&rIcsCmd,
		NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);

	if (u4Status != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "Ics Off Failed ret:%u\n", u4Status);
}
#endif /* CFG_SUPPORT_ICS */

uint32_t wlanPowerOffWifi(struct ADAPTER *prAdapter)
{
	uint32_t rStatus;
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	uint8_t ucBand;

	for (ucBand = ENUM_BAND_0; ucBand < ENUM_BAND_NUM; ucBand++) {
		/* turn off ICS for each band */
		wlanSendIcsOffCmd(prAdapter, ucBand);
	}
#endif /* CFG_SUPPORT_ICS */

	/* Hif power off wifi */
	rStatus = halHifPowerOffWifi(prAdapter);
	prAdapter->fgIsCr4FwDownloaded = FALSE;

	return rStatus;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will check command queue to find out if any could be
 *	  dequeued and/or send to HIF to MT6620
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 * \param prCmdQue       Pointer of Command Queue (in Glue Layer)
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanProcessCommandQueue(struct ADAPTER *prAdapter,
				 struct QUE *prCmdQue)
{
	uint32_t rStatus;
	struct QUE rTempCmdQue, rMergeCmdQue, rStandInCmdQue;
	struct QUE *prTempCmdQue, *prMergeCmdQue, *prStandInCmdQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct CMD_INFO *prCmdInfo;
	struct MSDU_INFO *prMsduInfo;
	enum ENUM_FRAME_ACTION eFrameAction = FRAME_ACTION_DROP_PKT;
#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prCmdQue);

	prTempCmdQue = &rTempCmdQue;
	prMergeCmdQue = &rMergeCmdQue;
	prStandInCmdQue = &rStandInCmdQue;

	QUEUE_INITIALIZE(prTempCmdQue);
	QUEUE_INITIALIZE(prMergeCmdQue);
	QUEUE_INITIALIZE(prStandInCmdQue);

	/* 4 <1> Move whole list of CMD_INFO to temp queue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_QUE);

	/* 4 <2> Dequeue from head and check it is able to be sent */
	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;
#if CFG_DBG_MGT_BUF
		prMemTrack = NULL;
		if (prCmdInfo->pucInfoBuffer &&
		    !IS_FROM_BUF(prAdapter, prCmdInfo->pucInfoBuffer)) {
			prMemTrack = CONTAINER_OF(
				(uint8_t (*)[])prCmdInfo->pucInfoBuffer,
				struct MEM_TRACK, aucData);
		}

		/* 0x11 means the CmdId drop in driver */
		if (prMemTrack)
			prMemTrack->ucWhere = 0x11;
#endif
		switch (prCmdInfo->eCmdType) {
		case COMMAND_TYPE_NETWORK_IOCTL:
			/* command packet will be always sent */
			eFrameAction = FRAME_ACTION_TX_PKT;
			break;

		case COMMAND_TYPE_MANAGEMENT_FRAME:
			/* inquire with QM */
			prMsduInfo = prCmdInfo->prMsduInfo;

			eFrameAction = qmGetFrameAction(prAdapter,
						prMsduInfo->ucBssIndex,
						prMsduInfo->ucStaRecIndex,
						prMsduInfo, FRAME_TYPE_MMPDU,
						prMsduInfo->u2FrameLength);
			break;

		default:
			ASSERT(0);
			break;
		}
#if (CFG_SUPPORT_STATISTICS == 1)
		wlanWakeLogCmd(prCmdInfo->ucCID);
#endif
		/* 4 <3> handling upon dequeue result */
		if (eFrameAction == FRAME_ACTION_DROP_PKT) {
			DBGLOG(INIT, INFO,
			       "DROP CMD TYPE[%u] ID[0x%02X] SEQ[%u]\n",
			       prCmdInfo->eCmdType, prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum);
#if CFG_DBG_MGT_BUF
			/* 0x12 means the CmdId drop in driver */
			if (prMemTrack)
				prMemTrack->ucWhere = 0x12;
#endif
			wlanReleaseCommand(prAdapter, prCmdInfo,
					   TX_RESULT_DROPPED_IN_DRIVER);
			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		} else if (eFrameAction == FRAME_ACTION_QUEUE_PKT) {
			DBGLOG(INIT, TRACE,
			       "QUE back CMD TYPE[%u] ID[0x%02X] SEQ[%u]\n",
			       prCmdInfo->eCmdType, prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum);
#if CFG_DBG_MGT_BUF
			/* 0x13 means the CmdId queue back to rCmdQueue */
			if (prMemTrack)
				prMemTrack->ucWhere = 0x13;
#endif
			QUEUE_INSERT_TAIL(prMergeCmdQue, prQueueEntry);
		} else if (eFrameAction == FRAME_ACTION_TX_PKT) {
			/* 4 <4> Send the command */
#if CFG_SUPPORT_MULTITHREAD
			rStatus = wlanSendCommandMthread(prAdapter, prCmdInfo);
			/* no more TC4 resource for further
			 * transmission
			 */
			if (rStatus == WLAN_STATUS_RESOURCES) {
				/*
				 * Because 7961 cmd res is not sufficient,
				 * so will dump log here frequently, and
				 * will result system busy.
				 */
				if (prAdapter->CurNoResSeqID !=
					prCmdInfo->ucCmdSeqNum) {
					DBGLOG(INIT, WARN,
				    "NO Res CMD TYPE[%u] ID[0x%02X] SEQ[%u]\n",
				    prCmdInfo->eCmdType, prCmdInfo->ucCID,
				    prCmdInfo->ucCmdSeqNum);
				}
				prAdapter->CurNoResSeqID =
					prCmdInfo->ucCmdSeqNum;

				prAdapter->u4HifDbgFlag |= DEG_HIF_ALL;
				kalSetHifDbgEvent(prAdapter->prGlueInfo);

				QUEUE_INSERT_TAIL(prMergeCmdQue, prQueueEntry);

				/*
				 * We reserve one TC4 resource for CMD
				 * specially, only break checking the left tx
				 * request if no resource for true CMD.
				 */
				if (prCmdInfo->eCmdType !=
					COMMAND_TYPE_MANAGEMENT_FRAME)
					break;
			} else if (rStatus == WLAN_STATUS_PENDING) {
				/* Do nothing */
			} else if (rStatus == WLAN_STATUS_SUCCESS) {
				/* Do nothing */
			} else {
				struct CMD_INFO *prCmdInfo = (struct CMD_INFO *)
							     prQueueEntry;

				if (prCmdInfo->fgIsOid) {
					kalOidComplete(prAdapter->prGlueInfo,
						       prCmdInfo,
						       prCmdInfo->u4SetInfoLen,
						       rStatus);
				}
				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			}

#else
			rStatus = wlanSendCommand(prAdapter, prCmdInfo);

			if (rStatus == WLAN_STATUS_RESOURCES) {
				/* no more TC4 resource for further
				 * transmission
				 */

				DBGLOG(INIT, WARN,
				       "NO Resource for CMD TYPE[%u] ID[0x%02X] SEQ[%u]\n",
				       prCmdInfo->eCmdType, prCmdInfo->ucCID,
				       prCmdInfo->ucCmdSeqNum);

				QUEUE_INSERT_TAIL(prMergeCmdQue, prQueueEntry);
				break;
			} else if (rStatus == WLAN_STATUS_PENDING) {
				/* command packet which needs further handling
				 * upon response
				 */
				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
						      SPIN_LOCK_CMD_PENDING);
				QUEUE_INSERT_TAIL(
						&(prAdapter->rPendingCmdQueue),
						prQueueEntry);
				KAL_RELEASE_SPIN_LOCK(prAdapter,
						      SPIN_LOCK_CMD_PENDING);
			} else {
				struct CMD_INFO *prCmdInfo = (struct CMD_INFO *)
							     prQueueEntry;

				if (rStatus == WLAN_STATUS_SUCCESS) {
					if (prCmdInfo->pfCmdDoneHandler) {
						prCmdInfo->pfCmdDoneHandler(
						    prAdapter, prCmdInfo,
						    prCmdInfo->pucInfoBuffer);
					}
				} else {
					if (prCmdInfo->fgIsOid) {
						kalOidComplete(
						    prAdapter->prGlueInfo,
						    prCmdInfo,
						    prCmdInfo->u4SetInfoLen,
						    rStatus);
					}
				}

				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			}
#endif
		} else {
			ASSERT(0);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	/* 4 <3> Merge back to original queue */
	/* 4 <3.1> Merge prMergeCmdQue & prTempCmdQue */
	QUEUE_CONCATENATE_QUEUES(prMergeCmdQue, prTempCmdQue);

	/* 4 <3.2> Move prCmdQue to prStandInQue, due to prCmdQue might differ
	 *         due to incoming 802.1X frames
	 */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_QUE);
	QUEUE_MOVE_ALL(prStandInCmdQue, prCmdQue);

	/* 4 <3.3> concatenate prStandInQue to prMergeCmdQue */
	QUEUE_CONCATENATE_QUEUES(prMergeCmdQue, prStandInCmdQue);

	/* 4 <3.4> then move prMergeCmdQue to prCmdQue */
	QUEUE_MOVE_ALL(prCmdQue, prMergeCmdQue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_QUE);

#if CFG_SUPPORT_MULTITHREAD
	kalSetTxCmdEvent2Hif(prAdapter->prGlueInfo);
#endif

	return WLAN_STATUS_SUCCESS;
}				/* end of wlanProcessCommandQueue() */

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will take CMD_INFO_T which carry some information of
 *        incoming OID and notify the NIC_TX to send CMD.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 * \param prCmdInfo      Pointer of P_CMD_INFO_T
 *
 * \retval WLAN_STATUS_SUCCESS   : CMD was written to HIF and be freed(CMD Done)
 *				   immediately.
 * \retval WLAN_STATUS_RESOURCE  : No resource for current command, need to wait
 *				   for previous
 *                                 frame finishing their transmission.
 * \retval WLAN_STATUS_FAILURE   : Get failure while access HIF or been
 *				   rejected.
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSendCommand(struct ADAPTER *prAdapter,
			 struct CMD_INFO *prCmdInfo)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t ucTC;		/* "Traffic Class" SW(Driver) resource
				 * classification
				 */
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	prTxCtrl = &prAdapter->rTxCtrl;

	while (1) {
		/* <0> card removal check */
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE
		    || fgIsBusAccessFailed == TRUE) {
			rStatus = WLAN_STATUS_FAILURE;
			break;
		}

		/* <1.1> Assign Traffic Class(TC) */
		ucTC = nicTxGetCmdResourceType(prCmdInfo);

		/* <1.2> Check if pending packet or resource was exhausted */
		rStatus = nicTxAcquireResource(prAdapter, ucTC,
			       nicTxGetCmdPageCount(prAdapter, prCmdInfo),
			       TRUE);
		if (rStatus == WLAN_STATUS_RESOURCES) {
			if (nicTxPollingResource(prAdapter, ucTC) !=
			    WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, ERROR,
					"Fail to get TX resource return within timeout\n");
				rStatus = WLAN_STATUS_FAILURE;
				prAdapter->fgIsChipNoAck = TRUE;
				break;
			}
			continue;
		}

		GLUE_INC_REF_CNT(prAdapter->rHifStats.u4CmdInCount);
		/* <1.3> Forward CMD_INFO_T to NIC Layer */
		rStatus = nicTxCmd(prAdapter, prCmdInfo, ucTC);

		/* <1.4> Set Pending in response to Query Command/Need Response
		 */
		if (rStatus == WLAN_STATUS_SUCCESS) {
			if ((!prCmdInfo->fgSetQuery) || (prCmdInfo->fgNeedResp))
				rStatus = WLAN_STATUS_PENDING;
		}

		break;
	}
	return rStatus;
}				/* end of wlanSendCommand() */

#if CFG_SUPPORT_MULTITHREAD

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will take CMD_INFO_T which carry some information of
 *        incoming OID and notify the NIC_TX to send CMD.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 * \param prCmdInfo      Pointer of P_CMD_INFO_T
 *
 * \retval WLAN_STATUS_SUCCESS   : CMD was written to HIF and be freed(CMD Done)
 *				   immediately.
 * \retval WLAN_STATUS_RESOURCE  : No resource for current command, need to wait
 *				   for previous
 *                                 frame finishing their transmission.
 * \retval WLAN_STATUS_FAILURE   : Get failure while access HIF or been
 *				   rejected.
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSendCommandMthread(struct ADAPTER
				*prAdapter, struct CMD_INFO *prCmdInfo)
{
	struct TX_CTRL *prTxCtrl;
	uint8_t ucTC;		/* "Traffic Class" SW(Driver) resource
				 * classification
				 */
	uint32_t rStatus = WLAN_STATUS_SUCCESS;

	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue;

#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack = NULL;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);
	prTxCtrl = &prAdapter->rTxCtrl;

#if CFG_DBG_MGT_BUF
	if (prCmdInfo->pucInfoBuffer &&
	    !IS_FROM_BUF(prAdapter, prCmdInfo->pucInfoBuffer)) {
		prMemTrack =
			CONTAINER_OF((uint8_t (*)[])prCmdInfo->pucInfoBuffer,
				     struct MEM_TRACK, aucData);
	}
#endif

	prTempCmdQue = &rTempCmdQue;
	QUEUE_INITIALIZE(prTempCmdQue);

	do {
		/* <0> card removal check */
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == TRUE ||
		    fgIsBusAccessFailed == TRUE) {
			rStatus = WLAN_STATUS_FAILURE;
#if CFG_DBG_MGT_BUF
			/* 0x14 means the CmdId can't enqueue to TxCmdQueue
			 * due to card removal
			 */
			if (prMemTrack)
				prMemTrack->ucWhere = 0x14;
#endif
			break;
		}
		/* <1> Normal case of sending CMD Packet */
		/* <1.1> Assign Traffic Class(TC) */
		ucTC = nicTxGetCmdResourceType(prCmdInfo);

		/* <1.2> Check if pending packet or resource was exhausted */
		rStatus = nicTxAcquireResource(prAdapter, ucTC,
			       nicTxGetCmdPageCount(prAdapter, prCmdInfo),
			       TRUE);
		if (rStatus == WLAN_STATUS_RESOURCES) {
#if 0
			DBGLOG(INIT, WARN,
			       "%s: NO Resource for CMD TYPE[%u] ID[0x%02X] SEQ[%u] TC[%u]\n",
			       __func__, prCmdInfo->eCmdType, prCmdInfo->ucCID,
			       prCmdInfo->ucCmdSeqNum, ucTC);
#endif
#if CFG_DBG_MGT_BUF
			/* 0x15 means the CmdId can't enqueue to TxCmdQueue
			 * due to out of resource
			 */
			if (prMemTrack)
				prMemTrack->ucWhere = 0x15;
#endif

			break;
		}

		/* Process to pending command queue firest */
		if ((!prCmdInfo->fgSetQuery) || (prCmdInfo->fgNeedResp)) {
			/* command packet which needs further handling upon
			 * response
			 */
			/*
			 *  KAL_ACQUIRE_SPIN_LOCK(prAdapter,
			 *			  SPIN_LOCK_CMD_PENDING);
			 *  QUEUE_INSERT_TAIL(&(prAdapter->rPendingCmdQueue),
			 *		      (struct QUE_ENTRY *)prCmdInfo);
			 *  KAL_RELEASE_SPIN_LOCK(prAdapter,
			 *			  SPIN_LOCK_CMD_PENDING);
			 */
		}

#if CFG_DBG_MGT_BUF
		/* 0x20 means the CmdId is in TxCmdQueue and is waiting for
		 * main_thread handling
		 */
		if (prMemTrack)
			prMemTrack->ucWhere = 0x20;
#endif

		QUEUE_INSERT_TAIL(prTempCmdQue, prCmdInfo);

		/* <1.4> Set Pending in response to Query Command/Need Response
		 */
		if (rStatus == WLAN_STATUS_SUCCESS) {
			if (!prCmdInfo->fgSetQuery ||
			    prCmdInfo->fgNeedResp) {
				rStatus = WLAN_STATUS_PENDING;
			}
		}
	} while (FALSE);

	GLUE_ADD_REF_CNT(prTempCmdQue->u4NumElem,
			prAdapter->rHifStats.u4CmdInCount);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
	QUEUE_CONCATENATE_QUEUES(&(prAdapter->rTxCmdQueue),
				 prTempCmdQue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);

	return rStatus;
}				/* end of wlanSendCommandMthread() */

u_int8_t wlanIfCmdDbgEn(struct ADAPTER *prAdapter)
{
	u_int8_t fgCmdDbgEn;

	if (!prAdapter)
		return FALSE;

	if (IS_FEATURE_ENABLED(prAdapter->rWifiVar.ucCmdDbg))
		fgCmdDbgEn = TRUE;
	else
		fgCmdDbgEn = FALSE;

	return fgCmdDbgEn;
}

void wlanTxCmdDoneCb(struct ADAPTER *prAdapter,
		     struct CMD_INFO *prCmdInfo)
{
#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack = NULL;
#endif
#if CFG_TX_CMD_SMART_SEQUENCE
	struct MSDU_INFO *prMsduInfo = prCmdInfo->prMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();
#endif /* CFG_TX_CMD_SMART_SEQUENCE */

	/* prevent print log inside spinlock */
	if (!prCmdInfo->fgSetQuery || prCmdInfo->fgNeedResp) {
		if (wlanIfCmdDbgEn(prAdapter)) {
			DBGLOG(TX, INFO,
				"Add command: %p, %ps, cmd=0x%02X, seq=%u",
				prCmdInfo, prCmdInfo->pfCmdDoneHandler,
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		} else {
			DBGLOG(TX, INFO,
				"Add command: %p, %p, cmd=0x%02X, seq=%u",
				prCmdInfo, prCmdInfo->pfCmdDoneHandler,
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
		}
		removeDuplicatePendingCmd(prAdapter, prCmdInfo);
	}

	if (!prCmdInfo->fgSetQuery || prCmdInfo->fgNeedResp) {
#if CFG_DBG_MGT_BUF
		if (prCmdInfo->pucInfoBuffer &&
		    !IS_FROM_BUF(prAdapter, prCmdInfo->pucInfoBuffer))
			prMemTrack = CONTAINER_OF(
					(uint8_t (*)[])prCmdInfo->pucInfoBuffer,
					struct MEM_TRACK, aucData);

		/* 0x50 means the CmdId is sent to WFDMA by HIF */
		if (prMemTrack)
			prMemTrack->ucWhere = 0x50;
#endif

#if CFG_TX_CMD_SMART_SEQUENCE
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* CFG_TX_CMD_SMART_SEQUENCE */

		QUEUE_INSERT_TAIL(&prAdapter->rPendingCmdQueue, prCmdInfo);

#if CFG_TX_CMD_SMART_SEQUENCE
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
#endif /* CFG_TX_CMD_SMART_SEQUENCE */
	}

#if CFG_TX_CMD_SMART_SEQUENCE
	if (prMsduInfo && prMsduInfo->pfHifTxMsduDoneCb)
		prMsduInfo->pfHifTxMsduDoneCb(prAdapter, prMsduInfo);
#endif /* CFG_TX_CMD_SMART_SEQUENCE */
}

uint32_t wlanTxCmdMthread(struct ADAPTER *prAdapter)
{
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue;
	struct QUE rTempCmdDoneQue;
	struct QUE *prTempCmdDoneQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct CMD_INFO *prCmdInfo;
	/* P_CMD_ACCESS_REG prCmdAccessReg;
	 * P_CMD_ACCESS_REG prEventAccessReg;
	 * UINT_32 u4Address;
	 */
	uint32_t u4TxDoneQueueSize, u4Ret;
#if CFG_DBG_MGT_BUF
	struct MEM_TRACK *prMemTrack;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (halIsHifStateSuspend(prAdapter)) {
		DBGLOG(TX, ERROR, "Suspend TxCmdMthread\n");
		return WLAN_STATUS_SUCCESS;
	}

#if defined(_HIF_USB)
	if (halTxGetFreeCmdCnt(prAdapter) <= 0) {
		DBGLOG(TX, ERROR, "Waiting for HIF-resource\n");
		return WLAN_STATUS_RESOURCES;
	}
#endif

	prTempCmdQue = &rTempCmdQue;
	QUEUE_INITIALIZE(prTempCmdQue);

	prTempCmdDoneQue = &rTempCmdDoneQue;
	QUEUE_INITIALIZE(prTempCmdDoneQue);

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_CMD_CLEAR);

	/* TX Command Queue */
	/* 4 <1> Move whole list of CMD_INFO to temp queue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rTxCmdQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);

	/* 4 <2> Dequeue from head and check it is able to be sent */
	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;
		prCmdInfo->pfHifTxCmdDoneCb = wlanTxCmdDoneCb;
#if CFG_DBG_MGT_BUF
		prMemTrack = NULL;
		if (prCmdInfo->pucInfoBuffer &&
		    !IS_FROM_BUF(prAdapter, prCmdInfo->pucInfoBuffer)) {
			prMemTrack = CONTAINER_OF(
					(uint8_t (*)[])prCmdInfo->pucInfoBuffer,
					struct MEM_TRACK, aucData);
		}

		if (prMemTrack) {
			/* 0x30 means the CmdId needs to send to FW via HIF */
			/* 0x40 means the CmdId enqueues to TxCmdDone queue */
			if (!prCmdInfo->fgSetQuery || prCmdInfo->fgNeedResp)
				prMemTrack->ucWhere = 0x30;
			else
				prMemTrack->ucWhere = 0x40;
		}
#endif

		u4Ret = nicTxCmd(prAdapter, prCmdInfo, TC4_INDEX);
		if (u4Ret != WLAN_STATUS_SUCCESS)
			DBGLOG(INIT, WARN, "nicTxCmd returns error[0x%08x]\n",
			       u4Ret);

		if (u4Ret == WLAN_STATUS_RESOURCES) {
			QUEUE_INSERT_HEAD(prTempCmdQue, prQueueEntry);
			break;
		}

		if (prCmdInfo->fgSetQuery && !prCmdInfo->fgNeedResp)
			QUEUE_INSERT_TAIL(prTempCmdDoneQue, prQueueEntry);

		/* DBGLOG(INIT, INFO, "==> TX CMD QID: %d (Q:%d)\n",
		 *        prCmdInfo->ucCID, prTempCmdQue->u4NumElem));
		 */

		GLUE_DEC_REF_CNT(prAdapter->prGlueInfo->i4TxPendingCmdNum);
		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	if (!QUEUE_IS_EMPTY(prTempCmdQue)) {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
		QUEUE_CONCATENATE_QUEUES(prTempCmdQue, &prAdapter->rTxCmdQueue);
		QUEUE_CONCATENATE_QUEUES(&prAdapter->rTxCmdQueue, prTempCmdQue);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
	}

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);
	QUEUE_CONCATENATE_QUEUES(&prAdapter->rTxCmdDoneQueue,
				 prTempCmdDoneQue);
	u4TxDoneQueueSize = prAdapter->rTxCmdDoneQueue.u4NumElem;
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);

	KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_CMD_CLEAR);

	/* call tx thread to work */
	if (u4TxDoneQueueSize > 0)
		kalSetTxCmdDoneEvent(prAdapter->prGlueInfo);

	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanTxCmdDoneMthread(struct ADAPTER *prAdapter)
{
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = NULL;
	struct CMD_INFO *prCmdInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	if (halIsHifStateSuspend(prAdapter)) {
		DBGLOG(TX, WARN, "Suspend TxCmdDoneMthread\n");
		return WLAN_STATUS_SUCCESS;
	}

	prTempCmdQue = &rTempCmdQue;
	QUEUE_INITIALIZE(prTempCmdQue);

	/* 4 <1> Move whole list of CMD_INFO to temp queue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rTxCmdDoneQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);

	/* 4 <2> Dequeue from head and check it is able to be sent */
	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prCmdInfo->pucInfoBuffer);
		/* Not pending cmd, free it after TX succeed! */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in TX command queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearTxCommandQueue(struct ADAPTER *prAdapter)
{
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();
	QUEUE_INITIALIZE(prTempCmdQue);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rTxCmdQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->pfCmdTimeoutHandler)
			prCmdInfo->pfCmdTimeoutHandler(prAdapter, prCmdInfo);
		else
			wlanReleaseCommand(prAdapter, prCmdInfo,
					   TX_RESULT_QUEUE_CLEARANCE);

		/* Release Tx resource for CMD which resource is allocated but
		 * not used
		 */
		nicTxReleaseResource_PSE(prAdapter,
			nicTxGetCmdResourceType(prCmdInfo),
			nicTxGetCmdPageCount(prAdapter, prCmdInfo), TRUE);

		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear OID commands in TX command queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearTxOidCommand(struct ADAPTER *prAdapter)
{
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();
	QUEUE_INITIALIZE(prTempCmdQue);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);

	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rTxCmdQueue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);

	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->fgIsOid) {
			DBGLOG(OID, INFO,
				"Clear pending OID CMD ID[0x%02X] SEQ[%u]\n",
				prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum);
			if (prCmdInfo->pfCmdTimeoutHandler)
				prCmdInfo->pfCmdTimeoutHandler(prAdapter,
							       prCmdInfo);
			else
				wlanReleaseCommand(prAdapter, prCmdInfo,
						   TX_RESULT_QUEUE_CLEARANCE);

			/* Release Tx resource for CMD which resource is
			 * allocated but not used
			 */
			nicTxReleaseResource_PSE(prAdapter,
				nicTxGetCmdResourceType(prCmdInfo),
				nicTxGetCmdPageCount(prAdapter, prCmdInfo),
				TRUE);

			cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

		} else {
			QUEUE_INSERT_TAIL(&prAdapter->rTxCmdQueue,
					prQueueEntry);
		}

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_QUE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in TX command done queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearTxCommandDoneQueue(struct ADAPTER
				 *prAdapter)
{
	struct QUE rTempCmdDoneQue;
	struct QUE *prTempCmdDoneQue = &rTempCmdDoneQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();
	QUEUE_INITIALIZE(prTempCmdDoneQue);

	/* 4 <1> Move whole list of CMD_INFO to temp queue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);
	QUEUE_MOVE_ALL(prTempCmdDoneQue,
		       &prAdapter->rTxCmdDoneQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_CMD_DONE_QUE);

	/* 4 <2> Dequeue from head and check it is able to be sent */
	QUEUE_REMOVE_HEAD(prTempCmdDoneQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->pfCmdDoneHandler)
			prCmdInfo->pfCmdDoneHandler(prAdapter, prCmdInfo,
						    prCmdInfo->pucInfoBuffer);
		/* Not pending cmd, free it after TX succeed! */
		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

		QUEUE_REMOVE_HEAD(prTempCmdDoneQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all buffer in port 0/1 queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearDataQueue(struct ADAPTER *prAdapter)
{
	if (HAL_IS_TX_DIRECT(prAdapter))
		nicTxDirectClearHifQ(prAdapter);
	else {
		struct QUE qDataPort[MAX_BSSID_NUM][TC_NUM];
		struct QUE *prDataPort[MAX_BSSID_NUM][TC_NUM];
		struct MSDU_INFO *prMsduInfo = NULL;
		int32_t i, j;

		KAL_SPIN_LOCK_DECLARATION();
#if (CFG_TX_MGMT_BY_DATA_Q == 1)
		nicTxClearMgmtDirectTxQ(prAdapter);
#endif /* CFG_TX_MGMT_BY_DATA_Q == 1 */

		for (i = 0; i < MAX_BSSID_NUM; i++) {
			for (j = 0; j < TC_NUM; j++) {
				prDataPort[i][j] = &qDataPort[i][j];
				QUEUE_INITIALIZE(prDataPort[i][j]);
			}
		}

		/* <1> Move whole list of CMD_INFO to temp queue */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			for (j = 0; j < TC_NUM; j++) {
				QUEUE_MOVE_ALL(prDataPort[i][j],
					&prAdapter->rTxPQueue[i][j]);
				kalTraceEvent("Move TxPQueue%d_%d %d", i, j,
					prDataPort[i][j]->u4NumElem);
			}
		}
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_TX_PORT_QUE);

#if (CFG_TX_HIF_PORT_QUEUE == 1)
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			for (j = 0; j < TC_NUM; j++) {
				QUEUE_CONCATENATE_QUEUES_HEAD(prDataPort[i][j],
					&prAdapter->rTxHifPQueue[i][j]);
				kalTraceEvent("Move TxHifPQueue%d_%d %d", i, j,
					prDataPort[i][j]->u4NumElem);
			}
		}
#endif

		/* <2> Return sk buffer */
		for (i = 0; i < MAX_BSSID_NUM; i++) {
			for (j = 0; j < TC_NUM; j++) {
				if (!QUEUE_GET_HEAD(prDataPort[i][j]))
					continue;
				nicTxReleaseMsduResource(prAdapter,
					QUEUE_GET_HEAD(prDataPort[i][j]));
				nicTxFreeMsduInfoPacket(prAdapter,
					QUEUE_GET_HEAD(prDataPort[i][j]));
				nicTxReturnMsduInfo(prAdapter,
					QUEUE_GET_HEAD(prDataPort[i][j]));
			}
		}

		/* <3> Clear pending MSDU info in data done queue */
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
		while (QUEUE_IS_NOT_EMPTY(&prAdapter->rTxDataDoneQueue)) {
			QUEUE_REMOVE_HEAD(&prAdapter->rTxDataDoneQueue,
					  prMsduInfo, struct MSDU_INFO *);

			nicTxFreePacket(prAdapter, prMsduInfo, FALSE);
			nicTxReturnMsduInfo(prAdapter, prMsduInfo);
		}
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_DATA_DONE_QUE);
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all buffer in port 0/1 queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearRxToOsQueue(struct ADAPTER *prAdapter)
{
	struct QUE rTempRxQue;
	struct QUE *prTempRxQue = &rTempRxQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;

	KAL_SPIN_LOCK_DECLARATION();
	QUEUE_INITIALIZE(prTempRxQue);

	/* 4 <1> Move whole list of CMD_INFO to temp queue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);
	QUEUE_MOVE_ALL(prTempRxQue, &prAdapter->rRxQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_TO_OS_QUE);

	/* 4 <2> Remove all skbuf */
	QUEUE_REMOVE_HEAD(prTempRxQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		kalPacketFree(prAdapter->prGlueInfo,
				(void *) GLUE_GET_PKT_DESCRIPTOR(prQueueEntry));
		QUEUE_REMOVE_HEAD(prTempRxQue, prQueueEntry,
				struct QUE_ENTRY *);
	}

}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is used to clear all commands in pending command queue
 * \param prAdapter  Pointer of Adapter Data Structure
 *
 * \retval none
 */
/*----------------------------------------------------------------------------*/
void wlanClearPendingCommandQueue(struct ADAPTER *prAdapter)
{
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();
	QUEUE_INITIALIZE(prTempCmdQue);

	ASSERT(prAdapter);

	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
	QUEUE_MOVE_ALL(prTempCmdQue, &prAdapter->rPendingCmdQueue);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
			  struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;

		if (prCmdInfo->pfCmdTimeoutHandler)
			prCmdInfo->pfCmdTimeoutHandler(prAdapter, prCmdInfo);
		else
			wlanReleaseCommand(prAdapter, prCmdInfo,
					   TX_RESULT_QUEUE_CLEARANCE);

		nicTxCancelSendingCmd(prAdapter, prCmdInfo);

		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}
}

void wlanReleaseCommand(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	wlanReleaseCommandEx(prAdapter, prCmdInfo, rTxDoneStatus, TRUE);
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will release thd CMD_INFO upon its attribution
 *
 * \param prAdapter  Pointer of Adapter Data Structure
 * \param prCmdInfo  Pointer of CMD_INFO_T
 * \param rTxDoneStatus  Tx done status
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanReleaseCommandEx(struct ADAPTER *prAdapter,
			struct CMD_INFO *prCmdInfo,
			enum ENUM_TX_RESULT_CODE rTxDoneStatus,
			u_int8_t fgIsNeedHandler)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prMsduInfo;

	ASSERT(prAdapter);
	ASSERT(prCmdInfo);

	prTxCtrl = &prAdapter->rTxCtrl;

	switch (prCmdInfo->eCmdType) {
	case COMMAND_TYPE_NETWORK_IOCTL:
		DBGLOG(INIT, INFO,
		       "Free CMD: ID[0x%x] SeqNum[%u] OID[%u]\n",
		       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum,
		       prCmdInfo->fgIsOid);

		if (prCmdInfo->fgIsOid) {
			kalOidComplete(prAdapter->prGlueInfo,
				       prCmdInfo,
				       prCmdInfo->u4SetInfoLen,
				       WLAN_STATUS_FAILURE);
		}
		break;

	case COMMAND_TYPE_MANAGEMENT_FRAME:
		prMsduInfo = prCmdInfo->prMsduInfo;

		DBGLOG(INIT, INFO,
			"Free MGMT Frame: B[%u]W:P[%u:%u]TS[%u]STA[%u]RSP[%u]CS[%u]\n",
		    prMsduInfo->ucBssIndex,
		    prMsduInfo->ucWlanIndex,
		    prMsduInfo->ucPID,
		    prMsduInfo->ucTxSeqNum,
		    prMsduInfo->ucStaRecIndex,
		    prMsduInfo->pfTxDoneHandler ? TRUE : FALSE,
		    prCmdInfo->ucCmdSeqNum);

		/* invoke callbacks */
		if (fgIsNeedHandler) {
			if (prMsduInfo->pfTxDoneHandler != NULL)
				prMsduInfo->pfTxDoneHandler(prAdapter,
					prMsduInfo, rTxDoneStatus);
		} else {
			nicDumpMsduInfo(prMsduInfo);
		}

		if (prCmdInfo->eCmdType == COMMAND_TYPE_MANAGEMENT_FRAME)
			GLUE_DEC_REF_CNT(prTxCtrl->i4TxMgmtPendingNum);

		cnmMgtPktFree(prAdapter, prMsduInfo);
		break;

	default:
		ASSERT(0);
		break;
	}

}				/* end of wlanReleaseCommand() */

uint32_t wlanGetThreadWakeUp(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.u4WakeLockThreadWakeup;
}

uint32_t wlanGetTxdAppendSize(struct ADAPTER *prAdapter)
{
	return prAdapter->chip_info->txd_append_size;
}


/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will search the CMD Queue to look for the pending OID
 *	  and compelete it immediately when system request a reset.
 *
 * \param prAdapter  ointer of Adapter Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanReleasePendingOid(struct ADAPTER *prAdapter,
			   uintptr_t ulParamPtr)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;
#if (CFG_SUPPORT_DEBUG_SOP == 1)
	struct CHIP_DBG_OPS *prChipDbg = (struct CHIP_DBG_OPS *) NULL;
#endif

	KAL_SPIN_LOCK_DECLARATION();

	if (prAdapter == NULL) {
		DBGLOG(INIT, WARN, "prAdapter is NULL!\n");

		return;
	}

	do {
		if (ulParamPtr == 1)
			break;

		prAdapter->ucOidTimeoutCount++;
		if (prAdapter->ucOidTimeoutCount >=
		    WLAN_OID_NO_ACK_THRESHOLD) {
			if (!prAdapter->fgIsChipNoAck) {
				DBGLOG(INIT, WARN,
				       "No response from chip for %u times, set NoAck flag!\n",
				       prAdapter->ucOidTimeoutCount);
#if (CFG_SUPPORT_DEBUG_SOP == 1)
				prChipDbg = prAdapter->chip_info->prDebugOps;
				prChipDbg->show_debug_sop_info(prAdapter,
				  SLAVENORESP);
#endif
			}

			prAdapter->u4HifDbgFlag |= DEG_HIF_ALL;
#if CFG_MTK_MDDP_SUPPORT
			prAdapter->u4HifDbgFlag |= HIF_CHK_MD_RX_HANG;
#endif
			kalSetHifDbgEvent(prAdapter->prGlueInfo);
		}
	} while (FALSE);

	do {
#if CFG_SUPPORT_MULTITHREAD
		KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_TX_CMD_CLEAR);
#endif
		/* 1: Clear pending OID in glue layer command queue */
		kalOidCmdClearance(prAdapter->prGlueInfo);

#if CFG_SUPPORT_MULTITHREAD
		/* Clear pending OID in main_thread to hif_thread command queue
		 */
		wlanClearTxOidCommand(prAdapter);
#endif

		/* 2: Clear Pending OID in prAdapter->rPendingCmdQueue */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

		prCmdQue = &prAdapter->rPendingCmdQueue;
		QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
		while (prQueueEntry) {
			prCmdInfo = (struct CMD_INFO *) prQueueEntry;

			if (prCmdInfo->fgIsOid) {
				DBGLOG(OID, INFO,
				       "Clear pending OID CMD ID[0x%02X] SEQ[%u] buf[0x%p]\n",
				       prCmdInfo->ucCID, prCmdInfo->ucCmdSeqNum,
				       prCmdInfo->pucInfoBuffer);

				if (prCmdInfo->pfCmdTimeoutHandler) {
					prCmdInfo->pfCmdTimeoutHandler(
							prAdapter, prCmdInfo);
				} else {
					kalOidComplete(prAdapter->prGlueInfo,
						       prCmdInfo,
						       0, WLAN_STATUS_FAILURE);
				}

				KAL_RELEASE_SPIN_LOCK(prAdapter,
						      SPIN_LOCK_CMD_PENDING);
				nicTxCancelSendingCmd(prAdapter, prCmdInfo);
				KAL_ACQUIRE_SPIN_LOCK(prAdapter,
						      SPIN_LOCK_CMD_PENDING);

				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			} else {
				QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
			}

			QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
					  struct QUE_ENTRY *);
		}

		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

		/* 3: Clear pending OID queued in pvOidEntry with REQ_FLAG_OID
		 *    set
		 */
		kalOidClearance(prAdapter->prGlueInfo);

#if CFG_SUPPORT_MULTITHREAD
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_TX_CMD_CLEAR);
#endif
	} while (FALSE);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function will search the CMD Queue to look for the pending
 *	  CMD/OID for specific
 *        NETWORK TYPE and compelete it immediately when system request a reset.
 *
 * \param prAdapter  ointer of Adapter Data Structure
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanReleasePendingCMDbyBssIdx(struct ADAPTER
				   *prAdapter, uint8_t ucBssIndex)
{
#if 0
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);

	do {
		/* 1: Clear Pending OID in prAdapter->rPendingCmdQueue */
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

		prCmdQue = &prAdapter->rPendingCmdQueue;
		QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
		while (prQueueEntry) {
			prCmdInfo = (struct CMD_INFO *) prQueueEntry;

			DBGLOG(P2P, TRACE, "Pending CMD for BSS:%d\n",
			       prCmdInfo->ucBssIndex);

			if (prCmdInfo->ucBssIndex == ucBssIndex) {
				if (prCmdInfo->pfCmdTimeoutHandler) {
					prCmdInfo->pfCmdTimeoutHandler(
							prAdapter, prCmdInfo);
				} else if (prCmdInfo->fgIsOid) {
					kalOidComplete(prAdapter->prGlueInfo,
						       prCmdInfo,
						       0, WLAN_STATUS_FAILURE);
				}

				cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
			} else {
				QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
			}

			QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
					  struct QUE_ENTRY *);
		}

		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

	} while (FALSE);
#endif


}				/* wlanReleasePendingCMDbyBssIdx */

/*----------------------------------------------------------------------------*/
/*!
 * \brief Return the indicated packet buffer and reallocate one to the RFB
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
void wlanReturnPacketDelaySetup(struct ADAPTER *prAdapter)
{
	struct RX_CTRL *prRxCtrl;
	struct SW_RFB *prSwRfb = NULL;

	KAL_SPIN_LOCK_DECLARATION();
	uint32_t status = WLAN_STATUS_SUCCESS;

	ASSERT(prAdapter);

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	DBGLOG(RX, LOUD, "IndicatedRfbList num = %u\n",
		RX_GET_INDICATED_RFB_CNT(prRxCtrl));

	while (QUEUE_IS_NOT_EMPTY(&prRxCtrl->rIndicatedRfbList)) {
		KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
		QUEUE_REMOVE_HEAD(&prRxCtrl->rIndicatedRfbList, prSwRfb,
				  struct SW_RFB *);
		KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);

		if (prSwRfb) {
#if CFG_RFB_TRACK
			RX_RFB_TRACK_UPDATE(prAdapter, prSwRfb,
				RFB_TRACK_PACKET_SETUP);
#endif /* CFG_RFB_TRACK */
			status = nicRxSetupRFB(prAdapter, prSwRfb);
			nicRxReturnRFB(prAdapter, prSwRfb);
		} else {
			log_dbg(RX, WARN, "prSwRfb == NULL\n");
			break;
		}

		if (status != WLAN_STATUS_SUCCESS)
			break;
	}

#if CFG_SUPPORT_SKB_ALLOC_WORK
	if (prAdapter->ulNoMoreRfb != 0 &&
		RX_GET_FREE_RFB_CNT(prRxCtrl)) {
		DBGLOG_LIMITED(RX, INFO, "Free rfb and set IntEvent!!!!!\n");
		kalSetDrvIntEvent(prAdapter->prGlueInfo);
	}

	/* call SkbAllocWork again when there is no OOM Issue */
	if (kalSkbAllocIsNoOOM(prAdapter->prGlueInfo) &&
		RX_GET_INDICATED_RFB_CNT(prRxCtrl)) {
		kalSkbAllocWorkSchedule(prAdapter->prGlueInfo, TRUE);
		return;
	}
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

	if (status != WLAN_STATUS_SUCCESS) {
		DBGLOG(RX, WARN,
			"Restart ReturnIndicatedRfb Timer (%ums) I,F:%u,%u\n",
			RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC,
			RX_GET_INDICATED_RFB_CNT(prRxCtrl),
			RX_GET_FREE_RFB_CNT(prRxCtrl));
		/* restart timer */
		cnmTimerStartTimer(prAdapter,
			&prAdapter->rPacketDelaySetupTimer,
			RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC);
	}
}

#if (CFG_SUPPORT_RETURN_TASK == 1)
void wlanReturnPacketDelaySetupTasklet(uintptr_t data)
{
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;

	wlanReturnPacketDelaySetup(prGlueInfo->prAdapter);
}
#endif

void wlanReturnPacketDelaySetupTimeout(struct ADAPTER
				       *prAdapter, uintptr_t ulParamPtr)
{
#if CFG_SUPPORT_SKB_ALLOC_WORK
	kalSkbAllocWorkSchedule(prAdapter->prGlueInfo, TRUE);
#elif CFG_SUPPORT_RETURN_TASK
	kal_tasklet_hi_schedule(&prAdapter->prGlueInfo->rRxRfbRetTask);
#elif CFG_SUPPORT_RETURN_WORK
	kalRxRfbReturnWorkSchedule(prAdapter->prGlueInfo);
#else
	wlanReturnPacketDelaySetup(prAdapter);
#endif /* CFG_SUPPORT_RETURN_TASK */
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Return the packet buffer and reallocate one to the RFB
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 * \param pvPacket       Pointer of returned packet
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
void wlanReturnPacket(struct ADAPTER *prAdapter,
		      void *pvPacket)
{
	struct RX_CTRL *prRxCtrl;
#if !CFG_SUPPORT_RETURN_WORK
	struct SW_RFB *prSwRfb = NULL;

	KAL_SPIN_LOCK_DECLARATION();
#endif /* !CFG_SUPPORT_RETURN_WORK */

	ASSERT(prAdapter);

	prRxCtrl = &prAdapter->rRxCtrl;
	ASSERT(prRxCtrl);

	if (pvPacket) {
		kalPacketFree(prAdapter->prGlueInfo, pvPacket);
		RX_ADD_CNT(prRxCtrl, RX_DATA_RETURNED_COUNT, 1);
#if CFG_NATIVE_802_11
		if (GLUE_TEST_FLAG(prAdapter->prGlueInfo, GLUE_FLAG_HALT)) {
			/*Todo:: nothing */
			/*Todo:: nothing */
		}
#endif
	}

#if CFG_SUPPORT_RETURN_WORK
	if (QUEUE_IS_NOT_EMPTY(&prRxCtrl->rIndicatedRfbList))
		kalRxRfbReturnWorkSchedule(prAdapter->prGlueInfo);
#else /* CFG_SUPPORT_RETURN_WORK */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
	QUEUE_REMOVE_HEAD(&prRxCtrl->rIndicatedRfbList, prSwRfb,
			  struct SW_RFB *);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_RX_FREE_QUE);
	if (!prSwRfb) {
		DBGLOG(RX, WARN, "No free SwRfb!\n");
		return;
	}

	if (nicRxSetupRFB(prAdapter, prSwRfb)) {
		DBGLOG(RX, WARN,
		       "Cannot allocate packet buffer for SwRfb!\n");
		if (!timerPendingTimer(
		    &prAdapter->rPacketDelaySetupTimer)) {
			DBGLOG(RX, WARN,
			       "Start ReturnIndicatedRfb Timer (%ums) I,F:%u,%u\n",
			       RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC,
			       RX_GET_INDICATED_RFB_CNT(prRxCtrl),
			       RX_GET_FREE_RFB_CNT(prRxCtrl));
			cnmTimerStartTimer(prAdapter,
			    &prAdapter->rPacketDelaySetupTimer,
			    RX_RETURN_INDICATED_RFB_TIMEOUT_MSEC);
		}
	}
	nicRxReturnRFB(prAdapter, prSwRfb);
#endif /* CFG_SUPPORT_RETURN_WORK */
}

/*
 * Note:
 * This API can only called in main_thread,
 * please do not use this unless you actually know what you are doing.
 */
/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is a required function that allows bound protocol
 *	  drivers, or NDIS, to request changes in the state information that
 *	  the miniport maintains for particular object identifiers, such as
 *	  changes in multicast addresses.
 *
 * \param[IN] prAdapter     Pointer to the Glue info structure.
 * \param[IN] pfnOidSetHandler     Points to the OID set handlers.
 * \param[IN] pvInfoBuf     Points to a buffer containing the OID-specific data
 *			    for the set.
 * \param[IN] u4InfoBufLen  Specifies the number of bytes at prSetBuffer.
 * \param[OUT] pu4SetInfoLen Points to the number of bytes it read or is needed.
 *
 * \retval WLAN_STATUS_xxx Different WLAN_STATUS code returned by different
 *			   handlers.
 *
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanSetInformation(struct ADAPTER *prAdapter,
		   PFN_OID_HANDLER_FUNC pfnOidSetHandler,
		   void *pvInfoBuf, uint32_t u4InfoBufLen,
		   uint32_t *pu4SetInfoLen)
{
	uint32_t status;

	ASSERT(prAdapter);
	ASSERT(pu4SetInfoLen);

	/* ignore any OID request after connected, under PS current measurement
	 * mode
	 */
	DBGLOG(NIC, TRACE, "u4PsCurrentMeasureEn=%u, aisGetConnectedBssInfo=%p",
		prAdapter->u4PsCurrentMeasureEn,
		aisGetConnectedBssInfo(prAdapter));
	if (prAdapter->u4PsCurrentMeasureEn &&
	    aisGetConnectedBssInfo(prAdapter)) {
		/* note: return WLAN_STATUS_FAILURE or WLAN_STATUS_SUCCESS
		 * for blocking OIDs during current measurement ??
		 */
		return WLAN_STATUS_SUCCESS;
	}
	/* most OID handler will just queue a command packet
	 * for power state transition OIDs, handler will acquire power control
	 * by itself
	 */
	status = pfnOidSetHandler(prAdapter, pvInfoBuf,
				  u4InfoBufLen, pu4SetInfoLen);
	DBGLOG(NIC, TRACE, "%ps returns %u", pfnOidSetHandler, status);

	return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to set RX filter to Promiscuous Mode.
 *
 * \param[IN] prAdapter        Pointer to the Adapter structure.
 * \param[IN] fgEnablePromiscuousMode Enable/ disable RX Promiscuous Mode.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanSetPromiscuousMode(struct ADAPTER *prAdapter,
			    u_int8_t fgEnablePromiscuousMode)
{
	ASSERT(prAdapter);

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to set RX filter to allow to receive
 *        broadcast address packets.
 *
 * \param[IN] prAdapter        Pointer to the Adapter structure.
 * \param[IN] fgEnableBroadcast Enable/ disable broadcast packet to be received.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
void wlanRxSetBroadcast(struct ADAPTER *prAdapter,
			u_int8_t fgEnableBroadcast)
{
	ASSERT(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to send out CMD_ID_DUMMY command packet
 *
 * \param[IN] prAdapter        Pointer to the Adapter structure.
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSendDummyCmd(struct ADAPTER *prAdapter,
			  u_int8_t fgIsReqTxRsrc)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return wlanSendSetQueryCmdHelper(
#else
	return wlanSendSetQueryCmdAdv(
#endif
		prAdapter, CMD_ID_DUMMY_RSV, 0, TRUE,
		FALSE, TRUE, NULL, NULL, 0, NULL, NULL, 0,
		fgIsReqTxRsrc ? CMD_SEND_METHOD_REQ_RESOURCE :
		CMD_SEND_METHOD_TX);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to send out CMD_NIC_POWER_CTRL command packet
 *
 * \param[IN] prAdapter        Pointer to the Adapter structure.
 * \param[IN] ucPowerMode      refer to CMD/EVENT document
 *
 * \return WLAN_STATUS_SUCCESS
 * \return WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSendNicPowerCtrlCmd(struct ADAPTER
				 *prAdapter, uint8_t ucPowerMode)
{
	uint32_t status = WLAN_STATUS_SUCCESS;
	struct CMD_NIC_POWER_CTRL rNicPwrCtrl = {0};

	ASSERT(prAdapter);

	rNicPwrCtrl.ucPowerMode = ucPowerMode;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	status = wlanSendSetQueryCmdHelper(
#else
	status = wlanSendSetQueryCmdAdv(
#endif
		prAdapter, CMD_ID_NIC_POWER_CTRL, 0, TRUE,
		FALSE, TRUE, NULL, NULL,
		sizeof(struct CMD_NIC_POWER_CTRL),
		(uint8_t *)&rNicPwrCtrl, NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);

	/* 5. Add flag */
	if (ucPowerMode == 1)
		prAdapter->fgIsEnterD3ReqIssued = TRUE;

	return status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This function is called to check if it is RF test mode and
 *        the OID is allowed to be called or not
 *
 * \param[IN] prAdapter        Pointer to the Adapter structure.
 * \param[IN] fgEnableBroadcast Enable/ disable broadcast packet to be received.
 *
 * \return (none)
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanIsHandlerAllowedInRFTest(PFN_OID_HANDLER_FUNC pfnOidHandler,
				      u_int8_t fgSetInfo)
{
	PFN_OID_HANDLER_FUNC *apfnOidHandlerAllowedInRFTest;
	uint32_t i;
	uint32_t u4NumOfElem;

	if (fgSetInfo) {
		apfnOidHandlerAllowedInRFTest =
			apfnOidSetHandlerAllowedInRFTest;
		u4NumOfElem = sizeof(apfnOidSetHandlerAllowedInRFTest) /
			      sizeof(PFN_OID_HANDLER_FUNC);
	} else {
		apfnOidHandlerAllowedInRFTest =
			apfnOidQueryHandlerAllowedInRFTest;
		u4NumOfElem = sizeof(apfnOidQueryHandlerAllowedInRFTest) /
			      sizeof(PFN_OID_HANDLER_FUNC);
	}

	for (i = 0; i < u4NumOfElem; i++) {
		if (apfnOidHandlerAllowedInRFTest[i] == pfnOidHandler)
			return TRUE;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to get the chip information
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *
 * @return
 */
/*----------------------------------------------------------------------------*/

uint32_t wlanSetChipEcoInfo(struct ADAPTER *prAdapter)
{
	uint32_t hw_version = 0, sw_version = 0;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
	uint32_t chip_id = prChipInfo->chip_id;
	/* WLAN_STATUS status; */
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

#if (CFG_DIRECT_READ_CHIP_INFO == 1)
	HAL_RMCR_RD(ONOFF_READ, prAdapter, prChipInfo->top_hvr, &hw_version);
	HAL_RMCR_RD(ONOFF_READ, prAdapter, prChipInfo->top_fvr, &sw_version);

	if ((hw_version == 0) || (sw_version == 0)) {
		DBGLOG(INIT, ERROR,
		  "wlanSetChipEcoInfo can't get TOP_HVR(0x%x)/TOP_FVR(0x%x)\n",
		  hw_version, sw_version);
		u4Status = WLAN_STATUS_FAILURE;
#else
	if (wlanAccessRegister(prAdapter,
		prChipInfo->top_hvr, &hw_version, 0, 0) !=
	    WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
		       "wlanSetChipEcoInfo >> get TOP_HVR failed.\n");
		u4Status = WLAN_STATUS_FAILURE;
	} else if (wlanAccessRegister(prAdapter,
		prChipInfo->top_fvr, &sw_version, 0, 0) !=
	    WLAN_STATUS_SUCCESS) {
		DBGLOG(INIT, ERROR,
		       "wlanSetChipEcoInfo >> get TOP_FVR failed.\n");
		u4Status = WLAN_STATUS_FAILURE;
#endif
	} else {
		/* success */
		nicSetChipHwVer((uint8_t)(GET_HW_VER(hw_version) & 0xFF));
		nicSetChipFactoryVer((uint8_t)((GET_HW_VER(hw_version) >> 8) &
				     0xF));
		nicSetChipSwVer((uint8_t)GET_FW_VER(sw_version));

		/* Assign current chip version */
		prAdapter->chip_info->eco_ver = nicGetChipEcoVer(prAdapter);
	}

	DBGLOG(INIT, INFO,
	       "Chip ID[%04X] Version[E%u] HW[0x%08x] SW[0x%08x]\n",
	       chip_id, prAdapter->chip_info->eco_ver, hw_version,
	       sw_version);

	return u4Status;
}

#if (CFG_ENABLE_FW_DOWNLOAD == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to read/write a certain N9
 *        register by inband command in blocking mode in ROM code stage
 *
 * @param prAdapter      Pointer to the Adapter structure.
 *        u4DestAddr     Address of destination address
 *        u4ImgSecSize   Length of the firmware block
 *        fgReset        should be set to TRUE if this is the 1st configuration
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanAccessRegister(struct ADAPTER *prAdapter,
	uint32_t u4Addr, uint32_t *pru4Result,
	uint32_t u4Data, uint8_t ucSetQuery)
{
	struct INIT_CMD_ACCESS_REG rCmd = {0};
	void *prEvt;
	uint8_t ucEvtId;
	uint32_t u4EvtSize;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	rCmd.ucSetQuery = ucSetQuery;
	rCmd.u4Address = u4Addr;
	rCmd.u4Data = u4Data;

	if (ucSetQuery == 1) {
		ucEvtId = INIT_EVENT_ID_CMD_RESULT;
		u4EvtSize = sizeof(struct INIT_EVENT_CMD_RESULT);
	} else {
		ucEvtId = INIT_EVENT_ID_ACCESS_REG;
		u4EvtSize = sizeof(struct INIT_EVENT_ACCESS_REG);
	}

	prEvt = kalMemAlloc(u4EvtSize, VIR_MEM_TYPE);
	if (!prEvt) {
		DBGLOG(INIT, ERROR, "Alloc evt packet failed.\n");
		u4Status = WLAN_STATUS_FAILURE;
		goto exit;
	}

	u4Status = wlanSendInitSetQueryCmd(prAdapter,
		INIT_CMD_ID_ACCESS_REG, &rCmd, sizeof(rCmd),
		TRUE, FALSE,
		ucEvtId, prEvt, u4EvtSize);
	if (u4Status != WLAN_STATUS_SUCCESS)
		goto exit;

	if (ucSetQuery == 1) {
		struct INIT_EVENT_CMD_RESULT *prEvent =
			(struct INIT_EVENT_CMD_RESULT *)prEvt;

		if (prEvent->ucStatus != 0) {
			DBGLOG(INIT, ERROR, "Event status: %d\n",
				prEvent->ucStatus);
			u4Status = WLAN_STATUS_FAILURE;
			goto exit;
		}
	} else {
		struct INIT_EVENT_ACCESS_REG *prEvent =
			(struct INIT_EVENT_ACCESS_REG *)prEvt;

		if (prEvent->u4Address != u4Addr) {
			DBGLOG(INIT, ERROR,
			       "Address incorrect. 0x%08x, 0x%08x.\n",
			       u4Addr, prEvent->u4Address);
			u4Status = WLAN_STATUS_FAILURE;
			goto exit;
		}
		*pru4Result = prEvent->u4Data;
	}

exit:
	if (prEvt)
		kalMemFree(prEvt, VIR_MEM_TYPE, u4EvtSize);

	if (u4Status != WLAN_STATUS_SUCCESS)
		GL_DEFAULT_RESET_TRIGGER(prAdapter,
					 RST_ACCESS_REG_FAIL);
	return u4Status;
}
#endif /* CFG_ENABLE_FW_DOWNLOAD == 1 */

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to purge queued TX packets
 *        by indicating failure to OS and returned to free list
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *        prMsduInfoListHead Pointer to head of TX packets link list
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanProcessQueuedMsduInfo(struct ADAPTER *prAdapter,
				   struct MSDU_INFO *prMsduInfoListHead)
{
	ASSERT(prAdapter);
	ASSERT(prMsduInfoListHead);

	nicTxFreeMsduInfoPacket(prAdapter, prMsduInfoListHead);
	nicTxReturnMsduInfo(prAdapter, prMsduInfoListHead);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to check if the OID handler needs timeout
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *        pfnOidHandler      Pointer to the OID handler
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanoidTimeoutCheck(struct ADAPTER *prAdapter,
			     PFN_OID_HANDLER_FUNC pfnOidHandler)
{
	PFN_OID_HANDLER_FUNC *apfnOidHandlerWOTimeoutCheck;
	uint32_t i;
	uint32_t u4NumOfElem;
	uint32_t u4OidTimeout;

	apfnOidHandlerWOTimeoutCheck = apfnOidWOTimeoutCheck;
	u4NumOfElem = ARRAY_SIZE(apfnOidWOTimeoutCheck);

	for (i = 0; i < u4NumOfElem; i++) {
		if (apfnOidHandlerWOTimeoutCheck[i] == pfnOidHandler)
			return FALSE;
	}

	/* Decrease OID timeout threshold if chip NoAck/resetting */
	if (wlanIsChipNoAck(prAdapter)) {
		u4OidTimeout = WLAN_OID_TIMEOUT_THRESHOLD_IN_RESETTING;
		DBGLOG(INIT, INFO,
		       "Decrease OID timeout to %ums due to NoACK/CHIP-RESET\n",
		       u4OidTimeout);
	} else {
		u4OidTimeout = WLAN_OID_TIMEOUT_THRESHOLD;
	}

	/* Set OID timer for timeout check */
	cnmTimerStartTimer(prAdapter,
			   &(prAdapter->rOidTimeoutTimer), u4OidTimeout);

	return TRUE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to clear any pending OID timeout check
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanoidClearTimeoutCheck(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	cnmTimerStopTimer(prAdapter, &(prAdapter->rOidTimeoutTimer));
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to override network address
 *        if NVRAM has a valid value
 *
 * @param prAdapter          Pointer to the Adapter structure.
 *
 * @return WLAN_STATUS_FAILURE   The request could not be processed
 *         WLAN_STATUS_PENDING   The request has been queued for later
 *				 processing
 *         WLAN_STATUS_SUCCESS   The request has been processed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanUpdateNetworkAddress(struct ADAPTER
				  *prAdapter)
{
	const uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	uint8_t rMacAddr[PARAM_MAC_ADDR_LEN];
	uint32_t u4SysTime;

	ASSERT(prAdapter);

	if (kalRetrieveNetworkAddress(prAdapter->prGlueInfo, rMacAddr) == FALSE
	    || IS_BMCAST_MAC_ADDR(rMacAddr)
	    || EQUAL_MAC_ADDR(aucZeroMacAddr, rMacAddr)) {
		/* eFUSE has a valid address, don't do anything */
		if (prAdapter->fgIsEmbbededMacAddrValid == TRUE) {
#if CFG_SHOW_MACADDR_SOURCE
			DBGLOG(INIT, INFO, "Using embedded MAC address");
#endif
			return WLAN_STATUS_SUCCESS;
		}
#if CFG_SHOW_MACADDR_SOURCE
		DBGLOG(INIT, INFO,
		       "Using dynamically generated MAC address");
#endif
		/* dynamic generate */
		u4SysTime = (uint32_t) kalGetTimeTick();

		rMacAddr[0] = 0x00;
		rMacAddr[1] = 0x08;
		rMacAddr[2] = 0x22;

		kalMemCopy(&rMacAddr[3], &u4SysTime, 3);

	} else {
#if CFG_SHOW_MACADDR_SOURCE
		DBGLOG(INIT, INFO, "Using host-supplied MAC address");
#endif
	}

#if WLAN_INCLUDE_SYS
	sysMacAddrOverride(rMacAddr);
#endif

	COPY_MAC_ADDR(prAdapter->rWifiVar.aucMacAddress, rMacAddr);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to update basic configuration into firmware
 *        domain
 *
 * @param prAdapter		Pointer to the Adapter structure.
 *
 * @return WLAN_STATUS_FAILURE	The request could not be processed
 *         WLAN_STATUS_PENDING	The request has been queued for later
 *				processing
 *         WLAN_STATUS_SUCCESS	The request has been processed
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanUpdateBasicConfig(struct ADAPTER *prAdapter)
{
	struct CMD_BASIC_CONFIG rCmdBasicConfig = {0};
	struct CMD_BASIC_CONFIG *prCmdBasicConfig = &rCmdBasicConfig;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	ASSERT(prAdapter);

	prCmdBasicConfig->ucNative80211 = 0;
	prCmdBasicConfig->rCsumOffload.u2RxChecksum = 0;
	prCmdBasicConfig->rCsumOffload.u2TxChecksum = 0;
	prCmdBasicConfig->ucCtrlFlagAssertPath =
		prWifiVar->ucCtrlFlagAssertPath;
	prCmdBasicConfig->ucCtrlFlagDebugLevel =
		prWifiVar->ucCtrlFlagDebugLevel;

#if CFG_TCP_IP_CHKSUM_OFFLOAD
	if (prAdapter->fgIsSupportCsumOffload) {
		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_TCP)
			prCmdBasicConfig->rCsumOffload.u2TxChecksum |= BIT(2);

		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_UDP)
			prCmdBasicConfig->rCsumOffload.u2TxChecksum |= BIT(1);

		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_TX_IP)
			prCmdBasicConfig->rCsumOffload.u2TxChecksum |= BIT(0);

		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_RX_TCP)
			prCmdBasicConfig->rCsumOffload.u2RxChecksum |= BIT(2);

		if (prAdapter->u4CSUMFlags & CSUM_OFFLOAD_EN_RX_UDP)
			prCmdBasicConfig->rCsumOffload.u2RxChecksum |= BIT(1);

		if (prAdapter->u4CSUMFlags & (CSUM_OFFLOAD_EN_RX_IPv4 |
					      CSUM_OFFLOAD_EN_RX_IPv6))
			prCmdBasicConfig->rCsumOffload.u2RxChecksum |= BIT(0);
	}
#endif

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return wlanSendSetQueryCmdHelper(
#else
	return wlanSendSetQueryCmdAdv(
#endif
		prAdapter, CMD_ID_BASIC_CONFIG, 0, TRUE,
		FALSE, TRUE, NULL, NULL,
		sizeof(struct CMD_BASIC_CONFIG),
		(uint8_t *)prCmdBasicConfig, NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to check if the device is in RF test mode
 *
 * @param pfnOidHandler      Pointer to the OID handler
 *
 * @return TRUE
 *         FALSE
 */
/*----------------------------------------------------------------------------*/
u_int8_t wlanQueryTestMode(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	return prAdapter->fgTestMode;
}

u_int8_t wlanProcessTxFrame(struct ADAPTER *prAdapter, void *prPacket)
{
	uint32_t u4SysTime;
	uint8_t ucMacHeaderLen;
	struct TX_PACKET_INFO rTxPacketInfo;
	struct mt66xx_chip_info *prChipInfo = NULL;

	/* struct PACKET_PRIVATE_DATA*/
	_Static_assert(ENUM_PKT_FLAG_NUM <=
		       sizeof(rTxPacketInfo.u2Flag) * CHAR_BIT,
		       "Too many entries defined in ENUM_PKT_FLAG");

	ASSERT(prAdapter);
	ASSERT(prPacket);
	prChipInfo = prAdapter->chip_info;

	if (kalQoSFrameClassifierAndPacketInfo(
		    prAdapter->prGlueInfo, prPacket, &rTxPacketInfo)) {

		/* Save the value of Priority Parameter */
		GLUE_SET_PKT_TID(prPacket, rTxPacketInfo.ucPriorityParam);

		if (rTxPacketInfo.u2Flag) {
			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_1X)) {
				struct STA_RECORD *prStaRec;

				DBGLOG(RSN, TRACE, "T1X len=%d\n",
				       rTxPacketInfo.u4PacketLen);

				prStaRec = cnmGetStaRecByAddress(prAdapter,
						GLUE_GET_PKT_BSS_IDX(prPacket),
						rTxPacketInfo.aucEthDestAddr);

				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_1X);
			}

			if (rTxPacketInfo.u2Flag &
			    BIT(ENUM_PKT_NON_PROTECTED_1X))
				GLUE_SET_PKT_FLAG(prPacket,
						  ENUM_PKT_NON_PROTECTED_1X);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_802_3))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_802_3);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_VLAN_EXIST)
			    && FEAT_SUP_LLC_VLAN_TX(prChipInfo))
				GLUE_SET_PKT_FLAG(prPacket,
						  ENUM_PKT_VLAN_EXIST);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_DHCP))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_DHCP);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_ARP)) {
				struct BSS_INFO *prBssInfo;
				uint8_t ucBssIndex =
					GLUE_GET_PKT_BSS_IDX(prPacket);

				prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
					ucBssIndex);
				if (prBssInfo && prBssInfo->fgFirstArp) {
					DBGLOG(TX, INFO,
						"FirstARP from OS, stop connect protect\n");
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
					nicUniCmdSetCoexStopConnProtect(
						prAdapter, ucBssIndex);
#endif
					prBssInfo->fgFirstArp = FALSE;
				}

				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_ARP);
			}

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_ICMP))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_ICMP);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_TDLS))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_TDLS);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_DNS))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_DNS);

			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_ICMPV6))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_ICMPV6);

#if (CFG_IP_FRAG_DISABLE_HW_CHECKSUM == 1)
			if (rTxPacketInfo.u2Flag & BIT(ENUM_PKT_IP_FRAG))
				GLUE_SET_PKT_FLAG(prPacket, ENUM_PKT_IP_FRAG);
#endif
		}

		ucMacHeaderLen = ETHER_HEADER_LEN;

		/* Save the value of Header Length */
		GLUE_SET_PKT_HEADER_LEN(prPacket, ucMacHeaderLen);

		/* Save the value of Frame Length */
		GLUE_SET_PKT_FRAME_LEN(prPacket,
				       (uint16_t) rTxPacketInfo.u4PacketLen);

		/* Save the value of Arrival Time */
		u4SysTime = (OS_SYSTIME) kalGetTimeTick();
		GLUE_SET_PKT_ARRIVAL_TIME(prPacket, u4SysTime);

		return TRUE;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called before AIS is starting a new scan
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanClearScanningResult(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	u_int8_t fgKeepCurrOne = FALSE;
	uint32_t i;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;
	struct PARAM_BSSID_EX *prCurrBssid;

	ASSERT(prAdapter);
	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	prCurrBssid = aisGetCurrBssId(prAdapter, ucBssIndex);

	/* clear scanning result */
	if (kalGetMediaStateIndicated(prAdapter->prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED)
		goto clear_all;

	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {

		if (!EQUAL_MAC_ADDR(prCurrBssid->arMacAddress,
				    prScanResult[i].arMacAddress))
			continue;

		fgKeepCurrOne = TRUE;

		if (i != 0) /* copy structure */
			prScanResult[0] = prScanResult[i];

		if (prScanResult[i].u4IELength > 0) {
			if (prScanResult[i].pucIE != prWlanInfo->aucScanIEBuf) {
				/* move IEs to head */
				kalMemCopy(prWlanInfo->aucScanIEBuf,
					   prScanResult[i].pucIE,
					   prScanResult[i].u4IELength);
			}

			/* modify IE pointer */
			prScanResult[0].pucIE = prWlanInfo->aucScanIEBuf;
		} else {
			prScanResult[0].pucIE = NULL;
		}

		break;
	}

	if (fgKeepCurrOne == TRUE) {
		prWlanInfo->u4ScanResultNum = 1;
		prWlanInfo->u4ScanIEBufferUsage =
			ALIGN_4(prScanResult[0].u4IELength);
	} else {
clear_all:
		prWlanInfo->u4ScanResultNum = 0;
		prWlanInfo->u4ScanIEBufferUsage = 0;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called when AIS received a beacon timeout event
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param arBSSID        MAC address of the specified BSS
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanClearBssInScanningResult(struct ADAPTER
				  *prAdapter, uint8_t *arBSSID)
{
	uint32_t i, j, u4IELength = 0, u4IEMoveLength;
	uint8_t *pucIEPtr;
	struct WLAN_INFO *prWlanInfo;
	struct PARAM_BSSID_EX *prScanResult;

	ASSERT(prAdapter);
	prWlanInfo = &prAdapter->rWlanInfo;
	prScanResult = prWlanInfo->arScanResult;

	/* clear scanning result */
	for (i = 0; i < prWlanInfo->u4ScanResultNum; i++) {
		/* NOTE: prWlanInfo->u4ScanResultNumi could change in loop */
		if (!EQUAL_MAC_ADDR(arBSSID, prScanResult[i].arMacAddress))
			continue;

		/* backup current IE length */
		u4IELength = ALIGN_4(prScanResult[i].u4IELength);
		pucIEPtr = prScanResult[i].pucIE;

		/* removed from middle */
		for (j = i + 1; j < prWlanInfo->u4ScanResultNum; j++) {
			prScanResult[j - 1] = prScanResult[j];

			prScanResult[j - 1].pucIE = prScanResult[j].pucIE;
		}

		prWlanInfo->u4ScanResultNum--;

		/* remove IE buffer if needed := move rest of IE buffer
		 */
		if (u4IELength > 0) {
			u4IEMoveLength = prWlanInfo->u4ScanIEBufferUsage
				- (((uintptr_t) pucIEPtr)
				+ u4IELength
				- ((uintptr_t)
				(&(prWlanInfo->aucScanIEBuf[0]))));

			kalMemCopy(pucIEPtr,
				   (uint8_t *) (((uintptr_t)
				   pucIEPtr) + u4IELength),
				   u4IEMoveLength);

			prWlanInfo->u4ScanIEBufferUsage -= u4IELength;

			/* correction of pointers to IE buffer */
			for (j = 0; j < prWlanInfo->u4ScanResultNum; j++) {
				if (prScanResult[j].pucIE > pucIEPtr) {
					prScanResult[j].pucIE =
						(uint8_t *)((uintptr_t)
						    (prScanResult[j].pucIE) -
						    u4IELength);
				}
			}
		}
	}
}

#if CFG_TEST_WIFI_DIRECT_GO
void wlanEnableP2pFunction(struct ADAPTER *prAdapter)
{
#if 0
	P_MSG_P2P_FUNCTION_SWITCH_T prMsgFuncSwitch =
		(P_MSG_P2P_FUNCTION_SWITCH_T) NULL;

	prMsgFuncSwitch =
		(P_MSG_P2P_FUNCTION_SWITCH_T) cnmMemAlloc(prAdapter,
			RAM_TYPE_MSG, sizeof(MSG_P2P_FUNCTION_SWITCH_T));
	if (!prMsgFuncSwitch) {
		ASSERT(FALSE);
		return;
	}

	prMsgFuncSwitch->rMsgHdr.eMsgId = MID_MNY_P2P_FUN_SWITCH;
	prMsgFuncSwitch->fgIsFuncOn = TRUE;

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prMsgFuncSwitch, MSG_SEND_METHOD_BUF);
#endif

}

void wlanEnableATGO(struct ADAPTER *prAdapter)
{

	struct MSG_P2P_CONNECTION_REQUEST *prMsgConnReq =
		(struct MSG_P2P_CONNECTION_REQUEST *) NULL;
	uint8_t aucTargetDeviceID[MAC_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF,
						    0xFF, 0xFF };

	prMsgConnReq =
		(struct MSG_P2P_CONNECTION_REQUEST *) cnmMemAlloc(prAdapter,
		RAM_TYPE_MSG, sizeof(struct MSG_P2P_CONNECTION_REQUEST));
	if (!prMsgConnReq) {
		ASSERT(FALSE);
		return;
	}

	prMsgConnReq->rMsgHdr.eMsgId = MID_MNY_P2P_CONNECTION_REQ;

	/*=====Param Modified for test=====*/
	COPY_MAC_ADDR(prMsgConnReq->aucDeviceID, aucTargetDeviceID);
	prMsgConnReq->fgIsTobeGO = TRUE;
	prMsgConnReq->fgIsPersistentGroup = FALSE;

	/*=====Param Modified for test=====*/

	mboxSendMsg(prAdapter, MBOX_ID_0,
		    (struct MSG_HDR *) prMsgConnReq, MSG_SEND_METHOD_BUF);

}
#endif

void wlanPrintVersion(struct ADAPTER *prAdapter)
{
	uint8_t aucBuf[512];

	kalMemZero(aucBuf, 512);

#if CFG_ENABLE_FW_DOWNLOAD
	fwDlGetFwdlInfo(prAdapter, aucBuf, 512);
#endif
	DBGLOG(SW4, INFO, "%s", aucBuf);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to retrieve NIC capability from firmware
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanQueryNicCapability(struct ADAPTER
				*prAdapter)
{
	struct mt66xx_chip_info *prChipInfo;
	uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	uint32_t u4RxPktLength;
	uint8_t *aucBuffer;
	uint32_t u4EventSize;
	struct WIFI_EVENT *prEvent;
	struct EVENT_NIC_CAPABILITY *prEventNicCapability;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	wlanSendSetQueryCmdAdv(
		prAdapter, CMD_ID_GET_NIC_CAPABILITY, 0,
		FALSE, TRUE, FALSE, NULL, NULL, 0, NULL, NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);

	u4EventSize = prChipInfo->rxd_size + prChipInfo->event_hdr_size +
		sizeof(struct EVENT_NIC_CAPABILITY);
	aucBuffer = kalMemAlloc(u4EventSize, PHY_MEM_TYPE);
	if (!aucBuffer) {
		DBGLOG(INIT, ERROR, "Not enough memory for aucBuffer\n");
		return WLAN_STATUS_FAILURE;
	}

	while (TRUE) {
		if (nicRxWaitResponse(prAdapter, 1, aucBuffer, u4EventSize,
				      &u4RxPktLength) != WLAN_STATUS_SUCCESS) {

			DBGLOG(INIT, WARN, "%s: wait for event failed!\n",
			       __func__);
			kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
			return WLAN_STATUS_FAILURE;
		}
		/* header checking .. */
		if (NIC_RX_GET_U2_SW_PKT_TYPE(aucBuffer)
			!= prChipInfo->u2RxSwPktEvent) {
			DBGLOG(INIT, WARN,
				"skip unexpected Rx pkt type[0x%04x]\n",
				NIC_RX_GET_U2_SW_PKT_TYPE(aucBuffer));
			continue;
		}

		prEvent = (struct WIFI_EVENT *)
			(aucBuffer + prChipInfo->rxd_size);
		prEventNicCapability =
			(struct EVENT_NIC_CAPABILITY *)prEvent->aucBuffer;

		if (prEvent->ucEID != EVENT_ID_NIC_CAPABILITY) {
			DBGLOG(INIT, WARN,
			      "%s: skip unexpected event ID[0x%02x]\n",
			      __func__, prEvent->ucEID);
			continue;
		} else {
			break;
		}
	}

	prEventNicCapability = (struct EVENT_NIC_CAPABILITY *) (
				       prEvent->aucBuffer);

	prAdapter->rVerInfo.u2FwProductID =
		prEventNicCapability->u2ProductID;
	kalMemCopy(prAdapter->rVerInfo.aucFwBranchInfo,
		   prEventNicCapability->aucBranchInfo, 4);
	prAdapter->rVerInfo.u2FwOwnVersion =
		prEventNicCapability->u2FwVersion;
	prAdapter->rVerInfo.ucFwBuildNumber =
		prEventNicCapability->ucFwBuildNumber;
	kalMemCopy(prAdapter->rVerInfo.aucFwDateCode,
		   prEventNicCapability->aucDateCode, 16);
	prAdapter->rVerInfo.u2FwPeerVersion =
		prEventNicCapability->u2DriverVersion;
	prAdapter->fgIsHw5GBandDisabled =
			(u_int8_t)prEventNicCapability->ucHw5GBandDisabled;
	prAdapter->fgIsEepromUsed =
			(u_int8_t)prEventNicCapability->ucEepromUsed;
	prAdapter->fgIsEmbbededMacAddrValid =
			(u_int8_t)(!IS_BMCAST_MAC_ADDR(
				prEventNicCapability->aucMacAddr) &&
				!EQUAL_MAC_ADDR(aucZeroMacAddr,
				prEventNicCapability->aucMacAddr));

	COPY_MAC_ADDR(prAdapter->rWifiVar.aucPermanentAddress,
		      prEventNicCapability->aucMacAddr);
	COPY_MAC_ADDR(prAdapter->rWifiVar.aucMacAddress,
		      prEventNicCapability->aucMacAddr);

	prAdapter->rWifiVar.ucStaVht &=
				(!(prEventNicCapability->ucHwNotSupportAC));
	prAdapter->rWifiVar.ucApVht &=
				(!(prEventNicCapability->ucHwNotSupportAC));
	prAdapter->rWifiVar.ucP2pGoVht &=
				(!(prEventNicCapability->ucHwNotSupportAC));
	prAdapter->rWifiVar.ucP2pGcVht &=
				(!(prEventNicCapability->ucHwNotSupportAC));
	prAdapter->rWifiVar.ucHwNotSupportAC =
				prEventNicCapability->ucHwNotSupportAC;

	prAdapter->u4FwCompileFlag0 =
		prEventNicCapability->u4CompileFlag0;
	prAdapter->u4FwCompileFlag1 =
		prEventNicCapability->u4CompileFlag1;
	prAdapter->u4FwFeatureFlag0 =
		prEventNicCapability->u4FeatureFlag0;
	prAdapter->u4FwFeatureFlag1 =
		prEventNicCapability->u4FeatureFlag1;

	if (prEventNicCapability->ucHwSetNss1x1)
		prAdapter->rWifiVar.ucNSS = 1;

#if CFG_SUPPORT_DBDC
	if (prEventNicCapability->ucHwNotSupportDBDC)
		prAdapter->rWifiVar.eDbdcMode = ENUM_DBDC_MODE_DISABLED;
#endif
	if (prEventNicCapability->ucHwBssIdNum > 0
	    && prEventNicCapability->ucHwBssIdNum <= MAX_BSSID_NUM) {
		prAdapter->ucHwBssIdNum =
			prEventNicCapability->ucHwBssIdNum;
		prAdapter->ucP2PDevBssIdx = prAdapter->ucHwBssIdNum;
		/* v1 event does not report WmmSetNum,
		 * Assume it is the same as HwBssNum
		 */
		prAdapter->ucWmmSetNum =
			prEventNicCapability->ucHwBssIdNum;
	}

#if CFG_ENABLE_CAL_LOG
	DBGLOG(INIT, TRACE,
	       "RF CAL FAIL  = (%d),BB CAL FAIL  = (%d)\n",
	       prEventNicCapability->ucRfCalFail,
	       prEventNicCapability->ucBbCalFail);
#endif
	kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);

	return WLAN_STATUS_SUCCESS;
}

#if TXPWR_USE_PDSLOPE

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanQueryPdMcr(struct ADAPTER *prAdapter,
			struct PARAM_MCR_RW_STRUCT *prMcrRdInfo)
{
	struct mt66xx_chip_info *prChipInfo;
	uint32_t u4RxPktLength;
	uint8_t *aucBuffer;
	uint32_t u4EventSize;
	struct WIFI_EVENT *prEvent;
	struct CMD_ACCESS_REG *prCmdMcrQuery;
	uint16_t u2PacketType;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;

	u4EventSize = prChipInfo->rxd_size + prChipInfo->event_hdr_size +
		struct CMD_ACCESS_REG;
	aucBuffer = kalMemAlloc(u4EventSize, PHY_MEM_TYPE);

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	wlanSendSetQueryCmdHelper(
#else
	wlanSendSetQueryCmdAdv(
#endif
		prAdapter, CMD_ID_ACCESS_REG, 0, FALSE,
		TRUE, FALSE, NULL, nicOidCmdTimeoutCommon,
		sizeof(struct CMD_ACCESS_REG),
		(uint8_t *)prMcrRdInfo, NULL, 0,
		CMD_SEND_METHOD_REQ_RESOURCE);

	if (nicRxWaitResponse(prAdapter, 1, aucBuffer, u4EventSize,
			      &u4RxPktLength) != WLAN_STATUS_SUCCESS) {
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}
	/* header checking .. */
	u2PacketType = NIC_RX_GET_PKT_TYPE(aucBuffer);
	if (u2PacketType != RX_PKT_TYPE_SW_DEFINED) {
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}

	prEvent = (struct WIFI_EVENT *)
		(aucBuffer + prChipInfo->rxd_size);
	if (prEvent->ucEID != EVENT_ID_ACCESS_REG) {
		kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);
		return WLAN_STATUS_FAILURE;
	}

	prCmdMcrQuery = (struct CMD_ACCESS_REG *) (
				prEvent->aucBuffer);
	prMcrRdInfo->u4McrOffset = prCmdMcrQuery->u4Address;
	prMcrRdInfo->u4McrData = prCmdMcrQuery->u4Data;

	kalMemFree(aucBuffer, PHY_MEM_TYPE, u4EventSize);

	return WLAN_STATUS_SUCCESS;
}

static int32_t wlanIntRound(int32_t au4Input)
{

	if (au4Input >= 0) {
		if ((au4Input % 10) == 5) {
			au4Input = au4Input + 5;
			return au4Input;
		}
	}

	if (au4Input < 0) {
		if ((au4Input % 10) == -5) {
			au4Input = au4Input - 5;
			return au4Input;
		}
	}

	return au4Input;
}

static int32_t wlanCal6628EfuseForm(struct ADAPTER
				    *prAdapter, int32_t au4Input)
{

	struct PARAM_MCR_RW_STRUCT rMcrRdInfo;
	int32_t au4PdSlope, au4TxPwrOffset, au4TxPwrOffset_Round;
	int8_t auTxPwrOffset_Round;

	rMcrRdInfo.u4McrOffset = 0x60205c68;
	rMcrRdInfo.u4McrData = 0;
	au4TxPwrOffset = au4Input;
	wlanQueryPdMcr(prAdapter, &rMcrRdInfo);

	au4PdSlope = (rMcrRdInfo.u4McrData) & BITS(0, 6);
	au4TxPwrOffset_Round = wlanIntRound((au4TxPwrOffset *
					     au4PdSlope)) / 10;

	au4TxPwrOffset_Round = -au4TxPwrOffset_Round;

	if (au4TxPwrOffset_Round < -128)
		au4TxPwrOffset_Round = 128;
	else if (au4TxPwrOffset_Round < 0)
		au4TxPwrOffset_Round += 256;
	else if (au4TxPwrOffset_Round > 127)
		au4TxPwrOffset_Round = 127;

	auTxPwrOffset_Round = (uint8_t) au4TxPwrOffset_Round;

	return au4TxPwrOffset_Round;
}

#endif

#define NVRAM_OFS(elment) OFFSET_OF(struct WIFI_CFG_PARAM_STRUCT, elment)

uint32_t wlanGetMiniTxPower(struct ADAPTER *prAdapter,
				  enum ENUM_BAND eBand,
				  enum ENUM_PHY_MODE_TYPE ePhyMode,
				  int8_t *pTxPwr)
{
#ifdef SOC3_0
	struct REG_INFO *prRegInfo;
	struct WIFI_CFG_PARAM_STRUCT *prNvramSettings;
	uint8_t *pu1Addr;
	int8_t bandIdx = 0;

	int8_t minTxPwr = 0x7F;
	int8_t nvTxPwr = 0;
	uint32_t rateIdx = 0;
	uint32_t startOfs = 0;
	uint32_t endOfs = 0;

	struct NVRAM_FRAGMENT_RANGE arRange[2][PHY_MODE_TYPE_NUM] = {
		/*BAND2G4*/
		{
			/*CCK*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrCck1M),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrCck11M)},
			/*OFDM*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrOfdm6M),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrOfdm54M)},
			/*HT20*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrHt20Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrHt20Mcs7)},
			/*HT40*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrHt40Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrHt40Mcs32)},
			/*VHT20*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrVht20Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrVht20Mcs9)},
			/*VHT40*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrVht40Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrVht40Mcs9)},
			/*VHT80*/
			{0, 0},
			/*SU20*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU242Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU242Mcs11)},
			/*SU40*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU484Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU484Mcs11)},
			/*SU80*/
			{0, 0},
			/*RU26*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU26Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU26Mcs11)},
			/*RU52*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU52Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU52Mcs11)},
			/*RU106*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU106Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU106Mcs11)},
			/*RU242*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU242Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU242Mcs11)},
			/*RU484*/
			{NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU484Mcs0),
			NVRAM_OFS(r2G4Pwr.uc2G4TxPwrRU484Mcs11)},
			/*RU996*/
			{0, 0},
		},
		/*BAND5G*/
		{
			/*CCK*/
			{0, 0},
			/*OFDM*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrOfdm6M),
			NVRAM_OFS(r5GPwr.uc5GTxPwrOfdm54M)},
			/*HT20*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrHt20Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrHt20Mcs7)},
			/*HT40*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrHt40Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrHt40Mcs32)},
			/*VHT20*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrVht20Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrVht20Mcs9)},
			/*VHT40*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrVht40Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrVht40Mcs9)},
			/*VHT80*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrVht80Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrVht80Mcs9)},
			/*SU20*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU242Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU242Mcs11)},
			/*SU40*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU484Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU484Mcs11)},
			/*SU80*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU996Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU996Mcs11)},
			/*RU26*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU26Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU26Mcs11)},
			/*RU52*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU52Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU52Mcs11)},
			/*RU106*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU106Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU106Mcs11)},
			/*RU242*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU242Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU242Mcs11)},
			/*RU484*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU484Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU484Mcs11)},
			/*RU996*/
			{NVRAM_OFS(r5GPwr.uc5GTxPwrRU996Mcs0),
			NVRAM_OFS(r5GPwr.uc5GTxPwrRU996Mcs11)},
		}
	};

	ASSERT(prAdapter);
	prRegInfo = &prAdapter->prGlueInfo->rRegInfo;

	ASSERT(prRegInfo);
	prNvramSettings = prRegInfo->prNvramSettings;
	ASSERT(prNvramSettings);

	if (prAdapter->prGlueInfo->fgNvramAvailable == FALSE) {
		DBGLOG(INIT, WARN, "check Nvram fail\n");
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	if (eBand == BAND_2G4)
		bandIdx = 0;
	else if (eBand == BAND_5G)
		bandIdx = 1;
	else {
		DBGLOG(INIT, WARN, "check Band fail(%d)\n",
			eBand);
		return WLAN_STATUS_NOT_ACCEPTED;
	}
	if (ePhyMode >= PHY_MODE_TYPE_NUM) {
		DBGLOG(INIT, WARN, "check phy mode fail(%d)\n",
			ePhyMode);
		return WLAN_STATUS_NOT_ACCEPTED;
	}

	startOfs = arRange[(uint8_t)bandIdx][(uint8_t)ePhyMode].startOfs;
	endOfs = arRange[(uint8_t)bandIdx][(uint8_t)ePhyMode].endOfs;


	/*NVRAM start addreess :0*/
	pu1Addr = (uint8_t *)&prNvramSettings->u2Part1OwnVersion;

	for (rateIdx = startOfs; rateIdx <= endOfs; rateIdx++) {
		nvTxPwr = *(pu1Addr + rateIdx);

		if (nvTxPwr < minTxPwr)
			minTxPwr = nvTxPwr;
	}

	/*NVRAM resolution is S6.1 format*/
	*pTxPwr = minTxPwr >> 1;

	DBGLOG(INIT, INFO, "Band[%s],PhyMode[%d],get mini txpwr=%d\n",
		(eBand == BAND_2G4)?"2G4":"5G",
		ePhyMode,
		*pTxPwr);

	return WLAN_STATUS_SUCCESS;
#else
	DBGLOG(INIT, WARN, "Not supported!");
	return WLAN_STATUS_NOT_SUPPORTED;
#endif
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to load manufacture data from NVRAM
 * if available and valid
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param prRegInfo      Pointer of REG_INFO_T
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_FAILURE
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanLoadManufactureData(struct ADAPTER
				 *prAdapter, struct REG_INFO *prRegInfo)
{

#if CFG_SUPPORT_RDD_TEST_MODE
	struct CMD_RDD_CH rRddParam;
#endif
	uint8_t index = 0;
	uint8_t *pu1Addr;
	uint8_t u1TypeID;
	uint8_t u1LenLSB;
	uint8_t u1LenMSB;
	uint8_t fgSupportFragment = FALSE;
	uint32_t u4NvramStartOffset = 0, u4NvramOffset = 0;
	uint32_t u4NvramFragmentSize = 0;
	struct CMD_NVRAM_FRAGMENT *prCmdNvramFragment;
	struct WIFI_NVRAM_TAG_FORMAT *prTagDataCurr;
	struct CMD_NVRAM_SETTING *prCmdNvramSettings;

	if (prRegInfo == NULL) {
		DBGLOG(INIT, ERROR, "prRegInfo = NULL");
		return WLAN_STATUS_FAILURE;
	}
	ASSERT(prAdapter);

	fgSupportFragment = prAdapter->chip_info->is_support_nvram_fragment;


	/* 1. Version Check */
	if (prAdapter->prGlueInfo->fgNvramAvailable == TRUE) {
		prAdapter->rVerInfo.u2Part1CfgOwnVersion =
			prRegInfo->prNvramSettings->u2Part1OwnVersion;
		prAdapter->rVerInfo.u2Part1CfgPeerVersion =
			prRegInfo->prNvramSettings->u2Part1PeerVersion;
		prAdapter->rVerInfo.u2Part2CfgOwnVersion = 0;
			/* prRegInfo->prNvramSettings->u2Part2OwnVersion; */
		prAdapter->rVerInfo.u2Part2CfgPeerVersion = 0;
			/* prRegInfo->prNvramSettings->u2Part2PeerVersion; */
	}

	/* 3. Check if needs to support 5GHz */
	if (prRegInfo->ucEnable5GBand) {
		/* check if it is disabled by hardware */
		if (prAdapter->fgIsHw5GBandDisabled
		    || prRegInfo->ucSupport5GBand == 0)
			prAdapter->fgEnable5GBand = FALSE;
		else
			prAdapter->fgEnable5GBand = TRUE;
	} else
		prAdapter->fgEnable5GBand = FALSE;

	DBGLOG(INIT, INFO, "Enable5GBand = %d, Detail = [%d,%d,%d]",
		prAdapter->fgEnable5GBand,
		prRegInfo->ucEnable5GBand,
		prRegInfo->ucSupport5GBand,
		prAdapter->fgIsHw5GBandDisabled);

	/* 5. Get 16-bits Country Code and Bandwidth */
	prAdapter->rWifiVar.u2CountryCode =
		(((uint16_t) prRegInfo->au2CountryCode[0]) << 8) | (((
			uint16_t) prRegInfo->au2CountryCode[1]) & BITS(0, 7));

	/* 6. Set domain and channel information to chip
	 * Note :
	 * Skip send dynamic txpower result to FW
	 * when send NVRAM to FW during Wi-Fi on,
	 * due to config haven't been load yet.
	 * During Wi-Fi on, we will send power limit only once
	 * after load config.
	 * The result of power limit send to FW will be the minimum value
	 * of country power limit and config setting
	 */
	rlmDomainSendCmd(prAdapter, FALSE);

	/* Update supported channel list in channel table */
	wlanUpdateChannelTable(prAdapter->prGlueInfo);

	/* 9. Send the full Parameters of NVRAM to FW */

	/*NVRAM Parameter fragment, if NVRAM length is over 2048 bytes*/
	/*Driver will split NVRAM by TLV Group ,each TX CMD is 1564 bytes*/
	if (fgSupportFragment) {
		prCmdNvramFragment = (struct CMD_NVRAM_FRAGMENT *)kalMemAlloc(
					  sizeof(struct CMD_NVRAM_FRAGMENT),
					  VIR_MEM_TYPE);
		if (prCmdNvramFragment == NULL) {
			DBGLOG(INIT, ERROR,
				"prCmdNvramFragment allocate fail\n");
			return WLAN_STATUS_FAILURE;
		}

		u4NvramStartOffset = 0;
		pu1Addr = (uint8_t *)
			&prRegInfo->prNvramSettings->u2Part1OwnVersion;

		prTagDataCurr =
			(struct WIFI_NVRAM_TAG_FORMAT *)(pu1Addr
			+ OFFSET_OF(struct WIFI_CFG_PARAM_STRUCT, ucTypeID0));
		u1TypeID = prTagDataCurr->u1NvramTypeID;
		u1LenLSB = prTagDataCurr->u1NvramTypeLenLsb;
		u1LenMSB = prTagDataCurr->u1NvramTypeLenMsb;
		u4NvramOffset +=
			OFFSET_OF(struct WIFI_CFG_PARAM_STRUCT, ucTypeID0);

		DBGLOG(INIT, TRACE,
			   "NVRAM-Frag Startofs[0x%08X]ID[%d][0x%08X]Len:%d\n"
			    , u4NvramStartOffset
			    , u1TypeID
			    , u4NvramOffset
			    , (u1LenMSB << 8) | (u1LenLSB));

		while (1) {

			kalMemZero(prCmdNvramFragment,
				 sizeof(struct CMD_NVRAM_FRAGMENT));

			u4NvramOffset += NVRAM_TAG_HDR_SIZE;
			u4NvramOffset += (u1LenMSB << 8) | (u1LenLSB);
			u4NvramFragmentSize = u4NvramOffset-u4NvramStartOffset;

			DBGLOG(INIT, TRACE,
			   "NVRAM Fragement(%d)Startofs[0x%08X]ID[%d]Len:%d\n",
			   index, u4NvramStartOffset,
			   u1TypeID, u4NvramFragmentSize);

			if (u4NvramFragmentSize >
				sizeof(struct CMD_NVRAM_FRAGMENT)) {
				DBGLOG(INIT, ERROR,
				"ID[%d]copy size[%d]bigger than buf size[%d]\n",
				u1TypeID,
				u4NvramFragmentSize,
				sizeof(struct CMD_NVRAM_FRAGMENT));
				return WLAN_STATUS_FAILURE;
			}

			kalMemCopy(prCmdNvramFragment,
					   (pu1Addr + u4NvramStartOffset),
					   u4NvramFragmentSize);

			wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_NVRAM_SETTINGS,
				TRUE,
				FALSE,
				FALSE, NULL, NULL,
				sizeof(struct CMD_NVRAM_FRAGMENT),
				(uint8_t *) prCmdNvramFragment, NULL, 0);

			/*update the next Fragment group start offset*/
			u4NvramStartOffset = u4NvramOffset;
			index++;

			/*get the nex TLV format*/
			prTagDataCurr = (struct WIFI_NVRAM_TAG_FORMAT *)
				(pu1Addr + u4NvramOffset);

			u1TypeID = prTagDataCurr->u1NvramTypeID;
			u1LenLSB = prTagDataCurr->u1NvramTypeLenLsb;
			u1LenMSB = prTagDataCurr->u1NvramTypeLenMsb;
			/*sanity check*/
			if ((u1TypeID == 0) &&
				(u1LenLSB == 0) && (u1LenMSB == 0)) {
				DBGLOG(INIT, INFO,
					"TLV is Null, last index = %d\n",
					index);
				break;
			}
		}

		kalMemFree(prCmdNvramFragment, VIR_MEM_TYPE,
			 sizeof(struct CMD_NVRAM_FRAGMENT));

	} else {

		prCmdNvramSettings = (struct CMD_NVRAM_SETTING *)kalMemAlloc(
				      sizeof(struct CMD_NVRAM_SETTING),
				      VIR_MEM_TYPE);

		if (prCmdNvramSettings == NULL) {
			DBGLOG(INIT, ERROR, "can't alloc prCmdNvramSettings\n");
			return WLAN_STATUS_FAILURE;
		}

		kalMemCopy(&prCmdNvramSettings->rNvramSettings,
			   &prRegInfo->prNvramSettings->u2Part1OwnVersion,
			   sizeof(struct CMD_NVRAM_SETTING));
		ASSERT(sizeof(struct WIFI_CFG_PARAM_STRUCT) == 2048);
		wlanSendSetQueryCmd(prAdapter,
				    CMD_ID_SET_NVRAM_SETTINGS,
				    TRUE,
				    FALSE,
				    FALSE, NULL, NULL,
				    sizeof(*prCmdNvramSettings),
				    (uint8_t *) prCmdNvramSettings, NULL, 0);

		kalMemFree(prCmdNvramSettings, VIR_MEM_TYPE,
					sizeof(struct CMD_NVRAM_SETTING));
	}


	wlanNvramSetState(NVRAM_STATE_SEND_TO_FW);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to check if any pending timer has expired
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanTimerTimeoutCheck(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	cnmTimerDoTimeOutCheck(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to check if any pending mailbox message
 *        to be handled
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanProcessMboxMessage(struct ADAPTER
				*prAdapter)
{
	uint32_t i;

	ASSERT(prAdapter);

	for (i = 0; i < MBOX_ID_TOTAL_NUM; i++)
		mboxRcvAllMsg(prAdapter, (enum ENUM_MBOX_ID) i);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to enqueue a single TX packet into CORE
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *        prNativePacket Pointer of Native Packet
 *
 * @return WLAN_STATUS_SUCCESS
 *         WLAN_STATUS_RESOURCES
 *         WLAN_STATUS_INVALID_PACKET
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanEnqueueTxPacket(struct ADAPTER *prAdapter,
			     void *prNativePacket)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prMsduInfo;

	ASSERT(prAdapter);

	prTxCtrl = &prAdapter->rTxCtrl;

	prMsduInfo = cnmPktAlloc(prAdapter, 0);

	if (!prMsduInfo)
		return WLAN_STATUS_RESOURCES;

	if (nicTxFillMsduInfo(prAdapter, prMsduInfo,
			      prNativePacket)) {
		TX_INC_CNT(&prAdapter->rTxCtrl, TX_MSDUINFO_COUNT);

		/* prMsduInfo->eSrc = TX_PACKET_OS; */

		/* Tx profiling */
		wlanTxProfilingTagMsdu(prAdapter, prMsduInfo,
				       TX_PROF_TAG_DRV_ENQUE);

#if CFG_FAST_PATH_SUPPORT
		/* Check if need to send a MSCS request */
		if (mscsIsNeedRequest(prAdapter, prNativePacket)) {
			/* Request a mscs frame if needed */
			mscsRequest(prAdapter, prNativePacket, MSCS_REQUEST,
				FRAME_CLASSIFIER_TYPE_4);
		}
#endif

		/* enqueue to QM */
		nicTxEnqueueMsdu(prAdapter, prMsduInfo);

		return WLAN_STATUS_SUCCESS;
	}
	kalSendComplete(prAdapter->prGlueInfo, prNativePacket,
			WLAN_STATUS_INVALID_PACKET);

	nicTxReturnMsduInfo(prAdapter, prMsduInfo);

	return WLAN_STATUS_INVALID_PACKET;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to flush pending TX packets in CORE
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanFlushTxPendingPackets(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	return nicTxFlush(prAdapter);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief this function sends pending MSDU_INFO_T to MT6620
 *
 * @param prAdapter      Pointer to the Adapter structure.
 * @param pfgHwAccess    Pointer for tracking LP-OWN status
 *
 * @retval WLAN_STATUS_SUCCESS   Reset is done successfully.
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanTxPendingPackets(struct ADAPTER *prAdapter,
			      u_int8_t *pfgHwAccess)
{
	struct TX_CTRL *prTxCtrl;
	struct MSDU_INFO *prMsduInfo;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

#if !CFG_SUPPORT_MULTITHREAD
	ASSERT(pfgHwAccess);
#endif

	/* <1> dequeue packet by txDequeuTxPackets() */
#if CFG_SUPPORT_MULTITHREAD
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
	prMsduInfo = qmDequeueTxPacketsMthread(prAdapter,
					       &prTxCtrl->rTc);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
#else
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
	prMsduInfo = qmDequeueTxPackets(prAdapter, &prTxCtrl->rTc);
	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_QM_TX_QUEUE);
#endif
	if (prMsduInfo != NULL) {
		if (kalIsCardRemoved(prAdapter->prGlueInfo) == FALSE) {
#if !CFG_SUPPORT_MULTITHREAD
			/* <2> Acquire LP-OWN if necessary */
			if (*pfgHwAccess == FALSE) {
				*pfgHwAccess = TRUE;

				wlanAcquirePowerControl(prAdapter);
			}
#endif
			/* <3> send packets */
#if CFG_SUPPORT_MULTITHREAD
			nicTxMsduInfoListMthread(prAdapter, prMsduInfo);
#else
			nicTxMsduInfoList(prAdapter, prMsduInfo);
#endif
			/* <4> update TC by txAdjustTcQuotas() */
			nicTxAdjustTcq(prAdapter);
		} else
			wlanProcessQueuedMsduInfo(prAdapter, prMsduInfo);
	}

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to acquire power control from firmware
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanAcquirePowerControl(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	/* DBGLOG(INIT, INFO, ("Acquire Power Ctrl\n")); */

#if CFG_ENABLE_FULL_PM
	if (nicpmSetDriverOwn(prAdapter) != TRUE)
		return WLAN_STATUS_FAILURE;
#endif

	/* Reset sleepy state */
	if (prAdapter->fgWiFiInSleepyState == TRUE)
		prAdapter->fgWiFiInSleepyState = FALSE;

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to release power control to firmware
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanReleasePowerControl(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	/* DBGLOG(INIT, INFO, ("Release Power Ctrl\n")); */

	RECLAIM_POWER_CONTROL_TO_PM(prAdapter, FALSE);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is called to report currently pending TX frames count
 *        (command packets are not included)
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return number of pending TX frames
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanGetTxPendingFrameCount(struct ADAPTER *prAdapter)
{
	struct TX_CTRL *prTxCtrl;
	uint32_t u4Num;

	ASSERT(prAdapter);
	prTxCtrl = &prAdapter->rTxCtrl;

	u4Num = kalGetTxPendingFrameCount(prAdapter->prGlueInfo) +
		(uint32_t) GLUE_GET_REF_CNT(
			prTxCtrl->i4PendingFwdFrameCount);

	return u4Num;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to report current ACPI state
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return ACPI_STATE_D0 Normal Operation Mode
 *         ACPI_STATE_D3 Suspend Mode
 */
/*----------------------------------------------------------------------------*/
enum ENUM_ACPI_STATE wlanGetAcpiState(struct ADAPTER *prAdapter)
{
	ASSERT(prAdapter);

	return prAdapter->rAcpiState;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to update current ACPI state only
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param ePowerState    ACPI_STATE_D0 Normal Operation Mode
 *                       ACPI_STATE_D3 Suspend Mode
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanSetAcpiState(struct ADAPTER *prAdapter,
		      enum ENUM_ACPI_STATE ePowerState)
{
	ASSERT(prAdapter);
	ASSERT(ePowerState <= ACPI_STATE_D3);

	prAdapter->rAcpiState = ePowerState;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to query ECO version from HIFSYS CR
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return zero      Unable to retrieve ECO version information
 *         non-zero  ECO version (1-based)
 */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetEcoVersion(struct ADAPTER *prAdapter)
{
	uint8_t ucEcoVersion;

	ASSERT(prAdapter);

#if CFG_MULTI_ECOVER_SUPPORT
	ucEcoVersion = nicGetChipEcoVer(prAdapter);
	DBGLOG(INIT, TRACE, "%s: %u\n", __func__, ucEcoVersion);
	return ucEcoVersion;
#else
	if (nicVerifyChipID(prAdapter) == TRUE)
		return prAdapter->ucRevID + 1;
	else
		return 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to query ROM version from HIFSYS CR
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return zero      Unable to retrieve ROM version information
 *         non-zero  ROM version (1-based)
 */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetRomVersion(struct ADAPTER *prAdapter)
{
	uint8_t ucRomVersion;

	ASSERT(prAdapter);

	ucRomVersion = nicGetChipSwVer();
	DBGLOG(INIT, TRACE, "%s: %u\n", __func__, ucRomVersion);
	return ucRomVersion;

}

#if (CFG_TESTMODE_FWDL_SUPPORT == 1)
void set_wifi_test_mode_fwdl(const int mode)
{
	wifi_test_mode_fwdl = mode;
}

uint8_t get_wifi_test_mode_fwdl(void)
{
	return wifi_test_mode_fwdl;
}

void set_wifi_in_switch_mode(const int enabled)
{
	wifi_in_switch_mode = enabled;
}

uint8_t get_wifi_in_switch_mode(void)
{
	return wifi_in_switch_mode;
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to
 *        set preferred band configuration corresponding to network type
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param eBand          Given band
 * @param ucBssIndex     BSS Info Index
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanSetPreferBandByNetwork(struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand, uint8_t ucBssIndex)
{
	ASSERT(prAdapter);
	ASSERT(eBand <= BAND_NUM);
	if (ucBssIndex >= prAdapter->ucSwBssIdNum) {
		DBGLOG(INIT, ERROR, "BSS index %d is invalid\n", ucBssIndex);
		return;
	}

	/* 1. set prefer band according to network type */
	prAdapter->aePreferBand[ucBssIndex] = eBand;

	/* 2. remove buffered BSS descriptors correspondingly */
	if (eBand == BAND_2G4)
		scanRemoveBssDescByBandAndNetwork(prAdapter, BAND_5G,
						  ucBssIndex);
	else if (eBand == BAND_5G)
		scanRemoveBssDescByBandAndNetwork(prAdapter, BAND_2G4,
						  ucBssIndex);
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eBand == BAND_6G)
		scanRemoveBssDescByBandAndNetwork(prAdapter, BAND_6G,
						  ucBssIndex);
#endif

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to
 *        get channel information corresponding to specified network type
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param ucBssIndex     BSS Info Index
 *
 * @return channel number
 */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetChannelNumberByNetwork(struct ADAPTER
				      *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(INIT, ERROR,
			"Invalid bss idx: %d\n",
			ucBssIndex);
		return 1;
	}

	return prBssInfo->ucPrimaryChannel;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to
 *        get band information corresponding to specified network type
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 * @param ucBssIndex     BSS Info Index
 *
 * @return Band index
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanGetBandIndexByNetwork(struct ADAPTER
				      *prAdapter, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;

	ASSERT(prAdapter);
	ASSERT(ucBssIndex <= prAdapter->ucSwBssIdNum);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

	if (!prBssInfo) {
		DBGLOG(INIT, ERROR,
			"Invalid bss idx: %d\n",
			ucBssIndex);
		return BAND_2G4;
	}

	return prBssInfo->eBand;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to
 *        check unconfigured system properties and generate related message on
 *        scan list to notify users
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanCheckSystemConfiguration(struct ADAPTER *prAdapter)
{
#if (CFG_NVRAM_EXISTENCE_CHECK == 1) || (CFG_SW_NVRAM_VERSION_CHECK == 1)
	const uint8_t aucZeroMacAddr[] = NULL_MAC_ADDR;
	u_int8_t fgIsConfExist = TRUE;
	u_int8_t fgGenErrMsg = FALSE;
	struct REG_INFO *prRegInfo = NULL;
#if 0
	const uint8_t aucBCAddr[] = BC_MAC_ADDR;
	struct WLAN_BEACON_FRAME *prBeacon = NULL;
	struct IE_SSID *prSsid = NULL;
	uint32_t u4ErrCode = 0;
	uint8_t aucErrMsg[32];
	struct PARAM_SSID rSsid;
	struct PARAM_802_11_CONFIG rConfiguration;
	uint8_t rSupportedRates[PARAM_MAX_LEN_RATES_EX];
#endif
#endif

	ASSERT(prAdapter);

#if (CFG_NVRAM_EXISTENCE_CHECK == 1)
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) ==
	    FALSE) {
		fgIsConfExist = FALSE;
		fgGenErrMsg = TRUE;
	}
#endif

#if (CFG_SW_NVRAM_VERSION_CHECK == 1)
	prRegInfo = kalGetConfiguration(prAdapter->prGlueInfo);

	if (prRegInfo == NULL) {
		DBGLOG(INIT, ERROR, "prRegInfo = NULL");
		return WLAN_STATUS_FAILURE;
	}

#if (CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1)
	if (fgIsConfExist == TRUE
	    && (prAdapter->rVerInfo.u2Part1CfgPeerVersion >
		CFG_DRV_OWN_VERSION
		|| prAdapter->rVerInfo.u2Part2CfgPeerVersion >
		CFG_DRV_OWN_VERSION
#if (CFG_DRV_PEER_VERSION > 0)
		|| prAdapter->rVerInfo.u2Part1CfgOwnVersion <
		CFG_DRV_PEER_VERSION
		|| prAdapter->rVerInfo.u2Part2CfgOwnVersion <
		CFG_DRV_PEER_VERSION/* NVRAM */
		|| prAdapter->rVerInfo.u2FwOwnVersion < CFG_DRV_PEER_VERSION
#endif
		|| prAdapter->rVerInfo.u2FwPeerVersion > CFG_DRV_OWN_VERSION
		|| (prAdapter->fgIsEmbbededMacAddrValid == FALSE &&
		    (IS_BMCAST_MAC_ADDR(prRegInfo->aucMacAddr)
		     || EQUAL_MAC_ADDR(aucZeroMacAddr, prRegInfo->aucMacAddr)))
		|| prAdapter->fgIsPowerLimitTableValid == FALSE))
		fgGenErrMsg = TRUE;
#else
	if (fgIsConfExist == TRUE
	    && (prAdapter->rVerInfo.u2Part1CfgPeerVersion >
		CFG_DRV_OWN_VERSION
		|| prAdapter->rVerInfo.u2Part2CfgPeerVersion >
		CFG_DRV_OWN_VERSION
		|| prAdapter->rVerInfo.u2Part1CfgOwnVersion <
		CFG_DRV_PEER_VERSION
		|| prAdapter->rVerInfo.u2Part2CfgOwnVersion <
		CFG_DRV_PEER_VERSION/* NVRAM */
		|| prAdapter->rVerInfo.u2FwPeerVersion > CFG_DRV_OWN_VERSION
		|| prAdapter->rVerInfo.u2FwOwnVersion < CFG_DRV_PEER_VERSION
		|| (prAdapter->fgIsEmbbededMacAddrValid == FALSE &&
		    (IS_BMCAST_MAC_ADDR(prRegInfo->aucMacAddr)
		     || EQUAL_MAC_ADDR(aucZeroMacAddr, prRegInfo->aucMacAddr)))
		))
		fgGenErrMsg = TRUE;
#endif
#endif
#if 0/* remove NVRAM WARNING in scan result */
	if (fgGenErrMsg == TRUE) {
		prBeacon = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
				       sizeof(struct WLAN_BEACON_FRAME) +
				       sizeof(struct IE_SSID));

		/* initialization */
		kalMemZero(prBeacon, sizeof(struct WLAN_BEACON_FRAME) +
			   sizeof(struct IE_SSID));

		/* prBeacon initialization */
		prBeacon->u2FrameCtrl = MAC_FRAME_BEACON;
		COPY_MAC_ADDR(prBeacon->aucDestAddr, aucBCAddr);
		COPY_MAC_ADDR(prBeacon->aucSrcAddr, aucZeroMacAddr);
		COPY_MAC_ADDR(prBeacon->aucBSSID, aucZeroMacAddr);
		prBeacon->u2BeaconInterval = 100;
		prBeacon->u2CapInfo = CAP_INFO_ESS;

		/* prSSID initialization */
		prSsid = (struct IE_SSID *) (&prBeacon->aucInfoElem[0]);
		prSsid->ucId = ELEM_ID_SSID;

		/* rConfiguration initialization */
		rConfiguration.u4Length = sizeof(struct
						 PARAM_802_11_CONFIG);
		rConfiguration.u4BeaconPeriod = 100;
		rConfiguration.u4ATIMWindow = 1;
		rConfiguration.u4DSConfig = 2412;
		rConfiguration.rFHConfig.u4Length = sizeof(
				struct PARAM_802_11_CONFIG_FH);

		/* rSupportedRates initialization */
		kalMemZero(rSupportedRates,
			   (sizeof(uint8_t) * PARAM_MAX_LEN_RATES_EX));
	}
#if (CFG_NVRAM_EXISTENCE_CHECK == 1)
#define NVRAM_ERR_MSG "NVRAM WARNING: Err = 0x01"
	if (kalIsConfigurationExist(prAdapter->prGlueInfo) ==
	    FALSE) {
		COPY_SSID(prSsid->aucSSID, prSsid->ucLength, NVRAM_ERR_MSG,
			  (uint8_t) (strlen(NVRAM_ERR_MSG)));

		kalIndicateBssInfo(prAdapter->prGlueInfo,
				   (uint8_t *) prBeacon,
				   OFFSET_OF(struct WLAN_BEACON_FRAME,
					     aucInfoElem) + OFFSET_OF(
					     struct IE_SSID, aucSSID) +
					     prSsid->ucLength, 1, 0);

		COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, NVRAM_ERR_MSG,
			  strlen(NVRAM_ERR_MSG));
		nicAddScanResult(prAdapter,
				 prBeacon->aucBSSID,
				 &rSsid,
				 0,
				 0,
				 PARAM_NETWORK_TYPE_FH,
				 &rConfiguration,
				 NET_TYPE_INFRA,
				 rSupportedRates,
				 OFFSET_OF(struct WLAN_BEACON_FRAME,
					aucInfoElem) + OFFSET_OF(
					struct IE_SSID, aucSSID) +
					prSsid->ucLength -
					WLAN_MAC_MGMT_HEADER_LEN,
				 (uint8_t *) ((uintptr_t) (prBeacon) +
					WLAN_MAC_MGMT_HEADER_LEN));
	}
#endif

#if (CFG_SW_NVRAM_VERSION_CHECK == 1)
#define VER_ERR_MSG     "NVRAM WARNING: Err = 0x%02X"
	if (fgIsConfExist == TRUE) {
		if ((prAdapter->rVerInfo.u2Part1CfgPeerVersion >
		     CFG_DRV_OWN_VERSION
		     || prAdapter->rVerInfo.u2Part2CfgPeerVersion >
		     CFG_DRV_OWN_VERSION
		     || prAdapter->rVerInfo.u2Part1CfgOwnVersion <
		     CFG_DRV_PEER_VERSION
		     || prAdapter->rVerInfo.u2Part2CfgOwnVersion <
		     CFG_DRV_PEER_VERSION	/* NVRAM */
		     || prAdapter->rVerInfo.u2FwPeerVersion >
			CFG_DRV_OWN_VERSION
		     || prAdapter->rVerInfo.u2FwOwnVersion <
		     CFG_DRV_PEER_VERSION))
			u4ErrCode |= NVRAM_ERROR_VERSION_MISMATCH;

		if (prAdapter->fgIsEmbbededMacAddrValid == FALSE
		    && (IS_BMCAST_MAC_ADDR(prRegInfo->aucMacAddr)
			|| EQUAL_MAC_ADDR(aucZeroMacAddr,
					  prRegInfo->aucMacAddr))) {
			u4ErrCode |= NVRAM_ERROR_INVALID_MAC_ADDR;
		}
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
		if (prAdapter->fgIsPowerLimitTableValid == FALSE)
			u4ErrCode |= NVRAM_POWER_LIMIT_TABLE_INVALID;
#endif
		if (u4ErrCode != 0) {
			sprintf(aucErrMsg, VER_ERR_MSG,
				(unsigned int)u4ErrCode);
			COPY_SSID(prSsid->aucSSID, prSsid->ucLength, aucErrMsg,
				  (uint8_t) (strlen(aucErrMsg)));

			kalIndicateBssInfo(prAdapter->prGlueInfo,
					   (uint8_t *) prBeacon,
					   OFFSET_OF(struct WLAN_BEACON_FRAME,
						     aucInfoElem) + OFFSET_OF(
						     struct IE_SSID, aucSSID) +
						     prSsid->ucLength, 1, 0);

			COPY_SSID(rSsid.aucSsid, rSsid.u4SsidLen, NVRAM_ERR_MSG,
				  strlen(NVRAM_ERR_MSG));
			nicAddScanResult(prAdapter, prBeacon->aucBSSID, &rSsid,
					 0, 0, PARAM_NETWORK_TYPE_FH,
					 &rConfiguration, NET_TYPE_INFRA,
					 rSupportedRates,
					 OFFSET_OF(struct WLAN_BEACON_FRAME,
						aucInfoElem) +
					 OFFSET_OF(struct IE_SSID,
						aucSSID) + prSsid->ucLength -
						WLAN_MAC_MGMT_HEADER_LEN,
					 (uint8_t *) ((uintptr_t) (prBeacon)
						+ WLAN_MAC_MGMT_HEADER_LEN));
		}
	}
#endif

	if (fgGenErrMsg == TRUE)
		cnmMemFree(prAdapter, prBeacon);
#endif
	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanoidQueryBssStatistics(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer, uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen)
{
	struct PARAM_GET_BSS_STATISTICS *prQueryBssStatistics;
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint32_t rResult = WLAN_STATUS_FAILURE;
	uint8_t ucBssIndex;
	enum ENUM_WMM_ACI eAci;

	do {
		ASSERT(pvQueryBuffer);

		/* 4 1. Sanity test */
		if ((prAdapter == NULL) || (pu4QueryInfoLen == NULL))
			break;

		if ((u4QueryBufferLen) && (pvQueryBuffer == NULL))
			break;

		if (u4QueryBufferLen <
		    sizeof(struct PARAM_GET_BSS_STATISTICS *)) {
			*pu4QueryInfoLen =
				sizeof(struct PARAM_GET_BSS_STATISTICS *);
			rResult = WLAN_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		prQueryBssStatistics = (struct PARAM_GET_BSS_STATISTICS *)
				       pvQueryBuffer;
		*pu4QueryInfoLen = sizeof(struct PARAM_GET_BSS_STATISTICS);

		ucBssIndex = prQueryBssStatistics->ucBssIndex;
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);

		if (prBssInfo) {	/*AIS*/
			if (prBssInfo->eCurrentOPMode ==
			    OP_MODE_INFRASTRUCTURE) {
			struct WIFI_WMM_AC_STAT *prQueryLss = NULL;
			struct WIFI_WMM_AC_STAT *prStaLss = NULL;
			struct WIFI_WMM_AC_STAT *prBssLss = NULL;

			prQueryLss = prQueryBssStatistics->arLinkStatistics;
			prBssLss = prBssInfo->arLinkStatistics;
			prStaRec = prBssInfo->prStaRecOfAP;
			if (prStaRec) {
				prStaLss = prStaRec->arLinkStatistics;
				for (eAci = 0;
				     eAci < WMM_AC_INDEX_NUM; eAci++) {
				prQueryLss[eAci].u4TxMsdu =
					prStaLss[eAci].u4TxMsdu;
				prQueryLss[eAci].u4RxMsdu =
					prStaLss[eAci].u4RxMsdu;
				prQueryLss[eAci].u4TxDropMsdu =
					prStaLss[eAci].u4TxDropMsdu +
					prBssLss[eAci].u4TxDropMsdu;
				prQueryLss[eAci].u4TxFailMsdu =
					prStaLss[eAci].u4TxFailMsdu;
				prQueryLss[eAci].u4TxRetryMsdu =
					prStaLss[eAci].u4TxRetryMsdu;
				}
			}
			}
			rResult = WLAN_STATUS_SUCCESS;

			/*P2P */
			/* TODO */

			/*BOW*/
			/* TODO */
		}

	} while (FALSE);

	return rResult;

}

void wlanDumpBssStatistics(struct ADAPTER *prAdapter,
			   uint8_t ucBssIdx)
{
	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	enum ENUM_WMM_ACI eAci;
	struct WIFI_WMM_AC_STAT arLLStats[WMM_AC_INDEX_NUM];
	uint8_t ucIdx;

	if (ucBssIdx > prAdapter->ucSwBssIdNum) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIdx);
		return;
	}

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIdx);
	if (!prBssInfo) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIdx);
		return;
	}
	/* <1> fill per-BSS statistics */
#if 0
	/*AIS*/ if (prBssInfo->eCurrentOPMode ==
		    OP_MODE_INFRASTRUCTURE) {
		prStaRec = prBssInfo->prStaRecOfAP;
		if (prStaRec) {
			for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
				prBssInfo->arLinkStatistics[eAci].u4TxMsdu
					= prStaRec->arLinkStatistics[eAci]
					  .u4TxMsdu;
				prBssInfo->arLinkStatistics[eAci].u4RxMsdu
					= prStaRec->arLinkStatistics[eAci]
					  .u4RxMsdu;
				prBssInfo->arLinkStatistics[eAci].u4TxDropMsdu
					+= prStaRec->arLinkStatistics[eAci]
					   .u4TxDropMsdu;
				prBssInfo->arLinkStatistics[eAci].u4TxFailMsdu
					= prStaRec->arLinkStatistics[eAci]
					  .u4TxFailMsdu;
				prBssInfo->arLinkStatistics[eAci].u4TxRetryMsdu
					= prStaRec->arLinkStatistics[eAci]
					  .u4TxRetryMsdu;
			}
		}
	}
#else
	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		arLLStats[eAci].u4TxMsdu =
			prBssInfo->arLinkStatistics[eAci].u4TxMsdu;
		arLLStats[eAci].u4RxMsdu =
			prBssInfo->arLinkStatistics[eAci].u4RxMsdu;
		arLLStats[eAci].u4TxDropMsdu =
			prBssInfo->arLinkStatistics[eAci].u4TxDropMsdu;
		arLLStats[eAci].u4TxFailMsdu =
			prBssInfo->arLinkStatistics[eAci].u4TxFailMsdu;
		arLLStats[eAci].u4TxRetryMsdu =
			prBssInfo->arLinkStatistics[eAci].u4TxRetryMsdu;
	}

	for (ucIdx = 0; ucIdx < CFG_STA_REC_NUM; ucIdx++) {
		prStaRec = cnmGetStaRecByIndex(prAdapter, ucIdx);
		if (!prStaRec)
			continue;
		if (prStaRec->ucBssIndex != ucBssIdx)
			continue;
		/* now the valid sta_rec is valid */
		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
			arLLStats[eAci].u4TxMsdu +=
				prStaRec->arLinkStatistics[eAci].u4TxMsdu;
			arLLStats[eAci].u4RxMsdu +=
				prStaRec->arLinkStatistics[eAci].u4RxMsdu;
			arLLStats[eAci].u4TxDropMsdu +=
				prStaRec->arLinkStatistics[eAci].u4TxDropMsdu;
			arLLStats[eAci].u4TxFailMsdu +=
				prStaRec->arLinkStatistics[eAci].u4TxFailMsdu;
			arLLStats[eAci].u4TxRetryMsdu +=
				prStaRec->arLinkStatistics[eAci].u4TxRetryMsdu;
		}
	}
#endif

	/* <2>Dump BSS statistics */
	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		DBGLOG(SW4, INFO,
		       "LLS BSS[%u] %s: T[%06u] R[%06u] T_D[%06u] T_F[%06u]\n",
		       prBssInfo->ucBssIndex, apucACI2Str[eAci],
		       arLLStats[eAci].u4TxMsdu,
		       arLLStats[eAci].u4RxMsdu, arLLStats[eAci].u4TxDropMsdu,
		       arLLStats[eAci].u4TxFailMsdu);
	}
}

void __wlanDumpAllBssStatistics(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo;
	/* ENUM_WMM_ACI_T eAci; */
	uint32_t ucIdx;

	/* wlanUpdateAllBssStatistics(prAdapter); */

	for (ucIdx = 0; ucIdx < prAdapter->ucSwBssIdNum; ucIdx++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucIdx);
		if (prBssInfo && !IS_BSS_ACTIVE(prBssInfo)) {
			DBGLOG(SW4, TRACE,
			       "Invalid BssInfo index[%u], skip dump!\n",
			       ucIdx);
			continue;
		}

		wlanDumpBssStatistics(prAdapter, ucIdx);
	}
}

void wlanDumpAllBssStatistics(struct ADAPTER *prAdapter)
{
	struct QUE_MGT *prQM = &prAdapter->rQM;
	OS_SYSTIME rCurTime;

	/* Trigger FW stats log every 20s */
	rCurTime = (OS_SYSTIME) kalGetTimeTick();

	DBGLOG(INIT, LOUD, "CUR[%u] LAST[%u] TO[%u]\n", rCurTime,
	       prQM->rLastTxPktDumpTime,
	       CHECK_FOR_TIMEOUT(rCurTime, prQM->rLastTxPktDumpTime,
				 MSEC_TO_SYSTIME(
				 prAdapter->rWifiVar.u4StatsLogTimeout)));

	if (CHECK_FOR_TIMEOUT(rCurTime, prQM->rLastTxPktDumpTime,
			      MSEC_TO_SYSTIME(
			      prAdapter->rWifiVar.u4StatsLogTimeout))) {

		__wlanDumpAllBssStatistics(prAdapter);

		prQM->rLastTxPktDumpTime = rCurTime;
	}
}

uint32_t
wlanoidQueryStaStatistics(struct ADAPTER *prAdapter,
			  void *pvQueryBuffer,
			  uint32_t u4QueryBufferLen,
			  uint32_t *pu4QueryInfoLen)
{
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	uint8_t ucBssIndex ;

	ucBssIndex = GET_IOCTL_BSSIDX(prAdapter);
	if (ucBssIndex == aisGetDefaultLinkBssIndex(prAdapter) &&
			!CHECK_FOR_TIMEOUT(kalGetTimeTick(),
			prAdapter->u4LastLinkQuality,
			MSEC_TO_SYSTIME(SEC_TO_MSEC(CFG_LQ_MONITOR_FREQUENCY)))
	) {
		kalMemCopy((struct PARAM_GET_STA_STATISTICS *)pvQueryBuffer,
			   &prAdapter->rQueryStaStatistics,
			   sizeof(struct PARAM_GET_STA_STATISTICS));
		return WLAN_STATUS_SUCCESS;
	} else {
		uint32_t r;

		DBGLOG(REQ, TRACE, "Call, pvQueryBuffer=%p, pu4QueryInfoLen=%p",
			pvQueryBuffer, pu4QueryInfoLen);
		r = wlanQueryStaStatistics(prAdapter, pvQueryBuffer,
				u4QueryBufferLen, pu4QueryInfoLen, TRUE);
		DBGLOG(REQ, TRACE, "r=%u, pvQueryBuffer=%p, pu4QueryInfoLen=%p",
				r, pvQueryBuffer, pu4QueryInfoLen);
		return r;
	}
#else
	return wlanQueryStaStatistics(prAdapter, pvQueryBuffer,
				      u4QueryBufferLen, pu4QueryInfoLen, TRUE);
#endif
}

uint32_t
updateStaStats(struct ADAPTER *prAdapter,
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics)
{
	struct STA_RECORD *prStaRec;
	struct QUE_MGT *prQM;
	uint8_t ucIdx;
	enum ENUM_WMM_ACI eAci;

	prQM = &prAdapter->rQM;
	/* 4 5. Get driver global QM counter */
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	for (ucIdx = TC0_INDEX; ucIdx <= TC3_INDEX; ucIdx++) {
		prQueryStaStatistics->au4TcAverageQueLen[ucIdx] =
			prQM->au4AverageQueLen[ucIdx];
		prQueryStaStatistics->au4TcCurrentQueLen[ucIdx] =
			prQM->au4CurrentTcResource[ucIdx];
	}
#endif

	/* 4 2. Get StaRec by MAC address */
	prStaRec = cnmGetStaRecByAddress(prAdapter, ANY_BSS_INDEX,
					 prQueryStaStatistics->aucMacAddr);

	if (!prStaRec || !prStaRec->fgIsValid) {
		DBGLOG(NIC, WARN, "starec invalid mac[" MACSTR "]\n",
			MAC2STR(prQueryStaStatistics->aucMacAddr));
		return WLAN_STATUS_INVALID_DATA;
	}

	prQueryStaStatistics->u4Flag |= BIT(0);

#if CFG_ENABLE_PER_STA_STATISTICS
	/* 4 3. Get driver statistics */
	prQueryStaStatistics->u4TxTotalCount =
		prStaRec->u4TotalTxPktsNumber;
	prQueryStaStatistics->u4RxTotalCount =
		prStaRec->u4TotalRxPktsNumber;
	prQueryStaStatistics->u4TxExceedThresholdCount =
		prStaRec->u4ThresholdCounter;
	prQueryStaStatistics->u4TxMaxTime =
		prStaRec->u4MaxTxPktsTime;
	prQueryStaStatistics->u4TxMaxHifTime =
		prStaRec->u4MaxTxPktsHifTime;

	if (prStaRec->u4TotalTxPktsNumber) {
		prQueryStaStatistics->u4TxAverageProcessTime =
			(prStaRec->u4TotalTxPktsTime /
			 prStaRec->u4TotalTxPktsNumber);
		prQueryStaStatistics->u4TxAverageHifTime =
			prStaRec->u4TotalTxPktsHifTxTime /
			prStaRec->u4TotalTxPktsNumber;
	} else
		prQueryStaStatistics->u4TxAverageProcessTime = 0;

	/*link layer statistics */
	for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
		prQueryStaStatistics->arLinkStatistics[eAci].u4TxMsdu =
			prStaRec->arLinkStatistics[eAci].u4TxMsdu;
		prQueryStaStatistics->arLinkStatistics[eAci].u4RxMsdu =
			prStaRec->arLinkStatistics[eAci].u4RxMsdu;
		prQueryStaStatistics->arLinkStatistics[
			eAci].u4TxDropMsdu =
			prStaRec->arLinkStatistics[eAci].u4TxDropMsdu;
	}

	for (ucIdx = TC0_INDEX; ucIdx <= TC3_INDEX; ucIdx++) {
		prQueryStaStatistics->au4TcResourceEmptyCount[ucIdx] =
			prQM->au4QmTcResourceEmptyCounter[
			prStaRec->ucBssIndex][ucIdx];
		/* Reset */
		prQM->au4QmTcResourceEmptyCounter[
			prStaRec->ucBssIndex][ucIdx] = 0;
		prQueryStaStatistics->au4TcResourceBackCount[ucIdx] =
			prQM->au4QmTcResourceBackCounter[ucIdx];
		prQM->au4QmTcResourceBackCounter[ucIdx] = 0;
		prQueryStaStatistics->au4DequeueNoTcResource[ucIdx]
			= prQM->au4DequeueNoTcResourceCounter[ucIdx];
		prQM->au4DequeueNoTcResourceCounter[ucIdx] = 0;
		prQueryStaStatistics->au4TcResourceUsedPageCount[ucIdx]
			= prQM->au4QmTcUsedPageCounter[ucIdx];
		prQM->au4QmTcUsedPageCounter[ucIdx] = 0;
		prQueryStaStatistics->au4TcResourceWantedPageCount[
			ucIdx] = prQM->au4QmTcWantedPageCounter[ucIdx];
		prQM->au4QmTcWantedPageCounter[ucIdx] = 0;
	}

	prQueryStaStatistics->u4EnqueueCounter =
		prQM->u4EnqueueCounter;
	prQueryStaStatistics->u4EnqueueStaCounter =
		prStaRec->u4EnqueueCounter;

	prQueryStaStatistics->u4DequeueCounter =
		prQM->u4DequeueCounter;
	prQueryStaStatistics->u4DequeueStaCounter =
		prStaRec->u4DeqeueuCounter;

	prQueryStaStatistics->IsrCnt =
		prAdapter->prGlueInfo->IsrCnt;
	prQueryStaStatistics->IsrPassCnt =
		prAdapter->prGlueInfo->IsrPassCnt;
	prQueryStaStatistics->TaskIsrCnt =
		prAdapter->prGlueInfo->TaskIsrCnt;

	prQueryStaStatistics->IsrAbnormalCnt =
		prAdapter->prGlueInfo->IsrAbnormalCnt;
	prQueryStaStatistics->IsrSoftWareCnt =
		prAdapter->prGlueInfo->IsrSoftWareCnt;
	prQueryStaStatistics->IsrRxCnt =
		prAdapter->prGlueInfo->IsrRxCnt;
	prQueryStaStatistics->IsrTxCnt =
		prAdapter->prGlueInfo->IsrTxCnt;

	/* 4 4.1 Reset statistics */
	if (prQueryStaStatistics->ucReadClear) {
		prStaRec->u4ThresholdCounter = 0;
		prStaRec->u4TotalTxPktsNumber = 0;
		prStaRec->u4TotalTxPktsHifTxTime = 0;

		prStaRec->u4TotalTxPktsTime = 0;
		prStaRec->u4TotalRxPktsNumber = 0;
		prStaRec->u4MaxTxPktsTime = 0;
		prStaRec->u4MaxTxPktsHifTime = 0;
		prQM->u4EnqueueCounter = 0;
		prQM->u4DequeueCounter = 0;
		prStaRec->u4EnqueueCounter = 0;
		prStaRec->u4DeqeueuCounter = 0;

		prAdapter->prGlueInfo->IsrCnt = 0;
		prAdapter->prGlueInfo->IsrPassCnt = 0;
		prAdapter->prGlueInfo->TaskIsrCnt = 0;

		prAdapter->prGlueInfo->IsrAbnormalCnt = 0;
		prAdapter->prGlueInfo->IsrSoftWareCnt = 0;
		prAdapter->prGlueInfo->IsrRxCnt = 0;
		prAdapter->prGlueInfo->IsrTxCnt = 0;
	}
	/*link layer statistics */
	if (prQueryStaStatistics->ucLlsReadClear) {
		for (eAci = 0; eAci < WMM_AC_INDEX_NUM; eAci++) {
			prStaRec->arLinkStatistics[eAci].u4TxMsdu = 0;
			prStaRec->arLinkStatistics[eAci].u4RxMsdu = 0;
			prStaRec->arLinkStatistics[eAci].u4TxDropMsdu
								  = 0;
		}
	}
#endif

	for (ucIdx = TC0_INDEX; ucIdx <= TC3_INDEX; ucIdx++)
		prQueryStaStatistics->au4TcQueLen[ucIdx] =
			prStaRec->aprTargetQueue[ucIdx]->u4NumElem;

	return WLAN_STATUS_SUCCESS;
}

uint32_t
wlanQueryStaStatistics(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer,
		       uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen,
		       u_int8_t fgIsOid)
{
	uint32_t rResult = WLAN_STATUS_FAILURE;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;
	struct CMD_GET_STA_STATISTICS rQueryCmdStaStatistics = {0};

	if (prAdapter == NULL)
		return WLAN_STATUS_FAILURE;

	if (prAdapter->fgIsEnableLpdvt)
		return WLAN_STATUS_NOT_SUPPORTED;

	do {
		struct STA_RECORD *prStaRec;

		ASSERT(pvQueryBuffer);

		/* 4 1. Sanity test */
		if (pu4QueryInfoLen == NULL)
			break;

		if ((u4QueryBufferLen) && (pvQueryBuffer == NULL))
			break;

		if (u4QueryBufferLen <
		    sizeof(struct PARAM_GET_STA_STATISTICS)) {
			*pu4QueryInfoLen =
					sizeof(struct PARAM_GET_STA_STATISTICS);
			rResult = WLAN_STATUS_BUFFER_TOO_SHORT;
			break;
		}

		prQueryStaStatistics = (struct PARAM_GET_STA_STATISTICS *)
				       pvQueryBuffer;
		*pu4QueryInfoLen = sizeof(struct PARAM_GET_STA_STATISTICS);

		rResult = updateStaStats(prAdapter, prQueryStaStatistics);
		if (rResult != WLAN_STATUS_SUCCESS)
			break;

		prStaRec = cnmGetStaRecByAddress(prAdapter, ANY_BSS_INDEX,
					prQueryStaStatistics->aucMacAddr);

		if (!prStaRec || !prStaRec->fgIsValid)
			return WLAN_STATUS_INVALID_DATA;

		/* 4 6. Ensure FW supports get station link status */
		rQueryCmdStaStatistics.ucIndex = prStaRec->ucIndex;
		COPY_MAC_ADDR(rQueryCmdStaStatistics.aucMacAddr,
			      prQueryStaStatistics->aucMacAddr);
		rQueryCmdStaStatistics.ucLlsReadClear =
			prQueryStaStatistics->ucLlsReadClear;
		rQueryCmdStaStatistics.ucResetCounter =
			prQueryStaStatistics->ucResetCounter;

		DBGLOG(REQ, TRACE, "Call fgIsOid=%u, pvQueryBuffer=%p",
				fgIsOid, pvQueryBuffer);
		rResult = wlanSendSetQueryCmd(prAdapter,
				      CMD_ID_GET_STA_STATISTICS,
				      FALSE,
				      TRUE,
				      fgIsOid,
				      nicCmdEventQueryStaStatistics,
				      nicOidCmdTimeoutCommon,
				      sizeof(struct CMD_GET_STA_STATISTICS),
				      (uint8_t *)&rQueryCmdStaStatistics,
				      pvQueryBuffer, u4QueryBufferLen);
		DBGLOG(REQ, TRACE, "rResult=%u, fgIsOid=%u, pvQueryBuffer=%p",
				rResult, fgIsOid, pvQueryBuffer);

		if ((!fgIsOid) && (rResult == WLAN_STATUS_PENDING))
			rResult = WLAN_STATUS_SUCCESS;

		prQueryStaStatistics->u4Flag |= BIT(1);

	} while (FALSE);

	return rResult;
}				/* wlanoidQueryP2pVersion */

uint32_t
wlanQueryStatistics(struct ADAPTER *prAdapter,
		       void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		       uint32_t *pu4QueryInfoLen, uint8_t fgIsOid)
{
	struct CMD_QUERY_STATISTICS rQueryCmdStatistics = {0};

	ASSERT(prAdapter);
	if (u4QueryBufferLen)
		ASSERT(pvQueryBuffer);
	ASSERT(pu4QueryInfoLen);

	*pu4QueryInfoLen = sizeof(struct PARAM_802_11_STATISTICS_STRUCT);

	if (prAdapter->rAcpiState == ACPI_STATE_D3) {
		DBGLOG(REQ, WARN,
		       "Fail in query receive error! (Adapter not ready). ACPI=D%d, Radio=%d\n",
		       prAdapter->rAcpiState, prAdapter->fgIsRadioOff);
		*pu4QueryInfoLen = sizeof(uint32_t);
		return WLAN_STATUS_ADAPTER_NOT_READY;
	} else if (u4QueryBufferLen < sizeof(struct
					     PARAM_802_11_STATISTICS_STRUCT)) {
		DBGLOG(REQ, WARN, "Too short length %u\n",
		       u4QueryBufferLen);
		return WLAN_STATUS_INVALID_LENGTH;
	}
#if CFG_ENABLE_STATISTICS_BUFFERING
	if (IsBufferedStatisticsUsable(prAdapter) == TRUE) {
		struct PARAM_802_11_STATISTICS_STRUCT *prStatistics;

		*pu4QueryInfoLen = sizeof(struct
					  PARAM_802_11_STATISTICS_STRUCT);
		prStatistics = (struct PARAM_802_11_STATISTICS_STRUCT *)
			       pvQueryBuffer;

		prStatistics->u4Length = sizeof(struct
						PARAM_802_11_STATISTICS_STRUCT);
		prStatistics->rTransmittedFragmentCount =
			prAdapter->rStatStruct.rTransmittedFragmentCount;
		prStatistics->rMulticastTransmittedFrameCount =
			prAdapter->rStatStruct.rMulticastTransmittedFrameCount;
		prStatistics->rFailedCount =
			prAdapter->rStatStruct.rFailedCount;
		prStatistics->rRetryCount =
			prAdapter->rStatStruct.rRetryCount;
		prStatistics->rMultipleRetryCount =
			prAdapter->rStatStruct.rMultipleRetryCount;
		prStatistics->rRTSSuccessCount =
			prAdapter->rStatStruct.rRTSSuccessCount;
		prStatistics->rRTSFailureCount =
			prAdapter->rStatStruct.rRTSFailureCount;
		prStatistics->rACKFailureCount =
			prAdapter->rStatStruct.rACKFailureCount;
		prStatistics->rFrameDuplicateCount =
			prAdapter->rStatStruct.rFrameDuplicateCount;
		prStatistics->rReceivedFragmentCount =
			prAdapter->rStatStruct.rReceivedFragmentCount;
		prStatistics->rMulticastReceivedFrameCount =
			prAdapter->rStatStruct.rMulticastReceivedFrameCount;
		prStatistics->rFCSErrorCount =
			prAdapter->rStatStruct.rFCSErrorCount;
		prStatistics->rTKIPLocalMICFailures.QuadPart = 0;
		prStatistics->rTKIPICVErrors.QuadPart = 0;
		prStatistics->rTKIPCounterMeasuresInvoked.QuadPart = 0;
		prStatistics->rTKIPReplays.QuadPart = 0;
		prStatistics->rCCMPFormatErrors.QuadPart = 0;
		prStatistics->rCCMPReplays.QuadPart = 0;
		prStatistics->rCCMPDecryptErrors.QuadPart = 0;
		prStatistics->rFourWayHandshakeFailures.QuadPart = 0;
		prStatistics->rWEPUndecryptableCount.QuadPart = 0;
		prStatistics->rWEPICVErrorCount.QuadPart = 0;
		prStatistics->rDecryptSuccessCount.QuadPart = 0;
		prStatistics->rDecryptFailureCount.QuadPart = 0;

		return WLAN_STATUS_SUCCESS;
	}
#endif

	kalMemZero(&rQueryCmdStatistics,
		   sizeof(struct CMD_QUERY_STATISTICS));
	rQueryCmdStatistics.ucBssIndex = aisGetDefaultLinkBssIndex(prAdapter);

	return wlanSendSetQueryCmd(prAdapter,
				CMD_ID_GET_STATISTICS,
				FALSE,
				TRUE,
				fgIsOid,
				nicCmdEventQueryStatistics,
				nicOidCmdTimeoutCommon,
				sizeof(struct CMD_QUERY_STATISTICS),
				(uint8_t *)&rQueryCmdStatistics,
				pvQueryBuffer, u4QueryBufferLen);

} /* wlanQueryStatistics */

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
static uint32_t sendStatsUniCmd(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen, uint8_t fgIsOid,
		uint32_t cmd_len)
{
	uint32_t rResult = WLAN_STATUS_SUCCESS;
	struct UNI_CMD_GET_STATISTICS *uni_cmd;
	uint8_t *buf;

#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	struct UNI_CMD_REGULAR_STATS *regStats;
#else
	struct UNI_CMD_BASIC_STATISTICS *basicStatsTag;
	struct UNI_CMD_BSS_LINK_QUALITY *lQTag;
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	struct UNI_CMD_BSS_CURRENT_TX_RATE *txRateTag;
#endif /* CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS */
	struct UNI_CMD_STA_STATISTICS *staStatsTag;
	struct UNI_CMD_LINK_LAYER_STATS *llsTag;

	struct BSS_INFO *prBssInfo;
	struct STA_RECORD *prStaRec;
	uint8_t i;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;

#endif /* CFG_SUPPORT_REG_STAT_FROM_EMI */

	uni_cmd = cnmMemAlloc(prAdapter, RAM_TYPE_MSG, cmd_len);
	if (!uni_cmd) {
		DBGLOG(INIT, ERROR,
		       "Allocate UNI_CMD_GET_STATISTICS ==> FAILED.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* prepare unified cmd tags */
	buf = uni_cmd->aucTlvBuffer;
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	regStats = (struct UNI_CMD_REGULAR_STATS *) buf;
	regStats->u2Tag = UNI_CMD_GET_STATISTICS_TAG_REGULAR_STATS;
	regStats->u2Length = sizeof(*regStats);
#else
	/* UNI_CMD_GET_STATISTICS_TAG_BASIC */
	basicStatsTag = (struct UNI_CMD_BASIC_STATISTICS *) buf;
	basicStatsTag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BASIC;
	basicStatsTag->u2Length = sizeof(*basicStatsTag);
	buf += sizeof(*basicStatsTag);

	/* UNI_CMD_GET_STATISTICS_TAG_BSS_LINK_QUALITY */
	lQTag = (struct UNI_CMD_BSS_LINK_QUALITY *) buf;
	lQTag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BSS_LINK_QUALITY;
	lQTag->u2Length = sizeof(*lQTag);
	buf += sizeof(*lQTag);

	/* UNI_CMD_GET_STATISTICS_TAG_STA for connected AIS BSS */
	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
		if (!prBssInfo || !IS_BSS_AIS(prBssInfo) ||
			kalGetMediaStateIndicated(prAdapter->prGlueInfo,
				i) != MEDIA_STATE_CONNECTED)
			continue;

		prQueryStaStatistics = &prAdapter->rQueryStaStatistics[i];

		prStaRec = cnmGetStaRecByAddress(prAdapter, ANY_BSS_INDEX,
					prQueryStaStatistics->aucMacAddr);
		if (!prStaRec || !prStaRec->fgIsValid)
			continue;

		staStatsTag = (struct UNI_CMD_STA_STATISTICS *) buf;
		staStatsTag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_STA;
		staStatsTag->u2Length = sizeof(*staStatsTag);
		/* FW starec idx is WTBL idx */
		staStatsTag->u1Index = prStaRec->ucWlanIndex;
		staStatsTag->ucLlsReadClear =
			prQueryStaStatistics->ucLlsReadClear;
		staStatsTag->ucResetCounter =
			prQueryStaStatistics->ucResetCounter;
		buf += sizeof(*staStatsTag);
	}

	/* UNI_CMD_GET_STATISTICS_TAG_LINK_LAYER_STATS */
	llsTag = (struct UNI_CMD_LINK_LAYER_STATS *) buf;
	llsTag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_LINK_LAYER_STATS;
	llsTag->u2Length = sizeof(*llsTag);

#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
	buf += sizeof(*llsTag);
	/* UNI_CMD_GET_STATISTICS_TAG_BSS_CURRENT_TX_RATE */
	txRateTag = (struct UNI_CMD_BSS_CURRENT_TX_RATE *) buf;
	txRateTag->u2Tag = UNI_CMD_GET_STATISTICS_TAG_BSS_CURRENT_TX_RATE;
	txRateTag->u2Length = sizeof(*txRateTag);
#endif
#endif /* CFG_SUPPORT_REG_STAT_FROM_EMI */

	rResult = wlanSendSetQueryUniCmd(prAdapter,
			UNI_CMD_ID_GET_STATISTICS,
			FALSE,
			TRUE,
			fgIsOid,
			nicUniEventAllStatsOneCmd,
			nicUniCmdTimeoutCommon,
			cmd_len,
			(void *)uni_cmd,
			pvQueryBuffer, u4QueryBufferLen);
	DBGLOG(REQ, TRACE, "rResult=%u, pvQueryBuffer=%p",
			rResult, pvQueryBuffer);
	cnmMemFree(prAdapter, uni_cmd);
	return rResult;
}

uint32_t wlanQueryStatsOneCmd(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen, uint8_t fgIsOid,
		uint8_t ucBssIndex)
{
	uint32_t rResult = WLAN_STATUS_SUCCESS;
	uint8_t i;
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;
	uint32_t max_cmd_len;
	struct BSS_INFO *prBssInfo;
	uint8_t ucConnBss[MAX_BSSID_NUM] = {0};
	struct LINK_SPEED_EX_ *prLq;
	struct PARAM_GET_STATS_ONE_CMD *prParam;
	uint32_t u4CurrTick;
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	uint32_t u4EmiUpdateMs = 0;
	struct timespec64 rNow;
	struct timespec64 rDrvDiff = {0};
	struct timespec64 rFwDiff = {0};
	struct timespec64 rUpdate;
	struct timespec64 rTimeout = {0};
	struct timespec64 rPeriod;
#endif

	if (unlikely(ucBssIndex >= MAX_BSSID_NUM))
		return WLAN_STATUS_INVALID_DATA;

	DBGLOG(NIC, TRACE, "lastAllStatsUpdateTime:%u\n",
		prAdapter->rAllStatsUpdateTime);

	/* caller should get from cache directly */
	/* basic_stats: prAdapter->rStat */
	/* linkQuality: prAdapter->rLinkQuality.rLq[ucBssIndex] */
	/* staStats: prAdapter->rQueryStaStatistics[ucBssIndex] */

	if (!pvQueryBuffer)
		return WLAN_STATUS_FAILURE;

	GET_CURRENT_SYSTIME(&u4CurrTick);
	prParam = (struct PARAM_GET_STATS_ONE_CMD *)pvQueryBuffer;

	prLq = &prAdapter->rLinkQuality.rLq[ucBssIndex];
	DBGLOG(NIC, TRACE,
		"bssIdx:%u curTime:%u LRValid:%u period:%u\n",
		ucBssIndex, u4CurrTick,
		prLq->fgIsLinkRateValid, prParam->u4Period);
	if (prLq->fgIsLinkRateValid &&
		!CHECK_FOR_TIMEOUT(u4CurrTick,
			prAdapter->rAllStatsUpdateTime,
			MSEC_TO_SYSTIME(prParam->u4Period)))
		return rResult;

	prAdapter->rAllStatsUpdateTime = u4CurrTick;

	/* prepare staStats driver stuff */
	max_cmd_len = sizeof(struct UNI_CMD_GET_STATISTICS) +
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
		sizeof(struct UNI_CMD_REGULAR_STATS);
#else
		sizeof(struct UNI_CMD_BASIC_STATISTICS) +
		sizeof(struct UNI_CMD_BSS_LINK_QUALITY) +
#if CFG_SUPPORT_LLS && CFG_REPORT_TX_RATE_FROM_LLS
		sizeof(struct UNI_CMD_BSS_CURRENT_TX_RATE) +
#endif
		sizeof(struct UNI_CMD_LINK_LAYER_STATS);
#endif /* CFG_SUPPORT_REG_STAT_FROM_EMI */

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, i);
		if (!prBssInfo || !IS_BSS_AIS(prBssInfo) ||
			kalGetMediaStateIndicated(prAdapter->prGlueInfo, i) !=
				MEDIA_STATE_CONNECTED)
			continue;

		prQueryStaStatistics = &prAdapter->rQueryStaStatistics[i];
		COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr,
			prBssInfo->aucBSSID);

		if (updateStaStats(prAdapter, prQueryStaStatistics) !=
			WLAN_STATUS_SUCCESS)
			continue;

		ucConnBss[i] = 1;
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 0)
		max_cmd_len += sizeof(struct UNI_CMD_STA_STATISTICS);
#endif
	}

	/* send cmd if emi doesnot update */
#if (CFG_SUPPORT_REG_STAT_FROM_EMI == 1)
	if (!prAdapter->prStatsAllRegStat) {
		DBGLOG(REQ, WARN, "reg stat emi mapping not done.\n");
		return WLAN_STATUS_FAILURE;
	}

	/* get last sync driver/fw time */
	ktime_get_ts64(&rNow);

	/* get EMI update time */
	kalMemCopyFromIo(&u4EmiUpdateMs,
			&prAdapter->prStatsAllRegStat->u4LastUpdateTime,
			sizeof(uint32_t));

	if (u4EmiUpdateMs == 0)
		goto send_cmd;

	DBGLOG(REQ, TRACE,
		"update:%u drvCur=%ld.%09ld drvSync=%ld.%09ld fwSync=%ld.%09ld\n",
		u4EmiUpdateMs,
		rNow.tv_sec, rNow.tv_nsec,
		prAdapter->rRegStatSyncDrvTs.tv_sec,
		prAdapter->rRegStatSyncDrvTs.tv_nsec,
		prAdapter->rRegStatSyncFwTs.tv_sec,
		prAdapter->rRegStatSyncFwTs.tv_nsec);

	KAL_SET_MSEC_TO_TIME(rUpdate, u4EmiUpdateMs);
	KAL_SET_MSEC_TO_TIME(rPeriod, prParam->u4Period);

	if (kalGetDeltaTime(&rNow, &prAdapter->rRegStatSyncDrvTs, &rDrvDiff) &&
	    kalGetDeltaTime(&rUpdate, &prAdapter->rRegStatSyncFwTs, &rFwDiff) &&
	    kalGetDeltaTime(&rDrvDiff, &rFwDiff, &rTimeout) &&
	    kalTimeCompare(&rTimeout, &rPeriod) <= 0) {
		nicCollectRegStatFromEmi(prAdapter);
	} else {
		DBGLOG(REQ, TRACE,
			"drvDiff=%ld.%09ld fwDiff=%ld.%09ld to=%ld.%09ld per=%ld.%09ld\n",
			rDrvDiff.tv_sec, rDrvDiff.tv_nsec,
			rFwDiff.tv_sec, rFwDiff.tv_nsec,
			rTimeout.tv_sec, rTimeout.tv_nsec,
			rPeriod.tv_sec, rPeriod.tv_nsec);
send_cmd:
		rResult = sendStatsUniCmd(prAdapter, pvQueryBuffer,
			u4QueryBufferLen, pu4QueryInfoLen,
			fgIsOid, max_cmd_len);
	}
#else
	rResult = sendStatsUniCmd(prAdapter, pvQueryBuffer,
		u4QueryBufferLen, pu4QueryInfoLen,
		fgIsOid, max_cmd_len);
#endif

	for (i = 0; i < MAX_BSSID_NUM; i++) {
		if (!ucConnBss[i])
			continue;
		prQueryStaStatistics = &prAdapter->rQueryStaStatistics[i];
		prQueryStaStatistics->u4Flag |= BIT(1);
	}

	return rResult;

}
#endif

/**
 * Called to save STATS_LLS_BSSLOAD_INFO information on scan operation.
 * Update slot by bit mask in fgIsConnected.
 */
void updateLinkStatsApRec(struct ADAPTER *prAdapter, struct BSS_DESC *prBssDesc)
{
#if CFG_SUPPORT_LLS
	struct STATS_LLS_PEER_AP_REC *prPeerApRec;
	struct AIS_FSM_INFO *prAisFsmInfo;
	struct BSS_DESC *prBss;
	int32_t i;
	int32_t j;

	if (!prBssDesc->fgIsConnected)
		return;

	for (i = 0; i < KAL_AIS_NUM; i++) {
		prAisFsmInfo = aisFsmGetInstance(prAdapter, i);
		for (j = 0; j < MLD_LINK_MAX; j++) {
			prBss = aisGetLinkBssDesc(prAisFsmInfo, j);
			if (prBss == prBssDesc)
				break;
		}
	}

	if (i == KAL_AIS_NUM || j == MLD_LINK_MAX) {
		DBGLOG(REQ, WARN, "AP connected flag set (%u,%u) over limit",
		       i, j);
		return;
	}

	prPeerApRec = &prAdapter->rPeerApRec[i][j];
	COPY_MAC_ADDR(prPeerApRec->mac_addr, prBssDesc->aucBSSID);
	prPeerApRec->sta_count = prBssDesc->u2StaCnt;
	prPeerApRec->chan_util = prBssDesc->ucChnlUtilization;

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		DBGLOG(REQ, INFO, "Update record (%u,%u): " MACSTR " %u %u",
				i, j, MAC2STR(prBssDesc->aucBSSID),
				prBssDesc->u2StaCnt,
				prBssDesc->ucChnlUtilization);
#endif
}

#if CFG_SUPPORT_LLS
/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to get LLS data from FW
 *
 * \param[in]  prAdapter        A pointer to the Adapter structure.
 * \param[in]  pvQueryBuffer    Buffer holding query command, also used to hold
 *                              the returned result if any
 * \param[in]  u4QueryBufferLen The size of query command buffer
 * \param[out] pu4QueryInfoLen  Pointer to integer to pass returned size copied
 *                              to return buffer
 *
 * \retval WLAN_STATUS_SUCCESS: Send command success
 *         WLAN_STATUS_FAILURE: Send command failed
 *         WLAN_STATUS_PENDING: Pending for waiting event
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanQueryLinkStats(struct ADAPTER *prAdapter,
		void *pvQueryBuffer, uint32_t u4QueryBufferLen,
		uint32_t *pu4QueryInfoLen)
{
	struct CMD_GET_STATS_LLS rQuery;
	struct CMD_GET_STATS_LLS *cmd =
		(struct CMD_GET_STATS_LLS *)pvQueryBuffer;

	DBGLOG(REQ, TRACE, "cmd: u4Tag=0x%08x, args=%u/%u/%u/%u, len=%u",
			cmd->u4Tag, cmd->ucArg0, cmd->ucArg1,
			cmd->ucArg2, cmd->ucArg3, *pu4QueryInfoLen);
	rQuery = *cmd;

#if CFG_MTK_MDDP_SUPPORT && CFG_SUPPORT_LLS_MDDP
	if (cmd->u4Tag == STATS_LLS_TAG_LLS_DATA)
		mddpGetMdLlsStats(prAdapter);
#endif

	return wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			    CMD_ID_GET_STATS_LLS,	/* ucCID */
			    FALSE,	/* fgSetQuery */
			    TRUE,	/* fgNeedResp */
			    TRUE,	/* fgIsOid */
			    nicCmdEventQueryLinkStats,    /* pfCmdDoneHandler */
			    nicOidCmdTimeoutCommon, /* pfCmdTimeoutHandler */
			    *pu4QueryInfoLen,    /* u4SetQueryInfoLen */
			    (uint8_t *)&rQuery,  /* pucInfoBuffer */
			    pvQueryBuffer,       /* pvSetQueryBuffer */
			    u4QueryBufferLen);   /* u4SetQueryBufferLen */
}
#endif


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to query Nic resource information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
void wlanQueryNicResourceInformation(struct ADAPTER *prAdapter)
{
	/* 3 1. Get Nic resource information from FW */

	/* 3 2. Setup resource parameter */

	/* 3 3. Reset Tx resource */
	nicTxResetResource(prAdapter);
}

#ifndef CFG_SUPPORT_UNIFIED_COMMAND
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to query Nic resource information
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanQueryNicCapabilityV2(struct ADAPTER *prAdapter)
{
	uint32_t u4RxPktLength;
	uint8_t *prEventBuff;
	struct WIFI_EVENT *prEvent;
	struct mt66xx_chip_info *prChipInfo;
	uint32_t chip_id;
	uint32_t u4Time;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	chip_id = prChipInfo->chip_id;

	ASSERT(prAdapter);

	/* Get Nic resource information from FW */
	if (!prChipInfo->isNicCapV1
	    || (prAdapter->u4FwFeatureFlag0 &
		FEATURE_FLAG0_NIC_CAPABILITY_V2)) {

		DBGLOG(INIT, INFO, "Support NIC_CAPABILITY_V2 feature\n");

		wlanSendSetQueryCmdAdv(
			prAdapter, CMD_ID_GET_NIC_CAPABILITY_V2, 0, FALSE,
			TRUE, FALSE, NULL, NULL, 0, NULL, NULL, 0,
			CMD_SEND_METHOD_REQ_RESOURCE);

		/*
		 * receive nic_capability_v2 event
		 */

		/* allocate event buffer */
		prEventBuff = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
					  CFG_RX_MAX_PKT_SIZE);
		if (!prEventBuff) {
			DBGLOG(INIT, WARN, "%s: event buffer alloc failed!\n",
			       __func__);
			return WLAN_STATUS_FAILURE;
		}

		/* get event */
		u4Time = kalGetTimeTick();
		while (TRUE) {
			if (nicRxWaitResponse(prAdapter,
					      1,
					      prEventBuff,
					      CFG_RX_MAX_PKT_SIZE,
					      &u4RxPktLength)
			    != WLAN_STATUS_SUCCESS) {
				DBGLOG(INIT, WARN,
				       "%s: wait for event failed!\n",
				       __func__);

				/* free event buffer */
				cnmMemFree(prAdapter, prEventBuff);

				return WLAN_STATUS_FAILURE;
			}

			if (CHECK_FOR_TIMEOUT(kalGetTimeTick(), u4Time,
				MSEC_TO_SYSTIME(3000))) {
				DBGLOG(HAL, ERROR,
					"Query nic capability timeout\n");
				return WLAN_STATUS_FAILURE;
			}

			/* header checking .. */
			if ((NIC_RX_GET_U2_SW_PKT_TYPE(prEventBuff) &
				prChipInfo->u2RxSwPktBitMap) !=
				prChipInfo->u2RxSwPktEvent) {
				DBGLOG(INIT, WARN,
				       "%s: skip unexpected Rx pkt type[0x%04x]\n",
				       __func__,
				       NIC_RX_GET_U2_SW_PKT_TYPE(prEventBuff));

				continue;
			}

			prEvent = (struct WIFI_EVENT *)
				(prEventBuff + prChipInfo->rxd_size);
			if (prEvent->ucEID != EVENT_ID_NIC_CAPABILITY_V2) {
				DBGLOG(INIT, WARN,
				       "%s: skip unexpected event ID[0x%02x]\n",
				       __func__, prEvent->ucEID);

				continue;
			} else {
				/* hit */
				break;
			}

		}

		/*
		 * parsing elemens
		 */

		nicCmdEventQueryNicCapabilityV2(prAdapter,
						prEvent->aucBuffer);

		/*
		 * free event buffer
		 */
		cnmMemFree(prAdapter, prEventBuff);
	}

	/* Fill capability for different Chip version */
	if (chip_id == 0x6632) {
		/* 6632 only */
		prAdapter->fgIsSupportBufferBinSize16Byte = TRUE;
		prAdapter->fgIsSupportDelayCal = FALSE;
		prAdapter->fgIsSupportGetFreeEfuseBlockCount = FALSE;
		prAdapter->fgIsSupportQAAccessEfuse = FALSE;
		prAdapter->fgIsSupportPowerOnSendBufferModeCMD = FALSE;
		prAdapter->fgIsSupportGetTxPower = FALSE;
	} else {
		prAdapter->fgIsSupportBufferBinSize16Byte = FALSE;
		prAdapter->fgIsSupportDelayCal = TRUE;
		prAdapter->fgIsSupportGetFreeEfuseBlockCount = TRUE;
		prAdapter->fgIsSupportQAAccessEfuse = TRUE;
		prAdapter->fgIsSupportPowerOnSendBufferModeCMD = TRUE;
		prAdapter->fgIsSupportGetTxPower = TRUE;
	}

	return WLAN_STATUS_SUCCESS;
}
#endif

void wlanSetNicResourceParameters(struct ADAPTER
				  *prAdapter)
{
	uint8_t string[128], idx;
	uint32_t u4share;
	uint32_t u4MaxDataPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxDataPageCntPerFrame;
	uint32_t u4MaxCmdPageCntPerFrame =
		prAdapter->rTxCtrl.u4MaxCmdPageCntPerFrame;
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	struct QUE_MGT *prQM = &prAdapter->rQM;
#endif

	/*
	 * Use the settings in config file first,
	 * else, use the settings reported from firmware.
	 */


	/*
	 * 1. assign free page count for each TC
	 */

	/* 1 1. update free page count in TC control: MCU and LMAC */
	prWifiVar->au4TcPageCount[TC4_INDEX] =
		prAdapter->nicTxReousrce.u4CmdTotalResource *
		u4MaxCmdPageCntPerFrame;	 /* MCU */

	u4share = prAdapter->nicTxReousrce.u4DataTotalResource /
		  (TC_NUM - 1); /* LMAC. Except TC_4, which is MCU */
	for (idx = TC0_INDEX; idx < TC_NUM; idx++) {
		if (idx != TC4_INDEX)
			prWifiVar->au4TcPageCount[idx] =
				u4share * u4MaxDataPageCntPerFrame;
	}

	/* 1 2. if there is remaings, give them to TC_3, which is VO */
	prWifiVar->au4TcPageCount[TC3_INDEX] +=
		(prAdapter->nicTxReousrce.u4DataTotalResource %
		 (TC_NUM - 1)) * u4MaxDataPageCntPerFrame;

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/*
	 * 2. assign guaranteed page count for each TC
	 */

	/* 2 1. update guaranteed page count in QM */
	for (idx = 0; idx < TC_NUM; idx++)
		prQM->au4GuaranteedTcResource[idx] =
			prWifiVar->au4TcPageCount[idx];
#endif


#if CFG_SUPPORT_CFG_FILE
	/*
	 * 3. Use the settings in config file first,
	 *    else, use the settings reported from firmware.
	 */

	/* 3 1. update for free page count */
	for (idx = 0; idx < TC_NUM; idx++) {

		/* construct prefix: Tc0Page, Tc1Page... */
		memset(string, 0, sizeof(string));
		kalSnprintf(string, sizeof(string), "Tc%xPage", idx);

		/* update the final value */
		prWifiVar->au4TcPageCount[idx] =
			(uint32_t) wlanCfgGetUint32(prAdapter, string,
					prWifiVar->au4TcPageCount[idx],
					FEATURE_TO_CUSTOMER);
	}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 3 2. update for guaranteed page count */
	for (idx = 0; idx < TC_NUM; idx++) {
		/* construct prefix: Tc0Grt, Tc1Grt... */
		kalMemZero(string, sizeof(string));
		kalSnprintf(string, sizeof(string), "Tc%xGrt", idx);

		/* update the final value */
		prQM->au4GuaranteedTcResource[idx] =
			(uint32_t) wlanCfgGetUint32(prAdapter, string,
					prQM->au4GuaranteedTcResource[idx],
					FEATURE_DEBUG_ONLY);
	}
#endif /* end of #if QM_ADAPTIVE_TC_RESOURCE_CTRL */

#endif /* end of #if CFG_SUPPORT_CFG_FILE */
}


#if CFG_SUPPORT_IOT_AP_BLOCKLIST
void wlanCfgDumpIotApRule(struct ADAPTER *prAdapter)
{
	uint8_t ucRuleIdx;
	struct WLAN_IOT_AP_RULE_T *prIotApRule;

	ASSERT(prAdapter);
	for (ucRuleIdx = 0; ucRuleIdx < CFG_IOT_AP_RULE_MAX_CNT; ucRuleIdx++) {
		prIotApRule = &prAdapter->rIotApRule[ucRuleIdx];
		if (!prIotApRule->u2MatchFlag)
			continue;
		DBGLOG(INIT, TRACE, "IOTAP%d is valid rule\n", ucRuleIdx);
		DBGLOG(INIT, TRACE, "IOTAP%d Flag:0x%X Ver:0x%X\n",
			ucRuleIdx, prIotApRule->u2MatchFlag,
			prIotApRule->ucVersion);
		DBGLOG(INIT, TRACE, "IOTAP%d OUI:%02X:%02X:%02X\n",
			ucRuleIdx,
			prIotApRule->aVendorOui[0],
			prIotApRule->aVendorOui[1],
			prIotApRule->aVendorOui[2]);
		DBGLOG(INIT, TRACE, "IOTAP%d Data:"MACSTR" Mask:"MACSTR"\n",
			ucRuleIdx, MAC2STR(prIotApRule->aVendorData),
			MAC2STR(prIotApRule->aVendorDataMask));
		DBGLOG(INIT, TRACE, "IOTAP%d aBssid:"MACSTR" Mask:"MACSTR"\n",
			ucRuleIdx, MAC2STR(prIotApRule->aBssid),
			MAC2STR(prIotApRule->aBssidMask));
		DBGLOG(INIT, TRACE, "IOTAP%d NSS:%X HT:%X Band:%X Act:%X\n",
			ucRuleIdx, prIotApRule->ucNss,
			prIotApRule->ucHtType,
			prIotApRule->ucBand,
			prIotApRule->u8Action);
	}
}


void wlanCfgLoadIotApRule(struct ADAPTER *prAdapter)
{
	uint8_t  ucCnt;
	uint8_t  *pOffset;
	uint8_t  *pCurTok;
	uint8_t  ucTokId;
	uint8_t  *pNexTok;
	uint8_t  ucStatus;
	uint8_t  aucCfgKey[WLAN_CFG_KEY_LEN_MAX];
	uint8_t  aucCfgVal[WLAN_CFG_VALUE_LEN_MAX];
	struct WLAN_IOT_AP_RULE_T *prIotApRule = NULL;
	struct BSS_DESC *prBssDesc = NULL;
	struct LINK *prBSSDescList =
		&prAdapter->rWifiVar.rScanInfo.rBSSDescList;
	int8_t  aucEleSize[] = {
		sizeof(prIotApRule->ucVersion),
		sizeof(prIotApRule->aVendorOui),
		sizeof(prIotApRule->aVendorData),
		sizeof(prIotApRule->aVendorDataMask),
		sizeof(prIotApRule->aBssid),
		sizeof(prIotApRule->aBssidMask),
		sizeof(prIotApRule->ucNss),
		sizeof(prIotApRule->ucHtType),
		sizeof(prIotApRule->ucBand) + sizeof(prIotApRule->aReserved),
		sizeof(prIotApRule->u8Action)
		};

	ASSERT(prAdapter);
	ASSERT(prAdapter->rIotApRule);

	DBGLOG(INIT, INFO, "IOTAP: Start Parsing Rules\n");
	for (ucCnt = 0; ucCnt < CFG_IOT_AP_RULE_MAX_CNT; ucCnt++) {
		prIotApRule = &prAdapter->rIotApRule[ucCnt];
		kalMemSet(prIotApRule, '\0', sizeof(struct WLAN_IOT_AP_RULE_T));
		kalMemSet(aucCfgVal, '\0', WLAN_CFG_VALUE_LEN_MAX);
		kalMemSet(aucCfgKey, '\0', WLAN_CFG_KEY_LEN_MAX);
		pOffset = (uint8_t *)prIotApRule +
			OFFSET_OF(struct WLAN_IOT_AP_RULE_T, ucVersion);
		pCurTok = &aucCfgVal[0];
		pNexTok = &aucCfgVal[0];
		kalSnprintf(aucCfgKey, WLAN_CFG_KEY_LEN_MAX, "IOTAP%d", ucCnt);
		ucStatus = wlanCfgGet(prAdapter, aucCfgKey, aucCfgVal, NULL, 0,
				      FEATURE_TO_CUSTOMER);
		/*Skip empty rule*/
		if (ucStatus != WLAN_STATUS_SUCCESS)
			continue;

		/*Rule String Check*/
		ucStatus = 0;
		while (*pCurTok != '\0') {
			if (*pCurTok == ':')
				ucStatus++;
			else if (wlanHexToNum(*pCurTok) == -1) {
				ucStatus = -EINVAL;
				break;
			}
			pCurTok++;
		}
		if (ucStatus != WLAN_IOT_AP_FG_MAX-1) {
			DBGLOG(INIT, INFO,
				"Invalid rule IOTAP%d with status %d\n",
				ucCnt, ucStatus);
			continue;
		}

		for (ucTokId = 0; ucTokId < WLAN_IOT_AP_FG_MAX; ucTokId++) {
			pCurTok = kalStrSep((char **)&pNexTok, ":");
			if (pCurTok) {
				if (ucTokId == WLAN_IOT_AP_FG_ACTION) {
					ucStatus = wlanHexToArray(pCurTok,
						pOffset, aucEleSize[ucTokId]);
				} else
					ucStatus = wlanHexToArrayR(pCurTok,
						pOffset, aucEleSize[ucTokId]);
			} else {
				DBGLOG(INIT, TRACE,
				       "Invalid Tok IOTAP%d\n", ucCnt);
				continue;
			}
			DBGLOG(INIT, TRACE,
				"IOTAP%d tok:%d Str:%s status:%d len:%d flag:0x%x\n",
				ucCnt, ucTokId, pCurTok, ucStatus,
				aucEleSize[ucTokId], prIotApRule->u2MatchFlag);

			if (ucStatus) {
				prIotApRule->u2MatchFlag |= BIT(ucTokId);
				/*Record vendor data length*/
				if (ucTokId == WLAN_IOT_AP_FG_DATA)
					prIotApRule->ucDataLen = ucStatus;
				if (ucTokId == WLAN_IOT_AP_FG_DATA_MASK)
					prIotApRule->ucDataMaskLen = ucStatus;
			}
			pOffset += aucEleSize[ucTokId];
		}
		/*Rule Check*/
		if (prIotApRule->ucDataMaskLen &&
			prIotApRule->ucDataMaskLen != prIotApRule->ucDataLen)
			prIotApRule->u2MatchFlag = 0;
		if (prIotApRule->u2MatchFlag == 0)
			DBGLOG(INIT, INFO, "Invalid Rule IOTAP%d\n", ucCnt);
	}

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
			    struct BSS_DESC) {
		prBssDesc->fgIotApActionValid = FALSE;
	}
}
#endif



/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to re-assign tx resource based on firmware's report
 *
 * @param prAdapter      Pointer of Adapter Data Structure
 *
 * @return WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
void wlanUpdateNicResourceInformation(struct ADAPTER
				      *prAdapter)
{
	/*
	 * 3 1. Query TX resource
	 */

	/* information is not got from firmware, use default value */
	if (prAdapter->fgIsNicTxReousrceValid != TRUE)
		return;

	/* 3 2. Setup resource parameters */
	if (prAdapter->nicTxReousrce.txResourceInit)
		prAdapter->nicTxReousrce.txResourceInit(prAdapter);
	else
		wlanSetNicResourceParameters(prAdapter);/* 6632, 7668 ways*/

	/* 3 3. Reset Tx resource */
	nicTxResetResource(prAdapter);

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	/* 3 4. Reset QM resource */
	qmResetTcControlResource(
		prAdapter); /*CHIAHSUAN, TBD, NO PLE YET*/
#endif

	halTxResourceResetHwTQCounter(prAdapter);
}


#if 0
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to SET network interface index for a network
 *        interface.
 *        A network interface is a TX/RX data port hooked to OS.
 *
 * @param prGlueInfo                     Pointer of prGlueInfo Data Structure
 * @param ucNetInterfaceIndex            Index of network interface
 * @param ucBssIndex                     Index of BSS
 *
 * @return VOID
 */
/*----------------------------------------------------------------------------*/
void wlanBindNetInterface(struct GLUE_INFO *prGlueInfo,
			  uint8_t ucNetInterfaceIndex,
			  void *pvNetInterface)
{
	struct NET_INTERFACE_INFO *prNetIfInfo;

	prNetIfInfo =
		&prGlueInfo->arNetInterfaceInfo[ucNetInterfaceIndex];

	prNetIfInfo->pvNetInterface = pvNetInterface;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to SET BSS index for a network interface.
 *           A network interface is a TX/RX data port hooked to OS.
 *
 * @param prGlueInfo                     Pointer of prGlueInfo Data Structure
 * @param ucNetInterfaceIndex            Index of network interface
 * @param ucBssIndex                     Index of BSS
 *
 * @return VOID
 */
/*----------------------------------------------------------------------------*/
void wlanBindBssIdxToNetInterface(struct GLUE_INFO *prGlueInfo,
				  uint8_t ucBssIndex,
				  void *pvNetInterface)
{
	struct NET_INTERFACE_INFO *prNetIfInfo;

	if (ucBssIndex >= prGlueInfo->prAdapter->ucSwBssIdNum) {
		DBGLOG(INIT, ERROR,
		       "Array index out of bound, ucBssIndex=%u\n", ucBssIndex);
		return;
	}

	DBGLOG(INIT, LOUD,
		"ucBssIndex = %d, pvNetInterface = %p\n",
		ucBssIndex, pvNetInterface);

	prNetIfInfo = &prGlueInfo->arNetInterfaceInfo[ucBssIndex];

	prNetIfInfo->ucBssIndex = ucBssIndex;
	prNetIfInfo->pvNetInterface = pvNetInterface;
	/* prGlueInfo->aprBssIdxToNetInterfaceInfo[ucBssIndex] = prNetIfInfo; */
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to GET BSS index for a network interface.
 *           A network interface is a TX/RX data port hooked to OS.
 *
 * @param prGlueInfo                     Pointer of prGlueInfo Data Structure
 * @param ucNetInterfaceIndex       Index of network interface
 *
 * @return UINT_8                         Index of BSS
 */
/*----------------------------------------------------------------------------*/
uint8_t wlanGetBssIdxByNetInterface(struct GLUE_INFO *prGlueInfo,
			    void *pvNetInterface)
{
	uint8_t ucIdx = 0;

	for (ucIdx = 0; ucIdx < MAX_BSSID_NUM; ucIdx++) {
		if (prGlueInfo->arNetInterfaceInfo[ucIdx].pvNetInterface ==
		    pvNetInterface)
			break;
	}

	return ucIdx;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to GET network interface for a BSS.
 *           A network interface is a TX/RX data port hooked to OS.
 *
 * @param prGlueInfo                     Pointer of prGlueInfo Data Structure
 * @param ucBssIndex                     Index of BSS
 *
 * @return PVOID                         pointer of network interface structure
 */
/*----------------------------------------------------------------------------*/
void *wlanGetNetInterfaceByBssIdx(struct GLUE_INFO *prGlueInfo,
				uint8_t ucBssIndex)
{
	return prGlueInfo->arNetInterfaceInfo[ucBssIndex].pvNetInterface;
}

static void wlanParseMloFreqList(struct ADAPTER *prAdapter,
	uint8_t *aucCfgValue, uint32_t *pu4FreqList, uint32_t u4MaxFreqListNum)
{
	const uint8_t acDelim[] = " ";
	uint8_t *pucPtr = NULL;
	uint32_t u4Freq = 0, u4Idx = 0;
	int32_t i4Ret = 0;

	while ((pucPtr = kalStrSep((char **)&aucCfgValue, acDelim)) != NULL) {
		if (u4Idx >= u4MaxFreqListNum) {
			DBGLOG(INIT, WARN,
				"Exceeds max freq list num (%u)\n",
				u4MaxFreqListNum);
			break;
		}

		if (!kalStrCmp(pucPtr, ""))
			continue;

		i4Ret = kalkStrtou32(pucPtr, 0, &u4Freq);
		if (i4Ret || !u4Freq)
			continue;

		pu4FreqList[u4Idx++] = u4Freq;
	}
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to initialize WLAN feature options
 *
 * @param prAdapter  Pointer of ADAPTER_T
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanInitFeatureOption(struct ADAPTER *prAdapter)
{
	wlanInitFeatureOptionImpl(prAdapter, NULL);
}

void wlanInitFeatureOptionImpl(struct ADAPTER *prAdapter, uint8_t *pucKey)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	struct QUE_MGT *prQM = &prAdapter->rQM;
#endif
	uint32_t u4TxHifRes = 0, u4Idx = 0;
	uint32_t u4PlatformBoostCpuTh;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

	/* Constant feature options */
#if CFG_SUPPORT_LITTLE_CPU_BOOST
	prWifiVar->u4BoostLittleCpuTh = kalGetLittleCpuBoostThreshold();
#endif /* CFG_SUPPORT_LITTLE_CPU_BOOST */

/* Even if the array size not enough, this always assign __VAL to __FREATURE. */
/* If __DBG is TRUE, it means the config is unable to modify in user load */
#define INIT_STR(__FEATURE, __KEY, __VAL, __DBG) \
{\
	if (!pucKey || !kalStrnCmp(pucKey, __KEY, WLAN_CFG_KEY_LEN_MAX - 1)) { \
		if (wlanCfgGet(prAdapter, __KEY, __FEATURE, __VAL, \
				0, __DBG) != WLAN_STATUS_SUCCESS) \
			DBGLOG(INIT, WARN, \
				"Fail to get key %s and set val %s\n", \
				__KEY, __VAL); \
	} \
}

#define INIT_TYPE(__FEATURE, __FUNC, __KEY, __VAL, __DBG) \
{\
	if (!pucKey || !kalStrnCmp(pucKey, __KEY, WLAN_CFG_KEY_LEN_MAX - 1)) { \
		__FEATURE = TYPEOF(__FEATURE)__FUNC(prAdapter, __KEY, __VAL, \
						    __DBG); \
	} \
}

#define INIT_UINT(__FEATURE, __KEY, __VAL, __DBG) \
	INIT_TYPE(__FEATURE, wlanCfgGetUint32, __KEY, __VAL, __DBG)

#define INIT_INT(__FEATURE, __KEY, __VAL, __DBG) \
	INIT_TYPE(__FEATURE, wlanCfgGetInt32, __KEY, __VAL, __DBG)


	/* Feature options will be filled by config file */
	INIT_UINT(prWifiVar->ucQoS, "Qos", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaHt, "StaHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaVht, "StaVHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		INIT_UINT(prWifiVar->ucStaHe, "StaHE", FEATURE_ENABLED,
			  FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucApHe, "ApHE", FEATURE_ENABLED,
			  FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucP2pGoHe, "P2pGoHE", FEATURE_ENABLED,
			  FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucP2pGcHe, "P2pGcHE", FEATURE_ENABLED,
			  FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucHeMaxMcsMap2g, "HeMaxMcsMap2g",
			  HE_CAP_INFO_MCS_MAP_MCS11, FEATURE_DEBUG_ONLY);
		INIT_UINT(prWifiVar->ucHeMaxMcsMap5g, "HeMaxMcsMap5g",
			  HE_CAP_INFO_MCS_MAP_MCS11, FEATURE_DEBUG_ONLY);
		INIT_UINT(prWifiVar->ucHeMaxMcsMap6g, "HeMaxMcsMap6g",
			  HE_CAP_INFO_MCS_MAP_MCS11, FEATURE_DEBUG_ONLY);
		if (prWifiVar->ucHeMaxMcsMap2g >= HE_CAP_INFO_MCS_NOT_SUPPORTED)
			prWifiVar->ucHeMaxMcsMap2g = HE_CAP_INFO_MCS_MAP_MCS11;
		if (prWifiVar->ucHeMaxMcsMap5g >= HE_CAP_INFO_MCS_NOT_SUPPORTED)
			prWifiVar->ucHeMaxMcsMap5g = HE_CAP_INFO_MCS_MAP_MCS11;
		if (prWifiVar->ucHeMaxMcsMap6g >= HE_CAP_INFO_MCS_NOT_SUPPORTED)
			prWifiVar->ucHeMaxMcsMap6g = HE_CAP_INFO_MCS_MAP_MCS11;
	}
#endif
	INIT_UINT(prWifiVar->ucStaMaxMcsMap, "StaMaxMcsMap", 0xFF,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucForceTrxConfig,
		"ForceTrxConfig", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#if (CFG_SUPPORT_802_11BE == 1)
	INIT_UINT(prWifiVar->ucStaEht, "StaEHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApEht, "ApEHT", AP_EHT_DEFAULT_VALUE,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGoEht, "P2pGoEHT", FEATURE_FORCE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGcEht, "P2pGcEHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2RxEhtBaSize,
		"RxEhtBaSize", WLAN_EHT_MAX_BA_SIZE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2TxEhtBaSize,
		"TxEhtBaSize", WLAN_EHT_MAX_BA_SIZE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgMoveWinOnMissingLast, "MoveWinOnMissingLast",
		  !RX_REORDER_WAIT_FOR_LAST_FRAG, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RxDropResetThreshold,
		"RxDropResetThreshold", 5, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2BaExtSize,
		"BaExtSize", WLAN_RX_BA_EXT_SIZE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4AisEHTNumber,
		"AisEHTNumber", KAL_AIS_NUM, FEATURE_TO_CUSTOMER);
	if (prWifiVar->u2BaExtSize > WLAN_RX_BA_EXT_MAX_SIZE)
		prWifiVar->u2BaExtSize = WLAN_RX_BA_EXT_MAX_SIZE;
	INIT_UINT(prWifiVar->u4BaVerboseLogging, "BaVerboseLog", 0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtAmsduInAmpduRx,
		"EhtAmsduInAmpduRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucEhtAmsduInAmpduTx,
		"EhtAmsduInAmpduTx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaEhtBfee, "StaEHTBfee", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucEhtOMCtrl, "EhtOMCtrl", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaEht242ToneRUWt20M,
		"StaEht242ToneRUWt20M", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtNDP4xLTF3dot2usGI,
		"EhtNDP4xLTF3dot2usGI", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtSUBfer, "EhtSUBfer", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtSUBfee, "EhtSUBfee", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtBfeeSSLeEq80m, "EhtBfeeSSLeEq80m", 3,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtBfee160m, "EhtBfee160m", 3,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtBfee320m, "EhtBfee320m", 3,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtNG16SUFeedback,
		"EhtNG16SUFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtNG16MUFeedback,
		"EhtNG16MUFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtCodebook75MuFeedback,
		"EhtCodebook75MuFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtTrigedSUBFFeedback,
		"EhtTrigedSUBFFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtTrigedMUBFPartialBWFeedback,
		"EhtTrigedMUBFPartialBWFeedback", FEATURE_ENABLED,
		FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtTrigedCQIFeedback,
		"EhtTrigedCQIFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtPartialBwDLMUMIMO,
		"EhtPartialBwDLMUMIMO", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtMUPPDU4xEHTLTFdot8usGI,
		"EhtMUPPDU4xEHTLTFdot8usGI", FEATURE_ENABLED,
		FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtNonTrigedCQIFeedback,
		"EhtNonTrigedCQIFeedback", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtTx1024QAM4096QAMLe242ToneRU,
		"EhtTx1024QAM4096QAMLe242ToneRU", FEATURE_ENABLED,
		FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtRx1024QAM4096QAMLe242ToneRU,
		"EhtRx1024QAM4096QAMLe242ToneRU", FEATURE_ENABLED,
		FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtCommonNominalPktPadding,
		"EhtCommonNominalPktPadding", COMMON_NOMINAL_PAD_16_US,
		FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtMaxLTFNum, "EhtMaxLTFNum", 0x0B,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtMCS15, "EhtMCS15", 0xff,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtDup6G, "EhtDup6G", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEht20MRxNDPWiderBW,
		"Eht20MRxNDPWiderBW", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEhtTbSndFBRateLimit,
		"EhtTbSndFBRateLimit", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	if (!pucKey)
		prWifiVar->ucPresetLinkId = MLD_LINK_ID_NONE;
	INIT_UINT(prWifiVar->fgForceRrmMloScan,
		"ForceRrmMloScan", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucMldLinkMax, "MldLinkMax", MLD_LINK_MAX,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaMldLinkMax, "StaMldLinkMax", MLD_STA_LINK_MAX,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApMldLinkMax, "ApMldLinkMax", MLD_AP_LINK_MAX,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pMldLinkMax, "P2pMldLinkMax", MLD_P2P_LINK_MAX,
		  FEATURE_TO_CUSTOMER);
	if (prWifiVar->ucStaMldLinkMax > prWifiVar->ucMldLinkMax) {
		DBGLOG(INIT, WARN,
			"StaMldLinkMax %d => %d\n",
			prWifiVar->ucStaMldLinkMax, prWifiVar->ucMldLinkMax);
		prWifiVar->ucStaMldLinkMax = prWifiVar->ucMldLinkMax;
	}
	if (prWifiVar->ucApMldLinkMax > prWifiVar->ucMldLinkMax) {
		DBGLOG(INIT, WARN,
			"ApMldLinkMax %d => %d\n",
			prWifiVar->ucApMldLinkMax, prWifiVar->ucMldLinkMax);
		prWifiVar->ucApMldLinkMax = prWifiVar->ucMldLinkMax;
	}
	if (prWifiVar->ucP2pMldLinkMax > prWifiVar->ucMldLinkMax) {
		DBGLOG(INIT, WARN,
			"P2pMldLinkMax %d => %d\n",
			prWifiVar->ucP2pMldLinkMax, prWifiVar->ucMldLinkMax);
		prWifiVar->ucP2pMldLinkMax = prWifiVar->ucMldLinkMax;
	}

	INIT_UINT(prWifiVar->ucStaMldMainLinkIdx,
		"StaMldMainLinkIdx", MLD_LINK_ID_NONE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaPreferMldAddr,
		"StaPreferMldAddr", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucEmlsrLinkWeight,
		"EmlsrLinkWeight", 50, FEATURE_TO_CUSTOMER);
	INIT_STR(prWifiVar->aucMloP2pPreferFreq,
		"MloP2pPreferFreq", "", FEATURE_TO_CUSTOMER);
	wlanParseMloFreqList(prAdapter,
			     prWifiVar->aucMloP2pPreferFreq,
			     prWifiVar->au4MloP2p2ndLinkFreqs,
			     ARRAY_SIZE(prWifiVar->au4MloP2p2ndLinkFreqs));
	INIT_STR(prWifiVar->aucMloSapPreferFreq,
		"MloSapPreferFreq", "", FEATURE_TO_CUSTOMER);
	wlanParseMloFreqList(prAdapter,
			     prWifiVar->aucMloSapPreferFreq,
			     prWifiVar->au4MloSap2ndLinkFreqs,
			     ARRAY_SIZE(prWifiVar->au4MloSap2ndLinkFreqs));
	INIT_UINT(prWifiVar->ucMlProbeRetryLimit,
		"MlProbeRetryLimit", ML_PROBE_RETRY_COUNT, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucEnableMlo, "EnableMlo", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucMaxSimuLinks,
		"MaxSimultaneousLinks", 0xff, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucMldRetryCount, "MldRetryCount", MLD_RETRY_COUNT,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgApRemovalByT2LM,
		"ApRemovalByT2LM", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4ApRemovalMarginMs,
		"ApRemovalMarginMs", 250, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNonApMldEMLSupport,
		"NonApMldEML", CFG_DEFAULT_ENABLE_EMLSR, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApMldEMLSupport,
		"ApMldEML", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgEnTuao, "EnableTuao", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgMldSyncLinkAddr,
		"MldSyncLinkAddr", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucNonApHyMloSupport,
		"NonApHybridMlo", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);

#if (CFG_SUPPORT_802_11BE_MLO == 1)
	INIT_UINT(prWifiVar->ucT2LMNegotiationSupport, "T2LMNegotiationSupport",
		  T2LM_ALL_TIDS_SAME_LINK, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4T2LMMarginMs, "T2LMMarginMs", 250,
		  FEATURE_DEBUG_ONLY);
#if (CFG_SUPPORT_802_11BE_EPCS == 1)
	INIT_UINT(prWifiVar->fgEnEpcs, "EnableEpcs", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
#endif
#endif /* CFG_SUPPORT_802_11BE_MLO */
#endif /* CFG_SUPPORT_802_11BE */
	INIT_UINT(prWifiVar->ucApHt, "ApHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#if CFG_TC1_FEATURE
	INIT_UINT(prWifiVar->ucApVht, "ApVHT", FEATURE_FORCE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->ucApVht, "ApVHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->ucP2pGoHt,	"P2pGoHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGoVht, "P2pGoVHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGcHt,	"P2pGcHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGcVht, "P2pGcVHT", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAmpduRx,	"AmpduRx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAmpduTx,	"AmpduTx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucAmsduInAmpduRx,
		"AmsduInAmpduRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAmsduInAmpduTx,
		"AmsduInAmpduTx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucHtAmsduInAmpduRx,
		"HtAmsduInAmpduRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucHtAmsduInAmpduTx,
		"HtAmsduInAmpduTx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucVhtAmsduInAmpduRx,
		"VhtAmsduInAmpduRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucVhtAmsduInAmpduTx,
		"VhtAmsduInAmpduTx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucUapsd, "Uapsd",	FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaUapsd, "StaUapsd", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucApUapsd, "ApUapsd", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
#if (CFG_ENABLE_WIFI_DIRECT && CFG_MTK_ANDROID_WMT)
	INIT_UINT(prWifiVar->u4RegP2pIfAtProbe,
		"RegP2pIfAtProbe", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#else
	INIT_UINT(prWifiVar->u4RegP2pIfAtProbe,
		"RegP2pIfAtProbe", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
#endif

	INIT_UINT(prWifiVar->ucRegP2pMode,
		"RegP2pMode", DEFAULT_RUNNING_P2P_MODE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pShareMacAddr,
		"P2pShareMacAddr", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->ucTxShortGI, "SgiTx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxShortGI, "SgiRx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTxLdpc, "LdpcTx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxLdpc, "LdpcRx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTxStbc, "StbcTx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxStbc, "StbcRx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxStbcNss, "StbcRxNss", 1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTxGf, "GfTx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxGf, "GfRx", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucMCS32, "MCS32", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#if (CFG_SUPPORT_WIFI_6G == 1)
	INIT_UINT(prWifiVar->ucUnsolProbeResp,
		"UnsolProbeResp", FEATURE_FORCE_ENABLED, FEATURE_DEBUG_ONLY);
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		INIT_UINT(prWifiVar->ucHeAmsduInAmpduRx, "HeAmsduInAmpduRx",
			  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucHeAmsduInAmpduTx, "HeAmsduInAmpduTx",
			  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucTrigMacPadDur, "TrigMacPadDur",
			  HE_CAP_TRIGGER_PAD_DURATION_16, FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucMaxAmpduLenExp, "MaxAmpduLenExt",
			  HE_CAP_MAX_AMPDU_LEN_EXP, FEATURE_DEBUG_ONLY);
		INIT_UINT(prWifiVar->fgEnableSR,
			"SREnable", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
		INIT_UINT(prWifiVar->ucHeSUMU4xHeLTF,
			"HeSUMU4xHeLTF", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	}
#endif

#if (CFG_SUPPORT_TWT == 1)
	INIT_UINT(prWifiVar->ucTWTRequester, "TWTRequester", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTWTResponder, "TWTResponder", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
#if (CFG_SUPPORT_TWT_STA_CNM == 1)
	INIT_UINT(prWifiVar->u4TwtCnmAbortTimeoutMs, "TwtCnmAbortTimeoutMs",
			TWT_CNM_GRANT_DEFAULT_INTERVAL_MS, FEATURE_TO_CUSTOMER);
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	INIT_UINT(prWifiVar->ucTWTStaBandBitmap, "TWTStaBandBitmap",
		  BIT(BAND_2G4) | BIT(BAND_5G) | BIT(BAND_6G),
		  FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->ucTWTStaBandBitmap, "TWTStaBandBitmap",
		  BIT(BAND_2G4) | BIT(BAND_5G), FEATURE_TO_CUSTOMER);
#endif
#endif

#if (CFG_SUPPORT_TWT_HOTSPOT == 1)
	INIT_UINT(prWifiVar->ucTWTHotSpotSupport,
		"TWTHotSpotSupport", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif

#if (CFG_SUPPORT_BTWT == 1)
	INIT_UINT(prWifiVar->ucBTWTSupport, "BTWTSupport", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
#endif

#if (CFG_SUPPORT_RTWT == 1)
	INIT_UINT(prWifiVar->ucRTWTSupport, "RTWTSupport", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucRTWTStautProtect, "RTWTStautProtect",
		  FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
#endif

	INIT_UINT(prWifiVar->ucSigTaRts, "SigTaRts", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucDynBwRts, "DynBwRts", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucTxopPsTx, "TxopPsTx", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	/* HT BFee has IOT issue
	 * only support HT BFee when force mode for testing
	 */
	INIT_UINT(prWifiVar->ucStaHtBfee, "StaHTBfee", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaVhtBfee, "StaVHTBfee", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaVhtMuBfee, "StaVHTMuBfee", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaHtBfer, "StaHTBfer", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucStaVhtBfer, "StaVHTBfer", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);

#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		INIT_UINT(prWifiVar->ucStaHeBfee,
			"StaHEBfee", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
		INIT_UINT(prWifiVar->ucStaHeSuBfer,
			"StaHESUBfer", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	}
	INIT_UINT(prWifiVar->ucHeOMCtrl, "HeOMCtrl", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucRxCtrlToMutiBss,
		"RxCtrlToMutiBss", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucStaHePpRx, "StaHePpRx", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucHeDynamicSMPS,
		"HeDynamicSMPS", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucHeHTC, "HeHTC", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#endif

	INIT_UINT(prWifiVar->ucBtmCap, "BtmCap", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);

	/* Init HT MU dynamic SMPS STR Capability*/
	INIT_UINT(prWifiVar->ucHtSmps2g4, "Sta2gHtSmpsCap",
	      DEFAULT_HT_SMPS_2G4_CAP, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucHtSmps5g, "Sta5gHtSmpsCap",
	      DEFAULT_HT_SMPS_5G_CAP, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucHtSmps6g, "Sta6gHtSmpsCap",
	      DEFAULT_HT_SMPS_6G_CAP, FEATURE_DEBUG_ONLY);

	/* 0: disabled
	 * 1: Tx done event to driver
	 * 2: Tx status to FW only
	 */
	INIT_UINT(prWifiVar->ucDataTxDone, "DataTxDone", 0, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucDataTxRateMode,
		"DataTxRateMode", DATA_RATE_MODE_AUTO, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DataTxRateCode, "DataTxRateCode", 0x0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucApWpsMode, "ApWpsMode", 0, FEATURE_TO_CUSTOMER);
	DBGLOG(INIT, TRACE, "ucApWpsMode = %u\n", prWifiVar->ucApWpsMode);

	INIT_UINT(prWifiVar->ucThreadScheduling, "ThreadSched", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucThreadPriority, "ThreadPriority",
		  WLAN_THREAD_TASK_PRIORITY, FEATURE_TO_CUSTOMER);
	INIT_INT(prWifiVar->cThreadNice, "ThreadNice", WLAN_THREAD_TASK_NICE,
		 FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rQM.u4MaxForwardBufferCount, "ApForwardBufferCnt",
		  QM_FWD_PKT_QUE_THRESHOLD, FEATURE_TO_CUSTOMER);

	/* AP channel setting
	 * 0: auto
	 */
	INIT_UINT(prWifiVar->ucApChannel, "ApChannel", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2ApFreq, "ApFreq", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApAcsChannel[0], "ApAcs2gChannel", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApAcsChannel[1], "ApAcs5gChannel", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApAcsChannel[2], "ApAcs6gChannel", 0,
		  FEATURE_TO_CUSTOMER);

	/*
	 * 0: SCN
	 * 1: SCA
	 * 2: RES
	 * 3: SCB
	 */
	INIT_UINT(prWifiVar->ucApSco, "ApSco", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pGoSco, "P2pGoSco", 0, FEATURE_DEBUG_ONLY);

	/* Max bandwidth setting
	 * 0: 20Mhz
	 * 1: 40Mhz
	 * 2: 80Mhz
	 * 3: 160Mhz
	 * 4: 80+80Mhz
	 * Note: For VHT STA, BW 80Mhz is a must!
	 */
	INIT_UINT(prWifiVar->ucStaBandwidth, "StaBw", MAX_BW_320_2MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucSta2gBandwidth, "Sta2gBw", DEFAULT_STA_2G_BW,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucSta5gBandwidth, "Sta5gBw", DEFAULT_STA_5G_BW,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucSta6gBandwidth, "Sta6gBw", DEFAULT_STA_6G_BW,
		  FEATURE_TO_CUSTOMER);

	/* GC,GO */
	INIT_UINT(prWifiVar->ucP2p2gBandwidth, "P2p2gBw", DEFAULT_P2P_2G_BW,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2p5gBandwidth, "P2p5gBw", MAX_BW_80MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2p6gBandwidth, "P2p6gBw", MAX_BW_320_1MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApBandwidth, "ApBw", MAX_BW_320_2MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp2gBandwidth, "Ap2gBw", DEFAULT_SAP_2G_BW,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp5gBandwidth, "Ap5gBw", MAX_BW_80MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp6gBandwidth, "Ap6gBw", MAX_BW_320_1MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApChnlDefFromCfg,
		"ApChnlDefFromCfg", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucApAllowHtVhtTkip,
		"ApAllowHtVhtTkip", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucApForceSleep, "ApForceSleep", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucNSS, "Nss", DEFAULT_NSS, FEATURE_TO_CUSTOMER);

#ifdef CFG_FORCE_AP1NSS
	INIT_UINT(prWifiVar->ucAp6gNSS, "Ap6gNss", 1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp5gNSS, "Ap5gNss", 1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp2gNSS, "Ap2gNss", 1, FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->ucAp6gNSS, "Ap6gNss", DEFAULT_NSS,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp5gNSS, "Ap5gNss", DEFAULT_NSS,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAp2gNSS, "Ap2gNss", DEFAULT_NSS,
		  FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->ucGo6gNSS, "Go6gNss", DEFAULT_NSS,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucGo5gNSS, "Go5gNss", DEFAULT_NSS,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucGo2gNSS, "Go2gNss", DEFAULT_NSS,
		  FEATURE_DEBUG_ONLY);

	/* Max Rx MPDU length setting
	 * 0: 3k
	 * 1: 8k
	 * 2: 11k
	 */
	INIT_UINT(prWifiVar->ucRxMaxMpduLen, "RxMaxMpduLen",
		  VHT_CAP_INFO_MAX_MPDU_LEN_3K, FEATURE_DEBUG_ONLY);

#if (CFG_SUPPORT_RX_QUOTA_INFO == 1)
	INIT_UINT(prWifiVar->ucRxQuotaInfoEn,
		"RxQuotaInfoEn", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#endif
	/* Max Tx AMSDU in AMPDU length *in BYTES* */
	INIT_UINT(prWifiVar->u4HtTxMaxAmsduInAmpduLen, "HtTxMaxAmsduInAmpduLen",
		  WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4VhtTxMaxAmsduInAmpduLen,
		  "VhtTxMaxAmsduInAmpduLen", WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TxMaxAmsduInAmpduLen, "TxMaxAmsduInAmpduLen",
		  WLAN_TX_MAX_AMSDU_IN_AMPDU_LEN, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucTcRestrict, "TcRestrict", 0xFF,
		  FEATURE_DEBUG_ONLY);
	/* Max Tx dequeue limit: 0 => auto */
	INIT_UINT(prWifiVar->u4MaxTxDeQLimit, "MaxTxDeQLimit", 0x0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucAlwaysResetUsedRes, "AlwaysResetUsedRes", 0x0,
		  FEATURE_DEBUG_ONLY);

	/* debug usage, skip specefic bssindex */
	INIT_UINT(prWifiVar->ucBssIdStartValue, "BssIdStartValue", 0,
		  FEATURE_DEBUG_ONLY);

#if CFG_SUPPORT_MTK_SYNERGY
	INIT_UINT(prWifiVar->ucMtkOui, "MtkOui", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	if (!pucKey) {
		prWifiVar->aucMtkFeature[0] = 0xff;
		prWifiVar->aucMtkFeature[1] = 0xff;
		prWifiVar->aucMtkFeature[2] = 0xff;
		prWifiVar->aucMtkFeature[3] = 0xff;
	}
	INIT_UINT(prWifiVar->ucGbandProbe256QAM,
		"Probe256QAM", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif
#if CFG_SUPPORT_VHT_IE_IN_2G
	INIT_UINT(prWifiVar->ucVhtIeIn2g, "VhtIeIn2G", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->fgApLegacyQosMap, "ApLegacyQosMap",
			FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucCmdRsvResource,
		"TxCmdRsv", QM_CMD_RESERVED_THRESHOLD, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4MgmtQueueDelayTimeout, "TxMgmtQueTO",
		  QM_MGMT_QUEUED_TIMEOUT, FEATURE_TO_CUSTOMER); /* ms */

	/* Performance related */
	INIT_UINT(prWifiVar->u4HifIstLoopCount,
		"IstLoop", CFG_IST_LOOP_COUNT, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4Rx2OsLoopCount, "Rx2OsLoop", 4,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4HifTxloopCount, "HifTxLoop", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TxFromOsLoopCount, "OsTxLoop", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TxRxLoopCount, "Rx2ReorderLoop", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TxIntThCount, "IstTxTh", HIF_IST_TX_THRESHOLD,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4NetifStopTh, "NetifStopTh",
		  CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4NetifStartTh, "NetifStartTh",
		  CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);

#if CFG_ADJUST_NETIF_TH_BY_BAND
	/*
	 * 2.4g Band:
	 * StartTh: 128
	 * StopTh : 256
	 */
	INIT_UINT(prWifiVar->au4NetifStopTh[BAND_2G4], "2gNetifStopTh",
		  NIC_BSS_LOW_RATE_TOKEN_CNT, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->au4NetifStartTh[BAND_2G4], "2gNetifStartTh",
		  NIC_BSS_LOW_RATE_TOKEN_CNT >> 1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->au4NetifStopTh[BAND_5G], "5gNetifStopTh",
		  CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->au4NetifStartTh[BAND_5G], "5gNetifStartTh",
		  CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);
#if (CFG_SUPPORT_WIFI_6G == 1)
	INIT_UINT(prWifiVar->au4NetifStopTh[BAND_6G], "6gNetifStopTh",
		  CFG_TX_STOP_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->au4NetifStartTh[BAND_6G], "6gNetifStartTh",
		  CFG_TX_START_NETIF_PER_QUEUE_THRESHOLD, FEATURE_DEBUG_ONLY);
#endif
#endif /* CFG_ADJUST_NETIF_TH_BY_BAND */

	INIT_UINT(prWifiVar->ucTxBaSize, "TxBaSize", WLAN_LEGACY_MAX_BA_SIZE,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucRxHtBaSize,
		"RxHtBaSize", WLAN_LEGACY_MAX_BA_SIZE, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucRxVhtBaSize,
		"RxVhtBaSize", WLAN_LEGACY_MAX_BA_SIZE, FEATURE_DEBUG_ONLY);
#if (CFG_SUPPORT_802_11AX == 1)
	if (fgEfuseCtrlAxOn == 1) {
		INIT_UINT(prWifiVar->u2RxHeBaSize,
			"RxHeBaSize", WLAN_HE_MAX_BA_SIZE, FEATURE_DEBUG_ONLY);
		INIT_UINT(prWifiVar->u2TxHeBaSize,
			"TxHeBaSize", WLAN_HE_MAX_BA_SIZE, FEATURE_DEBUG_ONLY);
	}
#endif

#if CFG_SUPPORT_SMART_GEAR
	INIT_UINT(prWifiVar->ucSGCfg, "SGCfg", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	/* 2.4G default is WF0 when enable SG SISO mode*/
	INIT_UINT(prWifiVar->ucSG24GFavorANT,
		"SG24GFavorANT", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
	/* 5G default is WF1 when enable SG SISO mode*/
	INIT_UINT(prWifiVar->ucSG5GFavorANT,
		"SG5GFavorANT", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif
	/* Tx Buffer Management */
	INIT_UINT(prWifiVar->ucExtraTxDone, "ExtraTxDone", 1,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucTxDbg, "TxDbg", 0, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->ucCmdDbg, "CmdDbg", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);

	if (!pucKey)
		kalMemZero(prWifiVar->au4TcPageCount,
			sizeof(prWifiVar->au4TcPageCount));

	INIT_UINT(prWifiVar->au4TcPageCount[TC0_INDEX],
		"Tc0Page", NIC_TX_PAGE_COUNT_TC0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC1_INDEX],
		"Tc1Page", NIC_TX_PAGE_COUNT_TC1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC2_INDEX],
		"Tc2Page", NIC_TX_PAGE_COUNT_TC2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC3_INDEX],
		"Tc3Page", NIC_TX_PAGE_COUNT_TC3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC4_INDEX],
		"Tc4Page", NIC_TX_PAGE_COUNT_TC4, FEATURE_TO_CUSTOMER);
#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
	INIT_UINT(prWifiVar->au4TcPageCount[TC5_INDEX],
		"Tc5Page", NIC_TX_PAGE_COUNT_TC0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC6_INDEX],
		"Tc6Page", NIC_TX_PAGE_COUNT_TC1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC7_INDEX],
		"Tc7Page", NIC_TX_PAGE_COUNT_TC2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC8_INDEX],
		"Tc8Page", NIC_TX_PAGE_COUNT_TC3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC9_INDEX],
		"Tc9Page", NIC_TX_PAGE_COUNT_TC0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC10_INDEX],
		"Tc10Page", NIC_TX_PAGE_COUNT_TC1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC11_INDEX],
		"Tc11Page", NIC_TX_PAGE_COUNT_TC2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC12_INDEX],
		"Tc12Page", NIC_TX_PAGE_COUNT_TC3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4TcPageCount[TC13_INDEX],
		"Tc13Page", NIC_TX_PAGE_COUNT_TC1, FEATURE_TO_CUSTOMER);
#endif

	INIT_UINT(prWifiVar->ucTxMsduQueueInit, "NicTxMsduQueue", 0,
		  FEATURE_DEBUG_ONLY);
	prWifiVar->ucTxMsduQueue = prWifiVar->ucTxMsduQueueInit;

	/* 1 resource for AC_BK(TC0_INDEX), AC_BE(TC1_INDEX) */
	/* 2 resource for AC_VI(TC2_INDEX) */
	/* 4 resource for AC_VO(TC3_INDEX) */
	/* 1 resource for MGMT(TC4_INDEX) & TC_NUM */
	INIT_UINT(prWifiVar->u4TxHifRes, "TxHifResCtl", 0x00114211,
		  FEATURE_DEBUG_ONLY);
	u4TxHifRes = prWifiVar->u4TxHifRes;
	for (u4Idx = 0; u4Idx < TC_NUM && u4TxHifRes; u4Idx++) {
		prAdapter->au4TxHifResCtl[u4Idx] = u4TxHifRes & BITS(0, 3);
		u4TxHifRes = u4TxHifRes >> 4;
	}

#if QM_ADAPTIVE_TC_RESOURCE_CTRL
	INIT_UINT(prQM->au4MinReservedTcResource[TC0_INDEX],
		"Tc0MinRsv", QM_MIN_RESERVED_TC0_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4MinReservedTcResource[TC1_INDEX],
		"Tc1MinRsv", QM_MIN_RESERVED_TC1_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4MinReservedTcResource[TC2_INDEX],
		"Tc2MinRsv", QM_MIN_RESERVED_TC2_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4MinReservedTcResource[TC3_INDEX],
		"Tc3MinRsv", QM_MIN_RESERVED_TC3_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4MinReservedTcResource[TC4_INDEX],
		"Tc4MinRsv", QM_MIN_RESERVED_TC4_RESOURCE, FEATURE_TO_CUSTOMER);

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
#define __STR_MIN_RESERVED(_idx_) STR_HELPER(Tc##_idx_##MinRsv)
#define CONF_QM_TC_MIN_RESERVED_TEMPLATE(_idx_)\
	INIT_UINT(prQM->au4MinReservedTcResource[TC##_idx_##_INDEX], \
		__STR_MIN_RESERVED(_idx_),\
		QM_MIN_RESERVED_TC##_idx_##_RESOURCE, FEATURE_TO_CUSTOMER)
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(5);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(6);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(7);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(8);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(9);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(10);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(11);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(12);
	CONF_QM_TC_MIN_RESERVED_TEMPLATE(13);
#undef CONF_QM_TC_MIN_RESERVED_TEMPLATE
#undef __STR_MIN_RESERVED
#endif

	INIT_UINT(prQM->au4GuaranteedTcResource[TC0_INDEX],
		"Tc0Grt", QM_GUARANTEED_TC0_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4GuaranteedTcResource[TC1_INDEX],
		"Tc1Grt", QM_GUARANTEED_TC1_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4GuaranteedTcResource[TC2_INDEX],
		"Tc2Grt", QM_GUARANTEED_TC2_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4GuaranteedTcResource[TC3_INDEX],
		"Tc3Grt", QM_GUARANTEED_TC3_RESOURCE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prQM->au4GuaranteedTcResource[TC4_INDEX],
		"Tc4Grt", QM_GUARANTEED_TC4_RESOURCE, FEATURE_TO_CUSTOMER);

#if (CFG_TX_RSRC_WMM_ENHANCE == 1)
#define __STR_GUARANTEED(_idx_) STR_HELPER(Tc##_idx_##Grt)
#define CONF_QM_TC_GUARANTEED_TEMPLATE(_idx_)\
	INIT_UINT(prQM->au4GuaranteedTcResource[TC##_idx_##_INDEX], \
		__STR_GUARANTEED(_idx_),\
		QM_GUARANTEED_TC##_idx_##_RESOURCE, FEATURE_TO_CUSTOMER)

	CONF_QM_TC_GUARANTEED_TEMPLATE(5);
	CONF_QM_TC_GUARANTEED_TEMPLATE(6);
	CONF_QM_TC_GUARANTEED_TEMPLATE(7);
	CONF_QM_TC_GUARANTEED_TEMPLATE(8);
	CONF_QM_TC_GUARANTEED_TEMPLATE(9);
	CONF_QM_TC_GUARANTEED_TEMPLATE(10);
	CONF_QM_TC_GUARANTEED_TEMPLATE(11);
	CONF_QM_TC_GUARANTEED_TEMPLATE(12);
	CONF_QM_TC_GUARANTEED_TEMPLATE(13);
#undef CONF_QM_TC_GUARANTEED_TEMPLATE
#undef __STR_MIN_RESERVED
#endif

	INIT_UINT(prQM->u4TimeToAdjustTcResource, "TcAdjustTime",
		  QM_INIT_TIME_TO_ADJUST_TC_RSC, FEATURE_DEBUG_ONLY);
	INIT_UINT(prQM->u4TimeToUpdateQueLen, "QueLenUpdateTime",
		  QM_INIT_TIME_TO_UPDATE_QUE_LEN, FEATURE_DEBUG_ONLY);
	INIT_UINT(prQM->u4QueLenMovingAverage, "QueLenMovingAvg",
		  QM_QUE_LEN_MOVING_AVE_FACTOR, FEATURE_DEBUG_ONLY);
	INIT_UINT(prQM->u4ExtraReservedTcResource, "TcExtraRsv",
		  QM_EXTRA_RESERVED_RESOURCE_WHEN_BUSY, FEATURE_DEBUG_ONLY);
#endif

	/* Stats log */
	INIT_UINT(prWifiVar->u4StatsLogTimeout,
		"StatsLogTO", WLAN_TX_STATS_LOG_TIMEOUT, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4StatsLogDuration,
		"StatsLogDur", WLAN_TX_STATS_LOG_DURATION, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->ucDhcpTxDone, "DhcpTxDone", 1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucArpTxDone, "ArpTxDone", 1, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->ucMacAddrOverride, "MacOverride", 0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucP2pMacAddrOverride, "P2pMacOverride", 0,
		  FEATURE_DEBUG_ONLY);
	INIT_STR(prWifiVar->aucMacAddrStr, "MacAddr", "00:0c:e7:66:32:e1",
		 FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucCtiaMode, "CtiaMode", 0, FEATURE_DEBUG_ONLY);

	/* Combine ucTpTestMode and ucSigmaTestMode in one flag */
	/* ucTpTestMode == 0, for normal driver */
	/* ucTpTestMode == 1, for pure throughput test mode (ex: RvR) */
	/* ucTpTestMode == 2, for sigma TGn/TGac/PMF */
	/* ucTpTestMode == 3, for sigma WMM PS */
	INIT_UINT(prWifiVar->ucTpTestMode, "TpTestMode", 0, FEATURE_DEBUG_ONLY);
#if CFG_SUPPORT_TPENHANCE_MODE
	/* tp enhance config */
	INIT_UINT(prWifiVar->ucTpEnhanceEnable, "TpEnhanceEnable", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTpEnhancePktNum, "TpEnhancePktNum", 20,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TpEnhanceInterval, "TpEnhanceInterval", 6000,
		  FEATURE_TO_CUSTOMER);
	INIT_INT(prWifiVar->cTpEnhanceRSSI, "TpEnhanceRSSI", -65,
		 FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TpEnhanceThreshold, "TpEnhanceThreshold", 300,
		  FEATURE_TO_CUSTOMER);
#endif /* CFG_SUPPORT_TPENHANCE_MODE */

#if CFG_SUPPORT_DBDC
	INIT_UINT(prWifiVar->eDbdcMode, "DbdcMode", DEFAULT_DBDC_MODE,
		  FEATURE_TO_CUSTOMER);

	/* ucDbdcOMFrame == 1, for OMI only */
	/* ucDbdcOMFrame == 2, for OMN only */
	/* ucDbdcOMFrame == 3, for OMI + OMN */
	INIT_UINT(prWifiVar->ucDbdcOMFrame, "DbdcOMFrame", 3,
		  FEATURE_TO_CUSTOMER);
#else
	prWifiVar->eDbdcMode = ENUM_DBDC_MODE_DISABLED;
	prWifiVar->fgDbDcModeEn = false;
#endif /*CFG_SUPPORT_DBDC*/

	INIT_UINT(prWifiVar->ucCsaDeauthClient,
		"CsaDeauthClient", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucCsaDoneTimeout,
		"CsaDoneTimeout", 10, FEATURE_DEBUG_ONLY);

#if (CFG_EFUSE_BUFFER_MODE_DELAY_CAL == 1)
	INIT_UINT(prWifiVar->ucEfuseBufferModeCal, "EfuseBufferModeCal", 0,
		  FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->ucCalTimingCtrl, "CalTimingCtrl", 0,
		  FEATURE_TO_CUSTOMER); /* power on full cal */
	INIT_UINT(prWifiVar->ucWow, "Wow", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAdvPws, "AdvPws", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucWowOnMdtim, "WowOnMdtim", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucWowOffMdtim, "WowOffMdtim", 3,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucEapolSuspendOffload,
		"EapolSuspendOffload", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);

#if CFG_WOW_SUPPORT
	INIT_UINT(prAdapter->rWowCtrl.fgWowEnable,
		"WowEnable", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.ucScenarioId,
		"WowScenarioId", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.ucBlockCount, "WowPinCnt", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.astWakeHif[0].ucWakeupHif,
		"WowHif", ENUM_HIF_TYPE_GPIO, FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.astWakeHif[0].ucGpioPin,
		"WowGpioPin", 0xFF, FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.astWakeHif[0].ucTriggerLvl,
		"WowTriigerLevel", 3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prAdapter->rWowCtrl.astWakeHif[0].u4GpioInterval,
		"GpioInterval", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucWowDetectType,
		"WowDetectType", WOWLAN_DETECT_TYPE_MAGIC, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2WowDetectTypeExt, "WowDetectTypeExt",
		  WOWLAN_DETECT_TYPE_EXT_PORT, FEATURE_TO_CUSTOMER);
#endif

	INIT_UINT(prWifiVar->u4TxHangFullDumpMode, "TxHangFullDumpMode", 0,
		  FEATURE_DEBUG_ONLY);

	/* SW Test Mode: Mainly used for Sigma */
	INIT_UINT(prWifiVar->u4SwTestMode,
		"SwTestMode", ENUM_SW_TEST_MODE_NONE, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucCtrlFlagAssertPath,
		"AssertPath", DBG_ASSERT_PATH_DEFAULT, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucCtrlFlagDebugLevel, "AssertLevel",
		  DBG_ASSERT_CTRL_LEVEL_DEFAULT, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4ScanCtrl,
		"ScanCtrl", SCN_CTRL_DEFAULT_SCAN_CTRL, FEATURE_DEBUG_ONLY);

	/* Wake lock related configuration */
	INIT_UINT(prWifiVar->u4WakeLockRxTimeout,
		"WakeLockRxTO", WAKE_LOCK_RX_TIMEOUT, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4WakeLockThreadWakeup, "WakeLockThreadTO",
		  WAKE_LOCK_THREAD_WAKEUP_TIMEOUT, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucSmartRTS, "SmartRTS", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ePowerMode, "PowerSave", Param_PowerModeMax,
		  FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->u2ListenInterval,
		"ListenInterval", INVALID_LISTEN_INTERVAL, FEATURE_DEBUG_ONLY);

	/* add more cfg from RegInfo */
	INIT_UINT(prWifiVar->u4UapsdAcBmp, "UapsdAcBmp", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4MaxSpLen, "MaxSpLen", 0, FEATURE_TO_CUSTOMER);
#if CFG_P2P_UAPSD_SUPPORT
	INIT_UINT(prWifiVar->u4P2pUapsdAcBmp, "P2pUapsdAcBmp", PM_UAPSD_ALL,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4P2pMaxSpLen,
		"P2pMaxSpLen", WMM_MAX_SP_LENGTH_2, FEATURE_DEBUG_ONLY);
#else
	INIT_UINT(prWifiVar->u4P2pUapsdAcBmp, "P2pUapsdAcBmp", PM_UAPSD_NONE,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4P2pMaxSpLen, "P2pMaxSpLen", 0,
		  FEATURE_DEBUG_ONLY);
#endif
	INIT_UINT(prWifiVar->fgDisOnlineScan, "DisOnlineScan", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisBcnLostDetection, "DisBcnLostDetection", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisAgingLostDetection,
		"DisAgingLostDetection", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisForceSCC, "DisForceSCC", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisRoaming, "DisRoaming", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisGTKCipherCheck, "DisGTKCipherCheck", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgDisSecurityCheck, "DisSecurityCheck", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAllowBtmReqMode, "AllowBtmReqMode", 0xff,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RejectBtmReqReason, "RejectBtmReqReason", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgRoamByBTO, "RoamByBTO", 0, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4AisRoamingNumber,
		"AisRoamingNumber", KAL_AIS_NUM, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnArpFilter, "EnArpFilter", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);

	/* Driver Flow Control Dequeue Quota. Now is only used by DBDC */
	INIT_UINT(prWifiVar->uDeQuePercentEnable, "DeQuePercentEnable", 1,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DeQuePercentVHT80Nss1, "DeQuePercentVHT80NSS1",
		  QM_DEQUE_PERCENT_VHT80_NSS1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DeQuePercentVHT40Nss1, "DeQuePercentVHT40NSS1",
		  QM_DEQUE_PERCENT_VHT40_NSS1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DeQuePercentVHT20Nss1, "DeQuePercentVHT20NSS1",
		  QM_DEQUE_PERCENT_VHT20_NSS1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DeQuePercentHT40Nss1, "DeQuePercentHT40NSS1",
		  QM_DEQUE_PERCENT_HT40_NSS1, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4DeQuePercentHT20Nss1, "DeQuePercentHT20NSS1",
		  QM_DEQUE_PERCENT_HT20_NSS1, FEATURE_DEBUG_ONLY);

	/* Support TDLS 5.5.4.2 optional case */
	INIT_UINT(prWifiVar->fgTdlsBufferSTASleep,
		"TdlsBufferSTASleep", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);

#if CFG_PERF_MON_FAST
	INIT_UINT(prWifiVar->u4PerfMonUpdatePeriod, "PerfMonPeriod",
		PERF_MON_UPDATE_MIN_INTERVAL, FEATURE_DEBUG_ONLY);

	if (prWifiVar->u4PerfMonUpdatePeriod < PERF_MON_UPDATE_MIN_INTERVAL) {
		prWifiVar->u4PerfMonUpdatePeriod = PERF_MON_UPDATE_MIN_INTERVAL;
		DBGLOG(INIT, TRACE, "u4PerfMonUpdatePeriod set to min(%d).\n",
			PERF_MON_UPDATE_MIN_INTERVAL);
	}
#else /* CFG_PERF_MON_FAST */
	INIT_UINT(prWifiVar->u4PerfMonUpdatePeriod, "PerfMonPeriod",
		PERF_MON_UPDATE_INTERVAL, FEATURE_DEBUG_ONLY);
#endif /* CFG_PERF_MON_FAST */

	INIT_UINT(prWifiVar->u4PerfMonTpTh[0], "PerfMonLv1", 20,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[1], "PerfMonLv2", 50,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[2], "PerfMonLv3", 100,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[3], "PerfMonLv4", 180,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[4], "PerfMonLv5", 250,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[5], "PerfMonLv6", 300,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[6], "PerfMonLv7", 700,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[7], "PerfMonLv8", 1200,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[8], "PerfMonLv9", 2000,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[9], "PerfMonLv10", 3000,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[10], "PerfMonLv11", 4000,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpTh[11], "PerfMonLv12", 5000,
		  FEATURE_TO_CUSTOMER);

#if CFG_SUPPORT_RX_NAPI
	/* unit: s */
	INIT_UINT(prWifiVar->u4NapiScheduleTimeout, "NapiScheduleTimeout", 60,
		  FEATURE_TO_CUSTOMER);
#endif

#if CFG_NAPI_DELAY
	/* unit: Mbps */
	INIT_UINT(prWifiVar->u4NapiDelayTputTh, "NapiDelayTputTh", 200,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4NapiDelayCntTh, "NapiDelayCntTh",
		NAPI_POLL_WEIGHT, FEATURE_TO_CUSTOMER);
	/* unit: ms */
	INIT_UINT(prWifiVar->u4NapiDelayTimeout, "NapiDelayTimeout", 1,
		  FEATURE_TO_CUSTOMER);
#endif /* CFG_NAPI_DELAY */

#if CFG_DYNAMIC_RFB_ADJUSTMENT
	INIT_UINT(prWifiVar->u4RfbBoostTpTh[0], "RfbBoostTpTh0", 50,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RfbBoostTpTh[1], "RfbBoostTpTh1", 300,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RfbBoostTpTh[2], "RfbBoostTpTh2", 1200,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RfbUnUseCnt[0], "RfbUnUseCnt0",
			(CFG_RX_MAX_PKT_NUM >> 2) * 3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RfbUnUseCnt[1], "RfbUnUseCnt1",
			(CFG_RX_MAX_PKT_NUM >> 1), FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4RfbUnUseCnt[2], "RfbUnUseCnt2", 0,
		  FEATURE_TO_CUSTOMER);
	/* just set it, not need to adjust it immediately */
	nicRxSetUnUseCnt(prAdapter, prWifiVar->u4RfbUnUseCnt[0], FALSE);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */

#if CFG_SUPPORT_MCC_BOOST_CPU
	INIT_UINT(prWifiVar->u4MccBoostTputLvTh, "MccBoostTputLvTh",
		MCC_BOOST_LEVEL, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4MccBoostPresentTime, "MccBoostPresentTimeMin",
		  MCC_BOOST_MIN_TIME, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4MccBoostForAllTputLvTh, "MccBoostForAllTputLvTh",
		  MCC_BOOST_FOR_ALL_LEVEL, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_MCC_BOOST_CPU */

#if CFG_SUPPORT_SKB_ALLOC_WORK
	INIT_UINT(prWifiVar->fgSkbAllocWorkEn, "SkbAllocWorkEn",
			FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4SkbAllocScheduleTh, "SkbAllocScheduleTh",
		  256, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_SKB_ALLOC_WORK */

#if CFG_SUPPORT_TX_FREE_SKB_WORK
	INIT_UINT(prWifiVar->fgTxFreeSkbWorkEn, "TxFreeSkbWorkEn",
			FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_TX_FREE_SKB_WORK */

#if CFG_SUPPORT_LLS
	INIT_UINT(prWifiVar->fgLinkStatsDump, "LinkStatsDump", 0,
		  FEATURE_DEBUG_ONLY);

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	INIT_UINT(prWifiVar->u4LlsStatsCmdPeriod, "LinkStatsCmdPeriod",
		  CFG_STATS_ONE_CMD_PERIOD, FEATURE_TO_CUSTOMER);
#endif
#endif

#if CFG_SUPPORT_TX_LATENCY_STATS
	INIT_UINT(prWifiVar->fgPacketLatencyLog, "TxLatencyPacketLog", 0,
		  FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->fgTxLatencyKeepCounting,
		"TxLatencyKeepCounting", 0, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->fgTxLatencyPerBss, "TxLatencyPerBss", 0,
		  FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->u4MsduStatsUpdateInterval,
		  "TxLatencyUpdateInterval", TX_LATENCY_STATS_UPDATE_INTERVAL,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4ContinuousTxFailThreshold,
		"TxLatencyContinuousFailThrehold",
		TX_LATENCY_STATS_CONTINUOUS_FAIL_THREHOLD, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->au4DriverTxDelayMax[0],
		"TxLatencyDriverDelayMaxL1",
		TX_LATENCY_STATS_MAX_DRIVER_DELAY_L1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverTxDelayMax[1],
		"TxLatencyDriverDelayMaxL2",
		TX_LATENCY_STATS_MAX_DRIVER_DELAY_L2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverTxDelayMax[2],
		"TxLatencyDriverDelayMaxL3",
		TX_LATENCY_STATS_MAX_DRIVER_DELAY_L3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverTxDelayMax[3],
		"TxLatencyDriverDelayMaxL4",
		TX_LATENCY_STATS_MAX_DRIVER_DELAY_L4, FEATURE_TO_CUSTOMER);
	prWifiVar->au4DriverTxDelayMax[4] = UINT_MAX;

	INIT_UINT(prWifiVar->au4DriverHifTxDelayMax[0],
		"TxLatencyDriver1DelayMaxL1",
		TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverHifTxDelayMax[1],
		"TxLatencyDriver1DelayMaxL2",
		TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverHifTxDelayMax[2],
		"TxLatencyDriver1DelayMaxL3",
		TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4DriverHifTxDelayMax[3],
		"TxLatencyDriver1DelayMaxL4",
		TX_LATENCY_STATS_MAX_DRIVER1_DELAY_L4, FEATURE_TO_CUSTOMER);
	prWifiVar->au4DriverHifTxDelayMax[4] = UINT_MAX;

	INIT_UINT(prWifiVar->au4ConnsysTxDelayMax[0],
		"TxLatencyConnsysDelayMaxL1",
		TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxDelayMax[1],
		"TxLatencyConnsysDelayMaxL2",
		TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxDelayMax[2],
		"TxLatencyConnsysDelayMaxL3",
		TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxDelayMax[3],
		"TxLatencyConnsysDelayMaxL4",
		TX_LATENCY_STATS_MAX_CONNSYS_DELAY_L4, FEATURE_TO_CUSTOMER);
	prWifiVar->au4ConnsysTxDelayMax[4] = UINT_MAX;

	INIT_UINT(prWifiVar->au4MacTxDelayMax[0],
		"TxLatencyMacDelayMaxL1",
		TX_LATENCY_STATS_MAX_MAC_DELAY_L1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4MacTxDelayMax[1],
		"TxLatencyMacDelayMaxL2",
		TX_LATENCY_STATS_MAX_MAC_DELAY_L2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4MacTxDelayMax[2],
		"TxLatencyMacDelayMaxL3",
		TX_LATENCY_STATS_MAX_MAC_DELAY_L3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4MacTxDelayMax[3],
		"TxLatencyMacDelayMaxL4",
		TX_LATENCY_STATS_MAX_MAC_DELAY_L4, FEATURE_TO_CUSTOMER);
	prWifiVar->au4MacTxDelayMax[4] = UINT_MAX;

	INIT_UINT(prWifiVar->au4AirTxDelayMax[0], "TxLatencyAirDelayMaxL1",
		  TX_LATENCY_STATS_MAX_AIR_DELAY_L1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4AirTxDelayMax[1], "TxLatencyAirDelayMaxL2",
		  TX_LATENCY_STATS_MAX_AIR_DELAY_L2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4AirTxDelayMax[2], "TxLatencyAirDelayMaxL3",
		  TX_LATENCY_STATS_MAX_AIR_DELAY_L3, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4AirTxDelayMax[3], "TxLatencyAirDelayMaxL4",
		  TX_LATENCY_STATS_MAX_AIR_DELAY_L4, FEATURE_TO_CUSTOMER);
	prWifiVar->au4AirTxDelayMax[4] = UINT_MAX;

	INIT_UINT(prWifiVar->au4ConnsysTxFailDelayMax[0],
		"TxLatencyFailConnsysDelayMaxL1",
		TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L1,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxFailDelayMax[1],
		"TxLatencyFailConnsysDelayMaxL2",
		TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L2,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxFailDelayMax[2],
		"TxLatencyFailConnsysDelayMaxL3",
		TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L3,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->au4ConnsysTxFailDelayMax[3],
		"TxLatencyFailConnsysDelayMaxL4",
		TX_LATENCY_STATS_MAX_FAIL_CONNSYS_DELAY_L4,
		FEATURE_TO_CUSTOMER);
	prWifiVar->au4ConnsysTxFailDelayMax[4] = UINT_MAX;
#endif /* CFG_SUPPORT_TX_LATENCY_STATS */

	INIT_UINT(prWifiVar->fgBoostCpuEn,  "BoostCpuEn", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgBoostCpuByPPSEn,  "BoostCpuByPPSEn",
		FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgBoostCpuPolicyEn,  "BoostCpuPolicyEn",
		FEATURE_ENABLED, FEATURE_DEBUG_ONLY);

	/* Boost Cpu Policy Options  */
	INIT_UINT(prWifiVar->ucBCPPerTh,  "BCPPerTh", 30, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4BCPBigCPUIncrementHz,
		"BCPBigIncrementHz", 200000, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4BCPBigCPUDecrementHz,
		"BCPBigDecrementHz", 200000, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4BCPLitCPUIncrementHz,
		"BCPLitIncrementHz", 200000, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4BCPLitCPUDecrementHz,
		"BCPLitDecrementHz", 200000, FEATURE_DEBUG_ONLY);
	u4PlatformBoostCpuTh = kalGetCpuBoostThreshold();
	INIT_UINT(prWifiVar->u4BoostCpuTh,
		"BoostCpuTh", u4PlatformBoostCpuTh, FEATURE_TO_CUSTOMER);
	prWifiVar->fgIsBoostCpuThAdjustable = FALSE;
	if (!wlanCfgGetEntry(prAdapter, "BoostCpuTh", FALSE)) {
		prWifiVar->fgIsBoostCpuThAdjustable  = TRUE;
		DBGLOG(INIT, TRACE, "BoostCPUTh is not config, adjustable\n");
	}
	INIT_UINT(prWifiVar->au4CpuBoostMinFreq, "CpuBoostMinFreq", 1300,
		  FEATURE_TO_CUSTOMER);

#if CFG_TX_CUSTOMIZE_LTO
	INIT_UINT(prWifiVar->ucEnableConfigLTO, "EnableConfigLTO",
		FEATURE_ENABLED, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->u4LTOValue, "LTOValue", 2000, FEATURE_TO_CUSTOMER);
#endif /* CFG_TX_CUSTOMIZE_LTO */

	INIT_UINT(prWifiVar->fgWarningTxTimeout,
		"WarningTxTimeout", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TxTimeoutWarningThr, "TxTimeoutWarningThr",
		  NIC_MSDU_REPORT_TIMEOUT_SER_TIME, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->fgIgnoreLowIdleSlot,
		"IgnoreLowIdleSlot", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4LowIdleSlotThr, "LowIdleSlotThr", 10000,
		  FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->u4SameTokenThr, "SameTokenThr", 11,
		  FEATURE_DEBUG_ONLY);

	/**
	 * A debugging switch for development phase to check the difference of
	 * tput imposed by SW reordering, including the operation workload and
	 * frame drops, etc.
	 */
	INIT_UINT(prWifiVar->fgSwRxReordering,
		"SwRxReordering", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	/**
	 * A debugging switch enables RXD, RXP dumping when driver drops packets
	 * for ICV error.
	 */
	INIT_UINT(prWifiVar->fgRxIcvErrDbg, "RxIcvErrDbg", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	/**
	 * Switching dumping TX/RX memory, set by bitmap format.
	 * TXP_FULL(0x08),       TXP(0x04), TXDMAD(0x02), TXD(0x01),
	 * RXEvent(0x80), RXDSEGMENT(0x40), RXDMAD(0x20), RXD(0x10).
	 */
	INIT_UINT(prWifiVar->u4TxRxDescDump, "TRXDescDump", 0x0,
		  FEATURE_DEBUG_ONLY);

#if CFG_DEBUG_RX_SEGMENT
	INIT_UINT(prWifiVar->fgRxSegmentDebugEn, "RxSegmentDebugEn",
		FEATURE_ENABLED, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->u4RxSegmentDebugTimeout,
		"RxSegmentDebugTimeout", RX_SEGMENT_DEBUG_TIMEOUT,
		FEATURE_DEBUG_ONLY);

	if (IS_FEATURE_ENABLED(prWifiVar->fgRxSegmentDebugEn)) {
		/* force enable it for better debugging */
		prWifiVar->fgDumpRxDsegment = 0x1;
		DBGLOG(INIT, TRACE, "Force Enable fgDumpRxDsegment\n");
	}
#endif /* CFG_DEBUG_RX_SEGMENT */

	DBGLOG(INIT, TRACE,
		"TxPfull,TxP,TxDmad,TxD/RxDsegment,RxDmad,RxD,RxEvt=%u,%u,%u,%u/%u,%u,%u,%u",
		prWifiVar->fgDumpTxPfull, prWifiVar->fgDumpTxP,
		prWifiVar->fgDumpTxDmad, prWifiVar->fgDumpTxD,
		prWifiVar->fgDumpRxDsegment, prWifiVar->fgDumpRxDmad,
		prWifiVar->fgDumpRxD, prWifiVar->fgDumpRxEvt);

#if CFG_SUPPORT_LOWLATENCY_MODE
	INIT_UINT(prWifiVar->u4BaShortMissTimeoutMs, "BaShortMissTimeoutMs",
		  QM_RX_BA_ENTRY_MISS_TIMEOUT_MS_SHORT, FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->u4BaMissTimeoutMs, "BaMissTimeoutMs",
		  QM_RX_BA_ENTRY_MISS_TIMEOUT_MS, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->u4PerfMonPendingTh, "PerfMonPendingTh", 80,
		  FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->u4PerfMonUsedTh, "PerfMonUsedTh", 80,
		  FEATURE_DEBUG_ONLY);

	/* SER related fields */

	/* for L0 SER */
	INIT_UINT(prWifiVar->fgEnableSerL0,
		"EnableSerL0", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);

	/* for L0.5 SER */
	INIT_UINT(prWifiVar->eEnableSerL0p5,
		"EnableSerL0p5", FEATURE_OPT_SER_ENABLE, FEATURE_TO_CUSTOMER);

	/* for L1 SER */
	INIT_UINT(prWifiVar->eEnableSerL1,
		"EnableSerL1", FEATURE_OPT_SER_ENABLE, FEATURE_TO_CUSTOMER);

	/* for L0 SER using WDT on some legacy CE USB project like MT7668 and
	 * MT7663. The difference between fgEnableSerL0 and fgChipResetRecover
	 * is that the former one toggles reset pin and the latter one sends
	 * USB EP0 vendor request to fw, which makes fw assert and triggers
	 * L0 reset when watchdog timeout. The purpose to reserve this mothod
	 * is that some customers might not have dedicated reset pin on the
	 * platform.
	 */
	INIT_UINT(prWifiVar->fgChipResetRecover,
		"ChipResetRecover", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);

	/*
	 * For Certification purpose,forcibly set
	 * "Compressed Steering Number of Beamformer Antennas Supported" to our
	 * own capability.
	 */
	INIT_UINT(prWifiVar->fgForceSTSNum, "ForceSTSNum", 0,
		  FEATURE_TO_CUSTOMER);
#if CFG_SUPPORT_IDC_CH_SWITCH
	INIT_UINT(prWifiVar->ucChannelSwtichColdownTime, "CSACdTime", 60,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgCrossBandSwitchEn, "SapCrossBandSwitchEn", 0,
		  FEATURE_TO_CUSTOMER);
#endif
#if CFG_SUPPORT_PERF_IND
	INIT_UINT(prWifiVar->fgPerfIndicatorEn, "PerfIndicatorEn", 1,
		  FEATURE_TO_CUSTOMER);
#endif
#if CFG_SUPPORT_SPE_IDX_CONTROL
	INIT_UINT(prWifiVar->ucSpeIdxCtrl, "SpeIdxCtrl", 2,
		  FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_LOWLATENCY_MODE
	INIT_UINT(prWifiVar->ucLowLatencyModeScan,
		"LowLatencyModeScan", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucLowLatencyModeReOrder,
		"LowLatencyModeReOrder", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucLowLatencyModePower,
		"LowLatencyModePower", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucLowLatencyPacketPriority,
		"LowLatencyPacketPriority", BITS(0, 1), FEATURE_TO_CUSTOMER);
#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

	INIT_UINT(prWifiVar->u4MTU, "MTU", 0, FEATURE_DEBUG_ONLY);

#if CFG_SUPPORT_RX_GRO
	INIT_UINT(prWifiVar->ucGROFlushTimeout, "GROFlushTimeout", 1,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucGROEnableTput, "GROEnableTput", 6250000,
		  FEATURE_DEBUG_ONLY);
#endif
	INIT_UINT(prWifiVar->u4MsduReportTimeout, "MsduReportTimeout",
		  NIC_MSDU_REPORT_DUMP_TIMEOUT, FEATURE_DEBUG_ONLY);
#if CFG_DISABLE_TXTIMEOUT_SER
	/* default not trigger SER during SQC */
	INIT_UINT(prWifiVar->u4MsduReportTimeoutSerTime,
		  "MsduReportTimeoutSerTime",
		  NIC_MSDU_REPORT_DISABLE_SER_TIME, FEATURE_DEBUG_ONLY);
#else
	INIT_UINT(prWifiVar->u4MsduReportTimeoutSerTime,
		  "MsduReportTimeoutSerTime",
		  NIC_MSDU_REPORT_TIMEOUT_SER_TIME, FEATURE_DEBUG_ONLY);
#endif

#if CFG_SUPPORT_DATA_STALL
	INIT_UINT(prWifiVar->u4PerHighThreshole, "PerHighThreshole",
		  EVENT_PER_HIGH_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TxLowRateThreshole, "TxLowRateThreshole",
		  EVENT_TX_LOW_RATE_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4RxLowRateThreshole, "RxLowRateThreshole",
		  EVENT_RX_LOW_RATE_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4ReportEventInterval, "ReportEventInterval",
		  REPORT_EVENT_INTERVAL, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TrafficThreshold,
		"TrafficThreshold", TRAFFIC_RHRESHOLD, FEATURE_DEBUG_ONLY);

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	INIT_UINT(prWifiVar->fgLowRateUevtEn, "LowRateEn",
		  FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4LowRateUevtIntv, "LowRateInterval",
		  LOW_RATE_MONITOR_INTERVAL, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4LowRateUevtTh, "LowRateTh",
		  LOW_RATE_MONITOR_THRESHOLD, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4LowRateUevtTputTh, "LowRateTputTh",
		  LOW_RATE_MONITOR_TPUT_THRESHOLD, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u8LowRateUevtMpduTh, "LowRateMpduTh",
		  LOW_RATE_MONITOR_MPDU_THRESHOLD, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4LowRateUevtReptIntv, "LowRateReptInterval",
		  LOW_RATE_MONITOR_EVENT_REPORT_INTERVAL, FEATURE_TO_CUSTOMER);
#endif
#endif

#if CFG_SUPPORT_HE_ER
	INIT_UINT(prWifiVar->u4ExtendedRange,
		"ExtendedRange", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgErTx,
		"ErTx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgErRx,
		"ErRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgErSuRx,
		"ErSuRx", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif
#if (CFG_SUPPORT_BSS_MAX_IDLE_PERIOD == 1)
	INIT_UINT(prWifiVar->fgBssMaxIdle,
		"BssMaxIdle", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2BssMaxIdlePeriod, "BssMaxIdlePeriod",
		  BSS_MAX_IDLE_PERIOD_VALUE, FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_NAN
	INIT_UINT(prWifiVar->ucNanMacAddrOverride, "NanMacOverride", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_STR(prWifiVar->aucNanMacAddrStr,
		"NanMacAddr", "00:0c:e7:11:22:33", FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->ucMasterPref, "NanMasterPref", 2,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucConfig5gChannel, "NanConfig5gChannel", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucChannel5gVal, "NanChannel5gVal", 149,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucAisQuotaVal, "NanAisQuota", 8,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDftNdlQuotaVal, "NanDftNdlQuota",
#if (CFG_NAN_SCHEDULER_VERSION == 1)
		NAN_DEFAULT_NDL_QUOTA_UP_BOUND, FEATURE_TO_CUSTOMER);
#else
		5, FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->ucDftRangQuotaVal, "NanDftRangQuota", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDftQuotaStartOffset,
		"NanDftQuotaStartOffset", NAN_FAW_OFFSET, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDftNdcStartOffset, "NanDftNdcStartOffset", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanFixChnl, "NanFixChnl", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableNDPE, "NanEnableNDPE", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2DftNdlQosLatencyVal, "NanDftNdlQosLatency", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDftNdlQosQuotaVal, "NanDftNdlQosQuota", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnNanVHT, "NanVHT", 1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanFtmBw,
		"NanFtmBw", FTM_FORMAT_BW_HT_MIXED_BW20, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanDiscBcnInterval, "NanDiscBcnInterval", 100,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanCommittedDw, "NanDftCommittedDw", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgNoPmf, "NanForceNoPmf", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgNanIsSigma, "NanIsSigma", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNan2gBandwidth, "Nan2gBw", MAX_BW_20MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNan5gBandwidth, "Nan5gBw", MAX_BW_80MHZ,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNdlFlowCtrlVer,
		"NanNdlFlowCtrlVer", CFG_SUPPORT_NAN_ADVANCE_DATA_CONTROL,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanMaxNdpSession, "NanMaxNdpSession",
		NAN_MAX_NDP_SESSIONS, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRandNdpid, "NanEnableRandNdpid", 1,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4NanSendPacketGuardTime,
		  "NanSendPacketGuardTime", NAN_SEND_PKT_TIME_GUARD_TIME,
		  FEATURE_TO_CUSTOMER);
	if (prWifiVar->ucNanFixChnl == 0) {
		INIT_UINT(prWifiVar->fgNanWmmSeq, "NanWmmSeq", 1,
			  FEATURE_TO_CUSTOMER);
	} else {
		INIT_UINT(prWifiVar->fgNanWmmSeq, "NanWmmSeq",
		(prWifiVar->ucNanFixChnl < 36) ? 0:1, FEATURE_TO_CUSTOMER);
	}
	INIT_UINT(prWifiVar->fgNanUnrollInstallTk, "NanUnrollInstallTk", 0,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanFixBand, "NanFixBand", BAND_2G4,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanEnable6g, "NanEnable6g", 1,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNanBandChnlType, "NanBandChnlType",
		NAN_BAND_CH_ENTRY_LIST_TYPE_CHNL,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucNan6gBandwidth, "Nan6gBw", MAX_BW_20MHZ,
		FEATURE_TO_CUSTOMER);
#endif

#ifdef CFG_REUSE_RSN_IE
	INIT_UINT(prWifiVar->fgReuseRSNIE, "ReuseRSNIE", (CFG_REUSE_RSN_IE),
		  FEATURE_DEBUG_ONLY);
#else
	INIT_UINT(prWifiVar->fgReuseRSNIE, "ReuseRSNIE", (FEATURE_DISABLED),
		  FEATURE_DEBUG_ONLY);
#endif

#if CFG_COALESCING_INTERRUPT
	INIT_UINT(prWifiVar->u2CoalescingIntMaxPk, "CoalescingIntMaxPkt",
		  COALESCING_INT_MAX_PKT, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u2CoalescingIntMaxTime, "CoalescingIntMaxTime",
		  COALESCING_INT_MAX_TIME, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2CoalescingIntuFilterMask,
		"CoalescingIntFilterMask",
		CMD_PF_CF_COALESCING_INT_FILTER_MASK_DEFAULT,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PerfMonTpCoalescingIntTh,
		"PerfMonTpCoalescingIntTh", 6, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgCoalescingIntEn,
		"CoalescingIntEn", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_ROAMING
	INIT_UINT(prWifiVar->u4DiscoverTimeout, "DiscoverTimeout",
		  ROAMING_DISCOVER_TIMEOUT_SEC, FEATURE_TO_CUSTOMER);
#endif

#if (CFG_MTK_MDDP_SUPPORT == 0) || (CFG_SUPPORT_MDDP_DYNAMIC_DISABLE == 1)
	INIT_UINT(prWifiVar->fgMddpSupport, "MddpSupport", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->fgMddpSupport, "MddpSupport", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#endif
	wlanCfgSetUint32(prAdapter, "MddpSupport", prWifiVar->fgMddpSupport);

#if (CFG_DBDC_SW_FOR_P2P_LISTEN == 1)
	INIT_UINT(prWifiVar->ucDbdcP2pLisEn,
		"DbdcP2pLisEn", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->u4DbdcP2pLisSwDelayTime, "DbdcP2pLisSwDelayTime",
		  DBDC_P2P_LISTEN_SW_DELAY_TIME, FEATURE_TO_CUSTOMER);
#endif

	INIT_UINT(prWifiVar->ucDisallowBand2G, "DisallowBand2G", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDisallowBand5G, "DisallowBand5G", 0,
		  FEATURE_TO_CUSTOMER);
#if (CFG_SUPPORT_WIFI_6G == 1)
	INIT_UINT(prWifiVar->ucDisallowBand6G, "DisallowBand6G", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucDisallowAcs6G,
		"DisallowAcs6G", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_ROAMING
	INIT_UINT(prWifiVar->u4InactiveTimeout, "InactiveTimeout",
		  ROAMING_INACTIVE_TIMEOUT_SEC, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4BtmDelta, "BtmDelta", ROAMING_BTM_DELTA,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4BtmDisThreshold, "BtmDisThreshold",
		  AIS_BTM_DIS_IMMI_TIMEOUT, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4BtmTimerThreshold, "BtmTimerThreshold",
		  AIS_BTM_TIMER_THRESHOLD, FEATURE_TO_CUSTOMER);
#endif

#if ARP_MONITER_ENABLE
	INIT_UINT(prWifiVar->uArpMonitorNumber, "ArpMonitorNumber", 5,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->uArpMonitorRxPktNum, "ArpMonitorRxPktNum", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->uArpMonitorCriticalThres,
			"ArpMonitorCriticalThres", 2, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucArpMonitorUseRule, "ArpMonitorUseRule", 1,
		  FEATURE_TO_CUSTOMER);
#endif /* ARP_MONITER_ENABLE */


#if CFG_RFB_TRACK
	INIT_UINT(prWifiVar->fgRfbTrackEn, "RfbTrackEn", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	/* unit: second */
	INIT_UINT(prWifiVar->u4RfbTrackInterval,
		"RfbTrackInterval", RFB_TRACK_INTERVAL, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4RfbTrackTimeout,
		"RfbTrackTimeout", RFB_TRACK_TIMEOUT, FEATURE_DEBUG_ONLY);
#endif /* CFG_RFB_TRACK */

#if CFG_SUPPORT_SCAN_NO_AP_RECOVERY
	INIT_UINT(prWifiVar->ucScanNoApRecover,
		"ScanNoApRecover", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucScanNoApRecoverTh, "ucScanNoApRecoverTh", 3,
		  FEATURE_TO_CUSTOMER);
#endif

#if CFG_ENABLE_WIFI_DIRECT
	INIT_UINT(prWifiVar->fgSapCheckPmkidInDriver,
		"SapCheckPmkidInDriver", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapOffload, "SapOffload", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapGoSkipObss,
		"SapGoSkipObss", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgP2pGcCsa, "P2pGcCsa", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgSkipP2pIe, "SkipP2pIe", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSkipP2pProbeResp,
		"SkipP2pProbeResp", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapChannelSwitchPolicy, "SapChannelSwitchPolicy",
		  P2P_CHANNEL_SWITCH_POLICY_SCC, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapConcurrencyPolicy,
		"SapConcurrencyPolicy", P2P_CONCURRENCY_POLICY_REMOVE,
		FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapAuthPolicy,
		"SapAuthPolicy", P2P_AUTH_POLICY_NONE, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgSapOverwriteAcsChnlBw,
		"SapOverwriteAcsChnlBw", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);

	INIT_UINT(prWifiVar->fgSapAddTPEIE, "SapAddTPEIE", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
#endif
	INIT_UINT(prWifiVar->ucDfsRegion, "DfsRegion", 0, FEATURE_TO_CUSTOMER);
	if (prWifiVar->ucDfsRegion)
		rlmDomainSetDfsRegion(prWifiVar->ucDfsRegion);

#if (CFG_SUPPORT_DFS_MASTER == 1)
	INIT_UINT(prWifiVar->u4ByPassCacTime, "ByPassCacTime", 0,
		  FEATURE_TO_CUSTOMER);
	if (prWifiVar->u4ByPassCacTime) {
		p2pFuncEnableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	} else {
		p2pFuncDisableManualCac();
		p2pFuncSetDriverCacTime(prWifiVar->u4ByPassCacTime);
	}
#endif
	INIT_UINT(prWifiVar->u4CC2Region, "CC2Region", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);

#if CFG_ENABLE_WIFI_DIRECT
	INIT_UINT(prWifiVar->u4ApChnlHoldTime,
		"ApChnlHoldTime", SAP_CHNL_HOLD_TIME_MS, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4P2pChnlHoldTime,
		"P2pChnlHoldTime", P2P_CHNL_HOLD_TIME_MS, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucProbeRspRetryLimit, "ProbeRspRetryLimit",
		  DEFAULT_P2P_PROBERESP_RETRY_LIMIT, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucP2pMgmtTxRetryLimit,
		"P2pMgmtTxRetryLimit", 0, FEATURE_DEBUG_ONLY);
#endif
	INIT_UINT(prWifiVar->fgAllowSameBandDualSta,
		"AllowSameBandDualSta", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);

#if CFG_SUPPORT_TDLS_AUTO
	INIT_UINT(prWifiVar->u4TdlsAuto,
		"TdlsAuto",
		(CFG_TC10_FEATURE) ? TDLS_AUTO_ALL : TDLS_AUTO_NONE,
		FEATURE_DEBUG_ONLY);
#endif

#if CFG_MODIFY_TX_POWER_BY_BAT_VOLT
	INIT_UINT(prWifiVar->u4BackoffLevel, "BackoffLevel", 0,
		  FEATURE_TO_CUSTOMER);
#endif

#if (CFG_SUPPORT_APF == 1)
	INIT_UINT(prWifiVar->ucApfEnable, "ApfEnable", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_DISABLE_DATA_DDONE_INTR
	INIT_UINT(prWifiVar->u4TputThresholdMbps, "TputThresholdMbps", 50,
		  FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_DISABLE_DATA_DDONE_INTR */

	INIT_UINT(prWifiVar->u4TxHighTputTh, "TxHighTputTh", 4000,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4RxHighTputTh, "RxHighTputTh", 2000,
		  FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->u4RxRateProtoFilterMask, "RxRateProtoFilterMask",
			BIT(ENUM_PKT_ARP), FEATURE_TO_CUSTOMER);

#if CFG_SUPPORT_BAR_DELAY_INDICATION
	INIT_UINT(prWifiVar->fgBARDelayIndicationEn,
		"BARDelayIndicationEn", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_BAR_DELAY_INDICATION */

#if CFG_SUPPORT_DHCP_RESET_BA_WINDOW
	INIT_UINT(prWifiVar->fgDhcpResetBaWindow,
		"DhcpResetBaWindow", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_DHCP_RESET_BA_WINDOW */

#if CFG_SUPPORT_LIMITED_PKT_PID
	INIT_UINT(prWifiVar->u4PktPIDTimeout, "PktPIDTimeout", 1000,
		  FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */
#if CFG_SUPPORT_ICS_TIMESYNC
	INIT_UINT(prWifiVar->u4IcsTimeSyncCnt, "IcsTimeSyncCnt", 1000,
		  FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_ICS_TIMESYNC */
#if (CFG_SUPPORT_WIFI_6G == 1)
	INIT_UINT(prWifiVar->fgEnOnlyScan6g,
		"EnableOnlyScan6g", FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#endif /* CFG_SUPPORT_LIMITED_PKT_PID */

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	INIT_INT(prWifiVar->i4Ed2GNonEU, "Ed2GNonEU", ED_CCA_BW20_2G_DEFAULT,
		 FEATURE_DEBUG_ONLY);
	INIT_INT(prWifiVar->i4Ed5GNonEU, "Ed5GNonEU", ED_CCA_BW20_5G_DEFAULT,
		 FEATURE_DEBUG_ONLY);
	INIT_INT(prWifiVar->i4Ed2GEU, "Ed2GEU", ED_CCA_BW20_2G_DEFAULT,
		 FEATURE_DEBUG_ONLY);
	INIT_INT(prWifiVar->i4Ed5GEU, "Ed5GEU", ED_CCA_BW20_5G_DEFAULT,
		 FEATURE_DEBUG_ONLY);
#endif

#if CFG_MTK_FPGA_PLATFORM
	INIT_UINT(prWifiVar->u4FpgaSpeedFactor,	"FpgaSpeedFactor", 0,
		  FEATURE_DEBUG_ONLY);
#endif
#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
	INIT_UINT(prWifiVar->fgEnableMawd, "EnableMawd", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableMawdTx, "EnableMawdTx", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgEnableSdo, "EnableSdo", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRro, "EnableRro", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRro2Md, "EnableRro2Md", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRroPreFillRxRing, "EnableRroPreFillRxRing",
		  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRroDbg, "EnableRroDbg", FEATURE_DISABLED,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnableRroAdvDump,
		  "EnableRroAdvDump", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgEnableMawdSramDump,
		  "EnableMawdSramDump", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);

	if (IS_FEATURE_FORCE_ENABLED(prWifiVar->fgEnableMawd))
		prWifiVar->fgEnableMawd = FEATURE_ENABLED;
	else if (!prChipInfo->is_support_mawd || !kalIsSupportMawd())
		prWifiVar->fgEnableMawd = FEATURE_DISABLED;

	if (IS_FEATURE_FORCE_ENABLED(prWifiVar->fgEnableSdo))
		prWifiVar->fgEnableSdo = FEATURE_ENABLED;
	else if (!prChipInfo->is_support_sdo || !kalIsSupportSdo())
		prWifiVar->fgEnableSdo = FEATURE_DISABLED;

	if (IS_FEATURE_FORCE_ENABLED(prWifiVar->fgEnableRro))
		prWifiVar->fgEnableRro = FEATURE_ENABLED;
	else if (!prChipInfo->is_support_rro || !kalIsSupportRro())
		prWifiVar->fgEnableRro = FEATURE_DISABLED;
	wlanCfgSetUint32(prAdapter, "EnableRro", prWifiVar->fgEnableRro);
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

	INIT_UINT(prWifiVar->u4WfdmaRxHangRecoveryCnt,
		  "WfdmaRxHangRecoveryCnt", 3,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4WfdmaRxHangCnt, "WfdmaRxHangCnt", 5,
		  FEATURE_DEBUG_ONLY);

#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
	for (u4Idx = 0; u4Idx < MAX_BSSID_NUM; u4Idx++) {
		INIT_INT(prWifiVar->i4BssCount[u4Idx], "wfdSccBalanceBssCount",
			 0, FEATURE_TO_CUSTOMER);
	}
	INIT_UINT(prWifiVar->u4WfdSccBalanceMode,
			"wfdSccBalanceMode", 0, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4WfdSccBalanceRatio,
			"wfdSccBalanceRatio", 10, FEATURE_DEBUG_ONLY);
#if (CFG_WFD_SCC_BALANCE_DEF_ENABLE == 1)
	INIT_UINT(prWifiVar->u4WfdSccBalanceEnable, "wfdSccBalanceEnable",
		  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->u4WfdSccBalanceEnable, "wfdSccBalanceEnable",
		  FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#endif
#endif
	INIT_UINT(prWifiVar->fgIcmpTxDone, "IcmpTxDone", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);

	/* Fast Path Config */
	INIT_UINT(prWifiVar->ucUdpTspecUp, "UdpTspecUp", 7, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucTcpTspecUp, "TcpTspecUp", 5, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4UdpDelayBound, "UdpDelayBound", 7000,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TcpDelayBound, "TcpDelayBound", 10000,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucDataRate, "TspecDataRate", 0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucSupportProtocol, "SupportProtocol", 0,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucCheckBeacon,
		"MscsCheckBeacon", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucEnableFastPath,
		"EnableFastPath", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucFastPathAllPacket,
		"FastPathAllPacket", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
#if (CFG_TX_HIF_PORT_QUEUE == 1)
	INIT_UINT(prWifiVar->ucEnableTxHifPortQ,
		"EnableTxHifPortQ", FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
#endif

#if (CFG_VOLT_INFO == 1)
	INIT_UINT(prWifiVar->fgVnfEn, "VoltInfoEnable",
		kalVnfGetEnInitStatus(), FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4VnfDebTimes, "VoltInfoDebTimes",
		  VOLT_INFO_DEBOUNCE_TIMES, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4VnfDebInterval, "VoltInfoDebInterval",
		  VOLT_INFO_DEBOUNCE_INTERVAL, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4VnfDelta,
		"VoltInfoDelta", VOLT_INFO_DELTA, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4VnfLowBnd,
		"VoltInfoLowBound", kalVnfGetVoltLowBnd(), FEATURE_TO_CUSTOMER);
#endif /* CFG_VOLT_INFO  */

#if CFG_SUPPORT_MLR
	INIT_UINT(prWifiVar->fgEnForceTxFrag,
		"EnForceTxFrag", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u2TxFragThr, "TxFragSplitThr", 1000,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2TxFragSplitSize, "TxFragSplitSize", 0,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucTxMlrRateRcpiThr, "TxMlrRateRcpiThr", 40,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnTxFragDebug,
		"EnTxFragDebug", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgEnTxFragTxDone,
		"EnTxFragTxDone", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->ucErrPos, "ErrPos", 0, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4MlrCfg, "MlrCfg", 0x3, FEATURE_TO_CUSTOMER);
#endif

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
	INIT_UINT(prWifiVar->u4TxDataDelayTimeout, "TxDataDelayTimeout", 2,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4TxDataDelayCnt, "TxDataDelayCnt", 10,
		  FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgEnTxDataDelayDbg,
		  "EnTxDataDelayDbg", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_TX_DATA_DELAY == 1 */

#if (CFG_SUPPORT_POWER_THROTTLING == 1)
	INIT_INT(prWifiVar->i4ThrmCtrlTemp,
		"ThrmCtrlTemp", THRM_PROT_DUTY_CTRL_TEMP, FEATURE_TO_CUSTOMER);
	INIT_INT(prWifiVar->i4ThrmRadioOffTemp, "ThrmRadioOffTemp",
		 THRM_PROT_RADIO_OFF_TEMP, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[0], "ThrmLv0TxDuty",
		  THRM_PROT_DEFAULT_LV0_DUTY, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[1], "ThrmLv1TxDuty",
		  THRM_PROT_DEFAULT_LV1_DUTY, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[2], "ThrmLv2TxDuty",
		  THRM_PROT_DEFAULT_LV2_DUTY, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[3], "ThrmLv3TxDuty",
		  THRM_PROT_DEFAULT_LV3_DUTY, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[4], "ThrmLv4TxDuty",
		  THRM_PROT_DEFAULT_LV4_DUTY, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->aucThrmLvTxDuty[5], "ThrmLv5TxDuty",
		  THRM_PROT_DEFAULT_LV5_DUTY, FEATURE_TO_CUSTOMER);
#endif
#if (CFG_SUPPORT_FW_IDX_LOG_TRANS == 1)
	INIT_UINT(prWifiVar->fgFwIdxLogTrans, "FwIdxLogTrans",
			FEATURE_DISABLED, FEATURE_TO_CUSTOMER);
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if (CFG_SUPPORT_FW_IDX_LOG_SAVE == 1)
	INIT_UINT(prWifiVar->fgFwIdxLogSave, "FwIdxLogSave",
			FW_IDX_LOG_SAVE_DISABLE, FEATURE_DEBUG_ONLY);
#endif /* CFG_SUPPORT_FW_IDX_LOG_TRANS */

#if CFG_SUPPORT_PCIE_ASPM
	INIT_UINT(prWifiVar->fgPcieEnableL1ss, "PcieEnableL1ss", 1,
		  FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_PCIE_GEN_SWITCH
	INIT_UINT(prWifiVar->u4PcieGenSwitchTputThr,
		  "PcieGenSwitchTputThr", 100, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4PcieGenSwitchJudgeTime,
		  "PcieGenSwitchJudgeTime", 10, FEATURE_TO_CUSTOMER);
#endif

	INIT_UINT(prWifiVar->fgEnWfdmaNoMmioRead,
		  "EnWfdmaNoMmioRead", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	if (IS_FEATURE_FORCE_ENABLED(prWifiVar->fgEnWfdmaNoMmioRead))
		prWifiVar->fgEnWfdmaNoMmioRead = FEATURE_ENABLED;
	else if (!prChipInfo->is_en_wfdma_no_mmio_read)
		prWifiVar->fgEnWfdmaNoMmioRead = FEATURE_DISABLED;

#if CFG_MTK_WIFI_SW_EMI_RING
	INIT_UINT(prWifiVar->fgEnSwEmiDbg, "EnSwEmiDbg", FEATURE_DISABLED,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgEnSwEmiRead, "EnSwEmiRead", FEATURE_ENABLED,
		  FEATURE_TO_CUSTOMER);
	if (IS_FEATURE_FORCE_ENABLED(prWifiVar->fgEnSwEmiRead))
		prWifiVar->fgEnSwEmiRead = FEATURE_ENABLED;
	else if (!prChipInfo->is_en_sw_emi_read)
		prWifiVar->fgEnSwEmiRead = FEATURE_DISABLED;
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

	INIT_UINT(prWifiVar->u4PrdcIntTime, "PrdcIntTime", 25,
		FEATURE_TO_CUSTOMER); /* unit: 20us */
	INIT_UINT(prWifiVar->u4SuspendPrdcIntTime, "SuspendPrdcIntTime", 50,
		FEATURE_TO_CUSTOMER); /* unit: 20us */
	INIT_UINT(prWifiVar->fgEnDlyInt, "EnDlyInt", 1, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u4DlyIntTime, "DlyIntTime", 2,
		FEATURE_TO_CUSTOMER); /* unit: 20us */
	/* 0: No check by count */
	INIT_UINT(prWifiVar->u4DlyIntCnt, "DlyIntCnt", 0, FEATURE_TO_CUSTOMER);

#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
#if CFG_DYNAMIC_RFB_ADJUSTMENT
	INIT_UINT(prWifiVar->u4PagePoolMinCnt, "PagePoolMinCnt",
		CFG_RX_MAX_PKT_NUM - nicRxGetUnUseCnt(prAdapter),
		FEATURE_TO_CUSTOMER);
#else
	INIT_UINT(prWifiVar->u4PagePoolMinCnt, "PagePoolMinCnt",
		  CFG_RX_MAX_PKT_NUM, FEATURE_TO_CUSTOMER);
#endif /* CFG_DYNAMIC_RFB_ADJUSTMENT */
	INIT_UINT(prWifiVar->u4PagePoolMaxCnt, "PagePoolMaxCnt",
		  CFG_RX_MAX_PKT_NUM * 3, FEATURE_TO_CUSTOMER);
	kalSetupPagePoolPageMaxMinNum(prWifiVar->u4PagePoolMinCnt,
				      prWifiVar->u4PagePoolMaxCnt);
#if ((CFG_SUPPORT_ICS == 1) || (CFG_SUPPORT_PHY_ICS == 1))
	INIT_UINT(prWifiVar->fgDynamicIcs, "DynamicIcsEn", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
#endif
#endif /* CFG_SUPPORT_DYNAMIC_PAGE_POOL */
#if (CFG_HW_DETECT_REPORT == 1)
	INIT_UINT(prWifiVar->fgHwDetectReportEn, "HwDetectReportEnable",
		  2, FEATURE_TO_CUSTOMER);
#endif /* CFG_HW_DETECT_REPORT  */

	INIT_INT(prWifiVar->iBTMMinRssi, "BTMMinRssi",
		 -83, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->fgBTMBlocklist, "BTMBlocklist",
		  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2DisallowBtmTimeout, "DisallowBtmTimeout",
		  1800, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->u2ConsecutiveBtmReqTimeout,
		  "ConsecutiveBtmReqTimeout", 300, FEATURE_TO_CUSTOMER);
	INIT_UINT(prWifiVar->ucConsecutiveBtmReqNum,
		  "ConsecutiveBtmReqNum", 3, FEATURE_TO_CUSTOMER);

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
	INIT_INT(prWifiVar->icTxPwrEnvLmtMin, "TxPwrEnvLmtMin",
		TX_PWR_ENV_LMT_MIN, FEATURE_TO_CUSTOMER);
#endif
#if CFG_CH_SELECT_ENHANCEMENT
	INIT_UINT(prWifiVar->ucStaSapIndoorConn, "StaSapIndoorConn",
		  FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif

#if WLAN_INCLUDE_SYS
	sysGetExtCfg(prAdapter);
#endif
	INIT_UINT(prWifiVar->fgEnSwAmsduSorting, "EnSwAmsduSorting",
		  FEATURE_DISABLED, FEATURE_DEBUG_ONLY);

#if CFG_SUPPORT_THERMAL_QUERY
	INIT_INT(prWifiVar->i4MaxTempLimit, "ThermalMaxTempThreshold",
		 MAX_TEMP_THRESHOLD, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4MinTempQueryTime, "ThermalMinTempQueryTime",
		  MIN_TEMP_QUERY_TIME, FEATURE_DEBUG_ONLY);
#endif
	INIT_INT(prWifiVar->icRegPwrLmtMin, "RegPwrLmtMin",
		TX_PWR_REG_LMT_MIN, FEATURE_DEBUG_ONLY);
	INIT_INT(prWifiVar->icRegPwrLmtMax, "RegPwrLmtMax",
		TX_PWR_REG_LMT_MAX, FEATURE_DEBUG_ONLY);
#if CFG_MTK_WIFI_WFDMA_WB
	INIT_UINT(prWifiVar->u4WfdmaCidxFetchTimeout,
		  "WfdmaCidxFetchTimeout", 500, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->fgWfdmaCidxFetchDbg,
		  "WfdmaCidxFetchDbg", FEATURE_DISABLED, FEATURE_DEBUG_ONLY);
#endif /* CFG_MTK_WIFI_WFDMA_WB */

	INIT_UINT(prWifiVar->u4RecoveryMsiRxCnt, "RecoveryMsiRxCnt", 5,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4RecoveryMsiTime, "RecoveryMsiTime", 1000,
		  FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4RecoveryMsiShortTime,
		  "RecoveryMsiShortTime", 200, FEATURE_DEBUG_ONLY);

	INIT_UINT(prWifiVar->u4HifDetectTxHangPeriod,
		  "HifDetectTxHangPeriod",
		  HIF_DETECT_TX_HANG_INTERVAL, FEATURE_DEBUG_ONLY);

#if CFG_UPDATE_PACING_SHIFT_SUPPORT
	/* Default TCP Small queue budget is ~1 ms of data (1sec >> 10).
	 * u4PacingShift is used to update the scaling factor for TSQ.
	 * This implies that a smaller value of u4PacingShift allows for a
	 * larger TSQ budget.
	 * For instance, if u4PacingShift is set to 1, the TSQ budget could be
	 * equivalent to 500ms of data (1sec >> 1).
	 * The value of u4PacingShift can be set within the range of 1 to 10.
	 */
	INIT_INT(prWifiVar->u4PacingShift, "PacingShift", 0,
		 FEATURE_TO_CUSTOMER);
#endif

#if CFG_SUPPORT_TPUT_FACTOR
	INIT_UINT(prWifiVar->fgTputFactorDump, "TputFactorDump",
		  FEATURE_ENABLED, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TputFactorDumpPeriodL1,
		  "TputFactorDumpPeriodL1", 500, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TputFactorDumpPeriodL2,
		  "TputFactorDumpPeriodL2", 10000, FEATURE_DEBUG_ONLY);
	INIT_UINT(prWifiVar->u4TputFactorDumpThresh,
		  "TputFactorDumpThresh", 100, FEATURE_DEBUG_ONLY);
#endif
#if (CFG_DYNAMIC_DMASHDL_MAX_QUOTA == 1)
	{
		uint8_t aucBuf[WLAN_CFG_VALUE_LEN_MAX];
		uint32_t u4Pos = 0;

		kalMemZero(aucBuf, WLAN_CFG_VALUE_LEN_MAX);
		for (u4Idx = 0; u4Idx < ENUM_BAND_NUM; u4Idx++) {
			u4Pos += kalSnprintf(
				aucBuf + u4Pos,
				WLAN_CFG_VALUE_LEN_MAX - u4Pos,
				"%s0x%x",
				(u4Idx == 0) ? "" : " ",
				prChipInfo->au4DmaMaxQuotaBand[u4Idx]);
		}

		for (u4Idx = 0; u4Idx < BAND_NUM; u4Idx++) {
			u4Pos += kalSnprintf(
				aucBuf + u4Pos,
				WLAN_CFG_VALUE_LEN_MAX - u4Pos,
				" 0x%x",
				prChipInfo->au4DmaMaxQuotaRfBand[u4Idx]);
		}
		INIT_STR(prWifiVar->aucDmaMaxQuota, "DmaMaxQuota", aucBuf,
			 FEATURE_DEBUG_ONLY);
		wlanCfgSet(prAdapter, "DmaMaxQuota",
			   prWifiVar->aucDmaMaxQuota, WLAN_CFG_DEFAULT);
	}
#endif
#if CFG_SUPPORT_WED_PROXY
	INIT_UINT(prWifiVar->fgEnableWed, "EnableWed", FEATURE_ENABLED,
		  FEATURE_DEBUG_ONLY);
#endif

#if CFG_ENABLE_WIFI_DIRECT && CFG_SUPPORT_CCM
	INIT_UINT(prWifiVar->eP2pCcmMode, "P2pCcmMode", P2P_CCM_MODE_SCC,
		  FEATURE_DEBUG_ONLY);
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	INIT_UINT(prWifiVar->fgSpPwrLmtBackoff,
		  "SpPwrLmtBackoff", FEATURE_ENABLED, FEATURE_TO_CUSTOMER);
#endif
}

void wlanCfgSetSwCtrl(struct ADAPTER *prAdapter)
{
	uint32_t i = 0;
	int8_t aucKey[WLAN_CFG_VALUE_LEN_MAX];
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];

	const int8_t acDelim[] = " ";
	int8_t *pcPtr = NULL;
	int8_t *pcDupValue = NULL;
	uint32_t au4Values[2] = {0};
	uint32_t u4TokenCount = 0;
	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;
	int32_t u4Ret = 0;

	for (i = 0; i < WLAN_CFG_SET_SW_CTRL_LEN_MAX; i++) {
		kalMemZero(aucValue, WLAN_CFG_VALUE_LEN_MAX);
		kalMemZero(aucKey, WLAN_CFG_VALUE_LEN_MAX);
		kalSnprintf(aucKey, WLAN_CFG_VALUE_LEN_MAX, "SwCtrl%d", i);

		/* get nothing */
		if (wlanCfgGet(prAdapter, aucKey, aucValue, NULL,
			       0, FEATURE_DEBUG_ONLY) != WLAN_STATUS_SUCCESS)
			continue;
		if (!kalStrCmp(aucValue, ""))
			continue;

		pcDupValue = aucValue;
		u4TokenCount = 0;

		while ((pcPtr = kalStrSep((char **)(&pcDupValue), acDelim))
		       != NULL) {

			if (!kalStrCmp(pcPtr, ""))
				continue;

			/* au4Values[u4TokenCount] = kalStrtoul(pcPtr, NULL, 0);
			 */
			u4Ret = kalkStrtou32(pcPtr, 0,
					     &(au4Values[u4TokenCount]));
			if (u4Ret)
				DBGLOG(INIT, LOUD,
				       "parse au4Values error u4Ret=%d\n",
				       u4Ret);
			u4TokenCount++;

			/* Only need 2 tokens */
			if (u4TokenCount >= 2)
				break;
		}

		if (u4TokenCount != 2)
			continue;

		rSwCtrlInfo.u4Id = au4Values[0];
		rSwCtrlInfo.u4Data = au4Values[1];

		rStatus = kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite,
				   &rSwCtrlInfo, sizeof(rSwCtrlInfo),
				   &u4BufLen);

	}
}

void wlanCfgSetChip(struct ADAPTER *prAdapter)
{
	uint32_t i = 0;
	int8_t aucKey[WLAN_CFG_VALUE_LEN_MAX];
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];

	uint32_t u4BufLen = 0;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo;

	for (i = 0; i < WLAN_CFG_SET_CHIP_LEN_MAX; i++) {
		kalMemZero(aucValue, WLAN_CFG_VALUE_LEN_MAX);
		kalMemZero(aucKey, WLAN_CFG_VALUE_LEN_MAX);
		kalSnprintf(aucKey, sizeof(aucKey), "SetChip%d", i);

		/* get nothing */
		if (wlanCfgGet(prAdapter, aucKey, aucValue, NULL,
			       0, FEATURE_DEBUG_ONLY) != WLAN_STATUS_SUCCESS)
			continue;
		if (!kalStrCmp(aucValue, ""))
			continue;

		kalMemZero(&rChipConfigInfo, sizeof(rChipConfigInfo));

		rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
		rChipConfigInfo.u2MsgSize = kalStrnLen(aucValue,
						       WLAN_CFG_VALUE_LEN_MAX);
		kalStrnCpy(rChipConfigInfo.aucCmd, aucValue,
			   CHIP_CONFIG_RESP_SIZE);

		rStatus = kalIoctl(prGlueInfo, wlanoidSetChipConfig,
				   &rChipConfigInfo, sizeof(rChipConfigInfo),
				   &u4BufLen);
	}
}

void wlanCfgSetDebugLevel(struct ADAPTER *prAdapter)
{
	uint32_t i = 0;
	int8_t aucKey[WLAN_CFG_VALUE_LEN_MAX];
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];
	const int8_t acDelim[] = " ";
	int8_t *pcDupValue;
	int8_t *pcPtr = NULL;

	uint32_t au4Values[2] = {0};
	uint32_t u4TokenCount = 0;
	uint32_t u4DbgIdx = 0;
	uint32_t u4DbgMask = 0;
	int32_t u4Ret = 0;

	for (i = 0; i < WLAN_CFG_SET_DEBUG_LEVEL_LEN_MAX; i++) {
		kalMemZero(aucValue, WLAN_CFG_VALUE_LEN_MAX);
		kalMemZero(aucKey, WLAN_CFG_VALUE_LEN_MAX);
		kalSnprintf(aucKey, WLAN_CFG_VALUE_LEN_MAX, "DbgLevel%d", i);

		/* get nothing */
		if (wlanCfgGet(prAdapter, aucKey, aucValue, NULL,
			       0, FEATURE_DEBUG_ONLY) != WLAN_STATUS_SUCCESS)
			continue;
		if (!kalStrCmp(aucValue, ""))
			continue;

		pcDupValue = aucValue;
		u4TokenCount = 0;

		while ((pcPtr = kalStrSep((char **)(&pcDupValue),
					  acDelim)) != NULL) {

			if (!kalStrCmp(pcPtr, ""))
				continue;

			/* au4Values[u4TokenCount] =
			 *			kalStrtoul(pcPtr, NULL, 0);
			 */
			u4Ret = kalkStrtou32(pcPtr, 0,
					     &(au4Values[u4TokenCount]));
			if (u4Ret)
				DBGLOG(INIT, LOUD,
				       "parse au4Values error u4Ret=%d\n",
				       u4Ret);
			u4TokenCount++;

			/* Only need 2 tokens */
			if (u4TokenCount >= 2)
				break;
		}

		if (u4TokenCount != 2)
			continue;

		u4DbgIdx = au4Values[0];
		u4DbgMask = au4Values[1];

		/* DBG level special control */
		if (u4DbgIdx == 0xFFFFFFFF) {
			wlanSetDriverDbgLevel(DBG_ALL_MODULE_IDX, u4DbgMask);
			DBGLOG(INIT, INFO,
			       "Set ALL DBG module log level to [0x%02x]!",
			       (uint8_t) u4DbgMask);
		} else if (u4DbgIdx == 0xFFFFFFFE) {
			wlanDebugInit();
			DBGLOG(INIT, INFO,
			       "Reset ALL DBG module log level to DEFAULT!");
		} else if (u4DbgIdx < DBG_MODULE_NUM) {
			wlanSetDriverDbgLevel(u4DbgIdx, u4DbgMask);
			DBGLOG(INIT, INFO,
			       "Set DBG module[%u] log level to [0x%02x]!",
			       u4DbgIdx, (uint8_t) u4DbgMask);
		}
	}
}

void wlanCfgSetCountryCode(struct ADAPTER *prAdapter)
{
	int8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];

	/* Apply COUNTRY Config */
	if (wlanCfgGet(prAdapter, "Country", aucValue, NULL,
		       0, FEATURE_TO_CUSTOMER) == WLAN_STATUS_SUCCESS) {
		prAdapter->rWifiVar.u2CountryCode =
			(((uint16_t) aucValue[0]) << 8) |
			((uint16_t) aucValue[1]);

		DBGLOG(INIT, TRACE, "u2CountryCode=0x%04x\n",
		       prAdapter->rWifiVar.u2CountryCode);

		if (regd_is_single_sku_en()) {
			rlmDomainOidSetCountry(prAdapter, aucValue, 2, 1);
			return;
		}

		/* Force to re-search country code in regulatory domains */
		prAdapter->prDomainInfo = NULL;
		rlmDomainSendCmd(prAdapter, TRUE);

		/* Update supported channel list in channel table based on
		 * current country domain
		 */
		wlanUpdateChannelTable(prAdapter->prGlueInfo);
	}
}

#if CFG_SUPPORT_CFG_FILE

struct WLAN_CFG_ENTRY *wlanCfgGetEntry(struct ADAPTER *prAdapter,
				       const int8_t *pucKey,
				       uint32_t u4Flags)
{

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct WLAN_CFG *prWlanCfg = NULL;
	struct WLAN_CFG_REC *prWlanCfgRec = NULL;
	struct WLAN_CFG *prWlanCfgEm = NULL;
	uint32_t i, u32MaxNum;

	if (u4Flags == WLAN_CFG_REC) {
		prWlanCfgRec = prAdapter->prWlanCfgRec;
		u32MaxNum = WLAN_CFG_REC_ENTRY_NUM_MAX;
		ASSERT(prWlanCfgRec);
	} else if (u4Flags == WLAN_CFG_EM) {
		prWlanCfgEm = prAdapter->prWlanCfgEm;
		u32MaxNum = WLAN_CFG_ENTRY_NUM_MAX;
		ASSERT(prWlanCfgEm);
	} else {
		prWlanCfg = prAdapter->prWlanCfg;
		u32MaxNum = WLAN_CFG_ENTRY_NUM_MAX;
		ASSERT(prWlanCfg);
	}


	ASSERT(pucKey);

	prWlanCfgEntry = NULL;

	for (i = 0; i < u32MaxNum; i++) {
		if (u4Flags == WLAN_CFG_REC)
			prWlanCfgEntry = &prWlanCfgRec->arWlanCfgBuf[i];
		else if (u4Flags == WLAN_CFG_EM)
			prWlanCfgEntry = &prWlanCfgEm->arWlanCfgBuf[i];
		else
			prWlanCfgEntry = &prWlanCfg->arWlanCfgBuf[i];

		if (prWlanCfgEntry->aucKey[0] != '\0') {
			if (kalStrnCmp(pucKey, prWlanCfgEntry->aucKey,
				       WLAN_CFG_KEY_LEN_MAX - 1) == 0)
				return prWlanCfgEntry;
		}
	}

	return NULL;

}

uint32_t wlanCfgGetTotalCfgNum(
	struct ADAPTER *prAdapter, uint32_t flag)
{
	uint32_t i = 0;
	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	uint32_t count = 0;

	for (i = 0; i < WLAN_CFG_REC_ENTRY_NUM_MAX; i++) {
		prWlanCfgEntry = wlanCfgGetEntryByIndex(prAdapter, i, flag);

		if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0'))
			break;

		count++;
	}
	return count;
}


struct WLAN_CFG_ENTRY *wlanCfgGetEntryByIndex(
	struct ADAPTER *prAdapter, const uint8_t ucIdx,
	uint32_t flag)
{

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct WLAN_CFG *prWlanCfg;
	struct WLAN_CFG_REC *prWlanCfgRec;
	struct WLAN_CFG *prWlanCfgEm;

	if (!prAdapter) {
		DBGLOG(INIT, WARN, "prAdapter is NULL\n");
		return NULL;
	}

	prWlanCfg = prAdapter->prWlanCfg;
	prWlanCfgRec = prAdapter->prWlanCfgRec;
	prWlanCfgEm = prAdapter->prWlanCfgEm;

	ASSERT(prWlanCfg);
	ASSERT(prWlanCfgRec);
	ASSERT(prWlanCfgEm);


	prWlanCfgEntry = NULL;

	if (flag == WLAN_CFG_REC)
		prWlanCfgEntry = &prWlanCfgRec->arWlanCfgBuf[ucIdx];
	else if (flag == WLAN_CFG_EM)
		prWlanCfgEntry = &prWlanCfgEm->arWlanCfgBuf[ucIdx];
	else
		prWlanCfgEntry = &prWlanCfg->arWlanCfgBuf[ucIdx];

	if (prWlanCfgEntry->aucKey[0] != '\0') {
		DBGLOG(INIT, LOUD, "get Index(%d) saved key %s\n", ucIdx,
		       prWlanCfgEntry->aucKey);
		return prWlanCfgEntry;
	}

	return NULL;

}

/*----------------------------------------------------------------------------*/
/*!
 * wlanCfgGet() - Get value by key. If key is not exist, set default value.
 * @prAdapter:    Pointer of Adapter Data Structure.
 * @pucKey:       Key to search in @prAdapter.
 * @pucValue:     The value get from @prAdapter or setted by @pucValueDef.
 * @pucValueDef:  The default value if @pucKey is not exist in @prAdapter.
 * @u4Flags:      Flags for wlanCfgGetEntry().
 *
 * For @pucValueDef, we expect to pass NULL instead of "" if caller don't want
 * to set default value. Otherwise, passing none NULL value will always return
 * WLAN_STATUS_SUCCESS.
 *
 * Return: WLAN_STATUS_SUCCESS  @pucValue are setted.
 *         WLAN_STATUS_FAILURE  @pucKey is not exist in @prAdapter, and
 *                              @pucValueDef is NULL.
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanCfgGet(struct ADAPTER *prAdapter,
	    const int8_t *pucKey, int8_t *pucValue, int8_t *pucValueDef,
	    uint32_t u4Flags, enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed)
{

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;

	ASSERT(pucValue);

	if (u4Flags == WLAN_CFG_DEFAULT && !BUILD_QA_DBG && fgIsDebugUsed) {
		/* do not pass to FW */
		wlanCfgSet(prAdapter, pucKey, NULL, WLAN_CFG_DEFAULT);
		goto done;
	}

	/* Find the exist */
	prWlanCfgEntry = wlanCfgGetEntry(prAdapter, pucKey, u4Flags);

	if (prWlanCfgEntry) {
		kalStrnCpy(pucValue, prWlanCfgEntry->aucValue,
			   WLAN_CFG_VALUE_LEN_MAX);
		return WLAN_STATUS_SUCCESS;
	}
done:
	if (pucValueDef) {
		kalStrnCpy(pucValue, pucValueDef,
			   WLAN_CFG_VALUE_LEN_MAX);
		return WLAN_STATUS_SUCCESS;
	}

	return WLAN_STATUS_FAILURE;
}

void wlanCfgRecordValue(struct ADAPTER *prAdapter,
			const int8_t *pucKey, uint32_t u4Value)
{
	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	uint8_t aucBuf[WLAN_CFG_VALUE_LEN_MAX];

	prWlanCfgEntry = wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_REC);

	kalMemZero(aucBuf, sizeof(aucBuf));

	kalSnprintf(aucBuf, WLAN_CFG_VALUE_LEN_MAX, "0x%x",
		    (unsigned int)u4Value);

	wlanCfgSet(prAdapter, pucKey, aucBuf, WLAN_CFG_REC);
}



uint32_t wlanCfgGetUint32(struct ADAPTER *prAdapter, const int8_t *pucKey,
	uint32_t u4ValueDef, enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed)
{
	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct WLAN_CFG *prWlanCfg;
	uint32_t u4Value;
	int32_t u4Ret;

	prWlanCfg = prAdapter->prWlanCfg;

	ASSERT(prWlanCfg);

	u4Value = u4ValueDef;

	if (!BUILD_QA_DBG && fgIsDebugUsed) {
		/* do not pass to FW */
		wlanCfgSet(prAdapter, pucKey, NULL, WLAN_CFG_DEFAULT);
		goto done;
	}

	/* Find the exist */
	prWlanCfgEntry = wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_DEFAULT);

	if (prWlanCfgEntry) {
		/* u4Ret = kalStrtoul(prWlanCfgEntry->aucValue, NULL, 0); */
		u4Ret = kalkStrtou32(prWlanCfgEntry->aucValue, 0, &u4Value);
		if (u4Ret)
			DBGLOG(INIT, LOUD, "parse aucValue error u4Ret=%d\n",
			       u4Ret);
	}
done:
	wlanCfgRecordValue(prAdapter, pucKey, u4Value);

	return u4Value;
}

int32_t wlanCfgGetInt32(struct ADAPTER *prAdapter, const int8_t *pucKey,
	int32_t i4ValueDef, enum ENUM_FEATURE_SUPPORT_SCOPE fgIsDebugUsed)
{
	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct WLAN_CFG *prWlanCfg;
	int32_t i4Value = 0;
	int32_t i4Ret = 0;

	prWlanCfg = prAdapter->prWlanCfg;

	ASSERT(prWlanCfg);

	i4Value = i4ValueDef;

	if (!BUILD_QA_DBG && fgIsDebugUsed) {
		/* do not pass to FW */
		wlanCfgSet(prAdapter, pucKey, NULL, WLAN_CFG_DEFAULT);
		goto done;
	}

	/* Find the exist */
	prWlanCfgEntry = wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_DEFAULT);

	if (prWlanCfgEntry) {
		/* i4Ret = kalStrtol(prWlanCfgEntry->aucValue, NULL, 0); */
		i4Ret = kalkStrtos32(prWlanCfgEntry->aucValue, 0, &i4Value);
		if (i4Ret)
			DBGLOG(INIT, LOUD, "parse aucValue error i4Ret=%d\n",
			       i4Ret);
	}
done:
	wlanCfgRecordValue(prAdapter, pucKey, (uint32_t)i4Value);

	return i4Value;
}

uint32_t wlanCfgSet(struct ADAPTER *prAdapter,
		    const int8_t *pucKey, int8_t *pucValue, uint32_t u4Flags)
{

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	struct WLAN_CFG *prWlanCfg = NULL;
	struct WLAN_CFG *prWlanCfgEm = NULL;
	struct WLAN_CFG_REC *prWlanCfgRec = NULL;
	uint32_t u4EntryIndex;
	uint32_t i;
	uint8_t ucExist;
#if CFG_TC10_FEATURE
	int32_t i4ReadValue = 0;
	int32_t i4Ret = 0;
#endif

	ASSERT(pucKey);

#if CFG_TC10_FEATURE
	if (pucValue) {
		i4Ret = kalkStrtos32(pucValue, 0, &i4ReadValue);
		DBGLOG(INIT, INFO, "[%s]:[%d] OP:%d\n",
			pucKey, i4ReadValue, u4Flags);
	} else
		DBGLOG(INIT, INFO, "[%s]:[NA] OP:%d\n", pucKey, u4Flags);
#else
	DBGLOG(INIT, LOUD, "[%s]:[%s] OP:%d\n", pucKey, pucValue, u4Flags);
#endif
	/* Find the exist */
	ucExist = 0;
	if (u4Flags == WLAN_CFG_REC) {
		prWlanCfgEntry =
			wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_REC);
		prWlanCfgRec = prAdapter->prWlanCfgRec;
		ASSERT(prWlanCfgRec);
	} else if (u4Flags == WLAN_CFG_EM) {
		prWlanCfgEntry =
			wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_EM);
		prWlanCfgEm = prAdapter->prWlanCfgEm;
		ASSERT(prWlanCfgEm);
	} else {
		prWlanCfgEntry =
			wlanCfgGetEntry(prAdapter, pucKey, WLAN_CFG_DEFAULT);
		prWlanCfg = prAdapter->prWlanCfg;
		ASSERT(prWlanCfg);
	}

	if (!prWlanCfgEntry) {
		/* Find the empty */
		for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {
			if (u4Flags == WLAN_CFG_REC)
				prWlanCfgEntry = &prWlanCfgRec->arWlanCfgBuf[i];
			else if (u4Flags == WLAN_CFG_EM)
				prWlanCfgEntry = &prWlanCfgEm->arWlanCfgBuf[i];
			else
				prWlanCfgEntry = &prWlanCfg->arWlanCfgBuf[i];

			if (prWlanCfgEntry->aucKey[0] == '\0')
				break;
		}

		u4EntryIndex = i;
		if (u4EntryIndex < WLAN_CFG_ENTRY_NUM_MAX) {
			if (u4Flags == WLAN_CFG_REC)
				prWlanCfgEntry =
				    &prWlanCfgRec->arWlanCfgBuf[u4EntryIndex];
			else if (u4Flags == WLAN_CFG_EM)
				prWlanCfgEntry =
				    &prWlanCfgEm->arWlanCfgBuf[u4EntryIndex];
			else
				prWlanCfgEntry =
				    &prWlanCfg->arWlanCfgBuf[u4EntryIndex];
			kalMemZero(prWlanCfgEntry,
				   sizeof(struct WLAN_CFG_ENTRY));
		} else {
			prWlanCfgEntry = NULL;
		}
	} /* !prWlanCfgEntry */
	else
		ucExist = 1;

	if (prWlanCfgEntry) {
		if (ucExist == 0) {
			kalStrnCpy(prWlanCfgEntry->aucKey, pucKey,
				   WLAN_CFG_KEY_LEN_MAX - 1);
			prWlanCfgEntry->aucKey[WLAN_CFG_KEY_LEN_MAX - 1] = '\0';
		}
		if (pucValue && pucValue[0] != '\0') {
			kalStrnCpy(prWlanCfgEntry->aucValue, pucValue,
				   WLAN_CFG_VALUE_LEN_MAX - 1);
			prWlanCfgEntry->aucValue[WLAN_CFG_VALUE_LEN_MAX - 1] =
									'\0';
		} else {
			/* remove the entry if value is empty */
			kalMemZero(prWlanCfgEntry,
				   sizeof(struct WLAN_CFG_ENTRY));
		}
	}
	/* prWlanCfgEntry */
	if (prWlanCfgEntry) {
		return WLAN_STATUS_SUCCESS;
	}

	DBGLOG(INIT, ERROR,
			"WIFI CFG has no empty entry, key \'%s\', value \'%s\'\n",
			pucKey ? pucKey : NULL, pucValue ? pucValue : NULL);

	return WLAN_STATUS_FAILURE;
}

uint32_t wlanCfgSetUint32(struct ADAPTER *prAdapter,
			  const int8_t *pucKey, uint32_t u4Value)
{

	struct WLAN_CFG *prWlanCfg;
	uint8_t aucBuf[WLAN_CFG_VALUE_LEN_MAX];

	prWlanCfg = prAdapter->prWlanCfg;

	ASSERT(prWlanCfg);

	kalMemZero(aucBuf, sizeof(aucBuf));

	kalSnprintf(aucBuf, WLAN_CFG_VALUE_LEN_MAX, "0x%x",
		    (unsigned int)u4Value);

	return wlanCfgSet(prAdapter, pucKey, aucBuf, WLAN_CFG_DEFAULT);
}

enum {
	STATE_EOF = 0,
	STATE_TEXT = 1,
	STATE_NEWLINE = 2
};

struct WLAN_CFG_PARSE_STATE_S {
	int8_t *ptr;
	int8_t *text;
#if CFG_SUPPORT_EASY_DEBUG
	uint32_t textsize;
#endif
	int32_t nexttoken;
	uint32_t maxSize;
};

int32_t wlanCfgFindNextToken(struct WLAN_CFG_PARSE_STATE_S
			     *state)
{
	int8_t *x = state->ptr;
	int8_t *s;

	if (state->nexttoken) {
		int32_t t = state->nexttoken;

		state->nexttoken = 0;
		return t;
	}

	for (;;) {
		switch (*x) {
		case 0:
			state->ptr = x;
			return STATE_EOF;
		case '\n':
			x++;
			state->ptr = x;
			return STATE_NEWLINE;
		case ' ':
		case ',':
		/*case ':':  should not including : , mac addr would be fail*/
		case '\t':
		case '\r':
			x++;
			continue;
		case '#':
			while (*x && (*x != '\n'))
				x++;
			if (*x == '\n') {
				state->ptr = x + 1;
				return STATE_NEWLINE;
			}
			state->ptr = x;
			return STATE_EOF;

		default:
			goto text;
		}
	}

textdone:
	state->ptr = x;
	*s = 0;
	return STATE_TEXT;
text:
	state->text = s = x;
textresume:
	for (;;) {
		switch (*x) {
		case 0:
			goto textdone;
		case ' ':
		case ',':
		/* case ':': */
		case '\t':
		case '\r':
			x++;
			goto textdone;
		case '\n':
			state->nexttoken = STATE_NEWLINE;
			x++;
			goto textdone;
		case '"':
			x++;
			for (;;) {
				switch (*x) {
				case 0:
					/* unterminated quoted thing */
					state->ptr = x;
					return STATE_EOF;
				case '"':
					x++;
					goto textresume;
				default:
					*s++ = *x++;
				}
			}
			break;
		case '\\':
			x++;
			switch (*x) {
			case 0:
				goto textdone;
			case 'n':
				*s++ = '\n';
				break;
			case 'r':
				*s++ = '\r';
				break;
			case 't':
				*s++ = '\t';
				break;
			case '\\':
				*s++ = '\\';
				break;
			case '\r':
				/* \ <cr> <lf> -> line continuation */
				if (x[1] != '\n') {
					x++;
					continue;
				}
				kal_fallthrough;
			case '\n':
				/* \ <lf> -> line continuation */
				x++;
				/* eat any extra whitespace */
				while ((*x == ' ') || (*x == '\t'))
					x++;
				continue;
			default:
				/* unknown escape -- just copy */
				*s++ = *x++;
			}
			continue;
		default:
			*s++ = *x++;
#if CFG_SUPPORT_EASY_DEBUG
			state->textsize++;
#endif
		}
	}
	return STATE_EOF;
}

/**
 * wlanCfgFindNextTokenWithEqual() - cfg and ini file parsing
 *
 * This function is called from wlanCfgParse()
 */
int32_t wlanCfgFindNextTokenWithEqual(struct WLAN_CFG_PARSE_STATE_S
			     *state)
{
	int8_t *x = state->ptr;
	int8_t *s;

	if (state->nexttoken) {
		int32_t t = state->nexttoken;

		state->nexttoken = 0;
		return t;
	}

	for (;;) {
		switch (*x) {
		case 0:
			state->ptr = x;
			return STATE_EOF;
		case '\n':
			x++;
			state->ptr = x;
			return STATE_NEWLINE;
		case ' ':
		case ',':
		/*case ':':  should not including : , mac addr would be fail*/
		case '\t':
		case '\r':
		case '=':
			x++;
			continue;
		case '#':
			while (*x && (*x != '\n'))
				x++;
			if (*x == '\n') {
				state->ptr = x + 1;
				return STATE_NEWLINE;
			}
			state->ptr = x;
			return STATE_EOF;

		default:
			goto text;
		}
	}

textdone:
	state->ptr = x;
	*s = 0;
	return STATE_TEXT;
text:
	state->text = s = x;
textresume:
	for (;;) {
		switch (*x) {
		case 0:
			goto textdone;
		case ' ':
		case ',':
		/* case ':': */
		case '\t':
		case '\r':
		case '=':
			x++;
			goto textdone;
		case '\n':
			state->nexttoken = STATE_NEWLINE;
			x++;
			goto textdone;
		case '"':
			x++;
			for (;;) {
				switch (*x) {
				case 0:
					/* unterminated quoted thing */
					state->ptr = x;
					return STATE_EOF;
				case '"':
					x++;
					goto textresume;
				default:
					*s++ = *x++;
				}
			}
			break;
		case '\\':
			x++;
			switch (*x) {
			case 0:
				goto textdone;
			case 'n':
				*s++ = '\n';
				break;
			case 'r':
				*s++ = '\r';
				break;
			case 't':
				*s++ = '\t';
				break;
			case '\\':
				*s++ = '\\';
				break;
			case '\r':
				/* \ <cr> <lf> -> line continuation */
				if (x[1] != '\n') {
					x++;
					continue;
				}
				kal_fallthrough;
			case '\n':
				/* \ <lf> -> line continuation */
				x++;
				/* eat any extra whitespace */
				while ((*x == ' ') || (*x == '\t'))
					x++;
				continue;
			default:
				/* unknown escape -- just copy */
				*s++ = *x++;
			}
			continue;
		default:
			*s++ = *x++;
#if CFG_SUPPORT_EASY_DEBUG
			state->textsize++;
#endif
		}
	}
	return STATE_EOF;
}

void wlanCfgParseArgument(int8_t *cmdLine,
			      int32_t *argc, int8_t *argv[])
{
	struct WLAN_CFG_PARSE_STATE_S state;
	int8_t **args;
	int32_t nargs;

	if (cmdLine == NULL || argc == NULL || argv == NULL) {
		DBGLOG(INIT, ERROR, "parameter is NULL: %p, %p, %p\n",
		       cmdLine, argc, argv);
		return;
	}
	args = argv;
	nargs = 0;
	*argc = 0;
	state.ptr = cmdLine;
	state.nexttoken = 0;
	state.maxSize = 0;
#if CFG_SUPPORT_EASY_DEBUG
	state.textsize = 0;
#endif

	if (kalStrnLen(cmdLine, 512) >= 512) {
		DBGLOG(INIT, ERROR, "cmdLine >= 512\n");
		return;
	}

	for (;;) {
		switch (wlanCfgFindNextToken(&state)) {
		case STATE_EOF:
			goto exit;
		case STATE_NEWLINE:
			goto exit;
		case STATE_TEXT:
			if (nargs < WLAN_CFG_ARGV_MAX) {
				DBGLOG(REQ, LOUD, "arg%u=%s",
				       nargs, state.text);
				args[nargs++] = state.text;
			}
			break;
		}
	}

exit:
	*argc = nargs;
	return;
}

#if CFG_WOW_SUPPORT
uint32_t wlanCfgParseArgumentLong(int8_t *cmdLine,
				  int32_t *argc, int8_t *argv[])
{
	struct WLAN_CFG_PARSE_STATE_S state;
	int8_t **args;
	int32_t nargs;

	if (cmdLine == NULL || argc == NULL || argv == NULL) {
		DBGLOG(INIT, ERROR, "parameter is NULL: %p, %p, %p\n",
		       cmdLine, argc, argv);
		return WLAN_STATUS_FAILURE;
	}
	args = argv;
	nargs = 0;
	state.ptr = cmdLine;
	state.nexttoken = 0;
	state.maxSize = 0;
#if CFG_SUPPORT_EASY_DEBUG
	state.textsize = 0;
#endif

	if (kalStrnLen(cmdLine, 512) >= 512) {
		DBGLOG(INIT, ERROR, "cmdLine >= 512\n");
		return WLAN_STATUS_FAILURE;
	}

	for (;;) {
		switch (wlanCfgFindNextToken(&state)) {
		case STATE_EOF:
			goto exit;
		case STATE_NEWLINE:
			goto exit;
		case STATE_TEXT:
			if (nargs < WLAN_CFG_ARGV_MAX_LONG)
				args[nargs++] = state.text;
			break;
		}
	}

exit:
	*argc = nargs;
	return WLAN_STATUS_SUCCESS;
}
#endif

uint32_t
wlanCfgParseAddEntry(struct ADAPTER *prAdapter,
		     uint8_t *pucKeyHead, uint8_t *pucKeyTail,
		     uint8_t *pucValueHead, uint8_t *pucValueTail)
{

	uint8_t aucKey[WLAN_CFG_KEY_LEN_MAX];
	uint8_t aucValue[WLAN_CFG_VALUE_LEN_MAX];
	uint32_t u4Len;

	kalMemZero(aucKey, sizeof(aucKey));
	kalMemZero(aucValue, sizeof(aucValue));

	if ((pucKeyHead == NULL)
	    || (pucValueHead == NULL)
	   )
		return WLAN_STATUS_FAILURE;

	if (pucKeyTail) {
		if (pucKeyHead > pucKeyTail)
			return WLAN_STATUS_FAILURE;
		u4Len = pucKeyTail - pucKeyHead + 1;
	} else
		u4Len = kalStrnLen(pucKeyHead, WLAN_CFG_KEY_LEN_MAX - 1);

	if (u4Len >= WLAN_CFG_KEY_LEN_MAX)
		u4Len = WLAN_CFG_KEY_LEN_MAX - 1;

	kalStrnCpy(aucKey, pucKeyHead, u4Len);

	if (pucValueTail) {
		if (pucValueHead > pucValueTail)
			return WLAN_STATUS_FAILURE;
		u4Len = pucValueTail - pucValueHead + 1;
	} else
		u4Len = kalStrnLen(pucValueHead,
				   WLAN_CFG_VALUE_LEN_MAX - 1);

	if (u4Len >= WLAN_CFG_VALUE_LEN_MAX)
		u4Len = WLAN_CFG_VALUE_LEN_MAX - 1;

	kalStrnCpy(aucValue, pucValueHead, u4Len);

	return wlanCfgSet(prAdapter, aucKey, aucValue, WLAN_CFG_DEFAULT);
}

enum {
	WAIT_KEY_HEAD = 0,
	WAIT_KEY_TAIL,
	WAIT_VALUE_HEAD,
	WAIT_VALUE_TAIL,
	WAIT_COMMENT_TAIL
};

#if CFG_SUPPORT_EASY_DEBUG

uint32_t wlanCfgParseToFW(int8_t **args, int8_t *args_size,
			  uint8_t nargs, int8_t *buffer, uint8_t times)
{
	uint8_t *data = NULL;
	char ch;
	int32_t i = 0, j = 0;
	uint32_t bufferindex = 0, base = 0;
	uint32_t sum = 0, startOffset = 0;
	struct CMD_FORMAT_V1 cmd_v1;

	memset(&cmd_v1, 0, sizeof(struct CMD_FORMAT_V1));

	cmd_v1.itemType = ITEM_TYPE_DEC;

	if (buffer == NULL ||
	    args_size[ED_STRING_SITE] == 0 ||
	    args_size[ED_VALUE_SITE] == 0 ||
	    cmd_v1.itemType < ITEM_TYPE_DEC ||
	    cmd_v1.itemType > ITEM_TYPE_STR) {
		DBGLOG(INIT, ERROR, "cfg args wrong\n");
		return WLAN_STATUS_FAILURE;
	}

	cmd_v1.itemStringLength = args_size[ED_STRING_SITE];
	strncpy(cmd_v1.itemString, args[ED_STRING_SITE],
		cmd_v1.itemStringLength);
	DBGLOG(INIT, INFO, "itemString:");
	for (i = 0; i < cmd_v1.itemStringLength; i++)
		DBGLOG(INIT, INFO, "%c", cmd_v1.itemString[i]);
	DBGLOG(INIT, INFO, "\n");

	DBGLOG(INIT, INFO, "cmd_v1.itemType = %d\n",
	       cmd_v1.itemType);
	if (cmd_v1.itemType == ITEM_TYPE_DEC ||
	    cmd_v1.itemType == ITEM_TYPE_HEX) {
		data = args[ED_VALUE_SITE];

		switch (cmd_v1.itemType) {
		case ITEM_TYPE_DEC:
			base = 10;
			startOffset = 0;
			break;
		case ITEM_TYPE_HEX:
			ch = *data;
			if (args_size[ED_VALUE_SITE] < 3 || ch != '0') {
				DBGLOG(INIT, WARN,
				       "Hex args must have prefix '0x'\n");
				return WLAN_STATUS_FAILURE;
			}

			data++;
			ch = *data;
			if (ch != 'x' && ch != 'X') {
				DBGLOG(INIT, WARN,
				       "Hex args must have prefix '0x'\n");
				return WLAN_STATUS_FAILURE;
			}
			data++;
			base = 16;
			startOffset = 2;
			break;
		}

		for (j = args_size[ED_VALUE_SITE] - 1 - startOffset; j >= 0;
		     j--) {
			sum = sum * base + hexDigitToInt(*data);
			DBGLOG(INIT, WARN, "size:%d data[%d]=%u, sum=%u\n",
			       args_size[ED_VALUE_SITE], j,
				   hexDigitToInt(*data), sum);

			data++;
		}

		bufferindex = 0;
		do {
			cmd_v1.itemValue[bufferindex++] = sum & 0xFF;
			sum = sum >> 8;
		} while (sum > 0);
		cmd_v1.itemValueLength = bufferindex;
	} else if (cmd_v1.itemType == ITEM_TYPE_STR) {
		cmd_v1.itemValueLength = args_size[ED_VALUE_SITE];
		strncpy(cmd_v1.itemValue, args[ED_VALUE_SITE],
			cmd_v1.itemValueLength);
	}

	DBGLOG(INIT, INFO, "Length = %d itemValue:",
	       cmd_v1.itemValueLength);
	for (i = cmd_v1.itemValueLength - 1; i >= 0; i--)
		DBGLOG(INIT, ERROR, "%d,", cmd_v1.itemValue[i]);
	DBGLOG(INIT, INFO, "\n");
	memcpy(((struct CMD_FORMAT_V1 *)buffer) + times, &cmd_v1,
	       sizeof(struct CMD_FORMAT_V1));

	return WLAN_STATUS_SUCCESS;
}


/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is to send WLAN feature options to firmware
 *
 * @param prAdapter  Pointer of ADAPTER_T
 *
 * @return none
 */
/*----------------------------------------------------------------------------*/
void wlanFeatureToFw(struct ADAPTER *prAdapter, uint32_t u4Flag,
	uint8_t *pucKey)
{

	struct WLAN_CFG_ENTRY *prWlanCfgEntry;
	uint32_t i;
	struct CMD_HEADER rCmdV1Header;
	uint32_t rStatus;
	struct CMD_FORMAT_V1 rCmd_v1;
	uint8_t  ucTimes = 0;



	rCmdV1Header.cmdType = CMD_TYPE_SET;
	rCmdV1Header.cmdVersion = CMD_VER_1;
	rCmdV1Header.cmdBufferLen = 0;
	rCmdV1Header.itemNum = 0;

	kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
	kalMemSet(&rCmd_v1, 0, sizeof(struct CMD_FORMAT_V1));


	prWlanCfgEntry = NULL;

	for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {

		prWlanCfgEntry = wlanCfgGetEntryByIndex(prAdapter, i, u4Flag);

		if (prWlanCfgEntry) {

			if (pucKey != NULL) {
				if (kalStrnCmp(pucKey, prWlanCfgEntry->aucKey,
					MAX_CMD_NAME_MAX_LENGTH) != 0)
					continue;

				if (ucTimes != 0)
					break;
			}

			rCmd_v1.itemType = ITEM_TYPE_STR;


			/*send string format to firmware */
			rCmd_v1.itemStringLength = kalStrLen(
							prWlanCfgEntry->aucKey);
			if (rCmd_v1.itemStringLength > MAX_CMD_NAME_MAX_LENGTH)
				continue;
			kalMemZero(rCmd_v1.itemString, MAX_CMD_NAME_MAX_LENGTH);
			kalMemCopy(rCmd_v1.itemString, prWlanCfgEntry->aucKey,
				   rCmd_v1.itemStringLength);


			rCmd_v1.itemValueLength = kalStrLen(
						  prWlanCfgEntry->aucValue);
			if (rCmd_v1.itemValueLength > MAX_CMD_VALUE_MAX_LENGTH)
				continue;
			kalMemZero(rCmd_v1.itemValue, MAX_CMD_VALUE_MAX_LENGTH);
			kalMemCopy(rCmd_v1.itemValue, prWlanCfgEntry->aucValue,
				   rCmd_v1.itemValueLength);



			DBGLOG(INIT, TRACE,
			       "Send key word (%s) WITH (%s) to firmware\n",
			       rCmd_v1.itemString, rCmd_v1.itemValue);

			kalMemCopy(((struct CMD_FORMAT_V1 *)rCmdV1Header.buffer)
				   + ucTimes,
				   &rCmd_v1, sizeof(struct CMD_FORMAT_V1));


			ucTimes++;
			rCmdV1Header.cmdBufferLen +=
						sizeof(struct CMD_FORMAT_V1);
			rCmdV1Header.itemNum += ucTimes;

			if (ucTimes == MAX_CMD_ITEM_MAX) {
				/* Send to FW */
				rCmdV1Header.itemNum = ucTimes;

				rStatus = wlanSendSetQueryCmd(
						/* prAdapter */
						prAdapter,
						/* 0x70 */
						CMD_ID_GET_SET_CUSTOMER_CFG,
						/* fgSetQuery */
						TRUE,
						/* fgNeedResp */
						FALSE,
						/* fgIsOid */
						FALSE,
						/* pfCmdDoneHandler*/
						NULL,
						/* pfCmdTimeoutHandler */
						NULL,
						/* u4SetQueryInfoLen */
						sizeof(struct CMD_HEADER),
						/* pucInfoBuffer */
						(uint8_t *)&rCmdV1Header,
						/* pvSetQueryBuffer */
						NULL,
						/* u4SetQueryBufferLen */
						0);

				if (rStatus == WLAN_STATUS_FAILURE)
					DBGLOG(INIT, INFO,
					       "[Fail]kalIoctl wifiSefCFG fail 0x%x\n",
					       rStatus);

				DBGLOG(INIT, TRACE,
				       "kalIoctl wifiSefCFG num:%d\n", ucTimes);
				kalMemSet(rCmdV1Header.buffer, 0,
					  MAX_CMD_BUFFER_LENGTH);
				rCmdV1Header.cmdBufferLen = 0;
				ucTimes = 0;
			}
		}
	}


	if (ucTimes != 0) {
		/* Send to FW */
		rCmdV1Header.itemNum = ucTimes;

		rStatus = wlanSendSetQueryCmd(
			  prAdapter,	/* prAdapter */
			  CMD_ID_GET_SET_CUSTOMER_CFG,	/* 0x70 */
			  TRUE,		/* fgSetQuery */
			  FALSE,	/* fgNeedResp */
			  FALSE,	/* fgIsOid */
			  NULL,		/* pfCmdDoneHandler*/
			  NULL,		/* pfCmdTimeoutHandler */
			  sizeof(struct CMD_HEADER),	/* u4SetQueryInfoLen */
			  (uint8_t *)&rCmdV1Header,/* pucInfoBuffer */
			  NULL,	/* pvSetQueryBuffer */
			  0);	/* u4SetQueryBufferLen */

		if (rStatus == WLAN_STATUS_FAILURE)
			DBGLOG(INIT, INFO,
			       "[Fail]kalIoctl wifiSefCFG fail 0x%x\n",
			       rStatus);

		DBGLOG(INIT, TRACE,
			"cmdV1Header.itemNum:%d, kalIoctl wifiSefCFG num:%d\n",
			rCmdV1Header.itemNum,
			ucTimes);
		kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
		rCmdV1Header.cmdBufferLen = 0;
		ucTimes = 0;
	}

#if CFG_SUPPORT_SMART_GEAR
	/*Send Event, Notify Fwks*/
	#if CFG_SUPPORT_DATA_STALL
	if (prAdapter->rWifiVar.ucSGCfg == 0x00) {
		enum ENUM_VENDOR_DRIVER_EVENT eEvent = EVENT_SG_DISABLE;

		KAL_REPORT_ERROR_EVENT(prAdapter,
			eEvent, (uint16_t)sizeof(u_int8_t),
			0,
			TRUE);
	}
	#endif /* CFG_SUPPORT_DATA_STALL */
#endif

}

uint32_t wlanCfgParse(struct ADAPTER *prAdapter,
		      uint8_t *pucConfigBuf, uint32_t u4ConfigBufLen,
		      u_int8_t isFwConfig)
{
	struct WLAN_CFG_PARSE_STATE_S state;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
	int8_t **ppcArgs;
	int32_t i4Nargs;
	int8_t   arcArgv_size[WLAN_CFG_ARGV_MAX];
	uint8_t  ucTimes = 0;
	uint32_t rStatus;
	struct CMD_HEADER rCmdV1Header;
	int8_t ucTmp[WLAN_CFG_VALUE_LEN_MAX];
	uint8_t i;

	uint8_t *pucCurrBuf = ucTmp;
	uint32_t u4CurrSize = ARRAY_SIZE(ucTmp);
	uint32_t u4RetSize = 0;

	rCmdV1Header.cmdType = CMD_TYPE_SET;
	rCmdV1Header.cmdVersion = CMD_VER_1;
	rCmdV1Header.cmdBufferLen = 0;
	kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);

	if (pucConfigBuf == NULL) {
		DBGLOG(INIT, ERROR, "pucConfigBuf is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4ConfigBufLen == 0)
		return WLAN_STATUS_FAILURE;

	ppcArgs = apcArgv;
	i4Nargs = 0;
	state.ptr = pucConfigBuf;
	state.nexttoken = 0;
	state.textsize = 0;
	state.maxSize = u4ConfigBufLen;
	DBGLOG(INIT, INFO, "wlanCfgParse()\n");

	for (;;) {
		switch (wlanCfgFindNextTokenWithEqual(&state)) {
		case STATE_EOF:
			if (i4Nargs < 2)
				goto exit;

			DBGLOG(INIT, INFO, "STATE_EOF\n");

			/* 3 parmeter mode transforation */
			if (i4Nargs == 3 && !isFwConfig &&
			    arcArgv_size[0] == 1) {

				/* parsing and transfer the format
				 * Format  1:Dec 2.Hex 3.String
				 */

				kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);
				pucCurrBuf = ucTmp;
				u4CurrSize = ARRAY_SIZE(ucTmp);

				if ((*ppcArgs[0] == '2') &&
				    (*(ppcArgs[2]) != '0') &&
				    (*(ppcArgs[2] + 1) != 'x')) {
					DBGLOG(INIT, WARN,
					       "config file got a hex format\n"
					       );
					kalSnprintf(pucCurrBuf, u4CurrSize,
						    "0x%s", ppcArgs[2]);
				} else {
					kalSnprintf(pucCurrBuf, u4CurrSize,
						    "%s", ppcArgs[2]);
				}
				DBGLOG(INIT, WARN,
				       "[3 parameter mode][%s],[%s],[%s]\n",
				       ppcArgs[0], ppcArgs[1], ucTmp);
				wlanCfgParseAddEntry(prAdapter, ppcArgs[1],
						     NULL, ucTmp, NULL);
				kalMemSet(arcArgv_size, 0, WLAN_CFG_ARGV_MAX);
				kalMemSet(apcArgv, 0,
					WLAN_CFG_ARGV_MAX * sizeof(int8_t *));
				i4Nargs = 0;
				goto exit;

			}

			wlanCfgParseAddEntry(prAdapter, ppcArgs[0], NULL,
					     ppcArgs[1], NULL);

			if (isFwConfig) {
				uint32_t ret;

				ret = wlanCfgParseToFW(ppcArgs, arcArgv_size,
						       i4Nargs,
						       rCmdV1Header.buffer,
						       ucTimes);
				if (ret == WLAN_STATUS_SUCCESS) {
					ucTimes++;
					rCmdV1Header.cmdBufferLen +=
						sizeof(struct CMD_FORMAT_V1);
				}
			}

			goto exit;


		case STATE_NEWLINE:
			if (i4Nargs < 2)
				break;

			DBGLOG(INIT, INFO, "STATE_NEWLINE\n");
#if 1
			/* 3 parmeter mode transforation */
			if (i4Nargs == 3 && !isFwConfig &&
			    arcArgv_size[0] == 1) {
				/* parsing and transfer the format
				 * Format  1:Dec 2.Hex 3.String
				 */
				kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);
				pucCurrBuf = ucTmp;
				u4CurrSize = ARRAY_SIZE(ucTmp);

				if ((*ppcArgs[0] == '2') &&
				    (*(ppcArgs[2]) != '0') &&
				    (*(ppcArgs[2] + 1) != 'x')) {
					DBGLOG(INIT, WARN,
					       "config file got a hex format\n");
					kalSnprintf(pucCurrBuf, u4CurrSize,
						    "0x%s", ppcArgs[2]);

				} else {
					kalSnprintf(pucCurrBuf, u4CurrSize,
						    "%s", ppcArgs[2]);
				}


				DBGLOG(INIT, WARN,
				       "[3 parameter mode][%s],[%s],[%s]\n",
				       ppcArgs[0], ppcArgs[1], ucTmp);
				wlanCfgParseAddEntry(prAdapter, ppcArgs[1],
						     NULL, ucTmp, NULL);
				kalMemSet(arcArgv_size, 0, WLAN_CFG_ARGV_MAX);
				kalMemSet(apcArgv, 0,
					WLAN_CFG_ARGV_MAX * sizeof(int8_t *));
				i4Nargs = 0;
				break;

			}
#if 1
			/*combine the argument to save in temp*/
			pucCurrBuf = ucTmp;
			u4CurrSize = ARRAY_SIZE(ucTmp);

			kalMemZero(ucTmp, WLAN_CFG_VALUE_LEN_MAX);

			if (i4Nargs == 2) {
				/* no space for it, driver can't accept space in
				 * the end of the line
				 */
				/* ToDo: skip the space when parsing */
				kalSnprintf(pucCurrBuf, u4CurrSize, "%s",
					    ppcArgs[1]);
			} else {
				for (i = 1; i < i4Nargs; i++) {
					if (u4CurrSize <= 1) {
						DBGLOG(INIT, ERROR,
						       "write to pucCurrBuf out of bound, i=%d\n",
						       i);
						break;
					}
					u4RetSize = kalScnprintf(pucCurrBuf,
							      u4CurrSize, "%s ",
							      ppcArgs[i]);
					pucCurrBuf += u4RetSize;
					u4CurrSize -= u4RetSize;
				}
			}

			DBGLOG(INIT, TRACE,
			       "Save to driver temp buffer as [%s]\n",
			       ucTmp);
			wlanCfgParseAddEntry(prAdapter, ppcArgs[0], NULL, ucTmp,
					     NULL);
#else
			wlanCfgParseAddEntry(prAdapter, ppcArgs[0], NULL,
					     ppcArgs[1], NULL);
#endif

			if (isFwConfig) {

				uint32_t ret;

				ret = wlanCfgParseToFW(ppcArgs, arcArgv_size,
					i4Nargs, rCmdV1Header.buffer, ucTimes);
				if (ret == WLAN_STATUS_SUCCESS) {
					ucTimes++;
					rCmdV1Header.cmdBufferLen +=
						sizeof(struct CMD_FORMAT_V1);
				}

				if (ucTimes == MAX_CMD_ITEM_MAX) {
					/* Send to FW */
					rCmdV1Header.itemNum = ucTimes;
					rStatus = wlanSendSetQueryCmd(
						/* prAdapter */
						prAdapter,
						/* 0x70 */
						CMD_ID_GET_SET_CUSTOMER_CFG,
						/* fgSetQuery */
						TRUE,
						/* fgNeedResp */
						FALSE,
						/* fgIsOid */
						FALSE,
						/* pfCmdDoneHandler*/
						NULL,
						/* pfCmdTimeoutHandler */
						NULL,
						/* u4SetQueryInfoLen */
						sizeof(struct CMD_HEADER),
						/* pucInfoBuffer */
						(uint8_t *) &rCmdV1Header,
						/* pvSetQueryBuffer */
						NULL,
						/* u4SetQueryBufferLen */
						0);

					if (rStatus == WLAN_STATUS_FAILURE)
						DBGLOG(INIT, INFO,
						       "kalIoctl wifiSefCFG fail 0x%x\n",
						       rStatus);
					DBGLOG(INIT, INFO,
					       "kalIoctl wifiSefCFG num:%d X\n",
					       ucTimes);
					kalMemSet(rCmdV1Header.buffer, 0,
						  MAX_CMD_BUFFER_LENGTH);
					rCmdV1Header.cmdBufferLen = 0;
					ucTimes = 0;
				}

			}

#endif
			kalMemSet(arcArgv_size, 0, WLAN_CFG_ARGV_MAX);
			kalMemSet(apcArgv, 0,
				WLAN_CFG_ARGV_MAX * sizeof(int8_t *));
			i4Nargs = 0;
			break;

		case STATE_TEXT:
			if (i4Nargs >= 0 && i4Nargs < WLAN_CFG_ARGV_MAX) {
				ppcArgs[i4Nargs++] = state.text;
				arcArgv_size[i4Nargs - 1] = state.textsize;
				state.textsize = 0;
				DBGLOG(INIT, INFO,
				       " nargs= %d STATE_TEXT = %s, SIZE = %d\n",
				       i4Nargs - 1, ppcArgs[i4Nargs - 1],
				       arcArgv_size[i4Nargs - 1]);
			}
			break;
		}
	}

exit:
	if (ucTimes != 0 && isFwConfig) {
		/* Send to FW */
		rCmdV1Header.itemNum = ucTimes;

		DBGLOG(INIT, INFO, "cmdV1Header.itemNum:%d\n",
		       rCmdV1Header.itemNum);
		rStatus = wlanSendSetQueryCmd(
			  prAdapter,	/* prAdapter */
			  CMD_ID_GET_SET_CUSTOMER_CFG,	/* 0x70 */
			  TRUE,		/* fgSetQuery */
			  FALSE,	/* fgNeedResp */
			  FALSE,	/* fgIsOid */
			  NULL,		/* pfCmdDoneHandler*/
			  NULL,		/* pfCmdTimeoutHandler */
			  sizeof(struct CMD_HEADER), /* u4SetQueryInfoLen */
			  (uint8_t *) &rCmdV1Header, /* pucInfoBuffer */
			  NULL,		/* pvSetQueryBuffer */
			  0		/* u4SetQueryBufferLen */
			  );

		if (rStatus == WLAN_STATUS_FAILURE)
			DBGLOG(INIT, WARN, "kalIoctl wifiSefCFG fail 0x%x\n",
			       rStatus);

		DBGLOG(INIT, WARN, "kalIoctl wifiSefCFG num:%d X\n",
		       ucTimes);
		kalMemSet(rCmdV1Header.buffer, 0, MAX_CMD_BUFFER_LENGTH);
		rCmdV1Header.cmdBufferLen = 0;
		ucTimes = 0;
	}
	return WLAN_STATUS_SUCCESS;
}

#else
uint32_t wlanCfgParse(struct ADAPTER *prAdapter,
		      uint8_t *pucConfigBuf, uint32_t u4ConfigBufLen)
{

	struct WLAN_CFG_PARSE_STATE_S state;
	int8_t *apcArgv[WLAN_CFG_ARGV_MAX];
	int8_t **args;
	int32_t nargs;

	if (pucConfigBuf == NULL) {
		DBGLOG(INIT, ERROR, "pucConfigBuf is NULL\n");
		return WLAN_STATUS_FAILURE;
	}
	if (kalStrnLen(pucConfigBuf, 4000) >= 4000) {
		DBGLOG(INIT, ERROR, "pucConfigBuf >= 4000\n");
		return WLAN_STATUS_FAILURE;
	}
	if (u4ConfigBufLen == 0)
		return WLAN_STATUS_FAILURE;
	args = apcArgv;
	nargs = 0;
	state.ptr = pucConfigBuf;
	state.nexttoken = 0;
	state.maxSize = u4ConfigBufLen;

	for (;;) {
		switch (wlanCfgFindNextTokenWithEqual(&state)) {
		case STATE_EOF:
			if (nargs > 1)
				wlanCfgParseAddEntry(prAdapter, args[0], NULL,
						     args[1], NULL);
			goto exit;
		case STATE_NEWLINE:
			if (nargs > 1)
				wlanCfgParseAddEntry(prAdapter, args[0], NULL,
						     args[1], NULL);
			/*args[0] is parameter, args[1] is the value*/
			nargs = 0;
			break;
		case STATE_TEXT:
			if (nargs < WLAN_CFG_ARGV_MAX)
				args[nargs++] = state.text;
			break;
		}
	}

exit:
	return WLAN_STATUS_SUCCESS;

#if 0
	/* Old version */
	uint32_t i;
	uint8_t c;
	uint8_t *pbuf;
	uint8_t ucState;
	uint8_t *pucKeyTail = NULL;
	uint8_t *pucKeyHead = NULL;
	uint8_t *pucValueHead = NULL;
	uint8_t *pucValueTail = NULL;

	ucState = WAIT_KEY_HEAD;
	pbuf = pucConfigBuf;

	for (i = 0; i < u4ConfigBufLen; i++) {
		c = pbuf[i];
		if (c == '\r' || c == '\n') {

			if (ucState == WAIT_VALUE_TAIL) {
				/* Entry found */
				if (pucValueHead)
					wlanCfgParseAddEntry(prAdapter,
						pucKeyHead, pucKeyTail,
						pucValueHead, pucValueTail);
			}
			ucState = WAIT_KEY_HEAD;
			pucKeyTail = NULL;
			pucKeyHead = NULL;
			pucValueHead = NULL;
			pucValueTail = NULL;

		} else if (c == '=') {
			if (ucState == WAIT_KEY_TAIL) {
				pucKeyTail = &pbuf[i - 1];
				ucState = WAIT_VALUE_HEAD;
			}
		} else if (c == ' ' || c == '\t') {
			if (ucState == WAIT_KEY_TAIL) {
				pucKeyTail = &pbuf[i - 1];
				ucState = WAIT_VALUE_HEAD;
			}
		} else {

			if (c == '#') {
				/* comments */
				if (ucState == WAIT_KEY_HEAD)
					ucState = WAIT_COMMENT_TAIL;
				else if (ucState == WAIT_VALUE_TAIL)
					pucValueTail = &pbuf[i];

			} else {
				if (ucState == WAIT_KEY_HEAD) {
					pucKeyHead = &pbuf[i];
					pucKeyTail = &pbuf[i];
					ucState = WAIT_KEY_TAIL;
				} else if (ucState == WAIT_VALUE_HEAD) {
					pucValueHead = &pbuf[i];
					pucValueTail = &pbuf[i];
					ucState = WAIT_VALUE_TAIL;
				} else if (ucState == WAIT_VALUE_TAIL)
					pucValueTail = &pbuf[i];
			}
		}

	}			/* for */

	if (ucState == WAIT_VALUE_TAIL) {
		/* Entry found */
		if (pucValueTail)
			wlanCfgParseAddEntry(prAdapter, pucKeyHead, pucKeyTail,
					     pucValueHead, pucValueTail);
	}
#endif

	return WLAN_STATUS_SUCCESS;
}
#endif


uint32_t wlanCfgInit(struct ADAPTER *prAdapter,
		     uint8_t *pucConfigBuf, uint32_t u4ConfigBufLen,
		     uint32_t u4Flags)
{
	struct WLAN_CFG *prWlanCfg;
	struct WLAN_CFG_REC *prWlanCfgRec;
	struct WLAN_CFG *prWlanCfgEm;

	/* P_WLAN_CFG_ENTRY_T prWlanCfgEntry; */
	prAdapter->prWlanCfg = &prAdapter->rWlanCfg;
	prWlanCfg = prAdapter->prWlanCfg;

	prAdapter->prWlanCfgRec = &prAdapter->rWlanCfgRec;
	prWlanCfgRec = prAdapter->prWlanCfgRec;

	prAdapter->prWlanCfgEm = &prAdapter->rWlanCfgEm;
	prWlanCfgEm = prAdapter->prWlanCfgEm;

	kalMemZero(prWlanCfg, sizeof(struct WLAN_CFG));
	ASSERT(prWlanCfg);

	kalMemZero(prWlanCfgEm, sizeof(struct WLAN_CFG));
	ASSERT(prWlanCfgEm);

	prWlanCfg->u4WlanCfgEntryNumMax = WLAN_CFG_ENTRY_NUM_MAX;
	prWlanCfg->u4WlanCfgKeyLenMax = WLAN_CFG_KEY_LEN_MAX;
	prWlanCfg->u4WlanCfgValueLenMax = WLAN_CFG_VALUE_LEN_MAX;

	prWlanCfgRec->u4WlanCfgEntryNumMax =
		WLAN_CFG_REC_ENTRY_NUM_MAX;
	prWlanCfgRec->u4WlanCfgKeyLenMax =
		WLAN_CFG_KEY_LEN_MAX;
	prWlanCfgRec->u4WlanCfgValueLenMax =
		WLAN_CFG_VALUE_LEN_MAX;

	prWlanCfgEm->u4WlanCfgEntryNumMax = WLAN_CFG_ENTRY_NUM_MAX;
	prWlanCfgEm->u4WlanCfgKeyLenMax = WLAN_CFG_KEY_LEN_MAX;
	prWlanCfgEm->u4WlanCfgValueLenMax = WLAN_CFG_VALUE_LEN_MAX;


	DBGLOG(INIT, INFO, "Init wifi config len %u max entry %u\n",
	       u4ConfigBufLen, prWlanCfg->u4WlanCfgEntryNumMax);
#if DBG
	/* self test */
	wlanCfgSet(prAdapter, "ConfigValid", "0x123", WLAN_CFG_DEFAULT);
	if (wlanCfgGetUint32(prAdapter, "ConfigValid", WLAN_CFG_DEFAULT)
		!= 0x123)
		DBGLOG(INIT, INFO, "wifi config error %u\n", __LINE__);

	wlanCfgSet(prAdapter, "ConfigValid", "1", WLAN_CFG_DEFAULT);
	if (wlanCfgGetUint32(prAdapter, "ConfigValid", WLAN_CFG_DEFAULT) != 1)
		DBGLOG(INIT, INFO, "wifi config error %u\n", __LINE__);

#endif
	/*load default value because kalMemZero in this function*/
	wlanLoadDefaultCustomerSetting(prAdapter);

	/* Parse the pucConfigBuf */
	if (pucConfigBuf && (u4ConfigBufLen > 0))
#if CFG_SUPPORT_EASY_DEBUG
		wlanCfgParse(prAdapter, pucConfigBuf, u4ConfigBufLen,
			     FALSE);
#else
		wlanCfgParse(prAdapter, pucConfigBuf, u4ConfigBufLen);
#endif
	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_SUPPORT_CFG_FILE */

int32_t wlanHexToNum(int8_t c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

int32_t wlanHexToByte(int8_t *hex)
{
	int32_t a, b;

	a = wlanHexToNum(*hex++);
	if (a < 0)
		return -1;
	b = wlanHexToNum(*hex++);
	if (b < 0)
		return -1;
	return (a << 4) | b;
}

int32_t wlanHexToArray(int8_t *hexString, int8_t *hexArray, uint8_t arrayLen)
{
	uint8_t converted = 0;
	uint8_t len = (kalStrLen(hexString) + 1)/2;
	uint8_t *tail = hexString + kalStrLen(hexString);

	len = arrayLen < len ? arrayLen : len;
	for (converted = 0; converted < len; converted++) {
		if (kalStrLen(hexString) - converted*2 >= 2)
			hexArray[converted] =
				wlanHexToByte(tail - converted*2 - 2);
		else
			hexArray[converted] =
				wlanHexToNum(*(tail - converted*2 - 1));
	}
	return converted;
}

int32_t wlanHexToArrayR(int8_t *hexString, int8_t *hexArray, uint8_t arrayLen)
{
	uint8_t converted = 0;
	uint8_t len = (kalStrLen(hexString) + 1)/2;

	len = arrayLen < len ? arrayLen : len;
	for (converted = 0; converted < len; converted++) {
		if (converted*2 + 2  <= kalStrLen(hexString))
			hexArray[converted] =
				wlanHexToByte(hexString + converted*2);
		else
			hexArray[converted] =
				wlanHexToNum(*(hexString + converted*2));
	}
	return converted;
}



int32_t wlanHwAddrToBin(int8_t *txt, uint8_t *addr)
{
	int32_t i;
	int8_t *pos = txt;

	for (i = 0; i < 6; i++) {
		int32_t a, b;

		while (*pos == ':' || *pos == '.' || *pos == '-')
			pos++;

		a = wlanHexToNum(*pos++);
		if (a < 0)
			return -1;
		b = wlanHexToNum(*pos++);
		if (b < 0)
			return -1;
		*addr++ = (a << 4) | b;
	}

	return pos - txt;
}

int32_t wlanNumBitSet(uint32_t val)
{
	int32_t c;

	for (c = 0; val; c++)
		val &= val - 1;
	return c;
}

u_int8_t wlanIsChipNoAck(struct ADAPTER *prAdapter)
{
	u_int8_t fgIsNoAck;

	fgIsNoAck = prAdapter->fgIsChipNoAck
#if CFG_CHIP_RESET_SUPPORT
		    || kalIsResetting()
#endif
		    || fgIsBusAccessFailed;

	return fgIsNoAck;
}

u_int8_t wlanIsChipRstRecEnabled(struct ADAPTER
				 *prAdapter)
{
	return prAdapter->rWifiVar.fgChipResetRecover;
}

u_int8_t wlanIsChipAssert(struct ADAPTER *prAdapter)
{
	return prAdapter->rWifiVar.fgChipResetRecover
		&& prAdapter->fgIsChipAssert;
}

void wlanChipRstPreAct(struct ADAPTER *prAdapter)
{
	struct BSS_INFO *prBssInfo = (struct BSS_INFO *) NULL;
	int32_t i4BssIdx;
#if CFG_ENABLE_WIFI_DIRECT
	uint32_t u4ClientCount = 0;
	struct STA_RECORD *prCurrStaRec = (struct STA_RECORD *)
					  NULL;
	struct STA_RECORD *prNextCurrStaRec = (struct STA_RECORD *)
					      NULL;
	struct LINK *prClientList;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
#endif

	KAL_ACQUIRE_MUTEX(prAdapter, MUTEX_CHIP_RST);
	if (prAdapter->fgIsChipAssert) {
		KAL_RELEASE_MUTEX(prAdapter, MUTEX_CHIP_RST);
		return;
	}
	prAdapter->fgIsChipAssert = TRUE;
	KAL_RELEASE_MUTEX(prAdapter, MUTEX_CHIP_RST);

	for (i4BssIdx = 0; i4BssIdx < prAdapter->ucSwBssIdNum;
	     i4BssIdx++) {
		prBssInfo = prAdapter->aprBssInfo[i4BssIdx];

		if (!prBssInfo->fgIsInUse)
			continue;
#if CFG_ENABLE_WIFI_DIRECT
		if (prBssInfo->eNetworkType == NETWORK_TYPE_P2P) {
			if (prBssInfo->eCurrentOPMode == OP_MODE_ACCESS_POINT) {
				u4ClientCount = bssGetClientCount(prAdapter,
								  prBssInfo);
				if (u4ClientCount == 0)
					continue;

				prClientList = &prBssInfo->rStaRecOfClientList;
				LINK_FOR_EACH_ENTRY_SAFE(prCurrStaRec,
				    prNextCurrStaRec, prClientList,
				    rLinkEntry, struct STA_RECORD) {
					if (!prCurrStaRec)
						break;
					kalP2PGOStationUpdate(
					    prAdapter->prGlueInfo,
					    (uint8_t) prBssInfo->u4PrivateData,
					    prCurrStaRec, FALSE);
					LINK_REMOVE_KNOWN_ENTRY(prClientList,
					    &prCurrStaRec->rLinkEntry);
				}
			} else if (prBssInfo->eCurrentOPMode ==
				   OP_MODE_INFRASTRUCTURE) {
				if (prBssInfo->prStaRecOfAP == NULL)
					continue;
				kalP2PGCIndicateConnectionStatus(prGlueInfo,
					(uint8_t) prBssInfo->u4PrivateData,
					NULL, NULL, 0,
					REASON_CODE_DEAUTH_LEAVING_BSS,
					WLAN_STATUS_MEDIA_DISCONNECT_LOCALLY);
				prBssInfo->prStaRecOfAP = NULL;

			}
		}
#endif
	}
}

#if CFG_SUPPORT_TX_LATENCY_STATS
/**
 * wlanCountTxDelayOverLimit() - Count and check whether the TX delay over limit
 *
 * @prAdapter: pointer to Adapter
 * @type: delay type, could be DRIVER_DELAY or MAC_DELAY
 * @u4Latency: the measured latency for one transmitted MSDU
 *
 * This function shall be called on handling each MSDU transmission result.
 */
void wlanCountTxDelayOverLimit(struct ADAPTER *prAdapter,
		enum ENUM_TX_OVER_LIMIT_DELAY_TYPE type,
		uint32_t u4Latency)
{
	struct TX_DELAY_OVER_LIMIT_REPORT_STATS *stats =
			&prAdapter->rTxDelayOverLimitStats;

	if (!stats->fgTxDelayOverLimitReportEnabled)
		return;

	if (stats->eTxDelayOverLimitStatsType == REPORT_AVERAGE) {
		stats->u4Delay[type] += u4Latency;
		stats->u4DelayNum[type]++;
	} else if (u4Latency >= stats->u4DelayLimit[type] &&
		   !stats->fgReported[type]) {
		wlanReportTxDelayOverLimit(prAdapter, type, u4Latency);
		stats->fgReported[type] = true;
	}
}

static void halAddDriverLatencyCount(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint32_t u4DriverLatency)
{
	struct TX_LATENCY_STATS *prCounting;
	uint32_t *pDriverDelay;
	uint32_t *pMaxDriverDelay = prAdapter->rWifiVar.au4DriverTxDelayMax;
	uint8_t i;

	if (ucBssIndex >= BSSID_NUM)
		return;

	prCounting = &prAdapter->rMsduReportStats.rCounting;
	prCounting->au8AccumulatedDelay[DRIVER_TX_DELAY][ucBssIndex] +=
					u4DriverLatency;

	pDriverDelay = prCounting->au4DriverLatency[ucBssIndex];

	for (i = 0; i < LATENCY_STATS_MAX_SLOTS; i++) {
		if (u4DriverLatency <= *pMaxDriverDelay++) {
			GLUE_INC_REF_CNT(pDriverDelay[i]);
			break;
		}
	}

	wlanCountTxDelayOverLimit(prAdapter, DRIVER_DELAY, u4DriverLatency);
}

static void halAddDriverHifLatencyCount(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint32_t u4DriverHifLatency)
{
	struct TX_LATENCY_STATS *prCounting;
	uint32_t *pDriverHifDelay;
	uint32_t *pMaxDriverHifDelay =
		prAdapter->rWifiVar.au4DriverHifTxDelayMax;
	uint8_t i;

	if (ucBssIndex >= BSSID_NUM)
		return;

	prCounting = &prAdapter->rMsduReportStats.rCounting;
	prCounting->au8AccumulatedDelay[DRIVER_HIF_TX_DELAY][ucBssIndex] +=
					u4DriverHifLatency;

	pDriverHifDelay = prCounting->au4DriverHifLatency[ucBssIndex];

	for (i = 0; i < LATENCY_STATS_MAX_SLOTS; i++) {
		if (u4DriverHifLatency <= *pMaxDriverHifDelay++) {
			GLUE_INC_REF_CNT(pDriverHifDelay[i]);
			break;
		}
	}
}
#endif

#if CFG_ENABLE_PKT_LIFETIME_PROFILE && CFG_ENABLE_PER_STA_STATISTICS
void wlanTxLifetimeUpdateStaStats(struct ADAPTER
				  *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	struct STA_RECORD *prStaRec;
	uint32_t u4DeltaTime;
	uint32_t u4DeltaHifTxTime;
	struct PKT_PROFILE *prPktProfile = &prMsduInfo->rPktProfile;
#if 0
	struct QUE_MGT *prQM = &prAdapter->rQM;
	uint32_t u4PktPrintPeriod = 0;
#endif

	prStaRec = cnmGetStaRecByIndex(prAdapter,
				       prMsduInfo->ucStaRecIndex);

	if (prStaRec) {
		u4DeltaTime = (uint32_t) (prPktProfile->rHifTxDoneTimestamp -
				prPktProfile->rHardXmitArrivalTimestamp);
		u4DeltaHifTxTime = (uint32_t) (
				prPktProfile->rHifTxDoneTimestamp -
				prPktProfile->rDequeueTimestamp);

		/* Update StaRec statistics */
		prStaRec->u4TotalTxPktsNumber++;
		prStaRec->u4TotalTxPktsTime += u4DeltaTime;
		prStaRec->u4TotalTxPktsHifTxTime += u4DeltaHifTxTime;

		if (u4DeltaTime > prStaRec->u4MaxTxPktsTime)
			prStaRec->u4MaxTxPktsTime = u4DeltaTime;

		if (u4DeltaHifTxTime > prStaRec->u4MaxTxPktsHifTime)
			prStaRec->u4MaxTxPktsHifTime = u4DeltaHifTxTime;

		if (u4DeltaTime >= NIC_TX_TIME_THRESHOLD)
			prStaRec->u4ThresholdCounter++;

#if 0
		if (u4PktPrintPeriod &&
		    (prStaRec->u4TotalTxPktsNumber >= u4PktPrintPeriod)) {
			DBGLOG(TX, INFO, "[%u]N[%u]A[%u]M[%u]T[%u]E[%4u]\n",
			       prStaRec->ucIndex,
			       prStaRec->u4TotalTxPktsNumber,
			       prStaRec->u4TotalTxPktsTime,
			       prStaRec->u4MaxTxPktsTime,
			       prStaRec->u4ThresholdCounter,
			       prQM->au4QmTcResourceEmptyCounter[
				prStaRec->ucBssIndex][TC2_INDEX]);

			prStaRec->u4TotalTxPktsNumber = 0;
			prStaRec->u4TotalTxPktsTime = 0;
			prStaRec->u4MaxTxPktsTime = 0;
			prStaRec->u4ThresholdCounter = 0;
			prQM->au4QmTcResourceEmptyCounter[
				prStaRec->ucBssIndex][TC2_INDEX] = 0;
		}
#endif
	}
}
#endif

#if CFG_ENABLE_PKT_LIFETIME_PROFILE
u_int8_t wlanTxLifetimeIsProfilingEnabled(
	struct ADAPTER *prAdapter)
{
	u_int8_t fgEnabled = TRUE;
#if CFG_SUPPORT_WFD
	struct WFD_CFG_SETTINGS *prWfdCfgSettings =
		(struct WFD_CFG_SETTINGS *) NULL;

	prWfdCfgSettings =
		&prAdapter->rWifiVar.rWfdConfigureSettings;

	if (prWfdCfgSettings->ucWfdEnable > 0)
		fgEnabled = TRUE;
#endif

	return fgEnabled;
}

u_int8_t wlanTxLifetimeIsTargetMsdu(struct ADAPTER
				    *prAdapter, struct MSDU_INFO *prMsduInfo)
{
	u_int8_t fgResult = TRUE;

#if 0
	switch (prMsduInfo->ucTID) {
	/* BK */
	case 1:
	case 2:

	/* BE */
	case 0:
	case 3:
		fgResult = FALSE;
		break;
	/* VI */
	case 4:
	case 5:

	/* VO */
	case 6:
	case 7:
		fgResult = TRUE;
		break;
	default:
		break;
	}
#endif
	return fgResult;
}

void wlanTxLifetimeTagPacket(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_PROFILING_TAG eTag)
{
	struct PKT_PROFILE *prPktProfile = &prMsduInfo->rPktProfile;
#if CFG_SUPPORT_TX_LATENCY_STATS
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
#endif

#if CFG_SUPPORT_TDLS_AUTO
	struct MSG_AUTO_TDLS_INFO *prAutoTdls =
		(struct MSG_AUTO_TDLS_INFO *) NULL;
#endif

	if (!wlanTxLifetimeIsProfilingEnabled(prAdapter))
		return;

	switch (eTag) {
	case TX_PROF_TAG_OS_TO_DRV:
		/* arrival time is tagged in wlanProcessTxFrame */
		break;

	case TX_PROF_TAG_DRV_ENQUE:
		/* Reset packet profile */
		prPktProfile->fgIsValid = FALSE;
		if (wlanTxLifetimeIsTargetMsdu(prAdapter, prMsduInfo)) {
			/* Enable packet lifetime profiling */
			prPktProfile->fgIsValid = TRUE;

			/* Packet arrival time at kernel Hard Xmit */
			prPktProfile->rHardXmitArrivalTimestamp =
				GLUE_GET_PKT_ARRIVAL_TIME(prMsduInfo->prPacket);

			/* Packet enqueue time */
			prPktProfile->rEnqueueTimestamp = (OS_SYSTIME)
							  kalGetTimeTick();
#if CFG_SUPPORT_TX_LATENCY_STATS
			prPktProfile->u8XmitArrival =
				GLUE_GET_PKT_XTIME(prMsduInfo->prPacket);
#if (CFG_SUPPORT_STATISTICS == 1)
			prPktProfile->u8EnqTime = StatsEnvTimeGet();
#endif
#endif

		}
		break;

	case TX_PROF_TAG_DRV_DEQUE:
		if (prPktProfile->fgIsValid) {
			prPktProfile->rDequeueTimestamp = (OS_SYSTIME)
							  kalGetTimeTick();
#if (CFG_SUPPORT_STATISTICS == 1) && CFG_SUPPORT_TX_LATENCY_STATS
			prPktProfile->u8DeqTime = StatsEnvTimeGet();
#endif
		}
		break;

	case TX_PROF_TAG_DRV_TX_DONE:
		if (prPktProfile->fgIsValid) {
			prPktProfile->rHifTxDoneTimestamp = (OS_SYSTIME)
							    kalGetTimeTick();
#if CFG_SUPPORT_TX_LATENCY_STATS
#if (CFG_SUPPORT_STATISTICS == 1)
			prPktProfile->u8HifTxTime = StatsEnvTimeGet();
#endif
			if (prWifiVar->fgPacketLatencyLog)
				DBGLOG(TX, INFO,
					"Latency(us) HIF_D:%u, DEQ_D:%u, ENQ_D:%u A:%llu BSSIDX:WIDX:PID[%u:%u:%u] IPID:0x%04x SeqNo:%d\n",
					NSEC_TO_USEC((uint32_t)(
					prPktProfile->u8HifTxTime -
					prPktProfile->u8XmitArrival)),
					NSEC_TO_USEC((uint32_t)(
					prPktProfile->u8DeqTime -
					prPktProfile->u8XmitArrival)),
					NSEC_TO_USEC((uint32_t)(
					prPktProfile->u8EnqTime -
					prPktProfile->u8XmitArrival)),
					prPktProfile->u8XmitArrival,
					prMsduInfo->ucBssIndex,
					prMsduInfo->ucWlanIndex,
					prMsduInfo->ucPID,
					GLUE_GET_PKT_IP_ID(
					prMsduInfo->prPacket),
					GLUE_GET_PKT_SEQ_NO(
					prMsduInfo->prPacket));

			halAddDriverLatencyCount(prAdapter,
				prMsduInfo->ucBssIndex,
				((uint32_t)(prPktProfile->u8HifTxTime -
				prPktProfile->u8XmitArrival)) /
				USEC_PER_SEC);
#endif

#if CFG_ENABLE_PKT_LIFETIME_PROFILE && CFG_ENABLE_PER_STA_STATISTICS
			wlanTxLifetimeUpdateStaStats(prAdapter, prMsduInfo);
#endif

#if CFG_SUPPORT_TDLS_AUTO
			prAutoTdls =
				(struct MSG_AUTO_TDLS_INFO *)
				cnmMemAlloc(prAdapter,
				RAM_TYPE_MSG,
				sizeof(struct MSG_AUTO_TDLS_INFO));
			if (!prAutoTdls) {
				DBGLOG(TX, ERROR, "TDLS: MemAlloc Fail");
				ASSERT(FALSE);
				return;
			}
			prAutoTdls->rMsgHdr.eMsgId = MID_TDLS_AUTO;
			prAutoTdls->ucBssIndex = prMsduInfo->ucBssIndex;
			prAutoTdls->u2FrameLength = prMsduInfo->u2FrameLength;
			COPY_MAC_ADDR(prAutoTdls->aucEthDestAddr,
				prMsduInfo->aucEthDestAddr);
			mboxSendMsg(prAdapter, MBOX_ID_0,
				(struct MSG_HDR *) prAutoTdls,
				MSG_SEND_METHOD_BUF);
#endif
		}
		break;
	case TX_PROF_TAG_ACQR_MSDU_TOK:
		if (prPktProfile->fgIsValid) {
#if CFG_SUPPORT_TX_LATENCY_STATS
			prPktProfile->u8HifAcqrMsduTime = StatsEnvTimeGet();

			halAddDriverHifLatencyCount(prAdapter,
				prMsduInfo->ucBssIndex,
				((uint32_t)(prPktProfile->u8HifAcqrMsduTime -
				prPktProfile->u8HifTxTime)) /
				USEC_PER_SEC);
#endif /* CFG_SUPPORT_TX_LATENCY_STATS */
		}
		break;
	default:
		break;
	}
}
#endif

void wlanTxProfilingTagPacket(struct ADAPTER *prAdapter,
			      void *prPacket,
			      enum ENUM_TX_PROFILING_TAG eTag)
{
	if (!prPacket)
		return;

#if CFG_MET_PACKET_TRACE_SUPPORT
	kalMetTagPacket(prAdapter->prGlueInfo, prPacket, eTag);
#endif

	switch (eTag) {
	case TX_PROF_TAG_OS_TO_DRV:
		kalTraceEvent("Xmit ipid=0x%04x seq=%d",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		DBGLOG(TX, TEMP, "Xmit ipid=%d seq=%d\n",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		break;
	case TX_PROF_TAG_DRV_ENQUE:
		kalTraceEvent("Enq ipid=0x%04x seq=%d",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		DBGLOG(TX, TEMP, "Enq ipid=%d seq=%d\n",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		break;
	case TX_PROF_TAG_DRV_FREE:
		kalTraceEvent("Cmpl ipid=0x%04x seq=%d",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		DBGLOG(TX, TEMP, "Cmpl ipid=%d seq=%d\n",
			GLUE_GET_PKT_IP_ID(prPacket),
			GLUE_GET_PKT_SEQ_NO(prPacket));
		break;
	default:
		break;
	}
}

void wlanTxProfilingTagMsdu(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo,
			    enum ENUM_TX_PROFILING_TAG eTag)
{
#if CFG_ENABLE_PKT_LIFETIME_PROFILE
	wlanTxLifetimeTagPacket(prAdapter, prMsduInfo, eTag);
#endif
	if (prMsduInfo->eSrc == TX_PACKET_OS) {
		/* only profile packets from OS */
		wlanTxProfilingTagPacket(prAdapter, prMsduInfo->prPacket, eTag);
	}
}

void wlanUpdateTxStatistics(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo,
			    u_int8_t fgTxDrop)
{
	struct STA_RECORD *prStaRec;
	struct BSS_INFO *prBssInfo;
	enum ENUM_WMM_ACI eAci = WMM_AC_BE_INDEX;
	struct WIFI_WMM_AC_STAT *prAcStats;

	eAci = aucTid2ACI[prMsduInfo->ucUserPriority];

	prStaRec = cnmGetStaRecByIndex(prAdapter,
				       prMsduInfo->ucStaRecIndex);

	if (eAci < WMM_AC_INDEX_NUM) {
		if (prStaRec) {
			if (fgTxDrop)
				prStaRec->arLinkStatistics[eAci].u4TxDropMsdu++;
			else
				prStaRec->arLinkStatistics[eAci].u4TxMsdu++;
		} else {
			prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter,
						  prMsduInfo->ucBssIndex);

			if (prBssInfo) {
				prAcStats = &prBssInfo->arLinkStatistics[eAci];
				if (fgTxDrop)
					prAcStats->u4TxDropMsdu++;
				else
					prAcStats->u4TxMsdu++;
			}
		}
	}

#if CFG_AP_80211KVR_INTERFACE
	prStaRec = cnmGetStaRecByAddress(prAdapter,
				prMsduInfo->ucBssIndex,
				prMsduInfo->aucEthDestAddr);
	if (prStaRec && !fgTxDrop)
		prStaRec->u8TotalTxBytes += prMsduInfo->u2FrameLength;
#endif
}

void wlanUpdateRxStatistics(struct ADAPTER *prAdapter,
			    struct SW_RFB *prSwRfb)
{
	struct STA_RECORD *prStaRec;
	enum ENUM_WMM_ACI eAci = WMM_AC_BE_INDEX;

	eAci = aucTid2ACI[prSwRfb->ucTid];

	prStaRec = cnmGetStaRecByIndex(prAdapter,
				       prSwRfb->ucStaRecIdx);
	if (prStaRec && eAci < WMM_AC_INDEX_NUM)
		prStaRec->arLinkStatistics[eAci].u4RxMsdu++;
}

void wlanReportTxDelayOverLimit(struct ADAPTER *prAdapter,
		enum ENUM_TX_OVER_LIMIT_DELAY_TYPE type, uint32_t delay)
{
#if CFG_SUPPORT_TX_LATENCY_STATS
	char uevent[64];

	if (!prAdapter->rTxDelayOverLimitStats.fgTxDelayOverLimitReportEnabled)
		return;

	DBGLOG(HAL, INFO, "Send uevent abnormaltrx=DIR:TX,event:Ab%sDelay:%u",
			type == DRIVER_DELAY ? "Driver" : "Mac", delay);
	kalSnprintf(uevent, sizeof(uevent),
			"abnormaltrx=DIR:TX,event:Ab%sDelay:%u",
			type == DRIVER_DELAY ? "Driver" : "Mac", delay);
	kalSendUevent(prAdapter, uevent);
#endif
}

/**
 * wlanSetTxDelayOverLimitReport - configure TX delay over limit report
 *
 * @enable: switching the report on/off
 * @isAverage: choice of report types, average or immediate
 * @interval: interval for counting average report (ms)
 * @driver_limit: driver delay limit triggering indication (ms)
 * @mac_limit: MAC delay limit triggering indication (ms)
 */
int wlanSetTxDelayOverLimitReport(struct ADAPTER *prAdapter,
		bool enable, bool isAverage,
		uint32_t interval, uint32_t driver_limit, uint32_t mac_limit)
{
	int32_t rStatus = WLAN_STATUS_NOT_SUPPORTED;
#if CFG_SUPPORT_TX_LATENCY_STATS
	struct TX_DELAY_OVER_LIMIT_REPORT_STATS *prTxDelayOverLimitStats;

	prTxDelayOverLimitStats = &prAdapter->rTxDelayOverLimitStats;
	prTxDelayOverLimitStats->fgTxDelayOverLimitReportEnabled = enable;
	prTxDelayOverLimitStats->eTxDelayOverLimitStatsType =
			isAverage ? REPORT_AVERAGE : REPORT_IMMEDIATE;
	prTxDelayOverLimitStats->fgTxDelayOverLimitReportInterval = interval;
	prTxDelayOverLimitStats->u4DelayLimit[DRIVER_DELAY] = driver_limit;
	prTxDelayOverLimitStats->u4DelayLimit[MAC_DELAY] = mac_limit;

	DBGLOG(INIT, INFO, "en=%u, type=%u, interval=%u, limit=%u/%u",
		prTxDelayOverLimitStats->fgTxDelayOverLimitReportEnabled,
		prTxDelayOverLimitStats->eTxDelayOverLimitStatsType,
		prTxDelayOverLimitStats->fgTxDelayOverLimitReportInterval,
		prTxDelayOverLimitStats->u4DelayLimit[DRIVER_DELAY],
		prTxDelayOverLimitStats->u4DelayLimit[MAC_DELAY]);

	rStatus = WLAN_STATUS_SUCCESS;
#endif
	return rStatus;
}

uint32_t wlanPktTxDone(struct ADAPTER *prAdapter,
		       struct MSDU_INFO *prMsduInfo,
		       enum ENUM_TX_RESULT_CODE rTxDoneStatus)
{
	struct GLUE_INFO *prGlueInfo;
	OS_SYSTIME rCurrent = kalGetTimeTick();
#if CFG_ENABLE_PKT_LIFETIME_PROFILE
	struct PKT_PROFILE *prPktProfile = &prMsduInfo->rPktProfile;
#if CFG_SUPPORT_TX_LATENCY_STATS
	uint64_t u8Now = StatsEnvTimeGet();
	uint32_t u4DelayXmitToHif;
	uint32_t u4DelayHifToDone;
#endif
#endif
	struct EVENT_TX_DONE *prTxDone = prMsduInfo->prTxDone;
#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
	struct WLAN_MAC_HEADER *prWlanHeader = NULL;
	void *pvPacket = NULL;
	uint8_t *pucBuf = NULL;
	uint32_t u4PacketLen = 0;
	u_int8_t fgIsSuccess = FALSE;
#endif
	char aucDelayInfo[80] = {0};

	prGlueInfo = prAdapter->prGlueInfo;

	if (prMsduInfo->ucPktType >= ENUM_PKT_FLAG_NUM)
		prMsduInfo->ucPktType = 0;

#if CFG_ENABLE_PKT_LIFETIME_PROFILE
	if (prPktProfile->fgIsValid &&
		((prMsduInfo->ucPktType == ENUM_PKT_ARP) ||
		(prMsduInfo->ucPktType == ENUM_PKT_DHCP))) {
		if (rCurrent - prPktProfile->rHardXmitArrivalTimestamp > 2000) {
			DBGLOG(TX, INFO,
				"valid %d; ArriveDrv %u, Enq %u, Deq %u, LeaveDrv %u, TxDone %u\n",
				prPktProfile->fgIsValid,
				prPktProfile->rHardXmitArrivalTimestamp,
				prPktProfile->rEnqueueTimestamp,
				prPktProfile->rDequeueTimestamp,
				prPktProfile->rHifTxDoneTimestamp, rCurrent);

			if (prMsduInfo->ucPktType == ENUM_PKT_ARP)
				prGlueInfo->fgTxDoneDelayIsARP = TRUE;
			prGlueInfo->u4ArriveDrvTick =
				prPktProfile->rHardXmitArrivalTimestamp;
			prGlueInfo->u4EnQueTick =
				prPktProfile->rEnqueueTimestamp;
			prGlueInfo->u4DeQueTick =
				prPktProfile->rDequeueTimestamp;
			prGlueInfo->u4LeaveDrvTick =
				prPktProfile->rHifTxDoneTimestamp;
			prGlueInfo->u4CurrTick = rCurrent;
			prGlueInfo->u8CurrTime = kalGetTimeTickNs();
		}
	}
#endif

#if CFG_ENABLE_PKT_LIFETIME_PROFILE
#if CFG_SUPPORT_TX_LATENCY_STATS
	u4DelayXmitToHif = NSEC_TO_USEC((uint32_t)
				(prMsduInfo->rPktProfile.u8HifTxTime -
				 prMsduInfo->rPktProfile.u8XmitArrival));
	u4DelayHifToDone = NSEC_TO_USEC((uint32_t)
				(u8Now - prMsduInfo->rPktProfile.u8HifTxTime));

	/* Unit: TU (1024 micro seconds, 1.024 milliseconds) */
	kalSnprintf(aucDelayInfo, sizeof(aucDelayInfo),
		    "Xmit~Hif:%u.%03lu Hif~Done:%u.%03lu",
		    u4DelayXmitToHif >> 10, u4DelayXmitToHif & BITS(0, 9),
		    u4DelayHifToDone >> 10, u4DelayHifToDone & BITS(0, 9));
#endif
#endif

#if CFG_SUPPORT_MLR
	if (MLR_CHECK_IF_ENABLE_DEBUG(prAdapter))
		DBGLOG(TX, INFO,
			"TX DONE, Type[%s] Tag[0x%08x] WIDX:PID[%u:%u] SN[%d] Status[%u], MAC: "
			MACSTR " SeqNo: %d %s\n",
			TXS_PACKET_TYPE[prMsduInfo->ucPktType],
			prMsduInfo->u4TxDoneTag,
			prMsduInfo->ucWlanIndex,
			prMsduInfo->ucPID,
			prTxDone ? prTxDone->u2SequenceNumber : -1,
			rTxDoneStatus,
			MAC2STR(prMsduInfo->aucEthDestAddr),
			prMsduInfo->ucTxSeqNum,
			aucDelayInfo);
	else
#endif
		DBGLOG_LIMITED(TX, INFO,
			"TX DONE, Type[%s] Tag[0x%08x] WIDX:PID[%u:%u] SN[%d] Status[%u], MAC: "
			MACSTR " SeqNo: %d %s\n",
			TXS_PACKET_TYPE[prMsduInfo->ucPktType],
			prMsduInfo->u4TxDoneTag,
			prMsduInfo->ucWlanIndex,
			prMsduInfo->ucPID,
			prTxDone ? prTxDone->u2SequenceNumber : -1,
			rTxDoneStatus,
			MAC2STR(prMsduInfo->aucEthDestAddr),
			prMsduInfo->ucTxSeqNum,
			aucDelayInfo);

#if (CFG_SUPPORT_CONN_LOG == 1)
	connLogPkt(prAdapter, prMsduInfo, rTxDoneStatus);
#endif
#if CFG_ENABLE_WIFI_DIRECT
	if (prMsduInfo->ucPktType == ENUM_PKT_1X)
		p2pRoleFsmNotifyEapolTxStatus(prAdapter, prMsduInfo->ucBssIndex,
				prMsduInfo->eEapolKeyType, rTxDoneStatus);
#endif
#if CFG_SUPPORT_TDLS
	if (prMsduInfo->ucPktType == ENUM_PKT_TDLS)
		TdlsHandleTxDoneStatus(prAdapter, rTxDoneStatus);
#endif /* CFG_SUPPORT_TDLS */

#if CFG_SUPPORT_TX_MGMT_USE_DATAQ
	if (prMsduInfo->ucPktType == ENUM_PKT_802_11_MGMT) {

		u4PacketLen = kalQueryPacketLength(prMsduInfo->prPacket);
		kalGetPacketBufHeadManipulate(prMsduInfo->prPacket, &pucBuf,
			u4PacketLen - prMsduInfo->u2FrameLength);
		prWlanHeader = (struct WLAN_MAC_HEADER *)pucBuf;

		if (prMsduInfo->u4Option & MSDU_OPT_PROTECTED_FRAME)
			prWlanHeader->u2FrameCtrl &=
				~MASK_FC_PROTECTED_FRAME;
		fgIsSuccess = (rTxDoneStatus == TX_RESULT_SUCCESS) ?
				TRUE : FALSE;
		kalIndicateMgmtTxStatus(prGlueInfo, prMsduInfo->u8Cookie,
					fgIsSuccess, pucBuf,
					(uint32_t)prMsduInfo->u2FrameLength,
					prMsduInfo->ucBssIndex);
	}
#endif

	if (GLUE_GET_PKT_IS_CONTROL_PORT_TX(prMsduInfo->prPacket))
		kalIndicateControlPortTxStatus(prAdapter, prMsduInfo,
					       rTxDoneStatus);

	return WLAN_STATUS_SUCCESS;
}
#if (CFG_CE_ASSERT_DUMP == 1)
void wlanCorDumpTimerInit(struct ADAPTER *prAdapter)
{
	cnmTimerInitTimer(prAdapter,
			  &prAdapter->rN9CorDumpTimer,
			  (PFN_MGMT_TIMEOUT_FUNC) wlanN9CorDumpTimeOut,
			  (uintptr_t) NULL);
}

void wlanCorDumpTimerReset(struct ADAPTER *prAdapter)
{

	if (prAdapter->fgN9AssertDumpOngoing) {
		cnmTimerStopTimer(prAdapter,
				  &prAdapter->rN9CorDumpTimer);
		cnmTimerStartTimer(prAdapter,
				   &prAdapter->rN9CorDumpTimer, 5000);
	} else {
		DBGLOG(INIT, INFO,
		       "Cr4, N9 CorDump Is not ongoing, ignore timer reset\n");
	}
}

void wlanN9CorDumpTimeOut(struct ADAPTER *prAdapter,
			  uintptr_t ulParamPtr)
{
	/* Trigger RESET */
	GL_DEFAULT_RESET_TRIGGER(prAdapter, RST_FW_ASSERT_TIMEOUT);
}

#endif

u_int8_t
wlanGetWlanIdxByAddress(struct ADAPTER *prAdapter,
			uint8_t *pucAddr, uint8_t *pucIndex)
{
	uint8_t ucStaRecIdx;
	struct STA_RECORD *prTempStaRec;

	for (ucStaRecIdx = 0; ucStaRecIdx < CFG_STA_REC_NUM;
	     ucStaRecIdx++) {
		prTempStaRec = &(prAdapter->arStaRec[ucStaRecIdx]);
		if (pucAddr) {
			if (prTempStaRec->fgIsInUse &&
			    EQUAL_MAC_ADDR(prTempStaRec->aucMacAddr,
			    pucAddr)) {
				*pucIndex = prTempStaRec->ucWlanIndex;
				return TRUE;
			}
		} else {
			if (prTempStaRec->fgIsInUse
			    && prTempStaRec->ucStaState == STA_STATE_3) {
				*pucIndex = prTempStaRec->ucWlanIndex;
				return TRUE;
			}
		}
	}
	return FALSE;
}


uint8_t *
wlanGetStaAddrByWlanIdx(struct ADAPTER *prAdapter,
			uint8_t ucIndex)
{
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);
	prWtbl = prAdapter->rWifiVar.arWtbl;
	if (ucIndex < WTBL_SIZE) {
		if (prWtbl[ucIndex].ucUsed && prWtbl[ucIndex].ucPairwise)
			return &prWtbl[ucIndex].aucMacAddr[0];
	}
	return NULL;
}

uint32_t
wlanGetStaIdxByWlanIdx(struct ADAPTER *prAdapter,
		       uint8_t ucIndex, uint8_t *pucStaIdx)
{
	struct WLAN_TABLE *prWtbl;

	ASSERT(prAdapter);
	prWtbl = prAdapter->rWifiVar.arWtbl;

	if (ucIndex < WTBL_SIZE) {
		if (prWtbl[ucIndex].ucUsed && prWtbl[ucIndex].ucPairwise) {
			if (prWtbl[ucIndex].ucStaIndex >= CFG_STA_REC_NUM)
				return WLAN_STATUS_FAILURE;

			*pucStaIdx = prWtbl[ucIndex].ucStaIndex;
			return WLAN_STATUS_SUCCESS;
		}
	}
	return WLAN_STATUS_FAILURE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Add dirtiness to neighbor channels of a BSS to estimate channel
 *        quality.
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[in]  prBssDesc        Pointer to the BSS description.
 * \param[in]  u4Dirtiness      Expected dirtiness value.
 * \param[in]  ucCentralChannel Central channel of the given BSS.
 * \param[in]  ucCoveredRange   With ucCoveredRange and ucCentralChannel,
 *                              all the affected channels can be enumerated.
 */
/*----------------------------------------------------------------------------*/
static void
wlanAddDirtinessToAffectedChannels(struct ADAPTER *prAdapter,
				   struct BSS_DESC *prBssDesc,
				   uint32_t u4Dirtiness,
				   uint8_t ucCentralChannel,
				   uint8_t ucCoveredRange)
{
	uint8_t ucIdx, ucStart, ucEnd;
	u_int8_t bIsABand = FALSE;
	uint8_t ucLeftNeighborChannel, ucRightNeighborChannel,
		ucLeftNeighborChannel2 = 0, ucRightNeighborChannel2 = 0,
		ucLeftestCoveredChannel, ucRightestCoveredChannel;
	struct PARAM_GET_CHN_INFO *prGetChnLoad = &
			(prAdapter->rWifiVar.rChnLoadInfo);

	ucLeftestCoveredChannel = ucCentralChannel > ucCoveredRange
				  ?
				  ucCentralChannel - ucCoveredRange : 1;

	ucLeftNeighborChannel = ucLeftestCoveredChannel ?
				ucLeftestCoveredChannel - 1 : 0;

	if (prBssDesc->eBand == BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
		|| prBssDesc->eBand == BAND_6G
#endif
	) {
		bIsABand = TRUE;
	}

	/* align leftest covered ch and left neighbor ch to valid 5g ch */
	if (bIsABand) {
		ucLeftestCoveredChannel += 2;
		ucLeftNeighborChannel -= 1;
	} else {
		/* we select the nearest 2 ch to the leftest covered ch as left
		 * neighbor chs
		 */
		ucLeftNeighborChannel2 = ucLeftNeighborChannel > 1 ?
						ucLeftNeighborChannel - 1 : 0;
	}

	/* handle corner cases of 5g ch*/
	if (prBssDesc->eBand == BAND_5G &&
			ucLeftestCoveredChannel > 14 &&
			ucLeftestCoveredChannel <= 36) {
		ucLeftestCoveredChannel = 36;
		ucLeftNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_5G &&
			ucLeftestCoveredChannel > 64 &&
			ucLeftestCoveredChannel <= 100) {
		ucLeftestCoveredChannel = 100;
		ucLeftNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_5G &&
			ucLeftestCoveredChannel > 144 &&
			ucLeftestCoveredChannel <= 149) {
		ucLeftestCoveredChannel = 149;
		ucLeftNeighborChannel = 0;
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssDesc->eBand == BAND_6G &&
			ucLeftestCoveredChannel < 5) {
		ucLeftestCoveredChannel = 5;
		ucLeftNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_6G &&
			ucLeftestCoveredChannel > 229) {
		ucLeftestCoveredChannel = 229;
		ucLeftNeighborChannel = 0;
	}
#endif

	/*
	 * because ch 14 is 12MHz away to ch13, we must shift the leftest
	 * covered ch and left neighbor ch when central ch is ch 14
	 */
	if (prBssDesc->eBand == BAND_2G4 &&
		ucCentralChannel == 14) {
		ucLeftestCoveredChannel = 13;
		ucLeftNeighborChannel = 12;
		ucLeftNeighborChannel2 = 11;
	}

	ucRightestCoveredChannel = ucCentralChannel +
				   ucCoveredRange;
	ucRightNeighborChannel = ucRightestCoveredChannel + 1;

	/* align rightest covered ch and right neighbor ch to valid 5g ch */
	if (bIsABand) {
		ucRightestCoveredChannel -= 2;
		ucRightNeighborChannel += 1;
	} else {
		/* we select the nearest 2 ch to the rightest covered ch as
		 * right neighbor ch
		 */
		ucRightNeighborChannel2 = ucRightNeighborChannel < 13 ?
						ucRightNeighborChannel + 1 : 0;
	}

	/* handle corner cases */
	if (prBssDesc->eBand == BAND_5G &&
			ucRightestCoveredChannel >= 14 &&
			ucRightestCoveredChannel < 36) {
		if (ucRightestCoveredChannel == 14) {
			ucRightestCoveredChannel = 13;
			ucRightNeighborChannel = 14;
		} else {
			ucRightestCoveredChannel = 14;
			ucRightNeighborChannel = 0;
		}

		ucRightNeighborChannel2 = 0;
	} else if (prBssDesc->eBand == BAND_5G &&
			ucRightestCoveredChannel >= 64 &&
			ucRightestCoveredChannel < 100) {
		ucRightestCoveredChannel = 64;
		ucRightNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_5G &&
			ucRightestCoveredChannel >= 144 &&
			ucRightestCoveredChannel < 149) {
		ucRightestCoveredChannel = 144;
		ucRightNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_5G &&
			ucRightestCoveredChannel >= 165) {
		ucRightestCoveredChannel = 165;
		ucRightNeighborChannel = 0;
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (prBssDesc->eBand == BAND_6G &&
			ucRightestCoveredChannel < 5) {
		ucRightestCoveredChannel = 5;
		ucRightNeighborChannel = 0;
	} else if (prBssDesc->eBand == BAND_6G &&
			ucRightestCoveredChannel > 229) {
		ucRightestCoveredChannel = 229;
		ucRightNeighborChannel = 0;
	}
#endif

	log_dbg(SCN, TEMP, "central ch %u\n", ucCentralChannel);

	ucStart = wlanGetChannelIndex(prBssDesc->eBand,
				ucLeftestCoveredChannel);
	ucEnd = wlanGetChannelIndex(prBssDesc->eBand,
				ucRightestCoveredChannel);
	if (ucStart >= MAX_CHN_NUM || ucEnd >= MAX_CHN_NUM) {
		DBGLOG(SCN, ERROR, "Invalid ch idx of start %u, or end %u\n",
			ucStart, ucEnd);
		return;
	}

	for (ucIdx = ucStart; ucIdx <= ucEnd; ucIdx++) {
		prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness +=
			u4Dirtiness;
		log_dbg(SCN, TEMP, "Add dirtiness %d, to covered ch %d\n",
		       u4Dirtiness,
		       prGetChnLoad->rEachChnLoad[ucIdx].ucChannel);
	}

	if (ucLeftNeighborChannel != 0) {
		ucIdx = wlanGetChannelIndex(prBssDesc->eBand,
					ucLeftNeighborChannel);

		if (ucIdx < MAX_CHN_NUM) {
			prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness +=
				(u4Dirtiness >> 1);
			log_dbg(SCN, TEMP,
				"Add dirtiness %d, to neighbor ch %d\n",
				u4Dirtiness >> 1,
				prGetChnLoad->rEachChnLoad[ucIdx].ucChannel);
		}
	}

	if (ucRightNeighborChannel != 0) {
		ucIdx = wlanGetChannelIndex(prBssDesc->eBand,
					ucRightNeighborChannel);
		if (ucIdx < MAX_CHN_NUM) {
			prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness +=
				(u4Dirtiness >> 1);
			log_dbg(SCN, TEMP,
				"Add dirtiness %d, to neighbor ch %d\n",
				u4Dirtiness >> 1,
				prGetChnLoad->rEachChnLoad[ucIdx].ucChannel);
		}
	}

	if (bIsABand)
		return;

	/* Only necesaary for 2.5G */
	if (ucLeftNeighborChannel2 != 0) {
		ucIdx = wlanGetChannelIndex(prBssDesc->eBand,
					ucLeftNeighborChannel2);
		if (ucIdx < MAX_CHN_NUM) {
			prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness +=
				(u4Dirtiness >> 1);
			log_dbg(SCN, TEMP,
				"Add dirtiness %d, to neighbor ch %d\n",
				u4Dirtiness >> 1,
				prGetChnLoad->rEachChnLoad[ucIdx].ucChannel);
		}
	}

	if (ucRightNeighborChannel2 != 0) {
		ucIdx = wlanGetChannelIndex(prBssDesc->eBand,
					ucRightNeighborChannel2);
		if (ucIdx < MAX_CHN_NUM) {
			prGetChnLoad->rEachChnLoad[ucIdx].u4Dirtiness +=
				(u4Dirtiness >> 1);
			log_dbg(SCN, TEMP,
				"Add dirtiness %d, to neighbor ch %d\n",
				u4Dirtiness >> 1,
				prGetChnLoad->rEachChnLoad[ucIdx].ucChannel);
		}
	}

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief For a scanned BSS, add dirtiness to the channels 1)around its primary
 *        channels and 2) in its working BW to represent the quality degrade.
 *
 * \param[in]  prAdapter        Pointer to the Adapter structure.
 * \param[in]  prBssDesc        Pointer to the BSS description.
 * \param[in]  u4Dirtiness      Expected dirtiness value.
 * \param[in]  bIsIndexOne      True means index 1, False means index 2.
 */
/*----------------------------------------------------------------------------*/
static void
wlanCalculateChannelDirtiness(struct ADAPTER *prAdapter,
			      struct BSS_DESC *prBssDesc, uint32_t u4Dirtiness,
			      u_int8_t bIsIndexOne)
{
	uint8_t ucCoveredRange = 0, ucCentralChannel = 0,
		ucCentralChannel2 = 0;

	if (bIsIndexOne) {
		DBGLOG(SCN, TEMP, "Process dirtiness index 1\n");
		ucCentralChannel = prBssDesc->ucChannelNum;
		ucCoveredRange = 2;
	} else {
		DBGLOG(SCN, TEMP, "Process dirtiness index 2, ");
		switch (prBssDesc->eChannelWidth) {
		case CW_20_40MHZ:
			if (prBssDesc->eSco == CHNL_EXT_SCA) {
				DBGLOG(SCN, TEMP, "BW40\n");
				ucCentralChannel = prBssDesc->ucChannelNum + 2;
				ucCoveredRange = 4;
			} else if (prBssDesc->eSco == CHNL_EXT_SCB) {
				DBGLOG(SCN, TEMP, "BW40\n");
				ucCentralChannel = prBssDesc->ucChannelNum - 2;
				ucCoveredRange = 4;
			} else {
				DBGLOG(SCN, TEMP, "BW20\n");
				ucCentralChannel = prBssDesc->ucChannelNum;
				ucCoveredRange = 2;
			}
			break;
		case CW_80MHZ:
			DBGLOG(SCN, TEMP, "BW80\n");
			ucCentralChannel = prBssDesc->ucCenterFreqS1;
			ucCoveredRange = 8;
			break;
		case CW_160MHZ:
			DBGLOG(SCN, TEMP, "BW160\n");
			ucCentralChannel = prBssDesc->ucCenterFreqS1;
			ucCoveredRange = 16;
			break;
		case CW_80P80MHZ:
			DBGLOG(SCN, TEMP, "BW8080\n");
			ucCentralChannel = prBssDesc->ucCenterFreqS1;
			ucCentralChannel2 = prBssDesc->ucCenterFreqS2;
			ucCoveredRange = 8;
			break;
		default:
			ucCentralChannel = prBssDesc->ucChannelNum;
			ucCoveredRange = 2;
			break;
		};
	}

	wlanAddDirtinessToAffectedChannels(prAdapter, prBssDesc,
					   u4Dirtiness,
					   ucCentralChannel, ucCoveredRange);

	/* 80 + 80 secondary 80 case */
	if (bIsIndexOne || ucCentralChannel2 == 0)
		return;

	wlanAddDirtinessToAffectedChannels(prAdapter, prBssDesc,
					   u4Dirtiness,
					   ucCentralChannel2, ucCoveredRange);
}

void
wlanInitChnLoadInfoChannelList(struct ADAPTER *prAdapter)
{
	uint8_t ucIdx = 0;
	struct PARAM_GET_CHN_INFO *prGetChnLoad = &
			(prAdapter->rWifiVar.rChnLoadInfo);

	for (ucIdx = 0; ucIdx < MAX_CHN_NUM; ucIdx++) {
		prGetChnLoad->rEachChnLoad[ucIdx].eBand =
			wlanGetChannelBandFromIndex(ucIdx);
		prGetChnLoad->rEachChnLoad[ucIdx].ucChannel =
			wlanGetChannelNumFromIndex(ucIdx);
	}
}

uint32_t
wlanCalculateAllChannelDirtiness(struct ADAPTER
				 *prAdapter)
{
	uint32_t rResult = WLAN_STATUS_SUCCESS;
	int32_t i4Rssi = 0;
	struct BSS_DESC *prBssDesc = NULL;
	uint32_t u4Dirtiness = 0;
	struct LINK *prBSSDescList =
				&(prAdapter->rWifiVar.rScanInfo.rBSSDescList);

	LINK_FOR_EACH_ENTRY(prBssDesc, prBSSDescList, rLinkEntry,
			    struct BSS_DESC) {
		i4Rssi = RCPI_TO_dBm(prBssDesc->ucRCPI);

		if (i4Rssi >= ACS_AP_RSSI_LEVEL_HIGH)
			u4Dirtiness = ACS_DIRTINESS_LEVEL_HIGH;
		else if (i4Rssi >= ACS_AP_RSSI_LEVEL_LOW)
			u4Dirtiness = ACS_DIRTINESS_LEVEL_MID;
		else
			u4Dirtiness = ACS_DIRTINESS_LEVEL_LOW;

		DBGLOG(SCN, TEMP, "Found an AP(%s), primary ch %d\n",
		       prBssDesc->aucSSID, prBssDesc->ucChannelNum);

		/* dirtiness index1 */
		wlanCalculateChannelDirtiness(prAdapter, prBssDesc,
					      u4Dirtiness, TRUE);

		/* dirtiness index2 */
		wlanCalculateChannelDirtiness(prAdapter, prBssDesc,
					      u4Dirtiness >> 1, FALSE);
	}

	return rResult;
}

uint8_t
wlanGetChannelIndex(enum ENUM_BAND eBand, uint8_t channel)
{
	uint8_t ucIdx = MAX_CHN_NUM - 1;

	if (eBand == BAND_2G4)
		ucIdx = channel - 1;
	else if (eBand == BAND_5G && channel >= 36 && channel <= 64)
		ucIdx = 14 + (channel - 36) / 4;
	else if (eBand == BAND_5G && channel >= 100 && channel <= 144)
		ucIdx = 14 + 8 + (channel - 100) / 4;
	else if (eBand == BAND_5G && channel >= 149 && channel <= 165)
		ucIdx = 14 + 8 + 12 + (channel - 149) / 4;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eBand == BAND_6G && channel >= 1 && channel <= 233)
		ucIdx = 14 + 8 + 12 + 5 + (channel - 1) / 4;
#endif

	return ucIdx;
}

/*---------------------------------------------------------------------*/
/*!
 * \brief Get ch index by the given ch num; the reverse function of
 *        wlanGetChannelIndex
 *
 * \param[in]    ucIdx                 Channel index
 * \param[out]   ucChannel             Channel number
 */
/*---------------------------------------------------------------------*/

uint8_t
wlanGetChannelNumFromIndex(uint8_t ucIdx)
{
	uint8_t ucChannel = 0;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (ucIdx >= 39)
		ucChannel = ((ucIdx - 39) << 2) + 1;
	else
#endif
	if (ucIdx >= 34)
		ucChannel = ((ucIdx - 34) << 2) + 149;
	else if (ucIdx >= 22)
		ucChannel = ((ucIdx - 22) << 2) + 100;
	else if (ucIdx >= 14)
		ucChannel = ((ucIdx - 14) << 2) + 36;
	else
		ucChannel = ucIdx + 1;

	return ucChannel;
}

enum ENUM_BAND
wlanGetChannelBandFromIndex(uint8_t ucIdx)
{
	enum ENUM_BAND eBand = BAND_NULL;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (ucIdx >= 39)
		eBand = BAND_6G;
	else
#endif
	if (ucIdx >= 14)
		eBand = BAND_5G;
	else
		eBand = BAND_2G4;

	return eBand;
}

void
wlanSortChannel(struct ADAPTER *prAdapter,
		enum ENUM_CHNL_SORT_POLICY ucSortType)
{
	struct PARAM_GET_CHN_INFO *prChnLoadInfo = &
			(prAdapter->rWifiVar.rChnLoadInfo);
	int8_t ucIdx = 0, ucRoot = 0, ucChild = 0;
#if (CFG_SUPPORT_P2PGO_ACS == 1)
	uint8_t i = 0, ucBandIdx = 0, ucNumOfChannel = 0, uc2gChNum = 0;
	struct RF_CHANNEL_INFO aucChannelList[MAX_PER_BAND_CHN_NUM] = { { 0 } };
#endif
	struct PARAM_CHN_RANK_INFO rChnRankInfo;
	/* prepare unsorted ch rank list */
#if (CFG_SUPPORT_P2PGO_ACS == 1)
	if (ucSortType == CHNL_SORT_POLICY_BY_CH_DOMAIN) {
		for (ucBandIdx = BAND_2G4; ucBandIdx < BAND_NUM; ucBandIdx++) {
			rlmDomainGetChnlList(prAdapter, ucBandIdx,
				FALSE, MAX_PER_BAND_CHN_NUM,
				&ucNumOfChannel, aucChannelList);

			DBGLOG(SCN, LOUD, "[ACS]Band=%d, Channel Number=%d\n",
				ucBandIdx,
				ucNumOfChannel);

			for (i = 0; i < ucNumOfChannel; i++) {
				ucIdx = wlanGetChannelIndex(
					aucChannelList[i].eBand,
					aucChannelList[i].ucChannelNum);
				prChnLoadInfo->
					rChnRankList[uc2gChNum+i].eBand =
					prChnLoadInfo->rEachChnLoad[ucIdx].
						eBand;
				prChnLoadInfo->
					rChnRankList[uc2gChNum+i].ucChannel =
					prChnLoadInfo->rEachChnLoad[ucIdx].
						ucChannel;
				prChnLoadInfo->
					rChnRankList[uc2gChNum+i].u4Dirtiness =
					prChnLoadInfo->rEachChnLoad[ucIdx].
						u4Dirtiness;

				DBGLOG(SCN, LOUD, "[ACS]Ch[%d],cIdx[%d]\n",
					aucChannelList[i].ucChannelNum,
					uc2gChNum+i);
				DBGLOG(SCN, LOUD, "[ACS]ChR[%d],eCh[%d]\n",
					prChnLoadInfo->
						rChnRankList[uc2gChNum+i].
						ucChannel,
					prChnLoadInfo->rEachChnLoad[ucIdx].
						ucChannel);
			}
			uc2gChNum = uc2gChNum + ucNumOfChannel;
		}

		/*Set the reset idx to invalid value*/
		for (i = uc2gChNum; i < MAX_CHN_NUM; i++) {
			prChnLoadInfo->rChnRankList[i].u4Dirtiness = 0xFFFFFFFF;
			prChnLoadInfo->rChnRankList[i].ucChannel = 0xFF;

			DBGLOG(SCN, LOUD, "uc2gChNum=%d,[ACS]Chn=%d,D=0x%x\n",
				i,
				prChnLoadInfo->rChnRankList[i].ucChannel,
				prChnLoadInfo->rChnRankList[i].u4Dirtiness);
		}
	} else
#endif
	{
		for (ucIdx = 0; ucIdx < MAX_CHN_NUM; ++ucIdx) {
			prChnLoadInfo->rChnRankList[ucIdx].eBand =
				prChnLoadInfo->rEachChnLoad[ucIdx].eBand;
			prChnLoadInfo->rChnRankList[ucIdx].ucChannel =
				prChnLoadInfo->rEachChnLoad[ucIdx].ucChannel;
			prChnLoadInfo->rChnRankList[ucIdx].u4Dirtiness =
				prChnLoadInfo->rEachChnLoad[ucIdx].u4Dirtiness;

		}
	}
	/* heapify ch rank list */
	for (ucIdx = MAX_CHN_NUM / 2 - 1; ucIdx >= 0; --ucIdx) {
		for (ucRoot = ucIdx; ucRoot * 2 + 1 < MAX_CHN_NUM;
				ucRoot = ucChild) {
			ucChild = ucRoot * 2 + 1;
			/* Coverity check*/
			if (ucChild < 0 || ucChild >= MAX_CHN_NUM ||
			    ucRoot < 0 || ucRoot >= MAX_CHN_NUM)
				break;
			if (ucChild < MAX_CHN_NUM - 1 && prChnLoadInfo->
			    rChnRankList[ucChild + 1].u4Dirtiness >
			    prChnLoadInfo->rChnRankList[ucChild].u4Dirtiness)
				ucChild += 1;
			if (prChnLoadInfo->rChnRankList[ucChild].u4Dirtiness <=
			    prChnLoadInfo->rChnRankList[ucRoot].u4Dirtiness)
				break;
			DBGLOG(SCN, LOUD, "[ACS]root Chn=%d,D=0x%x\n",
					   prChnLoadInfo->
					   rChnRankList[ucRoot].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucRoot].u4Dirtiness);
			DBGLOG(SCN, LOUD, "[ACS]child Chn=%d,D=0x%x\n",
					   prChnLoadInfo->
					   rChnRankList[ucChild].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucChild].u4Dirtiness);
			kalMemCopy(&rChnRankInfo,
				&(prChnLoadInfo->rChnRankList[ucChild]),
				sizeof(struct PARAM_CHN_RANK_INFO));
			kalMemCopy(&prChnLoadInfo->rChnRankList[ucChild],
				&prChnLoadInfo->rChnRankList[ucRoot],
				sizeof(struct PARAM_CHN_RANK_INFO));
			kalMemCopy(&prChnLoadInfo->rChnRankList[ucRoot],
				&rChnRankInfo,
				sizeof(struct PARAM_CHN_RANK_INFO));
			DBGLOG(SCN, LOUD,
				"[ACS]After root uChn=%d,D=0x%x\n",
					   prChnLoadInfo->
					   rChnRankList[ucRoot].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucRoot].u4Dirtiness);
			DBGLOG(SCN, LOUD,
				"[ACS]AFter child Chn=%d,D=0x%x\n",
					   prChnLoadInfo->
					   rChnRankList[ucChild].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucChild].u4Dirtiness);
			}
	}
	/* sort ch rank list */
	for (ucIdx = MAX_CHN_NUM - 1; ucIdx > 0; ucIdx--) {
		rChnRankInfo = prChnLoadInfo->rChnRankList[0];
		prChnLoadInfo->rChnRankList[0] =
			prChnLoadInfo->rChnRankList[ucIdx];
		prChnLoadInfo->rChnRankList[ucIdx] = rChnRankInfo;
		for (ucRoot = 0; ucRoot * 2 + 1 < ucIdx; ucRoot = ucChild) {
			ucChild = ucRoot * 2 + 1;
			/* Coverity check*/
			if (ucChild < 0 || ucChild >= MAX_CHN_NUM ||
			    ucRoot < 0 || ucRoot >= MAX_CHN_NUM)
				break;
			if (ucChild < ucIdx - 1 && prChnLoadInfo->
			    rChnRankList[ucChild + 1].u4Dirtiness >
			    prChnLoadInfo->rChnRankList[ucChild].u4Dirtiness)
				ucChild += 1;
			if (prChnLoadInfo->rChnRankList[ucChild].u4Dirtiness <=
			    prChnLoadInfo->rChnRankList[ucRoot].u4Dirtiness)
				break;
			DBGLOG(SCN, LOUD,
				"[ACS]root ChNum=%d D=0x%x",
					   prChnLoadInfo->
					   rChnRankList[ucRoot].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucRoot].u4Dirtiness);
			DBGLOG(SCN, LOUD, "[ACS]child ChNum=%d D=0x%x\n",
					   prChnLoadInfo->
					   rChnRankList[ucChild].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucChild].u4Dirtiness);
			kalMemCopy(&rChnRankInfo,
				&(prChnLoadInfo->rChnRankList[ucChild]),
				sizeof(struct PARAM_CHN_RANK_INFO));
			kalMemCopy(&prChnLoadInfo->rChnRankList[ucChild],
				&prChnLoadInfo->rChnRankList[ucRoot],
				sizeof(struct PARAM_CHN_RANK_INFO));
			kalMemCopy(&prChnLoadInfo->rChnRankList[ucRoot],
				&rChnRankInfo,
				sizeof(struct PARAM_CHN_RANK_INFO));
			DBGLOG(SCN, LOUD,
				"[ACS]New root ChNum=%d D=0x%x",
					   prChnLoadInfo->
					   rChnRankList[ucRoot].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucRoot].u4Dirtiness);

			DBGLOG(SCN, LOUD,
				"[ACS]New child ChNum=%d D=0x%x",
					   prChnLoadInfo->
					   rChnRankList[ucChild].ucChannel,
					   prChnLoadInfo->
					   rChnRankList[ucChild].u4Dirtiness);
		}
	}

	for (ucIdx = 0; ucIdx < MAX_CHN_NUM; ++ucIdx) {
		DBGLOG(SCN, LOUD, "[ACS]band=%d,channel=%d,dirtiness=0x%x\n",
			prChnLoadInfo->rChnRankList[ucIdx].eBand,
			prChnLoadInfo->rChnRankList[ucIdx].ucChannel,
			prChnLoadInfo->rChnRankList[ucIdx].u4Dirtiness);
	}

}

#if ((CFG_SISO_SW_DEVELOP == 1) || (CFG_SUPPORT_SPE_IDX_CONTROL == 1))
uint8_t
wlanGetAntPathType(struct ADAPTER *prAdapter,
		   enum ENUM_WF_PATH_FAVOR_T eWfPathFavor)
{
	uint8_t ucFianlWfPathType = eWfPathFavor;
#if (CFG_SUPPORT_SPE_IDX_CONTROL == 1)
	uint8_t ucNss = prAdapter->rWifiVar.ucNSS;
	uint8_t ucSpeIdxCtrl = prAdapter->rWifiVar.ucSpeIdxCtrl;

	if (ucNss <= 2) {
		if (ucSpeIdxCtrl == 0)
			ucFianlWfPathType = ENUM_WF_0_ONE_STREAM_PATH_FAVOR;
		else if (ucSpeIdxCtrl == 1)
			ucFianlWfPathType = ENUM_WF_1_ONE_STREAM_PATH_FAVOR;
		else if (ucSpeIdxCtrl == 2) {
			if (ucNss > 1)
				ucFianlWfPathType =
					ENUM_WF_0_1_DUP_STREAM_PATH_FAVOR;
			else
				ucFianlWfPathType = ENUM_WF_NON_FAVOR;
		} else
			ucFianlWfPathType = ENUM_WF_NON_FAVOR;
	}
#endif
	return ucFianlWfPathType;
}

uint8_t
wlanAntPathFavorSelect(struct ADAPTER *prAdapter,
		       enum ENUM_WF_PATH_FAVOR_T eWfPathFavor)
{
	uint8_t ucRetValSpeIdx = 0x18;
#if (CFG_SUPPORT_SPE_IDX_CONTROL == 1)
	uint8_t ucNss = prAdapter->rWifiVar.ucNSS;

	if (ucNss <= 2) {
		if ((eWfPathFavor == ENUM_WF_NON_FAVOR) ||
			(eWfPathFavor == ENUM_WF_0_ONE_STREAM_PATH_FAVOR) ||
			(eWfPathFavor == ENUM_WF_0_1_TWO_STREAM_PATH_FAVOR))
			ucRetValSpeIdx = ANTENNA_WF0;
		else if (eWfPathFavor == ENUM_WF_0_1_DUP_STREAM_PATH_FAVOR)
			ucRetValSpeIdx = 0x18;
		else if (eWfPathFavor == ENUM_WF_1_ONE_STREAM_PATH_FAVOR)
			ucRetValSpeIdx = ANTENNA_WF1;
		else
			ucRetValSpeIdx = ANTENNA_WF0;
	}
#endif
	return ucRetValSpeIdx;
}
#endif

uint8_t
wlanGetSpeIdx(struct ADAPTER *prAdapter,
	      uint8_t ucBssIndex,
	      enum ENUM_WF_PATH_FAVOR_T eWfPathFavor)
{
	uint8_t ucRetValSpeIdx = 0;
#if ((CFG_SISO_SW_DEVELOP == 1) || (CFG_SUPPORT_SPE_IDX_CONTROL == 1))
	struct BSS_INFO *prBssInfo;
	enum ENUM_BAND eBand = BAND_NULL;

	if (ucBssIndex > prAdapter->ucSwBssIdNum) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIndex);
		return ucRetValSpeIdx;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(SW4, ERROR,
			"Invalid bss idx: %d\n",
			ucBssIndex);
		return ucRetValSpeIdx;
	}
	/*
	 * if DBDC enable return 0, else depend 2.4G/5G & support WF path
	 * retrun accurate value
	 */
	if (prAdapter->chip_info->eDefaultDbdcMode == ENUM_DBDC_MODE_STATIC ||
	    !prAdapter->rWifiVar.fgDbDcModeEn) {
		eBand = prBssInfo->eBand;

		if (eBand == BAND_2G4) {
			if (IS_WIFI_2G4_SISO(prAdapter)) {
				if (IS_WIFI_2G4_WF0_SUPPORT(prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else
					ucRetValSpeIdx = ANTENNA_WF1;
			} else {
				if (IS_WIFI_SMART_GEAR_SUPPORT_WF0_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else if (IS_WIFI_SMART_GEAR_SUPPORT_WF1_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF1;
				else
					ucRetValSpeIdx = wlanAntPathFavorSelect(
						prAdapter, eWfPathFavor);
			}
		} else if (eBand == BAND_5G) {
			if (IS_WIFI_5G_SISO(prAdapter)) {
				if (IS_WIFI_5G_WF0_SUPPORT(prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else
					ucRetValSpeIdx = ANTENNA_WF1;
			} else {
				if (IS_WIFI_SMART_GEAR_SUPPORT_WF0_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else if (IS_WIFI_SMART_GEAR_SUPPORT_WF1_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF1;
				else
					ucRetValSpeIdx = wlanAntPathFavorSelect(
						prAdapter, eWfPathFavor);
			}
#if (CFG_SUPPORT_WIFI_6G == 1)
		} else if (eBand == BAND_6G) {
			if (IS_WIFI_6G_SISO(prAdapter)) {
				if (IS_WIFI_6G_WF0_SUPPORT(prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else
					ucRetValSpeIdx = ANTENNA_WF1;
			} else {
				if (IS_WIFI_SMART_GEAR_SUPPORT_WF0_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF0;
				else if (IS_WIFI_SMART_GEAR_SUPPORT_WF1_SISO(
				    prAdapter))
					ucRetValSpeIdx = ANTENNA_WF1;
				else
					ucRetValSpeIdx = wlanAntPathFavorSelect(
						prAdapter, eWfPathFavor);
			}
#endif
		} else
			ucRetValSpeIdx = wlanAntPathFavorSelect(prAdapter,
				eWfPathFavor);
	}
	DBGLOG(INIT, TRACE,
		"SpeIdx:%d,D:%d,RfBand=%d,Bss=%d,HwBand=%d,BkHwBand=%d\n",
		ucRetValSpeIdx, prAdapter->rWifiVar.fgDbDcModeEn,
		eBand, ucBssIndex,
		prBssInfo->eHwBandIdx, prBssInfo->eBackupHwBandIdx);
#endif
	return ucRetValSpeIdx;
}

uint8_t
wlanGetSupportNss(struct ADAPTER *prAdapter,
		  uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
	struct BSS_DESC *prBssDesc;
#endif
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	struct MLD_BSS_INFO *mld_bssinfo;
#endif
#endif

	uint8_t ucRetValNss = prAdapter->rWifiVar.ucNSS;
#if CFG_SISO_SW_DEVELOP
	enum ENUM_BAND eBand = BAND_NULL;
#endif
#if (CFG_SUPPORT_POWER_THROTTLING == 1 && CFG_SUPPORT_CNM_POWER_CTRL == 1)
	if (prAdapter->fgPowerForceOneNss) {
		DBGLOG(INIT, TRACE, "Force 1 Nss\n",
		       ucBssIndex);
		return 1;
	}
#endif

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(CNM, ERROR,
			"Invalid bss idx: %d\n",
			ucBssIndex);
		return 1;
	}

/*The chip capability as DBDC 1x1, we should set
 *the NSS capability as 1x1 in STR mode. It is no
 *need to switch OP mode by OMN/OMI.It can avoid
 *some IOT issue.
 */
#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1)
#if (CFG_SUPPORT_802_11BE_MLO == 1)
	mld_bssinfo = mldBssGetByBss(prAdapter, prBssInfo);
	if (IS_MLD_BSSINFO_MULTI(mld_bssinfo) &&
		(mld_bssinfo->ucMaxSimuLinks >= 1 ||
		(mld_bssinfo->ucMaxSimuLinks == 0 &&
		 mld_bssinfo->ucEmlEnabled == FALSE &&
		 mld_bssinfo->ucHmloEnabled == FALSE)) &&
		 prAdapter->rWifiVar.fgDbDcModeEn == TRUE) {
		DBGLOG(CNM, INFO, "STR mode work in 1SS\n");
		return 1;
	}

#endif
#endif

#if CFG_ENABLE_WIFI_DIRECT
	if (IS_BSS_APGO(prBssInfo)) {
		if (p2pFuncIsAPMode(
			prAdapter->rWifiVar.prP2PConnSettings
			[prBssInfo->u4PrivateData])) {
			if (prBssInfo->eBand == BAND_2G4)
				ucRetValNss = prAdapter->rWifiVar.ucAp2gNSS;
			else if (prBssInfo->eBand == BAND_5G)
				ucRetValNss = prAdapter->rWifiVar.ucAp5gNSS;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucRetValNss = prAdapter->rWifiVar.ucAp6gNSS;
#endif
		} else {
			if (prBssInfo->eBand == BAND_2G4)
				ucRetValNss = prAdapter->rWifiVar.ucGo2gNSS;
			else if (prBssInfo->eBand == BAND_5G)
				ucRetValNss = prAdapter->rWifiVar.ucGo5gNSS;
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (prBssInfo->eBand == BAND_6G)
				ucRetValNss = prAdapter->rWifiVar.ucGo6gNSS;
#endif
		}
	}
#endif /* CFG_ENABLE_WIFI_DIRECT */
#if CFG_SUPPORT_IOT_AP_BLOCKLIST
	else if (IS_BSS_AIS(prBssInfo)) {
		struct AIS_FSM_INFO *prAisFsmInfo =
			aisGetAisFsmInfo(prAdapter, ucBssIndex);

		if (prAisFsmInfo) {
			prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIndex);
			if (prBssDesc != NULL &&
			    bssIsIotAp(prAdapter, prBssDesc,
				       WLAN_IOT_AP_DBDC_1SS)) {
				DBGLOG(SW4, INFO, "Use 1x1 due to DBDC blk\n");
				ucRetValNss = 1;
			} else if (prAdapter->rWifiVar.fgSta1NSS) {
				DBGLOG(SW4, INFO, "Use 1x1 due to FWK cmd\n");
				ucRetValNss = 1;
			}
		}
	}
#endif

#if (CFG_SUPPORT_DBDC_DOWNGRADE_NSS == 1) && CFG_ENABLE_WIFI_DIRECT
	if (IS_BSS_P2P(prBssInfo) &&
		(p2pFuncIsDualAPMode(prAdapter) ||
		p2pFuncIsDualGOMode(prAdapter))) {
		DBGLOG(SW4, LOUD, "Use 1x1 due to dual ap/go mode\n");
		ucRetValNss = 1;
	}
#endif

	if (ucRetValNss > prAdapter->rWifiVar.ucNSS)
		ucRetValNss = prAdapter->rWifiVar.ucNSS;

#if CFG_SISO_SW_DEVELOP
	if (ucBssIndex > prAdapter->ucSwBssIdNum) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u], skip dump!\n",
		       ucBssIndex);
		return ucRetValNss;
	}
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	/*
	 * depend 2.4G/5G support SISO/MIMO
	 * retrun accurate value
	 */
	eBand = prBssInfo->eBand;

	if ((eBand == BAND_2G4) && IS_WIFI_2G4_SISO(prAdapter))
		ucRetValNss = 1;
	else if ((eBand == BAND_5G) && IS_WIFI_5G_SISO(prAdapter))
		ucRetValNss = 1;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if ((eBand == BAND_6G) && IS_WIFI_6G_SISO(prAdapter))
		ucRetValNss = 1;
#endif
	DBGLOG(INIT, TRACE, "Nss=%d,B=%d,Bss=%d\n",
	       ucRetValNss, eBand, ucBssIndex);
#endif

	return ucRetValNss;
}

#if CFG_SUPPORT_LOWLATENCY_MODE
/*----------------------------------------------------------------------------*/
/*!
 * \brief This is a private routine, which is used to initialize the variables
 *        for low latency mode.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanAdapterStartForLowLatency(struct ADAPTER *prAdapter)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;

	/* Default disable low latency mode */
	prAdapter->fgEnLowLatencyMode = FALSE;

	/* Default enable scan */
	prAdapter->fgEnCfg80211Scan = TRUE;

	return u4Status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This is a private routine, which is used to initialize the variables
 *        for low latency mode.
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 *
 * \retval WLAN_STATUS_SUCCESS: Success
 * \retval WLAN_STATUS_FAILURE: Failed
 */
/*----------------------------------------------------------------------------*/
uint32_t
wlanConnectedForLowLatency(struct ADAPTER *prAdapter, uint8_t ucBssIndex)
{
	uint32_t u4Events = 0;

	/* Query setting from wifi adaptor module */
#if CFG_MTK_ANDROID_WMT
	u4Events = get_low_latency_mode();
#endif

	/* Set low latency mode */
	DBGLOG(AIS, INFO, "LowLatency(Connected) event:0x%x\n", u4Events);
	wlanSetLowLatencyMode(prAdapter, u4Events, ucBssIndex);

	return WLAN_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to enable/disable low latency mode
 * \param[in]  prAdapter       A pointer to the Adapter structure.
 * \param[in]  pvSetBuffer     A pointer to the buffer that holds the
 *                             OID-specific data to be set.
 * \param[in]  u4SetBufferLen  The number of bytes the set buffer.
 * \param[out] pu4SetInfoLen   Points to the number of bytes it read or is
 *                             needed
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSetLowLatencyMode(
	struct ADAPTER *prAdapter,
	uint32_t u4Events, uint8_t ucBssIndex)
{
	struct BSS_INFO *prBssInfo;
	u_int8_t fgEnMode = FALSE; /* Low Latency Mode */
	u_int8_t fgEnScan = FALSE; /* Scan management */
	u_int8_t fgEnPM = TRUE; /* Power management */
	u_int8_t fgEnRoaming = TRUE; /* Roaming management */
	struct PARAM_POWER_MODE_ rPowerMode;
	struct WIFI_VAR *prWifiVar = NULL;
	char arCmd[64]; /* Roaming command buffer */

	ASSERT(prAdapter);

	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo) {
		DBGLOG(SW4, INFO, "Invalid BssInfo index[%u]\n", ucBssIndex);
		return WLAN_STATUS_INVALID_DATA;
	}

	/* Initialize */
	prWifiVar = &prAdapter->rWifiVar;

#define LOW_LATENCY_LOG_TEMPLATE \
	"LowLatency(gaming) event - gas:0x%x, net:0x%x, " \
	"allowlist:0x%x, disable roaming:0x%x, CAM:0x%x, " \
	"scan=%u, reorder=%u, power=%u\n"

	DBGLOG(OID, INFO, LOW_LATENCY_LOG_TEMPLATE,
		(u4Events & GED_EVENT_GAS),
		(u4Events & GED_EVENT_NETWORK),
		(u4Events & GED_EVENT_DOPT_WIFI_SCAN),
		(u4Events & GED_EVENT_DISABLE_ROAMING),
		(u4Events & GED_EVENT_CAM_MODE),
		(uint32_t)prWifiVar->ucLowLatencyModeScan,
		(uint32_t)prWifiVar->ucLowLatencyModeReOrder,
		(uint32_t)prWifiVar->ucLowLatencyModePower);

	rPowerMode.ucBssIdx = ucBssIndex;

	/* Enable/disable low latency mode decision:
	 *
	 * Enable if it's GAS and network event
	 * and the Glue media state is connected.
	 */
	if ((u4Events & GED_EVENT_GAS) != 0
		&& (u4Events & GED_EVENT_NETWORK) != 0
		&& MEDIA_STATE_CONNECTED
			== kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex))
		fgEnMode = TRUE; /* It will enable low latency mode */

#if CFG_FAST_PATH_SUPPORT
	if (fgEnMode != prAdapter->fgEnLowLatencyMode) {
		if (!fgEnMode && (MEDIA_STATE_CONNECTED
			== kalGetMediaStateIndicated(prAdapter->prGlueInfo,
			ucBssIndex)))
			mscsDeactivate(prAdapter,
				aisGetStaRecOfAP(prAdapter, AIS_DEFAULT_INDEX));
	}
#endif

	/* Enable/disable scan management decision:
	 *
	 * Enable if it will enable low latency mode.
	 * Or, enable if it is a allow list event.
	 */
	if (fgEnMode != TRUE || (u4Events & GED_EVENT_DOPT_WIFI_SCAN) != 0)
		fgEnScan = TRUE; /* It will enable scan management */

	/* Enable/disable power management decision:
	 *
	 * Enable constantly awake mode (equivalent to disable power management)
	 * if user manually set in low latency mode
	 */
	if (fgEnMode == TRUE && (u4Events & GED_EVENT_CAM_MODE) != 0)
		fgEnPM = FALSE;

	/* Enable/disable roaming decision:
	 *
	 * Disable roaming if user manually set in low latency mode
	 */
	if (fgEnMode == TRUE && (u4Events & GED_EVENT_DISABLE_ROAMING) != 0)
		fgEnRoaming = FALSE; /* It will disable roaming */

	/* Debug log for the actions
	 * log when low latency mode, scan setting changes
	 * or PS, roaming is manually disable under low latency mode
	 */
	if (fgEnMode != prAdapter->fgEnLowLatencyMode
		|| fgEnScan != prAdapter->fgEnCfg80211Scan
		|| (fgEnMode && !fgEnPM)
		|| (fgEnMode && !fgEnRoaming)) {
		DBGLOG(OID, INFO,
			"LowLatency(gaming) change (m:%d,s:%d,PM:%d,r:%d))\n",
			fgEnMode, fgEnScan, fgEnPM, fgEnRoaming);
	}

	/* Scan management:
	 *
	 * Disable/enable scan
	 */
	if ((prWifiVar->ucLowLatencyModeScan == FEATURE_ENABLED) &&
	    (fgEnScan != prAdapter->fgEnCfg80211Scan))
		prAdapter->fgEnCfg80211Scan = fgEnScan;

	if ((prWifiVar->ucLowLatencyModeReOrder == FEATURE_ENABLED) &&
	    (fgEnMode != prAdapter->fgEnLowLatencyMode)) {
		/* Queue management:
		 *
		 * Change QM RX BA timeout if the gaming mode state changed
		 */
		if (fgEnMode) {
			prAdapter->u4QmRxBaMissTimeout
				= prWifiVar->u4BaShortMissTimeoutMs;
		} else {
			prAdapter->u4QmRxBaMissTimeout
				= prWifiVar->u4BaMissTimeoutMs;
		}
	}

	/* Power management:
	 *
	 * Set power saving mode profile to FW
	 *
	 * Do if 1. the power saving caller including GPU
	 * and 2. it will disable low latency mode.
	 * Or, do if 1. the power saving caller is not including GPU
	 * and 2. it will enable low latency mode.
	 */
	if (prWifiVar->ucLowLatencyModePower == FEATURE_ENABLED) {
		if (fgEnPM == FALSE)
			rPowerMode.ePowerMode = Param_PowerModeCAM;
		else
			rPowerMode.ePowerMode = Param_PowerModeFast_PSP;

		nicConfigPowerSaveProfile(prAdapter, rPowerMode.ucBssIdx,
			rPowerMode.ePowerMode, FALSE, PS_CALLER_GPU);
	}

	/* Roaming management:
	 *
	 * Disable/enable roaming
	 */
	kalSnprintf(arCmd, sizeof(arCmd), "RoamingEnable %d", fgEnRoaming?1:0);
	wlanChipCommand(prAdapter, &arCmd[0], sizeof(arCmd));

	if (fgEnMode != prAdapter->fgEnLowLatencyMode)
		prAdapter->fgEnLowLatencyMode = fgEnMode;

	DBGLOG(OID, INFO,
		"LowLatency(gaming) fgEnMode=[%d]\n", fgEnMode);

	/* Force RTS to protect game packet */
	wlanSetForceRTS(prAdapter, fgEnMode);

	return WLAN_STATUS_SUCCESS;
}

#endif /* CFG_SUPPORT_LOWLATENCY_MODE */

#if CFG_SUPPORT_EASY_DEBUG

void wlanCfgFwSetParam(uint8_t *fwBuffer, char *cmdStr, char *value, int num,
		       int type)
{
	struct CMD_FORMAT_V1 *cmd = (struct CMD_FORMAT_V1 *)fwBuffer + num;

	kalMemSet(cmd, 0, sizeof(struct CMD_FORMAT_V1));
	cmd->itemType = type;

	cmd->itemStringLength = strlen(cmdStr);
	if (cmd->itemStringLength > MAX_CMD_NAME_MAX_LENGTH)
		cmd->itemStringLength = MAX_CMD_NAME_MAX_LENGTH;

	/* here will not ensure the end will be '\0' */
	kalMemCopy(cmd->itemString, cmdStr, cmd->itemStringLength);

	cmd->itemValueLength = strlen(value);
	if (cmd->itemValueLength > MAX_CMD_VALUE_MAX_LENGTH)
		cmd->itemValueLength = MAX_CMD_VALUE_MAX_LENGTH;

	/* here will not ensure the end will be '\0' */
	kalMemCopy(cmd->itemValue, value, cmd->itemValueLength);
}

uint32_t wlanCfgSetGetFw(struct ADAPTER *prAdapter, const char *fwBuffer,
			 int cmdNum, enum CMD_TYPE cmdType)
{
	struct CMD_HEADER *pcmdV1Header = NULL;
	uint32_t rStatus;

	pcmdV1Header = (struct CMD_HEADER *)
			kalMemAlloc(sizeof(struct CMD_HEADER), VIR_MEM_TYPE);

	if (pcmdV1Header == NULL)
		return WLAN_STATUS_FAILURE;

	kalMemSet(pcmdV1Header->buffer, 0, MAX_CMD_BUFFER_LENGTH);
	pcmdV1Header->cmdType = cmdType;
	pcmdV1Header->cmdVersion = CMD_VER_1_EXT;
	pcmdV1Header->itemNum = cmdNum;
	pcmdV1Header->cmdBufferLen = cmdNum * sizeof(struct CMD_FORMAT_V1);
	kalMemCopy(pcmdV1Header->buffer, fwBuffer, pcmdV1Header->cmdBufferLen);

	rStatus = wlanSendSetQueryCmd(prAdapter, CMD_ID_GET_SET_CUSTOMER_CFG,
				TRUE, FALSE, FALSE,
				NULL, NULL,
				sizeof(struct CMD_HEADER),
				(uint8_t *) pcmdV1Header,
				NULL, 0);

	if (rStatus == WLAN_STATUS_FAILURE)
		DBGLOG(INIT, INFO, "kalIoctl wifiSefCFG fail 0x%x\n", rStatus);

	kalMemFree(pcmdV1Header, VIR_MEM_TYPE, sizeof(struct CMD_HEADER));
	return WLAN_STATUS_SUCCESS;
}

uint32_t wlanFwCfgParse(struct ADAPTER *prAdapter, uint8_t *pucConfigBuf)
{
	/* here return a list should be better */
	char *saveptr1, *saveptr2;
	char *cfgItems = pucConfigBuf;
	uint8_t cmdNum = 0;

	uint8_t *cmdBuffer = kalMemAlloc(MAX_CMD_BUFFER_LENGTH, VIR_MEM_TYPE);

	if (cmdBuffer == 0) {
		DBGLOG(INIT, INFO, "omega, cmd buffer return fail!");
		return WLAN_STATUS_FAILURE;
	}
	kalMemSet(cmdBuffer, 0, MAX_CMD_BUFFER_LENGTH);

	while (1) {
		char *keyStr = NULL;
		char *valueStr = NULL;
		char *cfgEntry = kalStrtokR(cfgItems, "\n\r", &saveptr1);

		if (!cfgEntry) {
			if (cmdNum)
				wlanCfgSetGetFw(prAdapter, cmdBuffer, cmdNum,
						CMD_TYPE_SET);

			if (cmdBuffer)
				kalMemFree(cmdBuffer, VIR_MEM_TYPE,
					   MAX_CMD_BUFFER_LENGTH);

			return WLAN_STATUS_SUCCESS;
		}
		cfgItems = NULL;

		keyStr = kalStrtokR(cfgEntry, " \t", &saveptr2);
		valueStr = kalStrtokR(NULL, "\0", &saveptr2);

		/* maybe a blank line, but with some tab or whitespace */
		if (!keyStr)
			continue;

		/* here take '#' at the beginning of line as comment */
		if (keyStr[0] == '#')
			continue;

		/* remove the \t " " at the beginning of the valueStr */
		while (valueStr && (*valueStr == '\t' || *valueStr == ' '))
			valueStr++;

		if (keyStr && valueStr) {
			wlanCfgFwSetParam(cmdBuffer, keyStr, valueStr, cmdNum,
					  1);
			cmdNum++;
			if (cmdNum == MAX_CMD_ITEM_MAX) {
				wlanCfgSetGetFw(prAdapter, cmdBuffer,
						MAX_CMD_ITEM_MAX, CMD_TYPE_SET);
				kalMemSet(cmdBuffer, 0, MAX_CMD_BUFFER_LENGTH);
				cmdNum = 0;
			}
		} else {
			/* here will not to try send the cmd has been parsed,
			 * but not sent yet
			 */
			if (cmdBuffer)
				kalMemFree(cmdBuffer, VIR_MEM_TYPE,
					   MAX_CMD_BUFFER_LENGTH);
			return WLAN_STATUS_FAILURE;
		}
	}
}
#endif /* CFG_SUPPORT_EASY_DEBUG */

void wlanReleasePendingCmdById(struct ADAPTER *prAdapter, uint8_t ucCid)
{
	struct QUE *prCmdQue;
	struct QUE rTempCmdQue;
	struct QUE *prTempCmdQue = &rTempCmdQue;
	struct QUE_ENTRY *prQueueEntry = (struct QUE_ENTRY *) NULL;
	struct CMD_INFO *prCmdInfo = (struct CMD_INFO *) NULL;

	KAL_SPIN_LOCK_DECLARATION();

	ASSERT(prAdapter);
	DBGLOG(OID, INFO, "Remove pending Cmd: CID %d\n", ucCid);

	/* 1: Clear Pending OID in prAdapter->rPendingCmdQueue */
	KAL_ACQUIRE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);

	prCmdQue = &prAdapter->rPendingCmdQueue;
	QUEUE_MOVE_ALL(prTempCmdQue, prCmdQue);

	QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry, struct QUE_ENTRY *);
	while (prQueueEntry) {
		prCmdInfo = (struct CMD_INFO *) prQueueEntry;
		if (prCmdInfo->ucCID != ucCid) {
			QUEUE_INSERT_TAIL(prCmdQue, prQueueEntry);
			continue;
		}

		if (prCmdInfo->pfCmdTimeoutHandler) {
			prCmdInfo->pfCmdTimeoutHandler(prAdapter, prCmdInfo);
		} else if (prCmdInfo->fgIsOid) {
			kalOidComplete(prAdapter->prGlueInfo,
				       prCmdInfo, 0,
				       WLAN_STATUS_FAILURE);
		}

		cmdBufFreeCmdInfo(prAdapter, prCmdInfo);
		QUEUE_REMOVE_HEAD(prTempCmdQue, prQueueEntry,
				  struct QUE_ENTRY *);
	}

	KAL_RELEASE_SPIN_LOCK(prAdapter, SPIN_LOCK_CMD_PENDING);
}

/* Translate Decimals string to Hex
** The result will be put in a 2bytes variable.
** Integer part will occupy the left most 3 bits, and decimal part is in the
** left 13 bits
** Integer part can be parsed by kstrtou16, decimal part should be translated by
** mutiplying
** 16 and then pick integer part.
** For example
*/
uint32_t wlanDecimalStr2Hexadecimals(uint8_t *pucDecimalStr, uint16_t *pu2Out)
{
	uint8_t aucDecimalStr[32] = {0};
	uint8_t *pucDecimalPart = NULL;
	uint8_t *tmp = NULL;
	uint32_t u4Result = 0;
	uint32_t u4Ret = 0;
	uint32_t u4Degree = 0;
	uint32_t u4Remain = 0;
	uint8_t ucAccuracy = 4; /* Hex decimals accuarcy is 4 bytes */
	uint32_t u4Base = 1;

	if (!pu2Out || !pucDecimalStr)
		return 1;

	while (*pucDecimalStr == '0')
		pucDecimalStr++;
	kalStrnCpy(aucDecimalStr, pucDecimalStr, sizeof(aucDecimalStr) - 1);
	aucDecimalStr[31] = 0;
	pucDecimalPart = strchr(aucDecimalStr, '.');
	if (!pucDecimalPart) {
		DBGLOG(INIT, INFO, "No decimal part, ori str %s\n",
		       pucDecimalStr);
		goto integer_part;
	}
	*pucDecimalPart++ = 0;
	/* get decimal degree */
	tmp = pucDecimalPart + strlen(pucDecimalPart);
	do {
		if (tmp == pucDecimalPart) {
			DBGLOG(INIT, INFO,
			       "Decimal part are all 0, ori str %s\n",
			       pucDecimalStr);
			goto integer_part;
		}
		tmp--;
	} while (*tmp == '0');

	*(++tmp) = 0;
	u4Degree = (uint32_t)(tmp - pucDecimalPart);
	/* if decimal part is not 0, translate it to hexadecimal decimals */
	/* Power(10, degree) */
	for (; u4Remain < u4Degree; u4Remain++)
		u4Base *= 10;

	while (*pucDecimalPart == '0')
		pucDecimalPart++;

	u4Ret = kalkStrtou32(pucDecimalPart, 0, &u4Remain);
	if (u4Ret) {
		DBGLOG(INIT, ERROR, "Parse decimal str %s error, degree %u\n",
			   pucDecimalPart, u4Degree);
		return u4Ret;
	}

	do {
		u4Remain *= 16;
		u4Result |= (u4Remain / u4Base) << ((ucAccuracy-1) * 4);
		u4Remain %= u4Base;
		ucAccuracy--;
	} while (u4Remain && ucAccuracy > 0);
	/* Each Hex Decimal byte was left shift more than 3 bits, so need
	** right shift 3 bits at last
	** For example, mmmnnnnnnnnnnnnn.
	** mmm is integer part, n represents decimals part.
	** the left most 4 n are shift 9 bits. But in for loop, we shift 12 bits
	**/
	u4Result >>= 3;
	u4Remain = 0;

integer_part:
	u4Ret = kalkStrtou32(aucDecimalStr, 0, &u4Remain);
	u4Result |= u4Remain << 13;

	if (u4Ret)
		DBGLOG(INIT, ERROR, "Parse integer str %s error\n",
		       aucDecimalStr);
	else {
		*pu2Out = u4Result & 0xffff;
		DBGLOG(INIT, TRACE, "Result 0x%04x\n", *pu2Out);
	}
	return u4Ret;
}

uint64_t wlanGetSupportedFeatureSet(struct GLUE_INFO *prGlueInfo)
{
	uint64_t u8FeatureSet = WIFI_HAL_FEATURE_SET;
	struct REG_INFO *prRegInfo;

	prRegInfo = &(prGlueInfo->rRegInfo);
	if ((prRegInfo != NULL) && (prRegInfo->ucSupport5GBand))
		u8FeatureSet |= WIFI_FEATURE_INFRA_5G;

#if CFG_SUPPORT_LLS
	if (!kalIsHalted() && prGlueInfo->prAdapter &&
	    prGlueInfo->prAdapter->pucLinkStatsSrcBufAddr)
		u8FeatureSet |= WIFI_FEATURE_LINK_LAYER_STATS;
#endif

#if CFG_SUPPORT_TDLS_OFFCHANNEL
	u8FeatureSet |= WIFI_FEATURE_TDLS_OFFCHANNEL;
#endif

	return u8FeatureSet;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is a wrapper to send eapol offload (rekey) command
* with PN sync consideration
*
* @param prGlueInfo  Pointer of prGlueInfo Data Structure
*
* @return VOID
*/
/*----------------------------------------------------------------------------*/
uint32_t
wlanSuspendRekeyOffload(struct GLUE_INFO *prGlueInfo, uint8_t ucRekeyMode)
{
	uint32_t u4BufLen;
	struct PARAM_GTK_REKEY_DATA *prGtkData;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4Rslt = WLAN_STATUS_FAILURE;
	struct GL_WPA_INFO *prWpaInfo;
	struct BSS_INFO *prBssInfo = NULL;

	if (!prGlueInfo)
		return WLAN_STATUS_NOT_ACCEPTED;

	prBssInfo = aisGetConnectedBssInfo(
		prGlueInfo->prAdapter);

	/* prAisBssInfo exist only when connect, skip if disconnect */
	if (!prBssInfo)
		return u4Rslt;

	prGtkData =
		(struct PARAM_GTK_REKEY_DATA *) kalMemAlloc(sizeof(
				struct PARAM_GTK_REKEY_DATA), VIR_MEM_TYPE);

	if (!prGtkData)
		return WLAN_STATUS_SUCCESS;

	kalMemZero(prGtkData, sizeof(struct PARAM_GTK_REKEY_DATA));

	prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
		prBssInfo->ucBssIndex);

	DBGLOG(RSN, INFO, "GTK Rekey ucRekeyMode = %d, BssIndex = %d\n",
		ucRekeyMode, prBssInfo->ucBssIndex);

	/* if enable, FW rekey offload. if disable, rekey back to supplicant */
	prGtkData->ucRekeyMode = ucRekeyMode;

	if (ucRekeyMode == GTK_REKEY_CMD_MODE_OFFLOAD_ON) {
		DBGLOG(RSN, INFO, "kek\n");
		DBGLOG_MEM8(RSN, INFO, (uint8_t *)prWpaInfo->aucKek,
			NL80211_KEK_LEN);
		DBGLOG(RSN, INFO, "kck\n");
		DBGLOG_MEM8(RSN, INFO, (uint8_t *)prWpaInfo->aucKck,
			NL80211_KCK_LEN);
		DBGLOG(RSN, INFO, "replay count\n");
		DBGLOG_MEM8(RSN, INFO,
			(uint8_t *)prWpaInfo->aucReplayCtr,
			NL80211_REPLAY_CTR_LEN);

		kalMemCopy(prGtkData->aucKek, prWpaInfo->aucKek,
			NL80211_KEK_LEN);
		kalMemCopy(prGtkData->aucKck, prWpaInfo->aucKck,
			NL80211_KCK_LEN);
		kalMemCopy(prGtkData->aucReplayCtr,
			prWpaInfo->aucReplayCtr,
			NL80211_REPLAY_CTR_LEN);

		prGtkData->ucBssIndex =	prBssInfo->ucBssIndex;

		prGtkData->u4Proto = NL80211_WPA_VERSION_2;
		if (prWpaInfo->u4WpaVersion == IW_AUTH_WPA_VERSION_WPA)
			prGtkData->u4Proto = NL80211_WPA_VERSION_1;

		if (GET_SELECTOR_TYPE(prBssInfo->
			u4RsnSelectedPairwiseCipher) == CIPHER_SUITE_TKIP)
			prGtkData->u4PairwiseCipher = BIT(3);
		else if (GET_SELECTOR_TYPE(prBssInfo->
			u4RsnSelectedPairwiseCipher) == CIPHER_SUITE_CCMP)
			prGtkData->u4PairwiseCipher = BIT(4);
		else {
			kalMemFree(prGtkData, VIR_MEM_TYPE,
				   sizeof(struct PARAM_GTK_REKEY_DATA));
			return 0;
		}

		if (GET_SELECTOR_TYPE(prBssInfo->
			u4RsnSelectedGroupCipher) == CIPHER_SUITE_TKIP)
			prGtkData->u4GroupCipher = BIT(3);
		else if (GET_SELECTOR_TYPE(prBssInfo->
			u4RsnSelectedGroupCipher) == CIPHER_SUITE_CCMP)
			prGtkData->u4GroupCipher = BIT(4);
		else {
			kalMemFree(prGtkData, VIR_MEM_TYPE,
				   sizeof(struct PARAM_GTK_REKEY_DATA));
			return 0;
		}

		prGtkData->u4KeyMgmt = prBssInfo->u4RsnSelectedAKMSuite;
		prGtkData->u4MgmtGroupCipher = 0;
	}

	if (ucRekeyMode == GTK_REKEY_CMD_MODE_OFLOAD_OFF) {
		/* inform FW disable EAPOL offload */
		prGtkData->ucBssIndex =	prBssInfo->ucBssIndex;
		DBGLOG(RSN, INFO, "Disable EAPOL offload\n");
	}

	rStatus = kalIoctl(prGlueInfo,
				wlanoidSetGtkRekeyData,
				prGtkData, sizeof(struct PARAM_GTK_REKEY_DATA),
				&u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS)
		DBGLOG(INIT, ERROR, "Suspend rekey data err:%x\n", rStatus);
	else
		u4Rslt = WLAN_STATUS_SUCCESS;

	kalMemFree(prGtkData, VIR_MEM_TYPE,
		sizeof(struct PARAM_GTK_REKEY_DATA));

	return u4Rslt;
}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is a wrapper to send power-saving mode command
*        when AIS enter wow, and send WOW command
*        Also let GC/GO/AP enter deactivate state to enter TOP sleep
*
* @param prGlueInfo                     Pointer of prGlueInfo Data Structure
*
* @return VOID
*/
/*----------------------------------------------------------------------------*/
void wlanSuspendPmHandle(struct GLUE_INFO *prGlueInfo)
{
	uint8_t idx, i;
	struct STA_RECORD *prStaRec;
	struct RX_BA_ENTRY *prRxBaEntry;
#if CFG_WOW_SUPPORT
	enum PARAM_POWER_MODE ePwrMode;
#endif
	struct BSS_INFO *prAisBssInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t ucKekZeroCnt = 0;
	uint8_t ucKckZeroCnt = 0;
	uint8_t ucGtkOffload = TRUE;
	struct GL_WPA_INFO *prWpaInfo;

	if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
		return;

	prAisBssInfo = aisGetConnectedBssInfo(
		prGlueInfo->prAdapter);
	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	/* process if AIS is connected */
	if (prAisBssInfo) {
		prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
					  prAisBssInfo->ucBssIndex);

		/* if EapolOffload is 0 => rekey when suspend wow */
		if (prWifiVar->ucEapolSuspendOffload) {

			/*
			 * check if KCK, KEK not sync from supplicant.
			 * if no these info updated from supplicant,
			 * disable GTK offload feature.
			 */
			for (i = 0; i < NL80211_KEK_LEN; i++) {
				if (prWpaInfo->aucKek[i] == 0x00)
					ucKekZeroCnt++;
			}

			for (i = 0; i < NL80211_KCK_LEN; i++) {
				if (prWpaInfo->aucKck[i] == 0x00)
					ucKckZeroCnt++;
			}

			if ((ucKekZeroCnt == NL80211_KCK_LEN) ||
			    (ucKckZeroCnt == NL80211_KCK_LEN)) {
				DBGLOG(RSN, INFO,
				       "no offload, no KCK/KEK from cfg\n");

				ucGtkOffload = FALSE;
			}

			if (ucGtkOffload)
				wlanSuspendRekeyOffload(prGlueInfo,
						 GTK_REKEY_CMD_MODE_OFFLOAD_ON);

			DBGLOG(HAL, STATE, "Suspend rekey offload\n");
		}
	}

#if CFG_WOW_SUPPORT
	/* 1) wifi cfg "Wow" is true              */
	/* 2) wow is enable                       */
	/* 3) WIFI connected => execute WOW flow  */
	/* 4) WIFI disconnected                   */
	/*      => is schedlued scanning &        */
	/*          schedlued scan wakeup enabled */
	/*      => execute WOW flow               */

	if (IS_FEATURE_ENABLED(prWifiVar->ucWow)) {
		/* Pending Timer related to CNM need to check and
		 * perform corresponding timeout handler. Without it,
		 * Might happen CNM abnormal after resume or during suspend.
		 */
		cnmStopPendingJoinTimerForSuspend(prGlueInfo->prAdapter);

		if (IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWowCtrl.fgWowEnable) ||
			IS_FEATURE_ENABLED(prWifiVar->ucAdvPws)) {
			if (prAisBssInfo) {
				/* AIS bss enter wow power mode, default fast pws */
				ePwrMode = Param_PowerModeFast_PSP;
				idx = prAisBssInfo->ucBssIndex;
				nicConfigPowerSaveProfileEntry(
					prGlueInfo->prAdapter,
					idx, ePwrMode, FALSE, PS_CALLER_WOW);
				DBGLOG(HAL, STATE, "Wow AIS_idx:%d, pwr mode:%d\n",
					idx, ePwrMode);

				DBGLOG(HAL, EVENT, "enter WOW flow\n");
				kalWowProcess(prGlueInfo, TRUE);
			} else if ((prWifiVar->rScanInfo.fgSchedScanning) &&
				(prGlueInfo->prAdapter->rWifiVar.ucWowDetectType
				& WOWLAN_DETECT_TYPE_SCHD_SCAN_SSID_HIT)) {
				DBGLOG(HAL, EVENT,
					"Sched Scanning, enter WOW flow\n");
				kalWowProcess(prGlueInfo, TRUE);
			}
		}
	}
#endif

	/* After resuming, WinStart will unsync with AP's SN.
	 * Set fgFirstSnToWinStart for all valid BA entry before suspend.
	 */
	for (idx = 0; idx < CFG_STA_REC_NUM; idx++) {
		prStaRec = cnmGetStaRecByIndex(prGlueInfo->prAdapter, idx);
		if (!prStaRec)
			continue;

		for (i = 0; i < CFG_RX_MAX_BA_TID_NUM; i++) {
			prRxBaEntry = prStaRec->aprRxReorderParamRefTbl[i];
			if (!prRxBaEntry || !(prRxBaEntry->fgIsValid))
				continue;
#if CFG_WOW_SUPPORT
			prRxBaEntry->fgFirstSnToWinStart = TRUE;
#endif
		}
	}

}

/*----------------------------------------------------------------------------*/
/*!
* @brief This function is to restore power-saving mode command when leave wow
*        But ignore GC/GO/AP role
*
* @param prGlueInfo                     Pointer of prGlueInfo Data Structure
*
* @return VOID
*/
/*----------------------------------------------------------------------------*/
void wlanResumePmHandle(struct GLUE_INFO *prGlueInfo)
{
#if CFG_WOW_SUPPORT
	struct BSS_INFO *prAisBssInfo = NULL;
	struct WIFI_VAR *prWifiVar = NULL;
	uint8_t ucKekZeroCnt = 0;
	uint8_t ucKckZeroCnt = 0;
	uint8_t ucGtkOffload = TRUE;
	uint8_t i = 0;
	struct GL_WPA_INFO *prWpaInfo;
#if CFG_SUPPORT_REPLAY_DETECTION
	uint8_t ucKeyIdx = 0;
	struct GL_DETECT_REPLAY_INFO *prDetRplyInfo = NULL;
#endif

	if (prGlueInfo == NULL || prGlueInfo->prAdapter == NULL)
		return;

	prAisBssInfo = aisGetConnectedBssInfo(
		prGlueInfo->prAdapter);

	prWifiVar = &prGlueInfo->prAdapter->rWifiVar;

	/* process if AIS is connected */
	if (prAisBssInfo) {
		prWpaInfo = aisGetWpaInfo(prGlueInfo->prAdapter,
					  prAisBssInfo->ucBssIndex);

		/* if cfg EAPOL offload disble, we disable offload
		 * when leave wow
		 */
		if (prWifiVar->ucEapolSuspendOffload) {

			/*
			 * check if KCK, KEK not sync from supplicant.
			 * if no these info updated from supplicant,
			 *disable GTK offload feature.
			 */
			for (i = 0; i < NL80211_KEK_LEN; i++) {
				if (prWpaInfo->aucKek[i] == 0x00)
					ucKekZeroCnt++;
			}

			for (i = 0; i < NL80211_KCK_LEN; i++) {
				if (prWpaInfo->aucKck[i] == 0x00)
					ucKckZeroCnt++;
			}

			if ((ucKekZeroCnt == NL80211_KCK_LEN) ||
			    (ucKckZeroCnt == NL80211_KCK_LEN)) {

				DBGLOG(RSN, INFO,
				       "no offload, no KCK/KEK from cfg\n");

				ucGtkOffload = FALSE;
			}

#if CFG_SUPPORT_REPLAY_DETECTION
			prDetRplyInfo = aisGetDetRplyInfo(prGlueInfo->prAdapter,
						      prAisBssInfo->ucBssIndex);

			/* Reset BC/MC KeyRSC to prevent incorrect replay
			 * detect
			 */
			for (ucKeyIdx = 0; ucKeyIdx < 4; ucKeyIdx++) {
				kalMemZero(
				   prDetRplyInfo->arReplayPNInfo[ucKeyIdx].auPN,
				   NL80211_KEYRSC_LEN);
			}

#endif

			if (ucGtkOffload) {
				wlanSuspendRekeyOffload(prGlueInfo,
						 GTK_REKEY_CMD_MODE_OFLOAD_OFF);

				DBGLOG(HAL, STATE,
				       "Resume rekey offload disable\n");
			}
		}
	}

	if (IS_FEATURE_ENABLED(prWifiVar->ucWow) &&
		(IS_FEATURE_ENABLED(prGlueInfo->prAdapter->rWowCtrl.fgWowEnable) ||
		IS_FEATURE_ENABLED(prWifiVar->ucAdvPws))) {
		if (prAisBssInfo) {
			DBGLOG(HAL, EVENT, "leave WOW flow\n");
			kalWowProcess(prGlueInfo, FALSE);

			/* Restore AIS pws when leave wow, ignore ePwrMode */
			nicConfigPowerSaveProfileEntry(prGlueInfo->prAdapter,
				prAisBssInfo->ucBssIndex,
				Param_PowerModeCAM,
				FALSE, PS_CALLER_WOW);
		} else if ((prWifiVar->rScanInfo.fgSchedScanning) &&
			(prGlueInfo->prAdapter->rWifiVar.ucWowDetectType
			& WOWLAN_DETECT_TYPE_SCHD_SCAN_SSID_HIT)) {
			DBGLOG(HAL, EVENT,
				"Sched Scanning, leave WOW flow\n");
			kalWowProcess(prGlueInfo, FALSE);
		}
	}
#endif
}


/*----------------------------------------------------------------------------*/
/*!
* @brief This function is to wake up WiFi
*
* @param prAdapter      Pointer to the Adapter structure.
*
* @return WLAN_STATUS_SUCCESS
*         WLAN_STATUS_FAILURE
*/
/*----------------------------------------------------------------------------*/
uint32_t wlanWakeUpWiFi(struct ADAPTER *prAdapter)
{
	u_int8_t fgReady;
	struct mt66xx_chip_info *prChipInfo;

	if (!prAdapter)
		return WLAN_STATUS_FAILURE;
	prChipInfo = prAdapter->chip_info;

	HAL_WIFI_FUNC_READY_CHECK(prAdapter, prChipInfo->sw_ready_bits,
			&fgReady);

	if (fgReady) {
#if defined(_HIF_USB)
		DBGLOG(INIT, INFO,
			"Wi-Fi is already ON!, turn off before FW DL!\n");
		if (wlanPowerOffWifi(prAdapter) != WLAN_STATUS_SUCCESS)
			return WLAN_STATUS_FAILURE;
#else
		DBGLOG(INIT, INFO, "Wi-Fi is already ON!\n");
#endif
	}

	nicpmWakeUpWiFi(prAdapter);
	HAL_HIF_INIT(prAdapter);

	return WLAN_STATUS_SUCCESS;
}

static uint32_t wlanHwRateOfdmNum(uint16_t ofdm_idx)
{
	switch (ofdm_idx) {
	case 11: /* 6M */
		return g_rOfdmDataRateMappingTable.rate[0];
	case 15: /* 9M */
		return g_rOfdmDataRateMappingTable.rate[1];
	case 10: /* 12M */
		return g_rOfdmDataRateMappingTable.rate[2];
	case 14: /* 18M */
		return g_rOfdmDataRateMappingTable.rate[3];
	case 9: /* 24M */
		return g_rOfdmDataRateMappingTable.rate[4];
	case 13: /* 36M */
		return g_rOfdmDataRateMappingTable.rate[5];
	case 8: /* 48M */
		return g_rOfdmDataRateMappingTable.rate[6];
	case 12: /* 54M */
		return g_rOfdmDataRateMappingTable.rate[7];
	default:
		return 0;
	}
}

/**
 * wlanQueryRateByTable() - get phy rate by parameters in the unit of 0.1M
 * @mode: TX_RATE_MODE_CCK (0),
 *	  TX_RATE_MODE_OFDM (1),
 *	  TX_RATE_MODE_HTMIX (2), TX_RATE_MODE_HTGF (3),
 *	  TX_RATE_MODE_VHT (4),
 *	  TX_RATE_MODE_HE_SU (8), TX_RATE_MODE_HE_ER (9)
 * @rate: MCS index, [0, ..]
 * @bw: bandwidth, 0 for 11n, [0, 3] for 11ac, 11ax for 20, 40, 80, 160
 * @gi: GI, [0, 1] for 11n, 11ac, 1 as short GI
 *	    [0, 2] for 11ax, 0 as shortest, 2 as longest
 * @nsts: NSTS, [1, 3]
 * @pu4CurRate: returning current phy rate by given parameters
 * @pu4MaxRate: returning max phy rate (max MCS)
 *
 * Return:
 *	0: Success
 *	-1: Failure
 */
int wlanQueryRateByTable(uint32_t txmode, uint32_t rate,
			uint32_t frmode, uint32_t gi, uint32_t nsts,
			uint32_t *pu4CurRate, uint32_t *pu4MaxRate)
{
	uint32_t u4CurRate = 0, u4MaxRate = 0;
	uint8_t ucMaxSize = 0;

	if (txmode == TX_RATE_MODE_CCK) { /* 11B */
		ucMaxSize = ARRAY_SIZE(g_rCckDataRateMappingTable.rate);
		/* short preamble */
		if (rate >= 5 && rate <= 7)
			rate -= 4;
		if (rate >= ucMaxSize) {
			DBGLOG_LIMITED(SW4, ERROR, "rate error for CCK: %u\n",
					rate);
			return -1;
		}
		u4CurRate = g_rCckDataRateMappingTable.rate[rate];
		u4MaxRate = g_rCckDataRateMappingTable
			.rate[MCS_IDX_MAX_RATE_CCK];
	} else if (txmode == TX_RATE_MODE_OFDM) { /* 11G */
		u4CurRate = wlanHwRateOfdmNum(rate);
		if (u4CurRate == 0) {
			DBGLOG_LIMITED(SW4, ERROR, "rate error for OFDM\n");
			return -1;
		}
		u4MaxRate = g_rOfdmDataRateMappingTable
			.rate[MCS_IDX_MAX_RATE_OFDM];
	} else if (txmode == TX_RATE_MODE_HTMIX ||
		   txmode == TX_RATE_MODE_HTGF) { /* 11N */
		if (nsts == 0 || nsts >= 4) {
			DBGLOG_LIMITED(SW4, ERROR, "nsts error: %u\n", nsts);
			return -1;
		}
		if (gi >= 2) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "gi error for 11N: %u\n", gi);
			return -1;
		}

		ucMaxSize = 8;
		if (rate > 23) {
			DBGLOG_LIMITED(SW4, ERROR, "rate error for 11N: %u\n",
					rate);
			return -1;
		}
		rate %= ucMaxSize;

		if (frmode > 1) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "frmode error for 11N: %u\n", frmode);
			return -1;
		}
		u4CurRate = g_rDataRateMappingTable.nsts[nsts - 1].bw[frmode]
				.sgi[gi].rate[rate];
		u4MaxRate = g_rDataRateMappingTable.nsts[nsts - 1].bw[frmode]
				.sgi[gi].rate[MCS_IDX_MAX_RATE_HT];
	} else if (txmode == TX_RATE_MODE_VHT) { /* 11AC */
		if (nsts == 0 || nsts >= 4) {
			DBGLOG_LIMITED(SW4, ERROR, "nsts error: %u\n", nsts);
			return -1;
		}

		if (frmode > 3) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "frmode error for 11AC: %u\n", frmode);
			return -1;
		}

		if (gi >= 2) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "gi error for 11AC: %u\n", gi);
			return -1;
		}

		ucMaxSize = ARRAY_SIZE(g_rDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].sgi[gi].rate);
		if (rate >= ucMaxSize) {
			DBGLOG_LIMITED(SW4, ERROR, "rate error for 11AC: %u\n",
					rate);
			return -1;
		}

		u4CurRate = g_rDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].sgi[gi].rate[rate];
		u4MaxRate = g_rDataRateMappingTable.nsts[nsts - 1].bw[frmode]
				.sgi[gi].rate[MCS_IDX_MAX_RATE_VHT];
	} else if (txmode == TX_RATE_MODE_HE_SU ||
		   txmode == TX_RATE_MODE_HE_ER ||
		   txmode == TX_RATE_MODE_HE_MU) { /* AX */
		uint8_t dcm = 0, ru106 = 0;

		if (nsts == 0 || nsts >= 5) {
			DBGLOG_LIMITED(SW4, ERROR, "nsts error: %u\n", nsts);
			return -1;
		}
		if (frmode > 3) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "frmode error for 11AX: %u\n", frmode);
			return -1;
		}
		if (gi >= 3) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "gi error for 11AX: %u\n", gi);
			return -1;
		}

		/* bit 4: dcm, bit 5: RU106 */
		dcm = rate & BIT(4);
		ru106 = rate & BIT(5);
		rate = rate & BITS(0, 3);

		ucMaxSize = ARRAY_SIZE(g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate);
		if (rate >= ucMaxSize) {
			DBGLOG_LIMITED(SW4, ERROR, "rate error for 11AX: %u\n",
					rate);
			return -1;
		}

		u4CurRate = g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate[rate];
		u4MaxRate = g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate[MCS_IDX_MAX_RATE_HE];

		if (dcm || ru106) {
			u4CurRate = u4CurRate >> 1;
			u4MaxRate = u4MaxRate >> 1;
		}
	} else if (txmode == TX_RATE_MODE_EHT_ER ||
		   txmode == TX_RATE_MODE_EHT_TRIG ||
		   txmode == TX_RATE_MODE_EHT_MU) { /* BE */
		uint8_t ru106 = 0;

		if (nsts == 0 || nsts >= 5) {
			DBGLOG_LIMITED(SW4, ERROR, "nsts error: %u\n", nsts);
			return -1;
		}
		if (frmode > 5) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "frmode error for 11BE: %u\n", frmode);
			return -1;
		}
		if (frmode == 5) /* Both 4, 5 are 320MHz, look up by index 4 */
			frmode--;

		if (gi >= 3) {
			DBGLOG_LIMITED(SW4, ERROR,
			       "gi error for 11BE: %u\n", gi);
			return -1;
		}

		/* DCM = MCS15, bit 5: RU106 */
		rate = rate & BITS(0, 3);
		ru106 = rate & BIT(5);

		ucMaxSize = ARRAY_SIZE(g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate);
		if (rate >= ucMaxSize) {
			DBGLOG_LIMITED(SW4, ERROR,
				       "rate error for 11BE: %u\n", rate);
			return -1;
		}

		u4CurRate = g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate[rate];
		u4MaxRate = g_rAxDataRateMappingTable.nsts[nsts - 1]
				.bw[frmode].gi[gi].rate[MCS_IDX_MAX_RATE_EHT];

		if (ru106) {
			u4CurRate = u4CurRate >> 1;
			u4MaxRate = u4MaxRate >> 1;
		}
	} else {
		DBGLOG_LIMITED(SW4, ERROR,
				"Unknown rate for [%d,%d,%d,%d,%d]\n",
				txmode, nsts, frmode, gi, rate);
		return -1;
	}

	if (pu4CurRate)
		*pu4CurRate = u4CurRate;
	if (pu4MaxRate)
		*pu4MaxRate = u4MaxRate;
	return 0;
}

#if CFG_REPORT_MAX_TX_RATE
int wlanGetMaxTxRate(struct ADAPTER *prAdapter,
		 void *prBssPtr, struct STA_RECORD *prStaRec,
		 uint32_t *pu4CurRate, uint32_t *pu4MaxRate)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucPhyType, ucTxMode = 0, ucMcsIdx = 0, ucSgi = 0;
	uint8_t ucBw = 0, ucAPBwPermitted = 0, ucNss = 0, ucApNss = 0;
	struct BSS_DESC *prBssDesc = NULL;

	*pu4CurRate = 0;
	*pu4MaxRate = 0;
	prBssInfo = (struct BSS_INFO *) prBssPtr;

	/* get tx mode and MCS index */
	ucPhyType = prBssInfo->ucPhyTypeSet;

	DBGLOG(SW4, TRACE,
		   "ucPhyType: 0x%x\n",
		   ucPhyType);

#if (CFG_SUPPORT_802_11BE == 1)
	if (ucPhyType & PHY_TYPE_SET_802_11BE)
		ucTxMode = TX_RATE_MODE_EHT_MU;
	else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (ucPhyType & PHY_TYPE_SET_802_11AX)
		ucTxMode = TX_RATE_MODE_HE_SU;
	else
#endif
	if (ucPhyType & PHY_TYPE_SET_802_11AC)
		ucTxMode = TX_RATE_MODE_VHT;
	else if (ucPhyType & PHY_TYPE_SET_802_11N)
		ucTxMode = TX_RATE_MODE_HTMIX;
	else if (ucPhyType & PHY_TYPE_SET_802_11G) {
		ucTxMode = TX_RATE_MODE_OFDM;
		ucMcsIdx = 12;
	} else if (ucPhyType & PHY_TYPE_SET_802_11A) {
		ucTxMode = TX_RATE_MODE_OFDM;
		ucMcsIdx = 12;
	} else if (ucPhyType & PHY_TYPE_SET_802_11B)
		ucTxMode = TX_RATE_MODE_CCK;
	else {
		DBGLOG(SW4, ERROR,
		       "unknown wifi type, prBssInfo->ucPhyTypeSet: %u\n",
		       ucPhyType);
		goto errhandle;
	}

	/* get bandwidth */
	ucBw = cnmGetBssMaxBw(prAdapter, prBssInfo->ucBssIndex);
	ucAPBwPermitted = rlmGetBssOpBwByVhtAndHtOpInfo(prBssInfo);
	if (ucAPBwPermitted < ucBw)
		ucBw = ucAPBwPermitted;

	/* Get Short GI Tx capability for HT/VHT, check from HT or VHT
	 * capability BW SGI bit. Refer to 802.11 Short GI operation
	 */
	if (ucTxMode == TX_RATE_MODE_HTMIX || ucTxMode == TX_RATE_MODE_HTGF
	    || ucTxMode == TX_RATE_MODE_VHT) {
		if (prStaRec->u2HtCapInfo & HT_CAP_INFO_SHORT_GI_20M) {
			DBGLOG(RLM, TRACE, "HT_CAP_INFO_SHORT_GI_20M\n");
			ucSgi = 1;
		}
		if (prStaRec->u2HtCapInfo & HT_CAP_INFO_SHORT_GI_40M) {
			DBGLOG(RLM, TRACE, "HT_CAP_INFO_SHORT_GI_40M\n");
			ucSgi = 1;
		}

#if CFG_SUPPORT_802_11AC
		if (prStaRec->u4VhtCapInfo & VHT_CAP_INFO_SHORT_GI_80) {
			DBGLOG(RLM, TRACE, "VHT_CAP_INFO_SHORT_GI_80\n");
			ucSgi = 1;
		}
		if (prStaRec->u4VhtCapInfo & VHT_CAP_INFO_SHORT_GI_160_80P80) {
			DBGLOG(RLM, TRACE, "VHT_CAP_INFO_SHORT_GI_160_80P80\n");
			ucSgi = 1;
		}
#endif
	}

	if (ucTxMode == TX_RATE_MODE_HE_SU) {
		DBGLOG(SW4, TRACE, "TX_RATE_MODE_HE_SU\n");
		ucSgi = 0;
	}

	/* get antenna number */
	prBssDesc = aisGetTargetBssDesc(prAdapter, prBssInfo->ucBssIndex);
	ucNss = wlanGetSupportNss(prAdapter, prBssInfo->ucBssIndex);
	if (prBssDesc) {
		ucApNss = bssGetRxNss(prBssDesc);
		if (ucApNss > 0 && ucApNss < ucNss)
			ucNss = ucApNss;
	}
	if ((ucNss < 1) || (ucNss > 3)) {
		DBGLOG(RLM, ERROR, "error ucNss: %u\n", ucNss);
		goto errhandle;
	}

	DBGLOG(SW4, TRACE,
	       "txmode=[%u], mcs idx=[%u], bandwidth=[%u], sgi=[%u], nsts=[%u]\n",
	       ucTxMode, ucMcsIdx, ucBw, ucSgi, ucNss
	);

	if (wlanQueryRateByTable(ucTxMode, ucMcsIdx, ucBw, ucSgi,
				ucNss, pu4CurRate, pu4MaxRate) < 0)
		goto errhandle;

	DBGLOG(SW4, TRACE,
	       "*pu4CurRate=[%u], *pu4MaxRate=[%u]\n",
	       *pu4CurRate, *pu4MaxRate);

	return 0;

errhandle:
	DBGLOG(SW4, ERROR,
	       "txmode=[%u], mcs idx=[%u], bandwidth=[%u], sgi=[%u], nsts=[%u]\n",
	       ucTxMode, ucMcsIdx, ucBw, ucSgi, ucNss);
	return -1;
}
#endif /* CFG_REPORT_MAX_TX_RATE */

/**
 * wlanGetRxRate - Get RX rate from information in RXV
 */
static int wlanGetRxRate(struct GLUE_INFO *prGlueInfo, uint32_t *prRxV,
		uint32_t *pu4CurRate, uint32_t *pu4MaxRate,
		struct RxRateInfo *prRxRateInfo)
{
	struct ADAPTER *prAdapter;
	struct RxRateInfo rRxRateInfo = {0};
	int32_t rv;
#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	struct CHIP_DBG_OPS *prChipDbg;
#endif

	if (pu4CurRate)
		*pu4CurRate = 0;
	if (pu4MaxRate)
		*pu4MaxRate = 0;
	if (prRxRateInfo)
		*prRxRateInfo = (const struct RxRateInfo){0};
	prAdapter = prGlueInfo->prAdapter;

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
	prChipDbg = prAdapter->chip_info->prDebugOps;

	if (prChipDbg && prChipDbg->get_rx_rate_info) {
		rv = prChipDbg->get_rx_rate_info(prRxV, &rRxRateInfo);

		if (rv < 0)
			goto errhandle;
	}
#else
	goto errhandle;
#endif

	if (prRxRateInfo)
		*prRxRateInfo = rRxRateInfo;

	rv = wlanQueryRateByTable(rRxRateInfo.u4Mode, rRxRateInfo.u4Rate,
				rRxRateInfo.u4Bw, rRxRateInfo.u4Gi,
				rRxRateInfo.u4Nss, pu4CurRate, pu4MaxRate);
	if (rv < 0)
		goto errhandle;

	return 0;

errhandle:
	DBGLOG(SW4, TRACE,
		"rxmode=[%u], rate=[%u], frmode=[%u], sgi=[%u], nss=[%u]\n",
		rRxRateInfo.u4Mode, rRxRateInfo.u4Rate, rRxRateInfo.u4Bw,
		rRxRateInfo.u4Gi, rRxRateInfo.u4Nss);
	return -1;
}

/**
 * wlanGetRxRateByBssid() - Get the RX rate in last cached RXV data
 *			    (unit: 0.1Mbps).
 * @prGlueInfo: Pointer to GLUE info
 * @ucBssIdx: BSS index to query by parsing RX rate from RXV saved in SatRec.
 * @pu4CurRate: Returning current rate for given RX rate parameters if non-NULL
 *              pointer is passed-in
 * @pu4MaxRate: Returning max supported rate for given RX parameters with max
 *              MCS index if non-NULL pointer is passed-in
 * @prRxRateInfo: Pointer to structure returning RxRateInfo.
 *		If caller want to get RxRateInfo parameters, call with a valid
 *		pointer to returning structure.
 *		  u4Mode: Returning RX mode
 *		  u4Nss: Returning NSS [1..]
 *		  u4Bw: Returning bandwidth
 *		  u4Gi: Returning guard interval
 *		  u4Rate: Returning MCS index
 *		Check wlanQueryRateByTable() for returning values.
 *
 * This function gets RX rate from the last cached RX rate from RXV saved in
 *     prAdapter->arStaRec[i].u4RxV[*].
 *     The u4RxV were saved earlier on calling
 *     asicConnac2xRxProcessRxvforMSP() in nicRxIndicatePackets() or on calling
 *     nicRxProcessRxReport() on receiving RX_PKT_TYPE_RX_REPORT.
 *
 *     The two functions filling u4RxV are mutual exclusive.
 *     In the chip supporting getting the rate from RX report,
 *     CONNAC2X_RXV_FROM_RX_RPT(prAdapter) returns TRUE, the values will not be
 *     filled in asicConnac2xRxProcessRxvforMSP().
 *
 * Return:0 on success, -1 on failure.
 *	The caller shall pass valid pointers in the arguments of interested
 *	results.
 */
int wlanGetRxRateByBssid(struct GLUE_INFO *prGlueInfo, uint8_t ucBssIdx,
		uint32_t *pu4CurRate, uint32_t *pu4MaxRate,
		struct RxRateInfo *prRxRateInfo)
{
	struct ADAPTER *prAdapter;
	struct STA_RECORD *prStaRec;
	uint32_t *prRxV = NULL; /* pointer to stored RxV */
	uint8_t ucWlanIdx;
	uint8_t ucStaIdx;

	prAdapter = prGlueInfo->prAdapter;

	if (!IS_BSS_INDEX_AIS(prAdapter, ucBssIdx))
		return -1;

	prStaRec = aisGetStaRecOfAP(prAdapter, ucBssIdx);
	if (prStaRec) {
		ucWlanIdx = prStaRec->ucWlanIndex;
	} else {
		DBGLOG(SW4, ERROR, "prStaRecOfAP is null\n");
		return -1;
	}

	if (wlanGetStaIdxByWlanIdx(prAdapter, ucWlanIdx, &ucStaIdx) ==
		WLAN_STATUS_SUCCESS) {
		prRxV = prAdapter->arStaRec[ucStaIdx].au4RxV;
	} else {
		DBGLOG_LIMITED(SW4, ERROR, "wlanGetStaIdxByWlanIdx fail\n");
		return -1;
	}

	return wlanGetRxRate(prGlueInfo, prRxV,
			pu4CurRate, pu4MaxRate, prRxRateInfo);
}

#if CFG_SUPPORT_LINK_QUALITY_MONITOR
uint32_t wlanLinkQualityMonitor(struct GLUE_INFO *prGlueInfo, bool bFgIsOid)
{
	struct ADAPTER *prAdapter;
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo = NULL;
#if (CFG_SUPPORT_STATS_ONE_CMD == 0)
	struct PARAM_GET_STA_STATISTICS *prQueryStaStatistics;
#else
	struct PARAM_GET_STATS_ONE_CMD rParam;
	uint32_t u4QueryInfoLen;
#endif
	struct PARAM_802_11_STATISTICS_STRUCT *prStat;
	uint8_t arBssid[PARAM_MAC_ADDR_LEN];
	uint32_t u4Status = WLAN_STATUS_FAILURE;
	uint8_t ucBssIndex;
	struct BSS_INFO *prBssInfo;

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(SW4, ERROR, "prAdapter is null\n");
		return u4Status;
	}

	ucBssIndex = aisGetDefaultLinkBssIndex(prAdapter);
	if (kalGetMediaStateIndicated(prGlueInfo, ucBssIndex) !=
	    MEDIA_STATE_CONNECTED) {
		/* not connected */
		DBGLOG(SW4, ERROR, "not yet connected\n");
		return u4Status;
	}

	/* Completely record the Link quality and store the current time */
	prAdapter->u4LastLinkQuality = kalGetTimeTick();
	DBGLOG(NIC, TRACE, "LastLinkQuality:%u\n",
			prAdapter->u4LastLinkQuality);

	kalMemZero(arBssid, MAC_ADDR_LEN);
	prBssInfo = GET_BSS_INFO_BY_INDEX(prAdapter, ucBssIndex);
	if (!prBssInfo)
		return u4Status;
	COPY_MAC_ADDR(arBssid, prBssInfo->aucBSSID);

	/* send cmd to firmware */
	prStat = &(prAdapter->rStat);

	kalMemZero(prStat, sizeof(struct PARAM_802_11_STATISTICS_STRUCT));

#if (CFG_SUPPORT_STATS_ONE_CMD == 1)
	rParam.u4Period = SEC_TO_MSEC(CFG_LQ_MONITOR_FREQUENCY);
	u4Status = wlanQueryStatsOneCmd(prAdapter,
				&rParam,
				sizeof(rParam),
				&u4QueryInfoLen,
				FALSE,
				ucBssIndex);
	DBGLOG(REQ, TRACE,
			"u4Status=%u", u4Status);
#else
	prQueryStaStatistics = &(prAdapter->rQueryStaStatistics);
	COPY_MAC_ADDR(prQueryStaStatistics->aucMacAddr, arBssid);
	prQueryStaStatistics->ucReadClear = TRUE;
	DBGLOG(REQ, TRACE, "Call prQueryStaStatistics=%p, u4BufLen=%p",
			prQueryStaStatistics, &prAdapter->u4BufLen);
	u4Status = wlanQueryStaStatistics(prAdapter,
				prQueryStaStatistics,
				sizeof(struct PARAM_GET_STA_STATISTICS),
				&(prAdapter->u4BufLen),
				FALSE);
	DBGLOG(REQ, TRACE,
			"u4Status=%u, prQueryStaStatistics=%p, u4BufLen=%p",
			u4Status, prQueryStaStatistics,
			&prAdapter->u4BufLen);

	u4Status = wlanQueryStatistics(prAdapter,
				prStat,
				sizeof(struct PARAM_802_11_STATISTICS_STRUCT),
				&(prAdapter->u4BufLen),
				FALSE);

#endif
	wlanFinishCollectingLinkQuality(prAdapter->prGlueInfo);

	if (bFgIsOid == FALSE)
		u4Status = WLAN_STATUS_SUCCESS;

	if ((bFgIsOid == TRUE) || (prAdapter->u4LastLinkQuality <= 0))
		return u4Status;

	prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);

	DBGLOG(SW4, INFO,
	       "Link Quality: Tx(rate:%u, total:%lu, retry:%lu, fail:%lu, RTS fail:%lu, ACK fail:%lu), Rx(rate:%u, total:%lu, dup:%u, error:%lu), PER(%u), Congestion(idle slot:%lu, diff:%lu, AwakeDur:%u)\n",
	       prLinkQualityInfo->u4CurTxRate, /* current tx link speed */
	       prLinkQualityInfo->u8TxTotalCount, /* tx total packages */
	       prLinkQualityInfo->u8TxRetryCount, /* tx retry count */
	       prLinkQualityInfo->u8TxFailCount, /* tx fail count */
	       prLinkQualityInfo->u8TxRtsFailCount, /* tx RTS fail count */
	       prLinkQualityInfo->u8TxAckFailCount, /* tx ACK fail count */
	       prLinkQualityInfo->u4CurRxRate, /* current rx link speed */
	       prLinkQualityInfo->u8RxTotalCount, /* rx total packages */
	       prLinkQualityInfo->u4RxDupCount, /* rx duplicate package count */
	       prLinkQualityInfo->u8RxErrCount, /* rx fcs fail count */
	       prLinkQualityInfo->u4CurTxPer, /* current Tx PER */
	       /* congestion stats */
	       prLinkQualityInfo->u8IdleSlotCount, /* idle slot */
	       prLinkQualityInfo->u8DiffIdleSlotCount, /* idle slot diff */
	       prLinkQualityInfo->u4HwMacAwakeDuration
	);

	return u4Status;
}

void wlanFinishCollectingLinkQuality(struct GLUE_INFO *prGlueInfo)
{
	struct ADAPTER *prAdapter;
	struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo = NULL;
	uint32_t u4CurRxRate, u4MaxRxRate;
	uint64_t u8TxFailCntDif, u8TxTotalCntDif;
	uint8_t ucBssIndex;

	prAdapter = prGlueInfo->prAdapter;
	if (prAdapter == NULL) {
		DBGLOG(SW4, ERROR, "prAdapter is null\n");
		return;
	}

	/* prepare to set/get statistics from BSSInfo's rLinkQualityInfo */
	prLinkQualityInfo = &(prAdapter->rLinkQualityInfo);

	/* Calculate current tx PER */
	u8TxTotalCntDif = (prLinkQualityInfo->u8TxTotalCount >
			   prLinkQualityInfo->u8LastTxTotalCount) ?
			  (prLinkQualityInfo->u8TxTotalCount -
			   prLinkQualityInfo->u8LastTxTotalCount) : 0;
	u8TxFailCntDif = (prLinkQualityInfo->u8TxFailCount >
			  prLinkQualityInfo->u8LastTxFailCount) ?
			 (prLinkQualityInfo->u8TxFailCount -
			  prLinkQualityInfo->u8LastTxFailCount) : 0;
	if (u8TxTotalCntDif >= u8TxFailCntDif)
		prLinkQualityInfo->u4CurTxPer = (u8TxTotalCntDif == 0) ? 0 :
			kal_div64_u64(u8TxFailCntDif * 100, u8TxTotalCntDif);
	else
		prLinkQualityInfo->u4CurTxPer = 0;

	/* Calculate idle slot diff */
	if (prLinkQualityInfo->u8IdleSlotCount <
			prLinkQualityInfo->u8LastIdleSlotCount) {
		prLinkQualityInfo->u8DiffIdleSlotCount = 0;
		DBGLOG(NIC, WARN, "idle slot is error\n");
	} else
		prLinkQualityInfo->u8DiffIdleSlotCount =
			prLinkQualityInfo->u8IdleSlotCount -
			prLinkQualityInfo->u8LastIdleSlotCount;

	ucBssIndex = aisGetDefaultLinkBssIndex(prAdapter);
	/* get current rx rate */
	if (wlanGetRxRateByBssid(prGlueInfo,
			ucBssIndex,
			&u4CurRxRate, &u4MaxRxRate, NULL) < 0)
		prLinkQualityInfo->u4CurRxRate = 0;
	else
		prLinkQualityInfo->u4CurRxRate = u4CurRxRate;

#if (CFG_SUPPORT_DATA_STALL && CFG_SUPPORT_LINK_QUALITY_MONITOR)
	wlanCustomMonitorFunction(prAdapter, prLinkQualityInfo, ucBssIndex);
#endif

	prLinkQualityInfo->u8LastTxTotalCount =
					prLinkQualityInfo->u8TxTotalCount;
	prLinkQualityInfo->u8LastTxFailCount =
					prLinkQualityInfo->u8TxFailCount;
	prLinkQualityInfo->u8LastIdleSlotCount =
					prLinkQualityInfo->u8IdleSlotCount;
	prLinkQualityInfo->u8LastRxTotalCount =
					prLinkQualityInfo->u8RxTotalCount;
}
#endif /* CFG_SUPPORT_LINK_QUALITY_MONITOR */

#if (CFG_SUPPORT_DATA_STALL && CFG_SUPPORT_LINK_QUALITY_MONITOR)

void wlanLowDataRateMonitor(struct ADAPTER *prAdapter,
	 struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo, uint8_t ucBssIdx)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint32_t u4TxRateMbps, u4RxRateMbps, u4TxTput, u4RxTput;
	uint64_t u8TxTotalCntDif, u8RxTotalCntDif;
	uint32_t u4CurTick;
	char uevent[300];

	GET_CURRENT_SYSTIME(&u4CurTick);

	u4TxTput = kalGetTpMbpsByBssId(prAdapter, PKT_PATH_TX, ucBssIdx);
	u4RxTput = kalGetTpMbpsByBssId(prAdapter, PKT_PATH_RX, ucBssIdx);

	u8TxTotalCntDif = (prLinkQualityInfo->u8TxTotalCount >
			   prLinkQualityInfo->u8LastTxTotalCount) ?
			  (prLinkQualityInfo->u8TxTotalCount -
			   prLinkQualityInfo->u8LastTxTotalCount) : 0;

	u8RxTotalCntDif = (prLinkQualityInfo->u8RxTotalCount >
			   prLinkQualityInfo->u8LastRxTotalCount) ?
			  (prLinkQualityInfo->u8RxTotalCount -
			   prLinkQualityInfo->u8LastRxTotalCount) : 0;

	/* change unit from 100kbps to mbps */
	u4TxRateMbps = prLinkQualityInfo->u4CurTxRate / 10;
	u4RxRateMbps = prLinkQualityInfo->u4CurRxRate / 10;

	DBGLOG_LIMITED(SW4, TRACE,
		"Tput:%u/%u mpdu:%llu/%llu dur:%u/%u rate:%u/%u",
		u4TxTput, u4RxTput,
		u8TxTotalCntDif, u8RxTotalCntDif,
		prAdapter->u4LowTxRateDur,
		prAdapter->u4LowRxRateDur,
		prLinkQualityInfo->u4CurTxRate,
		prLinkQualityInfo->u4CurRxRate);

	if (u4TxRateMbps >= prWifiVar->u4LowRateUevtTh ||
		u4TxTput < prWifiVar->u4LowRateUevtTputTh &&
		u8TxTotalCntDif < prWifiVar->u8LowRateUevtMpduTh) {
		prAdapter->u4LowTxRateDur = 0;
		prAdapter->fgSendTxUevt = FALSE;
	} else if (u4TxRateMbps < prWifiVar->u4LowRateUevtTh)
		prAdapter->u4LowTxRateDur++;

	if (u4RxRateMbps >= prWifiVar->u4LowRateUevtTh ||
		u4RxTput < prWifiVar->u4LowRateUevtTputTh &&
		u8RxTotalCntDif < prWifiVar->u8LowRateUevtMpduTh) {
		prAdapter->u4LowRxRateDur = 0;
		prAdapter->fgSendRxUevt = FALSE;
	} else if (u4RxRateMbps < prWifiVar->u4LowRateUevtTh)
		prAdapter->u4LowRxRateDur++;

	/* send uevent if keep low rate for u4LowRateUevtIntv seconds
	 * and uevent has not been sent during this low rate period.
	 * If the data rate fluctuates around the threshold,
	 * we won't send uevent within u4LowRateUevtReptIntv seconds.
	 */
	if (prAdapter->u4LowTxRateDur >= prWifiVar->u4LowRateUevtIntv &&
		!prAdapter->fgSendTxUevt) {
		DBGLOG_LIMITED(SW4, TRACE,
			"now:%u lastTxUevt:%u ReptIntv:%u\n", u4CurTick,
			prAdapter->u4LastLowTxRateUevt,
			prWifiVar->u4LowRateUevtReptIntv);
		if (!CHECK_FOR_TIMEOUT(u4CurTick,
			prAdapter->u4LastLowTxRateUevt,
			MSEC_TO_SYSTIME(SEC_TO_MSEC(
				prWifiVar->u4LowRateUevtReptIntv))))
			return;

		DBGLOG(SW4, INFO,
			"Send uevent %s Rate:%u,Tput:%u,Count:%llu",
			"abnormaltrx=DIR:TX,Event:LowRate",
			u4TxRateMbps, u4TxTput, u8TxTotalCntDif);
		kalSnprintf(uevent, sizeof(uevent),
			"abnormaltrx=DIR:TX,Event:LowRate Rate:%u,Tput:%u,Count:%llu",
			u4TxRateMbps, u4TxTput, u8TxTotalCntDif);
		kalSendUevent(prAdapter, uevent);
		prAdapter->fgSendTxUevt = TRUE;
		prAdapter->u4LastLowTxRateUevt = u4CurTick;
	}

	if (prAdapter->u4LowRxRateDur >= prWifiVar->u4LowRateUevtIntv &&
		!prAdapter->fgSendRxUevt) {
		DBGLOG_LIMITED(SW4, TRACE,
			"now:%u lastRxUevt:%u ReptIntv:%u\n", u4CurTick,
			prAdapter->u4LastLowRxRateUevt,
			prWifiVar->u4LowRateUevtReptIntv);
		if (!CHECK_FOR_TIMEOUT(u4CurTick,
			prAdapter->u4LastLowRxRateUevt,
			MSEC_TO_SYSTIME(SEC_TO_MSEC(
				prWifiVar->u4LowRateUevtReptIntv))))
			return;

		DBGLOG(SW4, INFO,
			"Send uevent %s Rate:%u,Tput:%u,Count:%llu",
			"abnormaltrx=DIR:RX,Event:LowRate",
			u4RxRateMbps, u4RxTput, u8RxTotalCntDif);
		kalSnprintf(uevent, sizeof(uevent),
			"abnormaltrx=DIR:RX,Event:LowRate Rate:%u,Tput:%u,Count:%llu",
			u4RxRateMbps, u4RxTput, u8RxTotalCntDif);
		kalSendUevent(prAdapter, uevent);
		prAdapter->fgSendRxUevt = TRUE;
		prAdapter->u4LastLowRxRateUevt = u4CurTick;
	}
}

void wlanCustomMonitorFunction(struct ADAPTER *prAdapter,
	 struct WIFI_LINK_QUALITY_INFO *prLinkQualityInfo, uint8_t ucBssIdx)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;
	uint64_t u8TxTotalCntDif;

	u8TxTotalCntDif = (prLinkQualityInfo->u8TxTotalCount >
			   prLinkQualityInfo->u8LastTxTotalCount) ?
			  (prLinkQualityInfo->u8TxTotalCount -
			   prLinkQualityInfo->u8LastTxTotalCount) : 0;

	/* Add custom monitor here */
	if (u8TxTotalCntDif >= prWifiVar->u4TrafficThreshold) {
		if (prLinkQualityInfo->u4CurTxRate <
			prWifiVar->u4TxLowRateThreshole)
			KAL_REPORT_ERROR_EVENT(prAdapter,
				EVENT_TX_LOW_RATE,
				(uint16_t)sizeof(uint32_t),
				ucBssIdx,
				FALSE);
		else if (prLinkQualityInfo->u4CurRxRate <
			prWifiVar->u4RxLowRateThreshole)
			KAL_REPORT_ERROR_EVENT(prAdapter,
				EVENT_RX_LOW_RATE,
				(uint16_t)sizeof(uint32_t),
				ucBssIdx,
				FALSE);
		else if (prLinkQualityInfo->u4CurTxPer >
			prWifiVar->u4PerHighThreshole)
			KAL_REPORT_ERROR_EVENT(prAdapter,
				EVENT_PER_HIGH,
				(uint16_t)sizeof(uint32_t),
				ucBssIdx,
				FALSE);
	}

	if (IS_FEATURE_ENABLED(prWifiVar->fgLowRateUevtEn))
		wlanLowDataRateMonitor(prAdapter, prLinkQualityInfo, ucBssIdx);
}
#endif

uint8_t wlanCheckExtCapBit(struct STA_RECORD *prStaRec, uint8_t *pucIE,
		uint8_t ucTargetBit)
{
	uint8_t *pucIeExtCap;
	uint8_t ucTargetByteIndex = (ucTargetBit >> 3);
	uint8_t ucTargetBitInByte = ucTargetBit % 8;

	if ((prStaRec == NULL) || (pucIE == NULL))
		return FALSE;

	if (IE_ID(pucIE) != ELEM_ID_EXTENDED_CAP)
		return FALSE;

	if (IE_LEN(pucIE) < (ucTargetByteIndex + 1))
		return FALSE;

	/* parse */
	pucIeExtCap = pucIE + 2;
	/* shift to the byte we care about */
	pucIeExtCap += ucTargetByteIndex;

	if ((*pucIeExtCap) & BIT(ucTargetBitInByte))
		return TRUE;

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief This routine is called to set enable/disable force RTS mode to FW
 *
 * \param[in]  prAdapter       A pointer to the Adapter structure.
 *
 * \retval WLAN_STATUS_SUCCESS
 */
/*----------------------------------------------------------------------------*/
uint32_t wlanSetForceRTS(
	struct ADAPTER *prAdapter,
	u_int8_t fgEnForceRTS)
{
	struct CMD_SET_FORCE_RTS rForceRts = {0};

	rForceRts.ucForceRtsEn = fgEnForceRTS;
	rForceRts.ucRtsPktNum = 0;
	DBGLOG(REQ, INFO, "fgEnForceRTS = %d\n",
			fgEnForceRTS);
	wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			    CMD_ID_SET_FORCE_RTS,	/* ucCID */
			    TRUE,	/* fgSetQuery */
			    FALSE,	/* fgNeedResp */
			    FALSE,	/* fgIsOid */
			    NULL,	/* pfCmdDoneHandler */
			    NULL,	/* pfCmdTimeoutHandler */
			    sizeof(struct CMD_SET_FORCE_RTS),
			    (uint8_t *)&rForceRts,	/* pucInfoBuffer */
			    NULL,	/* pvSetQueryBuffer */
			    0	/* u4SetQueryBufferLen */
	);

	return WLAN_STATUS_SUCCESS;
}

void
wlanLoadDefaultCustomerSetting(struct ADAPTER *
	prAdapter)
{

	uint8_t ucItemNum, i;

	/* default setting*/
	ucItemNum = ARRAY_SIZE(g_rDefaulteSetting);

	DBGLOG(INIT, STATE, "Default firmware setting %d item\n",
			ucItemNum);


	for (i = 0; i < ucItemNum; i++) {
		wlanCfgSet(prAdapter,
			g_rDefaulteSetting[i].aucKey,
			g_rDefaulteSetting[i].aucValue,
			g_rDefaulteSetting[i].u4Flag);
		DBGLOG(INIT, TRACE, "%s with %s\n",
			g_rDefaulteSetting[i].aucKey,
			g_rDefaulteSetting[i].aucValue);
	}


#if 1
	/*If need to re-parsing , included wlanInitFeatureOption*/
	wlanInitFeatureOption(prAdapter);
#endif
}
/*wlan on*/
void
wlanResoreEmCfgSetting(struct ADAPTER *
	prAdapter)
{
	uint32_t i;

	for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {

		if (g_rEmCfgBk[i].aucKey[0] == '\0')
			continue;

		wlanCfgSet(prAdapter, g_rEmCfgBk[i].aucKey,
			g_rEmCfgBk[i].aucValue, WLAN_CFG_EM);

		DBGLOG(INIT, STATE,
			   "cfg restore:(%s,%s) op:%d\n",
			   g_rEmCfgBk[i].aucKey,
			   g_rEmCfgBk[i].aucValue,
			   g_rEmCfgBk[i].u4Flag);

	}

}

/*wlan off*/
void
wlanBackupEmCfgSetting(struct ADAPTER *
	prAdapter)
{
	uint32_t i;
	struct WLAN_CFG_ENTRY *prWlanCfgEntry = NULL;

	kalMemZero(&g_rEmCfgBk, sizeof(g_rEmCfgBk));

	for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {
		prWlanCfgEntry =
			wlanCfgGetEntryByIndex(prAdapter, i, WLAN_CFG_EM);

		if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0'))
			break;


		kalStrnCpy(g_rEmCfgBk[i].aucKey, prWlanCfgEntry->aucKey,
			WLAN_CFG_KEY_LEN_MAX - 1);
		prWlanCfgEntry->aucKey[WLAN_CFG_KEY_LEN_MAX - 1] = '\0';

		kalStrnCpy(g_rEmCfgBk[i].aucValue, prWlanCfgEntry->aucValue,
			WLAN_CFG_VALUE_LEN_MAX - 1);
		prWlanCfgEntry->aucValue[WLAN_CFG_VALUE_LEN_MAX - 1] = '\0';


		g_rEmCfgBk[i].u4Flag = WLAN_CFG_EM;

		DBGLOG(INIT, STATE,
			   "cfg backup:(%s,%s) op:%d\n",
			   g_rEmCfgBk[i].aucKey,
			   g_rEmCfgBk[i].aucValue,
			   g_rEmCfgBk[i].u4Flag);

	}


}

void
wlanCleanAllEmCfgSetting(struct ADAPTER *
	prAdapter)
{
	uint32_t i;
	struct WLAN_CFG_ENTRY *prWlanCfgEntry = NULL;

	for (i = 0; i < WLAN_CFG_ENTRY_NUM_MAX; i++) {
		prWlanCfgEntry =
			wlanCfgGetEntryByIndex(prAdapter, i, WLAN_CFG_EM);

		if ((!prWlanCfgEntry) || (prWlanCfgEntry->aucKey[0] == '\0'))
			break;

		DBGLOG(INIT, STATE,
			   "cfg clean:(%s,%s) op:%d\n",
			   prWlanCfgEntry->aucKey,
			   prWlanCfgEntry->aucValue,
			   prWlanCfgEntry->u4Flags);

		kalMemZero(prWlanCfgEntry, sizeof(struct WLAN_CFG_ENTRY));

	}
}

u_int8_t wlanWfdEnabled(struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_WFD
	if (prAdapter)
		return prAdapter->rWifiVar.rWfdConfigureSettings.ucWfdEnable;
#endif
	return FALSE;
}

int wlanChipConfig(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen)
{
	return wlanChipConfigWithType(prAdapter, pcCommand,
				i4TotalLen, CHIP_CONFIG_TYPE_ASCII);
}

int wlanChipConfigWithType(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen, uint8_t type)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4BytesWritten = 0;
	uint32_t u4BufLen = 0;
	uint32_t u2MsgSize = 0;
	uint32_t u4CmdLen = 0;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

	if (prAdapter == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter null");
		return -1;
	}
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

	rChipConfigInfo.ucType = type;
	rChipConfigInfo.u2MsgSize = u4CmdLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand,
		   CHIP_CONFIG_RESP_SIZE - 1);
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';
	rStatus = kalIoctl(prAdapter->prGlueInfo, wlanoidQueryChipConfig,
		&rChipConfigInfo, sizeof(rChipConfigInfo), &u4BufLen);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "%s: kalIoctl ret=%d\n", __func__,
		       rStatus);
		return -1;
	}
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

	/* Check respType */
	u2MsgSize = rChipConfigInfo.u2MsgSize;
	DBGLOG(REQ, INFO, "%s: RespTyep  %u\n", __func__,
	       rChipConfigInfo.ucRespType);
	DBGLOG(REQ, INFO, "%s: u2MsgSize %u\n", __func__,
	       rChipConfigInfo.u2MsgSize);

	if (rChipConfigInfo.ucRespType != type) {
		DBGLOG(REQ, WARN, "only return as ASCII");
		return -1;
	}
	if (u2MsgSize > sizeof(rChipConfigInfo.aucCmd)) {
		DBGLOG(REQ, INFO, "%s: u2MsgSize error ret=%u\n",
		       __func__, rChipConfigInfo.u2MsgSize);
		return -1;
	}
	i4BytesWritten = snprintf(pcCommand, i4TotalLen, "%s",
		     rChipConfigInfo.aucCmd);
	if (i4BytesWritten < 0) {
		DBGLOG(REQ, INFO, "%s: snprintf error ret=%d\n",
		       __func__, i4BytesWritten);
		return -1;
	}

	return i4BytesWritten;
}

int wlanChipCommand(struct ADAPTER *prAdapter,
	char *pcCommand, int i4TotalLen)
{
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	int32_t i4BytesWritten = 0;
	uint32_t u4BufLen = 0;
	uint32_t u4CmdLen = 0;
	struct PARAM_CUSTOM_CHIP_CONFIG_STRUCT rChipConfigInfo = {0};

	if (prAdapter == NULL) {
		DBGLOG(REQ, ERROR, "prAdapter null");
		return -1;
	}
	DBGLOG(REQ, LOUD, "command is %s\n", pcCommand);

	u4CmdLen = kalStrnLen(pcCommand, i4TotalLen);

	rChipConfigInfo.ucType = CHIP_CONFIG_TYPE_WO_RESPONSE;
	rChipConfigInfo.u2MsgSize = u4CmdLen;
	kalStrnCpy(rChipConfigInfo.aucCmd, pcCommand,
		   CHIP_CONFIG_RESP_SIZE - 1);
	rChipConfigInfo.aucCmd[CHIP_CONFIG_RESP_SIZE - 1] = '\0';

	rStatus = wlanSetChipConfig(prAdapter,
				&rChipConfigInfo, sizeof(rChipConfigInfo),
				&u4BufLen, FALSE);

	if (rStatus != WLAN_STATUS_SUCCESS) {
		DBGLOG(REQ, ERROR, "wlan ret=%d\n", rStatus);
		i4BytesWritten = -1;
	}

	return i4BytesWritten;
}

uint32_t wlanSetRxBaSize(struct GLUE_INFO *prGlueInfo,
	int8_t i4Type, uint16_t u2BaSize)
{
	struct ADAPTER *prAdapter;
	uint32_t i;
	struct STA_RECORD *prStaRec;
	struct CMD_ADDBA_REJECT rAddbaReject;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	prAdapter = prGlueInfo->prAdapter;
#if (CFG_SUPPORT_802_11BE == 1)
	if (i4Type == WLAN_TYPE_EHT)
		prAdapter->rWifiVar.u2RxEhtBaSize = u2BaSize;
	else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (i4Type == WLAN_TYPE_HE)
		prAdapter->rWifiVar.u2RxHeBaSize = u2BaSize;
	else
#endif
	{
		prAdapter->rWifiVar.ucRxHtBaSize = (uint8_t) u2BaSize;
		prAdapter->rWifiVar.ucRxVhtBaSize = (uint8_t) u2BaSize;
	}

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];
		if (prStaRec->fgIsInUse)
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
	}

	rAddbaReject.fgEnable = FALSE;
	rAddbaReject.fgApply = TRUE;

	rStatus = kalIoctl(prGlueInfo,
		wlanoidSetAddbaReject, &rAddbaReject,
		sizeof(struct CMD_ADDBA_REJECT), &u4BufLen);

	DBGLOG(OID, INFO, "%s i4Type:%d BaSize:%d\n",
		__func__, i4Type, u2BaSize);

	return rStatus;
}

uint32_t wlanSetTxBaSize(struct GLUE_INFO *prGlueInfo,
	int8_t i4Type, uint16_t u2BaSize)
{
	struct ADAPTER *prAdapter;
	uint32_t i;
	struct STA_RECORD *prStaRec;
	struct CMD_TX_AMPDU rTxAmpdu;
	uint32_t rStatus = WLAN_STATUS_SUCCESS;
	uint32_t u4BufLen = 0;

	prAdapter = prGlueInfo->prAdapter;
#if (CFG_SUPPORT_802_11BE == 1)
	if (i4Type == WLAN_TYPE_EHT)
		prAdapter->rWifiVar.u2TxEhtBaSize = u2BaSize;
	else
#endif
#if (CFG_SUPPORT_802_11AX == 1)
	if (i4Type == WLAN_TYPE_HE)
		prAdapter->rWifiVar.u2TxHeBaSize = u2BaSize;
	else
#endif
		prAdapter->rWifiVar.ucTxBaSize = (uint8_t) u2BaSize;

	for (i = 0; i < CFG_STA_REC_NUM; i++) {
		prStaRec = &prAdapter->arStaRec[i];
		if (prStaRec->fgIsInUse)
			cnmStaSendUpdateCmd(prAdapter, prStaRec, NULL, FALSE);
	}

	rTxAmpdu.fgEnable = TRUE;
	rTxAmpdu.fgApply = TRUE;

	rStatus = kalIoctl(prGlueInfo, wlanoidSetTxAmpdu,
			&rTxAmpdu, sizeof(struct CMD_TX_AMPDU), &u4BufLen);

	DBGLOG(OID, INFO, "%s i4Type:%d BaSize:%d\n",
		__func__, i4Type, u2BaSize);

	return rStatus;
}

void
wlanGetTRXInfo(struct ADAPTER *prAdapter,
	struct TRX_INFO *prTRxInfo)
{
	char arQueryMib[64] = "getMibCount";
	char arQueryTRx[64] = "getTxRxCount";
	uint8_t *pucItem = NULL;
	char *pucSavedPtr = NULL;
	uint32_t u4temp = 0;
	uint32_t index = 0;

	wlanChipConfig(prAdapter, &arQueryMib[0], sizeof(arQueryMib));
	DBGLOG(REQ, INFO, "Mib:%s\n", arQueryMib);
	pucItem = (uint8_t *)kalStrtokR(&arQueryMib[0], " ", &pucSavedPtr);
	while (pucItem) {
		kalkStrtou32(pucItem, 0, &u4temp);
		*(((uint32_t *)prTRxInfo) + index) = u4temp;
		pucItem =
			(uint8_t *)kalStrtokR(NULL, " ", &pucSavedPtr);
		index++;
	}
	DBGLOG(REQ, INFO, "TxFail:%d %d, RxFail:%d %d, Retry: %d %d\n",
		prTRxInfo->u4TxFail[0], prTRxInfo->u4TxFail[1],
		prTRxInfo->u4RxFail[0], prTRxInfo->u4RxFail[1],
		prTRxInfo->u4TxHwRetry[0], prTRxInfo->u4TxHwRetry[1]);

	pucItem = NULL;
	pucSavedPtr = NULL;
	u4temp = 0;
	index = 0;
	wlanChipConfig(prAdapter, &arQueryTRx[0], sizeof(arQueryTRx));
	DBGLOG(REQ, INFO, "TRX:%s\n", arQueryTRx);
	pucItem = kalStrtokR(&arQueryTRx[0], " ", &pucSavedPtr);
	while (pucItem) {
		kalkStrtou32(pucItem, 0, &u4temp);
		if (index % 2 == 0)
			prTRxInfo->u4TxOk[index / 2] = u4temp;
		else
			prTRxInfo->u4RxOk[index / 2] = u4temp;
		pucItem = kalStrtokR(NULL, " ", &pucSavedPtr);
		index++;
	}
	DBGLOG(REQ, INFO, "TxOk:%d %d %d %d, RxOk:%d %d %d %d\n",
		prTRxInfo->u4TxOk[0], prTRxInfo->u4TxOk[1],
		prTRxInfo->u4TxOk[2], prTRxInfo->u4TxOk[3],
		prTRxInfo->u4RxOk[0], prTRxInfo->u4RxOk[1],
		prTRxInfo->u4RxOk[2], prTRxInfo->u4RxOk[3]);
}

void wlanGetChipDbgOps(struct ADAPTER *prAdapter, uint32_t **pu4Handle)
{
	struct mt66xx_chip_info *prChipInfo;

	ASSERT(prAdapter);
	prChipInfo = prAdapter->chip_info;
	ASSERT(prChipInfo);

	*pu4Handle = (uint32_t *)(prChipInfo->prDebugOps);
}

#if (CFG_WOW_SUPPORT == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This is a routine, which is used to release tx cmd after bus suspend
 *
 * \param prAdapter      Pointer of Adapter Data Structure
 */
/*----------------------------------------------------------------------------*/
void wlanReleaseAllTxCmdQueue(struct ADAPTER *prAdapter)
{
	if (prAdapter == NULL)
		return;

	/* dump queue info before release for debug */
	cmdBufDumpCmdQueue(&prAdapter->rPendingCmdQueue,
				   "waiting response CMD queue");
#if CFG_SUPPORT_MULTITHREAD
	cmdBufDumpCmdQueue(&prAdapter->rTxCmdQueue,
				   "Tx CMD queue");
#endif

	DBGLOG(OID, INFO, "Remove all pending Cmd\n");
	/* 1: Clear Pending OID */
	wlanReleasePendingOid(prAdapter, 1);

	/* Release all CMD/MGMT/CmdData frame in command queue */
	kalClearCommandQueue(prAdapter->prGlueInfo, TRUE);

	/* Release all CMD in pending command queue */
	wlanClearPendingCommandQueue(prAdapter);

#if CFG_SUPPORT_MULTITHREAD

	/* Flush all items in queues for multi-thread */
	wlanClearTxCommandQueue(prAdapter);

	wlanClearTxCommandDoneQueue(prAdapter);

#endif
}

void
wlanWaitCfg80211SuspendDone(struct GLUE_INFO *prGlueInfo)
{
	uint8_t u1Count = 0;
#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
	uint32_t u4Ret = 0;
	uint8_t fgWaitCompletion = FALSE;
	struct GL_P2P_INFO *prP2PInfo;
	uint8_t ucRoleIndex;
#endif

	if (prGlueInfo->prAdapter == NULL)
		return;

	while (!(KAL_TEST_BIT(SUSPEND_FLAG_CLEAR_WHEN_RESUME,
		prGlueInfo->prAdapter->ulSuspendFlag))) {
		if (u1Count > HIF_SUSPEND_MAX_WAIT_TIME) {
			DBGLOG(HAL, ERROR, "cfg80211 not suspend\n");
			/* no cfg80211 suspend called */
			/* do pre-suspend flow here */
			aisPreSuspendFlow(prGlueInfo->prAdapter);
#if CFG_ENABLE_WIFI_DIRECT
			p2pRoleProcessPreSuspendFlow(prGlueInfo->prAdapter);
#endif
			break;
		}
		kalUsleep_range(5000, 6000);
		u1Count++;
		DBGLOG(HAL, TRACE, "wait cfg80211 suspend %d\n", u1Count);
	}

#if (CFG_SUPPORT_SUSPEND_NOTIFY_APGO_STOP == 1)
	for (ucRoleIndex = 0; ucRoleIndex < BSS_P2P_NUM; ucRoleIndex++) {
		prP2PInfo = prGlueInfo->prP2PInfo[ucRoleIndex];
		if (prP2PInfo &&
			KAL_TEST_BIT(SUSPEND_STOP_APGO_WAITING_0,
				prP2PInfo->ulSuspendStopAp)) {
			fgWaitCompletion = TRUE;
			break;
		}
	}
	if (fgWaitCompletion) {
		u4Ret = wait_for_completion_timeout(
			&prP2PInfo->rSuspendStopApComp,
			MSEC_TO_JIFFIES(P2P_DEAUTH_TIMEOUT_TIME_MS));
		if (!u4Ret)
			DBGLOG(P2P, WARN, "timeout\n");
		else
			DBGLOG(P2P, INFO, "complete\n");
		KAL_CLR_BIT(SUSPEND_STOP_APGO_WAITING_0,
			prP2PInfo->ulSuspendStopAp);
	}
#endif
}
#endif /* #if (CFG_WOW_SUPPORT == 1) */

void wlanSetConnsysFwLog(struct ADAPTER *prAdapter)
{
#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	struct CMD_CONNSYS_FW_LOG rFwLogCmd;
	uint32_t u4BufLen;
#else
	int32_t u4LogLevel = ENUM_WIFI_LOG_LEVEL_DEFAULT;
#endif

#ifdef CFG_MTK_CONNSYS_DEDICATED_LOG_PATH
	kalMemZero(&rFwLogCmd, sizeof(rFwLogCmd));

	rFwLogCmd.fgCmd = (int)FW_LOG_CMD_ON_OFF;
	rFwLogCmd.fgValue = getFWLogOnOff();
	rFwLogCmd.fgEarlySet = TRUE;

	connsysFwLogControl(prAdapter,
		&rFwLogCmd,
		sizeof(struct CMD_CONNSYS_FW_LOG),
		&u4BufLen);

	if (getFWLogLevel() != -1) {
		rFwLogCmd.fgCmd =
			(int)FW_LOG_CMD_SET_LEVEL;
		rFwLogCmd.fgValue = getFWLogLevel();
		rFwLogCmd.fgEarlySet = TRUE;

		connsysFwLogControl(prAdapter,
		&rFwLogCmd,
		sizeof(struct CMD_CONNSYS_FW_LOG),
		&u4BufLen);
	}
#else
	/* Enable FW log */
	wlanDbgGetGlobalLogLevel(
		ENUM_WIFI_LOG_MODULE_FW, &u4LogLevel);
	if (u4LogLevel > ENUM_WIFI_LOG_LEVEL_DEFAULT)
		wlanDbgSetLogLevel(prAdapter,
			ENUM_WIFI_LOG_LEVEL_VERSION_V1,
			ENUM_WIFI_LOG_MODULE_FW,
			u4LogLevel, TRUE);
#endif
}

uint32_t wlanSendFwLogControlCmd(struct ADAPTER *prAdapter,
				uint8_t ucCID,
				PFN_CMD_DONE_HANDLER pfCmdDoneHandler,
				PFN_CMD_TIMEOUT_HANDLER pfCmdTimeoutHandler,
				uint32_t u4SetQueryInfoLen,
				int8_t *pucInfoBuffer)
{
#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	return wlanSendSetQueryCmdHelper(
#else
	return wlanSendSetQueryCmdAdv(
#endif
		prAdapter, ucCID, 0, TRUE, FALSE, FALSE,
		pfCmdDoneHandler, pfCmdTimeoutHandler, u4SetQueryInfoLen,
		pucInfoBuffer, NULL, 0, CMD_SEND_METHOD_REQ_RESOURCE);
}

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
uint32_t wlanSetEd(struct ADAPTER *prAdapter, int32_t i4EdVal2G,
	int32_t i4EdVal5G, uint32_t u4Sel)
{
	uint32_t u4BufLen = 0;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct PARAM_CUSTOM_SW_CTRL_STRUCT rSwCtrlInfo;

	rSwCtrlInfo.u4Id = CMD_SW_DBGCTL_ADVCTL_SET_ID + CMD_ADVCTL_ED_ID;
	rSwCtrlInfo.u4Data = ((i4EdVal2G & 0xFF) |
		((i4EdVal5G & 0xFF)<<16) | (u4Sel << 31));

	DBGLOG(REQ, INFO, "rSwCtrlInfo.u4Data=0x%x,\n", rSwCtrlInfo.u4Data);

	return kalIoctl(prGlueInfo, wlanoidSetSwCtrlWrite, &rSwCtrlInfo,
		sizeof(rSwCtrlInfo), &u4BufLen);
}
#endif

#if (CFG_WIFI_GET_MCS_INFO == 1)
void wlanRxMcsInfoMonitor(struct ADAPTER *prAdapter,
					    uintptr_t ulParamPtr)
{
	static uint8_t ucSmapleCnt;
	uint8_t ucBssIdx = 0;
	struct STA_RECORD *prStaRec;

	ucBssIdx = GET_IOCTL_BSSIDX(prAdapter);
	prStaRec = aisGetTargetStaRec(prAdapter, ucBssIdx);

	if (prStaRec == NULL)
		goto out;

	if (prStaRec->fgIsValid && prStaRec->fgIsInUse) {
		prStaRec->au4RxV0[ucSmapleCnt] = prStaRec->au4RxV[0];
		prStaRec->au4RxV1[ucSmapleCnt] = prStaRec->au4RxV[1];
		prStaRec->au4RxV2[ucSmapleCnt] = prStaRec->au4RxV[2];

		ucSmapleCnt = (ucSmapleCnt + 1) % MCS_INFO_SAMPLE_CNT;
	}

out:
	cnmTimerStartTimer(prAdapter, &prAdapter->rRxMcsInfoTimer,
			   MCS_INFO_SAMPLE_PERIOD);
}
#endif /* CFG_WIFI_GET_MCS_INFO */

uint32_t wlanQueryThermalTemp(struct ADAPTER *ad,
	struct THERMAL_TEMP_DATA *data)
{
	struct GLUE_INFO *glue = ad->prGlueInfo;
	PFN_OID_HANDLER_FUNC handler = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t len = 0;

	if (!data)
		return WLAN_STATUS_FAILURE;

	if (data->eType == THERMAL_TEMP_TYPE_ADIE)
		handler = wlanoidQueryThermalAdieTemp;
	else
		handler = wlanoidQueryThermalDdieTemp;

	status = kalIoctl(glue, handler, data, sizeof(*data), &len);

	return status;
}

uint32_t wlanQueryThermalTempV2(struct ADAPTER *ad,
	struct THERMAL_TEMP_DATA_V2 *data)
{
	struct GLUE_INFO *glue = ad->prGlueInfo;
	PFN_OID_HANDLER_FUNC handler = NULL;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t len = 0;

	if (!data)
		return WLAN_STATUS_FAILURE;

	handler = wlanoidQueryThermalAdcTemp;

	status = kalIoctl(glue, handler, data, sizeof(*data), &len);

	return status;
}

uint32_t wlanSetRFTestModeCMD(struct GLUE_INFO *prGlueInfo, bool fgEn)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
#if CFG_SUPPORT_QA_TOOL
	PFN_OID_HANDLER_FUNC handler = NULL;
	uint32_t u4Buflen = 0;

	DBGLOG(REQ, INFO, "%s set %s Test Mode CMD\n", __func__,
		(fgEn) ? "Enter" : "Abort");

	if (fgEn == 1)
		handler = wlanoidRftestSetTestMode;
	else
		handler = wlanoidRftestSetAbortTestMode;

	u4Status = kalIoctl(prGlueInfo, handler, NULL, 0, &u4Buflen);

	DBGLOG(REQ, INFO, "%s status : %d\n", __func__, u4Status);
#endif
	return u4Status;
}

int8_t hexDigitToInt(uint8_t ch)
{
	if (ch >= 'a' && ch <= 'f')
		return 10 + ch - 'a';
	else if (ch >= 'A' && ch <= 'F')
		return 10 + ch - 'A';
	else if (ch >= '0' && ch <= '9')
		return ch - '0';

	return 0;
}

#if (CFG_SUPPORT_XONVRAM && CFG_SUPPORT_QA_TOOL)
uint32_t wlanTestModeXoCal(struct ADAPTER *ad,
	struct TEST_MODE_XO_CAL *data)
{
	struct GLUE_INFO *glue = ad->prGlueInfo;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t len = 0;

	if (!data)
		return WLAN_STATUS_FAILURE;

	status = kalIoctl(glue,
				wlanoidRftestDoXOCal,
				data,
				sizeof(*data),
				&len);

	return status;
}
#endif /* CFG_SUPPORT_XONVRAM */

#if CFG_SUPPORT_PLCAL
uint32_t wlanTestModePlCal(struct ADAPTER *ad,
	struct TEST_MODE_PL_CAL *data)
{
	struct GLUE_INFO *glue = ad->prGlueInfo;
	uint32_t status = WLAN_STATUS_SUCCESS;
	uint32_t len = 0;

	if (!data)
		return WLAN_STATUS_FAILURE;

	status = kalIoctl(glue,
				wlanoidRftestDoPlCal,
				data,
				sizeof(*data),
				&len);

	return status;
}
#endif /* CFG_SUPPORT_PLCAL */
