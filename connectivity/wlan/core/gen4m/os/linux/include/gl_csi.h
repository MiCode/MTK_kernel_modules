/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GL_CSI_H
#define _GL_CSI_H

#if CFG_SUPPORT_CSI
/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define CSI_RING_SIZE 1000
#define CSI_MAX_DATA_COUNT 1024		/* for BW320 */
#define CSI_MAX_RSVD1_COUNT 10
#define CSI_MAX_RSVD2_COUNT 10
#define CSI_MAX_STA_MAC_NUM 5		/* Depend on FW desing */

#define CSI_FIX_DATA_SIZE 107
#define CSI_H_max_Index 4		/* for 2x2 support */
#define Max_Stream_Bytes 3000
#define CSI_MAX_BUFFER_SIZE 4300

#define CSI_INFO_RSVD1 BIT(0)
#define CSI_INFO_RSVD2 BIT(1)

#define CSI_EVENT_RX_IDX_MASK	BITS(0, 15)
#define CSI_EVENT_RX_IDX_SHFIT	0
#define CSI_EVENT_TX_IDX_MASK	BITS(16, 31)
#define CSI_EVENT_TX_IDX_SHFIT	16
#define CSI_EVENT_RX_MODE_MASK  BITS(0, 15)
#define CSI_EVENT_RX_MODE_SHFIT 0
#define CSI_EVENT_RATE_MASK	BITS(16, 31)
#define CSI_EVENT_RATE_SHFIT    16

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
enum CSI_OPT_NUMBER {
	CSI_OPT_1 = 2,
	CSI_OPT_2,
	CSI_OPT_3,
	CSI_OPT_4,
	CSI_OPT_5,
	CSI_OPT_NUM =
		CSI_OPT_5 - 1
};

enum CSI_CONTROL_MODE_T {
	CSI_CONTROL_MODE_STOP,
	CSI_CONTROL_MODE_START,
	CSI_CONTROL_MODE_SET,
	CSI_CONTROL_MODE_NUM
};

enum CSI_CONFIG_ITEM_T {
	CSI_CONFIG_RSVD1,
	CSI_CONFIG_WF,
	CSI_CONFIG_RSVD2,
	CSI_CONFIG_FRAME_TYPE,
	CSI_CONFIG_TX_PATH,
	CSI_CONFIG_OUTPUT_FORMAT,
	CSI_CONFIG_INFO,
	CSI_CONFIG_CHAIN_NUMBER,
	CSI_CONFIG_FILTER_MODE,
	CSI_CONFIG_OUTPUT_METHOD,
	CSI_CONFIG_BAND,
	CSI_CONFIG_ITEM_NUM
};

enum CSI_OUTPUT_FORMAT_T {
	CSI_OUTPUT_RAW,
	CSI_OUTPUT_TONE_MASKED,
	CSI_OUTPUT_TONE_MASKED_SHIFTED,
	CSI_OUTPUT_FORMAT_NUM
};

enum CSI_FILTER_MODE_T {
	CSI_LENGTH_FILTER,
	CSI_MAC_FILTER,
	CSI_FILTER_MODE_NUM
};

enum CSI_STA_MAC_MODE_T {
	CSI_STA_MAC_DEL,
	CSI_STA_MAC_ADD,
	CSI_STA_MAC_SHOW,
	CSI_STA_MAC_MODE_NUM
};

enum CSI_EVENT_TLV_TAG {
	CSI_EVENT_VERSION,
	CSI_EVENT_CBW,
	CSI_EVENT_RSSI,
	CSI_EVENT_SNR,
	CSI_EVENT_BAND,
	CSI_EVENT_CSI_NUM,
	CSI_EVENT_CSI_I_DATA,
	CSI_EVENT_CSI_Q_DATA,
	CSI_EVENT_DBW,
	CSI_EVENT_CH_IDX,
	CSI_EVENT_TA,
	CSI_EVENT_EXTRA_INFO,
	CSI_EVENT_RX_MODE,
	CSI_EVENT_RSVD1,
	CSI_EVENT_RSVD2,
	CSI_EVENT_RSVD3,
	CSI_EVENT_RSVD4,
	CSI_EVENT_H_IDX,
	CSI_EVENT_TX_RX_IDX,
	CSI_EVENT_RSVD5,
	CSI_EVENT_BW_SEG,
	CSI_EVENT_REMAIN_LAST,
	CSI_EVENT_RSVD6,
	CSI_EVENT_TONE_VALID,
	CSI_EVENT_TLV_TAG_NUM,    /* csi event end, must be exist */
};

