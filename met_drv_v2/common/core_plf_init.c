// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/kallsyms.h>
#include "met_drv.h"
#include "met_api.h"
#include "interface.h"
#include "core_plf_init.h"
#include "met_kernel_symbol.h"

#undef	DEBUG

#ifdef MET_GPU
/*
 *   GPU
 */
bool (*mtk_get_gpu_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_gpu_block_symbol)(unsigned int *pBlock);
bool (*mtk_get_gpu_idle_symbol)(unsigned int *pIdle);
bool (*mtk_get_gpu_dvfs_from_symbol)(enum MTK_GPU_DVFS_TYPE *peType, unsigned long *pulFreq);
bool (*mtk_get_gpu_sub_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_3D_fences_count_symbol)(int *pi32Count);
bool (*mtk_get_gpu_memory_usage_symbol)(unsigned int *pMemUsage);
bool (*mtk_get_gpu_power_loading_symbol)(unsigned int *pLoading);
bool (*mtk_get_custom_boost_gpu_freq_symbol)(unsigned int *pulFreq);
bool (*mtk_get_custom_upbound_gpu_freq_symbol)(unsigned int *pulFreq);
bool (*mtk_get_vsync_based_target_freq_symbol)(unsigned long *pulFreq);
bool (*mtk_get_vsync_offset_event_status_symbol)(unsigned int *pui32EventStatus);
bool (*mtk_get_vsync_offset_debug_status_symbol)(unsigned int *pui32EventStatus);
bool (*mtk_enable_gpu_perf_monitor_symbol)(bool enable);
bool (*mtk_get_gpu_pmu_init_symbol)(struct GPU_PMU *pmus, int pmu_size, int *ret_size);
bool (*mtk_get_gpu_pmu_swapnreset_symbol)(struct GPU_PMU *pmus, int pmu_size);
#if 1
bool (*mtk_get_gpu_pmu_deinit_symbol)(void);
bool (*mtk_get_gpu_pmu_swapnreset_stop_symbol)(void);
#endif
unsigned int (*mt_gpufreq_get_cur_freq_symbol)(void);
unsigned int (*mt_gpufreq_get_thermal_limit_freq_symbol)(void);
bool (*mtk_register_gpu_power_change_symbol)(const char *name, void (*callback)(int power_on));
bool (*mtk_unregister_gpu_power_change_symbol)(const char *name);
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
/*
 *   VCORE DVFS
 */
#ifdef VCOREDVFS_OLD_VER

#include <mtk_vcorefs_governor.h>
#include <mtk_spm_vcore_dvfs.h>

u32 (*spm_vcorefs_get_MD_status_symbol)(void);
void (*spm_vcorefs_register_handler_symbol)(vcorefs_handler_t handler);
void (*vcorefs_register_req_notify_symbol)(vcorefs_req_handler_t handler);
char *(*governor_get_kicker_name_symbol)(int id);
int (*vcorefs_enable_debug_isr_symbol)(bool);
int (*vcorefs_get_hw_opp_symbol)(void);
int (*vcorefs_get_curr_vcore_symbol)(void);
int (*vcorefs_get_curr_ddr_symbol)(void);
int *kicker_table_symbol;
#else

#include <dvfsrc-exp.h>

#endif /* end else VCOREDVFS_OLD_VER*/
int  (*vcorefs_get_opp_info_num_symbol)(void);
char ** (*vcorefs_get_opp_info_name_symbol)(void);
unsigned int * (*vcorefs_get_opp_info_symbol)(void);
int  (*vcorefs_get_src_req_num_symbol)(void);
char ** (*vcorefs_get_src_req_name_symbol)(void);
unsigned int * (*vcorefs_get_src_req_symbol)(void);
int (*vcorefs_get_num_opp_symbol)(void);

#endif /* MET_VCOREDVFS */


#ifdef MET_EMI
void *(*mt_cen_emi_base_get_symbol)(void);
void *(*mt_chn_emi_base_get_symbol)(int chn);
unsigned int (*mtk_dramc_get_data_rate_symbol)(void);
unsigned int (*mtk_dramc_get_ddr_type_symbol)(void);
int (*get_cur_ddr_ratio_symbol)(void);
/* legacy emi api before mt6885 */
unsigned int (*get_dram_data_rate_symbol)(void);
int (*get_ddr_type_symbol)(void);
#endif /* MET_EMI */

