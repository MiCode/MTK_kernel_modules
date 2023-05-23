// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <asm/page.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <linux/hrtimer.h>

#include "met_drv.h"
#include "trace.h"
#include "interface.h"
#include "mtk_typedefs.h"

#include "mtk_gpu_metmonitor.h"
#include "core_plf_init.h"
#include "core_plf_trace.h"


/*
 * define if the hal implementation might re-schedule, cannot run inside softirq
 * undefine this is better for sampling jitter if HAL support it
 */
#undef GPU_HAL_RUN_PREMPTIBLE


/*
 * GPU monitor HAL comes from alps\mediatek\kernel\include\linux\mtk_gpu_utility.h
 *
 * mtk_get_gpu_memory_usage(unsigned int* pMemUsage) in unit of bytes
 *
 * mtk_get_gpu_xxx_loading are in unit of %
*/
#ifdef MET_GPU_LOAD_MONITOR

#ifdef GPU_HAL_RUN_PREMPTIBLE
static struct delayed_work gpu_dwork;
#endif

enum MET_GPU_PROFILE_INDEX {
	eMET_GPU_LOADING = 0,
	eMET_GPU_BLOCK_LOADING,	/* 1 */
	eMET_GPU_IDLE_LOADING,	/* 2 */
	eMET_GPU_PROFILE_CNT
};

static unsigned long g_u4AvailableInfo;



noinline void GPU_Loading(unsigned char cnt, unsigned int *value)
{
	switch (cnt) {
	case 1:
		MET_TRACE("%u\n", value[0]);
		break;
	case 2:
		MET_TRACE("%u,%u\n", value[0], value[1]);
		break;
	case 3:
		MET_TRACE("%u,%u,%u\n", value[0], value[1], value[2]);
		break;
	case 4:
		MET_TRACE("%u,%u,%u,%u\n", value[0], value[1], value[2], value[3]);
		break;
	default:
		break;
	}

}

#ifdef GPU_HAL_RUN_PREMPTIBLE
static void gpu_GPULoading(struct work_struct *work)
{
	unsigned int	pu4Value[eMET_GPU_PROFILE_CNT];
	unsigned long	u4Index = 0;
	unsigned int	loading = 0;
	int		count = 0;

	memset(pu4Value, 0x00, eMET_GPU_PROFILE_CNT);
	if ((1 << eMET_GPU_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_loading_symbol && mtk_get_gpu_loading_symbol(&pu4Value[u4Index]))
			u4Index += 1;
	}

	if ((1 << eMET_GPU_BLOCK_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_block_symbol && mtk_get_gpu_block_symbol(&pu4Value[u4Index]))
			u4Index += 1;
	}

	if ((1 << eMET_GPU_IDLE_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_idle_symbol && mtk_get_gpu_idle_symbol(&pu4Value[u4Index]))
			u4Index += 1;
	}

	if (g_u4AvailableInfo)
		GPU_Loading(u4Index, pu4Value);
}
#else
static void gpu_GPULoading(unsigned long long stamp, int cpu)
{
	unsigned int	pu4Value[eMET_GPU_PROFILE_CNT];
	unsigned long	u4Index = 0;

	memset(pu4Value, 0x00, eMET_GPU_PROFILE_CNT);
	if ((1 << eMET_GPU_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_loading_symbol) {
			mtk_get_gpu_loading_symbol(&pu4Value[u4Index]);
			u4Index += 1;
		}
	}

	if ((1 << eMET_GPU_BLOCK_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_block_symbol) {
			mtk_get_gpu_block_symbol(&pu4Value[u4Index]);
			u4Index += 1;
		}
	}

	if ((1 << eMET_GPU_IDLE_LOADING) & g_u4AvailableInfo) {
		if (mtk_get_gpu_idle_symbol) {
			mtk_get_gpu_idle_symbol(&pu4Value[u4Index]);
			u4Index += 1;
		}
	}

	if (g_u4AvailableInfo)
		GPU_Loading(u4Index, pu4Value);
}
#endif

