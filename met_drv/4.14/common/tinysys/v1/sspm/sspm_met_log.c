// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/freezer.h>
#include <linux/uaccess.h>
#include <linux/completion.h>
#include <linux/module.h>	/* symbol_get */

#include "sspm_reservedmem.h"
#include "sspm_reservedmem_define.h"

#include "interface.h"

#include "sspm_met_log.h"
#include "sspm_met_ipi_handle.h"


/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define SSPM_LOG_REQ 1
#define SSPM_LOG_STOP 2

#define PID_NONE (-1)

#define SSPM_LOG_STOP_MODE 	0
#define SSPM_LOG_RUN_MODE 	1
#define SSPM_LOG_DEBUG_MODE 	2

#define _sspm_log_req_init(req, cmd, s, n, pf, p)	\
	do {						\
		INIT_LIST_HEAD(&req->list);		\
		req->cmd_type = cmd;			\
		req->src = s;				\
		req->num = n;				\
		req->on_fini_cb = pf;			\
		req->param = p;				\
	} while (0)

#define _sspm_log_req_fini(req)			\
	do {						\
		if (req->on_fini_cb)			\
			req->on_fini_cb(req->param);	\
		kfree(req);				\
	} while (0)


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
struct sspm_log_req_q_t {
	struct list_head listq;
	struct mutex lockq;
	struct completion new_evt_comp;
	int closeq_flag;
} sspm_log_req_q;

struct sspm_log_req {
	struct list_head list;
	int cmd_type;
	const char *src;
	size_t num;

	void (*on_fini_cb)(const void *p);
	const void *param;
};


/*****************************************************************************
 * external function declaration
 *****************************************************************************/


/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _log_stop_cb(const void *p);
static int _down_freezable_interruptible(struct completion *comp);

static void _sspm_log_req_q_init(struct sspm_log_req_q_t *q);
static void _sspm_log_req_undeq(struct sspm_log_req *req);
static int _sspm_log_req_enq(struct sspm_log_req *req);
static struct sspm_log_req *_sspm_log_req_deq(void);
static void _sspm_log_req_open(void);
static int _sspm_log_req_closed(void);
static int _sspm_log_req_working(void);
static void *_sspm_trace_seq_next(struct seq_file *seqf, loff_t *offset);

static void *sspm_trace_seq_start(struct seq_file *seqf, loff_t *offset);
static void sspm_trace_seq_stop(struct seq_file *seqf, void *p);
static void *sspm_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset);
static int sspm_trace_seq_show(struct seq_file *seqf, void *p);
static int sspm_trace_open(struct inode *inode, struct file *fp);

static ssize_t ondiemet_log_write_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);
static ssize_t ondiemet_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf);
static ssize_t ondiemet_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
void *sspm_log_virt_addr;

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
dma_addr_t sspm_log_phy_addr;
#else
unsigned int sspm_log_phy_addr;
#endif

unsigned int sspm_buffer_size;

extern int sspm_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static int sspm_trace_run;
static pid_t trace_owner_pid = PID_NONE;

static struct mutex lock_tracef;
static struct mutex lock_trace_owner_pid;

static struct completion log_start_comp;
static struct completion log_stop_comp;

static DEVICE_ATTR(ondiemet_log_write, 0220, NULL, ondiemet_log_write_store);
static DEVICE_ATTR(ondiemet_log_run, 0664, ondiemet_log_run_show, ondiemet_log_run_store);;

static const struct seq_operations sspm_trace_seq_ops = {
	.start = sspm_trace_seq_start,
	.next = sspm_trace_seq_next,
	.stop = sspm_trace_seq_stop,
	.show = sspm_trace_seq_show
};

static const struct file_operations sspm_trace_fops = {
	.owner = THIS_MODULE,
	.open = sspm_trace_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};


/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/
int sspm_log_init(struct device *dev)
{
	int ret = 0;
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *d;
	struct dentry *met_dir = NULL;
#else
	struct proc_dir_entry *d;
	struct proc_dir_entry *met_dir = NULL;
#endif
	phys_addr_t (*get_size_sym)(unsigned int id) = NULL;

	met_dir = dev_get_drvdata(dev);
	mutex_init(&lock_tracef);
	_sspm_log_req_q_init(&sspm_log_req_q);
	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);
	mutex_init(&lock_trace_owner_pid);

