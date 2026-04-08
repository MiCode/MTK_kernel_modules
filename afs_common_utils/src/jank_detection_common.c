// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include "jank_detection_common.h"
#include "jank_detection_common_core.h"
#include "jank_detection_utils.h"
#include <linux/tracepoint.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <trace/events/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/stdarg.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/jiffies.h>


#define MAX_CALLBACKS 100
extern int (*register_jank_ux_callback_fp)(ux_heavy_fp cb);
extern int (*unregister_jank_ux_callback_fp)(ux_heavy_fp cb);
extern void (*enable_ux_jank_detection_fp)(bool enable, const char *info, int tgid, int pid);
extern void (*sbe_frame_hint_fp)(int frame_start, int perf_index, int capacity_area, int buffer_count, unsigned long long frame_id);
extern int (*fpsgo2jank_detection_register_callback_fp)(heavy_fp cb);
extern int (*fpsgo2jank_detection_unregister_callback_fp)(heavy_fp cb);
extern void fbt_set_per_task_cap(int pid, unsigned int min_blc, unsigned int max_blc, unsigned int max_util);
extern int fbt_set_affinity(pid_t pid, unsigned int prefer_type);

static tracepoint_fp tracepoint_callbacks[MAX_CALLBACKS] = {NULL};
static heavy_fp heavy_callbacks[MAX_CALLBACKS] = {NULL};
static ux_heavy_fp jank_ux_callbacks[MAX_CALLBACKS] = {NULL};
static struct kobject *jank_detection_kobject;
static struct tracepoint *ktp = NULL;
static struct tracepoint *ktp_exit = NULL;
static unsigned int features = 0;
static const char *ux_info = NULL;
static bool feature_status = false;

static int ux_perf_index = 0;
static int ux_capacity_area = 0;
static int ux_buffer_count = 0;
static int detect_tgid = -1;
static int detect_pid = -1;
static unsigned long long ux_frame_id = 0;
static ktime_t frame_start_time;
static ktime_t frame_end_time;
static int frame_length = 0;
static int collect_signal = 0;
static int predict_signal = 0;
static int forcepull_signal = 0;
static int update_signal = 0;
static int boost_signal = 0;
static int start_frame_id = 0;
static int end_frame_id = 0;
static int pool_state = 0;
static int thread_state = 0;
static int is_target_device = 0;
static int is_target_app = 0;
static int feature_state = 0;
static int num_exit_worker = 0;

// static int run_small_core = 0;
static int target_device_only = 0;
static int target_app_only = 1;
// module_param(run_small_core, int, 0644);
module_param(target_device_only, int, 0644);
module_param(target_app_only, int, 0644);

static DEFINE_MUTEX(pool_mutex);
static DEFINE_MUTEX(thread_mutex);

static struct hrtimer timer;
static struct hrtimer timer_forcepull;
static ktime_t kt;
static ktime_t kt_forcepull;
struct task_info {
    struct task_struct *task;
    struct completion work_done;
};
static struct task_info updata_task_info;
static struct task_info predict_task_info;
static struct task_info forcepull_task_info;

void (*collect_time_init_fp)(void);
EXPORT_SYMBOL(collect_time_init_fp);
void (*update_tree_fp)(int capacity_area, int frame_length);
EXPORT_SYMBOL(update_tree_fp);
void (*predict_tree_fp)(int pref_index, int buffer_count, unsigned long long frame_id);
EXPORT_SYMBOL(predict_tree_fp);
void (*init_tree_fp)(int detect_pid);
EXPORT_SYMBOL(init_tree_fp);
void (*free_tree_fp)(void);
EXPORT_SYMBOL(free_tree_fp);
void (*forcepull_fp)(void);
EXPORT_SYMBOL(forcepull_fp);

static struct workqueue_struct *process_exit_wq;
void process_exit_worker(struct work_struct *work) {
  if (feature_state == -1) return;
  unsigned long timeout_ms = jiffies + msecs_to_jiffies(60000);
  feature_state = -1;
  detect_tgid = -1;
  detect_pid = -1;
  if (pool_state == 0) {
    feature_state = 0;
  } else if (num_exit_worker == 0) {
    // pool_state == 1 or pool_state == -1
    num_exit_worker = 1;
    while (time_before(jiffies, timeout_ms)) {
      if (pool_state == 1) {
        if (mutex_trylock(&pool_mutex)) {
          free_tree_fp();
          mutex_unlock(&pool_mutex);
        }
      }
      if (pool_state == 0) {
        feature_state = 0;
        num_exit_worker = 0;
        return;
      }
      msleep(10);
    }
  }
}
static DECLARE_WORK(process_exit_work, process_exit_worker);

