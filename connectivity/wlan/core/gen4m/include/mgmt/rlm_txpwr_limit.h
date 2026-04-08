/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_tx_pwr_limit.h"
 *    \brief
 */

#ifndef _RLM_TX_PWR_LIMIT_H
#define _RLM_TX_PWR_LIMIT_H

#if (CFG_SUPPORT_PWR_LMT_EMI == 0)

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */


/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum ENUM_PWR_LIMIT_TYPE {
	PWR_LIMIT_TYPE_COMP_11AC = 0,
	PWR_LIMIT_TYPE_COMP_11AC_V2 = 1,
	PWR_LIMIT_TYPE_COMP_11AX = 2,
	PWR_LIMIT_TYPE_COMP_ANT = 3,
	PWR_LIMIT_TYPE_COMP_6E_1 = 4,
	PWR_LIMIT_TYPE_COMP_6E_2 = 5,
	PWR_LIMIT_TYPE_COMP_6E_3 = 6,
	PWR_LIMIT_TYPE_COMP_ANT_V2 = 7,
	PWR_LIMIT_TYPE_COMP_11AX_BW160 = 8,
	PWR_LIMIT_TYPE_COMP_11BE_1 = 9,
	PWR_LIMIT_TYPE_COMP_11BE_2 = 10,
	PWR_LIMIT_TYPE_COMP_11BE_6G_1 = 11,
	PWR_LIMIT_TYPE_COMP_11BE_6G_2 = 12,
	PWR_LIMIT_TYPE_COMP_11BE_6G_3 = 13,
	PWR_LIMIT_TYPE_COMP_11BE_6G_4 = 14,
	PWR_LIMIT_TYPE_COMP_11BE_6G_5 = 15,
	PWR_LIMIT_TYPE_COMP_11BE_6G_6 = 16,
	PWR_LIMIT_TYPE_COMP_LEGACY_6G_1 = 17,
	PWR_LIMIT_TYPE_COMP_LEGACY_6G_2 = 18,
	PWR_LIMIT_TYPE_COMP_LEGACY_6G_3 = 19,
	PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_1 = 20,
	PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_2 = 21,
	PWR_LIMIT_TYPE_COMP_LEGACY_V2_6G_3 = 22,
	PWR_LIMIT_TYPE_COMP_NUM,
};

enum ENUM_POWER_LIMIT {
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	PWR_LIMIT_CCK_L,
	PWR_LIMIT_CCK_H,
	PWR_LIMIT_OFDM_L,
	PWR_LIMIT_OFDM_H,
#else
	PWR_LIMIT_CCK,
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
	PWR_LIMIT_20M_L,
	PWR_LIMIT_20M_H,
	PWR_LIMIT_40M_L,
	PWR_LIMIT_40M_H,
	PWR_LIMIT_80M_L,
	PWR_LIMIT_80M_H,
	PWR_LIMIT_160M_L,
	PWR_LIMIT_160M_H,
	PWR_LIMIT_NUM,
	PWR_LIMIT_LEGACY_6G_NUM = PWR_LIMIT_NUM,
};

enum ENUM_POWER_LIMIT_HE {
	PWR_LIMIT_RU26_L, /* MCS0~4 */
	PWR_LIMIT_RU26_H, /* MCS5~9 */
	PWR_LIMIT_RU26_U, /* MCS10~11 */

	PWR_LIMIT_RU52_L, /* MCS0~4 */
	PWR_LIMIT_RU52_H, /* MCS5~9 */
	PWR_LIMIT_RU52_U, /* MCS10~11 */

	PWR_LIMIT_RU106_L, /* MCS0~4 */
	PWR_LIMIT_RU106_H, /* MCS5~9 */
	PWR_LIMIT_RU106_U, /* MCS10~11 */

	PWR_LIMIT_RU242_L, /* MCS0~4 */
	PWR_LIMIT_RU242_H, /* MCS5~9 */
	PWR_LIMIT_RU242_U, /* MCS10~11 */

	PWR_LIMIT_RU484_L, /* MCS0~4 */
	PWR_LIMIT_RU484_H, /* MCS5~9 */
	PWR_LIMIT_RU484_U, /* MCS10~11 */

