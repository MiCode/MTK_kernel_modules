/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */


/******************************************************************************
 *                         C O M P I L E R   F L A G S
 ******************************************************************************
 */

/******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 ******************************************************************************
 */

#include "btmtk_tasar.h"
#include "btmtk_main.h"

/******************************************************************************
 *                              C O N S T A N T S
 ******************************************************************************
 */

/******************************************************************************
 *                             D A T A   T Y P E S
 ******************************************************************************
 */


/******************************************************************************
 *                            P U B L I C   D A T A
 ******************************************************************************
 */
/******************************************************************************
 *                           P R I V A T E   D A T A
 ******************************************************************************s
 */

static struct btmtk_tasar g_tasar;

static struct tasar_country g_rTasarCountryTbl_reg0[] = {
	{{'T', 'W'}}, {{'J', 'P'}}
};
static struct tasar_country g_rTasarCountryTbl_reg1[] = {
	{{'A', 'D'}}
};
static struct tasar_country g_rTasarCountryTbl_reg2[] = {
	{{'A', 'E'}}
};
static struct tasar_country g_rTasarCountryTbl_reg3[] = {
	{{'A', 'G'}}
};
static struct tasar_country g_rTasarCountryTbl_reg4[] = {
	{{'A', 'I'}}
};
static struct tasar_country g_rTasarCountryTbl_reg5[] = {
	{{'A', 'L'}}
};
static struct tasar_country g_rTasarCountryTbl_reg6[] = {
	{{'A', 'M'}}
};
static struct tasar_country g_rTasarCountryTbl_reg7[] = {
	{{'A', 'T'}}
};
static struct tasar_country g_rTasarCountryTbl_reg8[] = {
	{{'B', 'B'}}
};
static struct tasar_country g_rTasarCountryTbl_reg9[] = {
	{{'B', 'E'}}
};
static struct tasar_country g_rTasarCountryTbl_reg10[] = {
	{{'B', 'E'}}
};
static struct tasar_country g_rTasarCountryTbl_reg11[] = {
	{{'B', 'E'}}
};

static struct tasar_country_tbl_all g_rTasarCountryTbl[] =
{
	REG_TBL_REG(g_rTasarCountryTbl_reg0, "REG0_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg1, "REG1_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg2, "REG2_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg3, "REG3_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg4, "REG4_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg5, "REG5_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg6, "REG6_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg7, "REG7_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg8, "REG8_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg9, "REG9_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg10, "REG10_ConnsysTasar.cfg"),
	REG_TBL_REG(g_rTasarCountryTbl_reg11, "REG11_ConnsysTasar.cfg")
};

/******************************************************************************
 *                                 M A C R O S
 ******************************************************************************
 */
#define TASAR_LOG 0
#define TASAR_COUNTRY_TBL_SIZE (sizeof(g_rTasarCountryTbl)/sizeof(struct tasar_country_tbl_all))

/******************************************************************************
 *                  F U N C T I O N   D E C L A R A T I O N S
 ******************************************************************************
 */

/******************************************************************************
 * 	                          F U N C T I O N S
 ******************************************************************************
 */

static void btmtk_array_reverse_copy(uint8_t* dest, uint8_t* src, uint32_t cnt) {
	int i = 0;

	for (i = 0; i < cnt; i++) {
		dest[i] = src[cnt - 1 - i];
	}
}

uint32_t btmtk_tasar_data_extract(uint8_t* ctx, uint32_t cnt) {
	uint32_t i, j = 0;

	for (i = 0; i < cnt; i++)
		j += ctx[i] << (i * 8);
	return j;
}

