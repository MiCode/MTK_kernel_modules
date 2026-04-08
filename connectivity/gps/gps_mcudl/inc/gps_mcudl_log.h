/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_LOG_H
#define _GPS_MCUDL_LOG_H

#include "gps_dl_config.h"
#include "gps_dl_log.h"
#include "gps_mcudl_xlink.h"
#if GPS_DL_ON_LINUX
#include <linux/printk.h>
#define __MDL_LOGE(mod, fmt, ...) pr_notice("GDL[E:%d] [%s:%d]: "fmt, \
	mod, __func__, __LINE__, ##__VA_ARGS__)
#define __MDL_LOGW(mod, fmt, ...) pr_notice("GDL[W:%d] [%s:%d]: "fmt, \
	mod, __func__, __LINE__, ##__VA_ARGS__)
#define __MDL_LOGI(mod, fmt, ...) pr_info("GDL[I:%d] [%s:%d]: "fmt, \
	mod, __func__, __LINE__, ##__VA_ARGS__)
#define __MDL_LOGD(mod, fmt, ...) pr_info("GDL[D:%d] [%s:%d]: "fmt, \
	mod, __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGXE(mod, x_id, fmt, ...) pr_notice("GDLX-%d[E:%d,%s] [%s:%d]: "fmt, \
	x_id, mod, gpsmdl_xid_name(x_id), __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGXW(mod, x_id, fmt, ...) pr_notice("GDLX-%d[W:%d,%s] [%s:%d]: "fmt, \
	x_id, mod, gpsmdl_xid_name(x_id), __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGXI(mod, x_id, fmt, ...)   pr_info("GDLX-%d[I:%d,%s] [%s:%d]: "fmt, \
	x_id, mod, gpsmdl_xid_name(x_id), __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGXD(mod, x_id, fmt, ...)   pr_info("GDLX-%d[D:%d,%s] [%s:%d]: "fmt, \
	x_id, mod, gpsmdl_xid_name(x_id), __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGYE(mod, link_id, fmt, ...) pr_notice("GDLY-%d[E:%d] [%s:%d]: "fmt, \
	link_id, mod, __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGYW(mod, link_id, fmt, ...) pr_notice("GDLY-%d[W:%d] [%s:%d]: "fmt, \
	link_id, mod, __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGYI(mod, link_id, fmt, ...) pr_info("GDLY-%d[I:%d] [%s:%d]: "fmt, \
	link_id, mod, __func__, __LINE__, ##__VA_ARGS__)

#define __MDL_LOGYD(mod, link_id, fmt, ...) pr_info("GDLY-%d[D:%d] [%s:%d]: "fmt, \
	link_id, mod, __func__, __LINE__, ##__VA_ARGS__)
#elif GPS_DL_ON_CTP
#include "gps_dl_ctp_log.h"
#endif /* GPS_DL_ON_XX */


#define _MDL_LOGE(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_ERR) __MDL_LOGE(__VA_ARGS__); } while (0)
#define _MDL_LOGW(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_WARN) __MDL_LOGW(__VA_ARGS__); } while (0)
#define _MDL_LOGI(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_INFO) __MDL_LOGI(__VA_ARGS__); } while (0)
#define _MDL_LOGD(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_DBG) __MDL_LOGD(__VA_ARGS__); } while (0)

#define _MDL_LOGXE(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_ERR) __MDL_LOGXE(__VA_ARGS__); } while (0)
#define _MDL_LOGXW(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_WARN) __MDL_LOGXW(__VA_ARGS__); } while (0)
#define _MDL_LOGXI(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_INFO) __MDL_LOGXI(__VA_ARGS__); } while (0)
#define _MDL_LOGXD(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_DBG) __MDL_LOGXD(__VA_ARGS__); } while (0)

#define _MDL_LOGYE(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_ERR) __MDL_LOGYE(__VA_ARGS__); } while (0)
#define _MDL_LOGYW(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_WARN) __MDL_LOGYW(__VA_ARGS__); } while (0)
#define _MDL_LOGYI(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_INFO) __MDL_LOGYI(__VA_ARGS__); } while (0)
#define _MDL_LOGYD(...) \
	do { if (gps_dl_log_level_get() <= GPS_DL_LOG_LEVEL_DBG) __MDL_LOGYD(__VA_ARGS__); } while (0)


#define MDL_LOGE2(mod, ...) \
	do { if (1) \
		_MDL_LOGE(mod, __VA_ARGS__); } while (0)
#define MDL_LOGW2(mod, ...) \
	do { if (1) \
		_MDL_LOGW(mod, __VA_ARGS__); } while (0)
#define MDL_LOGI2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGI(mod, __VA_ARGS__); } while (0)
#define MDL_LOGD2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGD(mod, __VA_ARGS__); } while (0)


#define MDL_LOGE(...) MDL_LOGE2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGW(...) MDL_LOGW2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGI(...) MDL_LOGI2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGD(...) MDL_LOGD2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)

#define MDL_LOGD_ONF(...) MDL_LOGD2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)

#define MDL_LOGI_DRW(...) MDL_LOGI2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)

#define MDL_LOGW_RRW(...) MDL_LOGI2(GPS_DL_LOG_MOD_REG_RW, __VA_ARGS__)
#define MDL_LOGI_RRW(...) MDL_LOGI2(GPS_DL_LOG_MOD_REG_RW, __VA_ARGS__)

#define MDL_LOGE_EVT(...) MDL_LOGE2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGW_EVT(...) MDL_LOGW2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGD_EVT(...) MDL_LOGD2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)

#define MDL_LOGE_INI(...) MDL_LOGE2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGW_INI(...) MDL_LOGW2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGI_INI(...) MDL_LOGI2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGD_INI(...) MDL_LOGD2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)

#define MDL_LOGXE2(mod, ...) \
	do { if (1) \
		_MDL_LOGXE(mod, __VA_ARGS__); } while (0)
#define MDL_LOGXW2(mod, ...) \
	do { if (1) \
		_MDL_LOGXW(mod, __VA_ARGS__); } while (0)
#define MDL_LOGXI2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGXI(mod, __VA_ARGS__); } while (0)
#define MDL_LOGXD2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGXD(mod, __VA_ARGS__); } while (0)

#define MDL_LOGXE(...) MDL_LOGXE2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGXW(...) MDL_LOGXW2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGXI(...) MDL_LOGXI2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGXD(...) MDL_LOGXD2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)

#define MDL_LOGXE_ONF(...) MDL_LOGXE2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGXW_ONF(...) MDL_LOGXW2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGXI_ONF(...) MDL_LOGXI2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGXD_ONF(...) MDL_LOGXD2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)

#define MDL_LOGXE_DRW(...) MDL_LOGXE2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGXW_DRW(...) MDL_LOGXW2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGXI_DRW(...) MDL_LOGXI2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGXD_DRW(...) MDL_LOGXD2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)

#define MDL_LOGXE_STA(...) MDL_LOGXE2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)
#define MDL_LOGXW_STA(...) MDL_LOGXW2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)
#define MDL_LOGXI_STA(...) MDL_LOGXI2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)
#define MDL_LOGXD_STA(...) MDL_LOGXD2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)

#define MDL_LOGXW_EVT(...) MDL_LOGXW2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGXI_EVT(...) MDL_LOGXI2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGXD_EVT(...) MDL_LOGXD2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)

#define MDL_LOGXE_INI(...) MDL_LOGXE2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGXI_INI(...) MDL_LOGXI2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGXD_INI(...) MDL_LOGXD2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)


#define MDL_LOGYE2(mod, ...) \
	do { if (1) \
		_MDL_LOGYE(mod, __VA_ARGS__); } while (0)
#define MDL_LOGYW2(mod, ...) \
	do { if (1) \
		_MDL_LOGYW(mod, __VA_ARGS__); } while (0)
#define MDL_LOGYI2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGYI(mod, __VA_ARGS__); } while (0)
#define MDL_LOGYD2(mod, ...) \
	do { if (gps_dl_log_mod_is_on(mod)) \
		_MDL_LOGYD(mod, __VA_ARGS__); } while (0)

#define MDL_LOGYE(...) MDL_LOGYE2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGYW(...) MDL_LOGYW2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGYI(...) MDL_LOGYI2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)
#define MDL_LOGYD(...) MDL_LOGYD2(GPS_DL_LOG_MOD_DEFAULT, __VA_ARGS__)

#define MDL_LOGYE_ONF(...) MDL_LOGYE2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGYW_ONF(...) MDL_LOGYW2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGYI_ONF(...) MDL_LOGYI2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)
#define MDL_LOGYD_ONF(...) MDL_LOGYD2(GPS_DL_LOG_MOD_OPEN_CLOSE, __VA_ARGS__)

#define MDL_LOGYE_DRW(...) MDL_LOGYE2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGYW_DRW(...) MDL_LOGYW2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGYI_DRW(...) MDL_LOGYI2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)
#define MDL_LOGYD_DRW(...) MDL_LOGYD2(GPS_DL_LOG_MOD_READ_WRITE, __VA_ARGS__)

#define MDL_LOGYE_STA(...) MDL_LOGYE2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)
#define MDL_LOGYW_STA(...) MDL_LOGYW2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)
#define MDL_LOGYI_STA(...) MDL_LOGYI2(GPS_DL_LOG_MOD_STATUS, __VA_ARGS__)

#define MDL_LOGYW_EVT(...) MDL_LOGYW2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGYI_EVT(...) MDL_LOGYI2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)
#define MDL_LOGYD_EVT(...) MDL_LOGYD2(GPS_DL_LOG_MOD_EVENT, __VA_ARGS__)

#define MDL_LOGYE_INI(...) MDL_LOGYE2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGYI_INI(...) MDL_LOGYI2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)
#define MDL_LOGYD_INI(...) MDL_LOGYD2(GPS_DL_LOG_MOD_INIT, __VA_ARGS__)


#endif /* _GPS_MCUDL_LOG_H */

