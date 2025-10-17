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
#include "sspm_ipi_id.h"  /* for sspm_ipidev */
#include "sspm_met_log.h" /* for sspm_ipidev_symbol */
#include "sspm_met_ipi_handle.h"
#include "interface.h"
#include "core_plf_init.h"
#include "tinysys_sspm.h"
#include "tinysys_mgr.h" /* for ondiemet_module */


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
static int _sspm_recv_thread(void *data);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
int sspm_buf_available;
EXPORT_SYMBOL(sspm_buf_available);


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static unsigned int ridx, widx, wlen;
static unsigned int recv_buf[4];
static unsigned int ackdata;
static unsigned int rdata;
static unsigned int log_size;
static struct task_struct *_sspm_recv_task;
static int sspm_ipi_thread_started;
static int sspm_buffer_dumping;
static int sspm_recv_thread_comp;
static int sspm_run_mode = SSPM_RUN_NORMAL;



/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
void start_sspm_ipi_recv_thread()
{
	int ret = 0;

	if (sspm_ipidev_symbol == NULL) {
		sspm_ipidev_symbol = (struct mtk_ipi_device *)symbol_get(sspm_ipidev);
	}

	if (sspm_ipidev_symbol == NULL) {
		return;
	}

	// tinysys send ipi to APSYS
	ret = mtk_ipi_register(sspm_ipidev_symbol, IPIR_C_MET, _met_ipi_cb,
				NULL, (void *)&recv_buf);
	if (ret) {
		PR_BOOTMSG("[MET] ipi_register:%d failed:%d\n", IPIR_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register IPIR_C_MET success \n");
	}

	// APSYS send ipi to tinysys
	ret = mtk_ipi_register(sspm_ipidev_symbol, IPIS_C_MET, NULL,
				NULL, (void *)&ackdata);
	if (ret) {
		pr_debug("[MET] ipi_register:%d failed:%d\n", IPIS_C_MET, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register IPIS_C_MET success \n");
	}

	if (sspm_ipi_thread_started != 1) {
		sspm_recv_thread_comp = 0;
		_sspm_recv_task = kthread_run(_sspm_recv_thread,
						NULL, "sspmsspm_recv");
		if (IS_ERR(_sspm_recv_task)) {
			pr_debug("MET: Can not create sspmsspm_recv\n");
		} else {
			sspm_ipi_thread_started = 1;
		}
	}
}


void stop_sspm_ipi_recv_thread()
{
	if (_sspm_recv_task) {
		sspm_recv_thread_comp = 1;

		if (sspm_ipidev_symbol) {
			// tinysys send ipi to APSYS
			mtk_ipi_unregister(sspm_ipidev_symbol, IPIR_C_MET);
			// APSYS send ipi to tinysys
			mtk_ipi_unregister(sspm_ipidev_symbol, IPIS_C_MET);
		}

		kthread_stop(_sspm_recv_task);
		_sspm_recv_task = NULL;
		sspm_ipi_thread_started = 0;
	}
}


void sspm_start(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;

	/* clear DRAM buffer */
	if (sspm_log_virt_addr != NULL) {
		memset_io((void *)sspm_log_virt_addr, 0, sspm_buffer_size);
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
	ipi_buf[1] = (unsigned int)sspm_log_phy_addr; /* address */
	if (ret == 0)
		ipi_buf[2] = platform_id;
	else
		ipi_buf[2] = 0;

	ipi_buf[3] = 0;
	ret = met_ipi_to_sspm_command(ipi_buf, 0, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_SSPM] ;
	ipi_buf[2] = SSPM_LOG_FILE;
	ipi_buf[3] = SSPM_RUN_NORMAL;
	ret = met_ipi_to_sspm_command(ipi_buf, 0, &rdata, 1);
}


void sspm_stop(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int chip_id = 0;

	chip_id = met_get_chip_id();
	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_STOP;
		ipi_buf[1] = chip_id;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command(ipi_buf, 0, &rdata, 1);
	}
}


void sspm_extract(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int count;

	count = 20;
	if (sspm_buf_available == 1) {
		while ((sspm_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command(ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL) {
		ondiemet_module[ONDIEMET_SSPM]  = 0;
	}
}


void sspm_flush(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	if (sspm_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}

	if (sspm_run_mode == SSPM_RUN_NORMAL) {
		ondiemet_module[ONDIEMET_SSPM] = 0;
	}
}


int met_ipi_to_sspm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (sspm_ipidev_symbol == NULL) {
		PR_BOOTMSG("%s\n", __FUNCTION__);
		return -1;
	}

	ret = mtk_ipi_send_compl(sspm_ipidev_symbol, IPIS_C_MET, IPI_SEND_WAIT,
				(void*)buffer, slot, 2000);
	*retbuf = ackdata;

	if (ret != 0) {
		PR_BOOTMSG("%s 0x%X, 0x%X, 0x%X, 0x%X\n", __FUNCTION__,
			buffer[0], buffer[1], buffer[2], buffer[3]);
		pr_debug("met_ipi_to_sspm_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_sspm_command);


int met_ipi_to_sspm_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if(sspm_ipidev_symbol == NULL) {
		PR_BOOTMSG("%s\n", __FUNCTION__);
		return -1;
	}

	ret = mtk_ipi_send(sspm_ipidev_symbol, IPIS_C_MET, IPI_SEND_WAIT,
			(void*)buffer, slot, 2000);
	*retbuf = ackdata;

	if (ret != 0) {
		PR_BOOTMSG("%s 0x%X, 0x%X, 0x%X, 0x%X\n", __FUNCTION__,
			buffer[0], buffer[1], buffer[2], buffer[3]);
		pr_debug("met_ipi_to_sspm_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_sspm_command_async);


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
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}


static int _met_ipi_cb(unsigned int ipi_id, void *prdata, void *data, unsigned int len)
{
	unsigned int *cmd_buf = (unsigned int *)data;
	unsigned int cmd;
	int ret;

	if (sspm_recv_thread_comp == 1) {
		PR_BOOTMSG("%s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}

	cmd = cmd_buf[0] & MET_SUB_ID_MASK;
	switch (cmd) {
	case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
		sspm_buffer_dumping = 1;
		ridx = cmd_buf[1];
		widx = cmd_buf[2];
		log_size = cmd_buf[3];
		break;

	case MET_CLOSE_FILE:	/* no argument */
		/* do close file */
		ridx = cmd_buf[1];
		widx = cmd_buf[2];
		if (widx < ridx) {	/* wrapping occurs */
			wlen = log_size - ridx;
			sspm_log_req_enq((char *)(sspm_log_virt_addr) + (ridx << 2),
					wlen * 4, _log_done_cb, (void *)1);
			sspm_log_req_enq((char *)(sspm_log_virt_addr),
					widx * 4, _log_done_cb, (void *)1);
		} else {
			wlen = widx - ridx;
			sspm_log_req_enq((char *)(sspm_log_virt_addr) + (ridx << 2),
					wlen * 4, _log_done_cb, (void *)1);
		}
		ret = sspm_log_stop();
		break;

	case MET_RESP_MD2AP:
		break;

	default:
		break;
	}
	return 0;
}


static int _sspm_recv_thread(void *data)
{
	int ret = 0;
	unsigned int cmd = 0;

	do {
		ret = mtk_ipi_recv_reply(sspm_ipidev_symbol, IPIR_C_MET, (void *)&rdata, 1);
		if (ret) {
			pr_debug("[MET] ipi_register:%d failed:%d\n", IPIR_C_MET, ret);
		}

		if (sspm_recv_thread_comp == 1) {
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
				sspm_log_req_enq((char *)(sspm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				sspm_log_req_enq((char *)(sspm_log_virt_addr),
						widx * 4, _log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				sspm_log_req_enq((char *)(sspm_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)0);
			}
			break;

		case MET_CLOSE_FILE:	/* no argument */
			if (sspm_run_mode == SSPM_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)sspm_log_virt_addr, 0,
					  sspm_buffer_size);
				/* re-start ondiemet again */
				sspm_start();
			}
			break;

		case MET_RESP_MD2AP:
			sspm_buffer_dumping = 0;
			break;

		default:
			break;
		}
	} while (!kthread_should_stop());

	return 0;
}

