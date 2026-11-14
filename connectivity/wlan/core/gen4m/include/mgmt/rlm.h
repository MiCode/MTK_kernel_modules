/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "rlm.h"
 *    \brief
 */


#ifndef _RLM_H
#define _RLM_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
extern u_int8_t g_bIcapEnable;
extern u_int8_t g_bCaptureDone;
extern uint16_t g_u2DumpIndex;
#if CFG_SUPPORT_QA_TOOL
extern uint32_t g_au4Offset[2][2];
extern uint32_t g_au4IQData[256];
#endif

#if (CFG_SUPPORT_802_11AX == 1)
extern uint8_t  g_fgSigmaCMDHt;
extern uint8_t  g_ucHtSMPSCapValue;
#endif

static const char * const apucOpBw[MAX_BW_UNKNOWN+1] = {
	[MAX_BW_20MHZ] = "MAX_BW_20MHZ",
	[MAX_BW_40MHZ] = "MAX_BW_40MHZ",
	[MAX_BW_80MHZ] = "MAX_BW_80MHZ",
	[MAX_BW_160MHZ] = "MAX_BW_160MHZ",
	[MAX_BW_80_80_MHZ] = "MAX_BW_80_80_MHZ",
	[MAX_BW_320_1MHZ] = "MAX_BW_320_1MHZ",
	[MAX_BW_320_2MHZ] = "MAX_BW_320_2MHZ",
	[MAX_BW_UNKNOWN] = "MAX_BW_UNKNOWN",
};

static const char * const apucVhtOpBw[CW_NUM] = {
	[CW_20_40MHZ] = "CW_20_40MHZ",
	[CW_80MHZ] = "CW_80MHZ",
	[CW_160MHZ] = "CW_160MHZ",
	[CW_80P80MHZ] = "CW_80P80MHZ",
	[CW_320_1MHZ] = "CW_320_1MHZ",
	[CW_320_2MHZ] = "CW_320_2MHZ",
};

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define ELEM_EXT_CAP_DEFAULT_VAL \
	(ELEM_EXT_CAP_20_40_COEXIST_SUPPORT /*| ELEM_EXT_CAP_PSMP_CAP*/)

#if CFG_SUPPORT_RX_STBC
#define FIELD_HT_CAP_INFO_RX_STBC   HT_CAP_INFO_RX_STBC_1_SS
#else
#define FIELD_HT_CAP_INFO_RX_STBC   HT_CAP_INFO_RX_STBC_NO_SUPPORTED
#endif

#if CFG_SUPPORT_RX_SGI
#define FIELD_HT_CAP_INFO_SGI_20M   HT_CAP_INFO_SHORT_GI_20M
#define FIELD_HT_CAP_INFO_SGI_40M   HT_CAP_INFO_SHORT_GI_40M
#else
#define FIELD_HT_CAP_INFO_SGI_20M   0
#define FIELD_HT_CAP_INFO_SGI_40M   0
#endif

#if CFG_SUPPORT_RX_HT_GF
#define FIELD_HT_CAP_INFO_HT_GF     HT_CAP_INFO_HT_GF
#else
#define FIELD_HT_CAP_INFO_HT_GF     0
#endif

#define HT_CAP_INFO_DEFAULT_VAL \
	(HT_CAP_INFO_SUP_CHNL_WIDTH | HT_CAP_INFO_DSSS_CCK_IN_40M \
		| HT_CAP_INFO_SM_POWER_SAVE)

#if (CFG_SUPPORT_SMALL_PKT == 1)
#define AMPDU_PARAM_DEFAULT_VAL \
	(AMPDU_PARAM_MAX_AMPDU_LEN_64K | AMPDU_PARAM_MSS_1_US)
#else
#define AMPDU_PARAM_DEFAULT_VAL \
	(AMPDU_PARAM_MAX_AMPDU_LEN_64K | AMPDU_PARAM_MSS_NO_RESTRICIT)
#endif

#define SUP_MCS_TX_DEFAULT_VAL \
	SUP_MCS_TX_SET_DEFINED	/* TX defined and TX/RX equal (TBD) */

#if CFG_SUPPORT_MFB
#define FIELD_HT_EXT_CAP_MFB    HT_EXT_CAP_MCS_FEEDBACK_BOTH
#else
#define FIELD_HT_EXT_CAP_MFB    HT_EXT_CAP_MCS_FEEDBACK_NO_FB
#endif

#if CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_RDR    HT_EXT_CAP_RD_RESPONDER
#else
#define FIELD_HT_EXT_CAP_RDR    0
#endif

#if CFG_SUPPORT_MFB || CFG_SUPPORT_RX_RDG
#define FIELD_HT_EXT_CAP_HTC    HT_EXT_CAP_HTC_SUPPORT
#else
#define FIELD_HT_EXT_CAP_HTC    0
#endif

#define HT_EXT_CAP_DEFAULT_VAL \
	(HT_EXT_CAP_PCO | HT_EXT_CAP_PCO_TRANS_TIME_NONE | \
	 FIELD_HT_EXT_CAP_MFB | FIELD_HT_EXT_CAP_HTC | \
	 FIELD_HT_EXT_CAP_RDR)

#define TX_BEAMFORMING_CAP_DEFAULT_VAL        0

#if CFG_SUPPORT_BFEE
#define TX_BEAMFORMING_CAP_BFEE \
	(TXBF_RX_NDP_CAPABLE | \
	 TXBF_EXPLICIT_COMPRESSED_FEEDBACK_IMMEDIATE_CAPABLE | \
	 TXBF_MINIMAL_GROUPING_1_2_3_CAPABLE | \
	 TXBF_COMPRESSED_TX_ANTENNANUM_4_SUPPORTED | \
	 TXBF_CHANNEL_ESTIMATION_4STS_CAPABILITY)
#else
#define TX_BEAMFORMING_CAP_BFEE        0
#endif

#if CFG_SUPPORT_BFER
#define TX_BEAMFORMING_CAP_BFER \
	(TXBF_TX_NDP_CAPABLE | \
	 TXBF_EXPLICIT_COMPRESSED_TX_CAPAB)
