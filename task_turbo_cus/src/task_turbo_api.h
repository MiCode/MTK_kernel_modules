// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/tracepoint.h>

#define INVALID_TGID -1
#define INVALID_LOADING -1

/* struct define */ 
struct list_head;

struct win_info {
	int tgid;
	struct list_head list;
};

int init_tt_vip(void);

/* hook */
int tt_vip_algo(int ct_vip_qualified, int inputDispatcher_tgid, int systemui_tgid, int surfaceFlinger_tgid, bool touching);
extern int (*tt_vip_algo_hook)(int, int, int, int, bool);
int disable_tt_vip(u64 enforced_qualified_mask);
extern int (*disable_tt_vip_hook)(u64);
int wi_add_tgid(int);
extern int (*wi_add_tgid_hook)(int);
int wi_del_tgid(int);
extern int (*wi_del_tgid_hook)(int);
int is_target_found(const char* comm, int target);
extern int (*is_target_found_hook)(const char*, int);
extern void (*set_td_hook)(int td);
extern void (*unset_td_hook)(int td);
extern int (*set_tgd_hook)(int tgd);
extern int (*unset_tgd_hook)(int tgd);
extern void (*turn_on_tgd_hook)(void);
extern void (*turn_off_tgd_hook)(void);
void set_tid_tgid_vip(int);
extern void (*set_tdtgd_hook)(int);
void unset_tid_tgid_vip(int);
extern void (*unset_tdtgd_hook)(int);
void binder_start_vip_inherit(int to_pid, int inherited_vip_prio);
extern void (*binder_start_vip_inherit_hook)(int, int);
void binder_stop_vip_inherit(int pid, int inherited_vip_prio);
extern void (*binder_stop_vip_inherit_hook)(int, int);

/* extern func */
extern void set_task_basic_vip(int pid);
extern void unset_task_basic_vip(int pid);
extern int set_tgid_vip(int tgid);
extern int unset_tgid_vip(int tgid);
extern void turn_on_tgid_vip(void);
extern void turn_off_tgid_vip(void);
extern int get_cam_hal_pid_for_task_turbo(void);
extern int get_cam_server_pid_for_task_turbo(void);
extern bool get_cam_status_for_task_turbo(void);
extern void exp_trace_turbo_vip(const char *desc, int pid);
extern void set_task_basic_vip_and_throttle(int pid, unsigned int throttle_time);
extern void set_task_priority_based_vip_and_throttle(int pid, int prio, unsigned int throttle_time);
extern void unset_task_priority_based_vip(int pid);
extern void set_task_vvip_and_throttle(int pid, unsigned int desired_throttle_time);
extern void unset_task_vvip(int pid);
