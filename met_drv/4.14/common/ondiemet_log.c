/*
 * Copyright (C) 2019 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/freezer.h>
#include <linux/uaccess.h>
#include <linux/completion.h>

#include "ondiemet_log.h"

#define ONDIEMET_LOG_REQ 1
/* TODO: abandon this constatnt */
#define ONDIEMET_LOG_STOP 2

#define PID_NONE (-1)

#define ONDIEMET_LOG_STOP_MODE 0
#define ONDIEMET_LOG_RUN_MODE 1
#define ONDIEMET_LOG_DEBUG_MODE 2

static int ondiemet_trace_run;
static struct dentry *dbgfs_met_dir;

struct mutex lock_tracef;
struct ondiemet_log_req_q_t {
	struct list_head listq;
	struct mutex lockq;
	/* struct semaphore new_evt_sema; */
	struct completion new_evt_comp;
	int closeq_flag;
} ondiemet_log_req_q;

struct ondiemet_log_req {
	struct list_head list;
	int cmd_type;
	const char *src;
	size_t num;

	void (*on_fini_cb)(const void *p);
	const void *param;
};

#define __ondiemet_log_req_init(req, cmd, s, n, pf, p)	\
	do {						\
		INIT_LIST_HEAD(&req->list);		\
		req->cmd_type = cmd;			\
		req->src = s;				\
		req->num = n;				\
		req->on_fini_cb = pf;			\
		req->param = p;				\
	} while (0)

#define __ondiemet_log_req_fini(req)		        \
	do {					        \
		if (req->on_fini_cb)			\
			req->on_fini_cb(req->param);	\
		kfree(req);				\
	} while (0)

static void __ondiemet_log_req_q_init(struct ondiemet_log_req_q_t *q)
{
	INIT_LIST_HEAD(&q->listq);
	mutex_init(&q->lockq);
	/* sema_init(&q->new_evt_sema, 0); */
	init_completion(&q->new_evt_comp);
	q->closeq_flag = 1;
}

/* undequeue is seen as a roll-back operation, so it can be done even when the queue is closed */
static void __ondiemet_log_req_undeq(struct ondiemet_log_req *req)
{
	mutex_lock(&ondiemet_log_req_q.lockq);
	list_add(&req->list, &ondiemet_log_req_q.listq);
	mutex_unlock(&ondiemet_log_req_q.lockq);

	/* up(&ondiemet_log_req_q.new_evt_sema); */
	complete(&ondiemet_log_req_q.new_evt_comp);
}

static int __ondiemet_log_req_enq(struct ondiemet_log_req *req)
{
	mutex_lock(&ondiemet_log_req_q.lockq);
	if (ondiemet_log_req_q.closeq_flag) {
		mutex_unlock(&ondiemet_log_req_q.lockq);
		return -EBUSY;
	}

	list_add_tail(&req->list, &ondiemet_log_req_q.listq);
	if (req->cmd_type == ONDIEMET_LOG_STOP)
		ondiemet_log_req_q.closeq_flag = 1;
	mutex_unlock(&ondiemet_log_req_q.lockq);

	/* up(&ondiemet_log_req_q.new_evt_sema); */
	complete(&ondiemet_log_req_q.new_evt_comp);

	return 0;
}

int ondiemet_log_req_enq(const char *src, size_t num, void (*on_fini_cb)(const void *p), const void *param)
{
	struct ondiemet_log_req *req = kmalloc(sizeof(*req), GFP_KERNEL);

	__ondiemet_log_req_init(req, ONDIEMET_LOG_REQ, src, num, on_fini_cb, param);
	return __ondiemet_log_req_enq(req);
}

/*int down_freezable_interruptible(struct semaphore *sem) */
int down_freezable_interruptible(struct completion *comp)
{

	int ret;

	freezer_do_not_count();
	/* ret = down_interruptible(sem); */
	ret = wait_for_completion_interruptible(comp);
	freezer_count();

	return ret;
}

