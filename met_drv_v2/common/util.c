// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "util.h"
#include <linux/fs.h>
#include <linux/kernel.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>

#ifdef FILELOG

static char tmp[1000] = { 0 };

 /*TODO*/
/**
 * open file
 * @param name path to open
 * @return file pointer
 */
struct file *open_file(const char *name)
{
	struct file *fp = NULL;

	fp = filp_open(name, O_WRONLY | O_APPEND /*| O_TRUNC */  | O_CREAT, 0664);
	if (unlikely(fp == NULL)) {
		pr_debug(KERNEL_INFO "can not open result file");
		return NULL;
	}
	return fp;
}

/**
 * write to file
 * @param fp file pointer
 * @param format format string
 * @param ... variable-length subsequent arguments
 */
void write_file(struct file *fp, const char *format, ...)
{
	va_list va;
	mm_segment_t fs = get_fs();

	va_start(va, format);
	vsnprintf(tmp, sizeof(tmp), format, va);
	set_fs(KERNEL_DS);
	vfs_write(fp, tmp, strlen(tmp), &(fp->f_pos));
	set_fs(fs);
	va_end(va);
}

/**
 * close file
 * @param fp file pointer
 * @return exit code
 */
int close_file(struct file *fp)
{
	if (likely(fp != NULL)) {
		filp_close(fp, NULL);
		fp = NULL;
		return 0;
	}
	pr_debug("cannot close file pointer:%p\n", fp);
	return -1;
}

void filelog(char *str)
{
	struct file *fp;

	fp = open_file("/data/met.log");
	if (fp != NULL) {
		write_file(fp, "%s", str);
		close_file(fp);
	}
}

#endif				/* FILELOG */