#else
#define TX_BEAMFORMING_CAP_BFER        0
#endif

#define ASEL_CAP_DEFAULT_VAL                        0

/* Define bandwidth from user setting */
#define CONFIG_BW_20_40M            0
#define CONFIG_BW_20M               1	/* 20MHz only */

#define RLM_INVALID_POWER_LIMIT                     -127 /* dbm */

#define RLM_MAX_TX_PWR		20	/* dbm */
#define RLM_MIN_TX_PWR		8	/* dbm */

#if CFG_SUPPORT_BFER
#define MODE_HT 2
#define MODE_VHT 4
#if (CFG_SUPPORT_802_11AX == 1)
#define MODE_HE_SU 8
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#define MODE_EHT_SU 15
#endif

#endif

#if CFG_SUPPORT_802_11AC
#if CFG_SUPPORT_BFEE
#define FIELD_VHT_CAP_INFO_BFEE \
		(VHT_CAP_INFO_SU_BEAMFORMEE_CAPABLE)
#define VHT_CAP_INFO_BEAMFORMEE_STS_CAP_MAX	3
#else
#define FIELD_VHT_CAP_INFO_BFEE     0
#endif

#if CFG_SUPPORT_BFER
#define FIELD_VHT_CAP_INFO_BFER \
		(VHT_CAP_INFO_SU_BEAMFORMER_CAPABLE| \
		VHT_CAP_INFO_NUMBER_OF_SOUNDING_DIMENSIONS_2_SUPPORTED)
#else
#define FIELD_VHT_CAP_INFO_BFER     0
#endif

#define VHT_CAP_INFO_DEFAULT_VAL \
	(VHT_CAP_INFO_MAX_MPDU_LEN_3K | \
	 (AMPDU_PARAM_MAX_AMPDU_LEN_1024K \
		 << VHT_CAP_INFO_MAX_AMPDU_LENGTH_OFFSET))

#define VHT_CAP_INFO_DEFAULT_HIGHEST_DATA_RATE			0
#define VHT_CAP_INFO_EXT_NSS_BW_CAP				BIT(13)

#endif

#if (CFG_SUPPORT_FACT_CAL == 1)
#define FACT_CAL_CMD_EVENT_WAITTIME_MS (1000) /* uint: msec */
#define FACT_CAL_GET_TIMEOUT_TH (10) /* uint: sec */

/* Define for fact cal result to 4 byte-align */
#define FACT_CAL_COM_CAL_RESULT_LEN 600
#define FACT_CAL_GRP_CAL_RESULT_LEN  4500  // Reserved more than 4384 bytes
#define FACT_CAL_GRP_CAL_DBDC_RESULT_LEN 4000 // Reserved more than 3880 bytes
#define FACT_CAL_GRP_BW160_CAL_RESULT_LEN 1720
#define FACT_CAL_GRP_BW160_CAL_DBDC_RESULT_LEN 1472
#define FACT_CAL_CH_CAL_RESULT_LEN  700

#if CFG_FACT_CAL_DATA_BUF_NUM_MAX
 /* Maximum of channel FE path num */
#define FACT_CAL_DATA_BUF_NUM_MAX (CFG_FACT_CAL_DATA_BUF_NUM_MAX)
#else
#define FACT_CAL_DATA_BUF_NUM_MAX (0)
#endif

#define FACT_CAL_CH_NUM_2G (14)	/* ARRAY_SIZE g_au1ChList2G */
#define FACT_CAL_CH_NUM_5G (68) /* ARRAY_SIZE g_au1ChList5G */
#define FACT_CAL_CH_NUM_6G (109) /* ARRAY_SIZE g_au1ChList6G */
#define FACT_CAL_CH_NUM_ALL ((FACT_CAL_CH_NUM_2G) + (FACT_CAL_CH_NUM_5G)+ \
					+ (FACT_CAL_CH_NUM_6G))
#define FACT_CAL_CH_PATH_ALL \
	((FACT_CAL_CH_NUM_ALL)*(FACT_CAL_DATA_BUF_NUM_MAX))

#define FACT_CAL_DATA_BUF_CFG_U32_LEN (4)
#define FACT_CAL_DATA_BUF_CFG_U8_LEN (16)
/* buffer format [address, u4Length, others 1, others 2] = 4*4 = 16bytes */
/* Total BUF_NUM_MAX buf = 16 * BUF_NUM_MAX */
#define FACT_CAL_DATA_MAX_BUF_LEN \
	((FACT_CAL_DATA_BUF_NUM_MAX)*(FACT_CAL_DATA_BUF_CFG_U32_LEN))
#define FACT_CAL_DATA_BUF_LEN (1400)

#if CFG_FACT_CAL_BUF_LEN_COM
#define FACT_CAL_BUF_LEN_COM (CFG_FACT_CAL_BUF_LEN_COM)
#else
#define FACT_CAL_BUF_LEN_COM 0
#endif

#if CFG_FACT_CAL_BUF_LEN_GRP
/* Define Group Band0 & 1 total cal. size by project */
#define FACT_CAL_BUF_LEN_GRP (CFG_FACT_CAL_BUF_LEN_GRP)
#else
#define FACT_CAL_BUF_LEN_GRP 0
#endif

#if CFG_FACT_CAL_BUF_LEN_CH
/* Define channel total cal. size by project */
// Equal to Each channel cal. size * total FE paths
#define FACT_CAL_BUF_LEN_CH \
	(CFG_FACT_CAL_BUF_LEN_CH * FACT_CAL_DATA_BUF_NUM_MAX)
#else
#define FACT_CAL_BUF_LEN_CH 0
#endif

#if CFG_FACT_CAL_GROUP_NUM

#define FACT_CAL_GROUP_NUM (CFG_FACT_CAL_GROUP_NUM)
#else
#define FACT_CAL_GROUP_NUM 0
#endif

// Follow FW to use common as input param
#define FACT_CAL_PARAM_COMMON_MASK                        BITS(0, 7)