#ifdef ONDIEMET_MOUNT_DEBUGFS
	d = debugfs_create_file("trace", 0600, met_dir, NULL, &sspm_trace_fops);
	if (!d) {
		PR_BOOTMSG("can not create devide node in debugfs: sspm_trace\n");
		return -ENOMEM;
	}
#else
	d = proc_create("trace", 0600, met_dir, &sspm_trace_fops);
	if (!d) {
		PR_BOOTMSG("can not create devide node in procfs: sspm_trace\n");
		return -ENOMEM;
	}
#endif

	sspm_trace_run = _sspm_log_req_working();
	ret = device_create_file(dev, &dev_attr_ondiemet_log_run);
	if (ret != 0) {
		PR_BOOTMSG("can not create device node: sspm_log_run\n");
		return ret;
	}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
	sspm_log_virt_addr = dma_alloc_coherent(dev, 0x800000,
				&sspm_log_phy_addr, GFP_KERNEL);
	if (sspm_log_virt_addr) {
		sspm_buffer_size = 0x800000;
		sspm_buf_available = 1;
	} else {
		sspm_buf_available = 0;
	}
#else
	get_size_sym = symbol_get(sspm_reserve_mem_get_size);
	if (get_size_sym) {
		sspm_buffer_size = get_size_sym(MET_MEM_ID);
		PR_BOOTMSG("sspm_buffer_size=%x \n", sspm_buffer_size);
	} else {
		PR_BOOTMSG("symbol_get sspm_reserve_mem_get_size failure\n");
	}

	if (sspm_buffer_size > 0) {
		phys_addr_t (*get_phys_sym)(unsigned int id) = NULL;
		phys_addr_t (*get_virt_sym)(unsigned int id) = NULL;

		get_phys_sym = symbol_get(sspm_reserve_mem_get_virt);
		get_virt_sym = symbol_get(sspm_reserve_mem_get_phys);
		if (get_phys_sym) {
			sspm_log_virt_addr = (void*)get_phys_sym(MET_MEM_ID);
			PR_BOOTMSG("sspm_log_virt_addr=%x \n", sspm_log_virt_addr);
		} else {
			PR_BOOTMSG("symbol_get sspm_reserve_mem_get_virt failure\n");
		}
		if (get_virt_sym) {
			sspm_log_phy_addr = get_virt_sym(MET_MEM_ID);
			PR_BOOTMSG("sspm_log_phy_addr=%x \n", sspm_log_phy_addr);
		} else {
			PR_BOOTMSG("symbol_get sspm_reserve_mem_get_phys failure\n");
		}
		sspm_buf_available = 1;
	} else {
		sspm_buf_available = 0;
	}

#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

	start_sspm_ipi_recv_thread();

	return 0;
}


int sspm_log_uninit(struct device *dev)
{
	stop_sspm_ipi_recv_thread();

	if (sspm_log_virt_addr != NULL) {
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
		dma_free_coherent(dev, sspm_buffer_size, sspm_log_virt_addr,
			sspm_log_phy_addr);
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
		sspm_log_virt_addr = NULL;
	}

	device_remove_file(dev, &dev_attr_ondiemet_log_run);
	return 0;
}


int sspm_log_start(void)
{
	int ret = 0;

	/* TODO: choose a better return value */
	if (_sspm_log_req_working()) {
		return -EINVAL;
	}

	if (!_sspm_log_req_closed()) {
		ret = wait_for_completion_killable(&log_start_comp);
		if (ret) {
			return ret;
		}
	}

	_sspm_log_req_open();

	return 0;
}


int sspm_log_stop(void)
{
	int ret = 0;
	struct sspm_log_req *req = NULL;

	if (_sspm_log_req_closed()) {
		return -EINVAL;
	}

	req = kmalloc(sizeof(*req), GFP_KERNEL);
	if (req == NULL)
		return -1;
	_sspm_log_req_init(req, SSPM_LOG_STOP, NULL, 0, _log_stop_cb, NULL);

	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);

	ret = _sspm_log_req_enq(req);
	if (ret) {
		return ret;
	}

	return wait_for_completion_killable(&log_stop_comp);
}


int sspm_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param)
{
	struct sspm_log_req *req = kmalloc(sizeof(*req), GFP_KERNEL);

	if (req == NULL)
		return -1;
	_sspm_log_req_init(req, SSPM_LOG_REQ, src, num, on_fini_cb, param);
	return _sspm_log_req_enq(req);
}


