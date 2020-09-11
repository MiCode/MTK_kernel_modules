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

#include "mt-plat/mtk_tinysys_ipi.h"  /* for mtk_ipi_device */
#include "mcupm_ipi_id.h"  /* for mcupm_ipidev */

#include "cpu_eb_met_log.h"
#include "cpu_eb_met_ipi_handle.h"
#include "tinysys_cpu_eb.h" /* for mcupm_ipidev_symbol */
#include "tinysys_mgr.h" /* for ondiemet_module */
#include "interface.h"
#include "core_plf_init.h"


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
static int _cpu_eb_recv_thread(void *data);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
int cpu_eb_buf_available;
EXPORT_SYMBOL(cpu_eb_buf_available);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
struct mtk_ipi_device *mcupm_ipidev_symbol;
static unsigned int recv_buf[4];
static unsigned int rdata;
static unsigned int ackdata;
static unsigned int ridx, widx, wlen;
static unsigned int log_size;
static struct task_struct *_cpu_eb_recv_task;
static int cpu_eb_ipi_thread_started;
static int cpu_eb_buffer_dumping;
static int cpu_eb_recv_thread_comp;
static int cpu_eb_run_mode = CPU_EB_RUN_NORMAL;


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
void start_cpu_eb_ipi_recv_thread()
{
	int ret = 0;

	if (mcupm_ipidev_symbol == NULL) {
		mcupm_ipidev_symbol = (struct mtk_ipi_device *)symbol_get(mcupm_ipidev);
	}

	if (mcupm_ipidev_symbol == NULL) {
		PR_BOOTMSG("mcupm_ipidev_symbol is NULL\n", __FUNCTION__);
		return;
	}

	// Tinysys send ipi to APSYS
	ret = mtk_ipi_register(mcupm_ipidev_symbol, IPIR_C_MET, _met_ipi_cb,
				NULL, (void *) &recv_buf);
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", IPIR_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register IPIR_C_MET success \n");
	}

	// APSYS send ipi to Tinysys
	ret = mtk_ipi_register(mcupm_ipidev_symbol, IPIS_C_MET, NULL,
				NULL, (void *) &ackdata);
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", IPIS_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register IPIS_C_MET success \n");
	}

	if (cpu_eb_ipi_thread_started != 1) {
		cpu_eb_recv_thread_comp = 0;
		_cpu_eb_recv_task = kthread_run(_cpu_eb_recv_thread,
						NULL, "cpu_ebcpu_eb_recv");
		if (IS_ERR(_cpu_eb_recv_task)) {
			PR_BOOTMSG("Can not create cpu_ebcpu_eb_recv\n");

		} else {
			cpu_eb_ipi_thread_started = 1;
		}
	}
}


void stop_cpu_eb_ipi_recv_thread()
{
	if (_cpu_eb_recv_task) {
		cpu_eb_recv_thread_comp = 1;

		kthread_stop(_cpu_eb_recv_task);
		_cpu_eb_recv_task = NULL;
		cpu_eb_ipi_thread_started = 0;

		if (mcupm_ipidev_symbol) {
			// Tinysys send ipi to APSYS
			mtk_ipi_unregister(mcupm_ipidev_symbol, IPIR_C_MET);
			// APSYS send ipi to Tinysys
			mtk_ipi_unregister(mcupm_ipidev_symbol, IPIS_C_MET);
		}
	}
}


void cpu_eb_start(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;

	/* clear DRAM buffer */
	if (cpu_eb_log_virt_addr != NULL) {
		memset_io((void *)cpu_eb_log_virt_addr, 0, cpu_eb_buffer_size);
	} else {
		return;
	}

	platform_name = met_get_platform();
	if (platform_name) {
		char buf[5];

		memset(buf, 0x0, 5);
		memcpy(buf, &platform_name[2], 4);
		ret = kstrtouint(buf, 10, &platform_id);
	}

	/* send DRAM physical address */
	ipi_buf[0] = MET_MAIN_ID | MET_BUFFER_INFO;
	ipi_buf[1] = (unsigned int)cpu_eb_log_phy_addr; /* address */
	if (ret == 0) {
		ipi_buf[2] = platform_id;
	} else {
		ipi_buf[2] = 0;
	}

	ipi_buf[3] = 0;
	ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 4, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_CPU_EB] ;
	ipi_buf[2] = CPU_EB_LOG_FILE;
	ipi_buf[3] = CPU_EB_RUN_NORMAL;
	ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 4, &rdata, 1);
}


