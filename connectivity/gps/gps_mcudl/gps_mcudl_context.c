/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2012 MediaTek Inc.
 */
#include "gps_mcudl_context.h"
#include "gps_dl_context.h"

static struct gps_mcudl_ctx s_gps_mcudl_ctx = {
	.links = { /* gps_mcudl_each_link */
		[GPS_MDLX_MCUSYS] = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MCUFN]  = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MNL]    = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE * 10} },
		[GPS_MDLX_AGENT]  = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE * 10} },
		[GPS_MDLX_NMEA]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE * 10} },
		[GPS_MDLX_GDLOG]  = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE * 10} },
		[GPS_MDLX_GDLOG2] = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE * 10} },
		[GPS_MDLX_PMTK]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MEAS]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_CORR]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_DSP0]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_DSP1]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_AOL_TEST] = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MPE_TEST] = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_SCP_TEST] = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_LPPM]     = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MPELOG]   = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
		[GPS_MDLX_MPELOG2]  = {
			.cfg = {.tx_buf_size = GPS_DL_TX_BUF_SIZE, .rx_buf_size = GPS_DL_RX_BUF_SIZE} },
	},
#if GPS_DL_ON_LINUX
	.devices = { /* gps_mcudl_each_device */
		[GPS_MDLX_MCUSYS] = {
			.cfg = {.dev_name = "gpsmdl-mcusys", .xid = GPS_MDLX_MCUSYS} },
		[GPS_MDLX_MCUFN]  = {
			.cfg = {.dev_name = "gpsmdl-mcufn",  .xid = GPS_MDLX_MCUFN} },
		[GPS_MDLX_MNL]    = {
			.cfg = {.dev_name = "gpsmdl-mnl",    .xid = GPS_MDLX_MNL} },
		[GPS_MDLX_AGENT]  = {
			.cfg = {.dev_name = "gpsmdl-agent",  .xid = GPS_MDLX_AGENT} },
		[GPS_MDLX_NMEA]   = {
			.cfg = {.dev_name = "gpsmdl-nmea",   .xid = GPS_MDLX_NMEA} },
		[GPS_MDLX_GDLOG]  = {
			.cfg = {.dev_name = "gpsmdl-gdlog",  .xid = GPS_MDLX_GDLOG} },
		[GPS_MDLX_GDLOG2] = {
			.cfg = {.dev_name = "gpsmdl-gdlog2", .xid = GPS_MDLX_GDLOG2} },
		[GPS_MDLX_PMTK]   = {
			.cfg = {.dev_name = "gpsmdl-pmtk",   .xid = GPS_MDLX_PMTK} },
		[GPS_MDLX_MEAS]   = {
			.cfg = {.dev_name = "gpsmdl-meas",   .xid = GPS_MDLX_MEAS} },
		[GPS_MDLX_CORR]   = {
			.cfg = {.dev_name = "gpsmdl-corr",   .xid = GPS_MDLX_CORR} },
		[GPS_MDLX_DSP0]   = {
			.cfg = {.dev_name = "gpsmdl-dsp0",   .xid = GPS_MDLX_DSP0} },
		[GPS_MDLX_DSP1]   = {
			.cfg = {.dev_name = "gpsmdl-dsp1",   .xid = GPS_MDLX_DSP1} },
		[GPS_MDLX_AOL_TEST] = {
			.cfg = {.dev_name = "gpsmdl-aolts",  .xid = GPS_MDLX_AOL_TEST} },
		[GPS_MDLX_MPE_TEST] = {
			.cfg = {.dev_name = "gpsmdl-mpets",  .xid = GPS_MDLX_MPE_TEST} },
		[GPS_MDLX_SCP_TEST] = {
			.cfg = {.dev_name = "gpsmdl-scpts",  .xid = GPS_MDLX_SCP_TEST} },
		[GPS_MDLX_LPPM]     = {
			.cfg = {.dev_name = "gpsmdl-lppm",   .xid = GPS_MDLX_LPPM} },
		[GPS_MDLX_MPELOG]   = {
			.cfg = {.dev_name = "gpsmdl-mpelog", .xid = GPS_MDLX_MPELOG} },
		[GPS_MDLX_MPELOG2]  = {
			.cfg = {.dev_name = "gpsmdl-mpelog2", .xid = GPS_MDLX_MPELOG2} },
	},
	.nv_devices = { /* gps_nv_each_device */
		[GPS_MCUSYS_NV_DATA_ID_EPO] = {
			.cfg = {.dev_name = "gpsstrg-epo",   .nv_id = GPS_MCUSYS_NV_DATA_ID_EPO} },
		[GPS_MCUSYS_NV_DATA_ID_GG_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-qegg",  .nv_id = GPS_MCUSYS_NV_DATA_ID_GG_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_BD_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-qebd",  .nv_id = GPS_MCUSYS_NV_DATA_ID_BD_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_GA_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-qega",  .nv_id = GPS_MCUSYS_NV_DATA_ID_GA_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_NVFILE] = {
			.cfg = {.dev_name = "gpsstrg-dat",   .nv_id = GPS_MCUSYS_NV_DATA_ID_NVFILE} },
		[GPS_MCUSYS_NV_DATA_ID_CACHE] = {
			.cfg = {.dev_name = "gpsstrg-cache", .nv_id = GPS_MCUSYS_NV_DATA_ID_CACHE} },
		[GPS_MCUSYS_NV_DATA_ID_CONFIG] = {
			.cfg = {.dev_name = "gpsstrg-cfg",   .nv_id = GPS_MCUSYS_NV_DATA_ID_CONFIG} },
		[GPS_MCUSYS_NV_DATA_ID_CONFIG_WRITE] = {
			.cfg = {.dev_name = "gpsstrg-cfgw",  .nv_id = GPS_MCUSYS_NV_DATA_ID_CONFIG_WRITE} },
		[GPS_MCUSYS_NV_DATA_ID_DSPL1] = {
			.cfg = {.dev_name = "gpsstrg-dsp0",  .nv_id = GPS_MCUSYS_NV_DATA_ID_DSPL1} },
		[GPS_MCUSYS_NV_DATA_ID_DSPL5] = {
			.cfg = {.dev_name = "gpsstrg-dsp1",  .nv_id = GPS_MCUSYS_NV_DATA_ID_DSPL5} },
		[GPS_MCUSYS_NV_DATA_ID_XEPO] = {
			.cfg = {.dev_name = "gpsstrg-xepo",  .nv_id = GPS_MCUSYS_NV_DATA_ID_XEPO} },
		[GPS_MCUSYS_NV_DATA_ID_QZ_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-qzepo",  .nv_id = GPS_MCUSYS_NV_DATA_ID_QZ_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_IR_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-irepo",  .nv_id = GPS_MCUSYS_NV_DATA_ID_IR_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_KR_QEPO] = {
			.cfg = {.dev_name = "gpsstrg-krepo",  .nv_id = GPS_MCUSYS_NV_DATA_ID_KR_QEPO} },
		[GPS_MCUSYS_NV_DATA_ID_DSPL1_CW] = {
			.cfg = {.dev_name = "gpsstrg-dsp0cw",  .nv_id = GPS_MCUSYS_NV_DATA_ID_DSPL1_CW} },
		[GPS_MCUSYS_NV_DATA_ID_DSPL5_CW] = {
			.cfg = {.dev_name = "gpsstrg-dsp1cw",  .nv_id = GPS_MCUSYS_NV_DATA_ID_DSPL5_CW} },
		[GPS_MCUSYS_NV_DATA_ID_MPENV] = {
			.cfg = {.dev_name = "gpsstrg-mpedat",  .nv_id = GPS_MCUSYS_NV_DATA_ID_MPENV} },
		[GPS_MCUSYS_NV_DATA_ID_MPE_CFG] = {
			.cfg = {.dev_name = "gpsstrg-mpecfg",  .nv_id = GPS_MCUSYS_NV_DATA_ID_MPE_CFG} },
		[GPS_MCUSYS_NV_DATA_ID_AP_MPE] = {
			.cfg = {.dev_name = "gpsstrg-apmpe",  .nv_id = GPS_MCUSYS_NV_DATA_ID_AP_MPE} },
		[GPS_MCUSYS_NV_DATA_ID_RAW_MEAS] = {
			.cfg = {.dev_name = "gpsstrg-rawmeas",  .nv_id = GPS_MCUSYS_NV_DATA_ID_RAW_MEAS} },
		[GPS_MCUSYS_NV_DATA_ID_RAW_HIGEO] = {
			.cfg = {.dev_name = "gpsstrg-higeo",  .nv_id = GPS_MCUSYS_NV_DATA_ID_RAW_HIGEO} },
	},
