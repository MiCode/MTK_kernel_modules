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
 Module Name:
 hif_pdma.h
 */

#ifndef __HIF_PDMA_H__
#define __HIF_PDMA_H__

#include <linux/list_sort.h>
#include <linux/hashtable.h>
#include "mt66xx_reg.h"

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define NUM_OF_WFDMA1_TX_RING			0
#define NUM_OF_WFDMA1_RX_RING			0

#if (CFG_SUPPORT_CONNAC2X == 1 || CFG_SUPPORT_CONNAC3X == 1)

#undef NUM_OF_WFDMA1_TX_RING
#ifdef CONFIG_NUM_OF_WFDMA_TX_RING
#define NUM_OF_WFDMA1_TX_RING			(CONFIG_NUM_OF_WFDMA_TX_RING)
#else
#define NUM_OF_WFDMA1_TX_RING			1  /* WA CMD Ring */
#endif

#undef NUM_OF_WFDMA1_RX_RING
#ifdef CONFIG_NUM_OF_WFDMA_RX_RING
#define NUM_OF_WFDMA1_RX_RING			(CONFIG_NUM_OF_WFDMA_RX_RING)
#else
#define NUM_OF_WFDMA1_RX_RING			5
#endif

#endif /* CFG_SUPPORT_CONNAC2X == 1 */

/*
 * 3 data ring (ring0 + ring1[DBDC] + ring2[priority] + ring3[ALTX])
 * fwdl ring
 * cmd ring
 * wa cmd ring
 */
#define NUM_OF_TX_RING				(6 + NUM_OF_WFDMA1_TX_RING)
#define NUM_OF_RX_RING				(2 + NUM_OF_WFDMA1_RX_RING)

#define RX_RING_MAX_SIZE			4095

#if defined(CONFIG_MTK_WIFI_BW320)
#ifdef BELLWETHER
#define TX_RING_SIZE				1024
#else
#define TX_RING_SIZE				3072
#endif
#define TX_RING_DATA_SIZE			TX_RING_SIZE
#define TX_RING_CMD_SIZE			320

#define HIF_NUM_OF_QM_RX_PKT_NUM		10240
#define HIF_PLE_PAGE_SIZE			0xBC0
#define HIF_AMSDU_COUNT				4
#define HIF_TX_MSDU_TOKEN_NUM \
	(HIF_PLE_PAGE_SIZE * HIF_AMSDU_COUNT)

#elif defined(CONFIG_MTK_WIFI_HE160)
#define TX_RING_SIZE				1024
#define TX_RING_DATA_SIZE			1024
#define TX_RING_CMD_SIZE			256
#define HIF_NUM_OF_QM_RX_PKT_NUM		4096
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 4)

#elif defined(CONFIG_MTK_WIFI_HE80)
#define TX_RING_SIZE				1024
#define TX_RING_DATA_SIZE			1024
#define TX_RING_CMD_SIZE			256
#define HIF_NUM_OF_QM_RX_PKT_NUM		2048
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 2)

#elif defined(CONFIG_MTK_WIFI_VHT80)
#define TX_RING_SIZE				512
#define TX_RING_DATA_SIZE			512
#define TX_RING_CMD_SIZE			256
#define HIF_NUM_OF_QM_RX_PKT_NUM		2048
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 3)

#else
#define TX_RING_SIZE				256
#define TX_RING_DATA_SIZE			256
#define TX_RING_CMD_SIZE			256
#define HIF_NUM_OF_QM_RX_PKT_NUM		2048
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 3)
#endif

/* TXD_SIZE = TxD + TxInfo */
#define TXD_SIZE					16
#define RXD_SIZE					16

#define RX_BUFFER_AGGRESIZE			3840
#define RX_BUFFER_NORMSIZE			3840
#define TX_BUFFER_NORMSIZE			3840


#define HIF_TX_MAX_SIZE_PER_FRAME         (NIC_TX_MAX_SIZE_PER_FRAME +      \
					   NIC_TX_DESC_AND_PADDING_LENGTH)

#define HIF_TX_PREALLOC_DATA_BUFFER			1

#define HIF_IST_LOOP_COUNT					32
/* Min msdu count to trigger Tx during INT polling state */
#define HIF_IST_TX_THRESHOLD				1

#define HIF_TX_BUFF_COUNT_TC0				4096
#define HIF_TX_BUFF_COUNT_TC1				4096
#define HIF_TX_BUFF_COUNT_TC2				4096
#define HIF_TX_BUFF_COUNT_TC3				4096
#define HIF_TX_BUFF_COUNT_TC4				(TX_RING_CMD_SIZE - 1)
#define HIF_TX_BUFF_COUNT_TC5				4096

/* enable/disable TX resource control */
#define HIF_TX_RESOURCE_CTRL                1
/* enable/disable TX resource control PLE */
#define HIF_TX_RESOURCE_CTRL_PLE            0


#define HIF_TX_PAGE_SIZE_IN_POWER_OF_2		11
/* in unit of bytes */
#define HIF_TX_PAGE_SIZE					2048

#define HIF_EXTRA_IO_BUFFER_SIZE			0

#define HIF_TX_COALESCING_BUFFER_SIZE		(TX_BUFFER_NORMSIZE)
#define HIF_RX_COALESCING_BUFFER_SIZE		(RX_BUFFER_AGGRESIZE)

#define HIF_CR4_FWDL_SECTION_NUM			1
#define HIF_IMG_DL_STATUS_PORT_IDX			1

#define HIF_TX_INIT_CMD_PORT				TX_RING_FWDL

#define HIF_TX_PAYLOAD_LENGTH				72

#define HIF_MSDU_REPORT_RETURN_TIMEOUT		10	/* sec */
#define HIF_SER_TIMEOUT				10000	/* msec */
#define HIF_SER_POWER_OFF_RETRY_COUNT		100
#define HIF_SER_POWER_OFF_RETRY_TIME		10	/* msec */

#define MT_RINGREG_DIFF		0x10
#define MT_RINGREG_EXT_DIFF	0x04

#define MT_TX_RING_BASE		WPDMA_TX_RING0_CTRL0
#define MT_TX_RING_PTR		WPDMA_TX_RING0_CTRL0
#define MT_TX_RING_CNT		WPDMA_TX_RING0_CTRL1
#define MT_TX_RING_CIDX		WPDMA_TX_RING0_CTRL2
#define MT_TX_RING_DIDX		WPDMA_TX_RING0_CTRL3

