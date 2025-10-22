/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"
#include "gps_mcudl_hw_mcu.h"
#include "gps_mcudl_hw_ccif.h"
#include "gps_mcudl_hal_conn.h"
#include "gps_mcudl_hal_mcu.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcudl_hw_dep_macro.h"
#include "gps_mcu_hif_api.h"
#include "gps_dl_isr.h"
#include "gps_dl_hal.h"
#include "gps_dl_subsys_reset.h"
#include "gps_mcudl_log.h"
#if GPS_DL_ON_LINUX
#include "gps_dl_linux_reserved_mem_v2.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_dl_iomem_dump.h"
#include <asm/io.h>
#endif

bool gps_mcudl_xlink_on(const struct gps_mcudl_fw_list *p_fw_list)
{
	bool is_okay = true;
	bool ntf_set_fw_own = false;

#if GPS_DL_ON_CTP
	is_okay = gps_dl_hal_conn_infra_driver_on();
	if (!is_okay)
		return false;
#endif
	is_okay = gps_mcudl_hw_conn_force_wake(true);
	if (!is_okay)
		return false;

	gps_mcudl_hal_user_fw_own_init(GMDL_FW_OWN_CTRL_BY_POS);
	is_okay = gps_mcudl_conninfra_is_okay_or_handle_it();
	if (!is_okay)
		return false;

	/* init status is clr_fw_own and force_wake should be true, so bypass it:
	 * (void)gps_mcudl_hw_conn_force_wake(false);
	 */
	/*gps_mcudl_hal_get_ecid_info();*/
	is_okay = gps_mcudl_hal_mcu_do_on(p_fw_list);
	ntf_set_fw_own = gps_mcudl_hal_user_set_fw_own_may_notify(GMDL_FW_OWN_CTRL_BY_POS);
	MDL_LOGI("ok=%d, ntf=%d", is_okay, ntf_set_fw_own);
	return is_okay;
}

bool gps_mcudl_xlink_off(void)
{
	bool is_okay = true;

	(void)gps_mcudl_hal_user_clr_fw_own(GMDL_FW_OWN_CTRL_BY_POS);
	(void)gps_mcudl_hw_conn_force_wake(true);
	(void)gps_mcudl_conninfra_is_okay_or_handle_it();

	gps_mcudl_hal_mcu_do_off();
	gps_mcudl_hal_user_fw_own_deinit(GMDL_FW_OWN_CTRL_BY_POS);
	(void)gps_mcudl_hw_conn_force_wake(false);
#if GPS_DL_ON_CTP
	gps_dl_hal_conn_infra_driver_off();
#endif
	MDL_LOGI("ok=%d", is_okay);
	return is_okay;
}

void gps_mcudl_hal_load_fw(const struct gps_mcudl_fw_list *p_fw_list)
{
	const struct gps_mcudl_fw_desc *p_desc;
	unsigned int i;
	bool is_okay = true;
	void *p_dst = NULL;
	unsigned long phys_addr = 0;
#if GPS_DL_ON_LINUX
	struct gps_dl_iomem_addr_map_entry *p_iomem_info;
#endif

	for (i = 0; i < p_fw_list->n_desc; i++) {
		p_desc = &p_fw_list->p_desc[i];
		p_dst = NULL;
		switch (p_desc->type) {
		case MCU_FW_ROM:
#if GPS_DL_ON_LINUX
			p_iomem_info = gps_dl_get_dyn_iomem_info();
			p_dst = (void *)p_iomem_info->host_virt_addr;
			phys_addr = p_iomem_info->host_phys_addr;
#else
			p_dst = (void *)0x18D00000;
			phys_addr = 0x18D00000;
#endif
			/* set dynamic mapping*/
			gps_dl_hw_set_gps_dyn_remapping_tmp(0x2200000);
			break;
		case MCU_FW_EMI:
#if GPS_DL_ON_LINUX
			p_dst = gps_dl_get_conn_emi_virt_addr();
			phys_addr = gps_dl_get_conn_emi_phys_addr();
			gps_dl_hw_set_mcu_emi_remapping_tmp((phys_addr >> 16) & 0xFFFFF);
#else
			/* TODO: by alloc */
			/* set emi mapping*/
			p_dst = (void *)0x87000000;
			phys_addr = 0x87000000;
			gps_dl_hw_set_mcu_emi_remapping_tmp(0x8700);
#endif
			break;
		default:
			is_okay = false;
			break;
		}

		if (is_okay && p_dst && p_desc->ptr && p_desc->len != 0) {
#if GPS_DL_ON_LINUX
			memcpy_toio(p_dst, p_desc->ptr, p_desc->len);
#else
			memcpy(p_dst, p_desc->ptr, p_desc->len);
#endif
		}
		MDL_LOGW("type=%d, ok=%d, p_dst=0x%p(phys=0x%lx), p_src=0x%p, len=%d",
			p_desc->type, is_okay, p_dst, phys_addr,
			p_desc->ptr, p_desc->len);
	}
}

