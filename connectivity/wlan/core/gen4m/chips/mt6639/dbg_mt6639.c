/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */
/******************************************************************************
 *[File]             dbg_mt6639.c
 *[Version]          v1.0
 *[Revision Date]    2020-05-22
 *[Author]
 *[Description]
 *    The program provides WIFI FALCON MAC Debug APIs
 *[Copyright]
 *    Copyright (C) 2015 MediaTek Incorporation. All Rights Reserved.
 ******************************************************************************/

#ifdef MT6639

#include "precomp.h"
#include "mt6639.h"
#include "dbg_mt6639.h"
#if defined(_HIF_PCIE)
#include "hif_pdma.h"
#endif
#include "coda/mt6639/wf_ple_top.h"
#include "coda/mt6639/wf_pse_top.h"
#include "coda/mt6639/wf_wfdma_host_dma0.h"
#include "coda/mt6639/wf_hif_dmashdl_top.h"
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
#include "connv3.h"
#include "connectivity_build_in_adapter.h"
#endif

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
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

/*******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */

struct PLE_TOP_CR rMt6639PleTopCr = {
	.rAc0QueueEmpty0 = {
		WF_PLE_TOP_AC0_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc1QueueEmpty0 = {
		WF_PLE_TOP_AC1_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc2QueueEmpty0 = {
		WF_PLE_TOP_AC2_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rAc3QueueEmpty0 = {
		WF_PLE_TOP_AC3_QUEUE_EMPTY0_ADDR,
		0,
		0
	},
	.rCpuPgInfo = {
		WF_PLE_TOP_CPU_PG_INFO_ADDR,
		0,
		0
	},
	.rCpuPgInfoCpuRsvCnt = {
		0,
		WF_PLE_TOP_CPU_PG_INFO_CPU_RSV_CNT_MASK,
		WF_PLE_TOP_CPU_PG_INFO_CPU_RSV_CNT_SHFT
	},
	.rCpuPgInfoCpuSrcCnt = {
		0,
		WF_PLE_TOP_CPU_PG_INFO_CPU_SRC_CNT_MASK,
		WF_PLE_TOP_CPU_PG_INFO_CPU_SRC_CNT_SHFT
	},
	.rDisStaMap0 = {
		WF_PLE_TOP_DIS_STA_MAP0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0 = {
		WF_PLE_TOP_FL_QUE_CTRL_0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0Execute = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_0_EXECUTE_MASK,
		0
	},
	.rFlQueCtrl0QBufWlanid = {
		0,
		0,
		WF_PLE_TOP_FL_QUE_CTRL_0_Q_BUF_WLANID_SHFT
	},
	.rFlQueCtrl2 = {
		WF_PLE_TOP_FL_QUE_CTRL_2_ADDR,
		0,
		0
	},
	.rFlQueCtrl2QueueHeadFid = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_SHFT
	},
	.rFlQueCtrl2QueueTailFid = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_SHFT
	},
	.rFlQueCtrl3 = {
		WF_PLE_TOP_FL_QUE_CTRL_3_ADDR,
		0,
		0
	},
	.rFlQueCtrl3QueuePktNum = {
		0,
		WF_PLE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_MASK,
		WF_PLE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_SHFT
	},
	.rFreepgCnt = {
		WF_PLE_TOP_FREEPG_CNT_ADDR,
		0,
		0
	},
	.rFreepgCntFfaCnt = {
		0,
		WF_PLE_TOP_FREEPG_CNT_FFA_CNT_MASK,
		WF_PLE_TOP_FREEPG_CNT_FFA_CNT_SHFT
	},
	.rFreepgCntFreepgCnt = {
		0,
		WF_PLE_TOP_FREEPG_CNT_FREEPG_CNT_MASK,
		WF_PLE_TOP_FREEPG_CNT_FREEPG_CNT_SHFT
	},
	.rFreepgHeadTail = {
		WF_PLE_TOP_FREEPG_HEAD_TAIL_ADDR,
		0,
		0
	},
	.rFreepgHeadTailFreepgHead = {
		0,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_MASK,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_SHFT
	},
	.rFreepgHeadTailFreepgTail = {
		0,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_MASK,
		WF_PLE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_SHFT
	},
	.rFsmPeekCr00 = {
		WF_PLE_TOP_PEEK_CR_00_ADDR,
		0,
		0
	},
	.rFsmPeekCr01 = {
		WF_PLE_TOP_PEEK_CR_01_ADDR,
		0,
		0
	},
	.rFsmPeekCr02 = {
		WF_PLE_TOP_PEEK_CR_02_ADDR,
		0,
		0
	},
	.rFsmPeekCr03 = {
		WF_PLE_TOP_PEEK_CR_03_ADDR,
		0,
		0
	},
	.rFsmPeekCr04 = {
		WF_PLE_TOP_PEEK_CR_04_ADDR,
		0,
		0
	},
	.rFsmPeekCr05 = {
		WF_PLE_TOP_PEEK_CR_05_ADDR,
		0,
		0
	},
	.rFsmPeekCr06 = {
		WF_PLE_TOP_PEEK_CR_06_ADDR,
		0,
		0
	},
	.rFsmPeekCr07 = {
		WF_PLE_TOP_PEEK_CR_07_ADDR,
		0,
		0
	},
	.rFsmPeekCr08 = {
		WF_PLE_TOP_PEEK_CR_08_ADDR,
		0,
		0
	},
	.rFsmPeekCr09 = {
		WF_PLE_TOP_PEEK_CR_09_ADDR,
		0,
		0
	},
	.rFsmPeekCr10 = {
		WF_PLE_TOP_PEEK_CR_10_ADDR,
		0,
		0
	},
	.rFsmPeekCr11 = {
		WF_PLE_TOP_PEEK_CR_11_ADDR,
		0,
		0
	},
	.rHifPgInfo = {
		WF_PLE_TOP_HIF_PG_INFO_ADDR,
		0,
		0
	},
	.rHifPgInfoHifRsvCnt = {
		0,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_RSV_CNT_SHFT
	},
	.rHifPgInfoHifSrcCnt = {
		0,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_PG_INFO_HIF_SRC_CNT_SHFT
	},
	.rHifTxcmdPgInfo = {
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_ADDR,
		0,
		0
	},
	.rHifTxcmdPgInfoHifTxcmdRsvCnt = {
		0,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_RSV_CNT_SHFT
	},
	.rHifTxcmdPgInfoHifTxcmdSrcCnt = {
		0,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_TXCMD_PG_INFO_HIF_TXCMD_SRC_CNT_SHFT
	},
	.rHifWmtxdPgInfo = {
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_ADDR,
		0,
		0
	},
	.rHifWmtxdPgInfoHifWmtxdRsvCnt = {
		0,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_RSV_CNT_MASK,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_RSV_CNT_SHFT
	},
	.rHifWmtxdPgInfoHifWmtxdSrcCnt = {
		0,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_SRC_CNT_MASK,
		WF_PLE_TOP_HIF_WMTXD_PG_INFO_HIF_WMTXD_SRC_CNT_SHFT
	},
	.rIntN9ErrSts = {
		WF_PLE_TOP_INT_N9_ERR_STS_ADDR,
		0,
		0
	},
	.rIntN9ErrSts1 = {
		WF_PLE_TOP_INT_N9_ERR_STS_1_ADDR,
		0,
		0
	},
	.rPbufCtrl = {
		WF_PLE_TOP_PBUF_CTRL_ADDR,
		0,
		0
	},
	.rPbufCtrlPageSizeCfg = {
		0,
		WF_PLE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_MASK,
		WF_PLE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_SHFT
	},
	.rPbufCtrlPbufOffset = {
		0,
		WF_PLE_TOP_PBUF_CTRL_PBUF_OFFSET_MASK,
		WF_PLE_TOP_PBUF_CTRL_PBUF_OFFSET_SHFT
	},
	.rPbufCtrlTotalPageNum = {
		0,
		WF_PLE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_MASK,
		WF_PLE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_SHFT
	},
	.rPgCpuGroup = {
		WF_PLE_TOP_PG_CPU_GROUP_ADDR,
		0,
		0
	},
	.rPgCpuGroupCpuMaxQuota = {
		0,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MAX_QUOTA_SHFT
	},
	.rPgCpuGroupCpuMinQuota = {
		0,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_CPU_GROUP_CPU_MIN_QUOTA_SHFT
	},
	.rPgHifGroup = {
		WF_PLE_TOP_PG_HIF_GROUP_ADDR,
		0,
		0
	},
	.rPgHifGroupHifMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MAX_QUOTA_SHFT
	},
	.rPgHifGroupHifMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_GROUP_HIF_MIN_QUOTA_SHFT
	},
	.rPgHifTxcmdGroup = {
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_ADDR,
		0,
		0
	},
	.rPgHifTxcmdGroupHifTxcmdMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MAX_QUOTA_SHFT
	},
	.rPgHifTxcmdGroupHifTxcmdMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_TXCMD_GROUP_HIF_TXCMD_MIN_QUOTA_SHFT
	},
	.rPgHifWmtxdGroup = {
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_ADDR,
		0,
		0
	},
	.rPgHifWmtxdGroupHifWmtxdMaxQuota = {
		0,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MAX_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MAX_QUOTA_SHFT
	},
	.rPgHifWmtxdGroupHifWmtxdMinQuota = {
		0,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MIN_QUOTA_MASK,
		WF_PLE_TOP_PG_HIF_WMTXD_GROUP_HIF_WMTXD_MIN_QUOTA_SHFT
	},
	.rQueueEmpty = {
		WF_PLE_TOP_QUEUE_EMPTY_ADDR,
		0,
		0
	},
	.rQueueEmptyAllAcEmpty = {
		0,
		WF_PLE_TOP_QUEUE_EMPTY_ALL_AC_EMPTY_MASK,
		0
	},
	/*
	.rStationPause0 = {
		WF_PLE_TOP_STATION_PAUSE0_ADDR,
		0,
		0
	},
	*/
	.rTxdQueueEmpty = {
		WF_PLE_TOP_TXD_QUEUE_EMPTY_ADDR,
		0,
		0
	},
	.rToN9IntToggle = {
		WF_PLE_TOP_TO_N9_INT_TOGGLE_ADDR,
		WF_PLE_TOP_TO_N9_INT_TOGGLE_MASK,
		WF_PLE_TOP_TO_N9_INT_TOGGLE_SHFT
	},
};