	PWR_LIMIT_RU996_L, /* MCS0~4 */
	PWR_LIMIT_RU996_H, /* MCS5~9 */
	PWR_LIMIT_RU996_U, /* MCS10~11 */
	PWR_LIMIT_HE_NUM,
	PWR_LIMIT_RU1992_L = PWR_LIMIT_HE_NUM, /* MCS0~4 */
	PWR_LIMIT_RU1992_H, /* MCS5~9 */
	PWR_LIMIT_RU1992_U, /* MCS10~11 */
	PWR_LIMIT_6E_NUM,
	PWR_LIMIT_HE_BW160_NUM = PWR_LIMIT_6E_NUM,
};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
enum ENUM_POWER_LIMIT_EHT {
	PWR_LIMIT_EHT26_L, /* MCS0~4 */
	PWR_LIMIT_EHT26_H, /* MCS5~9 */
	PWR_LIMIT_EHT26_U, /* MCS10~15 */

	PWR_LIMIT_EHT52_L, /* MCS0~4 */
	PWR_LIMIT_EHT52_H, /* MCS5~9 */
	PWR_LIMIT_EHT52_U, /* MCS10~15 */

	PWR_LIMIT_EHT106_L, /* MCS0~4 */
	PWR_LIMIT_EHT106_H, /* MCS5~9 */
	PWR_LIMIT_EHT106_U, /* MCS10~15 */

	PWR_LIMIT_EHT242_L, /* MCS0~4 */
	PWR_LIMIT_EHT242_H, /* MCS5~9 */
	PWR_LIMIT_EHT242_U, /* MCS10~15 */

	PWR_LIMIT_EHT484_L, /* MCS0~4 */
	PWR_LIMIT_EHT484_H, /* MCS5~9 */
	PWR_LIMIT_EHT484_U, /* MCS10~15 */

	PWR_LIMIT_EHT996_L, /* MCS0~4 */
	PWR_LIMIT_EHT996_H, /* MCS5~9 */
	PWR_LIMIT_EHT996_U, /* MCS10~15 */

	PWR_LIMIT_EHT996X2_L, /* MCS0~4 */
	PWR_LIMIT_EHT996X2_H, /* MCS5~9 */
	PWR_LIMIT_EHT996X2_U, /* MCS10~15 */

	PWR_LIMIT_EHT26_52_L, /* MCS0~4 */
	PWR_LIMIT_EHT26_52_H, /* MCS5~9 */
	PWR_LIMIT_EHT26_52_U, /* MCS10~15 */

	PWR_LIMIT_EHT26_106_L, /* MCS0~4 */
	PWR_LIMIT_EHT26_106_H, /* MCS5~9 */
	PWR_LIMIT_EHT26_106_U, /* MCS10~15 */

	PWR_LIMIT_EHT484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT996_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT996_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT996_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT996_484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT996_484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT996_484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_NUM,
};
#if (CFG_SUPPORT_WIFI_6G == 1)
enum ENUM_POWER_LIMIT_EHT_6G {
	PWR_LIMIT_EHT_6G_26_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_26_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_26_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_52_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_52_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_52_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_106_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_106_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_106_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996X2_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996X2_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996X2_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996X4_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996X4_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996X4_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_26_52_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_26_52_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_26_52_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_26_106_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_26_106_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_26_106_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996_484_242_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996_484_242_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996_484_242_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996X2_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996X2_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996X2_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996X3_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996X3_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996X3_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_996X3_484_L, /* MCS0~4 */
	PWR_LIMIT_EHT_6G_996X3_484_H, /* MCS5~9 */
	PWR_LIMIT_EHT_6G_996X3_484_U, /* MCS10~15 */

	PWR_LIMIT_EHT_6G_NUM,
};

#endif /* CFG_SUPPORT_WIFI_6G */
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if (CFG_SUPPORT_WIFI_6G == 1)
struct CMD_CHANNEL_POWER_LIMIT_6E {
	uint8_t ucCentralCh;
	int8_t cPwrLimitRU26L; /* MCS0~4 */
	int8_t cPwrLimitRU26H; /* MCS5~9 */
	int8_t cPwrLimitRU26U; /* MCS10~11 */

	int8_t cPwrLimitRU52L; /* MCS0~4 */
	int8_t cPwrLimitRU52H; /* MCS5~9 */
	int8_t cPwrLimitRU52U; /* MCS10~11 */

	int8_t cPwrLimitRU106L; /* MCS0~4 */
	int8_t cPwrLimitRU106H; /* MCS5~9 */
	int8_t cPwrLimitRU106U; /* MCS10~11 */
	/*RU242/SU20*/
	int8_t cPwrLimitRU242L; /* MCS0~4 */
	int8_t cPwrLimitRU242H; /* MCS5~9 */
	int8_t cPwrLimitRU242U; /* MCS10~11 */
	/*RU484/SU40*/
	int8_t cPwrLimitRU484L; /* MCS0~4 */
	int8_t cPwrLimitRU484H; /* MCS5~9 */
	int8_t cPwrLimitRU484U; /* MCS10~11 */
	/*RU996/SU80*/
	int8_t cPwrLimitRU996L; /* MCS0~4 */
	int8_t cPwrLimitRU996H; /* MCS5~9 */
	int8_t cPwrLimitRU996U; /* MCS10~11 */
	/*RU1992/SU160*/
	int8_t cPwrLimitRU1992L; /* MCS0~4 */
	int8_t cPwrLimitRU1992H; /* MCS5~9 */
	int8_t cPwrLimitRU1992U; /* MCS10~11 */