// Follow FW to use group as input param
#define FACT_CAL_PARAM_GROUP_MASK                         BITS(0, 15)
// Dbdc band mask used for both group & channel type
#define FACT_CAL_DBDC_BAND_MASK                           (BIT(16))
#define FACT_CAL_DBDC_BAND_SHFT                           (16)
#define FACT_CAL_EMLSR_MASK                               (BIT(17))
#define FACT_CAL_EMLSR_SHFT                               (17)
#define FACT_CAL_POWERON_GROUP_MASK                       (BIT(18))
#define FACT_CAL_POWERON_GROUP_SHFT                       (18)

// Use central channel as input param
#define FACT_CAL_CENT_CH_PARAM_CHAN_MASK                  BITS(0, 11)
#define FACT_CAL_CENT_CH_PARAM_CHAN_OFFSET                (0)
#define FACT_CAL_CENT_CH_PARAM_RF_BAND_MASK               BITS(12, 15)
#define FACT_CAL_CENT_CH_PARAM_RF_BAND_OFFSET             (12)
#define FACT_CAL_CENT_CH_PARAM_REF_GROUP_MASK             BITS(17, 22)
#define FACT_CAL_CENT_CH_PARAM_REF_GROUP_OFFSET           (17)

#define BAND_TO_FACT_BAND(_ucBand) ((_ucBand) - 1)
#define FACT_CAL_DATA_INVALID_IDX 0xFFFFFFFF

#endif //#if CFG_SUPPORT_FACT_CAL

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
#define TX_PWR_ENV_LMT_MIN                0 /* LSB = 0.5dBm */
#define TX_PWR_ENV_BW_SHIFT_BW20          0
#define TX_PWR_ENV_BW_SHIFT_BW40          2
#define TX_PWR_ENV_BW_SHIFT_BW80          6
#define TX_PWR_ENV_BW_SHIFT_BW160        14

/* PSD to Power dBm transfer func : 10*log(BW) * 2 */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW20    26 /* 10*log( 20) * 2  = 26, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW40    32 /* 10*log( 40) * 2  = 32, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW80    38 /* 10*log( 80) * 2  = 38, 0.5dBm */
#define TX_PWR_ENV_PSD_TRANS_DBM_BW160   44 /* 10*log(160) * 2  = 44, 0.5dBm */

#define TX_PWR_ENV_INT8_MIN             -128
#define TX_PWR_ENV_INT8_MAX              127
#endif

#define TX_PWR_REG_LMT_MIN   5  /* LSB = 1 dBm */
#define TX_PWR_REG_LMT_MAX  30  /* LSB = 1 dBm */
#define TX_PWR_MAX          63  /* LSB = 1 dBm */
#define TX_PWR_MIN         -64  /* LSB = 1 dBm */
/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

enum ENUM_OP_NOTIFY_STATE_T {
	OP_NOTIFY_STATE_KEEP = 0, /* Won't change OP mode */
	OP_NOTIFY_STATE_SENDING,  /* Sending OP notification frame */
	OP_NOTIFY_STATE_SUCCESS,  /* OP notification Tx success */
	OP_NOTIFY_STATE_FAIL,     /* OP notification Tx fail(over retry limit)*/
	OP_NOTIFY_STATE_ROLLBACK, /* OP notification rollback */
	OP_NOTIFY_STATE_NUM
};

#if (CFG_SUPPORT_FACT_CAL == 1)

enum FACT_CAL_ACTION {
	FACT_CAL_ACTION_GET = 0,
	FACT_CAL_ACTION_SET = 1,
	FACT_CAL_ACTION_TRIGGER = 2,
	FACT_CAL_ACTION_UPDATE_FLAG = 3,
	FACT_CAL_ACTION_DUMP = 4,
	FACT_CAL_ACTION_LOAD_FILE = 5,
	FACT_CAL_ACTION_SAVE_FILE = 6,
	FACT_CAL_ACTION_NUM
};

enum FACT_CAL_TYPE {
	FACT_CAL_TYPE_COMMON = 0,
	FACT_CAL_TYPE_GROUP = 1,
	FACT_CAL_TYPE_CHANNEL = 2,
	FACT_CAL_TYPE_ALL = 3,
	FACT_CAL_TYPE_FORCE = 4,
	FACT_CAL_TYPE_NUM
};

enum FACT_CAL_TYPE_CE {
	FACT_CAL_TYPE_POWERON = 0,
	FACT_CAL_TYPE_SETCHANNEL = 1,
	FACT_CAL_TYPE_BAND = 2,
	FACT_CAL_TYPE_GET_ALL = 3,
	FACT_CAL_TYPE_MAPPING_TBL = 4
};

enum FACT_CAL_COMMON_BAND {
	FACT_CAL_COMMON_BAND_G = 0,
	FACT_CAL_COMMON_BAND_A = 1,
	FACT_CAL_COMMON_BAND_NUM
};

/*
 * Mapping to halPhyFactCal()
 * ENUM_BAND: NULL=0, 2G=1, 5G=2, 6G=3
 * FACT_CAL_BAND: 2G=0, 5G=1, 6G=2
 */
enum FACT_CAL_BAND {
	FACT_CAL_BAND_2G = 0,
	FACT_CAL_BAND_5G = 1,
#if (CFG_SUPPORT_WIFI_6G == 1)
	FACT_CAL_BAND_6G = 2,
#endif
	FACT_CAL_BAND_NUM
};

struct FACT_CAL_GROUP_DEF_ENTRY {
	uint8_t ucGroupIdx;
	uint16_t ucFreqStart;
	uint16_t ucFreqEnd;
};

struct FACT_CAL_BUF_INFO {
	/* Caltype : enum FACT_CAL_TYPE */
	uint32_t ucCalType;

