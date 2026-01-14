// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#ifndef C2PS_COMMON_INCLUDE_C2PS_COMMON_H_
#define C2PS_COMMON_INCLUDE_C2PS_COMMON_H_

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/topology.h>
#include <linux/time.h>
#include <uapi/linux/sched/types.h>

#define MAX_WINDOW_SIZE 70
#define MAX_CPU_NUM CONFIG_MAX_NR_CPUS
// ms
#define BACKGROUND_MONITOR_DURATION 33
#define MAX_NUMBER_OF_CLUSTERS CONFIG_MAX_NR_CPUS
#define MAX_TASK_NAME_SIZE 10
#define MAX_UCLAMP 1024
#define MIN_UCLAMP_MARGIN 50

extern int proc_time_window_size;
extern int debug_log_on;
extern unsigned int c2ps_nr_clusters;

struct c2ps_task_info {
	u32 task_id;
	pid_t pid;
	u32 loading;
	u32 nr_hist_info;
	u32 default_uclamp;
	u64 task_target_time;
	u64 start_time;
	u64 end_time;
	u64 proc_time;
	u64 sum_exec_runtime_start;
	u64 real_exec_runtime;
	u64 hist_proc_time_sum;
	u64 hist_window_size;
	u64 hist_proc_time_avg;
	u32 hist_loading[MAX_WINDOW_SIZE];
	u64 hist_proc_time[MAX_WINDOW_SIZE];
	char task_name[MAX_TASK_NAME_SIZE];
	struct hlist_node hlist;
	struct c2ps_task_info *overlap_task;
	struct list_head task_group_list;
	struct task_group_info *tsk_group;
	struct mutex mlock;
	bool is_dynamic_tid;
	bool is_vip_task;
	bool is_active;
	bool is_scene_changed;

	/**
	* update by uclamp regulator
	*/
	unsigned int latest_uclamp;
};

struct task_group_info {
	int group_head;
	u64 group_start_time;
	u64 group_target_time;
	u64 accumulate_time;
	struct hlist_node hlist;
	struct mutex mlock;
};

struct per_cpu_idle_rate {
	unsigned int idle;
	u64 wall_time;
	u64 idle_time;
};

