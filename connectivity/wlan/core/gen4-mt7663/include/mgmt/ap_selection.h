/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

#ifndef _AP_SELECTION_H
#define _AP_SELECTION_H

/* Support AP Selection */
struct BSS_DESC *scanSearchBssDescByScoreForAis(struct ADAPTER *prAdapter);
void scanGetCurrentEssChnlList(struct ADAPTER *prAdapter);
/* end Support AP Selection */

#endif
