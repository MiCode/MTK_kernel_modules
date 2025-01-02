/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2016 MediaTek Inc.
 */

#include <linux/slab.h>
#include <linux/vmalloc.h>

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

#include "mt6635_fm_reg.h"
#include "mt6635_fm_lib.h"

#define HQA_RETURN_ZERO_MAP 0
#define HQA_ZERO_DESENSE_MAP 0
#define FM_POLLING_LIMIT     100

/* #include "mach/mt_gpio.h" */

/* #define MT6635_FM_PATCH_PATH "/etc/firmware/mt6635/mt6635_fm_patch.bin" */
/* #define MT6635_FM_COEFF_PATH "/etc/firmware/mt6635/mt6635_fm_coeff.bin" */
/* #define MT6635_FM_HWCOEFF_PATH "/etc/firmware/mt6635/mt6635_fm_hwcoeff.bin" */
/* #define MT6635_FM_ROM_PATH "/etc/firmware/mt6635/mt6635_fm_rom.bin" */

static struct fm_patch_tbl connac2x_patch_tbl[5] = {
	{FM_ROM_V1, "mt6635_fm_v1_patch.bin", "mt6635_fm_v1_coeff.bin", NULL, NULL},
	{FM_ROM_V2, "mt6635_fm_v2_patch.bin", "mt6635_fm_v2_coeff.bin", NULL, NULL},
	{FM_ROM_V3, "mt6635_fm_v3_patch.bin", "mt6635_fm_v3_coeff.bin", NULL, NULL},
	{FM_ROM_V4, "mt6635_fm_v4_patch.bin", "mt6635_fm_v4_coeff.bin", NULL, NULL},
	{FM_ROM_V5, "mt6635_fm_v5_patch.bin", "mt6635_fm_v5_coeff.bin", NULL, NULL}
};

static struct fm_hw_info connac2x_hw_info = {
	.chip_id = 0x00006635,
	.eco_ver = 0x00000000,
	.rom_ver = 0x00000000,
	.patch_ver = 0x00000000,
	.reserve = 0x00000000,
};

static struct fm_callback *fm_cb_op;

/* static bool rssi_th_set = false; */

#if 0				/* def CONFIG_MTK_FM_50KHZ_SUPPORT */
static struct fm_fifo *cqi_fifo;
#endif
static signed int connac2x_is_dese_chan(unsigned short freq);

static unsigned short connac2x_chan_para_get(unsigned short freq);
static signed int connac2x_desense_check(unsigned short freq, signed int rssi);
static bool connac2x_TDD_chan_check(unsigned short freq);
static bool connac2x_SPI_hopping_check(unsigned short freq);
static signed int connac2x_soft_mute_tune(unsigned short freq, signed int *rssi, signed int *valid);

static signed int connac2x_pwron(signed int data)
{
	if (fm_wcn_ops.ei.wmt_func_on && !fm_wcn_ops.ei.wmt_func_on()) {
		WCN_DBG(FM_ERR | CHIP, "WMT turn on FM Fail!\n");
		return -FM_ELINK;
	}

	WCN_DBG(FM_NTC | CHIP, "WMT turn on FM OK!\n");
	return 0;
}

static signed int connac2x_pwroff(signed int data)
{
	if (fm_wcn_ops.ei.wmt_func_off && !fm_wcn_ops.ei.wmt_func_off()) {
		WCN_DBG(FM_ERR | CHIP, "WMT turn off FM Fail!\n");
		return -FM_ELINK;
	}

	WCN_DBG(FM_NTC | CHIP, "WMT turn off FM OK!\n");
	return 0;
}

static unsigned short connac2x_get_chipid(void)
{
	return connac2x_hw_info.chip_id;
}

/*  connac2x_SetAntennaType - set Antenna type
 *  @type - 1, Short Antenna;  0, Long Antenna
 */
static signed int connac2x_SetAntennaType(signed int type)
{
	unsigned short dataRead = 0;

	WCN_DBG(FM_DBG | CHIP, "set ana to %s\n", type ? "short" : "long");
	fm_reg_read(FM_MAIN_CG2_CTRL, &dataRead);

	if (type)
		dataRead |= ANTENNA_TYPE;
	else
		dataRead &= (~ANTENNA_TYPE);

	fm_reg_write(FM_MAIN_CG2_CTRL, dataRead);

	return 0;
}

static signed int connac2x_GetAntennaType(void)
{
	unsigned short dataRead = 0;

	fm_reg_read(FM_MAIN_CG2_CTRL, &dataRead);
	WCN_DBG(FM_DBG | CHIP, "get ana type: %s\n", (dataRead & ANTENNA_TYPE) ? "short" : "long");

	if (dataRead & ANTENNA_TYPE)
		return FM_ANA_SHORT;	/* short antenna */
	else
		return FM_ANA_LONG;	/* long antenna */
}

static signed int connac2x_Mute(bool mute)
{
	signed int ret = 0;
	unsigned short dataRead = 0;

	WCN_DBG(FM_DBG | CHIP, "set %s\n", mute ? "mute" : "unmute");
	/* fm_reg_read(FM_MAIN_CTRL, &dataRead); */
	fm_reg_read(0x9C, &dataRead);

	/* fm_top_reg_write(0x0050, 0x00000007); */
	if (mute == 1)
		ret = fm_reg_write(0x9C, (dataRead & 0xFFFC) | 0x0003);
	else
		ret = fm_reg_write(0x9C, (dataRead & 0xFFFC));

	/* fm_top_reg_write(0x0050, 0x0000000F); */

	return ret;
}

static signed int connac2x_rampdown_reg_op(unsigned char *buf, signed int buf_size)
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


	/* A1. Clear DSP state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFFF0, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* A2. Set DSP ramp down state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFFFF, RAMP_DOWN, &buf[pkt_size], buf_size - pkt_size);
	/* @Wait for STC_DONE interrupt@ */
	pkt_size += fm_bop_rd_until(FM_MAIN_INTR, FM_INTR_STC_DONE, FM_INTR_STC_DONE, &buf[pkt_size],
				    buf_size - pkt_size);
	/* A6. Clear DSP ramp down state */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, (~RAMP_DOWN), 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* A7. Write 1 clear the STC_DONE interrupt status flag */
	pkt_size += fm_bop_modify(FM_MAIN_INTR, 0xFFFF, FM_INTR_STC_DONE, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}
/*
 * connac2x_rampdown - f/w will wait for STC_DONE interrupt
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_rampdown(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	pkt_size = connac2x_rampdown_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_RAMPDOWN_OPCODE, pkt_size);
}

/* FMSYS Ramp Down Sequence*/
static signed int connac2x_RampDown(void)
{
	struct fm_ext_interface *ei = &fm_wcn_ops.ei;
	signed int ret = 0;
	unsigned short pkt_size;
	/* unsigned short tmp; */

	WCN_DBG(FM_DBG | CHIP, "ramp down\n");

	/* disable SPI hopping */
	if (ei->disable_spi_hopping)
		ret = ei->disable_spi_hopping();

	/* A0.0 Host control RF register */
	ret = fm_set_bits(0x60, 0x0007, 0xFFF0);  /*Set 0x60 [D3:D0] = 0x7*/
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down HOST control rf: Set 0x60 [D3:D0] = 0x7 failed\n");
		return ret;
	}

	/* A0.1 Update FM ADPLL fast tracking mode gain */
	ret = fm_set_bits(0x0F, 0x0000, 0xF800);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down ADPLL gainA/B: Set 0xFH [D10:D0] = 0x000 failed\n");
		return ret;
	}

	/* A0.2 Host control RF register */
	ret = fm_set_bits(0x60, 0x000F, 0xFFF0);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x60 failed\n");
		return ret;
	}
	/* A1. Clear dsp state*/
	ret = fm_set_bits(0x63, 0x0000, 0xFFF0);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x63 failed\n");
		return ret;
	}
	/* A2. Set DSP ramp down state*/
	ret = fm_set_bits(0x63, 0x0010, 0xFFEF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down Host control RF registerwr top 0x63 failed\n");
		return ret;
	}

    /* A3. Clear STC_DONE interrupt */
	ret = fm_reg_write(FM_MAIN_INTRMASK, 0x0000);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "ramp down clean FM_MAIN_INTRMASK failed\n");

	ret = fm_reg_write(FM_MAIN_EXTINTRMASK, 0x0000);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down clean FM_MAIN_EXTINTRMASK failed\n");
		return ret;
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = connac2x_rampdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_RAMPDOWN, SW_RETRY_CNT, RAMPDOWN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down failed\n");
		return ret;
	}

	ret = fm_reg_write(FM_MAIN_EXTINTRMASK, 0x0021);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "ramp down wr FM_MAIN_EXTINTRMASK failed\n");
		return ret;
	}

	ret = fm_reg_write(FM_MAIN_INTRMASK, 0x0021);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "ramp down wr FM_MAIN_INTRMASK failed\n");

	return ret;
}

