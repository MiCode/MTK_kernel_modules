/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#ifndef __BTMTK_TASAR_H__
#define __BTMTK_TASAR_H__


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */
#include "btmtk_main.h"
#include "rlm_tasar_data.h"
#include <linux/types.h>

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
    struct tasar_country* tbl;
    unsigned int cnt;
    char *ucFileName;
};
struct tasar_scenrio_ctrl {
    unsigned int u4RegTblIndex;
    unsigned short int u2CountryCode;
    unsigned int u4Eci;
};
struct btmtk_tasar {
	bool is_enable;
	struct tasar_config rTasarCfg;
	struct tasar_scenrio_ctrl rTasarScenrio;
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

#define REG_TBL_REG(_table, _fileName)	{(_table), (ARRAY_SIZE((_table))), (_fileName)}
#define ACC(_ctx) (btmtk_tasar_data_extract(_ctx, ARRAY_SIZE(_ctx)))

/* VERSION */
#define TAS_VERSION(_config) \
    (ACC((_config)->version))

/* tasar_algorithm_common */
#define CONN_STATUS_RESEND_INTERVAL(_config) \
    (ACC(_config->common.ucConnStatusResendInterval))
#define CONN_STATUS_RESEND_TIMES(_config) \
    (ACC(_config->common.ucConnStatusResendtimes))
#define SCF_UART_ERR_MARGIN(_config) \
    (ACC(_config->common.ucScfUartErrMargin))
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

/* tasar_algorithm_reg_specific */
#define SCF_CHIPS_LAT_MARGIN(_config) \
    (ACC(_config->reg_specific.ucScfChipsLatMargin))
#define SCF_DELIMIT_MARGIN(_config) \
    (ACC(_config->reg_specific.ucScfDeLimitMargin))
#define SCF_MD_DELIMIT(_config) \
    (ACC(_config->reg_specific.ucScfMdDeLimit))
#define SCF_CONN_DELIMIT(_config) \
    (ACC(_config->reg_specific.ucScConnDeLimit))
#define MD_MAX_REF_PERIOD(_config) \
    (ACC(_config->reg_specific.ucMdMaxRefPeriod))
#define MD_MAX_POWER(_config) \
    (ACC(_config->reg_specific.ucMdMaxPower))
#define JOINT_MODE(_config) \
    (_config->reg_specific.ucJointMode)
#define SCF_CONN_OFF_MARGIN(_config) \
    (ACC(_config->reg_specific.ucScfConnOffMargin))
#define SCF_MD_OFF_MARGIN(_config) \
    (ACC(_config->reg_specific.ucScfMdOffMargin))
#define SCF_CHG_INSTANT_EN(_config) \
    (_config->reg_specific.ucScfChgInstantEn)
#define IS_FBO_EN(_config) \
		(_config->reg_specific.ucIsFboEn)
#define BT_TA_VER(_config) \
		(_config->reg_specific.ucBtTaVer)
#define WIFI_TA_VER(_config) \
		(_config->reg_specific.ucWifiTaVer)
#define SCF_SUB6_EXP_TH(_config) \
		(ACC(_config->reg_specific.ucScfSub6ExpireThresh))
#define SMOOTH_CHG_SPEED_SCALE(_config) \
		(_config->reg_specific.ucSmoothChgSpeedScale)
#define REGULATORY(_config) \
		(_config->reg_specific.ucRegulatory)
/* for big endian value */
#define LITTLE_TO_BIG_ENDIAN_COPY REVERSE_COPY_MEMBER
#define REVERSE_COPY_MEMBER(p_out, p_in, is_array) \
	do { \
		if (is_array) { \
			btmtk_array_reverse_copy((p_out), (p_in), sizeof(p_in)); \
			(p_out) += sizeof(p_in); \
		} else { \
			*(p_out) = *(p_in); \
			(p_out)++; \
		} \
	} while (0)
/* transfer from 0.01 unit to u1.15 */
#define TRANSFER_U115(val, ptr) \
	do { \
		uint32_t result = ((uint32_t)(val) << 15) / 100; \
		if (result > 0x8000) BTMTK_WARN("%s: value[0x%x] more than 0x8000", __func__, result); \
		(ptr)[0] = (uint8_t)(result >> 8);\
		(ptr)[1] = (uint8_t)(result & 0xFF);\
	} while(0)

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

int btmtk_set_tasar_from_host(struct btmtk_dev *bdev, unsigned short int countryCode, unsigned int eci);
void btmtk_tasar_init(void);
int btmtk_send_tasar_evt_to_host(struct btmtk_dev *bdev);


#endif // __BTMTK_TASAR_H__

