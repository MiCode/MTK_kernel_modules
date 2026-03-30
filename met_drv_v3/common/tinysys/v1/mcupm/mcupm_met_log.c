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

#include "interface.h"
#include "core_plf_init.h"

#include "mcupm_driver.h"
#include "mcupm_driver.h"

#include "mcupm_met_log.h"
#include "mcupm_met_ipi_handle.h"
#include <linux/of.h>
#include <linux/io.h>

/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define MUCPM_LOG_REQ 		1
#define MUCPM_LOG_STOP 	2

#define PID_NONE (-1)

#define MUCPM_LOG_STOP_MODE 	0
#define MUCPM_LOG_RUN_MODE 	1
#define MUCPM_LOG_DEBUG_MODE 	2

#define _mcupm_log_req_init(req, cmd, s, n, pf, p)	\
	do {						\
		INIT_LIST_HEAD(&req->list);		\
		req->cmd_type = cmd;			\
		req->src = s;				\
		req->num = n;				\
		req->on_fini_cb = pf;			\
		req->param = p;				\
	} while (0)

#define _mcupm_log_req_fini(req)			\
	do {						\
		if (req->on_fini_cb)			\
			req->on_fini_cb(req->param);	\
		kfree(req);				\
	} while (0)


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
struct mcupm_log_req_q_t {
	struct list_head listq;
	struct mutex lockq;
	struct completion new_evt_comp;
	int closeq_flag;
} mcupm_log_req_q;

struct mcupm_log_req {
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

static void _mcupm_log_req_q_init(struct mcupm_log_req_q_t *q);
static void _mcupm_log_req_undeq(struct mcupm_log_req *req);
static int _mcupm_log_req_enq(struct mcupm_log_req *req);
static struct mcupm_log_req *_mcupm_log_req_deq(void);
static void _mcupm_log_req_open(void);
static int _mcupm_log_req_closed(void);
static int _mcupm_log_req_working(void);
static void *_mcupm_trace_seq_next(struct seq_file *seqf, loff_t *offset);

static void *mcupm_trace_seq_start(struct seq_file *seqf, loff_t *offset);
static void mcupm_trace_seq_stop(struct seq_file *seqf, void *p);
static void *mcupm_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset);
static int mcupm_trace_seq_show(struct seq_file *seqf, void *p);
static int mcupm_trace_open(struct inode *inode, struct file *fp);

static ssize_t mcupm_log_write_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);
static ssize_t mcupm_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf);
static ssize_t mcupm_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
void *mcupm_log_virt_addr;

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
dma_addr_t mcupm_log_phy_addr;
#else
phys_addr_t mcupm_log_phy_addr;
#endif

unsigned int mcupm_buffer_size;

extern int mcupm_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static int mcupm_trace_run;
static pid_t trace_owner_pid = PID_NONE;

static struct mutex lock_tracef;
static struct mutex lock_trace_owner_pid;

static struct completion log_start_comp;
static struct completion log_stop_comp;

static DEVICE_ATTR(mcupm_log_write, 0220, NULL, mcupm_log_write_store);
static DEVICE_ATTR(mcupm_log_run, 0664, mcupm_log_run_show, mcupm_log_run_store);;

static const struct seq_operations mcupm_trace_seq_ops = {
	.start = mcupm_trace_seq_start,
	.next = mcupm_trace_seq_next,
	.stop = mcupm_trace_seq_stop,
	.show = mcupm_trace_seq_show
};

static const struct proc_ops mcupm_trace_fops = {
	.proc_open = mcupm_trace_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release
};

#ifdef ONDIEMET_MOUNT_DEBUGFS
static struct dentry *trace_dentry;
#else
static struct proc_dir_entry *trace_dentry;
#endif

/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/
int mcupm_log_init(struct device *dev)
{
	int ret = 0;
	struct device_node *np;
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *met_dir = NULL;
#else
	struct proc_dir_entry *met_dir = NULL;
#endif

	met_dir = dev_get_drvdata(dev);
	mutex_init(&lock_tracef);
	_mcupm_log_req_q_init(&mcupm_log_req_q);
	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);
	mutex_init(&lock_trace_owner_pid);

