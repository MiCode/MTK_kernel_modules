/**
 *  Copyright (c) 2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <btmtk_main.h>
#include "connv3.h"
#include "btmtk_uart_tty.h"


#define BT_CR_DUMP_BUF_SIZE		(1024)
#define DBG_TAG	"[btmtk_dbg_sop]"
#define RHW_DBG_TAG	"[btmtk_dbg_sop_rhw]"
#define HIF_DBG_TAG	"[btmtk_dbg_sop_hif]"

#define CHECK_ERR_MSG(val) (((val) >> 16) == 0xdead)

#define FW_CR_WR_CMD_PKT_LEN	24
#define FW_CR_RD_CMD_PKT_LEN	16
#define FW_CR_WR_EVT_PKT_LEN	11
#define FW_CR_RD_EVT_PKT_LEN	19
#define FW_CR_ADDR_LEN	4
#define FW_CR_DATA_LEN	4
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
	if ((pos & 0xffff0000) == 0x18020000)
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
			RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT);
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
			RETRY_TIMES, BTMTK_TX_PKT_SEND_DIRECT);

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

int CONNV3_RHW_WRITE(uint32_t addr, uint32_t val)
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

	BTMTK_DBG("%s: write addr[%x], value[0x%08x]", __func__, addr, val);
	memcpy(&cmd[FW_CR_CMD_ADDR_OFFSET], &addr, FW_CR_ADDR_LEN);
	memcpy(&cmd[FW_CR_CMD_DATA_OFFSET], &val, FW_CR_DATA_LEN);

	ret = btmtk_main_send_cmd(g_sbdev, cmd, FW_CR_WR_CMD_PKT_LEN, evt, FW_CR_WR_EVT_PKT_LEN,
			DELAY_TIMES, RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);
	if (ret < 0)
		BTMTK_ERR("%s failed", __func__);

	return ret ;
}

int CONNV3_RHW_READ(uint32_t addr, uint32_t *val)
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

	memcpy(&cmd[FW_CR_CMD_ADDR_OFFSET], &addr, sizeof(addr));
	memcpy(&evt[FW_CR_EVT_ADDR_OFFSET], &addr, sizeof(addr));

	ret = btmtk_main_send_cmd(g_sbdev, cmd, FW_CR_RD_CMD_PKT_LEN,
			evt, FW_CR_EVT_DATA_OFFSET, DELAY_TIMES,
			RETRY_TIMES, BTMTK_TX_CMD_FROM_DRV);

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
	return CONNV3_RHW_READ(addr, value);
}

static int btmtk_connv3_cr_write_cb(void* priv_data, unsigned int addr, unsigned int value)
{
	return CONNV3_RHW_WRITE(addr, value);
}

static int btmtk_connv3_cr_write_mask_cb(void* priv_data, unsigned int addr, unsigned int mask, unsigned int value)
{
	int ret = 0;
	uint32_t org_value;
	ret = CONNV3_RHW_READ(addr, &org_value);
	if (ret < 0) {
		BTMTK_ERR("%s: read [%x] err", __func__, addr);
		return ret;
	}

	org_value = (org_value & ~mask ) | value;

	return CONNV3_RHW_WRITE(addr, org_value);
}

struct connv3_cr_cb btmtk_connv3_cr_cb = {
	//.priv_data,
	.read = btmtk_connv3_cr_read_cb,
	.write = btmtk_connv3_cr_write_cb,
	.write_mask = btmtk_connv3_cr_write_mask_cb
};

void btmtk_uart_sp_dump_debug_sop(struct btmtk_dev *bdev)
{
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
	BTMTK_INFO("%s: end", __func__);
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

	HIF_READ(0x18812000, &value);
	if (value != 0x03000000) {
		BTMTK_WARN("%s: check 0x18812000 != 0x03000000 failed, value[0x%08x]", __func__, value);
		return -1;
	}
	return 0;
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
	for (i = 0; i <= 0x0D; i++) {
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

static inline void btmtk_hif_dump_dma1(void) {
	uint32_t value, cr_count = 6;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s readable check failed, skip", HIF_DBG_TAG, cr_count);
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


static inline void btmtk_hif_dump_bg_sysram1(void) {
	uint32_t value, pos, cr_count = 8;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x1881042C, &value);

	/* boundary check */
	if(value < 0x00400000 || value > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, value);
		return;
	}

	/* dynamic mapping to 0x1890_0000 */
	value = (value & ~(0x3)) - 32;
	HIF_WRITE(0x18815014, value);

	BTMTK_INFO("%s bg_sysram1 for 0x1881042C[0x%08x], read from 0x1890_0000, count[%d]"
				, HIF_DBG_TAG, value, cr_count);

	pos = 0x18900000;

	for (; cr_count > 0; cr_count--, pos+=4) {
		HIF_READ(pos, &value);
		BT_DUMP_CR_PRINT(value);
	}
}