#define MT_RX_RING_BASE		WPDMA_RX_RING0_CTRL0
#define MT_RX_RING_PTR		WPDMA_RX_RING0_CTRL0
#define MT_RX_RING_CNT		WPDMA_RX_RING0_CTRL1
#define MT_RX_RING_CIDX		WPDMA_RX_RING0_CTRL2
#define MT_RX_RING_DIDX		WPDMA_RX_RING0_CTRL3

#define MT_RING_CIDX_MASK	0x00000FFF
#define MT_RING_DIDX_MASK	0x00000FFF
#define MT_RING_CNT_MASK	0x00000FFF

#define DMA_LOWER_32BITS_MASK   0x00000000FFFFFFFF
#define DMA_HIGHER_4BITS_MASK   0x0000000F
#define DMA_BITS_OFFSET		32

#define DMA_DONE_WAITING_TIME   10
#define DMA_DONE_WAITING_COUNT  (100 * 1000)

#define MT_TX_RING_BASE_EXT WPDMA_TX_RING0_BASE_PTR_EXT
#define MT_RX_RING_BASE_EXT WPDMA_RX_RING0_BASE_PTR_EXT

#define PDMA_DUMMY_RESET_VALUE  0x0F
#define PDMA_DUMMY_MAGIC_NUM    0x13

#define TXD_DW1_RMVL            BIT(10)
#define TXD_DW1_VLAN            BIT(11)
#define TXD_DW1_ETYP            BIT(12)
#define TXD_DW1_AMSDU_C         BIT(20)

#define HIF_DEADFEED_VALUE      0xdeadfeed

#define HIF_DEFAULT_BSS_FREE_CNT	64
#if (CFG_TX_HIF_CREDIT_FEATURE == 1)
#define HIF_TX_CREDIT_STEP_LEVET (TX_RING_SIZE / 2)
#define HIF_TX_CREDIT_STEP_COUNT (TX_RING_SIZE / HIF_TX_CREDIT_STEP_LEVET)
#define HIF_DEFAULT_MAX_BSS_TX_CREDIT	(TX_RING_SIZE * 2)
#define HIF_DEFAULT_MIN_BSS_TX_CREDIT	(TX_RING_SIZE >> 3)
#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
#define HIF_DEFAULT_MAX_BSS_BALANCE_TX_CREDIT	(HIF_TX_MSDU_TOKEN_NUM)
#define HIF_DEFAULT_MIN_BSS_BALANCE_TX_CREDIT	(TX_RING_SIZE >> 4)
#endif
#define HIF_TX_CREDIT_HIGH_USAGE	70
#define HIF_TX_CREDIT_LOW_USAGE		30
#endif

#define HIF_FLAG_SW_WFDMA_INT		BIT(0)
#define HIF_FLAG_SW_WFDMA_INT_BIT	(0)

#define SW_WFDMA_CMD_NUM		4
#define SW_WFDMA_CMD_PKT_SIZE		1600
#define SW_WFDMA_EMI_SIZE \
	(SW_WFDMA_CMD_NUM * SW_WFDMA_CMD_PKT_SIZE + 8)
#define SW_WFDMA_MAX_RETRY_COUNT	100
#define SW_WFDMA_RETRY_TIME		10

#define SW_EMI_MEMORY_SIZE		2048
#define SW_EMI_RING_SIZE		16

#define MSDU_TOKEN_HISTORY_NUM 5

#define LOG_DUMP_COUNT_PERIOD		5
#define LOG_DUMP_FULL_DUMP_TIMES	2

#define WFDMA_MAGIC_CNT_NUM      16
#define INDCMD_MAGIC_CNT_NUM     8
#define RX_BLK_MAGIC_CNT_NUM     4
#define MAWD_RX_BLK_RING_SIZE    4095
#define MAWD_ENABLE_WAKEUP_SLEEP 1
#define MAWD_POWER_UP_RETRY_CNT  10000
#define MAWD_POWER_UP_WAIT_TIME  10
#define MAWD_MAX_PATCH_NUM       19
#define MAWD_MD_TX_RING_NUM      2
#define MAWD_CR_BACKUP_VALID     88
#define MAWD_CR_BACKUP_OFFSET    89
#define RRO_HASH_TABLE_SIZE      (RX_RING_MAX_SIZE * 2)
#define RRO_BA_BITMAP_SIZE       128
#if (CFG_MTK_FPGA_PLATFORM == 1)
#define RRO_MAX_STA_NUM          8
#else
#define RRO_MAX_STA_NUM          16
#endif
#define RRO_MAX_TID_NUM          8
#define RRO_ADDR_ELEM_SIZE       16
#define RRO_TOTAL_ADDR_ELEM_NUM  (RRO_MAX_STA_NUM * RRO_MAX_TID_NUM)
#define RRO_MAX_WINDOW_NUM       1024
#define RRO_IND_CMD_RING_SIZE    1024
#define RRO_DROP_BY_HIF          0

#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
#define HIF_TX_DATA_DELAY_TIMEOUT_BIT        0
#define HIF_TX_DATA_DELAY_TIMER_RUNNING_BIT  1
#endif

#define HIF_INT_TIME_DEBUG              0

#define FW_BIN_FLAVOR_KEY		"flavor-bin"

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define INC_RING_INDEX(_idx, _RingSize)		\
{ \
	(_idx) = (_idx+1) % (_RingSize); \
	KAL_MB_W(); \
}

#define RTMP_HOST_IO_READ32(_A, _R, _pV) \
{ \
	(*(_pV) = readl((void *)(_A->HostCSRBaseAddress + \
				 (_R - _A->u4HostCsrOffset)))); \
}

#define RTMP_HOST_IO_WRITE32(_A, _R, _V) \
{ \
	writel(_V, (void *)(_A->HostCSRBaseAddress + \
			    (_R - _A->u4HostCsrOffset))); \
}

#define RTMP_IO_READ32(_A, _R, _pV) \
{ \
	(*(_pV) = readl((void *)((_A)->CSRBaseAddress + (_R)))); \
}

#define RTMP_IO_WRITE32(_A, _R, _V) \
{ \
	writel(_V, (void *)((_A)->CSRBaseAddress + (_R))); \
}

#define RTMP_IO_READ_RANGE(_A, _D, _S, _N) \
{ \
	memcpy_fromio((void *) _S, (void *)((_A)->CSRBaseAddress + (_D)), _N); \
}

#define RTMP_IO_WRITE_RANGE(_A, _D, _S, _N) \
{ \
	memcpy_toio((void *)((_A)->CSRBaseAddress + (_D)), (void *) _S, _N); \
}

#define HAL_SET_RING_CIDX(_G, _R, _V) \
{ \
	kalDevRegWrite(_G, _R->hw_cidx_addr, _V << _R->hw_cidx_shift); \
}