	uint8_t ucFlag;
	uint8_t ucValid;
};

struct CMD_CHANNEL_POWER_LIMIT_LEGACY_6G {
	uint8_t ucCentralCh;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	int8_t cPwrLimitCCK_L; /* CCK_L, 1M,2M */
	int8_t cPwrLimitCCK_H; /* CCK_H, 5.5M,11M */
	int8_t cPwrLimitOFDM_L; /* OFDM_L,  6M ~ 18M */
	int8_t cPwrLimitOFDM_H; /* OFDM_H, 24M ~ 54M */
#else
	int8_t cPwrLimitCCK;
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
	int8_t cPwrLimit20L; /* MCS0~4 */
	int8_t cPwrLimit20H; /* MCS5~8 */
	int8_t cPwrLimit40L; /* MCS0~4 */
	int8_t cPwrLimit40H; /* MCS5~9 */
	int8_t cPwrLimit80L; /* MCS0~4 */
	int8_t cPwrLimit80H; /* MCS5~9 */
	int8_t cPwrLimit160L; /* MCS0~4 */
	int8_t cPwrLimit160H; /* MCS5~9 */

	uint8_t ucFlag; /*Not used in driver*/
	uint8_t ucValid;
};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
struct CMD_CHANNEL_POWER_LIMIT_EHT_6G {
	uint8_t ucCentralCh;
	int8_t cPwrLimitEHT26L; /* MCS0~4 */
	int8_t cPwrLimitEHT26H; /* MCS5~9 */
	int8_t cPwrLimitEHT26U; /* MCS10~15 */

	int8_t cPwrLimitEHT52L; /* MCS0~4 */
	int8_t cPwrLimitEHT52H; /* MCS5~9 */
	int8_t cPwrLimitEHT52U; /* MCS10~15 */

	int8_t cPwrLimitEHT106L; /* MCS0~4 */
	int8_t cPwrLimitEHT106H; /* MCS5~9 */
	int8_t cPwrLimitEHT106U; /* MCS10~15 */
	/*RU242/SU20*/
	int8_t cPwrLimitEHT242L; /* MCS0~4 */
	int8_t cPwrLimitEHT242H; /* MCS5~9 */
	int8_t cPwrLimitEHT242U; /* MCS10~15 */
	/*RU484/SU40*/
	int8_t cPwrLimitEHT484L; /* MCS0~4 */
	int8_t cPwrLimitEHT484H; /* MCS5~9 */
	int8_t cPwrLimitEHT484U; /* MCS10~15 */
	/*RU996/SU80*/
	int8_t cPwrLimitEHT996L; /* MCS0~4 */
	int8_t cPwrLimitEHT996H; /* MCS5~9 */
	int8_t cPwrLimitEHT996U; /* MCS10~15 */
	/*RU1992/SU160*/
	int8_t cPwrLimitEHT996X2L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X2H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X2U; /* MCS10~15 */
	/*RU1992/SU320*/
	int8_t cPwrLimitEHT996X4L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X4H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X4U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_52L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_52H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_52U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_106L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_106H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_106U; /* MCS10~15 */

	int8_t cPwrLimitEHT484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT484_242U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484_242U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X2_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X2_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X2_484U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X3L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X3H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X3U; /* MCS10~15 */

	int8_t cPwrLimitEHT996X3_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X3_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X3_484U; /* MCS10~15 */

	uint8_t ucFlag;
	uint8_t ucValid;

};
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

/* CMD_SET_PWR_LIMIT_TABLE */
struct CMD_CHANNEL_POWER_LIMIT {
	uint8_t ucCentralCh;
#if (CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING == 1)
	int8_t cPwrLimitCCK_L; /* CCK_L, 1M,2M */
	int8_t cPwrLimitCCK_H; /* CCK_H, 5.5M,11M */
	int8_t cPwrLimitOFDM_L; /* OFDM_L,  6M ~ 18M */
	int8_t cPwrLimitOFDM_H; /* OFDM_H, 24M ~ 54M */
#else
	int8_t cPwrLimitCCK;
#endif /* CFG_SUPPORT_DYNA_TX_PWR_CTRL_11AC_V2_SETTING */
	int8_t cPwrLimit20L; /* MCS0~4 */
	int8_t cPwrLimit20H; /* MCS5~8 */
	int8_t cPwrLimit40L; /* MCS0~4 */
	int8_t cPwrLimit40H; /* MCS5~9 */
	int8_t cPwrLimit80L; /* MCS0~4 */
	int8_t cPwrLimit80H; /* MCS5~9 */
	int8_t cPwrLimit160L; /* MCS0~4 */
	int8_t cPwrLimit160H; /* MCS5~9 */

