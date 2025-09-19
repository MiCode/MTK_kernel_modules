/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc.
 */

/*! \file   "ie_sort.h"
 *  \brief
 */

#ifndef _IE_SORT_H
#define _IE_SORT_H

int sortMsduPayloadOffset(struct ADAPTER *prAdapter,
		    struct MSDU_INFO *prMsduInfo);
int sortGetPayloadOffset(struct ADAPTER *prAdapter,
		    uint8_t *pucFrame);

void sortMgmtFrameIE(struct ADAPTER *prAdapter,
		    struct MSDU_INFO *prMsduInfo);

#endif /* !_IE_SORT_H */