void start_jank_detection_thread(void);
void stop_jank_detection_thread(void);

static noinline int tracing_mark_write(const char *buf) {
  trace_printk(buf);
  return 0;
}

void fpsgo_systrace_c(pid_t pid, unsigned long long bufID, int val, const char *fmt, ...){
  char log[256];
  va_list args;
  int len;
  char buf2[256];

  memset(log, ' ', sizeof(log));
  va_start(args, fmt);
  len = vsnprintf(log, sizeof(log), fmt, args);
  va_end(args);

  if (unlikely(len < 0))
    return;
  else if (unlikely(len == 256))
    log[255] = '\0';

  if (!bufID) {
    len = snprintf(buf2, sizeof(buf2), "C|%d|%s|%d\n", pid, log, val);
  } else {
    len = snprintf(buf2, sizeof(buf2), "C|%d|%s|%d|0x%llx\n", pid, log, val, bufID);
  }
  if (unlikely(len < 0))
    return;
  else if (unlikely(len == 256))
    buf2[255] = '\0';

  tracing_mark_write(buf2);
}

typedef struct {
  unsigned int entry_num;
  unsigned int data[100];
} DEVINFO_S;

static unsigned int getSegCode(void) {
  struct device_node *np;
  const void *prop;
  DEVINFO_S devinfo;

  np = of_find_node_by_path("/chosen");
  if (!np) {
      pr_info("[JD] of_find_node_by_path failed!\n");
      return 0;
  }

  prop = of_get_property(np, "atag,devinfo", NULL);
  if (!prop) {
      pr_info("[JD] of_get_property failed!\n");
      of_node_put(np);
      return 0;
  }

  memcpy(&devinfo, prop, sizeof(DEVINFO_S));
  of_node_put(np);
  return (devinfo.data[3] >> 2) & 0x05;
}

void receive_jank_ux_detection(int jank, int tgid, unsigned long long frame_id) {
  fpsgo_systrace_c(0, 0, boost_signal, "[JD] receive_jank");
  boost_signal = ~boost_signal;
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (jank_ux_callbacks[i] != NULL) {
      jank_ux_callbacks[i](jank, tgid, detect_pid, frame_id);
    }
  }
}
EXPORT_SYMBOL(receive_jank_ux_detection);

void receive_pool_state(int state) {
  pool_state = state;
  pr_info("[JD] receive_pool_state: %d", state);
}
EXPORT_SYMBOL(receive_pool_state);

int register_heavy_callback(heavy_fp cb) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (heavy_callbacks[i] == NULL) {
      heavy_callbacks[i] = cb;
      return 0;
    }
  }
  return -1;
}
EXPORT_SYMBOL(register_heavy_callback);

int unregister_heavy_callback(heavy_fp cb) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (heavy_callbacks[i] == cb) {
      heavy_callbacks[i] = NULL;
      return 0;
    }
  }
  return -1;
}
EXPORT_SYMBOL(unregister_heavy_callback);

void receive_jank_detection(int jank, int pid) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (heavy_callbacks[i] != NULL) {
      heavy_callbacks[i](jank, pid);
    }
  }
}
EXPORT_SYMBOL(receive_jank_detection);

int is_feature_enabled(unsigned int feature) {
    return (features & feature) != 0;
}
EXPORT_SYMBOL(is_feature_enabled);

int register_tracepoint_callback(tracepoint_fp cb) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (tracepoint_callbacks[i] == NULL) {
      tracepoint_callbacks[i] = cb;
      return 0;
    }
}
  return -1;
}
EXPORT_SYMBOL(register_tracepoint_callback);

int unregister_tracepoint_callback(tracepoint_fp cb) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (tracepoint_callbacks[i] == cb) {
      tracepoint_callbacks[i] = NULL;
      return 0;
    }
  }
  return -1;
}
EXPORT_SYMBOL(unregister_tracepoint_callback);

const char* get_ux_info(void) {
  return ux_info;
}
EXPORT_SYMBOL(get_ux_info);

static ssize_t jank_detection_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