static void gpu_monitor_start(void)
{
	if (mtk_get_gpu_loading_symbol)
		g_u4AvailableInfo |= (1 << eMET_GPU_LOADING);
	if (mtk_get_gpu_block_symbol)
		g_u4AvailableInfo |= (1 << eMET_GPU_BLOCK_LOADING);
	if (mtk_get_gpu_idle_symbol)
		g_u4AvailableInfo |= (1 << eMET_GPU_IDLE_LOADING);

#ifdef GPU_HAL_RUN_PREMPTIBLE
	INIT_DELAYED_WORK(&gpu_dwork, gpu_GPULoading);
#endif
}

#ifdef GPU_HAL_RUN_PREMPTIBLE
static void gpu_monitor_stop(void)
{
	cancel_delayed_work_sync(&gpu_dwork);
}

static void GPULoadingNotify(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&gpu_dwork, 0);
}
#endif

static char help[] =
	"  --gpu					monitor gpu status\n";
static int gpu_status_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, help);
}

static char g_pComGPUStatusHeader[] =
	"met-info [000] 0.0: met_gpu_loading_header: ";
static int gpu_status_print_header(char *buf, int len)
{
	int ret = 0;

	ret = SNPRINTF(buf, PAGE_SIZE, "%s", g_pComGPUStatusHeader);

	if ((1 << eMET_GPU_LOADING) & g_u4AvailableInfo)
		ret += SNPRINTF(buf+ret, PAGE_SIZE-ret, "%s", "Loading,");

	if ((1 << eMET_GPU_BLOCK_LOADING) & g_u4AvailableInfo)
		ret += SNPRINTF(buf+ret, PAGE_SIZE-ret, "%s", "Block,");

	if ((1 << eMET_GPU_IDLE_LOADING) & g_u4AvailableInfo)
		ret += SNPRINTF(buf+ret, PAGE_SIZE-ret, "%s", "Idle");

	ret += SNPRINTF(buf+ret, PAGE_SIZE-ret, "%s", "\n");

	return ret;
}

MET_DEFINE_DEPENDENCY_BY_NAME(met_gpu_dependencies) = {
	{.symbol=(void**)&met_gpu_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=0},
};

struct metdevice met_gpu = {
	.name			= "gpu",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.cpu_related		= 0,
	.start			= gpu_monitor_start,
	.mode			= 0,
	.polling_interval	= 1,	/* ms */
#ifdef GPU_HAL_RUN_PREMPTIBLE
	.timed_polling		= GPULoadingNotify,
	.stop			= gpu_monitor_stop,
#else
	.timed_polling		= gpu_GPULoading,
#endif
	.print_help		= gpu_status_print_help,
	.print_header		= gpu_status_print_header,
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(met_gpu_dependencies)
};
#endif

/*
 * GPU DVFS Monitor
 */
#ifdef MET_GPU_DVFS_MONITOR
/* the mt_gpufreq_get_thermal_limit_freq use mutex_lock to do its job */
/* so, change the gpu-dvfs implementation to dwork */
static struct delayed_work gpu_dvfs_dwork;

noinline void GPU_DVFS(unsigned int curFreq)
{
	MET_TRACE("%u\n", curFreq);
}

noinline void GPU_OPP_IDX(int curOppIdx, int floorIdx, int ceilingIdx)
{
	MET_TRACE("%d,%d,%d\n", curOppIdx, floorIdx, ceilingIdx);
}

noinline void GPU_OPP_LIMITER(unsigned int floorLimiter, unsigned int ceilingLimiter)
{
	MET_TRACE("%u,%u\n", floorLimiter, ceilingLimiter);
}