#define HAL_GET_RING_CIDX(_G, _R, _V) \
do { \
	kalDevRegRead(_G, _R->hw_cidx_addr, _V); \
	*_V = (*_V & _R->hw_cidx_mask) >> _R->hw_cidx_shift; \
} while (0)

#define HAL_GET_RING_DIDX(_G, _R, _V) \
do { \
	kalDevRegRead(_G, _R->hw_didx_addr, _V); \
	*_V = (*_V & _R->hw_didx_mask) >> _R->hw_didx_shift; \
} while (0)

#define HAL_GET_RING_MCNT(_G, _R, _V) \
do { \
	kalDevRegRead(_G, _R->hw_cnt_addr, _V); \
	*_V = (*_V & _R->hw_cnt_mask) >> _R->hw_cnt_shift; \
} while (0)

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */
/* hw defined tx ring idx */
enum ENUM_HW_WFDMA0_TX_RING_IDX {
	HW_WFDMA0_TX_RING_IDX_0 = 0,
	HW_WFDMA0_TX_RING_IDX_1,
	HW_WFDMA0_TX_RING_IDX_2,
	HW_WFDMA0_TX_RING_IDX_3,
	HW_WFDMA0_TX_RING_IDX_4,
	HW_WFDMA0_TX_RING_IDX_5,
	HW_WFDMA0_TX_RING_IDX_6,
	HW_WFDMA0_TX_RING_IDX_7,
	HW_WFDMA0_TX_RING_IDX_8,
	HW_WFDMA0_TX_RING_IDX_9,
	HW_WFDMA0_TX_RING_IDX_10,
	HW_WFDMA0_TX_RING_IDX_11,
	HW_WFDMA0_TX_RING_IDX_12,
	HW_WFDMA0_TX_RING_IDX_13,
	HW_WFDMA0_TX_RING_IDX_14,
	HW_WFDMA0_TX_RING_IDX_15,
	HW_WFDMA0_TX_RING_IDX_16,
	HW_WFDMA0_TX_RING_IDX_17
};

/* hw defined rx ring idx */
enum ENUM_HW_WFDMA0_RX_RING_IDX {
	HW_WFDMA0_RX_RING_IDX_0 = 0,
	HW_WFDMA0_RX_RING_IDX_1,
	HW_WFDMA0_RX_RING_IDX_2,
	HW_WFDMA0_RX_RING_IDX_3,
	HW_WFDMA0_RX_RING_IDX_4,
	HW_WFDMA0_RX_RING_IDX_5,
	HW_WFDMA0_RX_RING_IDX_6,
	HW_WFDMA0_RX_RING_IDX_7,
	HW_WFDMA0_RX_RING_IDX_8,
	HW_WFDMA0_RX_RING_IDX_9,
};

struct GL_HIF_INFO;

#if (CFG_TX_HIF_CREDIT_FEATURE == 1)
#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
enum ENUM_WFD_BSS_BALANCE_STATE {
	WFD_BSS_BALANCE_NO_LIMIT_STATE = 0,
	WFD_BSS_BALANCE_QUICK_STATE,
	WFD_BSS_BALANCE_MAIN_STATE,
	WFD_BSS_BALANCE_STEP_STATE,
	WFD_BSS_BALANCE_FORCE_STATE
};
#endif

enum ENUM_WFD_ADJUST_CTRL_MODE {
	WFD_DEFAULT_MODE = 0,
	WFD_SCC_BALANCE_MODE
};
#endif

/* ============================================================================
 * PCI/RBUS TX / RX Frame Descriptors format
 *
 * Memory Layout
 *
 * 1. Tx Descriptor
 * TxD (12 bytes) + TXINFO (4 bytes)
 * 2. Packet Buffer
 * TXWI + 802.11
 * 31                                                                         0
 * +--------------------------------------------------------------------------+
 * |                                   SDP0[31:0]                             |
 * +-+--+---------------------+-+--+------------------------------------------+
 * |D |L0|       SDL0[13:0]              |B|L1|                    SDL1[13:0] |
 * +-+--+---------------------+-+--+------------------------------------------+
 * |                                   SDP1[31:0]                             |
 * +--------------------------------------------------------------------------+
 * |                                  TX / RX INFO                            |
 * +--------------------------------------------------------------------------+
 * =========================================================================
 */
/*
 *  TX descriptor format for Tx Data/Mgmt Rings
 */
struct TXD_STRUCT {
	/* Word 0 */
	uint32_t SDPtr0;

	/* Word 1 */
	uint32_t SDLen1:14;
	uint32_t LastSec1:1;
	uint32_t Burst:1;
	uint32_t SDLen0:14;
	uint32_t LastSec0:1;
	uint32_t DMADONE:1;

	/*Word2 */
	uint32_t SDPtr1;

	/*Word3 */
	uint16_t SDPtr0Ext;
	uint16_t SDPtr1Ext;
};

/*
 *  Rx descriptor format for Rx Rings
 */
struct RXD_STRUCT {
	/* Word 0 */
	uint32_t SDPtr0;

	/* Word 1 */
	uint32_t SDLen1:14;
	uint32_t LastSec1:1;
	uint32_t Burst:1;
	uint32_t SDLen0:14;
	uint32_t LastSec0:1;
	uint32_t DMADONE:1;

	/* Word 2 */
	uint32_t SDPtr1;

	/* Word 3 */
	uint32_t RXINFO:28;
	uint32_t MagicCnt:4;
};

/*
 *	Data buffer for DMA operation, the buffer must be contiguous
 *	physical memory Both DMA to / from CPU use the same structure.
 */
struct RTMP_DMABUF {
	unsigned long AllocSize;
	void *AllocVa;		/* TxBuf virtual address */
	phys_addr_t AllocPa;		/* TxBuf physical address */
	u_int8_t fgIsCopyPath;	/* RxBuf is copy path */
};

/*
 *	Control block (Descriptor) for all ring descriptor DMA operation,
 *	buffer must be contiguous physical memory. NDIS_PACKET stored the
 *	binding Rx packet descriptor which won't be released, driver has to
 *	wait until upper layer return the packet before giveing up this rx
 *	ring descriptor to ASIC. NDIS_BUFFER is assocaited pair to describe
 *	the packet buffer. For Tx, NDIS_PACKET stored the tx packet descriptor
 *  which driver should ACK upper layer when the tx is physically done or
 *  failed.
 */
struct RTMP_DMACB {
	unsigned long AllocSize;	/* Control block size */
	void *AllocVa;			/* Control block virtual address */
	phys_addr_t AllocPa;	        /* Control block physical address */
	void *pPacket;
	void *pBuffer;
	phys_addr_t PacketPa;
	struct RTMP_DMABUF DmaBuf;	/* Associated DMA buffer structure */
	struct MSDU_TOKEN_ENTRY *prToken;
};