struct PSE_TOP_CR rMt6639PseTopCr = {
	.rFlQueCtrl0 = {
		WF_PSE_TOP_FL_QUE_CTRL_0_ADDR,
		0,
		0
	},
	.rFlQueCtrl0Execute = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_0_EXECUTE_MASK,
		0
	},
	.rFlQueCtrl0QBufQid = {
		0,
		0,
		WF_PSE_TOP_FL_QUE_CTRL_0_Q_BUF_QID_SHFT
	},
	.rFlQueCtrl2 = {
		WF_PSE_TOP_FL_QUE_CTRL_2_ADDR,
		0,
		0
	},
	.rFlQueCtrl2QueueHeadFid = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_HEAD_FID_SHFT
	},
	.rFlQueCtrl2QueueTailFid = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_2_QUEUE_TAIL_FID_SHFT
	},
	.rFlQueCtrl3 = {
		WF_PSE_TOP_FL_QUE_CTRL_3_ADDR,
		0,
		0
	},
	.rFlQueCtrl3QueuePktNum = {
		0,
		WF_PSE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_MASK,
		WF_PSE_TOP_FL_QUE_CTRL_3_QUEUE_PKT_NUM_SHFT
	},
	.rFreepgCnt = {
		WF_PSE_TOP_FREEPG_CNT_ADDR,
		0,
		0
	},
	.rFreepgCntFfaCnt = {
		0,
		WF_PSE_TOP_FREEPG_CNT_FFA_CNT_MASK,
		WF_PSE_TOP_FREEPG_CNT_FFA_CNT_SHFT
	},
	.rFreepgCntFreepgCnt = {
		0,
		WF_PSE_TOP_FREEPG_CNT_FREEPG_CNT_MASK,
		WF_PSE_TOP_FREEPG_CNT_FREEPG_CNT_SHFT
	},
	.rFreepgHeadTail = {
		WF_PSE_TOP_FREEPG_HEAD_TAIL_ADDR,
		0,
		0
	},
	.rFreepgHeadTailFreepgHead = {
		0,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_MASK,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_HEAD_SHFT
	},
	.rFreepgHeadTailFreepgTail = {
		0,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_MASK,
		WF_PSE_TOP_FREEPG_HEAD_TAIL_FREEPG_TAIL_SHFT
	},
	.rFsmPeekCr00 = {
		WF_PSE_TOP_PSE_SEEK_CR_00_ADDR,
		0,
		0
	},
	.rFsmPeekCr01 = {
		WF_PSE_TOP_PSE_SEEK_CR_01_ADDR,
		0,
		0
	},
	.rFsmPeekCr02 = {
		WF_PSE_TOP_PSE_SEEK_CR_02_ADDR,
		0,
		0
	},
	.rFsmPeekCr03 = {
		WF_PSE_TOP_PSE_SEEK_CR_03_ADDR,
		0,
		0
	},
	.rFsmPeekCr04 = {
		WF_PSE_TOP_PSE_SEEK_CR_04_ADDR,
		0,
		0
	},
	.rFsmPeekCr05 = {
		WF_PSE_TOP_PSE_SEEK_CR_05_ADDR,
		0,
		0
	},
	.rFsmPeekCr06 = {
		WF_PSE_TOP_PSE_SEEK_CR_06_ADDR,
		0,
		0
	},
	.rFsmPeekCr07 = {
		WF_PSE_TOP_PSE_SEEK_CR_07_ADDR,
		0,
		0
	},
	.rFsmPeekCr08 = {
		WF_PSE_TOP_PSE_SEEK_CR_08_ADDR,
		0,
		0
	},
	.rFsmPeekCr09 = {
		WF_PSE_TOP_PSE_SEEK_CR_09_ADDR,
		0,
		0
	},
	.rHif0PgInfoHif0RsvCnt = {
		0,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_RSV_CNT_MASK,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_RSV_CNT_SHFT
	},
	.rHif0PgInfoHif0SrcCnt = {
		0,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_SRC_CNT_MASK,
		WF_PSE_TOP_HIF0_PG_INFO_HIF0_SRC_CNT_SHFT
	},
	.rIntN9Err1Sts = {
		WF_PSE_TOP_INT_N9_ERR1_STS_ADDR,
		0,
		0
	},
	.rIntN9ErrSts = {
		WF_PSE_TOP_INT_N9_ERR_STS_ADDR,
		0,
		0
	},
	.rPbufCtrl = {
		WF_PSE_TOP_PBUF_CTRL_ADDR,
		0,
		0
	},
	.rPbufCtrlPageSizeCfg = {
		0,
		WF_PSE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_MASK,
		WF_PSE_TOP_PBUF_CTRL_PAGE_SIZE_CFG_SHFT
	},
	.rPbufCtrlPbufOffset = {
		0,
		WF_PSE_TOP_PBUF_CTRL_PBUF_OFFSET_MASK,
		WF_PSE_TOP_PBUF_CTRL_PBUF_OFFSET_SHFT
	},
	.rPbufCtrlTotalPageNum = {
		0,
		WF_PSE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_MASK,
		WF_PSE_TOP_PBUF_CTRL_TOTAL_PAGE_NUM_SHFT
	},
	.rPgHif0GroupHif0MaxQuota = {
		0,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MAX_QUOTA_MASK,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MAX_QUOTA_SHFT
	},
	.rPgHif0GroupHif0MinQuota = {
		0,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MIN_QUOTA_MASK,
		WF_PSE_TOP_PG_HIF0_GROUP_HIF0_MIN_QUOTA_SHFT
	},
	.rQueueEmpty = {
		WF_PSE_TOP_QUEUE_EMPTY_ADDR,
		WF_PSE_TOP_QUEUE_EMPTY_MASK_ADDR,
		0
	},
	.rQueueEmptyCpuQ0Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q0_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q0_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ1Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q1_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q1_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ2Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q2_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q2_EMPTY_SHFT
	},
	.rQueueEmptyCpuQ3Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q3_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_CPU_Q3_EMPTY_SHFT
	},
	.rQueueEmptyHif0Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_0_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_0_EMPTY_SHFT
	},
	.rQueueEmptyHif10Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_10_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_10_EMPTY_SHFT
	},
	.rQueueEmptyHif11Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_11_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_11_EMPTY_SHFT
	},
	.rQueueEmptyHif1Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_1_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_1_EMPTY_SHFT
	},
	.rQueueEmptyHif2Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_2_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_2_EMPTY_SHFT
	},
	.rQueueEmptyHif3Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_3_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_3_EMPTY_SHFT
	},
	.rQueueEmptyHif4Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_4_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_4_EMPTY_SHFT
	},
	.rQueueEmptyHif5Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_5_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_5_EMPTY_SHFT
	},
	.rQueueEmptyHif6Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_6_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_6_EMPTY_SHFT
	},
	.rQueueEmptyHif7Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_7_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_7_EMPTY_SHFT
	},
	.rQueueEmptyHif8Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_8_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_8_EMPTY_SHFT
	},
	.rQueueEmptyHif9Empty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_9_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_HIF_9_EMPTY_SHFT
	},
	.rQueueEmptyLmacTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_LMAC_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_LMAC_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxioc1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpRxiocQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_RXIOC_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTx1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxioc1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyMdpTxiocQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_MDP_TXIOC_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptyRlsQEmtpy = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_RLS_Q_EMTPY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_RLS_Q_EMTPY_SHFT
	},
	.rQueueEmptySecRxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_RX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_RX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySecTx1QueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX1_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX1_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySecTxQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SEC_TX_QUEUE_EMPTY_SHFT
	},
	.rQueueEmptySfdParkQueueEmpty = {
		0,
		WF_PSE_TOP_QUEUE_EMPTY_SFD_PARK_QUEUE_EMPTY_MASK,
		WF_PSE_TOP_QUEUE_EMPTY_SFD_PARK_QUEUE_EMPTY_SHFT
	},
};