static void btmtk_tasar_dump_config(struct tasar_config *rCfg)
{
	if (!rCfg) {
		BTMTK_WARN("%s: rCfg is NULL", __func__);
		return;
	}

	BTMTK_DBG("[TAS]tasar_config adr = %p, size:%lu",
		rCfg, sizeof(struct tasar_config));
	BTMTK_DBG("[TAS]version adr = %p, size:%lu",
		&rCfg->version[0], sizeof(rCfg->version));
	BTMTK_DBG("[TAS]common adr = %p, size:%lu",
		&rCfg->common, sizeof(rCfg->common));
	BTMTK_DBG("[TAS]reg_specific adr = %p, size:%lu",
		&rCfg->reg_specific, sizeof(rCfg->reg_specific));
	BTMTK_DBG("[TAS]chngrp[0] adr = %p, size:%lu",
		&rCfg->chngrp[0], sizeof(rCfg->chngrp[0]));
	BTMTK_DBG("[TAS]plimit[0] adr = %p, size:%lu",
		&rCfg->plimit[0], sizeof(rCfg->plimit[0]));

	BTMTK_DBG("[TAS]version [%d]", TAS_VERSION(rCfg));
}

static bool btmtk_tasar_update_config(struct btmtk_tasar *pTasar, int8_t *pcFileName)
{
	const u8 *pucConfigBuf = NULL;
	uint32_t u4ConfigReadLen = 0;
#if (TASAR_LOG == 1)
	uint32_t i = 0, j = 0;
#endif
	const struct firmware *fw_entry = NULL;
	struct device *dev = NULL;
	int err = 0;

	BTMTK_DBG("%s: begin setting_file_name = %s", __func__, pcFileName);
	err = request_firmware(&fw_entry, pcFileName, dev);
	if (err != 0 || fw_entry == NULL) {
		BTMTK_WARN("%s: request_firmware fail, maybe file %s not exist, err = %d, fw_entry = %p",
				__func__, pcFileName, err, fw_entry);
		if (fw_entry)
			release_firmware(fw_entry);
		return FALSE;
	}

	BTMTK_DBG("%s: setting file request_firmware size %zu success", __func__, fw_entry->size);

	pucConfigBuf = fw_entry->data;
	u4ConfigReadLen = fw_entry->size;

	if (!pucConfigBuf) {
		BTMTK_INFO("[TAS] File invalid");
		return FALSE;
	}

	BTMTK_INFO("%s: [TAS] structure size/file size[%lu/%d]", __func__, sizeof(pTasar->rTasarCfg), u4ConfigReadLen);

	if (sizeof (pTasar->rTasarCfg) != u4ConfigReadLen) {
		BTMTK_INFO("%s: [TAS] invalid file size", __func__);
		return FALSE;
	}

 #if (TASAR_LOG == 1)
	for (i = 0; i < u4ConfigReadLen - 16; i = i + 16) {
		BTMTK_INFO("%s: [TAS] pucConfigBuf[%x~%x] : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
			__func__,
			i, i + 15,
			pucConfigBuf[0 + i],pucConfigBuf[1 + i],pucConfigBuf[2 + i],pucConfigBuf[3 + i],
			pucConfigBuf[4 + i],pucConfigBuf[5 + i],pucConfigBuf[6 + i],pucConfigBuf[7 + i],
			pucConfigBuf[8 + i],pucConfigBuf[9 + i],pucConfigBuf[10 + i],pucConfigBuf[11 + i],
			pucConfigBuf[12 + i],pucConfigBuf[13 + i],pucConfigBuf[14 + i],pucConfigBuf[15 + i]);
	}
	for (j = i; j < u4ConfigReadLen; j++){
		BTMTK_INFO("%s: [TAS] pucConfigBuf[%x] %x", __func__, j, pucConfigBuf[j]);
	}
#endif // (TASAR_LOG == 1)

	memcpy(&pTasar->rTasarCfg, pucConfigBuf, u4ConfigReadLen);
	release_firmware(fw_entry);

	btmtk_tasar_dump_config(&pTasar->rTasarCfg);

    return TRUE;
}