	/* Input data for halPhyFactCal()
	 * If Cal type == Common
	 *	 Bit[0:7]: (2G : 0 / 5G : 1 / 6G : 2)
	 *	 Bit[8:31]: reserved for future use
	 * If Cal type == Group
	 *	 Bit[0:15]: group id (0~34)
	 *	 Bit[16]: Dbdc band (0: Band0, 1: Band1)
	 *	 Bit[17]: EMLSR group (0: No, 1: Yes)
	 *	 Bit[18]: PowerOn group (0: No, 1: Yes)
	 *	 Bit[18:31]: reserved for future use
	 * If Cal type == Channel
	 *	 Bit[0:11]: central channel
	 *	   E.g. CH6 set to 6
	 *	 Bit[12:15]: Channel Band (2G : 0 / 5G : 1 / 6G : 2)
	 *	 Bit[16]: Dbdc band (0: Band0, 1: Band1)
	 *	 Bit[17:22]: Reference Group
	 *	 Bit[23:31]: reserved for future use
	 */
	uint32_t u4CalParam;

	/*
	 * u4BufSeqNum
	 * bit[0:31]: Current buf seq in this type cal
	 */
	uint32_t u4BufSeqNum;

	/*
	 * u4TotalBufNum
	 * bit[0:31]: Total buf number in this type cal
	 */
	uint32_t u4TotalBufNum;

	/*
	 * u4BufDataLength
	 * bit[0:31] : len of cal data length
	 */
	uint32_t u4BufDataLength;

	/*
	 * fgValid
	 * Valid or not : (1: valid, 0: invalid)
	 */
	uint8_t fgValid;

	/* Done or not */
	uint8_t ucDone;

	/* each memory buf set is consisted of 4 uint32 of
	 * [address, u4Length, others 1, others 2]
	 */
	uint32_t au4BufCfgInfo[FACT_CAL_DATA_MAX_BUF_LEN];
};


/* Unit of CMD/EVENT cal data */
struct FACT_CAL_DATA_BUF {
	uint8_t fgValid;
	uint8_t ucCalType;
	uint32_t u4CalParam;
	uint32_t u4BufNum;
	uint32_t u4BufSeqNum;
	uint8_t *pBuf;
	uint32_t u4BufLen;
	uint32_t *pBufCfg;
	uint32_t u4BufCfgLen;
};

/* Uint of cal data saved in host mem */
struct FACT_CAL_COM {
	struct FACT_CAL_BUF_INFO rFactCalBufInfo;
	uint8_t aucCalData[FACT_CAL_BUF_LEN_COM];
};

/* Uint of cal data saved in host mem */
struct FACT_CAL_GRP {
	struct FACT_CAL_BUF_INFO rFactCalBufInfo;
	uint8_t aucCalData[FACT_CAL_BUF_LEN_GRP];
};

/* Uint of cal data saved in host mem */
struct FACT_CAL_CH {
	struct FACT_CAL_BUF_INFO rFactCalBufInfo;
	uint8_t aucCalData[FACT_CAL_BUF_LEN_CH];
};

enum FACT_CAL_STORE_ACTION {
	FACT_CAL_STORE_DATA_HEAD = 0,
	FACT_CAL_STORE_DATA = 1,
	FACT_CAL_STORE_DATA_DONE = 2,
	FACT_CAL_STORE_ACTION_NUM
};

struct FACT_CAL_COMMON_LOOKUP_TABLE {
    /* Common Cal for each A band and G band */
	struct FACT_CAL_COM rComCalData[FACT_CAL_COMMON_BAND_NUM];
};
struct FACT_CAL_GROUP_LOOKUP_TABLE {
    /* 2G : Group0 with 2 SX paths */
	/* 5G : Group1 ~ 8 with 2 SX paths */
	/* 6G : Group9 ~ 23 with 2 SX paths */
	/* 5G 160M: Group 24 ~ 27 with 2SX paths */
	/* 6G 160M: Group 28 ~ 34 with 2SX paths */
	struct FACT_CAL_GRP rGrpCalData[FACT_CAL_GROUP_NUM];
};
struct FACT_CAL_CHANNEL_LOOKUP_TABLE {
    /* Channel Cal for 2G, 5G, 6G channels */
	struct FACT_CAL_CH rChCalData[FACT_CAL_CH_NUM_ALL];
};
struct FACT_CAL_BASE_LOOKUP_TABLE {
	struct FACT_CAL_COMMON_LOOKUP_TABLE *common_t;
	struct FACT_CAL_GROUP_LOOKUP_TABLE *group_t;
	struct FACT_CAL_CHANNEL_LOOKUP_TABLE *channel_t;
#if (CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL == 1)
	dma_addr_t Group_pa;
	dma_addr_t Common_pa;
	dma_addr_t Channel_pa;
#endif /* CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL */
};

#if (CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL == 1)
#define GROUP_1 1  // Aband Group Start
#define GROUP_24 24 // Aband BW160 Group Start

struct FACT_CAL_COMMON_MAPPING_TABLE {
	uint32_t u4PhyAddr_H;
	uint32_t u4PhyAddr_L;
	uint32_t u4Offset;
	uint8_t ucBand[FACT_CAL_COMMON_BAND_NUM];
	uint8_t aucReserved[2];
};
struct FACT_CAL_GROUP_MAPPING_TABLE {
	uint32_t u4PhyAddr_H;
	uint32_t u4PhyAddr_L;
	uint32_t u4Offset;
	uint8_t u1Group[FACT_CAL_GROUP_NUM];
	uint8_t aucReserved;
};

/*
 * u4CacheMark made 32-bits mark, in format:
 * BIT[31]:     channel is bw160 or not
 * BITS[30:28]: cal path, from input ucCalPath
 * BITS[27:24]: antenna position, from input ucAntPos
 * BITS[23:00]: channel, from input u4DpdCalCh
 */
struct FACT_CAL_CHANNEL_MAPPING_TABLE {
	uint32_t u4PhyAddr_H;
	uint32_t u4PhyAddr_L;
	uint32_t u4Offset;
	uint32_t u4CenterCh[FACT_CAL_CH_NUM_ALL];
	uint32_t u4TotalBufNum[FACT_CAL_CH_NUM_ALL];
	uint32_t u4CacheMark[FACT_CAL_CH_PATH_ALL];
	int8_t cTemperature[FACT_CAL_CH_PATH_ALL];
};