struct PP_TOP_CR rMt6639PpTopCr = {
	.rDbgCtrl = {
		WF_PP_TOP_DBG_CTRL_ADDR,
		0,
		0
	},
	.rDbgCs0 = {
		WF_PP_TOP_DBG_CS_0_ADDR,
		0,
		0
	},
	.rDbgCs1 = {
		WF_PP_TOP_DBG_CS_1_ADDR,
		0,
		0
	},
	.rDbgCs2 = {
		WF_PP_TOP_DBG_CS_2_ADDR,
		0,
		0
	},
};

/*******************************************************************************
 *                           P R I V A T E   D A T A
 *******************************************************************************
 */
void mt6639_show_wfdma_dbg_probe_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type)
{
	uint32_t dbg_cr_idx[] = {0x0, 0x1, 0x2, 0x3, 0x30, 0x5, 0x7, 0xA, 0xB,
		0xC};
	uint32_t i = 0, u4DbgIdxAddr = 0, u4DbgProbeAddr = 0, u4DbgIdxValue = 0,
		u4DbgProbeValue = 0;

	if (!prAdapter)
		return;

	if (enum_wfdma_type != WFDMA_TYPE_HOST)
		return;

	u4DbgIdxAddr = WF_WFDMA_HOST_DMA0_WPDMA_DBG_IDX_ADDR;
	u4DbgProbeAddr = WF_WFDMA_HOST_DMA0_WPDMA_DBG_PROBE_ADDR;

	for (i = 0; i < ARRAY_SIZE(dbg_cr_idx); i++) {
		u4DbgIdxValue = 0x100 + dbg_cr_idx[i];
		HAL_MCR_WR(prAdapter, u4DbgIdxAddr, u4DbgIdxValue);
		HAL_MCR_RD(prAdapter, u4DbgProbeAddr, &u4DbgProbeValue);
		DBGLOG(HAL, INFO, "\t Write(0x%2x) DBG_PROBE[0x%X]=0x%08X\n",
			u4DbgIdxValue, u4DbgProbeAddr, u4DbgProbeValue);
	}
}

