// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "operation.h"

#define CFG_WAIT_TSSI_READY 0
#define MAC_TA_ADDRESS_OFFSET_ENB BIT(18)

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
#define PROACTIVE_BW160 3
#define PROACTIVE_BW320 4
#endif
#define HETB_PROACTIVE_BW20 0
#define HETB_PROACTIVE_BW40 1
#define HETB_PROACTIVE_BW80 2
#define HETB_PROACTIVE_BW160 3

extern s_int32 mt_engine_calc_phy(
	struct test_ru_info *ru_info,
	u_int32 apep_length,
	u_int8 stbc,
	u_int8 ltf_gi,
	u_int8 max_tpe);

union hetb_rx_cmm {
	struct {
		unsigned long long tigger_type:4;
		unsigned long long ul_length:12;
		unsigned long long cascade_ind:1;
		unsigned long long cs_required:1;
		unsigned long long ul_bw:2;
		unsigned long long gi_ltf:2;
		unsigned long long mimo_ltf:1;
		unsigned long long ltf_sym_midiam:3;
		unsigned long long stbc:1;
		unsigned long long ldpc_extra_sym:1;
		unsigned long long ap_tx_pwr:6;
		unsigned long long t_pe:3;
		unsigned long long spt_reuse:16;
		unsigned long long doppler:1;
		unsigned long long sig_a_reserved:9;
		unsigned long long reserved:1;
	} field;
	unsigned long long cmm_info;
};

union hetb_tx_usr {
	struct {
		u_int32 aid:12;
		u_int32 allocation:8;
		u_int32 coding:1;
		u_int32 mcs:4;
		u_int32 dcm:1;
		u_int32 ss_allocation:6;
	} field;
	u_int32 usr_info;
};

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
union ehttb_tx_usr {
	struct {
		u_int64 aid:8;
		u_int64 neoab:1;
		u_int64 mimo_nss:3;
		u_int64 allocation:8;
		u_int64 coding:1;
		u_int64 mcs:4;
		u_int64 dcm:1;
		u_int64 start_ss:4;
		u_int64 Nos:2;
		u_int64 TargetRssi:7;
		u_int64 Ps160:1;
		u_int64 reserve2:22;
		u_int64 UserType:2;
	} field;
	u_int64 usr_info;
};

union ehttb_spec_usr {
	struct {
		u_int64 aid:12;
		u_int64 PhyId:3;
		u_int64 ExtUlBw:2;
		u_int64 SR1:4;
		u_int64 SR2:4;
		u_int64 disregard:12;
		u_int64 reserve:25;
		u_int64 UserType:2;
	} field;
	u_int64 usr_info;
};
#endif

struct icap_dump_iq {
	u_int32 wf_num;
	u_int32 iq_type;
	u_int32 icap_cnt; /*IQ Data Simple Count*/
	u_int32 icap_data_len;
	u_int8 *picap_data;
};

/*****************************************************************************
 *	Enum value definition
 *****************************************************************************/
/* gen4m Function ID List */
enum ENUM_RF_AT_FUNCID {
	RF_AT_FUNCID_VERSION = 0,
	RF_AT_FUNCID_COMMAND,
	RF_AT_FUNCID_POWER,
	RF_AT_FUNCID_RATE,
	RF_AT_FUNCID_PREAMBLE,
	RF_AT_FUNCID_ANTENNA,
	RF_AT_FUNCID_PKTLEN,
	RF_AT_FUNCID_PKTCNT,
	RF_AT_FUNCID_PKTINTERVAL,
	RF_AT_FUNCID_TEMP_COMPEN,
	RF_AT_FUNCID_TXOPLIMIT,
	RF_AT_FUNCID_ACKPOLICY,
	RF_AT_FUNCID_PKTCONTENT,
	RF_AT_FUNCID_RETRYLIMIT,
	RF_AT_FUNCID_QUEUE,
	RF_AT_FUNCID_BANDWIDTH,
	RF_AT_FUNCID_GI,
	RF_AT_FUNCID_STBC,
	RF_AT_FUNCID_CHNL_FREQ,
	RF_AT_FUNCID_RIFS,
	RF_AT_FUNCID_TRSW_TYPE,
	RF_AT_FUNCID_RF_SX_SHUTDOWN,
	RF_AT_FUNCID_PLL_SHUTDOWN,
	RF_AT_FUNCID_SLOW_CLK_MODE,
	RF_AT_FUNCID_ADC_CLK_MODE,
	RF_AT_FUNCID_MEASURE_MODE,
	RF_AT_FUNCID_VOLT_COMPEN,
	RF_AT_FUNCID_DPD_TX_GAIN,
	RF_AT_FUNCID_DPD_MODE,
	RF_AT_FUNCID_TSSI_MODE,
	RF_AT_FUNCID_TX_GAIN_CODE,
	RF_AT_FUNCID_TX_PWR_MODE,

	/* Query command */
	RF_AT_FUNCID_TXED_COUNT = 32,
	RF_AT_FUNCID_TXOK_COUNT,
	RF_AT_FUNCID_RXOK_COUNT,
	RF_AT_FUNCID_RXERROR_COUNT,
	RF_AT_FUNCID_RESULT_INFO,
	RF_AT_FUNCID_TRX_IQ_RESULT,
	RF_AT_FUNCID_TSSI_RESULT,
	RF_AT_FUNCID_DPD_RESULT,
	RF_AT_FUNCID_RXV_DUMP,
	RF_AT_FUNCID_RX_PHY_STATIS,
	RF_AT_FUNCID_MEASURE_RESULT,
	RF_AT_FUNCID_TEMP_SENSOR,
	RF_AT_FUNCID_VOLT_SENSOR,
	RF_AT_FUNCID_READ_EFUSE,
	RF_AT_FUNCID_RX_RSSI,
	RF_AT_FUNCID_FW_INFO,
	RF_AT_FUNCID_DRV_INFO,
	RF_AT_FUNCID_PWR_DETECTOR,
	RF_AT_FUNCID_WBRSSI_IBSSI,

	/* Set command */
	RF_AT_FUNCID_SET_DPD_RESULT = 64,
	RF_AT_FUNCID_SET_CW_MODE,
	RF_AT_FUNCID_SET_JAPAN_CH14_FILTER,
	RF_AT_FUNCID_WRITE_EFUSE,
	RF_AT_FUNCID_SET_MAC_ADDRESS,
	RF_AT_FUNCID_SET_TA,
	RF_AT_FUNCID_SET_RX_MATCH_RULE,

	/* 80211AC & Jmode */
	RF_AT_FUNCID_SET_CBW = 71,
	RF_AT_FUNCID_SET_DBW,
	RF_AT_FUNCID_SET_PRIMARY_CH,
	RF_AT_FUNCID_SET_ENCODE_MODE,
	RF_AT_FUNCID_SET_J_MODE,

	/* ICAP command */
	RF_AT_FUNCID_SET_ICAP_CONTENT = 80,
	RF_AT_FUNCID_SET_ICAP_MODE,
	RF_AT_FUNCID_SET_ICAP_STARTCAP,
	RF_AT_FUNCID_SET_ICAP_SIZE = 83,
	RF_AT_FUNCID_SET_ICAP_TRIGGER_OFFSET,
	RF_AT_FUNCID_QUERY_ICAP_DUMP_FILE = 85,

	/* 2G 5G Band */
	RF_AT_FUNCID_SET_BAND = 90,

	/* Reset Counter */
	RF_AT_FUNCID_RESETTXRXCOUNTER = 91,

	/* FAGC RSSI Path */
	RF_AT_FUNCID_FAGC_RSSI_PATH = 92,

	/* Set RX Filter Packet Length */
	RF_AT_FUNCID_RX_FILTER_PKT_LEN = 93,

	/* Tone */
	RF_AT_FUNCID_SET_TONE_RF_GAIN = 96,
	RF_AT_FUNCID_SET_TONE_DIGITAL_GAIN = 97,
	RF_AT_FUNCID_SET_TONE_TYPE = 98,
	RF_AT_FUNCID_SET_TONE_DC_OFFSET = 99,
	RF_AT_FUNCID_SET_TONE_BW = 100,

	/* MT6632 Add */
	RF_AT_FUNCID_SET_MAC_HEADER = 101,
	RF_AT_FUNCID_SET_SEQ_CTRL = 102,
	RF_AT_FUNCID_SET_PAYLOAD = 103,
	RF_AT_FUNCID_SET_DBDC_BAND_IDX = 104,
	RF_AT_FUNCID_SET_BYPASS_CAL_STEP = 105,

	/* Set RX Path */
	RF_AT_FUNCID_SET_RX_PATH = 106,

	/* Set Frequency Offset */
	RF_AT_FUNCID_SET_FRWQ_OFFSET = 107,

	/* Get Frequency Offset */
	RF_AT_FUNCID_GET_FREQ_OFFSET = 108,

	/* Set RXV Debug Index */
	RF_AT_FUNCID_SET_RXV_INDEX = 109,

	/* Set Test Mode DBDC Enable */
	RF_AT_FUNCID_SET_DBDC_ENABLE = 110,

	/* Get Test Mode DBDC Enable */
	RF_AT_FUNCID_GET_DBDC_ENABLE = 111,

	/* Set ICAP Ring Capture */
	RF_AT_FUNCID_SET_ICAP_RING = 112,

	/* Set TX Path */
	RF_AT_FUNCID_SET_TX_PATH = 113,

	/* Set Nss */
	RF_AT_FUNCID_SET_NSS = 114,

	/* Set TX Antenna Mask */
	RF_AT_FUNCID_SET_ANTMASK = 115,

	/* TMR set command */
	RF_AT_FUNCID_SET_TMR_ROLE = 116,
	RF_AT_FUNCID_SET_TMR_MODULE = 117,
	RF_AT_FUNCID_SET_TMR_DBM = 118,
	RF_AT_FUNCID_SET_TMR_ITER = 119,

	/* Set ADC For IRR Feature */
	RF_AT_FUNCID_SET_ADC = 120,

	/* Set RX Gain For IRR Feature */
	RF_AT_FUNCID_SET_RX_GAIN = 121,

	/* Set TTG For IRR Feature */
	RF_AT_FUNCID_SET_TTG = 122,

	/* Set TTG ON/OFF For IRR Feature */
	RF_AT_FUNCID_TTG_ON_OFF = 123,

	/* Set TSSI for QA Tool Setting */
	RF_AT_FUNCID_SET_TSSI = 124,

	/* Set Recal Cal Step */
	RF_AT_FUNCID_SET_RECAL_CAL_STEP = 125,

	/* Set iBF/eBF enable */
	RF_AT_FUNCID_SET_IBF_ENABLE = 126,
	RF_AT_FUNCID_SET_EBF_ENABLE = 127,

	/* Set MPS Setting */
	RF_AT_FUNCID_SET_MPS_SIZE = 128,
	RF_AT_FUNCID_SET_MPS_SEQ_DATA = 129,
	RF_AT_FUNCID_SET_MPS_PAYLOAD_LEN = 130,
	RF_AT_FUNCID_SET_MPS_PKT_CNT = 131,
	RF_AT_FUNCID_SET_MPS_PWR_GAIN = 132,
	RF_AT_FUNCID_SET_MPS_NSS = 133,
	RF_AT_FUNCID_SET_MPS_PACKAGE_BW = 134,

	RF_AT_FUNCID_GET_TX_POWER = 136,
	/* Antenna swap feature*/
	RF_AT_FUNCID_SET_ANT_SWP = 153,
	RF_AT_FUNCID_SET_RX_MU_AID = 157,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR0 = 158,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR1 = 159,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR2 = 160,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR3 = 161,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR4 = 162,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR5 = 163,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR6 = 164,
	RF_AT_FUNCID_SET_SECURITY_MODE = 165,
	RF_AT_FUNCID_SET_LP_MODE = 166,

	/* Set HW TX enable */
	RF_AT_FUNCID_SET_HWTX_MODE = 167,

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	/* 11 be */
	RF_AT_FUNCID_SET_PUNCTURE = 168,
	RF_AT_FUNCID_GET_CFG_ON_OFF = 169,

	/* Set FREQ OFFSET C2 */
	RF_AT_FUNCID_SET_FREQ_OFFSET_C2_SET = 171,
	RF_AT_FUNCID_SET_FREQ_OFFSET_C2_GET = 172,

	RF_AT_FUNCID_SET_CFG_ON = 176,
	RF_AT_FUNCID_SET_CFG_OFF = 177,

	RF_AT_FUNCID_SET_BSSID = 189,
	RF_AT_FUNCID_SET_TX_TIME = 190,
	RF_AT_FUNCID_SET_MAX_PE = 191,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR7 = 192,
	RF_AT_FUNCID_SET_TX_HE_TB_TTRCR8 = 193,
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