static uint32_t btmtk_tasar_search_reg(struct tasar_scenrio_ctrl rScenrio)
{
	uint32_t i, j;
	uint16_t u2CountryCode = 0;

	for(i = 0; i < TASAR_COUNTRY_TBL_SIZE; i++){
		for(j = 0; j < g_rTasarCountryTbl[i].cnt; j++){

			u2CountryCode =
				(((uint16_t) g_rTasarCountryTbl[i].tbl[j].aucCountryCode[0] << 8) |
				(uint16_t) g_rTasarCountryTbl[i].tbl[j].aucCountryCode[1]);
			BTMTK_INFO("%s: [TAS] Tbl:[group=%d][idx=%d]/CC:[%c%c=0x%x]/FileName:[%s]",
				__func__, i, j,
				g_rTasarCountryTbl[i].tbl[j].aucCountryCode[0],
				g_rTasarCountryTbl[i].tbl[j].aucCountryCode[1],
				u2CountryCode,
				g_rTasarCountryTbl[i].ucFileName);

			if (rScenrio.u2CountryCode == u2CountryCode) {
				return i;
			}
		}
	}

	/* return TASAR_COUNTRY_TBL_SIZE mean no match found*/
	BTMTK_INFO("%s: [TAS] no match reg found in this cc 0x%x",
		__func__, rScenrio.u2CountryCode);

	return TASAR_COUNTRY_TBL_SIZE;
}

void btmtk_tasar_init(void)
{
	/* Reset to default status when BT on */
	g_tasar.rTasarScenrio.u4RegTblIndex = TASAR_COUNTRY_TBL_SIZE;
	g_tasar.rTasarScenrio.u2CountryCode = 0;
	g_tasar.rTasarScenrio.u4Eci = 0xFFFF;
	g_tasar.is_enable = false;

	BTMTK_INFO("%s: reg:%d, CC:0x%x, Eci:%d, enable:%d",
		__func__,
		g_tasar.rTasarScenrio.u4RegTblIndex,
		g_tasar.rTasarScenrio.u2CountryCode,
		g_tasar.rTasarScenrio.u4Eci,
		g_tasar.is_enable);
}

/* one shot para cmd */
static int btmtk_tasar_init_para_cmd(struct btmtk_dev *bdev, struct btmtk_tasar *pTasar) {

	u8 cmd[] = { 0x01, 0x2D, 0xFC, 0x30,
				 0x0D,					// sub opcode
				 0x00, 0x00, 0x00, 0x00,		// conn_status_resend_interval
				 0x00, 0x00, 0x00, 0x00,		// conn_status_resend_times
				 0x00, 0x00,				// scf_uart_err_margin
				 0x00,					// time_window_0_sec
				 0x00,					// time_window_2_sec
				 0x00,					// time_window_4_sec
				 0x00,					// time_window_30_sec
				 0x00,					// time_window_60_sec
				 0x00,					// time_window_100_sec
				 0x00,					// time_window_360_sec
				 0x00, 0x00,				// scf_chips_lat_margin
				 0x00, 0x00,				// scf_de_limit_margin
				 0x00, 0x00,				// scf_md_de_limit
				 0x00, 0x00,				// scf_conn_de_limit
				 0x00, 0x00, 0x00, 0x00,		// md_max_ref_period
				 0x00, 0x00,				// md_max_power
				 0x00,					// joint_mode
				 0x00, 0x00,				// scf_conn_off_margin
				 0x00, 0x00,				// scf_md_off_margin
				 0x00,					// scf_chg_instant_en
				 0x00,					// is_fbo_en
				 0x00,					// bt_ta_ver
				 0x00,					// wifi_ta_ver
				 0x00, 0x00,				// scf_sub6_expire_thresh
				 0x00,					// smooth_chg_speed_scale
				 0x00,					// regulatory
				 0x00,					// max_scf_wf_flight
				 0x00,					// max_scf_bt_flight
				 0x00};					// max_scf_conn_flight

	u8 evt[] = {0x04, 0x0E, 0x04, 0x01, 0x2D, 0xFC};
	u8 *para_ptr = &cmd[5];
	int ret = 0;

	BTMTK_DBG("%s", __func__);

	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_WORKING) {
		BTMTK_INFO("%s: bt already closed, not send cmd", __func__);
		return 0;
	}

	/* update para common part */
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.common.ucConnStatusResendInterval, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.common.ucConnStatusResendtimes, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.common.ucScfUartErrMargin, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow0Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow2Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow4Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow30Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow60Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow100Sec, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.common.ucTimeWindow360Sec, FALSE);

	/* update para reg specific part */
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfChipsLatMargin, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfDeLimitMargin, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfMdDeLimit, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScConnDeLimit, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucMdMaxRefPeriod, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucMdMaxPower, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucJointMode, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfConnOffMargin, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfMdOffMargin, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucScfChgInstantEn, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucIsFboEn, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucBtTaVer, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucWifiTaVer, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, pTasar->rTasarCfg.reg_specific.ucScfSub6ExpireThresh, TRUE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucSmoothChgSpeedScale, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucRegulatory, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucMaxScfWfFlight, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucMaxScfBTFlight, FALSE);
	LITTLE_TO_BIG_ENDIAN_COPY(para_ptr, &pTasar->rTasarCfg.reg_specific.ucMaxScfConnFlight, FALSE);

	BTMTK_INFO_RAW(bdev, cmd, sizeof(cmd), "%s: len[%lu]", __func__, sizeof(cmd));

	ret = btmtk_main_send_cmd(bdev,
			cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	/* event status */
	if (bdev->io_buf[6] != 0x00) {
		BTMTK_WARN("%s Rejected by fw. Do not send cmd after tasar enable", __func__);
		return 0;
	}

	if (ret < 0) {
		BTMTK_WARN("%s fail, ret[%d]", __func__, ret);
		return -1;
	}

	return ret;
}