#ifdef ONDIEMET_MOUNT_DEBUGFS
	trace_dentry = debugfs_create_file("mcupm_trace", 0600, met_dir, NULL, &mcupm_trace_fops);
	if (!trace_dentry) {
		PR_BOOTMSG("can not create devide node in debugfs: mcupm_trace\n");
		return -ENOMEM;
	}
#else
	trace_dentry = proc_create("mcupm_trace", 0600, met_dir, &mcupm_trace_fops);
	if (!trace_dentry) {
		PR_BOOTMSG("can not create devide node in procfs: mcupm_trace\n");
		return -ENOMEM;
	}
#endif

	mcupm_trace_run = _mcupm_log_req_working();
	ret = device_create_file(dev, &dev_attr_mcupm_log_run);
	if (ret != 0) {
		PR_BOOTMSG("can not create device node: mcupm_log_run\n");
		return ret;
	}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
	mcupm_log_virt_addr = dma_alloc_coherent(dev, 0x800000,
				&mcupm_log_phy_addr, GFP_ATOMIC);
	if (mcupm_log_virt_addr) {
		mcupm_buffer_size = 0x800000;
		mcupm_buf_available = 1;
	} else {
		mcupm_buf_available = 0;
	}
#else
	np = of_find_node_by_name(NULL, "met_res_ram_mcupm");
	if (!np) {
		pr_debug("unable to find met_res_ram_mcupm\n");
		return 0;
	}
	of_property_read_u64(np, "start", &mcupm_log_phy_addr);
	of_property_read_u32(np, "size", &mcupm_buffer_size);

	if ((mcupm_log_phy_addr > 0) && (mcupm_buffer_size > 0)) {
		mcupm_log_virt_addr = (void*)ioremap_wc(mcupm_log_phy_addr, mcupm_buffer_size);

		mcupm_buf_available = 1;
	} else {
		mcupm_buf_available = 0;
	}
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

	start_mcupm_ipi_recv_thread();

	return 0;
}


int mcupm_log_uninit(struct device *dev)
{
	stop_mcupm_ipi_recv_thread();

	if (mcupm_log_virt_addr != NULL) {
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
		dma_free_coherent(dev, mcupm_buffer_size, mcupm_log_virt_addr,
			mcupm_log_phy_addr);
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
		mcupm_log_virt_addr = NULL;
	}

	device_remove_file(dev, &dev_attr_mcupm_log_run);

#ifdef ONDIEMET_MOUNT_DEBUGFS
       debugfs_remove(trace_dentry);
#else
       proc_remove(trace_dentry);
#endif

	return 0;
}


int mcupm_log_start(void)
{
	int ret = 0;

	/* TODO: choose a better return value */
	if (_mcupm_log_req_working()) {
		return -EINVAL;
	}

	if (!_mcupm_log_req_closed()) {
		/*ret = down_killable(&log_start_sema);*/
		ret = wait_for_completion_killable(&log_start_comp);
		if (ret) {
			return ret;
		}
	}

	_mcupm_log_req_open();

	return 0;
}


int mcupm_log_stop(void)
{
	int ret = 0;
	struct mcupm_log_req *req = NULL;

	if (_mcupm_log_req_closed()) {
		return -EINVAL;
	}

	req = kmalloc(sizeof(*req), GFP_ATOMIC);
	if (req == NULL) {
		return -EINVAL;
	}
	_mcupm_log_req_init(req, MUCPM_LOG_STOP, NULL, 0, _log_stop_cb, NULL);

	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);

	ret = _mcupm_log_req_enq(req);
	if (ret) {
		return ret;
	}

	return wait_for_completion_killable(&log_stop_comp);
}


int mcupm_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param)
{
	struct mcupm_log_req *req = kmalloc(sizeof(*req), GFP_ATOMIC);

	if (req == NULL) {
		return -EINVAL;
	}

	_mcupm_log_req_init(req, MUCPM_LOG_REQ, src, num, on_fini_cb, param);
	return _mcupm_log_req_enq(req);
}