void mt6639_show_wfdma_wrapper_info(struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type)
{
	uint32_t u4DmaCfgCr = 0;
	uint32_t u4RegValue = 0;

	if (!prAdapter)
		return;

	if (enum_wfdma_type == WFDMA_TYPE_HOST) {
		u4DmaCfgCr = 0x7c027044;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, INFO, "WFDMA_HIF_BUSY(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c027050;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, INFO, "WFDMA_AXI_SLPPROT_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c027078;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, INFO, "WFDMA_AXI_SLPPROT0_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);

		u4DmaCfgCr = 0x7c02707C;
		HAL_MCR_RD(prAdapter, u4DmaCfgCr, &u4RegValue);
		DBGLOG(INIT, INFO, "WFDMA_AXI_SLPPROT1_CTRL(0x%08x): 0x%08x\n",
				u4DmaCfgCr,
				u4RegValue);
	}
}

#if defined(_HIF_PCIE)
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
void mt6639_dumpPcieReg(void)
{
	uint32_t u4Value = 0;

	DBGLOG(HAL, INFO, "Start mt6639_dumpPcieReg.\n");
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x70003014, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x70003014] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x70025014, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x70025014] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x70025404, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x70025404] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030150, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030150] value[0x%08x]\n", u4Value);

	connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030150, 0x3ffff);

	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030150, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030150] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030018, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030018] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030080, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030080] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030098, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030098] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x740300C0, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x740300C0] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x740300C8, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x740300C8] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030150, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030150] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030154, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030154] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030180, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030180] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030184, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030184] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030188, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030188] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403018C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403018C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030194, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030194] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030198, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030198] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403019C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403019C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x740301A4, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x740301A4] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x740301A8, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x740301A8] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x740301AC, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x740301AC] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D00, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D00] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D38, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D38] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D3C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D3C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D40, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D40] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D44, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D44] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D48, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D48] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D4C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D4C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D50, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D50] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D54, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D54] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D58, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D58] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D5C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D5C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D60, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D60] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D64, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D64] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D68, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D68] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D6C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D6C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D70, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D70] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D74, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D74] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D78, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D78] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030D7C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030D7C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E00, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E00] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E04, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E04] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E08, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E08] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E0C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E0C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E10, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E10] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E14, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E14] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E18, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E18] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E1C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E1C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E20, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E20] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E24, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E24] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E28, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E28] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E2C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E2C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E30, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E30] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E34, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E34] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E38, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E38] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E3C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E3C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E40, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E40] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E44, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E44] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E48, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E48] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74030E4C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74030E4C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031000, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031000] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031004, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031004] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031008, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031008] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403100C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403100C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031010, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031010] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031014, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031014] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031018, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031018] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403101C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403101C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031080, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031080] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031084, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031084] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031088, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031088] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403108C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403108C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031090, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031090] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031094, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031094] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031098, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031098] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403109C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403109C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031110, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031110] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031114, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031114] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031118, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031118] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403111C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403111C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031200, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031200] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031204, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031204] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031208, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031208] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403120C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403120C] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031210, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031210] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031214, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031214] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x74031218, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x74031218] value[0x%08x]\n", u4Value);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
		0x7403121C, &u4Value);
	DBGLOG(HAL, INFO, "CR[0x7403121C] value[0x%08x]\n", u4Value);
}