/* Dynamci para cmd */
static int btmtk_tasar_update_eci(struct btmtk_dev *bdev, struct btmtk_tasar *pTasar, unsigned int newEci) {

	u8 cmd[] = { 0x01, 0x2D, 0xFC, 0x15,
				 0x08,				// sub opcode
				 0xFF,				// Plimit (min value of every ant of band, for script mode test)
				 0x00, 0x00,			// BT scaling factor (format u1.15)
				 0x00, 0x00,			// WIFI scaling factor (format u1.15)
				 0x00, 0x00,			// sub 6 scaling factor (format u1.15)
				 0x00, 0x00,			// mmw 6 scaling factor (format u1.15)
				 0x7F,				// Fixed power (normal case use 0x7F)
				 0x01,				// Scenario (0: test caset, 1: normal case)
				 0x00,				// Plimit ANT0 2G
				 0x00,				// Plimit ANT0 5G
				 0x00,				// Plimit ANT0 6G
				 0x00,				// Plimit ANT1 2G
				 0x00,				// Plimit ANT1 5G
				 0x00,				// Plimit ANT1 6G
				 0x00,				// Plimit ANT2 2G
				 0x00,				// Plimit ANT2 5G
				 0x00};				// Plimit ANT2 6G
	u8 evt[] = {0x04, 0x0E, 0x04, 0x01, 0x2D, 0xFC};
	int ret = 0, i = 0, pos = 0;

	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_WORKING) {
		BTMTK_INFO("%s: bt already closed, not send cmd", __func__);
		return 0;
	}

	if (pTasar->rTasarScenrio.u4Eci == newEci) {
		BTMTK_WARN("%s: same ECI[%u], not update", __func__, newEci);
		return 0;
	}

	if (newEci >= TASAR_ECI_NUM) {
		BTMTK_WARN("%s: ECI[%u] invalid", __func__, newEci);
		return -1;
	}

	BTMTK_DBG("%s: ucBTCust[%u], ucWifiCust[%u], ucSub6Cust[%u], ucMmw6Cust[%u]",
				__func__,
				pTasar->rTasarCfg.plimit[newEci].scf_factor.ucBTCust,
				pTasar->rTasarCfg.plimit[newEci].scf_factor.ucWifiCust,
				pTasar->rTasarCfg.plimit[newEci].scf_factor.ucSub6Cust,
				pTasar->rTasarCfg.plimit[newEci].scf_factor.ucMmw6Cust);

	/* scaling factor for radios */
	pos = 6;
	TRANSFER_U115(pTasar->rTasarCfg.plimit[newEci].scf_factor.ucBTCust, &cmd[pos]);
	pos += 2;
	TRANSFER_U115(pTasar->rTasarCfg.plimit[newEci].scf_factor.ucWifiCust, &cmd[pos]);
	pos += 2;
	TRANSFER_U115(pTasar->rTasarCfg.plimit[newEci].scf_factor.ucSub6Cust, &cmd[pos]);
	pos += 2;
	TRANSFER_U115(pTasar->rTasarCfg.plimit[newEci].scf_factor.ucMmw6Cust, &cmd[pos]);

	/* BT p_limit (2G, 5G, 6G)*/
	pos = 16;
	for (i = 0; i < TASAR_BT_CHANNEL_GRP_NUM; i++) {
		cmd[pos + i] = pTasar->rTasarCfg.plimit[newEci].bt_plimit[i].ucAnt0;
		cmd[pos + i + 3] = pTasar->rTasarCfg.plimit[newEci].bt_plimit[i].ucAnt1;
		cmd[pos + i + 6] = pTasar->rTasarCfg.plimit[newEci].bt_plimit[i].ucAnt2;
	}

	/* get the min plimit value */
	for (i = 0; i < TASAR_BT_CHANNEL_GRP_NUM * 3; i++) {
		cmd[5] = MIN(cmd[5], cmd[pos + i]);
	}

	BTMTK_INFO_RAW(bdev, cmd, sizeof(cmd), "%s: TAS-version[0x%x] reg[%d] newEci[%u] len[%lu]",
				__func__, TAS_VERSION((&pTasar->rTasarCfg)), pTasar->rTasarScenrio.u4RegTblIndex,
				newEci, sizeof(cmd));

	ret = btmtk_main_send_cmd(bdev,
			cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	/* event status */
	if (bdev->io_buf[6] != 0x00) {
		BTMTK_WARN("%s Rejected by fw. Do not send cmd after tasar enable", __func__);
		return 0;
	}

	if (ret < 0) {
		BTMTK_WARN("%s fail, ret[%d]", __func__, ret);
		return -1;
	}

	/* update record */
	pTasar->rTasarScenrio.u4Eci = newEci;

	return 0;
}

