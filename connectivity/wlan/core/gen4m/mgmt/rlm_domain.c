/******************************************************************************
 *
 * This file is provided under a dual license.  When you use or
 * distribute this software, you may choose to be licensed under
 * version 2 of the GNU General Public License ("GPLv2 License")
 * or BSD License.
 *
 * GPLv2 License
 *
 * Copyright(C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 *
 * BSD LICENSE
 *
 * Copyright(C) 2016 MediaTek Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
/*
 * Id: //Department/DaVinci/BRANCHES/MT6620_WIFI_DRIVER_V2_3/mgmt/rlm_domain.c#2
 */

/*! \file   "rlm_domain.c"
 *    \brief
 *
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
#include "rlm_txpwr_init.h"

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
char *g_au1TxPwrChlTypeLabel[] = {
	"NORMAL",
	"ALL",
	"RANGE",
	"2G4",
	"5G",
	"BANDEDGE_2G4",
	"BANDEDGE_5G",
	"5GBAND1",
	"5GBAND2",
	"5GBAND3",
	"5GBAND4",
};

char *g_au1TxPwrAppliedWayLabel[] = {
	"wifi on",
	"ioctl"
};

char *g_au1TxPwrOperationLabel[] = {
	"power level",
	"power offset"
};

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
typedef int32_t (*PFN_TX_PWR_TAG_PARA_FUNC) (
	char *, char *, uint8_t, struct TX_PWR_CTRL_ELEMENT *);

struct TX_PWR_TAG_TABLE {
	const char arTagNames[32];
	uint8_t ucTagParaNum;
	PFN_TX_PWR_TAG_PARA_FUNC pfnParseTagParaHandler;
} g_auTxPwrTagTable[] = {
	{
		"MIMO_1T",
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		txPwrParseTagMimo1T
	},
	{
		"MIMO_2T",
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		txPwrParseTagMimo2T
	},
	{
		"ALL_T",
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		txPwrParseTagAllT
	},
	{
		"ALL_T_6G",
		(POWER_ANT_6G_BAND_NUM * POWER_ANT_NUM),
		txPwrParseTagAllT6G
	}
};
#endif

#endif

#define PWR_BUF_LEN 1024

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
char *g_au1TxPwrDefaultSetting[] = {
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	"_SAR_PwrLevel;1;2;1;[2G4,,,,,,,,,,,,][5G,,,,,,,,,,,,]",
	"_G_Scenario;1;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;2;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;3;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;4;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;5;2;1;[ALL,,,,,,,,,,,,]",
#else
	/*
	* lct.zhangzichao1 Distinguish between factory and user RF parameters begin
	* jira :LCFEAT-123505
	* Distinguish between user and factory versions of WiFi SAR parameters.
	*/
	#ifdef CONFIG_FACTORY_BUILD
		"_SAR_PwrLevel;1;2;1;[2G4,,,,,,,,,][5G,,,,,,,,,]",
		"_G_Scenario;1;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;2;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;3;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;4;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;5;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;6;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;7;2;1;[ALL,,,,,,,,,]",
		"_G_Scenario;8;2;1;[ALL,,,,,,,,,]",
	#else
		"_SAR_PwrLevel;1;2;1;[2G4,,,,,,,,,][5G,,,,,,,,,]",
		"_G_Scenario;1;2;1;[ALL,,,,,,,,,]", // default
		"_G_Scenario;2;2;1;[ALL,,,,,,,,,]", // ant check
		"_G_Scenario;3;2;1;[2G4,,,,,,,,,][5G,,,,,,,,,]", // CN sar
		"_G_Scenario;4;2;1;[2G4,26,24,24,,,,,,][5G,22,22,22,22,22,22,22,,]", // IN sar
		"_G_Scenario;5;2;1;[2G4,26,26,26,,,,,,][5G,24,24,24,24,24,24,22,,]", // GL sar
		"_G_Scenario;6;2;1;[2G4,26,26,26,,,,,,][5G,24,24,24,24,24,24,22,,]", // GL ERP sar
		"_G_Scenario;7;2;1;[ALL,,,,,,,,,]", // JP sar
		"_G_Scenario;8;2;1;[ALL,,,,,,,,,]", // other sar
	#endif
	/* lct.zhangzichao1 Distinguish between factory and user RF parameters end */
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_OFDM_SETTING */
};
#endif
/* The following country or domain shall be set from host driver.
 * And host driver should pass specified DOMAIN_INFO_ENTRY to MT6620 as
 * the channel list of being a STA to do scanning/searching AP or being an
 * AP to choose an adequate channel if auto-channel is set.
 */

/* Define mapping tables between country code and its channel set
 */
static const uint16_t g_u2CountryGroup0[] = {
	COUNTRY_CODE_AD, COUNTRY_CODE_AF, COUNTRY_CODE_AO, COUNTRY_CODE_AZ,
	COUNTRY_CODE_BF, COUNTRY_CODE_BG, COUNTRY_CODE_BI, COUNTRY_CODE_BJ,
	COUNTRY_CODE_BT, COUNTRY_CODE_BW, COUNTRY_CODE_CD, COUNTRY_CODE_CF,
	COUNTRY_CODE_CG, COUNTRY_CODE_CI, COUNTRY_CODE_CM, COUNTRY_CODE_CV,
	COUNTRY_CODE_DJ, COUNTRY_CODE_FO, COUNTRY_CODE_GA, COUNTRY_CODE_GE,
	COUNTRY_CODE_GF, COUNTRY_CODE_GG, COUNTRY_CODE_GL, COUNTRY_CODE_GM,
	COUNTRY_CODE_GN, COUNTRY_CODE_GP, COUNTRY_CODE_GQ, COUNTRY_CODE_GW,
	COUNTRY_CODE_IM, COUNTRY_CODE_IQ, COUNTRY_CODE_JE, COUNTRY_CODE_KE,
	COUNTRY_CODE_KM, COUNTRY_CODE_KW, COUNTRY_CODE_LB, COUNTRY_CODE_LI,
	COUNTRY_CODE_LS, COUNTRY_CODE_LY, COUNTRY_CODE_MC, COUNTRY_CODE_MD,
	COUNTRY_CODE_ME, COUNTRY_CODE_MK, COUNTRY_CODE_ML, COUNTRY_CODE_MQ,
	COUNTRY_CODE_MR, COUNTRY_CODE_MU, COUNTRY_CODE_MZ, COUNTRY_CODE_NC,
	COUNTRY_CODE_NE, COUNTRY_CODE_NR, COUNTRY_CODE_PF, COUNTRY_CODE_PM,
	COUNTRY_CODE_RE, COUNTRY_CODE_RO, COUNTRY_CODE_RS, COUNTRY_CODE_SM,
	COUNTRY_CODE_SO, COUNTRY_CODE_ST, COUNTRY_CODE_SZ, COUNTRY_CODE_TD,
	COUNTRY_CODE_TF, COUNTRY_CODE_TG, COUNTRY_CODE_TJ, COUNTRY_CODE_TM,
	COUNTRY_CODE_TR, COUNTRY_CODE_TV, COUNTRY_CODE_TZ, COUNTRY_CODE_VA,
	COUNTRY_CODE_WF, COUNTRY_CODE_YT, COUNTRY_CODE_ZM
};
static const uint16_t g_u2CountryGroup1[] = {
	COUNTRY_CODE_AG, COUNTRY_CODE_AI, COUNTRY_CODE_AM, COUNTRY_CODE_AN,
	COUNTRY_CODE_AQ, COUNTRY_CODE_AW, COUNTRY_CODE_AX, COUNTRY_CODE_BB,
	COUNTRY_CODE_BM, COUNTRY_CODE_BN, COUNTRY_CODE_BO, COUNTRY_CODE_BS,
	COUNTRY_CODE_BV, COUNTRY_CODE_BZ, COUNTRY_CODE_CO, COUNTRY_CODE_DO,
	COUNTRY_CODE_EC, COUNTRY_CODE_FJ, COUNTRY_CODE_FK, COUNTRY_CODE_FM,
	COUNTRY_CODE_GD, COUNTRY_CODE_GI, COUNTRY_CODE_GS, COUNTRY_CODE_GY,
	COUNTRY_CODE_HN, COUNTRY_CODE_HT, COUNTRY_CODE_IL, COUNTRY_CODE_IN,
	COUNTRY_CODE_IO, COUNTRY_CODE_IR, COUNTRY_CODE_KG, COUNTRY_CODE_KH,
	COUNTRY_CODE_KN, COUNTRY_CODE_KP, COUNTRY_CODE_KY, COUNTRY_CODE_ZA,
	COUNTRY_CODE_LA, COUNTRY_CODE_LC, COUNTRY_CODE_LK, COUNTRY_CODE_LR,
	COUNTRY_CODE_MH, COUNTRY_CODE_MN, COUNTRY_CODE_MO, COUNTRY_CODE_MS,
	COUNTRY_CODE_MW, COUNTRY_CODE_NA, COUNTRY_CODE_NI, COUNTRY_CODE_NU,
	COUNTRY_CODE_PA, COUNTRY_CODE_PG, COUNTRY_CODE_PH, COUNTRY_CODE_PN,
	COUNTRY_CODE_PS, COUNTRY_CODE_PW, COUNTRY_CODE_PY, COUNTRY_CODE_QA,
	COUNTRY_CODE_RW, COUNTRY_CODE_SB, COUNTRY_CODE_SC, COUNTRY_CODE_SD,
	COUNTRY_CODE_SG, COUNTRY_CODE_SH, COUNTRY_CODE_SJ, COUNTRY_CODE_SN,
	COUNTRY_CODE_SS, COUNTRY_CODE_SV, COUNTRY_CODE_SX, COUNTRY_CODE_SY,
	COUNTRY_CODE_TC, COUNTRY_CODE_TH, COUNTRY_CODE_TK, COUNTRY_CODE_TO,
	COUNTRY_CODE_TT, COUNTRY_CODE_TW, COUNTRY_CODE_UA, COUNTRY_CODE_VC,
	COUNTRY_CODE_VG, COUNTRY_CODE_VN, COUNTRY_CODE_VU, COUNTRY_CODE_WS,
	COUNTRY_CODE_YE
};
static const uint16_t g_u2CountryGroup2[] = {
	COUNTRY_CODE_BY, COUNTRY_CODE_ET, COUNTRY_CODE_EU, COUNTRY_CODE_MF,
	COUNTRY_CODE_MG, COUNTRY_CODE_MM, COUNTRY_CODE_OM, COUNTRY_CODE_SL,
	COUNTRY_CODE_SR, COUNTRY_CODE_ZW
};
static const uint16_t g_u2CountryGroup3[] = {
	COUNTRY_CODE_CU, COUNTRY_CODE_DM, COUNTRY_CODE_GT
};
static const uint16_t g_u2CountryGroup4[] = {
	COUNTRY_CODE_AR, COUNTRY_CODE_CC, COUNTRY_CODE_CX, COUNTRY_CODE_HM,
	COUNTRY_CODE_MX, COUNTRY_CODE_NF
};
static const uint16_t g_u2CountryGroup5[] = {
	COUNTRY_CODE_BH, COUNTRY_CODE_CN, COUNTRY_CODE_MV, COUNTRY_CODE_UY,
	COUNTRY_CODE_VE
};
// lc.zhangzichao1 Modify PK region channel rules jira id HTH-494370
static const uint16_t g_u2CountryGroup6[] = {
	COUNTRY_CODE_BD, COUNTRY_CODE_JM
};
static const uint16_t g_u2CountryGroup7[] = {
	COUNTRY_CODE_NG
};
static const uint16_t g_u2CountryGroup8[] = {
	COUNTRY_CODE_CA
};
static const uint16_t g_u2CountryGroup9[] = {
	COUNTRY_CODE_ER, COUNTRY_CODE_RKS,
};
static const uint16_t g_u2CountryGroup10[] = {
	COUNTRY_CODE_DZ
};
static const uint16_t g_u2CountryGroup11[] = {
	COUNTRY_CODE_EG, COUNTRY_CODE_EH, COUNTRY_CODE_MA, COUNTRY_CODE_UZ
};
static const uint16_t g_u2CountryGroup12[] = {
	COUNTRY_CODE_JO
};
static const uint16_t g_u2CountryGroup13[] = {
	COUNTRY_CODE_JP
};
static const uint16_t g_u2CountryGroup14[] = {
	COUNTRY_CODE_MY
};
static const uint16_t g_u2CountryGroup15[] = {
	COUNTRY_CODE_RU
};
static const uint16_t g_u2CountryGroup16[] = {
	COUNTRY_CODE_GH, COUNTRY_CODE_UG
};
static const uint16_t g_u2CountryGroup17[] = {
	COUNTRY_CODE_TN
};
static const uint16_t g_u2CountryGroup18[] = {
	COUNTRY_CODE_AL, COUNTRY_CODE_AT, COUNTRY_CODE_BA, COUNTRY_CODE_BE,
	COUNTRY_CODE_CH, COUNTRY_CODE_CY, COUNTRY_CODE_CZ, COUNTRY_CODE_DE,
	COUNTRY_CODE_DK, COUNTRY_CODE_EE, COUNTRY_CODE_ES, COUNTRY_CODE_FI,
	COUNTRY_CODE_FR, COUNTRY_CODE_GR, COUNTRY_CODE_HR, COUNTRY_CODE_HU,
	COUNTRY_CODE_IE, COUNTRY_CODE_IS, COUNTRY_CODE_IT, COUNTRY_CODE_LT,
	COUNTRY_CODE_LU, COUNTRY_CODE_LV, COUNTRY_CODE_MT, COUNTRY_CODE_NL,
	COUNTRY_CODE_NO, COUNTRY_CODE_PL, COUNTRY_CODE_PT, COUNTRY_CODE_SE,
	COUNTRY_CODE_SI, COUNTRY_CODE_SK, COUNTRY_CODE_XK
};
static const uint16_t g_u2CountryGroup19[] = {
	COUNTRY_CODE_AE, COUNTRY_CODE_HK
};
static const uint16_t g_u2CountryGroup20[] = {
	COUNTRY_CODE_PR
};
static const uint16_t g_u2CountryGroup21[] = {
	COUNTRY_CODE_NP
};
static const uint16_t g_u2CountryGroup22[] = {
	COUNTRY_CODE_BR, COUNTRY_CODE_CR, COUNTRY_CODE_KR, COUNTRY_CODE_PE
};
static const uint16_t g_u2CountryGroup23[] = {
	COUNTRY_CODE_GU, COUNTRY_CODE_MP, COUNTRY_CODE_UM, COUNTRY_CODE_VI
};
static const uint16_t g_u2CountryGroup24[] = {
	COUNTRY_CODE_AU
};
static const uint16_t g_u2CountryGroup25[] = {
	COUNTRY_CODE_SA
};
static const uint16_t g_u2CountryGroup26[] = {
	COUNTRY_CODE_AS, COUNTRY_CODE_US
};
static const uint16_t g_u2CountryGroup27[] = {
	COUNTRY_CODE_CK, COUNTRY_CODE_CL, COUNTRY_CODE_GB, COUNTRY_CODE_KI,
	COUNTRY_CODE_NZ, COUNTRY_CODE_TL
};
static const uint16_t g_u2CountryGroup28[] = {
	COUNTRY_CODE_ID
};

static const uint16_t g_u2CountryGroup29[] = {
	COUNTRY_CODE_KZ
};
/* lc.zhangzichao1 Modify PK region channel rules jira id HTH-494370 begin */
static const uint16_t g_u2CountryGroup30[] = {
	COUNTRY_CODE_PK
};
/* lc.zhangzichao1 Modify PK region channel rules jira id HTH-494370 end */
#if (CFG_SUPPORT_SINGLE_SKU == 1)
struct mtk_regd_control g_mtk_regd_control = {
	.en = FALSE,
	.state = REGD_STATE_UNDEFINED
};

#if CFG_SUPPORT_BW320
#define BW_6G 320
#define BW_5G 160
#elif CFG_SUPPORT_BW160
#define BW_5G 160
#define BW_6G 160
#else
#define BW_5G 80
#define BW_6G 80
#endif

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
const struct ieee80211_regdomain default_regdom_ww = {
	.n_reg_rules = 5,
	.alpha2 = "99",
	.reg_rules = {
	/* channels 1..13 */
	REG_RULE_LIGHT(2412-10, 2472+10, 40, 0),
	/* channels 14 */
	REG_RULE_LIGHT(2484-10, 2484+10, 20, 0),
	/* channel 36..64 */
	REG_RULE_LIGHT(5150-10, 5350+10, BW_5G, 0),
	/* channel 100..165 */
	REG_RULE_LIGHT(5470-10, 5850+10, BW_5G, 0),
	/* 6G channel 1..17 */
	REG_RULE_LIGHT(5935-10, 7135+10, BW_6G, 0),
	}
};
#else
const struct ieee80211_regdomain default_regdom_ww = {
	.n_reg_rules = 4,
	.alpha2 = "99",
	.reg_rules = {
	/* channels 1..13 */
	REG_RULE_LIGHT(2412-10, 2472+10, 40, 0),
	/* channels 14 */
	REG_RULE_LIGHT(2484-10, 2484+10, 20, 0),
	/* channel 36..64 */
	REG_RULE_LIGHT(5150-10, 5350+10, BW_5G, 0),
	/* channel 100..165 */
	REG_RULE_LIGHT(5470-10, 5850+10, BW_5G, 0),
	}
};
#endif
#endif

struct TX_PWR_LIMIT_SECTION {
	uint8_t ucSectionNum;
	const char *arSectionNames[TX_PWR_LIMIT_SECTION_NUM];
} gTx_Pwr_Limit_Section[] = {
	{5,
	 {"legacy", "ht20", "ht40", "vht20", "offset"}
	},
	{9,
	 {"cck", "ofdm", "ht20", "ht40", "vht20", "vht40",
	  "vht80", "vht160", "txbf_backoff"}
	},
	{15,
	 {"cck", "ofdm", "ht20", "ht40", "vht20", "vht40",
	  "vht80", "vht160", "ru26", "ru52", "ru106", "ru242",
	  "ru484", "ru996", "ru996x2"}
	},
};


const u8 gTx_Pwr_Limit_Element_Num[][TX_PWR_LIMIT_SECTION_NUM] = {
	{7, 6, 7, 7, 5},
	{POWER_LIMIT_SKU_CCK_NUM, POWER_LIMIT_SKU_OFDM_NUM,
	 POWER_LIMIT_SKU_HT20_NUM, POWER_LIMIT_SKU_HT40_NUM,
	 POWER_LIMIT_SKU_VHT20_NUM, POWER_LIMIT_SKU_VHT40_NUM,
	 POWER_LIMIT_SKU_VHT80_NUM, POWER_LIMIT_SKU_VHT160_NUM,
	 POWER_LIMIT_TXBF_BACKOFF_PARAM_NUM},
	{POWER_LIMIT_SKU_CCK_NUM, POWER_LIMIT_SKU_OFDM_NUM,
	 POWER_LIMIT_SKU_HT20_NUM, POWER_LIMIT_SKU_HT40_NUM,
	 POWER_LIMIT_SKU_VHT20_2_NUM, POWER_LIMIT_SKU_VHT40_2_NUM,
	 POWER_LIMIT_SKU_VHT80_2_NUM, POWER_LIMIT_SKU_VHT160_2_NUM,
	 POWER_LIMIT_SKU_RU26_NUM, POWER_LIMIT_SKU_RU52_NUM,
	 POWER_LIMIT_SKU_RU106_NUM, POWER_LIMIT_SKU_RU242_NUM,
	 POWER_LIMIT_SKU_RU484_NUM, POWER_LIMIT_SKU_RU996_NUM,
	 POWER_LIMIT_SKU_RU996X2_NUM},
};

const char *gTx_Pwr_Limit_Element[]
	[TX_PWR_LIMIT_SECTION_NUM]
	[TX_PWR_LIMIT_ELEMENT_NUM] = {
	{
		{"cck1_2", "cck_5_11", "ofdm6_9", "ofdm12_18", "ofdm24_36",
		 "ofdm48", "ofdm54"},
		{"mcs0_8", "mcs1_2_9_10", "mcs3_4_11_12", "mcs5_13", "mcs6_14",
		 "mcs7_15"},
		{"mcs0_8", "mcs1_2_9_10", "mcs3_4_11_12", "mcs5_13", "mcs6_14",
		 "mcs7_15", "mcs32"},
		{"mcs0", "mcs1_2", "mcs3_4", "mcs5_6", "mcs7", "mcs8", "mcs9"},
		{"lg40", "lg80", "vht40", "vht80", "vht160nc"},
	},
	{
		{"c1", "c2", "c5", "c11"},
		{"o6", "o9", "o12", "o18",
		 "o24", "o36", "o48", "o54"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m32"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9"},
		{"2to1"},
	},
	{
		{"c1", "c2", "c5", "c11"},
		{"o6", "o9", "o12", "o18",
		 "o24", "o36", "o48", "o54"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m32"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "rsvd", "rsvd"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "rsvd", "rsvd"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "rsvd", "rsvd"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "rsvd", "rsvd"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11"},
	},
};

static const int8_t gTx_Pwr_Limit_2g_Ch[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
static const int8_t gTx_Pwr_Limit_5g_Ch[] = {
	36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 100, 102,
	104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 132,
	134, 136, 138, 140, 142, 144, 149, 151, 153, 155, 157, 159, 161, 165};
#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
static const int8_t gTx_Pwr_Limit_6g_Ch[] = {
	1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 33, 35,
	37, 39,	41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 65, 67, 69,
	71, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 97, 99, 101, 103,
	105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 125, 129, 131,
	133, 135, 137, 139, 141, 143, 145, 147, 149, 151, 153, 155, 157,
	161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185,
	187, 189, 193, 195, 197, 199, 201, 203, 205, 207, 209, 211, 213,
	215, 217, 219, 221, 225, 227, 229, 233};
#endif

#define TX_PWR_LIMIT_2G_CH_NUM (ARRAY_SIZE(gTx_Pwr_Limit_2g_Ch))
#define TX_PWR_LIMIT_5G_CH_NUM (ARRAY_SIZE(gTx_Pwr_Limit_5g_Ch))
#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
#define TX_PWR_LIMIT_6G_CH_NUM (ARRAY_SIZE(gTx_Pwr_Limit_6g_Ch))
#endif

u_int8_t g_bTxBfBackoffExists = FALSE;

#endif

struct DOMAIN_INFO_ENTRY arSupportedRegDomains[] = {
	{
		(uint16_t *) g_u2CountryGroup0, sizeof(g_u2CountryGroup0) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
			,	/*CH_SET_UNII_MID_100_140 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/*CH_SET_UNII_UPPER_149_165 */
			{125, BAND_NULL, 0, 0, 0, FALSE}
				/* CH_SET_UNII_UPPER_NA */
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup1, sizeof(g_u2CountryGroup1) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup2, sizeof(g_u2CountryGroup2) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,			/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,			/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
			,			/* CH_SET_UNII_WW_100_140 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup3, sizeof(g_u2CountryGroup3) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
			,	/* CH_SET_2G4_1_11 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup4, sizeof(g_u2CountryGroup4) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 5, TRUE}
			,	/* CH_SET_UNII_WW_100_116 */
			{121, BAND_5G, CHNL_SPAN_20, 132, 4, TRUE}
			,	/* CH_SET_UNII_WW_132_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/* CH_SET_UNII_UPPER_149_165 */
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup5, sizeof(g_u2CountryGroup5) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/* CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup6, sizeof(g_u2CountryGroup6) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_LOW_NA */
			{118, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_MID_NA */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/*CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup7, sizeof(g_u2CountryGroup7) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_LOW_NA */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/*CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup8, sizeof(g_u2CountryGroup8) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
			,	/*CH_SET_2G4_1_11 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 5, TRUE}
			,	/*CH_SET_UNII_MID_100_116 */
			{121, BAND_5G, CHNL_SPAN_20, 132, 4, TRUE}
			,	/*CH_SET_UNII_MID_132_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/*CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
				/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup9, sizeof(g_u2CountryGroup9) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_LOW_NA */
			{118, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_MID_NA */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup10, sizeof(g_u2CountryGroup10) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 9, TRUE}
			,	/*CH_SET_UNII_MID_100_132 */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup11, sizeof(g_u2CountryGroup11) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_4 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_4 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup12, sizeof(g_u2CountryGroup12) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_MID_NA */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/*CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup13, sizeof(g_u2CountryGroup13) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{82, BAND_2G4, CHNL_SPAN_5, 14, 1, FALSE}
			,	/*CH_SET_2G4_14_1 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
				/*CH_SET_UNII_MID_100_144 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup14, sizeof(g_u2CountryGroup14) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 8, TRUE}
			,	/*CH_SET_UNII_MID_100_128 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/*CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup15, sizeof(g_u2CountryGroup15) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/*CH_SET_UNII_MID_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/*CH_SET_UNII_UPPER_149_165 */
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup16, sizeof(g_u2CountryGroup16) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/*CH_SET_UNII_MID_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
			,	/*CH_SET_UNII_UPPER_149_161 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup17, sizeof(g_u2CountryGroup17) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 5, TRUE}
			,	/*CH_SET_UNII_MID_100_116 */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup18, sizeof(g_u2CountryGroup18) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
			,	/*CH_SET_UNII_MID_100_140 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/*CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup19, sizeof(g_u2CountryGroup19) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
			,	/* 6G_CH_1_93 */
#endif
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup20, sizeof(g_u2CountryGroup20) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
			,	/* CH_SET_2G4_1_11 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 8, FALSE}
				/* CH_SET_UNII_UPPER_149_177 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
				/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup21, sizeof(g_u2CountryGroup21) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
			,	/* CH_SET_UNII_UPPER_149_161 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup22, sizeof(g_u2CountryGroup22) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
				/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup23, sizeof(g_u2CountryGroup23) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
			,	/* CH_SET_2G4_1_11 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 8, FALSE}
			,	/* CH_SET_UNII_UPPER_149_177 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup24, sizeof(g_u2CountryGroup24) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 5, TRUE}
			,	/* CH_SET_UNII_WW_100_116 */
			{121, BAND_5G, CHNL_SPAN_20, 132, 4, TRUE}
			,	/* CH_SET_UNII_WW_132_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup25, sizeof(g_u2CountryGroup25) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/*CH_SET_UNII_MID_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
				/*CH_SET_UNII_UPPER_149_161 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
				/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup26, sizeof(g_u2CountryGroup26) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 11, FALSE}
			,	/* CH_SET_2G4_1_11 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 8, FALSE}
				/* CH_SET_UNII_UPPER_149_177 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
				/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup27, sizeof(g_u2CountryGroup27) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup28, sizeof(g_u2CountryGroup28) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
			,	/* CH_SET_UNII_UPPER_149_161 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup29, sizeof(g_u2CountryGroup29) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/*CH_SET_UNII_WW_NA */
			/* lc.zhangzichao1 Modify KZ region add channel 165 jira id HTH-494370 begin */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, TRUE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			/* lc.zhangzichao1 Modify KZ region add channel 165 jira id HTH-494370 end */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	/* lc.zhangzichao1 Modify PK region channel rules jira id HTH-494370 begin */
	{
		(uint16_t *) g_u2CountryGroup30, sizeof(g_u2CountryGroup30) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	/* lc.zhangzichao1 Modify PK region channel rules jira id HTH-494370 end */
	{
		/* Note: Default group if no matched country code */
		NULL, 0,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,			/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,			/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,			/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 8, FALSE}
						/* CH_SET_UNII_UPPER_149_177 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
						/* 6G_CH_1_233 */
#endif
		}
	}
};

static const uint16_t g_u2CountryGroup0_Passive[] = {
	COUNTRY_CODE_TW
};

struct DOMAIN_INFO_ENTRY arSupportedRegDomains_Passive[] = {
	{
		(uint16_t *) g_u2CountryGroup0_Passive,
		sizeof(g_u2CountryGroup0_Passive) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 0, FALSE}
			,			/* CH_SET_2G4_1_14_NA */
			{82, BAND_2G4, CHNL_SPAN_5, 14, 0, FALSE}
			,

			{115, BAND_5G, CHNL_SPAN_20, 36, 0, FALSE}
			,			/* CH_SET_UNII_LOW_NA */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
			,			/* CH_SET_UNII_WW_100_140 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 0, FALSE}
						/* CH_SET_UNII_UPPER_NA */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 0, FALSE}
						/* 6G_CH_1_233 */
#endif
		}
	}
	,
	{
		/* Default passive scan channel table: ch52~64, ch100~144 */
		NULL,
		0,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 0, FALSE}
			,			/* CH_SET_2G4_1_14_NA */
			{82, BAND_2G4, CHNL_SPAN_5, 14, 0, FALSE}
			,

			{115, BAND_5G, CHNL_SPAN_20, 36, 0, FALSE}
			,			/* CH_SET_UNII_LOW_NA */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,			/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 0, FALSE}
						/* CH_SET_UNII_UPPER_NA */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 0, FALSE}
						/* 6G_CH_1_233 */
#endif
		}
	}
};

#if (CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1)
struct SUBBAND_CHANNEL g_rRlmSubBand[] = {

	{BAND_2G4, BAND_2G4_LOWER_BOUND, BAND_2G4_UPPER_BOUND, 1, 0} /* 2.4G */
	,
	{BAND_5G, UNII1_LOWER_BOUND, UNII1_UPPER_BOUND, 2, 0} /* 5G 36~50 */
	,
	{BAND_5G, UNII2A_LOWER_BOUND, UNII2A_UPPER_BOUND, 2, 0} /* 5G 52~64 */
	,
	{BAND_5G, UNII2C_LOWER_BOUND, UNII2C_UPPER_BOUND, 2, 0} /* 5G 100~144 */
	,
	{BAND_5G, UNII3_LOWER_BOUND, UNII3_UPPER_BOUND, 2, 0} /* 5G 149~165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
	,
	{BAND_6G, UNII5A_LOWER_BOUND, UNII5A_UPPER_BOUND, 2, 0} /* 6G 1~47 */
	,
	{BAND_6G, UNII5B_LOWER_BOUND, UNII5B_UPPER_BOUND, 2, 0} /* 6G 49~93 */
	,
	{BAND_6G, UNII6_LOWER_BOUND, UNII6_UPPER_BOUND, 2, 0} /* 6G 95~115 */
	,
	{BAND_6G, UNII7A_LOWER_BOUND, UNII7A_UPPER_BOUND, 2, 0} /* 6G 117~151 */
	,
	{BAND_6G, UNII7B_LOWER_BOUND, UNII7B_UPPER_BOUND, 2, 0} /* 6G 153~185 */
	,
	{BAND_6G, UNII8_LOWER_BOUND, UNII8_UPPER_BOUND, 2, 0} /* 6G 187~233 */
#endif /* CFG_SUPPORT_WIFI_6G */
};
#endif
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in/out]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
struct DOMAIN_INFO_ENTRY *rlmDomainGetDomainInfo(struct ADAPTER *prAdapter)
{
#define REG_DOMAIN_GROUP_NUM  \
	(sizeof(arSupportedRegDomains) / sizeof(struct DOMAIN_INFO_ENTRY))
#define REG_DOMAIN_DEF_IDX	(REG_DOMAIN_GROUP_NUM - 1)

	struct DOMAIN_INFO_ENTRY *prDomainInfo = NULL;
	struct REG_INFO *prRegInfo;
	uint16_t u2TargetCountryCode;
	uint16_t i, j;

	ASSERT(prAdapter);

	if (prAdapter->prDomainInfo)
		return prAdapter->prDomainInfo;

	prRegInfo = &prAdapter->prGlueInfo->rRegInfo;

	DBGLOG(RLM, TRACE, "eRegChannelListMap=%d, u2CountryCode=0x%04x\n",
			   prRegInfo->eRegChannelListMap,
			   prAdapter->rWifiVar.u2CountryCode);

	/*
	 * Domain info can be specified by given idx of arSupportedRegDomains
	 * table, customized, or searched by country code,
	 * only one is set among these three methods in NVRAM.
	 */
	if (prRegInfo->eRegChannelListMap == REG_CH_MAP_TBL_IDX &&
	    prRegInfo->ucRegChannelListIndex < REG_DOMAIN_GROUP_NUM) {
		/* by given table idx */
		DBGLOG(RLM, TRACE, "ucRegChannelListIndex=%d\n",
		       prRegInfo->ucRegChannelListIndex);
		prDomainInfo = &arSupportedRegDomains
					[prRegInfo->ucRegChannelListIndex];
	} else if (prRegInfo->eRegChannelListMap == REG_CH_MAP_CUSTOMIZED) {
		/* by customized */
		prDomainInfo = &prRegInfo->rDomainInfo;
	} else {
		/* by country code */
		u2TargetCountryCode =
				prAdapter->rWifiVar.u2CountryCode;

		for (i = 0; i < REG_DOMAIN_GROUP_NUM; i++) {
			prDomainInfo = &arSupportedRegDomains[i];

			if ((prDomainInfo->u4CountryNum &&
			     prDomainInfo->pu2CountryGroup) ||
			    prDomainInfo->u4CountryNum == 0) {
				for (j = 0;
				     j < prDomainInfo->u4CountryNum;
				     j++) {
					if (prDomainInfo->pu2CountryGroup[j] ==
							u2TargetCountryCode)
						break;
				}
				if (j < prDomainInfo->u4CountryNum)
					break;	/* Found */
			}
		}

		/* If no matched country code,
		 * use the default regulatory domain
		 */
		if (i >= REG_DOMAIN_GROUP_NUM) {
			DBGLOG(RLM, INFO,
			       "No matched country code, use the default regulatory domain\n");
			prDomainInfo = &arSupportedRegDomains
							[REG_DOMAIN_DEF_IDX];
		}
	}

	prAdapter->prDomainInfo = prDomainInfo;
	return prDomainInfo;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Retrieve the supported channel list of specified band
 *
 * \param[in/out] eSpecificBand:   BAND_2G4, BAND_5G or BAND_NULL
 *                                 (both 2.4G and 5G)
 *                fgNoDfs:         whether to exculde DFS channels
 *                ucMaxChannelNum: max array size
 *                pucNumOfChannel: pointer to returned channel number
 *                paucChannelList: pointer to returned channel list array
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
rlmDomainGetChnlList_V2(struct ADAPTER *prAdapter,
			enum ENUM_BAND eSpecificBand, u_int8_t fgNoDfs,
			uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
			struct RF_CHANNEL_INFO *paucChannelList)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	enum ENUM_BAND band;
	uint8_t max_count, i, ucNum;
	struct CMD_DOMAIN_CHANNEL *prCh;