enum CSI_DATA_TLV_TAG {
	CSI_DATA_VER,
	CSI_DATA_TYPE,
	CSI_DATA_TS,
	CSI_DATA_RSSI,
	CSI_DATA_SNR,
	CSI_DATA_DBW,
	CSI_DATA_CH_IDX,
	CSI_DATA_TA,
	CSI_DATA_I,
	CSI_DATA_Q,
	CSI_DATA_EXTRA_INFO,
	CSI_DATA_RSVD1,
	CSI_DATA_RSVD2,
	CSI_DATA_RSVD3,
	CSI_DATA_RSVD4,
	CSI_DATA_TX_IDX,
	CSI_DATA_RX_IDX,
	CSI_DATA_FRAME_MODE,
	CSI_DATA_H_IDX,
	CSI_DATA_RX_RATE,
	CSI_DATA_RSVD5,
	CSI_DATA_RSVD6,
	CSI_DATA_BAND,
	CSI_DATA_TONE_VALID,
	CSI_DATA_TLV_TAG_NUM,
};

enum ENUM_CSI_OUTPUT_METHOD {
	CSI_OUTPUT_PROC_FILE = 0,
	CSI_OUTPUT_VENDOR_EVENT,
	CSI_OUTPUT_METHOD_NUM
};

enum CSI_OUTPUT_METHOND_COMMAND {
	CSI_PROC_FILE_COMMAND = 0,
	CSI_VENDOR_EVENT_COMMAND,
	CSI_OUTPUT_COMMAND_NUM
};

/*
 * CSI_DATA_T is used for representing
 * the CSI and other useful information
 * for application usage
 */
struct CSI_DATA_T {
	uint8_t ucFwVer;
	uint8_t ucBw;
	u_int8_t bIsCck;
	uint16_t u2DataCount;
	int16_t ac2IData[CSI_MAX_DATA_COUNT];
	int16_t ac2QData[CSI_MAX_DATA_COUNT];
	uint8_t ucDbdcIdx;
	int8_t cRssi;
	uint8_t ucSNR;
	uint64_t u8TimeStamp;
	uint8_t ucDataBw;
	uint8_t ucPrimaryChIdx;
	uint8_t aucTA[MAC_ADDR_LEN];
	uint32_t u4ExtraInfo;
	uint8_t ucRxMode;
	uint16_t u2RxRate;
	int32_t ai4Rsvd1[CSI_MAX_RSVD1_COUNT];
	int32_t au4Rsvd2[CSI_MAX_RSVD2_COUNT];
	uint8_t ucRsvd1Cnt;
	uint8_t ucRsvd2Cnt;
	int32_t i4Rsvd3;
	uint8_t ucRsvd4;
	uint32_t Antenna_pattern;
	uint32_t u4TRxIdx;
	uint32_t u4Rsvd5;
	uint32_t u4Rsvd6;
	uint32_t u4ToneValid;
	uint32_t u4SegmentNum;
	uint8_t ucRemainLast;
};

/*
 * CSI_INFO_T is used to store the CSI
 * settings and CSI event data
 */
struct CSI_INFO_T {
	/* Variables for manipulate the CSI data in g_aucProcBuf */
	u_int8_t bIncomplete;
	int32_t u4CopiedDataSize;
	int32_t u4RemainingDataSize;
	/* Variable for recording the CSI function config */
	uint8_t ucMode;
	uint8_t ucValue1[CSI_CONFIG_ITEM_NUM];
	uint32_t u4Value2[CSI_CONFIG_ITEM_NUM];
	/* Variable for manipulating the CSI ring buffer */
	struct CSI_DATA_T arCSIBuffer[CSI_RING_SIZE];
	struct CSI_DATA_T rCSISegmentTemp;
	uint32_t u4CSIBufferHead;
	uint32_t u4CSIBufferTail;
	uint32_t u4CSIBufferUsed;
	int16_t ai2TempIData[CSI_MAX_DATA_COUNT];
	int16_t ai2TempQData[CSI_MAX_DATA_COUNT];
	/* For usr to get the specific H(jw), 0 for all */
	uint16_t Matrix_Get_Bit;
	/* Send bytes to proc interfacel */
	uint8_t byte_stream[Max_Stream_Bytes];
	/* Station mac addr for filter */
	uint8_t *apucStaMacAddr[MAC_ADDR_LEN];
	uint8_t ucStaCount;
	struct LINK rStaList;
	enum ENUM_CSI_OUTPUT_METHOD eCSIOutput;
	struct CSI_DATA_T *prCSIData;
};