static signed int connac2x_get_rom_version(void)
{
	unsigned short flag_Romcode = 0;
	unsigned short nRomVersion = 0;
#define ROM_CODE_READY 0x0001
	signed int ret = 0;

	/* A1.1 DSP rom code version request enable --- set 0x61 b15=1 */
	fm_set_bits(0x61, 0x8000, 0x7FFF);

	/* A1.2 Release ASIP reset --- set 0x61 b1=1 */
	fm_set_bits(0x61, 0x0002, 0xFFFD);

	/* A1.3 Enable ASIP power --- set 0x61 b0=0 */
	fm_set_bits(0x61, 0x0000, 0xFFFE);

	/* A1.4 Wait until DSP code version ready --- wait loop 1ms */
	do {
		fm_delayus(1000);
		ret = fm_reg_read(0x84, &flag_Romcode);
		/* ret=-4 means signal got when control FM. usually get sig 9 to kill FM process. */
		/* now cancel FM power up sequence is recommended. */
		if (ret)
			return ret;

		WCN_DBG(FM_DBG | CHIP, "ROM_CODE_READY flag 0x84=%x\n", flag_Romcode);
	} while (flag_Romcode != ROM_CODE_READY);


	/* A1.5 Read FM DSP code version --- rd 0x83[15:8] */
	fm_reg_read(0x83, &nRomVersion);
	nRomVersion = (nRomVersion >> 8);

	/* A1.6 DSP rom code version request disable --- set 0x61 b15=0 */
	fm_set_bits(0x61, 0x0000, 0x7FFF);

	/* A1.7 Reset ASIP --- set 0x61[1:0] = 1 */
	fm_set_bits(0x61, 0x0001, 0xFFFC);

	return (signed int) nRomVersion;
}

