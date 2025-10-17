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
#include <linux/platform_device.h>  /* for struct platform_device used in gpueb_reserved_mem.h & gpueb_ipi.h */
#include "gpueb_ipi.h"  /* for gpueb_ipidev */

#include "gpueb_met_log.h"
#include "gpueb_met_ipi_handle.h"
#include "tinysys_gpueb.h" /* for gpueb_ipidev_symbol */
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
static int _gpueb_recv_thread(void *data);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/
int gpueb_buf_available;


/*****************************************************************************
 * internal variable declaration
 *****************************************************************************/
static int id_ch_ipir_c_met = -1, id_ch_ipis_c_met = -1;
static struct mtk_ipi_device *gpueb_ipidev_symbol;
static unsigned int recv_buf[4], recv_buf_copy[4];
static unsigned int ackdata;
static unsigned int reply_data;
static unsigned int log_size;
static struct task_struct *_gpueb_recv_task;
static int gpueb_ipi_thread_started;
static int gpueb_buffer_dumping;
static int gpueb_recv_thread_comp;
static int gpueb_run_mode = GPUEB_RUN_NORMAL;


/*****************************************************************************
 * internal function ipmlement
 *****************************************************************************/
void start_gpueb_ipi_recv_thread(void)
{
	int ret = 0;

	if (gpueb_ipidev_symbol == NULL) {
		gpueb_ipidev_symbol = get_gpueb_ipidev_symbol();
	}

	if (gpueb_ipidev_symbol == NULL) {
		PR_BOOTMSG("gpueb_ipidev_symbol is NULL,get symbol fail\n");
		return;
	}

	if (!gpueb_ipidev_symbol->ipi_inited) {
		PR_BOOTMSG("gpueb_ipidev_symbol ipi_inited is 0\n");
		return;
	}

	// Tinysys send ipi to APSYS
	if (id_ch_ipir_c_met == -1) {
		if (gpueb_get_recv_PIN_ID_by_name_symbol) {
			id_ch_ipir_c_met = gpueb_get_recv_PIN_ID_by_name_symbol("CH_IPIR_C_MET");
		}
		if (id_ch_ipir_c_met == -1) {   // pin ID is not found
			PR_BOOTMSG("get GPUEB IPI CH_IPIR_C_MET's pin ID fail\n");
			return;
		} else {   // pin ID is found
			PR_BOOTMSG("GPUEB IPI CH_IPIR_C_MET's pin ID: %d\n", id_ch_ipir_c_met);
		}
	}

	if (mtk_ipi_register_symbol) {
		ret = mtk_ipi_register_symbol(gpueb_ipidev_symbol, id_ch_ipir_c_met, _met_ipi_cb,
				NULL, (void *) &recv_buf);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_register is not linked!\n", __FILE__, __LINE__);
		return;
	}
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", id_ch_ipir_c_met, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register CH_IPIR_C_MET success \n");
	}

	// APSYS send ipi to Tinysys
	if (id_ch_ipis_c_met == -1) {
		if (gpueb_get_send_PIN_ID_by_name_symbol) {
			id_ch_ipis_c_met = gpueb_get_send_PIN_ID_by_name_symbol("CH_IPIS_C_MET");
		}
		if (id_ch_ipis_c_met == -1) {   // pin ID is not found
			PR_BOOTMSG("get GPUEB IPI CH_IPIS_C_MET's pin ID fail\n");
			return;
		} else {   // pin ID is found
			PR_BOOTMSG("GPUEB IPI CH_IPIS_C_MET's pin ID: %d\n", id_ch_ipis_c_met);
		}
	}

	if (mtk_ipi_register_symbol) {
		ret = mtk_ipi_register_symbol(gpueb_ipidev_symbol, id_ch_ipis_c_met, NULL,
				NULL, (void *) &ackdata);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_register is not linked!\n", __FILE__, __LINE__);
		return;
	}
	if (ret) {
		PR_BOOTMSG("mtk_ipi_register:%d failed:%d\n", id_ch_ipis_c_met, ret);
	} else {
		PR_BOOTMSG("mtk_ipi_register CH_IPIS_C_MET success \n");
	}

	if (gpueb_ipi_thread_started != 1) {
		gpueb_recv_thread_comp = 0;
		_gpueb_recv_task = kthread_run(_gpueb_recv_thread,
						NULL, "gpuebgpueb_recv");
		if (IS_ERR(_gpueb_recv_task)) {
			PR_BOOTMSG("Can not create gpuebgpueb_recv\n");

		} else {
			gpueb_ipi_thread_started = 1;
		}
	}
}


void stop_gpueb_ipi_recv_thread(void)
{
	int ret;

	if (_gpueb_recv_task) {
		gpueb_recv_thread_comp = 1;

		if (gpueb_ipidev_symbol) {
			
			if (mtk_ipi_unregister_symbol) {
				// Tinysys send ipi to APSYS
				ret = mtk_ipi_unregister_symbol(gpueb_ipidev_symbol, id_ch_ipir_c_met);
				if (ret)
					PR_BOOTMSG("mtk_ipi_unregister:%d failed:%d\n", id_ch_ipir_c_met, ret);
				// APSYS send ipi to Tinysys
				ret = mtk_ipi_unregister_symbol(gpueb_ipidev_symbol, id_ch_ipis_c_met);
				if (ret)
					PR_BOOTMSG("mtk_ipi_unregister:%d failed:%d\n", id_ch_ipis_c_met, ret);
			} else {
				PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_unregister is not linked!\n", __FILE__, __LINE__);
			}
		}

		kthread_stop(_gpueb_recv_task);
		_gpueb_recv_task = NULL;
		gpueb_ipi_thread_started = 0;
	}
}


