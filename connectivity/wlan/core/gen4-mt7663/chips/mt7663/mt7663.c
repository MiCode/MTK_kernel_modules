/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   mt7663.c
 *  \brief  Internal driver stack will export the required procedures here
 *          for GLUE Layer.
 *
 *    This file contains all routines which are exported from MediaTek 802.11
 *    Wireless LAN driver stack to GLUE Layer.
 */

#ifdef MT7663

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "precomp.h"

#include "mt7663.h"
#include "pse.h"
#include "wf_ple.h"


/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */
#define SER_STATUS_CR             (0x820600d0)
#define PLE_STATUS_CR             (0x820600d4)
#define PSE_STATUS_CR             (0x820600d8)
#define LMAC_STATUS_CR            (0x820681e4)
#define PP_DMASHDL_STATUS_CR      (0x820682ec)

/*******************************************************************************
 *                                 M A C R O S
 *******************************************************************************
 */

/*******************************************************************************
 *                   F U N C T I O N   D E C L A R A T I O N S
 *******************************************************************************
 */

/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */


/*******************************************************************************
 *                            P U B L I C   D A T A
 *******************************************************************************
 */
struct ECO_INFO mt7663_eco_table[] = {
	/* HW version,  ROM version,    Factory version */
	{0x00, 0x00, 0x0A, 0x01},	/* E1 */
	{0x10, 0x01, 0x0A, 0x02},	/* E2 */
	{0x00, 0x00, 0x00, 0x00}	/* End of table */
};

#if defined(_HIF_PCIE)
struct PCIE_CHIP_CR_MAPPING mt7663_bus2chip_cr_mapping[] = {
	/* chip addr, bus addr, range */
	{0x80000000, 0x00002000, 0x00001000}, /* MCU_CFG */

	{0x50000000, 0x00004000, 0x00004000}, /* CONN_HIF (PDMA) */
	{0x50002000, 0x00005000, 0x00001000}, /* CONN_HIF (Reserved) */
	{0x5000A000, 0x00006000, 0x00001000}, /* CONN_HIF (DMASHDL) */
	{0x000E0000, 0x00007000, 0x00001000}, /* CONN_HIF_ON (HOST_CSR) */

	{0x82060000, 0x00008000, 0x00004000}, /* WF_UMAC_TOP (PLE) */
	{0x82068000, 0x0000C000, 0x00002000}, /* WF_UMAC_TOP (PSE) */
	{0x8206C000, 0x0000E000, 0x00002000}, /* WF_UMAC_TOP (PP) */
	{0x82070000, 0x00010000, 0x00010000}, /* WF_PHY */
	{0x820F0000, 0x00020000, 0x00010000}, /* WF_LMAC_TOP */
	{0x820E0000, 0x00030000, 0x00010000}, /* WF_LMAC_TOP (WF_WTBL) */

	{0x81000000, 0x00050000, 0x00010000}, /* BTSYS_OFF */
	{0x80070000, 0x00060000, 0x00010000}, /* GPSSYS */
	{0x40000000, 0x00070000, 0x00010000}, /* WF_SYSRAM */
	{0x00300000, 0x00080000, 0x00010000}, /* MCU_SYSRAM */

	{0x80010000, 0x000A1000, 0x00001000}, /* CONN_MCU_DMA */
	{0x80030000, 0x000A2000, 0x00001000}, /* CONN_MCU_BTIF0 */
	{0x81030000, 0x000A3000, 0x00001000}, /* CONN_MCU_CFG_ON */
	{0x80050000, 0x000A4000, 0x00001000}, /* CONN_UART_PTA */
	{0x81040000, 0x000A5000, 0x00001000}, /* CONN_MCU_CIRQ */
	{0x81050000, 0x000A6000, 0x00001000}, /* CONN_MCU_GPT */
	{0x81060000, 0x000A7000, 0x00001000}, /* CONN_PTA */
	{0x81080000, 0x000A8000, 0x00001000}, /* CONN_MCU_WDT */
	{0x81090000, 0x000A9000, 0x00001000}, /* CONN_MCU_PDA */
	{0x810A0000, 0x000AA000, 0x00001000}, /* CONN_RDD_AHB_WRAP0 */
	{0x810B0000, 0x000AB000, 0x00001000}, /* BTSYS_ON */
	{0x810C0000, 0x000AC000, 0x00001000}, /* CONN_RBIST_TOP */
	{0x810D0000, 0x000AD000, 0x00001000}, /* CONN_RDD_AHB_WRAP0 */
	{0x820D0000, 0x000AE000, 0x00001000}, /* WFSYS_ON */
	{0x60000000, 0x000AF000, 0x00001000}, /* CONN_MCU_PDA */

	{0x80020000, 0x000B0000, 0x00010000}, /* CONN_TOP_MISC_OFF */
	{0x81020000, 0x000C0000, 0x00010000}, /* CONN_TOP_MISC_ON */
	{0x7c030000, 0x000F0000, 0x00010000}, /* CONN_TOP_MISC_ON */

	{0x0, 0x0, 0x0}
};
#endif /* _HIF_PCIE */