static signed int connac2x_pwrup_clock_on_reg_op(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 4;
	unsigned short de_emphasis;
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

#if 0
	if (connac2x_hw_info.chip_id == 0x6637) {
		/* enable FM RF xo buffer TODO: remove it */
		pkt_size += fm_bop_top_write(0xB18, 0x00000004, &buf[pkt_size], buf_size - pkt_size);
	}
#endif

	/* 3,enable MTCMOS */
	pkt_size += fm_bop_top_write(0x160, 0x00000030, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 160 30 */

	pkt_size += fm_bop_top_write(0x160, 0x00000035, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 160 35 */
	pkt_size += fm_bop_udelay(10, &buf[pkt_size], buf_size - pkt_size);
	/* delay 10us */
	pkt_size += fm_bop_top_write(0x160, 0x00000015, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 160 15 */
	pkt_size += fm_bop_top_write(0x160, 0x00000005, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 160 5 */

	pkt_size += fm_bop_udelay(10, &buf[pkt_size], buf_size - pkt_size);
	/* delay 10us */
	pkt_size += fm_bop_top_write(0x160, 0x00000045, &buf[pkt_size], buf_size - pkt_size);
	/* wr top 160 45 */

	/* 4,set comspi fm slave dumy count	*/
	pkt_size += fm_bop_write(0x7f, 0x801f, &buf[pkt_size], buf_size - pkt_size);	/* wr 7f 801f */

	/* A. FM digital clock enable */
	/* A1. Wait 3ms */
	pkt_size += fm_bop_udelay(3000, &buf[pkt_size], buf_size - pkt_size);

	/* A2. Release HW clock gating*/
	pkt_size += fm_bop_write(0x60, 0x0007, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 7 */
	/* A3. Enable DSP auto clock gating */
	pkt_size += fm_bop_write(0x70, 0x0040, &buf[pkt_size], buf_size - pkt_size);	/* wr 70 0040 */
	/* A4. Deemphasis setting: Set 0 for 50us, Set 1 for 75us */
	pkt_size += fm_bop_modify(0x61, ~DE_EMPHASIS, (de_emphasis << 12), &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}
/*
 * connac2x_pwrup_clock_on - Wholechip FM Power Up: step 1, FM Digital Clock enable
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_pwrup_clock_on(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	pkt_size = connac2x_pwrup_clock_on_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static signed int connac2x_pwrup_digital_init_reg_op(unsigned char *buf, signed int buf_size)
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

	/* Part D   FM RF&ADPLL divider setting */

	/* D2.1 set cell mode */
	/* wr 30 D3:D2 00:FDD(default),01:both.10: TDD, 11 FDD */
	/* pkt_size += fm_bop_modify(0x30, 0xFFF3, 0x0000, &buf[pkt_size], buf_size - pkt_size); */

	/* D2.2 set ADPLL divider */
	pkt_size += fm_bop_write(0x21, 0xE000, &buf[pkt_size], buf_size - pkt_size);	/* wr 21 E000 */
	/* D2.3 set SDM coeff0_H */
	pkt_size += fm_bop_write(0xD8, 0x03F0, &buf[pkt_size], buf_size - pkt_size);	/* wr D8 0x03F0 */
	/* D2.4 set SDM coeff0_L */
	pkt_size += fm_bop_write(0xD9, 0x3F04, &buf[pkt_size], buf_size - pkt_size);	/* wr D9 0x3F04 */
	/* D2.5 set SDM coeff1_H */
	pkt_size += fm_bop_write(0xDA, 0x0014, &buf[pkt_size], buf_size - pkt_size);	/* wr DA 0x0014 */
	/* D2.6 set SDM coeff1_L */
	pkt_size += fm_bop_write(0xDB, 0x2A38, &buf[pkt_size], buf_size - pkt_size);	/* wr DB 0x2A38 */

	if (connac2x_hw_info.chip_id == 0x6635) {
		/* D2.7 set 26M clock */
		pkt_size += fm_bop_write(0x23, 0x4000, &buf[pkt_size], buf_size - pkt_size); /* wr 23 4000 */
	}

	/* Part E: FM Digital Init: fm_rgf_maincon */

	/* E4. Set appropriate interrupt mask behavior as desired */
	/* Enable stc_done_mask, Enable rgf_rds_mask*/
	pkt_size += fm_bop_write(0x6A, 0x0021, &buf[pkt_size], buf_size - pkt_size);	/* wr 6A 0021 */
	pkt_size += fm_bop_write(0x6B, 0x0021, &buf[pkt_size], buf_size - pkt_size);	/* wr 6B 0021 */

	/* E5. Enable hw auto control */
	pkt_size += fm_bop_write(0x60, 0x0000000F, &buf[pkt_size], buf_size - pkt_size);	/* wr 60 f */

	/* E6. Release ASIP reset */
	pkt_size += fm_bop_modify(0x61, 0xFFFD, 0x0002, &buf[pkt_size], buf_size - pkt_size);	/* wr 61 D1=1 */
	/* E7. Enable ASIP power */
	pkt_size += fm_bop_modify(0x61, 0xFFFE, 0x0000, &buf[pkt_size], buf_size - pkt_size);	/* wr 61 D0=0 */

	/* E8 */
	pkt_size += fm_bop_udelay(100000, &buf[pkt_size], buf_size - pkt_size);	/* delay 100ms */
	/* E9. Check HW initial complete */
	pkt_size += fm_bop_rd_until(0x64, 0x001F, 0x0002, &buf[pkt_size], buf_size - pkt_size);	/* Poll 64[0~4] = 2 */

	return pkt_size - 4;
}

/*
 * connac2x_pwrup_digital_init - Wholechip FM Power Up: step 4, FM Digital Init: fm_rgf_maincon
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_pwrup_digital_init(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	pkt_size = connac2x_pwrup_digital_init_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static signed int mt6635_c2x_pwrup_fine_tune_reg_op(unsigned char *buf, signed int buf_size)
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

	/* F1 set host control RF register */
	pkt_size += fm_bop_write(0x60, 0x0007, &buf[pkt_size], buf_size - pkt_size);

	/* F2 fine tune RF setting */
	pkt_size += fm_bop_write(0x01, 0xBEE8, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x03, 0xF6ED, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x15, 0x0D80, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x16, 0x0068, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x17, 0x092A, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x34, 0x807F, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x35, 0x311E, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x40, 0x0100, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x03, 0xFAF5, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x05, 0x7A80, &buf[pkt_size], buf_size - pkt_size);

	/* F4 set DSP control RF register */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

static signed int mt6637_c2x_pwrup_fine_tune_reg_op(unsigned char *buf, signed int buf_size)
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

	/* C10.1 set host control RF register */
	pkt_size += fm_bop_write(0x60, 0x0007, &buf[pkt_size], buf_size - pkt_size);

	/* C10.2 fine tune RF setting */
	pkt_size += fm_bop_write(0x01, 0xBEE8, &buf[pkt_size], buf_size - pkt_size);
	/* C10.3 */
	pkt_size += fm_bop_write(0x03, 0xF6ED, &buf[pkt_size], buf_size - pkt_size);
	/* C10.4 */
	pkt_size += fm_bop_write(0x15, 0x0D80, &buf[pkt_size], buf_size - pkt_size);
	/* C10.5 */
	pkt_size += fm_bop_write(0x16, 0x0068, &buf[pkt_size], buf_size - pkt_size);
	/* C10.6 */
	pkt_size += fm_bop_write(0x17, 0x0932, &buf[pkt_size], buf_size - pkt_size);
	/* C10.7 */
	pkt_size += fm_bop_write(0x34, 0x807F, &buf[pkt_size], buf_size - pkt_size);
	/* C10.8 */
	pkt_size += fm_bop_write(0x35, 0x311E, &buf[pkt_size], buf_size - pkt_size);
	/* C10.9 */
	pkt_size += fm_bop_write(0x40, 0x0100, &buf[pkt_size], buf_size - pkt_size);

	/* C10.11 */
	pkt_size += fm_bop_write(0x43, 0x816F, &buf[pkt_size], buf_size - pkt_size);
	/* C10.12 wr 43[10]=1 */
	pkt_size += fm_bop_modify(0x6E, 0xF7FF, 0x0800, &buf[pkt_size], buf_size - pkt_size);
	/* C10.13 wait 52us */
	pkt_size += fm_bop_udelay(52, &buf[pkt_size], buf_size - pkt_size);
	/* C10.14 rd 42[5]=1 */
	pkt_size += fm_bop_rd_until(0x42, 0x0020, 0x0020, &buf[pkt_size], buf_size - pkt_size);
	/* C10.15 wr 43[10]=0 */
	pkt_size += fm_bop_modify(0x6E, 0xF7FF, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	/* C10.16 rd 42[4:0] && C10.17 wr 9[4:0]*/
	pkt_size += fm_bop_copy_by_mask(0x42, 0x9, 0x1F, &buf[pkt_size], buf_size - pkt_size);

	/* C11 set DSP control RF register */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

/*
 * connac2x_pwrup_fine_tune - Wholechip FM Power Up: step 5, FM RF fine tune setting
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_pwrup_fine_tune(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	if (connac2x_hw_info.chip_id == 0x6635)
		pkt_size = mt6635_c2x_pwrup_fine_tune_reg_op(buf, buf_size);
	else if (connac2x_hw_info.chip_id == 0x6637)
		pkt_size = mt6637_c2x_pwrup_fine_tune_reg_op(buf, buf_size);
	else
		WCN_DBG(FM_ERR | CHIP, "%s invalid chip_id:0x%08x\n", __func__,
			connac2x_hw_info.chip_id);

	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static signed int connac2x_pwrdown_reg_op(unsigned char *buf, signed int buf_size)
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
	pkt_size += fm_bop_top_write(0x160, 0x0005, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_write(0x160, 0x0015, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_top_write(0x160, 0x0035, &buf[pkt_size], buf_size - pkt_size);

	if (connac2x_hw_info.chip_id == 0x6635) {
		pkt_size += fm_bop_top_write(0x160, 0x0030, &buf[pkt_size], buf_size - pkt_size);
		pkt_size += fm_bop_top_rd_until(0x160, 0x0000000A, 0x0, &buf[pkt_size], buf_size - pkt_size);
	} else if (connac2x_hw_info.chip_id == 0x6637) {
		/* prevent current leakage */
		pkt_size += fm_bop_top_write(0x160, 0x0035, &buf[pkt_size], buf_size - pkt_size);
		pkt_size += fm_bop_top_rd_until(0x160, 0x0000000A, 0x0000000A, &buf[pkt_size], buf_size - pkt_size);
	}

	return pkt_size - 4;
}
/*
 * connac2x_pwrdown - Wholechip FM Power down: Digital Modem Power Down
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_pwrdown(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	pkt_size = connac2x_pwrdown_reg_op(buf, buf_size);
	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static signed int connac2x_tune_reg_op(unsigned char *buf, signed int buf_size, unsigned short freq,
					unsigned short chan_para)
{
	signed int pkt_size = 4;

	WCN_DBG(FM_ALT | CHIP, "%s enter connac2x_tune function\n", __func__);

	if (buf == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid pointer\n", __func__);
		return -1;
	}
	if (buf_size < TX_BUF_SIZE) {
		WCN_DBG(FM_ERR | CHIP, "%s invalid buf size(%d)\n", __func__, buf_size);
		return -2;
	}

	/* A2 Enable hardware controlled tuning sequence */
	pkt_size += fm_bop_modify(FM_MAIN_CTRL, 0xFFF8, TUNE, &buf[pkt_size], buf_size - pkt_size);/*Set 0x63 D0=1*/
	/* Wait for STC_DONE interrupt */


#ifdef FM_TUNE_USE_POLL
	/* A3 Wait for STC_DONE interrupt */
	/* A4 Wait for STC_DONE interrupt status flag */
	pkt_size += fm_bop_rd_until(FM_MAIN_INTR, FM_INTR_STC_DONE,
								FM_INTR_STC_DONE, &buf[pkt_size], buf_size - pkt_size);
	/* A6 Write 1 clear the STC_DONE interrupt status flag */
	pkt_size += fm_bop_modify(FM_MAIN_INTR, 0xFFFF, FM_INTR_STC_DONE, &buf[pkt_size], buf_size - pkt_size);
#endif

	pkt_size += fm_bop_udelay(100000, &buf[pkt_size], buf_size - pkt_size);
	WCN_DBG(FM_ALT | CHIP, "connac2x_tune delay 100 ms wait 0x69 to change\n");

	WCN_DBG(FM_ALT | CHIP, "%s leave connac2x_tune function\n", __func__);

	return pkt_size - 4;
}

/*
 * connac2x_tune - execute tune action,
 * @buf - target buf
 * @buf_size - buffer size
 * @freq - 760 ~ 1080, 100KHz unit
 * return package size
 */
static signed int connac2x_tune(unsigned char *buf, signed int buf_size, unsigned short freq,
				unsigned short chan_para)
{
	signed int pkt_size = 0;

	pkt_size = connac2x_tune_reg_op(buf, buf_size, freq, chan_para);
	return fm_op_seq_combine_cmd(buf, FM_TUNE_OPCODE, pkt_size);
}

/*
 * connac2x_pwrup_DSP_download - execute dsp/coeff patch dl action,
 * @patch_tbl - current chip patch table
 * return patch dl ok or not
 */
static signed int connac2x_pwrup_DSP_download(struct fm_patch_tbl *patch_tbl)
{
#define PATCH_BUF_SIZE (4096*6)
	signed int ret = 0;
	signed int patch_len = 0;
	unsigned char *dsp_buf = NULL;
	unsigned short tmp_reg = 0;

	if (fm_wcn_ops.ei.wmt_ic_info_get)
		connac2x_hw_info.eco_ver = fm_wcn_ops.ei.wmt_ic_info_get();
	WCN_DBG(FM_DBG | CHIP, "ECO version:0x%08x\n", connac2x_hw_info.eco_ver);

	/*  Wholechip FM Power Up: step 3, get connac2x DSP ROM version */
	ret = connac2x_get_rom_version();
	if (ret >= 0) {
		connac2x_hw_info.rom_ver = ret;
		WCN_DBG(FM_NTC | CHIP, "%s ROM version: v%d\n", __func__, connac2x_hw_info.rom_ver);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get ROM version failed\n");
		if (ret == -4)
			WCN_DBG(FM_ERR | CHIP, "signal got when control FM, usually get sig 9 to kill FM process.\n");
			/* now cancel FM power up sequence is recommended. */
		goto out;
	}

	/* Wholechip FM Power Up: step 4 download patch */
	dsp_buf = fm_vmalloc(PATCH_BUF_SIZE);
	if (!dsp_buf) {
		WCN_DBG(FM_ALT | CHIP, "-ENOMEM\n");
		return -ENOMEM;
	}

	patch_len = fm_get_patch_path(connac2x_hw_info.rom_ver, dsp_buf, PATCH_BUF_SIZE, patch_tbl);
	if (patch_len <= 0) {
		WCN_DBG(FM_ALT | CHIP, " fm_get_patch_path failed\n");
		ret = patch_len;
		goto out;
	}

	ret = fm_download_patch((const unsigned char *)dsp_buf, patch_len, IMG_PATCH);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " DL DSPpatch failed\n");
		goto out;
	}

	patch_len = fm_get_coeff_path(connac2x_hw_info.rom_ver, dsp_buf, PATCH_BUF_SIZE, patch_tbl);
	if (patch_len <= 0) {
		WCN_DBG(FM_ALT | CHIP, " fm_get_coeff_path failed\n");
		ret = patch_len;
		goto out;
	}

	connac2x_hw_info.rom_ver += 1;

	tmp_reg = dsp_buf[38] | (dsp_buf[39] << 8);	/* to be confirmed */
	connac2x_hw_info.patch_ver = (signed int) tmp_reg;
	WCN_DBG(FM_NTC | CHIP, "Patch version: 0x%08x\n", connac2x_hw_info.patch_ver);

	ret = fm_download_patch((const unsigned char *)dsp_buf, patch_len, IMG_COEFFICIENT);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, " Download DSP coefficient failed\n");
		goto out;
	}

	/* Download HWACC coefficient */
	fm_reg_write(0x92, 0x0000);
	fm_reg_write(0x90, 0x0040); /* Reset download control  */
	fm_reg_write(0x90, 0x0000); /* Disable memory control from host*/