static void gpu_dvfs(void)
{
	unsigned int curFreq = 0;
	int curOppIdx = 0;
	int floorIdx = 0;
	int ceilingIdx = 0;
	unsigned int floorLimiter = 0;
	unsigned int ceilingLimiter = 0;

	if (mtk_get_gpu_cur_freq_symbol)
		mtk_get_gpu_cur_freq_symbol(&curFreq);
	if (mtk_get_gpu_cur_oppidx_symbol)
		mtk_get_gpu_cur_oppidx_symbol(&curOppIdx);
	if (mtk_get_gpu_floor_index_symbol)
		mtk_get_gpu_floor_index_symbol(&floorIdx);
	if (mtk_get_gpu_ceiling_index_symbol)
		mtk_get_gpu_ceiling_index_symbol(&ceilingIdx);
	if (mtk_get_gpu_floor_limiter_symbol)
		mtk_get_gpu_floor_limiter_symbol(&floorLimiter);
	if (mtk_get_gpu_ceiling_limiter_symbol)
		mtk_get_gpu_ceiling_limiter_symbol(&ceilingLimiter);

	GPU_DVFS(curFreq);
	GPU_OPP_IDX(curOppIdx, floorIdx, ceilingIdx);
	GPU_OPP_LIMITER(floorLimiter, ceilingLimiter);

}

static void gpu_dvfs_work(struct work_struct *work)
{
	gpu_dvfs();
}

static void gpu_dvfs_monitor_start(void)
{
	gpu_dvfs();
	INIT_DELAYED_WORK(&gpu_dvfs_dwork, gpu_dvfs_work);
}

static void gpu_dvfs_monitor_stop(void)
{
	cancel_delayed_work_sync(&gpu_dvfs_dwork);
	gpu_dvfs();
}

static void gpu_dvfs_monitor_polling(unsigned long long stamp, int cpu)
{
	schedule_delayed_work(&gpu_dvfs_dwork, 0);
}

static int gpu_dvfs_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE,
			"  --gpu-dvfs				monitor gpu freq\n");
}

static int gpu_dvfs_print_header(char *buf, int len)
{
	int ret = 0;

	ret = SNPRINTF(buf, PAGE_SIZE,
			"met-info [000] 0.0: met_gpu_dvfs_header: Freq(kHz)\n");
	ret += SNPRINTF(buf+ret, PAGE_SIZE-ret,
			"met-info [000] 0.0: met_gpu_opp_idx_header: CurIdx,FloorIdx,CeilingIdx\n");
	ret += SNPRINTF(buf+ret, PAGE_SIZE-ret,
			"met-info [000] 0.0: met_gpu_opp_limiter_header: FloorLimiter,CeilingLimiter\n");

	return ret;
}

MET_DEFINE_DEPENDENCY_BY_NAME(met_gpudvfs_dependencies) = {
	{.symbol=(void**)&met_gpu_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=0},
};

struct metdevice met_gpudvfs = {
	.name			= "gpu-dvfs",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.cpu_related		= 0,
	.start			= gpu_dvfs_monitor_start,
	.stop			= gpu_dvfs_monitor_stop,
	.polling_interval	= 1,	/* ms */
	.timed_polling		= gpu_dvfs_monitor_polling,
	.print_help		= gpu_dvfs_print_help,
	.print_header		= gpu_dvfs_print_header,
	.ondiemet_mode		= 0,
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(met_gpudvfs_dependencies)
};
#endif

/*
 * GPU MEM monitor
 */
#ifdef MET_GPU_MEM_MONITOR
static unsigned long g_u4MemProfileIsOn;

static void gpu_mem_monitor_start(void)
{
	if (!mtk_get_gpu_memory_usage_symbol)
		return;

#if 0
	if (mtk_get_gpu_memory_usage_symbol(&u4Value))
		g_u4MemProfileIsOn = 1;
#endif
	g_u4MemProfileIsOn = 1;
}

noinline void GPU_MEM(unsigned long long stamp, int cpu)
{
	unsigned int u4Value = 0;

	if (!mtk_get_gpu_memory_usage_symbol)
		return;

	if (g_u4MemProfileIsOn == 1) {
		mtk_get_gpu_memory_usage_symbol(&u4Value);
		MET_TRACE("%d\n", u4Value);
	}
}

static void gpu_mem_monitor_stop(void)
{
	g_u4MemProfileIsOn = 0;
}

static char help_mem[] =
	"  --gpu-mem				monitor gpu memory status\n";
static int gpu_mem_status_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, help_mem);
}

static char g_pComGPUMemHeader[] =
	"met-info [000] 0.0: met_gpu_mem_header: Usage\n";
