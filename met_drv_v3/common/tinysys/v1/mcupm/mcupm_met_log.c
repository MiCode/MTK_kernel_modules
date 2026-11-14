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
#include <linux/version.h>

#include "interface.h"
#include "core_plf_init.h"

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
	bool closeq_flag;
};
struct mcupm_log_req_q_t *mcupm_log_req_q;

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
static void _mcupm_log_req_undeq(struct mcupm_log_req *req, struct mcupm_log_req_q_t *q);
static int _mcupm_log_req_enq(struct mcupm_log_req *req, struct mcupm_log_req_q_t *q);
static struct mcupm_log_req *_mcupm_log_req_deq(struct mcupm_log_req_q_t *q);
static void _mcupm_log_req_open(struct mcupm_log_req_q_t *q);
static bool _is_mcupm_log_req_closed_and_empty(struct mcupm_log_req_q_t *q);
static bool _is_mcupm_log_req_closed(struct mcupm_log_req_q_t *q);
static void *_mcupm_trace_seq_next(unsigned int mcupm_no, struct seq_file *seqf, loff_t *offset);
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
unsigned int mcupm_count = 0;
void **mcupm_log_virt_addr;

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
dma_addr_t mcupm_log_phy_addr;
#else
phys_addr_t *mcupm_log_phy_addr;
#endif

unsigned int *mcupm_buffer_size;

int *mcupm_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static int mcupm_trace_run;
static pid_t *trace_owner_pid;

static struct mutex *lock_tracef;
static struct mutex *lock_trace_owner_pid;

static struct completion *log_start_comp;
static struct completion *log_stop_comp;

static unsigned int *mcupm_reg_cb_no;

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
static struct dentry **trace_dentry;
#else
static struct proc_dir_entry **trace_dentry;
#endif

/*****************************************************************************
 * external function ipmlement
 *****************************************************************************/

static int _alloc_all_mcupm_para(void)
{

	trace_owner_pid = kmalloc_array(mcupm_count, sizeof(pid_t), GFP_KERNEL);
	if (!trace_owner_pid) {
		PR_BOOTMSG("can not allocate trace_owner_pid\n");
		return -ENOMEM;
	}

	lock_tracef = kmalloc_array(mcupm_count, sizeof(struct mutex), GFP_KERNEL);
	if (!lock_tracef) {
		PR_BOOTMSG("can not allocate lock_tracef\n");
		return -ENOMEM;
	}

	lock_trace_owner_pid = kmalloc_array(mcupm_count, sizeof(struct mutex), GFP_KERNEL);
	if (!lock_trace_owner_pid) {
		PR_BOOTMSG("can not allocate lock_trace_owner_pid\n");
		return -ENOMEM;
	}

	log_start_comp = kmalloc_array(mcupm_count, sizeof(struct completion), GFP_KERNEL);
	if (!log_start_comp) {
		PR_BOOTMSG("can not allocate log_start_comp\n");
		return -ENOMEM;
	}

	log_stop_comp = kmalloc_array(mcupm_count, sizeof(struct completion), GFP_KERNEL);
	if (!log_stop_comp) {
		PR_BOOTMSG("can not allocate log_stop_comp\n");
		return -ENOMEM;
	}

	mcupm_reg_cb_no = kmalloc_array(mcupm_count, sizeof(unsigned int), GFP_KERNEL);
	if (!mcupm_reg_cb_no) {
		PR_BOOTMSG("can not allocate mcupm_reg_cb_no\n");
		return -ENOMEM;
	}

	for (int mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		mcupm_reg_cb_no[mcupm_no] = mcupm_no;
		
		mutex_init(&lock_tracef[mcupm_no]);
		mutex_init(&lock_trace_owner_pid[mcupm_no]);
		init_completion(&log_start_comp[mcupm_no]);
		init_completion(&log_stop_comp[mcupm_no]);
	}

	mcupm_log_req_q = kmalloc_array(mcupm_count, sizeof(struct mcupm_log_req_q_t), GFP_KERNEL);
	if (!mcupm_log_req_q) {
		PR_BOOTMSG("can not allocate mcupm_log_req_q\n");
		return -ENOMEM;
	}
	
	// Coverity issue : SIZEOF_MISMATCH
	mcupm_log_virt_addr = kmalloc_array(mcupm_count, sizeof(phys_addr_t *), GFP_KERNEL);
	if (!mcupm_log_virt_addr) {
		PR_BOOTMSG("can not allocate mcupm_log_virt_addr\n");
		return -ENOMEM;
	}

#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
	dma_addr_t mcupm_log_phy_addr;
#else
	mcupm_log_phy_addr = kmalloc_array(mcupm_count, sizeof(phys_addr_t), GFP_KERNEL);
	if (!mcupm_log_phy_addr) {
		PR_BOOTMSG("can not allocate mcupm_log_phy_addr\n");
		return -ENOMEM;
	}
#endif

	mcupm_buffer_size = kmalloc_array(mcupm_count, sizeof(unsigned int), GFP_KERNEL);
	if (!mcupm_buffer_size) {
		PR_BOOTMSG("can not allocate mcupm_buffer_size\n");
		return -ENOMEM;
	}

	mcupm_buf_available = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_buf_available) {
		PR_BOOTMSG("can not allocate mcupm_buf_available\n");
		return -ENOMEM;
	}