	if (eSpecificBand == BAND_2G4) {
		i = 0;
		max_count = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	} else if (eSpecificBand == BAND_5G) {
		i = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
		max_count = rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eSpecificBand == BAND_6G) {
		i = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
		max_count = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ)
			+rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ);
	}
#endif
	else {
		i = 0;
		max_count =
#if (CFG_SUPPORT_WIFI_6G == 1)
			rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ) +
#endif
			rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	}

	ucNum = 0;
	for (; i < max_count; i++) {
		prCh = rlmDomainGetActiveChannels() + i;
		if (fgNoDfs && (prCh->eFlags & IEEE80211_CHAN_RADAR))
			continue; /*not match*/

		if (i < rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ))
			band = BAND_2G4;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (i < rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ) +
			rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ))
			band = BAND_5G;
		else
			band = BAND_6G;
#else
		else
			band = BAND_5G;
#endif

		paucChannelList[ucNum].eBand = band;
		paucChannelList[ucNum].ucChannelNum = prCh->u2ChNum;

		ucNum++;
		if (ucMaxChannelNum == ucNum)
			break;
	}

	*pucNumOfChannel = ucNum;
#else
	*pucNumOfChannel = 0;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Retrieve the supported channel list of specified band
 *
 * \param[in/out] eSpecificBand:   BAND_2G4, BAND_5G or BAND_NULL
 *                                 (both 2.4G and 5G)
 *                fgNoDfs:         whether to exculde DFS channels
 *                ucMaxChannelNum: max array size
 *                pucNumOfChannel: pointer to returned channel number
 *                paucChannelList: pointer to returned channel list array
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void
rlmDomainGetChnlList(struct ADAPTER *prAdapter,
		     enum ENUM_BAND eSpecificBand, u_int8_t fgNoDfs,
		     uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
		     struct RF_CHANNEL_INFO *paucChannelList)
{
	uint8_t i, j, ucNum, ch;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	ASSERT(prAdapter);
	ASSERT(paucChannelList);
	ASSERT(pucNumOfChannel);

	if (regd_is_single_sku_en()) {
		rlmDomainGetChnlList_V2(prAdapter, eSpecificBand,
					       fgNoDfs, ucMaxChannelNum,
					       pucNumOfChannel,
					       paucChannelList);
		return;
	}

	/* If no matched country code, the final one will be used */
	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	ucNum = 0;
	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_NULL ||
		    prSubband->ucBand >= BAND_NUM ||
		    (prSubband->ucBand == BAND_5G &&
		     !prAdapter->fgEnable5GBand))
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prSubband->ucBand == BAND_6G && !prAdapter->fgIsHwSupport6G)
			continue;
#endif

		/* repoert to upper layer only non-DFS channel
		 * for ap mode usage
		 */
		if (fgNoDfs == TRUE && prSubband->fgDfs == TRUE)
			continue;

		if (eSpecificBand == BAND_NULL ||
		    prSubband->ucBand == eSpecificBand) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {
				if (ucNum >= ucMaxChannelNum)
					break;

				ch = prSubband->ucFirstChannelNum +
				     j * prSubband->ucChannelSpan;

				if (!kalIsValidChnl(prAdapter->prGlueInfo, ch,
						prSubband->ucBand)) {
					DBGLOG(RLM, INFO,
						   "Not support ch%d!\n", ch);
					continue;
				}

				paucChannelList[ucNum].eBand =
							prSubband->ucBand;
				paucChannelList[ucNum].ucChannelNum = ch;
				paucChannelList[ucNum].fgDFS = prSubband->fgDfs;
				ucNum++;
			}
		}
	}

	*pucNumOfChannel = ucNum;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Retrieve the supported channel list of specified ucRegClass
 *
 * \param[in/out] ucOpClass:            specified ucRegClass
 *                pucChannelListNum:    pointer to returned channel number
 *                paucChannelList:      pointer to returned channel list array
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmDomainGetChnlListFromOpClass(struct ADAPTER *prAdapter,
	uint8_t ucOpClass, struct RF_CHANNEL_INFO *paucChannelList,
	uint8_t *pucChannelListNum)
{
	uint8_t i, j, ucNum = 0, ucCh;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_NULL ||
		    prSubband->ucBand >= BAND_NUM ||
		    (prSubband->ucBand == BAND_5G &&
		     !prAdapter->fgEnable5GBand))
			continue;

		if (ucOpClass == prSubband->ucRegClass) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {

				ucCh = prSubband->ucFirstChannelNum +
				     j * prSubband->ucChannelSpan;
				if (!kalIsValidChnl(prAdapter->prGlueInfo,
						ucCh, prSubband->ucBand)) {
					DBGLOG(RLM, INFO,
					       "Not support ch%d!\n", ucCh);
					continue;
				}
				paucChannelList[ucNum].eBand =
							prSubband->ucBand;
				paucChannelList[ucNum].ucChannelNum = ucCh;
				paucChannelList[ucNum].fgDFS = prSubband->fgDfs;
				ucNum++;
			}
		}
	}

	*pucChannelListNum = ucNum;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Retrieve DFS channels from 5G band
 *
 * \param[in/out] ucMaxChannelNum: max array size
 *                pucNumOfChannel: pointer to returned channel number
 *                paucChannelList: pointer to returned channel list array
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/
void rlmDomainGetDfsChnls(struct ADAPTER *prAdapter,
			  uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
			  struct RF_CHANNEL_INFO *paucChannelList)
{
	uint8_t i, j, ucNum, ch;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	ASSERT(prAdapter);
	ASSERT(paucChannelList);
	ASSERT(pucNumOfChannel);

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	ucNum = 0;
	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G) {
			if (!prAdapter->fgEnable5GBand)
				continue;

			if (prSubband->fgDfs == TRUE) {
				for (j = 0; j < prSubband->ucNumChannels; j++) {
					if (ucNum >= ucMaxChannelNum)
						break;

					ch = prSubband->ucFirstChannelNum +
					     j * prSubband->ucChannelSpan;

					if (!kalIsValidChnl(
							prAdapter->prGlueInfo,
							ch,
							prSubband->ucBand)) {
						DBGLOG(RLM, INFO,
						   "Not support ch%d!\n", ch);
						continue;
					}

					paucChannelList[ucNum].eBand =
					    prSubband->ucBand;
					paucChannelList[ucNum].ucChannelNum =
					    ch;
					ucNum++;
				}
			}
		}
	}

	*pucNumOfChannel = ucNum;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check whether input is dfs channel
 *
 * \param[in/out] ucChannel: channel to check
 *
 * \return true if dfs channel
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmDomainIsDfsChnls(struct ADAPTER *prAdapter, uint8_t ucChannel)
{
	uint8_t ucNumOfChannel = 0;
	struct RF_CHANNEL_INFO aucChannelList[64] = {0};
	uint8_t ucCount = 0;

	rlmDomainGetDfsChnls(prAdapter, 64, &ucNumOfChannel, aucChannelList);
	for (; ucCount < ucNumOfChannel; ucCount++) {
		if (ucChannel == aucChannelList[ucCount].ucChannelNum)
			return TRUE;
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainSendCmd(struct ADAPTER *prAdapter, bool fgPwrLmtSend)
{

	if (!regd_is_single_sku_en())
		rlmDomainSendPassiveScanInfoCmd(prAdapter);
	rlmDomainSendDomainInfoCmd(prAdapter);
#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	if (fgPwrLmtSend)
		rlmDomainSendPwrLimitCmd(prAdapter);
#endif
}

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
static bool isEUCountry(struct ADAPTER *prAdapter, uint32_t u4CountryCode)
{
	uint16_t i;
	uint16_t u2TargetCountryCode = 0;

	u2TargetCountryCode =
	    ((u4CountryCode & 0xff) << 8) | ((u4CountryCode & 0xff00) >> 8);

	DBGLOG(RLM, INFO, " Target country code='%c%c'(0x%4x)\n",
	    ((u4CountryCode & 0xff) - 'A'),
		(((u4CountryCode & 0xff00) >> 8) - 'A'),
		u2TargetCountryCode);

	for (i = 0; i < (sizeof(g_u2CountryGroup4) / sizeof(uint16_t)); i++) {
		if (g_u2CountryGroup4[i] == u2TargetCountryCode)
			return TRUE;
	}

	return FALSE;
}

static void rlmSetEd_EU(struct ADAPTER *prAdapter, uint32_t u4CountryCode)
{
	struct WIFI_VAR *prWifiVar = &prAdapter->rWifiVar;

	if (isEUCountry(prAdapter, u4CountryCode)) {
		if ((prWifiVar->i4Ed2GEU != 0) && (prWifiVar->i4Ed5GEU != 0)) {
			wlanSetEd(prAdapter, prWifiVar->i4Ed2GEU,
			    prWifiVar->i4Ed5GEU, 1);
			DBGLOG(RLM, INFO, "Set Ed for EU: 2G=%d, 5G=%d\n",
				prWifiVar->i4Ed2GEU, prWifiVar->i4Ed5GEU);
		}
	} else {
		if ((prWifiVar->i4Ed2GNonEU != 0) &&
			(prWifiVar->i4Ed5GNonEU != 0)) {
			wlanSetEd(prAdapter, prWifiVar->i4Ed2GNonEU,
				prWifiVar->i4Ed5GNonEU, 1);
			DBGLOG(RLM, INFO,
				"Set Ed for non EU: 2G=%d, 5G=%d\n",
				prWifiVar->i4Ed2GNonEU, prWifiVar->i4Ed5GNonEU);
		}
	}
}
#endif

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainSendDomainInfoCmd_V2(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	u8 max_channel_count = 0;
	u32 buff_max_size, buff_valid_size;
	struct CMD_SET_DOMAIN_INFO_V2 *prCmd;
	struct CMD_DOMAIN_ACTIVE_CHANNEL_LIST *prChs;

	max_channel_count = kalGetChannelCount(prAdapter->prGlueInfo);

	if (max_channel_count == 0) {
		DBGLOG(RLM, ERROR, "%s, invalid channel count.\n", __func__);
		return;
	}


	buff_max_size = sizeof(struct CMD_SET_DOMAIN_INFO_V2) +
		max_channel_count * sizeof(struct CMD_DOMAIN_CHANNEL);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, buff_max_size);
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Alloc cmd buffer failed\n");
		return;
	}
	prChs = &(prCmd->arActiveChannels);


	/*
	 * Fill in the active channels
	 */
	rlmExtractChannelInfo(max_channel_count, prChs);

	prCmd->u4CountryCode = rlmDomainGetCountryCode();
	prCmd->uc2G4Bandwidth = prAdapter->rWifiVar.uc2G4BandwidthMode;
	prCmd->uc5GBandwidth = prAdapter->rWifiVar.uc5GBandwidthMode;
	prCmd->uc6GBandwidth = prAdapter->rWifiVar.uc6GBandwidthMode;
	prCmd->aucPadding[0] = 0;

	buff_valid_size = sizeof(struct CMD_SET_DOMAIN_INFO_V2) +
		(prChs->u1ActiveChNum2g + prChs->u1ActiveChNum5g +
		prChs->u1ActiveChNum6g) *
		sizeof(struct CMD_DOMAIN_CHANNEL);

	DBGLOG(RLM, INFO,
	       "rlmDomainSendDomainInfoCmd_V2(), buff_valid_size = 0x%x\n",
	       buff_valid_size);

	/* Set domain info to chip */
	wlanSendSetQueryCmd(prAdapter, /* prAdapter */
			    CMD_ID_SET_DOMAIN_INFO, /* ucCID */
			    TRUE,  /* fgSetQuery */
			    FALSE, /* fgNeedResp */
			    FALSE, /* fgIsOid */
			    NULL,  /* pfCmdDoneHandler */
			    NULL,  /* pfCmdTimeoutHandler */
			    buff_valid_size,
			    (uint8_t *) prCmd, /* pucInfoBuffer */
			    NULL,  /* pvSetQueryBuffer */
			    0      /* u4SetQueryBufferLen */
	    );

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	rlmSetEd_EU(prAdapter, prCmd->u4CountryCode);
#endif

	cnmMemFree(prAdapter, prCmd);
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainSendDomainInfoCmd(struct ADAPTER *prAdapter)
{
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	struct CMD_SET_DOMAIN_INFO *prCmd;
	struct DOMAIN_SUBBAND_INFO *prSubBand;
	uint8_t i;

	if (regd_is_single_sku_en()) {
		rlmDomainSendDomainInfoCmd_V2(prAdapter);
		return;
	}


	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
			    sizeof(struct CMD_SET_DOMAIN_INFO));
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Alloc cmd buffer failed\n");
		return;
	}
	kalMemZero(prCmd, sizeof(struct CMD_SET_DOMAIN_INFO));

	prCmd->u2CountryCode =
		prAdapter->rWifiVar.u2CountryCode;
	prCmd->u2IsSetPassiveScan = 0;
	prCmd->uc2G4Bandwidth =
		prAdapter->rWifiVar.uc2G4BandwidthMode;
	prCmd->uc5GBandwidth =
		prAdapter->rWifiVar.uc5GBandwidthMode;
	prCmd->aucReserved[0] = 0;
	prCmd->aucReserved[1] = 0;

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];

		prCmd->rSubBand[i].ucRegClass = prSubBand->ucRegClass;
		prCmd->rSubBand[i].ucBand = prSubBand->ucBand;

		if (prSubBand->ucBand != BAND_NULL && prSubBand->ucBand
								< BAND_NUM) {
			prCmd->rSubBand[i].ucChannelSpan
						= prSubBand->ucChannelSpan;
			prCmd->rSubBand[i].ucFirstChannelNum
						= prSubBand->ucFirstChannelNum;
			prCmd->rSubBand[i].ucNumChannels
						= prSubBand->ucNumChannels;
		}
	}

	/* Set domain info to chip */
	wlanSendSetQueryCmd(prAdapter, /* prAdapter */
		CMD_ID_SET_DOMAIN_INFO, /* ucCID */
		TRUE,  /* fgSetQuery */
		FALSE, /* fgNeedResp */
		FALSE, /* fgIsOid */
		NULL,  /* pfCmdDoneHandler */
		NULL,  /* pfCmdTimeoutHandler */
		sizeof(struct CMD_SET_DOMAIN_INFO), /* u4SetQueryInfoLen */
		(uint8_t *) prCmd, /* pucInfoBuffer */
		NULL,  /* pvSetQueryBuffer */
		0      /* u4SetQueryBufferLen */
	    );

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	rlmSetEd_EU(prAdapter, prCmd->u2CountryCode);
#endif

	cnmMemFree(prAdapter, prCmd);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainSendPassiveScanInfoCmd(struct ADAPTER *prAdapter)
{
#define REG_DOMAIN_PASSIVE_DEF_IDX	1
#define REG_DOMAIN_PASSIVE_GROUP_NUM \
	(sizeof(arSupportedRegDomains_Passive)	\
	 / sizeof(struct DOMAIN_INFO_ENTRY))

	struct DOMAIN_INFO_ENTRY *prDomainInfo = NULL;
	struct CMD_SET_DOMAIN_INFO *prCmd;
	struct DOMAIN_SUBBAND_INFO *prSubBand;
	uint16_t u2TargetCountryCode;
	uint8_t i, j;

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
			    sizeof(struct CMD_SET_DOMAIN_INFO));
	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Alloc cmd buffer failed\n");
		return;
	}
	kalMemZero(prCmd, sizeof(struct CMD_SET_DOMAIN_INFO));

	prCmd->u2CountryCode = prAdapter->rWifiVar.u2CountryCode;
	prCmd->u2IsSetPassiveScan = 1;
	prCmd->uc2G4Bandwidth =
		prAdapter->rWifiVar.uc2G4BandwidthMode;
	prCmd->uc5GBandwidth =
		prAdapter->rWifiVar.uc5GBandwidthMode;
	prCmd->aucReserved[0] = 0;
	prCmd->aucReserved[1] = 0;

	DBGLOG(RLM, TRACE, "u2CountryCode=0x%04x\n",
	       prAdapter->rWifiVar.u2CountryCode);

	u2TargetCountryCode = prAdapter->rWifiVar.u2CountryCode;

	for (i = 0; i < REG_DOMAIN_PASSIVE_GROUP_NUM; i++) {
		prDomainInfo = &arSupportedRegDomains_Passive[i];

		for (j = 0; j < prDomainInfo->u4CountryNum; j++) {
			if (prDomainInfo->pu2CountryGroup[j] ==
						u2TargetCountryCode)
				break;
		}
		if (j < prDomainInfo->u4CountryNum)
			break;	/* Found */
	}

	if (i >= REG_DOMAIN_PASSIVE_GROUP_NUM)
		prDomainInfo = &arSupportedRegDomains_Passive
					[REG_DOMAIN_PASSIVE_DEF_IDX];

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];

		prCmd->rSubBand[i].ucRegClass = prSubBand->ucRegClass;
		prCmd->rSubBand[i].ucBand = prSubBand->ucBand;

		if (prSubBand->ucBand != BAND_NULL && prSubBand->ucBand
		    < BAND_NUM) {
			prCmd->rSubBand[i].ucChannelSpan =
						prSubBand->ucChannelSpan;
			prCmd->rSubBand[i].ucFirstChannelNum =
						prSubBand->ucFirstChannelNum;
			prCmd->rSubBand[i].ucNumChannels =
						prSubBand->ucNumChannels;
		}
	}

	/* Set passive scan channel info to chip */
	wlanSendSetQueryCmd(prAdapter, /* prAdapter */
		CMD_ID_SET_DOMAIN_INFO, /* ucCID */
		TRUE,  /* fgSetQuery */
		FALSE, /* fgNeedResp */
		FALSE, /* fgIsOid */
		NULL,  /* pfCmdDoneHandler */
		NULL,  /* pfCmdTimeoutHandler */
		sizeof(struct CMD_SET_DOMAIN_INFO), /* u4SetQueryInfoLen */
		(uint8_t *) prCmd, /* pucInfoBuffer */
		NULL,  /* pvSetQueryBuffer */
		0      /* u4SetQueryBufferLen */
	    );

#if (CFG_SUPPORT_DYNAMIC_EDCCA == 1)
	rlmSetEd_EU(prAdapter, prCmd->u2CountryCode);
#endif

	cnmMemFree(prAdapter, prCmd);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in/out]
 *
 * \return TRUE  Legal channel
 *         FALSE Illegal channel for current regulatory domain
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmDomainIsLegalChannel_V2(struct ADAPTER *prAdapter,
				    enum ENUM_BAND eBand,
				    uint8_t ucChannel)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t idx, start_idx, end_idx;
	struct CMD_DOMAIN_CHANNEL *prCh;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		start_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ) +
			rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ);
	} else
#endif
	if (eBand == BAND_2G4) {
		start_idx = 0;
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	} else {
		start_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
		end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ) +
				rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
	}

	for (idx = start_idx; idx < end_idx; idx++) {
		prCh = rlmDomainGetActiveChannels() + idx;

		if (prCh->u2ChNum == ucChannel)
			return TRUE;
	}

	return FALSE;
#else
	return FALSE;
#endif
}

u_int8_t rlmDomainIsLegalChannel(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand, uint8_t ucChannel)
{
	uint8_t i, j;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	if (regd_is_single_sku_en())
		return rlmDomainIsLegalChannel_V2(prAdapter, eBand, ucChannel);

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G && !prAdapter->fgEnable5GBand)
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prSubband->ucBand == BAND_6G && !prAdapter->fgIsHwSupport6G)
			continue;
#endif

		if (prSubband->ucBand == eBand) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {
				if ((prSubband->ucFirstChannelNum + j *
				    prSubband->ucChannelSpan) == ucChannel) {

					if (!kalIsValidChnl(
							prAdapter->prGlueInfo,
							ucChannel,
							prSubband->ucBand)) {
						DBGLOG(RLM, INFO,
							   "Not support ch%d!\n",
							   ucChannel);
						return FALSE;
					} else
						return TRUE;
				}
			}
		}
	}

	return FALSE;
}