struct global_info {
	int cfg_camfps;
	int max_uclamp[MAX_NUMBER_OF_CLUSTERS];
	int camfps;
	u64 vsync_time;
	struct per_cpu_idle_rate cpu_idle_rates[MAX_CPU_NUM];
	int last_sum_idle_rate;
	/**
	 * need_update_uclamp definition:
	 * [if any needs update,
	 *  LCore needs update, MCore needs update, LCore needs update]
	 *
	 *  set 2: enter dangerous idle rate status, release uclamp max
	 *  set 1: need to increase uclamp
	 *  set 0: no need to modify uclamp
	 *  set -1: be able to decrease uclamp
	 *  set -2: decrease uclamp faster
	 */
	int need_update_uclamp[1 + MAX_NUMBER_OF_CLUSTERS];
	int curr_max_uclamp[MAX_NUMBER_OF_CLUSTERS];
	bool use_uclamp_max_floor;
	int special_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int recovery_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int overwrite_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder1[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder2[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder3[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_floor[MAX_NUMBER_OF_CLUSTERS];
	// due to alert 100 is used to trigger special uclamp max, we need to backup
	// idle alert value
	int backup_idle_alert;
	struct mutex mlock;
};

struct eas_settings {
	// disable flt
	bool flt_ctrl_force;
	int group_get_mode;
	int grp_dvfs_ctrl;
	// skip idle
	int ignore_idle_ctrl;
};

struct regulator_req {
	struct c2ps_task_info *tsk_info;
	struct global_info *glb_info;
	struct list_head queue_list;
	bool is_flush;
};


#define C2PS_LOGD(fmt, ...)                                         \
	do {                                                            \
		if (debug_log_on)                                           \
			pr_debug("[C2PS]: %s " fmt, __func__, ##__VA_ARGS__);   \
	} while (0)

#define C2PS_LOGW(fmt, ...)                                         \
	do {                                                            \
		if (debug_log_on)                                           \
			pr_warn("[C2PS]: %s " fmt, __func__, ##__VA_ARGS__);    \
	} while (0)

#define C2PS_LOGW_ONCE(fmt, ...) pr_warn_once("[C2PS]: %s %s %d " fmt, \
	__FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define C2PS_LOGE(fmt, ...) pr_err("[C2PS]: %s %s %d " fmt, \
	__FILE__, __func__, __LINE__, ##__VA_ARGS__)

int init_c2ps_common(void);
void exit_c2ps_common(void);
int set_curr_uclamp_hint(int pid, int set);
int set_curr_uclamp_hint_wo_lock(struct task_struct *p, int set);
struct c2ps_task_info *c2ps_find_task_info_by_tskid(int task_id);
int c2ps_add_task_info(struct c2ps_task_info *tsk_info);
void c2ps_clear_task_info_table(void);
u64 c2ps_task_sched_runtime(struct task_struct *p);
u64 c2ps_get_sum_exec_runtime(int pid);
void c2ps_task_info_tbl_lock(const char *tag);
void c2ps_task_info_tbl_unlock(const char *tag);
struct task_group_info *c2ps_find_task_group_info_by_grphd(int group_head);
int c2ps_create_task_group(int group_head, u64 task_group_target_time);
int c2ps_add_task_to_group(struct c2ps_task_info *tsk_info, int group_head);
void c2ps_clear_task_group_info_table(void);
void c2ps_task_group_info_tbl_lock(const char *tag);
void c2ps_task_group_info_tbl_unlock(const char *tag);
void c2ps_info_lock(struct mutex *mlock);
void c2ps_info_unlock(struct mutex *mlock);
u64 c2ps_get_time(void);
void c2ps_update_task_info_hist(struct c2ps_task_info *tsk_info);
struct global_info *get_glb_info(void);
void set_config_camfps(int camfps);
void decide_special_uclamp_max(int placeholder_type);
void update_vsync_time(u64 ts);
void update_camfps(int camfps);
bool is_group_head(struct c2ps_task_info *tsk_info);
void c2ps_systrace_c(pid_t pid, int val, const char *fmt, ...);
void c2ps_bg_info_systrace(const char *fmt, ...);
void c2ps_main_systrace(const char *fmt, ...);
void c2ps_critical_task_systrace(struct c2ps_task_info *tsk_info);
void *c2ps_alloc_atomic(int i32Size);
void c2ps_free(void *pvBuf, int i32Size);
void set_glb_info_bg_uclamp_max(void);
void update_cpu_idle_rate(void);
bool need_update_background(void);
void reset_need_update_status(void);
void set_heavyloading_special_setting(void);
void reset_heavyloading_special_setting(void);
unsigned long c2ps_get_uclamp_freq(int cpu,  unsigned int uclamp);
bool c2ps_get_cur_cpu_floor(const int cpu, int *floor_uclamp, int *floor_freq);
int c2ps_get_cpu_min_uclamp(const int cpu);
int c2ps_get_cpu_max_uclamp(const int cpu);
bool c2ps_boost_cur_uclamp_max(const int cluster, struct global_info *g_info);
int c2ps_get_first_cpu_of_cluster(int cluster);
unsigned long c2ps_get_cluster_uclamp_freq(int cluster,  unsigned int uclamp);
bool need_update_single_shot_uclamp_max(int *uclamp_max);


extern void set_curr_uclamp_ctrl(int val);
extern void set_gear_uclamp_ctrl(int val);
extern void set_gear_uclamp_max(int gearid, int val);
extern int get_gear_uclamp_max(int gearid);
extern unsigned long pd_get_util_freq(int cpu, unsigned long util);
extern unsigned int get_nr_gears(void);
extern void set_wl_type_manual(int val);
extern int get_nr_wl_type(void);
// extern void set_rt_aggre_preempt(int val);
extern unsigned int get_adaptive_margin(int cpu);
extern struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu);
extern void cpufreq_cpu_put(struct cpufreq_policy *policy);
extern unsigned long pd_get_freq_util(unsigned int cpu, unsigned long freq);
extern struct cpumask *get_gear_cpumask(unsigned int gear);
extern bool flt_ctrl_force_get(void);
extern void flt_ctrl_force_set(int set);
extern u32 group_get_mode(void);
extern void group_set_mode(u32 mode);
extern int get_grp_dvfs_ctrl(void);
extern void set_grp_dvfs_ctrl(int set);
extern bool get_ignore_idle_ctrl(void);
extern void set_ignore_idle_ctrl(bool val);

#endif  // C2PS_COMMON_INCLUDE_C2PS_COMMON_H_