bool mt6639_CheckDumpViaBt(void)
{
	return (fgIsBusAccessFailed || fgIsMcuOff) && fgTriggerDebugSop;
}
#endif
void mt6639_dumpCbtopReg(struct ADAPTER *ad)
{
	uint32_t u4Value = 0;

	DBGLOG(HAL, INFO, "Start mt6639_dumpCbtopReg.\n");
	if (fgIsBusAccessFailed || fgIsMcuOff) {

		/* 1. dump 0x18023C00[31:0] -> 0x7c023c00 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x7c023c00, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x7c023c00] value[0x%08x]\n", u4Value);

		/* 2. dump 0x70007204, read patch version */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70007204, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70007204] value[0x%08x]\n", u4Value);

		/* 3. dump 0x7002500C, cb_infra_slp_status */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x7002500C, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x7002500C] value[0x%08x]\n", u4Value);

		/* 4. dump slp_ctrl setting: 0x70025004/0x7002500C/0x70025014 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70025004, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70025004] value[0x%08x]\n", u4Value);
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x7002500C, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x7002500C] value[0x%08x]\n", u4Value);
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70025014, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70025014] value[0x%08x]\n", u4Value);

		/* 5. dump slp_ctrl cnt: 0x70025400/0x70025404 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70025400, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70025400] value[0x%08x]\n", u4Value);
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70025404, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70025404] value[0x%08x]\n", u4Value);

		/* 6. dump ap2conn gals dbg : 0x70026008 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70026008, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70026008] value[0x%08x]\n", u4Value);

		/* 7. dump conn2ap gals dbg : 0x70026000 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70026000, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70026000] value[0x%08x]\n", u4Value);

		/* 8. dump dma2ap gals dbg : 0x70026100 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70026100, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70026100] value[0x%08x]\n", u4Value);

		/* 9. dump 0x70025300  // debug index */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70025300, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70025300] value[0x%08x]\n", u4Value);

		/* 10. dump 0x70026550 // debug index */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70026550, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70026550] value[0x%08x]\n", u4Value);

		/* 11. dump 0x7002801C // debug index */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x7002801C, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x7002801C] value[0x%08x]\n", u4Value);

		/* 12. write 0x70003020 = 0x0 // set vlp_uds_ctrl probe 00 */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70003020, 0x0);

		/* 13. write 0x70007150 = 0x2 */
		connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70007150, 0x2);

		/* 14. CBTOP REGs dump 0x70007154 */
		connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT,
			0x70007154, &u4Value);
		DBGLOG(HAL, INFO, "CR[0x70007154] value[0x%08x]\n", u4Value);
	}
}

void mt6639_dumpWfsyscpupcr(struct ADAPTER *ad)
{
#define CPUPCR_LOG_NUM	5
#define CPUPCR_BUF_SZ	50

	uint32_t i = 0;
	uint32_t var_pc = 0;
	uint32_t var_lp = 0;
	uint64_t log_sec = 0;
	uint64_t log_nsec = 0;
	char log_buf_pc[CPUPCR_LOG_NUM][CPUPCR_BUF_SZ];
	char log_buf_lp[CPUPCR_LOG_NUM][CPUPCR_BUF_SZ];

	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
		0x3F,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_MASK);
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
		0x3F,
		CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_WF_MCU_DBG_PC_LOG_MASK);
	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	for (i = 0; i < CPUPCR_LOG_NUM; i++) {
		log_sec = local_clock();
		log_nsec = do_div(log_sec, 1000000000)/1000;
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			   &var_pc);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			   &var_lp);

		kalSnprintf(log_buf_pc[i],
			    CPUPCR_BUF_SZ,
			    "%llu.%06llu/0x%08x;",
			    log_sec,
			    log_nsec,
			    var_pc);

		kalSnprintf(log_buf_lp[i],
			    CPUPCR_BUF_SZ,
			    "%llu.%06llu/0x%08x;",
			    log_sec,
			    log_nsec,
			    var_lp);
	}

	DBGLOG(HAL, INFO, "wm pc=%s%s%s%s%s\n",
		log_buf_pc[0],
		log_buf_pc[1],
		log_buf_pc[2],
		log_buf_pc[3],
		log_buf_pc[4]);

	DBGLOG(HAL, INFO, "wm lp=%s%s%s%s%s\n",
		log_buf_lp[0],
		log_buf_lp[1],
		log_buf_lp[2],
		log_buf_lp[3],
		log_buf_lp[4]);
}

void mt6639_dumpPcGprLog(struct ADAPTER *ad)
{
#define PC_LOG_NUM			35
#define GPR_LOG_NUM			35

	uint32_t i = 0;
	uint32_t pc_dump[PC_LOG_NUM];
	uint32_t gpr_dump[GPR_LOG_NUM];

	DBGLOG(HAL, INFO, "Dump PC log / GPR log\n");

	HAL_MCR_WR_FIELD(ad,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR,
		0x0,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_SHFT,
		CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_WF_MCU_DBGOUT_SEL_MASK);

	kalMemZero(pc_dump, sizeof(pc_dump));
	for (i = 0; i < PC_LOG_NUM; i++) {
		HAL_MCR_WR_FIELD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_ADDR,
			i,
			CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_SHFT,
			CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_SEL_WF_MCU_DBG_PC_LOG_SEL_MASK);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_DBG_PC_LOG_ADDR,
			   &pc_dump[i]);
	}
	connac3x_dump_format_memory32(pc_dump, PC_LOG_NUM, "PC log");

	kalMemZero(gpr_dump, sizeof(gpr_dump));
	for (i = 0; i < GPR_LOG_NUM; i++) {
		HAL_MCR_WR_FIELD(ad,
			CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_ADDR,
			i,
			CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_SHFT,
			CONN_DBG_CTL_WF_MCU_DBG_GPR_LOG_SEL_WF_MCU_DBG_GPR_LOG_SEL_MASK);
		HAL_MCR_RD(ad,
			   CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR,
			   &gpr_dump[i]);
	}
	connac3x_dump_format_memory32(gpr_dump, GPR_LOG_NUM, "GPR log");
}

void mt6639_dumpN45CoreReg(struct ADAPTER *ad)
{
#define GENERAL_LOG_NUM			32
#define CTRL_LOG_NUM			5

	uint32_t i = 0, idx = 0;
	uint32_t val = 0;
	uint32_t general_dump[GENERAL_LOG_NUM];
	uint32_t ctl_status_dump[CTRL_LOG_NUM];

	if (!mt6639_is_ap2conn_off_readable(ad) ||
	    !mt6639_is_conn2wf_readable(ad))
		return;

	kalMemZero(ctl_status_dump, sizeof(ctl_status_dump));
	for (i = 0, idx = 0; i < ARRAY_SIZE(n45_general_dump_list); i++) {
		if (n45_general_dump_list[i].read) {
			HAL_MCR_RD(ad,
				   n45_general_dump_list[i].addr,
				   &val);
			general_dump[idx++] = val;
		} else {
			HAL_MCR_WR_FIELD(ad,
				n45_general_dump_list[i].addr,
				n45_general_dump_list[i].value,
				n45_general_dump_list[i].shift,
				n45_general_dump_list[i].mask);
		}
	}
	connac3x_dump_format_memory32(general_dump,
		GENERAL_LOG_NUM,
		"N45 General Purpose Registers");

	kalMemZero(ctl_status_dump, sizeof(ctl_status_dump));
	for (i = 0, idx = 0; i < ARRAY_SIZE(n45_ctrl_status_dump_list); i++) {
		if (n45_ctrl_status_dump_list[i].read) {
			HAL_MCR_RD(ad,
				   n45_ctrl_status_dump_list[i].addr,
				   &val);
			ctl_status_dump[idx++] = val;
		} else {
			HAL_MCR_WR_FIELD(ad,
				n45_ctrl_status_dump_list[i].addr,
				n45_ctrl_status_dump_list[i].value,
				n45_ctrl_status_dump_list[i].shift,
				n45_ctrl_status_dump_list[i].mask);
		}
	}
	connac3x_dump_format_memory32(ctl_status_dump,
		CTRL_LOG_NUM,
		"N45 Control & Status Registers");
}

