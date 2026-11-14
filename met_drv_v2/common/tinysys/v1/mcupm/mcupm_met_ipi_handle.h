/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __MUCPM_MET_IPI_HANDLE_H__
#define __MUCPM_MET_IPI_HANDLE_H__
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
void start_mcupm_ipi_recv_thread(void);
void stop_mcupm_ipi_recv_thread(void);

void mcupm_start(void);
void mcupm_stop(void);
void mcupm_extract(void);
void mcupm_flush(void);

int met_ipi_to_mcupm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);
int met_ipi_to_mcupm_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif  /* __MUCPM_MET_IPI_HANDLE_H__ */