void gpueb_start(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	const char* platform_name = NULL;
	unsigned int platform_id = 0;

	/* clear DRAM buffer */
	if (gpueb_log_virt_addr != NULL) {
		memset_io((void *)gpueb_log_virt_addr, 0, gpueb_buffer_size);
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
	ipi_buf[2] = (unsigned int)(gpueb_log_phy_addr & 0xFFFFFFFF); /* addr low */
	ipi_buf[3] = (unsigned int)(gpueb_log_phy_addr >> 32); /* addr high*/

	ret = met_ipi_to_gpueb_command((void *)ipi_buf, 4, &rdata, 1);

	/* start ondiemet now */
	ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_START;
	ipi_buf[1] = ondiemet_module[ONDIEMET_GPUEB] ;
	ipi_buf[2] = GPUEB_LOG_FILE;
	ipi_buf[3] = GPUEB_RUN_NORMAL;
	ret = met_ipi_to_gpueb_command((void *)ipi_buf, 4, &rdata, 1);
}


void gpueb_stop(void)
{
	int ret = 0;
	unsigned int rdata = 0;
	unsigned int ipi_buf[4];
	unsigned int chip_id = 0;

	chip_id = met_get_chip_id();
	if (gpueb_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_STOP;
		ipi_buf[1] = chip_id;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_gpueb_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


void gpueb_extract(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];
	int count;

	count = 20;
	if (gpueb_buf_available == 1) {
		while ((gpueb_buffer_dumping == 1) && (count != 0)) {
			msleep(50);
			count--;
		}
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_EXTRACT;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_gpueb_command((void *)ipi_buf, 4, &rdata, 1);

	}
}


void gpueb_flush(void)
{
	int ret;
	unsigned int rdata;
	unsigned int ipi_buf[4];

	if (gpueb_buf_available == 1) {
		ipi_buf[0] = MET_MAIN_ID | MET_OP | MET_OP_FLUSH;
		ipi_buf[1] = 0;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_gpueb_command((void *)ipi_buf, 4, &rdata, 1);
	}
}


int met_ipi_to_gpueb_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (gpueb_ipidev_symbol == NULL) {
		return -1;
	}

	if (mtk_ipi_send_compl_to_gpueb_symbol) {
		ret = mtk_ipi_send_compl_to_gpueb_symbol(id_ch_ipis_c_met,
			IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_send_compl_to_gpueb is not linked!\n", __FILE__, __LINE__);
		return -1;
	}
	*retbuf = ackdata;
	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_gpueb_command error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_gpueb_command);


int met_ipi_to_gpueb_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot)
{
	int ret = 0;

	if (gpueb_ipidev_symbol == NULL) {
		return -1;
	}
	if (mtk_ipi_send_symbol) {
		ret = mtk_ipi_send_symbol(gpueb_ipidev_symbol, id_ch_ipis_c_met,
			IPI_SEND_WAIT, (void*)buffer, slot, 2000);
	} else {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_send is not linked!\n", __FILE__, __LINE__);
		return -1;
	}
	*retbuf = ackdata;

	if (ret != 0) {
		PR_BOOTMSG("met_ipi_to_gpueb_command_async error(%d)\n", ret);
	}

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_gpueb_command_async);


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
		ret = met_ipi_to_gpueb_command((void *)ipi_buf, 0, &rdata, 1);
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
		gpueb_buffer_dumping = 0;
		break;
	}

	return 0;
}

static int _gpueb_recv_thread(void *data)
{
	int ret = 0;
	unsigned int ridx, widx, wlen;

	if (!mtk_ipi_recv_reply_symbol) {
		PR_BOOTMSG("[MET] [%s,%d] mtk_ipi_recv_reply is not linked!\n", __FILE__, __LINE__);
		return -1;
	}

	do {
		ret = mtk_ipi_recv_reply_symbol(gpueb_ipidev_symbol, id_ch_ipir_c_met,
				(void *)&reply_data, 1);
		if (ret) {
			// skip cmd handling if receive fail
			continue;
		}

		if (gpueb_recv_thread_comp == 1) {
			while (!kthread_should_stop()) {
				;
			}
			return 0;
		}

		/* heavyweight cmd handling (not support reply_data) */
		switch (recv_buf_copy[0] & MET_SUB_ID_MASK) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			gpueb_buffer_dumping = 1;
			ridx = recv_buf_copy[1];
			widx = recv_buf_copy[2];
			log_size = recv_buf_copy[3];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr),
						widx * 4, _log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)0);
			}
			break;

		case MET_CLOSE_FILE:	/* no argument */
			ridx = recv_buf_copy[1];
			widx = recv_buf_copy[2];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr),
						widx * 4, _log_done_cb, (void *)1);
			} else {
				wlen = widx - ridx;
				gpueb_log_req_enq((char *)(gpueb_log_virt_addr) + (ridx << 2),
						wlen * 4, _log_done_cb, (void *)1);
			}
			ret = gpueb_log_stop();
			if (ret)
				PR_BOOTMSG("[MET] gpueb_log_stop ret=%d\n", ret);

			/* continuous mode handling */
			if (gpueb_run_mode == GPUEB_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)gpueb_log_virt_addr, 0,
					  gpueb_buffer_size);
				/* re-start ondiemet again */
				gpueb_start();
			}
			break;

		default:
			break;
		}
	} while (!kthread_should_stop());

	return 0;
}