#ifdef MET_PTPOD
unsigned int (*mt_gpufreq_get_cur_volt_symbol)(void);
unsigned int (*mt_cpufreq_get_cur_volt_symbol)(unsigned int cluster_id);
#endif /* MET_PTPOD */

#ifdef MET_SPM_TWAM
/* ap side used */
#ifdef SPMTWAM_AP
void (*spm_twam_enable_monitor_symbol)(const struct twam_sig *twamsig, bool speed_mode);
void (*spm_twam_register_handler_symbol)(twam_handler_t handler);
void (*spm_twam_disable_monitor_symbol)(void);
void (*spm_twam_set_idle_select_symbol)(unsigned int sel);
void (*spm_twam_set_window_length_symbol)(unsigned int len);
void (*spm_twam_set_mon_type_symbol)(struct twam_sig *mon);
#endif

/* sspm side used */
#ifdef SPMTWAM_SSPM
void (*spm_twam_enable_monitor_symbol)(bool en_monitor, bool debug_signal, twam_handler_t cb_handler);
bool (*spm_twam_met_enable_symbol)(void);
void (*spm_twam_config_channel_symbol)(struct twam_cfg *cfg, bool speed_mode, unsigned int window_len_hz);
#endif
#endif


static int met_symbol_get(void)
{
#ifdef MET_GPU
	_MET_SYMBOL_GET(mtk_get_gpu_loading);
	_MET_SYMBOL_GET(mtk_get_gpu_block);
	_MET_SYMBOL_GET(mtk_get_gpu_idle);
	_MET_SYMBOL_GET(mtk_get_gpu_dvfs_from);
	_MET_SYMBOL_GET(mtk_get_gpu_sub_loading);
	_MET_SYMBOL_GET(mtk_get_3D_fences_count);
	_MET_SYMBOL_GET(mtk_get_gpu_memory_usage);
	_MET_SYMBOL_GET(mtk_get_gpu_power_loading);
	_MET_SYMBOL_GET(mtk_get_custom_boost_gpu_freq);
	_MET_SYMBOL_GET(mtk_get_custom_upbound_gpu_freq);
	_MET_SYMBOL_GET(mtk_get_vsync_based_target_freq);
	_MET_SYMBOL_GET(mtk_get_vsync_offset_event_status);
	_MET_SYMBOL_GET(mtk_get_vsync_offset_debug_status);
	_MET_SYMBOL_GET(mtk_enable_gpu_perf_monitor);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_init);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_swapnreset);
	_MET_SYMBOL_GET(mt_gpufreq_get_cur_freq);
	_MET_SYMBOL_GET(mt_gpufreq_get_thermal_limit_freq);
	_MET_SYMBOL_GET(mtk_register_gpu_power_change);
	_MET_SYMBOL_GET(mtk_unregister_gpu_power_change);
#if 1
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_swapnreset_stop);
	_MET_SYMBOL_GET(mtk_get_gpu_pmu_deinit);
#endif
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
	_MET_SYMBOL_GET(vcorefs_get_num_opp);
	_MET_SYMBOL_GET(vcorefs_get_opp_info_num);
	_MET_SYMBOL_GET(vcorefs_get_opp_info_name);
	_MET_SYMBOL_GET(vcorefs_get_opp_info);
	_MET_SYMBOL_GET(vcorefs_get_src_req_num);
	_MET_SYMBOL_GET(vcorefs_get_src_req_name);
	_MET_SYMBOL_GET(vcorefs_get_src_req);

#ifdef VCOREDVFS_OLD_VER
	_MET_SYMBOL_GET(spm_vcorefs_get_MD_status);
	_MET_SYMBOL_GET(spm_vcorefs_register_handler);
	_MET_SYMBOL_GET(vcorefs_register_req_notify);
	_MET_SYMBOL_GET(governor_get_kicker_name);
	_MET_SYMBOL_GET(vcorefs_enable_debug_isr);
	_MET_SYMBOL_GET(vcorefs_get_hw_opp);
	_MET_SYMBOL_GET(vcorefs_get_curr_vcore);
	_MET_SYMBOL_GET(vcorefs_get_curr_ddr);
	_MET_SYMBOL_GET(kicker_table);
#endif

#endif

#ifdef MET_EMI
	_MET_SYMBOL_GET(mt_cen_emi_base_get);
	_MET_SYMBOL_GET(mt_chn_emi_base_get);
	_MET_SYMBOL_GET(mtk_dramc_get_data_rate);
	_MET_SYMBOL_GET(mtk_dramc_get_ddr_type);
	_MET_SYMBOL_GET(get_cur_ddr_ratio);
	/* legacy emi api before mt6885 */
	_MET_SYMBOL_GET(get_dram_data_rate);
	_MET_SYMBOL_GET(get_ddr_type);
