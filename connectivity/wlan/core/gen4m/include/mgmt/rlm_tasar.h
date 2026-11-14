/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm_tasar.h"
 *    \brief
 */


#ifndef _RLM_TASAR_H
#define _RLM_TASAR_H

#if (CFG_SUPPORT_TAS_HOST_CONTROL == 1)
/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "precomp.h"
#include "rlm_tasar_data.h"
/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */

struct tasar_country {
	unsigned char aucCountryCode[2];
};
struct tasar_country_tbl_all {
	struct tasar_country *tbl;
	unsigned int cnt;
	char *ucFileName;
};
struct tasar_scenrio_ctrl {
	unsigned int u4RegTblIndex;
	unsigned short int u2CountryCode;
	unsigned int u4Eci;
};
/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */

/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************s
 */

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */
#define REG_TBL_REG(_table, _fileName) \
	{(_table), (ARRAY_SIZE((_table))), (_fileName)}

#define acc(_ctx) \
	(tasarDataExtract(_ctx, ARRAY_SIZE(_ctx)))

/*VERSION*/
#define TAS_VERSION(_config) \
	(acc(_config->version))

/*tasar_algorithm_common*/
#define CONN_STATUS_RESEND_INTERVAL(_config) \
	(acc(_config->common.ucConnStatusResendInterval))
#define CONN_STATUS_RESEND_TIMES(_config) \
	(acc(_config->common.ucConnStatusResendtimes))
#define SCF_UART_ERR_MARGIN(_config) \
	(acc(_config->common.ucScfUartErrMargin))
#define TIME_WD_0_SEC(_config) \
	(_config->common.ucTimeWindow0Sec)
#define TIME_WD_2_SEC(_config) \
	(_config->common.ucTimeWindow2Sec)
#define TIME_WD_4_SEC(_config) \
	(_config->common.ucTimeWindow4Sec)
#define TIME_WD_30_SEC(_config) \
	(_config->common.ucTimeWindow30Sec)
#define TIME_WD_60_SEC(_config) \
	(_config->common.ucTimeWindow60Sec)
#define TIME_WD_100_SEC(_config) \
	(_config->common.ucTimeWindow100Sec)
#define TIME_WD_360_SEC(_config) \
	(_config->common.ucTimeWindow360Sec)

/*tasar_algorithm_reg_specific*/
#define SCF_CHIPS_LAT_MARGIN(_config) \
	(acc(_config->reg_specific.ucScfChipsLatMargin))
#define SCF_DELIMIT_MARGIN(_config) \
	(acc(_config->reg_specific.ucScfDeLimitMargin))
#define SCF_MD_DELIMIT(_config) \
	(acc(_config->reg_specific.ucScfMdDeLimit))
#define SCF_CONN_DELIMIT(_config) \
	(acc(_config->reg_specific.ucScConnDeLimit))
#define MD_MAX_REF_PERIOD(_config) \
	(acc(_config->reg_specific.ucMdMaxRefPeriod))
#define MD_MAX_POWER(_config) \
	(acc(_config->reg_specific.ucMdMaxPower))
#define JOINT_MODE(_config) \
	(_config->reg_specific.ucJointMode)
#define SCF_CONN_OFF_MARGIN(_config) \
	(acc(_config->reg_specific.ucScfConnOffMargin))
#define SCF_MD_OFF_MARGIN(_config) \
	(acc(_config->reg_specific.ucScfMdOffMargin))
#define SCF_CHG_INSTANT_EN(_config) \
	(_config->reg_specific.ucScfChgInstantEn)
#define IS_FBO_EN(_config) \
	(_config->reg_specific.ucIsFboEn)
#define BT_TA_VER(_config) \
	(_config->reg_specific.ucBtTaVer)
#define WIFI_TA_VER(_config) \
	(_config->reg_specific.ucWifiTaVer)
#define SCF_SUB6_EXP_TH(_config) \
	(acc(_config->reg_specific.ucScfSub6ExpireThresh))
#define SMOOTH_CHG_SPEED_SCALE(_config) \
	(_config->reg_specific.ucSmoothChgSpeedScale)
#define REGULATORY(_config) \
	(_config->reg_specific.ucRegulatory)
#define MAX_SCF_WF_FLIGHT(_config) \
	(_config->reg_specific.ucMaxScfWfFlight)
#define MAX_SCF_BT_FLIGHT(_config) \
	(_config->reg_specific.ucMaxScfBTFlight)
#define MAX_SCF_CONN_FLIGHT(_config) \
	(_config->reg_specific.ucMaxScfConnFlight)

/*tasar_wifi_channel_group*/
#define WIFI_CHANNEL_GRP_BAND(_config, _grp) \
	(_config->chngrp[_grp].ucBand)
#define WIFI_CHANNEL_GRP_LOW_CH(_config, _grp) \
	(_config->chngrp[_grp].ucLowChannel)
#define WIFI_CHANNEL_GRP_UP_CH(_config, _grp) \
	(_config->chngrp[_grp].ucUppChannel)
#define WIFI_CHANNEL_GRP_SISO_MIMO_DELTA(_config, _grp) \
	(_config->chngrp[_grp].ucSisoMimoDelta)

/*tasar_scf_factor*/
#define WIFI_PLIMIT_SCF_SUB6(_config, _eci) \
	(_config->plimit[_eci].scf_factor.ucSub6Cust)
#define WIFI_PLIMIT_SCF_MMW6(_config, _eci) \
	(_config->plimit[_eci].scf_factor.ucMmw6Cust)
#define WIFI_PLIMIT_SCF_WF(_config, _eci) \
	(_config->plimit[_eci].scf_factor.ucWifiCust)
#define WIFI_PLIMIT_SCF_BT(_config, _eci) \
	(_config->plimit[_eci].scf_factor.ucBTCust)

/*tasar_wifi_plimit*/
#define WIFI_PLIMIT_WF0_ANT0(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF0Ant0)
#define WIFI_PLIMIT_WF0_ANT1(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF0Ant1)
#define WIFI_PLIMIT_WF1_ANT0(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF1Ant0)
#define WIFI_PLIMIT_WF1_ANT1(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF1Ant1)
#define WIFI_PLIMIT_WF2_ANT0(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF2Ant0)
#define WIFI_PLIMIT_WF2_ANT1(_config, _grp, _eci) \
	(_config->plimit[_eci].wifi_plimit[_grp].ucWF2Ant1)
/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */
void tasarInit(struct ADAPTER *prAdapter);
uint32_t tasarDataExtract(uint8_t *ctx, uint32_t cnt);
void tasarUpdateScenrio(struct ADAPTER *prAdapter,
	struct tasar_scenrio_ctrl rScenrio);

#endif /* CFG_SUPPORT_TAS_HOST_CONTROL == 1 */

#endif /* _RLM_TASAR_H */