struct RTMP_TX_RING {
	struct RTMP_DMACB Cell[TX_RING_SIZE];
	uint32_t TxCpuIdx;
	uint32_t TxDmaIdx;
	uint32_t u4BufSize;
	uint32_t u4RingSize;
	uint32_t TxSwUsedIdx;
	uint32_t u4UsedCnt;
	uint32_t hw_desc_base;
	uint32_t hw_desc_base_ext;
	uint32_t hw_cidx_addr;
	uint32_t hw_cidx_mask;
	uint32_t hw_cidx_shift;
	uint32_t hw_didx_addr;
	uint32_t hw_didx_mask;
	uint32_t hw_didx_shift;
	uint32_t hw_cnt_addr;
	uint32_t hw_cnt_mask;
	uint32_t hw_cnt_shift;
	spinlock_t rTxDmaQLock;
	u_int8_t fgStopRecycleDmad;
};

struct RTMP_RX_RING {
	struct RTMP_DMACB Cell[RX_RING_MAX_SIZE];
	uint32_t RxCpuIdx;
	uint32_t RxDmaIdx;
	uint32_t u4BufSize;
	uint32_t u4RingSize;
	u_int8_t fgRxSegPkt;
	uint32_t hw_desc_base;
	uint32_t hw_desc_base_ext;
	uint32_t hw_cidx_addr;
	uint32_t hw_cidx_mask;
	uint32_t hw_cidx_shift;
	uint32_t hw_didx_addr;
	uint32_t hw_didx_mask;
	uint32_t hw_didx_shift;
	uint32_t hw_cnt_addr;
	uint32_t hw_cnt_mask;
	uint32_t hw_cnt_shift;
	bool fgIsDumpLog;
	bool fgIsWaitRxDmaDoneTimeout;
	uint32_t u4LastRxEventWaitDmaDoneCnt;
	uint32_t u4PendingCnt;
	void *pvPacket;
	uint32_t u4PacketLen;
	uint32_t u4MagicCnt;
};

struct PCIE_CHIP_CR_MAPPING {
	uint32_t u4ChipAddr;
	uint32_t u4BusAddr;
	uint32_t u4Range;
};

struct pcie2ap_remap {
	uint32_t reg_base;
	uint32_t reg_mask;
	uint32_t reg_shift;
	uint32_t base_addr;
};

struct ap2wf_remap {
	uint32_t reg_base;
	uint32_t reg_mask;
	uint32_t reg_shift;
	uint32_t base_addr;
};

struct PCIE_CHIP_CR_REMAPPING {
	const struct pcie2ap_remap *pcie2ap;
	const struct ap2wf_remap *ap2wf;
};

struct MSDU_TOKEN_ENTRY {
	uint32_t u4Token;
	u_int8_t fgInUsed;
	struct timespec64 rTs;
	uint32_t u4CpuIdx;	/* tx ring cell index */
	struct MSDU_INFO *prMsduInfo;
	void *prPacket;
	phys_addr_t rDmaAddr;
	uint32_t u4DmaLength;
	phys_addr_t rPktDmaAddr;
	uint32_t u4PktDmaLength;
	uint16_t u2Port; /* tx ring number */
	uint8_t ucWlanIndex;
	uint8_t ucBssIndex;
};

struct TOKEN_HISTORY {
	uint32_t u4UsedCnt;
	uint32_t u4LongestId;
};

struct MSDU_TOKEN_HISTORY_INFO {
	struct TOKEN_HISTORY au4List[MSDU_TOKEN_HISTORY_NUM];
	uint32_t u4CurIdx;
};

#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
struct WFD_LLS_TX_BIT_RATE {
	uint32_t au4CurrentBitrate[BSSID_NUM];
	uint32_t au4PredictBitrate[BSSID_NUM];
};
#endif

struct MSDU_TOKEN_INFO {
	uint32_t u4UsedCnt;
	struct MSDU_TOKEN_ENTRY *aprTokenStack[HIF_TX_MSDU_TOKEN_NUM];
	spinlock_t rTokenLock;
	struct MSDU_TOKEN_ENTRY arToken[HIF_TX_MSDU_TOKEN_NUM];
	uint32_t u4TokenNum;

	/* control bss index packet number */
	uint32_t u4TxBssCnt[MAX_BSSID_NUM];
	uint32_t u4MaxBssFreeCnt;
#if (CFG_TX_HIF_CREDIT_FEATURE == 1)
	enum ENUM_WFD_ADJUST_CTRL_MODE u4EnAdjustCtrlMode;
	bool fgEnAdjustCtrl;
	uint32_t u4TxCredit[MAX_BSSID_NUM];
	uint32_t u4LastTxBssCnt[MAX_BSSID_NUM];
	uint32_t u4MaxBssTxCredit;
	uint32_t u4MinBssTxCredit;
#if (CFG_WFD_SCC_BALANCE_SUPPORT == 1)
	enum ENUM_WFD_BSS_BALANCE_STATE u4WFDBssBalanceState;
	uint32_t u4MaxBssBalanceTxCredit;
	uint32_t u4MinBssBalanceTxCredit;
	uint32_t u4DataMsduRptCountPerBss[MAX_BSSID_NUM]; /* HIF_STATS */
	signed long u4TxDiffBytes[MAX_BSSID_NUM]; /* kalPerMonUpdate */
	signed long u4RxDiffBytes[MAX_BSSID_NUM]; /* kalPerMonUpdate */
	struct WFD_LLS_TX_BIT_RATE bitrate;
#endif
#endif
	struct MSDU_TOKEN_HISTORY_INFO rHistory;
};

struct TX_CMD_REQ {
	struct CMD_INFO rCmdInfo;
	uint8_t aucBuff[TX_BUFFER_NORMSIZE];
	uint8_t ucTC;
	struct list_head list;
};

struct TX_DATA_REQ {
	struct MSDU_INFO *prMsduInfo;
	struct list_head list;
};

struct AMSDU_MAC_TX_DESC {
	uint16_t u2TxByteCount;
	uint16_t u2DW0;
	uint32_t u4DW1;
	uint32_t u4DW2:31;
	uint32_t u4FR:1;
	uint32_t u4DW3;
	uint32_t u4DW4;
	uint32_t u4DW5_1:9;
	uint32_t u4TXS:2;
	uint32_t u4DW5_2:21;
	uint32_t u4DW6;
	uint32_t u4DW7;
};

struct ERR_RECOVERY_CTRL_T {
	uint8_t eErrRecovState;
	uint32_t u4Status;
	uint32_t u4BackupStatus;
};

