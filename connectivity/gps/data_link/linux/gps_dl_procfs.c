/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 - 2021 MediaTek Inc.
 */

#include "gps_dl_config.h"

#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include "gps_dl_osal.h"
#include "gps_dl_procfs.h"
#include "gps_dl_context.h"
#include "gps_each_link.h"
#include "gps_dl_subsys_reset.h"
#include "gps_dl_hal_met2_0.h"
#include "gps_dl_hist_rec2.h"
#include "gps_dl_linux_plat_drv.h"
#include "gps_each_device.h"
#if GPS_DL_HAS_MCUDL
#include "gps_mcudl_xlink.h"
#include "gps_mcudl_ylink.h"
#include "gps_mcudl_hal_user_fw_own_ctrl.h"
#include "gps_mcu_hif_host.h"
#include "gps_mcudl_data_pkt_payload_struct.h"
#include "gps_mcudl_data_pkt_host_api.h"
#include "gps_mcudl_each_link.h"
#include "gps_mcudl_plat_api.h"
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
#include "gps_mcudl_hal_stat.h"
#endif
#include "gps_mcudl_hal_conn.h"
#endif
#include "gps_dl_iomem_dump.h"

int gps_dl_procfs_dummy_op(int y, int z)
{
	if (y == 1) {
		gps_dl_show_major_iomem_info();
		return 0;
	}
	GDL_LOGW("do nothing: y = %d, z = %d", y, z);
	return 0;
}

int gps_dl_procfs_read_iomem(int y, int z)
{
	gps_dl_iomem_dump((unsigned int)y, (unsigned int)z);
	return 0;
}

#if GPS_DL_HAS_MCUDL
int gps_dl_procfs_read_mcu_reg(int y, int z)
{
	gps_mcudl_xlink_test_read_mcu_reg((unsigned int)y, (unsigned int)z);
	return 0;
}
#endif

#ifdef GPS_DL_ENABLE_MET
int gps_dl_procfs_set_met(int y, int z)
{
	int ret = 0;

	GDL_LOGW("gps_dl_procfs_set_met: y = %d, z = %d", y, z);
	switch (y) {
	case GPS_DEBUG_OP_START:
		ret = gps_debug_met_start(&g_gps_debug_met_contex);
		break;
	case GPS_DEBUG_OP_STOP:
		ret = gps_debug_met_stop(&g_gps_debug_met_contex);
		break;
	case GPS_DEBUG_OP_CLEAR:
		gps_debug_met_clear(&g_gps_debug_met_contex);
		break;
	case GPS_DEBUG_OP_SET_BUFFER_MODE:
	case GPS_DEBUG_OP_SET_SAMPLE_RATE:
	case GPS_DEBUG_OP_SET_MASK_SIGNAL:
	case GPS_DEBUG_OP_SET_EVENT_SIGNAL:
	case GPS_DEBUG_OP_SET_EDGE_DETECTION:
	case GPS_DEBUG_OP_SET_EVENT_SELECT:
	case GPS_DEBUG_OP_SET_TIMER_SOURCE:
#if GPS_DL_MET_V2
		/*Interntional fallthrough*/
	case GPS_DEBUG_OP_SET_MASK_SIGNAL2:
		/*Interntional fallthrough*/
	case GPS_DEBUG_OP_SET_EDGE_DETECTION2:
		/*Interntional fallthrough*/
	case GPS_DEBUG_OP_SET_EDGE_DETECTION3:
		/*Interntional fallthrough*/
	case GPS_DEBUG_OP_SET_EDGE_DETECTION4:
		/*Interntional fallthrough*/
#endif
		ret = gps_debug_met_set_parameter(&g_gps_debug_met_contex, y, z);
		break;
	default:
		ret = -1;
		GDL_LOGE("gps_dl_procfs_set_met,ERROR input y = %d, z = %d", y, z);
		break;
	}
	return ret;
}
#endif