	/* GAIN CAL */
	RF_AT_FUNCID_SET_EFEM_MODE = 196,
	RF_AT_FUNCID_SET_TX_GAIN = 197,
	RF_AT_FUNCID_SET_ETSSI_GAIN = 198,
	RF_AT_FUNCID_GET_TSSI_MEAS_DBV = 199,
	RF_AT_FUNCID_SET_GAIN_ENABLE = 200,
	RF_AT_FUNCID_SET_GAIN_VALUE = 201,

	/* EHT TB dRU enable */
	RF_AT_FUNCID_SET_EHTTB_DRU_ENABLE = 222,

	/* Get EEPROM/NVRAM/Bufferbin default power */
	RF_AT_FUNCID_GET_DEFAULT_TX_POWER = 224,

	/* TMR Toae Cal and Restore */
	RF_AT_FUNCID_SET_TMR_TOAE_CAL_RESOTRE = 225,

	/* Set & get power type. 0:NVRAM, 1:UI*/
	RF_AT_CMD_SET_GET_POWER_TYPE = 227,

	RF_AT_FUNCID_GET_SLEEP_CHECK = 236,

	RF_AT_FUNCID_NULL = 0xFF
};

/* Command */
enum ENUM_RF_AT_COMMAND {
	RF_AT_COMMAND_STOPTEST = 0,
	RF_AT_COMMAND_STARTTX,
	RF_AT_COMMAND_STARTRX,
	RF_AT_COMMAND_RESET,
	RF_AT_COMMAND_OUTPUT_POWER,	/* Payload */
	/* Local freq is renamed to Local leakage */
	RF_AT_COMMAND_LO_LEAKAGE,
	/* OFDM (LTF/STF), CCK (PI,PI/2) */
	RF_AT_COMMAND_CARRIER_SUPPR,
	RF_AT_COMMAND_TRX_IQ_CAL,
	RF_AT_COMMAND_TSSI_CAL,
	RF_AT_COMMAND_DPD_CAL,
	RF_AT_COMMAND_CW,
	RF_AT_COMMAND_ICAP,
	RF_AT_COMMAND_RDD,
	RF_AT_COMMAND_CH_SWITCH_FOR_ICAP,
	RF_AT_COMMAND_RESET_DUMP_NAME,
	RF_AT_COMMAND_SINGLE_TONE,
	RF_AT_COMMAND_RDD_OFF,
	RF_AT_COMMAND_NUM
};

/* Preamble */
enum ENUM_RF_AT_PREAMBLE {
	RF_AT_PREAMBLE_NORMAL = 0,
	RF_AT_PREAMBLE_CCK_SHORT,
	RF_AT_PREAMBLE_11N_MM,
	RF_AT_PREAMBLE_11N_GF,
	RF_AT_PREAMBLE_11AC,
	RF_AT_PREAMBLE_NUM
};

/* Ack Policy */
enum ENUM_RF_AT_ACK_POLICY {
	RF_AT_ACK_POLICY_NORMAL = 0,
	RF_AT_ACK_POLICY_NOACK,
	RF_AT_ACK_POLICY_NOEXPLICTACK,
	RF_AT_ACK_POLICY_BLOCKACK,
	RF_AT_ACK_POLICY_NUM
};

enum ENUM_RF_AUTOTEST_STATE {
	RF_AUTOTEST_STATE_STANDBY = 0,
	RF_AUTOTEST_STATE_TX,
	RF_AUTOTEST_STATE_RX,
	RF_AUTOTEST_STATE_RESET,
	RF_AUTOTEST_STATE_OUTPUT_POWER,
	RF_AUTOTEST_STATE_LOCA_FREQUENCY,
	RF_AUTOTEST_STATE_CARRIER_SUPRRESION,
	RF_AUTOTEST_STATE_NUM
};

enum {
	ATE_LOG_RXV = 1,
	ATE_LOG_RDD,
	ATE_LOG_RE_CAL,
	ATE_LOG_TYPE_NUM,
	ATE_LOG_RXINFO,
	ATE_LOG_TXDUMP,
	ATE_LOG_TEST
};

enum {
	ATE_LOG_OFF,
	ATE_LOG_ON,
	ATE_LOG_DUMP,
	ATE_LOG_CTRL_NUM,
};

enum ENUM_ATE_CAP_TYPE {
	/* I/Q Type */
	ATE_CAP_I_TYPE = 0,
	ATE_CAP_Q_TYPE = 1,
	ATE_NUM_OF_CAP_TYPE
};
/*****************************************************************************
 *	Global Variable
 *****************************************************************************/
static struct hqa_m_rx_stat test_hqa_rx_stat;
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
static u_char g_tx_mode;
#endif /* (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0) */

static u_int32 tm_ch_num_to_freq(u_int32 ch_num)
{
	u_int32 ch_in_mhz;

	if (ch_num >= 1 && ch_num <= 13)
		ch_in_mhz = 2412 + (ch_num - 1) * 5;
	else if (ch_num == 14)
		ch_in_mhz = 2484;
	else if (ch_num == 133)
		ch_in_mhz = 3665;	/* 802.11y */
	else if (ch_num == 137)
		ch_in_mhz = 3685;	/* 802.11y */
	else if ((ch_num >= 34 && ch_num <= 181)
		 || (ch_num == 16))
		ch_in_mhz = 5000 + ch_num * 5;
	else if (ch_num >= 182 && ch_num <= 196)
		ch_in_mhz = 4000 + ch_num * 5;
	else if (ch_num == 201)
		ch_in_mhz = 2730;
	else if (ch_num == 202)
		ch_in_mhz = 2498;
	else
		ch_in_mhz = 0;

	return 1000 * ch_in_mhz;
}

static u_int32 tm_bw_hqa_mapping_at(u_int32 bw)
{
	u_int32 bw_mapping = 0;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	/* BW Mapping in QA Tool
	 * 0: BW20
	 * 1: BW40
	 * 2: BW80
	 * 3: BW10
	 * 4: BW5
	 * 5: BW160C
	 * 6: BW160NC
	 * 7: BW0_5
	 * 8: BW1
	 * 9: BW6
	 * 10: BW7
	 * 12: BW320
	 */
	/* BW Mapping in FW hal_cal_flow_rom.h
	 * 0: CDBW_20
	 * 1: CDBW_40
	 * 2: CDBW_80
	 * 3: CDBW_160
	 * 4: CDBW_320
	 * 5: CDBW_5
	 * 6: CDBW_10
	 * 7: CDBW_80P80
	 */

	u_int32 mapping_table[13] = {
		0, 1, 2, 6, 5, 3, 7, 0,
		0, 0, 0, 0, 4
	};

	if (bw < 13)
		bw_mapping = mapping_table[bw];

#else

	/* BW Mapping in QA Tool
	 * 0: BW20
	 * 1: BW40
	 * 2: BW80
	 * 3: BW10
	 * 4: BW5
	 * 5: BW160C
	 * 6: BW160NC
	 * 12: BW320 (radar)
	 */
	/* BW Mapping in FW
	 * 0: BW20
	 * 1: BW40
	 * 2: BW80
	 * 3: BW160C
	 * 4: BW160NC
	 * 5: BW5
	 * 6: BW10
	 * 8: BW320 (radar)
	 */
	if (bw == 0)
		bw_mapping = 0;
	else if (bw == 1)
		bw_mapping = 1;
	else if (bw == 2)
		bw_mapping = 2;
	else if (bw == 3)
		bw_mapping = 6;
	else if (bw == 4)
		bw_mapping = 5;
	else if (bw == 5)
		bw_mapping = 3;
	else if (bw == 6)
		bw_mapping = 4;
	else if (bw == 12)
		bw_mapping = 8;
#endif

	return bw_mapping;
}

static s_int32 tm_rftest_set_auto_test(
	struct test_wlan_info *winfos,
	u_int32 func_idx,
	u_int32 func_data)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rf_at_info.func_idx = func_idx;
	rf_at_info.func_data = func_data;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_RFTEST_SET_AUTO_TEST,
		&rf_at_info,
		sizeof(rf_at_info),
		NULL,
		NULL);

	return ret;

}

static s_int32 tm_rftest_query_auto_test(
	struct test_wlan_info *winfos,
	struct param_mtk_wifi_test_struct *prf_at_info,
	u_int32 *buf_len)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_RFTEST_QUERY_AUTO_TEST,
		prf_at_info,
		sizeof(*prf_at_info),
		NULL,
		NULL);

	return ret;

}

static s_int32 tm_icap_mode(
	struct test_wlan_info *winfos)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_SET_TEST_ICAP_MODE,
		NULL,
		0,
		NULL,
		NULL);

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("Switch ICAP Mode,ret=0x%08x\n", ret));

	return SERV_STATUS_SUCCESS;
}

static s_int32 tm_log_query_auto_test(
	struct test_wlan_info *winfos,
	struct param_rdd_log_struct *log_info,
	u_int32 *buf_len)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_SET_LOG_ONFF,
		log_info,
		sizeof(*log_info),
		NULL,
		NULL);

	return ret;

}

s_int32 mt_op_set_tr_mac(
	struct test_wlan_info *winfos,
	s_int32 op_type, boolean enable, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_tx_stream(
	struct test_wlan_info *winfos,
	u_int32 stream_nums, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_tx_path(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	s_int32 ret;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_PATH,
			(u_int32)configs->tx_ant);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

err_out:
	return ret;
}

s_int32 mt_op_set_rx_path(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 rx_ant = (u_int32)configs->rx_ant;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

	rx_ant = ((rx_ant << 16) | (0 & BITS(0, 15)));
	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_RX_PATH,
			rx_ant);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

err_out:
	return ret;
}

s_int32 mt_op_set_rx_filter(
	struct test_wlan_info *winfos, struct rx_filter_ctrl rx_filter)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_clean_persta_txq(
	struct test_wlan_info *winfos,
	boolean sta_pause_enable, u_char omac_idx, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_cfg_on_off(
	struct test_wlan_info *winfos,
	u_int32 type,
	u_int32 enable,
	u_int32 band_idx,
	u_int32 ch_band)
{
	s_int32 ret;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_BAND, ch_band);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

	/* type pass through to FW */
	if (enable)
		ret = tm_rftest_set_auto_test(
			winfos, RF_AT_FUNCID_SET_CFG_ON, type);
	else
		ret = tm_rftest_set_auto_test(
			winfos, RF_AT_FUNCID_SET_CFG_OFF, type);

	if (ret != SERV_STATUS_SUCCESS)
		goto err_out;

#else

	u_int32 func_index, func_data;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	switch (type) {
	case 0: /* TSSI */
		func_index = RF_AT_FUNCID_SET_TSSI;
		break;
	case 1: /* DPD */
		func_index = RF_AT_FUNCID_DPD_MODE;
		break;
	default:
		ret = SERV_STATUS_HAL_OP_FAIL;
		return ret;
	}

	func_data = 0;

	if (enable == 0)
		func_data &= ~BIT(0);
	else
		func_data |= BIT(0);

	if (band_idx == 0)
		func_data &= ~BIT(1);
	else
		func_data |= BIT(1);

	ret = tm_rftest_set_auto_test(
		winfos, func_index, func_data);

#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
err_out:
#endif
	return ret;
}

s_int32 mt_op_log_on_off(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 log_type,
	u_int32 log_ctrl,
	u_int32 log_size)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	struct param_rdd_log_struct log_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;
	u_int32 rxv;
	s_int32 i, target_len = 0, max_dump_rxv_cnt = 500;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	if ((log_ctrl == ATE_LOG_DUMP) && (log_type == ATE_LOG_RXV))	{
		rf_at_info.func_idx = RF_AT_FUNCID_RESULT_INFO;
		rf_at_info.func_data = RF_AT_FUNCID_RXV_DUMP;
		ret = tm_rftest_query_auto_test(winfos,
			&rf_at_info, &buf_len);
		if (ret == SERV_STATUS_SUCCESS) {
			target_len = rf_at_info.func_data * 36;
			if (target_len >= (max_dump_rxv_cnt * 36))
				target_len = (max_dump_rxv_cnt * 36);
		}

		for (i = 0; i < target_len; i += 4)	{
			rf_at_info.func_idx = RF_AT_FUNCID_RXV_DUMP;
			rf_at_info.func_data = i;
			ret = tm_rftest_query_auto_test(winfos,
				&rf_at_info, &buf_len);

			if (ret == SERV_STATUS_SUCCESS) {
				rxv = rf_at_info.func_data;
				if (i % 36 == 0) {
					/* Todo */
					/* TOOL_PRINTLOG... */
				}

				if (((i % 36) / 4) + 1 == 9) {
					/* Todo */
					/* TOOL_PRINTLOG... */
				}
			}

		}

		/* TOOL_PRINTLOG(RFTEST, ERROR, "[LOG DUMP END]\n"); */
	}

	if (log_type == ATE_LOG_RDD) {

		log_info.band_idx = band_idx;
		log_info.log_size = log_size;
		log_info.log_ctrl = log_ctrl;

		ret = tm_log_query_auto_test(winfos,
			&log_info, &buf_len);
	}

	return ret;
}

