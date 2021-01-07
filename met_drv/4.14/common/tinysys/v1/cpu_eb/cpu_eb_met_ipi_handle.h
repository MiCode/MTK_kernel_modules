/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __CPU_EB_MET_IPI_HANDLE_H__
#define __CPU_EB_MET_IPI_HANDLE_H__
/*****************************************************************************
 * headers
 *****************************************************************************/


/*****************************************************************************
 * define declaration
 *****************************************************************************/


/*****************************************************************************
 * struct & enum declaration
 *****************************************************************************/


/*****************************************************************************
 * external function declaration
 *****************************************************************************/
void start_cpu_eb_ipi_recv_thread(void);
void stop_cpu_eb_ipi_recv_thread(void);

void cpu_eb_start(void);
void cpu_eb_stop(void);
void cpu_eb_extract(void);
void cpu_eb_flush(void);

int met_ipi_to_cpu_eb_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);
int met_ipi_to_cpu_eb_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif  /* __CPU_EB_MET_IPI_HANDLE_H__ */