u_int8_t rlmDomainIsLegalDfsChannel(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucChannel)
{
	uint8_t i, j;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G
			&& !prAdapter->fgEnable5GBand)
			continue;

		if (prSubband->ucBand == eBand
			&& prSubband->fgDfs == TRUE) {
			for (j = 0; j < prSubband->ucNumChannels; j++) {
				if ((prSubband->ucFirstChannelNum + j *
					prSubband->ucChannelSpan)
					== ucChannel) {
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in/out]
 *
 * \return none
 */
/*----------------------------------------------------------------------------*/

uint32_t rlmDomainSupOperatingClassIeFill(uint8_t *pBuf)
{
	/*
	 *  The Country element should only be included for
	 *  Status Code 0 (Successful).
	 */
	uint32_t u4IeLen;
	uint8_t aucClass[12] = { 0x01, 0x02, 0x03, 0x05, 0x16, 0x17, 0x19, 0x1b,
		0x1c, 0x1e, 0x20, 0x21
	};

	/*
	 * The Supported Operating Classes element is used by a STA to
	 * advertise the operating classes that it is capable of operating
	 * with in this country.
	 * The Country element (see 8.4.2.10) allows a STA to configure its
	 * PHY and MAC for operation when the operating triplet of Operating
	 * Extension Identifier, Operating Class, and Coverage Class fields
	 * is present.
	 */
	SUP_OPERATING_CLASS_IE(pBuf)->ucId = ELEM_ID_SUP_OPERATING_CLASS;
	SUP_OPERATING_CLASS_IE(pBuf)->ucLength = 1 + sizeof(aucClass);
	SUP_OPERATING_CLASS_IE(pBuf)->ucCur = 0x0c;	/* 0x51 */
	kalMemCopy(SUP_OPERATING_CLASS_IE(pBuf)->ucSup, aucClass,
		   sizeof(aucClass));
	u4IeLen = (SUP_OPERATING_CLASS_IE(pBuf)->ucLength + 2);
#if CFG_SUPPORT_802_11D
	pBuf += u4IeLen;

	COUNTRY_IE(pBuf)->ucId = ELEM_ID_COUNTRY_INFO;
	COUNTRY_IE(pBuf)->ucLength = 6;
	COUNTRY_IE(pBuf)->aucCountryStr[0] = 0x55;
	COUNTRY_IE(pBuf)->aucCountryStr[1] = 0x53;
	COUNTRY_IE(pBuf)->aucCountryStr[2] = 0x20;
	COUNTRY_IE(pBuf)->arCountryStr[0].ucFirstChnlNum = 1;
	COUNTRY_IE(pBuf)->arCountryStr[0].ucNumOfChnl = 11;
	COUNTRY_IE(pBuf)->arCountryStr[0].cMaxTxPwrLv = 0x1e;
	u4IeLen += (COUNTRY_IE(pBuf)->ucLength + 2);
#endif
	return u4IeLen;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (fgValid) : 0 -> inValid, 1 -> Valid
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmDomainCheckChannelEntryValid(struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand, uint8_t ucCentralCh)
{
	u_int8_t fgValid = FALSE;
	uint8_t ucTemp = 0xff;
	uint8_t i;
	/*Check Power limit table channel efficient or not */

	/* CH50 is not located in any FCC subbands
	 * but it's a valid central channel for 160C
	 */
	if (eBand == BAND_5G && ucCentralCh == 50) {
		fgValid = TRUE;
		return fgValid;
	}

	for (i = PWR_LMT_SUBBAND_2G4; i < PWR_LMT_SUBAND_NUM; i++) {
		if ((eBand == BAND_NULL || eBand == g_rRlmSubBand[i].eBand) &&
			(ucCentralCh >= g_rRlmSubBand[i].ucStartCh) &&
			(ucCentralCh <= g_rRlmSubBand[i].ucEndCh))
			ucTemp = (ucCentralCh - g_rRlmSubBand[i].ucStartCh) %
				 g_rRlmSubBand[i].ucInterval;
		if (ucTemp == 0)
			break;
	}

	if (ucTemp == 0)
		fgValid = TRUE;
	return fgValid;

}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomainGetCenterChannel(enum ENUM_BAND eBand, uint8_t ucPriChannel,
				  enum ENUM_CHNL_EXT eExtend)
{
	uint8_t ucCenterChannel;

	if (eExtend == CHNL_EXT_SCA)
		ucCenterChannel = ucPriChannel + 2;
	else if (eExtend == CHNL_EXT_SCB)
		ucCenterChannel = ucPriChannel - 2;
	else
		ucCenterChannel = ucPriChannel;

	return ucCenterChannel;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief
 *
 * \param[in]
 *
 * \return
 */
/*----------------------------------------------------------------------------*/
u_int8_t
rlmDomainIsValidRfSetting(struct ADAPTER *prAdapter,
			  enum ENUM_BAND eBand,
			  uint8_t ucPriChannel,
			  enum ENUM_CHNL_EXT eExtend,
			  enum ENUM_CHANNEL_WIDTH eChannelWidth,
			  uint8_t ucChannelS1, uint8_t ucChannelS2)
{
	uint8_t ucCenterCh = 0;
	uint8_t  ucUpperChannel;
	uint8_t  ucLowerChannel;
	u_int8_t fgValidChannel = TRUE;
	u_int8_t fgUpperChannel = TRUE;
	u_int8_t fgLowerChannel = TRUE;
	u_int8_t fgValidBW = TRUE;
	u_int8_t fgValidRfSetting = TRUE;
	uint32_t u4PrimaryOffset;

	/*DBG msg for Channel InValid */
	if (eChannelWidth == CW_20_40MHZ) {
		ucCenterCh = rlmDomainGetCenterChannel(eBand, ucPriChannel,
						       eExtend);

		/* Check Central Channel Valid or Not */
		fgValidChannel = rlmDomainCheckChannelEntryValid(prAdapter,
			eBand, ucCenterCh);
		if (fgValidChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf20: CentralCh=%d\n", ucCenterCh);

		/* Check Upper Channel and Lower Channel */
		switch (eExtend) {
		case CHNL_EXT_SCA:
			ucUpperChannel = ucPriChannel + 4;
			ucLowerChannel = ucPriChannel;
			break;
		case CHNL_EXT_SCB:
			ucUpperChannel = ucPriChannel;
			ucLowerChannel = ucPriChannel - 4;
			break;
		default:
			ucUpperChannel = ucPriChannel;
			ucLowerChannel = ucPriChannel;
			break;
		}

		fgUpperChannel = rlmDomainCheckChannelEntryValid(prAdapter,
			eBand, ucUpperChannel);
		if (fgUpperChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf20: UpperCh=%d\n", ucUpperChannel);

		fgLowerChannel = rlmDomainCheckChannelEntryValid(prAdapter,
			eBand, ucLowerChannel);
		if (fgLowerChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf20: LowerCh=%d\n", ucLowerChannel);

		/* Check S1, S2 */
		if (ucChannelS2 != 0) {
			fgValidChannel = FALSE;
			DBGLOG(RLM, WARN, "Rf20: S1=%d, S2=%d\n",
				ucChannelS1, ucChannelS2);
		}
	} else if ((eChannelWidth == CW_80MHZ) ||
		   (eChannelWidth == CW_160MHZ)) {
		ucCenterCh = ucChannelS1;

		/* Check Central Channel Valid or Not */
		if (eChannelWidth != CW_160MHZ) {
			/* BW not check , ex: primary 36 and
			 * central channel 50 will fail the check
			 */
			fgValidChannel =
				rlmDomainCheckChannelEntryValid(prAdapter,
					eBand, ucCenterCh);
		}

		if (fgValidChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf80/160C: CentralCh=%d\n",
			       ucCenterCh);
	} else if (eChannelWidth == CW_80P80MHZ) {
		ucCenterCh = ucChannelS1;

		fgValidChannel = rlmDomainCheckChannelEntryValid(prAdapter,
			eBand, ucCenterCh);

		if (fgValidChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf160NC: CentralCh1=%d\n",
			       ucCenterCh);

		ucCenterCh = ucChannelS2;

		fgValidChannel = rlmDomainCheckChannelEntryValid(prAdapter,
			eBand, ucCenterCh);

		if (fgValidChannel == FALSE)
			DBGLOG(RLM, WARN, "Rf160NC: CentralCh2=%d\n",
			       ucCenterCh);

		/* Check Central Channel Valid or Not */
	} else if (eChannelWidth == CW_320_1MHZ ||
		   eChannelWidth == CW_320_2MHZ) {
		//TODO: add checking for 320MHZ
		DBGLOG(RLM, TRACE, "CW320 %d\n", eChannelWidth);
	} else {
		DBGLOG(RLM, ERROR, "Wrong BW =%d\n", eChannelWidth);
		fgValidChannel = FALSE;
	}

	/* Check BW Setting Correct or Not */
	if (eBand == BAND_2G4) {
		if (eChannelWidth != CW_20_40MHZ) {
			fgValidBW = FALSE;
			DBGLOG(RLM, WARN, "Rf: B=%d, W=%d\n",
			       eBand, eChannelWidth);
		}
	} else {
		if ((eChannelWidth == CW_80MHZ) ||
				(eChannelWidth == CW_80P80MHZ)) {
			u4PrimaryOffset = CAL_CH_OFFSET_80M(ucPriChannel,
							    ucChannelS1);
			if (u4PrimaryOffset >= 4) {
				fgValidBW = FALSE;
				DBGLOG(RLM, WARN, "Rf: PriOffSet=%d, W=%d\n",
				       u4PrimaryOffset, eChannelWidth);
			}
			if (ucPriChannel == 165 && eBand == BAND_5G) {
				fgValidBW = FALSE;
				DBGLOG(RLM, WARN,
				       "Rf: PriOffSet=%d, W=%d C=%d\n",
				       u4PrimaryOffset, eChannelWidth,
				       ucPriChannel);
			}
		} else if (eChannelWidth == CW_160MHZ) {
			u4PrimaryOffset = CAL_CH_OFFSET_160M(ucPriChannel,
							     ucCenterCh);
			if (u4PrimaryOffset >= 8) {
				fgValidBW = FALSE;
				DBGLOG(RLM, WARN,
				       "Rf: PriOffSet=%d, W=%d\n",
				       u4PrimaryOffset, eChannelWidth);
			}
		}
		//TODO: add 320MHZ?
	}

	if ((fgValidBW == FALSE) || (fgValidChannel == FALSE) ||
	    (fgUpperChannel == FALSE) || (fgLowerChannel == FALSE))
		fgValidRfSetting = FALSE;

	return fgValidRfSetting;

}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
/*
 * This function coverts country code from alphabet chars to u32,
 * the caller need to pass country code chars and do size check
 */
uint32_t rlmDomainAlpha2ToU32(char *pcAlpha2, uint8_t ucAlpha2Size)
{
	uint8_t ucIdx;
	uint32_t u4CountryCode = 0;

	if (ucAlpha2Size > TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN) {
		DBGLOG(RLM, ERROR, "alpha2 size %d is invalid!(max: %d)\n",
			ucAlpha2Size, TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN);
		ucAlpha2Size = TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN;
	}

	for (ucIdx = 0; ucIdx < ucAlpha2Size; ucIdx++)
		u4CountryCode |= (pcAlpha2[ucIdx] << (ucIdx * 8));

	return u4CountryCode;
}



#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)

uint32_t
rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
	uint32_t u4CountryCode)
{
	const struct ieee80211_regdomain *pRegdom = NULL;
	char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};
	uint32_t u4FinalCountryCode = u4CountryCode;

	rlmDomainU32ToAlpha(u4FinalCountryCode, acCountryCodeStr);
	pRegdom =
		rlmDomainSearchRegdomainFromLocalDataBase(acCountryCodeStr);
	if (!pRegdom) {
		DBGLOG(RLM, ERROR,
	       "Cannot find the %s RegDomain. Set to default WW\n",
	       acCountryCodeStr);
		pRegdom = &default_regdom_ww;
		u4FinalCountryCode = COUNTRY_CODE_WW;
	}

	kalApplyCustomRegulatory(pRegdom);

	return u4FinalCountryCode;
}
#else
uint32_t
rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
	uint32_t u4CountryCode)
{
	return 0;
}
#endif

uint8_t
rlmDomainCountryCodeUpdateSanity(
	struct GLUE_INFO *prGlueInfo,
	struct ADAPTER **prAdapter)
{
	enum regd_state eCurrentState = rlmDomainGetCtrlState();

	/* Always use the wlan GlueInfo as parameter. */
	if (!prGlueInfo) {
		DBGLOG(RLM, ERROR, "prGlueInfo is NULL!\n");
		return FALSE;
	}

	if (!prGlueInfo->prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter is NULL!\n");
		return FALSE;
	}
	*prAdapter = prGlueInfo->prAdapter;

	if (eCurrentState == REGD_STATE_INVALID ||
		eCurrentState == REGD_STATE_UNDEFINED) {
		DBGLOG(RLM, ERROR, "regd is in an invalid state\n");
		return FALSE;
	}

	return TRUE;
}

void rlmDomainCountryCodeUpdate(
	struct ADAPTER *prAdapter,
	uint32_t u4CountryCode)
{
	uint32_t u4FinalCountryCode = u4CountryCode;
	char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};
#ifdef CFG_SUPPORT_BT_SKU
	typedef void (*bt_fn_t) (char *);
	bt_fn_t bt_func = NULL;
	char *bt_func_name = "btmtk_set_country_code_from_wifi";
	void *func_addr = NULL;
#endif

	if (rlmDomainIsUsingLocalRegDomainDataBase()) {
		u4FinalCountryCode =
			rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
				u4CountryCode);
	}

	rlmDomainU32ToAlpha(u4FinalCountryCode, acCountryCodeStr);

	if (u4FinalCountryCode != u4CountryCode)
		rlmDomainSetCountryCode(acCountryCodeStr,
			MAX_COUNTRY_CODE_LEN);

	DBGLOG(RLM, INFO, "g_mtk_regd_control.alpha2 = %s\n", acCountryCodeStr);
#ifdef CFG_SUPPORT_BT_SKU
#if (CFG_ENABLE_GKI_SUPPORT != 1)
	func_addr = GLUE_SYMBOL_GET(bt_func_name);
#endif
	if (func_addr) {
		bt_func = (bt_fn_t) func_addr;
		bt_func(acCountryCodeStr);
#if (CFG_ENABLE_GKI_SUPPORT != 1)
		GLUE_SYMBOL_PUT(bt_func_name);
#endif
	} else {
		DBGLOG(RLM, ERROR,
		       "Can't find function %s\n",
		       bt_func_name);
	}
#endif

	rlmDomainParsingChannel(prAdapter);

	if (!regd_is_single_sku_en())
		return;

	prAdapter->rWifiVar.u2CountryCode =
		(uint16_t)rlmDomainGetCountryCode();

	/* Send commands to firmware */
	rlmDomainSendCmd(prAdapter, TRUE);

}
void
rlmDomainSetCountry(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = rlmDomainGetGlueInfo();
	struct ADAPTER *prBaseAdapter;

	if (!rlmDomainCountryCodeUpdateSanity(
		prGlueInfo, &prBaseAdapter)) {
		DBGLOG(RLM, WARN, "sanity check failed, skip update\n");
		return;
	}

	rlmDomainCountryCodeUpdate(
		prBaseAdapter,
		rlmDomainGetCountryCode());
}

uint8_t rlmDomainTxPwrLimitGetTableVersion(
	uint8_t *pucBuf, uint32_t u4BufLen)
{
#define TX_PWR_LIMIT_VERSION_STR_LEN 7
#define TX_PWR_LIMIT_MAX_VERSION 2
	uint32_t u4TmpPos = 0;
	uint8_t ucVersion = 0;

	while (u4TmpPos < u4BufLen && pucBuf[u4TmpPos] != '<')
		u4TmpPos++;

	if (u4TmpPos >= (u4BufLen - TX_PWR_LIMIT_VERSION_STR_LEN))
		return ucVersion;

	if (kalStrnCmp(&pucBuf[u4TmpPos + 1], "Ver:", 4) == 0) {
		ucVersion = (pucBuf[u4TmpPos + 5] - '0') * 10 +
			(pucBuf[u4TmpPos + 6] - '0');
	}

	if (ucVersion > TX_PWR_LIMIT_MAX_VERSION)
		ucVersion = 0;

	return ucVersion;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Search the tx power limit setting range of the specified in the text
 *        file
 *
 * \param[IN] u4CountryCode The u32 type of the specified country.
 * \param[IN] pucBuf The content of the text file.
 * \param[IN] u4cBufLen End boundary of the text file.
 * \param[OUT] pu4CountryStart Store the start position of the desired country
 *             settings.
 * \param[OUT] pu4CountryEnd Store the end position of the desired country
 *             settings.
 *
 * \retval TRUE Success.
 * \retval FALSE Failure.
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmDomainTxPwrLimitGetCountryRange(
	uint32_t u4CountryCode, uint8_t *pucBuf, uint32_t u4BufLen,
	uint32_t *pu4CountryStart, uint32_t *pu4CountryEnd)
{
	uint32_t u4TmpPos = 0;
	char pcrCountryStr[TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN + 1] = {0};
	uint8_t cIdx = 0;

	while (1) {
		while (u4TmpPos < u4BufLen && pucBuf[u4TmpPos] != '[')
			u4TmpPos++;

		u4TmpPos++; /* skip the '[' char */

		cIdx = 0;
		while ((u4TmpPos < u4BufLen) &&
			   (cIdx < TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN) &&
			   (pucBuf[u4TmpPos] != ']')) {
			pcrCountryStr[cIdx++] = pucBuf[u4TmpPos];
			u4TmpPos++;
		}

		u4TmpPos++; /* skip the ']' char */

		if ((u4TmpPos >= u4BufLen) ||
		    (cIdx > TX_PWR_LIMIT_COUNTRY_STR_MAX_LEN))
			return FALSE;

		if (u4CountryCode ==
			rlmDomainAlpha2ToU32(pcrCountryStr, cIdx)) {
			DBGLOG(RLM, INFO,
				"Found TxPwrLimit table for CountryCode \"%s\"\n",
				pcrCountryStr);
			*pu4CountryStart = u4TmpPos;
			/* the location after char ']' */
			break;
		}
	}

	while (u4TmpPos < u4BufLen && pucBuf[u4TmpPos] != '[')
		u4TmpPos++;

	*pu4CountryEnd = u4TmpPos;

	return TRUE;
}

u_int8_t rlmDomainTxPwrLimitSearchSection(const char *pSectionName,
	uint8_t *pucBuf, uint32_t *pu4Pos, uint32_t u4BufEnd)
{
	uint32_t u4TmpPos = *pu4Pos;
	uint8_t uSectionNameLen = kalStrLen(pSectionName);

	while (1) {
		while (u4TmpPos < u4BufEnd && pucBuf[u4TmpPos] != '<')
			u4TmpPos++;

		u4TmpPos++; /* skip char '<' */

		if (u4TmpPos + uSectionNameLen >= u4BufEnd)
			return FALSE;

		if (kalStrnCmp(&pucBuf[u4TmpPos],
				pSectionName, uSectionNameLen) == 0) {

			/* Go to the end of section header line */
			while ((u4TmpPos < u4BufEnd) &&
				   (pucBuf[u4TmpPos] != '\n'))
				u4TmpPos++;

			*pu4Pos = u4TmpPos;

			break;
		}
	}

	return TRUE;
}

u_int8_t rlmDomainTxPwrLimitSectionEnd(uint8_t *pucBuf,
	const char *pSectionName, uint32_t *pu4Pos, uint32_t u4BufEnd)
{
	uint32_t u4TmpPos = *pu4Pos;
	char cTmpChar = 0;
	uint8_t uSectionNameLen = kalStrLen(pSectionName);

	while (u4TmpPos < u4BufEnd) {
		cTmpChar = pucBuf[u4TmpPos];

		/* skip blank lines */
		if (cTmpChar == ' ' || cTmpChar == '\t' ||
			cTmpChar == '\n' || cTmpChar == '\r') {
			u4TmpPos++;
			continue;
		}

		break;
	}

	/* 2 means '/' and '>' */
	if (u4TmpPos + uSectionNameLen + 2 >= u4BufEnd) {
		*pu4Pos = u4BufEnd;
		return FALSE;
	}

	if (pucBuf[u4TmpPos] != '<')
		return FALSE;

	if (pucBuf[u4TmpPos + 1] != '/' ||
		pucBuf[u4TmpPos + 2 + uSectionNameLen] != '>' ||
		kalStrnCmp(&pucBuf[u4TmpPos + 2],
			pSectionName, uSectionNameLen)) {

		*pu4Pos = u4TmpPos + uSectionNameLen + 2;
		return FALSE;
	}

	/* 3 means go to the location after '>' */
	*pu4Pos = u4TmpPos + uSectionNameLen + 3;
	return TRUE;
}

int8_t rlmDomainTxPwrLimitGetChIdx(
	struct TX_PWR_LIMIT_DATA *pTxPwrLimit, uint8_t ucChannel)
{
	int8_t cIdx = 0;

	for (cIdx = 0; cIdx < pTxPwrLimit->ucChNum; cIdx++)
		if (ucChannel ==
			pTxPwrLimit->rChannelTxPwrLimit[cIdx].ucChannel)
			return cIdx;

	DBGLOG(RLM, ERROR,
		"Can't find idx of channel %d in TxPwrLimit data\n",
		ucChannel);

	return -1;
}

u_int8_t rlmDomainTxPwrLimitIsTxBfBackoffSection(
	uint8_t ucVersion, uint8_t ucSectionIdx)
{
	if (ucVersion == 1 && ucSectionIdx == 8)
		return TRUE;

	return FALSE;
}
u_int8_t rlmDomainTxPwrLimitLoadChannelSetting(
	uint8_t ucVersion, uint8_t *pucBuf, uint32_t *pu4Pos, uint32_t u4BufEnd,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimit, uint8_t ucSectionIdx)
{
	uint32_t u4TmpPos = *pu4Pos;
	char cTmpChar = 0;
	struct CHANNEL_TX_PWR_LIMIT *prChTxPwrLimit = NULL;
	u_int8_t bNeg = FALSE;
	int8_t cLimitValue = 0, cChIdx = 0;
	uint8_t ucIdx = 0, ucChannel = 0;
	uint8_t ucElementNum =
		gTx_Pwr_Limit_Element_Num[ucVersion][ucSectionIdx];

	/* skip blank lines */
	while (u4TmpPos < u4BufEnd) {
		cTmpChar = pucBuf[u4TmpPos];

		if (cTmpChar == ' ' || cTmpChar == '\t' ||
			cTmpChar == '\n' || cTmpChar == '\r') {
			u4TmpPos++;
			continue;
		}

		break;
	}

	/* current is at the location of 'c',
	 * check remaining buf length for 'chxxx'
	 */
	if (u4TmpPos + 5 >= u4BufEnd) {
		DBGLOG(RLM, ERROR,
			"Invalid location of ch setting: %u/%u\n",
			u4TmpPos, u4BufEnd);
		return FALSE;
	}

	if (pucBuf[u4TmpPos] == 'c' && pucBuf[u4TmpPos + 1] == 'h') {
		ucChannel = (pucBuf[u4TmpPos + 2] - '0') * 100 +
					(pucBuf[u4TmpPos + 3] - '0') * 10 +
					(pucBuf[u4TmpPos + 4] - '0');
	} else { /* invalid format */
		*pu4Pos = u4TmpPos;
		DBGLOG(RLM, ERROR,
			"Invalid ch setting starting chars: %c%c\n",
			pucBuf[u4TmpPos], pucBuf[u4TmpPos + 1]);

		/* goto next line */
		while (*pu4Pos < u4BufEnd && pucBuf[*pu4Pos] != '\n')
			(*pu4Pos)++;

		return TRUE;
	}

	cChIdx = rlmDomainTxPwrLimitGetChIdx(pTxPwrLimit, ucChannel);

	if (cChIdx == -1) {
		*pu4Pos = u4TmpPos;
		DBGLOG(RLM, ERROR, "Invalid ch %u %c%c%c\n", ucChannel,
			pucBuf[u4TmpPos + 2],
			pucBuf[u4TmpPos + 3], pucBuf[u4TmpPos + 4]);

		/* goto next line */
		while (*pu4Pos < u4BufEnd && pucBuf[*pu4Pos] != '\n')
			(*pu4Pos)++;

		return TRUE;
	}

	u4TmpPos += 5;

	prChTxPwrLimit = &pTxPwrLimit->rChannelTxPwrLimit[cChIdx];

	/* read the channel TxPwrLimit settings */
	for (ucIdx = 0; ucIdx < ucElementNum; ucIdx++) {

		/* skip blank and comma */
		while (u4TmpPos < u4BufEnd) {
			cTmpChar = pucBuf[u4TmpPos];

			if ((cTmpChar == ' ') ||
				(cTmpChar == '\t') ||
				(cTmpChar == ',')) {
				u4TmpPos++;
				continue;
			}
			break;
		}

		if (cTmpChar == '\n')
			break;

		if (u4TmpPos >= u4BufEnd) {
			*pu4Pos = u4BufEnd;
			DBGLOG(RLM, ERROR,
				"Invalid location of ch tx pwr limit val: %u/%u\n",
				u4TmpPos, u4BufEnd);
			return FALSE;
		}

		bNeg = FALSE;

		cTmpChar = pucBuf[u4TmpPos];

		if (cTmpChar == '-') {
			bNeg = TRUE;
			u4TmpPos++;
		} else {
			if (cTmpChar == 'x') {
				if (!rlmDomainTxPwrLimitIsTxBfBackoffSection(
					ucVersion, ucSectionIdx)) {
					prChTxPwrLimit->
						rTxPwrLimitValue
						[ucSectionIdx][ucIdx] =
						TX_PWR_LIMIT_MAX_VAL;
				} else {
					prChTxPwrLimit->rTxBfBackoff[ucIdx] =
						TX_PWR_LIMIT_MAX_VAL;
				}
				u4TmpPos++;
				continue;
			}
		}

		cLimitValue = 0;
		while (u4TmpPos < u4BufEnd) {
			cTmpChar = pucBuf[u4TmpPos];

			if (cTmpChar < '0' || cTmpChar > '9')
				break;

			cLimitValue = (cLimitValue * 10) + (cTmpChar - '0');
			u4TmpPos++;
		}

		if (bNeg)
			cLimitValue = -cLimitValue;
		if (!rlmDomainTxPwrLimitIsTxBfBackoffSection(
			ucVersion, ucSectionIdx)) {
			prChTxPwrLimit->rTxPwrLimitValue[ucSectionIdx][ucIdx] =
				cLimitValue;
		} else {
			prChTxPwrLimit->rTxBfBackoff[ucIdx] =
				cLimitValue;
		}
	}

	*pu4Pos = u4TmpPos;
	return TRUE;
}

void rlmDomainTxPwrLimitRemoveComments(
	uint8_t *pucBuf, uint32_t u4BufLen)
{
	uint32_t u4TmpPos = 0;
	char cTmpChar = 0;

	while (u4TmpPos < u4BufLen) {
		cTmpChar = pucBuf[u4TmpPos];

		if (cTmpChar == '#') {
			while (cTmpChar != '\n') {
				pucBuf[u4TmpPos] = ' ';

				u4TmpPos++;
				if (u4TmpPos >= u4BufLen)
					break;

				cTmpChar = pucBuf[u4TmpPos];
			}
		}
		u4TmpPos++;
	}
}

u_int8_t rlmDomainTxPwrLimitLoad(
	struct ADAPTER *prAdapter, uint8_t *pucBuf, uint32_t u4BufLen,
	uint8_t ucVersion, uint32_t u4CountryCode,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	uint8_t uSecIdx = 0;
	uint8_t ucSecNum = gTx_Pwr_Limit_Section[ucVersion].ucSectionNum;
	uint32_t u4CountryStart = 0, u4CountryEnd = 0, u4Pos = 0;
	struct TX_PWR_LIMIT_SECTION *prSection =
		&gTx_Pwr_Limit_Section[ucVersion];
	uint8_t *prFileName = prAdapter->chip_info->prTxPwrLimitFile;


	if (!rlmDomainTxPwrLimitGetCountryRange(u4CountryCode, pucBuf,
		u4BufLen, &u4CountryStart, &u4CountryEnd)) {
		DBGLOG(RLM, ERROR, "Can't find specified table in %s\n",
			prFileName);

		/* Use WW as default country */
		if (!rlmDomainTxPwrLimitGetCountryRange(COUNTRY_CODE_WW, pucBuf,
			u4BufLen, &u4CountryStart, &u4CountryEnd)) {
			DBGLOG(RLM, ERROR,
				"Can't find default table (WW) in %s\n",
				prFileName);
			return FALSE;
		}
	}

	u4Pos = u4CountryStart;

	for (uSecIdx = 0; uSecIdx < ucSecNum; uSecIdx++) {
		const uint8_t *pSecName =
			prSection->arSectionNames[uSecIdx];
		if (!rlmDomainTxPwrLimitSearchSection(
				pSecName, pucBuf, &u4Pos,
				u4CountryEnd)) {
			DBGLOG(RLM, ERROR,
				"Can't find specified section %s in %s\n",
				pSecName,
				prFileName);
			continue;
		}

		DBGLOG(RLM, INFO, "Find specified section %s in %s\n",
			pSecName,
			prFileName);

		while (!rlmDomainTxPwrLimitSectionEnd(pucBuf,
			pSecName,
			&u4Pos, u4CountryEnd) &&
			u4Pos < u4CountryEnd) {
			if (!rlmDomainTxPwrLimitLoadChannelSetting(
				ucVersion, pucBuf, &u4Pos, u4CountryEnd,
				pTxPwrLimitData, uSecIdx))
				return FALSE;
			if (rlmDomainTxPwrLimitIsTxBfBackoffSection(
				ucVersion, uSecIdx))
				g_bTxBfBackoffExists = TRUE;
		}
	}

	DBGLOG(RLM, INFO, "Load %s finished\n", prFileName);
	return TRUE;
}

void rlmDomainTxPwrLimitSetChValues(
	uint8_t ucVersion,
	struct CMD_CHANNEL_POWER_LIMIT_V2 *pCmd,
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit)
{
	uint8_t section = 0, e = 0;
	uint8_t ucElementNum = 0;

	pCmd->tx_pwr_dsss_cck = pChTxPwrLimit->rTxPwrLimitValue[0][0];
	pCmd->tx_pwr_dsss_bpsk = pChTxPwrLimit->rTxPwrLimitValue[0][1];

	/* 6M, 9M */
	pCmd->tx_pwr_ofdm_bpsk = pChTxPwrLimit->rTxPwrLimitValue[0][2];
	/* 12M, 18M */
	pCmd->tx_pwr_ofdm_qpsk = pChTxPwrLimit->rTxPwrLimitValue[0][3];
	/* 24M, 36M */
	pCmd->tx_pwr_ofdm_16qam = pChTxPwrLimit->rTxPwrLimitValue[0][4];
	pCmd->tx_pwr_ofdm_48m = pChTxPwrLimit->rTxPwrLimitValue[0][5];
	pCmd->tx_pwr_ofdm_54m = pChTxPwrLimit->rTxPwrLimitValue[0][6];

	/* MCS0*/
	pCmd->tx_pwr_ht20_bpsk = pChTxPwrLimit->rTxPwrLimitValue[1][0];
	/* MCS1, MCS2*/
	pCmd->tx_pwr_ht20_qpsk = pChTxPwrLimit->rTxPwrLimitValue[1][1];
	/* MCS3, MCS4*/
	pCmd->tx_pwr_ht20_16qam = pChTxPwrLimit->rTxPwrLimitValue[1][2];
	/* MCS5*/
	pCmd->tx_pwr_ht20_mcs5 = pChTxPwrLimit->rTxPwrLimitValue[1][3];
	/* MCS6*/
	pCmd->tx_pwr_ht20_mcs6 = pChTxPwrLimit->rTxPwrLimitValue[1][4];
	/* MCS7*/
	pCmd->tx_pwr_ht20_mcs7 = pChTxPwrLimit->rTxPwrLimitValue[1][5];

	/* MCS0*/
	pCmd->tx_pwr_ht40_bpsk = pChTxPwrLimit->rTxPwrLimitValue[2][0];
	/* MCS1, MCS2*/
	pCmd->tx_pwr_ht40_qpsk = pChTxPwrLimit->rTxPwrLimitValue[2][1];
	/* MCS3, MCS4*/
	pCmd->tx_pwr_ht40_16qam = pChTxPwrLimit->rTxPwrLimitValue[2][2];
	/* MCS5*/
	pCmd->tx_pwr_ht40_mcs5 = pChTxPwrLimit->rTxPwrLimitValue[2][3];
	/* MCS6*/
	pCmd->tx_pwr_ht40_mcs6 = pChTxPwrLimit->rTxPwrLimitValue[2][4];
	/* MCS7*/
	pCmd->tx_pwr_ht40_mcs7 = pChTxPwrLimit->rTxPwrLimitValue[2][5];
	/* MCS32*/
	pCmd->tx_pwr_ht40_mcs32 = pChTxPwrLimit->rTxPwrLimitValue[2][6];

	/* MCS0*/
	pCmd->tx_pwr_vht20_bpsk = pChTxPwrLimit->rTxPwrLimitValue[3][0];
	/* MCS1, MCS2*/
	pCmd->tx_pwr_vht20_qpsk = pChTxPwrLimit->rTxPwrLimitValue[3][1];
	/* MCS3, MCS4*/
	pCmd->tx_pwr_vht20_16qam = pChTxPwrLimit->rTxPwrLimitValue[3][2];
	/* MCS5, MCS6*/
	pCmd->tx_pwr_vht20_64qam = pChTxPwrLimit->rTxPwrLimitValue[3][3];
	pCmd->tx_pwr_vht20_mcs7 = pChTxPwrLimit->rTxPwrLimitValue[3][4];
	pCmd->tx_pwr_vht20_mcs8 = pChTxPwrLimit->rTxPwrLimitValue[3][5];
	pCmd->tx_pwr_vht20_mcs9 = pChTxPwrLimit->rTxPwrLimitValue[3][6];

	pCmd->tx_pwr_vht_40 = pChTxPwrLimit->rTxPwrLimitValue[4][2];
	pCmd->tx_pwr_vht_80 = pChTxPwrLimit->rTxPwrLimitValue[4][3];
	pCmd->tx_pwr_vht_160c = pChTxPwrLimit->rTxPwrLimitValue[4][5];
	pCmd->tx_pwr_vht_160nc = pChTxPwrLimit->rTxPwrLimitValue[4][4];
	pCmd->tx_pwr_lg_40 = pChTxPwrLimit->rTxPwrLimitValue[4][0];
	pCmd->tx_pwr_lg_80 = pChTxPwrLimit->rTxPwrLimitValue[4][1];


	DBGLOG(RLM, TRACE, "ch %d\n", pCmd->ucCentralCh);
	for (section = 0; section < TX_PWR_LIMIT_SECTION_NUM; section++) {
		struct TX_PWR_LIMIT_SECTION *pSection =
			&gTx_Pwr_Limit_Section[ucVersion];
		ucElementNum = gTx_Pwr_Limit_Element_Num[ucVersion][section];
		for (e = 0; e < ucElementNum; e++)
			DBGLOG(RLM, TRACE, "TxPwrLimit[%s][%s]= %d\n",
				pSection->arSectionNames[section],
				gTx_Pwr_Limit_Element[ucVersion][section][e],
				pChTxPwrLimit->rTxPwrLimitValue[section][e]);
	}
}

void rlmDomainTxPwrLimitPerRateSetChValues(
	uint8_t ucVersion,
	struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE *pCmd,
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit)
{
	uint8_t section = 0, e = 0, count = 0;
	uint8_t ucElementNum = 0;

	for (section = 0; section < TX_PWR_LIMIT_SECTION_NUM; section++) {
		if (rlmDomainTxPwrLimitIsTxBfBackoffSection(ucVersion, section))
			continue;
		ucElementNum = gTx_Pwr_Limit_Element_Num[ucVersion][section];
		for (e = 0; e < ucElementNum; e++) {
			pCmd->aucTxPwrLimit.i1PwrLimit[count] =
				pChTxPwrLimit->rTxPwrLimitValue[section][e];
			count++;
		}
	}

	DBGLOG(RLM, TRACE, "ch %d\n", pCmd->u1CentralCh);
	count = 0;
	for (section = 0; section < TX_PWR_LIMIT_SECTION_NUM; section++) {
		struct TX_PWR_LIMIT_SECTION *pSection =
			&gTx_Pwr_Limit_Section[ucVersion];
		if (rlmDomainTxPwrLimitIsTxBfBackoffSection(ucVersion, section))
			continue;
		ucElementNum = gTx_Pwr_Limit_Element_Num[ucVersion][section];
		for (e = 0; e < ucElementNum; e++) {
			DBGLOG(RLM, TRACE, "TxPwrLimit[%s][%s]= %d\n",
				pSection->arSectionNames[section],
				gTx_Pwr_Limit_Element[ucVersion][section][e],
				pCmd->aucTxPwrLimit.i1PwrLimit[count]);
			count++;
		}
	}
}

void rlmDomainTxPwrLimitSetValues(
	uint8_t ucVersion,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2 *pSetCmd,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimit)
{
	uint8_t ucIdx = 0;
	int8_t cChIdx = 0;
	struct CMD_CHANNEL_POWER_LIMIT_V2 *pCmd = NULL;
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit = NULL;

	if (!pSetCmd || !pTxPwrLimit) {
		DBGLOG(RLM, ERROR, "Invalid TxPwrLimit request\n");
		return;
	}

	for (ucIdx = 0; ucIdx < pSetCmd->ucNum; ucIdx++) {
		pCmd = &(pSetCmd->rChannelPowerLimit[ucIdx]);
		cChIdx = rlmDomainTxPwrLimitGetChIdx(pTxPwrLimit,
			pCmd->ucCentralCh);
		if (cChIdx == -1) {
			DBGLOG(RLM, ERROR,
				"Invalid ch idx found while assigning values\n");
			continue;
		}
		pChTxPwrLimit = &pTxPwrLimit->rChannelTxPwrLimit[cChIdx];
		rlmDomainTxPwrLimitSetChValues(ucVersion, pCmd, pChTxPwrLimit);
	}
}

void rlmDomainTxPwrLimitPerRateSetValues(
	uint8_t ucVersion,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *pSetCmd,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimit)
{
	uint8_t ucIdx = 0;
	int8_t cChIdx = 0;
	struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE *pChPwrLimit = NULL;
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit = NULL;

	if (pSetCmd == NULL) {
		DBGLOG(RLM, ERROR, "%s pSetCmd is NULL\n", __func__);
		return;
	}

	for (ucIdx = 0; ucIdx < pSetCmd->ucNum; ucIdx++) {
		pChPwrLimit = &(pSetCmd->rChannelPowerLimit[ucIdx]);
		cChIdx = rlmDomainTxPwrLimitGetChIdx(pTxPwrLimit,
			pChPwrLimit->u1CentralCh);

		if (cChIdx == -1) {
			DBGLOG(RLM, ERROR,
				"Invalid ch idx found while assigning values\n");
			continue;
		}
		pChTxPwrLimit = &pTxPwrLimit->rChannelTxPwrLimit[cChIdx];
		rlmDomainTxPwrLimitPerRateSetChValues(ucVersion,
			pChPwrLimit, pChTxPwrLimit);
	}
}

u_int8_t rlmDomainTxPwrLimitLoadFromFile(
	struct ADAPTER *prAdapter,
	uint8_t **pucConfigBuf, uint32_t *pu4ConfigReadLen)
{
#define TXPWRLIMIT_FILE_LEN 64
	u_int8_t bRet = TRUE;
	uint8_t *prFileName = prAdapter->chip_info->prTxPwrLimitFile;
	uint8_t aucPath[TXPWRLIMIT_FILE_LEN];

	if (!prFileName || kalStrLen(prFileName) == 0) {
		bRet = FALSE;
		DBGLOG(RLM, ERROR, "Invalid TxPwrLimit dat file name!!\n");
		goto error;
	}

	kalMemZero(aucPath, sizeof(aucPath));
	kalSnprintf(aucPath, TXPWRLIMIT_FILE_LEN, "%s", prFileName);

	if (kalRequestFirmware(
			aucPath,
			pucConfigBuf,
			pu4ConfigReadLen,
			FALSE,
			kalGetGlueDevHdl(prAdapter->prGlueInfo)) == 0) {
		/* ToDo:: Nothing */
	} else {
		bRet = FALSE;
		goto error;
	}

	if (*pu4ConfigReadLen == 0) {
		bRet = FALSE;
		goto error;
	}

error:

	return bRet;
}

u_int8_t rlmDomainGetTxPwrLimit(
	uint32_t country_code,
	uint8_t *pucVersion,
	struct GLUE_INFO *prGlueInfo,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	u_int8_t bRet;
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;

	bRet = rlmDomainTxPwrLimitLoadFromFile(prGlueInfo->prAdapter,
		&pucConfigBuf, &u4ConfigReadLen);

	if (!bRet)
		goto error;

	rlmDomainTxPwrLimitRemoveComments(pucConfigBuf, u4ConfigReadLen);
	*pucVersion = rlmDomainTxPwrLimitGetTableVersion(pucConfigBuf,
		u4ConfigReadLen);

	if (!rlmDomainTxPwrLimitLoad(prGlueInfo->prAdapter,
		pucConfigBuf, u4ConfigReadLen, *pucVersion,
		country_code, pTxPwrLimitData)) {
		bRet = FALSE;
		goto error;
	}

error:

	if (pucConfigBuf)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);

	return bRet;
}

#endif

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY

/*----------------------------------------------------------------------------*/
/*!
 * @brief Check if power limit setting is in the range [MIN_TX_POWER,
 *        MAX_TX_POWER]
 *
 * @param[in]
 *
 * @return (fgValid) : 0 -> inValid, 1 -> Valid
 */
/*----------------------------------------------------------------------------*/
u_int8_t
rlmDomainCheckPowerLimitValid(struct ADAPTER *prAdapter,
			      struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION
						rPowerLimitTableConfiguration,
			      uint8_t ucPwrLimitNum)
{
	uint16_t i;
	u_int8_t fgValid = TRUE;
	int8_t *prPwrLimit;

	prPwrLimit = &rPowerLimitTableConfiguration.aucPwrLimit[0];

	for (i = 0; i < ucPwrLimitNum; i++, prPwrLimit++) {
		if (*prPwrLimit > MAX_TX_POWER || *prPwrLimit < MIN_TX_POWER) {
			fgValid = FALSE;
			break;	/*Find out Wrong Power limit */
		}
	}
	return fgValid;

}

/*----------------------------------------------------------------------------*/
/*!
 * @brief 1.Check if power limit configuration table valid(channel intervel)
 *	2.Check if power limit configuration/default table entry repeat
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainCheckCountryPowerLimitTable(struct ADAPTER *prAdapter)
{
#define PwrLmtConf g_rRlmPowerLimitConfiguration
	uint16_t i, j;
	uint16_t u2CountryCodeTable, u2CountryCodeCheck;
	u_int8_t fgChannelValid = FALSE;
	u_int8_t fgPowerLimitValid = FALSE;
	u_int8_t fgEntryRepetetion = FALSE;
	u_int8_t fgTableValid = TRUE;

	/*1.Configuration Table Check */
	for (i = 0; i < sizeof(PwrLmtConf) /
	     sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION); i++) {
		/*Table Country Code */
		WLAN_GET_FIELD_BE16(&PwrLmtConf[i].aucCountryCode[0],
				    &u2CountryCodeTable);

		/*<1>Repetition Entry Check */
		for (j = i + 1; j < sizeof(PwrLmtConf) /
		     sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION);
		     j++) {

			WLAN_GET_FIELD_BE16(&PwrLmtConf[j].aucCountryCode[0],
					    &u2CountryCodeCheck);
			if (((PwrLmtConf[i].ucCentralCh) ==
			     PwrLmtConf[j].ucCentralCh)
			    && (u2CountryCodeTable == u2CountryCodeCheck)) {
				fgEntryRepetetion = TRUE;
				DBGLOG(RLM, LOUD,
				       "Domain: Configuration Repetition CC=%c%c, Ch=%d\n",
				       PwrLmtConf[i].aucCountryCode[0],
				       PwrLmtConf[i].aucCountryCode[1],
				       PwrLmtConf[i].ucCentralCh);
			}
		}

		/*<2>Channel Number Interval Check */
		fgChannelValid =
		    rlmDomainCheckChannelEntryValid(prAdapter,
				BAND_NULL, PwrLmtConf[i].ucCentralCh);

		/*<3>Power Limit Range Check */
		fgPowerLimitValid =
		    rlmDomainCheckPowerLimitValid(prAdapter,
						  PwrLmtConf[i],
						  PWR_LIMIT_NUM);

		if (fgChannelValid == FALSE || fgPowerLimitValid == FALSE) {
			fgTableValid = FALSE;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
			DBGLOG(RLM, LOUD,
				"Domain: CC=%c%c, Ch=%d, Limit: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d, Valid:%d,%d\n",
				PwrLmtConf[i].aucCountryCode[0],
				PwrLmtConf[i].aucCountryCode[1],
				PwrLmtConf[i].ucCentralCh,
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_CCK_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_CCK_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_OFDM_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_OFDM_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_20M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_20M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_40M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_40M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_80M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_80M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_160M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_160M_H],
				fgChannelValid,
				fgPowerLimitValid);
#else
			DBGLOG(RLM, LOUD,
				"Domain: CC=%c%c, Ch=%d, Limit: %d,%d,%d,%d,%d,%d,%d,%d,%d, Valid:%d,%d\n",
				PwrLmtConf[i].aucCountryCode[0],
				PwrLmtConf[i].aucCountryCode[1],
				PwrLmtConf[i].ucCentralCh,
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_CCK],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_20M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_20M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_40M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_40M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_80M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_80M_H],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_160M_L],
				PwrLmtConf[i].aucPwrLimit[PWR_LIMIT_160M_H],
				fgChannelValid,
				fgPowerLimitValid);
#endif
		}

		if (u2CountryCodeTable == COUNTRY_CODE_NULL) {
			DBGLOG(RLM, LOUD, "Domain: Full search down\n");
			break;	/*End of country table entry */
		}

	}

	if (fgEntryRepetetion == FALSE)
		DBGLOG(RLM, TRACE,
		       "Domain: Configuration Table no Repetiton.\n");

	/*Configuration Table no error */
	if (fgTableValid == TRUE)
		prAdapter->fgIsPowerLimitTableValid = TRUE;
	else
		prAdapter->fgIsPowerLimitTableValid = FALSE;

	/*2.Default Table Repetition Entry Check */
	fgEntryRepetetion = FALSE;
	for (i = 0; i < sizeof(g_rRlmPowerLimitDefault) /
	     sizeof(struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT); i++) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[i].
							aucCountryCode[0],
				    &u2CountryCodeTable);

		for (j = i + 1; j < sizeof(g_rRlmPowerLimitDefault) /
		     sizeof(struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT); j++) {
			WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[j].
							aucCountryCode[0],
					    &u2CountryCodeCheck);
			if (u2CountryCodeTable == u2CountryCodeCheck) {
				fgEntryRepetetion = TRUE;
				DBGLOG(RLM, LOUD,
				       "Domain: Default Repetition CC=%c%c\n",
				       g_rRlmPowerLimitDefault[j].
							aucCountryCode[0],
				       g_rRlmPowerLimitDefault[j].
							aucCountryCode[1]);
			}
		}
	}
	if (fgEntryRepetetion == FALSE)
		DBGLOG(RLM, TRACE, "Domain: Default Table no Repetiton.\n");
#undef PwrLmtConf
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief
 *
 * @param[in]
 *
 * @return (u2TableIndex) : if  0xFFFF -> No Table Match
 */