s_int32 mt_op_set_antenna_port(
	struct test_wlan_info *winfos,
	u_int8 rf_mode_mask, u_int8 rf_port_mask, u_int8 ant_port_mask)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_slot_time(
	struct test_wlan_info *winfos,
	u_int8 slot_time, u_int8 sifs_time,
	u_int8 rifs_time, u_int16 eifs_time, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_power_drop_level(
	struct test_wlan_info *winfos,
	u_int8 pwr_drop_level, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_rx_filter_pkt_len(
	struct test_wlan_info *winfos,
	u_int8 enable, u_char band_idx, u_int32 rx_pkt_len)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 func_data;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	func_data = rx_pkt_len & BITS(0, 23);
	func_data |= (u_int32)(band_idx << 24);

	if (enable == 1)
		func_data |= BIT(30);
	else
		func_data &= ~BIT(30);

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RX_FILTER_PKT_LEN, func_data);

	return ret;
}

s_int32 mt_op_get_antswap_capability(
	struct test_wlan_info *winfos,
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	u_char band_idx,
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */
	u_int32 *antswap_support)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_ANTSWAP_CAPBILITY,
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
		(void *)&band_idx,
#else
		NULL,
#endif /*(CFG_SUPPORT_CONNAC3X == 1)*/
		0,
		antswap_support,
		NULL);

	return ret;
}

s_int32 mt_op_set_antswap(
	struct test_wlan_info *winfos,
	u_int32 ant)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_ANT_SWP, ant);

	return ret;
}

s_int32 mt_op_set_freq_offset(
	struct test_wlan_info *winfos,
	u_int32 freq_offset, u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_FRWQ_OFFSET, freq_offset);

	return ret;
}

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
s_int32 mt_op_set_freq_offset_C2(
	struct test_wlan_info *winfos,
	u_int32 freq_offset, u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_FREQ_OFFSET_C2_SET, freq_offset);

	return ret;
}
#endif

s_int32 mt_op_set_phy_counter(
	struct test_wlan_info *winfos,
	s_int32 control, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_rxv_index(
	struct test_wlan_info *winfos,
	u_int8 group_1, u_int8 group_2, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_fagc_path(
	struct test_wlan_info *winfos,
	u_int8 path, u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_fw_mode(
	struct test_wlan_info *winfos, u_char fw_mode)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_rf_test_mode(
	struct test_wlan_info *winfos,
	u_int32 op_mode, u_int8 icap_len, u_int16 rsp_len)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_test_mode_start(
	struct test_wlan_info *winfos)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		 OP_WLAN_OID_SET_TEST_MODE_START,
		 NULL,
		 0,
		 NULL,
		 NULL);

	return ret;
}

s_int32 mt_op_set_test_mode_abort(
	struct test_wlan_info *winfos)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret =  pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		 OP_WLAN_OID_SET_TEST_MODE_ABORT,
		 NULL,
		 0,
		 NULL,
		 NULL);

	return ret;
}

s_int32 mt_op_backup_and_set_cr(
	struct test_wlan_info *winfos,
	struct test_bk_cr *bks,
	u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_restore_cr(
	struct test_wlan_info *winfos,
	struct test_bk_cr *bks,
	u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_ampdu_ba_limit(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_int8 agg_limit)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_sta_pause_cr(
	struct test_wlan_info *winfos)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_ifs_cr(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_write_mac_bbp_reg(
	struct test_wlan_info *winfos,
	struct test_register *regs)
{
	struct param_custom_mcr_rw_struct mcr_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	s_int32 ret = SERV_STATUS_SUCCESS;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	mcr_info.mcr_offset = regs->cr_addr;
	mcr_info.mcr_data = *regs->cr_val;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
		("%s: cr_val=0x%08x\n",
			__func__, *regs->cr_val));

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_SET_MCR_WRITE,
		&mcr_info,
		sizeof(mcr_info),
		NULL,
		NULL);

	return ret;
}

s_int32 mt_op_read_bulk_mac_bbp_reg(
	struct test_wlan_info *winfos,
	struct test_register *regs)
{
	struct param_custom_mcr_rw_struct mcr_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 idx;
	s_int32 ret = SERV_STATUS_SUCCESS;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	for (idx = 0; idx < regs->cr_num ; idx++) {
		mcr_info.mcr_offset = regs->cr_addr + idx * 4;
		mcr_info.mcr_data = 0;

		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_QUERY_MCR_READ,
			&mcr_info,
			sizeof(mcr_info),
			NULL,
			NULL);

		if (ret == SERV_STATUS_SUCCESS) {
			regs->cr_val[idx] = mcr_info.mcr_data;
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: cr_val=0x%08x\n",
					__func__, *regs->cr_val));
		} else {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: read fail ret = %d\n",
					__func__, ret));
		}
	}

	return ret;
}

s_int32 mt_op_read_bulk_rf_reg(
	struct test_wlan_info *winfos,
	struct test_register *regs)
{
	struct param_custom_mcr_rw_struct mcr_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 idx;
	s_int32 ret = SERV_STATUS_SUCCESS;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	regs->cr_addr = regs->cr_addr | 0x99000000 | (regs->wf_sel<<16);
#else
	regs->cr_addr = regs->cr_addr | 0x99900000 | (regs->wf_sel<<16);
#endif

	/*
	 * wf_sel
	 * 0: WF0 -> 0x9990xxxx
	 * 1: WF1 -> 0x9991xxxx
	 * 2: WF2 -> 0x9992xxxx
	 * 3: WF3 -> 0x9993xxxx
	 * 4: WF4 -> 0x9994xxxx
	 * 15: ATOP -> 0x999Fxxxx
	 */

	/* 1. Correct to 1 byte/step between two continuous CR.
	 * 2. If access invalid A Die CR, it will get 0x0 for all project
	 *    without any error.
	 * 3. 6631 and 6635 A Die CR is 4 bytes/step.
	 * 4. 6637, 6639 and future project A Die CR is 1 byte/step.
	 */
	for (idx = 0; idx < regs->cr_num; idx++) {
		mcr_info.mcr_offset = regs->cr_addr + idx;
		mcr_info.mcr_data = 0;

		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_QUERY_MCR_READ,
			&mcr_info,
			sizeof(mcr_info),
			NULL,
			NULL);

		if (ret == SERV_STATUS_SUCCESS) {
			regs->cr_val[idx] = mcr_info.mcr_data;
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: cr_val=0x%08x\n",
				__func__, *regs->cr_val));
		} else {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: read fail ret = %d\n",
				__func__, ret));
		}
	}

	return ret;
}

s_int32 mt_op_write_bulk_rf_reg(
	struct test_wlan_info *winfos,
	struct test_register *regs)
{
	struct param_custom_mcr_rw_struct mcr_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	s_int32 ret = SERV_STATUS_SUCCESS;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#ifdef CFG_SUPPORT_UNIFIED_COMMAND
	regs->cr_addr = regs->cr_addr | 0x99000000 | (regs->wf_sel<<16);
#else
	regs->cr_addr = regs->cr_addr | 0x99900000 | (regs->wf_sel<<16);
#endif

	/*
	 * wf_sel
	 * 0: WF0 -> 0x9990xxxx
	 * 1: WF1 -> 0x9991xxxx
	 * 2: WF2 -> 0x9992xxxx
	 * 3: WF3 -> 0x9993xxxx
	 * 4: WF4 -> 0x9994xxxx
	 * 15: ATOP -> 0x999Fxxxx
	 */

	mcr_info.mcr_offset = regs->cr_addr;
	mcr_info.mcr_data = *regs->cr_val;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
		("%s: cr_val=0x%08x\n",
			__func__, *regs->cr_val));

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_SET_MCR_WRITE,
		&mcr_info,
		sizeof(mcr_info),
		NULL,
		NULL);

	return ret;
}

s_int32 mt_op_read_bulk_eeprom(
	struct test_wlan_info *winfos,
	struct test_eeprom *eprms)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_EPRM_READ,
			(void *)eprms,
			sizeof(struct test_eeprom),
			NULL,
			NULL);

	return ret;
}

s_int32 mt_op_write_bulk_eeprom(
	struct test_wlan_info *winfos,
	struct test_eeprom *eprms)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_EPRM_WRITE,
			(void *)eprms,
			sizeof(struct test_eeprom),
			NULL,
			NULL);

	return ret;
}

s_int32 mt_op_get_free_efuse_block(
	struct test_wlan_info *winfos,
	struct test_eeprom *eprms)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_EFUSE_FREE_BLOCK,
		(void *)eprms,
		sizeof(struct test_eeprom),
		NULL,
		NULL);

	return ret;
}

static void mt_op_set_manual_he_tb_value(
	struct test_wlan_info *winfos,
	struct test_ru_info *ru_sta,
	struct test_configuration *configs)
{
	union hetb_rx_cmm cmm;
	union hetb_tx_usr usr;
	u_int8 ltf_sym_code[] = {
		0, 0, 1, 2, 2, 3, 3, 4, 4   /* SS 1~8 */
	};
	u_int32 mapping_bw = 0;

	/* setup MAC start */
	/* step 1, common info of TF */
	sys_ad_zero_mem(&cmm, sizeof(cmm));
	cmm.field.sig_a_reserved = 0x1ff;
	cmm.field.ul_length = ru_sta->l_len;
	cmm.field.t_pe =
	(ru_sta->afactor_init & 0x3) | ((ru_sta->pe_disamb & 0x1) << 2);
	cmm.field.ldpc_extra_sym = ru_sta->ldpc_extr_sym;
	if (configs->stbc && ru_sta->nss == 1)
		cmm.field.ltf_sym_midiam = ltf_sym_code[ru_sta->nss+1];
	else
		cmm.field.ltf_sym_midiam = ltf_sym_code[ru_sta->nss];
	cmm.field.gi_ltf = configs->sgi;
	mapping_bw = tm_bw_hqa_mapping_at((u_int32) configs->per_pkt_bw);
	if (mapping_bw > HETB_PROACTIVE_BW160)
		cmm.field.ul_bw = HETB_PROACTIVE_BW160;
	else
		cmm.field.ul_bw = mapping_bw;
	cmm.field.stbc = configs->stbc;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("%s: [TF TTRCR0 ] tigger_type:0x%x,\n",
	__func__, cmm.field.tigger_type));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("ul_length:0x%x cascade_ind:0x%x,\n",
	cmm.field.ul_length, cmm.field.cascade_ind));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("cs_required:0x%x, ul_bw:0x%x,\n",
	cmm.field.cs_required, cmm.field.ul_bw));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("gi_ltf:0x%x, mimo_ltf:0x%x,\n",
	cmm.field.gi_ltf, cmm.field.mimo_ltf));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("ltf_sym_midiam:0x%x, stbc:0x%x,\n",
	cmm.field.ltf_sym_midiam, cmm.field.stbc));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("ldpc_extra_sym :0x%x, ap_tx_pwr: 0x%x\n",
	cmm.field.ldpc_extra_sym, cmm.field.ap_tx_pwr));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("%s: [TF TTRCR0 ] cr_value:0x%08x\n",
	__func__, (u_int32)(cmm.cmm_info & 0xffffffff)));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("%s: [TF TTRCR1 ] cr_value:0x%08x\n",
	__func__, (u_int32)((cmm.cmm_info & 0xffffffff00000000) >> 32)));

	/* step 1, users info */
	sys_ad_zero_mem(&usr, sizeof(usr));
	usr.field.aid = 0x1;
	usr.field.allocation = ru_sta->ru_index;
	usr.field.coding = ru_sta->ldpc;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	usr.field.mcs = ru_sta->rate & ~BIT(4);
	usr.field.dcm = (ru_sta->rate & BIT(4)) >> 4;
#else
	usr.field.mcs = ru_sta->rate & ~BIT(5);
	usr.field.dcm = (ru_sta->rate & BIT(5)) >> 5;
#endif
	usr.field.ss_allocation =
	((ru_sta->nss-1) << 3) | (ru_sta->start_sp_st & 0x7);

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("%s: [TF TTRCR2 ] aid:%d\n",
	__func__,  usr.field.aid));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("allocation:%d, coding:%d,\n",
	usr.field.allocation, usr.field.coding));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("mcs:0x%x, dcm:%d,\n",
	usr.field.mcs, usr.field.dcm));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("ss_allocation:%d\n",
	usr.field.ss_allocation));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
	("%s: [TF TTRCR2 ] cr_value:0x%08x\n",
	__func__, usr.usr_info));

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR0,
		(cmm.cmm_info & 0xffffffff));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR1,
		((cmm.cmm_info & 0xffffffff00000000) >> 32));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR2, usr.usr_info);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR3, 0x7f);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR4, 0xffffffff);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR5, 0xffffffff);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR6, 0xffffffff);
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR7, 0);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR8, 0);
#endif

}

#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
static s_int32 tm_trans_Preamble_rate(
	struct test_wlan_info *winfos,
	struct test_configuration *configs)
{
	struct test_ru_info *ru_sta = &configs->ru_info_list[0];

