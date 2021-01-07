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
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/freezer.h>
#include <linux/uaccess.h>
#include <linux/completion.h>
#include <linux/module.h>	/* symbol_get */

#include "interface.h"

#include "mcupm_driver.h"
#include "mcupm_driver.h"

#include "cpu_eb_met_log.h"
#include "cpu_eb_met_ipi_handle.h"


/*****************************************************************************
 * define declaration
 *****************************************************************************/
#define CPU_EB_LOG_REQ 		1
#define CPU_EB_LOG_STOP 	2

#define PID_NONE (-1)

#define CPU_EB_LOG_STOP_MODE 	0
#define CPU_EB_LOG_RUN_MODE 	1
#define CPU_EB_LOG_DEBUG_MODE 	2

#define _cpu_eb_log_req_init(req, cmd, s, n, pf, p)	\
	do {						\
		INIT_LIST_HEAD(&req->list);		\
		req->cmd_type = cmd;			\
		req->src = s;				\
		req->num = n;				\
		req->on_fini_cb = pf;			\
		req->param = p;				\
	} while (0)

#define _cpu_eb_log_req_fini(req)			\
	do {						\
		if (req->on_fini_cb)			\
			req->on_fini_cb(req->param);	\
		kfree(req);				\
	} while (0)


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/
struct cpu_eb_log_req_q_t {
	struct list_head listq;
	struct mutex lockq;
	struct completion new_evt_comp;
	int closeq_flag;
} cpu_eb_log_req_q;

struct cpu_eb_log_req {
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

static void _cpu_eb_log_req_q_init(struct cpu_eb_log_req_q_t *q);
static void _cpu_eb_log_req_undeq(struct cpu_eb_log_req *req);
static int _cpu_eb_log_req_enq(struct cpu_eb_log_req *req);
static struct cpu_eb_log_req *_cpu_eb_log_req_deq(void);
static void _cpu_eb_log_req_open(void);
static int _cpu_eb_log_req_closed(void);
static int _cpu_eb_log_req_working(void);
static void *_cpu_eb_trace_seq_next(struct seq_file *seqf, loff_t *offset);

static void *cpu_eb_trace_seq_start(struct seq_file *seqf, loff_t *offset);
static void cpu_eb_trace_seq_stop(struct seq_file *seqf, void *p);
static void *cpu_eb_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset);
static int cpu_eb_trace_seq_show(struct seq_file *seqf, void *p);
static int cpu_eb_trace_open(struct inode *inode, struct file *fp);

static ssize_t cpu_eb_log_write_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);
static ssize_t cpu_eb_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf);
static ssize_t cpu_eb_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
void *cpu_eb_log_virt_addr;

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
dma_addr_t cpu_eb_log_phy_addr;
#else
unsigned int cpu_eb_log_phy_addr;
#endif

unsigned int cpu_eb_buffer_size;

extern int cpu_eb_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static int cpu_eb_trace_run;
static pid_t trace_owner_pid = PID_NONE;

static struct mutex lock_tracef;
static struct mutex lock_trace_owner_pid;

static struct completion log_start_comp;
static struct completion log_stop_comp;

static DEVICE_ATTR(cpu_eb_log_write, 0220, NULL, cpu_eb_log_write_store);
static DEVICE_ATTR(cpu_eb_log_run, 0664, cpu_eb_log_run_show, cpu_eb_log_run_store);;

static const struct seq_operations cpu_eb_trace_seq_ops = {
	.start = cpu_eb_trace_seq_start,
	.next = cpu_eb_trace_seq_next,
	.stop = cpu_eb_trace_seq_stop,
	.show = cpu_eb_trace_seq_show
};

static const struct file_operations cpu_eb_trace_fops = {
	.owner = THIS_MODULE,
	.open = cpu_eb_trace_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};


