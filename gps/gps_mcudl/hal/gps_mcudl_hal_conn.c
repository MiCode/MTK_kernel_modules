/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_dl_log.h"
#include "gps_mcudl_log.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcudl_ylink.h"
#include "gps_dl_hw_dep_api.h"


void gps_mcudl_hal_get_ecid_info(void)
{
	gps_dl_hw_dep_gps_control_adie_on();
	gps_dl_hw_dep_gps_get_ecid_info();
	gps_dl_hw_dep_gps_control_adie_off();
}

bool gps_mcudl_hal_dump_power_state(void)
{
	bool is_gps_awake = true;
	bool is_sw_clk_ext = false;
	struct gps_dl_power_raw_state raw;
	unsigned int L1_on_mode = 0, L1_off_mode = 0;
	unsigned int L5_on_mode = 0, L5_off_mode = 0;
	unsigned int flag = 0;
	unsigned int on_off_cnt = 0;
	gpsmdl_u32 xbitmask;

	memset(&raw, 0, sizeof(raw));
	gps_dl_hw_dep_gps_dump_power_state(&raw);
	xbitmask = gps_mcudl_ylink_get_xbitmask(GPS_MDLY_NORMAL);

	/* calculate and print readable log */

	if (raw.sw_gps_ctrl == 0x0000) {
		/* sw_gps_ctrl not support or gps dsp has not ever turned on */
		is_sw_clk_ext = false;
	} else if (raw.sw_gps_ctrl == 0xFFFF) {
		/* gps dsp is working */
		is_sw_clk_ext = false;
	} else {
		/* gps dsp is turned off */
		L1_on_mode  = ((raw.sw_gps_ctrl & 0xC000) >> 14);
		L5_on_mode  = ((raw.sw_gps_ctrl & 0x3000) >> 12);
		L1_off_mode = ((raw.sw_gps_ctrl & 0x0C00) >> 10);
		L5_off_mode = ((raw.sw_gps_ctrl & 0x0300) >>  8);
		flag        = ((raw.sw_gps_ctrl & 0x0080) >>  7);
		on_off_cnt  = ((raw.sw_gps_ctrl & 0x007F) >>  0);
		if (L1_off_mode != 0 || L5_off_mode != 0)
			is_sw_clk_ext = flag; /* dsp is in deep stop mode or clk_ext */
		else
			is_sw_clk_ext = false; /* dsp is off */
	}

	is_gps_awake = is_sw_clk_ext || raw.is_hw_clk_ext || (raw.mcu_pc != 0);

	MDL_LOGI(
		"awake=%d,mcu_pc=0x%08x,clk_ext=%d,%d,sw_ctrl=0x%04X[on=%u,%u,off=%u,%u,flag=%u,cnt=%u],xbitmask=0x%08x",
		is_gps_awake, raw.mcu_pc, raw.is_hw_clk_ext, is_sw_clk_ext, raw.sw_gps_ctrl,
		L1_on_mode, L5_on_mode, L1_off_mode, L5_off_mode, flag, on_off_cnt, xbitmask);
	return is_gps_awake;
}