struct SW_WFDMA_INFO;

struct SW_WFDMAD {
	uint32_t u4DrvIdx;
	uint32_t u4FwIdx;
	uint8_t aucBuf[SW_WFDMA_CMD_NUM][SW_WFDMA_CMD_PKT_SIZE];
};

struct SW_WFDMA_OPS {
	void (*init)(struct GLUE_INFO *prGlueInfo);
	void (*uninit)(struct GLUE_INFO *prGlueInfo);
	void (*enable)(struct GLUE_INFO *prGlueInfo, bool fgEn);
	void (*reset)(struct SW_WFDMA_INFO *prSwWfdmaInfo);
	void (*backup)(struct GLUE_INFO *prGlueInfo);
	void (*restore)(struct GLUE_INFO *prGlueInfo);
	void (*getCidx)(struct GLUE_INFO *prGlueInfo, uint32_t *pu4Cidx);
	void (*setCidx)(struct GLUE_INFO *prGlueInfo, uint32_t u4Cidx);
	void (*getDidx)(struct GLUE_INFO *prGlueInfo, uint32_t *pu4Didx);
	bool (*writeCmd)(struct GLUE_INFO *prGlueInfo);
	bool (*processDmaDone)(struct GLUE_INFO *prGlueInfo);
	void (*triggerInt)(struct GLUE_INFO *prGlueInfo);
	void (*getIntSta)(struct GLUE_INFO *prGlueInfo,  uint32_t *pu4Sta);
	void (*dumpDebugLog)(struct GLUE_INFO *prGlueInfo);
};

struct SW_WFDMA_INFO {
	struct SW_WFDMA_OPS rOps;
	struct SW_WFDMAD *prDmad;
	struct SW_WFDMAD rBackup;
	bool fgIsEnSwWfdma;
	bool fgIsEnAfterFwdl;
	void *pucIoremapAddr;
	uint32_t u4PortIdx;
	uint32_t u4EmiOffsetAddr;
	uint32_t u4EmiOffsetBase;
	uint32_t u4EmiOffsetMask;
	uint32_t u4EmiOffset;
	uint32_t u4CcifStartAddr;
	uint32_t u4CcifTchnumAddr;
	uint32_t u4CcifChlNum;
	uint32_t u4CpuIdx;
	uint32_t u4DmaIdx;
	uint32_t u4CpuIdxBackup;
	uint32_t u4DmaIdxBackup;
	uint32_t u4MaxCnt;
	uint8_t aucCID[SW_WFDMA_CMD_NUM];
};

struct SW_EMI_CTX {
	uint32_t u4DrvIdx;
	uint32_t u4FwIdx;
	uint32_t u4RingSize;
	uint32_t au4Addr[SW_EMI_RING_SIZE];
	uint32_t au4Val[SW_EMI_RING_SIZE];
};

struct SW_EMI_RING_INFO;

struct SW_EMI_RING_OPS {
	void (*init)(struct GLUE_INFO *prGlueInfo);
	u_int8_t (*read)(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
			 uint32_t *pu4Val);
	void (*triggerInt)(struct GLUE_INFO *prGlueInfo);
	void (*debug)(struct GLUE_INFO *prGlueInfo);
};

struct SW_EMI_RING_INFO {
	struct SW_EMI_RING_OPS rOps;
	struct SW_EMI_CTX *prEmi;
	u_int8_t fgIsSupport;
	u_int8_t fgIsEnable;
	uint32_t u4CcifTchnumAddr;
	uint32_t u4CcifChlNum;
	spinlock_t rRingLock;
};

enum mtk_queue_attr {
	Q_TX_DATA,
	Q_TX_CMD,
	Q_TX_CMD_WM,
	Q_TX_FWDL,
	Q_RX_DATA,
	Q_RX_EVENT_WM,
	Q_RX_EVENT_WA,
	Q_ATTR_NUM
};

struct pci_queue_desc {
	enum mtk_queue_attr q_attr;
	u32 hw_desc_base;
	u32 hw_int_mask;
	u32 desc_size;
	u16 q_size;
	u8 band_idx;
	char *const q_info;
};

enum ENUM_DMA_INT_TYPE {
	DMA_INT_TYPE_MCU2HOST,
	DMA_INT_TYPE_TRX,
	DMA_INT_TYPE_NUM
};

enum ENUM_WFDMA_RING_TYPE {
	TX_RING,
	RX_RING
};

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
union mawd_l2tbl {
	struct {
		uint8_t key_ip[16];
		uint8_t d_mac[MAC_ADDR_LEN];
		uint8_t s_mac[MAC_ADDR_LEN];
		uint32_t wlan_id:12;
		uint32_t bss_id:8;
		uint32_t reserved:12;
	} sram;

	uint32_t data[8];
};

struct RX_BLK_DESC {
	uint32_t addr;
	uint32_t addr_h:4;
	uint32_t msdu_cnt:11;
	uint32_t out_of_range:1;
	uint32_t ind_reason:4;
	uint32_t rsv:10;
	uint32_t magic_cnt:2;
};

struct RX_CTRL_BLK {
	phys_addr_t rPhyAddr;
	struct sk_buff *prSkb;
	uint32_t u4Idx;
	struct list_head rNode;
};

struct RCB_NODE {
	uint64_t u8Key;
	struct sk_buff *prSkb;
	struct RX_CTRL_BLK *prRcb;
	struct hlist_node rNode;
};

struct RRO_ADDR_ELEM_SINGLE {
	uint32_t addr;
	uint32_t addr_h:4;
	uint32_t msdu_cnt:11;
	uint32_t out_of_range:1;
	uint32_t rsv:13;
	uint32_t signature:3;
};

struct RRO_ADDR_ELEM {
	struct RRO_ADDR_ELEM_SINGLE elem0;
	struct RRO_ADDR_ELEM_SINGLE elem1;
};

struct RRO_IND_CMD {
	uint32_t session_id:16;
	uint32_t start_sn:12;
	uint32_t ind_reason:4;
	uint32_t ind_cnt:13;
	uint32_t win_sz:3;
	uint32_t rsv:13;
	uint32_t magic_cnt:3;
};

union RRO_ACK_SN_CMD {
	struct {
		uint32_t session_id:12;
		uint32_t rsv0:4;
		uint32_t ack_sn:12;
		uint32_t rsv1:3;
		uint32_t is_last:1;
	} field;

	uint32_t word;
};
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

enum pcie_msi_int_type {
	AP_INT,
	AP_MISC_INT,
	MDDP_INT,
	CCIF_INT,
	AP_DRV_OWN,
	NONE_INT
};

