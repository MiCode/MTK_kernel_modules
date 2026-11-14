// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
/*****************************************************************************
 * headers
 *****************************************************************************/
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/signal.h>
#include <linux/semaphore.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/io.h>  /* for ioremap and iounmap */

#include "mtk_tinysys_ipi.h"  /* for mtk_ipi_device */
#include "mcupm_ipi_id.h"  /* for mcupm_ipidev */
/* #include "mcupm_ipi_table.h" */

#include "mcupm_met_log.h"
#include "mcupm_met_ipi_handle.h"
#include "tinysys_mcupm.h" /* for mcupm_ipidev_symbol */
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "interface.h"
#include "core_plf_init.h"
#include "met_kernel_symbol.h"


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
extern unsigned int met_get_chip_id(void);
extern char *met_get_platform(void);

/*****************************************************************************
 * internal function declaration
 *****************************************************************************/
static void _log_done_cb(const void *p);
static int _met_ipi_cb(
	unsigned int ipi_id,
	void *prdata,
	void *data,
	unsigned int len);
static int _mcupm_recv_thread(void *data);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
extern int *mcupm_buf_available;
extern unsigned int mcupm_count;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static struct mtk_ipi_device **mcupm_ipidev_symbol;
static unsigned int **recv_buf;
static unsigned int **recv_buf_copy;
static unsigned int *ackdata;
static unsigned int *reply_data;
static unsigned int *log_size;
static struct task_struct **_mcupm_recv_task;
static int *mcupm_ipi_thread_started;
static int *mcupm_buffer_dumping;
static int *mcupm_recv_thread_comp;
static int *mcupm_ipi_cb_no;
static int *mcupm_kthread_no;
static int mcupm_run_mode = MCUPM_RUN_NORMAL;
unsigned int wait_for_file_write;

/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
int _alloc_all_ipi_thread_para(void)
{
	int mcupm_no = 0;

	mcupm_ipidev_symbol = kmalloc_array(mcupm_count, sizeof(struct mtk_ipi_device *), GFP_KERNEL);
	if (!mcupm_ipidev_symbol) {
		PR_BOOTMSG("can not allocate mcupm_ipidev_symbol\n");
		return -ENOMEM;
	}

	ackdata = kmalloc_array(mcupm_count, sizeof(unsigned int), GFP_KERNEL);
	if (!ackdata) {
		PR_BOOTMSG("can not allocate ackdata\n");
		return -ENOMEM;
	}

	reply_data = kmalloc_array(mcupm_count, sizeof(unsigned int), GFP_KERNEL);
	if (!reply_data) {
		PR_BOOTMSG("can not allocate reply_data\n");
		return -ENOMEM;
	}

	log_size = kmalloc_array(mcupm_count, sizeof(unsigned int), GFP_KERNEL);
	if (!log_size) {
		PR_BOOTMSG("can not allocate log_size\n");
		return -ENOMEM;
	}

	_mcupm_recv_task = kmalloc_array(mcupm_count, sizeof(struct task_struct *), GFP_KERNEL);
	if (!_mcupm_recv_task) {
		PR_BOOTMSG("can not allocate _mcupm_recv_task\n");
		return -ENOMEM;
	}

	mcupm_ipi_thread_started = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_ipi_thread_started) {
		PR_BOOTMSG("can not allocate mcupm_ipi_thread_started\n");
		return -ENOMEM;
	}

	mcupm_buffer_dumping = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_buffer_dumping) {
		PR_BOOTMSG("can not allocate mcupm_buffer_dumping\n");
		return -ENOMEM;
	}

	mcupm_recv_thread_comp = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_recv_thread_comp) {
		PR_BOOTMSG("can not allocate mcupm_recv_thread_comp\n");
		return -ENOMEM;
	}

	mcupm_ipi_cb_no = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_ipi_cb_no) {
		PR_BOOTMSG("can not allocate mcupm_ipi_cb_no\n");
		return -ENOMEM;
	}

	mcupm_kthread_no = kmalloc_array(mcupm_count, sizeof(int), GFP_KERNEL);
	if (!mcupm_kthread_no) {
		PR_BOOTMSG("can not allocate mcupm_kthread_no\n");
		return -ENOMEM;
	}

	recv_buf = (unsigned int **)kmalloc_array(mcupm_count, sizeof(unsigned int *), GFP_KERNEL);
	if (!recv_buf) {
		PR_BOOTMSG("can not allocate recv_buf\n");
		return -ENOMEM;
	}

	recv_buf_copy = (unsigned int **)kmalloc_array(mcupm_count, sizeof(unsigned int *), GFP_KERNEL);
	if (!recv_buf_copy) {
		PR_BOOTMSG("can not allocate recv_buf_copy\n");
		return -ENOMEM;
	}

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		mcupm_ipi_cb_no[mcupm_no] = mcupm_no;
		mcupm_kthread_no[mcupm_no] = mcupm_no;

		recv_buf[mcupm_no] = (unsigned int *)kmalloc_array(4, sizeof(unsigned int), GFP_KERNEL);
		if (!recv_buf[mcupm_no]) {
			PR_BOOTMSG("can not allocate recv_buf[%d]\n", mcupm_no);
			return -ENOMEM;
		}

		recv_buf_copy[mcupm_no] = (unsigned int *)kmalloc_array(4, sizeof(unsigned int), GFP_KERNEL);
		if (!recv_buf_copy[mcupm_no]) {
			PR_BOOTMSG("can not allocate recv_buf_copy[%d]\n", mcupm_no);
			return -ENOMEM;
		}
	}

	return 0;
}
 