/*----------------------------------------------------------------------------*/
uint16_t rlmDomainPwrLimitDefaultTableDecision(struct ADAPTER *prAdapter,
					       uint16_t u2CountryCode)
{
	uint16_t i, j;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	uint16_t u2TableIndex = POWER_LIMIT_TABLE_NULL;	/* No Table Match */
	struct COUNTRY_POWER_LIMIT_GROUP_TABLE *prCountryGrpInfo;

	for (i = 0; i < COUNTRY_LIMIT_GROUP_NUM; i++) {
		prCountryGrpInfo = &arSupportCountryPowerLmtGrps[i];

		if (!prCountryGrpInfo->u4CountryNum ||
			!prCountryGrpInfo->prGroup)
			continue;

		for (j = 0; j < prCountryGrpInfo->u4CountryNum; j++) {
			WLAN_GET_FIELD_BE16(
				&prCountryGrpInfo->prGroup[j].aucCountryCode[0],
				&u2CountryCodeTable);

			if (u2CountryCodeTable == u2CountryCode) {
				WLAN_GET_FIELD_BE16(
					&prCountryGrpInfo->aucGroupCode[0],
					&u2CountryCode);
				break;
			}
		}
		if (j < prCountryGrpInfo->u4CountryNum)
			break;	/* Found */
	}

	if (i >= COUNTRY_LIMIT_GROUP_NUM) {
		DBGLOG(RLM, TRACE,
			"Can't find Country = (%c%c) in any group!\n",
			((u2CountryCode & 0xff00) >> 8),
			(u2CountryCode & 0x00ff));
	}

	/*Default Table Index */
	for (i = 0; i < sizeof(g_rRlmPowerLimitDefault) /
	     sizeof(struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT); i++) {

		WLAN_GET_FIELD_BE16(&g_rRlmPowerLimitDefault[i].
						aucCountryCode[0],
				    &u2CountryCodeTable);

		if (u2CountryCodeTable == u2CountryCode) {
			u2TableIndex = i;
			break;	/*match country code */
		} else if (u2CountryCodeTable == COUNTRY_CODE_NULL) {
			u2TableIndex = i;
			break;	/*find last one country- Default */
		}
	}

	return u2TableIndex;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Get build default table power limit cmd
 *        channel range start & end index
 *
 * @param[in] eType : Power limit type
 *
 * @param[out] pu1StartIdx : start index
 *             pu1EndIdx   : end index
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
rlmDomainGetStartEndIdx(enum ENUM_PWR_LIMIT_TYPE eType,
		uint8_t *pu1StartIdx, uint8_t *pu1EndIdx)
{
	if (pu1StartIdx == NULL || pu1EndIdx == NULL)
		return;

	switch (eType) {
	case PWR_LIMIT_TYPE_COMP_11AC:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11AC_V2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11AX:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11AX_BW160: {
		*pu1StartIdx = PWR_LMT_SUBBAND_2G4;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII3;
	}
		break;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_LIMIT_TYPE_COMP_11BE_1: {
		*pu1StartIdx = PWR_LMT_SUBBAND_2G4;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII2A;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_2: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII2C;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII3;
	}
		break;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	case PWR_LIMIT_TYPE_COMP_6E_1: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII5A;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII5B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_6E_2: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII6;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII7B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_6E_3: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII8;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII8;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII5A;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII5B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII6;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII7B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_3:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII8;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII8;
	}
		break;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_LIMIT_TYPE_COMP_11BE_6G_1: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII5A;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII5A;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_2: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII5B;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII5B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_3: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII6;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII6;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_4: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII7A;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII7A;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_5: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII7B;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII7B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_6: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII8;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII8;
	}
		break;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	default:
		break;
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Get default table subband txpower index
 *
 * @param[in] eSubBandIdx : Subband index use in g_rRlmSubBand
 *
 * @param[out] pu1SubBandPwrIdx : Subband power index in default table
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
rlmDomainGetSubBandDefPwrIdx(enum ENUM_PWR_LMT_SUBBAND eSubBandIdx,
	uint8_t *pu1SubBandPwrIdx)
{
	if (pu1SubBandPwrIdx == NULL)
		return;

	switch (eSubBandIdx) {
	case PWR_LMT_SUBBAND_2G4:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_2G4;
		break;
	case PWR_LMT_SUBBAND_UNII1:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII1;
		break;
	case PWR_LMT_SUBBAND_UNII2A:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII2A;
		break;
	case PWR_LMT_SUBBAND_UNII2C:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII2C;
		break;
	case PWR_LMT_SUBBAND_UNII3:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII3;
		break;
#if (CFG_SUPPORT_WIFI_6G == 1)
	case PWR_LMT_SUBBAND_UNII5A:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII5;
		break;
	case PWR_LMT_SUBBAND_UNII5B:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII5;
		break;
	case PWR_LMT_SUBBAND_UNII6:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII6;
		break;
	case PWR_LMT_SUBBAND_UNII7A:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII7;
		break;
	case PWR_LMT_SUBBAND_UNII7B:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII7;
		break;
	case PWR_LMT_SUBBAND_UNII8:
		*pu1SubBandPwrIdx = PWR_LMT_SUBBAND_PWR_UNII8;
		break;
#endif /* CFG_SUPPORT_WIFI_6G */
	default:
		break;
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill power limit CMD by Power Limit Default Table(regulation)
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void
rlmDomainBuildCmdByDefaultTable(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT
				*prCmd,
				uint16_t u2DefaultTableIndex)
{
	uint16_t i, k;
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitSubBand = NULL;
	struct CMD_CHANNEL_POWER_LIMIT *prPwrLimit = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_HE *prPwrLmtHE = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prPwrLmtHEBW160 = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prPwrLmtEHT = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct CMD_CHANNEL_POWER_LIMIT_6E *prPwrLmt6E = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prPwrLmtLegacy6G = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prPwrLmtEHT_6G = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	enum ENUM_PWR_LIMIT_TYPE eType;
	int8_t startIndex = 0, endIndex = 0;

	int8_t cLmtBand = 0;
	uint8_t u1PwrIdx = 0;
	ASSERT(prCmd);

	prPwrLimitSubBand = &g_rRlmPowerLimitDefault[u2DefaultTableIndex];
	eType = prCmd->ucLimitType;
	if (eType == PWR_LIMIT_TYPE_COMP_11AX)
		prPwrLmtHE = &prCmd->u.rChPwrLimtHE[0];
	else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160)
		prPwrLmtHEBW160 = &prCmd->u.rChPwrLimtHEBW160[0];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
				eType == PWR_LIMIT_TYPE_COMP_11BE_2)
		prPwrLmtEHT = &prCmd->u.rChPwrLimtEHT[0];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
				eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
				eType == PWR_LIMIT_TYPE_COMP_6E_3)
		prPwrLmt6E = &prCmd->u.rChPwrLimt6E[0];
	else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
		eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3)
		prPwrLmtLegacy6G = &prCmd->u.rChPwrLimtLegacy_6G[0];
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (eType >=
				PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_11BE_6G_6)
		prPwrLmtEHT_6G = &prCmd->u.rChPwrLimtEHT_6G[0];
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2)
		prPwrLimit = &prCmd->u.rChannelPowerLimit[0];

	/* choose power limit cmd channel range index*/
	rlmDomainGetStartEndIdx(eType, &startIndex, &endIndex);

	/*Build power limit cmd by default table information */
	for (i = startIndex; i <= endIndex; i++) {

		for (k = g_rRlmSubBand[i].ucStartCh;
		     k <= g_rRlmSubBand[i].ucEndCh;
		     k += g_rRlmSubBand[i].ucInterval) {

			/*
			* Get subband power limit from default table
			* by mapping subband index
			*/
			rlmDomainGetSubBandDefPwrIdx(i, &u1PwrIdx);

			/* cLmtBand need reset by each channel */
			cLmtBand =
				prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx];
			if (prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx]
				> MAX_TX_POWER) {
				DBGLOG(RLM, WARN,
				"SubBand[%d] Pwr(%d) > Max (%d)",
				u1PwrIdx,
				prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx],
				MAX_TX_POWER);
				cLmtBand = MAX_TX_POWER;
			}

			if (eType == PWR_LIMIT_TYPE_COMP_11AX)
				prPwrLmtHE->ucCentralCh = k;
			else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160)
				prPwrLmtHEBW160->ucCentralCh = k;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2)
				prPwrLmtEHT->ucCentralCh = k;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3)
				prPwrLmt6E->ucCentralCh = k;

			else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3)
				prPwrLmtLegacy6G->ucCentralCh = k;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <= PWR_LIMIT_TYPE_COMP_11BE_6G_6)
				prPwrLmtEHT_6G->ucCentralCh = k;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
			else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2)
				prPwrLimit->ucCentralCh = k;

			if ((prPwrLimitSubBand->ucPwrUnit
						& BIT(u1PwrIdx)) == 0) {

				if (eType == PWR_LIMIT_TYPE_COMP_11AX)
					kalMemSet(&prPwrLmtHE->cPwrLimitRU26L,
						cLmtBand,
						PWR_LIMIT_HE_NUM);
				else if (eType ==
					PWR_LIMIT_TYPE_COMP_11AX_BW160)
					kalMemSet(
					  &prPwrLmtHEBW160->cPwrLimitRU26L,
					  cLmtBand,
					  PWR_LIMIT_HE_BW160_NUM);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2)
					kalMemSet(&prPwrLmtEHT->cPwrLimitEHT26L,
						cLmtBand,
						PWR_LIMIT_EHT_NUM);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3)
					kalMemSet(&prPwrLmt6E->cPwrLimitRU26L,
						cLmtBand,
						PWR_LIMIT_6E_NUM);

				else if (eType >=
					PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3)
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
					kalMemSet(
					&prPwrLmtLegacy6G->cPwrLimitCCK_L,
					cLmtBand,
					PWR_LIMIT_LEGACY_6G_NUM);
#else
					kalMemSet(
					&prPwrLmtLegacy6G->cPwrLimitCCK,
					cLmtBand,
					PWR_LIMIT_LEGACY_6G_NUM);
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <= PWR_LIMIT_TYPE_COMP_11BE_6G_6)
					kalMemSet(
					&prPwrLmtEHT_6G->cPwrLimitEHT26L,
					cLmtBand,
					PWR_LIMIT_EHT_6G_NUM);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2)
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
					kalMemSet(&prPwrLimit->cPwrLimitCCK_L,
						cLmtBand, PWR_LIMIT_NUM);
#else
					kalMemSet(&prPwrLimit->cPwrLimitCCK,
						cLmtBand, PWR_LIMIT_NUM);
#endif

			} else {
					/* ex: 40MHz power limit(mW\MHz)
					 *	= 20MHz power limit(mW\MHz) * 2
					 * ---> 40MHz power limit(dBm)
					 *	= 20MHz power limit(dBm) + 6;
					 */

				if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
					/* BW20 */
					prPwrLmtHE->cPwrLimitRU26L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU26H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU26U = cLmtBand;

					prPwrLmtHE->cPwrLimitRU52L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU52H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU52U = cLmtBand;

					prPwrLmtHE->cPwrLimitRU106L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU106H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU106U = cLmtBand;

					prPwrLmtHE->cPwrLimitRU242L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU242H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU242U = cLmtBand;

				} else if (eType ==
					PWR_LIMIT_TYPE_COMP_11AX_BW160) {
					/* BW20 */
					prPwrLmtHEBW160->cPwrLimitRU26L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU26H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU26U =
						cLmtBand;

					prPwrLmtHEBW160->cPwrLimitRU52L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU52H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU52U =
						cLmtBand;

					prPwrLmtHEBW160->cPwrLimitRU106L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU106H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU106U =
						cLmtBand;

					prPwrLmtHEBW160->cPwrLimitRU242L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU242H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU242U =
						cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
					/* BW20 */
					prPwrLmtEHT->cPwrLimitEHT26L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT52L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT52H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT52U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT106L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT106H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT106U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT242L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT242H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT242U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT26_52L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26_52H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26_52U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT26_106L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26_106H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT26_106U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
					/* BW20 */
					prPwrLmt6E->cPwrLimitRU26L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU26H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU26U = cLmtBand;

					prPwrLmt6E->cPwrLimitRU52L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU52H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU52U = cLmtBand;

					prPwrLmt6E->cPwrLimitRU106L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU106H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU106U = cLmtBand;

					prPwrLmt6E->cPwrLimitRU242L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU242H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU242U = cLmtBand;
				} else if (eType >=
				PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
					/* BW20 */
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
					prPwrLmtLegacy6G->cPwrLimitCCK_L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitCCK_H
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitOFDM_L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitOFDM_H
						= cLmtBand;
#else
					prPwrLmtLegacy6G->cPwrLimitCCK
						= cLmtBand;
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
					prPwrLmtLegacy6G->cPwrLimit20L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimit20H
						= cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
					/* BW20 */
					prPwrLmtEHT_6G->cPwrLimitEHT26L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT52L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT52H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT52U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT106L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT106H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT106U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT242L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT242H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT242U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT26_52L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26_52H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26_52U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT26_106L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26_106H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT26_106U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
					/* BW20 */
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
					prPwrLimit->cPwrLimitCCK_L = cLmtBand;
					prPwrLimit->cPwrLimitCCK_H = cLmtBand;
					prPwrLimit->cPwrLimitOFDM_L = cLmtBand;
					prPwrLimit->cPwrLimitOFDM_H = cLmtBand;
#else
					prPwrLimit->cPwrLimitCCK = cLmtBand;
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
					prPwrLimit->cPwrLimit20L = cLmtBand;
					prPwrLimit->cPwrLimit20H = cLmtBand;
				}

				/* BW40 */
				cLmtBand += 6;

				if (cLmtBand > MAX_TX_POWER)
					cLmtBand = MAX_TX_POWER;

				if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
					prPwrLmtHE->cPwrLimitRU484L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU484H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU484U = cLmtBand;
				} else if (eType ==
					PWR_LIMIT_TYPE_COMP_11AX_BW160) {
					prPwrLmtHEBW160->cPwrLimitRU484L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU484H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU484U =
						cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
					prPwrLmtEHT->cPwrLimitEHT484L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT484H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT484U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT484_242L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT484_242H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT484_242U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
					prPwrLmt6E->cPwrLimitRU484L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU484H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU484U = cLmtBand;
				} else if (eType >=
				PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
					prPwrLmtLegacy6G->cPwrLimit40L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimit40H
						= cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
					prPwrLmtEHT_6G->cPwrLimitEHT484L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT484H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT484U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT484_242L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT484_242H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT484_242U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
					prPwrLimit->cPwrLimit40L = cLmtBand;
					prPwrLimit->cPwrLimit40H = cLmtBand;
				}

				/* BW80 */
				cLmtBand += 6;
				if (cLmtBand > MAX_TX_POWER)
					cLmtBand = MAX_TX_POWER;

				if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
					prPwrLmtHE->cPwrLimitRU996L = cLmtBand;
					prPwrLmtHE->cPwrLimitRU996H = cLmtBand;
					prPwrLmtHE->cPwrLimitRU996U = cLmtBand;
				} else if (eType ==
					PWR_LIMIT_TYPE_COMP_11AX_BW160) {
					prPwrLmtHEBW160->cPwrLimitRU996L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU996H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU996U =
						cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
					prPwrLmtEHT->cPwrLimitEHT996L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT996_484L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996_484H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996_484U
						=	cLmtBand;

					prPwrLmtEHT->cPwrLimitEHT996_484_242L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996_484_242H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996_484_242U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
					prPwrLmt6E->cPwrLimitRU996L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU996H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU996U = cLmtBand;
				} else if (eType >=
				PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
					prPwrLmtLegacy6G->cPwrLimit80L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimit80H
						= cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
					prPwrLmtEHT_6G->cPwrLimitEHT996L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT996_484L
						= cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996_484H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996_484U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT996_484_242L
						= cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996_484_242H
						= cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996_484_242U
						= cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
					prPwrLimit->cPwrLimit80L = cLmtBand;
					prPwrLimit->cPwrLimit80H = cLmtBand;
				}

					/* BW160 */
				cLmtBand += 6;
				if (cLmtBand > MAX_TX_POWER)
					cLmtBand = MAX_TX_POWER;
				if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
					/* prPwrLmtHE do nothing */
					prPwrLmtHEBW160->cPwrLimitRU1992L =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU1992H =
						cLmtBand;
					prPwrLmtHEBW160->cPwrLimitRU1992U =
						cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
					prPwrLmtEHT->cPwrLimitEHT996X2L
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996X2H
						=	cLmtBand;
					prPwrLmtEHT->cPwrLimitEHT996X2U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
					prPwrLmt6E->cPwrLimitRU1992L = cLmtBand;
					prPwrLmt6E->cPwrLimitRU1992H = cLmtBand;
					prPwrLmt6E->cPwrLimitRU1992U = cLmtBand;
				} else if (eType >=
				PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
					prPwrLmtLegacy6G->cPwrLimit160L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimit160H
						= cLmtBand;
				}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
					prPwrLmtEHT_6G->cPwrLimitEHT996X2L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X2H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X2U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT996X2_484L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X2_484H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X2_484U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
					prPwrLimit->cPwrLimit160L = cLmtBand;
					prPwrLimit->cPwrLimit160H = cLmtBand;
				}

				/* BW320 */
				cLmtBand += 6;
				if (cLmtBand > MAX_TX_POWER)
					cLmtBand = MAX_TX_POWER;

#if (CFG_SUPPORT_WIFI_6G == 1)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
					prPwrLmtEHT_6G->cPwrLimitEHT996X3L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X3H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X3U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT996X3_484L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X3_484H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X3_484U
						=	cLmtBand;

					prPwrLmtEHT_6G->cPwrLimitEHT996X4L
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X4H
						=	cLmtBand;
					prPwrLmtEHT_6G->cPwrLimitEHT996X4U
						=	cLmtBand;
				}
#endif /* CFG_SUPPORT_WIFI_6G  */
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
			}
				/* save to power limit array per
				 * subband channel
				 */
			if (eType == PWR_LIMIT_TYPE_COMP_11AX)
				prPwrLmtHE++;
			else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160)
				prPwrLmtHEBW160++;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2)
				prPwrLmtEHT++;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
			else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3)
				prPwrLmt6E++;
			else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3)
				prPwrLmtLegacy6G++;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <= PWR_LIMIT_TYPE_COMP_11BE_6G_6)
				prPwrLmtEHT_6G++;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
			else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2)
				prPwrLimit++;

			prCmd->ucNum++;

			if (prCmd->ucNum >= MAX_CMD_SUPPORT_CHANNEL_NUM) {
				DBGLOG(RLM, WARN,
					"etype = %d, out of MAX CH Num\n",
					eType);
				break;
			}

		}
	}

	DBGLOG(RLM, TRACE, "Build Default Limit(%c%c)ChNum=%d,typde=%d\n",
				((prCmd->u2CountryCode &
				0xff00) >> 8),
				(prCmd->u2CountryCode &
				0x00ff),
				prCmd->ucNum,
				eType);

}

void rlmDomainCopyFromConfigTable(struct CMD_CHANNEL_POWER_LIMIT *prCmdPwrLimit,
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION *prPwrLimitConfig)
{


#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK_L,
		   &prPwrLimitConfig->aucPwrLimit[0],
		   PWR_LIMIT_NUM);
#else
	kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK,
		   &prPwrLimitConfig->aucPwrLimit[0],
		   PWR_LIMIT_NUM);
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
}

static void PwrLmtTblArbitrator(int8_t *target,
	int8_t *compare,
	uint32_t size)
{
	uint8_t i = 0;

	/* Choose min value from target & compare */
	for (i = 0; i < size; i++) {
		if (target[i] > compare[i])
			target[i] = compare[i];

		/* Sanity check power boundary */
		if (target[i] > MAX_TX_POWER)
			target[i] = MAX_TX_POWER;
		else if (target[i] < MIN_TX_POWER)
			target[i] = MIN_TX_POWER;
	}
}

static void rlmDomainCompareFromConfigTable(int8_t *prPwrLmt,
	int8_t *prPwrLmtConf,
	enum ENUM_PWR_LIMIT_TYPE eType)
{
	uint32_t size;

	switch (eType) {
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	case PWR_LIMIT_TYPE_COMP_11AC_V2:
#else
	case PWR_LIMIT_TYPE_COMP_11AC:
#endif
		size = PWR_LIMIT_NUM;
		break;

	case PWR_LIMIT_TYPE_COMP_11AX:
		size = PWR_LIMIT_HE_NUM;
		break;

	case PWR_LIMIT_TYPE_COMP_11AX_BW160:
		size = PWR_LIMIT_HE_BW160_NUM;
		break;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_LIMIT_TYPE_COMP_11BE_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_2:
		size = PWR_LIMIT_EHT_NUM;
		break;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	case PWR_LIMIT_TYPE_COMP_6E_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_6E_2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_6E_3:
		size = PWR_LIMIT_6E_NUM;
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_3:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3:
		size = PWR_LIMIT_LEGACY_6G_NUM;
		break;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_LIMIT_TYPE_COMP_11BE_6G_1:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_6G_2:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_6G_3:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_6G_4:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_6G_5:
		/* fallthrough */
	case PWR_LIMIT_TYPE_COMP_11BE_6G_6:
		size = PWR_LIMIT_EHT_6G_NUM;
		break;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	default:
		DBGLOG(RLM, WARN,
			"Invalid rate type : etype = %d\n", eType);
		return;
	}

	/* Choose min value from defarult table & Conf table */
	PwrLmtTblArbitrator(prPwrLmt, prPwrLmtConf, size);
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief Fill power limit CMD by Power Limit Configurartion Table
 * (Bandedge and Customization)
 *
 * @param[in]
 *
 * @return (none)
 */
/*----------------------------------------------------------------------------*/
void rlmDomainBuildCmdByConfigTable(struct ADAPTER *prAdapter,
			struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd)
{
#define PwrLmtConf g_rRlmPowerLimitConfiguration
#define PwrLmtConfHE g_rRlmPowerLimitConfigurationHE
#define PwrLmtConfHEBW160 g_rRlmPowerLimitConfigurationHEBW160
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PwrLmtConfEHT g_rRlmPowerLimitConfigurationEHT
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
#define PwrLmtConf6E g_rRlmPowerLimitConfiguration6E
#define PwrLmtConfLegacy_6G g_rRlmPowerLimitConfigurationLegacy6G
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PwrLmtConfEHT_6G g_rRlmPowerLimitConfigurationEHT_6G
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	uint16_t i, k;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	enum ENUM_PWR_LIMIT_TYPE eType;
	struct CMD_CHANNEL_POWER_LIMIT *prCmdPwrLimit;
	struct CMD_CHANNEL_POWER_LIMIT_HE *prCmdPwrLimtHE;
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prCmdPwrLimtHEBW160;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prCmdPwrLimtEHT;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct CMD_CHANNEL_POWER_LIMIT_6E *prCmdPwrLimt6E;
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prCmdPwrLimtLegacy_6G;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prCmdPwrLimtEHT_6G;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	u_int8_t fgChannelValid;
	uint8_t ucCentCh;
	uint8_t ucPwrLmitConfSize = sizeof(PwrLmtConf) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION);

	uint8_t ucPwrLmitConfSizeHE = sizeof(PwrLmtConfHE) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE);
	uint8_t ucPwrLmitConfSizeHEBW160 = sizeof(PwrLmtConfHEBW160) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE_BW160);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	uint8_t ucPwrLmitConfSizeEHT = sizeof(PwrLmtConfEHT) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t ucPwrLmitConfSize6E = sizeof(PwrLmtConf6E) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_6E);
	uint8_t ucPwrLmitConfSizeLegacy_6G = sizeof(PwrLmtConfLegacy_6G) /
		sizeof(
		struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY_6G);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	uint8_t ucPwrLmitConfSizeEHT_6G = sizeof(PwrLmtConfEHT_6G) /
		sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT_6G);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	eType = prCmd->ucLimitType;

	/*Build power limit cmd by configuration table information */
	for (k = 0; k < prCmd->ucNum; k++) {

		if (k >= MAX_CMD_SUPPORT_CHANNEL_NUM) {
			DBGLOG(RLM, ERROR, "out of MAX CH Num\n");
			return;
		}

		if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
			prCmdPwrLimtHE = &prCmd->u.rChPwrLimtHE[k];
			ucCentCh = prCmdPwrLimtHE->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeHE; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConfHE[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
						PwrLmtConfHE[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConfHE[i].ucCentralCh)
					continue;

				/* Choose MINIMUN value from
				 * Default table & Conf table
				 * Cmd setting (Default table
				 * information) and Conf table
				 * has repetition channel entry,
				 * ex : Default table (ex: 2.4G,
				 *      limit = 20dBm) -->
				 *      ch1~14 limit =20dBm,
				 * Conf table (ex: ch1, limit =
				 *      22 dBm) --> ch 1 = 22 dBm
				 * Conf table (ex: ch2, limit =
				 *      18 dBm) --> ch 2 = 18 dBm
				 * Cmd final setting --> ch1 =
				 *      20dBm, ch2 = 18dBm ch3~14 = 20dBm
				 */
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtHE->cPwrLimitRU26L,
					&PwrLmtConfHE[i].aucPwrLimit[0],
					eType);
			}
		} else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
			prCmdPwrLimtHEBW160 = &prCmd->u.rChPwrLimtHEBW160[k];
			ucCentCh = prCmdPwrLimtHEBW160->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeHEBW160; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConfHEBW160[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
					  prAdapter,
					  BAND_NULL,
					  PwrLmtConfHEBW160[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConfHEBW160[i].ucCentralCh)
					continue;

				/* Choose MINIMUN value from
				 * Default table & Conf table
				 * Cmd setting (Default table
				 * information) and Conf table
				 * has repetition channel entry,
				 * ex : Default table (ex: 2.4G,
				 *      limit = 20dBm) -->
				 *      ch1~14 limit =20dBm,
				 * Conf table (ex: ch1, limit =
				 *      22 dBm) --> ch 1 = 22 dBm
				 * Conf table (ex: ch2, limit =
				 *      18 dBm) --> ch 2 = 18 dBm
				 * Cmd final setting --> ch1 =
				 *      20dBm, ch2 = 18dBm ch3~14 = 20dBm
				 */
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtHEBW160->cPwrLimitRU26L,
					&PwrLmtConfHEBW160[i].aucPwrLimit[0],
					eType);
			}
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
			prCmdPwrLimtEHT = &prCmd->u.rChPwrLimtEHT[k];
			ucCentCh = prCmdPwrLimtEHT->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeEHT; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConfEHT[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
					PwrLmtConfEHT[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConfEHT[i].ucCentralCh)
					continue;

				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtEHT->cPwrLimitEHT26L,
					&PwrLmtConfEHT[i].aucPwrLimit[0],
					eType);
			}
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
			prCmdPwrLimt6E = &prCmd->u.rChPwrLimt6E[k];
			ucCentCh = prCmdPwrLimt6E->ucCentralCh;
			for (i = 0; i < ucPwrLmitConfSize6E; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConf6E[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(prAdapter,
				    BAND_6G,
					PwrLmtConf6E[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConf6E[i].ucCentralCh)
					continue;

				/* Choose MINIMUN value from
				 * Default table & Conf table
				 * Cmd setting (Default table
				 * information) and Conf table
				 * has repetition channel entry,
				 * ex : Default table (ex: 6G,
				 *      limit = 20dBm) -->
				 *      ch1~14 limit =20dBm,
				 * Conf table (ex: ch1, limit =
				 *      22 dBm) --> ch 1 = 22 dBm
				 * Conf table (ex: ch2, limit =
				 *      18 dBm) --> ch 2 = 18 dBm
				 * Cmd final setting --> ch1 =
				 *      20dBm, ch2 = 18dBm ch3~14 = 20dBm
				 */
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimt6E->cPwrLimitRU26L,
					&PwrLmtConf6E[i].aucPwrLimit[0],
					eType);
			}
		} else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <=  PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
			prCmdPwrLimtLegacy_6G =
				&prCmd->u.rChPwrLimtLegacy_6G[k];
			ucCentCh = prCmdPwrLimtLegacy_6G->ucCentralCh;
			for (i = 0; i < ucPwrLmitConfSizeLegacy_6G; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConfLegacy_6G[i]
					.aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(
					prAdapter,
					BAND_6G,
					PwrLmtConfLegacy_6G[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConfLegacy_6G[i].ucCentralCh)
					continue;

				/* Choose MINIMUN value from
				 * Default table & Conf table
				 * Cmd setting (Default table
				 * information) and Conf table
				 * has repetition channel entry,
				 * ex : Default table (ex: 6G,
				 *      limit = 20dBm) -->
				 *      ch1~14 limit =20dBm,
				 * Conf table (ex: ch1, limit =
				 *      22 dBm) --> ch 1 = 22 dBm
				 * Conf table (ex: ch2, limit =
				 *      18 dBm) --> ch 2 = 18 dBm
				 * Cmd final setting --> ch1 =
				 *      20dBm, ch2 = 18dBm ch3~14 = 20dBm
				 */
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L,
					&PwrLmtConfLegacy_6G[i].aucPwrLimit[0],
					eType);
#else
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtLegacy_6G->cPwrLimitCCK,
					&PwrLmtConfLegacy_6G[i].aucPwrLimit[0],
					eType);
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
			}
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
			prCmdPwrLimtEHT_6G = &prCmd->u.rChPwrLimtEHT_6G[k];
			ucCentCh = prCmdPwrLimtEHT_6G->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeEHT_6G; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConfEHT_6G[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(prAdapter,
				    BAND_6G,
					PwrLmtConfEHT_6G[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh
					!= PwrLmtConfEHT_6G[i].ucCentralCh)
					continue;

				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtEHT_6G->cPwrLimitEHT26L,
					&PwrLmtConfEHT_6G[i].aucPwrLimit[0],
					eType);
			}
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
		else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
			eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
			prCmdPwrLimit = &prCmd->u.rChannelPowerLimit[k];
			ucCentCh = prCmdPwrLimit->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSize; i++) {

				WLAN_GET_FIELD_BE16(
					&PwrLmtConf[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
						PwrLmtConf[i].ucCentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (u2CountryCodeTable
					!= prCmd->u2CountryCode)
					continue;
				else if (fgChannelValid == FALSE)
					continue;
				else if (ucCentCh != PwrLmtConf[i].ucCentralCh)
					continue;

				/* Choose MINIMUN value from
				 * Default table & Conf table
				 * Cmd setting (Default table
				 * information) and Conf table
				 * has repetition channel entry,
				 * ex : Default table (ex: 2.4G,
				 *      limit = 20dBm) -->
				 *      ch1~14 limit =20dBm,
				 * Conf table (ex: ch1, limit =
				 *      22 dBm) --> ch 1 = 22 dBm
				 * Conf table (ex: ch2, limit =
				 *      18 dBm) --> ch 2 = 18 dBm
				 * Cmd final setting --> ch1 =
				 *      20dBm, ch2 = 18dBm ch3~14 = 20dBm
				 */
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimit->cPwrLimitCCK_L,
					&PwrLmtConf[i].aucPwrLimit[0],
					eType);
#else
				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimit->cPwrLimitCCK,
					&PwrLmtConf[i].aucPwrLimit[0],
					eType);
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */

			}
		}
	}

#undef PwrLmtConf
#undef PwrLmtConfHE
#undef PwrLmtConfHEBW160
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#undef PwrLmtConfEHT
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
#undef PwrLmtConf6E
#undef PwrLmtConfLegacy_6G
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#undef PwrLmtConfEHT_6G
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
struct TX_PWR_LIMIT_DATA *
rlmDomainInitTxPwrLimitData_6G(struct ADAPTER *prAdapter)
{
	uint8_t ch_cnt = 0;
	uint8_t ch_idx = 0;
	uint8_t ch_num = 0;
	const int8_t *prChannelList = NULL;
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData = NULL;
	struct CHANNEL_TX_PWR_LIMIT *prChTxPwrLimit = NULL;

	pTxPwrLimitData =
		(struct TX_PWR_LIMIT_DATA *)
		kalMemAlloc(sizeof(struct TX_PWR_LIMIT_DATA),
			VIR_MEM_TYPE);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Alloc buffer for TxPwrLimit main struct failed\n");
		return NULL;
	}

	pTxPwrLimitData->ucChNum = TX_PWR_LIMIT_6G_CH_NUM;

	pTxPwrLimitData->rChannelTxPwrLimit =
		(struct CHANNEL_TX_PWR_LIMIT *)
		kalMemAlloc(sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			(pTxPwrLimitData->ucChNum), VIR_MEM_TYPE);

	if (!pTxPwrLimitData->rChannelTxPwrLimit) {
		DBGLOG(RLM, ERROR,
			"Alloc buffer for TxPwrLimit ch values failed\n");

		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));
		return NULL;
	}

	for (ch_idx = 0; ch_idx < pTxPwrLimitData->ucChNum; ch_idx++) {
		prChTxPwrLimit =
			&(pTxPwrLimitData->rChannelTxPwrLimit[ch_idx]);
		kalMemSet(prChTxPwrLimit->rTxPwrLimitValue,
			MAX_TX_POWER,
			sizeof(prChTxPwrLimit->rTxPwrLimitValue));
		kalMemSet(prChTxPwrLimit->rTxBfBackoff,
			MAX_TX_POWER,
			sizeof(prChTxPwrLimit->rTxBfBackoff));
	}

	ch_cnt = 0;

	prChannelList = gTx_Pwr_Limit_6g_Ch;
	ch_num = TX_PWR_LIMIT_6G_CH_NUM;

	for (ch_idx = 0; ch_idx < ch_num; ch_idx++) {
		pTxPwrLimitData->rChannelTxPwrLimit[ch_cnt].ucChannel =
			prChannelList[ch_idx];
		++ch_cnt;
	}

	return pTxPwrLimitData;
}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