struct pcie_msi_layout {
	uint8_t name[32];
	irqreturn_t (*top_handler)(int irq, void *dev_instance);
	irqreturn_t (*thread_handler)(int irq, void *dev_instance);
	enum pcie_msi_int_type type;
	uint32_t irq_num;
};

struct pcie_msi_info {
	struct pcie_msi_layout *prMsiLayout;
	const uint32_t u4MaxMsiNum;
	u_int8_t fgMsiEnabled;
	uint32_t u4MsiNum;
	unsigned long ulEnBits;
};

enum pcie_msi_wfdma_ring {
	PCIE_MSI_TX_DATA_BAND0 = 0,
	PCIE_MSI_TX_DATA_BAND1,
	PCIE_MSI_TX_FREE_DONE,
	PCIE_MSI_RX_DATA_BAND0,
	PCIE_MSI_RX_DATA_BAND1,
	PCIE_MSI_EVENT,
	PCIE_MSI_CMD,
	PCIE_MSI_LUMP,
	PCIE_MSI_NUM
};

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
u_int8_t halIsDataRing(enum ENUM_WFDMA_RING_TYPE eType, uint32_t u4Idx);
void halHifRst(struct GLUE_INFO *prGlueInfo);
bool halWpdmaAllocRing(struct GLUE_INFO *prGlueInfo, bool fgAllocMem);
void halWpdmaFreeRing(struct GLUE_INFO *prGlueInfo);
void halWpdmaInitRing(struct GLUE_INFO *prGlueInfo, bool fgResetHif);
void halWpdmaInitTxRing(struct GLUE_INFO *prGlueInfo, bool fgResetHif);
void halWpdmaInitRxRing(struct GLUE_INFO *prGlueInfo);
uint8_t halSetRxRingHwAddr(
	struct RTMP_RX_RING *prRxRing,
	struct BUS_INFO *prBusInfo,
	uint32_t u4SwRingIdx);
void halWpdmaProcessCmdDmaDone(struct GLUE_INFO *prGlueInfo,
			       uint16_t u2Port);
void halWpdmaProcessDataDmaDone(struct GLUE_INFO *prGlueInfo,
				uint16_t u2Port);
u_int8_t halIsWfdmaRxRingReady(struct GLUE_INFO *prGlueInfo, uint8_t ucRingNum);
uint32_t halWpdmaGetRxDmaDoneCnt(struct GLUE_INFO *prGlueInfo,
				 uint8_t ucRingNum);
void halInitMsduTokenInfo(struct ADAPTER *prAdapter);
void halUninitMsduTokenInfo(struct ADAPTER *prAdapter);
uint32_t halGetMsduTokenFreeCnt(struct ADAPTER *prAdapter);
struct MSDU_TOKEN_ENTRY *halGetMsduTokenEntry(struct ADAPTER *prAdapter,
					      uint32_t u4TokenNum);
struct MSDU_TOKEN_ENTRY *halAcquireMsduToken(struct ADAPTER *prAdapter,
					     uint8_t ucBssIdx);
void halReturnMsduToken(struct ADAPTER *prAdapter, uint32_t u4TokenNum);
void halReturnTimeoutMsduToken(struct ADAPTER *prAdapter);
void halTxUpdateCutThroughDesc(struct GLUE_INFO *prGlueInfo,
			       struct MSDU_INFO *prMsduInfo,
			       struct MSDU_TOKEN_ENTRY *prFillToken,
			       struct MSDU_TOKEN_ENTRY *prDataToken,
			       uint32_t u4Idx, bool fgIsLast);
u_int8_t halChipToStaticMapBusAddr(struct mt66xx_chip_info *prChipInfo,
				   uint32_t u4ChipAddr,
				   uint32_t *pu4BusAddr);
u_int8_t halGetDynamicMapReg(struct GLUE_INFO *prGlueInfo,
			     uint32_t u4ChipAddr,
			     uint32_t *pu4Value);
u_int8_t halSetDynamicMapReg(struct GLUE_INFO *prGlueInfo,
			     uint32_t u4ChipAddr,
			     uint32_t u4Value);
void halConnacWpdmaConfig(struct GLUE_INFO *prGlueInfo, u_int8_t enable);
void halConnacEnableInterrupt(struct ADAPTER *prAdapter);
enum ENUM_CMD_TX_RESULT halWpdmaWriteCmd(struct GLUE_INFO *prGlueInfo,
		      struct CMD_INFO *prCmdInfo,
		      uint8_t ucTC);
bool halWpdmaWriteMsdu(struct GLUE_INFO *prGlueInfo,
		       struct MSDU_INFO *prMsduInfo,
		       struct list_head *prCurList);
bool halWpdmaWriteAmsdu(struct GLUE_INFO *prGlueInfo,
			struct list_head *prList,
			uint32_t u4Num, uint16_t u2Size);
void halWpdmaFreeMsdu(struct GLUE_INFO *prGlueInfo,
		      struct MSDU_INFO *prMsduInfo,
		      u_int8_t fgSetEvent,
		      struct QUE *prTxMsduRetQue);
u_int8_t halRxInsertRecvRfbList(
	struct ADAPTER *prAdapter,
	struct QUE *prReceivedRfbList,
	struct SW_RFB *prSwRfb);
void halWpdmaFreeMsduWork(struct GLUE_INFO *prGlueInfo);
#if CFG_SUPPORT_TASKLET_FREE_MSDU
void halWpdmaFreeMsduTasklet(unsigned long data);
#endif /* CFG_SUPPORT_TASKLET_FREE_MSDU */

#if CFG_TX_DIRECT_VIA_HIF_THREAD
#define KAL_HIF_TXDATAQ_LOCK_DECLARATION()
#define KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Port)
#define KAL_HIF_TXDATAQ_UNLOCK(prHifInfo, u4Port)
#else /* CFG_TX_DIRECT_VIA_HIF_THREAD */
#define KAL_HIF_TXDATAQ_LOCK_DECLARATION() \
	unsigned long __ulHifTxDataQFlags = 0

#define KAL_HIF_TXDATAQ_LOCK(prHifInfo, u4Port) \
	kalAcquireHifTxDataQLock(prHifInfo, u4Port, &__ulHifTxDataQFlags)

#define KAL_HIF_TXDATAQ_UNLOCK(prHifInfo, u4Port) \
	kalReleaseHifTxDataQLock(prHifInfo, u4Port, __ulHifTxDataQFlags)
#endif /* CFG_TX_DIRECT_VIA_HIF_THREAD */

#define KAL_HIF_TXRING_LOCK_DECLARATION() \
	unsigned long __ulHifTxRingFlags = 0

