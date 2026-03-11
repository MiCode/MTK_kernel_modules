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

#include "mcupm_met_log.h"
#include "mcupm_met_ipi_handle.h"
#include "tinysys_mcupm.h" /* for mcupm_ipidev_symbol */
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
static int _mcupm_recv_thread(void *data);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
int mcupm_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static struct mtk_ipi_device *mcupm_ipidev_symbol;
static unsigned int recv_buf[4], recv_buf_copy[4];
static unsigned int ackdata;
static unsigned int reply_data;
static unsigned int log_size;
static struct task_struct *_mcupm_recv_task;
static int mcupm_ipi_thread_started;
static int mcupm_buffer_dumping;
static int mcupm_recv_thread_comp;
static int mcupm_run_mode = MCUPM_RUN_NORMAL;


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
void start_mcupm_ipi_recv_thread()
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
	ret = mtk_ipi_register(mcupm_ipidev_symbol, CH_IPIR_C_MET, _met_ipi_cb,
				NULL, (void *) &recv_buf);
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", CH_IPIR_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register CH_IPIR_C_MET success \n");
	}

	// APSYS send ipi to Tinysys
	ret = mtk_ipi_register(mcupm_ipidev_symbol, CH_IPIS_C_MET, NULL,
				NULL, (void *) &ackdata);
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", CH_IPIS_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register CH_IPIS_C_MET success \n");
	}

	if (mcupm_ipi_thread_started != 1) {
		mcupm_recv_thread_comp = 0;
		_mcupm_recv_task = kthread_run(_mcupm_recv_thread,
						NULL, "mcupmmcupm_recv");
		if (IS_ERR(_mcupm_recv_task)) {
			PR_BOOTMSG("Can not create mcupmmcupm_recv\n");

		} else {
			mcupm_ipi_thread_started = 1;
		}
	}
}


void stop_mcupm_ipi_recv_thread()
{
	if (_mcupm_recv_task) {
		mcupm_recv_thread_comp = 1;

		if (mcupm_ipidev_symbol) {
			// Tinysys send ipi to APSYS
			mtk_ipi_unregister(mcupm_ipidev_symbol, CH_IPIR_C_MET);
			// APSYS send ipi to Tinysys
			mtk_ipi_unregister(mcupm_ipidev_symbol, CH_IPIS_C_MET);
		}

		kthread_stop(_mcupm_recv_task);
		_mcupm_recv_task = NULL;
		mcupm_ipi_thread_started = 0;
	}
}


void mcupm_start(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;

	/* clear DRAM buffer */
	if (mcupm_log_virt_addr != NULL) {
		memset_io((void *)mcupm_log_virt_addr, 0, mcupm_buffer_size);
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
	ipi_buf[1] = (ret == 0) ? platform_id : 0;
	ipi_buf[2] = (unsigned int)mcupm_log_phy_addr; /* address */
	ipi_buf[3] = 0;

	ret = met_ipi_to_mcupm_command((void *)ipi_buf, 4, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_MCUPM] ;
	ipi_buf[2] = MCUPM_LOG_FILE;
	ipi_buf[3] = MCUPM_RUN_NORMAL;
	ret = met_ipi_to_mcupm_command((void *)ipi_buf, 4, &rdata, 1);
}


void mcupm_stop(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int chip_id = 0;

	chip_id = met_get_chip_id();
	if (mcupm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_STOP;
		ipi_buf[1] = chip_id;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_mcupm_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


void mcupm_extract(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int count;

	count = 20;
	if (mcupm_buf_available == 1) {
		while ((mcupm_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_mcupm_command((void *)ipi_buf, 4, &rdata, 1);

	}
}


void mcupm_flush(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	if (mcupm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_mcupm_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


int met_ipi_to_mcupm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol == NULL) {
		return -1;
	}

	ret = mtk_ipi_send_compl(mcupm_ipidev_symbol, CH_IPIS_C_MET,
		IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	*retbuf = ackdata;
	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_mcupm_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_mcupm_command);


int met_ipi_to_mcupm_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (mcupm_ipidev_symbol == NULL) {
		return -1;
	}
	ret = mtk_ipi_send(mcupm_ipidev_symbol, CH_IPIS_C_MET,
		IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	*retbuf = ackdata;

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
	unsigned int opt = (p != NULL);

	if (opt == 0) {
		ipi_buf[0] = MET_MAIN_ID | MET_RESP_AP2MD;
		ipi_buf[1] = MET_DUMP_BUFFER;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_mcupm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}


static int _met_ipi_cb(unsigned int ipi_id,
	void *prdata,
	void *data,
	unsigned int len)
{
	/* prepare a copy of recv_buffer for deferred heavyweight cmd handling */
	memcpy(recv_buf_copy, recv_buf, sizeof(recv_buf_copy));

	/* lightweight cmd handling (support reply_data) */
	reply_data = 0;
	switch (recv_buf[0] & MET_SUB_ID_MASK) {
	case MET_RESP_MD2AP:
		mcupm_buffer_dumping = 0;
		break;
	}

	return 0;
}

static int _mcupm_recv_thread(void *data)
{
	int ret = 0;
	unsigned int ridx, widx, wlen;

	do {
		ret = mtk_ipi_recv_reply(mcupm_ipidev_symbol, CH_IPIR_C_MET,
				(void *)&reply_data, 1);
		if (ret) {
			// skip cmd handling if receive fail
			continue;
		}

		if (mcupm_recv_thread_comp == 1) {
			while (!kthread_should_stop()) {
				;
			}
			return 0;
		}

		/* heavyweight cmd handling (not support reply_data) */
		switch (recv_buf_copy[0] & MET_SUB_ID_MASK) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			mcupm_buffer_dumping = 1;
			ridx = recv_buf_copy[1];
			widx = recv_buf_copy[2];
			log_size = recv_buf_copy[3];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr),
						widx * 4, _log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)0);
			}
			break;

		case MET_CLOSE_FILE:	/* no argument */
			ridx = recv_buf_copy[1];
			widx = recv_buf_copy[2];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr),
						widx * 4, _log_done_cb, (void *)1);
			} else {
				wlen = widx - ridx;
				mcupm_log_req_enq((char *)(mcupm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
			}
			ret = mcupm_log_stop();

			/* continuous mode handling */
			if (mcupm_run_mode == MCUPM_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)mcupm_log_virt_addr, 0,
					  mcupm_buffer_size);
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