	u_char tx_mode = configs->tx_mode;
	u_char mcs = configs->mcs;
	u_int8 backukp_dmnt_ru_idx = configs->dmnt_ru_idx;

	/* Trans preamble and rate for get correct default txpwr*/
	if (tx_mode == TEST_MODE_OFDM) {
		tx_mode = TEST_MODE_CCK;
		mcs += 4;
	} else if ((tx_mode == TEST_MODE_CCK)
		&& ((mcs == 9)
		|| (mcs == 10) || (mcs == 11)))
		tx_mode = TEST_MODE_OFDM;

	tm_rftest_set_auto_test(winfos,
	RF_AT_FUNCID_PREAMBLE, tx_mode);

	if (tx_mode == TEST_MODE_CCK) {
		mcs |= 0x00000000;
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_RATE, mcs);
	} else if (tx_mode == TEST_MODE_OFDM) {
		if (mcs == 9)
			mcs = 1;
		else if (mcs == 10)
			mcs = 2;
		else if (mcs == 11)
			mcs = 3;
		mcs |= 0x00000000;

		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_RATE, mcs);
	} else if (tx_mode >= TEST_MODE_HTMIX &&
	tx_mode <= TEST_MODE_HE_TB) {

	if (tx_mode == TEST_MODE_HE_TB) {
		/*do ru operation*/
		if (ru_sta->valid) {
			/*Calculate HE TB PHY Info*/
			mt_engine_calc_phy(ru_sta,
			ru_sta->mpdu_length+13,
			configs->stbc,
			configs->sgi,
			configs->max_pkt_ext);

			configs->dmnt_ru_idx = 0;

			/*Replace mcs/nss/ldpc/mpdu_len setting*/
			configs->mcs = ru_sta->rate;
			configs->nss = ru_sta->nss;
			configs->ldpc = ru_sta->ldpc;
			tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PKTLEN,
			ru_sta->mpdu_length);

			/*Do Calc Manual HE TB TX*/
			mt_op_set_manual_he_tb_value(winfos,
			ru_sta, configs);

			/*restore configs->dmnt_ru_idx*/
			configs->dmnt_ru_idx = backukp_dmnt_ru_idx;
		}
	}
	mcs |= 0x80000000;
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RATE, mcs);
	}

	return SERV_STATUS_SUCCESS;
}
#endif /* (CFG_SUPPORT_CONNAC3X == 0) || (CFG_SUPPORT_CONNAC5X == 0) */


#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
static void mt_op_set_manual_eht_tb_value(
	struct test_wlan_info *winfos,
	struct test_ru_info *ru_sta,
	struct test_configuration *configs)
{
	union hetb_rx_cmm cmm;
	union ehttb_tx_usr usr;
	union ehttb_spec_usr specUsr;
	u_int32 nss;
	u_int32 mapping_bw;
	u_int8 ltf_sym_code[] = {
		0, 0, 1, 2, 2, 3, 3, 4, 4   /* SS 1~8 */
	};
	u_int8 i;

	/* setup MAC start */
	/* step 1-1, common info of TF */
	sys_ad_zero_mem(&cmm, sizeof(cmm));
	cmm.field.sig_a_reserved = 0x1fc;

	for (i = 0; i < 4; i++) {
		if ((configs->seg_sta_cnt[i] > 0) && (ru_sta->dRU_en == TRUE))
			cmm.field.sig_a_reserved &= ~BIT(2+i);
	}

	/* assign dRU EN */

	cmm.field.ul_length = ru_sta->l_len;
	cmm.field.t_pe =
	(ru_sta->afactor_init & 0x3) | ((ru_sta->pe_disamb & 0x1) << 2);
	cmm.field.ldpc_extra_sym = ru_sta->ldpc_extr_sym;
	nss = (ru_sta->ru_mu_nss > ru_sta->nss) ?
		ru_sta->ru_mu_nss : ru_sta->nss;
	if (ru_sta->ru_mu_nss > ru_sta->nss)
		cmm.field.mimo_ltf = 1;
	if (configs->stbc && nss == 1)
		cmm.field.ltf_sym_midiam = ltf_sym_code[nss+1];
	else
		cmm.field.ltf_sym_midiam = ltf_sym_code[nss];
	cmm.field.gi_ltf = configs->sgi;

	mapping_bw = tm_bw_hqa_mapping_at((u_int32) configs->per_pkt_bw);
	if (mapping_bw >= PROACTIVE_BW320)
		cmm.field.ul_bw = PROACTIVE_BW160;
	else
		cmm.field.ul_bw = mapping_bw;
	cmm.field.stbc = configs->stbc;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR0 ] tigger_type:0x%x,\n",
	__func__, cmm.field.tigger_type));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("ul_length:0x%x cascade_ind:0x%x,\n",
	cmm.field.ul_length, cmm.field.cascade_ind));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("cs_required:0x%x, ul_bw:0x%x,\n",
	cmm.field.cs_required, cmm.field.ul_bw));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("gi_ltf:0x%x, mimo_ltf:0x%x,\n",
	cmm.field.gi_ltf, cmm.field.mimo_ltf));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("ltf_sym_midiam:0x%x, stbc:0x%x,\n",
	cmm.field.ltf_sym_midiam, cmm.field.stbc));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("ldpc_extra_sym :0x%x, ap_tx_pwr: 0x%x\n",
	cmm.field.ldpc_extra_sym, cmm.field.ap_tx_pwr));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR0 ] cr_value:0x%08x\n",
	__func__, (u_int32)(cmm.cmm_info & 0xffffffff)));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR1 ] cr_value:0x%08x\n",
	__func__, (u_int32)((cmm.cmm_info & 0xffffffff00000000) >> 32)));

	/* step 1-2, users info */
	sys_ad_zero_mem(&usr, sizeof(usr));
	usr.field.aid = 0x0;
	usr.field.neoab = 0;
	usr.field.mimo_nss = 0;
	usr.field.allocation = ru_sta->ru_index;
	usr.field.coding = ru_sta->ldpc;
	usr.field.mcs = ru_sta->rate & ~BIT(4);
	usr.field.dcm = 0;
	usr.field.start_ss = ru_sta->start_sp_st & 0xF;
	usr.field.Nos = (nss-1);
	usr.field.Ps160 = ru_sta->ps160;
	usr.field.reserve2 = 0;
	usr.field.UserType = 2;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR2 ] aid:%d\n",
	__func__,  usr.field.aid));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("allocation:%d, coding:%d,\n",
	usr.field.allocation, usr.field.coding));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("mcs:0x%x, start_ss:%d,\n",
	usr.field.mcs, usr.field.start_ss));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("Nos:%d, Ps160:%d\n",
	usr.field.Nos, usr.field.Ps160));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR2 ] cr_value:0x%08x\n",
	__func__, (u_int32)(usr.usr_info & 0xffffffff)));

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR3 ] cr_value:0x%08x\n",
	__func__, (u_int32)((usr.usr_info & 0xffffffff00000000) >> 32)));

	/* step 2, spec users info */
	specUsr.field.aid = 2007;
	specUsr.field.PhyId = 0;
	specUsr.field.SR1 = 0;
	specUsr.field.SR2 = 0;
	specUsr.field.disregard = 0;
	specUsr.field.reserve = 0;
	specUsr.field.UserType = 2;

	/* handle BW_EXT */
	if (mapping_bw == PROACTIVE_BW160)
		specUsr.field.ExtUlBw = 1;
	else if (mapping_bw >= PROACTIVE_BW320)
		specUsr.field.ExtUlBw = 2;
	else
		specUsr.field.ExtUlBw = 0;

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("mapping_bw:%d, ExtUlBw:%d\n",
	mapping_bw, specUsr.field.ExtUlBw));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR7 ] cr_value:0x%08x\n",
	__func__, (u_int32)(specUsr.usr_info & 0xffffffff)));
	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_OFF,
	("%s: [TF TTRCR8 ] cr_value:0x%08x\n",
	__func__, (u_int32)((specUsr.usr_info & 0xffffffff00000000) >> 32)));

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR0,
		(cmm.cmm_info & 0xffffffff));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR1,
		((cmm.cmm_info & 0xffffffff00000000) >> 32));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR2,
		(usr.usr_info & 0xffffffff));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR3,
		((usr.usr_info & 0xffffffff00000000) >> 32));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR4, 0xffffffff);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR5, 0xffffffff);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR6, 0xffffffff);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR7,
		(specUsr.usr_info & 0xffffffff));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TX_HE_TB_TTRCR8,
		((specUsr.usr_info & 0xffffffff00000000) >> 32));

}
#endif

s_int32 mt_op_start_tx(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_char tx_pwr = configs->tx_pwr[configs->nss];
	u_int32 aifs = configs->ipg_param.ipg;
	u_int32 pkt_cnt = configs->tx_stat.tx_cnt;
	s_int32 ret = SERV_STATUS_SUCCESS;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	struct test_ru_info *ru_sta = &configs->ru_info_list[0];
#endif
	struct param_mtk_wifi_test_struct rf_at_info;
	u_int32 tx_cnt = 0, buf_len = 0;
#if (CFG_WAIT_TSSI_READY == 1)
	u_char i;
#endif

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PKTCNT, pkt_cnt);

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	/* QA tool pass through to FW */
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PREAMBLE, configs->tx_mode);

	if ((configs->tx_mode == TEST_MODE_HE_TB) ||
		(configs->tx_mode == TEST_MODE_EHT_TB_UL_OFDMA)) {
		/*do ru operation*/
		if (ru_sta->valid) {
			/*Calculate HE TB PHY Info*/
			mt_engine_calc_phy(ru_sta,
					ru_sta->mpdu_length+13,
					configs->stbc,
					configs->sgi,
					configs->max_pkt_ext);

			configs->dmnt_ru_idx = 0;

			/*Replace the mcs/nss/ldpc/mpdu_len setting*/
			configs->mcs = ru_sta->rate;
			configs->nss = ru_sta->nss;
			configs->ldpc = ru_sta->ldpc;
			tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PKTLEN, ru_sta->mpdu_length);

			if (configs->tx_mode == TEST_MODE_HE_TB)
				/*Do Calc Manual HE TB TX*/
				mt_op_set_manual_he_tb_value(winfos,
				ru_sta, configs);
			else
				/*Do Calc Manual EHT TB TX*/
				mt_op_set_manual_eht_tb_value(winfos,
				ru_sta, configs);
		}
	}

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RATE, configs->mcs);

	if (ru_sta->dRU_valid)
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_EHTTB_DRU_ENABLE, ru_sta->dRU_en);

#else
	tm_trans_Preamble_rate(winfos, configs);
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

	if (tx_pwr > 0x3F)
		tx_pwr += 128;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_POWER, tx_pwr);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_STBC, (u_int32)configs->stbc);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_ENCODE_MODE, (u_int32)configs->ldpc);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_IBF_ENABLE, (u_int32)configs->ibf);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_EBF_ENABLE, (u_int32)configs->ebf);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PKTINTERVAL, aifs);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_GI, configs->sgi);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_NSS, configs->nss);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_HWTX_MODE, winfos->hw_tx_enable);

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_PUNCTURE, configs->puncture);
#endif

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STARTTX);

	/* For production line test, get tx count for wait calibration ready */
#if (CFG_WAIT_TSSI_READY == 1)
	for (i = 0; i < 100; i++) {
#endif
		if (band_idx == TEST_DBDC_BAND0)
			rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT;
		else
			rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT | BIT(8);

		rf_at_info.func_data = 0;
		ret = tm_rftest_query_auto_test(winfos,
			&rf_at_info, &buf_len);
		if (ret == SERV_STATUS_SUCCESS)
			tx_cnt = rf_at_info.func_data;

#if (CFG_WAIT_TSSI_READY == 1)
		/* For production line test,
		 *  get tx count 16 for wait tssi ready
		 */
		if (tx_cnt >= 16)
			break;

		msleep(20);
	}
#endif


	return ret;
}

s_int32 mt_op_stop_tx(
	struct test_wlan_info *winfos,
	u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STOPTEST);

	return ret;
}

s_int32 mt_op_start_rx(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 func_data;
	struct param_mtk_wifi_test_struct rf_at_info;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PREAMBLE, configs->tx_mode);

	if ((configs->tx_mode == TEST_MODE_HE_MU)
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	|| (configs->tx_mode == TEST_MODE_EHT_MU_DL_SU)
	|| (configs->tx_mode == TEST_MODE_EHT_MU_UL_SU)
	|| (configs->tx_mode == TEST_MODE_EHT_MU_DL_OFDMA)
#endif
	){
		if (configs->mu_rx_aid)
			ret = tm_rftest_set_auto_test(winfos,
					RF_AT_FUNCID_SET_RX_MU_AID,
					configs->mu_rx_aid);
		else
			ret = tm_rftest_set_auto_test(winfos,
					RF_AT_FUNCID_SET_RX_MU_AID,
					0xf800);/* 0xf800 to disable */
	} else {
		ret = tm_rftest_set_auto_test(winfos,
					RF_AT_FUNCID_SET_RX_MU_AID,
					0xf800);/* 0xf800 to disable */
	}

	sys_ad_move_mem(&func_data, configs->own_mac, 4);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TA, func_data);

	func_data = 0;
	sys_ad_move_mem(&func_data, configs->own_mac + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_TA | BIT(18)), func_data);

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STARTRX);

	/* For production line test, get tx count for wait calibration ready */
	if (band_idx == TEST_DBDC_BAND0)
		rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT;
	else
		rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT | BIT(8);

	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

	return ret;
}

