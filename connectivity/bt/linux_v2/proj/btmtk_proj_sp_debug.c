/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include <btmtk_main.h>
#include "connv3.h"
#include "btmtk_uart_tty.h"
#include "mt6653_bt_dbg_sop.h"

#define BT_CR_DUMP_BUF_SIZE		(1024)
#define DBG_TAG	"[btmtk_dbg_sop]"
#define RHW_DBG_TAG	"[btmtk_dbg_sop_rhw]"
#define HIF_DBG_TAG	"[btmtk_dbg_sop_hif]"
/* unified debug sop format */
#define UNIFIED_DBG_TAG	"[PSOP_5_1]"
#define REG_DUMP_NUM_MAX		16
#define REG_DUMP_ARRAY_SIZE		256

#define CHECK_ERR_MSG(val) (((val) >> 16) == 0xdead)

#define FW_CR_WR_CMD_PKT_LEN	24
#define FW_CR_RD_CMD_PKT_LEN	16
#define FW_CR_WR_EVT_PKT_LEN	11
#define FW_CR_RD_EVT_PKT_LEN	19
#define FW_CR_ADDR_LEN	4
#define FW_CR_DATA_LEN	4
#define FW_CR_EVT_HEADER	5
#define FW_CR_CMD_ADDR_OFFSET	(FW_CR_RD_CMD_PKT_LEN - FW_CR_ADDR_LEN)
#define FW_CR_CMD_DATA_OFFSET	(FW_CR_CMD_ADDR_OFFSET + FW_CR_DATA_LEN)
#define FW_CR_EVT_ADDR_OFFSET	(FW_CR_RD_EVT_PKT_LEN - FW_CR_ADDR_LEN - FW_CR_DATA_LEN)
#define FW_CR_EVT_DATA_OFFSET	(FW_CR_EVT_ADDR_OFFSET + FW_CR_ADDR_LEN)
#define REMAP_DLM(addr)	(((addr) - 0x02200000) + 0x188C0000)

#define ALIGN4(addr)	((addr) & ~(0x3))

struct bt_dump_cr_buffer {
	uint8_t buffer[BT_CR_DUMP_BUF_SIZE];
	uint32_t cr_count;
	uint32_t count;
	uint8_t *pos;
	uint8_t *end;
};

struct bt_dump_cr_buffer g_btmtk_cr_dump;
extern struct btmtk_dev *g_sbdev;

static inline uint32_t btmtk_conninfra_view_remap(uint32_t pos)
{
	if ((pos & 0xff000000) == 0x18000000)
		return (pos & 0x00ffffff) | 0x7c000000;
	//else if (pos >= 0x1880000 && pos <= 0x18bfffff)
		//return (pos & 0x000fffff) | 0x18800000;
	return pos;
}

static inline void BT_DUMP_CR_BUFFER_RESET(void)
{
	memset(g_btmtk_cr_dump.buffer, 0, BT_CR_DUMP_BUF_SIZE);
	g_btmtk_cr_dump.pos = &g_btmtk_cr_dump.buffer[0];
	g_btmtk_cr_dump.end = g_btmtk_cr_dump.pos + BT_CR_DUMP_BUF_SIZE - 1;
}

static inline void BT_DUMP_CR_INIT(uint32_t cr_count)
{
	BT_DUMP_CR_BUFFER_RESET();
	g_btmtk_cr_dump.count = 0;
	g_btmtk_cr_dump.cr_count = cr_count;
}

static inline int BT_DUMP_CR_PRINT(uint32_t value)
{
	uint32_t ret = 0;

	ret = snprintf(g_btmtk_cr_dump.pos,
				  (g_btmtk_cr_dump.end - g_btmtk_cr_dump.pos + 1),
				  "%08x ", value);
	if (ret >= (g_btmtk_cr_dump.end - g_btmtk_cr_dump.pos + 1)) {
		BTMTK_ERR("%s %s: error in sprintf while dumping cr", DBG_TAG, __func__);
		if (g_btmtk_cr_dump.count)
			BTMTK_WARN("%s %s",DBG_TAG, g_btmtk_cr_dump.buffer);
		return -1;
	}

	g_btmtk_cr_dump.pos += ret;
	g_btmtk_cr_dump.count++;

	if ((g_btmtk_cr_dump.count & 0xF) == 0 ||
		 g_btmtk_cr_dump.count == g_btmtk_cr_dump.cr_count) {
		BTMTK_WARN("%s %s",DBG_TAG, g_btmtk_cr_dump.buffer);
		BT_DUMP_CR_BUFFER_RESET();
	}

	return 0;
}

int RHW_WRITE(uint32_t addr, uint32_t val)
{
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;

	/* ex: write dummy CR 0x022121cc = 0x44332222 */
	u8 cmd[RHW_PKT_LEN] = {0x40, 0x00, 0x00, 0x08, 0x00,
				0xCC, 0x21, 0x21, 0x02,
				0x22, 0x22, 0x33, 0x44};
	u8 evt[RHW_PKT_LEN] = {0x40, 0x00, 0x00, 0x08, 0x00,
				0xCC, 0x21, 0x21, 0x02,
				0x22, 0x22, 0x33, 0x44};

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (cif_dev->rhw_fail_cnt > BT_RHW_MAX_ERR_COUNT) {
		BTMTK_WARN_LIMITTED("%s skip, rhw_fail_cnt[%d]", __func__, cif_dev->rhw_fail_cnt);
		return ret;
	}

	BTMTK_DBG("%s: write addr[%x], value[0x%08x]", __func__, addr, val);
	memcpy(&cmd[RHW_ADDR_OFFSET_CMD], &addr, RHW_ADDR_LEN);
	memcpy(&cmd[RHW_VAL_OFFSET_CMD], &val, RHW_VAL_LEN);

	memcpy(&evt[RHW_ADDR_OFFSET_CMD], &addr, RHW_ADDR_LEN);

	ret = btmtk_main_send_cmd(g_sbdev, cmd, RHW_PKT_LEN, evt, RHW_PKT_COMP_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s failed, rhw_fail_cnt[%d]", __func__, ++cif_dev->rhw_fail_cnt);

	return ret ;

}

int RHW_READ(uint32_t addr, uint32_t *val)
{
	int ret = -1;
	struct btmtk_uart_dev *cif_dev = NULL;

	/* ex: read dummy CR 0x022121cc */
	u8 cmd[RHW_PKT_LEN] = {0x41, 0x00, 0x00, 0x08, 0x00,
				0xCC, 0x21, 0x21, 0x02,
				0x00, 0x00, 0x00, 0x00};

	u8 evt[RHW_PKT_LEN] = {0x41, 0x00, 0x00, 0x08, 0x00,
				0xCC, 0x21, 0x21, 0x02,
				0x00, 0x00, 0x00, 0x00};

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return -1;
	}
	cif_dev = (struct btmtk_uart_dev *)g_sbdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return -1;
	}

	if (cif_dev->rhw_fail_cnt > BT_RHW_MAX_ERR_COUNT) {
		*val = 0xdeaddead;
		BTMTK_WARN_LIMITTED("%s skip, rhw_fail_cnt[%d]", __func__, cif_dev->rhw_fail_cnt);
		return ret;
	}
	memcpy(&cmd[RHW_ADDR_OFFSET_CMD], &addr, sizeof(addr));
	memcpy(&evt[RHW_ADDR_OFFSET_CMD], &addr, sizeof(addr));

	ret = btmtk_main_send_cmd(g_sbdev, cmd, RHW_PKT_LEN, evt, RHW_PKT_COMP_LEN, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT, CMD_NO_NEED_FILTER);

	if (ret >= 0) {
		memcpy(val, g_sbdev->io_buf + RHW_PKT_COMP_LEN, sizeof(u32));
		*val = le32_to_cpu(*val);
	} else {
		*val = 0xdeaddead;
		BTMTK_ERR("%s failed, rhw_fail_cnt[%d]", __func__, ++cif_dev->rhw_fail_cnt);
	}
	BTMTK_DBG("%s: addr[%x], val[0x%08x]", __func__, addr, *val);

	return ret;

}

int WMT_WRITE(uint32_t addr, uint32_t val)
{
	int ret = -1;

	/* ex: write dummy CR 0x70025014 = 0x11223344 */
	u8 cmd[FW_CR_WR_CMD_PKT_LEN] = {0x01, 0x6F, 0xFC, 0x14,		// hci cmd header
									0x01, 0x08, 0x10, 0x00,		// fw cmd header
									0x01, 0x01, 0x00, 0x01,		// 0x01 write, 0x02 read
									0x14, 0x50, 0x02, 0x70,		// address
									0x44, 0x33, 0x22, 0x11,		// data
									0xFF, 0xFF, 0xFF, 0xFF};	// mask

	u8 evt[FW_CR_WR_EVT_PKT_LEN] = {0x04, 0xE4, 0x08, 0x02, 0x08,
									0x04, 0x00, 0x00, 0x00, 0x00, 0x01};

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_WORKING
			|| btmtk_fops_get_state(g_sbdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: not in working state(%d) fops(%d)"
			, __func__, btmtk_get_chip_state(g_sbdev), btmtk_fops_get_state(g_sbdev));
		return -1;
	}

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return -1;
	}

	BTMTK_DBG("%s: write addr[%x], value[0x%08x]", __func__, addr, val);
	memcpy(&cmd[FW_CR_CMD_ADDR_OFFSET], &addr, FW_CR_ADDR_LEN);
	memcpy(&cmd[FW_CR_CMD_DATA_OFFSET], &val, FW_CR_DATA_LEN);

	ret = btmtk_main_send_cmd(g_sbdev, cmd, FW_CR_WR_CMD_PKT_LEN, evt, FW_CR_EVT_HEADER,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);
	if (ret < 0)
		BTMTK_ERR("%s failed", __func__);

	return ret ;
}

