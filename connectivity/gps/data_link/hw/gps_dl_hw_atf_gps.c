/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"

#include "gps_dl_context.h"
#include "gps_dl_time_tick.h"
#include "gps_dsp_fsm.h"
#include "gps_dl_hw_api.h"
#include "gps_dl_hw_dep_macro.h"
#include "gps_dl_hw_priv_util.h"
#if GPS_DL_CONNAC2
#include "gps/bgf_gps_dma.h"
#elif GPS_DL_CONNAC3
#include "gps/conn_mcu_dma.h"
#include "gps/conn_mcu_config.h"
#endif
#include "gps/gps_aon_top.h"
#include "gps/gps_usrt_apb.h"
#include "gps/gps_l5_usrt_apb.h"

#if GPS_DL_CONNAC2
#define GPS_ADDR_ENTRY_NUM (1)
static const struct gps_dl_addr_map_entry g_gps_addr_table[GPS_ADDR_ENTRY_NUM] = {
	/* Put base list here: */
	/* BGF_GPS_CFG_BASE */
	{0x18C00000, 0x80000000, 0x90000},
};
#elif GPS_DL_CONNAC3
static const struct gps_dl_addr_map_entry g_gps_addr_table[] = {
	/* Put base list here: */
	{0x18C00000, 0x80000000, 0x10000}, /* MCU_CONFG  */
	{0x18C10000, 0x80010000,  0x1000}, /* DMA */
	{0x18C11000, 0x80020000,  0x1000}, /* BG_GPS_RGU */
	{0x18C12000, 0x80021000,  0x1000}, /* BG_GPS_CFG */
	{0x18C13000, 0x80022000,  0x1000}, /* BG_GPS_MISC_CTL */
	{0x18C14000, 0x80023000,  0x1000}, /* BG_GPS_MET_TOP */
	{0x18C15000, 0x80030000,  0x1000}, /* BUS_MON */
	{0x18C16000, 0x80050000,  0x1000}, /* BG_mcu_bus_cr_off */
	{0x18C19000, 0x80090000,  0x1000}, /* BG_MCU_UART0 */
	{0x18C1D000, 0x800D0000,  0x1000}, /* BG_MCU_UART1 */
	{0x18C1E000, 0x80100000,  0x1000}, /* BG_BUS_BCRM */
	{0x18C1F000, 0x80101000,  0x1000}, /* BG_BUS_DEBUG */
	{0x18C20000, 0x81020000,  0x1000}, /* BG_GPS_RGU_ON */
	{0x18C21000, 0x81021000,  0x1000}, /* BG_GPS_CFG_ON */
	{0x18C22000, 0x81022000,  0x1000}, /* BG_GPS_VLP_TOP */
	{0x18C23000, 0x81023000,  0x1000}, /* BG_MCU_BUS_CR_ON */
	{0x18C24000, 0x81024000,  0x1000}, /* BG_MCU_DEVAPC_AON */
	{0x18C25000, 0x81025000,  0x1000}, /* BG_MCU_CONFG_ON */
	{0x18C26000, 0x81026000,  0x1000}, /* BG_MCU_WDT */
	{0x18C27000, 0x81027000,  0x1000}, /* BG_MCU_GPT */
	{0x18C28000, 0x81028000,  0x1000}, /* BG_MCU_CIRQ/BG_MCU_DBG_CIRQ */
	{0x18C2B000, 0x80102000,  0x1000}, /* DEV_APC */
	{0x18C30000, 0x00900000, 0x40000}, /* ILM RAM */
	{0x18C70000, 0x80070000, 0x10000}, /* GPS_L1 */
	{0x18C80000, 0x80080000, 0x10000}, /* GPS_L5 */
	{0x18C90000, 0x00400000, 0x20000}, /* SYSRAM */
	{0x18CB0000, 0x80040000, 0x10000}, /* AES */
	{0x18CC0000, 0x02200000, 0x40000}, /* DLM RAM */
};
#define GPS_ADDR_ENTRY_NUM \
	(sizeof(g_gps_addr_table)/sizeof(struct gps_dl_addr_map_entry))