s_int32 mt_op_stop_rx(
	struct test_wlan_info *winfos,
	u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STOPTEST);

	return ret;
}

s_int32 mt_op_set_channel(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_char central_ch0 = configs->channel;
	u_char central_ch1 = configs->channel_2nd;
	u_char sys_bw = configs->bw;
	u_char per_pkt_bw = configs->per_pkt_bw;
	u_char ch_band = configs->ch_band;
	u_char pri_sel = configs->pri_sel;
	s_int32 SetFreq = 0;
	s_int32 ret = SERV_STATUS_SUCCESS;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	/* For POR Cal Setting - 20160601 */
	if ((central_ch0 == central_ch1) &&
		(sys_bw == 6) && (per_pkt_bw == 6)) {
		return ret;
	}

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if ((central_ch0 >= 7 && central_ch0 <= 16) && ch_band == 1) {
		/*Ch7 - Ch12, 5G (5035-5060)  ch_band: 0:2.4G    1:5G */
		SetFreq = 1000 * (5000 + central_ch0 * 5);
	} else if (central_ch0 == 6 && ch_band == 1) {
		SetFreq = 1000 * 5032;
	} else if ((central_ch0 >= 1 && central_ch0 <= 233) && ch_band == 2) {
		/*ch_band: 2: 6G */
	if (central_ch0 == 2)
		SetFreq = 5935000;
	else
		    SetFreq = 1000 * (5950 + central_ch0 * 5);
	} else {
		SetFreq = tm_ch_num_to_freq((u_int32)central_ch0);
	}

	tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_CBW,
			tm_bw_hqa_mapping_at((u_int32)sys_bw));

	/* For POR Cal Setting - 20160601 */
	if ((sys_bw == 6) && (per_pkt_bw == 6))
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBW, tm_bw_hqa_mapping_at(5));
	else {
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBW,
			tm_bw_hqa_mapping_at((u_int32)per_pkt_bw));
	}

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_PRIMARY_CH, pri_sel);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_BAND, ch_band);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_CHNL_FREQ, SetFreq);

	if (sys_bw == 6) {
		SetFreq = tm_ch_num_to_freq((u_int32)central_ch1);
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_CHNL_FREQ | BIT(16)), SetFreq);
	}

	return ret;
}

s_int32 mt_op_set_tx_content(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	struct serv_hdr_802_11 *phdr = NULL;
	u_int32 func_data;
	u_int16 fc_16;
	u_int32 fc, dur, seq;
	u_int32 gen_payload_rule = configs->fixed_payload;
	u_int32 pay_load = configs->payload[0];
	u_int32 tx_len = configs->tx_len;
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	u_int32 tx_time = configs->tx_time_param.pkt_tx_time;
	boolean enable = FALSE;
#endif

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	phdr = (struct serv_hdr_802_11 *)&configs->template_frame[0];
	sys_ad_move_mem(&fc_16, &phdr->fc, sizeof(fc_16));
	fc = (u_int32)fc_16;
	dur = (u_int32)phdr->duration;
	seq = (u_int32)phdr->sequence;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_MAC_HEADER,
		(fc | (dur << 16)));
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_SEQ_CTRL, seq);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_PAYLOAD,
		((gen_payload_rule << 16) | pay_load));

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	enable = configs->tx_time_param.pkt_tx_time_en;
	if (enable) {
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PKTLEN, 0);
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_TIME, tx_time);
	} else {
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PKTLEN, tx_len);
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_TIME, 0);
	}

	sys_ad_move_mem(&func_data, configs->addr2[0], 4);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TA, func_data);
	func_data = 0;
	sys_ad_move_mem(&func_data, configs->addr2[0] + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_TA | MAC_TA_ADDRESS_OFFSET_ENB), func_data);

	sys_ad_move_mem(&func_data, configs->addr3[0], 4);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_BSSID, func_data);
	func_data = 0;
	sys_ad_move_mem(&func_data, configs->addr3[0] + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_BSSID | MAC_TA_ADDRESS_OFFSET_ENB),
			func_data);

	sys_ad_move_mem(&func_data, configs->addr1[0], 4);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_MAC_ADDRESS, func_data);
	func_data = 0;
	sys_ad_move_mem(&func_data, configs->addr1[0] + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_MAC_ADDRESS | MAC_TA_ADDRESS_OFFSET_ENB),
		func_data);
#else
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PKTLEN, tx_len);

	sys_ad_move_mem(&func_data, configs->addr1[0], 4);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_MAC_ADDRESS, func_data);

	func_data = 0;
	sys_ad_move_mem(&func_data, configs->addr1[0] + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_MAC_ADDRESS | MAC_TA_ADDRESS_OFFSET_ENB),
		func_data);

	sys_ad_move_mem(&func_data, configs->addr2[0], 4);
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TA, func_data);

	func_data = 0;
	sys_ad_move_mem(&func_data, configs->addr2[0] + 4, 2);
	tm_rftest_set_auto_test(winfos,
		(RF_AT_FUNCID_SET_TA | MAC_TA_ADDRESS_OFFSET_ENB), func_data);
#endif  /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

	return ret;
}

s_int32 mt_op_set_tmr(
	struct test_wlan_info *winfos,
	struct test_tmr_info *tmr_info)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	do {
		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX, tmr_info->band_idx);
		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s RF_AT_FUNCID_SET_DBDC_BAND_IDX ret=%d\n",
				__func__, ret));

			break;
		}

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TMR_ROLE, tmr_info->setting);

		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s RF_AT_FUNCID_SET_TMR_ROLE ret=%d\n",
				__func__, ret));

			break;
		}

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TMR_MODULE, tmr_info->version);

		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s RF_AT_FUNCID_SET_TMR_MODULE ret=%d\n",
				__func__, ret));

			break;
		}

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TMR_DBM, tmr_info->through_hold);

		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s RF_AT_FUNCID_SET_TMR_DBM ret=%d\n",
				__func__, ret));

			break;
		}

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TMR_ITER, tmr_info->iter);

		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s RF_AT_FUNCID_SET_TMR_ITER ret=%d\n",
				__func__, ret));

			break;
		}

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TMR_TOAE_CAL_RESOTRE,
			tmr_info->toae_cal);

		if (ret != SERV_STATUS_SUCCESS) {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE, (
				"%s RF_AT_FUNCID_SET_TMR_TOAE_CAL_RESOTRE ret=%d\n",
				__func__, ret));

			break;
		}
	} while (0);

	return ret;

}

s_int32 mt_op_set_preamble(
	struct test_wlan_info *winfos,
	u_char mode)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	/* QA tool pass through to FW
	typedef enum
	{
		PREAMBLE_CCK = 0,
		PREAMBLE_OFDM,
		PREAMBLE_MIX_MODE,
		PREAMBLE_GREEN_FIELD,
		PREAMBLE_VHT,
		PREAMBLE_PLR_MODE,
		PREAMBLE_HE_SU = 8,
		PREAMBLE_HE_ER,
		PREAMBLE_HE_TRIG,
		PREAMBLE_HE_MU,
		PREAMBLE_EHT_MU_DL_SU = 13,
		PREAMBLE_EHT_MU_UL_SU,
		PREAMBLE_EHT_MU_DL_OFDMA,
		PREAMBLE_EHT_TB_UL_OFDMA
	} E_PREAMBLE_TYPE, *P_E_PREAMBLE_TYPE;
	*/

#else
	g_tx_mode = mode;

	if (mode == TEST_MODE_OFDM)
		mode = TEST_MODE_CCK;
#endif

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_ERROR,
			("%s: tx mode = %d\n",
				__func__, mode));

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PREAMBLE, mode);

	return ret;
}

s_int32 mt_op_set_rate(
	struct test_wlan_info *winfos,
	u_char mcs)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	/* QA tool pass through to FW */

#else

	if (g_tx_mode == TEST_MODE_OFDM) {
		g_tx_mode = TEST_MODE_CCK;
		mcs += 4;
	}

	if (g_tx_mode == TEST_MODE_CCK)
		mcs |= 0x00000000;
	else if (g_tx_mode == TEST_MODE_OFDM) {
		if (mcs == 9)
			mcs = 1;
		else if (mcs == 10)
			mcs = 2;
		else if (mcs == 11)
			mcs = 3;
		mcs |= 0x00000000;
	} else if (g_tx_mode >= TEST_MODE_HTMIX &&
	g_tx_mode <= TEST_MODE_HE_TB)
		mcs |= 0x80000000;
#endif

	SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_ERROR,
			("%s: mcs = %d\n",
				__func__, mcs));

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RATE, mcs);

	return ret;
}

s_int32 mt_op_set_system_bw(
	struct test_wlan_info *winfos,
	u_char sys_bw)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_CBW,
		tm_bw_hqa_mapping_at((u_int32)sys_bw));

	return ret;
}

s_int32 mt_op_set_per_pkt_bw(
	struct test_wlan_info *winfos,
	u_char per_pkt_bw)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBW,
			tm_bw_hqa_mapping_at((u_int32)per_pkt_bw));

	return ret;
}

s_int32 mt_op_reset_txrx_counter(
	struct test_wlan_info *winfos)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RESETTXRXCOUNTER, 0);

	return ret;
}

s_int32 mt_op_set_rx_vector_idx(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 group1,
	u_int32 group2)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 func_data;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	func_data = group1;
	func_data |= (group2 << 8);
	func_data |= (u_int32)(band_idx << 16);

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_RXV_INDEX, func_data);

	return ret;
}

s_int32 mt_op_get_rx_stat_leg(
	struct test_wlan_info *winfos,
	struct test_rx_stat_leg *rx_stat)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_fagc_rssi_path(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 fagc_path)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 func_data;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	func_data = (u_int32)(band_idx << 16) | (fagc_path & BITS(0, 15));

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_FAGC_RSSI_PATH, func_data);

	return ret;
}

s_int32 mt_op_dbdc_tx_tone(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 func_data;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	/*
	 * Select TX Antenna
	 * RF_Power: (1db) 0~15
	 * Digi_Power: (0.25db) -32~31
	 */
	func_data = (u_int32)configs->ant_idx << 16 | configs->rf_pwr;
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TONE_RF_GAIN, func_data);
	func_data = (u_int32)configs->ant_idx << 16 | configs->digi_pwr;
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_TONE_DIGITAL_GAIN, func_data);

	/* DBDC Band Index : Band0, Band1 */
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if (configs->tx_tone_en) {
		/* Band : 2.4G/5G */
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_BAND,
			(u_int32)configs->ch_band);

		/* ToneType : Single or Two */
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TONE_TYPE,
			(u_int32)configs->tone_type);

		/* ToneFreq: DC/5M/10M/20M/40M */
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TONE_BW,
			configs->tone_freq);

		/* DC Offset I, DC Offset Q */
		func_data = (configs->dc_offset_Q << 16) |
			configs->dc_offset_I;
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TONE_DC_OFFSET,
			func_data);

		/* Control TX Tone Start and Stop */
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_COMMAND,
			RF_AT_COMMAND_SINGLE_TONE);
	} else {
		/* Control TX Tone Start and Stop */
		tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STOPTEST);
	}

	return ret;
}

s_int32 mt_op_dbdc_tx_tone_pwr(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_dbdc_continuous_tx(
	struct test_wlan_info *winfos,
	u_char band_idx,
	struct test_configuration *configs)
{
	s_int32 ret;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 tx_mode = configs->tx_mode;
	u_int32 rate = configs->rate;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	if (configs->tx_tone_en) {
		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX,
			(u_int32)band_idx);

		tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_PRIMARY_CH,
			configs->pri_sel);

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
		/* QA tool pass through to FW */
		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PREAMBLE, tx_mode);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_RATE, rate);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