struct TX_PWR_LIMIT_DATA *
rlmDomainInitTxPwrLimitData(struct ADAPTER *prAdapter)
{
	uint8_t ch_cnt = 0;
	uint8_t ch_idx = 0;
	uint8_t band_idx = 0;
	uint8_t ch_num = 0;
	const int8_t *prChannelList = NULL;
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData = NULL;
	struct CHANNEL_TX_PWR_LIMIT *prChTxPwrLimit = NULL;

	pTxPwrLimitData =
		(struct TX_PWR_LIMIT_DATA *)
		kalMemAlloc(sizeof(struct TX_PWR_LIMIT_DATA),
			VIR_MEM_TYPE);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Alloc buffer for TxPwrLimit main struct failed\n");
		return NULL;
	}

	pTxPwrLimitData->ucChNum =
		TX_PWR_LIMIT_2G_CH_NUM + TX_PWR_LIMIT_5G_CH_NUM;

	pTxPwrLimitData->rChannelTxPwrLimit =
		(struct CHANNEL_TX_PWR_LIMIT *)
		kalMemAlloc(sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			(pTxPwrLimitData->ucChNum), VIR_MEM_TYPE);

	if (!pTxPwrLimitData->rChannelTxPwrLimit) {
		DBGLOG(RLM, ERROR,
			"Alloc buffer for TxPwrLimit ch values failed\n");

		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));
		return NULL;
	}

	for (ch_idx = 0; ch_idx < pTxPwrLimitData->ucChNum; ch_idx++) {
		prChTxPwrLimit =
			&(pTxPwrLimitData->rChannelTxPwrLimit[ch_idx]);
		kalMemSet(prChTxPwrLimit->rTxPwrLimitValue,
			MAX_TX_POWER,
			sizeof(prChTxPwrLimit->rTxPwrLimitValue));
		kalMemSet(prChTxPwrLimit->rTxBfBackoff,
			MAX_TX_POWER,
			sizeof(prChTxPwrLimit->rTxBfBackoff));
	}

	ch_cnt = 0;
	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		if (band_idx != KAL_BAND_2GHZ && band_idx != KAL_BAND_5GHZ)
			continue;

		prChannelList = (band_idx == KAL_BAND_2GHZ) ?
			gTx_Pwr_Limit_2g_Ch : gTx_Pwr_Limit_5g_Ch;

		ch_num =
			(band_idx == KAL_BAND_2GHZ) ?
				TX_PWR_LIMIT_2G_CH_NUM :
				TX_PWR_LIMIT_5G_CH_NUM;

		for (ch_idx = 0; ch_idx < ch_num; ch_idx++) {
			pTxPwrLimitData->rChannelTxPwrLimit[ch_cnt].ucChannel =
				prChannelList[ch_idx];
			++ch_cnt;
		}
	}

	return pTxPwrLimitData;
}

void
rlmDomainSendTxPwrLimitCmd(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	uint8_t ch_cnt = 0;
	uint8_t ch_idx = 0;
	uint8_t band_idx = 0;
	const int8_t *prChannelList = NULL;
	uint32_t rStatus;
	uint32_t u4SetQueryInfoLen;
	uint32_t u4SetCmdTableMaxSize[KAL_NUM_BANDS] = {0};
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2
		*prCmd[KAL_NUM_BANDS] = {0};
	struct CMD_CHANNEL_POWER_LIMIT_V2 *prCmdChPwrLimitV2 = NULL;
	uint32_t u4SetCountryCmdSize =
		sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2);
	uint32_t u4ChPwrLimitV2Size = sizeof(struct CMD_CHANNEL_POWER_LIMIT_V2);
	const uint8_t ucCmdBatchSize =
		prAdapter->chip_info->ucTxPwrLimitBatchSize;

	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		if (band_idx != KAL_BAND_2GHZ && band_idx != KAL_BAND_5GHZ)
			continue;

		prChannelList = (band_idx == KAL_BAND_2GHZ) ?
			gTx_Pwr_Limit_2g_Ch : gTx_Pwr_Limit_5g_Ch;

		ch_cnt = (band_idx == KAL_BAND_2GHZ) ? TX_PWR_LIMIT_2G_CH_NUM :
			TX_PWR_LIMIT_5G_CH_NUM;

		if (!ch_cnt)
			continue;

		u4SetCmdTableMaxSize[band_idx] = u4SetCountryCmdSize +
			ch_cnt * u4ChPwrLimitV2Size;

		prCmd[band_idx] = cnmMemAlloc(prAdapter,
			RAM_TYPE_BUF, u4SetCmdTableMaxSize[band_idx]);

		if (!prCmd[band_idx]) {
			DBGLOG(RLM, ERROR, "Domain: no buf to send cmd\n");
			goto error;
		}

		/*initialize tw pwr table*/
		kalMemSet(prCmd[band_idx], MAX_TX_POWER,
			u4SetCmdTableMaxSize[band_idx]);

		prCmd[band_idx]->ucNum = ch_cnt;
		prCmd[band_idx]->eband =
			(band_idx == KAL_BAND_2GHZ) ?
				BAND_2G4 : BAND_5G;
		prCmd[band_idx]->countryCode = rlmDomainGetCountryCode();

		DBGLOG(RLM, INFO,
			"%s, active n_channels=%d, band=%d\n",
			__func__, ch_cnt, prCmd[band_idx]->eband);

		for (ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
			prCmdChPwrLimitV2 =
				&(prCmd[band_idx]->rChannelPowerLimit[ch_idx]);
			prCmdChPwrLimitV2->ucCentralCh =
				prChannelList[ch_idx];
		}
	}

	rlmDomainTxPwrLimitSetValues(ucVersion,
		prCmd[KAL_BAND_2GHZ], pTxPwrLimitData);
	rlmDomainTxPwrLimitSetValues(ucVersion,
		prCmd[KAL_BAND_5GHZ], pTxPwrLimitData);

	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		uint8_t ucRemainChNum, i, ucTempChNum, prCmdBatchNum;
		uint32_t u4BufSize = 0;
		struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT_V2 *prTempCmd = NULL;
		enum ENUM_BAND eBand = (band_idx == KAL_BAND_2GHZ) ?
				BAND_2G4 : BAND_5G;
		uint16_t u2ChIdx = 0;

		if (!prCmd[band_idx])
			continue;

		ucRemainChNum = prCmd[band_idx]->ucNum;
		prCmdBatchNum = (ucRemainChNum +
			ucCmdBatchSize - 1) /
			ucCmdBatchSize;

		for (i = 0; i < prCmdBatchNum; i++) {
			if (i == prCmdBatchNum - 1)
				ucTempChNum = ucRemainChNum;
			else
				ucTempChNum = ucCmdBatchSize;

			u4BufSize = u4SetCountryCmdSize +
				ucTempChNum * u4ChPwrLimitV2Size;

			prTempCmd =
				cnmMemAlloc(prAdapter,
					RAM_TYPE_BUF, u4BufSize);

			if (!prTempCmd) {
				DBGLOG(RLM, ERROR,
					"Domain: no buf to send cmd\n");
				goto error;
			}

			/*copy partial tx pwr limit*/
			prTempCmd->ucNum = ucTempChNum;
			prTempCmd->eband = eBand;
			prTempCmd->countryCode = rlmDomainGetCountryCode();
			u2ChIdx = i * ucCmdBatchSize;
			kalMemCopy(&prTempCmd->rChannelPowerLimit[0],
				&prCmd[band_idx]->rChannelPowerLimit[u2ChIdx],
				ucTempChNum * u4ChPwrLimitV2Size);

			u4SetQueryInfoLen = u4BufSize;
			/* Update tx max. power info to chip */
			rStatus = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_COUNTRY_POWER_LIMIT,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				u4SetQueryInfoLen,
				(uint8_t *) prTempCmd,
				NULL,
				0);

			cnmMemFree(prAdapter, prTempCmd);

			ucRemainChNum -= ucTempChNum;
		}
	}

error:
	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		if (prCmd[band_idx])
			cnmMemFree(prAdapter, prCmd[band_idx]);
	}
}

#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
void rlmDomainTxPwrLimitSendPerRateCmd_6G(
	struct ADAPTER *prAdapter,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *prCmd
)
{
	uint32_t rStatus;
	uint32_t u4SetQueryInfoLen;
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE);
	const uint8_t ucCmdBatchSize =
		prAdapter->chip_info->ucTxPwrLimitBatchSize;

	uint8_t ucRemainChNum, i, ucTempChNum, prCmdBatchNum;
	uint32_t u4BufSize = 0;
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE
		*prTempCmd = NULL;
	enum ENUM_BAND eBand = 0x3;
	uint16_t u2ChIdx = 0;
	u_int8_t bCmdFinished = FALSE;

	if (!prCmd) {
		DBGLOG(RLM, ERROR, "%s: prCmd is NULL\n", __func__);
		return;
	}

	ucRemainChNum = prCmd->ucNum;
	prCmdBatchNum = (ucRemainChNum +
			ucCmdBatchSize - 1) /
			ucCmdBatchSize;

	for (i = 0; i < prCmdBatchNum; i++) {
		if (i == prCmdBatchNum - 1) {
			ucTempChNum = ucRemainChNum;
			bCmdFinished = TRUE;
		} else {
			ucTempChNum = ucCmdBatchSize;
		}

		u4BufSize = u4SetCountryTxPwrLimitCmdSize +
			ucTempChNum * u4ChPwrLimitSize;

		prTempCmd =
			cnmMemAlloc(prAdapter,
				RAM_TYPE_BUF, u4BufSize);

		if (!prTempCmd) {
			DBGLOG(RLM, ERROR,
				"%s: no buf to send cmd\n", __func__);
			return;
		}

		/*copy partial tx pwr limit*/
		prTempCmd->ucNum = ucTempChNum;
		prTempCmd->eBand = eBand;
		prTempCmd->u4CountryCode =
			rlmDomainGetCountryCode();
		prTempCmd->bCmdFinished = bCmdFinished;
		u2ChIdx = i * ucCmdBatchSize;
		kalMemCopy(
			&prTempCmd->rChannelPowerLimit[0],
			&prCmd->rChannelPowerLimit[u2ChIdx],
			ucTempChNum * u4ChPwrLimitSize);

		u4SetQueryInfoLen = u4BufSize;
		/* Update tx max. power info to chip */
		rStatus = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_COUNTRY_POWER_LIMIT_PER_RATE,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			u4SetQueryInfoLen,
			(uint8_t *) prTempCmd,
			NULL,
			0);

		cnmMemFree(prAdapter, prTempCmd);

		ucRemainChNum -= ucTempChNum;
	}
}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

u_int32_t rlmDomainInitTxPwrLimitPerRateCmd(
	struct ADAPTER *prAdapter,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *prCmd[],
	uint32_t prCmdSize[])
{
	uint8_t ch_cnt = 0;
	uint8_t ch_idx = 0;
	uint8_t band_idx = 0;
	const int8_t *prChannelList = NULL;
	uint32_t u4SetCmdTableMaxSize[KAL_NUM_BANDS] = {0};
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE);
	struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE *prChPwrLimit = NULL;

	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		if (band_idx != KAL_BAND_2GHZ && band_idx != KAL_BAND_5GHZ)
			continue;

		prChannelList = (band_idx == KAL_BAND_2GHZ) ?
			gTx_Pwr_Limit_2g_Ch : gTx_Pwr_Limit_5g_Ch;

		ch_cnt = (band_idx == KAL_BAND_2GHZ) ? TX_PWR_LIMIT_2G_CH_NUM :
			TX_PWR_LIMIT_5G_CH_NUM;

		if (!ch_cnt)
			continue;

		u4SetCmdTableMaxSize[band_idx] = u4SetCountryTxPwrLimitCmdSize +
			ch_cnt * u4ChPwrLimitSize;
		prCmdSize[band_idx] = u4SetCmdTableMaxSize[band_idx];

		prCmd[band_idx] = kalMemAlloc(u4SetCmdTableMaxSize[band_idx],
			VIR_MEM_TYPE);
		if (!prCmd[band_idx]) {
			DBGLOG(RLM, ERROR, "Domain: no buf to send cmd\n");
			return WLAN_STATUS_RESOURCES;
		}

		/*initialize tx pwr table*/
		kalMemSet(prCmd[band_idx]->rChannelPowerLimit, MAX_TX_POWER,
			ch_cnt * u4ChPwrLimitSize);

		prCmd[band_idx]->ucNum = ch_cnt;
		prCmd[band_idx]->eBand =
			(band_idx == KAL_BAND_2GHZ) ?
				BAND_2G4 : BAND_5G;
		prCmd[band_idx]->u4CountryCode = rlmDomainGetCountryCode();

		DBGLOG(RLM, INFO,
			"%s, active n_channels=%d, band=%d\n",
			__func__, ch_cnt, prCmd[band_idx]->eBand);

		for (ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
			prChPwrLimit =
				&(prCmd[band_idx]->
				  rChannelPowerLimit[ch_idx]);
			prChPwrLimit->u1CentralCh =	prChannelList[ch_idx];
		}

	}

	return WLAN_STATUS_SUCCESS;
}

void rlmDomainTxPwrLimitSendPerRateCmd(
	struct ADAPTER *prAdapter,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE *prCmd[]
)
{
	uint32_t rStatus;
	uint32_t u4SetQueryInfoLen;
	uint8_t band_idx = 0;
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE);
	const uint8_t ucCmdBatchSize =
		prAdapter->chip_info->ucTxPwrLimitBatchSize;

	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++) {
		uint8_t ucRemainChNum, i, ucTempChNum, prCmdBatchNum;
		uint32_t u4BufSize = 0;
		struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE
			*prTempCmd = NULL;
		enum ENUM_BAND eBand = (band_idx == KAL_BAND_2GHZ) ?
				BAND_2G4 : BAND_5G;
		uint16_t u2ChIdx = 0;
		u_int8_t bCmdFinished = FALSE;

		if (!prCmd[band_idx])
			continue;

		ucRemainChNum = prCmd[band_idx]->ucNum;
		prCmdBatchNum = (ucRemainChNum +
			ucCmdBatchSize - 1) /
			ucCmdBatchSize;

		for (i = 0; i < prCmdBatchNum; i++) {
			if (i == prCmdBatchNum - 1) {
				ucTempChNum = ucRemainChNum;
				bCmdFinished = TRUE;
			} else {
				ucTempChNum = ucCmdBatchSize;
			}

			u4BufSize = u4SetCountryTxPwrLimitCmdSize +
				ucTempChNum * u4ChPwrLimitSize;

			prTempCmd =
				cnmMemAlloc(prAdapter,
					RAM_TYPE_BUF, u4BufSize);

			if (!prTempCmd) {
				DBGLOG(RLM, ERROR,
					"Domain: no buf to send cmd\n");
				return;
			}

			/*copy partial tx pwr limit*/
			prTempCmd->ucNum = ucTempChNum;
			prTempCmd->eBand = eBand;
			prTempCmd->u4CountryCode =
				rlmDomainGetCountryCode();
			prTempCmd->bCmdFinished = bCmdFinished;
			u2ChIdx = i * ucCmdBatchSize;
			kalMemCopy(
				&prTempCmd->rChannelPowerLimit[0],
				&prCmd[band_idx]->
				 rChannelPowerLimit[u2ChIdx],
				ucTempChNum * u4ChPwrLimitSize);

			u4SetQueryInfoLen = u4BufSize;
			/* Update tx max. power info to chip */
			rStatus = wlanSendSetQueryCmd(prAdapter,
				CMD_ID_SET_COUNTRY_POWER_LIMIT_PER_RATE,
				TRUE,
				FALSE,
				FALSE,
				NULL,
				NULL,
				u4SetQueryInfoLen,
				(uint8_t *) prTempCmd,
				NULL,
				0);

			cnmMemFree(prAdapter, prTempCmd);

			ucRemainChNum -= ucTempChNum;
		}
	}
}

u_int32_t rlmDomainInitTxBfBackoffCmd(
	struct ADAPTER *prAdapter,
	struct CMD_TXPWR_TXBF_SET_BACKOFF **prCmd
)
{
	uint8_t ucChNum = TX_PWR_LIMIT_2G_CH_NUM + TX_PWR_LIMIT_5G_CH_NUM;
	uint8_t ucChIdx = 0;
	uint8_t ucChCnt = 0;
	uint8_t ucBandIdx = 0;
	uint8_t ucCnt = 0;
	const int8_t *prChannelList = NULL;
	uint32_t u4SetCmdSize = sizeof(struct CMD_TXPWR_TXBF_SET_BACKOFF);
	struct CMD_TXPWR_TXBF_CHANNEL_BACKOFF *prChTxBfBackoff = NULL;

	if (ucChNum >= CMD_POWER_LIMIT_TABLE_SUPPORT_CHANNEL_NUM) {
		DBGLOG(RLM, ERROR, "ChNum %d should <= %d\n",
			ucChNum, CMD_POWER_LIMIT_TABLE_SUPPORT_CHANNEL_NUM);
		return WLAN_STATUS_FAILURE;
	}

	*prCmd = cnmMemAlloc(prAdapter,
		RAM_TYPE_BUF, u4SetCmdSize);

	if (!*prCmd) {
		DBGLOG(RLM, ERROR, "Domain: no buf to send cmd\n");
		return WLAN_STATUS_RESOURCES;
	}

	/*initialize backoff table*/
	kalMemSet((*prCmd)->rChannelTxBfBackoff, MAX_TX_POWER,
		sizeof((*prCmd)->rChannelTxBfBackoff));

	(*prCmd)->ucNum = ucChNum;
	(*prCmd)->ucBssIdx = aisGetDefaultLinkBssIndex(prAdapter);

	for (ucBandIdx = 0; ucBandIdx < KAL_NUM_BANDS; ucBandIdx++) {
		if (ucBandIdx != KAL_BAND_2GHZ && ucBandIdx != KAL_BAND_5GHZ)
			continue;

		prChannelList = (ucBandIdx == KAL_BAND_2GHZ) ?
			gTx_Pwr_Limit_2g_Ch : gTx_Pwr_Limit_5g_Ch;
		ucChCnt = (ucBandIdx == KAL_BAND_2GHZ) ?
			TX_PWR_LIMIT_2G_CH_NUM : TX_PWR_LIMIT_5G_CH_NUM;

		for (ucChIdx = 0; ucChIdx < ucChCnt; ucChIdx++) {
			prChTxBfBackoff =
				&((*prCmd)->rChannelTxBfBackoff[ucCnt++]);
			prChTxBfBackoff->ucCentralCh =	prChannelList[ucChIdx];
		}
	}

	return WLAN_STATUS_SUCCESS;
}

void rlmDomainTxPwrTxBfBackoffSetValues(
	uint8_t ucVersion,
	struct CMD_TXPWR_TXBF_SET_BACKOFF *prTxBfBackoffCmd,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	uint8_t ucIdx = 0;
	int8_t cChIdx = 0;
	struct CMD_TXPWR_TXBF_CHANNEL_BACKOFF *pChTxBfBackoff = NULL;
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit = NULL;

	if (prTxBfBackoffCmd == NULL ||
		pTxPwrLimitData == NULL)
		return;

	for (ucIdx = 0; ucIdx < prTxBfBackoffCmd->ucNum; ucIdx++) {
		pChTxBfBackoff =
			&(prTxBfBackoffCmd->rChannelTxBfBackoff[ucIdx]);
		cChIdx = rlmDomainTxPwrLimitGetChIdx(pTxPwrLimitData,
			pChTxBfBackoff->ucCentralCh);

		if (cChIdx == -1) {
			DBGLOG(RLM, ERROR,
				"Invalid ch idx found while assigning values\n");
			return;
		}
		pChTxPwrLimit = &pTxPwrLimitData->rChannelTxPwrLimit[cChIdx];

		kalMemCopy(&pChTxBfBackoff->acTxBfBackoff,
			pChTxPwrLimit->rTxBfBackoff,
			sizeof(pChTxBfBackoff->acTxBfBackoff));
	}

	for (ucIdx = 0; ucIdx < prTxBfBackoffCmd->ucNum; ucIdx++) {
		pChTxBfBackoff =
			&(prTxBfBackoffCmd->rChannelTxBfBackoff[ucIdx]);

		DBGLOG(RLM, ERROR,
			"ch %d TxBf backoff 2to1 %d\n",
			pChTxBfBackoff->ucCentralCh,
			pChTxBfBackoff->acTxBfBackoff[0]);

	}

}

void rlmDomainTxPwrSendTxBfBackoffCmd(
	struct ADAPTER *prAdapter,
	struct CMD_TXPWR_TXBF_SET_BACKOFF *prCmd)
{
	uint32_t rStatus;
	uint32_t u4SetQueryInfoLen;
	uint32_t u4SetCmdSize = sizeof(struct CMD_TXPWR_TXBF_SET_BACKOFF);

	u4SetQueryInfoLen = u4SetCmdSize;
	/* Update tx max. power info to chip */
	rStatus = wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SET_TXBF_BACKOFF,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		u4SetQueryInfoLen,
		(uint8_t *) prCmd,
		NULL,
		0);

}

#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
void
rlmDomainSendTxPwrLimitPerRateCmd_6G(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE
		*prTxPwrLimitPerRateCmd_6G;
	uint32_t rTxPwrLimitPerRateCmdSize_6G = 0;

	uint32_t u4SetCmdTableMaxSize = 0;
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_PER_RATE);
	uint8_t ch_cnt = TX_PWR_LIMIT_6G_CH_NUM;
	uint8_t ch_idx = 0;
	const int8_t *prChannelList = &gTx_Pwr_Limit_6g_Ch[0];

	u4SetCmdTableMaxSize = u4SetCountryTxPwrLimitCmdSize +
	ch_cnt * u4ChPwrLimitSize;

	rTxPwrLimitPerRateCmdSize_6G = u4SetCmdTableMaxSize;
	prTxPwrLimitPerRateCmd_6G =
	kalMemAlloc(u4SetCmdTableMaxSize, VIR_MEM_TYPE);

	if (!prTxPwrLimitPerRateCmd_6G) {
		DBGLOG(RLM, ERROR,
		"%s no buf to send cmd\n", __func__);
		return;
	}

	/*initialize tx pwr table*/
	kalMemSet(prTxPwrLimitPerRateCmd_6G->rChannelPowerLimit, MAX_TX_POWER,
		ch_cnt * u4ChPwrLimitSize);

	prTxPwrLimitPerRateCmd_6G->ucNum = ch_cnt;
	prTxPwrLimitPerRateCmd_6G->eBand = 0x3;
	prTxPwrLimitPerRateCmd_6G->u4CountryCode = rlmDomainGetCountryCode();

	for (ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
	prTxPwrLimitPerRateCmd_6G->rChannelPowerLimit[ch_idx].u1CentralCh =
	prChannelList[ch_idx];
	}

	rlmDomainTxPwrLimitPerRateSetValues(ucVersion,
		prTxPwrLimitPerRateCmd_6G,
		pTxPwrLimitData);

	rlmDomainTxPwrLimitSendPerRateCmd_6G(prAdapter,
		prTxPwrLimitPerRateCmd_6G);

	cnmMemFree(prAdapter, prTxPwrLimitPerRateCmd_6G);

}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

void
rlmDomainSendTxPwrLimitPerRateCmd(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	uint8_t band_idx = 0;
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE
		*prTxPwrLimitPerRateCmd[KAL_NUM_BANDS] = {0};
	uint32_t prTxPwrLimitPerRateCmdSize[KAL_NUM_BANDS] = {0};

	if (rlmDomainInitTxPwrLimitPerRateCmd(
		prAdapter, prTxPwrLimitPerRateCmd,
		prTxPwrLimitPerRateCmdSize) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	rlmDomainTxPwrLimitPerRateSetValues(ucVersion,
		prTxPwrLimitPerRateCmd[KAL_BAND_2GHZ], pTxPwrLimitData);
	rlmDomainTxPwrLimitPerRateSetValues(ucVersion,
		prTxPwrLimitPerRateCmd[KAL_BAND_5GHZ], pTxPwrLimitData);
	rlmDomainTxPwrLimitSendPerRateCmd(prAdapter, prTxPwrLimitPerRateCmd);

error:
	for (band_idx = 0; band_idx < KAL_NUM_BANDS; band_idx++)
		if (prTxPwrLimitPerRateCmd[band_idx])
			kalMemFree(prTxPwrLimitPerRateCmd[band_idx],
				VIR_MEM_TYPE,
				prTxPwrLimitPerRateCmdSize[band_idx]);
}

void
rlmDomainSendTxBfBackoffCmd(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	struct CMD_TXPWR_TXBF_SET_BACKOFF
		*prTxBfBackoffCmd = NULL;

	if (rlmDomainInitTxBfBackoffCmd(
		prAdapter, &prTxBfBackoffCmd) !=
		WLAN_STATUS_SUCCESS)
		goto error;

	rlmDomainTxPwrTxBfBackoffSetValues(
		ucVersion, prTxBfBackoffCmd, pTxPwrLimitData);
	rlmDomainTxPwrSendTxBfBackoffCmd(prAdapter, prTxBfBackoffCmd);

error:

	if (prTxBfBackoffCmd)
		cnmMemFree(prAdapter, prTxBfBackoffCmd);
}
#endif/*CFG_SUPPORT_SINGLE_SKU*/

void rlmDomainSendPwrLimitCmd_V2(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t ucVersion = 0;
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData = NULL;

	DBGLOG(RLM, INFO, "rlmDomainSendPwrLimitCmd()\n");
	pTxPwrLimitData = rlmDomainInitTxPwrLimitData(prAdapter);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Init TxPwrLimitData failed\n");
		goto error;
	}

	/*
	 * Get Max Tx Power from MT_TxPwrLimit.dat
	 */
	if (!rlmDomainGetTxPwrLimit(rlmDomainGetCountryCode(),
		&ucVersion,
		prAdapter->prGlueInfo,
		pTxPwrLimitData)) {
		DBGLOG(RLM, ERROR,
			"Load TxPwrLimitData failed\n");
		goto error;
	}

	/* Prepare to send CMD to FW */
	if (ucVersion == 0) {
		rlmDomainSendTxPwrLimitCmd(prAdapter,
			ucVersion, pTxPwrLimitData);
	 } else if (ucVersion == 1 || ucVersion == 2) {
		rlmDomainSendTxPwrLimitPerRateCmd(prAdapter,
			ucVersion, pTxPwrLimitData);

		if (g_bTxBfBackoffExists)
			rlmDomainSendTxBfBackoffCmd(prAdapter,
				ucVersion, pTxPwrLimitData);

	} else {
		DBGLOG(RLM, WARN, "Unsupported TxPwrLimit dat version %u\n",
			ucVersion);
	}

#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
	if (pTxPwrLimitData && pTxPwrLimitData->rChannelTxPwrLimit)
		kalMemFree(pTxPwrLimitData->rChannelTxPwrLimit, VIR_MEM_TYPE,
			sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			pTxPwrLimitData->ucChNum);

	if (pTxPwrLimitData)
		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));

	pTxPwrLimitData = rlmDomainInitTxPwrLimitData_6G(prAdapter);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Init TxPwrLimitData 6G failed\n");
		goto error;
	}

	/*
	 * Get Max Tx Power from MT_TxPwrLimit_6G.dat
	 */
	prAdapter->chip_info->prTxPwrLimitFile = "TxPwrLimit6G_MT79x1.dat";
	if (!rlmDomainGetTxPwrLimit(rlmDomainGetCountryCode(),
		&ucVersion,
		prAdapter->prGlueInfo,
		pTxPwrLimitData)) {
		DBGLOG(RLM, ERROR,
			"Load TxPwrLimitData failed\n");
		goto error;
	}

	/* Prepare to send CMD to FW */
	if (ucVersion == 2) {
		rlmDomainSendTxPwrLimitPerRateCmd_6G(prAdapter,
			ucVersion, pTxPwrLimitData);
	} else {
		DBGLOG(RLM, WARN,
		"Unsupported TxPwrLimit6G_MT7961.dat version %u\n",
		ucVersion);
	}

	/* restore back to default value */
	prAdapter->chip_info->prTxPwrLimitFile = "TxPwrLimit_MT79x1.dat";
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

error:
	if (pTxPwrLimitData && pTxPwrLimitData->rChannelTxPwrLimit)
		kalMemFree(pTxPwrLimitData->rChannelTxPwrLimit, VIR_MEM_TYPE,
			sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			pTxPwrLimitData->ucChNum);

	if (pTxPwrLimitData)
		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));

	/* restore back to default value */
	prAdapter->chip_info->prTxPwrLimitFile = "TxPwrLimit_MT79x1.dat";
#endif
}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control: begin ********************************************/

char *txPwrGetString(char **pcContent, char *delim)
{
	char *result = NULL;

	if (pcContent == NULL)
		return NULL;

	result = kalStrSep(pcContent, delim);
	if ((pcContent == NULL) || (result == NULL) ||
	    ((result != NULL) && (kalStrLen(result) == 0)))
		return NULL;

	return result;
}

uint32_t txPwrParseNumber(char **pcContent, char *delim, uint8_t *op,
			  int8_t *value) {
	u_int8_t fgIsNegtive = FALSE;
	char *pcTmp = NULL;
	char *result = NULL;

	if ((!pcContent) || (*pcContent == NULL))
		return -1;

	if (**pcContent == '-') {
		fgIsNegtive = TRUE;
		*pcContent = *pcContent + 1;
	}
	pcTmp = *pcContent;

	result = kalStrSep(pcContent, delim);
	if (result == NULL) {
		return -1;
	} else if ((result != NULL) && (kalStrLen(result) == 0)) {
		if (fgIsNegtive)
			return -1;
		*value = 0;
		*op = 0;
	} else {
		if (kalkStrtou8(pcTmp, 0, value) != 0) {
			DBGLOG(RLM, ERROR,
			       "parse number error: invalid number [%s]\n",
			       pcTmp);
			return -1;
		}
		if (fgIsNegtive)
			*op = 2;
		else
			*op = 1;
	}

	return 0;
}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
void txPwrParseTagDump(struct TX_PWR_CTRL_ELEMENT *pRecord)
{
	uint32_t i = 0, j = 0;

	for (i = 0; i < POWER_ANT_TAG_NUM; i++) {
		DBGLOG(RLM, TRACE, "Tag id (%d) :", i);
		for (j = 0; j < POWER_ANT_NUM; j++) {
			DBGLOG(RLM, TRACE, "[%d]",
				pRecord->aiPwrAnt[i].aiPwrAnt2G4[j]);
			DBGLOG(RLM, TRACE, "[%d]",
				pRecord->aiPwrAnt[i].aiPwrAnt5GB1[j]);
			DBGLOG(RLM, TRACE, "[%d]",
				pRecord->aiPwrAnt[i].aiPwrAnt5GB2[j]);
			DBGLOG(RLM, TRACE, "[%d]",
				pRecord->aiPwrAnt[i].aiPwrAnt5GB3[j]);
			DBGLOG(RLM, TRACE, "[%d]",
				pRecord->aiPwrAnt[i].aiPwrAnt5GB4[j]);
		}
	}
}

