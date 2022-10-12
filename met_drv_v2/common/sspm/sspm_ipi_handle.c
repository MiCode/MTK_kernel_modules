// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

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
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "ondiemet_sspm.h"
#include "ondiemet_log.h"
#include "interface.h"
#include "met_kernel_symbol.h"


#ifndef SSPM_VERSION_V2
static struct ipi_action ondiemet_sspm_isr;
#else
#include "core_plf_init.h"
uint32_t rdata = 0;
uint32_t ridx, widx, wlen;
uint32_t ackdata = 0;
#endif
static uint32_t log_size;
static uint32_t recv_buf[4];
static struct task_struct *ondiemet_sspm_recv_task;
static int sspm_ipi_thread_started;
int sspm_buffer_dumping;
int sspm_recv_thread_comp;

void log_done_cb(const void *p)
{
	uint32_t ret;
	uint32_t rdata = 0;
	uint32_t ipi_buf[4];
	uint32_t opt = (p != NULL);

	if (opt == 0) {
		ipi_buf[0] = MET_MAIN_ID | MET_RESP_AP2MD;
		ipi_buf[1] = MET_DUMP_BUFFER;
		ipi_buf[2] = 0;
		ipi_buf[3] = 0;
		ret = met_ipi_to_sspm_command((void *)ipi_buf, 0, &rdata, 1);
	}
}
#ifndef SSPM_VERSION_V2
int ondiemet_sspm_recv_thread(void *data)
{
	uint32_t rdata = 0, cmd, ret;
	uint32_t ridx, widx, wlen;

	ondiemet_sspm_isr.data = (void *)recv_buf;
	ret = sspm_ipi_recv_registration(IPI_ID_TST1, &ondiemet_sspm_isr);
	do {
		sspm_ipi_recv_wait(IPI_ID_TST1);
		if (sspm_recv_thread_comp == 1) {
			while (!kthread_should_stop())
				;
			return 0;
		}
		cmd = recv_buf[0] & MET_SUB_ID_MASK;
		switch (cmd) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			sspm_buffer_dumping = 1;
			ridx = recv_buf[1];
			widx = recv_buf[2];
			log_size = recv_buf[3];
			sspm_ipi_send_ack(IPI_ID_TST1, &rdata);
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)1);
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr),
						     widx * 4, log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)0);
			}
			break;
		case MET_CLOSE_FILE:	/* no argument */
			/* do close file */
			ridx = recv_buf[1];
			widx = recv_buf[2];
			met_sspm_log_discard = recv_buf[3];
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)1);
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr),
						     widx * 4, log_done_cb, (void *)1);
			} else {
				wlen = widx - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)1);
			}
			ret = ondiemet_log_manager_stop();
			/* pr_debug("MET_CLOSE_FILE: ret=%d log_discard=%d\n", ret, met_sspm_log_discard); */
			sspm_ipi_send_ack(IPI_ID_TST1, &rdata);
			if (sspm_run_mode == SSPM_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)ondiemet_sspm_log_virt_addr, 0,
					  ondiemet_sspm_log_size);
				/* re-start ondiemet again */
				sspm_start();
			}
			break;
		case MET_RESP_MD2AP:
			sspm_ipi_send_ack(IPI_ID_TST1, &rdata);
			sspm_buffer_dumping = 0;
			break;
		default:
			sspm_ipi_send_ack(IPI_ID_TST1, &rdata);
			break;
		}
	} while (!kthread_should_stop());
	return 0;
}

#else /* SSPM_VERSION_V2 */

int met_ipi_cb(unsigned int ipi_id, void *prdata, void *data, unsigned int len)
{
	uint32_t *cmd_buf = (uint32_t *)data;
	uint32_t cmd;
	int ret;

	if (sspm_recv_thread_comp == 1)
		return 0;

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
		met_sspm_log_discard = cmd_buf[3];
		if (widx < ridx) {	/* wrapping occurs */
			wlen = log_size - ridx;
			ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
			wlen * 4, log_done_cb, (void *)1);
			ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr),
						 widx * 4, log_done_cb, (void *)1);
		} else {
			wlen = widx - ridx;
			ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
			wlen * 4, log_done_cb, (void *)1);
		}
		ret = ondiemet_log_manager_stop();
		/* pr_debug("MET_CLOSE_FILE: ret=%d log_discard=%d\n", ret, met_sspm_log_discard); */
		break;
	case MET_RESP_MD2AP:
		break;
	default:
		break;
	}
	return 0;
}