int sspm_parse_num(const char *str, unsigned int *value, int len)
{
	int ret = 0;

	if (len <= 0) {
		return -1;
	}

	if ((len > 2) && ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X')))) {
		ret = kstrtouint(str, 16, value);
	} else {
		ret = kstrtouint(str, 10, value);
	}

	if (ret != 0) {
		return -1;
	}

	return 0;
}


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
static void _log_stop_cb(const void *p)
{
	complete(&log_start_comp);
	complete(&log_stop_comp);
}


static int _down_freezable_interruptible(struct completion *comp)
{
	int ret = 0;

	freezer_do_not_count();
	ret = wait_for_completion_interruptible(comp);
	freezer_count();

	return ret;
}


static void _sspm_log_req_q_init(struct sspm_log_req_q_t *q)
{
	INIT_LIST_HEAD(&q->listq);
	mutex_init(&q->lockq);
	init_completion(&q->new_evt_comp);
	q->closeq_flag = 1;
}


/*
	undequeue is seen as a roll-back operation,
	so it can be done even when the queue is closed
*/
static void _sspm_log_req_undeq(struct sspm_log_req *req)
{
	mutex_lock(&sspm_log_req_q.lockq);
	list_add(&req->list, &sspm_log_req_q.listq);
	mutex_unlock(&sspm_log_req_q.lockq);

	complete(&sspm_log_req_q.new_evt_comp);
}


static int _sspm_log_req_enq(struct sspm_log_req *req)
{
	mutex_lock(&sspm_log_req_q.lockq);
	if (sspm_log_req_q.closeq_flag) {
		mutex_unlock(&sspm_log_req_q.lockq);
		return -EBUSY;
	}

	list_add_tail(&req->list, &sspm_log_req_q.listq);
	if (req->cmd_type == SSPM_LOG_STOP) {
		sspm_log_req_q.closeq_flag = 1;
	}
	mutex_unlock(&sspm_log_req_q.lockq);

	complete(&sspm_log_req_q.new_evt_comp);

	return 0;
}


static struct sspm_log_req *_sspm_log_req_deq(void)
{
	struct sspm_log_req *ret_req;

	if (_down_freezable_interruptible(&sspm_log_req_q.new_evt_comp)) {
		return NULL;
	}

	mutex_lock(&sspm_log_req_q.lockq);
	ret_req = list_entry(sspm_log_req_q.listq.next, struct sspm_log_req, list);
	list_del_init(&ret_req->list);
	mutex_unlock(&sspm_log_req_q.lockq);

	return ret_req;
}


static void _sspm_log_req_open(void)
{
	mutex_lock(&sspm_log_req_q.lockq);
	sspm_log_req_q.closeq_flag = 0;
	mutex_unlock(&sspm_log_req_q.lockq);
}


static int _sspm_log_req_closed(void)
{
	int ret = 0;

	mutex_lock(&sspm_log_req_q.lockq);
	ret = sspm_log_req_q.closeq_flag && list_empty(&sspm_log_req_q.listq);
	mutex_unlock(&sspm_log_req_q.lockq);

	return ret;
}


static int _sspm_log_req_working(void)
{
	int ret = 0;

	mutex_lock(&sspm_log_req_q.lockq);
	ret = !sspm_log_req_q.closeq_flag;
	mutex_unlock(&sspm_log_req_q.lockq);

	return ret;
}


static void *_sspm_trace_seq_next(struct seq_file *seqf, loff_t *offset)
{
	struct sspm_log_req *next_req;

	if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("_sspm_trace_seq_next: pid: %d\n", current->pid);
	}

	if (_sspm_log_req_closed()) {
		return NULL;
	}

	next_req = _sspm_log_req_deq();
	if (next_req == NULL) {
		return NULL;
	}

	if (next_req->cmd_type == SSPM_LOG_STOP) {
		_sspm_log_req_fini(next_req);
		return NULL;
	}

	return (void *) next_req;
}


static void *sspm_trace_seq_start(struct seq_file *seqf, loff_t *offset)
{
	void *ret;

	if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("sspm_trace_seq_start: locked_pid: %d, pid: %d, offset: %llu\n",
			 trace_owner_pid, current->pid, *offset);
	}

	if (!mutex_trylock(&lock_tracef)) {
		return NULL;
	}

	mutex_lock(&lock_trace_owner_pid);
	trace_owner_pid = current->pid;
	current->flags |= PF_NOFREEZE;
	mutex_unlock(&lock_trace_owner_pid);

	ret = _sspm_trace_seq_next(seqf, offset);

	return ret;
}


