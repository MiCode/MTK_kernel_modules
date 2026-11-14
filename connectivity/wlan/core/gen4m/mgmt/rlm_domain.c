// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
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
#if CFG_EXT_FEATURE
#include "rlm_txpwr_init_ext.h"
#endif
#include "gl_kal.h"
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
#include "hif_pdma.h"
#endif

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
#include "he_ie.h"
#endif

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
#include "rlm_txpwr_limit_emi.h"
#else
#include "rlm_txpwr_limit.h"
#endif
#if CFG_SUPPORT_NAN
#include "nanScheduler.h"
#endif
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

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
char *g_au1TxPwrRFBand[PWR_LIMIT_RF_BAND_NUM] = {
	"2.4G", /* PWR_LIMIT_RF_BAND_2G4 */
	"5G", /* PWR_LIMIT_RF_BAND_5G */
#if (CFG_SUPPORT_WIFI_6G == 1)
	"6G", /* PWR_LIMIT_RF_BAND_6G */
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
};

char *g_au1TxPwrProtocol[PWR_LIMIT_PROTOCOL_NUM] = {
	"LEGACY", /* PWR_LIMIT_PROTOCOL_LEGACY */
	"HE", /* PWR_LIMIT_PROTOCOL_HE */
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	"EHT", /* PWR_LIMIT_PROTOCOL_EHT */
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT == 1 */
};
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

typedef int32_t (*PFN_TX_PWR_TAG_PARA_FUNC) (
	char *, char *, uint8_t, struct TX_PWR_CTRL_ELEMENT *);

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
struct TX_PWR_ANT_EXT_OPTION g_arTxPwrAntExtOptionTbl[PWR_LMT_ANT_EXT_NUM] = {
	{"MAIN", PWR_LMT_ANT_EXT_MAIN, 0}, /* default */
	{"AUX", PWR_LMT_ANT_EXT_AUX, BIT(ANT_EXT_BIT_ANTSWAP_AUX)},
};

struct TX_PWR_ANT_CFG_PARA_TABLE g_auTxPwrAntBandCfgTbl[] = {
	{
		"1", /* 2.4G only */
		ANT_CFG_SUBBAND_NUM_2G4,
		PWR_LMT_CHAIN_2G4_BAND,
		PWR_LMT_CHAIN_2G4_BAND,
	},
	{
		"2", /* 2.4G + 5GBand1~5GBand4 */
		ANT_CFG_SUBBAND_NUM_2G4 + ANT_CFG_SUBBAND_NUM_5G,
		PWR_LMT_CHAIN_2G4_BAND,
		PWR_LMT_CHAIN_5G_BAND4,
	},
	{
		"3", /* 2.4G + 5GBand1~5GBand4 + 6GBand1~6GBand4 */
		ANT_CFG_SUBBAND_NUM_2G4 +
			ANT_CFG_SUBBAND_NUM_5G +
			ANT_CFG_SUBBAND_NUM_6G,
		PWR_LMT_CHAIN_2G4_BAND,
		PWR_LMT_CHAIN_6G_BAND4,
	},
	{
		"ALL_BAND",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_2G4_BAND,
		PWR_LMT_CHAIN_6G_BAND4,
	},
	{
		"2G4",
		ANT_CFG_SUBBAND_NUM_2G4,
		PWR_LMT_CHAIN_2G4_BAND,
		PWR_LMT_CHAIN_2G4_BAND,
	},
	{
		"5G",
		ANT_CFG_SUBBAND_NUM_5G,
		PWR_LMT_CHAIN_5G_BAND1,
		PWR_LMT_CHAIN_5G_BAND4,
	},
	{
		"5GBAND1",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_5G_BAND1,
		PWR_LMT_CHAIN_5G_BAND1,
	},
	{
		"5GBAND2",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_5G_BAND2,
		PWR_LMT_CHAIN_5G_BAND2,
	},
	{
		"5GBAND3",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_5G_BAND3,
		PWR_LMT_CHAIN_5G_BAND3,
	},
	{
		"5GBAND4",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_5G_BAND4,
		PWR_LMT_CHAIN_5G_BAND4,
	},
	{
		"6G",
		ANT_CFG_SUBBAND_NUM_6G,
		PWR_LMT_CHAIN_6G_BAND1,
		PWR_LMT_CHAIN_6G_BAND4,
	},
	{
		"6GBAND1",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_6G_BAND1,
		PWR_LMT_CHAIN_6G_BAND1,
	},
	{
		"6GBAND2",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_6G_BAND2,
		PWR_LMT_CHAIN_6G_BAND2,
	},
	{
		"6GBAND3",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_6G_BAND3,
		PWR_LMT_CHAIN_6G_BAND3,
	},
	{
		"6GBAND4",
		ANT_CFG_SUBBAND_NUM_SINGLE,
		PWR_LMT_CHAIN_6G_BAND4,
		PWR_LMT_CHAIN_6G_BAND4,
	},
};


struct TX_PWR_ANT_CFG_PARA_TABLE g_auTxPwrAntChainCfgTbl[] = {
	{
		"1", /* WF0 only */
		ANT_CFG_CHAIN_NUM_WF0,
		PWR_LMT_CHAIN_ANT_WF0,
		PWR_LMT_CHAIN_ANT_WF0,
	},
	{
		"2", /* WF0 + WF1 */
		ANT_CFG_CHAIN_NUM_WF0 + ANT_CFG_CHAIN_NUM_WF1,
		PWR_LMT_CHAIN_ANT_WF0,
		PWR_LMT_CHAIN_ANT_WF1,
	},
	{
		"3", /* WF0 + WF1 + WF2 */
		ANT_CFG_CHAIN_NUM_WF0 +
			ANT_CFG_CHAIN_NUM_WF1 +
			ANT_CFG_CHAIN_NUM_WF2,
		PWR_LMT_CHAIN_ANT_WF0,
		PWR_LMT_CHAIN_ANT_WF2,
	},
	{
		"ALL_WF",
		ANT_CFG_CHAIN_NUM_SINGLE,
		PWR_LMT_CHAIN_ANT_WF0,
		PWR_LMT_CHAIN_ANT_WF2,
	},
	{
		"WF0",
		ANT_CFG_CHAIN_NUM_SINGLE,
		PWR_LMT_CHAIN_ANT_WF0,
		PWR_LMT_CHAIN_ANT_WF0,
	},
	{
		"WF1",
		ANT_CFG_CHAIN_NUM_SINGLE,
		PWR_LMT_CHAIN_ANT_WF1,
		PWR_LMT_CHAIN_ANT_WF1,
	},
	{
		"WF2",
		ANT_CFG_CHAIN_NUM_SINGLE,
		PWR_LMT_CHAIN_ANT_WF2,
		PWR_LMT_CHAIN_ANT_WF2,
	},
};
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */

struct TX_PWR_TAG_TABLE {
	const char arTagNames[32];
	uint8_t ucCategory;
	uint8_t ucTagIdx;
	int8_t icInitVal;
	uint8_t ucTagParaNum;
	PFN_TX_PWR_TAG_PARA_FUNC pfnParseTagParaHandler;
} g_auTxPwrTagTable[] = {
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	{
		"MIMO_1T",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_MIMO_1T,
		PWR_CFG_BACKOFF_INIT,
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		NULL
	},
	{
		"MIMO_2T",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_MIMO_2T,
		PWR_CFG_BACKOFF_INIT,
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		NULL
	},
	{
		"ALL_T",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_ALL_T,
		PWR_CFG_BACKOFF_INIT,
		(POWER_ANT_BAND_NUM * POWER_ANT_NUM),
		NULL
	},
	{
		"ALL_T_6G",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_ALL_T_6G,
		PWR_CFG_BACKOFF_INIT,
		(POWER_ANT_6G_BAND_NUM * POWER_ANT_NUM),
		NULL
	},
	{
		"CHAIN_COMP",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_CHAIN_COMP,
		PWR_CFG_BACKOFF_INIT,
		0, /* Dynamic decision*/
		txPwrParseTagChainComp
	},
	{
		"CHAIN_ABS",
		POWER_TAG_CATEGORY_ANT,
		POWER_ANT_CHAIN_ABS,
		PWR_CFG_ABS_INIT,
		0, /* Dynamic decision*/
		txPwrParseTagChainAbs
	},
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	{
		"MULTIBAND",
		POWER_TAG_CATEGORY_MULTIBAND,
		0x0,
		PWR_CFG_ABS_INIT,
		0, /* Dynamic decision*/
		txPwrParseTagMultiBand
	}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
};


#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT */

#define PWR_BUF_LEN 1024

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
#define COUTRY_LMT_CH_MATCH_KEY_LEGACY(_i, _eBand, _i2CurrCh, _ucTarCh) \
		(((_i2CurrCh) == g_rCountryLmtChGrpTbl[(_i)].i2Key) && \
		 ((_ucTarCh) >= g_rCountryLmtChGrpTbl[(_i)].ucStartCh && \
		  (_ucTarCh) <= g_rCountryLmtChGrpTbl[(_i)].ucEndCh))

#define COUTRY_LMT_CH_MATCH_KEY_6G(_i, _eBand, _i2CurrCh, _ucTarCh) \
		(((_eBand) == g_rCountryLmtChGrpTbl[(_i)].eBand) && \
		((_i2CurrCh) == g_rCountryLmtChGrpTbl[(_i)].i2Key) && \
		((_ucTarCh) >= g_rCountryLmtChGrpTbl[(_i)].ucStartCh && \
		 (_ucTarCh) <= g_rCountryLmtChGrpTbl[(_i)].ucEndCh))

#define IS_FREQ_RANGE_OVERLAP_6G_UNII_BAND(_unii_idx, _start_freq, _end_freq) \
		((_start_freq) < _ar6GUniiFreqRange[_unii_idx][1] && \
		(_end_freq) > _ar6GUniiFreqRange[_unii_idx][0])

#if CFG_EXT_FEATURE
#define COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED \
				g_rCountryPwrMode6GSupport_Ext
#define COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED_SIZE \
				COUNTRY_PWR_MODE_6G_SUPPORT_TABLE_SIZE_EXT
#define COUNTRY_PWR_LMT_GROUPS_TBL_USED \
				arSupportCountryPowerLmtGrps_Ext
#define COUNTRY_PWR_LMT_GROUPS_TBL_USED_SIZE \
				COUNTRY_LIMIT_GROUP_EXT_NUM
#else
#define COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED \
				g_rCountryPwrMode6GSupport
#define COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED_SIZE \
				COUNTRY_PWR_MODE_6G_SUPPORT_TABLE_SIZE
#define COUNTRY_PWR_LMT_GROUPS_TBL_USED \
				arSupportCountryPowerLmtGrps
#define COUNTRY_PWR_LMT_GROUPS_TBL_USED_SIZE \
				COUNTRY_LIMIT_GROUP_NUM
#endif /* CFG_EXT_FEATURE */
/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT
/* dynamic tx power control */
char *g_au1TxPwrDefaultSetting[] = {
	"_SAR_PwrLevel;1;2;1;[2G4,,,,,,,,,,,,][5G,,,,,,,,,,,,]",
	"_G_Scenario;1;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;2;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;3;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;4;2;1;[ALL,,,,,,,,,,,,]",
	"_G_Scenario;5;2;1;[ALL,,,,,,,,,,,,]",
	"_Cus_TxPwr_Call;1;2;1;[ALL,,,,,,,,,,,,]",
};
#endif

#if (CFG_SUPPORT_RLM_DOMAIN_LOAD_FILE == 1)
#define WLAN_CFG_FILE_BUF_SIZE 2048
#define REG_DOMAIN_CFG_NUM 9
const char *g_aucDomainCfgFileName[REG_DOMAIN_CFG_NUM] = {
	"REG1_ChPlan.cfg",
	"REG2_ChPlan.cfg",
	"REG3_ChPlan.cfg",
	"REG4_ChPlan.cfg",
	"REG5_ChPlan.cfg",
	"REG6_ChPlan.cfg",
	"REG7_ChPlan.cfg",
	"REG8_ChPlan.cfg",
	"REG9_ChPlan.cfg"
};
static uint16_t g_u2CfgCountryCode = {0};
struct DOMAIN_INFO_ENTRY g_arCurRegDomain = {0};
#endif

struct COUNTRY_POWER_LIMIT_FILE_INFO
	g_arPwrLimitDefFile[PWR_LIMIT_DEFAULT_BASE_NUM] = {
		{"PwrDef.cfg", DEFAULT_TBL_SIZE, 0, 0, NULL},
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		{"PwrDef_VLP.cfg", DEFAULT_TBL_SIZE, 0, 0, NULL},
		{"PwrDef_SP.cfg", DEFAULT_TBL_SIZE, 0, 0, NULL}
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1*/
};

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
struct COUNTRY_POWER_LIMIT_CFG_FILE_INFO
	g_arPwrLimitCfgFile[PWR_LIMIT_CONFIG_BASE_NUM] = {
		{
			{"PwrCfgLegacy.cfg", CFG_TBL_SIZE_LEGACY, 0, 0, NULL},
			{"PwrCfgHE.cfg", CFG_TBL_SIZE_HE, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			{"PwrCfgEHT.cfg", CFG_TBL_SIZE_EHT, 0, 0, NULL},
#endif
		},
#if (CFG_SUPPORT_WIFI_6G == 1)
		{
			{"PwrCfgLegacy6G_LPI.cfg",
				CFG_TBL_SIZE_LEGACY, 0, 0, NULL},
			{"PwrCfgHE6G_LPI.cfg", CFG_TBL_SIZE_HE, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			{"PwrCfgEHT6G_LPI.cfg", CFG_TBL_SIZE_EHT, 0, 0, NULL},
#endif
		},
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		{
			{"PwrCfgLegacy6G_VLP.cfg",
				CFG_TBL_SIZE_LEGACY, 0, 0, NULL},
			{"PwrCfgHE6G_VLP.cfg", CFG_TBL_SIZE_HE, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			{"PwrCfgEHT6G_VLP.cfg", CFG_TBL_SIZE_EHT, 0, 0, NULL},
#endif
		},
		{
			{"PwrCfgLegacy6G_SP.cfg",
				CFG_TBL_SIZE_LEGACY, 0, 0, NULL},
			{"PwrCfgHE6G_SP.cfg", CFG_TBL_SIZE_HE, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
			{"PwrCfgEHT6G_SP.cfg", CFG_TBL_SIZE_EHT, 0, 0, NULL},
#endif
		},
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1 */
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
};
#endif /*#if ((CFG_SUPPORT_PWR_LMT_EMI == 1)*/

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
struct COUNTRY_POWER_LIMIT_FILE_INFO
	g_arPwrLimitCfgV0File[CFG_V0_NUM] = {
		{"PwrCfgLegacy.cfg", CFG_TBL_V0_SIZE_LEGACY, 0, 0, NULL},
		{"PwrCfgHE.cfg", CFG_TBL_V0_SIZE_HE, 0, 0, NULL},
		{"PwrCfgHE160.cfg", CFG_TBL_V0_SIZE_HE160, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		{"PwrCfgEHT.cfg", CFG_TBL_V0_SIZE_EHT, 0, 0, NULL},
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
		{"PwrCfgLegacy6G_LPI.cfg",
			CFG_TBL_V0_SIZE_LEGACY6G, 0, 0, NULL},
		{"PwrCfgHE6G_LPI.cfg", CFG_TBL_V0_SIZE_HE6G, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		{"PwrCfgEHT6G_LPI.cfg", CFG_TBL_V0_SIZE_EHT6G, 0, 0, NULL},
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		{"PwrCfgLegacy6G_VLP.cfg",
			CFG_TBL_V0_SIZE_LEGACY6G, 0, 0, NULL},
		{"PwrCfgHE6G_VLP.cfg", CFG_TBL_V0_SIZE_HE6G, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		{"PwrCfgEHT6G_VLP.cfg", CFG_TBL_V0_SIZE_EHT6G, 0, 0, NULL},
#endif
		{"PwrCfgLegacy6G_SP.cfg",
				CFG_TBL_V0_SIZE_LEGACY6G, 0, 0, NULL},
		{"PwrCfgHE6G_SP.cfg", CFG_TBL_V0_SIZE_HE6G, 0, 0, NULL},
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		{"PwrCfgEHT6G_SP.cfg", CFG_TBL_V0_SIZE_EHT6G, 0, 0, NULL},
#endif
#endif
#endif
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
	COUNTRY_CODE_BF, COUNTRY_CODE_BI, COUNTRY_CODE_BJ, COUNTRY_CODE_BT,
	COUNTRY_CODE_BW, COUNTRY_CODE_CD, COUNTRY_CODE_CF, COUNTRY_CODE_CG,
	COUNTRY_CODE_CI, COUNTRY_CODE_CM, COUNTRY_CODE_CV, COUNTRY_CODE_DJ,
	COUNTRY_CODE_FO, COUNTRY_CODE_GA, COUNTRY_CODE_GE, COUNTRY_CODE_GF,
	COUNTRY_CODE_GG, COUNTRY_CODE_GL, COUNTRY_CODE_GM, COUNTRY_CODE_GN,
	COUNTRY_CODE_GP, COUNTRY_CODE_GQ, COUNTRY_CODE_GW, COUNTRY_CODE_IM,
	COUNTRY_CODE_IQ, COUNTRY_CODE_JE, COUNTRY_CODE_KE, COUNTRY_CODE_KM,
	COUNTRY_CODE_LB, COUNTRY_CODE_LI, COUNTRY_CODE_LS, COUNTRY_CODE_LY,
	COUNTRY_CODE_MC, COUNTRY_CODE_ME, COUNTRY_CODE_MK, COUNTRY_CODE_ML,
	COUNTRY_CODE_MQ, COUNTRY_CODE_MR, COUNTRY_CODE_MU, COUNTRY_CODE_MZ,
	COUNTRY_CODE_NE, COUNTRY_CODE_NR, COUNTRY_CODE_PF, COUNTRY_CODE_PM,
	COUNTRY_CODE_RE, COUNTRY_CODE_SM, COUNTRY_CODE_SO, COUNTRY_CODE_ST,
	COUNTRY_CODE_SZ, COUNTRY_CODE_TD, COUNTRY_CODE_TF, COUNTRY_CODE_TG,
	COUNTRY_CODE_TJ, COUNTRY_CODE_TM, COUNTRY_CODE_TV, COUNTRY_CODE_TZ,
	COUNTRY_CODE_VA, COUNTRY_CODE_YT, COUNTRY_CODE_ZM
};
static const uint16_t g_u2CountryGroup1[] = {
	COUNTRY_CODE_AG, COUNTRY_CODE_AI, COUNTRY_CODE_AM, COUNTRY_CODE_AN,
	COUNTRY_CODE_AQ, COUNTRY_CODE_AW, COUNTRY_CODE_AX, COUNTRY_CODE_BB,
	COUNTRY_CODE_BM, COUNTRY_CODE_BN, COUNTRY_CODE_BS, COUNTRY_CODE_BV,
	COUNTRY_CODE_BZ, COUNTRY_CODE_CO, COUNTRY_CODE_DO, COUNTRY_CODE_EC,
	COUNTRY_CODE_FJ, COUNTRY_CODE_FK, COUNTRY_CODE_FM, COUNTRY_CODE_GD,
	COUNTRY_CODE_GI, COUNTRY_CODE_GS, COUNTRY_CODE_GY, COUNTRY_CODE_HN,
	COUNTRY_CODE_HT, COUNTRY_CODE_IN, COUNTRY_CODE_IO, COUNTRY_CODE_IR,
	COUNTRY_CODE_KG, COUNTRY_CODE_KH, COUNTRY_CODE_KN, COUNTRY_CODE_KP,
	COUNTRY_CODE_KY, COUNTRY_CODE_LA, COUNTRY_CODE_LC,
	COUNTRY_CODE_LK, COUNTRY_CODE_LR, COUNTRY_CODE_MH, COUNTRY_CODE_MN,
	COUNTRY_CODE_MS, COUNTRY_CODE_MW, COUNTRY_CODE_NA, COUNTRY_CODE_NI,
	COUNTRY_CODE_NU, COUNTRY_CODE_PA, COUNTRY_CODE_PG, COUNTRY_CODE_PN,
	COUNTRY_CODE_PS, COUNTRY_CODE_PW, COUNTRY_CODE_PY, COUNTRY_CODE_QA,
	COUNTRY_CODE_RW, COUNTRY_CODE_SB, COUNTRY_CODE_SC, COUNTRY_CODE_SD,
	COUNTRY_CODE_SH, COUNTRY_CODE_SJ, COUNTRY_CODE_SN, COUNTRY_CODE_SS,
	COUNTRY_CODE_SV, COUNTRY_CODE_SX, COUNTRY_CODE_SY, COUNTRY_CODE_TC,
	COUNTRY_CODE_TK, COUNTRY_CODE_TO, COUNTRY_CODE_TT, COUNTRY_CODE_UA,
	COUNTRY_CODE_VC, COUNTRY_CODE_VG, COUNTRY_CODE_VN, COUNTRY_CODE_VU,
	COUNTRY_CODE_WS, COUNTRY_CODE_YE
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
	COUNTRY_CODE_CC, COUNTRY_CODE_CX, COUNTRY_CODE_HM,
	COUNTRY_CODE_MX, COUNTRY_CODE_NF
};
static const uint16_t g_u2CountryGroup5[] = {
	COUNTRY_CODE_BH, COUNTRY_CODE_CN, COUNTRY_CODE_MV, COUNTRY_CODE_UY,
	COUNTRY_CODE_VE
};
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
	COUNTRY_CODE_EH, COUNTRY_CODE_UZ
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
	COUNTRY_CODE_MA
};
static const uint16_t g_u2CountryGroup16[] = {
	COUNTRY_CODE_GH, COUNTRY_CODE_UG
};
static const uint16_t g_u2CountryGroup17[] = {
	COUNTRY_CODE_TN
};
static const uint16_t g_u2CountryGroup18[] = {
	COUNTRY_CODE_AL, COUNTRY_CODE_AT, COUNTRY_CODE_BA, COUNTRY_CODE_BE,
	COUNTRY_CODE_BG, COUNTRY_CODE_CH, COUNTRY_CODE_CY, COUNTRY_CODE_CZ,
	COUNTRY_CODE_DE, COUNTRY_CODE_DK, COUNTRY_CODE_EE,
	COUNTRY_CODE_FI, COUNTRY_CODE_FR, COUNTRY_CODE_GR, COUNTRY_CODE_HR,
	COUNTRY_CODE_HU, COUNTRY_CODE_IE, COUNTRY_CODE_IS, COUNTRY_CODE_IT,
	COUNTRY_CODE_KW, COUNTRY_CODE_LT, COUNTRY_CODE_LU, COUNTRY_CODE_LV,
	COUNTRY_CODE_MD, COUNTRY_CODE_MT, COUNTRY_CODE_NC, COUNTRY_CODE_NL,
	COUNTRY_CODE_NO, COUNTRY_CODE_PL, COUNTRY_CODE_PT, COUNTRY_CODE_RO,
	COUNTRY_CODE_RS, COUNTRY_CODE_SE, COUNTRY_CODE_SI, COUNTRY_CODE_SK,
	COUNTRY_CODE_TR, COUNTRY_CODE_WF, COUNTRY_CODE_XK
};
static const uint16_t g_u2CountryGroup19[] = {
	COUNTRY_CODE_HK, COUNTRY_CODE_PH, COUNTRY_CODE_SG, COUNTRY_CODE_TH,
	COUNTRY_CODE_IL, COUNTRY_CODE_ZA
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
	COUNTRY_CODE_AE, COUNTRY_CODE_CK, COUNTRY_CODE_CL, COUNTRY_CODE_GB,
	COUNTRY_CODE_KI, COUNTRY_CODE_MO, COUNTRY_CODE_NZ, COUNTRY_CODE_TL
};
static const uint16_t g_u2CountryGroup28[] = {
	COUNTRY_CODE_AR,
};
static const uint16_t g_u2CountryGroup29[] = {
	COUNTRY_CODE_RU,
};
static const uint16_t g_u2CountryGroup30[] = {
	COUNTRY_CODE_PK
};
/*O6 code for HQFEAT-133094 by p-zhuhao21 at 20250702 start*/
static const uint16_t g_u2CountryGroup31[] = {
	COUNTRY_CODE_ID
};
static const uint16_t g_u2CountryGroup32[] = {
	COUNTRY_CODE_ES
};
static const uint16_t g_u2CountryGroup33[] = {
        COUNTRY_CODE_KZ
};
static const uint16_t g_u2CountryGroup34[] = {
        COUNTRY_CODE_EG
};
static const uint16_t g_u2CountryGroup35[] = {
        COUNTRY_CODE_BO
};
static const uint16_t g_u2CountryGroup36[] = {
        COUNTRY_CODE_TW
};

/*O6 code for HQFEAT-133094 by p-zhuhao21 at 20250702 end*/
#if (CFG_SUPPORT_SINGLE_SKU == 1)
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
	{35,
	 {"cck", "ofdm", "ofdm40", "ofdm80", "ofdm160", "ofdm320",
	  "ht20", "ht40", "vht20", "vht40",
	  "vht80", "vht160", "ru26", "ru52", "ru106", "ru242",
	  "ru484", "ru996", "ru996x2",
	  "eht26", "eht52", "eht106", "eht242", "eht484", "eht996",
	  "eht996x2", "eht996x4", "eht26_52", "eht26_106", "eht484_242",
	  "eht996_484", "eht996_484_242", "eht996x2_484", "eht996x3",
	  "eht996x3_484"}
	}
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

	{POWER_LIMIT_SKU_CCK_NUM,
	 POWER_LIMIT_SKU_OFDM_NUM,
	 POWER_LIMIT_SKU_OFDM_NUM,  /* OFDM40 */
	 POWER_LIMIT_SKU_OFDM_NUM,  /* OFDM80 */
	 POWER_LIMIT_SKU_OFDM_NUM,  /* OFDM160*/
	 POWER_LIMIT_SKU_OFDM_NUM,  /* OFDM320 */

	 POWER_LIMIT_SKU_HT20_NUM, POWER_LIMIT_SKU_HT40_NUM,
	 POWER_LIMIT_SKU_VHT20_2_NUM, POWER_LIMIT_SKU_VHT40_2_NUM,
	 POWER_LIMIT_SKU_VHT80_2_NUM, POWER_LIMIT_SKU_VHT160_2_NUM,
	 POWER_LIMIT_SKU_RU26_NUM, POWER_LIMIT_SKU_RU52_NUM,
	 POWER_LIMIT_SKU_RU106_NUM, POWER_LIMIT_SKU_RU242_NUM,
	 POWER_LIMIT_SKU_RU484_NUM, POWER_LIMIT_SKU_RU996_NUM,
	 POWER_LIMIT_SKU_RU996X2_NUM,
	 POWER_LIMIT_SKU_EHT26_NUM, POWER_LIMIT_SKU_EHT52_NUM,
	 POWER_LIMIT_SKU_EHT106_NUM, POWER_LIMIT_SKU_EHT242_NUM,
	 POWER_LIMIT_SKU_EHT484_NUM, POWER_LIMIT_SKU_EHT996_NUM,
	 POWER_LIMIT_SKU_EHT996X2_NUM, POWER_LIMIT_SKU_EHT996X4_NUM,
	 POWER_LIMIT_SKU_EHT26_52_NUM,
	 POWER_LIMIT_SKU_EHT26_106_NUM, POWER_LIMIT_SKU_EHT484_242_NUM,
	 POWER_LIMIT_SKU_EHT996_484_NUM, POWER_LIMIT_SKU_EHT996_484_242_NUM,
	 POWER_LIMIT_SKU_EHT996X2_484_NUM, POWER_LIMIT_SKU_EHT996X3_NUM,
	 POWER_LIMIT_SKU_EHT996X3_484_NUM
	}
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
	{
		{"c1", "c2", "c5", "c11"},
		{"o6", "o9", "o12", "o18", "o24", "o36", "o48", "o54"},
		{"o6", "o9", "o12", "o18", "o24", "o36", "o48", "o54"},
		{"o6", "o9", "o12", "o18", "o24", "o36", "o48", "o54"},
		{"o6", "o9", "o12", "o18", "o24", "o36", "o48", "o54"},
		{"o6", "o9", "o12", "o18", "o24", "o36", "o48", "o54"},
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
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},
		{"m0", "m1", "m2", "m3", "m4", "m5", "m6", "m7", "m8", "m9",
		 "m10", "m11", "m12", "m13", "m14", "m15"},

	},
};

static const int8_t gTx_Pwr_Limit_2g_Ch[] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
static const int8_t gTx_Pwr_Limit_5g_Ch[] = {
	36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 100, 102,
	104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 132,
	134, 136, 138, 140, 142, 144, 149, 151, 153, 155, 157, 159, 161,
#if (CFG_SUPPORT_UNII4 == 1)
	163,
#endif
	165
#if (CFG_SUPPORT_UNII4 == 1)
	, 167, 169, 171, 173, 175, 177
#endif
	};
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
			{0, BAND_NULL, 0, 0, 0, FALSE}
				/* CH_SET_UNII_UPPER_NA */
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
				/*CH_SET_UNII_MID_132_144 */
#if (CFG_SUPPORT_UNII4 == 0)
			,
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
				/*CH_SET_UNII_UPPER_149_165 */
#else
			,
			{125, BAND_5G, CHNL_SPAN_20, 149, 8, FALSE}
				/*CH_SET_UNII_UPPER_149_177 */
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
			,	/* 6G_CH_1_233 */
			/* This will exceed array size, mark as unsupported
			 * {136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
			 */
#endif

		},
		REG_CAT_FCC
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
	}
	,
	{
		(uint16_t *) g_u2CountryGroup11, sizeof(g_u2CountryGroup11) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
	}
	,
	{
		(uint16_t *) g_u2CountryGroup13, sizeof(g_u2CountryGroup13) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
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
		},
		REG_CAT_ARIB
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
				/*CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
			,	/* 6G_CH_1_93 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2 */
#endif

		},
		REG_CAT_ETSI
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
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_WW_NA */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
			{0, BAND_NULL, 0, 0, 0, FALSE}
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
			{0, BAND_NULL, 0, 0, 0, FALSE}
			,	/*CH_SET_UNII_UPPER_NA */
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
		},
		REG_CAT_ETSI
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
			,	/* 6G_CH_1_233 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2*/
#endif
		},
		REG_CAT_FCC
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
		},
		REG_CAT_ETSI
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
			,	/* 6G_CH_1_233 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2*/
#endif
		},
		REG_CAT_FCC
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
		},
		REG_CAT_ETSI
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
			,	/* 6G_CH_1_93 */
			/* This will exceed array size, mark as unsupported
			 * {136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
			 */
#endif
		},
		REG_CAT_ETSI
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
			,	/* 6G_CH_1_233 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2 */
#endif
		},
		REG_CAT_FCC
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
			,	/* 6G_CH_1_233 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2 */
#endif
		},
		REG_CAT_FCC
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
			,	/* 6G_CH_1_93 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2 */
#endif
		},
		REG_CAT_FCC
	}
	,
	{
		(uint16_t *) g_u2CountryGroup28, sizeof(g_u2CountryGroup28) / 2,
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
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
			,	/* 6G_CH_1_233 */
			/* This will exceed array size, mark as unsupported
			 * {136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
			 */
			/* 6G_CH_2 */
#endif
		},
		REG_CAT_FCC
	}
	,
	{
		(uint16_t *) g_u2CountryGroup29, sizeof(g_u2CountryGroup29) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/*CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,	/*CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/*CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 132, 4, TRUE}
			,	/*CH_SET_UNII_MID_132_144 */
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
		(uint16_t *) g_u2CountryGroup30, sizeof(g_u2CountryGroup30) / 2,
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
				/*CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			,
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
				/* 6G_CH_1_93 */
#endif
		}
	}
	,
/*O6 code for HQFEAT-133094 by p-zhuhao21 at 20250702 start*/
        {
                (uint16_t *) g_u2CountryGroup31, sizeof(g_u2CountryGroup31) / 2,
                {
                        {81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
                        ,        /*CH_SET_2G4_1_13 */
                        {115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
                        ,        /*CH_SET_UNII_LOW_36_48 */
                        {118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
                        ,        /*CH_SET_UNII_MID_52_64 */
                        {121, BAND_NULL, 0, 0, 0, FALSE}
                        ,        /*CH_SET_UNII_WW_NA */
                        {125, BAND_5G, CHNL_SPAN_20, 149, 4, FALSE}
                        ,        /* CH_SET_UNII_UPPER_149_161 */
                        {0, BAND_NULL, 0, 0, 0, FALSE}
                },
                REG_CAT_ETSI
        }
        ,
	{
		(uint16_t *) g_u2CountryGroup32, sizeof(g_u2CountryGroup32) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,			/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
			,			/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 11, TRUE}
			,			/* CH_SET_UNII_WW_100_140 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,			/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
			,			/* 6G_CH_1_233 */
#endif
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup33, sizeof(g_u2CountryGroup33) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,	/* CH_SET_2G4_1_13 */
			{115, BAND_5G, CHNL_SPAN_20, 36, 4, TRUE}
			,	/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,	/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,	/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, TRUE}
			,	/* CH_SET_UNII_UPPER_149_165 */
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup34, sizeof(g_u2CountryGroup34) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,			/* CH_SET_2G4_1_13 */

			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,			/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_NULL, 0, 0, 0, FALSE}
			,			/* CH_SET_UNII_WW_NA */
			{125, BAND_NULL, 0, 0, 0, FALSE}
			,			/* CH_SET_UNII_UPPER_NA */
#if (CFG_SUPPORT_WIFI_6G == 1)
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
			,			/* 6G_CH_1_233 */
#endif
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup35, sizeof(g_u2CountryGroup35) / 2,
		{
			{81, BAND_2G4, CHNL_SPAN_5, 1, 13, FALSE}
			,			/* CH_SET_2G4_1_13 */

			{115, BAND_5G, CHNL_SPAN_20, 36, 4, FALSE}
			,			/* CH_SET_UNII_LOW_36_48 */
			{118, BAND_5G, CHNL_SPAN_20, 52, 4, TRUE}
			,			/* CH_SET_UNII_MID_52_64 */
			{121, BAND_5G, CHNL_SPAN_20, 100, 12, TRUE}
			,			/* CH_SET_UNII_WW_100_144 */
			{125, BAND_5G, CHNL_SPAN_20, 149, 5, FALSE}
			,			/* CH_SET_UNII_UPPER_149_165 */
#if (CFG_SUPPORT_WIFI_6G == 1)
			{131, BAND_6G, CHNL_SPAN_20, 1, 59, FALSE}
			,			/* 6G_CH_1_233 */
#endif
			{0, BAND_NULL, 0, 0, 0, FALSE}
		}
	}
	,
	{
		(uint16_t *) g_u2CountryGroup36, sizeof(g_u2CountryGroup36) / 2,
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
#if (CFG_SUPPORT_WIFI_6G == 1)
			{131, BAND_6G, CHNL_SPAN_20, 1, 24, FALSE}
			,	/* 6G_CH_1_93 */
#endif
			{0, BAND_NULL, 0, 0, 0, FALSE}
		},
		REG_CAT_ETSI
	}
	,
/*O6 code for HQFEAT-133094 by p-zhuhao21 at 20250702 end*/
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
			,	/* 6G_CH_1_233 */
			{136, BAND_6G, CHNL_SPAN_20, 2, 1, FALSE}
				/* 6G_CH_2 */
#endif
		},
		REG_CAT_NULL
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

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
#define TX_PWR_ANT_GAIN_ELEMENT_NUM	(2)
#define TX_PWR_ANT_GAIN_GROUP_NUM	(6)

struct TX_PWR_ANT_GAIN_GROUP {
	// Group number
	uint8_t ucGroup;
	// Antenna gain for each group
	int8_t rTxPwrAntGainValue[BAND_NUM - 1][TX_PWR_ANT_GAIN_ELEMENT_NUM];
};

struct TX_PWR_ANT_GAIN_DATA {
	// Total group number
	uint8_t ucGroupNum;
	// Total antenna gain for all group
	struct TX_PWR_ANT_GAIN_GROUP *rAntGainGroup;
};

struct TX_PWR_ANT_GAIN_GROUP
	g_TxPwrAntGainGroupData[TX_PWR_ANT_GAIN_GROUP_NUM] = {0};
struct TX_PWR_ANT_GAIN_DATA g_TxPwrAntGainData;

const struct TX_PWR_ANT_GAIN_SECTION {
	uint8_t ucSectionNum;
	const char *arSectionNames[3];
} gTx_Pwr_Ant_Gain_Section = {
	3,
	{"ant_gain_2g", "ant_gain_5g", "ant_gain_6g"}
};
#endif  /*CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1*/

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
struct COUNTRY_LIMIT_CHNL_GRP_TABLE g_rCountryLmtChGrpTbl[] = {
	{BAND_2G4, COUNTRY_LMT_CH_GRP_KEY_ALL_CHNL, 1, 14},     /* key =  0*/
	{BAND_2G4, COUNTRY_LMT_CH_GRP_KEY_2G_ALL, 1, 14},       /* key = -1 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_ALL_CHNL, 36, 181},    /* key = -2 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_5G_ALL, 36, 181},      /* key = -3 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_5G_UNII_1, 36, 50},    /* key = -4 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_5G_UNII_2, 52, 64},    /* key = -5 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_5G_UNII_3, 100, 144},  /* key = -6 */
	{BAND_5G, COUNTRY_LMT_CH_GRP_KEY_5G_UNII_4, 149, 181},  /* key = -7*/
#if (CFG_SUPPORT_WIFI_6G == 1)
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_ALL_CHNL, 1, 233},     /* key = -8 */
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_6G_ALL, 1, 233},       /* key = -9 */
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_6G_UNII_5, 1, 93},     /* key = -10 */
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_6G_UNII_6, 95, 113},   /* key = -11 */
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_6G_UNII_7, 115, 181},  /* key = -12 */
	{BAND_6G, COUNTRY_LMT_CH_GRP_KEY_6G_UNII_8, 183, 233}   /* key = -13 */
#endif
};
#endif

/* group for countries that marked channel 36~64 as indoor channel */
static const uint16_t g_u2IndoorType0[] = {
	COUNTRY_CODE_GG, COUNTRY_CODE_GP, COUNTRY_CODE_GR, COUNTRY_CODE_GL,
	COUNTRY_CODE_ZA, COUNTRY_CODE_NL, COUNTRY_CODE_NO, COUNTRY_CODE_NF,
	COUNTRY_CODE_DK, COUNTRY_CODE_DE, COUNTRY_CODE_LV, COUNTRY_CODE_RU,
	COUNTRY_CODE_LB, COUNTRY_CODE_RE, COUNTRY_CODE_RO, COUNTRY_CODE_LU,
	COUNTRY_CODE_RW, COUNTRY_CODE_LT, COUNTRY_CODE_LI, COUNTRY_CODE_MQ,
	COUNTRY_CODE_YT, COUNTRY_CODE_MK, COUNTRY_CODE_IM, COUNTRY_CODE_MC,
	COUNTRY_CODE_MA, COUNTRY_CODE_MU, COUNTRY_CODE_MR, COUNTRY_CODE_ME,
	COUNTRY_CODE_MD, COUNTRY_CODE_MV, COUNTRY_CODE_MT, COUNTRY_CODE_MN,
	COUNTRY_CODE_BH, COUNTRY_CODE_VA, COUNTRY_CODE_BE, COUNTRY_CODE_BY,
	COUNTRY_CODE_BA, COUNTRY_CODE_BG, COUNTRY_CODE_BR, COUNTRY_CODE_SA,
	COUNTRY_CODE_SM, COUNTRY_CODE_ST, COUNTRY_CODE_PM, COUNTRY_CODE_RS,
	COUNTRY_CODE_SZ, COUNTRY_CODE_SE, COUNTRY_CODE_CH, COUNTRY_CODE_ES,
	COUNTRY_CODE_SK, COUNTRY_CODE_SI, COUNTRY_CODE_AE, COUNTRY_CODE_IS,
	COUNTRY_CODE_IE, COUNTRY_CODE_AZ, COUNTRY_CODE_AD, COUNTRY_CODE_AL,
	COUNTRY_CODE_EE, COUNTRY_CODE_EC, COUNTRY_CODE_GB, COUNTRY_CODE_IO,
	COUNTRY_CODE_OM, COUNTRY_CODE_AU, COUNTRY_CODE_AT, COUNTRY_CODE_WF,
	COUNTRY_CODE_IL, COUNTRY_CODE_EG, COUNTRY_CODE_IT, COUNTRY_CODE_ID,
	COUNTRY_CODE_JP, COUNTRY_CODE_JE, COUNTRY_CODE_GE, COUNTRY_CODE_CN,
	COUNTRY_CODE_GI, COUNTRY_CODE_CZ, COUNTRY_CODE_KZ, COUNTRY_CODE_QA,
	COUNTRY_CODE_KE, COUNTRY_CODE_CC, COUNTRY_CODE_KW, COUNTRY_CODE_HR,
	COUNTRY_CODE_CX, COUNTRY_CODE_CY, COUNTRY_CODE_TH, COUNTRY_CODE_TJ,
	COUNTRY_CODE_TR, COUNTRY_CODE_TM, COUNTRY_CODE_PS, COUNTRY_CODE_FO,
	COUNTRY_CODE_PT, COUNTRY_CODE_PL, COUNTRY_CODE_FR, COUNTRY_CODE_GF,
	COUNTRY_CODE_TF, COUNTRY_CODE_PF, COUNTRY_CODE_FI, COUNTRY_CODE_PN,
	COUNTRY_CODE_HM, COUNTRY_CODE_HU, COUNTRY_CODE_HK, COUNTRY_CODE_XK,
};

/* group for countries that marked channel 36~48 as indoor channel */
static const uint16_t g_u2IndoorType1[] = {
	COUNTRY_CODE_NP, COUNTRY_CODE_VE, COUNTRY_CODE_VN, COUNTRY_CODE_AR,
	COUNTRY_CODE_DZ, COUNTRY_CODE_SV, COUNTRY_CODE_UY, COUNTRY_CODE_CL,
	COUNTRY_CODE_CA, COUNTRY_CODE_PE
};

/* group for countries that marked channel 100~128 as indoor channel */
static const uint16_t g_u2IndoorType2[] = {
	COUNTRY_CODE_KZ, COUNTRY_CODE_QA, COUNTRY_CODE_MN, COUNTRY_CODE_BY,
};

/* group for countries that marked channel 100~140 as indoor channel */
static const uint16_t g_u2IndoorType3[] = {
	COUNTRY_CODE_KZ, COUNTRY_CODE_QA, COUNTRY_CODE_MN, COUNTRY_CODE_BY,
	COUNTRY_CODE_RU,
};

/* group for countries that marked channel 100~144 as indoor channel */
static const uint16_t g_u2IndoorType4[] = {
	COUNTRY_CODE_KZ, COUNTRY_CODE_QA, COUNTRY_CODE_MN, COUNTRY_CODE_RU,
};

/* group for countries that marked channel 149~165 as indoor channel */
static const uint16_t g_u2IndoorType5[] = {
	COUNTRY_CODE_KZ, COUNTRY_CODE_QA, COUNTRY_CODE_RU,
};

/* group for countries that marked channel 169~177 as indoor channel */
static const uint16_t g_u2IndoorType6[] = {
	COUNTRY_CODE_GU, COUNTRY_CODE_US, COUNTRY_CODE_UM, COUNTRY_CODE_VI,
	COUNTRY_CODE_MP, COUNTRY_CODE_AS, COUNTRY_CODE_PR,
};


struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT_INFO
	g_rlmPowerLimitDefaultTable[PWR_LIMIT_DEFAULT_BASE_NUM];

struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT_INFO
	g_rlmPowerLimitDefaultTable_bk[PWR_LIMIT_DEFAULT_BASE_NUM] = {
		PWR_LMT_TBL_REG(g_rRlmPowerLimitDefault),
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
		PWR_LMT_TBL_REG(g_rRlmPowerLimitDefault_VLP),
		PWR_LMT_TBL_REG(g_rRlmPowerLimitDefault_SP)
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1*/
};

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_INFO
	g_rlmPowerLimitConfigTable[PWR_LIMIT_CONFIG_BASE_NUM];

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_INFO
	g_rlmPowerLimitConfigTable_bk[PWR_LIMIT_CONFIG_BASE_NUM] = {
	{
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationLegacy),
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationHE),
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationEHT),
#endif
	},
#if (CFG_SUPPORT_WIFI_6G == 1)
	{
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationLegacy_6G),
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationHE_6G),
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationEHT_6G),
#endif
	},
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	{
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationLegacy_6G_VLP),
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationHE_6G_VLP),
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationEHT_6G_VLP),
#endif
	},
	{
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationLegacy_6G_SP),
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationHE_6G_SP),
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LMT_TBL_REG(g_rRlmPowerLimitConfigurationEHT_6G_SP),
#endif
	},
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE == 1 */
#endif /* CFG_SUPPORT_WIFI_6G == 1 */
};

struct PWR_LIMIT_INFO
	g_RlmPwrLimitInfo[PWR_LIMIT_RF_BAND_NUM][PWR_LIMIT_PROTOCOL_NUM] = {0};
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
struct MULTIBAND_PWR_LIMIT_INFO
	g_RlmMulitPwrLimitInfo[PWR_LIMIT_RF_BAND_NUM] = {0};
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
struct TX_PWR_BAND_MASK_TBL g_rBandMaskTbl[] = {
	{PWR_LIMIT_MULTIBAND_TYPE_0_1, BIT(0) + BIT(1)},
	{PWR_LIMIT_MULTIBAND_TYPE_0_2, BIT(0) + BIT(2)},
	{PWR_LIMIT_MULTIBAND_TYPE_1_2, BIT(1) + BIT(2)},
	{PWR_LIMIT_MULTIBAND_TYPE_0_1_2, BIT(0) + BIT(1) + BIT(2)}
};
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#endif /*#if ((CFG_SUPPORT_PWR_LMT_EMI == 1)*/

static const uint32_t _ar6GUniiFreqRange[SUBBAND_6G_NUM][2] = {
	{BAND_6G_UPPER_FREQ_UNII_5, BAND_6G_LOWER_FREQ_UNII_5},
	{BAND_6G_UPPER_FREQ_UNII_6, BAND_6G_LOWER_FREQ_UNII_6},
	{BAND_6G_UPPER_FREQ_UNII_7, BAND_6G_LOWER_FREQ_UNII_7},
	{BAND_6G_UPPER_FREQ_UNII_8, BAND_6G_LOWER_FREQ_UNII_8},
};
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
struct PWR_MODE_6G_BSS_INFO g_r6GPwrModeBssInfo[MAX_BSSID_NUM];
#endif
/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
#if (CFG_TC10_FEATURE == 1)
static char *g_sarScenarioName[SAR_NUM] = {
	"SAR_HEAD",
	"SAR_BODY",
	"SAR_NR_MMWAVE",
	"SAR_NR_SUB6",
	"SAR_MHS",
	"SAR_NR_SUB6_BAND_INFO_2",
	"SAR_NR_SUB6_BAND_INFO_7",
	"SAR_NR_SUB6_BAND_INFO_25",
	"SAR_NR_SUB6_BAND_INFO_38",
	"SAR_NR_SUB6_BAND_INFO_40",
	"SAR_NR_SUB6_BAND_INFO_41",
	"SAR_NR_SUB6_BAND_INFO_48",
	"SAR_NR_SUB6_BAND_INFO_66",
	"SAR_NR_SUB6_BAND_INFO_77",
	"SAR_NR_SUB6_BAND_INFO_78"
};
static struct SAR_SUB_IDX_REMAP_TBL_INFO g_SarSubIndxReMapTbl[] = {
	{0, 0},
	{1, BIT(SAR_HEAD)},
	{2, BIT(SAR_BODY)},
	{3, (BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{4, BIT(SAR_NR_MMWAVE)},
	{5, (BIT(SAR_NR_MMWAVE) | BIT(SAR_HEAD))},
	{6, (BIT(SAR_NR_MMWAVE) | BIT(SAR_BODY))},
	{7, (BIT(SAR_NR_MMWAVE) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{8, BIT(SAR_NR_SUB6)},
	{9, (BIT(SAR_NR_SUB6) | BIT(SAR_HEAD))},
	{10, (BIT(SAR_NR_SUB6) | BIT(SAR_BODY))},
	{11, (BIT(SAR_NR_SUB6) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{12, BIT(SAR_MHS)},
	{13, (BIT(SAR_MHS) | BIT(SAR_HEAD))},
	{14, (BIT(SAR_MHS) | BIT(SAR_BODY))},
	{15, (BIT(SAR_MHS) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{16, BIT(SAR_NR_SUB6_BAND_INFO_2)},
	{17, (BIT(SAR_NR_SUB6_BAND_INFO_2) | BIT(SAR_HEAD))},
	{18, (BIT(SAR_NR_SUB6_BAND_INFO_2) | BIT(SAR_BODY))},
	{19, (BIT(SAR_NR_SUB6_BAND_INFO_2) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{20, BIT(SAR_NR_SUB6_BAND_INFO_7)},
	{21, (BIT(SAR_NR_SUB6_BAND_INFO_7) | BIT(SAR_HEAD))},
	{22, (BIT(SAR_NR_SUB6_BAND_INFO_7) | BIT(SAR_BODY))},
	{23, (BIT(SAR_NR_SUB6_BAND_INFO_7) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{24, BIT(SAR_NR_SUB6_BAND_INFO_25)},
	{25, (BIT(SAR_NR_SUB6_BAND_INFO_25) | BIT(SAR_HEAD))},
	{26, (BIT(SAR_NR_SUB6_BAND_INFO_25) | BIT(SAR_BODY))},
	{27, (BIT(SAR_NR_SUB6_BAND_INFO_25) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{28, BIT(SAR_NR_SUB6_BAND_INFO_38)},
	{29, (BIT(SAR_NR_SUB6_BAND_INFO_38) | BIT(SAR_HEAD))},
	{30, (BIT(SAR_NR_SUB6_BAND_INFO_38) | BIT(SAR_BODY))},
	{31, (BIT(SAR_NR_SUB6_BAND_INFO_38) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{32, BIT(SAR_NR_SUB6_BAND_INFO_40)},
	{33, (BIT(SAR_NR_SUB6_BAND_INFO_40) | BIT(SAR_HEAD))},
	{34, (BIT(SAR_NR_SUB6_BAND_INFO_40) | BIT(SAR_BODY))},
	{35, (BIT(SAR_NR_SUB6_BAND_INFO_40) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{36, BIT(SAR_NR_SUB6_BAND_INFO_41)},
	{37, (BIT(SAR_NR_SUB6_BAND_INFO_41) | BIT(SAR_HEAD))},
	{38, (BIT(SAR_NR_SUB6_BAND_INFO_41) | BIT(SAR_BODY))},
	{39, (BIT(SAR_NR_SUB6_BAND_INFO_41) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{40, BIT(SAR_NR_SUB6_BAND_INFO_48)},
	{41, (BIT(SAR_NR_SUB6_BAND_INFO_48) | BIT(SAR_HEAD))},
	{42, (BIT(SAR_NR_SUB6_BAND_INFO_48) | BIT(SAR_BODY))},
	{43, (BIT(SAR_NR_SUB6_BAND_INFO_48) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{44, BIT(SAR_NR_SUB6_BAND_INFO_66)},
	{45, (BIT(SAR_NR_SUB6_BAND_INFO_66) | BIT(SAR_HEAD))},
	{46, (BIT(SAR_NR_SUB6_BAND_INFO_66) | BIT(SAR_BODY))},
	{47, (BIT(SAR_NR_SUB6_BAND_INFO_66) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{48, BIT(SAR_NR_SUB6_BAND_INFO_77)},
	{49, (BIT(SAR_NR_SUB6_BAND_INFO_77) | BIT(SAR_HEAD))},
	{50, (BIT(SAR_NR_SUB6_BAND_INFO_77) | BIT(SAR_BODY))},
	{51, (BIT(SAR_NR_SUB6_BAND_INFO_77) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
	{52, BIT(SAR_NR_SUB6_BAND_INFO_78)},
	{53, (BIT(SAR_NR_SUB6_BAND_INFO_78) | BIT(SAR_HEAD))},
	{54, (BIT(SAR_NR_SUB6_BAND_INFO_78) | BIT(SAR_BODY))},
	{55, (BIT(SAR_NR_SUB6_BAND_INFO_78) | BIT(SAR_HEAD) | BIT(SAR_BODY))},
};
static uint8_t g_ucSarApplyAnt[SAR_NUM];
static uint8_t g_ucSarApplyActAnt[SAR_NUM];
#endif /* CFG_TC10_FEATURE */
/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

static enum ENUM_PWR_LIMIT_DEFINE rlmDomainPwrLmtGetChannelDefine(void);

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
static uint8_t rlmDomainPwrLmt6GPwrModeGet(struct ADAPTER *prAdapter);

static uint32_t rlmDomainGetSubBandIdx(
	enum ENUM_BAND eBand,
	uint8_t ucCenterCh,
	uint8_t *pu1SubBandIdx);

static uint8_t rlmDomain6GPwrModeSupportStatusGet(
	struct COUNTRY_PWR_MODE_6G_SUPPORT_TABLE *prSupportTbl,
	uint8_t u1Band,
	uint8_t fgIsCh2,
	enum ENUM_PWR_MODE_6G_TYPE eMode);

static uint8_t rlmDomainGet6GBssChnl(struct ADAPTER *prAdapter,
						uint8_t ucBssIdx);

static uint8_t rlmDomainPwrLmt6GPwrModeGetByChnl(
	struct ADAPTER *prAdapter,
	uint8_t ucCurrChnl);

static uint8_t rlmDomainIsBssUse6g(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pucBw);
#endif

static uint8_t rlmDomainGetSubBandPwrLimit(
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitSubBand,
	uint32_t sub_band_idx);

static void txPwrCtrlSetAllRatePwrLimit(
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChnlSet,
	uint8_t op,
	uint8_t value);

static void txPwrCtrlSetSingleRatePwrLimit(
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChnlSet,
	enum ENUM_PWR_CFG_RATE_TAG eRateTag,
	uint8_t ofset,
	uint8_t op,
	uint8_t value);

static uint32_t txPwrCtrlApplyAntPowerSettings(
	struct ADAPTER *prAdapter,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd);

static int32_t txPwrOnPreParseAppendAntTag(
	struct TX_PWR_CTRL_ELEMENT *pRecord);

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)

static enum ENUM_BAND rlmDomainConvertRFBandEnum(
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static enum ENUM_PWR_LIMIT_CONFIG_BASE rlmDomainPwrLmtGetConfigBase(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_RF_BAND eRfBandIndex);

static void rlmDomainDumpPwrLimitEmiPayload(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	enum ENUM_PWR_LIMIT_PROTOCOL eProt);

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
static uint32_t txPwrCtrlApplyDynMultiBandPwrSetting(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rMultiBandPwrLimitInfo);

static uint8_t rlmDomainBuildDefaultMultiBandPwrLimitPayload(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rMultiBandPwrLimitInfo);

static void rlmDomainApplyDynMultiBandSettings(
	struct TX_PWR_CTRL_ELEMENT *element,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo);

static void rlmDomainDumpMultiBandPwrLimitPayload(
	struct ADAPTER *prAdapter,
	char *message,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo);

static uint32_t rlmDomainWriteMultiTxPwrEmiData(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo,
	char *prTxpMulitBandEmiAdr);

static uint32_t rlmDomainDumpMultiPwrLimitEmiPayload(
	struct ADAPTER *prAdapter,
	uint8_t u1BandMask,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo,
	char *prTxpMulitBandEmiAdr);
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

static void rlmDomainBuildDefaultPwrLimitPayload_Legacy(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo);

static void rlmDomainBuildConfigPwrLimitPayload_Legacy(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainDumpPwrLimitPayload_Legacy(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainApplyDynSettings_Legacy(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainWriteTxPwrEmiData_Legacy(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size);

static void rlmDomainBuildDefaultPwrLimitPayload_HE(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo);

static void rlmDomainBuildConfigPwrLimitPayload_HE(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainDumpPwrLimitPayload_HE(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainApplyDynSettings_HE(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainWriteTxPwrEmiData_HE(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size
);

static void rlmDomainBuildDefaultPwrLimitPayload_EHT(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo);

static void rlmDomainBuildConfigPwrLimitPayload_EHT(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainDumpPwrLimitPayload_EHT(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainApplyDynSettings_EHT(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

static void rlmDomainWriteTxPwrEmiData_EHT(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size
);

static uint32_t txPwrCtrlApplyDynPwrSetting(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_TYPE eLimitType,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex);

#endif /*#if ((CFG_SUPPORT_PWR_LMT_EMI == 1)*/
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
struct PWR_LIMIT_HANDLER_INFO g_rRlmPwrLimitHandler[PWR_LIMIT_TYPE_NUM] = {
	{
		rlmDomainBuildDefaultPwrLimitPayload_Legacy,
		rlmDomainBuildConfigPwrLimitPayload_Legacy,
		rlmDomainDumpPwrLimitPayload_Legacy,
		rlmDomainApplyDynSettings_Legacy,
		rlmDomainWriteTxPwrEmiData_Legacy,
	},
	{
		rlmDomainBuildDefaultPwrLimitPayload_HE,
		rlmDomainBuildConfigPwrLimitPayload_HE,
		rlmDomainDumpPwrLimitPayload_HE,
		rlmDomainApplyDynSettings_HE,
		rlmDomainWriteTxPwrEmiData_HE
	},
	{
		rlmDomainBuildDefaultPwrLimitPayload_EHT,
		rlmDomainBuildConfigPwrLimitPayload_EHT,
		rlmDomainDumpPwrLimitPayload_EHT,
		rlmDomainApplyDynSettings_EHT,
		rlmDomainWriteTxPwrEmiData_EHT
	},
};
#endif

static enum ENUM_PWR_LIMIT_DEFINE rlmDomainPwrLmtGetChannelDefine(void)
{
	enum ENUM_PWR_LIMIT_DEFINE ePwrLmtDef;

	/*  Primary channel or Center channel defination */
#if (COUNTRY_CHANNEL_TXPOWER_LIMIT_CHANNEL_DEFINE == 1)
	ePwrLmtDef = PWR_LIMIT_DEFINE_PRIMARY_CHANNEL;
#else
	ePwrLmtDef = PWR_LIMIT_DEFINE_CENTER_CHANNEL;
#endif

	return ePwrLmtDef;
}

static uint8_t rlmDomainPwrLmtGetDefaultBaseByCh(
	struct ADAPTER *prAdapter,
	uint8_t ucCurrChnl)
{
	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefaultPwrLmtBase =
		PWR_LIMIT_DEFAULT_BASE_NORMAL;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeType = PWR_MODE_6G_VLP;

	e6GPwrModeType = rlmDomainPwrLmt6GPwrModeGetByChnl(prAdapter,
		ucCurrChnl);
	if (e6GPwrModeType == PWR_MODE_6G_VLP)
		eDefaultPwrLmtBase = PWR_LIMIT_DEFAULT_BASE_VLP;
	else if ((e6GPwrModeType == PWR_MODE_6G_SP) ||
		(e6GPwrModeType == PWR_MODE_6G_LPI_SP))
		eDefaultPwrLmtBase = PWR_LIMIT_DEFAULT_BASE_SP;

	DBGLOG(RLM, LOUD, "pri[%d]ch[%d]mode[%d]base[%d]\n",
		rlmDomainPwrLmtGetChannelDefine(),
		ucCurrChnl,
		e6GPwrModeType,
		eDefaultPwrLmtBase);

#endif

	return eDefaultPwrLmtBase;
}

static enum ENUM_PWR_LIMIT_DEFAULT_BASE rlmDomainPwrLmtGetDefaultBase(
	struct ADAPTER *prAdapter)
{

	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefaultPwrLmtBase =
		PWR_LIMIT_DEFAULT_BASE_NORMAL;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeType = PWR_MODE_6G_VLP;

	e6GPwrModeType = rlmDomainPwrLmt6GPwrModeGet(prAdapter);
	if (e6GPwrModeType == PWR_MODE_6G_VLP)
		eDefaultPwrLmtBase = PWR_LIMIT_DEFAULT_BASE_VLP;
	else if ((e6GPwrModeType == PWR_MODE_6G_SP) ||
		(e6GPwrModeType == PWR_MODE_6G_LPI_SP))
		eDefaultPwrLmtBase = PWR_LIMIT_DEFAULT_BASE_SP;
#endif

	return eDefaultPwrLmtBase;
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
struct DOMAIN_INFO_ENTRY *rlmDomainGetDomainInfo(struct ADAPTER *prAdapter)
{
#define REG_DOMAIN_GROUP_NUM	(ARRAY_SIZE(arSupportedRegDomains))
#define REG_DOMAIN_DEF_IDX	(REG_DOMAIN_GROUP_NUM - 1)

	struct DOMAIN_INFO_ENTRY *prDomainInfo = NULL;
	struct REG_INFO *prRegInfo;
	uint8_t ucFound = FALSE;
	uint16_t u2TargetCountryCode, i, j;

	ASSERT(prAdapter);

	prRegInfo = &prAdapter->prGlueInfo->rRegInfo;

	if (prRegInfo->eRegChannelListMap == REG_CH_MAP_BLOCK_INDOOR)
		return &prAdapter->rBlockedDomainInfo;
	else if (prAdapter->prDomainInfo)
		return prAdapter->prDomainInfo;

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
		u2TargetCountryCode = prAdapter->rWifiVar.u2CountryCode;
#if (CFG_SUPPORT_RLM_DOMAIN_LOAD_FILE == 1)
		struct DOMAIN_INFO_CFG_ENTRY *prDomainInfoCfg;
		uint8_t *pucConfigBuf = NULL, ucCfgCountryNum;
		uint16_t u2CfgCountryCode;
		uint32_t u4CfgReadLen = 0;

		/* Check each cfg file to find target country. */
		for (i = 0; i < REG_DOMAIN_CFG_NUM; i++) {
			if (kalRequestFirmware(g_aucDomainCfgFileName[i],
				&pucConfigBuf, &u4CfgReadLen, FALSE,
				prAdapter->prGlueInfo->prDev) == 0) {
				/* ToDo:: Nothing */
			}
			if (!pucConfigBuf) {
				DBGLOG(RLM, INFO, "CFG file not existed!\n");
				break;
			}

			ucCfgCountryNum =
			  u4CfgReadLen / sizeof(struct DOMAIN_INFO_CFG_ENTRY);
			prDomainInfoCfg =
			  (struct DOMAIN_INFO_CFG_ENTRY *) pucConfigBuf;

			DBGLOG(RLM, TRACE,
				"%s ChannelPlan size[%d] CountryNum[%d]\n",
				g_aucDomainCfgFileName[i],
				u4CfgReadLen, ucCfgCountryNum);

			for (j = 0; j < ucCfgCountryNum; j++) {

				u2CfgCountryCode =
					(prDomainInfoCfg->u2CountryGroup >> 8) |
					((prDomainInfoCfg->u2CountryGroup
					& 0x00ff) << 8);
				/* Hit the target country. */
				if (u2TargetCountryCode == u2CfgCountryCode) {
					g_u2CfgCountryCode = u2CfgCountryCode;
					g_arCurRegDomain.pu2CountryGroup =
					  &g_u2CfgCountryCode;

					/* Save target country channel plan. */
					g_arCurRegDomain.u4CountryNum = 1;
					kalMemCopy(g_arCurRegDomain.rSubBand,
					  prDomainInfoCfg->rSubBand,
					  sizeof(g_arCurRegDomain.rSubBand));

				    DBGLOG(RLM, TRACE, "country %c%c in %s!\n",
					(g_arCurRegDomain.pu2CountryGroup[0]
					>> 8),
					(g_arCurRegDomain.pu2CountryGroup[0]
					& 0x00ff),
					g_aucDomainCfgFileName[i]);
					ucFound = TRUE;
					break;
				}
				prDomainInfoCfg++;
			}
			kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4CfgReadLen);
			if (ucFound) {
				prAdapter->prDomainInfo = &g_arCurRegDomain;
				return prAdapter->prDomainInfo;
			}
		}
#endif

		/* External cfg file not existed or country not found,
		 * use driver's.
		 */
		for (i = 0; i < REG_DOMAIN_GROUP_NUM; i++) {
			prDomainInfo = &arSupportedRegDomains[i];

			if ((prDomainInfo->u4CountryNum &&
			     prDomainInfo->pu2CountryGroup) ||
			    prDomainInfo->u4CountryNum == 0) {
				for (j = 0;
				     j < prDomainInfo->u4CountryNum;
				     j++) {
					if (prDomainInfo->pu2CountryGroup[j] ==
							u2TargetCountryCode) {
						ucFound = TRUE;
						break;
					}
				}
				if (ucFound)
					break;
			}
		}
		/* If no matched country code,
		 * use the default regulatory domain
		 */
		if (!ucFound) {
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
rlmDomainGetChnlList_V2(struct ADAPTER *prAd,
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
		max_count =
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
	} else if (eSpecificBand == BAND_5G) {
		i = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
		max_count =
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
	}
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eSpecificBand == BAND_6G) {
		i = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ);
		max_count = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ)
			+rlmDomainGetActiveChannelCount(prAd, KAL_BAND_6GHZ);
	}
#endif
	else {
		i = 0;
		max_count =
#if (CFG_SUPPORT_WIFI_6G == 1)
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_6GHZ) +
#endif
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
	}

	ucNum = 0;
	for (; i < max_count; i++) {
		prCh = rlmDomainGetActiveChannels(prAd) + i;
		if (fgNoDfs && kalIsChFlagMatch(prCh->eFlags, CHAN_RADAR))
			continue; /*not match*/

		if (i < rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ))
			band = BAND_2G4;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (i <
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ))
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

	if (regd_is_single_sku_en(prAdapter)) {
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
					DBGLOG(RLM, TRACE,
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
void rlmDomainGetDfsChnls_V2(struct ADAPTER *prAdapter,
			  uint8_t ucMaxChannelNum, uint8_t *pucNumOfChannel,
			  struct RF_CHANNEL_INFO *paucChannelList)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t idx, start_idx, end_idx, ucNum;
	struct CMD_DOMAIN_CHANNEL *prCh;

	/* 5G band */
	start_idx = rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_2GHZ);
	end_idx = rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_2GHZ) +
		rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_5GHZ);

	ucNum = 0;
	for (idx = start_idx; idx < end_idx; idx++) {
		prCh = rlmDomainGetActiveChannels(prAdapter) + idx;
		if (!kalIsChFlagMatch(prCh->eFlags, CHAN_RADAR))
			continue;

		paucChannelList[ucNum].eBand = BAND_5G;
		paucChannelList[ucNum].ucChannelNum = prCh->u2ChNum;

		ucNum++;
		if (ucMaxChannelNum == ucNum)
			break;
	}

	*pucNumOfChannel = ucNum;
#else
	*pucNumOfChannel = 0;
#endif /* CFG_SUPPORT_SINGLE_SKU */
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

	if (regd_is_single_sku_en(prAdapter))
		return rlmDomainGetDfsChnls_V2(prAdapter, ucMaxChannelNum,
				pucNumOfChannel, paucChannelList);

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

	if (!regd_is_single_sku_en(prAdapter))
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

	for (i = 0; i < ARRAY_SIZE(g_u2CountryGroup4); i++) {
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
	rlmExtractChannelInfo(prAdapter, max_channel_count, prChs);

	prCmd->u4CountryCode = rlmDomainGetCountryCode(prAdapter);
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

	if (regd_is_single_sku_en(prAdapter)) {
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
	prCmd->ucRegCat = (uint8_t)prDomainInfo->eRegCat;
	prCmd->aucReserved[0] = 0;

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];
#if (CFG_SUPPORT_WIFI_6G == 1)
		/* If HW doesn't support 6G,
		 * we should not add 6G channel into command.
		 */
		if (prAdapter->fgIsHwSupport6G == FALSE
			&& prSubBand->ucBand == BAND_6G)
			continue;
#endif
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
			prCmd->rSubBand[i].fgDfs
						= prSubBand->fgDfs;
		}
	}

	DBGLOG(RLM, TRACE,
	       "Domain: ValidCC=%c%c, PwrLimitCC=%c%c, Reg=%d\n",
	       (prAdapter->rWifiVar.u2CountryCode & 0xff00) >> 8,
	       (prAdapter->rWifiVar.u2CountryCode & 0x00ff),
	       ((prCmd->u2CountryCode & 0xff00) >> 8),
	       (prCmd->u2CountryCode & 0x00ff),
	       prCmd->ucRegCat);

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

	prCmd->ucRegCat = (uint8_t)prDomainInfo->eRegCat;

	for (i = 0; i < MAX_SUBBAND_NUM; i++) {
		prSubBand = &prDomainInfo->rSubBand[i];
#if (CFG_SUPPORT_WIFI_6G == 1)
		/* If HW doesn't support 6G,
		 * we should not add 6G channel into command.
		 */
		if (prAdapter->fgIsHwSupport6G == FALSE
			&& prSubBand->ucBand == BAND_6G)
			continue;
#endif
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

	/* Get domain info from arSupportedRegDomains
	 * to set regulatory category
	 */
	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	prCmd->ucRegCat = (uint8_t)prDomainInfo->eRegCat;

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
u_int8_t rlmDomainIsLegalChannel_V2(struct ADAPTER *prAd,
				    enum ENUM_BAND eBand,
				    uint8_t ucChannel)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t idx, start_idx, end_idx;
	struct CMD_DOMAIN_CHANNEL *prCh;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eBand == BAND_6G) {
		start_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ);
		end_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_6GHZ);
	} else
#endif
	if (eBand == BAND_2G4) {
		start_idx = 0;
		end_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
	} else {
		start_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ);
		end_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ) +
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ);
	}

	for (idx = start_idx; idx < end_idx; idx++) {
		prCh = rlmDomainGetActiveChannels(prAd) + idx;

		if (prCh->u2ChNum == ucChannel)
			return TRUE;
	}

	return FALSE;
#else
	return FALSE;
#endif
}

u_int8_t rlmDomainIsStaSapIndoorConn(struct ADAPTER *prAdapter)
{
#if (CFG_CH_SELECT_ENHANCEMENT == 1)
	return prAdapter->rWifiVar.ucStaSapIndoorConn;
#else
	return 0;
#endif
}

u_int8_t rlmDomainIsIndoorChannel(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand, uint8_t ucChannel)
{
	if (eBand == BAND_5G) {
		uint32_t u4CountryNum = 0;
		uint8_t i;

		if (ucChannel >= 36 && ucChannel <= 48) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType1);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType1[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 36 && ucChannel <= 64) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType0);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType0[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 100 && ucChannel <= 128) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType2);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType2[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 100 && ucChannel <= 140) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType3);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType3[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 100 && ucChannel <= 144) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType4);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType4[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 149 && ucChannel <= 165) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType5);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType5[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
		if (ucChannel >= 169 && ucChannel <= 177) {
			u4CountryNum = ARRAY_SIZE(g_u2IndoorType6);
			for (i = 0; i < u4CountryNum; i++) {
				if (g_u2IndoorType6[i] ==
				    prAdapter->rWifiVar.u2CountryCode)
					return TRUE;
			}
		}
	}

	return FALSE;
}

u_int8_t rlmDomainIsLegalChannel(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand, uint8_t ucChannel)
{
	uint8_t i, j;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	if (regd_is_single_sku_en(prAdapter))
		return rlmDomainIsLegalChannel_V2(prAdapter, eBand, ucChannel);

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	ASSERT(prDomainInfo);

	for (i = 0; i < ARRAY_SIZE(prDomainInfo->rSubBand); i++) {
		prSubband = &prDomainInfo->rSubBand[i];

		if (prSubband->ucBand == BAND_5G && !prAdapter->fgEnable5GBand)
			continue;

#if (CFG_SUPPORT_WIFI_6G == 1)
		if (prSubband->ucBand == BAND_6G && !prAdapter->fgIsHwSupport6G)
			continue;
#endif

		if (prSubband->ucBand != eBand)
			continue;

		for (j = 0; j < prSubband->ucNumChannels; j++) {
			if (prSubband->ucFirstChannelNum +
			    j * prSubband->ucChannelSpan != ucChannel)
				continue;

			if (!kalIsValidChnl(prAdapter->prGlueInfo,
					    ucChannel, prSubband->ucBand)) {
				DBGLOG(RLM, INFO, "Not support ch(%d) b(%d)!\n",
				       ucChannel, eBand);
				return FALSE;
			} else {
				return TRUE;
			}
		}
	}
	return FALSE;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is used to check whether the current channel for current
 *        network type is legal or not
 *
 * \param[in] prAdapter
 * \param[in] eBand : RF Band index
 * \param[in] ucPriCh : Primary
 * \param[in] ucBw : Bandwidth
 * \param[in] eNetType : Network type
 *
 * \return ucIsLegal : is legal or not.
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomainIsLegalChlByNetType(struct ADAPTER *prAdapter,
				 enum ENUM_BAND eBand,
				 uint8_t ucPriCh,
				 uint8_t ucBw,
				 enum ENUM_NETWORK_TYPE eNetType)
{
	uint8_t ucIsLegal = FALSE;
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1 && !CFG_SUPPORT_CE_6G_PWR_REGULATIONS)
	uint8_t ucIsSupport = 0;
	uint32_t u4Status = WLAN_STATUS_FAILURE;
#endif

	if (!prAdapter || (ucBw >= MAX_BW_UNKNOWN) ||
		(eNetType >= NETWORK_TYPE_NUM)) {
		DBGLOG(RLM, INFO,
			"Invalid param. prAdapter[%s]BW[%d]NetType[%d]\n",
				!prAdapter ? "NULL" : "Valid",
				ucBw,
				eNetType);
		return FALSE;
	}

	ucIsLegal = rlmDomainIsLegalChannel(prAdapter, eBand, ucPriCh);

	if (!ucIsLegal)
		return ucIsLegal;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (eBand != BAND_6G)
		return ucIsLegal;

	/* For STA, it will check whether the AP 6G power mode is support or not
	 * for current country & channe while scan, if not support, the AP will
	 * not show on scan list.
	 */
	if (eNetType == NETWORK_TYPE_AIS)
		return ucIsLegal;

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	/* Currently only consider P2P & NAN, since both NetType must use VLP */
	ucIsLegal = rlmDomain6GPwrModeSupportChk(
			prAdapter,
			PWR_MODE_6G_VLP,
			NULL,
			eBand,
			ucPriCh,
			ucBw);
#else
	/* Currently only consider P2P & NAN, since both NetType must use VLP */
	u4Status = rlmDomain6GPwrModeCountrySupportChk(
			prAdapter->rWifiVar.u2CountryCode,
			eBand,
			ucPriCh,
			ucBw,
			PWR_MODE_6G_VLP,
			&ucIsSupport);

	if (u4Status == WLAN_STATUS_SUCCESS)
		ucIsLegal = ucIsSupport;
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */

	DBGLOG(RLM, TRACE,
		"band[%d]PriCh[%d]Bw[%d]Net_Type[%d]mode[%d]Legal[%d]\n",
		eBand,
		ucPriCh,
		ucBw,
		eNetType,
		PWR_MODE_6G_VLP,
		ucIsLegal);

#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */

	return ucIsLegal;
}

u_int8_t rlmDomainIsLegalDfsChannel_V2(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucChannel)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	uint8_t idx, start_idx, end_idx;
	struct CMD_DOMAIN_CHANNEL *prCh;

	if (eBand != BAND_5G)
		return FALSE;

	start_idx = rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_2GHZ);
	end_idx = rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_2GHZ) +
		rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_5GHZ);

	for (idx = start_idx; idx < end_idx; idx++) {
		prCh = rlmDomainGetActiveChannels(prAdapter) + idx;
		if (prCh->u2ChNum == ucChannel &&
			kalIsChFlagMatch(prCh->eFlags, CHAN_RADAR)) {
			return TRUE;
		}
	}

	return FALSE;
#else
	return FALSE;
#endif
}

u_int8_t rlmDomainIsLegalDfsChannel(struct ADAPTER *prAdapter,
		enum ENUM_BAND eBand, uint8_t ucChannel)
{
	uint8_t i, j;
	struct DOMAIN_SUBBAND_INFO *prSubband;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;

	if (regd_is_single_sku_en(prAdapter))
		return rlmDomainIsLegalDfsChannel_V2(
				prAdapter, eBand, ucChannel);

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
 * @brief retrun channel interval
 *
 * @param[in] u2SubBandIdx
 * @param[in] ucCurrCh
 *
 * @return channel interval
 */
/*----------------------------------------------------------------------------*/
static uint8_t
rlmDomainGetChannelInterval(uint16_t u2SubBandIdx,
			uint8_t ucCurrCh)
{
	uint8_t ucInterval = 0;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if ((g_rRlmSubBand[u2SubBandIdx].eBand == BAND_6G) &&
		(ucCurrCh == 1 || ucCurrCh == 2))
		ucInterval = 1;
	else if ((g_rRlmSubBand[u2SubBandIdx].eBand == BAND_6G) &&
		(ucCurrCh == 221 || ucCurrCh == 229))
		ucInterval = 4;
	else
#endif
		ucInterval =  g_rRlmSubBand[u2SubBandIdx].ucInterval;

	return ucInterval;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is use to check whether current country limit channel
 *        match channel group keywork.
 *
 * @param[in] eBand : RF band
 * @param[in] i2CurrCh : Current country channel from configuration table
 * @param[in] ucTarCh : Target channel which is use for CMD
 *
 * @return bool : Match or not
 */
/*----------------------------------------------------------------------------*/
static bool rlmDomainIsMatchCntryChGrpKey(
	enum ENUM_BAND eBand,
	int16_t i2CurrCh,
	uint8_t ucTarCh)
{
	uint8_t i = 0;
	uint8_t ucTblSize =
		(sizeof(g_rCountryLmtChGrpTbl) /
		sizeof(struct COUNTRY_LIMIT_CHNL_GRP_TABLE));
	uint8_t fgDoArb = TRUE;

	for (i = 0; i < ucTblSize; i++) {
		if ((eBand == BAND_2G4 || eBand == BAND_5G) &&
		COUTRY_LMT_CH_MATCH_KEY_LEGACY(i, eBand, i2CurrCh, ucTarCh)) {
			fgDoArb = TRUE;
			break;
		} else if (
		COUTRY_LMT_CH_MATCH_KEY_6G(i, eBand, i2CurrCh, ucTarCh)) {
			fgDoArb = TRUE;
			break;
		}
	}

	if (i >= ucTblSize) {
		/* not found */
		fgDoArb = FALSE;
	}

	DBGLOG(RLM, LOUD, "Band[%d]CurrCh[%d]TarCh[%d]Arb[%d]",
					eBand,
					i2CurrCh,
					ucTarCh,
					fgDoArb);

	return fgDoArb;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is use to check current condition is valid, if valid
 *        will do power limt arbitor
 *
 * @param[in] u2CurrContryCode : Current country from configuration table
 * @param[in] u2TarContryCode : Target country channel is use for CMD
 * @param[in] fgChlValide : Channel valid or not
 * @param[in] eBand : RF band
 * @param[in] i2CurrCh : Current country channel from configuration table
 * @param[in] ucTarCh : Target channel which is use for CMD
 *
 * @return bool : Do arbitor or not
 */
/*----------------------------------------------------------------------------*/
static bool rlmDomainPwrLmtArbitorPreProccess(
	struct ADAPTER *prAdapter,
	uint16_t u2CurrContryCode,
	uint16_t u2TarContryCode,
	bool fgChlValide,
	enum ENUM_BAND eBand,
	int16_t i2CurrCh,
	uint8_t ucTarCh)
{

	if (u2CurrContryCode != u2TarContryCode) {
		return FALSE;
	} else if (fgChlValide == FALSE) {
		return FALSE;
	} else if (i2CurrCh != ucTarCh) {
		if (rlmDomainIsMatchCntryChGrpKey(eBand, i2CurrCh, ucTarCh)) {
			/* match key word */
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return TRUE;
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
u_int8_t rlmDomainCheckEntryValidSpecialCase(struct ADAPTER *prAdapter,
				enum ENUM_BAND eBand, int16_t i2CentralCh)
{
	uint8_t i = 0;
	uint8_t ucTblSize =
		(sizeof(g_rCountryLmtChGrpTbl) /
		sizeof(struct COUNTRY_LIMIT_CHNL_GRP_TABLE));

	if ((eBand == BAND_5G) && (i2CentralCh == 50)) {
		/* CH50 is not located in any FCC subbands
		 * but it's a valid central channel for 160C
		 */
		return TRUE;
	}

	for (i = 0; i < ucTblSize; i++) {
		if ((eBand == g_rCountryLmtChGrpTbl[i].eBand) &&
			(i2CentralCh == g_rCountryLmtChGrpTbl[i].i2Key)) {
			DBGLOG(RLM, LOUD, "Special case band[%d]Ch[%d]\n",
							eBand,
							i2CentralCh);
			return TRUE;
		}
	}

	return FALSE;
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
				enum ENUM_BAND eBand, int16_t i2CentralCh)
{
	u_int8_t fgValid = FALSE;
	uint8_t ucTemp = 0xff;
	uint8_t i;

	if (rlmDomainCheckEntryValidSpecialCase(prAdapter, eBand, i2CentralCh))
		return TRUE;

	for (i = PWR_LMT_SUBBAND_2G4; i < PWR_LMT_SUBAND_NUM; i++) {
		if ((eBand == BAND_NULL || eBand == g_rRlmSubBand[i].eBand) &&
			(i2CentralCh >= g_rRlmSubBand[i].ucStartCh) &&
			(i2CentralCh <= g_rRlmSubBand[i].ucEndCh)) {

			ucTemp = (i2CentralCh - g_rRlmSubBand[i].ucStartCh) %
			rlmDomainGetChannelInterval(i, i2CentralCh);
		}
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
#if (CFG_SUPPORT_UNII4 == 0)
			if (ucPriChannel == 165 && eBand == BAND_5G) {
				fgValidBW = FALSE;
				DBGLOG(RLM, WARN,
				       "Rf: PriOffSet=%d, W=%d C=%d\n",
				       u4PrimaryOffset, eChannelWidth,
				       ucPriChannel);
			}
#endif
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

uint16_t rlmDomainReverseAlpha2(uint16_t Alpha2)
{
	uint16_t u2CountryCode = 0;

	u2CountryCode = (Alpha2 & 0x00ff) << 8;
	u2CountryCode |= (Alpha2 & 0xff00) >> 8;

	return u2CountryCode;
}

#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)

uint32_t
rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4CountryCode, uint8_t fgNeedHoldRtnlLock)
{
	const void *pRegdom = NULL;
	char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};
	uint32_t u4FinalCountryCode = u4CountryCode;

	rlmDomainU32ToAlpha(u4FinalCountryCode, acCountryCodeStr);
	pRegdom =
		rlmDomainSearchRegdomainFromLocalDataBase(acCountryCodeStr);
	if (!pRegdom) {
		DBGLOG(RLM, ERROR,
	       "Cannot find the %s RegDomain. Set to default WW\n",
	       acCountryCodeStr);
		pRegdom = kalGetDefaultRegWW();
		u4FinalCountryCode = COUNTRY_CODE_WW;
	}

	kalApplyCustomRegulatory(prGlueInfo, pRegdom, fgNeedHoldRtnlLock);

	return u4FinalCountryCode;
}
#else
uint32_t
rlmDomainUpdateRegdomainFromaLocalDataBaseByCountryCode(
	struct GLUE_INFO *prGlueInfo,
	uint32_t u4CountryCode, uint8_t fgNeedHoldRtnlLock)
{
	return 0;
}
#endif

uint8_t
rlmDomainCountryCodeUpdateSanity(
	struct GLUE_INFO *prGlueInfo,
	struct ADAPTER **prAdapter)
{
	enum regd_state eCurrentState = rlmDomainGetCtrlState(prGlueInfo);

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
	uint32_t u4CountryCode,
	uint8_t fgNeedHoldRtnlLock)
{
	uint32_t u4FinalCountryCode = u4CountryCode;
	uint16_t u2CountryCode;
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
				prAdapter->prGlueInfo,
				u4CountryCode, fgNeedHoldRtnlLock);
	}

	rlmDomainU32ToAlpha(u4FinalCountryCode, acCountryCodeStr);

	if (u4FinalCountryCode != u4CountryCode)
		rlmDomainSetCountryCode(prAdapter,
			acCountryCodeStr,
			MAX_COUNTRY_CODE_LEN);

	DBGLOG(RLM, INFO, "prRegdControl->alpha2 = %s\n", acCountryCodeStr);
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

	if (!regd_is_single_sku_en(prAdapter))
		return;

	u2CountryCode = (uint16_t)rlmDomainGetCountryCode(prAdapter);
	prAdapter->rWifiVar.u2CountryCode = u2CountryCode;

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	u2CountryCode = rlmDomainReverseAlpha2(u2CountryCode);
	if ((u2CountryCode == COUNTRY_CODE_US)
		|| (u2CountryCode == COUNTRY_CODE_CA)) {
		prAdapter->fgTpcEn = TRUE;
		DBGLOG(RLM, INFO,
		"TPC flag is enabled for 6GHz in Country(%c%c)\n",
				((u2CountryCode & 0xff00) >> 8),
				(u2CountryCode & 0x00ff));
	} else {
		prAdapter->fgTpcEn = FALSE;
	}
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */

#if CFG_SUPPORT_NAN
	nanExtSetCountryCodeHandler(prAdapter);
#endif /* CFG_SUPPORT_NAN */

	/* Send commands to firmware */
	rlmDomainSendCmd(prAdapter, TRUE);

}
void
rlmDomainSetCountry(struct ADAPTER *prAdapter, uint8_t fgNeedHoldRtnlLock)
{
	struct GLUE_INFO *prGlueInfo = rlmDomainGetGlueInfo(prAdapter);
	struct ADAPTER *prBaseAdapter;

	if (!rlmDomainCountryCodeUpdateSanity(
		prGlueInfo, &prBaseAdapter)) {
		DBGLOG(RLM, WARN, "sanity check failed, skip update\n");
		return;
	}

	rlmDomainCountryCodeUpdate(
		prBaseAdapter,
		rlmDomainGetCountryCode(prAdapter),
		fgNeedHoldRtnlLock);
}

uint8_t rlmDomainTxPwrLimitGetTableVersion(
	uint8_t *pucBuf, uint32_t u4BufLen)
{
#define TX_PWR_LIMIT_VERSION_STR_LEN 7
#define TX_PWR_LIMIT_MAX_VERSION 3
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
	uint8_t *prFileName,
	struct ADAPTER *prAdapter, uint8_t *pucBuf, uint32_t u4BufLen,
	uint8_t ucVersion, uint32_t u4CountryCode,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	uint8_t uSecIdx = 0;
	uint8_t ucSecNum = gTx_Pwr_Limit_Section[ucVersion].ucSectionNum;
	uint32_t u4CountryStart = 0, u4CountryEnd = 0, u4Pos = 0;
	struct TX_PWR_LIMIT_SECTION *prSection =
		&gTx_Pwr_Limit_Section[ucVersion];
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

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
				prGlueInfo->bTxBfBackoffExists = TRUE;
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
	uint16_t section = 0, e = 0, count = 0;
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
	uint8_t *prFileName,
	struct ADAPTER *prAdapter,
	uint8_t **pucConfigBuf, uint32_t *pu4ConfigReadLen)
{
#define TXPWRLIMIT_FILE_LEN 64
	u_int8_t bRet = TRUE;
	uint8_t aucPath[TXPWRLIMIT_FILE_LEN];
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
#if CFG_SUPPORT_MULTI_CARD
	uint32_t u4Idx;
#endif

	if (!prFileName || kalStrLen(prFileName) == 0) {
		bRet = FALSE;
		DBGLOG(RLM, ERROR, "Invalid TxPwrLimit dat file name!!\n");
		goto error;
	}

	kalMemZero(aucPath, sizeof(aucPath));

	/*
	 * case 0: Read ./wlanXXX/TxPwrLimit.dat
	 * case 1: Read ./TxPwrLimit.dat
	 */
#if CFG_SUPPORT_MULTI_CARD
	for (u4Idx = READ_DEV_CFG; u4Idx < READ_CFG_TOTAL_NUM; u4Idx++) {
		if (u4Idx == READ_DEV_CFG &&
			prGlueInfo->aucDevCfgPath[0] == '\0')
			continue;

		kalMemZero(aucPath, sizeof(aucPath));

		if (u4Idx == READ_DEV_CFG)
			kalSnprintf(aucPath, TXPWRLIMIT_FILE_LEN, "%s/%s",
				prGlueInfo->aucDevCfgPath, prFileName);
		else
#endif /* CFG_SUPPORT_MULTI_CARD */
			kalSnprintf(aucPath, TXPWRLIMIT_FILE_LEN,
				"%s", prFileName);

		if (kalRequestFirmware(
				aucPath,
				pucConfigBuf,
				pu4ConfigReadLen,
				FALSE,
				kalGetGlueDevHdl(prGlueInfo)) == 0) {
			/* ToDo:: Nothing */
#if CFG_SUPPORT_MULTI_CARD
			break;
		} else if (u4Idx < READ_CFG_TOTAL_NUM - 1) {
			continue;
#endif
		} else {
			bRet = FALSE;
			goto error;
		}
#if CFG_SUPPORT_MULTI_CARD
	}
#endif

	if (*pu4ConfigReadLen == 0) {
		bRet = FALSE;
		goto error;
	}

error:

	return bRet;
}

u_int8_t rlmDomainGetTxPwrLimit(
	uint8_t *prFileName,
	uint32_t country_code,
	uint8_t *pucVersion,
	struct GLUE_INFO *prGlueInfo,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData)
{
	u_int8_t bRet;
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;

	bRet = rlmDomainTxPwrLimitLoadFromFile(prFileName,
		prGlueInfo->prAdapter,
		&pucConfigBuf, &u4ConfigReadLen);

	if (!bRet)
		goto error;

	rlmDomainTxPwrLimitRemoveComments(pucConfigBuf, u4ConfigReadLen);
	*pucVersion = rlmDomainTxPwrLimitGetTableVersion(pucConfigBuf,
		u4ConfigReadLen);

	if (!rlmDomainTxPwrLimitLoad(
		prFileName,
		prGlueInfo->prAdapter,
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
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
#define PwrLmtConf g_rRlmPowerLimitConfigurationLegacy
#define PerPwrLmtConfSize \
	sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY)
#define LEGACY_PWR_LIMIT_NUM PWR_LIMIT_LEGACY_NUM
#else
#define PwrLmtConf g_rRlmPowerLimitConfiguration
#define PerPwrLmtConfSize \
	sizeof(struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION)
#define LEGACY_PWR_LIMIT_NUM PWR_LIMIT_NUM
#endif

	uint16_t i, j, k;
	uint16_t u2CountryCodeTable, u2CountryCodeCheck;
	u_int8_t fgChannelValid = FALSE;
	u_int8_t fgEntryRepetetion = FALSE;
	u_int8_t fgTableValid = TRUE;
	char ucMsgBuf[PWR_BUF_LEN] = {0};
	u_int8_t ucMsgOfs = 0;
	u_int8_t ucUpBound = 0;
	u_int8_t ucLowBound = 0;

	/*1.Configuration Table Check */
	for (i = 0; i < sizeof(PwrLmtConf) /
	     PerPwrLmtConfSize; i++) {
		/*Table Country Code */
		WLAN_GET_FIELD_BE16(&PwrLmtConf[i].aucCountryCode[0],
				    &u2CountryCodeTable);

		/*<1>Repetition Entry Check */
		for (j = i + 1; j < sizeof(PwrLmtConf) /
		     PerPwrLmtConfSize;
		     j++) {

			WLAN_GET_FIELD_BE16(&PwrLmtConf[j].aucCountryCode[0],
					    &u2CountryCodeCheck);
			if (((PwrLmtConf[i].i2CentralCh) ==
			     PwrLmtConf[j].i2CentralCh)
			    && (u2CountryCodeTable == u2CountryCodeCheck)) {
				fgEntryRepetetion = TRUE;
				DBGLOG(RLM, LOUD,
				       "Domain: Configuration Repetition CC=%c%c, Ch=%d\n",
				       PwrLmtConf[i].aucCountryCode[0],
				       PwrLmtConf[i].aucCountryCode[1],
				       PwrLmtConf[i].i2CentralCh);
			}
		}

		/*<2>Channel Number Interval Check */
		fgChannelValid =
		    rlmDomainCheckChannelEntryValid(prAdapter,
				BAND_NULL, PwrLmtConf[i].i2CentralCh);

		if (fgChannelValid == FALSE) {
			fgTableValid = FALSE;

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
			ucUpBound = PWR_LIMIT_LEGACY_CCK_L;
			ucLowBound = PWR_LIMIT_LEGACY_160M_H;
#else
			ucUpBound = PWR_LIMIT_CCK_L;
			ucLowBound = PWR_LIMIT_160M_H;
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

			for (k = ucUpBound; k < ucLowBound; k++) {
				ucMsgOfs += snprintf(ucMsgBuf + ucMsgOfs,
					PWR_BUF_LEN - ucMsgOfs,
					"%d,",
					PwrLmtConf[i].aucPwrLimit[k]);
			}

			DBGLOG(RLM, LOUD,
				"Domain: CC=%c%c, Ch=%d, Limit: %s, Valid:%d\n",
				PwrLmtConf[i].aucCountryCode[0],
				PwrLmtConf[i].aucCountryCode[1],
				PwrLmtConf[i].i2CentralCh,
				ucMsgBuf,
				fgChannelValid);
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
#undef PerPwrLmtConfSize
#undef LEGACY_PWR_LIMIT_NUM
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
	uint16_t u2CountryCode,
	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefBase)
{
	uint16_t i, j;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	uint16_t u2TableIndex = POWER_LIMIT_TABLE_NULL;	/* No Table Match */
	struct COUNTRY_POWER_LIMIT_GROUP_TABLE *prCountryGrpInfo;
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLmtDefaultTable =
							g_rRlmPowerLimitDefault;
	uint16_t u2PwrLmtDefaultTalbeSize = sizeof(g_rRlmPowerLimitDefault) /
			sizeof(struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT);

	for (i = 0; i < COUNTRY_PWR_LMT_GROUPS_TBL_USED_SIZE; i++) {
		prCountryGrpInfo = &COUNTRY_PWR_LMT_GROUPS_TBL_USED[i];

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

	if (i >= COUNTRY_PWR_LMT_GROUPS_TBL_USED_SIZE) {
		DBGLOG(RLM, TRACE,
			"Can't find Country = (%c%c) in any group!\n",
			((u2CountryCode & 0xff00) >> 8),
			(u2CountryCode & 0x00ff));
	}

	if (eDefBase >= PWR_LIMIT_DEFAULT_BASE_NUM)
		return POWER_LIMIT_TABLE_NULL;

	prPwrLmtDefaultTable = PWR_LIMIT_COUNTRY_DEF_TBL_ALL(eDefBase);
	u2PwrLmtDefaultTalbeSize = PWR_LIMIT_COUNTRY_DEF_TBL_ALL_NUM(eDefBase);


	/*Default Table Index */
	for (i = 0; i < u2PwrLmtDefaultTalbeSize; i++) {

		WLAN_GET_FIELD_BE16(&prPwrLmtDefaultTable[i].
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
#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
void
rlmDomainGetStartEndIdx(enum ENUM_PWR_LIMIT_TYPE eType,
		uint8_t *pu1StartIdx, uint8_t *pu1EndIdx)
{
	if (pu1StartIdx == NULL || pu1EndIdx == NULL)
		return;

	switch (eType) {
	case PWR_LIMIT_TYPE_COMP_11AC:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11AC_V2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11AX:
		kal_fallthrough;
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
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII5A;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII5B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2: {
		*pu1StartIdx = PWR_LMT_SUBBAND_UNII6;
		*pu1EndIdx = PWR_LMT_SUBBAND_UNII7B;
	}
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_3:
		kal_fallthrough;
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 1)*/
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
#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
void
rlmDomainBuildCmdByDefaultTable(struct ADAPTER *prAdapter,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
	uint16_t u2DefaultTableIndex)
{
	uint16_t i, k;
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitSubBand =
			&g_rRlmPowerLimitDefault[u2DefaultTableIndex];
	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefBase;
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

	eType = prCmd->ucLimitType;

	eDefBase = rlmDomainPwrLmtGetDefaultBase(prAdapter);
	prPwrLimitSubBand =
		PWR_LIMIT_COUNTRY_DEF_TBL(eDefBase, u2DefaultTableIndex);

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
		     k += rlmDomainGetChannelInterval(i, k)) {

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
					kalMemSet(
					&prPwrLmtLegacy6G->cPwrLimitCCK_L,
					cLmtBand,
					PWR_LIMIT_LEGACY_6G_NUM);
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
					kalMemSet(&prPwrLimit->cPwrLimitCCK_L,
						cLmtBand, PWR_LIMIT_NUM);
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
					prPwrLmtLegacy6G->cPwrLimitCCK_L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitCCK_H
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitOFDM_L
						= cLmtBand;
					prPwrLmtLegacy6G->cPwrLimitOFDM_H
						= cLmtBand;
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
					prPwrLimit->cPwrLimitCCK_L = cLmtBand;
					prPwrLimit->cPwrLimitCCK_H = cLmtBand;
					prPwrLimit->cPwrLimitOFDM_L = cLmtBand;
					prPwrLimit->cPwrLimitOFDM_H = cLmtBand;
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
void rlmDomainCopyFromConfigTable(struct CMD_CHANNEL_POWER_LIMIT *prCmdPwrLimit,
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION *prPwrLimitConfig)
{
	kalMemCopy(&prCmdPwrLimit->cPwrLimitCCK_L,
		   &prPwrLimitConfig->aucPwrLimit[0],
		   PWR_LIMIT_NUM);
}
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

static void PwrLmtTblArbitrator(int8_t *target,
	int8_t *compare,
	uint32_t size)
{
	uint8_t i = 0;

	/* Choose min value from target & compare */
	for (i = 0; i < size; i++) {
		if (target[i] > compare[i])
			target[i] = compare[i];
	}
}

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
static void rlmDomainCompareFromConfigTable(int8_t *prPwrLmt,
	int8_t *prPwrLmtConf,
	enum ENUM_PWR_LIMIT_TYPE eType)
{
	uint32_t size;

	switch (eType) {
	case PWR_LIMIT_TYPE_COMP_11AC_V2:
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
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11BE_2:
		size = PWR_LIMIT_EHT_NUM;
		break;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#if (CFG_SUPPORT_WIFI_6G == 1)
	case PWR_LIMIT_TYPE_COMP_6E_1:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_6E_2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_6E_3:
		size = PWR_LIMIT_6E_NUM;
		break;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_1:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_6G_3:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3:
		size = PWR_LIMIT_LEGACY_6G_NUM;
		break;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_LIMIT_TYPE_COMP_11BE_6G_1:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_2:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_3:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_4:
		kal_fallthrough;
	case PWR_LIMIT_TYPE_COMP_11BE_6G_5:
		kal_fallthrough;
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/
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
#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
void rlmDomainBuildCmdByConfigTable(struct ADAPTER *prAdapter,
			struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd)
{
	uint16_t i, k;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	enum ENUM_PWR_LIMIT_TYPE eType = prCmd->ucLimitType;
	u_int8_t fgChannelValid;
	uint8_t ucCentCh;

	/* Legacy */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION
		*prPwrLmtConf = PWR_LIMIT_COUNTRT_CFG_V0_LEGACY_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT *prCmdPwrLimit;
	uint16_t ucPwrLmitConfSize = PWR_LIMIT_COUNTRT_CFG_V0_LEGACY_GET_SIZE();

	/* HE */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE
		*prPwrLmtConfHE = PWR_LIMIT_COUNTRT_CFG_V0_HE_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_HE *prCmdPwrLimtHE;
	uint8_t ucPwrLmitConfSizeHE = PWR_LIMIT_COUNTRT_CFG_V0_HE_GET_SIZE();

	/* HE160 */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE_BW160
		*prPwrLmtConfHEBW160 =
			PWR_LIMIT_COUNTRT_CFG_V0_HE160_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 *prCmdPwrLimtHEBW160;
	uint8_t ucPwrLmitConfSizeHEBW160 =
		PWR_LIMIT_COUNTRT_CFG_V0_HE160_GET_SIZE();

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/* EHT */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT
		*prPwrLmtConfEHT = PWR_LIMIT_COUNTRT_CFG_V0_EHT_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_EHT *prCmdPwrLimtEHT;
	uint8_t ucPwrLmitConfSizeEHT =
		PWR_LIMIT_COUNTRT_CFG_V0_EHT_GET_SIZE();
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	/* 6G */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_6E
		*prPwrLmtConf6E = PWR_LIMIT_COUNTRT_CFG_V0_HE6G_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_6E *prCmdPwrLimt6E;
	uint8_t ucPwrLmitConfSize6E = PWR_LIMIT_COUNTRT_CFG_V0_HE6G_GET_SIZE();

	/* Legacy 6G */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY_6G
		*prPwrLmtConfLegacy_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G *prCmdPwrLimtLegacy_6G;
	uint8_t ucPwrLmitConfSizeLegacy_6G =
		PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_GET_SIZE();
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/* Legacy 6G */
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT_6G
		*prPwrLmtConfEHT_6G = PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_GET_DATA();
	struct CMD_CHANNEL_POWER_LIMIT_EHT_6G *prCmdPwrLimtEHT_6G;
	uint8_t ucPwrLmitConfSizeEHT_6G =
		PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_GET_SIZE();
#endif
#endif
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (rlmDomainPwrLmt6GPwrModeGet(prAdapter) == PWR_MODE_6G_VLP) {
		prPwrLmtConf6E = PWR_LIMIT_COUNTRT_CFG_V0_HE6G_VLP_GET_DATA();
		ucPwrLmitConfSize6E =
			PWR_LIMIT_COUNTRT_CFG_V0_HE6G_VLP_GET_SIZE();
		prPwrLmtConfLegacy_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_VLP_GET_DATA();
		ucPwrLmitConfSizeLegacy_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_VLP_GET_SIZE();
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		prPwrLmtConfEHT_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_VLP_GET_DATA();
		ucPwrLmitConfSizeEHT_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_VLP_GET_SIZE();
#endif
	} else if ((rlmDomainPwrLmt6GPwrModeGet(prAdapter) == PWR_MODE_6G_SP) ||
		(rlmDomainPwrLmt6GPwrModeGet(prAdapter) ==
		PWR_MODE_6G_LPI_SP)) {
		prPwrLmtConf6E = PWR_LIMIT_COUNTRT_CFG_V0_HE6G_SP_GET_DATA();
		ucPwrLmitConfSize6E =
			PWR_LIMIT_COUNTRT_CFG_V0_HE6G_SP_GET_SIZE();
		prPwrLmtConfLegacy_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_SP_GET_DATA();
		ucPwrLmitConfSizeLegacy_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_LEGACY6G_SP_GET_SIZE();
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		prPwrLmtConfEHT_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_SP_GET_DATA();
		ucPwrLmitConfSizeEHT_6G =
			PWR_LIMIT_COUNTRT_CFG_V0_EHT6G_SP_GET_SIZE();
#endif
	}
#endif /* #CFG_SUPPORT_WIFI_6G_PWR_MODE */


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
					&prPwrLmtConfHE[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
						prPwrLmtConfHE[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_NULL,
					prPwrLmtConfHE[i].i2CentralCh,
					ucCentCh) == FALSE)
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
					&prPwrLmtConfHE[i].aucPwrLimit[0],
					eType);
			}
		} else if (eType == PWR_LIMIT_TYPE_COMP_11AX_BW160) {
			prCmdPwrLimtHEBW160 = &prCmd->u.rChPwrLimtHEBW160[k];
			ucCentCh = prCmdPwrLimtHEBW160->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeHEBW160; i++) {

				WLAN_GET_FIELD_BE16(
					&prPwrLmtConfHEBW160[i]
					.aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
					  prAdapter,
					  BAND_NULL,
					  prPwrLmtConfHEBW160[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_NULL,
					prPwrLmtConfHEBW160[i].i2CentralCh,
					ucCentCh) == FALSE)
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
					&prPwrLmtConfHEBW160[i].aucPwrLimit[0],
					eType);
			}
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType == PWR_LIMIT_TYPE_COMP_11BE_1 ||
					eType == PWR_LIMIT_TYPE_COMP_11BE_2) {

			if (k >= MAX_CMD_EHT_SUPPORT_CHANNEL_NUM) {
				DBGLOG(RLM, ERROR, "EHT of MAX CH num");
				return;
			}

			prCmdPwrLimtEHT = &prCmd->u.rChPwrLimtEHT[k];
			ucCentCh = prCmdPwrLimtEHT->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeEHT; i++) {

				WLAN_GET_FIELD_BE16(
					&prPwrLmtConfEHT[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
					prPwrLmtConfEHT[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_NULL,
					prPwrLmtConfEHT[i].i2CentralCh,
					ucCentCh) == FALSE)
					continue;

				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtEHT->cPwrLimitEHT26L,
					&prPwrLmtConfEHT[i].aucPwrLimit[0],
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
					&prPwrLmtConf6E[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(prAdapter,
				    BAND_6G,
					prPwrLmtConf6E[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_6G,
					prPwrLmtConf6E[i].i2CentralCh,
					ucCentCh) == FALSE)
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
					&prPwrLmtConf6E[i].aucPwrLimit[0],
					eType);
			}
		} else if (eType >= PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 &&
				eType <=  PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3) {
			prCmdPwrLimtLegacy_6G =
				&prCmd->u.rChPwrLimtLegacy_6G[k];
			ucCentCh = prCmdPwrLimtLegacy_6G->ucCentralCh;
			for (i = 0; i < ucPwrLmitConfSizeLegacy_6G; i++) {

				WLAN_GET_FIELD_BE16(
					&prPwrLmtConfLegacy_6G[i]
					.aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(
					prAdapter,
					BAND_6G,
					prPwrLmtConfLegacy_6G[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_6G,
					prPwrLmtConfLegacy_6G[i].i2CentralCh,
					ucCentCh) == FALSE)
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
					&prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L,
					&prPwrLmtConfLegacy_6G[i]
					.aucPwrLimit[0],
					eType);
			}
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (eType >=
					PWR_LIMIT_TYPE_COMP_11BE_6G_1 &&
					eType <=
					PWR_LIMIT_TYPE_COMP_11BE_6G_6) {

			if (k >= MAX_CMD_EHT_6G_SUPPORT_CHANNEL_NUM) {
				DBGLOG(RLM, ERROR, "EHT6G out of MAX CH num");
				return;
			}

			prCmdPwrLimtEHT_6G = &prCmd->u.rChPwrLimtEHT_6G[k];
			ucCentCh = prCmdPwrLimtEHT_6G->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSizeEHT_6G; i++) {

				WLAN_GET_FIELD_BE16(
					&prPwrLmtConfEHT_6G[i]
					.aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
				    rlmDomainCheckChannelEntryValid(prAdapter,
				    BAND_6G,
					prPwrLmtConfEHT_6G[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_6G,
					prPwrLmtConfEHT_6G[i].i2CentralCh,
					ucCentCh) == FALSE)
					continue;

				rlmDomainCompareFromConfigTable(
					&prCmdPwrLimtEHT_6G->cPwrLimitEHT26L,
					&prPwrLmtConfEHT_6G[i].aucPwrLimit[0],
					eType);
			}
		}
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */
		else if (
			eType == PWR_LIMIT_TYPE_COMP_11AC_V2
			) {
			prCmdPwrLimit = &prCmd->u.rChannelPowerLimit[k];
			ucCentCh = prCmdPwrLimit->ucCentralCh;

			for (i = 0; i < ucPwrLmitConfSize; i++) {

				WLAN_GET_FIELD_BE16(
					&prPwrLmtConf[i].aucCountryCode[0],
					&u2CountryCodeTable);

				fgChannelValid =
					rlmDomainCheckChannelEntryValid(
						prAdapter,
						BAND_NULL,
						prPwrLmtConf[i].i2CentralCh);

				if (u2CountryCodeTable == COUNTRY_CODE_NULL)
					break;	/*end of configuration table */
				else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prCmd->u2CountryCode,
					fgChannelValid,
					BAND_NULL,
					prPwrLmtConf[i].i2CentralCh,
					ucCentCh) == FALSE)
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
					&prCmdPwrLimit->cPwrLimitCCK_L,
					&prPwrLmtConf[i].aucPwrLimit[0],
					eType);
			}
		}
	}
}
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

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
		prCmd[band_idx]->countryCode =
			rlmDomainGetCountryCode(prAdapter);

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
			prTempCmd->countryCode =
				rlmDomainGetCountryCode(prAdapter);
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
			rlmDomainGetCountryCode(prAdapter);

		prTempCmd->eLimitType = prCmd->eLimitType;

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
		prCmd[band_idx]->u4CountryCode =
			rlmDomainGetCountryCode(prAdapter);

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

		if ((band_idx != KAL_BAND_2GHZ) && (band_idx != KAL_BAND_5GHZ))
			continue;

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
				rlmDomainGetCountryCode(prAdapter);
			prTempCmd->eLimitType = prCmd[band_idx]->eLimitType;
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
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData,
	enum ENUM_TX_POWER_LIMIT_PER_RATE_CMD_FORMAT_T eLimitType
)
{
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_PER_RATE
		*prTxPwrLimitPerRateCmd_6G;


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
	prTxPwrLimitPerRateCmd_6G->eBand = 0x3;  /* replace 0x3 with macro */
	prTxPwrLimitPerRateCmd_6G->u4CountryCode =
		rlmDomainGetCountryCode(prAdapter);
	prTxPwrLimitPerRateCmd_6G->eLimitType = eLimitType;

	for (ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
	prTxPwrLimitPerRateCmd_6G->rChannelPowerLimit[ch_idx].u1CentralCh =
	prChannelList[ch_idx];
	}

	/* fill to whole cmd buffer */
	rlmDomainTxPwrLimitPerRateSetValues(ucVersion,
		prTxPwrLimitPerRateCmd_6G,
		pTxPwrLimitData);

	/* separate single cmd to several cmds due to max cmd size  */
	rlmDomainTxPwrLimitSendPerRateCmd_6G(prAdapter,
		prTxPwrLimitPerRateCmd_6G);

	kalMemFree(prTxPwrLimitPerRateCmd_6G, VIR_MEM_TYPE,
		u4SetCmdTableMaxSize);

}

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
void rlmDomainTxPwrLimitVlpSetChValues(
	uint8_t ucVersion,
	struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_VLP *pCmd,
	struct CHANNEL_TX_PWR_LIMIT *pChTxPwrLimit)
{
	uint16_t section = 0, count = 0;

	for (section = 0; section < TX_PWR_LIMIT_SECTION_NUM; section++) {
		if (rlmDomainTxPwrLimitIsTxBfBackoffSection(ucVersion, section))
			continue;
		pCmd->i1PwrLimit[count] =
			pChTxPwrLimit->rTxPwrLimitValue[section][0];
		count++;
	}

	DBGLOG(RLM, TRACE, "ch %d\n", pCmd->u1CentralCh);
	count = 0;
	for (section = 0; section < TX_PWR_LIMIT_SECTION_NUM; section++) {
		struct TX_PWR_LIMIT_SECTION *pSection =
			&gTx_Pwr_Limit_Section[ucVersion];
		if (rlmDomainTxPwrLimitIsTxBfBackoffSection(ucVersion, section))
			continue;
		DBGLOG(RLM, TRACE, "TxPwrLimit[%s]= %d\n",
			pSection->arSectionNames[section],
			pCmd->i1PwrLimit[count]);
		count++;
	}
}

void rlmDomainTxPwrLimitVlpSetValues(
	uint8_t ucVersion,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP *pSetCmd,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimit)
{
	uint8_t ucIdx = 0;
	int8_t cChIdx = 0;
	struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_VLP *pChPwrLimit = NULL;
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
		rlmDomainTxPwrLimitVlpSetChValues(ucVersion,
			pChPwrLimit, pChTxPwrLimit);
	}
}

void rlmDomainTxPwrLimitSendVlpCmd(
	struct ADAPTER *prAdapter,
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP *prCmd
)
{
	uint32_t rStatus;
	uint32_t u4SetQueryInfoLen;
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_VLP);
	const uint8_t ucCmdBatchSize =
		prAdapter->chip_info->ucTxPwrLimitBatchSize;

	uint8_t ucRemainChNum, i, ucTempChNum, prCmdBatchNum;
	uint32_t u4BufSize = 0;
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP
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
			rlmDomainGetCountryCode(prAdapter);

		prTempCmd->eLimitType = prCmd->eLimitType;

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

void
rlmDomainSendTxPwrLimitVlpCmd(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData,
	enum ENUM_TX_POWER_LIMIT_PER_RATE_CMD_FORMAT_T eLimitType
	)
{
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP
		*prTxPwrLimitVlpCmd;


	uint32_t u4SetCmdTableMaxSize = 0;
	uint32_t u4SetCountryTxPwrLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_VLP);
	uint32_t u4ChPwrLimitSize =
		sizeof(struct CMD_TXPOWER_CHANNEL_POWER_LIMIT_VLP);
	uint8_t ch_cnt = TX_PWR_LIMIT_6G_CH_NUM;
	uint8_t ch_idx = 0;
	const int8_t *prChannelList = &gTx_Pwr_Limit_6g_Ch[0];

	u4SetCmdTableMaxSize = u4SetCountryTxPwrLimitCmdSize +
	ch_cnt * u4ChPwrLimitSize;

	prTxPwrLimitVlpCmd =
	kalMemAlloc(u4SetCmdTableMaxSize, VIR_MEM_TYPE);

	if (!prTxPwrLimitVlpCmd) {
		DBGLOG(RLM, ERROR,
		"%s no buf to send cmd\n", __func__);
		return;
	}

	/*initialize tx pwr table*/
	kalMemSet(prTxPwrLimitVlpCmd->rChannelPowerLimit, MAX_TX_POWER,
		ch_cnt * u4ChPwrLimitSize);

	prTxPwrLimitVlpCmd->ucNum = ch_cnt;
	prTxPwrLimitVlpCmd->eBand = 0x3;  /* replace 0x3 with macro */
	prTxPwrLimitVlpCmd->u4CountryCode = rlmDomainGetCountryCode(prAdapter);
	prTxPwrLimitVlpCmd->eLimitType = eLimitType;

	for (ch_idx = 0; ch_idx < ch_cnt; ch_idx++) {
		prTxPwrLimitVlpCmd->rChannelPowerLimit[ch_idx].u1CentralCh =
		prChannelList[ch_idx];
	}

	/* fill to whole cmd buffer */
	rlmDomainTxPwrLimitVlpSetValues(ucVersion,
		prTxPwrLimitVlpCmd,
		pTxPwrLimitData);

	/* separate single cmd to several cmds due to max cmd size  */
	rlmDomainTxPwrLimitSendVlpCmd(prAdapter,
		prTxPwrLimitVlpCmd);

	kalMemFree(prTxPwrLimitVlpCmd, VIR_MEM_TYPE,
		u4SetCmdTableMaxSize);

}

uint32_t rlmDomainUpdatePwrLimit_6G_By_PowerMode(struct ADAPTER *prAdapter,
	enum ENUM_PWR_MODE_6G_TYPE ePwrMode6G)
{
	uint32_t u4Ret = WLAN_STATUS_SUCCESS;
	uint8_t ucVersion = 0;
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData = NULL;
	char *prTxPwrLimit6GFile = NULL;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	pTxPwrLimitData = rlmDomainInitTxPwrLimitData_6G(prAdapter);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Init TxPwrLimitData 6G failed\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto error;
	}

	/* Update Tx power limit */
	if (ePwrMode6G == PWR_MODE_6G_SP)
		prTxPwrLimit6GFile =
		prAdapter->chip_info->prTxPwrLimit6GSpFile;
	else if (ePwrMode6G == PWR_MODE_6G_LPI)
		prTxPwrLimit6GFile =
		prAdapter->chip_info->prTxPwrLimit6GFile;
	else {
		DBGLOG(RLM, ERROR,
			"Invalid power mode for this API\n");
		u4Ret = WLAN_STATUS_INVALID_DATA;
		goto error;
	}

	if (!rlmDomainGetTxPwrLimit(
		prTxPwrLimit6GFile,
		rlmDomainGetCountryCode(prAdapter),
		&ucVersion,
		prAdapter->prGlueInfo,
		pTxPwrLimitData)) {
		DBGLOG(RLM, ERROR,
			"Load TxPwrLimitData6G failed\n");
		u4Ret = WLAN_STATUS_FAILURE;
		goto error;
	}

	/* Update Power table to FW */
	if (ucVersion == 2 || ucVersion == 3) {
		rlmDomainSendTxPwrLimitPerRateCmd_6G(prAdapter,
			ucVersion, pTxPwrLimitData,
			TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU);
	} else
		DBGLOG(RLM, ERROR,
		"Unsupported TxPwrLimit6G dat file version %u\n",
		ucVersion);

error:
	/* Finish and free memory */
	if (pTxPwrLimitData && pTxPwrLimitData->rChannelTxPwrLimit)
		kalMemFree(pTxPwrLimitData->rChannelTxPwrLimit,
			VIR_MEM_TYPE,
			sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			pTxPwrLimitData->ucChNum);

	if (pTxPwrLimitData)
		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));

	return u4Ret;
}
#endif  /*CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1*/
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
void rlmSendTpeLimit(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucPwrLmtNum,
	int8_t *pcTxPwrEnvMaxPwr,
	uint8_t fgPwrLmtEnable)
{
	uint32_t rStatus = WLAN_STATUS_FAILURE;
	uint32_t u4SetQueryInfoLen = 0, u4BufSize = 0;
	uint32_t u4SetTxPwrEnvLimitCmdSize =
		sizeof(struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_ENV);
	struct CMD_SET_TXPOWER_COUNTRY_TX_POWER_LIMIT_ENV *prTempCmd =
		NULL;
	enum TX_PWR_ENV_MAX_TXPWR_BW_TYPE eBwType = TX_PWR_ENV_MAX_TXPWR_BW_NUM;
	int8_t acAntGain[TX_PWR_ANT_GAIN_ELEMENT_NUM] = {0};

	if ((!prAdapter) || (!pcTxPwrEnvMaxPwr) || (!prAdapter->fgTpcEn))
		return;

	u4BufSize = u4SetTxPwrEnvLimitCmdSize;
	prTempCmd = cnmMemAlloc(prAdapter,
			RAM_TYPE_BUF, u4BufSize);

	if (!prTempCmd) {
		DBGLOG(RLM, ERROR,
		"[%s] [RLM] Domain: no buf to send cmd\n", __func__);
		goto err;
	}

	kalMemZero(prTempCmd, u4BufSize);
	kalMemSet(prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt,
		MAX_TX_POWER,
		sizeof(prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt));

	/*copy partial tx pwr limit*/
	prTempCmd->ucNum = 1;
	prTempCmd->eBand = (uint8_t)eBand;
	prTempCmd->eLimitType = TXPWR_LIMIT_PER_RATE_CMD_FORMAT_TPE;
	prTempCmd->bCmdFinished = true;
	prTempCmd->rTxPowerEnvLimit.fgPwrLmtEnable = fgPwrLmtEnable;
	prTempCmd->rTxPowerEnvLimit.ucBand = (uint8_t)eBand;
	prTempCmd->rTxPowerEnvLimit.ucPriCh = ucPriCh;
	prTempCmd->rTxPowerEnvLimit.ucPwrLmtNum = ucPwrLmtNum;
	prTempCmd->u2CmdLen = (uint16_t)u4BufSize;
	u4SetQueryInfoLen = u4BufSize;

	// Get Antenna Gain value
	rlmDomainAntGainGetGroupGain(ucPriCh, eBand, acAntGain);

	/* Fill in CMD content */
	for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < ucPwrLmtNum; eBwType++) {
		prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt[ANTENNA_WF0][eBwType]
			= pcTxPwrEnvMaxPwr[eBwType] - acAntGain[ANTENNA_WF0];
		prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt[ANTENNA_WF1][eBwType]
			= pcTxPwrEnvMaxPwr[eBwType] - acAntGain[ANTENNA_WF1];
	}

	DBGLOG(RLM, INFO, "[%d]Send_TPE, En[%u]Band[%u]Ch[%u]Num[%u]\n",
		__LINE__,
		prTempCmd->rTxPowerEnvLimit.fgPwrLmtEnable,
		prTempCmd->rTxPowerEnvLimit.ucBand,
		prTempCmd->rTxPowerEnvLimit.ucPriCh,
		prTempCmd->rTxPowerEnvLimit.ucPwrLmtNum);

	for (eBwType = TX_PWR_ENV_MAX_TXPWR_BW20;
			eBwType < TX_PWR_ENV_MAX_TXPWR_BW_NUM; eBwType++) {
		DBGLOG(RLM, INFO,
		"[%d][RLM]TPE_Bw[%u]Lmt[%d]Pwr0[%d]Pwr1[%d]Ant0[%d]Ant1[%d]\n",
		__LINE__,
		eBwType,
		pcTxPwrEnvMaxPwr[eBwType],
		prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt[ANTENNA_WF0][eBwType],
		prTempCmd->rTxPowerEnvLimit.acMaxTxPwrLmt[ANTENNA_WF1][eBwType],
		acAntGain[ANTENNA_WF0],
		acAntGain[ANTENNA_WF1]);
	}

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
err:
	cnmMemFree(prAdapter, prTempCmd);
}

u_int8_t rlmDomainAntGainLoadSetting(
	uint8_t *pucBuf, uint32_t *pu4Pos, uint32_t u4BufEnd, int BandIdx,
	struct TX_PWR_ANT_GAIN_DATA *pTxPwrAntGain)
{
	uint32_t u4TmpPos = *pu4Pos;
	char cTmpChar = 0;
	struct TX_PWR_ANT_GAIN_GROUP *prGroupAntGain = NULL;
	u_int8_t bNeg = FALSE;
	int8_t cLimitValue = 0;
	uint8_t ucIdx = 0, cGroupIdx = 0, ucGroup = 0, ucSectionOffset = 0;
	uint8_t ucElementNum = TX_PWR_ANT_GAIN_ELEMENT_NUM;

	/* skip blank lines */
	while (u4TmpPos < u4BufEnd) {
		cTmpChar = (char)pucBuf[u4TmpPos];

		if (cTmpChar == ' ' || cTmpChar == '\t' ||
			cTmpChar == '\n' || cTmpChar == '\r') {
			u4TmpPos++;
			continue;
		}

		break;
	}

	/* current is at the location of 'g',
	 * check remaining buf length for 'group'
	 */
	if (u4TmpPos + 7 >= u4BufEnd) {
		DBGLOG(RLM, ERROR,
			"Invalid location of group setting: %u/%u\n",
			u4TmpPos, u4BufEnd);
		return FALSE;
	}

	if ((pucBuf[u4TmpPos] == 'g') && (pucBuf[u4TmpPos+1] == 'r') &&
		(pucBuf[u4TmpPos+2] == 'o') && (pucBuf[u4TmpPos+3] == 'u') &&
		(pucBuf[u4TmpPos+4] == 'p')) {
		/* group1 to group9 */
		if (pucBuf[u4TmpPos+6] == ',') {
			ucGroup = (pucBuf[u4TmpPos+5] - '0');
			ucSectionOffset = 6;
		} else if (pucBuf[u4TmpPos+7] == ',') {
			/* group10 to group22 */
			ucGroup = (pucBuf[u4TmpPos+5] - '0') * 10 +
						(pucBuf[u4TmpPos + 6] - '0');
			ucSectionOffset = 7;
		} else {
			/* Invalid format */
			return FALSE;
		}

		DBGLOG(RLM, INFO,
			"[%s] [RLM] ucGroup[%u] pucBuf[%c %c]\n",
			__func__,
			ucGroup,
			pucBuf[u4TmpPos+5],
			pucBuf[u4TmpPos+6]);
	} else {
		/* invalid format */
		*pu4Pos = u4TmpPos;
		DBGLOG(RLM, ERROR,
			"[%s] [RLM] Invalid subband setting[%c %c]\n",
			__func__, pucBuf[u4TmpPos], pucBuf[u4TmpPos + 1]);

		/* goto next line */
		while (*pu4Pos < u4BufEnd && pucBuf[*pu4Pos] != '\n')
			(*pu4Pos)++;

		return TRUE;
	}

	if ((ucGroup == 0) ||
		(ucGroup > TX_PWR_ANT_GAIN_GROUP_NUM)) {
		*pu4Pos = u4TmpPos;

		DBGLOG(RLM, ERROR,
			"[%s] [RLM] Invalid subband setting %u\n",
			__func__,
			ucGroup);

		/* goto next line */
		while (*pu4Pos < u4BufEnd && pucBuf[*pu4Pos] != '\n')
			(*pu4Pos)++;

		return TRUE;
	}

	cGroupIdx = (ucGroup - 1);

	// Overwrite group number and get total group
	pTxPwrAntGain->ucGroupNum = ucGroup;

	// Get group array and set group number
	prGroupAntGain = &pTxPwrAntGain->rAntGainGroup[cGroupIdx];
	prGroupAntGain->ucGroup = cGroupIdx;

	u4TmpPos += ucSectionOffset;

	/* read the group antenna gain settings */
	for (ucIdx = 0; ucIdx < ucElementNum; ucIdx++) {

		/* skip blank and comma */
		while (u4TmpPos < u4BufEnd) {
			cTmpChar = (char)pucBuf[u4TmpPos];

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
				"Invalid location of ant_gain val: %u/%u\n",
				u4TmpPos, u4BufEnd);
			return FALSE;
		}

		bNeg = FALSE;

		cTmpChar = (char)pucBuf[u4TmpPos];

		if (cTmpChar == '-') {
			bNeg = TRUE;
			u4TmpPos++;
		} else {
			if ((cTmpChar == 'x') ||
				(cTmpChar == 'X')) {
				prGroupAntGain
				  ->rTxPwrAntGainValue[BandIdx][ucIdx] = 0;
				u4TmpPos++;
				continue;
			}
		}

		cLimitValue = 0;
		while (u4TmpPos < u4BufEnd) {
			cTmpChar = (char)pucBuf[u4TmpPos];

			if (cTmpChar < '0' || cTmpChar > '9')
				break;

			cLimitValue = (cLimitValue * 10) + (cTmpChar - '0');
			u4TmpPos++;
		}

		if (bNeg)
			cLimitValue = -cLimitValue;

		prGroupAntGain->rTxPwrAntGainValue[BandIdx][ucIdx] =
			cLimitValue;

		DBGLOG(RLM, ERROR,
			"ucIdx[%u] AntGain[%d] cLimitValue[%d]\n",
			ucIdx,
			prGroupAntGain->rTxPwrAntGainValue[BandIdx][ucIdx],
			cLimitValue);
	}

	*pu4Pos = u4TmpPos;
	return TRUE;
}

u_int8_t rlmDomainAntGainLoad(
	uint8_t *prFileName,
	uint8_t *pucBuf,
	uint32_t u4BufLen,
	struct TX_PWR_ANT_GAIN_DATA *pTxPwrAntGainData)
{
	uint32_t u4Pos = 0;
	enum ENUM_BAND eBand = BAND_NULL;
	int Idx = 0;
	const struct TX_PWR_ANT_GAIN_SECTION *prSection =
		&gTx_Pwr_Ant_Gain_Section;

	if ((!prFileName) || (!pucBuf) || (!pTxPwrAntGainData)) {
		DBGLOG(RLM, ERROR, "[%d]Invalid parameter\n", __LINE__);
		return FALSE;
	}

	for (eBand = BAND_2G4; eBand < BAND_NUM ; eBand++) {
		Idx = eBand - 1;
		if (!rlmDomainTxPwrLimitSearchSection(
			prSection->arSectionNames[Idx],
			pucBuf, &u4Pos, u4BufLen)) {
			DBGLOG(RLM, ERROR,
				"Can't find specified section %s in %s\n",
				prSection->arSectionNames,
				prFileName);
			return FALSE;
		}

		DBGLOG(RLM, INFO, "Find specified section %s in %s\n",
			prSection->arSectionNames[Idx],
			prFileName);

		while (!rlmDomainTxPwrLimitSectionEnd(pucBuf,
			prSection->arSectionNames[Idx],
			&u4Pos, u4BufLen) &&
			u4Pos < u4BufLen) {
			if (!rlmDomainAntGainLoadSetting(pucBuf,
				&u4Pos, u4BufLen, Idx, pTxPwrAntGainData))
				return FALSE;
		}
	}

	return true;

}

bool rlmDomainAntGainInit(
	struct ADAPTER *prAdapter)
{
	bool bRet = FALSE;
	uint8_t arFileName[] = "TxPwrLimit_AntGain.dat";
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;
	struct TX_PWR_ANT_GAIN_DATA *pTxPwrAntGainData = NULL;
	int i = 0, BandIdx = BAND_NULL;
	struct TX_PWR_ANT_GAIN_GROUP *prAntGainGroup = NULL;

	// Load AntGain file to configBuf
	bRet = rlmDomainTxPwrLimitLoadFromFile(arFileName,
		prAdapter, &pucConfigBuf, &u4ConfigReadLen);

	if (bRet == FALSE)
		goto error;

	// Load Antenna gain table from configBuf to AntGainData
	pTxPwrAntGainData = &g_TxPwrAntGainData;
	pTxPwrAntGainData->rAntGainGroup = &g_TxPwrAntGainGroupData[0];
	if (!rlmDomainAntGainLoad(arFileName,
		pucConfigBuf, u4ConfigReadLen,
		pTxPwrAntGainData)) {
		bRet = FALSE;
		goto error;
	}

	DBGLOG(RLM, INFO, "[%d][RLM]Ant_TotalGroup:%u, Read:%u\n",
		__LINE__,
		pTxPwrAntGainData->ucGroupNum,
		u4ConfigReadLen);

	for (i = 0; i < TX_PWR_ANT_GAIN_GROUP_NUM; i++) {
		for (BandIdx = BAND_2G4; BandIdx < BAND_NUM; BandIdx++) {
			prAntGainGroup = &pTxPwrAntGainData->rAntGainGroup[i];
			DBGLOG(RLM, INFO, "[RLM]Ant_Group[%u]=[%d][%d]\n",
			prAntGainGroup->ucGroup,
			prAntGainGroup->rTxPwrAntGainValue[BandIdx - 1][0],
			prAntGainGroup->rTxPwrAntGainValue[BandIdx - 1][1]);
		}
	}

error:
	if (pucConfigBuf)
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);

	return bRet;
}

uint32_t rlmDomainAntGetBandGrp(
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t *pu1BandGrp)
{
	if (!pu1BandGrp)  {
		DBGLOG(RLM, ERROR, "[%d][RLM]Invalid data\n", __LINE__);
		return WLAN_STATUS_INVALID_DATA;
	}

	switch (eBand) {
	case BAND_2G4:
		*pu1BandGrp = POWER_ANT_GAIN_GROUP_1;
		break;

	case BAND_5G:
	{
		if (ucPriCh <= POWER_LIMIT_A_BAND_CH_GROUP_1_END)
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_1;
		else if ((ucPriCh >= POWER_LIMIT_A_BAND_CH_GROUP_2_START)
			&& (ucPriCh <= POWER_LIMIT_A_BAND_CH_GROUP_2_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_2;
		else if ((ucPriCh >= POWER_LIMIT_A_BAND_CH_GROUP_3_START)
			&& (ucPriCh <= POWER_LIMIT_A_BAND_CH_GROUP_3_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_3;
		else if (ucPriCh >= POWER_LIMIT_A_BAND_CH_GROUP_4_START)
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_4;
		else
			return WLAN_STATUS_NOT_SUPPORTED;
		break;
	}

	case BAND_6G:
	{
		if (ucPriCh <= POWER_LIMIT_6G_BAND_CH_GROUP_1_END)
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_1;
		else if ((ucPriCh >= POWER_LIMIT_6G_BAND_CH_GROUP_2_START)
			&& (ucPriCh <= POWER_LIMIT_6G_BAND_CH_GROUP_2_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_2;
		else if ((ucPriCh >= POWER_LIMIT_6G_BAND_CH_GROUP_3_START)
			&& (ucPriCh <= POWER_LIMIT_6G_BAND_CH_GROUP_3_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_3;
		else if ((ucPriCh >= POWER_LIMIT_6G_BAND_CH_GROUP_4_START)
			&& (ucPriCh <= POWER_LIMIT_6G_BAND_CH_GROUP_4_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_4;
		else if ((ucPriCh >= POWER_LIMIT_6G_BAND_CH_GROUP_5_START)
			&& (ucPriCh <= POWER_LIMIT_6G_BAND_CH_GROUP_5_END))
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_5;
		else if (ucPriCh >= POWER_LIMIT_6G_BAND_CH_GROUP_6_START)
			*pu1BandGrp = POWER_ANT_GAIN_GROUP_6;
		else
			return WLAN_STATUS_NOT_SUPPORTED;
		break;
	}

	default:
		return WLAN_STATUS_NOT_SUPPORTED;
	}
	return WLAN_STATUS_SUCCESS;
}

uint32_t rlmDomainAntGainGetGroupGain(
	uint8_t ucPriCh,
	enum ENUM_BAND eBand,
	int8_t *acAntGain)
{
	struct TX_PWR_ANT_GAIN_GROUP *prAntGainGroup = NULL;
	int8_t AntIdx = 0, BandIdx = eBand - 1;
	uint8_t u1BandGrp = POWER_ANT_GAIN_GROUP_1;

	if (!acAntGain)  {
		DBGLOG(RLM, ERROR, "[%d][RLM]Invalid data\n", __LINE__);
		return WLAN_STATUS_INVALID_DATA;
	}

	rlmDomainAntGetBandGrp(eBand, ucPriCh, &u1BandGrp);
	if (u1BandGrp >= POWER_ANT_GAIN_GROUP_NUM)  {
		DBGLOG(RLM, ERROR,
			"[%d][RLM]Invalid data, u1BandGrp[%d]\n",
			__LINE__, u1BandGrp);
		return WLAN_STATUS_INVALID_DATA;
	}

	// Initial 0 for invalid group / not found case
	kalMemSet(&acAntGain[0], 0, TX_PWR_ANT_GAIN_ELEMENT_NUM);

	prAntGainGroup = &g_TxPwrAntGainGroupData[u1BandGrp];
	for (AntIdx = ANTENNA_WF0; AntIdx < TX_PWR_ANT_GAIN_ELEMENT_NUM;
		AntIdx++)
		acAntGain[AntIdx] =
		prAntGainGroup->rTxPwrAntGainValue[BandIdx][AntIdx];

	DBGLOG(RLM, INFO,
		"[%d][RLM]GroupNum[%d]BandIdx[%d]Ant0[%d]Ant1[%d]\n",
		__LINE__,
		u1BandGrp,
		BandIdx,
		acAntGain[ANTENNA_WF0],
		acAntGain[ANTENNA_WF1]);

	return WLAN_STATUS_SUCCESS;
}
#endif  /*CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1*/

void
rlmDomainSendTxPwrLimitPerRateCmd(struct ADAPTER *prAdapter,
	uint8_t ucVersion,
	struct TX_PWR_LIMIT_DATA *pTxPwrLimitData,
	enum ENUM_TX_POWER_LIMIT_PER_RATE_CMD_FORMAT_T eLimitType)
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

	prTxPwrLimitPerRateCmd[KAL_BAND_2GHZ]->eLimitType = eLimitType;
	prTxPwrLimitPerRateCmd[KAL_BAND_5GHZ]->eLimitType = eLimitType;

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
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;

	DBGLOG(RLM, INFO, "rlmDomainSendPwrLimitCmd()\n");

	if (prAdapter->chip_info->prTxPwrLimitFile == NULL) {
		DBGLOG(RLM, ERROR, "prTxPwrLimitFile is NULL\n");
		goto error;
	}

	pTxPwrLimitData = rlmDomainInitTxPwrLimitData(prAdapter);

	if (!pTxPwrLimitData) {
		DBGLOG(RLM, ERROR,
			"Init TxPwrLimitData failed\n");
		goto error;
	}

	/* Get Max Tx Power from MT_TxPwrLimit.dat */
	if (!rlmDomainGetTxPwrLimit(
		prAdapter->chip_info->prTxPwrLimitFile,
		rlmDomainGetCountryCode(prAdapter),
		&ucVersion,
		prGlueInfo,
		pTxPwrLimitData)) {
		DBGLOG(RLM, ERROR,
			"Load TxPwrLimitData failed\n");
		goto error;
	}

	/* Prepare to send CMD to FW */
	if (ucVersion == 0) {
		rlmDomainSendTxPwrLimitCmd(prAdapter,
			ucVersion, pTxPwrLimitData);
	} else if (ucVersion == 1 || ucVersion == 2 || ucVersion == 3) {

		rlmDomainSendTxPwrLimitPerRateCmd(prAdapter,
			ucVersion, pTxPwrLimitData,
			TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU);

		if (prGlueInfo->bTxBfBackoffExists)
			rlmDomainSendTxBfBackoffCmd(prAdapter,
				ucVersion, pTxPwrLimitData);

	} else {
		DBGLOG(RLM, WARN, "Unsupported TxPwrLimit dat version %u\n",
			ucVersion);
	}

#if (CFG_SUPPORT_POWER_SKU_ENHANCE == 1)
	/* Get Max Tx Power from MT_TxPwrLimit_1ss1t.dat */
	if (prAdapter->chip_info->prTxPwrLimit1ss1tFile == NULL)
		DBGLOG(RLM, ERROR, "prTxPwrLimit1ss1tFile is NULL\n");

	if (prAdapter->chip_info->prTxPwrLimit1ss1tFile) {
		if (!rlmDomainGetTxPwrLimit(
			prAdapter->chip_info->prTxPwrLimit1ss1tFile,
			rlmDomainGetCountryCode(prAdapter),
			&ucVersion,
			prAdapter->prGlueInfo,
			pTxPwrLimitData)) {
			DBGLOG(RLM, ERROR,
				"Load %s failed\n",
				prAdapter->chip_info->prTxPwrLimit1ss1tFile);
			goto error;
		}

		/* Prepare to send CMD to FW */
		if (ucVersion == 0) {
			rlmDomainSendTxPwrLimitCmd(prAdapter,
			ucVersion, pTxPwrLimitData);
		} else if (ucVersion == 1 || ucVersion == 2 || ucVersion == 3) {
			rlmDomainSendTxPwrLimitPerRateCmd(prAdapter,
			ucVersion, pTxPwrLimitData,
			TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU_1SS_1T);
		} else {
			DBGLOG(RLM, WARN,
			"Unsupported %s version %u\n",
			prAdapter->chip_info->prTxPwrLimit1ss1tFile,
			ucVersion);
		}
	}
#endif /* #if (CFG_SUPPORT_POWER_SKU_ENHANCE == 1) */

#if (CFG_SUPPORT_SINGLE_SKU_6G == 1)
#if (CFG_SUPPORT_WIFI_6G == 1)
	if (prAdapter->fgIsHwSupport6G == FALSE) {
		DBGLOG(RLM, ERROR,
			"fgIsHwSupport6G is False, skip 6G SKU process\n");
		goto error;
	}
#endif

	if (prAdapter->chip_info->prTxPwrLimit6GFile == NULL)
		DBGLOG(RLM, ERROR, "prTxPwrLimit6GFile is NULL\n");

	if (prAdapter->chip_info->prTxPwrLimit6GFile) {
		/* TODO: check if buffer allocation can be replaced by MEMSET */
		if (pTxPwrLimitData && pTxPwrLimitData->rChannelTxPwrLimit)
			kalMemFree(pTxPwrLimitData->rChannelTxPwrLimit,
				VIR_MEM_TYPE,
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

		if (!rlmDomainGetTxPwrLimit(
			prAdapter->chip_info->prTxPwrLimit6GFile,
			rlmDomainGetCountryCode(prAdapter),
			&ucVersion,
			prAdapter->prGlueInfo,
			pTxPwrLimitData)) {
			DBGLOG(RLM, ERROR,
				"Load TxPwrLimit6GFile failed\n");
			goto error;
		}

		/* Prepare to send CMD to FW */
		if (ucVersion == 2  || ucVersion == 3) {
			rlmDomainSendTxPwrLimitPerRateCmd_6G(prAdapter,
				ucVersion, pTxPwrLimitData,
				TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU);
		} else {
			DBGLOG(RLM, WARN,
			"Unsupported %s version %u\n",
			prAdapter->chip_info->prTxPwrLimit6GFile,
			ucVersion);
		}
	}

#if (CFG_SUPPORT_SINGLE_SKU_6G_1SS1T == 1)
	if (prAdapter->chip_info->prTxPwrLimit6G1ss1tFile == NULL)
		DBGLOG(RLM, ERROR, "prTxPwrLimit6G1ss1tFile is NULL\n");

	if (prAdapter->chip_info->prTxPwrLimit6G1ss1tFile) {
		if (!rlmDomainGetTxPwrLimit(
			prAdapter->chip_info->prTxPwrLimit6G1ss1tFile,
			rlmDomainGetCountryCode(prAdapter),
			&ucVersion,
			prAdapter->prGlueInfo,
			pTxPwrLimitData)) {
			DBGLOG(RLM, ERROR,
				"Load %s failed\n",
				prAdapter->chip_info->prTxPwrLimit6G1ss1tFile);
			goto error;
		}

		/* Prepare to send CMD to FW */
		if (ucVersion == 2 || ucVersion == 3) {
			rlmDomainSendTxPwrLimitPerRateCmd_6G(prAdapter,
				ucVersion, pTxPwrLimitData,
				TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU_1SS_1T);
		} else {
			DBGLOG(RLM, WARN,
			"Unsupported %s version %u\n",
			prAdapter->chip_info->prTxPwrLimit6G1ss1tFile,
			ucVersion);
		}
	}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G_1SS1T == 1) */
#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	if (prAdapter->chip_info->prTxPwrLimit6GSpFile) {
		if (!rlmDomainGetTxPwrLimit(
			prAdapter->chip_info->prTxPwrLimit6GSpFile,
			rlmDomainGetCountryCode(prAdapter),
			&ucVersion,
			prAdapter->prGlueInfo,
			pTxPwrLimitData)) {
			prAdapter->fg6GSupSpFile = FALSE;
			DBGLOG(RLM, ERROR,
				"Load %s failed\n",
				prAdapter->chip_info->prTxPwrLimit6GSpFile);
		} else {
			prAdapter->fg6GSupSpFile = TRUE;
		}
	}

	if (prAdapter->chip_info->prTxPwrLimit6GVlpFile == NULL)
		DBGLOG(RLM, ERROR, "prTxPwrLimit6GVlpFile is NULL\n");

	if (prAdapter->chip_info->prTxPwrLimit6GVlpFile) {
		if (!rlmDomainGetTxPwrLimit(
			prAdapter->chip_info->prTxPwrLimit6GVlpFile,
			rlmDomainGetCountryCode(prAdapter),
			&ucVersion,
			prAdapter->prGlueInfo,
			pTxPwrLimitData)) {
			prAdapter->fg6GSupVlpFile = FALSE;
			DBGLOG(RLM, ERROR,
				"Load %s failed\n",
				prAdapter->chip_info->prTxPwrLimit6GVlpFile);
			goto error;
		}

		 prAdapter->fg6GSupVlpFile = TRUE;
		/* Prepare to send CMD to FW */
		if (ucVersion == 2 || ucVersion == 3) {
			rlmDomainSendTxPwrLimitVlpCmd(prAdapter,
				ucVersion, pTxPwrLimitData,
				TXPWR_LIMIT_PER_RATE_CMD_FORMAT_CH_SKU_VLP);
		} else {
			DBGLOG(RLM, WARN,
			"Unsupported %s version %u\n",
			prAdapter->chip_info->prTxPwrLimit6GVlpFile,
			ucVersion);
		}
	}
#endif  /*CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1*/
#endif /* #if (CFG_SUPPORT_SINGLE_SKU_6G == 1) */

error:
	if (pTxPwrLimitData && pTxPwrLimitData->rChannelTxPwrLimit)
		kalMemFree(pTxPwrLimitData->rChannelTxPwrLimit, VIR_MEM_TYPE,
			sizeof(struct CHANNEL_TX_PWR_LIMIT) *
			pTxPwrLimitData->ucChNum);

	if (pTxPwrLimitData) {
		kalMemFree(pTxPwrLimitData, VIR_MEM_TYPE,
			sizeof(struct TX_PWR_LIMIT_DATA));
	}
#endif /* #if (CFG_SUPPORT_SINGLE_SKU == 1) */
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
			  uint16_t *value) {
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
		if (kalkStrtou16(pcTmp, 0, value) != 0) {
			DBGLOG(RLM, ERROR,
			       "parse number error: invalid number [%s][%s]\n",
			       pcTmp, delim);
			return -1;
		}
		if (fgIsNegtive)
			*op = 2;
		else
			*op = 1;
	}

	return 0;
}


void txPwrParseTagDump(struct TX_PWR_CTRL_ELEMENT *pRecord)
{
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	uint32_t i = 0, j = 0, k = 0;
	struct TX_PWR_CTRL_ANT_SETTING *prPwrAnt;

	for (k = 0; k < PWR_LMT_ANT_EXT_NUM; k++) {
		for (i = 0; i < POWER_ANT_TAG_NUM; i++) {
			DBGLOG(RLM, TRACE, "Tag id (%d) ext(%d) :", i, k);
			prPwrAnt = &pRecord->aiPwrAnt[i];
			for (j = 0; j < PWR_LMT_CHAIN_ANT_NUM; j++) {
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt2G4[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt5GB1[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt5GB2[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt5GB3[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt5GB4[j][k]);
#if (CFG_SUPPORT_WIFI_6G == 1)
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt6GB1[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt6GB2[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt6GB3[j][k]);
				DBGLOG(RLM, TRACE, "[%d]",
					prPwrAnt->aiPwrAnt6GB4[j][k]);
#endif
			}
		}
	}
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */
}

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
int32_t txPwrParseAntCfgParaBand(
	char *pContent,
	uint8_t *pucNum,
	uint8_t *pucStart,
	uint8_t *pucEnd)
{
	uint8_t ucIdx = 0;
	uint8_t ucSize = 0;

	if (!pContent || !pucNum || !pucStart || !pucEnd)
		return WLAN_STATUS_INVALID_DATA;

	ucSize = sizeof(g_auTxPwrAntBandCfgTbl) /
			sizeof(struct TX_PWR_ANT_CFG_PARA_TABLE);

	for (ucIdx = 0; ucIdx < ucSize; ucIdx++) {
		if (kalStrCmp(pContent,
			g_auTxPwrAntBandCfgTbl[ucIdx].arKeywords) == 0) {
			/* found */
			break;
		}
	}

	if (ucIdx >= ucSize) {
		DBGLOG(RLM, ERROR, "Undefine Band keyword [%s]\n", pContent);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	*pucNum = g_auTxPwrAntBandCfgTbl[ucIdx].ucSettingNum;
	*pucStart = g_auTxPwrAntBandCfgTbl[ucIdx].ucStart;
	*pucEnd = g_auTxPwrAntBandCfgTbl[ucIdx].ucEnd;

	if (*pucNum > PWR_LMT_CHAIN_BAND_NUM ||
		*pucStart > PWR_LMT_CHAIN_BAND_NUM ||
		*pucEnd > PWR_LMT_CHAIN_BAND_NUM) {
		DBGLOG(RLM, ERROR,
			"Invalid Band parameter num[%d]start[%d]end[%d]: %s\n",
			*pucNum,
			*pucStart,
			*pucEnd,
			pContent);

		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
};

int32_t txPwrParseAntCfgParaChain(
	char *pContent,
	uint8_t *pucNum,
	uint8_t *pucStart,
	uint8_t *pucEnd)
{
	uint8_t ucIdx = 0;
	uint8_t ucSize = 0;

	if (!pContent || !pucNum || !pucStart || !pucEnd)
		return WLAN_STATUS_INVALID_DATA;

	ucSize = sizeof(g_auTxPwrAntChainCfgTbl) /
			sizeof(struct TX_PWR_ANT_CFG_PARA_TABLE);

	for (ucIdx = 0; ucIdx < ucSize; ucIdx++) {
		if (kalStrCmp(pContent,
			g_auTxPwrAntChainCfgTbl[ucIdx].arKeywords) == 0) {
			/* found */
			break;
		}
	}

	if (ucIdx >= ucSize) {
		DBGLOG(RLM, ERROR, "Undefine Chain keyword [%s]\n", pContent);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	*pucNum = g_auTxPwrAntChainCfgTbl[ucIdx].ucSettingNum;
	*pucStart = g_auTxPwrAntChainCfgTbl[ucIdx].ucStart;
	*pucEnd = g_auTxPwrAntChainCfgTbl[ucIdx].ucEnd;

	if (*pucNum > PWR_LMT_CHAIN_ANT_NUM ||
		*pucStart > PWR_LMT_CHAIN_ANT_NUM ||
		*pucEnd > PWR_LMT_CHAIN_ANT_NUM) {
		DBGLOG(RLM, ERROR,
		"Invalid Chain parameter num[%d]start[%d]end[%d]: %s\n",
			*pucNum,
			*pucStart,
			*pucEnd,
			pContent);

		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t txPwrGetAntTagInitVal(
	uint8_t ucTag,
	int8_t *picInitVal)
{
	uint8_t ucIdx = 0;
	uint8_t ucSize = 0;

	if (!picInitVal)
		return WLAN_STATUS_INVALID_DATA;

	ucSize = sizeof(g_auTxPwrTagTable) /
			sizeof(struct TX_PWR_TAG_TABLE);

	for (ucIdx = 0; ucIdx < ucSize; ucIdx++) {
		if (g_auTxPwrTagTable[ucIdx].ucTagIdx == ucTag &&
			g_auTxPwrTagTable[ucIdx].ucCategory
				== POWER_TAG_CATEGORY_ANT) {
			/* found */
			break;
		}
	}
	if (ucIdx >= ucSize) {
		DBGLOG(RLM, ERROR, "Undefine Ant PwrLmt tag[%d]\n", ucIdx);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	*picInitVal = g_auTxPwrTagTable[ucIdx].icInitVal;

	/* Sanity check TxPower boundary */
	if (*picInitVal > MAX_TX_POWER || *picInitVal < MIN_TX_POWER) {
		DBGLOG(RLM, ERROR,
			"Ant tag[%d],invalid PwrLmt init value[%d]\n",
			ucTag,
			*picInitVal);

		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
}

uint32_t txPwrStoreSettingToList(
	struct TX_PWR_CTRL_ELEMENT *pRecord,
	enum ENUM_POWER_ANT_TAG eTag,
	uint8_t ucBandIdx,
	uint8_t ucAntIdx,
	int8_t icPwrSetting,
	enum ENUM_PWR_LMT_ANT_EXT eAntExt)
{

	if (!pRecord ||
		eTag >= POWER_ANT_TAG_NUM ||
		ucBandIdx >= PWR_LMT_CHAIN_BAND_NUM ||
		ucAntIdx >= PWR_LMT_CHAIN_ANT_NUM) {
		DBGLOG(RLM, ERROR, "Invalid setting:Tag[%d]Band[%d]Ant[%d]\n",
			eTag,
			ucBandIdx,
			ucAntIdx);
		return WLAN_STATUS_INVALID_DATA;
	}

	switch (ucBandIdx) {
	case PWR_LMT_CHAIN_2G4_BAND:
		pRecord->aiPwrAnt[eTag].aiPwrAnt2G4[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_5G_BAND1:
		pRecord->aiPwrAnt[eTag].aiPwrAnt5GB1[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_5G_BAND2:
		pRecord->aiPwrAnt[eTag].aiPwrAnt5GB2[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_5G_BAND3:
		pRecord->aiPwrAnt[eTag].aiPwrAnt5GB3[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_5G_BAND4:
		pRecord->aiPwrAnt[eTag].aiPwrAnt5GB4[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
#if (CFG_SUPPORT_WIFI_6G)
	case PWR_LMT_CHAIN_6G_BAND1:
		pRecord->aiPwrAnt[eTag].aiPwrAnt6GB1[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_6G_BAND2:
		pRecord->aiPwrAnt[eTag].aiPwrAnt6GB2[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_6G_BAND3:
		pRecord->aiPwrAnt[eTag].aiPwrAnt6GB3[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
	case PWR_LMT_CHAIN_6G_BAND4:
		pRecord->aiPwrAnt[eTag].aiPwrAnt6GB4[ucAntIdx][eAntExt] =
			icPwrSetting;
		break;
#endif
	default:
		DBGLOG(RLM, WARN, "Band index non support:Band[%d]\n",
		ucBandIdx);
		return WLAN_STATUS_NOT_SUPPORTED;
	}

	return WLAN_STATUS_SUCCESS;
}

int32_t txPwrGetAntCfgType(
	uint8_t ucChainNum,
	uint8_t ucBandNum,
	enum ENUM_PWR_LMT_CHAIN_CFG_TYPE  *peType
)
{
	if (!peType)
		return -1;

	if ((ucChainNum == ANT_CFG_CHAIN_NUM_SINGLE) &&
			(ucBandNum == ANT_CFG_SUBBAND_NUM_SINGLE)) {
		/* Single config element for chain &
		 * Single config element for subband
		 */
		*peType = PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_SGL_BAND;
	} else if ((ucChainNum == ANT_CFG_CHAIN_NUM_SINGLE) &&
			(ucBandNum != ANT_CFG_SUBBAND_NUM_SINGLE)) {
		/* Single config element for chain &
		 * Multiple config element for subband
		 */
		*peType = PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_MULTI_BAND;
	} else if ((ucChainNum != ANT_CFG_CHAIN_NUM_SINGLE) &&
			(ucBandNum == ANT_CFG_SUBBAND_NUM_SINGLE)) {
		/* Multiple config element for chain &
		 * Single config element for subband
		 */
		*peType = PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_SGL_BAND;
	} else if ((ucChainNum != ANT_CFG_CHAIN_NUM_SINGLE) &&
			(ucBandNum != ANT_CFG_SUBBAND_NUM_SINGLE)) {
		/* Multiple config element for chain &
		 * Multiple config element for subband
		 */
		*peType = PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_MULTI_BAND;
	} else {
		DBGLOG(RLM, WARN,
			"Non support Ant cfg type chain_num[%d]Band_num[%d]\n",
			ucChainNum,
			ucBandNum);
		return -1;
	}

	DBGLOG(RLM, TRACE, "Ant Cfg type[%d]\n", *peType);

	return 0;
}

int32_t txPwrParseAntCfgParaPwr(
	char *pcCurrent,
	struct TX_PWR_CTRL_ELEMENT *pRecord,
	enum ENUM_POWER_ANT_TAG eTag,
	enum ENUM_PWR_LMT_CHAIN_CFG_TYPE eCfgType,
	uint8_t ucChainNum,
	uint8_t ucBandNum,
	uint8_t ucTotalNum,
	uint8_t ucChainStart,
	uint8_t ucChainEnd,
	uint8_t ucBandStart,
	uint8_t ucBandEnd,
	enum ENUM_PWR_LMT_ANT_EXT eExt)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t op = 0;
	uint16_t value = 0;
	uint8_t ucCnt = 0;
	uint8_t ucBandIdx = 0;
	uint8_t ucAntIdx = 0;
	int8_t icInitVal = 0;
	int8_t icPwrSetting = 0;
	char *pcContent = NULL;

	if (!pcCurrent)
		return -1;

	DBGLOG(RLM, TRACE, "parse Pwr Para (%s) to aiPwrAnt[%u]",
		pcCurrent, eTag);

	u4Status = txPwrGetAntTagInitVal(eTag, &icInitVal);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(RLM, ERROR,
			"Get PwrLmt Ant init value fail,Tag[%d]Status[0x%x]\n",
			eTag,
			u4Status);
		return -1;
	}

	/* Parse Power config and store to list */
	for (ucCnt = 0; ucCnt < ucTotalNum; ucCnt++) {

		pcContent = txPwrGetString(&pcCurrent, ",");

		if (pcContent) {
			if (txPwrParseNumber(&pcContent, ",", &op,
							&value)) {
				DBGLOG(RLM, ERROR, "parse parameter error:%s\n",
				pcContent);
				break;
			}
			icPwrSetting = (op == 1) ? value : (0 - value);
		} else {
			DBGLOG(RLM, TRACE,
			"Without setting, using init val[%d]\n", icInitVal);
			icPwrSetting = icInitVal;
		}

		switch (eCfgType) {
		case PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_SGL_BAND:
			for (ucAntIdx = ucChainStart;
				ucAntIdx <= ucChainEnd; ucAntIdx++) {
				for (ucBandIdx = ucBandStart;
					ucBandIdx <= ucBandEnd; ucBandIdx++) {

					if (txPwrStoreSettingToList(
						pRecord, eTag, ucBandIdx,
						ucAntIdx, icPwrSetting,
						eExt)
						!= WLAN_STATUS_SUCCESS)
						return -1;
				}
			}
			break;
		case PWR_LMT_CHAIN_CFG_TYPE_SGL_WF_MULTI_BAND:
			for (ucAntIdx = ucChainStart;
				ucAntIdx <= ucChainEnd; ucAntIdx++) {

				ucBandIdx = (ucCnt % ucBandNum) + ucBandStart;

				if (txPwrStoreSettingToList(pRecord, eTag,
					ucBandIdx, ucAntIdx, icPwrSetting,
					eExt)
					!= WLAN_STATUS_SUCCESS)
					return -1;
			}
			break;
		case PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_SGL_BAND:
			for (ucBandIdx = ucBandStart;
				ucBandIdx <= ucBandEnd; ucBandIdx++) {

				ucAntIdx = (ucCnt % ucChainNum) + ucChainStart;

				if (txPwrStoreSettingToList(pRecord, eTag,
					ucBandIdx, ucAntIdx, icPwrSetting,
					eExt)
					!= WLAN_STATUS_SUCCESS)
					return -1;
			}
			break;
		case PWR_LMT_CHAIN_CFG_TYPE_MULTI_WF_MULTI_BAND:
			/* (WF0_2G, WF0_5G1, WF0_5G2, ..., WF0_6G4,
			* WF1_2G, WF1_5G1, ...  WFx_2G, WFx5G1,
			* ..., WFx_6G4)
			* The arrangement criteria is set WF0 for all
			* subband setting, and set WF1 for all subband
			* setting and so on.
			*/
			ucBandIdx = ucCnt % ucBandNum + ucBandStart;
			ucAntIdx = ucCnt / ucBandNum;

			if (txPwrStoreSettingToList(pRecord, eTag, ucBandIdx,
				ucAntIdx, icPwrSetting,
				eExt) != WLAN_STATUS_SUCCESS)
				return -1;
			break;
		default:
			DBGLOG(RLM, ERROR,
				"Non-support cfg type[%d]\n", eCfgType);
			break;
		}
	}

	DBGLOG(RLM, INFO, "[Success] Dump aiPwrAnt[%u] para: ", eTag);

	for (ucAntIdx = 0; ucAntIdx < PWR_LMT_CHAIN_ANT_NUM; ucAntIdx++) {
		DBGLOG(RLM, INFO, "ext:%d-[%d][%d][%d][%d][%d][%d][%d][%d][%d]",
			eExt,
			pRecord->aiPwrAnt[eTag].aiPwrAnt2G4[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB1[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB2[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB3[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt5GB4[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB1[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB2[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB3[ucAntIdx][eExt],
			pRecord->aiPwrAnt[eTag].aiPwrAnt6GB4[ucAntIdx][eExt]);
		DBGLOG(RLM, INFO, "\n");
	}

	return 0;
}
int32_t txPwrParseTagChainCfg(
	char *pcStart, char *pcEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord,
	enum ENUM_POWER_ANT_TAG eTag)
{
	char *pcCurrent = NULL;
	char *pcContent = NULL;
	char *pcContTmp = NULL;
	uint8_t ucTokenLen = 0;
	uint8_t ucCnt = 0, i = 0;
	uint8_t ucChainNum = 0, ucChainStart = 0, ucChainEnd = 0;
	uint8_t ucBandNum = 0, ucBandStart = 0, ucBandEnd = 0;
	uint8_t ucTotalNum = 0;
	enum ENUM_PWR_LMT_CHAIN_CFG_TYPE eCfgType;
	enum ENUM_PWR_LMT_ANT_EXT eAntExt = PWR_LMT_ANT_EXT_MAIN;

	if (!pcStart || !pcEnd || !pRecord || pcCurrent >= pcEnd)
		return -1;

	pcCurrent = pcStart;

	/* Parsing ANT_EXT */
	pcContent = txPwrGetString(&pcCurrent, ",");
	if (!pcContent) {
		DBGLOG(RLM, INFO,
			   "Pase Ext error, %s\n",
			   pcStart);
		return -1;
	}
	ucTokenLen = kalStrLen(pcContent);

	for (i = 0; i < ARRAY_SIZE(g_arTxPwrAntExtOptionTbl); i++) {
		DBGLOG(RLM, TRACE,
			"Parse ant ext [%s] [%s]\n", pcContent,
			g_arTxPwrAntExtOptionTbl[i].arKeywords);
		if (kalStrCmp(pcContent,
			g_arTxPwrAntExtOptionTbl[i].arKeywords) == 0) {
			eAntExt = g_arTxPwrAntExtOptionTbl[i].eAntExt;
			break;
		}
	}

	if (i == ARRAY_SIZE(g_arTxPwrAntExtOptionTbl)) {
		DBGLOG(RLM, INFO, "ant ext keep default\n");
		/* roll back pointer */
		pcCurrent = pcCurrent - 1;
		*pcCurrent = ',';
		pcCurrent = pcCurrent - ucTokenLen;
		eAntExt = PWR_LMT_ANT_EXT_MAIN;
	}


	DBGLOG(RLM, TRACE, "parse tag Para (%s) to aiPwrAnt[%u] ext[%d]",
		pcCurrent, eTag, eAntExt);

	/* Parsing chain_key */
	pcContent = txPwrGetString(&pcCurrent, ",");

	if (!pcContent || !pcCurrent) {
		DBGLOG(RLM, ERROR, "Parse ANT_NUM error: %s\n",
			pcStart);
		return -1;
	}

	if (txPwrParseAntCfgParaChain(pcContent, &ucChainNum, &ucChainStart,
		&ucChainEnd) != WLAN_STATUS_SUCCESS) {
		DBGLOG(RLM, ERROR, "Parse parameter chain num error: %s\n",
			pcStart);
		 return -1;
	}

	DBGLOG(RLM, TRACE, "Parse ChainNum[%d]Start[%d]End[%d]\n",
				ucChainNum,
				ucChainStart,
				ucChainEnd);

	/* Parsing band_key */
	pcContent = txPwrGetString(&pcCurrent, ",");

	if (!pcContent || !pcCurrent) {
		DBGLOG(RLM, ERROR, "Parse BAND_NUM error: %s\n",
			pcStart);
		return -1;
	}

	if (txPwrParseAntCfgParaBand(pcContent, &ucBandNum, &ucBandStart,
		&ucBandEnd) != WLAN_STATUS_SUCCESS) {
		DBGLOG(RLM, ERROR, "Parse parameter band num error: %s\n",
			pcStart);
		 return -1;
	}

	DBGLOG(RLM, TRACE, "Parse BandNum[%d]Start[%d]End[%d]\n",
				ucBandNum,
				ucBandStart,
				ucBandEnd);

	/* The number of element should be config */
	ucTotalNum = ucChainNum * ucBandNum;

	pcContTmp = pcCurrent;

	/* first cfg value is before the delimter */
	ucCnt = 1;
	/* Calculate config element in this segment*/
	while (pcContTmp < pcEnd) {
		if (*pcContTmp == ',')
			ucCnt++;

		pcContTmp++;
	}

	if (ucCnt != ucTotalNum) {
		DBGLOG(RLM, ERROR,
		"cfg num error:ChainNum[%d]BandNum[%d]Expect[%d]Cfg[%d]\n",
				ucChainNum,
				ucBandNum,
				ucTotalNum,
				ucCnt);
		return -1;
	}

	if (txPwrGetAntCfgType(ucChainNum, ucBandNum, &eCfgType))
		return -1;

	if (txPwrParseAntCfgParaPwr(pcCurrent, pRecord, eTag, eCfgType,
				ucChainNum, ucBandNum, ucTotalNum,
				ucChainStart, ucChainEnd,
				ucBandStart, ucBandEnd, eAntExt))
		return -1;

	return 0;
}

int32_t txPwrParseTagChainComp(
	char *pcStart, char *pcEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord) {

	return txPwrParseTagChainCfg(pcStart, pcEnd,
		cTagParaNum, pRecord, POWER_ANT_CHAIN_COMP);

}

int32_t txPwrParseTagChainAbs(
	char *pcStart, char *pcEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord) {

	return txPwrParseTagChainCfg(pcStart, pcEnd,
		cTagParaNum, pRecord, POWER_ANT_CHAIN_ABS);

}

static int32_t txPwrOnPreParseAppendAntTag(
	struct TX_PWR_CTRL_ELEMENT *pRecord)
{
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint8_t ucTagIdx = 0;
	int8_t icInitVal = 0;

	if (!pRecord)
		return -1;

	for (ucTagIdx = 0; ucTagIdx < POWER_ANT_TAG_NUM; ucTagIdx++) {

		u4Status = txPwrGetAntTagInitVal(ucTagIdx, &icInitVal);
		if (u4Status != WLAN_STATUS_SUCCESS) {
			DBGLOG(RLM, ERROR,
			"Get PwrLmt Ant init value fail,Tag[%d]Status[0x%x]\n",
			ucTagIdx,
			u4Status);
			return -1;
		}

		kalMemSet(&(pRecord->aiPwrAnt[ucTagIdx]),
			icInitVal,
			sizeof(struct TX_PWR_CTRL_ANT_SETTING));

		DBGLOG(RLM, TRACE,
			"PwrLmt Ant init tag[%d]val[%d]\n",
			ucTagIdx,
			icInitVal);

	}
	return 0;
}
#endif

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
int32_t txPwrBandIdxSearch(uint8_t u1BandMask)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(g_rBandMaskTbl); i++) {
		if (u1BandMask == g_rBandMaskTbl[i].u1BandMask)
			return g_rBandMaskTbl[i].idx;
	}
	/* miss */
	return PWR_LIMIT_MULTIBAND_TYPE_NUM;
}

int32_t txPwrBandMaskSearch(uint8_t u1BandIdx)
{
	uint8_t i = 0;

	for (i = 0; i < ARRAY_SIZE(g_rBandMaskTbl); i++)
		if (u1BandIdx == g_rBandMaskTbl[i].idx)
			return g_rBandMaskTbl[i].u1BandMask;
	/* miss */
	return 0;
}

int32_t txPwrParseTagMultiBand(
	char *pcStart, char *pcEnd, uint8_t cTagParaNum,
	struct TX_PWR_CTRL_ELEMENT *pRecord)
{
	char *pcCurrent = NULL;
	char *pcContOld = NULL;
	char *pcContTemp = NULL;
	char carySeperator[2] = {0, 0};
	uint8_t i = 0, j = 0, k = 0, cnt = 0, u1BandIdx = 0;
	uint8_t op = 0;
	uint16_t u2MultiBandNum = 0, u2ChCnt;
	uint16_t u2RfBand = 0, u2StartCh = 0, u2EndCh = 0, u2BandMask = 0;
	uint16_t u2Pwr = 0;
	struct TX_PWR_CTRL_MULTIBAND_SETTING *prMulBnSetting;
	char msgLimit[PWR_BUF_LEN];
	uint32_t u4MsgOfs = 0;
	struct TX_PWR_CTRL_MULTIBAND_PWR *prPwr;

	if (!pcStart || !pcEnd || !pRecord) {
		DBGLOG(RLM, INFO, "[MulBnPwr] invalid pointer\n");
		return -1;
	}

	pcCurrent = pcStart;

	/* Parsing MultiBandNum */
	pcContOld = pcCurrent;
	if (txPwrParseNumber(&pcCurrent, ",", &op,
				(uint16_t *)&u2MultiBandNum)) {
		DBGLOG(RLM, ERROR, "[MulBnPwr] parse parameter error:%s\n",
			pcContOld);
		return -1;
	}

	if (u2MultiBandNum > PWR_LIMIT_MULTIBAND_TYPE_NUM) {
		DBGLOG(RLM, ERROR, "[MulBnPwr] MultiBandNum error:%d>%d\n",
			u2MultiBandNum,
			PWR_LIMIT_MULTIBAND_TYPE_NUM);
		return -1;
	}


	for (i = 0; i < u2MultiBandNum; i++) {
		/* check BandMask & Channel Group Num*/
		pcContTemp = pcCurrent;
		cnt = 0;
		while (pcContTemp <= pcEnd) {
			if ((*pcContTemp) == ':')
				break;
			else if ((*pcContTemp) == ',')
				cnt++;

			pcContTemp++;
		}
		if (cnt != 1)
			return -1;

		pcContOld = pcCurrent;
		/* Parsing BandMask */
		if (txPwrParseNumber(&pcCurrent, ",", &op,
						(uint16_t *)&u2BandMask)) {
			DBGLOG(RLM, ERROR,
				"[MulBnPwr] parse parameter error:%s\n",
				pcContOld);
			return -1;
		}

		u1BandIdx = txPwrBandIdxSearch(u2BandMask);
		if (u1BandIdx >= PWR_LIMIT_MULTIBAND_TYPE_NUM)
			return -1;
		prMulBnSetting = &pRecord->rMultiBandCfg[u1BandIdx];

		/* Parsing Channel Group Num */
		pcContOld = pcCurrent;
		if (txPwrParseNumber(&pcCurrent, ":", &op,
						(uint16_t *)&u2ChCnt)) {
			DBGLOG(RLM, ERROR,
				"[MulBnPwr] parse parameter error:%s\n",
				pcContOld);
			return -1;
		}
		if (u2ChCnt >= MAX_SUPPORT_CHANNEL_NUMBER) {
			DBGLOG(RLM, ERROR,
				"[MulBnPwr] Warning ChGrpCnt more than max: %d\n",
				u2ChCnt,
				MAX_SUPPORT_CHANNEL_NUMBER);
			return -1;
		}

		prMulBnSetting->u1ChGrpCnt = u2ChCnt;

		for (j = 0; j < u2ChCnt; j++) {
			/* check rfband, start_ch and end_ch*/
			pcContTemp = pcCurrent;
			cnt = 0;
			while (pcContTemp <= pcEnd) {
				if ((*pcContTemp) == '(')
					break;
				else if ((*pcContTemp) == ',')
					cnt++;

				pcContTemp++;
			}
			if (cnt != 2) {
				DBGLOG(RLM, ERROR,
					"[MulBnPwr] parsing channel definition error\n");
				return -1;
			}

			/* Parsing RF Band */
			pcContOld = pcCurrent;
			if (txPwrParseNumber(&pcCurrent, ",",
				&op, (uint16_t *)&u2RfBand)) {
				DBGLOG(RLM, ERROR,
					"[MulBnPwr] parse parameter error:%s\n",
					pcContOld);
				return -1;
			}
			if (u2RfBand >= PWR_LIMIT_RF_BAND_NUM)
				return -1;

			/* Parsing start ch */
			pcContOld = pcCurrent;
			if (txPwrParseNumber(&pcCurrent, ",",
				&op, (uint16_t *)&u2StartCh)) {
				DBGLOG(RLM, ERROR,
					"[MulBnPwr] parse parameter error:%s\n",
					pcContOld);
				return -1;
			}

			/* Parsing end ch */
			pcContOld = pcCurrent;
			if (txPwrParseNumber(&pcCurrent, "(",
				&op, (uint16_t *)&u2EndCh)) {
				DBGLOG(RLM, ERROR,
					"[MulBnPwr] parse parameter error:%s\n",
					pcContOld);
				return -1;
			}
			if (u2StartCh > u2EndCh)
				return -1;

			prPwr = &prMulBnSetting->rMultiBandPwr[j];
			prPwr->u1RfBand = u2RfBand;
			prPwr->u1Startch = u2StartCh;
			prPwr->u1Endch = u2EndCh;

			/* check power limit setting cnt */
			pcContTemp = pcCurrent;
			cnt = 0;
			while (pcContTemp <= pcEnd) {
				if ((*pcContTemp) == ')')
					break;
				else if ((*pcContTemp) == ',')
					cnt++;

				pcContTemp++;
			}
			/* here is backward compatible code */
			if (cnt != (PWR_LIMIT_MULTIBAND_NUM - 1)) {
				DBGLOG(RLM, ERROR,
					"[MulBnPwr] powewr limit setting cnt error\n");
				return -1;
			}

			u4MsgOfs = 0;
			for (k = 0; k < (cnt + 1); k++) {
				/* Parsing power limit */
				if (k == cnt) /* last one*/
					carySeperator[0] = ')';
				else
					carySeperator[0] = ',';

				pcContOld = pcCurrent;

				if (txPwrParseNumber(&pcCurrent, carySeperator,
					&op, (uint16_t *)&u2Pwr)) {
					DBGLOG(RLM, ERROR,
						"[MulBnPwr] [%d-%s]parse parameter error:%s\n",
						cnt, carySeperator, pcContOld);
					return -1;
				}
				prPwr->Op[k] = op;
				prPwr->i1MBPwrLmt[k] =
					(int8_t)(op != 2) ? u2Pwr : (0 - u2Pwr);


				/*message head*/
				u4MsgOfs +=
					snprintf(
						msgLimit + u4MsgOfs,
						PWR_BUF_LEN - u4MsgOfs,
						"[%d/%d],",
						prPwr->Op[k],
						prPwr->i1MBPwrLmt[k]
					);
			}

			/*message tail*/
			if (u4MsgOfs >= 1)
				msgLimit[u4MsgOfs - 1] = '\0';
			else
				msgLimit[0] = '\0';

			DBGLOG(RLM, TRACE,
				"[MulBnPwr] [BN:%d,Mask:%d,ChGrp:%d,RF:%d,Ch:%d-%d,%s\n",
				u1BandIdx,
				u2BandMask,
				j,
				prMulBnSetting->rMultiBandPwr[j].u1RfBand,
				prMulBnSetting->rMultiBandPwr[j].u1Startch,
				prMulBnSetting->rMultiBandPwr[j].u1Endch,
				msgLimit);

			kalMemZero(msgLimit, PWR_BUF_LEN);
		}
	}

	return 0;
}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

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

	for (i = 0; i < ARRAY_SIZE(g_auTxPwrTagTable); i++) {

		DBGLOG(RLM, TRACE,
				"Parse tag name [%s] handler name[%s]\n", pNext,
				g_auTxPwrTagTable[i].arTagNames);

		if (kalStrCmp(pNext,
			g_auTxPwrTagTable[i].arTagNames) == 0) {

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

static int32_t txPwrOnPreParseAppendMultiBandTag(
	struct TX_PWR_CTRL_ELEMENT *pRecord)
{
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	uint8_t i = 0;

	if (!pRecord)
		return -1;

	for (i = 0; i < PWR_LIMIT_MULTIBAND_TYPE_NUM; i++)
		pRecord->rMultiBandCfg[i].u1ChGrpCnt = 0;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	return 0;
}

static int32_t txPwrOnPreParseAppendTag(
	struct TX_PWR_CTRL_ELEMENT *pRecord)
{
	bool ret = 0;

	if (!pRecord)
		return -1;

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	/* initialize of Power ctrl with antenna tag */
	if (txPwrOnPreParseAppendAntTag(pRecord))
		ret = -1;
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */

	/* initialize of Power ctrl with multi band tag */
	if (txPwrOnPreParseAppendMultiBandTag(pRecord))
		ret = -1;

	return ret;
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

	DBGLOG(RLM, INFO, "New config total %u tag append %s !\n",
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

}

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
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
		prRateOfs = &prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L;
		u1Idx = PWR_LIMIT_CCK_L;
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
		prRateOfs = &prPwrLimit->cPwrLimitCCK_L;
		u1Idx = PWR_LIMIT_CCK_L;
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
uint8_t txPwrIsAntTagSet(
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	enum ENUM_POWER_ANT_TAG tag) {

	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	struct TX_PWR_CTRL_ANT_SETTING *paiPwrAnt;
	enum ENUM_PWR_LMT_ANT_EXT eExt = PWR_LMT_ANT_EXT_MAIN;
	uint8_t ucTagIdx = 0;
	int8_t cInit = 0;
	uint8_t ucAnt = 0;

	if (tag >= POWER_ANT_TAG_NUM)
		return 0;

	u4Status = txPwrGetAntTagInitVal(tag, &cInit);

	if (u4Status != WLAN_STATUS_SUCCESS) {
		DBGLOG(RLM, ERROR,
			"Get PwrLmt Ant init value fail,Tag[%d]Status[0x%x]\n",
			ucTagIdx,
			u4Status);
		return 0;
	}

	paiPwrAnt = &prCurElement->aiPwrAnt[tag];
	for (eExt = 0; eExt < PWR_LMT_ANT_EXT_NUM; eExt++) {
		for (ucAnt = 0; ucAnt < PWR_LMT_CHAIN_ANT_NUM; ucAnt++) {
			if (paiPwrAnt->aiPwrAnt2G4[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt5GB1[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt5GB2[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt5GB3[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt5GB4[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt6GB1[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt6GB2[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt6GB3[ucAnt][eExt] != cInit)
				return 1;
			if (paiPwrAnt->aiPwrAnt6GB4[ucAnt][eExt] != cInit)
				return 1;
		}
	}
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

uint8_t txPwrAntChainMapping(
	enum ENUM_PWR_LMT_ANT_EXT eAntExt,
	uint8_t u1AntIdx
)
{
	uint8_t retval = 0;

	if (eAntExt >= PWR_LMT_ANT_EXT_NUM)
		retval = u1AntIdx;
	else {
		retval = g_arTxPwrAntExtOptionTbl[eAntExt].u4OptionBit +
			u1AntIdx;
	}

	return retval;
}

uint8_t txPwrAntTagMapping(
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandIdx
)
{
	enum ENUM_POWER_ANT_TAG eMappingTag;

	eMappingTag = tag;
#if (CFG_SUPPORT_CHAIN_COMP_ARCH == 0)
	if (tag == POWER_ANT_CHAIN_COMP) {
		if (u1BandIdx <= PWR_LMT_CHAIN_5G_BAND4)
			eMappingTag = POWER_ANT_ALL_T;
		else
			eMappingTag = POWER_ANT_ALL_T_6G;
	}
#endif
	return eMappingTag;
}

uint8_t txPwrBandIdxMapping(
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandIdx
)
{
	uint8_t u1MappingBandIdx;

	u1MappingBandIdx = u1BandIdx;

#if (CFG_SUPPORT_CHAIN_COMP_ARCH == 0)
	if (tag == POWER_ANT_CHAIN_COMP) {
		if (u1BandIdx >= PWR_LMT_CHAIN_6G_BAND1 &&
			u1BandIdx <= PWR_LMT_CHAIN_6G_BAND4)
			u1MappingBandIdx = u1BandIdx - PWR_LMT_CHAIN_6G_BAND1;
		else
			u1MappingBandIdx = u1BandIdx;
	}
#endif
	return u1MappingBandIdx;
}
void txPwrApplySingleAntChainCfg(
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt,
	struct TX_PWR_CTRL_ANT_SETTING *prTxPwrAnt,
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandIdx,
	uint8_t u1AntIdx,
	enum ENUM_PWR_LMT_ANT_EXT eAntExt
)
{
	enum ENUM_TX_POWER_CTRL_TYPE eCtrlType =
		PWR_CTRL_TYPE_IOCTL_POWER_OFFSET;

	if (tag == POWER_ANT_CHAIN_ABS)
		eCtrlType = PWR_CTRL_TYPE_IOCTL_POWER_LEVEL;

	if (!prCmdPwrAnt || !prTxPwrAnt)
		return;

	prCmdPwrAnt->cTagIdx = txPwrAntTagMapping(tag, u1BandIdx);
	prCmdPwrAnt->cBandIdx = txPwrBandIdxMapping(tag, u1BandIdx);
	prCmdPwrAnt->cAntIdx = txPwrAntChainMapping(eAntExt, u1AntIdx);

	if (u1BandIdx == PWR_LMT_CHAIN_2G4_BAND) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt2G4[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_5G_BAND1) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt5GB1[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_5G_BAND2) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt5GB2[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_5G_BAND3) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt5GB3[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_5G_BAND4) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt5GB4[u1AntIdx][eAntExt]);
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (u1BandIdx == PWR_LMT_CHAIN_6G_BAND1) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt6GB1[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_6G_BAND2) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt6GB2[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_6G_BAND3) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt6GB3[u1AntIdx][eAntExt]);
	} else if (u1BandIdx == PWR_LMT_CHAIN_6G_BAND4) {
		txPwrOperate(eCtrlType,
			&prCmdPwrAnt->cValue,
			&prTxPwrAnt->aiPwrAnt6GB4[u1AntIdx][eAntExt]);
#endif
	}

}

uint32_t txPwrApplyAntChainCfg(
	uint8_t u1Idx,
	enum ENUM_POWER_ANT_TAG tag,
	uint8_t u1BandNum,
	uint8_t u1AntNum,
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt,
	struct TX_PWR_CTRL_ELEMENT *prCurElem)
{
	uint8_t u1BandIdx = 0, u1AntIdx = 0;
	enum ENUM_PWR_LMT_ANT_EXT eAntExt = 0;
	struct TX_PWR_CTRL_ANT_SETTING *prTxPwrAnt = NULL;

	if (txPwrCheckPwrAntNum(tag, u1Idx) != WLAN_STATUS_SUCCESS)
		return u1Idx;

	prTxPwrAnt = &prCurElem->aiPwrAnt[tag];

	for (eAntExt = 0; eAntExt < PWR_LMT_ANT_EXT_NUM; eAntExt++) {
		for (u1BandIdx = 0; u1BandIdx < u1BandNum; u1BandIdx++) {
			for (u1AntIdx = 0; u1AntIdx < u1AntNum; u1AntIdx++) {
				txPwrApplySingleAntChainCfg(
					&prCmdPwrAnt[u1Idx], prTxPwrAnt,
					tag, u1BandIdx, u1AntIdx, eAntExt);
				u1Idx++;
			}
		}
	}
	return u1Idx;
}

uint32_t txPwrApplyOneSettingPwrAnt(
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	struct ADAPTER *prAdapter)
{
	uint8_t u1NextIdx = 0;
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	enum ENUM_CMD_PWR_LIMIT_TYPE eType;
#else
	enum ENUM_PWR_LIMIT_TYPE eType;
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */

	if (!prCurElement || !prCmd)
		return 1;

	eType = prCmd->ucLimitType;

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	if (eType != PWR_LIMIT_CMD_TYPE_ANT_V2)
#else
	if (eType != PWR_LIMIT_TYPE_COMP_ANT_V2)
#endif /* CFG_SUPPORT_PWR_LMT_EMI == 1 */
		return 0;

	prCmd->ucNum = 0;

	u1NextIdx = txPwrApplyAntChainCfg(
		0, POWER_ANT_CHAIN_COMP,
		PWR_LMT_CHAIN_BAND_NUM, PWR_LMT_CHAIN_ANT_NUM,
		&prAdapter->rAntPwr[POWER_ANT_CACHE_CHAIN_COMP][0],
		prCurElement);

	u1NextIdx = txPwrApplyAntChainCfg(
		0, POWER_ANT_CHAIN_ABS,
		PWR_LMT_CHAIN_BAND_NUM, PWR_LMT_CHAIN_ANT_NUM,
		&prAdapter->rAntPwr[POWER_ANT_CACHE_CHAIN_ABS][0],
		prCurElement);


	return 0;
}
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG */

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
uint32_t txPwrApplyOneSetting(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd,
			      struct TX_PWR_CTRL_ELEMENT *prCurElement,
			      uint8_t *bandedgeParam,
			      struct ADAPTER *prAdapter)
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
		txPwrApplyOneSettingPwrAnt(prCmd, prCurElement, prAdapter);
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/


void txPwrCtrlAntPowerCacheInit(struct ADAPTER *prAdapter)
{
	int32_t i, j;
	int8_t initVal = 0;

	for (i = 0; i < POWER_ANT_CACHE_TAG_NUM; i++) {
		if (i == POWER_ANT_CACHE_CHAIN_ABS)
			txPwrGetAntTagInitVal(POWER_ANT_CHAIN_ABS, &initVal);
		else
			txPwrGetAntTagInitVal(POWER_ANT_CHAIN_COMP, &initVal);

		for (j = 0; j < POWER_LIMIT_ANT_CONFIG_NUM; j++)
			prAdapter->rAntPwr[i][j].cValue = initVal;
	}
}
void txPwrCtrlSetAntPowerToCmdBuf(struct ADAPTER *prAdapter,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd)
{
	int8_t i, j, initVal = 0;
	bool fgSet[POWER_ANT_CACHE_TAG_NUM];
	enum ENUM_POWER_CACHE_ANT_TAG eApplied = POWER_ANT_CACHE_CHAIN_COMP;

	struct CMD_CHANNEL_POWER_LIMIT_ANT *prCmdPwrAnt = NULL;

	prCmdPwrAnt = &(prCmd->u.rChPwrLimtAnt[0]);

	for (i = 0; i < POWER_ANT_CACHE_TAG_NUM; i++) {
		if (i == POWER_ANT_CACHE_CHAIN_ABS)
			txPwrGetAntTagInitVal(POWER_ANT_CHAIN_ABS, &initVal);
		else
			txPwrGetAntTagInitVal(POWER_ANT_CHAIN_COMP, &initVal);

		for (j = 0; j < POWER_LIMIT_ANT_CONFIG_NUM; j++) {
			if (prAdapter->rAntPwr[i][j].cValue != initVal) {
				fgSet[i] = 1;
				break;
			}
		}
	}

	prCmd->ucNum = PWR_LMT_ANT_EXT_NUM *
		PWR_LMT_CHAIN_BAND_NUM * PWR_LMT_CHAIN_ANT_NUM;

	if (fgSet[POWER_ANT_CACHE_CHAIN_COMP] == 0 &&
		fgSet[POWER_ANT_CACHE_CHAIN_ABS] == 0) {
		prCmd->ucNum = 0;
		return;
#if (CFG_SUPPORT_CHAIN_ABS_ARCH == 1)
	} else if (fgSet[POWER_ANT_CACHE_CHAIN_ABS] == 1) {
		eApplied = POWER_ANT_CACHE_CHAIN_ABS;
#endif
	} else
		eApplied = POWER_ANT_CACHE_CHAIN_COMP;

	kalMemCopy(prCmdPwrAnt,
		&prAdapter->rAntPwr[eApplied][0],
		sizeof(struct CMD_CHANNEL_POWER_LIMIT_ANT) * prCmd->ucNum);

}

uint32_t txPwrCtrlApplyAntPowerSettings(struct ADAPTER *prAdapter,
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmd)
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

	txPwrCtrlAntPowerCacheInit(prAdapter);

	for (i = 0; i < ARRAY_SIZE(aryprlist); i++) {
		LINK_FOR_EACH_SAFE(prCur, prNext, aryprlist[i]) {
			element = LINK_ENTRY(prCur,
				struct TX_PWR_CTRL_ELEMENT, node);

			if (element->fgApplied == TRUE)
				txPwrApplyOneSettingPwrAnt(prCmd, element,
					prAdapter);

		}
	}

	 txPwrCtrlSetAntPowerToCmdBuf(prAdapter, prCmd);

	return 0;
}

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
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
					prCmd, element, bandedgeParam,
						prAdapter);
		}
	}

	return 0;
}
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/

bool txPwrCfgCheckCfgValid(uint8_t count, char *pcContCur)
{
	uint32_t i = 0;
	uint32_t au4ValidNum[] = {
		PWR_CFG_PRAM_NUM_ALL_RATE,
		PWR_LIMIT_PARSER_LEGACY_NUM, /* backward campatibility*/
		PWR_LIMIT_PARSER_LEGACY_NUM + 1, /* keyword : LEGACY (V1)*/
		PWR_LIMIT_PARSER_LEGACY_NUM_V2, /* backward campatibility*/
		PWR_LIMIT_PARSER_LEGACY_NUM_V2 + 1, /* keyword : LEGACY (V2)*/
		PWR_LIMIT_PARSER_HE_NUM, /* backward campatibility*/
		PWR_LIMIT_PARSER_HE_NUM + 1, /* backward campatibility*/
		PWR_LIMIT_PARSER_HE160_NUM, /* backward campatibility*/
		PWR_LIMIT_PARSER_HE160_NUM + 1, /* keyword : HE or AX160 */
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LIMIT_PARSER_EHT_NUM + 1, /* keyword : EHT */
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
		PWR_LIMIT_PARSER_HE160_NUM + 1, /* keyword : HE6G */
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		PWR_LIMIT_PARSER_EHT6G_NUM + 1, /* keyword : EHT6G */
#endif
#endif
	};

	if (!pcContCur) {
		DBGLOG(RLM, ERROR, "pcContCur null error\n");
		return FALSE;
	}


	for (i = 0; i < ARRAY_SIZE(au4ValidNum); i++) {
		if (count == au4ValidNum[i]) {
			DBGLOG(RLM, TRACE,
				"parse num success: count:%d content:%s\n",
					count, pcContCur);
			return TRUE;
		}
	}

	DBGLOG(RLM, ERROR,
		"parse error: not segments, count:%d content:%s\n",
		count, pcContCur);

	return FALSE;
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
		count == PWR_LIMIT_PARSER_LEGACY_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_LEGACY) == 0 &&
		count == PWR_LIMIT_PARSER_LEGACY_NUM_V2)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY_V2;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_AX160) == 0 &&
		count == PWR_LIMIT_PARSER_HE160_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_HE) == 0 &&
		count == PWR_LIMIT_PARSER_HE160_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_EHT) == 0 &&
		count == PWR_LIMIT_PARSER_EHT_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_EHT;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (kalStrCmp(pcContTmp,
			PWR_CTRL_RATE_TAG_KEY_LEGACY6G) == 0 &&
		count == PWR_LIMIT_PARSER_LEGACY_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY6G;
	else if (kalStrCmp(pcContTmp,
			PWR_CTRL_RATE_TAG_KEY_LEGACY6G) == 0 &&
		count == PWR_LIMIT_PARSER_LEGACY_NUM_V2)
		u1Result = PWR_CFG_RATE_TAG_HIT_LEGACY6G_V2;
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_HE6G) == 0 &&
		count == PWR_LIMIT_PARSER_HE160_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_HE6G;
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	else if (kalStrCmp(pcContTmp, PWR_CTRL_RATE_TAG_KEY_EHT6G) == 0 &&
		count == PWR_LIMIT_PARSER_EHT6G_NUM)
		u1Result = PWR_CFG_RATE_TAG_HIT_EHT6G;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G  */
	else
		u1Result = PWR_CFG_RATE_TAG_MISS;

	DBGLOG(RLM, TRACE, "Parse rate tag result[%d]\n", u1Result);
	return u1Result;
}
static void txPwrCtrlSetAllRatePwrLimit(
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChnlSet,
	uint8_t op,
	uint8_t value
)
{

	int8_t power = 0;
	uint8_t i = 0;

	if (op == 2)
		power = 0 - value;
	else
		power = value;

	/* Legacy */
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimit); i++) {
		prChnlSet->op[i] = op;
		prChnlSet->i8PwrLimit[i] = power;
	}
	/* HE */
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimitHE); i++) {
		prChnlSet->opHE[i] = op;
		prChnlSet->i8PwrLimitHE[i] = power;
	}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/* EHT */
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimitEHT); i++) {
		prChnlSet->opEHT[i] = op;
		prChnlSet->i8PwrLimitEHT[i] = power;
	}
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/

#if (CFG_SUPPORT_WIFI_6G == 1)
	/* Legacy 6G*/
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimitLegacy_6G); i++) {
		prChnlSet->opLegacy_6G[i] = op;
		prChnlSet->i8PwrLimitLegacy_6G[i] = power;
	}
	/* HE 6G*/
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimit6E); i++) {
		prChnlSet->op6E[i] = op;
		prChnlSet->i8PwrLimit6E[i] = power;
	}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	/* EHT 6G*/
	for (i = 0; i < ARRAY_SIZE(prChnlSet->i8PwrLimitEHT_6G); i++) {
		prChnlSet->opEHT_6G[i] = op;
		prChnlSet->i8PwrLimitEHT_6G[i] = power;
	}
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/
#endif /*CFG_SUPPORT_WIFI_6G*/
}

static void txPwrCtrlSetSingleRatePwrLimit(
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChnlSet,
	enum ENUM_PWR_CFG_RATE_TAG eRateTag,
	uint8_t ofset,
	uint8_t op,
	uint8_t value
)
{

	int8_t pwr = 0;
	uint8_t i = 0;


	if (op == 2)
		pwr = 0 - value;
	else
		pwr = value;

	switch (eRateTag) {
	case PWR_CFG_RATE_TAG_HIT_LEGACY:
		/* support pwr limit emi, merge legacy V1&V2
		 * extend CCK to CCK L/H, OFDM L/H
		 */
		if (ofset == 0) {
			for (i = 0; i < 2; i++) {
				prChnlSet->op[i] = op;
				prChnlSet->i8PwrLimit[i] = pwr;
			}
		} else if (ofset == 1) {
			for (i = 0; i < 3; i++) {
				prChnlSet->op[i + 2] = op;
				prChnlSet->i8PwrLimit[i + 2] = pwr;
			}
		} else {
			prChnlSet->op[ofset + 3] = op;
			prChnlSet->i8PwrLimit[ofset + 3] = pwr;
		}
		break;

	case PWR_CFG_RATE_TAG_HIT_LEGACY_V2:
			prChnlSet->op[ofset] = op;
			prChnlSet->i8PwrLimit[ofset] = pwr;
		break;

	case PWR_CFG_RATE_TAG_HIT_HE:
		prChnlSet->opHE[ofset] = op;
		prChnlSet->i8PwrLimitHE[ofset] = pwr;
		break;

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_CFG_RATE_TAG_HIT_EHT:
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
		/* support pwr limit emi, merge EHT / EHT6G
		 * Skip RU996X4 setting.
		 */
		if (ofset > PWR_LIMIT_EHT_RU996X2_U)
			ofset = ofset + 3;
#endif /*CFG_SUPPORT_PWR_LMT_EMI*/
		prChnlSet->opEHT[ofset] = op;
		prChnlSet->i8PwrLimitEHT[ofset] = pwr;
		break;
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/
#if (CFG_SUPPORT_WIFI_6G == 1)
	case PWR_CFG_RATE_TAG_HIT_LEGACY6G:
		/* support pwr limit emi, merge legacy V1&V2
		 * extend CCK to CCK L/H, OFDM L/H
		 */
		if (ofset == 0) {
			for (i = 0; i < 2; i++) {
				prChnlSet->opLegacy_6G[i] = op;
				prChnlSet->i8PwrLimitLegacy_6G[i] = pwr;
			}
		} else if (ofset == 1) {
			for (i = 0; i < 3; i++) {
				prChnlSet->opLegacy_6G[i + 2] = op;
				prChnlSet->i8PwrLimitLegacy_6G[i + 2] = pwr;
			}
		} else {
			prChnlSet->opLegacy_6G[ofset + 3] = op;
			prChnlSet->i8PwrLimitLegacy_6G[ofset + 3] = pwr;
		}
		break;

	case PWR_CFG_RATE_TAG_HIT_LEGACY6G_V2:
		prChnlSet->opLegacy_6G[ofset] = op;
		prChnlSet->i8PwrLimitLegacy_6G[ofset] = pwr;
		break;

	case PWR_CFG_RATE_TAG_HIT_HE6G:
		prChnlSet->op6E[ofset] = op;
		prChnlSet->i8PwrLimit6E[ofset] = pwr;
		break;

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	case PWR_CFG_RATE_TAG_HIT_EHT6G:
		prChnlSet->opEHT_6G[ofset] = op;
		prChnlSet->i8PwrLimitEHT_6G[ofset] = pwr;
		break;
#endif
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/
	default:
		break;
	}

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
	uint16_t value, value2, count = 0;
	uint16_t u2AppliedWay, u2Operation = 0;
	uint32_t u4RollBackStep = 0;
	char carySeperator[2] = { 0, 0 };
	enum ENUM_PWR_CFG_RATE_TAG u1RateTag = 0;
	char *pacParsePwrAC[PWR_LIMIT_PARSER_LEGACY_NUM] = {
		"CCK",
		"HT20L",
		"HT20H",
		"HT40L",
		"HT40H",
		"HT80L",
		"HT80H",
		"HT160L",
		"HT160H"
		};
	char *pacParsePwrAC_V2[PWR_LIMIT_PARSER_LEGACY_NUM_V2] = {
		"CCKL",
		"CCKH",
		"OFDML",
		"OFDMH",
		"HT20L",
		"HT20H",
		"HT40L",
		"HT40H",
		"HT80L",
		"HT80H",
		"HT160L",
		"HT160H"
		};
	char *pacParsePwrAX[PWR_LIMIT_PARSER_HE160_NUM] = {
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
	char *pacParsePwrEHT[PWR_LIMIT_PARSER_EHT_NUM] = {
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
	char *pacParsePwrEHT_6G[PWR_LIMIT_PARSER_EHT6G_NUM] = {
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
	if (txPwrParseNumber(&pcContCur, ";", &op, &u2AppliedWay)) {
		DBGLOG(RLM, ERROR, "parse applied way error: %s\n",
		       pcContOld);
		return NULL;
	}
	if ((u2AppliedWay < PWR_CTRL_TYPE_APPLIED_WAY_WIFION) ||
	    (u2AppliedWay > PWR_CTRL_TYPE_APPLIED_WAY_IOCTL)) {
		DBGLOG(RLM, ERROR,
		       "parse applied way error: value=%u\n",
		       u2AppliedWay);
		return NULL;
	}

	/* parese scenario applied type */
	pcContOld = pcContCur;
	if (txPwrParseNumber(&pcContCur, ";", &op, &u2Operation)) {
		DBGLOG(RLM, ERROR, "parse operation error: %s\n",
		       pcContOld);
		return NULL;
	}
	if ((u2Operation < PWR_CTRL_TYPE_OPERATION_POWER_LEVEL) ||
	    (u2Operation > PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)) {
		DBGLOG(RLM, ERROR,
		       "parse operation error: value=%u\n",
		       u2Operation);
		return NULL;
	}

	switch (u2AppliedWay) {
	case PWR_CTRL_TYPE_APPLIED_WAY_WIFION:
		if (u2Operation == PWR_CTRL_TYPE_OPERATION_POWER_LEVEL)
			value2 = PWR_CTRL_TYPE_WIFION_POWER_LEVEL;
		else
			value2 = PWR_CTRL_TYPE_WIFION_POWER_OFFSET;
		break;
	case PWR_CTRL_TYPE_APPLIED_WAY_IOCTL:
		if (u2Operation == PWR_CTRL_TYPE_OPERATION_POWER_LEVEL)
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
	u4MemSize +=
		(ucSettingCount * sizeof(struct TX_PWR_CTRL_CHANNEL_SETTING));

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
		prCurElement->index = (uint16_t)value;
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

		if (txPwrCfgCheckCfgValid(count, pcContCur) == FALSE)
			goto clearLabel;

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
				if (kalkStrtou8(pcContOld, 0,
						(uint8_t *)&value) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse channel error: %s\n",
					       pcContOld);
					goto clearLabel;
				}
				prTmpSetting->channelParam[0] = value;
#if (CFG_SUPPORT_WIFI_6G == 1)
				if (count == PWR_LIMIT_PARSER_HE160_NUM
					|| (PWR_CFG_RATE_TAG_HIT_LEGACY6G ==
						u1RateTag)
					|| (PWR_CFG_RATE_TAG_HIT_LEGACY6G_V2 ==
						u1RateTag)
					|| (PWR_CFG_RATE_TAG_HIT_HE6G ==
						u1RateTag)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
					|| (PWR_CFG_RATE_TAG_HIT_EHT6G ==
						u1RateTag)
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
					)
					prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_6G_NORMAL;
				else
#endif /* CFG_SUPPORT_WIFI_6G */
					prTmpSetting->eChnlType =
						PWR_CTRL_CHNL_TYPE_NORMAL;
			} else { /* case: channel range */
				if (kalkStrtou8(pcContTmp, 0,
						(uint8_t *)&value) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse first channel error, %s\n",
					       pcContTmp);
					goto clearLabel;
				}
				if (kalkStrtou8(pcContCur2, 0,
						(uint8_t *)&value2) != 0) {
					DBGLOG(RLM, ERROR,
					       "parse second channel error, %s\n",
					       pcContCur2);
					goto clearLabel;
				}
				prTmpSetting->channelParam[0] = value;
				prTmpSetting->channelParam[1] =	value2;
#if (CFG_SUPPORT_WIFI_6G == 1)
				if (count == PWR_LIMIT_PARSER_HE160_NUM
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
			if (txPwrParseNumber(&pcContCur, "]", &op,
					&value)) {
				DBGLOG(RLM, ERROR, "parse CCK error, %s\n",
					   pcContOld);
				goto clearLabel;
			}
			txPwrCtrlSetAllRatePwrLimit(prTmpSetting, op, value);
			goto skipLabel2;

		} else  if (count == PWR_LIMIT_PARSER_LEGACY_NUM ||
			u1RateTag == PWR_CFG_RATE_TAG_HIT_LEGACY) {
			for (j = 0; j < PWR_LIMIT_PARSER_LEGACY_NUM; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_LIMIT_PARSER_LEGACY_NUM - 1)
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

				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting,
					PWR_CFG_RATE_TAG_HIT_LEGACY,
					j, op, value);

				if ((prTmpSetting->op[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
		} else  if (u1RateTag == PWR_CFG_RATE_TAG_HIT_LEGACY_V2) {
			for (j = 0; j < PWR_LIMIT_PARSER_LEGACY_NUM_V2; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_LIMIT_PARSER_LEGACY_NUM_V2 - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse %s error, %s\n",
						pacParsePwrAC_V2[j], pcContOld);
					goto clearLabel;
				}

				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting,
					PWR_CFG_RATE_TAG_HIT_LEGACY_V2,
					j, op, value);

				if ((prTmpSetting->op[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAC_V2[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		} else if (count == PWR_LIMIT_PARSER_HE_NUM) {
			for (j = 0; j < PWR_LIMIT_PARSER_HE_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_PARSER_HE_NUM - 1)
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

				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting, PWR_CFG_RATE_TAG_HIT_HE,
					j, op, value);

				if ((prTmpSetting->opHE[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
			for (j = 0; j < PWR_LIMIT_PARSER_EHT_NUM; j++) {
				/* parse EHT26L ...  EHT996_484_242U setting */
				if (j == PWR_LIMIT_PARSER_EHT_NUM - 1)
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
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting, PWR_CFG_RATE_TAG_HIT_EHT,
					j, op, value);

				if ((prTmpSetting->opEHT[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
		else if (count == PWR_LIMIT_PARSER_HE160_NUM ||
			u1RateTag == PWR_CFG_RATE_TAG_HIT_HE6G) {
			for (j = 0; j < PWR_LIMIT_PARSER_HE160_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_PARSER_HE160_NUM - 1)
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
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting, PWR_CFG_RATE_TAG_HIT_HE6G,
					j, op, value);

				if ((prTmpSetting->op6E[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
			for (j = 0; j < PWR_LIMIT_PARSER_LEGACY_NUM; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_LIMIT_PARSER_LEGACY_NUM - 1)
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
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting,
					PWR_CFG_RATE_TAG_HIT_LEGACY6G,
					j, op, value);

				if ((prTmpSetting->opLegacy_6G[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
		} else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_LEGACY6G_V2) {
			for (j = 0; j < PWR_LIMIT_PARSER_LEGACY_NUM_V2; j++) {
				/* parse cck/20L/20H .. 160L/160H setting */
				if (j == PWR_LIMIT_PARSER_LEGACY_NUM_V2 - 1)
					carySeperator[0] = ']';
				else
					carySeperator[0] = ',';

				pcContOld = pcContCur;
				if (txPwrParseNumber(&pcContCur, carySeperator,
					&op, &value)) {
					DBGLOG(RLM, ERROR,
						"parse LEGACY6G %s error, %s\n",
						pacParsePwrAC_V2[j],
						pcContOld);
					goto clearLabel;
				}
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting,
					PWR_CFG_RATE_TAG_HIT_LEGACY6G_V2,
					j, op, value);

				if ((prTmpSetting->opLegacy_6G[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
					== PWR_CTRL_TYPE_OPERATION_POWER_OFFSET)
					) {
					DBGLOG(RLM, ERROR,
						"parse LEGACY6G %s error, Power_Offset value cannot be positive: %u\n",
						pacParsePwrAC_V2[j],
						value);
					goto clearLabel;
				}
			}
			goto skipLabel2;
		}
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		else if (u1RateTag == PWR_CFG_RATE_TAG_HIT_EHT6G) {
			for (j = 0; j < PWR_LIMIT_PARSER_EHT6G_NUM; j++) {
				/* parse EHT26L ...  EHT996X3_484U setting */
				if (j == PWR_LIMIT_PARSER_EHT6G_NUM - 1)
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
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting,
					PWR_CFG_RATE_TAG_HIT_EHT6G,
					j, op, value);

				if ((prTmpSetting->opEHT_6G[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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
			for (j = 0; j < PWR_LIMIT_PARSER_HE160_NUM; j++) {
				/* parse RU26L ...  RU996U setting */
				if (j == PWR_LIMIT_PARSER_HE160_NUM - 1)
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
				txPwrCtrlSetSingleRatePwrLimit(
					prTmpSetting, PWR_CFG_RATE_TAG_HIT_HE,
					j, op, value);

				if ((prTmpSetting->opHE[j]
					== PWR_CTRL_TYPE_POSITIVE) &&
					(u2Operation
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

	/* parse power limit append tag to list. */
	if (txPwrOnPreParseAppendTag(prCurElement)) {
		DBGLOG(RLM, INFO, "txPwrOnPreParseAppendTag fail.");
		goto clearLabel;
	}

	if (txPwrParseAppendTag(pcContCur, pcContEnd, prCurElement)) {
		DBGLOG(RLM, INFO, "txPwrParseAppendTag fail (%s).", pcContent);
		goto clearLabel;
	}

	return prCurElement;

clearLabel:
	if (prCurElement != NULL)
		kalMemFree(prCurElement, VIR_MEM_TYPE, u4MemSize);
	/* pcCurElement will be free on wifi off */
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
	int i = 0, count = 0, size = 0;

	size = txPwrCtrlListSize(prAdapter, filterType);

	if (size != 0) {
		if (filterType == 1)
			DBGLOG(RLM, TRACE, "Tx Power Ctrl List=[%s], Size=[%d]",
				message,
				txPwrCtrlListSize(prAdapter, filterType));
		else
			DBGLOG(RLM, TRACE, "Tx Power Ctrl List=[%s], Size=[%d]",
				message,
				txPwrCtrlListSize(prAdapter, filterType));
	}

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
						(ucSettingCount *
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

bool txPwrCtrlParseCountryCfgFile(
	struct ADAPTER *prAdapter,
	struct COUNTRY_POWER_LIMIT_FILE_INFO *prPwrLimitFileInfo)
{
	uint8_t *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;

	if (prPwrLimitFileInfo == NULL)
		return FALSE;

	prPwrLimitFileInfo->u4FileSize = 0;
	prPwrLimitFileInfo->fgValid = FALSE;
	prPwrLimitFileInfo->pData = NULL;

	if (kalRequestFirmware(prPwrLimitFileInfo->pcPwrLimitCfgFile,
		&pucConfigBuf, &u4ConfigReadLen, FALSE,
	    kalGetGlueDevHdl(prAdapter->prGlueInfo)) == 0) {
		/* ToDo:: Nothing */
	}

	if (!pucConfigBuf) {
		DBGLOG(RLM, TRACE, "[PwrLimitCfgFile] %s invalid\n",
			prPwrLimitFileInfo->pcPwrLimitCfgFile);
		return FALSE;
	}

	if (u4ConfigReadLen == 0 ||
		(u4ConfigReadLen % prPwrLimitFileInfo->u4Size) != 0) {
		DBGLOG(RLM, INFO, "[PwrLimitCfgFile] %s size invalid %d/%d\n",
			prPwrLimitFileInfo->pcPwrLimitCfgFile,
			u4ConfigReadLen,
			prPwrLimitFileInfo->u4Size);
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
		return FALSE;
	}

	prPwrLimitFileInfo->pData =
		(void *)kalMemAlloc(u4ConfigReadLen, VIR_MEM_TYPE);

	if (!prPwrLimitFileInfo->pData) {
		DBGLOG(RLM, INFO, "[PwrLimitCfgFile] %s malloc fail\n",
			prPwrLimitFileInfo->pcPwrLimitCfgFile);
		kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
		return FALSE;
	}

	kalMemZero(prPwrLimitFileInfo->pData, u4ConfigReadLen);
	kalMemCopy(prPwrLimitFileInfo->pData, pucConfigBuf, u4ConfigReadLen);

	prPwrLimitFileInfo->u4FileSize = u4ConfigReadLen;
	prPwrLimitFileInfo->fgValid = TRUE;
	DBGLOG(RLM, INFO, "[PwrLimitFile]%s valid size %d/%d en:%d Data:%x\n",
		prPwrLimitFileInfo->pcPwrLimitCfgFile,
		prPwrLimitFileInfo->u4FileSize,
		prPwrLimitFileInfo->u4Size,
		prPwrLimitFileInfo->fgValid,
		prPwrLimitFileInfo->pData);

	kalMemFree(pucConfigBuf, VIR_MEM_TYPE, u4ConfigReadLen);
	return TRUE;
}


bool txPwrCtrlFreeCountryCfgFile(
	struct ADAPTER *prAdapter,
	struct COUNTRY_POWER_LIMIT_FILE_INFO *prPwrLimitFileInfo)
{
	bool fgFree = FALSE;

	if (prPwrLimitFileInfo == NULL)
		return FALSE;

	if (prPwrLimitFileInfo->fgValid == TRUE && prPwrLimitFileInfo->pData) {
		kalMemFree(prPwrLimitFileInfo->pData,
			VIR_MEM_TYPE, prPwrLimitFileInfo->u4FileSize);
		prPwrLimitFileInfo->u4FileSize = 0;
		prPwrLimitFileInfo->fgValid = FALSE;
		prPwrLimitFileInfo->pData = NULL;
		fgFree = TRUE;
	}

	DBGLOG(RLM, TRACE, "[PwrLimitCfgFile] %s %s size %d/%d en:%d\n",
		(fgFree == TRUE) ? "free" : "no need to free",
		prPwrLimitFileInfo->pcPwrLimitCfgFile,
		prPwrLimitFileInfo->u4FileSize,
		prPwrLimitFileInfo->u4Size,
		prPwrLimitFileInfo->fgValid);

	return TRUE;
}

void txPwrCtrlPatchCountryCfg(void)
{
	struct COUNTRY_POWER_LIMIT_FILE_INFO *prFileInfo;
	uint8_t i = 0;

	/* Default Table */
	for (i = 0; i < PWR_LIMIT_DEFAULT_BASE_NUM; i++) {
		prFileInfo = &g_arPwrLimitDefFile[i];

		PWR_LIMIT_COUNTRY_DEF_TBL_SET(i,
			g_rlmPowerLimitDefaultTable_bk[i].prPwrLmtDefaultTable,
			g_rlmPowerLimitDefaultTable_bk[i].TableNum);

		if (prFileInfo->fgValid) {
			PWR_LIMIT_COUNTRY_DEF_TBL_SET(i,
				prFileInfo->pData,
				prFileInfo->u4FileSize / prFileInfo->u4Size);
		}
	}

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	/* Config Table */
	for (i = 0; i < PWR_LIMIT_CONFIG_BASE_NUM; i++) {
		prFileInfo = &g_arPwrLimitCfgFile[i].Legacy;
		PWR_LIMIT_COUNTRY_CFG_TBL_SET_LEGACY(i,
			g_rlmPowerLimitConfigTable_bk[i].Legacy.table,
			g_rlmPowerLimitConfigTable_bk[i].Legacy.table_num)

		if (prFileInfo->fgValid) {
			PWR_LIMIT_COUNTRY_CFG_TBL_SET_LEGACY(i,
				prFileInfo->pData,
				prFileInfo->u4FileSize / prFileInfo->u4Size)
		}

		prFileInfo = &g_arPwrLimitCfgFile[i].HE;
		PWR_LIMIT_COUNTRY_CFG_TBL_SET_HE(i,
			g_rlmPowerLimitConfigTable_bk[i].HE.table,
			g_rlmPowerLimitConfigTable_bk[i].HE.table_num)
		if (prFileInfo->fgValid) {
			PWR_LIMIT_COUNTRY_CFG_TBL_SET_HE(i,
				prFileInfo->pData,
				prFileInfo->u4FileSize / prFileInfo->u4Size)
		}

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		prFileInfo = &g_arPwrLimitCfgFile[i].EHT;
		PWR_LIMIT_COUNTRY_CFG_TBL_SET_EHT(i,
			g_rlmPowerLimitConfigTable_bk[i].EHT.table,
			g_rlmPowerLimitConfigTable_bk[i].EHT.table_num)
		if (prFileInfo->fgValid) {
			PWR_LIMIT_COUNTRY_CFG_TBL_SET_EHT(i,
				prFileInfo->pData,
				prFileInfo->u4FileSize / prFileInfo->u4Size)
		}
#endif
	}
#endif
}

void txPwrCtrlParseCountryCfg(struct ADAPTER *prAdapter)
{
	uint8_t i = 0;

	/* Default Table */
	for (i = 0; i < PWR_LIMIT_DEFAULT_BASE_NUM; i++)
		txPwrCtrlParseCountryCfgFile(prAdapter,
			&g_arPwrLimitDefFile[i]);


	/* Config Table */
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	for (i = 0; i < PWR_LIMIT_CONFIG_BASE_NUM; i++) {
		txPwrCtrlParseCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].Legacy);
		txPwrCtrlParseCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].HE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		txPwrCtrlParseCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].EHT);
#endif
	}
#else
	for (i = 0; i < CFG_V0_NUM; i++) {
		txPwrCtrlParseCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgV0File[i]);
	}
#endif
}

void txPwrCtrlFreeCountryCfg(struct ADAPTER *prAdapter)
{
	uint8_t i = 0;

	/* Default Table */
	for (i = 0; i < PWR_LIMIT_DEFAULT_BASE_NUM; i++)
		txPwrCtrlFreeCountryCfgFile(prAdapter, &g_arPwrLimitDefFile[i]);
	/* Config Table */
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	for (i = 0; i < PWR_LIMIT_CONFIG_BASE_NUM; i++) {
		txPwrCtrlFreeCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].Legacy);
		txPwrCtrlFreeCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].HE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
		txPwrCtrlFreeCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgFile[i].EHT);
#endif
	}
#else
	for (i = 0; i < CFG_V0_NUM; i++) {
		txPwrCtrlFreeCountryCfgFile(prAdapter,
			&g_arPwrLimitCfgV0File[i]);
	}
#endif
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

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
#if KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE
void txPwrCtrlCMDTimeout(struct timer_list *timer)
#else
void txPwrCtrlCMDTimeout(unsigned long data)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE
	struct GLUE_INFO *prGlueInfo =
		from_timer(prGlueInfo, timer, rTxPowerLimitTimer);
#else
	struct GLUE_INFO *prGlueInfo = (struct GLUE_INFO *)data;
#endif

	rlmDomainPwrLmtEmiStatusCtrl(prGlueInfo->prAdapter,
		TX_PWR_EMI_STATUS_ACTION_CLEAR);
}
#endif

void txPwrCtrlInit(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	uint32_t i, j, u4PwrLimitSize;
	struct GLUE_INFO *prGlueInfo;
#endif

	/* parse country power limit config */
	txPwrCtrlParseCountryCfg(prAdapter);

	/* patch country power limit config */
	txPwrCtrlPatchCountryCfg();

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	LINK_INITIALIZE(&prAdapter->rTxpEmiCtrl.rReqChLUT);
	u4PwrLimitSize = sizeof(struct SET_COUNTRY_CHANNEL_POWER_LIMIT);

	prAdapter->prPwrLimit =
		(struct SET_COUNTRY_CHANNEL_POWER_LIMIT **) kalMemAlloc(
			sizeof(struct SET_COUNTRY_CHANNEL_POWER_LIMIT *)
			* PWR_LIMIT_RF_BAND_NUM,
			VIR_MEM_TYPE);

	prGlueInfo = prAdapter->prGlueInfo;

#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_INIT(NULL, prGlueInfo->rTxPowerEmiWakeLock,
			   "Tx Power");
#endif

#if KERNEL_VERSION(4, 15, 0) <= CFG80211_VERSION_CODE
	timer_setup(&prGlueInfo->rTxPowerLimitTimer,
		txPwrCtrlCMDTimeout, 0);
#else
	init_timer(&prGlueInfo->rTxPowerLimitTimer);
	prGlueInfo->rTxPowerLimitTimer.data =
		(unsigned long)prGlueInfo;
	prGlueInfo->rTxPowerLimitTimer.function =
		txPwrCtrlCMDTimeout;
#endif

	if (prAdapter->prPwrLimit == NULL) {
		DBGLOG(RLM, INFO,
			"prAdapter->prPwrLimit alloc fail in txPwrCtrlInit\n");

		return;
	}

	for (i = 0; i < PWR_LIMIT_RF_BAND_NUM; i++) {
		prAdapter->prPwrLimit[i] =
			(struct SET_COUNTRY_CHANNEL_POWER_LIMIT *) kalMemAlloc(
				u4PwrLimitSize * PWR_LIMIT_PROTOCOL_NUM,
				VIR_MEM_TYPE);

		if (prAdapter->prPwrLimit[i] == NULL) {
			for (j = 0; j < i; j++) {
				kalMemFree(prAdapter->prPwrLimit[j],
					VIR_MEM_TYPE,
					u4PwrLimitSize
					* PWR_LIMIT_PROTOCOL_NUM);
				prAdapter->prPwrLimit[j] = NULL;
			}
			kalMemFree(prAdapter->prPwrLimit,
				VIR_MEM_TYPE,
				sizeof(struct SET_COUNTRY_CHANNEL_POWER_LIMIT *)
				* PWR_LIMIT_RF_BAND_NUM);
			prAdapter->prPwrLimit = NULL;
			return;
		}
	}
#endif


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
					(ucSettingCount * u4SettingSize);
				kalMemFree(prCurElement,
					VIR_MEM_TYPE, u4MemSize2);
			}
		}
	}

	txPwrCtrlFreeCountryCfg(prAdapter);

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
	txPwrConnectionDelAllLUTElement(prAdapter);

	for (i = 0; i < PWR_LIMIT_RF_BAND_NUM; i++) {
		kalMemFree(
			prAdapter->prPwrLimit[i],
			VIR_MEM_TYPE,
			sizeof(struct SET_COUNTRY_CHANNEL_POWER_LIMIT)
			* PWR_LIMIT_PROTOCOL_NUM);
	}
	kalMemFree(
		prAdapter->prPwrLimit,
		VIR_MEM_TYPE,
		sizeof(struct SET_COUNTRY_CHANNEL_POWER_LIMIT *)
		* PWR_LIMIT_RF_BAND_NUM);

#if (KERNEL_VERSION(6, 15, 0) > CFG80211_VERSION_CODE)
	del_timer_sync(&prAdapter->prGlueInfo->rTxPowerLimitTimer);
#else
	timer_delete_sync(&prAdapter->prGlueInfo->rTxPowerLimitTimer);
#endif

#if CFG_ENABLE_WAKE_LOCK
	/* Make sure wakelock ctrl is not race condition with
	 * txPwrCtrlCMDTimeout, so we use del_timer_sync to make
	 * here is always after txPwrCtrlCMDTimeout.
	 */
	if (KAL_WAKE_LOCK_ACTIVE(NULL,
				 prAdapter->prGlueInfo->rTxPowerEmiWakeLock))
		KAL_WAKE_UNLOCK(NULL,
			prAdapter->prGlueInfo->rTxPowerEmiWakeLock);
	KAL_WAKE_LOCK_DESTROY(NULL,
		prAdapter->prGlueInfo->rTxPowerEmiWakeLock);
#endif
#endif
}
/* dynamic tx power control: end **********************************************/
#endif /* CFG_SUPPORT_DYNAMIC_PWR_LIMIT */

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
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
			DBGLOG(RLM, TRACE,
				"%s ANT Cfg%d Tag[%d]Ant[%d]Band[%d]Val[%d]\n",
				message,
				i,
				prPwrLmtAnt[i].cTagIdx,
				prPwrLmtAnt[i].cAntIdx,
				prPwrLmtAnt[i].cBandIdx,
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
			if (i >= MAX_CMD_EHT_SUPPORT_CHANNEL_NUM) {
				DBGLOG(RLM, ERROR, "out of ETH CH Num\n");
				return;
			}

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
			prcRatePwr =
				&prCmdPwrLimtLegacy_6G->cPwrLimitCCK_L;
			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"Legacy_6G ch=%d,Limit=",
				prCmdPwrLimtLegacy_6G->ucCentralCh);

			/*message body*/
			for (j = PWR_LIMIT_CCK_L;
				j < PWR_LIMIT_LEGACY_6G_NUM ; j++)
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
			if (i >= MAX_CMD_EHT_6G_SUPPORT_CHANNEL_NUM) {
				DBGLOG(RLM, ERROR, "out of ETH 6G CH Num\n");
				return;
			}

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
			prcRatePwr = &prPwrLmt->cPwrLimitCCK_L;
			/*message head*/
			msgOfs += snprintf(msgLimit + msgOfs,
				PWR_BUF_LEN - msgOfs,
				"legacy ch=%d,Limit=",
				prPwrLmt->ucCentralCh);

			/*message body*/
			for (k = PWR_LIMIT_CCK_L; k < PWR_LIMIT_NUM ; k++)
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
#endif /*#if (CFG_SUPPORT_PWR_LMT_EMI == 0)*/


#if (CFG_SUPPORT_PWR_LMT_EMI == 0)
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
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLmtDefaultTable =
				g_rRlmPowerLimitDefault;
	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefBase;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	/* TODO : 5G band edge */

	if (regd_is_single_sku_en(prAdapter)) {
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

	eDefBase = rlmDomainPwrLmtGetDefaultBase(prAdapter);
	u2DefaultTableIndex =
	    rlmDomainPwrLimitDefaultTableDecision(prAdapter,
			prAdapter->rWifiVar.u2CountryCode,
			eDefBase);

	if (u2DefaultTableIndex == POWER_LIMIT_TABLE_NULL) {
		DBGLOG(RLM, ERROR,
			"Can't find any table index!\n");
		goto err;
	}

	prPwrLmtDefaultTable = PWR_LIMIT_COUNTRY_DEF_TBL_ALL(eDefBase);

	pu1PwrLmtCountryCode = &prPwrLmtDefaultTable[u2DefaultTableIndex]
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
	prCmd->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmd->ucNum = 0;
	prCmd->ucLimitType = PWR_LIMIT_TYPE_COMP_11AC_V2;
	prCmd->ucVersion = 1;

	prCmdHE->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdHE->ucNum = 0;
	prCmdHE->fgPwrTblKeep = TRUE;
	prCmdHE->ucLimitType = PWR_LIMIT_TYPE_COMP_11AX;

	if (prAdapter->rWifiVar.ucSta5gBandwidth == MAX_BW_160MHZ)
		prCmdHE->ucLimitType = PWR_LIMIT_TYPE_COMP_11AX_BW160;

	prCmdHE->ucVersion = 1;

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_1->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_1->ucNum = 0;
	prCmdEHT_1->fgPwrTblKeep = TRUE;
	prCmdEHT_1->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_1;
	prCmdEHT_1->ucVersion = 1;

	prCmdEHT_2->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_2->ucNum = 0;
	prCmdEHT_2->fgPwrTblKeep = TRUE;
	prCmdEHT_2->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_2;
	prCmdEHT_2->ucVersion = 1;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	prCmdAnt->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdAnt->ucNum = 0;
	prCmdAnt->fgPwrTblKeep = TRUE;
	/* ANT number if PWR_LIMIT_TYPE_COMP_ANT*/
	prCmdAnt->ucLimitType = PWR_LIMIT_TYPE_COMP_ANT_V2;
	prCmdAnt->ucVersion = 1;
#endif
#if (CFG_SUPPORT_WIFI_6G == 1)
	prCmd6E_1->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmd6E_1->ucNum = 0;
	prCmd6E_1->fgPwrTblKeep = TRUE;
	prCmd6E_1->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_1;

	prCmd6E_2->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmd6E_2->ucNum = 0;
	prCmd6E_2->fgPwrTblKeep = TRUE;
	prCmd6E_2->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_2;

	prCmd6E_3->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmd6E_3->ucNum = 0;
	prCmd6E_3->fgPwrTblKeep = TRUE;
	prCmd6E_3->ucLimitType = PWR_LIMIT_TYPE_COMP_6E_3;

	prCmdLegacy_6G_1->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdLegacy_6G_1->ucNum = 0;
	prCmdLegacy_6G_1->fgPwrTblKeep = TRUE;
	prCmdLegacy_6G_1->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1;

	prCmdLegacy_6G_2->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdLegacy_6G_2->ucNum = 0;
	prCmdLegacy_6G_2->fgPwrTblKeep = TRUE;
	prCmdLegacy_6G_2->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2;

	prCmdLegacy_6G_3->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdLegacy_6G_3->ucNum = 0;
	prCmdLegacy_6G_3->fgPwrTblKeep = TRUE;
	prCmdLegacy_6G_3->ucLimitType = PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3;

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	prCmdEHT_6G_1->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_1->ucNum = 0;
	prCmdEHT_6G_1->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_1->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_1;
	prCmdEHT_6G_1->ucVersion = 1;

	prCmdEHT_6G_2->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_2->ucNum = 0;
	prCmdEHT_6G_2->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_2->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_2;
	prCmdEHT_6G_2->ucVersion = 1;

	prCmdEHT_6G_3->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_3->ucNum = 0;
	prCmdEHT_6G_3->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_3->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_3;
	prCmdEHT_6G_3->ucVersion = 1;

	prCmdEHT_6G_4->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_4->ucNum = 0;
	prCmdEHT_6G_4->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_4->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_4;
	prCmdEHT_6G_4->ucVersion = 1;

	prCmdEHT_6G_5->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_5->ucNum = 0;
	prCmdEHT_6G_5->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_5->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_5;
	prCmdEHT_6G_5->ucVersion = 1;

	prCmdEHT_6G_6->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdEHT_6G_6->ucNum = 0;
	prCmdEHT_6G_6->fgPwrTblKeep = TRUE;
	prCmdEHT_6G_6->ucLimitType = PWR_LIMIT_TYPE_COMP_11BE_6G_6;
	prCmdEHT_6G_6->ucVersion = 1;
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmd,
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
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdHE,
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
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_1,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_1);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_2,
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
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmd6E_1,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmd6E_1);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmd6E_2,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmd6E_2);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmd6E_3,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmd6E_3);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdLegacy_6G_1,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdLegacy_6G_1);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdLegacy_6G_2,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdLegacy_6G_2);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdLegacy_6G_3,
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
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_1,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_6G_1);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_2,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_6G_2);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_3,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_6G_3);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_4,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_6G_4);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_5,
		u2DefaultTableIndex);

	/*<2>Command - configuration table information,
	 * replace specified channel
	 */
	rlmDomainBuildCmdByConfigTable(prAdapter,
		prCmdEHT_6G_5);

	/*<1>Command - default table information,
	 *fill all subband
	 */
	rlmDomainBuildCmdByDefaultTable(prAdapter,
		prCmdEHT_6G_6,
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
	txPwrCtrlAntPowerCacheInit(prAdapter);
	txPwrCtrlApplySettings(prAdapter, prCmdAnt, bandedgeParam);
	txPwrCtrlSetAntPowerToCmdBuf(prAdapter, prCmdAnt);
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
#endif /*CFG_SUPPORT_PWR_LMT_EMI*/
#if (CFG_SUPPORT_NAN == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is update for NAN timeline update
 *
 * \param[in] prAdapter : Pointer to adapter
 *
 * \return value : bool
 *                 FALSE : not prefer
 *                 TRUE : Prefer
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomainNanTimeLineUpdateNotify(
	struct ADAPTER *prAdapter
)
{
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo;

	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

		if (IS_BSS_NAN(prBssInfo) == FALSE)
			continue;

		if (prBssInfo->eBand != BAND_5G
#if (CFG_SUPPORT_WIFI_6G == 1)
			&& prBssInfo->eBand != BAND_6G
#endif
			) {
			/* NAN may use 6G but record prBssInfo->eBand = 5G */
			continue;
		}

		rlmDomainBssUpdateNotify(prAdapter, ucBssIdx);
	}

	return WLAN_STATUS_SUCCESS;
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use for rlm_domain handle bss update
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] ucBssIdx : Bss index
 *
 * \return value : WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomainBssUpdateNotify(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIdx)
{
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	struct BSS_INFO *prBssInfo = NULL;
	struct BSS_DESC *prBssDesc = NULL;
	enum ENUM_PWR_MODE_6G_TYPE ePwrMode = PWR_MODE_6G_LPI;
	uint8_t ucBw = 0;

	/* Parameter check */
	if ((!prAdapter) || (ucBssIdx >= MAX_BSSID_NUM)) {
		DBGLOG(RLM, ERROR,
			"invalid param, prAdapter[%s]BssIdx[%d]\n",
			!prAdapter ? "NULL" : "Valid",
			ucBssIdx);

		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	DBGLOG(RLM, TRACE,
		"BssIdx[%d]OpMode[%d]NetType[%d]Act[%d]use[%d]Connect[%d]PWR_STATE[%d]Alive[%d]\n",
		ucBssIdx,
		prBssInfo->eCurrentOPMode,
		prBssInfo->eNetworkType,
		prBssInfo->fgIsNetActive,
		prBssInfo->fgIsInUse,
		prBssInfo->eConnectionState,
		IS_NET_PWR_STATE_ACTIVE(prAdapter, prBssInfo->ucBssIndex),
		IS_BSS_ALIVE(prAdapter, prBssInfo));

	/* If not alive, reset to default LPI */
	if (IS_BSS_ALIVE(prAdapter, prBssInfo) != TRUE ||
		rlmDomainIsBssUse6g(prAdapter, prBssInfo, &ucBw) == FALSE) {
		rlmDomain6GPwrModeUpdate(prAdapter,
			ucBssIdx, PWR_MODE_6G_LPI);

		return WLAN_STATUS_SUCCESS;
	}

	if (IS_BSS_AIS(prBssInfo)) {
		/* For STA, get 6G power mode from Bss descripter */
		prBssDesc = aisGetTargetBssDesc(prAdapter, ucBssIdx);

		if (!prBssDesc)
			return WLAN_STATUS_INVALID_DATA;

		ePwrMode = prBssDesc->e6GPwrMode;
	}
#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	else if (IS_BSS_P2P(prBssInfo)) {
		/* For CE segment, P2P GC will align GO claim device type */
		prBssDesc = p2pGetTargetBssDesc(prAdapter, ucBssIdx);

		if (!prBssDesc)
			return WLAN_STATUS_INVALID_DATA;

		ePwrMode = prBssDesc->e6GPwrMode;
	}
#endif
	else {
		/* 1. For NAN, always set to VLP
		 * 2. For Mobile segment, P2P GC/GO always set to VLP
		 */
		ePwrMode = PWR_MODE_6G_VLP;
	}

	rlmDomain6GPwrModeUpdate(prAdapter, ucBssIdx, ePwrMode);
#endif
	return WLAN_STATUS_SUCCESS;
}
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use check bss is use 6G or not
 *
 * \param[in] prAdapter
 * \param[in] prBssInfo
 * \param[in] pucBw
 *
 * \return fgUse6g
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainIsBssUse6g(
	struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo,
	uint8_t *pucBw)
{
	uint8_t fgUse6g = FALSE;

	if (!prAdapter || !prBssInfo || !pucBw)
		return FALSE;

#if (CFG_SUPPORT_NAN == 1)
	if (IS_BSS_NAN(prBssInfo))
		fgUse6g =
		nanIs6gInUse(prAdapter, (enum _NAN_CHNL_BW_MAP *)(pucBw));
	else
		fgUse6g = (prBssInfo->eBand == BAND_6G) ? TRUE : FALSE;
#else
	fgUse6g = (prBssInfo->eBand == BAND_6G) ? TRUE : FALSE;
#endif

	return fgUse6g;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use get Bandwidth frequcy range
 *
 * \param[in] ucBw : Bandwidth
 * \param[in] pu2FreqRange : frequcy range
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomainGetBwFreqRang(
	uint8_t ucBw,
	uint16_t *pu2FreqRange
)
{
	switch (ucBw) {
	case MAX_BW_20MHZ:
		*pu2FreqRange = 20;
		break;
	case MAX_BW_40MHZ:
		*pu2FreqRange = 40;
		break;
	case MAX_BW_80MHZ:
		*pu2FreqRange = 80;
		break;
	case MAX_BW_160MHZ:
		*pu2FreqRange = 160;
		break;
	case MAX_BW_320_1MHZ:
	case MAX_BW_320_2MHZ:
		*pu2FreqRange = 320;
		break;
	default:
		DBGLOG(RLM, WARN, "unexpected channel width: %d\n", ucBw);
		return WLAN_STATUS_INVALID_DATA;
	}

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use check current channel & BW is prefer use or not
 *        by 6G power mode
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] eBand : RF band
 * \param[in] ucPriCh : Primary channel
 * \param[in] ucBw : Channel BW
 * \param[in] ePwrMode : 6G Power mode
 *
 * \return value : bool
 *                 FALSE : not prefer
 *                 TRUE : Prefer
 */
/*----------------------------------------------------------------------------*/
bool rlmDomain6GPwrModeIsChnlPrefer(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucBw,
	enum ENUM_PWR_MODE_6G_TYPE eMode)
{
	uint32_t u4CenterFreq = 0;
	uint32_t u4StartFreq = 0;
	uint16_t u2FreqRange = 0;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	uint8_t ucIsPrefer = TRUE;
	uint16_t u2CntryCode = 0;
	uint8_t fgCcNull = 0;

	if ((!prAdapter) || (ucBw >= MAX_BW_UNKNOWN) ||
		(eMode >= PWR_MODE_6G_NUM)) {
		DBGLOG(RLM, ERROR,
			"invalid param, prAdapter[%s]Bw[%d]mode[%d]\n",
			!prAdapter ? "NULL" : "Valid",
			ucBw,
			eMode);

		return FALSE;
	}

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	u2CntryCode = prAdapter->rWifiVar.u2CountryCode;
	fgCcNull = (u2CntryCode == COUNTRY_CODE_NULL) ? TRUE : FALSE;

	/* Currently only 6G VLP FCC need */
	if ((eBand == BAND_6G) && (eMode == PWR_MODE_6G_VLP) &&
		(prDomainInfo->eRegCat == REG_CAT_FCC)) {

		if (rlmDomainGetBwFreqRang(ucBw, &u2FreqRange) !=
						WLAN_STATUS_SUCCESS)
			return FALSE;

		u4CenterFreq =
			nicGetCenterChFreq(eBand, ucPriCh, CHNL_EXT_SCN, ucBw);

		if (u4CenterFreq == 0)
			return FALSE;

		u4StartFreq = u4CenterFreq - (u2FreqRange / 2);

		/* For FCC VLP prefer use freqency above 6105MHz */
		if (u4StartFreq < FCC_VLP_CHNL_FREQ_ABOVE_PREFER)
			ucIsPrefer =  FALSE;
	} else {
		ucIsPrefer = TRUE;
	}

	DBGLOG(RLM, TRACE,
		"(%c%c)reg[%d]band[%d]pri_ch[%d]bw[%d]mode[%d]center_f[%d]f_range[%d]start_f[%d]above_f[%d]prefer[%d]\n",
			fgCcNull ? '0' : ((u2CntryCode & 0xff00) >> 8),
			fgCcNull ? '0' : ((u2CntryCode & 0xff00) >> 8),
			prDomainInfo->eRegCat,
			eBand,
			ucPriCh,
			ucBw,
			eMode,
			u4CenterFreq,
			u2FreqRange,
			u4StartFreq,
			FCC_VLP_CHNL_FREQ_ABOVE_PREFER,
			ucIsPrefer);

	return ucIsPrefer;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update 6G power mode, when the power mode have
 *        been change, it will re-send country power limit cmd to FW
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] ucBssIndex : Bss index
 * \param[in] e6GPwrMode : Enum of 6G power mode
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomain6GPwrModeUpdate(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeBss)
{
	uint8_t ucCurrChnl = 0;
	uint8_t ucPreChnl = 0;
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeCurr = PWR_MODE_6G_LPI;
	uint8_t ucBw = 0;
	struct BSS_INFO *prBssInfo;
	uint8_t fgUpdate = FALSE;
#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeFinal = PWR_MODE_6G_LPI;
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */

	/* Sanity check parameter */
	if ((!prAdapter) ||
	    (ucBssIndex >= MAX_BSSID_NUM) ||
	    (e6GPwrModeBss >= PWR_MODE_6G_NUM)) {
		DBGLOG(RLM, ERROR, "invalid parameter, BssIdx[%d]PwrMode[%d]\n",
			ucBssIndex,
			e6GPwrModeBss);
		return WLAN_STATUS_INVALID_DATA;
	}

	prBssInfo = prAdapter->aprBssInfo[ucBssIndex];
	ucCurrChnl = rlmDomainGet6GBssChnl(prAdapter, ucBssIndex);
	ucPreChnl = g_r6GPwrModeBssInfo[ucBssIndex].ucChnl;

#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
	e6GPwrModeCurr = rlmDomainGetCurr6GPwrMode(prAdapter);
	prAdapter->e6GPwrMode[ucBssIndex] = e6GPwrModeBss;
	e6GPwrModeFinal = rlmDomainGetCurr6GPwrMode(prAdapter);

	if (prAdapter->e6GPwrModeCurr != e6GPwrModeFinal) {
		/* Resend power limit */
		DBGLOG(RLM, INFO, "Set to 6GPwrMode[%u->%u]Curr[%u]",
			prAdapter->e6GPwrModeCurr,
			e6GPwrModeFinal,
			e6GPwrModeCurr);
		prAdapter->e6GPwrModeCurr = e6GPwrModeFinal;
		rlmDomainUpdatePwrLimit_6G_By_PowerMode(
				prAdapter, e6GPwrModeFinal);
	}
#else

	e6GPwrModeCurr = prAdapter->e6GPwrMode[ucBssIndex];

	if (e6GPwrModeCurr != e6GPwrModeBss) {
		/* Power mode change */
		fgUpdate = TRUE;
	} else if (ucCurrChnl != ucPreChnl) {
		if (e6GPwrModeCurr == PWR_MODE_6G_LPI &&
			e6GPwrModeBss == PWR_MODE_6G_LPI) {
			/* Do nothing due to default is use LPI
			 * even channel is different, default setting is use LPI
			 * which is correct.
			 */
		} else {
			fgUpdate = TRUE;
		}
	}

	if (fgUpdate) {
		prAdapter->e6GPwrMode[ucBssIndex] = e6GPwrModeBss;
		g_r6GPwrModeBssInfo[ucBssIndex].ucChnl = ucCurrChnl;
		/* Resend power limit  */
		rlmDomainSendPwrLimitCmd(prAdapter);
	}
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */

	DBGLOG(RLM, TRACE,
		"BSS[%d]Use_6G[%d]Net[%d]Update[%d]PreCh[%d]CurrCh[%d]PreMode[%d]CurrMode[%d]force[%d]\n",
		ucBssIndex,
		rlmDomainIsBssUse6g(prAdapter, prBssInfo, &ucBw),
		prBssInfo->eNetworkType,
		fgUpdate,
		ucPreChnl,
		ucCurrChnl,
		e6GPwrModeCurr, /* Previous */
		prAdapter->e6GPwrMode[ucBssIndex], /* Current */
		prAdapter->fg6GPwrModeForce);

	return WLAN_STATUS_SUCCESS;
}
#if (CFG_SUPPORT_NAN == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get Bss op BW by Nan Chnl
 *
 * \param[in] prAdapter
 * \param[in] e6gBandwidth : NAN BW
 *
 * \return value : Bss Op BW
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainGetBssOpBwByNanChnl(enum _NAN_CHNL_BW_MAP eBw)
{
	switch (eBw) {
	case NAN_CHNL_BW_20:
		return MAX_BW_20MHZ;
	case NAN_CHNL_BW_40:
		return MAX_BW_40MHZ;
	case NAN_CHNL_BW_80:
		return MAX_BW_80MHZ;
	case NAN_CHNL_BW_160:
		return MAX_BW_160MHZ;
	case NAN_CHNL_BW_320:
		return MAX_BW_320_1MHZ;
	default:
		DBGLOG(RLM, WARN, "unexpected channel width: %d\n", eBw);
		return 0;
	}
}
#endif
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get bss channel
 *
 * \param[in] prAdapter
 * \param[in] ucBssIdx
 *
 * \return value : Bss channel
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainGet6GBssChnl(struct ADAPTER *prAdapter,
						uint8_t ucBssIdx)
{
	struct BSS_INFO *prBssInfo;
	uint8_t ucBssChnl;
	uint8_t ucBw;

	if (!prAdapter)
		return 0;

	prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

	if (rlmDomainIsBssUse6g(prAdapter, prBssInfo, &ucBw) == FALSE)
		return 0;

#if (CFG_SUPPORT_NAN == 1)
	if (IS_BSS_NAN(prBssInfo)) {
		if (rlmDomainPwrLmtGetChannelDefine()
				== PWR_LIMIT_DEFINE_PRIMARY_CHANNEL) {
			/* return NAN primary channel*/
			ucBssChnl =  prAdapter->rWifiVar.ucNan6gDefaultChannel;
		} else {
			ucBssChnl = nicGetCenterCh(BAND_6G,
				prAdapter->rWifiVar.ucNan6gDefaultChannel,
				CHNL_EXT_SCN,
				rlmDomainGetBssOpBwByNanChnl((
					enum _NAN_CHNL_BW_MAP)ucBw));

		}
	} else
#endif
	{
		if (rlmDomainPwrLmtGetChannelDefine()
			== PWR_LIMIT_DEFINE_PRIMARY_CHANNEL) {
			ucBssChnl = prBssInfo->ucPrimaryChannel;
		} else {
			ucBw = rlmGetBssOpBwByChannelWidth(
				prBssInfo->eBssSCO,
				prBssInfo->ucVhtChannelWidth);

			ucBssChnl = nicGetCenterCh(BAND_6G,
				prBssInfo->ucPrimaryChannel,
				CHNL_EXT_SCN,
				ucBw);
		}
	}

	DBGLOG(RLM, INFO, "Bss[%d]Net[%d]define_pri[%d]BssCh[%d]\n",
		ucBssIdx,
		prBssInfo->eNetworkType,
		rlmDomainPwrLmtGetChannelDefine(),
		ucBssChnl);

	return ucBssChnl;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get 6G power mode, default use LPI
 *
 * \param[in] prAdapter
 *
 * \return value : 6G power mode
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainPwrLmt6GPwrModeGetByChnl(
	struct ADAPTER *prAdapter,
	uint8_t ucCurrChnl)
{
	uint8_t ucBssIdx = 0;
	uint8_t fgUseDefault = TRUE;
	struct BSS_INFO *prBssInfo;
	enum ENUM_BAND eBand;
	enum ENUM_PWR_MODE_6G_TYPE eFinalMode, eCurrBssMode;
	uint8_t ucBw = 0;

	eFinalMode = PWR_MODE_6G_SP;
	eCurrBssMode = PWR_MODE_6G_SP;

	if (prAdapter->fg6GPwrModeForce == TRUE)
		return prAdapter->e6GPwrMode[0];

	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];

		eCurrBssMode = prAdapter->e6GPwrMode[ucBssIdx];
		eBand = prBssInfo->eBand;

		/* 1. For normal mode will check whether the net is active or
		 *    current mode is test mode.
		 */
		if (ALLOW_6G_PWR_MODE_CHECK(prAdapter, prBssInfo) == FALSE)
			continue;

		/* 2. STA/P2P or SAP will set usage band in prBssInfo->eBand,
		 *    but, NAN may use 6G, even if prBssInfo->eBand != BAND_6G
		 */
		if (rlmDomainIsBssUse6g(prAdapter, prBssInfo, &ucBw) == FALSE)
			continue;

		/* 3. 6G Power limit will be set by channel */
		if (rlmDomainGet6GBssChnl(prAdapter, ucBssIdx) != ucCurrChnl)
			continue;

		/* 4. 6G power mode priority :
		 *              VLP(H) > LPI(M) > SP(L) = LPI_SP(L)
		 */
		if (eCurrBssMode >= eFinalMode) {
			eFinalMode = eCurrBssMode;
			fgUseDefault = FALSE;

			DBGLOG(RLM, LOUD, "Valid BSS[%u]Curr[%u]Final[%u]",
				ucBssIdx,
				eCurrBssMode,
				eFinalMode);
		}
	}

	if (fgUseDefault)
		return PWR_MODE_6G_LPI; /* default mode */
	else
		return eFinalMode;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get 6G power mode, default use LPI
 *
 * \param[in] prAdapter
 *
 * \return value : 6G power mode
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainPwrLmt6GPwrModeGet(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIdx = 0;
	uint8_t fgUseDefault = TRUE;
	struct BSS_INFO *prBssInfo;
	enum ENUM_BAND eBand;
	enum ENUM_PWR_MODE_6G_TYPE eFinalMode, eCurrBssMode;
	uint8_t ucBw = 0;

	eFinalMode = PWR_MODE_6G_SP;
	eCurrBssMode = PWR_MODE_6G_SP;

	if (prAdapter->fg6GPwrModeForce == TRUE)
		return prAdapter->e6GPwrMode[0];

	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++) {

		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];
		eCurrBssMode = prAdapter->e6GPwrMode[ucBssIdx];
		eBand = prBssInfo->eBand;

		/* 1. For normal mode will check whether the net is active or
		 *    current mode is test mode.
		 */
		if (ALLOW_6G_PWR_MODE_CHECK(prAdapter, prBssInfo) == FALSE)
			continue;

		/* 2. STA/P2P or SAP will set usage band in prBssInfo->eBand,
		 *    but, NAN may use 6G, even if prBssInfo->eBand != BAND_6G
		 */
		if (rlmDomainIsBssUse6g(prAdapter, prBssInfo, &ucBw) == FALSE)
			continue;

		/* 3. 6G power mode priority :
		 *              VLP(H) > LPI(M) > SP(L) = LPI_SP(L)
		 */
		if (eCurrBssMode >= eFinalMode) {
			eFinalMode = eCurrBssMode;
			fgUseDefault = FALSE;

			DBGLOG(RLM, LOUD, "Valid BSS[%u]Curr[%u]Final[%u]",
				ucBssIdx,
				eCurrBssMode,
				eFinalMode);
		}
	}

	if (fgUseDefault)
		return PWR_MODE_6G_LPI; /* default mode */
	else
		return eFinalMode;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to 6G subband index by center channel
 *
 * \param[in] eBand : RF Band index
 * \param[in] ucCenterCh : Center channel
 * \param[in] pu1SubBandIdx : Pointer of subband index
 *
 * \return value : 6G power mode
 */
/*----------------------------------------------------------------------------*/
static uint32_t rlmDomainGetSubBandIdx(enum ENUM_BAND eBand,
	uint8_t ucCenterCh,
	uint8_t *pu1SubBandIdx)
{

	uint8_t u1SubBandSize = sizeof(g_rRlmSubBand) /
		sizeof(struct SUBBAND_CHANNEL);
	uint8_t u1Idx = 0;

	for (u1Idx = 0; u1Idx < u1SubBandSize; u1Idx++) {
		if ((eBand == g_rRlmSubBand[u1Idx].eBand) &&
			(ucCenterCh >= g_rRlmSubBand[u1Idx].ucStartCh) &&
			(ucCenterCh <= g_rRlmSubBand[u1Idx].ucEndCh))
			break; /* Found */
	}
	if (u1Idx >= u1SubBandSize) {
		DBGLOG(RLM, ERROR,
			"Can't find Band[%d]Ch[%d] in any Subband\n",
			eBand, ucCenterCh);
		return WLAN_STATUS_INVALID_DATA;
	}
	rlmDomainGetSubBandDefPwrIdx(u1Idx, pu1SubBandIdx);

	DBGLOG(RLM, TRACE, "Band[%d],Ch[%d],SubBandIdx[%d]\n",
		eBand, ucCenterCh, *pu1SubBandIdx);

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to set extent to check VLP mode or not
 *
 * \param[in] prAdapter : Flag for HE 6G info have present
 *
 * \return value : WLAN_STATUS
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomainSet6GPwrModeVlpExtChk(
	struct ADAPTER *prAdapter)
{
	if (!prAdapter)
		return WLAN_STATUS_INVALID_DATA;

#if WLAN_INCLUDE_SYS
	if (prAdapter->ucRfTestMode == 0 || prAdapter->ucRfTestMode == 2)
		prAdapter->rWifiVar.fgVlpExtChk = TRUE;
	else
		prAdapter->rWifiVar.fgVlpExtChk = FALSE;

	DBGLOG(RLM, STATE,
		"Set Vlp extent check[%d]RfTestMode[%d]\n",
		prAdapter->rWifiVar.fgVlpExtChk,
		prAdapter->ucRfTestMode);
#endif

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to decide the current 6G power mode for STA
 *
 * \param[in] fgIsHE6GPresent : Flag for HE 6G info have present
 * \param[in] uc6GHeRegInfo : HE regulaty info
 *
 * \return value : Enum of 6G Power mode
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomain6GPwrModeDecision(
	struct ADAPTER *prAdapter,
	uint8_t fgIsHE6GPresent,
	uint8_t uc6GHeRegInfo)
{
	enum ENUM_PWR_MODE_6G_TYPE ePwrMode6G = PWR_MODE_6G_LPI;
	uint8_t ucRfTestMode = 0;

	if (fgIsHE6GPresent) {
		if (uc6GHeRegInfo == HE_REG_INFO_LOW_POWER_INDOOR) {
			ePwrMode6G = PWR_MODE_6G_LPI;
		} else if (uc6GHeRegInfo == HE_REG_INFO_STANDARD_POWER) {
			ePwrMode6G = PWR_MODE_6G_SP;
		} else if (uc6GHeRegInfo == HE_REG_INFO_VERY_LOW_POWER) {
			ePwrMode6G = PWR_MODE_6G_VLP;
		} else if (uc6GHeRegInfo == HE_REG_INFO_INDOOR_STANDARD_POWER) {
			ePwrMode6G = PWR_MODE_6G_LPI_SP;
		} else {
			/* not sure which mode INDOOR_ENABLED and INDOOR_SP
			 * should be used, follow the same logic
			 */
			ePwrMode6G = PWR_MODE_6G_LPI;
		}
	} else {
#if WLAN_INCLUDE_SYS
		ucRfTestMode = prAdapter->ucRfTestMode;

		/* .psm.info = 1, ucRfTestMode = 0, nomral mode (LPI/VLP)
		 * .psm.info = 0, ucRfTestMode = 1, test mode (Only LPI)
		 * .psm.info = 2, ucRfTestMode = 2, test mode (LPI/VLP)
		 * .psm.info = 3, ucRfTestMode = 3, test mode (only VLP)
		 * .psm.info = 4, ucRfTestMode = 4, test mode (only SP)
		 */
		if (ucRfTestMode == 3)
			ePwrMode6G = PWR_MODE_6G_VLP;
		else if (ucRfTestMode == 4)
			ePwrMode6G = PWR_MODE_6G_SP;
		else
#endif
			ePwrMode6G = PWR_MODE_6G_LPI;
	}

	DBGLOG(RLM, TRACE,
		"HE6GPre[%d]HeRegInfo[%d]6GPwrMode[%d]RfTestMode[%d]\n",
		fgIsHE6GPresent,
		uc6GHeRegInfo,
		ePwrMode6G,
		ucRfTestMode);

	return ePwrMode6G;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use check whether the country/channel is support the
 *        current 6G power mode or not.
 *
 * \param[in] u2CountryCode : Country code
 * \param[in] eBand : RF Band index
 * \param[in] ucPriCh : Primary channel
 * \param[in] ucBw : Channel bandwidth
 * \param[in] eMode : Enum of 6G Power mode
 * \param[in] pfgSupport : Pointer of flag to indicate the support or not for
 *                         STA country
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomain6GPwrModeCountrySupportChk(
	uint16_t u2CntryCode,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucBw,
	enum ENUM_PWR_MODE_6G_TYPE eMode,
	uint8_t *pfgSupport)
{
	uint8_t fgNull = (u2CntryCode == COUNTRY_CODE_NULL) ? TRUE : FALSE;
	uint8_t ucCenterCh;
	uint16_t u2FreqRange, u2TblCntryCode, i;
	uint32_t u4CenterFreq, u4StartFreq, u4EndFreq;
	struct COUNTRY_PWR_MODE_6G_SUPPORT_TABLE *pCntryTbl = NULL;
	struct COUNTRY_PWR_MODE_6G_SUPPORT_TABLE *pDefault = NULL;
#if WLAN_INCLUDE_SYS
	uint8_t ucRfTestMode = 0;
#endif

	if ((eBand != BAND_6G) || (ucBw >= MAX_BW_UNKNOWN) ||
		(eMode >= PWR_MODE_6G_NUM) || (!pfgSupport)) {
		DBGLOG(RLM, ERROR,
			"Invalid band[%d]bw[%d]mode[%d]pfgSupport[%s]",
			eBand,
			eMode,
			ucBw,
			!pfgSupport ? "NULL" : "Valid");
		return WLAN_STATUS_INVALID_DATA;
	}

#if WLAN_INCLUDE_SYS

	ucRfTestMode = prAdapter->ucRfTestMode;

	/* .psm.info = 1, ucRfTestMode = 0, nomral mode (LPI/VLP/SP)
	 * .psm.info = 0, ucRfTestMode = 1, test mode (Only LPI)
	 * .psm.info = 2, ucRfTestMode = 2, test mode (LPI/VLP/SP)
	 * .psm.info = 3, ucRfTestMode = 3, test mode (only VLP)
	 * .psm.info = 4, ucRfTestMode = 4, test mode (only SP)
	 */
	if (ucRfTestMode == 1 && eMode != PWR_MODE_6G_LPI) {
		*pfgSupport = FALSE;
		return WLAN_STATUS_SUCCESS;
	} else if (ucRfTestMode == 3 && eMode != PWR_MODE_6G_VLP) {
		*pfgSupport = FALSE;
		return WLAN_STATUS_SUCCESS;
	} else if (ucRfTestMode == 4 && eMode != PWR_MODE_6G_SP) {
		*pfgSupport = FALSE;
		return WLAN_STATUS_SUCCESS;
	}
#endif

	if (rlmDomainGetBwFreqRang(ucBw, &u2FreqRange) != WLAN_STATUS_SUCCESS) {
		*pfgSupport = FALSE;
		return WLAN_STATUS_INVALID_DATA;
	}

	ucCenterCh = nicGetCenterCh(BAND_6G, ucPriCh, CHNL_EXT_SCN, ucBw);
	u4CenterFreq = nicGetCenterChFreq(BAND_6G, ucPriCh, CHNL_EXT_SCN, ucBw);

	if (ucCenterCh == 0 || u4CenterFreq == 0) {
		*pfgSupport = FALSE;
		return WLAN_STATUS_INVALID_DATA;
	}

	u4StartFreq = u4CenterFreq - (u2FreqRange / 2);
	u4EndFreq = u4CenterFreq + (u2FreqRange / 2);

	for (i = 0; i < COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED_SIZE; i++) {
		WLAN_GET_FIELD_BE16(
		&COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED[i].aucCountryCode[0],
		&u2TblCntryCode);

		if (u2TblCntryCode == u2CntryCode) {
			/* Found */
			pCntryTbl = &COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED[i];
			break;
		}

		if (u2TblCntryCode == COUNTRY_CODE_NULL)
			pDefault = &COUNTRY_PWR_MODE_6G_SUPPORT_TBL_USED[i];
	}

	if (pCntryTbl == NULL) {
		if (pDefault != NULL) {
			pCntryTbl = pDefault;
			DBGLOG(RLM, TRACE, "(%c%c)use default\n",
				fgNull ? '0' : ((u2CntryCode & 0xff00) >> 8),
				fgNull ? '0' : ((u2CntryCode & 0xff00)));
		} else {
			*pfgSupport = FALSE;
			DBGLOG(RLM, ERROR,
				"(%c%c) no found & no default,return[%d]\n",
				fgNull ? '0' : ((u2CntryCode & 0xff00) >> 8),
				fgNull ? '0' : ((u2CntryCode & 0xff00)),
				*pfgSupport);
			return WLAN_STATUS_FAILURE;
		}
	}

	*pfgSupport = TRUE;

	if (ucCenterCh == 2) {
		if (rlmDomain6GPwrModeSupportStatusGet(
				pCntryTbl, 0, TRUE, eMode) == FALSE)
			*pfgSupport = FALSE;
	} else {
		for (i = 0; i < SUBBAND_6G_NUM; i++) {
			if (IS_FREQ_RANGE_OVERLAP_6G_UNII_BAND(
				i, u4StartFreq, u4EndFreq)) {
				if (rlmDomain6GPwrModeSupportStatusGet(
					pCntryTbl, i, FALSE, eMode) == FALSE) {
					*pfgSupport = FALSE;
					break;
				}
			}
		}
	}

	DBGLOG(RLM, TRACE,
		"(%c%c)Band[%d]PriCh[%d]Bw[%d]CenterCh[%d]CernterFreq[%d]StartFreq[%d]EndFreq[%d]Mode[%d]Support[%d]\n",
		fgNull ? '0' : ((u2CntryCode & 0xff00) >> 8),
		fgNull ? '0' : (u2CntryCode & 0x00ff),
		eBand,
		ucPriCh,
		ucBw,
		ucCenterCh,
		u4CenterFreq,
		u4StartFreq,
		u4EndFreq,
		eMode,
		*pfgSupport);

	return WLAN_STATUS_SUCCESS;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get 6G power mode support status
 *
 * \param[in] prSupportTbl : 6G power mode support table
 * \param[in] u1BandIdx : 6G Subband index
 * \param[in] fgIsCh2 : Is channel 6G 2 or not
 * \param[in] eMode : Enum of 6G Power mode
 *
 * \return 6G power mode support status
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomain6GPwrModeSupportStatusGet(
	struct COUNTRY_PWR_MODE_6G_SUPPORT_TABLE *prSupportTbl,
	uint8_t u1Band,
	uint8_t fgIsCh2,
	enum ENUM_PWR_MODE_6G_TYPE eMode
)
{
	uint8_t fgSupport = FALSE;

	if (eMode == PWR_MODE_6G_SP) {
		/* [0]:SP */
		if (fgIsCh2) {
			fgSupport =
			prSupportTbl->rCh2Support.fgPwrMode6GSupport[0];
		} else {
			fgSupport =
			prSupportTbl->rSubBand[u1Band].fgPwrMode6GSupport[0];
		}
	} else if (eMode == PWR_MODE_6G_VLP) {
		/* [2]:VLP */
		if (fgIsCh2) {
			fgSupport =
			prSupportTbl->rCh2Support.fgPwrMode6GSupport[2];
		} else {
			fgSupport =
			prSupportTbl->rSubBand[u1Band].fgPwrMode6GSupport[2];
		}
	} else if (eMode == PWR_MODE_6G_LPI_SP) {
		/* [0]:SP | [1]:LPI */
		if (fgIsCh2) {
			fgSupport =
			(prSupportTbl->rCh2Support.fgPwrMode6GSupport[0] |
			prSupportTbl->rCh2Support.fgPwrMode6GSupport[1]);
		} else {
			fgSupport =
			(prSupportTbl->rSubBand[u1Band].fgPwrMode6GSupport[0] |
			prSupportTbl->rSubBand[u1Band].fgPwrMode6GSupport[1]);
		}
	} else {
		/* [1]:LPI */
		if (fgIsCh2) {
			fgSupport =
			prSupportTbl->rCh2Support.fgPwrMode6GSupport[1];
		} else {
			fgSupport =
			prSupportTbl->rSubBand[u1Band].fgPwrMode6GSupport[1];
		}
	}

	DBGLOG(RLM, TRACE,
		"Band[%d]IsCh2[%d]PwrMode[%d]Support[%d]LPI_SP[%d]SP[%d]LPI[%d]VLP[%d]",
		u1Band,
		fgIsCh2,
		eMode,
		fgSupport,
		PWR_MODE_6G_LPI_SP,
		PWR_MODE_6G_SP,
		PWR_MODE_6G_LPI,
		PWR_MODE_6G_VLP);

	return fgSupport;
}
#if (CFG_SUPPORT_CE_6G_PWR_REGULATIONS == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to get 6G power mode, default use LPI
 *
 * \param[in] prAdapter
 *
 * \return value : 6G power mode
 */
/*----------------------------------------------------------------------------*/
static uint8_t rlmDomainGetCurr6GPwrMode(struct ADAPTER *prAdapter)
{
	uint8_t ucBssIdx = 0;
	struct BSS_INFO *prBssInfo;
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode = 0;
	uint8_t fgUseDefault = TRUE;

	for (ucBssIdx = 0; ucBssIdx < MAX_BSSID_NUM; ucBssIdx++) {
		prBssInfo = prAdapter->aprBssInfo[ucBssIdx];
		/* 1. For normal mode will check whether the net is active or
		 *    not but test mode will not check
		 * 2. 6G power mode priority: LPI > SP
		 */
		if ((((prAdapter->fgTestMode != TRUE) &&
			(prBssInfo->fgIsNetActive)) ||
			(prAdapter->fgTestMode == TRUE)) &&
			(prBssInfo->eBand == BAND_6G) &&
			(prAdapter->e6GPwrMode[ucBssIdx] >= e6GPwrMode) &&
			(prAdapter->e6GPwrMode[ucBssIdx] < PWR_MODE_6G_VLP)) {
			e6GPwrMode = prAdapter->e6GPwrMode[ucBssIdx];
			fgUseDefault = FALSE;
			DBGLOG(RLM, LOUD, "Valid BSS[%u]6GPwrMode[%u]Final[%u]",
				ucBssIdx,
				prAdapter->e6GPwrMode[ucBssIdx],
				e6GPwrMode);
		}
	}

	if (fgUseDefault)
		return PWR_MODE_6G_LPI; /* default mode */
	else
		return e6GPwrMode;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is used to check whether the Config and TxPwrLimit file
 *       support the current 6G power mode or not.
 *
 * \param[in] prAdapter
 * \param[in] e6GPwrMode : Enum of 6G Power mode
 * \param[in] pfgSupport : Pointer of flag to indicate the support or not
 *
 * \return value : Enum of supported 6G Power mode
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomain6GPwrModeCfgSupportChk(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode,
	uint8_t *pfgSupport)
{
	struct WIFI_VAR *prWifiVar;
	enum ENUM_PWR_MODE_6G_TYPE e6FPwrModeFinal;

	if (!prAdapter) {
		DBGLOG(RLM, ERROR, "prAdapter is NULL\n");
		*pfgSupport = FALSE;
		return e6GPwrMode;
	}

	if (e6GPwrMode >= PWR_MODE_6G_NUM) {
		*pfgSupport = FALSE;
		DBGLOG(RLM, ERROR, "Invalid 6GPwrMode[%u]", e6GPwrMode);
		return e6GPwrMode;
	}

	prWifiVar = &prAdapter->rWifiVar;
	if (prWifiVar->fgPwrRdutMd) {
		*pfgSupport = TRUE;
		DBGLOG(RLM, TRACE, "PwrRdutMd is enabled");
		return e6GPwrMode;
	}

	if ((e6GPwrMode < PWR_MODE_6G_SP)
		|| (e6GPwrMode > PWR_MODE_6G_VLP)) {
		e6FPwrModeFinal = e6GPwrMode;
		*pfgSupport = TRUE;
	} else if ((e6GPwrMode == PWR_MODE_6G_LPI)
		&& (prWifiVar->fgLpiSup6G)) {
		e6FPwrModeFinal = PWR_MODE_6G_LPI;
		*pfgSupport = TRUE;
	} else if ((e6GPwrMode == PWR_MODE_6G_SP)
		&& (prAdapter->fg6GSupSpFile)
		&& (prWifiVar->fgSpSup6G)) {
		e6FPwrModeFinal = PWR_MODE_6G_SP;
		*pfgSupport = TRUE;
	} else if ((e6GPwrMode <= PWR_MODE_6G_VLP)
		&& (prAdapter->fg6GSupVlpFile)
		&& (prWifiVar->fgVlpSup6G)) {
		e6FPwrModeFinal = PWR_MODE_6G_VLP;
		*pfgSupport = TRUE;
	} else {
		e6FPwrModeFinal = e6GPwrMode;
		*pfgSupport = FALSE;
	}

	DBGLOG(RLM, TRACE,
	"PwrMode[%u->%u]File[%u %u]Cfg[%u %u %u]RdutMd[%u]Support[%u]\n",
		e6GPwrMode,
		e6FPwrModeFinal,
		prAdapter->fg6GSupSpFile,
		prAdapter->fg6GSupVlpFile,
		prWifiVar->fgSpSup6G,
		prWifiVar->fgVlpSup6G,
		prWifiVar->fgLpiSup6G,
		prWifiVar->fgPwrRdutMd,
		*pfgSupport);

	return e6FPwrModeFinal;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is used to check whether the Config, TxPwrLimit file
 *       and Country support the current 6G power mode or not.
 *
 * \param[in] prAdapter
 * \param[in] e6GPwrMode : Enum of 6G Power mode
 * \param[in] pe6GPwrModeFinal : Enum of 6G Power mode
 * \param[in] eBand : RF Band index
 * \param[in] ucChnlNum : Center Channel
 * \param[in] ucBw : Bandwidth
 *
 * \return 6G power mode support status
 */
/*----------------------------------------------------------------------------*/
u_int8_t rlmDomain6GPwrModeSupportChk(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrMode,
	enum ENUM_PWR_MODE_6G_TYPE *pe6GPwrModeFinal,
	enum ENUM_BAND eBand,
	uint8_t ucChnlNum,
	uint8_t ucBw)
{
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeFinal;
	uint32_t u4Status = WLAN_STATUS_SUCCESS;
	uint16_t u2CountryCode = 0;
	u_int8_t fgPwrMode6GSupport = TRUE;

	if (prAdapter->rWifiVar.fgPwrRdutMd) {
		DBGLOG(RLM, TRACE, "PwrRdutMd is enabled");
		return TRUE;
	}

	e6GPwrModeFinal =
	rlmDomain6GPwrModeCfgSupportChk(
			prAdapter,
			e6GPwrMode,
			&fgPwrMode6GSupport);

	if (pe6GPwrModeFinal)
		*pe6GPwrModeFinal = e6GPwrModeFinal;

	if (fgPwrMode6GSupport == FALSE) {
		DBGLOG(RLM, TRACE,
		"Config isn't support PwrMode[%u] in 6GHz band\n",
		e6GPwrMode);
		return FALSE;
	}

	u2CountryCode = rlmDomainReverseAlpha2(
		prAdapter->rWifiVar.u2CountryCode);
	u4Status = rlmDomain6GPwrModeCountrySupportChk(
			u2CountryCode,
			eBand,
			ucChnlNum,
			ucBw,
			e6GPwrModeFinal,
			&fgPwrMode6GSupport);

	if (u4Status == WLAN_STATUS_SUCCESS
		&& fgPwrMode6GSupport == FALSE) {
		DBGLOG(RLM, TRACE,
		"Country(%c%c) channel[%u] can't support PwrMode[%u]\n",
			((u2CountryCode & 0xff00) >> 8),
			(u2CountryCode & 0x00ff),
			ucChnlNum,
			e6GPwrModeFinal);
		return FALSE;
	}

	return TRUE;
}
#endif /* CFG_SUPPORT_CE_6G_PWR_REGULATIONS */
#endif /* CFG_SUPPORT_WIFI_6G_PWR_MODE */

#endif

static struct mtk_regd_control *rlmGetRegdControl(struct GLUE_INFO *prGlueInfo)
{
	if (!prGlueInfo)
		return NULL;

	return &prGlueInfo->rMtkRegdControl;
}

u_int8_t regd_is_single_sku_en(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return FALSE;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	return prRegdControl->en;
#else
	return FALSE;
#endif
}

#if (CFG_SUPPORT_SINGLE_SKU == 1)
enum regd_state rlmDomainGetCtrlState(struct GLUE_INFO *prGlueInfo)
{
	struct mtk_regd_control *prRegdControl = rlmGetRegdControl(prGlueInfo);

	return prRegdControl->state;
}


void rlmDomainResetActiveChannel(struct GLUE_INFO *prGlueInfo)
{
	struct mtk_regd_control *prRegdControl = NULL;

	prRegdControl = rlmGetRegdControl(prGlueInfo);

	prRegdControl->n_channel_active_2g = 0;
	prRegdControl->n_channel_active_5g = 0;
	prRegdControl->n_channel_active_6g = 0;
}

void rlmDomainAddActiveChannel(struct ADAPTER *prAdapter, u8 band)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	if (band == KAL_BAND_2GHZ)
		prRegdControl->n_channel_active_2g += 1;
	else if (band == KAL_BAND_5GHZ)
		prRegdControl->n_channel_active_5g += 1;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (band == KAL_BAND_6GHZ)
		prRegdControl->n_channel_active_6g += 1;
#endif
}

u8 rlmDomainGetActiveChannelCount(struct ADAPTER *prAdapter, u8 band)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	if (band == KAL_BAND_2GHZ)
		return prRegdControl->n_channel_active_2g;
	else if (band == KAL_BAND_5GHZ)
		return prRegdControl->n_channel_active_5g;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (band == KAL_BAND_6GHZ)
		return prRegdControl->n_channel_active_6g;
#endif
	else
		return 0;
}

struct CMD_DOMAIN_CHANNEL *rlmDomainGetActiveChannels(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return NULL;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	return prRegdControl->channels;
}

void rlmDomainSetDefaultCountryCode(struct GLUE_INFO *prGlueInfo)
{
	struct mtk_regd_control *prRegdControl = rlmGetRegdControl(prGlueInfo);

	prRegdControl->alpha2 = COUNTRY_CODE_WW;
}

void rlmDomainResetCtrlInfo(struct GLUE_INFO *prGlueInfo, u_int8_t force)
{
	struct mtk_regd_control *prRegdControl = rlmGetRegdControl(prGlueInfo);

	if ((prRegdControl->state == REGD_STATE_UNDEFINED) ||
	    (force == TRUE)) {
		memset(prRegdControl, 0, sizeof(struct mtk_regd_control));

		prRegdControl->state = REGD_STATE_INIT;

		rlmDomainSetDefaultCountryCode(prGlueInfo);
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

bool rlmDomainIsSameCountryCode(
	struct ADAPTER *prAdapter, char *alpha2, u8 size_of_alpha2)
{
	u8 idx;
	u32 alpha2_hex = 0;

	for (idx = 0; idx < size_of_alpha2; idx++)
		alpha2_hex |= (alpha2[idx] << (idx * 8));

	if (rlmDomainGetCountryCode(prAdapter) == alpha2_hex)
		return TRUE;
	else
		return FALSE;
}

void rlmDomainSetCountryCode(
		struct ADAPTER *prAdapter, char *alpha2, u8 size_of_alpha2)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;
	u8 max;
	u8 buf_size;

	if (!prAdapter)
		return;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	buf_size = sizeof(prRegdControl->alpha2);
	max = (buf_size < size_of_alpha2) ? buf_size : size_of_alpha2;

	prRegdControl->alpha2 = rlmDomainAlpha2ToU32(alpha2, max);
}

void rlmDomainSetDfsRegion(struct ADAPTER *prAdapter, u8 dfs_region)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	prRegdControl->dfs_region = dfs_region;
}

u8 rlmDomainGetDfsRegion(struct ADAPTER *prAdapter)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	return prRegdControl->dfs_region;
}

void rlmDomainSetDfsDbdcBand(
		struct ADAPTER *prAdapter, enum ENUM_MBMC_BN eDBDCBand)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	prRegdControl->eDBDCBand = eDBDCBand;
}

enum ENUM_MBMC_BN rlmDomainGetDfsDbdcBand(struct ADAPTER *prAdapter)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	return prRegdControl->eDBDCBand;
}

void rlmDomainSetTempCountryCode(
		struct ADAPTER *prAdapter, char *alpha2, u8 size_of_alpha2)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);
	u8 idx, max;
	u8 buf_size;

	buf_size = sizeof(prRegdControl->tmp_alpha2);
	max = (buf_size < size_of_alpha2) ? buf_size : size_of_alpha2;

	prRegdControl->tmp_alpha2 = 0;

	for (idx = 0; idx < max; idx++)
		prRegdControl->tmp_alpha2 |= (alpha2[idx] << (idx * 8));

}

enum regd_state rlmDomainStateTransition(
		struct ADAPTER *prAdapter, enum regd_state request_state)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	enum regd_state next_state, old_state;
	bool the_same = 0;

	old_state = prRegdControl->state;
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
		__func__, next_state, prRegdControl->state);

	prRegdControl->state = next_state;

	return prRegdControl->state;
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

	rlmDomainResetActiveChannel(prGlueInfo);

	pCh = rlmDomainGetActiveChannels(prAdapter);

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

void rlmExtractChannelInfo(struct ADAPTER *prAdapter,
			   u32 max_ch_count,
			   struct CMD_DOMAIN_ACTIVE_CHANNEL_LIST *prBuff)
{
	u32 ch_count, idx;
	struct CMD_DOMAIN_CHANNEL *pCh;

	prBuff->u1ActiveChNum2g =
		rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_2GHZ);
	prBuff->u1ActiveChNum5g =
		rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_5GHZ);
#if (CFG_SUPPORT_WIFI_6G == 1)
	prBuff->u1ActiveChNum6g =
		rlmDomainGetActiveChannelCount(prAdapter, KAL_BAND_6GHZ);
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

		pCh->u2ChNum =
			(rlmDomainGetActiveChannels(prAdapter) + idx)->u2ChNum;
		pCh->eFlags =
			(rlmDomainGetActiveChannels(prAdapter) + idx)->eFlags;
	}

}

const void *rlmDomainSearchRegdomainFromLocalDataBase(
	char *alpha2)
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


const void *rlmDomainGetLocalDefaultRegd(void)
{
#if (CFG_SUPPORT_SINGLE_SKU_LOCAL_DB == 1)
	return kalGetDefaultRegWW();
#else
	return NULL;
#endif
}
struct GLUE_INFO *rlmDomainGetGlueInfo(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	return prRegdControl->pGlueInfo;
}

bool rlmDomainIsEfuseUsed(struct ADAPTER *prAdapter)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	return prRegdControl->isEfuseCountryCodeUsed;
}

uint8_t rlmDomainGetChannelBw(
	struct ADAPTER *prAd, enum ENUM_BAND eBand, uint8_t channelNum)
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

	end_idx = rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ)
#if (CFG_SUPPORT_WIFI_6G == 1)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_6GHZ)
#endif
		;

	for (ch_idx = start_idx; ch_idx < end_idx; ch_idx++) {
		pCh = (rlmDomainGetActiveChannels(prAd) + ch_idx);

		if (ch_idx <
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ))
			eChBand = BAND_2G4;
#if (CFG_SUPPORT_WIFI_6G == 1)
		else if (ch_idx < rlmDomainGetActiveChannelCount(
							prAd, KAL_BAND_2GHZ)
			+ rlmDomainGetActiveChannelCount(prAd, KAL_BAND_5GHZ))
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
		if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_160MHZ))
			channelBw = MAX_BW_80MHZ;
		if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_80MHZ))
			channelBw = MAX_BW_40MHZ;
		if (kalIsChFlagMatch(pCh->eFlags, CHAN_NO_HT40)) {
			channelBw = MAX_BW_20MHZ;
			break;
		}

		/* To prevent using illegal max bandwidth by channel
		 * flag in reg domain : IEEE80211_CHAN_NO_HT40PLUS、
		 * IEEE80211_CHAN_NO_HT40MINUS
		 *
		 * IEEE80211_CHAN_NO_HT40 = IEEE80211_CHAN_NO_HT40PLUS |
		 * IEEE80211_CHAN_NO_HT40MINUS
		 *
		 * For example,
		 * IEEE80211_CHAN_NO_HT40 can not limit max bandwidth for
		 * 5G chnl 116 in CA to MAX_BW_20MHZ, because this channel
		 * flag only has IEEE80211_CHAN_NO_HT40PLUS not
		 * IEEE80211_CHAN_NO_HT40
		 */
		if (ch_idx >=
			rlmDomainGetActiveChannelCount(prAd, KAL_BAND_2GHZ)) {
			uint32_t u4ChnlSeq;
			int32_t i4StartIdx;
#if (CFG_SUPPORT_WIFI_6G == 1)
			/* For example,
			 * 5G chnl 116、6G chnl 33 -> case 1
			 * 5G chnl 120、6G chnl 37 -> case 2
			 * 5G chnl 124、6G chnl 41 -> case 3
			 * 5G chnl 128、6G chnl 45 -> case 0
			 * For 6G band : +1 is to align the cases with 5G band
			 */
			if (eChBand == BAND_6G) {
				u4ChnlSeq = ((pCh->u2ChNum >> 2) + 1) & 0x3;
				i4StartIdx =
					rlmDomainGetActiveChannelCount(
						prAd, KAL_BAND_2GHZ) +
					rlmDomainGetActiveChannelCount(
						prAd, KAL_BAND_5GHZ);
			}
			else
#endif
			{
				u4ChnlSeq = (pCh->u2ChNum >> 2) & 0x3;
				i4StartIdx =
					rlmDomainGetActiveChannelCount(
						prAd, KAL_BAND_2GHZ);
			}

			/* Limit MAX_BW_40MHz and above to MAX_BW_20MHZ */
			if (channelBw > MAX_BW_20MHZ) {
				/* Check flag for 5G chnl 116 or 124 */
				if ((u4ChnlSeq & 0x1) &&
					kalIsChFlagMatch(pCh->eFlags,
					CHAN_NO_HT40PLUS))
					channelBw = MAX_BW_20MHZ;
				/* Check flag for 5G chnl 120 or 128 */
				else if ((!(u4ChnlSeq & 0x1)) &&
					kalIsChFlagMatch(pCh->eFlags,
					CHAN_NO_HT40MINUS))
					channelBw = MAX_BW_20MHZ;
			}

			/* Limit MAX_BW_80MHz and above to MAX_BW_40MHZ */
			if (channelBw > MAX_BW_40MHZ) {
				struct CMD_DOMAIN_CHANNEL *pAdj20Chnl = NULL;
				struct CMD_DOMAIN_CHANNEL *pAdj40Chnl = NULL;
				int32_t offset = 0;
				int32_t i4ChIdx = ch_idx;
				int32_t i4EndIdx = end_idx;

				switch (u4ChnlSeq) {
				case 1:
					/* 5G chnl 116 to check chnl 120 flag */
					offset = 1;
					if ((i4ChIdx + offset) < i4EndIdx)
						pAdj20Chnl = (
						rlmDomainGetActiveChannels(
							prAd) +
						(i4ChIdx + offset));
					kal_fallthrough;
				case 2:
					/* 5G chnl 116 to check chnl 124 flag or
					 * 5G chnl 120 to check chnl 124 flag
					 */
					offset++;
					break;
				case 0:
					/* 5G chnl 128 to check chnl 124 flag */
					offset = -1;
					if ((i4ChIdx + offset) >= i4StartIdx)
						pAdj20Chnl = (
						rlmDomainGetActiveChannels(
							prAd) +
						(i4ChIdx + offset));
					kal_fallthrough;
				case 3:
					/* 5G chnl 128 to check chnl 120 flag or
					 * 5G chnl 124 to check chnl 120 flag
					 */
					offset--;
					kal_fallthrough;
				default:
					break;
				}

				if (((i4ChIdx + offset) >= i4StartIdx) &&
					((i4ChIdx + offset) < i4EndIdx))
					pAdj40Chnl = (
						rlmDomainGetActiveChannels(
							prAd) +
						(i4ChIdx + offset));

				if ((pAdj20Chnl) &&
					(kalIsChFlagMatch(
					pAdj20Chnl->eFlags, CHAN_NO_HT40PLUS) ||
					kalIsChFlagMatch(
					pAdj20Chnl->eFlags, CHAN_NO_HT40MINUS)))
					channelBw = MAX_BW_40MHZ;
				else if ((pAdj40Chnl) &&
					(kalIsChFlagMatch(
					pAdj40Chnl->eFlags, CHAN_NO_HT40PLUS) ||
					kalIsChFlagMatch(
					pAdj40Chnl->eFlags, CHAN_NO_HT40MINUS)))
					channelBw = MAX_BW_40MHZ;
			}
		}
		break;
	}

	DBGLOG(RLM, TRACE, "ch=%d, BW=%d\n", channelNum, channelBw);
	return channelBw;
}
#endif

uint32_t rlmDomainExtractSingleSkuInfoFromFirmware(struct ADAPTER *prAdapter,
						   uint8_t *pucEventBuf)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);
	struct SINGLE_SKU_INFO *prSkuInfo =
			(struct SINGLE_SKU_INFO *) pucEventBuf;

	prRegdControl->en = TRUE;

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

		rlmDomainSetCountryCode(prAdapter,
			(char *) &prSkuInfo->u4EfuseCountryCode,
			sizeof(prSkuInfo->u4EfuseCountryCode));

		prRegdControl->isEfuseCountryCodeUsed = TRUE;
	}
#endif

	return WLAN_STATUS_SUCCESS;
}

void rlmDomainSendInfoToFirmware(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	if (!regd_is_single_sku_en(prAdapter))
		return; /*not support single sku*/

	prRegdControl->pGlueInfo = prAdapter->prGlueInfo;
	rlmDomainSetCountry(prAdapter, 1);
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
#if (CFG_SUPPORT_UNII4 == 1)
		case 169:
		case 177:
#endif
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
#if (CFG_SUPPORT_UNII4 == 1)
		case 165:
		case 173:
#endif
			eSCO = CHNL_EXT_SCB;
			break;
		default:
			eSCO = CHNL_EXT_SCN;
			break;
		}
#if (CFG_SUPPORT_WIFI_6G == 1)
	} else if (band == BAND_6G) {
		if ((primary_ch & 0x7) == 1)
			eSCO = CHNL_EXT_SCA;
		else if ((primary_ch & 0x7) == 5)
			eSCO = CHNL_EXT_SCB;
		else
			eSCO = CHNL_EXT_SCN;
#endif
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

void rlmDomainOidSetCountry(struct ADAPTER *prAdapter,
	char *country,
	uint8_t size_of_country,
	uint8_t fgNeedHoldRtnlLock)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)

	if (!regd_is_single_sku_en(prAdapter)) {
		DBGLOG(RLM, ERROR, "regd control is not enabled\n");
		return;
	}

	if (rlmDomainIsUsingLocalRegDomainDataBase()) {

		if (rlmDomainIsSameCountryCode(
				prAdapter, country, size_of_country)) {
			char acCountryCodeStr[MAX_COUNTRY_CODE_LEN + 1] = {0};

			rlmDomainU32ToAlpha(
				rlmDomainGetCountryCode(prAdapter),
				acCountryCodeStr);
			DBGLOG(RLM, WARN,
				"Same as current country %s, skip!\n",
				acCountryCodeStr);
			return;
		}
		rlmDomainSetCountryCode(prAdapter, country, size_of_country);
		rlmDomainSetCountry(prAdapter, fgNeedHoldRtnlLock);
	} else {
		DBGLOG(RLM, INFO,
		       "%s(): Using driver hint to query CRDA getting regd.\n",
		       __func__);
		kalRegulatoryHint(prAdapter->prGlueInfo, country);
	}
#endif
}

u32 rlmDomainGetCountryCode(struct ADAPTER *prAdapter)
{
#if (CFG_SUPPORT_SINGLE_SKU == 1)
	struct GLUE_INFO *prGlueInfo = NULL;
	struct mtk_regd_control *prRegdControl = NULL;

	if (!prAdapter)
		return 0;

	prGlueInfo = prAdapter->prGlueInfo;
	prRegdControl = rlmGetRegdControl(prGlueInfo);

	return prRegdControl->alpha2;
#else
	return 0;
#endif /* CFG_SUPPORT_SINGLE_SKU */
}

void rlmDomainAssert(struct ADAPTER *prAdapter, u_int8_t cond)
{
	/* bypass this check because single sku is not enable */
	if (!regd_is_single_sku_en(prAdapter))
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
void rlm_get_alpha2(struct ADAPTER *prAdapter, char *alpha2)
{
	struct mtk_regd_control *prRegdControl =
			rlmGetRegdControl(prAdapter->prGlueInfo);

	rlmDomainU32ToAlpha(prRegdControl->alpha2, alpha2);
}
EXPORT_SYMBOL(rlm_get_alpha2);
#endif
#endif

static uint8_t rlmDomainGetSubBandPwrLimit(
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitSubBand,
	uint32_t sub_band_idx)
{

	uint8_t u1PwrIdx = 0;
	uint8_t ucPwrLimit = 0;

	rlmDomainGetSubBandDefPwrIdx(sub_band_idx, &u1PwrIdx);
	/* cLmtBand need reset by each channel */
	ucPwrLimit = prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx];
	if (prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx]
		> MAX_TX_POWER) {
		DBGLOG(RLM, WARN,
		"SubBand[%d] Pwr(%d) > Max (%d)",
		u1PwrIdx,
		prPwrLimitSubBand->aucPwrLimitSubBand[u1PwrIdx],
		MAX_TX_POWER);
		ucPwrLimit = MAX_TX_POWER;
	}

	return ucPwrLimit;
}

#if (CFG_SUPPORT_PWR_LMT_EMI == 1)
void rlmDomainSendPwrLimitCmd(struct ADAPTER *prAdapter)
{
	struct GLUE_INFO *prGlueInfo;
	uint32_t u4Status, u4BufLen = 0;
	static const char TARGET_THREAD[] = "main_thread";

	DBGLOG(RLM, INFO, "TxPower limt EMI Type!! Thread: [%s]\n",
		KAL_GET_CURRENT_THREAD_NAME());


	prGlueInfo = prAdapter->prGlueInfo;

	if (kalStrCmp(TARGET_THREAD, KAL_GET_CURRENT_THREAD_NAME()) != 0) {
		u4Status = kalIoctl(prGlueInfo,
			wlanoidSendPwrLimitToEmi,
			NULL,
			0,
			&u4BufLen);
	} else {
		u4Status = wlanoidSendPwrLimitToEmi(prAdapter,
			NULL,
			0,
			&u4BufLen);
	}
}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
static uint32_t txPwrCtrlApplyDynMultiBandPwrSetting(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rMultiBandPwrLimitInfo)
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

			if (element->fgApplied == TRUE) {
				rlmDomainApplyDynMultiBandSettings(
					element,
					eBandIdx,
					eRF,
					&prAdapter->rMulBnData[eBandIdx][eRF],
					rMultiBandPwrLimitInfo
				);
			}

		}
	}

	return 0;
}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

uint32_t txPwrCtrlApplyDynPwrSetting(struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_TYPE eLimitType,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBand)
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

			if (element->fgApplied == TRUE) {
				g_rRlmPwrLimitHandler[eLimitType].pfApplyDynSet(
					prPerPwrLimit,
					element,
					bandedgeParam,
					eRFBand
				);
			}

		}
	}

	return 0;
}

#define TXPWR_CMD_TO 5000 /* msec */
bool rlmDomainPwrLmtEmiStatusCtrl(struct ADAPTER *prAdapter,
	enum ENUM_TX_PWR_EMI_STATUS_ACTION action)
{
	bool ret = FALSE;
	struct GLUE_INFO *prGlueInfo;
#if CFG_ENABLE_WAKE_LOCK
	KAL_WAKE_LOCK_T * rTxWakeLock =
		prAdapter->prGlueInfo->rTxPowerEmiWakeLock;
#endif
	struct TXP_LIMIT_EMI_CTRL *prTxpEmiCtrl;
	static const char * const au1PwrLmtStatusAction[] = {
		"REQUEST_CHANNEL_START",
		"REQUEST_CHANNEL_END",
		"UPDATE_CMD",
		"UPDATE_EVENT",
		"CHECK",
		"CLEAR",
		"NAN_INIT_START",
		"NAN_INIT_END",
		"NAN_TIMELINE_UPDATE_START",
		"NAN_TIMELINE_UPDATE_END",
		"CNM_RLM_SYNC_OP_PARAMS_START",
		"CNM_RLM_SYNC_OP_PARAMS_END",
	};
	prGlueInfo = prAdapter->prGlueInfo;
	prTxpEmiCtrl = &prAdapter->rTxpEmiCtrl;
	/*********************************************************************/
	/* (1)req cmd -> (2)update cmd  -> (3)req end  -> (4)update done     */
	/* count++    ->                -> count--     ->                    */
	/* wake lock  -> cache data     -> wake unlock -> send cache data    */
	/* 1.Make sure host is wakeup between (1) ~ (3).                     */
	/* 2.This design is based on pwr limit cmd/event must appear in pairs*/
	/*********************************************************************/

	DBGLOG(RLM, TRACE,
		"[In]TxPower wakelock ctrl : action :%s, counter[%d], ret:%d",
		au1PwrLmtStatusAction[action],
		prTxpEmiCtrl->i4PwrLmtLockCounter,
		ret);

	switch (action) {
	case TX_PWR_EMI_STATUS_ACTION_UPDATE_CMD:
	case TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_START:
	case TX_PWR_EMI_STATUS_ACTION_NAN_INIT_START:
	case TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_START:
	case TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_START:
		prTxpEmiCtrl->i4PwrLmtLockCounter++;
		if (prTxpEmiCtrl->i4PwrLmtLockCounter == 1) {
#if CFG_ENABLE_WAKE_LOCK
			if (!KAL_WAKE_LOCK_ACTIVE(prAdapter, rTxWakeLock)) {
				DBGLOG(RLM, TRACE, "Start wake lock!");
				KAL_WAKE_LOCK(prAdapter, rTxWakeLock);
			}
			/* start timer */
			mod_timer(&prGlueInfo->rTxPowerLimitTimer,
				jiffies + TXPWR_CMD_TO * HZ / MSEC_PER_SEC);
#endif
		}
		break;
	case TX_PWR_EMI_STATUS_ACTION_UPDATE_EVENT:
	case TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_END:
	case TX_PWR_EMI_STATUS_ACTION_NAN_INIT_END:
	case TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_END:
	case TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_END:
		prTxpEmiCtrl->i4PwrLmtLockCounter--;
		if (prTxpEmiCtrl->i4PwrLmtLockCounter <= 0) {
#if CFG_ENABLE_WAKE_LOCK
			if (KAL_WAKE_LOCK_ACTIVE(prAdapter, rTxWakeLock)) {
				DBGLOG(RLM, TRACE, "Stop wake lock!");
				KAL_WAKE_UNLOCK(prAdapter, rTxWakeLock);
			}
#endif
			/* stop timer */
#if (KERNEL_VERSION(6, 15, 0) > CFG80211_VERSION_CODE)
			del_timer(&prGlueInfo->rTxPowerLimitTimer);
#else
			timer_delete(&prGlueInfo->rTxPowerLimitTimer);
#endif
			prTxpEmiCtrl->i4PwrLmtLockCounter = 0;
			rlmDomainSendCachePwrLmtData(prAdapter);
		}
		break;
	case TX_PWR_EMI_STATUS_ACTION_CHECK:
		if (prTxpEmiCtrl->i4PwrLmtLockCounter > 0)
			ret = TRUE;
		else
			ret = FALSE;
		break;
	case TX_PWR_EMI_STATUS_ACTION_CLEAR:
#if CFG_ENABLE_WAKE_LOCK
		prTxpEmiCtrl->i4PwrLmtLockCounter = 0;
		if (KAL_WAKE_LOCK_ACTIVE(prAdapter, rTxWakeLock)) {
			DBGLOG(RLM, TRACE, "Stop wake lock!");
			KAL_WAKE_UNLOCK(prAdapter, rTxWakeLock);
		}
#endif
		txPwrConnectionDelAllLUTElement(prAdapter);
		break;
	default:
		break;
	}

	DBGLOG(RLM, TRACE,
		"[Out]TxPower wakelock ctrl : action :%s, counter[%d], ret:%d",
		au1PwrLmtStatusAction[action],
		prTxpEmiCtrl->i4PwrLmtLockCounter,
		ret);

	return ret;
}

void rlmDoaminSetPwrLmtNewDataFlag(struct ADAPTER *prAdapter, bool en)
{
	prAdapter->fgPwrLmtCacheExist = en;
}
bool rlmDoaminGetPwrLmtNewDataFlag(struct ADAPTER *prAdapter)
{
	return prAdapter->fgPwrLmtCacheExist;
}
/*----------------------------------------------------------------------------*/
/*!
 * \brief Used to store the status of cnm request/grant/abort channel status
 *
 * \param[in/out] prAdapter
 *                ucBssIndex
 *                ucTokenID
 *                fgIsReg : request channel (1)/ grant & abort channel (0)
 *
 * \return 1 : fgIsReg(1) register success, fgIsReg(0) : del success
 * \return 0 : fgIsReg(1) already register, fgIsReg(0) : not find entry
 */
/*----------------------------------------------------------------------------*/
uint8_t txPwrConnectionLUTCtrl(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID,
	uint8_t fgIsReg
)
{
	struct TXP_REQUEST_CHANNEL_ELEMENT *pElement;
	uint8_t ucHit = FALSE;

	if (fgIsReg) { /* request channel */
		pElement = txPwrConnectionFindLUTElement(prAdapter,
			ucBssIndex, ucTokenID);
		if (!pElement) {
			txPwrConnectionAddLUTElement(prAdapter,
				ucBssIndex, ucTokenID);
			ucHit = TRUE;
		} else
			ucHit = FALSE;
	} else { /* abort channel or grant channel*/
		pElement = txPwrConnectionFindLUTElement(prAdapter,
			ucBssIndex, ucTokenID);
		if (pElement) {
			txPwrConnectionDelLUTElement(prAdapter,
				ucBssIndex, ucTokenID);
			ucHit = TRUE;
		} else
			ucHit = FALSE;
	}

	DBGLOG(RLM, INFO,
		"Connection fgIsReg[%d]BssId[%d]Token[%d]Hit[%d]\n",
		fgIsReg, ucBssIndex,
		ucTokenID,
		ucHit);

	txPwrConnectionShowLUTElement(prAdapter);
	return ucHit;
}

void txPwrConnectionAddLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID
)
{
	struct TXP_REQUEST_CHANNEL_ELEMENT *prNewElement;
	struct LINK_ENTRY *prNode;

	prNewElement = (struct TXP_REQUEST_CHANNEL_ELEMENT *)kalMemAlloc(
		sizeof(struct TXP_REQUEST_CHANNEL_ELEMENT), VIR_MEM_TYPE);
	if (!prNewElement) {
		DBGLOG(RLM, ERROR,
		"alloc connection element failed\n");
		return;
	}
	prNewElement->ucBssIndex = ucBssIndex;
	prNewElement->ucTokenID = ucTokenID;
	prNode = &prNewElement->node;
	linkAddTail(prNode, &prAdapter->rTxpEmiCtrl.rReqChLUT);
}

void txPwrConnectionDelLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID
)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TXP_REQUEST_CHANNEL_ELEMENT *prCurElement = NULL;
	u_int8_t fgFind;

	LINK_FOR_EACH_SAFE(prCur, prNext, &prAdapter->rTxpEmiCtrl.rReqChLUT) {
		fgFind = FALSE;
		prCurElement = LINK_ENTRY(prCur,
			struct TXP_REQUEST_CHANNEL_ELEMENT, node);
		if (prCurElement != NULL) {
			if (prCurElement->ucBssIndex == ucBssIndex &&
				prCurElement->ucTokenID == ucTokenID) {
				linkDel(prCur);
				kalMemFree(prCurElement, VIR_MEM_TYPE,
					sizeof(TXP_REQUEST_CHANNEL_ELEMENT));
				return;
			}
		}
	}
}

void txPwrConnectionDelAllLUTElement(
	struct ADAPTER *prAdapter
)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TXP_REQUEST_CHANNEL_ELEMENT *prCurElement = NULL;

	LINK_FOR_EACH_SAFE(prCur, prNext, &prAdapter->rTxpEmiCtrl.rReqChLUT) {
		prCurElement = LINK_ENTRY(prCur,
			struct TXP_REQUEST_CHANNEL_ELEMENT, node);
		linkDel(prCur);
		if (prCurElement != NULL) {
			kalMemFree(prCurElement, VIR_MEM_TYPE,
				sizeof(TXP_REQUEST_CHANNEL_ELEMENT));
		}
	}
}

struct TXP_REQUEST_CHANNEL_ELEMENT *txPwrConnectionFindLUTElement(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucTokenID
)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TXP_REQUEST_CHANNEL_ELEMENT *prCurElement = NULL;

	LINK_FOR_EACH_SAFE(prCur, prNext, &prAdapter->rTxpEmiCtrl.rReqChLUT) {
		prCurElement = LINK_ENTRY(prCur,
			struct TXP_REQUEST_CHANNEL_ELEMENT, node);
		if (prCurElement != NULL) {
			if (prCurElement->ucBssIndex == ucBssIndex &&
				prCurElement->ucTokenID == ucTokenID)
				return prCurElement;
		}
	}
	return NULL;

}
void txPwrConnectionShowLUTElement(struct ADAPTER *prAdapter)
{
	struct LINK_ENTRY *prCur, *prNext;
	struct TXP_REQUEST_CHANNEL_ELEMENT *prCurElement = NULL;
	int count = 0;

	LINK_FOR_EACH_SAFE(prCur, prNext, &prAdapter->rTxpEmiCtrl.rReqChLUT) {
		prCurElement = LINK_ENTRY(prCur,
			struct TXP_REQUEST_CHANNEL_ELEMENT, node);
		if (prCurElement != NULL) {
			DBGLOG(RLM, INFO,
				"Connection LUT Element-%d,BSSID[%d],Token[%d]\n",
				++count, prCurElement->ucBssIndex,
				prCurElement->ucTokenID);
		}
	}
}
void rlmDomainConnectionNotifiey(
	struct ADAPTER *prAdapter,
	enum ENUM_CONNECTION_NOTIFIED_REASON reason)
{
	enum ENUM_TX_PWR_EMI_SCENARIO_TYPE type;

	DBGLOG(RLM, TRACE, "Connection notify to - Tx Power, reason :%d",
		reason);

	if (reason == CNM_REQUEST_CHANNEL)
		type = TX_PWR_EMI_SCENARIO_TYPE_CONNECTION;
	else if (reason == NAN_INIT)
		type = TX_PWR_EMI_SCENARIO_TYPE_NAN_INIT;
	else if (reason == NAN_TIMELINE_UPDATE)
		type = TX_PWR_EMI_SCENARIO_TYPE_NAN_TIMELINE_UPDATE;
	else if (reason == CNM_RLM_SYNC_OP_PARAMS)
		type = TX_PWR_EMI_SCENARIO_TYPE_CNM_RLM_SYNC_OP_PARAMS;
	else
		return;

	rlmDomainPwrLmtConnectionCMD(prAdapter, type);
}

bool rlmDomainIsFWInReadEmiDataProcess(struct ADAPTER *prAdapter)
{
	return rlmDomainPwrLmtEmiStatusCtrl(
		prAdapter, TX_PWR_EMI_STATUS_ACTION_CHECK);
}

static bool rlmDomainIsNeedToDoArbitrator(
	uint32_t channel,
	uint32_t rng_start,
	uint32_t rng_end,
	uint8_t *bandedgeParam,
	enum ENUM_TX_POWER_CTRL_CHANNEL_TYPE eChanneltype,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex
)
{
	bool fgNeedArbitrator = FALSE;

	if (eChanneltype == PWR_CTRL_CHNL_TYPE_ALL) {
		fgNeedArbitrator = true;
		return fgNeedArbitrator;
	}

	if (eRFBandIndex <= PWR_LIMIT_RF_BAND_5G) {
		switch (eChanneltype) {
		case PWR_CTRL_CHNL_TYPE_NORMAL:
			if (channel == rng_start)
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_RANGE:
			if ((channel >= rng_start) && (channel <= rng_end))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_2G4:
			if (channel <= 14)
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_5G:
			if (channel > 14)
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_BANDEDGE_2G4:
			if ((channel == *bandedgeParam) ||
				(channel == *(bandedgeParam + 1)))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_BANDEDGE_5G:
			if ((channel == *bandedgeParam + 2) ||
				(channel == *(bandedgeParam + 3)))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_5G_BAND1:
			if ((channel >= UNII1_LOWER_BOUND) &&
				(channel <= UNII1_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_5G_BAND2:
			if ((channel >= UNII2A_LOWER_BOUND) &&
				(channel <= UNII2A_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_5G_BAND3:
			if ((channel >= UNII2C_LOWER_BOUND) &&
				(channel <= UNII2C_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_5G_BAND4:
			if ((channel >= UNII3_LOWER_BOUND) &&
				(channel <= UNII3_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		default:
				break;
		}
	}

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eRFBandIndex == PWR_LIMIT_RF_BAND_6G) {
		switch (eChanneltype) {
		case PWR_CTRL_CHNL_TYPE_6G:
			fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_NORMAL:
			if (channel == rng_start)
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_BAND1:
			if ((channel >= UNII5A_LOWER_BOUND) &&
				(channel <= UNII5B_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_BAND2:
			if ((channel >= UNII6_LOWER_BOUND) &&
				(channel <= UNII6_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_BAND3:
			if ((channel >= UNII7A_LOWER_BOUND) &&
				(channel <= UNII7B_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_BAND4:
			if ((channel >= UNII8_LOWER_BOUND) &&
				(channel <= UNII8_UPPER_BOUND))
				fgNeedArbitrator = TRUE;
			break;
		case PWR_CTRL_CHNL_TYPE_6G_RANGE:
			if ((channel >= rng_start) &&
				(channel <= rng_end))
				fgNeedArbitrator = TRUE;
			break;
		default:
			break;
		}
	}
#endif /*CFG_SUPPORT_WIFI_6G*/

	return fgNeedArbitrator;
}

static enum ENUM_PWR_LIMIT_CONFIG_BASE rlmDomainPwrLmtGetConfigBaseByCh(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_RF_BAND eRfBandIndex,
	uint8_t ucCurrChnl)
{

	enum ENUM_PWR_LIMIT_CONFIG_BASE eConfigPwrLmtBase =
		PWR_LIMIT_CONFIG_BASE_2G4_5G;
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeType = PWR_MODE_6G_VLP;

	if (eRfBandIndex == PWR_LIMIT_RF_BAND_6G) {
		eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G;

		e6GPwrModeType = rlmDomainPwrLmt6GPwrModeGetByChnl(prAdapter,
			ucCurrChnl);
		if (e6GPwrModeType == PWR_MODE_6G_VLP)
			eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G_VLP;
		else if ((e6GPwrModeType == PWR_MODE_6G_SP) ||
			(e6GPwrModeType == PWR_MODE_6G_LPI_SP))
			eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G_SP;

		DBGLOG(RLM, LOUD, "pri[%d]band[%d]ch[%d]mode[%d]base[%d]\n",
			rlmDomainPwrLmtGetChannelDefine(),
			eRfBandIndex,
			ucCurrChnl,
			e6GPwrModeType,
			eConfigPwrLmtBase);
	}
#endif
	return eConfigPwrLmtBase;
}

static enum ENUM_PWR_LIMIT_CONFIG_BASE rlmDomainPwrLmtGetConfigBase(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_RF_BAND eRfBandIndex)
{

	enum ENUM_PWR_LIMIT_CONFIG_BASE eConfigPwrLmtBase =
		PWR_LIMIT_CONFIG_BASE_2G4_5G;
#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	enum ENUM_PWR_MODE_6G_TYPE e6GPwrModeType = PWR_MODE_6G_VLP;

	if (eRfBandIndex == PWR_LIMIT_RF_BAND_6G) {
		eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G;

		e6GPwrModeType = rlmDomainPwrLmt6GPwrModeGet(prAdapter);
		if (e6GPwrModeType == PWR_MODE_6G_VLP)
			eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G_VLP;
		else if ((e6GPwrModeType == PWR_MODE_6G_SP) ||
			(e6GPwrModeType == PWR_MODE_6G_LPI_SP))
			eConfigPwrLmtBase = PWR_LIMIT_CONFIG_BASE_6G_SP;
	}
#endif
	return eConfigPwrLmtBase;
}

static enum ENUM_BAND rlmDomainConvertRFBandEnum(
	enum ENUM_PWR_LIMIT_RF_BAND eRFBandIndex)
{
	if (eRFBandIndex == PWR_LIMIT_RF_BAND_2G4)
		return BAND_2G4;
	else if (eRFBandIndex == PWR_LIMIT_RF_BAND_5G)
		return BAND_5G;
#if (CFG_SUPPORT_WIFI_6G == 1)
	else if (eRFBandIndex == PWR_LIMIT_RF_BAND_6G)
		return BAND_6G;
#endif
	else
		return BAND_NULL;
}

void rlmDomainPatchPwrLimitType(void)
{
	PWR_LMT_2G_INFO_REGISTER(
		PWR_LIMIT_TYPE_LEGACY, 1, PWR_LIMIT_PROTOCOL_LEGACY);
	PWR_LMT_2G_INFO_REGISTER(
		PWR_LIMIT_TYPE_HE, 1, PWR_LIMIT_PROTOCOL_HE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_LMT_2G_INFO_REGISTER(
		PWR_LIMIT_TYPE_EHT, 1, PWR_LIMIT_PROTOCOL_EHT);
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/

	PWR_LMT_5G_INFO_REGISTER(
		PWR_LIMIT_TYPE_LEGACY, 1, PWR_LIMIT_PROTOCOL_LEGACY);
	PWR_LMT_5G_INFO_REGISTER(
		PWR_LIMIT_TYPE_HE, 1, PWR_LIMIT_PROTOCOL_HE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_LMT_5G_INFO_REGISTER(
		PWR_LIMIT_TYPE_EHT, 1, PWR_LIMIT_PROTOCOL_EHT);
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	PWR_LMT_MULTIBAND_2G_INFO_REGISTER(1);
	PWR_LMT_MULTIBAND_5G_INFO_REGISTER(1);
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#if (CFG_SUPPORT_WIFI_6G == 1)
	PWR_LMT_6G_INFO_REGISTER(
		PWR_LIMIT_TYPE_LEGACY, 1, PWR_LIMIT_PROTOCOL_LEGACY);
	PWR_LMT_6G_INFO_REGISTER(
		PWR_LIMIT_TYPE_HE, 1, PWR_LIMIT_PROTOCOL_HE);
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	PWR_LMT_6G_INFO_REGISTER(
		PWR_LIMIT_TYPE_EHT, 1, PWR_LIMIT_PROTOCOL_EHT);
#endif /*CFG_SUPPORT_PWR_LIMIT_EHT*/
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	PWR_LMT_MULTIBAND_6G_INFO_REGISTER(1);
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
#endif /*CFG_SUPPORT_WIFI_6G*/
}

void rlmDomainSendCachePwrLmtData(struct ADAPTER *prAdapter)
{
	if (rlmDoaminGetPwrLmtNewDataFlag(prAdapter) == TRUE) {
		DBGLOG(NIC, DEBUG, "Send Tx Power Limit Cache Data !!\n");
		rlmDomainWritePwrLimitToEmi(prAdapter);
		rlmDoaminSetPwrLmtNewDataFlag(prAdapter, FALSE);
	} else {
		DBGLOG(NIC, TRACE, "No cache data !!\n");
	}

}
void rlmDomainPowerLimitEmiEvent(struct ADAPTER *prAdapter,
		uint8_t *pucEventBuf)
{
	uint8_t u4SenarioType;

	u4SenarioType = *pucEventBuf;

	DBGLOG(NIC, TRACE, "u4SenarioType = %d!\n", u4SenarioType);

	if (u4SenarioType == TX_PWR_EMI_SCENARIO_TYPE_UPDATE) {
		rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
			TX_PWR_EMI_STATUS_ACTION_UPDATE_EVENT);
	} else if (u4SenarioType == TX_PWR_EMI_SCENARIO_TYPE_CONNECTION) {
		rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
			TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_END);
	} else if (u4SenarioType == TX_PWR_EMI_SCENARIO_TYPE_NAN_INIT) {
		rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
			TX_PWR_EMI_STATUS_ACTION_NAN_INIT_END);
	} else if (u4SenarioType ==
		TX_PWR_EMI_SCENARIO_TYPE_NAN_TIMELINE_UPDATE) {
		rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
			TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_END);
	} else if (u4SenarioType ==
		TX_PWR_EMI_SCENARIO_TYPE_CNM_RLM_SYNC_OP_PARAMS) {
		rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
			TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_END);
	}

}

void rlmDomainSendPwrLimitEmiInfo(struct ADAPTER *prAdapter,
	struct CMD_EMI_POWER_LIMIT_FORMAT *prTxPwrEmiFormat)
{
	uint32_t rStatus = 0;

	prTxPwrEmiFormat->u1ScenarioType = TX_PWR_EMI_SCENARIO_TYPE_UPDATE;

	rStatus = wlanSendSetQueryCmd(prAdapter,
		CMD_ID_SET_PWR_LIMIT_EMI_INFO,
		TRUE,
		FALSE,
		FALSE,
		NULL,
		NULL,
		sizeof(struct CMD_EMI_POWER_LIMIT_FORMAT),
		(uint8_t *) prTxPwrEmiFormat,
		NULL,
		0
	);

	rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
		TX_PWR_EMI_STATUS_ACTION_UPDATE_CMD);
}

void rlmDomainPwrLmtConnectionCMD(
	struct ADAPTER *prAdapter,
	enum ENUM_TX_PWR_EMI_SCENARIO_TYPE type)
{
	uint32_t rStatus = 0;
	uint8_t fgNeedSendCmd = TRUE;
	struct CMD_EMI_POWER_LIMIT_FORMAT rTxPwrEmiFormat = {0};
	enum ENUM_TX_PWR_EMI_STATUS_ACTION action;

	rTxPwrEmiFormat.u1ScenarioType = type;

	if (type == TX_PWR_EMI_SCENARIO_TYPE_CONNECTION) {
		action = TX_PWR_EMI_STATUS_ACTION_REQUEST_CHANNEL_START;
		fgNeedSendCmd = FALSE;
	} else if (type == TX_PWR_EMI_SCENARIO_TYPE_NAN_INIT) {
		action = TX_PWR_EMI_STATUS_ACTION_NAN_INIT_START;
		fgNeedSendCmd = TRUE;
	} else if (type == TX_PWR_EMI_SCENARIO_TYPE_NAN_TIMELINE_UPDATE) {
		action = TX_PWR_EMI_STATUS_ACTION_NAN_TIMELINE_UPDATE_START;
		fgNeedSendCmd = FALSE;
	} else if (type == TX_PWR_EMI_SCENARIO_TYPE_CNM_RLM_SYNC_OP_PARAMS) {
		action = TX_PWR_EMI_STATUS_ACTION_CNM_RLM_SYNC_OP_PARAMS_START;
		fgNeedSendCmd = TRUE;
	} else
		return;

	if (fgNeedSendCmd == TRUE) {
		rStatus = wlanSendSetQueryCmd(prAdapter,
			CMD_ID_SET_PWR_LIMIT_EMI_INFO,
			TRUE,
			FALSE,
			FALSE,
			NULL,
			NULL,
			sizeof(struct CMD_EMI_POWER_LIMIT_FORMAT),
			(uint8_t *) &rTxPwrEmiFormat,
			NULL,
			0
		);
	}

	rlmDomainPwrLmtEmiStatusCtrl(prAdapter,
		action);
}

void rlmDomainWritePwrLimitToEmi(struct ADAPTER *prAdapter)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;
	struct PWR_LIMIT_INFO rPerPwrLimitInfo;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
	struct EMI_POWER_LIMIT_INFO *prPerTxpwrEmiInfo;
	struct CMD_EMI_POWER_LIMIT_FORMAT rEmiFormat = {0};
	uint32_t offset = 0, size = 0, u4ChIdx = 0;
	uint8_t *prTxPowrEmiAddress = NULL;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct HIF_MEM *prMem = NULL;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	uint32_t MulBnOfs = 0, MulBnSize = 0;
	uint8_t *prTxpMulitBandEmiAdr = NULL;
	uint8_t u1BandMask = 0;
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx = 0;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;

	if (prMemOps && prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
			WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT);
		if (prMem) {
			prTxPowrEmiAddress = (uint8_t *)prMem->va;
			DBGLOG(HAL, TRACE, "[TXP-S]base: pa: %pa, va:0x%llx\n",
				&prMem->pa, prMem->va);
		} else {
			DBGLOG(NIC, DEBUG, "Failed to obtain prMem\n");
			return;
		}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
		prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
			WIFI_MISC_MEM_BLOCK_MULTIBAND_TX_POWER_LIMIT);
		if (prMem) {
			prTxpMulitBandEmiAdr = (uint8_t *)prMem->va;
			DBGLOG(HAL, TRACE, "[TXP-M]base: pa: %pa, va:0x%llx\n",
				&prMem->pa, prMem->va);
		} else {
			DBGLOG(NIC, INFO, "Failed to obtain prMem\n");
			return;
		}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
	}
#endif

	if (prTxPowrEmiAddress == NULL) {
		DBGLOG(NIC, DEBUG, "TXP EMI Address is NULL\n");
		return;
	}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	if (prTxpMulitBandEmiAdr == NULL) {
		DBGLOG(NIC, INFO, "TXP MultiBand EMI Address is NULL\n");
		return;
	}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	/*write single band power limit emi*/
	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		prPerPwrLimit = &(prPwrLimit[eRF][0]);
		for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
			PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {

				prPerPwrLimit =
					&(prPwrLimit[eRF][eProt]);
				rPerPwrLimitInfo =
					g_RlmPwrLimitInfo[eRF][eProt];
				eLimitType =
					rPerPwrLimitInfo.eLimitType;
				prPerTxpwrEmiInfo =
					&(prAdapter->rTxpwrEmiInfo[eRF][eProt]);

				g_rRlmPwrLimitHandler[eLimitType].pfWriteEmi(
					u4ChIdx,
					prPerPwrLimit,
					prTxPowrEmiAddress + offset,
					&size
				);

				if (u4ChIdx == 0) {
					prPerTxpwrEmiInfo->u4EmiAddrOffset =
						offset;
					prPerTxpwrEmiInfo->u1LimitType =
						eLimitType;
					prPerTxpwrEmiInfo->u1Size =
						size;
					prPerTxpwrEmiInfo->u2ChannelNum =
						prPerPwrLimit->ucNum;
				}

				offset += size;
			}
		}
	}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	/*write multi band power limit emi */
	PWR_LIMIT_FOR_EACH_MULTI_BAND(eBandIdx) {
		/* band mask */
		u1BandMask = txPwrBandMaskSearch(eBandIdx);
		kalMemCopy(prTxpMulitBandEmiAdr + MulBnOfs,
			&u1BandMask,
			sizeof(u1BandMask));
		MulBnOfs += sizeof(u1BandMask);
		/* pwr limit */
		PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
			DBGLOG(NIC, LOUD,
				"[MulBnPwr] [B:%d/R:%d]MulBnOfs:%d\n",
				eBandIdx, eRF, MulBnOfs);
			MulBnSize = rlmDomainWriteMultiTxPwrEmiData(
				prAdapter,
				eBandIdx,
				eRF,
				g_RlmMulitPwrLimitInfo[eRF],
				prTxpMulitBandEmiAdr + MulBnOfs);
			MulBnOfs += MulBnSize;
		}
	}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
			rlmDomainDumpPwrLimitEmiPayload(prAdapter, eRF, eProt);
		}
	}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	MulBnOfs = 0;
	PWR_LIMIT_FOR_EACH_MULTI_BAND(eBandIdx) {
		u1BandMask = prTxpMulitBandEmiAdr[MulBnOfs];
		MulBnOfs += sizeof(u1BandMask);
		PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
			MulBnSize = rlmDomainDumpMultiPwrLimitEmiPayload(
				prAdapter,
				u1BandMask,
				eBandIdx,
				eRF,
				g_RlmMulitPwrLimitInfo[eRF],
				prTxpMulitBandEmiAdr + MulBnOfs);
			MulBnOfs += MulBnSize;
		}
	}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */


	rEmiFormat.u1RFBandNum = PWR_LIMIT_RF_BAND_NUM;
	rEmiFormat.u1ProtocolNum = PWR_LIMIT_PROTOCOL_NUM;
	rEmiFormat.u1ApplyMethod = rlmDomainPwrLmtGetChannelDefine();
	rEmiFormat.u1ScenarioType = TX_PWR_EMI_SCENARIO_TYPE_UPDATE;
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	rEmiFormat.u1MultiBandVer = 0;
	rEmiFormat.u1MultiBandNum = PWR_LIMIT_MULTIBAND_TYPE_NUM;
	rEmiFormat.u1MultiBandSize = PWR_LIMIT_MULTIBAND_NUM;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	DBGLOG(RLM, TRACE,
		"TXP CMD[RFN:%d,ProN:%d,M:%d,T:%d,MBV:%d,MBN:%d,MBS:%d]\n",
		rEmiFormat.u1RFBandNum,
		rEmiFormat.u1ProtocolNum,
		rEmiFormat.u1ApplyMethod,
		rEmiFormat.u1ScenarioType,
		rEmiFormat.u1MultiBandVer,
		rEmiFormat.u1MultiBandNum,
		rEmiFormat.u1MultiBandSize);

	kalMemCopy(&rEmiFormat.rTxpwrEmiInfo,
		&prAdapter->rTxpwrEmiInfo, sizeof(prAdapter->rTxpwrEmiInfo));

	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
			DBGLOG(RLM, TRACE,
			"TXP EMI INFO[%d][%d] [ofs:%d,type:%d,Size:%d,ChNum%d]\n",
			eRF, eProt,
			rEmiFormat.rTxpwrEmiInfo[eRF][eProt].u4EmiAddrOffset,
			rEmiFormat.rTxpwrEmiInfo[eRF][eProt].u1LimitType,
			rEmiFormat.rTxpwrEmiInfo[eRF][eProt].u1Size,
			rEmiFormat.rTxpwrEmiInfo[eRF][eProt].u2ChannelNum);
		}
	}

	rlmDomainSendPwrLimitEmiInfo(prAdapter, &rEmiFormat);
}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
static uint32_t rlmDomainDumpMultiPwrLimitEmiPayload(
	struct ADAPTER *prAdapter,
	uint8_t u1BandMask,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo,
	char *prTxpMulitBandEmiAdr
)
{
	uint32_t i = 0, u4ChIdx = 0, msgofs = 0, shift = 0;
	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData;
	char debugBuf[PWR_BUF_LEN];

	prData = &prAdapter->rMulBnData[eBandIdx][eRF];

	if (prTxpMulitBandEmiAdr == NULL) {
		DBGLOG(NIC, INFO, "TXP EMI Address is NULL\n");
		return 0;
	}

	for (u4ChIdx = 0; u4ChIdx < rPwrLmtInfo.u4ChCnt; u4ChIdx++) {
		msgofs = 0;
		msgofs +=
			kalScnprintf(
				debugBuf + msgofs,
				PWR_BUF_LEN - msgofs,
				"BandIdx[%d]BandMask[%d]RF[%d]CH[%d]EMI Pwr:",
				eBandIdx,
				u1BandMask,
				eRF,
				prData->u1Channel[u4ChIdx]);

		for (i = 0; i < PWR_LIMIT_MULTIBAND_NUM; i++) {
			msgofs +=
				kalScnprintf(
					debugBuf + msgofs,
					PWR_BUF_LEN - msgofs,
					" %2d,",
					prTxpMulitBandEmiAdr[shift + i]
				);
		}

		if (msgofs >= 1)
			debugBuf[msgofs - 1] = '\0';
		else
			debugBuf[0] = '\0';

		shift += PWR_LIMIT_MULTIBAND_NUM;
		DBGLOG(RLM, LOUD, "[MulBnPwr] %s\n", debugBuf);
		kalMemZero(debugBuf, PWR_BUF_LEN);
	}

	return shift;
}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

static void rlmDomainDumpPwrLimitEmiPayload(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	enum ENUM_PWR_LIMIT_PROTOCOL eProt
)
{
	uint32_t i, size, u4Offset, u4ChIdx, u4ChNum, u4MsgOffset = 0;
	uint32_t u4PerChDataSize = 0;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
	char debugBuf[PWR_BUF_LEN];
	struct EMI_POWER_LIMIT_INFO *prPerTxpwrEmiInfo;
	uint8_t *prTxPowrEmiAddress = NULL;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct HIF_MEM *prMem = NULL;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
			WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT);
		prTxPowrEmiAddress = (uint8_t *)prMem->va;
	}
#endif

	if (prTxPowrEmiAddress == NULL) {
		DBGLOG(NIC, DEBUG, "TXP EMI Address is NULL\n");
		return;
	}

	PWR_LIMIT_FOR_EACH_PROTOCOL(i) {
		u4PerChDataSize += prAdapter->rTxpwrEmiInfo[eRF][i].u1Size;
	}

	prPerTxpwrEmiInfo = &(prAdapter->rTxpwrEmiInfo[eRF][eProt]);

	u4Offset = prPerTxpwrEmiInfo->u4EmiAddrOffset;
	eLimitType = prPerTxpwrEmiInfo->u1LimitType;
	size = prPerTxpwrEmiInfo->u1Size;
	u4ChNum = prPerTxpwrEmiInfo->u2ChannelNum;

	for (u4ChIdx = 0; u4ChIdx < u4ChNum; u4ChIdx++) {

		u4MsgOffset +=
			kalScnprintf(
				debugBuf + u4MsgOffset,
				PWR_BUF_LEN - u4MsgOffset,
				"Channel[%d], Data:",
				prTxPowrEmiAddress[u4Offset]
			);

		for (i = 1; i < size; i++) {
			u4MsgOffset +=
				kalScnprintf(
					debugBuf + u4MsgOffset,
					PWR_BUF_LEN - u4MsgOffset,
					" %2d,",
					prTxPowrEmiAddress[u4Offset + i]
				);
		}

		if (u4MsgOffset >= 1)
			debugBuf[u4MsgOffset - 1] = '\0';
		else
			debugBuf[0] = '\0';

		u4MsgOffset = 0;
		u4Offset += u4PerChDataSize;

		DBGLOG(RLM, LOUD,
			"%s ,%s, ofs/LmtType/Size/ChNum:[%d,%d,%d,%d],%s\n",
			g_au1TxPwrRFBand[eRF],
			g_au1TxPwrProtocol[eProt],
			prPerTxpwrEmiInfo->u4EmiAddrOffset,
			prPerTxpwrEmiInfo->u1LimitType,
			prPerTxpwrEmiInfo->u1Size,
			prPerTxpwrEmiInfo->u2ChannelNum,
			debugBuf);

		kalMemZero(debugBuf, PWR_BUF_LEN);
	}

}

int32_t rlmDomainReadPwrLimitEmiData(
	struct ADAPTER *prAdapter,
	char *pcCommand,
	int32_t i4TotalLen,
	uint32_t u4RFBand,
	uint32_t u4Channel
)
{
	uint32_t i, j, size, offset, channel, channel_num;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
	int i4BytesWritten = 0;
	struct EMI_POWER_LIMIT_INFO *prPerTxpwrEmiInfo;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt;
	uint32_t u4PerChDataSize = 0;
	uint8_t *prTxPowrEmiAddress = NULL;
	struct GLUE_INFO *prGlueInfo = prAdapter->prGlueInfo;
	struct HIF_MEM *prMem = NULL;
	struct GL_HIF_INFO *prHifInfo = &prGlueInfo->rHifInfo;
	struct mt66xx_chip_info *prChipInfo = prAdapter->chip_info;

#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	struct HIF_MEM_OPS *prMemOps = &prHifInfo->rMemOps;

	if (prMemOps->getWifiMiscRsvEmi) {
		prMem = prMemOps->getWifiMiscRsvEmi(prChipInfo,
			WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT);
		prTxPowrEmiAddress = (uint8_t *)prMem->va;
	}
#endif

	if (prTxPowrEmiAddress == NULL) {
		DBGLOG(NIC, DEBUG, "TXP EMI Address is NULL\n");
		return i4BytesWritten;
	}

	if (u4RFBand >= PWR_LIMIT_RF_BAND_NUM)
		return i4BytesWritten;

	PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
		u4PerChDataSize +=
			prAdapter->rTxpwrEmiInfo[u4RFBand][eProt].u1Size;
	}

	PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
		prPerTxpwrEmiInfo =
			&(prAdapter->rTxpwrEmiInfo[u4RFBand][eProt]);
		offset = prPerTxpwrEmiInfo->u4EmiAddrOffset;
		eLimitType = prPerTxpwrEmiInfo->u1LimitType;
		size = prPerTxpwrEmiInfo->u1Size;
		channel_num = prPerTxpwrEmiInfo->u2ChannelNum;

		i4BytesWritten +=
			kalScnprintf(pcCommand + i4BytesWritten,
				i4TotalLen - i4BytesWritten,
				"\n TX POWER LIMIT DATA [%s][%s] ofs/Type/Size/ChNum:[%d,%d,%d,%d]\n",
				g_au1TxPwrRFBand[u4RFBand],
				g_au1TxPwrProtocol[eProt],
				offset,
				eLimitType,
				size,
				channel_num
			);

		for (i = 0; i < channel_num; i++) {
			channel = prTxPowrEmiAddress[offset];

			if (channel != u4Channel)
				continue;

			i4BytesWritten +=
				kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"  ==> Channel[%d], Data:",
					prTxPowrEmiAddress[offset]
				);

			for (j = 1; j < size; j++) {
				i4BytesWritten +=
					kalScnprintf(
						pcCommand + i4BytesWritten,
						i4TotalLen - i4BytesWritten,
						"%2d,",
						prTxPowrEmiAddress[offset + j]
					);
			}
			i4BytesWritten +=
				kalScnprintf(
					pcCommand + i4BytesWritten,
					i4TotalLen - i4BytesWritten,
					"\n"
				);

			break;
		}
			offset += u4PerChDataSize;
	}

	return i4BytesWritten;
}

void rlmDomainApplyDynPwrSetting(
	struct ADAPTER *prAdapter)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;
	struct PWR_LIMIT_INFO rPerPwrLimitInfo;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
	struct DOMAIN_INFO_ENTRY *prDomainInfo;
	uint8_t bandedgeParam[4] = { 0, 0, 0, 0 };
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	prDomainInfo = rlmDomainGetDomainInfo(prAdapter);
	if (prDomainInfo) {
		bandedgeParam[0] = prDomainInfo->rSubBand[0].ucFirstChannelNum;
		bandedgeParam[1] = bandedgeParam[0] +
			prDomainInfo->rSubBand[0].ucNumChannels - 1;
	}

	/* single band power limit */
	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {

			prPerPwrLimit = &(prPwrLimit[eRF][eProt]);
			rPerPwrLimitInfo = g_RlmPwrLimitInfo[eRF][eProt];
			eLimitType = rPerPwrLimitInfo.eLimitType;

			txPwrCtrlApplyDynPwrSetting(
				prAdapter,
				prPerPwrLimit,
				bandedgeParam,
				eLimitType,
				eRF
			);
		}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
		/* multi band power limit Ian */
		PWR_LIMIT_FOR_EACH_MULTI_BAND(eBandIdx) {
			txPwrCtrlApplyDynMultiBandPwrSetting(
				prAdapter,
				eBandIdx,
				eRF,
				g_RlmMulitPwrLimitInfo[eRF]
			);
		}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	}
}

void rlmDomainDumpAllPwrLmtData(
	char *message,
	struct ADAPTER *prAdapter)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;
	struct PWR_LIMIT_INFO rPerPwrLimitInfo;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

	/* single band power limit */
	PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
		PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
			prPerPwrLimit = &(prPwrLimit[eRF][eProt]);
			rPerPwrLimitInfo = g_RlmPwrLimitInfo[eRF][eProt];
			eLimitType = rPerPwrLimitInfo.eLimitType;

			g_rRlmPwrLimitHandler[eLimitType].pfDumpData(
				message,
				prPerPwrLimit,
				eRF
			);

		}
	}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	/* multi band power limit */
	PWR_LIMIT_FOR_EACH_MULTI_BAND(eBandIdx) {
		PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
			rlmDomainDumpMultiBandPwrLimitPayload(
				prAdapter,
				message,
				eRF,
				eBandIdx,
				g_RlmMulitPwrLimitInfo[eRF]
			);
		}
	}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
}

void rlmDomainSetPwrLimitHeader(
	struct ADAPTER *prAdapter)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	struct PWR_LIMIT_INFO rPerPwrLimitInfo;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;

	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
			prPerPwrLimit = &(prPwrLimit[eRF][eProt]);
			rPerPwrLimitInfo = g_RlmPwrLimitInfo[eRF][eProt];

			/* ucNum */
			prPerPwrLimit->ucNum = 0;

			/* ucLimitType and ucVersion */
			prPerPwrLimit->ucLimitType =
				rPerPwrLimitInfo.eLimitType;
			prPerPwrLimit->ucVersion =
				rPerPwrLimitInfo.ucVersion;

			DBGLOG(RLM, TRACE,
				"rf[%d]protocol[%d]limit_type[%d]ver[%d]\n",
				eRF, eProt,
				prPerPwrLimit->ucLimitType,
				prPerPwrLimit->ucVersion
			);

		}
	}
}

void rlmDomainSetPwrLimitPayload(struct ADAPTER *prAdapter)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	struct PWR_LIMIT_INFO rPerPwrLimitInfo;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;
	enum ENUM_PWR_LIMIT_TYPE eLimitType;
#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx;
	uint16_t u1ChCnt = 0;
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */


	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		/* single band power limit */
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {

			prPerPwrLimit = &(prPwrLimit[eRF][eProt]);
			rPerPwrLimitInfo = g_RlmPwrLimitInfo[eRF][eProt];
			eLimitType = rPerPwrLimitInfo.eLimitType;

			/* Sub band power limit */
			g_rRlmPwrLimitHandler[eLimitType].pfLoadDefTbl(
				prAdapter,
				prPerPwrLimit,
				rPerPwrLimitInfo
			);

			/* Channel & rate power limit */
			g_rRlmPwrLimitHandler[eLimitType].pfLoadCfgTbl(
				prAdapter,
				prPerPwrLimit,
				eRF
			);
		}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
		/* multi band power limit Ian */
		PWR_LIMIT_FOR_EACH_MULTI_BAND(eBandIdx) {
			u1ChCnt = rlmDomainBuildDefaultMultiBandPwrLimitPayload(
				prAdapter,
				eBandIdx,
				eRF,
				g_RlmMulitPwrLimitInfo[eRF]);
			g_RlmMulitPwrLimitInfo[eRF].u4ChCnt = u1ChCnt;
		}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */
	}
}

uint8_t *rlmDomainPwrLmtGetMatchCountryCode(
	struct ADAPTER *prAdapter,
	uint16_t u2CountryCode)
{
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prPwrLimitDefaultTable;
	uint16_t u2TblIndex;
	uint8_t *pu1PwrLmtCountryCode;

	u2TblIndex =
	    rlmDomainPwrLimitDefaultTableDecision(prAdapter,
			prAdapter->rWifiVar.u2CountryCode,
			PWR_LIMIT_DEFAULT_BASE_NORMAL);

	if (u2TblIndex == POWER_LIMIT_TABLE_NULL) {
		DBGLOG(RLM, ERROR,
			"Can't find any table index!\n");
		return NULL;
	}

	prPwrLimitDefaultTable =
		PWR_LIMIT_COUNTRY_DEF_TBL(
			PWR_LIMIT_DEFAULT_BASE_NORMAL,
			u2TblIndex);

	pu1PwrLmtCountryCode = &prPwrLimitDefaultTable->aucCountryCode[0];

	return pu1PwrLmtCountryCode;
}

void rlmDomainSendAntPowerSetting(struct ADAPTER *prAdapter)
{
#if CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG
	struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT *prCmdAnt = NULL;
	struct CMD_CHANNEL_POWER_LIMIT_ANT *prPwrLmtAnt = NULL;
	uint32_t u4SetQueryInfoLenAnt = 0, u4SetCmdTableMaxSize = 0, i, rStatus;

	u4SetCmdTableMaxSize =
		sizeof(struct CMD_SET_COUNTRY_CHANNEL_POWER_LIMIT);

	prCmdAnt = cnmMemAlloc(prAdapter, RAM_TYPE_BUF, u4SetCmdTableMaxSize);
	if (!prCmdAnt) {
		DBGLOG(RLM, ERROR, "Domain: Alloc cmd buffer failed\n");
		goto err;
	}
	kalMemZero(prCmdAnt, u4SetCmdTableMaxSize);

	prCmdAnt->ucCategoryId = POWER_LIMIT_TABLE_CTRL;
	prCmdAnt->ucNum = 0;
	prCmdAnt->fgPwrTblKeep = TRUE;
	/* ANT number if PWR_LIMIT_TYPE_COMP_ANT*/
	prCmdAnt->ucLimitType = PWR_LIMIT_CMD_TYPE_ANT_V2;
	prCmdAnt->ucVersion = 1;

	txPwrCtrlApplyAntPowerSettings(prAdapter, prCmdAnt);

	prPwrLmtAnt = &prCmdAnt->u.rChPwrLimtAnt[0];
	DBGLOG(RLM, INFO, "ANT Config #%d", prCmdAnt->ucNum);
	for (i = 0; i < prCmdAnt->ucNum; i++) {
		DBGLOG(RLM, TRACE,
			"Final ANT Cfg%d Tag[%d]Ant[%d]Band[%d]Val[%d]\n",
			i,
			prPwrLmtAnt[i].cTagIdx,
			prPwrLmtAnt[i].cAntIdx,
			prPwrLmtAnt[i].cBandIdx,
			prPwrLmtAnt[i].cValue);

		if (prPwrLmtAnt[i].cTagIdx == -1)
			break;
	}

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

err:
	cnmMemFree(prAdapter, prCmdAnt);
#endif /*CFG_SUPPORT_DYNAMIC_PWR_LIMIT_ANT_TAG*/
}

void rlmDomainSetPwrLimitCountryCode(
	struct ADAPTER *prAdapter,
	uint8_t *pu1PwrLmtCountryCode)
{
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT **prPwrLimit =
		prAdapter->prPwrLimit;
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit;
	enum ENUM_PWR_LIMIT_RF_BAND eRF = PWR_LIMIT_RF_BAND_2G4;
	enum ENUM_PWR_LIMIT_PROTOCOL eProt = PWR_LIMIT_PROTOCOL_LEGACY;

	PWR_LIMIT_FOR_EACH_RF_BAND(eRF) {
		PWR_LIMIT_FOR_EACH_PROTOCOL(eProt) {
			prPerPwrLimit = &(prPwrLimit[eRF][eProt]);
			WLAN_GET_FIELD_BE16(pu1PwrLmtCountryCode,
				&prPerPwrLimit->u2CountryCode);
		}
	}
}

#if (CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1)
static uint8_t rlmDomainBuildDefaultMultiBandPwrLimitPayload(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rMultiBandPwrLimitInfo)
{
	uint8_t u1SubBandIdx, u1Channel = 0, i = 0;
	uint16_t u2ChCnt = 0;
	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData;


	PWR_LIMIT_FOR_EACH_SUBBAND(u1SubBandIdx, rMultiBandPwrLimitInfo) {
		PWR_LIMIT_FOR_EACH_SUB_BAND_CHANNEL(u1Channel, u1SubBandIdx) {
			if (u2ChCnt >= MAX_SUPPORT_CHANNEL_NUMBER)
				return 0;

			prData =
				&prAdapter->rMulBnData[eBandIdx][eRF];

			prData->u1Channel[u2ChCnt] =
				u1Channel;

			for (i = 0; i < PWR_LIMIT_MULTIBAND_NUM; i++) {
				prData->i1MBPwrLmt[u2ChCnt][i] =
					MAX_TX_POWER;
			}
			u2ChCnt++;
		}
	}

	return u2ChCnt;
}

static void rlmDomainApplyDynMultiBandSettings(
	struct TX_PWR_CTRL_ELEMENT *element,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo)
{
	uint32_t u4ChIdx = 0, u4Idx = 0, u4Ofs = 0;
	int8_t *pcRefVal, *pcCfgOp, *pcCfgVal;
	bool fgNeedArbitrator = FALSE;
	struct TX_PWR_CTRL_MULTIBAND_SETTING *prMultiBnCfg;
	enum ENUM_TX_POWER_CTRL_CHANNEL_TYPE eChanneltype;
	uint8_t bandedgeParam[4] = { 0, 0, 0, 0 };
	enum ENUM_TX_POWER_CTRL_TYPE eCtrlType;
	struct TX_PWR_CTRL_MULTIBAND_PWR *prPwr;

	eChanneltype = PWR_CTRL_CHNL_TYPE_RANGE;
	eCtrlType = PWR_CTRL_TYPE_IOCTL_POWER_LEVEL;

#if (CFG_SUPPORT_WIFI_6G == 1)
	if (eRF == PWR_LIMIT_RF_BAND_6G)
		eChanneltype = PWR_CTRL_CHNL_TYPE_6G_RANGE;
#endif


	prMultiBnCfg = &element->rMultiBandCfg[eBandIdx];

	for (u4Idx = 0; u4Idx <= prMultiBnCfg->u1ChGrpCnt; u4Idx++) {
		prPwr = &prMultiBnCfg->rMultiBandPwr[u4Idx];

		if (prPwr->u1RfBand != eRF)
			continue;

		for (u4ChIdx = 0; u4ChIdx < rPwrLmtInfo.u4ChCnt; u4ChIdx++) {

			fgNeedArbitrator = rlmDomainIsNeedToDoArbitrator(
				prData->u1Channel[u4ChIdx],
				prPwr->u1Startch,
				prPwr->u1Endch,
				bandedgeParam,
				eChanneltype,
				eRF);

			if (!fgNeedArbitrator)
				continue;

			pcRefVal = prData->i1MBPwrLmt[u4ChIdx];
			pcCfgOp =
				&prPwr->Op[0];
			pcCfgVal =
				&prPwr->i1MBPwrLmt[0];

			for (u4Ofs = PWR_LIMIT_MULTIBAND_RU26;
				u4Ofs < PWR_LIMIT_MULTIBAND_NUM;
				u4Ofs++) {
				if (pcCfgOp[u4Ofs] !=
					PWR_CTRL_TYPE_NO_ACTION) {
					txPwrOperate(
						eCtrlType,
						pcRefVal + u4Ofs,
						pcCfgVal + u4Ofs);
				}
			}
		}
	}
}

static void rlmDomainDumpMultiBandPwrLimitPayload(
	struct ADAPTER *prAdapter,
	char *message,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo)
{
	char msgLimit[PWR_BUF_LEN];
	uint32_t u4MsgOfs = 0, j = 0, u4ChIdx = 0;
	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData;

	prData = &prAdapter->rMulBnData[eBandIdx][eRF];

	for (u4ChIdx = 0; u4ChIdx < rPwrLmtInfo.u4ChCnt; u4ChIdx++) {
		kalMemZero(msgLimit, sizeof(char) * PWR_BUF_LEN);
		u4MsgOfs = 0;

		/*message head*/
		u4MsgOfs +=
			snprintf(
				msgLimit + u4MsgOfs,
				PWR_BUF_LEN - u4MsgOfs,
				"MultiBand[%d]Mask[%d] ch=%d, %s,Limit=",
				eBandIdx,
				txPwrBandMaskSearch(eBandIdx),
				prData->u1Channel[u4ChIdx],
				g_au1TxPwrRFBand[eRF]);

		/*Payload*/
		for (j = 0; j < PWR_LIMIT_MULTIBAND_NUM; j++) {
			u4MsgOfs +=
				snprintf(msgLimit + u4MsgOfs,
					PWR_BUF_LEN - u4MsgOfs,
					"%d,",
					prData->i1MBPwrLmt[u4ChIdx][j]);
		}

		/*message tail*/
		msgLimit[u4MsgOfs - 1] = '\0';

		DBGLOG(RLM, LOUD, "[MulBnPwr] %s:%s\n", message, msgLimit);
	}
}

static uint32_t rlmDomainWriteMultiTxPwrEmiData(
	struct ADAPTER *prAdapter,
	enum ENUM_PWR_LIMIT_MULTIBAND_TYPE eBandIdx,
	enum ENUM_PWR_LIMIT_RF_BAND eRF,
	struct MULTIBAND_PWR_LIMIT_INFO rPwrLmtInfo,
	char *prTxpMulitBandEmiAdr)
{

	struct TX_PWR_CTRL_MULTIBAND_EMI_DATA *prData;
	uint32_t u4ChIdx = 0, u4Size = 0;
	int8_t *ptr;

	prData = &prAdapter->rMulBnData[eBandIdx][eRF];

	for (u4ChIdx = 0; u4ChIdx < rPwrLmtInfo.u4ChCnt; u4ChIdx++) {
		ptr = prData->i1MBPwrLmt[u4ChIdx];
		kalMemCopy(prTxpMulitBandEmiAdr + u4Size, ptr,
			PWR_LIMIT_MULTIBAND_NUM);
		u4Size += PWR_LIMIT_MULTIBAND_NUM;
	}
	return u4Size;
}
#endif /* CFG_SUPPORT_MULTIBAND_PWR_LMT_EMI == 1 */

static void rlmDomainGetDefaultTablePwrLimit(
	struct ADAPTER *prAdapter,
	uint32_t u4SubBandIdx,
	uint8_t ucChannel,
	uint8_t *pucPwr,
	bool *pfgPwrUnit
)
{
	struct COUNTRY_POWER_LIMIT_TABLE_DEFAULT *prDefTbl;
	enum ENUM_PWR_LIMIT_DEFAULT_BASE eDefBase =
		PWR_LIMIT_DEFAULT_BASE_NORMAL;
	uint16_t u2TblIndex = 0;
	uint8_t ucIdx = 0;

	if (!prAdapter || !pucPwr || !pfgPwrUnit)
		return;

#if (CFG_SUPPORT_WIFI_6G_PWR_MODE == 1)
	if (u4SubBandIdx >= PWR_LMT_SUBBAND_UNII5A
		&& u4SubBandIdx <= PWR_LMT_SUBBAND_UNII8)
		eDefBase = rlmDomainPwrLmtGetDefaultBaseByCh(prAdapter,
		ucChannel);
#endif

	u2TblIndex = rlmDomainPwrLimitDefaultTableDecision(
		prAdapter,
		prAdapter->rWifiVar.u2CountryCode,
		eDefBase);

	if (u2TblIndex != POWER_LIMIT_TABLE_NULL) {
		prDefTbl = PWR_LIMIT_COUNTRY_DEF_TBL(eDefBase, u2TblIndex);
		*pucPwr = rlmDomainGetSubBandPwrLimit(prDefTbl, u4SubBandIdx);
		rlmDomainGetSubBandDefPwrIdx(u4SubBandIdx, &ucIdx);
		*pfgPwrUnit = prDefTbl->ucPwrUnit & BIT(ucIdx);
	} else {
		*pucPwr = MAX_TX_POWER;
		*pfgPwrUnit = 0;
	}

}

static void rlmDomainBuildDefaultPwrLimitPayload_Legacy(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo)
{
	struct CHANNEL_POWER_LIMIT_LEGACY *prChPwrLimit_Legacy;
	uint32_t u4SubBandIdx, u4Channel = 0;
	uint8_t ucPwr = MAX_TX_POWER;
	bool fgPwrUnit = false;


	prChPwrLimit_Legacy = &prPerPwrLimit->u.rLegacy[0];

	PWR_LIMIT_FOR_EACH_SUBBAND(u4SubBandIdx, rPerPwrLimitInfo) {
		PWR_LIMIT_FOR_EACH_SUB_BAND_CHANNEL(u4Channel, u4SubBandIdx) {

			prChPwrLimit_Legacy->ucCentralCh = u4Channel;

			rlmDomainGetDefaultTablePwrLimit(
				prAdapter,
				u4SubBandIdx,
				prChPwrLimit_Legacy->ucCentralCh,
				&ucPwr,
				&fgPwrUnit);

			if (fgPwrUnit) {
				/*BW20*/
				prChPwrLimit_Legacy->cPwrLimitCCK_L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimitCCK_H = ucPwr;
				prChPwrLimit_Legacy->cPwrLimitOFDM_L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimitOFDM_H = ucPwr;
				prChPwrLimit_Legacy->cPwrLimit20L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimit20H = ucPwr;
				prChPwrLimit_Legacy->cPwrLimitCCK_L = ucPwr;

				/*BW40*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_Legacy->cPwrLimit40L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimit40H = ucPwr;

				/*BW80*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_Legacy->cPwrLimit80L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimit80H = ucPwr;

				/*BW160*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_Legacy->cPwrLimit160L = ucPwr;
				prChPwrLimit_Legacy->cPwrLimit160H = ucPwr;

			} else {
				kalMemSet(&prChPwrLimit_Legacy->cPwrLimitCCK_L,
					ucPwr, PWR_LIMIT_LEGACY_NUM);
			}

			prPerPwrLimit->ucNum++;
			prChPwrLimit_Legacy++;
		}
	}
};

static void rlmDomainBuildConfigPwrLimitPayload_Legacy(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
	struct CHANNEL_POWER_LIMIT_LEGACY *prChPwrLimit_Legacy;
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY *prCfgPwrLmt;
	enum ENUM_PWR_LIMIT_CONFIG_BASE eCfgBase;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	uint32_t u4ChIdx = 0, u4TableNum = 0, i = 0;
	bool fgChannelValid = FALSE;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {

		prChPwrLimit_Legacy = &prPerPwrLimit->u.rLegacy[u4ChIdx];
		eCfgBase = rlmDomainPwrLmtGetConfigBaseByCh(prAdapter, eRF,
			prChPwrLimit_Legacy->ucCentralCh);

		prCfgPwrLmt =
			g_rlmPowerLimitConfigTable[eCfgBase].Legacy.table;
		u4TableNum =
			g_rlmPowerLimitConfigTable[eCfgBase].Legacy.table_num;

		for (i = 0; i < u4TableNum; i++) {
			WLAN_GET_FIELD_BE16(
				&prCfgPwrLmt[i].aucCountryCode[0],
				&u2CountryCodeTable);

			fgChannelValid =
				rlmDomainCheckChannelEntryValid(
					prAdapter,
					rlmDomainConvertRFBandEnum(eRF),
					prCfgPwrLmt[i].i2CentralCh
				);

			if (u2CountryCodeTable == COUNTRY_CODE_NULL)
				break;	/*end of configuration table */
			else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prPerPwrLimit->u2CountryCode,
					fgChannelValid,
					rlmDomainConvertRFBandEnum(eRF),
					prCfgPwrLmt[i].i2CentralCh,
					prChPwrLimit_Legacy->ucCentralCh)
					== FALSE)
				continue;

			PwrLmtTblArbitrator(
				&prChPwrLimit_Legacy->cPwrLimitCCK_L,
				&prCfgPwrLmt[i].aucPwrLimit[0],
				PWR_LIMIT_LEGACY_NUM);
		}
	}
}

static void rlmDomainDumpPwrLimitPayload_Legacy(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
	struct CHANNEL_POWER_LIMIT_LEGACY *prChPwrLimit_Legacy;
	char msgLimit[PWR_BUF_LEN];
	uint32_t u4MsgOfs = 0, j = 0, u4ChIdx = 0;
	int8_t *pcRatePwr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_Legacy = &prPerPwrLimit->u.rLegacy[u4ChIdx];
		pcRatePwr = &prChPwrLimit_Legacy->cPwrLimitCCK_L;

		kalMemZero(msgLimit, sizeof(char) * PWR_BUF_LEN);
		u4MsgOfs = 0;

		/*message head*/
		u4MsgOfs +=
			snprintf(
				msgLimit + u4MsgOfs,
				PWR_BUF_LEN - u4MsgOfs,
				"Legacy ch=%d, %s,Limit=",
				prChPwrLimit_Legacy->ucCentralCh,
				g_au1TxPwrRFBand[eRF]
			);

		/*message body*/
		for (j = PWR_LIMIT_LEGACY_CCK_L; j < PWR_LIMIT_LEGACY_NUM; j++)
			u4MsgOfs +=
				snprintf(msgLimit + u4MsgOfs,
					PWR_BUF_LEN - u4MsgOfs,
					"%d,",
					*(pcRatePwr + j)
				);

		/*message tail*/
		if (u4MsgOfs >= 1)
			msgLimit[u4MsgOfs - 1] = '\0';
		else
			msgLimit[0] = '\0';

		DBGLOG(RLM, LOUD, "%s:%s\n", message, msgLimit);
	}
}

static void rlmDomainApplyDynSettings_Legacy(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
	struct CHANNEL_POWER_LIMIT_LEGACY *prChPwrLimit_Legacy;
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChlSetting = NULL;
	uint32_t u4ChIdx = 0, u4SetIdx = 0, u4OfsIdx = 0;
	uint32_t u4EleSetCount = prCurElement->settingCount;
	int8_t *pcRefValPtr, *pcApplyValPtr;
	bool fgNeedArbitrator = FALSE;
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN *pcApplyOpPtr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_Legacy = &prPerPwrLimit->u.rLegacy[u4ChIdx];
		pcRefValPtr = &prChPwrLimit_Legacy->cPwrLimitCCK_L;

		for (u4SetIdx = 0; u4SetIdx < u4EleSetCount; u4SetIdx++) {

			prChlSetting = &prCurElement->rChlSettingList[u4SetIdx];

			fgNeedArbitrator =
				rlmDomainIsNeedToDoArbitrator(
					prChPwrLimit_Legacy->ucCentralCh,
					prChlSetting->channelParam[0],
					prChlSetting->channelParam[1],
					bandedgeParam,
					prChlSetting->eChnlType,
					eRF
				);

			if (!fgNeedArbitrator)
				continue;

			pcApplyOpPtr =
				prChlSetting->op;
			pcApplyValPtr =
				prChlSetting->i8PwrLimit;

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eRF == PWR_LIMIT_RF_BAND_6G) {
				pcApplyOpPtr =
					prChlSetting->opLegacy_6G;
				pcApplyValPtr =
					prChlSetting->i8PwrLimitLegacy_6G;
			}
#endif

			for (u4OfsIdx = PWR_LIMIT_LEGACY_CCK_L;
				u4OfsIdx < PWR_LIMIT_LEGACY_NUM;
				u4OfsIdx++) {
				if (pcApplyOpPtr[u4OfsIdx]
					!= PWR_CTRL_TYPE_NO_ACTION) {
					txPwrOperate(prCurElement->eCtrlType,
						pcRefValPtr + u4OfsIdx,
						pcApplyValPtr + u4OfsIdx);
				}
			}
		}
	}
}

static void rlmDomainWriteTxPwrEmiData_Legacy(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size
)
{
	struct CHANNEL_POWER_LIMIT_LEGACY *prChPwrLimit_Legacy;
	*u4Size = sizeof(struct CHANNEL_POWER_LIMIT_LEGACY);
	prChPwrLimit_Legacy = &prPerPwrLimit->u.rLegacy[channel_index];
	kalMemCopy(prTxPowrEmiAddress, prChPwrLimit_Legacy, *u4Size);
}

static void rlmDomainBuildDefaultPwrLimitPayload_HE(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo)
{
	struct CHANNEL_POWER_LIMIT_HE *prChPwrLimit_HE;
	uint32_t u4SubBandIdx = 0, u4Channel = 0;
	uint8_t ucPwr = 0;
	bool fgPwrUnit = false;

	prChPwrLimit_HE = &prPerPwrLimit->u.rHE[0];

	PWR_LIMIT_FOR_EACH_SUBBAND(u4SubBandIdx, rPerPwrLimitInfo) {
		PWR_LIMIT_FOR_EACH_SUB_BAND_CHANNEL(u4Channel, u4SubBandIdx) {

			prChPwrLimit_HE->ucCentralCh = u4Channel;

			rlmDomainGetDefaultTablePwrLimit(
				prAdapter,
				u4SubBandIdx,
				prChPwrLimit_HE->ucCentralCh,
				&ucPwr,
				&fgPwrUnit);

			if (fgPwrUnit) {
				/*BW20*/
				prChPwrLimit_HE->cPwrLimitRU26L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU26H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU26U = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU52L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU52H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU52U = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU106L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU106H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU106U = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU242L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU242H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU242U = ucPwr;

				/*BW40*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_HE->cPwrLimitRU484L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU484H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU484U = ucPwr;

				/*BW80*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_HE->cPwrLimitRU996L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU996H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU996U = ucPwr;

				/*BW160*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_HE->cPwrLimitRU1992L = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU1992H = ucPwr;
				prChPwrLimit_HE->cPwrLimitRU1992U = ucPwr;

			} else {
				kalMemSet(&prChPwrLimit_HE->cPwrLimitRU26L,
					ucPwr, PWR_LIMIT_HE_NUM);
			}

			prPerPwrLimit->ucNum++;
			prChPwrLimit_HE++;
		}
	}
}


static void rlmDomainBuildConfigPwrLimitPayload_HE(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBand)
{
	struct CHANNEL_POWER_LIMIT_HE *prChPwrLimit_HE;
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE *prCfgPwrLmt;
	enum ENUM_PWR_LIMIT_CONFIG_BASE eCfgTblBase;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	uint32_t u4ChIdx = 0, u4TableNum, i;
	bool fgChannelValid = FALSE;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {

		prChPwrLimit_HE = &prPerPwrLimit->u.rHE[u4ChIdx];
		eCfgTblBase = rlmDomainPwrLmtGetConfigBaseByCh(prAdapter,
			eRFBand, prChPwrLimit_HE->ucCentralCh);

		prCfgPwrLmt =
			g_rlmPowerLimitConfigTable[eCfgTblBase].HE.table;
		u4TableNum =
			g_rlmPowerLimitConfigTable[eCfgTblBase].HE.table_num;

		for (i = 0; i < u4TableNum; i++) {

			WLAN_GET_FIELD_BE16(
				&prCfgPwrLmt[i].aucCountryCode[0],
				&u2CountryCodeTable);

			fgChannelValid =
				rlmDomainCheckChannelEntryValid(
					prAdapter,
					rlmDomainConvertRFBandEnum(eRFBand),
					prCfgPwrLmt[i].i2CentralCh
				);

			if (u2CountryCodeTable == COUNTRY_CODE_NULL)
				break;	/*end of configuration table */
			else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prPerPwrLimit->u2CountryCode,
					fgChannelValid,
					rlmDomainConvertRFBandEnum(eRFBand),
					prCfgPwrLmt[i].i2CentralCh,
					prChPwrLimit_HE->ucCentralCh) == FALSE)
				continue;

			PwrLmtTblArbitrator(
				&prChPwrLimit_HE->cPwrLimitRU26L,
				&prCfgPwrLmt[i].aucPwrLimit[0],
				PWR_LIMIT_HE_NUM);
		}
	}
}

static void rlmDomainDumpPwrLimitPayload_HE(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
	struct CHANNEL_POWER_LIMIT_HE *prChPwrLimit_HE;
	char msgLimit[PWR_BUF_LEN];
	uint32_t u4MsgOfs = 0, j = 0, u4ChIdx = 0;
	int8_t *pcRatePwr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_HE = &prPerPwrLimit->u.rHE[u4ChIdx];
		pcRatePwr = &prChPwrLimit_HE->cPwrLimitRU26L;

		kalMemZero(msgLimit, sizeof(char) * PWR_BUF_LEN);
		u4MsgOfs = 0;

		/*message head*/
		u4MsgOfs +=
			snprintf(msgLimit + u4MsgOfs,
				PWR_BUF_LEN - u4MsgOfs,
				"HE ch=%d, %s, Limit=",
				prChPwrLimit_HE->ucCentralCh,
				g_au1TxPwrRFBand[eRF]
			);

		/*message body*/
		for (j = PWR_LIMIT_HE_RU26_L; j < PWR_LIMIT_HE_NUM ; j++)
			u4MsgOfs +=
				snprintf(msgLimit + u4MsgOfs,
					PWR_BUF_LEN - u4MsgOfs,
					"%d,",
					*(pcRatePwr + j)
				);

		/*message tail*/
		if (u4MsgOfs >= 1)
			msgLimit[u4MsgOfs - 1] = '\0';
		else
			msgLimit[0] = '\0';

		DBGLOG(RLM, LOUD, "%s:%s\n", message, msgLimit);
	}
}

static void rlmDomainApplyDynSettings_HE(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
	struct CHANNEL_POWER_LIMIT_HE *prChPwrLimit_HE;
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChlSetting = NULL;
	uint32_t u4ChIdx = 0, u4SetIdx = 0, u4OfsIdx = 0;
	uint32_t u4EleSetCount = prCurElement->settingCount;
	int8_t *pcRefValPtr, *pcApplyValPtr;
	bool fgNeedArbitrator = FALSE;
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN *pcApplyOpPtr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_HE = &prPerPwrLimit->u.rHE[u4ChIdx];
		pcRefValPtr = &prChPwrLimit_HE->cPwrLimitRU26L;

		for (u4SetIdx = 0; u4SetIdx < u4EleSetCount; u4SetIdx++) {

			prChlSetting = &prCurElement->rChlSettingList[u4SetIdx];

			fgNeedArbitrator =
				rlmDomainIsNeedToDoArbitrator(
					prChPwrLimit_HE->ucCentralCh,
					prChlSetting->channelParam[0],
					prChlSetting->channelParam[1],
					bandedgeParam,
					prChlSetting->eChnlType,
					eRF
				);


			if (!fgNeedArbitrator)
				continue;

			pcApplyOpPtr =
				prChlSetting->opHE;
			pcApplyValPtr =
				prChlSetting->i8PwrLimitHE;

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eRF == PWR_LIMIT_RF_BAND_6G) {
				pcApplyOpPtr =
					prChlSetting->op6E;
				pcApplyValPtr =
					prChlSetting->i8PwrLimit6E;
			}
#endif
			for (u4OfsIdx = PWR_LIMIT_HE_RU26_L;
				u4OfsIdx < PWR_LIMIT_HE_NUM;
				u4OfsIdx++) {
				if (pcApplyOpPtr[u4OfsIdx]
					!= PWR_CTRL_TYPE_NO_ACTION) {
					txPwrOperate(prCurElement->eCtrlType,
						pcRefValPtr + u4OfsIdx,
						pcApplyValPtr + u4OfsIdx);
				}
			}
		}
	}
}

static void rlmDomainWriteTxPwrEmiData_HE(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size
)
{
	struct CHANNEL_POWER_LIMIT_HE *prChPwrLimit_HE;
	*u4Size = sizeof(struct CHANNEL_POWER_LIMIT_HE);
	prChPwrLimit_HE = &prPerPwrLimit->u.rHE[channel_index];
	kalMemCopy(prTxPowrEmiAddress, prChPwrLimit_HE, *u4Size);
}

static void rlmDomainBuildDefaultPwrLimitPayload_EHT(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct PWR_LIMIT_INFO rPerPwrLimitInfo)
{
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CHANNEL_POWER_LIMIT_EHT *prChPwrLimit_EHT;
	uint32_t u4SubBandIdx, u4Channel = 0;
	uint8_t ucPwr = 0;
	bool fgPwrUnit = false;

	prChPwrLimit_EHT = &prPerPwrLimit->u.rEHT[0];

	PWR_LIMIT_FOR_EACH_SUBBAND(u4SubBandIdx, rPerPwrLimitInfo) {
		PWR_LIMIT_FOR_EACH_SUB_BAND_CHANNEL(u4Channel, u4SubBandIdx) {

			prChPwrLimit_EHT->ucCentralCh = u4Channel;

			rlmDomainGetDefaultTablePwrLimit(
				prAdapter,
				u4SubBandIdx,
				prChPwrLimit_EHT->ucCentralCh,
				&ucPwr,
				&fgPwrUnit);

			if (fgPwrUnit) {
				/*BW20*/
				prChPwrLimit_EHT->cPwrLimitEHT26L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT52L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT52H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT52U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT106L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT106H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT106U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT242L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT242H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT242U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_52L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_52H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_52U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_106L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_106H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT26_106U = ucPwr;

				/*BW40*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_EHT->cPwrLimitEHT484L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT484H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT484U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT484_242L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT484_242H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT484_242U = ucPwr;

				/*BW80*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_EHT->cPwrLimitEHT996L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484_242L
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484_242H
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996_484_242U
					= ucPwr;

				/*BW160*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_EHT->cPwrLimitEHT996X2L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X2H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X2U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X2_484L
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X2_484H
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X2_484U
					= ucPwr;

				/*BW320*/
				PWR_LIMIT_ACCUMULATE(ucPwr, 6);
				prChPwrLimit_EHT->cPwrLimitEHT996X3L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X3H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X3U = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X3_484L
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X3_484H
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X3_484U
					= ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X4L = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X4H = ucPwr;
				prChPwrLimit_EHT->cPwrLimitEHT996X4U = ucPwr;

			} else {
				kalMemSet(&prChPwrLimit_EHT->cPwrLimitEHT26L,
					ucPwr, PWR_LIMIT_EHT_NUM);
			}

			prPerPwrLimit->ucNum++;
			prChPwrLimit_EHT++;
		}
	}
#endif
}

static void rlmDomainBuildConfigPwrLimitPayload_EHT(
	struct ADAPTER *prAdapter,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRFBand)
{
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CHANNEL_POWER_LIMIT_EHT *prChPwrLimit_EHT;
	struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT *prCfgPwrLmt;
	enum ENUM_PWR_LIMIT_CONFIG_BASE eCfgTblBase;
	uint16_t u2CountryCodeTable = COUNTRY_CODE_NULL;
	uint32_t u4ChIdx = 0, u4TableNum = 0, i = 0;
	bool fgChannelValid = FALSE;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {

		prChPwrLimit_EHT = &prPerPwrLimit->u.rEHT[u4ChIdx];
		eCfgTblBase = rlmDomainPwrLmtGetConfigBaseByCh(prAdapter,
			eRFBand, prChPwrLimit_EHT->ucCentralCh);

		prCfgPwrLmt =
			g_rlmPowerLimitConfigTable[eCfgTblBase].EHT.table;
		u4TableNum =
			g_rlmPowerLimitConfigTable[eCfgTblBase].EHT.table_num;

		for (i = 0; i < u4TableNum; i++) {

			WLAN_GET_FIELD_BE16(
				&prCfgPwrLmt[i].aucCountryCode[0],
				&u2CountryCodeTable);

			fgChannelValid =
				rlmDomainCheckChannelEntryValid(
					prAdapter,
					rlmDomainConvertRFBandEnum(eRFBand),
					prCfgPwrLmt[i].i2CentralCh
				);

			if (u2CountryCodeTable == COUNTRY_CODE_NULL)
				break;	/*end of configuration table */
			else if (rlmDomainPwrLmtArbitorPreProccess(
					prAdapter,
					u2CountryCodeTable,
					prPerPwrLimit->u2CountryCode,
					fgChannelValid,
					rlmDomainConvertRFBandEnum(eRFBand),
					prCfgPwrLmt[i].i2CentralCh,
					prChPwrLimit_EHT->ucCentralCh) == FALSE)
				continue;

			PwrLmtTblArbitrator(
				&prChPwrLimit_EHT->cPwrLimitEHT26L,
				&prCfgPwrLmt[i].aucPwrLimit[0],
				PWR_LIMIT_EHT_NUM);
		}
	}
#endif
}

static void rlmDomainDumpPwrLimitPayload_EHT(
	char *message,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CHANNEL_POWER_LIMIT_EHT *prChPwrLimit_EHT;
	char msgLimit[PWR_BUF_LEN];
	uint32_t u4MsgOfs = 0, j = 0, u4ChIdx = 0;
	int8_t *pcRatePwr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_EHT = &prPerPwrLimit->u.rEHT[u4ChIdx];
		pcRatePwr = &prChPwrLimit_EHT->cPwrLimitEHT26L;

		kalMemZero(msgLimit, sizeof(char) * PWR_BUF_LEN);
		u4MsgOfs = 0;

		/*message head*/
		u4MsgOfs +=
			snprintf(msgLimit + u4MsgOfs,
				PWR_BUF_LEN - u4MsgOfs,
				"EHT ch=%d,%s, Limit=",
				prChPwrLimit_EHT->ucCentralCh,
				g_au1TxPwrRFBand[eRF]
			);

		/*message body*/
		for (j = PWR_LIMIT_EHT_RU26_L; j < PWR_LIMIT_EHT_NUM ; j++)
			u4MsgOfs +=
				snprintf(msgLimit + u4MsgOfs,
					PWR_BUF_LEN - u4MsgOfs,
					"%d,",
					*(pcRatePwr + j)
				);

		/*message tail*/
		if (u4MsgOfs >= 1)
			msgLimit[u4MsgOfs - 1] = '\0';
		else
			msgLimit[0] = '\0';

		DBGLOG(RLM, LOUD, "%s:%s\n", message, msgLimit);
	}
#endif
}

static void rlmDomainApplyDynSettings_EHT(
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	struct TX_PWR_CTRL_ELEMENT *prCurElement,
	uint8_t *bandedgeParam,
	enum ENUM_PWR_LIMIT_RF_BAND eRF)
{
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CHANNEL_POWER_LIMIT_EHT *prChPwrLimit_EHT;
	struct TX_PWR_CTRL_CHANNEL_SETTING *prChlSetting = NULL;
	uint32_t u4ChIdx = 0, u4SetIdx = 0, u4OfsIdx = 0;
	uint32_t u4EleSetCount = prCurElement->settingCount;
	int8_t *pcRefValPtr, *pcApplyValPtr;
	bool fgNeedArbitrator = FALSE;
	enum ENUM_TX_POWER_CTRL_VALUE_SIGN *pcApplyOpPtr;

	for (u4ChIdx = 0; u4ChIdx < prPerPwrLimit->ucNum; u4ChIdx++) {
		prChPwrLimit_EHT = &prPerPwrLimit->u.rEHT[u4ChIdx];
		pcRefValPtr = &prChPwrLimit_EHT->cPwrLimitEHT26L;

		for (u4SetIdx = 0; u4SetIdx < u4EleSetCount; u4SetIdx++) {

			prChlSetting = &prCurElement->rChlSettingList[u4SetIdx];

			fgNeedArbitrator =
				rlmDomainIsNeedToDoArbitrator(
					prChPwrLimit_EHT->ucCentralCh,
					prChlSetting->channelParam[0],
					prChlSetting->channelParam[1],
					bandedgeParam,
					prChlSetting->eChnlType,
					eRF
				);

			if (!fgNeedArbitrator)
				continue;

			pcApplyOpPtr =
				prChlSetting->opEHT;
			pcApplyValPtr =
				prChlSetting->i8PwrLimitEHT;

#if (CFG_SUPPORT_WIFI_6G == 1)
			if (eRF == PWR_LIMIT_RF_BAND_6G) {
				pcApplyOpPtr =
					prChlSetting->opEHT_6G;
				pcApplyValPtr =
					prChlSetting->i8PwrLimitEHT_6G;
			}
#endif

			for (u4OfsIdx = PWR_LIMIT_EHT_RU26_L;
				u4OfsIdx < PWR_LIMIT_EHT_NUM;
				u4OfsIdx++) {
				if (pcApplyOpPtr[u4OfsIdx]
					!= PWR_CTRL_TYPE_NO_ACTION) {
					txPwrOperate(prCurElement->eCtrlType,
						pcRefValPtr + u4OfsIdx,
						pcApplyValPtr + u4OfsIdx);
				}
			}
		}
	}
#endif
}

static void rlmDomainWriteTxPwrEmiData_EHT(
	uint32_t channel_index,
	struct SET_COUNTRY_CHANNEL_POWER_LIMIT *prPerPwrLimit,
	char *prTxPowrEmiAddress,
	uint32_t *u4Size
)
{
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
	struct CHANNEL_POWER_LIMIT_EHT *prChPwrLimit_EHT;
	*u4Size = sizeof(struct CHANNEL_POWER_LIMIT_EHT);
	prChPwrLimit_EHT = &prPerPwrLimit->u.rEHT[channel_index];
	kalMemCopy(prTxPowrEmiAddress, prChPwrLimit_EHT, *u4Size);
#endif
}

#endif


#if (CFG_TC10_FEATURE == 1)
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is get each SAR scenario type apply antenna status
 *
 * @param[in] eType : SAR scenario type
 *
 * @return apply antenna status
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmDomainGetSarApplyActAntStatus(
	enum ENUM_SAR_TYPE eType
)
{
	if (eType >= SAR_NUM)
		return 0;

	DBGLOG(RLM, INFO, "[SAR]Sar Scenario Type[%d] apply_ant[%d]\n",
		eType,
		g_ucSarApplyActAnt[eType]);

	return g_ucSarApplyActAnt[eType];
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is disable all sub6 band info
 *
 * @param[in] void
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void rlmDomainSarSub6BandInfoDisableApplyAnt(void)
{
	uint8_t i = 0;

	for (i = SAR_NR_SUB6_BAND_INFO_2; i <= SAR_NR_SUB6_BAND_INFO_78; i++)
		g_ucSarApplyAnt[i] = SAR_APPLY_DISABLE_ANT_ALL;
}

/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is gen SAR bitmap
 *
 * @param[in] eType : SAR cmd type
 * @param[in] eId : Event ID
 * @param[in] ucInfo : other info
 *
 * @return void
 */
/*----------------------------------------------------------------------------*/
void rlmDomainGenSarBitMap(
	enum ENUM_SAR_CMD_TYPE eCmdType,
	enum ENUM_SAR_EVENT_ID eId,
	uint8_t ucInfo)
{
	bool fgSub6BandInfoValid = TRUE;
	bool fgSub6BandInfoApply = FALSE;
	uint8_t i = 0;
	enum ENUM_SAR_TYPE eSarType = SAR_HEAD;

	DBGLOG(RLM, INFO,
		"[SAR]SAR bitmap start: Cmd type[%d]id[%d]info[%d]\n",
		eCmdType,
		eId,
		ucInfo);

	if (eCmdType == SAR_TX_POWER_CALLING) {

		switch (eId) {
		case HEAD_SAR_BACKOFF_DISABLED:
			g_ucSarApplyAnt[SAR_HEAD] &= ~ucInfo;
			break;
		case HEAD_SAR_BACKOFF_ENABLED:
			g_ucSarApplyAnt[SAR_HEAD] |= ucInfo;
			break;
		case BODY_SAR_BACKOFF_DISABLED:
			g_ucSarApplyAnt[SAR_BODY] &= ~ucInfo;
			break;
		case BODY_SAR_BACKOFF_ENABLED:
			g_ucSarApplyAnt[SAR_BODY] |= ucInfo;
			break;
		case NR_MMWAVE_SAR_BACKOFF_DISABLED:
			g_ucSarApplyAnt[SAR_NR_MMWAVE] &= ~ucInfo;
			break;
		case NR_MMWAVE_SAR_BACKOFF_ENABLED:
			g_ucSarApplyAnt[SAR_NR_MMWAVE] |= ucInfo;
			/* mmWave & Sub6/Sub6_Band_info have same priority,
			 * If mmWave is enable, Sub6/Sub6_Band_info
			 * should be disable
			 */
			g_ucSarApplyAnt[SAR_NR_SUB6] &= ~ucInfo;
			rlmDomainSarSub6BandInfoDisableApplyAnt();
			break;
		case NR_SUB6_SAR_BACKOFF_DISABLED:
			g_ucSarApplyAnt[SAR_NR_SUB6] &= ~ucInfo;
			rlmDomainSarSub6BandInfoDisableApplyAnt();
			break;
		case NR_SUB6_SAR_BACKOFF_ENABLED:
			g_ucSarApplyAnt[SAR_NR_SUB6] |= ucInfo;
			/* Sub6 & mmWave/Sub6_Band_info have same priority,
			 * If Sub6 is enable, mmWave/Sub6_Band_info
			 * should be disable
			 */
			g_ucSarApplyAnt[SAR_NR_MMWAVE] &= ~ucInfo;
			rlmDomainSarSub6BandInfoDisableApplyAnt();
			break;
		case SAR_SAR_BACKOFF_DISABLE_ALL:
			for (i = 0; i < SAR_NUM; i++) {
				/* disable ant apply */
				g_ucSarApplyAnt[i] &= ~ucInfo;
			}
			break;
		case MHS_SAR_BACKOFF_DISABLED:
			g_ucSarApplyAnt[SAR_MHS] &= ~ucInfo;
			break;
		case MHS_SAR_BACKOFF_ENABLED:
			g_ucSarApplyAnt[SAR_MHS] |= ucInfo;
			break;
		default:
			DBGLOG(RLM, ERROR, "[SAR]Event ID[%d]not define\n",
						eId);
			break;
		}
	} else if (eCmdType == SAR_TX_POWER_SUB6_BAND) {

		fgSub6BandInfoValid = TRUE;

		switch (ucInfo) {
		case 2:
			eSarType = SAR_NR_SUB6_BAND_INFO_2;
			break;
		case 7:
			eSarType = SAR_NR_SUB6_BAND_INFO_7;
			break;
		case 25:
			eSarType = SAR_NR_SUB6_BAND_INFO_25;
			break;
		case 38:
			eSarType = SAR_NR_SUB6_BAND_INFO_38;
			break;
		case 40:
			eSarType = SAR_NR_SUB6_BAND_INFO_40;
			break;
		case 41:
			eSarType = SAR_NR_SUB6_BAND_INFO_41;
			break;
		case 48:
			eSarType = SAR_NR_SUB6_BAND_INFO_48;
			break;
		case 66:
			eSarType = SAR_NR_SUB6_BAND_INFO_66;
			break;
		case 77:
			eSarType = SAR_NR_SUB6_BAND_INFO_77;
			break;
		case 78:
			eSarType = SAR_NR_SUB6_BAND_INFO_78;
			break;
		default:
			fgSub6BandInfoValid = FALSE;
			DBGLOG(RLM, ERROR, "[SAR]Not support sub6_band[%d]\n",
						ucInfo);
			break;
		}

		if (fgSub6BandInfoValid) {
			/* At any given time, only one sub6_band_info
			 * secnario setting will be enabled.
			 * Therefore, we will first disable all
			 * sub6_band_info settings, and then re-enabled
			 * the setting that needs to be updated.
			 */
			rlmDomainSarSub6BandInfoDisableApplyAnt();
			g_ucSarApplyAnt[eSarType] = SAR_APPLY_ENABLE_ANT_ALL;

			/* Sub6_Band_info & Sub6/mmWave have same priority,
			 * If Sub6_Band_info is enable, Sub6/mmWave
			 * should be disable.
			 */
			g_ucSarApplyAnt[SAR_NR_SUB6] =
						SAR_APPLY_DISABLE_ANT_ALL;
			g_ucSarApplyAnt[SAR_NR_MMWAVE] =
						SAR_APPLY_DISABLE_ANT_ALL;
		}
	}

	kalMemCopy(g_ucSarApplyActAnt,
			g_ucSarApplyAnt,
			sizeof(g_ucSarApplyActAnt));

	/* mmWave/Sub6/Sub6_band_info have higher priority than MHS,
	 * MHS settinngs can be enable only when all mmWave/Sub6/Sub6_band_info
	 * is disable.
	 */
	for (i = SAR_NR_SUB6_BAND_INFO_2; i <= SAR_NR_SUB6_BAND_INFO_78; i++) {
		if (g_ucSarApplyAnt[i] != SAR_APPLY_DISABLE_ANT_ALL) {
			fgSub6BandInfoApply = TRUE;
			break;
		}
	}

	if (g_ucSarApplyAnt[SAR_NR_MMWAVE] != SAR_APPLY_DISABLE_ANT_ALL ||
	    g_ucSarApplyAnt[SAR_NR_SUB6] != SAR_APPLY_DISABLE_ANT_ALL ||
		fgSub6BandInfoApply == TRUE) {
		g_ucSarApplyActAnt[SAR_MHS] = SAR_APPLY_DISABLE_ANT_ALL;
	}
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is use get SAR scenario name
 *
 * @param[in] eType : SAR scenario type
 *
 * @return char * : SAR scenario name
 */
/*----------------------------------------------------------------------------*/
char *rlmDomainGetSarScenarioName(enum ENUM_SAR_TYPE eType)
{
	if (eType >= SAR_NUM)
		return NULL;

	return g_sarScenarioName[eType];
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is use check is SAR event concurrent
 *
 * @param[in] void
 *
 * @return uint8_t : TRUE : conncurrent, FALSE : no concurrent
 */
/*----------------------------------------------------------------------------*/
uint8_t rlmDomainIsSarEventConcurrent(uint32_t *pu4BitMap)
{
	uint8_t i = 0;
	uint8_t ucCnt = 0;

	*pu4BitMap = 0;

	for (i = 0; i < SAR_NUM; i++) {
		if (rlmDomainGetSarApplyActAntStatus(i) != 0) {
			*pu4BitMap |= BIT(i);
			ucCnt++;
			DBGLOG(RLM, INFO,
				"[SAR]Scenario Type[%d]name[%s]enable, total[%d]\n",
				i,
				rlmDomainGetSarScenarioName(i),
				ucCnt);
		}
	}

	if (ucCnt >= 2)
		return TRUE;

	return FALSE;
}
/*----------------------------------------------------------------------------*/
/*!
 * @brief This function is use get SAR sub-index by Bit Map
 *
 * @param[in] pu4SubIdx : pointer of sub-index
 *
 * @return sub index
 */
/*----------------------------------------------------------------------------*/
int32_t rlmDomainSarGetRemapSubIdx(uint16_t *pu2SubIdx)
{
	uint32_t i = 0;
	uint32_t u4Size = (sizeof(g_SarSubIndxReMapTbl) /
				sizeof(struct SAR_SUB_IDX_REMAP_TBL_INFO));
	uint8_t ucAntAppy = SAR_APPLY_DISABLE_ANT_ALL;
	uint16_t u2SubIdxOfst = 0;
	uint32_t u4BitMap = 0;

	if (rlmDomainIsSarEventConcurrent(&u4BitMap)) {
		ucAntAppy = SAR_APPLY_ENABLE_ANT_ALL;
	} else {
		for (i = 0; i < SAR_NUM; i++) {
			if (rlmDomainGetSarApplyActAntStatus(i) != 0) {
				ucAntAppy = rlmDomainGetSarApplyActAntStatus(i);
				DBGLOG(RLM, INFO,
				"[SAR]Scenario Type[%d]Name[%s]ant_apply[%d]\n",
				i,
				rlmDomainGetSarScenarioName(i),
				ucAntAppy);
				break;
			}
		}
	}

	DBGLOG(RLM, INFO, "[SAR]bit map = 0x%X\n", u4BitMap);

	if (ucAntAppy == SAR_APPLY_ENABLE_ANT_ALL)
		u2SubIdxOfst = 0;
	else if (ucAntAppy == SAR_APPLY_ENABLE_ONLY_WF0)
		u2SubIdxOfst = 1000;
	else if (ucAntAppy == SAR_APPLY_ENABLE_ONLY_WF1)
		u2SubIdxOfst = 2000;

	for (i = 0; i < u4Size; i++) {
		if (u4BitMap == g_SarSubIndxReMapTbl[i].u4BitMap) {

			if (ucAntAppy == SAR_APPLY_DISABLE_ANT_ALL) {
				*pu2SubIdx = 0;
			} else {
				*pu2SubIdx = g_SarSubIndxReMapTbl[i].u2SubIdx +
								u2SubIdxOfst;
			}

			DBGLOG(RLM, INFO,
			    "[SAR]BitMap[%d]ant_apply[%d]idx[%d]->final idx[%d]\n",
			    u4BitMap,
			    g_SarSubIndxReMapTbl[i].u2SubIdx,
			    ucAntAppy,
			    *pu2SubIdx);

			return WLAN_STATUS_SUCCESS;
		}
	}

	DBGLOG(RLM, ERROR, "[SAR]BitMap[%d], No match subIdx\n", u4BitMap);

	return WLAN_STATUS_FAILURE;
}
#endif /* CFG_TC10_FEATURE */