int ondiemet_sspm_recv_thread(void *data)
{
	int ret;
	uint32_t cmd;

	ret = mtk_ipi_register(sspm_ipidev_symbol, IPIR_C_MET, met_ipi_cb, NULL, (void *)recv_buf);
	if (ret)
		pr_debug("[MET] ipi_register:%d failed:%d\n", IPIR_C_MET, ret);

	do {
		mtk_ipi_recv_reply(sspm_ipidev_symbol,IPIR_C_MET, (void *)&rdata, 1);
		if (sspm_recv_thread_comp == 1) {
			while (!kthread_should_stop())
				;
			return 0;
		}
		cmd = recv_buf[0] & MET_SUB_ID_MASK;
		switch (cmd) {
		case MET_DUMP_BUFFER:	/* mbox 1: start index; 2: size */
			if (widx < ridx) {	/* wrapping occurs */
				wlen = log_size - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)1);
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr),
							 widx * 4, log_done_cb, (void *)0);
			} else {
				wlen = widx - ridx;
				ondiemet_log_req_enq((char *)(ondiemet_sspm_log_virt_addr) + (ridx << 2),
				wlen * 4, log_done_cb, (void *)0);
			}
			break;
		case MET_CLOSE_FILE:	/* no argument */
			if (sspm_run_mode == SSPM_RUN_CONTINUOUS) {
				/* clear the memory */
				memset_io((void *)ondiemet_sspm_log_virt_addr, 0,
					  ondiemet_sspm_log_size);
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

#endif /* SSPM_VERSION_V2 */



void start_sspm_ipi_recv_thread(void)
{
#ifdef SSPM_VERSION_V2
	int ret;

	if(!sspm_ipidev_symbol)
		return;

	ret = mtk_ipi_register(sspm_ipidev_symbol, IPIS_C_MET, NULL, NULL, (void *) &ackdata);
	if (ret)
		pr_debug("[MET] ipi_register:%d failed:%d\n", IPIS_C_MET, ret);
#endif

	if (sspm_ipi_thread_started != 1) {
		sspm_recv_thread_comp = 0;
		ondiemet_sspm_recv_task =
		    kthread_run(ondiemet_sspm_recv_thread, NULL, "ondiemet_sspm_recv");
		if (IS_ERR(ondiemet_sspm_recv_task))
			pr_debug("MET: Can not create ondiemet_sspm_recv\n");
		else
			sspm_ipi_thread_started = 1;
	}
}

void stop_sspm_ipi_recv_thread(void)
{
	if (ondiemet_sspm_recv_task) {
		sspm_recv_thread_comp = 1;
#ifndef SSPM_VERSION_V2
		sspm_ipi_recv_complete(IPI_ID_TST1);
#endif
		kthread_stop(ondiemet_sspm_recv_task);
		ondiemet_sspm_recv_task = NULL;
		sspm_ipi_thread_started = 0;
#ifndef SSPM_VERSION_V2
		sspm_ipi_recv_unregistration(IPI_ID_TST1);
#else
		mtk_ipi_unregister(sspm_ipidev_symbol, IPIR_C_MET);
		mtk_ipi_unregister(sspm_ipidev_symbol, IPIS_C_MET);
#endif
	}
}

int met_ipi_to_sspm_command(void *buffer, int slot, unsigned int *retbuf, int retslot)
{
	int ret;

#ifndef SSPM_VERSION_V2
	ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, buffer, slot, retbuf, retslot);
#else
	if(!sspm_ipidev_symbol)
		ret = -1;
	else {
		ret = mtk_ipi_send_compl(sspm_ipidev_symbol, IPIS_C_MET, IPI_SEND_WAIT, buffer, slot, 2000);
		*retbuf = ackdata;
	}
#endif
	if (ret != 0)
		pr_debug("met_ipi_to_sspm_command error(%d)\n", ret);

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_sspm_command);


int met_ipi_to_sspm_command_async(void *buffer, int slot, unsigned int *retbuf, int retslot)
{
	int ret;

#ifndef SSPM_VERSION_V2
	ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, buffer, slot, retbuf, retslot);
#else
	if(!sspm_ipidev_symbol)
		ret = -1;
	else {
		ret = mtk_ipi_send(sspm_ipidev_symbol, IPIS_C_MET, IPI_SEND_WAIT, buffer, slot, 2000);
		*retbuf = ackdata;
	}
#endif
	if (ret != 0)
		pr_debug("met_ipi_to_sspm_command error(%d)\n", ret);

	return ret;
}
EXPORT_SYMBOL(met_ipi_to_sspm_command_async);



