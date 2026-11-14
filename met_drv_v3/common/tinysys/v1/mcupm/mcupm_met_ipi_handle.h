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
int _alloc_all_ipi_thread_para(void);

void start_mcupm_ipi_recv_thread(int mcupm_no);
void stop_mcupm_ipi_recv_thread(int mcupm_no);

void mcupm_start(void);
void mcupm_stop(void);
void mcupm_extract(void);
void mcupm_flush(void);

int met_ipi_to_mcupm_command(
	unsigned int mcupm_no,
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);

int met_ipi_to_all_mcupm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);

int met_ipi_to_mcupm_command_async(
	unsigned int mcupm_no,
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif  /* __MUCPM_MET_IPI_HANDLE_H__ */