bool gps_mcudl_hal_mcu_do_on(const struct gps_mcudl_fw_list *p_fw_list)
{
	bool is_okay;

	gps_mcudl_hw_ccif_clr_tch_busy_bitmask();
	gps_mcudl_hw_ccif_clr_rch_busy_bitmask();
	is_okay = gps_mcudl_hw_mcu_do_on_with_rst_held();
	gps_mcudl_hal_get_ecid_info();
	MDL_LOGW("is_okay=%d, p_fw_list=0x%p", is_okay, p_fw_list);
	if (is_okay && p_fw_list) {
		gps_mcudl_hal_load_fw(p_fw_list);
		gps_mcudl_hw_mcu_speed_up_clock();
		gps_mcudl_hw_mcu_release_rst();
		is_okay = gps_mcudl_hw_mcu_wait_idle_loop_or_timeout_us(500 * 1000);
	}
	if (!is_okay) {
		gps_mcudl_hal_mcu_show_pc_log();
		gps_mcudl_hal_mcu_show_status();
		return false;
	}
	/* gps_mcudl_hal_mcu_clr_fw_own();*/
	gps_mcudl_hal_set_ccif_irq_en_flag(true);
	gps_mcudl_hal_wdt_init();
	gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_HAL);
	gps_dl_irq_unmask(gps_dl_irq_index_to_id(GPS_DL_IRQ_WDT), GPS_DL_IRQ_CTRL_FROM_HAL);
	return is_okay;
}

void gps_mcudl_hal_mcu_do_off(void)
{
	gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_WDT), GPS_DL_IRQ_CTRL_FROM_HAL);
	if (gps_mcudl_hal_get_ccif_irq_en_flag())
		gps_dl_irq_mask(gps_dl_irq_index_to_id(GPS_DL_IRQ_CCIF), GPS_DL_IRQ_CTRL_FROM_HAL);

	gps_mcudl_hw_mcu_do_off();
	gps_mcudl_hw_ccif_clr_tch_busy_bitmask();
	gps_mcudl_hw_ccif_clr_rch_busy_bitmask();
}

bool gps_mcudl_hal_mcu_set_fw_own(void)
{
	bool is_okay;

	is_okay = gps_mcudl_hw_mcu_set_or_clr_fw_own(true);
	(void)gps_mcudl_hw_conn_force_wake(false);
	return is_okay;
}

bool gps_mcudl_hal_mcu_clr_fw_own(void)
{
	bool is_okay;

	is_okay = gps_mcudl_hw_conn_force_wake(true);
	if (!is_okay) {
		MDL_LOGW("force wake fail!");
		return false;
	}
	is_okay = gps_mcudl_hw_mcu_set_or_clr_fw_own(false);
	return is_okay;
}

void gps_mcudl_hal_mcu_show_status(void)
{
	gps_mcudl_hw_mcu_show_status();
}

void gps_mcudl_hal_mcu_show_pc_log(void)
{
	gps_mcudl_hw_mcu_show_pc_log();
}

bool gps_mcudl_hal_bg_is_readable(bool check_conn_off)
{
#if GPS_DL_HAS_CONNINFRA_DRV
	if (check_conn_off) {
		if (gps_mcudl_coredump_conninfra_off_is_readable() == 0)
			return false;
	}
#endif
	return gps_mcudl_hw_bg_is_readable();
}

void gps_mcudl_hal_vdnr_dump(void)
{
#if GPS_DL_ON_LINUX
	gps_dl_iomem_dump(VDNR_DEBUG_AO_CTRL_ADDR, 0x4);
	gps_dl_iomem_dump(VDNR_DEBUG_CTRL_RESULT_ADDR, 0x3c);
#else
	MDL_LOGW("not impl!");
#endif
}