out:
	if (dsp_buf)
		fm_vfree(dsp_buf);
	return ret;
}
static void connac2x_show_reg(void)
{
	struct fm_ext_interface *ei = &fm_wcn_ops.ei;
	unsigned int debug_reg1[3] = {0};
	unsigned short debug_reg2[3] = {0};

	if (ei->host_reg_dump)
		ei->host_reg_dump();

	fm_top_reg_read(0x003c, &debug_reg1[0]);
	fm_top_reg_read(0x0a18, &debug_reg1[1]);
	fm_top_reg_read(0x0160, &debug_reg1[2]);
	fm_reg_read(0x7f, &debug_reg2[0]);
	fm_reg_read(0x62, &debug_reg2[1]);
	fm_reg_read(0x60, &debug_reg2[2]);
	WCN_DBG(FM_ALT | CHIP,
		"top cr 0x3c = 0x%08x, 0xa18 = 0x%08x, 0x160 = 0x%08x, fmreg 0x7f = 0x%08x, 0x62 = 0x%08x, 0x60 = 0x%08x\n",
		debug_reg1[0], debug_reg1[1], debug_reg1[2], debug_reg2[0], debug_reg2[1], debug_reg2[2]);
}

static signed int connac2x_PowerUp(unsigned short *chip_id, unsigned short *device_id)
{
	struct fm_ext_interface *ei = &fm_wcn_ops.ei;
	signed int ret = 0;
	unsigned short pkt_size;
	unsigned short tmp_reg = 0;
	unsigned int tem = 0;

	if (chip_id == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (device_id == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (ei->host_pre_on == NULL || ei->host_post_on == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	WCN_DBG(FM_DBG | CHIP, "pwr on seq......\n");

	ret = ei->host_pre_on();
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Host pre on failed\n");
		return ret;
	}

	if (!(ei->is_aoc_support())) {
		/* B1 Enable Top Clock */
		ret = fm_top_reg_write(0xA00, 0xFFFFFFFF);
		if (ret) {
			WCN_DBG(FM_ALT | CHIP, "Enable top clock failed\n");
			return ret;
		}
	} else
		WCN_DBG(FM_NTC | CHIP, "skip 0xA00 write\n");

	/* B2 Read A-die id */
	ret = fm_top_reg_read(0x02C, &tem);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Enable top clock failed\n");
		return ret;
	}

	if ((tem & 0xFFFF0000) == 0x66350000 ||
		(tem & 0xFFFF0000) == 0x66370000) {
		*chip_id = tem >> 16;
		connac2x_hw_info.chip_id = (signed int) *chip_id;
		WCN_DBG(FM_NTC | CHIP, "A-die id 0x%08x\n", tem);
	} else {
		WCN_DBG(FM_ALT | CHIP, "Unexpected A-die id 0x%08x\n", tem);
		return -FM_EFW;
	}

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;

	pkt_size = connac2x_pwrup_clock_on(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "connac2x_pwrup_clock_on failed\n");
		return ret;
	}

	/* Wholechip FM Power Up: step 2, read HW version */
	connac2x_show_reg();
	fm_reg_read(0x62, &tmp_reg);
	if (tmp_reg == *chip_id)
		*device_id = tmp_reg;
	else {
		connac2x_show_reg();
		WCN_DBG(FM_NTC | CHIP, "fm sys error, A die id 0x%04x\n", tmp_reg);
		return -FM_EFW;
	}

	/* Wholechip FM Power Up: step 3, patch download */
	ret = connac2x_pwrup_DSP_download(connac2x_patch_tbl);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "connac2x_pwrup_DSP_download failed\n");
		return ret;
	}

	/* Wholechip FM Power Up: step 4, FM Digital Init: fm_rgf_maincon */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = connac2x_pwrup_digital_init(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "connac2x_pwrup_digital_init failed\n");
		return ret;
	}

	/* Wholechip FM Power Up: step 5, FM RF fine tune setting */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = connac2x_pwrup_fine_tune(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "connac2x_pwrup_fine_tune failed\n");
		return ret;
	}

	/* Enable connsys FM 2 wire RX */
	fm_reg_write(0x9B, 0xF9AB);                /* G1: Set audio output i2s TX mode */

	ret = ei->host_post_on();
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Host post on failed\n");
		return ret;
	}

	WCN_DBG(FM_NTC | CHIP, "pwr on seq ok\n");
	return ret;
}

static signed int connac2x_PowerDown(void)
{
	struct fm_ext_interface *ei = &fm_wcn_ops.ei;
	signed int ret = 0;
	unsigned short pkt_size;

	if (ei->host_pre_off == NULL || ei->host_post_off == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	WCN_DBG(FM_DBG | CHIP, "pwr down seq\n");

	/* Disable D-die Audio RX */
	ret = ei->host_pre_off();
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Host post on failed\n");
		return ret;
	}

	/* disable SPI hopping */
	if (ei->disable_spi_hopping)
		ret = ei->disable_spi_hopping();

	if (ret)
		WCN_DBG(FM_ALT | CHIP, "PowerDown: Enable 26M crystal sleep failed\n");

	/* A0:set audio output I2X Rx mode: */
	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = connac2x_pwrdown(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);

	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "connac2x_pwrdown failed\n");
		return ret;
	}

	/* RF power off */

	/* SLP_CTRL setting */
	ret = ei->host_post_off();
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "Host post on failed\n");
		return ret;
	}

	return ret;
}

static signed int mt6635_c2x_set_freq_fine_tune_reg_op(unsigned char *buf, signed int buf_size)
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
	pkt_size += fm_bop_write(0x07, 0x0100, &buf[pkt_size], buf_size - pkt_size);
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
	/* set threshold = 4 */
	pkt_size += fm_bop_modify(0x3F, 0x0FFF, 0x4000, &buf[pkt_size], buf_size - pkt_size);
	/* enable DCOC IDAC auto-disable */
	pkt_size += fm_bop_modify(0x33, 0xFDFF, 0x0200, &buf[pkt_size], buf_size - pkt_size);

	/* F4 set DSP control RF register */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

static signed int mt6637_c2x_set_freq_fine_tune_reg_op(unsigned char *buf, signed int buf_size)
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
	pkt_size += fm_bop_write(0x03, 0xF6ED, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x07, 0x0140, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x01, 0xEEE8, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x3F, 0x3221, &buf[pkt_size], buf_size - pkt_size);
	/* wait 1ms */
	pkt_size += fm_bop_udelay(1000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_rd_until(0x3F, 0x001F, 0x0001, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x3F, 0x0220, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x40, 0x0100, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x01, 0xBEE8, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x30, 0x0000, &buf[pkt_size], buf_size - pkt_size);
	pkt_size += fm_bop_write(0x36, 0x017A, &buf[pkt_size], buf_size - pkt_size);
	/* set threshold = 3 */
	pkt_size += fm_bop_modify(0x3F, 0x0FFF, 0x3000, &buf[pkt_size], buf_size - pkt_size);
	/* enable DCOC IDAC auto-disable */
	pkt_size += fm_bop_modify(0x33, 0xFDFF, 0x0200, &buf[pkt_size], buf_size - pkt_size);

	/* F4 set DSP control RF register */
	pkt_size += fm_bop_write(0x60, 0x000F, &buf[pkt_size], buf_size - pkt_size);

	return pkt_size - 4;
}