int32_t txPwrParseTagXXXT(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord,
	enum ENUM_POWER_ANT_TAG eTag) {

	char *pCurent = NULL, *pContent = NULL;
	uint8_t i = 0, j = 0;
	uint8_t ucBandIdx = 0, ucAntIdx = 0;
	uint8_t op = 0, value = 0;
	int8_t backoff = 0;

	if (!pStart || !pEnd || !pRecord)
		return -1;

	if (cTagParaNum != (POWER_ANT_BAND_NUM * POWER_ANT_NUM))
		return -1;

	DBGLOG(RLM, TRACE, "pase tag Para (%s) to aiPwrAnt[%u]",
		pStart, eTag);

	pCurent = pStart;

	for (i = 0; i < cTagParaNum; i++) {

		if (!pCurent || pCurent >= pEnd)
			break;

		pContent = txPwrGetString(&pCurent, ",");

		if (!pContent) {
			DBGLOG(RLM, TRACE, "tag parameter format error: %s\n",
			       pStart);
			break;
		}

		if (txPwrParseNumber(&pContent, ",", &op, &value)) {
			DBGLOG(RLM, TRACE, "parse parameter error: %s\n",
			       pContent);
			break;
		}

		backoff = (op == 1) ? value : (0 - value);
		if (backoff < PWR_CFG_BACKOFF_MIN)
			backoff = PWR_CFG_BACKOFF_MIN;

		if (backoff > PWR_CFG_BACKOFF_MAX)
			backoff = PWR_CFG_BACKOFF_MAX;

		/* (wf02g, wf05g,  wf12g, wf15g, ...  wfx2g, wfx5g)
		 * i is parameter index, start from 0.
		 * i % POWER_ANT_BAND_NUM : 0 for 5g  1 for 2g
		 * i / POWER_ANT_BAND_NUM : is wf idx,  wfx
		 *
		 * for example:
		 * i = 3, it means the fourth parameter.
		 * i%POWER_ANT_BAND_NUM is 1 , so means 2g
		 * i/POWER_ANT_BAND_NUM is 1, so means wf1
		 * the i parameter is wf12g.
		 */
		ucBandIdx = i%POWER_ANT_BAND_NUM;
		ucAntIdx = i/POWER_ANT_BAND_NUM;
		switch (ucBandIdx) {
		case POWER_ANT_2G4_BAND:
			pRecord->aiPwrAnt[eTag].aiPwrAnt2G4[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_5G_BAND1:
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB1[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_5G_BAND2:
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB2[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_5G_BAND3:
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB3[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_5G_BAND4:
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB4[ucAntIdx]
				= backoff;
			break;
		default:
			DBGLOG(RLM, INFO, "Never happen: %s\n",
				pStart);
			return -1;
		}

		if (pCurent >= pEnd)
			break;

	}

	if (i != cTagParaNum) {
		DBGLOG(RLM, INFO, "parameter number error: %s\n",
			       pStart);
		for (j = 0; j < POWER_ANT_NUM; j++) {
			pRecord->aiPwrAnt[eTag].aiPwrAnt2G4[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB1[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB2[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB3[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB4[j] = 0;
		}
		return -1;
	}

	DBGLOG(RLM, TRACE, "[Success] Dump aiPwrAnt[%u] para: ", eTag);
	for (j = 0; j < POWER_ANT_NUM; j++)
		DBGLOG(RLM, TRACE, "[%d][%d][%d][%d][%d]",
			       pRecord->aiPwrAnt[eTag].aiPwrAnt2G4[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt5GB1[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt5GB2[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt5GB3[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt5GB4[j]);
	DBGLOG(RLM, TRACE, "\n");

	return 0;
}

int32_t txPwrParseTagMimo1T(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord) {
	return txPwrParseTagXXXT(pStart, pEnd,
		cTagParaNum, pRecord, POWER_ANT_MIMO_1T);
}

int32_t txPwrParseTagMimo2T(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord){
	return txPwrParseTagXXXT(pStart, pEnd,
		cTagParaNum, pRecord, POWER_ANT_MIMO_2T);
}

int32_t txPwrParseTagAllT(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord){
	return txPwrParseTagXXXT(pStart, pEnd,
		cTagParaNum, pRecord, POWER_ANT_ALL_T);
}

int32_t txPwrParseTagAllT6G(
	char *pStart, char *pEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord){

	char *pCurent = NULL, *pContent = NULL;
	uint8_t i = 0, j = 0;
	uint8_t ucBandIdx = 0, ucAntIdx = 0;
	uint8_t op = 0, value = 0;
	int8_t backoff = 0;
	enum ENUM_POWER_ANT_TAG eTag = POWER_ANT_ALL_T_6G;

	if (!pStart || !pEnd || !pRecord)
		return -1;

	if (cTagParaNum != (POWER_ANT_6G_BAND_NUM * POWER_ANT_NUM))
		return -1;

	DBGLOG(RLM, TRACE, "pase tag Para (%s) to aiPwrAnt[%u]",
		pStart, eTag);

	pCurent = pStart;

	for (i = 0; i < cTagParaNum; i++) {

		if (!pCurent || pCurent >= pEnd)
			break;

		pContent = txPwrGetString(&pCurent, ",");

		if (!pContent) {
			DBGLOG(RLM, TRACE, "tag parameter format error: %s\n",
			       pStart);
			break;
		}

		if (txPwrParseNumber(&pContent, ",", &op, &value)) {
			DBGLOG(RLM, TRACE, "parse parameter error: %s\n",
			       pContent);
			break;
		}

		backoff = (op == 1) ? value : (0 - value);
		if (backoff < PWR_CFG_BACKOFF_MIN)
			backoff = PWR_CFG_BACKOFF_MIN;

		if (backoff > PWR_CFG_BACKOFF_MAX)
			backoff = PWR_CFG_BACKOFF_MAX;

		ucBandIdx = i%POWER_ANT_6G_BAND_NUM;
		ucAntIdx = i/POWER_ANT_6G_BAND_NUM;
		switch (ucBandIdx) {
		case POWER_ANT_6G_BAND1:
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB1[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_6G_BAND2:
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB2[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_6G_BAND3:
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB3[ucAntIdx]
				= backoff;
			break;
		case POWER_ANT_6G_BAND4:
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB4[ucAntIdx]
				= backoff;
			break;
		default:
			DBGLOG(RLM, INFO, "Never happen: %s\n",
				pStart);
			return -1;
		}

		if (pCurent >= pEnd)
			break;

	}

	if (i != cTagParaNum) {
		DBGLOG(RLM, INFO, "parameter number error: %s\n",
			       pStart);
		for (j = 0; j < POWER_ANT_NUM; j++) {
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB1[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB2[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB3[j] = 0;
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB4[j] = 0;
		}
		return -1;
	}

	DBGLOG(RLM, TRACE, "[Success] Dump aiPwrAnt[%u] para: ", eTag);
	for (j = 0; j < POWER_ANT_NUM; j++)
		DBGLOG(RLM, TRACE, "[%d][%d][%d][%d]",
			       pRecord->aiPwrAnt[eTag].aiPwrAnt6GB1[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt6GB2[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt6GB3[j],
			       pRecord->aiPwrAnt[eTag].aiPwrAnt6GB4[j]);
	DBGLOG(RLM, TRACE, "\n");

	return 0;
}

int32_t txPwrParseTag(char *pTagStart, char *pTagEnd,
	struct TX_PWR_CTRL_ELEMENT *pRecord) {
	uint8_t i = 0;
	int32_t ret = 0;
	char *pCurent = NULL, *pNext = NULL;

	if (!pTagStart || !pTagEnd || !pRecord)
		return -1;

	DBGLOG(RLM, TRACE, "Pase new tag %s\n", pTagStart);

	pCurent = pTagStart;

	pNext = txPwrGetString(&pCurent, ",");
	if (!pNext) {
		DBGLOG(RLM, INFO,
			   "Pase tag name error, %s\n",
			   pTagStart);
		return -1;
	}

	for (i = 0;
		i < sizeof(g_auTxPwrTagTable)/sizeof(struct TX_PWR_TAG_TABLE);
		i++){

		DBGLOG(RLM, TRACE,
				"Parse tag name [%s] handler name[%s]\n", pNext,
				g_auTxPwrTagTable[i].arTagNames);

		if (kalStrCmp(pNext,
			g_auTxPwrTagTable[i].arTagNames) == 0){

			if (g_auTxPwrTagTable[i].pfnParseTagParaHandler
				== NULL) {
				DBGLOG(RLM, TRACE,
			       "No parse handler: tag name [%s]\n",
			       pNext);
				return -1;
			}
			ret = g_auTxPwrTagTable[i].pfnParseTagParaHandler(
				pCurent, pTagEnd,
				g_auTxPwrTagTable[i].ucTagParaNum,
				pRecord);

			txPwrParseTagDump(pRecord);

			return ret;
		}
	}

	DBGLOG(RLM, INFO,
		"Undefined tag name [%s]\n",
		pCurent);

	return -1;
}

int32_t txPwrOnPreParseAppendTag(
	struct TX_PWR_CTRL_ELEMENT *pRecord) {

	if (!pRecord)
		return -1;

	/* init aiPwrAnt to 0 for tag :
	  * MIMO_1T/MIMO_2T/ALL_T/ALL_T_6G
	  */
	kalMemSet(&(pRecord->aiPwrAnt[0]), 0,
		POWER_ANT_TAG_NUM *
		sizeof(struct TX_PWR_CTRL_ANT_SETTING));

	/* Init pRecord for your new tag*/

	return 0;
}


int32_t txPwrParseAppendTag(char *pcStart,
	char *pcEnd, struct TX_PWR_CTRL_ELEMENT *pRecord) {

	char *pcContCur = NULL, *pcContTemp = NULL;
	uint32_t ucTagCount1 = 0, ucTagCount2 = 0;
	uint8_t i = 0;

	if (!pcStart || !pcEnd || !pRecord) {
		DBGLOG(RLM, INFO, "%p-%p-%p!\n",
			pcStart, pcEnd, pRecord);
		return -1;
	}

	/* parse power limit append tag to list. */
	pcContCur = pcStart;

	pcContTemp = pcContCur;

	while (pcContTemp <= pcEnd) {
		if ((*pcContTemp) == '<')
			ucTagCount1++;

		if ((*pcContTemp) == '>')
			ucTagCount2++;
		pcContTemp++;
	}

	if (ucTagCount1 != ucTagCount2) {
		DBGLOG(RLM, INFO,
		       "Wrong tag append %s!\n", pcStart);
		return -1;
	}

	/* No tag need parsing */
	if (ucTagCount1 == 0)
		return 0;

	DBGLOG(RLM, TRACE, "New config total %u tag append %s !\n",
		ucTagCount1, pcStart);

	for (i = 0; i < ucTagCount1; i++) {

		pcContTemp = txPwrGetString(&pcContCur, "<");

		if (!pcContCur || pcContCur > pcEnd) {
			DBGLOG(RLM, INFO,
		       "No content after '<' !\n");
			return -1;
		}

		/* verify there is > symbol */
		pcContTemp = txPwrGetString(&pcContCur, ">");
		if (!pcContTemp) {
			DBGLOG(RLM, INFO,
		       "Can not find content after '<' !\n");
			return -1;
		}

		if (!pcContCur || pcContCur > pcEnd) {
			DBGLOG(RLM, INFO,
			   "No content after '>' !\n");
			return -1;
		}

		if (txPwrParseTag(pcContTemp, pcContCur - 1, pRecord)) {
			DBGLOG(RLM, INFO,
			   "Parse one tag fail !\n");
			return -1;
		}

		DBGLOG(RLM, TRACE,
			   "Parse tag (%u/%u) success!\n", i + 1, ucTagCount1);
	}

	return 0;
}
#endif
void txPwrOperate(enum ENUM_TX_POWER_CTRL_TYPE eCtrlType,
		  int8_t *operand1, int8_t *operand2)
{
	switch (eCtrlType) {
	case PWR_CTRL_TYPE_WIFION_POWER_LEVEL:
	case PWR_CTRL_TYPE_IOCTL_POWER_LEVEL:
		if (*operand1 > *operand2)
			*operand1 = *operand2;
		break;
	case PWR_CTRL_TYPE_WIFION_POWER_OFFSET:
	case PWR_CTRL_TYPE_IOCTL_POWER_OFFSET:
		*operand1 += *operand2;
		break;
	default:
		break;
	}

	if (*operand1 > MAX_TX_POWER)
		*operand1 = MAX_TX_POWER;
	else if (*operand1 < MIN_TX_POWER)
		*operand1 = MIN_TX_POWER;
}

uint32_t txPwrArbitrator(enum ENUM_TX_POWER_CTRL_TYPE eCtrlType,
			 void *pvBuf,
			 struct TX_PWR_CTRL_CHANNEL_SETTING *prChlSetting,
			 enum ENUM_PWR_LIMIT_TYPE eType)
{
	struct CMD_CHANNEL_POWER_LIMIT *prPwrLimit;
	struct CMD_CHANNEL_POWER_LIMIT_HE *prPwrLimitHE;
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prPwrLimitHEBW160;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prPwrLimitEHT;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
	uint8_t rateIdxHe, rateIdx;
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct CMD_CHANNEL_POWER_LIMIT_6E *prPwrLimit6E;
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prCmdPwrLimtLegacy_6G;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prPwrLimitEHT_6G;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	int8_t *prRateOfs;
	uint8_t u1Idx;

	if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
		prPwrLimitHE = (struct CMD_CHANNEL_POWER_LIMIT_HE *) pvBuf;
		prRateOfs = &prPwrLimitHE->cPwrLimitRU26L;

		for (rateIdxHe = PWR_LIMIT_RU26_L;
			rateIdxHe < PWR_LIMIT_HE_NUM ; rateIdxHe++) {
			if (prChlSetting->opHE[rateIdxHe]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdxHe,
				&prChlSetting->i8PwrLimitHE[rateIdxHe]);
			}
		}
	} else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
		prPwrLimitHEBW160 =
			(struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *) pvBuf;
		prRateOfs = &prPwrLimitHEBW160->cPwrLimitRU26L;

		for (rateIdxHe = PWR_LIMIT_RU26_L;
			rateIdxHe < PWR_LIMIT_HE_BW160_NUM ; rateIdxHe++) {
			if (prChlSetting->opHE[rateIdxHe]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdxHe,
				&prChlSetting->i8PwrLimitHE[rateIdxHe]);
			}
		}
	}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
				eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
		prPwrLimitEHT = (struct CMD_CHANNEL_POWER_LIMIT_EHT *) pvBuf;
		prRateOfs = &prPwrLimitEHT->cPwrLimitEHT26L;
		for (rateIdx = PWR_LIMIT_EHT26_L;
			rateIdx < PWR_LIMIT_EHT_NUM ; rateIdx++) {
			if (prChlSetting->opEHT[rateIdx]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdx,
					&prChlSetting->i8PwrLimitEHT[rateIdx]);
			}
		}
	}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
				eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
				eType == PWR_LIMIT_TYPE_COMP_6E_3) {
		prPwrLimit6E = (struct CMD_CHANNEL_POWER_LIMIT_6E *) pvBuf;
		prRateOfs = &prPwrLimit6E->cPwrLimitRU26L;
		for (rateIdx = PWR_LIMIT_RU26_L;
			rateIdx < PWR_LIMIT_6E_NUM ; rateIdx++) {
			if (prChlSetting->op6E[rateIdx]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdx,
					&prChlSetting->i8PwrLimit6E[rateIdx]);
			}
		}
	} else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
		eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
		prCmdPwrLimtLegacy_6G =
			(struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *) pvBuf;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
		prRateOfs = &prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L;
		u1Idx = PWR_LIMIT_CCK_L;
#else
		prRateOfs = &prCmdPwrLimtLegacy_6G->cPwrLimitCCK;
		u1Idx = PWR_LIMIT_CCK;
#endif
		for (rateIdx = u1Idx;
			rateIdx < PWR_LIMIT_LEGACY_6G_NUM; rateIdx++) {
			if (prChlSetting->opLegacy_6G[rateIdx]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdx,
				&prChlSetting->i8PwrLimitLegacy_6G[rateIdx]);
			}
		}
	}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (eType >=
				PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
				eType <=
				PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
		prPwrLimitEHT_6G =
			(struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *) pvBuf;
		prRateOfs = &prPwrLimitEHT_6G->cPwrLimitEHT26L;
		for (rateIdx = PWR_LIMIT_EHT_6G_26_L;
			rateIdx < PWR_LIMIT_EHT_6G_NUM ; rateIdx++) {
			if (prChlSetting->opEHT_6G[rateIdx]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdx,
				&prChlSetting->i8PwrLimitEHT_6G[rateIdx]);
			}
		}
	}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
		eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
		prPwrLimit = (struct CMD_CHANNEL_POWER_LIMIT *) pvBuf;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
		prRateOfs = &prPwrLimit->cPwrLimitCCK_L;
		u1Idx = PWR_LIMIT_CCK_L;
#else
		prRateOfs = &prPwrLimit->cPwrLimitCCK;
		u1Idx = PWR_LIMIT_CCK;
#endif
		for (rateIdx = u1Idx;
			rateIdx < PWR_LIMIT_NUM ; rateIdx++) {
			if (prChlSetting->op[rateIdx]
				!= PWR_CTRL_TYPE_NO_ACTION) {
				txPwrOperate(eCtrlType, prRateOfs + rateIdx,
				&prChlSetting->i8PwrLimit[rateIdx]);
			}
		}
	}
	return 0;
}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
uint8_t txPwrIsAntTagSet(
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	enum ENUM_POWER_ANT_TAG tag) {
	uint8_t i = 0;

	if (tag >= POWER_ANT_TAG_NUM)
		return 0;
	for (i = 0; i < POWER_ANT_NUM; i++) {
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt2G4[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt5GB1[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt5GB2[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt5GB3[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt5GB4[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt6GB1[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt6GB2[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt6GB3[i] != 0)
			return 1;
		if (prCurElement->aiPwrAnt[tag].aiPwrAnt6GB4[i] != 0)
			return 1;
	}
	return 0;
}

uint8_t txPwrIsAntTagNeedApply(
	struct TX_PWR_CTRL_ELEMENT *prCurElement)
{
	uint8_t fgAllTSet = 0, fg1TSet = 0, fg2TSet = 0;

	if (!prCurElement)
		return 0;

	fgAllTSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_ALL_T);
	fg1TSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_MIMO_1T);
	fg2TSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_MIMO_2T);

	if ((fgAllTSet && !fg1TSet && !fg2TSet)
		|| (!fgAllTSet && (fg1TSet || fg2TSet)))
		/* only MIMO_1T or MIMO_2T */
		return 1;

	DBGLOG(RLM, TRACE, "No need apply [%u-%u-%u]\n",
		fgAllTSet, fg1TSet, fg2TSet);

	return 0;
}

uint32_t txPwrCheckPwrAntNum(
	enum ENUM_POWER_ANT_TAG tag, uint8_t u1Num)
{
	if (u1Num >= POWER_LIMIT_ANT_CONFIG_NUM) {
		DBGLOG(RLM, ERROR,
			"PwrLimit ant element idx invalid,tag(%d)num(%d)\n",
			tag, u1Num);
		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t txPwrApplyPwrAnt(
	uint8_t u1Idx,
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandNum,
	uint8_t u1AntNum,
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt,
	struct TX_PWR_CTRL_ELEMENT *prCurElem)
{
	uint8_t u1BandIdx = 0, u1AntIdx = 0;

	if (txPwrCheckPwrAntNum(tag, u1Idx) != WLAN_STATUS_SUCCESS)
		return u1Idx;

	for (u1BandIdx = 0; u1BandIdx < u1BandNum; u1BandIdx++) {
		for (u1AntIdx = 0; u1AntIdx < u1AntNum; u1AntIdx++) {
			prCmdPwrAnt[u1Idx].cTagIdx = tag;
			prCmdPwrAnt[u1Idx].cBandIdx = u1BandIdx;
			prCmdPwrAnt[u1Idx].cAntIdx = u1AntIdx;

			if (u1BandIdx == POWER_ANT_2G4_BAND) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt2G4[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_5G_BAND1) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt5GB1[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_5G_BAND2) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt5GB2[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_5G_BAND3) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt5GB3[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_5G_BAND4) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt5GB4[u1AntIdx];
			}
			u1Idx++;
		}
	}
	return u1Idx;
}

uint32_t txPwrApplyPwrAnt6G(
	uint8_t u1Idx,
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandNum,
	uint8_t u1AntNum,
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt,
	struct TX_PWR_CTRL_ELEMENT *prCurElem)
{
	uint8_t u1BandIdx = 0, u1AntIdx = 0;

	if (txPwrCheckPwrAntNum(tag, u1Idx) != WLAN_STATUS_SUCCESS)
		return u1Idx;

	for (u1BandIdx = 0; u1BandIdx < u1BandNum; u1BandIdx++) {
		for (u1AntIdx = 0; u1AntIdx < u1AntNum; u1AntIdx++) {
			prCmdPwrAnt[u1Idx].cTagIdx = tag;
			prCmdPwrAnt[u1Idx].cBandIdx = u1BandIdx;
			prCmdPwrAnt[u1Idx].cAntIdx = u1AntIdx;

			if (u1BandIdx == POWER_ANT_6G_BAND1) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt6GB1[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_6G_BAND2) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt6GB2[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_6G_BAND3) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt6GB3[u1AntIdx];
			} else if (u1BandIdx == POWER_ANT_6G_BAND4) {
				prCmdPwrAnt[u1Idx].cValue =
				prCurElem->aiPwrAnt[tag].aiPwrAnt6GB4[u1AntIdx];
			}
			u1Idx++;
		}
	}
	return u1Idx;
}

uint32_t txPwrApplyOneSettingPwrAnt(
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
	struct TX_PWR_CTRL_ELEMENT *prCurElement)
{
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt = NULL;
	uint8_t u1NextIdx = 0;
	uint8_t fgAllTSet = 0, fg1TSet = 0, fg2TSet = 0;
	uint8_t fgAllT6GSet = 0;

	enum ENUM_PWR_LIMIT_TYPE eType;

	if (!prCurElement || !prCmd)
		return 1;

	eType = prCmd->ucLimitType;

	if (eType != PWR_LIMIT_TYPE_COMP_ANT_V2)
		return 0;

	prCmdPwrAnt = &(prCmd->u.rChPwrLimtAnt[0]);

	fgAllTSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_ALL_T);
	fg1TSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_MIMO_1T);
	fg2TSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_MIMO_2T);
	fgAllT6GSet = txPwrIsAntTagSet(prCurElement, POWER_ANT_ALL_T_6G);

	/* check scenario reasonable */
	if (!(((fgAllTSet || fgAllT6GSet) && !fg1TSet && !fg2TSet)
		|| (!(fgAllTSet || fgAllT6GSet) && (fg1TSet || fg2TSet)))) {
		return 0;
	}
	/* TODO : refactory when fg1TSet/fg2TSet online */
	prCmd->ucNum = 0;

	if (fgAllTSet) {
		u1NextIdx = txPwrApplyPwrAnt(
			u1NextIdx, POWER_ANT_ALL_T,
			POWER_ANT_BAND_NUM, POWER_ANT_NUM,
			prCmdPwrAnt, prCurElement);
	}

	if (fg1TSet) {
		u1NextIdx = txPwrApplyPwrAnt(
			u1NextIdx, POWER_ANT_MIMO_1T,
			POWER_ANT_BAND_NUM, POWER_ANT_NUM,
			prCmdPwrAnt, prCurElement);
	}

	if (fg2TSet) {
		u1NextIdx = txPwrApplyPwrAnt(
			u1NextIdx, POWER_ANT_MIMO_2T,
			POWER_ANT_BAND_NUM, POWER_ANT_NUM,
			prCmdPwrAnt, prCurElement);
	}

	if (fgAllT6GSet) {
		u1NextIdx = txPwrApplyPwrAnt6G(
			u1NextIdx, POWER_ANT_ALL_T_6G,
			POWER_ANT_6G_BAND_NUM, POWER_ANT_NUM,
			prCmdPwrAnt, prCurElement);
	}

	prCmd->ucNum = u1NextIdx;
	return 0;
}
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */
uint32_t txPwrApplyOneSetting(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
			      struct TX_PWR_CTRL_ELEMENT *prCurElement,
			      uint8_t *bandedgeParam)
{
	struct CMD_CHANNEL_POWER_LIMIT *prCmdPwrLimit = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_HE *prCmdPwrLimitHE = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prCmdPwrLimitHEBW160 = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prCmdPwrLimitEHT = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct CMD_CHANNEL_POWER_LIMIT_6E *prCmdPwrLimit6E = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prCmdPwrLimtLegacy_6G = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prCmdPwrLimitEHT_6G = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
	u_int8_t fgDoArbitrator6E;
#endif /* CFG_SUPPORT_WIFI_6G */
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChlSetting = NULL;
	uint8_t i = 0, j = 0, channel = 0, channel2 = 0, channel3 = 0;
	u_int8_t fgDoArbitrator;
	enum ENUM_PWR_LIMIT_TYPE eType;

	ASSERT(prCmd);

	eType = prCmd->ucLimitType;

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	if (eType == PWR_LIMIT_TYPE_COMP_ANT_V2) {
		txPwrApplyOneSettingPwrAnt(prCmd, prCurElement);
		return 0;
	}
#endif

	for (i = 0; i < prCmd->ucNum; i++) {

		if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
			prCmdPwrLimitHE = &prCmd->u.rChPwrLimtHE[i];
			channel = prCmdPwrLimitHE->ucCentralCh;
		} else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
			prCmdPwrLimitHEBW160 = &prCmd->u.rChPwrLimtHEBW160[i];
			channel = prCmdPwrLimitHEBW160->ucCentralCh;
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
			prCmdPwrLimitEHT = &prCmd->u.rChPwrLimtEHT[i];
			channel = prCmdPwrLimitEHT->ucCentralCh;
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
			prCmdPwrLimit6E = &prCmd->u.rChPwrLimt6E[i];
			channel = prCmdPwrLimit6E->ucCentralCh;
		} else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
			eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
			prCmdPwrLimtLegacy_6G =
				&prCmd->u.rChPwrLimtLegacy_6G[i];
			channel = prCmdPwrLimtLegacy_6G->ucCentralCh;
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
			prCmdPwrLimitEHT_6G = &prCmd->u.rChPwrLimtEHT_6G[i];
			channel = prCmdPwrLimitEHT_6G->ucCentralCh;
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
		else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
			eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
			prCmdPwrLimit = &prCmd->u.rChannelPowerLimit[i];
			channel = prCmdPwrLimit->ucCentralCh;
		}

		for (j = 0; j < prCurElement->settingCount; j++) {
			prChlSetting = &prCurElement->rChlSettingList[j];
			channel2 = prChlSetting->channelParam[0];
			channel3 = prChlSetting->channelParam[1];
			fgDoArbitrator = FALSE;
#if (CFG_SUPPORT_WIFI_6G == 1)
			fgDoArbitrator6E = FALSE;
#endif
			switch (prChlSetting->eChnlType) {
				case PWR_CTRL_CHNL_TYPE_NORMAL: {
					if (channel == channel2)
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_ALL: {
					fgDoArbitrator = TRUE;
#if (CFG_SUPPORT_WIFI_6G == 1)
					fgDoArbitrator6E = TRUE;
#endif
					break;
				}
				case PWR_CTRL_CHNL_TYPE_RANGE: {
					if ((channel >= channel2) &&
					    (channel <= channel3))
						fgDoArbitrator = TRUE;

					break;
				}
				case PWR_CTRL_CHNL_TYPE_2G4: {
					if (channel <= 14)
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_5G: {
					if (channel > 14)
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_BANDEDGE_2G4: {
					if ((channel == *bandedgeParam) ||
					    (channel == *(bandedgeParam + 1)))
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_BANDEDGE_5G: {
					if ((channel == *(bandedgeParam + 2)) ||
					    (channel == *(bandedgeParam + 3)))
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_5G_BAND1: {
					if ((channel >=
						g_rRlmSubBand[1].ucStartCh) &&
						(channel <=
						g_rRlmSubBand[1].ucEndCh))
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_5G_BAND2: {
					if ((channel >=
						g_rRlmSubBand[2].ucStartCh) &&
						(channel <=
						g_rRlmSubBand[2].ucEndCh))
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_5G_BAND3: {
					if ((channel >=
						g_rRlmSubBand[3].ucStartCh) &&
						(channel <=
						g_rRlmSubBand[3].ucEndCh))
						fgDoArbitrator = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_5G_BAND4: {
					if ((channel >=
						g_rRlmSubBand[4].ucStartCh) &&
						(channel <=
						g_rRlmSubBand[4].ucEndCh))
						fgDoArbitrator = TRUE;
					break;
				}
#if (CFG_SUPPORT_WIFI_6G == 1)
				case PWR_CTRL_CHNL_TYPE_6G: {
					fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_NORMAL: {
					if (channel == channel2)
						fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_BAND1: {
					if ((channel >=
						UNII5A_LOWER_BOUND) &&
						(channel <=
						UNII5B_UPPER_BOUND))
						fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_BAND2: {
					if ((channel >=
						UNII6_LOWER_BOUND) &&
						(channel <=
						UNII6_UPPER_BOUND))
						fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_BAND3: {
					if ((channel >=
						UNII7A_LOWER_BOUND) &&
						(channel <=
						UNII7B_UPPER_BOUND))
						fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_BAND4: {
					if ((channel >=
						UNII8_LOWER_BOUND) &&
						(channel <=
						UNII8_UPPER_BOUND))
						fgDoArbitrator6E = TRUE;
					break;
				}
				case PWR_CTRL_CHNL_TYPE_6G_RANGE: {
					if ((channel >= channel2) &&
					    (channel <= channel3))
						fgDoArbitrator6E = TRUE;

					break;
				}
#endif
			}

			if (fgDoArbitrator) {
				if (eType == PWR_LIMIT_TYPE_COMP_11AX)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimitHE,
							prChlSetting,
							eType);
				else if (eType ==
					PWR_LIMIT_TYPE_COMP_11AX_BW160)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimitHEBW160,
							prChlSetting,
							eType);
				else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
					eType == PWR_LIMIT_TYPE_COMP_11AC_V2)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimit,
							prChlSetting,
							eType);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimitEHT,
							prChlSetting,
							eType);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
			}

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (fgDoArbitrator6E) {
				if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimit6E,
							prChlSetting,
							eType);
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimtLegacy_6G,
							prChlSetting,
							eType);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
				else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <= PWR_LIMIT_TYPE_COMP_11BE_6G_6)
					txPwrArbitrator(prCurElement->eCtrlType,
							prCmdPwrLimitEHT_6G,
							prChlSetting,
							eType);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
			}
#endif /* CFG_SUPPORT_WIFI_6G */
		}

	}
	return 0;
}

uint32_t txPwrCtrlApplySettings(struct ADAPTER *prAdapter,
			struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
			uint8_t *bandedgeParam)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *element = NULL;
	struct LINK *aryprlist[2] = {
		&prAdapter->rTxPwr_DefaultList,
		&prAdapter->rTxPwr_DynamicList
	};
	int32_t i;

	/* show the tx power ctrl applied list */
	txPwrCtrlShowList(prAdapter, 1, "applied list");

	for (i = 0; i < ARRAY_SIZE(aryprlist); i++) {
		LINK_FOR_EACH_SAFE(prCur, prNext, aryprlist[i]) {
			element = LINK_ENTRY(prCur,
					struct TX_PWR_CTRL_ELEMENT, node);
			if (element->fgApplied == TRUE)
				txPwrApplyOneSetting(
					prCmd, element, bandedgeParam);
		}
	}

	return 0;
}

uint32_t txPwrCfgParsingRateTag(char *pcContTmp, uint8_t count)
{
	uint8_t u1Result;

	/* Sanity check */
	if (pcContTmp == NULL)
		return PWR_CFG_RATE_TAG_MISS;

	/* Remove rate tag num */
	count = count - 1;

	if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_LEGACY) == 0 &&
		count == PWR_LIMIT_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_AX160) == 0 &&
		count == PWR_LIMIT_HE_BW160_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_HE) == 0 &&
		count == PWR_LIMIT_HE_BW160_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_EHT) == 0 &&
		count == PWR_LIMIT_EHT_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_EHT;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (kalStrCmp(pcContTmp,
			PWR_CTRL_RATE_TAG_KEY_LEGACY6G) == 0 &&
		count == PWR_LIMIT_LEGACY_6G_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY6G;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_HE6G) == 0 &&
		count == PWR_LIMIT_6E_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE6G;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_EHT6G) == 0 &&
		count == PWR_LIMIT_EHT_6G_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_EHT6G;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G  */
	else
		u1Result = PWR_CFG_RATE_TAG_MISS;

	DBGLOG(RLM, TRACE, "Parse rate tag result[%d]\n", u1Result);
	return u1Result;
}

struct TX_PWR_CTRL_ELEMENT *txPwrCtrlStringToStruct(char *pcContent,
						    u_int8_t fgSkipHeader)
{
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;
	struct TX_PWR_CTRL_CHANNEL_SETTING *prTmpSetting;
	char acTmpName[MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE];
	char *pcContCur = NULL, *pcContCur2 = NULL, *pcContEnd = NULL;
	char *pcContTmp = NULL, *pcContNext = NULL, *pcContOld = NULL;
	char *pcContTag = NULL;
	uint32_t u4MemSize = sizeof(struct TX_PWR_CTRL_ELEMENT);
	uint32_t copySize = 0;
	uint8_t i, j, op, ucSettingCount = 0;
	uint8_t value, value2, count = 0;
	uint8_t ucAppliedWay, ucOperation = 0;
	uint32_t u4RollBackStep = 0;
	char carySeperator[2] = { 0, 0 };
	enum ENUM_PWR_CFG_RATE_TAG u1RateTag = 0;
	char *pacParsePwrAC[PWR_CFG_PRAM_NUM_AC] = {
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
		"CCKL",
		"CCKH",
		"OFDML",
		"OFDMH",
#else
		"CCK",
#endif
		"HT20L",
		"HT20H",
		"HT40L",
		"HT40H",
		"HT80L",
		"HT80H",
		"HT160L",
		"HT160H"
		};
	char *pacParsePwrAX[PWR_LIMIT_HE_BW160_NUM] = {
		"RU26L",
		"RU26H",
		"RU26U",
		"RU52L",
		"RU52H",
		"RU52U",
		"RU106L",
		"RU106H",
		"RU106U",
		"RU242L",
		"RU242H",
		"RU242U",
		"RU484L",
		"RU484H",
		"RU484U",
		"RU996L",
		"RU996H",
		"RU996U",
		"RU1992L",
		"RU1992H",
		"RU1992U"
		};
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	char *pacParsePwrEHT[PWR_LIMIT_EHT_NUM] = {
		"EHT26L",
		"EHT26H",
		"EHT26U",
		"EHT52L",
		"EHT52H",
		"EHT52U",
		"EHT106L",
		"EHT106H",
		"EHT106U",
		"EHT242L",
		"EHT242H",
		"EHT242U",
		"EHT484L",
		"EHT484H",
		"EHT484U",
		"EHT996L",
		"EHT996H",
		"EHT996U",
		"EHT996X2L",
		"EHT996X2H",
		"EHT996X2U",
		"EHT26_52L",
		"EHT26_52H",
		"EHT26_52U",
		"EHT26_106L",
		"EHT26_106H",
		"EHT26_106U",
		"EHT484_242L",
		"EHT484_242H",
		"EHT484_242U",
		"EHT996_484L",
		"EHT996_484H",
		"EHT996_484U",
		"EHT996_484_242L",
		"EHT996_484_242H",
		"EHT996_484_242U"
		};
#if (CFG_SUPPORT_WIFI_6G == 1)
	char *pacParsePwrEHT_6G[PWR_LIMIT_EHT_6G_NUM] = {
		"EHT26L",
		"EHT26H",
		"EHT26U",
		"EHT52L",
		"EHT52H",
		"EHT52U",
		"EHT106L",
		"EHT106H",
		"EHT106U",
		"EHT242L",
		"EHT242H",
		"EHT242U",
		"EHT484L",
		"EHT484H",
		"EHT484U",
		"EHT996L",
		"EHT996H",
		"EHT996U",
		"EHT996X2L",
		"EHT996X2H",
		"EHT996X2U",
		"EHT996X4L",
		"EHT996X4H",
		"EHT996X4U",
		"EHT26_52L",
		"EHT26_52H",
		"EHT26_52U",
		"EHT26_106L",
		"EHT26_106H",
		"EHT26_106U",
		"EHT484_242L",
		"EHT484_242H",
		"EHT484_242U",
		"EHT996_484L",
		"EHT996_484H",
		"EHT996_484U",
		"EHT996_484_242L",
		"EHT996_484_242H",
		"EHT996_484_242U",
		"EHT996X2_484L",
		"EHT996X2_484H",
		"EHT996X2_484U",
		"EHT996X3L",
		"EHT996X3H",
		"EHT996X3U",
		"EHT996X3_484L",
		"EHT996X3_484H",
		"EHT996X3_484U"
		};
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

	if (!pcContent) {
		DBGLOG(RLM, ERROR, "pcContent is null\n");
		return NULL;
	}

	pcContCur = pcContent;
	pcContEnd = pcContent + kalStrLen(pcContent);

	DBGLOG(RLM, TRACE, "parse config:%s\n", pcContCur);

	if (fgSkipHeader == TRUE)
		goto skipLabel;

	/* insert element into prTxPwrCtrlList */
	/* parse scenario name */
	kalMemZero(acTmpName, MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE);
	pcContOld = pcContCur;
	pcContTmp = txPwrGetString(&pcContCur, ";");
	if (!pcContTmp) {
		DBGLOG(RLM, ERROR, "parse scenario name error: %s\n",
		       pcContOld);
		return NULL;
	}

	copySize = kalStrLen(pcContTmp);
	if (copySize >= MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE)
		copySize = MAX_TX_PWR_CTRL_ELEMENT_NAME_SIZE - 1;
	kalMemCopy(acTmpName, pcContTmp, copySize);
	acTmpName[copySize] = 0;

	/* parese scenario sub index */
	pcContOld = pcContCur;
	if (txPwrParseNumber(&pcContCur, ";", &op, &value)) {
		DBGLOG(RLM, ERROR, "parse scenario sub index error: %s\n",
		       pcContOld);
		return NULL;
	}
	if (op != 1) {
		DBGLOG(RLM, ERROR,
		       "parse scenario sub index error: op=%u, val=%d\n",
		       op, value);
		return NULL;
	}

	/* parese scenario applied way */
	pcContOld = pcContCur;
	if (txPwrParseNumber(&pcContCur, ";", &op, &ucAppliedWay)) {
		DBGLOG(RLM, ERROR, "parse applied way error: %s\n",
		       pcContOld);
		return NULL;
	}
	if ((ucAppliedWay < PWR_CTRL_TYPE_APPLIED_WAY_WIFION) ||
	    (ucAppliedWay > PWR_CTRL_TYPE_APPLIED_WAY_IOCTL)) {
		DBGLOG(RLM, ERROR,
		       "parse applied way error: value=%u\n",
		       ucAppliedWay);
		return NULL;
	}

	/* parese scenario applied type */
	pcContOld = pcContCur;
	if (txPwrParseNumber(&pcContCur, ";", &op, &ucOperation)) {
		DBGLOG(RLM, ERROR, "parse operation error: %s\n",
		       pcContOld);
		return NULL;
	}
	if ((ucOperation < PWR_CTRL_TYPE_OPERATION_POWER_LEVEL) ||
	    (ucOperation > PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)) {
		DBGLOG(RLM, ERROR,
		       "parse operation error: value=%u\n",
		       ucOperation);
		return NULL;
	}

	switch (ucAppliedWay) {
	case PWR_CTRL_TYPE_APPLIED_WAY_WIFION:
		if (ucOperation == PWR_CTRL_TYPE_OPERATION_POWER_LEVEL)
			value2 = PWR_CTRL_TYPE_WIFION_POWER_LEVEL;
		else
			value2 = PWR_CTRL_TYPE_WIFION_POWER_OFFSET;
		break;
	case PWR_CTRL_TYPE_APPLIED_WAY_IOCTL:
		if (ucOperation == PWR_CTRL_TYPE_OPERATION_POWER_LEVEL)
			value2 = PWR_CTRL_TYPE_IOCTL_POWER_LEVEL;
		else
			value2 = PWR_CTRL_TYPE_IOCTL_POWER_OFFSET;
		break;
	}

skipLabel:
	/* decide how many channel setting */
	pcContOld = pcContCur;
	while (pcContCur <= pcContEnd) {
		if ((*pcContCur) == '[')
			ucSettingCount++;
		pcContCur++;
	}

	if (ucSettingCount == 0) {
		DBGLOG(RLM, ERROR,
		       "power ctrl channel setting is empty\n");
		return NULL;
	}

	/* allocate memory for control element */
	u4MemSize += (ucSettingCount == 1) ? 0 : (ucSettingCount - 1) *
			sizeof(struct TX_PWR_CTRL_CHANNEL_SETTING);
	prCurElement = (struct TX_PWR_CTRL_ELEMENT *)kalMemAlloc(
					u4MemSize, VIR_MEM_TYPE);
	if (!prCurElement) {
		DBGLOG(RLM, ERROR,
		       "alloc power ctrl element failed\n");
		return NULL;
	}

	/* assign values into control element */
	kalMemZero(prCurElement, u4MemSize);
	if (fgSkipHeader == FALSE) {
		kalMemCopy(prCurElement->name, acTmpName, copySize);
		prCurElement->index = (uint8_t)value;
		prCurElement->eCtrlType = (enum ENUM_TX_POWER_CTRL_TYPE)value2;
		if (prCurElement->eCtrlType <=
		    PWR_CTRL_TYPE_WIFION_POWER_OFFSET)
			prCurElement->fgApplied = TRUE;
	}
	prCurElement->settingCount = ucSettingCount;

	/* parse channel setting list */
	pcContCur = pcContOld + 1; /* skip '[' */

	for (i = 0; i < ucSettingCount; i++) {
		if (pcContCur >= pcContEnd) {
			DBGLOG(RLM, ERROR,
			       "parse error: out of bound\n");
			goto clearLabel;
		}

		prTmpSetting = &prCurElement->rChlSettingList[i];

		/* verify there is ] symbol */
		pcContNext = kalStrChr(pcContCur, ']');
		if (!pcContNext) {
			DBGLOG(RLM, ERROR,
			       "parse error: miss symbol ']', %s\n",
			       pcContCur);
			goto clearLabel;
		}
		/* Calculate config element in this segment*/
		pcContTmp = pcContCur;
		count = 0;
		while (pcContTmp < pcContNext) {
			if (*pcContTmp == ',')
				count++;
			pcContTmp++;
		}

		if ((count != PWR_LIMIT_NUM) &&
			/* include keyword : LEGACY */
			(count != PWR_LIMIT_NUM + 1) &&
			(count != PWR_LIMIT_HE_NUM) &&
			/* include keyword : HE or AX160 */
			(count != PWR_LIMIT_HE_BW160_NUM + 1) &&
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			/* include keyword : EHT */
			(count != PWR_LIMIT_EHT_NUM + 1) &&
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
			(count != PWR_LIMIT_6E_NUM) &&
			/* include keyword : LEGACY6G */
			(count != PWR_LIMIT_LEGACY_6G_NUM + 1) &&
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			/* include keyword : EHT6G */
			(count != PWR_LIMIT_EHT_6G_NUM + 1) &&
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
			(count != PWR_CFG_PRAM_NUM_ALL_RATE)) {
			DBGLOG(RLM, ERROR,
			       "parse error: not segments, %s\n",
			       pcContCur);
			goto clearLabel;
		}

		/* parse channel setting type */
		pcContOld = pcContCur;
		pcContTmp = txPwrGetString(&pcContCur, ",");
		if (!pcContTmp) {
			DBGLOG(RLM, ERROR,
			       "parse channel setting type error, %s\n",
			       pcContOld);
			goto clearLabel;
		}

		if (!pcContCur) {
			DBGLOG(RLM, ERROR,
			       "parse rate tag error\n");
			goto clearLabel;
		}

		/* If cfg parameter only one,
		 * no need to parse rate tag
		 */
		if (count != PWR_CFG_PRAM_NUM_ALL_RATE) {
			/* Start Parsing Rate Tag */
			pcContTag = txPwrGetString(&pcContCur, ",");
			if (!pcContCur) {
				DBGLOG(RLM, ERROR,
					"parse rate tag error\n");
				goto clearLabel;
			}

			if (pcContTag == NULL)
				u4RollBackStep = 0;
			else
				u4RollBackStep = kalStrLen(pcContTag);

			u1RateTag = txPwrCfgParsingRateTag(pcContTag, count);

			if (u1RateTag == PWR_CFG_RATE_TAG_MISS) {
				/* 1. Miss Rate Tag, string roll back
				 * 2. strsep will switch the the token which is
				 * ',' here to '\0'
				 */
				pcContCur = pcContCur - 1;
				*pcContCur = ',';
				pcContCur = pcContCur - u4RollBackStep;
			}
		}
		DBGLOG(RLM, TRACE,
			"RateTag[%d]RollBack[%d]After parse rate tag:%s\n",
			u1RateTag,
			u4RollBackStep,
			pcContCur);

		/* "ALL" */
		if (kalStrCmp(pcContTmp,
				     PWR_CTRL_CHNL_TYPE_KEY_ALL) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_ALL;
		/* "2G4" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_2G4) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_2G4;
		/* "5G" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_5G) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_5G;
		/* "BANDEDGE2G4" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_BANDEDGE_2G4) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_BANDEDGE_2G4;
		/* "BANDEDGE5G" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_BANDEDGE_5G) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_BANDEDGE_5G;
		/* "5GBAND1" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_5G_BAND1) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_5G_BAND1;
		/* "5GBAND2" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_5G_BAND2) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_5G_BAND2;
		/* "5GBAND3" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_5G_BAND3) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_5G_BAND3;
		/* "5GBAND4" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_5G_BAND4) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_5G_BAND4;
#if (CFG_SUPPORT_WIFI_6G == 1)
		/* "6G" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_6G) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_6G;
		/* "6GBAND1" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_6G_BAND1) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_6G_BAND1;
		/* "6GBAND2" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_6G_BAND2) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_6G_BAND2;
		/* "6GBAND3" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_6G_BAND3) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_6G_BAND3;
		/* "6GBAND4" */
		else if (kalStrCmp(pcContTmp,
				   PWR_CTRL_CHNL_TYPE_KEY_6G_BAND4) == 0)
			prTmpSetting->eChnlType =
				PWR_CTRL_CHNL_TYPE_6G_BAND4;
#endif
		else {
			pcContCur2 = pcContOld;
			pcContTmp = txPwrGetString(&pcContCur2, "-");
			if (!pcContTmp) {
				DBGLOG(RLM, ERROR,
					"parse channel range error: %s\n",
					pcContOld);
				goto clearLabel;
			}

			if (pcContCur2 == NULL) { /* case: normal channel */
				if (kalkStrtou8(pcContOld, 0, &value) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse channel error: %s\n",
					       pcContOld);
					goto clearLabel;
				}
				prTmpSetting->channelParam[0] = value;
#if (CFG_SUPPORT_WIFI_6G == 1)
				if (count == PWR_LIMIT_6E_NUM
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_LEGACY6G)
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_HE6G)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_EHT6G)
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
					)
					prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_6G_NORMAL;
				else
#endif /* CFG_SUPPORT_WIFI_6G */
					prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_NORMAL;
			} else { /* case: channel range */
				if (kalkStrtou8(pcContTmp, 0, &value) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse first channel error, %s\n",
					       pcContTmp);
					goto clearLabel;
				}
				if (kalkStrtou8(pcContCur2, 0, &value2) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse second channel error, %s\n",
					       pcContCur2);
					goto clearLabel;
				}
				prTmpSetting->channelParam[0] = value;
				prTmpSetting->channelParam[1] =	value2;
#if (CFG_SUPPORT_WIFI_6G == 1)
				if (count == PWR_LIMIT_6E_NUM
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_LEGACY6G)
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_HE6G)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
					|| (u1RateTag ==
						PWR_CFG_RATE_TAG_HIT_EHT6G)
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
					)
					prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_6G_RANGE;
				else