struct FACT_CAL_MAPPING_TABLE {
	struct FACT_CAL_COMMON_MAPPING_TABLE CommMap_t;
	struct FACT_CAL_GROUP_MAPPING_TABLE GrpMap_t;
	struct FACT_CAL_CHANNEL_MAPPING_TABLE ChMap_t;
	uint32_t u4ComLen;
	uint32_t u4GrpALen;
	uint32_t u4GrpADbdcLen;
	uint32_t u4GrpABW160Len;
	uint32_t u4GrpABW160DbdcLen;
	uint32_t u4ChLen;
};
#endif /* CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL */

#endif /* CFG_SUPPORT_FACT_CAL */

typedef void (*PFN_OPMODE_NOTIFY_DONE_FUNC)(
	struct ADAPTER *, uint8_t, bool);

enum ENUM_OP_NOTIFY_TYPE_T {
	OP_NOTIFY_TYPE_VHT_NSS_BW = 0,
	OP_NOTIFY_TYPE_HT_NSS,
	OP_NOTIFY_TYPE_HT_BW,
	OP_NOTIFY_TYPE_OMI_NSS_BW,
	OP_NOTIFY_TYPE_NUM
};

enum ENUM_OP_CHANGE_STATUS_T {
	OP_CHANGE_STATUS_INVALID = 0, /* input invalid */
	/* input valid, but no need to change */
	OP_CHANGE_STATUS_VALID_NO_CHANGE,
	/* process callback done before function return */
	OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_DONE,
	/* wait next INT to call callback */
	OP_CHANGE_STATUS_VALID_CHANGE_CALLBACK_WAIT,
	OP_CHANGE_STATUS_NUM
};

enum ENUM_OP_CHANGE_SEND_ACT_T {
	/* Do not send action frame */
	OP_CHANGE_SEND_ACT_DISABLE = 0,
	/* Send action frame if change */
	OP_CHANGE_SEND_ACT_DEFAULT = 1,
	/* Send action frame w/wo change */
	OP_CHANGE_SEND_ACT_FORCE = 2,
	OP_CHANGE_SEND_ACT_NUM
};

struct SUB_ELEMENT_LIST {
	struct SUB_ELEMENT_LIST *prNext;
	struct SUB_ELEMENT rSubIE;
};

#if CFG_SUPPORT_DFS
enum ENUM_CHNL_SWITCH_MODE {
	MODE_ALLOW_TX,
	MODE_DISALLOW_TX,
	MODE_NUM
};

struct SWITCH_CH_AND_BAND_PARAMS {
	enum ENUM_BAND eCsaBand;
	uint8_t ucCsaNewCh;
	uint8_t ucCsaCount;
	uint8_t ucVhtS1;
	uint8_t ucVhtS2;
	uint8_t ucVhtBw;
	enum ENUM_CHNL_EXT eSco;
	uint8_t ucBssIndex;
	uint8_t fgHasStopTx;
	uint8_t fgIsCrossBand;
	enum ENUM_CHNL_SWITCH_MODE ucCsaMode;
	uint32_t u4MaxSwitchTime;
};
#endif /* CFG_SUPPORT_DFS */

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

/* It is used for RLM module to judge if specific network is valid
 * Note: Ad-hoc mode of AIS is not included now. (TBD)
 */
#define RLM_NET_PARAM_VALID(_prBssInfo) \
	(IS_BSS_ACTIVE(_prBssInfo) && \
	 ((_prBssInfo)->eConnectionState == MEDIA_STATE_CONNECTED || \
	  (_prBssInfo)->eCurrentOPMode == OP_MODE_ACCESS_POINT || \
	  IS_BSS_BOW(_prBssInfo)) \
	)

#define RLM_NET_IS_11N(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11N)
#define RLM_NET_IS_11GN(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11GN)

#if CFG_SUPPORT_802_11AC
#define RLM_NET_IS_11AC(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11AC)
#endif
#if (CFG_SUPPORT_802_11AX == 1)
#define RLM_NET_IS_11AX(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11AX)
#endif
#if (CFG_SUPPORT_802_11BE == 1)
#define RLM_NET_IS_11BE(_prBssInfo) \
	((_prBssInfo)->ucPhyTypeSet & PHY_TYPE_SET_802_11BE)
#endif

#if CFG_SUPPORT_DFS
#define MAX_CSA_COUNT 255
#define HAS_CH_SWITCH_PARAMS(prCSAParams) (prCSAParams->ucCsaNewCh > 0)
#define HAS_SCO_PARAMS(prCSAParams) (prCSAParams->eSco > 0)
#define HAS_WIDE_BAND_PARAMS(prCSAParams) \
	(prCSAParams->ucVhtBw > 0 || \
	 prCSAParams->ucVhtS1 > 0 || \
	 prCSAParams->ucVhtS2 > 0)
#define SHOULD_CH_SWITCH(current, prCSAParams) \
	(HAS_CH_SWITCH_PARAMS(prCSAParams) && \
	 (current < prCSAParams->ucCsaCount))
#endif

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

void rlmFsmEventInit(struct ADAPTER *prAdapter);

void rlmFsmEventUninit(struct ADAPTER *prAdapter);

void rlmReqGenerateHtCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGeneratePowerCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGenerateSupportedChIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmReqGenerateExtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateHtCapIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateExtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateHtOpIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo);

void rlmGeneratePwrConstraintIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateErpIE(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo);

uint8_t rlmCheckMtkOuiChipCap(uint8_t *pucIe, uint64_t u8ChipCap);