#ifdef ONDIEMET_MOUNT_DEBUGFS
	trace_dentry = kmalloc_array(mcupm_count, sizeof(struct dentry *), GFP_KERNEL);
#else
	trace_dentry = kmalloc_array(mcupm_count, sizeof(struct proc_dir_entry *), GFP_KERNEL);
#endif
	if (!trace_dentry) {
		PR_BOOTMSG("can not allocate trace_dentry\n");
		return -ENOMEM;
	}

	return _alloc_all_ipi_thread_para();
}


static int _init_mcupm_para_and_queue(int mcupm_no, struct device *dev)
{
	struct device_node *np;
	char _name_buf[50] = {0};
    int ret = 0;
#ifdef ONDIEMET_MOUNT_DEBUGFS
	struct dentry *met_dir = NULL;
#else
	struct proc_dir_entry *met_dir = NULL;
#endif
	met_dir = dev_get_drvdata(dev);

	_mcupm_log_req_q_init(&mcupm_log_req_q[mcupm_no]);

	if (mcupm_no == 0) {
		ret = snprintf(_name_buf, sizeof(_name_buf), "mcupm_trace");
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			PR_BOOTMSG("Error in snprintf for mcupm_trace\n");
			return -ENOMEM;
		}
	} else {
		ret = snprintf(_name_buf, sizeof(_name_buf), "mcupm_slv_%d_trace", mcupm_no - 1);
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			PR_BOOTMSG("Error in snprintf for mcupm_slv_%d_trace\n", mcupm_no - 1);
			return -ENOMEM;
		}
	}

#ifdef ONDIEMET_MOUNT_DEBUGFS
	trace_dentry[mcupm_no] = debugfs_create_file(_name_buf, 0600, met_dir, NULL, &mcupm_trace_fops);
	if (!trace_dentry[mcupm_no]) {
		PR_BOOTMSG("can not create devide node in debugfs: %s\n", _name_buf);
		return -ENOMEM;
	}
#else
	trace_dentry[mcupm_no] = proc_create_data(_name_buf, 0600, met_dir, &mcupm_trace_fops, (void *)(uintptr_t)mcupm_no);
	if (!trace_dentry[mcupm_no]) {
		PR_BOOTMSG("can not create devide node in procfs: %s\n", _name_buf);
		return -ENOMEM;
	}
#endif

	if (true == _is_mcupm_log_req_closed(&mcupm_log_req_q[mcupm_no])) {
		mcupm_trace_run = 0;
	} else {
		mcupm_trace_run = 1;
	}
	trace_owner_pid[mcupm_no] = PID_NONE;


