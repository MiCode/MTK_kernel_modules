/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _NAN_INSTANT_COMM_MODE_H
#define _NAN_INSTANT_COMM_MODE_H

/*******************************************************************************
 *                         C O M P I L E R   F L A G S
 *******************************************************************************
 */
#if CFG_SUPPORT_NAN

/*******************************************************************************
 *                    E X T E R N A L   R E F E R E N C E S
 *******************************************************************************
 */
#include "nan_base.h"

extern uint8_t g_ucNanIsOn;
extern union _NAN_BAND_CHNL_CTRL g_r2gDwChnl;
extern union _NAN_BAND_CHNL_CTRL g_r5gDwChnl;

/*******************************************************************************
 *                              C O N S T A N T S
 *******************************************************************************
 */

/*******************************************************************************
 *                             D A T A   T Y P E S
 *******************************************************************************
 */

u_int8_t
nanIsInstantCommModeSupport(struct ADAPTER *prAdapter);

void
nanEnableInstantCommMode(struct ADAPTER *prAdapter, u_int8_t fgEnable);

void nanInstantCommModeOnHandler(struct ADAPTER *prAdapter);

void nanInstantCommModeBackToNormal(struct ADAPTER *prAdapter);

#endif
#endif