int gps_dl_procfs_set_opt(int y, int z)
{
	if (y == 0)
		gps_dl_log_info_show();
	else if (y == 1) {
		enum gps_dl_log_level_enum level_old, level_new;

		level_old = gps_dl_log_level_get();
		gps_dl_log_level_set((enum gps_dl_log_level_enum)z);
		level_new = gps_dl_log_level_get();
		GDL_LOGW("log level change: %d to %d", level_old, level_new);
	} else if (y == 2) {
		unsigned int mod_old, mod_new;

		mod_old = gps_dl_log_mod_bitmask_get();
		gps_dl_log_mod_bitmask_set(z);
		mod_new = gps_dl_log_level_get();
		GDL_LOGW("log modules change: 0x%x to 0x%x", mod_old, mod_new);
	} else if (y == 3) {
		unsigned int mod_old, mod_new;

		mod_old = gps_dl_log_mod_bitmask_get();
		gps_dl_log_mod_on((enum gps_dl_log_module_enum)z);
		mod_new = gps_dl_log_level_get();
		GDL_LOGW("log modules change: 0x%x to 0x%x", mod_old, mod_new);
	} else if (y == 4) {
		unsigned int mod_old, mod_new;

		mod_old = gps_dl_log_mod_bitmask_get();
		gps_dl_log_mod_off((enum gps_dl_log_module_enum)z);
		mod_new = gps_dl_log_level_get();
		GDL_LOGW("log modules change: 0x%x to 0x%x", mod_old, mod_new);
	} else if (y == 5) {
		bool rrw_old, rrw_new;

		rrw_old = gps_dl_show_reg_rw_log();
		if (z == 0)
			gps_dl_set_show_reg_rw_log(false);
		else if (z == 1)
			gps_dl_set_show_reg_rw_log(true);
		rrw_new = gps_dl_show_reg_rw_log();
		GDL_LOGW("log rrw change: %d to %d", rrw_old, rrw_new);
	}

	return 0;
}

int gps_dl_procfs_trigger_reset(int y, int z)
{
	/* Replace the new API to avoid QA tests hitting the same keywords as the normal EE keywords */
	if (y == 0)
		gps_dl_trigger_connsys_reset_v2();
	else if (y == 1)
		gps_dl_trigger_gps_subsys_reset((bool)z);
	else if (y == 2 && (z >= 0 && z <= GPS_DATA_LINK_NUM))
		gps_each_link_reset(z);
	else if (y == 3)
		gps_dl_trigger_gps_print_hw_status();
	else if (y == 4 && (z >= 0 && z <= GPS_DATA_LINK_NUM))
		gps_dl_test_mask_hasdata_irq_set(z, true);
	else if (y == 6 && (z >= 0 && z <= GPS_DATA_LINK_NUM))
		gps_dl_test_mask_mcub_irq_on_open_set(z, true);
	else if (y == 7)
		gps_dl_trigger_gps_print_data_status();
#if GPS_DL_HAS_MCUDL
	else if (y == 0x10)
		(void)gps_mcudl_xlink_test_toggle_reset_by_gps_hif((unsigned int)z);
	else if (y == 0x11)
		gps_mcudl_xlink_test_bypass_mcu2ap_data(z != 0);
	else if (y == 0x12)
		gps_mcudl_trigger_gps_subsys_reset(true, "GNSS intended for testing");
#endif
	return 0;
}

int gps_dl_procfs_set_data_routing_status(int y, int z)
{
	if (y == 0)
		gps_dl_hist_rec2_disable_data_routing();
	else if (y == 1)
		gps_dl_hist_rec2_enable_data_routing();
	return 0;
}