#endif

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#include "gps_dl_hw_atf.h"

unsigned int gps_bus_to_host(unsigned int gps_addr)
{
	unsigned int i;
	const struct gps_dl_addr_map_entry *p = NULL;

	for (i = 0; i < GPS_ADDR_ENTRY_NUM; i++) {
		p = &g_gps_addr_table[i];
		if (gps_addr >= p->bus_addr &&
			gps_addr < (p->bus_addr + p->length))
			return ((gps_addr - p->bus_addr) + p->host_addr);
	}

	return 0;
}


void gps_dl_hw_set_dma_start(enum gps_dl_hal_dma_ch_index channel,
	struct gdl_hw_dma_transfer *p_transfer)
{
	gps_dl_hw_dep_set_dma_start(channel, p_transfer);
}

void gps_dl_hw_set_dma_stop(enum gps_dl_hal_dma_ch_index channel)
{
	/* Poll until DMA IDLE */
	gps_dl_hw_dep_set_dma_stop(channel);
}

bool gps_dl_hw_get_dma_int_status(enum gps_dl_hal_dma_ch_index channel)
{
	/* ASSERT(channel >= 0 && channel <= GPS_DL_DMA_CH_NUM); */
	return gps_dl_hw_dep_get_dma_int_status(channel);
}

void gps_dl_hw_save_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	gps_dl_hw_dep_save_dma_status_struct(ch, p);
}

void gps_dl_hw_print_dma_status_struct(
	enum gps_dl_hal_dma_ch_index ch, struct gps_dl_hw_dma_status_struct *p)
{
	if (!gps_dl_show_reg_wait_log())
		return;

	gps_dl_hw_dep_print_dma_status_struct(ch, p);
}

enum GDL_RET_STATUS gps_dl_hw_wait_until_dma_complete_and_stop_it(
	enum gps_dl_hal_dma_ch_index ch, int timeout_usec, bool return_if_not_start)
{
	struct gps_dl_hw_dma_status_struct dma_status;
	struct gps_dl_hw_usrt_status_struct usrt_status;
	enum gps_dl_link_id_enum link_id = DMA_CH_TO_LINK_ID(ch);
	bool last_rw_log_on = false;
	unsigned long tick0, tick1;
	bool conninfra_okay = true;
	bool do_stop = true;
	enum GDL_RET_STATUS ret = GDL_OKAY;
	int loop_cnt;

	tick0 = gps_dl_tick_get();
	loop_cnt = 0;
	while (1) {
		gps_dl_hw_save_dma_status_struct(ch, &dma_status);
		if (gps_dl_only_show_wait_done_log())
			last_rw_log_on = gps_dl_set_show_reg_rw_log(false);
		else
			gps_dl_hw_print_dma_status_struct(ch, &dma_status);

		if (GDL_HW_GET_GPS_DMA_START_STR_STATUS(dma_status.start_flag)) {
			if (gps_dl_only_show_wait_done_log()) {
				gps_dl_set_show_reg_rw_log(last_rw_log_on);
				gps_dl_hw_print_dma_status_struct(ch, &dma_status);
				gps_dl_hw_save_dma_status_struct(ch, &dma_status);
			}
			break; /* to next while-loop */
		} else if (return_if_not_start) {
			ret = GDL_OKAY;
			do_stop = false;
			goto _end;
		}

		tick1 = gps_dl_tick_get();
		if (timeout_usec > GPS_DL_RW_NO_TIMEOUT && (
			gps_dl_tick_delta_to_usec(tick0, tick1) >= timeout_usec ||
			loop_cnt * GDL_HW_STATUS_POLL_INTERVAL_USEC >= timeout_usec)) {
			ret = GDL_FAIL_TIMEOUT;
			do_stop = false;
			goto _end;
		}

		gps_dl_wait_us(GDL_HW_STATUS_POLL_INTERVAL_USEC);
		loop_cnt++;
	}