#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
	mcupm_log_virt_addr[mcupm_no] = dma_alloc_coherent(dev, 0x800000,
				&mcupm_log_phy_addr[mcupm_no], GFP_ATOMIC);
	if (mcupm_log_virt_addr[mcupm_no]) {
		mcupm_buffer_size[mcupm_no] = 0x800000;
		mcupm_buf_available[mcupm_no] = 1;
	} else {
		mcupm_buf_available[mcupm_no] = 0;
	}
#elif defined(MET_MCUPM)
	mcupm_buf_available[mcupm_no] = 0;
	if (mcupm_no == 0) {
		//Coverity issue : CERT POS54-C,CERT ERR33-C
		int ret = snprintf(_name_buf, sizeof(_name_buf), "met-res-ram-mcupm");
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			/* Handle error, e.g., log and exit or return error */
			PR_BOOTMSG("Error in snprintf for met-res-ram-mcupm\n");
			return -1;
		}
	} else {
		int ret = snprintf(_name_buf, sizeof(_name_buf), "met-res-ram-mcupm-slv-%d", mcupm_no - 1);
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			/* Handle error, e.g., log and exit or return error */
			PR_BOOTMSG("Error in snprintf for met-res-ram-mcupm-slv-%d\n", mcupm_no - 1);
			return -1;
		}
	}

	np = of_find_node_by_name(NULL, _name_buf);
	if (!np) {
		pr_debug("unable to find %s\n", _name_buf);
		return 0;
	}
	of_property_read_u64(np, "start", &mcupm_log_phy_addr[mcupm_no]);
	of_property_read_u32(np, "size", &mcupm_buffer_size[mcupm_no]);

	if ((mcupm_log_phy_addr[mcupm_no] > 0) && (mcupm_buffer_size[mcupm_no] > 0)) {
		mcupm_log_virt_addr[mcupm_no] = (void*)ioremap_wc(mcupm_log_phy_addr[mcupm_no], mcupm_buffer_size[mcupm_no]);

		mcupm_buf_available[mcupm_no] = 1;
	} else {
		mcupm_buf_available[mcupm_no] = 0;
	}

	// When there is ONLY ONE MCUPM, MET can reserve memory by the API.
	if ((mcupm_count == 1) && (mcupm_buf_available[0] == 0)) {
		if(mcupm_reserve_mem_get_size_symbol){
			mcupm_buffer_size[0] = mcupm_reserve_mem_get_size_symbol(MCUPM_MET_ID);
			PR_BOOTMSG("mcupm_buffer_size=%x\n", mcupm_buffer_size[0]);
		}else {
			PR_BOOTMSG("[MET] [%s,%d] mcupm_reserve_mem_get_size is not linked!\n", __FILE__, __LINE__);
			return -1;
		}
		if (mcupm_buffer_size[0] > 0) {
			if(mcupm_reserve_mem_get_virt_symbol){
				mcupm_log_virt_addr[0] = (void*)mcupm_reserve_mem_get_virt_symbol(MCUPM_MET_ID);
				PR_BOOTMSG("mcupm_log_virt_addr=%p\n", mcupm_log_virt_addr[0]);
			}else {
				PR_BOOTMSG("[MET] [%s,%d] mcupm_reserve_mem_get_virt is not linked!\n", __FILE__, __LINE__);
				return -1;
			}
			if(mcupm_reserve_mem_get_phys_symbol){
				mcupm_log_phy_addr[0] = mcupm_reserve_mem_get_phys_symbol(MCUPM_MET_ID);
				PR_BOOTMSG("mcupm_log_phy_addr=%u\n", (unsigned int) mcupm_log_phy_addr[0]);
			}else {
				PR_BOOTMSG("[MET] [%s,%d] mcupm_reserve_mem_get_phys is not linked!\n", __FILE__, __LINE__);
				return -1;
			}

			if ((mcupm_log_phy_addr[0] > 0) && (mcupm_log_virt_addr[0] > 0)) {
				mcupm_buf_available[0] = 1;
			} else {
				mcupm_buf_available[0] = 0;
			}
		}else {
			PR_BOOTMSG("mcupm_reserve_mem_get_size fail\n");
		}
	}
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE || defined(CONFIG_MTK_MET_MEM_ALLOC) */

	start_mcupm_ipi_recv_thread(mcupm_no);

	return 0;
}

