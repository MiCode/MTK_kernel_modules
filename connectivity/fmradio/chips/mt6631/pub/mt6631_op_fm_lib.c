/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include "mt6631_op_fm_lib.h"
#include "plat.h"
#include "fm_typedef.h"
#include "fm_dbg.h"
#include "fm_err.h"
#include "fm_interface.h"
#include "fm_stdlib.h"
#include "fm_patch.h"
#include "fm_utils.h"
#include "fm_link.h"
#include "fm_config.h"
#include "fm_cmd.h"

#include "mt6631_fm_reg.h"

#define HQA_RETURN_ZERO_MAP 0

/* #define MT6631_FM_PATCH_PATH "/etc/firmware/mt6631/mt6631_fm_patch.bin" */
/* #define MT6631_FM_COEFF_PATH "/etc/firmware/mt6631/mt6631_fm_coeff.bin" */
/* #define MT6631_FM_HWCOEFF_PATH "/etc/firmware/mt6631/mt6631_fm_hwcoeff.bin" */
/* #define MT6631_FM_ROM_PATH "/etc/firmware/mt6631/mt6631_fm_rom.bin" */

struct fm_patch_tbl mt6631_patch_tbl[5] = {
	{FM_ROM_V1, "mt6631_fm_v1_patch.bin", "mt6631_fm_v1_coeff.bin", NULL, NULL},
	{FM_ROM_V2, "mt6631_fm_v2_patch.bin", "mt6631_fm_v2_coeff.bin", NULL, NULL},
	{FM_ROM_V3, "mt6631_fm_v3_patch.bin", "mt6631_fm_v3_coeff.bin", NULL, NULL},
	{FM_ROM_V4, "mt6631_fm_v4_patch.bin", "mt6631_fm_v4_coeff.bin", NULL, NULL},
	{FM_ROM_V5, "mt6631_fm_v5_patch.bin", "mt6631_fm_v5_coeff.bin", NULL, NULL}
};

struct fm_patch_tbl *mt6631_get_patch_tbl(void)
{
	return &(mt6631_patch_tbl[0]);
}

signed int mt6631_TurnOn_RgTopGbLdo(void)
{
	signed int ret;
	unsigned int tem = 0;

	/* turn on RG_TOP_BGLDO */
	ret = fm_top_reg_read(0x00c0, &tem);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "power up read top 0xc0 failed\n");
		return ret;
	}
	ret = fm_top_reg_write(0x00c0, tem | (0x3 << 27));
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "power up write top 0xc0 failed\n");
		return ret;
	}

	return ret;
}