struct ondiemet_log_req *__ondiemet_log_req_deq(void)
{
	struct ondiemet_log_req *ret_req;

	/*if (down_freezable_interruptible(&ondiemet_log_req_q.new_evt_sema))*/
	if (down_freezable_interruptible(&ondiemet_log_req_q.new_evt_comp))
		return NULL;

	mutex_lock(&ondiemet_log_req_q.lockq);
	ret_req = list_entry(ondiemet_log_req_q.listq.next, struct ondiemet_log_req, list);
	list_del_init(&ret_req->list);
	mutex_unlock(&ondiemet_log_req_q.lockq);

	return ret_req;
}

void __ondiemet_log_req_open(void)
{
	mutex_lock(&ondiemet_log_req_q.lockq);
	ondiemet_log_req_q.closeq_flag = 0;
	mutex_unlock(&ondiemet_log_req_q.lockq);
}

int __ondiemet_log_req_closed(void)
{
	int ret;

	mutex_lock(&ondiemet_log_req_q.lockq);
	ret = ondiemet_log_req_q.closeq_flag && list_empty(&ondiemet_log_req_q.listq);
	mutex_unlock(&ondiemet_log_req_q.lockq);

	return ret;
}

int __ondiemet_log_req_working(void)
{
	int ret;

	mutex_lock(&ondiemet_log_req_q.lockq);
	ret = !ondiemet_log_req_q.closeq_flag;
	mutex_unlock(&ondiemet_log_req_q.lockq);

	return ret;
}

static void *__ondiemet_trace_seq_next(struct seq_file *seqf, loff_t *offset)
{
	struct ondiemet_log_req *next_req;

	if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
		pr_debug("[met] __ondiemet_trace_seq_next: pid: %d\n", current->pid);

	if (__ondiemet_log_req_closed())
		return NULL;

	next_req = __ondiemet_log_req_deq();

	if (next_req == NULL)
		return NULL;

	if (next_req->cmd_type == ONDIEMET_LOG_STOP) {
		__ondiemet_log_req_fini(next_req);
		return NULL;
	}

	return (void *) next_req;
}

struct mutex lock_trace_owner_pid;
pid_t trace_owner_pid = PID_NONE;
static void *ondiemet_trace_seq_start(struct seq_file *seqf, loff_t *offset)
{
	void *ret;

	if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE) {
		pr_debug("[met] ondiemet_trace_seq_start: locked_pid: %d, pid: %d, offset: %llu\n",
			 trace_owner_pid, current->pid, *offset);
	}

	if (!mutex_trylock(&lock_tracef))
		return NULL;

	mutex_lock(&lock_trace_owner_pid);
	trace_owner_pid = current->pid;
	current->flags |= PF_NOFREEZE;
	mutex_unlock(&lock_trace_owner_pid);

	ret = __ondiemet_trace_seq_next(seqf, offset);

	return ret;
}

static void *ondiemet_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset)
{
	if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
		pr_debug("[met] ondiemet_trace_seq_next: pid: %d\n", current->pid);

	(*offset)++;
	return __ondiemet_trace_seq_next(seqf, offset);
}