static int count_met_res_ram_mcupm_nodes(void)
{
	struct device_node *res_ram_root = NULL, *child = NULL;
	int _mcupm_count = 0;

	res_ram_root = of_find_node_by_name(NULL, "met-res-ram");
	if (!res_ram_root) {
		pr_debug("unable to find res_ram_root\n");
		return 0;
	}

	for_each_available_child_of_node(res_ram_root, child) {
		if (child->name != NULL) {
			if (strncmp(child->name, "met-res-ram-mcupm", strlen("met-res-ram-mcupm")) == 0) {
				_mcupm_count++;
			}
		}
	}
	of_node_put(res_ram_root);

	return _mcupm_count;
}


int mcupm_log_init(struct device *dev)
{
	int ret = 0;
	int mcupm_no = 0;

	mcupm_count = count_met_res_ram_mcupm_nodes();
	if (mcupm_count == 0) {
		PR_BOOTMSG("No reserved mcupm ram @ %s\n", __FUNCTION__);
		return -ENODEV;;
	}

	ret = _alloc_all_mcupm_para();
	if (ret != 0) {
		PR_BOOTMSG("initialize fail @ %s\n", __FUNCTION__);
		return ret;
	}

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		ret = _init_mcupm_para_and_queue(mcupm_no, dev);
		if (ret != 0) {
			PR_BOOTMSG("initialize fail @ %s\n", __FUNCTION__);
			return ret;
		}
	}


	// mcupm_log_run is only one entry point accessed by tinysys_log_writer
	ret = device_create_file(dev, &dev_attr_mcupm_log_run);
	if (ret != 0) {
		PR_BOOTMSG("can not create device node: mcupm_log_run\n");
		return ret;
	}

	return 0;
}


int mcupm_log_uninit(struct device *dev)
{
	int mcupm_no = 0;
	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		stop_mcupm_ipi_recv_thread(mcupm_no);

		if (mcupm_log_virt_addr[mcupm_no] != NULL) {
			mcupm_log_virt_addr[mcupm_no] = NULL;
#if defined(CONFIG_MTK_GMO_RAM_OPTIMIZE) || defined(CONFIG_MTK_MET_MEM_ALLOC)
			dma_free_coherent(dev, mcupm_buffer_size[0], mcupm_log_virt_addr[0],
				mcupm_log_phy_addr[0]);
#endif /* CONFIG_MTK_GMO_RAM_OPTIMIZE */
		}
#ifdef ONDIEMET_MOUNT_DEBUGFS
			debugfs_remove(trace_dentry[mcupm_no]);
#else
			proc_remove(trace_dentry[mcupm_no]);
#endif
	}

	device_remove_file(dev, &dev_attr_mcupm_log_run);

	return 0;
}


int mcupm_log_start(void)
{
	int ret = 0;
	int mcupm_no = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		/* TODO: choose a better return value */
		if (false == _is_mcupm_log_req_closed(&mcupm_log_req_q[mcupm_no])) {
			return -EINVAL;
		}

		if (false == _is_mcupm_log_req_closed_and_empty(&mcupm_log_req_q[mcupm_no])) {
			/*ret = down_killable(&log_start_sema);*/
			ret = wait_for_completion_killable(&log_start_comp[mcupm_no]);
			if (ret) {
				return ret;
			}
		}

		_mcupm_log_req_open(&mcupm_log_req_q[mcupm_no]);
	}

	return 0;
}


