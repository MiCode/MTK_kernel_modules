// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/version.h>
#if (KERNEL_VERSION(6, 7, 0) >= LINUX_VERSION_CODE)
#include <linux/sched.h>
#else
#include <linux/pid.h>
#endif
#include <linux/random.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/printk.h>
#include "jank_detection_utils.h"

static spinlock_t my_lock;
static unsigned long flags;

void *hd_kmalloc(int size) { return kmalloc(size, GFP_KERNEL); }
EXPORT_SYMBOL(hd_kmalloc);

void *hd_vmalloc(int size) { return vmalloc(size); }
EXPORT_SYMBOL(hd_vmalloc);

void hd_memset(void *p, int value, int size) { memset(p, value, size); }
EXPORT_SYMBOL(hd_memset);

void hd_kfree(void *p) { kfree(p); }
EXPORT_SYMBOL(hd_kfree);

void hd_vfree(void *p) { vfree(p); }
EXPORT_SYMBOL(hd_vfree);

int hd_task_pid_nr(void) { return (int)task_pid_nr(current); }
EXPORT_SYMBOL(hd_task_pid_nr);

void hd_get_random_bytes(void *buf, int len) { get_random_bytes(buf, len); }
EXPORT_SYMBOL(hd_get_random_bytes);

void hd_spin_lock_irqsave(void) { spin_lock_irqsave(&my_lock, flags); }
EXPORT_SYMBOL(hd_spin_lock_irqsave);

void hd_spin_unlock_irqrestore(void) { spin_unlock_irqrestore(&my_lock, flags); }
EXPORT_SYMBOL(hd_spin_unlock_irqrestore);

void hd_pr_info(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vprintk(fmt, args);
  va_end(args);
}
EXPORT_SYMBOL(hd_pr_info);

static noinline int tracing_mark_write(const char *buf) {
  trace_printk(buf);
  return 0;
}

void hd_fpsgo_systrace_c(unsigned long long bufID, int val, const char *fmt, ...){
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
    len = snprintf(buf2, sizeof(buf2), "C|%d|%s|%d\n", 0, log, val);
  } else {
    len = snprintf(buf2, sizeof(buf2), "C|%d|%s|%d|0x%llx\n", 0, log, val, bufID);
  }
  if (unlikely(len < 0))
    return;
  else if (unlikely(len == 256))
    buf2[255] = '\0';

  tracing_mark_write(buf2);
}
EXPORT_SYMBOL(hd_fpsgo_systrace_c);