static int ondiemet_trace_seq_show(struct seq_file *seqf, void *p)
{
	struct ondiemet_log_req *req = (struct ondiemet_log_req *) p;
	size_t l_sz;
	size_t r_sz;
	struct ondiemet_log_req *l_req;
	struct ondiemet_log_req *r_req;
	int ret;

	if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
		pr_debug("[met] ondiemet_trace_seq_show: pid: %d\n", current->pid);

	if (req->num >= seqf->size) {
		l_req = kmalloc(sizeof(*req), GFP_KERNEL);
		r_req = req;

		l_sz = seqf->size >> 1;
		r_sz = req->num - l_sz;
		__ondiemet_log_req_init(l_req, ONDIEMET_LOG_REQ, req->src, l_sz, NULL, NULL);
		__ondiemet_log_req_init(r_req, ONDIEMET_LOG_REQ, req->src + l_sz,
					r_sz, req->on_fini_cb, req->param);

		__ondiemet_log_req_undeq(r_req);
		req = l_req;

		if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
			pr_debug("[met] ondiemet_trace_seq_show: split request\n");
	}

	ret = seq_write(seqf, req->src, req->num);

	if (ret) {
		/* check if seq_file buffer overflows */
		if (seqf->count == seqf->size) {
			__ondiemet_log_req_undeq(req);
		} else {
			if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
				pr_debug("[met] ondiemet_trace_seq_show: reading trace record failed, some data may be lost or corrupted\n");
			__ondiemet_log_req_fini(req);
		}
		return 0;
	}

	__ondiemet_log_req_fini(req);
	return 0;
}

static void ondiemet_trace_seq_stop(struct seq_file *seqf, void *p)
{
	if (ondiemet_trace_run == ONDIEMET_LOG_DEBUG_MODE)
		pr_debug("[met] ondiemet_trace_seq_stop: pid: %d\n", current->pid);

	mutex_lock(&lock_trace_owner_pid);
	if (current->pid == trace_owner_pid) {
		trace_owner_pid = PID_NONE;
		mutex_unlock(&lock_tracef);
	}
	mutex_unlock(&lock_trace_owner_pid);
}

static const struct seq_operations ondiemet_trace_seq_ops = {
	.start = ondiemet_trace_seq_start,
	.next = ondiemet_trace_seq_next,
	.stop = ondiemet_trace_seq_stop,
	.show = ondiemet_trace_seq_show
};

static int ondiemet_trace_open(struct inode *inode, struct file *fp)
{
	return seq_open(fp, &ondiemet_trace_seq_ops);
}

static const struct file_operations ondiemet_trace_fops = {
	.owner = THIS_MODULE,
	.open = ondiemet_trace_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

/*struct semaphore log_start_sema;*/
struct completion log_start_comp;
int ondiemet_log_manager_start(void)
{
	int ret;

	/* TODO: choose a better return value */
	if (__ondiemet_log_req_working())
		return -EINVAL;

	if (!__ondiemet_log_req_closed()) {
		/*ret = down_killable(&log_start_sema);*/
		ret = wait_for_completion_killable(&log_start_comp);
		if (ret)
			return ret;
	}

	__ondiemet_log_req_open();

	return 0;
}

/*struct semaphore log_stop_sema;*/
struct completion log_stop_comp;
static void __log_stop_cb(const void *p)
{
	/* up(&log_start_sema); */
	/* up(&log_stop_sema); */
	complete(&log_start_comp);
	complete(&log_stop_comp);
}

int ondiemet_log_manager_stop(void)
{
	int ret;
	struct ondiemet_log_req *req;

	/* TODO: choose a better return value */
	if (__ondiemet_log_req_closed())
		return -EINVAL;

	req = kmalloc(sizeof(*req), GFP_KERNEL);

	__ondiemet_log_req_init(req, ONDIEMET_LOG_STOP, NULL, 0, __log_stop_cb, NULL);
	/*sema_init(&log_start_sema, 0); */
	/*sema_init(&log_stop_sema, 0); */
	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);

	ret = __ondiemet_log_req_enq(req);
	if (ret)
		return ret;

	/* XXX: blocking may be break by SIGKILL */
	/*return down_killable(&log_stop_sema);*/
	return wait_for_completion_killable(&log_stop_comp);
}

int ondiemet_parse_num(const char *str, unsigned int *value, int len)
{
	int ret;

	if (len <= 0)
		return -1;

	if ((len > 2) &&
	    ((str[0] == '0') &&
	     ((str[1] == 'x') || (str[1] == 'X')))) {
		ret = kstrtouint(str, 16, value);
	} else {
		ret = kstrtouint(str, 10, value);
	}

	if (ret != 0)
		return -1;

	return 0;
}

