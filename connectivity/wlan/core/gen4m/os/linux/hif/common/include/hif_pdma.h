/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*
 Module Name:
 hif_pdma.h
 */

#ifndef __HIF_PDMA_H__
#define __HIF_PDMA_H__

#include <linux/list_sort.h>
#include <linux/hashtable.h>
#include "mt66xx_reg.h"

#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#include "connv3.h"
#endif

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#if (CFG_SUPPORT_CONNAC1X == 1)
#define NUM_OF_WFDMA1_TX_RING			0
#define NUM_OF_WFDMA1_RX_RING			0
#else
#ifdef CONFIG_NUM_OF_WFDMA_TX_RING
#define NUM_OF_WFDMA1_TX_RING			(CONFIG_NUM_OF_WFDMA_TX_RING)
#else
#define NUM_OF_WFDMA1_TX_RING			1  /* WA CMD Ring */
#endif

#ifdef CONFIG_NUM_OF_WFDMA_RX_RING
#define NUM_OF_WFDMA1_RX_RING			(CONFIG_NUM_OF_WFDMA_RX_RING)
#else
#define NUM_OF_WFDMA1_RX_RING			5
#endif
#endif /* CFG_SUPPORT_CONNAC1X == 1 */

/*
 * 6 data ring:
 * 1. ring0 (AC00~AC02)
 * 2. ring1 (AC10~AC12)
 * 3. ring2 (AC20~AC22)
 * 4. ring3 (AC30~AC32)
 * 5. ring4 (AC03~AC33) (priority)
 * 6. ring5 ALTX
 * fwdl ring
 * cmd ring
 */
#define NUM_OF_TX_RING				(8 + NUM_OF_WFDMA1_TX_RING)
#define NUM_OF_RX_RING				(2 + NUM_OF_WFDMA1_RX_RING)

#define RX_RING_MAX_SIZE			4095

#ifdef CFG_NUM_OF_TX_CMD_RING_SIZE
#define TX_RING_CMD_SIZE			(CFG_NUM_OF_TX_CMD_RING_SIZE)
#else
#if defined(CONFIG_MTK_WIFI_BW320)
#define TX_RING_CMD_SIZE			320
#else
#define TX_RING_CMD_SIZE			256
#endif
#endif /* CFG_NUM_OF_TX_CMD_RING_SIZE */

#if defined(CONFIG_MTK_WIFI_BW320)
#ifdef BELLWETHER
#define TX_RING_SIZE				1024
#else
#define TX_RING_SIZE				3072
#endif
#define TX_RING_DATA_SIZE			TX_RING_SIZE

#define CMA_MEM_MAX_SIZE			128

#define HIF_NUM_OF_QM_RX_PKT_NUM		10240
#define HIF_PLE_PAGE_SIZE			0xBC0
#define HIF_AMSDU_COUNT				4
#if defined(MT6653) || defined(MT7990)
#define HIF_TX_MSDU_TOKEN_NUM			28000
#else
#define HIF_TX_MSDU_TOKEN_NUM \
	(HIF_PLE_PAGE_SIZE * HIF_AMSDU_COUNT)
#endif
#define HIF_TX_MSDU_TOKEN_NUM_MIN	(1024 * 7)
/* ToDo fine tune for owl EHT160 */
#elif defined(CONFIG_MTK_WIFI_HE160) || defined(CONFIG_MTK_WIFI_EHT160)
#define TX_RING_SIZE				1024
#define TX_RING_DATA_SIZE			1024
#if defined(MT7925)
#define HIF_TX_MSDU_TOKEN_NUM			8064
#else
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 4)
#endif
#define HIF_NUM_OF_QM_RX_PKT_NUM		4096



#elif defined(CONFIG_MTK_WIFI_HE80)
#define TX_RING_SIZE				1024
#define TX_RING_DATA_SIZE			1024
#define HIF_NUM_OF_QM_RX_PKT_NUM		2048
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 2)

#elif defined(CONFIG_MTK_WIFI_VHT80)
#define TX_RING_SIZE				512
#define TX_RING_DATA_SIZE			512
#define HIF_NUM_OF_QM_RX_PKT_NUM		2048
#define HIF_TX_MSDU_TOKEN_NUM			(TX_RING_DATA_SIZE * 3)

#else
#define TX_RING_SIZE				256
#define TX_RING_DATA_SIZE			256
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
#define HIF_TX_RESOURCE_CTRL                CFG_SUPPORT_HIF_TX_RESOURCE_CTRL
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
#define HIF_SER_TIMEOUT				1000	/* msec */
#define HIF_SER_MAX_TIMEOUT_CNT			10	/* msec */
#define HIF_SER_POWER_OFF_RETRY_COUNT		100
#define HIF_SER_POWER_OFF_RETRY_TIME		10	/* msec */
#define HIF_CMD_POWER_OFF_RETRY_COUNT		100
#define HIF_CMD_POWER_OFF_RETRY_TIME		10	/* msec */

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