void start_mcupm_ipi_recv_thread(int mcupm_no)
{
	int ret = 0;
	char _name_buf[50] = {0};

	if (mcupm_ipidev_symbol[mcupm_no] == NULL) {
#ifdef GET_MCUPM_IPIDEV
		mcupm_ipidev_symbol[mcupm_no] = GET_MCUPM_IPIDEV(mcupm_no);
#else
		mcupm_ipidev_symbol[mcupm_no] = get_mcupm_ipidev_symbol();
#endif
		if (mcupm_ipidev_symbol[mcupm_no] == NULL) {
			PR_BOOTMSG("get_mcupm_ipidev_symbol is NULL,get symbol fail\n");
			return;
		}
	}

	if (!mcupm_ipidev_symbol[mcupm_no]->ipi_inited) {
		PR_BOOTMSG("mcupm_ipidev_symbol ipi_inited is 0\n");
		return;
	}

	// Tinysys send ipi to APSYS
	if (mtk_ipi_register_symbol) {
		ret = mtk_ipi_register_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIR_C_MET, _met_ipi_cb,
				&mcupm_ipi_cb_no[mcupm_no], (void *) recv_buf[mcupm_no]);
		if (ret) {
			PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", CH_IPIR_C_MET, ret);
			return;
		} else {
			PR_BOOTMSG("mtk_ipi_register CH_IPIR_C_MET success \n");
		}
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_register is not linked!\n", __FILE__, __LINE__);
		return;
	}

	// APSYS send ipi to Tinysys
	if (mtk_ipi_register_symbol) {
		ret = mtk_ipi_register_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIS_C_MET, NULL,
				&mcupm_ipi_cb_no[mcupm_no], (void *) &ackdata[mcupm_no]);
		if (ret) {
			PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", CH_IPIS_C_MET, ret);
			return;
		} else {
			PR_BOOTMSG("mtk_ipi_register CH_IPIS_C_MET success \n");
		}
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_register is not linked!\n", __FILE__, __LINE__);
		return;
	}

	if (mcupm_no == 0) {
		ret = snprintf(_name_buf, sizeof(_name_buf), "mcupmmcupm_recv");
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			PR_BOOTMSG("Error in snprintf for mcupmmcupm_recv\n");
			return;
		}
	} else {
	    ret = snprintf(_name_buf, sizeof(_name_buf), "mcupmmcupm_recv_slv_%d", mcupm_no - 1);
		if (ret < 0 || ret >= sizeof(_name_buf)) {
			PR_BOOTMSG("Error in snprintf for mcupmmcupm_recv_slv_%d\n", mcupm_no - 1);
			return;
		}
	}
	if (mcupm_ipi_thread_started[mcupm_no] != 1) {
		mcupm_recv_thread_comp[mcupm_no] = 0;
		_mcupm_recv_task[mcupm_no] = kthread_run(_mcupm_recv_thread,
						&mcupm_kthread_no[mcupm_no], _name_buf);
		if (IS_ERR(_mcupm_recv_task)) {
			PR_BOOTMSG("Can not create %s\n", _name_buf);

		} else {
			mcupm_ipi_thread_started[mcupm_no] = 1;
		}
	}
}