signed int mt6631_pwrup_clock_on_reg_op(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 4;
	unsigned short de_emphasis;
	signed int hwid = fm_wcn_ops.ei.get_hw_version();
	/* unsigned short osc_freq; */

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	de_emphasis = fm_config.rx_cfg.deemphasis;
	de_emphasis &= 0x0001;	/* rang 0~1 */

	/* 3,enable MTCMOS */
	pkt_size += fm_bop_top_write(0x60, 0x00000030, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 60 30 */

	pkt_size += fm_bop_top_write(0x60, 0x00000035, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 60 35 */
	pkt_size += fm_bop_udelay(10, &buf[pkt_size], buf_size - pkt_size);
	/* delay 10us */
	pkt_size += fm_bop_top_write(0x60, 0x00000015, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 60 15 */
	pkt_size += fm_bop_top_write(0x60, 0x00000005, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 60 5 */

	pkt_size += fm_bop_udelay(10, &buf[pkt_size], buf_size - pkt_size);
	/* delay 10us */
	pkt_size += fm_bop_top_write(0x60, 0x00000045, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 60 45 */

	/* 4,set comspi fm slave dumy count	*/
	if (hwid >= FM_CONNAC_1_5)
		pkt_size += fm_bop_write(0x7f, 0x801f, &buf[pkt_size], buf_size - pkt_size);	/* wr 7f 801f */
	else
		pkt_size += fm_bop_write(0x7f, 0x800f, &buf[pkt_size], buf_size - pkt_size);	/* wr 7f 800f */

	/* A. FM digital clock enable */
	/* A1. Enable digital OSC */
	if (hwid <= FM_CONNAC_1_2)
		pkt_size += fm_bop_write(0x60, 0x00000001, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 1 */

	/* A2. Wait 3ms */
	pkt_size += fm_bop_udelay(3000, &buf[pkt_size], buf_size - pkt_size);

	/* A3. Set OSC clock output to FM */
	if (hwid <= FM_CONNAC_1_2)
		pkt_size += fm_bop_write(0x60, 0x00000003, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 3 */
	/* A4. Release HW clock gating*/
	pkt_size += fm_bop_write(0x60, 0x00000007, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 7 */
	/* Enable DSP auto clock gating */
	pkt_size += fm_bop_write(0x70, 0x0040, &buf[pkt_size], buf_size - pkt_size);	/* wr 70 0040 */
	/* A7. Deemphasis setting: Set 0 for 50us, Set 1 for 75us */
	pkt_size += fm_bop_modify(0x61, ~DE_EMPHASIS, (de_emphasis << 12), &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

signed int mt6631_pwrdown_reg_op(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 4;

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* A1:set audio output I2S Tx mode: */
	pkt_size += fm_bop_modify(0x9B, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);

	/* B0:Disable HW clock control */
	pkt_size += fm_bop_write(0x60, 0x330F, &buf[pkt_size], buf_size - pkt_size);
	/* B1:Reset ASIP : Set 0x61,  [D1 = 0, D0=1] */
	pkt_size += fm_bop_modify(0x61, 0xFFFD, 0x0001, &buf[pkt_size], buf_size - pkt_size);

	/* B2:digital core + digital rgf reset */
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_modify(0x6E, 0xFFF8, 0x0000, &buf[pkt_size], buf_size - pkt_size);

	/* B3:Disable all clock */
	pkt_size += fm_bop_write(0x60, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* B4:Reset rgfrf */
	pkt_size += fm_bop_write(0x60, 0x4000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x60, 0x0000, &buf[pkt_size], buf_size - pkt_size);

	/* MTCMOS power off */
	/* C0:disable MTCMOS */
	pkt_size += fm_bop_top_write(0x60, 0x0005, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_write(0x60, 0x0015, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_write(0x60, 0x0035, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_write(0x60, 0x0030, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_rd_until(0x60, 0x0000000A, 0x0, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

void mt6631_show_fm_reg(void)
{
	unsigned int debug_reg1[4] = {0};
	unsigned short debug_reg2[4] = {0};

	fm_top_reg_read(0x00c0, &debug_reg1[0]);
	fm_top_reg_read(0x00c8, &debug_reg1[1]);
	fm_top_reg_read(0x0060, &debug_reg1[2]);
	fm_top_reg_read(0x00a0, &debug_reg1[3]);
	fm_reg_read(0x7f, &debug_reg2[0]);
	fm_reg_read(0x62, &debug_reg2[1]);
	fm_reg_read(0x60, &debug_reg2[2]);
	fm_reg_read(0x61, &debug_reg2[3]);
	WCN_DBG(FM_ALT | CHIP,
		"top cr 0xc0 = 0x%08x, 0xc8 = 0x%08x, 0x60 = 0x%08x, 0xa0 = 0x%08x "
		"fmreg 0x7f = 0x%08x, 0x62 = 0x%08x, 0x60 = 0x%08x, 0x61 = 0x%08x\n",
		debug_reg1[0], debug_reg1[1], debug_reg1[2], debug_reg1[3],
		debug_reg2[0], debug_reg2[1], debug_reg2[2], debug_reg2[3]);
}

signed int mt6631_set_freq_fine_tune_reg_op(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 4;

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* disable DCOC IDAC auto-disable */
	pkt_size += fm_bop_modify(0x33, 0xFDFF, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* A1 Host control RF register */
	pkt_size += fm_bop_write(0x60, 0x0007, &buf[pkt_size], buf_size - pkt_size);
	/* F3 DCOC @ LNA = 7 */
	pkt_size += fm_bop_write(0x40, 0x01AF, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x03, 0xFAF5, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x01, 0xEEE8, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x3F, 0x3221, &buf[pkt_size], buf_size - pkt_size);
	/* wait 1ms */
	pkt_size += fm_bop_udelay(1000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_rd_until(0x3F, 0x001F, 0x0001, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x3F, 0x0220, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x40, 0x0100, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x01, 0xAEE8, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x30, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x36, 0x017A, &buf[pkt_size], buf_size - pkt_size);

	/* F4 set DSP control RF register */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

static const signed char mt6631_chan_para_map[] = {
/*      0, X, 1, X, 2, X, 3, X, 4, X, 5, X, 6, X, 7, X, 8, X, 9, X                   */
	0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6500~6595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6600~6695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 6700~6795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6800~6895 */
	0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6900~6995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7000~7095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7100~7195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 7200~7295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,	/* 7300~7395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,	/* 7400~7495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7500~7595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,	/* 7600~7695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7700~7795 */
	8, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7800~7895 */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7900~7995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8000~8095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8100~8195 */
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8200~8295 */
	0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 8300~8395 */
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8400~8495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8500~8595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8600~8695 */
	0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8700~8795 */
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8800~8895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8900~8995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9000~9095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9100~9195 */
	0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9200~9295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9300~9395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9400~9495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9500~9595 */
	3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9600~9695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9700~9795 */
	0, 0, 0, 0, 0, 0, 8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9800~9895 */
	0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,	/* 9900~9995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10000~10095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10100~10195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,	/* 10200~10295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0,	/* 10300~10395 */
	8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10400~10495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 10500~10595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10600~10695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,	/* 10700~10795 */
	0			/* 10800 */
};

static const unsigned short mt6631_TDD_list[] = {
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6500~6595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6600~6695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6700~6795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6800~6895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 6900~6995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7000~7095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7100~7195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7200~7295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7300~7395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7400~7495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7500~7595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7600~7695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7700~7795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7800~7895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 7900~7995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8000~8095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8100~8195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8200~8295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8300~8395 */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8400~8495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8500~8595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8600~8695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8700~8795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8800~8895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8900~8995 */
	0x0000, 0x0000, 0x0101, 0x0101, 0x0101,	/* 9000~9095 */
	0x0001, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9100~9195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9200~9295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9300~9395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9400~9495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9500~9595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9600~9695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100,	/* 9700~9795 */
	0x0101, 0x0101, 0x0101, 0x0101, 0x0101,	/* 9800~9895 */
	0x0101, 0x0101, 0x0001, 0x0000, 0x0000,	/* 9900~9995 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10000~10095 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10100~10195 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10200~10295 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10300~10395 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10400~10495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 10500~10595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0100,	/* 10600~10695 */
	0x0101, 0x0101, 0x0101, 0x0101, 0x0101,	/* 10700~10795 */
	0x0001			/* 10800 */
};

static const unsigned short mt6631_TDD_Mask[] = {
	0x0001, 0x0010, 0x0100, 0x1000
};

/* get channel parameter, HL side/ FA / ATJ */
unsigned short mt6631_chan_para_get(unsigned short freq)
{
	signed int pos, size;

	if (1 == HQA_RETURN_ZERO_MAP) {
		WCN_DBG(FM_NTC | CHIP, "HQA_RETURN_ZERO_CHAN mt6631_chan_para_map enabled!\n");
		return 0;
	}

	if (fm_get_channel_space(freq) == 0)
		freq *= 10;

	if (freq < 6500)
		return 0;

	pos = (freq - 6500) / 5;

	size = ARRAY_SIZE(mt6631_chan_para_map);

	pos = (pos > (size - 1)) ? (size - 1) : pos;

	return mt6631_chan_para_map[pos];
}

bool mt6631_TDD_chan_check(unsigned short freq)
{
	unsigned int i = 0;
	unsigned short freq_tmp = freq;
	signed int ret = 0;

	ret = fm_get_channel_space(freq_tmp);
	if (ret == 0)
		freq_tmp *= 10;
	else if (ret == -1)
		return false;

	i = (freq_tmp - 6500) / 5;
	if ((i / 4) >= ARRAY_SIZE(mt6631_TDD_list)) {
		WCN_DBG(FM_ERR | CHIP, "Freq index out of range(%d),max(%zd)\n",
			i / 4, ARRAY_SIZE(mt6631_TDD_list));
		return false;
	}

	if (mt6631_TDD_list[i / 4] & mt6631_TDD_Mask[i % 4]) {
		WCN_DBG(FM_DBG | CHIP, "Freq %d use TDD solution\n", freq);
		return true;
	} else
		return false;
}