static void mt6639_dumpWfTopMiscOn(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000010, 0x00000012, 0x00000017, 0x00000018, 0x00000019,
		0x0000001A, 0x0000001B, 0x0000001D
	};

	u4WrAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_EN_FR_HIF_ADDR;
	u4WrVal = 0x00000001;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, INFO,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	u4WrAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_SEL_FR_HIF_ADDR;
	u4RdAddr = CONN_HOST_CSR_TOP_WF_ON_MONFLG_OUT_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tW 0x%08x=[0x%08x], R 0x%08x=[0x%08x]\n",
		       u4WrAddr, u4WrVal, u4RdAddr, u4Val);
	}
}

static void mt6639_dumpWfTopMiscVon(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t au4List[] = {
		0x00000000, 0x00000001, 0x00000002, 0x00000003, 0x00000004,
		0x00000008
	};

	u4WrAddr = CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_EN_FR_HIF_ADDR;
	u4WrVal = 0x00000001;
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, INFO,
	       "\tW 0x%08x=[0x%08x]\n",
	       u4WrAddr, u4WrVal);

	u4WrAddr = CONN_HOST_CSR_TOP_ADDR_WF_VON_MONFLG_SEL_FR_HIF_ADDR;
	u4RdAddr = CONN_DBG_CTL_WF_VON_DEBUG_OUT_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tW 0x%08x=[0x%08x], R 0x%08x=[0x%08x]\n",
		       u4WrAddr, u4WrVal, u4RdAddr, u4Val);
	}
}

static void mt6639_dumpWfTopCfgon(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	uint32_t au4List[] = {
		WF_TOP_CFG_ON_DEBUG_FLAG0_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG1_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG2_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG3_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG4_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG5_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG6_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG7_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG8_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG9_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG10_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG11_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG12_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG13_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG14_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG15_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG16_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG17_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG18_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG19_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG20_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG21_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG22_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG23_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG24_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG25_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG26_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG27_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG28_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG29_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG30_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG31_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG32_ADDR,
		WF_TOP_CFG_ON_DEBUG_FLAG33_ADDR,
	};

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tR 0x%08x=[0x%08x]\n",
		       u4RdAddr, u4Val);
	}
}

void mt6639_dumpWfTopReg(struct ADAPTER *ad)
{
	/* Section A: Dump wf_top_misc_on monflag */
	mt6639_dumpWfTopMiscOn(ad);

	/* Section B: Dump wf_top_misc_von monflag */
	mt6639_dumpWfTopMiscVon(ad);

	/* Section C: Dump wf_top_cfg_on debug CR */
	mt6639_dumpWfTopCfgon(ad);
}

static void mt6639_dumpHostVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4WrVal = 0, u4Val = 0, u4Idx, u4RdAddr, u4WrAddr;
	uint32_t au4List[] = {
		0x00010001, 0x00020001, 0x00030001, 0x00040001, 0x00050001,
		0x00060001, 0x00070001, 0x00080001, 0x00090001, 0x00010002,
		0x00020002, 0x00030002, 0x00040002, 0x00050002
	};

	u4RdAddr = CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_BUS_TIMEOUT_IRQ_ADDR;
	u4WrAddr = CONN_DBG_CTL_WF_MCU_DBGOUT_SEL_ADDR;
	u4WrVal = 0x4;
	HAL_MCR_RD(ad, u4RdAddr, &u4Val);
	HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
	DBGLOG(HAL, INFO,
	       "\tR 0x%08x=[0x%08x], W 0x%08x=[0x%08x]\n",
	       u4RdAddr, u4Val, u4WrAddr, u4WrVal);

	u4WrAddr = CONN_DBG_CTL_WF_MCUSYS_INFRA_VDNR_GEN_DEBUG_CTRL_AO_DEBUGSYS_CTRL_ADDR;
	u4RdAddr = CONN_DBG_CTL_WF_MCU_GPR_BUS_DBGOUT_LOG_ADDR;
	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4WrVal = au4List[u4Idx];
		HAL_MCR_WR(ad, u4WrAddr, u4WrVal);
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tW 0x%08x=[0x%08x], R 0x%08x=[0x%08x]\n",
		       u4WrAddr, u4WrVal, u4RdAddr, u4Val);
	}
}

static void mt6639_dumpWfVdnrTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	uint32_t au4List[] = {
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT2_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT3_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT4_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT5_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT6_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT7_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT8_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT9_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT10_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT11_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT12_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT13_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT14_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_RESULT15_ADDR,
		WF_MCUSYS_VDNR_GEN_BUS_U_DEBUG_CTRL_AO_WFMCU_PWA_CTRL0_ADDR,
	};

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tR 0x%08x=[0x%08x]\n",
		       u4RdAddr, u4Val);
	}
}

static void mt6639_dumpAhbApbTimeoutInfo(struct ADAPTER *ad)
{
	uint32_t u4RdAddr, u4Val = 0, u4Idx;
	uint32_t au4List[] = {
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_ADDR_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_INFO_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_ID_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_LYR_ADDR,
		CONN_MCU_BUS_CR_AHB_APB_TIMEOUT_CTRL_ADDR
	};

	for (u4Idx = 0; u4Idx < ARRAY_SIZE(au4List); u4Idx++) {
		u4RdAddr = au4List[u4Idx];
		HAL_MCR_RD(ad, u4RdAddr, &u4Val);
		DBGLOG(HAL, INFO,
		       "\tR 0x%08x=[0x%08x]\n",
		       u4RdAddr, u4Val);
	}
}