#endif /* GPS_DL_ON_LINUX */
};

#if 0
static struct gps_dl_runtime_cfg s_gps_rt_cfg = {
	.dma_is_1byte_mode = true,
	.dma_is_enabled = true,
	.show_reg_rw_log = false,
	.show_reg_wait_log = true,
	.only_show_wait_done_log = true,
	.log_level = GPS_DL_LOG_DEF_SETTING_LEVEL,
	.log_mod_bitmask = GPS_DL_LOG_DEF_SETTING_MODULES,
	.log_reg_rw_bitmask = GPS_DL_LOG_REG_RW_BITMASK,
};
#endif

struct gps_mcudl_each_link *gps_mcudl_link_get(enum gps_mcudl_xid x_id)
{
	if (x_id >= 0 && x_id < GPS_MDLX_CH_NUM)
		return &s_gps_mcudl_ctx.links[x_id];

	return NULL;
}

#if 0
struct gps_each_irq *gps_dl_irq_get(enum gps_dl_irq_index_enum irq_idx)
{
	if (irq_idx >= 0 && irq_idx < GPS_DL_IRQ_NUM)
		return &s_gps_mcudl_ctx.irqs[irq_idx];

	return NULL;
}
#endif

#if GPS_DL_ON_LINUX
struct gps_mcudl_each_device *gps_mcudl_device_get(enum gps_mcudl_xid x_id)
{
	if (x_id >= 0 && x_id < GPS_MDLX_CH_NUM)
		return &s_gps_mcudl_ctx.devices[x_id];