void enable_ux_jank_detection(bool enable, const char *info, int tgid, int pid) {
  if (feature_state == -1) return;
  if (is_target_device != 1 && target_device_only == 1) return;

  if(!features){
    if (ux_info) {
      kfree(ux_info);
      ux_info = NULL;
    }
    ux_info = kstrdup(info, GFP_KERNEL);
  }
  if (enable) {
    jank_detection_store(NULL, NULL, "0x12", 0);
    fpsgo_systrace_c(0, 0, tgid, "[JD] TGID");
    if (detect_tgid != tgid) {
      pr_info("[JD] change tgid from %d to %d\n", detect_tgid, tgid);
      feature_state = 0;
      if (detect_tgid != -1) {
        if (pool_state == 1) {
          if (mutex_trylock(&pool_mutex)) {
            free_tree_fp();
            mutex_unlock(&pool_mutex);
          } else return;
        }
        detect_tgid = -1;
        detect_pid = -1;
      }

      if (target_app_only == 1) {
        is_target_app = 0;
        if (strcmp(info, "com.sina.weibo") == 0) is_target_app = 1;
        if (strcmp(info, "droid.ugc.aweme") == 0) is_target_app = 1;
        if (is_target_app == 0) return;
      }
      pr_info("[JD] enable_ux_jank_detection, enable:%d, tgid:%d",enable, tgid);

      if (pool_state == 0) {
        if (mutex_trylock(&pool_mutex)) {
          init_tree_fp(tgid);
          mutex_unlock(&pool_mutex);
        } else return;
      }
      if (pool_state != 1) return;
      detect_tgid = tgid;
      detect_pid = pid;
      feature_state = 1;
    }
  } else {
    jank_detection_store(NULL, NULL, "0x02", 0);
  }
}
EXPORT_SYMBOL(enable_ux_jank_detection);

static void tracepoint_callback_func(void *p, struct pt_regs *regs, long id) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (tracepoint_callbacks[i] != NULL) {
        tracepoint_callbacks[i](p, regs->regs, id);
    }
  }
}

static void find_sys_enter_tracepoint(struct tracepoint *tp, void *priv) {
  if (strcmp(tp->name, "sys_enter") == 0) {
    ktp = tp;
  }
}

static int register_syscall(void) {
  int ret;

  if (!ktp) {
    return -ENODEV;
  }

  ret = tracepoint_probe_register(ktp, tracepoint_callback_func, NULL);
  if (ret) {
    return ret;
  }

  return 0;
}

static void unregister_syscall(void) {
  tracepoint_probe_unregister(ktp, tracepoint_callback_func, NULL);
}

static void find_process_exit_tracepoint(struct tracepoint *tp, void *priv) {
  if (strcmp(tp->name, "sched_process_exit") == 0) {
    ktp_exit = tp;
  }
}

void process_exit_callback(void *ignore, struct task_struct *task) {
  if (task->pid == detect_tgid) {
    queue_work(process_exit_wq, &process_exit_work);
  }
}

static void register_process_exit(void) {
  int ret;

  if (!ktp_exit) {
    return ;
  }

  ret = tracepoint_probe_register(ktp_exit, process_exit_callback, NULL);
}

static void unregister_process_exit(void) {
  tracepoint_probe_unregister(ktp_exit, process_exit_callback, NULL);
}

static int update_thread(void *data) {
  struct task_info *info = (struct task_info*)data;
  long thread_timeout_ms = msecs_to_jiffies(60000);
  long ret;
  // if (run_small_core == 1) {
  //   int update_pid = hd_task_pid_nr();
  //   fbt_set_affinity(update_pid, 2);
  //   fbt_set_per_task_cap(update_pid, 1, 1, 1024);
  // }
  while(!kthread_should_stop()) {
    ret = wait_for_completion_interruptible_timeout(&info->work_done, thread_timeout_ms);
    if (ret <= 0) {
      if (kthread_should_stop()) {
        reinit_completion(&info->work_done);
        break;
      }
      continue;
    }

    if (feature_state == 1) {
      if (mutex_trylock(&pool_mutex)) {
        if (mutex_trylock(&thread_mutex)) {
          update_tree_fp(ux_capacity_area, frame_length);
          mutex_unlock(&thread_mutex);
        }
        mutex_unlock(&pool_mutex);
      }
    }
    reinit_completion(&info->work_done);
  }
  return 0;
}

static int predict_thread(void *data) {
  struct task_info *info = (struct task_info*)data;
  long thread_timeout_ms = msecs_to_jiffies(60000);
  long ret;
  // if (run_small_core == 1) {
  //   int predict_pid = hd_task_pid_nr();
  //   fbt_set_affinity(predict_pid, 2);
  //   fbt_set_per_task_cap(predict_pid, 1, 1, 1024);
  // }
  while(!kthread_should_stop()) {
    ret = wait_for_completion_interruptible_timeout(&info->work_done, thread_timeout_ms);
    if (ret <= 0) {
      if (kthread_should_stop()) {
        reinit_completion(&info->work_done);
        break;
      }
      continue;
    }

    if (feature_state == 1) {
      if (mutex_trylock(&pool_mutex)) {
        if (mutex_trylock(&thread_mutex)) {
          predict_tree_fp(ux_perf_index, ux_buffer_count, ux_frame_id);
          mutex_unlock(&thread_mutex);
        }
        mutex_unlock(&pool_mutex);
      }
    }
    reinit_completion(&info->work_done);
  }
  return 0;
}