void mt6639_dumpWfBusReg(struct ADAPTER *ad)
{
	/* Section A: Dump VDNR timeout host side info */
	mt6639_dumpHostVdnrTimeoutInfo(ad);

	/* Section B: Dump VDNR timeout wf side info */
	mt6639_dumpWfVdnrTimeoutInfo(ad);

	/* Section C: Dump AHB APB timeout info */
	mt6639_dumpAhbApbTimeoutInfo(ad);
}

static void mt6639_dumpConninfraBus(struct ADAPTER *ad)
{
	uint32_t WFDrvOwnStat = 0, MDDrvOwnStat = 0;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
#endif

	if (!ad) {
		DBGLOG(HAL, ERROR, "NULL ADAPTER.\n");
		return;
	}

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	prDebugOps = ad->chip_info->prDebugOps;
		if (prDebugOps && prDebugOps->checkDumpViaBt)
			dumpViaBt = prDebugOps->checkDumpViaBt();

	connv3_conninfra_bus_dump(dumpViaBt ?
		CONNV3_DRV_TYPE_BT : CONNV3_DRV_TYPE_WIFI);

	HAL_MCR_RD(ad, CONNAC3X_BN0_LPCTL_ADDR, &WFDrvOwnStat);
	HAL_MCR_RD(ad, CONNAC3X_BN0_LPCTL_MD_ADDR, &MDDrvOwnStat);
	DBGLOG(HAL, INFO, "WF DrvOwn stat=0x%08x, MD DrvOwn stat=0x%08x.\n",
		WFDrvOwnStat, MDDrvOwnStat);
#endif
}

void mt6639_DumpBusHangCr(struct ADAPTER *ad)
{
	struct GL_HIF_INFO *prHifInfo = NULL;
	struct BUS_INFO *prBusInfo = NULL;
	bool readable = TRUE;
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	struct CHIP_DBG_OPS *prDebugOps = NULL;
	bool dumpViaBt = FALSE;
	int ret = 0;
#endif

	if (!ad) {
		DBGLOG(HAL, ERROR, "NULL ADAPTER.\n");
		return;
	}

	prBusInfo = ad->chip_info->bus_info;
	prHifInfo = &ad->prGlueInfo->rHifInfo;
	prHifInfo->fgForceReadWriteReg = true;

	if (prBusInfo->dumpPcieStatus)
		readable = prBusInfo->dumpPcieStatus(ad->prGlueInfo);

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	prDebugOps = ad->chip_info->prDebugOps;
	if (prDebugOps && prDebugOps->checkDumpViaBt)
		dumpViaBt = prDebugOps->checkDumpViaBt();

	if (readable == FALSE && !dumpViaBt)
		goto exit;
#else
	if (readable == FALSE)
		goto exit;
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (dumpViaBt) {
		/* Notify BT to start */
		ret = connv3_hif_dbg_start(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT);
		if (ret != 0) {
			DBGLOG(HAL, ERROR, "connv3_hif_dbg_start failed.\n");
			goto exit_debug_sop;
		} else
			DBGLOG(HAL, INFO, "start BT dump.\n");
	}
#endif

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	mt6639_dumpConninfraBus(ad);
	if (dumpViaBt)
		mt6639_dumpPcieReg();
#endif

	mt6639_dumpCbtopReg(ad);
	mt6639_dumpWfsyscpupcr(ad);
	mt6639_dumpPcGprLog(ad);
	mt6639_dumpN45CoreReg(ad);
	mt6639_dumpWfTopReg(ad);
	mt6639_dumpWfBusReg(ad);

#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
	if (dumpViaBt) {
		/* Notify BT to end */
		ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_WIFI,
			CONNV3_DRV_TYPE_BT);
		if (ret != 0)
			DBGLOG(HAL, ERROR, "connv3_hif_dbg_end failed.\n");
	}

exit_debug_sop:
	if (dumpViaBt) {
		DBGLOG(HAL, INFO, "Trigger SCP dump.\n");
		connectivity_export_conap_scp_trigger_cmd(CONN_HIF_DBG_WF,
			CONN_HIF_DBG_CMD_PCIE, 0);

		/* delay 200 ms for async dump */
		msleep(200);
		fgTriggerDebugSop = FALSE;
	}

#endif

exit:
	prHifInfo->fgForceReadWriteReg = false;
}
#endif

#ifdef CFG_SUPPORT_LINK_QUALITY_MONITOR
int mt6639_get_rx_rate_info(const uint32_t *prRxV,
		struct RxRateInfo *prRxRateInfo)
{
	uint32_t rxmode = 0, rate = 0, frmode = 0, sgi = 0, nsts = 0;
	uint32_t stbc = 0, nss = 0;
	uint32_t mu_mimo = 0;

	if (!prRxRateInfo || !prRxV)
		return -1;

	rate = RXV_GET_RX_RATE(prRxV[0]);
	nsts = RXV_GET_RX_NSTS(prRxV[0]);
	mu_mimo = RXV_GET_RX_MUMIMO(prRxV[0]);
	rxmode = RXV_GET_TXMODE(prRxV[2]);
	frmode = RXV_GET_FR_MODE(prRxV[2]);
	sgi = RXV_GET_GI(prRxV[2]);
	stbc = RXV_GET_STBC(prRxV[2]);

	/* HE-SU: set to the number of space time streams minus 1
	 * HE_ER: 0 for 1 space time stream when STBC == 0
	 *        1 for 2 space time stream when STBC == 1
	 * HE_MU MU-MIMO: set to the number of space time streams (no minus 1);
	 * HE_MU Non-MU-MIMO: set to the number of space time streams minus 1
	 */
	if (!(rxmode == TX_RATE_MODE_HE_MU && mu_mimo))
		nsts += 1;

	if (nsts == 1)
		nss = nsts;
	else
		nss = stbc ? (nsts >> 1) : nsts;

	if (frmode > 5) { /* 320M */
		DBGLOG(SW4, ERROR, "frmode error: %u\n", frmode);
		return -1;
	}

