/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __GPUEB_MET_IPI_HANDLE_H__
#define __GPUEB_MET_IPI_HANDLE_H__
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
void start_gpueb_ipi_recv_thread(void);
void stop_gpueb_ipi_recv_thread(void);

void gpueb_start(void);
void gpueb_stop(void);
void gpueb_extract(void);
void gpueb_flush(void);

int met_ipi_to_gpueb_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);
int met_ipi_to_gpueb_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif  /* __GPUEB_MET_IPI_HANDLE_H__ */