void
mt7663ConstructFirmwarePrio(struct GLUE_INFO *prGlueInfo,
	uint8_t **apucNameTable, uint8_t **apucName,
	uint8_t *pucNameIdx, uint8_t ucMaxNameIdx)
{
	struct mt66xx_chip_info *prChipInfo = prGlueInfo->prAdapter->chip_info;
	uint32_t chip_id = prChipInfo->chip_id;
	uint8_t sub_idx = 0;

	for (sub_idx = 0; apucNameTable[sub_idx]; sub_idx++) {
		if (((*pucNameIdx) + 3) < ucMaxNameIdx) {
			/* Type 1. WIFI_RAM_CODE_MTxxxx.bin */
			snprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s%x.bin",
				apucNameTable[sub_idx], chip_id);
			(*pucNameIdx) += 1;

			/* Type 2. WIFI_RAM_CODE_MTxxxx */
			snprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s%x",
				apucNameTable[sub_idx], chip_id);
			(*pucNameIdx) += 1;

			/* Type 3. WIFI_RAM_CODE_MTxxxx_Ex.bin */
			snprintf(*(apucName + (*pucNameIdx)),
					CFG_FW_NAME_MAX_LEN, "%s%x_E%u.bin",
					apucNameTable[sub_idx], chip_id,
					wlanGetEcoVersion(
					prGlueInfo->prAdapter));
			(*pucNameIdx) += 1;

			/* Type 4. WIFI_RAM_CODE_MTxxxx_Ex */
			snprintf(*(apucName + (*pucNameIdx)),
				CFG_FW_NAME_MAX_LEN, "%s%x_E%u",
					apucNameTable[sub_idx], chip_id,
					wlanGetEcoVersion(
					prGlueInfo->prAdapter));
			(*pucNameIdx) += 1;
		} else {
			/* the table is not large enough */
			DBGLOG(INIT, ERROR,
				"kalFirmwareImageMapping >> file name array is not enough.\n");
			ASSERT(0);
		}
	}
}

void mt7663DumpSerDummyCR(
	IN struct ADAPTER *prAdapter)
{
	uint32_t u4MacVal;

	DBGLOG(HAL, INFO, "%s\n", __func__);

	DBGLOG(HAL, INFO, "=====Dump Start====\n");

	HAL_MCR_RD(prAdapter, SER_STATUS_CR, &u4MacVal);
	DBGLOG(HAL, INFO, "SER STATUS[0x%08x]: 0x%08x\n",
	       SER_STATUS_CR, u4MacVal);

	HAL_MCR_RD(prAdapter, PLE_STATUS_CR, &u4MacVal);
	DBGLOG(HAL, INFO, "PLE STATUS[0x%08x]: 0x%08x\n",
	       PLE_STATUS_CR, u4MacVal);

	HAL_MCR_RD(prAdapter, PSE_STATUS_CR, &u4MacVal);
	DBGLOG(HAL, INFO, "PSE STATUS[0x%08x]: 0x%08x\n",
	       PSE_STATUS_CR, u4MacVal);

	HAL_MCR_RD(prAdapter, LMAC_STATUS_CR, &u4MacVal);
	DBGLOG(HAL, INFO, "LMAC STATUS[0x%08x]: 0x%08x\n",
	       LMAC_STATUS_CR, u4MacVal);

	HAL_MCR_RD(prAdapter, PP_DMASHDL_STATUS_CR, &u4MacVal);
	DBGLOG(HAL, INFO, "PP DMASHDL STATUS[0x%08x]: 0x%08x\n",
	       PP_DMASHDL_STATUS_CR, u4MacVal);

	DBGLOG(HAL, INFO, "=====Dump End====\n");

}
uint32_t mt7663GetDongleType(struct ADAPTER *prAdapter)
{
	return MT7663_CHIP_ID;
}

