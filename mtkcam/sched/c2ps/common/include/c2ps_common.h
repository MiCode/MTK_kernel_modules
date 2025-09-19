/* SPDX-License-Identifier: GPL-2.0 */
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
#include <linux/hashtable.h>
#include <uapi/linux/sched/types.h>
#include <linux/version.h>
#include <linux/pm_qos.h>

#define MAX_WINDOW_SIZE 70
#define MAX_CPU_NUM CONFIG_MAX_NR_CPUS
// ms
#define BACKGROUND_MONITOR_DURATION 33
#define MAX_NUMBER_OF_CLUSTERS CONFIG_MAX_NR_CPUS
#define MAX_TASK_NAME_SIZE 10
#define MAX_UCLAMP 1024
#define MIN_UCLAMP_MARGIN 50
#define MAX_CRITICAL_TASKS 20
#define MAX_DEP_THREAD_NUM 5
#define MAX_DEP_THREAD_SEARCH_COUNT 10
// anchor kf param, use micro sec as unit scale
#define ANCHOR_QUEUE_LEN 5
#define ANC_KF_MIN_EST_ERR 100
#define ANC_KF_QVAL 50
#define ANC_KF_MEAS_ERR (5*1000)
//
#define LxF_KF_MEAS_ERR 1000
#define LxF_KF_MIN_EST_ERR 10
#define LxF_S_KF_QVAL 10
#define LxF_F_KF_QVAL 20
#define LxF_DIFF_THRES 10000

#define DEFAULT_UM_MIN 65
#define RESET_VAL 999999
#define LMCORE_UM_RATIO_MAX 20

extern int proc_time_window_size;
extern int debug_log_on;
extern unsigned int c2ps_nr_clusters;
extern bool c2ps_um_mode_on;
extern int c2ps_regulator_base_update_um;
extern int c2ps_regulator_um_min;
extern int c2ps_lcore_mcore_um_ratio;

