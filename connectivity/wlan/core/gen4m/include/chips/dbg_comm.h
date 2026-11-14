/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file  dbg_comm.h
 *    \brief This file contains the info of the dbg common function
 */


#ifndef _DBG_COMMON_H_
#define _DBG_COMMON_H_

void show_wfdma_interrupt_info(
	struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);

void show_wfdma_glo_info(
	struct ADAPTER *prAdapter,
	enum _ENUM_WFDMA_TYPE_T enum_wfdma_type);


#endif /* _DBG_COMMON_H_ */