void halShowPseInfo(IN struct ADAPTER *prAdapter)
{
	uint32_t pse_buf_ctrl = 0, pg_sz, pg_num;
	uint32_t pse_stat = 0, pg_flow_ctrl[16] = {0};
	uint32_t fpg_cnt, ffa_cnt, fpg_head, fpg_tail;
	uint32_t max_q, min_q, rsv_pg, used_pg;
	uint32_t i, page_offset, addr, value = 0;

	HAL_MCR_RD(prAdapter, PSE_PBUF_CTRL, &pse_buf_ctrl);
	HAL_MCR_RD(prAdapter, PSE_QUEUE_EMPTY, &pse_stat);
	HAL_MCR_RD(prAdapter, PSE_FREEPG_CNT, &pg_flow_ctrl[0]);
	HAL_MCR_RD(prAdapter, PSE_FREEPG_HEAD_TAIL, &pg_flow_ctrl[1]);
	HAL_MCR_RD(prAdapter, PSE_PG_HIF0_GROUP, &pg_flow_ctrl[2]);
	HAL_MCR_RD(prAdapter, PSE_HIF0_PG_INFO, &pg_flow_ctrl[3]);
	HAL_MCR_RD(prAdapter, PSE_PG_HIF1_GROUP, &pg_flow_ctrl[4]);
	HAL_MCR_RD(prAdapter, PSE_HIF1_PG_INFO, &pg_flow_ctrl[5]);
	HAL_MCR_RD(prAdapter, PSE_PG_CPU_GROUP, &pg_flow_ctrl[6]);
	HAL_MCR_RD(prAdapter, PSE_CPU_PG_INFO, &pg_flow_ctrl[7]);
	HAL_MCR_RD(prAdapter, PSE_PG_LMAC0_GROUP, &pg_flow_ctrl[8]);
	HAL_MCR_RD(prAdapter, PSE_LMAC0_PG_INFO, &pg_flow_ctrl[9]);
	HAL_MCR_RD(prAdapter, PSE_PG_LMAC1_GROUP, &pg_flow_ctrl[10]);
	HAL_MCR_RD(prAdapter, PSE_LMAC1_PG_INFO, &pg_flow_ctrl[11]);
	HAL_MCR_RD(prAdapter, PSE_PG_LMAC2_GROUP, &pg_flow_ctrl[12]);
	HAL_MCR_RD(prAdapter, PSE_LMAC2_PG_INFO, &pg_flow_ctrl[13]);
	HAL_MCR_RD(prAdapter, PSE_PG_PLE_GROUP, &pg_flow_ctrl[14]);
	HAL_MCR_RD(prAdapter, PSE_PLE_PG_INFO, &pg_flow_ctrl[15]);

	/* Configuration Info */
	DBGLOG(HAL, INFO, "PSE Configuration Info:\n");

	HAL_MCR_RD(prAdapter, PSE_GC, &value);
	DBGLOG(HAL, INFO, "\tGC(0x82068000): 0x%08x\n", value);
	HAL_MCR_RD(prAdapter, PSE_INT_STS, &value);
	DBGLOG(HAL, INFO, "\tINT_STS(0x82068024): 0x%08x\n", value);
	HAL_MCR_RD(prAdapter, PSE_INT_ERR_STS, &value);
	DBGLOG(HAL, INFO, "\tINT_ERR_STS(0x82068028): 0x%08x\n", value);

	DBGLOG(HAL, INFO, "\tPacket Buffer Control(0x82068014): 0x%08x\n",
		pse_buf_ctrl);
	pg_sz = (pse_buf_ctrl & (0x1 << 31)) >> 31;
	DBGLOG(HAL, INFO,
		"\t\tPage Size=%d(%d bytes per page)\n",
		pg_sz, (pg_sz == 1 ? 256 : 128));
	page_offset  = (pse_buf_ctrl & 0x3FFFFFF) >> 17;
	DBGLOG(HAL, INFO,
		 "\t\tPage Offset=%d(in unit of 2KB)\n", page_offset);
	pg_num = (pse_buf_ctrl & PSE_TOTAL_PAGE_NUM_MASK);
	DBGLOG(HAL, INFO, "\t\tAvailable Total Page=%d pages\n", pg_num);
	/* Page Flow Control */
	DBGLOG(HAL, INFO, "PSE Page Flow Control:\n");

	DBGLOG(HAL, INFO,
		"\tFree page counter(0x82068100): 0x%08x\n", pg_flow_ctrl[0]);
	fpg_cnt = pg_flow_ctrl[0] & 0xfff;
	DBGLOG(HAL, INFO,
		"\t\tThe toal page number of free=0x%03x\n", fpg_cnt);
	ffa_cnt = (pg_flow_ctrl[0] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe free page numbers of free for all=0x%03x\n", ffa_cnt);
	DBGLOG(HAL, INFO,
		"\tFree page head and tail(0x82068104): 0x%08x\n",
		pg_flow_ctrl[1]);
	fpg_head = pg_flow_ctrl[1] & 0xfff;
	fpg_tail = (pg_flow_ctrl[1] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe tail/head page of free page list=0x%03x/0x%03x\n",
		fpg_tail, fpg_head);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of HIF0 group(0x82068110): 0x%08x\n",
		pg_flow_ctrl[2]);
	DBGLOG(HAL, INFO,
		"\tHIF0 group page status(0x82068114): 0x%08x\n",
		pg_flow_ctrl[3]);
	min_q = pg_flow_ctrl[2] & 0xfff;
	max_q = (pg_flow_ctrl[2] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of HIF0 group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[3] & 0xfff;
	used_pg = (pg_flow_ctrl[3] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of HIF0 group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of HIF1 group(0x82068118): 0x%08x\n",
		pg_flow_ctrl[4]);
	DBGLOG(HAL, INFO,
		"\tHIF1 group page status(0x8206811c): 0x%08x\n",
		pg_flow_ctrl[5]);
	min_q = pg_flow_ctrl[4] & 0xfff;
	max_q = (pg_flow_ctrl[4] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of HIF1 group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[5] & 0xfff;
	used_pg = (pg_flow_ctrl[5] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of HIF1 group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of CPU group(0x82068150): 0x%08x\n",
		pg_flow_ctrl[6]);
	DBGLOG(HAL, INFO,
		"\tCPU group page status(0x82068154): 0x%08x\n",
		pg_flow_ctrl[7]);
	min_q = pg_flow_ctrl[6] & 0xfff;
	max_q = (pg_flow_ctrl[6] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of CPU group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[7] & 0xfff;
	used_pg = (pg_flow_ctrl[7] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of CPU group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of LMAC0 group(0x82068170): 0x%08x\n",
		pg_flow_ctrl[8]);
	DBGLOG(HAL, INFO,
		"\tLMAC0 group page status(0x82068174): 0x%08x\n",
		pg_flow_ctrl[9]);
	min_q = pg_flow_ctrl[8] & 0xfff;
	max_q = (pg_flow_ctrl[8] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of LMAC0 group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[9] & 0xfff;
	used_pg = (pg_flow_ctrl[9] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of LMAC0 group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of LMAC1 group(0x82068178): 0x%08x\n",
		pg_flow_ctrl[10]);
	DBGLOG(HAL, INFO,
		"\tLMAC1 group page status(0x8206817c): 0x%08x\n",
		pg_flow_ctrl[11]);
	min_q = pg_flow_ctrl[10] & 0xfff;
	max_q = (pg_flow_ctrl[10] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of LMAC1 group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[11] & 0xfff;
	used_pg = (pg_flow_ctrl[11] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of LMAC1 group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of LMAC2 group(0x82068180): 0x%08x\n",
		pg_flow_ctrl[12]);
	DBGLOG(HAL, INFO,
		"\tLMAC2 group page status(0x82068184): 0x%08x\n",
		pg_flow_ctrl[13]);
	min_q = pg_flow_ctrl[12] & 0xfff;
	max_q = (pg_flow_ctrl[12] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of LMAC2 group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[13] & 0xfff;
	used_pg = (pg_flow_ctrl[13] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of LMAC2 group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of PLE group(0x82068190): 0x%08x\n",
		pg_flow_ctrl[14]);
	DBGLOG(HAL, INFO,
		"\tPLE group page status(0x82068194): 0x%08x\n",
		pg_flow_ctrl[15]);
	min_q = pg_flow_ctrl[14] & 0xfff;
	max_q = (pg_flow_ctrl[14] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of PLE group=0x%03x/0x%03x\n",
		max_q, min_q);
	rsv_pg = pg_flow_ctrl[15] & 0xfff;
	used_pg = (pg_flow_ctrl[15] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of PLE group=0x%03x/0x%03x\n",
		used_pg, rsv_pg);
	/* Queue Empty Status */
	DBGLOG(HAL, INFO, "PSE Queue Empty Status:\n");
	DBGLOG(HAL, INFO,
		"\tQUEUE_EMPTY(0x820680b0): 0x%08x\n", pse_stat);
	DBGLOG(HAL, INFO,
		"\t\tCPU Q0/1/2/3 empty=%d/%d/%d/%d\n",
		 pse_stat & 0x1, ((pse_stat & 0x2) >> 1),
		 ((pse_stat & 0x4) >> 2), ((pse_stat & 0x8) >> 3));
	DBGLOG(HAL, INFO,
		"\t\tHIF Q0/1 empty=%d/%d\n",
		 ((pse_stat & (0x1 << 16)) >> 16),
		 ((pse_stat & (0x1 << 17)) >> 17));
	DBGLOG(HAL, INFO,
		"\t\tLMAC TX Q empty=%d\n",
		 ((pse_stat & (0x1 << 24)) >> 24));
	DBGLOG(HAL, INFO,
		"\t\tRLS_Q empty=%d\n",
		 ((pse_stat & (0x1 << 31)) >> 31));
	DBGLOG(HAL, INFO, "Nonempty Q info:\n");

	for (i = 0; i < 31; i++) {
		if (((pse_stat & (0x1 << i)) >> i) == 0) {
			uint32_t hfid, tfid, pktcnt, fl_que_ctrl[3] = {0};

			if (i < 4) {
				DBGLOG(HAL, INFO,
						 "\tCPU Q%d: ", i);
				fl_que_ctrl[0] |= (0x1 << 14);
				fl_que_ctrl[0] |= (i << 8);
			} else if (i == 16) {
				DBGLOG(HAL, INFO, "\tHIF Q0: ");
				fl_que_ctrl[0] |= (0x0 << 14);
				fl_que_ctrl[0] |= (0x0 << 8);
			} else if (i == 17) {
				DBGLOG(HAL, INFO, "\tHIF  Q1: ");
				fl_que_ctrl[0] |= (0x0 << 14);
				fl_que_ctrl[0] |= (0x1 << 8);
			} else if (i == 24) {
				DBGLOG(HAL, INFO, "\tLMAC TX Q: ");
				fl_que_ctrl[0] |= (0x2 << 14);
				fl_que_ctrl[0] |= (0x0 << 8);
			} else if (i == 31) {
				DBGLOG(HAL, INFO, "\tRLS Q: ");
				fl_que_ctrl[0] |= (0x3 << 14);
				fl_que_ctrl[0] |= (i << 8);
			} else
				continue;

			fl_que_ctrl[0] |= (0x1 << 31);
			HAL_MCR_WR(prAdapter, PSE_FL_QUE_CTRL_0,
				fl_que_ctrl[0]);
			HAL_MCR_RD(prAdapter, PSE_FL_QUE_CTRL_2,
				&fl_que_ctrl[1]);
			HAL_MCR_RD(prAdapter, PSE_FL_QUE_CTRL_3,
				&fl_que_ctrl[2]);
			hfid = fl_que_ctrl[1] & 0xfff;
			tfid = (fl_que_ctrl[1] & 0xfff << 16) >> 16;
			pktcnt = fl_que_ctrl[2] & 0xfff;
			DBGLOG(HAL, INFO,
				"tail/head fid = 0x%03x/0x%03x, pkt cnt = %x\n",
				tfid, hfid, pktcnt);
		}
	}

	for (i = 0; i < PSE_PEEK_CR_NUM; i++) {
		if (i == 0x7)
			continue;
		addr = PSE_PEEK_CR_0 + PSE_PEEK_CR_OFFSET * i;
		HAL_MCR_RD(prAdapter, addr, &value);
		DBGLOG(HAL, INFO, "PSE_PEEK_CR_%u[0x%08x/0x%08x]\n",
		       i, addr, value);
	}

	for (i = 0; i < PSE_ENDEQ_NUM; i++) {
		addr = PSE_ENQ_0 + PSE_ENDEQ_OFFSET * i;
		HAL_MCR_RD(prAdapter, addr, &value);
		DBGLOG(HAL, INFO, "PSE_ENQ_%u[0x%08x/0x%08x]\n",
		       i, addr, value);
	}

	for (i = 0; i < PSE_ENDEQ_NUM; i++) {
		addr = PSE_DEQ_0 + PSE_ENDEQ_OFFSET * i;
		HAL_MCR_RD(prAdapter, addr, &value);
		DBGLOG(HAL, INFO, "PSE_DEQ_%u[0x%08x/0x%08x]\n",
		       i, addr, value);
	}
}

#define UMAC_FID_FAULT	0xFFF
#define DUMP_MEM_SIZE 64
static int8_t *sta_ctrl_reg[] = {"ENABLE", "*DISABLE", "*PAUSE"};
static struct EMPTY_QUEUE_INFO Queue_Empty_info[] = {
	{"CPU Q0",  ENUM_UMAC_CPU_PORT_1,     ENUM_UMAC_CTX_Q_0},
	{"CPU Q1",  ENUM_UMAC_CPU_PORT_1,     ENUM_UMAC_CTX_Q_1},
	{"CPU Q2",  ENUM_UMAC_CPU_PORT_1,     ENUM_UMAC_CTX_Q_2},
	{"CPU Q3",  ENUM_UMAC_CPU_PORT_1,     ENUM_UMAC_CTX_Q_3},
	{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0},
	{"ALTX Q0", ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_ALTX_0},
	{"BMC Q0",  ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_BMC_0},
	{"BCN Q0",  ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_BNC_0},
	{"PSMP Q0", ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_PSMP_0},
	{"ALTX Q1", ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_ALTX_1},
	{"BMC Q1",  ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_BMC_1},
	{"BCN Q1",  ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_BNC_1},
	{"PSMP Q1", ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_PSMP_1},
	{"NAF Q",   ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_NAF},
	{"NBCN Q",  ENUM_UMAC_LMAC_PORT_2,    ENUM_UMAC_LMAC_PLE_TX_Q_NBCN},
	{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0},
	{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0},
	{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0},
	{NULL, 0, 0}, {NULL, 0, 0}, {NULL, 0, 0},
	{"RLS Q",   ENUM_PLE_CTRL_PSE_PORT_3, ENUM_UMAC_PLE_CTRL_P3_Q_0X1E},
	{"RLS2 Q",  ENUM_PLE_CTRL_PSE_PORT_3, ENUM_UMAC_PLE_CTRL_P3_Q_0X1F}
};

void halShowPleInfo(IN struct ADAPTER *prAdapter)
{
	uint32_t ple_buf_ctrl[3] = {0}, pg_sz, pg_num, bit_field_1, bit_field_2;
	uint32_t ple_stat[17] = {0}, pg_flow_ctrl[6] = {0};
	uint32_t sta_pause[4] = {0}, dis_sta_map[4] = {0};
	uint32_t fpg_cnt, ffa_cnt, fpg_head, fpg_tail, hif_max_q, hif_min_q;
	uint32_t rpg_hif, upg_hif, cpu_max_q, cpu_min_q, rpg_cpu, upg_cpu;
	uint32_t i, j, addr, value;

	HAL_MCR_RD(prAdapter, PLE_PBUF_CTRL, &ple_buf_ctrl[0]);
	HAL_MCR_RD(prAdapter, PLE_RELEASE_CTRL, &ple_buf_ctrl[1]);
	HAL_MCR_RD(prAdapter, PLE_HIF_REPORT, &ple_buf_ctrl[2]);
	HAL_MCR_RD(prAdapter, PLE_QUEUE_EMPTY, &ple_stat[0]);
	HAL_MCR_RD(prAdapter, PLE_AC0_QUEUE_EMPTY_0, &ple_stat[1]);
	HAL_MCR_RD(prAdapter, PLE_AC0_QUEUE_EMPTY_1, &ple_stat[2]);
	HAL_MCR_RD(prAdapter, PLE_AC0_QUEUE_EMPTY_2, &ple_stat[3]);
	HAL_MCR_RD(prAdapter, PLE_AC0_QUEUE_EMPTY_3, &ple_stat[4]);
	HAL_MCR_RD(prAdapter, PLE_AC1_QUEUE_EMPTY_0, &ple_stat[5]);
	HAL_MCR_RD(prAdapter, PLE_AC1_QUEUE_EMPTY_1, &ple_stat[6]);
	HAL_MCR_RD(prAdapter, PLE_AC1_QUEUE_EMPTY_2, &ple_stat[7]);
	HAL_MCR_RD(prAdapter, PLE_AC1_QUEUE_EMPTY_3, &ple_stat[8]);
	HAL_MCR_RD(prAdapter, PLE_AC2_QUEUE_EMPTY_0, &ple_stat[9]);
	HAL_MCR_RD(prAdapter, PLE_AC2_QUEUE_EMPTY_1, &ple_stat[10]);
	HAL_MCR_RD(prAdapter, PLE_AC2_QUEUE_EMPTY_2, &ple_stat[11]);
	HAL_MCR_RD(prAdapter, PLE_AC2_QUEUE_EMPTY_3, &ple_stat[12]);
	HAL_MCR_RD(prAdapter, PLE_AC3_QUEUE_EMPTY_0, &ple_stat[13]);
	HAL_MCR_RD(prAdapter, PLE_AC3_QUEUE_EMPTY_1, &ple_stat[14]);
	HAL_MCR_RD(prAdapter, PLE_AC3_QUEUE_EMPTY_2, &ple_stat[15]);
	HAL_MCR_RD(prAdapter, PLE_AC3_QUEUE_EMPTY_3, &ple_stat[16]);
	HAL_MCR_RD(prAdapter, PLE_FREEPG_CNT, &pg_flow_ctrl[0]);
	HAL_MCR_RD(prAdapter, PLE_FREEPG_HEAD_TAIL, &pg_flow_ctrl[1]);
	HAL_MCR_RD(prAdapter, PLE_PG_HIF_GROUP, &pg_flow_ctrl[2]);
	HAL_MCR_RD(prAdapter, PLE_HIF_PG_INFO, &pg_flow_ctrl[3]);
	HAL_MCR_RD(prAdapter, PLE_PG_CPU_GROUP, &pg_flow_ctrl[4]);
	HAL_MCR_RD(prAdapter, PLE_CPU_PG_INFO, &pg_flow_ctrl[5]);
	HAL_MCR_RD(prAdapter, DIS_STA_MAP0, &dis_sta_map[0]);
	HAL_MCR_RD(prAdapter, DIS_STA_MAP1, &dis_sta_map[1]);
	HAL_MCR_RD(prAdapter, DIS_STA_MAP2, &dis_sta_map[2]);
	HAL_MCR_RD(prAdapter, DIS_STA_MAP3, &dis_sta_map[3]);
	HAL_MCR_RD(prAdapter, STATION_PAUSE0, &sta_pause[0]);
	HAL_MCR_RD(prAdapter, STATION_PAUSE1, &sta_pause[1]);
	HAL_MCR_RD(prAdapter, STATION_PAUSE2, &sta_pause[2]);
	HAL_MCR_RD(prAdapter, STATION_PAUSE3, &sta_pause[3]);
	/* Configuration Info */
	DBGLOG(HAL, INFO, "PLE Configuration Info:\n");

	HAL_MCR_RD(prAdapter, PLE_GC, &value);
	DBGLOG(HAL, INFO, "\tGC(0x82060000): 0x%08x\n", value);
	HAL_MCR_RD(prAdapter, PLE_INT_STS, &value);
	DBGLOG(HAL, INFO, "\tINT_STS(0x82060024): 0x%08x\n", value);
	HAL_MCR_RD(prAdapter, PLE_INT_ERR_STS, &value);
	DBGLOG(HAL, INFO, "\tINT_ERR_STS(0x82060028): 0x%08x\n", value);

	DBGLOG(HAL, INFO,
		"\tPacket Buffer Control(0x82060014): 0x%08x\n",
		ple_buf_ctrl[0]);
	pg_sz = (ple_buf_ctrl[0] & (0x1 << 31)) >> 31;
	DBGLOG(HAL, INFO,
		"\t\tPage Size=%d(%d bytes per page)\n",
		pg_sz, (pg_sz == 1 ? 128 : 64));
	DBGLOG(HAL, INFO,
		"\t\tPage Offset=%d(in unit of 2KB)\n",
		(ple_buf_ctrl[0] & (0xf << 17)) >> 17);
	pg_num = (ple_buf_ctrl[0] & 0xfff);
	DBGLOG(HAL, INFO,
		"\t\tTotal Page=%d pages\n", (ple_buf_ctrl[0] & 0xfff));
	DBGLOG(HAL, INFO,
		"\tRelease Control(0x82060030): 0x%08x\n", ple_buf_ctrl[1]);
	bit_field_1 = (ple_buf_ctrl[1] & 0x1f);
	bit_field_2 = ((ple_buf_ctrl[1] & (0x3 << 6)) >> 6);
	DBGLOG(HAL, INFO,
		"\t\tNormalTx Release Pid/Qid=%d/%d\n",
		bit_field_2, bit_field_1);
	bit_field_1 = ((ple_buf_ctrl[1] & (0x1f << 8)) >> 8);
	bit_field_2 = ((ple_buf_ctrl[1] & (0x3 << 14)) >> 14);
	DBGLOG(HAL, INFO,
		"\t\tDropTx Release Pid/Qid=%d/%d\n", bit_field_2, bit_field_1);
	bit_field_1 = ((ple_buf_ctrl[1] & (0x1f << 16)) >> 16);
	bit_field_2 = ((ple_buf_ctrl[1] & (0x3 << 22)) >> 22);
	DBGLOG(HAL, INFO,
		"\t\tBCN0 Release Pid/Qid=%d/%d\n", bit_field_2, bit_field_1);
	bit_field_1 = ((ple_buf_ctrl[1] & (0x1f << 24)) >> 24);
	bit_field_2 = ((ple_buf_ctrl[1] & (0x3 << 30)) >> 30);
	DBGLOG(HAL, INFO,
		"\t\tBCN1 Release Pid/Qid=%d/%d\n", bit_field_2, bit_field_1);
	DBGLOG(HAL, INFO,
		"\tHIF Report Control(0x82060034): 0x%08x\n", ple_buf_ctrl[2]);
	bit_field_1 = ((ple_buf_ctrl[2] & (0x1 << 1)) >> 1);
	DBGLOG(HAL, INFO,
		"\t\tHostReportQSel/HostReportDisable=%d/%d\n",
			  (ple_buf_ctrl[2] & 0x1), bit_field_1);
	/* Page Flow Control */
	DBGLOG(HAL, INFO, "PLE Page Flow Control:\n");
	DBGLOG(HAL, INFO,
		"\tFree page counter(0x82060100): 0x%08x\n", pg_flow_ctrl[0]);
	fpg_cnt = pg_flow_ctrl[0] & 0xfff;
	DBGLOG(HAL, INFO,
		"\t\tThe toal page number of free=0x%03x\n", fpg_cnt);
	ffa_cnt = (pg_flow_ctrl[0] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe free page numbers of free for all=0x%03x\n", ffa_cnt);
	DBGLOG(HAL, INFO,
		"\tFree page head and tail(0x82060104): 0x%08x\n",
		pg_flow_ctrl[1]);
	fpg_head = pg_flow_ctrl[1] & 0xfff;
	fpg_tail = (pg_flow_ctrl[1] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe tail/head page of free page list=0x%03x/0x%03x\n",
		fpg_tail, fpg_head);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of HIF group(0x82060110): 0x%08x\n",
		pg_flow_ctrl[2]);
	DBGLOG(HAL, INFO,
		"\tHIF group page status(0x82060114): 0x%08x\n",
		pg_flow_ctrl[3]);
	hif_min_q = pg_flow_ctrl[2] & 0xfff;
	hif_max_q = (pg_flow_ctrl[2] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of HIF group=0x%03x/0x%03x\n",
		hif_max_q, hif_min_q);
	rpg_hif = pg_flow_ctrl[3] & 0xfff;
	upg_hif = (pg_flow_ctrl[3] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of HIF group=0x%03x/0x%03x\n",
		upg_hif, rpg_hif);
	DBGLOG(HAL, INFO,
		"\tReserved page counter of CPU group(0x82060150): 0x%08x\n",
		pg_flow_ctrl[4]);
	DBGLOG(HAL, INFO,
		"\tCPU group page status(0x82060154): 0x%08x\n",
		pg_flow_ctrl[5]);
	cpu_min_q = pg_flow_ctrl[4] & 0xfff;
	cpu_max_q = (pg_flow_ctrl[4] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe max/min quota pages of CPU group=0x%03x/0x%03x\n",
		cpu_max_q, cpu_min_q);
	rpg_cpu = pg_flow_ctrl[5] & 0xfff;
	upg_cpu = (pg_flow_ctrl[5] & (0xfff << 16)) >> 16;
	DBGLOG(HAL, INFO,
		"\t\tThe used/reserved pages of CPU group=0x%03x/0x%03x\n",
		upg_cpu, rpg_cpu);

	if (((ple_stat[0] & (0x1 << 24)) >> 24) == 0) {
		DBGLOG(HAL, INFO,
			"\tAC0_QUEUE_EMPTY0(0x82060300): 0x%08x\n",
			ple_stat[1]);
		DBGLOG(HAL, INFO,
			"\tAC1_QUEUE_EMPTY0(0x82060310): 0x%08x\n",
			ple_stat[5]);
		DBGLOG(HAL, INFO,
			"\tAC2_QUEUE_EMPTY0(0x82060320): 0x%08x\n",
			ple_stat[9]);
		DBGLOG(HAL, INFO,
			"\tAC3_QUEUE_EMPTY0(0x82060330): 0x%08x\n",
			ple_stat[13]);

		for (j = 0; j < 16; j = j + 4) {
			if (j % 4 == 0)
				DBGLOG(HAL, INFO, "\tNonempty AC%d Q of STA#:",
					j / 4);

			for (i = 0; i < 32; i++) {
				if (((ple_stat[j + 1] & (0x1 << i)) >> i) == 0)
					DBGLOG(HAL, INFO, "%d ",
						i + (j % 4) * 32);
			}
		}

		DBGLOG(HAL, INFO, "\n");
	}

	DBGLOG(HAL, INFO, "Nonempty Q info:\n");

	for (i = 0; i < 31; i++) {
		if (((ple_stat[0] & (0x1 << i)) >> i) == 0) {
			uint32_t hfid, tfid, pktcnt, fl_que_ctrl[3] = {0};

			if (Queue_Empty_info[i].QueueName != NULL) {
				DBGLOG(HAL, INFO, "\t%s: ",
					Queue_Empty_info[i].QueueName);
				fl_que_ctrl[0] |= (0x1 << 31);
				fl_que_ctrl[0] |=
					(Queue_Empty_info[i].Portid << 14);
				fl_que_ctrl[0] |=
					(Queue_Empty_info[i].Queueid << 8);
			} else
				continue;

			HAL_MCR_WR(prAdapter,
				PLE_FL_QUE_CTRL_0, fl_que_ctrl[0]);
			HAL_MCR_RD(prAdapter,
				PLE_FL_QUE_CTRL_2, &fl_que_ctrl[1]);
			HAL_MCR_RD(prAdapter,
				PLE_FL_QUE_CTRL_3, &fl_que_ctrl[2]);
			hfid = fl_que_ctrl[1] & 0xfff;
			tfid = (fl_que_ctrl[1] & 0xfff << 16) >> 16;
			pktcnt = fl_que_ctrl[2] & 0xfff;
			DBGLOG(HAL, INFO,
				"tail/head fid = 0x%03x/0x%03x, pkt cnt = %x\n",
				 tfid, hfid, pktcnt);
		}
	}

	for (j = 0; j < 16; j = j + 4) { /* show AC Q info */
		for (i = 0; i < 32; i++) {
			if (((ple_stat[j + 1] & (0x1 << i)) >> i) == 0) {
				uint32_t hfid, tfid, pktcnt,
					ac_num = j / 4, ctrl = 0;
				uint32_t sta_num = i + (j % 4) * 32,
					fl_que_ctrl[3] = {0};

				DBGLOG(HAL, INFO, "\tSTA%d AC%d: ",
					sta_num, ac_num);
				fl_que_ctrl[0] |= (0x1 << 31);
				fl_que_ctrl[0] |= (0x2 << 14);
				fl_que_ctrl[0] |= (ac_num << 8);
				fl_que_ctrl[0] |= sta_num;
				HAL_MCR_WR(prAdapter,
					PLE_FL_QUE_CTRL_0, fl_que_ctrl[0]);
				HAL_MCR_RD(prAdapter,
					PLE_FL_QUE_CTRL_2, &fl_que_ctrl[1]);
				HAL_MCR_RD(prAdapter,
					PLE_FL_QUE_CTRL_3, &fl_que_ctrl[2]);
				hfid = fl_que_ctrl[1] & 0xfff;
				tfid = (fl_que_ctrl[1] & 0xfff << 16) >> 16;
				pktcnt = fl_que_ctrl[2] & 0xfff;
				DBGLOG(HAL, INFO,
					"tail/head fid = 0x%03x/0x%03x, pkt cnt = %x",
					 tfid, hfid, pktcnt);

				if (((sta_pause[j % 4] & 0x1 << i) >> i) == 1)
					ctrl = 2;

				if (((dis_sta_map[j % 4] & 0x1 << i) >> i) == 1)
					ctrl = 1;

				DBGLOG(HAL, INFO, " ctrl = %s",
					sta_ctrl_reg[ctrl]);
			}
		}
	}

	for (i = 0; i < PLE_PEEK_CR_NUM; i++) {
		addr = PLE_PEEK_CR_0 + PLE_PEEK_CR_OFFSET * i;
		HAL_MCR_RD(prAdapter, addr, &value);
		DBGLOG(HAL, INFO, "PLE_PEEK_CR_%u[0x%08x/0x%08x]\n",
		       i, addr, value);
	}
}


struct BUS_INFO mt7663_bus_info = {
#if defined(_HIF_PCIE)
	.top_cfg_base = MT7663_TOP_CFG_BASE,
	.bus2chip = mt7663_bus2chip_cr_mapping,
	.tx_ring_fwdl_idx = 3,
	.tx_ring_cmd_idx = 15,
	.tx_ring_data_idx = 0,
	.fgCheckDriverOwnInt = FALSE,
	.fgInitPCIeInt = TRUE,
	.u4DmaMask = 36,

	.pdmaSetup = asicPdmaConfig,
#if CFG_SUPPORT_PCIE_L2
	.pdmaStop = asicPdmaStop,
#endif
	.enableInterrupt = asicEnableInterrupt,
	.disableInterrupt = asicDisableInterrupt,
	.lowPowerOwnRead = asicLowPowerOwnRead,
	.lowPowerOwnSet = asicLowPowerOwnSet,
	.lowPowerOwnClear = asicLowPowerOwnClearPCIe,
	.wakeUpWiFi = asicWakeUpWiFi,
	.isValidRegAccess = NULL,
	.getMailboxStatus = asicGetMailboxStatus,
	.setDummyReg = asicSetDummyReg,
	.checkDummyReg = asicCheckDummyReg,
#endif /* _HIF_PCIE */
#if defined(_HIF_USB)
	.u4UdmaWlCfg_0_Addr = CONNAC_UDMA_WLCFG_0,
	.u4UdmaWlCfg_1_Addr = CONNAC_UDMA_WLCFG_1,
	.u4UdmaWlCfg_0 =
		(UDMA_WLCFG_0_TX_EN(1) |
		UDMA_WLCFG_0_RX_EN(1) |
		UDMA_WLCFG_0_RX_MPSZ_PAD0(1) |
		UDMA_WLCFG_0_1US_TIMER_EN(1)),
	.u4UdmaTxTimeout = UDMA_TX_TIMEOUT_LIMIT,
	.asicUsbSuspend = asicUsbSuspend,
	.asicUsbEventEpDetected = asicUsbEventEpDetected,
#endif /* _HIF_USB */
#if defined(_HIF_SDIO)
	.halTxGetFreeResource = halTxGetFreeResource_v1,
	.halTxReturnFreeResource = halTxReturnFreeResource_v1,
	.halRestoreTxResource = halRestoreTxResource_v1,
	.halUpdateTxDonePendingCount = halUpdateTxDonePendingCount_v1,
#endif /* _HIF_SDIO */
};

struct FWDL_OPS_T mt7663_fw_dl_ops = {
	.constructFirmwarePrio = mt7663ConstructFirmwarePrio,
	.downloadPatch = wlanDownloadPatch,
	.downloadFirmware = wlanConnacFormatDownload,
	.getFwInfo = wlanGetConnacFwInfo,
	.getFwDlInfo = asicGetFwDlInfo,
};

struct TX_DESC_OPS_T mt7663TxDescOps = {
	.fillNicAppend = fillNicTxDescAppend,
	.fillHifAppend = fillTxDescAppendByHostV2,
	.fillTxByteCount = fillTxDescTxByteCount,
};

#if CFG_SUPPORT_QA_TOOL
struct ATE_OPS_T mt7663AteOps = {
	.setICapStart = connacSetICapStart,
	.getICapStatus = connacGetICapStatus,
	.getICapIQData = connacGetICapIQData,
	.getRbistDataDumpEvent = nicExtEventICapIQData,
};
#endif

struct CHIP_DBG_OPS mt7663_debug_ops = {
#if defined(_HIF_PCIE) || defined(_HIF_AXI)
	.showPdmaInfo = halShowPdmaInfo,
	.showCsrInfo = halShowHostCsrInfo,
	.showDmaschInfo = halShowDmaschInfo,
#else
	.showPdmaInfo = NULL,
	.showPseInfo = halShowPseInfo,
	.showPleInfo = halShowPleInfo,
	.showCsrInfo = NULL,
	.showDmaschInfo = NULL,
#endif
};

/* Litien code refine to support multi chip */
struct mt66xx_chip_info mt66xx_chip_info_mt7663 = {
	.bus_info = &mt7663_bus_info,
	.fw_dl_ops = &mt7663_fw_dl_ops,
	.prTxDescOps = &mt7663TxDescOps,
#if CFG_SUPPORT_QA_TOOL
	.prAteOps = &mt7663AteOps,
#endif
	.prDebugOps = &mt7663_debug_ops,

	.chip_id = MT7663_CHIP_ID,
	.should_verify_chip_id = FALSE,
	.sw_sync0 = MT7663_SW_SYNC0,
	.sw_ready_bits = WIFI_FUNC_NO_CR4_READY_BITS,
	.sw_ready_bit_offset = MT7663_SW_SYNC0_RDY_OFFSET,
	.patch_addr = MT7663_PATCH_START_ADDR,
	.is_support_cr4 = FALSE,
	.txd_append_size = MT7663_TX_DESC_APPEND_LENGTH,
	.rxd_size = MT7663_RX_DESC_LENGTH,
	.init_event_size = MT7663_RX_INIT_EVENT_LENGTH,
	.event_hdr_size = MT7663_RX_EVENT_HDR_LENGTH,
	.eco_info = mt7663_eco_table,
	.isNicCapV1 = FALSE,
	.is_support_efuse = TRUE,

	.asicCapInit = asicCapInit,
	.asicEnableFWDownload = asicEnableFWDownload,
	.asicGetChipID = NULL,
	.downloadBufferBin = wlanConnacDownloadBufferBin,
	.showTaskStack = NULL,
	.is_support_hw_amsdu = TRUE,
	.ucMaxSwAmsduNum = 0,
	.workAround = 0,
	.prTxPwrLimitFile = "TxPwrLimit_MT76x3.dat",
	.ucTxPwrLimitBatchSize = 16,
	.asicDumpSerDummyCR = mt7663DumpSerDummyCR,
	.getDongleType = mt7663GetDongleType,
};

struct mt66xx_hif_driver_data mt66xx_driver_data_mt7663 = {
	.chip_info = &mt66xx_chip_info_mt7663,
};

#endif /* MT7663 */