int gps_dl_procfs_set_opid_duration(int y, int z)
{
	if (y == 0)
		gps_dl_opid_timeout_info_show();
	else if (y == 1) {
		unsigned long enque_timeout_old, enque_timeout_new;

		enque_timeout_old = gps_dl_opid_enque_timeout_get();
		gps_dl_opid_enque_timeout_set((unsigned long)z);
		enque_timeout_new = gps_dl_opid_enque_timeout_get();
		GDL_LOGW("opid enque timeout change: %lu to %lu", enque_timeout_old, enque_timeout_new);
	} else if (y == 2) {
		unsigned long opfunc_timeout_old, opfunc_timeout_new;

		opfunc_timeout_old = gps_dl_opid_opfunc_timeout_get();
		gps_dl_opid_opfunc_timeout_set((unsigned long)z);
		opfunc_timeout_new = gps_dl_opid_opfunc_timeout_get();
		GDL_LOGW("opfunc timeout change: %lu to %lu", opfunc_timeout_old, opfunc_timeout_new);
	}
	return 0;
}
#if GPS_DL_HAS_MCUDL
/* arrive here if x == 0x10 */
int gps_mcudl_procfs_dbg(int y, int z)
{
	if (y == 0)
		gps_mcudl_xlink_trigger_print_hw_status();
	else if (y == 1) {
		if (z == 0 || z == 1)
			gps_mcudl_xlink_test_fw_own_ctrl(z != 0);
		else if (z == 2)
			gps_mcudl_hal_user_fw_own_status_dump();
		else if (z == 3)
			gps_mcudl_hal_set_non_lppm_sleep_flag(true);
		else if (z == 4)
			gps_mcudl_hal_set_non_lppm_sleep_flag(false);
		else if (z == 5)
			gps_mcu_hif_host_trans_hist_dump();
		else if (z == 6) {
			gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_HOST_WR);
			gps_mcu_host_trans_hist_dump(GPS_MCUDL_HIST_REC_MCU_ACK);
		} else if (z == 7) {
			gps_mcudl_host_sta_hist_dump(GPS_MDLY_NORMAL);
			gps_mcudl_host_sta_hist_dump(GPS_MDLY_URGENT);
		} else if (z == 8) {
			gps_mcudl_mcu2ap_rec_dump();
			gps_mcudl_xlink_dump_all_rec();
		} else if (z == 9) {
			GDL_LOGW("timeout_ms_to_set_fw_own=%u, fw_own_op_duration_us_to_warn=%u",
				gps_mcudl_hal_user_get_timeout_ms_to_set_fw_own(),
				gps_mcudl_hal_user_get_fw_own_op_duration_us_to_warn());
		} else if (z == 0xA || z == 0xB)
			gps_mcudl_set_need_to_load_fw_in_drv(z == 0xB);
		else if (z == 0x0C) {
			GDL_LOGW("gps_mcudl_ylink_on_ap_resume - test");
			gps_mcudl_ylink_on_ap_resume();
		}
	}
	else if (y == 2)
		gps_mcudl_xlink_test_toggle_ccif(z);
	else if (y == 3)
		gps_mcudl_xlink_fw_log_ctrl(z != 0);
	else if (y == 4)
		gps_mcudl_xlink_test_query_ver();
	else if (y == 5)
		gps_mcudl_xlink_test_wakeup_ap_later((unsigned int)z);
	else if (y == 6)
		gps_mcudl_xlink_test_send_4byte_mgmt_data((unsigned int)z);
	else if (y == 7)
		gps_mcudl_hal_user_set_timeout_ms_to_set_fw_own((unsigned int)z);
	else if (y == 8)
		gps_mcudl_hal_user_set_fw_own_op_duration_us_to_warn((unsigned int)z);
	else if (y == 9) {
#ifdef GPS_DL_HAS_MCUDL_HAL_STAT
		gps_mcudl_stat_dump_mcu_data();
		gps_mcudl_stat_dump_lp_data();
#endif
	} else if (y == 0xA)
		gps_mcudl_hal_dump_set_pwr_wrn_sec_for_test((unsigned int)z);

	return 0;
}
#endif

gps_dl_procfs_test_func_type g_gps_dl_proc_test_func_list[] = {
	[0x00] = gps_dl_procfs_dummy_op,
	[0x01] = gps_dl_procfs_read_iomem,
	/* [0x02] = TODO: reg write */
	[0x02] = NULL,
	[0x03] = gps_dl_procfs_set_opt,
	[0x04] = gps_dl_procfs_trigger_reset,
	/* [0x05] = gps_dl_procfs_show_pmic_efuse_reg: corgin efuse read */
	[0x05] = NULL,
	/* [0x06] = gps_dl_procfs_set_vcore_power: corgin vcore setting */
	[0x06] = NULL,
	#ifdef GPS_DL_ENABLE_MET
	[0x07] = gps_dl_procfs_set_met,
	#else
	[0x07] = NULL,
	#endif
	[0x08] = gps_dl_procfs_set_data_routing_status,
	[0x09] = gps_dl_procfs_set_opid_duration,
#if GPS_DL_HAS_MCUDL
	[0x10] = gps_mcudl_procfs_dbg,
	[0x11] = gps_dl_procfs_read_mcu_reg,
#endif
};