static int gpu_mem_status_print_header(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, g_pComGPUMemHeader);
}

MET_DEFINE_DEPENDENCY_BY_NAME(met_gpumem_dependencies) = {
	{.symbol=(void**)&met_gpu_adv_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=0},
};

struct metdevice met_gpumem = {
	.name			= "gpu-mem",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.cpu_related		= 0,
	.start			= gpu_mem_monitor_start,
	.stop			= gpu_mem_monitor_stop,
	.mode			= 0,
	.polling_interval	= 1,	/* ms */
	.timed_polling		= GPU_MEM,
	.print_help		= gpu_mem_status_print_help,
	.print_header		= gpu_mem_status_print_header,
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(met_gpumem_dependencies)
};
#endif

/*
 * GPU power monitor
 */
#ifdef MET_GPU_PWR_MONITOR

#ifdef GPU_HAL_RUN_PREMPTIBLE
static struct delayed_work gpu_pwr_dwork;
#endif

static unsigned long g_u4PowerProfileIsOn;

#ifdef GPU_HAL_RUN_PREMPTIBLE
noinline void GPU_Power(struct work_struct *work)
{
	unsigned int u4Value = 0;

	if (!mtk_get_gpu_power_loading_symbol)
		return;

	mtk_get_gpu_power_loading_symbol(&u4Value);
	MET_TRACE("%d\n", u4Value);
}

static void GPU_PowerNotify(unsigned long long stamp, int cpu)
{
	if (g_u4PowerProfileIsOn == 1)
		schedule_delayed_work(&gpu_pwr_dwork, 0);
}
#else
noinline void GPU_Power(unsigned long long stamp, int cpu)
{
	unsigned int u4Value = 0;

	if (!mtk_get_gpu_power_loading_symbol)
		return;

	if (g_u4PowerProfileIsOn == 1) {
		mtk_get_gpu_power_loading_symbol(&u4Value);
		MET_TRACE("%d\n", u4Value);
	}
}
#endif

static void gpu_Power_monitor_start(void)
{
	if (!mtk_get_gpu_power_loading_symbol)
		return;

#if 0
	if (mtk_get_gpu_power_loading_symbol(&u4Value))
		g_u4PowerProfileIsOn = 1;
#endif
	g_u4PowerProfileIsOn = 1;
#ifdef GPU_HAL_RUN_PREMPTIBLE
	INIT_DELAYED_WORK(&gpu_pwr_dwork, GPU_Power);
#endif
}

static void gpu_Power_monitor_stop(void)
{
	g_u4PowerProfileIsOn = 0;

#ifdef GPU_HAL_RUN_PREMPTIBLE
	cancel_delayed_work_sync(&gpu_pwr_dwork);
#endif
}

static char help_pwr[] =
	"  --gpu-pwr				monitor gpu power status\n";
static int gpu_Power_status_print_help(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, help_pwr);
}

static char g_pComGPUPowerHeader[] =
	"met-info [000] 0.0: met_gpu_power_header: Loading\n";
static int gpu_Power_status_print_header(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, g_pComGPUPowerHeader);
}

MET_DEFINE_DEPENDENCY_BY_NAME(met_gpupwr_dependencies) = {
	{.symbol=(void**)&met_gpu_adv_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=0},
};

struct metdevice met_gpupwr = {
	.name			= "gpu-pwr",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.cpu_related		= 0,
	.start			= gpu_Power_monitor_start,
	.stop			= gpu_Power_monitor_stop,
	.mode			= 0,
	.polling_interval	= 1,	/* ms */
#ifdef GPU_HAL_RUN_PREMPTIBLE
	.timed_polling		= GPU_PowerNotify,
#else
	.timed_polling		= GPU_Power,
#endif
	.print_help		= gpu_Power_status_print_help,
	.print_header		= gpu_Power_status_print_header,
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(met_gpupwr_dependencies)
};
#endif

/*
 * GPU PMU
 */
#ifdef MET_GPU_PMU_MONITOR
#define UNUSE_ARG(arg) ((void)arg)

#ifdef GPU_HAL_RUN_PREMPTIBLE
static struct delayed_work gpu_pmu_dwork;
#endif