/* XXX: seq_file will output only when a page is filled */
static ssize_t ondiemet_log_write_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	char *plog = NULL;

	plog = kmalloc_array(count, sizeof(*plog), GFP_KERNEL);
	if (!plog) {
		/* TODO: use a better error code */
		return -EINVAL;
	}

	memcpy(plog, buf, count);

	mutex_lock(&dev->mutex);
	ondiemet_log_req_enq(plog, strnlen(plog, count), kfree, plog);
	mutex_unlock(&dev->mutex);

	return count;
}

static DEVICE_ATTR(ondiemet_log_write, 0664, NULL, ondiemet_log_write_store);

static ssize_t ondiemet_log_run_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int sz;

	mutex_lock(&dev->mutex);
	sz = snprintf(buf, PAGE_SIZE, "%d\n", ondiemet_trace_run);
	mutex_unlock(&dev->mutex);
	return sz;
}

static ssize_t ondiemet_log_run_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int prev_run_state;

	mutex_lock(&dev->mutex);

	prev_run_state = ondiemet_trace_run;

	if (kstrtoint(buf, 10, &ondiemet_trace_run) != 0)
		return -EINVAL;

	if (ondiemet_trace_run <= ONDIEMET_LOG_STOP_MODE) {
		ondiemet_trace_run = ONDIEMET_LOG_STOP_MODE;
		ondiemet_log_manager_stop();

		if (prev_run_state == ONDIEMET_LOG_DEBUG_MODE)
			device_remove_file(dev, &dev_attr_ondiemet_log_write);
	} else if (ondiemet_trace_run == ONDIEMET_LOG_RUN_MODE) {
		ondiemet_trace_run = ONDIEMET_LOG_RUN_MODE;
		ondiemet_log_manager_start();

		if (prev_run_state == ONDIEMET_LOG_DEBUG_MODE)
			device_remove_file(dev, &dev_attr_ondiemet_log_write);
	} else {
		ondiemet_trace_run = ONDIEMET_LOG_DEBUG_MODE;
		ondiemet_log_manager_start();

		if (prev_run_state != ONDIEMET_LOG_DEBUG_MODE) {
			ret = device_create_file(dev, &dev_attr_ondiemet_log_write);
			if (ret != 0)
				pr_debug("[met] can not create device node: ondiemet_log_write\n");
		}
	}

	mutex_unlock(&dev->mutex);

	return count;
}

static DEVICE_ATTR(ondiemet_log_run, 0660, ondiemet_log_run_show, ondiemet_log_run_store);

int ondiemet_log_manager_init(struct device *dev)
{
	int ret;
	struct dentry *d;

	mutex_init(&lock_tracef);

	__ondiemet_log_req_q_init(&ondiemet_log_req_q);

	/*sema_init(&log_start_sema, 0);*/
	/*sema_init(&log_stop_sema, 0);*/
	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);

	dbgfs_met_dir = debugfs_create_dir("ondiemet", NULL);
	if (!dbgfs_met_dir) {
		pr_debug("[met] can not create debugfs directory: met\n");
		return -ENOMEM;
	}

	mutex_init(&lock_trace_owner_pid);

	d = debugfs_create_file("trace", 0644, dbgfs_met_dir, NULL, &ondiemet_trace_fops);
	if (!d) {
		pr_debug("[met] can not create devide node in debugfs: ondiemet_trace\n");
		return -ENOMEM;
	}

	ondiemet_trace_run = __ondiemet_log_req_working();
	ret = device_create_file(dev, &dev_attr_ondiemet_log_run);
	if (ret != 0) {
		pr_debug("[met] can not create device node: ondiemet_log_run\n");
		return ret;
	}

	return 0;
}

int ondiemet_log_manager_uninit(struct device *dev)
{
	device_remove_file(dev, &dev_attr_ondiemet_log_run);
	debugfs_remove_recursive(dbgfs_met_dir);
	return 0;
}