/* one shot enable cmd */
static int btmtk_tasar_enable_cmd(struct btmtk_dev *bdev, struct btmtk_tasar *pTasar) {


	u8 cmd[] = { 0x01, 0x2D, 0xFC, 0x09,
				 0x0A,			// [4] sub opcode
				 0x03,			// [5] bt_ta_ver, 0x00-0x03
				 0x00, 0x00,		// [6-7] invalid field, not use
				 0xFF, 0xFF,		// [8-9] fixed time window, normal case use 0xFFFF
				 0x00,			// [10] REG
				 0x00, 0x00};		// [11-12] Min WF SD for 2.X, normal case use 0x0000
	u8 evt[] = {0x04, 0x0E, 0x04, 0x01, 0x2D, 0xFC};
	int ret = 0;


	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_WORKING) {
		BTMTK_INFO("%s: bt already closed, not send cmd", __func__);
		return 0;
	}

	/* update para */
	cmd[5] = pTasar->rTasarCfg.reg_specific.ucBtTaVer;
	cmd[10] = pTasar->rTasarCfg.reg_specific.ucRegulatory;

	BTMTK_INFO_RAW(bdev, cmd, sizeof(cmd), "%s: countryCode[%c%c=0x%x] len[%lu]",
				__func__,
				(pTasar->rTasarScenrio.u2CountryCode >> 8) & 0xFF,
				pTasar->rTasarScenrio.u2CountryCode & 0xFF,
				pTasar->rTasarScenrio.u2CountryCode,
				sizeof(cmd));

	ret = btmtk_main_send_cmd(bdev,
			cmd, sizeof(cmd), evt, sizeof(evt),
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	/* event status */
	if (bdev->io_buf[6] != 0x00) {
		BTMTK_WARN("%s Rejected by fw. Do not send cmd after tasar enable", __func__);
		return 0;
	}

	if (ret < 0) {
		BTMTK_WARN("%s fail, ret[%d]", __func__, ret);
		return -1;
	}

	/* update record */
	pTasar->is_enable = true;

	return ret;
}