/*
 * connac2x_set_freq_fine_tune - FM RF fine tune setting
 * @buf - target buf
 * @buf_size - buffer size
 * return package size
 */
static signed int connac2x_set_freq_fine_tune(unsigned char *buf, signed int buf_size)
{
	signed int pkt_size = 0;

	if (connac2x_hw_info.chip_id == 0x6635)
		pkt_size = mt6635_c2x_set_freq_fine_tune_reg_op(buf, buf_size);
	else if (connac2x_hw_info.chip_id == 0x6637)
		pkt_size = mt6637_c2x_set_freq_fine_tune_reg_op(buf, buf_size);
	else
		WCN_DBG(FM_ERR | CHIP, "%s invalid chip_id:0x%08x\n", __func__,
			connac2x_hw_info.chip_id);

	return fm_op_seq_combine_cmd(buf, FM_ENABLE_OPCODE, pkt_size);
}

static bool connac2x_do_SPI_hopping(unsigned short freq)
{
	struct fm_ext_interface *ei = &fm_wcn_ops.ei;
	signed int ret = -1;

	if (!connac2x_SPI_hopping_check(freq))
		return true;

	/* SPI hopping setting*/
	WCN_DBG(FM_NTC | CHIP,
		"%s: freq:%d is SPI hopping channel,turn on 64M PLL\n",
		__func__, freq);

	/* switch SPI clock to 64MHz */
	if (ei->spi_hopping)
		ret = ei->spi_hopping();

	return ret == 0;
}

static bool connac2x_SetFreq(unsigned short freq)
{
	signed int ret = 0;
	unsigned short pkt_size;
	unsigned short chan_para = 0;
	unsigned short freq_reg = 0;
	unsigned short tmp_reg[6] = {0};

	fm_cb_op->cur_freq_set(freq);

	/* FM VCO Calibration */
	ret = fm_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);

	fm_delayus(5);
	if (freq >= 750) {
		fm_reg_write(0x37, 0xF68C);
		fm_reg_write(0x38, 0x0B53);
		ret = fm_set_bits(0x30, 0x0014 << 8, 0xC0FF);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: Set 0x30 failed\n", __func__);

	} else {
		fm_reg_write(0x37, 0x0675);
		fm_reg_write(0x38, 0x0F54);
		ret = fm_set_bits(0x30, 0x001C << 8, 0xC0FF);
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: Set 0x30 failed\n", __func__);
	}
	ret = fm_set_bits(0x30, 0x0001 << 14, 0xBFFF);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x30 failed\n", __func__);

	fm_reg_write(0x40, 0x010F);
	fm_delayus(5);
	fm_reg_write(0x36, 0x037A);
	fm_reg_write(0x32, 0x8000);
	fm_delayus(200);
	ret = fm_set_bits(0x3D, 0x0001 << 2, 0xFFFB);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x3D failed\n", __func__);

	fm_reg_write(0x32, 0x0000);

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = connac2x_set_freq_fine_tune(cmd_buf, TX_BUF_SIZE);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_EN, SW_RETRY_CNT, EN_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);
	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "connac2x_pwrup_fine_tune failed\n");
		return ret;
	}

	/* A0. Host contrl RF register */
	ret = fm_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);

	/* A0.1 Update FM ADPLL fast tracking mode gain */
	ret = fm_set_bits(0x0F, 0x0455, 0xF800);
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set FM ADPLL gainA/B=0x455 failed\n", __func__);

	/* A0.2 Set FMSYS cell mode */
	if (connac2x_TDD_chan_check(freq)) {
		ret = fm_set_bits(0x30, 0x0008, 0xFFF3);	/* use TDD solution */
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: freq[%d]: use TDD solution failed\n", __func__, freq);
	} else {
		ret = fm_set_bits(0x30, 0x0000, 0xFFF3);	/* default use FDD solution */
		if (ret)
			WCN_DBG(FM_ERR | CHIP, "%s: freq[%d]: default use FDD solution failed\n", __func__, freq);
	}

	/* A0.3 Host control RF register */
	ret = fm_set_bits(0x60, 0x000F, 0xFFF0);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	/* A1 Get Channel parameter from map list*/

	chan_para = connac2x_chan_para_get(freq);
	WCN_DBG(FM_DBG | CHIP, "%s: %d chan para = %d\n", __func__, (signed int) freq, (signed int) chan_para);

	freq_reg = freq;
	if (fm_get_channel_space(freq_reg) == 0)
		freq_reg *= 10;

	freq_reg = (freq_reg - 6400) * 2 / 10;

	/*A1 Set rgfrf_chan = XXX*/
	ret = fm_set_bits(0x65, freq_reg, 0xFC00);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "%s: set rgfrf_chan = xxx = %d failed\n", __func__, freq_reg);
		return false;
	}

	ret = fm_set_bits(0x65, (chan_para << 12), 0x0FFF);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "set freq wr 0x65 failed\n");
		return false;
	}

	if (!connac2x_do_SPI_hopping(freq))
		WCN_DBG(FM_ERR | CHIP, "%s: spi hopping fail!\n", __func__);

	/* A0. Host contrl RF register */
	ret = fm_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);


	fm_reg_read(0x62, &tmp_reg[0]);
	fm_reg_read(0x64, &tmp_reg[1]);
	fm_reg_read(0x69, &tmp_reg[2]);
	fm_reg_read(0x6a, &tmp_reg[3]);
	fm_reg_read(0x6b, &tmp_reg[4]);
	fm_reg_read(0x9b, &tmp_reg[5]);

	WCN_DBG(FM_ALT | CHIP, "%s: Before tune--0x62 0x64 0x69 0x6a 0x6b 0x9b = %04x %04x %04x %04x %04x %04x\n",
			__func__,
			tmp_reg[0],
			tmp_reg[1],
			tmp_reg[2],
			tmp_reg[3],
			tmp_reg[4],
			tmp_reg[5]);

	/* A0.3 Host control RF register */
	ret = fm_set_bits(0x60, 0x000F, 0xFF00);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	if (FM_LOCK(cmd_buf_lock))
		return false;
	pkt_size = connac2x_tune(cmd_buf, TX_BUF_SIZE, freq, chan_para);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_TUNE | FLAG_TUNE_DONE, SW_RETRY_CNT, TUNE_TIMEOUT, NULL);
	FM_UNLOCK(cmd_buf_lock);


	/* A0. Host contrl RF register */
	ret = fm_set_bits(0x60, 0x0007, 0xFFF0);  /* Set 0x60 [D3:D0] = 0x07*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Host Control RF register 0x60 = 0x7 failed\n", __func__);

	memset(tmp_reg, 0, sizeof(tmp_reg[0])*6);

	fm_reg_read(0x62, &tmp_reg[0]);
	fm_reg_read(0x64, &tmp_reg[1]);
	fm_reg_read(0x69, &tmp_reg[2]);
	fm_reg_read(0x6a, &tmp_reg[3]);
	fm_reg_read(0x6b, &tmp_reg[4]);
	fm_reg_read(0x9b, &tmp_reg[5]);

	WCN_DBG(FM_ALT | CHIP, "%s: After tune--0x62 0x64 0x69 0x6a 0x6b 0x9b = %04x %04x %04x %04x %04x %04x\n",
			__func__,
			tmp_reg[0],
			tmp_reg[1],
			tmp_reg[2],
			tmp_reg[3],
			tmp_reg[4],
			tmp_reg[5]);

	/* A0.3 Host control RF register */
	ret = fm_set_bits(0x60, 0x000F, 0xFF00);	/* Set 0x60 [D3:D0] = 0x0F*/
	if (ret)
		WCN_DBG(FM_ERR | CHIP, "%s: Set 0x60 [D3:D0] = 0x0F failed\n", __func__);

	if (ret) {
		WCN_DBG(FM_ALT | CHIP, "%s: connac2x_tune failed\n", __func__);
		return false;
	}

	WCN_DBG(FM_DBG | CHIP, "%s: set freq to %d ok\n", __func__, freq);
#if 0
	/* ADPLL setting for dbg */
	fm_top_reg_write(0x0050, 0x00000007);
	fm_top_reg_write(0x0A08, 0xFFFFFFFF);
	connac2x_bt_write(0x82, 0x11);
	connac2x_bt_write(0x83, 0x11);
	connac2x_bt_write(0x84, 0x11);
	fm_top_reg_write(0x0040, 0x1C1C1C1C);
	fm_top_reg_write(0x0044, 0x1C1C1C1C);
	fm_reg_write(0x70, 0x0010);
	/*0x0806 DCO clk
	*0x0802 ref clk
	*0x0804 feedback clk
	*/
	fm_reg_write(0xE0, 0x0806);
