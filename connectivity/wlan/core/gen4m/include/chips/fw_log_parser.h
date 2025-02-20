/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  fw_dl.h
 */

#ifndef _IDX_LOG_H
#define _IDX_LOG_H

#define VER_TYPE_IDX_LOG_V2	0
#define VER_TYPE_CTRL		1
#define VER_TYPE_TXT_LOG	2

enum ENUM_FW_IDX_LOG_SAVE_CFG {
	FW_IDX_LOG_SAVE_DISABLE = 0,
	FW_IDX_LOG_SAVE_ONLY,
	FW_IDX_LOG_SAVE_WITH_PRINT
};

struct FW_LOG_IDX_DATA {
	/* FW Log Info Bin Data */
	void *prFwBuffer;
	uint32_t u4FwSize;
};

/* for little-endian CPU */
struct IDX_LOG_V2_FORMAT {
	uint8_t ucStx;
	uint8_t ucVerType : 3;
	uint8_t ucNumArgs : 5;
	uint8_t ucModId;
	uint8_t ucLevelId : 4;
	uint8_t ucSeqNo : 4;
	uint32_t u4TimeStamp;
	uint32_t u4IdxId;
	/* following is args 0~31, each is 4 bytes */
};

/* for little-endian CPU */
struct CTRL_TIME_SYNC_FORMAT {
	uint8_t ucStx;
	uint8_t ucVerType : 3;
	uint8_t ucCtrlType : 5;
	uint16_t u2Rsv;
	uint32_t u4TimeStamp;
	uint32_t u4UtcSec;
	uint32_t u4UtcUsec;
};

/* for little-endian CPU */
struct CTRL_LOG_LOST_FORMAT {
	uint8_t ucStx;
	uint8_t ucVerType : 3;
	uint8_t ucCtrlType : 5;
	uint16_t u2Rsv;
	uint32_t u4LogLostCount;
};

/* for little-endian CPU */
struct TEXT_LOG_FORMAT {
	uint8_t ucStx;
	uint8_t ucVerType : 3;
	uint8_t ucRsv0 : 5;
	uint8_t ucPayloadSize_wo_padding;
	uint8_t ucPayloadSize_w_padding;
	uint32_t u4TimeStamp;
	/* following is payload (text log) */
};

uint32_t wlanStoreIdxLogBin(struct ADAPTER *prAdapter, uint8_t *prSrc,
			    uint32_t u4FwSize);
uint32_t wlanOpenIdxLogBin(struct ADAPTER *prAdapter);
void wlanCloseIdxLogBin(struct ADAPTER *prAdapter);
uint32_t wlanFwLogIdxToStr(struct ADAPTER *prAdapter, uint8_t *pucIdxLog,
			   uint16_t u2MsgSize);

#endif /* _IDX_LOG_H */