	prRxRateInfo->u4Rate = rate;
	prRxRateInfo->u4Nss = nss;
	prRxRateInfo->u4Mode = rxmode;
	prRxRateInfo->u4Bw = frmode;
	prRxRateInfo->u4Gi = sgi;

	DBGLOG_LIMITED(SW4, TRACE,
		   "rxvec0=0x%x rxmode=%u, rate=%u, bw=%u, sgi=%u, nss=%u, mu_mimo=%u\n",
		   prRxV[0], rxmode, rate, frmode, sgi, nss, mu_mimo);

	return 0;
}
#endif

void mt6639_get_rx_link_stats(struct ADAPTER *prAdapter,
	struct SW_RFB *prSwRfb, uint32_t *pu4RxV)
{
#if CFG_SUPPORT_LLS
	static const uint8_t TX_MODE_2_LLS_MODE[] = {
		LLS_MODE_CCK,
		LLS_MODE_OFDM,	/* LG OFDM */
		LLS_MODE_HT,	/* MM */
		LLS_MODE_HT,	/* GF */
		LLS_MODE_VHT,
		LLS_MODE_RESERVED,
		LLS_MODE_RESERVED,
		LLS_MODE_RESERVED,
		LLS_MODE_HE,	/* HE-SU */
		LLS_MODE_HE,	/* HE-ER */
		LLS_MODE_HE,	/* HE-TB */
		LLS_MODE_HE,	/* HE-MU */
		LLS_MODE_RESERVED,
		LLS_MODE_EHT,	/* EHT_EXT */
		LLS_MODE_EHT,	/* EHT_TRIG */
		LLS_MODE_EHT,	/* EHT_MU */
	};
	static const uint8_t OFDM_RATE[STATS_LLS_OFDM_NUM] = {
						6, 4, 2, 0, 7, 5, 3, 1};
		/* report &= 7:  3  7  2   6   1   5   0    4 */
		/* Mbps       : 6M 9M 12M 18M 24M 36M 48M 54M */
		/* in 0.5 Mbps: 12 18 24  36  48  72  96  108 */
		/* Save format:  0  1  2   3   4   5   6    7 */
	struct STATS_LLS_WIFI_RATE rate = {0};
	struct STA_RECORD *prStaRec;
	uint32_t mcsIdx;

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		DBGLOG(RX, INFO, "RXV: pmbl=%u nsts=%u stbc=%u bw=%u mcs=%u",
			RXV_GET_TXMODE(pu4RxV[2]),
			RXV_GET_RX_NSTS(pu4RxV[0]),
			RXV_GET_STBC(pu4RxV[2]),
			RXV_GET_FR_MODE(pu4RxV[2]),
			RXV_GET_RX_RATE(pu4RxV[0]));

	if (!(prSwRfb->ucPayloadFormat == RX_PAYLOAD_FORMAT_MSDU ||
		prSwRfb->ucPayloadFormat == RX_PAYLOAD_FORMAT_FIRST_SUB_AMSDU))
		return;

	rate.preamble = TX_MODE_2_LLS_MODE[RXV_GET_TXMODE(pu4RxV[2])];

	if (rate.preamble == LLS_MODE_RESERVED)
		return;

	rate.bw = RXV_GET_FR_MODE(pu4RxV[2]);
	rate.nss = RXV_GET_RX_NSTS(pu4RxV[0]);
	if (rate.preamble >= LLS_MODE_VHT) {
		if (RXV_GET_STBC(pu4RxV[2]))
			rate.nss /= 2;
	}

	rate.rateMcsIdx = RXV_GET_RX_RATE(pu4RxV[0]) & 0xF; /* 0 ~ 15 */
	if (rate.preamble == LLS_MODE_CCK)
		rate.rateMcsIdx &= 0x3; /* 0: 1M; 1: 2M; 2: 5.5M; 3: 11M  */
	else if (rate.preamble == LLS_MODE_OFDM)
		rate.rateMcsIdx = OFDM_RATE[(uint8_t)(rate.rateMcsIdx & 0x7)];
	mcsIdx = rate.rateMcsIdx;

	if (rate.nss >= STATS_LLS_MAX_NSS_NUM)
		goto wrong_rate;

	prStaRec = cnmGetStaRecByIndex(prAdapter, prSwRfb->ucStaRecIdx);
	if (!prStaRec) {
		DBGLOG(RX, WARN, "StaRec %u not found", prSwRfb->ucStaRecIdx);
		goto wrong_rate;
	}

	if (rate.preamble == LLS_MODE_OFDM) {
		if (mcsIdx >= STATS_LLS_OFDM_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduOFDM[0][0][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_CCK) {
		if (mcsIdx >= STATS_LLS_CCK_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduCCK[0][0][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_HT) {
		if (rate.bw >= STATS_LLS_MAX_HT_BW_NUM ||
				mcsIdx >= STATS_LLS_HT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduHT[0][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_VHT) {
		if (rate.bw >= STATS_LLS_MAX_VHT_BW_NUM ||
				mcsIdx >= STATS_LLS_VHT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduVHT[rate.nss][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_HE) {
		if (rate.bw >= STATS_LLS_MAX_HE_BW_NUM ||
				mcsIdx >= STATS_LLS_HE_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduHE[rate.nss][rate.bw][mcsIdx]++;
	} else if (rate.preamble == LLS_MODE_EHT) {
		if (rate.bw >= STATS_LLS_MAX_EHT_BW_NUM ||
				mcsIdx >= STATS_LLS_EHT_NUM)
			goto wrong_rate;
		prStaRec->u4RxMpduEHT[rate.nss][rate.bw][mcsIdx]++;
	}

	if (prAdapter->rWifiVar.fgLinkStatsDump)
		DBGLOG(RX, INFO, "rate preamble=%u, nss=%u, bw=%u, mcsIdx=%u",
			rate.preamble, rate.nss, rate.bw, mcsIdx);
	return;

wrong_rate:
	DBGLOG_LIMITED(RX, WARN,
			"Invalid rate preamble=%u, nss=%u, bw=%u, mcsIdx=%u",
			rate.preamble, rate.nss, rate.bw, mcsIdx);
#endif
}

#endif /* MT6639 */
