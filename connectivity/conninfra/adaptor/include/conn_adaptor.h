/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#ifndef _CONN_ADAPTOR_H_
#define _CONN_ADAPTOR_H_

#include <linux/types.h>
#include <linux/mm.h>
#include "conn_drv.h"

enum conn_adaptor_drv_gen {
	CONN_ADAPTOR_DRV_GEN_CONNAC_2,
	CONN_ADAPTOR_DRV_GEN_CONNAC_3,
	CONN_ADAPTOR_DRV_GEN_SIZE
};

#define CONN_SUPPOPRT_DRV_WIFI_TYPE_BIT	(0x1 << CONN_ADAPTOR_DRV_WIFI)
#define CONN_SUPPOPRT_DRV_BT_TYPE_BIT	(0x1 << CONN_ADAPTOR_DRV_BT)
#define CONN_SUPPOPRT_DRV_GPS_TYPE_BIT	(0x1 << CONN_ADAPTOR_DRV_GPS)
#define CONN_SUPPOPRT_DRV_FM_TYPE_BIT	(0x1 << CONN_ADAPTOR_DRV_FM)

struct conn_adaptor_drv_gen_cb {
	u32 drv_radio_support;

	u32 (*get_chip_id)(void);
	u32 (*get_adie_id)(u32 drv_type);

	/* suspend/resume */
	void (*plat_suspend_notify)(void);
	void (*plat_resume_notify) (void);

	/* on/off */
	void (*power_on_off_notify)(int on_off);

	/* coredump */
	void (*set_coredump_mode) (int mode);

	/* read emi for coredump */
	ssize_t (*coredump_emi_read)(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);

	/* dbg read/write */
	int (*dump_power_state)(uint8_t *buf, u32 buf_sz);

	/* get_chip_info */
	int (*get_chip_info)(uint8_t *buf, u32 buf_sz);

	/* factory_testcase */
	int (*factory_testcase)(uint8_t *buf, u32 buf_sz);
};

int conn_adaptor_register_drv_gen(enum conn_adaptor_drv_gen drv_gen, struct conn_adaptor_drv_gen_cb* cb);
int conn_adaptor_unregister_drv_gen(enum conn_adaptor_drv_gen drv_gen);
/* Distinguish internal project or customer project */
extern bool conn_adaptor_is_internal(void);

#endif /* _CONN_ADAPTOR_H_ */