static inline void btmtk_hif_dump_bg_sysram2(void) {
	uint32_t value, pos, cr_count = 8, base, vff_size;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x18810730, &pos);

	/* boundary check */
	if(pos < 0x00400000 || pos > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, pos);
		return;
	}

	pos += 0x18440000;

	HIF_READ(0x1881072C, &base);
	if (CHECK_ERR_MSG(base)) {
		BTMTK_INFO("%s %s: get error code 0x1881072C[0x%08x]", HIF_DBG_TAG, __func__, base);
		return;
	}

	base += 0x18440000;

	HIF_READ(0x18810744, &vff_size);
	if (CHECK_ERR_MSG(vff_size)) {
		BTMTK_INFO("%s %s: get error code 0x18810744[0x%08x]", HIF_DBG_TAG, __func__, vff_size);
		return;
	}

	BTMTK_INFO("%s bg_sysram2 for 0x18810730[0x%08x], base[0x%08x], vff_size[0x%08x], count[%d]"
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
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	HIF_READ(0x18810734, &pos);

	/* boundary check */
	if(pos < 0x00400000 || pos > 0x00430000) {
		BTMTK_INFO("%s %s: invalid boundary [0x%08x]", HIF_DBG_TAG, __func__, pos);
		return;
	}

	pos += 0x18440000;

	HIF_READ(0x1881072C, &base);
	if (CHECK_ERR_MSG(base)) {
		BTMTK_INFO("%s %s: get error code 0x1881072C[0x%08x]", HIF_DBG_TAG, __func__, base);
		return;
	}

	base += 0x18440000;


	HIF_READ(0x18810744, &vff_size);
	if (CHECK_ERR_MSG(vff_size)) {
		BTMTK_INFO("%s %s: get error code 0x18810744[0x%08x]", HIF_DBG_TAG, __func__, vff_size);
		return;
	}

	BTMTK_INFO("%s bg_sysram3 for 0x18810734[0x%08x], base[0x%08x], vff_size[0x%08x], count[%d]"
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
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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
static inline void btmtk_hif_dump_hif_uart2(void) {
	uint32_t value, cr_count = 12;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
		return;
	}

	BT_DUMP_CR_INIT(cr_count);
	BTMTK_INFO("%s HIF_UART(0x1881900C) count[%d]", HIF_DBG_TAG, cr_count);

	HIF_WRITE(0x1881900C, 0x00000010);
	HIF_READ(0x18819000, &value);
	BT_DUMP_CR_PRINT(value);
	HIF_READ(0x18819004, &value);
	BT_DUMP_CR_PRINT(value);

	HIF_WRITE(0x1881900C, 0x000000BF);
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

	HIF_WRITE(0x1881900C, 0x00000003);
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

}

static inline void btmtk_hif_dump_cirq_eint(void) {
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_bt_hif_select(void)
{
	uint32_t value, cr_count = 3;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_bt_pos_check(void)
{
	uint32_t value, cr_count = 8;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

	/* UNDEF in CODA */
	HIF_READ(0x70025304, &value);
	BT_DUMP_CR_PRINT(value);

}

static inline void btmtk_hif_dump_mcu_var1(void)
{
	uint32_t i = 0, value, cr_count = 69;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_mcu_var2(void)
{
	uint32_t i = 0, value, cr_count = 42;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_mcu_var3(void)
{
	uint32_t value, cr_count = 10;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_mcu_var_stack(void)
{
	uint32_t i = 0, pos, value, cr_count = 64;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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

static inline void btmtk_hif_dump_mcu_var_assert_log_buf(void)
{
	uint32_t i = 0, value, cr_count = 180 / 4;

	if (btmtk_connv3_readable_check()) {
		BTMTK_INFO("%s %s: readable check failed, skip", HIF_DBG_TAG, __func__, cr_count);
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
	BTMTK_INFO("%s: start", __func__);
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
exit:
	if (atomic_read(&bdev->assert_state) == BTMTK_ASSERT_START) {
		BTMTK_INFO("%s: set bt assert_state end", __func__);
		atomic_set(&bdev->assert_state, BTMTK_ASSERT_END);
		complete_all(&bdev->dump_comp);
	} else
		BTMTK_INFO("%s: not set bt assert_state start yet", __func__);
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
