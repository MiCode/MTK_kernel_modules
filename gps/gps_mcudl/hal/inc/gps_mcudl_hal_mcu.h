/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _GPS_MCUDL_HAL_MCU_H
#define _GPS_MCUDL_HAL_MCU_H

#include "gps_mcudl_hw_type.h"

enum gps_mcudl_fw_type {
	MCU_FW_ROM,
	MCU_FW_EMI,
	MCUDL_FW_MAX_TYPE
};
struct gps_mcudl_fw_desc {
	enum gps_mcudl_fw_type type;
	const unsigned char *ptr;
	unsigned int len;
};
struct gps_mcudl_fw_list {
	const struct gps_mcudl_fw_desc *p_desc;
	unsigned int n_desc;
};

/* true is okay */
bool gps_mcudl_hal_mcu_do_on(const struct gps_mcudl_fw_list *p_fw_list);
void gps_mcudl_hal_mcu_do_off(void);
bool gps_mcudl_hal_mcu_set_fw_own(void);
bool gps_mcudl_hal_mcu_clr_fw_own(void);
void gps_mcudl_hal_mcu_show_status(void);
void gps_mcudl_hal_mcu_show_pc_log(void);
bool gps_mcudl_hal_bg_is_readable(bool check_conn_off);
void gps_mcudl_hal_vdnr_dump(void);

void gps_mcudl_hal_mcu_load_fw_all(void);
void gps_mcudl_hal_mcu_copy_fw_file(void *p_dst, const void *p_src, unsigned int size);

bool gps_mcudl_xlink_on(const struct gps_mcudl_fw_list *p_fw_list);
bool gps_mcudl_xlink_off(void);


#endif /* _GPS_MCUDL_HAL_MCU_H */