	uint8_t ucFlag; /*Not used in driver*/
	uint8_t ucValid;
};
struct CMD_CHANNEL_POWER_LIMIT_HE { /*HE SU design*/
	uint8_t ucCentralCh;
	int8_t cPwrLimitRU26L; /* MCS0~4 */
	int8_t cPwrLimitRU26H; /* MCS5~9 */
	int8_t cPwrLimitRU26U; /* MCS10~11 */

	int8_t cPwrLimitRU52L; /* MCS0~4 */
	int8_t cPwrLimitRU52H; /* MCS5~9 */
	int8_t cPwrLimitRU52U; /* MCS10~11 */

	int8_t cPwrLimitRU106L; /* MCS0~4 */
	int8_t cPwrLimitRU106H; /* MCS5~9 */
	int8_t cPwrLimitRU106U; /* MCS10~11 */
	/*RU242/SU20*/
	int8_t cPwrLimitRU242L; /* MCS0~4 */
	int8_t cPwrLimitRU242H; /* MCS5~9 */
	int8_t cPwrLimitRU242U; /* MCS10~11 */
	/*RU484/SU40*/
	int8_t cPwrLimitRU484L; /* MCS0~4 */
	int8_t cPwrLimitRU484H; /* MCS5~9 */
	int8_t cPwrLimitRU484U; /* MCS10~11 */
	/*RU996/SU80*/
	int8_t cPwrLimitRU996L; /* MCS0~4 */
	int8_t cPwrLimitRU996H; /* MCS5~9 */
	int8_t cPwrLimitRU996U; /* MCS10~11 */

	uint8_t ucFlag;
	uint8_t ucValid;

};

struct CMD_CHANNEL_POWER_LIMIT_HE_BW160 { /*HE SU design*/
	uint8_t ucCentralCh;
	int8_t cPwrLimitRU26L; /* MCS0~4 */
	int8_t cPwrLimitRU26H; /* MCS5~9 */
	int8_t cPwrLimitRU26U; /* MCS10~11 */

	int8_t cPwrLimitRU52L; /* MCS0~4 */
	int8_t cPwrLimitRU52H; /* MCS5~9 */
	int8_t cPwrLimitRU52U; /* MCS10~11 */

	int8_t cPwrLimitRU106L; /* MCS0~4 */
	int8_t cPwrLimitRU106H; /* MCS5~9 */
	int8_t cPwrLimitRU106U; /* MCS10~11 */
	/*RU242/SU20*/
	int8_t cPwrLimitRU242L; /* MCS0~4 */
	int8_t cPwrLimitRU242H; /* MCS5~9 */
	int8_t cPwrLimitRU242U; /* MCS10~11 */
	/*RU484/SU40*/
	int8_t cPwrLimitRU484L; /* MCS0~4 */
	int8_t cPwrLimitRU484H; /* MCS5~9 */
	int8_t cPwrLimitRU484U; /* MCS10~11 */
	/*RU996/SU80*/
	int8_t cPwrLimitRU996L; /* MCS0~4 */
	int8_t cPwrLimitRU996H; /* MCS5~9 */
	int8_t cPwrLimitRU996U; /* MCS10~11 */
	/*RU1992/SU160*/
	int8_t cPwrLimitRU1992L; /* MCS0~4 */
	int8_t cPwrLimitRU1992H; /* MCS5~9 */
	int8_t cPwrLimitRU1992U; /* MCS10~11 */

	uint8_t ucFlag;
	uint8_t ucValid;

};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
struct CMD_CHANNEL_POWER_LIMIT_EHT { /*HE SU design*/
	uint8_t ucCentralCh;
	int8_t cPwrLimitEHT26L; /* MCS0~4 */
	int8_t cPwrLimitEHT26H; /* MCS5~9 */
	int8_t cPwrLimitEHT26U; /* MCS10~15 */

	int8_t cPwrLimitEHT52L; /* MCS0~4 */
	int8_t cPwrLimitEHT52H; /* MCS5~9 */
	int8_t cPwrLimitEHT52U; /* MCS10~15 */