int mcupm_log_stop(void)
{
	int ret = 0;
	struct mcupm_log_req *req = NULL;
	int mcupm_no = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		if (true == _is_mcupm_log_req_closed_and_empty(&mcupm_log_req_q[mcupm_no])) {
			return -EINVAL;
		}

		req = kmalloc(sizeof(*req), GFP_ATOMIC);
		if (req == NULL) {
			return -EINVAL;
		}
		_mcupm_log_req_init(req, MUCPM_LOG_STOP, NULL, 0, _log_stop_cb, &mcupm_reg_cb_no[mcupm_no]);

		init_completion(&log_start_comp[mcupm_no]);
		init_completion(&log_stop_comp[mcupm_no]);

		ret = _mcupm_log_req_enq(req, &mcupm_log_req_q[mcupm_no]);
		if (ret) {
			return ret;
		}

		ret = wait_for_completion_killable(&log_stop_comp[mcupm_no]);
	}

	return ret;
}


int mcupm_log_req_enq(
	unsigned int mcupm_no,
	const char *src, size_t num,
	void (*on_fini_cb)(const void *p),
	const void *param)
{
	struct mcupm_log_req *req = kmalloc(sizeof(*req), GFP_ATOMIC);

	if (req == NULL) {
		return -EINVAL;
	}

	_mcupm_log_req_init(req, MUCPM_LOG_REQ, src, num, on_fini_cb, param);
	return _mcupm_log_req_enq(req, &mcupm_log_req_q[mcupm_no]);
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
	unsigned int mcupm_no = *(unsigned int *)p;

	complete(&log_start_comp[mcupm_no]);
	complete(&log_stop_comp[mcupm_no]);
}


static int _down_freezable_interruptible(struct completion *comp)
{
	int ret = 0;

#if KERNEL_VERSION(6, 1, 0) <= LINUX_VERSION_CODE
	ret = wait_for_completion_state(comp,
			TASK_INTERRUPTIBLE|TASK_FREEZABLE);
#else
	freezer_do_not_count();
	ret = wait_for_completion_interruptible(comp);
	freezer_count();
#endif

	return ret;
}


static void _mcupm_log_req_q_init(struct mcupm_log_req_q_t *q)
{
	INIT_LIST_HEAD(&q->listq);
	mutex_init(&q->lockq);
	init_completion(&q->new_evt_comp);
	q->closeq_flag = true;
}


/*
	undequeue is seen as a roll-back operation,
	so it can be done even when the queue is closed
*/
static void _mcupm_log_req_undeq(struct mcupm_log_req *req, struct mcupm_log_req_q_t *q)
{
	mutex_lock(&q->lockq);
	list_add(&req->list, &q->listq);
	mutex_unlock(&q->lockq);

	complete(&q->new_evt_comp);
}


static int _mcupm_log_req_enq(struct mcupm_log_req *req, struct mcupm_log_req_q_t *q)
{
	mutex_lock(&q->lockq);
	if (q->closeq_flag) {
		mutex_unlock(&q->lockq);
		return -EBUSY;
	}

	list_add_tail(&req->list, &q->listq);
	if (req->cmd_type == MUCPM_LOG_STOP) {
		q->closeq_flag = true;
	}
	mutex_unlock(&q->lockq);

	complete(&q->new_evt_comp);

	return 0;
}


static struct mcupm_log_req *_mcupm_log_req_deq(struct mcupm_log_req_q_t *q)
{
	struct mcupm_log_req *ret_req;

	if (_down_freezable_interruptible(&q->new_evt_comp)) {
		return NULL;
	}

	mutex_lock(&q->lockq);
	ret_req = list_entry(q->listq.next, struct mcupm_log_req, list);
	list_del_init(&ret_req->list);
	mutex_unlock(&q->lockq);

	return ret_req;
}


static void _mcupm_log_req_open(struct mcupm_log_req_q_t *q)
{
	mutex_lock(&q->lockq);
	q->closeq_flag = false;
	mutex_unlock(&q->lockq);
}


bool _is_mcupm_log_req_closed_and_empty(struct mcupm_log_req_q_t *q)
{
	bool ret = 0;

	mutex_lock(&q->lockq);
	ret = q->closeq_flag && list_empty(&q->listq);
	mutex_unlock(&q->lockq);

	return ret;
}


bool _is_mcupm_log_req_closed(struct mcupm_log_req_q_t *q)
{
	bool ret = 0;

	mutex_lock(&q->lockq);
	ret = q->closeq_flag;
	mutex_unlock(&q->lockq);

	return ret;
}