#endif /* CFG_SUPPORT_WIFI_6G */
				prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_RANGE;
			}
		}

		if (count == PWR_CFG_PRAM_NUM_ALL_RATE) {
			/* parse all rate setting */
			pcContOld = pcContCur;
			if (txPwrParseNumber(&pcContCur, "]", &op, &value)) {
				DBGLOG(RLM, ERROR, "parse CCK error, %s\n",
					   pcContOld);
				goto clearLabel;
			}

			for (j = 0; j < PWR_CFG_PRAM_NUM_AC; j++) {
				prTmpSetting->op[j] = op;
				prTmpSetting->i8PwrLimit[j] = (op != 2) ?
							value : (0 - value);
			}

			for (j = 0; j < PWR_LIMIT_HE_BW160_NUM; j++) {
				prTmpSetting->opHE[j] = op;
				prTmpSetting->i8PwrLimitHE[j] = (op != 2) ?
							value : (0 - value);
			}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			for (j = 0; j < PWR_LIMIT_EHT_NUM; j++) {
				prTmpSetting->opEHT[j] = op;
				prTmpSetting->i8PwrLimitEHT[j] = (op != 2) ?
							value : (0 - value);
			}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
			for (j = 0; j < PWR_LIMIT_6E_NUM; j++) {
				prTmpSetting->op6E[j] = op;
				prTmpSetting->i8PwrLimit6E[j] = (op != 2) ?
							value : (0 - value);
			}
			for (j = 0; j < PWR_LIMIT_LEGACY_6G_NUM; j++) {
				prTmpSetting->opLegacy_6G[j] = op;
				prTmpSetting->i8PwrLimitLegacy_6G[j]
					= (op != 2) ? value : (0 - value);
			}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			for (j = 0; j < PWR_LIMIT_EHT_6G_NUM; j++) {
				prTmpSetting->opEHT_6G[j] = op;
				prTmpSetting->i8PwrLimitEHT_6G[j] = (op != 2) ?
							value : (0 - value);
			}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
			goto skipLabel2;
		} else  if (count == PWR_LIMIT_NUM ||
			u1RateTag == PWR_CFG_RATE_TAG_HIT_LEGACY) {
			for (j = 0; j < PWR_CFG_PRAM_NUM_AC; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_CFG_PRAM_NUM_AC - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse %s error, %s\n",
						pacParsePwrAC[j], pcContOld);
					goto clearLabel;
				}
				prTmpSetting->op[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimit[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->op[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAC[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		} else if (count == PWR_LIMIT_HE_NUM) {
			for (j = 0; j < PWR_LIMIT_HE_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_HE_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse %s error, %s\n",
						pacParsePwrAX[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->opHE[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimitHE[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->opHE[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAX[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_EHT) {
			for (j = 0; j < PWR_LIMIT_EHT_NUM; j++) {
				/* parse EHT26L ...  EHT996_484_242U setting */
				if (j == PWR_LIMIT_EHT_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse EHT %s error, %s\n",
						pacParsePwrEHT[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->opEHT[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimitEHT[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->opEHT[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse EHT %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrEHT[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (count == PWR_LIMIT_6E_NUM ||
			u1RateTag == PWR_CFG_RATE_TAG_HIT_HE6G) {
			for (j = 0; j < PWR_LIMIT_6E_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_6E_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse %s error, %s\n",
						pacParsePwrAX[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->op6E[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimit6E[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->op6E[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAX[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		} else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_LEGACY6G) {
			for (j = 0; j < PWR_LIMIT_LEGACY_6G_NUM; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_LIMIT_LEGACY_6G_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse LEGACY6G %s error, %s\n",
						pacParsePwrAC[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->opLegacy_6G[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimitLegacy_6G[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->opLegacy_6G[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse LEGACY6G %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAC[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_EHT6G) {
			for (j = 0; j < PWR_LIMIT_EHT_6G_NUM; j++) {
				/* parse EHT26L ...  EHT996X3_484U setting */
				if (j == PWR_LIMIT_EHT_6G_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse EHT6G %s error, %s\n",
						pacParsePwrEHT_6G[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->opEHT_6G[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimitEHT_6G[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->opEHT_6G[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse EHT6G %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrEHT_6G[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
		else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_HE) {
			for (j = 0; j < PWR_LIMIT_HE_BW160_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_HE_BW160_NUM - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse %s error, %s\n",
						pacParsePwrAX[j],
						pcContOld);
					goto clearLabel;
				}
				prTmpSetting->opHE[j] =
					(enum ENUM_TX_POWER_CTRL_VALUE_SIGN)op;
				prTmpSetting->i8PwrLimitHE[j] =
					(op != 2) ? value : (0 - value);
				if ((prTmpSetting->opHE[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(ucOperation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAX[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}

skipLabel2:
		if (i + 1  < ucSettingCount) {
			/* Skip ']' and '[' */
			pcContCur = pcContNext + 2;
		} else {
			/* Last setting */
			pcContCur = pcContNext + 1;
		}
		pcContEnd = pcContCur + kalStrLen(pcContCur);
	}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	/* parse power limit append tag to list. */

	if (txPwrOnPreParseAppendTag(prCurElement)) {
		DBGLOG(RLM, INFO,
			"txPwrOnPreParseAppendTag fail.");
	}

	if (txPwrParseAppendTag(pcContCur, pcContEnd, prCurElement))
		DBGLOG(RLM, INFO, "txPwrParseAppendTag fail (%s).", pcContent);
#endif
	/* pcCurElement will be free on wifi off */
	return prCurElement;

clearLabel:
	if (prCurElement != NULL)
		kalMemFree(prCurElement, VIR_MEM_TYPE, u4MemSize);
	return NULL;
}

/* filterType: 0:no filter, 1:fgEnable is TRUE */
int txPwrCtrlListSize(struct ADAPTER *prAdapter, uint8_t filterType)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;
	struct LINK *aryprlist[2] = {
		&prAdapter->rTxPwr_DefaultList,
		&prAdapter->rTxPwr_DynamicList
	};
	int i, count = 0;

	for (i = 0; i < ARRAY_SIZE(aryprlist); i++) {
		LINK_FOR_EACH_SAFE(prCur, prNext, aryprlist[i]) {
			prCurElement = LINK_ENTRY(prCur,
				struct TX_PWR_CTRL_ELEMENT, node);
			if ((filterType == 1) &&
			    (prCurElement->fgApplied != TRUE))
				continue;
			count++;
		}
	}

	return count;
}

/* filterType: 0:no filter, 1:fgApplied is TRUE */
void txPwrCtrlShowList(struct ADAPTER *prAdapter, uint8_t filterType,
		       char *message)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;
	struct LINK *aryprlist[2] = {
		&prAdapter->rTxPwr_DefaultList,
		&prAdapter->rTxPwr_DynamicList
	};
	uint8_t ucAppliedWay, ucOperation;
	int i, count = 0;

	if (filterType == 1)
		DBGLOG(RLM, TRACE, "Tx Power Ctrl List=[%s], Size=[%d]",
		       message, txPwrCtrlListSize(prAdapter, filterType));
	else
		DBGLOG(RLM, TRACE, "Tx Power Ctrl List=[%s], Size=[%d]",
		       message, txPwrCtrlListSize(prAdapter, filterType));

	for (i = 0; i < ARRAY_SIZE(aryprlist); i++) {
		LINK_FOR_EACH_SAFE(prCur, prNext, aryprlist[i]) {
			prCurElement = LINK_ENTRY(prCur,
					struct TX_PWR_CTRL_ELEMENT, node);
			if ((filterType == 1) &&
			    (prCurElement->fgApplied != TRUE))
				continue;

			switch (prCurElement->eCtrlType) {
			case PWR_CTRL_TYPE_WIFION_POWER_LEVEL:
				ucAppliedWay =
					PWR_CTRL_TYPE_APPLIED_WAY_WIFION;
				ucOperation =
					PWR_CTRL_TYPE_OPERATION_POWER_LEVEL;
				break;
			case PWR_CTRL_TYPE_WIFION_POWER_OFFSET:
				ucAppliedWay =
					PWR_CTRL_TYPE_APPLIED_WAY_WIFION;
				ucOperation =
					PWR_CTRL_TYPE_OPERATION_POWER_OFFSET;
				break;
			case PWR_CTRL_TYPE_IOCTL_POWER_LEVEL:
				ucAppliedWay =
					PWR_CTRL_TYPE_APPLIED_WAY_IOCTL;
				ucOperation =
					PWR_CTRL_TYPE_OPERATION_POWER_LEVEL;
				break;
			case PWR_CTRL_TYPE_IOCTL_POWER_OFFSET:
				ucAppliedWay =
					PWR_CTRL_TYPE_APPLIED_WAY_IOCTL;
				ucOperation =
					PWR_CTRL_TYPE_OPERATION_POWER_OFFSET;
				break;
			default:
				ucAppliedWay = 0;
				ucOperation = 0;
				break;
			}

			DBGLOG(RLM, TRACE,
			       "Tx Power Ctrl Element-%u: name=[%s], index=[%u], appliedWay=[%u:%s], operation=[%u:%s], ChlSettingCount=[%u]\n",
			       ++count, prCurElement->name, prCurElement->index,
			       ucAppliedWay,
			       g_au1TxPwrAppliedWayLabel[ucAppliedWay - 1],
			       ucOperation,
			       g_au1TxPwrOperationLabel[ucOperation - 1],
			       prCurElement->settingCount);

		}
	}
}

/* This function used to delete element by specifying name or index
 * if index is 0, deletion only according name
 * if index >= 1, deletion according name and index
 */
void _txPwrCtrlDeleteElement(struct ADAPTER *prAdapter,
			     uint8_t *name,
			     uint32_t index,
			     struct LINK *prTxPwrCtrlList)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;
	uint32_t u4MemSize = sizeof(struct TX_PWR_CTRL_ELEMENT);
	uint32_t u4MemSize2;
	uint32_t u4SettingSize = sizeof(struct TX_PWR_CTRL_CHANNEL_SETTING);
	uint8_t ucSettingCount;
	u_int8_t fgFind;

	LINK_FOR_EACH_SAFE(prCur, prNext, prTxPwrCtrlList) {
		fgFind = FALSE;
		prCurElement = LINK_ENTRY(prCur, struct TX_PWR_CTRL_ELEMENT,
					  node);
		if (prCurElement != NULL
			&& kalStrCmp(prCurElement->name, name) == 0) {
			if (index == 0)
				fgFind = TRUE;
			else if (prCurElement->index == index)
				fgFind = TRUE;
			if (fgFind) {
				linkDel(prCur);
				if (prCurElement != NULL) {
					ucSettingCount =
						prCurElement->settingCount;
						u4MemSize2 = u4MemSize +
						((ucSettingCount == 1) ? 0 :
						(ucSettingCount - 1) *
						u4SettingSize);
					kalMemFree(prCurElement, VIR_MEM_TYPE,
						u4MemSize2);
				}
			}
		}
	}
}

void txPwrCtrlDeleteElement(struct ADAPTER *prAdapter,
			    uint8_t *name,
			    uint32_t index,
			    enum ENUM_TX_POWER_CTRL_LIST_TYPE eListType)
{
	if ((eListType == PWR_CTRL_TYPE_ALL_LIST) ||
	    (eListType == PWR_CTRL_TYPE_DEFAULT_LIST))
		_txPwrCtrlDeleteElement(prAdapter, name, index,
					&prAdapter->rTxPwr_DefaultList);

	if ((eListType == PWR_CTRL_TYPE_ALL_LIST) ||
	    (eListType == PWR_CTRL_TYPE_DYNAMIC_LIST))
		_txPwrCtrlDeleteElement(prAdapter, name, index,
					&prAdapter->rTxPwr_DynamicList);
}

struct TX_PWR_CTRL_ELEMENT *_txPwrCtrlFindElement(struct ADAPTER *prAdapter,
				 uint8_t *name, uint32_t index,
				 u_int8_t fgCheckIsApplied,
				 struct LINK *prTxPwrCtrlList)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;


	LINK_FOR_EACH_SAFE(prCur, prNext, prTxPwrCtrlList) {
		u_int8_t fgFind = FALSE;

		prCurElement = LINK_ENTRY(
			prCur, struct TX_PWR_CTRL_ELEMENT, node);
		if (kalStrCmp(prCurElement->name, name) == 0) {
			if ((!fgCheckIsApplied) ||
			    (fgCheckIsApplied &&
			    (prCurElement->fgApplied == TRUE))) {
				if (index == 0)
					fgFind = TRUE;
				else if (prCurElement->index == index)
					fgFind = TRUE;
			}
			if (fgFind)
				return prCurElement;
		}
	}
	return NULL;
}

struct TX_PWR_CTRL_ELEMENT *txPwrCtrlFindElement(struct ADAPTER *prAdapter,
				 uint8_t *name, uint32_t index,
				 u_int8_t fgCheckIsApplied,
				 enum ENUM_TX_POWER_CTRL_LIST_TYPE eListType)
{
	struct LINK *prTxPwrCtrlList = NULL;

	if (eListType == PWR_CTRL_TYPE_DEFAULT_LIST)
		prTxPwrCtrlList = &prAdapter->rTxPwr_DefaultList;
	if (eListType == PWR_CTRL_TYPE_DYNAMIC_LIST)
		prTxPwrCtrlList = &prAdapter->rTxPwr_DynamicList;
	if (prTxPwrCtrlList == NULL)
		return NULL;

	return _txPwrCtrlFindElement(prAdapter, name, index, fgCheckIsApplied,
				     prTxPwrCtrlList);
}

void txPwrCtrlAddElement(struct ADAPTER *prAdapter,
			 struct TX_PWR_CTRL_ELEMENT *prElement)
{
	struct LINK_ENTRY *prNode = &prElement->node;

	switch (prElement->eCtrlType) {
	case PWR_CTRL_TYPE_WIFION_POWER_LEVEL:
		linkAdd(prNode, &prAdapter->rTxPwr_DefaultList);
		break;
	case PWR_CTRL_TYPE_WIFION_POWER_OFFSET:
		linkAddTail(prNode, &prAdapter->rTxPwr_DefaultList);
		break;
	case PWR_CTRL_TYPE_IOCTL_POWER_LEVEL:
		linkAdd(prNode, &prAdapter->rTxPwr_DynamicList);
		break;
	case PWR_CTRL_TYPE_IOCTL_POWER_OFFSET:
		linkAddTail(prNode, &prAdapter->rTxPwr_DynamicList);
		break;
	}
}

void txPwrCtrlFileBufToList(struct ADAPTER *prAdapter, uint8_t *pucFileBuf)
{
	struct TX_PWR_CTRL_ELEMENT *prNewElement;
	char *oneLine;

	if (pucFileBuf == NULL)
		return;

	while ((oneLine = kalStrSep((char **)(&pucFileBuf), "\r\n"))
	       != NULL) {
		/* skip comment line and empty line */
		if ((oneLine[0] == '#') || (oneLine[0] == 0))
			continue;

		prNewElement = txPwrCtrlStringToStruct(oneLine, FALSE);
		if (prNewElement != NULL) {
			/* delete duplicated element
			 * by checking name and index
			 */
			txPwrCtrlDeleteElement(prAdapter,
				prNewElement->name, prNewElement->index,
				PWR_CTRL_TYPE_ALL_LIST);

			/* append to rTxPwr_List */
			txPwrCtrlAddElement(prAdapter, prNewElement);
		}
	}

	/* show the tx power ctrl list */
	txPwrCtrlShowList(prAdapter, 0, "config list, after loading cfg file");
}

void txPwrCtrlGlobalVariableToList(struct ADAPTER *prAdapter)
{
	struct TX_PWR_CTRL_ELEMENT *pcElement;
	char *ptr;
	int32_t i, u4MemSize;

	for (i = 0; i < ARRAY_SIZE(g_au1TxPwrDefaultSetting); i++) {
		/* skip empty line */
		if (g_au1TxPwrDefaultSetting[i][0] == 0)
			continue;
		u4MemSize = kalStrLen(g_au1TxPwrDefaultSetting[i]) + 1;
		ptr = (char *)kalMemAlloc(u4MemSize, VIR_MEM_TYPE);
		if (ptr == NULL) {
			DBGLOG(RLM, ERROR, "kalMemAlloc fail: %d\n", u4MemSize);
			continue;
		}
		kalMemCopy(ptr, g_au1TxPwrDefaultSetting[i], u4MemSize);
		*(ptr + u4MemSize - 1) = 0;
		pcElement = txPwrCtrlStringToStruct(ptr, FALSE);
		kalMemFree(ptr, VIR_MEM_TYPE, u4MemSize);
		if (pcElement != NULL) {
			/* delete duplicated element
			 * by checking name and index
			 */
			txPwrCtrlDeleteElement(prAdapter,
				pcElement->name, pcElement->index,
				PWR_CTRL_TYPE_ALL_LIST);

			/* append to rTxPwr_List */
			txPwrCtrlAddElement(prAdapter, pcElement);
		}
	}

	/* show the tx power ctrl cfg list */
	txPwrCtrlShowList(prAdapter, 0,
			  "config list, after loadding global variables");
}

void txPwrCtrlCfgFileToList(struct ADAPTER *prAdapter)
{
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;

	if (kalRequestFirmware("txpowerctrl.cfg", &pucConfigBuf,
	    &u4ConfigReadLen, TRUE,
	    kalGetGlueDevHdl(prAdapter->prGlueInfo)) == 0) {
		/* ToDo:: Nothing */
	}

	if (pucConfigBuf) {
		txPwrCtrlFileBufToList(prAdapter, pucConfigBuf);
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
	} else
		DBGLOG(RLM, INFO,
		       "no txpowerctrl.cfg or file is empty\n");
}

void txPwrCtrlLoadConfig(struct ADAPTER *prAdapter)
{
	/* 1. add records from global tx power ctrl setting into cfg list */
	txPwrCtrlGlobalVariableToList(prAdapter);

	/* 2. update cfg list by txpowerctrl.cfg */
	txPwrCtrlCfgFileToList(prAdapter);

#if CFG_SUPPORT_PWR_LIMIT_COUNTRY
	/* 3. send setting to firmware */
	rlmDomainSendPwrLimitCmd(prAdapter);
#endif
}

void txPwrCtrlInit(struct ADAPTER *prAdapter)
{
	LINK_INITIALIZE(&prAdapter->rTxPwr_DefaultList);
	LINK_INITIALIZE(&prAdapter->rTxPwr_DynamicList);
}

void txPwrCtrlUninit(struct ADAPTER *prAdapter)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TX_PWR_CTRL_ELEMENT *prCurElement = NULL;
	struct LINK *aryprlist[2] = {
		&prAdapter->rTxPwr_DefaultList,
		&prAdapter->rTxPwr_DynamicList
	};
	uint32_t u4MemSize = sizeof(struct TX_PWR_CTRL_ELEMENT);
	uint32_t u4MemSize2;
	uint32_t u4SettingSize = sizeof(struct TX_PWR_CTRL_CHANNEL_SETTING);
	uint8_t ucSettingCount;
	int32_t i;

	for (i = 0; i < ARRAY_SIZE(aryprlist); i++) {
		LINK_FOR_EACH_SAFE(prCur, prNext, aryprlist[i]) {
			prCurElement = LINK_ENTRY(prCur,
					struct TX_PWR_CTRL_ELEMENT, node);
			linkDel(prCur);
			if (prCurElement) {
				ucSettingCount = prCurElement->settingCount;
					u4MemSize2 = u4MemSize +
					((ucSettingCount <= 1) ? 0 :
					(ucSettingCount - 1) * u4SettingSize);
				kalMemFree(prCurElement,
					VIR_MEM_TYPE, u4MemSize2);
			}
		}
	}
}
/* dynamic tx power control: end **********************************************/
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT */

void rlmDomainShowPwrLimitPerCh(char *message,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd)
{
	/* for print usage */
	struct CMD_CHANNEL_POWER_LIMIT *prPwrLmt = NULL;
	/* for print usage */
	struct CMD_CHANNEL_POWER_LIMIT_HE *prPwrLmtHE = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prPwrLmtHEBW160 = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prPwrLmtEHT = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prPwrLmtAnt = NULL;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	struct CMD_CHANNEL_POWER_LIMIT_6E *prPwrLmt6E = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prCmdPwrLimtLegacy_6G = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prPwrLmtEHT_6G = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	enum ENUM_PWR_LIMIT_TYPE eType;
	uint8_t i = 0, j = 0, k = 0;
	char msgLimit[PWR_BUF_LEN];
	int msgOfs = 0;
	int8_t *prcRatePwr;

	ASSERT(prCmd);

	eType = prCmd->ucLimitType;

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	if (eType == PWR_LIMIT_TYPE_COMP_ANT_V2) {
		prPwrLmtAnt = &prCmd->u.rChPwrLimtAnt[0];
		DBGLOG(RLM, TRACE, "ANT Config #%d", prCmd->ucNum);
		for (i = 0; i < prCmd->ucNum; i++) {
			DBGLOG(RLM, TRACE, "%s ANT Config%d[%d:%d:%d:%d]\n",
				message,
				i,
				prPwrLmtAnt[i].cTagIdx,
				prPwrLmtAnt[i].cBandIdx,
				prPwrLmtAnt[i].cAntIdx,
				prPwrLmtAnt[i].cValue);

			if (prPwrLmtAnt[i].cTagIdx == -1)
				break;
		}

		return;
	}
#endif

	for (i = 0; i < prCmd->ucNum; i++) {

		if (i >= MAX_CMD_SUPPORT_CHANNEL_NUM) {
			DBGLOG(RLM, ERROR, "out of MAX CH Num\n");
			return;
		}

		kalMemZero(msgLimit, sizeof(char)*PWR_BUF_LEN);
		msgOfs = 0;
		if (eType == PWR_LIMIT_TYPE_COMP_11AX) {
			prPwrLmtHE = &prCmd->u.rChPwrLimtHE[i];
			prcRatePwr = &prPwrLmtHE->cPwrLimitRU26L;

			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"HE ch=%d,Limit=",
				prPwrLmtHE->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_RU26_L; j < PWR_LIMIT_HE_NUM ; j++)
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);

		} else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
			prPwrLmtHEBW160 = &prCmd->u.rChPwrLimtHEBW160[i];
			prcRatePwr = &prPwrLmtHEBW160->cPwrLimitRU26L;

			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"HE ch=%d,Limit=",
				prPwrLmtHEBW160->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_RU26_L;
				j < PWR_LIMIT_HE_BW160_NUM ; j++)
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);

		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {
			prPwrLmtEHT = &prCmd->u.rChPwrLimtEHT[i];
			prcRatePwr = &prPwrLmtEHT->cPwrLimitEHT26L;

			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"EHT ch=%d,Limit=",
				prPwrLmtEHT->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_EHT26_L; j < PWR_LIMIT_EHT_NUM ; j++)
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_6E_1 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_2 ||
					eType == PWR_LIMIT_TYPE_COMP_6E_3) {
			prPwrLmt6E = &prCmd->u.rChPwrLimt6E[i];
			prcRatePwr = &prPwrLmt6E->cPwrLimitRU26L;

			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"6E ch=%d,Limit=",
				prPwrLmt6E->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_RU26_L; j < PWR_LIMIT_6E_NUM ; j++)
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);

		} else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
			eType <= PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
			prCmdPwrLimtLegacy_6G =
				&prCmd->u.rChPwrLimtLegacy_6G[i];
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
			prcRatePwr =
				&prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L;
#else
			prcRatePwr =
				&prCmdPwrLimtLegacy_6G->cPwrLimitCCK;
#endif
			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"Legacy_6G ch=%d,Limit=",
				prCmdPwrLimtLegacy_6G->ucCentralCh);

			/*message body*/
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
			for (j = PWR_LIMIT_CCK_L;
				j < PWR_LIMIT_LEGACY_6G_NUM ; j++)
#else
			for (j = PWR_LIMIT_CCK;
				j < PWR_LIMIT_LEGACY_6G_NUM ; j++)
#endif
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {
			prPwrLmtEHT_6G = &prCmd->u.rChPwrLimtEHT_6G[i];
			prcRatePwr = &prPwrLmtEHT_6G->cPwrLimitEHT26L;

			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"EHT_6G ch=%d,Limit=",
				prPwrLmtEHT_6G->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_EHT_6G_26_L;
				j < PWR_LIMIT_EHT_6G_NUM; j++)
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + j));
			/*message tail*/
			if (msgOfs >= 1)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
		else if (eType == PWR_LIMIT_TYPE_COMP_11AC ||
			eType == PWR_LIMIT_TYPE_COMP_11AC_V2) {
			prPwrLmt = &prCmd->u.rChannelPowerLimit[i];
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
			prcRatePwr = &prPwrLmt->cPwrLimitCCK_L;
#else
			prcRatePwr = &prPwrLmt->cPwrLimitCCK;
#endif
			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"legacy ch=%d,Limit=",
				prPwrLmt->ucCentralCh);

			/*message body*/
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
			for (k = PWR_LIMIT_CCK_L; k < PWR_LIMIT_NUM ; k++)
#else
			for (k = PWR_LIMIT_CCK; k < PWR_LIMIT_NUM ; k++)
#endif
				msgOfs += snprintf(msgLimit + msgOfs,
					PWR_BUF_LEN - msgOfs,
					"%d,",
					*(prcRatePwr + k));

			/*message tail*/
			if (msgOfs > 0)
				msgLimit[msgOfs-1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE, "%s:%s\n", message, msgLimit);
		}
	}
}

void rlmDomainSendPwrLimitCmd(struct ADAPTER *prAdapter)
{
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdHE = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_1 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_2 = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdAnt = NULL;
	uint32_t u4SetQueryInfoLenAnt = 0;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	/* 6E have many channel need to send by serval commnd */
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd6E_1 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd6E_2 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd6E_3 = NULL;

	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdLegacy_6G_1 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdLegacy_6G_2 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdLegacy_6G_3 = NULL;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_1 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_2 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_3 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_4 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_5 = NULL;
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdEHT_6G_6 = NULL;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	uint32_t rStatus;
	uint16_t u2DefaultTableIndex;
	uint32_t u4SetCmdTableMaxSize;
	uint32_t u4SetQueryInfoLen;
	uint8_t bandedgeParam[4] = { 0, 0, 0, 0 };
	uint8_t *pu1PwrLmtCountryCode;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	/* TODO : 5G band edge */

	if (regd_is_single_sku_en()) {
		rlmDomainSendPwrLimitCmd_V2(prAdapter);
		return;
	}


	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	if (prDomainInfo) {
		bandedgeParam[0] = prDomainInfo->rSubBand[0].ucFirstChannelNum;
		bandedgeParam[1] = bandedgeParam[0] +
			prDomainInfo->rSubBand[0].ucNumChannels - 1;
	}

	u4SetCmdTableMaxSize =
	    sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT);

	prCmd = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmd) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmd, u4SetCmdTableMaxSize);

	prCmdHE = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdHE) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdHE, u4SetCmdTableMaxSize);

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_1 = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_1) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_1, u4SetCmdTableMaxSize);

	prCmdEHT_2 = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_2) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_2, u4SetCmdTableMaxSize);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	prCmdAnt = cnmMemAlloc(prAdapter, RAM_TYPE_BUF,
		u4SetCmdTableMaxSize);

	if (!prCmdAnt) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdAnt, u4SetCmdTableMaxSize);
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	prCmd6E_1 = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmd6E_1) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmd6E_1, u4SetCmdTableMaxSize);

	prCmd6E_2 = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmd6E_2) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmd6E_2, u4SetCmdTableMaxSize);

	prCmd6E_3 = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmd6E_3) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmd6E_3, u4SetCmdTableMaxSize);

	prCmdLegacy_6G_1 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdLegacy_6G_1) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdLegacy_6G_1, u4SetCmdTableMaxSize);

	prCmdLegacy_6G_2 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdLegacy_6G_2) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdLegacy_6G_2, u4SetCmdTableMaxSize);

	prCmdLegacy_6G_3 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdLegacy_6G_3) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdLegacy_6G_3, u4SetCmdTableMaxSize);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_6G_1 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_1) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_1, u4SetCmdTableMaxSize);

	prCmdEHT_6G_2 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_2) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_2, u4SetCmdTableMaxSize);

	prCmdEHT_6G_3 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_3) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_3, u4SetCmdTableMaxSize);

	prCmdEHT_6G_4 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_4) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_4, u4SetCmdTableMaxSize);

	prCmdEHT_6G_5 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_5) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_5, u4SetCmdTableMaxSize);

	prCmdEHT_6G_6 =
		cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);

	if (!prCmdEHT_6G_6) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdEHT_6G_6, u4SetCmdTableMaxSize);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	u2DefaultTableIndex =
	    rlmDomainPwrLimitDefaultTableDecision(prAdapter,
		prAdapter->rWifiVar.u2CountryCode);

	if (u2DefaultTableIndex == POWER_LIMIT_TABLE_NULL) {
		DBGLOG(RLM, ERROR,
					   "Can't find any table index!\n");
		goto err;
	}

	pu1PwrLmtCountryCode = &g_rRlmPowerLimitDefault
				[u2DefaultTableIndex]
				.aucCountryCode[0];

	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmd->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdHE->u2CountryCode);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_1->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_2->u2CountryCode);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmd6E_1->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmd6E_2->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmd6E_3->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdLegacy_6G_1->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdLegacy_6G_2->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdLegacy_6G_3->u2CountryCode);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_1->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_2->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_3->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_4->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_5->u2CountryCode);
	WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
		&prCmdEHT_6G_6->u2CountryCode);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	if (prCmd->u2CountryCode == COUNTRY_CODE_NULL)
		DBGLOG(RLM, TRACE,
			   "CC={0x00,0x00},Power Limit use Default setting!");


	/* Initialize channel number */
	prCmd->ucNum = 0;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	prCmd->ucLimitType = PWR_LIMIT_TYPE_COMP_11AC_V2;