void btmtk_tasar_update_reg(struct btmtk_dev *bdev, struct btmtk_tasar *pTasar, struct tasar_scenrio_ctrl rScenrio)
{
	uint32_t u4TblIdx = TASAR_COUNTRY_TBL_SIZE;

	if (!pTasar) {
		BTMTK_WARN("%s: pTasar null", __func__);
		return;
	}

	BTMTK_INFO("%s: change reg:%d/CC:0x%x/Eci:%d to [CC:0x%x/Eci:%u]",
		__func__,
		pTasar->rTasarScenrio.u4RegTblIndex,
		pTasar->rTasarScenrio.u2CountryCode,
		pTasar->rTasarScenrio.u4Eci,
		rScenrio.u2CountryCode,
		rScenrio.u4Eci);

	if(pTasar->rTasarScenrio.u2CountryCode != rScenrio.u2CountryCode) {
		/* find REG number */
		u4TblIdx = btmtk_tasar_search_reg(rScenrio);
		if (u4TblIdx >= TASAR_COUNTRY_TBL_SIZE) {
			BTMTK_WARN("%s: no match country group REG, return ", __func__);
			return;
		}

		/* update country code related config */
		if (u4TblIdx != pTasar->rTasarScenrio.u4RegTblIndex) {
			/* get country group config */
			if(btmtk_tasar_update_config(pTasar, g_rTasarCountryTbl[u4TblIdx].ucFileName)) {
				/* update countrycode & group table indx */
				pTasar->rTasarScenrio.u4RegTblIndex = u4TblIdx;
				pTasar->rTasarScenrio.u2CountryCode = rScenrio.u2CountryCode;
				/* send one shot para cmd */
				btmtk_tasar_init_para_cmd(bdev, pTasar);
			} else {
				BTMTK_WARN("%s: Host Config parser fail", __func__);
				return;
			}
		} else
			BTMTK_INFO("%s: country group REG no change ", __func__);
	}

	/* update ECI */
	btmtk_tasar_update_eci(bdev, pTasar, rScenrio.u4Eci);

	/* enable tasar */
	btmtk_tasar_enable_cmd(bdev, pTasar);

	BTMTK_DBG("%s: update reg:%d/CC:0x%x/Eci:%d",
		__func__,
		pTasar->rTasarScenrio.u4RegTblIndex,
		pTasar->rTasarScenrio.u2CountryCode,
		pTasar->rTasarScenrio.u4Eci);
}

// API for host
int btmtk_set_tasar_from_host(struct btmtk_dev *bdev, unsigned short int countryCode, unsigned int eci) {


	/* During a BT on period, tasar can only be enabled once.
	 * Afterwards, only ECI can be modified, and REG can not be modified.
	 */

	if (!bdev) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(bdev) != BTMTK_STATE_WORKING) {
		BTMTK_INFO("%s: bt already closed, not send cmd", __func__);
		return 0;
	}

	if (g_tasar.is_enable)
		btmtk_tasar_update_eci(bdev, &g_tasar, eci);
	else {
		struct tasar_scenrio_ctrl rScenrio = {0};

		rScenrio.u2CountryCode = countryCode;
		rScenrio.u4Eci = eci;
		btmtk_tasar_update_reg(bdev, &g_tasar, rScenrio);
	}

	return 0;
}


int btmtk_send_tasar_evt_to_host(struct btmtk_dev *bdev)
{
	struct sk_buff *skb = NULL;
	u8 tasar_event[] = {0x04, 0x0E, 0x04, 0x01, 0x2D, 0xFC, 0x00};
	int event_len = sizeof(tasar_event);

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return -1;
	}

	BTMTK_DBG_DEV(bdev, "%s", __func__);

	skb = alloc_skb(event_len + BT_SKB_RESERVE, GFP_KERNEL);
	if (skb == NULL) {
		BTMTK_ERR_DEV(bdev, "%s allocate skb failed!!", __func__);
	} else {
		/* send to RX buffer instead of hci driver */
		skb_reserve(skb, BT_SKB_RESERVE);
		hci_skb_pkt_type(skb) = tasar_event[0];
		skb->len = event_len - 1;
		memcpy(skb->data, &tasar_event[1], skb->len);

		BTMTK_INFO_RAW(bdev, skb->data, skb->len, "%s: ", __func__);
		skb_queue_tail(&bdev->rx_q, skb);
		queue_work(bdev->workqueue, &bdev->rx_work);
	}
	return 0;
}
