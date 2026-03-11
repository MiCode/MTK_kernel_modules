/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022 MediaTek Inc.
 */

void *hd_kmalloc(int size);
void *hd_vmalloc(int size);
void hd_memset(void *p, int value, int size);
void hd_kfree(void *p);
void hd_vfree(void *p);
int hd_task_pid_nr(void);
void hd_get_random_bytes(void *buf, int len);
void hd_spin_lock_irqsave(void);
void hd_spin_unlock_irqrestore(void);
void hd_pr_info(const char *fmt, ...);
void hd_fpsgo_systrace_c(unsigned long long bufID, int val, const char *fmt, ...);