uint32_t rlmCalculateMTKOuiIELen(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void rlmGenerateMTKOuiIE(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo);
uint16_t rlmGenerateMTKChipCapIE(uint8_t *pucBuf, uint16_t u2FrameLength,
	uint8_t fgNeedOui, uint64_t u8ChipCap);

uint32_t rlmCalculateWFACapLen(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void rlmGenerateWFACapIE(struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void rlmProcessWfaCapFrame(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec, bool fgUseMFP);

u_int8_t rlmParseCheckMTKOuiIE(struct ADAPTER *prAdapter,
			const uint8_t *pucBuf,  struct STA_RECORD *prStaRec);

#if CFG_SUPPORT_RXSMM_ALLOWLIST
u_int8_t rlmParseCheckRxsmmOuiIE(struct ADAPTER *prAdapter,
		const uint8_t *pucBuf, u_int8_t *pfgRxsmmEnable);
#endif
#if CFG_ENABLE_WIFI_DIRECT
uint32_t rlmCalculateCsaIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec);

void rlmGenerateCsaIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);
#endif
void rlmProcessBcn(struct ADAPTER *prAdapter,
		   struct SW_RFB *prSwRfb, uint8_t *pucIE,
		   uint16_t u2IELength);

void rlmProcessAssocRsp(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb, const uint8_t *pucIE,
			uint16_t u2IELength);

void rlmProcessHtAction(struct ADAPTER *prAdapter,
			struct SW_RFB *prSwRfb);

#if CFG_SUPPORT_NAN
uint32_t rlmFillNANVHTCapIE(struct ADAPTER *prAdapter,
			   struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);
uint32_t rlmFillNANHTCapIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);
#endif

#if CFG_SUPPORT_802_11AC
void rlmProcessVhtAction(struct ADAPTER *prAdapter,
			 struct SW_RFB *prSwRfb);
#endif

void rlmFillSyncCmdParam(struct CMD_SET_BSS_RLM_PARAM
			 *prCmdBody, struct BSS_INFO *prBssInfo);

void rlmSyncOperationParams(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo);

void rlmSyncAntCtrl(struct ADAPTER *prAdapter, uint8_t txNss, uint8_t rxNss);

void rlmSyncSapAntCtrl(struct ADAPTER *prAdapter,
			    uint8_t ucNss, uint8_t ucBssIndex);

void rlmBssInitForAPandIbss(struct ADAPTER *prAdapter,
			    struct BSS_INFO *prBssInfo);

void rlmProcessAssocReq(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec,
			uint8_t *pucIE, uint16_t u2IELength);

void rlmBssAborted(struct ADAPTER *prAdapter,
		   struct BSS_INFO *prBssInfo);

#if CFG_SUPPORT_TDLS
uint32_t
rlmFillHtCapIEByParams(u_int8_t fg40mAllowed,
		       u_int8_t fgShortGIDisabled,
		       uint8_t u8SupportRxSgi20,
		       uint8_t u8SupportRxSgi40, uint8_t u8SupportRxGf,
		       enum ENUM_OP_MODE eCurrentOPMode, uint8_t *pOutBuf);

uint32_t rlmFillHtCapIEByAdapter(struct ADAPTER *prAdapter,
				 struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);

uint32_t rlmFillVhtCapIEByAdapter(struct ADAPTER *prAdapter,
				  struct BSS_INFO *prBssInfo, uint8_t *pOutBuf);

#endif

#if CFG_SUPPORT_802_11AC
void rlmReqGenerateVhtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtCapIE(struct ADAPTER *prAdapter,
			    struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtOpIE(struct ADAPTER *prAdapter,
			   struct MSDU_INFO *prMsduInfo);

void rlmFillVhtOpIE(struct ADAPTER *prAdapter,
		    struct BSS_INFO *prBssInfo, struct MSDU_INFO *prMsduInfo);

void rlmGenerateVhtTPEIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);

void rlmRspGenerateVhtOpNotificationIE(struct ADAPTER
			       *prAdapter, struct MSDU_INFO *prMsduInfo);
void rlmReqGenerateVhtOpNotificationIE(struct ADAPTER
			       *prAdapter, struct MSDU_INFO *prMsduInfo);




#endif
#if CFG_SUPPORT_802_11D
void rlmGenerateCountryIE(struct ADAPTER *prAdapter,
			  struct MSDU_INFO *prMsduInfo);
#endif
#if CFG_SUPPORT_DFS
u_int8_t rlmIsCsaAllow(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

void rlmProcessExCsaIE(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	struct SWITCH_CH_AND_BAND_PARAMS *prCSAParams,
	uint8_t ucChannelSwitchMode, uint8_t ucNewOperatingClass,
	uint8_t ucNewChannelNum, uint8_t ucChannelSwitchCount);

void rlmProcessSpecMgtAction(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb);

void rlmProcessPublicAction(struct ADAPTER *prAdapter,
			     struct SW_RFB *prSwRfb);

void rlmResetCSAParams(struct BSS_INFO *prBssInfo, uint8_t fgClearAll);

void rlmCsaTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr);

u_int8_t rlmCheckRxStbc(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			enum ENUM_FEATURE_OPTION ucFeatureFlag);

u_int8_t rlmCheckTxStbc(struct ADAPTER *prAdapter,
			uint8_t ucBssIndex,
			enum ENUM_FEATURE_OPTION ucFeatureFlag);

void rlmCsaDoneTimeout(struct ADAPTER *prAdapter,
				uintptr_t ulParamPtr);
#endif

uint32_t rlmUpdateStbcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable, uint8_t notify);

uint32_t rlmUpdateMrcSetting(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, uint8_t enable);

uint32_t
rlmSendOpModeFrameByType(struct ADAPTER *prAdapter,
	struct STA_RECORD *prStaRec,
	uint8_t ucOpChangeType,
	uint8_t ucChannelWidth,
	uint8_t ucRxNss, uint8_t ucTxNss);

uint32_t
rlmSendOpModeNotificationFrame(struct ADAPTER *prAdapter,
			       struct STA_RECORD *prStaRec,
			       uint8_t ucChannelWidth, uint8_t ucNss);

uint32_t
rlmSendSmPowerSaveFrame(struct ADAPTER *prAdapter,
			struct STA_RECORD *prStaRec, uint8_t ucNss);

uint32_t
rlmSendNotifyChannelWidthFrame(
		struct ADAPTER *prAdapter,
		struct STA_RECORD *prStaRec,
		uint8_t ucChannelWidth);