static int forcepull_thread(void *data) {
  struct task_info *info = (struct task_info*)data;
  long thread_timeout_ms = msecs_to_jiffies(60000);
  long ret;
  while(!kthread_should_stop()) {
    ret = wait_for_completion_interruptible_timeout(&info->work_done, thread_timeout_ms);
    if (ret <= 0) {
      if (kthread_should_stop()) {
        reinit_completion(&info->work_done);
        break;
      }
      continue;
    }

    if (feature_state == 1) {
      if (mutex_trylock(&thread_mutex)) {
        forcepull_fp();
        mutex_unlock(&thread_mutex);
      }
    }
    reinit_completion(&info->work_done);
  }
  return 0;
}

void start_jank_detection_thread(void) {
  if (thread_state == 0) {
    mutex_lock(&thread_mutex);
    thread_state = -1;
    pr_info("[JD] start_jank_detection_thread start\n");
    init_completion(&updata_task_info.work_done);
    init_completion(&predict_task_info.work_done);
    init_completion(&forcepull_task_info.work_done);
    updata_task_info.task = kthread_run(update_thread, &updata_task_info, "jank_detection_update_tree_thread");
    predict_task_info.task = kthread_run(predict_thread, &predict_task_info, "jank_detection_predict_tree_thread");
    forcepull_task_info.task = kthread_run(forcepull_thread, &forcepull_task_info, "jank_detection_forcepull_tree_thread");
    thread_state = 1;
    mutex_unlock(&thread_mutex);
    pr_info("[JD] start_jank_detection_thread finish\n");
  }
}

void stop_jank_detection_thread(void) {
  if (thread_state == 1) {
    mutex_lock(&thread_mutex);
    thread_state = -1;
    pr_info("[JD] stop_jank_detection_thread start\n");
    if (updata_task_info.task) kthread_stop(updata_task_info.task);
    if (predict_task_info.task) kthread_stop(predict_task_info.task);
    if (forcepull_task_info.task) kthread_stop(forcepull_task_info.task);
    thread_state = 0;
    mutex_unlock(&thread_mutex);
    pr_info("[JD] stop_jank_detection_thread finish\n");
  }
}

enum hrtimer_restart timer_callback(struct hrtimer *timer) {
  fpsgo_systrace_c(0, 0, predict_signal, "[JD] timer_callback");
  predict_signal = ~predict_signal;
  if (thread_state == 1) complete(&predict_task_info.work_done);
  return HRTIMER_NORESTART;
}

enum hrtimer_restart timer_forcepull_callback(struct hrtimer *timer) {
  if (start_frame_id != end_frame_id){
    fpsgo_systrace_c(0, 0, forcepull_signal, "[JD] timer_forcepull_callback");
    forcepull_signal = ~forcepull_signal;
    if (thread_state == 1) complete(&forcepull_task_info.work_done);
  }
  return HRTIMER_NORESTART;
}

int register_jank_ux_callback(ux_heavy_fp cb) {
  register_process_exit();
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (jank_ux_callbacks[i] == NULL) {
      jank_ux_callbacks[i] = cb;
      return 0;
    }
  }
  return -1;
}
EXPORT_SYMBOL(register_jank_ux_callback);

int unregister_jank_ux_callback(ux_heavy_fp cb) {
  unregister_process_exit();
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (jank_ux_callbacks[i] == cb) {
      jank_ux_callbacks[i] = NULL;
      return 0;
    }
  }
  return -1;
}
EXPORT_SYMBOL(unregister_jank_ux_callback);

static int jank_detection_enable(void) {
  register_syscall();
  return 0;
}

static void jank_detection_disable(void) {
  //stop_jank_detection_thread();
  unregister_syscall();
}

