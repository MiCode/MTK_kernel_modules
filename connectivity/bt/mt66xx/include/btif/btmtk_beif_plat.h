// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

/* GDMA solution */

#ifndef BEIF_PLAT_H
#define BEIF_PLAT_H

#ifdef BEIF_CTP_LOAD
#include "typedefs.h"
#include "printf.h"
#include "time.h"
#include "stdio.h"
#include "string.h"
#include "mmio.h"
#include "CTP_mem.h"
#include "sizes.h"
#else
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <linux/spinlock_types.h>
#include <uapi/linux/sched/types.h>
#endif

#define EMI_READ32(addr) (readl(addr))
#define EMI_WRITE32(addr, data) (writel(data, addr))

#ifdef BEIF_CTP_LOAD
#define udelay(usecs) timer_udelay(usecs)
#define EXPORT_SYMBOL(x)
#ifdef CONFIG_ARCH_ARM64
typedef u64 phys_addr_t;
#else
typedef u32 phys_addr_t;
#endif
void memcpy_fromio(void *to, void *from, unsigned long count);
void memcpy_toio(volatile void *to, const void *from, unsigned long count);
void memset_io(volatile void *dst, int c, unsigned long count);
void *ioremap(phys_addr_t addr, unsigned int size);
void iounmap(void *addr);
void mutex_init(void *mutex);
void mutex_destroy(void *mutex);
int mutex_lock_killable(void *mutex);
int mutex_unlock(void *mutex);
#endif

void beif_get_local_time(u64 *sec, unsigned long *nsec);
#endif