int mcupm_parse_num(const char *str, unsigned int *value, int len)
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


static void _mcupm_log_req_q_init(struct mcupm_log_req_q_t *q)
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
static void _mcupm_log_req_undeq(struct mcupm_log_req *req)
{
	mutex_lock(&mcupm_log_req_q.lockq);
	list_add(&req->list, &mcupm_log_req_q.listq);
	mutex_unlock(&mcupm_log_req_q.lockq);

	complete(&mcupm_log_req_q.new_evt_comp);
}


static int _mcupm_log_req_enq(struct mcupm_log_req *req)
{
	mutex_lock(&mcupm_log_req_q.lockq);
	if (mcupm_log_req_q.closeq_flag) {
		mutex_unlock(&mcupm_log_req_q.lockq);
		return -EBUSY;
	}

	list_add_tail(&req->list, &mcupm_log_req_q.listq);
	if (req->cmd_type == MUCPM_LOG_STOP) {
		mcupm_log_req_q.closeq_flag = 1;
	}
	mutex_unlock(&mcupm_log_req_q.lockq);

	complete(&mcupm_log_req_q.new_evt_comp);

	return 0;
}


static struct mcupm_log_req *_mcupm_log_req_deq(void)
{
	struct mcupm_log_req *ret_req;

	if (_down_freezable_interruptible(&mcupm_log_req_q.new_evt_comp)) {
		return NULL;
	}

	mutex_lock(&mcupm_log_req_q.lockq);
	ret_req = list_entry(mcupm_log_req_q.listq.next, struct mcupm_log_req, list);
	list_del_init(&ret_req->list);
	mutex_unlock(&mcupm_log_req_q.lockq);

	return ret_req;
}


static void _mcupm_log_req_open(void)
{
	mutex_lock(&mcupm_log_req_q.lockq);
	mcupm_log_req_q.closeq_flag = 0;
	mutex_unlock(&mcupm_log_req_q.lockq);
}


static int _mcupm_log_req_closed(void)
{
	int ret = 0;

	mutex_lock(&mcupm_log_req_q.lockq);
	ret = mcupm_log_req_q.closeq_flag && list_empty(&mcupm_log_req_q.listq);
	mutex_unlock(&mcupm_log_req_q.lockq);

	return ret;
}


static int _mcupm_log_req_working(void)
{
	int ret = 0;

	mutex_lock(&mcupm_log_req_q.lockq);
	ret = !mcupm_log_req_q.closeq_flag;
	mutex_unlock(&mcupm_log_req_q.lockq);

	return ret;
}


static void *_mcupm_trace_seq_next(struct seq_file *seqf, loff_t *offset)
{
	struct mcupm_log_req *next_req;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("_mcupm_trace_seq_next: pid: %d\n", current->pid);
	}

	if (_mcupm_log_req_closed()) {
		return NULL;
	}

	next_req = _mcupm_log_req_deq();
	if (next_req == NULL) {
		return NULL;
	}

	if (next_req->cmd_type == MUCPM_LOG_STOP) {
		_mcupm_log_req_fini(next_req);
		return NULL;
	}

	return (void *) next_req;
}


static void *mcupm_trace_seq_start(struct seq_file *seqf, loff_t *offset)
{
	void *ret;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_start: locked_pid: %d, pid: %d, offset: %llu\n",
			 trace_owner_pid, current->pid, *offset);
	}

	if (!mutex_trylock(&lock_tracef)) {
		return NULL;
	}

	mutex_lock(&lock_trace_owner_pid);
	trace_owner_pid = current->pid;
	current->flags |= PF_NOFREEZE;
	mutex_unlock(&lock_trace_owner_pid);

	ret = _mcupm_trace_seq_next(seqf, offset);

	return ret;
}


static void mcupm_trace_seq_stop(struct seq_file *seqf, void *p)
{
	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_stop: pid: %d\n", current->pid);
	}

	mutex_lock(&lock_trace_owner_pid);
	if (current->pid == trace_owner_pid) {
		trace_owner_pid = PID_NONE;
		mutex_unlock(&lock_tracef);
	}
	mutex_unlock(&lock_trace_owner_pid);
}