#define SW_EMI_RING_SIZE		16

#define MSDU_REPORT_MAX_NUM		336

#define MSDU_TOKEN_HISTORY_NUM		5

#define LOG_DUMP_COUNT_PERIOD		5
#define LOG_DUMP_FULL_DUMP_TIMES	2

#define WFDMA_MAGIC_CNT_NUM      16
#define INDCMD_MAGIC_CNT_NUM     8
#define RX_BLK_MAGIC_CNT_NUM     4
#define MAWD_ENABLE_WAKEUP_SLEEP 1
#define MAWD_POWER_UP_RETRY_CNT  50000
#define MAWD_POWER_UP_WAIT_TIME  10
#define MAWD_MAX_PATCH_NUM       19
#define MAWD_MD_TX_RING_NUM      3

#if CFG_ENABLE_MAWD_MD_RING
#define MAWD_TX_RING_OFFSET      2
#define MAWD_NUM_OF_RX_BLK_RING  2
#else
#define MAWD_NUM_OF_RX_BLK_RING  1
#endif /* CFG_ENABLE_MAWD_MD_TX_RING */

#define RRO_HASH_TABLE_SIZE      (RX_RING_MAX_SIZE * 3)
#define RRO_BA_BITMAP_SIZE       128
#if CFG_MTK_FPGA_PLATFORM
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

#define WFDMA_MEMORY_ALIGNMENT      8
#define WFDMA_RXDMAD_SDP_MEMORY_ALIGNMENT  32
#define WFDMA_WB_MEMORY_ALIGNMENT   256
#define WFDMA_WB_MEMORY_SIZE   256

#define WFDMA_TX_RING_MAX_NUM    (CFG_NUM_OF_WFDMA_WB_TX_RING)
#define WFDMA_RX_RING_MAX_NUM    16

#define HIF_INT_TIME_DEBUG              0

#define FW_BIN_FLAVOR_KEY		"flavor-bin"
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
#if CFG_TC10_FEATURE
#define DEFAULT_MEMDUMP_KEY		"default-memdump"
#endif
#endif

#define TX_MSDU_MEM_ALLOC_MAX_TIME	3000

#if CFG_NEW_HIF_DEV_REG_IF
#define HIF_DEV_REG_HISTORY_SIZE    100
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if CFG_SUPPORT_HIF_REG_WORK
#define HIF_REG_WORK_WAIT_TIME	100  /* 100us */
#define HIF_REG_WORK_WAIT_CNT	10000
#endif /* CFG_SUPPORT_HIF_REG_WORK */

#define HIF_EMI_SER_STATUS_SIZE		16

#define HIF_RX_DMA_DONE_MAX_FAIL_CNT	3

enum WIFI_MEM_OPER_SETS {
	/* TRX DESC */
	WF_MEM_OP_TRX_DESC_ZERO_COPY_PATH = 0,
	WF_MEM_OP_TRX_DESC_COPY_PATH,

	/* TX DATA */
	WF_MEM_OP_TX_DATA_ZERO_COPY_PATH,
	WF_MEM_OP_TX_DATA_COPY_PATH,
	WF_MEM_OP_TX_DATA_COPY_PATH_TX_DYN_CMA,
	WF_MEM_OP_TX_DATA_COPY_PATH_TX_NON_CACHE,

	/* TX CMD */
	WF_MEM_OP_TX_CMD_ZERO_COPY_PATH,
	WF_MEM_OP_TX_CMD_COPY_PATH,

	/* RX DATA */
	WF_MEM_OP_RX_DATA_ZERO_COPY_PATH,
	WF_MEM_OP_RX_DATA_COPY_PATH,
	WF_MEM_OP_RX_DATA_ZERO_COPY_PATH_PAGE_POOL,

	/* RX EVT */
	WF_MEM_OP_RX_EVT_ZERO_COPY_PATH,
	WF_MEM_OP_RX_EVT_COPY_PATH,

	/* TX DUMP */
	WF_MEM_OP_TX_DUMP_ZERO_COPY_PATH,
	WF_MEM_OP_TX_DUMP_COPY_PATH,
	WF_MEM_OP_TX_DUMP_NULL,

	/* RX DUMP */
	WF_MEM_OP_RX_DUMP_ZERO_COPY_PATH,
	WF_MEM_OP_RX_DUMP_COPY_PATH,
	WF_MEM_OP_RX_DUMP_NULL,

	/* EXT_BUF */
	WF_MEM_OP_EXT_BUF_ZERO_COPY_PATH,
	WF_MEM_OP_EXT_BUF_COPY_PATH,
	WF_MEM_OP_EXT_BUF_NULL,