int WMT_READ(uint32_t addr, uint32_t *val)
{
	int ret = -1;

	/* ex: read dummy CR 0x70025014 = 0x11223344 */
	u8 cmd[FW_CR_RD_CMD_PKT_LEN] = {0x01, 0x6F, 0xFC, 0x0C,				// hci cmd header
									0x01, 0x08, 0x08, 0x00,				// fw cmd header
									0x02, 0x01, 0x00, 0x01,				// 0x01 write, 0x02 read
									0x14, 0x50, 0x02, 0x70};			// address

	u8 evt[FW_CR_RD_EVT_PKT_LEN] = {0x04, 0xE4, 0x10, 0x02, 0x08,
									0x0C, 0x00, 0x00, 0x00, 0x00, 0x01,
									0x14, 0x50, 0x02, 0x70, 			// address
									0x44, 0x33, 0x22, 0x11}; 			// data

	if (g_sbdev == NULL) {
		BTMTK_ERR("%s: g_sbdev is NULL", __func__);
		return -1;
	}

	if (btmtk_get_chip_state(g_sbdev) != BTMTK_STATE_WORKING
			|| btmtk_fops_get_state(g_sbdev) != BTMTK_FOPS_STATE_OPENED) {
		BTMTK_WARN("%s: not in working state(%d) fops(%d)"
			, __func__, btmtk_get_chip_state(g_sbdev), btmtk_fops_get_state(g_sbdev));
		return -1;
	}

	if (g_sbdev->dynamic_fwdl_start) {
		BTMTK_INFO("%s: fw doing re-download, skip", __func__);
		return -1;
	}

	memcpy(&cmd[FW_CR_CMD_ADDR_OFFSET], &addr, sizeof(addr));
	memcpy(&evt[FW_CR_EVT_ADDR_OFFSET], &addr, sizeof(addr));

	ret = btmtk_main_send_cmd(g_sbdev, cmd, FW_CR_RD_CMD_PKT_LEN,
			evt, FW_CR_EVT_HEADER, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV, CMD_NO_NEED_FILTER);

	if (ret >= 0) {
		memcpy(val, g_sbdev->io_buf + FW_CR_EVT_DATA_OFFSET, sizeof(u32));
		*val = le32_to_cpu(*val);
	} else {
		*val = 0xdeaddead;
		BTMTK_ERR("%s failed", __func__);
	}
	BTMTK_DBG("%s: addr[%x], val[0x%08x]", __func__, addr, *val);

	return ret;
}

static void rhw_dump_utility(const struct bt_dump_list *dump_list)
{
	uint32_t readCmdSize = dump_list->read_cmd_size;
	const struct bt_dbg_command *pCmdList = NULL;
	char dumpLineBuf[REG_DUMP_ARRAY_SIZE] = {0};
	uint32_t line = 0, readCount = 0, readVal = 0;
	uint32_t offset = 0, totalLen = REG_DUMP_ARRAY_SIZE;
	uint32_t i;

	/* Print register value */
	pCmdList = dump_list->cmd_list;
	for (i = 0; i < dump_list->dump_size; i++) {
		if (pCmdList[i].write) {
			if (pCmdList[i].mask == 0)
				RHW_WRITE(pCmdList[i].w_addr, (pCmdList[i].value));
			else
				RHW_WRITE(pCmdList[i].w_addr, (pCmdList[i].value & pCmdList[i].mask));
		}

		if (pCmdList[i].read) {
			if (readCount % REG_DUMP_NUM_MAX == 0) {
				offset += snprintf(dumpLineBuf + offset, totalLen - offset, "[%s][%d]", dump_list->tag, line);
				line++;
			}

			RHW_READ(pCmdList[i].r_addr, &readVal);
			offset += snprintf(dumpLineBuf + offset, totalLen - offset, " %08X", readVal);
			readCount++;

			if ((readCount % REG_DUMP_NUM_MAX == 0) || (readCount >= readCmdSize)) {
				BTMTK_INFO("%s\n", dumpLineBuf);
				memset(dumpLineBuf, 0, REG_DUMP_ARRAY_SIZE);
				offset = 0;
			}
		}
	}
}

static void bt_hw_dbg_unify_rhw_dump_utility(const struct bt_dump_list *dump_list)
{
	/* Print header */
	BTMTK_INFO("[%s][H] [%s][Count: %d]\n",
		dump_list->tag, dump_list->description, dump_list->read_cmd_size);

	/* Print list*/
	rhw_dump_utility(dump_list);
}

void mt6653_rhw_dumpReg(void)
{
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_top_hostcsr_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_top_hostcsr_b);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_top_hostcsr_c);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_top_apb_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_b);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_c);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_b);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_c);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_d);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_e);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_f);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_g);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_h);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_i);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_j);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_on_hostcsr_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_vlp_hostcsr_a);
	bt_hw_dbg_unify_rhw_dump_utility(&mt6653_dump_list_bg_mcu_common_off_hostcsr_a);
}

