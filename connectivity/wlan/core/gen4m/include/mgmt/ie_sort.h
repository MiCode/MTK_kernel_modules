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

uint8_t *sortBuildFragmentIE(uint8_t *dest_ie, uint8_t frag_eid,
	uint8_t *src, uint16_t src_size);

#endif /* !_IE_SORT_H */