#else

		if (tx_mode == 1) {
			tx_mode = 0;
			rate += 4;
		} else if ((tx_mode == 0) &&
			((rate == 9) || (rate == 10) || (rate == 11)))
			tx_mode = 1;

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_PREAMBLE, tx_mode);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

		if (tx_mode == 0) {
			rate |= 0x00000000;
			tm_rftest_set_auto_test(winfos,
				RF_AT_FUNCID_RATE, rate);
		} else if (tx_mode == 1) {
			if (rate == 9)
				rate = 1;
			else if (rate == 10)
				rate = 2;
			else if (rate == 11)
				rate = 3;
			rate |= 0x00000000;

			tm_rftest_set_auto_test(winfos,
				RF_AT_FUNCID_RATE, rate);
		} else if (tx_mode >= 2 && tx_mode <= 4) {
			rate |= 0x80000000;

			tm_rftest_set_auto_test(winfos,
				RF_AT_FUNCID_RATE, rate);
		}
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_CBW,
			tm_bw_hqa_mapping_at((u_int32)configs->bw));

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_CW_MODE,
			configs->tx_fd_mode);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_ANTMASK,
			(u_int32)configs->ant_mask);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_CW);
		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;

	} else {
		ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_COMMAND,
			RF_AT_COMMAND_STOPTEST);

		if (ret != SERV_STATUS_SUCCESS)
			return SERV_STATUS_HAL_OP_FAIL;
	}
	return ret;
}

s_int32 mt_op_get_tx_info(
	struct test_wlan_info *winfos,
	struct test_configuration *test_configs_band0,
	struct test_configuration *test_configs_band1,
	struct test_configuration *test_configs_band2)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	/* make sure use AT CMD (32) get txed count by BN0 dbdc idx */
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, TEST_DBDC_BAND0);

	rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT;
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		test_configs_band0->tx_stat.tx_done_cnt = rf_at_info.func_data;

	/* make sure use AT CMD (288) get txed count by BN1 dbdc idx */
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, TEST_DBDC_BAND1);

	rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT | BIT(8);
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		test_configs_band1->tx_stat.tx_done_cnt = rf_at_info.func_data;

#if (CFG_SUPPORT_CONNAC3X == 1)
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, TEST_DBDC_BAND2);

	rf_at_info.func_idx = RF_AT_FUNCID_TXED_COUNT;
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		test_configs_band2->tx_stat.tx_done_cnt = rf_at_info.func_data;
#endif

	return ret;
}

s_int32 mt_op_get_rx_statistics_all(
	struct test_wlan_info *winfos,
	struct hqa_comm_rx_stat *hqa_rx_stat)
{
	return SERV_STATUS_SERV_TEST_NOT_SUPPORTED;
}

s_int32 mt_op_get_capability(
	struct test_wlan_info *winfos,
	struct test_capability *capability)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_CAPABILITY,
		NULL,
		0,
		NULL,
		capability);

	return ret;
}

s_int32 mt_op_calibration_test_mode(
	struct test_wlan_info *winfos,
	u_char mode)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	if (mode == 2)
		ret = tm_icap_mode(winfos);

	return ret;
}

s_int32 mt_op_set_icap_start(
	struct test_wlan_info *winfos,
	u_int8 *data)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct test_struct_ext r_test_info;
	struct hqa_rbist_cap_start *pr_rbist_info;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	pr_rbist_info = &(r_test_info.data.icap_info);

	/*copy icap start parameters*/
	sys_ad_move_mem(pr_rbist_info,
		data,
		sizeof(struct hqa_rbist_cap_start));

	/* over write parameters for mobile setting */
	/* 0:32 1:96 2:128 3:64 4:disable(no need to translate) */
	pr_rbist_info->en_bit_width = winfos->icap_bitwidth;
	/* 0:Support on-chip, 1:Support on-the fly */
	pr_rbist_info->arch = winfos->icap_arch;
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	pr_rbist_info->phy_idx = winfos->icap_phy_idx;
#endif
	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("%s: en_bit_width = 0x%08x, arch = 0x%08x, phy_idx = 0x%08x\n",
		__func__,
		pr_rbist_info->en_bit_width,
		pr_rbist_info->arch,
		pr_rbist_info->phy_idx));

	pr_rbist_info->emi_start_addr =
		(u_int32) (winfos->emi_phy_base & 0xFFFFFFFF);
	pr_rbist_info->emi_end_addr =
		(u_int32) ((winfos->emi_phy_base +
			winfos->emi_phy_size) & 0xFFFFFFFF);
	pr_rbist_info->emi_msb_addr = 0; /*CONNAC 1.x useless*/

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("%s: StartAddr=0x%08x,EndAddr=0x%08x,MsbAddr=0x%08x\n",
		__func__,
		pr_rbist_info->emi_start_addr,
		pr_rbist_info->emi_end_addr,
		pr_rbist_info->emi_msb_addr));

	if (pr_rbist_info->trig == 1) { /*Start Capture data*/
		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_SET_TEST_ICAP_START,
			&r_test_info,
			sizeof(r_test_info),
			NULL,
			NULL);
	} else if (pr_rbist_info->trig == 0) { /*don't Capture data*/
		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_SET_TEST_ICAP_ABORT,
			&r_test_info,
			sizeof(r_test_info),
			NULL,
			NULL);
	}

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("Start ICAP trig:%d,node=0x%08x,ret=0x%08x\n",
		pr_rbist_info->trig,
		pr_rbist_info->cap_node,
		ret));

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_icap_status(
	struct test_wlan_info *winfos,
	s_int32 *icap_stat)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct test_struct_ext r_test_info;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_SET_TEST_ICAP_STATUS,
		&r_test_info,
		sizeof(r_test_info),
		NULL,
		icap_stat);

	if (ret)
		ret = SERV_STATUS_HAL_OP_FAIL_SEND_FWCMD;

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("Query Status ICAP %d, ret=0x%X\n",
		*icap_stat,  /*0:OK,1:WAITING,2:FAIL*/
		ret));

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_icap_max_data_len(
	struct test_wlan_info *winfos,
	u_long *max_data_len)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_TEST_ICAP_MAX_DATA_LEN,
		NULL,
		0,
		NULL,
		max_data_len);

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("ICAP max data len %lu", *max_data_len));


	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_icap_data(
	struct test_wlan_info *winfos,
	s_int32 *icap_cnt,
	s_int32 *icap_data,
	u_int32 wf_num,
	u_int32 iq_type)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct icap_dump_iq r_dump_iq;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	r_dump_iq.wf_num = wf_num;
	r_dump_iq.iq_type = iq_type;
	r_dump_iq.icap_cnt = 0; /* QATool's IQNumberCount */
	r_dump_iq.icap_data_len = 0; /* QATool's IQNumberTotalCount */
	r_dump_iq.picap_data = (s_int8 *)icap_data;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_TEST_ICAP_DATA,
		&r_dump_iq,
		sizeof(r_dump_iq),
		NULL,
		NULL);

	if (ret == SERV_STATUS_SUCCESS) {
		*icap_cnt = r_dump_iq.icap_cnt;
		/* debug */
		if (g_hqa_frame_ctrl == 1) {
			sys_ad_mem_dump32(icap_data, r_dump_iq.icap_data_len);
		}
	}

	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("ICAP:wf[%d][%c],cnt:%d,len:%d,data:0x%p,ret:0x%X\n",
		r_dump_iq.wf_num,
		(r_dump_iq.iq_type == ATE_CAP_I_TYPE)?'I':'Q',
		*icap_cnt,
		r_dump_iq.icap_data_len,
		icap_data,
		ret));


	return ret;
}

s_int32 mt_op_do_cal_item(
	struct test_wlan_info *winfos,
	u_int32 item,
	u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_RESET_RECAL_COUNT,
			NULL,
			0,
			NULL,
			NULL);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_RECAL_CAL_STEP, item);

	return ret;
}

s_int32 mt_op_set_band_mode(
	struct test_wlan_info *winfos,
	struct test_band_state *band_state)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 dbdc_enb;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	u_int32 fw_band_mode;
#endif

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	if (band_state->band_mode == TEST_BAND_MODE_DUAL)
		dbdc_enb = TEST_DBDC_ENABLE;
	else
		dbdc_enb = TEST_DBDC_DISABLE;

	SET_TEST_DBDC(winfos, dbdc_enb);

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)

	/* band_state->band_mode BIT24: TEST_BAND_MODE_SINGLE_BAND0/1 */
	fw_band_mode =
		(band_state->band_mode & 0xffffff00) |	dbdc_enb;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_ENABLE, fw_band_mode);
#else
	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_ENABLE, dbdc_enb);
#endif

	return ret;
}

s_int32 mt_op_get_chipid(
	struct test_wlan_info *winfos)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_mps_start(
	struct test_wlan_info *winfos,
	u_char band_idx)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_COMMAND, RF_AT_COMMAND_STARTTX);

	return ret;
}

s_int32 mt_op_mps_set_nss(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	s_int32 ret = SERV_STATUS_SUCCESS;

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_NSS | (i << 16)),
			mps_setting[i+1].nss);
	}

	return ret;
}

s_int32 mt_op_mps_set_per_packet_bw(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	s_int32 ret = SERV_STATUS_SUCCESS;

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_PACKAGE_BW | (i << 16)),
			mps_setting[i+1].pkt_bw);
	}

	return ret;
}

s_int32 mt_op_mps_set_packet_count(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	s_int32 ret = SERV_STATUS_SUCCESS;

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_PKT_CNT | (i << 16)),
			mps_setting[i+1].pkt_cnt);
	}

	return ret;
}

s_int32 mt_op_mps_set_payload_length(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	s_int32 ret = SERV_STATUS_SUCCESS;

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_PAYLOAD_LEN | (i << 16)),
			mps_setting[i+1].pkt_len);
	}

	return ret;
}

s_int32 mt_op_mps_set_power_gain(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	s_int32 ret = SERV_STATUS_SUCCESS;

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_PWR_GAIN | (i << 16)),
			mps_setting[i+1].pwr);
	}

	return ret;
}

s_int32 mt_op_mps_set_seq_data(
	struct test_wlan_info *winfos,
	u_int32 len,
	struct test_mps_setting *mps_setting)
{
	u_int32 i;
	u_int32 *mps_set = NULL;
	u_int32 mode, mcs, tx_path;
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	ret = sys_ad_alloc_mem((u_char **)&mps_set, sizeof(u_int32) * len);

	if (pr_oid_funcptr == NULL)	{
		sys_ad_free_mem(mps_set);
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;
	}

	for (i = 0; i < len; i++) {
		mode = mps_setting[i+1].tx_mode;
		mcs = mps_setting[i+1].mcs;
		tx_path = mps_setting[i+1].tx_ant;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
		/* QA tool pass through to FW */
#else

		if (mode == 1) {
			mode = 0;
			mcs += 4;
		} else if ((mode == 0) && ((mcs == 9) || (mcs == 10) ||
			(mcs == 11))) {
			mode = 1;
		}

		if (mode == 0) {
			mcs |= 0x00000000;
		} else if (mode == 1) {
			if (mcs == 9)
				mcs = 1;
			else if (mcs == 10)
				mcs = 2;
			else if (mcs == 11)
				mcs = 3;
			mcs |= 0x00000000;
		} else if (mode >= 2 && mode <= 4) {
			mcs |= 0x80000000;
		}
#endif  /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */
		mps_set[i] = (mcs) | (tx_path << 8) | (mode << 24);

	}

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_MPS_SIZE, len);

	for (i = 0; i < len; i++) {
		tm_rftest_set_auto_test(winfos,
			(RF_AT_FUNCID_SET_MPS_SEQ_DATA | (i << 16)),
			mps_set[i]);
	}

	sys_ad_free_mem(mps_set);

	return ret;
}

s_int32 mt_op_get_tx_pwr(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx,
	u_char channel,
	u_char ant_idx,
	u_int32 *power)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct param_mtk_wifi_test_struct rf_at_info;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	tm_trans_Preamble_rate(winfos, configs);
#endif /* (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0) */

	rf_at_info.func_idx = RF_AT_FUNCID_GET_TX_POWER;
	rf_at_info.func_data = 0;

	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS)	{
		*power = rf_at_info.func_data;

		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: pwr:%u!\n",
			__func__, *power));
	} else {
		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s:  fail!\n",
			__func__));
	}

	return ret;
}

s_int32 mt_op_get_tx_default_pwr(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx,
	u_char channel,
	u_char ant_idx,
	u_int32 *power)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct param_mtk_wifi_test_struct rf_at_info;
	u_int32 buf_len = 0;
#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	struct test_ru_info *ru_sta = &configs->ru_info_list[0];
#endif

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_PREAMBLE, configs->tx_mode);

	if ((configs->tx_mode == TEST_MODE_HE_TB) ||
		(configs->tx_mode == TEST_MODE_EHT_TB_UL_OFDMA)) {
		if (ru_sta->valid) {
			configs->dmnt_ru_idx = 0;

			/* apply ru rate*/
			configs->mcs = ru_sta->rate;
			configs->nss = ru_sta->nss;
			configs->ldpc = ru_sta->ldpc;
			if (configs->tx_mode == TEST_MODE_HE_TB)
				/*Do Calc Manual HE TB TX*/
				mt_op_set_manual_he_tb_value(winfos,
				ru_sta, configs);
			else
				/*Do Calc Manual EHT TB TX*/
				mt_op_set_manual_eht_tb_value(winfos,
				ru_sta, configs);
		}
	}

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_RATE, configs->mcs);
#else
	tm_trans_Preamble_rate(winfos, configs);
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

	rf_at_info.func_idx = RF_AT_FUNCID_GET_DEFAULT_TX_POWER;
	rf_at_info.func_data = 0;

	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS)	{
		*power = rf_at_info.func_data;

		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: pwr:%u!\n",
			__func__, *power));
	} else {
		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s:  fail!\n",
			__func__));
	}

	return ret;
}