static void *_mcupm_trace_seq_next(unsigned int mcupm_no, struct seq_file *seqf, loff_t *offset)
{
	struct mcupm_log_req *next_req;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("_mcupm_trace_seq_next: pid: %d\n", current->pid);
	}

	if (true == _is_mcupm_log_req_closed_and_empty(&mcupm_log_req_q[mcupm_no])) {
		return NULL;
	}

	next_req = _mcupm_log_req_deq(&mcupm_log_req_q[mcupm_no]);
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
	const struct file *file = seqf->file;
	unsigned int mcupm_no = 0;
	mcupm_no = (unsigned int)(uintptr_t)file->f_inode->i_private;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_start: locked_pid: %d, pid: %d, offset: %llu\n",
			 trace_owner_pid[mcupm_no], current->pid, *offset);
	}

	if (!mutex_trylock(&lock_tracef[mcupm_no])) {
		return NULL;
	}

	mutex_lock(&lock_trace_owner_pid[mcupm_no]);
	trace_owner_pid[mcupm_no] = current->pid;
	current->flags |= PF_NOFREEZE;
	mutex_unlock(&lock_trace_owner_pid[mcupm_no]);

	ret = _mcupm_trace_seq_next(mcupm_no, seqf, offset);

	return ret;
}


static void mcupm_trace_seq_stop(struct seq_file *seqf, void *p)
{
	const struct file *file = seqf->file;
	unsigned int mcupm_no = 0;
	mcupm_no = (unsigned int)(uintptr_t)file->f_inode->i_private;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_stop: pid: %d\n", current->pid);
	}

	mutex_lock(&lock_trace_owner_pid[mcupm_no]);
	if (current->pid == trace_owner_pid[mcupm_no]) {
		trace_owner_pid[mcupm_no] = PID_NONE;
		mutex_unlock(&lock_tracef[mcupm_no]);
	}
	mutex_unlock(&lock_trace_owner_pid[mcupm_no]);
}


static void *mcupm_trace_seq_next(struct seq_file *seqf, void *p, loff_t *offset)
{
	const struct file *file = seqf->file;
	unsigned int mcupm_no = 0;
	mcupm_no = (unsigned int)(uintptr_t)file->f_inode->i_private;

	if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
		PR_BOOTMSG("mcupm_trace_seq_next: pid: %d\n", current->pid);
	}
	(*offset)++;
	return _mcupm_trace_seq_next(mcupm_no, seqf, offset);
}


static int mcupm_trace_seq_show(struct seq_file *seqf, void *p)
{
	struct mcupm_log_req *req = (struct mcupm_log_req *) p;
	size_t l_sz;
	size_t r_sz;
	struct mcupm_log_req *l_req;
	struct mcupm_log_req *r_req;
	int ret = 0;

	const struct file *file = seqf->file;
	unsigned int mcupm_no = 0;
	mcupm_no = (unsigned int)(uintptr_t)file->f_inode->i_private;

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

		_mcupm_log_req_undeq(r_req, &mcupm_log_req_q[mcupm_no]);
		req = l_req;

		if (mcupm_trace_run == MUCPM_LOG_DEBUG_MODE) {
			PR_BOOTMSG("mcupm_trace_seq_show: split request\n");
		}
	}

	ret = seq_write(seqf, req->src, req->num);
	if (ret) {
		/* check if seq_file buffer overflows */
		if (seqf->count == seqf->size) {
			_mcupm_log_req_undeq(req, &mcupm_log_req_q[mcupm_no]);
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
	int mcupm_no = 0;
	char *plog = NULL;

	plog = kmalloc_array(count+1, sizeof(*plog), GFP_ATOMIC);
	if (!plog) {
		/* TODO: use a better error code */
		return -EINVAL;
	}

	strscpy(plog, buf, count+1);

	mutex_lock(&dev->mutex);
	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		mcupm_log_req_enq(mcupm_no, plog, strnlen(plog, count), kfree, plog);
	}
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