/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/
int cpu_eb_log_init(struct device *dev)
{
	int ret = 0;
	struct dentry *d;
	struct dentry *met_dir = NULL;
	phys_addr_t (*get_size_sym)(unsigned int id) = NULL;

	met_dir = dev_get_drvdata(dev);
	mutex_init(&lock_tracef);
	_cpu_eb_log_req_q_init(&cpu_eb_log_req_q);
	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);
	mutex_init(&lock_trace_owner_pid);

	d = debugfs_create_file("cpu_eb_trace", 0664, met_dir, NULL, &cpu_eb_trace_fops);
	if (!d) {
		PR_BOOTMSG("can not create devide node in debugfs: cpu_eb_trace\n");
		return -ENOMEM;
	}

	cpu_eb_trace_run = _cpu_eb_log_req_working();
	ret = device_create_file(dev, &dev_attr_cpu_eb_log_run);
	if (ret != 0) {
		PR_BOOTMSG("can not create device node: cpu_eb_log_run\n");
		return ret;
	}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
	cpu_eb_log_virt_addr = dma_alloc_coherent(dev, 0x800000,
				&cpu_eb_log_phy_addr, GFP_ATOMIC);
	if (cpu_eb_log_virt_addr) {
		cpu_eb_buffer_size = 0x800000;
		cpu_eb_buf_available = 1;
	} else {
		cpu_eb_buf_available = 0;
	}
#else
	get_size_sym = symbol_get(mcupm_reserve_mem_get_size);
	if (get_size_sym) {
		cpu_eb_buffer_size = get_size_sym(MCUPM_MET_ID);
		PR_BOOTMSG("cpu_eb_buffer_size=%x\n", cpu_eb_buffer_size);
	} else {
		PR_BOOTMSG("symbol_get mcupm_reserve_mem_get_size failure\n");
	}

	if (cpu_eb_buffer_size > 0) {
		phys_addr_t (*get_phys_sym)(unsigned int id) = NULL;
		phys_addr_t (*get_virt_sym)(unsigned int id) = NULL;

		get_phys_sym = symbol_get(mcupm_reserve_mem_get_virt);
		get_virt_sym = symbol_get(mcupm_reserve_mem_get_phys);
		if (get_phys_sym) {
			cpu_eb_log_virt_addr = (void*)get_phys_sym(MCUPM_MET_ID);
			PR_BOOTMSG("cpu_eb_log_virt_addr=%x\n", cpu_eb_log_virt_addr);
		} else {
			PR_BOOTMSG("symbol_get mcupm_reserve_mem_get_virt failure\n");
		}
		if (get_virt_sym) {
			cpu_eb_log_phy_addr = get_virt_sym(MCUPM_MET_ID);
			PR_BOOTMSG("cpu_eb_log_phy_addr=%x\n", cpu_eb_log_phy_addr);
		} else {
			PR_BOOTMSG("symbol_get mcupm_reserve_mem_get_phys failure\n");
		}
		cpu_eb_buf_available = 1;
	} else {
		cpu_eb_buf_available = 0;
	}
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */

	start_cpu_eb_ipi_recv_thread();

	return 0;
}


int cpu_eb_log_uninit(struct device *dev)
{
	stop_cpu_eb_ipi_recv_thread();

	if (cpu_eb_log_virt_addr != NULL) {
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
		dma_free_coherent(dev, cpu_eb_buffer_size, cpu_eb_log_virt_addr,
			cpu_eb_log_phy_addr);
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
		cpu_eb_log_virt_addr = NULL;
	}

	device_remove_file(dev, &dev_attr_cpu_eb_log_run);
	return 0;
}


int cpu_eb_log_start(void)
{
	int ret = 0;

	/* TODO: choose a better return value */
	if (_cpu_eb_log_req_working()) {
		return -EINVAL;
	}

	if (!_cpu_eb_log_req_closed()) {
		/*ret = down_killable(&log_start_sema);*/
		ret = wait_for_completion_killable(&log_start_comp);
		if (ret) {
			return ret;
		}
	}

	_cpu_eb_log_req_open();

	return 0;
}


int cpu_eb_log_stop(void)
{
	int ret = 0;
	struct cpu_eb_log_req *req = NULL;

	if (_cpu_eb_log_req_closed()) {
		return -EINVAL;
	}

	req = kmalloc(sizeof(*req), GFP_ATOMIC);
	_cpu_eb_log_req_init(req, CPU_EB_LOG_STOP, NULL, 0, _log_stop_cb, NULL);

	init_completion(&log_start_comp);
	init_completion(&log_stop_comp);

	ret = _cpu_eb_log_req_enq(req);
	if (ret) {
		return ret;
	}

	return wait_for_completion_killable(&log_stop_comp);
}


int cpu_eb_log_req_enq(
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param)
{
	struct cpu_eb_log_req *req = kmalloc(sizeof(*req), GFP_ATOMIC);

	_cpu_eb_log_req_init(req, CPU_EB_LOG_REQ, src, num, on_fini_cb, param);
	return _cpu_eb_log_req_enq(req);
}


