/*
 * Copyright (C) 2018 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
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

static struct ipi_action ondiemet_sspm_isr;
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
		ret = sspm_ipi_send_sync(IPI_ID_MET, IPI_OPT_WAIT, (void *)ipi_buf, 0, &rdata, 1);
	}
}

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

void start_sspm_ipi_recv_thread(void)
{
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
		sspm_ipi_recv_complete(IPI_ID_TST1);
		kthread_stop(ondiemet_sspm_recv_task);
		ondiemet_sspm_recv_task = NULL;
		sspm_ipi_thread_started = 0;
		sspm_ipi_recv_unregistration(IPI_ID_TST1);
	}
}