static const char help_pmu[] = "  --gpu-pmu				monitor gpu pmu status";
static const char header_pmu[] = "met-info [000] 0.0: met_gpu_pmu_header: ";
static unsigned int output_header_pmu_len;
static unsigned int output_pmu_str_len;
static char *pmu_str = NULL;
static int pmu_cnt;
/* static int gpu_pwr_status = 1; */
static struct GPU_PMU *pmu_list;
unsigned int *pmu_value;


noinline void GPU_PMU_RAW(
	unsigned long long stamp,
	int cpu)
{
	bool ret;
	int i = 0;
	char *SOB, *EOB;


	if (stamp == 0 && cpu == 0) {
		for (i = 0; i < pmu_cnt; i++)
			pmu_value[i] = 0;

		MET_TRACE_GETBUF(&SOB, &EOB);
		EOB = ms_formatH(EOB, pmu_cnt, pmu_value);
		MET_TRACE_PUTBUF(SOB, EOB);
		return;
	}

	if (mtk_get_gpu_pmu_swapnreset_symbol) {
		ret = mtk_get_gpu_pmu_swapnreset_symbol(pmu_list, pmu_cnt);
		if (ret) {
			for (i = 0; i < pmu_cnt; i++) {
				if (pmu_list[i].overflow)
					pmu_list[i].value = 0xFFFFFFFF;
				pmu_value[i] = pmu_list[i].value;
			}
			MET_TRACE_GETBUF(&SOB, &EOB);
			EOB = ms_formatH(EOB, pmu_cnt, pmu_value);
			MET_TRACE_PUTBUF(SOB, EOB);
		}
	}
}

static int create_gpu_pmu_list(void)
{
	int ret = 0;
	int len = 0;
	int i = 0;
	int max_pmu_str_len = 0;

	if (mtk_get_gpu_pmu_init_symbol) {
		ret = mtk_get_gpu_pmu_init_symbol(NULL, 0, &pmu_cnt);
		if (pmu_cnt == 0 || ret == 0)
			return 0;
	} else
		return 0;

	pmu_list = kmalloc_array(pmu_cnt, sizeof(struct GPU_PMU), GFP_KERNEL);
	if (pmu_list) {
		memset(pmu_list, 0x00, sizeof(struct GPU_PMU)*pmu_cnt);
		ret = mtk_get_gpu_pmu_init_symbol(pmu_list, pmu_cnt, NULL);

		max_pmu_str_len += pmu_cnt;//comma & \0
		for (i = 0; i < pmu_cnt; i++)
			max_pmu_str_len += strlen(pmu_list[i].name);

		pmu_str = (char*)kmalloc(sizeof(char) * max_pmu_str_len, GFP_KERNEL);
		if (pmu_str) {
			memset(pmu_str, 0x00, max_pmu_str_len);
			len = SNPRINTF(pmu_str, max_pmu_str_len, "%s", pmu_list[0].name);
			for (i = 1; i < pmu_cnt; i++)
				len += SNPRINTF(pmu_str + len, max_pmu_str_len - len, ",%s", pmu_list[i].name);
		} else
			return 0;
		

		/*
		* dummy read in order to reset GPU PMU counter
		*/
		if (mtk_get_gpu_pmu_swapnreset_symbol)
			mtk_get_gpu_pmu_swapnreset_symbol(pmu_list, pmu_cnt);
	}

	pmu_value = kmalloc_array(pmu_cnt, sizeof(unsigned int), GFP_KERNEL);
	if (pmu_value) {
		memset(pmu_value, 0x00, sizeof(unsigned int)*pmu_cnt);
	}

	/* init state */
	met_gpu_pmu.header_read_again = 0;
	output_header_pmu_len = 0;
	output_pmu_str_len = 0;

	return ret;
}

static void delete_gpu_pmu_list(void)
{
	kfree(pmu_list);
	kfree(pmu_value);
	pmu_list = NULL;
	pmu_cnt = 0;
}