#define KAL_HIF_TXRING_LOCK(prTxRing) \
	kalAcquireHifTxRingLock(prTxRing, &__ulHifTxRingFlags)

#define KAL_HIF_TXRING_UNLOCK(prTxRing) \
	kalReleaseHifTxRingLock(prTxRing, __ulHifTxRingFlags)

#define KAL_HIF_BH_DISABLE(prGlueInfo) \
	kalBhDisable(prGlueInfo)

#define KAL_HIF_BH_ENABLE(prGlueInfo) \
	kalBhEnable(prGlueInfo)

#define KAL_HIF_OWN_LOCK(prAdapter) \
	kalAcquireHifOwnLock(prAdapter)

#define KAL_HIF_OWN_UNLOCK(prAdapter) \
	kalReleaseHifOwnLock(prAdapter)

void kalBhDisable(struct GLUE_INFO *prGlueInfo);
void kalBhEnable(struct GLUE_INFO *prGlueInfo);
void kalAcquireHifTxDataQLock(struct GL_HIF_INFO *prHifInfo,
		uint32_t u4Port,
		unsigned long *plHifTxDataQFlags);
void kalReleaseHifTxDataQLock(struct GL_HIF_INFO *prHifInfo,
		uint32_t u4Port,
		unsigned long ulHifTxDataQFlags);
void kalAcquireHifTxRingLock(struct RTMP_TX_RING *prTxRing,
		unsigned long *plHifTxRingFlags);
void kalReleaseHifTxRingLock(struct RTMP_TX_RING *prTxRing,
		unsigned long ulHifTxRingFlags);

void kalAcquireHifOwnLock(struct ADAPTER *prAdapter);
void kalReleaseHifOwnLock(struct ADAPTER *prAdapter);

bool kalDevReadData(struct GLUE_INFO *prGlueInfo, uint16_t u2Port,
		    struct SW_RFB *prSwRfb);
bool kalDevKickCmd(struct GLUE_INFO *prGlueInfo);

/* SER functions */
void halSetDrvSer(struct ADAPTER *prAdapter);
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
void halHwRecoveryTimeout(struct timer_list *timer);
#else
void halHwRecoveryTimeout(unsigned long arg);
#endif
void halHwRecoveryFromError(struct ADAPTER *prAdapter);
#if (CFG_SUPPORT_TX_DATA_DELAY == 1)
void halStartTxDelayTimer(struct ADAPTER *prAdapter);
#endif

/* Debug functions */
int halTimeCompare(struct timespec64 *prTs1, struct timespec64 *prTs2);
u_int8_t halGetDeltaTime(struct timespec64 *prTs1, struct timespec64 *prTs2,
			 struct timespec64 *prTsRst);
void halShowPdmaInfo(struct ADAPTER *prAdapter);
bool halShowHostCsrInfo(struct ADAPTER *prAdapter);
void kalDumpTxRing(struct GLUE_INFO *prGlueInfo,
		   struct RTMP_TX_RING *prTxRing,
		   uint32_t u4Num, bool fgDumpContent);
void kalDumpRxRing(struct GLUE_INFO *prGlueInfo,
		   struct RTMP_RX_RING *prRxRing,
		   uint32_t u4Num, bool fgDumpContent);
int wf_ioremap_read(phys_addr_t addr, unsigned int *val);
int wf_ioremap_write(phys_addr_t addr, unsigned int val);
void halEnableSlpProt(struct GLUE_INFO *prGlueInfo);
void halDisableSlpProt(struct GLUE_INFO *prGlueInfo);

void halSwWfdmaInit(struct GLUE_INFO *prGlueInfo);
void halSwWfdmaUninit(struct GLUE_INFO *prGlueInfo);
void halSwWfdmaEn(struct GLUE_INFO *prGlueInfo, bool fgEn);
void halSwWfdmaReset(struct SW_WFDMA_INFO *prSwWfdmaInfo);
void halSwWfdmaBackup(struct GLUE_INFO *prGlueInfo);
void halSwWfdmaRestore(struct GLUE_INFO *prGlueInfo);
void halSwWfdmaGetCidx(struct GLUE_INFO *prGlueInfo, uint32_t *pu4Cidx);
void halSwWfdmaSetCidx(struct GLUE_INFO *prGlueInfo, uint32_t u4Cidx);
void halSwWfdmaGetDidx(struct GLUE_INFO *prGlueInfo, uint32_t *pu4Didx);
bool halSwWfdmaWriteCmd(struct GLUE_INFO *prGlueInfo);
bool halSwWfdmaProcessDmaDone(struct GLUE_INFO *prGlueInfo);
void halSwWfdmaDumpDebugLog(struct GLUE_INFO *prGlueInfo);

void halSwEmiInit(struct GLUE_INFO *prGlueInfo);
u_int8_t halSwEmiRead(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
		      uint32_t *pu4Val);
void halSwEmiDebug(struct GLUE_INFO *prGlueInfo);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
/* Host Offload */
void halRroTurnOff(struct GLUE_INFO *prGlueInfo);
void halRroInit(struct GLUE_INFO *prGlueInfo);
void halRroUninit(struct GLUE_INFO *prGlueInfo);
void halOffloadAllocMem(struct GLUE_INFO *prGlueInfo);
void halOffloadFreeMem(struct GLUE_INFO *prGlueInfo);
void halRroAllocMem(struct GLUE_INFO *prGlueInfo);
void halRroResetMem(struct GLUE_INFO *prGlueInfo);
void halRroAllocRcbList(struct GLUE_INFO *prGlueInfo);
void halRroFreeRcbList(struct GLUE_INFO *prGlueInfo);
void halRroResetRcbList(struct GLUE_INFO *prGlueInfo);
void halRroReadRxData(struct ADAPTER *prAdapter);
void halRroUpdateWfdmaRxBlk(struct GLUE_INFO *prGlueInfo,
			    uint16_t u2Port, uint32_t u4ResCnt);
struct RX_CTRL_BLK *halRroGetFreeRcbBlk(
	struct GL_HIF_INFO *prHifInfo,
	struct RTMP_DMABUF *pDmaBuf,
	uint32_t u4Idx);
void halRroMawdInit(struct GLUE_INFO *prGlueInfo);
int halMawdPwrOn(void);
void halMawdPwrOff(void);
u_int8_t halMawdCheckInfra(struct ADAPTER *prAdapter);
u_int8_t halMawdAllocTxRing(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem);
void halMawdAllocRxBlkRing(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem);
void halMawdInitRxBlkRing(struct GLUE_INFO *prGlueInfo);
void halMawdInitTxRing(struct GLUE_INFO *prGlueInfo);
u_int8_t halMawdFillTxRing(struct GLUE_INFO *prGlueInfo,
		       struct MSDU_TOKEN_ENTRY *prToken);