	int8_t cPwrLimitEHT106L; /* MCS0~4 */
	int8_t cPwrLimitEHT106H; /* MCS5~9 */
	int8_t cPwrLimitEHT106U; /* MCS10~15 */
	/*RU242/SU20*/
	int8_t cPwrLimitEHT242L; /* MCS0~4 */
	int8_t cPwrLimitEHT242H; /* MCS5~9 */
	int8_t cPwrLimitEHT242U; /* MCS10~15 */
	/*RU484/SU40*/
	int8_t cPwrLimitEHT484L; /* MCS0~4 */
	int8_t cPwrLimitEHT484H; /* MCS5~9 */
	int8_t cPwrLimitEHT484U; /* MCS10~15 */
	/*RU996/SU80*/
	int8_t cPwrLimitEHT996L; /* MCS0~4 */
	int8_t cPwrLimitEHT996H; /* MCS5~9 */
	int8_t cPwrLimitEHT996U; /* MCS10~15 */
	/*RU1992/SU160*/
	int8_t cPwrLimitEHT996X2L; /* MCS0~4 */
	int8_t cPwrLimitEHT996X2H; /* MCS5~9 */
	int8_t cPwrLimitEHT996X2U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_52L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_52H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_52U; /* MCS10~15 */

	int8_t cPwrLimitEHT26_106L; /* MCS0~4 */
	int8_t cPwrLimitEHT26_106H; /* MCS5~9 */
	int8_t cPwrLimitEHT26_106U; /* MCS10~15 */

	int8_t cPwrLimitEHT484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT484_242U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484U; /* MCS10~15 */

	int8_t cPwrLimitEHT996_484_242L; /* MCS0~4 */
	int8_t cPwrLimitEHT996_484_242H; /* MCS5~9 */
	int8_t cPwrLimitEHT996_484_242U; /* MCS10~15 */

	uint8_t ucFlag;
	uint8_t ucValid;

};
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	/* Note: this array doesn't include cPwrLimitOFDM_L & cPwrLimitOFDM_H */
	int8_t aucPwrLimit[PWR_LIMIT_NUM];
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_HE_NUM];
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_HE_BW160 {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_HE_BW160_NUM];
};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_EHT_NUM];
};
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */

#if (CFG_SUPPORT_WIFI_6G == 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_6E {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_6E_NUM];
};

struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_LEGACY_6G {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_LEGACY_6G_NUM];
};

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
struct COUNTRY_POWER_LIMIT_TABLE_CONFIGURATION_EHT_6G {
	uint8_t aucCountryCode[2];
	int16_t i2CentralCh;
	int8_t aucPwrLimit[PWR_LIMIT_EHT_6G_NUM];
};
#endif /* CFG_SUPPORT_PWR_LIMIT_EHT */
#endif /* CFG_SUPPORT_WIFI_6G */

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/* backward compatible for txPwrCtrlStringToStruct parser use. */
#define PWR_LIMIT_PARSER_LEGACY_NUM (PWR_LIMIT_NUM)
#define PWR_LIMIT_PARSER_HE_NUM (PWR_LIMIT_HE_NUM)
#define PWR_LIMIT_PARSER_HE160_NUM (PWR_LIMIT_6E_NUM)

#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_LIMIT_PARSER_EHT_NUM (PWR_LIMIT_EHT_NUM)
#if (CFG_SUPPORT_WIFI_6G == 1)
#define PWR_LIMIT_PARSER_EHT6G_NUM (PWR_LIMIT_EHT_6G_NUM)
#endif
#endif

/* backward compatible for struct TX_PWR_CTRL_CHANNEL_SETTING use*/
#define PWR_LIMIT_DYN_LEGACY_NUM (PWR_LIMIT_NUM)
#define PWR_LIMIT_DYN_LEGACY_6G_NUM (PWR_LIMIT_LEGACY_6G_NUM)
#define PWR_LIMIT_DYN_HE_NUM (PWR_LIMIT_HE_BW160_NUM)
#define PWR_LIMIT_DYN_HE_6G_NUM (PWR_LIMIT_6E_NUM)
#if (CFG_SUPPORT_PWR_LIMIT_EHT == 1)
#define PWR_LIMIT_DYN_EHT_NUM (PWR_LIMIT_EHT_NUM)
#define PWR_LIMIT_DYN_EHT_6G_NUM (PWR_LIMIT_EHT_6G_NUM)
#endif
/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

#endif /*CFG_SUPPORT_PWR_LMT_EMI == 0 && CFG_SUPPORT_PWR_LIMIT_COUNTRY == 1*/
#endif /*_RLM_TX_PWR_LIMIT_EMI_H*/
