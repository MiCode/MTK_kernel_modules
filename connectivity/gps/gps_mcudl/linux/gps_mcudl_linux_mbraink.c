/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 MediaTek Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>

#include "gps_dl_config.h"
#include "gps_mcudl_config.h"
#include "gps_mcudl_linux_mbraink.h"
#include "gps_mcudl_hal_stat.h"


static struct mbraink2gps_ops gps2mbraink_ops = {
	.get_lp_data = gps_mcudl_mbraink2gps_get_lp_data,
	.get_mcu_data = gps_mcudl_mbraink2gps_get_mcu_data,
};

void gps_mcudl_linux_register_cbs_to_mbraink(void)
{
	register_gps2mbraink_ops(&gps2mbraink_ops);
	GDL_LOGI("ok");
}

void gps_mcudl_linux_unregister_cbs_to_mbraink(void)
{
	unregister_gps2mbraink_ops();
	GDL_LOGI("ok");
}

enum gps_mcudl_stat_get_reason gps_mcudl_convert_resaon_from_mbrain(enum mbr2gnss_reason reason)
{
	switch (reason) {
	case MBR2GNSS_TEST:
		return GPS_MCUDL_STAT_GET_FOR_TEST;
	case MBR2GNSS_PERIODIC:
		return GPS_MCUDL_STAT_GET_MBRAIN_PERIODIC;
	case MBR2GNSS_APP_SWITCH:
		return GPS_MCUDL_STAT_GET_MBRAIN_APP_SWITCH;
	case MBR2GNSS_AP_RESUME:
		return GPS_MCUDL_STAT_GET_MBRAIN_AP_RESUME;
	default:
		return GPS_MCUDL_STAT_GET_FOR_TEST;
	}
}

enum gnss2mbr_status gps_mcudl_convert_status_to_mbrain(enum gps_mcudl_stat_get_result result)
{
	switch (result) {
	case GPS_MCUDL_STAT_NO_DATA:
		return GNSS2MBR_NO_DATA;
	case GPS_MCUDL_STAT_OK_NO_MORE:
		return GNSS2MBR_OK_NO_MORE;
	case GPS_MCUDL_STAT_OK_MORE:
		return GNSS2MBR_OK_MORE;
	default:
		return GNSS2MBR_NO_DATA;
	}
}

enum gnss2mbr_status gps_mcudl_mbraink2gps_get_lp_data(
	enum mbr2gnss_reason reason, struct gnss2mbr_lp_data *p)
{
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
	struct gps_mcudl_stat_lp_data lp_data;
	enum gps_mcudl_stat_get_result result;
	enum gnss2mbr_status ret;

	result = gps_mcudl_stat_get_lp_data(
		GPS_MCUDL_STAT_GET_BY_MBRAIN,
		gps_mcudl_convert_resaon_from_mbrain(reason), &lp_data);
	ret = gps_mcudl_convert_status_to_mbrain(result);

	if ((ret == GNSS2MBR_OK_NO_MORE) || (ret == GNSS2MBR_OK_MORE)) {
		p->hdr_out.data_size = sizeof(*p);
		p->hdr_out.major_ver = LP_DATA_MAJOR_VER;
		p->hdr_out.minor_ver = LP_DATA_MINOR_VER;

		if (p->hdr_out.data_size <= p->hdr_in.data_size) {
			p->dump_ts       = lp_data.dump_ktime_ms;
			p->dump_index    = lp_data.dump_index;

			p->gnss_mcu_sid     = lp_data.gnss_mcu_sid;
			p->gnss_mcu_is_on   = lp_data.gnss_mcu_is_on;
			p->gnss_pwr_is_hi   = lp_data.gnss_pwr_is_hi;
			p->gnss_pwr_wrn     = lp_data.gnss_pwr_wrn;
			p->gnss_pwr_wrn_cnt = lp_data.gnss_pwr_wrn_cnt;
		} else {
			GDL_LOGW("ver_sz mismatch: in=%u,%u,%u out=%u,%u,%u",
				p->hdr_in.major_ver,
				p->hdr_in.minor_ver,
				p->hdr_in.data_size,
				p->hdr_out.major_ver,
				p->hdr_out.minor_ver,
				p->hdr_out.data_size
			);
		}
	}
	return ret;
#else
	return GNSS2MBR_NO_DATA;
#endif
}

enum gnss2mbr_status gps_mcudl_mbraink2gps_get_mcu_data(
	enum mbr2gnss_reason reason, struct gnss2mbr_mcu_data *p)
{
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
	struct gps_mcudl_stat_mcu_data mcu_data;
	enum gps_mcudl_stat_get_result result;
	enum gnss2mbr_status ret;

	result = gps_mcudl_stat_get_mcu_data(
		GPS_MCUDL_STAT_GET_BY_MBRAIN,
		gps_mcudl_convert_resaon_from_mbrain(reason), &mcu_data);
	ret = gps_mcudl_convert_status_to_mbrain(result);

	if ((ret == GNSS2MBR_OK_NO_MORE) || (ret == GNSS2MBR_OK_MORE)) {
		p->hdr_out.data_size = sizeof(*p);
		p->hdr_out.major_ver = MCU_DATA_MAJOR_VER;
		p->hdr_out.minor_ver = MCU_DATA_MINOR_VER;

		if (p->hdr_out.data_size <= p->hdr_in.data_size) {
			p->gnss_mcu_sid  = mcu_data.gnss_mcu_sid;
			p->clock_cfg_val = mcu_data.clock_cfg_val;

			p->open_ts       = mcu_data.open_ktime_ms;
			p->open_duration = mcu_data.open_duration;
			p->open_duration_max = mcu_data.open_duration_max;

			p->close_ts      = mcu_data.close_ktime_ms;
			p->close_duration = mcu_data.close_duration;
			p->close_duration_max = mcu_data.close_duration_max;

			p->has_exception = mcu_data.has_exception;
			p->force_close   = mcu_data.force_close;
			p->exception_cnt = mcu_data.exception_cnt;
			p->force_close_cnt = mcu_data.force_close_cnt;
		} else {
			GDL_LOGW("ver_sz mismatch: in=%u,%u,%u out=%u,%u,%u",
				p->hdr_in.major_ver,
				p->hdr_in.minor_ver,
				p->hdr_in.data_size,
				p->hdr_out.major_ver,
				p->hdr_out.minor_ver,
				p->hdr_out.data_size
			);
		}
	}
	return ret;
#else
	return GNSS2MBR_NO_DATA;
#endif
}