struct CSI_STA {
	struct LINK_ENTRY rLinkEntry;
	uint8_t aucMacAddress[MAC_ADDR_LEN];
};

struct CMD_CSI_CONTROL_T {
	uint8_t ucBandIdx;
	uint8_t ucMode;
	uint8_t ucCfgItem;
	uint8_t ucValue1;
	uint32_t u4Value2;
	uint8_t aucMacAddr[MAC_ADDR_LEN];
	/* For 32-bit alighment */
	uint8_t aucReserved1[2];
	/* Ask more for extension used */
	uint8_t aucReserved2[32];
};

struct CSI_TLV_ELEMENT {
	uint32_t tag_type;
	uint32_t body_len;
	uint8_t aucbody[];
};

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
#define GET_CSI_RX_IDX(TRX_IDX)	\
		((TRX_IDX & CSI_EVENT_RX_IDX_MASK) >> CSI_EVENT_RX_IDX_SHFIT)

#define GET_CSI_TX_IDX(TRX_IDX)	\
		((TRX_IDX & CSI_EVENT_TX_IDX_MASK) >> CSI_EVENT_TX_IDX_SHFIT)

#define GET_CSI_RX_MODE(DATA) \
		((DATA & CSI_EVENT_RX_MODE_MASK) >> CSI_EVENT_RX_MODE_SHFIT)

#define GET_CSI_RATE(DATA) \
		((DATA & CSI_EVENT_RATE_MASK) >> CSI_EVENT_RATE_SHFIT)

/*******************************************************************************
 *                             F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */
uint8_t glCsiGetBandIdx(struct GLUE_INFO *prGlueInfo);
void glCsiSetBandIdx(struct GLUE_INFO *prGlueInfo, uint8_t ucBandIdx);
struct CSI_INFO_T *glCsiGetCSIInfo(struct GLUE_INFO *prGlueInfo);
uint8_t *glCsiGetCSIBuf(struct GLUE_INFO *prGlueInfo);
struct CSI_DATA_T *glCsiGetCSIData(struct GLUE_INFO *prGlueInfo);
void glCsiSupportInit(struct GLUE_INFO *prGlueInfo);
void glCsiSupportDeinit(struct GLUE_INFO *prGlueInfo);
void glCsiSetEnable(struct GLUE_INFO *prGlueInfo,
	struct CSI_INFO_T *prCSIInfo, u_int8_t fgEnable);
int32_t glCsiAddSta(struct GLUE_INFO *prGlueInfo,
	struct CMD_CSI_CONTROL_T *prCSICtrl);
int32_t glCsiDelSta(struct GLUE_INFO *prGlueInfo,
	struct CMD_CSI_CONTROL_T *prCSICtrl);
void glCsiFreeStaList(struct GLUE_INFO *prGlueInfo);
void nicEventCSIData(struct ADAPTER *prAdapter,
	struct WIFI_EVENT *prEvent);
u_int8_t wlanPushCSISegmentData(struct ADAPTER *prAdapter,
	struct CSI_DATA_T *prCSIData);
u_int8_t wlanPushCSIData(struct ADAPTER *prAdapter,
	struct CSI_DATA_T *prCSIData);
u_int8_t wlanPopCSIData(struct ADAPTER *prAdapter,
	struct CSI_DATA_T *prCSIData);
ssize_t wlanCSIDataPrepare(uint8_t *buf,
	struct CSI_INFO_T *prCSIInfo, struct CSI_DATA_T *prCSIData);
void wlanApplyCSIToneMask(
	uint8_t ucRxMode,
	uint8_t ucCBW,
	uint8_t ucDBW,
	uint8_t ucPrimaryChIdx,
	int16_t *ai2IData,
	int16_t *ai2QData);
void wlanShiftCSI(
	uint8_t ucRxMode,
	uint8_t ucCBW,
	uint8_t ucDBW,
	uint8_t ucPrimaryChIdx,
	int16_t *ai2IData,
	int16_t *ai2QData,
	int16_t *ai2ShiftIData,
	int16_t *ai2ShiftQData);
#endif /* CFG_SUPPORT_CSI */

#endif /* _GL_CSI_H */
