/*  SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */
#ifndef __SSPM_MET_IPI_HANDLE_H__
#define __SSPM_MET_IPI_HANDLE_H__
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
void start_sspm_ipi_recv_thread(void);
void stop_sspm_ipi_recv_thread(void);

void sspm_start(void);
void sspm_stop(void);
void sspm_extract(void);
void sspm_flush(void);

int met_scmi_to_sspm_command(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);
int met_scmi_to_sspm_command_async(
	unsigned int *buffer,
	int slot,
	unsigned int *retbuf,
	int retslot);


/*****************************************************************************
 * external variable declaration
 *****************************************************************************/


#endif				/* __SSPM_MET_IPI_HANDLE_H__ */
