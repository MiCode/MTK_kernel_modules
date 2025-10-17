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
#include "jank_detection_utils.h"

static spinlock_t my_lock;
static unsigned long flags;

void *hd_kmalloc(int size) { return kmalloc(size, GFP_KERNEL); }
EXPORT_SYMBOL(hd_kmalloc);

void hd_kfree(void *p) { kfree(p); }
EXPORT_SYMBOL(hd_kfree);

int hd_task_pid_nr(void) { return (int)task_pid_nr(current); }
EXPORT_SYMBOL(hd_task_pid_nr);

void hd_get_random_bytes(void *buf, int len) { get_random_bytes(buf, len); }
EXPORT_SYMBOL(hd_get_random_bytes);

void hd_spin_lock_irqsave(void) { spin_lock_irqsave(&my_lock, flags); }
EXPORT_SYMBOL(hd_spin_lock_irqsave);

void hd_spin_unlock_irqrestore(void) {
  spin_unlock_irqrestore(&my_lock, flags);
}
EXPORT_SYMBOL(hd_spin_unlock_irqrestore);