#else
	prCmd->ucLimitType = PWR_LIMIT_TYPE_COMP_11AC;
#endif
	prCmd->ucVersion = 1;

	prCmdHE->ucNum = 0;
	prCmdHE->fgPwrTblKeep = TRUE;
	prCmdHE->ucLimitType = PWR_LIMIT_TYPE_COMP_11AX;

	if (prAdapter->rWifiVar.ucSta5gBandwidth == MAX_BW_160MHZ)
		prCmdHE->ucLimitType = PWR_LIMIT_TYPE_COMP_11AX_BW160;

	prCmdHE->ucVersion = 1;

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_1->ucNum = 0;
	prCmdEHT_1->fgPwrTblKeep = TRUE;
	prCmdEHT_1->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_1;
	prCmdEHT_1->ucVersion = 1;

	prCmdEHT_2->ucNum = 0;
	prCmdEHT_2->fgPwrTblKeep = TRUE;
	prCmdEHT_2->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_2;
	prCmdEHT_2->ucVersion = 1;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	prCmdAnt->ucNum = 0;
	prCmdAnt->fgPwrTblKeep = TRUE;
	/* ANT number if PWR_LIMIT_TYPE_COMP_ANT*/
	prCmdAnt->ucLimitType = PWR_LIMIT_TYPE_COMP_ANT_V2;
	prCmdAnt->ucVersion = 1;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	prCmd6E_1->ucNum = 0;
	prCmd6E_1->fgPwrTblKeep = TRUE;
	prCmd6E_1->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_1;

	prCmd6E_2->ucNum = 0;
	prCmd6E_2->fgPwrTblKeep = TRUE;
	prCmd6E_2->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_2;

	prCmd6E_3->ucNum = 0;
	prCmd6E_3->fgPwrTblKeep = TRUE;
	prCmd6E_3->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_3;

	prCmdLegacy_6G_1->ucNum = 0;
	prCmdLegacy_6G_1->fgPwrTblKeep = TRUE;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	prCmdLegacy_6G_1->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1;
#else
	prCmdLegacy_6G_1->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_6G_1;
#endif

	prCmdLegacy_6G_2->ucNum = 0;
	prCmdLegacy_6G_2->fgPwrTblKeep = TRUE;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	prCmdLegacy_6G_2->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2;
#else
	prCmdLegacy_6G_2->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_6G_2;
#endif

	prCmdLegacy_6G_3->ucNum = 0;
	prCmdLegacy_6G_3->fgPwrTblKeep = TRUE;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	prCmdLegacy_6G_3->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3;
#else
	prCmdLegacy_6G_3->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_6G_3;
#endif

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_6G_1->ucNum = 0;
	prCmdEHT_6G_1->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_1->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_1;
	prCmdEHT_6G_1->ucVersion = 1;

	prCmdEHT_6G_2->ucNum = 0;
	prCmdEHT_6G_2->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_2->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_2;
	prCmdEHT_6G_2->ucVersion = 1;

	prCmdEHT_6G_3->ucNum = 0;
	prCmdEHT_6G_3->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_3->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_3;
	prCmdEHT_6G_3->ucVersion = 1;

	prCmdEHT_6G_4->ucNum = 0;
	prCmdEHT_6G_4->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_4->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_4;
	prCmdEHT_6G_4->ucVersion = 1;

	prCmdEHT_6G_5->ucNum = 0;
	prCmdEHT_6G_5->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_5->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_5;
	prCmdEHT_6G_5->ucVersion = 1;

	prCmdEHT_6G_6->ucNum = 0;
	prCmdEHT_6G_6->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_6->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_6;
	prCmdEHT_6G_6->ucVersion = 1;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prCmd,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmd);

#if (CFG_SUPPORT_PWR_LIMIT_HE == 1)
	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prCmdHE,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdHE);
#endif

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_1,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_1);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_2,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_2);

#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if (CFG_SUPPORT_WIFI_6G == 1)
		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmd6E_1,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmd6E_1);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmd6E_2,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmd6E_2);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmd6E_3,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmd6E_3);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdLegacy_6G_1,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdLegacy_6G_1);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdLegacy_6G_2,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdLegacy_6G_2);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdLegacy_6G_3,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdLegacy_6G_3);

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_1,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_1);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_2,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_2);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_3,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_3);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_4,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_4);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_5,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_5);

		/*<1>Command - default table information,
		 *fill all subband
		 */
		rlmDomainBuildCmdByDefaultTable(prCmdEHT_6G_6,
			u2DefaultTableIndex);

		/*<2>Command - configuration table information,
		 * replace specified channel
		 */
		rlmDomainBuildCmdByConfigTable(prAdapter,
			prCmdEHT_6G_6);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */


	DBGLOG(RLM, TRACE,
	       "Domain: ValidCC=%c%c, PwrLimitCC=%c%c, PwrLimitChNum=%d\n",
	       (prAdapter->rWifiVar.u2CountryCode & 0xff00) >> 8,
	       (prAdapter->rWifiVar.u2CountryCode & 0x00ff),
	       ((prCmd->u2CountryCode & 0xff00) >> 8),
	       (prCmd->u2CountryCode & 0x00ff),
	       prCmd->ucNum);

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
	rlmDomainShowPwrLimitPerCh("Old", prCmd);
	/* apply each setting into country channel power table */
	txPwrCtrlApplySettings(prAdapter, prCmd, bandedgeParam);
	/* show tx power table after applying setting */
	rlmDomainShowPwrLimitPerCh("Final", prCmd);

#if (CFG_SUPPORT_PWR_LIMIT_HE == 1)
	rlmDomainShowPwrLimitPerCh("Old", prCmdHE);
	txPwrCtrlApplySettings(prAdapter, prCmdHE, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdHE);
#endif /*#if (CFG_SUPPORT_PWR_LIMIT_HE == 1)*/

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_1);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_1, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_1);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_2);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_2, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_2);
#endif

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	txPwrCtrlApplySettings(prAdapter, prCmdAnt, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdAnt);
#endif

#if (CFG_SUPPORT_WIFI_6G == 1)
	rlmDomainShowPwrLimitPerCh("Old", prCmd6E_1);
	txPwrCtrlApplySettings(prAdapter, prCmd6E_1, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmd6E_1);

	rlmDomainShowPwrLimitPerCh("Old", prCmd6E_2);
	txPwrCtrlApplySettings(prAdapter, prCmd6E_2, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmd6E_2);

	rlmDomainShowPwrLimitPerCh("Old", prCmd6E_3);
	txPwrCtrlApplySettings(prAdapter, prCmd6E_3, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmd6E_3);

	rlmDomainShowPwrLimitPerCh("Old", prCmdLegacy_6G_1);
	txPwrCtrlApplySettings(prAdapter, prCmdLegacy_6G_1, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdLegacy_6G_1);

	rlmDomainShowPwrLimitPerCh("Old", prCmdLegacy_6G_2);
	txPwrCtrlApplySettings(prAdapter, prCmdLegacy_6G_2, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdLegacy_6G_2);

	rlmDomainShowPwrLimitPerCh("Old", prCmdLegacy_6G_3);
	txPwrCtrlApplySettings(prAdapter, prCmdLegacy_6G_3, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdLegacy_6G_3);

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_1);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_1, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_1);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_2);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_2, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_2);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_3);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_3, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_3);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_4);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_4, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_4);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_5);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_5, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_5);

	rlmDomainShowPwrLimitPerCh("Old", prCmdEHT_6G_6);
	txPwrCtrlApplySettings(prAdapter, prCmdEHT_6G_6, bandedgeParam);
	rlmDomainShowPwrLimitPerCh("Final", prCmdEHT_6G_6);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT */

	u4SetQueryInfoLen =
		sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT);

	/* Update domain info to chip */
	if (prCmd->ucNum <= MAX_CMD_SUPPORT_CHANNEL_NUM) {
		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
				TRUE,	/* fgSetQuery */
				FALSE,	/* fgNeedResp */
				FALSE,	/* fgIsOid */
				NULL,	/* pfCmdDoneHandler */
				NULL,	/* pfCmdTimeoutHandler */
				u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
				(uint8_t *) prCmd,	/* pucInfoBuffer */
				NULL,	/* pvSetQueryBuffer */
				0	/* u4SetQueryBufferLen */
		    );
		if (rStatus != WLAN_STATUS_PENDING)
			DBGLOG(RLM, ERROR, "Power limit channel 0x%08x\n",
				rStatus);
#if (CFG_SUPPORT_PWR_LIMIT_HE == 1)
		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
				TRUE,	/* fgSetQuery */
				FALSE,	/* fgNeedResp */
				FALSE,	/* fgIsOid */
				NULL,	/* pfCmdDoneHandler */
				NULL,	/* pfCmdTimeoutHandler */
				u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
				(uint8_t *) prCmdHE,	/* pucInfoBuffer */
				NULL,	/* pvSetQueryBuffer */
				0	/* u4SetQueryBufferLen */
		    );
		if (rStatus != WLAN_STATUS_PENDING)
			DBGLOG(RLM, ERROR, "Power limit channel HE 0x%08x\n",
				rStatus);

#endif
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_1,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

	   rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_2,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if (CFG_SUPPORT_WIFI_6G == 1)
		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmd6E_1,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmd6E_2,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmd6E_3,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdLegacy_6G_1,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdLegacy_6G_2,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdLegacy_6G_3,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_1,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_2,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_3,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_4,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_5,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

		rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
			CMD_ID_SET_COUNTRY_POWER_LIMIT,	/* ucCID */
			TRUE,	/* fgSetQuery */
			FALSE,	/* fgNeedResp */
			FALSE,	/* fgIsOid */
			NULL,	/* pfCmdDoneHandler */
			NULL,	/* pfCmdTimeoutHandler */
			u4SetQueryInfoLen,	/* u4SetQueryInfoLen */
			(uint8_t *) prCmdEHT_6G_6,	/* pucInfoBuffer */
			NULL,	/* pvSetQueryBuffer */
			0	/* u4SetQueryBufferLen */
	    );

#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
	} else {
		DBGLOG(RLM, ERROR, "Domain: illegal power limit table\n");
	}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG

	u4SetQueryInfoLenAnt =
		sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT);

	rStatus = wlanSendSetQueryCmd(prAdapter,	/* prAdapter */
				CMD_ID_SET_COUNTRY_POWER_LIMIT, /* ucCID */
				TRUE,	/* fgSetQuery */
				FALSE,	/* fgNeedResp */
				FALSE,	/* fgIsOid */
				NULL,	/* pfCmdDoneHandler */
				NULL,	/* pfCmdTimeoutHandler */
				u4SetQueryInfoLenAnt,	/* u4SetQueryInfoLen */
				(uint8_t *) prCmdAnt,	/* pucInfoBuffer */
				NULL,	/* pvSetQueryBuffer */
				0	/* u4SetQueryBufferLen */
			);
	if (rStatus != WLAN_STATUS_PENDING)
		DBGLOG(RLM, ERROR, "Power limit revise 0x%08x\n", rStatus);
#endif

err:
	cnmMemFree(prAdapter, prCmd);
	cnmMemFree(prAdapter, prCmdHE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	cnmMemFree(prAdapter, prCmdEHT_1);
	cnmMemFree(prAdapter, prCmdEHT_2);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	cnmMemFree(prAdapter, prCmdAnt);
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	cnmMemFree(prAdapter, prCmd6E_1);
	cnmMemFree(prAdapter, prCmd6E_2);
	cnmMemFree(prAdapter, prCmd6E_3);
	cnmMemFree(prAdapter, prCmdLegacy_6G_1);
	cnmMemFree(prAdapter, prCmdLegacy_6G_2);
	cnmMemFree(prAdapter, prCmdLegacy_6G_3);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	cnmMemFree(prAdapter, prCmdEHT_6G_1);
	cnmMemFree(prAdapter, prCmdEHT_6G_2);
	cnmMemFree(prAdapter, prCmdEHT_6G_3);
	cnmMemFree(prAdapter, prCmdEHT_6G_4);
	cnmMemFree(prAdapter, prCmdEHT_6G_5);
	cnmMemFree(prAdapter, prCmdEHT_6G_6);
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
}
#endif
u_int8_t regd_is_single_sku_en(void)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	return g_mtk_regd_control.en;
#else
	return FALSE;
#endif
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
enum regd_state rlmDomainGetCtrlState(void)
{
	return g_mtk_regd_control.state;
}


void rlmDomainResetActiveChannel(void)
{
	g_mtk_regd_control.n_channel_active_2g = 0;
	g_mtk_regd_control.n_channel_active_5g = 0;
	g_mtk_regd_control.n_channel_active_6g = 0;
}

void rlmDomainAddActiveChannel(u8 band)

{
	if (band == KAL_BAND_2GHZ)
		g_mtk_regd_control.n_channel_active_2g += 1;
	else if (band == KAL_BAND_5GHZ)
		g_mtk_regd_control.n_channel_active_5g += 1;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (band == KAL_BAND_6GHZ)
		g_mtk_regd_control.n_channel_active_6g += 1;
#endif
}

u8 rlmDomainGetActiveChannelCount(u8 band)
{
	if (band == KAL_BAND_2GHZ)
		return g_mtk_regd_control.n_channel_active_2g;
	else if (band == KAL_BAND_5GHZ)
		return g_mtk_regd_control.n_channel_active_5g;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (band == KAL_BAND_6GHZ)
		return g_mtk_regd_control.n_channel_active_6g;
#endif
	else
		return 0;
}

struct CMD_DOMAIN_CHANNEL *rlmDomainGetActiveChannels(void)
{
	return g_mtk_regd_control.channels;
}

void rlmDomainSetDefaultCountryCode(void)
{
	g_mtk_regd_control.alpha2 = COUNTRY_CODE_WW;
}

void rlmDomainResetCtrlInfo(u_int8_t force)
{
	if ((g_mtk_regd_control.state == REGD_STATE_UNDEFINED) ||
	    (force == TRUE)) {
		memset(&g_mtk_regd_control, 0, sizeof(struct mtk_regd_control));

		g_mtk_regd_control.state = REGD_STATE_INIT;

		rlmDomainSetDefaultCountryCode();
	}
}

u_int8_t rlmDomainIsUsingLocalRegDomainDataBase(void)
{
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
	return TRUE;
#else
	return FALSE;
#endif
}

bool rlmDomainIsSameCountryCode(char *alpha2, u8 size_of_alpha2)
{
	u8 idx;
	u32 alpha2_hex = 0;

	for (idx = 0; idx < size_of_alpha2; idx++)
		alpha2_hex |= (alpha2[idx] << (idx * 8));

	return (rlmDomainGetCountryCode() == alpha2_hex) ? TRUE : FALSE;
}

void rlmDomainSetCountryCode(char *alpha2, u8 size_of_alpha2)
{
	u8 max;
	u8 buf_size;

	buf_size = sizeof(g_mtk_regd_control.alpha2);
	max = (buf_size < size_of_alpha2) ? buf_size : size_of_alpha2;

	g_mtk_regd_control.alpha2 = rlmDomainAlpha2ToU32(alpha2, max);
}

void rlmDomainSetDfsRegion(enum nl80211_dfs_regions dfs_region)
{
	g_mtk_regd_control.dfs_region = dfs_region;
}

enum nl80211_dfs_regions rlmDomainGetDfsRegion(void)
{
	return g_mtk_regd_control.dfs_region;
}

void rlmDomainSetDfsDbdcBand(enum ENUM_MBMC_BN eDBDCBand)
{
	g_mtk_regd_control.eDBDCBand = eDBDCBand;
}

enum ENUM_MBMC_BN rlmDomainGetDfsDbdcBand(void)
{
	return g_mtk_regd_control.eDBDCBand;
}

void rlmDomainSetTempCountryCode(char *alpha2, u8 size_of_alpha2)
{
	u8 idx, max;
	u8 buf_size;

	buf_size = sizeof(g_mtk_regd_control.tmp_alpha2);
	max = (buf_size < size_of_alpha2) ? buf_size : size_of_alpha2;

	g_mtk_regd_control.tmp_alpha2 = 0;

	for (idx = 0; idx < max; idx++)
		g_mtk_regd_control.tmp_alpha2 |= (alpha2[idx] << (idx * 8));

}

enum regd_state rlmDomainStateTransition(
				enum regd_state request_state)
{
	enum regd_state next_state, old_state;
	bool the_same = 0;

	old_state = g_mtk_regd_control.state;
	next_state = REGD_STATE_INVALID;

	if (old_state == REGD_STATE_INVALID)
		DBGLOG(RLM, ERROR,
		       "%s(): invalid state. trasntion is not allowed.\n",
		       __func__);

	switch (request_state) {
	case REGD_STATE_SET_WW_CORE:
		if ((old_state == REGD_STATE_SET_WW_CORE) ||
		    (old_state == REGD_STATE_INIT) ||
		    (old_state == REGD_STATE_SET_COUNTRY_USER) ||
		    (old_state == REGD_STATE_SET_COUNTRY_IE))
			next_state = request_state;
		break;
	case REGD_STATE_SET_COUNTRY_USER:
		/* Allow user to set multiple times */
		if ((old_state == REGD_STATE_SET_WW_CORE) ||
		    (old_state == REGD_STATE_INIT) ||
		    (old_state == REGD_STATE_SET_COUNTRY_USER) ||
		    (old_state == REGD_STATE_SET_COUNTRY_IE))
			next_state = request_state;
		else
			DBGLOG(RLM, ERROR, "Invalid old state = %d\n",
			       old_state);
		break;
	case REGD_STATE_SET_COUNTRY_DRIVER:
		if (old_state == REGD_STATE_SET_COUNTRY_USER) {
			/*
			 * Error.
			 * Mixing using set_country_by_user and
			 * set_country_by_driver is not allowed.
			 */
			break;
		}

		next_state = request_state;
		break;
	case REGD_STATE_SET_COUNTRY_IE:
		next_state = request_state;
		break;
	default:
		break;
	}

	if (next_state == REGD_STATE_INVALID) {
		DBGLOG(RLM, ERROR,
		       "%s():  ERROR. trasntion to invalid state. o=%x, r=%x, s=%x\n",
		       __func__, old_state, request_state, the_same);
	} else
		DBGLOG(RLM, INFO, "%s():  trasntion to state = %x (old = %x)\n",
		__func__, next_state, g_mtk_regd_control.state);

	g_mtk_regd_control.state = next_state;

	return g_mtk_regd_control.state;
}

void rlmDomainParsingChannel(struct ADAPTER *prAdapter)
{
	struct CMD_DOMAIN_CHANNEL *pCh;
	u_int8_t fgDisconnection = FALSE;
	struct GLUE_INFO *prGlueInfo;
	uint8_t ucChannelNum = 0;
#if (CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED == 1)
	uint32_t rStatus, u4BufLen;
#endif

	if (!prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter = NULL.\n");
		return;
	}

	prGlueInfo = prAdapter->prGlueInfo;

#if (CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED == 1)
	/* Retrieve connected channel */
	if (prGlueInfo && kalGetMediaStateIndicated(prGlueInfo) ==
		MEDIA_STATE_CONNECTED) {
		ucChannelNum =
			wlanGetChannelNumberByNetwork(prGlueInfo->prAdapter,
			   prGlueInfo->prAdapter->prAisBssInfo->ucBssIndex);
	}
#endif
	/*
	 * Ready to parse the channel for bands
	 */

	rlmDomainResetActiveChannel();

	pCh = rlmDomainGetActiveChannels();

	fgDisconnection = kalFillChannels(prGlueInfo,
			pCh,
			MAX_SUPPORTED_CH_COUNT,
			ucChannelNum,
#if (CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED == 1)
			TRUE
#else
			FALSE
#endif
		);

#if (CFG_SUPPORT_REGD_UPDATE_DISCONNECT_ALLOWED == 1)
	/* Disconnect with AP if connected channel is disabled in new country */
	if (fgDisconnection) {
		DBGLOG(RLM, STATE, "%s(): Disconnect! CH%d is DISABLED\n",
			__func__, ucChannelNum);
		rStatus = kalIoctl(prGlueInfo, wlanoidSetDisassociate,
				   NULL, 0, &u4BufLen);

		if (rStatus != WLAN_STATUS_SUCCESS)
			DBGLOG(RLM, WARN, "disassociate error:%lx\n", rStatus);
	}
#endif
}

void rlmExtractChannelInfo(u32 max_ch_count,
			   struct CMD_DOMAIN_ACTIVE_CHANNEL_LIST *prBuff)
{
	u32 ch_count, idx;
	struct CMD_DOMAIN_CHANNEL *pCh;

	prBuff->u1ActiveChNum2g = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ);
	prBuff->u1ActiveChNum5g = rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ);
#if (CFG_SUPPORT_WIFI_6G == 1)
	prBuff->u1ActiveChNum6g = rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ);
#else
	prBuff->u1ActiveChNum6g = 0;
#endif
	ch_count = prBuff->u1ActiveChNum2g +
			prBuff->u1ActiveChNum5g +
			prBuff->u1ActiveChNum6g;

	if (ch_count > max_ch_count) {
		ch_count = max_ch_count;
		DBGLOG(RLM, WARN,
		       "%s(); active channel list is not a complete one.\n",
		       __func__);
	}

	for (idx = 0; idx < ch_count; idx++) {
		pCh = &(prBuff->arChannels[idx]);

		pCh->u2ChNum = (rlmDomainGetActiveChannels() + idx)->u2ChNum;
		pCh->eFlags = (rlmDomainGetActiveChannels() + idx)->eFlags;
	}

}

const struct ieee80211_regdomain
*rlmDomainSearchRegdomainFromLocalDataBase(char *alpha2)
{
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
	u8 idx;
	const struct mtk_regdomain *prRegd;

	idx = 0;
	while (g_prRegRuleTable[idx]) {
		prRegd = g_prRegRuleTable[idx];

		if ((prRegd->country_code[0] == alpha2[0]) &&
			(prRegd->country_code[1] == alpha2[1]) &&
			(prRegd->country_code[2] == alpha2[2]) &&
			(prRegd->country_code[3] == alpha2[3]))
			return prRegd->prRegdRules;

		idx++;
	}

	return NULL; /*default world wide*/
#else
	return NULL;
#endif
}


const struct ieee80211_regdomain *rlmDomainGetLocalDefaultRegd(void)
{
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
	return &default_regdom_ww;
#else
	return NULL;
#endif
}
struct GLUE_INFO *rlmDomainGetGlueInfo(void)
{
	return g_mtk_regd_control.pGlueInfo;
}

bool rlmDomainIsEfuseUsed(void)
{
	return g_mtk_regd_control.isEfuseCountryCodeUsed;
}

uint8_t rlmDomainGetChannelBw(enum ENUM_BAND eBand, uint8_t channelNum)
{
	uint32_t ch_idx = 0, start_idx = 0, end_idx = 0;
#if (CFG_SUPPORT_WIFI_6G == 1)
	uint8_t channelBw = MAX_BW_320_2MHZ;
#else
	uint8_t channelBw = MAX_BW_80_80_MHZ;
#endif
	struct CMD_DOMAIN_CHANNEL *pCh;
	enum ENUM_BAND eChBand;

	//TODO: remove this
	channelBw = MAX_BW_320_2MHZ;

	end_idx = rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ)
#if (CFG_SUPPORT_WIFI_6G == 1)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_6GHZ)
#endif
		;

	for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {
		pCh = (rlmDomainGetActiveChannels() + ch_idx);

		if (ch_idx < rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ))
			eChBand = BAND_2G4;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (ch_idx < rlmDomainGetActiveChannelCount(KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(KAL_BAND_5GHZ))
			eChBand = BAND_5G;
		else
			eChBand = BAND_6G;
#else
		else
			eChBand = BAND_5G;
#endif

		if ((eChBand != eBand) || (pCh->u2ChNum != channelNum))
			continue;

		DBGLOG(RLM, TRACE, "band=%d, ch=%d, flags=%x\n",
			eBand, channelNum, pCh->eFlags);

		/* Max BW */
		if ((pCh->eFlags & IEEE80211_CHAN_NO_160MHZ)
						== IEEE80211_CHAN_NO_160MHZ)
			channelBw = MAX_BW_80MHZ;
		if ((pCh->eFlags & IEEE80211_CHAN_NO_80MHZ)
						== IEEE80211_CHAN_NO_80MHZ)
			channelBw = MAX_BW_40MHZ;
		if ((pCh->eFlags & IEEE80211_CHAN_NO_HT40)
						== IEEE80211_CHAN_NO_HT40)
			channelBw = MAX_BW_20MHZ;
	}

	DBGLOG(RLM, TRACE, "ch=%d, BW=%d\n", channelNum, channelBw);
	return channelBw;
}
#endif

uint32_t rlmDomainExtractSingleSkuInfoFromFirmware(struct ADAPTER *prAdapter,
						   uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	struct SINGLE_SKU_INFO *prSkuInfo =
			(struct SINGLE_SKU_INFO *) pucEventBuf;

	g_mtk_regd_control.en = TRUE;

	if (prSkuInfo->isEfuseValid) {
		if (!rlmDomainIsUsingLocalRegDomainDataBase()) {

			DBGLOG(RLM, ERROR,
				"Error. In efuse mode, must use local data base.\n");

			ASSERT(0);
			/* force using local db if getting
			 * country code from efuse
			 */
			return WLAN_STATUS_NOT_SUPPORTED;
		}

		rlmDomainSetCountryCode(
			(char *) &prSkuInfo->u4EfuseCountryCode,
			sizeof(prSkuInfo->u4EfuseCountryCode));

		g_mtk_regd_control.isEfuseCountryCodeUsed = TRUE;
	}
#endif

	return WLAN_STATUS_SUCCESS;
}

void rlmDomainSendInfoToFirmware(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	if (!regd_is_single_sku_en())
		return; /*not support single sku*/

	g_mtk_regd_control.pGlueInfo = prAdapter->prGlueInfo;
	rlmDomainSetCountry(prAdapter);
#endif
}

enum ENUM_CHNL_EXT rlmSelectSecondaryChannelType(struct ADAPTER *prAdapter,
						 enum ENUM_BAND band,
						 u8 primary_ch)
{
	enum ENUM_CHNL_EXT eSCO = CHNL_EXT_SCN;
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	if (band == BAND_5G) {
		switch ((uint32_t)primary_ch) {
		case 36:
		case 44:
		case 52:
		case 60:
		case 100:
		case 108:
		case 116:
		case 124:
		case 132:
		case 140:
		case 149:
		case 157:
			eSCO = CHNL_EXT_SCA;
			break;
		case 40:
		case 48:
		case 56:
		case 64:
		case 104:
		case 112:
		case 120:
		case 128:
		case 136:
		case 144:
		case 153:
		case 161:
			eSCO = CHNL_EXT_SCB;
			break;
		case 165:
		default:
			eSCO = CHNL_EXT_SCN;
			break;
		}
	} else {
		u8 below_ch, above_ch;

		below_ch = primary_ch - CHNL_SPAN_20;
		above_ch = primary_ch + CHNL_SPAN_20;

		if (rlmDomainIsLegalChannel(prAdapter, band, above_ch))
			return CHNL_EXT_SCA;

		if (rlmDomainIsLegalChannel(prAdapter, band, below_ch))
			return CHNL_EXT_SCB;
	}

#endif
	return eSCO;
}

void rlmDomainOidSetCountry(struct ADAPTER *prAdapter, char *country,
			    u8 size_of_country)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)

	if (rlmDomainIsUsingLocalRegDomainDataBase()) {

		if (rlmDomainIsSameCountryCode(country, size_of_country)) {
			char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};

			rlmDomainU32ToAlpha(
				rlmDomainGetCountryCode(), acCountryCodeStr);
			DBGLOG(RLM, WARN,
				"Same as current country %s, skip!\n",
				acCountryCodeStr);
			return;
		}
		rlmDomainSetCountryCode(country, size_of_country);
		rlmDomainSetCountry(prAdapter);
	} else {
		DBGLOG(RLM, INFO,
		       "%s(): Using driver hint to query CRDA getting regd.\n",
		       __func__);
		kalRegulatoryHint(country);
	}
#endif
}

u32 rlmDomainGetCountryCode(void)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	return g_mtk_regd_control.alpha2;
#else
	return 0;
#endif
}

void rlmDomainAssert(u_int8_t cond)
{
	/* bypass this check because single sku is not enable */
	if (!regd_is_single_sku_en())
		return;

	if (!cond) {
		KAL_WARN_ON(1);
		DBGLOG(RLM, ERROR, "[WARNING!!] RLM unexpected case.\n");
	}

}

void rlmDomainU32ToAlpha(uint32_t u4CountryCode, char *pcAlpha)
{
	uint8_t ucIdx;

	for (ucIdx = 0; ucIdx < MAX_COUNTRY_CODE_LEN; ucIdx++)
		pcAlpha[ucIdx] = ((u4CountryCode >> (ucIdx * 8)) & 0xff);
}
#if 0
#if (CFG_SUPPORT_SINGLE_SKU == 1)
void rlm_get_alpha2(char *alpha2)
{
	rlmDomainU32ToAlpha(g_mtk_regd_control.alpha2, alpha2);
}
EXPORT_SYMBOL(rlm_get_alpha2);
#endif
#endif