void sbe_frame_hint(int frame_start, int perf_index, int capacity_area, int buffer_count, unsigned long long frame_id) {
  if (feature_state == -1) return;
  if (detect_tgid == -1) return;

  if (frame_start) {
    ux_perf_index = perf_index;
    ux_buffer_count = buffer_count;
    ux_frame_id = frame_id;
    frame_start_time = ktime_get();
    fpsgo_systrace_c(0, 0, collect_signal, "[JD] sbe_frame_hint/collect_time");
    collect_signal = ~collect_signal;
    if (features & FEATURE_OFF) {
      return;
    }
    start_frame_id = ~start_frame_id;
    if (thread_state == 1) {
      if (mutex_trylock(&thread_mutex)) {
        collect_time_init_fp();
        hrtimer_start(&timer, kt, HRTIMER_MODE_REL);
        hrtimer_start(&timer_forcepull, kt_forcepull, HRTIMER_MODE_REL);
        mutex_unlock(&thread_mutex);
      }
    }
  } else {
    ux_capacity_area = capacity_area *100 / 1024;
    frame_end_time = ktime_get();
    frame_length = (int)ktime_to_ns(ktime_sub(frame_end_time, frame_start_time))/1000;
    fpsgo_systrace_c(0, 0, update_signal, "[JD] sbe_frame_hint/update_task");
    update_signal = ~update_signal;
    if (features & FEATURE_OFF) {
      return;
    }
    end_frame_id = start_frame_id;
    if (thread_state == 1) complete(&updata_task_info.work_done);
  }
}

static ssize_t jank_detection_show(struct kobject *kobj,
                                    struct kobj_attribute *attr, char *buf) {
  return snprintf(buf, 2048, "%d\n", features);
}

static ssize_t jank_detection_store(struct kobject *kobj,
                                     struct kobj_attribute *attr,
                                     const char *buf, size_t count) {
  int ret;
  int value;
  int enable;
  unsigned int feature_code;

  ret = kstrtoint(buf, 0, &value);
  if (ret < 0)
    return ret;

  enable = (value & 0x10) >> 4;
  feature_code = value & 0x0F;

  if (enable) {
      switch (feature_code) {
          case FEATURE_FPSGO:
              features |= FEATURE_FPSGO;
              break;
          case FEATURE_SBE:
              features |= FEATURE_SBE;
              break;
          case FEATURE_OFF:
              features |= FEATURE_OFF;
              break;
      }
  } else {
      switch (feature_code) {
          case FEATURE_FPSGO:
              features &= ~FEATURE_FPSGO;
              break;
          case FEATURE_SBE:
              features &= ~FEATURE_SBE;
              break;
          case FEATURE_OFF:
              features &= ~FEATURE_OFF;
              break;
      }
  }

  if (!feature_status && !(features & FEATURE_OFF) && ((features & FEATURE_FPSGO) || (features & FEATURE_SBE))) {
    ret = jank_detection_enable();
    if (ret < 0)
      return ret;
    feature_status = true;
  } else if (feature_status && (!features | (features & FEATURE_OFF))) {
    jank_detection_disable();
    feature_status = false;
  }

  return count;
}

struct kobj_attribute jank_detection_attribute =
    __ATTR(jank_detection, 0664, jank_detection_show, jank_detection_store);

static int __init jank_detection_init(void) {
  int error = 0;

  jank_detection_kobject =
      kobject_create_and_add("jank_detection", kernel_kobj);
  if (!jank_detection_kobject)
    return -ENOMEM;

  error = sysfs_create_file(jank_detection_kobject,
                            &jank_detection_attribute.attr);

  register_jank_ux_callback_fp = register_jank_ux_callback;
  unregister_jank_ux_callback_fp = unregister_jank_ux_callback;
  enable_ux_jank_detection_fp = enable_ux_jank_detection;
  sbe_frame_hint_fp = sbe_frame_hint;

  fpsgo2jank_detection_register_callback_fp = register_heavy_callback;
  fpsgo2jank_detection_unregister_callback_fp = unregister_heavy_callback;

  if (feature_state == 0) start_jank_detection_thread();
  hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  hrtimer_init(&timer_forcepull, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  timer.function = timer_callback;
  timer_forcepull.function = timer_forcepull_callback;
  kt = ktime_set(0, 2000*1000);
  kt_forcepull = ktime_set(0, 7000*1000);
  for_each_kernel_tracepoint(find_sys_enter_tracepoint, NULL);
  for_each_kernel_tracepoint(find_process_exit_tracepoint, NULL);
  is_target_device = getSegCode();

  process_exit_wq = create_singlethread_workqueue("process_exit_wq");
  if (!process_exit_wq) return -ENOMEM;

  return error;
}

static void __exit jank_detection_exit(void) {
  kobject_put(jank_detection_kobject);
  destroy_workqueue(process_exit_wq);
}

module_init(jank_detection_init);
module_exit(jank_detection_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek jank Detection");
MODULE_AUTHOR("MediaTek Inc.");