#if (CFG_SUPPORT_802_11AX == 1) || (CFG_SUPPORT_802_11BE == 1)
uint32_t
rlmSendOMIDataFrame(struct ADAPTER *prAdapter,
		    struct STA_RECORD *prStaRec,
		    uint8_t ucChannelWidth,
		    uint8_t ucOpRxNss,
		    uint8_t ucOpTxNss,
		    PFN_TX_DONE_HANDLER pfTxDoneHandler);
#endif

void rlmReqGenerateOMIIE(struct ADAPTER *prAdapter,
		struct BSS_INFO *prBssInfo);

void
rlmSendChannelSwitchFrame(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

uint16_t
rlmOpClassToBandwidth(uint8_t ucOpClass);

uint32_t
rlmNotifyVhtOpModeTxDone(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyOMIOpModeTxDone(struct ADAPTER *prAdapter,
			 struct MSDU_INFO *prMsduInfo,
			 enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyApGoOmiOpModeTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmSmPowerSaveTxDone(struct ADAPTER *prAdapter,
		     struct MSDU_INFO *prMsduInfo,
		     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint32_t
rlmNotifyChannelWidthtTxDone(struct ADAPTER *prAdapter,
			     struct MSDU_INFO *prMsduInfo,
			     enum ENUM_TX_RESULT_CODE rTxDoneStatus);

uint8_t
rlmGetBssOpBwByVhtAndHtOpInfo(struct BSS_INFO *prBssInfo);

uint8_t
rlmGetBssOpBwByOwnAndPeerCapability(struct ADAPTER *prAdapter,
	struct BSS_INFO *prBssInfo);

uint8_t rlmGetBssOpBwByChannelWidth(enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChannelWidth);

uint8_t
rlmGetVhtOpBwByBssOpBw(uint8_t ucBssOpBw);

uint8_t rlmGetVhtOpBw320ByS1(uint8_t ucS1);

void
rlmFillVhtOpInfoByBssOpBw(struct BSS_INFO *prBssInfo,
			  uint8_t ucChannelWidth);

enum ENUM_OP_CHANGE_STATUS_T
rlmChangeOperationMode(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	uint8_t ucChannelWidth,
	uint8_t ucOpRxNss,
	uint8_t ucOpTxNss,
	enum ENUM_OP_CHANGE_SEND_ACT_T ucSendAct,
	PFN_OPMODE_NOTIFY_DONE_FUNC pfOpChangeHandler
);

void
rlmDummyChangeOpHandler(struct ADAPTER *prAdapter,
	uint8_t ucBssIndex, bool fgIsChangeSuccess);

#if CFG_SUPPORT_BFER
void
rlmBfStaRecPfmuUpdate(struct ADAPTER *prAdapter, struct STA_RECORD *prStaRec);

void
rlmETxBfTriggerPeriodicSounding(struct ADAPTER *prAdapter);

bool
rlmClientSupportsVhtETxBF(struct STA_RECORD *prStaRec);

uint8_t
rlmClientSupportsVhtBfeeStsCap(struct STA_RECORD *prStaRec);

bool
rlmClientSupportsHtETxBF(struct STA_RECORD *prStaRec);
#endif

#if (CFG_SUPPORT_FACT_CAL == 1)
int rlmGetCenterCh(enum ENUM_BAND eBand, uint8_t ucCh,
		enum ENUM_CHANNEL_WIDTH eBw, enum ENUM_CHNL_EXT eSco);

uint32_t rlmFactCalStart(struct ADAPTER *prAdapter);

void rlmFactCalStop(struct ADAPTER *prAdapter);

void rlmFactCalDump(struct ADAPTER *prAdapter, enum FACT_CAL_TYPE eType);

uint32_t rlmFactCalHandler(struct ADAPTER *prAdapter,
		uint32_t u4Action, uint32_t u4CalType, uint32_t u4CalParam);

uint32_t rlmFactCalUpdateStruct(struct ADAPTER *prAdapter,
		enum FACT_CAL_STORE_ACTION eAction,
		struct UNI_EVENT_FACT_CAL_RAPID_GET_DATA *prCalData);

uint32_t rlmFactCalGetBufInfo(struct ADAPTER *prAdapter,
		struct FACT_CAL_DATA_BUF *prCalData, uint32_t u4CalDataIdx);

uint32_t rlmFactCalGet(struct ADAPTER *prAdapter,
			uint32_t u4CalType, uint8_t u4Band, uint32_t u4Channel);

uint32_t rlmFactCalSet(struct ADAPTER *prAdapter,
			uint32_t u4CalType, uint8_t u1Band, uint32_t u4Channel);

uint32_t rlmFactCalSetCalDataForSend(struct ADAPTER *prAdapter,
			uint32_t u4CalType, uint8_t u1Band,
			uint32_t u4Group,
			uint32_t u4Channel,
			uint32_t u4CalDataIdx);

uint32_t rlmFactCalSetByPassCal(struct ADAPTER *prAdapter, uint32_t u4Enable);

uint32_t rlmFactCalSetGrpAndCh(struct ADAPTER *prAdapter,
			enum FACT_CAL_BAND eBand, uint8_t ucCenterCh);

uint32_t rlmFactCalFileHandler(struct ADAPTER *prAdapter, uint32_t u4CalType,
			uint8_t fgWrite);

uint32_t rlmFactCalSetDefaultComAndGrp(struct ADAPTER *prAdapter);
#if (CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL == 1)
uint32_t rlmFactCalSetMappingTable(struct ADAPTER *prAdapter);
#endif /* CFG_SUPPORT_FACT_CAL_AXIDMA_MAPPING_TBL */
#endif /* CFG_SUPPORT_FACT_CAL */

void rlmModifyVhtBwPara(uint8_t *pucVhtChannelFrequencyS1,
			uint8_t *pucVhtChannelFrequencyS2,
			uint8_t ucHtChannelFrequencyS3,
			uint8_t *pucVhtChannelWidth);

#if (CFG_SUPPORT_WIFI_6G == 1)
void rlmTransferHe6gOpInfor(uint8_t ucChannelNum,
	uint8_t ucChannelWidth,
	uint8_t *pucChannelWidth,
	uint8_t *pucCenterFreqS1,
	uint8_t *pucCenterFreqS2,
	enum ENUM_CHNL_EXT *peSco);

void rlmModifyHE6GBwPara(uint8_t ucBw,
	uint8_t ucHe6gPrimaryChannel,
	uint8_t *pucHe6gChannelFrequencyS1,
	uint8_t *pucHe6gChannelFrequencyS2,
	enum ENUM_CHNL_EXT *peSco);
#endif

void rlmReviseMaxBw(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	enum ENUM_CHNL_EXT *peExtend,
	enum ENUM_CHANNEL_WIDTH *peChannelWidth,
	uint8_t *pucS1,
	uint8_t *pucPrimaryCh);

void rlmRevisePreferBandwidthNss(struct ADAPTER *prAdapter,
					uint8_t ucBssIndex,
					struct STA_RECORD *prStaRec);

void rlmSetMaxTxPwrLimit(struct ADAPTER *prAdapter, int8_t cLimit,
			 uint8_t ucEnable);

void rlmSyncExtCapIEwithSupplicant(uint8_t *aucCapabilities,
	const uint8_t *supExtCapIEs, size_t IElen);

int32_t rlmGetOpClassForChannel(int32_t channel,
	enum ENUM_BAND band, enum ENUM_CHNL_EXT eSco,
	enum ENUM_CHANNEL_WIDTH eChBw, uint16_t u2Country);

#if (CFG_SUPPORT_802_11AX == 1)
void rlmSetSrControl(struct ADAPTER *prAdapter, bool fgIsEnableSr);
#endif

#if CFG_AP_80211K_SUPPORT
void rlmMulAPAgentGenerateApRRMEnabledCapIE(
				struct ADAPTER *prAdapter,
				struct MSDU_INFO *prMsduInfo);
void rlmMulAPAgentTxMeasurementRequest(struct ADAPTER *prAdapter,
				       struct STA_RECORD *prStaRec,
				       struct SUB_ELEMENT_LIST *prSubIEs,
				       uint8_t ucToken,
				       uint16_t u2Repetitions);

void rlmMulAPAgentProcessRadioMeasurementResponse(
		struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb,
		void *pvHeader);

void rlmProcessRadioMeasurementResponse(
		struct ADAPTER *prAdapter, struct SW_RFB *prSwRfb);

#endif /* CFG_AP_80211K_SUPPORT */

#if (CFG_SUPPORT_TX_PWR_ENV == 1)
/*----------------------------------------------------------------------------*/
/*!
 * \brief Init Transmit Power Envelope max TxPower limit to max TxPower
 *
 * \param[in] picMaxTxPwr : Pointer of max TxPower limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrInit(int8_t *picMaxTxPwr);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use send Tranmit Power Envelope max TxPower limit to FW
 *
 * \param[in] prAdapter : Pointer to adapter
 * \param[in] eBand : RF Band index
 * \param[in] ucPriCh : Primary Channel
 * \param[in] picTxPwrEnvMaxPwr : Pointer to Tranmit Power Envelope max TxPower
 *                                limit
 *
 * \return value : void
 */
/*----------------------------------------------------------------------------*/
void rlmTxPwrEnvMaxPwrSend(
	struct ADAPTER *prAdapter,
	enum ENUM_BAND eBand,
	uint8_t ucPriCh,
	uint8_t ucPwrLmtNum,
	int8_t *picTxPwrEnvMaxPwr,
	uint8_t fgPwrLmtEnable);
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update TxPwr Envelope if need.
 *        1. It will translate IE content if the content is represent as PSD
 *        2. Update the TxPwr Envelope TxPower limit to BSS_DESC if the IE
 *           content is smaller than the current exit setting.
 *        3. If the TxPower limit have update, it will send cmd to FW to reset
 *           TxPower limit
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prTxPwrEnvIE : Pointer of TxPwer Envelope IE
 *
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmTxPwrEnvMaxPwrUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_TX_PWR_ENV_FRAME *prTxPwrEnvIE);
#endif /* CFG_SUPPORT_TX_PWR_ENV */

#if CFG_SUPPORT_802_11K
/*----------------------------------------------------------------------------*/
/*!
 * \brief This func is use to update Regulatory TxPower limit if need.
 *
 * \param[in] prAdapter : Pointer of adapter
 * \param[in] prBssDesc : Pointer ofBSS desription
 * \param[in] eHwBand : RF Band
 * \param[in] prCountryIE : Pointer of Country IE
 * \param[in] ucPowerConstraint : TxPower constraint value from Power
 *                                Constrait IE
 * \return value : Success : WLAN_STATUS_SUCCESS
 *                 Fail    : WLAN_STATUS_INVALID_DATA
 */
/*----------------------------------------------------------------------------*/
uint32_t rlmRegTxPwrLimitUpdate(
	struct ADAPTER *prAdapter,
	struct BSS_DESC *prBssDesc,
	enum ENUM_BAND eHwBand,
	struct IE_COUNTRY *prCountryIE,
	uint8_t ucPowerConstraint);
#endif /* CFG_SUPPORT_802_11K */

uint32_t rlmCalculateTpeIELen(struct ADAPTER *prAdapter,
			      uint8_t ucBssIndex, struct STA_RECORD *prStaRec);

void rlmGenerateTpeIE(struct ADAPTER *prAdapter,
		      struct MSDU_INFO *prMsduInfo);

enum ENUM_MAX_BANDWIDTH_SETTING
rlmVhtBw2OpBw(uint8_t ucVhtBw, enum ENUM_CHNL_EXT eSco);

#if CFG_SUPPORT_GEN_OP_CLASS
uint32_t rlmCalculateSupportedOpClassIELen(
	struct ADAPTER *prAdapter,
	uint8_t ucBssIndex,
	struct STA_RECORD *prStaRec);

void rlmGenerateSupportedOpClassIE(
	struct ADAPTER *prAdapter,
	struct MSDU_INFO *prMsduInfo);
#endif
/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */

#ifndef _lint
static __KAL_INLINE__ void rlmDataTypeCheck(void)
{
}
#endif /* _lint */

#endif /* _RLM_H */