	while (1) {
#if GPS_DL_ON_LINUX
		conninfra_okay = gps_dl_conninfra_is_okay_or_handle_it(NULL, true);
#endif
		if (!conninfra_okay) {
			ret = GDL_FAIL_CONN_NOT_OKAY;
			do_stop = false;
			break;
		}

		gps_dl_hw_save_dma_status_struct(ch, &dma_status);
		gps_dl_hw_save_usrt_status_struct(link_id, &usrt_status);
		if (gps_dl_only_show_wait_done_log())
			last_rw_log_on = gps_dl_set_show_reg_rw_log(false);
		else {
			gps_dl_hw_print_dma_status_struct(ch, &dma_status);
			gps_dl_hw_print_usrt_status_struct(link_id, &usrt_status);
		}

		if (GDL_HW_GET_GPS_DMA_INTSTA_STATUS(dma_status.intr_flag) &&
			GDL_HW_MAY_GET_DMA_STATE_STATUS(dma_status.state)) {
			if (gps_dl_only_show_wait_done_log()) {
				gps_dl_set_show_reg_rw_log(last_rw_log_on);
				gps_dl_hw_print_dma_status_struct(ch, &dma_status);
				gps_dl_hw_save_dma_status_struct(ch, &dma_status);
			}

			/* DMA ready to stop */
			gps_dl_hw_set_dma_stop(ch);
			gps_dl_hw_save_dma_status_struct(ch, &dma_status);
			gps_dl_hw_print_dma_status_struct(ch, &dma_status);
			ret = GDL_OKAY;
			do_stop = true;
			break;
		}

		tick1 = gps_dl_tick_get();
		if (timeout_usec > GPS_DL_RW_NO_TIMEOUT && (
			gps_dl_tick_delta_to_usec(tick0, tick1) >= timeout_usec ||
			loop_cnt * GDL_HW_STATUS_POLL_INTERVAL_USEC >= timeout_usec)) {
			ret = GDL_FAIL_TIMEOUT;
			do_stop = false;
			break;
		}

		gps_dl_wait_us(GDL_HW_STATUS_POLL_INTERVAL_USEC);
		loop_cnt++;
	}

_end:
	tick1 = gps_dl_tick_get();
	GDL_LOGW("ch = %d, d_us = %d, do_stop = %d, ret = %s",
		ch, gps_dl_tick_delta_to_usec(tick0, tick1), do_stop, gdl_ret_to_name(ret));
	return ret;
}

unsigned int gps_dl_hw_get_dma_left_len(enum gps_dl_hal_dma_ch_index channel)
{
	/* ASSERT(channel >= 0 && channel <= GPS_DL_DMA_CH_NUM); */
	return gps_dl_hw_dep_get_dma_left_len(channel);
}

void gps_dl_hw_get_link_status(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_link_status_struct *p)
{
	unsigned int reg_val;
	unsigned int offset =
		(GPS_L5_USRT_APB_APB_CTRL_ADDR - GPS_USRT_APB_APB_CTRL_ADDR) * link_id;

	/* todo: link_id error handling */

	if (link_id == GPS_DATA_LINK_ID0) {
		p->tx_dma_done = gps_dl_hw_get_dma_int_status(GPS_DL_DMA_LINK0_A2D);
		p->rx_dma_done = gps_dl_hw_get_dma_int_status(GPS_DL_DMA_LINK0_D2A);
	} else if (link_id == GPS_DATA_LINK_ID1) {
		p->tx_dma_done = gps_dl_hw_get_dma_int_status(GPS_DL_DMA_LINK1_A2D);
		p->rx_dma_done = gps_dl_hw_get_dma_int_status(GPS_DL_DMA_LINK1_D2A);
	} else {
		p->tx_dma_done = false;
		p->rx_dma_done = false;
	}

	reg_val = GDL_HW_RD_GPS_REG(GPS_USRT_APB_APB_STA_ADDR + offset);
	p->usrt_has_data = GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_IND, reg_val);
	p->usrt_has_nodata = GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_NODAINTB, reg_val);
}