int cpu_eb_parse_num(const char *str, unsigned int *value, int len)
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


static void _cpu_eb_log_req_q_init(struct cpu_eb_log_req_q_t *q)
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
static void _cpu_eb_log_req_undeq(struct cpu_eb_log_req *req)
{
	mutex_lock(&cpu_eb_log_req_q.lockq);
	list_add(&req->list, &cpu_eb_log_req_q.listq);
	mutex_unlock(&cpu_eb_log_req_q.lockq);

	complete(&cpu_eb_log_req_q.new_evt_comp);
}


static int _cpu_eb_log_req_enq(struct cpu_eb_log_req *req)
{
	mutex_lock(&cpu_eb_log_req_q.lockq);
	if (cpu_eb_log_req_q.closeq_flag) {
		mutex_unlock(&cpu_eb_log_req_q.lockq);
		return -EBUSY;
	}

	list_add_tail(&req->list, &cpu_eb_log_req_q.listq);
	if (req->cmd_type == CPU_EB_LOG_STOP) {
		cpu_eb_log_req_q.closeq_flag = 1;
	}
	mutex_unlock(&cpu_eb_log_req_q.lockq);

	complete(&cpu_eb_log_req_q.new_evt_comp);

	return 0;
}


static struct cpu_eb_log_req *_cpu_eb_log_req_deq(void)
{
	struct cpu_eb_log_req *ret_req;

	if (_down_freezable_interruptible(&cpu_eb_log_req_q.new_evt_comp)) {
		return NULL;
	}

	mutex_lock(&cpu_eb_log_req_q.lockq);
	ret_req = list_entry(cpu_eb_log_req_q.listq.next, struct cpu_eb_log_req, list);
	list_del_init(&ret_req->list);
	mutex_unlock(&cpu_eb_log_req_q.lockq);

	return ret_req;
}


static void _cpu_eb_log_req_open(void)
{
	mutex_lock(&cpu_eb_log_req_q.lockq);
	cpu_eb_log_req_q.closeq_flag = 0;
	mutex_unlock(&cpu_eb_log_req_q.lockq);
}


static int _cpu_eb_log_req_closed(void)
{
	int ret = 0;

	mutex_lock(&cpu_eb_log_req_q.lockq);
	ret = cpu_eb_log_req_q.closeq_flag && list_empty(&cpu_eb_log_req_q.listq);
	mutex_unlock(&cpu_eb_log_req_q.lockq);

	return ret;
}


static int _cpu_eb_log_req_working(void)
{
	int ret = 0;

	mutex_lock(&cpu_eb_log_req_q.lockq);
	ret = !cpu_eb_log_req_q.closeq_flag;
	mutex_unlock(&cpu_eb_log_req_q.lockq);

	return ret;
}


static void *_cpu_eb_trace_seq_next(struct seq_file *seqf, loff_t *offset)
{
	struct cpu_eb_log_req *next_req;

	if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
		PR_BOOTMSG("_cpu_eb_trace_seq_next: pid: %d\n", current->pid);
	}

	if (_cpu_eb_log_req_closed()) {
		return NULL;
	}

	next_req = _cpu_eb_log_req_deq();
	if (next_req == NULL) {
		return NULL;
	}

	if (next_req->cmd_type == CPU_EB_LOG_STOP) {
		_cpu_eb_log_req_fini(next_req);
		return NULL;
	}

	return (void *) next_req;
}


static void *cpu_eb_trace_seq_start(struct seq_file *seqf, loff_t *offset)
{
	void *ret;

	if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
		PR_BOOTMSG("cpu_eb_trace_seq_start: locked_pid: %d, pid: %d, offset: %llu\n",
			 trace_owner_pid, current->pid, *offset);
	}

	if (!mutex_trylock(&lock_tracef)) {
		return NULL;
	}

	mutex_lock(&lock_trace_owner_pid);
	trace_owner_pid = current->pid;
	current->flags |= PF_NOFREEZE;
	mutex_unlock(&lock_trace_owner_pid);

	ret = _cpu_eb_trace_seq_next(seqf, offset);

	return ret;
}


static void cpu_eb_trace_seq_stop(struct seq_file *seqf, void *p)
{
	if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
		PR_BOOTMSG("cpu_eb_trace_seq_stop: pid: %d\n", current->pid);
	}

	mutex_lock(&lock_trace_owner_pid);
	if (current->pid == trace_owner_pid) {
		trace_owner_pid = PID_NONE;
		mutex_unlock(&lock_tracef);
	}
	mutex_unlock(&lock_trace_owner_pid);
}