	return NULL;
}

struct gps_nv_each_device *gps_nv_device_get(enum gps_mcusys_nv_data_id nv_id)
{
	if ((unsigned int)nv_id >= (unsigned int)GPS_MCUSYS_NV_DATA_NUM)
		return NULL;

	return &s_gps_mcudl_ctx.nv_devices[nv_id];
}

struct gps_each_link_waitable *gps_nv_each_link_get_read_waitable_ptr(enum gps_mcusys_nv_data_id nv_id)
{
	struct gps_nv_each_device *p_dev;

	if ((unsigned int)nv_id >= (unsigned int)GPS_MCUSYS_NV_DATA_NUM)
		return NULL;

	p_dev = &s_gps_mcudl_ctx.nv_devices[nv_id];
	return &p_dev->wait_read;
}
#endif

#if 0
struct gps_dl_remap_ctx *gps_dl_remap_ctx_get(void)
{
	return &s_gps_mcudl_ctx.remap_ctx;
}

bool gps_dl_is_1byte_mode(void)
{
	return s_gps_rt_cfg.dma_is_1byte_mode;
}

bool gps_dl_set_1byte_mode(bool is_1byte_mode)
{
	bool old = s_gps_rt_cfg.dma_is_1byte_mode;

	s_gps_rt_cfg.dma_is_1byte_mode = is_1byte_mode;
	return old;
}