#endif
	return true;
}

#define FM_CQI_LOG_PATH "/mnt/sdcard/fmcqilog"

static signed int connac2x_full_cqi_get(signed int min_freq, signed int max_freq, signed int space, signed int cnt)
{
	signed int ret = 0;
	unsigned short pkt_size;
	unsigned short freq, orig_freq;
	signed int i, j, k;
	signed int space_val, max, min, num;
	struct mt6635_full_cqi *p_cqi;
	unsigned char *cqi_log_title = "Freq, RSSI, PAMD, PR, FPAMD, MR, ATDC, PRX, ATDEV, SMGain, DltaRSSI\n";
	unsigned char cqi_log_buf[100] = { 0 };
	signed int pos;
	unsigned char cqi_log_path[100] = { 0 };

	/* for soft-mute tune, and get cqi */
	freq = fm_cb_op->cur_freq_get();
	if (fm_get_channel_space(freq) == 0)
		freq *= 10;

	/* get cqi */
	orig_freq = freq;
	if (fm_get_channel_space(min_freq) == 0)
		min = min_freq * 10;
	else
		min = min_freq;

	if (fm_get_channel_space(max_freq) == 0)
		max = max_freq * 10;
	else
		max = max_freq;

	if (space == 0x0001)
		space_val = 5;	/* 50Khz */
	else if (space == 0x0002)
		space_val = 10;	/* 100Khz */
	else if (space == 0x0004)
		space_val = 20;	/* 200Khz */
	else
		space_val = 10;

	num = (max - min) / space_val + 1;	/* Eg, (8760 - 8750) / 10 + 1 = 2 */
	for (k = 0; (orig_freq == 10000) && (g_dbg_level == 0xffffffff) && (k < cnt); k++) {
		WCN_DBG(FM_NTC | CHIP, "cqi file:%d\n", k + 1);
		freq = min;
		pos = 0;
		fm_memcpy(cqi_log_path, FM_CQI_LOG_PATH, strlen(FM_CQI_LOG_PATH));
		if (sprintf(&cqi_log_path[strlen(FM_CQI_LOG_PATH)], "%d.txt", k + 1) < 0)
			WCN_DBG(FM_NTC | CHIP, "sprintf fail\n");
		fm_file_write(cqi_log_path, cqi_log_title, strlen(cqi_log_title), &pos);
		for (j = 0; j < num; j++) {
			if (FM_LOCK(cmd_buf_lock))
				return -FM_ELOCK;
			pkt_size = fm_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
			ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT,
					SM_TUNE_TIMEOUT, fm_get_read_result);
			FM_UNLOCK(cmd_buf_lock);

			if (!ret && fm_res) {
				WCN_DBG(FM_NTC | CHIP, "smt cqi size %d\n", fm_res->cqi[0]);
				p_cqi = (struct mt6635_full_cqi *)&fm_res->cqi[2];
				for (i = 0; i < fm_res->cqi[1]; i++) {
					/* just for debug */
					WCN_DBG(FM_NTC | CHIP,
						"freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
						p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
						p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
						p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
						p_cqi[i].smg, p_cqi[i].drssi);
					/* format to buffer */
					if (sprintf(cqi_log_buf,
							"%04d, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x, %04x,\n",
							p_cqi[i].ch, p_cqi[i].rssi, p_cqi[i].pamd,
							p_cqi[i].pr, p_cqi[i].fpamd, p_cqi[i].mr,
							p_cqi[i].atdc, p_cqi[i].prx, p_cqi[i].atdev,
							p_cqi[i].smg, p_cqi[i].drssi) < 0)
						WCN_DBG(FM_NTC | CHIP, "sprintf fail\n");
					/* write back to log file */
					fm_file_write(cqi_log_path, cqi_log_buf, strlen(cqi_log_buf), &pos);
				}
			} else {
				WCN_DBG(FM_ALT | CHIP, "smt get CQI failed\n");
				ret = -1;
			}
			freq += space_val;
		}
		fm_cb_op->cur_freq_set(0);	/* avoid run too much times */
	}

	return ret;
}

/*
 * connac2x_GetCurRSSI - get current freq's RSSI value
 * RS=RSSI
 * If RS>511, then RSSI(dBm)= (RS-1024)/16*6
 *				else RSSI(dBm)= RS/16*6
 */
static signed int connac2x_GetCurRSSI(signed int *pRSSI)
{
	unsigned short tmp_reg = 0;

	fm_reg_read(FM_RSSI_IND, &tmp_reg);
	tmp_reg = tmp_reg & 0x03ff;

	if (pRSSI) {
		*pRSSI = (tmp_reg > 511) ? (((tmp_reg - 1024) * 6) >> 4) : ((tmp_reg * 6) >> 4);
		WCN_DBG(FM_DBG | CHIP, "rssi:%d, dBm:%d\n", tmp_reg, *pRSSI);
	} else {
		WCN_DBG(FM_ERR | CHIP, "get rssi para error\n");
		return -FM_EPARA;
	}

	return 0;
}

static unsigned short connac2x_vol_tbl[16] = { 0x0000, 0x0519, 0x066A, 0x0814,
	0x0A2B, 0x0CCD, 0x101D, 0x1449,
	0x198A, 0x2027, 0x287A, 0x32F5,
	0x4027, 0x50C3, 0x65AD, 0x7FFF
};

static signed int connac2x_SetVol(unsigned char vol)
{
	signed int ret = 0;

	vol = (vol > 15) ? 15 : vol;
	ret = fm_reg_write(0x7D, connac2x_vol_tbl[vol]);
	if (ret) {
		WCN_DBG(FM_ERR | CHIP, "Set vol=%d Failed\n", vol);
		return ret;
	}
	WCN_DBG(FM_DBG | CHIP, "Set vol=%d OK\n", vol);


	if (vol == 10) {
		fm_print_cmd_fifo();	/* just for debug */
		fm_print_evt_fifo();
	}
	return 0;
}