static void sspm_trace_seq_stop(struct seq_file *seqf, void *p)
{
	if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("sspm_trace_seq_stop: pid: %d\n", current->pid);
	}

	mutex_lock(&lock_trace_owner_pid);
	if (current->pid == trace_owner_pid) {
		trace_owner_pid = PID_NONE;
		mutex_unlock(&lock_tracef);
	}
	mutex_unlock(&lock_trace_owner_pid);
}


static void *sspm_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset)
{
	if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("sspm_trace_seq_next: pid: %d\n", current->pid);
	}
	(*offset)++;
	return _sspm_trace_seq_next(seqf, offset);
}


static int sspm_trace_seq_show(struct seq_file *seqf, void *p)
{
	struct sspm_log_req *req = (struct sspm_log_req *)p;
	size_t l_sz;
	size_t r_sz;
	struct sspm_log_req *l_req;
	struct sspm_log_req *r_req;
	int ret = 0;

	if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("sspm_trace_seq_show: pid: %d\n", current->pid);
	}

	if (req->num >= seqf->size) {
		l_req = kmalloc(sizeof(*req), GFP_KERNEL);
		if (l_req == NULL)
			return -1;

		r_req = req;

		l_sz = seqf->size >> 1;
		r_sz = req->num - l_sz;
		_sspm_log_req_init(l_req, SSPM_LOG_REQ, req->src, l_sz, NULL, NULL);
		_sspm_log_req_init(r_req, SSPM_LOG_REQ, req->src + l_sz,
					r_sz, req->on_fini_cb, req->param);

		_sspm_log_req_undeq(r_req);
		req = l_req;

		if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
			PR_BOOTMSG("sspm_trace_seq_show: split request\n");
		}
	}

	ret = seq_write(seqf, req->src, req->num);
	if (ret) {
		/* check if seq_file buffer overflows */
		if (seqf->count == seqf->size) {
			_sspm_log_req_undeq(req);
		} else {
			if (sspm_trace_run == SSPM_LOG_DEBUG_MODE) {
				PR_BOOTMSG("sspm_trace_seq_show: \
					reading trace record failed, \
					some data may be lost or corrupted\n");
			}
			_sspm_log_req_fini(req);
		}
		return 0;
	}

	_sspm_log_req_fini(req);
	return 0;
}


static int sspm_trace_open(struct inode *inode, struct file *fp)
{
	return seq_open(fp, &sspm_trace_seq_ops);
}


static ssize_t ondiemet_log_write_store(
	struct device *dev,
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
	sspm_log_req_enq(plog, strnlen(plog, count), kfree, plog);
	mutex_unlock(&dev->mutex);

	return count;
}


static ssize_t ondiemet_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int sz;

	mutex_lock(&dev->mutex);
	sz = snprintf(buf, PAGE_SIZE, "%d\n", sspm_trace_run);
	mutex_unlock(&dev->mutex);
	return sz;
}


static ssize_t ondiemet_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int ret = 0;
	int prev_run_state;

	mutex_lock(&dev->mutex);
	prev_run_state = sspm_trace_run;
	if (kstrtoint(buf, 10, &sspm_trace_run) != 0) {
		return -EINVAL;
	}

	if (sspm_trace_run <= SSPM_LOG_STOP_MODE) {
		sspm_trace_run = SSPM_LOG_STOP_MODE;
		sspm_log_stop();

		if (prev_run_state == SSPM_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_ondiemet_log_write);
		}
	} else if (sspm_trace_run == SSPM_LOG_RUN_MODE) {
		sspm_trace_run = SSPM_LOG_RUN_MODE;
		sspm_log_start();

		if (prev_run_state == SSPM_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_ondiemet_log_write);
		}
	} else {
		sspm_trace_run = SSPM_LOG_DEBUG_MODE;
		sspm_log_start();

		if (prev_run_state != SSPM_LOG_DEBUG_MODE) {
			ret = device_create_file(dev, &dev_attr_ondiemet_log_write);
			if (ret != 0)  {
				PR_BOOTMSG("can not create device node: \
					sspm_log_write\n");
			}
		}
	}
	mutex_unlock(&dev->mutex);

	return count;
}