void gps_dl_hw_save_usrt_status_struct(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_usrt_status_struct *p)
{
	unsigned int offset =
		(GPS_L5_USRT_APB_APB_CTRL_ADDR - GPS_USRT_APB_APB_CTRL_ADDR) * link_id;

	p->ctrl_setting = GDL_HW_RD_GPS_REG(GPS_USRT_APB_APB_CTRL_ADDR + offset);
	p->intr_enable = GDL_HW_RD_GPS_REG(GPS_USRT_APB_APB_INTEN_ADDR + offset);
	p->state = GDL_HW_RD_GPS_REG(GPS_USRT_APB_APB_STA_ADDR + offset);

	p->mcub_a2d_flag = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCUB_A2DF_ADDR + offset);
	p->mcub_d2a_flag = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCUB_D2AF_ADDR + offset);

	p->mcub_a2d_d0 = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCU_A2D0_ADDR + offset);
	p->mcub_a2d_d1 = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCU_A2D1_ADDR + offset);
	p->mcub_d2a_d0 = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCU_D2A0_ADDR + offset);
	p->mcub_d2a_d1 = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MCU_D2A1_ADDR + offset);
	p->monf = GDL_HW_RD_GPS_REG(GPS_USRT_APB_MONF_ADDR + offset);
}

void gps_dl_hw_print_usrt_status_struct(
	enum gps_dl_link_id_enum link_id, struct gps_dl_hw_usrt_status_struct *p)
{
	if (!gps_dl_show_reg_wait_log())
		return;

	GDL_LOGXW(link_id, "usrt ctrl = 0x%08x[DMA_EN RX=%d,TX=%d; 1BYTE=%d], intr_en = 0x%08x, monf = 0x%08x",
		p->ctrl_setting,
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_CTRL_RX_EN, p->ctrl_setting),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_CTRL_TX_EN, p->ctrl_setting),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_CTRL_BYTEN, p->ctrl_setting),
		p->intr_enable, p->monf);

	GDL_LOGXW(link_id, "usrt state = 0x%08x, [UOEFS]RX=%d%d%d%d(%d),TX=%d%d%d%d(%d)",
		p->state,
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_RX_UNDR, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_RX_OVF, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_RX_EMP, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_RX_FULL, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_RX_ST, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_UNDR, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_OVF, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_EMP, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_FULL, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_ST, p->state));

	GDL_LOGXW(link_id, "usrt TxReg_em=%d, TX_IND=%d, TX_IND=%d, U_em=%d, NOD_INT=%d",
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_REGE, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TX_IND, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_TXINTB, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_URAME, p->state),
		GDL_HW_EXTRACT_ENTRY(GPS_USRT_APB_APB_STA_NODAINTB, p->state));

	GDL_LOGXW(link_id, "mcub d2a flag=0x%08x, d0=0x%08x, d1=0x%08x",
		p->mcub_d2a_flag, p->mcub_d2a_d0, p->mcub_d2a_d1);

	GDL_LOGXW(link_id, "mcub a2d flag=0x%08x, d0=0x%08x, d1=0x%08x",
		p->mcub_a2d_flag, p->mcub_a2d_d0, p->mcub_a2d_d1);
}

void gps_dl_hw_switch_dsp_jtag(void)
{
	unsigned int value, value_new;

	value = GDL_HW_RD_GPS_REG(0x80073160);
	value_new = value & 0xFFFFFFFE;
	value_new = value_new | ((~(value & 0x00000001)) & 0x00000001);
	GDL_HW_WR_GPS_REG(0x80073160, value_new);
}