#endif

#ifdef MET_PTPOD
	_MET_SYMBOL_GET(mt_gpufreq_get_cur_volt);
	_MET_SYMBOL_GET(mt_cpufreq_get_cur_volt);
#endif

#ifdef MET_SPM_TWAM
		/* ap side used */
#ifdef SPMTWAM_AP
		_MET_SYMBOL_GET(spm_twam_enable_monitor);
		_MET_SYMBOL_GET(spm_twam_register_handler);
		_MET_SYMBOL_GET(spm_twam_disable_monitor);
		_MET_SYMBOL_GET(spm_twam_set_idle_select);
		_MET_SYMBOL_GET(spm_twam_set_window_length);
		_MET_SYMBOL_GET(spm_twam_set_mon_type);
#endif

		/* sspm side used */
#ifdef SPMTWAM_SSPM
		_MET_SYMBOL_GET(spm_twam_enable_monitor);
		_MET_SYMBOL_GET(spm_twam_met_enable);
		_MET_SYMBOL_GET(spm_twam_config_channel);
#endif
#endif

	return 0;
}

static int met_symbol_put(void)
{
#ifdef MET_GPU
	_MET_SYMBOL_PUT(mtk_get_gpu_loading);
	_MET_SYMBOL_PUT(mtk_get_gpu_block);
	_MET_SYMBOL_PUT(mtk_get_gpu_idle);
	_MET_SYMBOL_PUT(mtk_get_gpu_dvfs_from);
	_MET_SYMBOL_PUT(mtk_get_gpu_sub_loading);
	_MET_SYMBOL_PUT(mtk_get_3D_fences_count);
	_MET_SYMBOL_PUT(mtk_get_gpu_memory_usage);
	_MET_SYMBOL_PUT(mtk_get_gpu_power_loading);
	_MET_SYMBOL_PUT(mtk_get_custom_boost_gpu_freq);
	_MET_SYMBOL_PUT(mtk_get_custom_upbound_gpu_freq);
	_MET_SYMBOL_PUT(mtk_get_vsync_based_target_freq);
	_MET_SYMBOL_PUT(mtk_get_vsync_offset_event_status);
	_MET_SYMBOL_PUT(mtk_get_vsync_offset_debug_status);
	_MET_SYMBOL_PUT(mtk_enable_gpu_perf_monitor);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_init);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_swapnreset);
	_MET_SYMBOL_PUT(mt_gpufreq_get_cur_freq);
	_MET_SYMBOL_PUT(mt_gpufreq_get_thermal_limit_freq);
	_MET_SYMBOL_PUT(mtk_register_gpu_power_change);
	_MET_SYMBOL_PUT(mtk_unregister_gpu_power_change);
#if 1
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_swapnreset_stop);
	_MET_SYMBOL_PUT(mtk_get_gpu_pmu_deinit);
#endif
#endif /* MET_GPU */

#ifdef MET_VCOREDVFS
	_MET_SYMBOL_PUT(vcorefs_get_num_opp);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info_num);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info_name);
	_MET_SYMBOL_PUT(vcorefs_get_opp_info);
	_MET_SYMBOL_PUT(vcorefs_get_src_req_num);
	_MET_SYMBOL_PUT(vcorefs_get_src_req_name);
	_MET_SYMBOL_PUT(vcorefs_get_src_req);

#ifdef VCOREDVFS_OLD_VER
	_MET_SYMBOL_PUT(spm_vcorefs_get_MD_status);
	_MET_SYMBOL_PUT(spm_vcorefs_register_handler);
	_MET_SYMBOL_PUT(vcorefs_register_req_notify);
	_MET_SYMBOL_PUT(governor_get_kicker_name);
	_MET_SYMBOL_PUT(vcorefs_enable_debug_isr);
	_MET_SYMBOL_PUT(vcorefs_get_hw_opp);
	_MET_SYMBOL_PUT(vcorefs_get_curr_vcore);
	_MET_SYMBOL_PUT(vcorefs_get_curr_ddr);
	_MET_SYMBOL_PUT(kicker_table);
#endif

#endif