uint32_t halMawdGetRxBlkDoneCnt(struct GLUE_INFO *prGlueInfo);
u_int8_t halMawdWakeup(struct GLUE_INFO *prGlueInfo);
u_int8_t halMawdSleep(struct GLUE_INFO *prGlueInfo);
void halMawdReset(struct GLUE_INFO *prGlueInfo);
void halMawdUpdateL2Tbl(struct GLUE_INFO *prGlueInfo,
			union mawd_l2tbl rL2Tbl, uint32_t u4Set);
#else
static inline int halMawdPwrOn(void) { return 0; }
static inline void halMawdPwrOff(void) {}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

int halInitResvMem(struct platform_device *pdev);
int halAllocHifMem(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData);
void halFreeHifMem(struct platform_device *pdev);

void halCopyPathAllocTxDesc(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
void halCopyPathAllocRxDesc(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
void halCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing);
bool halCopyPathAllocTxCmdBuf(struct RTMP_DMABUF *prDmaBuf,
			      uint32_t u4Num, uint32_t u4Idx);
void halCopyPathAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken,
			       uint32_t u4Idx);
void *halCopyPathAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx);
bool halCopyPathCopyCmd(struct GL_HIF_INFO *prHifInfo,
			struct RTMP_DMACB *prTxCell, void *pucBuf,
			void *pucSrc1, uint32_t u4SrcLen1,
			void *pucSrc2, uint32_t u4SrcLen2);
bool halCopyPathCopyEvent(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RXD_STRUCT *pRxD,
			  struct RTMP_DMABUF *prDmaBuf,
			  uint8_t *pucDst, uint32_t u4Len);
bool halCopyPathCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			   void *pucSrc, uint32_t u4Len);
bool halCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RTMP_DMABUF *prDmaBuf,
			   struct SW_RFB *prSwRfb);
void halCopyPathFreeExtBuf(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing);
void halCopyPathDumpTx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
void halCopyPathDumpRx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
void halZeroCopyPathAllocDesc(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMABUF *prDescRing,
			  uint32_t u4Num);
void halZeroCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing);
void *halZeroCopyPathAllocRxBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDmaBuf,
			    uint32_t u4Num, uint32_t u4Idx);
void halZeroCopyPathAllocTxDataBuf(struct MSDU_TOKEN_ENTRY *prToken,
			       uint32_t u4Idx);
void *halZeroCopyPathAllocRuntimeMem(uint32_t u4SrcLen);
bool halZeroCopyPathCopyCmd(struct GL_HIF_INFO *prHifInfo,
			struct RTMP_DMACB *prTxCell, void *pucBuf,
			void *pucSrc1, uint32_t u4SrcLen1,
			void *pucSrc2, uint32_t u4SrcLen2);
bool halZeroCopyPathCopyEvent(struct GL_HIF_INFO *prHifInfo,
			  struct RTMP_DMACB *pRxCell,
			  struct RXD_STRUCT *pRxD,
			  struct RTMP_DMABUF *prDmaBuf,
			  uint8_t *pucDst, uint32_t u4Len);
bool halZeroCopyPathCopyTxData(struct MSDU_TOKEN_ENTRY *prToken,
			   void *pucSrc, uint32_t u4Len);
bool halZeroCopyPathCopyRxData(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMACB *pRxCell,
			   struct RTMP_DMABUF *prDmaBuf,
			   struct SW_RFB *prSwRfb);
phys_addr_t halZeroCopyPathMapTxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
phys_addr_t halZeroCopyPathMapRxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
void halZeroCopyPathUnmapTxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
void halZeroCopyPathUnmapRxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
void halZeroCopyPathFreeDesc(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMABUF *prDescRing);
void halZeroCopyPathFreeBuf(void *pucSrc, uint32_t u4Len);
void halZeroCopyPathFreePacket(struct GL_HIF_INFO *prHifInfo,
			   void *pvPacket, uint32_t u4Num);
void halZeroCopyPathDumpTx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
void halZeroCopyPathDumpRx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);

#if CFG_MTK_WIFI_SW_EMI_RING
struct HIF_MEM *halGetRsvEmi(struct GL_HIF_INFO *prHifInfo);
#endif

#if CFG_SUPPORT_RX_PAGE_POOL
void halZeroCopyPathFreePagePoolPacket(struct GL_HIF_INFO *prHifInfo,
				       void *pvPacket, uint32_t u4Num);
void *halZeroCopyPathAllocPagePoolRxBuf(struct GL_HIF_INFO *prHifInfo,
					struct RTMP_DMABUF *prDmaBuf,
					uint32_t u4Num, uint32_t u4Idx);

void kalSkbMarkForRecycle(struct sk_buff *pkt);
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
void kalSetupPagePoolPageMaxMinNum(uint32_t u4Min, uint32_t u4Max);
uint32_t kalGetPagePoolPageNum(void);
u_int8_t kalSetPagePoolPageMaxNum(void);
u_int8_t kalIncPagePoolPageNum(void);
u_int8_t kalDecPagePoolPageNum(void);
u_int8_t kalSetPagePoolPageNum(uint32_t u4Num);
#endif
struct sk_buff *kalAllocRxSkb(uint8_t **ppucData);
u_int8_t kalCreateHifSkbList(struct mt66xx_chip_info *prChipInfo);
void kalReleaseHifSkbList(void);
struct sk_buff *kalAllocHifSkb(void);
void kalFreeHifSkb(struct sk_buff *prSkb);

extern struct page *wifi_page_pool_alloc_page(void) __attribute__((weak));
extern void wifi_page_pool_set_page_num(uint32_t num) __attribute__((weak));
extern uint32_t wifi_page_pool_get_page_num(void) __attribute__((weak));
extern uint32_t wifi_page_pool_get_max_page_num(void) __attribute__((weak));
#endif
void halWpdmaStopRecycleDmad(struct GLUE_INFO *prGlueInfo,
				       uint16_t u2Port);


#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
int32_t wf_reg_read_wrapper(void *priv,
	uint32_t addr, uint32_t *value);
int32_t wf_reg_write_wrapper(void *priv,
	uint32_t addr, uint32_t value);
int32_t wf_reg_write_mask_wrapper(void *priv,
	uint32_t addr, uint32_t mask, uint32_t value);
int32_t wf_reg_start_wrapper(enum connv3_drv_type from_drv,
	void *priv_data);
int32_t wf_reg_end_wrapper(enum connv3_drv_type from_drv,
	void *priv_data);
#endif
#endif /* HIF_PDMA_H__ */