bool gps_dl_is_dma_enabled(void)
{
	return s_gps_rt_cfg.dma_is_enabled;
}

bool gps_dl_set_dma_enabled(bool to_enable)
{
	bool old = s_gps_rt_cfg.dma_is_enabled;

	s_gps_rt_cfg.dma_is_enabled = to_enable;
	return old;
}

bool gps_dl_show_reg_rw_log(void)
{
	return s_gps_rt_cfg.show_reg_rw_log;
}

bool gps_dl_show_reg_wait_log(void)
{
	return s_gps_rt_cfg.show_reg_wait_log;
}

bool gps_dl_only_show_wait_done_log(void)
{
	return s_gps_rt_cfg.only_show_wait_done_log;
}

bool gps_dl_set_show_reg_rw_log(bool on)
{
	bool last_on = s_gps_rt_cfg.show_reg_rw_log;

	s_gps_rt_cfg.show_reg_rw_log = on;
	return last_on;
}

void gps_dl_set_show_reg_wait_log(bool on)
{
	s_gps_rt_cfg.show_reg_wait_log = on;
}

int gps_dl_set_rx_transfer_max(enum gps_mcudl_xid link_id, int max)
{
	struct gps_mcudl_each_link *p_link;
	int old_max = 0;

	p_link = gps_mcudl_link_get(link_id);

	if (p_link) {
		old_max = p_link->rx_dma_buf.transfer_max;
		p_link->rx_dma_buf.transfer_max = max;
		GDL_LOGXD(link_id, "old_max = %d, new_max = %d", old_max, max);
	}

	return old_max;
}

int gps_dl_set_tx_transfer_max(enum gps_mcudl_xid link_id, int max)
{
	struct gps_mcudl_each_link *p_link;
	int old_max = 0;

	p_link = gps_mcudl_link_get(link_id);

	if (p_link) {
		old_max = p_link->tx_dma_buf.transfer_max;
		p_link->tx_dma_buf.transfer_max = max;
		GDL_LOGXD(link_id, "old_max = %d, new_max = %d", old_max, max);
	}

	return old_max;
}

enum gps_dl_log_level_enum gps_dl_log_level_get(void)
{
	return s_gps_rt_cfg.log_level;
}

void gps_dl_log_level_set(enum gps_dl_log_level_enum level)
{
	s_gps_rt_cfg.log_level = level;
}

bool gps_dl_log_mod_is_on(enum gps_dl_log_module_enum mod)
{
	return (bool)(s_gps_rt_cfg.log_mod_bitmask & (1UL << mod));
}

void gps_dl_log_mod_on(enum gps_dl_log_module_enum mod)
{
	s_gps_rt_cfg.log_mod_bitmask |= (1UL << mod);
}

void gps_dl_log_mod_off(enum gps_dl_log_module_enum mod)
{
	s_gps_rt_cfg.log_mod_bitmask &= ~(1UL << mod);
}

void gps_dl_log_mod_bitmask_set(unsigned int bitmask)
{
	s_gps_rt_cfg.log_mod_bitmask = bitmask;
}

unsigned int gps_dl_log_mod_bitmask_get(void)
{
	return s_gps_rt_cfg.log_mod_bitmask;
}

bool gps_dl_log_reg_rw_is_on(enum gps_dl_log_reg_rw_ctrl_enum log_reg_rw)
{
	return (bool)(s_gps_rt_cfg.log_reg_rw_bitmask & (1UL << log_reg_rw));
}

void gps_dl_log_info_show(void)
{
	bool show_reg_rw_log = gps_dl_show_reg_rw_log();

	GDL_LOGE("level = %d, bitmask = 0x%08x, rrw = %d",
		s_gps_rt_cfg.log_level, s_gps_rt_cfg.log_mod_bitmask, show_reg_rw_log);
}
#endif