static inline void btmtk_dump_bg_mcu_core(void)
{
	uint32_t i = 0, org_value, value, cr_count = 0x26 + 4;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCU core registers] - mcu_flg1, mcu_flg2 count[%d]", RHW_DBG_TAG, cr_count);
	RHW_WRITE(0x81025020, 0x00000201);
	RHW_READ(0x80000A00, &org_value);

	/* mcu flag1 */
	for (i = 0; i < 0x26; i++) {
		value = (org_value & 0xC0FFFFFF) | (i << 24);

		RHW_WRITE(0x80000A00, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	for (i = 0; i < 4; i++) {
		value = 0x0403 | (i << 16);

		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_dsp_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 0x17 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DSP debug flags] - mcu_flg3, mcu_flg4 count[%d]", RHW_DBG_TAG, cr_count);

	/* mcu_flag3 */
	RHW_WRITE(0x81025020, 0x00000605);
	RHW_READ(0x8000040C, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag4 */
	for (i = 0; i <= 0x16; i++) {
		value = 0x0807 | (i << 16);

		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_mcusys_clk_gals_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 4 + 8;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCUSYS CLK and GALS debug flags] - mcu_flg5, mcu_flg6 count[%d]", RHW_DBG_TAG, cr_count);

	/* mcu_flag5 */
	for (i = 0; i < 4; i++) {
		value = 0x0A09 | (i << 16);

		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag6 */
	RHW_WRITE(0x81025020, 0x00000C0B);
	for (i = 0; i < 8; i++) {
		value =  i << 16;

		RHW_WRITE(0x80000408, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_mcu_pc_lr(void)
{
	uint32_t i = 0, value, cr_count = 0x55;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCU PC/LR log] - mcu_flg7[84:168] cpu_dbg_pc_log0 ~ conn_debug_port84 count[%d]"
				, RHW_DBG_TAG, cr_count);

	/* mcu_flag7 */
	RHW_WRITE(0x81025020, 0x00000E0D);
	for (i = 0; i <= 0x54; i++) {
		RHW_WRITE(0x80000400, i);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_dsp_pc_lr(void)
{
	uint32_t i = 0, value, cr_count = 0x44;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DSP PC/LR log] - mcu_flg7[169:236] cpu1_dbg_pc_log0 ~ cpu1_lr count[%d]"
				, RHW_DBG_TAG, cr_count);

	/* mcu_flag7 */
	RHW_WRITE(0x81025020, 0x00000E0D);
	for (i = 0x55; i <= 0x98; i++) {
		RHW_WRITE(0x80000400, i);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_peri_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 4 + 6;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG PERI debug flags] - mcu_flg11, mcu_flg12 count[%d]", RHW_DBG_TAG, cr_count);

	/* mcu_flag11 */
	for (i = 0; i < 4; i++) {
		value = 0x00001615 | (i << 16);
		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag12 */
	for (i = 0; i < 6; i++) {
		value = 0x00001817 | (i << 16);
		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_bus_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 10 + 15 + 17;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG BUS debug flags] - mcu_flg13, mcu_flg14, mcu_flg16 count[%d]", RHW_DBG_TAG, cr_count);

	/* mcu_flag13 */
	RHW_WRITE(0x81025020, 0x00001A19);

	for (i = 0; i <= 9; i++) {
		value = (i << 12);
		RHW_WRITE(0x80000408, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg14 */
	for (i = 0; i < 0xF; i++) {
		value = 0x00001C1B | (i << 16);
		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg16 */
	for (i = 0; i <= 0x10; i++) {
		value = 0x0000201F | (i << 16);
		RHW_WRITE(0x81025020, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_dump_dma_uart_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 8 + 1 + 14 + 17;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DMA and UART debug flags] - mcu_flg19, mcu_flg20, mcu_flg23, mcu_flg24 count[%d]"
				, RHW_DBG_TAG, cr_count);

	/* mcu_flag19 */
	RHW_WRITE(0x81025020, 0x00002625);
	for (i = 0; i <= 7; i++) {
		value = (i << 8);
		RHW_WRITE(0x80000408, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg20 */
	RHW_WRITE(0x81025020, 0x00002827);
	RHW_READ(0x8000040C, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flag23 */
	RHW_WRITE(0x81025020, 0x00002E2D);
	for (i = 0; i <= 0x0D; i++) {
		value = (i << 8);
		RHW_WRITE(0x80000404, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag24 */
	RHW_WRITE(0x81025020, 0x0000302F);
	for (i = 0; i <= 0x0F; i++) {
		value = (i << 16);
		RHW_WRITE(0x80000404, value);
		RHW_READ(0x8000040C, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	RHW_WRITE(0x80000404, 0x001E0000);
	RHW_READ(0x8000040C, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_dump_cryto_debug_flags(void)
{
	uint32_t value, cr_count = 1 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG CRYPTO debug flags] - mcu_flg21, mcu_flg22 (optional) count[%d]", RHW_DBG_TAG, cr_count);

	/* mcu_flag21 */
	RHW_WRITE(0x81025020, 0x00002827);
	RHW_READ(0x8000040C, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flg22 */
	RHW_WRITE(0x81025020, 0x00002A29);
	RHW_READ(0x8000040C, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_dump_dma_uart_cfg2(void)
{
	uint32_t value, pos, forward_dump_count = 8, cr_count = 8 + 8 + 1;
	BT_DUMP_CR_INIT(cr_count);

	/* dma_cfg1 */
	RHW_READ(0x80010000, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x80010414, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x80010418, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x8001041C, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x80010424, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x8001072C, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x80010738, &value);
	BT_DUMP_CR_PRINT(value);
	RHW_READ(0x80010744, &value);
	BT_DUMP_CR_PRINT(value);

	/* dma_cfg2 */
	RHW_READ(0x8001042C, &pos);
	BT_DUMP_CR_PRINT(pos);
	BTMTK_INFO("%s [DMA UART CFG] - dma_cfg1, dma_cfg2, uart_cfg1[0x8001042C=0x%08x] count[%d]"
			, RHW_DBG_TAG, pos, cr_count);

	/* 4 bytes alignment */
	pos &= ~(0x3);

	/* forward dump 32 bytes */
	for (; forward_dump_count > 0; pos-=4, forward_dump_count--) {
		BTMTK_DBG("%s pos[0x%08x]", __func__, pos);
		RHW_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}

	/* uart_cfg1 */
	RHW_READ(0x8009006C, &value);
	BT_DUMP_CR_PRINT(value);

}


static inline void btmtk_dump_dma_uart_cfg3(void)
{
	uint32_t value, pos, cr_count = 8;
	uint32_t base, vff_size;
	BT_DUMP_CR_INIT(cr_count);

	/* dma_cfg3 */
	RHW_READ(0x80010734, &pos);

	RHW_READ(0x8001072C, &base);

	RHW_READ(0x80010744, &vff_size);

	BTMTK_INFO("%s [dma_cfg3] - 0x80010734[0x%08x], 0x8001072C[0x%08x], 0x80010744[0x%08x] count[%d]"
			, RHW_DBG_TAG, pos, base, vff_size, cr_count);

	/* 4 bytes alignment */
	pos &= ~(0x3);

	for (; cr_count > 0; pos-=4, cr_count--) {
		/* wrap around */
	    if (pos < base)
			pos = base + vff_size - 4;
		BTMTK_DBG("%s pos[0x%08x]", __func__, pos);
		RHW_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}

}

static inline void btmtk_dump_dma_uart_cfg4(void)
{
	uint32_t value, pos, cr_count = 8;
	uint32_t base, vff_size;
	BT_DUMP_CR_INIT(cr_count);

	/* dma_cfg4 */
	RHW_READ(0x80010730, &pos);

	RHW_READ(0x8001072C, &base);

	RHW_READ(0x80010744, &vff_size);

	BTMTK_INFO("%s [dma_cfg4] - 0x80010730[0x%08x], 0x8001072C[0x%08x], 0x80010744[0x%08x] count[%d]"
			, RHW_DBG_TAG, pos, base, vff_size, cr_count);

	/* 4 bytes alignment */
	pos &= ~(0x3);

	for (; cr_count > 0; pos-=4, cr_count--) {
	    if (pos < base)
			pos = base + vff_size - 4;
		BTMTK_DBG("%s pos[0x%08x]", __func__, pos);
		RHW_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}
}

static inline void btmtk_dump_bt_mcysys_vlp(void)
{
	uint32_t value, cr_count = 10, i = 0;
	BT_DUMP_CR_INIT(cr_count);

	BTMTK_INFO("%s [BT MCUSYS VLP] - mcu_vlp_flg0 count[%d]" , RHW_DBG_TAG, cr_count);

	RHW_WRITE(0x81030408, 0x10);
	for (i = 0; i < 3; i++) {
		RHW_WRITE(0x81030408, 0x11 + i);
		RHW_READ(0x81030414, &value);
		BT_DUMP_CR_PRINT(value);
	}

	for (i = 0; i <= 3; i++) {
		RHW_WRITE(0x81030408, 0x20 + (i << 4));
		RHW_READ(0x81030414, &value);
		BT_DUMP_CR_PRINT(value);
	}

	RHW_READ(0x81030434, &value);
	BT_DUMP_CR_PRINT(value);

	RHW_READ(0x81030438, &value);
	BT_DUMP_CR_PRINT(value);

	RHW_READ(0x8103040C, &value);
	BT_DUMP_CR_PRINT(value);

	RHW_WRITE(0x81030408, 0x60);
	RHW_READ(0x81030414, &value);
	BT_DUMP_CR_PRINT(value);
}

/*	connv3_conninfra_bus_dump(enum connv3_drv_type drv_type, struct connv3_cr_cb *cb, void * priv_data)
 *
 *	drv_type: driver type
 *	cb: callback function provided by subsys to read/write CR
 *	addr is fw view
 */

static int btmtk_connv3_cr_read_cb(void* priv_data, unsigned int addr, unsigned int *value)
{
#if CFG_SUPPORT_RHW_DUMP
	return RHW_READ(addr, value);
#else
	return WMT_READ(addr, value);
#endif
}

static int btmtk_connv3_cr_write_cb(void* priv_data, unsigned int addr, unsigned int value)
{
#if CFG_SUPPORT_RHW_DUMP
	return RHW_WRITE(addr, value);
#else
	return WMT_WRITE(addr, value);
#endif
}

static int btmtk_connv3_cr_write_mask_cb(void* priv_data, unsigned int addr, unsigned int mask, unsigned int value)
{
	int ret = 0;
	uint32_t org_value;
#if CFG_SUPPORT_RHW_DUMP
	ret = RHW_READ(addr, &org_value);
#else
	ret = WMT_READ(addr, &org_value);
#endif
	if (ret < 0) {
		BTMTK_ERR("%s: read [%x] err", __func__, addr);
		return ret;
	}

	org_value = (org_value & ~mask ) | value;
#if CFG_SUPPORT_RHW_DUMP
	return RHW_WRITE(addr, org_value);
#else
	return WMT_WRITE(addr, org_value);
#endif
}

struct connv3_cr_cb btmtk_connv3_cr_cb = {
	//.priv_data,
	.read = btmtk_connv3_cr_read_cb,
	.write = btmtk_connv3_cr_write_cb,
	.write_mask = btmtk_connv3_cr_write_mask_cb
};

void btmtk_uart_sp_dump_debug_sop(struct btmtk_dev *bdev)
{
#if CFG_SUPPORT_RHW_DBG_SOP
	struct btmtk_uart_dev *cif_dev = NULL;
	int state = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}

	state = btmtk_get_chip_state(bdev);
	BTMTK_INFO("%s: start, bt assert_state[%d] rhw_en[%d] bt_state[%d]",
				__func__, atomic_read(&bdev->assert_state), cif_dev->rhw_en, state);

	if (cif_dev->rhw_en && state < BTMTK_STATE_PROBE && state > BTMTK_STATE_SEND_ASSERT) {
		BTMTK_ERR("%s: not trigger debug_sop", __func__);
		return;
	}

	if (bdev->chip_id == 0x6653) {
		BTMTK_INFO("%s version=%s", UNIFIED_DBG_TAG, MT6653_BT_DEBUGSOP_DUMP_VERSION);
		/* unified dump */
		mt6653_rhw_dumpReg();

	} else {
		btmtk_dump_bg_mcu_core();
		btmtk_dump_dsp_debug_flags();
		btmtk_dump_mcusys_clk_gals_debug_flags();
		btmtk_dump_mcu_pc_lr();
		btmtk_dump_dsp_pc_lr();
		btmtk_dump_peri_debug_flags();
		btmtk_dump_bus_debug_flags();
		btmtk_dump_dma_uart_debug_flags();
		btmtk_dump_cryto_debug_flags();
		btmtk_dump_dma_uart_cfg2();
		btmtk_dump_dma_uart_cfg3();
		btmtk_dump_dma_uart_cfg4();
		btmtk_dump_bt_mcysys_vlp();
		/* cannot call connv3_conninfra_bus_dump at here, connv3_cored may trigger assert */
	}

	BTMTK_INFO("%s: end", __func__);
#else
	/* use RHW_READ to do drv own for assert cmd */
	uint32_t value = 0;
	RHW_READ(0x80010000, &value);

	/* not support rhw dump, avoid conflict with fw TX*/
	BTMTK_INFO("%s: not support", __func__);
#endif
}


/*
 *******************************************************************************
 *						 bt HIF dump feature
 *******************************************************************************
 */

/* set bit from left to right range */
#define SET_BIT_RANGE(l, r) (((1 << (l + 1)) - 1) ^ ((1 << r) - 1))

int HIF_WRITE(uint32_t addr, uint32_t val) {
	BTMTK_DBG("%s: addr[0x%08x], val[0x%08x], map_addr[0x%08x]", __func__, addr, val,
			btmtk_conninfra_view_remap(ALIGN4(addr)));
	return connv3_hif_dbg_write(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI,
			btmtk_conninfra_view_remap(ALIGN4(addr)), val);
}

int HIF_WRITE_MASK(uint32_t addr, uint32_t end, uint32_t start,uint32_t val) {
	return connv3_hif_dbg_write_mask(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI,
			btmtk_conninfra_view_remap(ALIGN4(addr)),
			SET_BIT_RANGE(end, start), (val << start));
}

int HIF_READ(uint32_t addr, uint32_t *val) {
	BTMTK_DBG("%s: addr[0x%08x], map_addr[0x%08x]", __func__, addr,
			btmtk_conninfra_view_remap(ALIGN4(addr)));
	return connv3_hif_dbg_read(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI,
			btmtk_conninfra_view_remap(ALIGN4(addr)), val);
}
static inline int btmtk_connv3_readable_check(void){
	unsigned int value;
	if (connv3_conninfra_bus_dump(CONNV3_DRV_TYPE_WIFI)) {
		BTMTK_WARN("%s: connv3_conninfra_bus_dump", __func__);
		return -1;
	}

	HIF_READ(0x7C011454, &value);
	if ((value & SET_BIT_RANGE(23, 22)) != 0) {
		BTMTK_WARN("%s: check 0x7C011454[23:22] != 0 failed, value[0x%08x]", __func__, value);
		return -1;
	}

	HIF_READ(0x7C812000, &value);
	switch (g_sbdev->chip_id) {
	case 0x6635:
	case 0x6639:
		if (value != 0x03000000) {
			BTMTK_WARN("%s: check 0x18812000 != 0x03000000 failed, value[0x%08x]", __func__, value);
			return -1;
		}
		break;
	case 0x6653:
		if (value != 0x03040100) {
			BTMTK_WARN("%s: check 0x7C812000 != 0x03040100 failed, value[0x%08x]", __func__, value);
			return -1;
		}
		break;
	}

	return 0;
}

static void hif_dump_utility(const struct bt_dump_list *dump_list)
{
	uint32_t readCmdSize = dump_list->read_cmd_size;
	const struct bt_dbg_command *pCmdList = NULL;
	char dumpLineBuf[REG_DUMP_ARRAY_SIZE] = {0};
	uint32_t line = 0, readCount = 0, readVal;
	uint32_t offset = 0, totalLen = REG_DUMP_ARRAY_SIZE;
	uint32_t i;

	/* Print register value */
	pCmdList = dump_list->cmd_list;
	for (i = 0; i < dump_list->dump_size; i++) {
		if (pCmdList[i].write) {
			if (pCmdList[i].mask == 0) {
				HIF_WRITE(pCmdList[i].w_addr, (pCmdList[i].value));
			} else {
				HIF_READ(pCmdList[i].w_addr, &readVal);
				readVal &= ~(pCmdList[i].mask);
				readVal |= pCmdList[i].value;
				HIF_WRITE(pCmdList[i].w_addr, readVal);
			}
		}

		if (pCmdList[i].read) {
			if (readCount % REG_DUMP_NUM_MAX == 0) {
				offset += snprintf(dumpLineBuf + offset, totalLen - offset, "[%s][%d]", dump_list->tag, line);
				line++;
			}

			HIF_READ(pCmdList[i].r_addr, &readVal);
			offset += snprintf(dumpLineBuf + offset, totalLen - offset, " %08X", readVal);
			readCount++;

			if ((readCount % REG_DUMP_NUM_MAX == 0) || (readCount >= readCmdSize)) {
				BTMTK_INFO("%s\n", dumpLineBuf);
				memset(dumpLineBuf, 0, REG_DUMP_ARRAY_SIZE);
				offset = 0;
			}
		}
	}
}

static void bt_hw_dbg_unify_hif_dump_utility(const struct bt_dump_list *dump_list)
{
	/* Print header */
	BTMTK_INFO("[%s][H] [%s][Count: %d]\n",
		dump_list->tag, dump_list->description, dump_list->read_cmd_size);

	/* Print list*/
	hif_dump_utility(dump_list);
}

void mt6653_hif_dumpReg(void)
{
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_top_hostcsr_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_top_hostcsr_b);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_top_hostcsr_c);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_top_apb_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_b);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bt_mcu_bus_timeout_hostcsr_c);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_b);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_c);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_d);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_e);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_f);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_g);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_h);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_i);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_off_hostcsr_j);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_on_hostcsr_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_vlp_hostcsr_a);
        bt_hw_dbg_unify_hif_dump_utility(&mt6653_dump_list_bg_mcu_common_off_hostcsr_a);
}

static inline void btmtk_hif_dump_bg_mcu_core(void)
{
	uint32_t i = 0, value, cr_count = 0x26 + 4;

	if (btmtk_connv3_readable_check()) {
		cr_count = 4;
		BTMTK_INFO("%s [BG MCU core registers] - mcu_flg2 count[%d]", HIF_DBG_TAG, cr_count);
		BT_DUMP_CR_INIT(cr_count);
	} else {
		BTMTK_INFO("%s [BG MCU core registers] - mcu_flg1, mcu_flg2 count[%d]", HIF_DBG_TAG, cr_count);
		BT_DUMP_CR_INIT(cr_count);
		/* mcu_flag1 */
		for (i = 0; i < 0x26; i++) {
			HIF_WRITE(0x7C023A0C, 0xC0040100 + i);
			HIF_WRITE_MASK(0x18800A00, 29, 24, i);
			HIF_READ(0x7C023A10, &value);
			if (BT_DUMP_CR_PRINT(value))
				return;
		}
	}

	/* mcu_flag2 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0040300 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_bg_mcu_core_dx4(void)
{
	uint32_t i = 0, value, cr_count = 0x26 + 4;

	if (btmtk_connv3_readable_check()) {
		cr_count = 4;
		BTMTK_INFO("%s [BG MCU core registers] - mcu_flg2 count[%d]", HIF_DBG_TAG, cr_count);
		BT_DUMP_CR_INIT(cr_count);
	} else {
		BTMTK_INFO("%s [BG MCU core registers] - mcu_flg1, mcu_flg2 count[%d]", HIF_DBG_TAG, cr_count);
		BT_DUMP_CR_INIT(cr_count);

		/* 0x7C800A00[0] = 0x0 */
		HIF_READ(0x7C800A00, &value);
		value &= 0xFFFFFFFE;
		HIF_WRITE(0x7C800A00, value);

		/* mcu_flag1 */
		for (i = 0; i < 0x26; i++) {
			HIF_WRITE(0x20023A0C, 0xC0040200 + i);
			//HIF_WRITE_MASK(0x18800A00, 29, 24, i);
			HIF_READ(0x20023A10, &value);
			if (BT_DUMP_CR_PRINT(value))
				return;
		}
	}

	/* mcu_flag2 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x20023A0C, 0xC0040400 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_dsp_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 0x17 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DSP debug flags] - mcu_flg3, mcu_flg4 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag3 */
	HIF_WRITE(0x7C023A0C, 0xC0040500);
	HIF_READ(0x7C023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag4 */
	for (i = 0; i <= 0x16; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0040700 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_dsp_debug_flags_dx4(void)
{
	uint32_t i = 0, value, cr_count = 0x18 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DSP debug flags] - mcu_flg3, mcu_flg4 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag3 */
	HIF_WRITE(0x20023A0C, 0xC0040600);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag4 */
	for (i = 0; i <= 0x17; i++) {
		HIF_WRITE(0x20023A0C, 0xC0040800 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcusys_clk_gals_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 4 + 8;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCUSYS CLK and GALS debug flags] - mcu_flg5, mcu_flg6 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag5 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0040900 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag6 */
	for (i = 0; i < 8; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0040B00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcusys_clk_gals_debug_flags_dx4(void)
{
	uint32_t i = 0, value, cr_count = 4 + 16;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCUSYS CLK and GALS debug flags] - mcu_flg5, mcu_flg6 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag5 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x20023A0C, 0xC0040A00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag6 */
	for (i = 0; i < 16; i++) {
		HIF_WRITE(0x20023A0C, 0xC0040C00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcu_pc_lr(void)
{
	uint32_t i = 0, value, cr_count = 0x55;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCU PC/LR log] - mcu_flg7[84:168] cpu_dbg_pc_log0 ~ conn_debug_port84 count[%d]"
				, HIF_DBG_TAG, cr_count);

	/* mcu_flag7 */
	for (i = 0; i <= 0x54; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0040D00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcu_pc_lr_dx4(void)
{
	uint32_t i = 0, value, cr_count = 0x55;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG MCU PC/LR log] - mcu_flg7[84:168] cpu_dbg_pc_log0 ~ conn_debug_port84 count[%d]"
				, HIF_DBG_TAG, cr_count);

	/* mcu_flag7 */
	for (i = 0; i <= 0x54; i++) {
		HIF_WRITE(0x20023A0C, 0xC0040E00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcusys_common_off_dx4(void)
{
	uint32_t i = 0, value, cr_count = 35;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [bt mcusys common off] - mcu_flg0~16 count[%d]"
				, HIF_DBG_TAG, cr_count);

	/* mcu_flag0 */
	HIF_WRITE(0x20023A0C, 0xC0050000);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag1 */
	HIF_WRITE(0x20023A0C, 0xC0050100);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag2 */
	HIF_WRITE(0x20023A0C, 0xC0050241);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag3 ~ mcu_flag10 */
	for (i = 0; i <= 7; i++) {
		HIF_WRITE(0x20023A0C, 0xC0050300 + (i << 8));
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag11 */
	for (i = 0; i <= 9; i++) {
		HIF_WRITE(0x20023A0C, 0xC0050B00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag12 */
	for (i = 0; i <= 9; i++) {
		HIF_WRITE(0x20023A0C, 0xC0050C00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag13 */
	HIF_WRITE(0x20023A0C, 0xC0050D00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag14 */
	HIF_WRITE(0x20023A0C, 0xC0050E00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag15 */
	HIF_WRITE(0x20023A0C, 0xC0050F00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag16 */
	HIF_WRITE(0x20023A0C, 0xC0051000);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_peri_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 4 + 6;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG PERI debug flags] - mcu_flg11, mcu_flg12 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag11 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0041500 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag12 */
	for (i = 0; i < 6; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0041700 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_peri_debug_flags_dx4(void)
{
	uint32_t i = 0, value, cr_count = 4 + 6;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG PERI debug flags] - mcu_flg11, mcu_flg12 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag11 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x20023A0C, 0xC0041600 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag12 */
	for (i = 0; i < 6; i++) {
		HIF_WRITE(0x20023A0C, 0xC0041800 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_bus_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 10 + 15 + 17;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG BUS debug flags] - mcu_flg13, mcu_flg14, mcu_flg16 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag13 */
	for (i = 0; i <= 9; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0041900 + (i << 4));
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg14 */
	for (i = 0; i < 0xF; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0041B00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg16 */
	for (i = 0; i <= 0x10; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0041F00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_bus_debug_flags_dx4(void)
{
	uint32_t i = 0, value, cr_count = 5 + 18 + 1 + 17 + 1 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG BUS debug flags] - mcu_flg13.5, mcu_flg14.5, mcu_flg15.5, mcu_flg16.5, mcu_flg17.5, mcu_flg18.5 count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag13.5 */
	for (i = 0; i <= 4; i++) {
		HIF_WRITE(0x20023A0C, 0xC0041A00 + (i << 4));
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg14.5 */
	for (i = 0; i < 0x12; i++) {
		HIF_WRITE(0x20023A0C, 0xC0041C00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg15.5 */
	HIF_WRITE(0x20023A0C, 0xC0041E00);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flg16.5 */
	for (i = 0; i <= 0x10; i++) {
		HIF_WRITE(0x20023A0C, 0xC0042000 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg17.5 */
	HIF_WRITE(0x20023A0C, 0xC0042200);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flg18.5 */
	HIF_WRITE(0x20023A0C, 0xC0042400);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* bt mcusys on */
	/* mcu_flag13~15 count [21] */

	/* mcu_flg13 */
	for (i = 0; i <= 1; i++) {
		HIF_WRITE(0x20023A1C, 0x300D80 + (i << 4));
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg14 */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x20023A1C, 0x300E00 + (i << 4));
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg15 */
	for (i = 0; i < 3; i++) {
		HIF_WRITE(0x20023A1C, 0x300F00 + (i << 4));
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg15 */
	for (i = 0; i < 0xB; i++) {
		HIF_WRITE(0x20023A1C, 0x300F81 + i);
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg15 */
	HIF_WRITE(0x20023A1C, 0x300FC0);
	HIF_READ(0x20023A20, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_dma_uart_debug_flags(void)
{
	uint32_t i = 0, value, cr_count = 8 + 1 + 14 + 17;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DMA and UART debug flags] - mcu_flg19, mcu_flg20, mcu_flg23, mcu_flg24 count[%d]"
				, HIF_DBG_TAG, cr_count);

	/* mcu_flag19 */
	for (i = 0; i <= 7; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0042500 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg20 */
	HIF_WRITE(0x7C023A0C, 0xC0042700);
	HIF_READ(0x7C023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flag23 */
	for (i = 0; i <= 0x0F; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0042D00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag24 */
	for (i = 0; i <= 0x0F; i++) {
		HIF_WRITE(0x7C023A0C, 0xC0042F00 + i);
		HIF_READ(0x7C023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	HIF_WRITE(0x7C023A0C, 0xC0042F1E);
	HIF_READ(0x7C023A10, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_dma_uart_debug_flags_dx4(void)
{
	uint32_t i = 0, value, cr_count = 11 + 1 + 1 + 1 + 28 + 17;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG DMA and UART debug flags] - mcu_flg19, mcu_flg20, mcu_flg21, mcu_flg22, mcu_flg23, mcu_flg24 count[%d]"
				, HIF_DBG_TAG, cr_count);

	/* mcu_flag19 */
	for (i = 0; i <= 10; i++) {
		HIF_WRITE(0x20023A0C, 0xC0042600 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flg20 */
	HIF_WRITE(0x20023A0C, 0xC0042800);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flg21 */
	HIF_WRITE(0x20023A0C, 0xC0042A00);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flg22 */
	HIF_WRITE(0x20023A0C, 0xC0042C00);
	HIF_READ(0x20023A10, &value);
	if (BT_DUMP_CR_PRINT(value))
		return;

	/* mcu_flag23 */
	for (i = 0; i <= 0x1B; i++) {
		HIF_WRITE(0x20023A0C, 0xC0042E00 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_flag24 */
	for (i = 0; i <= 0xF; i++) {
		HIF_WRITE(0x20023A0C, 0xC0043000 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	HIF_WRITE(0x20023A0C, 0xC004301E);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_cryto_debug_flags(void)
{
	uint32_t value, cr_count = 1 + 1;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG CRYPTO debug flags] - mcu_flg21, mcu_flg22 (optional) count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag21 */
	HIF_WRITE(0x7C023A0C, 0xC0042900);
	HIF_READ(0x7C023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flg22 */
	HIF_WRITE(0x7C023A0C, 0xC0042B00);
	HIF_READ(0x7C023A10, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_cryto_debug_flags_dx4(void)
{
	uint32_t value, cr_count = 8;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG CRYPTO debug flags] - mcu_flg25 ~ mcu_flg32 (optional) count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_flag25 */
	HIF_WRITE(0x20023A0C, 0xC0043200);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag26 */
	HIF_WRITE(0x20023A0C, 0xC0043400);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag27 */
	HIF_WRITE(0x20023A0C, 0xC0043600);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag28 */
	HIF_WRITE(0x20023A0C, 0xC0043800);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag29 */
	HIF_WRITE(0x20023A0C, 0xC0043A00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag30 */
	HIF_WRITE(0x20023A0C, 0xC0043C00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag31 */
	HIF_WRITE(0x20023A0C, 0xC0043E00);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_flag32 */
	HIF_WRITE(0x20023A0C, 0xC0044000);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_dma1(void) {
	uint32_t value, cr_count = 6;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s readable check failed, skip", HIF_DBG_TAG);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s DMA1 count[%d]", HIF_DBG_TAG, cr_count);

	HIF_READ(0x18810000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18810414, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18810418, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x1881041C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18810424, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18810738, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_dma1_dx4(void) {
	uint32_t value, cr_count = 7;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s readable check failed, skip", HIF_DBG_TAG);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s DMA1 count[%d]", HIF_DBG_TAG, cr_count);

	HIF_READ(0x20810000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x20810410, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x20810414, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x20810418, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x2081041C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x20810424, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x20810738, &value);
	BT_DUMP_CR_PRINT(value);
}


static inline void btmtk_hif_dump_bg_sysram1(void) {
	uint32_t value, pos, cr_count = 8;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x7C81042C, &value);

	/* boundary check */
	if(value < 0x00400000 || value > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, value);
		return;
	}

	/* dynamic mapping to 0x1890_0000 */
	value = (value & ~(0x3)) - 32;
	HIF_WRITE(0x7C815014, value);

	BTMTK_INFO("%s bg_sysram1 for 0x7C81042C[0x%08x], read from 0x7C90_0000, count[%d]"
				, HIF_DBG_TAG, value, cr_count);

	pos = 0x7C900000;

	for (; cr_count > 0; cr_count--, pos+=4) {
		HIF_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}
}

static inline void btmtk_hif_dump_bg_sysram2(void) {
	uint32_t value, pos, cr_count = 8, base, vff_size;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x7C810730, &pos);

	/* boundary check */
	if(pos < 0x00400000 || pos > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, pos);
		return;
	}

	pos += 0x7C440000;

	HIF_READ(0x7C81072C, &base);
	if (CHECK_ERR_MSG(base)) {
		BTMTK_INFO("%s %s: get error code 0x7C81072C[0x%08x]", HIF_DBG_TAG, __func__, base);
		return;
	}

	base += 0x7C440000;

	HIF_READ(0x7C810744, &vff_size);
	if (CHECK_ERR_MSG(vff_size)) {
		BTMTK_INFO("%s %s: get error code 0x7C810744[0x%08x]", HIF_DBG_TAG, __func__, vff_size);
		return;
	}

	BTMTK_INFO("%s bg_sysram2 for 0x7C810730[0x%08x], base[0x%08x], vff_size[0x%08x], count[%d]"
				, HIF_DBG_TAG, pos, base, vff_size, cr_count);

	/* 4 bytes alignment */
	pos &= ~(0x3);

	/* forward dump 32 bytes */
	for (; cr_count > 0; pos-=4, cr_count--) {
		/* wrap around */
	    if (pos < base)
			pos = base + vff_size -4;
		BTMTK_DBG("%s pos[0x%08x]", __func__, pos);
		HIF_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}
}

static inline void btmtk_hif_dump_bg_sysram3(void) {
	uint32_t value, pos, cr_count = 8, base, vff_size;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x7C810734, &pos);

	/* boundary check */
	if(pos < 0x00400000 || pos > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, pos);
		return;
	}

	pos += 0x7C440000;

	HIF_READ(0x7C81072C, &base);
	if (CHECK_ERR_MSG(base)) {
		BTMTK_INFO("%s %s: get error code 0x1881072C[0x%08x]", HIF_DBG_TAG, __func__, base);
		return;
	}

	base += 0x7C440000;


	HIF_READ(0x7C810744, &vff_size);
	if (CHECK_ERR_MSG(vff_size)) {
		BTMTK_INFO("%s %s: get error code 0x18810744[0x%08x]", HIF_DBG_TAG, __func__, vff_size);
		return;
	}

	BTMTK_INFO("%s bg_sysram3 for 0x7C810734[0x%08x], base[0x%08x], vff_size[0x%08x], count[%d]"
				, HIF_DBG_TAG, pos, base, vff_size, cr_count);

	/* 4 bytes alignment */
	pos &= ~(0x3);

	/* forward dump 32 bytes */
	for (; cr_count > 0; pos-=4, cr_count--) {
		/* wrap around */
	    if (pos < base)
			pos = base + vff_size - 4;
		BTMTK_DBG("%s pos[0x%08x]", __func__, pos);
		HIF_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}
}

static inline void btmtk_hif_dump_hif_uart1(void) {
	uint32_t value, cr_count = 23;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s HIF_UART count[%d]", HIF_DBG_TAG, cr_count);

	HIF_READ(0x1881900C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819004, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819008, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819010, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819014, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819018, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x1881901C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819024, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819028, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x1881902C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819040, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819044, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819054, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819058, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x1881906C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190A0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190A4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190A8, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190AC, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190B0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190B4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188190B8, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_hif_uart1_dx4(void) {
	uint32_t value, cr_count = 42;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s HIF_UART count[%d]", HIF_DBG_TAG, cr_count);

	HIF_READ(0x7C81900C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819004, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819008, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819010, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819014, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819018, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81901C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819024, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819028, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81902C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819040, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819044, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819054, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819058, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819068, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81906C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819070, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819074, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819078, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81907C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819080, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819084, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819088, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81908C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819090, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190A0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190A4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190A8, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190AC, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190B0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190B4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190B8, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190BC, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190C0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190C4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190C8, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190CC, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190D0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190D4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190D8, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8190E4, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_hif_uart2(void) {
	uint32_t value, cr_count = 12;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s HIF_UART(0x7C81900C) count[%d]", HIF_DBG_TAG, cr_count);

	HIF_WRITE(0x7C81900C, 0x00000010);
	HIF_READ(0x7C819000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819004, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x7C81900C, 0x000000BF);
	HIF_READ(0x7C819008, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819010, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819014, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819018, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81901C, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x7C81900C, 0x00000003);
	HIF_READ(0x7C819008, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819010, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819014, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C819018, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81901C, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_cirq_eint(void) {
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [CIRQ, EINT] count[%d]", HIF_DBG_TAG, cr_count);

	/* CIRQ */
	HIF_READ(0x18828070, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828074, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188280E0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188280E4, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828400, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828404, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828900, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828904, &value);
	BT_DUMP_CR_PRINT(value);

	/* EINT */
	HIF_READ(0x18830604, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18830604, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_bg_cfg(void)
{
	uint32_t i = 0, value, cr_count = 6 + 6 + 3 + 3 + 13 + 6;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG CFG] count[%d]", HIF_DBG_TAG, cr_count);

	/* bg_cfg */
	for (i = 0; i < 6; i++) {
		HIF_READ(0x18812168 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* conn_mcu_cirq_on */
	for (i = 0; i < 4; i++) {
		HIF_READ(0x18828400 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	HIF_READ(0x18828070, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828074, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_eint_vlp */
	HIF_READ(0x18860600, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18860604, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188606C0, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_dbg_cirq_on */
	HIF_READ(0x18828900, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18828904, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x188004E4, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_confg */
	HIF_READ(0x18800410, &value);
	BT_DUMP_CR_PRINT(value);
	for (i = 0; i < 10; i++) {
		HIF_READ(0x18800418 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	HIF_READ(0x18802214, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18802218, &value);
	BT_DUMP_CR_PRINT(value);

	/* con_mcu_bus_cr */
	for (i = 0; i < 6; i++) {
		HIF_READ(0x18815010 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_bg_cfg_dx4(void)
{
	uint32_t i = 0, value, cr_count = 3 + 8 + 3 + 3 + 13 + 6;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BG CFG] count[%d]", HIF_DBG_TAG, cr_count);

	/* bg_cfg */
	for (i = 0; i < 3; i++) {
		HIF_READ(0x7C812168 + (i << 3), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* conn_mcu_cirq_on */
	for (i = 0; i < 4; i++) {
		HIF_READ(0x7C828400 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	HIF_READ(0x7C8280E0, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8280E4, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_READ(0x7C828070, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C828074, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_eint_vlp */
	HIF_READ(0x7C860600, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860604, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8606C0, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_dbg_cirq_on */
	HIF_READ(0x7C828900, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C828904, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C8004E4, &value);
	BT_DUMP_CR_PRINT(value);

	/* conn_mcu_confg */
	HIF_READ(0x7C800410, &value);
	BT_DUMP_CR_PRINT(value);
	for (i = 0; i < 10; i++) {
		HIF_READ(0x7C800418 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	HIF_READ(0x7C802214, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C802218, &value);
	BT_DUMP_CR_PRINT(value);

	/* con_mcu_bus_cr */
	for (i = 0; i < 6; i++) {
		HIF_READ(0x7C815010 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_bt_mcusys_vlp(void)
{
	uint32_t i = 0, value, cr_count = 14;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT MCUSYS VLP] count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_vlp_flg1 */
	for (i = 0; i < 3; i++) {
		HIF_WRITE(0x7C060C04, 0x30520 + i);
		HIF_READ(0x7C060C00, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	/* mcu_vlp_flg2 */
	HIF_WRITE(0x7C060C04, 0x30540);
	HIF_READ(0x7C060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg3 */
	HIF_WRITE(0x7C060C04, 0x30560);
	HIF_READ(0x7C060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg4 */
	HIF_WRITE(0x7C060C04, 0x30580);
	HIF_READ(0x7C060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg5 */
	for (i = 0; i < 7; i++) {
		HIF_WRITE(0x7C060C04, 0x305A0 + i);
		HIF_READ(0x7C060C00, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_vlp_flg6 */
	HIF_WRITE(0x7C060C04, 0x305C0);
	HIF_READ(0x7C060C00, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_bt_mcusys_vlp_dx4(void)
{
	uint32_t i = 0, value, cr_count = 16;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT MCUSYS VLP] count[%d]", HIF_DBG_TAG, cr_count);

	/* mcu_vlp_flg1 */
	for (i = 0; i < 3; i++) {
		HIF_WRITE(0x20060C04, 0x30510 + i);
		HIF_READ(0x20060C00, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	/* mcu_vlp_flg2 */
	HIF_WRITE(0x20060C04, 0x30520);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg3 */
	HIF_WRITE(0x20060C04, 0x30530);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg4 */
	HIF_WRITE(0x20060C04, 0x30540);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg5 */
	for (i = 0; i < 7; i++) {
		HIF_WRITE(0x20060C04, 0x30550 + i);
		HIF_READ(0x20060C00, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* mcu_vlp_flg6 */
	HIF_WRITE(0x20060C04, 0x30560);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg7 */
	HIF_WRITE(0x20060C04, 0x30570);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* mcu_vlp_flg8 */
	HIF_WRITE(0x20060C04, 0x30580);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_bt_hif_select(void)
{
	uint32_t value, cr_count = 3;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT HIF SELECT] count[%d]", HIF_DBG_TAG, cr_count);

	/* CBTOP_STRAP_03 */
	HIF_READ(0x7001000C, &value);
	BT_DUMP_CR_PRINT(value);

	/* COS_System_Config.host_bt_select */
	HIF_READ(0x188C8288, &value);
	BT_DUMP_CR_PRINT(value);

	/* DMA_MASTER */
	HIF_READ(0x18810714, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_bt_hif_select_dx4(void)
{
	uint32_t value, cr_count = 3;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT HIF SELECT] count[%d]", HIF_DBG_TAG, cr_count);

	/* CBTOP_STRAP_03 */
	HIF_READ(0x7001000C, &value);
	BT_DUMP_CR_PRINT(value);

	/* COS_System_Config.host_bt_select */
	HIF_READ(0x188C7D60, &value);
	BT_DUMP_CR_PRINT(value);

	/* CONN_MCU_DMA_DMA7_CON */
	HIF_READ(0x18810714, &value);
	BT_DUMP_CR_PRINT(value);
}

static inline void btmtk_hif_dump_bt_pos_check(void)
{
	uint32_t value, cr_count = 8;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT POS CHECK] count[%d]", HIF_DBG_TAG, cr_count);

	/* wifi radio off cnt */
	HIF_READ(0x70025410, &value);
	BT_DUMP_CR_PRINT(value);

	/* bt radio off cnt */
	HIF_READ(0x70025414, &value);
	BT_DUMP_CR_PRINT(value);

	/* BT 1st power on */
	HIF_READ(0x18860000, &value);
	BT_DUMP_CR_PRINT(value);

	/* UDS cnt */
	HIF_READ(0x70003014, &value);
	BT_DUMP_CR_PRINT(value);

	/* CBTOP_STRAP_03*/
	HIF_READ(0x7001000C, &value);
	BT_DUMP_CR_PRINT(value);

	/* CBTOP_STRAP_01 */
	HIF_READ(0x70010004, &value);
	BT_DUMP_CR_PRINT(value);

	/* GPIO_DIN0 */
	HIF_READ(0x70005200, &value);
	BT_DUMP_CR_PRINT(value);

	/* VLP_CR */
	HIF_READ(0x70007204, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_bt_pos_check_dx4(void)
{
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [BT POS CHECK] count[%d]", HIF_DBG_TAG, cr_count);

	/* wifi radio off cnt */
	HIF_READ(0x70025410, &value);
	BT_DUMP_CR_PRINT(value);

	/* bt radio off cnt */
	HIF_READ(0x70025414, &value);
	BT_DUMP_CR_PRINT(value);

	/* BT 1st power on */
	HIF_READ(0x18860000, &value);
	BT_DUMP_CR_PRINT(value);

	/* UDS cnt */
	HIF_READ(0x70003014, &value);
	BT_DUMP_CR_PRINT(value);

	/* CBTOP_STRAP_03*/
	HIF_READ(0x7001000C, &value);
	BT_DUMP_CR_PRINT(value);

	/* CBTOP_STRAP_01 */
	HIF_READ(0x70010004, &value);
	BT_DUMP_CR_PRINT(value);

	/* GPIO_DIN0 */
	HIF_READ(0x70005200, &value);
	BT_DUMP_CR_PRINT(value);

	/* VLP_MISC_RSRV_ALL0_0 */
	HIF_READ(0x70007200, &value);
	BT_DUMP_CR_PRINT(value);

	/* VLP_MISC_RSRV_ALL1_0 */
	HIF_READ(0x70007210, &value);
	BT_DUMP_CR_PRINT(value);

	/* VLP_MISC_RSRV_ALL1_1 */
	HIF_READ(0x70007214, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_cbtop(void)
{
	uint32_t i = 0, value, cr_count = 14;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [CBTOP] count[%d]", HIF_DBG_TAG, cr_count);

	for (i = 0; i < cr_count; i++) {
		HIF_READ(0x70005300 + (i << 4), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_conninfra(void)
{
	uint32_t value, cr_count = 3;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [CONNINFRA] count[%d]", HIF_DBG_TAG, cr_count);

	/* CONN_HOST_CSR_TOP_CONNSYS_PWR_STATES */
	HIF_READ(0x7C060A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* CONN_HOST_CSR_TOP_CONN_INFRA_SYSSTRAP_OUT */;
	HIF_READ(0x7C060A00, &value);
	BT_DUMP_CR_PRINT(value);

	/* UNDEF in CODA */;
	HIF_READ(0x7C060230, &value);
	BT_DUMP_CR_PRINT(value);

	/* CONN_RGU_ON_BGFSYS_ON_TOP_PWR_CTL */;
	HIF_READ(0x7C000020, &value);
	BT_DUMP_CR_PRINT(value);

	/* UNDEF in CODA */
	HIF_READ(0x70025304, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_mcu_var1(void)
{
	uint32_t i = 0, value, cr_count = 69;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR1] ctrl_info_ram[0], g1_exp_counter, g_exp_type, g4_exp_1st_PC_log count[%d]"
			, HIF_DBG_TAG, cr_count);

	/* CACHE memory REMAP : 0xE0900000 -> 0x18C00000 for patch not enable yet */
	HIF_WRITE(0x18815018, 0xE0900000);

	/* ctrl_info_ram[0] */
	HIF_READ(0x18B16F60, &value);
	BT_DUMP_CR_PRINT(value);

	/* g1_exp_counter */
	HIF_READ(0x188C8F4F, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_type */
	HIF_READ(0x18B172D0, &value);
	BT_DUMP_CR_PRINT(value);

	/* g4_exp_1st_PC_log */
	for (i = 0; i < 66; i++) {
		HIF_READ(0x18B17004 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_mcu_var1_dx4(void)
{
	uint32_t i = 0, value, cr_count = 69;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR1] ctrl_info_ram[0], g1_exp_counter, g_exp_type, g4_exp_1st_PC_log count[%d]"
			, HIF_DBG_TAG, cr_count);

	/* CACHE memory REMAP : 0x00B00000 -> 0x18C00000 for patch not enable yet */
	HIF_WRITE(0x7C815018, 0x00B00000);

	/* ctrl_info_ram[0] */
	HIF_READ(0x7CB00678, &value);
	BT_DUMP_CR_PRINT(value);

	/* g1_exp_counter */
	HIF_READ(0x7C8C89CC, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_type */
	HIF_READ(0x7CB00914, &value);
	BT_DUMP_CR_PRINT(value);

	/* g4_exp_1st_PC_log */
	for (i = 0; i < 66; i++) {
		HIF_READ(0x7CB00708 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_mcu_var2(void)
{
	uint32_t i = 0, value, cr_count = 42;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR2] csr_reg_value, register_value count[%d]", HIF_DBG_TAG, cr_count);

	/* csr_reg_value */
	for (i = 0; i < 5; i++) {
		HIF_READ(0x188C8D54 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	/* register_value */
	for (i = 0; i < 37; i++) {
		HIF_READ(0x188C96D8 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_mcu_var2_dx4(void)
{
	uint32_t i = 0, value, cr_count = 42;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR2] csr_reg_value, register_value count[%d]", HIF_DBG_TAG, cr_count);

	/* csr_reg_value */
	for (i = 0; i < 6; i++) {
		HIF_READ(0x188C8818 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
	/* register_value */
	for (i = 0; i < 37; i++) {
		HIF_READ(0x188C9470 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

}

static inline void btmtk_hif_dump_mcu_var3(void)
{
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR3] g_exp_eva ~ processing_eintx count[%d]", HIF_DBG_TAG, cr_count);

	/* g_exp_eva */
	HIF_READ(0x18B171F4, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_ipc */
	HIF_READ(0x18B171FC, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_itype */
	HIF_READ(0x18B17204, &value);
	BT_DUMP_CR_PRINT(value);

	/* Current_Task_Id */
	HIF_READ(0x188C8324, &value);
	BT_DUMP_CR_PRINT(value);

	/* Current_Task_Indx */
	HIF_READ(0x188C8328, &value);
	BT_DUMP_CR_PRINT(value);

	/* COS_Interrupt_Count */
	HIF_READ(0x188C0038, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_irqx */
	HIF_READ(0x188C96D2, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_lisr */
	HIF_READ(0x188C96D4, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_eint_lisr */
	HIF_READ(0x188C96CC, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_eintx */
	HIF_READ(0x188C96D0, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_mcu_var3_dx4(void)
{
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR3] g_exp_eva ~ processing_eintx count[%d]", HIF_DBG_TAG, cr_count);

	/* g_exp_eva */
	HIF_READ(0x18B00838, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_ipc */
	HIF_READ(0x18B00840, &value);
	BT_DUMP_CR_PRINT(value);

	/* g_exp_itype */
	HIF_READ(0x18B00848, &value);
	BT_DUMP_CR_PRINT(value);

	/* Current_Task_Id */
	HIF_READ(0x188C7DEC, &value);
	BT_DUMP_CR_PRINT(value);

	/* Current_Task_Indx */
	HIF_READ(0x188C7DF0, &value);
	BT_DUMP_CR_PRINT(value);

	/* COS_Interrupt_Count */
	HIF_READ(0x188C0024, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_irqx */
	HIF_READ(0x188C9464, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_lisr */
	HIF_READ(0x188C9468, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_eint_lisr */
	HIF_READ(0x188C945C, &value);
	BT_DUMP_CR_PRINT(value);

	/* processing_eintx */
	HIF_READ(0x188C9460, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_mcu_var_stack(void)
{
	uint32_t i = 0, pos, value, cr_count = 64;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);

	HIF_READ(REMAP_DLM(0x02209764), &pos);
	BTMTK_INFO("%s [MCU VAR STACK] 0x02209764[0x%08x] count[%d]", HIF_DBG_TAG, pos, cr_count);

	if (pos == 0) {
		BTMTK_WARN("%s %s: pos is invalid, skip", HIF_DBG_TAG, __func__);
		return;
	}

	pos = REMAP_DLM(pos);
	/* <Stack> */
	for (i = 0; i < cr_count; i++) {
		HIF_READ(pos + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcu_var_stack_dx4(void)
{
	uint32_t i = 0, pos, value, cr_count = 64;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);

	HIF_READ(REMAP_DLM(0x022094FC), &pos);
	BTMTK_INFO("%s [MCU VAR STACK] 0x022094FC[0x%08x] count[%d]", HIF_DBG_TAG, pos, cr_count);

	if (pos == 0) {
		BTMTK_WARN("%s %s: pos is invalid, skip", HIF_DBG_TAG, __func__);
		return;
	}

	pos = REMAP_DLM(pos);
	/* <Stack> */
	for (i = 0; i < cr_count; i++) {
		HIF_READ(pos + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_mcu_var_assert_log_buf(void)
{
	uint32_t i = 0, value, cr_count = 180 / 4;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [MCU VAR ASSERT LOG BUF] count[%d]", HIF_DBG_TAG, cr_count);

	/* g_exp_assert_log_buf */
	for (i = 0; i < cr_count; i++) {
		HIF_READ(0x18B17138 + (i << 2), &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}
}

static inline void btmtk_hif_dump_bt_top_host_dx4(void)
{
	uint32_t value, cr_count = 6 + 6 + 1 + 26 + 14 + 7 + 4 + 1 + 1;
	uint32_t i = 0;

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [bt_top host dump] count[%d]", HIF_DBG_TAG, cr_count);

    /* bg_bt_cfg_Debug_Signal */
	for (i = 0; i < 6; i++) {
		HIF_WRITE(0x20023A0C, 0xC0000000 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* bg_bt_clkgen_off_Signal */
	for (i = 0; i < 6; i++) {
		HIF_WRITE(0x20023A0C, 0xC0000100 + i);
		HIF_READ(0x20023A10, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* bg_bt_met_Debug_Signal */
	HIF_WRITE(0x20023A0C, 0xC0000200);
	HIF_READ(0x20023A10, &value);
	BT_DUMP_CR_PRINT(value);

	/* bg_bt_cfg_on_Debug_Signal */
	for (i = 0; i < 26; i++) {
		HIF_WRITE(0x20023A1C, 0x00300040 + i);
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* bg_bt_rgu_on_Debug_Signal */
	for (i = 0; i < 14; i++) {
		HIF_WRITE(0x20023A1C, 0x00300001 + i);
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	/* bg_bt_rgu_on_Debug_Signal */
	for (i = 0; i < 4; i++) {
		HIF_WRITE(0x20023A1C, 0x00300080 + i);
		HIF_READ(0x20023A20, &value);
		if (BT_DUMP_CR_PRINT(value))
			return;
	}

	HIF_WRITE(0x20023A1C, 0x00300100);
	HIF_READ(0x20023A20, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x20023A1C, 0x00300140);
	HIF_READ(0x20023A20, &value);
	BT_DUMP_CR_PRINT(value);


	/* bg_bt_misc_vlp_Debug_Signal */
	HIF_WRITE(0x20060C04, 0x00030000);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x20060C04, 0x00030010);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x20060C04, 0x00030020);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x20060C04, 0x00030050);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

	/* bg_bt_met_on_Debug_Signal */
	HIF_WRITE(0x20060C04, 0x00030040);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

/* bg_bt_kmem_Debug_Signal */
	HIF_WRITE(0x20060C04, 0x00030100);
	HIF_READ(0x20060C00, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_bt_top_mcu_dx4(void)
{
	uint32_t value, cr_count = 40;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s [bt_top mcu dump] count[%d]", HIF_DBG_TAG, cr_count);

	HIF_READ(0x7C860000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860020, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860028, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860030, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860034, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C860128, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C86012C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821120, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821148, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821200, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821210, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821504, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821508, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C82150C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821510, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821640, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821644, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821648, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C82164C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C821700, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_READ(0x7C820004, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C820010, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C82006C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C820088, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C820090, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812100, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812104, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812108, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812114, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812118, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812120, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812124, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812150, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812168, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81216C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812170, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812174, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812178, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C81217C, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C812400, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C825230, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x7C82523C, &value);
	BT_DUMP_CR_PRINT(value);

}

void btmtk_hif_dump_work(struct work_struct *work)
{
	struct btmtk_dev *bdev = container_of(work, struct btmtk_dev, hif_dump_work);
	struct btmtk_uart_dev *cif_dev = NULL;
	int ret = 0;

	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}
	cif_dev = (struct btmtk_uart_dev *)bdev->cif_dev;
	if (cif_dev == NULL) {
		BTMTK_ERR("%s: cif_dev is NULL", __func__);
		return;
	}
	BTMTK_INFO("%s: start, bdev->chip_id: 0x%04x", __func__, bdev->chip_id);
	/*
	 * connv3_drv_type
	 *
	 *	  CONNV3_DRV_TYPE_BT = 0
	 *	  CONNV3_DRV_TYPE_WIFI = 1
	 */

#if 0 // test: wifi trigger bt
	uint32_t value;

	ret = connv3_hif_dbg_start(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT);
	if (ret) {
		BTMTK_ERR("%s: [BT]connv3_hif_dbg_start fail, ret[%d]", __func__, ret);
	}
	connv3_hif_dbg_write(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT, 0x81025020, 0x00000605);
	connv3_hif_dbg_read(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT, 0x8000040C, &value);
	BTMTK_INFO("[BT test] mcu_flg3 0x8000040C[0x%08x]", value);
	ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_WIFI, CONNV3_DRV_TYPE_BT);
#endif

	ret = connv3_hif_dbg_start(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI);
	if (ret) {
		BTMTK_ERR("%s: connv3_hif_dbg_start fail, ret[%d]", __func__, ret);
		goto exit;
	}
	BTMTK_INFO("%s version = %s", UNIFIED_DBG_TAG, MT6653_BT_DEBUGSOP_DUMP_VERSION);

	switch (bdev->chip_id) {
	case 0x6635:
	case 0x6639:
		/* HIF_MCU dump */
		btmtk_hif_dump_bg_mcu_core();
		btmtk_hif_dump_dsp_debug_flags();
		btmtk_hif_dump_mcusys_clk_gals_debug_flags();
		btmtk_hif_dump_mcu_pc_lr();
		//btmtk_hif_dump_dsp_pc_lr();
		btmtk_hif_dump_peri_debug_flags();
		btmtk_hif_dump_bus_debug_flags();
		btmtk_hif_dump_dma_uart_debug_flags();
		btmtk_hif_dump_cryto_debug_flags();

		/* HIF_UART dump */
		btmtk_hif_dump_dma1();
		btmtk_hif_dump_bg_sysram1();
		btmtk_hif_dump_bg_sysram2();
		btmtk_hif_dump_bg_sysram3();
		btmtk_hif_dump_hif_uart1();
		btmtk_hif_dump_hif_uart2();
		btmtk_hif_dump_cirq_eint();
		btmtk_hif_dump_bg_cfg();
		btmtk_hif_dump_bt_mcusys_vlp();
		btmtk_hif_dump_bt_hif_select();
		btmtk_hif_dump_bt_pos_check();
		btmtk_hif_dump_cbtop();
		btmtk_hif_dump_conninfra();
		btmtk_hif_dump_mcu_var1();
		btmtk_hif_dump_mcu_var2();
		btmtk_hif_dump_mcu_var3();
		btmtk_hif_dump_mcu_var_stack();
		btmtk_hif_dump_mcu_var_assert_log_buf();
		ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI);
		break;
	case 0x6653:
		mt6653_hif_dumpReg();
#if 1
		/* HIF_MCU dump */
		btmtk_hif_dump_bg_mcu_core_dx4();
		btmtk_hif_dump_dsp_debug_flags_dx4();
		btmtk_hif_dump_mcusys_clk_gals_debug_flags_dx4();
		btmtk_hif_dump_mcu_pc_lr_dx4();
		btmtk_hif_dump_mcusys_common_off_dx4();
		//
		btmtk_hif_dump_peri_debug_flags_dx4();
		btmtk_hif_dump_bus_debug_flags_dx4();
		btmtk_hif_dump_dma_uart_debug_flags_dx4();
		btmtk_hif_dump_cryto_debug_flags_dx4();

		/* HIF_UART dump */
		btmtk_hif_dump_dma1_dx4();
		btmtk_hif_dump_bg_sysram1();
		btmtk_hif_dump_bg_sysram2();
		btmtk_hif_dump_bg_sysram3();
		btmtk_hif_dump_hif_uart1_dx4();
		btmtk_hif_dump_hif_uart2();
		btmtk_hif_dump_bg_cfg_dx4();
		btmtk_hif_dump_bt_mcusys_vlp_dx4();
		btmtk_hif_dump_bt_hif_select_dx4();
		btmtk_hif_dump_bt_pos_check_dx4();
		btmtk_hif_dump_cbtop();
		btmtk_hif_dump_conninfra();
		btmtk_hif_dump_mcu_var1_dx4();
		btmtk_hif_dump_mcu_var2_dx4();
		btmtk_hif_dump_mcu_var3_dx4();
		btmtk_hif_dump_mcu_var_stack_dx4();

		/* bt_top host dump */
		btmtk_hif_dump_bt_top_host_dx4();

		/* bt_top mcu dump */
		btmtk_hif_dump_bt_top_mcu_dx4();
#endif
		ret = connv3_hif_dbg_end(CONNV3_DRV_TYPE_BT, CONNV3_DRV_TYPE_WIFI);
		break;
	default:
		BTMTK_ERR("Invalid bdev->chip_id: 0x%04x!", bdev->chip_id);
		break;
	}

exit:
	if (atomic_read(&bdev->assert_state) == BTMTK_ASSERT_START && btmtk_get_chip_state(bdev) != BTMTK_STATE_FW_DUMP) {
		BTMTK_INFO("%s: set bt assert_state end", __func__);
		atomic_set(&bdev->assert_state, BTMTK_ASSERT_END);
		complete_all(&bdev->dump_comp);
	} else
		BTMTK_INFO("%s: not set bt assert_state start yet, assert_state[%d], state[%d]", __func__, atomic_read(&bdev->assert_state), btmtk_get_chip_state(bdev));
}
void btmtk_hif_sp_dump_debug_sop(struct btmtk_dev *bdev)
{
	if (bdev == NULL) {
		BTMTK_ERR("%s: bdev is NULL", __func__);
		return;
	}
	BTMTK_INFO("%s: start", __func__);
	/* use schedule_work, cause connv3 may trigger assert and do hif debug sop */
	schedule_work(&bdev->hif_dump_work);
}