s_int32 mt_op_set_get_pwr_type(
	struct test_wlan_info *winfos,
	u_int32_t powertype)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_CMD_SET_GET_POWER_TYPE, powertype);

	return ret;
}

s_int32 mt_op_set_tx_pwr(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx,
	struct test_txpwr_param *pwr_param)
{
	u_int32 Pwr = pwr_param->power;
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	if (Pwr > 0x3F)
		Pwr += 128;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_POWER, Pwr);

	return ret;
}

s_int32 mt_op_get_freq_offset(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 *freq_offset)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rf_at_info.func_idx = RF_AT_FUNCID_GET_FREQ_OFFSET;
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		*freq_offset = rf_at_info.func_data;

	return ret;
}

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
s_int32 mt_op_get_freq_offset_C2(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 *freq_offset)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rf_at_info.func_idx = RF_AT_FUNCID_SET_FREQ_OFFSET_C2_GET;
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		*freq_offset = rf_at_info.func_data;

	return ret;
}
#endif

s_int32 mt_op_get_cfg_on_off(
	struct test_wlan_info *winfos,
	u_int32 type,
	u_int32 band_idx,
	u_int32 ch_band,
	u_int32 *result)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
	struct param_mtk_wifi_test_struct rf_at_info;
	u_int32 buf_len = 0;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_BAND, ch_band);

	rf_at_info.func_idx = RF_AT_FUNCID_GET_CFG_ON_OFF;
	rf_at_info.func_data = type;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);
	if (ret == SERV_STATUS_SUCCESS)
		*result = rf_at_info.func_data;
#endif

	return ret;
}

s_int32 mt_op_get_tx_tone_pwr(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 ant_idx,
	u_int32 *power)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_reset_recal_cnt(
	struct test_wlan_info *winfos
)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_RESET_RECAL_COUNT,
		NULL,
		0,
		0,
		NULL);

	return ret;
}

s_int32 mt_op_get_recal_cnt(
	struct test_wlan_info *winfos,
	u_int32 *recal_cnt,
	u_int32 *recal_dw_num)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;


	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_RECAL_COUNT,
		NULL,
		0,
		recal_cnt,
		NULL);

	*recal_dw_num = 3;	/* ncal_id, cal_addr, cal_value */

	return ret;
}

s_int32 mt_op_get_recal_content(
	struct test_wlan_info *winfos,
	u_int32 *content)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_RECAL_CONTENT,
		NULL,
		0,
		NULL,
		content);

	return ret;
}

s_int32 mt_op_get_rxv_cnt(
	struct test_wlan_info *winfos,
	u_int32 *rxv_cnt,
	u_int32 *rxv_dw_num)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rf_at_info.func_idx = RF_AT_FUNCID_RESULT_INFO;
	rf_at_info.func_data = RF_AT_FUNCID_RXV_DUMP;
	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS)	{
		if (rf_at_info.func_data > 56)
			rf_at_info.func_data = 56;

		*rxv_cnt = rf_at_info.func_data;
		*rxv_dw_num = 36;  /* 9 cycle * 4 bytes = 36, 9 cycles */

		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: rxv_cnt = %d, rxv_dw_num = %d\n",
				__func__, *rxv_cnt, *rxv_dw_num));
	} else {
		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: get rxv count fail ret = %d\n",
				__func__, ret));

		return ret;
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_rxv_content(
	struct test_wlan_info *winfos,
	u_int32 dw_cnt,
	u_int32 *content)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 i, buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	for (i = 0; i < dw_cnt; i += 4) {
		rf_at_info.func_idx = RF_AT_FUNCID_RXV_DUMP;
		rf_at_info.func_data = i;
		ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

		if (ret == SERV_STATUS_SUCCESS) {
			*content = rf_at_info.func_data;
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: content[%d] = %x\n",
					__func__, i, *content));
		} else {
			SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
				("%s: get rxv data fail ret = %d\n",
					__func__, ret));

			return ret;
		}
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_cal_bypass(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int32 cal_item)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_dpd(
	struct test_wlan_info *winfos,
	u_int32 on_off,
	u_int32 wf_sel)
{
	return SERV_STATUS_SUCCESS;
}

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)
s_int32 mt_op_set_max_pac_ext(
	struct test_wlan_info *winfos,
	u_int32 max_pac_ext)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_MAX_PE, max_pac_ext);

	return ret;
}
#endif

s_int32 mt_op_set_tssi(
	struct test_wlan_info *winfos,
	u_int32 on_off,
	u_int32 wf_sel)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_thermal_val(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx,
	u_int32 *value)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_mtk_wifi_test_struct rf_at_info;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	u_int32 buf_len = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rf_at_info.func_idx = RF_AT_FUNCID_TEMP_SENSOR;
	rf_at_info.func_data = 0;

	ret = tm_rftest_query_auto_test(winfos,
		&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS) {
		*value = rf_at_info.func_data;
		*value = *value >> 16;
		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: value = %x\n",
				__func__, *value));
	} else {
		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_TRACE,
			("%s: get thermal fail ret = %d\n",
				__func__, ret));

		return ret;
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_rdd_test(
	struct test_wlan_info *winfos,
	u_int32 rdd_idx,
	u_int32 rdd_sel,
	u_int32 enable)
{
	struct test_rdd_params rdd_info;
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	rdd_info.rdd_idx = rdd_idx;
	rdd_info.rdd_sel = rdd_sel;


	if (enable) {
		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_SET_TEST_RDD_START,
			&rdd_info,
			sizeof(rdd_info),
			NULL,
			NULL);
	} else {
		ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
			OP_WLAN_OID_SET_TEST_RDD_STOP,
			&rdd_info,
			sizeof(rdd_info),
			NULL,
			NULL);
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_off_ch_scan(
	struct test_wlan_info *winfos,
	struct test_configuration *configs,
	u_char band_idx,
	struct test_off_ch_param *param)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_rdd_cnt(
	struct test_wlan_info *winfos,
	u_int32 *rdd_cnt,
	u_int32 *rdd_dw_num)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct test_rdd_dump_params rdd_info;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_RDD_CNT,
		&rdd_info,
		sizeof(rdd_info),
		NULL,
		NULL);
	SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("%s : %d, %d\n", __func__,
		rdd_info.rdd_cnt, rdd_info.rdd_dw_num));
	if (ret == SERV_STATUS_SUCCESS) {
		SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("%s : %d, %d\n", __func__,
		rdd_info.rdd_cnt, rdd_info.rdd_dw_num));
		*rdd_cnt = rdd_info.rdd_cnt;
		*rdd_dw_num = rdd_info.rdd_dw_num;
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_rdd_content(
	struct test_wlan_info *winfos,
	u_int32 *content,
	u_int32 *total_cnt)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct test_rdd_log *result = NULL;
	s_int32 *pulse = NULL;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;
	struct test_log_dump_cb rdd_info;
	int32_t idx = 0;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_GET_RDD_CONTENT,
		&rdd_info,
		sizeof(rdd_info),
		NULL,
		NULL);

	if (ret == SERV_STATUS_SUCCESS) {

		SERV_LOG(SERV_DBG_CAT_MISC, SERV_DBG_LVL_WARN,
		("%s : %d, %d, %d, %d\n", __func__,
		idx, rdd_info.len, rdd_info.idx, rdd_info.entry[0].un_dumped));

		do {
			idx = (idx % (rdd_info.len));
			/* 1 pulse: 64 bits */
			result = &rdd_info.entry[idx].rdd;
			pulse = (s_int32 *)result->buffer;

			rdd_info.entry[idx].un_dumped = FALSE;

			*content = pulse[0];
			content++;
			*content = pulse[1];
			content++;
			*total_cnt = *total_cnt + 2;

			INC_RING_INDEX1(idx, rdd_info.len);
		} while (idx != rdd_info.idx);

		SERV_LOG(SERV_DBG_CAT_TEST, SERV_DBG_LVL_ERROR,
		("[After RDD dumping] idx: %d, end: %d, total_cnt: %d\n",
		idx, rdd_info.len, *total_cnt));
	}

	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_tam_arb(
	struct test_wlan_info *winfos,
	u_int8 arb_op_mode)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_set_muru_manual(
	void *virtual_device,
	struct test_wlan_info *winfos,
	struct test_configuration *configs)
{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_hetb_ctrl(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_char ctrl_type,
	u_char enable,
	u_int8 bw,
	u_int8 ltf_gi,
	u_int8 stbc,
	u_int8 pri_ru_idx,
	struct test_ru_info *ru_info)
{
	return SERV_STATUS_SUCCESS;
}


s_int32 mt_op_get_rx_stat_band(
	struct test_wlan_info *winfos,
	u_int8 band_idx,
	u_int8 blk_idx,
	struct test_rx_stat_band_info *rx_st_band,
	u_int32 precise_rssi)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	struct param_custom_access_rx_stat rx_stat_test;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)

	rx_stat_test.seq_num = 0;
	rx_stat_test.total_num = sizeof(test_hqa_rx_stat);
	rx_stat_test.band_idx = band_idx;
	/* connac3 version(2) */
	/*BIT(4) = precise_rssi*/
	rx_stat_test.data = 2;
	if (precise_rssi == 1)
		rx_stat_test.data |= BIT(4);

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		 OP_WLAN_OID_QUERY_RX_STATISTICS,
		 &rx_stat_test,
		 sizeof(rx_stat_test),
		 NULL,
		 &test_hqa_rx_stat);

	rx_st_band->mac_rx_fcs_err_cnt =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4MacRxFcsErrCnt);
	rx_st_band->mac_rx_mdrdy_cnt =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4MacRxMdrdyCnt);
	rx_st_band->mac_rx_len_mismatch =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4MacRxLenMisMatch);
	rx_st_band->mac_rx_fcs_ok_cnt =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4MacRxFcsOkCnt);
	rx_st_band->phy_rx_fcs_err_cnt_cck =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxFcsErrCntCck);
	rx_st_band->phy_rx_fcs_err_cnt_ofdm =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxFcsErrCntOfdm);
	rx_st_band->phy_rx_pd_cck =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxPdCck);
	rx_st_band->phy_rx_pd_ofdm =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxPdOfdm);
	rx_st_band->phy_rx_sig_err_cck =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxSigErrCck);
	rx_st_band->phy_rx_sfd_err_cck =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxSfdErrCck);
	rx_st_band->phy_rx_sig_err_ofdm =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxSigErrOfdm);
	rx_st_band->phy_rx_tag_err_ofdm =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxTagErrOfdm);
	rx_st_band->phy_rx_mdrdy_cnt_cck =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxMdrdyCntCck);
	rx_st_band->phy_rx_mdrdy_cnt_ofdm =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBand[band_idx].u4PhyRxMdrdyCntOfdm);
	rx_st_band->phy_rx_pd_alr =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBandExt1[band_idx].u4PhyRxPdAlr);
	rx_st_band->mac_rx_u2m_mpdu_cnt =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoBandExt1[band_idx].u4RxU2MMpduCnt);