	/* RUNTIME_MEM */
	WF_MEM_OP_RUNTIME_MEM_ZERO_COPY_PATH,
	WF_MEM_OP_RUNTIME_MEM_NULL,

	/* WIFI_MISC */
	WF_MEM_OP_WIFI_MISC_EMI_ENABLE,
	WF_MEM_OP_WIFI_MISC_EMI_NULL,
};

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

#define INC_RING_INDEX(_idx, _RingSize)		\
{ \
	(_idx) = (_idx+1) % (_RingSize); \
	KAL_MB_W(); \
}

#define DEC_RING_INDEX(_idx, _RingSize)		\
{ \
	(_idx) = (_idx == 0) ? (_RingSize - 1) : (_idx - 1); \
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

#if CFG_MTK_WIFI_WFDMA_WB
#define HAL_GET_RING_DIDX(_RSN, _A, _R, _V)	\
do { \
	if (_R->fgEnEmiDidx) { \
		*_V = (*_R->pu2EmiDidx & _R->hw_didx_mask); \
	} else { \
		HAL_RMCR_RD(_RSN, _A, _R->hw_didx_addr, _V); \
		*_V = (*_V & _R->hw_didx_mask) >> _R->hw_didx_shift; \
	} \
} while (0)

#define HAL_SET_RING_CIDX(_A, _R, _V) \
{ \
	if (_R->fgEnEmiCidx) { \
		_R->u4LastCidx = *_R->pu2EmiCidx; \
		_R->u4LastDidx = *_R->pu2EmiDidx; \
		*_R->pu2EmiCidx = _V; \
		if (_R->triggerCidx) \
			_R->triggerCidx(_A->prGlueInfo, _R); \
	} else { \
		HAL_MCR_WR(_A, _R->hw_cidx_addr, _V << _R->hw_cidx_shift); \
	} \
}

#define HAL_GET_RING_CIDX(_RSN, _A, _R, _V)	\
do { \
	if (_R->fgEnEmiCidx) { \
		*_V = *_R->pu2EmiCidx; \
	} else { \
		HAL_RMCR_RD(_RSN, _A, _R->hw_cidx_addr, _V); \
		*_V = (*_V & _R->hw_cidx_mask) >> _R->hw_cidx_shift; \
	} \
} while (0)
#else
#define HAL_GET_RING_DIDX(_RSN, _A, _R, _V) \
do { \
	HAL_RMCR_RD(_RSN, _A, _R->hw_didx_addr, _V); \
	*_V = (*_V & _R->hw_didx_mask) >> _R->hw_didx_shift; \
} while (0)

#define HAL_SET_RING_CIDX(_A, _R, _V) \
{ \
	HAL_MCR_WR(_A, _R->hw_cidx_addr, _V << _R->hw_cidx_shift);	\
}

#define HAL_GET_RING_CIDX(_RSN, _A, _R, _V)	\
do { \
	HAL_RMCR_RD(_RSN, _A, _R->hw_cidx_addr, _V); \
	*_V = (*_V & _R->hw_cidx_mask) >> _R->hw_cidx_shift; \
} while (0)
#endif /* CFG_ENABLE_MAWD_MD_RING */

#define HAL_GET_RING_MCNT(_RSN, _A, _R, _V) \
do { \
	HAL_RMCR_RD(_RSN, _A, _R->hw_cnt_addr, _V); \
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

enum ENUM_WIFI_RSV_MEM_IDX {
	WIFI_RSV_MEM_WFDMA = 0,
	WIFI_RSV_MEM_WIFI_MISC,
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
	WIFI_RSV_MEM_WIFI_CMA_NON_CACHE,
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
	WIFI_RSV_MEM_MAX_NUM
};

enum WIFI_MISC_MEM_BLOCK_NAME {
	WIFI_MISC_MEM_BLOCK_NON_MMIO = 0,
	WIFI_MISC_MEM_BLOCK_TX_POWER_LIMIT,
	WIFI_MISC_MEM_BLOCK_TX_POWER_STATUS,
	WIFI_MISC_MEM_BLOCK_SER_STATUS,
	WIFI_MISC_MEM_BLOCK_SCREEN_STATUS,
	WIFI_MISC_MEM_BLOCK_WF_M_BRAIN,
	WIFI_MISC_MEM_BLOCK_WF_GEN_SWITCH,
	WIFI_MISC_MEM_BLOCK_WF_RSVED,
	WIFI_MISC_MEM_BLOCK_PRECAL,
	WIFI_MISC_MEM_BLOCK_MULTIBAND_TX_POWER_LIMIT,
	WIFI_MISC_MEM_BLOCK_PERF_IND,
	WIFI_MISC_MEM_BLOCK_MAX_NUM
};

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

struct HIF_MEM {
	phys_addr_t pa;
	void *va;
	uint32_t align_size;
};

/*
 *	Data buffer for DMA operation, the buffer must be contiguous
 *	physical memory Both DMA to / from CPU use the same structure.
 */
struct RTMP_DMABUF {
	unsigned long AllocSize;
	void *AllocVa;		/* TxBuf virtual address */
	phys_addr_t AllocPa;		/* TxBuf physical address */
	struct HIF_MEM rMem;
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
	uint32_t TxCpuIdxRec;
	uint32_t TxDmaIdxRec;
	uint32_t u4BufSize;
	uint32_t u4RingSize;
	uint32_t u4RingIdx;
	uint32_t TxSwUsedIdx;
	uint32_t u4UsedCnt;
	uint32_t u4TotalCnt;
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
#if CFG_MTK_WIFI_WFDMA_WB
	u_int8_t fgEnEmiDidx;
	u_int8_t fgEnEmiCidx;
	uint16_t *pu2EmiDidx;
	uint32_t *pu2EmiCidx;
	uint32_t u4LastCidx;
	uint32_t u4LastDidx;
	void (*triggerCidx)(struct GLUE_INFO *prGlueInfo,
			    struct RTMP_TX_RING *prTxRing);
#endif /* CFG_ENABLE_MAWD_MD_RING */
};

struct RTMP_RX_RING {
	struct RTMP_DMACB Cell[RX_RING_MAX_SIZE];
	uint32_t RxCpuIdx;
	uint32_t RxDmaIdx;
	uint32_t u4BufSize;
	uint32_t u4RingSize;
	uint32_t u4RingIdx;
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
	uint32_t u4RxDmaDoneFailCnt;
	uint32_t u4LastRxEventWaitDmaDoneCnt;
	uint32_t u4PendingCnt;
	uint32_t u4TotalCnt;
#if (CFG_SUPPORT_PDMA_SCATTER == 1)
	void *pvSegPkt;
	uint32_t u4SegPktLen;
	uint32_t u4SegPktLenMax;
	uint32_t u4SegPktIdx;
	uint32_t u4SegPktIdxMax;
#endif
	uint32_t u4MagicCnt;
#if CFG_MTK_WIFI_WFDMA_WB
	u_int8_t fgEnEmiDidx;
	u_int8_t fgEnEmiCidx;
	uint16_t *pu2EmiDidx;
	uint32_t *pu2EmiCidx;
	uint32_t u4LastCidx;
	uint32_t u4LastDidx;
	void (*triggerCidx)(struct GLUE_INFO *prGlueInfo,
			    struct RTMP_RX_RING *prRxRing);
#endif /* CFG_ENABLE_MAWD_MD_RING */
	uint32_t u4CidxRec;
	uint32_t u4CidxErrCnt;
	uint32_t u4TotalCntRec;
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
	uint32_t def_val;
};

struct ap2wf_remap {
	uint32_t reg_base;
	uint32_t reg_mask;
	uint32_t reg_shift;
	uint32_t base_addr;
};

struct remap_range {
	const uint32_t start;
	const uint32_t end;
};

struct PCIE_CHIP_CR_REMAPPING {
	const struct pcie2ap_remap *pcie2ap;
	const struct pcie2ap_remap *pcie2ap_cbtop;
	const struct ap2wf_remap *ap2wf;
	const struct remap_range *cbtop_ranges;
};

struct MSDU_TOKEN_ENTRY {
	uint32_t u4Token;
	u_int8_t fgInUsed;
	uint64_t u8Tm;
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
	uint32_t key;
	struct hlist_node node; /* htbl node */
	struct list_head msdu_list;
	/* Log info for key TX packets */
	u_int8_t fgTxDoneHandler;
	uint8_t ucTxSeqNum;
	uint8_t ucPktType;
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
	spinlock_t rTokenLock;
	struct MSDU_TOKEN_ENTRY arToken[HIF_TX_MSDU_TOKEN_NUM];
	uint32_t u4TokenNum;
	uint32_t u4FifoErrCnt;
#if CFG_SUPPORT_HIF_FIFO_TOKEN
	struct kfifo rTokenFifo;
	void **aucTokenFifoBuf;
	uint32_t u4TokenFifoLen;
#else
	struct list_head used_msdu_list; /* msdu wait tx done */
	struct hlist_head used_msdu_htbl[HIF_TX_MSDU_TOKEN_NUM];
#endif
	struct list_head init_msdu_list; /* msdu w/o data */
	struct list_head free_msdu_list; /* msdu w/ data */

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
	uint32_t u4PktCnt;
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
	uint32_t u4TimeoutCnt;
};

struct SER_EMI_STATUS {
	uint8_t ucStatus[HIF_EMI_SER_STATUS_SIZE];
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

#if CFG_MTK_WIFI_MBU
struct MBU_MSI_MIRROR {
	uint32_t u4IntSta;
	uint32_t au4SidebandSignal[2];
	uint32_t au4Rsv;
};
struct MBU_EMI_CTX {
	uint32_t u4LowVal;
	uint32_t u4HighVal;
	uint32_t au4Rsv[2];
	struct MBU_MSI_MIRROR arMsiMirror[8];
};
#endif /* CFG_MTK_WIFI_MBU */

#if CFG_MTK_WIFI_SW_EMI_RING
struct SW_EMI_RING_OPS {
	void (*init)(struct GLUE_INFO *prGlueInfo);
	void (*uninit)(struct GLUE_INFO *prGlueInfo);
	u_int8_t (*read)(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
			 uint32_t *pu4Val);
	u_int8_t (*read8)(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
			  uint32_t *pu4LowVal, uint32_t *pu4HighVal);
	void (*triggerInt)(struct GLUE_INFO *prGlueInfo);
	void (*enableDebug)(struct GLUE_INFO *prGlueInfo);
	void (*debug)(struct GLUE_INFO *prGlueInfo);
	void (*dumpDebugCr)(struct GLUE_INFO *prGlueInfo);
};

struct SW_EMI_RING_INFO {
	struct SW_EMI_RING_OPS rOps;
	struct SW_EMI_CTX *prEmi;
	u_int8_t fgIsSupport;
	u_int8_t fgIsEnable;
	uint32_t u4CcifTchnumAddr;
	uint32_t u4CcifChlNum;
	uint32_t u4ReadBlockCnt;
#if CFG_MTK_WIFI_MBU
	struct MBU_EMI_CTX *prMbuEmiData;
	uint32_t u4RemapAddr;
	uint32_t u4RemapVal;
	uint32_t u4RemapDefVal;
	uint32_t u4RemapRegAddr;
	uint32_t u4RemapBusAddr;
	uint32_t u4TimeoutCnt;
	uint32_t u4DumpDbgCrCnt;
#endif
};
#endif /* CFG_MTK_WIFI_SW_EMI_RING */

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

enum ENUM_RX_SEGMENT_TYPE {
	RX_SEGMENT_NONE = 0,
	RX_SEGMENT_FIRST,
	RX_SEGMENT_MIDDLE,
	RX_SEGMENT_LAST
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
	PCIE_GEN_SWITCH_INT,
	NONE_INT
};

struct pcie_msi_layout {
	uint8_t name[32];
	irqreturn_t (*top_handler)(int irq, void *dev_instance);
	irqreturn_t (*thread_handler)(int irq, void *dev_instance);
	enum pcie_msi_int_type type;
	int32_t irq_num;
};

struct pcie_msi_info {
	struct pcie_msi_layout *prMsiLayout;
	const uint32_t u4MaxMsiNum;
	u_int8_t fgMsiEnabled;
	uint32_t u4MsiNum;
	unsigned long ulEnBits;
#if CFG_SUPPORT_WED_PROXY
	unsigned long address_lo;
	unsigned long address_hi;
#endif
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

#if CFG_MTK_WIFI_WFDMA_WB
struct WFDMA_EMI_RING_IDX_0 {
	uint16_t u2TxRing[11];
	uint16_t u2RxRing[5];
};

struct WFDMA_EMI_RING_IDX_1 {
	uint16_t u2TxRing[8];
	uint16_t u2RxRing[5];
	uint16_t Rsv[3];
};

struct WFDMA_EMI_DONE_FLAG {
	uint32_t tx_int0;
	uint32_t tx_int1;
	uint32_t rx_int0;
	uint32_t rx_int1;
	uint32_t err_int;
	uint32_t sw_int;
	uint32_t subsys_int;
	uint32_t rro;
#if (CFG_SUPPORT_WFDMA_WB_INT_MASK == 1)
	uint32_t tx_int0_mask;
	uint32_t tx_int1_mask;
	uint32_t rx_int0_mask;
	uint32_t rx_int1_mask;
	uint32_t err_int_mask;
	uint32_t sw_int_mask;
	uint32_t subsys_int_mask;
	uint32_t rro_mask;
#endif /* CFG_SUPPORT_WFDMA_WB_INT_MASK */
};

struct WFDMA_EMI_RING_DIDX {
	uint16_t tx_ring[WFDMA_TX_RING_MAX_NUM];
	uint16_t rx_ring[WFDMA_RX_RING_MAX_NUM];
};

struct WFDMA_EMI_RING_CIDX {
	uint32_t tx_ring[WFDMA_TX_RING_MAX_NUM];
	uint32_t rx_ring[WFDMA_RX_RING_MAX_NUM];
};
#endif /* CFG_MTK_WIFI_WFDMA_WB */

struct EMI_WIFI_MISC_RSV_MEM_INFO {
	const enum WIFI_MISC_MEM_BLOCK_NAME block_name;
	uint32_t size;
	struct HIF_MEM rRsvEmiMem;
};

#if CFG_NEW_HIF_DEV_REG_IF
struct HIF_DEV_REG_RECORD {
	enum HIF_DEV_REG_REASON eReason;
	uint32_t u4Reg;
	uint32_t u4Mod;
};
#endif /* CFG_NEW_HIF_DEV_REG_IF */

#if CFG_SUPPORT_HIF_REG_WORK
enum WF_REG_REQ_OP {
	WF_REG_READ = 0,
	WF_REG_WRITE,
	WF_REG_NUM
};

enum WF_REG_REQ_STATUS {
	WF_REG_PENDING = 0,
	WF_REG_SUCCESS,
	WF_REG_FAILURE,
	WF_REG_DROP,
	WF_REG_STATUS_NUM
};

struct WF_REG_REQ {
	enum WF_REG_REQ_OP eOp;
	uint32_t u4Addr;
	uint32_t u4Val;
	enum WF_REG_REQ_STATUS eStatus;
};
#endif /* CFG_SUPPORT_HIF_REG_WORK */

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
uint32_t halWpdmaGetTxDmaDoneCnt(struct GLUE_INFO *prGlueInfo,
				 uint8_t ucRingNum);
void halWpdmaProcessCmdDmaDone(struct GLUE_INFO *prGlueInfo,
			       uint16_t u2Port);
void halWpdmaProcessDataDmaDone(struct GLUE_INFO *prGlueInfo,
				uint16_t u2Port);
u_int8_t halIsWfdmaRxRingReady(struct GLUE_INFO *prGlueInfo, uint8_t ucRingNum);
u_int8_t halIsWfdmaRxRingsEmpty(struct GLUE_INFO *prGlueInfo);
uint32_t halWpdmaGetRxDmaDoneCnt(struct GLUE_INFO *prGlueInfo,
				 uint8_t ucRingNum);
uint32_t halGetWfdmaRxCnt(struct ADAPTER *prAdapter);
bool halInitOneMsduTokenInfo(struct ADAPTER *prAdapter,
	struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx);
void halUninitOneMsduTokenInfo(struct ADAPTER *prAdapter,
	struct MSDU_TOKEN_ENTRY *prToken);
u_int8_t halInitMsduTokenInfo(struct ADAPTER *prAdapter);
void halUninitMsduTokenInfo(struct ADAPTER *prAdapter);
uint32_t halGetMsduTokenFreeCnt(struct ADAPTER *prAdapter);
struct MSDU_TOKEN_ENTRY *halGetMsduTokenEntry(struct ADAPTER *prAdapter,
					      uint32_t u4TokenNum);
struct MSDU_TOKEN_ENTRY *halAcquireMsduToken(struct ADAPTER *prAdapter,
		uint8_t ucBssIdx, struct MSDU_INFO *prMsduInfo);
void halReturnMsduToken(struct ADAPTER *prAdapter, uint32_t u4TokenNum);
u_int8_t halHandleAllTokensUnused(
	struct ADAPTER *prAdapter, u_int8_t fgIsCheck);
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

#define KAL_HIF_OWN_TRYLOCK(prAdapter) \
	kalHifOwnTryLock(prAdapter)

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
int kalHifOwnTryLock(struct ADAPTER *prAdapter);

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
void halCancleTxDelayTimer(struct ADAPTER *prAdapter);
u_int8_t halCheckAndStartTxDelayTimer(struct ADAPTER *prAdapter);
#endif
#if CFG_SUPPORT_HIF_RX_NAPI
uint32_t halGetTxMsduCnt(struct ADAPTER *prAdapter);
uint32_t halIsTxMsduWithTxDoneCb(struct ADAPTER *prAdapter);
#endif /* CFG_SUPPORT_HIF_RX_NAPI */

u_int8_t halIsWfdmaRxCidxChanged(struct ADAPTER *prAdapter, uint32_t u4Idx);
void halDetectHifStall(struct ADAPTER *prAdapter);

/* Debug functions */
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
#if CFG_MTK_WIFI_SW_EMI_RING
void halSwEmiInit(struct GLUE_INFO *prGlueInfo);
u_int8_t halSwEmiRead(struct GLUE_INFO *prGlueInfo, uint32_t u4Addr,
		      uint32_t *pu4Val);
void halSwEmiDebug(struct GLUE_INFO *prGlueInfo);
#endif
#if CFG_MTK_WIFI_MBU
void halMbuInit(struct GLUE_INFO *prGlueInfo);
void halMbuUninit(struct GLUE_INFO *prGlueInfo);
u_int8_t halMbuRead4(struct GLUE_INFO *prGlueInfo, uint32_t u4ReadAddr,
		     uint32_t *pu4Val);
u_int8_t halMbuRead8(struct GLUE_INFO *prGlueInfo, uint32_t u4ReadAddr,
		     uint32_t *pu4LowVal, uint32_t *pu4HighVal);
void halMbuEnableDebug(struct GLUE_INFO *prGlueInfo);
void halMbuDebug(struct GLUE_INFO *prGlueInfo);
#endif

void halUpdateRxDmadSdp(struct RXD_STRUCT *pRxD, struct RTMP_DMABUF *prDmaBuf);

#if (CFG_SUPPORT_HOST_OFFLOAD == 1)
/* Host Offload */
void halRroTurnOff(struct GLUE_INFO *prGlueInfo);
void halRroInit(struct GLUE_INFO *prGlueInfo);
void halRroUninit(struct GLUE_INFO *prGlueInfo);
void halOffloadAllocMem(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem);
void halOffloadFreeMem(struct GLUE_INFO *prGlueInfo);
void halRroAllocMem(struct GLUE_INFO *prGlueInfo, u_int8_t fgAllocMem);
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
void halMawdAllocRxBlkRing(struct GLUE_INFO *prGlueInfo,
			   u_int8_t fgAllocMem, uint32_t u4Num);
void halMawdInitRxBlkRing(struct GLUE_INFO *prGlueInfo);
void halMawdInitTxRing(struct GLUE_INFO *prGlueInfo);
u_int8_t halMawdFillTxRing(struct GLUE_INFO *prGlueInfo,
		       struct MSDU_TOKEN_ENTRY *prToken);
uint32_t halMawdGetRxBlkDoneCnt(struct GLUE_INFO *prGlueInfo, uint32_t u4Num);
u_int8_t halMawdWakeup(struct GLUE_INFO *prGlueInfo);
u_int8_t halMawdSleep(struct GLUE_INFO *prGlueInfo);
void halMawdReset(struct GLUE_INFO *prGlueInfo);
void halMawdUpdateL2Tbl(struct GLUE_INFO *prGlueInfo,
			union mawd_l2tbl rL2Tbl, uint32_t u4Set);
void halMawdDumpSram(struct GLUE_INFO *prGlueInfo);
#else
static inline int halMawdPwrOn(void) { return 0; }
static inline void halMawdPwrOff(void) {}
#endif /* CFG_SUPPORT_HOST_OFFLOAD == 1 */

int halInitResvMem(struct platform_device *pdev,
		enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx);
#if (CFG_MTK_WIFI_TX_CMA_MEM == 1)
int halInitTxCmaMem(struct platform_device *pdev);
void halFreeTxCmaMem(struct platform_device *pdev);
bool halTxDataCmaIsCmaMem(void);
void wifi_tx_cma_set_mem_data_num(struct ADAPTER *prAdapter,
	uint32_t size);
uint32_t wifi_tx_cma_get_mem_size(void);
uint32_t wifi_tx_cma_get_mem_data_num(void);
void halCopyPathAllocTxCmaTxDataBuf(
	struct MSDU_TOKEN_ENTRY *prToken, uint32_t u4Idx);
bool halCopyPathCopyTxCmaTxData(struct MSDU_TOKEN_ENTRY *prToken,
			  void *pucSrc, uint32_t u4Len);
phys_addr_t halCopyPathTxCmaMapTxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
void halCopyPathTxCmaUnmapTxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
#endif /* CFG_MTK_WIFI_TX_CMA_MEM */
void halCopyPathAllocTxDesc(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
void halCopyPathAllocRxDesc(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Num);
#if CFG_SUPPORT_WIFI_RSV_MEM
int halAllocHifMem(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData);
void halFreeHifMem(struct platform_device *pdev,
		  enum ENUM_WIFI_RSV_MEM_IDX u4RsvMemIdx);
#endif
#if (CFG_MTK_ANDROID_WMT == 1)
void halCopyPathAllocExtBuf(struct GL_HIF_INFO *prHifInfo,
			    struct RTMP_DMABUF *prDescRing,
			    uint32_t u4Align);
void halCopyPathFreeExtBuf(struct GL_HIF_INFO *prHifInfo,
			   struct RTMP_DMABUF *prDescRing);
#endif
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
			   struct RXD_STRUCT *pRxD,
			   struct SW_RFB *prSwRfb,
			   u_int8_t fgUpdatePtr);
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
				struct RTMP_DMABUF *prDescRing,
				uint32_t u4Align);
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
phys_addr_t halZeroCopyPathMapTxDataBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
phys_addr_t halZeroCopyPathMapTxCmdBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
phys_addr_t halZeroCopyPathMapRxBuf(struct GL_HIF_INFO *prHifInfo,
			  void *pucBuf, uint32_t u4Offset, uint32_t u4Len);
void halZeroCopyPathUnmapTxDataBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
void halZeroCopyPathUnmapTxCmdBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
void halZeroCopyPathUnmapRxBuf(struct GL_HIF_INFO *prHifInfo,
			   phys_addr_t rDmaAddr, uint32_t u4Len);
void halZeroCopyPathFreeDesc(struct GL_HIF_INFO *prHifInfo,
			 struct RTMP_DMABUF *prDescRing);
void halZeroCopyPathFreeCmdBuf(void *pucSrc, uint32_t u4Len);
void halZeroCopyPathFreePacket(struct GL_HIF_INFO *prHifInfo,
			   void *pvPacket, uint32_t u4Num);
void halZeroCopyPathDumpTx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_TX_RING *prTxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
void halZeroCopyPathDumpRx(struct GL_HIF_INFO *prHifInfo,
		       struct RTMP_RX_RING *prRxRing,
		       uint32_t u4Idx, uint32_t u4DumpLen);
struct HIF_MEM *halGetWiFiMiscRsvEmi(
	struct mt66xx_chip_info *prChipInfo,
	enum WIFI_MISC_MEM_BLOCK_NAME u4idx);
#if (CFG_MTK_WIFI_CONNV3_SUPPORT == 1)
u_int8_t kalDevRegReadViaBT(struct GLUE_INFO *prGlueInfo,
				uint32_t u4Register, uint32_t *pu4Value);
u_int8_t kalDevRegWriteViaBT(struct GLUE_INFO *prGlueInfo,
				uint32_t u4Register, uint32_t u4Value);
#endif

#if CFG_SUPPORT_PAGE_POOL_USE_CMA
void halZeroCopyPathFreePagePoolPacket(struct GL_HIF_INFO *prHifInfo,
				       void *pvPacket, uint32_t u4Num);
void *halZeroCopyPathAllocPagePoolRxBuf(struct GL_HIF_INFO *prHifInfo,
					struct RTMP_DMABUF *prDmaBuf,
					uint32_t u4Num, uint32_t u4Idx);
#if CFG_SUPPORT_DYNAMIC_PAGE_POOL
void kalSetupPagePoolPageMaxMinNum(uint32_t u4Min, uint32_t u4Max);
uint32_t kalGetPagePoolPageNum(void);
u_int8_t kalSetPagePoolPageMaxNum(void);
u_int8_t kalIncPagePoolPageNum(void);
u_int8_t kalDecPagePoolPageNum(void);
u_int8_t kalSetPagePoolPageNum(uint32_t u4Num);
#endif
struct sk_buff *kalAllocRxSkbFromCmaPp(
	struct GLUE_INFO *prGlueInfo, uint8_t **ppucData);
u_int8_t kalCreateHifSkbList(struct mt66xx_chip_info *prChipInfo);
void kalReleaseHifSkbList(void);
struct sk_buff *kalAllocHifSkb(void);
void kalFreeHifSkb(struct sk_buff *prSkb);

extern struct page *wifi_page_pool_alloc_page(void) __attribute__((weak));
extern void wifi_page_pool_set_page_num(uint32_t num) __attribute__((weak));
extern uint32_t wifi_page_pool_get_page_num(void) __attribute__((weak));
extern uint32_t wifi_page_pool_get_max_page_num(void) __attribute__((weak));
#endif /* CFG_SUPPORT_PAGE_POOL_USE_CMA */

void halWpdmaStopRecycleDmad(struct GLUE_INFO *prGlueInfo,
				       uint16_t u2Port);
#if (CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE == 1)
int halInitTxCmaNonCacheMem(struct platform_device *pdev);
int halUninitTxCmaNonCacheMem(void);
int halAllocHifMemForTxCmaNonCache(
	struct platform_device *pdev,
	struct mt66xx_hif_driver_data *prDriverData);
void halGetTxCmaNonCacheMemUsage(void);
#endif /* CFG_MTK_WIFI_TX_CMA_MEM_NON_CACHE */
#if (CFG_MTK_WIFI_MISC_RSV_MEM == 1)
int halAllocHifMemForWiFiMisc(struct platform_device *pdev,
		   struct mt66xx_hif_driver_data *prDriverData);
#endif
#if CFG_SUPPORT_HIF_REG_WORK
void wf_reg_enable(u_int8_t fgEn);
int32_t wf_reg_read_wrapper(void *priv,
	uint32_t addr, uint32_t *value);
int32_t wf_reg_write_wrapper(void *priv,
	uint32_t addr, uint32_t value);
int32_t wf_reg_write_mask_wrapper(
	void *priv, uint32_t addr, uint32_t mask, uint32_t value);
int32_t wf_reg_start_wrapper(enum connv3_drv_type from_drv, void *priv_data);
int32_t wf_reg_end_wrapper(enum connv3_drv_type from_drv, void *priv_data);
void halHandleHifRegReq(struct GLUE_INFO *prGlueInfo);
#endif /* CFG_SUPPORT_HIF_REG_WORK */
#endif /* HIF_PDMA_H__ */