static void gpu_pmu_monitor_start(void)
{
	int ret;

	ret = create_gpu_pmu_list();
	if (ret == 0)
		return;

#ifdef GPU_HAL_RUN_PREMPTIBLE
	INIT_DELAYED_WORK(&gpu_pmu_dwork, GPU_PMU_RAW);
#endif
}

static void gpu_pmu_monitor_stop(void)
{
#ifdef GPU_HAL_RUN_PREMPTIBLE
	cancel_delayed_work_sync(&gpu_pmu_dwork);
#endif

	delete_gpu_pmu_list();

#if 1
	/* stop polling counter */
	if (mtk_get_gpu_pmu_swapnreset_stop_symbol)
		mtk_get_gpu_pmu_swapnreset_stop_symbol();
	/* release resource */
	if (mtk_get_gpu_pmu_deinit_symbol)
		mtk_get_gpu_pmu_deinit_symbol();
#endif
}

#ifdef GPU_HAL_RUN_PREMPTIBLE
static void gpu_pmu_timed_polling_notify(
	unsigned long long stamp,
	int cpu)
{
	UNUSE_ARG(stamp);
	UNUSE_ARG(cpu);

	if (gpu_pwr_status == 1)
		schedule_delayed_work(&gpu_pmu_dwork, 0);
}
#else
static void gpu_pmu_timed_polling(
	unsigned long long stamp,
	int cpu)
{
	UNUSE_ARG(stamp);
	UNUSE_ARG(cpu);

	GPU_PMU_RAW(stamp, cpu);
}
#endif

static int gpu_pmu_print_help(
	char *buf,
	int len)
{
	UNUSE_ARG(len);
	return SNPRINTF(buf, PAGE_SIZE, "%s\n", help_pmu);
}

static int gpu_pmu_print_header(
	char *buf,
	int len)
{
	if (pmu_str){
		if(output_header_pmu_len == 0){
			len = SNPRINTF(buf, PAGE_SIZE, "%s", header_pmu);
			met_gpu_pmu.header_read_again = 1;
			output_header_pmu_len = len;
		}
		else{
			if( (strlen(pmu_str) - output_pmu_str_len) > PAGE_SIZE ){
				char output_buf[PAGE_SIZE/4];

				strncpy(output_buf, pmu_str+output_pmu_str_len, PAGE_SIZE/4);
				len = SNPRINTF(buf, PAGE_SIZE, "%s", output_buf);
				output_pmu_str_len += len;
			}
			else{
				len = SNPRINTF(buf, PAGE_SIZE, "%s\n", pmu_str+output_pmu_str_len);

				/* reset state */
				met_gpu_pmu.header_read_again = 0;
				output_header_pmu_len = 0;
				output_pmu_str_len = 0;
			}
		}
	}
	

	return len;
}


// static int gpu_pmu_create_subfs(struct kobject *parent)
// {
// 	int ret = 0;
// 	return ret;
// }

static void gpu_pmu_delete_subfs(void)
{
	if (pmu_str)
		kfree(pmu_str);
}


MET_DEFINE_DEPENDENCY_BY_NAME(met_gpu_pmu_dependencies) = {
	{.symbol=(void**)&met_gpu_adv_api_ready, .init_once=0, .cpu_related=0, .ondiemet_mode=0},
};

struct metdevice met_gpu_pmu = {
	.name			= "gpu-pmu",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_PMU,
	.cpu_related		= 0,
	// .create_subfs		= gpu_pmu_create_subfs,
	.delete_subfs		= gpu_pmu_delete_subfs,
	.start			= gpu_pmu_monitor_start,
	.stop			= gpu_pmu_monitor_stop,
	.mode			= 0,
	.polling_interval	= 1,	/* ms */
#ifdef GPU_HAL_RUN_PREMPTIBLE
	.timed_polling		= gpu_pmu_timed_polling_notify,
#else
	.timed_polling		= gpu_pmu_timed_polling,
#endif
	.print_help		= gpu_pmu_print_help,
	.print_header		= gpu_pmu_print_header,
	MET_DEFINE_METDEVICE_DEPENDENCY_BY_NAME(met_gpu_pmu_dependencies)
};
#endif

/*
 * GPU stall counters
 */