void stop_mcupm_ipi_recv_thread(int mcupm_no)
{
	int ret;
	if (_mcupm_recv_task[mcupm_no]) {
		mcupm_recv_thread_comp[mcupm_no] = 1;

		if (mcupm_ipidev_symbol[mcupm_no]) {
			if (mtk_ipi_unregister_symbol) {
				// Tinysys send ipi to APSYS
				ret = mtk_ipi_unregister_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIR_C_MET);
				if (ret)
					PR_BOOTMSG("mtk_ipi_unregister:%d failed:%d\n", CH_IPIR_C_MET, ret);
				// APSYS send ipi to Tinysys
				ret = mtk_ipi_unregister_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIS_C_MET);
				if (ret)
					PR_BOOTMSG("mtk_ipi_unregister:%d failed:%d\n", CH_IPIS_C_MET, ret);
			} else {
				PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_unregister is not linked!\n", __FILE__, __LINE__);
			}
		}

		kthread_stop(_mcupm_recv_task[mcupm_no]);
		_mcupm_recv_task[mcupm_no] = NULL;
		mcupm_ipi_thread_started[mcupm_no] = 0;
	}
}


void mcupm_start(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;
	int mcupm_no = 0;

	platform_name = met_get_platform();
	if (platform_name) {
		char buf[5];
		memset(buf, 0x0, 5);
		memcpy(buf, &platform_name[2], 4);
		ret = kstrtouint(buf, 10, &platform_id);
	}

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		/* clear DRAM buffer */
		if (mcupm_log_virt_addr[mcupm_no] != NULL) {
			memset_io((void *)mcupm_log_virt_addr[mcupm_no], 0, mcupm_buffer_size[mcupm_no]);
		} else {
			return;
		}

		/* send DRAM physical address */
		ipi_buf[0] = MET_MAIN_ID | MET_BUFFER_INFO;
		ipi_buf[1] = (ret == 0) ? platform_id : 0;
		ipi_buf[2] = (unsigned int)(mcupm_log_phy_addr[mcupm_no] & 0xFFFFFFFF); /* addr low */
		ipi_buf[3] = (unsigned int)(mcupm_log_phy_addr[mcupm_no] >> 32); /* addr high*/

		ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 4, &rdata, 1);

		/* start ondiemet now */
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
		ipi_buf[1] = ondiemet_module[ONDIEMET_MCUPM] ;
		ipi_buf[2] = MCUPM_LOG_FILE;
		ipi_buf[3] = MCUPM_RUN_NORMAL;
		ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 4, &rdata, 1);
	}
}


void mcupm_stop(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int chip_id = 0;
	int mcupm_no = 0;
	wait_for_file_write=0;

	chip_id = met_get_chip_id();
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_STOP;
	ipi_buf[1] = chip_id;
	ipi_buf[2] = 0;
	ipi_buf[3] = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		if (mcupm_buf_available[mcupm_no] == 1) {
			ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 4, &rdata, 1);
		}
	}
}


void mcupm_extract(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int count;
	int mcupm_no = 0;

	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_EXTRACT;
	ipi_buf[1] = 0;
	ipi_buf[2] = 0;
	ipi_buf[3] = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		count = 20;
		if (mcupm_buf_available[mcupm_no] == 1) {
			while ((mcupm_buffer_dumping[mcupm_no] == 1) && (count != 0)) {
				msleep(50);
				count--;
			}
			ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 4, &rdata, 1);
		}
	}
}


void mcupm_flush(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int mcupm_no = 0;

	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_FLUSH;
	ipi_buf[1] = 0;
	ipi_buf[2] = 0;
	ipi_buf[3] = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		if (mcupm_buf_available[mcupm_no] == 1) {
			ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 4, &rdata, 1);
		}
	}
}

int met_ipi_to_all_mcupm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret;
	unsigned int rdata;
	int mcupm_no = 0;

	for (mcupm_no = 0; mcupm_no < mcupm_count; mcupm_no++) {
		ret = met_ipi_to_mcupm_command(mcupm_no, (void *)buffer, 4, &rdata, 1);
		if (ret)
			return ret;
	}
	return ret;
}


int met_ipi_to_mcupm_command(
	unsigned int mcupm_no,
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol[mcupm_no] == NULL) {
		return -1;
	}

	if (mtk_ipi_send_compl_symbol) {
		ret = mtk_ipi_send_compl_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIS_C_MET,
			IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_send_compl is not linked!\n", __FILE__, __LINE__);
		return -1;
	}
	*retbuf = ackdata[mcupm_no];
	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_mcupm_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_mcupm_command);