#else

	rx_stat_test.seq_num = 0;
	rx_stat_test.total_num = 72;

	tm_rftest_set_auto_test(winfos,
		RF_AT_FUNCID_SET_DBDC_BAND_IDX, band_idx);

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		 OP_WLAN_OID_QUERY_RX_STATISTICS,
		 &rx_stat_test,
		 sizeof(rx_stat_test),
		 NULL,
		 &test_hqa_rx_stat);

	if (band_idx == M_BAND_0) {
		rx_st_band->mac_rx_fcs_err_cnt =
			SERV_OS_NTOHL(test_hqa_rx_stat.mac_rx_fcs_err_cnt);
		rx_st_band->mac_rx_mdrdy_cnt =
			SERV_OS_NTOHL(test_hqa_rx_stat.mac_rx_mdrdy_cnt);
		rx_st_band->mac_rx_len_mismatch =
			SERV_OS_NTOHL(test_hqa_rx_stat.mac_rx_len_mismatch);
		rx_st_band->mac_rx_fcs_ok_cnt =	rx_st_band->mac_rx_mdrdy_cnt -
			rx_st_band->mac_rx_fcs_err_cnt;
		rx_st_band->phy_rx_fcs_err_cnt_cck =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_fcs_err_cnt_cck);
		rx_st_band->phy_rx_fcs_err_cnt_ofdm =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_fcs_err_cnt_ofdm);
		rx_st_band->phy_rx_pd_cck =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_pd_cck);
		rx_st_band->phy_rx_pd_ofdm =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_pd_ofdm);
		rx_st_band->phy_rx_sig_err_cck =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_sig_err_cck);
		rx_st_band->phy_rx_sfd_err_cck =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_sfd_err_cck);
		rx_st_band->phy_rx_sig_err_ofdm =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_sig_err_ofdm);
		rx_st_band->phy_rx_tag_err_ofdm =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_tag_err_ofdm);
		rx_st_band->phy_rx_mdrdy_cnt_cck =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_mdrdy_cnt_cck);
		rx_st_band->phy_rx_mdrdy_cnt_ofdm =
			SERV_OS_NTOHL(test_hqa_rx_stat.phy_rx_mdrdy_cnt_ofdm);
	} else {
		rx_st_band->mac_rx_fcs_err_cnt =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.mac_rx_fcs_err_cnt_band1);
		rx_st_band->mac_rx_mdrdy_cnt =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.mac_rx_mdrdy_cnt_band1);
		rx_st_band->mac_rx_len_mismatch =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.mac_rx_len_mismatch_band1);
		rx_st_band->mac_rx_fcs_ok_cnt = rx_st_band->mac_rx_mdrdy_cnt -
			rx_st_band->mac_rx_fcs_err_cnt;
		rx_st_band->phy_rx_fcs_err_cnt_cck =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_fcs_err_cnt_cck_band1);
		rx_st_band->phy_rx_fcs_err_cnt_ofdm =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_fcs_err_cnt_ofdm_band1);
		rx_st_band->phy_rx_pd_cck =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_pd_cck_band1);
		rx_st_band->phy_rx_pd_ofdm =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_pd_ofdm_band1);
		rx_st_band->phy_rx_sig_err_cck =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_sig_err_cck_band1);
		rx_st_band->phy_rx_sfd_err_cck =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_sfd_err_cck_band1);
		rx_st_band->phy_rx_sig_err_ofdm =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_sig_err_ofdm_band1);
		rx_st_band->phy_rx_tag_err_ofdm =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_tag_err_ofdm_band1);
		rx_st_band->phy_rx_mdrdy_cnt_cck =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_mdrdy_cnt_cck_band1);
		rx_st_band->phy_rx_mdrdy_cnt_ofdm =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.phy_rx_mdrdy_cnt_ofdm_band1);
	}
#endif /* (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1) */

	return ret;
}

s_int32 mt_op_set_mutb_spe(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_char tx_mode,
	u_int8 spe_idx)
{
	return SERV_STATUS_SUCCESS;
}


s_int32 mt_op_get_rx_stat_path(
	struct test_wlan_info *winfos,
	u_int8 band_idx,
	u_int8 blk_idx,
	struct test_rx_stat_path_info *rx_st_path)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	switch (blk_idx) {
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	case ANT_WF0:
		rx_st_path->rcpi =
			SERV_OS_NTOHL(test_hqa_rx_stat.rcpi0);
		rx_st_path->rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.rssi0);
		rx_st_path->fagc_ib_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.fagc_ib_RSSSI0);
		rx_st_path->fagc_wb_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.fagc_wb_RSSSI0);
		rx_st_path->inst_ib_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.inst_ib_RSSSI0);
		rx_st_path->inst_wb_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.inst_wb_RSSSI0);
		break;
	case ANT_WF1:
		rx_st_path->rcpi =
			SERV_OS_NTOHL(test_hqa_rx_stat.rcpi1);
		rx_st_path->rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.rssi1);
		rx_st_path->fagc_ib_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.fagc_ib_RSSSI1);
		rx_st_path->fagc_wb_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.fagc_wb_RSSSI1);
		rx_st_path->inst_ib_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.inst_ib_RSSSI1);
		rx_st_path->inst_wb_rssi =
			SERV_OS_NTOHL(test_hqa_rx_stat.inst_wb_RSSSI1);
		break;
#else

	case ANT_WF0:
		rx_st_path->rcpi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[0].u4Rcpi);
		rx_st_path->rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[0].u4Rssi);
		rx_st_path->fagc_ib_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoFagc[0].u4RssiIb);
		rx_st_path->fagc_wb_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoFagc[0].u4RssiWb);
		rx_st_path->inst_ib_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[0].u4RssiIb);
		rx_st_path->inst_wb_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[0].u4RssiWb);
		rx_st_path->adc_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[0].u4AdcRssi);
		rx_st_path->cca_idle_pwr =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[0].u4CcaIdlePwr);
		break;
	case ANT_WF1:
		rx_st_path->rcpi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[1].u4Rcpi);
		rx_st_path->rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[1].u4Rssi);
		rx_st_path->fagc_ib_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoFagc[1].u4RssiIb);
		rx_st_path->fagc_wb_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoFagc[1].u4RssiWb);
		rx_st_path->inst_ib_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[1].u4RssiIb);
		rx_st_path->inst_wb_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[1].u4RssiWb);
		rx_st_path->adc_rssi =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoRXV[1].u4AdcRssi);
		rx_st_path->cca_idle_pwr =
			SERV_OS_NTOHL(
			test_hqa_rx_stat.rInfoInst[1].u4CcaIdlePwr);
		break;
#endif
	default:
		ret = SERV_STATUS_HAL_OP_FAIL;
		break;
	}

	return ret;
}

s_int32 mt_op_set_ru_aid(
	struct test_wlan_info *winfos,
	u_char band_idx,
	u_int16 mu_rx_aid)

{
	return SERV_STATUS_SUCCESS;
}

s_int32 mt_op_get_rx_stat_user(
	struct test_wlan_info *winfos,
	u_int8 band_idx,
	u_int8 blk_idx,
	struct test_rx_stat_user_info *rx_st_user)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	rx_st_user->freq_offset_from_rx =
		SERV_OS_NTOHL(test_hqa_rx_stat.freq_offset_from_rx);

	rx_st_user->fcs_error_cnt =
		SERV_OS_NTOHL(test_hqa_rx_stat.mac_rx_fcs_err_cnt);

	if (band_idx == M_BAND_0)
		rx_st_user->snr = SERV_OS_NTOHL(test_hqa_rx_stat.snr0);
	else
		rx_st_user->snr = SERV_OS_NTOHL(test_hqa_rx_stat.snr1);
#else
	rx_st_user->freq_offset_from_rx =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoUser[0].u4FreqOffsetFromRx);

	rx_st_user->fcs_error_cnt =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoUser[0].u4FcsErrorCnt);

	rx_st_user->snr =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoUser[0].u4Snr);
#endif /* (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0) */

	return ret;
}

s_int32 mt_op_get_rx_stat_comm(
	struct test_wlan_info *winfos,
	u_int8 band_idx,
	u_int8 blk_idx,
	struct test_rx_stat_comm_info *rx_st_comm)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

#if (CFG_SUPPORT_CONNAC3X == 0) && (CFG_SUPPORT_CONNAC5X == 0)
	rx_st_comm->rx_fifo_full =
		SERV_OS_NTOHL(test_hqa_rx_stat.rx_fifo_full);
	rx_st_comm->aci_hit_low =
		SERV_OS_NTOHL(test_hqa_rx_stat.aci_hit_low);
	rx_st_comm->aci_hit_high =
		SERV_OS_NTOHL(test_hqa_rx_stat.aci_hit_high);
	rx_st_comm->mu_pkt_count =
		SERV_OS_NTOHL(test_hqa_rx_stat.mu_pkt_count);
	rx_st_comm->sig_mcs =
		SERV_OS_NTOHL(test_hqa_rx_stat.sig_mcs);
	rx_st_comm->sinr =
		SERV_OS_NTOHL(test_hqa_rx_stat.sinr);

	if (band_idx == M_BAND_0) {
		rx_st_comm->driver_rx_count =
		SERV_OS_NTOHL(test_hqa_rx_stat.driver_rx_count);
	} else {
		rx_st_comm->driver_rx_count =
		SERV_OS_NTOHL(test_hqa_rx_stat.driver_rx_count1);
	}

#else
	rx_st_comm->rx_fifo_full =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoComm[band_idx].u4MacRxFifoFull);
	rx_st_comm->mu_pkt_count =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoCommExt1[band_idx].u4MuRxCnt);
	rx_st_comm->sig_mcs =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoCommExt1[band_idx].u4EhtSigMcs);
	rx_st_comm->sinr =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoCommExt1[band_idx].u4Sinr);
	rx_st_comm->ne_var_db =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoUserExt1[0].u4NeVarDbAllUser);
	rx_st_comm->driver_rx_count =
		SERV_OS_NTOHL(
		test_hqa_rx_stat.rInfoCommExt1[band_idx].u4DrvRxCnt);
#endif
	return ret;
}

s_int32 mt_op_get_wf_path_comb(
	struct test_wlan_info *winfos,
	u_int8 band_idx,
	boolean dbdc_mode_en,
	u_int8 *path,
	u_int8 *path_len)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	u_int8 i = 0;

	/* sanity check for null pointer */
	if (!path)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	/* sanity check for null pointer */
	if (!path_len)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

#if (CFG_SUPPORT_CONNAC3X == 1) || (CFG_SUPPORT_CONNAC5X == 1)

	*path_len = 2;
	for (i = 0; i < *path_len; i++)
		*(path + i) = i;

#else

	if (dbdc_mode_en) {
		*path_len = 1;
		*path = 0;
	} else {
		*path_len = 2;
		for (i = 0; i < *path_len; i++)
			*(path + i) = i;
	}

	if (*path_len > MAX_ANT_NUM)
		ret = FALSE;

#endif /*(CFG_SUPPORT_CONNAC3X == 1)*/

	return ret;
}

s_int32 mt_op_listmode_cmd(
	struct test_wlan_info *winfos,
	u_int8 *para,
	u_int16 para_len,
	u_int32 *rsp_len,
	void *rsp_data)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	wlan_oid_handler_t pr_oid_funcptr = winfos->oid_funcptr;

	if (pr_oid_funcptr == NULL)
		return SERV_STATUS_HAL_OP_INVALID_NULL_POINTER;

	ret = pr_oid_funcptr(winfos, /*call back to ServiceWlanOid*/
		OP_WLAN_OID_LIST_MODE,
		(void *)para,
		(u_int32)para_len,
		rsp_len,
		rsp_data);

	return ret;
}

s_int32 mt_op_set_efem_mode(
	struct test_wlan_info *winfos,
	u_int32 band_idx,
	u_int32 ch_band,
	u_int32 wf_path,
	u_int32 enable,
	u_int32 mode,
	u_int32 level)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX,
			band_idx);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_BAND,
			ch_band);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_PATH,
			wf_path);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_ENABLE,
			enable);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_VALUE,
			level);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_EFEM_MODE,
			mode);

	return ret;
}

s_int32 mt_op_set_tx_gain(
	struct test_wlan_info *winfos,
	u_int32 band_idx,
	u_int32 ch_band,
	u_int32 wf_path,
	u_int32 enable,
	u_int32 gain_type,
	u_int32 value)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX,
			band_idx);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_PATH,
			wf_path);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_ENABLE,
			enable);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_VALUE,
			value);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_GAIN,
			gain_type);

	return ret;
}

s_int32 mt_op_set_etssi_gain(
	struct test_wlan_info *winfos,
	u_int32 band_idx,
	u_int32 ch_band,
	u_int32 wf_path,
	u_int32 enable,
	u_int32 gain_value)
{
	s_int32 ret = SERV_STATUS_SUCCESS;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX,
			band_idx);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_PATH,
			wf_path);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_ENABLE,
			enable);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_GAIN_VALUE,
			gain_value);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_ETSSI_GAIN,
			gain_value);

	return ret;
}

s_int32 mt_op_get_tssi_meas_dbv(
	struct test_wlan_info *winfos,
	u_int32 band_idx,
	u_int32 wf_path,
	u_int32 *dbv_value)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	u_int32 buf_len = 0;
	struct param_mtk_wifi_test_struct rf_at_info;

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_DBDC_BAND_IDX,
			band_idx);

	ret = tm_rftest_set_auto_test(winfos,
			RF_AT_FUNCID_SET_TX_PATH,
			wf_path);

	rf_at_info.func_idx = RF_AT_FUNCID_GET_TSSI_MEAS_DBV;
	rf_at_info.func_data = 0;
	ret = tm_rftest_query_auto_test(winfos,
			&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS)
		*dbv_value = rf_at_info.func_data;
	else
		*dbv_value = 0;

	return ret;
}

s_int32 mt_op_get_sleep_check(
	struct test_wlan_info *winfos,
	u_int32 action,
	u_int32 *sleep_result)
{
	s_int32 ret = SERV_STATUS_SUCCESS;
	u_int32 buf_len = 0;
	struct param_mtk_wifi_test_struct rf_at_info;

	rf_at_info.func_idx = RF_AT_FUNCID_GET_SLEEP_CHECK;
	rf_at_info.func_data = action;
	ret = tm_rftest_query_auto_test(winfos,
			&rf_at_info, &buf_len);

	if (ret == SERV_STATUS_SUCCESS)
		*sleep_result = rf_at_info.func_data;
	else
		*sleep_result = 0;

	return ret;
}