enum c2ps_env_status : int {
	C2PS_STAT_NODEF = 0,
	C2PS_STAT_STABLE,
	C2PS_STAT_TRANSIENT,
};

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
	bool vip_set_by_monitor;
	bool is_enable_dep_thread;
	// dependency thread
	pid_t dep_thread[MAX_DEP_THREAD_NUM];
	int dep_thread_search_count;

	/**
	* update by regulator
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
	unsigned int s_idle;
	unsigned int l_idle;
	u64 s_wall_time;
	u64 s_idle_time;
	u64 l_wall_time;
	u64 l_idle_time;
	u64 counter;
};

struct kf_est {
	int64_t est_val;
	int64_t est_err;
	int64_t min_est_err;
	int64_t q_val;
	int64_t meas_err;
};

struct um_update_vote {
	u32 total_voter_mul;
	u32 curr_voter_mul;
	bool last_anchor_decided;
	// vote_result: -1:reduce um, 0:netual, 1:increase um
	int vote_result;
};

struct um_table_item {
	u8 um;
	u64 latency;
	u64 end_diff;
	u64 jitter;
	u64 jitter_total_access;
	u64 jitter_hit_cnt;
	u64 um_stay_cnt;
	struct kf_est lat_est;
	struct kf_est end_diff_est;
	struct kf_est jit_est;
	struct hlist_node hlist;
};

struct c2ps_anchor {
	int anchor_id;
	u32 latency_spec;
	u32 jitter_spec;
	bool is_last_anchor;
	u8 s_idx;
	u8 e_idx;
	u64 s_hist_t[ANCHOR_QUEUE_LEN];
	u64 e_hist_t[ANCHOR_QUEUE_LEN];
	// latest info
	u64 latest_duration;
	u64 latest_to_fixed_start_duration;
	// stable um table
	struct hlist_head um_table_stbl[8];
	// transient um table
	struct hlist_node hlist;
};

struct global_info {
	int cfg_camfps;
	int max_uclamp[MAX_NUMBER_OF_CLUSTERS];
	int camfps;
	u64 vsync_time;
	/******** cpu idle rate related ********/
	struct per_cpu_idle_rate cpu_idle_rates[MAX_CPU_NUM];
	int avg_cluster_idle_rate[MAX_CPU_NUM];
	int last_sum_idle_rate;
	// TODO(MTK): check if this can be simplified
	u64 s_loadxfreq[MAX_CPU_NUM];
	u64 l_loadxfreq[MAX_CPU_NUM];
	struct kf_est slow_lxf_est[MAX_CPU_NUM];
	struct kf_est fast_lxf_est[MAX_CPU_NUM];
	// CPU floor frequency of the scenario
	u32 scn_cpu_freq_floor[MAX_NUMBER_OF_CLUSTERS];
	u32 possible_config_cpu_freq[MAX_NUMBER_OF_CLUSTERS];
	bool is_cpu_boost;

	/**
	 * need_update_bg definition:
	 * [if any needs update,
	 *  LCore needs update, MCore needs update, BCore needs update]
	 *
	 *  set 2: enter dangerous idle rate status, release uclamp max/um
	 *  set 1: need to increase uclamp/um
	 *  set 0: no need to modify uclamp/um
	 *  set -1: be able to decrease uclamp/um
	 *  set -2: decrease uclamp/um faster
	 */
	int need_update_bg[1 + MAX_NUMBER_OF_CLUSTERS];
	/******** per-gear uclamp max related ********/
	int curr_max_uclamp[MAX_NUMBER_OF_CLUSTERS];
	bool use_uclamp_max_floor;
	int special_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int recovery_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int overwrite_uclamp_max[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder1[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder2[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_placeholder3[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_floor[MAX_NUMBER_OF_CLUSTERS];
	int uclamp_max_ceiling[MAX_NUMBER_OF_CLUSTERS];
	int overwrite_idle_alert;
	/******** QoS related ********/
	int ineff_cpu_freq[MAX_NUMBER_OF_CLUSTERS];
	struct freq_qos_request qos_req[MAX_NUMBER_OF_CLUSTERS];
	/********  um related ********/
	// TODO(MTK): anc_fixed functionality is not ready
	// struct c2ps_anchor *anc_fixed;

	// um setting when spec. provided
	int curr_um;
	// um setting for idle rate control
	int curr_um_idle;
	struct um_update_vote um_vote;
	/******** single shot um related ********/
	u32 overwrite_util_margin;
	u32 decided_um_placeholder_val;
	u32 um_placeholder1;
	u32 um_placeholder2;
	u32 um_placeholder3;
	enum c2ps_env_status stat;
	bool has_anchor_spec;
	u32 single_shot_enable_ineff_cpufreq_cnt;
	bool switch_um_idle_rate_mode;
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
	struct c2ps_anchor *anc_info;
	struct list_head queue_list;
	enum c2ps_env_status stat;
	bool is_flush;
	int curr_um;
};

#define C2PS_LOGD(fmt, ...)                                                 \
	do {                                                                    \
		if (unlikely(debug_log_on)) {                                       \
			switch (debug_log_on) {                                         \
			case 1:                                                         \
				pr_debug("[C2PS]: %s " fmt, __func__, ##__VA_ARGS__);       \
				break;                                                      \
			case 2:                                                         \
				pr_warn("[C2PS]: %s " fmt, __func__, ##__VA_ARGS__);        \
				break;                                                      \
			default:                                                        \
				break;                                                      \
			}                                                               \
		}                                                                   \
	} while (0)

#define C2PS_LOGW(fmt, ...)                                         \
	do {                                                            \
		if (unlikely(debug_log_on))                                 \
			pr_warn("[C2PS]: %s " fmt, __func__, ##__VA_ARGS__);    \
	} while (0)

#define C2PS_LOGW_ONCE(fmt, ...) pr_warn_once("[C2PS]: %s %s %d " fmt, \
	__FILE__, __func__, __LINE__, ##__VA_ARGS__)

#define C2PS_LOGE(fmt, ...) pr_err("[C2PS]: %s %s %d " fmt, \
	__FILE__, __func__, __LINE__, ##__VA_ARGS__)

/* c2ps_main */
int c2ps_notify_init(
	int cfg_camfps, int max_uclamp_cluster0, int max_uclamp_cluster1,
	int max_uclamp_cluster2, int ineff_cpu_ceiling_freq0,
	int ineff_cpu_ceiling_freq1, int ineff_cpu_ceiling_freq2,
	int lcore_mcore_um_ratio, int um_floor);

int init_c2ps_common(int cfg_camfps);
void exit_c2ps_common(void);
int set_curr_uclamp_hint(int pid, int set);
int set_curr_uclamp_hint_wo_lock(struct task_struct *p, int set);
struct c2ps_task_info *c2ps_find_task_info_by_tskid(int task_id);
int c2ps_add_task_info(struct c2ps_task_info *tsk_info);
void c2ps_clear_task_info_table(void);
struct c2ps_anchor *c2ps_find_anchor_by_id(int anc_id);
int c2ps_add_anchor(struct c2ps_anchor *anc_info);
void c2ps_clear_anchor_table(void);
u64 c2ps_task_sched_runtime(struct task_struct *p);
u64 c2ps_get_sum_exec_runtime(int pid);
struct task_struct *c2ps_find_waker_task(struct task_struct *cur_task);
int c2ps_find_waker_pid(int cur_task_pid);
void c2ps_add_waker_pid_to_task_info(struct c2ps_task_info *tsk_info);
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
void c2ps_anchor_tbl_lock(void);
void c2ps_anchor_tbl_unlock(void);
struct um_table_item *c2ps_find_um_table_by_um(struct c2ps_anchor *anc, int um);
int c2ps_add_um_table(struct c2ps_anchor *anc, struct um_table_item *table);
void c2ps_init_kf(
	struct kf_est *kf, int64_t q_val, int64_t meas_err, int64_t min_est_err);
u64 c2ps_cal_kf_est(struct kf_est *kf, u64 cur_obs);
void c2ps_update_um_table(struct c2ps_anchor *anc);
void c2ps_check_last_anc(struct c2ps_anchor *anc);
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
void c2ps_bg_info_um_systrace(const char *fmt, ...);
void c2ps_bg_info_um_default_systrace(const char *fmt, ...);
void c2ps_main_systrace(const char *fmt, ...);
void c2ps_critical_task_systrace(struct c2ps_task_info *tsk_info);
void *c2ps_alloc_atomic(int i32Size);
void c2ps_free(void *pvBuf, int i32Size);
void set_glb_info_bg_uclamp_max(void);
void update_cpu_idle_rate(void);
bool need_update_background(void);
void reset_need_update_status(void);
void set_eas_setting(void);
void reset_eas_setting(void);
unsigned long c2ps_get_um_virtual_ceiling(int cpu, unsigned int um);
unsigned long c2ps_get_uclamp_freq(int cpu,  unsigned int uclamp);
bool c2ps_get_cur_cpu_floor_uclamp(const int cpu, int *floor_uclamp, int floor_freq);
u32 c2ps_get_cur_cpu_freq_floor(const int cpu);
u32 c2ps_get_cur_cpu_freq(const int cpu);
int c2ps_get_cpu_min_uclamp(const int cpu);
int c2ps_get_cpu_max_uclamp(const int cpu);
bool c2ps_boost_cur_uclamp_max(
	const int cluster, int cpu_floor_freq, struct global_info *g_info);
int c2ps_get_first_cpu_of_cluster(int cluster);
int c2ps_get_nr_cpus_of_cluster(int cluster);
unsigned long c2ps_get_cluster_uclamp_freq(int cluster,  unsigned int uclamp);
bool need_update_single_shot_uclamp_max(int *uclamp_max);
bool need_update_critical_task_uclamp(int *critical_task_uclamp);
void c2ps_set_util_margin(int cluster, int um);
void c2ps_set_turn_point_freq(int cluster, unsigned int freq);
void set_glb_info_bg_util_margin(void);
void c2ps_set_vip_task(int pid, int vip_prior, unsigned int vip_throttle_time);
bool is_task_vip(int pid);
void c2ps_set_ineff_cpu_freq_ceiling(int cluster, int ineff_cpu_ceiling_freq);
void c2ps_update_cpu_freq_ceiling(int cluster, int cpu_ceiling_freq);
void c2ps_reset_cpu_freq_ceiling(int cluster);
void c2ps_remove_qos_setting(void);
bool use_overwrite_uclamp_max(void);
void update_critical_task_uclamp_by_tsk_id(
	int *critical_task_ids, int *critical_task_uclamp);
void set_uclamp(const int pid, unsigned int max_util, unsigned int min_util);
void reset_task_eas_setting(struct c2ps_task_info *tsk_info);
void reset_task_uclamp(int pid);
void cache_possible_config_cpu_freq_info(void);

// EAS
extern void set_curr_uclamp_ctrl(int val);
extern void set_gear_uclamp_ctrl(int val);
extern void set_gear_uclamp_max(int gearid, int val);
extern int get_gear_uclamp_max(int gearid);
extern unsigned long pd_get_util_freq(int cpu, unsigned long util);
extern unsigned int get_nr_gears(void);
extern void set_wl_manual(int val);
extern int get_nr_wl(void);
extern unsigned int get_adaptive_margin(int cpu);
extern struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu);
extern void cpufreq_cpu_put(struct cpufreq_policy *policy);
extern unsigned long pd_get_freq_util(unsigned int cpu, unsigned long freq);
extern struct cpumask *get_gear_cpumask(unsigned int gear);
extern void set_task_ls(int pid);
extern void unset_task_ls(int pid);
extern void set_task_basic_vip(int pid);
extern void unset_task_basic_vip(int pid);
extern int set_sched_capacity_margin_dvfs(int capacity_margin);
extern int set_target_margin_low(int cpu, int margin);
extern int set_target_margin(int cpu, int margin);
extern int set_turn_point_freq(int cpu, unsigned long freq);
extern bool flt_ctrl_force_get(void);
extern void flt_ctrl_force_set(int set);
extern u32 group_get_mode(void);
extern void group_set_mode(u32 mode);
extern int get_grp_dvfs_ctrl(void);
extern void set_grp_dvfs_ctrl(int set);
extern bool get_ignore_idle_ctrl(void);
extern void set_ignore_idle_ctrl(bool val);

#ifdef NEW_C2PS_API_K66
int get_vip_task_prio_by_pid(int pid);
void c2ps_unset_vip_task(int pid);
extern void set_task_vvip_and_throttle(int pid, unsigned int throttle_time);
extern void set_task_priority_based_vip_and_throttle(
	int pid, int prio, unsigned int throttle_time);
extern void set_task_basic_vip_and_throttle(
	int pid, unsigned int throttle_time);
extern int get_vip_task_prio(struct task_struct *p);
extern bool prio_is_vip(int vip_prio, int type);
extern void unset_task_priority_based_vip(int pid);
extern void unset_task_vvip(int pid);
#endif

// QoS
extern int freq_qos_add_request(struct freq_constraints *qos,
			struct freq_qos_request *req,
			enum freq_qos_req_type type, s32 value);
extern int freq_qos_update_request(struct freq_qos_request *req, s32 new_value);
extern int freq_qos_remove_request(struct freq_qos_request *req);

#endif  // C2PS_COMMON_INCLUDE_C2PS_COMMON_H_