static void *mcupm_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset)
{
	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_next: pid: %d\n", current->pid);
	}
	(*offset)++;
	return _mcupm_trace_seq_next(seqf, offset);
}


static int mcupm_trace_seq_show(struct seq_file *seqf, void *p)
{
	struct mcupm_log_req *req = (struct mcupm_log_req *) p;
	size_t l_sz;
	size_t r_sz;
	struct mcupm_log_req *l_req;
	struct mcupm_log_req *r_req;
	int ret = 0;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_show: pid: %d\n", current->pid);
	}

	if (req->num >= seqf->size) {
		l_req = kmalloc(sizeof(*req), GFP_ATOMIC);
		if (l_req == NULL) {
			return -EINVAL;
		}
		r_req = req;

		l_sz = seqf->size >> 1;
		r_sz = req->num - l_sz;
		_mcupm_log_req_init(l_req, MUCPM_LOG_REQ, req->src, l_sz, NULL, NULL);
		_mcupm_log_req_init(r_req, MUCPM_LOG_REQ, req->src + l_sz,
					r_sz, req->on_fini_cb, req->param);

		_mcupm_log_req_undeq(r_req);
		req = l_req;

		if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
			PR_BOOTMSG("mcupm_trace_seq_show: split request\n");
		}
	}

	ret = seq_write(seqf, req->src, req->num);
	if (ret) {
		/* check if seq_file buffer overflows */
		if (seqf->count == seqf->size) {
			_mcupm_log_req_undeq(req);
		} else {
			if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
				PR_BOOTMSG("mcupm_trace_seq_show: \
					reading trace record failed, \
					some data may be lost or corrupted\n");
			}
			_mcupm_log_req_fini(req);
		}
		return 0;
	}

	_mcupm_log_req_fini(req);
	return 0;
}


static int mcupm_trace_open(struct inode *inode, struct file *fp)
{
	return seq_open(fp, &mcupm_trace_seq_ops);
}


static ssize_t mcupm_log_write_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	char *plog = NULL;

	plog = kmalloc_array(count, sizeof(*plog), GFP_ATOMIC);
	if (!plog) {
		/* TODO: use a better error code */
		return -EINVAL;
	}

	memcpy(plog, buf, count);

	mutex_lock(&dev->mutex);
	mcupm_log_req_enq(plog, strnlen(plog, count), kfree, plog);
	mutex_unlock(&dev->mutex);

	return count;
}


static ssize_t mcupm_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int sz;

	mutex_lock(&dev->mutex);
	sz = snprintf(buf, PAGE_SIZE, "%d\n", mcupm_trace_run);
	mutex_unlock(&dev->mutex);

	if (sz < 0)
		sz = 0;

	return sz;
}


static ssize_t mcupm_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int ret = 0;
	int prev_run_state;

	mutex_lock(&dev->mutex);

	prev_run_state = mcupm_trace_run;
	if (kstrtoint(buf, 10, &mcupm_trace_run) != 0) {
		return -EINVAL;
	}

	if (mcupm_trace_run <= MUCPM_LOG_STOP_MODE) {
		mcupm_trace_run = MUCPM_LOG_STOP_MODE;
		mcupm_log_stop();

		if (prev_run_state == MUCPM_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_mcupm_log_write);
		}
	} else if (mcupm_trace_run == MUCPM_LOG_RUN_MODE) {
		mcupm_trace_run = MUCPM_LOG_RUN_MODE;
		mcupm_log_start();

		if (prev_run_state == MUCPM_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_mcupm_log_write);
		}
	} else {
		mcupm_trace_run = MUCPM_LOG_DEBUG_MODE;
		mcupm_log_start();

		if (prev_run_state != MUCPM_LOG_DEBUG_MODE) {
			ret = device_create_file(dev, &dev_attr_mcupm_log_write);
			if (ret != 0)  {
				PR_BOOTMSG("can not create device node: \
					mcupm_log_write\n");
			}
		}
	}

	mutex_unlock(&dev->mutex);

	return count;
}