#define UNLOCK_MAGIC 0xDB9DB9
#define PROCFS_WR_BUF_SIZE 256
ssize_t gps_dl_procfs_write(struct file *filp, const char __user *buffer, size_t count, loff_t *f_pos)
{
	size_t len = count, sub_len;
	char buf[PROCFS_WR_BUF_SIZE];
	char *pBuf = NULL;
	int x = 0, y = 0, z = 0;
	char *pToken = NULL;
	long res;
	static bool gpsdl_dbg_enabled;

	GDL_LOGD("write parameter len = %d", (int)len);
	if (len >= PROCFS_WR_BUF_SIZE) {
		GDL_LOGE("input handling fail!");
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';
	pBuf = buf;
	do {
		if (!pBuf) {
			GDL_LOGW("x,y,z use default value - case0");
			break;
		}
		res = 0;
		sub_len = strlen(pBuf);
		GDL_LOGD("write parameter data = %s, len = %ld", pBuf, sub_len);
		if (sub_len != 0)
			pToken = gps_dl_osal_strsep(&pBuf, "\t\n\r ");
		if (sub_len != 0 && pToken != NULL) {
			if (gps_dl_osal_strtol(pToken, 16, &res) < 0) {
				GDL_LOGW("x use default value");
				break;
			}
			x = (int)res;
		} else {
			GDL_LOGW("x use default value");
			break;
		}

		if (!pBuf) {
			GDL_LOGW("y use default value - case1");
			break;
		}
		res = 0;
		sub_len = strlen(pBuf);
		GDL_LOGD("write parameter data = %s, len = %ld", pBuf, sub_len);
		if (sub_len != 0)
			pToken = gps_dl_osal_strsep(&pBuf, "\t\n\r ");
		if (sub_len != 0 && pToken != NULL) {
			gps_dl_osal_strtol(pToken, 16, &res);
			y = (int)res;
		} else {
			GDL_LOGW("y use default value - case2");
			break;
		}

		if (!pBuf) {
			GDL_LOGW("z use default value - case1");
			break;
		}
		res = 0;
		sub_len = strlen(pBuf);
		GDL_LOGD("write parameter data = %s, len = %ld", pBuf, sub_len);
		if (sub_len != 0)
			pToken = gps_dl_osal_strsep(&pBuf, "\t\n\r ");
		if (sub_len != 0 && pToken != NULL) {
			gps_dl_osal_strtol(pToken, 16, &res);
			z = (int)res;
		} else {
			GDL_LOGW("z use default value - case2");
			break;
		}
	} while (0);
	GDL_LOGW("x = 0x%08x, y = 0x%08x, z = 0x%08x", x, y, z);

	/* For eng and userdebug load, have to enable gpsdl_dbg by
	 * writing 0xDB9DB9 to * "/proc/driver/gpsdl_dbg" to avoid
	 * some malicious use
	 */
	if (x == UNLOCK_MAGIC) {
		gpsdl_dbg_enabled = true;
		return len;
	}

	if (!gpsdl_dbg_enabled) {
		GDL_LOGW("please enable gpsdl_dbg firstly");
		return len;
	}

	if (ARRAY_SIZE(g_gps_dl_proc_test_func_list) > x && NULL != g_gps_dl_proc_test_func_list[x])
		(*g_gps_dl_proc_test_func_list[x])(y, z);
	else
		GDL_LOGW("no handler defined for command id, x = 0x%08x", x);

	return len;
}

ssize_t gps_dl_procfs_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static const struct proc_ops gps_dl_procfs_fops = {
	.proc_read = gps_dl_procfs_read,
	.proc_write = gps_dl_procfs_write,
};
#else
static const struct file_operations gps_dl_procfs_fops = {
	.owner = THIS_MODULE,
	.read = gps_dl_procfs_read,
	.write = gps_dl_procfs_write,
};
#endif

static struct proc_dir_entry *g_gps_dl_procfs_entry;
#define GPS_DL_PROCFS_NAME "driver/gpsdl_dbg"

int gps_dl_procfs_setup(void)
{

	int i_ret = 0;

	g_gps_dl_procfs_entry = proc_create(GPS_DL_PROCFS_NAME,
		0600, NULL, &gps_dl_procfs_fops);

	if (g_gps_dl_procfs_entry == NULL) {
		GDL_LOGE("Unable to create gps proc entry");
		i_ret = -1;
	}

	return i_ret;
}

int gps_dl_procfs_remove(void)
{
	if (g_gps_dl_procfs_entry != NULL)
		proc_remove(g_gps_dl_procfs_entry);
	return 0;
}