static signed int connac2x_GetVol(unsigned char *pVol)
{
	int ret = 0;
	unsigned short tmp = 0;
	signed int i;

	if (pVol == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	ret = fm_reg_read(0x7D, &tmp);
	if (ret) {
		*pVol = 0;
		WCN_DBG(FM_ERR | CHIP, "Get vol Failed\n");
		return ret;
	}

	for (i = 0; i < 16; i++) {
		if (connac2x_vol_tbl[i] == tmp) {
			*pVol = i;
			break;
		}
	}

	WCN_DBG(FM_DBG | CHIP, "Get vol=%d OK\n", *pVol);
	return 0;
}

static signed int connac2x_dump_reg(void)
{
	signed int i;
	unsigned short TmpReg = 0;

	for (i = 0; i < 0xff; i++) {
		fm_reg_read(i, &TmpReg);
		WCN_DBG(FM_NTC | CHIP, "0x%02x=0x%04x\n", i, TmpReg);
	}
	return 0;
}

/*0:mono, 1:stereo*/
static bool connac2x_GetMonoStereo(unsigned short *pMonoStereo)
{
#define FM_BF_STEREO 0x1000
	unsigned short TmpReg = 0;

	if (pMonoStereo) {
		fm_reg_read(FM_RSSI_IND, &TmpReg);
		*pMonoStereo = (TmpReg & FM_BF_STEREO) >> 12;
	} else {
		WCN_DBG(FM_ERR | CHIP, "MonoStero: para err\n");
		return false;
	}

	WCN_DBG(FM_NTC | CHIP, "Get MonoStero:0x%04x\n", *pMonoStereo);
	return true;
}

static signed int connac2x_SetMonoStereo(signed int MonoStereo)
{
	signed int ret = 0;

	WCN_DBG(FM_NTC | CHIP, "set to %s\n", MonoStereo ? "mono" : "auto");
	fm_reg_write(0x60, 0x0007);

	if (MonoStereo)	/*mono */
		ret = fm_set_bits(0x75, 0x0008, ~0x0008);
	else
		ret = fm_set_bits(0x75, 0x0000, ~0x0008);

	fm_reg_write(0x60, 0x000F);
	return ret;
}

static signed int connac2x_GetCapArray(signed int *ca)
{
	unsigned short dataRead = 0;
	unsigned short tmp = 0;

	if (ca == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	fm_reg_read(0x60, &tmp);
	fm_reg_write(0x60, tmp & 0xFFF7);	/* 0x60 D3=0 */

	fm_reg_read(0x26, &dataRead);
	*ca = dataRead;

	fm_reg_write(0x60, tmp);	/* 0x60 D3=1 */
	return 0;
}

/*
 * connac2x_GetCurPamd - get current freq's PAMD value
 * PA=PAMD
 * If PA>511 then PAMD(dB)=  (PA-1024)/16*6,
 *				else PAMD(dB)=PA/16*6
 */
static bool connac2x_GetCurPamd(unsigned short *pPamdLevl)
{
	unsigned short tmp_reg = 0;
	unsigned short dBvalue, valid_cnt = 0;
	int i, total = 0;

	for (i = 0; i < 8; i++) {
		if (fm_reg_read(FM_ADDR_PAMD, &tmp_reg)) {
			*pPamdLevl = 0;
			return false;
		}

		tmp_reg &= 0x03FF;
		dBvalue = (tmp_reg > 256) ? ((512 - tmp_reg) * 6 / 16) : 0;
		if (dBvalue != 0) {
			total += dBvalue;
			valid_cnt++;
			WCN_DBG(FM_DBG | CHIP, "[%d]PAMD=%d\n", i, dBvalue);
		}
		fm_delayms(3);
	}
	if (valid_cnt != 0)
		*pPamdLevl = total / valid_cnt;
	else
		*pPamdLevl = 0;

	WCN_DBG(FM_NTC | CHIP, "PAMD=%d\n", *pPamdLevl);
	return true;
}

static signed int connac2x_i2s_info_get(signed int *ponoff, signed int *pmode, signed int *psample)
{
	if (ponoff == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (pmode == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (psample == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	*ponoff = fm_config.aud_cfg.i2s_info.status;
	*pmode = fm_config.aud_cfg.i2s_info.mode;
	*psample = fm_config.aud_cfg.i2s_info.rate;

	return 0;
}

static signed int connac2x_get_audio_info(struct fm_audio_info_t *data)
{
	memcpy(data, &fm_config.aud_cfg, sizeof(struct fm_audio_info_t));
	return 0;
}

static signed int connac2x_hw_info_get(struct fm_hw_info *req)
{
	if (req == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,invalid pointer\n", __func__);
		return -FM_EPARA;
	}

	req->chip_id = connac2x_hw_info.chip_id;
	req->eco_ver = connac2x_hw_info.eco_ver;
	req->patch_ver = connac2x_hw_info.patch_ver;
	req->rom_ver = connac2x_hw_info.rom_ver;

	return 0;
}

static signed int connac2x_pre_search(void)
{
	connac2x_RampDown();
	/* disable audio output I2S Tx mode */
	fm_reg_write(0x9B, 0x0000);

	return 0;
}

static signed int connac2x_restore_search(void)
{
	connac2x_RampDown();
	/* set audio output I2S Tx mode */
	fm_reg_write(0x9B, 0xF9AB);
	return 0;
}

static signed int connac2x_soft_mute_tune(unsigned short freq, signed int *rssi, signed int *valid)
{
	signed int ret = 0;
	unsigned short pkt_size;
	struct mt6635_full_cqi *p_cqi;
	signed int RSSI = 0, PAMD = 0, MR = 0, ATDC = 0;
	unsigned int PRX = 0, ATDEV = 0;
	unsigned short softmuteGainLvl = 0;

    /* Set rgf_host2dsp_reserve[2] = 1 */

	ret = connac2x_chan_para_get(freq);
	if (ret == 2)
		ret = fm_set_bits(FM_CHANNEL_SET, 0x2000, 0x0FFF);	/* mdf HiLo */
	else
		ret = fm_set_bits(FM_CHANNEL_SET, 0x0000, 0x0FFF);	/* clear FA/HL/ATJ */

	if (!connac2x_do_SPI_hopping(freq))
		WCN_DBG(FM_ERR | CHIP, "%s: spi hopping fail!\n", __func__);

	if (FM_LOCK(cmd_buf_lock))
		return -FM_ELOCK;
	pkt_size = fm_full_cqi_req(cmd_buf, TX_BUF_SIZE, &freq, 1, 1);
	ret = fm_cmd_tx(cmd_buf, pkt_size, FLAG_SM_TUNE, SW_RETRY_CNT, SM_TUNE_TIMEOUT, fm_get_read_result);
	FM_UNLOCK(cmd_buf_lock);

	if (!ret && fm_res) {
		p_cqi = (struct mt6635_full_cqi *)&fm_res->cqi[2];
		/* just for debug */
		WCN_DBG(FM_NTC | CHIP,
			"freq %d, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x, 0x%04x\n",
			p_cqi->ch, p_cqi->rssi, p_cqi->pamd, p_cqi->pr, p_cqi->fpamd, p_cqi->mr,
			p_cqi->atdc, p_cqi->prx, p_cqi->atdev, p_cqi->smg, p_cqi->drssi);
		RSSI = ((p_cqi->rssi & 0x03FF) >= 512) ? ((p_cqi->rssi & 0x03FF) - 1024) : (p_cqi->rssi & 0x03FF);
		PAMD = ((p_cqi->pamd & 0x1FF) >= 256) ? ((p_cqi->pamd & 0x01FF) - 512) : (p_cqi->pamd & 0x01FF);
		MR = ((p_cqi->mr & 0x01FF) >= 256) ? ((p_cqi->mr & 0x01FF) - 512) : (p_cqi->mr & 0x01FF);
		ATDC = (p_cqi->atdc >= 32768) ? (65536 - p_cqi->atdc) : (p_cqi->atdc);
		if (ATDC < 0)
			ATDC = (~(ATDC)) - 1;	/* Get abs value of ATDC */

		PRX = (p_cqi->prx & 0x00FF);
		ATDEV = p_cqi->atdev;
		softmuteGainLvl = p_cqi->smg;
		/* check if the channel is valid according to each CQIs */
		if ((fm_config.rx_cfg.long_ana_rssi_th <= RSSI)
		    && (fm_config.rx_cfg.pamd_th >= PAMD)
		    && (fm_config.rx_cfg.atdc_th >= ATDC)
		    && (fm_config.rx_cfg.mr_th <= MR)
		    && (fm_config.rx_cfg.prx_th <= PRX)
		    && (ATDEV >= ATDC)	/* sync scan algorithm */
		    && (fm_config.rx_cfg.smg_th <= softmuteGainLvl)) {
			*valid = true;
		} else {
			*valid = false;
		}
		*rssi = RSSI;
	} else {
		WCN_DBG(FM_ALT | CHIP, "smt get CQI failed\n");
		return false;
	}
	WCN_DBG(FM_NTC | CHIP, "valid=%d\n", *valid);
	return true;
}

static bool connac2x_em_test(unsigned short group_idx, unsigned short item_idx, unsigned int item_value)
{
	return true;
}

/*
*parm:
*	parm.th_type: 0, RSSI. 1, desense RSSI. 2, SMG.
*	parm.th_val: threshold value
*/
static signed int connac2x_set_search_th(signed int idx, signed int val, signed int reserve)
{
	switch (idx) {
	case 0:	{
		fm_config.rx_cfg.long_ana_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set rssi th =%d\n", val);
		break;
	}
	case 1:	{
		fm_config.rx_cfg.desene_rssi_th = val;
		WCN_DBG(FM_NTC | CHIP, "set desense rssi th =%d\n", val);
		break;
	}
	case 2:	{
		fm_config.rx_cfg.smg_th = val;
		WCN_DBG(FM_NTC | CHIP, "set smg th =%d\n", val);
		break;
	}
	default:
		break;
	}
	return 0;
}

static signed int connac2x_low_power_wa_default(signed int fmon)
{
	return 0;
}

signed int connac2x_fm_low_ops_register(struct fm_callback *cb, struct fm_basic_interface *bi)
{
	signed int ret = 0;
	/* Basic functions. */

	if (bi == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,bi invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (cb->cur_freq_get == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,cb->cur_freq_get invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	if (cb->cur_freq_set == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,cb->cur_freq_set invalid pointer\n", __func__);
		return -FM_EPARA;
	}
	fm_cb_op = cb;

	bi->pwron = connac2x_pwron;
	bi->pwroff = connac2x_pwroff;
	bi->chipid_get = connac2x_get_chipid;
	bi->mute = connac2x_Mute;
	bi->rampdown = connac2x_RampDown;
	bi->pwrupseq = connac2x_PowerUp;
	bi->pwrdownseq = connac2x_PowerDown;
	bi->setfreq = connac2x_SetFreq;
	bi->low_pwr_wa = connac2x_low_power_wa_default;
	bi->get_aud_info = connac2x_get_audio_info;
	bi->rssiget = connac2x_GetCurRSSI;
	bi->volset = connac2x_SetVol;
	bi->volget = connac2x_GetVol;
	bi->dumpreg = connac2x_dump_reg;
	bi->msget = connac2x_GetMonoStereo;
	bi->msset = connac2x_SetMonoStereo;
	bi->pamdget = connac2x_GetCurPamd;
	bi->em = connac2x_em_test;
	bi->anaswitch = connac2x_SetAntennaType;
	bi->anaget = connac2x_GetAntennaType;
	bi->caparray_get = connac2x_GetCapArray;
	bi->hwinfo_get = connac2x_hw_info_get;
	bi->i2s_get = connac2x_i2s_info_get;
	bi->is_dese_chan = connac2x_is_dese_chan;
	bi->softmute_tune = connac2x_soft_mute_tune;
	bi->desense_check = connac2x_desense_check;
	bi->cqi_log = connac2x_full_cqi_get;
	bi->pre_search = connac2x_pre_search;
	bi->restore_search = connac2x_restore_search;
	bi->set_search_th = connac2x_set_search_th;

	cmd_buf_lock = fm_lock_create("31_cmd");
	ret = fm_lock_get(cmd_buf_lock);

	cmd_buf = fm_zalloc(TX_BUF_SIZE + 1);

	if (!cmd_buf) {
		WCN_DBG(FM_ALT | CHIP, "connac2x fm lib alloc tx buf failed\n");
		ret = -1;
	}

	return ret;
}

signed int connac2x_fm_low_ops_unregister(struct fm_basic_interface *bi)
{
	signed int ret = 0;
	/* Basic functions. */
	if (bi == NULL) {
		WCN_DBG(FM_ERR | CHIP, "%s,bi invalid pointer\n", __func__);
		return -FM_EPARA;
	}

#if 0				/* def CONFIG_MTK_FM_50KHZ_SUPPORT */
	fm_fifo_release(cqi_fifo);
#endif

	if (cmd_buf) {
		fm_free(cmd_buf);
		cmd_buf = NULL;
	}

	ret = fm_lock_put(cmd_buf_lock);
	fm_memset(bi, 0, sizeof(struct fm_basic_interface));

	fm_cb_op = NULL;

	return ret;
}

static const signed char connac2x_chan_para_map[] = {
/*      0, X, 1, X, 2, X, 3, X, 4, X, 5, X, 6, X, 7, X, 8, X, 9, X                   */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 6500~6595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6600~6695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 6700~6795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6800~6895 */
	0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 6900~6995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7000~7095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7100~7195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 7200~7295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7300~7395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,	/* 7400~7495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7500~7595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,	/* 7600~7695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7700~7795 */
	8, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7800~7895 */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 7900~7995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8000~8095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8100~8195 */
	0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8200~8295 */
	0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,	/* 8300~8395 */
	0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8400~8495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8500~8595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8600~8695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8700~8795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8800~8895 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 8900~8995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9000~9095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9100~9195 */
	0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9200~9295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9300~9395 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9400~9495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9500~9595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9600~9695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 9700~9795 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 9800~9895 */
	0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0,	/* 9900~9995 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10000~10095 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10100~10195 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10200~10295 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10300~10395 */
	8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10400~10495 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,	/* 10500~10595 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 10600~10695 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 0,	/* 10700~10795 */
	0			/* 10800 */
};

static const unsigned short connac2x_scan_dese_list[] = {
	6910, 6920, 7680, 7800, 8450, 9210, 9220, 9230, 9590, 9600, 9830, 9900, 9980, 9990, 10400, 10750, 10760
};

static const unsigned short connac2x_SPI_hopping_list[] = {
	6510, 6520, 6530, 7780, 7790, 7800, 7810, 7820, 9090, 9100, 9110, 9120, 10380, 10390, 10400, 10410, 10420
};

static const unsigned short connac2x_I2S_hopping_list[] = {
	6550, 6760, 6960, 6970, 7170, 7370, 7580, 7780, 7990, 8810, 9210, 9220, 10240
};

static const unsigned short mt6635_c2x_TDD_list[] = {
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
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9100~9195 */
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

static const unsigned short mt6637_c2x_TDD_list[] = {
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
	0x0000, 0x0000, 0x0000, 0x0100, 0x1000,	/* 8300~8395 */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8400~8495 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8500~8595 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8600~8695 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8700~8795 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8800~8895 */
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000,	/* 8900~8995 */
	0x0000, 0x0000, 0x0101, 0x0101, 0x0101,	/* 9000~9095 */
	0x0101, 0x0000, 0x0000, 0x0000, 0x0000,	/* 9100~9195 */
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
	0x0000, 0x0000, 0x0000, 0x0000, 0x0111,	/* 10600~10695 */
	0x0101, 0x0101, 0x0101, 0x0101, 0x0101,	/* 10700~10795 */
	0x0001			/* 10800 */
};

static const unsigned short connac2x_TDD_Mask[] = {
	0x0001, 0x0010, 0x0100, 0x1000
};

/* return value: 0, not a de-sense channel; 1, this is a de-sense channel; else error no */
static signed int connac2x_is_dese_chan(unsigned short freq)
{
	signed int size;

	if (1 == HQA_ZERO_DESENSE_MAP) /*HQA only :skip desense channel check. */
		return 0;

	size = ARRAY_SIZE(connac2x_scan_dese_list);

	if (fm_get_channel_space(freq) == 0)
		freq *= 10;

	while (size) {
		if (connac2x_scan_dese_list[size - 1] == freq)
			return 1;

		size--;
	}

	return 0;
}

/*  return value:
*1, is desense channel and rssi is less than threshold;
*0, not desense channel or it is but rssi is more than threshold.
*/
static signed int connac2x_desense_check(unsigned short freq, signed int rssi)
{
	if (connac2x_is_dese_chan(freq)) {
		if (rssi < fm_config.rx_cfg.desene_rssi_th)
			return 1;

		WCN_DBG(FM_DBG | CHIP, "desen_rssi %d th:%d\n", rssi, fm_config.rx_cfg.desene_rssi_th);
	}
	return 0;
}

static bool connac2x_TDD_chan_check(unsigned short freq)
{
	unsigned int i = 0;
	unsigned short freq_tmp = freq;
	signed int ret = 0;
	const unsigned short *pTDD_list;
	signed int array_size = 0;

	if (connac2x_hw_info.chip_id == 0x6635) {
		pTDD_list = &mt6635_c2x_TDD_list[0];
		array_size = ARRAY_SIZE(mt6635_c2x_TDD_list);
	} else if (connac2x_hw_info.chip_id == 0x6637) {
		pTDD_list = &mt6637_c2x_TDD_list[0];
		array_size = ARRAY_SIZE(mt6637_c2x_TDD_list);
	} else
		WCN_DBG(FM_ERR | CHIP, "%s invalid chip_id:0x%08x\n", __func__,
			connac2x_hw_info.chip_id);

	ret = fm_get_channel_space(freq_tmp);
	if (ret == 0)
		freq_tmp *= 10;
	else if (ret == -1)
		return false;

	i = (freq_tmp - 6500) / 5;
	if ((i / 4) >= array_size) {
		WCN_DBG(FM_ERR | CHIP, "Freq index out of range(%d),max(%d)\n",
			i / 4, array_size);
		return false;
	}

	if (pTDD_list[i / 4] & connac2x_TDD_Mask[i % 4]) {
		WCN_DBG(FM_DBG | CHIP, "Freq %d use TDD solution\n", freq);
		return true;
	} else
		return false;
}

/* get channel parameter, HL side/ FA / ATJ */
static unsigned short connac2x_chan_para_get(unsigned short freq)
{
	signed int pos, size;

	if (1 == HQA_RETURN_ZERO_MAP) {
		WCN_DBG(FM_NTC | CHIP, "HQA_RETURN_ZERO_CHAN connac2x_chan_para_map enabled!\n");
		return 0;
	}

	if (fm_get_channel_space(freq) == 0)
		freq *= 10;

	if (freq < 6500)
		return 0;

	pos = (freq - 6500) / 5;

	size = ARRAY_SIZE(connac2x_chan_para_map);

	pos = (pos > (size - 1)) ? (size - 1) : pos;

	return connac2x_chan_para_map[pos];
}


static bool connac2x_SPI_hopping_check(unsigned short freq)
{
	signed int size;

	size = ARRAY_SIZE(connac2x_SPI_hopping_list);

	if (fm_get_channel_space(freq) == 0)
		freq *= 10;

	while (size) {
		if (connac2x_SPI_hopping_list[size - 1] == freq)
			return 1;
		size--;
	}

	return 0;
}