static void *cpu_eb_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset)
{
	if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
		PR_BOOTMSG("cpu_eb_trace_seq_next: pid: %d\n", current->pid);
	}
	(*offset)++;
	return _cpu_eb_trace_seq_next(seqf, offset);
}


static int cpu_eb_trace_seq_show(struct seq_file *seqf, void *p)
{
	struct cpu_eb_log_req *req = (struct cpu_eb_log_req *) p;
	size_t l_sz;
	size_t r_sz;
	struct cpu_eb_log_req *l_req;
	struct cpu_eb_log_req *r_req;
	int ret = 0;

	if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
		PR_BOOTMSG("cpu_eb_trace_seq_show: pid: %d\n", current->pid);
	}

	if (req->num >= seqf->size) {
		l_req = kmalloc(sizeof(*req), GFP_ATOMIC);
		r_req = req;

		l_sz = seqf->size >> 1;
		r_sz = req->num - l_sz;
		_cpu_eb_log_req_init(l_req, CPU_EB_LOG_REQ, req->src, l_sz, NULL, NULL);
		_cpu_eb_log_req_init(r_req, CPU_EB_LOG_REQ, req->src + l_sz,
					r_sz, req->on_fini_cb, req->param);

		_cpu_eb_log_req_undeq(r_req);
		req = l_req;

		if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
			PR_BOOTMSG("cpu_eb_trace_seq_show: split request\n");
		}
	}

	ret = seq_write(seqf, req->src, req->num);
	if (ret) {
		/* check if seq_file buffer overflows */
		if (seqf->count == seqf->size) {
			_cpu_eb_log_req_undeq(req);
		} else {
			if (cpu_eb_trace_run == CPU_EB_LOG_DEBUG_MODE) {
				PR_BOOTMSG("cpu_eb_trace_seq_show: \
					reading trace record failed, \
					some data may be lost or corrupted\n");
			}
			_cpu_eb_log_req_fini(req);
		}
		return 0;
	}

	_cpu_eb_log_req_fini(req);
	return 0;
}


static int cpu_eb_trace_open(struct inode *inode, struct file *fp)
{
	return seq_open(fp, &cpu_eb_trace_seq_ops);
}


static ssize_t cpu_eb_log_write_store(
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
	cpu_eb_log_req_enq(plog, strnlen(plog, count), kfree, plog);
	mutex_unlock(&dev->mutex);

	return count;
}


static ssize_t cpu_eb_log_run_show(
	struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int sz;

	mutex_lock(&dev->mutex);
	sz = snprintf(buf, PAGE_SIZE, "%d\n", cpu_eb_trace_run);
	mutex_unlock(&dev->mutex);
	return sz;
}


static ssize_t cpu_eb_log_run_store(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	int ret = 0;
	int prev_run_state;

	mutex_lock(&dev->mutex);

	prev_run_state = cpu_eb_trace_run;
	if (kstrtoint(buf, 10, &cpu_eb_trace_run) != 0) {
		return -EINVAL;
	}

	if (cpu_eb_trace_run <= CPU_EB_LOG_STOP_MODE) {
		cpu_eb_trace_run = CPU_EB_LOG_STOP_MODE;
		cpu_eb_log_stop();

		if (prev_run_state == CPU_EB_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_cpu_eb_log_write);
		}
	} else if (cpu_eb_trace_run == CPU_EB_LOG_RUN_MODE) {
		cpu_eb_trace_run = CPU_EB_LOG_RUN_MODE;
		cpu_eb_log_start();

		if (prev_run_state == CPU_EB_LOG_DEBUG_MODE) {
			device_remove_file(dev, &dev_attr_cpu_eb_log_write);
		}
	} else {
		cpu_eb_trace_run = CPU_EB_LOG_DEBUG_MODE;
		cpu_eb_log_start();

		if (prev_run_state != CPU_EB_LOG_DEBUG_MODE) {
			ret = device_create_file(dev, &dev_attr_cpu_eb_log_write);
			if (ret != 0)  {
				PR_BOOTMSG("can not create device node: \
					cpu_eb_log_write\n");
			}
		}
	}

	mutex_unlock(&dev->mutex);

	return count;
}