void cpu_eb_stop(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int chip_id = 0;

	chip_id = met_get_chip_id();
	if (cpu_eb_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_STOP;
		ipi_buf[1] = chip_id;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


void cpu_eb_extract(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int count;

	count = 20;
	if (cpu_eb_buf_available == 1) {
		while ((cpu_eb_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 4, &rdata, 1);

	}
}


void cpu_eb_flush(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	if (cpu_eb_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


int met_ipi_to_cpu_eb_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol == NULL) {
		return -1;
	}

	ret = mtk_ipi_send_compl(mcupm_ipidev_symbol, IPIS_C_MET,
		IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	*retbuf = ackdata;
	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_cpu_eb_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_cpu_eb_command);


int met_ipi_to_cpu_eb_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol == NULL) {
		return -1;
	}
	ret = mtk_ipi_send(mcupm_ipidev_symbol, IPIS_C_MET,
		IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	*retbuf = ackdata;

	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_cpu_eb_command_async error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_cpu_eb_command_async);


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
static void _log_done_cb(const void *p)
{
	unsigned int ret;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int opt = (p != NULL);

	if (opt == 0) {
		cpu_eb_buffer_dumping = 0;

		ipi_buf[0] = MET_MAIN_ID | MET_RESP_AP2MD;
		ipi_buf[1] = MET_DUMP_BUFFER;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_cpu_eb_command((void *)ipi_buf, 0, &rdata, 1);
	}
}


static int _met_ipi_cb(unsigned int ipi_id, void *prdata, void *data, unsigned int len)
{
	unsigned int cmd;

	if (cpu_eb_recv_thread_comp == 1) {
		PR_BOOTMSG("eb %s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}

	cmd = recv_buf[0] & MET_SUB_ID_MASK;
	switch (cmd) {
	case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
		cpu_eb_buffer_dumping = 1;
		ridx = recv_buf[1];
		widx = recv_buf[2];
		log_size = recv_buf[3];
		break;

	case MET_CLOSE_FILE:	/* no argument */
		/* do close file */
		ridx = recv_buf[1];
		widx = recv_buf[2];
		break;

	default:
		break;
	}
	return 0;
}


static int _cpu_eb_recv_thread(void *data)
{
	int ret = 0;
	unsigned int cmd = 0;

	do {
		ret = mtk_ipi_recv_reply(mcupm_ipidev_symbol, IPIR_C_MET, (void *)&rdata, 1);
		if (ret) {
			PR_BOOTMSG("ipi_register:%d failed:%d\n", IPIR_C_MET, ret);
		}
		if (cpu_eb_recv_thread_comp == 1) {
			while (!kthread_should_stop()) {
				;
			}
			return 0;
		}

		cmd = recv_buf[0] & MET_SUB_ID_MASK;
		switch (cmd) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr),
						widx * 4, _log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)0);
			}
			break;

		case MET_CLOSE_FILE:	/* no argument */
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr),
						widx * 4, _log_done_cb, (void *)1);
			} else {
				wlen = widx - ridx;
				cpu_eb_log_req_enq((char *)(cpu_eb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
			}
			ret = cpu_eb_log_stop();
			if (cpu_eb_run_mode == CPU_EB_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)cpu_eb_log_virt_addr, 0,
					  cpu_eb_buffer_size);
				/* re-start ondiemet again */
				cpu_eb_start();
			}
			break;

		default:
			break;
		}
	} while (!kthread_should_stop());

	return 0;
}