#ifdef MET_GPU_STALL_MONITOR
static void __iomem	*io_addr_gpu_stall;

static int gpu_stall_create_subfs(struct kobject *parent)
{
	io_addr_gpu_stall = ioremap(IO_ADDR_GPU_STALL, IO_SIZE_GPU_STALL);
	if (!io_addr_gpu_stall) {
		PR_BOOTMSG("Failed to init GPU stall counters!!\n");
		return -ENODEV;
	}

	return 0;
}

static void gpu_stall_delete_subfs(void)
{
	if (io_addr_gpu_stall) {
		iounmap(io_addr_gpu_stall);
		io_addr_gpu_stall = NULL;
	}
}

static void gpu_stall_start(void)
{
#ifdef GPU_STALL_CNT_SINGLE
	unsigned int value = 0x00000001;
#else
	unsigned int value = 0x00010001;
#endif
	writel(value, io_addr_gpu_stall+OFFSET_STALL_GPU_M0_CHECK);
	writel(value, io_addr_gpu_stall+OFFSET_STALL_GPU_M1_CHECK);
	writel(value, io_addr_gpu_stall+OFFSET_STALL_GPU_M0_EMI_CHECK);
	writel(value, io_addr_gpu_stall+OFFSET_STALL_GPU_M1_EMI_CHECK);
}

static void gpu_stall_stop(void)
{
	writel(0x00000000, io_addr_gpu_stall+OFFSET_STALL_GPU_M0_CHECK);
	writel(0x00000000, io_addr_gpu_stall+OFFSET_STALL_GPU_M1_CHECK);
	writel(0x00000000, io_addr_gpu_stall+OFFSET_STALL_GPU_M0_EMI_CHECK);
	writel(0x00000000, io_addr_gpu_stall+OFFSET_STALL_GPU_M1_EMI_CHECK);
}

noinline void GPU_STALL_RAW(void)
{
	unsigned int	stall_counters[4];
	char		*SOB, *EOB;

	stall_counters[0] = (unsigned int)readl(io_addr_gpu_stall+OFFSET_STALL_GPU_M0_CHECK);
	stall_counters[1] = (unsigned int)readl(io_addr_gpu_stall+OFFSET_STALL_GPU_M1_CHECK);
	stall_counters[2] = (unsigned int)readl(io_addr_gpu_stall+OFFSET_STALL_GPU_M0_EMI_CHECK);
	stall_counters[3] = (unsigned int)readl(io_addr_gpu_stall+OFFSET_STALL_GPU_M1_EMI_CHECK);

	MET_TRACE_GETBUF(&SOB, &EOB);
	EOB = ms_formatH(EOB, ARRAY_SIZE(stall_counters), stall_counters);
	MET_TRACE_PUTBUF(SOB, EOB);
}

static void gpu_stall_timed_polling(unsigned long long stamp, int cpu)
{
	GPU_STALL_RAW();
}

#ifdef GPU_STALL_CNT_SINGLE
static char g_pComGPUStallHeader[] =
	"met-info [000] 0.0: met_gpu_stall_header: M0_WR,M0_RD,M1_WR,M1_RD\n";
#else
static char g_pComGPUStallHeader[] =
	"met-info [000] 0.0: met_gpu_stall_header: M0_STATUS_1,M1_STATUS_1,M0_STATUS_2,M1_STATUS_2\n";
#endif
static int gpu_stall_print_header(char *buf, int len)
{
	return SNPRINTF(buf, PAGE_SIZE, g_pComGPUStallHeader);
}

struct metdevice met_gpu_stall = {
	.name			= "gpu-stall",
	.owner			= THIS_MODULE,
	.type			= MET_TYPE_BUS,
	.cpu_related		= 0,
	.create_subfs		= gpu_stall_create_subfs,
	.delete_subfs		= gpu_stall_delete_subfs,
	.start			= gpu_stall_start,
	.stop			= gpu_stall_stop,
	.mode			= 0,
	.polling_interval	= 1,	/* ms */
	.timed_polling		= gpu_stall_timed_polling,
	.print_header		= gpu_stall_print_header,
};
#endif	/* MET_GPU_STALL_MONITOR */