int met_ipi_to_mcupm_command_async(
	unsigned int mcupm_no,
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol[mcupm_no] == NULL) {
		return -1;
	}
	if (mtk_ipi_send_symbol) {
		ret = mtk_ipi_send_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIS_C_MET,
			IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_send is not linked!\n", __FILE__, __LINE__);
		return -1;
	}
	*retbuf = ackdata[mcupm_no];

	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_mcupm_command_async error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_mcupm_command_async);


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
static void _log_done_cb(const void *p)
{
	unsigned int ret;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int mcupm_no = (unsigned int)(uintptr_t)p;

	if (mcupm_no != 0xDEADBEEF) {
		ipi_buf[0] = MET_MAIN_ID | MET_RESP_AP2MD;
		ipi_buf[1] = MET_DUMP_BUFFER;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_mcupm_command(mcupm_no, (void *)ipi_buf, 0, &rdata, 1);
	}
}


static int _met_ipi_cb(unsigned int ipi_id,
	void *prdata,
	void *data,
	unsigned int len)
{
	int mcupm_no = *(int *)prdata;

	/* prepare a copy of recv_buffer for deferred heavyweight cmd handling */
	memcpy(recv_buf_copy[mcupm_no], recv_buf[mcupm_no], 4 * sizeof(unsigned int));

	/* lightweight cmd handling (support reply_data) */
	reply_data[mcupm_no] = 0;
	switch (recv_buf[mcupm_no][0] & MET_SUB_ID_MASK) {
	case MET_RESP_MD2AP:
		mcupm_buffer_dumping[mcupm_no] = 0;
		break;
	}

	return 0;
}

static int _mcupm_recv_thread(void *data)
{
	int ret = 0;
	unsigned int ridx, widx, wlen;
	int mcupm_no = *(int *)data;

	if (!mtk_ipi_recv_reply_symbol) {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_recv_reply is not linked!\n", __FILE__, __LINE__);
		return -1;
	}

	do {
		ret = mtk_ipi_recv_reply_symbol(mcupm_ipidev_symbol[mcupm_no], CH_IPIR_C_MET,
				(void *)&reply_data[mcupm_no], 1);
		if (ret) {
			// skip cmd handling if receive fail
			continue;
		}

		if (mcupm_recv_thread_comp[mcupm_no] == 1) {
			while (!kthread_should_stop()) {
				;
			}
			return 0;
		}

		/* heavyweight cmd handling (not support reply_data) */
		switch (recv_buf_copy[mcupm_no][0] & MET_SUB_ID_MASK) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			mcupm_buffer_dumping[mcupm_no] = 1;
			ridx = recv_buf_copy[mcupm_no][1];
			widx = recv_buf_copy[mcupm_no][2];
			log_size[mcupm_no] = recv_buf_copy[mcupm_no][3];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size[mcupm_no] - ridx;
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)(uintptr_t)0xDEADBEEF);
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]),
						widx * 4, _log_done_cb, (void *)(uintptr_t)mcupm_no);
			} else {
				wlen = widx - ridx;
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)(uintptr_t)mcupm_no);
			}
			break;

		case MET_CLOSE_FILE:	/* no argument */
			ridx = recv_buf_copy[mcupm_no][1];
			widx = recv_buf_copy[mcupm_no][2];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size[mcupm_no] - ridx;
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)(uintptr_t)0xDEADBEEF);
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]),
						widx * 4, _log_done_cb, (void *)(uintptr_t)0xDEADBEEF);
			} else {
				wlen = widx - ridx;
				mcupm_log_req_enq(mcupm_no, (char *)(mcupm_log_virt_addr[mcupm_no]) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)(uintptr_t)0xDEADBEEF);
			}
			wait_for_file_write |= 1 << mcupm_no;
			if(wait_for_file_write == ( (1<<mcupm_count) -1) ){
				ret = mcupm_log_stop();
				if (ret)
					PR_BOOTMSG("[MET] mcupm_log_stop ret=%d\n", ret);
			}
			/* continuous mode handling */
			if (mcupm_run_mode == MCUPM_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)mcupm_log_virt_addr[mcupm_no], 0,
					  mcupm_buffer_size[mcupm_no]);
				/* re-start ondiemet again */
				mcupm_start();
			}
			break;

		default:
			break;
		}
	} while (!kthread_should_stop());

	return 0;
}