enum GDL_HW_RET gps_dl_hw_get_mcub_info(enum gps_dl_link_id_enum link_id, struct gps_dl_hal_mcub_info *p)
{
	struct arm_smccc_res res;
	int ret;

	if (p == NULL)
		return E_INV_PARAMS;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GET_MCUB_INFO_OPID,
			link_id, 0, 0, 0, 0, 0, &res);
	ret = (enum GDL_HW_RET)res.a0;

	if (ret == HW_OKAY) {
		p->flag = res.a1;
		p->dat0 = res.a2;
		p->dat1 = res.a3;
	}

	return ret;
}

void gps_dl_hw_clear_mcub_d2a_flag(enum gps_dl_link_id_enum link_id, unsigned int d2a_flag)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_CLEAR_MCUB_D2A_FLAG_OPID,
			link_id, d2a_flag, 0, 0, 0, 0, &res);
	ret = res.a0;
}

unsigned int gps_dl_hw_get_mcub_a2d_flag(enum gps_dl_link_id_enum link_id)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_GET_MCUB_A2D_FLAG_OPID,
			link_id, 0, 0, 0, 0, 0, &res);
	ret = res.a0;
	return ret;
}

enum GDL_RET_STATUS gps_dl_hw_mcub_dsp_read_request(enum gps_dl_link_id_enum link_id, u16 dsp_addr)
{
	struct arm_smccc_res res;
	int ret;

	arm_smccc_smc(MTK_SIP_KERNEL_GPS_CONTROL, SMC_GPS_DL_HW_MCUB_DSP_READ_REQUEST_OPID,
			link_id, dsp_addr, 0, 0, 0, 0, &res);
	ret = (enum GDL_RET_STATUS)res.a0;
	return ret;
}

struct gps_dl_hw_pair_time_struct pair_time;
void gps_dl_hw_print_ms_counter_status(void)
{
	unsigned int record_dsleep_ctl, record_wakeup_ctl, tcxo_low, tcxo_high;

	record_dsleep_ctl = gps_dl_bus_rd_opt(GPS_DL_GPS_BUS, GPS_AON_TOP_DSLEEP_CTL_ADDR, BMASK_RW_NONEED_PRINT);
	record_wakeup_ctl = gps_dl_bus_rd_opt(GPS_DL_GPS_BUS, GPS_AON_TOP_WAKEUP_CTL_ADDR, BMASK_RW_NONEED_PRINT);
	gps_dl_hw_get_dsp_ms_counter(&tcxo_low, &tcxo_high);

	GDL_LOGI("DSLEEP = 0x%08x, WAKEUP = 0x%08x, TCXO_MS_L = 0x%x, TCXO_MS_H = 0x%x, pair: %x/%lu",
		record_dsleep_ctl, record_wakeup_ctl, tcxo_low, tcxo_high, pair_time.dsp_ms, pair_time.kernel_ms);
}

struct gps_dl_hw_pair_time_struct *gps_dl_hw_get_dsp_ms_counter(
	unsigned int *p_tcxo_low, unsigned int *p_tcxo_high)
{
	*p_tcxo_low = gps_dl_bus_rd_opt(GPS_DL_GPS_BUS, GPS_AON_TOP_TCXO_MS_H_ADDR, BMASK_RW_NONEED_PRINT);
	*p_tcxo_high = gps_dl_bus_rd_opt(GPS_DL_GPS_BUS, GPS_AON_TOP_TCXO_MS_L_ADDR, BMASK_RW_NONEED_PRINT);

	if ((*p_tcxo_low >= 0x10000) || (*p_tcxo_high >= 0x10000))
		GDL_LOGE("abnormal, p_tcxo_low = %x, p_tcxo_high = %x", *p_tcxo_low, *p_tcxo_high);

	pair_time.kernel_ms = gps_dl_tick_get_ms();
	pair_time.dsp_ms = (*p_tcxo_low & 0XFFFF) | ((*p_tcxo_high & 0XFFFF) << 16);

	return &pair_time;
}