#ifdef MET_EMI
	_MET_SYMBOL_PUT(mt_cen_emi_base_get);
	_MET_SYMBOL_PUT(mt_chn_emi_base_get);
	_MET_SYMBOL_PUT(mtk_dramc_get_data_rate);
	_MET_SYMBOL_PUT(mtk_dramc_get_ddr_type);
	_MET_SYMBOL_PUT(get_cur_ddr_ratio);
	/* legacy emi api before mt6885 */
	_MET_SYMBOL_PUT(get_dram_data_rate);
	_MET_SYMBOL_PUT(get_ddr_type);
#endif

#ifdef MET_PTPOD
	_MET_SYMBOL_PUT(mt_gpufreq_get_cur_volt);
	_MET_SYMBOL_PUT(mt_cpufreq_get_cur_volt);
#endif

#ifdef MET_SPM_TWAM
		/* ap side used */
#ifdef SPMTWAM_AP
		_MET_SYMBOL_PUT(spm_twam_enable_monitor);
		_MET_SYMBOL_PUT(spm_twam_register_handler);
		_MET_SYMBOL_PUT(spm_twam_disable_monitor);
		_MET_SYMBOL_PUT(spm_twam_set_idle_select);
		_MET_SYMBOL_PUT(spm_twam_set_window_length);
		_MET_SYMBOL_PUT(spm_twam_set_mon_type);
#endif

		/* sspm side used */
#ifdef SPMTWAM_SSPM
		_MET_SYMBOL_PUT(spm_twam_enable_monitor);
		_MET_SYMBOL_PUT(spm_twam_met_enable);
		_MET_SYMBOL_PUT(spm_twam_config_channel);
#endif
#endif

	return 0;
}

int core_plf_init(void)
{
	/*initial met external symbol*/
	met_symbol_get();

#ifdef MET_GPU
	met_register(&met_gpu);
	met_register(&met_gpudvfs);
	met_register(&met_gpumem);
	met_register(&met_gpupwr);
	met_register(&met_gpu_pmu);
#ifdef MET_GPU_STALL_MONITOR
	met_register(&met_gpu_stall);
#endif
#endif

#ifdef MET_VCOREDVFS
	met_register(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_register(&met_sspm_emi);
#endif

#ifdef MET_SMI
	met_register(&met_sspm_smi);
#endif

#ifdef MET_PTPOD
	met_register(&met_ptpod);
#endif

#ifdef MET_WALLTIME
	met_register(&met_wall_time);
#endif

#ifdef MTK_TINYSYS_SSPM_SUPPORT
	met_register(&met_sspm_common);
#endif

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#ifdef MET_SSPM_WALLTIME
	met_register(&met_sspm_walltime);
#endif
#endif
#endif

#ifdef MET_CPUDSU
	met_register(&met_cpudsu);
#endif

#ifdef MET_SPM_TWAM
	met_register(&met_spmtwam);
#endif

#ifdef MET_BACKLIGHT
	met_register(&met_backlight);
#endif

	return 0;
}

void core_plf_exit(void)
{
	/*release met external symbol*/
	met_symbol_put();

#ifdef MET_GPU
	met_deregister(&met_gpu);
	met_deregister(&met_gpudvfs);
	met_deregister(&met_gpumem);
	met_deregister(&met_gpupwr);
	met_deregister(&met_gpu_pmu);
#ifdef MET_GPU_STALL_MONITOR
	met_deregister(&met_gpu_stall);
#endif
#endif

#ifdef MET_VCOREDVFS
	met_deregister(&met_vcoredvfs);
#endif

#ifdef MET_EMI
	met_deregister(&met_sspm_emi);
#endif

#ifdef MET_SMI
	met_deregister(&met_sspm_smi);
#endif

#ifdef MET_PTPOD
	met_deregister(&met_ptpod);
#endif

#ifdef MET_WALLTIME
	met_deregister(&met_wall_time);
#endif

#ifdef MTK_TINYSYS_SSPM_SUPPORT
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
	met_deregister(&met_sspm_common);
#endif
#endif

#if IS_ENABLED(CONFIG_MTK_TINYSYS_SSPM_SUPPORT)
#if defined(ONDIEMET_SUPPORT) || defined(TINYSYS_SSPM_SUPPORT)
#ifdef MET_SSPM_WALLTIME
	met_deregister(&met_sspm_walltime);
#endif
#endif
#endif

#ifdef MET_CPUDSU
	met_deregister(&met_cpudsu);
#endif

#ifdef MET_SPM_TWAM
	met_deregister(&met_spmtwam);
#endif

#ifdef MET_BACKLIGHT
	met_deregister(&met_backlight);
#endif
}
